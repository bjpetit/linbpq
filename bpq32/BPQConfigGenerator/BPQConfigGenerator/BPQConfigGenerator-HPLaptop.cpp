#include "BPQConfigGenerator.h"
#ifdef Q_OS_WIN32
#include <Windows.h>
#else

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
bool HadARDOP = false;
bool HadWINMOR = false;
bool HadUZ7HO = false;
bool HadVARA = false;

int BBSAppl = 0;
int ChatAppl = 0;
int RMSAppl = 0;

char ValfromReg[MAX_PATH] = "";

BPQConfigGenerator::BPQConfigGenerator(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// Get defualt BPQ Directory

#ifdef Q_OS_WIN32

	HKEY REGTREE = HKEY_CURRENT_USER;

	HKEY hKey = 0;
	HKEY hKeyIn = 0;
	HKEY hKeyOut = 0;
	int disp;
	int retCode, i;
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

	QObject::connect(ui.UpdateConfig, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.Save, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.ARDOP, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.UZ7HO, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.WINMOR, SIGNAL(clicked()), this, SLOT(clickedSlot()));
	QObject::connect(ui.VARA, SIGNAL(clicked()), this, SLOT(clickedSlot()));
//	QObject::connect(ui., SIGNAL(clicked()), this, SLOT(clickedSlot()));
}

