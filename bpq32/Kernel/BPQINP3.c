//
//	INP3 Suport Code for BPQ32 Switch
//

//	All code runs from the BPQ32 Received or Timer Routines under Semaphore.


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
extern VOID Q_ADD();


DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call);
DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall);
VOID __cdecl Debugprintf(const char * format, ...);


VOID SendINP3RIF(struct ROUTE * Route, UCHAR * Call, UCHAR * Alias, int Hops, int RTT);
VOID SendOurRIF(struct ROUTE * Route);
VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt);
VOID UpdateRoute(struct DEST_LIST * Dest, struct DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt);
VOID KillRoute(struct DEST_ROUTE_ENTRY * ROUTEPTR);
VOID AddHere(struct DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt);
VOID SendNetFrame(struct _MESSAGE * Frame);
VOID SendRIPToNeighbour(struct ROUTE * Route);
VOID DeleteNETROMRoutes(struct ROUTE * Route);
VOID DeleteINP3Routes(struct ROUTE * Route);

//#define NOINP3

#ifdef NOINP3

TellINP3LinkGone(struct ROUTE * Route)
{
	return 0;
}
VOID TellINP3LinkSetupFailed(struct ROUTE * Route)
{
}

VOID ProcessINP3RIF(struct ROUTE * Route, UCHAR * ptr1, int msglen, int Port)
{
	return;
}
VOID ProcessRTTMsg(struct ROUTE * Route, struct _L3MESSAGE * Buff, int Len, int Port)
{
	return;
}
INP3TIMER()
{
	return 0;
}



#else


struct _RTTMSG RTTMsg = {""};

//struct ROUTE DummyRoute = {"","",""};

int RIPTimerCount = 0;				// 1 sec to 10 sec counter
int PosTimerCount = 0;				// 1 sec to 5 Mins counter

// Timer Runs every 10 Secs

int MAXRTT = 9000;			// 90 secs

int RTTInterval = 24;			// 4 Minutes
int RTTRetries = 2;
int RTTTimeout = 6;				// 1 Min (Horizon is 1 min)

VOID InitialiseRTT()
{
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memcpy(RTTMsg.VERSION, "LEVEL3_V2.1 ", 12);
	memcpy(RTTMsg.SWVERSION, "BPQ32001 ", 9);
	wsprintf(RTTMsg.FLAGS, "$M%d $N   ", MAXRTT);
	memcpy(RTTMsg.ALIAS, &MYALIAS, 6);
	RTTMsg.ALIAS[6] = ' ';
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memset(RTTMsg.PADDING, ' ', sizeof(RTTMsg.PADDING));
}

VOID TellINP3LinkGone(struct ROUTE * Route)
{
	struct DEST_LIST * Dest =  DataBase->DESTS;
	char call[11]="";

	ConvFromAX25(Route->NEIGHBOUR_CALL, call);
	Debugprintf("BPQ32 L2 Link to Neighbour %s lost", call);

	if (Route->NEIGHBOUR_LINK)
		Debugprintf("BPQ32 Neighbour_Link not cleared");


	if (Route->INP3Node == 0)
		DeleteNETROMRoutes(Route);
	else
		DeleteINP3Routes(Route);
}

