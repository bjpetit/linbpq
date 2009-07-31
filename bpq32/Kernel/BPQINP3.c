//
//	INP3 Suport Code for BPQ32 Switch
//

//	All code runs from the BPQ32 Received or Timer Routines under Semaphore.
//	As most data areas are dynamically allocated, they will not survive a Timer Process Swap.
//	Shared data can be used for Config Info.


#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#pragma data_seg("_BPQDATA")

#include "windows.h"

#include "time.h"
#include "stdio.h"
#include "io.h"
#include <fcntl.h>					 
//#include "vmm.h"
#include "SHELLAPI.H"

#include "AsmStrucs.h"

typedef struct _RTTMSG
{
	UCHAR ID[7];
	UCHAR TXTIME[11];
	UCHAR SMOOTHEDRTT[11];
	UCHAR LASTRTT[11];
	UCHAR POINTER[11];
	UCHAR ALIAS[7];
	UCHAR VERSION[12];
	UCHAR SWVERSION[9];
	UCHAR FLAGS[10];
	UCHAR PADDING[147];

} RTTMSG;


#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

extern int SENDNETFRAME();

DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call);
DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall);
VOID __cdecl Debugprintf(const char * format, ...);


VOID SendNRRecordRoute(char * Call, struct TRANSPORTENTRY * Session);
VOID SendINP3RIF(struct ROUTE * Route, UCHAR * Call, UCHAR * Alias, int Hops, int RTT);
VOID SendOurRIF(struct ROUTE * Route);
VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt);
VOID UpdateRoute(struct DEST_LIST * Dest, struct DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt);

struct _RTTMSG RTTMsg = {""};

struct TRANSPORTENTRY * NRRSession;

// Timer Runs every 10 Secs

int RTTInterval = 24;			// 4 Minutes
int RTTRetries = 2;
int RTTTimeout = 6;				// 1 Min (Horizon is 1 min)

VOID InitialiseRTT()
{
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memcpy(RTTMsg.VERSION, "LEVEL3_V2.1 ", 12);
	memcpy(RTTMsg.SWVERSION, "BPQ32001 ", 9);
	memcpy(RTTMsg.FLAGS, "$M60000 $N ", 10);
	memcpy(RTTMsg.ALIAS, &MYALIAS, 6);
	RTTMsg.ALIAS[6] = ' ';
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memset(RTTMsg.PADDING, ' ', sizeof(RTTMsg.PADDING));
}

VOID SendNetFrame(struct _MESSAGE * Frame)
{
	UCHAR Port = Frame->PORT;
	int Len = Frame->LENGTH;
		
	_asm {

		pushfd
		cld
		pushad

		mov	ecx,Len
		mov	esi,Frame
		add	esi,7
		sub ecx,7

		mov	dl,Port
		call	SENDNETFRAME

		popad
		popfd
	}
		
	return;
}

TellINP3LinkGone(struct ROUTE * Route)
{
	Route->SRTT = 0;
	Route->RTT = 0;
	Route->BCTimer = 0;
	Route->Status = 0;
	Route->Timeout = 0;
}

VOID ProcessRTTReply(struct ROUTE * Route, struct _L3MESSAGE * Buff)
{
	int RTT;
	unsigned int OrigTime;

	if ((Route->Status & GotRTTResponse) == 0)
	{
		// Link is just starting

		Route->Status |= GotRTTResponse;

		if (Route->Status & GotRTTRequest)
		{
			Route->Status |= SentOurRIF;	
			SendOurRIF(Route);
		}
	}

	Route->Timeout = 0;			// Got Response
	
	sscanf(&Buff->L4DATA[6], "%d", &OrigTime);
	RTT = GetTickCount() - OrigTime;

	if (RTT > 60000)
		return;					// Ignore if more than 60 secs

	Route->RTT = RTT;

	if (Route->SRTT == 0)
		Route->SRTT = RTT;
	else
		Route->SRTT = ((Route->SRTT * 80)/100) + ((RTT * 20)/100);
}

