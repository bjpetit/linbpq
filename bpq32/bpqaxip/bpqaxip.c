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

#define _CRT_SECURE_NO_DEPRECATE

#include <winsock2.h>
#include "windows.h"
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "winver.h"
#include "SHELLAPI.H"

#include "resource.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

#define BUFFLEN	360	

//	BUFFLEN-4 = L2 POINTER (FOR CLEARING TIMEOUT WHEN ACKMODE USED)
//	BUFFLEN-8 = TIMESTAMP
//	BUFFLEN-12 = BUFFER ALLOCATED FLAG (ADDR OF ALLOCATING ROUTINE)
	
#define MAXDATA	BUFFLEN-16

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD



#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

//unsigned long _beginthread( void( *start_address )( void *), unsigned stack_size, char * arglist);

DllImport int ResetExtDriver(int num);


//UCHAR * APIENTRY GetBPQDirectory();

//UCHAR * (FAR WINAPI * GetBPQDirectory)();


void ResolveNames(void *dummy);
void OpenSockets(void *dummy);
void CloseSockets();


int CONVFROMAX25(char * incall, char * outcall);
void CreateMHWindow();
int Update_MH_List(struct in_addr ipad, char * call, char proto, short port);
int Update_MH_KeepAlive(struct in_addr ipad, char proto, short port);
unsigned short int compute_crc(unsigned char *buf,int l);
unsigned int find_arp(unsigned char * call);
BOOL add_arp_entry(unsigned char * call, unsigned long * ip, int len, int port,unsigned char * name, int keepalive, BOOL BCFlag, BOOL AutoAdded, int TCPMode);
BOOL add_bc_entry(unsigned char * call, int len);
BOOL convtoax25(unsigned char * callsign, unsigned char * ax25call, int * calllen);
BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
int CheckKeepalives();
BOOL CopyScreentoBuffer(char * buff);
int DumpFrameInHex(unsigned char * msg, int len);
VOID SendFrame(struct arp_table_entry * arp_table, UCHAR * buff, int txlen);
BOOL CheckSourceisResolvable(char * call, int Port);
int DataSocket_Read(struct arp_table_entry * sockptr, SOCKET sock);
int GetMessageFromBuffer(char * Buffer);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
int Socket_Accept(int SocketId);
int Socket_Connect(int SocketId, int Error);
int Socket_Data(int sock, int error, int eventcode);
VOID TCPConnectThread(struct arp_table_entry * arp);
VOID __cdecl Debugprintf(const char * format, ...);
BOOL OpenListeningSocket(struct arp_table_entry * arp);

BOOL Checkifcanreply=TRUE;

BOOL AutoAddARP=FALSE;

BOOL MinimizetoTray=FALSE;

BOOL StartMinimized=FALSE;

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

//
//	'ARP' table - converts AX.25 address to IP address
//

#pragma pack(1) 

#define MAX_ENTRIES 128

struct arp_table_entry
{
	unsigned char callsign[7];
	unsigned char len;			// bytes to compare (6 or 7)
	union
	{
		struct in_addr in_addr;
		unsigned int ipaddr;
	};
	unsigned short port;
	unsigned char hostname[64];
	unsigned int error;
	BOOL ResolveFlag;			// True if need to resolve name
	unsigned int keepalive;
	unsigned int keepaliveinit;
	BOOL BCFlag;				// Frue if we want broadcasts to got to this call
	BOOL AutoAdded;				// Set if Entry created as a result of AUTOADDMAP
	SOCKET TCPListenSock;			// Listening socket if slave
	SOCKET TCPSock;
	int  TCPMode;				// TCPMaster ot TCPSlave
	UCHAR * TCPBuffer;			// Area for building TCP message from byte stream
	int InputLen;				// Bytes in TCPBuffer
	SOCKADDR_IN sin; 
	SOCKADDR_IN destaddr;
	BOOL TCPState;
	UINT TCPThreadID;			// Thread ID if TCP Master
	UINT TCPOK; 				// Cleared when Message RXed . Incremented by timer

};

#define TCPMaster 1
#define TCPSlave 2

#define TCPListening 1
#define TCPConnecting 2
#define TCPConnected 4


struct broadcast_table_entry
{
	unsigned char callsign[7];
	unsigned char len;			// bytes to compare (6 or 7)
};

#define MAX_BROADCASTS 8

struct broadcast_table_entry BroadcastAddresses[MAX_BROADCASTS];

int NumberofBroadcastAddreses = 0;

unsigned char  hostaddr[64];

HMENU trayMenu;

LOGFONT LFTTYFONT ;

HFONT hFont ;

int arp_table_len=0;
//int index=0;					// pointer for table search
int ResolveIndex=-1;			// pointer to entry being resolved

struct arp_table_entry arp_table[MAX_ENTRIES];

struct arp_table_entry default_arp;

BOOL MHEnabled=FALSE;
BOOL MHAvailable=FALSE;			// Enabled with config file directive

#define MaxMHEntries 40

struct MHTableEntry
{
	unsigned char callsign[7];
	char proto;
	short port;
	struct in_addr ipaddr;
	time_t	LastHeard;		// Time last packet received
	int Keepalive;
};


struct MHTableEntry MHTable[MaxMHEntries];

 
struct tagMSG Msg;

char buf[MAXGETHOSTSTRUCT];

#pragma pack()

#define MAXUDPPORTS 30

static short udpport[MAXUDPPORTS+2];

int NumberofUDPPorts;

BOOL needip = FALSE;
BOOL NeedResolver = FALSE;
BOOL NeedTCP = FALSE;

SOCKET sock;
SOCKET udpsock[MAXUDPPORTS+2];

SOCKADDR_IN sinx; 
SOCKADDR_IN rxaddr;
SOCKADDR_IN destaddr;
int addrlen=sizeof(sinx);

extern short CRCTAB;

unsigned int AXIPInst = 0;
int AttachedProcesses=0;

HWND hResWnd, hMHWnd, ConfigWnd;

BOOL GotMsg;

