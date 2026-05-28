/*
 * Copyright (c) 2021 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from Realtek
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TAG "Aplay"

#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include "ameba.h"
#include "os_wrapper.h"

#include "ameba_audio_mixer_usrcfg.h"
#include "audio/audio_control.h"
#include "audio/audio_track.h"
#include "audio/audio_service.h"
#include "audio/audio_equalizer.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

#define ID_RIFF                  0x46464952
#define ID_WAVE                  0x45564157
#define ID_FMT                   0x20746d66
#define ID_DATA                  0x61746164

#define APLAY_FILE_TYPE_DEFAULT -1
#define APLAY_FILE_TYPE_RAW      0
#define APLAY_FILE_TYPE_WAV      1
#define APLAY_FILE_TYPE_SINE     2

#define M_PI                     3.14159265358979323846
#define MAX_TRACKS               8

typedef struct {
    int   channels;
    int   rate;
    int   frames;
    int   bits;
    int   freq;
    int   gain;
    float vol;
    int   mute;
    int   speed;
    int   buf_multi;
    int   gen_cnt;
    int   device;
    int   file_type;
    int   quiet;
    int   verbose;
    int   duration;
    int   loop;
    int   eq;
    char  filename[64];
    int   wav_header_size;
} aplay_params_t;

static const aplay_params_t APLAY_DEFAULT_PARAMS = {
    .channels             = 2,
    .rate                 = 16000,
    .frames               = 480,
    .bits                 = 16,
    .freq                 = 1000,
    .gain                 = 0,
    .vol                  = 1.0f,
    .mute                 = 0,
    .speed                = 0,
    .buf_multi            = 4,
    .gen_cnt              = 0,
    .file_type            = APLAY_FILE_TYPE_DEFAULT,
    .quiet                = 0,
    .verbose              = 0,
    .duration             = 86400,
    .loop                 = 1,
    .eq                   = 0,
    .wav_header_size      = 0,
};
static aplay_params_t s_params[MAX_TRACKS];
static uint32_t s_cfg_cnt = 0;

typedef struct {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
} aplay_wav_riff_t;

typedef struct {
    uint32_t id;
    uint32_t sz;
} aplay_wav_chunk_t;

typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} aplay_wav_fmt_t;

typedef struct {
    int32_t fd;
    int     file_type;
    int     wav_header_size;
    int     wav_channels;
    int     wav_rate;
    int     wav_bits;
} aplay_file_context_t;
static aplay_file_context_t s_file_ctx;

static inline const char *aplay_get_file_type_name(int file_type)
{
    switch (file_type) {
    case APLAY_FILE_TYPE_RAW: return "raw";
    case APLAY_FILE_TYPE_WAV: return "wav";
    case APLAY_FILE_TYPE_SINE: return "sine";
    default:                   return "unknown";
    }
}

static inline int aplay_parse_file_type(const char *name)
{
    if (strcasecmp(name, "raw") == 0)  return APLAY_FILE_TYPE_RAW;
    if (strcasecmp(name, "wav") == 0)  return APLAY_FILE_TYPE_WAV;
    if (strcasecmp(name, "sine") == 0) return APLAY_FILE_TYPE_SINE;
    return APLAY_FILE_TYPE_DEFAULT;
}

static int aplay_file_parse_wav_header(const uint8_t *header, int header_size,
                                  int *channels, int *rate, int *bits, int *data_offset)
{
    if (header_size < 44) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "WAV header too small: %d\n", header_size);
        return -1;
    }

    aplay_wav_riff_t *riff = (aplay_wav_riff_t *)header;
    if (riff->riff_id != ID_RIFF || riff->wave_id != ID_WAVE) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "Not a valid WAV file\n");
        return -1;
    }

    int offset = 12;
    aplay_wav_chunk_t chunk;
    *data_offset = 0;

    while (offset + 8 <= header_size) {
        memcpy(&chunk, header + offset, sizeof(chunk));
        offset += 8;

        if (chunk.id == ID_FMT) {
            if ((size_t)offset + sizeof(aplay_wav_fmt_t) <= (size_t)header_size) {
                aplay_wav_fmt_t *fmt = (aplay_wav_fmt_t *)(header + offset);
                *channels = fmt->num_channels;
                *rate = fmt->sample_rate;
                *bits = fmt->bits_per_sample;
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "WAV: channels=%d, rate=%d, bits=%d\n",
                         *channels, *rate, *bits);
            }
        } else if (chunk.id == ID_DATA) {
            *data_offset = offset;
            break;
        }

        offset += chunk.sz;
    }

    if (*data_offset == 0) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "WAV data chunk not found\n");
        return -1;
    }

    return 0;
}

static void aplay_file_open(aplay_file_context_t *ctx, const char *filename, int file_type, int quiet)
{
    ctx->fd = 0;
    ctx->file_type = file_type;
    ctx->wav_header_size = 0;
    ctx->wav_channels = 2;
    ctx->wav_rate = 16000;
    ctx->wav_bits = 16;

    if (file_type == APLAY_FILE_TYPE_WAV) {
        if (!filename || filename[0] == 0) {
            if (!quiet) {
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: -t wav requires a filename\n");
            }
            return;
        }

        ctx->fd = (int32_t)fopen(filename, "r");
        if (ctx->fd <= 0) {
            if (!quiet) {
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: cannot open file: %s\n", filename);
            }
            ctx->fd = 0;
            return;
        }

        uint8_t header[256];
        int header_read = fread(header, 1, sizeof(header), (FILE *)ctx->fd);
        if (header_read > 44) {
            int channels = 0, rate = 0, bits = 0, data_offset = 0;
            if (aplay_file_parse_wav_header(header, header_read,
                                        &channels, &rate, &bits, &data_offset) == 0) {
                ctx->wav_header_size = data_offset;
                ctx->wav_channels = channels;
                ctx->wav_rate = rate;
                ctx->wav_bits = bits;
                if (!quiet) {
                    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "WAV parsed: header_size=%d, ch=%d, rate=%d, bits=%d\n",
                             ctx->wav_header_size, ctx->wav_channels, ctx->wav_rate, ctx->wav_bits);
                }
                fseek((FILE *)ctx->fd, ctx->wav_header_size, SEEK_SET);
            } else {
                if (!quiet) {
                    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: invalid WAV file: %s\n", filename);
                }
                fclose((FILE *)ctx->fd);
                ctx->fd = 0;
                return;
            }
        }
    } else if (file_type == APLAY_FILE_TYPE_SINE || file_type == APLAY_FILE_TYPE_DEFAULT) {
        // sine wave - no file to open
    } else if (file_type == APLAY_FILE_TYPE_RAW) {
        if (!filename || filename[0] == 0) {
            if (!quiet) {
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: -t raw requires a filename\n");
            }
            return;
        }

        ctx->fd = (int32_t)fopen(filename, "r");
        if (ctx->fd <= 0) {
            if (!quiet) {
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: cannot open file: %s\n", filename);
            }
            ctx->fd = 0;
            return;
        }

        fseek((FILE *)ctx->fd, 0L, SEEK_END);
        int length = ftell((FILE *)ctx->fd);
        if (!quiet) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "RAW file length: %d \n", length);
        }
        fseek((FILE *)ctx->fd, 0, SEEK_SET);
    } else if (filename && filename[0] != 0) {
        ctx->fd = (int32_t)fopen(filename, "r");
        if (ctx->fd <= 0) {
            if (!quiet) {
                RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: cannot open file: %s\n", filename);
            }
            ctx->fd = 0;
            return;
        }

        fseek((FILE *)ctx->fd, 0L, SEEK_END);
        int length = ftell((FILE *)ctx->fd);
        if (!quiet) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "file length:%d \n", length);
        }
        fseek((FILE *)ctx->fd, 0, SEEK_SET);
    }
}

static void aplay_file_close(aplay_file_context_t *ctx)
{
    if (ctx->fd > 0) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "close file. \n");
        fclose((FILE *)ctx->fd);
        ctx->fd = 0;
    }
}

static int32_t aplay_file_read(aplay_file_context_t *ctx, void *buffer, int32_t size)
{
    int32_t bytes_read = 0;
    if (ctx->fd > 0) {
        bytes_read = fread(buffer, 1, size, (FILE *)ctx->fd);
    }
    return bytes_read;
}

static void aplay_sine_gen(int8_t *buffer, uint32_t freq, uint32_t count, uint32_t rate, uint32_t channels, uint32_t bits, double *_phase)
{
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase * freq / (double)rate;
    uint8_t *samples[channels];
    int32_t steps[channels];
    uint32_t chn;
    int32_t format_bits = bits;
    uint32_t maxval = (1 << (format_bits - 1)) - 1;
    int32_t bps = format_bits / 8;
    int32_t to_unsigned = 0;

    for (chn = 0; chn < channels; chn++) {
        steps[chn] = bps * channels;
        samples[chn] = (uint8_t *)(buffer + chn * bps);
    }

    while (count-- > 0) {
        int32_t res, i;
        res = sin(phase) * maxval;
        if (to_unsigned) {
            res ^= 1U << (format_bits - 1);
        }
        for (chn = 0; chn < channels; chn++) {
            {
                for (i = 0; i < bps; i++) {
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
                }
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase) {
            phase -= max_phase;
        }
    }
    *_phase = phase;
}

static int32_t aplay_cook_buffer(int8_t *sine_buf, aplay_params_t *param, double *phase, aplay_file_context_t *file_ctx)
{
    int32_t size = param->frames * param->channels  * param->bits / 8;
    int32_t cooked_size = size;

    if (param->file_type == APLAY_FILE_TYPE_SINE || param->file_type == APLAY_FILE_TYPE_DEFAULT) {
        aplay_sine_gen(sine_buf, param->freq, param->frames,
                        param->rate, param->channels, param->bits, phase);
        param->gen_cnt ++;
    } else if ((param->file_type == APLAY_FILE_TYPE_WAV || param->file_type == APLAY_FILE_TYPE_RAW) && file_ctx && file_ctx->fd > 0) {
        cooked_size = aplay_file_read(file_ctx, sine_buf, size);
    }

    return cooked_size;
}

static bool aplay_refine_buffer_bytes(aplay_params_t *param) {
    (void)param;
    bool refine = false;
#if defined(CONFIG_AUDIO_PASSTHROUGH)
    if (kEqVersion == SW_EQ_VERSION_1_0 && param->eq) {
        refine = true;
    }
#endif
    return refine;
}

static struct AudioEqualizer *audio_equalizer = NULL;

static void aplay_eq_create(void)
{
    if (audio_equalizer) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "eq already started \n");
        return;
    }

    int16_t band_filter_type = 0;
    int16_t band_level = 0;
    int32_t center_freq = 0;
    int32_t qfactor = 0;
    int16_t band_index = 0;

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "eq version:%d \n", kEqVersion);

    audio_equalizer = AudioEqualizer_Create();
    AudioEqualizer_Init(audio_equalizer, 0, 0);
    int16_t bands = AudioEqualizer_GetNumberOfBands(audio_equalizer);
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "total bands:%d \n", bands);

    int16_t *range = AudioEqualizer_GetBandLevelRange(audio_equalizer);
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "band range:(%d, %d) \n", *range, *(range + 1));
    free(range);
    range = NULL;

    bands = 10;
    AudioEqualizer_SetNumberOfBands(audio_equalizer, bands);

    // fc: 40hz, 90hz, 180hz...
    int32_t fc[10] = {40, 90, 180, 380, 760, 1000, 3020, 6010, 12010, 18010};
    // db: -2db, +4db, -1db...
    int32_t gain[10] = {-200, 400, -100, 300, -800, 800, -200, 300, -100, 200};
    // q: 0.7, 0.96, 0.6...
    int32_t qfactor_arr[10] = {70, 96, 60, 70, 60, 50, 50, 66, 40, 70};
    int32_t type[10] = {[0 ... 9] = AUDIO_EQUALIZER_TYPE_PEAKING};
    type[0] = AUDIO_EQUALIZER_TYPE_HIGHPASS;
    type[9] = AUDIO_EQUALIZER_TYPE_LOWPASS;

    for (int i = 0; i < bands; i++) {
        AudioEqualizer_SetCenterFreq(audio_equalizer, i, fc[i]);
        AudioEqualizer_SetBandLevel(audio_equalizer, i, gain[i]);
        AudioEqualizer_SetQfactor(audio_equalizer, i, qfactor_arr[i]);
        AudioEqualizer_SetBandFilterType(audio_equalizer, i, type[i]);
    }

    AudioEqualizer_SetEnabled(audio_equalizer, true);

    for (; band_index < bands; band_index++) {
        band_filter_type = AudioEqualizer_GetBandFilterType(audio_equalizer, band_index);
        band_level = AudioEqualizer_GetBandLevel(audio_equalizer, band_index);
        center_freq = AudioEqualizer_GetCenterFreq(audio_equalizer, band_index);
        qfactor = AudioEqualizer_GetQfactor(audio_equalizer, band_index);
        RTK_LOGA(TAG, "band:%d, filter:%d, center_freq:%d, qfactor:%d, level:%d\n",
                    band_index, band_filter_type, center_freq, qfactor, band_level);
    }
}

static void aplay_eq_destroy(void)
{
    AudioEqualizer_SetEnabled(audio_equalizer, false);
    AudioEqualizer_Destroy(audio_equalizer);
    audio_equalizer = NULL;
}

static uint32_t aplay_get_format_for_bits(uint32_t bits)
{
    switch (bits) {
    case 16: return AUDIO_FORMAT_PCM_16_BIT;
    case 24: return AUDIO_FORMAT_PCM_24_BIT;
    case 32: return AUDIO_FORMAT_PCM_32_BIT;
    default: return AUDIO_FORMAT_INVALID;
    }
}

static void play_sample(aplay_params_t *param, aplay_file_context_t *file_ctx)
{
    struct AudioTrack *aplay;

    uint64_t frames_written = 0;
    uint32_t frame_size = param->channels * param->bits / 8;
    int32_t track_buf_size = 4096;
    uint64_t play_frame_size = (uint64_t)param->rate * (uint64_t)param->duration;

    AudioTimestamp tstamp;
    uint32_t frames_played = 0;
    uint64_t frames_played_at_us = 0;
    int64_t now_us = 0;

    uint32_t sine_frames_count = param->frames;

    int8_t *sine_buf = malloc(sine_frames_count * frame_size);

    int32_t size = sine_frames_count * frame_size;
    double phase = 0;

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "play sample channels:%u, rate:%u,param->bits=%u,period_size=%u \n",
                                   param->channels, param->rate, param->bits, param->frames);

    aplay = AudioTrack_Create();
    if (!aplay) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: new AudioTrack failed \n");
        return;
    }

    track_buf_size = AudioTrack_GetMinBufferBytes(aplay, AUDIO_CATEGORY_MEDIA,
                     param->rate, aplay_get_format_for_bits(param->bits), param->channels) * param->buf_multi;

    if (aplay_refine_buffer_bytes(param)) {
        track_buf_size = size;
    }

    AudioTrackConfig  track_config;
    track_config.category_type = AUDIO_CATEGORY_MEDIA;
    track_config.sample_rate = param->rate;
    track_config.format = aplay_get_format_for_bits(param->bits);
    track_config.channel_count = param->channels;
    track_config.buffer_bytes = track_buf_size;

    AudioTrack_Init(aplay, &track_config, AUDIO_OUTPUT_FLAG_NONE);

    AudioTrack_SetVolume(aplay, 1.0, 1.0);
    AudioTrack_SetStartThresholdBytes(aplay, track_buf_size);
    AudioControl_SetHardwareVolume(param->vol, param->vol);

    AudioPlaybackRate speed;
    speed.speed = 2.0;
    speed.pitch = 1.0;

    if (AudioTrack_Start(aplay) != AUDIO_OK) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: aplay start fail \n");
        return;
    }

    if (param->eq) {
        //for passthrough, must create after track start.
        //for mixer, no limit.
        aplay_eq_create();
    }

    if (param->speed) {
        AudioTrack_SetPlaybackRate(aplay, speed);
    }

    while (1) {
        int32_t cooked_size = aplay_cook_buffer(sine_buf, param, &phase, file_ctx);
        if (cooked_size < size) {
            break;
        }

        AudioTrack_Write(aplay, (u8 *)sine_buf, size, true);

        now_us = rtos_time_get_current_system_time_ms() * 1000;
        if (AudioTrack_GetTimestamp(aplay, &tstamp) == AUDIO_OK) {
            if (param->verbose) {
                RTK_LOGA(TAG, "timestamp position:%lld, sec:%lld, nsec:%ld \n",
                           tstamp.position, tstamp.time.tv_sec, tstamp.time.tv_nsec);
            }
            frames_played = tstamp.position;
            frames_played_at_us = tstamp.time.tv_sec * 1000000LL + tstamp.time.tv_nsec / 1000;
        }

        frames_written += (uint64_t)(size / frame_size);
        if (frames_written >= play_frame_size) {
            RTK_LOGA(TAG, "frames_written:%llu, play_frame_size:%llu \n",
                           frames_written, play_frame_size);
            break;
        }
    }

    int64_t duration_us = frames_played * 1000000LL / param->rate + now_us - frames_played_at_us;
    int64_t frames_written_us = frames_written * 1000000LL / param->rate;
    uint32_t wait_ms = (frames_written_us - duration_us) / 1000;

    rtos_time_delay_ms(wait_ms);

    AudioTrack_Pause(aplay);
    AudioTrack_Flush(aplay);
    AudioTrack_Stop(aplay);
    AudioTrack_Destroy(aplay);

    if (sine_buf) {
        free(sine_buf);
    }

    if (param->eq) {
        aplay_eq_destroy();
    }

    aplay = NULL;
}

static void example_aplay_thread(void *param)
{
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "Aplay demo begin, free heap:%d\n",
                  rtos_mem_get_free_heap_size());
    aplay_params_t *params = (aplay_params_t *)param;

    memset(&s_file_ctx, 0, sizeof(s_file_ctx));

    if (params->filename[0] != '\0') {
        aplay_file_open(&s_file_ctx, params->filename, params->file_type, params->quiet);

        if (params->file_type == APLAY_FILE_TYPE_WAV && s_file_ctx.fd <= 0 && !params->quiet) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "warning: WAV file open failed, using sine wave instead\n");
        }
        if (params->file_type == APLAY_FILE_TYPE_WAV && s_file_ctx.fd > 0) {
            params->channels = s_file_ctx.wav_channels;
            params->rate = s_file_ctx.wav_rate;
            params->bits = s_file_ctx.wav_bits;
            params->wav_header_size = s_file_ctx.wav_header_size;
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "Using WAV params: ch=%d, rate=%d, bits=%d\n",
                     params->channels, params->rate, params->bits);
        }
    }

    AudioService_Init();

    play_sample(params, &s_file_ctx);
    rtos_time_delay_ms(2000);

    aplay_file_close(&s_file_ctx);

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "Aplay demo end, free heap:%d\n",
                  rtos_mem_get_free_heap_size());

    rtos_task_delete(NULL);
}

static void aplay_help(void)
{
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "aplay [OPTION...] [file]\n"
        "\t-h, --help              show this help message\n"
        "\t--version               show version\n"
        /* Lowercase short + long */
        "\t-c, --channels          number of channels (default: 2)\n"
        "\t-r, --rate              sample rate (default: 16000)\n"
        "\t-b, --buffer-frames     buffer frames (default: 480)\n"
        "\t-d, --duration          playback duration in seconds\n"
        "\t-e, --eq                enable eq\n"
        "\t-f, --format            sample format (bits, default: 16)\n"
        "\t-g, --gain              audio gain (default: 0)\n"
        "\t-l, --loop              loop playback count\n"
        "\t-m, --mute              mute audio output\n"
        "\t-q, --quiet             quiet mode\n"
        "\t-s, --speed             playback speed (default: 1.0)\n"
        "\t-t, --file-type         file type (raw, wav)\n"
        /* Uppercase short + long */
        "\t-F, --freq              sine wave frequency (default: 1000)\n"
        "\t-M, --buf-multi         buffer multiple\n"
        "\t-N, --gen-cnt           generated sample count\n"
        "\t-V, --verbose           verbose output\n"
        "\t-X, --eq_gain           eq gain\n"
        /* Float */
        "\t-v, --volume            volume (0.0-1.0, default: 1.0)\n"
        /* Long only */
        "\t--period-size           audio period size (default: 1024)\n"
        "\t--min-stage             audio minimum stage (default: 1)\n"
        "\nFile types:\n"
        "\twav    - WAV file\n"
        "\nExamples:\n"
        "\tif using vfs file, make sure vfs has such file in it\n"
        "\tuser can refer to online document to check how to use vfs\n"
        "\taplay -t wav vfs://dance.wav\n"
        "\taplay -r 48000 -t raw vfs://dance_48000_2ch_16bit.raw\n"
        "\taplay -r 48000 -c 2 -f 16 -d 20 -v 0.8\n");
}

