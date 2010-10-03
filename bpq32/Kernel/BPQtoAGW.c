//
//	DLL to provide interface to allow G8BPQ switch to use AGWPE as a Port Driver 
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

unsigned long _beginthread( void( *start_address )( int ), unsigned stack_size, int arglist);

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

//int ResetExtDriver(int num);
extern char * PortConfig[33];

void ConnecttoAGWThread(int port);

void CreateMHWindow();
int Update_MH_List(struct in_addr ipad, char * call, char proto);

static BOOL ReadConfigFile(char * filename, int Port);
int ConnecttoAGW();
int ProcessReceivedData(int bpqport);
static ProcessLine(char * buf, int Port, BOOL CheckPort);

UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
extern UINT FREE_Q;

extern UCHAR BPQDirectory[];

#pragma pack(1)

static struct AGWHEADER
{
	byte Port;
	byte filler1[3];
	char DataKind;
	byte filler2;
	unsigned char PID;
	byte filler3;
	unsigned char callfrom[10];
	unsigned char callto[10];
	int DataLength;
	int reserved;

} AGWHeader;

static struct AGWHEADER RXHeader;


#pragma pack()


#define MAXBPQPORTS 32
#define MAXAGWPORTS 16

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static int AGWChannel[MAXBPQPORTS+1];			// BPQ Port to AGW Port
static int BPQPort[MAXAGWPORTS][MAXBPQPORTS+1];	// AGW Port and Connection to BPQ Port
static int AGWtoBPQ_Q[MAXBPQPORTS+1];			// Frames for BPQ, indexed by BPQ Port
static int BPQtoAGW_Q[MAXBPQPORTS+1];			// Frames for AGW. indexed by AGW port. Only used it TCP session is blocked

//	Each port may be on a different machine. We only open one connection to each AGW instance

static SOCKET AGWSock[MAXBPQPORTS+1];			// Socket, indexed by BPQ Port

static int MasterPort[MAXBPQPORTS+1];			// Pointer to first BPQ port for a specific AGW host

static char * AGWSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static char * AGWHostName[MAXBPQPORTS+1];		// AGW Host - may be dotted decimal or DNS Name

#pragma pack()

static unsigned int AGWInst = 0;
static int AttachedProcesses=0;

static HWND hResWnd,hMHWnd;
static BOOL GotMsg;

static HANDLE STDOUT=0;

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;
static SOCKADDR_IN destaddr[MAXBPQPORTS+1];

static int addrlen=sizeof(sinx);

//static short AGWPort=0;

static time_t ltime,lasttime[MAXBPQPORTS+1];

static BOOL CONNECTING[MAXBPQPORTS+1];
static BOOL CONNECTED[MAXBPQPORTS+1];

//HANDLE hInstance;


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;


