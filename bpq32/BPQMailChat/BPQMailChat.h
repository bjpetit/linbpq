#pragma once

#include "resource.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2

#define _USE_32BIT_TIME_T
#include "time.h"

struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
};

#define InputBufferLen 500

struct ConnectionInfo
{
	int Number;					// Number of record - for Connections display
//    SOCKET socket;
//	SOCKADDR_IN sin;  
	BOOL Active;
    int BPQStream;
    byte Callsign[10];
    BOOL GotHeader;
    byte InputBuffer[500];
    int InputLen;
	struct UserInfo * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	int Flags;
//	BOOL DoingCommand;			// Processing Telnet Command
//	BOOL DoEcho;				// Telnet Echo option accepted
	int Conference;				// Conf number in chat mode
	UCHAR * OutputQueue;			// Messages to user
	int OutputQueueLength;
};

// Flags Equates

#define GETTINGUSER 1
#define GETTINGBBS 2
#define CHATMODE 4

struct UserInfo{

	char	Call[10]     ;	/* 8  Callsign */
//	indicat relai[8]  ;	/* 64 Digis path */
	long	lastmsg  ;	/* 4  Last L number */
	long	nbcon;	/* 4  Number of connexions */
	time_t	TimeLastCOnnected;  //Last connexion date */
//	long	lastyap __a2__  ;	/* 4  Last YN date */
	short	flags    ;	/* 2  Flags */
	short	on_base  ;	/* 2  ON Base number */

	UCHAR	nbl       ;	/* 1  Lines paging */
	UCHAR	lang      ;	/* 1  Language */

	long	newbanner;	/* 4  Last Banner date */
	short 	download  ;	/* 2  download size (KB) = 100 */
	char	xfree[20]  ;	/* 20 Reserved */
	char	theme     ;	/* 1  Current topic */

	char	Name[18]   ;	/* 18 1st Name */
	char	Address[61] ;	/* 61 Address */
	char	City[31] ;	/* 31 City */
	char	HomeBBS[41]  ;	/* 41 home BBS */
	char	QRA[7]    ;	/* 7  Qth Locator */
	char	pass[13]  ;	/* 13 Password */
	char	ZIP[9]    ;	/* 9  Zipcode */

} ;                /* Total : 360 bytes */


#define Connect(stream) SessionControl(stream,1,0)
#define Disconnect(stream) SessionControl(stream,2,0)
#define ReturntoNode(stream) SessionControl(stream,3,0)

#define SE 240 // End of subnegotiation parameters
#define NOP 241 //No operation
#define DM 242 //Data mark Indicates the position of a Synch event within the data stream. This should always be accompanied by a TCP urgent notification.
#define BRK 243 //Break Indicates that the "break" or "attention" key was hi.
#define IP 244 //Suspend Interrupt or abort the process to which the NVT is connected.
#define AO 245 //Abort output Allows the current process to run to completion but does not send its output to the user.
#define AYT 246 //Are you there Send back to the NVT some visible evidence that the AYT was received.
#define EC 247 //Erase character The receiver should delete the last preceding undeleted character from the data stream.
#define EL 248 //Erase line Delete characters from the data stream back to but not including the previous CRLF.
#define GA 249 //Go ahead Under certain circumstances used to tell the other end that it can transmit.
#define SB 250 //Subnegotiation Subnegotiation of the indicated option follows.
#define WILL 251 //will Indicates the desire to begin performing, or confirmation that you are now performing, the indicated option.
#define WONT 252 //wont Indicates the refusal to perform, or continue performing, the indicated option.
#define DOx 253 //do Indicates the request that the other party perform, or confirmation that you are expecting the other party to perform, the indicated option.
#define DONT 254 //dont Indicates the demand that the other party stop performing, or confirmation that you are no longer expecting the other party to perform, the indicated option.
#define IAC  255

#define suppressgoahead 3 //858
#define Status 5 //859
#define echo 1 //857
#define timingmark 6 //860
#define terminaltype 24 //1091
#define windowsize 31 //1073
#define terminalspeed 32 //1079
#define remoteflowcontrol 33 //1372
#define linemode 34 //1184
#define environmentvariables 36 //1408

