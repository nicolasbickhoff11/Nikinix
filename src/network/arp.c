#include <stdint.h>
#include "boolean.h"
#include "string.h"
#include "netinclude.h"

ARPEntry arpTable[ARP_TABLE_SIZE];
int arpTableSize = 0x00;

void init_arp()
{
    memset(arpTable, 0x00, sizeof(arpTable));    
}

void arp_req(uint32_t targetIp)
{
    ARPHeader header;

    header.hwType = htons(ARP_HW_TYPE_ETHERNET);
    header.protoType = htons(ARP_PROTO_IP);
    header.hwAddrLen = ARP_HW_ADDR_LEN;
    header.protoAddrLen = ARP_PROTO_ADDR_LEN;
    header.opcode = htons(ARP_REQUEST);

    header.senderProtoAddr = htonl(0xC0A80103); //192.168.1.3
    header.targetProtoAddr = htonl(targetIp);

    memcpy(header.senderHwAddr, "\x00\x0C\x29\x3D\x59\x0A", ARP_HW_ADDR_LEN);
    memset(header.targetHwAddr, 0x00, ARP_HW_ADDR_LEN);

    rtl_send_packet(&header, sizeof(header), "\xFF\xFF\xFF\xFF\xFF\xFF", 0x01);
}

void arp_reply(ARPHeader *arpHdr)
{
    for (int i = 0; i < arpTableSize; i++) 
    {
        if (arpTable[i].ipAddress == ntohl(arpHdr->senderProtoAddr)) 
        {
            return;
        }
    }

    if (arpTableSize < ARP_TABLE_SIZE)
    {
        ARPEntry* entry = &arpTable[arpTableSize++];

        entry->ipAddress = ntohl(arpHdr->senderProtoAddr);
        memcpy(entry->macAddress, arpHdr->senderHwAddr, ARP_HW_ADDR_LEN);
    }
}

unsigned char arp_lookup(uint32_t ip, uint8_t* mac)
{
    for (int i = 0; i < arpTableSize; i++)
    {
        if (arpTable[i].ipAddress == ip)
        {
            memcpy(mac, arpTable[i].macAddress, ARP_HW_ADDR_LEN);
            return True;
        }
    }

    return False;
}

void arp_rec_pack(uint8_t* packet, int length)
{
    if (length < sizeof(ARPHeader))
    {
        return;
    }

    ARPHeader* arpHdr = (ARPHeader*) packet;

    if (ntohs(arpHdr->hwType) != ARP_HW_TYPE_ETHERNET || ntohs(arpHdr->protoType) != ARP_PROTO_IP)
    {
        return;
    }

    if (ntohs(arpHdr->opcode) == ARP_REQUEST || ntohs(arpHdr->opcode) == ARP_REPLY)
    {
        ARPReply(arpHdr);
    }
}
