
// Module to provide a basic Gateway between IP over AX.25 and the Internet.

// Uses WinPcap on Windows, TAP Driver on Linux

// Basically operates as a mac level bridge, with headers converted between ax.25 and Ethernet.
// ARP frames are also reformatted, and monitored to build a simple routing table.
// Apps may not use ARP (MSYS is configured with an ax.25 default route, rather than an IP one),
// so the default route must be configured. 

// Intended as a gateway for legacy apps, rather than a full function ip over ax.25 router.
// Suggested config is to use the Internet Ethernet Adapter, behind a NAT/PAT Router.
// The ax.25 applications will appear as single addresses on the Ethernet LAN

// The code can also switch packets between ax.25 interfaces

// First Version, July 2008

// Version 1.2.1 January 2009

//	Add IP Address Mapping option

//	June 2014. Convert to Router instead of MAC Bridge, and include a RIP44 decoder
//	so packets can be routed from RF to/from encapsulated 44 net subnets.
//	Routes may also be learned from received RF packets, or added from config file

/*
TODo	?Multiple Adapters
*/

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <stdio.h>
#include <time.h>

#include "CHeaders.h"

#include "IPCode.h"

#ifdef WIN32
#include "pcap.h"
#endif

#ifndef LINBPQ
#include "kernelresource.h"
LRESULT CALLBACK ResWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

extern BPQVECSTRUC * IPHOSTVECTORPTR;

BOOL APIENTRY  Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port);
VOID SENDSABM(struct _LINKTABLE * LINK);
BOOL FindLink(UCHAR * LinkCall, UCHAR * OurCall, int Port, struct _LINKTABLE ** REQLINK);
BOOL ProcessConfig();
VOID RemoveARP(PARPDATA Arp);

VOID ProcessTunnelMsg(PIPMSG IPptr);
VOID ProcessRIP44Message(PIPMSG IPptr);
PROUTEENTRY LookupRoute(ULONG IPADDR, ULONG Mask, BOOL Add, BOOL * Found);
BOOL ProcessROUTELine(char * buf, BOOL Locked);

#define ARPTIMEOUT 3600

//       ARP REQUEST (AX.25)

AXARP AXARPREQMSG = {0};

//		ARP REQUEST/REPLY (Eth)

ETHARP ETHARPREQMSG = {0};

ARPDATA ** ARPRecords = NULL;				// ARP Table - malloc'ed as needed

int NumberofARPEntries = 0;

ROUTEENTRY ** RouteRecords = NULL;

int NumberofRoutes = 0;

//HANDLE hBPQNET = INVALID_HANDLE_VALUE;

ULONG OurIPAddr = 0;
ULONG EncapAddr = 0;

ULONG OurIPBroadcast = 0;
ULONG OurNetMask = 0xffffffff;

BOOL WantTAP = FALSE;

int tap_fd = 0;

int FramesForwarded = 0;
int FramesDropped = 0;
int ARPTimeouts = 0;
int SecTimer = 10;

BOOL NeedResolver = FALSE;

static HMENU hMenu;
extern HMENU hWndMenu;
static HMENU hPopMenu;

extern HKEY REGTREE;

extern int OffsetH, OffsetW;

extern HMENU hMainFrameMenu, hBaseMenu;
extern HWND ClientWnd, FrameWnd;

int map_table_len = 0;
//int index=0;					// pointer for table search
int ResolveIndex=-1;			// pointer to entry being resolved

struct map_table_entry map_table[MAX_ENTRIES];

int Windowlength, WindowParam;


time_t ltime,lasttime;

char ConfigClassName[]="CONFIG";

HWND hIPResWnd = 0;

BOOL IPMinimized;

extern char * PortConfig[];

int baseline=0;

unsigned char  hostaddr[64];


// Following two fields used by stats to get round shared memmory problem

ARPDATA Arp={0};
int ARPFlag = -1;

// Following Buffer is used for msgs from WinPcap. Put the Enet message part way down the buffer, 
//	so there is room for ax.25 header instead of Enet header when we route the frame to ax.25
//	Enet Header ia 14 bytes, AX.25 UI is 16

// Also used to reassemble NOS Fragmented ax.25 packets

static UCHAR Buffer[4096] = {0};

#define EthOffset 30				// Should be plenty

DWORD IPLen = 0;

UCHAR QST[7]={'Q'+'Q','S'+'S','T'+'T',0x40,0x40,0x40,0xe0};		//QST IN AX25

#ifdef WIN32
UCHAR ourMACAddr[6] = {02,'B','P','Q',0,2};
#else
UCHAR ourMACAddr[6] = {02,'B','P','Q',0,1};
#endif

ULONG DefaultIPAddr = 0;

int IPPortMask = 0;

IPSTATS IPStats = {0};

UCHAR BPQDirectory[260];

char ARPFN[MAX_PATH];

HANDLE handle;

#ifdef WIN32
pcap_t *adhandle = 0;
pcap_t * (FAR * pcap_open_livex)(const char *, int, int, int, char *);
#endif

static char Adapter[256];

int Promiscuous = 1;			// Default to Promiscuous

#ifdef WIN32

HINSTANCE PcapDriver=0;

typedef int (FAR *FARPROCX)();

int (FAR * pcap_sendpacketx)();

FARPROCX pcap_compilex;
FARPROCX pcap_setfilterx;
FARPROCX pcap_datalinkx;
FARPROCX pcap_next_exx;
FARPROCX pcap_geterrx;


char Dllname[6]="wpcap";

FARPROCX GetAddress(char * Proc);

#else

#define pcap_compilex pcap_compile
#define pcap_open_livex pcap_open_live
#define pcap_setfilterx pcap_setfilter
#define pcap_datalinkx pcap_datalink
#define pcap_next_exx pcap_next_ex
#define pcap_geterrx pcap_geterr
#define pcap_sendpacketx pcap_sendpacket
#endif
VOID __cdecl Debugprintf(const char * format, ...);

HANDLE hInstance;

void OpenTAP();

