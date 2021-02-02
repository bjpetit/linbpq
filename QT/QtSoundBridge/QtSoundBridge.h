#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtSoundBridge.h"
#include "ui_devicesDialog.h"
#include "QThread"
#include <QLabel>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QSystemTrayIcon>

#include "tcpCode.h"


class QtSoundBridge : public QMainWindow
{
	Q_OBJECT

public:

	QtSoundBridge(QWidget *parent = Q_NULLPTR);
	void changeEvent(QEvent * e);
	void closeEvent(QCloseEvent * event);
	~QtSoundBridge();

	void onTEselectionChanged();

	void menuChecked();
	void initWaterfall(int chan, int state);

private slots:

	void doDevices();
	void MinimizetoTray();
	void TrayActivated(QSystemTrayIcon::ActivationReason reason);
	void MyTimerSlot();
	void clickedSlot();
	void SoundModeChanged(bool State);
	void deviceaccept();
	void devicereject();
	void handleButton(int Port, int Act);
	void doAbout();

protected:
	 
	bool eventFilter(QObject * obj, QEvent * evt);
	void resizeEvent(QResizeEvent *event) override;

private:
	Ui::QtSoundBridgeClass ui;
	QStringList m_TableHeader;

	QMenu *setupMenu;
	QMenu *viewMenu;

	QAction *actDevices;
	QAction *actModems;
	QAction *actMintoTray;
	QAction *actCalib;
	QAction *actAbout;
	QAction *actWaterfall1;
	QAction *actWaterfall2;

};

class myResize : public QObject
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
};

