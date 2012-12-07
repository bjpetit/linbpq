

// General C Routines common to bpq32 and linbpq.mainly moved from BPQ32.c

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma data_seg("_BPQDATA")
				

#include "CHeaders.h"
#include "tncinfo.h"

#define MAXDATA BUFFLEN-16

extern struct TNCINFO * TNCInfo[34];		// Records are Malloc'd
extern char * PortConfig[];
extern int Semaphore;
extern UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

void GetSemaphore();
void FreeSemaphore();
Dll VOID APIENTRY Send_AX(UCHAR * Block, DWORD Len, UCHAR Port);
TRANSPORTENTRY * SetupSessionFromHost(PBPQVECSTRUC HOST, UINT ApplMask);
int Check_Timer();
VOID SENDUIMESSAGE(struct DATAMESSAGE * Msg);
DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID APIENTRY md5 (char *arg, unsigned char * checksum);

// Get buffer from Queue


VOID * Q_REM(VOID *PQ)
{
	UINT * Q = (UINT *) PQ;
	UINT  * first,next;

	if (Semaphore == 0)
		Debugprintf("Q_REM called without semaphore");

	first = (UINT *)Q[0];
	
	if (first == 0) return (0);			// Empty
	
	next=first[0];						// Address of next buffer
	
	Q[0]=next;

	return (first);

}


// Return Buffer to Free Queue

UINT ReleaseBuffer(VOID *pBUFF)
{
	UINT * pointer, * BUFF = pBUFF;

	if (Semaphore == 0)
		Debugprintf("ReleaseBuffer called without semaphore");
	
	pointer = (UINT *)FREE_Q;

	*BUFF=(UINT)pointer;

	FREE_Q=(UINT)BUFF;

	QCOUNT++;

	return 0;
}

int C_Q_ADD(VOID *PQ,VOID *PBUFF)
{
	UINT * Q = (UINT *) PQ;
	UINT * BUFF = (UINT *)PBUFF;
	UINT * next;

	if (Semaphore == 0)
		Debugprintf("C_Q_ADD called without semaphore");

	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front

		return(0);
	}

	next = (UINT *)Q[0];

	while (next[0]!=0)
	{
		next=(UINT *)next[0];			// Chain to end of queue
	}
	next[0]=(UINT)BUFF;					// New one on end

	return(0);

}

int C_Q_COUNT(VOID *PQ)
{
	UINT * Q = (UINT *) PQ;

	//	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER 

	int count = 0;

	while (*Q)
	{
		count++;
		Q = (UINT *)*Q;
	}
	
	return count;
}

VOID * GetBuff()
{
	UINT * Temp = Q_REM(&FREE_Q);

	if (Semaphore == 0)
		Debugprintf("GetBuff called without semaphore");

	if (Temp)
	{
		QCOUNT--;

		if (QCOUNT < MINBUFFCOUNT)
			MINBUFFCOUNT = QCOUNT;
	}
	
	return Temp;
}

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

VOID DISPLAYCIRCUIT(TRANSPORTENTRY * L4, char * Buffer)
{
	UCHAR Type = L4->L4CIRCUITTYPE;
	struct PORTCONTROL * PORT;
	struct _LINKTABLE * LINK;
	BPQVECSTRUC * VEC;
	struct DEST_LIST * DEST;

	char Normcall[11] = "";
	char Normcall2[11] = "";
	char Alias[11] = "";

	Buffer[0] = 0;

	switch (Type)
	{
	case PACTOR+UPLINK:

		PORT = L4->L4TARGET.PORT;
		
		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');
		
		sprintf(Buffer, "%s %d/%d(%s)", "TNC Uplink Port", PORT->PORTNUMBER, L4->KAMSESSION, Normcall);

		return;
	
		
	case PACTOR+DOWNLINK:

		PORT = L4->L4TARGET.PORT;
		sprintf(Buffer, "%s %d/%d", "Attached to Port", PORT->PORTNUMBER, L4->KAMSESSION);
		return;


	case L2LINK+UPLINK:

		LINK = L4->L4TARGET.LINK;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		sprintf(Buffer, "%s %d(%s)", "Uplink", LINK->LINKPORT->PORTNUMBER, Normcall);
		return;

/*
	MOV	ESI,L4TARGET[ESI]		; NODE
	MOV	AL,LINKPORT[ESI]
	CALL	CONV_DIGITS

	POP	EAX

	jmp	SHORT CMDS17

	PUBLIC	CMDS11
CMDS11:
*/
	case L2LINK+DOWNLINK:

		LINK = L4->L4TARGET.LINK;
			
		ConvFromAX25(LINK->OURCALL, Normcall);
		strlop(Normcall, ' ');

		ConvFromAX25(LINK->LINKCALL, Normcall2);
		strlop(Normcall2, ' ');

		sprintf(Buffer, "%s %d(%s %s)", "Downlink", LINK->LINKPORT->PORTNUMBER, Normcall, Normcall2);
		return;

	case BPQHOST + UPLINK:
	case BPQHOST + DOWNLINK:

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');
		VEC = L4->L4TARGET.HOST;
		sprintf(Buffer, "%s%02d(%s)", "Host", (VEC - BPQHOSTVECTOR) + 1, Normcall);
		return;

	case SESSION + DOWNLINK:
	case SESSION + UPLINK:
	
		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		DEST = L4->L4TARGET.DEST;

		ConvFromAX25(DEST->DEST_CALL, Normcall2);
		strlop(Normcall2, ' ');

		memcpy(Alias, DEST->DEST_ALIAS, 6);
		strlop(Alias, ' ');

		sprintf(Buffer, "Circuit(%s:%s %s)", Alias, Normcall2, Normcall);

		return;
	}
}

