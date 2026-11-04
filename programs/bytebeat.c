#include "macros.h"
#include "audiocardboard/audio.h"

#define SAMPLE_RATE 64000

void _start()
{
    BYTE test[SAMPLE_RATE];

    for (int t = 0; t < SAMPLE_RATE; t++)
    {
        test[t] = ((((t * (t >> 8 | t >> 41)) << (17 & t >> 20))) + (t >> t))
    }

    play_bytebeat(test, sizeof(test));
}
