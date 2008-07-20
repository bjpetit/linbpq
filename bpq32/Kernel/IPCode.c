
// Module to provide a basic Gateway from IP over AX.25 to Internet.

// Uses BPQNET Virtual Network Interface 

// First Version, July 2008


#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#include "windows.h"

#include "time.h"
#include "stdio.h"
#include "io.h"
#include <fcntl.h>					 
//#include "vmm.h"
#include "SHELLAPI.H"

#include "AsmStrucs.h"


extern short NUMBEROFPORTS;
extern short QCOUNT;
extern PMESSAGE FREE_Q;

extern struct PORTCONTROL * PORTTABLE;

extern char MYCALL[];	// 7 chars, ax.25 format
extern char MYNODECALL;	// 10 chars,not null terminated
extern char MYALIAS;		// 6 chars, not null terminated
extern struct APPLCALLS  APPLCALLTABLE[8];
extern char SIGNONMSG;

extern struct BPQVECSTRUC BPQHOSTVECTOR[65];
extern struct BPQVECSTRUC IPHOSTVECTOR;

extern char * CONFIGFILENAME;

extern int BPQHOSTAPI();
extern int INITIALISEPORTS();
extern int TIMERINTERRUPT();
extern int MONDECODE();
extern int BPQMONOPTIONS();
extern int RAWTX();

extern int WritetoConsoleLocal(char * buff);

VOID SetupBPQDirectory();

unsigned long _beginthread( void( *start_address )( int ), unsigned stack_size, int arglist);

extern int VECTORLENGTH;

#define IOCTL_NETVMINI_READ_DATA \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_NETVMINI_WRITE_DATA \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_NETVMINI_GETMACADDR \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_READ_ACCESS)


#pragma pack(1) 

typedef struct _ETHMSG
{
	UCHAR	DEST[6];
	UCHAR	SOURCE[6];
	USHORT	ETYPE;

} ETHMSG, *PETHMSG;


typedef struct _IPMSG
{
//       FORMAT OF IP HEADER
//
//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
//

	UCHAR	VERLEN;          // 4 BITS VERSION, 4 BITS LENGTH
	UCHAR	TOS;             // TYPE OF SERVICE
	USHORT	IPLENGTH;        // DATAGRAM LENGTH
	USHORT	IPID;            // IDENTIFICATION
	USHORT	FRAGWORD;        // 3 BITS FLAGS, 13 BITS OFFSET
	UCHAR	IPTTL;
	UCHAR	IPPROTOCOL;      // HIGHER LEVEL PROTOCOL
	USHORT	IPCHECKSUM;      // HEADER CHECKSUM
	ULONG	IPSOURCE;
	ULONG	IPDEST;

	UCHAR	Data;

} IPMSG, *PIPMSG;

/*
;       FORMAT OF TCP HEADER WITHIN AN IP DATAGRAM
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

TCPSTRUC        STRUC

TCPSOURCEPORT   DW      0
TCPDESTPORT     DW      0

SEQNUM          DD      0
ACKNUM          DD      0

TCPCONTROL      DB      0       ; 4 BITS DATA OFFSET 4 RESERVED
TCPFLAGS        DB      0       ; (2 RESERVED) URG ACK PSH RST SYN FIN

TCPWINDOW       DW      0
CHECKSUM        DW      0
URGPTR          DW      0
;
;       OPTIONS AND/OR DATA MAY FOLLOW
;
TCPOPTIONS      DB      4 DUP (0)

TCPSTRUC        ENDS
;
;       TCPFLAGS BITS
;
FIN     EQU     1B
SYN     EQU     10B
RST     EQU     100B
PSH     EQU     1000B
ACK     EQU     10000B
URG     EQU     100000B

*/

//		ICMP MESSAGE STRUCTURE

typedef struct _ICMPMSG
{

//       FORMAT OF ICMP HEADER WITHIN AN IP DATAGRAM

//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	UCHAR	ICMPTYPE;
	UCHAR	ICMPCODE;
	USHORT	ICMPCHECKSUM;

	USHORT	ICMPID;
	USHORT	ICMPSEQUENCE;

} ICMPMSG, *PICMPMSG;



/*
;
;       UDP MESSAGE STRUCTURE
;
UDPMSGS         STRUC
;
;       FORMAT OF UDP HEADER WITHIN AN IP DATAGRAM
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

SOURCEPORT      DW      0
DESTPORT        DW      0
UDPLENGTH       DW      0
UDPCHECKSUM     DW      0
;
;       DATA FOLLOWS
;
UDPDATA         DB      0

UDPMSGS         ENDS
;
*/

