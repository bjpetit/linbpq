//
//	DLL to provide interface to allow G8BPQ switch to use WINMOR as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//


//  Version 1.0 January 2009 - Initial Version
//

// March 22 2010

// Send FAULTS to Monitor Window
// Force PROTOCOL = WINMOR/PACTOR (to simplifiy Config)

// July 2010
// Support up to 32 BPQ Ports
// Support up to 32 Applications

// Version 1.2.1.2 August 2010 

// Save Minimized State
// Handle new "BLOCKED by Busy channel" message from TNC

// Version 1.2.1.4 August 2010 

// Add Scan control of BW setting
// Reset TNC if stuck in Disconnecting
// Add option to send reports to WL2K
// Disconnect if appl not available

// Version 1.2.1.5 August 2010 

// Updates to WL2K Reporting
// Send Watchdog polls every minute and restart if no response.
// Don't connect if channel is busy (unless specifically overridden)

// Version 1.2.1.6 September 2010

// Add option to kill and restart TNC after each transfer
// Fix PTT operation after Node reconfig

// Version 1.2.2.1 September 2010

// Add option to get config from bpq32.cfg
// Merge with BPQ32.dll


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

static char ClassName[]="WINMORSTATUS";
static char WindowTitle[] = "WINMOR";
static int RigControlRow = 165;

#define WINMOR
#define WL2K
#define NARROWMODE 21
#define WIDEMODE 22

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

	if (_stricmp(buf, "ADDR"))
		return FALSE;						// Must start with ADDR

	ptr = strtok(NULL, " \t\n\r");

	BPQport = Port;
	p_ipad = ptr;

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
					else if (_stricmp(ptr, "CAT") == 0)
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

			strcpy(errbuf, buf);

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}
				
			if ((_memicmp(buf, "CAPTURE", 7) == 0) || (_memicmp(buf, "PLAYBACK", 8) == 0))
			{}		// Ignore
			else
/*
			if (_memicmp(buf, "PATH", 4) == 0)
			{
				char * Context;
				p_cmd = strtok_s(&buf[5], "\n\r", &Context);
				if (p_cmd) TNC->ProgramPath = _strdup(p_cmd);
			}
			else
*/
			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
				DecodeWL2KReportLine(TNC, buf, NARROWMODE, WIDEMODE);
			else
			if (_memicmp(buf, "BUSYHOLD", 8) == 0)		// Hold Time for Busy Detect
				TNC->BusyHold = atoi(&buf[8]);

			else
			if (_memicmp(buf, "BUSYWAIT", 8) == 0)		// Wait time beofre failing connect if busy
				TNC->BusyWait = atoi(&buf[8]);

			else

			strcat (TNC->InitScript, buf);
		}


	return (TRUE);	
}



void ConnecttoWINMORThread(int port);
VOID ProcessDataSocketData(int port);
int ConnecttoWINMOR();
int ProcessReceivedData(struct TNCINFO * TNC);
int V4ProcessReceivedData(struct TNCINFO * TNC);
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

#pragma pack()

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

VOID ChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

//	send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);

	datalen = wsprintf(TXMsg, "MYC %s\r\n", Call);
	send(TNC->WINMORSock,TXMsg, datalen, 0);

