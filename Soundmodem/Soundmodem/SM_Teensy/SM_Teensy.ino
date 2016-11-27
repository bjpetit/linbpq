// Arduino interface code for Soundmodem running on a Teensie 3.1 or 3.6


// So far...
// 1200 works both ways at 12000K sampling, but RX not at 48k
// 9600 works at 48K
// 1200 Ok on Tom's board.
// 9600 uses about 14% of Teensy 3.1

#define TEENSY

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

#include <DMAChannel.h>
#include "SPI.h"
#include "ILI9341_t3.h"
#include <EEPROM.h>

DMAChannel dma1(false);

unsigned char RXBUFFER[300];	// Async RX Buffer

extern int Baud;		// Modem Speed (1200 or 9600 for noe)
extern int AFSK;		// Modem Mode

#define TRUE 1
#define FALSE 0

int loadCounter = 0;		// for performance monitor
int lastLoadTicks = 0;

int PKTLEDTimer = 0;

extern ILI9341_t3 tft;

extern volatile int RXBPtr;
volatile int flag = 0;
volatile int flag2 = 0;
extern int inIndex;			// ADC Buffer half being used 0 or 1

void yDisplayCall(int dirn, char * Call);
void yDisplayState(char * State);

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

#define pttPin 6

#define LED0 24
#define LED1 25
#define LED2 26
#define LED3 31

#define DCDLED LED0
#define PKTLED LED1			// flash when packet received

#define SW1 27
#define SW2 28
#define SW3 29
#define SW4 30

// CAT4016 10 LED display

#define CLK 2
#define BLANK 3
#define LATCH 4
#define SIN 5

