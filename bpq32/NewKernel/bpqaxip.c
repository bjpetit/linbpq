//
//	DLL to provide AXIP support for G8BPQ switch in a 
//	32bit environment,
//
//	Uses BPQ EXTERNAL interface
//

//	Version 1.1 August 2001				   
//
//		Send to all matching entries in map table 
//		(Mainly for NODES braodcasts to multiple stations)
//

//	Version 1.2 September 2001
//
//		Support UDP as well as raw IP

//	Version 1.3 October 2001
//
//		Allow host names as well as numeric IP addresses
//
//  Version 1.4 November 2002
//
//		Implement keepalive for NAT routers
//

//  Version 1.5 December 2004
//
//		Implement a "MHEARD" facility
//


//  Version 1.6	August 2005
//
//		Treat NULL string in Registry as use current directory


//  Version 1.7	December 2005
//
//		Create a separate thread to open sockets to avoid hang on XP SP2


//  Version 1.8	January 2006
//
//		Get config file location from Node (will check bpq directory)


//  Version 1.9	March 2006
//
//		Allow multiple listening UDP ports
//		Kick off resolver on EXTRESTART
//		Remove redundant DYNAMIC processing
 
//  Version 1.10 October 2006
//
//		Add "Minimize to Tray" option
//		Write diagnostics to BPQ console window instead of STDOUT

//  Version 1.11 October 2007
//
//		Sort MHeard and discard last entry if full
//		Add Commands to re-read config file and manually add an ARP entry 

//  Version 1.12 February 2008
//
//		Check received length
//		Changes for unload of bpq32.dll
//			Add Close Driver function
//			Dynamic Load of bpq32.dll

//	Version 1.13 October 2008
//
//		Add Linux-style config of broadcast addressess

//	Version 1.13.2 January 2009
//
//		Add Start Minimized Option

//	Version 1.13.3 February 2009
//
//		Save Window positions

//	Version 1.13.4 March 2009
//
//		Fix loop on config file error

// Version 1.14.1 April 2009
//
//		Add option to reject messages if sender is not in ARP Table
//		Add option to add received calls to ARP Table

// Version 1.15.1 May 2009
//
//		Add IP/TCP option

// Version 1.15.2 August 2009
//
//		Extra Debug Output in TCP Mode
//		Fix problem if TCP entry was first in table
//		Include TCP sessions in MHEARD
//		Add T flag to Resolver window fot TCP Sessions
//		Set SO_KEEPALIVE and SO_CONDITIONAL_ACCEPT socket options

// Version 1.15.4 August 2009

//		Recycle Listening Socket if no connect for 60 mins
//		Clear data connections if no data for 60 mins
//		Repaint MH Window after clear.

// Version 1.15.5 Spetmber 2010

//		Add option to get config from bpq32.dll
//		Moved to BPQ32.dll - no separate version number onw.

//	October 2010

//		Allow multiple axip ports.

//  June 2011

//		Add IPv6 support

#define _CRT_SECURE_NO_DEPRECATE

#include "winsock2.h"
#include "WS2tcpip.h"
#include "windows.h"
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "winver.h"
#include "SHELLAPI.H"

#include "kernelresource.h"
#include "bpq32.h"
#include "AsmStrucs.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

#define BUFFLEN	360	

//	BUFFLEN-4 = L2 POINTER (FOR CLEARING TIMEOUT WHEN ACKMODE USED)
//	BUFFLEN-8 = TIMESTAMP
//	BUFFLEN-12 = BUFFER ALLOCATED FLAG (ADDR OF ALLOCATING ROUTINE)
	
#define MAXDATA	BUFFLEN-16


#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

VOID * zalloc(int len);

int ResetExtDriver(int num);
BOOL ProcessConfig();
VOID FreeConfig();
extern char * PortConfig[35];

extern UCHAR BPQDirectory[];

extern HKEY REGTREE;

extern HWND ClientWnd, FrameWnd;
extern HMENU hMainFrameMenu, hBaseMenu, hWndMenu;
extern HBRUSH bgBrush;

extern int OffsetH, OffsetW;

void ResolveNames(struct PORTINFO * PORT);
void OpenSockets(struct PORTINFO * PORT);
void CloseSockets();


static int CONVFROMAX25(char * incall, char * outcall);
void CreateMHWindow(struct PORTINFO * PORT);
int Update_MH_List(struct PORTINFO * PORT, UCHAR * ipad, char * call, char proto, short port, BOOL IPv6);
int Update_MH_KeepAlive(struct PORTINFO * PORT, struct in_addr ipad, char proto, short port);
unsigned short int compute_crc(unsigned char *buf,int l);
unsigned int find_arp(unsigned char * call);
BOOL add_arp_entry(struct PORTINFO * PORT, unsigned char * call, UCHAR * ip, int len, int port,unsigned char * name,
		int keepalive, BOOL BCFlag, BOOL AutoAdded, int TCPMode, int SourcePort, BOOL IPv6);
BOOL add_bc_entry(struct PORTINFO * PORT, unsigned char * call, int len);
BOOL convtoax25(unsigned char * callsign, unsigned char * ax25call, int * calllen);
BOOL ReadConfigFile(Port);
int ProcessLine(char * buf, struct PORTINFO * PORT);
int CheckKeepalives(struct PORTINFO * PORT);
BOOL CopyScreentoBuffer(char * buff, struct PORTINFO * PORT);
int DumpFrameInHex(unsigned char * msg, int len);
VOID SendFrame(struct PORTINFO * PORT, struct arp_table_entry * arp_table, UCHAR * buff, int txlen);
BOOL CheckSourceisResolvable(struct PORTINFO * PORT, char * call, int Port, VOID * rxaddr);
int DataSocket_Read(struct arp_table_entry * sockptr, SOCKET sock);
int GetMessageFromBuffer(struct PORTINFO * PORT, char * Buffer);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
int Socket_Accept(int SocketId);
int Socket_Connect(int SocketId, int Error);
int Socket_Data(int sock, int error, int eventcode);
VOID TCPConnectThread(struct arp_table_entry * arp);
VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Consoleprintf(const char * format, ...);
BOOL OpenListeningSocket(struct PORTINFO * PORT, struct arp_table_entry * arp);
VOID Format_Addr(unsigned char * Addr, char * Output, BOOL IPV6);
static void CreateResolverWindow(struct PORTINFO * PORT);
VOID SaveMDIWindowPos(HWND hWnd, char * RegKey, char * Value, BOOL Minimized);


#pragma pack(1) 

#define MAX_ENTRIES 128
#define MaxMHEntries 40
#define MAX_BROADCASTS 8
#define MAXUDPPORTS 30 

struct arp_table_entry
{
	unsigned char callsign[7];
	unsigned char len;			// bytes to compare (6 or 7)
	BOOL IPv6;

//	union
//	{
//		struct in_addr in_addr;
//		unsigned int ipaddr;
//		struct in6_addr in6_addr;
//	};

	unsigned short port;
	unsigned char hostname[64];
	unsigned int error;
	BOOL ResolveFlag;			// True if need to resolve name
	unsigned int keepalive;
	unsigned int keepaliveinit;
	BOOL BCFlag;				// True if we want broadcasts to got to this call
	BOOL AutoAdded;				// Set if Entry created as a result of AUTOADDMAP
	SOCKET TCPListenSock;			// Listening socket if slave
	SOCKET TCPSock;
	int  TCPMode;				// TCPMaster ot TCPSlave
	UCHAR * TCPBuffer;			// Area for building TCP message from byte stream
	int InputLen;				// Bytes in TCPBuffer

	union
	{
		SOCKADDR_IN6 destaddr6;  
		SOCKADDR_IN destaddr;
	};

	BOOL TCPState;
	UINT TCPThreadID;			// Thread ID if TCP Master
	UINT TCPOK; 				// Cleared when Message RXed . Incremented by timer
	int SourceSocket;			// The socket to use (sets the from Address
	struct PORTINFO * PORT;
};


struct broadcast_table_entry
{
	unsigned char callsign[7];
	unsigned char len;			// bytes to compare (6 or 7)
};


struct MHTableEntry
{
	unsigned char callsign[7];
	char proto;
	short port;
	union
	{
		struct in_addr ipaddr;
		struct in_addr6 ipaddr6;
	};
	time_t	LastHeard;		// Time last packet received
	int Keepalive;
	BOOL IPv6;
};


struct PORTINFO
{
	int Port;

	struct MHTableEntry MHTable[MaxMHEntries];
	struct broadcast_table_entry BroadcastAddresses[MAX_BROADCASTS];

	int NumberofBroadcastAddreses;
	BOOL Checkifcanreply;
	
	int arp_table_len;
	int ResolveIndex;			// pointer to entry being resolved

	struct arp_table_entry arp_table[MAX_ENTRIES];

	struct arp_table_entry default_arp;

	BOOL MHEnabled;
	BOOL MHAvailable;			// Enabled with config file directive

	BOOL AutoAddARP;

	unsigned char  hostaddr[64];

	HWND hResWnd, hMHWnd, ConfigWnd;

	BOOL GotMsg;

	int udpport[MAXUDPPORTS+2];
	BOOL IPv6[MAXUDPPORTS+2];

	int NumberofUDPPorts;

	BOOL needip;
	BOOL NeedResolver;
	BOOL NeedTCP;

	SOCKET sock;						// IP 93 Sock
	SOCKET udpsock[MAXUDPPORTS+2];

	time_t ltime,lasttime;
	int baseline;

	char buf[MAXGETHOSTSTRUCT];

	int MaxMHWindowlength;
	int MaxResWindowlength;

