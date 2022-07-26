#include "gp_neopixel.h"

NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> strip(NEOPIXEL_COUNT, NEOPIXEL_PIN);

RgbColor BLACK  = RgbColor(0, 0, 0);
RgbColor RED    = RgbColor(255, 0, 0);
RgbColor GREEN  = RgbColor(0, 255, 0);
RgbColor BLUE   = RgbColor(0, 0, 255);
RgbColor CYAN   = RgbColor(0, 180, 255);
RgbColor YELLOW = RgbColor(255, 255, 0);
RgbColor PURPLE = RgbColor(180, 0, 255);
RgbColor GRAY   = RgbColor(100, 100, 100);
RgbColor WHITE  = RgbColor(255, 255, 255);

RgbColor idle_color = PURPLE;

// Colors don't really work if value is < 100

void gp_neopixel_init() {
  strip.Begin();
  strip.SetBrightness(10);
}

void gp_neopixel_set(uint8_t index, RgbColor color) {
  strip.SetPixelColor(index, color);
}

void gp_neopixel_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  strip.SetPixelColor(index, RgbColor(r, g, b));
}

void gp_neopixel_set_all(RgbColor color) {
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}

void gp_neopixel_show() {
  strip.Show();
}
