/*
 * Copyright (c) 2026 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from Realtek
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Spectrum analyzer test command.
 *
 * IMPORTANT: the spectrum analyzer taps the FINAL mixed PCM of the primary
 * output (speaker) thread; it does NOT generate sound itself. You must start
 * playback FIRST (e.g. `aplay some.wav`) and, while it is still playing, run
 * this `spectrum` command. It attaches to the primary output session, pulls the
 * frequency-domain (FFT) data periodically, folds it into log-spaced frequency
 * bands and prints the per-band power in dB so a host script can parse the log
 * and draw the spectrum.
 *
 * Typical session on the serial console:
 *      aplay -c 2 -r 48000 -f 16 xxx.wav      # start playback first
 *      spectrum -b 10 -d 15 -i 500            # then analyze while it plays
 *
 * How the FFT is obtained mirrors component/audio/audio_test/spectrum, but here
 * it goes through the high-level AudioSpectrum_* interface instead of the raw
 * AirHarmony_Spectrum* algorithm API.
 */

#define LOG_TAG "Spectrum"

#include <math.h>

#include "ameba.h"
#include "os_wrapper.h"

#include "audio/audio_service.h"
#include "audio/audio_spectrum.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

#define MAX_SPECTRUM_INSTANCES 1
#define SPECTRUM_MAX_BANDS     20

typedef struct {
    int bands;        /* number of frequency bands to report: 10 or 20 */
    int duration;     /* total analysis time, in seconds */
    int interval_ms;  /* time between two FFT reads / prints, in ms */
    int rate;         /* sampling rate hint for band mapping, in Hz */
} spectrum_params_t;

static const spectrum_params_t SPECTRUM_DEFAULT_PARAMS = {
    .bands       = 10,
    .duration    = 15,
    .interval_ms = 500,
    .rate        = 48000,
};

static spectrum_params_t s_params[MAX_SPECTRUM_INSTANCES];
static int s_cfg_cnt = 0;

/* Log-spaced band center frequencies, identical to the reference test. */
static const float BAND_10_FREQ[10] = {
    31.25f, 62.5f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};
static const float BAND_20_FREQ[20] = {
    31.25f, 45.0f, 63.0f, 90.0f, 125.0f, 180.0f, 250.0f, 355.0f, 500.0f, 710.0f,
    1000.0f, 1400.0f, 2000.0f, 2800.0f, 4000.0f, 5600.0f, 8000.0f, 11200.0f, 16000.0f, 20000.0f
};

/*
 * For each band center frequency, find the FFT bins just below and just above
 * it. Adapted from component/audio/audio_test/spectrum/audio_fft_analysis.c,
 * with the FFT size passed in rather than hard-coded.
 */
static void spectrum_map_bands(const float *bin_freqs, int nbins, const float *band_freqs,
                               int band_count, int *lower_idx, int *upper_idx)
{
    for (int i = 0; i < band_count; ++i) {
        float target = band_freqs[i];
        int lower = -1, upper = -1;
        float min_up = bin_freqs[nbins - 1];
        float min_low = bin_freqs[nbins - 1];
        for (int j = 0; j < nbins; ++j) {
            float diff = bin_freqs[j] - target;
            if (diff == 0.0f) {
                lower = j;
                upper = j;
                break;
            } else if (diff > 0.0f && diff < min_up) {
                min_up = diff;
                upper = j;
            } else if (diff < 0.0f && -diff < min_low) {
                min_low = -diff;
                lower = j;
            }
        }
        if (lower == -1 && upper != -1) {
            lower = upper - 1;
        }
        if (upper == -1 && lower != -1) {
            upper = lower + 1;
        }
        if (lower < 0) {
            lower = 0;
        }
        if (upper < 0) {
            upper = 0;
        }
        lower_idx[i] = lower;
        upper_idx[i] = upper;
    }
}

/*
 * Convert the interleaved complex FFT (re, im per bin) into per-band power in
 * dB. Same math as calculate_power_per_band() in the reference test.
 */
