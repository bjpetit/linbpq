#include "BPQConfigGenerator.h"
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
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2)
{
	unsigned char c1, c2;
	int  v;

	if (pStr1 == NULL)
	{
		return 1;
	}


	do {
		c1 = *pStr1++;
		c2 = *pStr2++;
		/* The casts are necessary when pStr1 is shorter & char is signed */
		v = tolower(c1) - tolower(c2);
	} while ((v == 0) && (c1 != '\0') && (c2 != '\0'));

	return v;
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


bool HadConfig = false;		// Set if we've read a config 
bool UnrecognisedLine = false;
bool HadARDOP = false;
bool HadWINMOR = false;
bool HadUZ7HO = false;
bool HadVARA = false;

int BBSAppl = 0;
int ChatAppl = 0;
int RMSAppl = 0;

int nextPortnum = 0;
int activePortnum = 0;	// May be different from above if PORTNUM used

char ValfromReg[MAX_PATH] = "";

QLabel * gPortNum[32];
QLineEdit * gPortName[32];
QLineEdit * gPortType[32];
QLineEdit * gParam1[32];
QLineEdit * gParam2[32];

char ID[32][33] = { "" };
char Param1[32][32] = { "" };
int Param2[32] = { 0 };
int PortType[32] = { 0 };

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

#define NUMBEROFTYPES 22

char Types[NUMBEROFTYPES][14] = {
	"AEA Pactor", "ARDOP", "AXIP", "BPQETHER",
	"FLDIGI", "HAL", "KAM Pactor", "LOOPBACK",
	"KISS Serial", "KISS TCP", "KISS UDP", "MULTIPSK", 
	"SCS Pactor", "SCS Tracker","SCS TRKMULTI","Serial",
	"TELNET", "UZ7HO", "V4", "VARA",
	"VKISS", "WINMOR"};

BPQConfigGenerator::BPQConfigGenerator(QWidget *parent)
	: QMainWindow(parent)
{
	int i;
	char Label[10];
	ui.setupUi(this);

	// Set up port list

	for (i = 0; i < 32; i++)
	{
		sprintf(Label, "%d", i + 1);
		gPortNum[i] = new QLabel(Label, ui.scrollAreaWidgetContents);
		gPortNum[i]->setGeometry(QRect(5, 22 * i + 5, 15, 18));

		gPortType[i] = new QLineEdit(ui.scrollAreaWidgetContents);
		gPortType[i]->setGeometry(QRect(20, 22 * i + 5, 100, 18));

	//	for (int j = 0; j < NUMBEROFTYPES; j++)
	//		gPortType[i]->addItem(&Types[j][0]);

		gParam1[i] = new QLineEdit(ui.scrollAreaWidgetContents);
		gParam1[i]->setGeometry(QRect(125, 22 * i + 5, 100, 18));

		gParam2[i] = new QLineEdit(ui.scrollAreaWidgetContents);
		gParam2[i]->setGeometry(QRect(230, 22 * i + 5, 60, 18));

		gPortName[i] = new QLineEdit(ui.scrollAreaWidgetContents);
		gPortName[i]->setGeometry(QRect(295, 22 * i + 5, 250, 18));

		portSetVisible(i, 0);
	}
	 
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
		// Try "BPQ Directory"

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

	if (ValfromReg[0] == 0)
		GetCurrentDirectoryA(MAX_PATH, ValfromReg);

	ui.Directory->setText(ValfromReg);

#else
	char * dir = getenv("PWD");
	ui.Directory->setText(dir);
#endif

	QObject::connect(ui.ReadConfig, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.Save, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.ADDPORT, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.ADDAPP, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.Update, SIGNAL(clicked()), this, SLOT(clickedSlot()));
}

void BPQConfigGenerator::clickedSlot()
{
	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "ReadConfig") == 0)
	{
		char FN[260];
		FILE * fp;
		FILE * savefp;

		char line[512];
		char * ptr;
		char * Context;
		int Port;
		bool Comment = false;
		bool inPort = false;		// parsing a PORT section
		bool gotUser = false;

		char * NODECALL = NULL;
		char * NODEALIAS = NULL;
		char * LOC = NULL;

		nextPortnum = 0;
		activePortnum = 0;

		for (int i = 0; i < 32; i++)
			portSetVisible(i, 0);
	
		sprintf(FN, "%s/bpq32x.cfg", ui.Directory->text().toLocal8Bit().constData());

		if ((fp = fopen(FN, "r")) == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		// Read first line and check that writted by us

		if (fgets(line, 512, fp) == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Empty bpq32.cfg");
			msgBox.exec();
			return;
		}

		if (strcmp(line, "; Written by BPQConfigGenerator\n") != 0)
		{
			// Warn
			int ret = QMessageBox::warning(this, tr("Warning!!"),
				tr("File not written by BPQConfigGenerator. You can continue but file may not"
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

			if (ptr == NULL || ptr[0] == 0)
				continue;

			if (ptr[0] == ';')
				continue;

			if (strlen(ptr) > 1)
			{
				if (memcmp(ptr, "/*", 2) == 0)
				{
					Comment = TRUE;
					continue;
				}
				else if (memcmp(ptr, "*/", 2) == 0)
				{
					Comment = FALSE; 
					continue;
				}
			}

			if (Comment)
				continue;
			
			strlop(Context, ';');

			if (stricmp(ptr, "#include") == 0)
			{
				savefp = fp;

				sprintf(FN, "%s/%s", ui.Directory->text().toLocal8Bit().constData(), Context);
				strlop(FN, 10);

				if ((fp = fopen(FN, "r")) == NULL)
				{
					QMessageBox msgBox;
					msgBox.setWindowTitle("Failed to open include file");
					msgBox.setText(FN);
					msgBox.exec();
			
					fp = savefp;
					savefp = NULL;
				}
			
				continue;			// get next line
			}


			if (inPort)
			{
				if (stricmp(ptr, "ENDPORT") == 0)
				{
					inPort = FALSE;
					portSetVisible(activePortnum, 1);
					gPortType[activePortnum]->setText(Types[PortType[activePortnum]]);
				
					if (Param1[activePortnum][0])
						gParam1[activePortnum]->setText(Param1[activePortnum]);

					if (Param2[activePortnum])
						gParam2[activePortnum]->setText(QString::number(Param2[activePortnum]));

					if (activePortnum == nextPortnum)  // PORTNUM Not Used
						nextPortnum++;
				
					continue;
				}

				if (stricmp(ptr, "ID") == 0)
				{
					char * id = strtok_s(NULL, "\r\n;", &Context);
					
					if (strlen(id) > 31)
						id[31] = 0;

					strcpy(ID[activePortnum], id);

					gPortName[activePortnum]->setText(id);
					continue;
				}

				if (stricmp(ptr, "TYPE") == 0)
				{
					char * type = strtok_s(NULL, " =\t\r\n", &Context);

					if (stricmp(type, "ASYNC") == 0)
					{
						PortType[activePortnum] = H_KISS;
					}

					if (stricmp(type, "LOOPBACK") == 0 || stricmp(type, "INTERNAL") == 0)
					{
						PortType[activePortnum] = H_LOOP;
					}

					continue;

				}

				if (stricmp(ptr, "IPADDR") == 0 || stricmp(ptr, "COMPORT") == 0 || stricmp(ptr, "IOADDR") == 0)
				{
					char * addr = strtok_s(NULL, " =\t\r\n", &Context);
				
					strcpy(Param1[activePortnum], addr);
					continue;
				}

				if (stricmp(ptr, "SPEED") == 0)
				{
					char * addr = strtok_s(NULL, " =\t\r\n", &Context);

					Param2[activePortnum] = atoi(addr);
					continue;
				}

				if (stricmp(ptr, "TCPPORT") == 0)
				{
					char * addr = strtok_s(NULL, " =\t\r\n", &Context);

					Param2[activePortnum] = atoi(addr);

					if (PortType[activePortnum] == H_KISS)
						PortType[activePortnum] = H_KISSTCP;

					continue;
				}

				if (stricmp(ptr, "UDPPORT") == 0)
				{
					char * addr = strtok_s(NULL, " =\t\r\n", &Context);

					Param2[activePortnum] = atoi(addr);

					if (PortType[activePortnum] == H_KISS)
						PortType[activePortnum] = H_KISSUDP;

						continue;
				}

				if (stricmp(ptr, "USER") == 0)
				{
					// Telnet USER line - Host then Port

					if (gotUser == FALSE)	// only on first USER line
					{
						char * user = strtok_s(NULL, " ,=\t\r\n", &Context);
						char * pass = strtok_s(NULL, " ,=\t\r\n", &Context);

						ui.sysopUser->setText(user);
						ui.sysopPass->setText(pass);

						gotUser = TRUE;
					}
					continue;
				}

				if (stricmp(ptr, "ADDR") == 0)
				{
					// ADDR line - Host then Port
					
					char * addr = strtok_s(NULL, " =\t\r\n", &Context);
					strcpy(Param1[activePortnum], addr);

					addr = strtok_s(NULL, " =\t\r\n", &Context);
					Param2[activePortnum] = atoi(addr);

					continue;
				}


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

				if (stricmp(ptr, "DRIVER") == 0 || stricmp(ptr, "DLLNAME") == 0)
				{
					ptr = strtok_s(NULL, ".\t\r\n", &Context);

					if (ptr == NULL)
						continue;

					if (stricmp(ptr, "ARDOP") == 0)
					{
						HadARDOP = true;
						PortType[activePortnum] = H_ARDOP;

						continue;
					}

					if (stricmp(ptr, "BPQAXIP") == 0)
					{
						PortType[activePortnum] = H_AXIP;
						continue;
					}

					if (stricmp(ptr, "BPQETHER") == 0)
					{
						PortType[activePortnum] = H_ETHER;
						continue;
					}

					if (stricmp(ptr, "BPQVKISS") == 0)
					{
						PortType[activePortnum] = H_VKISS;
						continue;
					}

					if (stricmp(ptr, "FLDIGI") == 0)
					{
						PortType[activePortnum] = H_FLDIGI;
						continue;
					}

					if (stricmp(ptr, "KAMPACTOR") == 0)
					{
						PortType[activePortnum] = H_KAM;
						continue;
					}

					if (stricmp(ptr, "SCSPACTOR") == 0)
					{
						PortType[activePortnum] = H_SCS;
						continue;
					}

					if (stricmp(ptr, "SERIAL") == 0)
					{
						PortType[activePortnum] = H_SERIAL;
						continue;
					}

					if (stricmp(ptr, "TELNET") == 0)
					{
						PortType[activePortnum] = H_TELNET;
						continue;
					}
					if (stricmp(ptr, "UZ7HO") == 0)
					{
						HadUZ7HO = true;
						PortType[activePortnum] = H_UZ7HO;
						continue;
					}

					if (stricmp(ptr, "WINMOR") == 0)
					{
						HadWINMOR = true;
						PortType[activePortnum] = H_WINMOR;
						continue;
					}

					if (stricmp(ptr, "VARA") == 0)
					{
						HadVARA = true;
						PortType[activePortnum] = H_VARA;
						continue;
					}
				}
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
				LOC = strtok_s(NULL, "\t\r\n", &Context);
				ui.LOC->setText(LOC);
				continue;
			}

			if (stricmp(ptr, "PORT") == 0)
			{
				inPort = TRUE;
				activePortnum = nextPortnum;
				continue;
			}

			if (stricmp(ptr, "APPLICATION") == 0)
			{
				char * Num= strtok_s(NULL, " ,\t\r", &Context);
				char * Appl = strtok_s(NULL, " ,\t\r", &Context);
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
					if (strcmp(Appl, "BBS") == 0)
					{
						BBSAppl = atoi(Num);
						ui.BBS->setChecked(true);
						ui.BBSCall->setText(ApplCall);
						ui.BBSAlias->setText(ApplAlias);
						ui.BBSQual->setText(ApplQual);

					}
					else if (strcmp(Appl, "RMS") == 0)
					{
						RMSAppl = atoi(Num);
						ui.RMS->setChecked(true);
						ui.RMSCall->setText(ApplCall);
						ui.RMSAlias->setText(ApplAlias);
						ui.RMSQual->setText(ApplQual);
					}
					else if (_stricmp(Appl, "CHAT") == 0)
					{
						ChatAppl = atoi(Num);
						ui.CHAT->setChecked(true);
						ui.ChatCall->setText(ApplCall);
						ui.ChatAlias->setText(ApplAlias);
						ui.ChatQual->setText(ApplQual);
					}
				}
				continue;
			}

			if (stricmp(line, "SIMPLE") == 0)
				continue;

			if (stricmp(line, "LINMAIL") == 0)
				continue;

			if (stricmp(line, "LINCHAT") == 0)
				continue;

			// Unknown line

			UnrecognisedLine = true;
		}

		fclose(fp);

		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "Save") == 0)
	{
		CreateBasicConfig();
		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "ADDPORT") == 0)
	{
		// This runs the AGW Configuration dialog

		PortDialog dialog(0);
		dialog.exec();

 		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "UZ7HO") == 0)
	{
		// Add a UZ7HO Port

		QMessageBox msgBox;

		QString str = ui.Directory->text();
		QString str3 = ui.LOC->text().toUpper();

		char FN[260];
		FILE *fp;
		char line[512];
		char * ptr;
		char * Context;
		int Port;

		sprintf(FN, "%s/bpq32x.cfg", str.toLocal8Bit().constData());

		fp = fopen(FN, "ab");

		if (fp == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		WriteUZ7HOPort(fp);

		fclose(fp);

		msgBox.setWindowTitle("Save");
		msgBox.setText("UZ7HO port added to bpq32.cfg");
		msgBox.exec();

		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "WINMOR") == 0)
	{
		// Add an WINMOR Port

		QMessageBox msgBox;

		QString str = ui.Directory->text();
		QString str3 = ui.LOC->text().toUpper();

		char FN[260];
		FILE *fp;
		char line[512];
		char * ptr;
		char * Context;
		int Port;

		sprintf(FN, "%s/bpq32x.cfg", str.toLocal8Bit().constData());

		fp = fopen(FN, "ab");

		if (fp == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		WriteWINMORPort(fp);

		fclose(fp);

		msgBox.setWindowTitle("Save");
		msgBox.setText("WINMOR port added to bpq32.cfg");
		msgBox.exec();

		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "VARA") == 0)
	{
		// Add a VARA Port

		QMessageBox msgBox;

		QString str = ui.Directory->text();
		QString str3 = ui.LOC->text().toUpper();

		char FN[260];
		FILE *fp;
		char line[512];
		char * ptr;
		char * Context;
		int Port;

		sprintf(FN, "%s/bpq32x.cfg", str.toLocal8Bit().constData());

		fp = fopen(FN, "ab");

		if (fp == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		WriteVARAPort(fp);

		fclose(fp);

		msgBox.setWindowTitle("Save");
		msgBox.setText("VARA port added to bpq32.cfg");
		msgBox.exec();

		return;
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("MessageBox Title");
	msgBox.setText("You Clicked " + ((QPushButton*)sender())->objectName());
	msgBox.exec();
}

void BPQConfigGenerator::WriteARDOPPort(FILE * fp)
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


void BPQConfigGenerator::WriteUZ7HOPort(FILE * fp)
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

void BPQConfigGenerator::WriteWINMORPort(FILE *fp)
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

void BPQConfigGenerator::WriteVARAPort(FILE * fp)
{
	fprintf(fp, "\r\nPORT\r\n");
	fprintf(fp, " ID=VARA\r\n");
	fprintf(fp, " DRIVER=VARA\r\n");
	fprintf(fp, " INTERLOCK=1\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  ADDR 127.0.0.1 8300\r\n");
	fprintf(fp, "ENDPORT\r\n");
}

void BPQConfigGenerator::AddTCPKISSPort(FILE * fp)
{
	fprintf(fp, "PORT\r\n");

	fprintf(fp, "ID=KISS over TCP Slave\r\n");
	fprintf(fp, "TYPE=ASYNC\r\n");
	fprintf(fp, "IPADDR=0.0.0.0\r\n");
	fprintf(fp, "TCPPORT=8100\r\n");
	fprintf(fp, "ENDPORT\r\n");
	fprintf(fp, "\r\n");
}


void BPQConfigGenerator::portSetVisible(int i, int v)
{
	gPortNum[i]->setVisible(v);
	gPortType[i]->setVisible(v);
	gParam1[i]->setVisible(v);
	gParam2[i]->setVisible(v);
	gPortName[i]->setVisible(v);
}

void BPQConfigGenerator::CreateBasicConfig()
{
	QMessageBox msgBox;

	QString str = ui.Directory->text();
	QString NodeCall = ui.NODECALL->text().toUpper();
	QString NodeAlias = ui.NODEALIAS->text().toUpper();
	QString LOC = ui.LOC->text().toUpper();

	QString User = ui.sysopUser->text();
	QString Pass = ui.sysopPass->text();

	char LoppedCall[80];

	char FN[260];
	FILE *fp;
	char line[512];
	char * ptr;
	char * Context;
	int Port;

	if (NodeCall.size() == 0)
	{
		msgBox.setWindowTitle("Error");
		msgBox.setText("Must Set NODECALL before saving config");
		msgBox.exec();
		return;
	}

	strcpy(LoppedCall, NodeCall.toLocal8Bit().constData());

	ptr = strchr(LoppedCall, '-');
	if (ptr)
		*ptr = 0;

	sprintf(FN, "%s/bpq32x.cfg", str.toLocal8Bit().constData());

	fp = fopen(FN, "wb");

	if (fp == NULL)
	{
		msgBox.setWindowTitle("Error");
		msgBox.setText("Failed to open bpq32.cfg");
		msgBox.exec();
		return;
	}

	fprintf(fp, "; Written by BPQConfigGenerator\r\n");
	fprintf(fp, "\r\n");
	fprintf(fp, "SIMPLE\r\n");
	fprintf(fp, "NODECALL=%s\r\n", NodeCall.toLocal8Bit().constData());

	if (NodeAlias.size())
		fprintf(fp, "NODEALIAS=%s\r\n", NodeAlias.toLocal8Bit().constData());

	if (LOC.size())
		fprintf(fp, "LOCATOR=%s\r\n", LOC.toLocal8Bit().constData());

	fprintf(fp, "\r\n");

	// Write PORT Lines

	QString CMSCall = ui.CMSCALL->text().toUpper();;
	QString CMSPass = ui.CMSPASS->text();

	fprintf(fp, "PORT\r\n");

	//	CMS = 1
	//	CMSPASS = BPQSTAR
	//	RELAYAPPL = PMS

	fprintf(fp, " ID=Telnet Server\r\n");
	fprintf(fp, " DRIVER=TELNET\r\n");
	fprintf(fp, " CONFIG\r\n");
	fprintf(fp, "  LOGGING=1\r\n");
	fprintf(fp, "  TCPPORT=8010\r\n");
	fprintf(fp, "  FBBPORT=8011\r\n");
	fprintf(fp, "  HTTPPORT=8080\r\n");

	if (ui.RMS->isChecked() || CMSCall.size() || CMSPass.size())
	{
		fprintf(fp, "  CMS=1\r\n");
		if (CMSCall.size())
			fprintf(fp, "  CMSCALL=%s\r\n", CMSCall.toLocal8Bit().constData());
		if (CMSPass.size())
			fprintf(fp, "  CMSPASS=%s\r\n", CMSPass.toLocal8Bit().constData());
	}

	fprintf(fp, "  USER=%s,%s,%s,,SYSOP;\r\n", User.toLocal8Bit().constData(), Pass.toLocal8Bit().constData(), LoppedCall);
	fprintf(fp, "ENDPORT\r\n");
	fprintf(fp, "\r\n");


	// Write any application lines

	fprintf(fp, "\r\n");

	//APPLICATION 1,RMS,C 2 CMS,G8BPQ-10,BPQRMS,255

	if (ui.BBS->isChecked())
	{
		QString str1 = ui.BBSCall->text().toUpper();
		QString str2 = ui.BBSAlias->text().toUpper();
		int Qual = ui.BBSQual->text().toInt();

		fprintf(fp, "APPLICATION 1,BBS,,%s,%s,%d\r\n",
			str1.toLocal8Bit().constData(),
			str2.toLocal8Bit().constData(), Qual);
	}

	if (ui.CHAT->isChecked())
	{
		QString str1 = ui.ChatCall->text().toUpper();
		QString str2 = ui.ChatAlias->text().toUpper();
		int Qual = ui.ChatQual->text().toInt();

		fprintf(fp, "APPLICATION 2,CHAT,,%s,%s,%d\r\n",
			str1.toLocal8Bit().constData(),
			str2.toLocal8Bit().constData(), Qual);
	}

	if (ui.RMS->isChecked())
	{
		QString str1 = ui.RMSCall->text().toUpper();
		QString str2 = ui.RMSAlias->text().toUpper();
		int Qual = ui.RMSQual->text().toInt();

		fprintf(fp, "APPLICATION 3,RMS,C 1 CMS,%s,%s,%d\r\n",
			str1.toLocal8Bit().constData(),
			str2.toLocal8Bit().constData(), Qual);
	}

	fprintf(fp, "\r\n");

	//#ifndef Q_OS_WIN32

	if (ui.BBS->isChecked())
		fprintf(fp, "LINMAIL\r\n");

	if (ui.CHAT->isChecked())
		fprintf(fp, "LINCHAT\r\n");

	//#endif

	fclose(fp);

	msgBox.setWindowTitle("Save");

	if (HadConfig)
		msgBox.setText("bpq32.cfg updated");
	else
		msgBox.setText("bpq32.cfg created");

	msgBox.exec();
}








QComboBox * pPortType;
QLineEdit * pParam1;
QLineEdit * pParam2;

void PortDialog::PortTypeChanged(int Selected)
{
	QVariant Q = pPortType->currentText();
}

PortDialog::PortDialog(QWidget *parent) : QDialog(parent)
{
	this->resize(562, 491);

	setWindowTitle(tr("Add Port"));

	pPortType = new QComboBox(this);
	pPortType->setGeometry(QRect(20, 22, 100, 18));

	pParam1 = new QLineEdit(this);
	pParam1->setGeometry(QRect(20, 52, 100, 18));
	pParam2 = new QLineEdit(this);
	pParam2->setGeometry(QRect(20, 82, 100, 18));

	for (int j = 0; j < NUMBEROFTYPES; j++)
		pPortType->addItem(&Types[j][0]);

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttonBox->setGeometry(QRect(400, 400, 100, 18));

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

	//	QString val = Sess->portNo->text();A
	//	QByteArray qb = val.toLatin1();
	//	char * ptr = qb.data();

	PortDialog::accept();

}

void PortDialog::myreject()
{
	PortDialog::reject();
}
