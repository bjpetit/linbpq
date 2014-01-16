//
//	FLARQ Emulator/FLDIGI Interface for BPQ32
//

#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "CHeaders.h"
#include <stdio.h>
#include <time.h>

#include "tncinfo.h"

#include "bpq32.h"

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#define TIMESTAMP 352

#define CONTIMEOUT 1200

#define AGWHDDRLEN sizeof(struct AGWHEADER)

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

extern int (WINAPI FAR *GetModuleFileNameExPtr)();

//int ResetExtDriver(int num);
extern char * PortConfig[33];
int SemHeldByAPI;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

static void ConnecttoFLDigiThread(int port);

void CreateMHWindow();
int Update_MH_List(struct in_addr ipad, char * call, char proto);

static int ConnecttoFLDigi();
static int ProcessReceivedData(int bpqport);
static ProcessLine(char * buf, int Port);
int KillTNC(struct TNCINFO * TNC);
int RestartTNC(struct TNCINFO * TNC);
VOID ProcessFLDigiPacket(struct TNCINFO * TNC, char * Message, int Len);
struct TNCINFO * GetSessionKey(char * key, struct TNCINFO * TNC);
VOID SendARQData(struct TNCINFO * TNC, UINT * Buffer);
static VOID DoMonitorHddr(struct TNCINFO * TNC, struct AGWHEADER * RXHeader, UCHAR * Msg);
VOID SendRPBeacon(struct TNCINFO * TNC);
VOID FLReleaseTNC(struct TNCINFO * TNC);
unsigned int CalcCRC(UCHAR * ptr, int Len);
VOID ARQTimer(struct TNCINFO * TNC);
VOID SaveAndSend(struct TNCINFO * TNC, struct ARQINFO * ARQ, SOCKET sock, char * Msg, int MsgLen);
VOID ProcessARQStatus(struct TNCINFO * TNC, struct ARQINFO * ARQ, char *Input);
VOID SendXMLPoll(struct TNCINFO * TNC);
static int ProcessXMLData(int port);

int DoScanLine(struct TNCINFO * TNC, char * Buff, int Len);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);

char * strlop(char * buf, char delim);

extern UCHAR BPQDirectory[];

#define MAXBPQPORTS 32
#define MAXMPSKPORTS 16

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static int MPSKChannel[MAXBPQPORTS+1];			// BPQ Port to MPSK Port
static int BPQPort[MAXMPSKPORTS][MAXBPQPORTS+1];	// MPSK Port and Connection to BPQ Port
static int MPSKtoBPQ_Q[MAXBPQPORTS+1];			// Frames for BPQ, indexed by BPQ Port
static int BPQtoMPSK_Q[MAXBPQPORTS+1];			// Frames for MPSK. indexed by MPSK port. Only used it TCP session is blocked

//	Each port may be on a different machine. We only open one connection to each MPSK instance

static char * MPSKSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static unsigned int MPSKInst = 0;
static int AttachedProcesses=0;

static HWND hResWnd,hMHWnd;
static BOOL GotMsg;

static HANDLE STDOUT=0;

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;
static SOCKADDR_IN destaddr[MAXBPQPORTS+1];

static int addrlen=sizeof(sinx);

//static short MPSKPort=0;

static time_t ltime,lasttime[MAXBPQPORTS+1];

static BOOL CONNECTING[MAXBPQPORTS+1];
static BOOL CONNECTED[MAXBPQPORTS+1];

//HANDLE hInstance;

static char WindowTitle[] = "FLDIGI";
static char ClassName[] = "FLDIGISTATUS";
static int RigControlRow = 165;

static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

#ifndef LINBPQ

static BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[200];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;
	char FN[MAX_PATH] = "";

	if (TNC->ProgramPath == NULL)
		return FALSE;

	GetWindowText(hwnd, wtext, 199);

	if (strstr(wtext,"* MULTIPSK"))
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		TNC->WIMMORPID = ProcessId;
		return FALSE;
	}
	
	return (TRUE);
}

#endif

