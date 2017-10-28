// Test code for PI Teensy Interface board

#define RigAddr 0x88				// ICOM CI-V address
#define CATPORT Serial3
//#include "TeensyConfig.h"
//#include "i2c_t3.h"

#include <DMAChannel.h>
#include "SPI.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>

#define OLED_DC     8
#define OLED_CS     9
#define OLED_RESET  7

#define _MOSI 11
#define _SCLK 13
#define _MISO 12
//Adafruit_SSD1306 display1(OLED_DC, OLED_RESET, OLED_CS);


#define pttPin 6

#define LED0 2
#define LED1 3
#define LED2 4
#define LED3 5

#define SW1 27
#define SW2 28
#define SW3 29
#define SW4 30

// SPI POT

#define slaveSelectPin 10

DMAChannel dma3(false);

int count = 0;

int leds = 0;
int switches = 0;
unsigned short barvalue = 1; 	// CAT4016

int adcint = 0;
int pdbint = 0;
char c = 0;
int value;
unsigned int lastmicros;
int adcints = 0;
int pdbints = 0;


HardwareSerial* SaveSerial;

void tick1()
{
  int newswitches;
  char Line[80];

  // Test individual leds

  leds++;

  if (leds == 16)
    leds = 0;

  digitalWriteFast(LED0, leds & 1);
  digitalWriteFast(LED1, (leds >> 1) & 1);
  digitalWriteFast(LED2, (leds >> 2) & 1);
  digitalWriteFast(LED3, (leds >> 3) & 1);

  if (newswitches != switches)
  {
    switches = newswitches;
    sprintf(Line, "Switches SW1 %d, SW2 %d, SW3 %d, SW4 %d",
            switches & 1, (switches >> 1) & 1, (switches >> 2) & 1, (switches >> 3) & 1);
    Serial.println(Line);
    Serial1.println(Line);
  }



  // Check CAT Port

  RadioPoll();

  // kick watchdog

  noInterrupts();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  interrupts();
}

int dacout = 0;

//void tick2()
//{
// analogWriteDAC0(dacout++);

//  if (dacout == 4096)
//    dacout = 0;
//}