Dll BOOL APIENTRY Init_IP()
{
	ARPDATA * ARPptr;

#ifndef LINBPQ

	if (hIPResWnd)
	{
		PostMessage(hIPResWnd, WM_CLOSE,0,0);
//		DestroyWindow(hIPResWnd);

		Debugprintf("IP Init Destroying IP Resolver");
	}
	
	hIPResWnd= NULL;

#endif

	if (BPQDirectory[0] == 0)
	{
		strcpy(ARPFN,"BPQARP.dat");
	}
	else
	{
		strcpy(ARPFN,BPQDirectory);
		strcat(ARPFN,"/");
		strcat(ARPFN,"BPQARP.dat");
	}
	
	ARPRecords = NULL;				// ARP Table - malloc'ed as needed
	NumberofARPEntries=0;

	ReadConfigFile();
	
	// Clear old packets

	IPHOSTVECTORPTR->HOSTAPPLFLAGS = 0x80;			// Request IP frams from Node

	// Set up static fields in ARP messages

	AXARPREQMSG.HWTYPE=0x0300;				//	AX25
	memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
	memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
	AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;		// Set End of Call
	AXARPREQMSG.MSGHDDR.PID = 0xcd;			// ARP
	AXARPREQMSG.MSGHDDR.CTL = 03;			// UI

	AXARPREQMSG.PID=0xcc00;					// TYPE
	AXARPREQMSG.HWTYPE=0x0300;	
	AXARPREQMSG.HWADDRLEN = 7;
	AXARPREQMSG.IPADDRLEN = 4;

	memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
	AXARPREQMSG.SENDIPADDR = OurIPAddr;

	memset(ETHARPREQMSG.MSGHDDR.DEST, 255, 6);
	memcpy(ETHARPREQMSG.MSGHDDR.SOURCE, ourMACAddr, 6);
	ETHARPREQMSG.MSGHDDR.ETYPE = 0x0608;			// ARP

	ETHARPREQMSG.HWTYPE=0x0100;				//	Eth
	ETHARPREQMSG.PID=0x0008;	
	ETHARPREQMSG.HWADDRLEN = 6;
	ETHARPREQMSG.IPADDRLEN = 4;

#ifdef WIN32

    //
    // Open PCAP Driver

	if (Adapter[0])					// Don't have to have ethernet, if used just as ip over ax.25 switch 
	{
		OpenPCAP();

		if (adhandle == NULL)
		{
			WritetoConsoleLocal("Failed to open pcap device - IP Support Disabled\n");
			return FALSE;
		} 

		// Allocate ARP Entry for Default Gateway, and send ARP for it

		if (DefaultIPAddr)
		{
			ARPptr = AllocARPEntry();

			if (ARPptr != NULL)
			{
				ARPptr->ARPINTERFACE = 255;
				ARPptr->ARPTYPE = 'E';
				ARPptr->IPADDR = DefaultIPAddr;
				ARPptr->LOCKED = TRUE;

				SendARPMsg(ARPptr);
			}
		}
	}

#else

	// Linux - if TAP requested, open it
#ifndef MACBPQ

	if (WantTAP)
		OpenTAP();

#endif
#endif

	ReadARP();

#ifndef LINBPQ

	if (NeedResolver)
	{
		WNDCLASS  wc;
		int i;
		char WindowTitle[100];
		int retCode, Type, Vallen;
		HKEY hKey;
		char Size[80];
		RECT Rect = {0,0,0,0};

		retCode = RegOpenKeyEx (REGTREE, "SOFTWARE\\G8BPQ\\BPQ32", 0, KEY_QUERY_VALUE, &hKey);

		if (retCode == ERROR_SUCCESS)
		{
			Vallen=80;

			retCode = RegQueryValueEx(hKey,"IPResSize",0,			
				(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

			if (retCode == ERROR_SUCCESS)
				sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &IPMinimized);

			if (Rect.top < - 500 || Rect.left < - 500)
			{
				Rect.left = 0;
				Rect.top = 0;
				Rect.right = 600;
				Rect.bottom = 400;
			}

			RegCloseKey(hKey);
		}

		// Fill in window class structure with parameters that describe
		// the main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
        wc.lpfnWndProc   = (WNDPROC)ResWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName =  NULL ;
        wc.lpszClassName = "IPAppName";

        // Register the window classes

		RegisterClass(&wc);

		i=GetLastError();
 
		Windowlength=(map_table_len)*14+100;
		WindowParam=WS_OVERLAPPEDWINDOW | WS_VSCROLL;

		sprintf(WindowTitle,"IP Gateway Resolver");

		hIPResWnd = CreateMDIWindow("IPAppName", WindowTitle, WindowParam,
			  Rect.left - (OffsetW /2), Rect.top - OffsetH, Rect.right - Rect.left, Rect.bottom - Rect.top,
			  ClientWnd, hInstance, 1234);

		hPopMenu = CreatePopupMenu();
		AppendMenu(hPopMenu, MF_STRING, BPQREREAD, "ReRead Config");

		SetScrollRange(hIPResWnd,SB_VERT,0, map_table_len,TRUE);

		if (IPMinimized)
			ShowWindow(hIPResWnd, SW_SHOWMINIMIZED);
		else
			ShowWindow(hIPResWnd, SW_RESTORE);

		_beginthread(IPResolveNames, 0, NULL );
	}
#endif
	WritetoConsoleLocal("\n");
	WritetoConsoleLocal("IP Support Enabled\n");

	return TRUE;

}

VOID IPClose()
{
#ifndef LINBPQ
	PostMessage(hIPResWnd, WM_CLOSE,0,0);
//	DestroyWindow(hIPResWnd);

	hIPResWnd = NULL;
#endif
}

Dll BOOL APIENTRY Poll_IP()
{
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;

	// Entered every 100 mS
	
	// if ARPFlag set, copy requested ARP record (For BPQStatus GUI)

	if (ARPFlag != -1)
	{
		memcpy(&Arp, ARPRecords[ARPFlag], sizeof (ARPDATA));
		ARPFlag = -1;
	}

	SecTimer--;

	if (SecTimer == 0)
	{
		SecTimer = 10;
		DoARPTimer();
	}

Pollloop:

#ifdef WIN32

	if (adhandle)
	{
		res = pcap_next_exx(adhandle, &header, &pkt_data);

		if (res > 0)
		{
			PETHMSG ethptr = (PETHMSG)&Buffer[EthOffset];

			if (header->len > 1514)
			{
//				Debugprintf("Ether Packet Len = %d", header->len);
				goto Pollloop;
			}

			memcpy(&Buffer[EthOffset],pkt_data, header->len);

			if (ethptr->ETYPE == 0x0008)
			{
				ProcessEthIPMsg((PETHMSG)&Buffer[EthOffset]);
			//	PIPMSG ipptr = (PIPMSG)&Buffer[EthOffset+14];
			//	ProcessIPMsg(ipptr, ethptr->SOURCE, 'E', 255);
				goto Pollloop;
		}

			if (ethptr->ETYPE == 0x0608)
			{
				ProcessEthARPMsg((PETHARP)ethptr);
				goto Pollloop;
			}

			// Ignore anything else

			goto Pollloop;
		}
	}
#endif

PollTAPloop:

	if (WantTAP && tap_fd)
	{
		int nread;

		nread = read(tap_fd, &Buffer[EthOffset], 1600);

		if (nread > 0)
		{
			PETHMSG ethptr = (PETHMSG)&Buffer[EthOffset];

			if (ethptr->ETYPE == 0x0008)
			{
				ProcessEthIPMsg((PETHMSG)&Buffer[EthOffset]);
				goto PollTAPloop;
			}

			if (ethptr->ETYPE == 0x0608)
			{
				ProcessEthARPMsg((PETHARP)ethptr);
				goto PollTAPloop;
			}

			// if 08FF pass to BPQETHER Driver
/*
			if (ethptr->ETYPE == 0xFF08)
			{
				PBUFFHEADER axmsg;
				PBUFFHEADER savemsg;
				int len;
			
				// BPQEther Encap

				len = Buffer[EthOffset + 15]*256 + Buffer[EthOffset + 14];

				axmsg = (PBUFFHEADER)&Buffer[EthOffset + 9];
				axmsg->LENGTH = len;
				axmsg->PORT = 99;			// Dummy for IP Gate

				printf("BPQ Eth Len %d PID %d\n", len, axmsg->PID);

				if ((len < 16) || (len > 320))
					goto PollTAPloop; // Probably RLI Mode Frame


				//len-=3;
		
			//memcpy(&buff[7],&pkt_data[16],len);
		
			//		len+=5;

				savemsg=axmsg;

				// Packet from AX.25

				if (CompareCalls(axmsg->ORIGIN, MYCALL))
					return 0;				// Echoed packet

				switch (axmsg->PID)
				{
				case  0xcc:

				// IP Message

				{
					PIPMSG ipptr = (PIPMSG)++axmsg;
					axmsg--;
					ProcessIPMsg(ipptr, axmsg->ORIGIN, (axmsg->CTL == 3) ? 'D' : 'V', axmsg->PORT);
					break;
				}

				case 0xcd:
		
				// ARP Message

					ProcessAXARPMsg((PAXARP)axmsg);
					SaveARP();
					break;

	//		case 0x08:
				}

				goto PollTAPloop;
			}
*/
			// Ignore anything else

//			printf("TAP ype %X\n", ntohs(ethptr->ETYPE));

			goto PollTAPloop;
		}
	}

	if (IPHOSTVECTORPTR->HOSTTRACEQ != 0)
	{
		PBUFFHEADER axmsg;
		PBUFFHEADER savemsg;

		axmsg = (PBUFFHEADER)IPHOSTVECTORPTR->HOSTTRACEQ;

		IPHOSTVECTORPTR->HOSTTRACEQ = axmsg->CHAIN;

		savemsg=axmsg;

		// Packet from AX.25

		if (CompareCalls(axmsg->ORIGIN, MYCALL))
		{
			ReleaseBuffer(axmsg);
			return 0;				// Echoed packet
		}
		switch (axmsg->PID)
		{
		case  0xcc:

			// IP Message

			{
				PIPMSG ipptr = (PIPMSG)++axmsg;
				axmsg--;
				ProcessIPMsg(ipptr, axmsg->ORIGIN, (axmsg->CTL == 3) ? 'D' : 'V', axmsg->PORT);
				break;
			}

		case 0xcd:
		
			// ARP Message

			ProcessAXARPMsg((PAXARP)axmsg);
			SaveARP();
			break;

		case 0x08:

			// Fragmented message

			// The L2 code ensures that the last fragment is present before passing the
			// message up to us. It is just possible that bits are missing
			{
				UCHAR * ptr = &axmsg->PID;
				UCHAR * nextfrag;

				int frags;
				int len;

				ptr++;

				if (!(*ptr & 0x80))
					break;					// Not first fragment???
				
				frags=*ptr++ & 0x7f;

				len = axmsg->LENGTH;

				len-= sizeof(BUFFHEADER);
				len--;						// Remove Frag Control Byte

				memcpy(&Buffer[EthOffset], ptr, len);

				nextfrag = &Buffer[EthOffset]+len;

				// Release Buffer
fragloop:
				ReleaseBuffer(savemsg);

				if (IPHOSTVECTORPTR->HOSTTRACEQ == 0)	goto Pollloop;		// Shouldn't happen

				axmsg = (PBUFFHEADER)IPHOSTVECTORPTR->HOSTTRACEQ;
				IPHOSTVECTORPTR->HOSTTRACEQ = axmsg->CHAIN;
				savemsg=axmsg;

				ptr = &axmsg->PID;
				ptr++;
				
				if (--frags != (*ptr++ & 0x7f))
					break;					// Out of sequence

				len = axmsg->LENGTH;

				len-= sizeof(BUFFHEADER);
				len--;						// Remove Frag Control Byte

				memcpy(nextfrag, ptr, len);

				nextfrag+=len;

				if (frags != 0) goto fragloop;

				ProcessIPMsg((PIPMSG)&Buffer[EthOffset+1],axmsg->ORIGIN, (axmsg->CTL == 3) ? 'D' : 'V', axmsg->PORT);

				break;
			}

		}
		// Release the buffer

		ReleaseBuffer(savemsg);

		goto Pollloop;
	}
	return TRUE;
}


BOOL Send_ETH(VOID * Block, DWORD len)
{
	if (WantTAP)
	{
		if (tap_fd)
			write(tap_fd, Block, len);

		return TRUE;
	}

#ifdef WIN32
	if (adhandle)
	{
		if (len < 60) len = 60;

		// Send down the packet 

		pcap_sendpacketx(adhandle,	// Adapter
			Block,				// buffer with the packet
			len);				// size
	}
#endif			
    return TRUE;
}

#define AX25_P_SEGMENT  0x08
#define SEG_REM         0x7F
#define SEG_FIRST       0x80

static VOID Send_AX_Datagram(PMESSAGE Block, DWORD Len, UCHAR Port, UCHAR * HWADDR)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(Block->DEST, HWADDR, 7);
	memcpy(Block->ORIGIN, MYCALL, 7);
	Block->DEST[6] &= 0x7e;						// Clear End of Call
	Block->ORIGIN[6] |= 1;						// Set End of Call
	Block->CTL = 3;		//UI

	if (Port == 99)				// BPQETHER over BPQTUN Port
	{
		// Add BPQETHER Header

		int txlen = Block->LENGTH;
		UCHAR txbuff[1600];
		
		if (txlen < 1 || txlen > 400)
			return;

		// Length field is little-endian

		// BPQEther Header is 14 bytes before the Length 

		txbuff[14]=(txlen & 0xff);
		txbuff[15]=(txlen >> 8);


		memcpy(&txbuff[16],&Block[7],txlen);
		
		//memcpy(&txbuff[0],&EthDest[0],6);
		//memcpy(&txbuff[6],&EthSource[0],6);
		//memcpy(&txbuff[12],&EtherType,2);
	
		write(tap_fd, Block, Len);
		return;
	}
 
	Send_AX(Block, Len, Port);	
	return;

}
VOID Send_AX_Connected(VOID * Block, DWORD Len, UCHAR Port, UCHAR * HWADDR)
{
	DWORD PACLEN=255;
	int fragno;
	int first=1;
	UCHAR * p;
	int txlen;

	// if Len - 16 is grater than PACLEN, then fragment (only possible on Virtual Circuits,
	//	as fragmentation relies upon reliable delivery
	
	if ((Len - 16) <= PACLEN)
	{
		SendNetFrame(HWADDR, MYCALL, Block, Len, Port);
		return;
	}

	Len=Len-15;

	PACLEN-=2;               /* Allow for fragment control info */

	fragno = Len / PACLEN;

	if (Len % PACLEN == 0) fragno--;

	p=Block;
	p+=20;

	while (Len > 0)
	{

	*p++ = AX25_P_SEGMENT;

	*p = fragno--;
	if (first)
	{
		*p |= SEG_FIRST;
		first = 0;
	}

	txlen = (PACLEN > Len) ? Len : PACLEN;

	SendNetFrame(HWADDR, MYCALL, p-23, txlen+17, Port);

	p+=(txlen-1);
	Len-=txlen;
	}

	return;
}

#define MAXDATA BUFFLEN-16