static void parse_aplay_params(cmd_params_t *params, aplay_params_t *p)
{
    *p = APLAY_DEFAULT_PARAMS;

    cmd_parse_entry_t int_entries[] = {
        /* Lowercase short + long */
        {"-c",        &p->channels,        APLAY_DEFAULT_PARAMS.channels},
        {"--channels",&p->channels,        APLAY_DEFAULT_PARAMS.channels},
        {"-r",        &p->rate,            APLAY_DEFAULT_PARAMS.rate},
        {"--rate",    &p->rate,            APLAY_DEFAULT_PARAMS.rate},
        {"-b",        &p->frames,          APLAY_DEFAULT_PARAMS.frames},
        {"--buffer-frames", &p->frames,    APLAY_DEFAULT_PARAMS.frames},
        {"-d",        &p->duration,        APLAY_DEFAULT_PARAMS.duration},
        {"--duration",&p->duration,        APLAY_DEFAULT_PARAMS.duration},
        {"-e",        &p->eq,              APLAY_DEFAULT_PARAMS.eq},
        {"--eq",      &p->eq,              APLAY_DEFAULT_PARAMS.eq},
        {"-f",        &p->bits,            APLAY_DEFAULT_PARAMS.bits},
        {"--format",  &p->bits,            APLAY_DEFAULT_PARAMS.bits},
        {"-g",        &p->gain,            APLAY_DEFAULT_PARAMS.gain},
        {"--gain",    &p->gain,            APLAY_DEFAULT_PARAMS.gain},
        {"-l",        &p->loop,            APLAY_DEFAULT_PARAMS.loop},
        {"--loop",    &p->loop,            APLAY_DEFAULT_PARAMS.loop},
        {"-m",        &p->mute,            APLAY_DEFAULT_PARAMS.mute},
        {"--mute",    &p->mute,            APLAY_DEFAULT_PARAMS.mute},
        {"-q",        &p->quiet,           APLAY_DEFAULT_PARAMS.quiet},
        {"--quiet",   &p->quiet,           APLAY_DEFAULT_PARAMS.quiet},
        {"-s",        &p->speed,           APLAY_DEFAULT_PARAMS.speed},
        {"--speed",   &p->speed,           APLAY_DEFAULT_PARAMS.speed},
        {"-t",        &p->file_type,       APLAY_DEFAULT_PARAMS.file_type},
        {"--file-type", &p->file_type,     APLAY_DEFAULT_PARAMS.file_type},
        /* Uppercase short + long */
        {"-F",        &p->freq,            APLAY_DEFAULT_PARAMS.freq},
        {"--freq",    &p->freq,            APLAY_DEFAULT_PARAMS.freq},
        {"-M",        &p->buf_multi,       APLAY_DEFAULT_PARAMS.buf_multi},
        {"--buf-multi", &p->buf_multi,     APLAY_DEFAULT_PARAMS.buf_multi},
        {"-N",        &p->gen_cnt,         0},
        {"--gen-cnt", &p->gen_cnt,         0},
        {"-V",        &p->verbose,         APLAY_DEFAULT_PARAMS.verbose},
        {"--verbose", &p->verbose,         APLAY_DEFAULT_PARAMS.verbose},
    };

    cmd_parse_float_entry_t float_entries[] = {
        {"-v",        &p->vol,             APLAY_DEFAULT_PARAMS.vol},
        {"--volume",  &p->vol,             APLAY_DEFAULT_PARAMS.vol},
    };

    cmd_parse_all_int(params, int_entries, sizeof(int_entries) / sizeof(int_entries[0]));
    cmd_parse_all_float(params, float_entries, sizeof(float_entries) / sizeof(float_entries[0]));

    for (int i = 0; i < params->argc - 1; i++) {
        if (params->argv[i] && (strcmp(params->argv[i], "-t") == 0 || strcmp(params->argv[i], "--file-type") == 0)) {
            p->file_type = aplay_parse_file_type(params->argv[i + 1]);
        }
    }

    if ((p->file_type == APLAY_FILE_TYPE_WAV || p->file_type == APLAY_FILE_TYPE_RAW) && params->argc > 0) {
        const char *last_arg = params->argv[params->argc - 1];
        if (last_arg && last_arg[0] != '-') {
            strncpy(p->filename, last_arg, sizeof(p->filename) - 1);
            p->filename[sizeof(p->filename) - 1] = '\0';

            if (strstr(last_arg, ".wav") != NULL || strstr(last_arg, ".WAV") != NULL) {
                p->file_type = APLAY_FILE_TYPE_WAV;
            } else {
                p->file_type = APLAY_FILE_TYPE_RAW;
            }
        }
    }

    if (!p->quiet) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "file_type: %s (%d), filename: %s\n",
                  aplay_get_file_type_name(p->file_type), p->file_type, p->filename);
    }

    CMD_PARSE_INT(kEqVersion,                               "--version",        1);
    CMD_PARSE_INT(kPrimaryAudioConfig.out_period_frames,    "--period-size",    1024);
    CMD_PARSE_INT(kPrimaryAudioConfig.out_min_frames_stage, "--min-stage",      1);
}

static uint32_t aplay_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        aplay_help();
        return TRUE;
    }

    if (s_cfg_cnt >= MAX_TRACKS) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "support less than %d tracks \n", MAX_TRACKS);
        return FALSE;
    }

    parse_aplay_params(params, &s_params[s_cfg_cnt]);

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "create aplay example \n");

    rtos_task_t track_task;
    if (rtos_task_create(&track_task, "example_aplay_thread",
                        example_aplay_thread, &s_params[s_cfg_cnt],
                        1024 * 2, 1) != RTK_SUCCESS) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: rtos_task_create(example_aplay_thread) failed \n");
        return FALSE;
    }

    s_cfg_cnt++;

    return TRUE;
}

DEFINE_CMD_WRAPPER(aplay, aplay_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE aplay_cmd_table[] = {
    {
        "aplay", aplay_cmd_thread
    },
};
