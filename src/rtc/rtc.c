#include "rtc.h"
#include "io.h"
#include <stdint.h>
#include "print.h"

#define FALLBACK_CENTURY 20
#define NMI_DISABLE 0x80

uint8_t century = FALLBACK_CENTURY;

uint8_t read_cmos_register(uint8_t reg) {
    outb(0x70, NMI_DISABLE | (reg & 0x7F));
    io_wait();
    return inb(0x71);
}

void write_cmos_register(uint8_t reg, uint8_t value) {
    outb(0x70, NMI_DISABLE | (reg & 0x7F));
    io_wait();
    outb(0x71, value);
}

void check_24_hour() {
    uint8_t regB = read_cmos_register(0x0B);

    uint8_t is_24 = (regB >> 1) & 1;
    uint8_t is_bcd = !(regB & 0x04);

    if (!is_24) {
        kprintf("[LOG] Detected 12-hour mode in RTC!\n");
    } else {
        kprintf("[LOG] Detected 24-hour mode in RTC!\n");
    }
}

void read_rtc(struct rtc_time *rtc) {
    uint8_t regB = read_cmos_register(0x0B);

    uint8_t is_24 = (regB >> 1) & 1;
    uint8_t is_bcd = !(regB & 0x04);

//    if (!is_24) {
//        kprintf("[LOG] Detected 12-hour mode in RTC!\n");
//    } else {
//        kprintf("[LOG] Detected 24-hour mode in RTC!\n");
//    }

    uint8_t hours = read_cmos_register(0x04);
    uint8_t minutes = read_cmos_register(0x02);
    uint8_t seconds = read_cmos_register(0x00);

    uint8_t day   = read_cmos_register(0x07);
    uint8_t month = read_cmos_register(0x08);
    uint8_t year  = read_cmos_register(0x09);

    // BCD convert if needed
    if (is_bcd) {
        hours   = bcd_to_bin(hours);
        minutes = bcd_to_bin(minutes);
        seconds = bcd_to_bin(seconds);
        day     = bcd_to_bin(day);
        month   = bcd_to_bin(month);
        year    = bcd_to_bin(year);
    }

    // 12h → 24h conversion
    if (!is_24) {
        uint8_t h = hours & 0x7F;
        uint8_t pm = (hours >> 7) & 1;

        if (h == 12)
            hours = pm ? 12 : 0;
        else
            hours = pm ? h + 12 : h;
    }

    // fill struct
    rtc->sec = seconds;
    rtc->min = minutes;
    rtc->hour   = hours;
    rtc->day    = day;
    rtc->month  = month;
    rtc->year   = year + (century * 100); // simplificação comum

}
