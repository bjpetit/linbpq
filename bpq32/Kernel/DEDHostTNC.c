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

#define MaxStreams 10	

#include "TNCInfo.h"

#include "ASMStrucs.h"

#include "bpq32.h"

static char ClassName[]="PACTORSTATUS";
static char WindowTitle[] = "DED Host";
static int RigControlRow = 210;

#define NARROWMODE 12
#define WIDEMODE 16			// PIII only

extern UCHAR BPQDirectory[];

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];

static RECT Rect;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

VOID __cdecl Debugprintf(const char * format, ...);

char NodeCall[11];		// Nodecall, Null Terminated

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

static ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	ptr = strtok(NULL, " \t\n\r");

	if (_stricmp(buf, "ADDR") == 0)			// Winmor Using BPQ32 COnfig
	{
		BPQport = Port;
		p_ipad = ptr;
	}
	else
	if (_stricmp(buf, "APPL") == 0)			// Using BPQ32 COnfig
	{
		BPQport = Port;
		p_cmd = ptr;
	}
	else
	if (_stricmp(buf, "PORT") != 0)			// Using Old Config
	{
		// New config without a PORT or APPL  - this is a Config Command

		strcpy(buf, errbuf);
		strcat(buf, "\r");

		BPQport = Port;

		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

		TNC->InitScript = malloc(1000);
		TNC->InitScript[0] = 0;
		goto ConfigLine;
	}
	else

	{

		// Old Config from file

	BPQport=0;
	BPQport = atoi(ptr);
	
	p_cmd = strtok(NULL, " \t\n\r");

	if (Port && Port != BPQport)
	{
		// Want a particular port, and this isn't it

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

		}
	}
	}
	if(BPQport > 0 && BPQport < 33)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

		TNC->InitScript = malloc(1000);
		TNC->InitScript[0] = 0;

		if (p_cmd != NULL)
		{
			if (p_cmd[0] != ';' && p_cmd[0] != '#')
				TNC->ApplCmd=_strdup(p_cmd);
		}

		// Read Initialisation lines

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

			if (_memicmp(buf, "RIGCONTROL", 10) == 0)
			{
				char * vcom = strstr(buf, "VCOM");
				if (vcom)
	
					// SCS Virtual COM Channel

					TNC->VCOMPort = atoi(&vcom[4]);		

				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				TNC->RigConfigMsg = _strdup(buf);
			}
			else
				
			if (_memicmp(buf, "PACKETCHANNELS", 14) == 0)
	
				// Packet Channels

				TNC->PacketChannels = atoi(&buf[14]);
			else

			if (_memicmp(buf, "SCANFORROBUSTPACKET", 19) == 0)
			{
				// Spend a percentage of scan time in Robust Packet Mode

				double Robust = atof(&buf[19]);
				TNC->RobustTime = Robust * 10;
			}

			if (_memicmp(buf, "USEAPPLCALLS", 12) == 0)
				TNC->UseAPPLCalls = TRUE;

			else

			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
				DecodeWL2KReportLine(TNC, buf, NARROWMODE, WIDEMODE);
			else
				strcat (TNC->InitScript, buf);

		}
	}

	return (TRUE);
	
}

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
VOID DEDPoll(int Port);
VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len);
unsigned short int compute_crc(unsigned char *buf,int len);
int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID ExitHost(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);
BOOL OpenVirtualSerialPort(struct TNCINFO * TNC);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);
Switchmode(struct TNCINFO * TNC, int Mode);
VOID SwitchToPactor(struct TNCINFO * TNC);
VOID SwitchToPacket(struct TNCINFO * TNC);

extern UCHAR NEXTID;
extern  struct TRANSPORTENTRY * L4TABLE;
extern  WORD MAXCIRCUITS;
extern  UCHAR L4DEFAULTWINDOW;
extern  WORD L4T1;

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

static char status[8][8] = {"ERROR",  "REQUEST", "TRAFFIC", "IDLE", "OVER", "PHASE", "SYNCH", ""};

static char ModeText[8][14] = {"STANDBY", "AMTOR-ARQ",  "PACTOR-ARQ", "AMTOR-FEC", "PACTOR-FEC", "RTTY / CW", "LISTEN", "Channel-Busy"};

