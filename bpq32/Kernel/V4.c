//
//	DLL to provide interface to allow G8BPQ switch to use the V4 TNC as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//
// Uses a number of routines in WINMOR.c


#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <time.h>

int (WINAPI FAR *GetModuleFileNameExPtr)();

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"
#include "TNCINFO.h"

#include "AsmStrucs.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

static int Socket_Data(int sock, int error, int eventcode);
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);
KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
SendReporttoWL2K(struct TNCINFO * TNC);
BOOL CheckAppl(struct TNCINFO * TNC, char * Appl);
static VOID ChangeMYC(struct TNCINFO * TNC, char * Call);

static char ClassName[]="V4STATUS";
static char WindowTitle[] = "V4TNC";
static int RigControlRow = 160;

#define V4
#define NARROWMODE 0
#define WIDEMODE 0

#include <commctrl.h>

extern UCHAR BPQDirectory[];

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];

static RECT Rect;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

static int ProcessLine(char * buf, int Port);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1




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
			if (_stricmp(ptr, "PTT") == 0)
			{
				ptr = strtok(NULL, " \t\n\r");

				if (ptr)
				{
					if (_stricmp(ptr, "CI-V") == 0)
						TNC->PTTMode = PTTCI_V;
					else if (_stricmp(ptr, "RTS") == 0)
						TNC->PTTMode = PTTRTS;
					else if (_stricmp(ptr, "DTR") == 0)
						TNC->PTTMode = PTTDTR;
					else if (_stricmp(ptr, "DTRRTS") == 0)
						TNC->PTTMode = PTTDTR | PTTRTS;

					ptr = strtok(NULL, " \t\n\r");
				}
			}
		}
		
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
				TNC->RigConfigMsg = _strdup(buf);
			}
			else
				
			if ((_memicmp(buf, "CAPTURE", 7) == 0) || (_memicmp(buf, "PLAYBACK", 8) == 0))
			{}		// Ignore
			else
			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
			{}		// Ignore
			else

			strcat (TNC->InitScript, buf);
		}
	}

	return (TRUE);	
}



void ConnecttoWINMORThread(int port);
VOID V4ProcessDataSocketData(int port);
int ConnecttoWINMOR();
int ProcessReceivedData(struct TNCINFO * TNC);
VOID ReleaseTNC(struct TNCINFO * TNC);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

extern UCHAR NEXTID;
extern struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern UCHAR L4DEFAULTWINDOW;
extern WORD L4T1;
extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;

extern struct BPQVECSTRUC * BPQHOSTVECPTR;

#define MAXBPQPORTS 32

static time_t ltime;

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static char * WINMORSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static char * WINMORHostName[MAXBPQPORTS+1];		// WINMOR Host - may be dotted decimal or DNS Name

#pragma pack()

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);

//static short WINMORPort=0;

//HANDLE hInstance;



static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

static VOID ChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

//	send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);

	datalen = wsprintf(TXMsg, "MYCALL %s\r\n", Call);
	send(TNC->WINMORSock,TXMsg, datalen, 0);

//	send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
//	TNC->StartSent = TRUE;

