// Qt Version of BPQAPRS

#define VersionString "0.0.0.7"

#define _USE_32BIT_TIME_T


#include <QScroller>
#include <QScrollBar>
#include <QScrollerProperties>
#include <QResizeEvent>
#include <QMutex>
#include <QFileInfo>
#include <QTimer>
#include <QDir>
#include <QSettings>
#include <QPainter>
#include <QImageReader>
#include <QTextEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QKeyEvent>


#include "QtBPQAPRS.h"

//#include <QNetworkAccessManager>

#ifdef WIN32

#define _X86_
#include "winreg.h"
#include "winbase.h"

#else
#define _strdup strdup
#define __cdecl
#define TRUE 1
#define FALSE 0
#define VOID void
#define BOOL int
#endif


struct SharedMem
{
	unsigned char Version;				// For compatibility check
	unsigned char NeedRefresh;			// Messages Have Changed
	unsigned char ClearRX;
	unsigned char ClearTX;
	int SharedMemLen;					// So Client knows size to map

	struct APRSMESSAGE * Messages;
	struct APRSMESSAGE * OutstandingMsgs;
};


extern "C"
{
	int long2tilex(double lon, int z);
	int lat2tiley(double lat, int z);
	double long2x(double lon, int z);
	double lat2y(double lat, int z);
	double tilex2long(double x, int z);
	double tiley2lat(double y, int z);
	int  GetLocPixels(double Lat, double Lon, int * X, int * Y);
	int CentrePosition(double Lat, double Lon);
	int CentrePositionToMouse(double Lat, double Lon);
	void GetCornerLatLon(double * TLLat, double * TLLon, double * BRLat, double * BRLon);
	void OSMGet(int x, int y, int zoom);
	int GetSharedMemory();
	char * strlop(char * buf, char delim);
}

void RefreshStationList();
void RefreshMessages();
//void GetCentrePosition(double * Lat, double * Lon);
void SendFilterCommand(char * Filter);
double Distance(double laa, double loa);
double Bearing(double lat2, double lon2);
void DecodeWXReport(struct APRSConnectionInfo * sockptr, char * WX);


extern "C" struct SharedMem * SMEM;

extern "C" int ReloadMaps;

extern "C" int SetBaseX;				// Lowest Tiles in currently loaded set
extern "C" int SetBaseY;

extern "C" int Zoom;

extern "C" int MaxZoom;

extern "C" int cxWinSize, cyWinSize;
extern "C" int cxImgSize, cyImgSize;

extern "C" int ScrollX;
extern "C" int ScrollY;

extern "C" int MapCentreX;
extern "C" int MapCentreY;

extern "C" int MouseX, MouseY;
extern "C" int PopupX, PopupY;

extern "C" double Lat, Lon;
extern "C" char APRSCall[10];
extern "C" char LoppedAPRSCall[10];


extern "C" struct STATIONRECORD ** StationRecords;

QScrollBar  * VBar, * HBar;
QLabel * Status1;
QLabel * Status2;
QLabel * Status3;
QLabel * Status4;
QLabel * Status5;

//QImage * MapImage;
QPixmap * MapPixmap;

QLabel * ImageLabel;
QPainter * p;

QPixmap * TileMissing;
QPixmap * OutsideMap;

bool Missing[8][8];		// Tile missing flag

QScroller *scroller;

QNetworkAccessManager *manager;

int hMax, vMax, hPos, vPos;

char OSMDir[250] = "";
char APRSDir[250] = "";
char Symbols[250] = "";

int newTiles = 0;						// New Tiles have been downloaded

int WindowX = 100, WindowY = 100;			// Position of window on screen
int WindowWidth = 788;
int WindowHeight = 788;

int stnWinWidth = 300;
int stnWinHeight = 300;
int stnWinX = 200;
int stnWinY = 200;

int msgWinWidth = 300;
int msgWinHeight = 300;
int msgWinX = 100;
int msgWinY = 100;
int Split = 100;				// Rx/Tx Window split

int TileX = 0;
int TileY = 0;

char ISFilter[1000] = "m/50 u/APBPQ*";

int iconFontSize = 9;
int TrackExpireTime = 1440;
BOOL SuppressNullPosn = FALSE;
BOOL AddViewToFilter = FALSE;
BOOL LocalTime = TRUE;
BOOL KM = FALSE;

BOOL AutoFilterTimer = 0;

#define AUTOFILTERDELAY 30		// Time from scrolling or zooming till filter is sent

int StationCount = 0;

messageDialog * Dlg = nullptr;
QTableWidget *rxTable;
QTableWidget *txTable;

int MessagesOpen = 0;

StationDialog * StationDlg = nullptr;
StationDialog * SaveStationDlg;

QTableWidget *stationList;
QStringList m_TableHeader;

int StationsOpen = 0;


#define MYTILES

//char Host[] = "oatile1.mqcdn.com";		//SAT

#ifdef MYTILES

//char Host[] = "192.168.1.63:7381";
//char Host[] = "g8bpqnotts.ddns.net:7381";

char Host1[] = "server1.g8bpq.net:7381";
char Host2[] = "server2.g8bpq.net:7381";


//char Host1[] = "192.168.1.63:7381";
//char Host2[] = "192.168.1.63:8080";

//#define MapServerPort 8090

#else

// Thunderforest

char Host[] = "tile.thunderforest.com";
char mapStyle[64] = "outdoors"; //"neighbourhood mobile-atlas
#define MapServerPort 80

#endif




int nextHost = 0;				// Toggle betwen two map servers

int hostCounts[2] = { 0 };

int CreateJPEG = 1;
int JPEGinterval = 300;
int JPEGCounter = 0;
char JPEGFilename[250] = "BPQAPRS/HTML/APRSImage.jpg";

unsigned char * iconImage;

int SymbolLineLen = 0;


#define BOOL int
#define FALSE 0
#define TRUE 1

BOOL OnlyMine = 0;
BOOL AllSSID = 1;
BOOL OnlySeq = 0;
BOOL ShowBulls = 1;
BOOL BeepOnMsg = 0;

// Messaging To station list

QVector<QString> ToCalls(0);

#ifndef WIN32


#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <unistd.h>


char TX_SOCK_PATH[] = "BPQAPRStxsock";
char RX_SOCK_PATH[] = "BPQAPRSrxsock";

int sfd;
struct sockaddr_un my_addr, peer_addr;
socklen_t peer_addr_size;
int maxfd;

#endif


QColor Colours[256] = { qRgb(0,0,0), qRgb(0,0,255), qRgb(0,128,0), qRgb(0,128,192),
		qRgb(0,192,0), qRgb(0,192,255), qRgb(0,255,0), qRgb(128,0,128),
		qRgb(128,64,0), qRgb(128,128,128), qRgb(192,0,0), qRgb(192,0,255),
		qRgb(192,64,128), qRgb(192,128,255), qRgb(255,0,0), qRgb(255,0,255),				// 81
		qRgb(255,64,0), qRgb(255,64,128), qRgb(255,64,192), qRgb(255,128,0) };




#include <stdarg.h>
#include <stdio.h>
#include <math.h>

QSystemTrayIcon * trayIcon;

extern "C" void __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	qDebug() << Mess;
	return;
}


void QtBPQAPRS::closeEvent(QCloseEvent *)
{
	// if message or station windows are open close them.

	if (MessagesOpen)
		Dlg->close();

	if (StationsOpen)
		SaveStationDlg->close();

	SaveConfig();

}


QtBPQAPRS::~QtBPQAPRS()
{
}