static void spectrum_power_per_band(const float *fft, int fft_size, const int *lower_idx,
                                    const int *upper_idx, int band_count, float *power_db)
{
    float norm = (float)(fft_size / 2) * (float)(fft_size / 2) * 2.0f;
    for (int i = 0; i < band_count; ++i) {
        int lo = lower_idx[i] * 2;
        int hi = upper_idx[i] * 2;
        float mag_lo = fft[lo] * fft[lo] + fft[lo + 1] * fft[lo + 1];
        float mag_hi = fft[hi] * fft[hi] + fft[hi + 1] * fft[hi + 1];
        float avg = (mag_lo + mag_hi) / norm;
        if (!isfinite(avg) || avg <= 0.0f) {
            avg = 1e-12f;   /* floor to avoid log10(0) / non-finite garbage */
        }
        power_db[i] = 10.0f * log10f(avg);
    }
}

static void SpectrumTask(void *param)
{
    spectrum_params_t *p = (spectrum_params_t *)param;
    struct AudioSpectrum *spectrum = NULL;
    float *fft = NULL;
    float *bin_freqs = NULL;
    const float *band_freqs = NULL;
    int lower_idx[SPECTRUM_MAX_BANDS];
    int upper_idx[SPECTRUM_MAX_BANDS];
    float power_db[SPECTRUM_MAX_BANDS];
    char line[256];

    int bands = (p->bands == 20) ? 20 : 10;
    band_freqs = (bands == 20) ? BAND_20_FREQ : BAND_10_FREQ;

    AudioService_Init();

    spectrum = AudioSpectrum_Create();
    if (!spectrum) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioSpectrum_Create failed\n");
        goto exit;
    }

    if (AudioSpectrum_Init(spectrum, 0, AUDIO_SPECTRUM_SESSION_OUTPUT_PRIMARY) != AUDIO_OK) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioSpectrum_Init failed, is aplay running?\n");
        goto exit;
    }

    int cap = AudioSpectrum_GetCaptureSize(spectrum);   /* == FFT size */
    int rate = AudioSpectrum_GetSamplingRate(spectrum);
    if (cap <= 0) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "invalid capture size:%d\n", cap);
        goto exit;
    }
    if (rate <= 0) {
        rate = p->rate;   /* fall back to the hint if the stream rate is unknown */
    }

    int nbins = cap / 2 + 1;                 /* number of usable FFT bins */
    int float_count = cap + 2;               /* interleaved re/im pairs */

    fft = (float *)malloc(sizeof(float) * float_count);
    bin_freqs = (float *)malloc(sizeof(float) * nbins);
    if (!fft || !bin_freqs) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "failed to malloc spectrum buffers\n");
        goto exit;
    }

    for (int i = 0; i < nbins; ++i) {
        bin_freqs[i] = (float)i * ((float)rate / (float)cap);
    }
    spectrum_map_bands(bin_freqs, nbins, band_freqs, bands, lower_idx, upper_idx);

    AudioSpectrum_SetEnabled(spectrum, true);

    /* Metadata line, parsed once by the host script. */
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "[SPEC] META rate=%d fft_size=%d nbins=%d bands=%d\n",
             rate, cap, nbins, bands);

    /* Band center frequencies for the x axis. */
    {
        int off = snprintf(line, sizeof(line), "[SPEC] FREQ ");
        for (int i = 0; i < bands && off < (int)sizeof(line); ++i) {
            off += snprintf(line + off, sizeof(line) - off, "%s%d",
                            (i == 0) ? "" : ",", (int)(band_freqs[i] + 0.5f));
        }
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "%s\n", line);
    }

    int frames = (p->duration * 1000) / p->interval_ms;
    if (frames <= 0) {
        frames = 1;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
             "spectrum start: %d bands, %d frames, every %d ms (play sound with aplay now)\n",
             bands, frames, p->interval_ms);

    for (int f = 0; f < frames; ++f) {
        rtos_time_delay_ms(p->interval_ms);

        if (AudioSpectrum_GetFft(spectrum, fft, float_count) != AUDIO_OK) {
            RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "GetFft failed at frame %d\n", f);
            continue;
        }

        spectrum_power_per_band(fft, cap, lower_idx, upper_idx, bands, power_db);

        int off = snprintf(line, sizeof(line), "[SPEC] FRAME %d BAND_DB ", f);
        for (int i = 0; i < bands && off < (int)sizeof(line); ++i) {
            off += snprintf(line + off, sizeof(line) - off, "%s%.2f",
                            (i == 0) ? "" : ",", power_db[i]);
        }
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "%s\n", line);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "[SPEC] END\n");

