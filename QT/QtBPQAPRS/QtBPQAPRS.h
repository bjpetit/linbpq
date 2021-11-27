#pragma once

#define _USE_32BIT_TIME_T	// Until the ASM code switches to 64 bit time


#include <QtWidgets/QMainWindow>
#include "ui_QtBPQAPRS.h"
#include <QLabel>
#include <QDialog>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QVariant>
#include <QHeaderView>
#include <QScrollArea>
#include <QTableWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QSystemTrayIcon>


#define TRACKPOINTS 100

class myQNetworkReply : public QNetworkReply
{
public:
	int Server;
	int Count;
};




typedef struct STATIONRECORD
{
	struct STATIONRECORD * Next;
	char Callsign[12];
	char Path[120];
	char Status[256];
	char LastPacket[392];		// Was 400. 8 bytes used for Approx Location Flag and Qt Icon pointer
	char Approx;
	char spare1;
	char spare2;
	char spare3;
	QImage * image;
	char LastWXPacket[256];
	int LastPort;
	double Lat;
	double Lon;
	double Course;
	double Speed;
	double Heading;
	double LatIncr;
	double LongIncr;
	double LastCourse;
	double LastSpeed;
	double Distance;
	double Bearing;
	double LatTrack[TRACKPOINTS];	// Cyclic Tracklog
	double LonTrack[TRACKPOINTS];
	time_t TrackTime[TRACKPOINTS];
	int Trackptr;					// Next record in Tracklog
	int Moved;						// Moved since last drawn
	time_t TimeAdded;
	time_t TimeLastUpdated;
	unsigned char Symbol;
	int iconRow;
	int iconCol;					// Symbol Pointer
	char IconOverlay;
	int DispX;						// Position in display buffer
	int DispY;
	int Index;						// List Box Index
	int NoTracks;					// Suppress displaying track
	QRgb TrackColour;
	char ObjState;					// Live/Killed flag. If zero, not an object
	char LastRXSeq[6];				// Seq from last received message (used for Reply-Ack system)
	int SimpleNumericSeq;			// Station treats seq as a number, not a text field
	struct STATIONRECORD * Object;	// Set if last record from station was an object
	time_t TimeLastTracked;			// Time of last trackpoint
	int NextSeq;

} StationRecord;


class QtBPQAPRS : public QMainWindow
{
    Q_OBJECT

public:
    QtBPQAPRS(QWidget *parent = Q_NULLPTR);
	void closeEvent(QCloseEvent * event);
	~QtBPQAPRS();
	void ZoomInOut();


private:
    Ui::QtBPQAPRSClass ui;
	void ReloadTiles(bool newTiles = false);
	void ReloadMap();
	void resizeEvent(QResizeEvent * e);
	int GetTile(int Zoom, int x, int y, int Server, int Count);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void replyFinished(QNetworkReply * reply);
	void OSMThread(int Server = -1);
	void GetMouseLatLon(double * Lat, double * Lon);
	void GetCentrePosition(double * Lat, double * Lon);
	int DrawStation(STATIONRECORD * ptr, int AllStations , QPainter * painter);
	void RefreshStationMap(int AllStations);
	void FindStationsByPixel(int MouseX, int MouseY, int wx, int wy);
	void CreateStationPopup(STATIONRECORD * ptr, int X, int Y);
	void CreateSelectionPopup(STATIONRECORD ** ptr, int Count, int X, int Y);
	void GetConfig();
	void SaveConfig();
	void GetMousePosn(int * MouseX, int * MouseY);

private slots:
	void stnDoubleClicked(int row, int col);
	void itemClicked(int row, int col);
	void VBarChanged(int val);
	void HBarChanged(int val);
	void MyTimer2();
	void MyTimer();
	void ZoomIn();
	void ZoomOut();
	void Setup();
	void myaccept();
	void myreject();
	void Messages();
	void clearTX();
	void clearRX();
	void cellDoubleClicked(int row, int col);
	void stateChanged(int);
	void returnPressed();
	void Stations();
	void callClicked(int row, int column);
	void sendBeacon();
	void FilterView();
	void Home();
	void OnlineHelp();
	void About();


protected:

	bool eventFilter(QObject * obj, QEvent * event);

};

//#include "Stations.h"




class StationDialog : public QDialog
{
	Q_OBJECT
private:
	void closeEvent(QCloseEvent *event);
public:
	explicit StationDialog(QWidget *parent = 0);
	void resizeEvent(QResizeEvent * e);
	~StationDialog();
	QScrollArea *scrollArea;
	QWidget *scrollAreaWidgetContents;

private slots:
//	void myaccept();
//	void myreject();

public:

};


class messageDialog : public QDialog
{
	Q_OBJECT

private:
	void closeEvent(QCloseEvent *event); 

public:
	explicit messageDialog(QWidget *parent = 0);

	void resizeEvent(QResizeEvent * e);

	QScrollArea *rxscrollArea;
	QWidget *rxContents;
	QScrollArea *txscrollArea;
	QWidget *txContents;
	QGroupBox *groupBox;
	QLineEdit *inputMessage;
	QComboBox *To;
	QLabel *label_2;
	QLabel *label;
	QGroupBox *groupBox_2;
	QCheckBox *checkBox;
	QCheckBox *checkBox_2;
	QCheckBox *checkBox_3;
	QCheckBox *checkBox_4;
	QCheckBox *checkBox_5;

};

class popupDialog : public QDialog
{
	Q_OBJECT

private:

	QLabel * popupLabel;

public:
	explicit popupDialog();
	void callClicked();
	void resizeEvent(QResizeEvent * e);
};

class Ui_configDialog : public QDialog
{
public:
	QWidget *layoutWidget;
	QHBoxLayout *hboxLayout;
	QPushButton *okButton;
	QPushButton *cancelButton;
	QLineEdit *trackExpireTime;
	QLineEdit *ISFIlter;
	QLineEdit *JPEGInterval;
	QLineEdit *JPEGFileName;
	QCheckBox *addCurrentView;
	QLabel *label;
	QLabel *label_4;
	QLabel *label_5;
	QLabel *label_10;
	QLabel *label_11;
	QLabel *label_6;
	QLineEdit *iconFontSize;

	QLineEdit *statusMsg;
	QCheckBox *suppressNoPosn;
	QCheckBox *useLocalTime;
	QCheckBox *distKM;
	QCheckBox *createJPEG;