typedef struct _AXARP
{
	USHORT	HWTYPE;      //    DB      0,3             ; AX.25
	USHORT	PID;			//	DB      0,0CCH          ; PID

	UCHAR	HWADDRLEN;       //      7
	UCHAR	IPADDRLEN;      //      4

	USHORT	ARPOPCODE;       //      200H            ; REQUEST/REPLY

	UCHAR	SENDHWADDR[7];
	UCHAR	SENDIPADDR[4];

	UCHAR	TARGETHWADDR[7];
	UCHAR	TARGETIPADDR[4];

} AXAPR, *PAXARP;
/*

;       ARP REQUEST (AX.25)
;
ARPHEADER       DW      0
		DW      46
		DW      OFFSET ARPREQUESTMSG

ARPREQUESTMSG   LABEL   BYTE

QSTCALL         DB      'Q'+'Q','S'+'S','T'+'T',40h,40h,40h,11100000B; QST IN AX25
MYADDR1         DB      7 DUP (0)       ; AX25 SOURCE   
		DB      03H             ; UI

		DB      0CDH            ; TYPE
		DB      0,3
		DB      0,0CCH
		DB      7,4             ; LENGTHS
		DB      0,1             ; REQUEST
MYADDR2         DB      7 DUP (0)
MYIPADDR        DD      0
		DB      7 DUP (0)       ; ADDRESS WE WANT
ARPIPTARGET     DB      44,131,4,18     ; 4 DUP (0)

;
BROADCAST	DB	6 DUP (0FFH)	; MAC BROADCAST ADDR
*/

typedef struct _ETHARP
{
	USHORT	HWTYPE;      //    DB      0,1             ; Eth
	USHORT	PID;			//	DB      8,0          ; PID

	UCHAR	HWADDRLEN;       //      7
	UCHAR	IPADDRLEN;      //      4

	USHORT	ARPOPCODE;       //      200H            ; REQUEST/REPLY

	UCHAR	SENDHWADDR[6];
	ULONG	SENDIPADDR;

	UCHAR	TARGETHWADDR[6];
	ULONG	TARGETIPADDR;

} ETHARP, *PETHARP;

typedef struct _ROUTEENTRY
{

//       INITIALLY A STATIC STRUCTURE, BUILT FROM THE CONFIGURATION FILE

	ULONG	NETWORK;	// NETWORK 
	ULONG	SUBNET;		// SUBNET MASK
	ULONG	GATEWAY;	// GATEWAY IP ADDRESS
	UCHAR	PORT;		// POINTER TO PORT ENTRY
	UCHAR	RTYPE;		// TYPE (NETROM/VC/DG/ETH)
	UCHAR	METRIC;		// FOR RIP 
	UCHAR	ROUTEINFO;  // TYPÅ (LEARNED¬ SYSOP¬ ETC)
	UCHAR	ROUTECHANGED;// CHANGED RECENTLY FLAG
	UCHAR	RIPTIMOUT;  // FOR REMOVING ACTIVE ENTRIES
	UCHAR	GARTIMOUT;  // FOR REMOVING DEAD ENTRIES
	int		FRAMECOUNT; // FRAMES SENT TO THIS NETWORK
	UCHAR	Target[7];	// MAC Address - either enet or ax.25 (may use arp later)


} ROUTEENTRY, *PROUTEENTRY;

ROUTEENTRY ROUTETABLE[10];


int NumberofRoutes=0;

#pragma pack()


extern BOOL IPActive;

HANDLE hBPQNET =INVALID_HANDLE_VALUE;

BOOL Init_IP();
BOOL Poll_IP();  
BOOL Send_IP(VOID * Block, DWORD len);
VOID ProcessEthARPMsg(PETHMSG ethptr, PETHARP arpptr);
VOID ProcessIPMsg(PIPMSG IPptr);
BOOL CheckIPChecksum(PIPMSG IPptr);
VOID ProcessICMPMsg(PIPMSG IPptr);
VOID RouteIPMsg(PIPMSG IPptr);
VOID SendIPtoBPQDEV(PIPMSG IPptr,PROUTEENTRY Route);
VOID SendIPtoAX25(PIPMSG IPptr, PROUTEENTRY Route);

ULONG OurIPAddr;
ULONG OurIPBroadcast;


// Following Buffer is used for msgs from BPQDEV Driver. Put the Enet message part way down the buffer, 
//	so there is room for ax.25 header instead of Enet header when we route the frame to ax.25
//	Enet Header ia 14 bytes, AX.25 UI is 16

#define EthOffset 10				// Should be plenty

UCHAR Buffer[1600]={0};
DWORD IPLen=0;
UCHAR Mac[12];

UCHAR ourMACAddr[6] = {0,'B','P','Q',0,1};

UCHAR AXTarg[7]= {'G'+'G','M'+'M','8'+'8','B'+'B','P'+'P','Q'+'Q',0xfe}; // GM8BPQ-15 IN AX25
UCHAR EthTarg1[6] = {0x02,0x50,0xf2,0x00,0x00,0x01};
UCHAR EthTarg2[6] = {0x00,0xc,0xf6,0x21,0x5b,0x69};

