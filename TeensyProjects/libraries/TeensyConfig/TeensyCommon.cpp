// Common Arduino interface code for modems running on a Teensy 3.6

// Currently used by ARDOP and Packet TNC

#include <EEPROM.h>
#include "TeensyConfig.h"
#include "TeensyCommon.h"

// This seems to be the only way to change the serial port buffer size
// without editing the IDE core file. We set the equates then include the
// core library source

// We need bigger buffers if we want to use a hardware serial port for the host
// interface (to avoid buffering delays)

// We need to be able to queue a full KISS or Hostmode frame to the host without
// waiting

#define SERIAL1_TX_BUFFER_SIZE	512 // number of outgoing bytes to buffer
#define SERIAL1_RX_BUFFER_SIZE	512

#include "serial1.c"

void CAT4016(int value);
void setupTFT();


// extern "C" {#include "..\..\ARDOPC.h"}

#include "SPI.h"

void StartADC();
void setupDAC();
void setupADC();
void i2csetup();

#include <DMAChannel.h>

DMAChannel dma1(true);
DMAChannel dma2(true);

int VRef = 32768;				// ADC and ADC reference (ideal is 32678)

unsigned int PKTLEDTimer = 0;

#define PKTLED LED3				// flash when packet received


void CommonSetup()
{	
#ifdef HOSTPORT
  HOSTPORT.begin(115200);
#endif
#ifdef MONPORT
  MONPORT.begin(115200);
  while (!MONPORT);
#endif
#ifdef CATPORT
  CATPORT.begin(CATSPEED);				// CAT Port
#endif

  // Set 10 second watchdog

  WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
  WDOG_TOVALL = 10000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
  WDOG_TOVALH = 0;
  WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
                  WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG

  WDOG_PRESC = 0; // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead of the default 1kHZ/4 = 200 HZ

#ifdef MONPORT
  MONPORT.printf("Monitor Buffer Space %d\r\n", MONPORT.availableForWrite());
#endif
#if defined HOSTPORT
  MONprintf("Host Buffer Space %d\r\n", HOSTPORT.availableForWrite());
#elif defined I2CHOST
  MONprintf("Host Connection is i2c on address %x Hex\r\n", I2CSLAVEADDR);
#endif

  if (RCM_SRS0 & 0X20)		// Watchdog Reset
    WriteDebugLog(LOGCRIT, "\n**** Reset by Watchdog ++++");

  pinMode(pttPin, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Flash Leds to show starting

  SetLED(LED0, 1);
  SetLED(LED1, 1);
  SetLED(LED2, 1);
  SetLED(LED3, 1);
  delay(200);
  SetLED(LED0, 0);
  delay(200);
  SetLED(LED1, 0);
  delay(200);
  SetLED(LED2, 0);
  delay(200);
  SetLED(LED3, 0);

#ifdef WDTBOARD

  pinMode (SW1, INPUT_PULLUP);
  pinMode (SW2, INPUT_PULLUP);
  pinMode (SW3, INPUT_PULLUP);
  pinMode (SW4, INPUT_PULLUP);

#endif
#ifdef BARLEDS

  pinMode(CLK, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(SIN, OUTPUT);

  // Clear CAT4016

  digitalWriteFast(BLANK, 0);	// Enable display
  digitalWriteFast(LATCH, 0);	// Strobe High to copy data from shift reg to display
  digitalWriteFast(CLK, 0);		// Strobe High to enter data to shift reg

  CAT4016(0);				// All off
#endif

#if defined I2CHOST || defined I2CKISS || defined I2CMONITOR
  i2csetup();
#endif

#ifdef SPIPOTS
  pinMode (SPIPOTCS, OUTPUT);	// SPI Pot
  digitalWriteFast (SPIPOTCS, HIGH);
#endif

#if defined(SPIPOTS) || defined (TFT)
  SPI.begin();
#endif

#ifdef TFT
  setupTFT();
#endif

  // Set intial TX and RX Levels

  if (RXLevel)			// Zero means auto set level
    AdjustRXLevel(RXLevel);
  else
    AdjustRXLevel(autoRXLevel);

  AdjustTXLevel(TXLevel);
}

extern "C"
{
  int LastNow;
  


  unsigned int getTicks()
  {
    return millis();
  }
  
   void printtick(char * msg)
  {
    Serial.printf("%s %i\r\n", msg, Now - LastNow);
    LastNow = Now;
  }

  int loadCounter = 0;		// for performance monitor
  int lastLoadTicks = 0;

  void Sleep(int mS)
  {
#if 0
    int loadtime = millis() - lastLoadTicks;

    loadCounter += mS;

    if (loadtime > 999)
    {
      Serial.printf("Load = %d\r\n" , 100 - (100 * loadCounter / loadtime));
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

	if (PKTLEDTimer && Now > PKTLEDTimer)
    {
      PKTLEDTimer = 0;
      SetLED(PKTLED, 0);				// turn off packet rxed led
    }
  }

  void txSleep(int mS)
  {
    // called while waiting for next TX buffer. Run background processes

    PollReceivedSamples();			// discard any received samples
    HostPoll();
    PlatformSleep();
    Sleep(mS);
  }



// Code to access Digital Pots (used for setting input and output levels)

// PI Board uses SPI, WDT Board uses i2c

#ifdef HASPOTS
  void AdjustRXLevel(int Level)
  {
    int Pot = (Level * 256) / 3000;

    WriteDebugLog(LOGINFO, "Adjusting RX Level %d mV Pot %d", Level, Pot);

    if (Level > 0)
    {
      // Zero means set level automatically

      SetPot(0, Pot);		// Write to live and nv regs
      //     SetPot(2, Pot);
    }
  }

  void AdjustTXLevel(int Level)
  {
    int Pot = (Level * 256) / 3000;

    WriteDebugLog(LOGINFO, "Adjusting TX Level %d mV Pot %d", Level, Pot);

    SetPot(1, Pot);		// Write to live
  }
#else
  void AdjustRXLevel(int Level)
  {}
  void AdjustTXLevel(int Level)
  {}
#endif

#ifdef SPIPOTS

  void SetPot(int address, unsigned int value)
  {
    int Command = address << 4;  // Reg addr to top 4 bits, write opcode is zero

    if (GetPot(address) == value)
      return;											// Don't write if same

    if (value & 0x100)						// 9th bit set?
      Command |= 1;								// goes in bottom bit of command

    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWriteFast(SPIPOTCS, LOW);

    SPI.transfer(Command);
    SPI.transfer(value);

    digitalWriteFast(SPIPOTCS, HIGH);
    // release control of the SPI port
    SPI.endTransaction();
  }

  unsigned int GetPot(int i)
  {
    byte thisRegister;
    unsigned int result = 0, result1 = 0;   // result to return

    thisRegister = (i << 4) | 0x0c;					// Read Command

    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

    digitalWrite(SPIPOTCS, LOW);

    // Device returns the top two bits of the value when address is sent

    result1 = SPI.transfer(thisRegister);
    // send a value of 0 to read the low 8 bits
    result = SPI.transfer(0x00);

    digitalWrite(SPIPOTCS, HIGH);
    SPI.endTransaction();

    return (result | (result1 << 8));
  }

#endif
#ifdef I2CPOTS

  void SetPot(int address, unsigned int value)
  {}

  unsigned int GetPot(int i)
  {}

#endif

#ifndef SPIPOTS
#ifndef I2CPOTS

  // Dummys for platforms without digital pots

  void SetPot(int address, unsigned int value)
  {}

  unsigned int GetPot(int i)
  {
    return 0;
  }
#endif
#endif
}


// LED Bargrath Code

void CAT4016(int value)
{
#ifdef BARLEDS
  // writes value to the 10 LED display
  int i;

  for (i = 0; i < 16; i++)			// must send all 16 to maintain sync
  {
    // Send each bit to display

    // We probably don't need a microsecond delay, but I doubt if it will
    // cause timing problems anywhere else

    // looks like we need to send data backwards (hi order bit first)

    digitalWriteFast(SIN, (value >> 15) & 1);
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
#endif
}
extern "C" void SaveEEPROM(int Reg, unsigned char Val)
{
  if (EEPROM.read(Reg) != Val)
    EEPROM.write(Reg, Val);
}

// DAC/ADC Code

// We use two buffers, but DMA treats as one long one, and we process as
// two halves

volatile uint16_t dac1_buffer[DAC_SAMPLES_PER_BLOCK * 2];
extern volatile uint16_t ADC_Buffer[ADC_SAMPLES_PER_BLOCK * 2];

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)

//	PDB count of 4999 should give 12K clock with 60 MHz bus clock

void setupPDB(int SampleRate)
{
  // Get PDB running to provide clock to DAC and ADC

  // DAC flow PDB->DMA->DAC
  // ADC flow PDB->DAC->DMA


  SIM_SCGC6 |= SIM_SCGC6_PDB;		// Enable PDB clock

  PDB0_IDLY = 1;
  PDB0_MOD = F_BUS / SampleRate - 1;

  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
  PDB0_CH0C1 = 0x0101;				// ?? PDB triggers ADC
}

void setupDAC()
{
  // Configure DAC

  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // enable the DAC module, 3.3V reference

  DAC0_DAT0L = 0;
  DAC0_DATH = 8;		// Set output to mid value (should ramp up??)
}
extern "C" void StartDAC()
{
  // Start sending a frame under DMA

  // We don't need to do all this each time, but probably not worth
  // working out which is needed

  dma1.disable();

  dma1.TCD->SADDR = dac1_buffer;
  dma1.TCD->SOFF = 2;
  dma1.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BIT);
  dma1.TCD->NBYTES_MLNO = 2;	// Bytes per minor loop
  dma1.TCD->SLAST = -sizeof(dac1_buffer);	// Reinit to start
  dma1.TCD->DADDR = &DAC0_DAT0L;					// Write to DAC data register
  dma1.TCD->DOFF = 0;
  dma1.TCD->CITER_ELINKNO = sizeof(dac1_buffer) / 2;
  dma1.TCD->DLASTSGA = 0;
  dma1.TCD->BITER_ELINKNO = sizeof(dac1_buffer) / 2;
  dma1.TCD->CSR = 0; //DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  dma1.enable();
}

extern "C" void stopDAC()
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

  analogRead(pin);

  dc_average = sum >> 10;

  ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;	// Hardware triggered

  // NVIC_ENABLE_IRQ(IRQ_PDB);
}

// not used, but leave for possible use for timing test

void pdb_isr()
{
  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt flag
}

void StartADC()
{

  // The ADC runs all the time. This resets the buffer pointers
  // after a transmit completes.

  dma2.disable();

  dma2.TCD->SADDR = &ADC0_RA;
  dma2.TCD->SOFF = 0;
  dma2.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  dma2.TCD->NBYTES_MLNO = 2;
  dma2.TCD->SLAST = 0;
  dma2.TCD->DADDR = ADC_Buffer;
  dma2.TCD->DOFF = 2;
  dma2.TCD->CITER_ELINKNO = sizeof(ADC_Buffer) / 2;
  dma2.TCD->DLASTSGA = -sizeof(ADC_Buffer);
  dma2.TCD->BITER_ELINKNO = sizeof(ADC_Buffer) / 2;
  dma2.TCD->CSR = 0; //DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);

  inIndex = 0;
  dma2.enable();
}
extern "C"
{
  int GetDMAPointer()
  {
    return dma1.TCD->CITER_ELINKNO;
  }
  int GetADCDMAPointer()
  {
    return dma2.TCD->CITER_ELINKNO;
  }
  void SetLED(int LED, int state)
  {
#ifdef PIBOARD
    state = !state;			// LEDS inverted on PI Board
#endif
    digitalWriteFast(LED, state);
  }

  BOOL KeyPTT(BOOL blnPTT)
  {
    digitalWriteFast(pttPin, blnPTT);
    return TRUE;
  }


}

// Code to drive a Adafruit/PRJC or 3.5" 480*320 PI TFT display

#ifdef TFT

#ifdef PI35TFT

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

// PI35TFT on Teensy

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 9
#define _rst 8

// Color definitions
#define	ILI9341_BLACK   0x0000
#define	ILI9341_BLUE    0x001F
#define	ILI9341_RED     0xF800
#define	ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_WHITE   0xFFFF

Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

#else

// PRJC/Adafruit Display

#include "ILI9341_t3.h"

#define TFT_DC  9
#define TFT_CS 10
#define _RST 255		// Not Used
#define _MOSI 11
#define _SCLK 14		// Clock moved to ALT pin as LED is on A13
#define _MISO 12

// Use hardware SPI

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, _RST, _MOSI, _SCLK, _MISO);
#endif

