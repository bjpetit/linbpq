
#define VersionString "0.0.0.7"

// 0.2 Allow updating CMSCALL and PASS
//	   Read as binary to preserve line endings

// 0.3 Fix saving Telnet USER Application field
//	   Add software install/update 
//	   Add ARPS Configuration
//	   Add AX/IP MAP line configuration

// 0.4 Minor Fixes

// 0.5 Add Edit Routes page

// 0.6 Fix hang if unknown MAP param specified
//	   Tidy ROUTES header

// 0.7 Add WinRPR and HSModem port types

#include "BPQConfigGen.h"
#ifdef WIN32
#include <Windows.h>
#else

#define TRUE 1
#define FALSE 0

#define strtok_s strtok_r

int memicmp(unsigned char *a, unsigned char *b, int n)
{
	if (n)
	{
		while (n && toupper(*a) == toupper(*b))
			n--, a++, b++;

		if (n)
			return toupper(*a) - toupper(*b);
	}
	return 0;
}
int stricmp(const char * pStr1, const char *pStr2)
{
	unsigned char c1, c2;
	int  v;

	if (pStr1 == NULL)
		return 1;
	
	do {
		c1 = *pStr1++;
		c2 = *pStr2++;
		/* The casts are necessary when pStr1 is shorter & char is signed */
		v = tolower(c1) - tolower(c2);
	} while ((v == 0) && (c1 != '\0') && (c2 != '\0'));

	return v;
}
char * strupr(char* s)
{
	char* p = s;

	if (s == 0)
		return 0;

	while ((*p = toupper(*p))) p++;
	return s;
}

#endif

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;

	return ptr;
}

void portSetVisible(int i, int v);
void applSetVisible(int i, int v);
void aprsSetVisible(int i, int v);

void refreshPorts();
void refreshAppls();
int APRSProcessLine(char * buf);

extern int restartingforupdate;

bool HadConfig = false;		// Set if we've read a config 

#define TCPMaster 1
#define TCPSlave 2

int BBSAppl = 0;
int ChatAppl = 0;
int RMSAppl = 0;
int TelnetPort = 0;

QAction * helpAction;

QString NodeCall;
QString NodeAlias;
QString LOC;

QString CMSCall;
QString CMSPass;
QString User;
QString Pass;
char LoppedCall[80];
int NodeSSID = 0;

QPoint mainpos;

QLineEdit * sysopUser;
QLineEdit * sysopPass;

int RMS;


char * NODECALL = NULL;
char * NODEALIAS = NULL;

char HFCText[82];

int nextPortnum = 0;
int activePortnum = 0;	// May be different from above if PORTNUM used

char ValfromReg[250] = "";

struct PORTREC
{
	QLabel * gPortNum;
	QLabel * gPortName;
	QLabel * gPortType;
	QPushButton * gPortEdit;

	// Fields for APRS - need to be linked to port defintions

	QCheckBox *Enable;
	QLabel *PortNo;
	QLabel *Port;
	QLineEdit *Path;
	QLineEdit *Digi;
	QLineEdit *Bridge;

	char * Pathval;
	char * Digival;
	char * Bridgeval;
	int Use;

	int PortType;

	char ID[32];
	char Param1[32];
	int Param2;
	char Param3[32];
	char * portConfig;	// Malloc'ed Config
};

struct PORTREC Ports[33];


#define H_AEA 0
#define H_ARDOP 1
#define H_AXIP 2
#define H_ETHER 3
#define H_FLDIGI 4
#define H_HAL 5
#define H_KAM 6
#define H_LOOP 7
#define H_KISS 8 
#define H_KISSTCP 9
#define H_KISSUDP 10
#define H_MPSK 11
#define H_SCS 12
#define H_TRK 13
#define H_TRKM 14
#define H_SERIAL 15
#define H_TELNET 16
#define H_UZ7HO 17
#define H_V4 18
//#define H_UIARQ 12
#define H_VARA 19
#define H_VKISS 20
#define H_WINMOR 21
#define H_KISSHF 22
#define H_WINRPR 23
#define H_HSMODEM 24



#define NUMBEROFTYPES 25

char Types[NUMBEROFTYPES][16] =
{
	"AEA Pactor", "ARDOP", "AXIP", "BPQETHER",
	"FLDIGI", "HAL", "KAM Pactor", "LOOPBACK",
	"KISS Serial", "KISS TCP", "KISS UDP", "MULTIPSK", 
	"SCS Pactor", "SCS Tracker","SCS TRKMULTI","Serial",
	"TELNET", "UZ7HO", "V4", "VARA",
	"VKISS", "WINMOR", "KISSHF", "WinRPR", "HSModem"
};

int defaultPort[NUMBEROFTYPES] =
{
	9600, 8515, 0, 0, 0, 9600, 9600, 0,
	9600, 8100, 8100, 0, 38400, 38400, 38400, 9600,
	0, 8000, 8510, 8300, 0, 8500, 8100, 8101, 40131
};

struct APPL
{
	int Num;
	char Command[12];
	char CommandAlias[48];
	char ApplCall[10];
	char ApplAlias[10];
	char L2Alias[10];
	int ApplQual;

};

	QLineEdit * gapplNum[32];
	QLineEdit * gapplCmd[32];
	QLineEdit * gapplCall[32];
	QLineEdit * gapplAlias[32];
	QLineEdit * gapplCmdAlias[32];
	QLineEdit * gapplQual[32];
	QPushButton * gapplEdit[32];


struct APPL Appls[33] = { 0, "", "", "", "", "", 0};

int nextApplNum = 0;

char mainConfig[32768] = "";
char aprsConfig[32768] = "";
char routeConfig[32768];

char blockConfig[10][16384];
char tncportConfig[16384];

int currentBlockType = 0;

// Block config types

#define IPGATEWAY 0 
#define PORTMAPPER 1
//#define ROUTES 2
#define INFOMSG 3
#define IDMSG 4 
#define BTEXT 5 
#define CTEXT 6



// Telnet Server User Record

struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
	char * Appl;				// Autoconnect APPL
	int Secure;				    // SYSOP flag

	QLineEdit * gCallsign;
	QLineEdit * gUserName;
	QLineEdit * gPassword;
	QLineEdit * gAppl;
	QCheckBox * gSYSOP;

};

struct UserRec Users[1024] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int NextUser = 0;


struct OBJECT
{
	char Path[128];		//	Dest, Source and up to 8 digis 
	char Message[81];
	char PortMap[33];
	int	Interval;

	QLabel *label_64;
	QLabel *label_65;
	QLabel *label_66;
	QLabel *label_67;
	QLineEdit *ObjPath;
	QLineEdit *ObjPorts;
	QLineEdit *ObjInterval;
	QLineEdit *ObjText;

};

struct OBJECT Objects[128];		// List of objects to send;

int NextObject = 0;


// axip MAP records

struct MAPRec
{
	char * Callsign;
	char * Host;
	char * Port;
	char * SourcePort;
	int UDP;			// Mode flag Raw, UDP, TCPMaster TCPSlave
	int noBroadcast;

	QLineEdit * gCallsign;
	QLineEdit * gHost;
	QLineEdit * gPort;
	QLineEdit * gSourcePort;
	QComboBox * gUDP;

};

struct MAPRec Maps[128];

int NextMap = 0;

struct ROUTECONFIG
{
	char call[80];		// May have VIA
	int quality;
	int port;
	int pwind;
	int pfrack;
	int ppacl;
	int farQual;
	int INP3;
	int NoKeepAlive;
	QLineEdit *routeCall;
	QLineEdit *routeQual;
	QLineEdit *routePort;
	QLineEdit *routeWindow;
	QLineEdit *routeFrack;
	QLineEdit *routePaclen;
	QLineEdit *routeFarQual;
	QCheckBox *routeINP3;
	QCheckBox *routeNoKeepalives;

};

#define MaxLockedRoutes 100

struct ROUTECONFIG Routes[MaxLockedRoutes];

int NextRoute = 0;

struct NODEPARAM
{
	char param[20];
	char value[20];
	QWidget * widget;
	bool inSimple;
	char simpleDefault[10];
	bool isCheckBox;
};

// !!! Dont re-order this list without changing initNodeParams();
struct NODEPARAM NodeParams[] = {
	{"NODE", "", 0, 1, "1", 1},
	{"BBS", "", 0, 1, "1", 1},
	{"OBSINIT", "", 0, 1, "6", 0},
	{"OBSMIN","", 0, 1, "5", 0},
	{"NODESINTERVAL","", 0, 1, "30", 0},
	{"L3TIMETOLIVE","", 0, 1, "25", 0},
	{"L4RETRIES","", 0, 1, "3", 0},
	{"L4TIMEOUT","", 0, 1, "60", 0},
	{"PACLEN","", 0, 1, "236", 0},
	{"T3","", 0, 1, "180", 0},
	{"IDLETIME","", 0, 1, "900", 0},
	{"MAXLINKS","", 0, 1, "64", 0},
	{"MAXNODES","", 0, 1, "250", 0},
	{"MAXROUTES","", 0, 1, "64", 0},
	{"MAXCIRCUITS","", 0, 1, "128", 0},
	{"MINQUAL","", 0, 1, "150", 0},
	{"HIDENODES","", 0, 1, "1", 1},
	{"L4DELAY","", 0, 1, "10", 0},
	{"L4WINDOW","", 0, 1, "4", 0},
	{"AUTOSAVE","", 0, 1, "1", 1},
	{"MAXRTT","", 0, 1, "90", 0},
	{"MAXHOPS","", 0, 1, "4", 0},
	{"SAVEMH","", 0, 1, "1", 1},
	{"ENABLEADIFLOG","", 0, 0, "", 1},
	{"NETROMCALL","", 0, 0, "", 0},
	{"LogL4Connects","", 0, 0, "", 1},
	{"ENABLE_LINKED","", 0, 1, "A", 0},
};

int NodeParamCount = sizeof(NodeParams) / sizeof(struct NODEPARAM);

QScrollArea *userscrollArea;
QWidget *userScroll;
QLabel * UserLabel;

QWidget * placeHolder;

char programPath[128] = "";
char configPath[128] = "";

char buildCPU[256] = "";
char thisCPU[256] = "";
char thisKernel[256] = "";
char thisProduct[256] = "";

int ARM = 0;
int ARM64 = 0;
int Intel32 = 0;
int Intel64 = 0;
int Windows = 0;
int Linux = 0;

void RemoveExtraBlankLines(char * msg)
{
	char * ptr;

	while ((ptr = strstr(msg, "\r\n\r\n\r\n")))
		memmove(ptr, ptr + 2, strlen(ptr) - 1);

	while ((ptr = strstr(msg, "\n\n\n")))
		memmove(ptr, ptr + 1, strlen(ptr));

}

void BPQConfigGen::initNodeParams()
{
	struct NODEPARAM *NodeParam = &NodeParams[0];

	NodeParam->widget = (QWidget *)ui.Node; NodeParam++;
	NodeParam->widget = (QWidget *)ui.BBS_2; NodeParam++;
	NodeParam->widget = (QWidget *)ui.ObsInit; NodeParam++;
	NodeParam->widget = (QWidget *)ui.ObsMin; NodeParam++;
	NodeParam->widget = (QWidget *)ui.NodesInterval; NodeParam++;
	NodeParam->widget = (QWidget *)ui.L3TTL; NodeParam++;
	NodeParam->widget = (QWidget *)ui.L4Retries; NodeParam++;
	NodeParam->widget = (QWidget *)ui.L4Timeout; NodeParam++;
	NodeParam->widget = (QWidget *)ui.Paclen; NodeParam++;
	NodeParam->widget = (QWidget *)ui.T3; NodeParam++;
	NodeParam->widget = (QWidget *)ui.IdleTime; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxLinks; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxNodes; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxRoutes; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxCircuits; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MinQual; NodeParam++;
	NodeParam->widget = (QWidget *)ui.HideNodes; NodeParam++;
	NodeParam->widget = (QWidget *)ui.L4Delay; NodeParam++;
	NodeParam->widget = (QWidget *)ui.L4Window; NodeParam++;
	NodeParam->widget = (QWidget *)ui.AutoSave; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxRTT; NodeParam++;
	NodeParam->widget = (QWidget *)ui.MaxHops_2; NodeParam++;
	NodeParam->widget = (QWidget *)ui.SaveMH; NodeParam++;
	NodeParam->widget = (QWidget *)ui.EnableADIFLog; NodeParam++;
	NodeParam->widget = (QWidget *)ui.NETROMCall; NodeParam++;
	NodeParam->widget = (QWidget *)ui.LogL4Connects; NodeParam++;
	NodeParam->widget = (QWidget *)ui.EnableLinked; NodeParam++;
}