extern "C"
{
  void WriteDebugLog(int Level, const char * format, ...);
  void InitSound();
  void HostInit();
  void CheckTimers();
  void HostPoll();
  void MainPoll();
  void InitDMA();
  void xStartDAC();
  void xStartADC();
  void DisplayCall(int dirn, char * Call);
  void DisplayState(char * State);
  void SoundModemInit();
  void PollReceivedSamples();
  void ProcessKISSPacket(unsigned char * rxbuffer, int Length);
  void mainLoop();

  extern unsigned int tmrPollOBQueue;

  extern volatile unsigned short ADC_Buffer[2 * ADC_SAMPLES_PER_BLOCK];

#define Now getTicks()

  void _getpid()		// Seem to be needed to satifay linker
  {
  }

  void _kill()
  {
  }

  void xStartDAC()
  {
    StartDAC();
  }

  void xstopDAC()
  {
    stopDAC();
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

    if (flag == 1)
    {
      WriteDebugLog(LOGDEBUG, "adc Interrupt %d %d &d ", millis(), dma1.TCD->CITER_ELINKNO,
                    ADC_Buffer[dma1.TCD->CITER_ELINKNO]);
      flag = 0;
    }
    PlatformSleep();
    Sleep(mS);
  }

  void SerialSendData(const uint8_t * Msg, int Len)
  {
    Serial.write(Msg, Len);
  }

  void Serial1Print(char * Msg, int Len)
  {
    Serial1.println(Msg);
  }

  void SetLED(int Pin, int State)
  {
    digitalWrite(Pin, State);
  }

  void HostPoll()
  {
    // Called roughly once every milisecond

    int RXBPtr = Serial.available();

    if (RXBPtr)
    {
      int Count;
      RXBUFFER[RXBPtr] = 0;
      Count = Serial.readBytes((char *)RXBUFFER, RXBPtr);
      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessKISSPacket(RXBUFFER, RXBPtr);
    }

    if (PKTLEDTimer)
    {
      PKTLEDTimer--;
      if (PKTLEDTimer == 0)
        SetLED(PKTLED, 0);				// turn off packet rxed led
    }
  }

  int GetDMAPointer()
  {
    return dma1.TCD->CITER_ELINKNO;
  }
  int GetADCDMAPointer()
  {
    return dma1.TCD->CITER_ELINKNO;
  }
}
void setup()
{
  uint32_t i, sum = 0;

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

  Baud = 1200;
  AFSK = TRUE;

  pinMode(13, OUTPUT);				// onboard LED
  pinMode(pttPin, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode (SW1, INPUT_PULLUP);
  pinMode (SW2, INPUT_PULLUP);
  pinMode (SW3, INPUT_PULLUP);
  pinMode (SW4, INPUT_PULLUP);

  pinMode(CLK, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(SIN, OUTPUT);


  setupTFT();
  Serial.begin(115200);
  Serial1.begin(115200);

  Serial1.print("Hardware Serial No ");
  print_mac();
  Serial1.println("");

  tft.println("Packet TNC based on Soundmodem by Thomas Sailer");
  Serial1.println("Packet TNC based on Soundmodem by Thomas Sailer");

  WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());

  Serial1.print( "EEPROM length: " );
  Serial1.println( EEPROM.length() );

  // if (EEPROM.read(1) != 100)
  // 	EEPROM.write(1, 100);

  //if (EEPROM.read(2) != 200)
  // 	EEPROM.write(2, 200);

  for (i = 0; i < 16; i++)
  {
    // read a byte from the current address of the EEPROM
    int value = EEPROM.read(i);

    Serial1.print(i);
    Serial1.print("\t");
    Serial1.print(value, DEC);
    Serial1.println();
  }
  Serial1.println(EEPROM[2]);
  if (Baud == 9600)
  {
    tft.println("9600 FSK Mode");
    Serial1.println("9600 FSK Mode");
  }
  else
  {
    tft.print(Baud);
    tft.println(" AFSK  Mode");
    Serial1.print(Baud);
    Serial1.println(" Baud AFSK  Mode");
  }
  dma1.begin(true);

  if (RCM_SRS0 & 0X20)		// Watchdog Reset
    WriteDebugLog(LOGCRIT, "\n**** Reset by Watchdog ++++");

  SoundModemInit();

  // Configure the ADC and run at least one software-triggered
  // conversion.  This completes the self calibration stuff and
  // leaves the ADC in a state that's mostly ready to use

  analogReadRes(16);
  analogReference(INTERNAL); // range 0 to 1.2 volts
  //analogReference(DEFAULT); // range 0 to 3.3 volts
  //analogReadAveraging(8);
  // Actually, do many normal reads, to start with a nice DC level

  for (i = 0; i < 1024; i++)
  {
    sum += analogRead(16);
  }

  StartDAC();
  stopDAC();
  setupADC(16);
  StartADC();

  WriteDebugLog(7, "CPU %d Bus %d FreeRAM %d", F_CPU, F_BUS, FreeRam());

  // Clear CAT4016

  digitalWriteFast(BLANK, 0);	// Enable display
  digitalWriteFast(LATCH, 0);	// Strobe High to copy data from shift reg to display
  digitalWriteFast(CLK, 0);		// Strobe High to enter data to shift reg

  CAT4016(0);				// All off
}


void loop()
{
  mainLoop();
  PlatformSleep();
  Sleep(1);
}

// DMA COde is here as it is more difficult to use the Arduino interface stuff
// from C (Arduino .ino files are compiles as c++)

// DAC uses DMA, trigged by the PDB.

// So far I can't get PDB+ADC+DMA to work, so I use PDB to generate an interrupt,
// which software triggers the ADC, which then used DMA to save the result on completion.

volatile uint16_t dac1_buffer[DAC_SAMPLES_PER_BLOCK * 2];

#define PDB_CONFIG_DAC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)
#define PDB_CONFIG_ADC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE)

//	PDB count of 4999 should give 12K clock with 60 MHz bus clock

//	 We use 12K of AFSK and 48K for 9600 (for now - may optimise)

#define PDB_PERIOD_12000 (F_BUS/12000 - 1)
#define PDB_PERIOD_24000 (F_BUS/24000 - 1)
#define PDB_PERIOD_48000 (F_BUS/48000 - 1)



void StartDAC()
{
  // Set up the DAC and start sending a frame under DMA

  dma1.disable();

  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // enable the DAC module, 3.3V reference

  SIM_SCGC6 |= SIM_SCGC6_PDB;

  PDB0_IDLY = 1;

  if (AFSK)
    PDB0_MOD = PDB_PERIOD_12000;
  else
    PDB0_MOD = PDB_PERIOD_48000;

  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_SWTRIG;

  dma1.TCD->SADDR = dac1_buffer;
  dma1.TCD->SOFF = 2;
  dma1.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BIT);
  dma1.TCD->NBYTES_MLNO = 2;	// Bytes per minor loop
  dma1.TCD->SLAST = -sizeof(dac1_buffer);	// Reinit to start
  dma1.TCD->DADDR = &DAC0_DAT0L;
  dma1.TCD->DOFF = 0;
  dma1.TCD->CITER_ELINKNO = sizeof(dac1_buffer) / 2;
  dma1.TCD->DLASTSGA = 0;
  dma1.TCD->BITER_ELINKNO = sizeof(dac1_buffer) / 2;
  //  dma1.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
  DMAMUX0_CHCFG0 = DMAMUX_SOURCE_PDB | DMAMUX_ENABLE;

  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);

  dma1.enable();
  DMA_CINT = DMA_CINT_CAIR;
}

