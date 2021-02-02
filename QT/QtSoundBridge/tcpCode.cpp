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

#include <QMessageBox>
#include "QtSoundBridge.h"
#include "UZ7HOStuff.h"


#define CONNECT(sndr, sig, rcvr, slt) connect(sndr, SIGNAL(sig), rcvr, SLOT(slt))

QList<QTcpSocket*>  _KISSSockets;
QList<QTcpSocket*>  _AGWSockets;

QTcpServer * _KISSserver;
QTcpServer * _AGWserver;

extern workerThread *t;
extern mynet m1;

extern "C" int KISSPort;
extern "C" void * initPulse();
extern "C" int SoundMode;

extern int UDPPort;
extern int TXPort;

int UDPServ = 1;

extern void saveSettings();

extern int Closing;				// Set to stop background thread

char LevelMsg[64] = "";

extern "C"
{
	void Debugprintf(const char * format, ...);
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
}

void mynet::start()
{
	if (SoundMode == 3)
		OpenUDP();

	if (UDPServ)
		OpenUDP();

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);
}

void mynet::MyTimerSlot()
{
	// 100 mS Timer Event

//	TimerEvent = TIMER_EVENT_ON;
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


void workerThread::run()
{
	if (SoundMode == 2)			// Pulse
	{
		if (initPulse() == nullptr)
		{
			QMessageBox msgBox;
			msgBox.setText("PulseAudio requested but pulseaudio libraries not found\nMode set to ALSA");
			msgBox.exec();
		
			SoundMode = 0;
			saveSettings();
		}
	}

	soundMain();

	if (SoundMode != 3)
	{
		if (!InitSound(1))
		{
			//		QMessageBox msgBox;
			//		msgBox.setText("Open Sound Card Failed");
			//		msgBox.exec();
		}
	}

	//	emit t->openSockets();

	while (Closing == 0)
	{
		// Run scheduling loop

		MainLoop();

		this->msleep(10);
	}

	qDebug() << "Saving Settings";

	saveSettings();

	qDebug() << "Main Loop exited";

	qApp->exit();

};

// Audio over UDP Code.

// Code can either send audio blocks from the sound card as UDP packets or use UDP packets from 
// a suitable source (maybe another copy of QtSM) and process them instead of input frm a sound card/

// ie act as a server or client for UDP audio.

// of course we need bidirectional audio, so even when we are a client we send modem generated samples
// to the server and as a server pass received smaples to modem

// It isn't logical to run as both client and server, so probably can use just one socket


QUdpSocket * udpSocket;

extern qint64 udpServerSeqno= 0;
qint64 udpClientLastSeq = 0;
qint64 udpServerLastSeq = 0;
extern "C" int droppedPackets = 0;
extern "C" int missedPackets = 0;
extern char UDPHost[64];

QQueue <unsigned char *> queue;
QMutex mutex;
extern mynet m1;

QTimer *timer;
QTimer *timercopy;

void mynet::OpenUDP()
{
	udpSocket = new QUdpSocket();

	udpSocket->bind(QHostAddress("0.0.0.0"), UDPPort);

	connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

	QTimer *timer = new QTimer(this);
	timercopy = timer;
	connect(timer, SIGNAL(timeout()), this, SLOT(dropPTT()));
}

void mynet::dropPTT()
{

}

void mynet::readPendingDatagrams()
{
	while (udpSocket->hasPendingDatagrams())
	{
		QHostAddress Addr;
		quint16 rxPort;
		unsigned char ucopy[1501];
		char copy[1501];
		int Dup = 0;

		// We expect datagrams of 1040 bytes containing a 16 byte header and 512 16 bit samples
		// We should get a datagram every 43 mS. We need to use a timeout to drop PTT

//		timercopy->start(1000);

		int Len = udpSocket->readDatagram(copy, 1500, &Addr, &rxPort);

		if (Len == 1040)
		{
			qint64 Seq;
			unsigned char txBuff[1048];

			memcpy(&Seq, copy, sizeof(udpServerSeqno));

			if (Seq == udpClientLastSeq)
			{
				Dup++;
				continue;
			}

			if (Seq < udpClientLastSeq || udpClientLastSeq == 0)

				// Client or Server Restarted

				udpClientLastSeq = Seq;

			else
			{
				int Missed = Seq - udpClientLastSeq;

				if (Missed > 100)			// probably stopped in debug
					Missed = 1;

				while (--Missed)
				{
					missedPackets++;
					
					// insert silence to maintian timing

					unsigned char * pkt = (unsigned char *)malloc(1024);

					memset(pkt, 0, 1024);

					mutex.lock();
					queue.append(pkt);
					mutex.unlock();
				}
			}

			udpClientLastSeq = Seq;

			unsigned char * pkt = (unsigned char *)malloc(1024);

			memcpy(pkt, &copy[16], 1024);

			mutex.lock();
			queue.append(pkt);
			mutex.unlock();
		}
	}
}

void  mynet::socketError()
{
	char errMsg[80];
	sprintf(errMsg, "%d %s", udpSocket->state(), udpSocket->errorString().toLocal8Bit().constData());
	//	qDebug() << errMsg;
	//	QMessageBox::question(NULL, "ARDOP GUI", errMsg, QMessageBox::Yes | QMessageBox::No);
}

extern "C" void sendSamplestoUDP(short * Samples, int nSamples, int Port)
{
	if (udpSocket == nullptr)
		return;
	
	unsigned char txBuff[1048];

	memcpy(txBuff, &udpServerSeqno, sizeof(udpServerSeqno));
	udpServerSeqno++;

	if (nSamples > 512)
		nSamples = 512;

	nSamples <<= 1;				// short to byte

	memcpy(&txBuff[16], Samples, nSamples);

	int n = udpSocket->writeDatagram((char *)txBuff, nSamples + 16, QHostAddress(UDPHost), Port);
}

static int min = 0, max = 0, lastlevelGUI = 0, lastlevelreport = 0;

static UCHAR CurrentLevel = 0;		// Peak from current samples

extern "C" short * SendtoCard(short * buf, int n);
extern "C" short * DMABuffer;
extern char UDPHost[64];

extern "C" void SendReceivedSamples()
{
	// Process any samples from UDP

	while (!queue.isEmpty())
	{
		if (queue.length() > 3)
		{
			droppedPackets++;
			Debugprintf("Dropping Packet to limit Q to 3");
			mutex.lock();
			short * ptr = (short *)queue.dequeue();
			mutex.unlock();
			free(ptr);
		}

		mutex.lock();
		short * ptr = (short *)queue.dequeue();
		mutex.unlock();
		short * save = ptr;

		// We get mono samples but soundcard expects stereo

		short * inptr = (short *)ptr;
		short * outptr = DMABuffer;

		for (int n = 0; n < 512; n++)
		{
			*(outptr++) = *inptr;
			*(outptr++) = *inptr++;	// Duplicate
		}

		DMABuffer = SendtoCard(DMABuffer, SendSize);

		free(save);
	}
}

int transmitting = 0;

int dispmax = 0;
int dispmin = 0;

extern "C" void ProcessNewSamples(short * Samples, int nSamples)
{
	// Send to UDP if audio is active (audio VOX)

	// we get strero data so Extract just left

	short Buff[1024];
	short * ptr = Samples;
	int i;
	int min = 0;
	int max = 0;
	short sample;

	int i1 = 0;

	for (i = 0; i < nSamples; i++)
	{
		sample = Samples[i1];
		Buff[i] = sample;
		i1 += 2;

		// Audo VOX

		if (sample < min)
		min = sample;
		else if (sample > max)
			max = sample;
	}

	if (max < 100)
	{
		if (transmitting)
		{
			// Audio just ending. Send special "Drop PTT" packet

			transmitting = 0;
			memset(Buff, 0, 1024);
			sendSamplestoUDP(Buff, 1024, TXPort);
			sendSamplestoUDP(Buff, 1024, TXPort);		// Twice to protect against loss
			dispmin = 0;
			dispmax = 0;
			sprintf(LevelMsg, "%d %d", dispmin, dispmax);
		}

		return;				// Too quiet
	}

	transmitting = 1;

	if (min < dispmin)
		dispmin = min;
	if (max > dispmax)
		dispmax = max;

	sendSamplestoUDP(Buff, 512, TXPort);
	sprintf(LevelMsg, "%d %d", dispmin, dispmax);

}






