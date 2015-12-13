
#include "mbed.h"
#include "ARDOPC.h"


#include <stdarg.h>

#include "stm32f4xx.h"


//AnalogOut my_output(PA_4);


//short buffer[BUFFER_SIZE];


extern "C" void GenerateFSKTemplates();
extern "C" void InitValidFrameTypes();
extern "C" void SendID(BOOL blnEnableCWID);

extern "C" void GetNextFECFrame();

extern "C" void ardopmain();
extern "C" void Sleep();
extern "C" void printtick(char * msg);
extern "C" VOID Debugprintf(const char * format, ...);
extern "C" void Config_ADC_DMA(void);
extern "C" void Start_ADC_DMA(void);
extern "C" void ProcessNewSamples(short * Samples, int nSamples);

extern "C"  uint32_t DMA_GetCurrentMemoryTarget(DMA_Stream_TypeDef* DMAy_Streamx);

InterruptIn mybutton(USER_BUTTON);
DigitalOut myled(LED1);
Ticker ti;
Serial serial(USBTX, USBRX);

float delay = 0.001; // 1 sec
volatile int iTick = 0;
volatile bool bTick = 0;
volatile int iClick = 0;
volatile bool bClick = 0;
extern volatile int Now;
volatile int ADCInterrupts = 0;
extern volatile int adc_buffer_mem;

#define SAMPLES_PER_BLOCK 1200

extern uint16_t ADC_Buffer[2][SAMPLES_PER_BLOCK];     // stereo capture buffers

short audioInputBuffer[SAMPLES_PER_BLOCK];


int i = 0;

void tick()
{
	bTick = true;
	Now++;
}

void pressed()
{
	iClick++;
	bClick = true;
 //   if (delay == 1.0)
 //       delay = 0.2; // 200 ms
 //   else
 //       delay = 1.0; // 1 sec
}

void InitSound()
{
}


int lastchan = 0;

extern int TXQueueLen;

int main()
{
	float jnw;
	int max, min, tot;

	jnw = 1.0;

	jnw = jnw * 4.0;

	serial.baud(115200);
	serial.printf("Clock Freq %d\r\n", SystemCoreClock);

	mybutton.fall(&pressed);

	blnTimeoutTriggered = FALSE;

	ti.attach(tick, .001);

	ardopmain();

	Config_ADC_DMA();
	Start_ADC_DMA();

    while (1)
    {
    	// Process any received samples

		// *************************
		// convert the saved ADC 12-bit unsigned samples into 16-bit signed samples

    	if (adc_buffer_mem >= 0 && adc_buffer_mem <= 1)
    	{
    		uint16_t *src = (uint16_t *)&ADC_Buffer[adc_buffer_mem];	// point to the DMA buffer where the ADC samples were saved
    		int16_t *dst = (int16_t *)&audioInputBuffer;				// point to our own buffer where we are going to copy them too

    		// 12-bit unsigned stereo to 16-bit signed mono

    		int i;

    		tot = 0;
    		max = 0;
    		min = 0;

    		for (i = 0; i < SAMPLES_PER_BLOCK; i++)
    		{
    			register int32_t s1 = ((int16_t)(*src++) - 2048) << 4;  // unsigned 12-bit to 16-bit signed .. ADC channel 1
     			*dst++ = (int16_t)s1;
     			tot += s1;
     			if (s1 > max)
     				max = s1;
     			if (s1 < min)
     				min = s1;
    		}

   	//	serial.printf("Max %d min %d av %d\n", max, min, tot/1200);
   	//		serial.printf("ADCInterrupts %i %d %d buffer no %d \r\n", ADCInterrupts,
   	//		ADC_Buffer[0][0], ADC_Buffer[1][0], DMA_GetCurrentMemoryTarget(DMA2_Stream0));

    		 myled = !myled;

    		 ProcessNewSamples(audioInputBuffer, SAMPLES_PER_BLOCK);

    		 adc_buffer_mem = -1;    // finished with the ADC buffer that the DMA filled
    	}

    	if (bTick)
    	{
  //			serial.printf("ADCInterrupts %i %d %d buffer no %d \r\n", ADCInterrupts,
  //					ADC_Buffer[0][0], ADC_Buffer[1][0], DMA_GetCurrentMemoryTarget(DMA2_Stream0));

   			bTick = false;
    	}

    	if (bClick)
    	{
   			serial.printf("button click %i\r\n", iClick);
   			bClick = false;

   			int TXQueueLen = 96;

   			ProtocolState = FECSend;
   			GetNextFECFrame();

   			ProtocolState = FECSend;
   			GetNextFECFrame();

   			ProtocolState = FECSend;
   			GetNextFECFrame();

   			SendID(0);
    	}

 //       myled = !myled;
       wait(delay);
    }
}

int Lasttick = 0;

void printtick(char * msg)
{
	serial.printf("%s %i\r\n", msg, Now - Lasttick);
	Lasttick = Now;
}

VOID Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	serial.printf(Mess);

	return;
}






