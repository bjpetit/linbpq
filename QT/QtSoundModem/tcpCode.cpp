
#include <QMessageBox>
#include "QtSoundModem.h"
#include "UZ7HOStuff.h"

#define CONNECT(sndr, sig, rcvr, slt) connect(sndr, SIGNAL(sig), rcvr, SLOT(slt))

QList<QTcpSocket*>  _KISSSockets;
QList<QTcpSocket*>  _AGWSockets;

QTcpServer * _KISSserver;
QTcpServer * _AGWserver;

extern workerThread *t;

extern "C" int KISSPort;

extern bool Closing;				// Set to stop background thread

extern "C"
{
	void KISSDataReceived(void * sender, char * data, int length);
	void AGW_explode_frame(void * soket, char * data, int len);
	void KISS_add_stream(void * Socket);
	void KISS_del_socket(void * Socket);
	void AGW_add_socket(void * Socket);
	void AGW_del_socket(void * socket);
	void Debugprintf(const char * format, ...);
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
	void set_speed(int snd_ch, int Modem);
	void init_speed(int snd_ch);

}

extern "C" int nonGUIMode;

void mynet::start()
{
	if (KISSServ)
	{
		_KISSserver = new(QTcpServer);

		if (_KISSserver->listen(QHostAddress::Any, KISSPort))
			connect(_KISSserver, SIGNAL(newConnection()), this, SLOT(onKISSConnection()));
		else
		{
			QMessageBox msgBox;
			msgBox.setText("Listen failed for KISS Port.");
			msgBox.exec();
		}
	}

	if (AGWServ)
	{
		_AGWserver = new(QTcpServer);
		if (_AGWserver->listen(QHostAddress::Any, AGWPort))
			connect(_AGWserver, SIGNAL(newConnection()), this, SLOT(onAGWConnection()));
		else
		{
			QMessageBox msgBox;
			msgBox.setText("Listen failed for AGW Port.");
			msgBox.exec();
		}
	}

	QObject::connect(t, SIGNAL(sendtoKISS(void *, unsigned char *, int)), this, SLOT(sendtoKISS(void *, unsigned char *, int)), Qt::QueuedConnection);


	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);
}

void mynet::MyTimerSlot()
{
	// 100 mS Timer Event

	TimerEvent = TIMER_EVENT_ON;
}


void mynet::onAGWConnection()
{
	QTcpSocket *clientSocket = _AGWserver->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onAGWReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onAGWSocketStateChanged(QAbstractSocket::SocketState)));

	_AGWSockets.push_back(clientSocket);

	AGW_add_socket(clientSocket);

	Debugprintf("AGW Connect Sock %x", clientSocket);
}


void mynet::onAGWSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

		AGW_del_socket(sender);

		_AGWSockets.removeOne(sender);
	}
}

void mynet::onAGWReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();

	AGW_explode_frame(sender, datas.data(), datas.length());
}


void mynet::onKISSConnection()
{
	QTcpSocket *clientSocket = _KISSserver->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onKISSReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onKISSSocketStateChanged(QAbstractSocket::SocketState)));

	_KISSSockets.push_back(clientSocket);

	KISS_add_stream(clientSocket);

	Debugprintf("KISS Connect Sock %x", clientSocket);
}

void mynet::onKISSSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

		KISS_del_socket(sender);

		_KISSSockets.removeOne(sender);
	}
}

void mynet::onKISSReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();

	KISSDataReceived(sender, datas.data(), datas.length());
}



void mynet::displayError(QAbstractSocket::SocketError socketError)
{
	if (socketError == QTcpSocket::RemoteHostClosedError)
		return;

	qDebug() << tcpClient->errorString();

	tcpClient->close();
	tcpServer->close();
}



void mynet::sendtoKISS(void * sock, unsigned char * Msg, int Len)
{
	if (sock == NULL)
	{
		for (QTcpSocket* socket : _KISSSockets)
		{
			socket->write((char *)Msg, Len);
		}
	}
	else
	{
		QTcpSocket* socket = (QTcpSocket*)sock;
		socket->write((char *)Msg, Len);
	}
	free(Msg);
}


extern "C" void KISSSendtoServer(void * sock, byte * Msg, int Len)
{
	emit t->sendtoKISS(sock, Msg, Len);
}

void workerThread::run()
{
	soundMain();

	if (!InitSound(1))
	{
		//		QMessageBox msgBox;
		//		msgBox.setText("Open Sound Card Failed");
		//		msgBox.exec();
	}

	// Initialise Modems

	init_speed(0);
	init_speed(1);

	//	emit t->openSockets();

	while (Closing == 0)
	{
		// Run scheduling loop

		MainLoop();

		this->msleep(10);
	}

	qApp->exit();

};


