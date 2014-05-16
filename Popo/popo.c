#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <syslog.h>

#include <linux/i2c-dev.h>

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

typedef DWORD   COLORREF;
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((USHORT)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)      rgb & 0xFF
#define GetGValue(rgb)      (rgb >> 8) & 0xFF
#define GetBValue(rgb)      (rgb >> 16) & 0xFF


#define HWND unsigned int
#define HINSTANCE unsigned int
#define HKEY unsigned int
#define UINT_PTR unsigned int *

#define HANDLE UINT
#define SOCKET int

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define HMENU UINT
#define WNDPROC UINT
#define __cdecl

#define strtok_s strtok_r

#define _memicmp memicmp
#define _stricmp stricmp
#define _strdup strdup
#define _strupr strupr
#define _strlwr strlwr

#define _gcvt gcvt
#define _fcvt fcvt
#define _atoi64 atoll 

#define DeleteFile unlink
#define MoveFile rename
#define CreateDirectory mkdir

int memicmp(unsigned char *a, unsigned char *b, int n);
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2);
char * strupr(char* s);
char * strlwr(char* s);

#define WSAGetLastError() errno
#define GetLastError() errno 
#define closesocket close
#define GetCurrentProcessId getpid

char * inet_ntoa(struct in_addr in);

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in *PSOCKADDR_IN;
typedef struct sockaddr_in *LPSOCKADDR_IN;

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;
typedef struct sockaddr *LPSOCKADDR;

#define __int16 short
#define __int32 long


#define SD_BOTH 2

struct I2CUART
{
	int	device;
	char RXBuffer[1024];
	char TXBuffer[1024];
	char Line[256];
	int RXPPTR;
	int RXGPTR;
	int TXPPTR;
	int TXGPTR;
	int Cursor;
};

struct I2CUART i2cUART[4];

void polli2c(struct I2CUART * Dev);

int i2c;

struct sockaddr_in sinx;
struct sockaddr_in rxaddr;

int addrlen=sizeof(sinx);


int udpsock;

