// Arduino interface code for ARDOP running on a Teensy 3.6

#include "TeensyConfig.h"

#define TEENSY

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);


unsigned char RXBUFFER[500];	// Async RX Buffer. Enough for Stuffed Host Mode Frame

extern volatile int RXBPtr;
volatile int flag = 0;
volatile int flag2 = 0;

extern int VRef;

void yDisplayCall(int dirn, char * Call);
void yDisplayState(char * State);
void i2csetup();
void i2cloop();
void CommonSetup();
void setupPDB(int SampleRate);
void setupDAC();
void setupADC(int Pin);
extern "C" void StartDAC();
extern "C" void StopDAC();
void StartADC();

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
  void DisplayCall(int dirn, char * Call);
  void DisplayState(char * State);

  void PollReceivedSamples();
  void ProcessSCSPacket(unsigned char * rxbuffer, int Length);
  void SetPot(int address, unsigned int value);

#include "..\..\ARDOPC.h"

  extern unsigned int tmrPollOBQueue;

#define Now getTicks()

  unsigned int getTicks()
  {
    return millis();
  }
  void Sleep(int mS)
  {
    delay(mS);
  }
  void PlatformSleep()
  {
    noInterrupts();
    WDOG_REFRESH = 0xA602;
    WDOG_REFRESH = 0xB480;
    interrupts();
  }

  void txSleep(int mS)
  {
    // called while waiting for next TX buffer. Run background processes

    PollReceivedSamples();			// discard any received samples
    HostPoll();

    PlatformSleep();
    Sleep(mS);
  }

  void CatWrite(const uint8_t * Buffer, int Len)
  {
    Serial5.write(Buffer, Len);
  }

  unsigned char CatRXbuffer[256];
  int CatRXLen = 0;

  void RadioPoll()
  {
    int Length = Serial5.available();
    int i, val;

    // only try to read number of bytes in queue

    if (CatRXLen + Length > 256)
      CatRXLen = 0;

    Length = Serial5.readBytes(&CatRXbuffer[CatRXLen], Length);

    if (Length == 0)
      return;					// Nothing doing

    CatRXLen += Length;

    Length = CatRXLen;

    MONPORT.print("CAT RX ");
    for (i = 0; i < Length; i++)
    {
      val = CatRXbuffer[i];
      MONPORT.print(val, HEX);
      MONPORT.print(" ");
    }
    MONPORT.println("");
  }

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

#ifdef HOSTPORT
  HOSTPORT.begin(115200);
  while (!HOSTPORT);
#endif

  MONPORT.begin(115200);
  Serial5.begin(19200);				// CAT Port

  while (!MONPORT);

  // Set 10 second watchdog

  WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
  WDOG_TOVALL = 10000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
  WDOG_TOVALH = 0;
  WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
                  WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG

  WDOG_PRESC = 0; // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead of the default 1kHZ/4 = 200 HZ

  CommonSetup();

  MONPORT.printf("Monitor Buffer Space %d\r\n", MONPORT.availableForWrite());
#if defined HOSTPORT
  MONPORT.printf("Host Buffer Space %d\r\n", HOSTPORT.availableForWrite());
#elif defined I2CHOST
  MONPORT.printf("Host Connection is i2c on address %x Hex\r\n", I2CSLAVEADDR);
#endif
  if (RCM_SRS0 & 0X20)		// Watchdog Reset
    WriteDebugLog(LOGCRIT, "\n**** Reset by Watchdog ++++");

  WriteDebugLog(LOGALERT, "ARDOPC Version %s CPU %d Bus %d", ProductVersion, F_CPU, F_BUS);

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
  MONPORT.printf("VREF %d offset %d\r\n", VRef, VRef - 32768);

  analogRead(16);		// Set ADC back to A0
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
  RadioPoll();

  if (blnClosing)
  {
    CPU_RESTART // reset processor
  }
}

extern "C" bool OKtoAdjustLevel()
{
  // Only auto adjust level when disconnected.
  // Level is set at end of each received packet when connected

  return (ProtocolState == DISC);
}

