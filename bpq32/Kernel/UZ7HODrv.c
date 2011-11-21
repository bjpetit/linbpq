//
//	DLL to provide interface to allow G8BPQ switch to use UZ7HOPE as a Port Driver 
//	32bit environment,
//
//	Uses BPQ EXTERNAL interface
//


//  Version 1.0 January 2005 - Initial Version
//

//  Version 1.1	August 2005
//
//		Treat NULL string in Registry as use current directory

//	Version 1.2 January 2006

//		Support multiple commections (not quire yet!)
//		Fix memory leak when AGEPE not running


//	Version 1.3 March 2006

//		Support multiple connections

//	Version 1.4 October 1006

//		Write diagmnostics to BPQ console window instead of STDOUT

//	Version 1.5 February 2008

//		Changes for dynamic unload of bpq32.dll

//	Version 1.5.1 September 2010

//		Add option to get config from BPQ32.cfg

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdio.h>
#include <time.h>

#include "AsmStrucs.h"

#include "bpq32.h"

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#define TIMESTAMP 352

#include "TNCINFO.h"

#define AGWHDDRLEN sizeof(struct AGWHEADER)

unsigned long _beginthread( void( *start_address )( int ), unsigned stack_size, int arglist);

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

//int ResetExtDriver(int num);
extern char * PortConfig[33];

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

void ConnecttoUZ7HOThread(int port);

void CreateMHWindow();
int Update_MH_List(struct in_addr ipad, char * call, char proto);

int ConnecttoUZ7HO();
int ProcessReceivedData(int bpqport);
static ProcessLine(char * buf, int Port);
KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);
VOID ProcessAGWPacket(struct TNCINFO * TNC, char * Message);
VOID GetSessionKey(char * key, struct TNCINFO * TNC);
static VOID SendData(struct TNCINFO * TNC, char * key, char * Msg, int MsgLen);

UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
extern UINT FREE_Q;

extern UCHAR BPQDirectory[];

#define MAXBPQPORTS 32
#define MAXUZ7HOPORTS 16

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static int UZ7HOChannel[MAXBPQPORTS+1];			// BPQ Port to UZ7HO Port
static int BPQPort[MAXUZ7HOPORTS][MAXBPQPORTS+1];	// UZ7HO Port and Connection to BPQ Port
static int UZ7HOtoBPQ_Q[MAXBPQPORTS+1];			// Frames for BPQ, indexed by BPQ Port
static int BPQtoUZ7HO_Q[MAXBPQPORTS+1];			// Frames for UZ7HO. indexed by UZ7HO port. Only used it TCP session is blocked

//	Each port may be on a different machine. We only open one connection to each UZ7HO instance

static char * UZ7HOSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

#pragma pack()

static unsigned int UZ7HOInst = 0;
static int AttachedProcesses=0;

static HWND hResWnd,hMHWnd;
static BOOL GotMsg;

static HANDLE STDOUT=0;

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;
static SOCKADDR_IN destaddr[MAXBPQPORTS+1];

static int addrlen=sizeof(sinx);

//static short UZ7HOPort=0;

static time_t ltime,lasttime[MAXBPQPORTS+1];

static BOOL CONNECTING[MAXBPQPORTS+1];
static BOOL CONNECTED[MAXBPQPORTS+1];

//HANDLE hInstance;


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

static BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"Soundmodem", 10) == 0)
	{
		{
			 // Our Process

			GetWindowThreadProcessId(hwnd, &TNC->WIMMORPID);

			wsprintf (wtext, "Soundmodem - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int i,winerr;
	int datalen;
	UINT * buffptr;
	char txbuff[500];

	unsigned int bytes,txlen=0;
	char ErrMsg[255];

	struct TNCINFO * TNC = TNCInfo[port];
	struct AGWINFO * AGW = TNC->AGWInfo;

	int Stream = 0;
	struct STREAMINFO * STREAM;
	int TNCOK;

	if (TNC == NULL)
		return 0;							// Port not defined

	switch (fn)
	{
	case 1:				// poll

	//	if (MasterPort[port] == port)
		{
			// Only on first port using a host

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect
		
				time( &ltime );
				if (ltime-lasttime[port] >9 )
				{
					ConnecttoUZ7HO(port);
					lasttime[port]=ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->WINMORSock,&readfs);

			
			FD_ZERO(&writefs);

			if (TNC->CONNECTING) FD_SET(TNC->WINMORSock,&writefs);	// Need notification of Connect

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORSock,&writefs);	// Need notification of busy clearing



			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING ||TNC->CONNECTED) FD_SET(TNC->WINMORSock,&errorfs);

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (readfs.fd_count == 1)
				{
			
					// data available
			
					ProcessReceivedData(port);
				
				}

				if (writefs.fd_count == 1)
				{
					if (BPQtoUZ7HO_Q[port] == 0)
					{
				
						//	Connect success

						TNC->CONNECTED = TRUE;
						TNC->CONNECTING = FALSE;

						// If required, send signon

						if (UZ7HOSignon[port])
							send(TNC->WINMORSock,UZ7HOSignon[port],546,0);

						// Request Raw Frames

						AGW->TXHeader.Port=0;
						AGW->TXHeader.DataKind='k';		// Raw Frames
						AGW->TXHeader.DataLength=0;
						send(TNC->WINMORSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);
		
						// Register all applcalls

						AGW->TXHeader.DataKind='X';		// Register
						strcpy(AGW->TXHeader.callfrom, TNC->NodeCall);
						send(TNC->WINMORSock,(const char FAR *)&AGW->TXHeader,AGWHDDRLEN,0);
					
						EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);

					}
					else
					{
						// Write block has cleared. Send rest of packet

						buffptr=Q_REM(&BPQtoUZ7HO_Q[port]);

						txlen=buffptr[1];

						memcpy(txbuff,buffptr+2,txlen);

						bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					
						ReleaseBuffer(buffptr);

					}

				}
					
				if (errorfs.fd_count == 1)
				{

					//	if connecting, then failed, if connected then has just disconnected

//					if (CONNECTED[port])
					if (!CONNECTING[port])
					{
						i=wsprintf(ErrMsg, "UZ7HO Connection lost for BPQ Port %d\r\n", port);
						WritetoConsole(ErrMsg);
					}

					CONNECTING[port]=FALSE;
					CONNECTED[port]=FALSE;
				
				}

			}

		}

		// See if any frames for this port

		for (Stream = 0; Stream <= TNC->AGWInfo->MaxSessions; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
			{
				// New Attach

				int calllen;
				char Msg[80];

				TNC->Streams[Stream].Attached = TRUE;

				calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
				TNC->Streams[Stream].MyCall[calllen] = 0;

				if (Stream == 0)
				{
		
					// Stop other ports in same group

					//	SuspendOtherPorts(TNC);

					wsprintf(Msg, "In Use by %s", TNC->Streams[0].MyCall);
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Msg);

					// Stop Scanning

					wsprintf(Msg, "%d SCANSTOP", TNC->Port);
	
					Rig_Command(-1, Msg);
				}
			}

			if (TNC->Streams[Stream].Attached)
				CheckForDetach(TNC, Stream, &TNC->Streams[Stream], TidyClose, ForcedClose, CloseComplete);

			if (TNC->Streams[Stream].ReportDISC)
			{
				TNC->Streams[Stream].ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}


		for (Stream = 0; Stream <= TNC->AGWInfo->MaxSessions; Stream++)
		{
			STREAM = &TNC->Streams[Stream];
			
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
				buff[5]=(datalen & 0xff);
				buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}
	
			
	
		return (0);



	case 2:				// send

		
		if (!TNC->CONNECTED) return 0;		// Don't try if not connected

		if (TNC->BPQtoWINMOR_Q) return 0;		// Socket is blocked - just drop packets
														// till it clears
		STREAM = &TNC->Streams[buff[4]]; 

		txlen=(buff[6]<<8) + buff[5] - 8;	
			
		if (STREAM->Connected)
			SendData(TNC, &STREAM->AGWKey[0], &buff[8], txlen);
		else
		{
			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}
	
			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff[8]) == 'C' && buff[9] == ' ' && txlen > 2)	// Connect
			{
				struct AGWINFO * AGW = TNC->AGWInfo;

				_strupr(&buff[8]);

				memset(STREAM->RemoteCall, 0, 10);
				memcpy(STREAM->RemoteCall, &buff[10], txlen-3);

				STREAM->AGWKey[0] = '1';
				strcpy(&STREAM->AGWKey[11], STREAM->MyCall);
				strcpy(&STREAM->AGWKey[1], STREAM->RemoteCall);

				AGW->TXHeader.Port = STREAM->AGWKey[0] - '1';
				AGW->TXHeader.DataKind='C';
				strcpy(AGW->TXHeader.callfrom, &STREAM->AGWKey[11]);
				strcpy(AGW->TXHeader.callto, &STREAM->AGWKey[1]);
				AGW->TXHeader.DataLength = 0;

				send(TNC->WINMORSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);

				STREAM->Connecting = TRUE;

//				wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
//				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			}
		}

