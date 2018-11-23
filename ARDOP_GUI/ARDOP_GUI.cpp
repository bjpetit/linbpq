#include "ardop_gui.h"
#include "ui_ardop_gui.h"
#include "TabDialog.h"

#include "QtNetwork/QUdpSocket"
#include "QTimer"
#include "QSettings"

int Keepalive = 0;

QRgb white = qRgb(255, 255, 255);
QRgb black = qRgb(0, 0, 0);
QRgb green = qRgb(0, 255, 0);
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

unsigned char  WaterfallLines[64][220] = {0};
int NextWaterfallLine = 0;

char Host[256]= "";
char Port[16] = "";
int PortNum = 0;

ARDOP_GUI::ARDOP_GUI(QWidget *parent) : QMainWindow(parent), ui(new Ui::ARDOP_GUI)
{
    char errMsg[80];
    QByteArray qb;
    char * ptr;
	int i;

    ui->setupUi(this);

    // Get Host and Port unless provided on command 

	if (Host[0] == 0 && Port[0] == 0)
	{
		QSettings settings("G8BPQ", "ARDOP_GUI");

		qb = settings.value("Host").toByteArray();
		ptr = qb.data();
		strcpy(Host, ptr);

		qb = settings.value("Port").toByteArray();
		ptr = qb.data();
		strcpy(Port, ptr);
	}

    // Create Menus

	configMenu = ui->menuBar->addMenu(tr("File"));
    actConfigure = new QAction(tr("Configure"), this);
    configMenu->addAction(actConfigure);

	graphicsMenu = ui->menuBar->addMenu(tr("Graphics"));
	actWaterfall = new QAction(tr("Waterfall"), this);
	graphicsMenu->addAction(actWaterfall);
	actSpectrum = new QAction(tr("Spectrum"), this);
	graphicsMenu->addAction(actSpectrum);
	actDisabled = new QAction(tr("Disable"), this);
	graphicsMenu->addAction(actDisabled);

	sendMenu = ui->menuBar->addMenu(tr("Send"));
	actSendID = new QAction(tr("Send ID"), this);
	sendMenu->addAction(actSendID);
	actTwoToneTest = new QAction(tr("Send Two Tone Test"), this);
	sendMenu->addAction(actTwoToneTest);
	actSendCWID = new QAction(tr("Send CWID"), this);
	sendMenu->addAction(actSendCWID);

	abortMenu = ui->menuBar->addMenu(tr("Abort"));

	Busy = new QLabel(this);
	Busy->setFixedHeight(20);
	Busy->setAlignment(Qt::AlignCenter);
	Busy->setText("  Channel Busy  ");
	Busy->setStyleSheet("QLabel { background-color : rgb(255, 215, 0); color : black; }");

	ui->menuBar->setCornerWidget(Busy);

//	Busy->setVisible(false);

	connect(actConfigure, &QAction::triggered, this, &ARDOP_GUI::Configure);
	connect(actWaterfall, &QAction::triggered, this, &ARDOP_GUI::setWaterfall);
	connect(actSpectrum, &QAction::triggered, this, &ARDOP_GUI::setSpectrum);
	connect(actDisabled, &QAction::triggered, this, &ARDOP_GUI::setDisabled);
	connect(actSendID, &QAction::triggered, this, &ARDOP_GUI::setSendID);
	connect(actSendCWID, &QAction::triggered, this, &ARDOP_GUI::setSendCWID);
	connect(actTwoToneTest, &QAction::triggered, this, &ARDOP_GUI::setSend2ToneTest);

    while (Host[0] == 0 || Port[0] == 0)
    {
        // Request Config

        TabDialog tabdialog(0);
        tabdialog.exec();
    }

    PortNum = atoi(Port);

    udpSocket = new QUdpSocket();
    udpSocket->bind(QHostAddress("0.0.0.0"), PortNum + 1);		// We send from Port + 1

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

    sprintf(errMsg, "%d %s", udpSocket->state(), udpSocket->errorString().toLocal8Bit().constData());
    qDebug() << errMsg;

    udpSocket->writeDatagram("ARDOP_GUI Running", 17, QHostAddress(Host), PortNum);

    Constellation = new QImage(91, 91, QImage::Format_RGB32);
    Waterfall = new QImage(205, 64, QImage::Format_Indexed8);
    RXLevel = new QImage(150, 10, QImage::Format_RGB32);

    Waterfall->setColorCount(16);

	for (i = 0; i < 16; i++)
	{
		Waterfall->setColor(i, vbColours[i]);
	}

    Constellation->fill(black);

    RefreshLevel(40);

    ui->Constellation->setPixmap(QPixmap::fromImage(*Constellation));
    ui->Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));
    ui->RXLevel->setPixmap(QPixmap::fromImage(*RXLevel));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
    timer->start(30);

	QCoreApplication::sendPostedEvents();
}

