#ifndef GP_NEOPIXEL_H
#define GP_NEOPIXEL_H

#include <NeoPixelBrightnessBus.h>

#define NEOPIXEL_PIN 2
#define BORDER_COUNT 24
#define INTERIOR_COUNT (1+2+3+4+5+6)
#define NEOPIXEL_COUNT (BORDER_COUNT + INTERIOR_COUNT)
#define TOGGLE_EVERY 2

extern NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> strip;

extern RgbColor BLACK;
extern RgbColor RED;
extern RgbColor GREEN;
extern RgbColor BLUE;
extern RgbColor CYAN;
extern RgbColor YELLOW;
extern RgbColor PURPLE;
extern RgbColor GRAY;
extern RgbColor WHITE;

extern RgbColor idle_color;

void gp_neopixel_init();
void gp_neopixel_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void gp_neopixel_set(uint8_t index, RgbColor color);
void gp_neopixel_set_all(RgbColor color);
void gp_neopixel_show();

#endif