VOID CheckForDetach(struct TNCINFO * TNC, int Stream, struct STREAMINFO * STREAM,
			VOID TidyCloseProc(), VOID ForcedCloseProc(), VOID CloseComplete())
{
	UINT * buffptr;

	if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
	{
		// Node has disconnected - clear any connection

 		if (STREAM->Disconnecting)
		{
			// Already detected the detach, and have started to close

			STREAM->DisconnectingTimeout--;
			
			if (STREAM->DisconnectingTimeout)
				return;							// Give it a bit longer

			// Close has timed out - force a disc, and clear

			ForcedCloseProc(TNC, Stream);		// Send Tidy Disconnect

			goto NotConnected;
		}

		// New Disconnect

		Debugprintf("New Disconnect Port %d Q %x", TNC->Port, STREAM->BPQtoPACTOR_Q);
			
		if (STREAM->Connected || STREAM->Connecting)
		{
			char logmsg[120];	
			time_t Duration;

			// Need to do a tidy close

			STREAM->Disconnecting = TRUE;
			STREAM->DisconnectingTimeout = 300;			// 30 Secs

			if (Stream == 0)
				SetWindowText(TNC->xIDC_TNCSTATE, "Disconnecting");

			// Create a traffic record

			Duration = time(NULL) - STREAM->ConnectTime;
				
			sprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, STREAM->BytesTXed/Duration,
				STREAM->BytesRXed, STREAM->BytesRXed/Duration, Duration);

			Debugprintf(logmsg);

			if (STREAM->BPQtoPACTOR_Q)					// Still data to send?
				return;									// Will close when all acked

//			if (STREAM->FramesOutstanding && TNC->Hardware == H_UZ7HO)
//				return;									// Will close when all acked
			
			TidyCloseProc(TNC, Stream);					// Send Tidy Disconnect

			return;
		}

		// Not connected 
NotConnected:

		STREAM->Disconnecting = FALSE;
		STREAM->Attached = FALSE;
		STREAM->Connecting = FALSE;
		STREAM->Connected = FALSE;

		if (Stream == 0)
			SetWindowText(TNC->xIDC_TNCSTATE, "Free");

		STREAM->FramesQueued = 0;
		STREAM->FramesOutstanding = 0;

		CloseComplete(TNC, Stream);

		while(STREAM->BPQtoPACTOR_Q)
		{
			buffptr=Q_REM(&STREAM->BPQtoPACTOR_Q);
			ReleaseBuffer(buffptr);
		}

		while(STREAM->PACTORtoBPQ_Q)
		{
			buffptr=Q_REM(&STREAM->PACTORtoBPQ_Q);
			ReleaseBuffer(buffptr);
		}
	}
}

char * CheckAppl(struct TNCINFO * TNC, char * Appl)
{
	APPLCALLS * APPL;
	BPQVECSTRUC * PORTVEC;
	int Allocated = 0, Available = 0;
	int App, Stream;
	struct TNCINFO * APPLTNC;

	Debugprintf("Checking if %s is running", Appl);

	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (_memicmp(APPL->APPLCMD, Appl, 12) == 0)
		{
			int _APPLMASK = 1 << App;

			// If App has an alias, assume it is running , unless a CMS alias - then check CMS

			if (APPL->APPLHASALIAS)
			{
				if (APPL->APPLPORT)
				{
					APPLTNC = TNCInfo[APPL->APPLPORT];
					{
						if (APPLTNC)
						{
							if (APPLTNC->TCPInfo && !APPLTNC->TCPInfo->CMSOK)
							return NULL;
						}
					}
				}
				return APPL->APPLCALL_TEXT;
			}

			// See if App is running

			PORTVEC=BPQHOSTVECPTR;

			for (Stream = 0; Stream < 64; Stream++)
			{	
				if (PORTVEC->HOSTAPPLMASK & _APPLMASK)
				{
					Allocated++;

					if (PORTVEC->HOSTSESSION == 0 && (PORTVEC->HOSTFLAGS &3) == 0)
					{
						// Free and no outstanding report
						
						return APPL->APPLCALL_TEXT;		// Running
					}
				}
				PORTVEC++;
			}
		}
	}

	return NULL;			// Not Running
}

VOID SetApplPorts()
{
	// If any appl has an alias, get port number

	struct APPLCONFIG * App;
	APPLCALLS * APPL;

	char C[80];
	char Port[80];
	char Call[80];

	int i, n;

	App = (struct APPLCONFIG *)&ConfigBuffer[ApplOffset];

	for (i=0; i < NumberofAppls; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLHASALIAS)
		{
			n = sscanf(App->CommandAlias, "%s %s %s", &C, &Port, &Call);
			if (n == 3)
				APPL->APPLPORT = atoi(Port);
		}
		App++;
	}
}


struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

BOOL ProcessIncommingConnect(struct TNCINFO * TNC, char * Call, int Stream, BOOL SENDCTEXT)
{
	TRANSPORTENTRY * Session;
	int Index = 0;
	UINT * buffptr;

	// Stop Scanner

	if (Stream == 0 || TNC->Hardware == H_UZ7HO)
	{
		char Msg[80];

		sprintf(Msg, "%d SCANSTOP", TNC->Port);
		
//		Rig_Command(-1, Msg);

//		UpdateMH(TNC, Call, '+', 'I');
	}
	
	Session=L4TABLE;

	// Find a free Circuit Entry

	while (Index < MAXCIRCUITS)
	{
		if (Session->L4USER[0] == 0)
			break;

		Session++;
		Index++;
	}

	if (Index == MAXCIRCUITS)
		return FALSE;					// Tables Full

	memset(Session, 0, sizeof(TRANSPORTENTRY));

	memcpy(TNC->Streams[Stream].RemoteCall, Call, 9);	// Save Text Callsign 

	ConvToAX25(Call, Session->L4USER);
	ConvToAX25(MYNODECALL, Session->L4MYCALL);

	Session->CIRCUITINDEX = Index;
	Session->CIRCUITID = NEXTID;
	NEXTID++;
	if (NEXTID == 0) NEXTID++;		// Keep non-zero

	TNC->PortRecord->ATTACHEDSESSIONS[Stream] = Session;
	TNC->Streams[Stream].Attached = TRUE;

	Session->L4TARGET.EXTPORT = TNC->PortRecord;
	
	Session->L4CIRCUITTYPE = UPLINK+PACTOR;
	Session->L4WINDOW = L4DEFAULTWINDOW;
	Session->L4STATE = 5;
	Session->SESSIONT1 = L4T1;
	Session->SESSPACLEN = TNC->PortRecord->PORTCONTROL.PORTPACLEN;
	Session->KAMSESSION = Stream;

	TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

	if (HFCTEXTLEN > 1 && SENDCTEXT)
	{
		buffptr = GetBuff();
		if (buffptr == 0) return TRUE;			// No buffers
					
		buffptr[1] = HFCTEXTLEN;
		memcpy(&buffptr[2], HFCTEXT, HFCTEXTLEN);
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
	}
	return TRUE;	
}

char * Config;
static char * ptr1, * ptr2;

BOOL ReadConfigFile(int Port, int ProcLine())
{
	char buf[256],errbuf[256];

	if (TNCInfo[Port])					// If restarting, free old config
		free(TNCInfo[Port]);
	
	TNCInfo[Port] = NULL;

	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		if (strlen(Config) == 0)
		{
			// Empty Config File - OK for most types

			struct TNCINFO * TNC = TNCInfo[Port] = zalloc(sizeof(struct TNCINFO));

			TNC->InitScript = malloc(2);
			TNC->InitScript[0] = 0;
		
			return TRUE;
		}

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1 + 1);
			buf[ptr2 - ptr1 + 1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcLine(buf, Port))
			{
				WritetoConsoleLocal("Bad config record ");
				WritetoConsoleLocal(errbuf);
			}
		}
	}
	else
	{
		sprintf(buf," ** Error - No Configuration info in bpq32.cfg");
		WritetoConsoleLocal(buf);
	}

	return (TRUE);
}
int GetLine(char * buf)
{
loop:

	if (ptr2 == NULL) 
		return 0;

	memcpy(buf, ptr1, ptr2 - ptr1 + 2);
	buf[ptr2 - ptr1 + 2] = 0;
	ptr1 = ptr2 + 2;
	ptr2 = strchr(ptr1, 13);
	
	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}
VOID DigiToMultiplePorts(struct PORTCONTROL * PORTVEC, PMESSAGE Msg)
{
	USHORT Mask=PORTVEC->DIGIMASK;
	int i;

	for (i=1; i<=NUMBEROFPORTS; i++)
	{
		if (Mask & 1)
		{
			// Block includes the Msg Header (7 bytes), Len Does not!

			Msg->PORT = i;
			Send_AX((UCHAR *)&Msg, Msg->LENGTH - 7, i);
			Mask>>=1;
		}
	}
}

int CompareAlias( void *a, void *b) 
{ 
	struct DEST_LIST * x;
	struct DEST_LIST * y;
	UINT * c;

	c = a;
	c = (UINT *)*c;
	x = (struct DEST_LIST *)c;

	c = b;
	c = (UINT *)*c;
	y = (struct DEST_LIST *)c;

	return memcmp(x->DEST_ALIAS, y->DEST_ALIAS, 6);
	/* strcmp functions works exactly as expected from
	comparison function */ 
} 


int CompareNode(void *a, void *b) 
{ 
	struct DEST_LIST * x;
	struct DEST_LIST * y;
	UINT * c;

	c = a;
	c = (UINT *)*c;
	x = (struct DEST_LIST *)c;

	c = b;
	c = (UINT *)*c;
	y = (struct DEST_LIST *)c;

	return memcmp(x->DEST_CALL, y->DEST_CALL, 7);
	/* strcmp functions works exactly as expected from
	comparison function */ 
} 

