
#include "ARDOP_GUI.h"
#include <QtWidgets>
#include "QtNetwork\QTNetwork"
#include "QtNetwork\QTcpsocket.h"


ARDOP_GUI::ARDOP_GUI(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	int sizeX = 150;
	int sizeY = 150;
	int i;
	char errMsg[80];

/*

QTcpServer * Server = new QTcpServer();


	if (Server->listen(QHostAddress("0.0.0.0"), 9999))
	{
		QMessageBox::question(this, tr("Listen OK"), tr("Listen OK"), QMessageBox::Yes | QMessageBox::No);
	}
	else
	{
		QMessageBox::question(this, tr("Listen Failed"), tr("Listen Failed"), QMessageBox::Yes | QMessageBox::No);
	}

*/
	// create the socket and connect various of its signals

	socket = new QTcpSocket();

	connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError()));

	// connect to the server

	qDebug() << "connecting...";

	socket->connectToHost(QHostAddress("127.0.0.1"), 8010);

	if (socket->waitForConnected(5000))
	{
		qDebug() << "Connected!";

		sprintf(errMsg, "%d %s", socket->state(), socket->errorString().toLocal8Bit().constData());
		qDebug() << errMsg;

		// send
	//	socket->write("Hello server\r\n\r\n");
	//	socket->waitForBytesWritten(1000);
	//	socket->waitForReadyRead(3000);
//
//		qDebug() << "Reading: " << socket->bytesAvailable();

		// get the data
//		qDebug() << socket->readAll();

		// close the connection
//		socket->close();
	}
	else
	{
		qDebug() << "Not connected!";
		sprintf(errMsg, "%d %s", socket->state(), socket->errorString().toLocal8Bit().constData());
		qDebug() << errMsg;

	}

	sprintf(errMsg, "%d %s", socket->state(), socket->errorString().toLocal8Bit().constData());
	qDebug() << errMsg;

	image = new QImage (100, 100, QImage::Format_RGB32);
	QRgb value;

	value = qRgb(255, 255, 255); // 0xffbd9527
	image->fill(value);

	value = qRgb(0, 0, 0); // 0xffedba31

	ui.Constellation->setPixmap(QPixmap::fromImage(*image));


	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(300);
}

void ARDOP_GUI::MyTimerSlot()
{
	char errMsg[80];

	socket->waitForReadyRead(10);

//	if (socket->bytesAvailable())
//	{
//		qDebug() << "Reading: " << socket->bytesAvailable();

		// get the data
//		qDebug() << socket->readAll();

//		sprintf(errMsg, "%d %s", socket->state(), socket->errorString().toLocal8Bit().constData());
//		qDebug() << errMsg;
//	}
}

void ARDOP_GUI::socketConnected()
{
	qDebug() << "Connected";
}

void  ARDOP_GUI::socketReadyRead()
{
	qDebug() << "Reading: " << socket->bytesAvailable();

	// get the data
	qDebug() << socket->readAll();

//	QMessageBox::question(this, "ARDOP GUI", "socketReadyRead", QMessageBox::Yes | QMessageBox::No);
}

void  ARDOP_GUI::socketClosed()
{
	QMessageBox::question(this, "ARDOP GUI", "Connection closed", QMessageBox::Yes | QMessageBox::No);
}

void  ARDOP_GUI::socketError()
{
	char errMsg[80];
	sprintf(errMsg, "%d %s", socket->state(), socket->errorString().toLocal8Bit().constData());
//	qDebug() << errMsg;
//	QMessageBox::question(NULL, "ARDOP GUI", errMsg, QMessageBox::Yes | QMessageBox::No);
}

