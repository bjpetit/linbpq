//
//	Interface to the Flexnet SoundModem Driver
//

//	Version 1.0 November 2010
//

#pragma data_seg("_BPQDATA")

#define WIN32_LEAN_AND_MEAN
#define _USE_32BIT_TIME_T

#include "windows.h"
#include "time.h"
#include <stdlib.h>

//#include <process.h>
//#include <time.h>

#include "ASMStrucs.h"
#include "bpq32.h"
 
#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )


#pragma pack(1)

#define MAXFLEN 400     /* Maximale Laenge eines Frames */
                        /* maximum length of a frame */
typedef signed short    i16;
typedef unsigned char   byte;
typedef unsigned long   u32;


/* Struct f. Treiberkommunikation bei TX und RX */
/* struct for communicating RX and TX packets to the driver */
typedef struct
{
    i16  len;               /* Laenge des Frames - length of the frame */
    byte kanal;             /* Kanalnummer - channel number */
    byte txdelay;           /* RX: Gemessenes TxDelay [*10ms],
                                   0 wenn nicht unterstuetzt
                               TX: Zu sendendes TxDelay */
                            /* RX: measured transmitter keyup delay (TxDelay) in 10ms units,
                                   0 if not supported
                               TX: transmitter keyup delay (TxDelay) that should be sent */
    byte frame[MAXFLEN];    /* L1-Frame (ohne CRC) - L1 frame without CRC */
} L1FRAME;

typedef struct
{
    u32 tx_error;           /* Underrun oder anderes Problem - underrun or some other problem */
    u32 rx_overrun;         /* Wenn Hardware das unterstuetzt - if supported by the hardware */
    u32 rx_bufferoverflow;
    u32 tx_frames;          /* Gesamt gesendete Frames - total number of sent frames */
    u32 rx_frames;          /* Gesamt empfangene Frames - total number of received frames */
    u32 io_error;           /* Reset von IO-Device - number of resets of the IO device */
    u32 reserve[4];         /* f. Erweiterungen, erstmal 0 lassen! - reserved for extensions, leave 0! */
} L1_STATISTICS;

#pragma pack()

// Each Soundcard needs a separate instance of the BPQSoundModem Program

// Each can support more than one bpq port - for instance one 1200 and one 2400 on same radio

// There is one Modem Channel per BPQ Port


struct CHANNELINFO
{
	int Channel;
	struct PORTCONTROL * PORTVEC;
	int BPQPort;
	BOOL RXReady;
};

typedef struct SOUNDTNCINFO
{ 
	int SoundCardNumber;
	HWND hDlg;							// Status Window Handle
	struct CHANNELINFO Channellist[5];	// Max Channels
	int PERSIST;						// CSMA PERSIST
	int TXQ;

	int State;					// Channel State Flags
	int PID;
	BOOL WeStartedTNC;
};

// These must all be in BPQDATA seg so it can be accessed from other processes, must be initialised

struct SOUNDTNCINFO * PortToTNC[33] = {0};
int PortToChannel[33] = {0};

struct SOUNDTNCINFO SoundCardList[11] = {0};
static struct CHANNELINFO * Portlist[33] = {0};

UINT RXQ[33] = {0};				// Frames waiting for BPQ32

typedef UINT (FAR *FARPROCX)();

UINT (FAR *SoundModem)() = NULL;

HINSTANCE ExtDriver=0;
HANDLE hRXThread;

L1FRAME * rxf;

UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
UINT ReleaseBuffer(UINT *BUFF);

VOID __cdecl Debugprintf(const char * format, ...);
HANDLE _beginthread( void( *start_address )(), unsigned stack_size, int arglist);
static int ExtProc(int fn, int port, unsigned char * buff);
VOID * zalloc(int len);
KillSoundTNC(struct SOUNDTNCINFO * TNC);
RestartSoundTNC(struct SOUNDTNCINFO * TNC);