int main(int argc, char* argv[])
{
    int bus_addr = -1, i;
    char buf[300] = "";
    int n, len;
	char c;
	struct I2CUART * Dev;
	u_long param=1;


	for (n = 0; n < 4; n++)
	{
		memset(&i2cUART[n], 0, sizeof(struct I2CUART));
		i2cUART[n].device = 8 + n + n;		// Two addresses per device
	}
	
	Dev = &i2cUART[0];
	Dev->TXPPTR = sprintf(Dev->TXBuffer, "1234567890 QWERTYUIOP XXXX \r1234567890 QWERTYUIOP YYYY 1234567890 QWERTYUIOP\r");
	
	Dev = &i2cUART[1];
	Dev->TXPPTR = sprintf(Dev->TXBuffer, "QWERTYUIOP XXXX \r1234567890 QWERTYUIOP YYYY\r1234567890 QWERTYUIOP\r");
	
	
#include <fcntl.h>
#include <stdio.h>

	// Open i2c Bus
	
    i2c = open("/dev/i2c-0", O_RDWR);
	
    if(i2c < 0)
    {
        perror("open");
        return 1;
    }
	
	// open UDP socket
	

	udpsock = socket(AF_INET,SOCK_DGRAM,0);

	ioctl(udpsock, FIONBIO, &param);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;		
	sinx.sin_port = htons(8874);

	if (bind(udpsock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		perror("bind");
	}


	while (1)
	{
		int rxgp;
		
		len = recvfrom(udpsock, buf, 256, 0, (LPSOCKADDR)&rxaddr, &addrlen);

		if (len > 0)
		{
			buf[len] = 0;
			if (!Check0183CheckSum(buf, len))
				printf("Checksum Error %s", buf);
			else
			{
				if (memcmp(buf, "$GPRMC", 6) == 0)
				{
					Dev = &i2cUART[0];
					Dev->TXPPTR += sprintf(&Dev->TXBuffer[Dev->TXPPTR], buf);
					Dev = &i2cUART[1];
					Dev->TXPPTR += sprintf(&Dev->TXBuffer[Dev->TXPPTR], buf);
					printf("%s", buf);
				}
			}			
		}
		
		for (n = 0; n < 4; n++)
		{
			Dev = &i2cUART[n];
				
			polli2c(Dev);				// See if any input and send any output
			
			//	See if a complete line received
			
			rxgp = Dev->RXGPTR;
	
			while (Dev->RXPPTR != rxgp)
			{
				c = Dev->RXBuffer[rxgp++];
				
				if (rxgp > 1023)
					rxgp = 0;
					
				if (c != 10)
					Dev->Line[Dev->Cursor++] = c;
				
				if (c == 13)
				{
					Dev->Line[Dev->Cursor++] = 0;
					printf("%d %s\n", n, Dev->Line);
					Dev->Cursor = 0;
				}		
			}
			
			Dev->RXGPTR = rxgp;

		}
								
    	usleep(100000);
	}
}


void polli2c(struct I2CUART * Dev)
{
	char buf[300] = "";
    int rc, n, tosend, txg, txp, space;
	
	ioctl(i2c, I2C_SLAVE, Dev->device);		// Select Device
		
    buf[0] = 0;
    
    rc = read(i2c, buf, 2);				// Get Buffer Levels
	   
	if (rc == -1)
		return;
	
    if (buf[0])
	{
		rc = read(i2c, buf, buf[0] + 2);
		
//		printf("rx got %d ", buf[0]);
	
	    for (n = 2; n < rc ; n++)
    	{
			Dev->RXBuffer[Dev->RXPPTR++] = buf[n];
			if (Dev->RXPPTR > 1023)
				Dev->RXPPTR = 0;
		}
	
	}
		
	// See if anything to send
	
	txg = Dev->TXGPTR;
	tosend = Dev->TXPPTR - txg;
	
	if (tosend == 0)
	{
		Dev->TXGPTR = Dev->TXPPTR = 0;
		return;
	}
	space = 32 - buf[1];
	
	if (space == 0)
		return;
	
	if (tosend > space)
		tosend = space;
		
	if ((txg + tosend) > 1023)
		tosend = 1023 - txg;
	
//	printf("txg %d tosend %d ", txg, tosend);
//	fflush(NULL);
	
	for (n = 0; n  < tosend; n++)
	{
		write(i2c, &Dev->TXBuffer[txg + n], 1);
	}
	txg += tosend;
	
	if (txg > 1023)
		txg = 0;
		
	Dev->TXGPTR = txg;		
			
	return;
}
	

BOOL BlueNMEAOK = FALSE;

char HostName[80] = "192.168.1.189";

char Buffer[8192];
int SavedLen = 0;

BOOL Check0183CheckSum(char * msg,int len)
{
	BOOL retcode=TRUE;
	char * ptr;
	UCHAR sum,xsum1,xsum2;

	sum=0;
	ptr=++msg;	//	Skip $

loop:

	if (*(ptr)=='*') goto eom;
	
	sum ^=*(ptr++);

	len--;

	if (len > 0) goto loop;

	return TRUE;		// No Checksum

eom:
	_strupr(ptr);

	xsum1=*(++ptr);
	xsum1-=0x30;
	if (xsum1 > 9) xsum1-=7;

	xsum2=*(++ptr);
	xsum2-=0x30;
	if (xsum2 > 9) xsum2-=7;

	xsum1=xsum1<<4;
	xsum1+=xsum2;

	return (xsum1==sum);
}



static VOID ProcessReceivedData(SOCKET TCPSock)
{
	char UDPMsg[8192];

	int len = recv(TCPSock, &Buffer[SavedLen], 8100 - SavedLen, 0);

	char * ptr;
	char * Lastptr;

	if (len <= 0)
	{
		closesocket(TCPSock);
		BlueNMEAOK = FALSE;
		return;
	}

	len += SavedLen;
	SavedLen = 0;

	ptr = Lastptr = Buffer;

	Buffer[len] = 0;

	while (len > 0)
	{
		ptr = strchr(Lastptr, 10);

		if (ptr)
		{
			int Len = ptr - Lastptr;
		
			memcpy(UDPMsg, Lastptr, Len);
			UDPMsg[Len++] = 13;
			UDPMsg[Len++] = 10;
			UDPMsg[Len++] = 0;

			if (!Check0183CheckSum(UDPMsg, Len))
			{
				printf("Checksum Error %s", UDPMsg);
			}
			else
			{			
	//			if (memcmp(UDPMsg, "$GPRMC", 6) == 0)
	//				DecodeRMC(UDPMsg, Len);

			}
			Lastptr = ptr + 1;
			len -= Len;
		}
		else
			SavedLen = len;
	}
}


	
static VOID TCPConnect()
{
	int err, ret;
	u_long param=1;
	BOOL bcopt=TRUE;
	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	SOCKADDR_IN sinx; 
	struct sockaddr_in destaddr;
	SOCKET TCPSock;
	int addrlen=sizeof(sinx);

	if (HostName[0] == 0)
		return;

	destaddr.sin_addr.s_addr = inet_addr(HostName);
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(4352);

	TCPSock = socket(AF_INET,SOCK_STREAM,0);

	if (TCPSock == INVALID_SOCKET)
	{
  	 	return; 
	}
 
	setsockopt (TCPSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TCPSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		closesocket(TCPSock);

  	 	return; 
	}

	if (connect(TCPSock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//
	}
	else
	{
		err=WSAGetLastError();
   		printf("Connect Failed for BlueNMEA socket - error code = %d\n", err);	
		closesocket(TCPSock);
		return;
	}

	BlueNMEAOK = TRUE;

	while (TRUE)
	{
		FD_ZERO(&readfs);	
		FD_ZERO(&errorfs);

		FD_SET(TCPSock,&readfs);
		FD_SET(TCPSock,&errorfs);

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;				// We should get messages more frequently that this

		ret = select(TCPSock + 1, &readfs, NULL, &errorfs, &timeout);
		
		if (ret == SOCKET_ERROR)
		{
			goto Lost;
		}
		if (ret > 0)
		{
			//	See what happened

			if (FD_ISSET(TCPSock, &readfs))
			{
				ProcessReceivedData(TCPSock);			
			}
								
			if (FD_ISSET(TCPSock, &errorfs))
			{
Lost:				
				printf("BlueNMEA Connection lost\n");			
				closesocket(TCPSock);
				BlueNMEAOK = FALSE;;
				return;
			}
		}
		else
		{
			// 60 secs without data. Shouldn't happen

			shutdown(TCPSock, SD_BOTH);
	    	usleep(100000);
			closesocket(TCPSock);
			BlueNMEAOK = FALSE;
			return;
		}
	}
}


char * strupr(char* s)
{
  char* p = s;

  if (s == 0)
	  return 0;

  while (*p = toupper( *p )) p++;
  return s;
}

char * strlwr(char* s)
{
  char* p = s;
  while (*p = tolower( *p )) p++;
  return s;
}

                                                                                                                                                                                                            