//	send(TNC->WINMORSock, "MYCALL\r\n", 8, 0);
}

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int i,winerr;
	int datalen;
	UINT * buffptr;
	char txbuff[500];
	char Status[80];
	unsigned int bytes,txlen=0;
	char ErrMsg[255];
	int Param;
	HKEY hKey=0;
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	if (TNC == NULL)
		return 0;							// Port not defined

	switch (fn)
	{
	case 1:				// poll

		if (TNC->BusyDelay)
		{
			// Still Busy?

			if ((TNC->Busy & CDBusy) == 0)
			{
				// No, so send

				send(TNC->WINMORSock, TNC->ConnectCmd, strlen(TNC->ConnectCmd), 0);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &TNC->ConnectCmd[11], strlen(TNC->ConnectCmd)-13);

				wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

				free(TNC->ConnectCmd);
				TNC->BusyDelay = 0;
			}
			else
			{
				// Wait Longer

				TNC->BusyDelay--;

				if (TNC->BusyDelay == 0)
				{
					// Timed out - Send Error Response

					UINT * buffptr = GetBuff();

					if (buffptr == 0) return (0);			// No buffers, so ignore

					buffptr[1]=39;
					memcpy(buffptr+2,"Sorry, Can't Connect - Channel is busy\r", 39);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					free(TNC->ConnectCmd);

				}
			}
		}

		if (TNC->HeartBeat++ > 600 || (TNC->Streams[0].Connected && TNC->HeartBeat > 50))			// Every Minute unless connected
		{
			TNC->HeartBeat = 0;

			if (TNC->CONNECTED)

				// Probe link

				send(TNC->WINMORSock, "BUFFER\r\n", 8, 0);
		}

		if (TNC->FECMode)
		{
			if (TNC->FECIDTimer++ > 6000)		// ID every 10 Mins
			{
				if (!TNC->Busy)
				{
					TNC->FECIDTimer = 0;
					send(TNC->WINMORSock, "SENDID 0\r\n", 10, 0);
				}
			}
			if (TNC->FECPending)	// Check if FEC Send needed
			{
				if (!TNC->Busy)
				{
					TNC->FECPending = 0;

					if (TNC->FEC1600)
						send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
				}
			}
		}

		if (STREAM->NeedDisc)
		{
			STREAM->NeedDisc--;

			if (STREAM->NeedDisc == 0)
			{
				// Send the DISCONNECT

				send(TNC->WINMORSock,"ARQEND\r\n", 8, 0);
				TNC->Streams[0].ARQENDSent = TRUE;
			}
		}

		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC

				if (TNC->WIMMORPID)
				{
					KillTNC(TNC);
					RestartTNC(TNC);
				}
			}
		}
