#include <stdint.h>
#include "io.h"

#include "audiocardboard/audio.h"

int playing

void write_dsp(uint8_t command)
{
    while ((inb(0x22C) & 0x80) != 0x00);
    outb(0x22C, command);
}

void soundblaster_handler()
{
    inb(0x22E);
    outb(0x20, 0x20);
    playing = 0x00;
}

void reset_soundblaster()
{
    outb(0x226, 0x01);
    outb(0x226, 0x00);
}

void start_soundblaster()
{
    write_dsp(0xD1);
}

void disable_soundblaster()
{
    write_dsp(0xD3);
}

void setup_soundblaster()
{
    reset_sound_blaster();
    start_sound_blaster();
}

void setup_dma(uint8_t* buff, uint16_t size)
{
    uint64_t address = (uint64_t)buff;
  
    outb(0x0A, 0x05);
    outb(0x0C, 0x00);
    outb(0x0B, 0x49);
    outb(0x02, address & 0xFF);
    outb(0x02, (address >> 8) & 0xFF);
    outb(0x83, (address >> 16) & 0xFF);
    outb(0x03, (size - 1) & 0xFF);
    outb(0x03, (size - 1) >> 8);
    outb(0x0A, 0x01)
}

void soundblaster_play(uint8_t* buff, uint16_t size)
{
    playing = 0x01;
    setup_dma(buff, size);
    write_dsp(0x40);
    write_dsp(256 - (1000000 / 8000); // 11kHz
    wrtie_dsp(0x14);
    write_dsp((size - 1) & 0xFF);
    write_dsp((size - 1) >> 8);

    for (int i = 0; i < 10000000; i++);
    playing = 0x00;
}

void play_bytebeat(uint8_t* buff, uint16_t size)
{
    soundblaster_play(buff, size);
    while (playing);
    disable_soundblaster();
}
