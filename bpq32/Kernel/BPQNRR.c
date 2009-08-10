//
//	Netrom Record ROute Suport Code for BPQ32 Switch
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

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

extern int SENDNETFRAME();
extern VOID Q_ADD();


DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call);
DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall);
VOID __cdecl Debugprintf(const char * format, ...);

struct TRANSPORTENTRY * NRRSession;


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
