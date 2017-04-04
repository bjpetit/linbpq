//	Teensy Soundcard emulator. Presents ADC and DAC to host as a SoundCard.

//	I don't need the overhead of the Audio Library, so I'll copy the interupt routines from
//	usb_audio.cpp and add my own wrapper
//	I see two uses for this. One combined with ARDOP (or possibly Packet) so host can run a
//  Soundcard mode (eg WINMOR) in parallel with ARDOP/

//	Or a simple passthough (a "Signalink emulator").

//	Sample rates may be a problem. WINMOR uses 48000, but ARDOP 12000. Will PC tell us what
//  it wants???

//	What happens if we try to run ARDOP at 48000???

/* Teensyduino Core Library
   http://www.pjrc.com/teensy/
   Copyright (c) 2016 PJRC.COM, LLC.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   1. The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   2. If the Software is incorporated into a build system that allows
   selection among a list of target devices, then similar target
   devices manufactured by PJRC.COM must be included in the list of
   target devices and selectable in the same manner.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include "TeensyConfig.h"
#include "TeensyCommon.h"

#include "SoundCard.h"
#include "usb_dev.h"
#include <string.h> // for memcpy()

#ifdef AUDIO_INTERFACE // defined by usb_dev.h -> usb_desc.h
#else
#error ("USB Audio Not Enabled")
#endif // AUDIO_INTERFACE

void CommonSetup();
void setupPDB(int SampleRate);
void setupDAC();
void setupADC(int Pin);
extern "C" void StartDAC();
extern "C" void StopDAC();
void StartADC();
void setupOLED();
void setupWDTTFT();

// Arduino is a c++ environment so functions in ardop must be defined as "C"

extern "C"
{
  void WriteDebugLog(int Level, const char * format, ...);
  void InitSound();
  void HostInit();
  void CheckTimers();
  void HostPoll();
  void MainPoll();
  void InitDMA();
  void PlatformSleep();

#define Now getTicks()
}

extern int VRef;

int TXLevel = 128;				// 300 mV p-p Used on Teensy
int RXLevel = 0;				// Configured Level - zero means auto tune
int autoRXLevel = 255;			// calculated level

extern int SampleRate;

// Uncomment this to work around a limitation in Macintosh adaptive rates
// This is not a perfect solution.  Details here:
// https://forum.pjrc.com/threads/34855-Distorted-audio-when-using-USB-input-on-Teensy-3-1
//#define MACOSX_ADAPTIVE_LIMIT

// We maintain a buffer pool here.

// I think we will just pass straight from DAC/ADC to USB, so don't need to buffer a lot
// (DAC/DAC work with 100 mS blocks). If we want to process sound (eg for ARDOP and also
// pass to host (which might make sense for multimode scanning) we may have to rethink this

// USB seems to send/requrst a block of samples every mS, so for 48K we send 192
// bytes (2 chan, 2 bytes per sample) so AUDIO_TX_SIZE, AUDIO_RX_SIZE have to be
// increased from 180 (for 44.1) to 192.

// But the pool block size is 128 (AUDIO_BLOCK_SAMPLES). WOuldn't it be better if same??

#define POOLCOUNT 32

volatile int rxtot = 0;
volatile int inInts = 0;
volatile int outInts = 0;

my_audio_block_t * free_q = NULL;

volatile int q_count = 0;

my_audio_block_t pool[POOLCOUNT];

my_audio_block_t * incoming_left;
my_audio_block_t * incoming_right;
my_audio_block_t * ready_left;
my_audio_block_t * ready_right;

uint16_t incoming_count;
uint8_t receive_flag;

struct usb_audio_features_struct features = {0, 0, FEATURE_MAX_VOLUME};

#define DMABUFATTR __attribute__ ((section(".dmabuffers"), aligned (4)))
uint16_t usb_audio_receive_buffer[AUDIO_RX_SIZE / 2] DMABUFATTR;
uint32_t usb_audio_sync_feedback DMABUFATTR;
uint8_t usb_audio_receive_setting = 0;

static uint32_t feedback_accumulator = (SampleRate / 1000) * 16384 * 256;

void AudioInputbegin(void)
{
  incoming_count = 0;
  incoming_left = NULL;
  incoming_right = NULL;
  ready_left = NULL;
  ready_right = NULL;
  receive_flag = 0;
  usb_audio_sync_feedback = feedback_accumulator >> 8;
}

static void copy_to_buffers(const uint32_t *src, int16_t *left, int16_t *right, unsigned int len)
{
  uint32_t *target = (uint32_t*) src + len;
  while ((src < target) && (((uintptr_t) left & 0x02) != 0)) {
    uint32_t n = *src++;
    *left++ = n & 0xFFFF;
    *right++ = n >> 16;
  }

  while ((src < target - 2)) {
    uint32_t n1 = *src++;
    uint32_t n = *src++;
    *(uint32_t *)left = (n1 & 0xFFFF) | ((n & 0xFFFF) << 16);
    left += 2;
    *(uint32_t *)right = (n1 >> 16) | ((n & 0xFFFF0000)) ;
    right += 2;
  }

  while ((src < target)) {
    uint32_t n = *src++;
    *left++ = n & 0xFFFF;
    *right++ = n >> 16;
  }
}

void update(void)
{
  my_audio_block_t *left, *right;

  __disable_irq();
  left = ready_left;
  ready_left = NULL;
  right = ready_right;
  ready_right = NULL;
  uint16_t c = incoming_count;
  uint8_t f = receive_flag;
  receive_flag = 0;
  __enable_irq();
  if (f) {
    int diff = AUDIO_BLOCK_SAMPLES / 2 - (int)c;
    feedback_accumulator += diff / 3;
    uint32_t feedback = (feedback_accumulator >> 8) + diff * 100;
#ifdef MACOSX_ADAPTIVE_LIMIT
    if (feedback > 722698) feedback = 722698;
#endif
    usb_audio_sync_feedback = feedback;
    //if (diff > 0) {
    //serial_print(".");
    //} else if (diff < 0) {
    //serial_print("^");
    //}
  }
  //serial_phex(c);
  //serial_print(".");
  if (!left || !right) {
    //serial_print("#"); // buffer underrun - PC sending too slow
    //if (f) feedback_accumulator += 10 << 8;
  }
  if (left) {
    transmit(left, 0);
    release(left);
  }
  if (right) {
    transmit(right, 1);
    release(right);
  }
}



my_audio_block_t * left_1st;
my_audio_block_t * left_2nd;
my_audio_block_t * right_1st;
my_audio_block_t * right_2nd;
uint16_t offset_1st;


uint16_t usb_audio_transmit_buffer[AUDIO_TX_SIZE / 2] DMABUFATTR;
uint8_t usb_audio_transmit_setting = 0;

void AudioOutputbegin(void)
{
  left_1st = NULL;
  right_1st = NULL;
}

static void copy_from_buffers(uint32_t *dst, int16_t *left, int16_t *right, unsigned int len)
{
  // TODO: optimize...
  while (len > 0) {
    *dst++ = (*right++ << 16) | (*left++ & 0xFFFF);
    len--;
  }
}

void AudioOutputUSBupdate(void)
{
  my_audio_block_t *left, *right;

  //left = receiveReadOnly(0); // input 0 = left channel
  //right = receiveReadOnly(1); // input 1 = right channel
  if (usb_audio_transmit_setting == 0) {
    if (left) release(left);
    if (right) release(right);
    if (left_1st) {
      release(left_1st);
      left_1st = NULL;
    }
    if (left_2nd) {
      release(left_2nd);
      left_2nd = NULL;
    }
    if (right_1st) {
      release(right_1st);
      right_1st = NULL;
    }
    if (right_2nd) {
      release(right_2nd);
      right_2nd = NULL;
    }
    offset_1st = 0;
    return;
  }
  if (left == NULL) {
    if (right == NULL) return;
    left = right;
  } else if (right == NULL) {
    right = left;
  }
  __disable_irq();
  if (left_1st == NULL) {
    left_1st = left;
    right_1st = right;
    offset_1st = 0;
  } else if (left_2nd == NULL) {
    left_2nd = left;
    right_2nd = right;
  } else {
    // buffer overrun - PC is consuming too slowly
    my_audio_block_t *discard1 = left_1st;
    left_1st = left_2nd;
    left_2nd = left;
    my_audio_block_t *discard2 = right_1st;
    right_1st = right_2nd;
    right_2nd = right;
    offset_1st = 0; // TODO: discard part of this data?
    //serial_print("*");
    release(discard1);
    release(discard2);
  }
  __enable_irq();
}



extern "C" void xxx_Setup(int bmRequestType)
{
  //	Serial.printf("usb_setup Type %x\r\n", bmRequestType);
}

int usb_audio_get_feature(void *stp, uint8_t *data, uint32_t *datalen)
{
  struct setup_struct setup = *((struct setup_struct *)stp);

  Serial.printf("get feature %x %x %x\r\n", setup.bmRequestType, setup.bCS, setup.bRequest);

  if (setup.bmRequestType == 0xA1) { // should check bRequest, bChannel, and UnitID
    if (setup.bCS == 0x01) { // mute
      data[0] = features.mute;  // 1=mute, 0=unmute
      *datalen = 1;
      return 1;
    }
    else if (setup.bCS == 0x02) { // volume
      if (setup.bRequest == 0x81) { // GET_CURR
        data[0] = features.volume & 0xFF;
        data[1] = (features.volume >> 8) & 0xFF;
      }
      else if (setup.bRequest == 0x82) { // GET_MIN
        //serial_print("vol get_min\n");
        data[0] = 0;     // min level is 0
        data[1] = 0;
      }
      else if (setup.bRequest == 0x83) { // GET_MAX
        data[0] = FEATURE_MAX_VOLUME & 0xFF;  // max level, for range of 0 to MAX
        data[1] = (FEATURE_MAX_VOLUME >> 8) & 0x0F;
      }
      else if (setup.bRequest == 0x84) { // GET_RES
        data[0] = 1; // increment vol by by 1
        data[1] = 0;
      }
      else { // pass over SET_MEM, etc.
        return 0;
      }
      *datalen = 2;
      return 1;
    }
  }
  return 0;
}

int usb_audio_set_feature(void *stp, uint8_t *buf)
{
  struct setup_struct setup = *((struct setup_struct *)stp);

  Serial.printf("set feature %x %x %x %x %x %x %x %x %x\r\n", setup.bmRequestType, setup.bCS, setup.bRequest, buf[0], buf[1],
                buf[2], buf[3], buf[4], buf[5]);

  if (setup.bmRequestType == 0x21) { // should check bRequest, bChannel and UnitID
    if (setup.bCS == 0x01) { // mute
      if (setup.bRequest == 0x01) { // SET_CUR
        features.mute = buf[0]; // 1=mute,0=unmute
        features.change = 1;
        return 1;
      }
    }
    else if (setup.bCS == 0x02) { // volume
      if (setup.bRequest == 0x01) { // SET_CUR
        features.volume = buf[0] + (buf[1] << 8);
        features.change = 1;
        return 1;
      }
    }
  }
  return 0;
}


#include "SPI.h"

void setup()
{
  SPI.begin();
  
  Serial.begin(115200);
  // while (!Serial);
  delay(1000);
  Serial.printf("Starting\r\n");

  // Set up ADC and DAC

  int i, sum = 0;

  Serial.printf("InitSound\r\n");
  InitSound();
  Serial.printf("InitSound ret\r\n");

  // Configure the ADC and run at least one software-triggered
  // conversion.  This completes the self calibration stuff and
  // leaves the ADC in a state that's mostly ready to use

  analogReadRes(16);
  //analogReference(INTERNAL); // range 0 to 1.2 volts
  analogReference(DEFAULT); // range 0 to 3.3 volts
  //analogReadAveraging(8);
  // Actually, do many normal reads, to start with a nice DC level

  for (i = 0; i < 1024; i++)
  {
    sum += analogRead(16);
  }

  Serial.printf("DAC Baseline %d\r\n", sum / 1024);

  setupPDB(12000);			// 12K sample rate
  setupDAC();
  setupADC(16);
  StartADC();

  // Read Vref

  SetPot(1, 256);				// TX Level Gain = 1
  delay(100);

  analogRead(17);

  Serial.printf("analogRead ret\r\n");

  for (i = 0; i < 100; i++)
  {
    VRef += analogRead(17);
  }
  VRef /= 100;
  analogRead(16);		// Set ADC back to A0

  Serial.printf("VREF %d offset %d\r\n", VRef, VRef - 32768);


  // Set up the pool

  i = 0;

  while (i < POOLCOUNT)
  {
    release(&pool[i++]);
  }
 
  AudioInputbegin();
  AudioOutputbegin();
}

int lastTicks = 0;

void loop()
{
  if ((millis() - lastTicks) > 999)
  {
    lastTicks = millis();
    Serial.printf("in Ints %d Out ints %d Q %d tot rx %d\r\n", inInts, outInts, q_count, rxtot / 4);
    inInts = outInts = rxtot = 0;
  }
}

my_audio_block_t * allocate(void)
{
  __disable_irq();

  my_audio_block_t * block = free_q;

  if (block == NULL)
  {
    __enable_irq();
    return NULL;
  }

  free_q = block->chain;
  q_count++;

  __enable_irq();
  return block;
}

// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool

void release(my_audio_block_t *block)
{
  __disable_irq();
  block->chain = free_q->chain;
  free_q = block;
  q_count++;
  __enable_irq();
}

void transmit(my_audio_block_t *block, unsigned char index)
{
  // I think this sends block from USB to DAC

  // Can we just append to the DAC cyclic buffer??

  // If input is stereo, do we average??

  // Also do Audio derived PTT (VOX) here

  release(block);
}
// Receive block from an input.  The block's data
// is sent to USB (I think!!). So this is called from
// PollReceivedSamples

my_audio_block_t * receive(unsigned int index)
{
  my_audio_block_t *in;
  return in;
}

