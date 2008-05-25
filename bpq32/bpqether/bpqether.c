//
//	DLL to provide BPQEther support for G8BPQ switch in a 
//	32bit environment,
//
//	Uses BPQ EXTERNAL interface
//
//	Uses WINPACAP library

//	Version 1.0 December 2005

//  Version 1.1	January 2006
//
//		Get config file location from Node (will check bpq directory)

//  Version 1.2	October 2006

//		Write diagnostics to BPQ console window instead of STDOUT

//	Version 1.3 February 2008

//		Changes for dynamic unload of bpq32.dll

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdio.h>
#include <process.h>
//#include <time.h>

#include "AsmStrucs.h"

#include "pcap.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"

//#include "packet32.h"
//#include "ntddndis.h"


#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )
 
typedef struct PCAPStruct
{
   pcap_t	*adhandle;
   UCHAR	EthSource[6];
   UCHAR	EthDest[6];
   short	EtherType;
   BOOL		RLITX;
   BOOL		RLIRX;

} PCAPINFO, *PPCAPINFO ;

PCAPINFO PCAPInfo[16];

//PPCAPINFO PCAPInfo[16]={0};


char Adapter[256];

UCHAR EthDest[7]={01,'B','P','Q',0,0};

char EtherType[10]="0x08FF";

//pcap_t *adhandle;

struct tagMSG Msg;

short udpport=0;


DllImport int ResetExtDriver(int num);


unsigned int OurInst = 0;
int AttachedProcesses=0;

HWND hWnd;
BOOL GotMsg;

HANDLE STDOUT=0;
DWORD n;


HINSTANCE PcapDriver=0;

typedef int (FAR *FARPROCX)();

int (FAR * pcap_sendpacketx)();


//FARPROCX pcap_sendpacketx;
FARPROCX pcap_open_livex;
FARPROCX pcap_compilex;
FARPROCX pcap_setfilterx;
FARPROCX pcap_datalinkx;
FARPROCX pcap_next_exx;
FARPROCX pcap_geterrx;

char Dllname[6]="wpcap";


int InitPCAP(void);
FARPROCX GetAddress(char * Proc);

BOOL ReadConfigFile(char * fn,int Port);
int ProcessLine(char * buf,int Port);
int OpenPCAP(int IOBASE, int PORTNUMBER);



HANDLE hInstance;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	hInstance=hInst;

	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		AttachedProcesses++;
		
		if (OurInst == 0)				// First entry
		{
			OurInst = GetCurrentProcessId();

			GetAPI();					// Load BPQ32

			if (!InitPCAP())
				return FALSE;

		}
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		if (OurInst == GetCurrentProcessId())
		{
		}
		
		AttachedProcesses--;

		if (AttachedProcesses == 0)
		{
			FreeLibrary(PcapDriver);
		}
		return 1;
	}
 
	return 1;
	
}

DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	int len,txlen=0,res;
	char txbuff[500];
	struct pcap_pkthdr *header;
	u_char *pkt_data;
	char buf[256];
	int n;

	//	char dcall[10],scall[10];

	if (PCAPInfo[port].adhandle == 0) return 0;

	switch (fn)
	{
	case 1:				// poll


		res = pcap_next_exx(PCAPInfo[port].adhandle, &header, &pkt_data);

		if(res == 0)
			/* Timeout elapsed */
			return 0;

		if(res==-1)
		{
			WritetoConsole("Error RX\n");

			return 0;
		}

		if (PCAPInfo[port].RLIRX)
		
		//	RLI MODE - An extra 3 bytes before len, seem to be 00 00 41

		{
			len=pkt_data[18]*256 + pkt_data[17];

			if ((len == 0) || (len > 320)) return 0; // Probably BPQ Mode Frame

			len-=3;
		
			memcpy(&buff[7],&pkt_data[19],len);
		
			len+=5;
		}
		else
		{
			len=pkt_data[15]*256 + pkt_data[14];

			if ((len == 0) || (len > 320)) return 0; // Probably RLI Mode Frame

			len-=3;
		
			memcpy(&buff[7],&pkt_data[16],len);
		
			len+=5;
		}

		buff[5]=(len & 0xff);
		buff[6]=(len >> 8);
		
		return 1;

		
	case 2:				// send
		
 		if (PCAPInfo[port].RLITX)
		
		//	RLI MODE - An extra 3 bytes before len, seem to be 00 00 41

		{
			txlen=(buff[6]<<8) + buff[5];		// BPQEther is DOS-based - chain word is 2 bytes

			txlen-=2;
			txbuff[16]=0x41;
			txbuff[17]=(txlen & 0xff);
			txbuff[18]=(txlen >> 8);

			memcpy(&txbuff[19],&buff[7],txlen);

		}
		else
		{
			txlen=(buff[6]<<8) + buff[5];		// BPQEther is DOS-based - chain word is 2 bytes

			txlen-=2;

			txbuff[14]=(txlen & 0xff);
			txbuff[15]=(txlen >> 8);

			memcpy(&txbuff[16],&buff[7],txlen);
		}

		memcpy(&txbuff[0],&PCAPInfo[port].EthDest[0],6);
		memcpy(&txbuff[6],&PCAPInfo[port].EthSource[0],6);
		memcpy(&txbuff[12],&PCAPInfo[port].EtherType,2);

		txlen+=14;
		

		if (txlen < 60) txlen = 60;

		// Send down the packet 

		if (pcap_sendpacketx(PCAPInfo[port].adhandle,	// Adapter
			txbuff,				// buffer with the packet
			txlen				// size
			) != 0)
		{

			n=wsprintf(buf,"\nError sending the packet: \n", pcap_geterrx(PCAPInfo[port].adhandle));		
			WritetoConsole(buf);
			
			return 3;
		}


		return (0);


		

	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK
			
		break;

	case 4:				// reinit

		return 0;

	case 5:				// reinit

		return 0;
	}

	return (0);
}


