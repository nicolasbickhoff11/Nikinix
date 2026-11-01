#include <stdint.h>

void write_dsp(uint8_t command);
void soundblaster_handler();
void reset_soundblaster();
void start_soundblaster();
void disable_soundblaster();
void setup_soundblaster();
void setup_dma(uint8_t* buff, uint16_t size);
void reset_soundblaster();
void soundblaster_play(uint8_t* buff, uint16_t size);
void play_bytebeat(uint8_t* buff, uint16_t size);