QtBPQAPRS::QtBPQAPRS(QWidget *parent)
	: QMainWindow(parent)
{
	char Title[80];

	ui.setupUi(this);

	sprintf(Title, "QtBPQAPRS Version %s", VersionString);

	this->setWindowTitle(Title);

//	trayIcon = new QSystemTrayIcon(QIcon("../QtSoundModem/soundmodem.ico"), this);
//	trayIcon->setToolTip("QtBPQAPRS");
//	trayIcon->show();

	// Linux uses current directory, Windows BPQDirectory

#ifdef WIN32

	// Get from Registry

	HKEY hKey = 0;
	DWORD Type, disp, Vallen = MAX_PATH;
	int retCode;
	char msg[512];
	char ValfromReg[MAX_PATH] = "";
	char BPQDirectory[MAX_PATH] = "";


	retCode = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	BPQDirectory[0] = 0;

	retCode = RegOpenKeyEx(HKEY_CURRENT_USER,
		"SOFTWARE\\G8BPQ\\BPQ32",
		0,
		KEY_QUERY_VALUE,
		&hKey);

	if (retCode == 0)
	{
		// Try "BPQ Directory"

		Vallen = MAX_PATH;
		retCode = RegQueryValueEx(hKey, "BPQ Directory", 0,
			&Type, (UCHAR *)&ValfromReg, &Vallen);

		if (retCode == 0)
		{
			if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
				ValfromReg[0] = 0;
		}

		if (ValfromReg[0] == 0)
		{
			// BPQ Directory absent or = "" - try "Config File Location"

			Vallen = MAX_PATH;

			retCode = RegQueryValueEx(hKey, "Config File Location", 0,
				&Type, (UCHAR *)&ValfromReg, &Vallen);

			if (retCode == 0)
			{
				if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
					ValfromReg[0] = 0;
			}
		}

		ExpandEnvironmentStrings(ValfromReg, BPQDirectory, MAX_PATH);

	}

	strcpy(OSMDir, BPQDirectory);
	strcat(OSMDir, "/");
	strcpy(APRSDir, BPQDirectory);
	strcat(APRSDir, "/");
#endif
	strcat(OSMDir, "BPQAPRS/OSMTiles");

	sprintf(Symbols, "%sBPQAPRS/Symbols.jpg", APRSDir);

	char * ptr;

	while ((ptr = strchr(Symbols, '\\')))		// replace \ with /
		*ptr = '/';

	QImageReader reader(Symbols);

	QImage * SymbolBytesIndexed = new QImage();
	
	reader.read(SymbolBytesIndexed);

	if (SymbolBytesIndexed->byteCount() == 0)
	{
		// try png

		strlop(Symbols, '.');
		strcat(Symbols, ".png");
		QImageReader reader(Symbols);
		reader.read(SymbolBytesIndexed);
	}

	if (SymbolBytesIndexed->byteCount() == 0)
	{
		// no symbols

		iconImage = (unsigned char *)malloc(343740);
		memset(iconImage, qRgb(255,255,255), 343740);
	}
	else
	{
		QImage SymbolBytesRGB = SymbolBytesIndexed->convertToFormat(QImage::Format_RGB32);

		SymbolLineLen = SymbolBytesRGB.bytesPerLine();
		int ImageSize = SymbolBytesRGB.byteCount();

		iconImage = (unsigned char *)malloc(ImageSize);
		memcpy(iconImage, SymbolBytesRGB.bits(), ImageSize);
	}

	while ((ptr = strchr(OSMDir, '\\')))		// replace \ with /
		*ptr = '/';

	if (!GetSharedMemory())
		exit(1);

	// Clear Station Image from Station List 

	struct STATIONRECORD * stn = *StationRecords;

	while (stn)
	{
		stn->image = 0;	
		stn = stn->Next;
	}

	GetConfig();

	this->setGeometry(WindowX, WindowY, WindowWidth, WindowHeight);

	ui.menuBar->setVisible(true);

	HBar = ui.scrollArea->horizontalScrollBar();
	VBar = ui.scrollArea->verticalScrollBar();

	connect(HBar, SIGNAL(valueChanged(int)), this, SLOT(HBarChanged(int)));
	connect(VBar, SIGNAL(valueChanged(int)), this, SLOT(VBarChanged(int)));


	ui.menuBar->addAction("&Basic Setup", this, SLOT(Setup()));
	ui.menuBar->addAction("&Messages", this, SLOT(Messages()));
	ui.menuBar->addAction("&Stations", this, SLOT(Stations()));
	ui.menuBar->addAction("Zoom &In", this, SLOT(ZoomIn()));
	ui.menuBar->addAction("Zoom &Out", this, SLOT(ZoomOut()));

	QMenu * menuActions = new QMenu("&Actions", ui.menuBar);
	QMenu * menu_Help = new QMenu("&Help", ui.menuBar);
	ui.menuBar->addMenu(menuActions);
	ui.menuBar->addMenu(menu_Help);

	QAction * actionSend_Beacons = menuActions->addAction("Send Beacons");
	QAction * actionFilterView = menuActions->addAction("Add current Map view to IS Filter");
	QAction * actionHome = menuActions->addAction("Home");

	QAction * actionOnline_Help = menu_Help->addAction("Online Help");
	QAction * actionAbout = menu_Help->addAction("About");

	connect(actionSend_Beacons, SIGNAL(triggered()), this, SLOT(sendBeacon()));
	connect(actionFilterView, SIGNAL(triggered()), this, SLOT(FilterView()));
	connect(actionHome, SIGNAL(triggered()), this, SLOT(Home()));

	connect(actionOnline_Help, SIGNAL(triggered()), this, SLOT(OnlineHelp()));
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(About()));

	Status1 = new QLabel("0");
	Status2 = new QLabel(QString::number(Zoom));
	Status3 = new QLabel("0");
	Status4 = new QLabel("0");
	Status5 = new QLabel("<html><body>Mapping &copy; OpenStreetMap contributors.Tiles &copy; openmaptiles.org</html></body>");

	statusBar()->addPermanentWidget(Status4);
	statusBar()->addPermanentWidget(Status3);
	statusBar()->addPermanentWidget(Status2);
	statusBar()->addPermanentWidget(Status1);
	statusBar()->addPermanentWidget(Status5);

	statusBar()->setVisible(true);

//	Zoom = 7;
//	Lat = 58.47578;
//	Lon = -6.21163;
	 
//	MapImage = new QImage(2048, 2048, QImage::Format_RGB32);
//	MapImage->fill(QRgb(0x7f7f7f));

	MapPixmap = new QPixmap(2048, 2048);


	ImageLabel = new QLabel(ui.scrollAreaWidgetContents);
	ImageLabel->setGeometry(0, 0, 2048, 2048);
	ImageLabel->installEventFilter(this);

	TileMissing = new QPixmap(256, 256);
	TileMissing->fill(Qt::lightGray);
	QPainter p(TileMissing);

	p.drawText(100, 128, "Missing Tile");

	OutsideMap = new QPixmap(256, 256);
	OutsideMap->fill(Qt::lightGray);

	QPainter p2(OutsideMap);
	p2.drawText(100, 128, "Outside Map");

	ui.scrollAreaWidgetContents->setGeometry(0, 0, 2048, 2048);

	scroller = QScroller::scroller(ui.scrollArea);
	scroller->grabGesture(ui.scrollArea, QScroller::LeftMouseButtonGesture);

	QSize Size(800, 600);						// Not actually used, but Event constructor needs it

	QResizeEvent event(Size, Size);

	QApplication::sendEvent(this, &event);		// Resize Widgets to fix Window

	manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished,
		this, &QtBPQAPRS::replyFinished);

//	ui.scrollArea->installEventFilter(this);
	QtBPQAPRS::installEventFilter(this);

	ImageLabel->setMouseTracking(1);

	// Open UNIX socket to send messages to Node

#ifndef WIN32

// Open unix socket for messaging app

	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sfd == -1)
	{
		perror("Socket");
	}
	else
	{
		u_long param = 1;
		ioctl(sfd, FIONBIO, &param);			// Set non-blocking

		memset(&my_addr, 0, sizeof(struct sockaddr_un));
		my_addr.sun_family = AF_UNIX;
		strncpy(my_addr.sun_path, RX_SOCK_PATH, sizeof(my_addr.sun_path) - 1);

		memset(&peer_addr, 0, sizeof(struct sockaddr_un));
		peer_addr.sun_family = AF_UNIX;
		strncpy(peer_addr.sun_path, TX_SOCK_PATH, sizeof(peer_addr.sun_path) - 1);

		unlink(RX_SOCK_PATH);

		if (bind(sfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) == -1)
			perror("bind");
	}
#endif


	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimer()));
	timer->start(1000);

	// fiddle to get screen positioned at startup - Zoom in and out
	
	QTimer::singleShot(10, this, SLOT(MyTimer2()));
}


void QtBPQAPRS::MyTimer2()
{
	if (Zoom < 4)
	{
		ZoomIn();
		ZoomOut();
	}
	else
	{
		ZoomOut();
		ZoomIn();
	}
}
void QtBPQAPRS::MyTimer()
{
	OSMThread();

	if (newTiles || ReloadMaps)
	{
		ReloadTiles(newTiles);
		RefreshStationMap(1);
		newTiles = 0;
	}

	RefreshStationList();
	RefreshMessages();
	RefreshStationMap(0);					// Only Changed stations

	JPEGCounter++;

	if (CreateJPEG && JPEGFilename[0])
	{
		if (JPEGCounter > JPEGinterval)
		{
			if (cxWinSize > 0 && cyWinSize > 0)
			{
				QPixmap copy = MapPixmap->copy(ScrollX, ScrollY, cxWinSize, cyWinSize);

				QString fileName(JPEGFilename);
				QFile file(fileName);
				if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
					copy.save(&file);
				}
			}
			JPEGCounter = 0;
		}

		if (AutoFilterTimer)
		{
			AutoFilterTimer--;

			if (AutoFilterTimer == 0 && AddViewToFilter)
			{
				// send filter to IS

				double TLLat, TLLon, BRLat, BRLon;
				char Filter[256];

				GetCornerLatLon(&TLLat, &TLLon, &BRLat, &BRLon);
				sprintf(Filter, "%s a/%.3f/%.3f/%.3f/%.3f", ISFilter, TLLat, TLLon, BRLat, BRLon);

				SendFilterCommand(Filter);
			}
		}
	}

	Status3->setText(QString::number(StationCount));
}

// should be protected
bool QtBPQAPRS::eventFilter(QObject *, QEvent *event)
{
	if (event->type() == QEvent::Wheel)
	{
		QWheelEvent * wevent = (QWheelEvent *)event;
		QPoint numDegrees = wevent->angleDelta() / 8;

		// Get Mouse Posn

		GetMousePosn(&MouseX, &MouseY);

		if (numDegrees.y() > 0)
			ZoomIn();
		else if (numDegrees.y() < 0)
			ZoomOut();

		event->accept();
		return true;
	}

	if (event->type() == QEvent::MouseMove)
	{
		QMouseEvent * Ev = (QMouseEvent *)event;

		// See if over a station

		int x = Ev->x();
		int y = Ev->y();

		int wx = Ev->globalX();
		int wy = Ev->globalY();

		FindStationsByPixel(x, y, wx, wy);

		double MouseLat, MouseLon;

		GetMouseLatLon(&MouseLat, &MouseLon);

		char pos[64];

		sprintf(pos, "%f %f", MouseLat, MouseLon);
		Status4->setText(pos);

		event->accept();
		return true;
	}


	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent * e = (QKeyEvent *)event;

		int Key = e->key();

		if (Key == '-')
		{
			ZoomOut();
			event->accept();
			return true;
		}
		if (Key == '+' || Key == '=')
		{
			ZoomIn();
			event->accept();
			return true;
		}
	}

	// Other events should propagate
	return false;
	//return QMainWindow::eventFilter(obj, event);
}

#ifdef WIN32
extern "C" __declspec(dllimport) VOID APIENTRY APISendBeacon();
#endif

void QtBPQAPRS::sendBeacon()
{
#ifdef WIN32
	APISendBeacon();
#endif
}

void QtBPQAPRS::FilterView()
{
	// send filter to IS

	double TLLat, TLLon, BRLat, BRLon;
	char Filter[256];

	GetCornerLatLon(&TLLat, &TLLon, &BRLat, &BRLon);
	sprintf(Filter, "%s a/%.3f/%.3f/%.3f/%.3f", ISFilter, TLLat, TLLon, BRLat, BRLon);

	SendFilterCommand(Filter);
}

void QtBPQAPRS::Home()
{
	struct STATIONRECORD * Station = *StationRecords;

	while (Station)
	{
		if (strcmp(Station->Callsign, LoppedAPRSCall) == 0)
		{
			CentrePosition(Station->Lat, Station->Lon);
			GetCentrePosition(&Lat, &Lon);	// Updates posn on status line
			ReloadMap();
			return;
		}

		Station = Station->Next;
	}
}

void QtBPQAPRS::OnlineHelp()
{
	QUrl url("http://g8bpq.org.uk/QtBPQAPRS.html");

	QDesktopServices::openUrl(url);
}