HANDLE STDOUT=0;
DWORD n;

int baseline=0;

int InitAXIP(void);
BOOL InitWS2(void);

RECT ResRect;
RECT MHRect;

time_t ltime,lasttime;

char ConfigClassName[]="CONFIG";

HANDLE hInstance;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

	hInstance=hInst;

	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		AttachedProcesses++;
		
		if (AXIPInst == 0)				// First entry
		{
			GetAPI();
			AXIPInst = GetCurrentProcessId();
		}
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		if (AXIPInst == GetCurrentProcessId())
		{
		//	KillTimer(NULL,TimerHandle);
		//	TimerHandle=0;
		//	ResetExtDriver(2);
			
		//	AXIPInst=0;
		//	Tell_Sessions();
		//	MessageBox(NULL,"Process holding AXIP closing","BPQ32",MB_OK);
		}
		
		AttachedProcesses--;

		if (AttachedProcesses == 0)
		{
        	if (MinimizetoTray)
			{
				DeleteTrayMenuItem(hMHWnd);
				DeleteTrayMenuItem(hResWnd);
			}

			ShowWindow(hMHWnd, SW_RESTORE);
			GetWindowRect(hMHWnd, &MHRect);

			ShowWindow(hResWnd, SW_RESTORE);
			GetWindowRect(hResWnd, &ResRect);
	
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQAXIP",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d",MHRect.left,MHRect.right,MHRect.top,MHRect.bottom);
				retCode = RegSetValueEx(hKey,"MHSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

				wsprintf(Size,"%d,%d,%d,%d",ResRect.left,ResRect.right,ResRect.top,ResRect.bottom);
				retCode = RegSetValueEx(hKey,"ResSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));
				RegCloseKey(hKey);
			}

//			CloseSockets();

//			DestroyWindow(hResWnd);
//			PostMessage(hResWnd, WM_QUIT,0,0);
//			DestroyWindow(hMHWnd);

	//		FreeLibrary(ExtDriver);
		}
		return 1;
	}
 
	return 1;
	
}

DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	struct iphdr * iphdrptr;
	int len,txlen=0,err,index,digiptr,i;
	unsigned short int crc;
	char rxbuff[500];
	char axcall[7];
	char errmsg[100];

	switch (fn)
	{
	case 1:				// poll

		//
		//	Check Keepalive timers
		//
		time( &ltime );
		if (ltime-lasttime >9 )
		{
			CheckKeepalives();
			lasttime=ltime;
		}

		if (needip)
		{
			len = recvfrom(sock,rxbuff,500,0,(LPSOCKADDR)&rxaddr,&addrlen);

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
						if (MHEnabled)
							Update_MH_KeepAlive(rxaddr.sin_addr,'I',93);
	
						return 0;
					}
					crc = compute_crc(&rxbuff[20], len);

					if (crc == 0xf0b8)		// Good CRC
					{
						len-=2;			// Remove CRC
					
						if (len > MAXDATA)
						{
							wsprintf(errmsg,"BPQAXIP Invalid Msg Len=%d Source=%s",len,inet_ntoa(rxaddr.sin_addr));
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

						if (MHEnabled)
							Update_MH_List(rxaddr.sin_addr,&buff[14],'I',93);

						if (Checkifcanreply)
						{
							char call[7];

							memcpy(call, &buff[14], 7);
							call[6] &= 0x7e;		// Mask End of Address bit

							if (CheckSourceisResolvable(call, 0))

								return 1;

							else
								// Can't reply. If AutoConfig is set, add to table and accept, else reject

								if (AutoAddARP)

									return add_arp_entry(call, (PVOID)&rxaddr.sin_addr, 7, 0, inet_ntoa(rxaddr.sin_addr), 0, TRUE, TRUE, 0);

								else

									return 0;
						}
						else
							return(1);
					}
					//
					//	CRC Error
					//
						
					wsprintf(errmsg,"BPQAXIP Invalid CRC=%d Source=%s",crc,inet_ntoa(rxaddr.sin_addr));
						OutputDebugString(errmsg);

					return (0);
				}

				//
				//	Bad Length
				//
	
				return (0);
	
			}
		}

		for (i=0;i<NumberofUDPPorts;i++)
		{
			len = recvfrom(udpsock[i],rxbuff,500,0,(LPSOCKADDR)&rxaddr,&addrlen);
	
			if (len == -1)
			{		
				err = WSAGetLastError();
			}
			else
			{
				if (memcmp(rxbuff, "Keepalive", 9) == 0 )
				{
					if (MHEnabled)
						Update_MH_KeepAlive(rxaddr.sin_addr, 'U', udpport[i]);
	
					continue;
				}
				
				crc = compute_crc(&rxbuff[0], len);

				if (crc == 0xf0b8)		// Good CRC
				{
					len-=2;				// Remove CRC

					if (len > MAXDATA)
					{
						wsprintf(errmsg,"BPQAXIP Invalid Msg Len=%d Source=%s Port %d",len,inet_ntoa(rxaddr.sin_addr),udpport[i]);
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

					if (MHEnabled)
						Update_MH_List(rxaddr.sin_addr,&buff[14],'U',udpport[i]);	

					if (Checkifcanreply)
					{
						char call[7];

						memcpy(call, &buff[14], 7);
						call[6] &= 0x7e;		// Mask End of Address bit

						if (CheckSourceisResolvable(call, udpport[i]))
							return 1;
						else
							// Can't reply. If AutoConfig is set, add to table and accept, else reject

							if (AutoAddARP)
								return add_arp_entry(call, (PVOID)&rxaddr.sin_addr, 7, udpport[i], inet_ntoa(rxaddr.sin_addr), 0, TRUE, TRUE, 0);
							else
								return 0;
					}
					else
						return(1);
				}

				//	
				//	CRC Error
				//

				wsprintf(errmsg,"BPQAXIP Invalid CRC=%d Source=%s Port %d",crc,inet_ntoa(rxaddr.sin_addr),udpport[i]);
				OutputDebugString(errmsg);

				return (0);
			}
		}

		if (NeedTCP)
		{
			len = GetMessageFromBuffer(rxbuff);

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

		for (i=0; i< NumberofBroadcastAddreses; i++)
		{
			if (memcmp(axcall, BroadcastAddresses[i].callsign, 7) == 0)
			{
				for (index = 0; index < arp_table_len; index++)
				{
					if (arp_table[index].BCFlag) SendFrame(&arp_table[index], &buff[7], txlen);
				}
				return 0;
			}
		}

//		Send to all matching calls in arp table

		index = 0;

		while (index < arp_table_len)
		{
			if (memcmp(arp_table[index].callsign,axcall,arp_table[index].len) == 0)
			{
				SendFrame(&arp_table[index], &buff[7], txlen);
			}
			index++;
		}
		return (0);

	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		CloseSockets();
		ReadConfigFile("BPQAXIP.CFG");
		_beginthread(OpenSockets, 0, NULL );
		PostMessage(hResWnd, WM_TIMER,0,0);

		break;

	case 5:				// Terminate

		CloseSockets();
		PostMessage(hResWnd, WM_QUIT,0,0);
		PostMessage(hMHWnd, WM_DESTROY,0,0);
		DestroyWindow(hMHWnd);

		if (MinimizetoTray)	
			DeleteTrayMenuItem(hResWnd);

//		FreeLibrary(ExtDriver);

		break;
	}
	return (0);
}

