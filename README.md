# ESP32S3 — ST7789 240×240 双缓冲显示 + 板载 WS2812

基于 **PlatformIO + Arduino 框架** 的 ESP32-S3（N16R8）工程：ST7789 SPI 屏做整屏双缓冲刷新，屏幕中央显示 `114514`，板载 WS2812 状态灯常绿。

## 硬件

| 项目 | 规格 |
|---|---|
| 主控 | ESP32-S3，**N16R8**（16MB Qio Flash + 8MB OPI PSRAM） |
| 板载 RGB 灯 | 1 颗 WS2812，数据脚 **GPIO46** |
| 屏幕 | ST7789 IPS **240×240**，SPI 接口，无 MISO、无触摸 |
| 串口 | CH343 USB-UART（开发时占用 COM7） |

### 接线

| 屏幕引脚 | ESP32-S3 GPIO |
|---|---|
| SCK（时钟） | 12 |
| SDA/MOSI（数据） | 11 |
| RST（复位） | 15 |
| DC（数据/命令） | 14 |
| CS（片选） | 10 |
| BLK（背光） | 16 |

## 功能

- **硬件 SPI**：走 ESP32-S3 的 GPSPI2 总线，80 MHz 时钟；整帧 240×240×2B ≈ 110KB，推送约 12 ms（上限 60–80 fps）。
- **整屏双缓冲**：绘制全部在 `TFT_eSprite` 离屏缓冲完成，`pushSprite()` 一次性整帧翻页，无闪烁、无撕裂。
- **PSRAM 全量启用**：8MB OPI PSRAM 可识别（`ESP.getPsramSize()` 返回约 8.38 MB）。

## 编译与烧录

```bash
# 依赖：PlatformIO Core（pio）
pio run                      # 编译
pio run -t upload            # 编译并烧录（自动选口）
pio device list              # 列出串口，确认板子在哪个 COM 口
pio run -t monitor           # 串口监视（115200），Ctrl+C 退出
```

多串口设备时显式指定端口：

```bash
pio run -t upload --upload-port COM7
```

烧录成功且运行正常时，串口（115200）输出：

```
114514 on screen. PSRAM: yes (8386263 bytes)
```

## 关键配置说明

全部在 `platformio.ini` 中通过 `build_flags` 注入，**不修改库内的 `User_Setup.h`**（改库文件在 PlatformIO 下不可复现，升级即丢）。

### N16R8 内存布局（缺一项 PSRAM 就用不了）

```ini
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.psram_type = opi
board_build.partitions = default_16MB.csv
```

### ⚠️ ESP32-S3 上 TFT_eSPI 必须加 `-DUSE_HSPI_PORT`

这个坑很反直觉，且与 ESP32 经典款结论**相反**：

- Arduino S3 核心里 `FSPI` 的枚举值是 **1**，而 IDF 的 `REG_SPI_BASE(i)` 在 `i < 2` 时直接返回 **0**；
- TFT_eSPI 默认 `SPI_PORT = FSPI`，于是 SPI 寄存器宏被算成地址 `0x10` 附近；
- 结果：`tft.init()` 第一次写寄存器就 `StoreProhibited`，板子陷入开机崩溃循环（回溯指向 `TFT_eSPI::begin_tft_write`，EXCVADDR=0x10）；
- 加 `-DUSE_HSPI_PORT` 后 `SPI_PORT = HSPI = 2`，寄存器正确落在 GPSPI2，一切正常。

这与库官方 `Setup70h_ESP32_S3_GC9A01`（同样是 S3、同样是 10/11/12 引脚）的配置一致。

### 其它要点

| 配置 | 说明 |
|---|---|
| `-DCGRAM_OFFSET=1` | 240×240 面板必须，否则画面偏移 80 像素 |
| `-DTFT_RGB_ORDER=TFT_BGR` | 红蓝互换时在 `TFT_BGR` / `TFT_RGB` 间切换 |
| `-DTFT_INVERSION_ON` | 画面像负片/发白时改为 `-DTFT_INVERSION_OFF` |
| `-DSPI_FREQUENCY=80000000` | 花屏先从 40 MHz 试起，再降 20 MHz |
| `-DTFT_MISO=-1` | 本屏无 MISO；开机日志里的 `spiAttachMISO(): HSPI Does not have default pins` 属无害提示 |

## 代码结构

```
src/main.cpp       # 双缓冲绘制 + WS2812 状态灯
platformio.ini     # 全部硬件与构建配置
```

核心绘制模式：

```cpp
TFT_eSprite fb = TFT_eSprite(&tft);
fb.setColorDepth(16);
fb.createSprite(240, 240);   // 整屏离屏帧缓冲，112.5KB

// 每帧
fb.fillSprite(TFT_BLACK);    // 只画 fb，绝不直接画 tft
fb.drawString("114514", 120, 110, 7);
fb.pushSprite(0, 0);         // 整帧翻页
```

## 可用 GPIO（本工程占用之外）

被占用：`10 11 12`（SPI）、`14 15 16`（DC/RST/BL）、`46`（WS2812）。
不可用：`26–32`（Flash）、`33–37`（OPI PSRAM）、`19/20`（USB）、`43/44`（UART0）。
空闲：`1–9`、`13`、`17`、`18`、`21`、`38–42`、`47`、`48`（`0/3/45` 是 strapping 脚，注意启动电平）。

## License

MIT