VOID ProcessINP3RIF(struct ROUTE * Route, UCHAR * ptr1, int msglen, int Port)
{
	char axcall[7];
	int hops;
	short rtt;
	int len;
	int opcode;
	char alias[6];
	USHORT Stamp;

	return;

	// Update TImestamp on Route

	_asm{

	push	0
	call	time

	add	esp,4

	MOV	EDX,0
	mov ecx, 86400
	DIV	ecx					; REMAINDER IS SECS IN DAY
	MOV	EAX,EDX
	MOV	EDX,0
	mov ecx, 3600
	DIV	ecx					; GIVES HOURS, REM = SECS
	mov	ebx,eax				; SAVE

	MOV	EAX,EDX
	MOV	EDX,0
	mov ecx, 60
	DIV	ecx					; GIVES MINS, REM = SECS
	shl ebx,8
	or	ebx, eax
	mov Stamp, bx

}
	Route->NEIGHBOUR_TIME = Stamp;

	while (msglen > 0)
	{
		memset(alias, ' ', 6);	
		memcpy(axcall, ptr1, 7);

		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		msglen -= 10;

		while (*ptr1)
		{
			len = *ptr1;
			opcode = *(ptr1+1);

			if (opcode == 0)
				memcpy(alias, ptr1+2, len-2);

			ptr1+=len;
			msglen -=len;
		}

		ptr1++;
		msglen--;		// EOP

		UpdateNode(Route, axcall, alias, hops, rtt);
	}
	
	return;
}

VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt)
{
	struct DEST_LIST * Dest;
	struct DEST_ROUTE_ENTRY * ROUTEPTR;
	int i;

	_asm{


	MOV	ESI,axcall
	CALL	FINDDESTINATION
	mov	Dest, ebx

	JZ SHORT Found			; ALREADY THERE

	CMP	EBX,0
	JNE SHORT New

	}

	return;					// No Room

New:

	memset(Dest, 0, sizeof(struct DEST_LIST));

	memcpy(Dest->DEST_CALL, axcall, 7);
	memcpy(Dest->DEST_ALIAS, alias, 6);

//	Set up First Route

	Dest->ROUTE1.Hops = hops;
	Dest->ROUTE1.SRTT = rtt;
	Dest->ROUTE1.ROUT_NEIGHBOUR = Route;
	Dest->ROUTE1.ROUT_OBSCOUNT = 5;
	Dest->ROUTE1.ROUT_QUALITY = 10;				// Till we work out what to do

	NUMBEROFNODES++;

Found:

	// Update ALIAS

	memcpy(Dest->DEST_ALIAS, alias, 6);

	// See if we are known to it, it not add

	ROUTEPTR = &Dest->ROUTE1;

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->ROUTE2;

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->ROUTE3;

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	// Not in list. If any spare, add.
	// If full, see if this is better

	ROUTEPTR = &Dest->ROUTE1;

	for (i = 1; i < 4; i++)
	{
		if (ROUTEPTR->ROUT_NEIGHBOUR == NULL)
		{
			// Add here

			Dest->ROUTE1.Hops = hops;
			Dest->ROUTE1.SRTT = rtt;
			Dest->ROUTE1.ROUT_NEIGHBOUR = Route;
			Dest->ROUTE1.ROUT_OBSCOUNT = 5;
			Dest->ROUTE1.ROUT_QUALITY = 10;				// Till we work out what to do

			return;
		}
		ROUTEPTR++;
	}

	return;


/*	LEA	EDI,DEST_CALL[EBX]
	MOV	ECX,7
	REP MOVSB

	MOV	ECX,6			; ADD ALIAS
	MOV	ESI,OFFSET32 TEMPFIELD
	REP MOVSB

	POP	ESI
;
;	GET NEIGHBOURS FOR THIS DESTINATION
;
	CALL	CONVTOAX25
	JNZ SHORT BADROUTE
;
	CALL	GETVALUE
	MOV	SAVEPORT,AL		; SET PORT FOR _FINDNEIGHBOUR

	CALL	GETVALUE
	MOV	ROUTEQUAL,AL
;
	MOV	ESI,OFFSET32 AX25CALL

	PUSH	EBX			; SAVE DEST
	CALL	_FINDNEIGHBOUR
	MOV	EAX,EBX			; ROUTE TO AX
	POP	EBX

	JZ SHORT NOTBADROUTE

	JMP SHORT BADROUTE

NOTBADROUTE:
;
;	UPDATE ROUTE LIST FOR THIS DEST
;
	MOV	ROUT1_NEIGHBOUR[EBX],EAX
	MOV	AL,ROUTEQUAL
	MOV	ROUT1_QUALITY[EBX],AL
	MOV	ROUT1_OBSCOUNT[EBX],255	; LOCKED
;
	POP	EDI
	POP	EBX
	
	INC	_NUMBEROFNODES

	JMP	SENDOK
*/


	return;

}