static VOID SendNetFrame(UCHAR * ToCall, UCHAR * FromCall, UCHAR * Block, DWORD Len, UCHAR Port)
{
//	ATTACH FRAME TO OUTBOUND L3 QUEUES (ONLY USED FOR IP ROUTER)

	MESSAGE * buffptr;
	struct _LINKTABLE * LINK;
	struct PORTCONTROL * PORT;

	memcpy(&Block[7],ToCall, 7);
	memcpy(&Block[14],FromCall, 7);

if (Len > MAXDATA)
		return;
	
	if (QCOUNT < 100)
		return;
	
	buffptr = GetBuff();

	if (buffptr == 0)
		return;			// No buffers

	Len -= 15;

	memcpy(&buffptr->DEST, &Block[22], Len);

	buffptr->LENGTH = Len;

//	SEE IF L2 OR L3

	if (Port == 0)				// L3
	{
		struct DEST_LIST * DEST;

		if (FindDestination(ToCall, &DEST) == 0)
		{
			ReleaseBuffer(buffptr);
			return;
		}

		C_Q_ADD(&DEST->DEST_Q, buffptr);
		return;
	}

//	SEND FRAME TO L2 DEST, CREATING A LINK ENTRY IF NECESSARY

	if (FindLink(ToCall, FromCall, Port, &LINK))
	{
		// Have a link

		C_Q_ADD(&LINK->TX_Q, buffptr);		
		return;
	}

	if (LINK == NULL)				// No spare space
	{
		ReleaseBuffer(buffptr);
		return;
	}
	
	LINK->LINKPORT = PORT = GetPortTableEntryFromPortNum(Port);

	if (PORT == NULL)
		return;						// maybe port has been deleted

	LINK->L2TIME = PORT->PORTT1;			// SET TIMER VALUE

	LINK->LINKWINDOW = PORT->PORTWINDOW;

	LINK->L2STATE = 2;

	memcpy(LINK->LINKCALL, ToCall, 7);
	memcpy(LINK->OURCALL, FromCall, 7);

	LINK->LINKTYPE = 2;						// Dopwnlink

	SENDSABM(LINK);

	C_Q_ADD(&LINK->TX_Q, buffptr);		
	return;
}

VOID ProcessEthIPMsg(PETHMSG Buffer)
{
	PIPMSG ipptr = (PIPMSG)&Buffer[1];

	if (memcmp(Buffer, ourMACAddr,6 ) != 0) 
		return;		// Not for us

	if (memcmp(&Buffer[6], ourMACAddr,6 ) == 0) 
		return;		// Discard our sends

	ProcessIPMsg(ipptr, Buffer->SOURCE, 'E', 255);
}

VOID ProcessEthARPMsg(PETHARP arpptr)
{
	int i=0, Mask=IPPortMask;
	PARPDATA Arp;
	BOOL Found;

	if (memcmp(&arpptr->MSGHDDR.SOURCE, ourMACAddr,6 ) == 0 ) 
		return;		// Discard our sends

	switch (arpptr->ARPOPCODE)
	{
	case 0x0100:

		if (arpptr->TARGETIPADDR == 0)		// Request for 0.0.0.0
			return;
	
		// Add to our table, as we will almost certainly want to send back to it

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
				goto AlreadyThere;				// Already there

		if (Arp == NULL) return;				// No point of table full
				
		Arp->IPADDR = arpptr->SENDIPADDR;
		Arp->ARPTYPE = 'E';
		Arp->ARPINTERFACE = 255;
		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,6);
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  ARPTIMEOUT;
		SaveARP();
	
AlreadyThere:

		if (arpptr->TARGETIPADDR == OurIPAddr)
		{
			arpptr->ARPOPCODE = 0x0200;
			memcpy(arpptr->TARGETHWADDR, arpptr->SENDHWADDR ,6);
			memcpy(arpptr->SENDHWADDR, ourMACAddr ,6);

			arpptr->TARGETIPADDR = arpptr->SENDIPADDR;
			arpptr->SENDIPADDR = OurIPAddr;

			memcpy(arpptr->MSGHDDR.DEST, arpptr->MSGHDDR.SOURCE ,6); 
			memcpy(arpptr->MSGHDDR.SOURCE, ourMACAddr ,6); 

			Send_ETH(arpptr,42);

			return;

		}

		// If for our Ethernet Subnet, Ignore or we send loads of unnecessary msgs to ax.25

		if ((arpptr->TARGETIPADDR & OurNetMask) == (OurIPAddr & OurNetMask))
			return;

		// Should't we just reply if we know it ?? (Proxy ARP)

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);

		if (Found)
		{
			if (Arp->ARPVALID && (Arp->ARPTYPE == 'E'))
				return;				// On LAN, so should reply direct

			ETHARPREQMSG.TARGETIPADDR = arpptr->SENDIPADDR;
			ETHARPREQMSG.ARPOPCODE = 0x0200;	// Reply
			ETHARPREQMSG.SENDIPADDR = arpptr->TARGETIPADDR;
					
			memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);
			memcpy(ETHARPREQMSG.MSGHDDR.DEST, arpptr->SENDHWADDR, 6);

			Send_ETH(&ETHARPREQMSG, 42);
			return;
		}

		// Not in our cache, so send to all other ports enabled for IP, reformatting as necessary

		AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
		memset(AXARPREQMSG.TARGETHWADDR, 0, 7);
		AXARPREQMSG.ARPOPCODE = 0x0100;

		for (i=1; i<=NUMBEROFPORTS; i++)
		{
			if (Mask & 1)
				Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, i, QST);

			Mask>>=1;
		}

		break;

	
	case 0x0200:

		if (memcmp(&arpptr->MSGHDDR.DEST, ourMACAddr,6 ) != 0 ) 
			return;		// Not for us

		// Update ARP Cache

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
			goto Update;

		if (Arp == NULL)
			goto SendBack;

Update:
		Arp->IPADDR = arpptr->SENDIPADDR;

		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,6);
		Arp->ARPTYPE = 'E';
		Arp->ARPINTERFACE = 255;
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  ARPTIMEOUT;
		SaveARP();

SendBack:
		
		//  Send Back to Originator of ARP Request

		if (arpptr->TARGETIPADDR == OurIPAddr)		// Reply to our request?
			break;

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);
				
		if (Found)
		{
			if (Arp->ARPINTERFACE == 255)
			{
				ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				ETHARPREQMSG.ARPOPCODE = 0x0200;	// Reply
				ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
					
				memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);
				memcpy(ETHARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 6);

				Send_ETH(&ETHARPREQMSG, 42);
				return;
			}
			else
			{
				AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				AXARPREQMSG.ARPOPCODE = 0x0200;		// Reply
				AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
				memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);
					
				Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, Arp->ARPINTERFACE, Arp->HWADDR);

				return;
			}
		}
		break;

	default:
		break;
	}
	return;
}

VOID ProcessAXARPMsg(PAXARP arpptr)
{
	int i=0, Mask=IPPortMask;
	PARPDATA Arp;
	BOOL Found;

	arpptr->MSGHDDR.ORIGIN[6] &= 0x7e;			// Clear end of Call

	if (memcmp(arpptr->MSGHDDR.ORIGIN, MYCALL, 7) == 0)	// ?Echoed packet?
		return;

	switch (arpptr->ARPOPCODE)
	{
	case 0x0100:
	{
		// Add to our table, as we will almost certainly want to send back to it

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
			goto AlreadyThere;				// Already there
				
		if (Arp != NULL)
		{
			//       ENTRY NOT FOUND - IF ANY SPARE ENTRIES, USE ONE

			Arp->IPADDR = arpptr->SENDIPADDR;

			memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,7);

			Arp->ARPTYPE = 'D';
			Arp->ARPINTERFACE = arpptr->MSGHDDR.PORT;
			Arp->ARPVALID = TRUE;
			Arp->ARPTIMER =  ARPTIMEOUT;
		}

AlreadyThere:

		if (arpptr->TARGETIPADDR == OurIPAddr)
		{	
			arpptr->ARPOPCODE = 0x0200;
			memcpy(arpptr->TARGETHWADDR, arpptr->SENDHWADDR, 7);
			memcpy(arpptr->SENDHWADDR, MYCALL, 7);

			arpptr->TARGETIPADDR = arpptr->SENDIPADDR;
			arpptr->SENDIPADDR = OurIPAddr;

			Send_AX_Datagram((PMESSAGE)arpptr, 46, arpptr->MSGHDDR.PORT, arpptr->MSGHDDR.ORIGIN);

			return;
		}

		// Should't we just reply if we know it ?? (Proxy ARP)

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);

		if (Found)
		{
			// if Trarget is the station we got the request from, there is a loop
			// KIll the ARP entry, and ignore

			if (memcmp(Arp->HWADDR, arpptr->SENDHWADDR, 7) == 0)
			{
				RemoveARP(Arp);
				return;
			}

			AXARPREQMSG.ARPOPCODE = 0x0200;		// Reply
			AXARPREQMSG.TARGETIPADDR = arpptr->SENDIPADDR;
			AXARPREQMSG.SENDIPADDR = arpptr->TARGETIPADDR;

			memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
			memcpy(AXARPREQMSG.TARGETHWADDR, arpptr->SENDHWADDR, 7);

			Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, arpptr->MSGHDDR.PORT, arpptr->SENDHWADDR);

			return;
		}

		// Not in our cache, so send to all other ports enabled for IP, reformatting as necessary
	
		AXARPREQMSG.ARPOPCODE = 0x0100;
		AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

		for (i=1; i<=NUMBEROFPORTS; i++)
		{
			if (i != arpptr->MSGHDDR.PORT)
				if (Mask & 1)
					Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, i, QST);

			Mask>>=1;
		}

		memset(ETHARPREQMSG.MSGHDDR.DEST, 0xff, 6);
		memcpy(ETHARPREQMSG.MSGHDDR.SOURCE, ourMACAddr, 6);

		ETHARPREQMSG.ARPOPCODE = 0x0100;
		ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
		memcpy(ETHARPREQMSG.SENDHWADDR, ourMACAddr, 6);

		Send_ETH(&ETHARPREQMSG, 42);

		break;
	}

	case 0x0200:

		// Update ARP Cache
	
		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);
		
		if (Found)
			goto Update;

		if (Arp == NULL)
			goto SendBack;

Update:
		Arp->IPADDR = arpptr->SENDIPADDR;

		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,7);
		Arp->ARPTYPE = 'D';
		Arp->ARPINTERFACE = arpptr->MSGHDDR.PORT;
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  ARPTIMEOUT;

SendBack:

		//  Send Back to Originator of ARP Request

		if (arpptr->TARGETIPADDR == OurIPAddr)		// Reply to our request?
			break;

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);

		if (Found)
		{
			if (Arp->ARPINTERFACE == 255)
			{
				ETHARPREQMSG.ARPOPCODE = 0x0200;	// Reply

				ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr,6);
				memcpy(ETHARPREQMSG.TARGETHWADDR, Arp->HWADDR, 6);
				memcpy(ETHARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 6);

				Send_ETH(&ETHARPREQMSG, 42);
				return;
			}
			else
			{
				AXARPREQMSG.ARPOPCODE = 0x0200;		// Reply

				AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
				memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);

				Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, Arp->ARPINTERFACE, Arp->HWADDR);

				return;
			}
		}
		
		break;

	
	default:

		return;
	}
}

