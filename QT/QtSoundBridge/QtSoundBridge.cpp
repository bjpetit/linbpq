/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundBridge

QtSoundBridge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundBridge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundBridge.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

// UZ7HO Soundmodem Port

// Not Working 4psk100 FEC 

#include "QtSoundBridge.h"
#include <qheaderview.h>
//#include <QDebug>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QSettings>
#include <QPainter>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <qevent.h>
#include <QStandardItemModel>
#include <QScrollBar>

#include "UZ7HOStuff.h"


QImage *Constellation;
QImage *Waterfall[4] = { 0,0,0,0 };
QImage *Header[4];
QLabel *DCDLabel[4];
QImage *DCDLed[4];

QImage *RXLevel;

QLabel *WaterfallCopy[2];
QLabel *HeaderCopy[2];

extern workerThread *t;
extern QtSoundBridge * w;

QList<QSerialPortInfo> Ports = QSerialPortInfo::availablePorts();

void saveSettings();
void getSettings();
extern "C" void CloseSound();
extern "C" void GetSoundDevices();
extern "C" char modes_name[modes_count][20];
extern "C" int speed[5];
extern "C" int KISSPort;
extern "C" short rx_freq[5];

extern "C" int CaptureCount;
extern "C" int PlaybackCount;

extern "C" int CaptureIndex;		// Card number
extern "C" int PlayBackIndex;

extern "C" char CaptureNames[16][256];
extern "C" char PlaybackNames[16][256];

extern "C" int SoundMode;
extern "C" int multiCore = 1;

extern "C" int refreshModems;

extern char LevelMsg[64];


extern "C"
{ 
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
	void modulator(UCHAR snd_ch, int buf_size);
	void SampleSink(int LR, short Sample);
	void AGW_Report_Modem_Change(int port);
	char * strlop(char * buf, char delim);
}


int ModemA = 2;
int ModemB = 2;
int FreqA = 1500;
int FreqB = 1500;
int DCD = 50;

int Closing = FALSE;				// Set to stop background thread

QRgb white = qRgb(255, 255, 255);
QRgb black = qRgb(0, 0, 0);

QRgb green = qRgb(0, 255, 0);
QRgb red = qRgb(255, 0, 0);
QRgb yellow = qRgb(255, 255, 0);
QRgb cyan = qRgb(0, 255, 255);

// Indexed colour list from ARDOPC

#define WHITE 0
#define Tomato 1
#define Gold 2
#define Lime 3
#define Yellow 4
#define Orange 5
#define Khaki 6
#define Cyan 7
#define DeepSkyBlue 8
#define RoyalBlue 9
#define Navy 10
#define Black 11
#define Goldenrod 12
#define Fuchsia 13

QRgb vbColours[16] = { qRgb(255, 255, 255), qRgb(255, 99, 71), qRgb(255, 215, 0), qRgb(0, 255, 0),
						qRgb(255, 255, 0), qRgb(255, 165, 0), qRgb(240, 240, 140), qRgb(0, 255, 255),
						qRgb(0, 191, 255), qRgb(65, 105, 225), qRgb(0, 0, 128), qRgb(0, 0, 0),
						qRgb(218, 165, 32), qRgb(255, 0, 255) };

unsigned char  WaterfallLines[2][80][4096] = { 0 };
int NextWaterfallLine[2] = { 0 };

unsigned int LastLevel = 255;
unsigned int LastBusy = 255;

int UDPPort = 0;
int TXPort = 0;

char UDPHost[64] = "";

QSystemTrayIcon * trayIcon = nullptr;

int MintoTray = 1;

extern "C" void WriteDebugLog(char * Mess)
{
	qDebug() << Mess;
}

UCHAR w_state = 0;