BPQConfigGen::BPQConfigGen(QWidget *parent)
	: QMainWindow(parent)
{
	int i;
	char Label[10];
	char Title[80];
	QSysInfo systemInfo;

	ui.setupUi(this);
	initNodeParams();

/*	qDebug("##### System Information #####");
	qDebug() << "Windows Version: " << systemInfo.windowsVersion();
	qDebug() << "Build Cpu Architecture: " << systemInfo.buildCpuArchitecture();
	qDebug() << "Current Cpu Architecture: " << systemInfo.currentCpuArchitecture();
	qDebug() << "Kernel Type: " << systemInfo.kernelType();
	qDebug() << "Kernel Version: " << systemInfo.kernelVersion();
	qDebug() << "Product Type: " << systemInfo.productType();
	qDebug() << "Product Version: " << systemInfo.productVersion();
	qDebug() << "Byte Order: " << systemInfo.buildAbi();
	qDebug() << "Pretty ProductName: " << systemInfo.prettyProductName();	qDebug("#####");
*/

	strcpy(buildCPU, systemInfo.buildCpuArchitecture().toUtf8());
	strcpy(thisCPU, systemInfo.currentCpuArchitecture().toUtf8());
	strcpy(thisKernel, systemInfo.kernelType().toUtf8());
	strcpy(thisProduct, systemInfo.productType().toUtf8());

//	qDebug() << buildCPU;
//	qDebug() << thisCPU;
//	qDebug() << thisKernel;
//	qDebug() << thisProduct;

	if (strcmp(thisCPU, "x86_64") == 0)
		Intel64 = 1;
	else if (strcmp(thisCPU, "arm") == 0)
		ARM = 1;
	else if (strcmp(thisCPU, "arm64") == 0)
		ARM64 = 1;

	QSettings settings("G8BPQ", "BPQConfigGen");

	strcpy(configPath, settings.value("configPath", "").toString().toUtf8());
	strcpy(programPath, settings.value("programPath", "").toString().toUtf8());

	if (configPath[0] == 0 || programPath[0] == 0)
	{
		// Get default BPQ Directory

#ifdef Q_OS_WIN32

		HKEY REGTREE = HKEY_CURRENT_USER;

		HKEY hKey = 0;
		HKEY hKeyIn = 0;
		HKEY hKeyOut = 0;
		int disp;
		int retCode;
		DWORD Type, Vallen = MAX_PATH;
		char msg[512];
		char DLLName[256] = "Not Known";
		char LogDir[256];

		retCode = RegOpenKeyExA(REGTREE,
			"SOFTWARE\\G8BPQ\\BPQ32",
			0,
			KEY_QUERY_VALUE,
			&hKey);

		if (retCode == ERROR_SUCCESS)
		{
			// Get Program Directory

			Vallen = MAX_PATH;
			retCode = RegQueryValueExA(hKey, "BPQ Program Directory", 0,
				&Type, (UCHAR *)&ValfromReg, &Vallen);

			if (retCode == ERROR_SUCCESS)
			{
				if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
					ValfromReg[0] = 0;
			}

			strcpy(programPath, ValfromReg);

			// Get "BPQ Directory"

			Vallen = MAX_PATH;
			retCode = RegQueryValueExA(hKey, "BPQ Directory", 0,
				&Type, (UCHAR *)&ValfromReg, &Vallen);

			if (retCode == ERROR_SUCCESS)
			{
				if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
					ValfromReg[0] = 0;
			}

			if (ValfromReg[0] == 0)
			{
				// BPQ Directory absent or = "" - try "Config File Location"

				Vallen = MAX_PATH;

				retCode = RegQueryValueExA(hKey, "Config File Location", 0,
					&Type, (UCHAR *)&ValfromReg, &Vallen);

				if (retCode == ERROR_SUCCESS)
				{
					if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
						ValfromReg[0] = 0;
				}
			}
		}

		strcpy(configPath, ValfromReg);
#else
		strcpy(configPath, QDir::homePath().toUtf8());
		strcat(configPath, "/linbpq");
		strcpy(programPath, configPath);
#endif

	}


	ui.ConfigDir->setText(configPath);
	ui.InstallDir->setText(programPath);

	// get local copies of some controls

	sysopUser = ui.sysopUser;
	sysopPass = ui.sysopPass;

	sprintf(Title, "Simple BPQ Config File Generator Version %s", VersionString);
	this->setWindowTitle(Title);

	placeHolder = new QWidget();

	// Set up port list

	memset(Ports, 0, sizeof(Ports));

	for (i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		sprintf(Label, "%d", i + 1);
		PORT->gPortNum = new QLabel(Label, ui.portScroll);
		PORT->gPortNum->setGeometry(QRect(5, 22 * i + 5, 20, 18));

		PORT->gPortType = new QLabel(ui.portScroll);
		PORT->gPortType->setGeometry(QRect(25, 22 * i + 5, 100, 18));

		PORT->gPortName = new QLabel(ui.portScroll);
		PORT->gPortName->setGeometry(QRect(130, 22 * i + 5, 250, 18));

		PORT->gPortEdit = new QPushButton("Edit", ui.portScroll);
		PORT->gPortEdit->setGeometry(QRect(385, 22 * i + 5, 60, 18));
		connect(PORT->gPortEdit, SIGNAL(clicked()), this, SLOT(clickedSlot()));

		PORT->Bridge = new QLineEdit(ui.scrollAreaWidgetContents);
		PORT->Bridge->setObjectName(QString::fromUtf8("lineEdit_4"));
		PORT->Bridge->setGeometry(QRect(505, 22 * i + 5, 56, 18));
		PORT->Path = new QLineEdit(ui.scrollAreaWidgetContents);
		PORT->Path->setObjectName(QString::fromUtf8("Path"));
		PORT->Path->setGeometry(QRect(225, 22 * i + 5, 191, 18));
		PORT->Digi = new QLineEdit(ui.scrollAreaWidgetContents);
		PORT->Digi->setObjectName(QString::fromUtf8("Digi"));
		PORT->Digi->setGeometry(QRect(425, 22 * i + 5, 71, 18));
		PORT->PortNo = new QLabel(ui.scrollAreaWidgetContents);
		PORT->PortNo->setObjectName(QString::fromUtf8("PortNo"));
		PORT->PortNo->setGeometry(QRect(25, 22 * i + 5, 20, 20));
		PORT->Port = new QLabel(ui.scrollAreaWidgetContents);
		PORT->Port->setObjectName(QString::fromUtf8("Port"));
		PORT->Port->setGeometry(QRect(50, 22 * i + 5, 170, 20));
		PORT->Enable = new QCheckBox(ui.scrollAreaWidgetContents);
		PORT->Enable->setObjectName(QString::fromUtf8("Enable"));
		PORT->Enable->setGeometry(QRect(5, 22 * i + 5, 21, 18));

		portSetVisible(i, 0);
	}



	ui.APRSOuterscrollAreaWidgetContents->setGeometry(0, 0, 585, 620);

	ui.scrollAreaWidgetContents->setGeometry(0, 0, 563, 22 * 33);

	for (i = 0; i < 32; i++)
	{
		sprintf(Label, "%d", i + 1); 
		gapplNum[i] = new QLineEdit(Label, ui.applScroll);
		gapplNum[i]->setGeometry(QRect(5, 22 * i + 5, 20, 18));
		gapplCmd[i] = new QLineEdit(ui.applScroll);
		gapplCmd[i]->setGeometry(QRect(25, 22 * i + 5, 100, 18));
		gapplCmdAlias[i] = new QLineEdit(ui.applScroll);
		gapplCmdAlias[i]->setGeometry(QRect(130, 22 * i + 5, 100, 18));
		gapplCall[i] = new QLineEdit(ui.applScroll);
		gapplCall[i]->setGeometry(QRect(235, 22 * i + 5, 100, 18));
		gapplAlias[i] = new QLineEdit(ui.applScroll);
		gapplAlias[i]->setGeometry(QRect(340, 22 * i + 5, 100, 18));
		gapplQual[i] = new QLineEdit(ui.applScroll);
		gapplQual[i]->setGeometry(QRect(445, 22 * i + 5, 30, 18));
		gapplEdit[i] = new QPushButton("Save", ui.applScroll);
		gapplEdit[i]->setGeometry(QRect(480, 22 * i + 5, 60, 18));
		connect(gapplEdit[i], SIGNAL(clicked()), this, SLOT(clickedSlot()));

		applSetVisible(i, 0);
	}

	// APRS Objects

	for (i = 0; i < 128; i++)
	{
		struct OBJECT * Object = &Objects[i];

		Object->label_64 = new QLabel("Path", ui.ObjectscrollAreaContents);
		Object->label_64->setObjectName(QString::fromUtf8("label_64"));
		Object->label_64->setGeometry(QRect(10, 5 + i * 50, 51, 20));
		Object->label_65 = new QLabel("Interval", ui.ObjectscrollAreaContents);
		Object->label_65->setObjectName(QString::fromUtf8("label_65"));
		Object->label_65->setGeometry(QRect(340, 5 + i * 50, 55, 20));
		Object->label_66 = new QLabel("Ports", ui.ObjectscrollAreaContents);
		Object->label_66->setObjectName(QString::fromUtf8("label_66"));
		Object->label_66->setGeometry(QRect(205, 5 + i * 50, 46, 20));
		Object->label_67 = new QLabel("Text", ui.ObjectscrollAreaContents);
		Object->label_67->setObjectName(QString::fromUtf8("label_67"));
		Object->label_67->setGeometry(QRect(10, 27 + i * 50, 46, 20));
		Object->ObjPath = new QLineEdit(ui.ObjectscrollAreaContents);
		Object->ObjPath->setObjectName(QString::fromUtf8("ObjPath"));
		Object->ObjPath->setGeometry(QRect(50, 5 + i * 50, 146, 20));
		Object->ObjPorts = new QLineEdit(ui.ObjectscrollAreaContents);
		Object->ObjPorts->setObjectName(QString::fromUtf8("ObjPorts"));
		Object->ObjPorts->setGeometry(QRect(250, 5 + i * 50, 81, 20));
		Object->ObjInterval = new QLineEdit(ui.ObjectscrollAreaContents);
		Object->ObjInterval->setObjectName(QString::fromUtf8("ObjInterval"));
		Object->ObjInterval->setGeometry(QRect(395, 5 + i * 50, 36, 20));
		Object->ObjText = new QLineEdit(ui.ObjectscrollAreaContents);
		Object->ObjText->setObjectName(QString::fromUtf8("ObjText"));
		Object->ObjText->setGeometry(QRect(50, 27 + i * 50, 491, 20));

		aprsSetVisible(i, 0);
	}

	aprsSetVisible(0, 1);				// Show blanks for user to add one or two 
	aprsSetVisible(1, 1);

	ui.ObjectscrollAreaContents->setGeometry(0, 0, 563, 50 * 129);

	// Routes dialog

	for (i = 0; i < MaxLockedRoutes; i++)
	{
		struct ROUTECONFIG * Route = &Routes[i];
		Route->routeCall = new QLineEdit(ui.routeScrollareaContents);
		Route->routeCall->setGeometry(QRect(5, 5 + i * 22, 196, 20));
		Route->routeQual = new QLineEdit(ui.routeScrollareaContents);
		Route->routeQual->setGeometry(QRect(255, 5 + i * 22, 31, 20));
		Route->routePort = new QLineEdit(ui.routeScrollareaContents);
		Route->routePort->setGeometry(QRect(215, 5 + i * 22, 24, 20));
		Route->routeWindow = new QLineEdit(ui.routeScrollareaContents);
		Route->routeWindow->setGeometry(QRect(305, 5 + i * 22, 24, 20));
		Route->routeFrack = new QLineEdit(ui.routeScrollareaContents);
		Route->routeFrack->setGeometry(QRect(340, 5 + i * 22, 36, 20));
		Route->routePaclen = new QLineEdit(ui.routeScrollareaContents);
		Route->routePaclen->setGeometry(QRect(380, 5 + i * 22, 32, 20));
		Route->routeFarQual = new QLineEdit(ui.routeScrollareaContents);
		Route->routeFarQual->setGeometry(QRect(420, 5 + i * 22, 32, 20));
		Route->routeINP3 = new QCheckBox(ui.routeScrollareaContents);
		Route->routeINP3->setGeometry(QRect(470, 5 + i * 22, 20, 20));
		Route->routeNoKeepalives = new QCheckBox(ui.routeScrollareaContents);
		Route->routeNoKeepalives->setGeometry(QRect(510, 5 + i * 22, 20, 20));
	}


	connect(ui.ReadConfig, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.Update, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.ADDPORT, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.ADDAPP, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.EDITOTHER, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.SAVEAPRS, SIGNAL(clicked()), this, SLOT(clickedSlot()));

	connect(ui.WEBMGMT, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.BBS, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.CHAT, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.RMS, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.EnableAPRS, SIGNAL(clicked()), this, SLOT(clickedSlot()));

	helpAction = ui.menuBar->addAction("Help");
	helpAction->setObjectName(QString::fromUtf8("HELP"));
	connect(helpAction, SIGNAL(triggered()), this, SLOT(clickedSlot()));

	connect(ui.UpdateRelease, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.UpdateBeta, SIGNAL(clicked()), this, SLOT(clickedSlot()));

	connect(ui.saveIPGateway, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.saveInfoMsg, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.saveNodePage, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.saveTNCPort, SIGNAL(clicked()), this, SLOT(clickedSlot()));
//	connect(ui.saveCtext, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	connect(ui.saveRoutes, SIGNAL(clicked()), this, SLOT(clickedSlot()));

	ui.OS->setText(thisKernel);
	ui.Arch->setText(thisCPU);

	getVersions();

	if (restartingforupdate)
	{
		// re=enter update dialog

		ui.tabWidget->setCurrentIndex(0);
		doUpdate(1);	// Beta
	}
}

void BPQConfigGen::resizeEvent(QResizeEvent *e)
{
	int window_height = BPQConfigGen::size().height();
	int window_width = BPQConfigGen::size().width();

	ui.scrollArea_3->setGeometry(QRect(0, 0, window_width, window_height - 30));
}

void createTelnetConfig(int p)
{
	struct PORTREC * PORT = &Ports[p];

	PORT->PortType = H_TELNET;
	strcpy(PORT->ID, "Telnet Server");

	char * ptr = PORT->portConfig = (char *)malloc(32768);

	TelnetPort = ++p;

	ptr += sprintf(ptr, "PORT\r\n");
	ptr += sprintf(ptr, "PORTNUM=%d\r\n", p);
	ptr += sprintf(ptr, " ID=Telnet Server\r\n");
	ptr += sprintf(ptr, " DRIVER=TELNET\r\n");
	ptr += sprintf(ptr, " CONFIG\r\n");
	ptr += sprintf(ptr, "  LOGGING=1\r\n");
	ptr += sprintf(ptr, "  TCPPORT=8010\r\n");
	ptr += sprintf(ptr, "  FBBPORT=8011\r\n");
	ptr += sprintf(ptr, "  HTTPPORT=8080\r\n");

	ptr += sprintf(ptr, "ENDPORT\r\n");
	ptr += sprintf(ptr, "\r\n");
}