void QtBPQAPRS::About()
{
	char about[512];
	
	sprintf(about, "G8BPQ APRS Client/I-Gate<br>Version %s<br>Copyright &copy; 2020 John Wiseman G8BPQ<br>"
		"APRS&reg; is a registered trademark of Bob Bruninga.<br>"
		"Mapping &copy; OpenStreetMap (http://openstreetmap.org/copyright)<br>"
		"Imagery &copy; OpenMapTiles (https://openmaptiles.org)<<br>", VersionString);

	QString msg(about);

	QMessageBox::about(this, "QtBPQAPRS", msg);
}

void QtBPQAPRS::ZoomIn()
{
	double MouseLat, MouseLon;
	
	if (Zoom < MaxZoom)
	{
		GetMouseLatLon(&MouseLat, &MouseLon);
		Zoom++;
		Status2->setText(QString::number(Zoom));
		CentrePositionToMouse(MouseLat, MouseLon);
		ReloadMap();
		GetCentrePosition(&Lat, &Lon);
	}
}

void QtBPQAPRS::ZoomOut()
{
	double MouseLat, MouseLon;

	if (Zoom > 2)
	{
		GetMouseLatLon(&MouseLat, &MouseLon);
		Zoom--;
		Status2->setText(QString::number(Zoom));
		CentrePositionToMouse(MouseLat, MouseLon);
		ReloadMap();
		GetCentrePosition(&Lat, &Lon);
	}
}

int CentrePositionToMouse(double Lat, double Lon)
{
	// Positions  specified location at the mouse

	int X, Y;

	SetBaseX = long2tilex(Lon, Zoom) - 4;
	SetBaseY = lat2tiley(Lat, Zoom) - 4;				// Set Location at middle

	if (GetLocPixels(Lat, Lon, &X, &Y) == FALSE)
		return FALSE;							// Off map

	ScrollX = X - cxWinSize / 2;
	ScrollY = Y - cyWinSize / 2;


	//	Map is now centered at loc cursor was at

	//  Need to move by distance mouse is from centre

		// if ScrollX, Y are zero, the centre of the map corresponds to 1024, 1024

	//	ScrollX -= 1024 - X;				// Posn to centre
	//	ScrollY -= 1024 - Y;

	ScrollX += cxWinSize / 2 - MouseX;
	ScrollY += cyWinSize / 2 - MouseY;

	if (ScrollX < 0 || ScrollY < 0)
	{
		// Need to move image

		while (ScrollX < 0)
		{
			SetBaseX--;
			ScrollX += 256;
		}

		while (ScrollY < 0)
		{
			SetBaseY--;
			ScrollY += 256;
		}

		ReloadMaps = TRUE;

	}

	AutoFilterTimer = AUTOFILTERDELAY;		// Update filter if no change for 30 secs

	return TRUE;
}

Ui_configDialog * Uix;

void QtBPQAPRS::Setup()
{
	Uix = new Ui_configDialog(this);

	Uix->setupUi(Uix);

	Uix->ISFIlter->setText(ISFilter);
	//		SetDlgItemText(hDlg, IDC_STATUS, APRSGetStatusMsgPtr());

	Uix->iconFontSize->setText(QString::number(iconFontSize));
	Uix->trackExpireTime->setText(QString::number(TrackExpireTime));
	Uix->createJPEG->setChecked(CreateJPEG);
	Uix->JPEGFileName->setText(JPEGFilename);
	Uix->JPEGInterval->setText(QString::number(JPEGinterval));
	Uix->suppressNoPosn->setChecked(SuppressNullPosn);
	Uix->addCurrentView->setChecked(AddViewToFilter);

	Uix->useLocalTime->setChecked(LocalTime);
	Uix->distKM->setChecked(KM);

	connect(Uix->okButton, SIGNAL(clicked()), this, SLOT(myaccept()));
	connect(Uix->cancelButton, SIGNAL(clicked()), this, SLOT(myreject()));

	Uix->show();
}
	



void QtBPQAPRS::myaccept()
{
	strcpy(ISFilter, Uix->ISFIlter->text().toUtf8());

	iconFontSize = Uix->iconFontSize->text().toInt();
	TrackExpireTime = Uix->trackExpireTime->text().toInt();
	CreateJPEG = Uix->createJPEG->isChecked();

	//		SetDlgItemText(hDlg, IDC_STATUS, APRSGetStatusMsgPtr());

	strcpy(JPEGFilename, Uix->JPEGFileName->text().toUtf8());
	JPEGinterval = Uix->JPEGInterval->text().toInt();

	SuppressNullPosn = Uix->suppressNoPosn->isChecked();
	AddViewToFilter = Uix->addCurrentView->isChecked();

	LocalTime = Uix->useLocalTime->isChecked();
	KM = Uix->distKM->isChecked();

	if (ISFilter[0])
		SendFilterCommand(ISFilter);

	SaveConfig();

}

void QtBPQAPRS::myreject()
{
}


QPushButton * clearRXButton;
QPushButton * clearTXButton;
QPushButton * Dummy;			// fiddle to stop focus going to clear buttons

