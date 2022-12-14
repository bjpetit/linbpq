//
//	DLL to inteface DED Host Mode TNCs to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"

#define MaxStreams 1	

#include "TNCInfo.h"

#include "ASMStrucs.h"

#include "bpq32.h"

static char ClassName[]="TRACKERSTATUS";
static char WindowTitle[] = "SCS Tracker";
static int RigControlRow = 140;

#define NARROWMODE 30
#define WIDEMODE 30			// Robust

extern UCHAR BPQDirectory[];

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];
extern char LOC[];

static RECT Rect;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd
extern char * RigConfigMsg[35];

VOID __cdecl Debugprintf(const char * format, ...);
char * strlop(char * buf, char delim);

char NodeCall[11];		// Nodecall, Null Terminated

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void DEDCheckRX(struct TNCINFO * TNC);
VOID DEDPoll(int Port);
VOID StuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len);
unsigned short int compute_crc(unsigned char *buf,int len);
int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID ExitHost(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);
BOOL OpenVirtualSerialPort(struct TNCINFO * TNC);
VOID DoMonitorHddr(struct TNCINFO * TNC, UCHAR * Msg, int Len, int Type);
VOID DoMonitorData(struct TNCINFO * TNC, UCHAR * Msg, int Len);
Switchmode(struct TNCINFO * TNC, int Mode);
VOID SwitchToRPacket(struct TNCINFO * TNC, char * Baud);
VOID SwitchToNormPacket(struct TNCINFO * TNC, char * Baud);
VOID SendRPBeacon(struct TNCINFO * TNC);
BOOL APIENTRY  Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port);

extern UCHAR NEXTID;
extern  struct TRANSPORTENTRY * L4TABLE;
extern  WORD MAXCIRCUITS;
extern  UCHAR L4DEFAULTWINDOW;
extern  WORD L4T1;

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

static ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_port = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	BPQport = Port;

	TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
	memset(TNC, 0, sizeof(struct TNCINFO));

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;

	strcpy(TNC->NormSpeed, "300");		// HF Packet

	goto ConfigLine;

	while(TRUE)
	{
		if (GetLine(buf) == 0)
			return TRUE;
ConfigLine:
		strcpy(errbuf, buf);

		if (memcmp(buf, "****", 4) == 0)
			return TRUE;

		ptr = strchr(buf, ';');

		if (ptr)
		{
			*ptr++ = 13;
			*ptr = 0;
		}

		if (_memicmp(buf, "APPL", 4) == 0)
		{
			p_cmd = strtok(&buf[4], " \t\n\r");

			if (p_cmd && p_cmd[0] != ';' && p_cmd[0] != '#')
				TNC->ApplCmd=_strdup(_strupr(p_cmd));
		}
		else
		
//		if (_mem`icmp(buf, "PACKETCHANNELS", 14) == 0)


//			// Packet Channels

//			TNC->PacketChannels = atoi(&buf[14]);
//		else

		if (_memicmp(buf, "SWITCHMODES", 11) == 0)
		{
			// Switch between Normal and Robust Packet 

			double Robust = atof(&buf[12]);
			#pragma warning(push)
			#pragma warning(disable : 4244)
			TNC->RobustTime = Robust * 10;
			#pragma warning(pop)
		}
		if (_memicmp(buf, "BEACONAFTERSESSION", 18) == 0) // Send Beacon after each session 
			TNC->RPBEACON = TRUE;
		else
		if (_memicmp(buf, "USEAPPLCALLS", 12) == 0)
			TNC->UseAPPLCalls = TRUE;
		else
		if (_memicmp(buf, "DEFAULT ROBUST", 14) == 0)
			TNC->RobustDefault = TRUE;
		else
		if (_memicmp(buf, "FORCE ROBUST", 12) == 0)
			TNC->ForceRobust = TNC->RobustDefault = TRUE;
		else
		if (_memicmp(buf, "UPDATEMAP", 9) == 0)
			TNC->PktUpdateMap = TRUE;
		else
		if (_memicmp(buf, "WL2KREPORT", 10) == 0)
			TNC->WL2K = DecodeWL2KReportLine(buf);
		else
		{
			strcat (TNC->InitScript, buf);

			// If %B param,and not R300 or R600 extract speed

			if (_memicmp(buf, "%B", 2) == 0)
			{
				ptr = strchr(buf, '\r');
				if (ptr) *ptr = 0;
				if (strchr(buf, 'R') == 0)
					strcpy(TNC->NormSpeed, &buf[3]);
			}
		}
	}
	return (TRUE);

}

static int ExtProc(int fn, int port, unsigned char * buff)
{
	int txlen = 0;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	int Param;
	int Stream = 0;
	struct STREAMINFO * STREAM;
	int TNCOK;
	struct ScanEntry * Scan;


	if (TNC == NULL)
		return 0;
	
	
	if (TNC->hDevice == (HANDLE) -1)
	{
		// Clear anything from UI_Q

		while (TNC->PortRecord->UI_Q)
		{
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);
		}

		if (fn > 3  && fn < 6)
			goto ok;

		// Try to reopen every 30 secs

		TNC->ReopenTimer++;

		if (TNC->ReopenTimer < 300)
			return 0;

		TNC->ReopenTimer = 0;
		
		OpenCOMMPort(TNC, TNC->PortRecord->PORTCONTROL.IOBASE, TNC->PortRecord->PORTCONTROL.BAUDRATE, TRUE);

		if (TNC->hDevice == (HANDLE) -1)
			return 0;
	}