void ARDOP_GUI::Configure()
{
 TabDialog tabdialog(0);
    tabdialog.exec();
}

void ARDOP_GUI::setWaterfall()
{
	udpSocket->writeDatagram("Waterfall", 9, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::setSpectrum()
{
	udpSocket->writeDatagram("Spectrum", 8, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::setDisabled()
{
	udpSocket->writeDatagram("Disable", 7, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::setSendID()
{
	udpSocket->writeDatagram("SENDID", 6, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::setSendCWID()
{
	udpSocket->writeDatagram("SENDCWID", 8, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::setSend2ToneTest()
{
	udpSocket->writeDatagram("TWOTONETEST", 11, QHostAddress(Host), PortNum);
}

void ARDOP_GUI::MyTimerSlot()
{
    Keepalive++;

    if (Keepalive > 333)
    {
        Keepalive = 0;
        udpSocket->writeDatagram("ARDOP_GUI Running", 17, QHostAddress(Host), PortNum);
    }
}

void ARDOP_GUI::readPendingDatagrams()
{
	while (udpSocket->hasPendingDatagrams())
	{
		QHostAddress Addr;
		quint16 Port;
		unsigned char ucopy[1500];
		char copy[1500];

		int Len = udpSocket->readDatagram(copy, 1500, &Addr, &Port);

		//		QNetworkDatagram datagram = udpSocket->receiveDatagram();
		//		QByteArray * ba = &datagram.data();
		//		unsigned char copy[1500];
		//     unsigned char * ptr = &copy[1];

		char Msg[80] = "";

		if (Len > 1500 || Len < 0)
			return;					// ignore it too big

		copy[Len] = 0;

		memcpy(ucopy, copy, Len);

		switch (copy[0])
		{
		case 'L':					// Signal Level
			RefreshLevel(ucopy[1]);
			sprintf(Msg, "%d %c %d", Len, copy[0], ucopy[1]);
			break;

		case 'C':					//Constellation Data
			RefreshConstellation(&ucopy[1], (Len - 1) / 3);		// 3 bytes per pixel x, y, colour
			return;

		case 'X':					// Spectrum display
			RefreshSpectrum(&ucopy[1], (Len - 1));
			return;

		case 'W':					//Waterfall Data
			RefreshWaterfall(&ucopy[1], (Len - 1));
			return;

		case 'S':					// Protocol State
			sprintf(Msg, "%d %c %s", Len, copy[0], &copy[1]);
			ui->State->setText(&copy[1]);
			break;

		case 'T':					// TX Frame Type
			sprintf(Msg, "%d %c %s", Len, copy[0], &copy[1]);
			ui->TXFrame->setText(&copy[1]);
			break;

		case 'Q':					// TX Frame Type
			sprintf(Msg, "%d %c %s", Len, copy[0], &copy[1]);
			ui->Quality->setText(&copy[1]);
			break;

		case 'R':					// RX Frame Type

			// First Byte is 0/1/2 - Pending/Good/Bad

			int colour = copy[1];

			sprintf(Msg, "%d %c %s", Len, copy[0], &copy[2]);

			QPalette palette = ui->RXFrame->palette();

			switch (colour)
			{
			case 0:
				palette.setColor(QPalette::Base, Qt::yellow);
				break;
			case 1:
				palette.setColor(QPalette::Base, Qt::green);
				break;
			case 2:
				palette.setColor(QPalette::Base, Qt::red);
			}
			ui->RXFrame->setPalette(palette);
			ui->RXFrame->setText(&copy[2]);
			break;
		}
		qDebug() << Msg;
	}
}

void  ARDOP_GUI::socketError()
{
    char errMsg[80];
    sprintf(errMsg, "%d %s", udpSocket->state(), udpSocket->errorString().toLocal8Bit().constData());
//	qDebug() << errMsg;
//	QMessageBox::question(NULL, "ARDOP GUI", errMsg, QMessageBox::Yes | QMessageBox::No);
}



unsigned int LastLevel = 255;
unsigned int LastBusy = 255;


void ARDOP_GUI::RefreshLevel(unsigned int Level)
{
    // Redraw the RX Level Bar Graph

    unsigned int  x, y;

    for (x = 0; x < 150; x++)
    {
        for (y = 0; y < 10; y++)
        {
            if (x < Level)
                RXLevel->setPixel(x, y, green);
            else
                RXLevel->setPixel(x, y, white);
        }
    }
    ui->RXLevel->setPixmap(QPixmap::fromImage(*RXLevel));
}

void ARDOP_GUI::RefreshConstellation(unsigned char * Data, int Count)
{
    int i;

    Constellation->fill(black);

    for (i = 0; i < 91; i++)
    {
        Constellation->setPixel(45, i, cyan);
        Constellation->setPixel(i, 45, cyan);
    }

    while (Count--)
    {
        Constellation->setPixel(Data[0], Data[1], vbColours[Data[2]]);
        Data += 3;
    }

    ui->Constellation->setPixmap(QPixmap::fromImage(*Constellation));
}

void ARDOP_GUI::RefreshSpectrum(unsigned char * Data, int Count)
{
	int i;

	// Last 4 bytes are level busy and Tuning lines

	Waterfall->fill(Black);

	if (Data[206] != LastLevel)
	{
		LastLevel = Data[206];
		RefreshLevel(LastLevel);
	}

	if (Data[207] != LastBusy)
	{
		LastBusy = Data[207];
		Busy->setVisible(LastBusy);
	}

	for (i = 0; i < 64; i++)
	{
		Waterfall->setPixel(Data[208], i, Lime);
		Waterfall->setPixel(Data[209], i, Lime);
	}

	for (i = 0; i < 205; i++)
	{
		int val = Data[0];

		if (val > 63)
			val = 63;

		Waterfall->setPixel(i, val, Yellow);
		if (val < 62)
			Waterfall->setPixel(i, val+1, Gold);
		Data++;
	}

	for (i = 0; i < 64; i++)
	{
		Waterfall->setPixel(103, i, Tomato);
	}

	ui->Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));

}

void ARDOP_GUI::RefreshWaterfall(unsigned char * Data, int Count)
{
    int j;
    unsigned char * Line;
    int len = Waterfall->bytesPerLine();
    int TopLine = NextWaterfallLine;

    // Write line to cyclic buffer then draw starting with the line just written

    memcpy(&WaterfallLines[NextWaterfallLine++][0], Data, Count - 2);
    if (NextWaterfallLine > 63)
        NextWaterfallLine = 0;

    for (j = 63; j > 0; j--)
    {
        Line = Waterfall->scanLine(j);
        memcpy(Line, &WaterfallLines[TopLine++][0], len);
        if (TopLine > 63)
            TopLine = 0;
    }

    ui->Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));

	// Next two bytes are level and busy

	if (Data[206] != LastLevel)
	{
		LastLevel = Data[206];
		RefreshLevel(LastLevel);
	}

	if (Data[207] != LastBusy)
	{
		LastBusy = Data[207];
		Busy->setVisible(LastBusy);
	}
}


ARDOP_GUI::~ARDOP_GUI()
{
 //   delete ui;
}
