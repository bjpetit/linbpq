// Arduino interface code for Soundmodem running on a Teensie 3.1 or 3.6

// So far...
// 1200 works both ways at 12000K sampling, but RX not at 48k
// 9600 works at 48K
// 1200 Ok on Tom's board.
// 9600 uses about 14% of Teensy 3.1

#include "TeensyConfig.h"
#include "TeensyCommon.h"

#ifndef PACKET
#error("PACKET not defined in TeensyConfig.h");
#endif


#include <DMAChannel.h>
#include "SPI.h"

#include <EEPROM.h>

unsigned char RXBUFFER[300];	// Async RX Bufferextern "C" void xxx_Setup(int bmRequestType)

extern int Baud;		// Modem Speed (1200 or 9600 for noe)
extern int AFSK;		// Modem Mode
extern int FSK;
extern int PSK;
extern int centreFreq;
extern int samplerate;
extern char VersionString[];
extern int VersionNo;
extern int KISSCHECKSUM;

extern int VRef;

extern int TXLevel;				// 300 mV p-p Used on Teensy
extern int RXLevel;				// Configured Level - zero means auto tune
extern int autoRXLevel;		// calculated level

#define TRUE 1
#define FALSE 0

int SoundIsPlaying = FALSE;
int Capturing = FALSE;
int SerialHost = TRUE;
int ActivePort = 0;						// Serial port in use


extern volatile int RXBPtr;
volatile int flag = 0;
volatile int flag2 = 0;
extern int inIndex;			// ADC Buffer half being used 0 or 1

void yDisplayCall(int dirn, char * Call);
void yDisplayState(char * State);

void CommonSetup();
void setupPDB(int SampleRate);
void setupDAC();
void setupADC(int Pin);
extern "C" void StartDAC();
extern "C" void StopDAC();
void StartADC();

extern "C" void SetPot(int address, unsigned int value);
extern "C" unsigned int GetPot(int address);
extern "C" void SetLED(int Pin, int State);
extern "C" void RunPSKReceive();
extern "C" void SetDefaultKISSParams(unsigned int txdelay, unsigned int ppersist,
                                     unsigned int slottime, unsigned int fullduplex, unsigned int txtail);


// Default KISS params. Will be overridden from EEPORM if set

unsigned int txdelay = 300;
unsigned int ppersist = 64;
unsigned int slottime = 100;
unsigned int fullduplex = 0;
unsigned int txtail = 0;

// TNC Params in EEPROM - compatible with pitnc_get/set

#define	TXDELAY		1
#define PERSIST		2
#define SLOTTIME	3
#define TXTAIL		4
#define FULLDUP		5
#define	KISSCHANNEL	6


void i2csetup();
void i2cloop();


#define ADC_SAMPLES_PER_BLOCK 1200
#define DAC_SAMPLES_PER_BLOCK 1200

#define LOGEMERGENCY 0
#define LOGALERT 1
#define LOGCRIT 2
#define LOGERROR 3
#define LOGWARNING 4
#define LOGNOTICE 5
#define LOGINFO 6
#define LOGDEBUG 7

// TNC Params in EEPROM - compatible with pitnc_get/set

#define	TXDELAY		1
#define PERSIST		2
#define SLOTTIME	3
#define TXTAIL		4
#define FULLDUP		5
#define	KISSCHANNEL	6
#define I2CADDRESS	7