/*

		
		AGW->TXHeader.Port=UZ7HOChannel[port];
		AGW->TXHeader.DataKind='K';				// raw send
		AGW->TXHeader.DataLength=txlen;

		memcpy(&txbuff, &AGW->TXHeader, AGWHDDRLEN);
		memcpy(&txbuff[AGWHDDRLEN],&buff[6],txlen);

		txbuff[AGWHDDRLEN] = 0;
		txlen+=AGWHDDRLEN;

		bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
		
		if (bytes != txlen)
		{

			// UZ7HO doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
				winerr=WSAGetLastError();
				
				i=wsprintf(ErrMsg, "UZ7HO Write Failed for port %d - error code = %d\r\n", port, winerr);
				WritetoConsole(ErrMsg);
					
	
//				if (winerr != WSAEWOULDBLOCK)
//				{
				
					closesocket(TNC->WINMORSock);
					
					TNC->CONNECTED = FALSE;

					return (0);
//				}
//				else
//				{
//					bytes=0;		// resent whole packet
//				}

//			}

			// Partial Send or WSAEWOULDBLOCK. Save data, and send once busy clears

			
			// Get a buffer
						
//			buffptr=GetBuff();

//			if (buffptr == 0)
//			{
				// No buffers, so can only break connection and try again

//				closesocket(UZ7HOSock[MasterPort[port]]);
					
//				CONNECTED[MasterPort[port]]=FALSE;

//				return (0);
//			}
	
//			buffptr[1]=txlen-bytes;			// Bytes still to send

//			memcpy(buffptr+2,&txbuff[bytes],txlen-bytes);

//			C_Q_ADD(&BPQtoUZ7HO_Q[MasterPort[port]],buffptr);
*/	
			return (0);
		

		return (0);

	case 3:	

		_asm 
		{

			MOV	EAX,buff
			mov Stream,eax
		}

		TNCOK = (TNC->CONNECTED);

		STREAM = &TNC->Streams[Stream];

		if (Stream == 0)
		{
			if (STREAM->FramesOutstanding  > 4)
				return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);
		}
		else
		{
			if (STREAM->FramesOutstanding > 3)	
				return (1 | TNCOK << 8 | STREAM->Disconnecting << 15);		}

		return TNCOK << 8 | STREAM->Disconnecting << 15;		// OK, but lock attach if disconnecting

			
		break;

	case 4:				// reinit

//		return(ReadConfigFile("BPQAXIP.CFG"));

		return (0);

	case 5:				// Close

		shutdown(TNC->WINMORDataSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->WINMORSock);

		SaveWindowPos(port);

		if (TNC->WIMMORPID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
		}

		return (0);
}
	return 0;
}

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

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	if (TNC->ProgramPath)
		return CreateProcess(TNC->ProgramPath, NULL, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);

	return 0;
}


UINT WINAPI UZ7HOExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	struct TNCINFO * TNC;
	char * ptr;

	//
	//	Will be called once for each UZ7HO port to be mapped to a BPQ Port
	//	The UZ7HO port number is in CHANNEL - A=0, B=1 etc
	//
	//	The Socket to connect to is in IOBASE
	//

	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(Msg," ** Error - no info in BPQ32.cfg for this port");
		WritetoConsole(Msg);

		return (int) ExtProc;
	}

	TNC->Port = port;

		TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->MAXHOSTMODESESSIONS = TNC->AGWInfo->MaxSessions;	
	PortEntry->SCANCAPABILITIES = NONE;			// Scan Control - pending connect only

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 64;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate



	if (TNC->ProgramPath)
		TNC->WeStartedTNC = RestartTNC(TNC);

	TNC->Hardware = H_UZ7HO;

	UZ7HOChannel[port] = PortEntry->PORTCONTROL.CHANNELNUM-65;

	i=wsprintf(Msg,"UZ7HO Port %d Host %s %d",port,TNC->WINMORHostName,htons(destaddr[port].sin_port));
	WritetoConsole(Msg);

	// See if we already have a port for this host

