#include <stdint.h>
#include "netdefs.h"

void init_arp();
void arp_req(uint32_t targetIp);
void arp_reply(ARPHeader *arpHdr);
BYTE arp_lookup(uint32_t ip, uint8_t* mac);
void arp_rec_pack(uint8_t* packet, int length);
