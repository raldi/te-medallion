#include <WiFiUdp.h>
#include "gp_packet.h"

#define GP_PORT 7016
WiFiUDP UdpIn;
WiFiUDP UdpOut;

void udp_init() {
  UdpIn.begin(GP_PORT);
}

int udp_getpacket(char * packet) {
  int packet_len = UdpIn.parsePacket();
  if (!packet_len) return 0;
  return UdpIn.read(packet, MAX_PACKET_LEN);
}

bool udp_sendpacket(IPAddress dest, const char * packet, uint8_t length) {
  if (!UdpOut.beginPacket(dest, GP_PORT)) return false;
  UdpOut.write(packet, length);
#if 0
  Serial.print("Sending packet of len ");
  Serial.print(length);
  Serial.print(" to ");
  Serial.print(dest);
  Serial.print(" port ");
  Serial.println(GP_PORT);
#endif
  return UdpOut.endPacket();
}

void udp_send_client_id() {
  IPAddress ip;
  IPAddress gateway;
  IPAddress dns;
  byte mac[6];
  byte ip_bytes[4];
  
  if (!wifi_info(&ip, &gateway, &dns, mac)) {
    Serial.println("Couldn't get wifi info for Client ID");
    return;
  }

  for (int i = 0; i < 4; i++) {
    ip_bytes[i] = ip[i];
  }
  
  char packet[MAX_PACKET_LEN+1];

  uint8_t length = encode_client_id(packet, ip_bytes, mac);

  if (gateway.isSet()) {
    if (!udp_sendpacket(gateway, packet, length)) {
      Serial.println("Failed to send Client ID to gateway");
    } else {
      Serial.println("Announced self to gateway");
    }
  }

  if (dns.isSet() && dns != gateway) {
    if (!udp_sendpacket(dns, packet, length)) {
      Serial.println("Failed to send Client ID to DNS server");
    } else {
      Serial.println("Announced self to DNS server");
    }
  }
}