/*
		if (TNC->UpdateWL2K)
		{
			TNC->UpdateWL2KTimer--;

			if (TNC->UpdateWL2KTimer == 0)
			{
				TNC->UpdateWL2KTimer = 32910/2;		// Every Hour
				if (CheckAppl(TNC, "RMS         ")) // Is RMS Available?
					SendReporttoWL2K(TNC);
			}
		}
*/
		if (TNC->RIG)
		{
			if (TNC->RIG->RigFreq != TNC->LastFreq)
			{
				char FREQMsg[80];
				int Len;
				
				TNC->LastFreq = TNC->RIG->RigFreq;
				Len = sprintf(FREQMsg, "DISPLAY CF:%1.4f\r\n", TNC->LastFreq + .0015);
				send(TNC->WINMORSock,FREQMsg, Len, 0);
			}
		}

		if (TNC->TimeSinceLast++ > 700)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "V4 TNC"))
				{
					struct tm * tm;
					char Time[80];
				
					TNC->Restarts++;
					TNC->LastRestart = time(NULL);

					tm = gmtime(&TNC->LastRestart);	
				
					sprintf_s(Time, sizeof(Time),"%04d/%02d/%02d %02d:%02dZ",
						tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

					SetDlgItemText(TNC->hDlg, IDC_RESTARTTIME, Time);
					sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
					SetDlgItemText(TNC->hDlg, IDC_RESTARTS, Time);

					KillTNC(TNC);
					RestartTNC(TNC);

					TNC->TimeSinceLast = 0;
				}
			}
		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Streams[0].Attached == 0)
		{
			// New Attach

			int calllen;
			char Msg[80];

			TNC->Streams[0].Attached = TRUE;
			TNC->Streams[0].ARQENDSent = FALSE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
			TNC->Streams[0].MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

//			send(TNC->WINMORSock, "LISTEN FALSE\r\n", 14, 0);
			ChangeMYC(TNC, TNC->Streams[0].MyCall);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			// Stop Scanning

			wsprintf(Msg, "%d SCANSTOP", TNC->Port);
	
			Rig_Command(-1, Msg);

		}

		if (TNC->Streams[0].Attached)
			CheckForDetach(TNC, 0, &TNC->Streams[0], TidyClose, ForcedClose, CloseComplete);

		if (TNC->Streams[0].ReportDISC)
		{
			TNC->Streams[0].ReportDISC = FALSE;
			buff[4] = 0;
			return -1;
		}

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect
		
				time(&ltime);
				if (ltime - TNC->lasttime >9 )
				{
					TNC->LastFreq = 0;			//	so display will be updated
					ConnecttoWINMOR(port);
					TNC->lasttime = ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&readfs);
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORDataSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&errorfs);

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (readfs.fd_count == 1)
					V4ProcessDataSocketData(port);			
				
				if (writefs.fd_count == 1)
				{
					// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
				}
					
				if (errorfs.fd_count == 1)
				{
					i=wsprintf(ErrMsg, "WINMOR Data Connection lost for BPQ Port %d\r\n", port);
					WritetoConsole(ErrMsg);
					TNC->CONNECTING = FALSE;
					TNC->CONNECTED = FALSE;
					TNC->Streams[0].ReportDISC = TRUE;
				}
			}
		
		// See if any frames for this port

		if (TNC->WINMORtoBPQ_Q != 0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen=buffptr[1];

			buff[4] = 0;						// Compatibility with Kam Driver
			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);	// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);
		}

		return (0);

	case 2:				// send

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			UINT * buffptr = GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr[1] = 24;
			memcpy(buffptr + 2, "No Connection to V4 TNC\r", 24);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->BPQtoWINMOR_Q)
			return 0;		// Socket is blocked - just drop packets till it clears

		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		
		if (TNC->Streams[0].Connected)
			bytes=send(TNC->WINMORDataSock,(const char FAR *)&buff[8],txlen,0);
		else
		{
			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}
	
			if (TNC->FECMode)
			{
				char Buffer[300];
				int len;

				// Send FEC Data

				buff[8 + txlen] = 0;
				len = wsprintf(Buffer, "%-9s: %s", TNC->Streams[0].MyCall, &buff[8]);

				send(TNC->WINMORDataSock, Buffer, len, 0);

/*				if (TNC->Busy)
				{
					TNC->FECPending = 1;
				}
				else
				{
					if (TNC->FEC1600)
						send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
				}
*/				return 0;
			}


			// See if Local command (eg RADIO)

			if (_memicmp(&buff[8], "RADIO ", 6) == 0)
			{
				wsprintf(&buff[8], "%d %s", TNC->Port, &buff[14]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &buff[8]))
				{
				}
				else
				{
					UINT * buffptr = GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &buff[8]);
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(&buff[8], "CODEC TRUE", 9) == 0)
				TNC->StartSent = TRUE;

			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TNC->Streams[0].ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			if (_memicmp(&buff[8], "FEC\r", 4) == 0 || _memicmp(&buff[8], "FEC ", 4) == 0)
			{
				TNC->FECMode = TRUE;
				TNC->FECIDTimer = 0;
				send(TNC->WINMORSock,"MODE FEC\r\n", 10, 0);
				SetDlgItemText(TNC->hDlg, IDC_MODE, "FEC");
				return 0;
			}

			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff[8]) == 'C' && buff[9] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80] = "ARQCONNECT ";

				memcpy(&Connect[11], &buff[10], txlen);
				txlen += 9;
				Connect[txlen++] = 0x0a;
				Connect[txlen] = 0;

				_strupr(Connect);

				// See if Busy
				
				if (TNC->Busy & CDBusy)
				{
					// Channel Busy. Unless override set, wait

					if (TNC->OverrideBusy == 0)
					{
						// Save Command, and wait up to 10 secs

						TNC->ConnectCmd = _strdup(Connect);
						TNC->BusyDelay = 100;		// 10 secs
						return 0;
					}
				}

				TNC->OverrideBusy = FALSE;

				bytes=send(TNC->WINMORSock, Connect, txlen, 0);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &Connect[11], txlen-13);

				wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			}
			else
			{
				buff[8 + txlen++] = 0x0a;
				bytes=send(TNC->WINMORSock,(const char FAR *)&buff[8],txlen,0);
			}
		}
		if (bytes != txlen)
		{

			// WINMOR doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
				winerr=WSAGetLastError();
				
				i=wsprintf(ErrMsg, "WINMOR Write Failed for port %d - error code = %d\r\n", port, winerr);
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

//				closesocket(WINMORSock[MasterPort[port]]);
					
//				CONNECTED[MasterPort[port]]=FALSE;

//				return (0);
//			}
	
//			buffptr[1]=txlen-bytes;			// Bytes still to send

//			memcpy(buffptr+2,&txbuff[bytes],txlen-bytes);

//			C_Q_ADD(&BPQtoWINMOR_Q[MasterPort[port]],buffptr);
	
//			return (0);
		}


		return (0);

	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		if (TNC->Streams[0].Attached == 0)
			return TNC->CONNECTED << 8 | 1;

		return (TNC->CONNECTED << 8 | TNC->Streams[0].Disconnecting << 15);		// OK
			
		break;

	case 4:				// reinit

		return (0);

	case 5:				// Close

		send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
		Sleep(100);
		shutdown(TNC->WINMORDataSock, SD_BOTH);
		shutdown(TNC->WINMORSock, SD_BOTH);
		Sleep(100);

		closesocket(TNC->WINMORDataSock);
		closesocket(TNC->WINMORSock);

		SaveWindowPos(port);

		if (TNC->WIMMORPID && TNC->WeStartedTNC)
		{
			KillTNC(TNC);
		}

		return (0);

	case 6:				// Scan Stop Interface

		_asm 
		{
			MOV	EAX,buff
			mov Param,eax
		}

		if (Param == 1)		// Request Permission
		{
			if (!TNC->ConnectPending)
				return 0;	// OK to Change

//			send(TNC->WINMORSock, "LISTEN FALSE\r\n", 14, 0);

			return TRUE;
		}

		if (Param == 2)		// Check  Permission
		{
			if (TNC->ConnectPending)
				return -1;	// Skip Interval

			return 1;		// OK to change
		}

		if (Param == 3)		// Release  Permission
		{
//			send(TNC->WINMORSock, "LISTEN TRUE\r\n", 13, 0);
			return 0;
		}

		if (Param == 4)		// Set Wide Mode
		{
			send(TNC->WINMORSock, "BW 1600\r\n", 9, 0);
			return 0;
		}

		if (Param == 5)		// Set Narrow Mode
		{
			send(TNC->WINMORSock, "BW 500\r\n", 8, 0);
			return 0;
		}

		return 0;
	}
	return 0;
}