/*	MasterPort[port]=port;

	for (i=1;i<port;i++)
	{
		if (i == port) continue;

		if (destaddr[i].sin_port == destaddr[port].sin_port &&
			 _stricmp(UZ7HOHostName[i],UZ7HOHostName[port]) == 0)
		{
			MasterPort[port]=i;
			break;
		}
	}


	BPQPort[PortEntry->CHANNELNUM-65][MasterPort[port]]=PortEntry->PORTNUMBER;
			
	if (MasterPort[port] == port)
*/
	ConnecttoUZ7HO(port);

	time(&lasttime[port]);			// Get initial time value
	
	return ((int) ExtProc);

}

/*

#	Config file for BPQtoUZ7HO
#
#	For each UZ7HO port defined in BPQCFG.TXT, Add a line here
#	Format is BPQ Port, Host/IP Address, Port

#
#	Any unspecified Ports will use 127.0.0.1 and port for BPQCFG.TXT IOADDR field
#

1 127.0.0.1 8000
2 127.0.0.1 8001

*/


static ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	struct AGWINFO * AGW;

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
	AGW = TNC->AGWInfo = zalloc(sizeof(struct AGWINFO)); // AGW Sream Mode Specific Data

	AGW->MaxSessions = 10;

	TNC->InitScript = malloc(1000);
	TNC->InitScript[0] = 0;
	
		if (p_ipad == NULL)
			p_ipad = strtok(NULL, " \t\n\r");

		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		WINMORport = atoi(p_port);

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(WINMORport);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(WINMORport+1);

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

			if (_memicmp(buf, "RIGCONTROL", 10) == 0)
			{
				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				TNC->RigConfigMsg = _strdup(buf);
			}
			else
			
			if (_memicmp(buf, "MAXSESSIONS", 11) == 0)
			{
				AGW->MaxSessions = atoi(&buf[12]);
				if (AGW->MaxSessions > 26 ) AGW->MaxSessions = 26;
			}
			else
				
//			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
//				DecodeWL2KReportLine(TNC, buf, NARROWMODE, WIDEMODE);
//			else

			strcat (TNC->InitScript, buf);
		}


	return (TRUE);	
}

static int ConnecttoUZ7HO(int port)
{
	_beginthread(ConnecttoUZ7HOThread,0,port);

	return 0;
}