void BPQConfigGen::clickedSlot()
{
	QPushButton * Button = static_cast<QPushButton*>(QObject::sender());
	char Name[32];

	mainpos = this->pos();				// for popup windows

	strcpy(Name, sender()->objectName().toUtf8());

	if (strcmp(Name, "HELP") == 0)
	{
		QMessageBox::about(this, tr("Help"),
			"BPQConfigGen is a program to create or edit a BPQ Node configuration file(bpq32.cfg).A minimal configuration can be created by entering a Node Callsign and adding at least one port.All LinBPQ users and most Windows users will want a Telnet port for "
			"management.This can be added by setting a username and password and enabling the Web Mgmt Interface.If you want them you can then enable the BBS, CHAT Server or Winlink Gateway by checking the appropriate box.You may want to customise the Application "
			"definitions by changing the callsign or setting a NETROM Alias and Quality.Other applications can be added using the \"Add Application\" button and filling in the details."
			"<br><br>"
			"BPQConfigGen only allows you to create or edit the basic Node identification info and your ports and application definitions.Other BPQ features, such as APRS, can be added using the \"Edit Main Config\" button and entering the necessary config statements."
			"<br><br>"
			"Once you have a config file you can edit it by using the \"Read Existing Config\" button.This can also read configs not created using BPQConfigGen.If that is the case you will get a warning message that the config may not be read correctly, but normally it will be fine."
			"<br><br>"
			"If you read an existing config file comments will normally be kept, but the order of configuration sections may be changed.The new file will have the Port and Application definitions at the end."
			"<br><br>"
			"Before a file is updated a copy of the original config is saved with a date / time stamp appended.There is no housekeeping of these - you will need to delete any you no longer need.");

		return;
	}

	if (strcmp(Name, "ReadConfig") == 0)
	{
		ReadConfigFile();
		return;
	}

	if (strcmp(Name, "UpdateRelease") == 0)
	{
		doUpdate(0);	// Release
		return;
	}

	if (strcmp(Name, "UpdateBeta") == 0)
	{
		doUpdate(1);	// Beta
		return;
	}

	GetParams();

	// Make sure NodeCall is set before we do anything

	if (NODECALL == 0 || NODECALL[0] == 0)
	{
		NodeCall = ui.NODECALL->text().toUpper();
		NODECALL = strdup(NodeCall.toLocal8Bit().constData());

		strcpy(LoppedCall, NodeCall.toLocal8Bit().constData());

		char * ptr = strchr(LoppedCall, '-');
		if (ptr)
		{
			*ptr++ = 0;
			NodeSSID = atoi(ptr);
		}

		if (NODECALL == 0 || NODECALL[0] == 0)
		{
			QMessageBox msgBox;

			msgBox.setWindowTitle("Error");
			msgBox.setText("Please Set NODECALL before configuring");
			msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
			msgBox.exec();

			Button->setChecked(false);

			return;
		}
	}

	if (strcmp(Name, "ADDPORT") == 0)
	{
		// This runs the Add Port dialog
		
		GetParams();			// Nodecall, etc

		PortDialog dialog(0);
		dialog.exec();

		refreshAPRS();

 		return;
	}

	if (strcmp(Name, "ADDAPP") == 0)
	{
		applSetVisible(nextApplNum++, 1);
		return;
	}

	if (strcmp(Name, "EDITOTHER") == 0)
	{
		editOtherDialog dialog(0);
		dialog.exec();

		return;
	}
	if (strcmp(Name, "SAVEAPRS") == 0)
	{
		SaveAPRS();
		return;
	}

	if (strcmp(Name, "BBS") == 0)
	{
		// Add BBS

		APPL * pAppl = &Appls[nextApplNum++];

		BBSAppl = pAppl->Num = 1;
		strcpy(pAppl->Command, "BBS");

		strcpy(pAppl->ApplCall, LoppedCall);

		if (strchr(NODECALL, '-') == 0)
			strcat(pAppl->ApplCall, "-1");

		//		strcpy(pAppl->ApplAlias, ApplA);

		pAppl->ApplQual = 0;

		Button->setEnabled(false);

		refreshAppls();

		return;
	}

	if (strcmp(Name, "CHAT") == 0)
	{
		APPL * pAppl = &Appls[nextApplNum++];

		BBSAppl = pAppl->Num = 2;
		strcpy(pAppl->Command, "CHAT");

		strcpy(pAppl->ApplCall, LoppedCall);

		if (NodeSSID == 4)
			strcat(pAppl->ApplCall, "-2");
		else
			strcat(pAppl->ApplCall, "-4");

		//		strcpy(pAppl->ApplAlias, ApplA);

		pAppl->ApplQual = 0;

		Button->setEnabled(false);

refreshAppls();

return;
	}

	if (strcmp(Name, "RMS") == 0)
	{
		APPL * pAppl = &Appls[nextApplNum++];

		if (TelnetPort == 0)
		{
			createTelnetConfig(nextPortnum++);
			refreshPorts();
		}

		BBSAppl = pAppl->Num = 3;
		strcpy(pAppl->Command, "RMS");

		strcpy(pAppl->ApplCall, LoppedCall);

		if (NodeSSID == 10)
			strcat(pAppl->ApplCall, "-11");
		else
			strcat(pAppl->ApplCall, "-10");

		sprintf(pAppl->CommandAlias, "C %d CMS", TelnetPort);
		//		strcpy(pAppl->ApplAlias, ApplA);

		pAppl->ApplQual = 0;

		Button->setEnabled(false);

		refreshAppls();

		return;
	}

	if (strcmp(Name, "WEBMGMT") == 0)
	{
		if (User.length() == 0 || Pass.length() == 0)
		{
			QMessageBox msgBox;

			msgBox.setWindowTitle("Error");
			msgBox.setText("Please Set User Name and Password before enabling Web Management interface");
			msgBox.move(mainpos.x() + 50, mainpos.y() + 300);
			msgBox.exec();

			Button->setChecked(false);
			return;
		}

		if (TelnetPort == 0)
		{
			createTelnetConfig(nextPortnum++);
			refreshPorts();
		}
		Button->setEnabled(false);
		return;
	}

	if (strcmp(Name, "Update") == 0)
	{
		CreateConfig();
		return;
	}

	if (strcmp(Name, "EnableAPRS") == 0)
	{
		ui.APRSTab->setEnabled(ui.EnableAPRS->isChecked());
		return;
	}

	if (strcmp(Name, "saveIPGateway") == 0)
	{
		saveConfigBlock(ui.IPGateway, IPGATEWAY);
		return;
	}

	if (strcmp(Name, "saveInfoMsg") == 0)
	{
		saveConfigBlock(ui.InfoMsg, INFOMSG);
		saveConfigBlock(ui.IDMsg, IDMSG);
		saveConfigBlock(ui.BText, BTEXT);
		saveConfigBlock(ui.CText, CTEXT);
		return;
	}

	if (strcmp(Name, "saveNodePage") == 0)
	{
		struct NODEPARAM * NodeParam;
		
		for (int i = 0; i < NodeParamCount; i++)
		{
			NodeParam = &NodeParams[i];

			QCheckBox * checkbox = (QCheckBox *)NodeParam->widget;
			QLineEdit * lineedit = (QLineEdit *)NodeParam->widget;

			if (checkbox)
			{
				if (NodeParam->isCheckBox)
				{
					if (checkbox->isChecked())
						strcpy(NodeParam->value, "1");
					else
						strcpy(NodeParam->value, "0");
				}
				else
				{
					memcpy(NodeParam->value, lineedit->text().toUtf8().data(), 19);
				}
			}
		}

		// Remind user to save config

		QMessageBox msgBox;
		msgBox.setWindowTitle("Save");
		msgBox.setText("Node Parameters saved. Remember to write the configuration to apply changes");
		msgBox.exec();

		ui.tabWidget->setCurrentIndex(1);	// Back to main tab

		return;
	}






	if (strcmp(Name, "saveRoutes") == 0)
	{
		int i;
		struct ROUTECONFIG * Route;

		for (i = 0; i < MaxLockedRoutes; i++)
		{
			Route = &Routes[i];

			strcpy(Route->call, Route->routeCall->text().toUtf8().toUpper());
			Route->port = Route->routePort->text().toInt();
			Route->quality = Route->routeQual->text().toInt();
			Route->pwind = Route->routeWindow->text().toInt();
			Route->pfrack = Route->routeFrack->text().toInt();
			Route->ppacl = Route->routePaclen->text().toInt();
			Route->farQual = Route->routeFarQual->text().toInt();
			Route->INP3 = Route->routeINP3->isChecked();
			Route->NoKeepAlive = Route->routeNoKeepalives->isChecked();
		}

		//  ?? also save comment block ??

		strcpy(routeConfig, ui.Routes->toPlainText().toLocal8Bit().data());

		// Remind user to save config

		QMessageBox msgBox;
		msgBox.setWindowTitle("Save");
		msgBox.setText("ROUTES saved. Remember to write the configuration to apply changes");
		msgBox.exec();

		ui.tabWidget->setCurrentIndex(1);	// Back to main tab
		return;
	}

	// See if Save Port

	int p = 0;

	for (int i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		if (Button == PORT->gPortEdit)
		{
			editPort(p);
			return;
		}

		p++;

		while (&Ports[p].portConfig == nullptr && p < 33)
			p++;
	}

	for (int i = 0; i < 32; i++)
	{
		if (Button == gapplEdit[i])
		{
			saveAppl(i);
			return;
		}
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("MessageBox Title");
	msgBox.setText("You Clicked " + ((QPushButton*)sender())->objectName());
	msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
	msgBox.exec();
}

void BPQConfigGen::saveConfigBlock(QPlainTextEdit * pt, int config)
{
	QString qs = pt->toPlainText();
	QByteArray qb = qs.toLocal8Bit();

	memcpy(blockConfig[config], qb.data(), qb.length());
	blockConfig[config][qb.length()] = 0;
	return;
}
	
void BPQConfigGen::datestampedbackup(char * FN)
{
	// Save 6 backups
	
	char FN1[256];
	QFile File(FN);

	if (File.exists())
	{
		QDateTime DateTime = QDateTime::currentDateTime();
		QString Time = DateTime.toString("yyyyMMddHHmmss");
		QByteArray ba = Time.toLatin1();
		sprintf(FN1, "%s.%s", FN, ba.data());
		File.rename(FN, FN1);
	}
}

void BPQConfigGen::ReadConfigFile()
{
	char FN[260];
	FILE * fp;
	FILE * savefp = nullptr;
	int i;

	char line[512];
	char linecopy[512];

	char * Context;
	bool Comment = false;
	bool inPort = false;		// parsing a PORT section
	bool inTNCPort = false;		// parsing a TNCPORT section
	bool inBlock = false;		// parsing a section ending in ***
	bool inAPRS = false;
	bool inRoutes = false;
	
	bool gotUser = false;

	bool keepComments = ui.keepComments->isChecked();

	char tempPort[32768] = "";

	mainConfig[0] = 0;
	aprsConfig[0] = 0;
	routeConfig[0] = 0;

	memset(blockConfig, 0, sizeof(blockConfig));
	tncportConfig[0] = 0;

	nextPortnum = 0;
	activePortnum = 0;
	nextApplNum = 0;
	NextUser = 0;
	NextMap = 0;
	NextObject = 0;
	NextRoute = 0;

	for (int i = 0; i < 128; i++)
	{
		aprsSetVisible(i, 0);
	}

	aprsSetVisible(0, 1);
	aprsSetVisible(1, 1);

	for (int i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		portSetVisible(i, 0);

		if (PORT->portConfig)
		{
			free(PORT->portConfig);
			PORT->portConfig = nullptr;
		}

		Appls[i].Num = 0;
		Appls[i].Command[0] = 0;
	}

	ui.BBS->setEnabled(true);
	ui.CHAT->setEnabled(true);
	ui.RMS->setEnabled(true);
	ui.WEBMGMT->setEnabled(true);
	ui.BBS->setChecked(false);
	ui.CHAT->setChecked(false);
	ui.RMS->setChecked(false);
	ui.WEBMGMT->setChecked(false);

	QString str = ui.ConfigDir->text();

	QSettings settings("G8BPQ", "BPQConfigGen");

	strcpy(configPath, str.toUtf8());

	settings.setValue("configPath", configPath);


	sprintf(FN, "%s/bpq32.cfg", configPath);

	if ((fp = fopen(FN, "rb")) == NULL)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Error");
		msgBox.setText("Failed to open bpq32.cfg");
		msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
		msgBox.exec();
		return;
	}

	// Read first line and check that writted by us

	if (fgets(line, 512, fp) == NULL)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Error");
		msgBox.setText("Empty bpq32.cfg");
		msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
		msgBox.exec();
		return;
	}

	if (strstr(line, "; Written by BPQConfigGen") == 0)
	{
		// Warn
		int ret = QMessageBox::warning(this, tr("Warning!!"),
			tr("File not written by BPQConfigGen. You can continue but file may not"
				" be read correctly and may be corrupted if saved"),
			QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

		if (ret == QMessageBox::Cancel)
			return;
	}

	HadConfig = true;		// we've read a config 

	while (1)
	{
		char * ret;

		ret = fgets(line, 512, fp);

		strlop(line, '\r');
		strlop(line, '\n');
		strcat(line, "\r\n");			// Ensure consistent line endings

		strcpy(linecopy, line);			// Save original

		if (ret == NULL)
		{
			if (savefp)
			{
				// we have reached eof on an include file - switch back

				fclose(fp);
				fp = savefp;
				savefp = NULL;
				continue;
			}

			break;		// end of config
		}

		char * ptr;

		ptr = strtok_s(line, " =\t\r\n", &Context);

		// Try keeping comments

		if (ptr == NULL || ptr[0] == 0 || ptr[0] == ';')
		{
			// Comment

			// try to trap lines with just a space or tab on front

			if (strlen(linecopy) == 3)
			{
				if (linecopy[0] == ' ' || linecopy[0] == 9)
					strcpy(linecopy, "\r\n");
			}

			if (!keepComments)
				continue;

			if (inPort)
				strcat(tempPort, linecopy);
			else if (inAPRS)
				strcat(aprsConfig, linecopy);
			else if (inRoutes)
				strcat(routeConfig, linecopy);
			else if (inBlock)
				strcat(blockConfig[currentBlockType], linecopy);
			else
				strcat(mainConfig, linecopy);

			continue;
		}

		if (strlen(ptr) > 1)
		{
			if (memcmp(ptr, "/*", 2) == 0)
			{
				Comment = TRUE;

				if (!keepComments)
					continue;

				if (inPort)
					strcat(tempPort, linecopy);
				else if (inAPRS)
					strcat(aprsConfig, linecopy);
				else if (inRoutes)
					strcat(routeConfig, linecopy);
				else if (inBlock)
					strcat(blockConfig[currentBlockType], linecopy);
				else
					strcat(mainConfig, linecopy);

				continue;
			}
			else if (memcmp(ptr, "*/", 2) == 0)
			{
				Comment = FALSE;

				if (!keepComments)
					continue;

				if (inPort)
					strcat(tempPort, linecopy);
				else if (inAPRS)
					strcat(aprsConfig, linecopy);
				else if (inRoutes)
					strcat(routeConfig, linecopy);
				else if (inBlock)
					strcat(blockConfig[currentBlockType], linecopy);
				else
					strcat(mainConfig, linecopy);
	
				continue;
			}
		}

		if (Comment)
		{
			if (!keepComments)
				continue;

			if (inPort)
				strcat(tempPort, linecopy);
			else if (inAPRS)
				strcat(aprsConfig, linecopy);
			else if (inRoutes)
				strcat(routeConfig, linecopy);
			else if (inBlock)
				strcat(blockConfig[currentBlockType], linecopy);
			else
				strcat(mainConfig, linecopy);

			continue;
		}

		strlop(Context, ';');

		if (stricmp(ptr, "#include") == 0)
		{
			savefp = fp;

			strlop(Context, 10);
			strlop(Context, 13);

			sprintf(FN, "%s/%s", configPath, Context);

			if ((fp = fopen(FN, "r")) == NULL)
			{
				QMessageBox msgBox;
				msgBox.setWindowTitle("Failed to open include file");
				msgBox.setText(FN);
				msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
				msgBox.exec();

				fp = savefp;
				savefp = NULL;
			}

			continue;			// get next line
		}

		if (inAPRS)
		{
			char aprscopy[512];

			strcpy(aprscopy, linecopy);

			if (APRSProcessLine(aprscopy))
				continue;

			if (memcmp(ptr, "***", 3) == 0)
			{
				inAPRS = FALSE;
				RemoveExtraBlankLines(aprsConfig);
			}
			else
				strcat(aprsConfig, linecopy);


			continue;
		}

		if (inRoutes)
		{
			if (memcmp(ptr, "***", 3) == 0)
			{
				inRoutes = FALSE;
				continue;
			}

			struct ROUTECONFIG * Route = &Routes[NextRoute];

			char Param[8][256];
			char * ptr1, *ptr2;
			int n = 0, inp3 = 0;

			// strtok and sscanf can't handle successive commas, so split up usig strchr

			memset(Param, 0, 2048);

			while ((ptr1 = strchr(ptr, 9)))
				*ptr1 = ' ';

			ptr1 = ptr;

			while (ptr1 && *ptr1 && n < 8)
			{
				ptr2 = strchr(ptr1, ',');
				if (ptr2) *ptr2++ = 0;

				strcpy(&Param[n++][0], ptr1);
				ptr1 = ptr2;
				while (ptr1 && *ptr1 && *ptr1 == ' ')
					ptr1++;
			}

			strcpy(Route->call, &Param[0][0]);

			Route->quality = atoi(Param[1]);
			Route->port = atoi(Param[2]);
			Route->pwind = atoi(Param[3]);
			Route->pfrack = atoi(Param[4]);
			Route->ppacl = atoi(Param[5]);
			inp3 = atoi(Param[6]);
			Route->farQual = atoi(Param[7]);

			// Use top bit of window as INP3 Flag, next as NoKeepAlive

			if (inp3 & 1)
				Route->INP3 |= 1;

			if (inp3 & 2)
				Route->NoKeepAlive |= 1;

			Route->routeCall->setText(Route->call);
			Route->routePort->setText(Param[2]);
			Route->routeQual->setText(Param[1]);
			Route->routeWindow->setText(Param[3]);
			Route->routeFrack->setText(Param[4]);
			Route->routePaclen->setText(Param[5]);
			Route->routeINP3->setChecked(Route->INP3);
			Route->routeNoKeepalives->setChecked(Route->NoKeepAlive);
			Route->routeFarQual->setText(Param[7]);

			if (NextRoute < MaxLockedRoutes)
				NextRoute++;

			continue;
		}


		if (inBlock)
		{
			if (memcmp(ptr, "***", 3) == 0)
				inBlock = FALSE;

			strcat(blockConfig[currentBlockType], linecopy);
			continue;
		}

		if (inTNCPort)
		{
			strcat (tncportConfig, linecopy);
			if (memcmp(ptr, "ENDPORT", 7) == 0)
				inTNCPort = FALSE;

			continue;
		}


		if (inPort)
		{
			struct PORTREC * PORT = &Ports[activePortnum];

			// Dont add USER records to port config so don't add to tempPort

			if (PORT->PortType == H_TELNET)
			{
				if (stricmp(ptr, "USER") == 0)
				{
					struct UserRec * USER = &Users[NextUser];
					char *User, *Pwd, *UserCall, *Secure, *Appl;
					char Param[8][256];
					char * ptr1, *ptr2;
					int n = 0;

					// can't use strtok as may have ,,

					// USER=user,password,call,appl,SYSOP

					memset(Param, 0, 2048);
					strlop(Context, 13);
					strlop(Context, ';');
					strlop(Context, ';');

					// replace any tab with space

					while ((ptr1 = strchr(Context, 9)))
						*ptr1 = ' ';


					ptr1 = Context;

					while (ptr1 && *ptr1 && n < 8)
					{
						ptr2 = strchr(ptr1, ',');
						if (ptr2) *ptr2++ = 0;

						strcpy(&Param[n][0], ptr1);
						strlop(Param[n++], ' ');
						ptr1 = ptr2;
						while (ptr1 && *ptr1 && *ptr1 == ' ')
							ptr1++;
					}

					User = &Param[0][0];

					Pwd = &Param[1][0];
	
					if (stricmp(User, "ANON") == 0)
					{
						if (Pwd[0] == 0) //ANON does't need call
							continue;

						strcpy(&Param[2][0], "ANON");
						strcpy(&Param[4][0], "");		// Dont allow SYSOP if ANON
					}

					UserCall = &Param[2][0];	
					Appl = &Param[3][0];
					Secure = &Param[4][0];
							
					if (User[0] == 0 || Pwd[0] == 0 || UserCall[0] == 0) // invalid record
						continue;
	
					USER->Callsign = strdup(UserCall);
					USER->Password = strdup(Pwd);
					USER->UserName = strdup(User);
					USER->Appl = (char *)malloc(32);
					USER->Appl[0] = 0;
					USER->Secure = FALSE;

					if (stricmp(Secure, "SYSOP") == 0)
						USER->Secure = TRUE;

					if (Appl[0] && strcmp(Appl, "\"\"") != 0)
					{
						strcpy(USER->Appl, Appl);
						strcat(USER->Appl, "\r\n");
					}

					NextUser++;

					if (gotUser == FALSE)	// only on first USER line
					{
						ui.sysopUser->setText(User);
						ui.sysopPass->setText(Pwd);

						ui.WEBMGMT->setChecked(true);
						ui.WEBMGMT->setEnabled(false);

						gotUser = TRUE;
					}

					continue;
				}

				// Don't want next three in port section - they are added along with users at the end

				if (stricmp(ptr, "CMS") == 0)
					continue;

				if (stricmp(ptr, "CMSCALL") == 0)
				{
					ui.CMSCALL->setText(strtok_s(NULL, " =\t\r\n", &Context));
					continue;
				}
				if (stricmp(ptr, "CMSPASS") == 0)
				{
					ui.CMSPASS->setText(strtok_s(NULL, " =\t\r\n", &Context));
					continue;
				}
			}

			if (PORT->PortType == H_AXIP)
			{
				if (stricmp(ptr, "MAP") == 0)
				{
					// MAP Call Address[UDP | TCP-Master | TCPSlave Port][KEEPALIVE nnn][SOURCEPORT nnn][B]

					struct MAPRec * MAP = &Maps[NextMap];
					char *p_call, *p_ipad, *p_UDP;
					char * p_udpport = 0;
					char * p_Interval;

					int bcflag = 0;
					int TCPMode = 0;

					p_call = strtok(Context, " \t\n\r");

					if (p_call == NULL)
						continue;

					strupr(p_call);

					p_ipad = strtok(NULL, " \t\n\r");

					if (p_ipad == NULL) 
						continue;

					p_UDP = strtok(NULL, " \t\n\r");

					bcflag = 0;
					TCPMode = 0;

					//
					//		Look for (optional) KEEPALIVE, DYNAMIC, UDP or BROADCAST params
					//

					while (p_UDP != NULL)
					{
						if (stricmp(p_UDP, "DYNAMIC") == 0)
						{
							p_UDP = strtok(NULL, " \t\n\r");
							continue;
						}

						if (stricmp(p_UDP, "KEEPALIVE") == 0)
						{
							p_Interval = strtok(NULL, " \t\n\r");

							if (p_Interval == NULL)
								continue;

							p_UDP = strtok(NULL, " \t\n\r");
							continue;
						}

						if (stricmp(p_UDP, "UDP") == 0)
						{
							p_udpport = strtok(NULL, " \t\n\r");

							if (p_udpport == NULL)
								continue;

							p_UDP = strtok(NULL, " \t\n\r");
							continue;
						}

						if (stricmp(p_UDP, "SOURCEPORT") == 0)
						{
							p_udpport = strtok(NULL, " \t\n\r");

							if (p_udpport == NULL)
							continue;

							p_UDP = strtok(NULL, " \t\n\r");
							continue;
						}

						if (stricmp(p_UDP, "TCP-Master") == 0 || stricmp(p_UDP, "TCPMaster") == 0)
						{
							p_udpport = strtok(NULL, " \t\n\r");

							if (p_udpport == NULL)
								continue;

							p_UDP = strtok(NULL, " \t\n\r");

							TCPMode = TCPMaster;

							continue;
						}

						if (stricmp(p_UDP, "TCP-Slave") == 0 || stricmp(p_UDP, "TCPSlave") == 0)
						{
							p_udpport = strtok(NULL, " \t\n\r");

							if (p_udpport == NULL)
								continue;

							p_UDP = strtok(NULL, " \t\n\r");

							TCPMode = TCPSlave;
							continue;

						}

						if (stricmp(p_UDP, "B") == 0)
						{
							bcflag = TRUE;
							p_UDP = strtok(NULL, " \t\n\r");
							continue;
						}

						if ((*p_UDP == ';') || (*p_UDP == '#'))	break;			// Comment on end
					
						// Unknown param

						p_UDP = strtok(NULL, " \t\n\r");
						continue;
					}

					MAP->Callsign = strdup(p_call);
					MAP->Host = strdup(p_ipad);
					
					if (p_udpport)
					{
						MAP->Port = strdup(p_udpport);
						MAP->UDP = TCPMode;
					}
					else
					{
						MAP->UDP = 3;
						MAP->Port = strdup("");
					}

					MAP->noBroadcast = !bcflag;
					NextMap++;
					continue;
				}
			}
			strcat(tempPort, linecopy);

			if (stricmp(ptr, "ENDPORT") == 0)
			{
				if (PORT->portConfig)
					free(PORT->portConfig);

				RemoveExtraBlankLines(tempPort);

				PORT->portConfig = strdup(tempPort);
				inPort = FALSE;
				portSetVisible(activePortnum, 1);
				PORT->gPortType->setText(Types[PORT->PortType]);

				if (activePortnum == nextPortnum)  // PORTNUM Not Used
				{
					nextPortnum++;
					PORT = &Ports[nextPortnum];
				}

				// Find next free port number

				while (PORT->Use == 0 && PORT->portConfig && nextPortnum < 31)
				{
					nextPortnum++;
					PORT = &Ports[nextPortnum];
				}

				continue;
			}

			if (stricmp(ptr, "PORTNUM") == 0)
			{
				char * num = strtok_s(NULL, " =\t\r\n", &Context);
				int i = atoi(num);

				if (i > 0 && i < 33)
					activePortnum = i - 1;

				continue;
			}
				
			if (stricmp(ptr, "ID") == 0)
			{
				char * id = strtok_s(NULL, "\r\n;", &Context);

				if (strlen(id) > 31)
					id[31] = 0;

				strcpy(PORT->ID, id);

				PORT->gPortName->setText(id);
				continue;
			}

			if (stricmp(ptr, "TYPE") == 0)
			{
				char * type = strtok_s(NULL, " =\t\r\n", &Context);

				if (stricmp(type, "ASYNC") == 0)
				{
					PORT->PortType = H_KISS;
				}

				if (stricmp(type, "LOOPBACK") == 0 || stricmp(type, "INTERNAL") == 0)
				{
					PORT->PortType = H_LOOP;
				}

				continue;

			}

			if (stricmp(ptr, "IPADDR") == 0 || stricmp(ptr, "COMPORT") == 0 || stricmp(ptr, "IOADDR") == 0)
			{
				char * addr = strtok_s(NULL, " =\t\r\n", &Context);

				if (addr)
					strcpy(PORT->Param1, addr);
				continue;
			}

			if (stricmp(ptr, "SPEED") == 0)
			{
				char * addr = strtok_s(NULL, " =\t\r\n", &Context);

				PORT->Param2 = atoi(addr);
				continue;
			}

			if (stricmp(ptr, "TCPPORT") == 0)
			{
				char * addr = strtok_s(NULL, " =\t\r\n", &Context);

				PORT->Param2 = atoi(addr);

				if (PORT->PortType== H_KISS)
					PORT->PortType= H_KISSTCP;

				continue;
			}

			if (stricmp(ptr, "UDPPORT") == 0)
			{
				char * addr = strtok_s(NULL, " =\t\r\n", &Context);

				PORT->Param2 = atoi(addr);

				if (PORT->PortType== H_KISS)
					PORT->PortType= H_KISSUDP;

				continue;
			}

			if (stricmp(ptr, "ADDR") == 0)
			{
				// ADDR line - Host then Port

				char * addr = strtok_s(NULL, " =\t\r\n", &Context);
				strcpy(PORT->Param1, addr);

				addr = strtok_s(NULL, " =\t\r\n", &Context);
				PORT->Param2 = atoi(addr);

				continue;
			}

			if (stricmp(ptr, "DRIVER") == 0 || stricmp(ptr, "DLLNAME") == 0)
			{
				ptr = strtok_s(NULL, ".\t\r\n", &Context);

				if (ptr == NULL)
					continue;

				if (stricmp(ptr, "ARDOP") == 0)
				{
					PORT->PortType = H_ARDOP;
					continue;
				}

				if (stricmp(ptr, "BPQAXIP") == 0)
				{
					PORT->PortType = H_AXIP;
					continue;
				}

				if (stricmp(ptr, "BPQETHER") == 0)
				{
					PORT->PortType = H_ETHER;
					continue;
				}

				if (stricmp(ptr, "BPQVKISS") == 0)
				{
					PORT->PortType= H_VKISS;
					continue;
				}

				if (stricmp(ptr, "FLDIGI") == 0)
				{
					PORT->PortType= H_FLDIGI;
					continue;
				}

				if (stricmp(ptr, "KAMPACTOR") == 0)
				{
					PORT->PortType= H_KAM;
					continue;
				}

				if (stricmp(ptr, "SCSPACTOR") == 0)
				{
					PORT->PortType= H_SCS;
					continue;
				}


				if (stricmp(ptr, "SCSTRACKER") == 0)
				{
					PORT->PortType = H_TRK;
					continue;
				}


				if (stricmp(ptr, "SERIAL") == 0)
				{
					PORT->PortType= H_SERIAL;
					continue;
				}

				if (stricmp(ptr, "TELNET") == 0)
				{
					PORT->PortType= H_TELNET;
					TelnetPort = activePortnum + 1;
					continue;
				}
				if (stricmp(ptr, "UZ7HO") == 0)
				{
					PORT->PortType= H_UZ7HO;
					continue;
				}

				if (stricmp(ptr, "WINMOR") == 0)
				{
					PORT->PortType= H_WINMOR;
					continue;
				}

				if (stricmp(ptr, "VARA") == 0)
				{
					PORT->PortType = H_VARA;
					continue;
				}

				if (stricmp(ptr, "KISSHF") == 0)
				{
					PORT->PortType = H_KISSHF;
					continue;
				}

				if (stricmp(ptr, "WINRPR") == 0)
				{
					PORT->PortType = H_WINRPR;
					continue;
				}
				if (stricmp(ptr, "HSMODEM") == 0)
				{
					PORT->PortType = H_HSMODEM;
					continue;
				}
			}
			continue;
		}

		if (stricmp(ptr, "APRSDIGI") == 0)
		{
			inAPRS = TRUE;
			ui.APRSTab->setEnabled(1);
			ui.EnableAPRS->setChecked(1);
			continue;
		}

		if (stricmp(ptr, "ROUTES:") == 0)
		{
			inRoutes = TRUE;
			continue;
		}

		if (stricmp(ptr, "IPGATEWAY") == 0)
		{
			currentBlockType = IPGATEWAY;
			inBlock = TRUE;
		}
		else if (stricmp(ptr, "PORTMAPPER") == 0)
		{
			currentBlockType = PORTMAPPER;
			inBlock = TRUE;
		}
		else if (stricmp(ptr, "INFOMSG:") == 0)
		{
			currentBlockType = INFOMSG;
			inBlock = TRUE;
		}
		else if (stricmp(ptr, "IDMSG:") == 0)
		{
			currentBlockType = IDMSG;
			inBlock = TRUE;
		}
		else if (stricmp(ptr, "BTEXT:") == 0)
		{
			currentBlockType = BTEXT;
			inBlock = TRUE;
		}
		else if (stricmp(ptr, "CTEXT:") == 0)
		{
			currentBlockType = CTEXT;
			inBlock = TRUE;
		}

		if (inBlock)
		{
			strcat(blockConfig[currentBlockType], linecopy);
			continue;
		}

		if (stricmp(ptr, "NODECALL") == 0)
		{
			NODECALL = strtok_s(NULL, " =\t\r\n", &Context);
			ui.NODECALL->setText(NODECALL);
			continue;
		}

		if (stricmp(ptr, "NODEALIAS") == 0)
		{
			NODEALIAS = strtok_s(NULL, " =\t\r\n", &Context);
			ui.NODEALIAS->setText(NODEALIAS);
			continue;
		}

		if (stricmp(ptr, "LOCATOR") == 0)
		{
			char * ptr = strtok_s(NULL, "\t\r\n", &Context);
			strlop(ptr, ';');
			LOC = ptr;
			ui.LOC->setText(LOC);
			continue;
		}

		if (stricmp(ptr, "HFCTEXT") == 0)
		{
			char * Msg = strtok_s(NULL, "=\t\r\n", &Context);
			ui.HFCTEXT->setText(Msg);
			if (strlen(Msg) > 80)
				Msg[80] = 0;

			strcpy(HFCText, Msg);

			continue;
		}

		if (stricmp(ptr, "TNCPORT") == 0)
		{
			strcpy(tncportConfig, linecopy);
			inTNCPort = TRUE;
			continue;
		}

		if (stricmp(ptr, "PORT") == 0)
		{
			strcpy(tempPort, linecopy);
			inPort = TRUE;
			activePortnum = nextPortnum;
			continue;
		}

		if (stricmp(ptr, "APPLICATION") == 0)
		{
			struct APPL * pAppl;

			int Num = atoi(strtok_s(NULL, " ,\t\r\n", &Context));
			char * Appl = strtok_s(NULL, " ,\t\r\n", &Context);
			char * CmdAlias = NULL;
			char * ApplCall = NULL;
			char * ApplAlias = NULL;
			char * ApplQual = NULL;

			if (Context[0] == ',')
				Context++;
			else
				CmdAlias = strtok_s(NULL, ",\t\r\n", &Context);

			if (Context[0] == ',')
				Context++;
			else
				ApplCall = strtok_s(NULL, " ,\t\r\n", &Context);

			if (Context[0] == ',')
				Context++;
			else
				ApplAlias = strtok_s(NULL, " ,\t\r\n", &Context);

			ApplQual = strtok_s(NULL, " ,\t\r\n", &Context);

			if (Appl)
			{
				pAppl = &Appls[nextApplNum++];

				pAppl->Num = Num;
				strcpy(pAppl->Command, Appl);
				if (ApplCall)
					strcpy(pAppl->ApplCall, ApplCall);
				if (ApplAlias)
					strcpy(pAppl->ApplAlias, ApplAlias);
				
				if (ApplQual)
					pAppl->ApplQual = atoi(ApplQual);

				if (CmdAlias)
					strcpy(pAppl->CommandAlias, CmdAlias);

				if (strcmp(Appl, "BBS") == 0)
				{
					BBSAppl = Num;
					ui.BBS->setChecked(true);
					ui.BBS->setEnabled(false);

				}


				else if (stricmp(Appl, "CHAT") == 0)
				{
					ChatAppl = Num;
					ui.CHAT->setChecked(true);
					ui.CHAT->setEnabled(false);
				}


				else if (strcmp(Appl, "RMS") == 0)
				{
					RMSAppl = Num;
					ui.RMS->setChecked(true);
					ui.RMS->setEnabled(false);

				}

			}
			continue;
		}

		if (stricmp(line, "SIMPLE") == 0)
		{
			ui.SIMPLE->setChecked(1);
			continue;
		}

		if (stricmp(line, "LINMAIL") == 0)
			continue;

		if (stricmp(line, "LINCHAT") == 0)
			continue;


		if (stricmp(line, "FULL_CTEXT") == 0)
		{
			ptr = strtok_s(NULL, " =\t\r\n", &Context);

			ui.FullCText->setChecked(atoi(ptr));
			continue;
		}

		if (stricmp(line, "IDINTERVAL") == 0)
		{
			ptr = strtok_s(NULL, " =\t\r\n", &Context);

			ui.IDInterval->setText(ptr);
			continue;
		}

		if (stricmp(line, "BTINTERVAL") == 0)
		{
			ptr = strtok_s(NULL, " =\t\r\n", &Context);

			ui.BTInterval->setText(ptr);
			continue;
		}

		for (i = 0; i < NodeParamCount; i++)
		{
			struct NODEPARAM * NodeParam = &NodeParams[i];

			if (stricmp(line, NodeParam->param) == 0)
			{
				QCheckBox * checkbox = (QCheckBox *)NodeParam->widget;
				QLineEdit * linedit = (QLineEdit *)NodeParam->widget;

				ptr = strtok_s(NULL, " =\t\r\n", &Context);

				memcpy(NodeParam->value, ptr, 19);
				
				if (checkbox)
				{
					if (NodeParam->isCheckBox)
						checkbox->setChecked(atoi(ptr));
					else
						linedit->setText(ptr);
				}
				break;
			}
		}
			
		if (i < NodeParamCount)
			continue;
		
		// Something we don't handle - write to main config section

//		qDebug() << linecopy;
		strcat(mainConfig, linecopy);
	}

	fclose(fp);

	RemoveExtraBlankLines(mainConfig);

	refreshPorts();
	refreshAppls();
	refreshAPRS();

	ui.IPGateway->setPlainText(blockConfig[IPGATEWAY]);
	ui.PortMapper->setPlainText(blockConfig[PORTMAPPER]);
	ui.InfoMsg->setPlainText(blockConfig[INFOMSG]);
	ui.IDMsg->setPlainText(blockConfig[IDMSG]);
	ui.BText->setPlainText(blockConfig[BTEXT]);
	ui.CText->setPlainText(blockConfig[CTEXT]);
	ui.Routes->setPlainText(routeConfig);
	ui.TNCPort->setPlainText(tncportConfig);
}