	BOOL ResMinimized;
	BOOL MHMinimized;

	HMENU hResMenu;
	HMENU hMHMenu;

};

extern BOOL StartMinimized;

union
{
	SOCKADDR_IN sinx; 
	SOCKADDR_IN6 sinx6; 
} sinx;
/*
union
{
	SOCKADDR_IN destaddr;
	SOCKADDR_IN6 destaddr6;
} destaddr;
*/

#define IP_AXIP 93				   // IP Protocol for AXIP

#pragma pack(1) 

struct iphdr {
//	unsigned int version:4;        // Version of IP
//	unsigned int h_len:4;          // length of the header
	unsigned char h_lenvers;       // Version + length of the header
	unsigned char tos;             // Type of service
	unsigned short total_len;      // total length of the packet
	unsigned short ident;          // unique identifier
	unsigned short frag_and_flags; // flags
	unsigned char  ttl; 
	unsigned char proto;           // protocol (TCP, UDP etc)
	unsigned short checksum;       // IP checksum

	unsigned int sourceIP;
	unsigned int destIP;

};

#pragma pack()


#define TCPMaster 1
#define TCPSlave 2

#define TCPListening 1
#define TCPConnecting 2
#define TCPConnected 4


LOGFONT LFTTYFONT ;

extern HFONT hFont ;

//struct tagMSG Msg;

//char buf[MAXGETHOSTSTRUCT];

#pragma pack()

int addrlen6 = sizeof(SOCKADDR_IN6);
int addrlen = sizeof(SOCKADDR_IN);

extern short CRCTAB;

unsigned int AXIPInst = 0;


DWORD n;

struct PORTINFO * Portlist[33];

int InitAXIP(int Port);

RECT ResRect;
RECT MHRect;

int CurrentMHEntries;
int CurrentResEntries;


static char ConfigClassName[]="CONFIG";

HANDLE hInstance;

VOID SaveAXIPWindowPos(int port)
{
	struct PORTINFO * PORT;
	char Key[80];

	PORT = Portlist[port];
		
	if (PORT == NULL)
		return;

	wsprintf(Key, "PACTOR\\PORT%d", port);

	SaveMDIWindowPos(PORT->hMHWnd, Key, "MHSize", PORT->MHMinimized);
	SaveMDIWindowPos(PORT->hResWnd, Key, "ResSize", PORT->ResMinimized);

	return;
}


static int ExtProc(int fn, int port,unsigned char * buff)
{
	struct iphdr * iphdrptr;
	int len,txlen=0,err,index,digiptr,i;
	unsigned short int crc;
	char rxbuff[500];
	char axcall[7];
	char errmsg[100];
	union
	{
		SOCKADDR_IN rxaddr;
		SOCKADDR_IN6 rxaddr6;
	} RXaddr;
	struct PORTINFO * PORT = Portlist[port];

	switch (fn)
	{
	case 1:				// poll

		//
		//	Check Keepalive timers
		//
		time( &PORT->ltime );
		if (PORT->ltime-PORT->lasttime >9 )
		{
			CheckKeepalives(PORT);
			PORT->lasttime=PORT->ltime;
		}

		if (PORT->needip)
		{
			len = recvfrom(PORT->sock,rxbuff,500,0,(LPSOCKADDR)&RXaddr.rxaddr,&addrlen);

			if (len == -1)
			{		
				err = WSAGetLastError();
			}
			else
			{
				iphdrptr=(struct iphdr *)&rxbuff;

				if (len == ntohs(iphdrptr->total_len))
				{
					len-=20;			// IP HEADER

					if (memcmp(&rxbuff[20], "Keepalive", 9) == 0 )
					{
						if (PORT->MHEnabled)
							Update_MH_KeepAlive(PORT, RXaddr.rxaddr.sin_addr,'I',93);
	
						return 0;
					}
					crc = compute_crc(&rxbuff[20], len);

					if (crc == 0xf0b8)		// Good CRC
					{
						len-=2;			// Remove CRC
					
						if (len > MAXDATA)
						{
							wsprintf(errmsg,"BPQAXIP Invalid Msg Len=%d Source=%s",len,inet_ntoa(RXaddr.rxaddr.sin_addr));
							OutputDebugString(errmsg);
							DumpFrameInHex(&rxbuff[20], len);
							return 0;
						}

						memcpy(&buff[7],&rxbuff[20],len);
						len+=7;
						buff[5]=(len & 0xff);
						buff[6]=(len >> 8);
		
						//
						//	Do MH Proccessing if enabled
						//

						if (PORT->MHEnabled)
							Update_MH_List(PORT, &RXaddr.rxaddr.sin_addr.s_net, &buff[14], 'I', 93, 0);

						if (PORT->Checkifcanreply)
						{
							char call[7];

							memcpy(call, &buff[14], 7);
							call[6] &= 0x7e;		// Mask End of Address bit

							if (CheckSourceisResolvable(PORT, call, 0, &RXaddr))

								return 1;

							else
								// Can't reply. If AutoConfig is set, add to table and accept, else reject

								if (PORT->AutoAddARP)

									return add_arp_entry(PORT, call, &RXaddr.rxaddr.sin_addr.s_net, 7, 0, inet_ntoa(RXaddr.rxaddr.sin_addr), 0, TRUE, TRUE, 0, 0, FALSE);

								else

									return 0;
						}
						else
							return(1);
					}
					//
					//	CRC Error
					//
						
					wsprintf(errmsg,"BPQAXIP Invalid CRC=%d Source=%s",crc,inet_ntoa(RXaddr.rxaddr.sin_addr));
						OutputDebugString(errmsg);

					return (0);
				}

				//
				//	Bad Length
				//
	
				return (0);
			}
		}

		for (i=0;i<PORT->NumberofUDPPorts;i++)
		{
		
			if (PORT->IPv6[i])
				len = recvfrom(PORT->udpsock[i],rxbuff,500,0,(LPSOCKADDR)&RXaddr.rxaddr, &addrlen6);
			else
				len = recvfrom(PORT->udpsock[i],rxbuff,500,0,(LPSOCKADDR)&RXaddr.rxaddr, &addrlen);
	
			if (len == -1)
			{		
				err = WSAGetLastError();
			}
			else
			{
				if (memcmp(rxbuff, "Keepalive", 9) == 0 )
				{
					if (PORT->MHEnabled)
						Update_MH_KeepAlive(PORT, RXaddr.rxaddr.sin_addr, 'U', PORT->udpport[i]);
	
					continue;
				}
				
				crc = compute_crc(&rxbuff[0], len);

				if (crc == 0xf0b8)		// Good CRC
				{
					len-=2;				// Remove CRC

					if (len > MAXDATA)
					{
						wsprintf(errmsg,"BPQAXIP Invalid Msg Len=%d Source=%s Port %d",len,inet_ntoa(RXaddr.rxaddr.sin_addr),PORT->udpport[i]);
						OutputDebugString(errmsg);
						DumpFrameInHex(&rxbuff[0], len);
						return 0;
					}

					memcpy(&buff[7],&rxbuff[0],len);
					len+=7;
					buff[5]=(len & 0xff);
					buff[6]=(len >> 8);

					//
					//	Do MH Proccessing if enabled
					//

					if (PORT->MHEnabled)
						if (PORT->IPv6[i])
							Update_MH_List(PORT, (UCHAR *)&RXaddr.rxaddr6.sin6_addr, &buff[14], 'U', PORT->udpport[i], TRUE);	
						else
							Update_MH_List(PORT, &RXaddr.rxaddr.sin_addr.s_net, &buff[14], 'U', PORT->udpport[i], FALSE);	

					if (PORT->Checkifcanreply)
					{
						char call[7];
 
						memcpy(call, &buff[14], 7);
						call[6] &= 0x7e;		// Mask End of Address bit

						if (CheckSourceisResolvable(PORT, call, htons(RXaddr.rxaddr.sin_port), &RXaddr))
							return 1;
						else
						{
							// Can't reply. If AutoConfig is set, add to table and accept, else reject
		
							if (PORT->AutoAddARP)
								if (PORT->IPv6[i])
								{
									char Addr[80];
									Format_Addr((UCHAR *)&RXaddr.rxaddr6.sin6_addr, Addr, TRUE);
									return add_arp_entry(PORT, call, (UCHAR *)&RXaddr.rxaddr6.sin6_addr, 7, htons(RXaddr.rxaddr6.sin6_port), Addr, 0, TRUE, TRUE, 0, PORT->udpport[i], TRUE);		
								}
								else
									return add_arp_entry(PORT, call, &RXaddr.rxaddr.sin_addr.s_net, 7, htons(RXaddr.rxaddr.sin_port), inet_ntoa(RXaddr.rxaddr.sin_addr), 0, TRUE, TRUE, 0, PORT->udpport[i], FALSE);		
							else
								return 0;
						}
					}
					else
						return(1);
				}

				//	
				//	CRC Error
				//

				wsprintf(errmsg,"BPQAXIP Invalid CRC=%d Source=%s Port %d",crc,inet_ntoa(RXaddr.rxaddr.sin_addr),PORT->udpport[i]);
				Debugprintf(errmsg);
				rxbuff[len] = 0;
				Debugprintf(rxbuff);


				return (0);
			}
		}

		if (PORT->NeedTCP)
		{
			len = GetMessageFromBuffer(PORT, rxbuff);

			if (len)
			{
				len = KissDecode(rxbuff, len-1);		// Len includes FEND
				len -= 2;	// Ignore Cheksum

				memcpy(&buff[7],&rxbuff[0],len);
				len+=7;
				buff[5]=(len & 0xff);
				buff[6]=(len >> 8);

				return 1;
			}
		}

		return (0);
		
	case 2:				// send

		txlen=(buff[6]<<8) + buff[5] - 5;			// Len includes buffer header (7) but we add crc

		crc=compute_crc(&buff[7], txlen - 2);
		crc ^= 0xffff;

		buff[txlen+5]=(crc&0xff);
		buff[txlen+6]=(crc>>8);

 		memcpy(axcall, &buff[7], 7);	// Set to send to dest addr

		// if digis are present, scan down list for first non-used call

		if  ((buff[20] & 1) == 0)
		{
			// end of addr bit not set, so scan digis

			digiptr=21;							// start of first digi

			while (((buff[digiptr+6] & 0x80) == 0x80) && ((buff[digiptr+6] & 0x1) == 0))
			{
				// This digi has been used, and it is not the last

				digiptr+=7;
			}

			// if this has not been used, use it

			if ((buff[digiptr+6] & 0x80) == 0)
				memcpy(axcall,&buff[digiptr],7);  // get next call
		}

		axcall[6] &= 0x7e;

//		If addresses to a broadcast address, send to all entries

		for (i=0; i< PORT->NumberofBroadcastAddreses; i++)
		{
			if (memcmp(axcall, PORT->BroadcastAddresses[i].callsign, 7) == 0)
			{
				for (index = 0; index < PORT->arp_table_len; index++)
				{
					if (PORT->arp_table[index].BCFlag) SendFrame(PORT, &PORT->arp_table[index], &buff[7], txlen);
				}
				return 0;
			}
		}

//		Send to all matching calls in arp table

		index = 0;

		while (index < PORT->arp_table_len)
		{
			if (memcmp(PORT->arp_table[index].callsign,axcall,PORT->arp_table[index].len) == 0)
			{
				SendFrame(PORT, &PORT->arp_table[index], &buff[7], txlen);
			}
			index++;
		}
		return (0);

	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		CloseSockets(PORT);
		ReadConfigFile(port);
		_beginthread(OpenSockets, 0, NULL );
		PostMessage(PORT->hResWnd, WM_TIMER,0,0);

		break;

	case 5:				// Terminate

		CloseSockets(PORT);
		SendMessage(PORT->hMHWnd, WM_CLOSE, 0, 0);
		SendMessage(PORT->hResWnd, WM_CLOSE, 0, 0);

		break;
	}
	return (0);
}