VOID UpdateRoute(struct DEST_LIST * Dest, struct DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt)
{
	if (ROUTEPTR->Hops == 0)
	{
		// This is not a INP3 Route - Convert it

		ROUTEPTR->Hops = hops;
		ROUTEPTR->SRTT = rtt;

		return;
	}

	if (ROUTEPTR->SRTT > rtt)
	{
		// Improved
		
		ROUTEPTR->SRTT = rtt;
		ROUTEPTR->Hops = hops;
	}
}



VOID ProcessRTTMsg(struct ROUTE * Route, struct _L3MESSAGE * Buff, int Len, int Port)
{
	// See if a reply to our message, or a new request

	if (memcmp(Buff->L3SRCE, MYCALL,7) == 0)
	{
		ProcessRTTReply(Route, Buff);
		return;
	}
	else
	{
		int OtherRTT;
		int Dummy;
		struct _MESSAGE Msg;

		// Extract other end's SRTT

		sscanf(&Buff->L4DATA[6], "%d %d", &Dummy, &OtherRTT);
		Route->NeighbourSRTT = OtherRTT * 10;  // We store in mS

		// Echo Back to sender
		
		Route->INP3Node = TRUE; // Must be!

		memcpy(Msg.DEST, Buff->L3SRCE, 7);
		memcpy(Msg.ORIGIN, MYCALL, 7);
		Msg.PORT = Port;
		Msg.PID = NRPID;

		memcpy(Msg.L2DATA, Buff, Len);

		Msg.LENGTH = Len + 14 + 2 + 7;
	
		SendNetFrame(&Msg);

		if ((Route->Status & GotRTTRequest) == 0)
		{
			// Link is just starting

			Route->Status |= GotRTTRequest;
			
			if (Route->Status & GotRTTResponse)
			{
				Route->Status |= SentOurRIF;	
				SendOurRIF(Route);
			}
			else
			{
				// We have not yet seen a response (and maybe haven't sent one

				Route->BCTimer = 0;		// So send one
			}
		}
	}
}

VOID SendRTTMsg(struct ROUTE * Route)
{
	struct _MESSAGE Msg;
	char Stamp[50];

	memcpy(Msg.DEST, Route->NEIGHBOUR_CALL, 7);
	memcpy(Msg.ORIGIN, MYCALL, 7);
	Msg.PORT = Route->NEIGHBOUR_PORT;
	Msg.PID = NRPID;

	memcpy(Msg.L3MSG.L3DEST, L3RTT, 7);
	memcpy(Msg.L3MSG.L3SRCE, MYCALL, 7);
	Msg.L3MSG.L3TTL = 2;
	Msg.L3MSG.L4ID = 0;
	Msg.L3MSG.L4INDEX = 0;
	Msg.L3MSG.L4RXNO = 0;
	Msg.L3MSG.L4TXNO = 0;
	Msg.L3MSG.L4FLAGS = L4INFO;


	wsprintf(Stamp, "%10d %10d %10d %10d ", GetTickCount(), Route->SRTT/10, Route->RTT/10, 0);
	memcpy(RTTMsg.TXTIME, Stamp, 44);

	memcpy(Msg.L3MSG.L4DATA, &RTTMsg, 236);

	Msg.LENGTH = 256 + 14 + 2 + 7;

	Route->Timeout = RTTTimeout;

	SendNetFrame(&Msg);
}

int BuildRIF(UCHAR * RIF, UCHAR * Call, UCHAR * Alias, int Hops, int RTT)
{
	int AliasLen;
	int RIFLen;
	UCHAR AliasCopy[10] = "";
	UCHAR * ptr;

	// Need to null-terminate Alias

	memcpy(AliasCopy, Alias, 6);
	ptr = strchr(AliasCopy, ' ');

	if (ptr)
		*ptr = 0;

	AliasLen = strlen(AliasCopy);

	memcpy(&RIF[0], Call, 7);
	RIF[7] = Hops;
	RIF[8] = HIBYTE(RTT);
	RIF[9] = LOBYTE(RTT);
	RIF[10] = AliasLen+2;
	RIF[11] = 0;
	memcpy(&RIF[12], Alias, AliasLen);
	RIF[12+AliasLen] = 0;

	RIFLen = 13 + AliasLen;

	return RIFLen;
}

