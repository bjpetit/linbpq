// Arduino interface code for Soundmodem running on a Teensie 3.1 or 3.6

// So far...
// 1200 works both ways at 12000K sampling, but RX not at 48k
// 9600 works at 48K
// 1200 Ok on Tom's board.
// 9600 uses about 14% of Teensy 3.1

// This file has to be in the Arduino user library folder TeensyConfig

#include "TeensyConfig.h"

#define TEENSY

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

#include <DMAChannel.h>
#include "SPI.h"

#define MONPORT Serial5

Print *tftp = NULL;

#include <EEPROM.h>

unsigned char RXBUFFER[300];	// Async RX Buffer

extern int Baud;		// Modem Speed (1200 or 9600 for noe)
extern int AFSK;		// Modem Mode

extern int VRef;

extern int TXLevel;				// 300 mV p-p Used on Teensy
extern int RXLevel;				// Configured Level - zero means auto tune
extern int autoRXLevel;			// calculated level

#define TRUE 1
#define FALSE 0

int loadCounter = 0;		// for performance monitor
int lastLoadTicks = 0;

int PKTLEDTimer = 0;

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
#define PKTLED LED1			// flash when packet received
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

#define Now getTicks()

  void _getpid()		// Seem to be needed to satifay linker
  {
  }

  void _kill()
  {
  }

  unsigned int getTicks()
  {
    return millis();
  }
  void Sleep(int mS)
  {
#if 0
    int loadtime = millis() - lastLoadTicks;

    loadCounter += mS;

    if (loadtime > 999)
    {
      WriteDebugLog(7, "Load = %d" , 100 - (100 * loadCounter / loadtime));
      lastLoadTicks = millis();
      loadCounter = 0;
    }
#endif
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

#define MEM_LEN 512
  extern uint8_t databuf[MEM_LEN];
  extern volatile int i2cputptr, i2cgetptr;

  void HostPoll()
  {
    // Called roughly once every milisecond

#ifdef I2CHOST

    int RXBPtr = 0;

    while (i2cgetptr != i2cputptr)
    {
      unsigned char c;
      c = databuf[i2cgetptr++];
      i2cgetptr &= 0x1ff;				// 512 cyclic

      RXBUFFER[RXBPtr++] = c;

      if (i2cgetptr == i2cputptr || RXBPtr > 498)
      {
        ProcessKISSPacket(RXBUFFER, RXBPtr);
        return;
      }
    }
#else

    int RXBPtr = HOSTPORT.available();

    if (RXBPtr)
    {
      int Count;
      Count = HOSTPORT.readBytes((char *)RXBUFFER, RXBPtr);
      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessKISSPacket(RXBUFFER, RXBPtr);
    }
#endif
    if (PKTLEDTimer)
    {
      PKTLEDTimer--;
      if (PKTLEDTimer == 0)
        SetLED(PKTLED, 0);				// turn off packet rxed led
    }
  }


  void setup()
  {
    uint32_t i, sum = 0;

#ifdef HOSTPORT
    HOSTPORT.begin(115200);
    while (!HOSTPORT);
#endif
#ifdef MONPORT
    MONPORT.begin(115200);
    while (!MONPORT);
#endif

    // Set 10 second watchdog

    WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
    WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
    WDOG_TOVALL = 10000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
    WDOG_TOVALH = 0;
    WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
                    WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG

    WDOG_PRESC = 0; // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead of the default 1kHZ/4 = 200 HZ

    Baud = 9600;	// FSK G3RUH
    AFSK = FALSE;

    //    Baud = 1200;
    //    AFSK = TRUE;

    CommonSetup();

    MONPORT.printf("Monitor Buffer Space %d\r\n", MONPORT.availableForWrite());
#if defined HOSTPORT
    MONPORT.printf("Host Buffer Space %d\r\n", HOSTPORT.availableForWrite());
#elif defined I2CHOST
    MONPORT.printf("Host Connection is i2c on address %x Hex\r\n", I2CSLAVEADDR);
#endif
    MONPORT.print("Hardware Serial No ");
    print_mac();
    MONPORT.println("");

    if (tftp)
      tftp->println("Packet TNC based on Soundmodem by Thomas Sailer");
    MONPORT.println("Packet TNC based on Soundmodem by Thomas Sailer");

    WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());

    MONPORT.print( "EEPROM length: " );
    MONPORT.println( EEPROM.length() );

    for (i = 0; i < 16; i++)
    {
      // read a byte from the current address of the EEPROM
      int value = EEPROM.read(i);

      MONPORT.print(i);
      MONPORT.print("\t");
      MONPORT.print(value, DEC);
      MONPORT.println();
    }

    if (Baud == 9600)
    {
      if (tftp)
        tftp->println("9600 FSK Mode");
      MONPORT.println("9600 FSK Mode");
    }
    else
    {
      if (tftp)
        tftp->printf("AFSK Mode %d Baud", Baud);

      MONPORT.printf("AFSK Mode %d Baud", Baud);
    }

    if (RCM_SRS0 & 0X20)		// Watchdog Reset
      WriteDebugLog(LOGCRIT, "\n**** Reset by Watchdog ++++");

    SoundModemInit();

    // Configure the ADC and run at least one software-triggered
    // conversion.  This completes the self calibration stuff and
    // leaves the ADC in a state that's mostly ready to use

    analogReadRes(16);
    //    analogReference(INTERNAL); // range 0 to 1.2 volts
    analogReference(DEFAULT); // range 0 to 3.3 volts
    //analogReadAveraging(8);
    // Actually, do many normal reads, to start with a nice DC level

    for (i = 0; i < 1024; i++)
    {
      sum += analogRead(16);
    }

    WriteDebugLog(LOGDEBUG, "DAC Baseline %d", sum / 1024);

    if (Baud == 9600)
      setupPDB(48000);			// 48K sample rate
    else
      setupPDB(12000);			// 12K sample rate

    /*
       // Read Vref

       SetPot(1, 256);				// TX Level Gain = 1
       delay(100);
       analogRead(17);

       VRef = 0;

       for (i = 0; i < 100; i++)
       {
         VRef += analogRead(17);
       }

       VRef /= 100;
    */

    MONPORT.printf("VREF %d offset %d\r\n", VRef, VRef - 32768);

    analogRead(16);		// Set ADC back to A0

    setupDAC();
    setupADC(16);
    StartADC();

    WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());
  }


  void loop()
  {
    mainLoop();
    PlatformSleep();
    Sleep(1);
#ifdef i2cSlaveSupport
    i2cloop();
#endif
  }
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

void print_mac()  {
  size_t count = 0;
  readserialno(mac);
  for (uint8_t i = 0; i < 4; ++i)
  {
    if (i != 0) count += MONPORT.print(":");
    count += MONPORT.print((*(mac + i) & 0xF0) >> 4, 16);
    count += MONPORT.print(*(mac + i) & 0x0F, 16);
  }
}

extern "C" bool OKtoAdjustLevel()
{
  // Only auto adjust level when disconnected.
  // Level is set at end of each received packet when connected

  return true;
}