VOID V4ProcessDataSocketData(int port)
{
	// Info on Data Socket - just packetize and send on
	
	struct TNCINFO * TNC = TNCInfo[port];
	int InputLen, PacLen = 236, i;
	UINT * buffptr;
	char * msg;
		
	TNC->TimeSinceLast = 0;

loop:
	buffptr = GetBuff();

	if (buffptr == NULL) return;			// No buffers, so ignore
			
	InputLen=recv(TNC->WINMORDataSock, (char *)&buffptr[2], PacLen, 0);

	if (InputLen == -1)
	{
		ReleaseBuffer(buffptr);
		return;
	}


	//Debugprintf("Winmor: RXD %d bytes", InputLen);

	if (InputLen == 0)
	{
		// Does this mean closed?
		
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC lost");
	
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		ReleaseBuffer(buffptr);
		return;					
	}


	msg = (char *)&buffptr[2];

	// Message should always be received in 17 char chunks. 17th is a status byte
	// In ARQ, 6 = "Echo as sent" ack

	if (InputLen != 17)
	{
		Debugprintf("V4 TNC incorrect RX Len  = %d", InputLen);
		goto loop;
	}

	if (msg[16] == 0x06)
		goto loop;

	InputLen = 16;

	for (i = 0; i < 16; i++)
	{
		if (msg[i] == 0)
			break;

		if (msg[i] == 10)
			continue;

		if (msg[i] < 0x20 || msg[i] > 0x7e)
			msg[i] = '?';
	}


	msg[InputLen] = 0;	
	
	WritetoTrace(TNC, msg, InputLen);
		
	// V4 Sends null padded blocks
	
	InputLen = strlen((char *)&buffptr[2]);

	if (msg[InputLen - 1] == 10)		// LF
	{
		// Replace with CRLF

		msg[InputLen-1] = 13;		// Add CR
		msg[InputLen++] = 10;
	}

	buffptr[1] = InputLen;
	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

	goto loop;
}




static VOID ReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call

	ChangeMYC(TNC, TNC->NodeCall);

//	send(TNC->WINMORSock, "LISTEN TRUE\r\nMAXCONREQ 4\r\n", 26, 0);

	SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

	//	Start Scanner
				

	ReleaseOtherPorts(TNC);

}




UINT WINAPI V4ExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	HMENU hMenu;
	struct TNCINFO * TNC;
	char * TempScript;
	
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

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = RestartTNC(TNC);

	TNC->Hardware = H_V4;


	if (TNC->RigConfigMsg)
	{
		char * SaveRigConfig = _strdup(TNC->RigConfigMsg);

		TNC->RIG = RigConfig(TNC->RigConfigMsg, port);

		if (TNC->RIG)
			TNC->RIG->PTTMode = TNC->PTTMode;
		else
		{
			// Report Error

			i=wsprintf(Msg,"Invalid Rig Config %s", TNC->RigConfigMsg);
			WritetoConsole(Msg);

		}
		free(SaveRigConfig);
	}

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->MAXHOSTMODESESSIONS = 1;	
//	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = malloc(1000);

	strcpy(TempScript, "DebugLog True\r\n");
	strcat(TempScript, "AUTOID FALSE\r\n");
	strcat(TempScript, "CODEC FALSE\r\n");
	strcat(TempScript, "TIMEOUT 90\r\n");
	strcat(TempScript, "MODE ARQ\r\n");
	strcat(TempScript, "TUNING 100\r\n");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Set MYCALL


