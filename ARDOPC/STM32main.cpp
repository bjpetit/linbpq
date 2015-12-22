
#include "mbed.h"
#include "ARDOPC.h"

#include <stdarg.h>

#include "stm32f4xx.h"

//AnalogOut my_output(PA_4);

//short buffer[BUFFER_SIZE];

extern "C" {
	void InitValidFrameTypes();
	void GetNextFECFrame();
	void ardopmain();
	void printtick(char * msg);
	void Debugprintf(const char * format, ...);
	void Config_ADC_DMA(void);
	void Start_ADC_DMA(void);
	void ProcessNewSamples(short * Samples, int nSamples);
	void CheckTimers();
	void MainPoll();
	void HostPoll();
	void SetARDOPProtocolState(int State);

	uint32_t DMA_GetCurrentMemoryTarget(DMA_Stream_TypeDef* DMAy_Streamx);
	void PollReceivedSamples();
	void SetLED(int blnPTT);
	void Sleep(int delay);
	void SerialSink(UCHAR c);
	void SerialSendData(unsigned char * Msg, int Len);
}

InterruptIn mybutton(USER_BUTTON);
DigitalOut myled(LED1);
Ticker ti;

Serial serial(USBTX, USBRX);		// Host PTC Emulation
Serial serial3(PC_10, PC_11);		// Debug Port

float delay = 0.001; // 1 mS

volatile int iTick = 0;
volatile bool bTick = 0;
volatile int iClick = 0;
volatile bool bClick = 0;
volatile int ticks;
volatile int ADCInterrupts = 0;
extern volatile int adc_buffer_mem;

#define SAMPLES_PER_BLOCK 1200

int i = 0;

void SetLED(int blnPTT)
{
	myled = blnPTT;
}

void tick()
{
	bTick = true;
	ticks++;
}

void pressed()
{
	iClick++;
	bClick = true;
}


int lastchan = 0;


// USB Port is used for SCS Host mode link to host.

// Must use interrupts (or possibly DMA) as we can't wait while processing sound.

// HostMode has a maximum frame size of around 262 bytes, and as it is polled
// we only need room for 1 frame

#define SCSBufferSize 280

char tx_buffer[SCSBufferSize];

// Circular buffer pointers
// volatile makes read-modify-write atomic
volatile int tx_in=0;
volatile int tx_out=0;
volatile int tx_stopped = 1;

char line[80];

void SerialSendData(unsigned char * Msg, int Len)
{
	int i;
	i = 0;

	while (i < Len)
	{
		tx_buffer[tx_in] = Msg[i++];
		tx_in = (tx_in + 1) % SCSBufferSize;
	}

	// disable ints to avoid possible race

    // Send first character to start tx interrupts, if stopped

	 __disable_irq();

    if (tx_stopped)
    {
        serial.putc(tx_buffer[tx_out]);
        tx_out = (tx_out + 1) % SCSBufferSize;
        tx_stopped = 0;
    }
	 __enable_irq();

    return;
}

void rxcallback()
{
    // Note: you need to actually read from the serial to clear the RX interrupt

	unsigned char c;
	c = serial.getc();

	SerialSink(c);
//	serial2.printf("%c", c);

//	 myled = !myled;
}

void txcallback()
{
	// Loop to fill more than one character in UART's transmit FIFO buffer
	// Stop if buffer empty

	 while ((serial.writeable()) && (tx_in != tx_out))
	 {
		 serial.putc(tx_buffer[tx_out]);
		 tx_out = (tx_out + 1) % SCSBufferSize;
	 }

	 if (tx_in == tx_out)
		 tx_stopped = 1;

	 return;
}

//  Port 3 is used for debugging

// Must use interrupts (or possibly DMA) as we can't wait while processing sound.

//	Not sure how big it needs to be. Don't want to use too mach RAM

#define DebugBufferSize 1024

char tx3_buffer[DebugBufferSize];

// Circular buffer pointers
// volatile makes read-modify-write atomic
volatile int tx3_in=0;
volatile int tx3_out=0;
volatile int tx3_stopped = 1;


void Serial3SendData(unsigned char * Msg, int Len)
{
	int i;
	i = 0;

	while (i < Len)
	{
		tx3_buffer[tx3_in] = Msg[i++];
		tx3_in = (tx3_in + 1) % DebugBufferSize;
	}

	// disable ints to avoid possible race

    // Send first character to start tx interrupts, if stopped

	 __disable_irq();

    if (tx3_stopped)
    {
        serial3.putc(tx3_buffer[tx3_out]);
        tx3_out = (tx3_out + 1) % DebugBufferSize;
        tx3_stopped = 0;
    }
	 __enable_irq();

    return;
}

void rx3callback()
{
    // Note: you need to actually read from the serial to clear the RX interrupt

	unsigned char c;

	c = serial3.getc();

//	SerialSink(c);
//	serial2.printf("%c", c);

//	 myled = !myled;
}

void tx3callback()
{
	// Loop to fill more than one character in UART's transmit FIFO buffer
	// Stop if buffer empty

	 while ((serial3.writeable()) && (tx3_in != tx3_out))
	 {
		 serial3.putc(tx3_buffer[tx3_out]);
		 tx3_out = (tx3_out + 1) % DebugBufferSize;
	 }

	 if (tx3_in == tx3_out)
		 tx3_stopped = 1;

	 return;
}




int main()
{
	serial.baud(115200);
	serial3.baud(115200);

	serial.attach(&rxcallback);
	serial.attach(&txcallback, Serial::TxIrq);

//	serial3.attach(&rxc3allback);
	serial3.attach(&tx3callback, Serial::TxIrq);

	Debugprintf("Clock Freq %d", SystemCoreClock);

	mybutton.fall(&pressed);

	ti.attach(tick, .001);

	ardopmain();
}

extern "C" void PlatformSleep()
{
	// Called at end of main loop

    if (bTick)
    {
  //			serial.printf("ADCInterrupts %i %d %d buffer no %d \r\n", ADCInterrupts,
  //					ADC_Buffer[0][0], ADC_Buffer[1][0], DMA_GetCurrentMemoryTarget(DMA2_Stream0));

   		bTick = false;
    }

    if (bClick)
    {
   			bClick = false;
    }

	myled = !myled;

    wait(delay);
}

void Sleep(int delay)
{
	wait(delay/1000);

	return;
}

VOID Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	Serial3SendData((unsigned char *)Mess, strlen(Mess));

	return;
}






