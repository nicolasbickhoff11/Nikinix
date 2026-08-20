#ifndef _RTC_H
#define _RTC_H 1

#include <stdint.h>

struct rtc_time {
    int year;   // 2026
    int month;  // 1-12
    int day;    // 1-31
    int hour;
    int min;
    int sec;
};

extern uint8_t century;

uint8_t read_cmos_register(uint8_t reg);
void check_24_hour();
void read_rtc(struct rtc_time *rtc);

static inline uint8_t bcd_to_bin(uint8_t v) {
    return (v & 0x0F) + ((v >> 4) * 10);
}

#endif
