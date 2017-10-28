/* Test code for Winlink ARDOP Modem Rev2a  Interface board
 *
 * Ver 10 
 *
 *
 * * CHANGE LOG:
 *
 *  DATE         REV  DESCRIPTION
 *  -----------  ---  ----------------------------------------------------------
 *  12-Jun-2017  10   TRL - EUI64, changes for Rev2a HW, CI-V Address for IC7000, fixed #ifdef
 *  
 *  
 *  Notes:
 *    Led 2 and 3 are uses as Bluetooth status
 *    
 */

#include <DMAChannel.h>
#include "SPI.h"
#include <i2c_t3.h>

// Address of i2c Ports
#define I2CADDR   0x28      // I2C Digital Pots
#define EUI64ADDR 0x50      // Address of EUI64

#define RigAddr 0x88        // ICOM CI-V address, IC-7000
//#define RigAddr 0x70        // ICOM CI-V address



/* ********************************************************************** */
//#define PI3_5TFT

#ifdef PI3_5TFT

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

// For Teensie

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 9
#define _rst 8

// Color definitions
#define    ILI9341_BLACK   0x0000
#define    ILI9341_BLUE    0x001F
#define    ILI9341_RED     0xF800
#define    ILI9341_GREEN   0x07E0
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
#define _RST 255        // Not Used
#define _MOSI 11
#define _SCLK 14        // Clock moved to ALT pin as LED is on A13
#define _MISO 12

// Use hardware SPI

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, _RST, _MOSI, _SCLK, _MISO);

#endif


/* ********************************************************************** */
#define pttPin 6          // Push to talk

#define LED1     8        // Only 3 LED in Rev2a board
#define LED2    26
#define LED3    25
//#define LED3  31

#define SW1     27
#define SW2     28
//#define SW3   29
//#define SW4   30

// CAT4016

#define CLK     2
#define BLANK   3
#define LATCH   4
#define ShiftIn 5

/* Bluetooth Defines for RN4678 */
//                          Pin   BT-Function
#define BT_WakeUp           A9    // In  H = Module On
#define BT_SoftwareButton   7     // In  H    L = Put Module in Standby
#define BT_Status1          A7    // Out      See table 2.3 in datasheet
#define BT_Status2          A8    // Out      See table 2.3 in datasheet
#define BT_CTS              24    // In   
#define BT_RTS              30    // Out
#define BT_PairingKey       35    // In  H    L = Force Standby
#define BT_UartRxInd        A17   // In  H    L = UART I/O In stanby mode
#define BT_LinkDropCtl      A18   // In  H    L = Drop BLE link
#define BT_InquiryCtl       A19   // In  H    L = Force BT-Classic mode
#define BT_ResetN           A20   // In  H    L = Module reset
#define BT_EAN              A6    // In  L    See table 2.1 in datasheet
#define BT_P2_4             A1    // In  H    See table 2.1 in datasheet
#define BT_P2_0             A3    // In  H    See table 2.1 in datasheet
#define BT_P3_7             29    // Out



DMAChannel dma1(false);

int count = 0;

int leds = 0;
int switches = 0;
unsigned short barvalue = 1;     // CAT4016

int adcint = 0;
int pdbint = 0;
char c = 0;
int value;
unsigned int lastmicros;
int adcints = 0;
int pdbints = 0;

uint8_t target = I2CADDR; // target Slave address

// Very simple command handler

static char Menu[] =
  "1  500 Hz sinewave\n"
  "2 1500 Hz sinewave\n"
  "3 3000 Hz sinewave\n"
  "4  500 Hz squarewave\n"
  "5 1500 Hz squarewave\n"
  "6 3000 Hz squarewave\n"
  "7 Echo ADC to DAC\n"
  "8 Toggle PTT\n"
  "9 Test CAT Port\n"
  "a Read i2c registers\n"
  "b  Set i2c reg (B reg val)"
  "e Read EUI64\n"
  "s Sync Pots\n"
 ;