//	strcat(TNC->InitScript,"FECRCV True\r\n");

	wsprintf(Msg, "MYCALL %s\r\nCODEC TRUE\r\nMYCALL\r\n", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);

	strcat(TNC->InitScript,"PROCESSID\r\n");

	strcpy(TNC->CurrentMYC, TNC->NodeCall);

	if (TNC->destaddr.sin_family == 0)
	{
		// not defined in config file, so use localhost and port from IOBASE

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE+1);

		TNC->WINMORHostName=malloc(10);

		if (TNC->WINMORHostName != NULL) 
			strcpy(TNC->WINMORHostName,"127.0.0.1");

	}

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, PacWndProc, 0);

	hMenu=CreateMenu();
	TNC->hPopMenu=CreatePopupMenu();
	SetMenu(TNC->hDlg,hMenu);

	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)TNC->hPopMenu,"Actions");

	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_KILL, "Kill V4 TNC");
	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart V4 TNC");
//	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after each Connection");
	
//	CheckMenuItem(TNC->hPopMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(TNC->hDlg);	

	TNC->hMonitor = GetDlgItem(TNC->hDlg, IDC_WINMORTRACE); 
	
	MoveWindows(TNC);

	i=wsprintf(Msg,"V4 Host %s %d", TNC->WINMORHostName, htons(TNC->destaddr.sin_port));
	WritetoConsole(Msg);

	SetDlgItemText(TNC->hDlg, IDC_MODE, "ARQ");
	ConnecttoWINMOR(port);

	time(&TNC->lasttime);			// Get initial time value

	return ((int) ExtProc);
}


static BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"Registration", 12) == 0)
	{
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return TRUE;
	}
	if (memcmp(wtext,"V4 Sound Card TNC", 17) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->WIMMORPID == ProcessId)
		{
			 // Our Process

			wsprintf (wtext, "V4 Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
	//		return FALSE;
		}
	}
	
	return (TRUE);
}