VOID SendFrame(struct PORTINFO * PORT, struct arp_table_entry * arp_table, UCHAR * buff, int txlen)
{				
	int txsock, i;

	if (arp_table->TCPMode)
	{
		if (arp_table->TCPState == TCPConnected)
		{
			char outbuff[1000];
			int newlen;

			newlen = KissEncode(buff, outbuff, txlen);
			send(arp_table->TCPSock, outbuff, newlen, 0);
		}

		return;
	}

	// Set up source port if not already done

	if (arp_table->SourceSocket == 0)
	{

	// First Set Default for Protocol

	for (i = 0; i < PORT->NumberofUDPPorts; i++)
	{
		if (PORT->IPv6[i] == arp_table->IPv6)
		{
			arp_table->SourceSocket = PORT->udpsock[i];	// Use as source socket, therefore source port
			break;
		}
	}

	for (i = 0; i < PORT->NumberofUDPPorts; i++)
	{
		if (PORT->udpport[i] == arp_table->port && PORT->IPv6[i] == arp_table->IPv6)
		{
			arp_table->SourceSocket = PORT->udpsock[i];	// Use as source socket, therefore source port
			break;
		}
	}
	}
			
	if (arp_table->error == 0)
	{
		if (arp_table->port == 0) txsock = PORT->sock; else txsock = arp_table->SourceSocket;

		if (sendto(txsock, buff, txlen, 0, (struct sockaddr *)&arp_table->destaddr6, sizeof(arp_table->destaddr6))== -1)
			i = GetLastError();
			
		// reset Keepalive Timer
					
		arp_table->keepalive=arp_table->keepaliveinit;
	}
}
		
UINT WINAPI AXIPExtInit(struct PORTCONTROL *  PortEntry)
{	
	WritetoConsole("AXIP ");

	InitAXIP(PortEntry->PORTNUMBER);

	WritetoConsole("\n");

	return ((int) ExtProc);
}

InitAXIP(int Port)
{
	struct PORTINFO * PORT;

	//
	//	Read config first, to get UDP info if needed
	//

	if (!ReadConfigFile(Port))
		return (FALSE);

	PORT = Portlist[Port];

	if (PORT == NULL)
		return FALSE;

	PORT->Port = Port;

	//
    //	Start Resolver Thread if needed
	//


	if (PORT->NeedResolver)
	{
		CreateResolverWindow(PORT);
		_beginthread(ResolveNames, 0, PORT );
	}

	time(&PORT->lasttime);			// Get initial time value
 
	_beginthread(OpenSockets, 0, PORT );

	// Start TCP outward connect threads
	//
	//	Open MH window if needed
	
	if (PORT->MHEnabled)
		CreateMHWindow(PORT);

	return (TRUE);	
}