void QtBPQAPRS::Messages()
{
	if (MessagesOpen)
	{
		Dlg->setWindowState(Qt::WindowState::WindowMinimized);
		Dlg->setWindowState(Qt::WindowState::WindowNoState); // Bring window to foreground
		return;
	}

	Dlg = new messageDialog();

	Dlg->groupBox = new QGroupBox(Dlg);
	Dlg->groupBox->setGeometry(QRect(0, 570, 821, 36));
	Dlg->inputMessage = new QLineEdit(Dlg->groupBox);
	Dlg->inputMessage->setGeometry(QRect(210, 5, 611, 22));
	Dlg->To = new QComboBox(Dlg->groupBox);
	Dlg->To->setGeometry(QRect(40, 5, 111, 22));

	Dlg->groupBox_2 = new QGroupBox(Dlg);
	Dlg->groupBox_2->setGeometry(QRect(0, 0, 831, 33));
	Dlg->groupBox_2->setLayoutDirection(Qt::RightToLeft);
	Dummy = new QPushButton(Dlg->groupBox_2);
	Dummy->setVisible(0);
	clearTXButton = new QPushButton(Dlg->groupBox_2);
	clearTXButton->setGeometry(QRect(755, 8, 70, 22));
	clearRXButton = new QPushButton(Dlg->groupBox_2);
	clearRXButton->setGeometry(QRect(680, 8, 70, 22));
	Dlg->checkBox = new QCheckBox(Dlg->groupBox_2);
	Dlg->checkBox->setGeometry(QRect(5, 8, 121, 22));
	Dlg->checkBox->setLayoutDirection(Qt::RightToLeft);
	Dlg->checkBox_2 = new QCheckBox(Dlg->groupBox_2);
	Dlg->checkBox_2->setGeometry(QRect(140, 8, 91, 22));
	Dlg->checkBox_2->setLayoutDirection(Qt::RightToLeft);
	Dlg->checkBox_3 = new QCheckBox(Dlg->groupBox_2);
	Dlg->checkBox_3->setGeometry(QRect(245, 8, 116, 22));
	Dlg->checkBox_3->setLayoutDirection(Qt::RightToLeft);
	Dlg->checkBox_4 = new QCheckBox(Dlg->groupBox_2);
	Dlg->checkBox_4->setGeometry(QRect(370, 8, 91, 22));
	Dlg->checkBox_4->setLayoutDirection(Qt::RightToLeft);
	Dlg->checkBox_5 = new QCheckBox(Dlg->groupBox_2);
	Dlg->checkBox_5->setGeometry(QRect(470, 8, 171, 22));
	Dlg->checkBox_5->setLayoutDirection(Qt::RightToLeft);

	Dlg->setGeometry(msgWinX, msgWinY, msgWinWidth, msgWinHeight);
	Dlg->rxscrollArea = new QScrollArea(Dlg);
	Dlg->rxscrollArea->setGeometry(QRect(0, 30, 826, 286));
	Dlg->rxscrollArea->setWidgetResizable(true);
	Dlg->rxContents = new QWidget();
	Dlg->rxContents->setGeometry(QRect(0, 0, 824, 284));
	rxTable = new QTableWidget(Dlg->rxContents);
	rxTable->setGeometry(QRect(0, 0, 831, 286));
	Dlg->rxscrollArea->setWidget(Dlg->rxContents);
	Dlg->txscrollArea = new QScrollArea(Dlg);
	Dlg->txscrollArea->setGeometry(QRect(0, 320, 826, 246));
	Dlg->txscrollArea->setWidgetResizable(true);
	Dlg->txContents = new QWidget();
	Dlg->txContents->setGeometry(QRect(0, 0, 824, 244));
	txTable = new QTableWidget(Dlg->txContents);
	txTable->setGeometry(QRect(0, 0, 831, 246));
	Dlg->txscrollArea->setWidget(Dlg->txContents);
	Dlg->label_2 = new QLabel(Dlg->groupBox);
	Dlg->label_2->setGeometry(QRect(175, 5, 36, 22));
	Dlg->label = new QLabel(Dlg->groupBox);
	Dlg->label->setGeometry(QRect(10, 5, 31, 22));
	connect(Dlg->checkBox, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
	connect(Dlg->checkBox_2, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
	connect(Dlg->checkBox_3, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
	connect(Dlg->checkBox_4, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
	connect(Dlg->checkBox_5, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
	connect(Dlg->inputMessage, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

	connect(rxTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(cellDoubleClicked(int, int)));

	rxTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	rxTable->verticalHeader()->setDefaultSectionSize(20);
	txTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	txTable->verticalHeader()->setDefaultSectionSize(20);


	rxTable->setColumnCount(5);
	rxTable->setRowCount(100);
	rxTable->horizontalHeader()->setStretchLastSection(true);
	rxTable->verticalHeader()->setVisible(false);
	rxTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	rxTable->setSelectionMode(QAbstractItemView::NoSelection);

	txTable->setColumnCount(4);
	txTable->setRowCount(100);
	txTable->horizontalHeader()->setStretchLastSection(true);
	txTable->verticalHeader()->setVisible(false);

	txTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	txTable->setSelectionMode(QAbstractItemView::NoSelection);

	QStringList Labs, Labt;

	Labs << "From" << "To" << "Seq" << "Time" << "Received";
	rxTable->setHorizontalHeaderLabels(Labs);

	Labt << "To" << "Seq" << "State" << "Sent";
	txTable->setHorizontalHeaderLabels(Labt);

	Dlg->setWindowTitle("APRS Messages");
	Dlg->groupBox->setTitle(QString());
	Dlg->label_2->setText("Text");
	Dlg->label->setText("To");
	Dlg->groupBox_2->setTitle(QString());
	Dlg->checkBox->setText("Show only Mine");
	Dlg->checkBox_2->setText("All SSID's:");
	Dlg->checkBox_3->setText("Only Sequenced");
	Dlg->checkBox_4->setText("Show Bulls");
	Dlg->checkBox_5->setText("Beep instead of Popoup:");
	clearRXButton->setText("Clear RX");
	clearTXButton->setText("Clear TX");

	connect(clearRXButton, SIGNAL(pressed()), this, SLOT(clearRX()));
	connect(clearTXButton, SIGNAL(pressed()), this, SLOT(clearTX()));

	Dlg->To->setEditable(1);

	Dlg->checkBox->setChecked(OnlyMine);
	Dlg->checkBox_2->setChecked(AllSSID);
	Dlg->checkBox_3->setChecked(OnlySeq);
	Dlg->checkBox_4->setChecked(ShowBulls);
	Dlg->checkBox_5->setChecked(BeepOnMsg);

	int i;

	for (i = 0; i < ToCalls.size(); ++i)
		Dlg->To->addItem(ToCalls[i]);

	Dlg->show();
	MessagesOpen = 1;

	SMEM->NeedRefresh = 1;
}


void QtBPQAPRS::clearTX()
{
	if (clearTXButton->isDown())
		SMEM->ClearTX = 1;
}

void QtBPQAPRS::clearRX()
{
	if (clearRXButton->isDown())			// Can be activated by sending a message if it has focus
		SMEM->ClearRX = 1;
}


void QtBPQAPRS::cellDoubleClicked(int row, int col)
{
	if (col == 0)
	{
		QTableWidgetItem * item = rxTable->item(row, col);

		if (item)
		{
			QString Call = item->data(Qt::DisplayRole).toString();

			if (!ToCalls.contains(Call))
				ToCalls.append(Call);

			if (Dlg)
			{
				int Index = Dlg->To->findText(Call);

				if (Index == -1)
					Dlg->To->addItem(Call);
			}
		}
	}
}

void QtBPQAPRS::stateChanged(int)
{
	OnlyMine = Dlg->checkBox->isChecked();
	AllSSID = Dlg->checkBox_2->isChecked();
	OnlySeq = Dlg->checkBox_3->isChecked();
	ShowBulls = Dlg->checkBox_4->isChecked();
	BeepOnMsg = Dlg->checkBox_5->isChecked();

	SMEM->NeedRefresh = 1;			// refresh messages
}

#ifdef WIN32
extern "C" __declspec(dllimport) int APIENTRY APISendAPRSMessage(char * Text, char * ToCall);
#else


void PutAPRSMessage(char * Frame, int Len)
{
	if (sendto(sfd, Frame, Len, 0, (struct sockaddr *) &peer_addr, sizeof(struct sockaddr_un)) != Len)
		perror("sendto");
}

void APISendAPRSMessage(const char * Text, char * ToCall)
{
	char Msg[255];

	strcpy(Msg, ToCall);
	strcpy(&Msg[10], Text);

	PutAPRSMessage(Msg, strlen(&Msg[10]) + 11);

	return;
}
#endif

void QtBPQAPRS::returnPressed()
{
	QString To = Dlg->To->currentText();
	To = To.toUpper();

	if (To.length() == 0)
	{
		QMessageBox msgBox;
		msgBox.setText("To Call Missing");
		msgBox.exec();
		return;
	}

	QString Msg = Dlg->inputMessage->text();

	if (Msg.length() == 0)
	{
		QMessageBox msgBox;
		msgBox.setText("Message Text Missing");
		msgBox.exec();
		return;
	}

	if (!ToCalls.contains(To))
		ToCalls.append(To);

	// Call must be null terminated

	char Call[16] = "";
	memcpy(Call, To.toLocal8Bit().data(), 9);

	APISendAPRSMessage(Msg.toLocal8Bit().data(), Call);

	Dlg->inputMessage->setText("");
}


void QtBPQAPRS::Stations()
{
	if (StationsOpen)
	{
		SaveStationDlg->setWindowState(Qt::WindowState::WindowMinimized); 
		SaveStationDlg->setWindowState(Qt::WindowState::WindowNoState); // Bring window to foreground
		return;
	}
	StationDialog * StationDlg = new StationDialog();

	SaveStationDlg = StationDlg;
	StationDlg->setWindowTitle("APRS Stations");

	StationDlg->setGeometry(stnWinX, stnWinY, stnWinWidth,stnWinHeight);

	StationDlg->scrollArea = new QScrollArea(StationDlg);
	StationDlg->scrollArea->setGeometry(QRect(2, 2, 776, 606));
	StationDlg->scrollArea->setWidgetResizable(true);
	StationDlg->scrollAreaWidgetContents = new QWidget();
	StationDlg->scrollAreaWidgetContents->setGeometry(QRect(0, 0, 774, 604));
	stationList = new QTableWidget(StationDlg->scrollAreaWidgetContents);
	stationList->setGeometry(QRect(2, 2, 766, 586));
	stationList->setColumnCount(7);
	stationList->setRowCount(100);

	stationList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	stationList->verticalHeader()->setDefaultSectionSize(20);

	stationList->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
	stationList->horizontalHeader()->setStretchLastSection(true);
	stationList->verticalHeader()->setVisible(false);
	m_TableHeader << "Callsign" << "Lat" << "Lon" << "Range" << "Bearing" << "Last Heard" << "Last Message";

	stationList->setHorizontalHeaderLabels(m_TableHeader);
	stationList->setColumnWidth(0, 80);
	stationList->setColumnWidth(1, 80);
	stationList->setColumnWidth(2, 80);
	stationList->setColumnWidth(3, 60);
	stationList->setColumnWidth(4, 60);
	stationList->setColumnWidth(5, 100);

	stationList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	stationList->setSelectionMode(QAbstractItemView::NoSelection);

	StationDlg->scrollArea->setWidget(StationDlg->scrollAreaWidgetContents);

	StationDlg->show();
	StationsOpen = 1;

	connect(stationList, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(callClicked(int, int)));

}

void QtBPQAPRS::callClicked(int row, int column)
{
	if (column != 0)
		return; 

	QString  Call = stationList->item(row, 0)->text();
	QByteArray BA = Call.toUtf8();

	
	// Center map on call

	struct STATIONRECORD * ptr = *StationRecords;

	int i = 0;


	while (ptr)
	{
		if (strcmp(ptr->Callsign, BA.data()) == 0)
		{
			Lat = ptr->Lat;
			Lon = ptr->Lon;
			CentrePosition(Lat, Lon);		// This sets posn at centre of 2048 * 2048 image
			GetCentrePosition(&Lat, &Lon);	// Updates posn on status line
			ReloadMap();
			return;
		}
		ptr = ptr->Next;
		i++;
	}
}

StationDialog::StationDialog(QWidget *parent) : QDialog(parent)
{
}

void StationDialog::resizeEvent(QResizeEvent* e)
{
	QSize s = e->size();

	SaveStationDlg->scrollArea->setGeometry(QRect(2, 2, s.width() - 4, s.height() - 4));
	SaveStationDlg->scrollAreaWidgetContents->setGeometry(QRect(0, 0, s.width() - 4, s.height() - 4));

	stationList->setGeometry(QRect(2, 2, s.width() - 10, s.height() - 10));
}

StationDialog::~StationDialog()
{
}

messageDialog::messageDialog(QWidget *parent) : QDialog(parent)
{
}

popupDialog * hPopupWnd = nullptr;
popupDialog * hSelWnd = nullptr;

struct APRSConnectionInfo			// Used for Web Server for thread-specific stuff
{
	struct STATIONRECORD * SelCall;	// Station Record for individual staton display
	char Callsign[12];
	int WindDirn, WindSpeed, WindGust, Temp, Humidity, Pressure; //WX Fields
	double  RainLastHour, RainLastDay, RainToday;
};

popupDialog::popupDialog()
{

}

void popupDialog::callClicked()
{
}


QTableWidget  * StnList = nullptr;


void popupDialog::resizeEvent(QResizeEvent* e)
{
	QSize s = e->size();

	if (StnList)
		StnList->setGeometry(0, 0, s.width(), s.height());
}

Ui_configDialog::Ui_configDialog(QWidget *parent) : QDialog(parent)
{
}



void StationDialog::closeEvent(QCloseEvent *)
{
	if (SaveStationDlg)
	{
		QRect rect = SaveStationDlg->geometry();

		stnWinX = rect.left();
		stnWinY = rect.top();
		stnWinWidth = rect.width();
		stnWinHeight = rect.height();

		StationsOpen = 0;
	}
}

void messageDialog::resizeEvent(QResizeEvent* e)
{
		QSize s = e->size();

		int split = s.height() / 2;

		Dlg->rxscrollArea->setGeometry(QRect(0, 34, s.width(), split - 34));
		Dlg->rxContents->setGeometry(QRect(0, 0, s.width(), split - 34));
		rxTable->setGeometry(QRect(0, 0, s.width(), split - 34));

		Dlg->txscrollArea->setGeometry(QRect(0, split, s.width(), split - 30));
		Dlg->txContents->setGeometry(QRect(0, 0, s.width(), split - 30));
		txTable->setGeometry(QRect(0, 0, s.width(), split - 30));

		Dlg->groupBox->setGeometry(0, s.height() - 30, s.width(), 28);
}

void messageDialog::closeEvent(QCloseEvent *)
{
	QRect rect = Dlg->geometry();
	
	msgWinX = rect.left();
	msgWinY = rect.top();
	msgWinWidth = rect.width();
	msgWinHeight = rect.height();

	MessagesOpen = 0;
}

bool Updating = false;
bool HUpdating = false;

int centreout[8] = { 7,0,6,1,5,2,4,3 };

/*

void QtBPQAPRS::ReloadTiles(bool newTiles)
{
	// Used when scrolling - we don't want to recalulate scroll, just load with current params
	// Scroll code has adjusted Base and Scroll

	int i, j, x, y;
	char TileName[128];
	bool TileLoaded = false;

	QImage * Tile = new QImage(256, 256, QImage::Format_RGB32);

	Updating = true;
	HUpdating = true;

	HBar->setValue(ScrollX);
	VBar->setValue(ScrollY);

	Updating = false;
	HUpdating = false;

	vMax = VBar->maximum();
	hMax = HBar->maximum();
	hPos = HBar->value();
	vPos = VBar->value();

	int Limit = (int)pow(2, Zoom);

	QPainter painter(MapImage);
//	MapImage->fill(QRgb(0x7f7f7f));

//	painter.drawText(1024, 1024, "Hello Hello Hello");

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			// Load tiles from the centre out

			i = centreout[x];
			j = centreout[y];

			// if newTiles is set we have been called after a new tile has been loaded
			// see if we need it

			if (newTiles)
				if (Missing[i][j] == false)			// already have it
					continue;

			if ((i + SetBaseX) >= Limit || (j + SetBaseY) >= Limit || i + SetBaseX < 0 || j + SetBaseY < 0)
			{
				painter.drawPixmap(i * 256, j * 256, *OutsideMap);
				continue;
			}

			sprintf(TileName, "%s/%02d/%d/%d.png", OSMDir, Zoom, i + SetBaseX, j + SetBaseY);
			if (Tile->load(TileName))
			{
				painter.drawImage(i * 256, j * 256, *Tile);
				TileLoaded = true;
				Missing[i][j] = false;
//				delete Tile;
			}
			else
			{
				sprintf(TileName, "%s/%02d/%d/%d.jpg", OSMDir, Zoom, i + SetBaseX, j + SetBaseY);
				if (Tile->load(TileName))
				{
					painter.drawImage(i * 256, j * 256, *Tile);
					TileLoaded = true;
					Missing[i][j] = false;
//					delete Tile;
				}
				else
				{
					if (Missing[i][j] == false || newTiles == false)
					{
						painter.drawPixmap(i * 256, j * 256, *TileMissing);
						OSMGet(i + SetBaseX, j + SetBaseY, Zoom);
						TileLoaded = true;
						Missing[i][j] = true;		// indicate missing
					}
				}
			}
		}
	}

	if (TileLoaded)
		RefreshStationMap(&painter);
//		ImageLabel->setPixmap(QPixmap::fromImage(*MapImage));

	delete Tile;
						
}
*/
void QtBPQAPRS::ReloadTiles(bool newTiles)
{
	// Used when scrolling - we don't want to recalulate scroll, just load with current params
	// Scroll code has adjusted Base and Scroll

	int i, j, x, y;
	char TileName[128];
//	bool TileLoaded = false;

	QPixmap * Tile = new QPixmap(256, 256);

	Updating = true;
	HUpdating = true;

	HBar->setValue(ScrollX);
	VBar->setValue(ScrollY);

	Updating = false;
	HUpdating = false;

	vMax = VBar->maximum();
	hMax = HBar->maximum();
	hPos = HBar->value();
	vPos = VBar->value();

	int Limit = (int)pow(2, Zoom);

	QPainter painter(MapPixmap);

	//	MapImage->fill(QRgb(0x7f7f7f));

	//	painter.drawText(1024, 1024, "Hello Hello Hello");

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			// Load tiles from the centre out

			i = centreout[x];
			j = centreout[y];

			// if newTiles is set we have been called after a new tile has been loaded
			// see if we need it

			if (newTiles)
				if (Missing[i][j] == false)			// already have it
					continue;

			if ((i + SetBaseX) >= Limit || (j + SetBaseY) >= Limit || i + SetBaseX < 0 || j + SetBaseY < 0)
			{
				painter.drawPixmap(i * 256, j * 256, *OutsideMap);
				continue;
			}

			sprintf(TileName, "%s/%02d/%d/%d.png", OSMDir, Zoom, i + SetBaseX, j + SetBaseY);
			if (Tile->load(TileName))
			{
				painter.drawPixmap(i * 256, j * 256, *Tile);
//				TileLoaded = true;
				Missing[i][j] = false;
				//				delete Tile;
			}
			else
			{
				sprintf(TileName, "%s/%02d/%d/%d.jpg", OSMDir, Zoom, i + SetBaseX, j + SetBaseY);
				if (Tile->load(TileName))
				{
					painter.drawPixmap(i * 256, j * 256, *Tile);
//					TileLoaded = true;
					Missing[i][j] = false;
					//					delete Tile;
				}
				else
				{
					if (Missing[i][j] == false || newTiles == false)
					{
						painter.drawPixmap(i * 256, j * 256, *TileMissing);
						OSMGet(i + SetBaseX, j + SetBaseY, Zoom);
//						TileLoaded = true;
						Missing[i][j] = true;		// indicate missing
					}
				}
			}
		}
	}

	delete Tile;
}
void QtBPQAPRS::ReloadMap()
{
	int X, Y;

	GetLocPixels(Lat, Lon, &X, &Y); // Pixel offset of posn within image
	ReloadTiles();
	RefreshStationMap(1);
}

void QtBPQAPRS::resizeEvent(QResizeEvent *)
{
	QRect rect = QtBPQAPRS::geometry();

	cyWinSize = rect.height();
	cxWinSize = rect.width();

	ui.scrollArea->setGeometry(QRect(2, 2, cxWinSize - 2, cyWinSize - 60));

	vMax = VBar->maximum();
	hMax = HBar->maximum();
	hPos = HBar->value();
	vPos = VBar->value();

	CentrePosition(Lat, Lon);		// This sets posn at centre of 2048 * 2048 image
	ReloadMap();
	GetCentrePosition(&Lat, &Lon);
}

void QtBPQAPRS::HBarChanged(int val)
{
	ScrollX = HBar->value();
	hMax = HBar->maximum();

	if (HUpdating)
		return;				// block re-entry

	HUpdating = true;

	if (val > (hMax - 64))
	{
		scroller->stop();
		ScrollX -= 256;
		SetBaseX++;
		ReloadTiles();
		RefreshStationMap(1);

	}
	else if (val < 64)
	{
		scroller->stop();
		ScrollX += 256;
		SetBaseX--;
		ReloadTiles();
	}

	HUpdating = false;
	RefreshStationMap(1);

	AutoFilterTimer = AUTOFILTERDELAY;		// Update filter if no change for 30 secs

	//	ScrollX = HBar->value();

}

void QtBPQAPRS::VBarChanged(int val)
{
	if (Updating)
		return;
	
	ScrollY = VBar->value();
	vMax = VBar->maximum();

	if (val < 64)			// 1/4 tile left to scroll so reload image
	{
		scroller->stop();
		Updating = true;
		ScrollY += 256;
		SetBaseY--;
		ReloadTiles();
		Updating = false;
	}
	if (val > (vMax - 64))
	{
		scroller->stop();
		Updating = true;
		ScrollY -= 256;
		SetBaseY++;
		ReloadTiles();
		Updating = false;
	}

	GetCentrePosition(&Lat, &Lon);

	AutoFilterTimer = AUTOFILTERDELAY;		// Update filter if no change for 30 secs


}

// Semaphore Code

QMutex mutex;

extern "C" void getMutex()
{
	mutex.lock();
}

extern "C" void releaseMutex()
{
	mutex.unlock();
}

int  gettingTile = 0;


int QtBPQAPRS::GetTile(int Zoom, int x, int y, int Server, int Count)
{
	char URL[256];

	gettingTile++;

	if (Server == 0)
		sprintf(URL, "http://%s/styles/klokantech-basic/%d/%d/%d.png", Host1, Zoom, x, y);
	else
		sprintf(URL, "http://%s/styles/klokantech-basic/%d/%d/%d.png", Host2, Zoom, x, y);

//	if (Server == 0)
//		sprintf(URL, "http://%s/styles/basic-preview/%d/%d/%d.png", Host1, Zoom, x, y);
//	else
//		sprintf(URL, "http://%s/styles/basic-preview/%d/%d/%d.png", Host2, Zoom, x, y);

	QNetworkReply * reply = manager->get(QNetworkRequest(QUrl(URL)));

	reply->setProperty("Server", Server);
	reply->setProperty("Count", Count);
	reply->setProperty("Zoom", Zoom);
	reply->setProperty("x", x);
	reply->setProperty("y", y);

	connect(reply, &QNetworkReply::downloadProgress, this, &QtBPQAPRS::downloadProgress);
	connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
		[=](QNetworkReply::NetworkError)
{

	int Server = reply->property("Server").toInt();
	int Count = reply->property("Count").toInt();
	int Zoom = reply->property("Zoom").toInt();
	int x = reply->property("x").toInt();
	int y = reply->property("y").toInt();

	// Resend request to other server

	QUrl url = reply->url();
	QString string = url.toString();
	QByteArray ba = string.toUtf8();

	qDebug() << string << reply->errorString();

//	gettingTile--;

	if (Count < 2)
	{

		Debugprintf("Getting %d %d %d from Server %d No %d", Zoom, x, y, Server ^ 1, gettingTile);

		GetTile(Zoom, x, y, Server ^ 1, ++Count);
	}
	else
		Debugprintf("Getting %d %d %d Failed", Zoom, x, y);

});

	return 0;

}



void QtBPQAPRS::downloadProgress(qint64, qint64)
{
}

char FN[256];


void QtBPQAPRS::replyFinished(QNetworkReply *reply)
{
	int len = reply->bytesAvailable();
	QByteArray Data = reply->readAll();
	QByteArray TempName;
	QUrl url = reply->url();
	QString string = url.toString();
	QByteArray ba =   string.toUtf8();
	char * ptr = ba.data();
	char * data = Data.data();

	int Server = reply->property("Server").toInt();

	hostCounts[Server]++;

	if (len < 10  || data[1] != 'P')
	{
		gettingTile--;
		OSMThread(Server);
		return;
	}

	char * ptr2 = strstr(ptr, ".png");
	
	while (*ptr2 != '/')
		ptr2--;

	ptr2--;
	while (*ptr2 != '/')
		ptr2--;
	
	ptr2--;
	while(*ptr2 != '/')
		ptr2--;

	ptr2++;


	if (ptr2[1] == '/')
		sprintf(FN, "%s/0%s", OSMDir, ptr2);
	else
		sprintf(FN, "%s/%s", OSMDir, ptr2);


	QFile newf(FN);

	if (!newf.open(QIODevice::ReadWrite))
	{
		// Open failed. Probably need to create directory

		QDir dir;
		char * Dir = _strdup(FN);
		int Len = strlen(Dir);

		while (Len && Dir[Len] != '/')
			Len--;

		Dir[Len] = 0;

		dir.mkpath(Dir);

		free(Dir);

		if (!newf.open(QIODevice::ReadWrite))
		{
			qDebug() << "Write failed " << FN;
			gettingTile--;
			OSMThread(Server);
			return;
		}
	}

	qDebug() << "Writing file " << FN << "Count" << gettingTile << "Server" << Server;

	newf.write(Data.data(), Data.length());
	newf.close();

	newTiles = 1;
	gettingTile--;

	OSMThread(Server);
}

struct OSMQUEUE
{
	struct OSMQUEUE * Next;
	int	Zoom;
	int x;
	int y;
};

extern "C" struct OSMQUEUE OSMQueue;

extern "C" int OSMQueueCount;
 
char HeaderTemplate[] = "Accept: */*\r\nHost: %s\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";
//char Header[] = "Accept: */*\r\nHost: tile.openstreetmap.org\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";

int totalLoaded = 0;

void QtBPQAPRS::OSMThread(int Server)
{
	// Request a page from OSM

	struct OSMQUEUE * OSMRec;
	int Zoom, x, y;

	while (OSMQueue.Next)
	{
		if (gettingTile > 5)
			return;

		if (gettingTile < 0)
			return;

		
		getMutex();

		OSMRec = OSMQueue.Next;
		OSMQueue.Next = OSMRec->Next;

		OSMQueueCount--;

		Status1->setText(QString::number(OSMQueueCount));

		releaseMutex();

		x = OSMRec->x;
		y = OSMRec->y;
		Zoom = OSMRec->Zoom;

		if (Zoom == 0)			// cancelled request
			continue;

		free(OSMRec);

		sprintf(FN, "%s/%02d/%d/%d.png", OSMDir, Zoom, x, y);

		QFileInfo check_file(FN);

		if (check_file.exists() && check_file.isFile())
		{
			Debugprintf(" File %s Exists - skipping", FN);
			continue;
		}

		if (Server == -1)
		{
			Debugprintf("Getting %s from Server %d No %d", FN, nextHost, gettingTile);
			GetTile(Zoom, x, y, nextHost, 0);
			nextHost ^= 1;
		}
		else
		{
			Debugprintf("Getting %s from Server %d No %d", FN, Server, gettingTile);
			GetTile(Zoom, x, y, Server, 0);
		}
	}
}


// Station Display Code

#define BOOL int
#define FALSE 0
#define TRUE 1

#define M_PI       3.14159265358979323846


time_t LastRefresh;


extern "C" char *  GetAPRSLatLon(double * PLat, double * PLon);

void QtBPQAPRS::GetMousePosn(int * MouseX, int * MouseY)
{
	QPoint globalCursorPos = QCursor::pos();
	QRect r = geometry();

	*MouseX = globalCursorPos.x() - r.x() - 6;
	*MouseY = globalCursorPos.y() - r.y() - 40;
}


void QtBPQAPRS::GetMouseLatLon(double * Lat, double * Lon)
{
	QPoint globalCursorPos = QCursor::pos();

	QRect r = geometry();

	MouseX = globalCursorPos.x() - r.x() - 6;
	MouseY = globalCursorPos.y() - r.y() - 40;

	int X = ScrollX + MouseX;
	int Y = ScrollY + MouseY;

	*Lat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*Lon = tilex2long(SetBaseX + (X / 256.0), Zoom);
}


void GetCornerLatLon(double * TLLat, double * TLLon, double * BRLat, double * BRLon)
{
	int X = ScrollX;
	int Y = ScrollY;

	*TLLat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*TLLon = tilex2long(SetBaseX + (X / 256.0), Zoom);

	X = ScrollX + cxWinSize;
	Y = ScrollY + cyWinSize;

	*BRLat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*BRLon = tilex2long(SetBaseX + (X / 256.0), Zoom);
}





double radians(double Degrees)
{
	return M_PI * Degrees / 180;
}
double degrees(double Radians)
{
	return Radians * 180 / M_PI;
}


double Distance(double laa, double loa)
{
	double lah, loh, dist;

	GetAPRSLatLon(&lah, &loh);

	/*

	'Great Circle Calculations.

	'dif = longitute home - longitute away


	'      (this should be within -180 to +180 degrees)
	'      (Hint: This number should be non-zero, programs should check for
	'             this and make dif=0.0001 as a minimum)
	'lah = latitude of home
	'laa = latitude of away

	'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
	'distance = dis / 180 * pi * ERAD
	'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

	'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians
	*/

	loh = radians(loh); lah = radians(lah);
	loa = radians(loa); laa = radians(laa);

	dist = 60 * degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa - loh))) * 1.15077945;

	if (KM)
		dist *= 1.60934;

	return dist;
}