VOID ProcessIPMsg(PIPMSG IPptr, UCHAR * MACADDR, char Type, UCHAR Port)
{
	ULONG Dest;
	PARPDATA Arp;
	BOOL Found;
	int index, Len;
	PTCPMSG TCPptr;
	PUDPMSG UDPptr;


	if (IPptr->VERLEN != 0x45) return;  // Only support Type = 4, Len = 20

	if (!CheckIPChecksum(IPptr)) return;

	// Make sure origin ia in ARP Table

	Arp = LookupARP(IPptr->IPSOURCE, TRUE, &Found);

	if (!Found)
	{
		// Add if possible

		if (Arp != NULL)
		{
			Arp->IPADDR = IPptr->IPSOURCE;

			if (Type == 'E')
			{
				memcpy(Arp->HWADDR, MACADDR, 6);
			}
			else
			{
				memcpy(Arp->HWADDR, MACADDR, 7);
				Arp->HWADDR[6] &= 0x7e;
			}
			Arp->ARPTYPE = Type;
			Arp->ARPINTERFACE = Port;
			Arp->ARPVALID = TRUE;
			Arp->ARPTIMER =  ARPTIMEOUT;
			SaveARP();
		}
	}
	else
		Arp->ARPTIMER =  ARPTIMEOUT;				// Refresh

	// See if for us - if not pass to router

	Dest = IPptr->IPDEST;

	if (Dest == OurIPAddr || Dest == 0xffffffff || Dest == EncapAddr || Dest == OurIPBroadcast)
		goto ForUs;

	RouteIPMsg(IPptr);

	return;

ForUs:

	if (IPptr->IPPROTOCOL == 4)		// AMPRNET Tunnelled Packet
	{
		ProcessTunnelMsg(IPptr);
		return;
	}

	if (IPptr->IPPROTOCOL == 1)		// ICMP
	{
		ProcessICMPMsg(IPptr);
		return;
	}

	// Support UDP for SNMPfudp

	if (IPptr->IPPROTOCOL == 17)		// UDP
	{
		UDPptr = (PUDPMSG)&IPptr->Data;

		if (UDPptr->DESTPORT == htons(161))
		{
			ProcessSNMPMessage(IPptr);
			return;
		}
	}

	// See if for a mapped Address

	if (IPptr->IPPROTOCOL != 6) return; // Only TCP

	TCPptr = (PTCPMSG)&IPptr->Data;

	Len = ntohs(IPptr->IPLENGTH);
	Len-=20;

	for (index=0; index < map_table_len; index++)
	{
		if ((map_table[index].sourceport == TCPptr->DESTPORT) &&
			map_table[index].sourceipaddr == IPptr->IPSOURCE)
		{
			//	Outgoing Message - replace Dest IP address and Port. Source Port remains unchanged

			IPptr->IPSOURCE = OurIPAddr;
			IPptr->IPDEST = map_table[index].mappedipaddr;
			TCPptr->DESTPORT = map_table[index].mappedport;
			CheckSumAndSend(IPptr, TCPptr, Len);
			return;
		}

		if ((map_table[index].mappedport == TCPptr->SOURCEPORT) &&
			map_table[index].mappedipaddr == IPptr->IPSOURCE)
		{
			//	Incomming Message - replace Dest IP address and Source Port

			IPptr->IPSOURCE = OurIPAddr;
			IPptr->IPDEST = map_table[index].sourceipaddr;
			TCPptr->SOURCEPORT = map_table[index].sourceport;
			CheckSumAndSend(IPptr, TCPptr, Len);
			return;
		}
	}
}

