# Ameba Audio

## Table of Contents

- [Ameba Audio](#ameba-audio)
	- [Table of Contents](#table-of-contents)
	- [About ](#about-)
	- [Supported IC ](#supported-ic-)
	- [Configurations ](#configurations-)
		- [Hardware configurations](#hardware-configurations)
   - [How to Build](#how-to-build)
	- [How to run ](#how-to-run)
	- [Note ](#note)

## About <a name = "about"></a>

Ameba audio project can achieve:
1. audio eq uart setting.
2. only support mixer architecture now:
   ./menuconfig.py  ---->  CONFIG APPLICATION ----> Audio Config ----> Select Audio Interfaces (Mixer)
3. please check the application note to see how to use interfaces.

## Supported IC <a name = "supported-ic"></a>
1. AmebaSmart
2. AmebaLite

## Configurations <a name = "configurations"></a>

User can use default configurations when running on demo board. If user wants to change configuration:
Please see discriptions in component/soc/usrcfg/xx/include/ameba_audio_hw_usrcfg.h, ameba_audio_hw_usrcfg.h is for audio hardware configurations.
Please see discriptions in component/audio/configs/include/ameba_audio_mixer_usrcfg.h, and setup component/audio/configs/ameba_audio_mixer_usrcfg.cpp.

### Hardware configurations

User can use default configurations when running on demo board. If user wants to change configuration:
1. Setup the hardware pins, like amplifier pins and so on.
2. Define whether using pll clock or xtal clock for playback.
3. Please refer to component/audio/audio_hal/xx/README.md.

## How to Build
Build and Download:
1. Refer to the online documentation to do menuconfig:
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     equalizer_tune
   ```

2. Refer to the online documentation to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.

2. Connect uart with PC with pinmux defined in equalizer_tune.h

3. Run cmd:
   ```
   equalizer_tune
   ```

4. Run AudioConfigTool in PC.

### Note

1. #define USING_CMD 1 : using equalizer_tune cmds to start communicating with PC.
2. #define USING_CMD 0 : using app_example interface to start communicating with PC directly after system boot without any cmd.
4. Please change uart pinmux and uart index according to your board, for example:
   ```c
   #define AUDIO_UART_INDEX                1
   #define AUDIO_UART_TX_PIN               PB_11
   #define AUDIO_UART_RX_PIN               PB_10
   ```
3. If you want to hear the effect, you can also build aplay cmd.
   enter cmd: equalizer_tune (with #define USING_CMD 1) and wait a while for system to be ready.
   enter cmd: aplay -r 48000 -c 2 -f 16 (not necessary).