/* ********************************************************************** */
void readEEPROM_MAC(int deviceaddress, byte eeaddress)
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(eeaddress); // LSB 
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 8); //request 8 bytes from the device

  Serial.print("EUI64: ");
  while (Wire.available()){
    Serial.print("0x");
    Serial.print(Wire.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();
}

/* ********************************************************************** */
void BT_Reset()
{
  digitalWriteFast(BT_ResetN, 0);
  delay(100);
  digitalWriteFast(BT_ResetN, 1);
  delay(1000);
}


/* ********************************************************************** */
void tick1()
{
  int newswitches;
  char Line[80];

  // Test individual leds

  leds++;

  if (leds == 32)
    leds = 0;

  digitalWriteFast(LED1, leds & 1);
//  digitalWriteFast(LED2, (leds >> 1) & 1);
//  digitalWriteFast(LED2, (leds >> 2) & 1);
//  digitalWriteFast(LED3, (leds >> 3) & 1);
  digitalWriteFast(13, (leds >> 4) & 1);

  // check switches and if changed report

  newswitches = digitalReadFast(SW1);
  newswitches |= digitalReadFast(SW2) << 1;
//  newswitches |= digitalReadFast(SW3) << 2;
//  newswitches |= digitalReadFast(SW4) << 3;

  if (newswitches != switches)
  {
    switches = newswitches;
    sprintf(Line, "Switches SW1 %d, SW2 %d", switches & 1, (switches >> 1) & 1);
    Serial.println(Line);
    //Serial1.println(Line);
  }

  // Update CAT4016

  CAT4016(barvalue); // <----------
  barvalue <<= 1;
  if (barvalue >= 8192)         // 12 LED
    {
      barvalue = 1;
      CAT4016(0xfff);   // <----------
    }

  // Write ADC to TFT

  //tft.setCursor(0, 96);
  //tft.print("          ");         // clear in case new value shorter
  //tft.setCursor(0, 96);
  //tft.print("ADC ");
  //tft.print(analogRead(16));

  // Check CAT Port

  RadioPoll();

  // kick watchdog

  noInterrupts();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  interrupts();
}


/* ********************************************************************** */
int dacout = 0;

//void tick2()
//{
// analogWriteDAC0(dacout++);

//  if (dacout == 4096)
//    dacout = 0;
//}


/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
void setup()
{
  int address, error;

  // Get serial up before enabling watchdog

  Serial.begin  (115200);
  Serial1.begin (115200);
  Serial4.begin (19200);         // CIV/RS232
  while (!Serial);

  // Set 10 second watchdog

  WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
  WDOG_TOVALL = 10000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
  WDOG_TOVALH = 0;
  WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
                  WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG

  WDOG_PRESC = 0; // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead of the default 1kHZ/4 = 200 HZ

  pinMode(13, OUTPUT);                // onboard LED
  pinMode(pttPin, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode (SW1, INPUT_PULLUP);
  pinMode (SW2, INPUT_PULLUP);
  //pinMode (SW3, INPUT_PULLUP);
  //pinMode (SW4, INPUT_PULLUP);

  pinMode(CLK, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(ShiftIn, OUTPUT);

  setupTFT();
  dma1.begin(true);

  tft.print("ARDOP Test V10");
  tft.println("");
  tft.print(Menu);

  Serial.println ("ARDOP Rev 2 Test Program V10");
  Serial1.println("ARDOP Rev 2 Test Program V10");

  if (RCM_SRS0 & 0X20)              // Watchdog Reset
  {
    Serial.println ("\n**** Reset by Watchdog ++++");
    //Serial1.println("\n**** Reset by Watchdog ++++");
  }

  analogReadRes(16);
  //analogReference(INTERNAL);  // range 0 to 1.2 volts
  analogReference(DEFAULT);     // range 0 to 3.3 volts
  analogWriteResolution(12);
  analogRead(16);                      // forces a calibrate

  // Clear CAT4016

  digitalWriteFast(BLANK, 0);      // Enable display
  digitalWriteFast(LATCH, 0);      // Strobe High to copy data from shift reg to display
  digitalWriteFast(CLK, 0);          // Strobe High to enter data to shift reg

  CAT4016(0xaaa);                // All off?

  Serial.println(F("\nScanning i2c bus"));
  //Serial1.println(F("Scanning i2c bus"));

 //  Setup for Slave mode, address 0x66, pins 18/19, external pullups, 400kHz
 //  Wire.begin(I2C_MASTER, 0, 0, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000, I2C_OP_MODE_IMM);

  Wire.begin();

  Wire.setDefaultTimeout(100000); // Microsecs (0.1 sec)

  for (address = 1; address <= 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print (F("i2c device at Address 0x"));
      Serial.println(address, HEX);
      //Serial1.print(F("i2c device at Address 0x"));
      //Serial1.println(address, HEX);
    }
    else if (error == 4)
    {
      Serial.print (F("i2c Error at Address 0x"));
      Serial.println(address, HEX);
      //Serial1.print(F("i2c Error at Address 0x"));
      //Serial1.println(address, HEX);
    }
  }


///* Bluetooth Define's for RN4678 */
////                          Pin   BT-Function
//#define BT_WakeUp           A9    // In  H = Module On
//#define BT_SoftwareButton   7     // In  H    L = Put Module in Standby
//#define BT_Status1          A7    // Out      See table 2.3 in datasheet
//#define BT_Status2          A8    // Out      See table 2.3 in datasheet
//#define BT_CTS              24    // In   
//#define BT_RTS              30    // Out
//#define BT_PairingKey       35    // In  H    L = Force Standby
//#define BT_UartRxInd        A17   // In  H    L = UART I/O In stanby mode
//#define BT_LinkDropCtl      A18   // In  H    L = Drop BLE link
//#define BT_InquiryCtl       A19   // In  H    L = Force BT-Classic mode
//#define BT_ResetN           A20   // In  H    L = Module reset
//#define BT_EAN              A6    // In  L    See table 2.1 in datasheet
//#define BT_P2_4             A1    // In  H    See table 2.1 in datasheet
//#define BT_P2_0             A3    // In  H    See table 2.1 in datasheet
//#define BT_P3_7             29    // Out

 // Set up BT

  Serial5.begin(115200);        // default for the module requires CTS/RTS


  pinMode (BT_WakeUp, OUTPUT);
  pinMode (BT_SoftwareButton, OUTPUT);
  pinMode (BT_Status1, INPUT);
  pinMode (BT_Status2, INPUT);
  pinMode (BT_CTS, OUTPUT);
  pinMode (BT_RTS, INPUT);
  pinMode (BT_PairingKey, OUTPUT);
  pinMode (BT_UartRxInd, OUTPUT);
  pinMode (BT_LinkDropCtl, OUTPUT);
  pinMode (BT_InquiryCtl, OUTPUT);
  pinMode (BT_ResetN, OUTPUT);
  pinMode (BT_EAN, OUTPUT);
  pinMode (BT_P2_4, OUTPUT);
  pinMode (BT_P2_0, OUTPUT);
  pinMode (BT_P3_7, INPUT);


  digitalWriteFast(BT_WakeUp, 1);         // 1 = normal
  digitalWriteFast(BT_SoftwareButton, 1); // 1 = Power on
  digitalWriteFast(BT_CTS, 0);            // Force CTS for now
  digitalWriteFast(BT_PairingKey, 1);     // 1 = normal
  digitalWriteFast(BT_UartRxInd, 1);      // 1 = normal
  digitalWriteFast(BT_LinkDropCtl, 1);    // 0 = disconnect BLE session    
  digitalWriteFast(BT_InquiryCtl, 1);     // 0 = Force Classic Mode
  digitalWriteFast(BT_ResetN, 1);         // 1 = normal
  digitalWriteFast(BT_EAN, 0);            // 0 = normal
  digitalWriteFast(BT_P2_4, 1);           // 1 = normal
  digitalWriteFast(BT_P2_0, 1);           // 1 = normal

// lets reset BT module
  digitalWriteFast(BT_ResetN, 0);
  delay(100);
  digitalWriteFast(BT_ResetN, 1);
  delay(1000);

// display EUI64
  readEEPROM_MAC(EUI64ADDR, 0xF8);  //0x50 is the I2c address, 0xF8 is the memory address where the read-only MAC value is

  //
  startDAC();
}


/* ********************************************************************** */
// This echo Serial1 --> Serial5 and Serial5 --> Serial1
// Used for testing BT module
void BT_serial()
{
   // read from port 1, send to port 5:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial5.write(inByte); 
  }
  
  // read from port 5, send to port 1:
  if (Serial5.available()) {
    int inByte = Serial5.read();
    Serial1.write(inByte); 
  }
}



/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
int lastmillis = 0;

void loop()
{
  if ((millis() - lastmillis) > 250)    // every 250 mS
  {
    tick1();                                // update leds and check switches
    lastmillis = millis();
  }

  // BT module status pin are displayed on Led 2 and 3
  boolean LED1test = digitalReadFast(BT_Status1);
  digitalWriteFast(LED2, LED1test);
          LED1test = digitalReadFast(BT_Status2);
  digitalWriteFast(LED3, LED1test);
  
  //  tick2();        // update DAC
  delay(1);
  
  HostPoll();

  BT_serial();      // Serial Echo for BT

}


/* ********************************************************************** */
void setupTFT()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
#ifdef PI3_5TFT
  tft.setRotation(3);
#else
  tft.setRotation(1);
#endif
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  //  tft.fillRect(100, 100, 60, 16, ILI9341_RED);

}


