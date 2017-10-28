
// Arduino interface code for ARDOP running on a Teensy 3.6

#include "TeensyConfig.h"
#include "TeensyCommon.h"

#ifndef ARDOP
#error("ARDOP not defined in TeensyCommon.h");
#endif

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

unsigned char RXBUFFER[500];	// Async RX Buffer. Enough for Stuffed Host Mode Frame

extern volatile int RXBPtr;
volatile int flag = 0;
volatile int flag2 = 0;
int SerialHost = TRUE;				// Will eventually allow switching from Serial to I2c without reconfig

int SerialWatchDog = 0;

extern int VRef;
extern int TXLevel;

void CommonSetup();
void setupPDB(int SampleRate);
void setupDAC();
void setupADC(int Pin);
extern "C" void StartDAC();
extern "C" void StopDAC();
void StartADC();
void setupOLED();
void setupKMR_18();

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
  void AdjustTXLevel(int Level);

  void ProcessSCSPacket(unsigned char * rxbuffer, int Length);

#include "../../ARDOPC/ARDOPC.h"

  extern unsigned int tmrPollOBQueue;

#define Now getTicks()

#define MEM_LEN 512
  extern uint8_t databuf[MEM_LEN];
  extern volatile int i2cputptr, i2cgetptr;

  void HostPoll()
  {
#ifdef I2CHOST

    while (i2cgetptr != i2cputptr)
    {
      unsigned char c;
      c = databuf[i2cgetptr++];
      i2cgetptr &= 0x1ff;				// 512 cyclic

      RXBUFFER[RXBPtr++] = c;

      if (i2cgetptr == i2cputptr || RXBPtr > 498)
      {
        ProcessSCSPacket(RXBUFFER, RXBPtr);
        return;
      }
    }
#else
    int Avail = HOSTPORT.available();

    if (Avail)
    {
      int Count;

      if (Avail > (499 - RXBPtr))
        Avail = 499 - RXBPtr;

      Count = HOSTPORT.readBytes((char *)&RXBUFFER[RXBPtr], Avail);
      RXBPtr += Count;
      ProcessSCSPacket(RXBUFFER, RXBPtr);
    }
#ifdef i2cSlaveSupport
    i2cloop();					// I2C but not for Host
#endif
#endif
  }
}

void setup()
{
  uint32_t i, sum = 0;

#ifdef I2CHOST
  SerialHost = FALSE;
#endif

  CommonSetup();

  WriteDebugLog(LOGALERT, "ARDOPC Version %s CPU %d Bus %d FreeRAM %d", ProductVersion, F_CPU, F_BUS, FreeRam());
  blnTimeoutTriggered = FALSE;
  SetARDOPProtocolState(DISC);

  InitSound();
  HostInit();
  tmrPollOBQueue = Now + 10000;
  ProtocolMode = ARQ;

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

  setupPDB(12000);			// 12K sample rate
  setupDAC();
  setupADC(16);
  StartADC();

  // Read Vref

  SetPot(1, 256);				// TX Level Gain = 1

  delay(100);

  analogRead(17);

  for (i = 0; i < 100; i++)
  {
    VRef += analogRead(17);
  }
  VRef /= 100;
  analogRead(16);		// Set ADC back to A0

  WriteDebugLog(0, "VREF %d offset %d", VRef, VRef - 32768);

  AdjustTXLevel(TXLevel);

#ifdef PLOTCONSTELLATION
#ifdef OLED
  setupOLED();
#endif
#ifdef WDTTFT
  setupWDTTFT();
#endif
#ifdef KMR_18
  setupKMR_18();
#endif

#endif
}

int lastticks = Now;

void loop()
{
  PollReceivedSamples();
  CheckTimers();
  HostPoll();
  MainPoll();

  if (Now - lastticks > 999)
  {
    // Debug 1 sec tick
  }
  PlatformSleep();
  //RadioPoll();
  Sleep(1);

#ifdef I2CHOST
  SerialWatchDog++;		// Reset if nothing for 60 secs
#endif

  if (blnClosing || SerialWatchDog > 60000)
  {
    CPU_RESTART // reset processor
  }
}

extern "C"
{
  int OKtoAdjustLevel()
  {
    // Only auto adjust level when disconnected.
    // Level is set at end of each received packet when connected

    return (ProtocolState == DISC);
  }

  void StartCapture()
  {
    Capturing = TRUE;
    DiscardOldSamples();
    ClearAllMixedSamples();
    State = SearchingForLeader;
  }
  void StopCapture()
  {
    Capturing = FALSE;
  }
  void TurnroundLink()
  {
    if (blnEnbARQRpt > 0 || blnDISCRepeating)	// Start Repeat Timer if frame should be repeated
      dttNextPlay = Now + intFrameRepeatInterval;
  }
}



// function from the sdFat library (SdFatUtil.cpp)
// licensed under GPL v3
// Full credit goes to William Greiman.

extern "C" char* sbrk(int incr);

extern char *__brkval;
extern char __bss_end;


int FreeRam()
{
  char top;

  return __brkval ? &top - __brkval : &top - &__bss_end;
}

int _gettimeofday()
{
  return 0;
}



