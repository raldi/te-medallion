#ifndef GP_WIFI_H
#define GP_WIFI_H

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi101.h>
#endif

extern uint8_t last_wifi_status;

void wifi_init(byte mac[6]);
void wifi_loop();
void wifi_update_timeout();
bool wifi_info(IPAddress *ip, IPAddress *gateway, IPAddress *dns, byte *mac);

#endif