/* ********************************************************************** */
// Mod for Rev2a, we now have 12 Led's
void CAT4016(int value)
{
  // writes value to the 12 LED display
  int i;

  for (i = 0; i < 16; i++)            // must send all 16 to maintain sync
  {
    // Send each bit to display

    // We probbly don't need a microsecond delay, but I doubt if it will
    // cauise timing problems anywhere else

    // looks like we need to send data backwards (hi order bit first)

    digitalWriteFast(ShiftIn, (value >> 15) & 1);
    __asm("nop");

    delayMicroseconds(1);            // Setup is around 20 nS
    digitalWriteFast(CLK, 1);        // Copy SR to Outputs
    delayMicroseconds(1);            // Setup is around 20 nS
    digitalWriteFast(CLK, 0);        // Strobe High to copy data from shift reg to display
    value = value << 1;                // ready for next bit
  }

  // copy Shift Reg to display

  digitalWriteFast(LATCH, 1);        //  Strobe High to copy data from shift reg to display
  delayMicroseconds(1);                // Setup is around 20 nS
  digitalWriteFast(LATCH, 0);
}


/* ********************************************************************** */
// ADC/DAC/DMA Code.

// The DAC buffer is continuously sent to the DAC. The program changes
// the contents to change the waveform generated

// DAC uses DMA, trigged by the PDB.

