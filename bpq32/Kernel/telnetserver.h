#pragma once

#include "resource.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_CONNECT WM_USER + 2
#define WSA_DATA WM_USER + 3

#define InputBufferLen 500

struct ConnectionInfo
{
	int Number;					// Number of record - for Connections display
    SOCKET socket;
	union
	{
		SOCKADDR_IN6 sin6;  
		SOCKADDR_IN sin;
	};
	BOOL SocketActive;
    int BPQStream;
    byte Callsign[10];
    BOOL GotHeader;
    byte InputBuffer[500];
    int InputLen;
	struct UserRec * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	BOOL DoingCommand;			// Processing Telnet Command
	BOOL DoEcho;				// Telnet Echo option accepted
	BOOL FBBMode;				// Pure TCP for FBB forwarding
	BOOL RelayMode;				// Pure TCP for RMS Relay Emulation forwarding
	BOOL Auth;					// Set if User is flagged as a Secure User
	BOOL BPQTermMode;			// Set if connected to BPQTermTCP
	BOOL MonitorNODES;			// Monitor Control Flags
	UINT MMASK;
	BOOL MCOM;
	BOOL MonitorColour;
	BOOL MTX;
	int CMSIndex;				// Pointer to CMS used for this connect
	UCHAR * FromHostBuffer;		// Somewhere to store msg from CMS - it sends the whole message at once
	int FromHostBufferSize;
	int FromHostBuffPutptr;	//
	int FromHostBuffGetptr;	//

	time_t ConnectTime;	
};

#define Disconnect(stream) SessionControl(stream,2,0)
#define Connect(stream) SessionControl(stream,1,0)

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
#define xStatus 5 //859
#define echo 1 //857
#define timingmark 6 //860
#define terminaltype 24 //1091
#define windowsize 31 //1073
#define terminalspeed 32 //1079
#define remoteflowcontrol 33 //1372
#define linemode 34 //1184
#define environmentvariables 36 //1408