void BPQConfigGen::WriteARDOPPort(FILE * fp)
{
	QString str3 = ui.LOC->text().toUpper();

	fprintf(fp, "\r\nPORT\r\n");
	fprintf(fp, " ID=ARDOP\r\n");
	fprintf(fp, " DRIVER=ARDOP\r\n");
	fprintf(fp, " INTERLOCK=1\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  ADDR 127.0.0.1 8515\r\n");
	fprintf(fp, "  DEBUGLOG TRUE\r\n");
	fprintf(fp, "  CMDTRACE TRUE\r\n");
	if (str3.size())
		fprintf(fp, "  GRIDSQUARE %s\r\n", str3.toLocal8Bit().constData());
	fprintf(fp, "ENDPORT\r\n");
}


void BPQConfigGen::WriteUZ7HOPort(FILE * fp)
{
	fprintf(fp, "\r\nPORT\r\n");
	fprintf(fp, " ID=UZ7HO\r\n");
	fprintf(fp, " DRIVER=UZ7HO\r\n");
	fprintf(fp, " CHANNEL=A\r\n");
	fprintf(fp, " INTERLOCK=1\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  ADDR 127.0.0.1 8000\r\n");
	fprintf(fp, "  UPDATEMAP\r\n");
	fprintf(fp, "ENDPORT\r\n");
}

void BPQConfigGen::WriteWINMORPort(FILE *fp)
{
	fprintf(fp, "\r\nPORT\r\n");
	fprintf(fp, " ID=WINMOR\r\n");
	fprintf(fp, " DRIVER=WINMOR\r\n");
	fprintf(fp, " INTERLOCK=1\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  ADDR 127.0.0.1 8500\r\n");
	fprintf(fp, "  DEBUGLOG TRUE\r\n");
	fprintf(fp, "  BUSYLOCK TRUE\r\n");
	fprintf(fp, "  BW 1600\r\n");
	fprintf(fp, "ENDPORT\r\n");
}

void BPQConfigGen::WriteVARAPort(FILE * fp)
{
	fprintf(fp, "\r\nPORT\r\n");
	fprintf(fp, " ID=VARA\r\n");
	fprintf(fp, " DRIVER=VARA\r\n");
	fprintf(fp, " INTERLOCK=1\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  ADDR 127.0.0.1 8300\r\n");
	fprintf(fp, "ENDPORT\r\n");
}

void BPQConfigGen::AddTCPKISSPort(FILE * fp)
{
	fprintf(fp, "PORT\r\n");

	fprintf(fp, "ID=KISS over TCP Slave\r\n");
	fprintf(fp, "TYPE=ASYNC\r\n");
	fprintf(fp, "IPADDR=0.0.0.0\r\n");
	fprintf(fp, "TCPPORT=8100\r\n");
	fprintf(fp, "ENDPORT\r\n");
}

void refreshPorts()
{
	int x = 5;

	for (int i = 0; i < 32; i++)
		portSetVisible(i, 0);

	for (int pnum = 0; pnum < 32; pnum++)
	{
		struct PORTREC * PORT = &Ports[pnum];

		if (PORT->portConfig || PORT->Use)
		{
			Ports[pnum].gPortNum->setText(QString::number(pnum + 1));
			Ports[pnum].gPortType->setText(Types[PORT->PortType]);
			Ports[pnum].gPortName->setText(PORT->ID);

			portSetVisible(pnum, 1);

			PORT->gPortNum->setGeometry(QRect(5, x, 20, 18));
			PORT->gPortType->setGeometry(QRect(25, x, 100, 18));
			PORT->gPortName->setGeometry(QRect(130, x, 250, 18));
			PORT->gPortEdit->setGeometry(QRect(385, x, 60, 18));
			PORT->Bridge->setGeometry(QRect(505, x, 56, 18));
			PORT->Path->setGeometry(QRect(225, x, 191, 18));
			PORT->Digi->setGeometry(QRect(425, x, 71, 18));
			PORT->PortNo->setGeometry(QRect(25, x, 20, 20));
			PORT->Port->setGeometry(QRect(50, x, 170, 20));
			PORT->Enable->setGeometry(QRect(5, x, 21, 18));

			x += 22;
		}
	}

}

void portSetVisible(int i, int v)
{
	struct PORTREC * PORT = &Ports[i];

	PORT->gPortNum->setVisible(v);
	PORT->gPortType->setVisible(v);
	PORT->gPortEdit->setVisible(v);
//	PORT->gParam2->setVisible(v);
	PORT->gPortName->setVisible(v);
	PORT->Enable->setVisible(v);
	PORT->Port->setVisible(v);
	PORT->PortNo->setVisible(v);
	PORT->Digi->setVisible(v);
	PORT->Path->setVisible(v);
	PORT->Bridge->setVisible(v);

	/*
		PORT->gPortNum->setGeometry(QRect(5, 22 * i + 5, 20, 18));
		PORT->gPortType->setGeometry(QRect(25, 22 * i + 5, 100, 18));
		PORT->gPortName->setGeometry(QRect(130, 22 * i + 5, 250, 18));
		PORT->gPortEdit->setGeometry(QRect(385, 22 * i + 5, 60, 18));
		PORT->Bridge->setGeometry(QRect(505, 22 * i + 5, 56, 18));
		PORT->Path->setGeometry(QRect(225, 22 * i + 5, 191, 18));
		PORT->Digi->setGeometry(QRect(425, 22 * i + 5, 71, 18));
		PORT->PortNo->setGeometry(QRect(25, 22 * i + 5, 20, 20));
		PORT->Port->setGeometry(QRect(50, 22 * i + 5, 170, 20));
		PORT->Enable->setGeometry(QRect(5, 22 * i + 5, 21, 18));
*/
}

void applSetVisible(int i, int v)
{
	gapplNum[i]->setVisible(v);
	gapplCmd[i]->setVisible(v);
	gapplCmdAlias[i]->setVisible(v);
	gapplCall[i]->setVisible(v);
	gapplAlias[i]->setVisible(v);
	gapplQual[i]->setVisible(v);
	gapplEdit[i]->setVisible(v);
}

void aprsSetVisible(int i, int v)
{
	struct OBJECT * Object = &Objects[i];

	Object->label_64->setVisible(v);
	Object->label_65->setVisible(v);
	Object->label_66->setVisible(v);
	Object->label_67->setVisible(v);

	Object->ObjInterval->setVisible(v);
	Object->ObjPath->setVisible(v);
	Object->ObjPorts->setVisible(v);
	Object->ObjText->setVisible(v);
}






void refreshAppls()
{
	int i = 0;

	for (i = 0; i < 32; i++)
		applSetVisible(i, 0);

	for (i = 0; i < 32; i++)
	{
		struct APPL * pAppl = &Appls[i];

		if (pAppl->Command[0])
		{
			gapplNum[i]->setText(QString::number(pAppl->Num));
			gapplCmd[i]->setText(pAppl->Command);
			gapplCmdAlias[i]->setText(pAppl->CommandAlias);
			gapplCall[i]->setText(pAppl->ApplCall);
			gapplAlias[i]->setText(pAppl->ApplAlias);
			gapplQual[i]->setText(QString::number(pAppl->ApplQual));
			applSetVisible(i, 1);
		}
	}
}