// ADC is triggered by PDB and generates an interrupt which echos
// received value back to DAC

#define PDB_CONFIG_DAC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)
#define PDB_CONFIG_ADC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE)

//    PDB count of 4999 should give 12K clock with 60 MHz bus clock
//    PDB count of 1249 should give 48K clock with 60 MHz bus clock

#define SAMPLERATE 192000
#define PDB_PERIOD (F_BUS/SAMPLERATE - 1)
#define PDB_PERIOD_48K (F_BUS/48000 - 1)

// Tone generation:
// For a freqency of X we need SAMPLERATE / X samples per cycle and X / 10 sets of
// samples for a DAC Buffer of length SAMPLERATE/10

volatile uint16_t dac1_buffer[SAMPLERATE / 10];        // Allows waveforms down to 10Hz


/* ********************************************************************** */
void startDAC()
{
  // Set up the DAC and start sending buffer under DMA

  dma1.disable();

  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS;       // enable the DAC module, 3.3V reference

  // Set up PDB to clock the DMA and thus write to the DAC

  SIM_SCGC6 |= SIM_SCGC6_PDB;                        // Enable PDB Clock

  PDB0_IDLY = 1;
  PDB0_MOD = PDB_PERIOD;

  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_SWTRIG;

  dma1.TCD->SADDR = dac1_buffer;
  dma1.TCD->SOFF = 2;
  dma1.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BIT);
  dma1.TCD->NBYTES_MLNO = 2;                    // Bytes per minor loop
  dma1.TCD->SLAST = -sizeof(dac1_buffer);        // Reinit to start
  dma1.TCD->DADDR = &DAC0_DAT0L;
  dma1.TCD->DOFF = 0;
  dma1.TCD->CITER_ELINKNO = sizeof(dac1_buffer) / 2;
  dma1.TCD->DLASTSGA = 0;
  dma1.TCD->BITER_ELINKNO = sizeof(dac1_buffer) / 2;

  DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
  DMAMUX0_CHCFG0 = DMAMUX_SOURCE_PDB | DMAMUX_ENABLE;

  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  dma1.enable();
}


