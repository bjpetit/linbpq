//
//	Passes audio samples to and from the sound interface

//	This is the Arduino/Teensie Version, using DMA to DAC/ADC

//	Also has some platform specific routines

#define TEENSIE
#include "C:\Users\John\OneDrive\Dev\Source\ARDOPC\ARDOPC.h"
#include <math.h>

extern BOOL blnDISCRepeating;

// the ADC DMA saves the incoming ADC samples into these 2 buffers

extern volatile unsigned short dac1_buffer[DAC_SAMPLES_PER_BLOCK * 2];

extern int ADCInterrupts;


void stopDAC();

// Windows and Linux work with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

unsigned short ADC_Buffer[2][ADC_SAMPLES_PER_BLOCK] = {0};	// Two Transfer/DMA buffers of 0.1 Sec
unsigned short work;

int max, min, tot;

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[] = "DMA";
char PlaybackDevice[] = "DMA";

char * CaptureDevices = CaptureDevice;
char * PlaybackDevices = PlaybackDevice;

int LastNow;

int Numbertosend = 0;				// Number waiting to be sent

unsigned short * DMABuffer;

void printtick(char * msg)
{
  WriteDebugLog("%s %i", msg, Now - LastNow);
  LastNow = Now;
}

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used 0 or 1

BOOL DMARunning = FALSE;		// Used to start DMA on first write
BOOL FirstTime = FALSE;

void InitSound()
{
  Config_ADC_DMA();
  Start_ADC_DMA();
}

unsigned short * SendtoCard(unsigned short buf, int n)
{
  if (Loopback)
  {
    // Loop back   to decode for testing

    ProcessNewSamples(buf, DAC_SAMPLES_PER_BLOCK);		// signed
  }

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
      txSleep(10);
  }

  //	printtick("Start Wait");

  while (1)
  {
    int Left = GetDMAPointer();

    //	WriteDebugLog("Index %d Left %d", Index, Left);

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
    txSleep(10);				// Run background while waiting
  }
  Index = !Index;
  txSleep(10);				// Run background while waiting
  //printtick("Stop Wait");

  return &dac1_buffer[Index * DAC_SAMPLES_PER_BLOCK];
}

//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

int min = 0, max = 0, leveltimer = 0;

void PollReceivedSamples()
{
  int Pointer = GetADCDMAPointer();

  if (inIndex == 0)
  {
    if (Pointer > ADC_SAMPLES_PER_BLOCK)
      return;								// Still reading into first half
  }
  else
  {
    if (Pointer < ADC_SAMPLES_PER_BLOCK)
      return;

  }

  // convert the saved ADC 16-bit unsigned samples into 16-bit signed samples

  {
    unsigned short  *src = (unsigned short *)&ADC_Buffer[inIndex];	// point to the DMA buffer where the ADC samples were saved
    short  *dst = (unsigned short *)src;				// reuse input buffer

    int i;

    for (i = 0; i < ADC_SAMPLES_PER_BLOCK; i++)
    {
      register int s1 = (unsigned short)(*src++);
      s1 -= 32768;
      *dst++ = s1;
      tot += s1;
      if (s1 > max)
        max = s1;
      if (s1 < min)
        min = s1;
    }

    //	serial.printf("Max %d min %d av %d\n", max, min, tot/ADC_SAMPLES_PER_BLOCK);

    //  	 printtick("Process Sample Start");

    ProcessNewSamples(&ADC_Buffer[inIndex], ADC_SAMPLES_PER_BLOCK);


    // 	 printtick("Process Sample End");

    if (leveltimer++ > 100)
    {
      leveltimer = 0;
      WriteDebugLog("Input peaks = %d, %d", min, max);
    }
    min = max = 0;
  }
  inIndex = !inIndex;
}

void StopCapture()
{
  Capturing = FALSE;
  //	printf("Stop Capture\n");
}

void StartCodec(char * strFault)
{
  strFault[0] = 0;
}

void StopCodec(char * strFault)
{
  strFault[0] = 0;
}

void StartCapture()
{
  Capturing = TRUE;
  DiscardOldSamples();
  ClearAllMixedSamples();
  State = SearchingForLeader;

  //	printf("Start Capture\n");
}
void CloseSound()
{
}

unsigned short * SoundInit()
{
  Index = 0;
  return &dac1_buffer[0];
}

//	Called at end of transmission

extern int Number;				// Number of samples waiting to be sent

void SoundFlush()
{
  int FlushEnd;

  // Append Trailer then send remaining samples

  AddTrailer();			// add the trailer.

  if (Loopback)
  {
    ProcessNewSamples(&dac1_buffer[Index * DAC_SAMPLES_PER_BLOCK], Number);
  }

//  WriteDebugLog ("Flush Index = %d Number = %d Left = %d", Index, Number, GetDMAPointer());

  if (Index == 0)
  
    //	Sending from first half of buffer. Stop when DMS Pointer gets to
    //	( 2 * DAC_SAMPLES_PER_BLOCK) - Number

    FlushEnd = ( 2 * DAC_SAMPLES_PER_BLOCK) - Number;
    
  else
  
    // Second Half. Stop when pointer gets to DAC_SAMPLES_PER_BLOCK - Number)
    FlushEnd = DAC_SAMPLES_PER_BLOCK - Number;

  while (GetDMAPointer() > FlushEnd)
    Sleep(1);
  
 // WriteDebugLog ("Stopped at = %d", GetDMAPointer());

  stopDAC();
  DMARunning = FALSE;
  SoundIsPlaying = FALSE;

  if (blnEnbARQRpt > 0 || blnDISCRepeating)	// Start Repeat Timer if frame should be repeated
    dttNextPlay = Now + intFrameRepeatInterval;
    
  KeyPTT(FALSE);		 // Unkey the Transmitter

  //	StartCapture();

  return;
}

//  Function to Key radio PTT

const char BoolString[2][6] = {"FALSE", "TRUE"};

BOOL KeyPTT(BOOL blnPTT)
{
  // Returns TRUE if successful False otherwise

  SetLED(blnPTT);
  return TRUE;
}

void Start_ADC_DMA(void)
{
}

void Config_ADC_DMA(void)
{
}




