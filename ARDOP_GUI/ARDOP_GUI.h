#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ARDOP_GUI.h"
#include "QtNetwork\QTcpsocket.h"

class ARDOP_GUI : public QMainWindow
{
	Q_OBJECT

public:
	ARDOP_GUI(QWidget *parent = Q_NULLPTR);

private:
	Ui::ARDOP_GUIClass ui;
private slots:
	void socketConnected();
	void socketReadyRead();
	void socketClosed();
	void socketError();
	void MyTimerSlot();
private:
	QImage *image;
	QTcpSocket * socket;
};
