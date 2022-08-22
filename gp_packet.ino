#include "gp_packet.h"

const char * MAGIC = "GLPX";
uint16_t my_gp_id = 0;

uint16_t unpack16(const char * bytes) {
  uint16_t rv;
  rv = (*bytes) << 8;
  rv |= *(bytes+1);
  return rv;
}

bool packet_parse(const char * packet, uint16_t packet_len, gp_t * gp) {
  if (packet_len < MIN_PACKET_LEN) return false;
  if (memcmp(packet, MAGIC, 4) != 0) return false;

  memset(gp, 0, sizeof(gp_t));

  uint16_t length = unpack16(packet + 5);
  gp->version = packet[4];
  gp->priority = packet[8];
  gp->flags = packet[9];
  gp->source = unpack16(packet + 10);
  gp->dest = unpack16(packet + 12);
  gp->packet_type = packet[7];

  if (packet_len == length) {
    gp->crc_status = CRC_NONE;
  } else {
    gp->crc_status = CRC_INVALID; // FIXME: Verify CRCs and start throwing away invalid packets
  }

  switch (gp->packet_type) {
    case PALETTE:
      parse_palette(gp, packet + 14, packet_len - 14);
      break;
    default:
      break;
  }

  if (gp->dest != 0 && gp->dest != my_gp_id) return false;

  return true;
}

void parse_palette(gp_t * gp, const char * payload, uint16_t payload_size) {
  if (payload_size < 2) return;

  uint16_t size = unpack16(payload);
  if (payload_size < 2 + size * 4) return;

  if (size > MAX_PALETTE_SIZE) size = MAX_PALETTE_SIZE;
  gp->palette_size = size;

  for (int i = 0; i < size; i++) {
    // Ignore frac
    gp->palette_r[i] = payload[2 + 4 * i + 1];
    gp->palette_g[i] = payload[2 + 4 * i + 2];
    gp->palette_b[i] = payload[2 + 4 * i + 3];
  }
}

uint8_t encode_client_id(char * buf, const byte *ip, const byte *mac) {
  memcpy(buf, MAGIC, 4);
  buf[4] = CURRENT_VERSION;
  buf[7] = CLIENTID;
  buf[8] = 5; // priority
  buf[9] = 0; // flags
  buf[10] = mac[4]; // Source
  buf[11] = mac[5]; // Source
  buf[12] = 0;  // Dest
  buf[13] = 0;  // Dest

  for (int i = 0 ; i < 6; i++) {
    buf[14 + i] = mac[i];
  }
  
  for (int i = 0 ; i < 4; i++) {
    buf[14 + 6 + i] = ip[i];
  }

  strncpy(buf + 14 + 6 + 4, MEDALLION_VERSION, NAME_LEN);

  uint8_t length = 14 + 6 + 4 + 30;

  buf[5] = length / 256;
  buf[6] = length % 256;

  return length;
  // FIXME: Do CRC and add 2 to return value
}
