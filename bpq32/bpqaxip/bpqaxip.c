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

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "winver.h"
#include "SHELLAPI.H"

#include "resource.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"

#define BUFFLEN	360	

//	BUFFLEN-4 = L2 POINTER (FOR CLEARING TIMEOUT WHEN ACKMODE USED)
//	BUFFLEN-8 = TIMESTAMP
//	BUFFLEN-12 = BUFFER ALLOCATED FLAG (ADDR OF ALLOCATING ROUTINE)
	
#define MAXDATA	BUFFLEN-16

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

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
unsigned short int compute_crc(unsigned char *buf,int l);
unsigned int find_arp(unsigned char * call);
BOOL add_arp_entry(unsigned char * call, unsigned long ip, int len, int port,unsigned char * name, int keepalive);
BOOL convtoax25(unsigned char * callsign, unsigned char * ax25call, int * calllen);
BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
int CheckKeepalives();
BOOL CopyScreentoBuffer(char * buff);
int DumpFrameInHex(unsigned char * msg, int len);

BOOL MinimizetoTray=FALSE;


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
	unsigned int ipaddr;
	unsigned short port;
	unsigned char hostname[64];
	unsigned int error;
	BOOL ResolveFlag;			// True if need to resolve name
	unsigned int keepalive;
	unsigned int keepaliveinit;
};

unsigned char  hostaddr[64];

HMENU trayMenu;

LOGFONT LFTTYFONT ;

HFONT hFont ;

int arp_table_len=0;
int index=0;					// pointer for table search
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


time_t ltime,lasttime;

char ConfigClassName[]="CONFIG";

HANDLE hInstance;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	hInstance=hInst;

	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		AttachedProcesses++;
		
		if (AXIPInst == 0)				// First entry
		{
			GetAPI();
			AXIPInst = GetCurrentProcessId();

//			STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);

			if (!InitWS2())
				return FALSE;

			if (!InitAXIP())
				return FALSE;

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
	int len,txlen=0,err,txsock,index,digiptr,i;
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
 
					return(1);
				}
				//	
				//	CRC Error
				//

				wsprintf(errmsg,"BPQAXIP Invalid CRC Len=%d Source=%s Port %d",crc,inet_ntoa(rxaddr.sin_addr),udpport[i]);
				OutputDebugString(errmsg);

				return (0);
			}
		}

		return (0);
		
	case 2:				// send

		
		txlen=(buff[6]<<8) + buff[5];

		crc=compute_crc(&buff[7], txlen-7);
		crc ^= 0xffff;

		buff[txlen]=(crc&0xff);
		buff[txlen+1]=(crc>>8);

 		memcpy(axcall,&buff[7],7);	// Set to send to dest addr

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

//
//		Send to all matching calls in arp table - may want to send NODES. ID etc
//		to more than one station
//

		index = 0;

		while (index < arp_table_len)
		{
			if (memcmp(arp_table[index].callsign,axcall,arp_table[index].len) == 0)
			{
				if (arp_table[index].ipaddr != 0)
				{
					destaddr.sin_addr.s_addr = arp_table[index].ipaddr;
					destaddr.sin_port = htons(arp_table[index].port);

					if (arp_table[index].port == 0) txsock = sock; else txsock = udpsock[0];

					sendto(txsock,&buff[7],txlen-5,0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
			
					// reset Keepalive Timer
					
					arp_table[index].keepalive=arp_table[index].keepaliveinit;

				}
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

//		FreeLibrary(ExtDriver);

		break;

	}
	return (0);
}

//DllExport int APIENTRY ExtRX(char * msg, int * len, int * count )
//{
//	return (0);
//}


DllExport int APIENTRY ExtInit(struct PORTCONTROL *  PortEntry)
{
	WritetoConsole("AXIP ");
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

	//
    //	Start Resolver Thread if needed
	//

	if (NeedResolver)
		_beginthread(ResolveNames, 0, NULL );

	time( &lasttime );			// Get initial time value
 
	_beginthread(OpenSockets, 0, NULL );

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
}	

void CloseSockets()
{
	int i;

	if (needip)
		closesocket(sock);

	for (i=0;i<NumberofUDPPorts;i++)
		closesocket(udpsock[i]);

	return ;
}	





LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
								
			i=wsprintf(line,"%.10s = %.64s = %-.30s",outcall,
											arp_table[index].hostname,
											hostaddr);

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
	BOOL UDPFlag;


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
					add_arp_entry(axcall,ipad,calllen,port,host,Interval);
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

	// Fill in window class structure with parameters that describe
    // the main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
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
		CW_USEDEFAULT, 0, 500, Windowlength,
		NULL, NULL, hInstance, NULL);

	hMenu=CreateMenu();
	hPopMenu=CreatePopupMenu();
	SetMenu(hResWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu,"Update");

	AppendMenu(hPopMenu,MF_STRING,BPQREREAD,"ReRead AXIP.cfg");
	AppendMenu(hPopMenu,MF_STRING,BPQADDARP,"Add Entry");

	DrawMenuBar(hMHWnd);	


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

	ShowWindow(hResWnd,SW_RESTORE);

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

				i=wsprintf(line,"%-10s%-15s %c %-6d %-26s",outcall,
						inet_ntoa(MHTable[index].ipaddr),
						MHTable[index].proto,
						MHTable[index].port,
						asctime(gmtime( &MHTable[index].LastHeard )));

				TextOut(hdc,0,(index)*14+2,line,i-2);
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
		CW_USEDEFAULT, 0, 500, (MaxMHEntries)*14+30,
		NULL, NULL, hInstance, NULL);

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

	ShowWindow(hMHWnd,SW_RESTORE);
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

