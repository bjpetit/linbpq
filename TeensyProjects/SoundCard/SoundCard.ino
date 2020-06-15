//	Teensy Soundcard emulator. Presents ADC and DAC to host as a SoundCard.

//	I don't need the overhead of the Audio Library, so I'll copy the interupt
//  routines from usb_audio.cpp and add my own wrapper
//	I see two uses for this. One combined with ARDOP (or possibly Packet) so host can
//  run a Soundcard mode (eg WINMOR) in parallel with ARDOP

//	Or a simple passthough (a "Signalink emulator").

//	Sample rates may be a problem. WINMOR uses 48000, but ARDOP 12000. Will PC tell us what
//  it wants??? Doesn't seem possible so fix at 48000.

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

#ifndef SOUNDCARD
#error("SOUNDCARD not defined in TeensyConfig.h");
#endif

#include "SoundCard.h"

#include "usb_dev.h"
#include <string.h> // for memcpy()

#ifdef AUDIO_INTERFACE // defined by usb_dev.h -> usb_desc.h
#else
#error ("USB Audio Not Enabled")
#endif // AUDIO_INTERFACE

#if AUDIO_TX_SIZE != 192
#error ("usb_desc.h has not been updated. See libraries/SoundCard/README.txt")
#endif


void CommonSetup();
void setupPDB(int SampleRate);
void setupDAC();
void setupADC(int Pin);
extern "C" void StartDAC();
extern "C" void stopDAC();
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
  void PlatformSleep(int mS);
  void SoundCardBG();

  my_audio_block_t * allocate(void);
  void release(my_audio_block_t *block);

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

// USB seems to send/request a block of samples every mS, so for 48K we send 192
// bytes (2 chan, 2 bytes per sample) so AUDIO_TX_SIZE, AUDIO_RX_SIZE have to be
// increased from 180 (for 44.1) to 192.

// But the pool block size is 128 (AUDIO_BLOCK_SAMPLES). WOuldn't it be better if same??

volatile int rxtot = 0;
volatile int txtot = 0;
volatile int inInts = 0;
volatile int outInts = 0;
extern volatile int dmaints;
extern volatile int samplessent;
extern volatile int samplesqueued;
volatile int lastperiod = 0, lastint = 0;

my_audio_block_t free_q = {0};
my_audio_block_t tohost_q = {0};

volatile int q_count = 0;

extern volatile int pttActive;
extern volatile int dacActive;
extern volatile int timestarted;

my_audio_block_t pool[POOLCOUNT];

uint16_t incoming_count;
uint8_t receive_flag;




// CAT over HID code

#define CATPORT Serial3
#define CATSPEED 19200

#define DMABUFATTR __attribute__ ((section(".dmabuffers"), aligned (4)))
uint16_t usb_audio_receive_buffer[AUDIO_RX_SIZE / 2] DMABUFATTR;
uint32_t usb_audio_sync_feedback DMABUFATTR;
uint8_t usb_audio_receive_setting = 0;

static uint32_t feedback_accumulator = (48 << 14) ;

void AudioInputbegin(void)
{
  incoming_count = 0;
  receive_flag = 0;
  usb_audio_sync_feedback = feedback_accumulator;
}

// Every few milliseconds the average sample rate over the last period
// is reported back as a 16.16 bit fixed point number. If the last
// period averaged out as 12.001 frames, then the value 0x000C0041
// will be reported (65536 * 12.001).

// Another source specifes 10.14 format


