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
User can use default configurations when running on demo board. If user wants to change configuration:
1. Please see discriptions in component/soc/usrcfg/xx/include/ameba_audio_hw_usrcfg.h, ameba_audio_hw_usrcfg.h is for audio hardware configurations.
2. Please see discriptions in component/audio/configs/include/ameba_audio_mixer_usrcfg.h, and setup component/audio/configs/ameba_audio_mixer_usrcfg.cpp.
3. Please see discriptions in component/audio/configs/include/ameba_audio_policy_configs.h, and setup component/audio/configs/audio_policy/ameba**/ameba_audio_policy_configs.c.

### Hardware Configurations
User can use default configurations when running on demo board. If user wants to change configuration:
1. Setup the hardware pins, like amplifier pins and so on.
2. Define whether using pll clock or xtal clock for playback.
3. Please refer to component/audio/audio_hal/xx/README.md.

## How to Build
1. Refer to the online documentation to do menuconfig:
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     aplay
   ```

2. Refer to the online documentation to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.

2. Run cmd to check how to run aplay:
   ```
   aplay -h
   ```

3. Here's a simple demo of playing sine tone through speaker:
   ```
   aplay -r 48000 -c 2 -f 16 -d 20 -v 0.8
   ```

4. Run cmd to check how to run aplay:
   ```
   amixer -h
   ```

5. Here's a simple demo of changing volume with amixer:
   ```
   amixer -v 0.5
   ```