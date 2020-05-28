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

#include "tcpCode.h"


class QtSoundModem : public QMainWindow
{
	Q_OBJECT
signals:

	void HLSetPTT(int c);

public:
	QtSoundModem(QWidget *parent = Q_NULLPTR);
	void closeEvent(QCloseEvent * event);
	~QtSoundModem();

	void RefreshWaterfall(int snd_ch, unsigned char * Data);
	void show_grid();

private slots:

	void doDevices();
	void MyTimerSlot();
	void clickedSlotI(int i);
	void doModems();
	void doFilter(int Chan, int Filter);
	void SoundModeChanged(bool State);
	void DualPTTChanged(bool State);
	void CATChanged(bool State);
	void PTTPortChanged(int);
	void deviceaccept();
	void devicereject();
	void modemaccept();
	void modemreject();
	void handleButton(int Port, int Act);
	void doCalibrate();
	void doAbout();
	void doupdateDCD(int, int);
	void sendtoTrace(char * Msg, int tx);
	void preEmphAllAChanged(int);
	void preEmphAllBChanged(int);
	void menuChecked();
	void onTEselectionChanged();
	void HAMLIBdisplayError(QAbstractSocket::SocketError socketError);
	void HAMLIBreadyRead();
	void onHAMLIBSocketStateChanged(QAbstractSocket::SocketState socketState);
	void ConnecttoHAMLIB();
	void doHLSetPTT(int c);
	void clickedSlot();

protected:
	 
	bool eventFilter(QObject * obj, QEvent * evt);
	void resizeEvent(QResizeEvent *event) override;

private:
	Ui::QtSoundModemClass ui;
	QTableWidget* sessionTable;
	QStringList m_TableHeader;

	QMenu *setupMenu;
	QMenu *viewMenu;

	QAction *actDevices;
	QAction *actModems;
	QAction *actCalib;
	QAction *actAbout;
	QAction *actWaterfall1;
	QAction *actWaterfall2;


	void RefreshSpectrum(unsigned char * Data);
};