static int ExtProc(int fn, int port,unsigned char * buff)
{
	UINT * buffptr;
	char txbuff[500];
	unsigned int bytes,txlen=0;
	struct TNCINFO * TNC = TNCInfo[port];
	int Stream = 0;
	struct STREAMINFO * STREAM;
	int TNCOK;
	short * sp;

	if (TNC == NULL)
		return 0;					// Port not defined

	// Look for attach on any call

//	for (Stream = 0; Stream <= 1; Stream++)
	{
		STREAM = &TNC->Streams[Stream];
	
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			char Cmd[80];

			// New Attach

			int calllen;
			STREAM->Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, STREAM->MyCall);
			STREAM->MyCall[calllen] = 0;
			STREAM->FramesOutstanding = 0;

			// Stop Scanning

			sprintf(Cmd, "%d SCANSTOP", TNC->Port);
			Rig_Command(-1, Cmd);

			sprintf(TNC->WEB_TNCSTATE, "In Use by %s", TNC->Streams[0].MyCall);
			SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

/*			len = sprintf(Cmd, "%cSTOP_BEACON_ARQ_FAE\x1b", '\x1a');
	
			if (TNC->MPSKInfo->TX)
				TNC->CmdSet = TNC->CmdSave = _strdup(Cmd);		// Savde till not transmitting
			else
				send(TNC->WINMORDataSock, Cmd, len, 0);
*/
		}
	}

	switch (fn)
	{
	case 7:			

		// 100 mS Timer. 

		while (TNC->PortRecord->UI_Q)			// Release anything accidentally put on UI_Q
		{
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);
			ReleaseBuffer(buffptr);
		}

		ARQTimer(TNC);
		SendXMLPoll(TNC);
		return 0;


	case 1:				// poll

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect
		
				time( &ltime );
				if (ltime-lasttime[port] >9 )
				{
					ConnecttoFLDigi(port);
					lasttime[port]=ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED)
			{
				FD_SET(TNC->WINMORSock,&readfs);
				FD_SET(TNC->WINMORDataSock,&readfs);
			}
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORDataSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTED)
			{
				FD_SET(TNC->WINMORSock,&errorfs);
				FD_SET(TNC->WINMORDataSock,&errorfs);
			}

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (FD_ISSET(TNC->WINMORDataSock,&readfs))
				{
					// data available
			
					ProcessReceivedData(port);			
				}

				if (FD_ISSET(TNC->WINMORSock,&readfs))
				{
					// data available
			
					ProcessXMLData(port);			
				}


				if (FD_ISSET(TNC->WINMORDataSock,&writefs))
				{
					if (BPQtoMPSK_Q[port] == 0)
					{
						//	Connect success

						TNC->CONNECTED = TRUE;
						TNC->CONNECTING = FALSE;

						sprintf(TNC->WEB_COMMSSTATE, "Connected to FLDIGI");
						SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);

						// If required, send signon
				
//						send(TNC->WINMORDataSock,"\x1a", 1, 0);
//						send(TNC->WINMORDataSock,"DIGITAL MODE ?", 14, 0);
//						send(TNC->WINMORDataSock,"\x1b", 1, 0);

//						EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);
					}
					else
					{
						// Write block has cleared. Send rest of packet

						buffptr=Q_REM(&BPQtoMPSK_Q[port]);

						txlen=buffptr[1];

						memcpy(txbuff,buffptr+2,txlen);

						bytes=send(TNC->WINMORDataSock,(const char FAR *)&txbuff,txlen,0);
					
						ReleaseBuffer(buffptr);

					}

				}
					
				if (FD_ISSET(TNC->WINMORDataSock,&errorfs) || FD_ISSET(TNC->WINMORSock,&errorfs))
				{
					//	if connecting, then failed, if connected then has just disconnected

//					if (CONNECTED[port])
//					if (!CONNECTING[port])
//					{
//						i=sprintf(ErrMsg, "MPSK Connection lost for BPQ Port %d\r\n", port);
//						WritetoConsole(ErrMsg);
//					}

					CONNECTING[port]=FALSE;
					CONNECTED[port]=FALSE;
				
				}

			}



		// See if any frames for this port

		for (Stream = 0; Stream <= 1; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			
			if (STREAM->Attached)
				CheckForDetach(TNC, Stream, STREAM, TidyClose, ForcedClose, CloseComplete);

			if (STREAM->ReportDISC)
			{
				STREAM->ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}

			// if Busy, send buffer status poll
	
			if (STREAM->PACTORtoBPQ_Q == 0)
			{
				if (STREAM->DiscWhenAllSent)
				{
					STREAM->DiscWhenAllSent--;
					if (STREAM->DiscWhenAllSent == 0)
						STREAM->ReportDISC = TRUE;				// Dont want to leave session attached. Causes too much confusion
				}
			}
			else
			{
				int datalen;
			
				buffptr=Q_REM(&STREAM->PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = Stream;
				buff[7] = 0xf0;
				memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
				datalen+=8;
				
				sp = (short *)&buff[5];
				*sp = datalen;

	//			buff[5]=(datalen & 0xff);
	//			buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}

		if (TNC->PortRecord->UI_Q)
		{
			struct _MESSAGE * buffptr;

			SOCKET Sock;	
			buffptr = Q_REM(&TNC->PortRecord->UI_Q);

			Sock = TNC->WINMORDataSock;
	
			ReleaseBuffer((UINT *)buffptr);
		}
			
	
		return (0);



	case 2:				// send

		
		if (!TNC->CONNECTED) return 0;		// Don't try if not connected to TNC

		Stream = buff[4];
		
		STREAM = &TNC->Streams[Stream]; 

//		txlen=(buff[6]<<8) + buff[5] - 8;	

		sp = (short *)&buff[5];
		txlen = *sp - 8;
				
		if (STREAM->Connected)
		{
			buffptr = GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore
		
			buffptr[1] = txlen;
			memcpy(buffptr+2, &buff[8], txlen);
		
			SendARQData(TNC, buffptr);
		}
		else
		{
			char Command[80];
			int len;

			buff[8 + txlen] = 0;
			_strupr(&buff[8]);

			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TidyClose(TNC, buff[4]);
				STREAM->ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			// See if Local command (eg RADIO)

			if (_memicmp(&buff[8], "RADIO ", 6) == 0)
			{
				sprintf(&buff[8], "%d %s", TNC->Port, &buff[14]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &buff[8]))
				{
				}
				else
				{
					UINT * buffptr = GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr[1] = sprintf((UCHAR *)&buffptr[2], "%s", &buff[8]);
					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (STREAM->Connecting && _memicmp(&buff[8], "ABORT", 5) == 0)
			{
//				len = sprintf(Command,"%cSTOP_SELECTIVE_CALL_ARQ_FAE\x1b", '\x1a');
	
//				if (TNC->MPSKInfo->TX)
//					TNC->CmdSet = TNC->CmdSave = _strdup(Command);		// Save till not transmitting
//				else
//					send(TNC->WINMORDataSock, Command, len, 0);

//				TNC->InternalCmd = TRUE;
				return (0);
			}

/*			if (_memicmp(&buff[8], "MODE", 4) == 0)
			{
				buff[7 + txlen] = 0;		// Remove CR
				
				len = sprintf(Command,"%cDIGITAL MODE %s\x1b", '\x1a', &buff[13]);
	
				if (TNC->MPSKInfo->TX)
					TNC->CmdSet = TNC->CmdSave = _strdup(Command);		// Save till not transmitting
				else
					send(TNC->WINMORDataSock, Command, len, 0);

				TNC->InternalCmd = TRUE;
				return (0);
			}
*/

			if (_memicmp(&buff[8], "INUSE?", 6) == 0)
			{
				// Return Error if in use, OK if not

				UINT * buffptr = GetBuff();
				int s = 0;

				while(s <= 1)
				{
					if (s != Stream)
					{		
						if (TNC->PortRecord->ATTACHEDSESSIONS[s])
						{
							buffptr[1] = sprintf((UCHAR *)&buffptr[2], "FLDig} Error - In use\r");
							C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
							return 1;							// Busy
						}
					}
					s++;
				}
				buffptr[1] = sprintf((UCHAR *)&buffptr[2], "FLDigi} Ok - Not in use\r");
				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			
				return 1;
			}

			// See if a Connect Command.

			if (toupper(buff[8]) == 'C' && buff[9] == ' ' && txlen > 2)	// Connect
			{
				char * ptr;
				char * context;
				unsigned short CRC;
				char crcstring[6];
				struct ARQINFO * ARQ = TNC->ARQInfo;
				int SendLen;
				char Reply[80];

				_strupr(&buff[8]);
				buff[8 + txlen] = 0;

				memset(STREAM->RemoteCall, 0, 10);

				ptr = strtok_s(&buff[10], " ,\r", &context);
				strcpy(STREAM->RemoteCall, ptr);

//<SOH>00cG8BPQ:1025 G8BPQ:24 0 7 T60R5W10FA36<EOT>

				SendLen = sprintf(Reply, "%c00c%s:1025 %s:24 8 7 T60R5W10", 1, STREAM->MyCall, STREAM->RemoteCall); 

				CRC = CalcCRC(Reply , SendLen);

				sprintf(crcstring, "%04X%c", CRC, 4);

				strcat(Reply, crcstring);

				SendLen += 5;

				ARQ->ARQState = ARQ_CONNECTING;
				SaveAndSend(TNC, ARQ, TNC->WINMORDataSock, Reply, SendLen);

				STREAM->Connecting = TRUE;	

				sprintf(TNC->WEB_TNCSTATE, "%s Connecting to %s", STREAM->MyCall, STREAM->RemoteCall);
				SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);


				return 0;
			}

			// Send any other command to Multipsk

			_strupr(&buff[8]);
			buff[7 + txlen] = 0;
			len = sprintf(Command,"%c%s\x1b", '\x1a', &buff[8]);

			send(TNC->WINMORDataSock, Command, len, 0);

			TNC->InternalCmd = TRUE;

		}

		return (0);

	case 3:	

		Stream = (int)buff;

		TNCOK = TNC->CONNECTED;

		STREAM = &TNC->Streams[Stream];

		if (STREAM->FramesOutstanding > 8)	
			return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);

		return TNCOK << 8 | STREAM->Disconnecting << 15;		// OK, but lock attach if disconnecting
	
		break;

	case 4:				// reinit

		shutdown(TNC->WINMORSock, SD_BOTH);
		shutdown(TNC->WINMORDataSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);
		TNC->CONNECTED = FALSE;

		if (TNC->WIMMORPID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
			RestartTNC(TNC);
		}

		return (0);

	case 5:				// Close

		shutdown(TNC->WINMORSock, SD_BOTH);
		shutdown(TNC->WINMORDataSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);

		if (TNC->WIMMORPID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
		}

		return 0;
	}

	return 0;
}