double Bearing(double lat2, double lon2)
{
	double lat1, lon1;
	double dlat, dlon, TC1;

	GetAPRSLatLon(&lat1, &lon1);

	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;

	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);

	if (fabs(TC1) > 89.5)
	{
		if (dlon > 0)
			return 90;
		else
			return 270;
	}

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}
	return 0;
}


void QtBPQAPRS::GetCentrePosition(double * Lat, double * Lon)
{
	// Returns Lat/Lon of centre of Window

	char pos[32];

	int X = ScrollX + cxWinSize / 2;
	int Y = ScrollY + cyWinSize / 2;

	*Lat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*Lon = tilex2long(SetBaseX + (X / 256.0), Zoom);

	double MouseLat, MouseLon;

	GetMouseLatLon(&MouseLat, &MouseLon);

	sprintf(pos, "%f %f", MouseLat, MouseLon);
	Status4->setText(pos);

}

int row = 0;

void RefreshStation(struct STATIONRECORD * ptr)
{
	int n;
	double Lat = ptr->Lat;
	double Lon = ptr->Lon;
	char NS = 'N', EW = 'E';
	char LatString[20], LongString[20], DistString[20], BearingString[20];
	int Degrees;
	double Minutes;
	char Time[80];
	struct tm * TM;

	if (StationsOpen == 0)
		return;

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return;

	if (ptr->ObjState == '_')	// Killed Object
		return;

	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

#ifdef _DEBUG	
	sprintf(Time, "%.2d:%.2d:%.2d TP %d",
		TM->tm_hour, TM->tm_min, TM->tm_sec, ptr->Trackptr);
#else
	sprintf(Time, "%.2d:%.2d:%.2d",
		TM->tm_hour, TM->tm_min, TM->tm_sec);
#endif

	if (Lat < 0)
	{
		NS = 'S';
		Lat = -Lat;
	}
	if (Lon < 0)
	{
		EW = 'W';
		Lon = -Lon;
	}

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif

	Degrees = Lat;
	Minutes = Lat * 60.0 - (60 * Degrees);

	sprintf(LatString, "%2d %05.2f'%c", Degrees, Minutes, NS);

	Degrees = Lon;

#ifdef WIN32
#pragma warning(pop)
#endif

	Minutes = Lon * 60 - 60 * Degrees;

	n = sprintf(LongString, "%3d %05.2f'%c", Degrees, Minutes, EW);

	sprintf(DistString, "%6.1f", Distance(ptr->Lat, ptr->Lon));
	sprintf(BearingString, "%3.0f", Bearing(ptr->Lat, ptr->Lon));

	if (n > 12)
		n++;

	stationList->setItem(row, 0, new QTableWidgetItem((char *)ptr->Callsign));
	stationList->setItem(row, 1, new QTableWidgetItem((char *)LatString));
	stationList->setItem(row, 2, new QTableWidgetItem((char *)LongString));
	stationList->setItem(row, 3, new QTableWidgetItem((char *)DistString));
	stationList->setItem(row, 4, new QTableWidgetItem((char *)BearingString));
	stationList->setItem(row, 5, new QTableWidgetItem((char *)Time));
	stationList->setItem(row, 6, new QTableWidgetItem((char *)ptr->LastPacket));

	row++;
}


