
// Module to provide a basic Gateway between IP over AX.25 and the Internet.

// Uses WinPcap

// Basically operates as a mac level bridge, with headers converted between ax.25 and Ethernet.
// ARP frames are also reformatted, and monitored to build a simple routing table.
// Apps may not use ARP (MSYS is configured with an ax.25 default route, rather than an IP one),
// so the default route must be configured. 

// Intended as a gateway for legacy apps, rather than a full function ip over ax.25 router.
// Suggested config is to use the Internet Ethernet Adapter, behind a NAT/PAT Router.
// The ax.25 applications will appear as single addresses on the Ethernet LAN

// First Version, July 2008

// Version 1.2.1 January 2009

//	Add IP Address Mapping option

/*
TODo	?Time Out ARP
		?Multiple Adapters
*/

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE 

#define Dll	__declspec( dllexport )

#include "windows.h"
#include <stdio.h>
#include "AsmStrucs.h"
#include "bpq32.h"
#include "IPCode.h"
#include "pcap.h"

static char MYCALL[7];	// 7 chars, ax.25 format

extern struct BPQVECSTRUC IPHOSTVECTOR;

BOOL APIENTRY  GETSENDNETFRAMEADDR();
BOOL APIENTRY  Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port);

BOOL ProcessConfig();
BOOL FreeConfig();

#define ARPTIMEOUT 86400

//       ARP REQUEST (AX.25)

AXARP AXARPREQMSG = {0};

//		ARP REQUEST/REPLY (Eth)

ETHARP ETHARPREQMSG = {0};

ARPDATA ** ARPRecords = NULL;				// ARP Table - malloc'ed as needed

int NumberofARPEntries=0;
short NUMBEROFPORTS;

//HANDLE hBPQNET = INVALID_HANDLE_VALUE;

ULONG OurIPAddr = 0;
ULONG OurIPBroadcast = 0;

int FramesForwarded = 0;
int FramesDropped = 0;
int ARPTimeouts = 0;
int SecTimer = 10;

BOOL NeedResolver = FALSE;

int map_table_len = 0;
//int index=0;					// pointer for table search
int ResolveIndex=-1;			// pointer to entry being resolved

struct map_table_entry map_table[MAX_ENTRIES];

struct tagMSG Msg;

char buf[MAXGETHOSTSTRUCT];

LOGFONT LFTTYFONT ;

HFONT hFont ;

time_t ltime,lasttime;

char ConfigClassName[]="CONFIG";

static HWND hResWnd;

extern BOOL StartMinimized;
extern BOOL MinimizetoTray;
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

UCHAR Buffer[1630] = {0};

#define EthOffset 30				// Should be plenty

DWORD IPLen = 0;

UCHAR QST[7]={'Q'+'Q','S'+'S','T'+'T',0x40,0x40,0x40,0xe0};		//QST IN AX25
UCHAR ourMACAddr[6] = {02,'B','P','Q',0,1};

ULONG DefaultIPAddr = 0;

int IPPortMask = 0;

IPSTATS IPStats = {0};

UCHAR BPQDirectory[];

char ARPFN[MAX_PATH];
char CFGFN[MAX_PATH];

HANDLE handle;

pcap_t *adhandle;

char Adapter[256];

HINSTANCE PcapDriver=0;

typedef int (FAR *FARPROCX)();

int (FAR * pcap_sendpacketx)();
pcap_t * (FAR * pcap_open_livex)(const char *, int, int, int, char *);
FARPROCX pcap_compilex;
FARPROCX pcap_setfilterx;
FARPROCX pcap_datalinkx;
FARPROCX pcap_next_exx;
FARPROCX pcap_geterrx;

char Dllname[6]="wpcap";

FARPROCX GetAddress(char * Proc);

HANDLE hInstance;


