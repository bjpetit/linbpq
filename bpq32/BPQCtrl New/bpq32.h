
//	Returns number of free buffers
//	(BPQHOST function 7 (part)).

extern "C" int  __stdcall GetFreeBuffs();



//	Returns count of packets waiting on stream
//	 (BPQHOST function 7 (part)).

extern "C" int  __stdcall RXCount(int Stream);




//	Returns number of packets on TX queue for stream
//	 (BPQHOST function 7 (part)).

extern "C" int  __stdcall TXCount(int Stream);



//	Returns number of monitor frames available
//	 (BPQHOST function 7 (part)).

extern "C" int  __stdcall MONCount(int Stream);



//	Returns call connecten on stream (BPQHOST function 8 (part)).
extern "C" int  __stdcall GetCallsign(int stream, char * callsign);

//	Returns connection info for stream (BPQHOST function 8).

extern "C" int  __stdcall GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window);



//	Send Session Control command (BPQHOST function 6)
//	Command = 0 Connect using APPL MASK IN param
//	Command = 1 Connect
//	Command = 2 Disconect
//	Command = 3 Return to Node
extern "C" int  __stdcall SessionControl(int stream, int command, int param);



//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	Top bit of flags enables monitoring

extern "C" int  __stdcall SetAppl(int stream, int flags, int mask);



//	Get current Session State. Any state changed is ACK'ed
//	automatically. See BPQHOST functions 4 and 5.

extern "C" int APIENTRY SessionState(int stream, int * state, int * change);

//	Get current Session State. Dont Ack state change
//	See BPQHOST function 4.

extern "C" int APIENTRY SessionStateNoAck(int stream, int * state);



//	Send message go stream (BPQHOST Function 2)

extern "C" int  __stdcall SendMsg(int stream, const char * msg, int len);



//	Send Raw (KISS mode) frame to port (BPQHOST function 10)

extern "C" int APIENTRY SendRaw(int port, const char * msg, int len);



//	Get message from stream. Returns length, and count of frames
//	still waiting to be collected. (BPQHOST function 3)

extern "C" int  __stdcall GetMsg(int stream, unsigned char * msg, int * len, int * count );



//	Get Raw (Trace) data (BPQHOST function 11)

extern "C" int APIENTRY GetRaw(int stream, char * msg, int * len, int * count );



//	This is not an API function. It is a utility to decode a received
//	monitor frame into ascii text.

extern "C" int APIENTRY DecodeFrame(char * msg, char * buffer, int Stamp);


//	Sets the tracing options for DecodeFrame. Mask is a bit
//	mask of ports to monitor (ie 101 binary will monitor ports
//	1 and 3). MTX enables monitoring on transmitted frames. MCOM
//	enables monitoring of protocol control frames (eg SABM, UA, RR),
//	as well as info frames.

extern "C" int APIENTRY SetTraceOptions(long mask, int mtxparam, int mcomparam);



//	Returns number of first unused BPQHOST stream. If none available,
//	returns 255. See API function 13.

extern "C" int APIENTRY FindFreeStream();



//	Allocate stream. If stream is already allocated, return nonzero.
//	Otherwise allocate stream, and return zero

extern "C" int APIENTRY AllocateStream(int stream);



//	Release stream.

extern "C" int APIENTRY DeallocateStream(int stream);

//	Get number of ports configured

extern "C" int APIENTRY GetNumberofPorts();

//	Enable async operation - new to Win32 version of API

extern "C" int APIENTRY BPQSetHandle(int Stream, HWND hWnd);

// Returns Path of BPQDirectroy

extern "C" UCHAR * APIENTRY GetBPQDirectory();


extern "C" UCHAR * APIENTRY GetNodeCall();
extern "C" UCHAR * APIENTRY GetNodeAlias();
extern "C" UCHAR * APIENTRY GetBBSCall();
extern "C" UCHAR * APIENTRY GetBBSAlias();

extern "C" char * APIENTRY GetApplCall(int Appl);
extern "C" BOOL APIENTRY SetApplCall(int Appl, const char * NewCall);

extern "C" char * APIENTRY GetApplAlias(int Appl);
extern "C" BOOL APIENTRY SetApplAlias(int Appl, const char * NewCall);

extern "C" long APIENTRY GetApplQual(int Appl);
extern "C" BOOL APIENTRY SetApplQual(int Appl, int Qual);




//
//	Constants and equates for async operation
//

char BPQWinMsg[] = "BPQWindowMessage";
UINT BPQMsg;

//
//	Values returned in lParam of Windows Message
//
#define BPQMonitorAvail 1
#define BPQDataAvail 2
#define BPQStateChange 4