static Print * tftptr = NULL;


void setupTFT()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tftptr = &tft;
}

 // Write to tft. Gets round problem of different TFT Hardware
 
 void TFTprintf(const char * format, ...)
  {
    char Mess[256];
    va_list(arglist);
    va_start(arglist, format);
    vsnprintf(Mess, sizeof(Mess), format, arglist);
    tft.print(Mess);
    return;
  }




extern "C"
{
  void displayCall(int dirn, char * Call)
  {
    char paddedcall[12] = "           ";

    paddedcall[0] = dirn;
    memcpy(paddedcall + 1, Call, strlen(Call));

    if (tftptr)
    {
      tft.setCursor(0, 72);
      tft.print(paddedcall);
    }
  }

  void displayState(const char * State)
  {
    if (tftptr)
    {
      tft.setCursor(0, 48);
      tft.print("          ");
      tft.setCursor(0, 48);
      tft.print(State);
    }
  }
}
#else

// Dummy routines

extern "C" void displayCall(int dirn, char * Call)
{}
extern "C" void displayState(const char * State)
{}

#endif

// Signal levels for each bar

const int barlevels[10] = {
  1000, 2000, 5000, 8000, 11000,
  16000, 24000, 28000, 30000, 32000
};
#ifdef TFT
const int barcolours[10] = {
  ILI9341_YELLOW, ILI9341_YELLOW, ILI9341_GREEN,
  ILI9341_GREEN, ILI9341_GREEN, ILI9341_GREEN, ILI9341_GREEN,
  ILI9341_RED, ILI9341_RED, ILI9341_RED
};
#endif
const int CAT4016Levels[11] = {
  0, 1, 0b11, 0b111, 0b1111, 0b11111,
  0b111111, 0b1111111, 0b11111111, 0b111111111, 0b1111111111
};


