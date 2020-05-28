#pragma once

#include <QtWidgets/QMainWindow>
#include "QMessageBox"
#include "QComboBox"
#include "ui_BPQConfigGen.h"
#include "QDialogButtonBox"
#include "QTextEdit"
#include "qdebug.h"
#include "QFile"
#include "QDateTime"
#include "QDir"
#include "QSettings"
#include "QtCore"
#include <QSysInfo>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QFileInfo>

class BPQConfigGen : public QMainWindow
{
	Q_OBJECT

public:
	BPQConfigGen(QWidget *parent = Q_NULLPTR);

private slots:
	void clickedSlot();

private:
	Ui::BPQConfigGenClass ui;
	void downloadFile(const char * Filename, int Beta, int ARM);
	void networkError(QNetworkReply::NetworkError error);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void replyFinished(QNetworkReply *reply);
	void SaveDownloadedFile(QByteArray * Data, int Len, const char * Name, char * programPath, bool unzip);
	void resizeEvent(QResizeEvent * e);
	void datestampedbackup(char * FN);
	void ReadConfigFile();
	void WriteARDOPPort(FILE * fp);
	void WriteUZ7HOPort(FILE * fp);
	void WriteWINMORPort(FILE * fp);
	void WriteVARAPort(FILE * fp);
	void AddTCPKISSPort(FILE * fp);
	void GetParams();
	void CreateConfig();
	void editPort(int i);
	void saveAppl(int i);
	void WriteAPRSConfig(FILE * fp);
	void refreshAPRS();
	void SaveAPRS();
	void getVersion(char * Program, char * Version);
	void getVersions();
	void doUpdate(int Beta);
};



class PortDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PortDialog(QWidget *parent = 0);
	~PortDialog();

public:
	QPushButton *okButton;
	QPushButton *cancelButton;

private slots:
	void PortTypeChanged(int Selected);
	void myaccept();
	void myreject();

private:
	QDialogButtonBox *buttonBox;
};


class editPortDialog : public QDialog
{
	Q_OBJECT

public:
	explicit editPortDialog(int n, QWidget *parent = 0);
	~editPortDialog();

public:
	QPushButton *okButton;
	QPushButton *cancelButton;

private slots:
	void myaccept();
	void myreject();

private:
	QDialogButtonBox *buttonBox;
};

class editOtherDialog : public QDialog
{
	Q_OBJECT

public:
	explicit editOtherDialog(int n, QWidget *parent = 0);
	~editOtherDialog();

public:
	QPushButton *okButton;
	QPushButton *cancelButton;

private slots:
	void myaccept();
	void myreject();

private:
	QDialogButtonBox *buttonBox;
};


class editAPRSDialog : public QDialog
{
	Q_OBJECT

public:
	explicit editAPRSDialog(int n, QWidget *parent = 0);
	~editAPRSDialog();

public:
	QPushButton *okButton;
	QPushButton *cancelButton;

private slots:
	void myaccept();
	void myreject();

private:
	QDialogButtonBox *buttonBox;
};