exit:
    if (spectrum) {
        AudioSpectrum_SetEnabled(spectrum, false);
        AudioSpectrum_Destroy(spectrum);
    }
    if (fft) {
        free(fft);
    }
    if (bin_freqs) {
        free(bin_freqs);
    }

    if (s_cfg_cnt > 0) {
        s_cfg_cnt--;
    }
    rtos_task_delete(NULL);
}

static cmd_parse_entry_t s_spectrum_entries[] = {
    {"-b",         NULL, SPECTRUM_DEFAULT_PARAMS.bands},
    {"--bands",    NULL, SPECTRUM_DEFAULT_PARAMS.bands},
    {"-d",         NULL, SPECTRUM_DEFAULT_PARAMS.duration},
    {"--duration", NULL, SPECTRUM_DEFAULT_PARAMS.duration},
    {"-i",         NULL, SPECTRUM_DEFAULT_PARAMS.interval_ms},
    {"--interval", NULL, SPECTRUM_DEFAULT_PARAMS.interval_ms},
    {"-r",         NULL, SPECTRUM_DEFAULT_PARAMS.rate},
    {"--rate",     NULL, SPECTRUM_DEFAULT_PARAMS.rate},
};

static void spectrum_help(void)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "spectrum [OPTION...] \n"
        "\tStart playback with aplay FIRST, then run this while it plays.\n"
        "\t-h, --help              show this help message\n");

    int entry_count = sizeof(s_spectrum_entries) / sizeof(s_spectrum_entries[0]);
    for (int i = 0; i < entry_count; i++) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "\t%-25s (default: %d)\n",
            s_spectrum_entries[i].arg, s_spectrum_entries[i].default_value);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
        "\nExamples (run AFTER `aplay xxx.wav` has started):\n"
        "\tspectrum\n"
        "\tspectrum -b 20 -d 20 -i 500\n");
}

static void parse_spectrum_params(cmd_params_t *params, spectrum_params_t *p)
{
    *p = SPECTRUM_DEFAULT_PARAMS;

    cmd_parse_entry_t entries[] = {
        {"-b",         &p->bands,       SPECTRUM_DEFAULT_PARAMS.bands},
        {"--bands",    &p->bands,       SPECTRUM_DEFAULT_PARAMS.bands},
        {"-d",         &p->duration,    SPECTRUM_DEFAULT_PARAMS.duration},
        {"--duration", &p->duration,    SPECTRUM_DEFAULT_PARAMS.duration},
        {"-i",         &p->interval_ms, SPECTRUM_DEFAULT_PARAMS.interval_ms},
        {"--interval", &p->interval_ms, SPECTRUM_DEFAULT_PARAMS.interval_ms},
        {"-r",         &p->rate,        SPECTRUM_DEFAULT_PARAMS.rate},
        {"--rate",     &p->rate,        SPECTRUM_DEFAULT_PARAMS.rate},
    };

    int entry_count = sizeof(entries) / sizeof(entries[0]);
    cmd_parse_all_int(params, entries, entry_count);

    if (p->interval_ms <= 0) {
        p->interval_ms = SPECTRUM_DEFAULT_PARAMS.interval_ms;
    }
}

static uint32_t spectrum_handler(cmd_params_t *params)
{
    if (params->argc > 1 &&
        (strcmp(params->argv[1], "-h") == 0 || strcmp(params->argv[1], "--help") == 0)) {
        spectrum_help();
        return TRUE;
    }

    if (s_cfg_cnt >= MAX_SPECTRUM_INSTANCES) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "support less than %d instances \n", MAX_SPECTRUM_INSTANCES);
        return FALSE;
    }

    parse_spectrum_params(params, &s_params[s_cfg_cnt]);

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "create spectrum example \n");

    rtos_task_t spectrum_task;
    if (rtos_task_create(&spectrum_task, "SpectrumTask",
                        SpectrumTask, &s_params[s_cfg_cnt],
                        8192, 5) != RTK_SUCCESS) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "error: rtos_task_create(SpectrumTask) failed \n");
        return FALSE;
    }

    s_cfg_cnt++;

    return TRUE;
}

DEFINE_CMD_WRAPPER(spectrum, spectrum_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE spectrum_cmd_table[] = {
    {
        "spectrum", spectrum_cmd_thread
    },
};
