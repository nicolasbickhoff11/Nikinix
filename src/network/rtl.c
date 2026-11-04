#include <stdint.h>
#include "io.h"
#include "string.h"
#include "pci.h"
#include "netinclude.h"

/*
  CODED BY Danoni631

  network driver (rtl230330318012938'0280213821831249384912839643846813659993897591882)
*/

static uint8_t RTL8139SLOT;
static uint8_t RTL8139BUS;

RTL rtldevice;
static uint32_t iobase;

void send_packet
(
    uint8_t data,
    uint16_t length,
    uint32_t dstIp,
    uint16_t dstPort
)
{
    EthernetFrame ethFrame;
    IPHeader ipHdr;
    TCPHeader tcpHdr;

    memset(&ethFrame, 0x00, sizeof(ethFrame));
    memset(&tcpHdr, 0x00, sizeof(tcpHdr));
    memset(&ipHdr, 0x00, sizeof(ipHdr));

    BYTE dstMac[6];
    if (!arp_lookup(dstIp, dstMac))
    {
        // ARP Lookup failed! Sending ARP Request...
        arp_req(dstIp);

        int timeout = 1000000;
        while (!arp_lookup(dstIp, dstMac) && --timeout);

        if (timeout == 0x00)
        {
            //ARP Request timed out!
            return;
        }
    }

    memcpy(ethFrame.dstMac, dstMac, 6);
    memcpy(ethFrame.srcMac, GetMAC(), 6);
    ethFrame.etherType = htons(ETHERNET_IP);

    ipHdr.versionIhl = (4 << 4) | (5);
    ipHdr.totalLength = htons(sizeof(IPHeader) + sizeof(TCPHeader) + length);
    ipHdr.id = htons(54321);
    ipHdr.ttl = 64;
    ipHdr.protocol = 6;
    ipHdr.srcIp = htonl(MY_IP);
    ipHdr.dstIp = htonl(dstIp);
    ipHdr.checksum = calculate_check_summary((LPWORD)&ipHdr, sizeof(IPHeader));

    tcpHdr.srcPort = htons(TCP_PORT);
    tcpHdr.dstPort = htons(dstPort);
    tcpHdr.seqNum = 0;
    tcpHdr.ackNum = 0;
    tcpHdr.offsetReserved = (5 << 4);
    tcpHdr.window = htons(5840);
    tcpHdr.checksum = calculate_check_summary((LPWORD)&tcpHdr, sizeof(TCPHeader));

    uint8_t txBuffer[sizeof(EthernetFrame) + sizeof(IPHeader) + sizeof(TCPHeader) + length];
    memcpy(txBuffer, &ethFrame, sizeof(EthernetFrame));
    memcpy(txBuffer + sizeof(EthernetFrame), &ipHdr, sizeof(IPHeader));
    memcpy(txBuffer + sizeof(EthernetFrame) + sizeof(IPHeader), &tcpHdr, sizeof(TCPHeader));
    memcpy(txBuffer + sizeof(EthernetFrame) + sizeof(IPHeader) + sizeof(TCPHeader), data, length);

    rtl_send_packet(txBuffer, sizeof(txBuffer), dstMac, ETHERNET_IP);
}

void recieve_packet()
{
    // idk
}

void handle_reply(ICMPHeader* icmpHdr)
{
    if (length < sizeof(ICMPHeader))
    {
        return;
    }

    if (icmpHdr->type == 0x00)
    {
        return "Received Echo Reply!";
    }
}