static char PactorLevelText[4][14] = {"Not Connected", "PACTOR-I", "PACTOR-II", "PACTOR-III"};

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int txlen = 0;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	int Param;
	int Stream = 0;
	struct STREAMINFO * STREAM;


	if (TNC == NULL || TNC->hDevice == (HANDLE) -1)
		return 0;			// Port not open

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

	
		CheckRX(TNC);
		DEDPoll(port);

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

		STREAM = &TNC->Streams[Stream];

		if (Stream == 0)
		{
			if (STREAM->FramesOutstanding  > 4)
				return (1 | TNC->HostMode << 8 | STREAM->Disconnecting << 15);
		}
		else
		{
			if (STREAM->FramesOutstanding > 3 || TNC->Buffers < 200)	
				return (1 | TNC->HostMode << 8 | STREAM->Disconnecting << 15);		}

		return TNC->HostMode << 8 | STREAM->Disconnecting << 15;		// OK, but lock attach if disconnecting


	case 4:				// reinit

		return (0);

	case 5:				// Close

		// Ensure in Pactor

		TNC->TXBuffer[2] = 31;
		TNC->TXBuffer[3] = 0x1;
		TNC->TXBuffer[4] = 0x1;
		memcpy(&TNC->TXBuffer[5], "PT", 2);

		CRCStuffAndSend(TNC, TNC->TXBuffer, 7);

		Sleep(25);

		ExitHost(TNC);

		Sleep(25);

		CloseHandle(TNCInfo[port]->hDevice);

		if (TNC->VCOMHandle)
			CloseHandle(TNC->VCOMHandle);
				
		SaveWindowPos(port);

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
				TNC->OKToChangeFreq = FALSE;
				return TRUE;
			}
			return 0;		// Don't lock scan if TNC isn't reponding
		

		case 2:		// Check  Permission
			return TNC->OKToChangeFreq;

		case 3:		// Release  Permission
		
			TNC->WantToChangeFreq = FALSE;
			TNC->DontWantToChangeFreq = TRUE;
			return 0;
		
		case 4:		// Set Wide Mode
		
			if (TNC->Bandwidth != 'W')
			{
				TNC->Bandwidth = 'W';
				Switchmode(TNC, 3);
			}

			if (TNC->RobustTime)
				SwitchToPacket(TNC);			// Always start in packet, switch to pactor after RobustTime ticks
			
			return 0;
		

		case 5:		// Set Narrow Mode
		

			if (TNC->Bandwidth != 'N')
			{
				TNC->Bandwidth = 'N';
				Switchmode(TNC, 2);
			}

			if (TNC->RobustTime)
				SwitchToPacket(TNC);			// Always start in packet, switch to pactor after RobustTime ticks

			return 0;

		case 6:		// Changing Freq (Called if changing freq but not bandwidth

			if (TNC->RobustTime)
				SwitchToPacket(TNC);			// Always start in packet, switch to pactor after RobustTime ticks
		}

	}
	return 0;
}

UINT WINAPI DEDExtInit(EXTPORTDATA *  PortEntry)
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

	wsprintf(msg,"DEDHOST COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile("", port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in BPQ32.cfg for this port");
		WritetoConsole(msg);

		return (int) ExtProc;
	}
	
	TNC->Port = port;
	TNC->Hardware = H_SCS;

	if (TNC->RigConfigMsg)
	{
		char * SaveRigConfig = _strdup(TNC->RigConfigMsg);

		TNC->RIG = RigConfig(TNC->RigConfigMsg, port);
			
		if (TNC->RIG == NULL)
		{
			// Report Error

			wsprintf(msg,"Invalid Rig Config %s", SaveRigConfig);
			WritetoConsole(msg);

		}
		free(SaveRigConfig);
	}


	// Set up DED addresses for streams (first stream (Pactor) = DED 31
	
	TNC->Streams[0].DEDStream = 31;

	for (Stream = 1; Stream <= MaxStreams; Stream++)
	{
		TNC->Streams[Stream].DEDStream = Stream;
	}

	if (TNC->PacketChannels > MaxStreams)
		TNC->PacketChannels = MaxStreams;

	PortEntry->MAXHOSTMODESESSIONS = TNC->PacketChannels + 1;
	PortEntry->PERMITGATEWAY = TRUE;					// Can change ax.25 call on each stream
	PortEntry->SCANCAPABILITIES = CONLOCK;				// Scan Control 3 stage/conlock 

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
		
	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 100;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// get NODECALL for RP tests

	memcpy(NodeCall, GetNodeCall(), 10);
		
	ptr=strchr(NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate


	// Set TONES to 4

	TempScript = malloc(1000);

	strcpy(TempScript, "EXIT\r");				// In case in pac: mode
	strcat(TempScript, "TONES 4\r");			// Tones may be changed but I want this as standard
	strcat(TempScript, "MAXERR 30\r");			// Max retries 
	strcat(TempScript, "MODE 0\r");				// ASCII mode, no PTC II compression (Forwarding will use FBB Compression)
	strcat(TempScript, "MAXSUM 20\r");			// Max count for memory ARQ
	strcat(TempScript, "CWID 0 2\r");			// CW ID disabled

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Others go on end so they can't be overriden

	strcat(TNC->InitScript, "ADDLF 0\r");      //      Auto Line Feed disabled
	strcat(TNC->InitScript, "ARX 0\r");      //        Amtor Phasing disabled
	strcat(TNC->InitScript, "BELL 0\r");      //       Disable Bell
	strcat(TNC->InitScript, "BC 0\r");      //         FEC reception is disabled
	strcat(TNC->InitScript, "BKCHR 2\r");      //      Breakin Char = 2
	strcat(TNC->InitScript, "CHOBELL 0\r");      //    Changeover Bell off
	strcat(TNC->InitScript, "CMSG 0\r");      //       Connect Message Off
	strcat(TNC->InitScript, "LFIGNORE 0\r");      //   No insertion of Line feed
	strcat(TNC->InitScript, "LISTEN 0\r");      //     Pactor Listen disabled
	strcat(TNC->InitScript, "MAIL 0\r");      //       Disable internal mailbox reporting
	strcat(TNC->InitScript, "REMOTE 0\r");      //     Disable remote control
	strcat(TNC->InitScript, "PAC CBELL 0\r");      //  
	strcat(TNC->InitScript, "PAC CMSG 0\r");      //  
	strcat(TNC->InitScript, "PAC PRBOX 0\r");      //  	Turn off Packet Radio Mailbox
	
	//  Automatic Status must be enabled for BPQ32
	//  Pactor must use Host Mode Chanel 31
	//  PDuplex must be set. The Node code relies on automatic IRS/ISS changeover
	//	5 second duplex timer

	strcat(TNC->InitScript, "STATUS 2\rPTCHN 31\rPDUPLEX 1\rPDTIMER 5\r");

	wsprintf(msg, "MYCALL %s\rPAC MYCALL %s\r", TNC->NodeCall, TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 0);

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

	if (TNC->VCOMPort)
		OpenVirtualSerialPort(TNC);

	return ((int)ExtProc);
}