void BPQConfigGenerator::clickedSlot()
{
	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "UpdateConfig") == 0)
	{
		char FN[260];
		FILE *fp;
		char line[512];
		char * ptr;
		char * Context;
		int Port;
		bool Comment = false;
		bool inPort = false;		// parsing a PORT section

		char * NODECALL = NULL;
		char * NODEALIAS = NULL;
		char * LOC = NULL;

		sprintf(FN, "%s/bpq32x.cfg", ui.Directory->text().toLocal8Bit().constData());

		if ((fp = fopen(FN, "r")) == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		HadConfig = true;		// we've read a config 

		ui.ARDOP->setEnabled(true);
		ui.UZ7HO->setEnabled(true);
		ui.WINMOR->setEnabled(true);
		ui.VARA->setEnabled(true);

		while (fgets(line, 512, fp) != NULL)
		{
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

			if (inPort)
			{
				if (stricmp(ptr, "ENDPORT") == 0)
				{
					inPort = FALSE;
					continue;
				}

				if (stricmp(ptr, "DRIVER") == 0)
				{
					ptr = strtok_s(NULL, "\t\r\n", &Context);

					if (ptr == NULL)
						continue;

					if (stricmp(ptr, "ARDOP") == 0)
					{
						HadARDOP = true;
						ui.ARDOP->setDisabled(true);
						continue;
					}

					if (stricmp(ptr, "UZ7HO") == 0)
					{
						HadUZ7HO = true;
						ui.UZ7HO->setDisabled(true);
						continue;
					}

					if (stricmp(ptr, "WINMOR") == 0)
					{
						HadWINMOR = true;
						ui.WINMOR->setDisabled(true);
						continue;
					}

					if (stricmp(ptr, "VARA") == 0)
					{
						HadVARA = true;
						ui.VARA->setDisabled(true);
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
		}

		fclose(fp);

		return;
	}

	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "Save") == 0)
	{
		QMessageBox msgBox;

		QString str = ui.Directory->text();
		QString str1 = ui.NODECALL->text().toUpper();
		QString str2 = ui.NODEALIAS->text().toUpper();
		QString str3 = ui.LOC->text().toUpper();

		char LoppedCall[80];

		char FN[260];
		FILE *fp;
		char line[512];
		char * ptr;
		char * Context;
		int Port;

		if (str1.size() == 0)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Must Set NODECALL before saving config");
			msgBox.exec();
			return;
		}

		if (HadARDOP)
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, "Your config contains an ARDOP Port. If you continue it will be removed", "Quit?",
				QMessageBox::Yes | QMessageBox::No);
			if (reply == QMessageBox::Yes)
			{
				return;
			}
			else
			{
			}
		}
		
			

		strcpy(LoppedCall, str1.toLocal8Bit().constData());

		ptr = strchr(LoppedCall, '-');
		if (ptr)
			*ptr = 0;

		sprintf(FN, "%s/bpq32x.cfg", str.toLocal8Bit().constData());

		fp = fopen(FN, "wb");

		if (fp == NULL)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle("Error");
			msgBox.setText("Failed to open bpq32.cfg");
			msgBox.exec();
			return;
		}

		fprintf(fp, "; Written by BPQConfigGenerator\r\n");
		fprintf(fp, "\r\n");
		fprintf(fp, "SIMPLE\r\n");
		fprintf(fp, "NODECALL=%s\r\n", str1.toLocal8Bit().constData());

		if (str2.size())
			fprintf(fp, "NODEALIAS=%s\r\n", str2.toLocal8Bit().constData());

		if (str3.size())
			fprintf(fp, "LOCATOR=%s\r\n", str3.toLocal8Bit().constData());

		fprintf(fp, "\r\n");

		// Write PORT Lines

		if (ui.Telnet->isChecked())
		{
			QString CMSCall = ui.CMSCALL->text();
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

			fprintf(fp, "  USER=%s,password,%s,,SYSOP;\r\n", LoppedCall, LoppedCall);
			fprintf(fp, "ENDPORT\r\n");
			fprintf(fp, "\r\n");
		}

		if (ui.ARDOP->isChecked())
		{
			fprintf(fp, "PORT\r\n");
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
			fprintf(fp, "\r\n");
		}

		if (ui.UZ7HO->isChecked())
		{
			fprintf(fp, "PORT\r\n");
			fprintf(fp, " ID=UZ7HO\r\n");
			fprintf(fp, " DRIVER=UZ7HO\r\n");
			fprintf(fp, " CHANNEL=A\r\n");
			fprintf(fp, " INTERLOCK=1\r\n");
			fprintf(fp, " CONFIG\r\n");
			fprintf(fp, "  ADDR 127.0.0.1 8000\r\n");
			fprintf(fp, "  UPDATEMAP\r\n");
			fprintf(fp, "ENDPORT\r\n");
			fprintf(fp, "\r\n");
		}

		if (ui.WINMOR->isChecked())
		{
			fprintf(fp, "PORT\r\n");
			fprintf(fp, " ID=WINMOR\r\n");
			fprintf(fp, " DRIVER=WINMOR\r\n");
			fprintf(fp, " INTERLOCK=1\r\n");
			fprintf(fp, " CONFIG\r\n");
			fprintf(fp, "  ADDR 127.0.0.1 8500\r\n");
			fprintf(fp, "  DEBUGLOG TRUE\r\n");
			fprintf(fp, "  BUSYLOCK TRUE\r\n");
			fprintf(fp, "  BW 1600\r\n");
			fprintf(fp, "ENDPORT\r\n");
			fprintf(fp, "\r\n");
		}

		if (ui.VARA->isChecked())
		{
			fprintf(fp, "PORT\r\n");
			fprintf(fp, " ID=VARA\r\n");
			fprintf(fp, " DRIVER=VARA\r\n");
			fprintf(fp, " INTERLOCK=1\r\n");
			fprintf(fp, " CONFIG\r\n");
			fprintf(fp, "  ADDR 127.0.0.1 8300\r\n");
			fprintf(fp, "ENDPORT\r\n");
			fprintf(fp, "\r\n");
		}

		// Write any application lines
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

		fclose(fp);

		msgBox.setWindowTitle("Save");

		if (HadConfig)
			msgBox.setText("bpq32.cfg updated");
		else
			msgBox.setText("bpq32.cfg created");

		msgBox.exec();
		return;
	}
	if (strcmp(((QPushButton*)sender())->objectName().toUtf8(), "ARDOP") == 0)
	{
		// Add an ARDOP Port

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
	
		fclose(fp);

		msgBox.setWindowTitle("Save");

		if (HadConfig)
			msgBox.setText("ARDOP port added to bpq32.cfg");
		
		msgBox.exec();
		return;
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("MessageBox Title");
	msgBox.setText("You Clicked " + ((QPushButton*)sender())->objectName());
	msgBox.exec();
}