	void setupUi(QDialog *configDialog)
	{
		if (configDialog->objectName().isEmpty())
			configDialog->setObjectName(QString::fromUtf8("configDialog"));
		configDialog->resize(715, 394);
		layoutWidget = new QWidget(configDialog);
		layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
		layoutWidget->setGeometry(QRect(142, 334, 351, 33));
		hboxLayout = new QHBoxLayout(layoutWidget);
		hboxLayout->setSpacing(6);
		hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
		hboxLayout->setContentsMargins(0, 0, 0, 0);
		okButton = new QPushButton(layoutWidget);
		okButton->setObjectName(QString::fromUtf8("okButton"));

		hboxLayout->addWidget(okButton);

		cancelButton = new QPushButton(layoutWidget);
		cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

		hboxLayout->addWidget(cancelButton);

		label_6 = new QLabel("Station Icon Font Size", configDialog);
		label_6->setGeometry(QRect(20, 10, 140, 20));
		iconFontSize = new QLineEdit(configDialog);
		iconFontSize->setGeometry(QRect(160, 10, 45, 20));

		trackExpireTime = new QLineEdit(configDialog);
		trackExpireTime->setObjectName(QString::fromUtf8("trackExpireTime"));
		trackExpireTime->setGeometry(QRect(160, 40, 45, 20));
		ISFIlter = new QLineEdit(configDialog);
		ISFIlter->setObjectName(QString::fromUtf8("ISFIlter"));
		ISFIlter->setGeometry(QRect(92, 100, 395, 20));
		JPEGInterval = new QLineEdit(configDialog);
		JPEGInterval->setObjectName(QString::fromUtf8("JPEGInterval"));
		JPEGInterval->setGeometry(QRect(308, 220, 47, 20));
		JPEGFileName = new QLineEdit(configDialog);
		JPEGFileName->setObjectName(QString::fromUtf8("JPEGFileName"));
		JPEGFileName->setGeometry(QRect(146, 260, 459, 20));
		addCurrentView = new QCheckBox(configDialog);
		addCurrentView->setObjectName(QString::fromUtf8("addCurrentView"));
		addCurrentView->setGeometry(QRect(502, 100, 205, 20));
		label = new QLabel(configDialog);
		label->setObjectName(QString::fromUtf8("label"));
		label->setGeometry(QRect(20, 40, 113, 20));
		label_4 = new QLabel(configDialog);
		label_4->setObjectName(QString::fromUtf8("label_4"));
		label_4->setGeometry(QRect(20, 100, 47, 20));
		label_5 = new QLabel(configDialog);
		label_5->setObjectName(QString::fromUtf8("label_5"));
		label_5->setGeometry(QRect(20, 140, 79, 20));
		label_10 = new QLabel(configDialog);
		label_10->setObjectName(QString::fromUtf8("label_10"));
		label_10->setGeometry(QRect(178, 220, 101, 20));
		label_11 = new QLabel(configDialog);
		label_11->setObjectName(QString::fromUtf8("label_11"));
		label_11->setGeometry(QRect(20, 262, 109, 20));
		statusMsg = new QLineEdit(configDialog);
		statusMsg->setObjectName(QString::fromUtf8("statusMsg"));
		statusMsg->setGeometry(QRect(92, 140, 497, 20));
		suppressNoPosn = new QCheckBox(configDialog);
		suppressNoPosn->setObjectName(QString::fromUtf8("suppressNoPosn"));
		suppressNoPosn->setGeometry(QRect(20, 180, 237, 20));
		useLocalTime = new QCheckBox(configDialog);
		useLocalTime->setObjectName(QString::fromUtf8("useLocalTime"));
		useLocalTime->setGeometry(QRect(260, 180, 125, 20));
		distKM = new QCheckBox(configDialog);
		distKM->setObjectName(QString::fromUtf8("distKM"));
		distKM->setGeometry(QRect(396, 180, 169, 20));
		createJPEG = new QCheckBox(configDialog);
		createJPEG->setObjectName(QString::fromUtf8("createJPEG"));
		createJPEG->setGeometry(QRect(20, 220, 159, 20));

		retranslateUi(configDialog);
		QObject::connect(okButton, SIGNAL(clicked()), configDialog, SLOT(accept()));
		QObject::connect(cancelButton, SIGNAL(clicked()), configDialog, SLOT(reject()));

		QMetaObject::connectSlotsByName(configDialog);
	} // setupUi

	void retranslateUi(QDialog *configDialog)
	{
		configDialog->setWindowTitle(QApplication::translate("configDialog", "BPQAPRS Basic Setup", nullptr));
		okButton->setText(QApplication::translate("configDialog", "OK", nullptr));
		cancelButton->setText(QApplication::translate("configDialog", "Cancel", nullptr));
		addCurrentView->setText(QApplication::translate("configDialog", "Add current view to filter string", nullptr));
		label->setText(QApplication::translate("configDialog", "Track Expire Time", nullptr));
		label_4->setText(QApplication::translate("configDialog", "IS Filter", nullptr));
		label_5->setText(QApplication::translate("configDialog", "Status Msg", nullptr));
		label_10->setText(QApplication::translate("configDialog", "Interval (Seconds)", nullptr));
		label_11->setText(QApplication::translate("configDialog", "JPEG File Name", nullptr));
		suppressNoPosn->setText(QApplication::translate("configDialog", "Supress Stations with zero Lat/Lon", nullptr));
		useLocalTime->setText(QApplication::translate("configDialog", "Use Local Time", nullptr));
		distKM->setText(QApplication::translate("configDialog", "Distance in Kilometers", nullptr));
		createJPEG->setText(QApplication::translate("configDialog", "Create JPEG of display", nullptr));
	} // retranslateUi

	Ui_configDialog(QWidget * parent);

};