static void CheckRX(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length, Len;
	UCHAR  * ptr;
	char character;
	UCHAR * CURSOR;

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

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

	// DED mode doesn't have an end of frame delimiter, so we need to konw msg type and lengh to know
	// that he have a full frame
	
	ptr = TNC->RXBuffer;

	CURSOR = &TNC->DEDBuffer[TNC->InputLen];
	
	while (Length--)
	{
		character = *(ptr++);

		if (TNC->HostMode)
		{
			switch(TNC->HOSTSTATE)
			{
			case 0: 	//  SETCHANNEL

				TNC->MSGCHANNEL = character;
				break;

			case 1:		//	SETMSGTYPE

				TNC->MSGTYPE = character;
				break;

			case 2:		//  SETLENGTH

				TNC->MSGCOUNT = TNC->MSGLENGTH = ++character;
				CURSOR = &TNC->DEDBuffer[0];

				break;

			default:

			//	RECEIVING COMMAND/DATA

			*(CURSOR++) = character;
			TNC->MSGCOUNT--;

			if (TNC->MSGCOUNT)
				continue;			// MORE TO COME

			ProcessDEDFrame(TNC);

			TNC->HOSTSTATE = 0;

			TNC->InputLen = 0;
			}
		}
	}

	// End of Input - Save buffer position

	TNC->InputLen = CURSOR - TNC->DEDBuffer;
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

	if (TNC->UpdateWL2K)
	{
		TNC->UpdateWL2KTimer--;

		if (TNC->UpdateWL2KTimer == 0)
		{
			TNC->UpdateWL2KTimer = 32910;		// Every Hour
		
			if (TNC->ApplCmd)
			{
				if (memcmp(TNC->ApplCmd, "RMS", 3) == 0)
				{
					char Appl[30];
					
					strcpy(Appl, TNC->ApplCmd);
					strcat (Appl, "          ");
					
					if (CheckAppl(TNC, Appl)) // Is RMS Available?
					{
						memcpy(TNC->RMSCall, TNC->NodeCall, 9);	// Should report Port/Node Call
						SendReporttoWL2K(TNC);
					}
				}
			}
		}
	}

	if (TNC->SwitchToPactor)
	{
		TNC->SwitchToPactor--;
	
		if (TNC->SwitchToPactor == 0)
			SwitchToPactor(TNC);
	}
		


	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			// New Attach

			// If Pactor, stop scanning and take out of listen mode.

			// Set call to connecting user's call

			// If Stream 0 Put in Pactor Mode so Busy Detect will work

			int calllen=0;

			TNC->Streams[Stream].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
			TNC->Streams[Stream].MyCall[calllen] = 0;

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->Streams[Stream].MyCall);

			if (Stream == 0)
			{
				wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

				wsprintf(TNC->Streams[Stream].CmdSet, "I%s\rPT\r", TNC->Streams[Stream].MyCall);

				// Stop Scanner
		
				wsprintf(Status, "%d SCANSTOP", TNC->Port);
				TNC->SwitchToPactor = 0;						// Cancel any RP to Pactor switch
		
				Rig_Command(-1, Status);
			}
		}
	}

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		TNC->Retries--;

		if(TNC->Retries)
		{
			WriteCommBlock(TNC);	// Retransmit Block
			return;
		}

		// Retried out.

		if (TNC->HostMode == 0)
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Retried out in host mode - Clear any connection and reinit the TNC

		Debugprintf("PACTOR - Link to TNC Lost");
		TNC->TNCOK = FALSE;

		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		TNC->HostMode = 0;
		TNC->ReinitState = 0;
		
		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			}
		}
	}

	// We delay clearing busy for 3 secs

	if (TNC->Busy)
		if (TNC->Mode != 7)
			TNC->Busy--;

	if (TNC->BusyDelay)		// Waiting to send connect
	{
		// Still Busy?

		if (TNC->Busy == 0)
		{
			// No, so send

			TNC->Streams[0].CmdSet = TNC->ConnectCmd;
			TNC->Streams[0].Connecting = TRUE;

			wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			TNC->BusyDelay = 0;
			return;
		}
		else
		{
			// Wait Longer

			TNC->BusyDelay--;

			if (TNC->BusyDelay == 0)
			{
				// Timed out - Send Error Response

				UINT * buffptr = GetBuff();

				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]=39;
				memcpy(buffptr+2,"Sorry, Can't Connect - Channel is busy\r", 39);

				C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);

				free(TNC->ConnectCmd);

			}
		}
	}

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		STREAM = &TNC->Streams[Stream];

		if (STREAM->Attached)
			CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

		if (TNC->Timeout)
			return;				// We've sent something
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	TNC->PollSent = FALSE;

	//If sending internal command list, send next element

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->Streams[Stream].CmdSet)
		{
			char * start, * end;
			int len;

			start = TNC->Streams[Stream].CmdSet;
		
			if (*(start) == 0)			// End of Script
			{
				free(TNC->Streams[Stream].CmdSave);
				TNC->Streams[Stream].CmdSet = NULL;
			}
			else
			{
				end = strchr(start, 13);
				len = ++end - start -1;	// exclude cr
				TNC->Streams[Stream].CmdSet = end;

				Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel
				Poll[3] = 1;			// Command
				Poll[4] = len - 1;
				memcpy(&Poll[5], start, len);
		
				CRCStuffAndSend(TNC, Poll, len + 5);

				return;
			}
		}
	}
	// if Freq Change needed, check if ok to do it.
	
	if (TNC->TNCOK)
	{
		if (TNC->WantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '0';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->WantToChangeFreq = FALSE;

			return;
		}

		if (TNC->DontWantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '1';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->DontWantToChangeFreq = FALSE;
			TNC->OKToChangeFreq = FALSE;

			return;
		}
	}

	// Send Radio Command if avail

	if (TNC->TNCOK && TNC->BPQtoRadio_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&TNC->BPQtoRadio_Q);

		datalen=buffptr[1];

		Poll[2] = 253;		// Radio Channel
		Poll[3] = 0;		// Data?
		Poll[4] = datalen - 1;
	
		memcpy(&Poll[5], buffptr+2, datalen);
		
		ReleaseBuffer(buffptr);
		
		CRCStuffAndSend(TNC, Poll, datalen + 5);