//UDP 9999                               # Port we listen on
//MAP G8BPQ-7 10.2.77.1                  # IP 93 for compatibility
//MAP BPQ7 10.2.77.1 UDP 2222            # UDP port to send to
//MAP BPQ8 10.2.77.2 UDP 3333            # UDP port to send to

	FILE *file;
	char buf[256],errbuf[256];

	HKEY hKey=0;
	UCHAR Value[100];
	UCHAR * BPQDirectory;

	NumberofUDPPorts=0;

	BPQDirectory=GetBPQDirectory();

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
		n=wsprintf(buf,"Config file %s could not be opened ",Value);
		WritetoConsole(buf);

		return (FALSE);
	}

	arp_table_len=0;

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
	char axcall[7];
	unsigned int ipad;
	int Interval;
	int Dynamic=FALSE;

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

	if(_stricmp(ptr,"MAP") == 0)
	{
		p_call = strtok(NULL, " \t\n\r");
		
		if (p_call == NULL) return (FALSE);

		p_ipad = strtok(NULL, " \t\n\r");
		
		if (p_ipad == NULL) return (FALSE);
	
		ipad = inet_addr(p_ipad);

//		if (ipad == INADDR_NONE) return (FALSE);

		p_UDP = strtok(NULL, " \t\n\r");
//
//		Look for (optional) KEEPALIVE and DYNAMIC params
//
		Interval=0;

		if (p_UDP != NULL)
		{
			if (_stricmp(p_UDP,"DYNAMIC") == 0)
			{
				Dynamic=TRUE;
				p_UDP = strtok(NULL, " \t\n\r");
			}

			if (_stricmp(p_UDP,"KEEPALIVE") == 0)
			{
				p_Interval = strtok(NULL, " \t\n\r");

				if (p_Interval == NULL) return (FALSE);

				Interval = atoi(p_Interval);

				p_UDP = strtok(NULL, " \t\n\r");
			}

			if (p_UDP != NULL)
			{
				if (_stricmp(p_UDP,"DYNAMIC") == 0)
				{
					Dynamic=TRUE;
					p_UDP = strtok(NULL, " \t\n\r");
				}	
			}
		
		
		}

		if (p_UDP == NULL)
		{
			needip = TRUE;
			port=0;				// Raw IP
		}
		else
		{
			//
			//	Check for "UDP port"
			//

			if (_stricmp(p_UDP,"UDP") != 0) return (FALSE);
	
			p_udpport = strtok(NULL, " \t\n\r");
			
			if (p_udpport == NULL) return (FALSE);

			port = atoi(p_udpport);

		}

		if (convtoax25(p_call,axcall,&calllen))
		{
			add_arp_entry(axcall,ipad,calllen,port,p_ipad,Interval);
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

unsigned int find_arp(unsigned char * call)
{
	while (index++ < arp_table_len)
	{
		if (memcmp(arp_table[index].callsign,call,arp_table[index].len) == 0)
			return (arp_table[index].ipaddr);
	}
	return (0);
}



BOOL add_arp_entry(unsigned char * call, unsigned long ip, int len, int port,unsigned char * name, int keepalive)
{
	if (arp_table_len == MAX_ENTRIES)
		//
		//	Table full
		//
		return (FALSE);

	if (ip == INADDR_NONE)
	{
		arp_table[arp_table_len].ResolveFlag=TRUE;
		NeedResolver=TRUE;
		ip= 0;
	}
	else
		arp_table[arp_table_len].ResolveFlag=FALSE;

	memcpy (arp_table[arp_table_len].callsign,call,7);
	memcpy (&arp_table[arp_table_len].ipaddr,&ip,4);
	strncpy((char *)&arp_table[arp_table_len].hostname,name,64);
	arp_table[arp_table_len].len = len;
	arp_table[arp_table_len].port = port;
	keepalive+=9;
	keepalive/=10;

	arp_table[arp_table_len].keepalive = keepalive;
	arp_table[arp_table_len].keepaliveinit = keepalive;
	
	arp_table_len++;

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

	return (0);

}
int Update_MH_List(struct in_addr ipad, char * call, char proto, short port)
{
	int index;
	char callsign[7];

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

			// Entry found, move preceeding entries down and put on front

			goto MoveEntries;
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
	time( &MHTable[0].LastHeard);

	InvalidateRect(hMHWnd,NULL,FALSE);
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