VOID SendFrame(struct arp_table_entry * arp_table, UCHAR * buff, int txlen)
{				
	int txsock;

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
			

	if (arp_table->ipaddr != 0)
	{
		destaddr.sin_addr.s_addr = arp_table->ipaddr;
		destaddr.sin_port = htons(arp_table->port);

		if (arp_table->port == 0) txsock = sock; else txsock = udpsock[0];

		sendto(txsock,buff, txlen,0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
			
		// reset Keepalive Timer
					
		arp_table->keepalive=arp_table->keepaliveinit;
	}
}
		

DllExport int APIENTRY ExtInit(struct PORTCONTROL *  PortEntry)
{
	WritetoConsole("AXIP ");

	if (!InitWS2()) return 0;

	if (!InitAXIP()) return 0;

	return ((int) ExtProc);
}

BOOL InitWS2(void)
{
    int           Error;              // catches return value of WSAStartup
    WORD          VersionRequested;   // passed to WSAStartup
    WSADATA       WsaData;            // receives data from WSAStartup
    BOOL          ReturnValue = TRUE; // return value
	int err;

    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().

    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR);
    Error = WSAStartup(VersionRequested, &WsaData);
    if (Error) {
					
		err = WSAGetLastError();

        MessageBox(NULL,
            "Could not find high enough version of WinSock",
            "BPQ AXIP", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
 
	} else {

        // Now confirm that the WinSock 2 DLL supports the exact version
        // we want. If not, make sure to call WSACleanup().
 
		if (LOBYTE(WsaData.wVersion) != VERSION_MAJOR ||
            HIBYTE(WsaData.wVersion) != VERSION_MINOR) {
            MessageBox(NULL,
                "Could not find the correct version of WinSock",
                "BPQ AXIP",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            WSACleanup();
            ReturnValue = FALSE;
        }
    }

	return(ReturnValue);

} // InitWS2()
  	
struct tagVS_FIXEDFILEINFO * HG;

char VersionString[100];

InitAXIP()
{
	HRSRC RH;
  	struct tagVS_FIXEDFILEINFO * HG;

#ifdef _DEBUG 
	char isDebug[]="Debug Build";
#else
	char isDebug[]="";
#endif

	HMODULE HM;

	HM=GetModuleHandle("bpqaxip.dll");

	RH=FindResource(HM,MAKEINTRESOURCE(VS_VERSION_INFO),RT_VERSION);

	HG=LoadResource(HM,RH);

	(int)HG+=40;

	sprintf(VersionString,"%d.%d.%d.%d %s",
					HIWORD(HG->dwFileVersionMS),
					LOWORD(HG->dwFileVersionMS),
					HIWORD(HG->dwFileVersionLS),
					LOWORD(HG->dwFileVersionLS),
					isDebug);

	//
	//	Read config first, to get UDP info if needed
	//

	if (!ReadConfigFile("BPQAXIP.CFG"))
		return (FALSE);

	MinimizetoTray=GetMinimizetoTrayFlag();

	if (GetStartMinimizedFlag) StartMinimized=GetStartMinimizedFlag();

	//
    //	Start Resolver Thread if needed
	//

	if (NeedResolver)
		_beginthread(ResolveNames, 0, NULL );

	time( &lasttime );			// Get initial time value
 
	_beginthread(OpenSockets, 0, NULL );

	// Start TCP outward connect threads
	//
	//	Open MH window if needed
	
	if (MHEnabled)
		CreateMHWindow();

	return (TRUE);	
}

void OpenSockets(void *dummy)
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

	if (needip)
	{
		sock=socket(AF_INET,SOCK_RAW,IP_AXIP);

		if (sock == INVALID_SOCKET)
		{
			MessageBox(NULL, (LPSTR) "Failed to create RAW socket",NULL,MB_OK);
			err = WSAGetLastError();
  	 		return; 
		}

		ioctlsocket (sock,FIONBIO,&param);
 
		setsockopt (sock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

		sinx.sin_family = AF_INET;
		sinx.sin_addr.s_addr = INADDR_ANY;
		sinx.sin_port = 0;

		destaddr.sin_family = AF_INET;
		destaddr.sin_port = htons(0);

		if (bind(sock, (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
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

	for (i=0;i<NumberofUDPPorts;i++)
	{
		udpsock[i]=socket(AF_INET,SOCK_DGRAM,0);

		if (udpsock[i] == INVALID_SOCKET)
		{
			MessageBox(NULL, (LPSTR) "Failed to create UDP socket",NULL,MB_OK);
			err = WSAGetLastError();
  	 		return; 
		}

		ioctlsocket (udpsock[i],FIONBIO,&param);
 
		setsockopt (udpsock[i],SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

		sinx.sin_family = AF_INET;
		sinx.sin_addr.s_addr = INADDR_ANY;
		sinx.sin_port = htons(udpport[i]);

		destaddr.sin_family = AF_INET;
		destaddr.sin_port = htons(0);

		if (bind(udpsock[i], (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
		{
			//
			//	Bind Failed
			//
			err = WSAGetLastError();
			wsprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
			MessageBox(NULL,Msg,NULL, MB_OK);
			return;
		}
	}

	// Open any TCP sockets

	while (index < arp_table_len)
	{
		arp = &arp_table[index++];

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
			OpenListeningSocket(arp);
		}
	}
}	
OpenListeningSocket(struct arp_table_entry * arp)
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
		MessageBox(hResWnd, Msg, "BPQAXIP", MB_OK);
		return FALSE;
	}

	Debugprintf("TCP Listening Socket Created - socket %d", arp->TCPListenSock);

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

	if ((status = WSAAsyncSelect(arp->TCPListenSock, hResWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{
		sprintf(Msg, "WSAAsyncSelect failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);
		return FALSE;
	}

	return TRUE;
}

void CloseSockets()
{
	int i;
	int index = 0;
	struct arp_table_entry * arp;

	if (needip)
		closesocket(sock);

	for (i=0;i<NumberofUDPPorts;i++)
	{
		closesocket(udpsock[i]);
	}
	
	// Close any open or listening TCP sockets

	while (index < arp_table_len)
	{
		arp = &arp_table[index++];

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





LRESULT CALLBACK ResWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT    hOldFont ;
	struct hostent * hostptr;
	struct in_addr ipad;
	char line[100];
	char outcall[10];
	int index,displayline;

	int i=1;

	int nScrollCode,nPos;

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


	case WM_USER+99:

		i=WSAGETASYNCERROR(lParam);

		arp_table[ResolveIndex].error=i;

		if (i ==0)
		{
			// resolved ok

			hostptr=(struct hostent *)&buf;
			memcpy(&arp_table[ResolveIndex].ipaddr,hostptr->h_addr,4);
		}

  		InvalidateRect(hWnd,NULL,FALSE);

		while (ResolveIndex < arp_table_len)
		{
			ResolveIndex++;
			
			if (arp_table[ResolveIndex].ResolveFlag )
			{
				WSAAsyncGetHostByName (hWnd,WM_USER+99,
						arp_table[ResolveIndex].hostname,
						buf,MAXGETHOSTSTRUCT);	
			
				break;
			}
		}
		break;


	case WM_CHAR:

		if (MHEnabled == FALSE && MHAvailable)
		{
			MHEnabled=TRUE;
			CreateMHWindow();
		}
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!


		if (wmId == BPQREREAD)
		{
			CloseSockets();
			ReadConfigFile("BPQAXIP.CFG");
			_beginthread(OpenSockets, 0, NULL );
			PostMessage(hResWnd, WM_TIMER,0,0);
			InvalidateRect(hWnd,NULL,TRUE);


			return 0;
		}

		if (wmId == BPQADDARP)
		{
			if (ConfigWnd == 0)
			{		
				ConfigWnd=CreateDialog(hInstance,ConfigClassName,0,NULL);
    
				if (!ConfigWnd)
				{
					i=GetLastError();
					return (FALSE);
				}
				ShowWindow(ConfigWnd, SW_SHOW);  
				UpdateWindow(ConfigWnd); 
  			}

			SetForegroundWindow(ConfigWnd);

			return(0);
		}
		return 0;

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

			case  SC_MINIMIZE: 

				GetWindowRect(hResWnd, &ResRect);

				if (MinimizetoTray)

					return ShowWindow(hWnd, SW_HIDE);
				else
					return (DefWindowProc(hWnd, message, wParam, lParam));
					
				break;
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_VSCROLL:
		
		nScrollCode = (int) LOWORD(wParam); // scroll bar value 
		nPos = (short int) HIWORD(wParam);  // scroll box position 

		//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

		if (nScrollCode == SB_LINEUP || nScrollCode == SB_PAGEUP)
		{
			baseline--;
			if (baseline <0)
				baseline=0;
		}

		if (nScrollCode == SB_LINEDOWN || nScrollCode == SB_PAGEDOWN)
		{
			baseline++;
			if (baseline > arp_table_len)
				baseline = arp_table_len;
		}

		if (nScrollCode == SB_THUMBTRACK)
		{
			baseline=nPos;
		}

		SetScrollPos(hWnd,SB_VERT,baseline,TRUE);

		InvalidateRect(hWnd,NULL,TRUE);
		break;


	case WM_PAINT:

		hdc = BeginPaint (hWnd, &ps);
		
		hOldFont = SelectObject( hdc, hFont) ;
			
		index = baseline;
		displayline=0;

		while (index < arp_table_len)
		{
			if (arp_table[index].ResolveFlag && arp_table[index].error != 0)
			{
					// resolver error - Display Error Code
				wsprintf(hostaddr,"Error %d",arp_table[index].error);
			}
			else
			{
				memcpy(&ipad,&arp_table[index].ipaddr,4);
				strncpy(hostaddr,inet_ntoa(ipad),16);
			}
				
			memcpy(&ipad,&arp_table[index].ipaddr,4);

			CONVFROMAX25(arp_table[index].callsign,outcall);
								
			i=wsprintf(line,"%.10s = %.64s = %-.30s %c %c",
				outcall,
				arp_table[index].hostname,
				hostaddr,
				arp_table[index].BCFlag ? 'B' : ' ',
				arp_table[index].TCPState ? 'C' : ' ');

			TextOut(hdc,0,(displayline++)*14+2,line,i);

			index++;
		}

		SelectObject( hdc, hOldFont ) ;
		EndPaint (hWnd, &ps);
	
		break;        

	case WM_DESTROY:
		
       	if (MinimizetoTray)
			DeleteTrayMenuItem(hWnd);

		PostQuitMessage(0);
			
		break;


	case WM_TIMER:
			
		for (ResolveIndex=0; ResolveIndex < arp_table_len; ResolveIndex++)
		{	
			if (arp_table[ResolveIndex].ResolveFlag)
			{
				WSAAsyncGetHostByName (hWnd,WM_USER+99,
						arp_table[ResolveIndex].hostname,
						buf,MAXGETHOSTSTRUCT);
				break;
			}
		}


		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

int FAR PASCAL ConfigWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

	int cmd,id,i;
	HWND hwndChild;
	BOOL OK1,OK2,OK3;

	char call[10], host[65];
	unsigned int ipad;
	int Interval;

	int calllen;
	int	port;
	char axcall[7];
	BOOL UDPFlag, BCFlag;

	switch (message)
	{

	case WM_COMMAND:	

		id = LOWORD(wParam);
        hwndChild = (HWND)(UINT)lParam;
        cmd = HIWORD(wParam);

		switch (id)
		{
		case ID_SAVE:

			OK1=GetDlgItemText(ConfigWnd,1001,(LPSTR)call,10);
			OK2=GetDlgItemText(ConfigWnd,1002,(LPSTR)host,64);
			OK3=1;

			for (i=0;i<7;i++)
				call[i] = toupper(call[i]);
			
			UDPFlag=IsDlgButtonChecked(ConfigWnd,1004);		
			BCFlag=IsDlgButtonChecked(ConfigWnd,1005);		

			if (UDPFlag)
				port=GetDlgItemInt(ConfigWnd,1003,&OK3,FALSE);
			else
				port=0;

			ipad = inet_addr(host);

			Interval=0;

			if (OK1 && OK2 && OK3==1)
			{
				if (convtoax25(call,axcall,&calllen))
				{
					add_arp_entry(axcall,&ipad,calllen,port,host,Interval, BCFlag, FALSE, 0);
					PostMessage(hResWnd, WM_TIMER,0,0);
					return(DestroyWindow(hWnd));
				}
			}

			// Validation failed

			if (!OK1) SetDlgItemText(ConfigWnd,1001,"????");
			if (!OK2) SetDlgItemText(ConfigWnd,1002,"????");
			if (!OK3) SetDlgItemText(ConfigWnd,1003,"????");

			break;

			case ID_CANCEL:

				return(DestroyWindow(hWnd));
		}
		break;

	case WM_CLOSE:
	
		return(DestroyWindow(hWnd));

	case WM_DESTROY:

		ConfigWnd=0;
	
		return(0);

	}		
	
	return (DefWindowProc(hWnd, message, wParam, lParam));

}

void ResolveNames( void *dummy )
{
    int Windowlength, WindowParam;
	WNDCLASS  wc;
	char WindowTitle[100];
	HMENU hMenu,hPopMenu;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];


	// Fill in window class structure with parameters that describe
    // the main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
        wc.lpfnWndProc   = (WNDPROC)ResWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON2));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName =  NULL ;
        wc.lpszClassName = "AppName";

        // Register the window classes

		RegisterClass(&wc);
 
	    wc.style = CS_HREDRAW | CS_VREDRAW;                                      
		wc.cbClsExtra = 0;                
		wc.cbWndExtra = DLGWINDOWEXTRA;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpfnWndProc = ConfigWndProc;       
  		wc.lpszClassName = ConfigClassName;
		RegisterClass(&wc);

	if (arp_table_len > 10 )
	{
		Windowlength=10*14+60;
		WindowParam=WS_OVERLAPPEDWINDOW | WS_VSCROLL;
	}
	else
	{
		Windowlength=(arp_table_len)*14+60;
		WindowParam=WS_OVERLAPPEDWINDOW ;
	}

	sprintf(WindowTitle,"AXIP Resolver Version %s",VersionString);

	hResWnd = CreateWindow("AppName",WindowTitle,
		WindowParam,
		CW_USEDEFAULT, 0, 400, Windowlength,
		NULL, NULL, hInstance, NULL);

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                "SOFTWARE\\G8BPQ\\BPQ32\\BPQAXIP",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"ResSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&ResRect.left,&ResRect.right,&ResRect.top,&ResRect.bottom);
	}

	if (ResRect.right < 100 || ResRect.bottom < 100)
	{
		GetWindowRect(hResWnd, &ResRect);
	}


	MoveWindow(hResWnd,ResRect.left,ResRect.top, ResRect.right-ResRect.left, ResRect.bottom-ResRect.top, TRUE);


	hMenu=CreateMenu();
	hPopMenu=CreatePopupMenu();
	SetMenu(hResWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu,"Update");

	AppendMenu(hPopMenu,MF_STRING,BPQREREAD,"ReRead AXIP.cfg");
	AppendMenu(hPopMenu,MF_STRING,BPQADDARP,"Add Entry");

	DrawMenuBar(hResWnd);	


	LFTTYFONT.lfHeight =			12;
    LFTTYFONT.lfWidth =          8 ;
    LFTTYFONT.lfEscapement =     0 ;
    LFTTYFONT.lfOrientation =    0 ;
    LFTTYFONT.lfWeight =         0 ;
    LFTTYFONT.lfItalic =         0 ;
    LFTTYFONT.lfUnderline =      0 ;
    LFTTYFONT.lfStrikeOut =      0 ;
    LFTTYFONT.lfCharSet =        OEM_CHARSET ;
    LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
    LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
    LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
    LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
    lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;

  	if (arp_table_len > 10 )
		SetScrollRange(hResWnd,SB_VERT,0,arp_table_len-10,TRUE);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
		AddTrayMenuItem(hResWnd, "AXIP Resolver");
	}

	if (StartMinimized)
		if (MinimizetoTray)
			ShowWindow(hResWnd, SW_HIDE);
		else
			ShowWindow(hResWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hResWnd, SW_RESTORE);

	SetTimer(hResWnd,1,15*60*1000,0);	

	PostMessage(hResWnd, WM_TIMER,0,0);

	while (GetMessage(&Msg, NULL, 0, 0)) 
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

	int index;

	int i;
            
	switch (message) { 

	case WM_CHAR:
 
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

			case BPQCLEAR:
				memset(MHTable, 0, sizeof(MHTable));
				InvalidateRect(hMHWnd,NULL,FALSE);
				return 0;

			case BPQCOPY:

				hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,2000);
		
				if (hMem != 0)
				{
					if (OpenClipboard(hWnd))
					{
						CopyScreentoBuffer(GlobalLock(hMem));
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
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

			case SC_MINIMIZE: 

				GetWindowRect(hMHWnd, &MHRect);

				if (MinimizetoTray)

				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));
				
			break;
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_PAINT:

		hdc = BeginPaint (hWnd, &ps);
		hOldFont = SelectObject( hdc, hFont) ;
			
		index = 0;

		while (index < MaxMHEntries)
		{	
			if (MHTable[index].proto != 0)
			{
				CONVFROMAX25(MHTable[index].callsign,outcall);

				i=wsprintf(line,"%-10s%-15s %c %-6d %-25s%c",outcall,
						inet_ntoa(MHTable[index].ipaddr),
						MHTable[index].proto,
						MHTable[index].port,
						asctime(gmtime( &MHTable[index].LastHeard )),
						(MHTable[index].Keepalive == 0) ? ' ' : 'K');

				line[i-2]= ' ';			// Clear CR returned by asctime

				TextOut(hdc,0,(index)*14+2,line,i);
			}
			index++;
		}

		SelectObject( hdc, hOldFont ) ;
		EndPaint (hWnd, &ps);
	
		break;        

		case WM_DESTROY:
		
			if (MinimizetoTray)
			{
				DeleteTrayMenuItem(hWnd);
			}
			
			MHEnabled=FALSE;
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}
BOOL CopyScreentoBuffer(char * buff)
{
	int index;
	char outcall[10];

	index = 0;

	while (index < MaxMHEntries)	
	{	
		if (MHTable[index].proto != 0)
		{
			CONVFROMAX25(MHTable[index].callsign,outcall);

			buff+=wsprintf(buff,"%-10s%-15s %c %-6d %-26s",outcall,
					inet_ntoa(MHTable[index].ipaddr),
					MHTable[index].proto,
					MHTable[index].port,
					asctime(gmtime( &MHTable[index].LastHeard )));
		}
		*(buff-2)=13;
		*(buff-1)=10;
		index++;

	}
	*(buff)=0;

	return 0;
}

void CreateMHWindow( void *dummy )
{
    WNDCLASS  wc;
	char WindowTitle[100];
	HMENU hMenu,hPopMenu;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];

	
	// Fill in window class structure with parameters that describe
    // the main window.

	wc.style         = CS_HREDRAW | CS_VREDRAW ;//| CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)MHWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName =  NULL ;
	wc.lpszClassName = "MHAppName";

        // Register the window class and return success/failure code.

	RegisterClass(&wc);

	sprintf(WindowTitle,"AXIP MHEARD Version %s",VersionString);
  
	hMHWnd = CreateWindow("MHAppName", WindowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 400, 150,
		NULL, NULL, hInstance, NULL);

		
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                "SOFTWARE\\G8BPQ\\BPQ32\\BPQAXIP",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"MHSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&MHRect.left,&MHRect.right,&MHRect.top,&MHRect.bottom);
	}

	if (MHRect.right < 100 || MHRect.bottom < 100)
	{
		GetWindowRect(hMHWnd, &MHRect);
	}


	MoveWindow(hMHWnd,MHRect.left,MHRect.top, MHRect.right-MHRect.left, MHRect.bottom-MHRect.top, TRUE);


	hMenu=CreateMenu();
	hPopMenu=CreatePopupMenu();
	SetMenu(hMHWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu,"Edit");

	AppendMenu(hPopMenu,MF_STRING,BPQCOPY,"Copy");
	AppendMenu(hPopMenu,MF_STRING,BPQCLEAR,"Clear");

	DrawMenuBar(hMHWnd);	

	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hMHWnd, "AXIP MHEARD");
	}

	if (StartMinimized)
		if (MinimizetoTray)
			ShowWindow(hMHWnd, SW_HIDE);
		else
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

BOOL ReadConfigFile(char * fn)
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

	FILE *file;
	char buf[256],errbuf[256];
	int Err;

	HKEY hKey=0;
	UCHAR Value[100];
	UCHAR * BPQDirectory;

	BPQDirectory=GetBPQDirectory();

	wsprintf(errbuf, "BPQAXIP BPQ Directory = %s Filename = %s\n", BPQDirectory, fn);
	OutputDebugString(errbuf);


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

	wsprintf(errbuf, "BPQAXIP Opening %s\n", Value);
	OutputDebugString(errbuf);

		
	if ((file = fopen(Value,"r")) == NULL)
	{
		Err = GetLastError();
		wsprintf(errbuf, "BPQAXIP Open Failed %d\n", Err);
		OutputDebugString(errbuf);

		n=wsprintf(buf,"Config file %s could not be opened ",Value);
		WritetoConsole(buf);

		return (FALSE);
	}

	arp_table_len=0;
	NumberofBroadcastAddreses = 0;
	NumberofUDPPorts=0;
	needip=FALSE;

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf))
		{
			WritetoConsole("BPQAXIP - Bad config record ");
			WritetoConsole(errbuf);
		}
	}
	
	fclose(file);

	if (NumberofUDPPorts > MAXUDPPORTS)
	{
		n=wsprintf(buf,"BPQAXIP - Too many UDP= lines - max is %d\n",MAXUDPPORTS);
		WritetoConsole(buf);
	}
	return (TRUE);
}

