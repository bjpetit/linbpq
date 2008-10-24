
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

/*
TODo	?Time Out ARP
		?Multiple Adapters
*/

#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#define Dll	__declspec( dllexport )

#include "windows.h"
#include <stdio.h>
#include "AsmStrucs.h"
#include "IPCode.h"
#include "pcap.h"

//       ARP REQUEST (AX.25)

AXARP AXARPREQMSG = {0};

//		ARP REQUEST/REPLY (Eth)

ETHARP ETHARPREQMSG = {0};

ARPDATA ** ARPRecords = NULL;				// ARP Table - malloc'ed as needed

int NumberofARPEntries=0;

extern BOOL IPActive;
extern short NUMBEROFPORTS;

//HANDLE hBPQNET = INVALID_HANDLE_VALUE;

ULONG OurIPAddr = 0;
ULONG OurIPBroadcast = 0;

int FramesForwarded = 0;
int FramesDropped = 0;
int ARPTimeouts = 0;
int SecTimer = 10;

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

BOOL Init_IP()
{
	ARPDATA * ARPptr;

	ReadConfigFile("IPGateway.cfg");
	
	ARPRecords = NULL;				// ARP Table - malloc'ed as needed

	NumberofARPEntries=0;

    //
    // Open PCAP Driver
 
	OpenPCAP();

    if (adhandle == NULL)
	{
		WritetoConsoleLocal("Failed to open pcap device - IP Support Disabled\n");
		IPActive=FALSE;
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
	
	WritetoConsoleLocal("\nIP Support Enabled\n");

	IPActive=TRUE;

	return TRUE;
}

BOOL Poll_IP()
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
		PMESSAGE axmsg;
		PMESSAGE savemsg;

		axmsg = IPHOSTVECTOR.HOSTTRACEQ;

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

				len-= sizeof(MESSAGE);
				len--;						// Remove Frag Control Byte

				memcpy(&Buffer[EthOffset], ptr, len);

				nextfrag = &Buffer[EthOffset]+len;

				// Release Buffer
fragloop:
				savemsg->CHAIN = FREE_Q;
				FREE_Q = savemsg;
				QCOUNT++;

				if (IPHOSTVECTOR.HOSTTRACEQ == 0)	goto Pollloop;		// Shouldn't happen

				axmsg = IPHOSTVECTOR.HOSTTRACEQ;
				IPHOSTVECTOR.HOSTTRACEQ = axmsg->CHAIN;
				savemsg=axmsg;

				ptr = &axmsg->PID;
				ptr++;
				
				if (--frags != (*ptr++ & 0x7f))
					break;					// Out of sequence

				len = axmsg->LENGTH;

				len-= sizeof(MESSAGE);
				len--;						// Remove Frag Control Byte

				memcpy(nextfrag, ptr, len);

				nextfrag+=len;

				if (frags != 0) goto fragloop;

				ProcessIPMsg((PIPMSG)&Buffer[EthOffset+1],axmsg->ORIGIN, (axmsg->CTL == 3) ? 'D' : 'V', axmsg->PORT);

				break;
			}

		}
		// Release the buffer


		savemsg->CHAIN = FREE_Q;
		FREE_Q = savemsg;

		QCOUNT++;

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
	Block->ORIGIN[6] |= 1;						// Set End of Call
	Block->CTL = 3;		//UI

	_asm {

	pushfd
	cld
	pushad

	mov	al,Port
	mov	ah,10
	mov	ecx,Len
	mov	esi,Block
	add	esi,7

	call	RAWTX

	popad
	popfd
	}
	return;

}
VOID Send_AX_CONNECTED(VOID * Block, DWORD Len, UCHAR Port, UCHAR * HWADDR)
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
VOID SendNetFrame(UCHAR * ToCall, UCHAR * FromCall, UCHAR * Block, DWORD Len, UCHAR Port)
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