void sendping(const char* dstIp)
{
    uint8_t packet[ICMP_PACKET_SIZE];
    memset(packet, 0x00, ICMP_PACKET_SIZE);

    uint8_t dstMac[6];
    uint32_t dstIpDWORD = ip_to_DWORD(dstIp);

    if (!arp_lookup(dstIpDWORD, dstMac))
    {
        // ARP Lookup failed! Sending ARP Request...
        arp_req(dstIpDWORD);

        int timeout = 1000000;
        while (!arp_lookup(dstIpDWORD, dstMac) && --timeout);

        if (timeout == 0x00)
        {
            //ARP Request timed out!
            return;
        }
    }

    IPHeader* ipHeader = (IPHeader*)packet;
    ipHeader->versionIhl = 0x45;
    ipHeader->dscpEcn = 0;
    ipHeader->totalLength = htons(sizeof(IPHeader) + sizeof(ICMPHeader));
    ipHeader->id = htons(54321);
    ipHeader->flagsOffset = 0;
    ipHeader->ttl = 64;
    ipHeader->protocol = 1;
    ipHeader->checksum = 0;
    ipHeader->srcIp = htonl(MY_IP);
    ipHeader->dstIp = htonl(dstIpDWORD);
    ipHeader->checksum = calculate_check_summary((LPWORD)ipHeader, sizeof(IPHeader));

    ICMPHeader* icmpHeader = (ICMPHeader*)(packet + sizeof(IPHeader));
    icmpHeader->type = 8;
    icmpHeader->code = 0;
    icmpHeader->checksum = 0;
    icmpHeader->id = htons(1);
    icmpHeader->sequence = htons(1);

    icmpHeader->checksum = calculate_check_summary((LPWORD)icmpHeader, sizeof(ICMPHeader));
    
    // Sending ICMP Echo Request...

    rtl_send_packet(packet, sizeof(IPHeader) + sizeof(ICMPHeader), dstMac, ETHERNET_IP);
}

uint32_t rtl_find_device()
{
    for (uint8_t bus = 0; bus < 256; bus++)
    {
        for (uin8_t slot = 0; slot < 32; slot++)
        {
            uint32_t vendorDevice = pci_config_read_DWORD(bus, slot, 0, 0x00);

            if (vendorDevice == ((RTL8139_DEVICE_ID << 16) | RTL8139_VENDOR_ID))
            {
                uint32_t bar = pci_config_read_DWORD(bus, slot, 0, 0x10);

                if (bar & 0x01)
                {
                    RTL8139BUS  = bus;
                    RTL8139SLOT = slot;
                  
                    return bar & ~0x3;
                }
            }
        }
}

uint8_t* get_mac()
{
    rtldevice.mac[0] = inb(iobase + 0x00);
    rtldevice.mac[1] = inb(iobase + 0x01);
    rtldevice.mac[2] = inb(iobase + 0x02);
    rtldevice.mac[3] = inb(iobase + 0x03);
    rtldevice.mac[4] = inb(iobase + 0x04);
    rtldevice.mac[5] = inb(iobase + 0x05);

    return rtldevice.mac;
}

void rtl_send_packet
(
    void* packet,
    uint16_t lenght,
    uint8_t* dstmac,
    uint16_t ethertype
)
{
    EthernetFrame ethFrame;
    memcpy(ethFrame.dstMac, dstMac, 6);
    memcpy(ethFrame.srcMac, GetMAC(), 6);
    ethFrame.etherType = htons(etherType);

    uint8_t txBuffer[sizeof(EthernetFrame) + length];
    memcpy(txBuffer, &ethFrame, sizeof(EthernetFrame));
    memcpy(txBuffer + sizeof(EthernetFrame), packet, length);

    uint32_t txAddr = RTL8139_REG_TX_ADDR_LOW + (currentTx * 4);
    uint32_t txStatus = RTL8139_REG_TX_ADDR_HIGH + (currentTx * 4);

    memcpy((void*)inl(iobase + txAddr), txBuffer, sizeof(txBuffer));
    outl(iobase + txStatus, sizeof(txBuffer) & 0xFFFF);

    currentTx = (currentTx + 1) % 4;
}

int rtl_recieve_packet
(
    uint8_t* buff,
    int lenght
)
{
    uint16_t packetLength = *(uint16_t*)(rxBuff + readptr + 2); 

    if (packetLength == 0 || packetLength > bufferLength) 
    {
        return 0;
    }

    memcpy(buffer, (void*)(rxBuff + readptr), packetLength);

    readptr = (readptr + packetLength + 3) & (~3);

    outw(iobase + 0x38, readptr - 0x10);

    return packetLength;
}

uint16_t calculate_check_summary
(
    uint16_t* addr,
    int lenght
)
{
    uint32_t sum = 0x00;

    while (length > 1)
    {
        sum += *addr++;
        length -= 2;
    }

    if (length == 1)
    {
        sum += *(uint8_t*) addr;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

uint32_t ip_to_DWORD(const char* ip)
{
    uint32_t result = 0;
    int part = 0;

    while (*ip) 
    {
        if (*ip >= '0' && *ip <= '9') 
        {
            part = part * 10 + (*ip - '0');
        } 
          
        else if (*ip == '.') 
        {
            result = (result << 8) | (part & 0xFF);           
            part = 0;
        }

        ip++;
    }

    result = (result << 8) | (part & 0xFF);

    return result;
}
