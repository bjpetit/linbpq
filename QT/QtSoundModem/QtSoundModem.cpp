

#include "QtSoundModem.h"
#include <qheaderview.h>
#include <QDebug>
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
QImage *Waterfall[4];
QImage *Header[4];
QLabel *DCDLabel[4];
QImage *DCDLed[4];

QImage *RXLevel;

QLabel *WaterfallCopy[2];
QLabel *HeaderCopy[2];

QTextEdit * monWindowCopy;

extern workerThread *t;

QList<QSerialPortInfo> Ports = QSerialPortInfo::availablePorts();

void saveSettings();
void getSettings();
extern "C" void CloseSound();

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


extern "C"
{ 
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
	void modulator(UCHAR snd_ch, int buf_size);
	void SampleSink(int LR, short Sample);
	void doCalib(int Port, int Act);
	int Freq_Change(int Chan, int Freq);
	void set_speed(int snd_ch, int Modem);
	void init_speed(int snd_ch);
	void wf_pointer(int snd_ch);
	void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);
	void dofft(short * in, float * outr, float * outi);
	void init_raduga();
	void wf_Scale(int Chan);
}

void make_graph_buf(float * buf, short tap, QPainter * bitmap);

int ModemA = 2;
int ModemB = 2;
int FreqA = 1500;
int FreqB = 1500;
int DCD = 50;

bool Closing = FALSE;				// Set to stop background thread

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

extern "C" void WriteDebugLog(int LogLevel, const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);
	char timebuf[128];
	UCHAR Value[100];

	va_start(arglist, format);
	vsnprintf(Mess, sizeof(Mess), format, arglist);
	
	qDebug() << Mess;

	return;
}

void QtSoundModem::doupdateDCD(int Chan, int State)
{
	DCDLabel[Chan]->setVisible(State);
}

extern "C" char * frame_monitor(string * frame, char * code, bool tx_stat);
extern "C" char * ShortDateTime();

extern "C" void put_frame(int snd_ch, string * frame, char * code, int  tx, int excluded)
{
	int Len;
	char * Msg = (char *)malloc(1024);		// Cant pass local variable via signal/slot

	string * x = newString();

	if (strcmp(code, "NON-AX25") == 0)
		sprintf(Msg, "%d: <NON-AX25 frame Len = %d [%s%c]\r", snd_ch, frame->Length, ShortDateTime(), 'R');
	else
		sprintf(Msg, "%d:%s", snd_ch + 1, frame_monitor(frame, code, tx));

	Len = strlen(Msg);

	if (Msg[Len - 1] != '\r')
	{
		Msg[Len++] = '\r';
		Msg[Len] = 0;
	}

	emit t->sendtoTrace(Msg, tx);
}

extern "C" void updateDCD(int Chan, bool State)
{
	emit t->updateDCD(Chan, State);
}

