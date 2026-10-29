// ST7789 240x240 显示 114514 + 板载 WS2812 绿灯
// 引脚/SPI 频率/颜色序全部来自 platformio.ini 的 build_flags
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel rgb(1, 46, NEO_GRB + NEO_KHZ800);

TFT_eSPI    tft = TFT_eSPI();
TFT_eSprite fb  = TFT_eSprite(&tft);   // 整屏离屏帧缓冲：画完整帧再一次翻页

void setup() {
  Serial.begin(115200);
  delay(300);                          // 等 USB 串口就绪

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);          // 开背光

  tft.init();
  tft.setRotation(0);                  // 竖屏

  fb.setColorDepth(16);
  fb.createSprite(240, 240);

  // ---- 本帧全部画进离屏缓冲 ----
  fb.fillSprite(TFT_BLACK);
  fb.setTextDatum(MC_DATUM);                   // 中心对齐，锚点(120,110)
  fb.setTextColor(TFT_GREEN, TFT_BLACK);
  fb.drawString("114514", 120, 110, 7);        // 7 号字体 = 48px 七段数码风格
  fb.setTextColor(TFT_DARKGREY, TFT_BLACK);
  fb.drawString("ESP32-S3 N16R8", 120, 190, 2);

  // ---- 整帧翻页 ----
  fb.pushSprite(0, 0);

  rgb.begin();
  rgb.setBrightness(20);
  rgb.setPixelColor(0, rgb.Color(0, 20, 0));   // 绿灯 = 运行中
  rgb.show();

  Serial.printf("114514 on screen. PSRAM: %s (%u bytes)\n",
                psramFound() ? "yes" : "no", ESP.getPsramSize());
}

void loop() {
  delay(1000);
}
