/*
Copyright 2001-2022 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/	

//
//	INP3 Suport Code for BPQ32 Switch
//

//	All code runs from the BPQ32 Received or Timer Routines under Semaphore.


#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#include "cheaders.h"

#include "time.h"
#include "stdio.h"
#include <fcntl.h>					 
//#include "vmm.h"

extern int DEBUGINP3;

int NegativePercent = 120;			// if time is 10% worse send negative info
int PositivePercent = 80;			// if time is 20% better send positive info
int NegativeDelay = 10;				// Seconds between checks for negative info - should be quite shourt
int PositiveDelay = 300;

time_t SENDRIFTIME = 0;
int RIFInterval = 3600;

VOID SendNegativeInfo();
VOID SortRoutes(struct DEST_LIST * Dest);
VOID SendRTTMsg(struct ROUTE * Route);
VOID TCPNETROMSend(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame);
void NETROMCloseTCP(struct ROUTE * Route);
VOID UpdateTTforRoute(struct ROUTE * Route, int TTChange);


static VOID SendNetFrame(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Frame)
{
	// INP3 should only ever send over an active link, so just queue the message

	if (Route->TCPPort)			// NETROM over TCP
	{
		TCPNETROMSend(Route, Frame);
		ReleaseBuffer(Frame);
		return;
	}

	if (Route->NEIGHBOUR_LINK)
		C_Q_ADD(&Route->NEIGHBOUR_LINK->TX_Q, Frame);
	else
		ReleaseBuffer(Frame);
}


typedef struct _RTTMSG
{
	UCHAR ID[6];
	UCHAR Space1;
	UCHAR TXTIME[10];
	UCHAR Space2;
	UCHAR SMOOTHEDRTT[10];
	UCHAR Space3;
	UCHAR LASTRTT[10];
	UCHAR Space4;
	UCHAR RTTID[10];
	UCHAR Space5;
	UCHAR ALIAS[7];
	UCHAR VERSION[12];
	UCHAR SWVERSION[9];
	UCHAR FLAGS[20];
	UCHAR PADDING[137];

} RTTMSG;

int COUNTNODES(struct ROUTE * ROUTE);

VOID __cdecl Debugprintf(const char * format, ...);

VOID SendINP3RIF(struct ROUTE * Route, UCHAR * Call, UCHAR * Alias, int Hops, int RTT);
VOID SendOurRIF(struct ROUTE * Route);
VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, int  hops, int rtt, unsigned char * Options);
VOID UpdateRoute(struct DEST_LIST * Dest, struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt);
VOID KillRoute(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR);
VOID AddHere(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt);
VOID SendRIFToNewNeighbour(struct ROUTE * Route);
VOID DecayNETROMRoutes(struct ROUTE * Route);
VOID DeleteINP3Routes(struct ROUTE * Route);
BOOL L2SETUPCROSSLINKEX(PROUTE ROUTE, int Retries);

//#define NOINP3

struct _RTTMSG RTTMsg = {""};

//struct ROUTE DummyRoute = {"","",""};

int RIPTimerCount = 10;				// 1 sec to 10 sec counter
int PosTimerCount = 0;
int NegTimerCount = 0;

// Timer Runs every 10 Secs

extern int MAXRTT;			// 90 secs
extern int MaxHops;

extern int RTTInterval;			// 4 Minutes
int RTTRetries = 2;
int RTTTimeout = 6;				// 1 Min (Horizon is 1 min)

uint32_t RTTID = 1;

// BPQ32001 - Original. Sends RIF in mS and adds link rtt before sending
// BPQ32002 - Sends RIF in 10mS and adds link rtt before sending
// BPQ32003 - Original. RIF in 10mS and doesn't add link rtt before sending (XR compatiblity)
// BPQ32004 - Original. RIF in 10mS ,doesn't add link rtt before sending and sends full routing table every hour



VOID InitialiseRTT()
{
	UCHAR temp[256] = "";

	SENDRIFTIME = NOW;

	memset(&RTTMsg, ' ', sizeof(struct _RTTMSG));
	memcpy(RTTMsg.ID, "L3RTT: ", 7);
	memcpy(RTTMsg.VERSION, "LEVEL3_V2.1 ", 12);
	memcpy(RTTMsg.SWVERSION, "BPQ32004 ", 9);				// Follows XR by not adding route time before sending RIF, send Hourly RIF refresh and time out routes
	_snprintf(temp, sizeof(temp), "$M%d $N $H%d            ", MAXRTT, MaxHops); // trailing spaces extend to ensure padding if the length of characters for MAXRTT changes.
	memcpy(RTTMsg.FLAGS, temp, 20);                 // But still limit the actual characters copied.
	memcpy(RTTMsg.ALIAS, &MYALIASTEXT, 6);
	RTTMsg.ALIAS[6] = ' ';
}

VOID TellINP3LinkGone(struct ROUTE * Route)
{
	struct DEST_LIST * Dest = DESTS;
	char call[11]="";

	ConvFromAX25(Route->NEIGHBOUR_CALL, call);
	Debugprintf("BPQ32 L2 Link to Neighbour %s lost", call);

	if (Route->NEIGHBOUR_LINK)
		Debugprintf("BPQ32 Neighbour_Link not cleared");

	// Link can have both NETROM and INP3 links

//	if (Route->INP3Node == 0)
		DecayNETROMRoutes(Route);
//	else
		DeleteINP3Routes(Route);
}

VOID DeleteINP3Routes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest = DESTS;
	char Call1[10];
	char Call2[10];

	Call1[ConvFromAX25(Route->NEIGHBOUR_CALL, Call1)] = 0;

	if (DEBUGINP3) Debugprintf("Deleting INP3 routes via %s", Call1);

	// Delete any INP3 Dest entries via this Route

	Route->SRTT = 0;
	Route->RTT = 0;
	Route->BCTimer = 0;
	Route->Status = 0;
	Route->Timeout = 0;
	Route->NeighbourSRTT = 0;
	Route->localport = 0;
	Dest--;

	// Delete any Dest entries via this Route 

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry

		if (Dest->NRROUTE[0].ROUT_OBSCOUNT >= 128)	 // Not if locked
			continue;

		Call2[ConvFromAX25(Dest->DEST_CALL, Call2)] = 0;

		if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == Route)
		{
			//	We are deleting the best INP3 route, so need to tell other nodes
			//	We need to keep the entry with a 60000 rtt so
			//	we can send it. Remove when all gone 

			//	How do we indicate is is dead - Maybe the 60000 is enough!

			// If we are cleaning up after a sabm on an existing link (frmr or other end reloaded) then we don't need to tell anyone - the routes should be reestablished very quickly

			if (DEBUGINP3) Debugprintf("Deleting First INP3 Route to %s", Call2);

			Dest->INP3ROUTE[0].STT = 60000;		// leave hops so we can check if we need to send

			if (DEBUGINP3) Debugprintf("Was the only INP3 route");

			if (Dest->DEST_ROUTE == 4)			// we were using it
				Dest->DEST_ROUTE = 0;

			continue;
		}

		// If we aren't removing the best, we don't need to tell anyone.
		
		if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == Route)
		{
			if (DEBUGINP3) Debugprintf("Deleting 2nd INP3 Route to %s", Call2);
			memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
			memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));

			continue;
		}

		if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == Route)
		{
			if (DEBUGINP3) Debugprintf("Deleting 3rd INP3 Route to %s", Call2);
			memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
			continue;
		}
	}

	// I think we should send Negative info immediately

	NegTimerCount = NegativeDelay;
	SendNegativeInfo();
}

VOID DecayNETROMRoutes(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest = DESTS;

	Dest--;

	// Decay any NETROM Dest entries via this Route. If OBS reaches zero, remove

	// OBSINIT is probably too many retries. Try decrementing by 2.

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)
			continue;										// Spare Entry

		if (Dest->NRROUTE[0].ROUT_NEIGHBOUR == Route)
		{
			if (Dest->NRROUTE[0].ROUT_OBSCOUNT && Dest->NRROUTE[0].ROUT_OBSCOUNT < 128)	 // Not if locked
			{
				Dest->NRROUTE[0].ROUT_OBSCOUNT--;
				if (Dest->NRROUTE[0].ROUT_OBSCOUNT)
					Dest->NRROUTE[0].ROUT_OBSCOUNT--;

			}
			if (Dest->NRROUTE[0].ROUT_OBSCOUNT == 0)
			{
				// Route expired

				if (Dest->NRROUTE[1].ROUT_NEIGHBOUR == 0)			// No more Netrom Routes
				{
					if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == 0)			// Any INP3 ROutes?
					{
						// No More Routes - ZAP Dest

						REMOVENODE(Dest);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry	
						continue;
					}
					else
					{
						// Still have an INP3 Route - just zap this entry

						memset(&Dest->NRROUTE[0], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
						continue;

					}
				}

				memcpy(&Dest->NRROUTE[0], &Dest->NRROUTE[1], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memcpy(&Dest->NRROUTE[1], &Dest->NRROUTE[2], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

				continue;
			}
		}
		
		if (Dest->NRROUTE[1].ROUT_NEIGHBOUR == Route)
		{
			Dest->NRROUTE[1].ROUT_OBSCOUNT--;

			if (Dest->NRROUTE[1].ROUT_OBSCOUNT == 0)
			{
				memcpy(&Dest->NRROUTE[1], &Dest->NRROUTE[2], sizeof(struct NR_DEST_ROUTE_ENTRY));
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));

				continue;
			}
		}

		if (Dest->NRROUTE[2].ROUT_NEIGHBOUR == Route)
		{
			Dest->NRROUTE[2].ROUT_OBSCOUNT--;

			if (Dest->NRROUTE[2].ROUT_OBSCOUNT == 0)
			{
				memset(&Dest->NRROUTE[2], 0, sizeof(struct NR_DEST_ROUTE_ENTRY));
				continue;
			}
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
		DecayNETROMRoutes(Route);
	else
		DeleteINP3Routes(Route);
}

VOID ProcessRTTReply(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Buff)
{
	uint32_t RTT;
	uint32_t OrigTime;
	int32_t TTChange;		// Old SRTT

	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Route->Timeout = 0;			// Got Response
	
	sscanf(&Buff->L4DATA[6], "%u", &OrigTime);
	RTT = GetTickCountINP3() - OrigTime;

	if (RTT > 60000)
		return;					// Ignore if more than 60 secs (why ??)

	if (Route->NPR)
	{
		Route->RTTIncrement = Route->SRTT = Route->RTT = Route->NPR;
		if (DEBUGINP3) Debugprintf("INP3 RTT reply from %s - NPR Set RTT was %d setting to %d", Normcall, RTT, Route->NPR);
	}
	else
	{
		if (RTT == 0)
			RTT = 1;				// Don't allow a Node TT of zero

		if (DEBUGINP3) Debugprintf("INP3 RTT reply from %s - SRTT was %d, Current RTT %d", Normcall, Route->SRTT, RTT);

		Route->RTT = RTT;

		if (Route->SRTT == 0)
			Route->SRTT = RTT;
		else
			Route->SRTT = ((Route->SRTT * 80)/100) + ((RTT * 20)/100);

		Route->RTTIncrement = Route->SRTT / 2;		// Half for one way time.

		if (Route->RTTIncrement == 0)
			Route->RTTIncrement = 1;
	}

	if (Route->senderaddsRTT)
		Route->TXRTTIncrement = Route->RTTIncrement;
	else
		Route->TXRTTIncrement = 0;


	if ((Route->Status & GotRTTResponse) == 0)
	{
		// Link is just starting

		if (DEBUGINP3) Debugprintf("INP3 got first RTT reply from %s - Link is (Re)starting", Normcall);

		Route->Status |= GotRTTResponse;
		Route->STTAtLastChange = Route->RTTIncrement;
	}
	else
	{
		// if significant change update dests via this route

		TTChange = Route->RTTIncrement - Route->STTAtLastChange;	// Change since last reported in 10mS units

		if (TTChange > 10 || TTChange < - 10)
		{
			if (DEBUGINP3) Debugprintf("INP3 Significant change to RTT by %s %d - updating routes", Normcall, TTChange);
			UpdateTTforRoute(Route, TTChange);
			Route->STTAtLastChange = Route->RTTIncrement;
		}
	}
}

UCHAR * SkipOptions(UCHAR * ptr1, int msglen);
void DecodeRIFOptions(struct DEST_LIST * Dest, UCHAR * Options);

VOID ProcessINP3RIF(struct ROUTE * Route, UCHAR * ptr1, int msglen, int Port)
{
	unsigned char axcall[7];
	int hops;
	unsigned short rtt;
	char alias[6];
	UINT Stamp, HH, MM;
	char Normcall[10];
	unsigned char * Options;
	int i;
	UCHAR * oldptr1;

	if (Route == 0 || Route->NEIGHBOUR_LINK == 0 || Route->NEIGHBOUR_LINK->LINKCALL == 0)
		return;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;
	if (DEBUGINP3) Debugprintf("Processing RIF from %s INP3Node %d Route SRTT %d", Normcall, Route->INP3Node, Route->SRTT);

	if (Route->SRTT == 0)
		if (DEBUGINP3) Debugprintf("INP3 Zero SRTT");


#ifdef NOINP3

	return;

#endif

	if (Route->INP3Node == 0)
		return;						// We don't want to use INP3

	// Update Timestamp on Route

	Stamp = NOW % 86400;		// Secs into day
	HH = Stamp / 3600;

	Stamp -= HH * 3600;
	MM = Stamp  / 60;

	Route->NEIGHBOUR_TIME = 256 * HH + MM;

	while (msglen > 0)
	{
		if (msglen < 10)
		{
			if (DEBUGINP3) Debugprintf("Corrupt INP3 Message");
			return;
		}

		memset(alias, ' ', 6);	
		memcpy(axcall, ptr1, 7);

		for (i = 0; i < 6; i++)
		{
			if (axcall[i] < 0x40 || (axcall[i] & 1))		// Not valid ax25 callsign
			return;					// Corrupt RIF
		}

		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		// rtt is value from remote node. Add our RTT to that node

		// if other end is old bpq then value is mS otherwise 10 mS unita

		if (Route->mSRIF == 1)
			rtt /= 10;

		if (Route->senderaddsRTT == 0)
			rtt += Route->RTTIncrement;

		msglen -= 10;

		Options = ptr1;

		oldptr1 = ptr1;
		
		ptr1 = SkipOptions(ptr1, msglen);	// We now extact options here and decode them once we have a Node record

		if (ptr1 == 0)
			return;					// Corrupt RIF

		msglen -= (ptr1 - oldptr1);	// options len

		ptr1++;
		msglen--;		// EOP

		UpdateNode(Route, axcall, hops, rtt, Options);

	}

	return;
}

VOID KillRoute(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR)
{
}


VOID UpdateNode(struct ROUTE * Route, UCHAR * axcall, int  hops, int rtt, unsigned char * Options)
{
	struct DEST_LIST * Dest;
	struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR;
	int i;
	char call[11]="";
	APPLCALLS * APPL;
	int App;
	XROptions noOptions;
	int n = sizeof(noOptions);


//	SEE IF any of OUR CALLs - DONT WANT TO PUT IT IN LIST!

	if (CompareCalls(axcall, MYCALL))
	{
		if (DEBUGINP3) Debugprintf("INP3 RIF for our Nodecall - discarding");
		return;
	}
	if (CompareCalls(axcall, NETROMCALL))
	{
		if (DEBUGINP3) Debugprintf("INP3 RIF for our NETROMCALL - discarding");
		return;
	}


	if (CheckExcludeList(axcall) == 0)
	{
		if (DEBUGINP3) Debugprintf("INP3 excluded - discarding");
		return;
	}
	
	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (CompareCalls(axcall, APPL->APPLCALL))
		{
			if (DEBUGINP3) Debugprintf("INP3 RIF for an APPLCALL - discarding");
			return;
		}
	}

	ConvFromAX25(axcall, call);

	// We need to detect unreachable here 

	if (rtt >= 60000 || hops > 30)	// I use 255, Paula uses 31 hops for unreachable 
	{
		// node is unreachable. I need propagate it to other neighbours.

		if (DEBUGINP3) Debugprintf("INP3 Node %s is unreachable via", call);

		if (FindDestination(axcall, &Dest))
		{
			if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == Route)		// Best route
			{
				Dest->INP3ROUTE[0].STT = 60000;		// Will be removed once reported. leave hops so we can check if we need to send

				if (Dest->DEST_ROUTE == 4)			// we were using it
					Dest->DEST_ROUTE = 0;

				NegTimerCount = 0;			// Send negative info asap
				return;
			}

			if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == Route)
			{
				if (DEBUGINP3) Debugprintf("Deleting 2nd INP3 Route to %s", call);
				memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));

				return;
			}

			if (Dest->INP3ROUTE[2].ROUT_NEIGHBOUR == Route)
			{
				if (DEBUGINP3) Debugprintf("Deleting 3rd INP3 Route to %s", call);
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
				return;
			}
		}

		// Not found or not in table - ignore

		return;
	}

	if (hops > MaxHops)
	{
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - Hop Count too high", call, hops, rtt);
		return;
	}

	if (rtt > MAXRTT)
	{
		if (DEBUGINP3) Debugprintf("INP3 Node %s Hops %d RTT %d Ignored - rtt too high", call, hops, rtt);
		return;
	}

	memset(&noOptions, 0, sizeof(noOptions));

	if (FindDestination(axcall, &Dest))
		goto Found;

	if (Dest == NULL)
	{
		if (DEBUGINP3) Debugprintf("INP3 Table Full - discarding");
		return;	// Table Full
	}
	
	// Adding New Node

	memset(Dest, 0, sizeof(struct DEST_LIST));

	memcpy(Dest->DEST_CALL, axcall, 7);

//	Set up First Route

	Dest->INP3ROUTE[0].Hops = hops;
	Dest->INP3ROUTE[0].STT = rtt;
	Dest->INP3ROUTE[0].LastRefreshed = NOW;

	Dest->INP3FLAGS = NewNode;

	Dest->INP3ROUTE[0].ROUT_NEIGHBOUR = Route;

	NUMBEROFNODES++;

	ConvFromAX25(Dest->DEST_CALL, call);
	if (DEBUGINP3) Debugprintf("INP3 Adding New Node %s Hops %d RTT %d", call, hops, rtt);

	DecodeRIFOptions(Dest, Options);

//	if (memcmp(Options, &noOptions, sizeof(noOptions)) != 0)
//	{
//		Dest->XROptions = malloc(sizeof(noOptions));
//		memcpy(Dest->XROptions, Options, sizeof(noOptions));
//		Options->LOC = Options->QTH = Options->Ver = 0;			// So not freed later
//	}

	return;

Found:

	if (Dest->DEST_STATE & 0x80)	// Application Entry
	{
		if (DEBUGINP3) Debugprintf("INP3 Application Entry - discarding");
		return;	
	}

	// Update ALIAS

	ConvFromAX25(Dest->DEST_CALL, call);
	if (DEBUGINP3) Debugprintf("INP3 Updating Node %s Hops %d TT %d", call, hops, rtt);

	DecodeRIFOptions(Dest, Options);

	// See if we are known to it, it not add

	ROUTEPTR = &Dest->INP3ROUTE[0];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[0] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[1];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[1] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	ROUTEPTR = &Dest->INP3ROUTE[2];

	if (ROUTEPTR->ROUT_NEIGHBOUR == Route)
	{
		if (DEBUGINP3) Debugprintf("INP3 Already have as route[2] - TT was %d updating to %d", ROUTEPTR->STT, rtt);
		UpdateRoute(Dest, ROUTEPTR, hops, rtt);
		return;
	}

	// Not in list. If any spare, add.
	// If full, see if this is better

	for (i = 0; i < 3; i++)
	{
		ROUTEPTR = &Dest->INP3ROUTE[i];
		
		if (ROUTEPTR->ROUT_NEIGHBOUR == NULL)
		{
			// Add here

			if (DEBUGINP3) Debugprintf("INP3 adding as route[%d]", i);
			AddHere(ROUTEPTR, Route, hops, rtt);
			if (i == 0)
				Dest->LastTT = 0;
			SortRoutes(Dest);
			return;
		}
	}

	if (DEBUGINP3) Debugprintf("INP3 All entries in use - see if this is better than existing");

	// Full, see if this is better

	// Note that wont replace any netrom routes with INP3 ones unless we add pseudo rtt values to netrom entries

	if (Dest->INP3ROUTE[0].STT > rtt)
	{
		// We are better. Move others down and add on front

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 0");

		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[0], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[0], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[1].STT > rtt)
	{
		// We are better. Move  2nd down and add

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 1");
		memcpy(&Dest->INP3ROUTE[2], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
		AddHere(&Dest->INP3ROUTE[1], Route, hops, rtt);
		return;
	}

	if (Dest->INP3ROUTE[2].STT > rtt)
	{
		// We are better. Add here

		if (DEBUGINP3) Debugprintf("INP3 Replacing route 2");
		AddHere(&Dest->INP3ROUTE[2], Route, hops, rtt);
		return;
	}


	if (DEBUGINP3) Debugprintf("INP3 Worse that any existing route");


	// Worse than any - ignore

}

VOID AddHere(struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR,struct ROUTE * Route , int  hops, int rtt)
{
	ROUTEPTR->Hops = hops;
	ROUTEPTR->STT = rtt;
	ROUTEPTR->ROUT_NEIGHBOUR = Route;
	ROUTEPTR->LastRefreshed = NOW;
	return;
}

	
struct INP3_DEST_ROUTE_ENTRY Temp;


VOID SortRoutes(struct DEST_LIST * Dest)
{
	 char Call1[10], Call2[10], Call3[10];
	 struct INP3_DEST_ROUTE_ENTRY * E0 = &Dest->INP3ROUTE[0];
	 struct INP3_DEST_ROUTE_ENTRY * E1 = &Dest->INP3ROUTE[1];
	 struct INP3_DEST_ROUTE_ENTRY * E2 = &Dest->INP3ROUTE[2];

	 // force route re-evaluation

	 Dest->DEST_ROUTE = 0;

	// May now be out of order

	if (E1->ROUT_NEIGHBOUR == 0)
	{
		if (DEBUGINP3)
		{
			Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
			Debugprintf("INP3 1 route %d %s",  E0->STT, Call1);
		}
		return;						// Only One, so can't be out of order
	}

	if (E2->ROUT_NEIGHBOUR == 0)
	{
		// Only 2

		if (DEBUGINP3) 
		{
			Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
			Call2[ConvFromAX25(E1->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
			Debugprintf("INP3 2 routes %d %s %d %s",  E0->STT, Call1, E1->STT, Call2);
		}

		if (E0->STT < E1->STT  || (E0->STT == E1->STT && E0->Hops <= E1->Hops))
			return;

		// Swap one and two

		memcpy(&Temp, E0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E0, E1, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E1, &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));

		if (DEBUGINP3) Debugprintf("INP3 2 routes %d %s %d %s",  E0->STT, Call2, E1->STT, Call1);

		return;
	}

	// Have 3 Entries


	if (DEBUGINP3) 
	{
		Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(E1->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
		Call3[ConvFromAX25(E2->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;
		Debugprintf("INP3 3 routes %d %s %d %s %d %s",  E0->STT, Call1, E1->STT, Call2, E2->STT, Call3);
	}

	// In order?

	if ((E0->STT < E1->STT  || (E0->STT == E1->STT && E0->Hops <= E1->Hops)) && (E1->STT < E2->STT  || (E1->STT == E2->STT && E1->Hops <= E2->Hops)))
		return;

	// If second is better that first swap

	if (E0->STT > E1->STT || (E0->STT == E1->STT && E0->Hops > E1->Hops))
	{
		memcpy(&Temp, E0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E0, E1, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E1, &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(E1->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(E2->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  E0->STT, Call1, E1->STT, Call2, E2->STT, Call3);

	// if 3 is better than 2 swap them. As two is worse than one. three will then be worst

	if (E1->STT > E2->STT || (E1->STT == E2->STT && E1->Hops > E2->Hops))
	{
		memcpy(&Temp, E1, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E1, E2, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E2, &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}


	Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
	Call2[ConvFromAX25(E1->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
	Call3[ConvFromAX25(E2->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 3 routes %d %s %d %s %d %s",  E0->STT, Call1, E1->STT, Call2, E2->STT, Call3);

	// 3 is now slowest. 2 could still be better than 1

	if (E0->STT > E1->STT || (E0->STT == E1->STT && E0->Hops > E1->Hops))
	{
		memcpy(&Temp, E0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E0, E1, sizeof(struct INP3_DEST_ROUTE_ENTRY));
		memcpy(E1, &Temp, sizeof(struct INP3_DEST_ROUTE_ENTRY));
	}

	if (DEBUGINP3)
	{
		E0 = &Dest->INP3ROUTE[0];
		E1 = &Dest->INP3ROUTE[1];
		E2 = &Dest->INP3ROUTE[2];

		Call1[ConvFromAX25(E0->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call1)] = 0;
		Call2[ConvFromAX25(E1->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;
		Call3[ConvFromAX25(E2->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call3)] = 0;

		Debugprintf("INP3 3 routes %d %s %d %s %d %s",  E0->STT, Call1, E1->STT, Call2, E2->STT, Call3);
	}

	if (E0->STT <= E1->STT && E1->STT <= E2->STT)// In order?
		return;

	// Something went wrong

	if (DEBUGINP3) Debugprintf("INP3 Sort Failed");

}

VOID UpdateTTforRoute(struct ROUTE * Route, int TTChange)
{
	// Look for any Nodes with INP3 routes via Route and adjust STT. Called when an RTT messages detects a change in RTT to Route

	struct DEST_LIST * Dest = DESTS;
	int i, n;

	for (i = 0; i < MAXDESTS; i++)
	{
		for (n = 0; n < 3; n++)
		{
			if (Dest->INP3ROUTE[n].ROUT_NEIGHBOUR == Route)
			{
				int newTT = Dest->INP3ROUTE[n].STT +TTChange;

				if (newTT > 0)
				{
					Dest->INP3ROUTE[n].STT = newTT; 
					SortRoutes(Dest);
				}
				break;
			}
		}
		Dest++;
	}
}



VOID UpdateRoute(struct DEST_LIST * Dest, struct INP3_DEST_ROUTE_ENTRY * ROUTEPTR, int  hops, int rtt)
{
	if (ROUTEPTR->Hops == 0)
	{
		// This is not a INP3 Route - Convert it

		ROUTEPTR->Hops = hops;
		ROUTEPTR->STT = rtt;
		ROUTEPTR->LastRefreshed = NOW;

		SortRoutes(Dest);
		return;
	}

	if (rtt == 60000)
	{
		ROUTEPTR->STT = rtt;
		ROUTEPTR->Hops = hops;
		ROUTEPTR->LastRefreshed = NOW;

		SortRoutes(Dest);
		return;

	}

	ROUTEPTR->STT = rtt;
	ROUTEPTR->Hops = hops;
	ROUTEPTR->LastRefreshed = NOW;
	
	SortRoutes(Dest);
	return;
}

VOID ProcessRTTMsg(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Buff, int Len, int Port)
{
	uint32_t OtherRTT;
	uint32_t Dummy;
	char * ptr;
	struct _RTTMSG * RTTMsg = (struct _RTTMSG *)&Buff->L4DATA[0];
	char Normcall[10];

	if (Route->NEIGHBOUR_LINK == 0)
		return;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;

	// See if a reply to our message, or a new request

	if (memcmp(Buff->L3SRCE, MYCALL,7) == 0)
	{
		ProcessRTTReply(Route, Buff);
		ReleaseBuffer(Buff);
		return;
	}

	// Check TTL

	if (Buff->L3TTL < 2)
	{
		ReleaseBuffer(Buff);
		return;
	}

	Buff->L3TTL--;

	if (Route->NEIGHBOUR_LINK->LINKPORT && (Route->NEIGHBOUR_LINK->LINKPORT->ALLOWINP3 || Route->NEIGHBOUR_LINK->LINKPORT->ENABLEINP3))
		Route->INP3Node = 1;

	if (Route->INP3Node == 0)
	{
		if (DEBUGINP3) Debugprintf("Ignoring RTT Msg from %s - not using INP3", Normcall);
		ReleaseBuffer(Buff);
		return;						// We don't want to use INP3
	}

	// Basic Validation - look for spaces in the right place

	if ((RTTMsg->Space1 | RTTMsg->Space2 | RTTMsg->Space3 | RTTMsg->Space4 | RTTMsg->Space5) != ' ')
	{
		Debugprintf("Corrupt INP3 RTT Message %s", &Buff->L4DATA[0]);
	}
	else
	{
		// Extract other end's SRTT

		// Get SWVERSION to see if other end is old (Buggy) BPQ (000) or not updating RIF before sending (001)

		int inpVer = 0;			// Not BPQ
		
		if (memcmp(RTTMsg->SWVERSION, "BPQ32", 5) == 0)
		{
			inpVer = atoi(&RTTMsg->SWVERSION[5]);
		
			if (inpVer == 1)
			{	
				Route->mSRIF = 1;				// Set if other end is BPQ sending RIF in mS (Ver < 2)
				Route->senderaddsRTT = 1;		// Set if link RTT added before sending (Ver = 1)
				Route->timeoutRoutes = 0;		// Set if other end always sends timed RIF updates (Hourly)
			}
			else if (inpVer == 2)
			{	
				Route->mSRIF = 0;				// Set if other end is BPQ sending RIF in mS (Ver < 2)
				Route->senderaddsRTT = 1;		// Set if link RTT added before sending (Ver = 1)
				Route->timeoutRoutes = 0;		// Set if other end always sends timed RIF updates (Hourly)
			}
			else if (inpVer == 3)
			{	
				Route->mSRIF = 0;				// Set if other end is BPQ sending RIF in mS (Ver < 2)
				Route->senderaddsRTT = 0;		// Set if link RTT added before sending (Ver = 1)
				Route->timeoutRoutes = 0;		// Set if other end always sends timed RIF updates (Hourly)
			}
			else if (inpVer >= 4)
			{	
				Route->mSRIF = 0;				// Set if other end is BPQ sending RIF in mS (Ver < 2)
				Route->senderaddsRTT = 0;		// Set if link RTT added before sending (Ver = 1)
				Route->timeoutRoutes = 1;		// Set if other end always sends timed RIF updates (Hourly)
			}
		}
		else
		{
			// XR or others

			Route->mSRIF = 0;				// Set if other end is BPQ sending RIF in mS (Ver < 2)
			Route->senderaddsRTT = 0;		// Set if link RTT added before sending (Ver = 1)
			Route->timeoutRoutes = 1;		// Set if other end always sends timed RIF updates (Hourly)

			Debugprintf(RTTMsg->SWVERSION);
		}

		sscanf(&Buff->L4DATA[6], "%u %u", &Dummy, &OtherRTT);

		if (OtherRTT < 60000)		// Don't save suspect values
			Route->NeighbourSRTT = OtherRTT;

		if (DEBUGINP3) Debugprintf("INP3 RTT Msg from %s remote SRTT %u", Normcall, OtherRTT);

	}

	// Look for $M and $H (MAXRTT MAXHOPS)

	ptr = strstr(RTTMsg->FLAGS, "$M");

	if (ptr)
		Route->RemoteMAXRTT = atoi(ptr + 2);

	ptr = strstr(RTTMsg->FLAGS, "$H");

	if (ptr)
		Route->RemoteMAXHOPS = atoi(ptr + 2);



	// Echo Back to sender

	SendNetFrame(Route, Buff);

	if ((Route->Status & GotRTTRequest) == 0)
	{
		// Link is just starting

		if (DEBUGINP3) Debugprintf("INP3 Processing first RTT frame from %s - link is (re)starting", Normcall);
		Route->Status |= GotRTTRequest;
	}
}


VOID SendRTTMsg(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg;
	char Stamp[50];
	char Normcall[10];
	unsigned char temp[256];
	uint32_t sendTime;
	int n;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Msg = GetBuff();
	if (Msg == 0)
		return;

	Msg->Port = Route->NEIGHBOUR_PORT;
	Msg->L3PID = NRPID;

	memcpy(Msg->L3DEST, L3RTT, 7);
	memcpy(Msg->L3SRCE, MYCALL, 7);
	Msg->L3TTL = 2;
	Msg->L4ID = 0;
	Msg->L4INDEX = 0;
	Msg->L4RXNO = 0;
	Msg->L4TXNO = 0;
	Msg->L4FLAGS = L4INFO;

	// Windows GetTickCount wraps every 54 days or so. INP3 doesn't care, so long as the edge
	// case where timer wraps between sending msg and getting response is ignored 
	// For platform independence use GetTickCountINP3() and map as appropriate

	sendTime = GetTickCountINP3();	// 10mS units

	sprintf(Stamp, "%10u %10d %10d %10d ", sendTime, Route->SRTT, Route->RTT, RTTID++);

	n = strlen(Stamp);

	if (n != 44)
	{
		Debugprintf("Trying to send corrupt RTT message %s", Stamp);
		return;
	}
	
	memcpy(RTTMsg.TXTIME, Stamp, 44);

	// We now allow MAXRTT and MAXHOPS to be reconfigured so should update header each time

	sprintf(temp, "$M%d $N $H%d            ", MAXRTT, MaxHops); // trailing spaces extend to ensure padding if the length of characters for MAXRTT changes.
	memcpy(RTTMsg.FLAGS, temp, 20);                 // But still limit the actual characters copied.

	// Normally we use Version BPQ32004 but if not using the new standard rif timer refresh (Hourly) switch to BPQ32003 which disables node timeout at other end

	if (RIFInterval == 3600)
		memcpy(RTTMsg.SWVERSION, "BPQ32004 ", 9);
	else
		memcpy(RTTMsg.SWVERSION, "BPQ32003 ", 9);

	memcpy(Msg->L4DATA, &RTTMsg, 236);

	Msg->LENGTH = 256 + 1 + MSGHDDRLEN;

	Route->Timeout = RTTTimeout;

	SendNetFrame(Route, Msg);

	if (Route->Status & SentRTTRequest)
	{
		if (DEBUGINP3) Debugprintf("INP3 Sending first RTT Msg to %s", Normcall);
		return;	
	}

	Route->Status |= SentRTTRequest;	
}

VOID SendKeepAlive(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg = GetBuff();

	if (Msg == 0)
		return;

	Msg->L3PID = NRPID;

	memcpy(Msg->L3DEST, L3KEEP, 7);
	memcpy(Msg->L3SRCE, MYCALL, 7);
	Msg->L3TTL = 1;
	Msg->L4ID = 0;
	Msg->L4INDEX = 0;
	Msg->L4RXNO = 0;
	Msg->L4TXNO = 0;
	Msg->L4FLAGS = L4INFO;

//	Msg->L3MSG.L4DATA[0] = 'K';

	Msg->LENGTH = 20 + MSGHDDRLEN + 1;

	SendNetFrame(Route, Msg);
}

int BuildRIF(UCHAR * RIF, UCHAR * Call, UCHAR * Alias, int Hops, int RTT, char * Dest, XROptions * Options)
{
	int AliasLen;
	int RIFLen;
	UCHAR AliasCopy[10] = "";
	UCHAR * ptr;
	char Normcall[10];

	Normcall[ConvFromAX25(Call, Normcall)] = 0;

	if (RTT > 60000) RTT = 60000;	// Dont send more than 60000

	memcpy(&RIF[0], Call, 7);
	RIF[7] = Hops;
	RIF[8] = RTT >> 8;
	RIF[9] = RTT & 0xff;

	if (Options && Options->Optionslist)
	{
		// Will include alias

		int optlen = Options->Optionslist[0];		// includes terminalting null
		memcpy(&RIF[10], &Options->Optionslist[1], optlen);
		RIFLen = 10 + optlen;
	
		if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s:%s %d %d to %s", AliasCopy, Normcall, Hops, RTT, Dest);
		return RIFLen;
	}

	if (Alias)
	{
		// Need to null-terminate Alias
		
		memcpy(AliasCopy, Alias, 6);
		ptr = strchr(AliasCopy, ' ');

		if (ptr)
			*ptr = 0;

		AliasLen = (int)strlen(AliasCopy);

		RIF[10] = AliasLen+2;
		RIF[11] = 0;
		memcpy(&RIF[12], Alias, AliasLen);

		RIF[12+AliasLen] = 0;
		RIFLen = 13 + AliasLen;

		if (DEBUGINP3) Debugprintf("INP3 sending RIF Entry %s %d %d to %s", Normcall, Hops, RTT, Dest);

		return RIFLen;
	}
	
	RIF[10] = 0;
	return 11;
}


VOID SendOurRIF(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg;
	int RIFLen;
	int totLen = 1;
	int App;
	APPLCALLS * APPL;
	int sendTT = Route->TXRTTIncrement;
	char Normcall[10];

	if (sendTT == 0)
		sendTT = 1;//  For no logical reason XR sends our routes at 10mS

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 Sending Our Call and Applcalls to %s ", Normcall);

	if (Route->mSRIF == 1)	// old bpq bug - send mS not 10 mS units
		sendTT *= 10;

	Msg = GetBuff();
	if (Msg == 0)
		return;

	Msg->L3SRCE[0] = 0xff;

	// send a RIF for our Node and all our APPLCalls

	RIFLen = BuildRIF(&Msg->L3SRCE[totLen], MYCALL, MYALIASTEXT, 1, sendTT, Normcall, 0);
	totLen += RIFLen;

	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			RIFLen = BuildRIF(&Msg->L3SRCE[totLen], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, sendTT, Normcall, 0);
			totLen += RIFLen;
		}
	}

	Msg->L3PID = NRPID;
	Msg->LENGTH = totLen + 1 + MSGHDDRLEN;

	SendNetFrame(Route, Msg);
}

int SendRIPTimer()
{
	int count, nodes;
	struct ROUTE * Route = NEIGHBOURS;
	int MaxRoutes = MAXNEIGHBOURS;
	int INP3Delay;
	char Normcall[10];

	for (count=0; count<MaxRoutes; count++)
	{
		if (Route->NEIGHBOUR_CALL[0] != 0)
		{
			if (Route->NoKeepAlive)					// Keepalive Disabled
			{
				Route++;
				continue;
			}
			
			if (Route->NEIGHBOUR_LINK == 0 || Route->NEIGHBOUR_LINK->LINKPORT == 0)
			{
				if (Route->NEIGHBOUR_QUAL == 0)
				{
					Route++;
					continue;						// Qual zero is a locked out route
				}

				// Dont Activate if link has no nodes unless INP3

				if (Route->INP3Node == 0)
				{
					nodes = COUNTNODES(Route);
			
					if (nodes == 0)
					{
						Route++;
						continue;
					}
				}

				if (Route->Stopped)
				{
					Route++;
					continue;
				}

				// Delay more if Locked - they could be retrying for a long time

				if (Route->ConnectionAttempts < 5)
					INP3Delay = 30;
				else
				{
					if ((Route->NEIGHBOUR_FLAG))	 // LOCKED ROUTE
						INP3Delay = 300;
					else
						INP3Delay = 120;
				}

				if (Route->LastConnectAttempt && (REALTIMETICKS - Route->LastConnectAttempt) < INP3Delay) 
				{
					Route++;
					continue;		// Not yet
				}

				// Try to activate link

				Route->ConnectionAttempts++;

				if (Route->INP3Node && ((Route->TCPPort == 0 || strcmp(Route->TCPHost, "0.0.0.0") != 0))) 
				{
					Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
					if (DEBUGINP3) Debugprintf("INP3 Activating link to %s", Normcall);
				}

				L2SETUPCROSSLINKEX(Route, 2);		// Only try SABM/XID twice
				Route->NeighbourSRTT = 0;			// just in case!
				Route->BCTimer = 0;

				Route->LastConnectAttempt = REALTIMETICKS;
				
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

			if (Route->NEIGHBOUR_LINK->KILLTIMER > ((L4LIMIT - 60) * 3))	// IDLETIME - 1 Minute
			{
				SendKeepAlive(Route);
				Route->NEIGHBOUR_LINK->KILLTIMER = 0;		// Keep Open
			}

#ifdef NOINP3

			Route++;
			continue;

#endif
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
							if (DEBUGINP3) Debugprintf("BPQ32 INP3 Neighbour %s Lost (No Response to RTT)", Call);

							DecayNETROMRoutes(Route);
							DeleteINP3Routes(Route);

							Route->Status = 0;	// Down

							// close the link

							if (Route->TCPPort == 0)	// NetromTCP doesn't have a real link
							{
								Route->NEIGHBOUR_LINK->KILLTIMER = 0;
								Route->NEIGHBOUR_LINK->L2TIMER = 1;		// TO FORCE DISC
								Route->NEIGHBOUR_LINK->L2STATE = 4;		// DISCONNECTING
							}
							else
							{
								// but we should reset the TCP connection

								NETROMCloseTCP(Route);
							}
						}

						Route->BCTimer = 5;		// Wait a while before retrying
					}
				}

				if (Route->BCTimer)
				{
					Route->BCTimer--;
				}
				else
				{
					Route->BCTimer = RTTInterval + rand() % 4;
					Route->Retries = RTTRetries;
					
					if (DEBUGINP3)
					{
						Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
						Debugprintf("INP3 Sending RTT Msg to %s BCTimer = %d", Normcall, Route->BCTimer);
					}
					SendRTTMsg(Route);
				}
			}
		}

		Route++;
	}

	return (0);
}

// Create an Empty RIF

struct _L3MESSAGEBUFFER * CreateRIFHeader(struct ROUTE * Route)
{
	struct _L3MESSAGEBUFFER * Msg = GetBuff();
	UCHAR AliasCopy[10] = "";

	if (Msg)
	{
		Msg->LENGTH = 1;
		Msg->L3SRCE[0] = 0xff;

		Msg->L3PID = NRPID;
	}
	return Msg;

}

VOID SendRIF(struct ROUTE * Route, struct _L3MESSAGEBUFFER * Msg)
{
	char Normcall[10];

	Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;

	Msg->LENGTH += MSGHDDRLEN + 1;		// PID

	if (DEBUGINP3) Debugprintf("Sending INP3 RIF length %d to %s", Msg->LENGTH, Normcall);
	SendNetFrame(Route, Msg);
}

VOID SendRIFToOtherNeighbours(struct DEST_LIST * Dest, UCHAR * alias, struct INP3_DEST_ROUTE_ENTRY * Entry, int Negative, int portNum, int includeOptions)
{
	UCHAR * axcall = Dest->DEST_CALL;
	struct ROUTE * Routes = NEIGHBOURS;
	struct _L3MESSAGEBUFFER * Msg;
	int count, MaxRoutes = MAXNEIGHBOURS;
	char NodeCall[10];
	char destCall[10];
	int rifLen;
	unsigned char rif[256];
	int sendHops, sendTT, lastTT, diff;

	// if portNum is set sending a periodic refresh. Just sent to this port

	NodeCall[ConvFromAX25(axcall, NodeCall)] = 0;

	for (count = 0; count < MaxRoutes; count++)
	{
		if (Routes->INP3Node && Routes->Status && Routes != Entry->ROUT_NEIGHBOUR)
		{	
			sendHops = Entry->Hops + 1;
			if (Entry->STT < 60000)
				sendTT = Entry->STT + Routes->TXRTTIncrement;
			else
				sendTT = 60000;

			lastTT = Dest->LastTT;

			destCall[ConvFromAX25(Routes->NEIGHBOUR_CALL, destCall)] = 0;

			if (!portNum)
			{ 
				// as we can now get very short times need to enforce absolute change as well as percentage
				
				diff = sendTT - lastTT;

				if (diff < 0)
					diff = -diff;

				if (diff < 3)			// need more than 20 ms change
				{
					Routes+=1;
					continue;
				}		

				if (Negative)
				{
					// only send if significantly worse

					if (sendTT < (lastTT * NegativePercent) / 100)
					{
						Routes+=1;
						continue;
					}
				}
				else
				{
					// Send if significantly better

					if (sendTT > (lastTT * PositivePercent) / 100)
					{
						Routes+=1;
						continue;
					}
				}

			}

			if (DEBUGINP3) Debugprintf("INP3 SendRIFToOtherNeighbours  need to send %s to %s", NodeCall, destCall);

			// Don't send if Node is the Neighbour we are sending to

			if (memcmp(Routes->NEIGHBOUR_CALL, axcall, 7) == 0)
			{
				if (DEBUGINP3) Debugprintf("INP3 SendRIFToOtherNeighbours Don't send %s to itself", NodeCall);
				Dest->LastTT = sendTT;		// But update or we will keep re-entering
				Routes+=1;
				continue;
			}

			if (portNum && Routes->NEIGHBOUR_PORT != portNum)
			{
				Routes+=1;
				continue;
			}

			if (portNum)
				Routes->Status &= ~SentOurRIF;

			Dest->LastTT = sendTT;

			// send, but only if within their constraints

			// Does it make any sense to send a node with hopcount of say 2 which was received from a node with
			// maxhops 2. The next hop (with hopcount of 3 or above) will get it but won't be able to reply. 

			if ((Routes->RemoteMAXHOPS == 0 || Routes->RemoteMAXHOPS >= sendHops) && 
				(Routes->RemoteMAXRTT == 0 || Routes->RemoteMAXRTT >= sendTT  || sendTT == 60000))
			{
				if (sendTT == 60000)
					sendHops = 31;

				if (DEBUGINP3)
				{
					if (portNum)
						Debugprintf("INP3 %s Timer Refresh Sending to port %d", NodeCall, portNum);
					else
						Debugprintf("INP3 %s Old TT %d New %d  Sufficent change so sending ", NodeCall, lastTT, sendTT);
				}

				Msg = Routes->Msg;

				if (Msg == NULL) 
				{
					if (DEBUGINP3) Debugprintf("INP3 Building RIF to send to %s", destCall);
					Msg = Routes->Msg = CreateRIFHeader(Routes);
				}

				if (Msg)
				{
					if (Routes->mSRIF == 1)	// old bpq bug - send mS not 10 mS units
						sendTT *= 10;

					if (includeOptions)
						rifLen = BuildRIF(rif, axcall, alias, sendHops, sendTT, destCall, Dest->XROptions);
					else
						rifLen = BuildRIF(rif, axcall, alias, sendHops, sendTT, destCall, 0);

					if (Msg->LENGTH + rifLen > 250)
					{
						SendRIF(Routes, Msg);
						Msg = Routes->Msg = CreateRIFHeader(Routes);
					}

					memcpy(&Msg->L3SRCE[Msg->LENGTH], rif, rifLen);
					Msg->LENGTH += rifLen;
				}
			}
		}
		Routes+=1;
	}
}

VOID SendRIFToNewNeighbour(struct ROUTE * Route)
{
	int i;
	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	struct _L3MESSAGEBUFFER * Msg;
	int sendHops, sendTT;
	char Normcall[10];
	int rifLen;
	unsigned char rif[256];

	if (Route->NEIGHBOUR_LINK == 0)		// shouldn't happen but to be safe..
		return;

	Normcall[ConvFromAX25(Route->NEIGHBOUR_LINK->LINKCALL, Normcall)] = 0;
	if (DEBUGINP3) Debugprintf("INP3 Sending Our Table to %s ", Normcall);

	Dest--;

	// Send all entries not via this Neighbour - used when link starts

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR == 0)
			continue;

		if (Route == Entry->ROUT_NEIGHBOUR)
			continue;

		// Best Route not via this neighbour - send, but only if within their constraints

		if (Entry->Hops)	
		{

			sendHops = Entry->Hops + 1;

			sendTT = Entry->STT + Entry->ROUT_NEIGHBOUR->TXRTTIncrement;
			Dest->LastTT = sendTT;

			if ((Route->RemoteMAXHOPS == 0 || Route->RemoteMAXHOPS >= Entry->Hops || Entry->Hops > 30) && 
				(Route->RemoteMAXRTT == 0 || Route->RemoteMAXRTT >= Entry->STT || Entry->STT == 60000))
			{
				Msg = Route->Msg;

				if (Msg == NULL) 
					Msg = Route->Msg = CreateRIFHeader(Route);

				if (Msg == NULL) 
					return;

				if (Route->mSRIF == 1)	// old bpq bug - send mS not 10 mS units
					sendTT *= 10;

				rifLen = BuildRIF(rif, Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT, Normcall, Dest->XROptions);
		
				if (Msg->LENGTH + rifLen > 250)
				{
					SendRIF(Route, Msg);
					Msg = Route->Msg = CreateRIFHeader(Route);
				}
				memcpy(&Msg->L3SRCE[Msg->LENGTH], rif, rifLen);
				Msg->LENGTH += rifLen;
			}
		}
	}
	if (Route->Msg)
	{
		SendRIF(Route, Route->Msg);
		Route->Msg = NULL;
	}
}

VOID FlushRIFs()
{
	struct ROUTE * Route = NEIGHBOURS;
	int count, MaxRoutes = MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		// Make sure we've sent our local calls

		if ((Route->Status & GotRTTRequest) && (Route->Status & GotRTTResponse) && ((Route->Status & SentOurRIF) == 0))
		{	
			Route->Status |= SentOurRIF;
			SendOurRIF(Route);
			SendRIFToNewNeighbour(Route);
		}
		
		if (Route->Msg)
		{
			char Normcall[10];

			Normcall[ConvFromAX25(Route->NEIGHBOUR_CALL, Normcall)] = 0;
			if (DEBUGINP3) Debugprintf("INP3 Flushing RIF to  %s", Normcall); 
			SendRIF(Route, Route->Msg);
			Route->Msg = NULL;
		}
		Route+=1;
	}
}

VOID SendNegativeInfo()
{
	int i;
	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	char call[11]="";

	Dest--;

	// Send RIF for any Dests that have got worse
	
	// ?? Should we send to one Neighbour at a time, or do all in parallel ??

	// The spec says send Negative info as soon as possible so I'll try building them in parallel
	// That will mean building several packets in parallel


	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)		// unused entry
			continue;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR == 0)
			continue;

		SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, TRUE, FALSE, FALSE);
			
		if (Entry->STT >= 60000)
		{
			// It is dead, and we have reported it if necessary, so remove if no NETROM Routes

			// Wrong. We may have other INP3 routes. Move them up. This will delete first if only one

			// I think I need to set lastTT on all routes.

			if (Dest->INP3ROUTE[1].ROUT_NEIGHBOUR == 0)			// No other INP3 routes
			{	
				if (DEBUGINP3) Debugprintf("Was the only INP3 route");
				memset(&Dest->INP3ROUTE[0], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
			}
			else
			{
				memcpy(&Dest->INP3ROUTE[0], &Dest->INP3ROUTE[1], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
				memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
				NegTimerCount = 0;			// Send negative info again asap to send new best
			}

			if (Dest->INP3ROUTE[0].ROUT_NEIGHBOUR == 0 && Dest->NRROUTE[0].ROUT_NEIGHBOUR == 0)		// No INP3 and no Netrom Routes
			{
				char call[11]="";
				ConvFromAX25(Dest->DEST_CALL, call);
				if (DEBUGINP3) Debugprintf("INP3 No INP3 and no Netrom Routes left - Deleting Node %s", call);
				REMOVENODE(Dest);			// Clear buffers, Remove from Sorted Nodes chain, and zap entry	
			}

			if (Dest->DEST_ROUTE == 4)			// we were using it
				Dest->DEST_ROUTE = 0;
		}
	}
}

VOID SendPositiveInfo()
{
	int i;
	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have got significantly better or are newly discovered

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->DEST_CALL[0] == 0)		// unused entry
			continue;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR)
			SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, FALSE, FALSE, FALSE);
	}
}

VOID SendNewInfo()
{
	int i;
	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	Dest--;

	// Send RIF for any Dests that have just been added

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		if (Dest->INP3FLAGS & NewNode)
		{
			char call[10];
			call[ConvFromAX25(Dest->DEST_CALL, call)] = 0;
			if (DEBUGINP3) Debugprintf("INP3 Sending New Node %s", call);
			Dest->INP3FLAGS &= ~NewNode;
			
			Entry = &Dest->INP3ROUTE[0];

			SendRIFToOtherNeighbours(Dest, Dest->DEST_ALIAS, Entry, TRUE, FALSE, TRUE);	// Send as negative so will always be worse than zero
		}
	}
}

// Refresh RIF entries for each route. Shouldn't be necessary, but add for now

int routeCount = 0;
struct ROUTE * Route = NULL;

VOID sendAlltoOneNeigbour(struct ROUTE * Route)
{
	char Call[10];
	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;

	int i;
	struct _L3MESSAGEBUFFER * Msg;
	int sendHops, sendTT, lastTT;
	APPLCALLS * APPL;
	int App;

	Call[ConvFromAX25(Route->NEIGHBOUR_CALL, Call)] = 0;

	if (DEBUGINP3) Debugprintf("INP3 Manual send RIF to %s", Call); 

	// send a RIF for our Node and all our APPLCalls

	Msg = Route->Msg;

	if (Msg == NULL) 
		Msg = Route->Msg = CreateRIFHeader(Route);

	if (Msg == 0)
		return;
				
	if (Route->mSRIF == 1)
		Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], MYCALL, MYALIASTEXT, 1, Route->TXRTTIncrement * 10, Call, 0);
	else
		Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], MYCALL, MYALIASTEXT, 1, 1, Call, 0);

	for (App = 0; App < NumberofAppls; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (APPL->APPLQUAL > 0)
		{
			if (Route->mSRIF == 1)
				Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, Route->TXRTTIncrement * 10, Call, 0);
			else
				Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], APPL->APPLCALL, APPL->APPLALIAS_TEXT, 1, 1, Call, 0);
		}
	}

	// Send all dests that have this route as their best inp3 route

	Dest--;

	for (i=0; i < MAXDESTS; i++)
	{
		Dest++;

		Entry = &Dest->INP3ROUTE[0];

		if (Entry->ROUT_NEIGHBOUR == 0)
			continue;

		if (Entry->ROUT_NEIGHBOUR && Route->INP3Node && Route->Status && Route != Entry->ROUT_NEIGHBOUR)	// Dont send to originator of route
		{
			// as the value sent will be different for each link, we need to check if change is enough here

			// Don't send if Node is the Neighbour we are sending to

			if (memcmp(Route->NEIGHBOUR_CALL, Dest->DEST_CALL, 7) == 0)
			{
				if (DEBUGINP3) Debugprintf("INP3 Timer RIF Don't send %s to itself", Call);
				continue;
			}

			sendHops = Entry->Hops + 1;
			sendTT = Entry->STT + Entry->ROUT_NEIGHBOUR->TXRTTIncrement;
			lastTT = Dest->LastTT;

			Dest->LastTT = sendTT;

			// send, but only if within their constraints

			if ((Route->RemoteMAXHOPS == 0 || Route->RemoteMAXHOPS >= Entry->Hops) &&  (Route->RemoteMAXRTT == 0 || Route->RemoteMAXRTT >= sendTT))
			{	
				Msg = Route->Msg;

				if (Msg == NULL) 
					Msg = Route->Msg = CreateRIFHeader(Route);

				if (Msg)
				{
					if (Route->mSRIF == 1)
						sendTT *= 10;
					
					Msg->LENGTH += BuildRIF(&Msg->L3SRCE[Msg->LENGTH], Dest->DEST_CALL, Dest->DEST_ALIAS, sendHops, sendTT, Call, 0);

					if (Msg->LENGTH > 250 - 15)
					{
						SendRIF(Route, Msg);
						Route->Msg = NULL;
					}
				}
			}
		}
	}

	if (Route->Msg)
	{
		SendRIF(Route, Route->Msg);
		Route->Msg = NULL; 
	}
}


VOID SendAllInfo()
{
	if (routeCount == 0)			// Not sending
	{
		if (RIFInterval == 0 || (NOW - SENDRIFTIME) < RIFInterval)	// Time for new send?
			return;

		Route = NEIGHBOURS;
	}

	// Build RIF

	while (Route->INP3Node == 0)
	{
		Route++;
		routeCount++;

		if (routeCount == MAXNEIGHBOURS)
		{
			//cycle finished

			SENDRIFTIME = NOW;
			routeCount = 0;
			return;
		}
	}

	sendAlltoOneNeigbour(Route);

	Route++;
	routeCount++;

	if (routeCount == MAXNEIGHBOURS)
	{
		//cycle finished

		SENDRIFTIME = NOW;
		routeCount = 0;
	}
}

int INP3NodeTimeout = 3600 *3;			// 3 Hours

void DecayINP3Routes()
{
	// Runs every 10 secs

	// Invalidate routes that haven't been refreshed for a while (if neighbour is sending periodic refresh)

	struct DEST_LIST * Dest = DESTS;
	struct INP3_DEST_ROUTE_ENTRY * Entry;
	int i, n;
	time_t xx;

	for (i=0; i < MAXDESTS; i++)
	{
		if (Dest->DEST_CALL == 0)
		{
			Dest++;
			continue;
		}

		for (n = 0; n < 3; n++)
		{
			Entry = &Dest->INP3ROUTE[n];

			if (Entry->ROUT_NEIGHBOUR == 0)			// Stop on first unused entry
				break;

			xx = NOW - Entry->LastRefreshed;

			if (Entry->ROUT_NEIGHBOUR->timeoutRoutes && Entry->LastRefreshed && (Entry->LastRefreshed + INP3NodeTimeout) < NOW)
			{
				char Call1[10];
				char Call2[10];
				Call1[ConvFromAX25(Dest->DEST_CALL, Call1)] = 0;
				Call2[ConvFromAX25(Entry->ROUT_NEIGHBOUR->NEIGHBOUR_CALL, Call2)] = 0;

				if (DEBUGINP3) Debugprintf("Timer Deleting INP3 Route %d to %s via %s", n + 1, Call1, Call2);

				if (n == 0)
				{
					// if removing best we need to tell others
					
					Entry->STT = 60000;		// leave hops so we can check if we need to send

					if (Dest->DEST_ROUTE == 4)			// we were using it
						Dest->DEST_ROUTE = 0;

				}
				else
				{
					// If we aren't removing the best, we don't need to tell anyone.

					if (n == 1)		// 2nd
					{
						memcpy(&Dest->INP3ROUTE[1], &Dest->INP3ROUTE[2], sizeof(struct INP3_DEST_ROUTE_ENTRY));
						memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
					}
					else
						memset(&Dest->INP3ROUTE[2], 0, sizeof(struct INP3_DEST_ROUTE_ENTRY));
				}
			}
		}
		Dest++;
	}
}

VOID INP3TIMER()
{
	if (RTTMsg.ID[0] == 0)
		InitialiseRTT();

	// Called once per second

#ifdef NOINP3

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;
		SendRIPTimer();
	}
	else
		RIPTimerCount--;

	return;

#endif

	SendNewInfo();					// Need to send to set up last sent time

	if (NegTimerCount == 0)
	{
		NegTimerCount = NegativeDelay;
		SendNegativeInfo();
	}
	else
		NegTimerCount--;

	if (RIPTimerCount == 0)
	{
		RIPTimerCount = 10;
		SendRIPTimer();
		SendAllInfo();					// Timer Driven refresh
		DecayINP3Routes();
	}
	else
		RIPTimerCount--;

	if (PosTimerCount == 0)
	{
		PosTimerCount = PositiveDelay;
		SendPositiveInfo();
	}
	else
		PosTimerCount--;

	FlushRIFs();

}


UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, int msglen)
{
	char call[10];
	int calllen;
	int hops;
	unsigned short rtt;
	unsigned int len;
	unsigned int opcode;
	char alias[10] = "";
	UCHAR IP[6];
	int i;

	ptr2+=sprintf(ptr2, " INP3 RIF:\r Alias  Call  Hops  RTT\r");

	while (msglen > 0)
	{
		calllen = ConvFromAX25(ptr1, call);
		call[calllen] = 0;

		// Validate the call

		for (i = 0; i < calllen; i++)
		{
			if (!isupper(call[i]) && !isdigit(call[i]) && call[i] != '-')
			{
				ptr2+=sprintf(ptr2, " Corrupt RIF Call\r");
				return ptr2;
			}
		}

		ptr1+=7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		IP[0] = 0;
		strcpy(alias, "      ");

		msglen -= 10;

		// Process optional fields


		while (*ptr1 && msglen > 0)			//  Have an option
		{
			len = *ptr1;
			opcode = *(ptr1+1);

			if (len < 2 || len > msglen)
			{
				ptr2+=sprintf(ptr2, " Corrupt RIF Opcode %d Len %d MsgLen %d \r", opcode, len, msglen);
				return ptr2;
			}
			if (opcode == 0 && len < 9)
			{
				memcpy(&alias[6 - (len - 2)], ptr1+2, len-2);		// Right Justiify
			}
			else if (opcode == 1 && len < 8)
			{
				memcpy(IP, ptr1+2, len-2);
			}

			ptr1 += len;
			msglen -= len;
		}

		ptr2+=sprintf(ptr2, " %s:%s %d %4.2d\r", alias, call, hops, rtt);

		ptr1++;
		msglen--;		// Over EOP

	}
	return ptr2;
}

// Paula's conversion of rtt to Quality

int inp3_tt2qual (int tt, int hops) 
{
	int qual;

	if (tt >= 60000 || hops > 30) 
		return(0);
	
	qual = 254 - (tt/20);
	
	if (qual > 254 - hops)
		qual = 254 - hops;
	
	if (qual < 0) 
		qual=0;
	
	return(qual); 
} 


/*

5.2 XRouter Extensions

The following identifiers are used in XRouter:

Type 0x01: AMPRNet IP Routing Data
Value: Exactly 7 bytes total length (Option Length = 0x07).
Contains a 4-byte IPv4 address within the 44.0.0.0/8 amateur
network (in network byte order) followed by a 1-byte packed
hostmask prefix length (e.g., 0x18 for a /24).

Type 0x10: Geographic Position
Value: Exactly 10 bytes total length. Contains two 32-bit signed
Big-Endian integers: Latitude followed by Longitude, expressed in
hundredths of a minute of arc.

Type 0x12: Metadata Timestamp
Value: Exactly 6 bytes total length. Contains a 32-bit unsigned
integer tracking standard Unix epoch time.

Type 0x13: TCP Service Port
Value: Exactly 4 bytes total length. A 16-bit unsigned integer
defining the active TCP port for user terminal access. Used in
conjunction with a Type 0x01 option.

Type 0x14: Timezone Offset
Value: Exactly 4 bytes total length. A 16-bit signed integer
defining the local timezone offset relative to GMT, measured in
minutes (e.g., -60).

Type 0x15: Maidenhead Locator
Value: A variable-length ASCII string representing the station
locator (e.g., IO83VK) with no null termination.

Type 0x16: QTH (Location Description)
Value: A variable-length plain ASCII string describing the
station's physical location (e.g., Niagara Park) with no null
termination.

Type 0x17: Software Version String
Value: A variable-length ASCII string representing the node
application's compile version (e.g., 501w) with no null
termination.

5.3 Detailed Flags Specification (Option 0x11)

Following the length byte (0x05) and type byte (0x11), this option
contains three sequential 1-byte fields:

+------------+------------+---------+---------+---------+
| len=0x05 | typ=0x11 | swtype | flags1 | flags_2 |
+------------+------------+---------+---------+---------+
Figure 4: Structure of the Option 0x11 Payload

5.3.1 Software Type (sw_type)

A 1-byte integer identifying the node software:

0 = Unknown, 1 = BPQ16 (DOS), 2 = XRouter16 (DOS),
3 = XServ16 (DOS), 4 = XRouter32 (Win GUI), 5 = XR32 (Win TUI),
6 = XS32 (Win TUI BBS), 7 = XRLin (Linux x86),
8 = XRouter (Raspberry Pi), 9 = BPQ32 (Windows),
10 = BPQ32 (Linux).

5.3.2 Capability Flags 1 (flags_1)

A 1-byte bitmask detailing supported network layer features:

0x01 = INP3 Capable
0x02 = NetRom L3 routing enabled
0x04 = Capable of Netrom Record Route
0x08 = Capable of Netrom Control Message Protocol
0x10 = Capable of NetRomX (CREQX)
0x20 = Supports NDP (Netrom Datagram Protocol)
0x40 = Node is a GlobalNet router/host
0x80 = Supports IP over NetRom
5.3.3 Capability Flags 2 (flags_2)

A 1-byte bitmask detailing hosted application services:

0x01 = Has general purpose L7 command line
0x02 = Is/Has PMS (Mailbox)
0x04 = Is/Has XR Chat
0x08 = Is/Has BPQ/Roundtable chat
0x10 = Is/Has BBS
0x20 = Is/Has DX Cluster
0x40 = Is/Has RMS gateway
0x80 = Reserved (extension bit)

G8PZT-1:XRLN64} Knoten:
Info for: KIDDER:G8PZT  (HOST) [XRPi]  FR=13203  RTT=0.17  Hop=1 XR {PEER}
  Pos=52.4000N   2.2500W  Loc=IO82VJ  Qth=KIDDERMINSTER, UK
  IP=44.136.16.50/32  TCP=23  v505G  OFF
  Supports: INP3 L3ROUT NRR NCMP L4X NDP
  BBS XRCHAT RTCHAT
  Updated: 02/07 13:40  Confirmed: 01/07 14:49

*/

