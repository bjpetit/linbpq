
#include "mbed.h"
#include "ARDOPC.h"


#include <stdarg.h>

//#include "stm32f4xx.h"


//AnalogOut my_output(PA_4);


//short buffer[BUFFER_SIZE];


extern "C" void GenerateFSKTemplates();
extern "C" void InitValidFrameTypes();
extern "C" void SendID(BOOL blnEnableCWID);
extern "C" void ardopmain();
extern "C" void printtick(char * msg);
extern "C" VOID Debugprintf(const char * format, ...);

InterruptIn mybutton(USER_BUTTON);
DigitalOut myled(LED1);
Ticker ti;
Serial serial(USBTX, USBRX);

float delay = 1.0; // 1 sec
volatile int iTick = 0;
volatile bool bTick = 0;
volatile int iClick = 0;
volatile bool bClick = 0;
volatile int Ticks = 0;

int i = 0;

void tick()
{
	bTick = true;
	Ticks++;
	iTick++;
}

void pressed()
{
	iClick++;
	bClick = true;
    if (delay == 1.0)
        delay = 0.2; // 200 ms
    else
        delay = 1.0; // 1 sec
}

void InitSound()
{
}


int lastchan = 0;


int main()
{
	float jnw;

	jnw = 1.0;

	jnw = jnw * 4.0;


	serial.baud(115200);
	serial.printf("Clock Freq %d\r\n", SystemCoreClock);

	mybutton.fall(&pressed);

	blnTimeoutTriggered = FALSE;

//	GenerateTwoToneLeaderTemplate();
//	GenerateFSKTemplates();
//	InitValidFrameTypes();
//	InitSound();


	ti.attach(tick, .001);

	ardopmain();

//	SendID(0);

	 ProtocolState = FECSend;
//	GetNextFECFrame();

//
//  SendID(0);
//  ModTwoToneTest();

    while (1)
    {
    	if (bTick)
    	{
 //  			serial.printf("timer tick %i\r\n", iTick);
   			bTick = false;
    	}
    	if (bClick)
    	{
   			serial.printf("button click %i\r\n", iClick);
   			bClick = false;
   			SendID(0);
    	}

	//	serial.printf("Delay Expires\r\n");

        myled = !myled;
        wait(delay);
    }
}

int Lasttick = 0;

void printtick(char * msg)
{
	serial.printf("%s %i\r\n", msg, iTick - Lasttick);
	Lasttick = iTick;
}

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	serial.printf(Mess);

	return;
}