ProcessLine(char * buf)
{
	char * ptr;
	char * p_call;
	char * p_ipad;
	char * p_UDP;
	char * p_udpport;
	char * p_Interval;

	int calllen;
	int	port;
	int bcflag;
	char axcall[7];
	unsigned int ipad;
	int Interval;
	int Dynamic=FALSE;
	int TCPMode;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if(_stricmp(ptr,"UDP") == 0)
	{
		if (NumberofUDPPorts > MAXUDPPORTS) NumberofUDPPorts--;

		p_udpport = strtok(NULL, " \t\n\r");
			
		if (p_udpport == NULL) return (FALSE);

		udpport[NumberofUDPPorts] = atoi(p_udpport);

		if (udpport[NumberofUDPPorts++] == 0) return (FALSE);
		
		return (TRUE);
	}

	if(_stricmp(ptr,"MHEARD") == 0)
	{
		MHEnabled = TRUE;
		MHAvailable = TRUE;

		return (TRUE);
	}

	if(_stricmp(ptr,"DONTCHECKSOURCECALL") == 0)
	{
		Checkifcanreply = FALSE;
		return (TRUE);
	}

	if(_stricmp(ptr,"AUTOADDMAP") == 0)
	{
		AutoAddARP = TRUE;
		return (TRUE);
	}
	

	if(_stricmp(ptr,"MAP") == 0)
	{
		p_call = strtok(NULL, " \t\n\r");
		
		if (p_call == NULL) return (FALSE);

		p_ipad = strtok(NULL, " \t\n\r");
		
		if (p_ipad == NULL) return (FALSE);
	
		ipad = inet_addr(p_ipad);

//		if (ipad == INADDR_NONE) return (FALSE);

		p_UDP = strtok(NULL, " \t\n\r");

		Interval=0;
		port=0;				// Raw IP
		bcflag=0;
		TCPMode=0;

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
			add_arp_entry(axcall,&ipad,calllen,port,p_ipad,Interval, bcflag, FALSE, TCPMode);
			return (TRUE);
		}
	}		// End of Process MAP

	if(_stricmp(ptr,"BROADCAST") == 0)
	{
		p_call = strtok(NULL, " \t\n\r");
		
		if (p_call == NULL) return (FALSE);

		if (convtoax25(p_call,axcall,&calllen))
		{
			add_bc_entry(axcall,calllen);
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

BOOL add_arp_entry(unsigned char * call, unsigned long * ip, int len, int port,unsigned char * name, int keepalive, BOOL BCFlag, BOOL AutoAdded, int TCPFlag)
{
	struct arp_table_entry * arp;

	if (arp_table_len == MAX_ENTRIES)
		//
		//	Table full
		//
		return (FALSE); 

	arp = &arp_table[arp_table_len];

	if (port == 0) needip = 1;			// Enable Raw IP Mode

	if (*ip == INADDR_NONE)
	{
		arp->ResolveFlag=TRUE;
		NeedResolver=TRUE;
		*ip = 0;
	}
	else
		arp->ResolveFlag=FALSE;

	memcpy (&arp->callsign,call,7);
	memcpy (&arp->ipaddr,ip,4);
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
	arp_table_len++;

	NeedResolver |= TCPFlag;					// Need Resolver window to handle tcp socket messages
	NeedTCP |= TCPFlag;

	return (TRUE);
}

BOOL add_bc_entry(unsigned char * call, int len)
{
	if (NumberofBroadcastAddreses == MAX_BROADCASTS)
		//
		//	Table full
		//
		return (FALSE);

	memcpy (BroadcastAddresses[NumberofBroadcastAddreses].callsign,call,7);
	BroadcastAddresses[NumberofBroadcastAddreses].len = len;
	NumberofBroadcastAddreses++;

	return (TRUE);
}


int CheckKeepalives()
{
	int index=0,txsock;

	while (index < arp_table_len)
	{
		if (arp_table[index].keepalive != 0)
		{
			arp_table[index].keepalive--;
			
			if (arp_table[index].keepalive == 0)
			{
			//
			//	Send Keepalive Packet
			//
				arp_table[index].keepalive=arp_table[index].keepaliveinit;

				if (arp_table[index].ipaddr != 0)
				{
					destaddr.sin_addr.s_addr = arp_table[index].ipaddr;
					destaddr.sin_port = htons(arp_table[index].port);

					if (arp_table[index].port == 0) txsock = sock; else txsock = udpsock[0];

					sendto(txsock,"Keepalive",9,0,(LPSOCKADDR)&destaddr,sizeof(destaddr));			
				}
			}
		}
	
	index++;

	}

	// Decrement MH Keepalive flags

	for (index = 0; index < MaxMHEntries; index++)
	{
		if (MHTable[index].Keepalive != 0) 
			MHTable[index].Keepalive--;			
	}

	return (0);
}

BOOL CheckSourceisResolvable(char * call, int Port)
{
	// Makes sure we can reply to call before accepting message

	int index = 0;
	struct arp_table_entry * arp;

	while (index < arp_table_len)
	{
		arp = &arp_table[index];

		if (memcmp(arp->callsign, call, arp->len) == 0)
		{
			// Call is present - if AutoAdded, refresh IP address and Port

			if (arp->AutoAdded)
			{
				memcpy (&arp->ipaddr, (PVOID)&rxaddr.sin_addr, 4);
				arp->port = Port;
			}
			return 1;		// Ok to process
		}
		index++;
	}

	return (0);				// Not in list
}

int Update_MH_List(struct in_addr ipad, char * call, char proto, short port)
{
	int index;
	char callsign[7];
	int SaveKeepalive=0;

	memcpy(callsign,call,7);
	callsign[6] &= 0x3e;				// Mask non-ssid bits

	for (index = 0; index < MaxMHEntries; index++)
	{
		if (MHTable[index].callsign[0] == 0) 

			//	empty entry, so call not present. Move all down, and add to front

			goto MoveEntries;

		if (memcmp(MHTable[index].callsign,callsign,7) == 0 &&
					memcmp(&MHTable[index].ipaddr,&ipad,4) == 0 &&
					MHTable[index].proto == proto &&
					MHTable[index].port == port)
		{
			// Entry found, move preceeding entries down and put on front

			SaveKeepalive = MHTable[index].Keepalive;
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
		memmove(&MHTable[1],&MHTable[0],index*sizeof(struct MHTableEntry));

	memcpy(MHTable[0].callsign,callsign,7);

	MHTable[0].ipaddr = ipad;
	MHTable[0].proto = proto;
	MHTable[0].port = port;
	time(&MHTable[0].LastHeard);
	MHTable[0].Keepalive = SaveKeepalive;
	InvalidateRect(hMHWnd,NULL,FALSE);
	return 0;

}

int Update_MH_KeepAlive(struct in_addr ipad, char proto, short port)
{
	int index;

	for (index = 0; index < MaxMHEntries; index++)
	{
		if (MHTable[index].callsign[0] == 0) 

			//	empty entry, so call not present.

			return 0;

		if (memcmp(&MHTable[index].ipaddr,&ipad,4) == 0 &&
				MHTable[index].proto == proto &&
				MHTable[index].port == port)
		{
			MHTable[index].Keepalive = 30;		// 5 Minutes at 10 sec ticks
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
	int addrlen;
	struct arp_table_entry * sockptr;
	SOCKET sock;
	char Msg[100];
	int index=0;
	BOOL bOptVal = TRUE;

	//  Find Socket entry

	Debugprintf("Incoming Connect - Socket %d", SocketId);

	while (index < arp_table_len)
	{
		sockptr = &arp_table[index];

		if (sockptr->TCPListenSock == SocketId)
		{
			addrlen=sizeof(struct sockaddr);

			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				sprintf(Msg, " accept() failed Error %d", WSAGetLastError());
				MessageBox(hResWnd, Msg, "BPQAXIP", MB_OK);
				return FALSE;
			}

			Debugprintf("Connect accepted - Socket %d", sock);

  
			if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bOptVal, 4) != SOCKET_ERROR)
			{

				Debugprintf("Set SO_KEEPALIVE: ON");
			}

			WSAAsyncSelect(sock, hResWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

//			closesocket(sockptr->TCPSock);		// CLose listening socket

			sockptr->TCPSock = sock;
			sockptr->TCPState = TCPConnected;

			return 0;
		}

		index++;
	}

	return 0;

}

int Socket_Connect(int SocketId, int Error)
{
	struct arp_table_entry * sockptr;

	int index=0;

	//   Find Socket entry

	//	Find Connection Record

	WSAGETSELECTERROR(index);


	while (index < arp_table_len)

	{
		sockptr = &arp_table[index++];

		if (sockptr->TCPSock == SocketId)
		{
			Debugprintf("TCP Connect Complete %d result %d", sockptr->TCPSock, Error);

			if (Error == 0)
			{
				sockptr->TCPState = TCPConnected;
			
				WSAAsyncSelect(SocketId, hResWnd, WSA_DATA,
						FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			}
			else
				sockptr->TCPState = 0;

		}
	}
	return 0;
}
int Socket_Data(int sock, int error, int eventcode)
{
	struct arp_table_entry  * sockptr;
	int index=0;

	//	Find Connection Record

	while (index < arp_table_len)
	{
		sockptr = &arp_table[index];

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

	if (InputLen == 0)
		return 0;					// Does this mean closed?

	sockptr->InputLen += InputLen;

	return 0;
}

int GetMessageFromBuffer(char * Buffer)
{
	struct arp_table_entry * sockptr;
	int index=0;
	int MsgLen;
	char * ptr, * ptr2;

	//   Look for data in tcp buffers

	while (index < arp_table_len)
	{
		sockptr = &arp_table[index++];

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
					OpenListeningSocket(sockptr);

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

						if (MHEnabled)
							Update_MH_List(sockptr->in_addr, &Buffer[7],'T', sockptr->port);

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
						if (MHEnabled)
							Update_MH_List(sockptr->in_addr, &Buffer[7],'T', sockptr->port);

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

	Sleep(10000);									// Delay startup a bit

	while(arp->TCPMode == TCPMaster)
	{		
		if (arp->TCPState == 0)
		{
			arp->destaddr.sin_addr.s_addr = arp->ipaddr;

			Debugprintf("TCP Connect Closing socket %d IP %s", arp->TCPSock, arp->hostname);
	
			closesocket(arp->TCPSock);

			arp->TCPSock=socket(AF_INET,SOCK_STREAM,0);

			if (arp->TCPSock == INVALID_SOCKET)
			{
				i=wsprintf(Msg, "Socket Failed for AX/TCP socket - error code = %d\r\n", WSAGetLastError());
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

			arp->destaddr.sin_family = AF_INET; 
			arp->destaddr.sin_addr.s_addr = arp->ipaddr;
			arp->destaddr.sin_port = htons(arp->port);

			if (bind(arp->TCPSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
			{
				//
				//	Bind Failed
				//
	
				i=wsprintf(Msg, "Bind Failed for AX/TCP socket - error code = %d\r\n", WSAGetLastError());
				WritetoConsole(Msg);

  				goto wait; 
			}

			if ((status = WSAAsyncSelect(arp->TCPSock, hResWnd, WSA_CONNECT, FD_CONNECT)) > 0)
			{
				sprintf(Msg, "WSAAsyncSelect failed Error %d", WSAGetLastError());
				MessageBox(hResWnd, Msg, "BPQAXIP", MB_OK);
				closesocket(arp->TCPSock);
				goto wait;
			}

			arp->TCPState = TCPConnecting;

			if (connect(arp->TCPSock,(LPSOCKADDR) &arp->destaddr, sizeof(destaddr)) == 0)
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
    				i=wsprintf(Msg, "Connect Failed for AX/TCP socket - error code = %d\r\n", err);
					WritetoConsole(Msg);
					OutputDebugString(Msg);
					closesocket(arp->TCPSock);

					arp->TCPState =0;
				}
			}
		}
wait:
		Sleep (120000);				// 2 Mins 
	}

	Debugprintf("TCP Connect Thread %x Closing", arp->TCPThreadID);

	arp->TCPThreadID = 0;
	
	return;		// Not Used

}



VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	OutputDebugString(Mess);

	return;
}