ok:
	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].ReportDISC)
			{
				TNC->Streams[Stream].ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}

		DEDCheckRX(TNC);
		DEDPoll(port);
		DEDCheckRX(TNC);

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = Stream;
				buff[7] = 0xf0;
				memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
				datalen+=8;
				buff[5]=(datalen & 0xff);
				buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}
	
			
		return 0;

	case 2:				// send

		buffptr = GetBuff();

		if (buffptr == 0) return (0);			// No buffers, so ignore

		Stream = buff[4];

		if (!TNC->TNCOK)
		{
			// Send Error Response

			buffptr[1] = 36;
			memcpy(buffptr+2, "No Connection to PACTOR TNC\r", 36);

			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

		TNC->Streams[Stream].FramesOutstanding++;
		
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding

		_asm 
		{

			MOV	EAX,buff
			mov Stream,eax
		}

		TNCOK = (TNC->HostMode == 1 && TNC->ReinitState != 10);

		STREAM = &TNC->Streams[Stream];

		if (Stream == 0)
		{
			if (STREAM->FramesOutstanding  > 4)
				return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);
		}
		else
		{
			if (STREAM->FramesOutstanding > 3 || TNC->Buffers < 200)	
				return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);		}

		return TNCOK << 8 | STREAM->Disconnecting << 15;		// OK, but lock attach if disconnecting


	case 4:				// reinit

		ExitHost(TNC);
		Sleep(50);
		CloseHandle(TNC->hDevice);
		TNC->hDevice =(HANDLE) -1;
		TNC->ReopenTimer = 250;
		TNC->HostMode = FALSE;

		return (0);

	case 5:				// Close

		// Ensure in Pactor

		ExitHost(TNC);

		Sleep(25);

		CloseHandle(TNC->hDevice);
		return (0);

	case 6:				// Scan Stop Interface

		_asm 
		{
			MOV	EAX,buff
			mov Param,eax
		}

		switch (Param)
		{
		case 1:		// Request Permission

			if (TNC->TNCOK)
			{
				TNC->WantToChangeFreq = TRUE;
				TNC->OKToChangeFreq = TRUE;
				return 0;
			}
			return 0;		// Don't lock scan if TNC isn't reponding
		

		case 2:		// Check  Permission
			return TNC->OKToChangeFreq;

		case 3:		// Release  Permission
		
			TNC->WantToChangeFreq = FALSE;
			TNC->DontWantToChangeFreq = TRUE;
			return 0;
		

		default: // Change Mode. Param is Address of a struct ScanEntry

			Scan = (struct ScanEntry *)buff;

			if (Scan->RPacketMode == '1')
			{
				SwitchToRPacket(TNC, "R300");
				return 0;
			}
			if (Scan->RPacketMode == '2')
			{
				SwitchToRPacket(TNC, "R600");
				return 0;
			}

			if (Scan->HFPacketMode == '1')
			{
				SwitchToNormPacket(TNC, "300");
				return 0;
			}

			if (Scan->HFPacketMode == '2')
			{
				SwitchToNormPacket(TNC, "1200");
				return 0;
			}
		}
	}
	return 0;
}

UINT WINAPI TrackerExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[500];
	struct TNCINFO * TNC;
	int port;
	char * ptr;
	int Stream = 0;
	char * TempScript;

	//
	//	Will be called once for each DED Host TNC Port
	//	The COM port number is in IOBASE
	//

	wsprintf(msg,"SCSTRK COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(msg);

		return (int) ExtProc;
	}
	
	TNC->Port = port;
	TNC->Hardware = H_TRK;

	// Set up DED addresses for streams
	
	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		TNC->Streams[Stream].DEDStream = Stream + 1;	// DED Stream = BPQ Stream + 1
	}

	if (TNC->PacketChannels > MaxStreams)
		TNC->PacketChannels = MaxStreams;

	PortEntry->MAXHOSTMODESESSIONS = 1;				//TNC->PacketChannels + 1;
	PortEntry->PERMITGATEWAY = TRUE;				// Can change ax.25 call on each stream
	PortEntry->SCANCAPABILITIES = NONE;				// Scan Control 3 stage/conlock 

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
		
	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.UICAPABLE = 1;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 100;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// get NODECALL for RP tests

	memcpy(NodeCall, GetNodeCall(), 10);
		
	ptr=strchr(NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	TempScript = malloc(1000);

	strcpy(TempScript, "M UISC\r");
	strcat(TempScript, "F 200\r");			// Sets SABM retry time to about 5 secs
	strcat(TempScript, "%F 1500\r");		// Tones may be changed but I want this as standard

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Others go on end so they can't be overriden

	strcat(TNC->InitScript, "Z 0\r");      //  	No Flow Control
	strcat(TNC->InitScript, "Y 1\r");      //  	One Channel
	strcat(TNC->InitScript, "E 1\r");      //  	Echo - Restart process needs echo
	
	wsprintf(msg, "I %s\r", TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 200);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,10,120,24, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,10,386,26, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,36,106,24, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,36,520,24, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,62,80,24, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,62,200,24, TNC->hDlg, NULL, hInstance, NULL);

	TNC->xIDC_BUFFERS = CreateWindowEx(0, "STATIC", "Buffers", WS_CHILD | WS_VISIBLE, 10,88,80,24, TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE, 116,88,144,24, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,114,80,24, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "RX 0 TX 0", WS_CHILD | WS_VISIBLE,116,114,374,24 , TNC->hDlg, NULL, hInstance, NULL);


	TNC->ClientHeight = 200;
	TNC->ClientWidth = 500;

	MoveWindows(TNC);
	
	if (TNC->RobustDefault)
	{
		TNC->Robust = TRUE;
		strcat(TNC->InitScript, "%B R600\r");
		SetWindowText(TNC->xIDC_MODE, "Robust Packet");
	}
	else
	{
		char Cmd[40];
		wsprintf(Cmd, "%%B %s\r", TNC->NormSpeed);
		strcat(TNC->InitScript, Cmd);
		SetWindowText(TNC->xIDC_MODE, "HF Packet");
	}

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE, FALSE);

	TNC->InitPtr = TNC->InitScript;

	if (RigConfigMsg[port] == NULL)
		TNC->SwitchToPactor = TNC->RobustTime;		// Don't alternate Modes if using Rig Control

	WritetoConsole("\n");

	return ((int)ExtProc);
}

static void DEDCheckRX(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length, Len;
	UCHAR  * ptr;
	UCHAR character;
	UCHAR * CURSOR;

	ComStat.cbInQue = 0;

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	if (ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat) == 0)
	{
		// Device failed. Close and try to reopen

//		CloseHandle(TNC->hDevice);

		Debugprintf("TRK - Read Failed - recovering");
		
		OpenCOMMPort(TNC, TNC->PortRecord->PORTCONTROL.IOBASE, TNC->PortRecord->PORTCONTROL.BAUDRATE, FALSE);

		if (TNC->hDevice == (HANDLE) -1)
			return;
		
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);
	}

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	while (Length > 20)
	{
		fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], 20, &Len, NULL);
		TNC->RXLen += Len;
		Length -= Len;
	}

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], 20, &Len, NULL);