DllExport int APIENTRY ChangeSessionCallsign(int Stream, unsigned char * AXCall)
{ 
	// Equivalent to "*** linked to" command

	memcpy(BPQHOSTVECTOR[Stream-1].HOSTSESSION->L4USER, AXCall, 7);
	return (0);
}

DllExport int APIENTRY ChangeSessionPaclen(int Stream, int Paclen)
{ 
	BPQHOSTVECTOR[Stream-1].HOSTSESSION->SESSPACLEN = Paclen;
	return (0);
}


DllExport int APIENTRY Get_APPLMASK(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLMASK;
}
DllExport int APIENTRY GetStreamPID(int Stream)
{ 
	return	BPQHOSTVECTOR[Stream-1].STREAMOWNER;
}

DllExport int APIENTRY GetApplFlags(int Stream)
{ 
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLFLAGS;
}

DllExport int APIENTRY GetApplNum(int Stream)
{ 
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLNUM;
}

DllExport BOOL APIENTRY GetAllocationState(int Stream)
{ 
	return	BPQHOSTVECTOR[Stream-1].HOSTFLAGS & 0x80;
}

VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);

extern int InitDone;
extern int SemHeldByAPI;
extern char pgm[256];		// Uninitialised so per process
extern int BPQHOSTAPI();
extern int SemProcessID;


VOID POSTSTATECHANGE(BPQVECSTRUC * SESS)
{
	//	Post a message if requested
#ifndef LINBPQ	
	if (SESS->HOSTHANDLE)
		PostMessage(SESS->HOSTHANDLE, BPQMsg, SESS->HOSTSTREAM, 4);
#endif
	return;
}


DllExport int APIENTRY SessionControl(int stream, int command, int Mask)
{
	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return (0);
	
	SESS = &BPQHOSTVECTOR[stream];

	//	Send Session Control command (BPQHOST function 6)
	//;	CL=0 CONNECT USING APPL MASK IN DL
	//;	CL=1, CONNECT. CL=2 - DISCONNECT. CL=3 RETURN TO NODE

	if 	(command > 1)
	{
		// Disconnect

		if (SESS->HOSTSESSION == 0)
		{
			SESS->HOSTFLAGS |= 1;		// State Change
			POSTSTATECHANGE(SESS);
			return 0;					// NOT CONNECTED
		}
			
		if (command == 3)
			SESS->HOSTFLAGS |= 0x20;	// Set Stay

		SESS->HOSTFLAGS |= 0x40;		// SET 'DISC REQ' FLAG

		return 0;
	}

	if (SESS->HOSTSESSION)				// ALREADY CONNECTED
	{
		SESS->HOSTFLAGS |= 1;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;
	}

	//	SET UP A SESSION FOR THE CONSOLE

	SESS->HOSTFLAGS |= 0x80;			// SET ALLOCATED BIT

	if (command == 1)
		Mask = SESS->HOSTAPPLMASK;		// SO WE GET CORRECT CALLSIGN

	L4 = SetupSessionFromHost(SESS, Mask);

	if (L4 == 0)						// tables Full
	{
		SESS->HOSTFLAGS |= 3;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;	
	}

	SESS->HOSTSESSION = L4;
	L4->L4CIRCUITTYPE = BPQHOST | UPLINK;
 	L4->Secure_Session = AuthorisedProgram;	// Secure Host Session

	SESS->HOSTFLAGS |= 1;		// State Change
	POSTSTATECHANGE(SESS);
	return 0;					// ALREADY CONNECTED
}

DllExport int APIENTRY FindFreeStream()
{
	int stream, n;
	BPQVECSTRUC * PORTVEC;

//	Returns number of first unused BPQHOST stream. If none available,
//	returns 255. See API function 13.

	// if init has not yet been run, wait.

	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == -1)			// Init failed
		exit(0);

	GetSemaphore();
	SemHeldByAPI = 9;

	stream = 0;
	n = 64;

	while (n--)
	{
		PORTVEC = &BPQHOSTVECTOR[stream++];
		if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
		{
			PORTVEC->STREAMOWNER=GetCurrentProcessId();
			PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
			memcpy(&PORTVEC->PgmName[0], pgm, 31);
			FreeSemaphore();
			return stream;
		}
	}		
	FreeSemaphore();
	return 255;
}

DllExport int APIENTRY AllocateStream(int stream)
{
//	Allocate stream. If stream is already allocated, return nonzero.
//	Otherwise allocate stream, and return zero.	
	
	BPQVECSTRUC * PORTVEC = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
	{
		PORTVEC->STREAMOWNER=GetCurrentProcessId();
		PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&PORTVEC->PgmName[0], pgm, 31);
		FreeSemaphore();
		return 0;
	}
	
	return 1;				// Already allocated
}