#ifndef LINBPQ

static KillTNC(struct TNCINFO * TNC)
{
	HANDLE hProc;

	if (TNC->PTTMode)
		Rig_PTT(TNC->RIG, FALSE);			// Make sure PTT is down

	if (TNC->WIMMORPID == 0) return 0;

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->WIMMORPID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	TNC->WIMMORPID = 0;			// So we don't try again

	return 0;
}

static RestartTNC(struct TNCINFO * TNC)
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	char HomeDir[MAX_PATH];
	int i, ret;

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	if (TNC->ProgramPath)
	{
		strcpy(HomeDir, TNC->ProgramPath);
		i = strlen(HomeDir);

		while(--i)
		{
			if (HomeDir[i] == '/' || HomeDir[i] == '\\')
			{
				HomeDir[i] = 0;
				break;
			}
		}
		ret = CreateProcess(TNC->ProgramPath, "MultiPSK TCP_IP_ON", NULL, NULL, FALSE,0 ,NULL ,HomeDir, &SInfo, &PInfo);

		if (ret)
			TNC->WIMMORPID = PInfo.dwProcessId;

		return ret;
	}
	return 0;
}
#endif

static int WebProc(struct TNCINFO * TNC, char * Buff, BOOL LOCAL)
{
	int Len = sprintf(Buff, "<html><meta http-equiv=expires content=0><meta http-equiv=refresh content=15>"
		"<script type=\"text/javascript\">\r\n"
		"function ScrollOutput()\r\n"
		"{var textarea = document.getElementById('textarea');"
		"textarea.scrollTop = textarea.scrollHeight;}</script>"
		"</head><title>FLDigi Status</title></head><body id=Text onload=\"ScrollOutput()\">"
		"<h2>FLDIGI Status</h2>");


	Len += sprintf(&Buff[Len], "<table style=\"text-align: left; width: 500px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>");

	Len += sprintf(&Buff[Len], "<tr><td width=110px>Comms State</td><td>%s</td></tr>", TNC->WEB_COMMSSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>TNC State</td><td>%s</td></tr>", TNC->WEB_TNCSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Mode</td><td>%s</td></tr>", TNC->WEB_MODE);
	Len += sprintf(&Buff[Len], "<tr><td>Channel State</td><td>%s</td></tr>", TNC->WEB_CHANSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Proto State</td><td>%s</td></tr>", TNC->WEB_PROTOSTATE);
	Len += sprintf(&Buff[Len], "<tr><td>Traffic</td><td>%s</td></tr>", TNC->WEB_TRAFFIC);
//	Len += sprintf(&Buff[Len], "<tr><td>TNC Restarts</td><td></td></tr>", TNC->WEB_RESTARTS);
	Len += sprintf(&Buff[Len], "</table>");

	Len += sprintf(&Buff[Len], "<textarea rows=10 style=\"width:500px; height:250px;\" id=textarea >%s</textarea>", TNC->WebBuffer);
	Len = DoScanLine(TNC, Buff, Len);

	return Len;
}


UINT FLDigiExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	struct TNCINFO * TNC;
	char * ptr;

	//
	//	Will be called once for each MPSK port to be mapped to a BPQ Port
	//	The MPSK port number is in CHANNEL - A=0, B=1 etc
	//
	//	The Socket to connect to is in IOBASE
	//

	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		sprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return (int) ExtProc;
	}

	TNC->Port = port;

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, MYNODECALL, 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PERMITGATEWAY = TRUE;					// Can change ax.25 call on each stream
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->SCANCAPABILITIES = NONE;					// Scan Control - None

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 64;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	TNC->Hardware = H_FLDIGI;

	MPSKChannel[port] = PortEntry->PORTCONTROL.CHANNELNUM-65;
	
	PortEntry->MAXHOSTMODESESSIONS = 1;	

	i=sprintf(Msg,"FLDigi Host %s Port %d \n",
		TNC->WINMORHostName, TNC->WINMORPort);

	WritetoConsole(Msg);

#ifndef LINBPQ

	if (EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC))
		if (TNC->ProgramPath)
			TNC->WeStartedTNC = RestartTNC(TNC);

	ConnecttoFLDigi(port);
#endif
	time(&lasttime[port]);			// Get initial time value

	TNC->WebWindowProc = WebProc;
	TNC->WebWinX = 520;
	TNC->WebWinY = 500;
	TNC->WebBuffer = zalloc(5000);

	TNC->WEB_COMMSSTATE = zalloc(100);
	TNC->WEB_TNCSTATE = zalloc(100);
	TNC->WEB_CHANSTATE = zalloc(100);
	TNC->WEB_BUFFERS = zalloc(100);
	TNC->WEB_PROTOSTATE = zalloc(100);
	TNC->WEB_RESTARTTIME = zalloc(100);
	TNC->WEB_RESTARTS = zalloc(100);

	TNC->WEB_MODE = zalloc(20);
	TNC->WEB_TRAFFIC = zalloc(100);


#ifndef LINBPQ

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 500, 450);

	CreateWindowEx(0, "STATIC", "Comms State", WS_CHILD | WS_VISIBLE, 10,6,120,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_COMMSSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,6,386,20, TNC->hDlg, NULL, hInstance, NULL);
	
	CreateWindowEx(0, "STATIC", "TNC State", WS_CHILD | WS_VISIBLE, 10,28,106,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TNCSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,28,520,20, TNC->hDlg, NULL, hInstance, NULL);

	CreateWindowEx(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 10,50,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_MODE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,50,200,20, TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Channel State", WS_CHILD | WS_VISIBLE, 10,72,110,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_CHANSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 116,72,144,20, TNC->hDlg, NULL, hInstance, NULL);
 
 	CreateWindowEx(0, "STATIC", "Proto State", WS_CHILD | WS_VISIBLE,10,94,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_PROTOSTATE = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,116,94,374,20 , TNC->hDlg, NULL, hInstance, NULL);
 
	CreateWindowEx(0, "STATIC", "Traffic", WS_CHILD | WS_VISIBLE,10,116,80,20, TNC->hDlg, NULL, hInstance, NULL);
	TNC->xIDC_TRAFFIC = CreateWindowEx(0, "STATIC", "0 0 0 0", WS_CHILD | WS_VISIBLE,116,116,374,20 , TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill FLDigi");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart FLDigi");

	MoveWindows(TNC);
#endif

	return ((int) ExtProc);

}


static ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	struct ARQINFO * ARQ;
	struct FLINFO * FL;

	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if (_stricmp(buf, "ADDR"))
		return FALSE;						// Must start with ADDR

	ptr = strtok(NULL, " \t\n\r");

	BPQport = Port;
	p_ipad = ptr;

	TNC = TNCInfo[BPQport] = zalloc(sizeof(struct TNCINFO));

	ARQ = TNC->ARQInfo = zalloc(sizeof(struct ARQINFO)); 
	FL = TNC->FLInfo = zalloc(sizeof(struct FLINFO)); 

	TNC->Timeout = 100;			// Default retry = 10 seconds
	TNC->Retries = 6;		// Default Retries

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
		if (p_ipad == NULL)
			p_ipad = strtok(NULL, " \t\n\r");

		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		TNC->WINMORPort = atoi(p_port);

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(TNC->WINMORPort + 40);		// Defaults XML 7362 ARQ 7322
		
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(TNC->WINMORPort);

		TNC->WINMORHostName = malloc(strlen(p_ipad)+1);

		if (TNC->WINMORHostName == NULL) return TRUE;

		strcpy(TNC->WINMORHostName,p_ipad);

		ptr = strtok(NULL, " \t\n\r");

		if (ptr)
		{
			if (_memicmp(ptr, "PATH", 4) == 0)
			{
				p_cmd = strtok(NULL, "\n\r");
				if (p_cmd) TNC->ProgramPath = _strdup(_strupr(p_cmd));
			}
		}

		// Read Initialisation lines

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			strcpy(errbuf, buf);

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}

			if (_memicmp(buf, "TIMEOUT", 7) == 0)
				TNC->Timeout = atoi(&buf[8]) * 10;
			else
			if (_memicmp(buf, "RETRIES", 7) == 0)
				TNC->Retries = atoi(&buf[8]) * 10;
			else
				
			strcat (TNC->InitScript, buf);
		}


	return (TRUE);	
}

static int ConnecttoFLDigi(int port)
{
	_beginthread(ConnecttoFLDigiThread,0,port);

	
	return 0;
}