void RefreshStationList()
{
	struct STATIONRECORD * ptr = *StationRecords;
	int i = 0;

	if (StationsOpen == 0)
		return;

	row = 0;

	while (ptr)
	{
		RefreshStation(ptr);
		ptr = ptr->Next;
		i++;
	}

	if (row)
	{
		QHeaderView * hddr = stationList->horizontalHeader();
		stationList->sortItems(hddr->sortIndicatorSection(), hddr->sortIndicatorOrder());
	}

	stationList->setRowCount(row + 1);
	stationList->setCurrentCell(0, row + 1);
}


unsigned char * Image = NULL;
unsigned char * PopupImage = NULL;

int WIDTH = 2048;
int HEIGHT = 2048;

BOOL ImageChanged = 0;

int Bytesperpixel = 4;

/*

void DrawCharacter(int X, int Y, int j, unsigned char chr)
{
	// Font is 5 bits wide x 8 high. Each byte of font contains one column, so 5 bytes per char

	int Pointer, i, c, index, bit, mask;

	Pointer = ((Y - 5) * WIDTH * Bytesperpixel) + ((X + 11) * Bytesperpixel) + (j * 6 * Bytesperpixel);

	mask = 1;

	for (i = 0; i < 2; i++)
	{
		for (index = 0; index < 6; index++)
		{
			Image[Pointer++] = 255;				// Blank lines above chars 
			Image[Pointer++] = 255;
			if (Bytesperpixel == 4)
			{
				Image[Pointer++] = 255;
				Pointer++;
			}
		}

		Pointer += (WIDTH - 6) * Bytesperpixel;
	}

	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

	for (i = 0; i < 7; i++)
	{
		Image[Pointer++] = 255;				// Blank col between chars
		Image[Pointer++] = 255;
		if (Bytesperpixel == 4)
		{
			Image[Pointer++] = 255;
			Pointer++;
		}
		for (index = 0; index < 5; index++)
		{
			c = ASCII[chr - 0x20][index];	// Font data
			bit = c & mask;

			if (bit)
			{
				Image[Pointer++] = 0;
				Image[Pointer++] = 0;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 0;
					Pointer++;
				}
			}
			else
			{
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}
			}
		}
		mask <<= 1;
		Pointer += (WIDTH - 6) * Bytesperpixel;
	}

	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

	mask = 1;

	for (i = 0; i < 2; i++)
	{
		for (index = 0; index < 6; index++)
		{
			Image[Pointer++] = 255;				// Blank lines below chars between chars
			Image[Pointer++] = 255;
			if (Bytesperpixel == 4)
			{
				Image[Pointer++] = 255;
				Pointer++;
			}
		}
		Pointer += (WIDTH - 6) * Bytesperpixel;
	}
}
*/