//	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	TNC->RXLen += Len;

	Length = TNC->RXLen;
	
	ptr = TNC->RXBuffer;

	CURSOR = &TNC->DEDBuffer[TNC->InputLen];

	if ((TNC->HostMode == 0 || TNC->ReinitState == 10) && Length > 80)
	{
		// Probably Signon Message

		ptr[Length] = 0;
		Debugprintf("TRK %s", ptr);
		TNC->RXLen = 0;
		return;
	}

	if (TNC->HostMode == 0)
	{
		// If we are just restarting, and TNC is in host mode, we may get "Invalid Channel" Back
		
		if (memcmp(ptr, "\x18\x02INVALID", 9) == 0)
		{
			TNC->HostMode = TRUE;
			TNC->HOSTSTATE = 0;
			TNC->Timeout = 0;
			TNC->RXLen = 0;
			return;
		}

		// Command is echoed as * command * 

		if (strstr(ptr, "*") || TNC->ReinitState == 5)		// 5 is waiting for reponse to JHOST1
		{
			ProcessTermModeResponse(TNC);
			TNC->RXLen = 0;
			TNC->HOSTSTATE = 0;

			return;
		}
	}

	if (TNC->ReinitState == 10)
	{
		if (Length == 1 && *(ptr) == '.')		// 01 echoed as .
		{
			// TNC is in Term Mode

			TNC->ReinitState = 0;
			TNC->HostMode = 0;

			return;
		}
	}


	while (Length--)
	{
		character = *(ptr++);

		if (TNC->HostMode)
		{
			// n       0        Success (nothing follows)
			// n       1        Success (message follows, null terminated)
			// n       2        Failure (message follows, null terminated)
			// n       3        Link Status (null terminated)
			// n       4        Monitor Header (null terminated)
			// n       5        Monitor Header (null terminated)
			// n       6        Monitor Information (preceeded by length-1)
			// n       7        Connect Information (preceeded by length-1)


			switch(TNC->HOSTSTATE)
			{
			case 0: 	//  SETCHANNEL

				TNC->MSGCHANNEL = character;
				TNC->HOSTSTATE++;
				break;

			case 1:		//	SETMSGTYPE

				TNC->MSGTYPE = character;

				if (character == 0)
				{
					// Success, no more info

					ProcessDEDFrame(TNC);
						
					TNC->HOSTSTATE = 0;
					break;
				}

				if (character > 0 && character < 6)
				{
					// Null Terminated Response)
					
					TNC->HOSTSTATE = 5;
					CURSOR = &TNC->DEDBuffer[0];
					break;
				}

				if (character > 5 && character < 8)
				{
					TNC->HOSTSTATE = 2;						// Get Length
					break;
				}

				// Invalid

				Debugprintf("TRK - Invalid MsgType %d %x %x %x", character, *(ptr), *(ptr+1), *(ptr+2));
				break;

			case 2:		//  Get Length

				TNC->MSGCOUNT = TNC->MSGLENGTH = ++character;
				CURSOR = &TNC->DEDBuffer[0];
				TNC->HOSTSTATE = 3;						// Get Data

				break;

			case 5:		//  Collecting Null Terminated Response

				*(CURSOR++) = character;
				
				if (character)
					continue;			// MORE TO COME

				ProcessDEDFrame(TNC);

				TNC->HOSTSTATE = 0;
				TNC->InputLen = 0;

				break;

			default:

			//	RECEIVING Counted Response

			*(CURSOR++) = character;
			TNC->MSGCOUNT--;

			if (TNC->MSGCOUNT)
				continue;			// MORE TO COME

			TNC->InputLen = CURSOR - TNC->DEDBuffer;
			ProcessDEDFrame(TNC);

			TNC->HOSTSTATE = 0;
			TNC->InputLen = 0;
			}
		}
	}

	// End of Input - Save buffer position

	TNC->InputLen = CURSOR - TNC->DEDBuffer;
	TNC->RXLen = 0;
}

static BOOL NEAR WriteCommBlock(struct TNCINFO * TNC)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(TNC->hDevice, TNC->TXBuffer, TNC->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (TNC->TXLen != dwBytesWritten))
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);
	}

	TNC->Timeout = 20;				// 2 secs

	return TRUE;
}

VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];
	int Stream = 0;
	int nn;
	struct STREAMINFO * STREAM;

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			// New Attach

			// Set call to null to stop inbound connects (We only support one stream)

			int calllen=0;

			TNC->Streams[Stream].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
			TNC->Streams[Stream].MyCall[calllen] = 0;

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = zalloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "\1\1\1IDSPTNC");

			if (Stream == 0)
			{
				wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
				SetWindowText(TNC->xIDC_TNCSTATE, Status);

				// Stop Scanner
		
				TNC->SwitchToPactor = 0;						// Cancel any RP to Pactor switch

				wsprintf(Status, "%d SCANSTOP", TNC->Port);
				Rig_Command(-1, Status);
			}
		}
	}

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		// Can't use retries, as we have no way of detecting lost chars. Have to re-init on timeout

		if (TNC->HostMode == 0 || TNC->ReinitState == 10)		// 10 is Recovery Mode
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Timed out in host mode - Clear any connection and reinit the TNC

		Debugprintf("DEDHOST - Link to TNC Lost");
		TNC->TNCOK = FALSE;

		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetWindowText(TNC->xIDC_COMMSSTATE, Status);

		TNC->HostMode = 0;
		TNC->ReinitState = 0;

		TNC->InitPtr = TNC->InitScript;
		TNC->HOSTSTATE = 0;

		
		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			}
		}

		// Clear anything from UI_Q

		while (TNC->PortRecord->UI_Q)
		{
			UINT * buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);
		}
	}

	if (TNC->SwitchToPactor)
	{
		TNC->SwitchToPactor--;
	
		if (TNC->SwitchToPactor == 0)
		{
			TNC->SwitchToPactor = TNC->RobustTime;
			if (TNC->Robust)
				SwitchToNormPacket(TNC, "");
			else
				SwitchToRPacket(TNC, "R600");
		}
	}
 

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		STREAM = &TNC->Streams[Stream];

		if (STREAM->Attached)
			CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

		if (STREAM->NeedDisc)
		{
			STREAM->NeedDisc--;

			if (STREAM->NeedDisc == 0)
					STREAM->ReportDISC = TRUE;

		}

		if (TNC->Timeout)
			return;				// We've sent something
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	if (TNC->InitPtr)
	{
		char * start, * end;
		int len;

		start = TNC->InitPtr;
		
		if (*(start) == 0)			// End of Script
		{
			TNC->InitPtr = NULL;
			Debugprintf("TRK - Init Complete");
		}
		else
		{
			end = strchr(start, 13);
			len = ++end - start -1;	// exclude cr
			
			TNC->InitPtr = end;

			Poll[0] = 0;			// Channel
			Poll[1] = 1;			// Command
			Poll[2] = len - 1;
			memcpy(&Poll[3], start, len);
		
			StuffAndSend(TNC, Poll, len + 3);

			return;

		}
	}

	if (TNC->NeedPACTOR)
	{
		TNC->NeedPACTOR--;

		if (TNC->NeedPACTOR == 0)
		{
			TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = zalloc(100);
			wsprintf(TNC->Streams[0].CmdSet, "\1\1\1%%B %s%c\1\1\1I%s\0", (TNC->RobustDefault) ? "R600" : TNC->NormSpeed, 0, TNC->NodeCall);
			
			strcpy(TNC->Streams[0].MyCall, TNC->NodeCall);
		}
	}
		
	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->Streams[Stream].CmdSet)
		{
			char * start, * end;
			int len;

			start = TNC->Streams[Stream].CmdSet;
		
			if (*(start + 2) == 0)			// End of Script
			{
				free(TNC->Streams[Stream].CmdSave);
				TNC->Streams[Stream].CmdSet = NULL;
			}
			else
			{
				end = strchr(start + 3, 0);
				len = ++end - start -1;	// exclude null
				TNC->Streams[Stream].CmdSet = end;

				Debugprintf("TRK Cmdset %s", start + 3);

				memcpy(&Poll[0], start, len);
				Poll[2] = len - 4;
		
				StuffAndSend(TNC, Poll, len);

				return;
			}
		}
	}

	for (nn = 0; nn <= MaxStreams; nn++)
	{
		Stream = TNC->LastStream++;

		if (TNC->LastStream > MaxStreams) TNC->LastStream = 0;

		if (TNC->TNCOK && TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			UINT * buffptr;
			char * Buffer;
			
			buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);

			datalen=buffptr[1];
			Buffer = (char *)&buffptr[2];	// Data portion of frame

			Poll[0] = TNC->Streams[Stream].DEDStream;		// Channel

			if (TNC->Streams[Stream].Connected)
			{
				if (TNC->SwallowSignon && Stream == 0)
				{
					TNC->SwallowSignon = FALSE;	
					if (strstr(Buffer, "Connected"))	// Discard *** connected
					{
						ReleaseBuffer(buffptr);
						return;
					}
				}

				Poll[1] = 0;			// Data
				TNC->Streams[Stream].BytesTXed += datalen;

				Poll[2] = datalen - 1;
				memcpy(&Poll[3], buffptr+2, datalen);
		
				ReleaseBuffer(buffptr);
		
				StuffAndSend(TNC, Poll, datalen + 3);

				TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

				if (STREAM->Disconnecting && TNC->Streams[Stream].BPQtoPACTOR_Q == 0)
					TidyClose(TNC, 0);

				// Make sure Node Keepalive doesn't kill session.
				
				{
					struct TRANSPORTENTRY * SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

					if (SESS)
					{
						SESS->L4KILLTIMER = 0;
						SESS = SESS->L4CROSSLINK;
						if (SESS)
							SESS->L4KILLTIMER = 0;
					}
				}

				ShowTraffic(TNC);
				return;
			}
			
			// Command. Do some sanity checking and look for things to process locally

			Poll[1] = 1;			// Command
			datalen--;				// Exclude CR

			if (datalen == 0)		// Null Command
			{
				ReleaseBuffer(buffptr);
				return;
			}

			Buffer[datalen] = 0;	// Null Terminate
			_strupr(Buffer);

			if (_memicmp(Buffer, "D", 1) == 0)
			{
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
				ReleaseBuffer(buffptr);
				return;
			}

			if (memcmp(Buffer, "RADIO ", 6) == 0)
			{
				wsprintf(&Buffer[40], "%d %s", TNC->Port, &Buffer[6]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &Buffer[40]))
				{
					ReleaseBuffer(buffptr);
				}
				else
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &Buffer[40]);
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				}
				return;
			}

			if ((Stream == 0) && memcmp(Buffer, "RPACKET", 7) == 0)
			{
				TNC->Robust = TRUE;
				buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "TRK} OK\r");
				C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
				SetWindowText(TNC->xIDC_MODE, "Robust Packet");

				return;
			}

			if ((Stream == 0) && memcmp(Buffer, "HFPACKET", 8) == 0)
			{
				if (TNC->ForceRobust)
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "TRK} HF Packet Disabled\r");
					C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
					return;
				}

				if (strlen(Buffer) > 10)
				{
					// Speed follows HFPACKET

					Buffer += 9;
					
					Buffer = strtok(Buffer, " \r");
					
					if (strlen(Buffer) < 6)
						strcpy(TNC->NormSpeed, Buffer);
				}

				buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "TRK} OK\r");
									
				C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
				TNC->Robust = FALSE;
				SetWindowText(TNC->xIDC_MODE, "HF Packet");
				return;
			}

			if (Buffer[0] == 'C' && datalen > 2)	// Connect
			{
				if (*(++Buffer) == ' ') Buffer++;		// Space isn't needed

				memcpy(TNC->Streams[Stream].RemoteCall, Buffer, 9);

				TNC->Streams[Stream].Connecting = TRUE;

				if (Stream == 0)
				{
					// Send MYCall, Mode Command followed by connect 

					TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = zalloc(100);
							
					wsprintf(TNC->Streams[0].CmdSet, "\1\1\1%%B %s%c\1\1\1I%s%c\1\1\1%s\0",
						(TNC->Robust) ? "R600" : TNC->NormSpeed, 0, TNC->Streams[0].MyCall,0,  buffptr+2);

					ReleaseBuffer(buffptr);
	
					wsprintf(Status, "%s Connecting to %s", TNC->Streams[Stream].MyCall, TNC->Streams[Stream].RemoteCall);
					SetWindowText(TNC->xIDC_TNCSTATE, Status);

					TNC->Streams[0].InternalCmd = FALSE;
					return;
				}
			}

			Poll[2] = datalen - 1;
			memcpy(&Poll[3], buffptr+2, datalen);
		
			ReleaseBuffer(buffptr);
		
			StuffAndSend(TNC, Poll, datalen + 3);

			TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

			return;
		}	
	}

	if (TNC->TNCOK && TNC->PortRecord->UI_Q)
	{
		int datalen;
		char * Buffer;
		char CCMD[80] = "C";
		char Call[12] = "           ";	
		struct _MESSAGE * buffptr;
			
		(UINT *)buffptr = Q_REM(&TNC->PortRecord->UI_Q);
		
		datalen = buffptr->LENGTH - 7;
		Buffer = &buffptr->DEST[0];		// Raw Frame
		
		Buffer[datalen] = 0;
							
		// Buffer has an ax.25 header, which we need to pick out and set as channel 0 Connect address
		// before sending the beacon

		ConvFromAX25(Buffer, &Call[1]);			// Dest
		strlop(&Call[1], ' ');
		strcat(CCMD, Call);
		Buffer += 14;							// Skip Origin
		datalen -= 7;

		while ((Buffer[-1] & 1) == 0)
		{
			ConvFromAX25(Buffer, &Call[1]);
			strlop(&Call[1], ' ');
			strcat(CCMD, Call);
			Buffer += 7;	// End of addr
			datalen -= 7;
		}

		if (Buffer[0] == 3)				// UI
		{
			Buffer += 2;
			datalen -= 2;

			Poll[0] = 0;				// UI Channel
			Poll[1] = 1;				// CMD
			Poll[2] = strlen(CCMD) - 1;
			strcpy(&Poll[3], CCMD);
			StuffAndSend(TNC, Poll, Poll[2] + 4);

			TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = zalloc(400);
			wsprintf(TNC->Streams[0].CmdSet, "%c%c%c%s", 0, 0, 1, Buffer);
		}

		ReleaseBuffer((UINT *)buffptr);
		return;
	}

	// if frames outstanding, issue a poll (but not too often)

	TNC->IntCmdDelay++;

	if (TNC->IntCmdDelay == 5)
	{
		Poll[0] = TNC->Streams[0].DEDStream;
		Poll[1] = 0x1;			// Command
		TNC->InternalCmd = TRUE;
	
		Poll[2] = 1;			// Len-1
		Poll[3] = '@';
		Poll[4] = 'B';			// Buffers
		StuffAndSend(TNC, Poll, 5);
		return;
	}

	if (TNC->IntCmdDelay > 10)
	{
		TNC->IntCmdDelay = 0;
		
		if (TNC->Streams[0].FramesOutstanding)
		{
			Poll[0] = TNC->Streams[0].DEDStream;
			Poll[1] = 0x1;			// Command
			TNC->InternalCmd = TRUE;
	
			Poll[2] = 0;			// Len-1
			Poll[3] = 'L';			// Status
			StuffAndSend(TNC, Poll, 4);

			return;	
		}
	}
	// Need to poll channels 0 and 1 in turn

	TNC->StreamtoPoll ++;

	if (TNC-> StreamtoPoll > 1)
		TNC->StreamtoPoll = 0;

	Poll[0] = TNC->StreamtoPoll;	// Channel
	Poll[1] = 0x1;			// Command
	Poll[2] = 0;			// Len-1
	Poll[3] = 'G';			// Poll

	StuffAndSend(TNC, Poll, 4);
	TNC->InternalCmd = FALSE;

	return;

}

