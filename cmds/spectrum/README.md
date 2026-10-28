# spectrum command

## Table of Contents
- [spectrum command](#spectrum-command)
   - [Table of Contents](#table-of-contents)
   - [About](#about)
   - [Supported IC](#supported-ic)
   - [Options](#options)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)
   - [Log Format](#log-format)
   - [Plotting on the Host](#plotting-on-the-host)

## About
The spectrum command analyzes the **final mixed PCM of the primary output
(speaker) path** and prints per-band frequency power in dB so it can be parsed
and plotted on a host.

1. It is a **read-only tap** on the primary output mixer — it does **not**
   produce any sound of its own.
2. It attaches to the primary output session, and whatever audio is being
   played at that moment is what it analyzes.
3. Playback must be started **first**: there must be audio flowing through the
   primary output when you run `spectrum`, otherwise every band reads the noise
   floor.

## Supported IC
1. AmebaSmart
2. AmebaLite(only km4)
3. AmebaDplus

## Options

| Option              | Default | Meaning                                          |
|---------------------|---------|--------------------------------------------------|
| `-b`, `--bands`     | `10`    | Number of log-spaced bands to report (`10`/`20`) |
| `-d`, `--duration`  | `15`    | Total capture time, seconds                      |
| `-i`, `--interval`  | `500`   | Time between two FFT reads/prints, ms            |
| `-r`, `--rate`      | `48000` | Sample-rate hint (used only if the stream rate is unknown) |
| `-h`, `--help`      |         | Show help                                        |

The FFT size is fixed by the analyzer (1024 by default), so the reported
sampling rate / FFT size / bin count come from the device and are printed in the
`[SPEC] META` line — the plot script reads them from the log, you don't have to
match them by hand.

## How to Build
1. Refer to the online documentation to do menuconfig:
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     spectrum
   ```

2. Refer to the online documentation to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.

2. Run cmd to check how to run spectrum:
   ```
   spectrum -h
   ```

3. The order matters — start playback FIRST and keep it running, then WHILE it
   is still playing, run the analyzer. On the device serial console:
   ```
   # 1) start playback first and keep it running
   aplay -c 2 -r 48000 -f 16

   # 2) WHILE it is still playing, run the analyzer
   spectrum -b 10 -d 15 -i 500
   ```

   `aplay` can also be run in a loop / with a long file so it keeps playing for
   the whole spectrum capture window.

## Log Format
All analyzer lines are prefixed with `[SPEC]`:

```
[SPEC] META rate=48000 fft_size=1024 nbins=513 bands=10
[SPEC] FREQ 31,63,125,250,500,1000,2000,4000,8000,16000
[SPEC] FRAME 0 BAND_DB -62.10,-58.44,-49.02,-41.55,...
[SPEC] FRAME 1 BAND_DB -61.88,-57.90,-48.71,...
...
[SPEC] END
```

- `META` — sampling rate, FFT size, usable bin count, number of bands.
- `FREQ` — band center frequencies in Hz (the x axis).
- `FRAME n BAND_DB ...` — the per-band power in dB for frame `n`.

## Plotting on the Host
1. Capture the console to a file (e.g. `log.txt`) and run:
   ```
   python parse_spectrum.py log.txt
   ```

   or：
   ```
   python parse_spectrum.py spectrum.txt --save spectrum.png --no-show
   ```

2. It draws a bar chart of the band power and a spectrogram (bands over time).
   The bar chart shows the **mean power averaged in the linear (power) domain**,
   not the raw dB numbers: dB is logarithmic, so averaging dB directly drags
   strong tones down (a 1000 Hz tone holding -3.7 dB would read ~-10 dB once
   quiet startup frames are mixed in). Use `--agg max` for peak hold,
   `--agg median` to ignore outlier frames, or `--frame N` for a single frame.
   See `parse_spectrum.py --help` for all options (`--save out.png`,
   `--frame N`, `--agg {mean,max,median}`, `--no-show`).