UCHAR * SkipOptions(UCHAR * ptr1, int msglen)
{
	int len, opcode;

	// Skip options and return pointer to next entry

	while (*ptr1 && msglen > 0)
	{
		len = *ptr1;
		opcode = *(ptr1+1);

		if (len < 2 || len > msglen)
			return 0;				// Duff RIF

		ptr1+=len;
		msglen -=len;
	}

	return ptr1;
}

/*
// See if any XR options to add

	if (memcmp(Options, &noOptions, sizeof(noOptions)) != 0)
	{
		if (Dest->XROptions == 0)		// None yet
			Dest->XROptions = zalloc(sizeof(noOptions));

		// Have options so copy across any new ones

		if (Options->IPADDR)
		{
			Dest->XROptions->IPADDR = Options->IPADDR;
			Dest->XROptions->Mask = Options->Mask;			// Always come togther
		}
		if (Options->Lat)
			Dest->XROptions->Lat = Options->Lat;

		if (Options->Lon)
			Dest->XROptions->Lon = Options->Lon;

		if (Options->Port)
			Dest->XROptions->Port = Options->Port;

		if (Options->Time)
			Dest->XROptions->Time = Options->Time;

		if (Options->TZOffset)
			Dest->XROptions->TZOffset = Options->TZOffset;

		if (Options->LOC)
		{
			if (Dest->XROptions->LOC)
				free(Dest->XROptions->LOC);

			Dest->XROptions->LOC = _strdup(Options->LOC);
		}

		if (Options->QTH)
		{
			if (Dest->XROptions->QTH)
				free(Dest->XROptions->QTH);

			Dest->XROptions->QTH = _strdup(Options->QTH);
		}

		if (Options->Ver)
		{
			if (Dest->XROptions->Ver)
				free(Dest->XROptions->Ver);

			Dest->XROptions->Ver = _strdup(Options->Ver);
		}

		if (Options->SWType | Options->NetworkFlags | Options->ApplFlags)		// All come together
		{
			Dest->XROptions->SWType = Options->SWType;
			Dest->XROptions->NetworkFlags = Options->NetworkFlags;
			Dest->XROptions->ApplFlags = Options->ApplFlags;
		}
	}
*/