/* ********************************************************************** */
void stopDAC()
{
  dma1.disable();
  DAC0_DAT0L = 0;
  DAC0_DATH = 8;                                  // Set output to mid value
}

/* ********************************************************************** */
void setupADC(int pin)
{

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

  analogRead(pin);

  ADC0_SC1A |= ADC_SC1_AIEN;
  // ADC0_SC2 |= ADC_SC2_ADTRG;
  // PDB0_CH0C1 = 0x0101; // enable pretrigger 0 (SC1A)

  NVIC_ENABLE_IRQ(IRQ_PDB);
  // NVIC_ENABLE_IRQ(IRQ_ADC0);
}

/* ********************************************************************** */
//void pdb_isr()
//{
//  unsigned short val = ADC0_RA;                // should clear Interrupt
//  ADC0_SC1A = 8; // Trigger read on A2
//  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt flag
//}


/* ********************************************************************** */
// pdb interrupt is enabled in case you need it. Not needed at the moment
void pdb_isr(void)
{
  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt
  pdbints++;
  uint16_t val = (uint16_t)ADC0_RA;        // Read ADC
  val >>= 4; //DAC is only 12 bit
  DAC0_DAT0L = val; // & 0xff;                // Echo back to DAC
  DAC0_DATH = val >> 8;

  ADC0_SC1A = 8; // Trigger new read on A2
}

/* ********************************************************************** */
/*void adc0_isr()
  {
  uint16_t val = (uint16_t)ADC0_RA;        // should clear Interrupt
  val >>= 4; //DAC is only 12 bit
  DAC0_DAT0L = val; // & 0xff;                        // Echo back to DAC
  DAC0_DATH = val >> 8;
  adcints++;
  }
*/

/* ********************************************************************** */
void startADC()
{
  dma1.disable();

  // Configure PDB to Interrupt

  PDB0_MOD = PDB_PERIOD_48K;
  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_SWTRIG | PDB_SC_LDOK;

  unsigned short val = ADC0_RA;                // should clear Interrupt
  ADC0_SC1A = ADC_SC1_AIEN | 8; // Trigger read on A2
}

/* ********************************************************************** */
void HostPoll()
{
  int n = Serial.available();
  char RXbuffer[80];
  unsigned char Msg[80];

  if (n)
  {
    if (n > 80)
      n = 80;

    Serial.readBytes(RXbuffer, n);

    //    Very simple 1 character command handler

 //   PDB0_SC = 0;            // Stop ADC to DAC

    switch (RXbuffer[0])
    {
      case '1':
        GenerateTone(500);
        break;
      case '2':
        GenerateTone(1500);
        break;
      case '3':
        GenerateTone(3000);
        break;
      case '4':
        GenerateSquareWave(500);
        break;
      case '5':
        GenerateSquareWave(1500);
        break;
      case '6':
        GenerateSquareWave(3000);
        break;
      case '7':
        stopDAC();
        setupADC(16);
        startADC();
        break;
      case '8':
        digitalWriteFast(pttPin, !digitalReadFast(pttPin));
        break;
      case '9':

        Msg[0] = 0xfe;
        Msg[1] = 0xfe;
        Msg[2] = RigAddr;
        Msg[3] = 0xE0;
        Msg[4] = 0x6;            // Set Mode and Filter
        Msg[5] = 3;                // Mode
        Msg[6] = 1;             //Filter
        Msg[7] = 0xFD;
        Serial4.write(Msg, 8);
        break;

      case 'a':
      case 'A':

        GetPots();
        break;

      case 'b':
      case 'B':

        {
          int a = 0, b = 0;

          sscanf(&RXbuffer[1], "%d %d", &a, &b);
          SetPots(a, b);
        }
        break;

        case 'e':
        case 'E':
        {
            readEEPROM_MAC(EUI64ADDR, 0xF8);  //0x50 is the I2c address, 0xF8 is the memory address where the read-only MAC value is
        }
        break;

        case 's':   // Sync Pot to NV
        case 'S':
        {
         
        }
        break;

      default:
        Serial.print(Menu);
        return;
    }
    Serial.println("Ok");
  }
}