VOID SendINP3RIF(struct ROUTE * Route, UCHAR * Call, UCHAR * Alias, int Hops, int RTT)
{
	struct _MESSAGE Msg;
	int RIFLen;
	int totLen = 1;
	UCHAR AliasCopy[10] = "";
	UCHAR * ptr;

	// Need to null-terminate Alias

	memcpy(AliasCopy, Alias, 6);
	ptr = strchr(AliasCopy, ' ');

	if (ptr)
		*ptr = 0;

	Msg.L3MSG.L3SRCE[0] = 0xff;

	RIFLen = BuildRIF(&Msg.L3MSG.L3SRCE[totLen], Call, AliasCopy, Hops, RTT);
	totLen += RIFLen;
	RIFLen = BuildRIF(&Msg.L3MSG.L3SRCE[totLen], "GM8BPQ-11", "BPQXXX", 2, 98);
	totLen += RIFLen;

	memcpy(Msg.DEST, Route->NEIGHBOUR_CALL, 7);
	memcpy(Msg.ORIGIN, MYCALL, 7);
	Msg.PORT = Route->NEIGHBOUR_PORT;
	Msg.PID = NRPID;
	Msg.LENGTH = totLen + 14 + 2 + 7;

	SendNetFrame(&Msg);
}

VOID SendOurRIF(struct ROUTE * Route)
{
	struct _MESSAGE Msg;
	int RIFLen;
	int totLen = 1;
	int App;
	struct APPLCALLS * APPL;

	Msg.L3MSG.L3SRCE[0] = 0xff;

	// send a RIF for our Node and all our APPLCalls

	RIFLen = BuildRIF(&Msg.L3MSG.L3SRCE[totLen], MYCALL, MYALIAS, 1, 0);
	totLen += RIFLen;

	for (App = 0; App < 8; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			RIFLen = BuildRIF(&Msg.L3MSG.L3SRCE[totLen], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, 0);
			totLen += RIFLen;
		}
	}

	memcpy(Msg.DEST, Route->NEIGHBOUR_CALL, 7);
	memcpy(Msg.ORIGIN, MYCALL, 7);
	Msg.PORT = Route->NEIGHBOUR_PORT;
	Msg.PID = NRPID;
	Msg.LENGTH = totLen + 14 + 2 + 7;

	SendNetFrame(&Msg);
}

UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, int msglen)
{
	char call[10];
	int calllen;
	int hops;
	unsigned short rtt;
	unsigned int len;
	int opcode;
	char alias[10] = "";

	ptr2+=wsprintf(ptr2, " INP3 RIF:\r  Call Alias Hops RTT\r");

	while (msglen > 0)
	{
		calllen = ConvFromAX25(ptr1, call);
		call[calllen] = 0;

		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		msglen -= 10;

		while (*ptr1)
		{
			len = *ptr1;
			opcode = *(ptr1+1);

			if (opcode == 0 && len < 9)
			{
				memcpy(alias, ptr1+2, len-2);
				alias[len-2] = 0;
			}
			ptr1+=len;
			msglen -=len;
		}

		ptr2+=wsprintf(ptr2, "  %s:%s% %d %4.2d\r", alias, call, hops, rtt);

		ptr1++;
		msglen--;		// EOP
	}
	
	return ptr2;
}

SendRIPTimer()
{
	int count;
	struct ROUTE * Route = DataBase->NEIGHBOURS;
	int MaxRoutes = DataBase->MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		if (Route->NEIGHBOUR_CALL[0] != 0)
		{
			if (Route->INP3Node)
			{
				if (Route->Timeout)
				{
					// Waiting for response

					Route->Timeout--;

					if (Route->Timeout)
						continue;				// Wait

					// No response Try again

					Route->Retries--;

					if (Route->Retries)
					{
						// More Left

						SendRTTMsg(Route);
					}
					else
					{
						// No Response - Kill all Nodes via this link

						if (Route->Status)
						{
							char Call [11] = "";

							ConvFromAX25(Route->NEIGHBOUR_CALL, Call);
							Debugprintf("BPQ32 IMP Neighbour %s Lost", Call);

							Route->Status = 0;	// Down
						}

						Route->BCTimer=5;		// Wait a while before retrying
					}
				}

				if (Route->BCTimer)
				{
					Route->BCTimer--;
				}
				else
				{
					Route->BCTimer = RTTInterval;
					Route->Retries = RTTRetries;
					SendRTTMsg(Route);
				}
//				SendNRRecordRoute("GM8BPQ-8", 2);
			}
		}

		Route++;
	}

	return (0);
}

INP3TIMER()
{
	if (RTTMsg.ID[0] == 0)
		InitialiseRTT();

	SendRIPTimer();
}