static int ExtProc(int fn, int port,unsigned char * buff)
{
	int i,winerr;
	int datalen;
	UINT * buffptr;
	char txbuff[500];

	unsigned int bytes,txlen=0;
	char ErrMsg[255];

	switch (fn)
	{
	case 1:				// poll

		if (MasterPort[port] == port)
		{
			// Only on first port using a host

			if (CONNECTED[port]==FALSE && CONNECTING[port]==FALSE)
			{
				//	See if time to reconnect
		
				time( &ltime );
				if (ltime-lasttime[port] >9 )
				{
					ConnecttoAGW(port);
					lasttime[port]=ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (CONNECTED[port]) FD_SET(AGWSock[port],&readfs);

			
			FD_ZERO(&writefs);

			if (CONNECTING[port]) FD_SET(AGWSock[port],&writefs);	// Need notification of Connect

			if (BPQtoAGW_Q[port]) FD_SET(AGWSock[port],&writefs);	// Need notification of busy clearing



			FD_ZERO(&errorfs);
		
			if (CONNECTING[port] || CONNECTED[port]) FD_SET(AGWSock[port],&errorfs);

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
					if (BPQtoAGW_Q[port] == 0)
					{
				
						//	Connect success

						CONNECTED[port]=TRUE;
						CONNECTING[port]=FALSE;

						// If required, send signon

						if (AGWSignon[port])
							send(AGWSock[port],AGWSignon[port],546,0);

						// Request Raw Frames

						AGWHeader.Port=0;
						AGWHeader.DataKind='k';
						AGWHeader.DataLength=0;

						send(AGWSock[port],(const char FAR *)&AGWHeader,sizeof(AGWHeader),0);
					}
					else
					{
						// Write block has cleared. Send rest of packet

						buffptr=Q_REM(&BPQtoAGW_Q[port]);

						txlen=buffptr[1];

						memcpy(txbuff,buffptr+2,txlen);

						bytes=send(AGWSock[port],(const char FAR *)&txbuff,txlen,0);
					
						ReleaseBuffer(buffptr);

					}

				}
					
				if (errorfs.fd_count == 1)
				{

					//	if connecting, then failed, if connected then has just disconnected

//					if (CONNECTED[port])
					if (!CONNECTING[port])
					{
						i=wsprintf(ErrMsg, "AGW Connection lost for BPQ Port %d\r\n", port);
						WritetoConsole(ErrMsg);
					}

					CONNECTING[port]=FALSE;
					CONNECTED[port]=FALSE;
				
				}

			}

		}

		// See if any frames for this port

		if (AGWtoBPQ_Q[port] !=0)
		{
			buffptr=Q_REM(&AGWtoBPQ_Q[port]);

			datalen=buffptr[1];

			memcpy(&buff[6],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
			datalen+=6;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);

		}

		return (0);



	case 2:				// send

		
		if (!CONNECTED[MasterPort[port]]) return 0;		// Don't try if not connected

		if (BPQtoAGW_Q[MasterPort[port]]) return 0;		// Socket is blocked - just drop packets
														// till it clears

		// AGW has a control byte on front, so only subtract 6 from BPQ length

		txlen=(buff[6]<<8) + buff[5]-6;	
		
		AGWHeader.Port=AGWChannel[port];
		AGWHeader.DataKind='K';				// raw send
		AGWHeader.DataLength=txlen;

		memcpy(&txbuff,&AGWHeader,sizeof(AGWHeader));
		memcpy(&txbuff[sizeof(AGWHeader)],&buff[6],txlen);
		txbuff[sizeof(AGWHeader)]=0;
		
		txlen+=sizeof(AGWHeader);

		bytes=send(AGWSock[MasterPort[port]],(const char FAR *)&txbuff,txlen,0);
		
		if (bytes != txlen)
		{

			// AGW doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
				winerr=WSAGetLastError();
				
				i=wsprintf(ErrMsg, "AGW Write Failed for port %d - error code = %d\r\n", port, winerr);
				WritetoConsole(ErrMsg);
					
	
//				if (winerr != WSAEWOULDBLOCK)
//				{
				
					closesocket(AGWSock[MasterPort[port]]);
					
					CONNECTED[MasterPort[port]]=FALSE;

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

//				closesocket(AGWSock[MasterPort[port]]);
					
//				CONNECTED[MasterPort[port]]=FALSE;

//				return (0);
//			}
	
//			buffptr[1]=txlen-bytes;			// Bytes still to send

//			memcpy(buffptr+2,&txbuff[bytes],txlen-bytes);

//			C_Q_ADD(&BPQtoAGW_Q[MasterPort[port]],buffptr);
	
//			return (0);
		}


		return (0);

	


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK
			
		break;

	case 4:				// reinit

//		return(ReadConfigFile("BPQAXIP.CFG"));

		return (0);
	}
	return 0;
}

UINT WINAPI AGWExtInit(struct PORTCONTROL *  PortEntry)