/* ********************************************************************** */
void GenerateTone(int Hz)
{
  float phase = 0.0;
  float twopi = 3.1415926 * 2;
  int steps = SAMPLERATE / Hz;          // steps to make one cycle
  int loops = Hz / 10;                            // sets of steps to fill DMA buffer
  float phaseincr = twopi / steps;
  volatile uint16_t * ptr = dac1_buffer;
  int i, j;

  for (i = 0; i < loops; i++)
  {
    phase = 0;
    for (j = 0; j < steps; j++)
    {
      *(ptr++) = sinf(phase) * 2048 * 0.9 + 2048;        // 0 - 4096, centered around half voltage
      phase += phaseincr;
      if (phase > twopi)
        phase = 0;
    }
  }
  startDAC();
}


/* ********************************************************************** */
void GenerateSquareWave(int Hz)
{
  int stepsperhalfcycle = (SAMPLERATE / 2) / Hz;        // steps to make each half of cycle
  int loops = Hz / 10;                                    // cycles to fill DMA buffer
  volatile short unsigned int * ptr = dac1_buffer;
  int i, j;

  for (i = 0; i < loops; i++)
  {
    // Write full scale for half cycle, then zero for rest

    for (j = 0; j < stepsperhalfcycle; j++)
      *(ptr++) = 4095;

    for (j = 0; j < stepsperhalfcycle; j++)
      *(ptr++) = 0;
  }
  startDAC();
}


/* ********************************************************************** */
unsigned char RXbuffer[256];
int RXLen = 0;

void RadioPoll()
{
  int Length = Serial4.available();
  int i, val;
  unsigned char * ptr;
  int len;

  // only try to read number of bytes in queue

  if (RXLen + Length > 256)
    RXLen = 0;

  Length = Serial4.readBytes(&RXbuffer[RXLen], Length);

  if (Length == 0)
    return;                    // Nothing doing

  RXLen += Length;

  Length = RXLen;

  Serial.print("CAT RX ");
  for (i = 0; i < Length; i++)
  {
    val = RXbuffer[i];
    Serial.print(val, HEX);
    Serial.print(" ");
  }
  Serial.println("");

  Serial1.print("CAT RX ");
  for (i = 0; i < Length; i++)
  {
    val = RXbuffer[i];
    Serial1.print(val, HEX);
    Serial1.print(" ");
  }
  Serial1.println("");


  if (Length < 6)                // Minimum Frame Sise
    return;

  if (RXbuffer[Length - 1] != 0xfd)
    return;

  ProcessICOMFrame(RXbuffer, Length);    // Could have multiple packets in buffer

  RXLen = 0;        // Ready for next frame
  return;
}


/* ********************************************************************** */
static char Modes[20][7] = {
  "LSB   ", "USB   ", "AM    ", "CW    ", "RTTY  ", "FM    ", "WFM   ",
  "CW-R  ", "RTTY-R", "????  ", "????  ", "????  ", "????  ", "????  ",
  "????  ", "????  ", "????  ", "DV    ", "????  "
};

void ProcessICOMFrame(unsigned char * rxbuffer, int Len)
{
  unsigned char * FendPtr;
  int NewLen;

  //    Split into Packets. By far the most likely is a single KISS frame
  //    so treat as special case

  FendPtr = (unsigned char *)memchr(rxbuffer, 0xfd, Len);

  if (FendPtr == &rxbuffer[Len - 1])
  {
    ProcessFrame(rxbuffer, Len);
    return;
  }

  // Process the first Packet in the buffer

  NewLen =  FendPtr - rxbuffer + 1;

  ProcessFrame(rxbuffer, NewLen);

  // Loop Back

  ProcessICOMFrame(FendPtr + 1, Len - NewLen);
  return;
}