bool QtSoundModem::eventFilter(QObject* obj, QEvent *evt)
{
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

void QtSoundModem::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	QRect r = geometry();

	int A, B, C, W;

	if (soundChannel[1])
	{
		ui.modeB->setVisible(1);
		ui.centerB->setVisible(1);
		ui.labelB->setVisible(1);
	}
	else
	{
		ui.modeB->setVisible(0);
		ui.centerB->setVisible(0);
		ui.labelB->setVisible(0);
	}

	if (UsingBothChannels)
	{
		// Two waterfalls

		ui.WaterfallA->setVisible(1);
		ui.HeaderA->setVisible(1);
		ui.WaterfallB->setVisible(1);
		ui.HeaderB->setVisible(1);

		A = r.height() - 258;   // Top of Waterfall A
		B = A + 115;			// Top of Waterfall B
	}
	else
	{
		// One waterfall

		// Could be Left or Right

		if (soundChannel[0] == RIGHT)
		{
			ui.WaterfallA->setVisible(0);
			ui.HeaderA->setVisible(0);
			ui.WaterfallB->setVisible(1);
			ui.HeaderB->setVisible(1);
		}
		else
		{
			ui.WaterfallA->setVisible(1);
			ui.HeaderA->setVisible(1);
			ui.WaterfallB->setVisible(0);
			ui.HeaderB->setVisible(0);
		}

		A = r.height() - 145;   // Top of Waterfall A
	}

	C = A - 150;			// Bottom of Monitor, Top of connection list
	W = r.width();

	// Calc Positions of Waterfalls

	ui.monWindow->setGeometry(QRect(0, 30, W, C - 56));
	sessionTable->setGeometry(QRect(0, C, W, 175));

	if (UsingBothChannels)
	{
		ui.HeaderA->setGeometry(QRect(0, A, W, 35));
		ui.WaterfallA->setGeometry(QRect(0, A + 35, W, 80));
		ui.HeaderB->setGeometry(QRect(0, B, W, 35));
		ui.WaterfallB->setGeometry(QRect(0, B + 35, W, 80));
	}
	else
	{
		if (soundChannel[0] == RIGHT)
		{
			ui.HeaderB->setGeometry(QRect(0, A, W, 35));
			ui.WaterfallB->setGeometry(QRect(0, A + 35, W, 80));
		}
		else
		{
			ui.HeaderA->setGeometry(QRect(0, A, W, 35));
			ui.WaterfallA->setGeometry(QRect(0, A + 35, W, 80));
		}
	}
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

void QtSoundModem::menuChecked()
{
	QAction * Act = static_cast<QAction*>(QObject::sender());

	int state = Act->isChecked();

	if (Act == actWaterfall1)
		Firstwaterfall = state;
	else if (Act == actWaterfall2)
		Secondwaterfall = state;
}

QtSoundModem::QtSoundModem(QWidget *parent) : QMainWindow(parent)
{
	int i;

	ui.setupUi(this);

	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);

	restoreGeometry(mysettings.value("geometry").toByteArray());
	restoreState(mysettings.value("windowState").toByteArray());

	sessionTable = new QTableWidget(this);

	sessionTable->verticalHeader()->setVisible(FALSE);
	sessionTable->verticalHeader()->setDefaultSectionSize(20);
	sessionTable->horizontalHeader()->setDefaultSectionSize(68);
	sessionTable->setRowCount(1);
	sessionTable->setColumnCount(11);
	m_TableHeader << "MyCall" << "DestCall" << "Status" << "Sent pkts" << "Sent Bytes" << "Rcvd pkts" << "Rcvd bytes" << "Rcvd FC" << "CPS TX" << "CPS RX" << "Direction";

	sessionTable->setStyleSheet("QHeaderView::section { background-color:rgb(224, 224, 224) }");

	sessionTable->setHorizontalHeaderLabels(m_TableHeader);
	sessionTable->setColumnWidth(0, 80);
	sessionTable->setColumnWidth(1, 80);
	sessionTable->setColumnWidth(4, 76);
	sessionTable->setColumnWidth(5, 76);
	sessionTable->setColumnWidth(6, 80);
	sessionTable->setColumnWidth(10, 72);

	for (int i = 0; i < modes_count; i++)
	{
		ui.modeA->addItem(modes_name[i]);
		ui.modeB->addItem(modes_name[i]);
	}

	char Title[128];

	sprintf(Title, "QtSoundModem Version %s", VersionString);

	this->setWindowTitle(Title);

	// Set up Menus

	setupMenu = ui.menuBar->addMenu(tr("Settings"));

	actDevices = new QAction("Setup Devices", this);
	setupMenu->addAction(actDevices);

	connect(actDevices, &QAction::triggered, this, [=] {doDevices(); });

	actModems = new QAction("Setup Modems", this);
	setupMenu->addAction(actModems);

	connect(actModems, &QAction::triggered, this, [=] {doModems(); });

	viewMenu = ui.menuBar->addMenu(tr("&View"));

	actWaterfall1 = setupMenuLine(viewMenu, (char *)"First waterfall", this, Firstwaterfall);
	actWaterfall2 = setupMenuLine(viewMenu, (char *)"Second Waterfall", this, Secondwaterfall);

	actCalib = ui.menuBar->addAction("&Calibration");

	connect(actCalib, SIGNAL(triggered()), this, SLOT(doCalibrate()));

	//	Constellation = new QImage(91, 91, QImage::Format_RGB32);
	Waterfall[0] = new QImage(1024, 80, QImage::Format_RGB32);
	Header[0] = new QImage(1024, 35, QImage::Format_RGB32);
	Waterfall[1] = new QImage(1024, 80, QImage::Format_RGB32);
	Header[1] = new QImage(1024, 35, QImage::Format_RGB32);
	RXLevel = new QImage(150, 10, QImage::Format_RGB32);

	DCDLabel[0] = new QLabel(this);
	DCDLabel[0]->setObjectName(QString::fromUtf8("DCDLedA"));
	DCDLabel[0]->setGeometry(QRect(234, 31, 12, 12));
	DCDLabel[0]->setVisible(FALSE);

	DCDLabel[1] = new QLabel(this);
	DCDLabel[1]->setObjectName(QString::fromUtf8("DCDLedB"));
	DCDLabel[1]->setGeometry(QRect(480, 31, 12, 12));
	DCDLabel[1]->setVisible(FALSE);

	DCDLed[0] = new QImage(12, 12, QImage::Format_RGB32);
	DCDLed[1] = new QImage(12, 12, QImage::Format_RGB32);

	DCDLed[0]->fill(red);
	DCDLed[1]->fill(red);

	DCDLabel[0]->setPixmap(QPixmap::fromImage(*DCDLed[0]));
	DCDLabel[1]->setPixmap(QPixmap::fromImage(*DCDLed[0]));

	//	Waterfall[0]->setColorCount(16);
	//	Waterfall[1]->setColorCount(16);


	//	for (i = 0; i < 16; i++)
	//	{
	//	Waterfall[0]->setColor(i, vbColours[i]);
	//		Waterfall[1]->setColor(i, vbColours[i]);
	//	}

	Header[0]->fill(black);
	Waterfall[1]->fill(black);
	Header[1]->fill(black);

	WaterfallCopy[0] = ui.WaterfallA;
	WaterfallCopy[1] = ui.WaterfallB;
	HeaderCopy[0] = ui.HeaderA;
	HeaderCopy[1] = ui.HeaderB;
	monWindowCopy = ui.monWindow;

	ui.WaterfallA->setPixmap(QPixmap::fromImage(*Waterfall[0]));
	ui.WaterfallB->setPixmap(QPixmap::fromImage(*Waterfall[1]));

	ui.HeaderA->setPixmap(QPixmap::fromImage(*Header[0]));
	ui.HeaderB->setPixmap(QPixmap::fromImage(*Header[1]));

	wf_pointer(soundChannel[0]);
	wf_pointer(soundChannel[1]);
	wf_Scale(0);
	wf_Scale(1);

	//	RefreshLevel(0);
	//	RXLevel->setPixmap(QPixmap::fromImage(*RXLevel));

	// Inline handlers for Modem Params as they are pretty small

	connect(ui.modeA, QOverload<int>::of(&QComboBox::currentIndexChanged),
		[=](int index)
	{
		ModemA = ui.modeA->currentIndex();
		set_speed(0, ModemA);
		saveSettings();
	});

	connect(ui.modeB, QOverload<int>::of(&QComboBox::currentIndexChanged),

		[=](int index)
	{
		ModemB = ui.modeB->currentIndex();
		set_speed(1, ModemB);
		saveSettings();
	});

	ui.modeA->setCurrentIndex(speed[0]);
	ui.modeB->setCurrentIndex(speed[1]);

	ModemA = ui.modeA->currentIndex();

	ui.centerA->setValue(rx_freq[0]);
	ui.centerB->setValue(rx_freq[1]);

	connect(ui.centerA, QOverload<int>::of(&QSpinBox::valueChanged),

		[=](int i)
	{
		if (i > 300)
		{
			ui.centerA->setValue(Freq_Change(0, i));
		}
	});

	connect(ui.centerB, QOverload<int>::of(&QSpinBox::valueChanged),

		[=](int i)
	{
		if (i > 300)
		{
			Freq_Change(1, i);
		}
	});

	ui.DCDSlider->setValue(dcd_threshold);

	connect(ui.DCDSlider, QOverload<int>::of(&QSlider::sliderMoved),

		[=](int i)
	{
		dcd_threshold = i;
	});

//	installEventFilter(this);

	QObject::connect(t, SIGNAL(sendtoTrace(char *, int)), this, SLOT(sendtoTrace(char *, int)), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(updateDCD(int, int)), this, SLOT(doupdateDCD(int, int)), Qt::QueuedConnection);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);

}

