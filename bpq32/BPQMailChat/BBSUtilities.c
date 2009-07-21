// Mail and Chat Server for BPQ32 Packet Switch
//
//	Utility Routines

#include "stdafx.h"

int SEMCLASHES = 0;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	OutputDebugString(Mess);

	return;
}

void GetSemaphore(int * Semaphore)
{
	//
	//	Wait for it to be free
	//
	
	if (*Semaphore != 0)
		Debugprintf("MailChat Semaphore Clash");

loop1:

	while (*Semaphore != 0)
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
void FreeSemaphore(int * Semaphore)
{
	*Semaphore=0;

	return; 
}
