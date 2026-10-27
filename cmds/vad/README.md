# Ameba Audio

## Table of Contents
- [Ameba Audio](#ameba-audio)
   - [Table of Contents](#table-of-contents)
   - [About](#about)
   - [Supported IC](#supported-ic)
   - [Architecture](#architecture)
   - [AIVoice Integration](#aivoice-integration)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)
   - [Command Options](#command-options)
   - [Expected Log Output](#expected-log-output)

## About

Ameba vad command drives a **two-stage** wake-up pipeline on CA32 (AP):

1. CA32 initialises hardware VAD (AMIC) and arms `VADBT_OR_VADPC` as AP wake source.
2. CA32 creates one **aivoice full_flow instance** (AFE + KWS + VAD + ASR all-in-one).
3. System enters sleep via `AT+TICKPS=R`. **`AudioRecord` / SPORT / GDMA are powered down** —
   they cannot capture PCM in CG4 sleep. Only the hardware VAD ADC keeps writing 16 kHz / 16-bit
   mono samples into the **VAD SRAM ring buffer** (`0x20020000-0x2003FFFF`, four 32 KB blocks
   ≈ 1 s each; the active block — `VAD_BLOCK_A`/`B`/`C`/`D` — is selected by menuconfig).
4. User speaks -> hardware VAD ISR fires, wakes CA32 and gives a semaphore.
5. Worker task runs the two stages in order:
   - **Stage 1 — VAD SRAM buffer ➜ aivoice AFE + KWS (per-frame feed).**
   - **Stage 2 — `AudioRecord` ➜ aivoice ASR.**
6. Worker re-arms hardware VAD (clears the pitch-detect latch, restores ADC routing,
   `VADMEM` clock back to `CKSL_VADM_VAD`) and lets the system sleep again.

> Why two stages? In CG4 sleep `AudioRecord` cannot capture PCM at all — only the VAD SRAM
> buffer is alive. Trying to detect the wake word through `AudioRecord` would mean opening
> it on every false trigger and missing the leading phoneme during codec setup. Reading the
> SRAM ring instead lets us evaluate the keyword on data that was actually recorded *during*
> sleep, and only pay the cost of bringing up `AudioRecord` once we know the user really said
> "你好小强".

## Supported IC

1. AmebaSmart (RTL8730E, CA32)

## Architecture

`vad.c` is a thin demo on top of the **AudioVad framework + HAL**. All hardware
VAD interaction (codec / AMIC bring-up, IRQ + wake semaphore, SRAM ring readback
and re-arm) lives in the framework + HAL — the demo only orchestrates aivoice
and the `AudioRecord` stage.

The demo flow:

```
AudioVad_Init(&cfg) → AudioVad_Start()                       (singleton; arm HW VAD)
loop:
  AudioVad_WaitWakeup(0xFFFFFFFF)                            (block until ISR)
  for n in 0..stage1_frames:                                 (Stage 1, per AFE frame)
    AudioVad_ReadLookback(buf, 512 * VAD_AFE_MIC_NUM)        (1st call: ~400 ms pre-roll
                                                              from hit_addr; then walks
                                                              forward, busy-waits live)
    aivoice.feed(buf, 512 * VAD_AFE_MIC_NUM)                 (interleaved frame → AFE+KWS;
                                                              on hit → break)
  if kws_hit:
    AudioVad_PrepareForRecord()                              (switch ADC clock)
    AudioRecord(channels=VAD_AFE_MIC_NUM) → feed aivoice     (Stage 2 ASR, interleaved)
    AudioRecord_Stop / Destroy
  AudioVad_Rearm()                                           (restore + sleep again)
AudioVad_Stop() → AudioVad_Deinit()                          (no Create/Destroy — singleton)
```

## Multi-channel VAD SRAM buffering

The wake decision always comes from a **single source** (codec 0,
`VAD_Codec_Select(0)`), but the VAD hardware can buffer up to **four codecs in
parallel** into SRAM blocks A/B/C/D while the AP sleeps. The framework exposes
this through `AudioVadConfig.channel_count` (range 1..4, 0 ⇒ 1):

The demo's channel count is **not a CLI option** — it is fixed at build time by
the menuconfig AFE resource (`AI Config → Select AFE Resource`), because the
aivoice `afe_config.mic_array` must match the linked AFE library. `vad.c` derives
`VAD_AFE_MIC_NUM` from the selected `CONFIG_AFE_RES_*` and uses it everywhere:

| menuconfig AFE resource | `CONFIG_AFE_RES_*` | `VAD_AFE_MIC_NUM` | AFE config macro |
|---|---|---|---|
| afe_res_1mic | `AFE_RES_1MIC` | 1 | `AFE_CONFIG_ASR_DEFAULT_1MIC` |
| afe_res_2mic30mm | `AFE_RES_2MIC30MM` | 2 | `AFE_CONFIG_ASR_DEFAULT_2MIC30MM` |
| afe_res_2mic50mm | `AFE_RES_2MIC50MM` | 2 | `AFE_CONFIG_ASR_DEFAULT_2MIC50MM` |
| afe_res_2mic70mm | `AFE_RES_2MIC70MM` | 2 | `AFE_CONFIG_ASR_DEFAULT_2MIC70MM` |
| afe_res_3mic50mm | `AFE_RES_CIRCLE3MIC50MM` | 3 | `AFE_CONFIG_ASR_DEFAULT_3MIC` |

## AIVoice Integration

`vad.c` integrates `aivoice_iface_full_flow_v1` when `CONFIG_AIVOICE_EN` is set. If aivoice is
disabled at build time, the file falls back to no-op stubs so it still compiles.

### Wake word

| Keyword | phoneme string | KWS resource required |
|---|---|---|
| 你好小强 (fallback) | `ni-hao-xiao-qiang` | Default `kws_xiaoqiangxiaoqiang_nihaoxiaoqiang_v4_300K` |

The fallback keyword ensures the demo wakes on hardware that ships with the default KWS resource.

### Callback events handled

| Event | Action |
|---|---|
| `AIVOICE_EVOUT_WAKEUP` | Sets `s_kws_woke = 1`; logs wake-word JSON |
| `AIVOICE_EVOUT_ASR_RESULT` | Sets `s_asr_session_done = 1`; logs ASR result JSON |
| `AIVOICE_EVOUT_ASR_REC_TIMEOUT` | Sets `s_asr_session_done = 1`; logs timeout |
| `AIVOICE_EVOUT_VAD` | Logs VAD status / offset (informational) |

## How to Build

### 1. menuconfig

```
CONFIG APPLICATION  --->
   Audio Config  --->
      CONFIG AUDIO CMD  --->
         [*] vad

   AI Config  --->
      [*] Enable TFLITE MICRO
      [*] Enable AIVoice
            Select AFE Resource (afe_res_1mic)  --->
            Select KWS Resource (kws_xiaoqiangxiaoqiang_nihaoxiaoqiang_v4_300K*)  --->   <- for 你好小强
            Select ASR Resource (asr_cn_v8_2M)  --->
```

### 2. usrcfg
```
struct PSCFG_TypeDef ps_config = {
    .km0_audio_vad_on = TRUE,
    .keep_osc4m_on = TRUE,             /* keep OSC4M off or on during sleep */
};
```

> **Note**: The CA32 `CA32_BL3_DRAM_NS` linker region must be large enough to hold the aivoice
> model libraries (AFE + KWS + ASR totals >2 MB). Expand the region in
> `component/soc/amebasmart/project/ameba_layout.ld` if the linker reports an
> overflow. Diff may be as follows for example:(may need hardware boards with more flash and memory.)

```
-#define PSRAM_END                                      (0x60800000)
+#define PSRAM_END                                      (0x640000000)

-CA32_BL3_DRAM_NS (rwx) :                ORIGIN = 0x60300000, LENGTH = 0x60700000 - 0x60300000   /* CA32 BL3 DRAM NS: 4MB */
-KM4_DRAM_HEAP_EXT (rwx) :               ORIGIN = 0x60700000, LENGTH = PSRAM_END - 0x60700000    /* KM4 PSRAM HEAP EXT: 1MB, (PSRAM Die is 8MB) */
+CA32_BL3_DRAM_NS (rwx) :                ORIGIN = 0x60300000, LENGTH = 0x60900000 - 0x60300000   /* CA32 BL3 DRAM NS: 4MB */
+KM4_DRAM_HEAP_EXT (rwx) :               ORIGIN = 0x60900000, LENGTH = PSRAM_END - 0x60900000    /* KM4 PSRAM HEAP EXT: 1MB, (PSRAM Die is 8MB) */

```

### 3. Build

Refer to the online documentation to compile and download images.

## How to Run

1. `Download` images to the board using Ameba Image Tool.

2. Show help:
   ```
   vad -h
   ```

3. Reference test sequence:
   ```
   AT+TICKPS=TYPE,CG       (only CG4 is supported)
   vad                     (init VAD + aivoice on CA32, start listener)
   AT+TICKPS=R             (enter sleep; speak the wake word when ready)
   ```

4. Run with default parameters (16 kHz, 16-bit, 8 s window, AMIC; channel count
   from the menuconfig AFE resource):
   ```
   vad
   ```

5. Run with a longer stage 1 budget (e.g. 96 frames ≈ 1536 ms):
   ```
   vad -n 96
   ```

## Command Options

```
vad [OPTION...]
    -h, --help              show this help message
    -r, --rate              record sample rate (default: 16000)
    -f, --format            record bits 16/24/32 (default: 16)
    -d, --duration          max recording seconds per wake-up window (default: 8)
    -m, --mic               mic source: 0=AMIC (PA_30, AMIC1), 1=DMIC (PB_22/PB_21, DMIC0), 2=I2S (default: 0)
    -n, --frames            stage1 max # of 16 ms AFE frames (default: 100 ≈ 100 * 16 ms)
        --amic-index        AMIC index 1..4 (default: 1)
        --dmic-index        DMIC index, 0-based (default: 0)
    -V, --verbose           verbose log
```

## Expected Log Output

### Successful wake-word + ASR

```
[Vad] vad demo begin, free heap:XXXXXX
[Vad] aivoice ready, kws=["ni-hao-xiao-qiang"] afe_mics=1
[Vad] vad armed [AMIC mic_index=1 vad_channels=1], run AT+TICKPS=R to enter sleep

[Vad] [CA32] woke up by VAD, hit_addr=0x2003XXXX
[Vad] [CA32] stage1 kws begin
[aivoice] wakeup: {"id":0,"keyword":"ni-hao-xiao-qiang","score":0.85}
[Vad] stage1: mics=1 fed=N/100 frames (~Xms) hit_addr=0x2003XXXX kws_hit=1
[Vad] [CA32] stage2 asr begin
[aivoice] asr result: {"type":0,"commands":[{"rec":"增大风速","id":28}]}
[Vad] stage2 end: asr_done=1 fed=XXXXX bytes
[Vad] [CA32] vad re-armed, waiting for next utterance
```

> With a multi-mic AFE resource selected in menuconfig the channel-count fields
> track it automatically, e.g. `afe_mics=2` / `vad_channels=2` / `mics=2` for a
> 2-mic resource, `=3` for `afe_res_3mic50mm`.
