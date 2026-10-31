#include <stdint.h>
#include "io.h"
#include "print.h"

/*
    Coded by Danoni631 and nicolasbickhoff11

    Complementary Metal-Oxide Semiconductor or only CMOS is a battery puted on
    mother board to store basic informations about system like date, time and memory size

    this code manipulate and use CMOS battery things

    thanks to NicolasBickHoff for inviting me
*/

uint8_t day, month, year;

uint16_t get_cmos_memory()
{
    outb(0x70, 0x30);
    uint8_t low = inb(0x71);

    outb(0x70, 0x31);
    uint8_t high = inb(0x71);
}

void get_cmos_date()
{
    outb(0x70, 0x0A);
    while(inb(0x71) & 0x80);

    outb(0x70, 0x07); // get day
    day = inb(0x71);
  
    outb(0x70, 0x08); // get month
    month = inb(0x71);
  
    outb(0x70, 0x09); // get year
    year = inb(0x71);
}

void show_cmos_info()
{
    // yes
    show_cmos_memory();
    kprintf("\n");
    show_cmos_date();
    kprintf("\n");
}

void show_cmos_memory()
{
    uint16_t cmos_memory = get_cmos_memory();

    kprintf("Total CMOS memory: %x\n", cmos_memory);
}

void show_cmos_date()
{
    get_cmos_date();
    
    day   = ((day   / 16) * 10) + (day   & 0x0F);
    month = ((month / 16) * 10) + (month & 0x0F);
    year  = ((year  / 16) * 10) + (year  & 0x0F);

    kprintf("%d/%d/%d", month, day, year);
}
