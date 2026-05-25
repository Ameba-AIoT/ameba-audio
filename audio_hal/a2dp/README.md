# Ameba Audio

## Table of Contents

- [Ameba Audio](#ameba-audio)
	- [Table of Contents](#table-of-contents)
	- [About](#about)
	- [Supported IC](#supported-ic)
	- [Configurations](#configurations)
	- [How to run](#how-to-run)

## About

Ameba audio project can achieve:
1. if a2dp connected, using chip as a2dp source, and play audio through a2dp.
2. if a2dp disconnected, play through speaker.

## Supported IC
1. AmebaSmart

## Configurations

Please see discriptions in component/audio/configs/include/ameba_audio_mixer_usrcfg.h, and setup component/audio/configs/ameba_audio_mixer_usrcfg.cpp.
Please see discriptions in component/audio/configs/include/ameba_audio_policy_configs.h, and setup component/audio/configs/audio_policy/ameba**/ameba_audio_policy_configs.c

Enter directory:amebasmart_gcc_project
./menuconfig.py--->CONFIG APPLICATION--->AUDIO CONFIG--->Select Audio Interface(Mixer)
./menuconfig.py--->CONFIG APPLICATION--->AUDIO CONFIG--->CONFIG AUDIO CMD--->aplay
./menuconfig.py--->CONFIG APPLICATION--->AUDIO CONFIG--->Audio Devices--->Bluetooth A2DP Device
./menuconfig.py--->CONFIG BT--->BT Example Demo--->BT A2DP

## How to run

Build and Download:
   * Refer to the SDK Examples section of the online documentation to generate images.
   * `Download` images to board by Ameba Image Tool.

### Note

1. One smart board as source, one smart board as slave, running same image.

2. After system boot, sink cmd:
   'AT+BTDEMO=a2dp,snk,1'

3. After system boot, source cmd:
   'AT+BTDEMO=a2dp,src,1'

4. After above steps, we can get bluetooth mac address through logs.

5. Source board connect to sink's bluetooth mac, suppose sink's mac is 00e04c800499
   'AT+BTA2DP=conn,00e04c800499'
   'AT+BTA2DP=start,00e04c800499'

6. Source board run:
   'aplay -r 16000 -c 2 -f 16'

7. After all above steps, we will here sine wave through sink board.

8. If user wants to disconnect a2dp, use following cmd, and sine will output from other connected device(speaker or uac...).
    AT+BRGAP=disc,00e04c800499

9. If user wants to test a2dp together with uac, user can run uac test together with this test.
    Then user will here a2dp's sine wave and uac's music mixed together and playing with a2dp device and uac device.

10. Notice: running uac and a2dp tests together takes 2.9M heap, please make sure memory is enough.
    adjust amebasmart_gcc_project/ameba_layout.ld if necessary.

11. ameba_audio_mixer_usrcfg.cpp config setting is very very important. If you have distortion, please change kMultipleCopiedBuffer value.




