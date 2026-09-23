# drc command

## Table of Contents
- [drc command](#drc-command)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
  - [Frame-size Requirement (IMPORTANT)](#frame-size-requirement-important)
  - [Supported IC](#supported-ic)
  - [Options](#options)
  - [How to Build](#how-to-build)
  - [How to Run](#how-to-run)
  - [Startup Order](#startup-order)

## About
The `drc` command drives the **AudioDrc_\* framework interface** on the
currently-active output effect chain. DRC sits between the equalizer and
the spectrum analyzer, so it modifies the same PCM the mixer/passthrough is
about to hand to the HAL.

1. It **does not** create an `AudioRecord` or `AudioTrack` of its own — it
   only attaches DRC to whatever audio is flowing.
2. The typical partner is `arecord`, whose mic → speaker loopback provides
   the AudioTrack that DRC attaches to. Enabling this menu automatically
   enables `arecord` in the same image (see Kconfig `CMD_DRC_MENU`).
3. DRC stays alive across cmd invocations — `drc` turns it on, the cmd task
   exits, and a later `drc --off` tears it down.

## Frame-size Requirement (IMPORTANT)

DRC processes audio in fixed blocks whose length must be a multiple of
**1 ms of audio** (48 frames at 48 kHz, 44 frames at 44.1 kHz). The audio
block size fed to DRC therefore has to be a whole-millisecond multiple, or
you will hear a periodic pop while DRC is on.

- **Mixer path** — the per-period block size is controlled by
  `--period-size`. Set it to a whole-millisecond multiple; for a 48 kHz
  output the recommended value is **1200** (= 25 ms). Both `aplay` and
  `arecord` accept `--period-size 1200` at runtime; the compile-time
  default lives in
  `component/audio/configs/ameba_audio_mixer_usrcfg.cpp`
  (`kPrimaryAudioConfig.out_period_frames`).

  Example:
  ```
  arecord -c 2 -r 48000 -f 16 --period-size 1200
  drc
  ```

- **Passthrough path** — DRC sees exactly the buffer size handed to each
  write, so the source command must write a whole-millisecond multiple of
  frames per call. For `arecord`, pick a `-b` (bytes per write) value such
  that `-b / (channels * bytes_per_sample)` is a whole-millisecond multiple
  of frames.

  Example (2 ch, 16-bit, 48 kHz, 25 ms chunks = 1200 frames = 4800 bytes):
  ```
  arecord -c 2 -r 48000 -f 16 -b 4800
  drc
  ```

## Supported IC
1. AmebaSmart
2. AmebaLite
3. AmebaDplus

## Options

| Option        | Default | Meaning                                            |
|---------------|---------|----------------------------------------------------|
| `--mode`      | `0`     | `0` = single-band DRC, `1` = three-band DRC        |
| `--off`       | `0`     | non-zero: disable + destroy the currently-active DRC |
| `-h`, `--help`|         | Show help                                          |

The compressor coefficients (knee points, attack/release, makeup gain, and
the multi-band crossover frequencies) are baked into this demo. Edit
`component/audio/cmds/drc/drc.c` (`DRC_CFG_SINGLE` / `DRC_CFG_MULTI`) to
change them, or drive the framework directly with `AudioDrc_SetConfig()` /
`AudioDrc_SetMultiBandsConfig()` from your application.

## How to Build
1. Refer to the online documentation to do menuconfig:
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     drc
   ```
   Enabling `drc` also auto-selects `arecord` (they are meant to be used
   together for this demo).

2. Refer to the online documentation to compile.

## How to Run
1. `Download` the image to the board.

2. Start the mic → speaker loopback with `arecord`:
   ```
   arecord -c 2 -r 48000 -f 16 --period-size 1200
   ```

3. Enable DRC:
   ```
   drc                # single-band DRC
   drc --mode 1       # three-band DRC
   ```
   DRC stays on after the cmd exits.

4. When you want to hear the stream without DRC, tear it down:
   ```
   drc --off 1
   ```
   `arecord` keeps playing (without DRC).

## Startup Order

Same rule as the equalizer / spectrum cmds:

- **Mixer path** — the primary speaker output is brought up during audio
  service init, before any playback starts, so `drc` attaches to it
  immediately whether or not an audio source is already running.
  **Any order works.**

- **Passthrough path** — the DRC effect only exists while an audio stream
  is active, so `drc` must be started **after** the audio source (e.g.
  `arecord` or `aplay`). If `drc` runs first it logs an error and exits;
  just retry it once the source is running.

Teardown side: call `drc --off` before stopping the audio source. The
reverse order (stopping the source first) is handled gracefully on the
passthrough path, but on the mixer path it is best to disable DRC before
tearing down audio.