static VOID DoTNCReinit(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Just Starting - Send a TNC Mode Command to see if in Terminal or Host Mode

		char Status[80];
		
		TNC->TNCOK = FALSE;
		wsprintf(Status,"COM%d Initialising TNC", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetWindowText(TNC->xIDC_COMMSSTATE, Status);

		memcpy(&TNC->TXBuffer[0], "\x18\x1b\r", 2);
		TNC->TXLen = 2;

		WriteCommBlock(TNC);
		return;

	}

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		// Put into Host Mode

		memcpy(Poll, "\x18\x1bJHOST1\r", 9);

		TNC->TXLen = 9;
		WriteCommBlock(TNC);

		TNC->ReinitState = 5;
		return;
	}

	if (TNC->ReinitState == 5)
		TNC->ReinitState = 0;

}

static VOID DoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		ExitHost(TNC);

		return;
	}

	if (TNC->ReinitState == 1)
	{
		// No Response to trying to enter term mode - do error recovery

		Debugprintf("TRK - Starting Resync");

		TNC->ReinitState = 10;
		TNC->ReinitCount = 256;
		TNC->HostMode = TRUE;			// Must be in Host Mode if we need recovery

		Poll[0] = 1;
		TNC->TXLen = 1;
		WriteCommBlock(TNC);
		TNC->Timeout = 10;				// 2 secs

		return;
	}

	if (TNC->ReinitState == 10)
	{
		// Continue error recovery

		TNC->ReinitCount--;

		if (TNC->ReinitCount)
		{
			Poll[0] = 1;
			TNC->TXLen = 1;
			WriteCommBlock(TNC);
			TNC->Timeout = 2;				// 1/2 secs

			return;
		}

		// Try Again

		Debugprintf("TRK Continuing recovery");
		
		TNC->ReinitState = 1;
		ExitHost(TNC);

		return;
	}
	if (TNC->ReinitState == 3)
	{
		// Entering Host Mode
	
		// Assume ok

		TNC->HostMode = TRUE;
		TNC->IntCmdDelay = 10;

		return;
	}
}


//#include "Mmsystem.h"

static VOID ExitHost(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	// Try to exit Host Mode

	TNC->TXBuffer[0] = 1;
	TNC->TXBuffer[1] = 1;
	TNC->TXBuffer[2] = 1;
	memcpy(&TNC->TXBuffer[3], "%R", 2);

	StuffAndSend(TNC, Poll, 5);

	return;
}

VOID StuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	TNC->TXLen = Len;
	WriteCommBlock(TNC);
}

static VOID ProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;
	}

	if (TNC->ReinitState == 1)
	{
		// trying to set term mode

		// If already in Term Mode, TNC echos command, with control chars replaced with '.'

		if (memcmp(TNC->RXBuffer, "....%R", 6) == 0)
		{
			// In term mode, Need to put into Host Mode

			TNC->ReinitState = 2;
			DoTNCReinit(TNC);
			return;
		}
	}

	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

		DoTNCReinit(TNC);		// Send Next Command
		return;
	}

	if (TNC->ReinitState == 5)	// Waiting for response to JHOST1
	{
		if (TNC->RXBuffer[TNC->RXLen-1] == 10 || TNC->RXBuffer[TNC->RXLen-1] == 13)	// NewLine
		{
			TNC->HostMode = TRUE;
			TNC->Timeout = 0;
		}
		return;
	}
}