void OpenSockets(struct PORTINFO * PORT)
{
	char Msg[255];
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	int i;
	int index = 0;
	struct arp_table_entry * arp;

	// Moved from InitAXIP, to avoid hang if started too early on XP SP2

	//
	//	Create and bind socket
	//

	if (PORT->needip)
	{
		PORT->sock=socket(AF_INET,SOCK_RAW,IP_AXIP);

		if (PORT->sock == INVALID_SOCKET)
		{
			MessageBox(NULL, (LPSTR) "Failed to create RAW socket",NULL,MB_OK);
			err = WSAGetLastError();
  	 		return; 
		}

		ioctlsocket (PORT->sock,FIONBIO,&param);
 
		setsockopt (PORT->sock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

		sinx.sinx.sin_family = AF_INET;
		sinx.sinx.sin_addr.s_addr = INADDR_ANY;
		sinx.sinx.sin_port = 0;

		if (bind(PORT->sock, (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
		{
			//
			//	Bind Failed
			//
			err = WSAGetLastError();
			wsprintf(Msg, "Bind Failed for RAW socket - error code = %d", err);
			MessageBox(NULL,Msg,NULL, MB_OK);
			return;
		}
	}

	for (i=0;i<PORT->NumberofUDPPorts;i++)
	{
		if (PORT->IPv6[i])
			PORT->udpsock[i]=socket(AF_INET6,SOCK_DGRAM,0);
		else
			PORT->udpsock[i]=socket(AF_INET,SOCK_DGRAM,0);

		if (PORT->udpsock[i] == INVALID_SOCKET)
		{
			MessageBox(NULL, (LPSTR) "Failed to create UDP socket",NULL,MB_OK);
			err = WSAGetLastError();
  	 		return; 
		}

		ioctlsocket (PORT->udpsock[i],FIONBIO,&param);
 
		setsockopt (PORT->udpsock[i],SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

		if (PORT->IPv6[i])
		{
			sinx.sinx.sin_family = AF_INET6;
			memset (&sinx.sinx6.sin6_addr, 0, 16);
		}
		else
		{
			sinx.sinx.sin_family = AF_INET;
			sinx.sinx.sin_addr.s_addr = INADDR_ANY;
		}
		
		sinx.sinx.sin_port = htons(PORT->udpport[i]);

		if (bind(PORT->udpsock[i], (LPSOCKADDR) &sinx.sinx, sizeof(sinx)) != 0 )
		{
			char Title[20];

			//	Bind Failed

			err = WSAGetLastError();
			wsprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
			wsprintf(Title, "AXIP Port %d", PORT->Port);
			MessageBox(NULL, Msg, Title, MB_OK);
			return;
		}
	}

	// Open any TCP sockets

	while (index < PORT->arp_table_len)
	{
		arp = &PORT->arp_table[index++];

		if (arp->TCPMode == TCPMaster)
		{
			arp->TCPBuffer=malloc(4000);
			arp->TCPState = 0;

			if (arp->TCPThreadID == 0)
			{
				arp->TCPThreadID = _beginthread(TCPConnectThread, 0, arp);
				Debugprintf("TCP Connect thread created for %s Handle %x", arp->hostname, arp->TCPThreadID);
			}
			continue;
		}


		if (arp->TCPMode == TCPSlave)
		{
			OpenListeningSocket(PORT, arp);
		}
	}
}	
OpenListeningSocket(struct PORTINFO * PORT, struct arp_table_entry * arp)
{
	char Msg[255];
	PSOCKADDR_IN psin;
	BOOL bOptVal = TRUE;
	SOCKADDR_IN local_sin;  /* Local socket - internet style */
	int status;

	arp->TCPBuffer=malloc(4000);
	arp->TCPState = 0;

	// create the socket. Set to listening mode if Slave

	arp->TCPListenSock = socket(AF_INET, SOCK_STREAM, 0);

	if (arp->TCPListenSock == INVALID_SOCKET)
	{
		sprintf(Msg, "socket() failed error %d", WSAGetLastError());
		MessageBox(PORT->hResWnd, Msg, "BPQAXIP", MB_OK);
		return FALSE;
	}

	Debugprintf("TCP Listening Socket Created - socket %d  port %d ", arp->TCPListenSock, arp->port);

	if (setsockopt(arp->TCPListenSock, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&bOptVal, 4) != SOCKET_ERROR)
	{
		Debugprintf("Set SO_CONDITIONAL_ACCEPT: ON\n");
	}

	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = htonl(INADDR_ANY);	// Local Host Only
	
	psin->sin_port = htons(arp->port);        /* Convert to network ordering */

	if (bind(arp->TCPListenSock , (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
		sprintf(Msg, "bind(sock) failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);

		return FALSE;
	}

	if (listen(arp->TCPListenSock, 1) < 0)
	{
		sprintf(Msg, "listen(sock) failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);
		return FALSE;
	}

	if ((status = WSAAsyncSelect(arp->TCPListenSock, PORT->hResWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{
		sprintf(Msg, "WSAAsyncSelect failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);
		return FALSE;
	}

	return TRUE;
}

void CloseSockets(struct PORTINFO * PORT)
{
	int i;
	int index = 0;
	struct arp_table_entry * arp;

	if (PORT->needip)
		closesocket(PORT->sock);

	for (i=0;i<PORT->NumberofUDPPorts;i++)
	{
		closesocket(PORT->udpsock[i]);
	}
	
	// Close any open or listening TCP sockets

	while (index < PORT->arp_table_len)
	{
		arp = &PORT->arp_table[index++];

		if (arp->TCPMode == TCPMaster)
		{
			if (arp->TCPState)
			{
				closesocket(arp->TCPSock);
				arp->TCPSock = 0;
			}
			continue;
		}

		if (arp->TCPMode == TCPSlave)
		{
			if (arp->TCPState)
			{
				closesocket(arp->TCPSock);
				arp->TCPSock = 0;
			}

			closesocket(arp->TCPListenSock);
			continue;
		}

	}

	return ;
}	


static LRESULT CALLBACK AXResWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT    hOldFont ;
	char line[100];
	char outcall[10];
	int index,displayline;
	struct PORTINFO * PORT;
	MINMAXINFO * mmi;	
	int nScrollCode,nPos;
	int i, Port;

	// Find our PORT Entry

	for (Port = 1; Port < 33; Port++)
	{
		PORT = Portlist[Port];
		if (PORT == NULL)
			continue;
		
		if (PORT->hResWnd == hWnd)
			break;
	}

	if (PORT == NULL)
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	i=1;

	switch (message) { 

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection is pending. */

		Socket_Connect(wParam, WSAGETSELECTERROR(lParam));
		return 0;

	case WM_GETMINMAXINFO:

		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 550;
		mmi->ptMaxSize.y = PORT->MaxResWindowlength;
		mmi->ptMaxTrackSize.x = 550;
		mmi->ptMaxTrackSize.y = PORT->MaxResWindowlength;

		break;


	case WM_MDIACTIVATE:
	{			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)PORT->hResMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM)hBaseMenu, (LPARAM)hWndMenu);
		}
		else
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);

		DrawMenuBar(FrameWnd);
		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);
	}

	case WM_CHAR:

		if (PORT->MHEnabled == FALSE && PORT->MHAvailable)
		{
			PORT->MHEnabled=TRUE;
			CreateMHWindow(PORT);
			ShowWindow(PORT->hMHWnd, SW_RESTORE);		// In case Start Minimized set
		}
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!


		if (wmId == BPQREREAD)
		{
			CloseSockets(PORT);

			ProcessConfig();
			FreeConfig();

			ReadConfigFile(Port);

			_beginthread(OpenSockets, 0, PORT );
			PostMessage(PORT->hResWnd, WM_TIMER,0,0);
			InvalidateRect(hWnd,NULL,TRUE);


			return 0;
		}

		if (wmId == BPQADDARP)
		{
			if (PORT->ConfigWnd == 0)
			{		
				PORT->ConfigWnd=CreateDialog(hInstance, ConfigClassName, 0, NULL);
    
				if (!PORT->ConfigWnd)
				{
					i=GetLastError();
					return (FALSE);
				}
				ShowWindow(PORT->ConfigWnd, SW_SHOW);  
				UpdateWindow(PORT->ConfigWnd); 
  			}

			SetForegroundWindow(PORT->ConfigWnd);

			return(0);
		}
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
			case SC_RESTORE:

				PORT->ResMinimized = FALSE;
				SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);

				break;

			case  SC_MINIMIZE: 

				PORT->ResMinimized = TRUE;

				break;
		}

		return DefMDIChildProc(hWnd, message, wParam, lParam);


	case WM_VSCROLL:
		
		nScrollCode = (int) LOWORD(wParam); // scroll bar value 
		nPos = (short int) HIWORD(wParam);  // scroll box position 

		//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

		if (nScrollCode == SB_LINEUP || nScrollCode == SB_PAGEUP)
		{
			PORT->baseline--;
			if (PORT->baseline <0)
				PORT->baseline=0;
		}

		if (nScrollCode == SB_LINEDOWN || nScrollCode == SB_PAGEDOWN)
		{
			PORT->baseline++;
			if (PORT->baseline > PORT->arp_table_len)
				PORT->baseline = PORT->arp_table_len;
		}

		if (nScrollCode == SB_THUMBTRACK)
		{
			PORT->baseline=nPos;
		}

		SetScrollPos(hWnd,SB_VERT,PORT->baseline,TRUE);

		InvalidateRect(hWnd,NULL,TRUE);
		break;


	case WM_PAINT:

		hdc = BeginPaint (hWnd, &ps);
		
		hOldFont = SelectObject( hdc, hFont) ;
			
		index = PORT->baseline;
		displayline=0;

		while (index < PORT->arp_table_len)
		{
			if (PORT->arp_table[index].ResolveFlag && PORT->arp_table[index].error != 0)
			{
					// resolver error - Display Error Code
				wsprintf(PORT->hostaddr,"Error %d",PORT->arp_table[index].error);
			}
			else
			{
				if (PORT->arp_table[index].IPv6)	
					Format_Addr((unsigned char *)&PORT->arp_table[index].destaddr6.sin6_addr, PORT->hostaddr, PORT->arp_table[index].IPv6);
				else
					Format_Addr((unsigned char *)&PORT->arp_table[index].destaddr.sin_addr, PORT->hostaddr, PORT->arp_table[index].IPv6);
			}
				
			CONVFROMAX25(PORT->arp_table[index].callsign,outcall);
								
			i=wsprintf(line,"%.10s = %.64s %d = %-.30s %c %c",
				outcall,
				PORT->arp_table[index].hostname,
				PORT->arp_table[index].port,
				PORT->hostaddr,
				PORT->arp_table[index].BCFlag ? 'B' : ' ',
				PORT->arp_table[index].TCPState ? 'C' : ' ');

			TextOut(hdc,0,(displayline++)*14+2,line,i);

			index++;
		}

		SelectObject( hdc, hOldFont ) ;
		EndPaint (hWnd, &ps);
	
		break;        

	case WM_DESTROY:

//		PostQuitMessage(0);
			
		break;


	case WM_TIMER:
			
		for (PORT->ResolveIndex=0; PORT->ResolveIndex < PORT->arp_table_len; PORT->ResolveIndex++)
		{	
			struct arp_table_entry * arp = &PORT->arp_table[PORT->ResolveIndex];

			if (arp->ResolveFlag)
			{
				struct addrinfo hints, *res;

				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
				hints.ai_socktype = SOCK_DGRAM;
				getaddrinfo(arp->hostname, NULL, &hints, &res);

				if (res)
				{
					arp->error = 0;
					if (res->ai_family == AF_INET)
					{
						memcpy(&arp->destaddr.sin_addr.s_addr, &res->ai_addr->sa_data[2], 4);
						arp->IPv6 = FALSE;
						arp->destaddr.sin_family = AF_INET;
					}
					else
					{
						PSOCKADDR_IN6 sa6 = (PSOCKADDR_IN6)res->ai_addr;

						memcpy(&arp->destaddr6.sin6_addr, &sa6->sin6_addr, 16);
						arp->IPv6 = TRUE;
						arp->destaddr.sin_family = AF_INET6;
					}
					arp->destaddr.sin_port = htons(arp->port);
					freeaddrinfo(res);
				}
				else
					PORT->arp_table[PORT->ResolveIndex].error = GetLastError();
				
				InvalidateRect(hWnd,NULL,FALSE);
			}
		}

		default:
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	}
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}

int FAR PASCAL ConfigWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	int cmd,id,i;
	HWND hwndChild;
	BOOL OK1,OK2,OK3;

	char call[10], host[65];
	int Interval;
	int calllen;
	int	port;
	char axcall[7];
	BOOL UDPFlag, BCFlag;
	struct PORTINFO * PORT;

	for (i=1; i<33; i++)
	{
		PORT = Portlist[i];
		if (PORT == NULL)
			continue;
		
		if (PORT->ConfigWnd == hWnd)
			break;
	}

	switch (message)
	{
	case WM_CTLCOLORDLG:
	
		return (LONG)bgBrush;

	case WM_COMMAND:	

		id = LOWORD(wParam);
        hwndChild = (HWND)(UINT)lParam;
        cmd = HIWORD(wParam);

		switch (id)
		{
		case ID_SAVE:

			OK1=GetDlgItemText(PORT->ConfigWnd,1001,(LPSTR)call,10);
			OK2=GetDlgItemText(PORT->ConfigWnd,1002,(LPSTR)host,64);
			OK3=1;

			for (i=0;i<7;i++)
				call[i] = toupper(call[i]);
			
			UDPFlag=IsDlgButtonChecked(PORT->ConfigWnd,1004);		
			BCFlag=IsDlgButtonChecked(PORT->ConfigWnd,1005);		

			if (UDPFlag)
				port=GetDlgItemInt(PORT->ConfigWnd,1003,&OK3,FALSE);
			else
				port=0;

			Interval=0;

			if (OK1 && OK2 && OK3==1)
			{
				if (convtoax25(call,axcall,&calllen))
				{
					add_arp_entry(PORT, axcall,0,calllen,port,host,Interval, BCFlag, FALSE, 0, port, FALSE);
					PostMessage(PORT->hResWnd, WM_TIMER,0,0);
					return(DestroyWindow(hWnd));
				}
			}

			// Validation failed

			if (!OK1) SetDlgItemText(PORT->ConfigWnd,1001,"????");
			if (!OK2) SetDlgItemText(PORT->ConfigWnd,1002,"????");
			if (!OK3) SetDlgItemText(PORT->ConfigWnd,1003,"????");

			break;

			case ID_CANCEL:

				return(DestroyWindow(hWnd));
		}
		break;

//	case WM_CLOSE:
	
//		return(DestroyWindow(hWnd));

	case WM_DESTROY:

		PORT->ConfigWnd=0;
	
		return(0);

	}		
	
	return (DefWindowProc(hWnd, message, wParam, lParam));

}

static void CreateResolverWindow(struct PORTINFO * PORT)
{
    int WindowParam;
	WNDCLASS  wc;
	char WindowTitle[100];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];

	HWND hResWnd;
	char Key[80];
	RECT Rect = {0, 0, 300, 300};
	int Top, Left, Width, Height;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", PORT->Port);
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"ResSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &PORT->ResMinimized);

		if (Rect.top < - 500 || Rect.left < - 500)
		{
			Rect.left = 0;
			Rect.top = 0;
			Rect.right = 600;
			Rect.bottom = 400;
		}

		RegCloseKey(hKey);
	}

	Top = Rect.top;
	Left = Rect.left;
	Width = Rect.right - Left;
	Height = Rect.bottom - Top;

	// Register the window classes

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)AXResWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName =  NULL ;
	wc.lpszClassName = "AXAppName";

	RegisterClass(&wc);
	
	wc.style = CS_HREDRAW | CS_VREDRAW;                                      
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = ConfigWndProc;       
	wc.lpszClassName = ConfigClassName;
	RegisterClass(&wc);

	WindowParam = WS_OVERLAPPEDWINDOW | WS_VSCROLL;
	
	sprintf(WindowTitle,"AXIP Port %d Resolver", PORT->Port);

	PORT->hResWnd = hResWnd = CreateMDIWindow("AXAppName", WindowTitle, WindowParam,
		  Left - (OffsetW /2), Top - OffsetH + 4, Width, Height, ClientWnd, hInstance, 1234);


	PORT->hResMenu = CreatePopupMenu();
	AppendMenu(PORT->hResMenu, MF_STRING, BPQREREAD, "ReRead Config");
	AppendMenu(PORT->hResMenu, MF_STRING, BPQADDARP, "Add Entry");

	SetScrollRange(hResWnd,SB_VERT, 0, PORT->arp_table_len, TRUE);

	if (PORT->ResMinimized)
		ShowWindow(hResWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hResWnd, SW_RESTORE);

}