VOID DeleteINP3Routes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DataBase->DESTS;

	// Delete any NETROM Dest entries via this Route

	Route->SRTT = 0;
	Route->RTT = 0;
	Route->BCTimer = 0;
	Route->Status = 0;
	Route->Timeout = 0;

	Dest--;

	// Delete any Dest entries via this Route

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry


		if (Dest->ROUTE1.ROUT_NEIGHBOUR == Route)
		{
			//	We are deleting the best route, so need to tell other nodes
			//	If this is the only one, we need to keep the entry with at 60000 rtt so
			//	we can send it. Remove when all gone

			//	How do we indicate is is dead - Maybe the 60000 is enough!
			if (Dest->ROUTE2.ROUT_NEIGHBOUR == 0)
			{
				// Only entry
				Dest->ROUTE1.SRTT = 60000;
				Dest->ROUTE1.Hops = 255;

				continue;
			}

			Dest->ROUTE2.LastRTT  = Dest->ROUTE1.SRTT;		// So next scan will check if rtt has increaced
															// enough to need a RIF
				
			memcpy(&Dest->ROUTE1, &Dest->ROUTE2, sizeof(struct DEST_ROUTE_ENTRY));
			memcpy(&Dest->ROUTE2, &Dest->ROUTE3, sizeof(struct DEST_ROUTE_ENTRY));
			memset(&Dest->ROUTE3, 0, sizeof(struct DEST_ROUTE_ENTRY));

			continue;
		}

		// If we aren't removing the best, we don't need to tell anyone.
		
		if (Dest->ROUTE2.ROUT_NEIGHBOUR == Route)
		{
			memcpy(&Dest->ROUTE2, &Dest->ROUTE3, sizeof(struct DEST_ROUTE_ENTRY));
			memset(&Dest->ROUTE3, 0, sizeof(struct DEST_ROUTE_ENTRY));

			continue;
		}

		if (Dest->ROUTE3.ROUT_NEIGHBOUR == Route)
		{
			memset(&Dest->ROUTE3, 0, sizeof(struct DEST_ROUTE_ENTRY));
			continue;
		}
	}
}

