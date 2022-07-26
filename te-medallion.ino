#include "gp_packet.h"
#include "gp_neopixel.h"
#include "gp_wifi.h"
#include "gp_udp.h"

#define BUTTON_PIN 0

bool network_sending_palettes;

int battery_level() {
  // Analog read level is 0-1023 (0V-1V).
  // The 1M & 220K voltage divider takes the max
  // LiPo value of 4.2V and drops it to 0.758V max.
  // This means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  #define MIN_BATT_RAW 580
  #define MAX_BATT_RAW 774
  int raw = analogRead(A0);
  int constrained = constrain(raw, MIN_BATT_RAW, MAX_BATT_RAW);
  // convert battery level to percent
  int level = map(raw, MIN_BATT_RAW, MAX_BATT_RAW, 0, 100);
  return level;
}

// Call this to reset the chip
void(* reset) (void) = 0;

void draw_mac(byte mac[6]) {
  RgbColor color;
  int i = 0;
  for (int mbyte = 5; mbyte >= 3; mbyte--) {
    for (int mbit = 0; mbit <= 7; mbit++) {
      color = mac[mbyte] & (1 << mbit) ? WHITE : GRAY;
      gp_neopixel_set(i++, color);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while(true) {
    if (Serial) break;
    if (millis() > 5000) break;
  }
  uint32_t start = millis();

  Serial.print("Booted ");
  Serial.println(MEDALLION_VERSION);

  Serial.print("Raw battery value is ");
  Serial.println(analogRead(A0));

/*
  Serial.print("Serial took ");
  Serial.print(now);
  Serial.println(" millis");
*/
  gp_neopixel_init();

  start = millis();

  byte mac[6];
  wifi_init(mac);
  my_gp_id = (mac[4] << 8) & mac[5];
  draw_mac(mac);

  RgbColor color;
  bool locked = false;
  while (millis() < start + 10000 || locked) {
    int level = battery_level();
    color = level >= 80 ? GREEN :
            level >= 25 ? YELLOW : RED;
    int pixel = BORDER_COUNT;
    int cutoff = 95;
    for (int row = 1; row <= 6; row++) {
      for (int i = 0; i < row; i++) {
        gp_neopixel_set(pixel++, level < cutoff ? BLACK : color);
      }
      cutoff -= 18;
    }
    gp_neopixel_show();
    delay(100);
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (locked) {
        delay(250);
        break;
      }
      locked = true;
      gp_neopixel_set_all(BLUE);
      delay(250);
      draw_mac(mac);
    }
  }
  gp_neopixel_set_all(GRAY);
  network_sending_palettes = false;
}


void handle_packet(gp_t *gp) {
  if (gp->packet_type != PALETTE || gp->palette_size == 0) return;

  network_sending_palettes = true;

  int i;
  for (i = 0; i < BORDER_COUNT; i++) {
    gp_neopixel_set(i, gp->palette_r[0], gp->palette_g[0], gp->palette_b[0]);
  }
  uint8_t bg = gp->palette_size == 1 ? 0 : 1;
  for ( ; i < NEOPIXEL_COUNT ; i++) {
    gp_neopixel_set(i, gp->palette_r[bg], gp->palette_g[bg], gp->palette_b[bg]);
  }
  gp_neopixel_show();
}

#define LOOPS_PER_FRAME 8
void idle_loop() {
  static uint32_t idle_phase = 0;
  idle_phase++;
  if (idle_phase % LOOPS_PER_FRAME != 0) return;
  int frame = idle_phase / LOOPS_PER_FRAME;
  int border_phase = frame % 3;
  int i;
  for (i = 0; i < BORDER_COUNT; i++) {
    gp_neopixel_set(i, i % 3 == border_phase ? idle_color : BLACK);
  }
  for ( ; i < NEOPIXEL_COUNT ; i++) {
    uint32_t n = (frame + i) % 255;
    if (n < 85) gp_neopixel_set(i, 255 - n * 3, 0, n * 3);
    else if (n < 170) gp_neopixel_set(i, 0, (n-85) * 3, 255 - (n-85) * 3);
    else gp_neopixel_set(i, (n-170) * 3, 255 - (n-170) * 3, 0);
  }
  gp_neopixel_show();
}

void loop() {
  delay(20);
  if (digitalRead(BUTTON_PIN) == LOW) {
    gp_neopixel_set_all(GRAY);
    reset();
  }

  wifi_loop();

  if (last_wifi_status != WL_CONNECTED) {
    network_sending_palettes = false;
  } else {
    char packet[MAX_PACKET_LEN+1];
    int packet_len = udp_getpacket(packet);

    if (packet_len > 0) {
      gp_t gp;
      bool valid_packet = packet_parse(packet, packet_len, &gp);
    
      if (valid_packet) {
        wifi_update_timeout();
        handle_packet(&gp);  // Sets network_sending_palettes=true
      }
    }
  }
  
  if (!network_sending_palettes) {
    idle_loop();
    return;
  }
  
}