/* ********************************************************************** */
void ProcessFrame(unsigned char * Msg, int framelen)
{
  int i;
  char Reply[80];

  if (Msg[0] != 0xfe || Msg[1] != 0xfe)

    // Duff Packer - return

    return;

  //  if (Msg[2] == 0xe0)
  //  {
  //    // Echo - Proves a CI-V interface is attached
  //   return;
  //  }

  if (Msg[4] == 0xFB)
  {
    // Accept

    // if it was the set freq, send the set mode

    return;
  }


  if (Msg[4] == 0xFA)
  {
    // Reject


    return;
  }

  if (Msg[4] == 3 || Msg[4] == 0)    // 0 sent in tranceive mode
  {
    // Rig Frequency
    int n, j, Freq = 0, decdigit;
    int start = 9;
    float RigFreq;
    char Valchar[16];

    for (j = start; j > 4; j--)
    {
      n = Msg[j];
      decdigit = (n >> 4);
      decdigit *= 10;
      decdigit += n & 0xf;
      Freq = (Freq * 100 ) + decdigit;
    }

    RigFreq = Freq / 1000.0;

    sprintf(Reply, "%10.3f", RigFreq);
    Serial.println(Reply);
    Serial1.println(Reply);

    tft.setCursor(0, 120);
    tft.print(Reply);

    return;
  }
  if (Msg[4] == 4 || Msg[4] == 1)
  {
    // Mode

    unsigned int Mode;

    Mode = (Msg[5] >> 4);
    Mode *= 10;
    Mode += Msg[5] & 0xf;

    if (Mode > 17) Mode = 17;

    Serial1.println(Modes[Mode]);
    Serial.println(Modes[Mode]);

    tft.setCursor(150, 120);
    tft.print(Modes[Mode]);
    return;
  }
}



/* ********************************************************************** */
// i2c pot routines


size_t idx;
#define MEM_LEN 256
char databuf[MEM_LEN];


int SetPots(int b, int a)
{
  Serial.printf("Writing %d to Reg %d: ", a, b);

  Wire.beginTransmission(target);       // Slave address
  Wire.write((b << 4) | + (a >> 8));      // opcode + top bit of value
  Wire.write(a & 0xff);    // value
  Wire.endTransmission();               // Transmit to Slave

  // Check if error occured
  if (Wire.getError())
    Serial.print("FAIL\n");
  else
    Serial.print("OK\n");

return 1;   // need to fix this
}

static char regdef[16][24] = {"Current Wiper 0", "Current Wiper 1",
                              "Nonvolatile Wiper 0", "Nonvolatile Wiper 1",
                              "TCON", "Status", "EEPROM", "EEPROM",
                              "EEPROM", "EEPROM","EEPROM", "EEPROM",
                              "EEPROM", "EEPROM","EEPROM", "EEPROM"
                             };

/* ********************************************************************** */
int GetPots()
{
  int i;

  for (i = 0; i < 16; i++)
  {
    Serial.printf("Select Chan %d(%s): ", i, &regdef[i]);

    Wire.beginTransmission(target);   // Slave address
    Wire.write((i << 4) | 0x0c);          // opcode
    Wire.endTransmission();           // Transmit to Slave

    // Check if error occured
    if (Wire.getError())
      Serial.print("FAIL ");
    else
      Serial.print("OK ");

    Serial.print("Reading: ");

    // Read from Slave

    Wire.requestFrom(target, 2); // Read from Slave (string len unknown, request full buffer)

    // Check if error occured
    if (Wire.getError())
      Serial.print("FAIL\n");
    else
    {
      // If no error then read Rx data into buffer and print
      idx = 0;
      while (Wire.available())
        databuf[idx++] = Wire.readByte();

      if (i == 4 || i == 5)
        Serial.printf("0x%x OK\n", (databuf[0] << 8) + databuf[1]);
      else
        Serial.printf("%d OK\n", (databuf[0] << 8) + databuf[1]);

    }
  }

  return 1;   // Need to fix this
}