void QtSoundModem::MyTimerSlot()
{
	// 100 mS Timer Event

	show_grid();
}

Ui_ModemDialog * Dlg;

void QtSoundModem::doModems()
{
	Dlg = new(Ui_ModemDialog);

	QDialog UI;
	char valChar[10];

	Dlg->setupUi(&UI);

	Dlg->preEmphAllA->setChecked(emph_all[0]);

	if (emph_all[0])
		Dlg->preEmphA->setDisabled(TRUE);
	else
		Dlg->preEmphA->setCurrentIndex(emph_db[0]);

	Dlg->preEmphAllB->setChecked(emph_all[1]);

	if (emph_all[1])
		Dlg->preEmphB->setDisabled(TRUE);
	else
		Dlg->preEmphB->setCurrentIndex(emph_db[1]);

	sprintf(valChar, "%d", txdelay[0]);
	Dlg->TXDelayA->setText(valChar);

	sprintf(valChar, "%d", txdelay[1]);
	Dlg->TXDelayB->setText(valChar);

	sprintf(valChar, "%d", txtail[0]);
	Dlg->TXTailA->setText(valChar);

	sprintf(valChar, "%d", txtail[1]);
	Dlg->TXTailB->setText(valChar);

	sprintf(valChar, "%d", RCVR[0]);
	Dlg->AddRXA->setText(valChar);

	sprintf(valChar, "%d", RCVR[01]);
	Dlg->AddRXB->setText(valChar);

	sprintf(valChar, "%d", rcvr_offset[0]);
	Dlg->RXShiftA->setText(valChar);

	sprintf(valChar, "%d", rcvr_offset[1]);
	Dlg->RXShiftB->setText(valChar);

	//	speed[1]
	//	speed[2];

	Dlg->leftA->setChecked(!soundChannel[0]);
	Dlg->leftB->setChecked(!soundChannel[1]);

	Dlg->rightA->setChecked(soundChannel[0]);
	Dlg->rightB->setChecked(soundChannel[1]);

	connect(Dlg->showBPF_A, &QPushButton::released, this, [=] { doFilter(0, 0); });
	connect(Dlg->showTXBPF_A, &QPushButton::released, this, [=] { doFilter(0, 1); });
	connect(Dlg->showLPF_A, &QPushButton::released, this, [=] { doFilter(0, 2); });

	connect(Dlg->showBPF_B, &QPushButton::released, this, [=] { doFilter(1, 0); });
	connect(Dlg->showTXBPF_B, &QPushButton::released, this, [=] { doFilter(1, 1); });
	connect(Dlg->showLPF_B, &QPushButton::released, this, [=] { doFilter(1, 2); });

	connect(Dlg->okButton, SIGNAL(clicked()), this, SLOT(modemaccept()));
	connect(Dlg->cancelButton, SIGNAL(clicked()), this, SLOT(modemreject()));

	connect(Dlg->preEmphAllA, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllAChanged(int)));
	connect(Dlg->preEmphAllB, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllBChanged(int)));

	UI.exec();
}