BOOL Init_IP()
{
	PROUTEENTRY Route;

    //
    // Open handle to the control device, if it's not already opened. Please
    // note that even a non-admin user can open handle to the device with 
    // FILE_READ_ATTRIBUTES | SYNCHRONIZE DesiredAccess and send IOCTLs if the 
    // IOCTL is defined with FILE_ANY_ACCESS. So for better security avoid 
    // specifying FILE_ANY_ACCESS in your IOCTL defintions. 
    // If you open the device with GENERIC_READ, you can send IOCTL with 
    // FILE_READ_DATA access rights. If you open the device with GENERIC_WRITE, 
    // you can sedn ioctl with FILE_WRITE_DATA access rights.
    // 
 
	hBPQNET = CreateFile ( 
            TEXT("\\\\.\\BPQ32NET"),
            GENERIC_READ | GENERIC_WRITE,//FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_SHARE_READ,
            NULL, // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING, // No special create flags
            FILE_ATTRIBUTE_NORMAL, // No special attributes
            NULL);

    if (INVALID_HANDLE_VALUE == hBPQNET)
	{
		WritetoConsoleLocal("Failed to open BPQNET device - IP Support Disabled\n");
		IPActive=FALSE;
		return FALSE;
	} 


	DeviceIoControl (hBPQNET, IOCTL_NETVMINI_GETMACADDR,NULL, 0, &Mac, 12, &IPLen, NULL);

	IPHOSTVECTOR.HOSTAPPLFLAGS = 0x80;

	// Set up config - Need to read from file

	OurIPAddr =inet_addr("192.168.0.129");
	OurIPBroadcast = inet_addr("192.168.0.255");

	// Set up routes - Initially a host route to MSYS and a default to BPQNET

	Route=&ROUTETABLE[0];

	Route->NETWORK = inet_addr("192.168.0.130"); 
	Route->SUBNET = inet_addr("255.255.255.254");		// Host Route
	Route->PORT = 4;				// BPQ Radio Port 4
	memcpy(Route->Target,AXTarg, 7);


	Route=&ROUTETABLE[1];

	Route->NETWORK = inet_addr("192.168.0.101"); 
	Route->SUBNET = inet_addr("255.255.255.255");		// Host Route
	Route->PORT=0xff;				// BPQDEV Driver
	memcpy(Route->Target,EthTarg1, 6);

	Route=&ROUTETABLE[2];

	Route->PORT=0xff;				// BPQDEV Driver
	memcpy(Route->Target,EthTarg2, 6);

	NumberofRoutes=3;

	WritetoConsoleLocal("IP Support Enabled\n");

	IPActive=TRUE;

	return TRUE;
}

BOOL Poll_IP()
{
	DeviceIoControl (hBPQNET,
          IOCTL_NETVMINI_READ_DATA,
          NULL, 0,
          &Buffer[EthOffset], 1600,				// Leave plenty of space on front
          &IPLen, NULL) ;

	if (IPLen > 0)
	{
		PETHMSG ethptr = (PETHMSG)&Buffer[EthOffset];

		if (ethptr->ETYPE == 0x0608)
		{
			PETHARP arpptr = (PETHARP)&Buffer[EthOffset+14];

			ProcessEthARPMsg(ethptr, arpptr);
		}
		else
		{
			if (ethptr->ETYPE == 0x0008)
			{
				PIPMSG ipptr = (PIPMSG)&Buffer[EthOffset+14];

				ProcessIPMsg(ipptr);
				return TRUE;
			}
		}

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

		if (axmsg->PID == 0xcc)
		{
			// IP Message

				PIPMSG ipptr = (PIPMSG)++axmsg;

				ProcessIPMsg(ipptr);

				// Release the buffer

				savemsg->CHAIN = FREE_Q;
				FREE_Q = savemsg;

				QCOUNT++;

				return TRUE;
		}


		return TRUE;
	}
	return TRUE;
}
  
BOOL Send_IP(VOID * Block, DWORD len)
{
    DWORD bytes;
	
	DeviceIoControl(hBPQNET,
		IOCTL_NETVMINI_WRITE_DATA,
		Block, len,
        NULL, 0,
        &bytes, NULL);
    
    return TRUE;
}