//	send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
//	TNC->StartSent = TRUE;

	send(TNC->WINMORSock, "MYC\r\n", 5, 0);
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
	struct ScanEntry * Scan;

	if (TNC == NULL)
		return 0;							// Port not defined

	switch (fn)
	{
	case 1:				// poll

		if (TNC->Busy)							//  Count down to clear
		{
			if ((TNC->BusyFlags & CDBusy) == 0)	// TNC Has reported not busy
			{
				TNC->Busy--;
				if (TNC->Busy == 0)
					SetWindowText(TNC->xIDC_CHANSTATE, "Clear");
			}
		}

		if (TNC->BusyDelay)
		{
			// Still Busy?

			if (InterlockedCheckBusy(TNC) == FALSE)
			{
				// No, so send

				send(TNC->WINMORSock, TNC->ConnectCmd, strlen(TNC->ConnectCmd), 0);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &TNC->ConnectCmd[8], strlen(TNC->ConnectCmd)-10);

				wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetWindowText(TNC->xIDC_TNCSTATE, Status);

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

					wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
					SetWindowText(TNC->xIDC_TNCSTATE, Status);

				}
			}
		}

		if (TNC->HeartBeat++ > 600 || (TNC->Streams[0].Connected && TNC->HeartBeat > 50))			// Every Minute unless connected
		{
			TNC->HeartBeat = 0;

			if (TNC->CONNECTED)

				// Probe link

				send(TNC->WINMORSock, "BUFFERS\r\n", 9, 0);
			
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

				send(TNC->WINMORSock, "DISCONNECT\r\n", 12, 0);
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

		if (TNC->TimeSinceLast++ > 700)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "WINMOR TNC"))
				{
					struct tm * tm;
					char Time[80];
				
					TNC->Restarts++;
					TNC->LastRestart = time(NULL);

					tm = gmtime(&TNC->LastRestart);	
				
					sprintf_s(Time, sizeof(Time),"%04d/%02d/%02d %02d:%02dZ",
						tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

					SetWindowText(TNC->xIDC_RESTARTTIME, Time);
					sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
					SetWindowText(TNC->xIDC_RESTARTS, Time);

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

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
			TNC->Streams[0].MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

			send(TNC->WINMORSock, "LISTEN FALSE\r\n", 14, 0);
			ChangeMYC(TNC, TNC->Streams[0].MyCall);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
			SetWindowText(TNC->xIDC_TNCSTATE, Status);

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
					ProcessDataSocketData(port);			
				
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

			buffptr[1]=36;
			memcpy(buffptr+2,"No Connection to WINMOR Virtual TNC\r", 36);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->Streams[0].BPQtoPACTOR_Q)		//Used for CTEXT
		{
			UINT * buffptr = Q_REM(&TNC->Streams[0].BPQtoPACTOR_Q);
			txlen=buffptr[1];
			memcpy(txbuff,buffptr+2,txlen);
			bytes = send(TNC->WINMORDataSock,(const char FAR *)&txbuff,txlen,0);
			STREAM->BytesTXed += bytes;
			ReleaseBuffer(buffptr);
		}
		
		if (TNC->SwallowSignon)
		{
			TNC->SwallowSignon = FALSE;		// Discard *** connected
			return 0;
		}


		txlen=(buff[6]<<8) + buff[5]-8;	
		
		if (TNC->Streams[0].Connected)
		{
			bytes=send(TNC->WINMORDataSock,(const char FAR *)&buff[8],txlen,0);
			STREAM->BytesTXed += bytes;
		}
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

				if (TNC->BusyFlags)
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
				return 0;
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

			if (_memicmp(&buff[8], "OVERRIDEBUSY", 12) == 0)
			{
				UINT * buffptr = GetBuff();

				TNC->OverrideBusy = TRUE;

				if (buffptr)
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}

				return 0;

			}


			if (_memicmp(&buff[8], "MAXCONREQ", 9) == 0)
			{
				if (buff[17] != 13)
				{
					// Limit connects

					int tries = atoi(&buff[18]);
					if (tries > 10) tries = 10;
					wsprintf(&buff[8], "MAXCONREQ %d\r\nMAXCONREQ\r\n", tries);

					send(TNC->WINMORSock,&buff[8],strlen(&buff[8]), 0);
					return 0;
				}
			}

			if ((_memicmp(&buff[8], "BW 500", 6) == 0) || (_memicmp(&buff[8], "BW 1600", 7) == 0))
			{
				// Generate a local response
				
				UINT * buffptr = GetBuff();

				if (buffptr)
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
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
				send(TNC->WINMORSock,"FECRCV TRUE\r\nFECRCV\r\n", 21, 0);
		
				if (_memicmp(&buff[8], "FEC 1600", 8) == 0)
					TNC->FEC1600 = TRUE;
				else
					TNC->FEC1600 = FALSE;

				return 0;
			}

			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff[8]) == 'C' && buff[9] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80] = "CONNECT ";

				memcpy(&Connect[8], &buff[10], txlen);
				txlen += 6;
				Connect[txlen++] = 0x0a;
				Connect[txlen] = 0;

				_strupr(Connect);

				// See if Busy
				
				if (InterlockedCheckBusy(TNC))
				{
					// Channel Busy. Unless override set, wait

					if (TNC->OverrideBusy == 0)
					{
						// Save Command, and wait up to 10 secs

						
						wsprintf(Status, "Waiting for clear channel");
						SetWindowText(TNC->xIDC_TNCSTATE, Status);

						TNC->ConnectCmd = _strdup(Connect);
						TNC->BusyDelay = TNC->BusyWait * 10;		// BusyWait secs
						return 0;
					}
				}

				TNC->OverrideBusy = FALSE;

				bytes=send(TNC->WINMORSock, Connect, txlen, 0);
				TNC->Streams[0].Connecting = TRUE;

				memset(TNC->Streams[0].RemoteCall, 0, 10);
				memcpy(TNC->Streams[0].RemoteCall, &Connect[8], txlen-10);

				wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
				SetWindowText(TNC->xIDC_TNCSTATE, Status);
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

		// Param is Address of a struct ScanEntry

		Scan = (struct ScanEntry *)buff;

		if (Scan->Bandwidth == 'W')		// Set Wide Mode
		{
			send(TNC->WINMORSock, "BW 1600\r\n", 9, 0);
			TNC->WL2KMode = 22;
			return 0;

		}

		if (Scan->Bandwidth == 'N')		// Set Wide Mode
		{
			send(TNC->WINMORSock, "BW 500\r\n", 8, 0);
			TNC->WL2KMode = 21;
			return 0;
		}

		return 0;
	}
	return 0;
}

