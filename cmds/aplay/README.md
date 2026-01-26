# Ameba Audio

## Table of Contents
- [Ameba Audio](#ameba-audio)
   - [Table of Contents](#table-of-contents)
   - [About](#about)
   - [Supported IC](#supported-ic)
   - [Configurations](#configurations)
      - [Hardware Configurations](#hardware-configurations)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)

## About
Ameba audio project can achieve:
1. audio playback.
2. before using this example, please check the application note to see how to choose audio architecture and compile.
3. please check the application note to see how to use interfaces.

## Supported IC
1. AmebaSmart
2. AmebaLite
3. AmebaDplus(for mixer architecture, please set menuconfig.py->Config Link Option->Img2 In Psram)
4. AmebaGreen2(for mixer architecture, please set menuconfig.py->Config Link Option->Img2 Code In Psram_DataHeapInSram)

## Configurations
Please see discriptions in component/soc/usrcfg/xx/include/ameba_audio_hw_usrcfg.h, ameba_audio_hw_usrcfg.h is for audio hardware configurations.
Please see discriptions in component/audio/configs/include/ameba_audio_mixer_usrcfg.h, and setup component/audio/configs/ameba_audio_mixer_usrcfg.cpp.
Please see discriptions in component/audio/configs/include/ameba_audio_policy_configs.h, and setup component/audio/configs/audio_policy/ameba**/ameba_audio_policy_configs.c

### Hardware Configurations
1. Setup the hardware pins, like amplifier pins and so on.
2. Define whether using pll clock or xtal clock for playback.
3. Please refer to component/audio/audio_hal/xx/README.md.

## How to Build
1. **GCC:** cd amebaxxx_gcc_project and run `./menuconfig.py` to enable configs.
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     aplay
   ```

2. **GCC:** cd amebaxxx_gcc_project and run `./build.py` to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.
2. For playing run command and parameters, please refer to aplay.c.