void setup()
{
  int address, error;

  // Get serial up before enabling watchdog

  Serial.begin(115200);
  while (!Serial);

  // Set 10 second watchdog

  WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
  WDOG_TOVALL = 10000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
  WDOG_TOVALH = 0;
  WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
                  WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG

  WDOG_PRESC = 0; // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead of the default 1kHZ/4 = 200 HZ

  pinMode(pttPin, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode (SW1, INPUT_PULLUP);
  pinMode (SW2, INPUT_PULLUP);
  pinMode (SW3, INPUT_PULLUP);
  pinMode (SW4, INPUT_PULLUP);

  pinMode (slaveSelectPin, OUTPUT);	// SPI Pot
  digitalWriteFast (slaveSelectPin, HIGH);

  
 // Serial.printf("Setup Display %x\r\n", &Wire1);

  delay(100);

// Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_37_38, I2C_PULLUP_EXT, 2500000, I2C_OP_MODE_IMM);
// Wire1.begin();

 //  Serial.printf("Setup Display %x\r\n", Wire1.bus);

  delay(100);
//  Wire1.setDefaultTimeout(10000); // 10ms
//  Serial.printf("Setup Display %x\r\n", Wire1.bus);

  delay(100);

 // Wire1.beginTransmission(0x3C);       // slave addr
 // Serial.printf("Setup Display %x\r\n", Wire1.bus);

  delay(100);
//  Wire1.endTransmission();              // no data, just addr
//  Serial.printf("Setup Display %x\r\n", Wire1.bus);

  delay(100);

   Serial.printf("Start Display %d\r\n", millis());
//  display1.display();
 Serial.printf("End Display %d\r\n", millis());


  // Clear the buffer.
//  Serial.printf("Start Clear %d\r\n", millis());
 // display.clearDisplay();
//  Serial.printf("End Clear %d\r\n", millis());
 //   display.display();
 
  
  SPI.begin();

  dma3.begin(true);

  Serial1.begin(115200);
  CATPORT.begin(19200);

  SaveSerial = (HardwareSerial*)&Serial;

  Serial.printf("Serial is %x\r\n", SaveSerial);

  SaveSerial->println("Should be USB - DOes this work?");

  SaveSerial = &Serial1;

  SaveSerial->println("Should be Serial1 DOes this work?");

  SaveSerial = (HardwareSerial*)&Serial;

 // SaveSerial->printf("SaveSerial Buffer Space %d\r\n", SaveSerial->availableForWrite());
  SaveSerial->printf("Serial Buffer Space %d\r\n", Serial.availableForWrite());
  SaveSerial->printf("Serial1 Space %d\r\n", Serial1.availableForWrite());


  Serial.println("ARDOP Interface Test Program");
  Serial1.println("ARDOP Interface Test Program");

  if (RCM_SRS0 & 0X20)		// Watchdog Reset
  {
    Serial.println("\n**** Reset by Watchdog ++++");
    Serial1.println("\n**** Reset by Watchdog ++++");
  }

  analogReadRes(16);
  //analogReference(INTERNAL); // range 0 to 1.2 volts
  analogReference(DEFAULT); // range 0 to 3.3 volts
  analogWriteResolution(12);
  analogRead(16);			// forces a calibrate

  xstartDAC();

}

int lastmillis = 0;

void loop()
{
  if ((millis() - lastmillis) > 250)	// every 250 mS
  {
    tick1();			// update leds and check switches
    lastmillis = millis();
  }
  //  tick2();		// update DAC
  delay(1);
  HostPoll();
}


// ADC/DAC/DMA Code.

// The DAC buffer is continuously sent to the DAC. The program changes
// the contents to change the waveform generated

// DAC uses DMA, trigged by the PDB.

// ADC is triggered by PDB and generates an interrupt which echos
// received value back to DAC

#define PDB_CONFIG_DAC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_DMAEN)
#define PDB_CONFIG_ADC (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)

//	PDB count of 4999 should give 12K clock with 60 MHz bus clock
//	PDB count of 1249 should give 48K clock with 60 MHz bus clock

#define SAMPLERATE 192000
#define PDB_PERIOD (F_BUS/SAMPLERATE - 1)
#define PDB_PERIOD_48K (F_BUS/48000 - 1)

// Tone generation:
// For a freqency of X we need SAMPLERATE / X samples per cycle and X / 10 sets of
// samples for a DAC Buffer of length SAMPLERATE/10

volatile uint16_t dac_buffer[SAMPLERATE / 10];		// Allows waveforms down to 10Hz

void xstartDAC()
{
  // Set up the DAC and start sending buffer under DMA

  // Set buffer to half rail

  int i;

  //  for (i = 0; i < SAMPLERATE / 10; i++)
  //  {
  //    dac_buffer[i] = 2048;
  //  }

  dma3.disable();

  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // enable the DAC module, 3.3V reference

  // Set up PDB to clock the DMA and thus write to the DAC

  SIM_SCGC6 |= SIM_SCGC6_PDB;			// Enable PDB Clock

  PDB0_IDLY = 1;
  PDB0_MOD = PDB_PERIOD;

  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_DAC | PDB_SC_SWTRIG;

  dma3.TCD->SADDR = dac_buffer;
  dma3.TCD->SOFF = 2;
  dma3.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BIT);
  dma3.TCD->NBYTES_MLNO = 2;	// Bytes per minor loop
  dma3.TCD->SLAST = -sizeof(dac_buffer);	// Reinit to start
  dma3.TCD->DADDR = &DAC0_DAT0L;
  dma3.TCD->DOFF = 0;
  dma3.TCD->CITER_ELINKNO = sizeof(dac_buffer) / 2;
  dma3.TCD->DLASTSGA = 0;
  dma3.TCD->BITER_ELINKNO = sizeof(dac_buffer) / 2;

  DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
  DMAMUX0_CHCFG0 = DMAMUX_SOURCE_PDB | DMAMUX_ENABLE;

  dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  dma3.enable();
}

void xstopDAC()
{
  dma3.disable();
  DAC0_DAT0L = 0;
  DAC0_DATH = 8;		// Set output to mid value
}

