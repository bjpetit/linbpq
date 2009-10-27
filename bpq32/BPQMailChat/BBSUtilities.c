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
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	WriteLogLine(NULL, '!',Mess, Len, LOG_DEBUG);
	#ifdef _DEBUG 
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
	#endif
	return;
}

VOID __cdecl Logprintf(int LogMode, CIRCUIT * conn, int InOut, const char * format, ...)
{
	char Mess[255];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	WriteLogLine(conn, InOut, Mess, Len, LogMode);

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

	WriteLogLine(conn, '>',buf,  strlen(buf) -1, LOG_BBS);

	QueueMsg(conn, buf, strlen(buf));
}

VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[255];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf_s(Mess, sizeof(Mess), format, arglist);

	QueueMsg(conn, Mess, len);

	WriteLogLine(conn, '>',Mess, len-1, LOG_BBS);

	return;
}

int compare( const void *arg1, const void *arg2 );

VOID SortBBSChain()
{
	struct UserInfo * user;
	struct UserInfo * users[100]; 
	int i = 0, n;

	// Get array of addresses

	for (user = BBSChain; user; user = user->BBSNext)
	{
		users[i++] = user;
		if (i > 99) break;
	}

	qsort((void *)users, i, 4, compare );

	BBSChain = NULL;

	// Rechain (backwards, as entries ate puton front of chain)

	for (n = i-1; n >= 0; n--)
	{
		users[n]->BBSNext = BBSChain;
		BBSChain = users[n];
	}
}

int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}