//		Debugprintf("SCS Sending Rig Command");

		return;
	}

		// Check status Periodically
		
	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 6)
		{
			Poll[2] = 254;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = 'G';			// Extended Status Poll
			Poll[6] = '3';

			CRCStuffAndSend(TNC, Poll, 7);
						
			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay == 4)
		{
			Poll[2] = 31;			 // Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '%';			// Bytes acked Status
			Poll[6] = 'T';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			Poll[2] = 31;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '@';			// Buffer Status
			Poll[6] = 'B';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay = 20;	// Every 2 secs

			return;
		}
		else
			TNC->IntCmdDelay--;
	}

	// If busy, send status poll, send Data if avail

	// We need to start where we last left off, or a busy stream will lock out the others

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

			Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel

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

				Poll[3] = 0;			// Data?
				TNC->Streams[Stream].BytesTXed += datalen;

				Poll[4] = datalen - 1;
				memcpy(&Poll[5], buffptr+2, datalen);
		
				ReleaseBuffer(buffptr);
		
				CRCStuffAndSend(TNC, Poll, datalen + 5);

				TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

				if (STREAM->Disconnecting && TNC->Streams[Stream].BPQtoPACTOR_Q == 0)
					TidyClose(TNC, 0);

				return;
			}
			
			// Command. Do some sanity checking and look for things to process locally

			Poll[3] = 1;			// Command
			datalen--;				// Exclude CR
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

				if (memcmp(Buffer, "MYLEVEL ", 8) == 0)
				{
					Switchmode(TNC, Buffer[8] - '0');

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Ok\r");		
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					return;
				}

				if (_memicmp(Buffer, "OVERRIDEBUSY", 12) == 0)
				{
					TNC->OverrideBusy = TRUE;

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "SCS} OK\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					return;
				}

				if ((Stream == 0) && memcmp(Buffer, "RPACKET", 7) == 0)
				{
					TNC->HFPacket = TRUE;
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "SCS} OK\r");
					C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
					return;
				}

				if ((Stream == 0) && memcmp(Buffer, "PACTOR", 6) == 0)
				{
					TNC->HFPacket = FALSE;
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "SCS} OK\r");
					C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
					return;
				}

				if (Stream == 0 && Buffer[0] == 'C' && datalen > 2)	    // Pactor Connect
					Poll[2] = TNC->Streams[0].DEDStream = 31;			// Pactor Channel

				if (Stream == 0 && Buffer[0] == 'R' && Buffer[1] == 'C')	// Robust Packet Connect
				{
					Poll[2] = TNC->Streams[0].DEDStream = 30;			// Last Packet Channel
					memmove(Buffer, &Buffer[1], datalen--);
				}

				if (Buffer[0] == 'C' && datalen > 2)	// Connect
				{
					if (*(++Buffer) == ' ') Buffer++;		// Space isn't needed

					if ((memcmp(Buffer, "P1 ", 3) == 0) ||(memcmp(Buffer, "P2 ", 3) == 0))
					{
						// Port Selector for Packet Connect convert to 2:CALL

						Buffer[0] = Buffer[1];		
						Buffer[1] = ':';
						memmove(&Buffer[2], &Buffer[3], datalen--);
						Buffer += 2;
					}

					memcpy(TNC->Streams[Stream].RemoteCall, Buffer, 9);

					TNC->Streams[Stream].Connecting = TRUE;

					if (Stream == 0)
					{
						// Send Call, Mode Command followed by connect 

						TNC->Streams[0].CmdSet = TNC->Streams[0].CmdSave = malloc(100);

						if (TNC->Streams[0].DEDStream == 30)
							wsprintf(TNC->Streams[0].CmdSet, "I%s\rPR\r%s\r", TNC->Streams[0].MyCall, buffptr+2);
						else
							wsprintf(TNC->Streams[0].CmdSet, "I%s\rPT\r%s\r", TNC->Streams[0].MyCall, buffptr+2);

						ReleaseBuffer(buffptr);
					
						// See if Busy
				
						if (TNC->Busy)
						{
							// Channel Busy. Unless override set, wait

							if (TNC->OverrideBusy == 0)
							{
								// Send Mode Command Now, Save Command, and wait up to 10 secs
								// No, leave in Pactor, or Busy Detect won't work. Queue the whole conect sequence

								TNC->ConnectCmd = TNC->Streams[0].CmdSet;
								TNC->Streams[0].CmdSet = NULL;

								wsprintf(Status, "Waiting for clear channel");
								SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

								TNC->BusyDelay = 100;		// 10 secs
								TNC->Streams[Stream].Connecting = FALSE;	// Not connecting Yet

								return;
							}
						}

						TNC->OverrideBusy = FALSE;

						wsprintf(Status, "%s Connecting to %s", TNC->Streams[Stream].MyCall, TNC->Streams[Stream].RemoteCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

						TNC->Streams[0].InternalCmd = FALSE;
						return;
					}
				}

			Poll[4] = datalen - 1;
			memcpy(&Poll[5], buffptr+2, datalen);
		
			ReleaseBuffer(buffptr);
		
			CRCStuffAndSend(TNC, Poll, datalen + 5);

			TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

			return;
		}

		// if frames outstanding, issue a poll

		if (TNC->Streams[Stream].FramesOutstanding)
		{
			Poll[2] = TNC->Streams[Stream].DEDStream;
			Poll[3] = 0x1;			// Command
			Poll[4] = 0;			// Len-1
			Poll[5] = 'L';			// Status

			CRCStuffAndSend(TNC, Poll, 6);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;
			return;
		}

	}

	TNC->PollSent = TRUE;

	// Use General Poll (255)

	Poll[2] = 255 ;			// Channel
	Poll[3] = 0x1;			// Command

	if (TNC->ReinitState == 3)
	{
		TNC->ReinitState = 0;
		Poll[3] = 0x41;
	}

	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	CRCStuffAndSend(TNC, Poll, 6);
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
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		Poll[0] = 13;
		TNC->TXLen = 1;

		WriteCommBlock(TNC);
		TNC->Retries = 1;
	}

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		char * start, * end;
		int len;

		start = TNC->InitPtr;
		
		if (*(start) == 0)			// End of Script
		{
			// Put into Host Mode

			TNC->TXBuffer[2] = 0;
			TNC->Toggle = 0;

			memcpy(Poll, "JHOST4\r", 7);

			TNC->TXLen = 7;
			WriteCommBlock(TNC);

			// Timeout will enter host mode

			TNC->Timeout = 1;
			TNC->Retries = 1;
			TNC->Toggle = 0;
			TNC->ReinitState = 3;	// Set toggle force bit

			return;
		}
		
		end = strchr(start, 13);
		len = ++end - start;
		TNC->InitPtr = end;
		memcpy(Poll, start, len);

		TNC->TXLen = len;
		WriteCommBlock(TNC);

		TNC->Retries = 2;
	}
}