void DACisr()
{
  DMA_CINT = DMA_CINT_CAIR;
  flag2 = 1;
}

void stopDAC()
{
  dma1.disable();
  StartADC();
  DAC0_DAT0L = 0;
  DAC0_DATH = 8;		// Set output to mid value
}

uint16_t dc_average;

void setupADC(int pin)
{
  uint32_t i, sum = 0;

  // pin must be 0 to 13 (for A0 to A13)
  // or 14 to 23 for digital pin numbers A0-A9
  // or 34 to 37 corresponding to A10-A13
  if (pin > 23 && !(pin >= 34 && pin <= 37)) return;

  // Configure the ADC and run at least one software-triggered
  // conversion.  This completes the self calibration stuff and
  // leaves the ADC in a state that's mostly ready to use

  analogReadRes(16);
  //analogReference(INTERNAL); // range 0 to 1.2 volts
  analogReference(DEFAULT); // range 0 to 3.3 volts
  //analogReadAveraging(8);
  // Actually, do many normal reads, to start with a nice DC level
  for (i = 0; i < 1024; i++) {
    sum += analogRead(pin);
  }
  dc_average = sum >> 10;

  ADC0_SC2 |= ADC_SC2_DMAEN;
  NVIC_ENABLE_IRQ(IRQ_PDB);
}

void pdb_isr()
{
  ADC0_SC1A = 8; // Trigger read on A2
  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt flag
}

void StartADC()
{
  dma1.disable();

  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_SWTRIG | PDB_SC_LDOK;

  dma1.TCD->SADDR = &ADC0_RA;
  dma1.TCD->SOFF = 0;
  dma1.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  dma1.TCD->NBYTES_MLNO = 2;
  dma1.TCD->SLAST = 0;
  dma1.TCD->DADDR = ADC_Buffer;
  dma1.TCD->DOFF = 2;
  dma1.TCD->CITER_ELINKNO = sizeof(ADC_Buffer) / 2;
  dma1.TCD->DLASTSGA = -sizeof(ADC_Buffer);
  dma1.TCD->BITER_ELINKNO = sizeof(ADC_Buffer) / 2;
  dma1.TCD->CSR = 0; //DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);

  inIndex = 0;
  dma1.enable();
}




extern "C" char* sbrk(int incr);

extern char *__brkval;
extern char __bss_end;


// function from the sdFat library (SdFatUtil.cpp)
// licensed under GPL v3
// Full credit goes to William Greiman.
int FreeRam() {
  char top;

  //   return &top -sbrk(0);
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
    if (i != 0) count += Serial1.print(":");
    count += Serial1.print((*(mac + i) & 0xF0) >> 4, 16);
    count += Serial1.print(*(mac + i) & 0x0F, 16);
  }
}

void CAT4016(int value)
{
  // writes value to the 10 LED display
  int i;

  for (i = 0; i < 16; i++)			// must send all 16 to maintain sync
  {
    // Send each bit to display

    // We probbly don't need a microsecond delay, but I doubt if it will
    // cauise timing problems anywhere else

    // looks like we need to send data backwards (hi order bit first)

    digitalWriteFast(SIN, (value >> 15) & 1);
    //   __asm("nop");
    delayMicroseconds(1);		// Setup is around 20 nS
    digitalWriteFast(CLK, 1);		// Copy SR to Outputs
    delayMicroseconds(1);		// Setup is around 20 nS
    digitalWriteFast(CLK, 0);		// Strobe High to copy data from shift reg to display
    value = value << 1;			// ready for next bit
    delayMicroseconds(1);		// Setup is around 20 nS
  }
  // copy Shift Reg to display

  digitalWriteFast(LATCH, 1);		//  Strobe High to copy data from shift reg to display
  delayMicroseconds(1);					// Setup is around 20 nS
  digitalWriteFast(LATCH, 0);
}