bool QtSoundBridge::eventFilter(QObject* obj, QEvent *evt)
{
	UNUSED(obj);

	if (evt->type() == QEvent::Resize)
	{
		return QWidget::event(evt);
	}

	if (evt->type() == QEvent::WindowStateChange)
	{
		if (windowState().testFlag(Qt::WindowMinimized) == true)
			w_state = WIN_MINIMIZED;
		else
			w_state = WIN_MAXIMIZED;
	}
//	if (evt->type() == QGuiApplication::applicationStateChanged) - this is a sigma;
//	{
//		qDebug() << "App State changed =" << evt->type() << endl;
//	}

	return QWidget::event(evt);
}

void QtSoundBridge::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	QRect r = geometry();

	int A, B, C, W;

	A = r.height() - 25;   // No waterfalls


	C = A - 150;			// Bottom of Monitor, Top of connection list
	W = r.width();

	// Calc Positions of Waterfalls

}

QAction * setupMenuLine(QMenu * Menu, char * Label, QObject * parent, int State)
{
	QAction * Act = new QAction(Label, parent);
	Menu->addAction(Act);

	Act->setCheckable(true);
	if (State)
		Act->setChecked(true);

	parent->connect(Act, SIGNAL(triggered()), parent, SLOT(menuChecked()));

	return Act;
}

void QtSoundBridge::menuChecked()
{
	QAction * Act = static_cast<QAction*>(QObject::sender());

	saveSettings();
}

void QtSoundBridge::initWaterfall(int chan, int state)
{
	QSize Size(800, 602);						// Not actually used, but Event constructor needs it
	QResizeEvent *event = new QResizeEvent(Size, Size);
	QApplication::sendEvent(this, event);
}

QtSoundBridge::QtSoundBridge(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);

	QSettings mysettings("QtSoundBridge.ini", QSettings::IniFormat);

	if (MintoTray)
	{
		trayIcon = new QSystemTrayIcon(QIcon(":/QtSoundBridge/soundmodem.ico"), this);
		trayIcon->setToolTip("QtSoundBridge");
		trayIcon->show();

		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(TrayActivated(QSystemTrayIcon::ActivationReason)));
	}


	restoreGeometry(mysettings.value("geometry").toByteArray());
	restoreState(mysettings.value("windowState").toByteArray());
	
	char Title[128];

	sprintf(Title, "QtSoundBridge Version %s", VersionString);

	this->setWindowTitle(Title);

	// Set up Menus

	setupMenu = ui.menuBar->addMenu(tr("Settings"));

	actDevices = new QAction("Setup Devices", this);
	setupMenu->addAction(actDevices);

	connect(actDevices, SIGNAL(triggered()), this, SLOT(clickedSlot()));
	actDevices->setObjectName("actDevices");

	actMintoTray = setupMenu->addAction("Minimize to Tray", this, SLOT(MinimizetoTray()));
	actMintoTray->setCheckable(1);
	actMintoTray->setChecked(MintoTray);

	actAbout = ui.menuBar->addAction("&About");

	connect(actAbout, SIGNAL(triggered()), this, SLOT(doAbout()));

	Header[0] = new QImage(1024, 35, QImage::Format_RGB32);
	Header[1] = new QImage(1024, 35, QImage::Format_RGB32);
	RXLevel = new QImage(150, 10, QImage::Format_RGB32);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(500);
}

void QtSoundBridge::MinimizetoTray()
{
	MintoTray = actMintoTray->isChecked();
	saveSettings();
	QMessageBox::about(this, tr("QtSoundBridge"),
	tr("Program must be restarted to change Minimize mode"));
}

extern "C" int droppedPackets;
extern "C" int missedPackets;


void QtSoundBridge::TrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == 3)
	{
		showNormal();
	} 
}

void QtSoundBridge::MyTimerSlot()
{
	// 100 mS Timer Event

	ui.seqErrors->setText(QString::number(missedPackets));
	ui.Dropped->setText(QString::number(droppedPackets));
	ui.TXLevel->setText(LevelMsg);
}

void QtSoundBridge::clickedSlot()
{
	char Name[32];

	strcpy(Name, sender()->objectName().toUtf8());

	if (strcmp(Name, "actDevices") == 0)
	{
		doDevices();
		return;
	}

}