Dll BOOL APIENTRY Init_IP()
{
	ARPDATA * ARPptr;

	NUMBEROFPORTS=GetNumberofPorts();
	ConvToAX25(GetNodeCall(), MYCALL);

	if (BPQDirectory[0] == 0)
	{
		strcpy(CFGFN,"IPGateway.cfg");
		strcpy(ARPFN,"BPQARP.dat");
	}
	else
	{
		strcpy(CFGFN,BPQDirectory);
		strcat(CFGFN,"\\");
		strcat(CFGFN,"IPGateway.cfg");

		strcpy(ARPFN,BPQDirectory);
		strcat(ARPFN,"\\");
		strcat(ARPFN,"BPQARP.dat");
	}
	
	ReadConfigFile();
	
	ARPRecords = NULL;				// ARP Table - malloc'ed as needed

	NumberofARPEntries=0;

    //
    // Open PCAP Driver
 
	OpenPCAP();

    if (adhandle == NULL)
	{
		WritetoConsole("Failed to open pcap device - IP Support Disabled\n");
		return FALSE;
	} 

	// Clear old packets

	IPHOSTVECTOR.HOSTAPPLFLAGS = 0x80;			// Request IP frams from Node

	// Set up static fields in ARP messages

	AXARPREQMSG.HWTYPE=0x0300;				//	AX25
	memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
	memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
	AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;			// Set End of Call
	AXARPREQMSG.MSGHDDR.PID = 0xcd;			// ARP
	AXARPREQMSG.MSGHDDR.CTL = 03;				// UI

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

	// Allocate ARP Entry for Default Gateway, and send ARP for it

	ARPptr = AllocARPEntry();

	if (ARPptr != NULL)
	{
		ARPptr->ARPINTERFACE = 255;
		ARPptr->ARPTYPE = 'E';
		ARPptr->IPADDR = DefaultIPAddr;

		SendARPMsg(ARPptr);
	}

	ReadARP();

	if (NeedResolver)
		_beginthread(IPResolveNames, 0, NULL );

	WritetoConsole("\nIP Support Enabled\n");

	return TRUE;
}

VOID IPClose()
{
	if (MinimizetoTray)
		DeleteTrayMenuItem(hResWnd);

	PostMessage(hResWnd, WM_DESTROY,0,0);
	DestroyWindow(hResWnd);

	hResWnd= NULL;
}

Dll BOOL APIENTRY Poll_IP()
{
	int res;
	struct pcap_pkthdr *header;
	u_char *pkt_data;

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

	res = pcap_next_exx(adhandle, &header, &pkt_data);

	if (res > 0)
	{
		PETHMSG ethptr = (PETHMSG)&Buffer[EthOffset];

		memcpy(&Buffer[EthOffset],pkt_data, header->len);

		if (ethptr->ETYPE == 0x0008)
		{
			ProcessEthIPMsg(&Buffer[EthOffset]);
		//	PIPMSG ipptr = (PIPMSG)&Buffer[EthOffset+14];
		//	ProcessIPMsg(ipptr, ethptr->SOURCE, 'E', 255);
			goto Pollloop;
		}

		if (ethptr->ETYPE == 0x0608)
		{
			ProcessEthARPMsg((PETHARP)ethptr);
			goto Pollloop;
		}

		goto Pollloop;

		// Ignore anything else
	}

	if (IPHOSTVECTOR.HOSTTRACEQ != 0)
	{
		PBUFFHEADER axmsg;
		PBUFFHEADER savemsg;

		axmsg = (PBUFFHEADER)IPHOSTVECTOR.HOSTTRACEQ;

		IPHOSTVECTOR.HOSTTRACEQ = axmsg->CHAIN;

		savemsg=axmsg;

		// Packet from AX.25

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
				RelBuff(savemsg);

				if (IPHOSTVECTOR.HOSTTRACEQ == 0)	goto Pollloop;		// Shouldn't happen

				axmsg = (PBUFFHEADER)IPHOSTVECTOR.HOSTTRACEQ;
				IPHOSTVECTOR.HOSTTRACEQ = axmsg->CHAIN;
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

		RelBuff(savemsg);

		goto Pollloop;

	}
	return TRUE;
}
  
BOOL Send_ETH(VOID * Block, DWORD len)
{ 
	if (len < 60) len = 60;

	// Send down the packet 

	pcap_sendpacketx(adhandle,	// Adapter
			Block,				// buffer with the packet
			len);				// size
			
    return TRUE;

}

#define AX25_P_SEGMENT  0x08
#define SEG_REM         0x7F
#define SEG_FIRST       0x80