{
	int i, port;
	char Msg[255];
	
	//
	//	Will be called once for each AGW port to be mapped to a BPQ Port
	//	The AGW port number is in CHANNEL - A=0, B=1 etc
	//
	//	The Socket to connect to is in IOBASE
	//

	port=PortEntry->PORTNUMBER;

	ReadConfigFile("BPQtoAGW.CFG", port);

	AGWChannel[port]=PortEntry->CHANNELNUM-65;

	if (destaddr[port].sin_family == 0)
	{
		// not defined in config file

		destaddr[port].sin_family = AF_INET;
		destaddr[port].sin_port = htons(PortEntry->IOBASE);

		AGWHostName[port]=malloc(10);

		if (AGWHostName[port] != NULL) 
			strcpy(AGWHostName[port],"127.0.0.1");

	}

	i=wsprintf(Msg,"AGW Port %d Host %s %d",AGWChannel[port]+1,AGWHostName[port],htons(destaddr[port].sin_port));
	WritetoConsole(Msg);

	// See if we already have a port for this host
	

	MasterPort[port]=port;

	for (i=1;i<port;i++)
	{
		if (i == port) continue;

		if (destaddr[i].sin_port == destaddr[port].sin_port &&
			 _stricmp(AGWHostName[i],AGWHostName[port]) == 0)
		{
			MasterPort[port]=i;
			break;
		}
	}


	BPQPort[PortEntry->CHANNELNUM-65][MasterPort[port]]=PortEntry->PORTNUMBER;
			
	if (MasterPort[port] == port)
		ConnecttoAGW(port);

	time(&lasttime[port]);			// Get initial time value

	
	return ((int) ExtProc);

}

/*

#	Config file for BPQtoAGW
#
#	For each AGW port defined in BPQCFG.TXT, Add a line here
#	Format is BPQ Port, Host/IP Address, Port

#
#	Any unspecified Ports will use 127.0.0.1 and port for BPQCFG.TXT IOADDR field
#

1 127.0.0.1 8000
2 127.0.0.1 8001

*/


BOOL ReadConfigFile(char * fn, int Port)
{
	FILE *file;
	char buf[256],errbuf[256];

	UCHAR Value[100];
	char * Config;

	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		char * ptr1 = Config, * ptr2;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf, Port, FALSE))
			{
				WritetoConsole("BPQEther - Bad config record ");
				WritetoConsole(errbuf);
			}
		}
		return (TRUE);
	}

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
			
	if ((file = fopen(Value,"r")) == NULL)
	{
//		n=wsprintf(buf,"BPQtoAGW - Config file %s not found ",Value);
//		WriteFile(STDOUT,buf,n,&n,NULL);

		return (TRUE);			// Dont need it at the moment
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf, Port, TRUE))
		{
			WritetoConsole("BPQtoAGW - Bad config record ");
			WritetoConsole(errbuf);
		}
				
	}

	fclose(file);

	return (TRUE);

}

ProcessLine(char * buf, int Port, BOOL CheckPort)
{
	char * ptr,* p_user,* p_password;
	char * p_ipad;
	char * p_udpport;
//	unsigned long ipad;
	unsigned short AGWport;
	int BPQport;
	int len=510;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if (CheckPort)
	{
		p_ipad = strtok(NULL, " \t\n\r");
			
		if (p_ipad == NULL) return (FALSE);

		BPQport = atoi(ptr);

		if (Port != BPQport) return TRUE;		// Not for us
	}
	else
	{
		BPQport = Port;
		p_ipad = ptr;
	}
	if(BPQport > 0 && BPQport <33)
	{	
		p_udpport = strtok(NULL, " \t\n\r");
			
		if (p_udpport == NULL) return (FALSE);

		AGWport = atoi(p_udpport);

		destaddr[BPQport].sin_family = AF_INET;
		destaddr[BPQport].sin_port = htons(AGWport);

		AGWHostName[BPQport]=malloc(strlen(p_ipad)+1);

		if (AGWHostName[BPQport] == NULL) return TRUE;

		strcpy(AGWHostName[BPQport],p_ipad);

		p_user = strtok(NULL, " \t\n\r");
			
		if (p_user == NULL) return (TRUE);

		p_password = strtok(NULL, " \t\n\r");
			
		if (p_password == NULL) return (TRUE);

		// Allocate buffer for signon message

		AGWSignon[BPQport]=malloc(546);

		if (AGWSignon[BPQport] == NULL) return TRUE;

		memset(AGWSignon[BPQport],0,546);

		AGWSignon[BPQport][4]='P';

		memcpy(&AGWSignon[BPQport][28],&len,4);

		strcpy(&AGWSignon[BPQport][36],p_user);

		strcpy(&AGWSignon[BPQport][291],p_password);

		return (TRUE);
	}

	//
	//	Bad line
	//
	return (FALSE);
	
}
	