void BPQConfigGen::GetParams()
{
	NodeCall = ui.NODECALL->text().toUpper();
	NodeAlias = ui.NODEALIAS->text().toUpper();
	LOC = ui.LOC->text().toUpper();

	User = ui.sysopUser->text();
	Pass = ui.sysopPass->text();

	CMSCall = ui.CMSCALL->text().toUpper();;
	CMSPass = ui.CMSPASS->text();

	RMS = ui.RMS->isChecked();

	strcpy(LoppedCall, NodeCall.toLocal8Bit().constData());

	char * ptr = strchr(LoppedCall, '-');
	if (ptr)
		*ptr = 0;

	if (User.length() || Pass.length())
	{
		// Create or update first User Record

		struct UserRec * USER = &Users[0];

		if (USER->UserName)
			free(USER->UserName);

		if (USER->Password)
			free(USER->Password);

		USER->UserName = strdup(User.toLocal8Bit());
		USER->Password = strdup(Pass.toLocal8Bit());

		if (NextUser == 0)
		{
			// Creating record

			USER->Callsign = strdup(LoppedCall);
			USER->Appl = strdup("");
			USER->Secure = 1;
			NextUser++;
		}
	}
}


void BPQConfigGen::CreateConfig()
{
	QMessageBox msgBox;

	QString str = ui.ConfigDir->text();

	QSettings settings("G8BPQ", "BPQConfigGen");

	strcpy(configPath, str.toUtf8());

	settings.setValue("configPath", configPath);

#ifndef WIN32
	strcpy(programPath, str.toUtf8());
	settings.setValue("programPath", programPath);
#endif


	char FN[260];
	FILE *fp;
	int portcount = 0;
	int val;

	GetParams();

	sprintf(FN, "%s/bpq32.cfg", str.toLocal8Bit().constData());

	datestampedbackup(FN);

	fp = fopen(FN, "wb");

	if (fp == NULL)
	{
		qDebug() << strerror(errno);
		msgBox.setWindowTitle("Error");
		msgBox.setText("Failed to open bpq32.cfg");
		msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
		msgBox.exec();
		return;
	}

	saveConfigBlock(ui.InfoMsg, INFOMSG);
	saveConfigBlock(ui.IDMsg, IDMSG);
	saveConfigBlock(ui.BText, BTEXT);
	saveConfigBlock(ui.CText, CTEXT);

	fprintf(fp, "; Written by BPQConfigGen\r\n");
	fprintf(fp, "\r\n");

	if (ui.SIMPLE->isChecked())
		fprintf(fp, "SIMPLE\r\n");
	
	fprintf(fp, "NODECALL=%s\r\n", NodeCall.toLocal8Bit().constData());

	if (NodeAlias.size())
		fprintf(fp, "NODEALIAS=%s\r\n", NodeAlias.toLocal8Bit().constData());

	if (LOC.size())
		fprintf(fp, "LOCATOR=%s\r\n", LOC.toLocal8Bit().constData());

	if (ui.FullCText->isChecked())
		fprintf(fp, "FULL_CTEXT=1\r\n");

	val = atoi(ui.IDInterval->text().toUtf8().data());
	fprintf(fp, "IDINTERVAL=%d\r\n", val);

	val = atoi(ui.BTInterval->text().toUtf8().data());
	fprintf(fp, "BTINTERVAL=%d\r\n", val);

	if (HFCText[0])
		fprintf(fp, "HFCTEXT=%s\r\n", HFCText);
		
	fprintf(fp, "\r\n");

	// Write Main Settings

	// If we are not using SIMPLE then write all params that have a default in SIMPLE even if not set

	struct NODEPARAM * NodeParam;

	for (int i = 0; i < NodeParamCount; i++)
	{
		NodeParam = &NodeParams[i];

		if (NodeParam->value[0])
			fprintf(fp, "%s=%s\r\n", NodeParam->param, NodeParam->value);
		else
		{
			// Not set, but if not using SIMPLE we need to write the default

			if (ui.SIMPLE->isChecked() == 0)
				if (NodeParam->inSimple && NodeParam->simpleDefault[0])
					fprintf(fp, "%s=%s\r\n", NodeParam->param, NodeParam->simpleDefault);
		}
	}

	// Any unsupported stuff

	fprintf(fp, mainConfig);
	fprintf(fp, "\r\n");

	// Write Block Configs

	for (int i = 0; i < 10; i++)
	{
		if (blockConfig[i][0])
		{
			fprintf(fp, blockConfig[i]);
			fprintf(fp, "\r\n");
		}
	}


	// Write PORT Lines 

	// write ports and applications

	for (int i = 0; i < 32; i++)
	{
		if (Ports[i].portConfig)
		{
			portcount++;

			if (Ports[i].PortType == H_TELNET)
			{
				char * ptr = strstr(Ports[i].portConfig, "ENDPORT");

				if (ptr)
					*ptr = 0;

				fprintf(fp, Ports[i].portConfig);

				// Add in CMSCall and Pass if present

				if (RMS)
					fprintf(fp, "  CMS=1\r\n");

				if (CMSCall.size())
					fprintf(fp, "  CMSCALL=%s\r\n", CMSCall.toLocal8Bit().constData());
				if (CMSPass.size())
					fprintf(fp, "  CMSPASS=%s\r\n", CMSPass.toLocal8Bit().constData());

				fprintf(fp, "\r\n");

				// add in USER lines

				for (int i = 0; i < NextUser; i++)
				{
					struct UserRec * USER = &Users[i];
					fprintf(fp, "  USER=%s,%s,%s,%s,%s;\r\n",
						USER->UserName, USER->Password, USER->Callsign, USER->Appl,
						(USER->Secure) ? "SYSOP" : "");
				}

				fprintf(fp, "\r\n");
				fprintf(fp, "ENDPORT\r\n\r\n");

				strcat(Ports[i].portConfig, "ENDPORT\r\n");
			}
			else if (Ports[i].PortType == H_AXIP)
			{
				char * ptr = strstr(Ports[i].portConfig, "ENDPORT");

				if (ptr)
					*ptr = 0;

				fprintf(fp, Ports[i].portConfig);

				// add in MAP lines

				for (int i = 0; i < NextMap; i++)
				{
					struct MAPRec * MAP = &Maps[i];
					fprintf(fp, "  MAP %s %s ", MAP->Callsign, MAP->Host);

					if (MAP->UDP == 0)
					{
						fprintf(fp, "UDP %s ", MAP->Port);
						if (MAP->SourcePort && MAP->SourcePort[0])
							fprintf(fp, "SOURCEPORT %s ", MAP->SourcePort);
					}
					else if (MAP->UDP == 1)
						fprintf(fp, "TCP-MASTER %s ", MAP->Port);
					else if (MAP->UDP == 2)
						fprintf(fp, "TCP-SLAVE %s ", MAP->Port);

					if (!MAP->noBroadcast)
						fprintf(fp, "B ");

					fprintf(fp, "\r\n");
				}

				fprintf(fp, "\r\n");
				fprintf(fp, "ENDPORT\r\n\r\n");

				strcat(Ports[i].portConfig, "ENDPORT\r\n");
			}
			else
			{
				fprintf(fp, Ports[i].portConfig);
				fprintf(fp, "\r\n");
			}
		}
	}

	fprintf(fp, "\r\n");

	if (portcount == 0)		// No ports
	{
		QMessageBox msgBox;

		msgBox.setWindowTitle("Warning");
		msgBox.setText("You have created a config without any ports. You will need to add at least one before node will start");
		msgBox.move(mainpos.x() + 50, mainpos.y() + 300);
		msgBox.exec();
	}

	// Write locked routes

	fprintf(fp, "ROUTES:\r\n");
	fprintf(fp, routeConfig);		// Comment block
	fprintf(fp, "\r\n");

	for (int i = 0; i < MaxLockedRoutes; i++)
	{
		struct ROUTECONFIG * Route = &Routes[i];

		if (Route->call[0])
		{
			int INP3Flag = 0;

			if (Route->INP3)
				INP3Flag |= 1;

			if (Route->NoKeepAlive)
				INP3Flag |= 2;


			fprintf(fp, "%s,%d,%d,%d,%d,%d,%d,%d\r\n",
				Route->call, Route->quality, Route->port,
				Route->pwind, Route->pfrack, Route->ppacl,
				INP3Flag, Route->farQual);
		}
	}
	fprintf(fp, "****\r\n\r\n");

	for (int i = 0; i < 32; i++)
	{
		struct APPL * pAppl = &Appls[i];

		if (pAppl->Command[0])
			fprintf(fp, "APPLICATION %d,%s,%s,%s,%s,%d\r\n",
				pAppl->Num,
				pAppl->Command,
				pAppl->CommandAlias,
				pAppl->ApplCall,
				pAppl->ApplAlias,
				pAppl->ApplQual);
	}

	fprintf(fp, "\r\n");

	// Write IPGateway, PortMapper, APRS

	WriteAPRSConfig(fp);

	fprintf(fp, "\r\n");

#ifndef WIN32

	if (ui.BBS->isChecked())
		fprintf(fp, "LINMAIL\r\n");

	if (ui.CHAT->isChecked())
		fprintf(fp, "LINCHAT\r\n");

#endif

	fclose(fp);

	refreshPorts();

	msgBox.move(mainpos.x() + 200, mainpos.y() + 300);

	msgBox.setWindowTitle("Save");

	if (HadConfig)
		msgBox.setText("bpq32.cfg updated");
	else
		msgBox.setText("bpq32.cfg created");

	msgBox.exec();
}



QComboBox * pPortType;
QLineEdit * pportNum;
QLineEdit * pParam0;
QLineEdit * pParam1;
QLineEdit * pParam2;
QLineEdit * pParam3;

QLabel * lportNum;
QLabel * pLabel0;
QLabel * pLabel1;
QLabel * pLabel2;
QLabel * pLabel3;

void PortDialog::PortTypeChanged(int Selected)
{
	QVariant Q = pPortType->currentText();

	pParam0->setText(Types[Selected]);

	pLabel1->setVisible(0);
	pParam1->setVisible(0);
	pLabel2->setVisible(0);
	pParam2->setVisible(0);
	pLabel3->setVisible(0);
	pParam3->setVisible(0);


	switch (Selected)
	{
	case H_ARDOP:
	case H_FLDIGI:
	case H_KISSTCP:
	case H_MPSK:
	case H_UZ7HO:
	case H_V4:
	case H_VARA:
	case H_WINMOR:
	case H_KISSHF:

		pLabel1->setText("Host Name");
		pLabel2->setText("TCP Port");

		pLabel1->setVisible(1);
		pParam1->setVisible(1);
		pLabel2->setVisible(1);
		pParam2->setVisible(1);

		pParam1->setText("127.0.0.1");
		pParam2->setText(QString::number(defaultPort[Selected]));

		break;

	case H_KISSUDP:
		
		pLabel1->setText("Host Name");
		pLabel2->setText("UDP Ports");

		pLabel1->setVisible(1);
		pParam1->setVisible(1);
		pLabel2->setVisible(1);
		pParam2->setVisible(1);

		break;

	case H_KISS:
	case H_AEA:
	case H_HAL:
	case H_KAM:
	case H_SCS:
	case H_TRK:
	case H_TRKM:
	case H_SERIAL:
	case H_VKISS:

		pLabel1->setText("Serial Port");
		pLabel2->setText("Speed");
		pParam2->setText(QString::number(defaultPort[Selected]));
		pLabel1->setVisible(1);
		pParam1->setVisible(1);
		pLabel2->setVisible(1);
		pParam2->setVisible(1);

		break;

	case H_LOOP:

		pParam0->setText("Loopback Port");
		break;

	}

	switch (Selected)
	{
	case H_KISS:
	case H_KISSTCP:
	case H_KISSUDP:
	case H_UZ7HO:

		pLabel3->setVisible(1);
		pParam3->setVisible(1);
		pParam3->setText("A");
	}
}

PortDialog::PortDialog(QWidget *parent) : QDialog(parent)
{
	this->resize(562, 491);

	this->move(mainpos.x() + 50, mainpos.y() + 50);

	setWindowTitle(tr("Add Port"));

	while (Ports[nextPortnum].portConfig && nextPortnum < 31)
		nextPortnum++;

	lportNum = new QLabel("Portnum", this);
	lportNum->setGeometry(QRect(20, 22, 60, 18));
	pportNum = new QLineEdit(this);
	pportNum->setGeometry(QRect(85, 22, 20, 18));
	pportNum->setText(QString::number(nextPortnum + 1));

	pPortType = new QComboBox(this);
	pPortType->setGeometry(QRect(125, 22, 120, 18));

	pLabel0 = new QLabel("ID", this);
	pLabel0->setGeometry(QRect(20, 52, 100, 18));
	pParam0 = new QLineEdit(this);
	pParam0->setGeometry(QRect(125, 52, 300, 18));

	pLabel1 = new QLabel("Serial Port", this);
	pLabel1->setGeometry(QRect(20, 82, 100, 18));
	pParam1 = new QLineEdit(this);
	pParam1->setGeometry(QRect(125, 82, 100, 18));

	pLabel2 = new QLabel("Speed", this);
	pLabel2->setGeometry(QRect(20, 112, 100, 18));
	pParam2 = new QLineEdit(this);
	pParam2->setGeometry(QRect(125, 112, 100, 18));

	pLabel3 = new QLabel("Channel", this);
	pLabel3->setGeometry(QRect(20, 142, 100, 18));
	pParam3 = new QLineEdit(this);
	pParam3->setGeometry(QRect(125, 142, 15, 18));

	pLabel3->setVisible(0);
	pParam3->setVisible(0);

	for (int j = 0; j < NUMBEROFTYPES; j++)
		pPortType->addItem(&Types[j][0]);

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttonBox->setGeometry(QRect(400, 400, 100, 18));

	PortTypeChanged(0);			// Set default

	connect(pPortType, SIGNAL(currentIndexChanged(int)), this, SLOT(PortTypeChanged(int)));

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(myreject()));
}


PortDialog::~PortDialog()
{
}

