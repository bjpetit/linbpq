// Mail and Chat Server for BPQ32 Packet Switch
//
//	Utility Routines

#include "stdafx.h"

int SEMCLASHES = 0;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	WriteLogLine(NULL, '!',Mess, Len, LOG_DEBUG);
//	#ifdef _DEBUG 
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
//	#endif
	return;
}

VOID __cdecl Logprintf(int LogMode, CIRCUIT * conn, int InOut, const char * format, ...)
{
	char Mess[1000];
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
//		Debugprintf("MailChat Semaphore Clash");
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
	char Mess[1000];
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

// Costimised message handling routines.
/*
	Variables - a subset of those used by FBB

 $C : Number of the next message.
 $I : First name of the connected user.
 $L : Number of the latest message.
 $N : Number of active messages.
 $U : Callsign of the connected user.
 $W : Inserts a carriage return.
 $Z : Last message read by the user (L command).
 %X : Number of messages for the user.
 %x : Number of new messages for the user.
*/

VOID ExpandAndSendMessage(CIRCUIT * conn, char * Msg, int LOG)
{
	char NewMessage[10000];
	char * OldP = Msg;
	char * NewP = NewMessage;
	char * ptr, * pptr;
	int len;
	char Dollar[] = "$";
	char CR[] = "\r";
	char num[20];


	ptr = strchr(OldP, '$');

	while (ptr)
	{
		len = ptr - OldP;		// Chars before $
		memcpy(NewP, OldP, len);
		NewP += len;

		switch (*++ptr)
		{
		case 'I': // First name of the connected user.

			pptr = conn->UserPointer->Name;
			break;

		case 'L': // Number of the latest message.

			wsprintf(num, "%d", LatestMsg);
			pptr = num;
			break;

		case 'N': // Number of active messages.

			wsprintf(num, "%d", NumberofMessages);
			pptr = num;
			break;


		case 'U': // Callsign of the connected user.

			pptr = conn->UserPointer->Call;
			break;

		case 'W': // Inserts a carriage return.

			pptr = CR;
			break;

		case 'Z': // Last message read by the user (L command).

			wsprintf(num, "%d", conn->UserPointer->lastmsg);
			pptr = num;
			break;

		case 'X': // Number of messages for the user.
		case 'x': // Number of new messages for the user.

		default:

			pptr = Dollar;		// Just Copy $
		}

		len = strlen(pptr);
		memcpy(NewP, pptr, len);
		NewP += len;

		OldP = ++ptr;
		ptr = strchr(OldP, '$');
	}

	strcpy(NewP, OldP);

	len = RemoveLF(NewMessage, strlen(NewMessage));

	WriteLogLine(conn, '>', NewMessage,  len, LOG);
	QueueMsg(conn, NewMessage, len);
}