UINT WINAPI SoundModemExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[80];
//	HKEY hKey=0;
//	int ret;
	int Port = PortEntry->PORTNUMBER;
	int Channel = PortEntry->CHANNELNUM - 65;
	int SoundCardNumber = PortEntry->IOBASE;
	struct SOUNDTNCINFO * SoundCard;
	struct CHANNELINFO * Chan;

	wsprintf(msg,"Sound Modem Port %d", Port);
	WritetoConsole(msg);

	if (Channel > 4)
	{
		wsprintf(msg,"Error - Channel must be A to E");
		WritetoConsole(msg);
		return ((UINT) ExtProc);
	}

	if (SoundCardNumber < 1 || SoundCardNumber > 10)
	{
		wsprintf(msg,"Error - IOADDR must be 1 to 10");
		WritetoConsole(msg);
		return ((UINT) ExtProc);
	}

	// See if we already have a port on this soundcard (Address in IOBASE)

	SoundCard = &SoundCardList[SoundCardNumber];
	SoundCard->SoundCardNumber = SoundCardNumber;
	SoundCard->PERSIST = PortEntry->PORTPERSISTANCE;

	Chan = &SoundCard->Channellist[Channel];

	Chan->PORTVEC = PortEntry;
	Chan->Channel = Channel;
	Chan->BPQPort = Port;

	PortToTNC[Port] = SoundCard;
	PortToChannel[Port] = Channel;

	Portlist[Port] = Chan;

	if (SoundCard->WeStartedTNC == 0)
		SoundCard->WeStartedTNC = RestartSoundTNC(SoundCard);

	return ((UINT) ExtProc);
}


static int ExtProc(int fn, int port, unsigned char * buff)
{
	int len = 0, txlen=0;
	struct CHANNELINFO * PORT = Portlist[port];
	struct PORTCONTROL * PORTVEC = PORT->PORTVEC;
	struct SOUNDTNCINFO * TNC = PortToTNC[port];
	int State = TNC->State;
	UINT * buffptr;

	switch (fn)
	{
	case 1:				// poll

		if (State & 0x20)
			PORTVEC->SENDING++;

		if (State & 0x10)
			PORTVEC->ACTIVE++;

		buffptr = Q_REM(&RXQ[port]);

		if (buffptr)
		{
			Debugprintf("Got Frame Port %d", port);
			len = buffptr[1];

			memcpy(&buff[7], &buffptr[2], len);
			len+=7;
			buff[5]=(len & 0xff);
			buff[6]=(len >> 8);

			ReleaseBuffer(buffptr);

			return len;
		}

		return 0;

	case 2: // send

		buffptr = GetBuff();
			
		if (buffptr == 0) return (0);			// No buffers, so ignore

		txlen = (buff[6] <<8 ) + buff[5] - 7;	
		buffptr[1] = txlen;
		buffptr[2] = PortToChannel[port];		// Channel on this Soundcard
		memcpy(buffptr+3, &buff[7], txlen);

		C_Q_ADD(&TNC->TXQ, buffptr);

		return 0;


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		return (0);		// OK
			
	case 5:				// Close

		if (TNC->PID)
		{
			KillSoundTNC(TNC);
			Sleep(100);
		}

		return (0);

	}

	return 0;

}

KillSoundTNC(struct SOUNDTNCINFO * TNC)
{
	HANDLE hProc;

	if (TNC->PID == 0) return 0;

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->PID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	TNC->PID = 0;			// So we don't try again

	return 0;
}

RestartSoundTNC(struct SOUNDTNCINFO * TNC)
{
	char cmdLine[80];
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	wsprintf(cmdLine, "BPQSoundModem.exe %d %d", TNC->SoundCardNumber, TNC->PERSIST); 

	return CreateProcess("BPQSoundModem.exe", cmdLine, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);

	return 0;
}

enum SMCmds
{
	INIT,
	CHECKCHAN,		// See if channel is configured
	GETBUFFER,
	POLL,
	RXPACKET,
	CLOSING
};

DllExport UINT APIENTRY SoundCardInterface(int CardNo, int Function, UINT Param1, UINT Param2)
{
	switch (Function)
	{
	case INIT:	
		SoundCardList[CardNo].PID = Param1;
		Debugprintf("SoundCard %d PID %d", CardNo, Param1);
		break;

	case CHECKCHAN:

		Debugprintf("CheckChan %d %d = %d", CardNo, Param1,  SoundCardList[CardNo].Channellist[Param1].BPQPort); 

		return SoundCardList[CardNo].Channellist[Param1].BPQPort;	// Nonzero if port is configured

	case GETBUFFER:
		return (UINT)GetBuff();

	case POLL:

		// See if anything on the TXQ

		SoundCardList[CardNo].State = Param1;		// PTT and DCD State bits

		return (UINT)Q_REM(&SoundCardList[CardNo].TXQ);

	case RXPACKET:

		// A received Packet. Param1 is buffer, Param2 the channel

		C_Q_ADD(&RXQ[SoundCardList[CardNo].Channellist[Param2].BPQPort], (UINT *)Param1);
		
		break;

	case CLOSING:
		break;
	}
	return 0;
}



	