extern "C"
{
  void displayLevel(int level)
  {
    int i;
#ifdef TFT
    for (i = 0; i < 10; i++)
    {
      if (level > barlevels[i])
        tft.fillRect(15 * i, 100, 14, 16, barcolours[i]);
      else
        tft.fillRect(15 * i, 100, 14, 16, ILI9341_BLACK);
    }
#endif
    for (i = 0; i < 10; i++)
    {
      if (level < barlevels[i])
        break;
    }
    CAT4016(CAT4016Levels[i]);
  }
}

// Routine to adjust the RX gain pot to keep input level in range

extern "C"
{
  void CheckandAdjustRXLevel(int maxlevel, int minlevel, bool Force)
  {
    int pktopk = (maxlevel - minlevel) * 3300 / 65536;
	
	if (RXLevel)
		return;						// Only autoadjuest if level = 0)

    if (!OKtoAdjustLevel() && !Force)			// Protocol specific test
      return;

    // Try adjusting pot to get about 2000 mV

    if (pktopk > 3200)
    {
      // Overloaded so can't get accurate level. Set Gain to minimum

      autoRXLevel = 3000;
      AdjustRXLevel(autoRXLevel);
      return;
    }
    if (pktopk < 40)
    {
      // Assume no input
      WriteDebugLog(LOGINFO, "Level below threshold - assume no input %d %d %d pk", maxlevel, minlevel, pktopk);
      return;
    }

    if (pktopk < 1600 || pktopk > 2400)
    {
      // Calculate actual input voltage from current pot gain setting

      pktopk = pktopk * autoRXLevel / 3000;

      WriteDebugLog(LOGINFO, "peak to peak input %d mV", pktopk);

      autoRXLevel = pktopk  * 3 / 2;			// try to get to 2/3rd

      if (autoRXLevel > 3000)
        autoRXLevel = 3000;

      AdjustRXLevel(autoRXLevel);
    }
  }

  void CatWrite(const uint8_t * Buffer, int Len)
  {
#ifdef CATPORT
    CATPORT.write(Buffer, Len);
#endif
  }

  unsigned char CatRXbuffer[256];
  int CatRXLen = 0;

  int RadioPoll()
  {
#ifdef CATPORT
    int Length = CATPORT.available();

    // only try to read number of bytes in queue

    if (CatRXLen + Length > 256)
      CatRXLen = 0;

    Length = CATPORT.readBytes(&CatRXbuffer[CatRXLen], Length);

    if (Length == 0)
      return 0;					// Nothing doing

    CatRXLen += Length;

    Length = CatRXLen;

    //    MONprintf("CAT RX ");
    //    for (i = 0; i < Length; i++)
    //      MONprintf("%x ", CatRXbuffer[i]);
    //    MONprintf("\r\n");
#endif
    return CatRXLen;
  }
}


