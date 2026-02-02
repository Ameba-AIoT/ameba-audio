# Ameba Player

## Table of Contents
- [Ameba Player](#ameba-player)
   - [Table of Contents](#table-of-contents)
   - [About ](#about)
   - [Supported IC](#supported-ic)
   - [Configurations](#configurations)
      - [Hardware configurations](#hardware-configurations)
      - [Software configurations](#software-configurations)
   - [How to Build](#how-to-build)
   - [How to Run](#how-to-run)

## About
Ameba player project can achieve:
1. audio playback.
2. MediaPlayer provides the APIs to implement operations related to manage playback, please check the application note to see how to use interfaces.
3. player.c gives an example of the detailed implementation of an MediaPlayer.

## Supported IC
1. AmebaLite
2. AmebaDplus
3. Amebagreen2
4. AmebaSmart CA32

## Configurations
### Hardware Configurations
Required Components: speaker
Connect the speaker to board.

### Software Configurations
If you want to use the functionality of caching files to flash
   **Modify** `player.c`
   ```
   #define USE_CACHE
   ```

## How to Build
1. **GCC:** cd amebaxxx_gcc_project and run `./menuconfig.py` to enable configs.
   ```
   CONFIG APPLICATION  --->
      Audio Config  --->
         CONFIG AUDIO CMD  --->
            [*]     player
   ```

2. **GCC:** cd amebaxxx_gcc_project and run `./build.py` to compile.

## How to Run
1. `Download` images to board by Ameba Image Tool.

2. **player [OPTION...]**
   ```
   [-F audio_path]          An audio file path to play
   [-md 0/1]                Use my data source flag
   ```

   Examples:
   ```
   1. play a http file:
      player -F http://192.168.31.226/1.mp3
   2. play my data source:
      player -F buffer -md 1
   ```

2. **Result description:**
   The corresponding music played in the speaker.