static VOID ConnecttoUZ7HOThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(5000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (&TNC->WINMORHostName[port]);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(TNC->WINMORSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->WINMORSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	TNC->CONNECTING = TRUE;

	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		TNC->CONNECTED=TRUE;
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=wsprintf(Msg, "Connect Failed for UZ7HO socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC failed");

			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	TNC->LastFreq = 0;			//	so V4 display will be updated

	SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connected to UZ7HO TNC");

	return;

}

static int ProcessReceivedData(int port)
{
	unsigned int bytes;
	int datalen,i;
	char ErrMsg[255];
	char Message[500];
	struct TNCINFO * TNC = TNCInfo[port];
	struct AGWINFO * AGW = TNC->AGWInfo;

	//	Need to extract messages from byte stream

	//	Use MSG_PEEK to ensure whole message is available

	bytes = recv(TNC->WINMORSock, (char *) &AGW->RXHeader, AGWHDDRLEN, MSG_PEEK);

	if (bytes == SOCKET_ERROR)
	{
		i=wsprintf(ErrMsg, "Read Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(ErrMsg);
				
		closesocket(TNC->WINMORSock);
					
		TNC->CONNECTED = FALSE;

		return (0);
	}

	if (bytes == 0)
	{
		//	zero bytes means connection closed

		i=wsprintf(ErrMsg, "UZ7HO Connection closed for BPQ Port %d\r\n", port);
		WritetoConsole(ErrMsg);

		TNC->CONNECTED = FALSE;
		return (0);
	}

	//	Have some data
	
	if (bytes == AGWHDDRLEN)
	{
		//	Have a header - see if we have any associated data
		
		datalen = AGW->RXHeader.DataLength;

		if (datalen > 0)
		{
			// Need data - See if enough there
				
			bytes = recv(TNC->WINMORSock, (char *)&Message, AGWHDDRLEN + datalen, MSG_PEEK);
		}

		if (bytes == AGWHDDRLEN + datalen)
		{
			bytes = recv(TNC->WINMORSock, (char *)&AGW->RXHeader, AGWHDDRLEN,0);

			if (datalen > 0)
			{
				bytes = recv(TNC->WINMORSock,(char *)&Message, datalen,0);
			}

			// Have header, and data if needed

			ProcessAGWPacket(TNC, Message);

			return (0);
		}

		// Have header, but not sufficient data

		return (0);
	
	}

	// Dont have at least header bytes
	
	return (0);

}
VOID ConnecttoMODEMThread(port);

int ConnecttoMODEM(int port)
{
	_beginthread(ConnecttoMODEMThread,0,port);

	return 0;
}

VOID ConnecttoMODEMThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(5000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname(&TNC->WINMORHostName[port]);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);
	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET || TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	setsockopt (TNC->WINMORDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->WINMORSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for UZ7HO socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	TNC->CONNECTING = TRUE;

	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		TNC->CONNECTED=TRUE;
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connected to UZ7HO TNC");
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=wsprintf(Msg, "Connect Failed for UZ7HO socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC failed");

			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	TNC->LastFreq = 0;			//	so V4 display will be updated

	return;
}
/*
UZ7HO C GM8BPQ GM8BPQ-2 *** CONNECTED To Station GM8BPQ-0

UZ7HO D GM8BPQ GM8BPQ-2 asasasas
M8BPQ
New Disconnect Port 7 Q 0
UZ7HO d GM8BPQ GM8BPQ-2 *** DISCONNECTED From Station GM8BPQ-0

New Disconnect Port 7 Q 0
*/

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


VOID ProcessAGWPacket(struct TNCINFO * TNC, char * Message)
{
	UINT * buffptr;
	MESSAGEY Monframe;

 	struct AGWINFO * AGW = TNC->AGWInfo;
	struct AGWHEADER * RXHeader = &AGW->RXHeader;
	char Key[21];
	int Stream;
	struct STREAMINFO * STREAM;

	Debugprintf("UZ7HO %c %s %s %s", RXHeader->DataKind, RXHeader->callfrom, RXHeader->callto, Message); 

	switch (RXHeader->DataKind)
	{
	case 'D':			// Appl Data

		GetSessionKey(Key, TNC);

		// Find our Session

		Stream = 0;

		while (Stream <= AGW->MaxSessions)
		{
			STREAM = &TNC->Streams[Stream];

			if (memcmp(STREAM->AGWKey, Key, 21) == 0)
			{
				// Found it;

				buffptr = GetBuff();
				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]  = RXHeader->DataLength;
				memcpy(&buffptr[2], Message, RXHeader->DataLength);

				C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
				return;
			}
			Stream++;
		}		
			
		// Not Found

		return;


	case 'd':			// Disconnected

		GetSessionKey(Key, TNC);

		// Find our Session

		Stream = 0;

		while (Stream <= AGW->MaxSessions)
		{
			STREAM = &TNC->Streams[Stream];

			if (memcmp(STREAM->AGWKey, Key, 21) == 0)
			{
				// Found it;

				if (STREAM->Connecting)
				{
					// Report Connect Failed, and drop back to command mode

					STREAM->Connecting = FALSE;
					buffptr = GetBuff();

					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "UZ7HO} Failure with %s\r", STREAM->RemoteCall);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);

					return;
				}

				// Release Session

				STREAM->Connecting = FALSE;
				STREAM->Connected = FALSE;		// Back to Command Mode
				STREAM->ReportDISC = TRUE;		// Tell Node

		//		if (STREAM->Disconnecting)		// 
		//			ReleaseTNC(TNC);

				STREAM->Disconnecting = FALSE;

				return;
			}
			Stream++;
		}

	case 'C':

        //   Connect. Can be Incoming or Outgoing

		// "*** CONNECTED To Station [CALLSIGN]" When the other station starts the connection
		// "*** CONNECTED With [CALLSIGN]" When we started the connection

        //   Create Session Key from port and callsign pair

		GetSessionKey(Key, TNC);

		if (strstr(Message, " To Station"))
		{
			// Incoming. Look for a free Stream

			Stream = AGW->MaxSessions;

			while(Stream)
			{
				if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
					goto GotStream;

				Stream--;
			}

			// No free streams - send Disconnect

			return;

	GotStream:

			STREAM = &TNC->Streams[Stream];
			memcpy(STREAM->AGWKey, Key, 21);
			STREAM->Connected = TRUE;
	
			UpdateMH(TNC, RXHeader->callfrom, '+', 'I');

			ProcessIncommingConnect(TNC, RXHeader->callfrom, Stream);

//			if (FULL_CTEXT && HFCTEXTLEN == 0)
			{
				int Len = CTEXTLEN, CTPaclen = 100;
				int Next = 0;

				while (Len > CTPaclen)		// CTEXT Paclen
				{
					SendData(TNC, &STREAM->AGWKey[0], &CTEXTMSG[Next], CTPaclen);
					Next += CTPaclen;
					Len -= CTPaclen;
				}
				SendData(TNC, &STREAM->AGWKey[0], &CTEXTMSG[Next], Len);
			}
			return;
		}
		else
		{
			// Connect Complete

			// Find our Session

			Stream = 0;

			while (Stream <= AGW->MaxSessions)
			{
				STREAM = &TNC->Streams[Stream];

				if (memcmp(STREAM->AGWKey, Key, 21) == 0)
				{
					// Found it;

					STREAM->Connected = TRUE;
					STREAM->Connecting = FALSE;

					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", RXHeader->callfrom);

					C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
					return;
				}
				Stream++;
			}		
			
			// Not Found

			return;
		}

	case 'K':				// raw data	

		Monframe.PORT = TNC->Port;
		
		Monframe.LENGTH = RXHeader->DataLength + 6;

		memcpy(&Monframe.DEST[0], &Message[1], RXHeader->DataLength);

		_asm {

		pushad

		lea edi, Monframe

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


		}
}
VOID GetSessionKey(char * key, struct TNCINFO * TNC)
{
	struct AGWINFO * AGW = TNC->AGWInfo;
	struct AGWHEADER * RXHeader = &AGW->RXHeader;

//   Create Session Key from port and callsign pair
        
	key[0] = RXHeader->Port + '1'; 
        
	memset(&key[1], 0, 20);
	strcpy(&key[1], RXHeader->callfrom);
	strcpy(&key[11], RXHeader->callto);

}