#define DCDLED LED0
#define PKTLED LED3		// flash when packet received
extern "C"
{
  void WriteDebugLog(int Level, const char * format, ...);
  void InitSound();
  void HostInit();
  void CheckTimers();
  void HostPoll();
  void MainPoll();
  void InitDMA();
  void DisplayCall(int dirn, char * Call);
  void DisplayState(char * State);
  void SoundModemInit();
  void PollReceivedSamples();
  void ProcessKISSPacket(unsigned char * rxbuffer, int Length);
  void mainLoop();
  int RadioPoll();

  // extern unsigned int tmrPollOBQueue;

  extern volatile unsigned short ADC_Buffer[2 * ADC_SAMPLES_PER_BLOCK];
  int Number = 0;


#define Now getTicks()

  void _getpid()		// Seem to be needed to satifay linker
  {
  }

  void _kill()
  {
  }

#define MEM_LEN 512
  extern uint8_t databuf[MEM_LEN];
  extern volatile int i2cputptr, i2cgetptr;

  void HostPoll()
  {
    // Called roughly once every milisecond

#ifdef HOSTPORT

    if (!SerialHost)
      return;

    int RXBPtr = HOSTPORT.available();

    if (RXBPtr)
    {
      int Count;
      if (ActivePort != 1)
      {
        ActivePort = 1;
        WriteDebugLog(LOGINFO, "Input is on Port %d", ActivePort);
      }
      Count = HOSTPORT.readBytes((char *)RXBUFFER, RXBPtr);

      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessKISSPacket(RXBUFFER, RXBPtr);
    }
#endif
#ifdef HOSTPORT2
    RXBPtr = HOSTPORT2.available();
    if (RXBPtr)
    {
      int Count = HOSTPORT2.readBytes((char *)RXBUFFER, RXBPtr);
      RXBPtr += Count;
      if (ActivePort != 2)
      {
        ActivePort = 2;
        WriteDebugLog(LOGINFO, "Input is on Port %d", ActivePort);
      }

      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessKISSPacket(RXBUFFER, RXBPtr);
    }
#endif
#ifdef HOSTPORT3
    RXBPtr = HOSTPORT3.available();
    if (RXBPtr)
    {
      int Count = HOSTPORT3.readBytes((char *)RXBUFFER, RXBPtr);
      RXBPtr += Count;
      if (ActivePort != 3)
      {
        ActivePort = 3;
        WriteDebugLog(LOGINFO, "Input is on Port %d", ActivePort);
      }

      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessKISSPacket(RXBUFFER, RXBPtr);
    }
#endif
  }
}


void setup()
{
  uint32_t i, sum = 0;

  Baud = 9600;	// FSK G3RUH
  AFSK = FALSE;
  FSK = TRUE;
  PSK = FALSE;

  // if EEPROM KISS Params are set use them to overide defaults
  // unset EEPROM defaults to 255

  int value = EEPROM.read(TXDELAY);

  if (value != 255)
    txdelay = value * 10;

  value = EEPROM.read(PERSIST);

  if (value != 255)
    ppersist = value;

  value = EEPROM.read(SLOTTIME);

  if (value != 255)
    slottime = value * 10;

  value = EEPROM.read(TXTAIL);

  if (value != 255)
    txtail = value;

  value = EEPROM.read(FULLDUP);

  if (value == 1)
    fullduplex = 1;

  SetDefaultKISSParams(txdelay,  ppersist, slottime, fullduplex, txtail);

  // if EEPROM Reg 8 is set to a valid speed set TNC mode
  // otherwise use compiled in defaults

  value = EEPROM.read(8);

  if (value == 3)
  {
    Baud = 300;
    AFSK = TRUE;
    FSK = FALSE;
  }
  else if (value == 12)
  {
    Baud = 1200;
    AFSK = TRUE;
    FSK = FALSE;
  }
  else if (value == 24)
  {
    Baud = 2400;
    AFSK = TRUE;
    FSK = FALSE;
  }
  else if (value == 96)
  {
    Baud = 9600;	// FSK G3RUH
    AFSK = FALSE;
    FSK = TRUE;
  }

  // if EEPROM Reg 13 is set to a reasonable value set TNC centre freq
  // otherwise use compiled in defaults

  value = EEPROM.read(13);

  if (value > 90 && value < 250)
    centreFreq = value * 10;

  RXLevel = EEPROM.read(9);
  TXLevel = EEPROM.read(10);

  CommonSetup();
  print_mac();

#ifdef TFT
  TFTprintf("Packet TNC based on Soundmodem by Thomas Sailer");
#endif
  MONprintf(VersionString);

  SaveEEPROM(0, VersionNo);

  WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());

  for (i = 0; i < 16; i++)
  {
    // read a byte from the current address of the EEPROM
    int value = EEPROM.read(i);
    WriteDebugLog(7, "%d\t%d", i, value);
  }

  if (PSK)
  {
#ifdef TFT
    TFTprintf("PSK Mode");
#endif
    MONprintf("PSK Mode");
  }
  else if (FSK)
  {
#ifdef TFT
    TFTprintf("9600 FSK Mode");
#endif
    MONprintf("9600 FSK Mode");
  }
  else
  {
#ifdef TFT
    TFTprintf("AFSK Mode %d Baud Centre Freq %d", Baud, centreFreq);
#endif
    MONprintf("AFSK Mode %d Baud Centre Freq %d", Baud, centreFreq);
  }

  if (RCM_SRS0 & 0X20)		// Watchdog Reset
    WriteDebugLog(LOGCRIT, "\n**** Reset by Watchdog ++++");

  SoundModemInit();

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

  WriteDebugLog(LOGDEBUG, "DAC Baseline %d", sum / 1024);