VOID DeleteNETROMRoutes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DataBase->DESTS;

	Dest--;

	// Delete any NETROM Dest entries via this Route

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry

		if (Dest->NRROUTE1.ROUT_NEIGHBOUR == Route)
		{	
			if (Dest->NRROUTE2.ROUT_NEIGHBOUR == 0)			// No more Netrom Routes
			{
				if (Dest->ROUTE1.ROUT_NEIGHBOUR == 0)		// Any INP3 ROutes?
				{
					// No More Routes - ZAP Dest

					{
						char call[11]="";
						ConvFromAX25(Dest->DEST_CALL, call);
						Debugprintf("Deleting NR Node %s", call);
					}

					_asm
					{
						pushad
						mov	EBX,Dest
						CALL REMOVENODE			// Clear buffers, Remove from Sorted Nodes chain, and zap entry
						popad
					}

					continue;
				}
				else
				{
					// Still have an INP3 Route - just zap this entry

					memset(&Dest->NRROUTE1, 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
					continue;
				}
			}

			memcpy(&Dest->NRROUTE1, &Dest->NRROUTE2, sizeof(struct NR_DEST_ROUTE_ENTRY));
			memcpy(&Dest->NRROUTE2, &Dest->NRROUTE3, sizeof(struct NR_DEST_ROUTE_ENTRY));
			memset(&Dest->NRROUTE3, 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

			continue;
		}
		
		if (Dest->NRROUTE2.ROUT_NEIGHBOUR == Route)
		{
			memcpy(&Dest->NRROUTE2, &Dest->NRROUTE3, sizeof(struct NR_DEST_ROUTE_ENTRY));
			memset(&Dest->NRROUTE3, 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

			continue;
		}

		if (Dest->NRROUTE3.ROUT_NEIGHBOUR == Route)
		{
			memset(&Dest->NRROUTE3, 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
			continue;
		}
	}
}


VOID TellINP3LinkSetupFailed(struct ROUTE * Route)
{
	// Attempt to activate Neighbour failed
	
//	char call[11]="";

//	ConvFromAX25(Route->NEIGHBOUR_CALL, call);
//	Debugprintf("BPQ32 L2 Link to Neighbour %s setup failed", call);


	if (Route->INP3Node == 0)
		DeleteNETROMRoutes(Route);
	else
		DeleteINP3Routes(Route);
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
			SendRIPToNeighbour(Route);
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
	unsigned short rtt;
	int len;
	int opcode;
	char alias[6];
	USHORT Stamp;

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

VOID KillRoute(struct DEST_ROUTE_ENTRY * ROUTEPTR)
{
}



VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, UCHAR * alias, int  hops, int rtt)
{
	struct DEST_LIST * Dest;
	struct DEST_ROUTE_ENTRY * ROUTEPTR;
	int i;
	char call[11]="";

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

	if (rtt >= 60000)
		return;				// No Point addind a new dead route

	memset(Dest, 0, sizeof(struct DEST_LIST));

	memcpy(Dest->DEST_CALL, axcall, 7);
	memcpy(Dest->DEST_ALIAS, alias, 6);

//	Set up First Route

	Dest->ROUTE1.Hops = hops;
	Dest->ROUTE1.SRTT = rtt;
	Dest->ROUTE1.LastRTT = 0;

	Dest->INP3FLAGS = NewNode;

	Dest->ROUTE1.ROUT_NEIGHBOUR = Route;

	NUMBEROFNODES++;

	ConvFromAX25(Dest->DEST_CALL, call);
	Debugprintf("Adding  Node %s Hops %d RTT %d", call, hops, rtt);

	return;

Found:

	// Update ALIAS

	memcpy(Dest->DEST_ALIAS, alias, 6);

	// See if we are known to it, it not add

	ROUTEPTR = &Dest->ROUTE1;

	if (rtt >= 60000)
	{
		i=rtt+1;
	}

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

	if (rtt >= 60000)
		return;				// No Point addind a new dead route

	ROUTEPTR = &Dest->ROUTE1;

	for (i = 1; i < 4; i++)
	{
		if (ROUTEPTR->ROUT_NEIGHBOUR == NULL)
		{
			// Add here

			Dest->ROUTE1.Hops = hops;
			Dest->ROUTE1.SRTT = rtt;
			Dest->ROUTE1.ROUT_NEIGHBOUR = Route;

			return;
		}
		ROUTEPTR++;
	}

	// Full, see if this is better

	// Note that wont replace any netrom routes with INP3 ones unless we add pseudo rtt values to netrom entries

	if (Dest->ROUTE1.SRTT > rtt)
	{
		// We are better. Move others down and add on front

		memcpy(&Dest->ROUTE3, &Dest->ROUTE2, sizeof(struct DEST_ROUTE_ENTRY));
		memcpy(&Dest->ROUTE2, &Dest->ROUTE1, sizeof(struct DEST_ROUTE_ENTRY));

		AddHere(&Dest->ROUTE1, Route, hops, rtt);
		return;
	}

	if (Dest->ROUTE2.SRTT > rtt)
	{
		// We are better. Move  2nd down and add

		memcpy(&Dest->ROUTE3, &Dest->ROUTE2, sizeof(struct DEST_ROUTE_ENTRY));

		AddHere(&Dest->ROUTE2, Route, hops, rtt);
		return;
	}

	if (Dest->ROUTE3.SRTT > rtt)
	{
		// We are better. Add here

		AddHere(&Dest->ROUTE3, Route, hops, rtt);
		return;
	}

	// Worse than any - ignoee

}

VOID AddHere(struct DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt)
{
	ROUTEPTR->Hops = hops;
	ROUTEPTR->SRTT = rtt;
	ROUTEPTR->LastRTT = 0;
	ROUTEPTR->RTT = 0;
	ROUTEPTR->ROUT_NEIGHBOUR = Route;

	return;
}


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


VOID SortRoutes(struct DEST_LIST * Dest)
{
	 struct DEST_ROUTE_ENTRY Temp;

	// May now be out of order

	if (Dest->ROUTE2.ROUT_NEIGHBOUR == 0)
		return;						// Only One, so cant be out of order
	
	if (Dest->ROUTE3.ROUT_NEIGHBOUR == 0)
	{
		// Only 2

		if (Dest->ROUTE1.SRTT <= Dest->ROUTE2.SRTT)
			return;

		// Swap one and two

		memcpy(&Temp, &Dest->ROUTE1, sizeof(struct DEST_ROUTE_ENTRY));
		memcpy(&Dest->ROUTE1, &Dest->ROUTE2, sizeof(struct DEST_ROUTE_ENTRY));
		memcpy(&Dest->ROUTE2, &Temp, sizeof(struct DEST_ROUTE_ENTRY));

		return;
	}

	// Have 3 Entries
}



VOID UpdateRoute(struct DEST_LIST * Dest, struct DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt)
{
	if (ROUTEPTR->Hops == 0)
	{
		// This is not a INP3 Route - Convert it

		ROUTEPTR->Hops = hops;
		ROUTEPTR->SRTT = rtt;

		SortRoutes(Dest);
		return;
	}

	if (rtt == 60000)
	{
		ROUTEPTR->SRTT = rtt;
		ROUTEPTR->Hops = hops;

		SortRoutes(Dest);
		return;

	}

	ROUTEPTR->SRTT = rtt;
	ROUTEPTR->Hops = hops;
	
	SortRoutes(Dest);
	return;
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

		if (Route->INP3Node == 0)
			return;						// We don't want to use INP3

		// Extract other end's SRTT

		sscanf(&Buff->L4DATA[6], "%d %d", &Dummy, &OtherRTT);
		Route->NeighbourSRTT = OtherRTT * 10;  // We store in mS

		// Echo Back to sender

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
				SendRIPToNeighbour(Route);

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

VOID SendKeepAlive(struct ROUTE * Route)
{
	struct _MESSAGE Msg;

	memcpy(Msg.DEST, Route->NEIGHBOUR_CALL, 7);
	memcpy(Msg.ORIGIN, MYCALL, 7);
	Msg.PORT = Route->NEIGHBOUR_PORT;
	Msg.PID = NRPID;

	memcpy(Msg.L3MSG.L3DEST, L3KEEP, 7);
	memcpy(Msg.L3MSG.L3SRCE, MYCALL, 7);
	Msg.L3MSG.L3TTL = 1;
	Msg.L3MSG.L4ID = 0;
	Msg.L3MSG.L4INDEX = 0;
	Msg.L3MSG.L4RXNO = 0;
	Msg.L3MSG.L4TXNO = 0;
	Msg.L3MSG.L4FLAGS = L4INFO;

	Msg.L3MSG.L4DATA[0] = 'K';

	Msg.LENGTH = 20 + 14 + 2 + 7;

	SendNetFrame(&Msg);
}

int BuildRIF(UCHAR * RIF, UCHAR * Call, UCHAR * Alias, int Hops, int RTT)
{
	int AliasLen;
	int RIFLen;
	UCHAR AliasCopy[10] = "";
	UCHAR * ptr;


	if (RTT > 60000) RTT = 60000;	// Dont send more than 60000

	memcpy(&RIF[0], Call, 7);
	RIF[7] = Hops;
	RIF[8] = HIBYTE(RTT);
	RIF[9] = LOBYTE(RTT);

	if (Alias)
	{
		// Need to null-terminate Alias
		
		memcpy(AliasCopy, Alias, 6);
		ptr = strchr(AliasCopy, ' ');

		if (ptr)
			*ptr = 0;

		AliasLen = strlen(AliasCopy);

		RIF[10] = AliasLen+2;
		RIF[11] = 0;
		memcpy(&RIF[12], Alias, AliasLen);
		RIF[12+AliasLen] = 0;

		RIFLen = 13 + AliasLen;
		return RIFLen;
	}
	RIF[10] = 0;
	
	return (11);
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

SendRIPTimer()
{
	int count;
	struct ROUTE * Route = DataBase->NEIGHBOURS;
	int MaxRoutes = DataBase->MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		if (Route->NEIGHBOUR_CALL[0] != 0)
		{
			if (Route->NEIGHBOUR_LINK == 0 || Route->NEIGHBOUR_LINK->LINKPORT == 0)
			{
				if (Route->NEIGHBOUR_QUAL == 0)
					continue;						// Qual zero is a locked out route

				// Try to activate link

				_asm
				{
					//	EBX POINTS TO A NEIGHBOUR - FIND AN L2 SESSION FROM US TO IT, OR Create one

					mov	ebx, Route
					pushad
					call	L2SETUPCROSSLINK
					popad
				}
				
				if (Route->NEIGHBOUR_LINK == 0)
				{
					Route++;
					continue;						// No room for link
				}
			}

			if (Route->NEIGHBOUR_LINK->L2STATE != 5)	// Not up yet
			{
				Route++;
				continue;
			}

			if (Route->NEIGHBOUR_LINK->KILLTIMER > 600)
			{
				SendKeepAlive(Route);
				Route->NEIGHBOUR_LINK->KILLTIMER = 0;		// Keep Open
			}

			if (Route->INP3Node)
			{
				if (Route->Timeout)
				{
					// Waiting for response

					Route->Timeout--;

					if (Route->Timeout)
					{
						Route++;
						continue;				// Wait
					}
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
			}
		}

		Route++;
	}

	return (0);
}

// Create an Empty RIF

struct _MESSAGE * CreateRIFHeader(struct ROUTE * Route)
{
	struct _MESSAGE * Msg = malloc(sizeof(struct _MESSAGE));
	UCHAR AliasCopy[10] = "";

	Msg->LENGTH = 1;
	Msg->L3MSG.L3SRCE[0] = 0xff;

	memcpy(Msg->DEST, Route->NEIGHBOUR_CALL, 7);
	memcpy(Msg->ORIGIN, MYCALL, 7);
	Msg->PORT = Route->NEIGHBOUR_PORT;
	Msg->PID = NRPID;

	return Msg;

}

SendRIF(struct _MESSAGE * Msg)
{
	Msg->LENGTH += 14 + 2 + 7;

	SendNetFrame(Msg);

	free(Msg);
}

SendRIPToOtherNeighbours(UCHAR * axcall, UCHAR * alias, struct DEST_ROUTE_ENTRY * Entry)
{
	struct ROUTE * Routes = DataBase->NEIGHBOURS;
	struct _MESSAGE * Msg;
	int count, MaxRoutes = DataBase->MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		if ((Routes->INP3Node) && 
			(Routes->Status) && 
			(Routes != Entry->ROUT_NEIGHBOUR))	// Dont send to originator of route
		{
			Msg = Routes->Msg;
			
			if (Msg == NULL) 
				Msg = Routes->Msg = CreateRIFHeader(Routes);
			
			Msg->LENGTH += BuildRIF(&Msg->L3MSG.L3SRCE[Msg->LENGTH],
				axcall, alias, Entry->Hops + 1, Entry->SRTT + Entry->ROUT_NEIGHBOUR->SRTT/10);

			if (Msg->LENGTH > 250 - 15)
//			if (Msg->LENGTH > Routes->NBOUR_PACLEN - 11)
			{
				SendRIF(Msg);
				Routes->Msg = NULL;
			}
		}
		Routes+=1;
	}
}

VOID SendRIPToNeighbour(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest =  DataBase->DESTS;
	struct DEST_ROUTE_ENTRY * Entry;
	struct _MESSAGE * Msg;

	Dest--;

	// Send all entries not via this Neighbour - used when link starts

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->ROUTE1;

		if (Entry->ROUT_NEIGHBOUR && Entry->Hops && Route != Entry->ROUT_NEIGHBOUR)	
		{
			// Best Route not via this neighbour - send
		
			Msg = Route->Msg;
			
			if (Msg == NULL) 
				Msg = Route->Msg = CreateRIFHeader(Route);
			
			Msg->LENGTH += BuildRIF(&Msg->L3MSG.L3SRCE[Msg->LENGTH],
				Dest->DEST_CALL, Dest->DEST_ALIAS,
				Entry->Hops + 1, Entry->SRTT + Entry->ROUT_NEIGHBOUR->SRTT/10);

			if (Msg->LENGTH > 250 - 15)
			{
				SendRIF(Msg);
				Route->Msg = NULL;
			}
		}
	}
	if (Route->Msg)
	{
		SendRIF(Route->Msg);
		Route->Msg = NULL;
	}
}

FlushRIFs()
{
	struct ROUTE * Routes = DataBase->NEIGHBOURS;
	int count, MaxRoutes = DataBase->MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		if (Routes->Msg)
		{
			SendRIF(Routes->Msg);
			Routes->Msg = NULL;
		}
		Routes+=1;
	}
}

VOID SendNegativeInfo()
{
	int i, Preload;
	struct DEST_LIST * Dest =  DataBase->DESTS;
	struct DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have got worse
	
	// ?? Should we send to one Neighbour at a time, or do all in parallel ??

	// The spec says send Negative info as soon as possible so I'll try building them in parallel
	// That will mean building several packets in parallel


	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->ROUTE1;

		if (Entry->SRTT > Entry->LastRTT)
		{
			if (Entry->LastRTT)		// if zero haven't yet reported +ve info
			{
				if (Entry->LastRTT == 1)	// if 1, probably new, so send alias
					SendRIPToOtherNeighbours(Dest->DEST_CALL, Dest->DEST_ALIAS, Entry);
				else
					SendRIPToOtherNeighbours(Dest->DEST_CALL, 0, Entry);

				Preload = Entry->SRTT /10;
				if (Entry->SRTT < 60000)
					Entry->LastRTT = Entry->SRTT + Preload;	//10% Negative Preload
			}
		}
			
		if (Entry->SRTT >= 60000)
		{
			// It is dead, and we have reported it if necessary, so remove if no NETROM Routes

			if (Dest->NRROUTE1.ROUT_NEIGHBOUR == 0)			// No more Netrom Routes
			{
				char call[11]="";
				ConvFromAX25(Dest->DEST_CALL, call);
				Debugprintf("Deleting Node %s", call);
				memset(Dest, 0, sizeof(struct DEST_LIST));
				NUMBEROFNODES--;
			}
			else
			{
				// Have a NETROM route, so zap the INP3 one

				memset(Entry, 0, sizeof(struct DEST_ROUTE_ENTRY));
			}
		}
	}
}