static VOID ProcessDEDFrame(struct TNCINFO * TNC)
{
	UINT * buffptr;
	char * Buffer;				// Data portion of frame
	char Status[80];
	UINT Stream = 0;
	UCHAR * Msg = TNC->DEDBuffer;
	int framelen = TNC->InputLen;

	if (TNC->ReinitState == 10)
	{
		// Recovering from Sync Failure

		// Any Response indicates we are in host mode, and back in sync

		TNC->HostMode = TRUE;
		TNC->Timeout = 0;
		TNC->ReinitState = 0;
		TNC->RXLen = 0;
		TNC->HOSTSTATE = 0;

		Debugprintf("TRK - Resync Complete");
		return;
	}

	// Any valid frame is an ACK

	TNC->Timeout = 0;

	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		TNC->TNCOK = TRUE;
		wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetWindowText(TNC->xIDC_COMMSSTATE, Status);
	}

	if (TNC->InitPtr)					// Response to Init Script
		return;

	if (TNC->MSGCHANNEL > 26)
		return;

	Stream = TNC->MSGCHANNEL - 1;

	//	See if Poll Reply or Data
	
	if (TNC->MSGTYPE == 0)
	{
		// Success - Nothing Follows

		if (Stream < 32)
			if (TNC->Streams[Stream].CmdSet)
				return;						// Response to Command Set or Init Script

		if ((TNC->TXBuffer[1] & 1) == 0)	// Data
			return;

		// If the response to a Command, then we should convert to a text "Ok" for forward scripts, etc

		if (TNC->TXBuffer[3] == 'G')	// Poll
			return;

		if (TNC->TXBuffer[3] == 'C')	// Connect - reply we need is async
			return;

		if (TNC->TXBuffer[3] == 'L')	// Shouldnt happen!
			return;


		if (TNC->TXBuffer[3] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[8] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return;
			}
		}

		if (TNC->MSGCHANNEL == 0)			// Unproto Channel
			return;

		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"TRK} Ok\r");

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (TNC->MSGTYPE > 0 &&TNC->MSGTYPE < 6)
	{
		// Success with message - null terminated

		UCHAR * ptr;
		int len;

		Buffer = Msg;
		
		ptr = strchr(Buffer, 0);

		if (ptr == 0)
			return;

		*(ptr++) = 13;
		*(ptr) = 0;

		len = ptr - Buffer;

		if (len > 256)
			return;

		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		if (TNC->MSGTYPE < 3)						// 1 or 2 - Success or Fail
		{
			// See if a response to internal command

			if (TNC->InternalCmd)
			{
				// Process it

				char LastCmd = TNC->TXBuffer[3];

				if (LastCmd == 'L')		// Status
				{
					int s1, s2, s3, s4, s5, s6, num;

					num = sscanf(Buffer, "%d %d %d %d %d %d", &s1, &s2, &s3, &s4, &s5, &s6);
			
					TNC->Streams[Stream].FramesOutstanding = s3;
					return;
				}

				if (LastCmd == '@')		// @ Commands
				{
					if (TNC->TXBuffer[4]== 'B')	// Buffer Status
					{
						TNC->Buffers = atoi(Buffer);
						SetWindowText(TNC->xIDC_BUFFERS, Buffer);
						return;
					}
				}

				if (LastCmd == '%')		// % Commands
				{
					char Status[80];
					
					if (TNC->TXBuffer[4]== 'T')	// TX count Status
					{
						wsprintf(Status, "RX %d TX %d ACKED %s", TNC->Streams[Stream].BytesRXed, TNC->Streams[Stream].BytesTXed, Buffer);
						SetWindowText(TNC->xIDC_TRAFFIC, Status);
						return;
					}

					if (TNC->TXBuffer[4] == 'W')	// Scan Control
					{
						if (Msg[4] == '1')			// Ok to Change
							TNC->OKToChangeFreq = 1;
						else
							TNC->OKToChangeFreq = -1;
					}
				}
				return;
			}
			
			// Not Internal Command, so send to user

			if (TNC->Streams[Stream].CmdSet || TNC->InitPtr)
				return;						// Response to Command Set or Init Script

		if ((TNC->TXBuffer[1] & 1) == 0)	// Data
			return;

		// If the response to a Command, then we should convert to a text "Ok" for forward scripts, etc

		if (TNC->TXBuffer[3] == 'G')	// Poll
			return;

		if (TNC->TXBuffer[3] == 'C')	// Connect - reply we need is async
			return;

		if (TNC->TXBuffer[3] == 'L')	// Shouldnt happen!
			return;

		if (TNC->TXBuffer[3] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[8] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return;
			}
		}

		if (TNC->MSGCHANNEL == 0)			// Unproto Channel
			return;

		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"TRK} %s", Buffer);

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;


		}

		if (TNC->MSGTYPE == 3)					// Status
		{			
			struct STREAMINFO * STREAM = &TNC->Streams[Stream];

			if (strstr(Buffer, "DISCONNECTED") || strstr(Buffer, "LINK FAILURE"))
			{
				if ((STREAM->Connecting | STREAM->Connected) == 0)
					return;

				if (STREAM->Connecting && STREAM->Disconnecting == FALSE)
				{
					// Connect Failed
			
					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->Streams[Stream].RemoteCall);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
	
					STREAM->Connecting = FALSE;
					STREAM->Connected = FALSE;				// In case!
					STREAM->FramesOutstanding = 0;

					if (Stream == 0)
					{
						wsprintf(Status, "In Use by %s", STREAM->MyCall);
						SetWindowText(TNC->xIDC_TNCSTATE, Status);
					}

					if (TNC->RPBEACON)
						SendRPBeacon(TNC);
				
					return;
				}
					
				// Must Have been connected or disconnecting - Release Session

				STREAM->Connecting = FALSE;
				STREAM->Connected = FALSE;		// Back to Command Mode
				STREAM->FramesOutstanding = 0;

				if (STREAM->Disconnecting == FALSE)
					STREAM->ReportDISC = TRUE;		// Tell Node

				STREAM->Disconnecting = FALSE;

				if (TNC->RPBEACON)
					SendRPBeacon(TNC);

				return;
			}

			if (strstr(Buffer, "CONNECTED"))
			{
				char * Call = strstr(Buffer, " to ");
				char * ptr;
				char MHCall[30];

				Call += 4;

				if (Call[1] == ':')
					Call +=2;

				ptr = strchr(Call, ' ');	
				if (ptr) *ptr = 0;

				ptr = strchr(Call, 13);	
				if (ptr) *ptr = 0;

				STREAM->Connected = TRUE;			// Subsequent data to data channel
				STREAM->Connecting = FALSE;
				STREAM->ConnectTime = time(NULL); 
				STREAM->BytesRXed = STREAM->BytesTXed = 0;

				//	Stop Scanner

				if (Stream == 0)
				{
					TNC->SwitchToPactor = 0;						// Cancel any RP to Pactor switch

					wsprintf(Status, "%d SCANSTOP", TNC->Port);
					Rig_Command(-1, Status);

					memcpy(MHCall, Call, 9);
					MHCall[9] = 0;
				}

				if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
				{
					// Incoming Connect

					struct APPLCALLS * APPL;
					char * ApplPtr = &APPLS;
					int App;
					char Appl[10];
					char DestCall[10];

					if (Stream == 0)
					{
						char Save = TNC->RIG->CurrentBandWidth;
						TNC->RIG->CurrentBandWidth = 'R';
						UpdateMH(TNC, MHCall, '+', 'I');
						TNC->RIG->CurrentBandWidth = Save;
					}

					ProcessIncommingConnect(TNC, Call, Stream, TRUE);

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Inbound Freq %s", STREAM->RemoteCall, TNC->NodeCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Inbound", STREAM->RemoteCall, TNC->NodeCall);
					
						SetWindowText(TNC->xIDC_TNCSTATE, Status);
					
						// If an autoconnect APPL is defined, send it
						// See which application the connect is for

						strcpy(DestCall, STREAM->MyCall);
					
						if (TNC->UseAPPLCalls && strcmp(DestCall, TNC->NodeCall) != 0)		// Not Connect to Node Call
						{
							if (strcmp(DestCall, NodeCall) == 0)		// Call to NodeCall (when not PortCall)
							{
								goto DontUseAPPLCmd;
							}

							for (App = 0; App < 32; App++)
							{
								APPL=&APPLCALLTABLE[App];
								memcpy(Appl, APPL->APPLCALL_TEXT, 10);
								ptr=strchr(Appl, ' ');

								if (ptr)
									*ptr = 0;
	
								if (_stricmp(DestCall, Appl) == 0)
									break;
							}

							if (App < 32)
							{
								char AppName[13];

								memcpy(AppName, &ApplPtr[App * 21], 12);
								AppName[12] = 0;

								// Make sure app is available

								if (CheckAppl(TNC, AppName))
								{
									int MsgLen = wsprintf(Buffer, "%s\r", AppName);
									buffptr = GetBuff();

									if (buffptr == 0) return;			// No buffers, so ignore

									buffptr[1] = MsgLen;
									memcpy(buffptr+2, Buffer, MsgLen);

									C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
									TNC->SwallowSignon = TRUE;
								}
								else
								{
									char Msg[] = "Application not available\r\n";
					
									// Send a Message, then a disconenct
					
									buffptr = GetBuff();
									if (buffptr == 0) return;			// No buffers, so ignore

									buffptr[1] = strlen(Msg);
									memcpy(&buffptr[2], Msg, strlen(Msg));
									C_Q_ADD(&STREAM->BPQtoPACTOR_Q, buffptr);

									STREAM->NeedDisc = 100;	// 10 secs
								}
								return;
							}

							// Not to a known appl - drop through to Node
						}

				//		if (TNC->UseAPPLCalls)
				//			goto DontUseAPPLCmd;
	
						if (TNC->ApplCmd)	
						{
							buffptr = GetBuff();
							if (buffptr == 0) return;			// No buffers, so ignore

							buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
							C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
							TNC->SwallowSignon = TRUE;
							return;
						}

					}	// End of Stream 0 or RP or Drop through from not APPL Connect
				
				DontUseAPPLCmd:

					if (FULL_CTEXT && HFCTEXTLEN == 0)
					{
						int Len = CTEXTLEN, CTPaclen = 100;
						int Next = 0;

						while (Len > CTPaclen)		// CTEXT Paclen
						{
							buffptr = GetBuff();
							if (buffptr == 0) return;			// No buffers, so ignore

							buffptr[1] = CTPaclen;
							memcpy(&buffptr[2], &CTEXTMSG[Next], CTPaclen);
							C_Q_ADD(&STREAM->BPQtoPACTOR_Q, buffptr);

							Next += CTPaclen;
							Len -= CTPaclen;
						}

						buffptr = GetBuff();
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = Len;
						memcpy(&buffptr[2], &CTEXTMSG[Next], Len);
						C_Q_ADD(&STREAM->BPQtoPACTOR_Q, buffptr);
					}

					return;
				}
				else
				{
					// Connect Complete
			
					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", Call);;

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Outbound Freq %s", STREAM->MyCall, STREAM->RemoteCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Outbound", STREAM->MyCall, STREAM->RemoteCall);

						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

						if (STREAM->DEDStream == 30)	// Robust Mode
						{
							char Save = TNC->RIG->CurrentBandWidth;
							TNC->RIG->CurrentBandWidth = 'R';
							UpdateMH(TNC, Call, '+', 'O');
							TNC->RIG->CurrentBandWidth = Save;
						}
						else
						{
							UpdateMH(TNC, Call, '+', 'O');
						}
					}
					return;
				}
			}
			return;
		}

		if (TNC->MSGTYPE == 4 || TNC->MSGTYPE == 5)
		{
			struct STREAMINFO * STREAM = &TNC->Streams[0];		// RP Stream

			// Monitor

			if (TNC->UseAPPLCalls && strstr(&Msg[4], "SABM") && STREAM->Attached == FALSE)
			{
				// See if a call to Nodecall or one of our APPLCALLS - if so, stop scan and switch MYCALL

				char DestCall[10] = "NOCALL  ";
				char * ptr1 = strstr(&Msg[7], "to ");
				int i;
				struct APPLCALLS * APPL;
				char Appl[11];
				char Status[80];

				if (ptr1) memcpy(DestCall, &ptr1[3], 10);
				
				ptr1 = strchr(DestCall, ' ');
				if (ptr1) *(ptr1) = 0;					// Null Terminate

				Debugprintf("RP SABM Received for %s" , DestCall);

				if (strcmp(TNC->NodeCall, DestCall) != 0)
				{
					// Not Calling NodeCall/Portcall

					if (strcmp(NodeCall, DestCall) == 0)
						goto SetThisCall;

					// See if to one of our ApplCalls

					for (i = 0; i < 32; i++)
					{
						APPL=&APPLCALLTABLE[i];

						if (APPL->APPLCALL_TEXT[0] > ' ')
						{
							char * ptr;
							memcpy(Appl, APPL->APPLCALL_TEXT, 10);
							ptr=strchr(Appl, ' ');

							if (ptr) *ptr = 0;

							if (strcmp(Appl, DestCall) == 0)
							{
						SetThisCall:
								Debugprintf("RP SABM is for NODECALL or one of our APPLCalls - setting MYCALL to %s and pausing scan", DestCall);

								wsprintf(Status, "%d SCANSTART 60", TNC->Port);	// Pause scan for 60 secs
								Rig_Command(-1, Status);
								
								if (RigConfigMsg[TNC->Port] == NULL && TNC->RobustTime)
									TNC->SwitchToPactor = 600;		// Don't change modes for 60 secs

								strcpy(STREAM->MyCall, DestCall);
								STREAM->CmdSet = STREAM->CmdSave = zalloc(100);
								wsprintf(STREAM->CmdSet, "\1\1\1I%s\0", DestCall);
								break;
							}
						}
					}
				}
			}

			DoMonitorHddr(TNC, Msg, framelen, TNC->MSGTYPE);
			return;

		}

		// 1, 2, 4, 5 - pass to Appl

		if (TNC->MSGCHANNEL == 0)			// Unproto Channel
			return;

		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"TRK} %s", &Msg[4]);

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (TNC->MSGTYPE == 6)
	{
		// Monitor Data With length)

		DoMonitorData(TNC, Msg, framelen);
		return;
	}

	if (TNC->MSGTYPE == 7)
	{
		//char StatusMsg[60];
		//int Status, ISS, Offset;
		
		// Connected Data
		
		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = framelen;				// Length
		TNC->Streams[Stream].BytesRXed += buffptr[1];
		memcpy(&buffptr[2], Msg, buffptr[1]);
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		ShowTraffic(TNC);

		return;
	}
}