VOID ProcessEthARPMsg(PETHMSG ethptr, PETHARP arpptr)
{
	int i=0;
	ULONG temp1, temp2;

	if (arpptr->ARPOPCODE == 0x0100)
	{
		// Request. Accept Proxy ARPs for anything in our routing table (except default route on end)

		for(i=0; i<NumberofRoutes-1; i++)
		{
			temp1 = ROUTETABLE[i].NETWORK;
			temp2 = arpptr->TARGETIPADDR & ROUTETABLE[i].SUBNET;

			if (temp1 == temp2)
			{
				arpptr->ARPOPCODE = 0x0200;
				memset(arpptr->TARGETHWADDR, 0 ,6);
				memcpy(arpptr->SENDHWADDR, ourMACAddr ,6);

				temp1 = arpptr->TARGETIPADDR;
				arpptr->TARGETIPADDR = arpptr->SENDIPADDR;
				arpptr->SENDIPADDR = temp1;

				memcpy(ethptr->DEST, ethptr->SOURCE ,6); 
				memcpy(ethptr->SOURCE, ourMACAddr ,6); 

				Send_IP(ethptr,42);

			}
		}

			// 
		
		i++;
		return;
	}
	
	if (arpptr->ARPOPCODE == 0x0200)
	{
		// Reply
		
		i++;
		return;
	}
	
	i++;

	return;
}

VOID ProcessIPMsg(PIPMSG IPptr)
{
	ULONG Dest;

	if (IPptr->VERLEN != 0x45) return; //Only support Type = 4, Len = 20

	if (!CheckIPChecksum(IPptr)) return;

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
	}

	if (ICMPptr->ICMPTYPE == 8)
	{
		//	ICMP_REPLY:
	}

	// Ingonre others
}

VOID RouteIPMsg(PIPMSG IPptr)
{
	ULONG temp1, temp2;
	int i;
	PROUTEENTRY Route;

	IPptr->IPTTL--;

	if (IPptr->IPTTL == 0) return;

	IPptr->IPCHECKSUM = 0;

	// CHECKSUM IT

	IPptr->IPCHECKSUM = Generate_CHECKSUM(IPptr, 20);

	for(i=0; i<NumberofRoutes; i++)
	{
		Route = &ROUTETABLE[i];
		
		temp1 = Route->NETWORK;
		temp2 = IPptr->IPDEST & Route->SUBNET;
			
		if (temp1 == temp2)
		{
			// Found Route

			if (Route->PORT == 0xff)
				SendIPtoBPQDEV(IPptr, Route);
			else
				SendIPtoAX25(IPptr, Route);

			break;
		}
	}
}

VOID SendIPtoBPQDEV(PIPMSG IPptr, PROUTEENTRY Route)
{	
	// AX.25 headers are bigger, so there will always be room in buffer for enet header
	
	PETHMSG Ethptr = (PETHMSG)IPptr;
	int Len;

	(UCHAR *)Ethptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=14;			// Add eth Header

	memcpy(Ethptr->DEST, Route->Target, 6);
	memcpy(Ethptr->SOURCE, ourMACAddr, 6);
	Ethptr->ETYPE= 0x0008;

	Send_IP(Ethptr,Len);

	return;
}

VOID SendIPtoAX25(PIPMSG IPptr, PROUTEENTRY Route)
{
	PMESSAGE Msgptr = (PMESSAGE)IPptr;
	int Len;

	(UCHAR *)Msgptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=16;

	memcpy(Msgptr->DEST, Route->Target, 7);
	memcpy(Msgptr->ORIGIN, MYCALL, 7);
	Msgptr->ORIGIN[6] |= 1;						// Set End of Call
	Msgptr->CTL = 3;		//UI
	Msgptr->PID = 0xcc;		//IP

	_asm {

	pushfd
	cld
	pushad

	mov	al,4
	mov	ah,10
	mov	ecx,Len
	mov	esi,Msgptr
	add esi,7

	call	RAWTX

	popad
	popfd

	}				// End of ASM

	return;
}

/*

IPSUMANDSEND:
;
;       ADD IP HEADER AND SEND
;
	ADD     BUFFLEN[BX],20          ; IP HDDR LEN
	SUB     BUFFPTR[BX],20          ; DITTO

	MOV     CX,BUFFLEN[BX]          ; GET LENGTH
	MOV     DI,BUFFPTR[BX]
;
;       DI NOW POINTER TO IP BIT
;
	XCHG	CH,CL
	MOV	IPLENGTH[DI],CX

	MOV	AL,TTL
	MOV     IPTTL[DI],AL            ; RESET TTL
	MOV     IPCHECKSUM[DI],0        ; WILL CALCULATE LATER

	MOV4    TEMP,IPDEST[DI]
	MOV4    IPDEST[DI],IPSOURCE[DI]
	MOV4    IPSOURCE[DI],TEMP       ; SWAP IP ADDRESS
;
;       HEADER COMPLETE - ADD CHECKSUM AND TRACE IF NECESSARY
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,10                   ; LENGTH

	CALL    DO_CHECKSUM
	NOT     DX
	MOV     IPCHECKSUM[DI],DX
;




*/

