// Common Arduino interface code for modems running on a Teensy 3.6

// Currently used by ARDOP and Packet TNC

#include <EEPROM.h>

#include "TeensyConfig.h"

extern "C"
{
#include "..\..\ARDOPC.h"
}

#include "SPI.h"

extern "C" void AdjustRXLevel(int Level);
extern "C" void AdjustTXLevel(int Level);
extern "C" void WriteDebugLog(int Level, const char * format, ...);
extern "C" void SetPot(int address, unsigned int value);
extern "C" unsigned int GetPot(int address);
extern "C" void SetLED(int Pin, int State);

extern "C" bool OKtoAdjustLevel();			// in platform specific code

void i2csetup();
extern "C" void StartDAC();

void StartADC();
void setupDAC();
void setupADC();
extern "C" void stopDAC();

extern int inIndex;			// ADC Buffer half being used 0 or 1

#include <DMAChannel.h>

DMAChannel dma1(true);
DMAChannel dma2(true);

int VRef = 32768;				// ADC and ADC reference (ideal is 32678)

void CommonSetup()
{
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

// Code to access Digital Pots (used for setting input and output levels)

// PI Board uses SPI, WDT Board uses i2c
extern "C"
{
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


Print * setupTFT()
{
  Serial1.println("INIT TFT");
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.printf("ARDOP TNC %s", ProductVersion);
  tftptr = &tft;
  return &tft;
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

extern "C"void displayCall(int dirn, char * Call)
{}
extern "C"void displayState(const char * State)
{}

#endif

// Signal levels for each bar

const int barlevels[10] = {
  1000, 2000, 5000, 8000, 11000,
  16000, 24000, 28000, 30000, 32000
};

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

extern "C" void CheckandAdjustRXLevel(int maxlevel, int minlevel, bool Force)
{
  int pktopk = (maxlevel - minlevel) * 3300 / 65536;

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
    // Calulate actual input voltage from current pot gain setting

    pktopk = pktopk * autoRXLevel / 3000;

    WriteDebugLog(LOGINFO, "peak to peak input %d mV", pktopk);

    autoRXLevel = pktopk  * 3 / 2;			// try to get to 2/3rd

    if (autoRXLevel > 3000)
      autoRXLevel = 3000;

    AdjustRXLevel(autoRXLevel);
  }
}