static void ResolveNames(struct PORTINFO * PORT)
{
	struct tagMSG Msg;

	SetTimer(PORT->hResWnd,1,15*60*1000,0);	

	PostMessage(PORT->hResWnd, WM_TIMER,0,0);

	while (GetMessage(&Msg, PORT->hResWnd, 0, 0)) 
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}		

}

extern HWND hWndPopup;

LRESULT CALLBACK MHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT    hOldFont ;
	char line[100];
	char outcall[10];
	HGLOBAL	hMem;
	struct PORTINFO * PORT;
	int index;
	MINMAXINFO * mmi;	

	int i;

	for (i=1; i<33; i++)
	{
		PORT = Portlist[i];
		if (PORT == NULL)
			continue;
		
		if (PORT->hMHWnd == hWnd)
			break;
	}

	if (PORT == NULL)
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	switch (message)
	{ 
	case WM_GETMINMAXINFO:

 		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 500;
		mmi->ptMaxSize.y = PORT->MaxMHWindowlength;
		mmi->ptMaxTrackSize.x = 500;
		mmi->ptMaxTrackSize.y = PORT->MaxMHWindowlength;
		break;

	case WM_MDIACTIVATE:
	{			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)PORT->hMHMenu, "Edit");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM)hBaseMenu, (LPARAM)hWndMenu);
		}
		else
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);

		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	}

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

			case BPQCLEAR:
				memset(PORT->MHTable, 0, sizeof(PORT->MHTable));
				InvalidateRect(hWnd,NULL,TRUE);
				return 0;

			case BPQCOPY:

				hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,2000);
		
				if (hMem != 0)
				{
					if (OpenClipboard(hWnd))
					{
						CopyScreentoBuffer(GlobalLock(hMem), PORT);
						GlobalUnlock(hMem);
						EmptyClipboard();
						SetClipboardData(CF_TEXT,hMem);
						CloseClipboard();
					}
					else
					{
						GlobalFree(hMem);
					}
				}
				return 0;

			default:
		
			return DefMDIChildProc(hWnd, message, wParam, lParam);
		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
			case SC_RESTORE:

				PORT->MHMinimized = FALSE;
				SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
				break;

			case  SC_MINIMIZE: 

				PORT->MHMinimized = TRUE;
				break;
		}

		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_PAINT:

		hdc = BeginPaint (hWnd, &ps);
		hOldFont = SelectObject( hdc, hFont) ;
			
		index = 0;
		CurrentMHEntries = 0;

		while (index < MaxMHEntries)
		{	
			if (PORT->MHTable[index].proto != 0)
			{
				char Addr[80];
				
				Format_Addr((unsigned char *)&PORT->MHTable[index].ipaddr6, Addr, PORT->MHTable[index].IPv6);

				CONVFROMAX25(PORT->MHTable[index].callsign,outcall);

				i=wsprintf(line,"%-10s%-15s %c %-6d %-25s%c",outcall,
						Addr,
						PORT->MHTable[index].proto,
						PORT->MHTable[index].port,
						asctime(gmtime( &PORT->MHTable[index].LastHeard )),
						(PORT->MHTable[index].Keepalive == 0) ? ' ' : 'K');

				line[i-2]= ' ';			// Clear CR returned by asctime

				TextOut(hdc,0,(index)*14+2,line,i);
				CurrentMHEntries ++;
			}
			index++;
		}

		if (PORT->MaxMHWindowlength < CurrentMHEntries * 14 + 40)
			PORT->MaxMHWindowlength = CurrentMHEntries * 14 + 40;

		SelectObject( hdc, hOldFont ) ;
		EndPaint (hWnd, &ps);
	
		break;        

		case WM_DESTROY:
					
			PORT->MHEnabled=FALSE;
			
			break;

		default:
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	}
			
	return DefMDIChildProc(hWnd, message, wParam, lParam);

}
BOOL CopyScreentoBuffer(char * buff, struct PORTINFO * PORT)
{
	int index;
	char outcall[10];

	index = 0;

	while (index < MaxMHEntries)	
	{	
		if (PORT->MHTable[index].proto != 0)
		{
			CONVFROMAX25(PORT->MHTable[index].callsign,outcall);

			buff+=wsprintf(buff,"%-10s%-15s %c %-6d %-26s",outcall,
					inet_ntoa(PORT->MHTable[index].ipaddr),
					PORT->MHTable[index].proto,
					PORT->MHTable[index].port,
					asctime(gmtime( &PORT->MHTable[index].LastHeard )));
		}
		*(buff-2)=13;
		*(buff-1)=10;
		index++;

	}
	*(buff)=0;

	return 0;
}

void CreateMHWindow(struct PORTINFO * PORT)
{
    WNDCLASS  wc;
	char WindowTitle[100];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];
	HWND hMHWnd;
	char Key[80];
	RECT Rect = {0, 0, 300, 300};
	int Top, Left, Width, Height;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", PORT->Port);
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"MHSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &PORT->MHMinimized);

		if (Rect.top < - 500 || Rect.left < - 500)
		{
			Rect.left = 0;
			Rect.top = 0;
			Rect.right = 600;
			Rect.bottom = 400;
		}

		RegCloseKey(hKey);
	}

	Top = Rect.top;
	Left = Rect.left;
	Width = Rect.right - Left;
	Height = Rect.bottom - Top;

	PORT->MaxMHWindowlength = Height;

	wc.style         = CS_HREDRAW | CS_VREDRAW ;//| CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)MHWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName =  NULL ;
	wc.lpszClassName = "MHAppName";

	RegisterClass(&wc);

	sprintf(WindowTitle,"AXIP Port %d MHEARD", PORT->Port);
  
	PORT->hMHWnd = hMHWnd = CreateMDIWindow("MHAppName", WindowTitle, WS_OVERLAPPEDWINDOW,
		  Left - (OffsetW /2), Top - OffsetH, Width, Height, ClientWnd, hInstance, 1234);

	PORT->hMHMenu = CreatePopupMenu();
	AppendMenu(PORT->hMHMenu, MF_STRING, BPQCOPY, "Copy");
	AppendMenu(PORT->hMHMenu, MF_STRING, BPQCLEAR, "Clear");

	if (PORT->MHMinimized)
		ShowWindow(hMHWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hMHWnd, SW_RESTORE);
}

unsigned short int compute_crc(unsigned char *buf,int len)
{
	int fcs;

	_asm{

	mov	esi,buf
	mov	ecx,len
	mov	edx,-1		; initial value

crcloop:

	lodsb

	XOR	DL,AL		; OLD FCS .XOR. CHAR
	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
	ADD	EBX,EBX
	MOV	DL,DH		; SHIFT DOWN 8 BITS
	XOR	DH,DH		; AND CLEAR TOP BITS
	XOR	DX,CRCTAB[EBX]	; XOR WITH TABLE ENTRY
	
	loop	crcloop

	mov	fcs,EDX

	}	

	return (fcs);

  }