void QtSoundModem::preEmphAllAChanged(int state)
{
	Dlg->preEmphA->setDisabled(state);
}

void QtSoundModem::preEmphAllBChanged(int state)
{
	Dlg->preEmphB->setDisabled(state);
}

void QtSoundModem::modemaccept()
{
	QVariant Q;
	
	emph_all[0] = Dlg->preEmphAllA->isChecked();
	emph_db[0] = Dlg->preEmphA->currentIndex();

	emph_all[1] = Dlg->preEmphAllB->isChecked();
	emph_db[1] = Dlg->preEmphB->currentIndex();

	if (emph_db[0] < 0 || emph_db[0] > nr_emph)
		emph_db[0] = 0;

	if (emph_db[1] < 0 || emph_db[1] > nr_emph)
		emph_db[1] = 0;

	Q = Dlg->TXDelayA->text();
	txdelay[0] = Q.toInt();

	Q = Dlg->TXDelayB->text();
	txdelay[1] = Q.toInt();
	
	Q = Dlg->TXTailA->text();
	txtail[0] = Q.toInt();

	Q = Dlg->TXTailB->text();
	txtail[1] = Q.toInt();

	Q = Dlg->AddRXA->text();
	RCVR[0] = Q.toInt();

	Q = Dlg->AddRXB->text();
	RCVR[1] = Q.toInt();

	Q = Dlg->RXShiftA->text();
	rcvr_offset[0] = Q.toInt();

	Q = Dlg->RXShiftB->text();
	rcvr_offset[1] = Q.toInt();

	soundChannel[0] = Dlg->rightA->isChecked();
	soundChannel[1] = Dlg->rightB->isChecked();


	delete(Dlg);
	saveSettings();
}

void QtSoundModem::modemreject()
{
	delete(Dlg);
}


void QtSoundModem::doFilter(int Chan, int Filter)
{
	Ui_Dialog Dev;
	QImage * bitmap;

	QDialog UI;

	Dev.setupUi(&UI);

	bitmap = new QImage(642, 312, QImage::Format_RGB32);

	bitmap->fill(qRgb(255, 255, 255));

	QPainter qPainter(bitmap);
	qPainter.setBrush(Qt::NoBrush);
	qPainter.setPen(Qt::black);

	if (Filter == 0)
		make_graph_buf(DET[0, 0]->BPF_core[Chan], BPF_tap[Chan], &qPainter);
	else if (Filter == 1)
		make_graph_buf(tx_BPF_core[Chan], tx_BPF_tap[Chan], &qPainter);
	else
		make_graph_buf(LPF_core[Chan], LPF_tap[Chan], &qPainter);

	bool bEnd = qPainter.end();
	Dev.label->setPixmap(QPixmap::fromImage(*bitmap));

	UI.exec();

}

Ui_devicesDialog * Dev;

