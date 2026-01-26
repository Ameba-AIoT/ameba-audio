# Ameba Audio

## Table of Contents
- [Ameba Audio](#ameba-audio)
   - [Table of Contents](#table-of-contents)
   - [About ](#about)
   - [Supported IC](#supported-ic)
   - [Configurations](#configurations)
      - [Hardware Configurations](#hardware-configurations)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)

## About
Ameba audio project can achieve:
1. audio record.
2. before using this example, please check the application note to see how to choose audio architecture and compile.
3. please check the application note to see how to use interfaces.

## Supported IC
1. AmebaSmart
2. AmebaLite
3. AmebaDplus(for mixer architecture, please set psram for img2 in menuconfig)

## Configurations
Please see discriptions in component/soc/usrcfg/xx/include/ameba_audio_hw_usrcfg.h, ameba_audio_hw_usrcfg.h is for audio hardware configurations.

### Hardware Configurations
1. Setup the hardware pins, like dmic pins and so on.
2. Define using pll clock or xtal clock for record.
3. Please refer to component/audio/audio_hal/xx/README.md.

## How to Build
1. **GCC:** cd amebaxxx_gcc_project and run `./menuconfig.py` to enable configs.
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     arecord
   ```

2. **GCC:** cd amebaxxx_gcc_project and run `./build.py` to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.
2. For record run command and parameters, please refer to arecord.c.