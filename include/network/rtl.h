#include <stdint.h>
#include "netdefs.h"

void send_packet
(
    uint8_t data,
    uint16_t length,
    uint32_t dstIp,
    uint16_t dstPort
);

void recieve_packet();

void handle_reply(ICMPHeader* icmpHdr);
void sendping(const char* dstIp);

uint32_t rtl_find_device();
uint8_t* get_mac();

void rtl_send_packet
(
    void* packet,
    uint16_t lenght,
    uint8_t* dstmac,
    uint16_t ethertype
);

int rtl_recieve_packet
(
    uint8_t* buff,
    int lenght
);

uint16_t calculate_check_summary
(
    uint16_t* addr,
    int lenght
);

uint32_t ip_to_DWORD(const char* ip);