int ConnecttoAGW(int port)
{
	_beginthread(ConnecttoAGWThread,0,port);

	return 0;
}

VOID ConnecttoAGWThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;

	//	Only called for the first BPQ port for a particular host/port combination

	destaddr[port].sin_addr.s_addr = inet_addr(AGWHostName[port]);

	if (destaddr[port].sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (AGWHostName[port]);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&destaddr[port].sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(AGWSock[port]);

	AGWSock[port]=socket(AF_INET,SOCK_STREAM,0);

	if (AGWSock[port] == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for AGW socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	ioctlsocket (AGWSock[port],FIONBIO,&param);
 
	setsockopt (AGWSock[port],SOL_SOCKET,SO_REUSEADDR,(const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(AGWSock[port], (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for AGW socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	if (connect(AGWSock[port],(LPSOCKADDR) &destaddr[port],sizeof(destaddr)) == 0)
	{

		//
		//	Connected successful
		//

		CONNECTED[port]=TRUE;

		return;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//
			//	Connect in Progressing
			//

			CONNECTING[port]=TRUE;
			return;
		}
		else
		{
			//
			//	Connect failed
			//
    		i=wsprintf(Msg, "Connect Failed for AGW socket - error code = %d\r\n", err);
			WritetoConsole(Msg);

			return;
		}
	}
	return;		// Not Used

}

int ProcessReceivedData(int port)
{
	unsigned int bytes;
	int datalen,i;
	char ErrMsg[255];
	char Message[500];
	UINT * buffptr;

	//	Need to extract messages from byte stream

	//	Use MSG_PEEK to ensure whole message is available

	bytes = recv(AGWSock[port],(char *)&RXHeader,sizeof(RXHeader),MSG_PEEK);

	if (bytes == SOCKET_ERROR)
	{
		i=wsprintf(ErrMsg, "Read Failed for AGW socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(ErrMsg);
				
		closesocket(AGWSock[port]);
					
		CONNECTED[port]=FALSE;

		return (0);
	}

	if (bytes == 0)
	{
		//	zero bytes means connection closed

		i=wsprintf(ErrMsg, "AGW Connection closed for BPQ Port %d\r\n", port);
		WritetoConsole(ErrMsg);


		CONNECTED[port]=FALSE;
		return (0);
	}

	//	Have some data
	
	if (bytes == sizeof(RXHeader))
	{
		
		//	Have a header - see if we have any associated data
		
		datalen=RXHeader.DataLength;

		if (datalen > 0)
		{

			// Need data - See if enough there
			
			
			bytes = recv(AGWSock[port],(char *)&Message,sizeof(RXHeader)+datalen,MSG_PEEK);
		}

		if (bytes == sizeof(RXHeader)+datalen)
		{
			bytes = recv(AGWSock[port],(char *)&RXHeader,sizeof(RXHeader),0);

			if (datalen > 0)
			{
				bytes = recv(AGWSock[port],(char *)&Message,datalen,0);
			}

			// Have header, and data if needed

			// Only use frame type 

			if (RXHeader.DataKind == 'K')				// raw data
			{
				
				//	Make sure it is for a port we want - we may not be using all AGW ports

				if (BPQPort[RXHeader.Port][MasterPort[port]] == 0)
					
					return (0);

				// Get a buffer
						
				buffptr=GetBuff();

				if (buffptr == 0) return (0);			// No buffers, so ignore


	
				buffptr[1]=datalen;
				memcpy(buffptr+2,&Message,datalen);

				C_Q_ADD(&AGWtoBPQ_Q[BPQPort[RXHeader.Port][MasterPort[port]]],buffptr);

			}

			return (0);
		}

		// Have header, but not sufficient data

		return (0);
	
	}

	// Dont have at least header bytes
	
	return (0);

}