#pragma pack(1) 

typedef struct _MESSAGEY
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGEY * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID; 

	union 
	{                   /*  array named screen */
		UCHAR L2DATA[256];
		struct _L3MESSAGE L3MSG;

	};

	char Padding[100];

}MESSAGEY;

#pragma pack() 

static MESSAGEY Monframe;		// I frames come in two parts.

#define TIMESTAMP 352

MESSAGEY * AdjMsg;		// Adjusted fir digis


VOID DoMonitorHddr(struct TNCINFO * TNC, UCHAR * Msg, int Len, int Type)
{
	// Convert to ax.25 form and pass to monitor

	// Only update MH on UI, SABM, UA

	UCHAR * ptr, * starptr;
	char * context;
	char MHCall[11];

	Monframe.LENGTH = 23;				// Control Frame
	Monframe.PORT = TNC->Port;
	
	AdjMsg = &Monframe;					// Adjusted fir digis
	ptr = strstr(Msg, "fm ");

	ConvToAX25(&ptr[3], Monframe.ORIGIN);

	memcpy(MHCall, &ptr[3], 11);
	strlop(MHCall, ' ');

	ptr = strstr(ptr, "to ");

	ConvToAX25(&ptr[3], Monframe.DEST);

	ptr = strstr(ptr, "via ");

	if (ptr)
	{
		// We have digis

		char Save[100];

		memcpy(Save, &ptr[4], 60);

		ptr = strtok_s(Save, " ", &context);
DigiLoop:
		_asm 
		{	
			mov eax,AdjMsg 
			add eax, 7
			mov	AdjMsg, eax
		}

		Monframe.LENGTH += 7;

		starptr = strchr(ptr, '*');
		if (starptr)
			*(starptr) = 0;

		ConvToAX25(ptr, AdjMsg->ORIGIN);

		if (starptr)
			AdjMsg->ORIGIN[6] |= 0x80;				// Set end of address

		ptr = strtok_s(NULL, " ", &context);

		if (memcmp(ptr, "ctl", 3))
			goto DigiLoop;
	}
	AdjMsg->ORIGIN[6] |= 1;				// Set end of address

	ptr = strstr(Msg, "ctl ");

	if (memcmp(&ptr[4], "SABM", 4) == 0)
	{
		AdjMsg->CTL = 0x2f;
		if (TNC->Robust)
			UpdateMH(TNC, MHCall, '.', 0);
		else
			UpdateMH(TNC, MHCall, ' ', 0);
	}
	else  
	if (memcmp(&ptr[4], "DISC", 4) == 0)
		AdjMsg->CTL = 0x43;
	else 
	if (memcmp(&ptr[4], "UA", 2) == 0)
	{
		AdjMsg->CTL = 0x63;
		if (TNC->Robust)
			UpdateMH(TNC, MHCall, '.', 0);
		else
			UpdateMH(TNC, MHCall, ' ', 0);
	}
	else  
	if (memcmp(&ptr[4], "DM", 2) == 0)
		AdjMsg->CTL = 0x0f;
	else 
	if (memcmp(&ptr[4], "UI", 2) == 0)
	{
		AdjMsg->CTL = 0x03;
		if (TNC->Robust)
			UpdateMH(TNC, MHCall, '.', 0);
		else
			UpdateMH(TNC, MHCall, ' ', 0);
	}
	else 
	if (memcmp(&ptr[4], "RR", 2) == 0)
		AdjMsg->CTL = 0x1 | (ptr[6] << 5);
	else 
	if (memcmp(&ptr[4], "RNR", 3) == 0)
		AdjMsg->CTL = 0x5 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "REJ", 3) == 0)
		AdjMsg->CTL = 0x9 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "FRMR", 4) == 0)
		AdjMsg->CTL = 0x87;
	else  
	if (ptr[4] == 'I')
	{
		AdjMsg->CTL = (ptr[5] << 5) | (ptr[6] & 7) << 1 ;
	}

	if (strchr(&ptr[4], '+'))
	{
		AdjMsg->CTL |= 0x10;
		Monframe.DEST[6] |= 0x80;				// SET COMMAND
	}

	if (strchr(&ptr[4], '-'))	
	{
		AdjMsg->CTL |= 0x10;
		Monframe.ORIGIN[6] |= 0x80;				// SET COMMAND
	}

	if (Type == 5)								// More to come
	{
		ptr = strstr(ptr, "pid ");	
		sscanf(&ptr[3], "%x", &AdjMsg->PID);
		return;	
	}

	_asm {

	pushad

	mov edi, offset Monframe

	push	ecx
	push	edx
	
	push	0
	call	time

	add	esp,4
	
	pop		edx
	pop		ecx

	MOV	TIMESTAMP[EDI],EAX

	mov eax, BPQTRACE
	
	call eax

	popad

	}