Dll VOID APIENTRY ProcessEthIPMsg(PETHMSG Buffer)
{
	PIPMSG ipptr = (PIPMSG)&Buffer[1];

	if (memcmp(Buffer, ourMACAddr,6 ) !=0 ) 
		return;		// Not for us

	if (memcmp(&Buffer[6], ourMACAddr,6 ) ==0 ) 
		return;		// Discard our sends


	ProcessIPMsg(ipptr, Buffer->SOURCE, 'E', 255);
}

Dll VOID APIENTRY ProcessEthARPMsg(PETHARP arpptr)
{
	int i=0, Mask=IPPortMask;
	PARPDATA Arp;
	BOOL Found;

	if (memcmp(&arpptr->MSGHDDR.SOURCE, ourMACAddr,6 ) == 0 ) 
		return;		// Discard out sends

	switch (arpptr->ARPOPCODE)
	{
	case 0x0100:
	
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
		Arp->ARPTIMER =  86400;
	
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
		Arp->ARPTIMER =  86400;

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
			Arp->ARPTIMER =  86400;
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
		Arp->ARPTIMER =  86400;

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

	if (IPptr->VERLEN != 0x45) return; //Only support Type = 4, Len = 20

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
				Arp->HWADDR[6] &= 0x7f;
			}
			Arp->ARPTYPE = Type;
			Arp->ARPINTERFACE = Port;
			Arp->ARPVALID = TRUE;
			Arp->ARPTIMER =  86400;
		}
	}

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
	PMESSAGE Msgptr = (PMESSAGE)IPptr;
	int Len;

	(UCHAR *)Msgptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=16;
	Msgptr->PID = 0xcc;		//IP

	if (Mode == 'D')		// Datagram
	{
		Send_AX_Datagram(Msgptr, Len, Port, HWADDR);
		return;
	}

	Send_AX_CONNECTED(Msgptr, Len, Port, HWADDR);


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


BOOL ReadConfigFile(char * fn)
{

// IPAddr 192.168.0.129
// IPBroadcast 192.168.0.255
// IPGateway 192.168.0.1
// IPPorts 1,4 

	FILE *file;
	char buf[256],errbuf[256];

	UCHAR Value[MAX_PATH];

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
		wsprintf(buf,"Config file %s could not be opened ",Value);
		WritetoConsoleLocal(buf);

		return (FALSE);
	}


	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf))
		{
			WritetoConsoleLocal("IP Gateway bad config record ");
			WritetoConsoleLocal(errbuf);
		}
				
	}
	
	fclose(file);

	return (TRUE);
}


ProcessLine(char * buf)
{
	PCHAR ptr, p_value, p_port;
	int i;

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
							 1,				// promiscuous mode (nonzero means promiscuous)
							 1,				// read timeout
							 errbuf			// error buffer
							 );
	
	if (adhandle == NULL)
	{
		n=wsprintf(buf,"Unable to open %s\n",Adapter);
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	/* Check the link layer. We support only Ethernet for simplicity. */
	if(pcap_datalinkx(adhandle) != DLT_EN10MB)
	{
		n=wsprintf(buf,"\nThis program works only on Ethernet networks.\n");
		WritetoConsoleLocal(buf);
		
		/* Free the device list */
		return -1;
	}

	netmask=0xffffff; 

	sprintf(packet_filter,"ether[12:2]=0x0800 or ether[12:2]=0x0806");

	//compile the filter
	if (pcap_compilex(adhandle, &fcode, packet_filter, 1, netmask) <0 )
	{	
		n=wsprintf(buf,"\nUnable to compile the packet filter. Check the syntax.\n");
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	//set the filter

	if (pcap_setfilterx(adhandle, &fcode)<0)
	{
		n=wsprintf(buf,"\nError setting the filter.\n");
		WritetoConsoleLocal(buf);

		/* Free the device list */
		return -1;
	}
	
	n=wsprintf(buf,"Using %s", Adapter);
	WritetoConsoleLocal(buf);

	return TRUE;

}