/*
datagrams (and other things) to be transported in Netrom L3 frames. 
When the frametype is 0x00, the "circuit index" and "circuit id" (first 2 
bytes of the transport header) take on a different meaning, something like 
"protocol family" and "protocol id". IP over netrom uses 0x0C for both 
bytes, TheNet uses 0x00 for both bytes when making L3RTT measurements, and 
Xnet uses family 0x00, protocol id 0x01 for Netrom Record Route. I believe 
there are authors using other values too. Unfortunately there is no 
co-ordinating authority for these numbers, so authors just pick an unused 
one. 
*/

VOID NRRecordRoute(char * Buff, int Len)
{
	// NRR frame for us. If We originated it, report outcome, else put our call on end, and send back
	
	struct _MESSAGE Msg = {0};
	int NRRLen = Len - 28;
	UCHAR Flags;
	char call[10];
	int calllen;


	if (memcmp(&Buff[28], MYCALL, 7) == 0)
	{
		UCHAR * BUFFER;
		UCHAR * ptr1;
		VOID * ptr;

		struct _MESSAGE * Msg;

		_asm{
			
		call GETBUFF
		mov BUFFER, edi

		}
		if (BUFFER == NULL)
			return;

		ptr1 = &BUFFER[7];
		
		*ptr1++ = 0xf0;			// PID

		_asm{

		mov	edi,ptr1

//		CALL	SETUPNODEHEADER		; PUT IN NODE ID

		mov ptr1,edi

		}

		ptr1 += wsprintf(ptr1, "NRR Response:");

		Buff+=28;
		Len -= 28;

		while (Len > 0)
		{
			calllen = ConvFromAX25(Buff, call);
			call[calllen] = 0;
			ptr1 += wsprintf(ptr1, " %s", call);
			if ((Buff[7] & 0x80) == 0x80)			// Check turnround bit
				*ptr1++ = '*';
	
			Buff+=8;
			Len -= 8;
		}

		// Add ours on end for neatness

		calllen = ConvFromAX25(MYCALL, call);
		call[calllen] = 0;
		ptr1 += wsprintf(ptr1, " %s", call);

		*ptr1++ = 0x0d;			// CR

		Len = ptr1 - BUFFER;

		Msg = (struct _MESSAGE *)BUFFER;
		
		Msg->LENGTH = Len;

		Msg->CHAIN = NULL;

		ptr1 = &NRRSession->L4TX_Q;

		_asm {	
			
		pushad

		mov esi, ptr1
		mov	edi, BUFFER;

		call Q_ADD

		mov	ebx, NRRSession 
		CALL	POSTDATAAVAIL

		popad

		}

		return;
	}

	// Add our call on end, and increase count

	Flags = Buff[Len -1];

	Flags--;

	if (Flags && NRRLen < 228)					// Dont update if full
	{
		Flags |= 0x80;			// Set End of route bit
		Msg.PID = NRPID;

		memcpy(Msg.L3MSG.L3DEST, &Buff[8], 7);
		memcpy(Msg.L3MSG.L3SRCE, &Buff[15], 7);
		Msg.L3MSG.L3TTL = L3LIVES;
		Msg.L3MSG.L4ID = 1;

		memcpy(Msg.L3MSG.L4DATA, &Buff[28], NRRLen);
		memcpy(&Msg.L3MSG.L4DATA[NRRLen], &Buff[15], 7);
		Msg.L3MSG.L4DATA[NRRLen+7] = Flags;
		NRRLen += 8;
	}
		
	Msg.LENGTH = NRRLen + 20 + 14 + 2 + 7;
	
	SendNetFrame(&Msg);

}

VOID SendNRRecordRoute(char * Call, struct TRANSPORTENTRY * Session)
{	
	struct _MESSAGE Msg = {0};
	int Stream = 1;

	NRRSession = Session;			// Save Session Pointer for reply

	Msg.PID = NRPID;

	memcpy(Msg.L3MSG.L3DEST, Call, 7);
	memcpy(Msg.L3MSG.L3SRCE, MYCALL, 7);
		
	Msg.L3MSG.L3TTL = L3LIVES;
	Msg.L3MSG.L4ID = 1;

	memcpy(Msg.L3MSG.L4DATA, MYCALL, 7);
	Msg.L3MSG.L4DATA[7] = Stream + 28;
		
	Msg.LENGTH = 8 + 20 + 14 + 2 + 7;
	
	SendNetFrame(&Msg);

}
