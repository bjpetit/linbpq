#include <QtCore/QCoreApplication>
#include <QtNetwork>
//#include <QDebug>

#define CONNECT(sndr, sig, rcvr, slt) connect(sndr, SIGNAL(sig), rcvr, SLOT(slt))

class mynet : public QObject
{
	Q_OBJECT

signals:

	void HLSetPTT(int c);

public:
	void start();
	void OpenUDP();


public slots:
	void MyTimerSlot();
	void dropPTT();
	void displayError(QAbstractSocket::SocketError socketError);

	void sendtoKISS(void * sock, unsigned char * Msg, int Len);

	void readPendingDatagrams();
	void socketError();

private:
	QTcpServer* tcpServer;
	QTcpSocket* tcpClient;
	QTcpSocket* tcpServerConnection;
	int bytesToWrite;
	int bytesWritten;
	int bytesReceived;
	int TotalBytes;
	int PayloadSize;
};


class workerThread : public QThread
{
	Q_OBJECT
signals:
	void updateDCD(int, int);

private:
	void run();
public:

};




