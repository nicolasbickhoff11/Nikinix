#include <stdint.h>
#include "io.h"
#include "malloc.h"
#include "pci.h"
#include "netinclude.h"

/*
  CODED BY Danoni631
  network driver :D
*/

void init_rtl()
{
    outb(iobase + 0x52, 0x00);
    outb(iobase + RTL8139_REG_COMMAND, RTL8139_CMD_RESET);

    while
    (
        (
            inb(iobase + RTL8139_REG_COMMAND)
            & RTL8139_CMD_RESET
        ) != 0x00
    ) 
    {
    }

    get_mac();
    
    rxBuff = (uint32_t)malloc(8192 + 16);

    if (!rxBuff)
    {
        return; // Failed to allocate memory
    }

    outl(iobase + RTL8139_REG_RX_ADDR_LOW, rxBuff);
    outw(iobase + RTL8139_REG_INTR_STATUS, 0xFFFF);
    outb(iobase + RTL8139_REG_COMMAND, 0x0C);

    outb
    (
        iobase + RTL8139_REG_COMMAND,
        RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN
    );
}

void init_ethernet()
{
    iobase = rtl_find_device();
    
    if (iobase == 0x00)
    {
        debug("RTL8139 NOT FOUND", 0x01);
    }
    else
    {
        init_rtl();

        BYTE irq =
        pci_config_read_DWORD
        (
            RTL8139BUS,
            RTL8139SLOT,
            0x00, 0x3C
        ) & 0xFF;

        // irq handler :P
    }
}

void init_ethernet()
{
    iobase = rtl_find_device();
    
    if (iobase == 0x00)
    {
        debug("RTL8139 NOT FOUND", 0x01);
    }
    else
    {
        init_rtl();

        BYTE irq =
        pci_config_read_DWORD
        (
            RTL8139BUS,
            RTL8139SLOT,
            0x00, 0x3C
        ) & 0xFF;

        // irq handler :P
    }
}