static VOID DoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		ExitHost(TNC);
		TNC->Retries = 1;

		return;
	}

	if (TNC->ReinitState == 1)
	{
		// Forcing back to Term Mode

		TNC->ReinitState = 0;
		DoTNCReinit(TNC);				// See if worked
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

static BOOL CheckRXText(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return FALSE;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return FALSE;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	TNC->RXBuffer[TNC->RXLen] = 0;

	if (strlen(TNC->RXBuffer) < TNC->RXLen)
		TNC->RXLen = 0;

	if ((strstr(TNC->RXBuffer, "cmd: ") == 0) && (strstr(TNC->RXBuffer, "pac: ") == 0))
		return 0;				// Wait for rest of frame

	// Complete Char Mode Frame

	TNC->RXLen = 0;		// Ready for next frame

	return TRUE;
					
}

static BOOL CheckRXHost(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;
	unsigned short crc;
	char UnstuffBuffer[500];

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return FALSE;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return FALSE;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	if (Length < 6)				// Minimum Frame Sise
		return FALSE;

	if (TNC->RXBuffer[2] == 170)
	{
		// Retransmit Request
	
		TNC->RXLen = 0;
		return FALSE;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice

	Length = Unstuff(&TNC->RXBuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return FALSE;				// Ignore for now
	}

	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		return TRUE;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return FALSE;
}


//#include "Mmsystem.h"

static VOID ExitHost(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	// Try to exit Host Mode

	TNC->TXBuffer[2] = 31;
	TNC->TXBuffer[3] = 0x41;
	TNC->TXBuffer[4] = 0x5;
	memcpy(&TNC->TXBuffer[5], "JHOST0", 6);

	CRCStuffAndSend(TNC, Poll, 11);

	return;
}

VOID StuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	unsigned short int crc;
	UCHAR StuffedMsg[500];
	int i, j;

    Msg[3] |= TNC->Toggle;
	TNC->Toggle ^= 0x80;

	crc = compute_crc(&Msg[2], Len-2);
	crc ^= 0xffff;

	Msg[Len++] = (crc&0xff);
	Msg[Len++] = (crc>>8);

	for (i = j = 2; i < Len; i++)
	{
		StuffedMsg[j++] = Msg[i];
		if (Msg[i] == 170)
		{
			StuffedMsg[j++] = 0;
		}
	}

	if (j != i)
	{
		Len = j;
		memcpy(Msg, StuffedMsg, j);
	}

	TNC->TXLen = Len;

	Msg[0] = 170;
	Msg[1] = 170;

	WriteCommBlock(TNC);

	TNC->Retries = 5;
}

static VOID ProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;

		// Send Restart to make sure PTC is in a known state

		strcpy(Poll, "RESTART\r");

		TNC->TXLen = 8;
		WriteCommBlock(TNC);

		TNC->Timeout = 60;				// 6 secs

		return;
	}
	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

		DoTNCReinit(TNC);		// Send Next Command
		return;
	}
}