// i2c support

#if defined I2CHOST || defined I2CKISS || defined I2CMONITOR

#include <i2c_t3.h>

void receiveEvent(size_t count);
void requestEvent(void);

#define MEM_LEN 512
uint8_t databuf[MEM_LEN];

volatile int i2cputptr = 0, i2cgetptr = 0, target = 0;

void i2csetup()
{
  // Setup for Slave mode, address I2SLAVEADDR, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, I2CSLAVEADDR, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

  // Data init

  memset(databuf, 0, sizeof(databuf));

  // register events

  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

unsigned char i2cMessage[512];
int i2cMsgPtr = 0;

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#ifdef I2CHOST

unsigned char i2creply[512] = {0xaa, 0};

volatile int i2creplyptr = 3;
volatile int i2creplylen = 0;

#else

unsigned char i2creply[512] = {192, 8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 3, 192};
unsigned char i2cidle[2] = {15};

volatile int i2creplyptr = 0;
volatile int i2creplylen = 14;

// This is only called if I2CKISS is enabled
void i2cloop()
{
  // print received data - this is done in main loop to keep time spent in I2C ISR to minimum

  while (i2cgetptr != i2cputptr)
  {
    unsigned char c;
    c = databuf[i2cgetptr++];
    i2cgetptr &= 0x1ff;				// 512 cyclic

    MONPORT.printf("%x %c ", c, c);

    if (c == FEND)
    {
      // Start or end of message

      int i, Len;

      if (i2cMsgPtr == 0)
        continue;							// Start of message

      Len = i2cMsgPtr;
      i2cMsgPtr = 0;

      // New message

      MONPORT.printf("Slave received: ");
      for (i = 0; i < Len; i++)
        MONPORT.printf("%x ", i2cMessage[i]);
      MONPORT.printf("\r\n");

      if (i2cMessage[0] == 15)
      {
        // Immediate Command

        if (i2cMessage[1] == 1)
        {
          // Get Params Command

          int Sum = 8, val;

          for (i = 0; i < 9; i++)
          {
            val = EEPROM.read(i);
            i2creply[i + 2] = val;
            Sum ^= val;
          }

          // Reg 9 and 10 are Digital Pots

          val = GetPot(0);
          i2creply[11] = val;
          Sum ^= val;

          val = GetPot(1);
          i2creply[12] = val;
          Sum ^= val;

          i2creply[13] = Sum;
          i2creply[14] = FEND;

          i2creplyptr = 0;
          i2creplylen = 15;
          return;
        }
        return;			// other immediate command
      }
      if (i2cMessage[0] && i2cMessage[0] != 12)
      {
        // Not Normal or Ackmode Data, so set param

        int reg = i2cMessage[0];
        int val = i2cMessage[1];

        if (reg == 9) // SPI Pots
          SetPot(0, val);
        else if (reg == 10)
          SetPot(1, val);
        else
          SaveEEPROM(reg, val);
        return;
      }

      // Data Frame

      return;
    }
    else
    {
      // normal char

      i2cMessage[i2cMsgPtr++] = c;
    }
  }
}

#endif

//
// handle Rx Event (incoming I2C data)
//
void receiveEvent(size_t count)
{
  target = Wire.getRxAddr(); 	// Get Slave address

  while (count--)
  {
    databuf[i2cputptr++] = Wire.readByte();
    i2cputptr &= 0x1ff;		// 512 cyclic buffer
  }
}

//
// handle Tx Event (outgoing I2C data)
//

void requestEvent(void)
{
#ifdef I2CHOST
  i2creply[1] = i2creplylen;
  i2creply[2] = i2creplylen >> 8;		// First two bytes are length
  Wire.write(i2creply, i2creplylen + 3);
  i2creplylen = 0;
  i2creplyptr = 3;
#else
  // TNC-PI interface works a byte at a time

  if (i2creplylen == 0)			// nothing to send
    Wire.write(i2cidle, 0); // return idle
  else
  {
    // return the next byte of the message
    Wire.write(&i2creply[i2creplyptr++], 1);
    i2creplylen--;
  }
#endif
}

#endif

#include <stdarg.h>
extern "C"
{
  int HostInit()
  {
    return true;
  }


  void PutString(const char * Msg)
  {
#ifdef I2CHOST
    memcpy(&i2creply[3], Msg, strlen(Msg));
    i2creplyptr = 3;
    i2creplylen = strlen(Msg);
#else
    HOSTPORT.write((const uint8_t *)Msg, strlen(Msg));
#endif
  }

  int PutChar(unsigned char c)
  {
#ifdef I2CHOST
    i2creply[i2creplyptr++] = c;
    i2creplylen++;
#else
    HOSTPORT.write(&c, 1);
#endif
    return 0;
  }

  void SerialSendData(const uint8_t * Msg, int Len)
  {
#ifdef I2CHOST
    memcpy(&i2creply[3], Msg, Len);
    i2creplyptr = 3;
    i2creplylen = Len;
#else
    HOSTPORT.write(Msg, Len);
#endif
  }

  void WriteDebugLog(int Level, const char * format, ...)
  {
    char Mess[256];
    va_list(arglist);

    va_start(arglist, format);
#ifdef LOGTOHOST
    Mess[0] = Level + '0';
    vsnprintf(&Mess[1], sizeof(Mess) -1, format, arglist);
    strcat(&Mess[1], "\r\n");
    SendLogToHost(Mess);
#endif
#ifdef MONPORT
    vsnprintf(Mess, sizeof(Mess), format, arglist);
    MONPORT.println(Mess);
#endif
    return;
  }

  // Write to Log, either via host or serial port
  void MONprintf(const char * format, ...)
  {
    char Mess[256];
    va_list(arglist);

    va_start(arglist, format);
#ifdef LOGTOHOST
    Mess[0] = '0';
    vsnprintf(&Mess[1], sizeof(Mess), format, arglist);
    strcat(&Mess[1], "\r\n");
    SendLogToHost(Mess);
#endif
#ifdef MONPORT
    vsnprintf(Mess, sizeof(Mess), format, arglist);
    MONPORT.println(Mess);
#endif
    return;
  }

  void CloseDebugLog()
  {
  }

  void CloseStatsLog()
  {
  }

#ifdef LOGTOHOST

#define LOGBUFFERSIZE 2048

  extern char LogToHostBuffer[LOGBUFFERSIZE];
  extern int LogToHostBufferLen;
  extern int PTCMode;

  void SendLogToHost(char * Cmd)
  {
    // I think we need log in text mode
    //	if (HostMode & !PTCMode)	// ARDOP Native

    if (!PTCMode)	// ARDOP Native
    {
      char * ptr = &LogToHostBuffer[LogToHostBufferLen];
      int len = strlen(Cmd);

      if (LogToHostBufferLen + len >= LOGBUFFERSIZE)
        return;			// ignore if full
		
	  Cmd[0] |= 0x80;		// Set top bit so we can unpack more easily

      memcpy(ptr, Cmd, len);
      LogToHostBufferLen += len;
    }
  }
#endif

// Sound Routines. These are all "C" Routines

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used 0 or 1

extern int Number;

BOOL DMARunning = FALSE;		// Used to start DMA on first write
BOOL FirstTime = FALSE;


void InitSound()
{
}

volatile unsigned short * SendtoCard(unsigned short buf, int n)
{
  // Start DMA if first call

  if (DMARunning == FALSE)
  {
    StartDAC();
    DMARunning = TRUE;
    FirstTime = TRUE;

    // We can immediately fill second half

    Index = 1;
    return &dac1_buffer[DAC_SAMPLES_PER_BLOCK];
  }

  // wait for other DMA buffer to finish

  // First time through we must wait till we are into the second
  //	(left < DAC_SAMPLES_PER_BLOCK)

  if (FirstTime)
  {
    FirstTime = FALSE;

    while (GetDMAPointer() >= DAC_SAMPLES_PER_BLOCK)
      txSleep(1);
  }

  // 	printtick("Start Wait");

  while (1)
  {
    int Left = GetDMAPointer();

    //	WriteDebugLog(LOGDEBUG, "Index %d Left %d", Index, Left);

    if (Index == 0)
    { // Just filled first buffer. Can return when left is less than half,
      // as then we are sending buffer 2

      if (Left > (DAC_SAMPLES_PER_BLOCK) )
        break;
    }
    else
    {
      // Just filled 2nd buffer, can return as soon as pointer is above half

      if (Left < (DAC_SAMPLES_PER_BLOCK))
        break;
    }
    txSleep(1);				// Run background while waiting
  }
  Index = !Index;
  txSleep(1);				// Run background while waiting
  //  printtick("Stop Wait");

  return &dac1_buffer[Index * DAC_SAMPLES_PER_BLOCK];
}


}














