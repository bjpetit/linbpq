#include "boatspeed.h"
#include "boatspeed.h"
#include "ui_boatspeed.h"

#include <QtNetwork/QUdpSocket>
#include "QTimer"
#include "QSettings"
#include <QScrollBar>
#include <QPainter>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

int PortNum = 45000;
QUdpSocket * udpSocket;

int last = 0;		// time since last message
int Count = 0;
double maxSOG = 0;
double SOG, COG, Lat, Lon;
int col = 0;

int graphWidth = 1600;
int graphHeight = 600;

QFile * file;

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}


BoatSpeed::BoatSpeed(QWidget *parent): QMainWindow(parent), ui(new Ui::BoatSpeed)
{
    ui->setupUi(this);

	udpSocket = new QUdpSocket();
	udpSocket->bind(QHostAddress("0.0.0.0"), PortNum);		// We send from Port + 1

	connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
//	udpSocket->writeDatagram("Poll", 4, QHostAddress("192.168.1.132"), PortNum);
	udpSocket->writeDatagram("Poll", 4, QHostAddress::Broadcast, PortNum);
	connect(ui->Reset, SIGNAL(clicked()), this, SLOT(Reset()));

	Graph = new QImage(graphWidth, graphHeight, QImage::Format_RGB32);
	Reset();
	
/*
auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	if (path.isEmpty()) qFatal("Cannot determine settings storage location");

	QDir d{ path };
	if (d.mkpath(d.absolutePath()) && QDir::setCurrent(d.absolutePath()))
	{
		qDebug() << "settings in" << QDir::currentPath();
	}
	ui->Log->insertPlainText(QDir::currentPath());

	QPixmap pixmap;
	pixmap = pixmap.fromImage(*Graph);

	QPainter painter(&pixmap);
	painter.drawLine(0, 0, 40, 0);

	painter.setFont(QFont("Arial"));
	painter.drawText(QPoint(100, 100), "Hello");
*/

	QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	qWarning("folder=%s", qPrintable(folder));
	ui->Log->insertPlainText(folder);

	file = new QFile(folder + "/rmc.txt");


	file->open(QIODevice::WriteOnly | QIODevice::Append);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(500);
}

BoatSpeed::~BoatSpeed()
{
	file->close();
	delete ui;
}


void BoatSpeed::MyTimerSlot()
{
	ui->Last->setText(QString::number(last / 2));
	last++;
	udpSocket->writeDatagram("Poll", 4, QHostAddress("192.168.4.1"), PortNum);
//	udpSocket->writeDatagram("Poll", 4, QHostAddress::Broadcast, PortNum);
}

void BoatSpeed::Reset()
{
	last = 0;		// time since last message
	Count = 0;
	maxSOG = 0;
	col = 0;
	SOG = 0.0;

	Graph->fill(qRgb(255, 255, 255));
	ui->Graph->setPixmap(QPixmap::fromImage(*Graph));
	ui->Log->clear();

	ui->Speed->setText(QString::number(SOG));
	ui->maxSpeed->setText(QString::number(maxSOG));

	for (int i = 0; i < 10; i++)
	{
		Graph->setPixel(i, 600 - (10 * 10), qRgb(0, 0, 0));
		Graph->setPixel(i, 600 - (20 * 10), qRgb(0, 0, 0));
		Graph->setPixel(i, 600 - (30 * 10), qRgb(0, 0, 0));
		Graph->setPixel(i, 600 - (40 * 10), qRgb(0, 0, 0));
		Graph->setPixel(i, 600 - (50 * 10), qRgb(0, 0, 0));
		Graph->setPixel(i, 600 - (60 * 10), qRgb(0, 0, 0));
	}
}