static VOID ProcessDEDFrame(struct TNCINFO * TNC)
{
	UINT * buffptr;
	char * Buffer;				// Data portion of frame
	char Status[80];
	int Stream = 0;
	UCHAR * Msg = TNC->DEDBuffer;
	int framelen = TNC->InputLen;

	// Any valid frame is an ACK

	TNC->Timeout = 0;

	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		TNC->TNCOK = TRUE;
		wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
	}

	Stream = TNC->MSGCHANNEL;

	if (Stream > 29) Stream = 0;				// 31 = Pactor or 30 = Robust Packet

	//	See if Poll Reply or Data
	
	if (TNC->MSGTYPE == 0)
	{
		// Success - Nothing Follows

		if (Stream < 32)
			if (TNC->Streams[Stream].CmdSet)
				return;						// Response to Command Set

		if ((TNC->TXBuffer[3] & 1) == 0)	// Data
			return;

		// If the response to a Command, then we should convert to a text "Ok" for forward scripts, etc

		if (TNC->TXBuffer[5] == 'G')	// Poll
			return;

		if (TNC->TXBuffer[5] == 'C')	// Connect - reply we need is async
			return;

		if (TNC->TXBuffer[5] == 'L')	// Shouldnt happen!
			return;

		if (TNC->TXBuffer[5] == '%' && TNC->TXBuffer[6] == 'W')	// Scan Control - Response to W1
			if (TNC->InternalCmd)
				return;					// Just Ignore

		if (TNC->TXBuffer[5] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[10] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return;
			}
		}	


		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} Ok\r");

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] > 0 && Msg[3] < 6)
	{
		// Success with message - null terminated

		UCHAR * ptr;
		int len;

		if (Msg[2] == 0xff)			// General Poll Response
		{
			UCHAR * Poll = TNC->TXBuffer;
			UCHAR Chan = Msg[4] - 1;

			if (Chan == 255)
				return;				// Nothing doing

			if (Msg[5] != 0)
			{
				// More than one to poll - save the list of channels to poll

				strcpy(TNC->NexttoPoll, &Msg[5]);
			}

			// Poll the channel that had data

			Poll[2] = Chan;			// Channel
			Poll[3] = 0x1;			// Command

			if (Chan == 254)		// Status - Send Extended Status (G3)
			{
				Poll[4] = 1;			// Len-1
				Poll[5] = 'G';			// Extended Status Poll
				Poll[6] = '3';
			}
			else
			{
				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll
			}

			CRCStuffAndSend(TNC, Poll, Poll[4] + 6);
			TNC->InternalCmd = FALSE;

			return;
		}

		Buffer = &Msg[4];
		
		ptr = strchr(Buffer, 0);

		if (ptr == 0)
			return;

		*(ptr++) = 13;
		*(ptr) = 0;

		len = ptr - Buffer;

		if (len > 256)
			return;

		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		if (Msg[3] < 3)						// 1 or 2 - Success or Fail
		{
			// See if a response to internal command

			if (TNC->InternalCmd)
			{
				// Process it

				char LastCmd = TNC->TXBuffer[5];

				if (LastCmd == 'L')		// Status
				{
					int s1, s2, s3, s4, s5, s6, num;

					num = sscanf(Buffer, "%d %d %d %d %d %d", &s1, &s2, &s3, &s4, &s5, &s6);
			
					TNC->Streams[Stream].FramesOutstanding = s3;
					return;
				}

				if (LastCmd == '@')		// @ Commands
				{
					if (TNC->TXBuffer[6]== 'B')	// Buffer Status
					{
						TNC->Buffers = atoi(Buffer);
						SetDlgItemText(TNC->hDlg, IDC_BUFFERS, Buffer);
						return;
					}
				}

				if (LastCmd == '%')		// % Commands
				{
					char Status[80];
					
					if (TNC->TXBuffer[6]== 'T')	// TX count Status
					{
						wsprintf(Status, "RX %d TX %d ACKED %s", TNC->Streams[Stream].BytesRXed, TNC->Streams[Stream].BytesTXed, Buffer);
						SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
						return;
					}

					if (TNC->TXBuffer[6] == 'W')	// Scan Control
					{
						if (Msg[4] == '1')			// Ok to Change
							TNC->OKToChangeFreq = 1;
						else
							TNC->OKToChangeFreq = -1;
					}
				}
				return;
			}
		}

		if (Msg[3] == 3)					// Status
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
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}

					return;
				}
					
				// Must Have been connected or disconnecting - Release Session

				STREAM->Connecting = FALSE;
				STREAM->Connected = FALSE;		// Back to Command Mode
				STREAM->FramesOutstanding = 0;

				if (STREAM->Disconnecting == FALSE)
					STREAM->ReportDISC = TRUE;		// Tell Node

				STREAM->Disconnecting = FALSE;
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

				STREAM->BytesRXed = STREAM->BytesTXed = 0;

				//	Stop Scanner

				if (Stream == 0 || TNC->HFPacket)
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

					if (TNC->HFPacket)
					{
						char Save = TNC->RIG->CurrentBandWidth;
						TNC->RIG->CurrentBandWidth = 'R';
						UpdateMH(TNC, MHCall, '+', 'I');
						TNC->RIG->CurrentBandWidth = Save;
					}
					ProcessIncommingConnect(TNC, Call, Stream);

					if (Stream == 0 || TNC->HFPacket)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Inbound Freq %s", STREAM->RemoteCall, TNC->NodeCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Inbound", STREAM->RemoteCall, TNC->NodeCall);
					
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					
						// If an autoconnect APPL is defined, send it
						// See which application the connect is for

						strcpy(DestCall, STREAM->MyCall);
					
						if (TNC->UseAPPLCalls && strcmp(DestCall, TNC->NodeCall) != 0)		// Not Connect to Node Call
						{		
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

						if (TNC->HFPacket && TNC->UseAPPLCalls)
							goto DontUseAPPLCmd;
	
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

					if (FULL_CTEXT)
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

		if (Msg[3] == 4 || Msg[3] == 5)
		{
			struct STREAMINFO * STREAM = &TNC->Streams[1];		// RP Stream

			// Monitor

			if (TNC->HFPacket && TNC->UseAPPLCalls && strstr(&Msg[4], "SABM") && STREAM->Connected == FALSE)
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

								wsprintf(Status, "%d SCANSTART 30", TNC->Port);
								Rig_Command(-1, Status);
								TNC->SwitchToPactor = 0;		// Stay in RP

								strcpy(STREAM->MyCall, DestCall);
								STREAM->CmdSet = STREAM->CmdSave = malloc(100);
								wsprintf(STREAM->CmdSet, "I%s\r", DestCall);
								break;
							}
						}
					}
				}
			}

			DoMonitor(TNC, &Msg[3], framelen - 3);
			return;

		}

		// 1, 2, 4, 5 - pass to Appl

		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} %s", &Msg[4]);

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] == 6)
	{
		// Monitor Data With length)

		DoMonitor(TNC, &Msg[3], framelen - 3);
		return;
	}

	if (Msg[3] == 7)
	{
		char StatusMsg[60];
		int Status, ISS, Offset;
		
		if (Msg[2] == 0xfe)						// Status Poll Response
		{
			Status = Msg[5];
			
			TNC->Streams[0].PTCStatus0 = Status;
			TNC->Streams[0].PTCStatus1 = Msg[6] & 3;		// Pactor Level 1-3
			TNC->Streams[0].PTCStatus2 = Msg[7];			// Speed Level
			Offset = Msg[8];

			if (Offset > 128)
				Offset -= 128;

			TNC->Streams[0].PTCStatus3 = Offset; 

			TNC->Mode = (Status >> 4) & 7;
			ISS = Status & 8;
			Status &= 7;

			wsprintf(StatusMsg, "%x %x %x %x", TNC->Streams[0].PTCStatus0,
				TNC->Streams[0].PTCStatus1, TNC->Streams[0].PTCStatus2, TNC->Streams[0].PTCStatus3);
			
			if (ISS)
				SetDlgItemText(TNC->hDlg, IDC_TXRX, "Sender");
			else
				SetDlgItemText(TNC->hDlg, IDC_TXRX, "Receiver");

			SetDlgItemText(TNC->hDlg, IDC_STATE, status[Status]);
			SetDlgItemText(TNC->hDlg, IDC_MODE, ModeText[TNC->Mode]);

			if (TNC->Mode == 7)
				TNC->Busy = 30;				// 3 sec delay

			if (Offset == 128)		// Undefined
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset Unknown",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7]);
			else
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset %d",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7], Offset);

			SetDlgItemText(TNC->hDlg, IDC_PACTORLEVEL, StatusMsg);

			return;
		}
		
		if (Msg[2] == 253)						// Rig Port Response
		{
			int ret;

			// (Win98)
			//	return DeviceIoControl(
			//			VCOMInfo[port]->ComDev,(VCOMInfo[port]->Port << 16) | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
			//else

			DeviceIoControl(
					TNC->VCOMHandle, IOCTL_SERIAL_SETDATA, &Msg[5], Msg[4] + 1, NULL, 0, &ret, NULL);

//			Debugprintf("SCS - Rig Control Response");


			return;
		}
		// Connected Data
		
		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = Msg[4] + 1;				// Length
		TNC->Streams[Stream].BytesRXed += buffptr[1];
		memcpy(&buffptr[2], &Msg[5], buffptr[1]);
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}
}