VOID ReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	ChangeMYC(TNC, TNC->NodeCall);

	send(TNC->WINMORSock, "LISTEN TRUE\r\nMAXCONREQ 4\r\n", 26, 0);

	SetWindowText(TNC->xIDC_TNCSTATE, "Free");

	//	Start Scanner
				
	wsprintf(TXMsg, "%d SCANSTART 15", TNC->Port);

	Rig_Command(-1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID SuspendOtherPorts(struct TNCINFO * ThisTNC)
{
	// Disable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i, Interlock = ThisTNC->Interlock;

	if (Interlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

//		if (Interlock == TNC->Interlock)	// Same Group	
//			send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
	}
}

VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC)
{
	// Enable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i, Interlock = ThisTNC->Interlock;

	if (Interlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

//		if (Interlock == TNC->Interlock)	// Same Group	
//			send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
	}
}


UINT WINAPI WinmorExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	struct APPLCALLS * APPL;
	struct TNCINFO * TNC;
	char Aux[100] = "MYAUX ";
	char Appl[11];
	char * TempScript;

	//
	//	Will be called once for each WINMOR port 
	//
	//	The Socket to connect to is in IOBASE
	//
	
	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(Msg," ** Error - no info in BPQ32.cfg for this port\n");
		WritetoConsole(Msg);

		return (int) ExtProc;
	}

	TNC->Port = port;

	if (TNC->ProgramPath)
		TNC->WeStartedTNC = RestartTNC(TNC);

	TNC->Hardware = H_WINMOR;

	if (TNC->BusyWait == 0)
		TNC->BusyWait = 10;

	if (TNC->BusyHold == 0)
		TNC->BusyHold = 1;

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->MAXHOSTMODESESSIONS = 1;	
	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only

	if (PortEntry->PORTCONTROL.PORTPACLEN == 0)
		PortEntry->PORTCONTROL.PORTPACLEN = 236;

	TNC->ModemCentre = 1500;				// WINMOR is always 1500 Offset

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = malloc(1000);

	strcpy(TempScript, "DebugLog True\r\n");
	strcat(TempScript, "CWID False\r\n");
	strcat(TempScript, "BW 1600\r\n");
	strcat(TempScript, "ROBUST False\r\n");
	strcat(TempScript, "MODE AUTO\r\n");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Set MYCALL

	strcat(TNC->InitScript,"FECRCV True\r\n");
	strcat(TNC->InitScript,"AUTOBREAK True\r\n");

	wsprintf(Msg, "MYC %s\r\nCODEC TRUE\r\nLISTEN TRUE\r\nMYC\r\n", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);
	strcat(TNC->InitScript,"PROCESSID\r\n");

	for (i = 0; i < 32; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLCALL_TEXT[0] > ' ')
		{
			char * ptr;
			memcpy(Appl, APPL->APPLCALL_TEXT, 10);
			ptr=strchr(Appl, ' ');

			if (ptr)
			{
				*ptr++ = ',';
				*ptr = 0;
			}

			strcat(Aux, Appl);
		}
	}
	strcat(TNC->InitScript, Aux);
	strcat(TNC->InitScript,"\r\nMYAUX\r\n");


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

	CreateWindowEx(0, "STATIC", "TNC Restarts", WS_CHILD | WS_VISIBLE,10,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "0", WS_CHILD | WS_VISIBLE,116,138,40,20 , TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Last Restart", WS_CHILD | WS_VISIBLE,140,138,100,20, TNC->hDlg, NULL, hInstance, NULL);
	CreateWindowEx(0, "STATIC", "Never", WS_CHILD | WS_VISIBLE,250,138,200,20, TNC->hDlg, NULL, hInstance, NULL);

	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
            LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL,
			0,170,250,300, TNC->hDlg, NULL, hInstance, NULL);

	TNC->ClientHeight = 450;
	TNC->ClientWidth = 500;

	TNC->hMenu = CreatePopupMenu();

	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_KILL, "Kill Winmor TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart Winmor TNC");
	AppendMenu(TNC->hMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after each Connection");
	
	CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	MoveWindows(TNC);

	Consoleprintf("WINMOR Host %s %d", TNC->WINMORHostName, htons(TNC->destaddr.sin_port));

	ConnecttoWINMOR(port);

	time(&TNC->lasttime);			// Get initial time value

	return ((int) ExtProc);
}