static VOID ProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	// Response on WINMOR control channel. Could be a reply to a command, or
	// an Async  Response

	UINT * buffptr;
	char Status[80];
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	if (_memicmp(Buffer, "FAULT failure to Restart Sound card", 20) == 0)
	{
		// Force a restart

			send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
			send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
	}
	else
	{
		TNC->TimeSinceLast = 0;
	}

	Buffer[MsgLen - 2] = 0;			// Remove CRLF

	if (_memicmp(Buffer, "PTT T", 5) == 0)
	{
		TNC->Busy |= PTTBusy;
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, TRUE);
		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		TNC->Busy &= ~PTTBusy;
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->Busy |= CDBusy;
		SetDlgItemText(TNC->hDlg, IDC_CHANSTATE, "Busy");
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->Busy &= ~CDBusy;
		SetDlgItemText(TNC->hDlg, IDC_CHANSTATE, "Clear");
		return;
	}

	Debugprintf(Buffer);

	if (_memicmp(Buffer, "OFFSET", 6) == 0)
	{
//		WritetoTrace(TNC, Buffer, MsgLen - 2);
//		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "CONNECTED", 9) == 0)
	{
		char Call[11];
		char * ptr;
		char * ApplPtr = &APPLS;
		struct APPLCALLS * APPL;
		int App;
		char Appl[10];

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
		{
			// Incomming Connect

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			ProcessIncommingConnect(TNC, Call, 0, TRUE);
			TNC->Streams[0].ARQENDSent = FALSE;

			if (TNC->RIG)
				wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
			else
				wsprintf(Status, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->TargetCall);

			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			// See which application the connect is for

			for (App = 0; App < 32; App++)
			{
				APPL=&APPLCALLTABLE[App];
				memcpy(Appl, APPL->APPLCALL_TEXT, 10);
				ptr=strchr(Appl, ' ');

				if (ptr)
					*ptr = 0;
	
				if (_stricmp(TNC->CurrentMYC, Appl) == 0)
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
					MsgLen = wsprintf(Buffer, "%s\r", AppName);
					buffptr = GetBuff();

					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1] = MsgLen;
					memcpy(buffptr+2, Buffer, MsgLen);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					TNC->SwallowSignon = TRUE;
				}
				else
				{
					char Msg[] = "Application not available\r\n";
					
					// Send a Message, then a disconenct
					
					send(TNC->WINMORDataSock, Msg, strlen(Msg), 0);
					STREAM->NeedDisc = 100;	// 10 secs
				}
			}

			return;
		}
		else
		{
			// Connect Complete

			char Reply[80];
			int ReplyLen;
			
			buffptr = GetBuff();

			if (buffptr == 0) return;			// No buffers, so ignore

			ReplyLen = wsprintf(Reply, "*** Connected to %s\r", &Buffer[10]);

			buffptr[1] = ReplyLen;
			memcpy(buffptr+2, Reply, ReplyLen);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG)
				wsprintf(Status, "%s Connected to %s Outbound Freq %s",  TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
			else
				wsprintf(Status, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);

			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			UpdateMH(TNC, Call, '+', 'O');

			return;
		}
	}

	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0)
	{
		if (TNC->FECMode)
			return;

		if (TNC->StartSent)
		{
			TNC->StartSent = FALSE;		// Disconnect reported following start codec
			return;
		}

		if (TNC->Streams[0].Connecting)
		{
			// Report Connect Failed, and drop back to command mode

			TNC->Streams[0].Connecting = FALSE;
			buffptr = GetBuff();

			if (buffptr == 0) return;			// No buffers, so ignore

			buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "V4} Failure with %s\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		// Release Session

		TNC->Streams[0].Connecting = FALSE;
		TNC->Streams[0].Connected = FALSE;		// Back to Command Mode
		TNC->Streams[0].ReportDISC = TRUE;		// Tell Node

		if (TNC->Streams[0].Disconnecting)		// 
			ReleaseTNC(TNC);

		TNC->Streams[0].Disconnecting = FALSE;

		return;
	}
		
	if (_memicmp(Buffer, "CMD", 3) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "PENDING", 6) == 0)
		return;

/*

	if (_memicmp(Buffer, "FAULT Not connected!", 20) == 0)
	{
		// If in response to ARQEND, assume Disconnected was missed

		if (TNC->Streams[0].Disconnecting) 
		{
			TNC->Streams[0].Connecting = FALSE;
			TNC->Streams[0].Connected = FALSE;		// Back to Command Mode
			TNC->Streams[0].ReportDISC = TRUE;		// Tell Node

			ReleaseTNC(TNC);
		
			TNC->Streams[0].Disconnecting = FALSE;
		}
	}
*/
	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	if (_memicmp(Buffer, "BUFFER", 6) == 0)
	{
		sscanf(&Buffer[7], "%d", &TNC->Streams[0].BytesOutstanding);

		if (TNC->Streams[0].BytesOutstanding == 0)
		{
			// all sent
			
			if (TNC->Streams[0].Disconnecting)						// Disconnect when all sent
			{
				if (TNC->Streams[0].ARQENDSent == FALSE)
				{
					send(TNC->WINMORSock,"ARQEND\r\n", 8, 0);
					TNC->Streams[0].ARQENDSent = TRUE;
				}
			}
		}
		else
		{
			// Make sure Node Keepalive doesn't kill session.

			struct TRANSPORTENTRY * SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];

			if (SESS)
			{
				SESS->L4KILLTIMER = 0;
				SESS = SESS->L4CROSSLINK;
				if (SESS)
					SESS->L4KILLTIMER = 0;
			}
		}

		SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, &Buffer[7]);
		return;
	}

	if (_memicmp(Buffer, "PROCESSID", 9) == 0)
	{
		HANDLE hProc;
		char ExeName[256] = "";

		TNC->WIMMORPID = atoi(&Buffer[10]);

		// Get the File Name in case we want to restart it.

		if (GetModuleFileNameExPtr)
		{
			hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->WIMMORPID);
	
			if (hProc)
			{
				GetModuleFileNameExPtr(hProc, 0,  ExeName, 255);
				CloseHandle(hProc);

				if (TNC->ProgramPath)
					free(TNC->ProgramPath);

				TNC->ProgramPath = _strdup(ExeName);
			}
		}

		// Set Window Title to reflect BPQ Port Description

		EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);
	}

	if (_memicmp(Buffer, "PLAYBACKDEVICES", 15) == 0)
	{
		TNC->PlaybackDevices = _strdup(&Buffer[16]);
	}
	// Others should be responses to commands

	if (_memicmp(Buffer, "BLOCKED", 6) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	if (_memicmp(Buffer, "CONREQ", 6) == 0)
	{
		// if to one of our APPLCALLS, change TNC MYCALL

		struct APPLCALLS * APPL;
		char Appl[11];
		char Target[20];
		char * ptr;
		int i;

		memcpy(Target, &Buffer[7], 12);
		ptr = memchr(Target, ' ', 12);
		if (ptr)
			*ptr = 0;

		if (strcmp(Target, TNC->NodeCall) == 0)
			ChangeMYC(TNC, Target);
		else
		{
			for (i = 0; i < 32; i++)
			{
				APPL=&APPLCALLTABLE[i];

				if (APPL->APPLCALL_TEXT[0] > ' ')
				{
					memcpy(Appl, APPL->APPLCALL_TEXT, 10);
					ptr=strchr(Appl, ' ');

					if (ptr)
						*ptr = 0;
	
					if (strcmp(Appl, Target) == 0)
					{
						ChangeMYC(TNC, Target);
						break;
					}
				}
			}
		}
		WritetoTrace(TNC, Buffer, MsgLen - 2);

		// Update MH

		ptr = strstr(Buffer, " de ");
		if (ptr)
			UpdateMH(TNC, ptr + 4, '!', 'O');
	}

	buffptr = GetBuff();

	if (buffptr == 0) return;			// No buffers, so ignore

	buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "V4} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
}