#ifdef PIPOARD

  // Can read DAC output to determine Vref

  SetPot(1, 256);				// TX Level Gain = 1

  delay(100);

  analogRead(17);

  for (i = 0; i < 100; i++)
  {
    VRef += analogRead(17);
  }
  VRef /= 100;
  MONprintf("VREF %d offset %d\r\n", VRef, VRef - 32768);

  analogRead(16);		// Set ADC back to A0
#endif

  SetPot(1, TXLevel);
  WriteDebugLog(LOGDEBUG, "TXLevel %d = %d mV", TXLevel, (TXLevel * 3000) / 256);

  if (RXLevel)
    SetPot(0, RXLevel);
  else
    SetPot(0, 256);					// Start at min gain

  setupPDB(samplerate);
  setupDAC();
  setupADC(16);
  StartADC();

  WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());

  //	if (PSK)
  //		RunPSKReceive();		// This does not return!!!!
}


void loop()
{
  mainLoop();
  PlatformSleep();
  Sleep(1);
  HostPoll();
#if defined I2CKISS || defined I2CMONITOR
  i2cloop();
#endif
}


extern "C" char* sbrk(int incr);

extern char *__brkval;
extern char __bss_end;


// function from the sdFat library (SdFatUtil.cpp)
// licensed under GPL v3
// Full credit goes to William Greiman.
int FreeRam()
{
  char top;

  return __brkval ? &top - __brkval : &top - &__bss_end;
}

// Function to get mac/serial number

uint8_t mac[4];

// http://forum.pjrc.com/threads/91-teensy-3-MAC-address

void readserialno(uint8_t *mac)
{
  noInterrupts();

  // With 3.6 have to read a block of 8 bytes at zero
  // With 3.1 two blocks of 4 at e and f

  FTFL_FCCOB0 = 0x41;             // Selects the READONCE command
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
  FTFL_FCCOB1 = 0;             // read the given word of read once area
#else
  FTFL_FCCOB1 = 0x0F;          // read the given word of read once area
#endif
  FTFL_FCCOB2 = 0;
  FTFL_FCCOB3 = 0;
  // launch command and wait until complete
  FTFL_FSTAT = FTFL_FSTAT_CCIF;
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF));

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)

  // Need last 4

  *(mac++) = FTFL_FCCOB8;
  *(mac++) = FTFL_FCCOB9;
  *(mac++) = FTFL_FCCOBA;
  *(mac++) = FTFL_FCCOBB;
#else

  // Need first 4

  *(mac++) = FTFL_FCCOB4;
  *(mac++) = FTFL_FCCOB5;
  *(mac++) = FTFL_FCCOB6;
  *(mac++) = FTFL_FCCOB7;
#endif

  interrupts();
}

void print_mac()
{
  readserialno(mac);
  WriteDebugLog(7, "Hardware Serial No %02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3]);
}
extern "C"
{
  unsigned short * SendtoCard(unsigned short * buf, int n);
  extern unsigned short * DMABuffer;
  extern int Number, totSamples;

  void SampleSink(short Sample)
  {
    int work = (short)Sample;

    DMABuffer[Number++] = (work + 32768) >> 4; // 12 bit left justify

    if (Number == SendSize)
    {
      // send this buffer to sound interface

      //  printtick("Enter SendtoCard");
      DMABuffer = SendtoCard(DMABuffer, SendSize);
      //  printtick("Leave SendtoCard");
      Number = 0;
    }
    totSamples++;
  }



  int OKtoAdjustLevel()
  {
    // Only auto adjust level when disconnected.
    // Level is set at end of each received packet when connected

    return true;
  }

  void TurnroundLink()
  {
  }

  int AddTrailer()
  {
    return 0;
  }

  int displayDCD(int state)
  {
    SetLED(DCDLED, state);
    return true;
  }

  void StopCapture()
  {
    Capturing = FALSE;
  }

  void StartCapture()
  {
    Capturing = TRUE;

    //	printf("Start Capture\n");
  }
}