int QtBPQAPRS::DrawStation(struct STATIONRECORD * ptr, int AllStations, QPainter * painter)
{
	int X, Y, i, calllen;
	unsigned int j;
	char Overlay;
	time_t AgeLimit = time(NULL) - (TrackExpireTime * 60);

//	char * off = (char *)&ptr->LastWXPacket;
//	char * jj = (char *)ptr;
//	int ll = off - jj;

	if (ptr->Moved == 0 && AllStations == 0)
		return 0;				// No need to repaint

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return 0;

	if (ptr->ObjState == '_')	// Killed Object
		return 0;

	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks

		if (X < 12 || Y < 12 || X >(WIDTH - 36) || Y >(HEIGHT - 36))
			return 0;				// Too close to edges

		if (ptr->LatTrack[0] && ptr->NoTracks == FALSE)
		{
			// Draw Track

			int Index = ptr->Trackptr;
			int n;
			int X, Y;
			int LastX = 0, LastY = 0;

			painter->setPen(Colours[ptr->TrackColour]);

			for (n = 0; n < TRACKPOINTS; n++)
			{
				if (ptr->LatTrack[Index] && ptr->TrackTime[Index] > AgeLimit)
				{
					if (GetLocPixels(ptr->LatTrack[Index], ptr->LonTrack[Index], &X, &Y))
					{
						if (LastX)
						{
							if (abs(X - LastX) < 600 && abs(Y - LastY) < 600)
								if (X > 0 && Y > 0 && X < (WIDTH - 5) && Y < (HEIGHT - 5))
									painter->drawLine(LastX, LastY, X, Y); //  Colours[ptr->TrackColour]);
						}

						LastX = X;
						LastY = Y;
					}
				}
				Index++;
				if (Index == TRACKPOINTS)
					Index = 0;

			}
		}
	
		
		ptr->Moved = 0;

		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks


		if (ptr->image == nullptr)
		{
			QImage timage(90, 18, QImage::Format_RGB32);		// Temp to get size

			QPainter tp(&timage);
			QRect Bounds;

			QFont font = tp.font();
			font.setPixelSize(iconFontSize);
			tp.setFont(font);

			Bounds = tp.boundingRect(20, 13, 100, 20, 0, ptr->Callsign);

			int right = 21 + Bounds.width();

			ptr->image = new QImage(right+1, 18, QImage::Format_RGB32);
			ptr->image->fill(Qt::lightGray);

			QPainter p(ptr->image);

			p.setFont(font);

			Bounds = p.boundingRect(20, 13, 100, 20, 0, ptr->Callsign);

			p.drawText(20, 13, ptr->Callsign);
			p.drawLine(0, 0, right, 0);
			p.drawLine(0, 17, right, 17);
			p.drawLine(0, 0, 0, 17);
			p.drawLine(17, 0, 17, 17);
			p.drawLine(right, 0, right, 17);

			// Draw Callsign Box

			QRgb* icon;

			// Draw Icon

			j = (ptr->iconRow * 21 * SymbolLineLen)
				+ (ptr->iconCol * 21 * Bytesperpixel)
				+ 3 * Bytesperpixel + 3 * SymbolLineLen;


			for (i = 0; i < 16; i++)
			{
				icon = (QRgb *)ptr->image->scanLine(i + 1);
				icon++;
				memcpy(icon, &iconImage[j], 16 * Bytesperpixel);
				j += SymbolLineLen;
			}

			// If an overlay is specified, add it


			Overlay = ptr->IconOverlay;

			if (Overlay)
			{
				// Create white background for overlay

				char Overlay[2] = "";

				Overlay[0] = ptr->IconOverlay;

				p.fillRect(6, 6, 7, 7, QColor(255, 255, 255));
				p.drawText(6, 13, Overlay);
			}

			calllen = strlen(ptr->Callsign);

			while (calllen && ptr->Callsign[calllen - 1] == ' ')		// Remove trailing spaces
				calllen--;

		}
		painter->drawImage(X, Y, *ptr->image);

	}
	else
	{
		ptr->DispX = -1;
		ptr->DispY = -1;					// Off Window
	}

	return 0;
}



void QtBPQAPRS::RefreshStationMap(int AllStations)
{
	struct STATIONRECORD * ptr = *StationRecords;

	int i = 0;

	QPainter painter(MapPixmap);

	StationCount = 0;

	while (ptr)
	{
		StationCount++;

		if (ptr->Moved || AllStations)
		{
			DrawStation(ptr, 1, &painter);

			if (AllStations == 0)
				ReloadMaps = 1;

			ptr->Moved = 0;

			i++;
		}
		ptr = ptr->Next;
	}

	ImageLabel->setPixmap(*MapPixmap);

	LastRefresh = time(NULL);

	//	if (RecsDeleted)
	RefreshStationList();


	//	Debugprintf("APRS Refresh - Sation Count = %d", StationCount);
}


// Find all stations displayed at location

void QtBPQAPRS::FindStationsByPixel(int MouseX, int MouseY, int wx, int wy)
{
	int j = 0;
	struct STATIONRECORD * ptr = *StationRecords;
	struct STATIONRECORD * List[1000];

	while (ptr && j < 999)
	{
		if (MouseX > ptr->DispX + 2 && MouseX < ptr->DispX + 80)		// 2 pixel margins for zoom
		{
			if (MouseY > ptr->DispY + 2 && MouseY < ptr->DispY + 16)
			{
				List[j++] = ptr;
			}
		}
		ptr = ptr->Next;
	}

	if (j == 0)
	{
		if (hPopupWnd)
		{
			hPopupWnd->close();
			hPopupWnd = 0;
		}

		if (hSelWnd)
		{
			hSelWnd->close();
			hSelWnd = 0;
		}

		return;
	}
	
	//	If only one, display info popup, else display selection popup
	
	if (hPopupWnd || hSelWnd)
			return;						// Already on display

	if (j == 1)
		CreateStationPopup(List[0], wx, wy);
	else
		CreateSelectionPopup(List, j, wx, wy);
}


void DecodeWXReport(struct APRSConnectionInfo * sockptr, char * WX)
{
	char * ptr = strchr(WX, '_');
	char Type;
	int Val;

	if (ptr == 0)
		return;

	sockptr->WindDirn = atoi(++ptr);
	ptr += 4;
	sockptr->WindSpeed = atoi(ptr);
	ptr += 3;
WXLoop:

	Type = *(ptr++);

	if (*ptr == '.')	// Missing Value
	{
		while (*ptr == '.')
			ptr++;

		goto WXLoop;
	}

	Val = atoi(ptr);

	switch (Type)
	{
	case 'c': // = wind direction (in degrees).	

		sockptr->WindDirn = Val;
		break;

	case 's': // = sustained one-minute wind speed (in mph).

		sockptr->WindSpeed = Val;
		break;

	case 'g': // = gust (peak wind speed in mph in the last 5 minutes).

		sockptr->WindGust = Val;
		break;

	case 't': // = temperature (in degrees Fahrenheit). Temperatures below zero are expressed as -01 to -99.

		sockptr->Temp = Val;
		break;

	case 'r': // = rainfall (in hundredths of an inch) in the last hour.

		sockptr->RainLastHour = Val / 100.0;
		break;

	case 'p': // = rainfall (in hundredths of an inch) in the last 24 hours.

		sockptr->RainLastDay = Val / 100.0;
		break;

	case 'P': // = rainfall (in hundredths of an inch) since midnight.

		sockptr->RainToday = Val / 100.0;
		break;

	case 'h': // = humidity (in %. 00 = 100%).

		sockptr->Humidity = Val;
		break;

	case 'b': // = barometric pressure (in tenths of millibars/tenths of hPascal).

		sockptr->Pressure = Val;
		break;

	default:

		return;
	}
	while (isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr != ' ')
		goto WXLoop;
}


void QtBPQAPRS::CreateStationPopup(struct STATIONRECORD * ptr, int X, int Y)
{
	if (hPopupWnd)
		return;

	popupDialog * Wnd = new popupDialog();

	StnList = new QTableWidget(Wnd);
	StnList->setColumnCount(1);
	StnList->setRowCount(100);

	StnList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	StnList->verticalHeader()->setDefaultSectionSize(20);

	StnList->horizontalHeader()->setMinimumSectionSize(200);
	StnList->horizontalHeader()->setStretchLastSection(true);

	StnList->verticalHeader()->setVisible(false);
	StnList->horizontalHeader()->setVisible(false);

	char Msg[256];
	struct tm * TM;
	int Len = 130;
	int width = 400;
	int i = 0;


	sprintf(Msg, "%s", ptr->Callsign);
	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

	if (ptr->Approx)
	{
		sprintf(Msg, "Approximate Position From Locator");
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));
	}
	sprintf(Msg, "%s", ptr->Path);
	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));
	sprintf(Msg, "%s", ptr->LastPacket);
	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));
	sprintf(Msg, "%s", ptr->Status);
	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

	sprintf(Msg, "Last Heard: %.2d:%.2d:%.2d on Port %d",
		TM->tm_hour, TM->tm_min, TM->tm_sec, ptr->LastPort);

	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

	sprintf(Msg, "Distance %6.1f Bearing %3.0f Course %1.0f\xC2\xB0 Speed %3.1f",
		Distance(ptr->Lat, ptr->Lon),
		Bearing(ptr->Lat, ptr->Lon), ptr->Course, ptr->Speed);

	StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

	if (ptr->LastWXPacket[0])
	{
		//display wx info

		struct APRSConnectionInfo temp;

		memset(&temp, 0, sizeof(temp));

		DecodeWXReport(&temp, ptr->LastWXPacket);

		sprintf(Msg, "Wind Speed %d MPH", temp.WindSpeed);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		sprintf(Msg, "Wind Gust %d MPH", temp.WindGust);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		sprintf(Msg, "Wind Direction %d\xC2\xB0", temp.WindDirn);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		sprintf(Msg, "Temperature %d\xC2\xB0 F", temp.Temp);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		sprintf(Msg, "Pressure %05.1f", temp.Pressure / 10.0);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		sprintf(Msg, "Humidity %d%%", temp.Humidity);
		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		Len += 100;

		sprintf(Msg, "Rainfall Last Hour/Last 24H/Today %.2f, %.2f, %.2f (inches)",
			temp.RainLastHour, temp.RainLastDay, temp.RainToday);

		StnList->setItem(i++, 0, new QTableWidgetItem(Msg));

		Len += 20;

		width = 500;
	}

	hPopupWnd = Wnd;

	StnList->setRowCount(i);
	StnList->setCurrentCell(0, i);

	connect(StnList, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(stnDoubleClicked(int, int)));

	hPopupWnd->setGeometry(X, Y, width, ++i * 20 );
	hPopupWnd->show();

}

int SelX, SelY;

