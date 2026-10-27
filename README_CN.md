<div align="center">

<img src=".github/assets/audio.png" alt="ameba-audio 音频框架" width="800">

# ameba-audio

**瑞昱 Ameba 系列芯片的音频框架 —— 播放、采集、编解码、音效与语音。**

[![SDK](https://badgen.net/badge/SDK/FreeRTOS/blue)](https://aiot.realmcu.com/zh/latest/rtos/index.html)
[![Component](https://badgen.net/badge/component/audio/orange)](https://github.com/Ameba-AIoT/ameba-rtos)
[![Language](https://badgen.net/badge/language/C%20%2F%20C++/blue)](https://github.com/Ameba-AIoT/ameba-rtos)
[![License](https://badgen.net/badge/License/Apache%202.0/lightgrey)](LICENSE)

[English](README.md) · [中文版](README_CN.md) · [文档](https://aiot.realmcu.com/zh/latest/rtos/index.html) · [ameba-rtos](https://github.com/Ameba-AIoT/ameba-rtos)

</div>

ameba-audio 是瑞昱官方为 Ameba 系列 SoC 打造的音频中间件。它提供一套分层的软件栈 —— 从流式 API（`AudioTrack` / `AudioRecord`）向下贯通音频 HAL 及 Codec/功放驱动 —— 并集成媒体播放器、硬件音效、USB 与蓝牙 A2DP 音频设备，以及语音活动检测（VAD）。它以 [ameba-rtos](https://github.com/Ameba-AIoT/ameba-rtos) 子模块（submodule）的形式发布，属于 **XDK（扩展版）** 下载内容。

## 🔌 支持的芯片

| 芯片                    |          master          |     release/v1.2         |     release/v1.1         |     release/v1.0         |
|:---------------------- |:------------------------:|:------------------------:|:------------------------:|:------------------------:|
| AmebaSmart (RTL8730E)  | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   |
| AmebaLite (RTL8720E)   | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   |
| AmebaDplus (RTL8721Dx) | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   | ![alt text][supported]   |
| AmebaGreen2            | ![alt text][supported]   | ![alt text][not-support] | ![alt text][not-support] | ![alt text][not-support] |

[supported]: https://img.shields.io/badge/-%E6%94%AF%E6%8C%81-green "supported"
[not-support]: https://img.shields.io/badge/-%E4%B8%8D%E6%94%AF%E6%8C%81-red "not support"

> 各芯片的功能可用性以对应目标的 Kconfig 为准。完整的逐芯片功能矩阵请查阅在线 SDK 文档。

## 🏗️ 仓库结构

```text
ameba-audio/
├── interfaces/          # 公开头文件 —— 对外 API 层
│   ├── audio/           #   AudioTrack、AudioRecord、Manager、Equalizer、VAD、类型定义
│   ├── media/           #   MediaPlayer、流数据源、parcel
│   ├── hardware/        #   面向 HAL 的硬件接口
│   ├── usb_audio/       #   USB 音频（UAC）管理器
│   └── common/          #   共享类型与错误码
├── audio_hal/           # 硬件抽象层（各 SoC + 通用）
│   ├── amebasmart/      #   AmebaSmart (RTL8730E) HAL
│   ├── amebalite/       #   AmebaLite (RTL8720E) HAL
│   ├── amebadplus/      #   AmebaDplus (RTL8721Dx) HAL
│   ├── amebagreen2/     #   AmebaGreen2 HAL
│   ├── a2dp/            #   蓝牙 A2DP 音频设备
│   ├── usb/             #   USB 音频设备
│   └── common/          #   流缓冲、参数处理、调试
├── audio_driver/        # Codec / 功放驱动（如 HT513、dummy amp）
├── media_registry/      # demuxer 与 codec 的运行时注册表
├── base/                # 基础工具：cutils、log、osal、xlib
├── foundation/          # AHandler / ALooper / AMessage 消息框架
├── configs/             # 音频策略与编译期硬件配置
├── cmds/                # 控制台命令：aplay、arecord、player、equalizer、vad …
├── examples/            # 可运行示例（HAL render/capture、manager、speexdsp …）
├── third_party/         # 编解码库：haac、flac、libopus、tremolo、libgsm、speexdsp
├── libs/                # 各 SoC 预编译二进制
├── audio_prebuilts/     # 各 SoC 预编译二进制
├── Kconfig              # menuconfig 配置项
└── CMakeLists.txt       # 组件构建入口
```

## ✨ 主要特性

- **播放** — 流式音频输出，支持音量、延迟与时间戳查询；线程安全的同步 API
- **采集** — 流式音频输入，采样率、声道数与格式可配置
- **双架构** — 可选 **Mixer**（多流软件混音）或 **PassThrough**（经由 `AudioManager` audio patch 的低开销直连路由）
- **媒体播放器** — 文件 / 流播放，支持可插拔 demuxer 与 codec：**WAV、MP3、AAC、M4A、FLAC、AMR、OGG（Vorbis / Opus）**
- **音效** — 均衡器（equalizer）、动态范围控制（drc）等
- **蓝牙 A2DP** — A2DP sink / source 音频设备集成
- **USB 音频** — UAC 设备支持
- **功放驱动** — 可插拔的外部功放支持（如 HT513）
- **控制台命令** — `aplay`、`arecord`、`player`、`pcrecord`、`equalizer`、`equalizer_tune`，用于快速调试与测试

## 📚 相关文档

最新版文档请访问：[FreeRTOS SDK 及使用指南](https://aiot.realmcu.com/zh/latest/rtos/index.html) —— 涵盖构建系统、逐 SoC 指南及完整的音频应用笔记。

- API 参考：[`interfaces/`](interfaces) 目录下带注释的公开头文件
- 逐 SoC HAL 说明：`audio_hal/<soc>/README.md`

更多关于 Ameba 系列芯片的信息，请访问[官方产品页面](https://aiot.realmcu.com/zh/product/index.html)。

## 📥 SDK 下载

ameba-audio 以 [ameba-rtos](https://github.com/Ameba-AIoT/ameba-rtos) 子模块的形式发布，仅在 **XDK（扩展版）** 检出时才会拉取：

```bash
git clone --recurse-submodules https://github.com/Ameba-AIoT/ameba-rtos.git
```

若已克隆 ameba-rtos 但未包含 submodule：

```bash
git submodule update --init --recursive component/audio/ameba-audio
```

> 基础 SDK 检出不包含 `component/audio/ameba-audio`。编译任何音频功能都必须使用上述 XDK 检出方式。

## 🚀 快速开始

针对某个受支持的 SoC 配置、编译并烧录（在 ameba-rtos 根目录下执行）：

```bash
source env.sh                 # 配置工具链（Linux；Windows 请用 env.bat）
ameba.py soc AmebaSmart       # 例如：AmebaSmart / AmebaLite / AmebaDplus / AmebaGreen2
ameba.py menuconfig           # → Audio Config
ameba.py build
ameba.py flash -p <PORT> -b <BAUDRATE> -i <BIN_FILE> <START_ADDR> <END_ADDR>
ameba.py monitor -p <PORT> -b 1500000
```

关键 menuconfig 配置项（详见 `Kconfig`）：

- **Enable Audio Framework** → 选择 **Mixer** 或 **PassThrough**
- **Audio Devices** → 蓝牙 A2DP、USB
- **Enable Media Player** → 选择媒体格式（WAV / MP3 / AAC / M4A / FLAC / AMR / OGG）
- **Audio Commands** → 编译 `cmds/` 下的控制台命令

硬件接线（功放、PLL 与 XTAL 时钟选择、端口引脚等）在各 SoC 的
`component/soc/usrcfg/<soc>/include/ameba_audio_hw_usrcfg.h` 中配置。

### 试用控制台命令

在 menuconfig 中开启 **Audio Config → CONFIG AUDIO CMD**，然后从串口控制台运行以下命令。

**播放正弦音** — [`aplay`](cmds/aplay/README.md)：

```bash
aplay -r 48000 -c 2 -f 16 -d 20 -v 0.8    # 48 kHz 立体声 16-bit，20 秒，音量 0.8
aplay -h                                   # 显示全部选项
```

**从麦克风录音** — [`arecord`](cmds/arecord/README.md)：

```bash
arecord -r 48000 -c 2 -f 16               # 48 kHz 立体声 16-bit 采集，回放至扬声器
```

**播放媒体文件** — [`player`](cmds/player/README.md)（MP3 / AAC / FLAC / OGG …）：

```bash
player -F http://192.168.31.226/1.mp3     # 播放一个 URL
player -F buffer -md 1                     # 从内存数据源播放
```

## 🌐 使用 Gitee 加速

对于可以访问 [Gitee](https://gitee.com) 的用户，当发现从 GitHub 下载仓库过慢时，建议使用 [ameba-rtos](https://gitee.com/ameba-aiot/ameba-rtos) 的 Gitee 镜像以提升下载速度。ameba-audio 子模块的拉取方式与之相同。

## 💬 反馈

- 问题与建议：[Real-AIOT 论坛](https://forum.real-aiot.com/)
- 错误报告与新功能需求：[ameba-rtos GitHub Issues](https://github.com/Ameba-AIoT/ameba-rtos/issues)（请先搜索已有 issue）
