#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtTermTCP.h"
#include "QTextEdit"
#include "QSplitter"
#include "QLineEdit"
#include "QTcpSocket"
#include <QDataStream>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTcpSocket;
class QNetworkSession;
QT_END_NAMESPACE

class QtTermTCP : public QMainWindow
{
	Q_OBJECT

public:
	QtTermTCP(QWidget *parent = Q_NULLPTR);
	~QtTermTCP();

private slots:
	void Connect(int i);
	void Disconnect();
	void SetupHosts(int i);
	void displayError(QAbstractSocket::SocketError socketError);
	void connected();
	void disconnected();
	void bytesWritten(qint64 bytes);
	void readyRead();
	void inputChanged(const QString &);
	void returnPressed();
	void selFont();
	void MyTimerSlot();
	void showContextMenuM(const QPoint &pt);
	void showContextMenuT(const QPoint &pt);
	void ontermselectionChanged();
	void onmonselectionChanged();
	void setSplit();

protected:
	void resizeEvent(QResizeEvent *event) override;
	bool eventFilter(QObject* obj, QEvent *event);

private:
	Ui::QtTermTCPClass ui;
	QSplitter *splitter;
	
	QMenu *connectMenu;
	QMenu *disconnectMenu;
	QMenu *setupMenu;
	QMenu *hostsubMenu;

	QAction *discAction;

};