void PortDialog::myaccept()
{
	QVariant Q;
	QString val = pportNum->text();
	QByteArray ba;
	char * ptr;

	int port = atoi(val.toLatin1()) - 1;

	struct PORTREC * PORT = &Ports[port];

	PORT->PortType = pPortType->currentIndex();

	val = pParam0->text();
	ba = val.toLatin1();

	strcpy(PORT->ID, ba.data());

	val = pParam1->text();
	ba = val.toLatin1();
	strcpy(PORT->Param1, ba.data());

	val = pParam2->text();
	ba = val.toLatin1();
	PORT->Param2 = atoi(ba.data());

	val = pParam3->text();
	ba = val.toLatin1();
	strcpy(PORT->Param3, ba.data());

//	gPortType->setText(Types[PortType]);

//	gPortName->setText(ID);

//	if (Param1[0])
//		gParam1->setText(Param1);

//	if (Param2)
//		gParam2->setText(QString::number(Param2));

	if (PORT->portConfig)
		free(PORT->portConfig);

	ptr = PORT->portConfig = (char *)malloc(32768);

	QMessageBox msgBox;

	switch (PORT->PortType)
	{
	case H_AEA:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=AEAPACTOR\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_ARDOP:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=ARDOP\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "  DEBUGLOG TRUE\r\n");
		ptr += sprintf(ptr, "  CMDTRACE TRUE\r\n");
		ptr += sprintf(ptr, "  CWID TRUE\r\n");
		ptr += sprintf(ptr, "  MAXCONREQ 10\r\n");
		if (LOC.size())
			ptr += sprintf(ptr, "  GRIDSQUARE %s\r\n", LOC.toLocal8Bit().constData());

		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_AXIP:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=BPQAXIP\r\n");
		ptr += sprintf(ptr, " PACLEN=236\r\n");
		ptr += sprintf(ptr, " FRACK=5000\r\n");
		ptr += sprintf(ptr, " RESPTIME=1500\r\n");
		ptr += sprintf(ptr, " MAXFRAME=4\r\n");
		ptr += sprintf(ptr, " RETRIES=6\r\n");
		ptr += sprintf(ptr, " QUALITY=192\r\n");
		ptr += sprintf(ptr, " MINQUAL=191\r\n");

		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  BROADCAST NODES\r\n");
		ptr += sprintf(ptr, "  MHEARD ON\r\n");
		ptr += sprintf(ptr, "  UDP 10093\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		{
			struct MAPRec * MAP = &Maps[NextMap];

			MAP->Callsign = strdup("XXXXXX");
			MAP->Host = strdup("xxxx.com");
			MAP->Port = strdup("10093");
			NextMap++;
		}
		msgBox.setWindowTitle("AXIP Port Config");
		msgBox.setText("Edit config and replace the sample MAP line with your MAP info");
		msgBox.move(mainpos.x() + 100, mainpos.y() + 300);
		msgBox.exec();

		break;


//	case H_FLDIGI:
//		break;


	case H_HAL:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=HALDRIVER\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  TONES 160/1800\r\n");
		ptr += sprintf(ptr, "  NEEDXONXOFF; Needed if DSP4200\r\n");
		ptr += sprintf(ptr, "  DEFAULTMODE CLOVER\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_KAM:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=KAMPACTOR\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;



	case H_KISS:
	case H_KISSTCP:
	case H_KISSUDP:
	case H_VKISS:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);

		if ((PORT->PortType) == H_VKISS)
		{
			ptr += sprintf(ptr, "DRIVER=BPQVKISS\r\n");
		}
		else
		{
			ptr += sprintf(ptr, " TYPE=ASYNC\r\n");
			ptr += sprintf(ptr, " PROTOCOL=KISS\r\n");
		}

		if ((PORT->PortType) == H_KISS || (PORT->PortType) == H_VKISS)
		{
			ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
			ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		}
		else if ((PORT->PortType) == H_KISSTCP)
		{
			ptr += sprintf(ptr, " IPADDR=%s\r\n", PORT->Param1);
			ptr += sprintf(ptr, " TCPPORT=%d\r\n", PORT->Param2);
		}
		else
		{
			ptr += sprintf(ptr, " IPADDR=%s\r\n", PORT->Param1);
			ptr += sprintf(ptr, " UDPPORT=%d", PORT->Param2);
		}

		ptr += sprintf(ptr, " CHANNEL=%s\r\n", PORT->Param3);
		ptr += sprintf(ptr, " PACLEN=128\r\n");
		ptr += sprintf(ptr, " TXDELAY=500\r\n");
		ptr += sprintf(ptr, " FRACK=7000\r\n");
		ptr += sprintf(ptr, " RESPTIME=1500\r\n");
		ptr += sprintf(ptr, " MAXFRAME=4\r\n");
		ptr += sprintf(ptr, " RETRIES=6\r\n");

		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_LOOP:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " TYPE=LOOPBACK\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

//	case H_MPSK:

//		break;

	case H_SCS:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=SCSPACTOR\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_SERIAL:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=SERIAL\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_TRK:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=SCSTRACKER\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  USEAPPLCALLS; Accept connects to all APPLCALLS\r\n");
		ptr += sprintf(ptr, "  %%F 1700; Centre Freq for Normal Packet(Default is 1500)\r\n");
		ptr += sprintf(ptr, "  %%B 300;  Radio Baud Rate for Normal Packet\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_TRKM:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=TRKMULTI\r\n");
		ptr += sprintf(ptr, " COMPORT=%s\r\n", PORT->Param1);
		ptr += sprintf(ptr, " SPEED=%d\r\n", PORT->Param2);
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  USEAPPLCALLS; Accept connects to all APPLCALLS\r\n");
		ptr += sprintf(ptr, "  %%F 1700; Centre Freq for Normal Packet(Default is 1500)\r\n");
		ptr += sprintf(ptr, "  %%B 1200;  Radio Baud Rate for Normal Packet\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_TELNET:

		createTelnetConfig(port);
		break;

	case H_UZ7HO:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=UZ7HO\r\n");
		ptr += sprintf(ptr, " CHANNEL=%s\r\n", PORT->Param3);
		ptr += sprintf(ptr, " PACLEN=128\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "  MAXSESSIONS 5\r\n");
		ptr += sprintf(ptr, "  UPDATEMAP\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_V4:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=V4\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_VARA:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=VARA\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_WINMOR:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=WINMOR\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "  DEBUGLOG TRUE\r\n");
		ptr += sprintf(ptr, "  BUSYLOCK TRUE\r\n");
		ptr += sprintf(ptr, "  CWID TRUE\r\n");
		ptr += sprintf(ptr, "  BW 1600\r\n");
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;

	case H_KISSHF:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, " DRIVER=KISSHF\r\n");
		ptr += sprintf(ptr, " INTERLOCK=1\r\n");
		ptr += sprintf(ptr, " CONFIG\r\n");
		ptr += sprintf(ptr, "  ADDR %s %d\r\n", PORT->Param1, PORT->Param2);
		ptr += sprintf(ptr, "ENDPORT\r\n");

		break;


	default:

		ptr += sprintf(ptr, "PORT\r\n");
		ptr += sprintf(ptr, " PORTNUM=%d\r\n", port + 1);
		ptr += sprintf(ptr, " ID=%s\r\n", PORT->ID);
		ptr += sprintf(ptr, "ENDPORT\r\n");

		msgBox.setWindowTitle("Port Config");
		msgBox.setText("Autoconfig not supported - Edit manually");
		msgBox.move(mainpos.x() + 200, mainpos.y() + 300);
		msgBox.exec();
	}

	refreshPorts();

	if (port == nextPortnum)  // PORTNUM Not Used
		nextPortnum++;

	PortDialog::accept();
}

void PortDialog::myreject()
{
	PortDialog::reject();
}

void BPQConfigGen::editPort(int i)
{
	if (Ports[i].portConfig)
	{
		editPortDialog dialog(i);
		dialog.exec();
	}
}


QTextEdit * textEdit;
int editPort;

editPortDialog::editPortDialog(int n, QWidget *parent) : QDialog(parent)
{
	struct PORTREC * PORT = &Ports[n];

	this->resize(550, 400);
	this->move(mainpos.x() + 50, mainpos.y() + 50);

	setWindowTitle(tr("Edit Port"));

	textEdit = new QTextEdit(this);
	textEdit->setText(PORT->portConfig);

	textEdit->setGeometry(QRect(5, 5, 540, 370));

	if (PORT->PortType == H_TELNET)
	{
		int i;

		this->resize(550, 600);

		UserLabel = new QLabel("   User                   Password                Callsign        Appl", this);
		UserLabel->setGeometry(QRect(5, 380, 490, 20));

		userscrollArea = new QScrollArea(this);
		userscrollArea->setObjectName(QString::fromUtf8("scrollArea"));
		userscrollArea->setGeometry(QRect(5, 405, 490, 170));
		userscrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		userscrollArea->setWidgetResizable(false);
		userScroll = new QWidget();
		userScroll->setObjectName(QString::fromUtf8("portScroll"));
		userScroll->setGeometry(QRect(0, 0, 450, 2000));
		userscrollArea->setWidget(userScroll);

		int spares = 0;

		for (i = 0; i < 1024 && spares < 20; i++)
		{
			struct UserRec * USER = &Users[i];

			USER->gUserName = new QLineEdit(userScroll);
			USER->gUserName->setGeometry(QRect(5, 22 * i + 5, 95, 18));

			USER->gPassword = new QLineEdit(userScroll);
			USER->gPassword->setGeometry(QRect(105, 22 * i + 5, 95, 18));

			USER->gCallsign = new QLineEdit(userScroll);
			USER->gCallsign->setGeometry(QRect(205, 22 * i + 5, 75, 18));


			USER->gAppl = new QLineEdit(userScroll);
			USER->gAppl->setGeometry(QRect(285, 22 * i + 5, 95, 18));

			USER->gSYSOP = new QCheckBox("SYSOP", userScroll);
			USER->gSYSOP->setGeometry(QRect(385, 22 * i + 5, 70, 18));

			if (USER->UserName)
			{
				USER->gUserName->setText(USER->UserName);
				USER->gPassword->setText(USER->Password);
				USER->gCallsign->setText(USER->Callsign);
				USER->gAppl->setText(USER->Appl);
				USER->gSYSOP->setChecked(USER->Secure);
			}
			else
				spares++;

		}


		userScroll->setGeometry(QRect(0, 0, 450, i * 22 + 10));

		buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
		buttonBox->setGeometry(QRect(250, 580, 100, 18));
	}
	else if (PORT->PortType == H_AXIP)
	{
		int i;

		this->resize(550, 600);

		UserLabel = new QLabel("                                                      MAP Table\n   Call                   Host                                   Port         Type                SourcePort", this);
		UserLabel->setGeometry(QRect(5, 380, 540, 30));

		userscrollArea = new QScrollArea(this);
		userscrollArea->setObjectName(QString::fromUtf8("scrollArea"));
		userscrollArea->setGeometry(QRect(5, 415, 540, 160));
		userscrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		userscrollArea->setWidgetResizable(false);
		userScroll = new QWidget();
		userScroll->setObjectName(QString::fromUtf8("portScroll"));
		userScroll->setGeometry(QRect(0, 0, 540, 2000));
		userscrollArea->setWidget(userScroll);

		int spares = 0;

		QStringList items;
		
		items << "UDP" << "TCPMaster" << "TCPSlave" << "IP" ;

		for (i = 0; i < 128 && spares < 10; i++)
		{
			struct MAPRec * MAP = &Maps[i];

			MAP->gCallsign = new QLineEdit(userScroll);
			MAP->gCallsign->setGeometry(QRect(5, 22 * i + 5, 75, 18));

			MAP->gHost = new QLineEdit(userScroll);
			MAP->gHost->setGeometry(QRect(85, 22 * i + 5, 170, 18));

			MAP->gPort = new QLineEdit(userScroll);
			MAP->gPort->setGeometry(QRect(260, 22 * i + 5, 55, 18));

			MAP->gUDP = new QComboBox(userScroll);
			MAP->gUDP->addItems(items);
			MAP->gUDP->setGeometry(QRect(325, 22 * i + 5, 95, 18));

			MAP->gSourcePort = new QLineEdit(userScroll);
			MAP->gSourcePort->setGeometry(QRect(425, 22 * i + 5, 55, 18));

			if (MAP->Callsign)
			{
				MAP->gCallsign->setText(MAP->Callsign);
				MAP->gHost->setText(MAP->Host);
				MAP->gPort->setText(MAP->Port);
				MAP->gUDP->setCurrentIndex(MAP->UDP);
				if (MAP->SourcePort)
					MAP->gSourcePort->setText(MAP->SourcePort);
			}
			else
				spares++;

		}

		userScroll->setGeometry(QRect(0, 0, 500, i * 22 + 10));

		buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
		buttonBox->setGeometry(QRect(250, 580, 100, 18));
	}
	else
	{
		buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
		buttonBox->setGeometry(QRect(200, 380, 100, 18));
	}

	editPort = n;

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(myreject()));
}




void editPortDialog::myaccept()
{
	QString qs = textEdit->toPlainText();
	QByteArray qb = qs.toLocal8Bit();

	struct PORTREC * PORT = &Ports[editPort];


	free(PORT->portConfig);
	PORT->portConfig = (char *)malloc(qb.length() + 32769);
	memcpy(PORT->portConfig, qb.data(), qb.length());
	PORT->portConfig[qb.length()] = 0;

	// if Telnet Port get the User Config

	if (PORT->PortType == H_TELNET)
	{
		QByteArray ba;
		QString val;
		int i;
		NextUser = 0;

		for (i = 0; i < 1024; i++)
		{
			struct UserRec * USER = &Users[i];

			val = USER->gUserName->text();
			ba = val.toLatin1();

			if (ba.length() == 0)
				break;

			if (USER->UserName)
				free(USER->UserName);

			USER->UserName = strdup(ba.data());

			val = USER->gPassword->text();
			ba = val.toLatin1();

			if (USER->Password)
				free(USER->Password);

			USER->Password = strdup(ba.data());

			val = USER->gCallsign->text();
			ba = val.toLatin1();

			if (ba.length() == 0)
				break;

			if (USER->Callsign)
				free(USER->Callsign);

			USER->Callsign = strdup(USER->gCallsign->text().toUtf8().toUpper());
			USER->gCallsign->setText(USER->Callsign);

			val = USER->gAppl->text();
			ba = val.toLatin1();

			if (USER->Appl)
				free(USER->Appl);

			USER->Appl = strdup(ba.data());

			USER->Secure = USER->gSYSOP->isChecked();
		}
		NextUser = i;
	}

	if (PORT->PortType == H_AXIP)
	{
		QByteArray ba;
		QString val;
		int i;
		NextUser = 0;

		for (i = 0; i < 128; i++)
		{
			struct MAPRec * MAP = &Maps[i];

			val = MAP->gCallsign->text();
			ba = val.toLatin1();
			if (ba.length() == 0)
				break;

			if (MAP->Callsign)
				free(MAP->Callsign);

			MAP->Callsign = strdup(MAP->gCallsign->text().toUtf8().toUpper());
			MAP->gCallsign->setText(MAP->Callsign);

			val = MAP->gHost->text();
			ba = val.toLatin1();

			if (MAP->Host)
				free(MAP->Host);

			MAP->Host = strdup(MAP->gHost->text().toUtf8());

			val = MAP->gPort->text();
			ba = val.toLatin1();

			if (MAP->Port)
				free(MAP->Port);

			MAP->Port = strdup(MAP->gPort->text().toUtf8());

			val = MAP->gSourcePort->text();
			ba = val.toLatin1();

			if (MAP->SourcePort)
				free(MAP->SourcePort);

			MAP->SourcePort = strdup(MAP->gSourcePort->text().toUtf8());

			MAP->UDP = MAP->gUDP->currentIndex();

		}
		NextMap = i;
	}

	editPortDialog::accept();
}

void editPortDialog::myreject()
{
	editPortDialog::reject();
}

editPortDialog::~editPortDialog()
{
}

void BPQConfigGen::saveAppl(int i)
{
	struct APPL * pAppl = &Appls[i];
	QByteArray ba;

	QString val = gapplNum[i]->text();
	ba = val.toLatin1();
	pAppl->Num = atoi(ba.data());

	val = gapplCmd[i]->text().toUpper();
	ba = val.toLatin1();
	strcpy(pAppl->Command, ba.data());

	val = gapplCmdAlias[i]->text();
	ba = val.toLatin1();
	strcpy(pAppl->CommandAlias, ba.data());

	val = gapplCall[i]->text().toUpper();
	ba = val.toLatin1();
	strcpy(pAppl->ApplCall, ba.data());
	
	val = gapplAlias[i]->text().toUpper();
	ba = val.toLatin1();
	strcpy(pAppl->ApplAlias, ba.data());

	val = gapplQual[i]->text();
	ba = val.toLatin1();
	pAppl->ApplQual = atoi(ba.data()); 

	refreshAppls();
}



editOtherDialog::editOtherDialog(int n, QWidget *parent) : QDialog(parent)
{
	this->resize(610, 510);

	this->move(mainpos.x() + 50, mainpos.y() + 50);

	setWindowTitle(tr("Edit Main Config"));

	textEdit = new QTextEdit(this);
	textEdit->setText(mainConfig);
	textEdit->setGeometry(QRect(5, 5, 600, 470));

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttonBox->setGeometry(QRect(250, 485, 100, 18));

	editPort = n;

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(myreject()));
}

void editOtherDialog::myaccept()
{
	QString qs = textEdit->toPlainText();
	QByteArray qb = qs.toLocal8Bit();

	memcpy(mainConfig, qb.data(), qb.length());
	mainConfig[qb.length()] = 0;

	editOtherDialog::accept();
}

void editOtherDialog::myreject()
{
	editOtherDialog::reject();
}

editOtherDialog::~editOtherDialog()
{
}


editAPRSDialog::editAPRSDialog(int n, QWidget *parent) : QDialog(parent)
{
	this->resize(610, 510);

	this->move(mainpos.x() + 50, mainpos.y() + 50);

	setWindowTitle(tr("Edit APRS Config"));

	textEdit = new QTextEdit(this);
	textEdit->setText(aprsConfig);
	textEdit->setGeometry(QRect(5, 5, 600, 470));

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttonBox->setGeometry(QRect(250, 485, 100, 18));

	editPort = n;

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(myreject()));
}

void editAPRSDialog::myaccept()
{
	QString qs = textEdit->toPlainText();
	QByteArray qb = qs.toLocal8Bit();

	memcpy(aprsConfig, qb.data(), qb.length());
	aprsConfig[qb.length()] = 0;

	editAPRSDialog::accept();
}

void editAPRSDialog::myreject()
{
	editAPRSDialog::reject();
}

editAPRSDialog::~editAPRSDialog()
{
}

#define BOOL int
#define FALSE 0
#define TRUE 1
#define MAX_PATH 260


char APRSCall[10] = "";
char APRSDest[10] = "APBPQ1";

char LoppedAPRSCall[10];

char WXPortList[128];		// Ports to send WX to
char WXFileName[MAX_PATH];
char WXComment[80];
int WXInterval = 0;

BOOL GPSOK = 0;
char RunProgram[128] = "";				// Program to start

char LAT[] = "0000.00N";	// in standard APRS Format      
char LON[] = "00000.00W";	//in standard APRS Format

char HostName[80];			// for BlueNMEA
BOOL BlueNMEAOK = FALSE;
int BlueNMEATimer = 0;

int MaxTraceHops = 2;
int MaxFloodHops = 2;
int MaxDigisforIS = 7;			// Dont send to IS if more digis uued to reach us

int ExpireTime = 120;
int MaxStations = 1000;

BOOL GPSSetsLocator = 0;	// Update Map Location from GPS
char GPSPort[80] = "";
int GPSSpeed = 0;
char GPSRelay[80] = "";

BOOL GateLocal = FALSE;
char GateLocalDistance[80] = "";
double SOG, COG;		// From GPS

double Lat = 0.0;
double Lon = 0.0;

BOOL PosnSet = FALSE;
/*
The null position should be include the \. symbol (unknown/indeterminate
position). For example, a Position Report for a station with unknown position
will contain the coordinates …0000.00N\00000.00W.…
*/
char FloodCalls[256] = "";			// Calls to relay using N-n without tracing
char TraceCalls[256] = "";			// Calls to relay using N-n with tracing
char DigiCalls[256] = "";			// Calls for normal relaying

int ISPort = 0;
char ISHost[256] = "";
int ISPasscode = 0;
char NodeFilter[1000] = "";		// Filter when the isn't an application
char ISFilter[1000] = "";		// Current Filter
char APPLFilter[1000] = "";			// Filter when an Applcation is running

BOOL IGateEnabled;

char StatusMsg[256] = "";

char CFGSYMBOL[3] = "a";
char CFGSYMSET[3] = "B";

BOOL TraceDigi = FALSE;					// Add Trace to packets relayed on Digi Calls
BOOL SATGate = FALSE;					// Delay Gating to IS directly heard packets
BOOL LogAPRSIS = FALSE;
BOOL BeacontoIS = FALSE;

BOOL DefaultLocalTime = FALSE;
BOOL DefaultDistKM = FALSE;

int BeaconInterval = 0;
int MobileBeaconInterval = 0;
int MobileBeaconIntervalSecs = 0;

int multiple = 0;						// Allows multiple copies of LinBPQ/APRS on one machine


// Code to parse APRS definiton

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;
	ptr = malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