VOID Send_AX_Datagram(PMESSAGE Block, DWORD Len, UCHAR Port, UCHAR * HWADDR)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(Block->DEST, HWADDR, 7);
	memcpy(Block->ORIGIN, MYCALL, 7);
	Block->DEST[6] &= 0x7e;						// Clear End of Call
	Block->ORIGIN[6] |= 1;						// Set End of Call
	Block->CTL = 3;		//UI

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
static VOID SendNetFrame(UCHAR * ToCall, UCHAR * FromCall, UCHAR * Block, DWORD Len, UCHAR Port)
{
	memcpy(&Block[7],ToCall, 7);
	memcpy(&Block[14],FromCall, 7);

	_asm {

		pushfd
		cld
		pushad

		mov	ecx,Len
		mov	esi,Block
		add	esi,7

		mov	dl,Port
		call	SENDNETFRAME

		popad
		popfd
		}
		
	return;
}

VOID ProcessEthIPMsg(PETHMSG Buffer)
{
	PIPMSG ipptr = (PIPMSG)&Buffer[1];

	if (memcmp(Buffer, ourMACAddr,6 ) !=0 ) 
		return;		// Not for us

	if (memcmp(&Buffer[6], ourMACAddr,6 ) ==0 ) 
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

		if (Arp == NULL) return;					// No point of table full
				
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

		// If already resolved, and on Enet, Ignore or we send loads of unnecessary msgs to ax.25

				
		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);

		if (Found)
			if (Arp->ARPVALID && (Arp->ARPTYPE == 'E')) return;

		// Send to all other ports enabled for IP, reformatting as necessary

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
				ETHARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST
				ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
					
				memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);
				memcpy(ETHARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 6);

				Send_ETH(&ETHARPREQMSG, 42);
				return;
			}
			else
			{
				AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				AXARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST
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

		// Send to all other ports enabled for IP, reformatting as necessary
	
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
				ETHARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST

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
				AXARPREQMSG.ARPOPCODE = 0x0200;		//             ; Reply

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

VOID ProcessIPMsg(PIPMSG IPptr, UCHAR * MACADDR, CHAR Type, UCHAR Port)
{
	ULONG Dest;
	PARPDATA Arp;
	BOOL Found;
	int index, Len;
	PTCPMSG TCPptr;

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

	if (Dest == OurIPAddr || Dest == 0xffffffff || Dest == OurIPBroadcast)
		goto ForUs;


	RouteIPMsg(IPptr);

	return;

ForUs:

	if (IPptr->IPPROTOCOL == 1)		// ICMP
	{
		ProcessICMPMsg(IPptr);
		return;
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



BOOL CheckIPChecksum(PIPMSG IPptr)
{
	USHORT checksum=0;

	if (IPptr->IPCHECKSUM == 0)
		return TRUE; //Not used

	_asm{

	mov ESI, IPptr
	MOV ECX,10
	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	mov	checksum,dx

	}

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
BOOL Check_Checksum(VOID * ptr1, int Len)
{
	USHORT checksum;

	_asm{

	mov ESI, ptr1
	MOV ECX, Len
	inc	ECX
	SHR	ECX,1				; Round up and convert to words

	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	mov	checksum,dx

	}

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
USHORT Generate_CHECKSUM(VOID * ptr1, int Len)
{
	USHORT checksum=0;

	_asm{

	mov ESI, ptr1
	MOV ECX, Len
	inc	ECX
	SHR	ECX,1				; Round up and convert to words
	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	NOT		DX
	mov	checksum,dx
	}

	return checksum ;
}

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

PARPDATA AllocARPEntry()
{
	ARPDATA * ARPptr;

	if (NumberofARPEntries == 0)

		ARPRecords=malloc(4);
	else
		ARPRecords=realloc(ARPRecords,(NumberofARPEntries+1)*4);

	ARPptr=malloc(sizeof(ARPDATA));

	if (ARPptr == NULL) return NULL;

	memset(ARPptr, 0, sizeof(ARPDATA));
	
	ARPRecords[NumberofARPEntries++]=ARPptr;
 
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

	for (i=0; i < NumberofARPEntries; i++)
	{
		if (Arp == ARPRecords[i])
		{
			if (i == 0)
			{
				// Dont remove first - it is the Default Gateway. Se to re-resolve

				Arp->ARPVALID = FALSE;
				Arp->ARPTIMER = 5;
				return;
			}

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

	_asm {
		
		mov esi, ARPRecs
		mov DWORD PTR[ESI], offset Arp
	
		mov esi, IPStatsParam
		mov DWORD PTR[ESI], offset IPStats 
	}

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

	FILE *file;
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
				WritetoConsole("IP Gateway bad config record ");
				WritetoConsole(errbuf);
			}
		}
	}
	else
	{
		if ((file = fopen(CFGFN,"r")) == NULL)
		{
			wsprintf(buf,"Config file %s could not be opened ",CFGFN);
			WritetoConsole(buf);

			return (FALSE);
		}

		while(fgets(buf, 255, file) != NULL)
		{
			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf))
			{
				WritetoConsole("IP Gateway bad config record ");
				WritetoConsole(errbuf);
			}			
		}
		fclose(file);
	}
	return (TRUE);
}


static ProcessLine(char * buf)
{
	PCHAR ptr, p_value, p_origport, p_host, p_port;
	int i, port, mappedport, ipad;

	ptr = strtok(buf, " \t\n\r");
	p_value = strtok(NULL, " \t\n\r");


	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if(_stricmp(ptr,"ADAPTER") == 0)
	{
		strcpy(Adapter,p_value);
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

	if (_stricmp(ptr,"MAP") == 0)
	{
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

		Arp->ARPTIMER--;
			
		if (Arp->ARPTIMER == 0)
		{
			// Remove Entry
				RemoveARP(Arp);
		}
	}
}

// PCAP Support Code



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

		n=wsprintf(buf,"Error finding %s - %d", Proc,err);
		WritetoConsole(buf);
	
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

	WritetoConsole("IP ");

	/* Open the adapter */
	adhandle= pcap_open_livex(Adapter,	// name of the device
							 65536,			// portion of the packet to capture. 
											// 65536 grants that the whole packet will be captured on all the MACs.
							 1,				// promiscuous mode (nonzero means promiscuous)
							 1,				// read timeout
							 errbuf			// error buffer
							 );
	
	if (adhandle == NULL)
	{
		n=wsprintf(buf,"Unable to open %s\n",Adapter);
		WritetoConsole(buf);

		/* Free the device list */
		return -1;
	}
	
	/* Check the link layer. We support only Ethernet for simplicity. */
	if(pcap_datalinkx(adhandle) != DLT_EN10MB)
	{
		n=wsprintf(buf,"\nThis program works only on Ethernet networks.\n");
		WritetoConsole(buf);
		
		/* Free the device list */
		return -1;
	}

	netmask=0xffffff; 

	sprintf(packet_filter,"ether[12:2]=0x0800 or ether[12:2]=0x0806");

	//compile the filter
	if (pcap_compilex(adhandle, &fcode, packet_filter, 1, netmask) <0 )
	{	
		n=wsprintf(buf,"\nUnable to compile the packet filter. Check the syntax.\n");
		WritetoConsole(buf);

		/* Free the device list */
		return -1;
	}
	
	//set the filter

	if (pcap_setfilterx(adhandle, &fcode)<0)
	{
		n=wsprintf(buf,"\nError setting the filter.\n");
		WritetoConsole(buf);

		/* Free the device list */
		return -1;
	}
	
	n=wsprintf(buf,"Using %s", Adapter);
	WritetoConsole(buf);

	return TRUE;

}
VOID ReadARP()
{
	FILE *file;
	char buf[256],errbuf[256];
	
	if ((file = fopen(ARPFN,"r")) == NULL) return;
	
	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessARPLine(buf))
		{
			WritetoConsole("IP Gateway bad ARP record ");
			WritetoConsole(errbuf);
		}
				
	}
	
	fclose(file);

	return;
}

