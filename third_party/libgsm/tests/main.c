#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include "gsm.h"
#include <stdint.h>



#define MP3BUFFER_SIZE 2804
#define WAV_TO_FILE 1

#define INPUT_FRAME_SIZE 1500

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1


int16_t gsm_buf[INPUT_FRAME_SIZE];
short WAV_Buf[MP3BUFFER_SIZE];


int	f_decode   = 0;		/* decode rather than encode	 (-d) */
int f_cat	   = 0;		/* write to stdout; implies -p   (-c) */
int	f_force	   = 0;		/* don't ask about replacements  (-f) */
int	f_precious = 0;		/* avoid deletion of original	 (-p) */
int	f_fast	   = 0;		/* use faster fpt algorithm	 (-F) */
int	f_verbose  = 0;		/* debugging			 (-V) */
int	f_ltp_cut  = 0;		/* LTP cut-off margin	      	 (-C) */


struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int main(int argc, const char *argv[])
{
    char* gsmPath = NULL;
    char* wavPath = NULL;
    if (argc < 2) {
        printf("args not ready, try : *.mp3\n");
        return -1;
    }

    if (strncmp(argv[1], "-l", 3) == 0) {
        printf("usage: %s <src-gsm-file>: [<tgt-wav-file>]", argv[0], argv[1]);
        return 1;
    } else {
        gsmPath = argv[1];
        wavPath = argv[2];
    }

    if (gsmPath == NULL) {
        printf("args not ready, gsmPath is NULL.\n");
        return -1;
    }

    gsm mGsm;
    gsm_frame frameBuf;
    gsm_signal outBuf[ 160 ];

    int drv_num = 0;
    int frame_size = 0;
    uint32_t read_length = 0;
    FILE* gsmFile;
    FILE* wavFile;
    uint32_t bytes_left;
    uint32_t file_size;
    int offset = 0;
    printf("emter audio_play_gsm ......\n");

    //Open source file
    gsmFile = fopen(gsmPath, "rb"); // open read only file
    if (gsmFile == NULL) {
        printf("Open source file %s fail.\n", gsmPath);
        return -1;
    }
    fseek(gsmFile, 0, SEEK_SET);
    if (wavPath != NULL) {
        printf("Create or open PCM file...\n");
        /* Create or open PCM file */
        wavFile = fopen(wavPath, "wb");
        if (wavFile == NULL) {
            printf("fail to open %s: %s\n", wavPath, strerror(errno));
            goto exit;
        }
#if WAV_TO_FILE
        fseek(wavFile, sizeof(struct wav_header), SEEK_SET);
#endif
    }
    struct stat st;
    if (stat(gsmPath, &st) != 0) {
        printf("failed to stat \"%s\": %s\n", gsmPath, strerror(errno));
        goto exit;
    }

    file_size = st.st_size;
    bytes_left = file_size;
    printf("File size is %d\n", file_size);

    mGsm = gsm_create();
    if (!mGsm) {
        printf("mp3 context create fail\n");
        goto exit;
    }

    int msopt = 1;
    //gsm_option(mGsm, GSM_OPT_WAV49, &msopt);

    int cc = 0;
    unsigned int pcmSize = 0;
    do {
        /* Read a block */
        cc = fread(frameBuf, 1, sizeof(frameBuf), gsmFile);
        //printf("mp3 read br=%d\n", br);
        if (cc != sizeof(frameBuf)) {
            if (cc >= 0) {
                printf("incomplete frame (%d byte%s missing)\n",
                    (int)(sizeof(frameBuf) - cc), "frameBuf" + (sizeof(frameBuf) - cc == 1));
            };
            gsm_destroy(mGsm);
            return -1;
        }
        if (gsm_decode(mGsm, frameBuf, outBuf)) {
            printf("bad frame\n");
            gsm_destroy(mGsm);
            return -1;
        }

        bytes_left -= sizeof(frameBuf);
        //printf("file_size size %d, bytes_left %d\n", file_size, bytes_left);
        fwrite((char*) outBuf, 1, sizeof(outBuf), wavFile);
        fseek(gsmFile, file_size-bytes_left, SEEK_SET);
        pcmSize += sizeof(outBuf);
        if (bytes_left <= 0) {
            break;
        }
    } while (1);

#if WAV_TO_FILE
    if (wavFile != NULL) {
        struct wav_header* header = (struct wav_header *)malloc(sizeof(struct wav_header));
        memset(header, 0, sizeof(struct wav_header));
        header->riff_id = ID_RIFF;
        header->riff_fmt = ID_WAVE;
        header->fmt_id = ID_FMT;
        header->fmt_sz = 16;
        header->audio_format = 1;
        header->num_channels = 2;//TODO: set 2 channels
        header->sample_rate = 48000;
        header->bits_per_sample = 16;//mp3player.bitsPerSample;
        header->byte_rate = (header->bits_per_sample / 8) * header->num_channels * header->sample_rate;
        header->block_align = header->num_channels * (header->bits_per_sample / 8);
        header->data_id = ID_DATA;
        header->data_sz = pcmSize;
        header->riff_sz = header->data_sz + sizeof(header) - 8;

        printf("pcmSize: %d\n", pcmSize);
        fseek(wavFile, 0, SEEK_SET);
        fwrite((const char *)header, sizeof (struct wav_header), 1, wavFile);
        fflush(wavFile);
    }
#endif
    //printf("channels %d, sample_rate %d\n", info.channels, info.sample_rate);
    printf("PCM done\n");

exit:
    // close source file
    fclose(gsmFile);
    fclose(wavFile);
}