VOID SendPositiveInfo()
{
	int i;
	struct DEST_LIST * Dest =  DataBase->DESTS;
	struct DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have got significantly better or are newly discovered

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->ROUTE1;

		if (( (Entry->SRTT) && (Entry->LastRTT == 0) )|| 		// if zero haven't yet reported +ve info
			((((Entry->SRTT * 125) /100) < Entry->LastRTT) && // Better by 25%
			((Entry->LastRTT - Entry->SRTT) > 10)))			  // and 100ms
		{
			SendRIPToOtherNeighbours(Dest->DEST_CALL, 0, Entry);
			Dest->ROUTE1.LastRTT = (Dest->ROUTE1.SRTT * 11) / 10;	//10% Negative Preload
		}
	}
}

VOID SendNewInfo()
{
	int i;
	unsigned int NewRTT;
	struct DEST_LIST * Dest =  DataBase->DESTS;
	struct DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have just been added

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->INP3FLAGS & NewNode)
		{
			Dest->INP3FLAGS &= ~NewNode;
			
			Entry = &Dest->ROUTE1;

			SendRIPToOtherNeighbours(Dest->DEST_CALL, Dest->DEST_ALIAS, Entry);

			NewRTT = (Entry->SRTT * 11) / 10;
			Entry->LastRTT = NewRTT;	//10% Negative Preload
		}
	}
}


INP3TIMER()
{
	if (RTTMsg.ID[0] == 0)
		InitialiseRTT();

	// Called once per second

	SendNegativeInfo();					// Urgent

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;
		SendNewInfo();					// Not quite so urgent
		SendRIPTimer();
	}
	else
		RIPTimerCount--;

	if (PosTimerCount == 0)
	{
		PosTimerCount = 300;			// 5 mins
		SendPositiveInfo();
	}
	else
		PosTimerCount--;

	FlushRIFs();

}


#endif

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