void BoatSpeed::ProcessRMC(char * msg)
{
	char * ptr1;
	char * ptr2;
	char TimHH[3], TimMM[3], TimSS[3];
	char OurSog[5], OurCog[4];
	char LatDeg[3], LonDeg[4];
	char Day[3];
	char Copy[1024];

	strcpy(Copy, msg);

	ui->Log->insertPlainText(msg);
	ui->Log->ensureCursorVisible();
	ui->Log->verticalScrollBar()->setValue(ui->Log->verticalScrollBar()->maximum());

	Count++;
	ui->Count->setText(QString::number(Count));

	ptr1 = &msg[7];

	ptr2 = (char *)memchr(ptr1, ',', 15);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++) = 0;

	memcpy(TimHH, ptr1, 2);
	memcpy(TimMM, ptr1 + 2, 2);
	memcpy(TimSS, ptr1 + 4, 2);
	TimHH[2] = 0;
	TimMM[2] = 0;
	TimSS[2] = 0;

	ptr1 = ptr2;

	if (*(ptr1) != 'A') // ' Data Not Valid
	{
		return;
	}

	ptr1 += 2;

	ptr2 = (char *)memchr(ptr1, ',', 15);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++) = 0;

	memcpy(LatDeg, ptr1, 2);
	LatDeg[2] = 0;
	Lat = atof(LatDeg) + (atof(ptr1 + 2) / 60);


	ptr1 = ptr2;

	if ((*ptr1) == 'S') Lat = -Lat;

	ptr1 += 2;

	ptr2 = (char *)memchr(ptr1, ',', 15);

	if (ptr2 == 0) return;	// Duff
	*(ptr2++) = 0;


	memcpy(LonDeg, ptr1, 3);
	LonDeg[3] = 0;
	Lon = atof(LonDeg) + (atof(ptr1 + 3) / 60);

	ptr1 = ptr2;

	if ((*ptr1) == 'W') Lon = -Lon;

	ptr1 += 2;

	ptr2 = (char *)memchr(ptr1, ',', 30);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++) = 0;

	memcpy(OurSog, ptr1, 4);
	OurSog[4] = 0;

	last = 0;

	ptr1 = ptr2;

	ptr2 = (char *)memchr(ptr1, ',', 15);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++) = 0;

	file->write(Copy);
	file->flush();

	memcpy(OurCog, ptr1, 3);
	OurCog[3] = 0;

	memcpy(Day, ptr2, 2);
	Day[2] = 0;

	SOG = atof(OurSog);
	COG = atof(OurCog);

	if (SOG > maxSOG)
		maxSOG = SOG;

	ui->Speed->setText(OurSog);
	ui->maxSpeed->setText(QString::number(maxSOG));

	Graph->setPixel(col, 599 - (SOG * 10), qRgb(0, 0, 0));
	Graph->setPixel(col++, 600 - (SOG * 10), qRgb(0, 0, 0));
	Graph->setPixel(col, 599 - (SOG * 10), qRgb(0, 0, 0));
	Graph->setPixel(col++, 600 - (SOG * 10), qRgb(0, 0, 0));
	ui->Graph->setPixmap(QPixmap::fromImage(*Graph));

	if (col > graphWidth)
		col = 0;

}


void BoatSpeed::readPendingDatagrams()
{
	while (udpSocket->hasPendingDatagrams())
	{
		QHostAddress Addr;
		quint16 rxPort;
		char copy[1501];

		int Len = udpSocket->readDatagram(copy, 1500, &Addr, &rxPort);

		//		QNetworkDatagram datagram = udpSocket->receiveDatagram();
		//		QByteArray * ba = &datagram.data();
		//		unsigned char copy[1500];
		//     unsigned char * ptr = &copy[1];

		char Msg[1600] = "";

		if (Len > 1500 || Len < 0)
			return;					// ignore it too big

		copy[Len] = 0;

//		strlop(copy, 13);
//		strlop(copy, 10);


		if (memcmp(copy, "$GPRMC", 6) == 0 || memcmp(copy, "$GNRMC", 6) == 0)
		{
			ProcessRMC(copy);
		}

	}
}

void  BoatSpeed::socketError()
{
	char errMsg[80];
	sprintf(errMsg, "%d %s", udpSocket->state(), udpSocket->errorString().toLocal8Bit().constData());
	//	qDebug() << errMsg;
	//	QMessageBox::question(NULL, "ARDOP GUI", errMsg, QMessageBox::Yes | QMessageBox::No);
}