static VOID ConnecttoFLDigiThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt = NULL;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(5000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (TNC->WINMORHostName);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for FLDigi Control socket - error code = %d\n", WSAGetLastError());
		WritetoConsole(Msg);
  	 	return; 
	}
 
	setsockopt (TNC->WINMORSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	TNC->CONNECTING = TRUE;

	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=sprintf(Msg, "Connect Failed for FLDigi Control socket - error code = %d\n", err);
			WritetoConsole(Msg);

			sprintf(TNC->WEB_COMMSSTATE, "Connection to TNC failed");
			SetWindowText(TNC->xIDC_COMMSSTATE, TNC->WEB_COMMSSTATE);
			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	TNC->LastFreq = 0;

	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	setsockopt (TNC->WINMORDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	if (TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=sprintf(Msg, "Socket Failed for FLDigi socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);
	 	TNC->CONNECTING = FALSE;

  	 	return; 
	}
 
	if (bind(TNC->WINMORDataSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=sprintf(Msg, "Bind Failed for FLDigi Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);
	 	TNC->CONNECTING = FALSE;
  	 	return; 
	}

	if (connect(TNC->WINMORDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		ioctlsocket (TNC->WINMORDataSock,FIONBIO,&param);		// Set nonblocking
		TNC->CONNECTED = TRUE;
	 	TNC->CONNECTING = FALSE;

		TNC->Alerted = TRUE;

		SetWindowText(TNC->xIDC_COMMSSTATE, "Connected to FlDigi");
	}
	else
	{
		sprintf(Msg, "Connect Failed for FLDigi Data socket Port %d - error code = %d\r\n", port, WSAGetLastError());
		WritetoConsole(Msg);

		closesocket(TNC->WINMORSock);
		closesocket(TNC->WINMORDataSock);
	 	TNC->CONNECTING = FALSE;
	}

	return;

}

static int ProcessReceivedData(int port)
{
	unsigned int bytes;
	int i;
	char ErrMsg[255];
	char Message[500];
	struct TNCINFO * TNC = TNCInfo[port];

	//	Need to extract messages from byte stream

	bytes = recv(TNC->WINMORDataSock,(char *)&Message, 500, 0);

	if (bytes == SOCKET_ERROR)
	{
//		i=sprintf(ErrMsg, "Read Failed for MPSK socket - error code = %d\r\n", WSAGetLastError());
//		WritetoConsole(ErrMsg);
				
		closesocket(TNC->WINMORDataSock);
					
		TNC->CONNECTED = FALSE;
		if (TNC->Streams[0].Attached)
			TNC->Streams[0].ReportDISC = TRUE;

		return (0);
	}

	if (bytes == 0)
	{
		//	zero bytes means connection closed

		i=sprintf(ErrMsg, "FlDigi Connection closed for BPQ Port %d\n", port);
		WritetoConsole(ErrMsg);

		TNC->CONNECTED = FALSE;
		if (TNC->Streams[0].Attached)
			TNC->Streams[0].ReportDISC = TRUE;

		return (0);
	}

	//	Have some data
	
	ProcessFLDigiPacket(TNC, Message, bytes);			// Data may be for another port

	return (0);

}

VOID ProcessFLDigiData(struct TNCINFO * TNC);

VOID ProcessFLDigiPacket(struct TNCINFO * TNC, char * Message, int Len)
{
	char * MPTR = Message;
	char c;

	// Look for SOH/EOT delimiters. May Have several SOH before EOT

	while(Len)
	{
		c = *(MPTR++);

		switch (c)
		{
		case 01:				// New Packet

			if (TNC->InPacket)
				ProcessFLDigiData(TNC);

			TNC->DataBuffer[0] = 1;
			TNC->DataLen = 1;
			TNC->InPacket = TRUE;
			break;

		case 04:

			ProcessFLDigiData(TNC);
			TNC->DataLen = 0;
			TNC->InPacket = FALSE;

			break;

		default:

			if (TNC->InPacket)
			{
				if (TNC->DataLen == 1 && c != '0')		
				{
					TNC->InPacket = 0;
					break;
				}
			
				TNC->DataBuffer[TNC->DataLen++] = c;
			}

			if (TNC->DataLen > 520)
				TNC->DataLen--;			// Protect Buffer

		}
		Len--;
	}
}

static int UnStuff(UCHAR * inbuff, int len)
{
	int i,txptr=0;
	UCHAR c;
	UCHAR * outbuff = inbuff;

	for (i = 0; i < len; i++)
	{
		c = inbuff[i];

		if (c == 0xc0)
			c = inbuff[++i] - 0x20;

		outbuff[txptr++]=c;
	}

	return txptr;
}

unsigned int crcval = 0xFFFF;

void update(char c)
{
	int i;
	
	crcval ^= c & 255;
    for (i = 0; i < 8; ++i)
	{
        if (crcval & 1)
            crcval = (crcval >> 1) ^ 0xA001;
        else
            crcval = (crcval >> 1);
    }
}
	
unsigned int CalcCRC(UCHAR * ptr, int Len)
{
	int i;
	
	crcval = 0xFFFF;
	for (i = 0; i < Len; i++)
	{
		update(*ptr++);
	}
	return crcval;
}
/*

<SOH>00cG8BPQ:1025 G8BPQ:24 0 8 T60R6W108E06<EOT>
<SOH>00kG8BPQ:24 G8BPQ 4 85F9B<EOT>

<SOH>00cG8BPQ:1025 GM8BPQ:24 0 7 T60R5W1051D5<EOT> (128, 5)

,<SOH>00cG8BPQ:1025 G8BPQ:24 0 7 T60R5W10FA36<EOT>
<SOH>00kG8BPQ:24 G8BPQ 5 89FCA<EOT>

First no sees to be a connection counter. Next may be stream


<SOH>08s___ABFC<EOT>
<SOH>08tG8BPQ:73 xxx 33FA<EOT>
<SOH>00tG8BPQ:73 yyy 99A3<EOT>
<SOH>08dG8BPQ:90986C<EOT>
<SOH>00bG8BPQ:911207<EOT>

call:90 for dis 91 for dis ack 73<sp> for chat)

<SOH>08pG8BPQ<SUB>?__645E<EOT>
<SOH>00s_??4235<EOT>

<SOH>08pG8BPQ<SUB>?__645E<EOT>
<SOH>00s_??4235<EOT>

i Ident
c Connect
k Connect Ack
r Connect NAK
d Disconnect req
s Data Ack/ Retransmit Req )status)
p Poll
f Format Fail
b dis ack
t talk

a Abort
o Abort ACK


<SOH>00cG8BPQ:1025 G8BPQ:24 0 7 T60R5W10FA36<EOT>
<SOH>00kG8BPQ:24 G8BPQ 6 49A3A<EOT>
<SOH>08s___ABFC<EOT>
<SOH>08 ARQ:FILE::flarqmail-1.eml
ARQ:EMAIL::
ARQ:SIZE::90
ARQ::STX
//FLARQ COMPOSER
Date: 09/01/2014 23:24:42
To: gm8bpq
From: 
SubjectA0E0<SOH>
<SOH>08!: Test

Test Message

ARQ::ETX
F0F2<SOH>
<SOH>08pG8BPQ<SUB>!__623E<EOT>
<SOH>08pG8BPQ<SUB>!__623E<EOT>
<SOH>08pG8BPQ<SUB>!__623E<EOT>




*/
VOID ProcessFLDigiData(struct TNCINFO * TNC)
{
	UINT * buffptr;
	int Stream = 0;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	int Len = TNC->DataLen - 4;		// Not including CRC
	unsigned short CRC;
	char crcstring[6];
	UCHAR * Input = &TNC->DataBuffer[0];
	char CTRL = Input[3];
	struct ARQINFO * ARQ = TNC->ARQInfo;
	int SendLen;
	char Reply[80];


	if (Len < 0)
		return;

	TNC->DataBuffer[TNC->DataLen] = 0;

	// Check Checksum

	CRC = CalcCRC(Input , Len);

	sprintf(crcstring, "%04X", CRC);

	if (memcmp(&Input[Len], crcstring, 4) !=0)
	{
		// CRC Error - could just be noise

		return;
	}

	// Process Data

	if (CTRL == 's')
	{
		// Status

		ARQ->ARQTimer = 0;			// Stop retry timer
		Input[Len] = 0;
		ProcessARQStatus(TNC, ARQ, &Input[4]);

		return;
	}

	if (CTRL == 'p')
	{
		// Poll

		SendLen = sprintf(Reply, "%c00s%c%c%c", 1, ARQ->TXSeq + 32, ARQ->RXNoGaps + 32, ARQ->RXHighest + 32);

		if (ARQ->RXHighest != ARQ->RXNoGaps)
		{
			int n = ARQ->RXNoGaps + 1;
			n &= 63;

			while (n != ARQ->RXHighest)
			{
				if (ARQ->RXHOLDQ[n] == 0)		// Dont have it
					SendLen += sprintf(&Reply[SendLen], "%c", n + 32);

				n++;
				n &= 63;
			}
		}
		CRC = CalcCRC(Reply , SendLen);

		sprintf(crcstring, "%04X%c", CRC, 4);

		strcat(Reply, crcstring);

		SendLen += 5;
		send(TNC->WINMORDataSock, Reply, SendLen, 0);
		return;
	}


	if (CTRL == 'k')
	{
		// Connect ACK

		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = 0;
		STREAM->Connected = TRUE;

		ARQ->ARQState = ARQ_CONNECTED;
		ARQ->ARQTimer = 0;

		if (TNC->RIG)
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound Freq %s",  STREAM->MyCall, STREAM->RemoteCall, TNC->RIG->Valchar);
		else
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Outbound", STREAM->MyCall, STREAM->RemoteCall);
			
		SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

		UpdateMH(TNC, STREAM->RemoteCall, '+', 'Z');
			
		memset(ARQ, 0, sizeof(struct ARQINFO));	// Reset ARQ State
		ARQ->TXSeq = ARQ->TXLastACK = 63;			// Last Sent
		ARQ->RXHighest = ARQ->RXNoGaps = 63;		// Last Received

		STREAM->NeedDisc = 0;

		// Reply with status

		SendLen = sprintf(Reply, "%c00s%c%c%c", 1, ARQ->TXSeq + 32, ARQ->RXNoGaps + 32, ARQ->RXHighest + 32);

		if (ARQ->RXHighest != ARQ->RXNoGaps)
		{
			int n = ARQ->RXNoGaps + 1;
			n &= 63;

			while (n != ARQ->RXHighest)
			{
				if (ARQ->RXHOLDQ[n] == 0)		// Dont have it
					SendLen += sprintf(&Reply[SendLen], "%c", n + 32);

				n++;
				n &= 63;
			}
		}
		CRC = CalcCRC(Reply , SendLen);

		sprintf(crcstring, "%04X%c", CRC, 4);

		strcat(Reply, crcstring);

		SendLen += 5;
		send(TNC->WINMORDataSock, Reply, SendLen, 0);
		return;
	}


	if (CTRL == 'a')
	{
		// Abort. Send Abort ACK - same as status

		SendLen = sprintf(Reply, "%c00o%c%c%c", 1, ARQ->TXSeq + 32, ARQ->RXNoGaps + 32, ARQ->RXHighest + 32);

		if (ARQ->RXHighest != ARQ->RXNoGaps)
		{
			int n = ARQ->RXNoGaps + 1;
			n &= 63;

			while (n != ARQ->RXHighest)
			{
				if (ARQ->RXHOLDQ[n] == 0)		// Dont have it
					SendLen += sprintf(&Reply[SendLen], "%c", n + 32);

				n++;
				n &= 63;
			}
		}
		CRC = CalcCRC(Reply , SendLen);

		sprintf(crcstring, "%04X%c", CRC, 4);

		strcat(Reply, crcstring);

		SendLen += 5;
		send(TNC->WINMORDataSock, Reply, SendLen, 0);
		return;
	}

	if (CTRL == 'i')
	{
		// Ident

		return;
	}

	if (CTRL == 't')
	{
		// Talk - not sure what to do with these

		return;
	}

	if (CTRL == 'c')
	{
		// Connect Request

		char * call1;
		char * call2;
		char * port1;
		char * port2;
		char * ptr;
		char * context;
		int FarStream = 0;
		int Window = 0;
		APPLCALLS * APPL;
		char * ApplPtr = APPLS;
		int App;
		char Appl[10];
		struct WL2KInfo * WL2K = TNC->WL2K;
		TRANSPORTENTRY * SESS;


		call1 = strtok_s(&Input[4], " ", &context);
		call2 = strtok_s(NULL, " ", &context);

		port1 = strlop(call1, ':');
		port2 = strlop(call2, ':');

		// See if for us

		for (App = 0; App < 32; App++)
		{
			APPL=&APPLCALLTABLE[App];
			memcpy(Appl, APPL->APPLCALL_TEXT, 10);
			ptr=strchr(Appl, ' ');

			if (ptr) *ptr = 0;
	
			if (_stricmp(call2, Appl) == 0)
					break;
		}

		if (App > 31)
			if (strcmp(TNC->NodeCall, call2) !=0)
				return;				// Not Appl or Port/Node Call

		// Get a Session

		SuspendOtherPorts(TNC);

		ProcessIncommingConnect(TNC, call1, 0, TRUE);
				
		SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

		strcpy(STREAM->MyCall, call2);
		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = 0;
		
		if (TNC->RIG && TNC->RIG != &TNC->DummyRig && strcmp(TNC->RIG->RigName, "PTT"))
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, call2, TNC->RIG->Valchar);
			SESS->Frequency = (atof(TNC->RIG->Valchar) * 1000000.0) + 1500;		// Convert to Centre Freq
			SESS->Mode = TNC->WL2KMode;
		}
		else
		{
			sprintf(TNC->WEB_TNCSTATE, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, call2);
			if (WL2K)
			{
				SESS->Frequency = WL2K->Freq;
				SESS->Mode = WL2K->mode;
			}
		}
			
		if (WL2K)
			strcpy(SESS->RMSCall, WL2K->RMSCall);

		SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

		memset(ARQ, 0, sizeof(struct ARQINFO));	// Reset ARQ State
		ARQ->TXSeq = ARQ->TXLastACK = 63;			// Last Sent
		ARQ->RXHighest = ARQ->RXNoGaps = 63;		// Last Received

		STREAM->NeedDisc = 0;

		if (App < 32)
		{
			char AppName[13];

			memcpy(AppName, &ApplPtr[App * 21], 12);
			AppName[12] = 0;

			// Make sure app is available

			if (CheckAppl(TNC, AppName))
			{
				char Buffer[32];
				int MsgLen = sprintf(Buffer, "%s\r", AppName);

				buffptr = GetBuff();

				if (buffptr == 0)
				{
					return;			// No buffers, so ignore
				}

				buffptr[1] = MsgLen;
				memcpy(buffptr+2, Buffer, MsgLen);

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

				TNC->SwallowSignon = TRUE;

				// Save Appl Call in case needed for 

			}
			else
			{
					
				STREAM->NeedDisc = 100;	// 10 secs
			}
		}
	
		ptr =  strtok_s(NULL, " ", &context);
		FarStream = atoi(ptr);
		ptr =  strtok_s(NULL, " ", &context);
		Window = atoi(ptr);

		SendLen = sprintf(Reply, "%c00k%s:24 %s 6 4", 1, call2, call1); 

		CRC = CalcCRC(Reply , SendLen);

		sprintf(crcstring, "%04X%c", CRC, 4);

		strcat(Reply, crcstring);

		SendLen += 5;

		send(TNC->WINMORDataSock, Reply, SendLen, 0);

		if (STREAM->NeedDisc)
		{
				// Send Not Avail as a talk Message
			
				SendLen = sprintf(Reply, "%c00t%s:73 Application not available ", 1, call2); 

				CRC = CalcCRC(Reply , SendLen);

				sprintf(crcstring, "%04X%c", CRC, 4);

				strcat(Reply, crcstring);

				SendLen += 5;

				send(TNC->WINMORDataSock, Reply, SendLen, 0);
		}

		return;
	}

	if (CTRL == 'd')
	{
		// Disconnect Request

		char * call1;
		char * context;
		int FarStream = 0;
		int Window = 0;

		call1 = strtok_s(&Input[4], " ", &context);
		strlop(call1, ':');

		if (strcmp(STREAM->RemoteCall, call1))
			return;

		SendLen = sprintf(Reply, "%c00b%s:91", 1, STREAM->MyCall); 

		CRC = CalcCRC(Reply , SendLen);

		sprintf(crcstring, "%04X%c", CRC, 4);

		strcat(Reply, crcstring);

		SendLen += 5;

		send(TNC->WINMORDataSock, Reply, SendLen, 0);

		STREAM->ReportDISC = TRUE;
		return;
	}

	if (CTRL == 'b')
	{
		// Disconnect ACK
	
		if (STREAM->Connected)
		{
			// Create a traffic record
		
			char logmsg[120];	
			time_t Duration;

			Duration = time(NULL) - STREAM->ConnectTime;
				
			sprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, (int)(STREAM->BytesTXed/Duration),
				STREAM->BytesRXed, (int)(STREAM->BytesRXed/Duration), (int)Duration);

			Debugprintf(logmsg);
		}

		STREAM->Connecting = FALSE;
		STREAM->Connected = FALSE;		// Back to Command Mode
		STREAM->ReportDISC = TRUE;		// Tell Node

		if (STREAM->Disconnecting)		// 
			FLReleaseTNC(TNC);

		STREAM->Disconnecting = FALSE;

		return;
	}

	if (STREAM->Connected)
	{
		if (CTRL >= ' ' && CTRL < 96)
		{
			// ARQ Data

			int Seq = CTRL - 32;
			int Work;

//			if (rand() % 5 == 2)
//			{
//				Debugprintf("Dropping %d", Seq);
//				return; 
//			}

			buffptr = GetBuff();
	
			if (buffptr == NULL)
				return;				// Sould never run out, but cant do much else

			buffptr[1]  = Len - 4;
			memcpy(&buffptr[2], &Input[4], Len - 4);
			STREAM->BytesRXed += Len - 4;

			// Safest always to save, then see what we can process

			if (ARQ->RXHOLDQ[Seq])
			{
				// Wot! Shouldn't happen

				ReleaseBuffer(ARQ->RXHOLDQ[Seq]);
				Debugprintf("ARQ Seq %d Duplicate");
			}

			ARQ->RXHOLDQ[Seq] = buffptr;
			Debugprintf("ARQ saving %d", Seq);

			// If this is higher that highest received, save. But beware of wrap'

			// Hi = 2, Seq = 60  dont save s=h = 58
			// Hi = 10 Seq = 12	 save s-h = 2
			// Hi = 14 Seq = 10  dont save s-h = -4
			// Hi = 60 Seq = 2	 save s-h = -58

			Work = Seq - ARQ->RXHighest;

			if ((Work > 0 && Work < 32) || Work < -32)
				ARQ->RXHighest = Seq;

			// We may now be able to process some

			Work = (ARQ->RXNoGaps + 1) & 63;		// The next one we need

			while (ARQ->RXHOLDQ[Work])
			{
				// We have it

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, ARQ->RXHOLDQ[Work]);
//				ReleaseBuffer(ARQ->RXHOLDQ[Work]);

				ARQ->RXHOLDQ[Work] = NULL;
				Debugprintf("Processing %d from Q", Work);

				ARQ->RXNoGaps = Work;
				Work = (Work + 1) & 63;		// The next one we need
			}

			return;
		}
	}
}



/*
		buffptr = GetBuff();
				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]  = RXHeader->DataLength;
				memcpy(&buffptr[2], Message, RXHeader->DataLength);

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				return;

		return;


	case 'd':			// Disconnected


	
	case 'C':

        //   Connect. Can be Incoming or Outgoing

		// "*** CONNECTED To Station [CALLSIGN]" When the other station starts the connection
		// "*** CONNECTED With [CALLSIGN]" When we started the connection

 */


VOID SendARQData(struct TNCINFO * TNC, UINT * Buffer)
{
	// Send Data, saving a copy until acked.

	struct ARQINFO * ARQ = TNC->ARQInfo;
	UCHAR TXBuffer[300];
	SOCKET sock = TNC->WINMORDataSock;
	int SendLen;
	unsigned short CRC;
	char crcstring[6];
	
	ARQ->TXSeq++;
	ARQ->TXSeq &= 63;
	
	SendLen = sprintf(TXBuffer, "%c00%c", 1, ARQ->TXSeq + 32);
	memcpy(&TXBuffer[SendLen], &Buffer[2], Buffer[1]);
	SendLen += Buffer[1];

	CRC = CalcCRC(TXBuffer , SendLen);

	sprintf(crcstring, "%04X%c", CRC, 4);

	memcpy(&TXBuffer[SendLen], crcstring, 5);

	SendLen += 5;

	if (rand() % 5 == 2)
		Debugprintf("Dropping %d", ARQ->TXSeq);
	else 
		send(sock, TXBuffer, SendLen, 0);

	ARQ->TXHOLDQ[ARQ->TXSeq] = Buffer;

	TNC->Streams[0].BytesTXed += Buffer[1];

	ARQ->ARQTimer = 10;			// wait up to 1 sec for more data before polling
	ARQ->Retries = 1;
	ARQ->ARQState = ARQ_WAITDATA;

}

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	char Reply[80];
	int SendLen;
	unsigned short CRC;
	char crcstring[6];

	SendLen = sprintf(Reply, "%c08d%s:90", 1, TNC->Streams[0].MyCall); 

	CRC = CalcCRC(Reply , SendLen);

	sprintf(crcstring, "%04X%c", CRC, 4);

	strcat(Reply, crcstring);

	SendLen += 5;

	send(TNC->WINMORDataSock, Reply, SendLen, 0);

}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	TidyClose(TNC, Stream);			// I don't think Hostmode has a DD
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	char Cmd[80];

	sprintf(Cmd, "%d SCANSTART 15", TNC->Port);
	Rig_Command(-1, Cmd);

	Cmd[0] = 0;
}