void update(void)
{
  my_audio_block_t *left = NULL, *right = NULL;

  __disable_irq();
  uint16_t c = incoming_count;
  uint8_t f = receive_flag;

  receive_flag = 0;
  __enable_irq();

  debugprintf("update count %d flag %d", c, f);

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

void HIDCatWrite(const uint8_t * Buffer, int Len)
{
#ifdef CATPORT
  CATPORT.write(Buffer, Len);
#endif
}

static unsigned char CatRXbuffer[256];
static int CatRXLen;

int lastAvail = 0;


uint16_t usb_audio_transmit_buffer[AUDIO_TX_SIZE / 2] DMABUFATTR;
uint8_t usb_audio_transmit_setting = 0;

char HIDMSG[64] = "Hello From HID";

// RawHID packets are always 64 bytes
byte buffer[64] = "\xeHello From HID";

elapsedMillis msUntilNextSend;
unsigned int packetCount = 0;


extern "C" void xxx_Setup(int bmRequestType)
{
  switch (bmRequestType)
  {
    case 0x2221: // CDC_SET_CONTROL_LINE_STATE
      return;
  }
#ifdef MONPORT
  MONPORT.printf("usb_setup Type %x\r\n", bmRequestType);
#endif
}


#include "SPI.h"

void setup()
{
  AudioInputbegin();

  SPI.begin();
#ifdef MONPORT
  MONPORT.begin(115200);
#endif
  //  while (!MONPORT);
  delay(5000);

  memset(usb_audio_transmit_buffer, 0, 192);

  // Set up ADC and DAC

  int i, sum = 0;

  // MONPORT.printf("%s\r\n", __FILE__);

  InitSound();

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

#ifdef MONPORT
  MONPORT.printf("DAC Baseline %d\r\n", sum / 1024);
#endif

#ifdef PIBOARD

  // PI Board can read Analog out which allows u to Read Vref

  SetPot(1, 256);        // TX Level Gain = 1

  delay(100);

  analogRead(17);

  for (i = 0; i < 100; i++)
  {
    VRef += analogRead(17);
  }
  VRef /= 100;
  analogRead(16);   // Set ADC back to A0

#endif

  (0, "VREF %d offset %d", VRef, VRef - 32768);

  // Set up the pool

  i = 0;

  while (i < POOLCOUNT)
  {
    release(&pool[i++]);
  }

  setupPDB(48000);			// 12K sample rate
  setupDAC();
  setupADC(16);
  StartADC();

  CommonSetup();

#ifdef CATPORT
  CATPORT.begin(CATSPEED);	// CAT Port
#ifdef SERIALCAT
  Serial.begin(115200);
#endif
#endif

}

int rx = 0;
char rxbuffer[80];

#ifdef CATPORT

void checkCATFromHost()
{
  int n = 0;

#ifdef HIDCAT
  n = RawHID.recv(rxbuffer, 0); // 0 timeout = do not wait

  if (n > 0)
  {
    char msg[80];

    // First byte is length

    int len = rxbuffer[0];

    sprintf(msg, "CAT Packet From Host Len %d Data %s", len, &rxbuffer[1]);
    Serial.println(msg);

    if (len < 64)
      CATPORT.write(&rxbuffer[1], len);
  }
#endif
#ifdef SERIALCAT
  n =  Serial.available();
  if (n > 0)
  {
    if (n > 80)
      n = 80;

    n = Serial.readBytes(rxbuffer, n);

    if (n > 0)
      CATPORT.write(rxbuffer, n);
  }
#endif
}

void checkCATFromRadio()
{
  int i;
  int Length = CATPORT.available();

  if (Length > lastAvail)
  {
    lastAvail = Length;
    delay(2);

    if (Length < 50)
      return 0;			   	// wait till we stop getting bytes
  }

  if (Length == 0)
    return 0;				// Nothing doing

  memset(CatRXbuffer, 0, 256);

  if (Length > 63)
    Length = 63;

  Length = CATPORT.readBytes(&CatRXbuffer[1], Length);
  CatRXbuffer[0] = Length;

  lastAvail = 0;

#ifdef HIDCAT
  //  Serial.printf("CAT RX Len %d ", Length);
  //  for (i = 0; i <= Length; i++)
  //    Serial.printf("%x ", CatRXbuffer[i]);
  //  Serial.printf("\r\n");

  Length++;				// Include Length Byte
  RawHID.send(CatRXbuffer, Length);
#endif
#ifdef SERIALCAT
  Serial.write(&CatRXbuffer[1], Length);		// Don't send Length
#endif
}
#endif

void loop()
{
  int n;

#ifdef CATPORT
  // only look for cat messages if CATPORT Defined
  checkCATFromHost();
  checkCATFromRadio();
#endif

  SoundCardBG();
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
  my_audio_block_t *in = NULL;
  return in;
}

extern "C"  void debugprintf(const char * format, ...)
{
#ifdef MONPORT
  char Mess[256];
  va_list(arglist);

  va_start(arglist, format);
  vsnprintf(Mess, sizeof(Mess), format, arglist);
  strcat(Mess, "\r\n");
  vsnprintf(Mess, sizeof(Mess), format, arglist);
  MONPORT.println(Mess);
#endif
  return;
}

// Stuff to support Common Code for ARDOP and Packet

extern "C" void pktProcessNewSamples(short * buf, int count) {}