QDialog * deviceUI;

Ui_devicesDialog * Dev;

char NewPTTPort[80];

int newSoundMode = 0;
int oldSoundMode = 0;

void QtSoundBridge::SoundModeChanged(bool State)
{
	UNUSED(State);

	// Mustn't change SoundMode until dialog is accepted

	if (Dev->PULSE->isChecked())
		newSoundMode = 2;
	else
		newSoundMode = Dev->OSS->isChecked();

}


bool myResize::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Resize)
	{
		QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
		QSize size = resizeEvent->size();
		int h = size.height();
		int w = size.width();

		Dev->scrollArea->setGeometry(QRect(5, 5, w - 10, h - 10));

		return true;
	}
	return QObject::eventFilter(obj, event);
}

void QtSoundBridge::doDevices()
{
	char valChar[10];

	Dev = new(Ui_devicesDialog);

	QDialog UI;

	int i;

	Dev->setupUi(&UI);

	deviceUI = &UI;

	myResize *resize = new myResize();

	UI.installEventFilter(resize);

	newSoundMode = -1;
	oldSoundMode = SoundMode;

#ifdef WIN32
	Dev->ALSA->setText("WaveOut");
	Dev->OSS->setVisible(0);
	Dev->PULSE->setVisible(0);
#endif

	if (SoundMode == 0)
		Dev->ALSA->setChecked(1);
	else if (SoundMode == 1)
		Dev->OSS->setChecked(1);
	else if (SoundMode == 2)
		Dev->PULSE->setChecked(1);

	connect(Dev->ALSA, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->OSS, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->PULSE, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));

	for (i = 0; i < PlaybackCount; i++)
		Dev->outputDevice->addItem(&PlaybackNames[i][0]);

	i = Dev->outputDevice->findText(PlaybackDevice, Qt::MatchContains);

	Dev->outputDevice->setCurrentIndex(i);

	for (i = 0; i < CaptureCount; i++)
		Dev->inputDevice->addItem(&CaptureNames[i][0]);

	i = Dev->inputDevice->findText(CaptureDevice, Qt::MatchContains);
	Dev->inputDevice->setCurrentIndex(i);

	if (UDPPort != TXPort)
		sprintf(valChar, "%d/%d", UDPPort, TXPort);
	else
		sprintf(valChar, "%d", UDPPort);

	Dev->UDPTXPort->setText(valChar);

	Dev->UDPTXHost->setText(UDPHost);

	QStringList items;

	QObject::connect(Dev->okButton, SIGNAL(clicked()), this, SLOT(deviceaccept()));
	QObject::connect(Dev->cancelButton, SIGNAL(clicked()), this, SLOT(devicereject()));

	UI.exec();

}