DllExport int APIENTRY DeallocateStream(int stream)
{
	BPQVECSTRUC * PORTVEC;
	UINT * monbuff;

//	Release stream.

	stream--;

	if (stream < 0 || stream > 63)
		return (0);
	
	PORTVEC=&BPQHOSTVECTOR[stream];
	
	PORTVEC->STREAMOWNER=0;
	PORTVEC->PgmName[0] = 0;
	PORTVEC->HOSTAPPLFLAGS=0;
	PORTVEC->HOSTAPPLMASK=0;
	PORTVEC->HOSTHANDLE=0;

	// Clear Trace Queue

	if (PORTVEC->HOSTSESSION)
		SessionControl(stream + 1, 2, 0);


	while (PORTVEC->HOSTTRACEQ)
	{
		monbuff = Q_REM(&PORTVEC->HOSTTRACEQ);
		ReleaseBuffer(monbuff);
	}

	PORTVEC->HOSTFLAGS &= 0x60;			// Clear Allocated. Must leave any DISC Pending bits

	return(0);
}
DllExport int APIENTRY SessionState(int stream, int * state, int * change)
{
	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough

	GetSemaphore();
	SemHeldByAPI = 20;


	//	CX = 0 if stream disconnected or CX = 1 if stream connected
	//	DX = 0 if no change of state since last read, or DX = 1 if
	//	       the connected/disconnected state has changed since
	//	       last read (ie. delta-stream status).

	//	HOSTFLAGS = Bit 80 = Allocated
	//		  Bit 40 = Disc Request
	//		  Bit 20 = Stay Flag
	//		  Bit 02 and 01 State Change Bits

	if ((HOST->HOSTFLAGS & 3) == 0)		
		// No Chaange
		*change = 0;
	else
		*change = 1;

	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;
	
	HOST->HOSTFLAGS &= 0xFC;		// Clear Change Bitd		

	FreeSemaphore();
	return 0;
}

DllExport int APIENTRY SessionStateNoAck(int stream, int * state)
{
	//	Get current Session State. Dont ACK any change
	//	See BPQHOST function 4

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough
	
	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;

	return 0;
}

DllExport int APIENTRY SendMsg(int stream, char * msg, int len)
{
	//	Send message to stream (BPQHOST Function 2)

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	PDATAMESSAGE MSG;

	Check_Timer();

	if (len > 256)
		return 0;						// IGNORE

	if (stream == 0)
	{
		// Send UNPROTO - SEND FRAME TO ALL RADIO PORTS

		//	COPY DATA TO A BUFFER IN OUR SEGMENTS - SIMPLFIES THINGS LATER

		if (QCOUNT < 20)
			return 0;					// DOnt want to run out

		GetSemaphore();
		SemHeldByAPI = 10;
		
		if ((MSG = GetBuff()) == 0)
		{
			FreeSemaphore();
			return 0;
		}

		MSG->PID = 0xF0;				// Normal Data PID

		memcpy(&MSG->L2DATA[0], msg, len);
		MSG->LENGTH = len + MSGHDDRLEN + 1;

		SENDUIMESSAGE(MSG);
		ReleaseBuffer(MSG);
		FreeSemaphore();
		return 0;
	}

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;

	GetSemaphore();
	SemHeldByAPI = 22;

	SESS->HOSTFLAGS |= 0x80;		// SET ALLOCATED BIT

	if (QCOUNT < 40)				// PLENTY FREE?
	{
		FreeSemaphore();
		return 1;
	}

	if ((MSG = GetBuff()) == 0)
	{
		FreeSemaphore();
		return 1;
	}

	MSG->PID = 0xF0;				// Normal Data PID

	memcpy(&MSG->L2DATA[0], msg, len);
	MSG->LENGTH = len + MSGHDDRLEN + 1;

	//	IF CONNECTED, PASS MESSAGE TO TARGET CIRCUIT - FLOW CONTROL AND
	//	DELAYED DISC ONLY WORK ON ONE SIDE

	Partner = L4->L4CROSSLINK;

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	if (Partner && Partner->L4STATE > 4)	// Partner and link up
	{
		//	Connected
	
		C_Q_ADD(&Partner->L4TX_Q, MSG);
		PostDataAvailable(Partner);
	}
	else
		C_Q_ADD(&L4->L4RX_Q, MSG);
	
	FreeSemaphore();
	return 0;
}
DllExport int APIENTRY SendRaw(int port, char * msg, int len)
{
	struct PORTCONTROL * PORT;
	MESSAGE * MSG;

	Check_Timer();

	//	Send Raw (KISS mode) frame to port (BPQHOST function 10)

	if (len > (MAXDATA - (MSGHDDRLEN + 8)))
		return 0;

	if (QCOUNT < 50)
		return 1;
	
	//	GET A BUFFER

	PORT = GetPortTableEntryFromSlot(port);

	if (PORT == 0)
		return 0;

	GetSemaphore();
	SemHeldByAPI = 24;

	MSG = GetBuff();
	
	if (MSG == 0)
	{
		FreeSemaphore();
		return 1;
	}

	memcpy(MSG->DEST, msg, len);

	MSG->LENGTH = len + MSGHDDRLEN;

	if (PORT->PROTOCOL == 10)		 // PACTOR/WINMOR Style
	{
		//	Pactor Style. Probably will only be used for Tracker uneless we do APRS over V4 or WINMOR

		EXTPORTDATA * EXTPORT = (EXTPORTDATA *) PORT;

		C_Q_ADD(&EXTPORT->UI_Q,	MSG);

		FreeSemaphore();
		return 0;
	}

	MSG->PORT = port; 

	PUT_ON_PORT_Q(PORT, MSG);
	
	FreeSemaphore();
	return 0;
}

DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count )
{
//	Get message from stream. Returns length, and count of frames
//	still waiting to be collected. (BPQHOST function 3)
//	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
//		in CX.  BX returns the number of outstanding frames still to
//		be received (ie. after this one) or zero if no more frames
//		(ie. this is last one).
//

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	PDATAMESSAGE MSG;
	int Msglen;

	Check_Timer();

	*len = 0;
	*count = 0;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;


	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore();
	SemHeldByAPI = 25;

	if (L4 == 0 || L4->L4TX_Q == 0)
	{
		FreeSemaphore();
		return 0;
	}

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	MSG = Q_REM(&L4->L4TX_Q);

	Msglen = MSG->LENGTH - (MSGHDDRLEN + 1);	// Dont want PID

	if (Msglen < 0)
	{
		FreeSemaphore();
		return 0;
	}

	if (Msglen > 256)
		Msglen = 256;

	memcpy(msg, &MSG->L2DATA[0], Msglen);

	*len = Msglen;

	ReleaseBuffer(MSG);

	*count = C_Q_COUNT(&L4->L4TX_Q);
	FreeSemaphore();
	
	return 0;
}


DllExport int APIENTRY RXCount(int stream)
{
//	Returns count of packets waiting on stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	return C_Q_COUNT(&L4->L4TX_Q);
}

DllExport int APIENTRY TXCount(int stream)
{
//	Returns number of packets on TX queue for stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	L4 = L4->L4CROSSLINK;

	if (L4 == 0)
		return 0;			// NOTHING ro Q on
	
	return (CountFramesQueuedOnSession(L4));
}

DllExport int APIENTRY MONCount(int stream)
{
//	Returns number of monitor frames available
//	 (BPQHOST function 7 (part)).
	
	BPQVECSTRUC * SESS;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];

	return C_Q_COUNT(&SESS->HOSTTRACEQ);
}


DllExport int APIENTRY GetCallsign(int stream, char * callsign)
{
	//	Returns call connected on stream (BPQHOST function 8 (part)).
	
	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    "; 
	UCHAR * AXCall;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore();
	SemHeldByAPI = 26;

	if (L4 == 0)
	{
		FreeSemaphore();
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED
			
				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore();
	return 0;
}

DllExport int APIENTRY GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window)
{
	// Return the Secure Session Flag rather than not connected

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    "; 
	UCHAR * AXCall;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore();
	SemHeldByAPI = 27;

	if (L4 == 0)
	{
		FreeSemaphore();
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	// Return the Secure Session Flag rather than not connected

	//		AL = Radio port on which channel is connected (or zero)
	//		AH = SESSION TYPE BITS
	//		EBX = L2 paclen for the radio port
	//		ECX = L2 maxframe for the radio port
	//		EDX = L4 window size (if L4 circuit, or zero) or -1 if not connected
	//		ES:DI = CALLSIGN

	*port = 0;
	*sesstype = 0;
	*paclen = 0;
	*maxframe = 0;
	*l4window = 0;
	*paclen = L4->SESSPACLEN;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		*l4window = Partner->L4WINDOW;
		*sesstype = Partner->L4CIRCUITTYPE;

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			//	EXTRACT PORT AND MAXFRAME

			*port = LINK->LINKPORT->PORTNUMBER;
			*maxframe = LINK->LINKWINDOW;
			*l4window = 0;

			AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED
			
				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			*port = EXTPORT->PORTCONTROL.PORTNUMBER;
			AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (L4->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore();

	if (Partner)
		return Partner->Secure_Session;
	
	return 0;
}


DllExport int APIENTRY SetAppl(int stream, int flags, int mask)
{
//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in EDX (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.


	BPQVECSTRUC * PORTVEC;
	stream--;

	if (stream < 0 || stream > 63)
		return (0);
	
	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->HOSTAPPLFLAGS = flags;
	PORTVEC->HOSTAPPLMASK = mask;
	
	// If either is non-zero, set allocated and Process. This gets round problem with
	// stations that don't call allocate stream
	
	if (flags || mask)
	{
		PORTVEC->STREAMOWNER=GetCurrentProcessId();
		PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&PORTVEC->PgmName[0], pgm, 31);
	}
	
	return (0);
} 

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntry(int portslot)		// Kept for Legacy apps
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

// Proc below renamed to avoid confusion with GetPortTableEntryFromPortNum

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

struct PORTCONTROL * APIENTRY GetPortTableEntryFromPortNum(int portnum)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	do
	{
		if (PORTVEC->PORTNUMBER == portnum)
			return PORTVEC;

		PORTVEC=PORTVEC->PORTPOINTER;
	}
	while (PORTVEC);

	return NULL;
}