/*
Port field is the port where we want the data to tx
DataKind field =MAKELONG('D',0); The ASCII value of letter D
CallFrom is our call
CallTo is the call of the other station
DataLen is the length of the data that follow
*/

VOID SendData(struct TNCINFO * TNC, char * Key, char * Msg, int MsgLen)
{
	struct AGWINFO * AGW = TNC->AGWInfo;
	
	AGW->TXHeader.Port = Key[0] - '1';
	AGW->TXHeader.DataKind='D';
	strcpy(AGW->TXHeader.callfrom, &Key[11]);
	strcpy(AGW->TXHeader.callto, &Key[1]);
	AGW->TXHeader.DataLength = MsgLen;

	send(TNC->WINMORSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
	send(TNC->WINMORSock, Msg, MsgLen, 0);
}

VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	char * Key = &TNC->Streams[Stream].AGWKey[0];
	struct AGWINFO * AGW = TNC->AGWInfo;
	
	AGW->TXHeader.Port = Key[0] - '1';
	AGW->TXHeader.DataKind='d';
	strcpy(AGW->TXHeader.callfrom, &Key[11]);
	strcpy(AGW->TXHeader.callto, &Key[1]);
	AGW->TXHeader.DataLength = 0;

	send(TNC->WINMORSock, (char *)&AGW->TXHeader, AGWHDDRLEN, 0);
}



VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	TidyClose(TNC, Stream);			// I don't think Hostmode has a DD
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
}