int V4ProcessReceivedData(struct TNCINFO * TNC)
{
	char ErrMsg[255];

	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (TNC->InputLen > 1000)	// Shouldnt have lines longer  than this on command connection
		TNC->InputLen=0;
				
	InputLen=recv(TNC->WINMORSock, &TNC->TCPBuffer[TNC->InputLen], 1000 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		if (!TNC->CONNECTING)
		{
			wsprintf(ErrMsg, "V4TNC Connection lost for BPQ Port %d\r\n", TNC->Port);
			WritetoConsole(ErrMsg);
		}
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		return 0;					
	}

	TNC->InputLen += InputLen;

loop:
	
	ptr = memchr(TNC->TCPBuffer, '\n', TNC->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &TNC->TCPBuffer[TNC->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			ProcessResponse(TNC, TNC->TCPBuffer, TNC->InputLen);
			TNC->InputLen=0;
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = TNC->InputLen - (ptr2-ptr);

			memcpy(Buffer, TNC->TCPBuffer, MsgLen);

			ProcessResponse(TNC, Buffer, MsgLen);

			memmove(TNC->TCPBuffer, ptr, TNC->InputLen-MsgLen);

			TNC->InputLen -= MsgLen;
			goto loop;
		}
	}
	return 0;
}

/*
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct TNCINFO * TNC = (struct TNCINFO * )lParam;
		char * ptr1, *ptr2;
		int ptr3 = 0;
		char Line[1000];
		int len;

		ptr1 = TNC->CaptureDevices;

		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_CAPTURE, Line);

		ptr3 = 0;

		ptr1 = TNC->PlaybackDevices;
	
		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_PLAYBACK, Line);

		SendDlgItemMessage(hDlg, IDC_PLAYBACK, EM_SETSEL, -1, 0);

//		KillTNC(TNC);

		return TRUE; 
	}

	case WM_SIZING:
	{
		return TRUE;
	}

	case WM_ACTIVATE:

//		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:


		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}
*/
static VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[0].BytesOutstanding == 0)
	{
		send(TNC->WINMORSock,"ARQEND\r\n", 8, 0);
		TNC->Streams[0].ARQENDSent = TRUE;
	}
}

static VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	send(TNC->WINMORSock,"ABORT\r\n", 7, 0);
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	ReleaseTNC(TNC);

	ChangeMYC(TNC, TNC->NodeCall);		// In case changed to an applcall

	if (TNC->FECMode)
	{
		TNC->FECMode = FALSE;
		send(TNC->WINMORSock,"MODE ARQ\r\n", 10, 0);
		SetDlgItemText(TNC->hDlg, IDC_MODE, "ARQ");
	}
}