int APRSProcessLine(char * buf)
{
	char * ptr, *p_value;

	ptr = strtok(buf, "= \t\n\r");

	if (ptr == NULL) return (TRUE);

//	 OBJECT PATH=APRS,WIDE1-1 PORT=1,IS INTERVAL=30 TEXT=;444.80TRF*111111z4807.60N/09610.63Wr%156 R15m

	if (stricmp(ptr, "OBJECT") == 0)
	{
		char * p_Path, *p_Port, *p_Text;
		int Interval;
		struct OBJECT * Object = &Objects[NextObject];;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (stricmp(p_value, "PATH"))
			return FALSE;

		p_Path = strtok(NULL, "\t\n\r ");
		if (p_Path == NULL) return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (stricmp(p_value, "PORT"))
			return FALSE;

		p_Port = strtok(NULL, "\t\n\r ");
		if (p_Port == NULL) return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (stricmp(p_value, "INTERVAL"))
			return FALSE;

		p_value = strtok(NULL, " \t");
		if (p_value == NULL) return FALSE;

		Interval = atoi(p_value);

		if (Interval == 0)
			return FALSE;

		p_value = strtok(NULL, "=");
		if (p_value == NULL) return FALSE;
		if (stricmp(p_value, "TEXT"))
			return FALSE;

		p_Text = strtok(NULL, "\n\r");
		if (p_Text == NULL) return FALSE;

		if (Interval < 10)
			Interval = 10;

		Object->Interval = Interval;

		strcpy(Object->Path, p_Path);
		strcpy(Object->PortMap, p_Port);
		strcpy(Object->Message, p_Text);

		NextObject++;

		return TRUE;
	}

	if (stricmp(ptr, "STATUSMSG") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		if (p_value)
			memcpy(StatusMsg, p_value, 128);	// Just in case too long
		return TRUE;
	}

	if (stricmp(ptr, "WXFileName") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		if (p_value)
			strcpy(WXFileName, p_value);
		return TRUE;
	}
	if (stricmp(ptr, "WXComment") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");

		if (p_value)
		{
			if (strlen(p_value) > 79)
				p_value[80] = 0;

			strcpy(WXComment, p_value);
		}
		return TRUE;
	}


	if (stricmp(ptr, "ISFILTER") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		if (p_value)
			strcpy(ISFilter, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "ReplaceDigiCalls") == 0)
	{
		TraceDigi = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "Multiple") == 0)
	{
		multiple = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "SATGate") == 0)
	{
		SATGate = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "DISTKM") == 0)
	{
		DefaultDistKM = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "LOCALTIME") == 0)
	{
		DefaultLocalTime = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "LOGAPRSIS") == 0)
	{
		LogAPRSIS = TRUE;
		return TRUE;
	}

	p_value = strtok(NULL, " \t\n\r");

	if (p_value == NULL)
		return FALSE;

	if (stricmp(ptr, "APRSCALL") == 0)
	{
		strcpy(APRSCall, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "APRSPATH") == 0)
	{
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		struct PORTREC * PORT = &Ports[Port - 1];

		// if not yet defined (because reading APRS first or invalid number create it

		if (PORT->portConfig == nullptr)
		{
			strcpy(PORT->ID, "Missing Port");
			PORT->portConfig = strdup("");
		}

		if (Context == NULL || Context[0] == 0)
			return TRUE;					// No dest - a receive-only port

		PORT->Pathval = strdup(strupr(Context));
		PORT->Use = 1;
		return TRUE;
	}


	if (stricmp(ptr, "DIGIMAP") == 0)
	{
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		struct PORTREC * PORT = &Ports[Port - 1];

		// if not yet defined (because reading APRS first or invalid number create it

		if (PORT->portConfig == nullptr)
		{
			strcpy(PORT->ID, "Missing Port");
			PORT->portConfig = strdup("");
		}

		if (Context == NULL || Context[0] == 0)
			return FALSE;

		PORT->Digival = strdup(strupr(Context));
		PORT->Use = 1;

		return TRUE;
	}

	if (stricmp(ptr, "BRIDGE") == 0)
	{
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		struct PORTREC * PORT = &Ports[Port - 1];

		// if not yet defined (because reading APRS first or invalid number create it

		if (PORT->portConfig == nullptr)
		{
			strcpy(PORT->ID, "Missing Port");
			PORT->portConfig = strdup("");
		}

		if (Context == NULL || Context[0] == 0)
			return FALSE;

		PORT->Bridgeval = strdup(strupr(Context));
		PORT->Use = 1;
		return TRUE;
	}

	if (stricmp(ptr, "BeaconInterval") == 0)
	{
		BeaconInterval = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "MobileBeaconInterval") == 0)
	{
		MobileBeaconInterval = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "MobileBeaconIntervalSecs") == 0)
	{
		MobileBeaconInterval = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "BeacontoIS") == 0)
	{
		BeacontoIS = atoi(p_value);
		return TRUE;
	}


	if (stricmp(ptr, "TRACECALLS") == 0)
	{
		strcpy(TraceCalls, strupr(p_value));
		return TRUE;
	}

	if (stricmp(ptr, "FLOODCALLS") == 0)
	{
		 strcpy(FloodCalls, strupr(p_value));
		return TRUE;
	}

	if (stricmp(ptr, "DIGICALLS") == 0)
	{
		strcpy(DigiCalls, strupr(p_value));
		return TRUE;
	}

	if (stricmp(ptr, "MaxStations") == 0)
	{
		MaxStations = atoi(p_value);

		if (MaxStations > 10000)
			MaxStations = 10000;

		return TRUE;
	}

	if (stricmp(ptr, "MaxAge") == 0)
	{
		ExpireTime = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "GPSPort") == 0)
	{
		if (strcmp(p_value, "0") != 0)
			strcpy(GPSPort, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "GPSSpeed") == 0)
	{
		GPSSpeed = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "GPSRelay") == 0)
	{
		if (strlen(p_value) > 79)
			return FALSE;

		strcpy(GPSRelay, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "BlueNMEA") == 0)
	{
		if (strlen(p_value) > 70)
			return FALSE;

		strcpy(HostName, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "GPSSetsLocator") == 0)
	{
		GPSSetsLocator = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "LAT") == 0)
	{
		if (strlen(p_value) != 8)
			return FALSE;

		memcpy(LAT, strupr(p_value), 8);
		PosnSet = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "LON") == 0)
	{
		if (strlen(p_value) != 9)
			return FALSE;

		memcpy(LON, strupr(p_value), 9);
		PosnSet = TRUE;
		return TRUE;
	}

	if (stricmp(ptr, "SYMBOL") == 0)
	{
		if (p_value[0] > ' ' && p_value[0] < 0x7f)
			CFGSYMBOL[0] = p_value[0];

		return TRUE;
	}

	if (stricmp(ptr, "SYMSET") == 0)
	{
		CFGSYMSET[0] = p_value[0];
		return TRUE;
	}

	if (stricmp(ptr, "MaxTraceHops") == 0)
	{
		MaxTraceHops = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "MaxFloodHops") == 0)
	{
		MaxFloodHops = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "ISHOST") == 0)
	{
		strncpy(ISHost, p_value, 250);
		return TRUE;
	}

	if (stricmp(ptr, "ISPORT") == 0)
	{
		ISPort = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "ISPASSCODE") == 0)
	{
		ISPasscode = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "MaxDigisforIS") == 0)
	{
		MaxDigisforIS = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "GateLocalDistance") == 0)
	{
		strcpy(GateLocalDistance, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "WXInterval") == 0)
	{
		WXInterval = atoi(p_value);
		return TRUE;
	}

	if (stricmp(ptr, "WXPortList") == 0)
	{
		strcpy(WXPortList, p_value);
		return TRUE;
	}

	if (stricmp(ptr, "Run") == 0)
	{
		strcpy(RunProgram, p_value);
		return TRUE;
	}

	//
	//	Bad line
	//
	return (FALSE);
}

void BPQConfigGen::WriteAPRSConfig(FILE *fp)
{
	if (!ui.EnableAPRS->isChecked())
		return;

	fprintf(fp, "APRSDIGI\r\n");

	fprintf(fp, " APRSCALL=%s\r\n", APRSCall);
	if (RunProgram[0])
		fprintf(fp, " RUN=%s\r\n", RunProgram);
	fprintf(fp, " Symbol=%s\r\n", CFGSYMBOL);
	fprintf(fp, " Symset=%s\r\n", CFGSYMSET);

	if (DefaultDistKM)
		fprintf(fp, " DISTKM\r\n");

	if (DefaultLocalTime)
		fprintf(fp, " LOCALTIME\r\n");

	fprintf(fp, " StatusMsg=%s\r\n", StatusMsg);
	fprintf(fp, " MaxStations=%d\r\n", MaxStations);
	fprintf(fp, " MaxAge=%d\r\n", ExpireTime);
	fprintf(fp, " BeaconInterval=%d\r\n", BeaconInterval);

	for (int i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		if (PORT->Use)
		{
			if (PORT->Pathval)
				fprintf(fp, " APRSPATH %d=%s\r\n", i + 1, PORT->Pathval);
			else
				fprintf(fp, " APRSPATH %d=\r\n", i + 1);
		}
	}

	for (int i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		if (PORT->Use)
		{
			if (PORT->Digival && PORT->Digival[0])
				fprintf(fp, " DIGIMAP %d=%s\r\n", i + 1, PORT->Digival);
		}
	}

	for (int i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		if (PORT->Use)
		{
			if (PORT->Bridgeval && PORT->Bridgeval[0])
				fprintf(fp, " BRIDGE %d=%s\r\n", i + 1, PORT->Bridgeval);
		}
	}

	// Objects

	for (int i = 0; i < NextObject; i++)
	{
		struct OBJECT * Object = &Objects[i];

		fprintf(fp, " OBJECT PATH=%s PORT=%s INTERVAL=%d TEXT=%s;\r\n",
			Object->Path, Object->PortMap, Object->Interval, Object->Message);
	}

	fprintf(fp, " TraceCalls=%s\r\n", TraceCalls);
	fprintf(fp, " FloodCalls=%s\r\n", FloodCalls);
	fprintf(fp, " DigiCalls=%s\r\n", DigiCalls);
	fprintf(fp, " MaxTraceHops=%d\r\n", MaxTraceHops);
	fprintf(fp, " MaxFloodHops=%d\r\n", MaxFloodHops);

	if (TraceDigi)
		fprintf(fp, " ReplaceDigiCalls\r\n");

	fprintf(fp, " GPSPort=%s\r\n", GPSPort);
	fprintf(fp, " GPSSpeed=%d\r\n", GPSSpeed);
	fprintf(fp, " GPSSetsLocator=%d\r\n", GPSSetsLocator);

	fprintf(fp, " LAT=%s\r\n", LAT);
	fprintf(fp, " LON=%s\r\n", LON);
	
	fprintf(fp, " ISHost=%s\r\n", ISHost);
	fprintf(fp, " ISPort=%d\r\n", ISPort);
	fprintf(fp, " ISPasscode=%d\r\n", ISPasscode);
	fprintf(fp, " ISFilter=%s\r\n", ISFilter);
	fprintf(fp, " BeacontoIS=%d\r\n", BeacontoIS);
	if (GateLocalDistance[0])
		fprintf(fp, " GateLocalDistance=%s\r\n", GateLocalDistance);

	if (SATGate)
		fprintf(fp, " SATGate\r\n");

	if (LogAPRSIS)
		fprintf(fp, " LogAPRSIS\r\n");

	if (WXFileName[0] && WXPortList[0])
	{
		fprintf(fp, " WXFileName=%s\r\n", WXFileName);
		fprintf(fp, " WXPortList=%s\r\n", WXPortList);
		if (WXComment[0])
			fprintf(fp, " WXComment=%s\r\n", WXComment);
		fprintf(fp, " WXInterval=%d\r\n", WXInterval);
	}

	// Add any unrecognised lines

	fprintf(fp, aprsConfig);

	fprintf(fp, "****\r\n");

}


void BPQConfigGen::refreshAPRS()
{
	int i, line = 0;

	ui.APRSCall->setText(APRSCall);
	ui.Symbol->setText(CFGSYMBOL);
	ui.SymbolSet->setText(CFGSYMSET);
	ui.LocalTime->setChecked(DefaultLocalTime);
	ui.DistanceKM->setChecked(DefaultDistKM);
	ui.StatusMessage->setText(StatusMsg);
	ui.MaxStations->setText(QString::number(MaxStations));
	ui.MaxAge->setText(QString::number(ExpireTime));
	ui.BeaconInterval->setText(QString::number(BeaconInterval));

	for (i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		if (PORT->portConfig || PORT->Use)
		{
			Ports[i].Enable->setChecked(PORT->Use);
			Ports[i].PortNo->setText(QString::number(i + 1));
			Ports[i].Port->setText(PORT->ID);
			if (PORT->Pathval)
				Ports[i].Path->setText(PORT->Pathval);
			if (PORT->Digival)
				Ports[i].Digi->setText(PORT->Digival);
			if (PORT->Bridgeval)
				Ports[i].Bridge->setText(PORT->Bridgeval);
			line++;
		}
	}

	for (i = 0; i < NextObject; i++)
	{
		struct OBJECT * Object = &Objects[i];

		Object->ObjPath->setText(Object->Path);
		Object->ObjPorts->setText(Object->PortMap);
		Object->ObjInterval->setText(QString::number(Object->Interval));
		Object->ObjText->setText(Object->Message);
		aprsSetVisible(i, 1);
	}
	aprsSetVisible(i++, 1);
	aprsSetVisible(i++, 1);

	ui.TraceCalls->setText(TraceCalls);
	ui.FloodCalls->setText(FloodCalls);
	ui.DigiCalls->setText(DigiCalls);
	ui.MaxHops->setText(QString::number(MaxTraceHops));
	ui.ReplaceDigiCalls->setChecked(TraceDigi);

	ui.GPSPort->setText(GPSPort);
	ui.GPSSpeed->setText(QString::number(GPSSpeed));
	ui.GPSSetsLocator->setChecked(GPSSetsLocator);

	ui.Lat->setText(LAT);
	ui.Long->setText(LON);

	ui.ISHost->setText(ISHost);
	ui.ISPort->setText(QString::number(ISPort));
	ui.ISPassCode->setText(QString::number(ISPasscode));
	ui.BeacontoIS->setChecked(BeacontoIS);
	ui.ISFilter->setText(ISFilter);
	ui.SatGate->setChecked(SATGate);
	ui.LogAPRSIS->setChecked(LogAPRSIS);
	ui.WXFileName->setText(WXFileName);
	ui.WXPath->setText(WXPortList);
	ui.WXInterval->setText(QString::number(WXInterval));
	ui.WXComment->setText(WXComment);
}

void BPQConfigGen::SaveAPRS()
{
	QString val;
	QByteArray ba;
	int i;

	strcpy(APRSCall, ui.APRSCall->text().toUtf8());
	strcpy(CFGSYMBOL, ui.Symbol->text().toUtf8());
	strcpy(CFGSYMSET, ui.SymbolSet->text().toUtf8());

	DefaultLocalTime = ui.LocalTime->isChecked();
	DefaultDistKM = ui.DistanceKM->isChecked();

	strcpy(StatusMsg, ui.StatusMessage->text().toUtf8());

	MaxStations = ui.MaxStations->text().toInt(0);
	BeaconInterval = ui.BeaconInterval->text().toInt(0);
	ExpireTime = ui.MaxAge->text().toInt(0);

	for (i = 0; i < 32; i++)
	{
		struct PORTREC * PORT = &Ports[i];

		PORT->Use = PORT->Enable->isChecked();

		PORT->Pathval = strdup(PORT->Path->text().toUtf8());
		PORT->Digival = strdup(PORT->Digi->text().toUtf8());
		PORT->Bridgeval = strdup(PORT->Bridge->text().toUtf8());
	}

	for (int i = 0; i < NextObject; i++)
	{
		struct OBJECT * Object = &Objects[i];

		strcpy(Object->Path, Object->ObjPath->text().toUtf8());
		strcpy(Object->PortMap, Object->ObjPorts->text().toUtf8());
		strcpy(Object->Path, Object->ObjPath->text().toUtf8());
		strcpy(Object->Message, Object->ObjText->text().toUtf8());

		Object->ObjPath->setText(Object->Path);
		Object->ObjPorts->setText(Object->PortMap);
		Object->Interval = Object->ObjInterval->text().toInt();
		Object->ObjText->setText(Object->Message);
	}

	strcpy(TraceCalls, ui.TraceCalls->text().toUtf8());
	strcpy(FloodCalls, ui.FloodCalls->text().toUtf8());
	strcpy(DigiCalls, ui.DigiCalls->text().toUtf8());
	MaxTraceHops = ui.MaxHops->text().toInt();
	TraceDigi = ui.ReplaceDigiCalls->isChecked();

	strcpy(GPSPort, ui.GPSPort->text().toUtf8());
	GPSSpeed = ui.GPSSpeed->text().toInt();
	GPSSetsLocator = ui.GPSSetsLocator->isChecked();

	strcpy(LAT, ui.Lat->text().toUtf8());
	strcpy(LON, ui.Long->text().toUtf8());

	strcpy(ISHost, ui.ISHost->text().toUtf8());
	ISPort = ui.ISPort->text().toInt();
	ISPasscode = ui.ISPassCode->text().toInt();
	BeacontoIS = ui.BeacontoIS->isChecked();
	strcpy(ISFilter, ui.ISFilter->text().toUtf8());

	SATGate = ui.SatGate->isChecked();
	LogAPRSIS = ui.LogAPRSIS->isChecked();

	strcpy(WXFileName, ui.WXFileName->text().toUtf8());
	strcpy(WXPortList, ui.WXPath->text().toUtf8());
	strcpy(WXComment, ui.WXComment->text().toUtf8());
	WXInterval = ui.WXInterval->text().toInt();
	
	QMessageBox msgBox;
	msgBox.setWindowTitle("Save");
	msgBox.setText("APRS Changes saved. Remember to write the configuration to apply changes");
	msgBox.exec();

	ui.tabWidget->setCurrentIndex(1);
}

// Install/Update

int stateMachine = 0;
int Mode;