int ConnecttoWINMOR(int port)
{
	_beginthread(ConnecttoWINMORThread,0,port);

	return 0;
}

VOID ConnecttoWINMORThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(5000);		// Allow init to complete 

	if (TNC->WINMORHostName == NULL)
		return;

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

	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);
	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET || TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
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
	
		i=wsprintf(Msg, "Bind Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	if (bind(TNC->WINMORDataSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for WINMOR Data socket - error code = %d\r\n", WSAGetLastError());
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
   			i=wsprintf(Msg, "Connect Failed for WINMOR socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			SetWindowText(TNC->xIDC_COMMSSTATE, "Connection to TNC failed");

			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	TNC->LastFreq = 0;			//	so V4 display will be updated

	if (connect(TNC->WINMORDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		TNC->DATACONNECTED=TRUE;
		ioctlsocket (TNC->WINMORSock,FIONBIO,&param);
		ioctlsocket (TNC->WINMORDataSock,FIONBIO,&param);
		TNC->CONNECTING = FALSE;

		// Send INIT script

		send(TNC->WINMORSock, TNC->InitScript , strlen(TNC->InitScript), 0);
		TNC->Alerted = TRUE;

		if (TNC->Hardware == H_V4)
			SetWindowText(TNC->xIDC_COMMSSTATE, "Connected to V4 TNC");
		else
			SetWindowText(TNC->xIDC_COMMSSTATE, "Connected to WINMOR TNC");

		// Use Async Notification on the Control port to give a fast enough response for Serial PTT

		if (WSAAsyncSelect(TNC->WINMORSock, TNC->hDlg, WSA_DATA, 
				FD_READ | FD_WRITE | FD_OOB | FD_CLOSE) > 0)

		{
			sprintf(Msg, "WSAAsyncSelect failed Error %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);
		}

		return;
	}

 	i=wsprintf(Msg, "Connect Failed for WINMOR Data socket Port %d - error code = %d\r\n", port, WSAGetLastError());
	WritetoConsole(Msg);
	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	return;
}

BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"WINMOR Sound Card TNC", 21) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->WIMMORPID == ProcessId)
		{
			 // Our Process

			wsprintf (wtext, "WINMOR Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}

VOID ProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
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
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, TRUE);
		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->BusyFlags |= CDBusy;
		TNC->Busy = TNC->BusyHold * 10;				// BusyHold  delay

		SetWindowText(TNC->xIDC_CHANSTATE, "Busy");
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->BusyFlags &= ~CDBusy;
		if (TNC->BusyHold)
			SetWindowText(TNC->xIDC_CHANSTATE, "BusyHold");
		else
			SetWindowText(TNC->xIDC_CHANSTATE, "Clear");
		return;
	}

	if (_memicmp(Buffer, "TARGET", 6) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

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
		struct APPLCALLS * APPL;
		char * ApplPtr = &APPLS;
		int App;
		char Appl[10];

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		STREAM->ConnectTime = time(NULL); 
		STREAM->BytesRXed = STREAM->BytesTXed = 0;

		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
		{
			struct TRANSPORTENTRY * SESS;
			
			// Incomming Connect

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			ProcessIncommingConnect(TNC, Call, 0, TRUE);

			SESS = TNC->PortRecord->ATTACHEDSESSIONS[0];
			
			if (TNC->RIG && TNC->RIG != &TNC->DummyRig)
			{
				wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
				SESS->Frequency = atof(TNC->RIG->Valchar) * 1000000.0;
			}
			else
			{
				wsprintf(Status, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->TargetCall);
				SESS->Frequency = atoi(TNC->WL2KFreq);
			}
			
			SESS->Mode = TNC->WL2KMode;
			
			SetWindowText(TNC->xIDC_TNCSTATE, Status);

			// See which application the connect is for

			for (App = 0; App < 32; App++)
			{
				APPL=&APPLCALLTABLE[App];
				memcpy(Appl, APPL->APPLCALL_TEXT, 10);
				ptr=strchr(Appl, ' ');

				if (ptr)
					*ptr = 0;
	
				if (_stricmp(TNC->TargetCall, Appl) == 0)
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

					// Save Appl Call in case needed for 

					memcpy(SESS->RMSCall, TNC->RMSCall, 10);

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

			SetWindowText(TNC->xIDC_TNCSTATE, Status);
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

			buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} Failure with %s\r", TNC->Streams[0].RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		// Release Session

		if (TNC->Streams[0].Connected)
		{
			// Create a traffic record
		
			char logmsg[120];	
			time_t Duration;

			Duration = time(NULL) - STREAM->ConnectTime;
				
			wsprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, STREAM->BytesTXed/Duration,
				STREAM->BytesRXed, STREAM->BytesRXed/Duration, Duration);

			Debugprintf(logmsg);
		}


		TNC->Streams[0].Connecting = FALSE;
		TNC->Streams[0].Connected = FALSE;		// Back to Command Mode
		TNC->Streams[0].ReportDISC = TRUE;		// Tell Node

		if (TNC->Streams[0].Disconnecting)		// 
			ReleaseTNC(TNC);

		TNC->Streams[0].Disconnecting = FALSE;

		return;
	}

	if (_memicmp(Buffer, "MONCALL", 7) == 0)
	{
		// Add to MHEARD

		WritetoTrace(TNC, Buffer, MsgLen - 2);
		UpdateMH(TNC, &Buffer[8], '!', 0);
		
		if (!TNC->FECMode)
			return;							// If in FEC mode pass ID messages to user.
	}
		
	if (_memicmp(Buffer, "CMD", 3) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "MODE", 4) == 0)
	{
	//	Debugprintf("WINMOR RX: %s", Buffer);
		SetWindowText(TNC->xIDC_MODE, &Buffer[5]);
		return;
	}

	if (_memicmp(Buffer, "PENDING", 6) == 0)
		return;

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	if (_memicmp(Buffer, "NEWSTATE", 8) == 0)
	{
		SetWindowText(TNC->xIDC_PROTOSTATE, &Buffer[9]);

		if (_memicmp(&Buffer[9], "CONNECTPENDING", 14) == 0)	// Save Pending state for scan control
			TNC->ConnectPending = TRUE;
		else
			TNC->ConnectPending = FALSE;
	
		if (_memicmp(&Buffer[9], "DISCONNECTING", 13) == 0)	// So we can timout stuck discpending
		{
			TNC->DiscPending = 600;
			return;
		}
		if (_memicmp(&Buffer[9], "DISCONNECTED", 12) == 0)	// Save Pending state for scan control
		{
			TNC->DiscPending = FALSE;
			if (TNC->RestartAfterFailure)
			{
				if (TNC->HadConnect)
				{
					TNC->HadConnect = FALSE;

					if (TNC->WIMMORPID)
					{
						KillTNC(TNC);
						RestartTNC(TNC);
					}
				}
			}
			return;
		}

		if (strcmp(&Buffer[9], "ISS") == 0)	// Save Pending state for scan control
			TNC->TXRXState = 'S';
		else if (strcmp(&Buffer[9], "IRS") == 0)
			TNC->TXRXState = 'R';
	
		return;
	}

	if (_memicmp(Buffer, "BUFFERS", 7) == 0)
	{
		int inq, inrx, Sent, BPM;

		sscanf(&Buffer[8], "%d%d%d%d%d", &inq, &inrx, &TNC->Streams[0].BytesOutstanding, &Sent, &BPM);

		if (TNC->Streams[0].BytesOutstanding == 0)
		{
			// all sent
			
			if (TNC->Streams[0].Disconnecting)						// Disconnect when all sent
			{
				if (STREAM->NeedDisc == 0)
					STREAM->NeedDisc = 60;								// 6 secs
			}
//			else
//			if (TNC->TXRXState == 'S')
//				send(TNC->WINMORSock,"OVER\r\n", 6, 0);

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

		SetWindowText(TNC->xIDC_TRAFFIC, &Buffer[8]);
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

	if ((_memicmp(Buffer, "FAULT Not from state FEC", 24) == 0) || (_memicmp(Buffer, "FAULT Blocked by Busy Lock", 24) == 0))
	{
		if (TNC->FECMode)
		{
			Sleep(1000);
			
			if (TNC->FEC1600)
				send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
			else
				send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
			return;
		}
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

	if (_memicmp(Buffer, "OVER", 4) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	buffptr = GetBuff();

	if (buffptr == 0) return;			// No buffers, so ignore

	buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
}

static int ProcessReceivedData(struct TNCINFO * TNC)
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
			wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", TNC->Port);
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


VOID ProcessDataSocketData(int port)
{
	// Info on Data Socket - just packetize and send on
	
	struct TNCINFO * TNC = TNCInfo[port];
	struct STREAMINFO * STREAM = &TNC->Streams[0];

	int InputLen, PacLen = 236;
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
		
		SetWindowText(TNC->xIDC_COMMSSTATE, "Connection to TNC lost");
	
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->Streams[0].ReportDISC = TRUE;

		ReleaseBuffer(buffptr);
		return;					
	}

	STREAM->BytesRXed += InputLen;

	msg = (char *)&buffptr[2];
	msg[InputLen] = 0;	
	
	WritetoTrace(TNC, msg, InputLen);
		
	if (TNC->FECMode)
	{	
		InputLen = strlen((char *)&buffptr[2]);

		if (msg[InputLen - 1] == 3)		// End of errored block
			msg[InputLen++] = 13;		// Add CR

	}
	buffptr[1] = InputLen;
	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

	goto loop;
}


int Winmor_Socket_Data(int sock, int error, int eventcode)
{
	int i=0;
	struct TNCINFO * TNC;
	char ErrMsg[255];

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;
			
		if (TNC->WINMORSock == sock)
			break;
	}
	if (!TNC)
		return 0;


	switch (eventcode)
	{
		case FD_READ:

			if (TNC->Hardware == H_V4)
				return 	V4ProcessReceivedData(TNC);			
			else
				return 	ProcessReceivedData(TNC);			

		case FD_WRITE:

//			sockptr->TCPState = TCPConnected;
/*
// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
*/

			return 0;

		case FD_OOB:

			return 0;

		case FD_CLOSE:

			i=wsprintf(ErrMsg, "TCP Connection lost for BPQ Port %d\r\n", TNC->Port);
			WritetoConsole(ErrMsg);

			SetWindowText(TNC->xIDC_COMMSSTATE, "Connection to TNC lost");
				
			TNC->CONNECTING = FALSE;
			TNC->CONNECTED = FALSE;
			TNC->Alerted = FALSE;

			if (TNC->PTTMode)
				Rig_PTT(TNC->RIG, FALSE);			// Make sure PTT is down

			if (TNC->Streams[0].Attached)
				TNC->Streams[0].ReportDISC = TRUE;

			return 0;

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
KillTNC(struct TNCINFO * TNC)
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

RestartTNC(struct TNCINFO * TNC)
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

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len)
{
	int index = 0;
	UCHAR * ptr1 = Msg, * ptr2;
	UCHAR Line[1000];
	int LineLen, i;

lineloop:

	if (Len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,Len);

		if (ptr2)
		{
			ptr2++;
			LineLen = ptr2 - ptr1;
			Len -= LineLen;
			memcpy(Line, ptr1, LineLen);
			memcpy(&Line[LineLen - 1], "<cr>", 4);
			LineLen += 3;

			if ((*ptr2) == 10)
			{
				memcpy(&Line[LineLen], "<lf>", 4);
				LineLen += 4;
				ptr2++;
				Len --;
			}
			
			Line[LineLen] = 0;

			// If line contains any data above 7f, assume binary and dont display

			for (i = 0; i < LineLen; i++)
			{
				if (Line[i] > 127)
					goto Skip;
			}

			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Line);
		Skip:
			ptr1 = ptr2;

			goto lineloop;

		}

		for (i = 0; i < Len; i++)
		{
			if (ptr1[i] > 127)
				break;
		}

		if (i == Len)
			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) ptr1 );

	}

	if (index > 1200)
						
	do{

		index=index=SendMessage(TNC->hMonitor, LB_DELETESTRING, 0, 0);
			
	} while (index > 1000);

	index=SendMessage(TNC->hMonitor, LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

}
VOID TidyClose(struct TNCINFO * TNC, int Stream)
{
	// If all acked, send disc
	
	if (TNC->Streams[0].BytesOutstanding == 0)
		send(TNC->WINMORSock,"DISCONNECT\r\n", 12, 0);
}

VOID ForcedClose(struct TNCINFO * TNC, int Stream)
{
	send(TNC->WINMORSock,"DIRTYDISCONNECT\r\n", 17, 0);
}

VOID CloseComplete(struct TNCINFO * TNC, int Stream)
{
	ReleaseTNC(TNC);

	if (TNC->FECMode)
	{
		TNC->FECMode = FALSE;
		send(TNC->WINMORSock,"SENDID 0\r\n", 10, 0);
	}
}