void QtSoundModem::doDevices()
{
	Dev = new(Ui_devicesDialog);
	{
		QDialog UI;
	
		int i;
		char portnum[16];

		Dev->setupUi(&UI);

		for (i = 0; i < PlaybackCount; i++)
			Dev->outputDevice->addItem(&PlaybackNames[i][0]);

		i = Dev->outputDevice->findText(PlaybackDevice, Qt::MatchContains);

		Dev->outputDevice->setCurrentIndex(i);

		for (i = 0; i < CaptureCount; i++)
			Dev->inputDevice->addItem(&CaptureNames[i][0]);

		i = Dev->inputDevice->findText(CaptureDevice, Qt::MatchContains);
		Dev->inputDevice->setCurrentIndex(i);

		Dev->Modem_1_Chan->setCurrentIndex(soundChannel[0]);
		Dev->Modem_2_Chan->setCurrentIndex(soundChannel[1]);

		// Disable "None" option in first modem
		
		QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(Dev->Modem_1_Chan->model());
		QStandardItem * item = model->item(0, 0);
		item->setEnabled(false);

		Dev->singleChannelOutput->setChecked(SCO);
		Dev->colourWaterfall->setChecked(raduga);

		sprintf(portnum, "%d", KISSPort);
		QString portname(portnum);
		Dev->KISSPort->setText(portname);
		Dev->KISSEnabled->setChecked(KISSServ);

		sprintf(portnum, "%d", AGWPort);
		QString portname1(portnum);
		Dev->AGWPort->setText(portname1);
		Dev->AGWEnabled->setChecked(AGWServ);

		QStringList items;

		for (const QSerialPortInfo &info : Ports)
		{
			items.append(info.portName());
		}

		items.sort();

		Dev->PTTPort->addItem("None");

		for (const QString &info : items)
		{
			Dev->PTTPort->addItem(info);
		}

		Dev->PTTPort->setCurrentIndex(Dev->PTTPort->findText(PTTPort, Qt::MatchFixedString));

		Dev->txRotation->setChecked(TX_rotate);
		Dev->DualPTT->setChecked(DualPTT);

		QObject::connect(Dev->okButton, SIGNAL(clicked()), this, SLOT(deviceaccept()));
		QObject::connect(Dev->cancelButton, SIGNAL(clicked()), this, SLOT(devicereject()));

		UI.exec();
	}
}

void QtSoundModem::deviceaccept()
{
	QVariant Q = Dev->inputDevice->currentText();
	int cardChanged = 0;

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

	soundChannel[0] = Dev->Modem_1_Chan->currentIndex();
	soundChannel[1]  = Dev->Modem_2_Chan->currentIndex();

	if (soundChannel[1])
		DualChan = 1;
	else
		DualChan = 0;

	if (DualChan && (soundChannel[0] != soundChannel[1]))
	
		// Different so need both sides 

		UsingBothChannels = 1;
	else
		UsingBothChannels = 0;

	if (soundChannel[0] == RIGHT)
		modemtoSoundLR[0] = 1;
	else
		modemtoSoundLR[0] = 0;

	if (soundChannel[1] == RIGHT)
		modemtoSoundLR[1] = 1;
	else
		modemtoSoundLR[1] = 0;

	SCO = Dev->singleChannelOutput->isChecked();
	raduga = Dev->colourWaterfall->isChecked();
	AGWServ = Dev->AGWEnabled->isChecked();
	KISSServ = Dev->KISSEnabled->isChecked();

	Q = Dev->KISSPort->text();
	KISSPort = Q.toInt();

	Q = Dev->AGWPort->text();
	AGWPort = Q.toInt();

	txdelay[1] = Q.toInt();

	Q = Dev->PTTPort->currentText();
	strcpy(PTTPort, Q.toString().toUtf8());

	DualPTT = Dev->DualPTT->isChecked();
	TX_rotate = Dev->txRotation->isChecked();

	ClosePTTPort();
	OpenPTTPort();

	wf_pointer(soundChannel[0]);
	if (DualChan)
		wf_pointer(soundChannel[1]);

	delete(Dev);
	saveSettings();

	if (cardChanged)
	{
		CloseSound();
		InitSound(1);
	}

	QSize newSize(this->size());
	QSize oldSize(this->size());

	QResizeEvent *myResizeEvent = new QResizeEvent(newSize, oldSize);

	QCoreApplication::postEvent(this, myResizeEvent);  
}

void QtSoundModem::devicereject()
{
	delete(Dev);
}

void QtSoundModem::handleButton(int Port, int Type)
{
	doCalib(Port, Type);

}

void QtSoundModem::doCalibrate()
{
	Ui_calDialog Calibrate;
	{
		QDialog UI;
		Calibrate.setupUi(&UI);

		connect(Calibrate.Low_A, &QPushButton::released, this, [=] { handleButton(1, 1); });
		connect(Calibrate.High_A, &QPushButton::released, this, [=] { handleButton(1, 2); });
		connect(Calibrate.Both_A, &QPushButton::released, this, [=] { handleButton(1, 3); });
		connect(Calibrate.Stop_A, &QPushButton::released, this, [=] { handleButton(1, 0); });
		connect(Calibrate.Low_B, &QPushButton::released, this, [=] { handleButton(2, 1); });
		connect(Calibrate.High_B, &QPushButton::released, this, [=] { handleButton(2, 2); });
		connect(Calibrate.Both_B, &QPushButton::released, this, [=] { handleButton(2, 3); });
		connect(Calibrate.Stop_B, &QPushButton::released, this, [=] { handleButton(2, 0); });

//		connect(Calibrate.High_A, SIGNAL(released()), this, SLOT(handleButton(1, 2)));

		UI.exec();
	}
}