// Message in EDI

}

VOID DoMonitorData(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	// // Second part of I or UI

	memcpy(AdjMsg->L2DATA, Msg, Len);
	Monframe.LENGTH += Len;

	_asm {

		pushad

		mov edi, offset Monframe
		
		push	ecx
		push	edx
	
		push	0
		call	time

		add	esp,4
	
		pop		edx
		pop		ecx

		MOV	TIMESTAMP[EDI],EAX

		mov eax, BPQTRACE
		call eax

		popad
	}

	return;
}


//1:fm G8BPQ to KD6PGI-1 ctl I11^ pid F0
//fm KD6PGI-1 to G8BPQ ctl DISC+

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// Queue it as we may have just sent data

	TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = zalloc(100);
	wsprintf(TNC->Streams[Stream].CmdSet, "\1\1\1D\0");
}


VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	TidyClose(TNC, Stream);			// I don't think Hostmode has a DD
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	char Status[80];

	TNC->NeedPACTOR = 20;		// Delay a bit for UA to be sent before changing mode and call
	
	wsprintf(Status, "%d SCANSTART 15", TNC->Port);
	
	Rig_Command(-1, Status);

	if (TNC->RIG == &TNC->DummyRig)		// Not using Rig control
		TNC->SwitchToPactor = TNC->RobustTime;
}

VOID SwitchToRPacket(struct TNCINFO * TNC, char * Baud)
{
	if (TNC->Robust == FALSE)
	{
		TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = zalloc(100);
		wsprintf(TNC->Streams[0].CmdSet, "\1\1\1%%B %s\0", Baud);
		TNC->Robust = TRUE;
		SetWindowText(TNC->xIDC_MODE, "Robust Packet");
	}
}
VOID SwitchToNormPacket(struct TNCINFO * TNC, char * Baud)
{
	if (TNC->ForceRobust)
		return;
	
	TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = zalloc(100);
		
	if (Baud[0] == 0)
		wsprintf(TNC->Streams[0].CmdSet, "\1\1\1%%B %s\0", TNC->NormSpeed);
	else
		wsprintf(TNC->Streams[0].CmdSet, "\1\1\1%%B %s\0", Baud);
			
	TNC->Robust = FALSE;

	SetWindowText(TNC->xIDC_MODE, "HF Packet");
}

VOID SendRPBeacon(struct TNCINFO * TNC)
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char BEACONMSG[80];

	int DataLen = wsprintf(BEACONMSG, "QRA %s %s", TNC->NodeCall, LOC);

	// Block includes the Msg Header (7 bytes), Len Does not!

	ConvToAX25("BEACON", AXPTR->DEST);
	ConvToAX25(TNC->NodeCall, AXPTR->ORIGIN);

	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;					//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, BEACONMSG, DataLen);

	Send_AX(&AXMSG, DataLen + 16, TNC->Port);
	return;

}



