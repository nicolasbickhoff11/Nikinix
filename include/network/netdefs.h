#define RTL8139_REG_COMMAND      0x37
#define RTL8139_REG_TX_ADDR_LOW  0x20
#define RTL8139_REG_TX_ADDR_HIGH 0x24
#define RTL8139_REG_RX_ADDR_LOW  0x30
#define RTL8139_REG_RX_ADDR_HIGH 0x34
#define RTL8139_REG_INTR_MASK    0x3C
#define RTL8139_REG_INTR_STATUS  0x3E

#define RTL8139_CMD_RESET        0x10
#define RTL8139_CMD_RX_EN        0x08
#define RTL8139_CMD_TX_EN        0x04

#define RTL8139_VENDOR_ID        0x10EC
#define RTL8139_DEVICE_ID        0x8139

#define ICMP_ECHO_REQUEST        0x08
#define ICMP_ECHO_REPLY          0x00

#define RTL8139_IRQ              10

#define MY_IP       0xC0A80001
#define MY_SUBNET   0xFFFFFF00
#define MY_GATEWAY  0xC0A800FE

#define ROK (1<<0)
#define RER	(1<<1)
#define TOK (1<<2)
#define TER	(1<<3)

typedef struct
{
    uint32_t tx_buff;
    uint32_t rx_buff;
    uint8_t  mac[6];
} RTL;

#define ETHERNET_IP 0x0800
#define TCP_PORT    80

typedef struct
{
    uint8_t dstMac[6];
    uint8_t srcMac[6];
    uint16_t etherType;
} EthernetFrame;

typedef struct
{
    uint8_t   versionIhl;
    uint8_t   dscpEcn;
    uint16_t  totalLength;
    uint16_t  id;
    uint16_t  flagsOffset;
    uint8_t   ttl;
    uint8_t  protocol;
    uint16_t  checksum;
    uint32_t srcIp;
    uint32_t dstIp;
} IPHeader;

typedef struct 
{
    uint16_t  srcPort;
    uint16_t  dstPort;
    uint32_t  seqNum;
    uint32_t  ackNum;
    uint8_t   offsetReserved;
    uint8_t   flags;
    uint16_t  window;
    uint16_t  checksum;
    uint16_t  urgentPtr;
} TCPHeader;

#define ICMP_PACKET_CAPACITY 64

#define ARP_REQUEST          0x01
#define ARP_REPLY            0x02
#define ARP_HW_TYPE_ETHERNET 0x01
#define ARP_PROTO_IP         0x0800
#define ARP_HW_ADDR_LEN      0x06
#define ARP_PROTO_ADDR_LEN   0x04
#define ARP_TABLE_SIZE       0x0A

typedef struct
{
    uint16_t  hwType;
    uint16_t  protoType;
    uint8_t   hwAddrLen;
    uint8_t   protoAddrLen;
    uint16_t  opcode;
    uint8_t   senderHwAddr[ARP_HW_ADDR_LEN];
    uint32_t  senderProtoAddr;
    uint8_t   targetHwAddr[ARP_HW_ADDR_LEN];
    uint32_t  targetProtoAddr;
} ARPHeader;

typedef struct
{
    uint32_t ipAddress;
    uint8_t  macAddress[ARP_HW_ADDR_LEN];
} ARPEntry;