void QtSoundModem::RefreshSpectrum(unsigned char * Data)
{
	int i;

	// Last 4 bytes are level busy and Tuning lines

	Waterfall[0]->fill(Black);

	if (Data[206] != LastLevel)
	{
		LastLevel = Data[206];
//		RefreshLevel(LastLevel);
	}

	if (Data[207] != LastBusy)
	{
		LastBusy = Data[207];
//		Busy->setVisible(LastBusy);
	}

	for (i = 0; i < 205; i++)
	{
		int val = Data[0];

		if (val > 63)
			val = 63;

		Waterfall[0]->setPixel(i, val, Yellow);
		if (val < 62)
			Waterfall[0]->setPixel(i, val + 1, Gold);
		Data++;
	}

	ui.WaterfallA->setPixmap(QPixmap::fromImage(*Waterfall[0]));

}

void QtSoundModem::RefreshWaterfall(int snd_ch, unsigned char * Data)
{
	int j;
	unsigned char * Line;
	int len = Waterfall[0]->bytesPerLine();
	int TopLine = NextWaterfallLine[snd_ch];

	// Write line to cyclic buffer then draw starting with the line just written

	// Length is 208 bytes, including Level and Busy flags

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Data, 206);
	if (NextWaterfallLine[snd_ch] > 63)
		NextWaterfallLine[snd_ch] = 0;

	for (j = 63; j > 0; j--)
	{
		Line = Waterfall[0]->scanLine(j);
		memcpy(Line, &WaterfallLines[snd_ch][TopLine++][0], len);
		if (TopLine > 63)
			TopLine = 0;
	}

	ui.WaterfallA->setPixmap(QPixmap::fromImage(*Waterfall[0]));
}


void QtSoundModem::sendtoTrace(char * Msg, int tx)
{

	QScrollBar *scrollbar = monWindowCopy->verticalScrollBar();
	int scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	if (scrollbarAtBottom)
		monWindowCopy->moveCursor(QTextCursor::End);

	if (tx)
		monWindowCopy->setTextColor(qRgb(192, 0, 0));
	else
		monWindowCopy->setTextColor(qRgb(0, 0, 192));

	monWindowCopy->insertPlainText(Msg);

	if (scrollbarAtBottom)
		monWindowCopy->moveCursor(QTextCursor::End);

	free(Msg);
}


// I think this does the waterfall

typedef struct TRGBQ_t
{
	byte b, g, r, re;

} TRGBWQ;

typedef struct tagRECT
{
	int    left;
	int    top;
	int    right;
	int    bottom;
} RECT;

unsigned int RGBWF[256] ;


extern "C" void init_raduga()
{

	byte offset[6] = {0, 51, 102, 153, 204};
	byte i, n;

	for (n = 0; n < 52; n++)
	{
		i = n * 5;

		RGBWF[n + offset[0]] = qRgb(0, 0, i);
		RGBWF[n + offset[1]] = qRgb(0, i, 255);
		RGBWF[n + offset[2]] = qRgb(0, 255, 255 - i);
		RGBWF[n + offset[3]] = qRgb(1, 255, 0);
		RGBWF[n + offset[4]] = qRgb(255, 255 - 1, 0);
	}
}

extern "C" int nonGUIMode;


// This draws the Frequency Scale on Waterfall

extern "C" void wf_Scale(int Chan)
{
	if (nonGUIMode)
		return;

	float k;
	int maxfreq, x, i;
	char Textxx[20];
	QImage * bm = Header[Chan];

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::black);
	qPainter.setPen(Qt::white);

	maxfreq = roundf(RX_Samplerate*0.005);
	k = 100 * fft_size / RX_Samplerate;

	if (Chan == 0)
		sprintf(Textxx, "Left");
	else
		sprintf(Textxx, "Right");

	qPainter.drawText(2, 1,
		100, 20, 0, Textxx);

	for (i = 0; i < maxfreq; i++)
	{
		x = round(k*i);
		if (x < 1025)
		{
			if ((i % 5) == 0)
				qPainter.drawLine(x, 20, x, 13);
			else
				qPainter.drawLine(x, 20, x, 16);

			if ((i % 10) == 0)
			{
				sprintf(Textxx, "%d", i * 100);

				qPainter.drawText(x - 12, 1,
					100, 20, 0, Textxx);
			}
		}
	}
	HeaderCopy[Chan]->setPixmap(QPixmap::fromImage(*bm));

}

// This draws the frequency Markers on the Waterfall