static BOOL ReadConfigFile(int Port)
{

/* Linux Format

broadcast QST-0 NODES-0
#
# ax.25 route definition, define as many as you need.
# format is route (call/wildcard) (ip host at destination)
# ssid of 0 routes all ssid's
#
# route <destcall> <destaddr> [flags]
#
# Valid flags are:
#         b  - allow broadcasts to be transmitted via this route
#         d  - this route is the default route
#
#route vk2sut-0 44.136.8.68 b
#route vk5xxx 44.136.188.221 b
#route vk2abc 44.1.1.1
#
*/

//UDP 9999                               # Port we listen on
//MAP G8BPQ-7 10.2.77.1                  # IP 93 for compatibility
//MAP BPQ7 10.2.77.1 UDP 2222            # UDP port to send to
//MAP BPQ8 10.2.77.2 UDP 3333            # UDP port to send to

	char buf[256],errbuf[256];
	HKEY hKey=0;
	char * Config;
	struct PORTINFO * PORT;

	Config = PortConfig[Port];

	if (Portlist[Port])					// Already defined, so must be re-read
	{
		PORT = Portlist[Port];

		PORT->NumberofBroadcastAddreses = 0;
		PORT->needip = FALSE;
		PORT->NeedTCP = FALSE;
		PORT->MHAvailable = FALSE;
		PORT->MHEnabled = FALSE;
		PORT->NumberofUDPPorts = 0;
		PORT->NeedResolver = FALSE;
		PORT->arp_table_len = 0;
		PORT->AutoAddARP = FALSE;
	}
	else
	{
		Portlist[Port] = PORT = zalloc(sizeof (struct PORTINFO));
	}

	PORT->Checkifcanreply = TRUE;

	if (Config)
	{
		char * ptr1 = Config, * ptr2;

		// Using config from bpq32.cfg

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf, PORT))
			{
				WritetoConsole("BPQAXIP - Bad config record");
				WritetoConsole(errbuf);
				WritetoConsole("\n");
			}
		}

		if (PORT->NumberofUDPPorts > MAXUDPPORTS)
		{
			n=wsprintf(buf,"BPQAXIP - Too many UDP= lines - max is %d\n", MAXUDPPORTS);
			WritetoConsole(buf);
		}
		return TRUE;
	}
			
	WritetoConsole("No Configuration info in bpq32.cfg");

	return FALSE;
}

static ProcessLine(char * buf, struct PORTINFO * PORT)
{
	char * ptr;
	char * p_call;
	char * p_ipad;
	char * p_UDP;
	char * p_udpport;
	char * p_Interval;

	int calllen;
	int	port, SourcePort;
	int bcflag;
	char axcall[7];
	int Interval;
	int Dynamic=FALSE;
	int TCPMode;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if(_stricmp(ptr,"UDP") == 0)
	{
		if (PORT->NumberofUDPPorts > MAXUDPPORTS) PORT->NumberofUDPPorts--;

		p_udpport = strtok(NULL, " ,\t\n\r");
			
		if (p_udpport == NULL) return (FALSE);

		PORT->udpport[PORT->NumberofUDPPorts] = atoi(p_udpport);

		if (PORT->udpport[PORT->NumberofUDPPorts] == 0) return (FALSE);

		ptr = strtok(NULL, " \t\n\r");

		if (ptr)
			if (_stricmp(ptr, "ipv6") == 0)
				PORT->IPv6[PORT->NumberofUDPPorts] = TRUE;

		PORT->NumberofUDPPorts++;

		return (TRUE);
	}

	if(_stricmp(ptr,"MHEARD") == 0)
	{
		PORT->MHEnabled = TRUE;
		PORT->MHAvailable = TRUE;

		return (TRUE);
	}

	if(_stricmp(ptr,"DONTCHECKSOURCECALL") == 0)
	{
		PORT->Checkifcanreply = FALSE;
		return (TRUE);
	}

	if(_stricmp(ptr,"AUTOADDMAP") == 0)
	{
		PORT->AutoAddARP = TRUE;
		return (TRUE);
	}
	

	if(_stricmp(ptr,"MAP") == 0)
	{
		p_call = strtok(NULL, " \t\n\r");
		
		if (p_call == NULL) return (FALSE);

		if (_stricmp(p_call, "DUMMY") == 0)
		{
			Consoleprintf("MAP DUMMY is no longer needed - statement ignored");
			return TRUE;
		}

		p_ipad = strtok(NULL, " \t\n\r");
		
		if (p_ipad == NULL) return (FALSE);
	
		p_UDP = strtok(NULL, " \t\n\r");

		Interval=0;
		port=0;				// Raw IP
		bcflag=0;
		TCPMode=0;
		SourcePort = 0;

//
//		Look for (optional) KEEPALIVE, DYNAMIC, UDP or BROADCAST params
//
		while (p_UDP != NULL)
		{
			if (_stricmp(p_UDP,"DYNAMIC") == 0)
			{
				Dynamic=TRUE;
				p_UDP = strtok(NULL, " \t\n\r");
				continue;
			}

			if (_stricmp(p_UDP,"KEEPALIVE") == 0)
			{
				p_Interval = strtok(NULL, " \t\n\r");

				if (p_Interval == NULL) return (FALSE);

				Interval = atoi(p_Interval);
				p_UDP = strtok(NULL, " \t\n\r");
				continue;
			}

			if (_stricmp(p_UDP,"UDP") == 0)
			{
				p_udpport = strtok(NULL, " \t\n\r");
			
				if (p_udpport == NULL) return (FALSE);

				port = atoi(p_udpport);
				p_UDP = strtok(NULL, " \t\n\r");
				continue;
			}

			if (_stricmp(p_UDP,"SOURCEPORT") == 0)
			{
				p_udpport = strtok(NULL, " \t\n\r");
			
				if (p_udpport == NULL) return (FALSE);

				SourcePort = atoi(p_udpport);
				p_UDP = strtok(NULL, " \t\n\r");
				continue;
			}

			if (_stricmp(p_UDP,"TCP-Master") == 0)
			{
				p_udpport = strtok(NULL, " \t\n\r");
			
				if (p_udpport == NULL) return (FALSE);

				port = atoi(p_udpport);
				p_UDP = strtok(NULL, " \t\n\r");

				TCPMode=TCPMaster;

				continue;
			}

			if (_stricmp(p_UDP,"TCP-Slave") == 0)
			{
				p_udpport = strtok(NULL, " \t\n\r");
			
				if (p_udpport == NULL) return (FALSE);

				port = atoi(p_udpport);
				p_UDP = strtok(NULL, " \t\n\r");

				TCPMode = TCPSlave;
				continue;

			}


			if (_stricmp(p_UDP,"B") == 0)
			{
				bcflag =TRUE;
				p_UDP = strtok(NULL, " \t\n\r");
				continue;
			}

			if ((*p_UDP == ';') || (*p_UDP == '#'))	break;			// Comment on end

			return FALSE;

		}

		if (convtoax25(p_call,axcall,&calllen))
		{
			if (SourcePort == 0)
				SourcePort = port;

			add_arp_entry(PORT, axcall, 0, calllen, port, p_ipad, Interval, bcflag, FALSE, TCPMode, SourcePort, FALSE);
			return (TRUE);
		}
	}		// End of Process MAP

	if(_stricmp(ptr,"BROADCAST") == 0)
	{
		p_call = strtok(NULL, " \t\n\r");
		
		if (p_call == NULL) return (FALSE);

		if (convtoax25(p_call,axcall,&calllen))
		{
			add_bc_entry(PORT, axcall,calllen);
			return (TRUE);
		}


		return (FALSE);		// Failed convtoax25
	}

	//
	//	Bad line
	//
	return (FALSE);
}
	
int CONVFROMAX25(char * incall, char * outcall)
{
	int in,out=0;
	unsigned char chr;
//
//	CONVERT AX25 FORMAT CALL IN incall TO NORMAL FORMAT IN out
//	   RETURNS LENGTH 
//
	memset(outcall,0x20,9);
	outcall[9]=0;

	for (in=0;in<6;in++)
	{
		chr=incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr=incall[6];				// ssid
	chr >>= 1;
	chr	&= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}
	return (out);
}


BOOL convtoax25(unsigned char * callsign, unsigned char * ax25call,int * calllen)
{
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID	

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				*calllen = 7;				// include ssid in test
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == ' ')
		{
			//
			//	End of call - no ssid
			//
			*calllen = 6;				// wildcard ssid
			return (TRUE);
		}
		
		ax25call[i] = callsign[i] << 1;
	}
	
	//
	//	Too many chars
	//

	return (FALSE);
}

