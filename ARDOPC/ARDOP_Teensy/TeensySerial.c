// ARDOP TNC Serial Interface for Teensy Board
//
#define TEENSY

// SCS Port is on USB. Debug on Serial1

#define TRUE 1
#define FALSE 0

int HostInit()
{
	return TRUE;
}

void PutString(unsigned char * Msg)
{
	SerialSendData(Msg, strlen(Msg));
}

int PutChar(unsigned char c)
{
	SerialSendData(&c, 1);
	return 0;
}

#include <stdarg.h>

void WriteDebugLog(int Level, const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	Serial1Print(Mess);

	return;
}
void WriteExceptionLog(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	Serial1Print(Mess);
	return;
}
void Statsprintf(const char * format, ...)
{
}
void CloseDebugLog()
{
}


void CloseStatsLog()
{
}




