// Mail and Chat Server for BPQ32 Packet Switch
//
//	Utility Routines

#include "stdafx.h"

int SEMCLASHES = 0;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf(Mess, format, arglist);
	WriteLogLine('!',Mess, Len, LOG_DEBUG);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

void GetSemaphore(struct SEM * Semaphore)
{
	//
	//	Wait for it to be free
	//
	
	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
		Debugprintf("MailChat Semaphore Clash");
	}
loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	mov ebx, Semaphore
	xchg [ebx],eax		// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, we've got the semaphore
;
	}

	return;
}
void FreeSemaphore(struct SEM * Semaphore)
{
	Semaphore->Flag=0;

	return; 
}
VOID BBSputs(CIRCUIT * conn, char * buf)
{
	// Sends to user and logs

	WriteLogLine('>',buf,  strlen(buf) -1, LOG_BBS);

	QueueMsg(conn, buf, strlen(buf));
}

VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[255];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf(Mess, format, arglist);

	QueueMsg(conn, Mess, len);

	WriteLogLine('>',Mess, len-1, LOG_BBS);

	return;
}