MESSAGE Monframe;		// I frames come in two parts.

#define TIMESTAMP 352

VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	// Convert to ax.25 form and pass to monitor

	UCHAR * ptr;

	if (Msg[0] == 6)		// Second part of I or UI
	{
		int len = Msg[1] +1;

		memcpy(Monframe.L2DATA, &Msg[2], len);
		Monframe.LENGTH += len;

		_asm {

		pushad

		mov edi, offset Monframe
		mov eax, BPQTRACE
		call eax

		popad
		}

		return;
	}

	Monframe.LENGTH = 23;				// Control Frame
	Monframe.PORT = TNC->Port;


	ptr = strstr(Msg, "fm ");

	ConvToAX25(&ptr[3], Monframe.ORIGIN);

	UpdateMH(TNC, &ptr[3], ' ', 0);

	ptr = strstr(ptr, "to ");

	ConvToAX25(&ptr[3], Monframe.DEST);

	Monframe.ORIGIN[6] |= 1;				// Set end of address

	ptr = strstr(ptr, "ctl ");

	if (memcmp(&ptr[4], "SABM", 4) == 0)
		Monframe.CTL = 0x2f;
	else  
	if (memcmp(&ptr[4], "DISC", 4) == 0)
		Monframe.CTL = 0x43;
	else 
	if (memcmp(&ptr[4], "UA", 2) == 0)
		Monframe.CTL = 0x63;
	else  
	if (memcmp(&ptr[4], "DM", 2) == 0)
		Monframe.CTL = 0x0f;
	else 
	if (memcmp(&ptr[4], "UI", 2) == 0)
		Monframe.CTL = 0x03;
	else 
	if (memcmp(&ptr[4], "RR", 2) == 0)
		Monframe.CTL = 0x1 | (ptr[6] << 5);
	else 
	if (memcmp(&ptr[4], "RNR", 3) == 0)
		Monframe.CTL = 0x5 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "REJ", 3) == 0)
		Monframe.CTL = 0x9 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "FRMR", 4) == 0)
		Monframe.CTL = 0x87;
	else  
	if (ptr[4] == 'I')
	{
		Monframe.CTL = (ptr[5] << 5) | (ptr[6] & 7) << 1 ;
	}

	if (strchr(&ptr[4], '+'))
	{
		Monframe.CTL |= 0x10;
		Monframe.DEST[6] |= 0x80;				// SET COMMAND
	}

	if (strchr(&ptr[4], '-'))	
	{
		Monframe.CTL |= 0x10;
		Monframe.ORIGIN[6] |= 0x80;				// SET COMMAND
	}

	if (Msg[0] == 5)							// More to come
	{
		ptr = strstr(ptr, "pid ");	
		sscanf(&ptr[3], "%x", &Monframe.PID);
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
//1:fm G8BPQ to KD6PGI-1 ctl I11^ pid F0
//fm KD6PGI-1 to G8BPQ ctl DISC+

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// Queue it as we may have just sent data

	TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
	wsprintf(TNC->Streams[Stream].CmdSet, "D\r");
}


VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	TidyClose(TNC, Stream);			// I don't think Hostmode has a DD
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	char Status[80];
	
	TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);

	if (Stream == 0 || TNC->HFPacket)
	{
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");
		wsprintf(Status, "%d SCANSTART 15", TNC->Port);
		Rig_Command(-1, Status);

		if (TNC->HFPacket)
		{
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\rPR\r", TNC->NodeCall);
			TNC->Streams[0].DEDStream = 30;		// Packet Channel
		}
		else
		{
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\rPT\r", TNC->NodeCall);
			TNC->Streams[0].DEDStream = 31;		// Pactor Channel
		}
	}
	else
		wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->NodeCall);
}

