#include <stdint.h>
#include "netinclude.h"

uint32_t htonl(uint32_t hostlong)
{
    return
    (
        ((hostlong & 0x000000FF) << 24) |
        ((hostlong & 0x0000FF00) << 8)  |
        ((hostlong & 0x00FF0000) >> 8)  |
        ((hostlong & 0xFF000000) >> 24)
    );
}

uint32_t ntohl(uint32_t netlong) 
{
    return
    (
        ((netlong & 0x000000FF) << 24) |
        ((netlong & 0x0000FF00) << 8)  |
        ((netlong & 0x00FF0000) >> 8)  |
        ((netlong & 0xFF000000) >> 24)
    );
}

uint16_t ntohs(uint16_t netshort)
{
    return (netshort >> 8) | (netshort << 8);
}

uint16_t htons(uint16_t hostshort) 
{
    return (hostshort << 8) | (hostshort >> 8);
}