BOOL ProcessARPLine(char * buf)
{
	PCHAR p_ip, p_mac, p_port, p_type;
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
		}
	}
	return TRUE;
}



VOID SaveARP ()
{
	PARPDATA Arp;
	int i;

	handle = CreateFile(ARPFN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	for (i=0; i < NumberofARPEntries; i++)
	{
		Arp = ARPRecords[i];
		if (Arp->ARPVALID) WriteARPLine(Arp);
	}

 	CloseHandle(handle);
	
	return ;
}

VOID WriteARPLine(PARPDATA ARPRecord)
{
	int SSID, Len, Cnt, j;
	char Mac[20];
	char Call[7];
	char IP[20];
	char Line[100];

	struct in_addr Addr;

	Addr.s_addr = ARPRecord->IPADDR;

	wsprintf(IP, "%d.%d.%d.%d", 
		Addr.S_un.S_un_b.s_b1, Addr.S_un.S_un_b.s_b2, Addr.S_un.S_un_b.s_b3, Addr.S_un.S_un_b.s_b4); 

	if(ARPRecord->ARPINTERFACE == 255)		// Ethernet
	{
			wsprintf(Mac," %02x:%02x:%02x:%02x:%02x:%02x", 
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
			
			wsprintf(Mac,"%s-%d", Call, SSID);
	}

	Len = wsprintf(Line,"%s %s %d %c\n",
			IP, Mac, ARPRecord->ARPINTERFACE, ARPRecord->ARPTYPE);
		
	WriteFile(handle,Line,Len,&Cnt,NULL);

	return;
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
	int index,displayline;

	int i=1;

	int nScrollCode,nPos;

	switch (message) { 

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


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!



		if (wmId == BPQREREAD)
		{
			ProcessConfig();
			FreeConfig();

			ReadConfigFile("IPGateway.CFG");
			PostMessage(hResWnd, WM_TIMER,0,0);
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
				wsprintf(hostaddr,"Error %d",map_table[index].error);
			}
			else
			{
				memcpy(&ipad,&map_table[index].mappedipaddr,4);
				strncpy(hostaddr,inet_ntoa(ipad),16);
			}
				
			memcpy(&ipad,&map_table[index].mappedipaddr,4);
								
			i=wsprintf(line,"%.64s = %-.30s",
				map_table[index].hostname,
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
			
		for (ResolveIndex=0; ResolveIndex < map_table_len; ResolveIndex++)
		{	
			WSAAsyncGetHostByName (hWnd,WM_USER+199,
						map_table[ResolveIndex].hostname,
						buf,MAXGETHOSTSTRUCT);
			break;	
		}

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

void IPResolveNames( void *dummy )
{
    int Windowlength, WindowParam;
	WNDCLASS  wc;
	int i;
	char WindowTitle[100];
	HMENU hMenu,hPopMenu;

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
 
	if (map_table_len > 10 )
	{
		Windowlength=10*14+100;
		WindowParam=WS_OVERLAPPEDWINDOW | WS_VSCROLL;
	}
	else
	{
		Windowlength=(map_table_len)*14+100;
		WindowParam=WS_OVERLAPPEDWINDOW ;
	}

	sprintf(WindowTitle,"IP Gateway Resolver");

	hResWnd = CreateWindow("IPAppName",WindowTitle,
		WindowParam,
		CW_USEDEFAULT, 0, 500, Windowlength,
		NULL, NULL, hInstance, NULL);

		i=GetLastError();

	
	hMenu=CreateMenu();
	hPopMenu=CreatePopupMenu();
	SetMenu(hResWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu,"Update");

	AppendMenu(hPopMenu,MF_STRING,BPQREREAD,"ReRead Config");
//	AppendMenu(hPopMenu,MF_STRING,BPQADDARP,"Add Entry");

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

  	if (map_table_len > 10 )
		SetScrollRange(hResWnd,SB_VERT,0,map_table_len-10,TRUE);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
		AddTrayMenuItem(hResWnd, "IP Gateway Resolver");
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
	USHORT PHSUM;
		
	IPptr->IPCHECKSUM = 0;


	_asm{

	mov	esi,TCPmsg
	movzx eax,Len
	add esi,eax
	mov byte ptr[esi],0

	mov	esi,IPptr

	MOV	DX,600H			; PROTOCOL (REVERSED)
	MOV	AX,Len		; TCP LENGTH
	XCHG	AH,AL
	ADD	DX,AX
	MOV	AX,12[esi]
	ADC	DX,AX
	MOV	AX,14[esi]
	ADC	DX,AX
	MOV	AX,16[esi]
	ADC	DX,AX
	MOV	AX,18[esi]
	ADC	DX,AX
	ADC	DX,0

	MOV	PHSUM,DX

	}

	TCPmsg->CHECKSUM = PHSUM;

	TCPmsg->CHECKSUM = Generate_CHECKSUM(TCPmsg, Len);

	// CHECKSUM IT

	IPptr->IPCHECKSUM = Generate_CHECKSUM(IPptr, 20);
	RouteIPMsg(IPptr);
	return 0;
}