void xsetupADC(int pin)
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

//void pdb_isr()
//{
//  unsigned short val = ADC0_RA;				// should clear Interrupt
//  ADC0_SC1A = 8; // Trigger read on A2
//  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt flag
//}


// pdb interrupt is enabled in case you need it. Not needed at the moment
void xpdb_isr(void)
{
  PDB0_SC &= ~PDB_SC_PDBIF; // clear interrupt
  pdbints++;
  uint16_t val = (uint16_t)ADC0_RA;		// Read ADC
  val >>= 4; //DAC is only 12 bit
  DAC0_DAT0L = val; // & 0xff;				// Echo back to DAC
  DAC0_DATH = val >> 8;

  ADC0_SC1A = 8; // Trigger new read on A2
}


/*void adc0_isr()
  {
  uint16_t val = (uint16_t)ADC0_RA;		// should clear Interrupt
  val >>= 4; //DAC is only 12 bit
  DAC0_DAT0L = val; // & 0xff;						// Echo back to DAC
  DAC0_DATH = val >> 8;
  adcints++;
  }
*/
void xstartADC()
{
  dma3.disable();

  // Configure PDB to Interrupt

  PDB0_MOD = PDB_PERIOD_48K;
  PDB0_SC = 0;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG_ADC | PDB_SC_SWTRIG | PDB_SC_LDOK;

  unsigned short val = ADC0_RA;				// should clear Interrupt
  ADC0_SC1A = ADC_SC1_AIEN | 8; // Trigger read on A2
}

// Very simple command handler

static char Menu[] =
  "1 500 Hz sinewave\n"
  "2 1500 Hz sinewave\n"
  "3 3000 Hz sinewave\n"
  "4 500 Hz squarewave\n"
  "5 1500 Hz squarewave\n"
  "6 3000 Hz squarewave\n"
  "7 Echo ADC to DAC\n"
  "8 Toggle PTT\n"
  "9 Test CAT Port\n"
  "A Read Pot registers\n"
  "B Set Pot register (B reg val)\n"
  "C Read Input and Output ADC pins\n";

void HostPoll()
{
  int n = SaveSerial->available();
  char RXbuffer[80];
  unsigned char Msg[80];

  if (n)
  {
    int i;

    if (n > 80)
      n = 80;

    SaveSerial->readBytes(RXbuffer, n);

   
    //	Very simple 1 character command handler

    //   PDB0_SC = 0;			// Stop ADC to DAC

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
        xstopDAC();
        xsetupADC(16);
        xstartADC();
        break;
      case '8':
        digitalWriteFast(pttPin, !digitalReadFast(pttPin));
        break;
      case '9':

        Msg[0] = 0xfe;
        Msg[1] = 0xfe;
        Msg[2] = RigAddr;
        Msg[3] = 0xE0;
        Msg[4] = 0x6;			// Set Mode and Filter
        Msg[5] = 3;				// Mode
        Msg[6] = 1; 			//Filter
        Msg[7] = 0xFD;
        CATPORT.write(Msg, 8);
        break;

      case 'a':
      case 'A':

        for (i = 0; i < 16; i++)
        {
          n = GetPotSPI(i) & 0x1ff;
          if (i == 4 || i == 5)
            SaveSerial->printf("0x%x OK\n", n);
          else
            SaveSerial->printf("%d OK\n", n);
        }
        break;

      case 'b':
      case 'B':

        {
          int a = 0, b = 0;

          sscanf(&RXbuffer[1], "%d %d", &a, &b);
          SaveSerial->printf("Setting Reg %d to %d\r\n", a, b);
          SetPotSPI(a, b);
        }
        break;

      case 'c':
      case 'C':


        for (i = 0; i < 10; i++)
        {
          SaveSerial->printf("A2 = %d A3 = %d\r\n", analogRead(16), analogRead(17));
        }
        break;

      default:
        SaveSerial->print(Menu);
        return;
    }
    SaveSerial->println("Ok");
  }
}