BOOL add_arp_entry(struct PORTINFO * PORT, UCHAR * call, UCHAR * ip, int len, int port,
				   UCHAR * name, int keepalive, BOOL BCFlag, BOOL AutoAdded, int TCPFlag, int SourcePort, BOOL IPv6)
{
	struct arp_table_entry * arp;

	if (PORT->arp_table_len == MAX_ENTRIES)
		//
		//	Table full
		//
		return (FALSE); 

	arp = &PORT->arp_table[PORT->arp_table_len];

	arp->SourceSocket = 0;

	arp->PORT = PORT;

	if (port == 0) PORT->needip = 1;			// Enable Raw IP Mode

	arp->ResolveFlag=TRUE;
	PORT->NeedResolver=TRUE;

	memcpy (&arp->callsign,call,7);
	strncpy((char *)&arp->hostname,name,64);
	arp->len = len;
	arp->port = port;
	keepalive+=9;
	keepalive/=10;

	arp->keepalive = keepalive;
	arp->keepaliveinit = keepalive;
	arp->BCFlag = BCFlag;
	arp->AutoAdded = AutoAdded;
	arp->TCPMode = TCPFlag;
	PORT->arp_table_len++;

	if (PORT->MaxResWindowlength < (PORT->arp_table_len * 14) + 70)
		PORT->MaxResWindowlength = (PORT->arp_table_len * 14) + 70;

	PORT->NeedResolver |= TCPFlag;					// Need Resolver window to handle tcp socket messages
	PORT->NeedTCP |= TCPFlag;

	if (ip)
	{
		// Only have an IP address if dynamically added - so update destaddr

		if (IPv6)
		{
			memcpy(&arp->destaddr6.sin6_addr, ip, 16);
			arp->IPv6 = TRUE;
			arp->destaddr.sin_family = AF_INET6;
		}
		else
		{
			memcpy(&arp->destaddr.sin_addr.s_addr, ip, 4);
			arp->IPv6 = FALSE;
			arp->destaddr.sin_family = AF_INET;
		}
		arp->destaddr.sin_port = htons(arp->port);
		InvalidateRect(PORT->hResWnd, NULL, FALSE);
	}

	return (TRUE);
}

BOOL add_bc_entry(struct PORTINFO * PORT, unsigned char * call, int len)
{
	if (PORT->NumberofBroadcastAddreses == MAX_BROADCASTS)
		//
		//	Table full
		//
		return (FALSE);

	memcpy (PORT->BroadcastAddresses[PORT->NumberofBroadcastAddreses].callsign,call,7);
	PORT->BroadcastAddresses[PORT->NumberofBroadcastAddreses].len = len;
	PORT->NumberofBroadcastAddreses++;

	return (TRUE);
}


int CheckKeepalives(struct PORTINFO * PORT)
{
	int index=0,txsock;
	struct arp_table_entry * arp;

	while (index < PORT->arp_table_len)
	{
		if (PORT->arp_table[index].keepalive != 0)
		{
			arp = &PORT->arp_table[index];
			arp->keepalive--;
			
			if (arp->keepalive == 0)
			{
			//
			//	Send Keepalive Packet
			//
				arp->keepalive=arp->keepaliveinit;

				if (arp->error == 0)
				{
					if (arp->port == 0) txsock = PORT->sock; else txsock = PORT->udpsock[0];

					sendto(txsock,"Keepalive",9,0,(LPSOCKADDR)&arp->destaddr,sizeof(arp->destaddr));			
				}
			}
		}
	
	index++;

	}

	// Decrement MH Keepalive flags

	for (index = 0; index < MaxMHEntries; index++)
	{
		if (PORT->MHTable[index].Keepalive != 0) 
			PORT->MHTable[index].Keepalive--;			
	}

	return (0);
}

BOOL CheckSourceisResolvable(struct PORTINFO * PORT, char * call, int Port, VOID * rxaddr)
{
	// Makes sure we can reply to call before accepting message

	int index = 0;
	struct arp_table_entry * arp;

	while (index < PORT->arp_table_len)
	{
		arp = &PORT->arp_table[index];

		if (memcmp(arp->callsign, call, arp->len) == 0)
		{
			// Call is present - if AutoAdded, refresh IP address and Port

			if (arp->AutoAdded)
			{
				if (arp->IPv6)
				{
					SOCKADDR_IN6 * SA6 = rxaddr;
					memcpy(&arp->destaddr6.sin6_addr, &SA6->sin6_addr, 16);
				}
				else
				{
					SOCKADDR_IN * SA = rxaddr;
					memcpy(&arp->destaddr.sin_addr.s_addr, &SA->sin_addr, 4);
				}
				arp->port = Port;
			}
			return 1;		// Ok to process
		}
		index++;
	}

	return (0);				// Not in list
}

int Update_MH_List(struct PORTINFO * PORT, UCHAR * ipad, char * call, char proto, short port, BOOL IPv6)
{
	int index;
	char callsign[7];
	int SaveKeepalive=0;
	struct MHTableEntry * MH;

	memcpy(callsign,call,7);
	callsign[6] &= 0x3e;				// Mask non-ssid bits

	for (index = 0; index < MaxMHEntries; index++)
	{
		MH = &PORT->MHTable[index];
		
		if (MH->callsign[0] == 0) 

			//	empty entry, so call not present. Move all down, and add to front

			goto MoveEntries;

		if (memcmp(MH->callsign,callsign,7) == 0 &&
			memcmp(&MH->ipaddr, ipad, (MH->IPv6) ? 16 : 4) == 0 &&
					MH->proto == proto &&
					MH->port == port)
		{
			// Entry found, move preceeding entries down and put on front

			SaveKeepalive = MH->Keepalive;
			goto MoveEntries;
		}
	}

	// Table full move MaxMHEntries-1 entries down, and add on front

		index=MaxMHEntries-1;

MoveEntries:

	//
	//	Move all preceeding entries down one, and put on front
	//
	
	if (index > 0)
		memmove(&PORT->MHTable[1],&PORT->MHTable[0],index*sizeof(struct MHTableEntry));

	MH = &PORT->MHTable[0];

	memcpy(MH->callsign,callsign,7);
	memcpy(&MH->ipaddr6, ipad, (IPv6) ? 16 : 4);
	MH->proto = proto;

	MH->port = port;
	time(&MH->LastHeard);
	MH->Keepalive = SaveKeepalive;
	MH->IPv6 = IPv6;
	InvalidateRect(PORT->hMHWnd,NULL,FALSE);
	return 0;

}

int Update_MH_KeepAlive(struct PORTINFO * PORT, struct in_addr ipad, char proto, short port)
{
	int index;

	for (index = 0; index < MaxMHEntries; index++)
	{
		if (PORT->MHTable[index].callsign[0] == 0) 

			//	empty entry, so call not present.

			return 0;

		if (memcmp(&PORT->MHTable[index].ipaddr,&ipad,4) == 0 &&
				PORT->MHTable[index].proto == proto &&
				PORT->MHTable[index].port == port)
		{
			PORT->MHTable[index].Keepalive = 30;		// 5 Minutes at 10 sec ticks
			return 0;
		}
	}

	return 0;

}


int DumpFrameInHex(unsigned char * msg, int len)
{
	char errmsg[100];
	int i=0;

	for (i=0;i<len;i+=16)
	{
		wsprintf(errmsg,"%04x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
			i, msg[i], msg[i+1],msg[i+2],msg[i+3],msg[i+4],msg[i+5],msg[i+6],msg[i+7],
			msg[i+8],msg[i+9],msg[i+10],msg[i+11],msg[i+12],msg[i+13],msg[i+14],msg[i+15]);
	
 			OutputDebugString(errmsg);
	}

	return 0;
}

int Socket_Accept(int SocketId)
{
	int addrlen, i;
	struct arp_table_entry * sockptr;
	SOCKET sock;
	int index;
	BOOL bOptVal = TRUE;
	struct PORTINFO * PORT;

	//  Find Socket entry

	Debugprintf("Incoming Connect - Socket %d", SocketId);

	for (i = 0; i < 33; i++)
	{
		PORT = Portlist[i];
		if (PORT == NULL)
			continue;
		
		index = 0;
	while (index < PORT->arp_table_len)
	{
		sockptr = &PORT->arp_table[index];

		if (sockptr->TCPListenSock == SocketId)
		{
			struct sockaddr sin;
			
			addrlen=sizeof(struct sockaddr);

			sock = accept(SocketId, &sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				Debugprintf("accept() failed Error %d", WSAGetLastError());
				return FALSE;
			}

			Debugprintf("Connect accepted - Socket %d", sock);

			if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bOptVal, 4) != SOCKET_ERROR)
			{

				Debugprintf("Set SO_KEEPALIVE: ON");
			}

			WSAAsyncSelect(sock, PORT->hResWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

//			closesocket(sockptr->TCPSock);		// CLose listening socket

			sockptr->TCPSock = sock;
			sockptr->TCPState = TCPConnected;

			return 0;
		}

		index++;
	}
}
	return 0;
	
}