void BPQConfigGen::getVersion(char * Program, char * Version)
{
#ifdef WIN32

	DWORD               dwSize = 0;
	BYTE                *pbVersionInfo = NULL;
	VS_FIXEDFILEINFO    *pFileInfo = NULL;
	UINT                puLenFileInfo = 0;

	// Get the version information for the file requested

	dwSize = GetFileVersionInfoSizeA(Program, NULL);

	if (dwSize == 0)
	{
		sprintf(Version, "Not Found");
		return;
	}

	pbVersionInfo = new BYTE[dwSize];

	if (!GetFileVersionInfoA(Program, 0, dwSize, pbVersionInfo))
	{
		sprintf(Version, "Error in GetFileVersionInfo: %d", GetLastError());
		delete[] pbVersionInfo;
		return;
	}

	if (!VerQueryValue(pbVersionInfo, TEXT("\\"), (LPVOID*)&pFileInfo, &puLenFileInfo))
	{
		sprintf(Version, "Error in VerQueryValue: %d", GetLastError());
		delete[] pbVersionInfo;
		return;
	}

	// pFileInfo->dwFileVersionMS is usually zero. However, you should check
	// this if your version numbers seem to be wrong

	sprintf(Version, "%d.%d.%d.%d",
		(pFileInfo->dwFileVersionMS >> 16) & 0xff,
		(pFileInfo->dwFileVersionMS >> 0) & 0xff,
		(pFileInfo->dwFileVersionLS >> 16) & 0xff,
		(pFileInfo->dwFileVersionLS >> 0) & 0xff);


	// pFileInfo->dwProductVersionMS is usually zero. However, you should check
	// this if your version numbers seem to be wrong.

#else

	// run linbpq --version

	qDebug() << "Running prog to get version " << Program;


	QString program = Program;
	QStringList arguments;
	arguments << "-v";

	QProcess *myProcess = new QProcess();
	myProcess->start(program, arguments);

	if (!myProcess->waitForFinished(2000))
	{
		qDebug() << "Didn't Finish";
		strcpy(Version, "Not Found");
		myProcess->kill();
		return;
	}

	QByteArray out = myProcess->readAllStandardOutput();

	char * ptr = strstr(out.data(), "Ver");

	if (ptr == 0)
	{
		strcpy(Version, "Not Found");
		return;
	}

	ptr += 8;

	strlop(ptr, '\r');
	strlop(ptr, ' ');

	strcpy(Version, ptr);

#endif
}

extern char * pgm;

int IsUserAdmin()
/*++
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token.
Arguments: None.
Return Value:
   TRUE - Caller has Administrators local group.
   FALSE - Caller does not have Administrators local group. --
*/
{
#ifdef WIN32

	int b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = 0;
		}
		FreeSid(AdministratorsGroup);
	}

	return(b);
#endif
	return 1;
}
void Elevate()
{
#ifdef WIN32

	const TCHAR *opVerb = TEXT("runas");
	::ShellExecuteA(NULL, "runas", pgm, "adminrestart", configPath, 1);

	return;
#endif
}

void BPQConfigGen::getVersions()
{
	char verString[128];
	char xprogram[256];

#ifdef WIN32

	getVersion("BPQ32.DLL", verString);
	ui.Ver1->setText(verString);

	sprintf(xprogram, "%s/%s", programPath, "BPQMail.exe");
	getVersion(xprogram, verString);
	ui.Ver2->setText(verString);

	sprintf(xprogram, "%s/%s", programPath, "BPQChat.exe");
	getVersion(xprogram, verString);
	ui.Ver3->setText(verString);

	sprintf(xprogram, "%s/%s", programPath, "BPQAPRS.exe");
	getVersion(xprogram, verString);
	ui.Ver4->setText(verString);

#else

	ui.Prog1->setText("linbpq");
	ui.Prog2->setText("BPQAPRS");
	ui.Prog3->setText("");
	ui.Prog4->setText("");

	sprintf(xprogram, "%s/%s", programPath, "linbpq");

	getVersion(xprogram, verString);
	ui.Ver1->setText(verString);

	if (ARM)
		sprintf(xprogram, "%s/%s", programPath, "piBPQAPRS");
	else
		sprintf(xprogram, "%s/%s", programPath, "x86BPQAPRS");
	getVersion(xprogram, verString);
	ui.Ver2->setText(verString);

#endif
}

void BPQConfigGen::doUpdate(int Beta)
{
	QString str = ui.InstallDir->text();

	QSettings settings("G8BPQ", "BPQConfigGen");

	strcpy(programPath, str.toUtf8());

	settings.setValue("programPath", programPath);

	ui.updateLog->clear();

#ifdef WIN32

	if (Beta == 0)
	{
		// Release update - run installer

		QMessageBox msgBox(this);
		msgBox.setWindowTitle("Install/Update");
		msgBox.setTextFormat(Qt::RichText);   //this is what makes the links clickable
		msgBox.setText("On Windows BPQ32 should be installed or updated by running the installer<br>"
			"Click OK to download and run installer<br><br>"
			"This may take a while!");

		msgBox.exec();

		stateMachine = 9;
		Mode = Beta;
		downloadFile("LatestInstaller/LatestInstaller.txt", 0, 0);

		return;
	}

	// Windows beta update - download bpq32, mail, chat, aprs

	stateMachine = 12;
	Mode = Beta;

	downloadFile("bpq32.zip", Beta, ARM);
	return;

#else

	// Linux

	// can't just call downloadFile several times as it returns
	// before transfer is complete. And can't just wait, or the GUI freezes
	// so need separate thread or state machine driven from event loop

	stateMachine = 1;
	Mode = Beta;
	downloadFile("linbpq", Beta, ARM);

#endif
}

void BPQConfigGen::downloadFile(const char * Filename, int Beta, int ARM)
{
	char URL[256];
	char logLine[512];

	// Build download URL, including pi prefix if running on ARM

	strcpy(URL, "http://www.cantab.net/users/john.wiseman/Downloads/");

	if (Beta)
		strcat(URL, "Beta/");


	if (ARM)
		strcat(URL, "pi");
	else if (strcmp(Filename, "BPQAPRS") == 0)
		strcat(URL, "x86");

	strcat(URL, Filename);

	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished,
		this, &BPQConfigGen::replyFinished);

	sprintf(logLine, "Downloading %s\n", URL);

	ui.updateLog->insertPlainText(QString::fromUtf8(logLine));


	QNetworkReply * reply = manager->get(QNetworkRequest(QUrl(URL)));

	connect(reply, &QNetworkReply::downloadProgress,
		this, &BPQConfigGen::downloadProgress);

	connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
		[=](QNetworkReply::NetworkError)
	{
		qDebug() << reply->errorString();
	});

}



void BPQConfigGen::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	char logLine[512];

	sprintf(logLine, "Download progress.. received %lld of %lld", bytesReceived, bytesTotal);

	ui.updateLog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
	ui.updateLog->moveCursor(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
	ui.updateLog->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
	ui.updateLog->textCursor().removeSelectedText();

	ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
}

char InstallerName[256];

void BPQConfigGen::replyFinished(QNetworkReply *reply)
{
	int len = reply->bytesAvailable();
	QByteArray Data = reply->readAll();
	char logLine[512];
	QByteArray TempName;

#ifdef WIN32
	unsigned char * Prog = (unsigned char *)Data.data();
	char FN[256];
#endif

	sprintf(logLine, "\nDownloading Complete - Length %d\n", len);

	ui.updateLog->insertPlainText(QString::fromUtf8(logLine));

	reply->deleteLater();

	// save file

	stateMachine++;

	switch (stateMachine)
	{
	case 2:

		SaveDownloadedFile(&Data, len, "linbpq", programPath, false);
		downloadFile("BPQAPRS", Mode, ARM);
		return;

	case 3:

		if (ARM)
			SaveDownloadedFile(&Data, len, "piBPQAPRS", programPath, false);
		else
			SaveDownloadedFile(&Data, len, "x86BPQAPRS", programPath, false);

		// if there isn't an APRS symbol file get APRS base files

		{
			char path[256];

			sprintf(path, "%s/BPQAPRS/Symbols.jpg", programPath);

			QFileInfo check_file(path);

			if (check_file.exists() && check_file.isFile())
			{
				ui.updateLog->insertPlainText(path);
				ui.updateLog->insertPlainText(" exists - not downloading ARPS Base Files\n");

				stateMachine = 0;
				getVersions();
				return;
			}
		}

		downloadFile("LinBPQAPRS.zip", 0, 0);
		return;

	case 4:

		SaveDownloadedFile(&Data, len, "BPQAPRS.zip", programPath, false);

		// Linux download complete

		stateMachine = 0;
		getVersions();
		return;

#ifdef WIN32

	case 10:

		// Windows Installer Filename Download

		strcpy(InstallerName, Data);
		strlop(InstallerName, 10);
		strlop(InstallerName, 13);

		sprintf(FN, "LatestInstaller/%s", InstallerName);

		downloadFile(FN, Mode, ARM);
		return;

	case 11:

		// Windows download complete

	{
		// Extra block so Temp File goes out of scope

		char fulltempname[256];

		QTemporaryDir dir;

		QByteArray TempDir = dir.path().toLatin1();


		// The QTemporaryDir destructor removes the temporary directory
		// as it goes out of scope.

		QByteArray TempName;
		{
			// Extra block so Temp File goes out of scope

			TempName.append(TempDir);
			TempName.append("/XXXXXX.zip");

			QTemporaryFile file(TempName);

			if (file.open())
			{
				sprintf(logLine, "Writing to Temporary File ");
				ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
				ui.updateLog->insertPlainText(file.fileName());
				ui.updateLog->insertPlainText("\n");

				TempName = file.fileName().toLatin1();

				file.setAutoRemove(false);		// so it isn't deleted
				file.write(Data.data(), len);
				file.close();
			}
		}

		// Exec unzip 

		QStringList arguments;
		char dparam[256] = "-o";
		strcat(dparam, TempDir.data());
		arguments << "e" << dparam << "-pbpq32" << "-y" << TempName;

		QProcess *myProcess = new QProcess();
		myProcess->setProgram("7za.exe");
		myProcess->setArguments(arguments);

		sprintf(logLine, "Unzip to Temporary Directory ");
		ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
		ui.updateLog->insertPlainText(TempDir.data());
		ui.updateLog->insertPlainText("\n");

		myProcess->start();

		if (!myProcess->waitForFinished(10000))
		{
			qDebug() << "Didn't Finish";
			myProcess->kill();
		}

		QFile::remove(TempName);

		// Exec file. Just running file doesn't seem top work so use cmd.exe to run it

		{
			QString program = TempDir;
			program.append("/");
			char * ptr = strstr(InstallerName, "zip");
			if (ptr)
				*ptr = 0;

			program.append(InstallerName);
			program.append("exe");

			QStringList arguments;
			arguments << "/C" << program;
			QProcess *myProcess = new QProcess();
			myProcess->setProgram("cmd.exe");
			myProcess->setArguments(arguments);

			sprintf(logLine, "Running Installer ");
			ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
			ui.updateLog->insertPlainText(program);
			ui.updateLog->insertPlainText("\n");

			myProcess->start();

			if (!myProcess->waitForFinished(120000))
			{
				QProcess::ProcessError error;

				error = myProcess->error();

				QByteArray out = myProcess->readAllStandardOutput();
				qDebug() << out;
				qDebug() << "Didn't Finish";
				myProcess->kill();
			}

			QFile::remove(program);
		}
		getVersions();

		stateMachine = 0;
		return;
	}
	case 13:			// get next Windows file

		SaveDownloadedFile(&Data, len, "bpq32.dll", "c:/windows/system32", TRUE);
		downloadFile("BPQMail.zip", Mode, ARM);
		return;

	case 14:

		SaveDownloadedFile(&Data, len, "BPQMail.exe", programPath, TRUE);
		downloadFile("BPQChat.exe", Mode, ARM);
		return;

	case 15:

		SaveDownloadedFile(&Data, len, "BPQChat.exe", programPath, FALSE);
		downloadFile("BPQAPRS.exe", Mode, ARM);
		return;

	case 16:

		SaveDownloadedFile(&Data, len, "BPQAPRS.exe", programPath, FALSE);
		stateMachine = 0;
		getVersions();
		return;
#endif
	}
	return;

}

#ifndef WIN32
#include <sys/stat.h>
#endif

void BPQConfigGen::SaveDownloadedFile(QByteArray * Data, int len, const char * Name, char * programPath, bool unzip)
{
	char logLine[256];

	char fullName[256];
	char fullTempFile[256];
	sprintf(fullName, "%s/%s", programPath, Name);

	if (unzip)
	{
		// write to temporary file, unzip to temporary directory and
		// compare with original. If different unzip it to target
		// delete temporary files

		QTemporaryDir dir;
		if (dir.isValid()) {
			// dir.path() returns the unique directory path
		}

		QByteArray TempDir = dir.path().toLatin1();


		// The QTemporaryDir destructor removes the temporary directory
		// as it goes out of scope.

		QByteArray TempName;
		{
			// Extra block so Temp File goes out of scope

			QTemporaryFile file("XXXXXX.zip");

			if (file.open())
			{
				sprintf(logLine, "Writing to Temporary File ");
				ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
				ui.updateLog->insertPlainText(file.fileName());
				ui.updateLog->insertPlainText("\n");

				TempName = file.fileName().toLatin1();

				file.setAutoRemove(false);		// so it isn't deleted
				file.write(Data->data(), len);
				file.close();
			}
		}

		// Exec unzip 

		QStringList arguments;
		char dparam[256] = "-o";
		strcat(dparam, TempDir.data());
		arguments << "e" << dparam << "-pbpq32" << "-y" << TempName;

		QProcess *myProcess = new QProcess();
		myProcess->setProgram("7za.exe");
		myProcess->setArguments(arguments);

		sprintf(logLine, "Unzip to Temporary Directory ");
		ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
		ui.updateLog->insertPlainText(TempDir.data());
		ui.updateLog->insertPlainText("\n");

		myProcess->start();

		if (!myProcess->waitForFinished(10000))
		{
			qDebug() << "Didn't Finish";
			myProcess->kill();
		}

		// Read old and new files

		sprintf(fullTempFile, "%s/%s", TempDir.data(), Name);
		{
			QFile old(fullName);

			qDebug() << "Checking old file " << fullTempFile;

			if (old.open(QIODevice::ReadOnly))
			{
				QByteArray olddata = old.readAll();

				old.close();

				QFile newf(fullTempFile);
				if (newf.open(QIODevice::ReadOnly))
				{
					QByteArray newdata = newf.readAll();

					newf.close();

					if (olddata.length() == newdata.length()
						&& memcmp(olddata.data(), newdata.data(), newdata.length()) == 0)
					{
						// no change - just exit

						QFile::remove(TempName);
						ui.updateLog->insertPlainText("File unchanged - not updating\n");
						return;
					}
				}
			}
		}

		// Update needed - unzip again to target

		// if updating on Windows make sure we have admin access

		if (IsUserAdmin() == FALSE)
		{
			QMessageBox::StandardButton resBtn = QMessageBox::question(this, "BPQConfigGen",
				tr("Update needed. BPQConfigGen will restart with Admin Access to allow this\n"),
				QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

			if (resBtn == QMessageBox::Yes)
			{
				Elevate();
				exit(0);
			}
		}
		
		datestampedbackup(fullName);

		arguments.clear();

		strcpy(dparam, "-o");
		strcat(dparam, programPath);

		arguments << "e" << dparam << "-pbpq32" << "-y" << TempName;

		myProcess = new QProcess();
		myProcess->setProgram("7za.exe");
		myProcess->setArguments(arguments);

		sprintf(logLine, "Unzip to Target Directory ");
		ui.updateLog->insertPlainText(QString::fromUtf8(logLine));
		ui.updateLog->insertPlainText(programPath);
		ui.updateLog->insertPlainText("\n");

		myProcess->start();

		if (!myProcess->waitForFinished(10000))
		{
			qDebug() << "Didn't Finish";
			myProcess->kill();
		}

		QByteArray out = myProcess->readAllStandardOutput();
		qDebug() << out;

		QFile::remove(TempName);

//		char * outtext = out.data();

//		if (strstr(outtext, "Everything"))
//			ui.updateLog->insertPlainText("file extracted ok\n");
//		else
			ui.updateLog->insertPlainText(out);

		return;
	}

	// uncompressed - just check if changed then write output

	{
		// block so QFile goes out of scope

		qDebug() << "Checking old file " << fullName;

		QFile old(fullName);

		if (old.open(QIODevice::ReadOnly))
		{
			QByteArray olddata = old.readAll();

			old.close();

			if (olddata.length() == Data->length()
				&& memcmp(olddata.data(), Data->data(), Data->length()) == 0)
			{
				// no change - just exit

				ui.updateLog->insertPlainText("File unchanged - not updating\n");
				return;
			}
		}
	}

	// if updating on Windows make sure we have admin access

	if (IsUserAdmin() == FALSE)
	{
		QMessageBox::StandardButton resBtn = QMessageBox::question(this, "BPQConfigGen",
			tr("Update needed. BPQConfigGen will restart with Admin Access to allow this\n"),
			QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

		if (resBtn == QMessageBox::Yes)
		{
			Elevate();
			exit(0);
		}
	}

	datestampedbackup(fullName);

	QFile newf(fullName);

	if (newf.open(QIODevice::ReadWrite))
	{
		qDebug() << "Writing file " << fullName;

		newf.write(Data->data(), Data->length());

		ui.updateLog->insertPlainText("File ");
		ui.updateLog->insertPlainText(fullName);
		ui.updateLog->insertPlainText(" updated\n");
		newf.close();

#ifndef WIN32

		// if APRS symbol file unzip it

		if (strcmp(Name, "BPQAPRS.zip") == 0)
		{
			ui.updateLog->insertPlainText("Unzipping APRS Symbol File\n");

			QString program = "unzip";
			QStringList arguments;
			arguments << "-u" << fullName;

			QProcess *myProcess = new QProcess();
			myProcess->setWorkingDirectory(programPath);
			myProcess->start(program, arguments);

			if (!myProcess->waitForFinished(5000))
			{
				qDebug() << "Didn't Finish";
				myProcess->kill();
			}

			QByteArray out = myProcess->readAllStandardOutput();

			ui.updateLog->insertPlainText(out);
			QFile::remove(fullName);

			return;
		}

		chmod(fullName, S_IRWXU);
#endif
		return;
	}
	else
		qDebug() << "failed to weite " << fullName;

}

// All Parameters

char keywords[60][20] = {
"OBSINIT", "OBSMIN", "NODESINTERVAL", "L3TIMETOLIVE", "L4RETRIES", "L4TIMEOUT",
"BUFFERS", "PACLEN", "TRANSDELAY", "T3", "IDLETIME", "BBS",
"NODE",  "BBSALIAS", "BBSCALL",
 "MAXLINKS",
"MAXNODES", "MAXROUTES", "MAXCIRCUITS" , "MINQUAL",
"HIDENODES", "L4DELAY", "L4WINDOW",  "UNPROTO", "BBSQUAL",
"APPLICATIONS", "ENABLE_LINKED",
 "SIMPLE", "AUTOSAVE", "L4APPL", "NETROMCALL", "C_IS_CHAT", "MAXRTT", "MAXHOPS",		// IPGATEWAY= no longer allowed
"LogL4Connects", "SAVEMH", "ENABLEADIFLOG"
};           /* parameter keywords */







