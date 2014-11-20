
//	I tried adding a second IPIP header, but for some reason Windows wouldn't send the reply. so
//	trying with UDP encap

#ifdef WIN32

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include "winsock2.h"
#include "WS2tcpip.h"

#define ioctl ioctlsocket
#else
#define ioctlsocket ioctl

#define Dll
#define DllExport 

#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BOOL int
#define VOID void
#define UCHAR unsigned char
#define USHORT unsigned short
#define ULONG unsigned long
#define UINT unsigned int
#define SHORT short
#define DWORD unsigned int
#define BYTE unsigned char
#define APIENTRY
#define WINAPI
#define WINUSERAPI
#define TCHAR char
#define TRUE 1
#define FALSE 0
#define FAR
#define byte UCHAR
#define Byte UCHAR
#define Word WORD

#define HANDLE UINT
#define SOCKET int

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define WSAGetLastError() errno
#define GetLastError() errno 

#endif

#include <stdio.h>

struct myin_addr {
	union {
		struct { unsigned char  s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { unsigned short s_w1,s_w2; } S_un_w;
		unsigned long S_addr;
	} S_un;
};

typedef struct _IPMSG
{
//       FORMAT OF IP HEADER
//
//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	unsigned char VERLEN;          // 4 BITS VERSION, 4 BITS LENGTH
	unsigned char TOS;             // TYPE OF SERVICE
	unsigned short IPLENGTH;        // DATAGRAM LENGTH
	unsigned short 	IPID;            // IDENTIFICATION
	unsigned short 	FRAGWORD;        // 3 BITS FLAGS, 13 BITS OFFSET
	unsigned char	IPTTL;
	unsigned char	IPPROTOCOL;      // HIGHER LEVEL PROTOCOL
	unsigned short 	IPCHECKSUM;      // HEADER CHECKSUM
	struct myin_addr IPSOURCE;
	struct myin_addr IPDEST;

	unsigned char	Data;

} IPMSG, *PIPMSG;


SOCKET EncapSock = 0;
SOCKET UDPSock = 0;
BOOL IPv6 = FALSE;
int UDPPort = 4473;


unsigned long UCSD44;		// 44.0.0.1
unsigned long NewAddr;		// Address to relay to 

union
{
	struct sockaddr_in txaddr;
	struct sockaddr_in6 txaddr6;
} TXaddr;

struct sockaddr_in EncapDest;

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


unsigned short Generate_CHECKSUM(VOID * ptr1, int Len)
{
	unsigned short checksum=0;

	checksum = cksum((unsigned short *)ptr1, Len);

	return ~checksum ;
}
/*
VOID ProcessTunnelMsg(PIPMSG IPptr)
{
	unsigned char * ptr;
	PIPMSG Outer = IPptr;			// Save tunnel header
	int Origlen;
//	int InnerLen;
	int sent;
	struct in_addr in;

	if (IPptr->VERLEN != 0x45)
		return;

	Origlen = htons(Outer->IPLENGTH);

//	ptr = (UCHAR *)IPptr;
//	ptr += 20;						// Skip IPIP Header
//	IPptr = (PIPMSG) ptr;
	
	Outer->IPDEST.S_un.S_addr = NewAddr;		// Redirct to target

	Outer->IPCHECKSUM = 0;
	Outer->IPCHECKSUM = Generate_CHECKSUM(Outer, 20);

	// This will add annother encap header, so the target knows the originator

	sent = sendto(EncapSock, (char *)IPptr, Origlen , 0 , (struct sockaddr *)&TXaddr, sizeof(struct sockaddr_in));
	sent = GetLastError();

	memcpy(&in, &Outer->IPSOURCE, 4);
	printf("%s\n", inet_ntoa(in));
}
*/

VOID ProcessTunnelMsg(PIPMSG IPptr)
{
	// UDP Version

	int sent;
	struct in_addr in;
	int Origlen;

	Origlen = htons(IPptr->IPLENGTH);

	// Send as UDP

//	if (IPv6)
//		sent = sendto(UDPSock, Origlen, txlen, 0, (struct sockaddr *)destaddr6, sizeof(destaddr6));
//	else
		sent = sendto(UDPSock, IPptr, Origlen, 0, (struct sockaddr *)&TXaddr, sizeof(struct sockaddr_in));
	
	
//	memcpy(&in, &IPptr->IPSOURCE, 4);
//	printf("%s\n", inet_ntoa(in));
}

VOID ProcessUDPMsg(PIPMSG IPptr, int nread)
{
	// Encap Address is after the message

	UCHAR * ptr;
	int sent;

	ptr = (UCHAR *)IPptr;
	nread -= 4;
	memcpy(&EncapDest.sin_addr.s_addr, ptr + nread, 4);

	sent = sendto(EncapSock, (char *)IPptr, nread, 0, (struct sockaddr *)&EncapDest, sizeof(struct sockaddr_in));
	sent = GetLastError();
}


int main(int argc, char* argv[])
{
	union
	{
		struct sockaddr_in sinx; 
		struct sockaddr_in6 sinx6; 
	} sinx = {0};

	union
	{
		struct sockaddr_in rxaddr;
		struct sockaddr_in6 rxaddr6;
	} RXaddr;
	
	unsigned long param = 1;
	int err;
	int ret;
	char Msg[80];
	int nread;
	int addrlen = sizeof(struct sockaddr_in);
	unsigned char Buffer[1600];
	fd_set readfd, writefd, exceptfd;
	struct timeval timeout;
	int retval;
	int n;
	int Active = 0;
	SOCKET maxsock;



#ifdef WIN32

	WSADATA	WsaData; 
	WSAStartup(MAKEWORD(2, 0), &WsaData);

#endif

	maxsock = EncapSock = socket(AF_INET, SOCK_RAW, 4);

	if (EncapSock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		printf("Failed to create RAW socket for IPIP Encap - error code = %d\n", err);
		return 0;
	}

	ioctl(EncapSock, FIONBIO, &param);

	sinx.sinx.sin_family = AF_INET;

	sinx.sinx.sin_port = 0;

	if (bind(EncapSock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		//	Bind Failed

		err = WSAGetLastError();
		printf("Bind Failed for for IPIP Encap socket - error code = %d\n", err);
	}

	UCSD44 = inet_addr("44.0.0.1");

	EncapDest.sin_family = AF_INET;
	EncapDest.sin_port = 0;

	printf("Net44 Tunnel opened\n");

	// Open UDP Socket

	if (IPv6)
		UDPSock = socket(AF_INET6,SOCK_DGRAM,0);
	else
		UDPSock= socket(AF_INET,SOCK_DGRAM,0);

	if (UDPSock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		printf("Failed to create UDP socket, %d", err);
		return; 
	}

	if (UDPSock > maxsock)
		maxsock = UDPSock;

	ioctl(UDPSock, FIONBIO, &param);
 
#ifndef WIN32

	if (IPv6)
		if (setsockopt(UDPSock, IPPROTO_IPV6, IPV6_V6ONLY, &param, sizeof(param)) < 0)
			perror("setting option IPV6_V6ONLY");
  
#endif
	
	if (IPv6)
	{
		sinx.sinx.sin_family = AF_INET6;
		memset (&sinx.sinx6.sin6_addr, 0, 16);
	}
	else
	{
		sinx.sinx.sin_family = AF_INET;
		sinx.sinx.sin_addr.s_addr = INADDR_ANY;
	}
		
	sinx.sinx.sin_port = htons(UDPPort + 1);	// Need different Send and Receive ports

	if (IPv6)
		ret = bind(UDPSock, (struct sockaddr *) &sinx.sinx, sizeof(sinx.sinx6));
	else
		ret = bind(UDPSock, (struct sockaddr *) &sinx.sinx, sizeof(sinx.sinx));

	if (ret != 0)
	{
		//	Bind Failed

		err = WSAGetLastError();
		printf("Bind Failed for UDP socket %d - error code = %d", UDPPort, err);;
		return;
	}

	NewAddr = inet_addr("192.168.1.64");
	TXaddr.txaddr.sin_addr.s_addr = NewAddr;
	TXaddr.txaddr.sin_family = AF_INET;
	TXaddr.txaddr.sin_port = htons(UDPPort);

	while (TRUE)
	{
		FD_ZERO(&readfd);
		FD_ZERO(&writefd);
		FD_ZERO(&exceptfd);

		FD_SET(EncapSock,&readfd);
		FD_SET(UDPSock,&readfd);

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		retval = select(maxsock + 1, &readfd, &writefd, &exceptfd, &timeout);

		if (retval == -1)
		{				
			perror("data select");;
		}
		else
		{
			if (retval)
			{
				// see who has data

				if (FD_ISSET(EncapSock, &readfd))
				{
					nread = recvfrom(EncapSock, Buffer, 1600, 0, (struct sockaddr *)&RXaddr.rxaddr, &addrlen);

					if (nread > 0)
					{
						PIPMSG IPptr = (PIPMSG)Buffer;

						if (IPptr->IPPROTOCOL == 4)		// AMPRNET Tunnelled Packet
						{
							ProcessTunnelMsg(IPptr);
						}
					}
				}

				if (FD_ISSET(UDPSock, &readfd))
				{
					nread = recvfrom(UDPSock, Buffer, 1600, 0, (struct sockaddr *)&RXaddr.rxaddr, &addrlen);

					if (nread > 0)
					{
						PIPMSG IPptr = (PIPMSG)Buffer;
						ProcessUDPMsg(IPptr, nread);
					}
					else
						nread = GetLastError();
				}
			}		
		}
	}
	
	return 0;
}