int Socket_Connect(int SocketId, int Error)
{
	struct arp_table_entry * sockptr;
	struct PORTINFO * PORT;

	int index, i;

	//   Find Socket entry

	//	Find Connection Record

	WSAGETSELECTERROR(index);

	for (i = 0; i < 33; i++)
	{
		PORT = Portlist[i];
		if (PORT == NULL)
			continue;

	index = 0;

	while (index < PORT->arp_table_len)
	{
		sockptr = &PORT->arp_table[index++];

		if (sockptr->TCPSock == SocketId)
		{
			Debugprintf("TCP Connect Complete %d result %d", sockptr->TCPSock, Error);

			if (Error == 0)
			{
				sockptr->TCPState = TCPConnected;
			
				WSAAsyncSelect(SocketId, PORT->hResWnd, WSA_DATA,
						FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			}
			else
				sockptr->TCPState = 0;

		}
	}
		}
	return 0;
}
int Socket_Data(int sock, int error, int eventcode)
{
	struct arp_table_entry  * sockptr;
	int index, i;
	struct PORTINFO * PORT;

	//	Find Connection Record

	for (i = 0; i < 33; i++)
	{
		PORT = Portlist[i];
		if (PORT == NULL)
			continue;

	index = 0;

	while (index < PORT->arp_table_len)
	{
		sockptr = &PORT->arp_table[index];

		if (sockptr->TCPSock == sock)
		{
			switch (eventcode)
			{
				case FD_READ:

					return DataSocket_Read(sockptr,sock);

				case FD_WRITE:

					sockptr->TCPState = TCPConnected;
					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					Debugprintf("TCP Close received for socket %d", sock);

					sockptr->TCPState = 0;
					closesocket(sock);
					return 0;
				}

			return 0;
		}
		index++;
	}
	}
	return 0;
}

int DataSocket_Read(struct arp_table_entry * sockptr, SOCKET sock)
{
	int InputLen;

	// May have several messages per packet, or message split over packets

	if (sockptr->InputLen > 3000)	// Shouldnt have lines longer  than this in text mode
	{
		sockptr->InputLen=0;
	}
				
	InputLen=recv(sock, &sockptr->TCPBuffer[sockptr->InputLen], 1000, 0);

	if (InputLen < 0)
	{
		sockptr->TCPState = 0;
		closesocket(sock);
		return 0;
	}

	if (InputLen == 0)
		return 0;					// Does this mean closed?

	sockptr->InputLen += InputLen;

	return 0;
}

int GetMessageFromBuffer(struct PORTINFO * PORT, char * Buffer)
{
	struct arp_table_entry * sockptr;
	int index=0;
	int MsgLen;
	char * ptr, * ptr2;

	//   Look for data in tcp buffers

	while (index < PORT->arp_table_len)
	{
		sockptr = &PORT->arp_table[index++];

		if (sockptr->TCPMode)
		{
			if (sockptr->InputLen == 0)
			{
				sockptr->TCPOK++;

				if (sockptr->TCPOK > 36000)		// 60 MINS
				{
					if (sockptr->TCPSock)
					{
						Debugprintf("No Data for 60 Mins on Data Sock %d State %d",
							sockptr->TCPListenSock, sockptr->TCPSock, sockptr->TCPState);

						sockptr->TCPState = 0;
						closesocket(sockptr->TCPSock);
						sockptr->TCPSock = 0;
					}

					closesocket(sockptr->TCPListenSock);
					OpenListeningSocket(PORT, sockptr);

					sockptr->TCPOK = 0;
				}
				continue;
			}
	
			ptr = memchr(sockptr->TCPBuffer, FEND, sockptr->InputLen);

			if (ptr)	//  FEND in buffer
			{
				ptr2 = &sockptr->TCPBuffer[sockptr->InputLen];
				ptr++;

				if (ptr == ptr2)
				{
					// Usual Case - single meg in buffer

					MsgLen = sockptr->InputLen;
					sockptr->InputLen = 0;

					if (MsgLen > 1)
					{
						memcpy(Buffer, sockptr->TCPBuffer, MsgLen);

						if (PORT->MHEnabled)
							Update_MH_List(PORT, &sockptr->destaddr.sin_addr.s_net, &Buffer[7],'T', sockptr->port, 0);

						sockptr->TCPOK = 0;

						return MsgLen;
					}
				}
				else
				{
					// buffer contains more that 1 message

					MsgLen = sockptr->InputLen - (ptr2-ptr);
					memcpy(Buffer, sockptr->TCPBuffer, MsgLen);

					memmove(sockptr->TCPBuffer, ptr, sockptr->InputLen-MsgLen);

					sockptr->InputLen -= MsgLen;

					if (MsgLen > 1)
					{
						if (PORT->MHEnabled)
							Update_MH_List(PORT, &sockptr->destaddr.sin_addr.s_net, &Buffer[7],'T', sockptr->port, 0);

						sockptr->TCPOK = 0;

						return MsgLen;
					}
				}
			}
		}
	}
	return 0;

}

int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	outbuff[0]=FEND;
	txptr=1;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];
		
		switch (c)
		{
		case FEND:
			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFEND;
			break;

		case FESC:

			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFESC;
			break;

		default:

			outbuff[txptr++]=c;
		}
	}

	outbuff[txptr++]=FEND;

	return txptr;

}
int	KissDecode(UCHAR * inbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];

		if (c == FESC)
		{
			c=inbuff[++i];
			{
				if (c == TFESC)
					c=FESC;
				else
				if (c == TFEND)
					c=FEND;
			}
		}

		inbuff[txptr++]=c;
	}

	return txptr;
}

VOID TCPConnectThread(struct arp_table_entry * arp)
{
	char Msg[255];
	int err, i, status;
	u_long param=1;
	BOOL bcopt=TRUE;
	SOCKADDR_IN sinx; 
//	struct PORTINFO * PORT;

	Sleep(5000);									// Delay startup a bit

	while(arp->TCPMode == TCPMaster)
	{		
		if (arp->TCPState == 0)
		{
			Debugprintf("TCP Connect Closing socket %d IP %s", arp->TCPSock, arp->hostname);
	
			closesocket(arp->TCPSock);

			arp->TCPSock=socket(AF_INET,SOCK_STREAM,0);

			if (arp->TCPSock == INVALID_SOCKET)
			{
				i=wsprintf(Msg, "Socket Failed for AX/TCP socket - error code = %d\n", WSAGetLastError());
				WritetoConsole(Msg);
  	 			goto wait; 
			}

			ioctlsocket (arp->TCPSock, FIONBIO, &param);
 
			setsockopt (arp->TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);
 
			if (setsockopt(arp->TCPSock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bcopt, 4) != SOCKET_ERROR)
			{
				Debugprintf("Set SO_KEEPALIVE: ON");
			}

			sinx.sin_family = AF_INET;
			sinx.sin_addr.s_addr = INADDR_ANY;
			sinx.sin_port = 0;

			if (bind(arp->TCPSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
			{
				//
				//	Bind Failed
				//
	
				i=wsprintf(Msg, "Bind Failed for AX/TCP socket - error code = %d\n", WSAGetLastError());
				WritetoConsole(Msg);

  				goto wait; 
			}

			if ((status = WSAAsyncSelect(arp->TCPSock, arp->PORT->hResWnd, WSA_CONNECT, FD_CONNECT)) > 0)
			{
				sprintf(Msg, "WSAAsyncSelect failed Error %d", WSAGetLastError());
				MessageBox(arp->PORT->hResWnd, Msg, "BPQAXIP", MB_OK);
				closesocket(arp->TCPSock);
				goto wait;
			}

			arp->TCPState = TCPConnecting;

			if (connect(arp->TCPSock,(LPSOCKADDR) &arp->destaddr, sizeof(arp->destaddr)) == 0)
			{
				//
				//	Connected successful
				//

				arp->TCPState = TCPConnected;
				OutputDebugString("TCP Connected\r\n");
			}
			else
			{
				err=WSAGetLastError();

				if (err == WSAEWOULDBLOCK)
				{
					//
					//	Connect in Progressing
					//

						Debugprintf("TCP Connect in Progress %d IP %s", arp->TCPSock, arp->hostname);
				}
				else
				{
					//
					//	Connect failed
					//
    				i=wsprintf(Msg, "Connect Failed for AX/TCP socket - error code = %d\n", err);
					WritetoConsole(Msg);
					OutputDebugString(Msg);
					closesocket(arp->TCPSock);

					arp->TCPState =0;
				}
			}
		}
wait:
		Sleep (115000);				// 2 Mins 
	}

	Debugprintf("TCP Connect Thread %x Closing", arp->TCPThreadID);

	arp->TCPThreadID = 0;
	
	return;		// Not Used

}

static VOID Format_Addr(unsigned char * Addr, char * Output, BOOL IPV6)
{
	unsigned char * src;
	char zeros[12] = "";
	char * ptr;
	struct
	{
		int base, len;
	} best, cur;
	unsigned int words[8];
	int i;

	if (IPV6 == FALSE)
	{
		wsprintf(Output, "%d.%d.%d.%d", Addr[0], Addr[1], Addr[2], Addr[3]);
		return;
	}

	src = Addr;

	// See if Encapsulated IPV4 addr

	if (src[12] != 0)
	{
		if (memcmp(src, zeros, 12) == 0)	// 12 zeros, followed by non-zero
		{
			wsprintf(Output, "::%d.%d.%d.%d", src[12], src[13], src[14], src[15]);
			return;
		}
	}

	// COnvert 16 bytes to 8 words
	
	for (i = 0; i < 16; i += 2)
	    words[i / 2] = (src[i] << 8) | src[i + 1];

	// Look for longest run of zeros
	
	best.base = -1;
	cur.base = -1;
	
	for (i = 0; i < 8; i++)
	{
		if (words[i] == 0)
		{
	        if (cur.base == -1)
				cur.base = i, cur.len = 1;		// New run, save start
	          else
	            cur.len++;						// Continuation - increment length
		}
		else
		{
			// End of a run of zeros

			if (cur.base != -1)
			{
				// See if this run is longer
				
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				
				cur.base = -1;	// Start again
			}
		}
	}
	
	if (cur.base != -1)
	{
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	
	if (best.base != -1 && best.len < 2)
	    best.base = -1;
	
	ptr = Output;
	  
	for (i = 0; i < 8; i++)
	{
		/* Are we inside the best run of 0x00's? */

		if (best.base != -1 && i >= best.base && i < (best.base + best.len))
		{
			// Just output one : for whole string of zeros
			
			*ptr++ = ':';
			i = best.base + best.len - 1;	// skip rest of zeros
			continue;
		}
	    
		/* Are we following an initial run of 0x00s or any real hex? */
		
		if (i != 0)
			*ptr++ = ':';
		
		ptr += sprintf (ptr, "%x", words[i]);
	        
		//	Was it a trailing run of 0x00's?
	}

	if (best.base != -1 && (best.base + best.len) == 8)
		*ptr++ = ':';
	
	*ptr++ = '\0';	
}
