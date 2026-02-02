# Ameba PCRecord

## Table of Contents
- [Ameba PCRecord](#ameba-pcrecord)
   - [Table of Contents](#table-of-contents)
   - [About ](#about)
   - [Supported IC](#supported-ic)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)

## About
This example support audio record and playback.

## Supported IC
1. AmebaSmart
2. AmebaLite

## How to Build
1. **GCC:** cd amebaxxx_gcc_project and run `./menuconfig.py` to enable configs.
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     pcrecord
   ```

2. **GCC:** cd amebaxxx_gcc_project and run `./build.py` to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.
2. Connect uart adapter with PC. TX: PA_25, RX: PA_24, baudrate: 1500000.
3. Default payload size is 2048 bytes, make sure settings on RtkAudioRecordTool.exe stay the same as demo.
4. Run RtkAudioTestToolv1.0.1/RtkAudioTestTool.exe on PC, and start to record.