unsigned short cksum(unsigned short *ip, int len)
{
	long sum = 0;  /* assume 32 bit long, 16 bit short */

	while(len > 1)
	{
		sum += *(ip++);
		if(sum & 0x80000000)   /* if high order bit set, fold */
		sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}

	if(len)       /* take care of left over byte */
		sum += (unsigned short) *(unsigned char *)ip;
          
	while(sum>>16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return sum;
}

BOOL CheckIPChecksum(PIPMSG IPptr)
{
	USHORT checksum=0;

	if (IPptr->IPCHECKSUM == 0)
		return TRUE; //Not used

	checksum = cksum((unsigned short *)IPptr, 20);

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
BOOL Check_Checksum(VOID * ptr1, int Len)
{
	USHORT checksum;

	checksum = cksum((unsigned short *)ptr1, Len);

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
USHORT Generate_CHECKSUM(VOID * ptr1, int Len)
{
	USHORT checksum=0;

	checksum = cksum((unsigned short *)ptr1, Len);

	return ~checksum ;
}

VOID ProcessTunnelMsg(PIPMSG IPptr)
{
	UCHAR * ptr;

	//	Make sure it is from UCSD Encap ??How??

	ptr = (UCHAR *)IPptr;
	ptr += 20;						// Skip IPIP Header
	IPptr = (PIPMSG) ptr;

	// First check for RIP44 Messages

	if (IPptr->IPPROTOCOL == 17)		// UDP
	{
		PUDPMSG UDPptr = (PUDPMSG)&IPptr->Data;

		if (UDPptr->DESTPORT == htons(520))
		{
			ProcessRIP44Message(IPptr);
			return;
		}
	}

	// I think anything else is just paaded to the router




		
}

VOID ProcessRIP44Message(PIPMSG IPptr)
{
	int Len;
	PUDPMSG UDPptr = (PUDPMSG)&IPptr->Data;
	PRIP2HDDR HDDR = (PRIP2HDDR)&UDPptr->UDPData;
	PRIP2ENTRY RIP2;

	Len = ntohs(IPptr->IPLENGTH);
	Len -= 20;

	if (Check_Checksum(UDPptr, Len) == FALSE)
		return;

	if	(HDDR->Command != 2 || HDDR->Version != 2)
		return;

	RIP2 = (PRIP2ENTRY) ++HDDR;

	Len -= 12;				// UDP and RIP Headers

	if (RIP2->AddrFamily == 0xFFFF)
	{
		//	Authentication Entry

		Len -= 20;
		RIP2++;
	}

	while (Len > 20)		// Entry LengtH
	{
		// See if already in table

		PROUTEENTRY Route;
		BOOL Found;

		// if for our subnet, ignore

		/*

		;
PUTINLIST:

	PUSH	SI
;
;	IF ENTRY IS A SUBNET ROUTE TO OUR SUBNET, IGNORE IT
;
	PUSH	SI

	MOV	SI,OFFSET TEMPENTRY
	MOV     AX,WORD PTR MYIPADDR+2
	AND	AX,WORD PTR SUBNET+2[SI]
	CMP     AX,WORD PTR NETWORK+2[SI]
	JNE	NOTOURNET1
;
	MOV     AX,WORD PTR MYIPADDR
	AND	AX,WORD PTR SUBNET[SI]
	CMP     AX,WORD PTR NETWORK[SI]
	JNE	NOTOURNET1

	POP	SI
	JMP	SKIPADD
;
*/

		Route = LookupRoute(RIP2->IPAddress, RIP2->Mask, TRUE, &Found);

		if (!Found)
		{
			// Add if possible

			if (Route != NULL && RIP2->Metric < 16)
			{
				Route->NETWORK = RIP2->IPAddress;
				Route->SUBNET = RIP2->Mask;
				Route->METRIC = RIP2->Metric;
				Route->Encap = RIP2->NextHop;
				Route->TYPE = 'T';
				Route->RIPTIMOUT = 600;			// 10 Mins for now
			}
		}
		else
		{
			//	Already in table

			//	Should we replace an RF route with an ENCAP route??
			//	For now, no. May make an option later

			if (Route->TYPE == 'T')
			{
				//	See if same Encap, and if not, is this better metric?

				//	Is this possible with RIP44??

				if (Route->Encap != RIP2->NextHop)
				{
					if (Route->METRIC > RIP2->Metric)
					{
						Route->METRIC = RIP2->Metric;			// Better, so change it
						Route->Encap = RIP2->NextHop;
					}
				}

				Route->METRIC = RIP2->Metric;

				if (RIP2->Metric >= 15)
				{

					//	HE IS TELLING US ITS UNREACHABLE - START DELETE TIMER
	
					Route->RIPTIMOUT = 0;
					if (Route->GARTIMOUT == 0)
						Route ->GARTIMOUT = 4;
				}
	else
				Route->RIPTIMOUT = 600;			// 10 Mins for now
	Route->GARTIMOUT = 0;		// In case started to delete

			}
		}

		Len -= 20;
		RIP2++;
	}

/*
	CALL	TIDY_LIST		; TRY TO MERGE ENTRIES 
;
;	SEE IF ANYTHING HAS CHANGED (DONE HERE SO ENTRIES REMOVED BY
;	TIDY AREN'T INCLUDED
;
	MOV     DI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES
	MOV	AL,0

RIPTESTLOOP:

	OR	AL,ROUTECHANGED[DI]
	MOV	ROUTECHANGED[DI],0

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP    RIPTESTLOOP
;
	OR	AL,AL
	JZ	NOCHANGE

	MOV	RIPTIM,2		; DO ANOTHER BROADCAST

NOCHANGE:

	MOV	AL,RIPTIM
	CALL	BYTE_TO_HEX

	RET
*/
}

/*
UPDATE_ENTRY:

	MOV4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM

	CMP	AX25_CTRL,0FFH
	JE	RIP_U_NET			; VIA NETROM
;
;	VIRTUAL CIRCUIT MODE
;
	MOV	AL,PORT_NO
	MOV     PORT[DI],AL

	MOV	RTYPE[DI],'V'
	JMP SHORT RIP_U_GOT_TYPE

RIP_U_NET:
;
	MOV	RTYPE[DI],'N'
	MOV	PORT[DI],0
;
RIP_U_GOT_TYPE:

	MOV	ROUTEINFO[DI],LEARNED
	MOV     ROUTECHANGED[DI],1

REFRESH_RIP:

	MOV     AL,RIPMETRIC		; INCOMING METRIC
	INC     AL
	CMP     AL,16
	JBE     U_MET_OK
;
;       HE IS TELLING US ITS UNREACHABLE - START DELETE TIMER
;
	MOV     METRIC[DI],16
	MOV     RIPTIMOUT[DI],0
;
	CMP     GARTIMOUT[DI],0
	JNE     NEXTRIPENTRY            ; TIMER ALREADY RUNNING
;
	MOV     GARTIMOUT[DI],4         ; SET WAITING TO DELETE
;
	JMP	NEXTRIPENTRY

U_MET_OK:

	MOV     METRIC[DI],AL

	MOV     RIPTIMOUT[DI],MAXRIP
	MOV     GARTIMOUT[DI],0         ; IN CASE WAITING TO DELETE
;
	JMP	NEXTRIPENTRY

*/




VOID ProcessICMPMsg(PIPMSG IPptr)
{
	int Len;
	PICMPMSG ICMPptr = (PICMPMSG)&IPptr->Data;

	Len = ntohs(IPptr->IPLENGTH);
	Len-=20;

	Check_Checksum(ICMPptr, Len);

	if (ICMPptr->ICMPTYPE == 8)
	{
		//	ICMP_ECHO

		ICMPptr->ICMPTYPE = 0;		// Convert to Reply

		ICMPptr->ICMPCHECKSUM = 0;

		// CHECKSUM IT

		ICMPptr->ICMPCHECKSUM = Generate_CHECKSUM(ICMPptr, Len);

		// Swap Dest to Origin

		IPptr->IPDEST = IPptr->IPSOURCE;

		IPptr->IPSOURCE = OurIPAddr;

		RouteIPMsg(IPptr);			// Send Back
	}

	if (ICMPptr->ICMPTYPE == 8)
	{
		//	ICMP_REPLY:
	}

	// Ingnore others
}

VOID RouteIPMsg(PIPMSG IPptr)
{
	PARPDATA Arp;
	BOOL Found;

	// We rely on the ARP messages generated by either end to route frames.
	//	If address is not in ARP cache (say call originated by MSYS), send to our default route

	// Look up ARP

	Arp = LookupARP(IPptr->IPDEST, FALSE, &Found);

	if (!Found)
		Arp = LookupARP(DefaultIPAddr, FALSE, &Found);

	if (!Found)
		return;				// No route or default
		
	if (Arp->ARPVALID)
	{
		if (Arp->ARPTYPE == 'E')
			SendIPtoBPQDEV(IPptr, Arp->HWADDR);
		else
			SendIPtoAX25(IPptr, Arp->HWADDR, Arp->ARPINTERFACE, Arp->ARPTYPE);
	}
	
	return;	
}

VOID SendIPtoBPQDEV(PIPMSG IPptr, UCHAR * HWADDR)
{	
	// AX.25 headers are bigger, so there will always be room in buffer for enet header
	
	PETHMSG Ethptr = (PETHMSG)IPptr;
	int Len;

	(UCHAR *)Ethptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=14;			// Add eth Header

	memcpy(Ethptr->DEST, HWADDR, 6);
	memcpy(Ethptr->SOURCE, ourMACAddr, 6);
	Ethptr->ETYPE= 0x0008;

	Send_ETH(Ethptr,Len);

	return;
}

VOID SendIPtoAX25(PIPMSG IPptr, UCHAR * HWADDR, int Port, char Mode)
{
	PBUFFHEADER Msgptr = (PBUFFHEADER)IPptr;
	int Len;

	(UCHAR *)Msgptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=16;
	Msgptr->PID = 0xcc;		//IP

	if (Mode == 'D')		// Datagram
	{
		Send_AX_Datagram((PMESSAGE)Msgptr, Len, Port, HWADDR);
		return;
	}

	Send_AX_Connected((PMESSAGE)Msgptr, Len, Port, HWADDR);
}

PROUTEENTRY AllocRouteEntry()
{
	PROUTEENTRY Routeptr;

	if (NumberofRoutes == 0)

		RouteRecords = malloc(4);
	else
		RouteRecords = realloc(RouteRecords,(NumberofRoutes + 1) * 4);

	Routeptr = zalloc(sizeof(ROUTEENTRY));

	if (Routeptr == NULL) return NULL;
	
	RouteRecords[NumberofRoutes++] = Routeptr;
 
	return Routeptr;
}


PARPDATA AllocARPEntry()
{
	ARPDATA * ARPptr;

	if (NumberofARPEntries == 0)

		ARPRecords = malloc(4);
	else
		ARPRecords = realloc(ARPRecords, (NumberofARPEntries+1)*4);

	ARPptr = malloc(sizeof(ARPDATA));

	if (ARPptr == NULL) return NULL;

	memset(ARPptr, 0, sizeof(ARPDATA));
	
	ARPRecords[NumberofARPEntries++] = ARPptr;
 
	return ARPptr;
}

 VOID SendARPMsg(PARPDATA Arp)
 {
	//	Send ARP. Initially used only to find default gateway

	Arp->ARPTIMER =  5;							// Retry periodically

	if (Arp->ARPINTERFACE == 255)				
	{
		ETHARPREQMSG.ARPOPCODE = 0x0100;		//             ; REQUEST

		ETHARPREQMSG.TARGETIPADDR = Arp->IPADDR;						
		memset(ETHARPREQMSG.TARGETHWADDR, 0, 6);

		ETHARPREQMSG.SENDIPADDR = OurIPAddr;
		memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);

		memcpy(ETHARPREQMSG.MSGHDDR.SOURCE, ourMACAddr, 6);
		memset(ETHARPREQMSG.MSGHDDR.DEST, 255, 6);

		Send_ETH(&ETHARPREQMSG, 42);

		return;
	}
	else
	{
		AXARPREQMSG.TARGETIPADDR = Arp->IPADDR;
		memset(AXARPREQMSG.TARGETHWADDR, 0, 7);
		AXARPREQMSG.ARPOPCODE = 0x0100;		//             ; REQUEST
		AXARPREQMSG.SENDIPADDR = OurIPAddr;

		memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
		memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);

		Send_AX_Datagram((PMESSAGE)&AXARPREQMSG, 46, Arp->ARPINTERFACE, QST);

		return;

	}
 }

PROUTEENTRY LookupRoute(ULONG IPADDR, ULONG Mask, BOOL Add, BOOL * Found)
{
	PROUTEENTRY Route = NULL;
	int i;

	for (i = 0; i < NumberofRoutes; i++)
	{
		Route = RouteRecords[i];

		if (Route->NETWORK == IPADDR && Route->SUBNET == Mask)
		{
			*Found = TRUE;
			return Route;
		}
	}

	// Not Found

	*Found = FALSE;

	if (Add)
	{
		Route = AllocRouteEntry();
		return Route;
	}
	else
		return NULL;
}

PARPDATA LookupARP(ULONG IPADDR, BOOL Add, BOOL * Found)
{
	PARPDATA Arp = NULL;
	int i;

	for (i=0; i < NumberofARPEntries; i++)
	{
		Arp = ARPRecords[i];

		if (Arp->IPADDR == IPADDR)
		{
			*Found = TRUE;
			return Arp;
		}
	}

	// Not Found

	*Found = FALSE;

	if (Add)
	{
		Arp = AllocARPEntry();
		return Arp;
	}
	else
		return NULL;
}

VOID RemoveARP(PARPDATA Arp)
{
	int i;

	if (Arp->IPADDR == DefaultIPAddr)
	{
		// Dont remove Default Gateway. Set to re-resolve

		Arp->ARPVALID = FALSE;
		Arp->ARPTIMER = 5;
		return;
	}

	for (i=0; i < NumberofARPEntries; i++)
	{
		if (Arp == ARPRecords[i])
		{
			while (i < NumberofARPEntries)
			{
				ARPRecords[i] = ARPRecords[i+1];
				i++;
			}
			free(Arp);
			NumberofARPEntries--;
			return;
		}
	}
}



	
Dll int APIENTRY GetIPInfo(VOID * ARPRecs, VOID * IPStatsParam, int index)
{
	IPStats.ARPEntries = NumberofARPEntries;
	
	ARPFlag = index;
#ifndef LINBPQ 
	_asm {
		
		mov esi, ARPRecs
		mov DWORD PTR[ESI], offset Arp
	
		mov esi, IPStatsParam
		mov DWORD PTR[ESI], offset IPStats 
	}
#endif
	return ARPFlag;
}


static BOOL ReadConfigFile()
{

// IPAddr 192.168.0.129
// IPBroadcast 192.168.0.255
// IPGateway 192.168.0.1
// IPPorts 1,4 

// MAP 192.168.0.100 1001 n9pmo.dyndns.org 1000

	char * Config;
	char * ptr1, * ptr2;

	char buf[256],errbuf[256];

	map_table_len = 0;				// For reread

	Config = PortConfig[33];		// Config fnom bpq32.cfg

	if (Config)
	{
		// Using config from bpq32.cfg

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf))
			{
				WritetoConsoleLocal("IP Gateway bad config record ");
				strcat(errbuf, "\n");
				WritetoConsoleLocal(errbuf);
			}
		}
	}
	return (TRUE);
}