VOID FLReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	strcpy(TNC->WEB_TNCSTATE, "Free");
	SetWindowText(TNC->xIDC_TNCSTATE, TNC->WEB_TNCSTATE);

	//	Start Scanner
				
	sprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command(-1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID SaveAndSend(struct TNCINFO * TNC, struct ARQINFO * ARQ, SOCKET sock, char * Msg, int MsgLen)
{
	// Used for Messages that need a reply. Save, send and set timeout

	memcpy(ARQ->LastMsg, Msg, MsgLen);
	ARQ->LastLen = MsgLen;
	send(sock, Msg, MsgLen, 0);
	ARQ->ARQTimer = TNC->Timeout;
	ARQ->Retries = TNC->Retries;

	return;
}


VOID ARQTimer(struct TNCINFO * TNC)
{
	struct ARQINFO * ARQ = TNC->ARQInfo;
	UINT * buffptr;
	struct STREAMINFO * STREAM = &TNC->Streams[0];
	unsigned short CRC;
	char crcstring[6];
	int SendLen;
	char Reply[80];
	struct FLINFO *	FL = TNC->FLInfo;

	if (ARQ->ARQTimer)
	{
		if (FL->TX)
		{
			// Only decrement if running send poll timer
			
			if (ARQ->ARQState != ARQ_WAITDATA)
				return;
		}

		ARQ->ARQTimer--;
		{
			if (ARQ->ARQTimer)
				return;					// Timer Still Running
		}

		ARQ->Retries--;

		if (ARQ->Retries)
		{
			// Retry Current Message

			send(TNC->WINMORDataSock, ARQ->LastMsg, ARQ->LastLen, 0);
			ARQ->ARQTimer = TNC->Timeout;

			return;
		}

		// Retried out.

		switch (ARQ->ARQState)
		{
		case ARQ_WAITDATA:

			// No more data available - send poll 

			SendLen = sprintf(Reply, "%c00p%s", 1, TNC->Streams[0].MyCall);

			CRC = CalcCRC(Reply , SendLen);

			sprintf(crcstring, "%04X%c", CRC, 4);
			strcat(Reply, crcstring);

			SendLen += 5;
			ARQ->ARQState = ARQ_WAITACK;
			SaveAndSend(TNC, ARQ, TNC->WINMORDataSock, Reply, SendLen);

			return;
	
		case ARQ_CONNECTING:

			// Report Connect Failed, and drop back to command mode

			buffptr = GetBuff();

			if (buffptr)
			{
				buffptr[1] = sprintf((UCHAR *)&buffptr[2], "FLDigi} Failure with %s\r", STREAM->RemoteCall);
				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
			}
	
			STREAM->Connected = FALSE;		// Back to Command Mode
			STREAM->Connecting = FALSE;		// Back to Command Mode
			STREAM->DiscWhenAllSent = 10;

			// Send Disc to TNC

			//		TidyClose(TNC, Stream);
			break;

		case ARQ_WAITACK:
		
			STREAM->Connected = FALSE;		// Back to Command Mode
			STREAM->DiscWhenAllSent = 5;

			break;

		}
	}
}
VOID ProcessARQStatus(struct TNCINFO * TNC, struct ARQINFO * ARQ, char * Input)
{
	// Release any acked frames and resend any outstanding

	int LastInSeq = Input[1] - 32;
	int LastRXed = Input[2] - 32;
	int FirstUnAcked = ARQ->TXLastACK;
	int n = strlen(Input) - 3;
	char * ptr;
	int NexttoResend;
	int First, Last;

	//	Release all up to LastInSeq
	
	while (FirstUnAcked != LastInSeq)
	{
		FirstUnAcked++;
		FirstUnAcked &= 63;

		if (ARQ->TXHOLDQ[FirstUnAcked])
		{
			ReleaseBuffer(ARQ->TXHOLDQ[FirstUnAcked]);
			ARQ->TXHOLDQ[FirstUnAcked] = NULL;
		}
	}

	ARQ->TXLastACK = FirstUnAcked;

	if (FirstUnAcked == ARQ->TXSeq)
		return;								// All Acked

	// Release any not in retry list up to LastRXed.

	ptr = &Input[3];

	while (n)
	{
		NexttoResend = *(ptr++) - 32;

		FirstUnAcked++;
		FirstUnAcked &= 63;

		while (FirstUnAcked != NexttoResend)
		{
			if (ARQ->TXHOLDQ[FirstUnAcked])
			{
				ReleaseBuffer(ARQ->TXHOLDQ[FirstUnAcked]);
				ARQ->TXHOLDQ[FirstUnAcked] = NULL;
			}
			FirstUnAcked++;
			FirstUnAcked &= 63;
		}

		// We don't ACK this one. Process any more resend values, then release up to LastRXed.

		n--;
	}

	//	Release rest up to LastRXed
	
	while (FirstUnAcked != LastRXed)
	{
		FirstUnAcked++;
		FirstUnAcked &= 63;

		if (ARQ->TXHOLDQ[FirstUnAcked])
		{
			ReleaseBuffer(ARQ->TXHOLDQ[FirstUnAcked]);
			ARQ->TXHOLDQ[FirstUnAcked] = NULL;
		}
	}

	// Resend anything in TX Buffer (From LastACK to TXSeq

	Last = ARQ->TXSeq + 1;
	Last &= 63;

	First = LastInSeq;

	while (First != Last)
	{
		First++;
		First &= 63;
			
		if(ARQ->TXHOLDQ[First])
		{
			UINT * Buffer = ARQ->TXHOLDQ[First];
			UCHAR TXBuffer[300];
			SOCKET sock = TNC->WINMORDataSock;
			int SendLen;
			unsigned short CRC;
			char crcstring[6];

			Debugprintf("Resend %d", First);
		
			SendLen = sprintf(TXBuffer, "%c00%c", 1, First + 32);
			memcpy(&TXBuffer[SendLen], &Buffer[2], Buffer[1]);
			SendLen += Buffer[1];

			CRC = CalcCRC(TXBuffer , SendLen);
			sprintf(crcstring, "%04X%c", CRC, 4);

			memcpy(&TXBuffer[SendLen], crcstring, 5);
			SendLen += 5;

			send(sock, TXBuffer, SendLen, 0);

			ARQ->ARQTimer = 10;			// wait up to 1 sec for more data before polling
			ARQ->Retries = 1;
			ARQ->ARQState = ARQ_WAITDATA;
		}
	}
}

static int ProcessXMLData(int port)
{
	unsigned int bytes;
	int i;
	char ErrMsg[255];
	char Message[500];
	struct TNCINFO * TNC = TNCInfo[port];
	struct FLINFO *	FL = TNC->FLInfo;
	char * ptr1, * ptr2;

	//	Need to extract messages from byte stream

	bytes = recv(TNC->WINMORSock,(char *)&Message, 500, 0);

	if (bytes == SOCKET_ERROR)
	{
//		i=sprintf(ErrMsg, "Read Failed for MPSK socket - error code = %d\r\n", WSAGetLastError());
//		WritetoConsole(ErrMsg);
				
		closesocket(TNC->WINMORSock);
					
		TNC->CONNECTED = FALSE;
		if (TNC->Streams[0].Attached)
			TNC->Streams[0].ReportDISC = TRUE;

		return (0);
	}

	if (bytes == 0)
	{
		//	zero bytes means connection closed

		i=sprintf(ErrMsg, "FlDigi Connection closed for BPQ Port %d\n", port);
		WritetoConsole(ErrMsg);

		TNC->CONNECTED = FALSE;
		if (TNC->Streams[0].Attached)
			TNC->Streams[0].ReportDISC = TRUE;

		return (0);
	}

	//	Have some data. Assume for now we get a whole packet

	ptr1 = strstr(Message, "<value>");

	if (ptr1)
	{
		ptr1+= 7;
		ptr2 = strstr(ptr1, "</value>");
		if (ptr2); *ptr2 = 0;

		if (strcmp(FL->LastXML, "modem.get_name") == 0)
		{
			strcpy(TNC->WEB_MODE, ptr1);
			SetWindowText(TNC->xIDC_MODE, ptr1);
		}
		else if (strcmp(FL->LastXML, "main.get_trx_state") == 0)
		{
			if (strcmp(ptr1, "RX") == 0)
				FL->TX = FALSE;
			else
				FL->TX = TRUE;
		}
		else if (strcmp(FL->LastXML, "main.get_squelch") == 0)
		{
/*
			if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		SetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		strcpy(TNC->WEB_CHANSTATE, "Busy");

		TNC->WinmorRestartCodecTimer = time(NULL);
*/
			return;
	}
/*
	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->BusyHold)
			strcpy(TNC->WEB_CHANSTATE, "BusyHold");
		else
			strcpy(TNC->WEB_CHANSTATE, "Clear");

		SetWindowText(TNC->xIDC_CHANSTATE, TNC->WEB_CHANSTATE);
		TNC->WinmorRestartCodecTimer = time(NULL);
		return;
	}
*/

	}
	
	return (0);

}

char MsgHddr[] = "POST /RPC2 HTTP/1.1\r\n"
					"User-Agent: XMLRPC++ 0.8\r\n"
					"Host: 127.0.0.1:7362\r\n"
					"Content-Type: text/xml\r\n"
					"Content-length: %d\r\n"
					"\r\n%s";

char Req[] = 	"<?xml version=\"1.0\"?>\r\n"
					"<methodCall><methodName>%s</methodName>\r\n"
					"</methodCall>\r\n";

VOID SendXMLPoll(struct TNCINFO * TNC)
{
	int Len;
	char ReqBuf[256];
	char SendBuff[256];
	struct FLINFO *	FL = TNC->FLInfo;
	struct ARQINFO * ARQ = TNC->ARQInfo;

	if (ARQ->ARQTimer)
	{
		// if timer is running, poll fot TX State
		
		strcpy(FL->LastXML, "main.get_trx_state");
		Len = sprintf(ReqBuf, Req, FL->LastXML);
		Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
		send(TNC->WINMORSock, SendBuff, Len, 0); 
		return;
	}

	FL->XMLControl++;


	if (FL->XMLControl > 20)
	{
		FL->XMLControl = 0;
		strcpy(FL->LastXML, "modem.get_name");
		Len = sprintf(ReqBuf, Req, FL->LastXML);
		Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
		send(TNC->WINMORSock, SendBuff, Len, 0); 
	}
	else
	{
		FL->XMLControl = 0;
		strcpy(FL->LastXML, "fldigi.list");//main.get_squelch
		Len = sprintf(ReqBuf, Req, FL->LastXML);
		Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);
		send(TNC->WINMORSock, SendBuff, Len, 0); 
	}
}