DllExport int APIENTRY ExtInit(struct PORTCONTROL *  PortEntry)

//DllExport int APIENTRY ExtInit()
{
	//	Can have multiple ports, each mapping to a different Ethernet Adapter
	
	//	The Adapter number is in IOADDR
	//

	OpenPCAP(PortEntry->IOBASE, PortEntry->PORTNUMBER);

		
	return ((int) ExtProc);
}


InitPCAP()
{
	char Msg[255];
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;

//	Use LoadLibrary/GetProcADDR to get round link problem

	PcapDriver=LoadLibrary(Dllname);

	if (PcapDriver == NULL)
	{
		err=GetLastError();
		wsprintf(Msg,"Error loading Driver %s - Error code %d",
				Dllname,err);
		
		MessageBox(NULL,Msg,"BPQETHER",MB_ICONSTOP);

		return(FALSE);
	}

	if ((pcap_sendpacketx=GetAddress("pcap_sendpacket")) == 0 ) return FALSE;

	if ((pcap_datalinkx=GetAddress("pcap_datalink")) == 0 ) return FALSE;

	if ((pcap_compilex=GetAddress("pcap_compile")) == 0 ) return FALSE;

	if ((pcap_setfilterx=GetAddress("pcap_setfilter")) == 0 ) return FALSE;
	
	if ((pcap_open_livex=GetAddress("pcap_open_live")) == 0 ) return FALSE;

	if ((pcap_geterrx=GetAddress("pcap_geterr")) == 0 ) return FALSE;

	if ((pcap_next_exx=GetAddress("pcap_next_ex")) == 0 ) return FALSE;
	
	
	return (TRUE);
		
}


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


#define Max_Num_Adapter 10


char        AdapterList[Max_Num_Adapter][1024];
char        AdapterLabel[Max_Num_Adapter][1024];

	char		AdapterName[8192]; // string that contains a list of the network adapters
	char		*temp,*temp1;



void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

 
int OpenPCAP(int IOBASE, int port)
{
	int i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	char packet_filter[30];
	struct bpf_program fcode;
	char buf[256];
	int n;


	WritetoConsole("BPQEther ");

	//
	//	Read config first, to get UDP info if needed
	//

	Adapter[0]=0;

	if (!ReadConfigFile("BPQETHER.CFG",port))
		return (FALSE);



	/* Open the adapter */
	if ((PCAPInfo[port].adhandle= pcap_open_livex(Adapter,	// name of the device
							 65536,			// portion of the packet to capture. 
											// 65536 grants that the whole packet will be captured on all the MACs.
							 1,				// promiscuous mode (nonzero means promiscuous)
							 1,				// read timeout
							 errbuf			// error buffer
							 )) == NULL)
	{

		n=wsprintf(buf,"Unable to open %s\n",Adapter);
		WritetoConsole(buf);

		/* Free the device list */
		return -1;
	}
	
	/* Check the link layer. We support only Ethernet for simplicity. */
	if(pcap_datalinkx(PCAPInfo[port].adhandle) != DLT_EN10MB)
	{
	
		n=wsprintf(buf,"\nThis program works only on Ethernet networks.\n");
		WritetoConsole(buf);
		
		/* Free the device list */
		return -1;
	}
	
		netmask=0xffffff; 


		sprintf(packet_filter,"ether[12:2]=0x%x",ntohs(PCAPInfo[port].EtherType));


	//compile the filter
	if (pcap_compilex(PCAPInfo[port].adhandle, &fcode, packet_filter, 1, netmask) <0 )
	{	
		n=wsprintf(buf,"\nUnable to compile the packet filter. Check the syntax.\n");
		WritetoConsole(buf);

		
		/* Free the device list */
		return -1;
	}
	
	//set the filter

	if (pcap_setfilterx(PCAPInfo[port].adhandle, &fcode)<0)
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


BOOL ReadConfigFile(char * fn,int Port)
{

//TYPE	1	08FF                              # Ethernet Type
//ETH	1	FF:FF:FF:FF:FF:FF				#	Target Ethernet AddrMAP G8BPQ-7 10.2.77.1                  # IP 93 for compatibility
//ADAPTER	1	\Device\NPF_{21B601E8-8088-4F7D-9629-EDE2A9243CF4}	# Adapter Name

	FILE *file;
	char buf[256],errbuf[256];

	UCHAR Value[MAX_PATH];

	UCHAR * BPQDirectory;

	BPQDirectory=GetBPQDirectory();

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
		n=wsprintf(buf,"Config file %s could not be opened ",Value);
		WritetoConsole(buf);

		return (FALSE);
	}


	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf,Port))
		{
			WritetoConsole("BPQEther bad config record ");
			WritetoConsole(errbuf);
		}
				
			//	MessageBox(NULL,errbuf,"BPQAXIP - Bad config record",MB_OK);
	}
	
	fclose(file);

	return (TRUE);

}


