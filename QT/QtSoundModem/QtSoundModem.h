#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtSoundModem.h"
#include "ui_calibrateDialog.h"
#include "ui_devicesDialog.h"
#include "ui_filterWindow.h"
#include "ui_ModemDialog.h"
#include "QThread"
#include <QLabel>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>


class QtSoundModem : public QMainWindow
{
	Q_OBJECT

public:
	QtSoundModem(QWidget *parent = Q_NULLPTR);
	~QtSoundModem();

	void RefreshWaterfall(int snd_ch, unsigned char * Data);

	QTcpServer  _KISSserver;
	QTcpServer  _AGWserver;

private slots:

	void doDevices();
	void MyTimerSlot();
	void openSockets();
	void doModems();
	void doFilter(int Chan, int Filter);
	void deviceaccept();
	void devicereject();
	void modemaccept();
	void modemreject();
	void handleButton(int Port, int Act);
	void doCalibrate();
	void onKISSSocketStateChanged(QAbstractSocket::SocketState socketState);
	void onKISSReadyRead();
	void onAGWSocketStateChanged(QAbstractSocket::SocketState socketState);
	void onAGWReadyRead();
	void doupdateDCD(int, int);
	void sendtoKISS(void * sock, char * Msg, int Len);
	void onKISSConnection();
	void onAGWConnection();
	void sendtoTrace(char * Msg, int tx);
	void preEmphAllAChanged(int);
	void preEmphAllBChanged(int);

protected:
	 
	void resizeEvent(QResizeEvent *event) override;

private:
	Ui::QtSoundModemClass ui;
	QTableWidget* sessionTable;
	QStringList m_TableHeader;

	QMenu *setupMenu;

	QAction *actDevices;
	QAction *actModems;
	QAction *actCalib;

	void RefreshSpectrum(unsigned char * Data);
	void show_grid();
};

class workerThread : public QThread
{
	Q_OBJECT
signals:
	void updateDCD(int, int);
	void sendtoTrace(char *, int);
	void sendtoKISS(void *, char *, int);
	void openSockets();

private:
	void run();
public:

};