void QtBPQAPRS::CreateSelectionPopup(struct STATIONRECORD ** ptr, int Count, int X, int Y)
{
	int i;

	if (hSelWnd)
		return;

	SelX = X;
	SelY = Y;

	popupDialog * Wnd = new popupDialog();

	StnList = new QTableWidget(Wnd);
	StnList->setColumnCount(1);
	StnList->setRowCount(100);

	StnList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	StnList->verticalHeader()->setDefaultSectionSize(20);

	StnList->horizontalHeader()->setMinimumSectionSize(60);
	StnList->horizontalHeader()->setStretchLastSection(true);

	StnList->verticalHeader()->setVisible(false);
	StnList->horizontalHeader()->setVisible(false);

	hSelWnd = Wnd;

	for (i = 0; i < Count; i++)
	{
		StnList->setItem(i, 0, new QTableWidgetItem(ptr[i]->Callsign));
	}

	StnList->setRowCount(i);
	StnList->setCurrentCell(0, i);


	hSelWnd->setGeometry(X, Y, 100, ++i * 20);
	StnList->sortItems(0);

	connect(StnList, SIGNAL(cellClicked(int, int)), this, SLOT(itemClicked(int, int)));

	hSelWnd->show();
}


void QtBPQAPRS::stnDoubleClicked(int row, int col)
{
	if (col == 0)
	{
		QTableWidgetItem * item = StnList->item(row, col);

		// Add to Calls QVector and to Combo box if on display

		if (item)
		{
			QString Call = item->data(Qt::DisplayRole).toString();

			if (!ToCalls.contains(Call))
				ToCalls.append(Call);

			if (Dlg)
			{
				int Index = Dlg->To->findText(Call);

				if (Index == -1)
					Dlg->To->addItem(Call);
			}
		}
	}
}


void QtBPQAPRS::itemClicked(int row, int col)
{
	QTableWidgetItem * item = StnList->item(row, col);

	if (item)
	{
		QVariant Call = item->data(Qt::DisplayRole);

		QByteArray QB = Call.toByteArray();
		char * ptr = QB.data();

		struct STATIONRECORD * stn = *StationRecords;

		while (stn)
		{
			if (strcmp(ptr, stn->Callsign) == 0)
			{
				if (hPopupWnd)
				{
					hPopupWnd->close();
					hPopupWnd = nullptr;
				}
				CreateStationPopup(stn, SelX + 100, SelY);
				return;
			}

			stn = stn->Next;
		}
	}
}


// Messaging Code

// Shared Memory Code

typedef void *HANDLE;

struct APRSMESSAGE
{
	struct APRSMESSAGE * Next;
	struct STATIONRECORD * ToStation;	// Set on messages we send
	char FromCall[12];
	char ToCall[12];
	char Text[104];
	char Seq[8];
	int Acked;
	int Retries;
	int RetryTimer;
	int Port;
	char Time[6];
	int Cancelled;
};


void RefreshMessages()
{
	struct APRSMESSAGE * ptr = SMEM->Messages;

	char BaseCall[10];
	char BaseFrom[10];

	if (MessagesOpen == 0)
		return;

	if (SMEM->NeedRefresh == 0)
		return;

	SMEM->NeedRefresh = 0;

	rxTable->clearContents();

	if (AllSSID)
	{
		memcpy(BaseCall, LoppedAPRSCall, 10);
		strlop(BaseCall, '-');
	}


	row = 0;

	rxTable->setRowCount(500);

	while (ptr)
	{
		if (ptr->ToCall[0] == 0)
		{
			ptr = ptr->Next;
			continue;
		}
		char ToLopped[11] = "";
		memcpy(ToLopped, ptr->ToCall, 10);
		strlop(ToLopped, ' ');

		if (memcmp(ToLopped, "BLN", 3) == 0)
			if (ShowBulls == TRUE)
				goto wantit;

		if (strcmp(ToLopped, LoppedAPRSCall) == 0)		//  to me?
			goto wantit;

		if (strcmp(ptr->FromCall, LoppedAPRSCall) == 0)	//  from me?
			goto wantit;

		if (AllSSID)
		{
			memcpy(BaseFrom, ptr->FromCall, 10);
			strlop(BaseFrom, '-');

			if (strcmp(BaseFrom, BaseCall) == 0)
				goto wantit;

			memcpy(BaseFrom, ToLopped, 10);
			strlop(BaseFrom, '-');

			if (strcmp(BaseFrom, BaseCall) == 0)
				goto wantit;
		}

		if (OnlyMine == FALSE)		// Want All
			if (OnlySeq == FALSE || (ptr && ptr->Seq[0] != 0))
				goto wantit;

		// ignore

		ptr = ptr->Next;
		continue;

	wantit:

		rxTable->setItem(row, 0, new QTableWidgetItem((char *)ptr->FromCall));
		rxTable->setItem(row, 1, new QTableWidgetItem((char *)ptr->ToCall));
		rxTable->setItem(row, 2, new QTableWidgetItem((char *)ptr->Seq));
		rxTable->setItem(row, 3, new QTableWidgetItem((char *)ptr->Time));
		rxTable->setItem(row, 4, new QTableWidgetItem((char *)ptr->Text));

		row++;
		ptr = ptr->Next;
	}

	rxTable->setRowCount(row + 1);
	rxTable->setCurrentCell(0, row + 1);

	ptr = SMEM->OutstandingMsgs;
	row = 0;

	while (ptr)
	{
		char Retries[10];

		sprintf(Retries, "%d", ptr->Retries);

		if (ptr->Acked)
			Retries[0] = 'A';
		else if (ptr->Retries == 0)
			Retries[0] = 'F';

		txTable->setItem(row, 0, new QTableWidgetItem((char *)ptr->ToCall));
		txTable->setItem(row, 2, new QTableWidgetItem((char *)Retries));
		txTable->setItem(row, 1, new QTableWidgetItem((char *)ptr->Seq));
		txTable->setItem(row, 3, new QTableWidgetItem((char *)ptr->Text));
		ptr = ptr->Next;
		row++;
	}

	txTable->setRowCount(row + 1);
	txTable->setCurrentCell(0, row + 1);
}


VOID SendFilterCommand(char * Filter)
{
	char Msg[2000];
	char Server[] = "SERVER";

	sprintf(Msg, "filter %s", Filter);

	APISendAPRSMessage(Msg, Server);
}



void QtBPQAPRS::GetConfig()
{
	QSettings settings("QtBPQAPRS.ini", QSettings::IniFormat);

	Zoom = settings.value("Zoom", 2).toInt();
	Lat = settings.value("Lat", 0.0).toDouble();
	Lon = settings.value("Lon", 0.0).toDouble();

	WindowX = settings.value("WindowX", 100).toInt();
	WindowY = settings.value("WindowY", 100).toInt();
	WindowWidth = settings.value("WindowWidth", 788).toInt();
	WindowHeight = settings.value("WindowHeight", 788).toInt();

	stnWinWidth = settings.value("stnWinWidth", 786).toInt();
	stnWinHeight = settings.value("stnWinHeight", 614).toInt();
	stnWinX = settings.value("stnWinX", 100).toInt();
	stnWinY = settings.value("stnWinY", 100).toInt();

	msgWinWidth = settings.value("msgWinWidth", 841).toInt();
	msgWinHeight = settings.value("msgWinHeight", 611).toInt();
	msgWinX = settings.value("msgWinX", 100).toInt();
	msgWinY = settings.value("msgWinY", 100).toInt();
	Split = settings.value("Split", 100).toInt();

	OnlyMine = settings.value("OnlyMine", 0).toInt();
	OnlySeq = settings.value("OnlySeq", 1).toInt();
	ShowBulls = settings.value("ShowBulls", 0).toInt();

	LocalTime = settings.value("LocalTime", 0).toInt();
	KM = settings.value("KM", 0).toInt();

	AddViewToFilter = settings.value("AddViewToFilter", 0).toInt();

	CreateJPEG = settings.value("CreateJPEG", 1).toInt();
	JPEGinterval = settings.value("JPEGInterval", 300).toInt();

	strcpy(JPEGFilename, settings.value("JPEGFileName", "").toString().toUtf8());
	strcpy(ISFilter, settings.value("ISFilter", "").toString().toUtf8());

	settings.setValue("JPEGFileName", JPEGFilename);

	iconFontSize = settings.value("iconFontSize", 9).toInt();
	TrackExpireTime = settings.value("TrackExpireTime", 1440).toInt();
	SuppressNullPosn = settings.value("SuppressNullPosn", 0).toInt();
	AddViewToFilter = settings.value("AddViewToFilter", 0).toInt();
}

void QtBPQAPRS::SaveConfig()
{
	QSettings settings("QtBPQAPRS.ini", QSettings::IniFormat);

	QRect rect = QtBPQAPRS::geometry();

	WindowHeight = cyWinSize = rect.height();
	WindowWidth = cxWinSize = rect.width();
	WindowX = rect.left();
	WindowY = rect.top();


	settings.setValue("Zoom", Zoom);
	settings.setValue("Lat", Lat);
	settings.setValue("Lon", Lon);

	settings.setValue("WindowX", WindowX);
	settings.setValue("WindowY", WindowY);
	settings.setValue("WindowWidth", WindowWidth);
	settings.setValue("WindowHeight", WindowHeight);

	settings.setValue("stnWinWidth", stnWinWidth);
	settings.setValue("stnWinHeight", stnWinHeight);
	settings.setValue("stnWinX", stnWinX);
	settings.setValue("stnWinY", stnWinY);


	settings.setValue("msgWinWidth", msgWinWidth);
	settings.setValue("msgWinHeight", msgWinHeight);
	settings.setValue("msgWinX", msgWinX);
	settings.setValue("msgWinY", msgWinY);
	settings.setValue("Split", Split);

	settings.setValue("OnlyMine", OnlyMine);
	settings.setValue("OnlySeq", OnlySeq);
	settings.setValue("ShowBulls", ShowBulls);

	settings.setValue("LocalTime", LocalTime);
	settings.setValue("KM", KM);

	settings.setValue("AddViewToFilter", AddViewToFilter);

	settings.setValue("CreateJPEG", CreateJPEG);
	settings.setValue("JPEGInterval", JPEGinterval);

	settings.setValue("JPEGFileName", JPEGFilename);
	settings.setValue("iconFontSize", iconFontSize);
	settings.setValue("TrackExpireTime", TrackExpireTime);
	settings.setValue("SuppressNullPosn", SuppressNullPosn);
	settings.setValue("AddViewToFilter", AddViewToFilter);

}