void do_pointer(int waterfall)
{
	if (nonGUIMode)
		return;

	float x;
	int Waterfall = 0;

	int x1, x2, y, k, pos1, pos2, pos3;
	QImage * bm = Header[waterfall];

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::NoBrush);
	qPainter.setPen(Qt::white);

	//	bm->fill(black);

	qPainter.fillRect(0, 26, 1024, 9, Qt::black);

	k = 29;
	x = fft_size / RX_Samplerate;

	// draw all enabled ports on the ports on this soundcard

	// First Modem is always on the first waterfall
	// If second is enabled it is on the first unless different
	//		channel from first

	for (int i = 0; i < 2; i++)
	{
		if (waterfall == 1 && i == 0)
			continue;						// First Modem is always on first waterfa;;

		if (waterfall == 1 && soundChannel[1] == soundChannel[0])
			continue;

		if ((waterfall == 0 && i == 1) &&
			(soundChannel[1] == NONE || soundChannel[1] != soundChannel[0]))
			break;

		pos1 = roundf((rx_freq[i] - 0.5*rx_shift[i])*x) - 5;
		pos2 = roundf((rx_freq[i] + 0.5*rx_shift[i])*x) - 5;
		pos3 = roundf(rx_freq[i] * x);
		x1 = pos1 + 5;
		x2 = pos2 + 5;
		y = k + 5;

		qPainter.drawLine(x1, k, x2, k);
		qPainter.drawLine(x1, k - 3, x1, k + 3);
		qPainter.drawLine(x2, k - 3, x2, k + 3);
		qPainter.drawLine(pos3, k - 3, pos3, k + 3);

	}
	HeaderCopy[waterfall]->setPixmap(QPixmap::fromImage(*bm));
}

void wf_pointer(int snd_ch)
{
	do_pointer(0);
	do_pointer(1);
}

extern "C" void doWaterfall(int snd_ch)
{
	if (nonGUIMode)
		return;

	if (Closing)
		return;

	QImage * bm = Waterfall[snd_ch];
	RECT s, d;
	word  i, wid;
	single  mag;
	UCHAR * p;
	UCHAR Line[4096];

	int lineLen, imax;
	word  hfft_size;
	byte  n;
	float RealOut[4096] = { 0 };
	float ImagOut[4096];
	QRegion exposed;
	QPixmap  *pm = (QPixmap *)WaterfallCopy[snd_ch]->pixmap();
	float max = 0;;

	hfft_size = fft_size / 2;

	// I think an FFT should produce n/2 bins, each of Samp/n Hz
	// Looks like my code only works with n a power of 2

	// So can use 1024 or 4096. 1024 gives 512 bins of 11.71875 and a 512 pixel 
	// display (is this enough?)

	// This does 2048

	dofft(&fft_buf[snd_ch][0], RealOut, ImagOut);

	//	FourierTransform(1024, &fft_buf[snd_ch][0], RealOut, ImagOut, 0);

	for (i = 0; i < hfft_size; i++)
	{
		//mag: = ComplexMag(fft_d[i])*0.00000042;

//		mag = sqrtf(powf(RealOut[i], 2) + powf(ImagOut[i], 2)) * 0.00000042f;

		mag = powf(RealOut[i], 2);
		mag += powf(ImagOut[i], 2);
		mag = sqrtf(mag);
		mag *= 0.00000042f;

		if (mag < 0.00001f)
			mag = 0.00001f;

		if (mag > 1.0f)
			mag = 1.0f;

		mag = 22 * log2f(mag) + 255;

		if (mag < 0)
			mag = 0;

		fft_disp[snd_ch][i] = round(mag);
	}

	/*
		for (i = 0; i < hfft_size; i++)
			fft[i] = (powf(RealOut[i], 2) + powf(ImagOut[i], 2));

		for (i = 0; i < hfft_size; i++)
		{
			if (fft[i] > max)
			{
				max = fft[i];
				imax = i;
			}
		}

		if (max > 0)
		{
			for (i = 0; i < hfft_size; i++)
				fft[i] = fft[i] / max;
		}


		for (i = 0; i < hfft_size; i++)
		{
			mag = fft[i];

			if (mag < 0.00001f)
				mag = 0.00001f;

			if (mag > 1.0f)
				mag = 1.0f;

			mag = 22 * log2f(mag) + 255;

			if (mag < 0)
				mag = 0;

			fft_disp[snd_ch][i] = round(mag);
		}

		*/

	d.left = 0;
	d.top = 1;
	d.right = bm->width();
	d.bottom = bm->height();

	d.left = 0;
	d.top = 0;
	d.right = bm->width();
	d.bottom = bm->height() - 1;

	//	bm[snd_ch].Canvas.CopyRect(d, bm[snd_ch].canvas, s)

	//pm->scroll(0, 1, 0, 0, 1024, 80, &exposed);

	// Each bin is 12000 /2048 = 5.859375
	// I think we plot at 6 Hz per pixel.

	wid = bm->width();
	if (wid > hfft_size)
		wid = hfft_size;

	wid = wid - 1;

	p = Line;
	lineLen = bm->bytesPerLine();

	if (wid > lineLen / 4)
		wid = lineLen / 4;

	if (raduga == DISP_MONO)
	{
		for (i = 0; i < wid; i++)
		{
			n = fft_disp[snd_ch][i];
			*(p++) = n;					// all colours the same
			*(p++) = n;
			*(p++) = n;
			p++;
		}
	}
	else
	{
		for (i = 0; i < wid; i++)
		{
			n = fft_disp[snd_ch][i];

			memcpy(p, &RGBWF[n], 4);
			p += 4;
		}
	}

	// Scroll

	int TopLine = NextWaterfallLine[snd_ch];

	// Write line to cyclic buffer then draw starting with the line just written

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Line, 4096);
	if (NextWaterfallLine[snd_ch] > 79)
		NextWaterfallLine[snd_ch] = 0;

	for (int j = 79; j > 0; j--)
	{
		p = bm->scanLine(j);
		memcpy(p, &WaterfallLines[snd_ch][TopLine][0], lineLen);
		TopLine++;
		if (TopLine > 79)
			TopLine = 0;
	}

	WaterfallCopy[snd_ch]->setPixmap(QPixmap::fromImage(*bm));
	//	WaterfallCopy[snd_ch - 1]->setPixmap(*pm);
		//	WaterfallCopy[1]->setPixmap(QPixmap::fromImage(*bm));

}


	
QtSoundModem::~QtSoundModem()
{
	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", saveGeometry());
	mysettings.setValue("windowState", saveState());
	
	saveSettings();	
	Closing = TRUE;

	QThread::msleep(100);
}