void DecodeRIFOptions(struct DEST_LIST * Dest, UCHAR * ptr1)
{
	int len, opcode;

	// Options should be saved and sent out as received, including any unrecongnised ones
	// Do we need to worry about getting different subsets of values at different times,
	// so we need to combine new nodes with any existing ones??

	// We treat alias separately as that is sent by all software

	int rawoptionslen = 0;
	unsigned char * startoptions = ptr1;
	int gotOptions = 0;
	XROptions * Options = Dest->XROptions;


	while (*ptr1)					// Format already checked above
	{
		len = *ptr1;
		opcode = *(ptr1+1);

		if (len < 2)
			return;					// Duff RIF

		if (opcode)
		{
			// Not alias so we need extended options field

			gotOptions = 1;
			
			if (Options == 0)
			{
				Options = Dest->XROptions = zalloc(sizeof(XROptions));
			}
		}

		switch (opcode)
		{
		case 0:			// Alias

			memset(Dest->DEST_ALIAS, ' ', 6);
			
			if (len > 1 && len < 9)
				memcpy(Dest->DEST_ALIAS, ptr1+2, len-2);
			break;
		
		case 1:			// IP Address and Mask

			if (len == 7)
			{
				memcpy(&Options->IPADDR, ptr1+2, 4);
				Options->Mask = (uint8_t)ptr1[6];
			}
			break;

		case 0x10:		// Geographic Position

			if (len == 10)
			{
				uint32_t w1;
				int32_t w2;

				memcpy(&w1, ptr1+2, 4); 
				w2 = htonl(w1);

				Options->Lat = w2 / 6000.0;				// 1/100th minute to degrees

				memcpy(&w1, ptr1+6, 4); 
				w2 = htonl(w1);

				Options->Lon = w2 / 6000.0;
				break;
			}
		

		case 0x11:		// Various flags

			if (len == 5)
			{
				Options->SWType = ptr1[2];
				Options->NetworkFlags = ptr1[3];
				Options->ApplFlags = ptr1[4];
				break;
			}
		
		case 0x12:		// Metadata Timestamp

			if (len == 6)
			{
				uint32_t w1;

				memcpy(&w1, ptr1+2, 4); 
				Options->Time = htonl(w1);
			}
			
			break;
		
		case 0x13:		// TCP Service Port

			if (len == 4)
			{
				uint16_t w1;

				memcpy(&w1, ptr1+2, 2); 
				Options->Port = htons(w1);
			}

		case 0x14:		// Timezone Offset

			if (len == 4)
			{
				uint16_t w1;

				memcpy(&w1, ptr1+2, 2); 
				Options->TZOffset = htons(w1);
			}
			
			break;

		case 0x15:		// Maidenhead Locator
			
			if (Options->LOC)
				free (Options->LOC);

			Options->LOC = zalloc(len);
			memcpy(Options->LOC, ptr1+2, len-2);
			
			break;

		case 0x16:		// QTH (Location Description)

			if (Options->QTH)
				free (Options->QTH);

			Options->QTH = zalloc(len);
			memcpy(Options->QTH, ptr1+2, len-2);

		case 0x17:		//  Software Version String

			if (Options->Ver)
				free (Options->Ver);
			
			Options->Ver = zalloc(len);
			memcpy(Options->Ver, ptr1+2, len-2);

		}

		ptr1+=len;
	}

	// ptr1 points to byte beyond end of options. Save the options string. Put length byte on front


	if (gotOptions == 0)		// Empty
		return;

	if (Options)			// Have other than alias
	{
		rawoptionslen = (ptr1 - startoptions) + 1;		// include terminating null

		if (Options->Optionslist)
		{
			// see if changed (not much use if options include timestamp)

			if (Options->Optionslist[0] == rawoptionslen && memcmp(&Options->Optionslist[1], startoptions, rawoptionslen) == 0)
				return;

			free(Options->Optionslist);
		}

		Options->Optionslist = malloc(rawoptionslen + 1);
		Options->Optionslist[0] = rawoptionslen;
		memcpy(&Options->Optionslist[1], startoptions, rawoptionslen);
	}

	return;

}

