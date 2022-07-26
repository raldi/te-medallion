#include "gp_wifi.h"
#include "gp_udp.h"
#include "gp_ssid_db.h"

#define WIFI_CS_PIN 8
#define WIFI_IRQ_PIN 7
#define WIFI_RESET_PIN 4
#define WIFI_ENABLE_PIN 2

#define CONNECT_TIMEOUT (10 * 1000)
#define IDLE_TIMEOUT (60 * 1000)
#define CLIENT_ID_PERIOD (60 * 5 * 1000)

uint8_t last_wifi_status;
uint32_t next_connect_attempt;
uint32_t next_client_id;
bool scan_pending;

#ifdef ESP8266
const char * ssidstr(String ssid) {
  static char rv[MAX_SSID_LENGTH+1];
  strncpy(rv, ssid.c_str(), MAX_SSID_LENGTH+1);
  rv[MAX_SSID_LENGTH] = '\0';
  return rv;
}
#else
#define ssidstr(ssid) (ssid)
#endif

void wifi_init(byte mac[6]) {
#ifdef ESP8266
  WiFi.mode(WIFI_STA);
#else
  WiFi.setPins(WIFI_CS_PIN, WIFI_IRQ_PIN, WIFI_RESET_PIN, WIFI_ENABLE_PIN);
#endif
  gp_ssid_db_init();
  next_connect_attempt = 0;
  last_wifi_status = WL_NO_SHIELD;
  WiFi.macAddress(mac);
  scan_pending = false;
}

bool is_gp(int network) {
  if (WiFi.encryptionType(network) != ENC_TYPE_NONE) return false;
  const char * ssid = ssidstr(WiFi.SSID(network));
  //Serial.print("Considering ");
  //Serial.println(ssid);
  return strstr(ssid, "*GP*") != NULL;
}


// Find least-recently-joined GP network, using signal strength as tiebreaker
const char * next_network(uint32_t now) {
  int8_t ssids_found = WiFi.scanComplete();

  scan_pending = false;

  if (ssids_found == WIFI_SCAN_FAILED) {
    Serial.println("Wifi scan failed");
    idle_color = RED;
    return NULL;
  } else if (ssids_found == WIFI_SCAN_RUNNING) {
    idle_color = CYAN;
    scan_pending = true;
    return NULL;
  } else if (ssids_found == 0) {
    Serial.println("No networks found of any kind");
  }

  idle_color = WHITE;

  uint32_t best_lja = 999999;

  for (int i = 0; i < ssids_found; i++) {
    if (!is_gp(i)) continue;
    uint32_t lja = get_last_join_attempt(ssidstr(WiFi.SSID(i)));
    if (lja < best_lja) best_lja = lja;
  }

  int32_t best_strength = -9999;
  int8_t strongest = -1;
  for (int i = 0; i < ssids_found; i++) {
    if (!is_gp(i)) continue;
    uint32_t lja = get_last_join_attempt(ssidstr(WiFi.SSID(i)));
    if (lja > best_lja) continue;
    int32_t strength = WiFi.RSSI(i);
    if (strength > best_strength) {
      best_strength = strength;
      strongest = i;
    }
  }
  if (strongest < 0) {
    Serial.println("No GP networks found");
    return NULL;
  }
  const char * ssid = ssidstr(WiFi.SSID(strongest));
  if (ssid[0] == 0) {
    Serial.print("Got blank SSID for position ");
    Serial.println(strongest);
    return NULL;
  }
  remember_join_attempt(ssid, now);
  return ssid;
}

void join_new_network(uint32_t now) {
  const char * ssid;

  ssid = next_network(now);

  if (ssid == NULL) {
    return;
  }

  Serial.print("Attempting to connect to ");
  Serial.println(ssid);
  gp_neopixel_set_all(YELLOW);
  idle_color = YELLOW;
  uint8_t rv = WiFi.begin(ssid);  // Could block for up to CONNECT_TIMEOUT
}

void wifi_loop() {
  uint32_t now = millis();

  if (now >= next_connect_attempt) {
    next_connect_attempt = now + CONNECT_TIMEOUT;
    WiFi.scanDelete();
    last_wifi_status = WL_IDLE_STATUS;
    WiFi.scanNetworks(true);
    scan_pending = true;
  }

  if (scan_pending) {
    join_new_network(now);
  }

  uint8_t wifi_status = WiFi.status();

  if (wifi_status == last_wifi_status) {
    if (wifi_status == WL_CONNECTED && now >= next_client_id) {
      next_client_id = now + CLIENT_ID_PERIOD;
      udp_send_client_id();
    }
    return;
  }
  
  last_wifi_status = wifi_status;
  switch (wifi_status) {
    case WL_NO_SHIELD:
      Serial.println("No WiFi shield detected");
      gp_neopixel_set_all(RED);
      while(true);
    case WL_NO_SSID_AVAIL:
      Serial.println("No SSID avail");
      // fallthrough
    case WL_CONNECT_FAILED:
    case WL_CONNECTION_LOST:
      idle_color = BLUE;
      // Fallthrough
    case WL_IDLE_STATUS:
      break;
    case WL_CONNECTED:
      idle_color = GREEN;
      next_client_id = 0;
      wifi_update_timeout();
      printWiFiStatus();
      udp_init();
      break;
    case WL_DISCONNECTED:
      // We maybe just started trying to join a network
      break;
    case WL_SCAN_COMPLETED:
#ifndef ESP8266
    case WL_AP_LISTENING:
    case WL_AP_CONNECTED:
    case WL_AP_FAILED:
    case WL_PROVISIONING:
    case WL_PROVISIONING_FAILED:
#endif
    default:
      Serial.print("Unknown WiFi status ");
      Serial.println(wifi_status);
      gp_neopixel_set_all(PURPLE);
      while(true);
  }
}

void wifi_update_timeout() {
  next_connect_attempt = millis() + IDLE_TIMEOUT;
}

bool wifi_info(IPAddress *ip, IPAddress *gateway, IPAddress *dns, byte *mac) {
  if (last_wifi_status != WL_CONNECTED) return false;
  *ip = WiFi.localIP();
  *gateway = WiFi.gatewayIP();
  *dns = WiFi.dnsIP();
  memset(mac, 0, sizeof(mac));
  WiFi.macAddress(mac);
  return true;
}

void printWiFiStatus() {
  IPAddress ip;
  IPAddress gateway;
  IPAddress dns;
  byte mac[6];

  if (!wifi_info(&ip, &gateway, &dns, mac)) {
    Serial.print("Couldn't get WiFi info");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("Gateway: ");
  Serial.println(gateway);

  Serial.print("DNS server: ");
  Serial.println(dns);
  
  Serial.print("MAC: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.println(mac[5],HEX);
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