DllExport UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	memcpy(Desc, PORTVEC->PORTDESCRIPTION, 30);
	Desc[30]=0;

	return 0;
}

// Standard serial port handling routines, used by lots of modules.

#ifdef WIN32

HANDLE OpenCOMPort(int port, int speed)
{            
	char szPort[15];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	int	Err;
	char buf[100];
	HANDLE fd;	
	DCB dcb;

	// load the COM prefix string and append port number
   
	sprintf( szPort, "\\\\.\\COM%d", port);

	// open COMM device

	fd = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (fd == (HANDLE) -1)
	{
		sprintf(buf,"COM%d could not be opened ", port);
		WritetoConsoleLocal(buf);
		strcat(buf, "\r\n");
		OutputDebugString(buf);
		return (FALSE);
	}
	
	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096 ) ;

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500 ;
	SetCommTimeouts(fd, &CommTimeOuts ) ;
	
   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState(fd, &dcb ) ;

   dcb.BaudRate = speed;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = 0;

   // setup hardware flow control

   dcb.fOutxDsrFlow = 0;
   dcb.fDtrControl = DTR_CONTROL_ENABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_ENABLE ;

   // setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

   fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		// assertDTR

		EscapeCommFunction(fd, SETDTR);
	}
	else
	{
		sprintf(buf,"COM%d Setup Failed %d ", port, GetLastError());
		WritetoConsoleLocal(buf);
		OutputDebugString(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;

}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue 
	
	ClearCommError(fd, &dwErrorFlags, &ComStat);

	dwLength = min((DWORD) MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL) ;

		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(fd, &dwErrorFlags, &ComStat ) ;
		}
	}

   return dwLength;
}


BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
	                       &BytesWritten, NULL );

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
		ClearCommError(fd, &ErrorFlags, &ComStat);
	
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	SetCommMask(fd, 0);

	// drop DTR

	EscapeCommFunction(fd, CLRDTR);

	// purge any outstanding reads/writes and close device handle

	EscapeCommFunction(fd, CLRDTR);
	EscapeCommFunction(fd, CLRDTR);
	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
	
	CloseHandle(fd);
}

#else

static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};
 

HANDLE OpenCOMPort(int port, int speed)
{
	char Port[256];
	char buf[100];

	//	Linux Version.

	int fd;
	int hwflag = 0;
	u_long param=1;
	struct termios old;
	struct termios new;
	struct termios term;
	struct speed_struct *s;

	// As Serial ports under linux can have all sorts of odd names, the code assumes that
	// they are symlinked to a com1-com255 in the BPQ Directory (normally the one it is started from

	sprintf(Port, "%s/com%d", BPQDirectory, port);

	if ((fd = open(Port, O_RDWR | O_NDELAY)) == -1)
	{
		perror("Com Open Failed");
		sprintf(buf,"%s could not be opened \n", Port);
		WritetoConsoleLocal(buf);
		Debugprintf(buf);
		return 0;
	}

	tcgetattr(0, &old);
	new = old;

	cfmakeraw(&new);

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)      
		if (s->user_speed == speed)
			break;

   if (s->user_speed == -1)
   {
	   fprintf(stderr, "tty_speed: invalid speed %d\n", speed);
	   return FALSE;
   }
   
   if (tcgetattr(fd, &term) == -1)
   {
	   perror("tty_speed: tcgetattr");
	   return FALSE;
   }

	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	return fd;
}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	DWORD Length;
	
	Length = read(fd, Block, MaxLength);

	if (Length < 0)
	{
		if	(errno != 11)					// Would Block
		{
			perror("read");
			printf("%d\n", errno);
		}
		return 0;
	}
	return Length;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	return write(fd, Block, BytesToWrite);
}

VOID CloseCOMPort(HANDLE fd)
{
	close(fd);
}

#endif


int MaxNodes;
int MaxRoutes;
int NodeLen;
int RouteLen;
struct DEST_LIST * Dests;
struct ROUTE * Routes;

FILE *file;

int DoRoutes()
{
	char digis[30] = "";
	char locked[3];
	int count, len;
	char Normcall[10], Portcall[10];
	char line[80];

	for (count=0; count<MaxRoutes; count++)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
		{
			len=ConvFromAX25(Routes->NEIGHBOUR_CALL,Normcall);
			Normcall[len]=0;

			if ((Routes->NEIGHBOUR_FLAG & 1) == 1)
	
				strcpy(locked," !");
			else
				strcpy(locked," ");


			if (Routes->NEIGHBOUR_DIGI1[0] != 0)
			{
				memcpy(digis," VIA ",5);

				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
				Portcall[len]=0;
				strcpy(&digis[5],Portcall);

				if (Routes->NEIGHBOUR_DIGI2[0] != 0)
				{
					len=ConvFromAX25(Routes->NEIGHBOUR_DIGI2,Portcall);
					Portcall[len]=0;
					strcat(digis," ");
					strcat(digis,Portcall);
				}
			}
			else
				digis[0] = 0;

			len=sprintf(line,
					"ROUTE ADD %s %d %d%s%s %d %d %d %d \r\n",
					Normcall,
					Routes->NEIGHBOUR_PORT,
					Routes->NEIGHBOUR_QUAL,	locked, digis,
					Routes->NBOUR_MAXFRAME,
					Routes->NBOUR_FRACK,
					Routes->NBOUR_PACLEN,
					Routes->INP3Node | (Routes->NoKeepAlive << 1));	

					fputs(line, file);
		}
		
		Routes+=1;
	}

	return (0);
}