int upd_time = 30;

void QtSoundModem::show_grid()
{
	// This refeshes the session list

	int  snd_ch, i, num_rows, row_idx;
	QTableWidgetItem *item;
	const char * msg;

	int  speed_tx, speed_rx;

	if (grid_time < 10)
	{
		grid_time++;
		return;
	}

	grid_time = 0;

	//label7.Caption = inttostr(stat_r_mem); mem_arq

	num_rows = 0;
	row_idx = 0;

	for (snd_ch = 0; snd_ch < 2; snd_ch++)
	{
		for (i = 0; i < port_num; i++)
		{
			if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
				num_rows++;
		}
	}

	if (num_rows == 0)
	{
		sessionTable->setRowCount(0);
		sessionTable->setRowCount(1);
	}
	else
		sessionTable->setRowCount(num_rows);


	for (snd_ch = 0; snd_ch < 2; snd_ch++)
	{
		for (i = 0; i < port_num; i++)
		{
			if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
			{
				switch (AX25Port[snd_ch][i].status)
				{
				case STAT_NO_LINK:

					msg = "No link";
					break;

				case STAT_LINK:

					msg = "Link";
					break;

				case STAT_CHK_LINK:

					msg = "Chk link";
					break;

				case STAT_WAIT_ANS:

					msg = "Wait ack";
					break;

				case STAT_TRY_LINK:

					msg = "Try link";
					break;

				case STAT_TRY_UNLINK:

					msg = "Try unlink";
				}


				item = new QTableWidgetItem((char *)AX25Port[snd_ch][i].mycall);
				sessionTable->setItem(row_idx, 0, item);

				item = new QTableWidgetItem(AX25Port[snd_ch][i].kind);
				sessionTable->setItem(row_idx, 10, item);

				item = new QTableWidgetItem((char *)AX25Port[snd_ch][i].corrcall);
				sessionTable->setItem(row_idx, 1, item);

			item = new QTableWidgetItem(msg);
				sessionTable->setItem(row_idx, 2, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_s_pkt));
				sessionTable->setItem(row_idx, 3, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_s_byte));
				sessionTable->setItem(row_idx, 4, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_pkt));
				sessionTable->setItem(row_idx, 5, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_byte));
				sessionTable->setItem(row_idx, 6, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_fc));
				sessionTable->setItem(row_idx, 7, item);

				if (grid_timer != upd_time)
					grid_timer++;
				else
				{
					grid_timer = 0;
					speed_tx = round(abs(AX25Port[snd_ch][i].info.stat_s_byte - AX25Port[snd_ch][i].info.stat_l_s_byte) / upd_time);
					speed_rx = round(abs(AX25Port[snd_ch][i].info.stat_r_byte - AX25Port[snd_ch][i].info.stat_l_r_byte) / upd_time);

					item = new QTableWidgetItem(QString::number(speed_tx));
					sessionTable->setItem(row_idx, 8, item);

					item = new QTableWidgetItem(QString::number(speed_rx));
					sessionTable->setItem(row_idx, 9, item);

					AX25Port[snd_ch][i].info.stat_l_r_byte = AX25Port[snd_ch][i].info.stat_r_byte;
					AX25Port[snd_ch][i].info.stat_l_s_byte = AX25Port[snd_ch][i].info.stat_s_byte;	
				}

				row_idx++;
			}
		}
	}
}