ProcessLine(char * buf,int Port)
{
	char * ptr;
	char * p_port;
	char * p_mac;
	char * p_Adapter;
	char * p_type;

	int	port;
	int a,b,c,d,e,f,num;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if(_stricmp(ptr,"ADAPTER") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			
		p_Adapter = strtok(NULL, " \t\n\r");
		
		strcpy(Adapter,p_Adapter);

		return (TRUE);

	}

	if(_stricmp(ptr,"TYPE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			
		p_type = strtok(NULL, " \t\n\r");
		
		if (p_type == NULL) return (FALSE);

		num=sscanf(p_type,"%x",&a);

		if (num != 1) return FALSE;

		PCAPInfo[Port].EtherType=htons(a);

		return (TRUE);

	}

	if(_stricmp(ptr,"RXMODE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			

		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		if(_stricmp(p_port,"RLI") == 0)
		{
			PCAPInfo[Port].RLIRX=TRUE;
			return (TRUE);
		}

		if(_stricmp(p_port,"BPQ") == 0)
		{
			PCAPInfo[Port].RLIRX=FALSE;
			return (TRUE);
		}

		return FALSE;
	
	}


	if(_stricmp(ptr,"TXMODE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			

		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		if(_stricmp(p_port,"RLI") == 0)
		{
			PCAPInfo[Port].RLITX=TRUE;
			return (TRUE);
		}

		if(_stricmp(p_port,"BPQ") == 0)
		{
			PCAPInfo[Port].RLITX=FALSE;
			return (TRUE);
		}

		return FALSE;

	}

	if(_stricmp(ptr,"DEST") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			
	
		p_mac = strtok(NULL, " \t\n\r");
		
		if (p_mac == NULL) return (FALSE);

		num=sscanf(p_mac,"%x-%x-%x-%x-%x-%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		PCAPInfo[Port].EthDest[0]=a;
		PCAPInfo[Port].EthDest[1]=b;
		PCAPInfo[Port].EthDest[2]=c;
		PCAPInfo[Port].EthDest[3]=d;
		PCAPInfo[Port].EthDest[4]=e;
		PCAPInfo[Port].EthDest[5]=f;


	//	strcpy(Adapter,p_Adapter);

		return (TRUE);

	}

	if(_stricmp(ptr,"SOURCE") == 0)
	{
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		port = atoi(p_port);

		if (Port != port) return TRUE;		// Not for us
			
	
		p_mac = strtok(NULL, " \t\n\r");
		
		if (p_mac == NULL) return (FALSE);

		num=sscanf(p_mac,"%x-%x-%x-%x-%x-%x",&a,&b,&c,&d,&e,&f);

		if (num != 6) return FALSE;

		PCAPInfo[Port].EthSource[0]=a;
		PCAPInfo[Port].EthSource[1]=b;
		PCAPInfo[Port].EthSource[2]=c;
		PCAPInfo[Port].EthSource[3]=d;
		PCAPInfo[Port].EthSource[4]=e;
		PCAPInfo[Port].EthSource[5]=f;


	//	strcpy(Adapter,p_Adapter);

		return (TRUE);

	}
	//
	//	Bad line
	//
	return (FALSE);
	
}
	