void GenerateTone(int Hz)
{
  float phase = 0.0;
  float twopi = 3.1415926 * 2;
  int steps = SAMPLERATE / Hz;		// steps to make one cycle
  int loops = Hz / 10;							// sets of steps to fill DMA buffer
  float phaseincr = twopi / steps;
  volatile uint16_t * ptr = dac_buffer;
  int i, j;

  for (i = 0; i < loops; i++)
  {
    phase = 0;
    for (j = 0; j < steps; j++)
    {
      *(ptr++) = sinf(phase) * 2048 * 0.9 + 2048;		// 0 - 4096, centered around half voltage
      phase += phaseincr;
      if (phase > twopi)
        phase = 0;
    }
  }
  xstartDAC();
}

void GenerateSquareWave(int Hz)
{
  int stepsperhalfcycle = (SAMPLERATE / 2) / Hz;		// steps to make each half of cycle
  int loops = Hz / 10;									// cycles to fill DMA buffer
  volatile short unsigned int * ptr = dac_buffer;
  int i, j;

  for (i = 0; i < loops; i++)
  {
    // Write full scale for half cycle, then zero for rest

    for (j = 0; j < stepsperhalfcycle; j++)
      *(ptr++) = 4095;

    for (j = 0; j < stepsperhalfcycle; j++)
      *(ptr++) = 0;
  }
  xstartDAC();
}

unsigned char RXbuffer[256];
int RXLen = 0;

void RadioPoll()
{
  int Length = CATPORT.available();
  int i, val;
  unsigned char * ptr;
  int len;

  // only try to read number of bytes in queue

  if (RXLen + Length > 256)
    RXLen = 0;

  Length = CATPORT.readBytes(&RXbuffer[RXLen], Length);

  if (Length == 0)
    return;					// Nothing doing

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


  if (Length < 6)				// Minimum Frame Sise
    return;

  if (RXbuffer[Length - 1] != 0xfd)
    return;

  ProcessICOMFrame(RXbuffer, Length);	// Could have multiple packets in buffer

  RXLen = 0;		// Ready for next frame
  return;
}


static char Modes[20][7] = {
  "LSB   ", "USB   ", "AM    ", "CW    ", "RTTY  ", "FM    ", "WFM   ",
  "CW-R  ", "RTTY-R", "????  ", "????  ", "????  ", "????  ", "????  ",
  "????  ", "????  ", "????  ", "DV    ", "????  "
};

void ProcessICOMFrame(unsigned char * rxbuffer, int Len)
{
  unsigned char * FendPtr;
  int NewLen;

  //	Split into Packets. By far the most likely is a single KISS frame
  //	so treat as special case

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

  if (Msg[4] == 3 || Msg[4] == 0)	// 0 sent in tranceive mode
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

    return;
  }
}

static char regdef[16][24] = {"Current Wiper 0", "Current Wiper 1",
                              "Nonvolatile Wiper 0", "Nonvolatile Wiper 1",
                              "TCON", "Status", "EEPROM", "EEPROM",
                              "EEPROM", "EEPROM", "EEPROM", "EEPROM",
                              "EEPROM", "EEPROM", "EEPROM", "EEPROM"
                             };



void SetPotSPI(int address, int value)
{
  int Command = address << 4;  // Reg addr to top 4 bits

  if (value & 0x100)						// 9th bit set?
    Command |= 1;								// goes in bottom bit of command

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWriteFast(slaveSelectPin, LOW);

  SPI.transfer(Command);
  SPI.transfer(value);

  digitalWriteFast(slaveSelectPin, HIGH);
  // release control of the SPI port
  SPI.endTransaction();
}

unsigned int GetPotSPI(int i)
{
  byte thisRegister;
  byte inByte = 0;           // incoming byte from the SPI
  unsigned int result = 0, result1 = 0;   // result to return

  SaveSerial->printf("Chan %d(%s): ", i, &regdef[i]);

  thisRegister = (i << 4) | 0x0c;

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(slaveSelectPin, LOW);

// Device returns the top two bits of the value when address is sent

  result1 = SPI.transfer(thisRegister);
  // send a value of 0 to read the low 8 bits
  result = SPI.transfer(0x00);

  digitalWrite(slaveSelectPin, HIGH);
  SPI.endTransaction();

  return (result | (result1 << 8));
}