static ProcessLine(char * buf)
{
	char * ptr, * p_value, * p_origport, * p_host, * p_port;
	int i, port, mappedport, ipad;

	ptr = strtok(buf, " \t\n\r");
	p_value = strtok(NULL, " \t\n\r");


	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if(_stricmp(ptr,"ADAPTER") == 0)
	{
#ifndef WIN32
		WritetoConsoleLocal("IPGating to Ethernet is not supported in this build\n");
		return TRUE;
#endif
		strcpy(Adapter,p_value);
		return (TRUE);
	}

	if(_stricmp(ptr,"promiscuous") == 0)
	{
		Promiscuous = atoi(p_value);
		return (TRUE);
	}

	if(_stricmp(ptr,"USEBPQTAP") == 0)
	{
		WantTAP = TRUE;
		return (TRUE);
	}

	if (_stricmp(ptr,"EncapAddr") == 0)			// For tunnelled packets - may be same as IPAddr
	{
		EncapAddr = inet_addr(p_value);

		if (EncapAddr == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPAddr") == 0)
	{
		OurIPAddr = inet_addr(p_value);

		if (OurIPAddr == INADDR_NONE) return (FALSE);

		return (TRUE);
	}
	if (_stricmp(ptr,"IPBroadcast") == 0)
	{
		OurIPBroadcast = inet_addr(p_value);

		if (OurIPBroadcast == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPNetMask") == 0)
	{
		OurNetMask = inet_addr(p_value);

		if (OurNetMask == INADDR_NONE) return (FALSE);

		return (TRUE);
	}


	if (_stricmp(ptr,"IPGateway") == 0)
	{
		DefaultIPAddr = inet_addr(p_value);

		if (DefaultIPAddr == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPPorts") == 0)
	{
		p_port = strtok(p_value, " ,\t\n\r");
		
		while (p_port != NULL)
		{
			i=atoi(p_port);
			if (i == 0) return FALSE;
			if (i > NUMBEROFPORTS) return FALSE;

			IPPortMask |= 1 << (i-1);
			p_port = strtok(NULL, " ,\t\n\r");
		}
		return (TRUE);
	}


// ARP 44.131.4.18 GM8BPQ-7 1 D

	if (_stricmp(ptr,"ARP") == 0)
	{
		p_value[strlen(p_value)] = ' ';		// put back together
		ProcessARPLine(p_value, TRUE);
		return TRUE;
	}

	if (_stricmp(ptr,"ROUTE") == 0)
	{
		p_value[strlen(p_value)] = ' ';		// put back together
		ProcessROUTELine(p_value, TRUE);
		return TRUE;
	}

	if (_stricmp(ptr,"MAP") == 0)
	{
#ifdef LINBPQ

		WritetoConsoleLocal("MAP not supported in LinBPQ IP Gateway\n");
		return TRUE;
#endif
		if (!p_value) return FALSE;

		p_origport = strtok(NULL, " ,\t\n\r");
		if (!p_origport) return FALSE;

		p_host = strtok(NULL, " ,\t\n\r");
		if (!p_host) return FALSE;

		p_port = strtok(NULL, " ,\t\n\r");
		if (!p_port) return FALSE;

		port=atoi(p_origport);
		if (port == 0) return FALSE;

		mappedport=atoi(p_port);
		if (mappedport == 0) return FALSE;

		ipad = inet_addr(p_value);

		map_table[map_table_len].sourceipaddr = ipad;
		strcpy(map_table[map_table_len].hostname, p_host);
		map_table[map_table_len].sourceport = ntohs(port);
		map_table[map_table_len++].mappedport = ntohs(mappedport);
	
		NeedResolver = TRUE;

		return (TRUE);
	}
	
	//
	//	Bad line
	//
	return (FALSE);
	
}VOID DoARPTimer()
{
	PARPDATA Arp = NULL;
	int i;

	for (i=0; i < NumberofARPEntries; i++)
	{
		Arp = ARPRecords[i];

		if (!Arp->ARPVALID)
		{
			Arp->ARPTIMER--;
			
			if (Arp->ARPTIMER == 0)
			{
				// Retry Request

				SendARPMsg(Arp);
			}
			continue;
		}

		// Time out active entries

		if (Arp->LOCKED == 0)
		{
			Arp->ARPTIMER--;
			
			if (Arp->ARPTIMER == 0)
			{
				// Remove Entry
					RemoveARP(Arp);
			}
		}
	}
}

// PCAP Support Code


#ifdef WIN32

FARPROCX GetAddress(char * Proc)
{
	FARPROCX ProcAddr;
	int err=0;
	char buf[256];
	int n;


	ProcAddr=(FARPROCX) GetProcAddress(PcapDriver,Proc);

	if (ProcAddr == 0)
	{
		err=GetLastError();

		n=sprintf(buf,"Error finding %s - %d", Proc,err);
		WritetoConsoleLocal(buf);
	
		return(0);
	}

	return ProcAddr;
}


void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

int OpenPCAP()
{
	u_long param=1;
	BOOL bcopt=TRUE;
	int i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	char packet_filter[60];
	struct bpf_program fcode;
	char buf[256];
	int n;


	PcapDriver=LoadLibrary(Dllname);

	if (PcapDriver == NULL) return(FALSE);
	
	if ((pcap_sendpacketx=GetAddress("pcap_sendpacket")) == 0 ) return FALSE;

	if ((pcap_datalinkx=GetAddress("pcap_datalink")) == 0 ) return FALSE;

	if ((pcap_compilex=GetAddress("pcap_compile")) == 0 ) return FALSE;

	if ((pcap_setfilterx=GetAddress("pcap_setfilter")) == 0 ) return FALSE;
	
	pcap_open_livex = (pcap_t * (__cdecl *)(const char *, int, int, int, char *)) GetProcAddress(PcapDriver,"pcap_open_live");

	if (pcap_open_livex == NULL) return FALSE;

	if ((pcap_geterrx=GetAddress("pcap_geterr")) == 0 ) return FALSE;

	if ((pcap_next_exx=GetAddress("pcap_next_ex")) == 0 ) return FALSE;


	WritetoConsoleLocal("IP ");

	/* Open the adapter */
	adhandle= pcap_open_livex(Adapter,	// name of the device
							 65536,			// portion of the packet to capture. 
											// 65536 grants that the whole packet will be captured on all the MACs.
							 Promiscuous,	// promiscuous mode (nonzero means promiscuous)
							 1,				// read timeout
							 errbuf			// error buffer
							 );
	
	if (adhandle == NULL)
	{
		n=sprintf(buf,"Unable to open %s\n",Adapter);
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	/* Check the link layer. We support only Ethernet for simplicity. */
	if(pcap_datalinkx(adhandle) != DLT_EN10MB)
	{
		n=sprintf(buf,"\nThis program works only on Ethernet networks.\n");
		WritetoConsoleLocal(buf);
		
		/* Free the device list */
		return -1;
	}

	netmask=0xffffff; 

	sprintf(packet_filter,"ether[12:2]=0x0800 or ether[12:2]=0x0806");

	//compile the filter
	if (pcap_compilex(adhandle, &fcode, packet_filter, 1, netmask) <0 )
	{	
		n=sprintf(buf,"\nUnable to compile the packet filter. Check the syntax.\n");
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	//set the filter

	if (pcap_setfilterx(adhandle, &fcode)<0)
	{
		n=sprintf(buf,"\nError setting the filter.\n");
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	n=sprintf(buf,"Using %s", Adapter);
	WritetoConsoleLocal(buf);

	return TRUE;

}
#endif

VOID ReadARP()
{
	FILE *file;
	char buf[256],errbuf[256];
	
	if ((file = fopen(ARPFN,"r")) == NULL) return;
	
	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessARPLine(buf, FALSE))
		{
			WritetoConsoleLocal("IP Gateway bad ARP record ");
			WritetoConsoleLocal(errbuf);
		}
				
	}
	
	fclose(file);

	return;
}

BOOL ProcessARPLine(char * buf, BOOL Locked)
{
	char * p_ip, * p_mac, * p_port, * p_type;
	int Port;
	char Mac[7];
	char AXCall[7];
	ULONG IPAddr;
	int a,b,c,d,e,f,num;
	
	PARPDATA Arp;
	BOOL Found;

//	192.168.0.131 GM8BPQ-13 1 D

	p_ip = strtok(buf, " \t\n\r");
	p_mac = strtok(NULL, " \t\n\r");
	p_port = strtok(NULL, " \t\n\r");
	p_type = strtok(NULL, " \t\n\r");

	if(p_ip == NULL) return (TRUE);

	if(*p_ip =='#') return (TRUE);			// comment

	if(*p_ip ==';') return (TRUE);			// comment

	if (p_mac == NULL) return FALSE;

	if (p_port == NULL) return FALSE;

	if (p_type == NULL) return FALSE;

	IPAddr = inet_addr(p_ip);

	if (IPAddr == INADDR_NONE) return FALSE;

	if (!((*p_type == 'D') || (*p_type == 'E') || (*p_type =='V'))) return FALSE;

	Port=atoi(p_port);

	if (p_mac == NULL) return (FALSE);

	if (*p_type == 'E')
	{
		num=sscanf(p_mac,"%x:%x:%x:%x:%x:%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		Mac[0]=a;
		Mac[1]=b;
		Mac[2]=c;
		Mac[3]=d;
		Mac[4]=e;
		Mac[5]=f;

		if (Port != 255) return FALSE;
	}
	else
	{
		if (!ConvToAX25(p_mac, AXCall)) return FALSE;
		if ((Port == 0) || (Port > NUMBEROFPORTS)) return FALSE;
	}

	Arp = LookupARP(IPAddr, TRUE, &Found);

	if (!Found)
	{
		// Add if possible

		if (Arp != NULL)
		{
			Arp->IPADDR = IPAddr;

			if (*p_type == 'E')
			{
				memcpy(Arp->HWADDR, Mac, 6);
			}
			else
			{
				memcpy(Arp->HWADDR, AXCall, 7);
				Arp->HWADDR[6] &= 0x7e;
			}
			Arp->ARPTYPE = *p_type;
			Arp->ARPINTERFACE = Port;
			Arp->ARPVALID = TRUE;
			Arp->ARPTIMER =  (Arp->ARPTYPE == 'E')? 300 : ARPTIMEOUT;
			Arp->LOCKED = Locked;
		}
	}
	return TRUE;
}


// ROUTE 44.131.4.18/32 D GM8BPQ-7 1		// Datagram?netrom/VC via Call
// ROUTE 44.131.4.18/32 T n.n.n.n			// Vis Tunnel Endpoint n.n.n.n
// ROUTE 44.131.4.18/32 E n.n.n.n			// Via IP address over Ethernet


BOOL ProcessROUTELine(char * buf, BOOL Locked)
{
	char * p_ip, * p_target, * p_port, * p_type, * p_mask;
	int Port;
	char Mac[7];
	char AXCall[7];
	ULONG IPAddr, IPMask;
	int a,b,c,d,e,f,num;
	
	PROUTEENTRY Route;
	BOOL Found;

//	192.168.0.131 GM8BPQ-13 1 D

	p_ip = strtok(buf, " \t\n\r");
	p_type = strtok(NULL, " \t\n\r");
	p_target = strtok(NULL, " \t\n\r");
	p_port = strtok(NULL, " \t\n\r");

	if(p_ip == NULL) return (TRUE);

	if(*p_ip =='#') return (TRUE);			// comment

	if(*p_ip ==';') return (TRUE);			// comment

	if (p_target == NULL) return FALSE;

	if (p_port == NULL) return FALSE;

	if (p_type == NULL) return FALSE;

	p_mask = strchr(p_ip, '/');

	if (p_mask)
	{
		int Bits = atoi(p_mask + 1);

		if (Bits > 32)
			Bits = 32;

		IPMask = (0xFFFFFFFF) << (32 - Bits);
	}
	else
		IPMask = 0;

	IPAddr = inet_addr(p_ip);

	if (IPAddr == INADDR_NONE) return FALSE;

	if (!((*p_type == 'D') || (*p_type == 'E') || (*p_type =='V') || (*p_type =='T'))) return FALSE;

	Port=atoi(p_port);

	if (*p_type == 'T')
	{
		num=sscanf(p_target,"%x.%x.%x.%x",&a,&b,&c,&d);

		if (num != 4) return FALSE;
	}
	else if (*p_type == 'E')
	{
		num=sscanf(p_target,"%x:%x:%x:%x:%x:%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		Mac[0]=a;
		Mac[1]=b;
		Mac[2]=c;
		Mac[3]=d;
		Mac[4]=e;
		Mac[5]=f;

		if (Port != 255) return FALSE;
	}
	else
	{
		if (!ConvToAX25(p_target, AXCall)) return FALSE;
		if ((Port == 0) || (Port > NUMBEROFPORTS)) return FALSE;
	}

	Route = LookupRoute(IPAddr, IPMask, TRUE, &Found);

	if (!Found)
	{
		// Add if possible

		if (Route != NULL)
		{
			Route->NETWORK = IPAddr;
			Route->SUBNET = IPMask;

			if (*p_type == 'E')
			{
				memcpy(Route->Target, Mac, 6);
			}
			else
			{
				memcpy(Route->Target, AXCall, 7);
				Route->Target[6] &= 0x7e;
			}
			Route->TYPE = *p_type;
			Route->PORT = Port;
			Route->LOCKED = Locked;
		}
	}
	return TRUE;
}


VOID SaveARP ()
{
	PARPDATA Arp;
	int i;
	FILE * file;

	if ((file = fopen(ARPFN, "w")) == NULL)
		return;

	for (i=0; i < NumberofARPEntries; i++)
	{
		Arp = ARPRecords[i];
		if (Arp->ARPVALID && !Arp->LOCKED) 
			WriteARPLine(Arp, file);
	}

 	fclose(file);
	
	return ;
}

VOID WriteARPLine(PARPDATA ARPRecord, FILE * file)
{
	int SSID, Len, j;
	char Mac[20];
	char Call[7];
	char IP[20];
	char Line[100];
	unsigned char work[4];

	memcpy(work, &ARPRecord->IPADDR, 4);

	sprintf(IP, "%d.%d.%d.%d", work[0], work[1], work[2], work[3]);

	if(ARPRecord->ARPINTERFACE == 255)		// Ethernet
	{
		sprintf(Mac," %02x:%02x:%02x:%02x:%02x:%02x", 
				ARPRecord->HWADDR[0],
				ARPRecord->HWADDR[1],
				ARPRecord->HWADDR[2],
				ARPRecord->HWADDR[3],
				ARPRecord->HWADDR[4],
				ARPRecord->HWADDR[5]);
	}
	else
	{
		for (j=0; j< 6; j++)
		{
			Call[j] = ARPRecord->HWADDR[j]/2;
			if (Call[j] == 32) Call[j] = 0;
		}
		Call[6] = 0;
		SSID = (ARPRecord->HWADDR[6] & 31)/2;
			
		sprintf(Mac,"%s-%d", Call, SSID);
	}

	Len = sprintf(Line,"%s %s %d %c\n",
			IP, Mac, ARPRecord->ARPINTERFACE, ARPRecord->ARPTYPE);

	fputs(Line, file);
	
	return;
}

#ifndef LINBPQ

extern HFONT hFont;
struct tagMSG Msg;
char buf[1024];

LRESULT CALLBACK ResWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT    hOldFont ;
	struct hostent * hostptr;
	struct in_addr ipad;
	char line[100];
	int index,displayline;
	MINMAXINFO * mmi;

	int i=1;

	int nScrollCode,nPos;

	switch (message)
	{
	case WM_GETMINMAXINFO:

 		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 400;
		mmi->ptMaxSize.y = Windowlength;
		mmi->ptMaxTrackSize.x = 400;
		mmi->ptMaxTrackSize.y = Windowlength;
		break;

	case WM_USER+199:

		i=WSAGETASYNCERROR(lParam);

		map_table[ResolveIndex].error=i;

		if (i ==0)
		{
			// resolved ok

			hostptr=(struct hostent *)&buf;
			memcpy(&map_table[ResolveIndex].mappedipaddr,hostptr->h_addr,4);
		}

  		InvalidateRect(hWnd,NULL,FALSE);

		while (ResolveIndex < map_table_len)
		{
			ResolveIndex++;
			
			WSAAsyncGetHostByName (hWnd,WM_USER+199,
						map_table[ResolveIndex].hostname,
						buf,MAXGETHOSTSTRUCT);	
			
			break;
		}
		break;

	case WM_MDIACTIVATE:
	{ 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (WPARAM)hPopMenu, "Actions");

			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM)hWndMenu);
		}
		else
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM)hMainFrameMenu, (LPARAM)NULL);
			
		DrawMenuBar(FrameWnd);

		return DefMDIChildProc(hWnd, message, wParam, lParam);

	}

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!



		if (wmId == BPQREREAD)
		{
			ProcessConfig();
			FreeConfig();

			ReadConfigFile();
			PostMessage(hIPResWnd, WM_TIMER,0,0);
			InvalidateRect(hWnd,NULL,TRUE);

			return 0;
		}
/*
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
*/
	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{ 
		case SC_RESTORE:

			IPMinimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			break;

		case SC_MINIMIZE: 

			IPMinimized = TRUE;
			break;
		}
		return DefMDIChildProc(hWnd, message, wParam, lParam);

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
			if (baseline > map_table_len)
				baseline = map_table_len;
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

		while (index < map_table_len)
		{
			if (map_table[index].ResolveFlag && map_table[index].error != 0)
			{
					// resolver error - Display Error Code
				sprintf(hostaddr,"Error %d",map_table[index].error);
			}
			else
			{
				memcpy(&ipad,&map_table[index].mappedipaddr,4);
				strncpy(hostaddr,inet_ntoa(ipad),16);
			}
				
			memcpy(&ipad,&map_table[index].mappedipaddr,4);
								
			i=sprintf(line,"%.64s = %-.30s",
				map_table[index].hostname,
				hostaddr);

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
			
		for (ResolveIndex=0; ResolveIndex < map_table_len; ResolveIndex++)
		{	
			WSAAsyncGetHostByName (hWnd,WM_USER+199,
						map_table[ResolveIndex].hostname,
						buf,MAXGETHOSTSTRUCT);
			break;	
		}

	default:
		break;
	}
			return DefMDIChildProc(hWnd, message, wParam, lParam);
}

void IPResolveNames( void *dummy )
{
	SetTimer(hIPResWnd,1,15*60*1000,0);	

	PostMessage(hIPResWnd, WM_TIMER,0,0);

	while (GetMessage(&Msg, hIPResWnd, 0, 0)) 
	{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
	}		
}

#endif

/*
;	DO PSEUDO HEADER FIRST
;
	MOV	DX,600H			; PROTOCOL (REVERSED)
	MOV	AX,TCPLENGTH		; TCP LENGTH
	XCHG	AH,AL
	ADD	DX,AX
	MOV	AX,WORD PTR LOCALADDR[BX]
	ADC	DX,AX
	MOV	AX,WORD PTR LOCALADDR+2[BX]
	ADC	DX,AX
	MOV	AX,WORD PTR REMOTEADDR[BX]
	ADC	DX,AX
	MOV	AX,WORD PTR REMOTEADDR+2[BX]
	ADC	DX,AX
	ADC	DX,0

	MOV	PHSUM,DX

	PUSH	BX

	MOV	BX,TXBUFFER		; HEADER

	MOV	CX,TCPLENGTH		; PUT LENGTH INTO HEADER
	MOV	BUFFLEN[BX],CX
;
	MOV	SI,BUFFPTR[BX]

	INC	CX			; ROUND UP
	SHR	CX,1			; WORD COUNT

	CALL	DO_CHECKSUM

	ADD	DX,PHSUM
	ADC	DX,0
	NOT	DX

	MOV	SI,BUFFPTR[BX]
	MOV	CHECKSUM[SI],DX


*/

int CheckSumAndSend(PIPMSG IPptr, PTCPMSG TCPmsg, USHORT Len)
{

	struct _IPMSG PH = {0};
		
	IPptr->IPCHECKSUM = 0;

	PH.IPPROTOCOL = 6;
	PH.IPLENGTH = htons(Len);
	memcpy(&PH.IPSOURCE, &IPptr->IPSOURCE, 4);
	memcpy(&PH.IPDEST, &IPptr->IPDEST, 4);

	TCPmsg->CHECKSUM = ~Generate_CHECKSUM(&PH, 20);

	TCPmsg->CHECKSUM = Generate_CHECKSUM(TCPmsg, Len);

	// CHECKSUM IT

	IPptr->IPCHECKSUM = Generate_CHECKSUM(IPptr, 20);
	RouteIPMsg(IPptr);
	return 0;
}

int CheckSumAndSendUDP(PIPMSG IPptr, PUDPMSG UDPmsg, USHORT Len)
{
	struct _IPMSG PH = {0};
		
	IPptr->IPCHECKSUM = 0;

	PH.IPPROTOCOL = 17;
	PH.IPLENGTH = htons(Len);
	memcpy(&PH.IPSOURCE, &IPptr->IPSOURCE, 4);
	memcpy(&PH.IPDEST, &IPptr->IPDEST, 4);

	UDPmsg->CHECKSUM = ~Generate_CHECKSUM(&PH, 20);

	UDPmsg->CHECKSUM = Generate_CHECKSUM(UDPmsg, Len);

	// CHECKSUM IT

	IPptr->IPCHECKSUM = Generate_CHECKSUM(IPptr, 20);
	RouteIPMsg(IPptr);
	return 0;
}
#ifndef WIN32
#ifndef MACBPQ

#include <net/if.h>
#include <linux/if_tun.h>

/* buffer for reading from tun/tap interface, must be >= 1500 */


#define BUFSIZE 2000   

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            must reserve enough space in *dev.                          *
 **************************************************************************/
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);
  return fd;
}


void OpenTAP()
{
  int flags = IFF_TAP;
  char if_name[IFNAMSIZ] = "bpqtap";

  uint nread, nwrite, plength;
  char buffer[BUFSIZE];

  int optval = 1;

  /* initialize tun/tap interface */

  if ((tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 )
  {
    printf("Error connecting to tun/tap interface %s!\n", if_name);
	tap_fd = 0;
    return;
  }

  printf("Successfully connected to interface %s\n", if_name);

  ioctl(tap_fd, FIONBIO, &optval);
  return;
}

#endif
#endif

extern struct DATAMESSAGE * REPLYBUFFER;

VOID SHOWARP(TRANSPORTENTRY * Session, char * Bufferptr, char * CmdTail, CMDX * CMD)
{
	//	DISPLAY IP Gateway ARP status or Clear
	
	int i;
	PARPDATA ARPRecord, Arp;
	int SSID, j;
	char Mac[20];
	char Call[7];
	char IP[20];
	unsigned char work[4];

	Bufferptr += sprintf(Bufferptr, "\r");

	if (IPRequired == FALSE)
	{
		Bufferptr += sprintf(Bufferptr, "IP Gateway is not enabled\r");
		SendCommandReply(Session, REPLYBUFFER, Bufferptr - (char *)REPLYBUFFER);
		return;
	}

	if (memcmp(CmdTail, "CLEAR ", 6) == 0)
	{
		int n = NumberofARPEntries;
		int rec = 0;

		for (i=0; i < n; i++)
		{
			Arp = ARPRecords[rec];
			if (Arp->LOCKED)
				rec++;
			else
				RemoveARP(Arp);
		}

		Bufferptr += sprintf(Bufferptr, "OK\r");
		SendCommandReply(Session, REPLYBUFFER, Bufferptr - (char *)REPLYBUFFER);
		SaveARP();

		return;
	}

	for (i=0; i < NumberofARPEntries; i++)
	{
		ARPRecord = ARPRecords[i];

		if (ARPRecord->ARPVALID)
		{
			Bufferptr = CHECKBUFFER(Session, Bufferptr);	// ENSURE ROOM
			memcpy(work, &ARPRecord->IPADDR, 4);
			sprintf(IP, "%d.%d.%d.%d", work[0], work[1], work[2], work[3]);

			if(ARPRecord->ARPINTERFACE == 255)		// Ethernet
			{
				sprintf(Mac," %02x:%02x:%02x:%02x:%02x:%02x", 
					ARPRecord->HWADDR[0],
					ARPRecord->HWADDR[1],
					ARPRecord->HWADDR[2],
					ARPRecord->HWADDR[3],
					ARPRecord->HWADDR[4],
					ARPRecord->HWADDR[5]);
			}
			else
			{
				for (j=0; j< 6; j++)
				{
					Call[j] = ARPRecord->HWADDR[j]/2;
					if (Call[j] == 32) Call[j] = 0;
				}
				Call[6] = 0;
				SSID = (ARPRecord->HWADDR[6] & 31)/2;
			
				sprintf(Mac,"%s-%d", Call, SSID);
			}

			Bufferptr += sprintf(Bufferptr, "%s %s %d %c %d %s\r",
				IP, Mac, ARPRecord->ARPINTERFACE, ARPRecord->ARPTYPE,
				(int)ARPRecord->ARPTIMER, ARPRecord->LOCKED?"Locked":"");
		}
	}

	SendCommandReply(Session, REPLYBUFFER, Bufferptr - (char *)REPLYBUFFER);
}

// SNMP Support Code. Pretty limited - basically just for MRTG

/*
Primitive ASN.1 Types	Identifier in hex
INTEGER	02
BIT STRING	03
OCTET STRING	04
NULL	05
OBJECT IDENTIFIER	06

Constructed ASN.1 type	Identifier in hex
SEQUENCE	30

Primitive SNMP application types	Identifier in hex
IpAddress	40
Opaque	44
NsapAddress	45
Counter64 (available only in SNMPv2)	46
Uinteger32 (available only in SNMPv2)	47

Context-specific types within an SNMP Message	Identifier in hex
GetRequest-PDU	A0
GetNextRequestPUD	A1
GetResponse-PDU (Response-PDU in SNMPv 2)	A2
SetRequest-PDU	A3
Trap-PDU (obsolete in SNMPv 2)	A4
GetBulkRequest-PDU (added in SNMPv 2)	A5
InformRequest-PDU (added in SNMPv 2)	A6
SNMPv2-Trap-PDU (added in SNMPv 2)	A7

*/

#define Counter32 0x41
#define Gauge32 0x42
#define TimeTicks	0x43



UCHAR ifInOctets[] = {'+',6,1,2,1,2,2,1,10};
UCHAR ifOutOctets[] = {'+',6,1,2,1,2,2,1,16};

int ifInOctetsLen = 9;		// Not Inc  Port
int ifOutOctetsLen = 9;		// Not Inc  Port

UCHAR sysUpTime[] = {'+', 6,1,2,1,1,3,0};
int sysUpTimeLen = 8;

UCHAR sysName[] = {'+', 6,1,2,1,1,5,0};
int sysNameLen = 8;

extern time_t TimeLoaded;

int InOctets[32] = {0};
int OutOctets[32] = {0};

//	ASN PDUs have to be constructed backwards, as each header included a length

//	This code assumes we have enough space in front of the buffer.

int ASNGetInt(UCHAR * Msg, int Len)
{
	int Val = 0;

	while(Len)
	{
		Val = (Val << 8) + *(Msg++);
		Len --;
	}

	return Val;
}


int ASNPutInt(UCHAR * Buffer, int Offset, unsigned int Val, int Type)
{
	int Len = 0;
	
	// Encode in minimum space. But servers seem to sign-extend top byte, so if top bit set add another zero;

	while(Val)
	{
		Buffer[--Offset] = Val & 255;	// Value
		Val = Val >> 8;
		Len++;
	}

	if (Len < 4 && (Buffer[Offset] & 0x80))	// Negative
	{
		Buffer[--Offset] = 0;
		Len ++;
	}

	Buffer[--Offset] = Len;				// Len
	Buffer[--Offset] = Type;

	return Len + 2;
}


int AddHeader(UCHAR * Buffer, int Offset, UCHAR Type, int Length)
{
	Buffer[Offset - 2] = Type;
	Buffer[Offset - 1] = Length;

	return 2;
}

int BuildReply(UCHAR * Buffer, int Offset, UCHAR * OID, int OIDLen, UCHAR * Value, int ReqID)
{
	int IDLen;
	int ValLen = Value[1] + 2;

	// Value is pre-encoded = type, len, data

	// Contruct the Varbindings. Sequence OID Value

	Offset -= ValLen;
	memcpy(&Buffer[Offset], Value, ValLen);
	Offset -= OIDLen;
	memcpy(&Buffer[Offset], OID, OIDLen);
	Buffer[--Offset] = OIDLen;
	Buffer[--Offset] = 6;				// OID Type

	Buffer[--Offset] = OIDLen + ValLen + 2;
	Buffer[--Offset] = 48;				// Sequence

	Buffer[--Offset] = OIDLen + ValLen + 4;
	Buffer[--Offset] = 48;				// Sequence


	// Add the error fields (two zero ints

	Buffer[--Offset] = 0;				// Value
	Buffer[--Offset] = 1;				// Len
	Buffer[--Offset] = 2;				// Int

	Buffer[--Offset] = 0;				// Value
	Buffer[--Offset] = 1;				// Len
	Buffer[--Offset] = 2;				// Int

	// ID

	IDLen = ASNPutInt(Buffer, Offset, ReqID, 2);
	Offset -= IDLen;

	// PDU Type

	Buffer[--Offset] = OIDLen + ValLen + 12 + IDLen;
	Buffer[--Offset] = 0xA2;				// Len

	return OIDLen + ValLen + 14 + IDLen;
}



//   snmpget -v1 -c jnos [ve4klm.ampr.org | www.langelaar.net] 1.3.6.1.2.1.2.2.1.16.5


VOID ProcessSNMPMessage(PIPMSG IPptr)
{
	int Len;
	PUDPMSG UDPptr = (PUDPMSG)&IPptr->Data;
	char Community[256];
	UCHAR OID[256];
	int OIDLen;
	UCHAR * Msg;
	int Type;
	int Length, ComLen;
	int  IntVal;
	int ReqID;
	int RequestType;

	Len = ntohs(IPptr->IPLENGTH);
	Len-=20;

	Check_Checksum(UDPptr, Len);

	// 4 bytes version
	// Null Terminated Community

	Msg = (char *) UDPptr;

	Msg += 8;				// Over UDP Header
	Len -= 8;

	// ASN 1 Encoding - Type, Len, Data

	while (Len > 0)
	{
		Type = *(Msg++);
		Length = *(Msg++);

		// First should be a Sequence

		if (Type != 0x30)
			return;

		Len -= 2;

		Type = *(Msg++);
		Length = *(Msg++);
		IntVal =  *(Msg++);

		// Should be Integer - SNMP Version - We support V1, identified by zero

		if (Type != 2 || Length != 1 || IntVal != 0)
			return;

		Len -= 3;

		Type = *(Msg++);
		ComLen = *(Msg++);

		// Should  Be String (community)

		if (Type != 4)
			return;

		memcpy(Community, Msg, ComLen);
		Community[ComLen] = 0;

		Len -=2;				// Header
		Len -= ComLen;

		Msg += (ComLen);

		// A Complex Data Types - GetRequest PDU etc

		RequestType = *(Msg);
		*(Msg++) = 0xA2;
		Length = *(Msg++);

		Len -= 2; 

		// A 2 byte value requestid

		// Next is integer requestid

		Type = *(Msg++);
		Length = *(Msg++);

		if (Type != 2)
			return;

		ReqID = ASNGetInt(Msg, Length);
 
		Len -= (2 + Length);
		Msg += Length;

		// Two more Integers - error status, error index
	
		Type = *(Msg++);
		Length = *(Msg++);

		if (Type != 2)
			return;

		ASNGetInt(Msg, Length);
 
		Len -= (2 + Length);
		Msg += Length;

		Type = *(Msg++);
		Length = *(Msg++);

		if (Type != 2)
			return;

		ASNGetInt(Msg, Length);
 
		Len -= (2 + Length);
		Msg += Length;

		// Two Variable-bindings structs - another Sequence

		Type = *(Msg++);
		Length = *(Msg++);

		Len -= 2;

		if (Type != 0x30)
			return;

		Type = *(Msg++);
		Length = *(Msg++);

		Len -= 2;

		if (Type != 0x30)
			return;

		// Next is OID 

		Type = *(Msg++);
		Length = *(Msg++);

		if (Type != 6)				// Object ID
			return;

		memcpy(OID, Msg, Length);
		OID[Length] = 0;

		OIDLen = Length;

		Len -=2;				// Header
		Len -= Length;
	
		Msg += Length;

		// Should Just have a null value left
		
		Type = *(Msg++);
		Length = *(Msg++);

		if (Type != 5 || Length != 0)
			return;

		Len -=2;				// Header

		// Should be nothing left
	}

	if (RequestType = 160)
	{
		UCHAR Reply[256];
		int Offset = 255;
		int PDULen, SendLen;
		char Value[256];
		int ValLen;
		
		//	Only Support Get

		if (memcmp(OID, sysName, sysNameLen) == 0)
		{
			ValLen = strlen(MYNODECALL);;
			Value[0] = 4;		// String
			Value[1] = ValLen;
			memcpy(&Value[2], MYNODECALL, ValLen);  
			
			PDULen = BuildReply(Reply, Offset, sysName, sysNameLen, Value, ReqID);
		}
		else if (memcmp(OID, sysUpTime, sysUpTimeLen) == 0)
		{
			int ValOffset = 10;
			ValLen = ASNPutInt(Value, ValOffset, (time(NULL) - TimeLoaded) * 100, TimeTicks);
			ValOffset -= ValLen;

			PDULen = BuildReply(Reply, Offset, sysUpTime, sysUpTimeLen, &Value[ValOffset], ReqID);
		}
		else if (memcmp(OID, ifOutOctets, ifOutOctetsLen) == 0)
		{
			int Port = OID[9];
			int ValOffset = 10;
			ValLen = ASNPutInt(Value, ValOffset, OutOctets[Port], Counter32);
			ValOffset -= ValLen;
			PDULen = BuildReply(Reply, Offset, OID, OIDLen, &Value[ValOffset], ReqID);

		}
		else if (memcmp(OID, ifInOctets, ifInOctetsLen) == 0)
		{
			int Port = OID[9];
			int ValOffset = 10;
			ValLen = ASNPutInt(Value, ValOffset, InOctets[Port], Counter32);
			ValOffset -= ValLen;
			PDULen = BuildReply(Reply, Offset, OID, OIDLen, &Value[ValOffset], ReqID);

		}
		else


			return;

		Offset -= PDULen;

		Offset -= ComLen;

		memcpy(&Reply[Offset], Community, ComLen);
		Reply[--Offset] = ComLen;
		Reply[--Offset] = 4;

		// Version

		Reply[--Offset] = 0;
		Reply[--Offset] = 1;
		Reply[--Offset] = 2;


		Reply[--Offset] = PDULen + ComLen + 5;
		Reply[--Offset] = 48;

		SendLen = PDULen + ComLen + 7;

		memcpy(UDPptr->UDPData, &Reply[Offset], SendLen);

		// Swap Dest to Origin

		IPptr->IPDEST = IPptr->IPSOURCE;

		IPptr->IPSOURCE = OurIPAddr;

		UDPptr->DESTPORT = UDPptr->SOURCEPORT;
		UDPptr->SOURCEPORT = htons(161);
		SendLen += 8;			// UDP Header
		UDPptr->LENGTH = htons(SendLen);
		IPptr->IPLENGTH = htons(SendLen + 20);

		CheckSumAndSendUDP(IPptr, UDPptr, SendLen);
	}

	// Ingnore others
}