int DoNodes()
{
	int count, len, cursor, i;
	char Normcall[10], Portcall[10];
	char line[80];
	char Alias[7];

	Dests-=1;

	for (count=0; count<MaxNodes; count++)
	{
		Dests+=1;
		
		if (Dests->NRROUTE[0].ROUT_NEIGHBOUR == 0)
			continue;

		{
			len=ConvFromAX25(Dests->DEST_CALL,Normcall);
			Normcall[len]=0;

			memcpy(Alias,Dests->DEST_ALIAS,6);
		
			Alias[6]=0;

			for (i=0;i<6;i++)
			{
				if (Alias[i] == ' ')
					Alias[i] = 0;
			}

			cursor=sprintf(line,"NODE ADD %s:%s ", Alias,Normcall);
				
			if (Dests->NRROUTE[0].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[0].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[0].ROUT_QUALITY);
	
				cursor+=len;

				if (Dests->NRROUTE[0].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

			if (Dests->NRROUTE[1].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[1].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[1].ROUT_QUALITY);
	
				cursor+=len;

				if (Dests->NRROUTE[1].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

		if (Dests->NRROUTE[2].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[2].ROUT_NEIGHBOUR->INP3Node == 0)
		{
			len=ConvFromAX25(
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;

			len=sprintf(&line[cursor],"%s %d %d ",
				Portcall,
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->NRROUTE[2].ROUT_QUALITY);
	
			cursor+=len;

			if (Dests->NRROUTE[2].ROUT_OBSCOUNT > 127)
			{
				len=sprintf(&line[cursor],"! ");
				cursor+=len;
			}
		}

		if (cursor > 30)
		{
			line[cursor++]='\r';
			line[cursor++]='\n';
			line[cursor++]=0;
			fputs(line, file);
		}
		}
	}
	return (0);
}

DllExport int APIENTRY SaveNodes ()
{
	char FN[250];

	Routes = NEIGHBOURS;
	RouteLen = ROUTE_LEN;
	MaxRoutes = MAXNEIGHBOURS;

	Dests = DESTS;
	NodeLen = DEST_LIST_LEN;
	MaxNodes = MAXDESTS;

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"BPQNODES.dat");
	}
		
	if ((file = fopen(FN, "w")) == NULL)
		return FALSE;

	DoRoutes();
	DoNodes();

	fclose(file);
	
	return (0);
}

DllExport int APIENTRY ClearNodes ()
{
	char FN[250];

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"BPQNODES.dat");
	}
/*
	handle = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

 	CloseHandle(handle);
*/	
	return (0);
}
char * FormatUptime(int Uptime)
 {
	struct tm * TM;
	static char UPTime[50];
	time_t szClock = Uptime * 60;

	TM = gmtime(&szClock);

	sprintf(UPTime, "Uptime (Days Hours Mins)     %.2d:%.2d:%.2d\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min);

	return UPTime;
 }

 char * FormatMH(PMHSTRUC MH)
 {
	struct tm * TM;
	static char MHTime[50];
	time_t szClock;
	char LOC[7];
	
	memcpy(LOC, MH->MHLocator, 6);
	LOC[6] = 0;

	szClock = time(NULL) - MH->MHTIME;

	TM = gmtime(&szClock);

	sprintf(MHTime, "%.2d:%.2d:%.2d:%.2d  %s %s\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);

	return MHTime;

 }

Dll VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset)
{
	// Create a time dependent One Time Password from the KeyPhrase
	// TimeOffset is used when checking to allow for slight variation in clocks

	time_t NOW = time(NULL);
	UCHAR Hash[16];
	char Key[1000];
	int i, chr;

	NOW = NOW/30 + TimeOffset;				// Only Change every 30 secs

	sprintf(Key, "%s%x", KeyPhrase, NOW);

	md5(Key, Hash);

	for (i=0; i<16; i++)
	{
		chr = (Hash[i] & 31);
		if (chr > 9) chr += 7;
		
		Password[i] = chr + 48; 
	}

	Password[16] = 0;
	return;
}
Dll BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase)
{
	char CheckPassword[17];
	int Offsets[10] = {0, -1, 1, -2, 2, -3, 3, -4, 4};
	int i, Pass;

	if (strlen(Password) < 16)
		Pass = atoi(Password);

	for (i = 0; i < 9; i++)
	{
		CreateOneTimePassword(CheckPassword, KeyPhrase, Offsets[i]);

		if (strlen(Password) < 16)
		{
			// Using a numeric extract 

			long long Val;

			memcpy(&Val, CheckPassword, 8);
			Val = Val %= 1000000;

			if (Pass == Val)
				return TRUE;
		}
		else
			if (memcmp(Password, CheckPassword, 16) == 0)
				return TRUE;
	}

	return FALSE;
}




