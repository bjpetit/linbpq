// Arduino interface code for ARDOP running on a Teensie 3.6

#define TEENSY

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

#include <DMAChannel.h>
#include "SPI.h"
#include "ILI9341_t3.h"

DMAChannel dma1(false);

unsigned char RXBUFFER[300];	// Async RX Buffer

extern ILI9341_t3 tft;

extern volatile int RXBPtr;
volatile int flag = 0;
volatile int flag2 = 0;
extern int inIndex;			// ADC Buffer half being used 0 or 1

void yDisplayCall(int dirn, char * Call);
void yDisplayState(char * State);


#define pttPin 13

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

  void PollReceivedSamples();
  void ProcessSCSPacket(unsigned char * rxbuffer, int Length);

#include "C:\SkyDrive\Dev\Source\ARDOPC\ARDOPC.h"

  extern unsigned int tmrPollOBQueue;

  extern volatile unsigned short ADC_Buffer[2 * ADC_SAMPLES_PER_BLOCK];

#define Now getTicks()

  void xStartDAC()
  {
    StartDAC();
  }

  void xstopDAC()
  {
    stopDAC();
  }

  void displayCall(int dirn, char * Call)
  {
    char paddedcall[12] = "           ";

    paddedcall[0] = dirn;
    memcpy(paddedcall + 1, Call, strlen(Call));

    tft.setCursor(0, 72);
    tft.print(paddedcall);
  }

  void displayState(const char * State)
  {
    tft.setCursor(0, 48);
    tft.print("          ");
    tft.setCursor(0, 48);
    tft.print(State);
  }


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

  void HostPoll()
  {
    RXBPtr = Serial.available();

    if (RXBPtr)
    {
      int Count;
      RXBUFFER[RXBPtr] = 0;
      Count = Serial.readBytes((char *)RXBUFFER, RXBPtr);
      if (Count != RXBPtr)
        WriteDebugLog(LOGDEBUG, "Serial Read Error");

      ProcessSCSPacket(RXBUFFER, RXBPtr);
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
  void SetLED(int blnPTT)
  {
    digitalWrite(pttPin, blnPTT);
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

  pinMode(pttPin, OUTPUT);

  setupTFT();
  Serial.begin(115200);
  Serial1.begin(115200);

  tft.print("ARDOP TNC ");
  tft.println(ProductVersion);
  dma1.begin(true);

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
  analogReference(INTERNAL); // range 0 to 1.2 volts
  //analogReference(DEFAULT); // range 0 to 3.3 volts
  //analogReadAveraging(8);
  // Actually, do many normal reads, to start with a nice DC level

  for (i = 0; i < 1024; i++)
  {
    sum += analogRead(16);
  }

  WriteDebugLog(LOGDEBUG, "DAC Baseline %d", sum / 1024);
  StartDAC();
  stopDAC();
  setupADC(16);
  StartADC();
}


void loop()
{
  PollReceivedSamples();
  CheckTimers();
  HostPoll();
  MainPoll();
  PlatformSleep();

  if (blnClosing)
  {
    CPU_RESTART // reset processor
  }
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

#define PDB_PERIOD (5000-1)


void StartDAC()
{
  // Set up the DAC and start sending a frame under DMA

  dma1.disable();

  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // enable the DAC module, 3.3V reference

  SIM_SCGC6 |= SIM_SCGC6_PDB;

  PDB0_IDLY = 1;
  PDB0_MOD = PDB_PERIOD;

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