void QtSoundBridge::deviceaccept()
{
	QVariant Q = Dev->inputDevice->currentText();
	int cardChanged = 0;
	char portString[32];

	if (Dev->PULSE->isChecked())
		SoundMode = 2;
	else
		SoundMode = Dev->OSS->isChecked();

	if (oldSoundMode != SoundMode)
	{
		//		QMessageBox::about(this, tr("Info"),
		//			tr("Program must restart to change Sound Mode"));

		QMessageBox msgBox;

		msgBox.setText("QtSoundBridge must restart to change Sound Mode.\n"
			"Program will close if you hit Ok\n"
			"You will need to reselect audio devices after restarting");

		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

		int i = msgBox.exec();

		if (i == QMessageBox::Ok)
		{
			SoundMode = newSoundMode;

			saveSettings();

			Closing = 1;
			return;
		}

		if (oldSoundMode == 0)
			Dev->ALSA->setChecked(1);
		else if (oldSoundMode == 1)
			Dev->OSS->setChecked(1);
		else
			Dev->PULSE->setChecked(1);

		QMessageBox::about(this, tr("Info"),
			tr("<p align = 'center'>Changes not saved</p>"));

		return;

	}

	if (strcmp(CaptureDevice, Q.toString().toUtf8()) != 0)
	{
		strcpy(CaptureDevice, Q.toString().toUtf8());
		cardChanged = 1;
	}

	CaptureIndex = Dev->inputDevice->currentIndex();

	Q = Dev->outputDevice->currentText();

	if (strcmp(PlaybackDevice, Q.toString().toUtf8()) != 0)
	{
		strcpy(PlaybackDevice, Q.toString().toUtf8());
		cardChanged = 1;
	}

	PlayBackIndex = Dev->outputDevice->currentIndex();

	Q = Dev->UDPTXPort->text();
	strcpy(portString, Q.toString().toUtf8());

	UDPPort = atoi(portString);

	if (strchr(portString, '/'))
	{
		char * ptr = strlop(portString, '/');
		TXPort = atoi(ptr);
	}
	else
		TXPort = UDPPort;

	Q = Dev->UDPTXHost->text();
	strcpy(UDPHost, Q.toString().toUtf8());


	delete(Dev);
	saveSettings();
	deviceUI->accept();

	if (cardChanged)
	{
		InitSound(1);
	}

	QSize newSize(this->size());
	QSize oldSize(this->size());

	QResizeEvent *myResizeEvent = new QResizeEvent(newSize, oldSize);

	QCoreApplication::postEvent(this, myResizeEvent);  
}

void QtSoundBridge::devicereject()
{	
	delete(Dev);
	deviceUI->reject();
}

void QtSoundBridge::handleButton(int Port, int Type)
{
}

void QtSoundBridge::doAbout()
{
	QMessageBox::about(this, tr("About"),
		tr("G8BPQ's port of UZ7HO's Soundmodem\n\nCopyright (C) 2019-2020 Andrei Kopanchuk UZ7HO"));
}



/*
#ifdef WIN32

#define pthread_t uintptr_t

extern "C" uintptr_t _beginthread(void(__cdecl *start_address)(void *), unsigned stack_size, void *arglist);

#else

#include <pthread.h>

extern "C" pthread_t _beginthread(void(*start_address)(void *), unsigned stack_size, void * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*)arglist) != 0)
		perror("New Thread");
	else
		pthread_detach(thread);

	return thread;
}

#endif
*/
extern "C" void doWaterfall(int snd_ch)
{
	if (Closing)
		return;

}


void QtSoundBridge::changeEvent(QEvent* e)
{
	if (e->type() == QEvent::WindowStateChange)
	{
		QWindowStateChangeEvent* ev = static_cast<QWindowStateChangeEvent*>(e);

		qDebug() << windowState();

		if (!(ev->oldState() & Qt::WindowMinimized) && windowState() & Qt::WindowMinimized)
		{
			if (trayIcon)
				setVisible(false);
		}
//		if (!(ev->oldState() != Qt::WindowNoState) && windowState() == Qt::WindowNoState)
//		{
//			QMessageBox::information(this, "", "Window has been restored");
//		}

	}
	QWidget::changeEvent(e);
}

#include <QCloseEvent>

void QtSoundBridge::closeEvent(QCloseEvent *event)
{
	UNUSED(event);

	QSettings mysettings("QtSoundBridge.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", QWidget::saveGeometry());
	mysettings.setValue("windowState", saveState());

	Closing = TRUE;
	qDebug() << "Closing";

	QThread::msleep(100);
}

	
QtSoundBridge::~QtSoundBridge()
{
	qDebug() << "Saving Settings";
		
	QSettings mysettings("QtSoundBridge.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", saveGeometry());
	mysettings.setValue("windowState", saveState());
	
	saveSettings();	
	Closing = TRUE;
	qDebug() << "Closing";

	QThread::msleep(100);
}

extern "C" void QSleep(int ms)
{
	QThread::msleep(ms);
}

int upd_time = 30;

// "Copy on Select" Code

void QtSoundBridge::onTEselectionChanged()
{
}

