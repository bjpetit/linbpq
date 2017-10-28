#include "usb_dev.h"

#define AUDIO_BLOCK_SAMPLES 192

extern volatile int rxtot;
extern volatile int inInts;
extern volatile int outInts; 

extern int SampleRate;

struct my_audio_block_struct;

typedef struct my_audio_block_struct
{
	struct my_audio_block_struct * chain;
	int16_t data[AUDIO_BLOCK_SAMPLES];
} my_audio_block_t;
