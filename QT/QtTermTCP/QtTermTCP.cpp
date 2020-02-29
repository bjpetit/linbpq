// Qt Version of BPQTermTCP

#define VersionString "0.0.0.26"

// .12 Save font weight
// .13 Display incomplete lines (ie without CR)
// .14 Add YAPP and Listen Mode
// .15 Reuse windows in Listen Mode 
// .17 MDI Version 7/1/20
// .18 Fix inpout window losing focus when data arrives on other window
// .19 Fix Scrollback
// .20 WinXP compatibility changes
// .21 Save Window Positions
// .22 Open a window on first start
// .23 Add Tabbed display option
// .24 Fix crash when setting Monitor flags in Tabbed mode
// .25 Add AGW mode
// .26 Add sending CTRL/C CTRL/D and CTRL/Z



#define _CRT_SECURE_NO_WARNINGS

#include "QtTermTCP.h"
#include "TabDialog.h"
#include <time.h>
#include <QVBoxLayout>
#include <QListWidgetItem>


#ifndef WIN32
#define strtok_s strtok_r
#endif


#define MAXHOSTS 16
#define MAXPORTS 32

#define UNREFERENCED_PARAMETER(P)          (P)

char Host[MAXHOSTS + 1][100] = { "" };
int Port[MAXHOSTS + 1] = { 0 };
char UserName[MAXHOSTS + 1][80] = { "" };
char Password[MAXHOSTS + 1][80] = { "" };
char MonParams[MAXHOSTS + 1][80] = { "" };
int ListenPort = 8015;

// Session Type Equates

#define Term 1
#define Mon 2
#define Listen 4

// Presentation - Single Window, MDI or Tabbed

int TermMode = 1;

#define Single 0
#define MDI 1
#define Tabbed 2

int singlemodeFormat = Mon + Term;




// There is something odd about this. It doesn't match BPQTERMTCP though it looks the same

// Chat uses these (+ 10)
//{ 0, 4, 9, 11, 13, 16, 17, 42, 45, 50, 61, 64, 66, 72, 81, 84, 85, 86, 87, 89 };

// As we have a white background we need dark colours

QRgb Colours[256] = { 0,
		qRgb(0,0,0), qRgb(0,0,128), qRgb(0,0,192), qRgb(0,0,255),				// 1 - 4
		qRgb(0,64,0), qRgb(0,64,128), qRgb(0,64,192), qRgb(0,64,255),			// 5 - 8
		qRgb(0,128,0), qRgb(0,128,128), qRgb(0,128,192), qRgb(0,128,255),		// 9 - 12
		qRgb(0,192,0), qRgb(0,192,128), qRgb(0,192,192), qRgb(0,192,255),		// 13 - 16
		qRgb(0,255,0), qRgb(0,255,128), qRgb(0,255,192), qRgb(0,255,255),		// 17 - 20

		qRgb(64,0,0), qRgb(64,0,128), qRgb(64,0,192), qRgb(0,0,255),				// 21 
		qRgb(64,64,0), qRgb(64,64,128), qRgb(64,64,192), qRgb(64,64,255),
		qRgb(64,128,0), qRgb(64,128,128), qRgb(64,128,192), qRgb(64,128,255),
		qRgb(64,192,0), qRgb(64,192,128), qRgb(64,192,192), qRgb(64,192,255),
		qRgb(64,255,0), qRgb(64,255,128), qRgb(64,255,192), qRgb(64,255,255),

		qRgb(128,0,0), qRgb(128,0,128), qRgb(128,0,192), qRgb(128,0,255),				// 41
		qRgb(128,64,0), qRgb(128,64,128), qRgb(128,64,192), qRgb(128,64,255),
		qRgb(128,128,0), qRgb(128,128,128), qRgb(128,128,192), qRgb(128,128,255),
		qRgb(128,192,0), qRgb(128,192,128), qRgb(128,192,192), qRgb(128,192,255),
		qRgb(128,255,0), qRgb(128,255,128), qRgb(128,255,192), qRgb(128,255,255),

		qRgb(192,0,0), qRgb(192,0,128), qRgb(192,0,192), qRgb(192,0,255),				// 61
		qRgb(192,64,0), qRgb(192,64,128), qRgb(192,64,192), qRgb(192,64,255),
		qRgb(192,128,0), qRgb(192,128,128), qRgb(192,128,192), qRgb(192,128,255),
		qRgb(192,192,0), qRgb(192,192,128), qRgb(192,192,192), qRgb(192,192,255),
		qRgb(192,255,0), qRgb(192,255,128), qRgb(192,255,192), qRgb(192,255,255),

		qRgb(255,0,0), qRgb(255,0,128), qRgb(255,0,192), qRgb(255,0,255),				// 81
		qRgb(255,64,0), qRgb(255,64,128), qRgb(255,64,192), qRgb(255,64,255),
		qRgb(255,128,0), qRgb(255,128,128), qRgb(255,128,192), qRgb(255,128,255),
		qRgb(255,192,0), qRgb(255,192,128), qRgb(255,192,192), qRgb(255,192,255),
		qRgb(255,255,0), qRgb(255,255,128), qRgb(255,255,192), qRgb(255,255,255)
};

int SavedHost = 0;				// from config

char * sessionList = NULL;		// Saved sessions

extern int ChatMode;
extern int Bells;

extern time_t LastWrite;
extern int AlertInterval;
extern int AlertBeep;
extern int AlertFreq;
extern int AlertDuration;

// AGW Host Interface stuff

int AGWEnable = 0;
char AGWTermCall[12] = "";
char AGWBeaconDest[12] = "";
char AGWBeaconPath[80] = "";
int AGWBeaconInterval = 0;
char AGWBeaconPorts[80];
char AGWHost[128] = "127.0.0.1";
int AGWPortNum = 8000;
int AGWPaclen = 80;
extern char * AGWPortList;
extern myTcpSocket * AGWSock; 

QStringList AGWToCalls;


extern char YAPPPath[256];

void menuChecked(QAction * Act);

void GetSettings();

// These widgets defined here as they are accessed from outside the framework

QLabel * Status1;
QLabel * Status2;
QLabel * Status3;
QLabel * Status4;

QAction *actHost[17];
QAction *actSetup[16];

QAction * TabSingle = NULL;
QAction * TabBoth = NULL;
QAction * TabMon = NULL;

QMenu *monitorMenu; 
QMenu * YAPPMenu;
QMenu *connectMenu;
QMenu *disconnectMenu;

QMenu * ListenMenu;

QAction *MonTX;
QAction *MonSup;
QAction *MonNodes;
QAction *MonUI;
QAction *MonColour;
QAction *MonPort[32];
QAction *actChatMode;
QAction *actBells;
QAction *actFonts;
QAction *YAPPSend;
QAction *YAPPSetRX;
QAction *singleAct;
QAction *singleAct2;
QAction *MDIAct;
QAction *tabbedAct;
QAction *discAction;

QTabWidget *tabWidget;
QMdiArea *mdiArea;
QWidget * mythis;

QList<Ui_ListenSession *> _sessions;

// Session Type Equates

#define Term 1
#define Mon 2
#define Listen 4

int TabType[10] = { Term, Term + Mon, Term, Term, Term, Term, Mon, Mon, Mon };

int listenPort = 8015;
bool listenEnable = false;
char listenCText[4096] = "";


int ConfigHost = 0;

int Split = 50;				// Mon/Term size split

int termX;

bool Cascading = false;		// Set to stop size being saved when cascading

QMdiSubWindow * ActiveSubWindow = NULL;
Ui_ListenSession * ActiveSession = NULL;

Ui_ListenSession * newWindow(QObject * parent, int Type, const char * Label = nullptr);
void Send_AGW_C_Frame(Ui_ListenSession * Sess, int Port, char * CallFrom, char * CallTo, char * Data, int DataLen);
void AGW_AX25_data_in(void * AX25Sess, int PID, unsigned char * data, int Len);

void EncodeSettingsLine(int n, char * String)
{
	sprintf(String, "%s|%d|%s|%s|%s", Host[n], Port[n], UserName[n], Password[n], MonParams[n]);
	return;
}

void DecodeSettingsLine(int n, char * String)
{
	char * Param = strdup(String);
	char * Rest;
	char * Save = Param;			// for Free

	Rest = strlop(Param, '|');

	if (Rest == NULL)
		return;

	strcpy(Host[n], Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	Port[n] = atoi(Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	strcpy(UserName[n], Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	strcpy(Password[n], Param);

	strcpy(MonParams[n], Rest);


	free(Save);
	return;
}



bool QtTermTCP::eventFilter(QObject* obj, QEvent *event)
{
	// See if from a Listening Session

	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);

//	Sess = _sessions.at(i);	for (Ui_ListenSession * Sess : _sessions)
//	{
		if (Sess == NULL)
			continue;

		if (Sess == obj)
		{
			if (event->type() == QEvent::Close)
			{
				// Window closing
				
				if (Sess->clientSocket)
					Sess->clientSocket->disconnectFromHost();
	
				_sessions.removeOne(Sess);
			}

			if (event->type() == QEvent::Resize)
			{
				QRect r = Sess->rect();

				int H, Mid, monHeight, termHeight, Width, Border = 3;

				Width = r.width();
				H = r.height();

				if (TermMode == Tabbed)
				{
					//					H -= 20;
					Width -= 7;
				}
				else if (TermMode == Single)
				{
					Width -= 7;
//					H += 20;
				}

				if (Sess->SessionType == Listen || Sess->SessionType == Term)
				{
					// Term and Input

					// Calc Positions of window

					termHeight = H - (25 + 3 * Border);

					Sess->termWindow->setGeometry(QRect(Border, Border, Width, termHeight));
					Sess->inputWindow->setGeometry(QRect(Border, H - (25 + Border), Width, 25));
				}
				else if (Sess->SessionType == (Term | Mon))
				{
					// All 3

					// Calc Positions of window

					Mid = (H * Split) / 100;

					monHeight = Mid - Border;
					termX = Mid;
					termHeight = H - Mid - (25 + 3 * Border);

					Sess->monWindow->setGeometry(QRect(Border, Border, Width, monHeight));
					Sess->termWindow->setGeometry(QRect(Border, Mid + Border, Width, termHeight));
					Sess->inputWindow->setGeometry(QRect(Border, H - (25 + Border), Width, 25));
				}
				else
				{
					// Should just be Mon only

					Sess->monWindow->setGeometry(QRect(Border, Border, Width, H - 2 * Border));
				}

			}
		}

		if (Sess->inputWindow == obj)
		{
			if (event->type() == QEvent::KeyPress)
			{
				QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

				int key = keyEvent->key();
				Qt::KeyboardModifiers modifier = keyEvent->modifiers();

				if (modifier == Qt::ControlModifier)
				{
					char Msg[] = "\0\r";

					if (key == Qt::Key_Z)
						Msg[0] = 0x1a;
					else if (key == Qt::Key_C)
						Msg[0] = 3;
					else if (key == Qt::Key_D)
						Msg[0] = 4;

					if (Msg[0])
					{

						if (Sess->AGWSession)
						{
							// Terminal is in AGWPE mode - send as AGW frame

							AGW_AX25_data_in(Sess->AGWSession, 240, (unsigned char *)Msg, strlen(Msg));

						}
						else if (Sess->clientSocket && Sess->clientSocket->state() == QAbstractSocket::ConnectedState)
						{
							Sess->clientSocket->write(Msg);
						}

						return true;
					}
				}
				
				if (key == Qt::Key_Up)
				{
					// Scroll up

					if (Sess->KbdStack[Sess->StackIndex] == NULL)
						return true;

					Sess->inputWindow->setText(Sess->KbdStack[Sess->StackIndex]);
					Sess->inputWindow->cursorForward(strlen(Sess->KbdStack[Sess->StackIndex]));

					Sess->StackIndex++;
					if (Sess->StackIndex == 50)
						Sess->StackIndex = 49;

					return true;
				}
				else if (key == Qt::Key_Down)
				{
					// Scroll down

					if (Sess->StackIndex == 0)
					{
						Sess->inputWindow->setText("");
						return true;
					}

					Sess->StackIndex--;

					if (Sess->StackIndex && Sess->KbdStack[Sess->StackIndex - 1])
					{
						Sess->inputWindow->setText(Sess->KbdStack[Sess->StackIndex - 1]);
						Sess->inputWindow->cursorForward(strlen(Sess->KbdStack[Sess->StackIndex - 1]));
					}
					else
						Sess->inputWindow->setText("");

					return true;
				}
				else if (key == Qt::Key_Return || key == Qt::Key_Enter)
				{
					LreturnPressed(Sess);
					return true;
				}

				return false;
			}

			if (event->type() == QEvent::MouseButtonPress)
			{
				QMouseEvent *k = static_cast<QMouseEvent *> (event);

				// Paste on Right Click

				if (k->button() == Qt::RightButton)
				{
					Sess->inputWindow->paste();
					return true;
				}
				return QMainWindow::eventFilter(obj, event);
			}
		}
	}
	return QMainWindow::eventFilter(obj, event);
}
	
QAction * setupMenuLine(QMenu * Menu, char * Label, QObject * parent, int State)
{
	QAction * Act = new QAction(Label, parent);
	if (Menu)
		Menu->addAction(Act);

	Act->setCheckable(true);
	if (State)
		Act->setChecked(true);

	parent->connect(Act, SIGNAL(triggered()), parent, SLOT(menuChecked()));

	return Act;
}

// This now creates all window types - Term, Mon, Combined, Listen

int Count = 0;

Ui_ListenSession * newWindow(QObject * parent, int Type, const char * Label)
{
	Ui_ListenSession * Sess = new(Ui_ListenSession);

	// Need to explicity initialise on Qt4

	
	Sess->termWindow = NULL;
	Sess->monWindow = NULL;
	Sess->inputWindow = NULL;

	for (int i = 0; i < 50; i++)
		Sess->KbdStack[i] = NULL;

	Sess->StackIndex = 0;
	Sess->InputMode = 0;
	Sess->SlowTimer = 0;
	Sess->MonData = 0;
	Sess->OutputSaveLen = 0;
	Sess->MonSaveLen = 0;
	Sess->PortMonString[0] = 0;
	Sess->portmask = 0;
	Sess->portmask = 1;
	Sess->mtxparam = 1;
	Sess->mcomparam = 1;
	Sess->monUI = 0;
	Sess->MonitorNODES = 0;
	Sess->MonitorColour = 1;
	Sess->CurrentHost = 0;

	Sess->SessionType = Type;
	Sess->clientSocket = NULL;
	Sess->AGWSession = NULL;

	_sessions.push_back(Sess);

	Sess->setObjectName(QString::fromUtf8("Sess"));

	if (TermMode == MDI)
	{
		Sess->sw = mdiArea->addSubWindow(Sess);
//		Sess->installEventFilter(parent);
		Sess->sw->resize(800, 600);
	}
	else if (TermMode == Tabbed)
	{
		Sess->Tab = Count++;

		if (Type == Mon)
			tabWidget->addTab(Sess, "Monitor");
		else
			tabWidget->addTab(Sess, Label);
	}

	Sess->installEventFilter(parent);

	QSettings settings("QtTermTCP.ini", QSettings::IniFormat);

	QFont font = QFont(settings.value("FontFamily", "Courier New").value<QString>(),
		settings.value("PointSize", 10).toInt(),
		settings.value("Weight", 50).toInt());

	if ((Type & Mon))
	{
		Sess->monWindow = new QTextEdit(Sess);
		Sess->monWindow->setReadOnly(1);
//		Sess->monWindow->setGeometry(QRect(0, 0, 770, 300));
		Sess->monWindow->setFont(font);
		mythis->connect(Sess->monWindow, SIGNAL(selectionChanged()), parent, SLOT(onTEselectionChanged()));
	}

	if ((Type & (Listen | Term)))
	{
		Sess->termWindow = new QTextEdit(Sess);
		Sess->termWindow->setReadOnly(1);
//		Sess->termWindow->setGeometry(QRect(0, 0, 770, 300));
		Sess->termWindow->setFont(font);
		mythis->connect(Sess->termWindow, SIGNAL(selectionChanged()), parent, SLOT(onTEselectionChanged()));

		Sess->inputWindow = new QLineEdit(Sess);
//		Sess->inputWindow->setGeometry(QRect(0, 0, 770, 650));
		Sess->inputWindow->installEventFilter(parent);
		Sess->inputWindow->setFont(font);
		Sess->inputWindow->setContextMenuPolicy(Qt::PreventContextMenu);

		mythis->connect(Sess->inputWindow, SIGNAL(selectionChanged()), parent, SLOT(onLEselectionChanged()));
	}

	if (Type == (Term | Mon))
	{

		// Add Custom Menu to set Mon/Term Split with Right Click

		Sess->monWindow->setContextMenuPolicy(Qt::CustomContextMenu);
		Sess->termWindow->setContextMenuPolicy(Qt::CustomContextMenu);


		mythis->connect(Sess->monWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
			parent, SLOT(showContextMenuM(const QPoint &)));

		mythis->connect(Sess->termWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
			parent, SLOT(showContextMenuT(const QPoint &)));
	}

	if (Sess->SessionType == Mon)			// Mon Only
		Sess->setWindowTitle("Monitor Session Disconnected");
	else
		Sess->setWindowTitle("Disconnected");

	Sess->installEventFilter(mythis);
	Sess->show();

	int pos = (_sessions.size() - 1) * 20;

	if (TermMode == MDI)
	{
		Sess->sw->move(pos, pos);
	}

	QSize Size(800, 602);						// Not actually used, but Event constructor needs it

	QResizeEvent event(Size, Size);
	QApplication::sendEvent(Sess, &event);		// Resize Widgets to fix Window

	return Sess;
}


QtTermTCP::QtTermTCP(QWidget *parent) : QMainWindow(parent)
{
	int i;
	char Title[80];

	mythis = this;

	ui.setupUi(this);

	QSettings mysettings("QtTermTCP.ini", QSettings::IniFormat);

	restoreGeometry(mysettings.value("geometry").toByteArray());
	restoreState(mysettings.value("windowState").toByteArray());

	GetSettings();

	if (TermMode == MDI)
	{
		mdiArea = new QMdiArea(ui.centralWidget);

		mdiArea->setGeometry(QRect(0, 0, 771, 571));

		mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

		connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(xon_mdiArea_changed()));

		setCentralWidget(mdiArea);
	}
	else if (TermMode == Tabbed)
	{
		tabWidget = new QTabWidget(this);

		ui.verticalLayout->addWidget(tabWidget);

		tabWidget->setTabPosition(QTabWidget::South);

		newWindow(this, TabType[0], "Sess 1");
		newWindow(this, TabType[1], "Sess 2");
		newWindow(this, TabType[2], "Sess 3");
		newWindow(this, TabType[3], "Sess 4");
		newWindow(this, TabType[4], "Sess 5");
		newWindow(this, TabType[5], "Sess 6");
		newWindow(this, TabType[6], "Sess 7");
		newWindow(this, TabType[7], "Monitor");
		newWindow(this, TabType[8], "Monitor");

		ActiveSession= _sessions.at(0);

		connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
	}

	sprintf(Title, "QtTermTCP Version %s", VersionString);

	this->setWindowTitle(Title);

	QFont menufont = QFont("Aerial", 10);
	
//	ui.menuBar->setGeometry(QRect(0, 0, 951, 50));
	ui.menuBar->setFont(menufont);

	windowMenu = menuBar()->addMenu(tr("&Window"));
	connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

	newTermAct = new QAction(tr("New Terminal Window"), this);
	connect(newTermAct, SIGNAL(triggered()), this, SLOT(doNewTerm()));

	newMonAct = new QAction(tr("New Monitor Window"), this);
	connect(newMonAct, SIGNAL(triggered()), this, SLOT(doNewMon()));

	newCombinedAct = new QAction(tr("New Combined Term/Mon Window"), this);
	connect(newCombinedAct, SIGNAL(triggered()), this, SLOT(doNewCombined()));

	if (TermMode == MDI)
	{
		closeAct = new QAction(tr("Cl&ose"), this);
		closeAct->setStatusTip(tr("Close the active window"));
		connect(closeAct, SIGNAL(triggered()), mdiArea, SLOT(closeActiveSubWindow()));

		closeAllAct = new QAction(tr("Close &All"), this);
		closeAllAct->setStatusTip(tr("Close all the windows"));
		connect(closeAllAct, SIGNAL(triggered()), mdiArea, SLOT(closeAllSubWindows()));

		tileAct = new QAction(tr("&Tile"), this);
		tileAct->setStatusTip(tr("Tile the windows"));
		connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

		cascadeAct = new QAction(tr("&Cascade"), this);
		cascadeAct->setStatusTip(tr("Cascade the windows"));
		connect(cascadeAct, SIGNAL(triggered()), this, SLOT(doCascade()));

		nextAct = new QAction(tr("Ne&xt"), this);
		nextAct->setShortcuts(QKeySequence::NextChild);
		nextAct->setStatusTip(tr("Move the focus to the next window"));
		connect(nextAct, SIGNAL(triggered()), mdiArea, SLOT(activateNextSubWindow()));

		previousAct = new QAction(tr("Pre&vious"), this);
		previousAct->setShortcuts(QKeySequence::PreviousChild);
		previousAct->setStatusTip(tr("Move the focus to the previous "
			"window"));
		connect(previousAct, SIGNAL(triggered()), mdiArea, SLOT(activatePreviousSubWindow()));
	}
	
	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(doQuit()));


	windowMenuSeparatorAct = new QAction(this);
	windowMenuSeparatorAct->setSeparator(true);

	updateWindowMenu();

	connectMenu = ui.menuBar->addMenu(tr("&Connect"));

	actHost[16] = new QAction("AGW Connect", this);
	actHost[16]->setFont(menufont);
	actHost[16]->setVisible(0);
	connectMenu->addAction(actHost[16]);

	connect(actHost[16], SIGNAL(triggered()), this, SLOT(Connect()));

	for (i = 0; i < MAXHOSTS; i++)
	{
		actHost[i] = new QAction(Host[i], this);
		actHost[i]->setFont(menufont);
		connectMenu->addAction(actHost[i]);
		connect(actHost[i], SIGNAL(triggered()), this, SLOT(Connect()));
	}

	discAction = ui.menuBar->addAction("&Disconnect");
	connect(discAction, SIGNAL(triggered()), this, SLOT(Disconnect()));
	discAction->setEnabled(false);

	setupMenu = ui.menuBar->addMenu(tr("&Setup"));
	hostsubMenu = setupMenu->addMenu("Setup Hosts");

	for (i = 0; i < MAXHOSTS; i++)
	{
		if (Host[i][0])
			actSetup[i] = new QAction(Host[i], this);
		else
			actSetup[i] = new QAction("New Host", this);

		hostsubMenu->addAction(actSetup[i]);
		connect(actSetup[i], SIGNAL(triggered()), this, SLOT(SetupHosts()));
	}

	// Setup Presentation Options

	setupMenu->addSeparator()->setText(tr("Presentation"));

	QActionGroup * termGroup = new QActionGroup(this);

	singleAct = setupMenuLine(nullptr, (char *)"Single Window (Term + Mon)", this, (TermMode == Single) && (singlemodeFormat == Term + Mon));
	singleAct2 = setupMenuLine(nullptr, (char *)"Single Window (Term only)", this, (TermMode == Single) && (singlemodeFormat == Term));
	MDIAct = setupMenuLine(nullptr, (char *)"MDI", this, TermMode == MDI);
	tabbedAct = setupMenuLine(nullptr, (char *)"Tabbed", this, TermMode == Tabbed);

	termGroup->addAction(singleAct);
	termGroup->addAction(singleAct2);
	termGroup->addAction(MDIAct);
	termGroup->addAction(tabbedAct);

	setupMenu->addAction(singleAct);
	setupMenu->addAction(singleAct2);
	setupMenu->addAction(MDIAct);
	setupMenu->addAction(tabbedAct);


	setupMenu->addSeparator();

	actFonts = new QAction("Setup Fonts", this);
	setupMenu->addAction(actFonts);
	connect(actFonts, SIGNAL(triggered()), this, SLOT(selFont()));

	AGWAction = new QAction("AGW Setup", this); 
	setupMenu->addAction(AGWAction);
	connect(AGWAction, SIGNAL(triggered()), this, SLOT(AGWSlot()));

	actChatMode = setupMenuLine(setupMenu, (char *)"Chat Terminal Mode", this, ChatMode);
	actBells = setupMenuLine(setupMenu, (char *)"Enable Bells", this, Bells);

	monitorMenu = ui.menuBar->addMenu(tr("&Monitor"));

	MonTX = setupMenuLine(monitorMenu, (char *)"Monitor TX", this, 1);
	MonSup = setupMenuLine(monitorMenu, (char *)"Monitor Supervisory", this, 1);
	MonUI = setupMenuLine(monitorMenu, (char *)"Only Monitor UI Frames", this, 0);
	MonNodes = setupMenuLine(monitorMenu, (char *)"Monitor NODES Broadcasts", this, 0);
	MonColour = setupMenuLine(monitorMenu, (char *)"Enable Colour", this, 1);

	for (i = 0; i < MAXPORTS; i++)
	{
		MonPort[i] = setupMenuLine(monitorMenu, (char *)"Port", this, 0);
		MonPort[i]->setVisible(false);
	}

	ListenAction = ui.menuBar->addAction("&Listen");
	connect(ListenAction, SIGNAL(triggered()), this, SLOT(ListenSlot()));

	YAPPMenu = ui.menuBar->addMenu(tr("&YAPP"));

	YAPPSend = new QAction("Send File", this);
	YAPPSetRX = new QAction("Set Receive Directory", this);

	YAPPMenu->addAction(YAPPSend);
	YAPPMenu->addAction(YAPPSetRX);
	YAPPSend->setEnabled(false);

	connect(YAPPSend, SIGNAL(triggered()), this, SLOT(doYAPPSend()));
	connect(YAPPSetRX, SIGNAL(triggered()), this, SLOT(doYAPPSetRX()));

	// Set up Status Bar

	
	setStyleSheet("QStatusBar{border-top: 1px outset black;} QStatusBar::item { border: 1px solid black; border-radius: 3px;}");
//	setStyleSheet("QStatusBar{border-top: 1px outset black;}");

	
	Status4 = new QLabel(""); 
	Status3 = new QLabel("         ");
	Status2 = new QLabel("    ");
	Status1 = new QLabel("  Disconnected  ");

	Status1->setMinimumWidth(120);
	Status2->setFixedWidth(100);
	Status3->setFixedWidth(100);
	Status4->setFixedWidth(100);


	statusBar()->addPermanentWidget(Status4);
	statusBar()->addPermanentWidget(Status3);
	statusBar()->addPermanentWidget(Status2);
	statusBar()->addPermanentWidget(Status1);

	if (AGWEnable == 0)
		Status1->setText("AGW Interface Disabled");

	// Restore saved sessions

	if (TermMode == Single)
	{
		Ui_ListenSession * Sess = newWindow(this, singlemodeFormat);

		ActiveSession = Sess;
		ui.verticalLayout->addWidget(Sess);
	}

	if (TermMode == MDI)
	{
		int n = atoi(sessionList);

		char *p, *Context;
		char delim[] = "|";

		int type = 0, host = 0, topx, leftx, bottomx, rightx;

		p = strtok_s((char *)sessionList, delim, &Context);

		while (n--)
		{
			p = strtok_s(NULL, delim, &Context);

			sscanf(p, "%d,%d,%d,%d,%d,%d", &type, &host, &topx, &leftx, &bottomx, &rightx);

			Ui_ListenSession * Sess = newWindow(this, type);

			Sess->CurrentHost = host;

			QRect r(leftx, topx, rightx - leftx, bottomx - topx);

			Sess->sw->setGeometry(r);
			Sess->sw->move(leftx, topx);

		}
	}

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(10000);

	// Run timer now to connect to AGW if configured

	MyTimerSlot();

	if (listenEnable)
		_server.listen(QHostAddress::Any, listenPort);
	
	connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

void QtTermTCP::doQuit()
{
	if (_server.isListening())
		_server.close();

	SaveSettings();

	QCoreApplication::quit();
}

// "Copy on select" Code


void QtTermTCP::onTEselectionChanged()
{
	QTextEdit * x = static_cast<QTextEdit*>(QObject::sender());
	x->copy();
}

void QtTermTCP::onLEselectionChanged()
{
	QLineEdit * x = static_cast<QLineEdit*>(QObject::sender());
	x->copy();
}

int splitY;					// Value when menu added

void QtTermTCP::setSplit()
{
	QAction * sender = static_cast<QAction*>(QObject::sender());

	QWidget * Parent = sender->parentWidget();

	if (Parent)
		Parent = Parent->parentWidget();

	int y = Parent->rect().height() - 50;

	// y is height of whole windom. splitX is new split position
	// So split = 100 * splitx/x

	Split = (splitY * 100) / y;

	if (Split < 10)
		Split = 10;
	else if (Split > 90)
		Split = 90;

	QSize size(800, 602);
	QResizeEvent event(size, size);

	eventFilter(Parent, &event);
}
void QtTermTCP::showContextMenuT(const QPoint &pt)				// Term Window
{
	QTextEdit* sender = static_cast<QTextEdit*>(QObject::sender());

	QMenu *menu = sender->createStandardContextMenu();
	QAction * actSplit = new QAction("Set Monitor/Output Split", sender);

	menu->addAction(actSplit);

	splitY = pt.y() + termX;

	connect(actSplit, SIGNAL(triggered()), this, SLOT(setSplit()));

	menu->exec(sender->mapToGlobal(pt));
	delete menu;
}

void QtTermTCP::showContextMenuM(const QPoint &pt)				// Mon Window
{
	QTextEdit* sender = static_cast<QTextEdit*>(QObject::sender());

	QMenu *menu = sender->createStandardContextMenu();
	QAction * actSplit = new QAction("Set Monitor/Output Split", sender);

	menu->addAction(actSplit);

	splitY = pt.y();

	connect(actSplit, SIGNAL(triggered()), this, SLOT(setSplit()));

	menu->exec(sender->mapToGlobal(pt));
	delete menu;
}

void setMenus(int State)
{
	// Sets Connect, Disconnect and YAPP Send  enable flags to match connection state

	if (State)
	{
		connectMenu->setEnabled(false);
		discAction->setEnabled(true);
		YAPPSend->setEnabled(true);
	}
	else
	{
		connectMenu->setEnabled(true);
		discAction->setEnabled(false);
		YAPPSend->setEnabled(false);
	}
}


void QtTermTCP::tabSelected(int Current)
{
	Ui_ListenSession * Sess = NULL;

	if (Current < _sessions.size())
	{
		Sess = _sessions.at(Current);

		if (Sess == nullptr)
			return;

		ActiveSession = Sess;

		if (Sess->clientSocket || Sess->AGWSession)
		{
			connectMenu->setEnabled(false);
			discAction->setEnabled(true);
			YAPPSend->setEnabled(true);
		}
		else
		{
			connectMenu->setEnabled(true);
			discAction->setEnabled(false);
			YAPPSend->setEnabled(false);
		}

		// If a monitor Window, change Monitor config settings

		if (Sess->PortMonString[0])
		{
			char * ptr = (char *)malloc(1024);
			memcpy(ptr, Sess->PortMonString, 1024);

			int NumberofPorts = atoi((char *)&ptr[2]);
			char *p, *Context;
			char msg[80];
			int portnum;
			char delim[] = "|";

			// Remove old Monitor menu

			for (int i = 0; i < 32; i++)
			{
				SetPortMonLine(i, (char *)"", 0, 0);			// Set all hidden
			}

			p = strtok_s((char *)&ptr[2], delim, &Context);

			while (NumberofPorts--)
			{
				p = strtok_s(NULL, delim, &Context);
				if (p == NULL)
					break;

				portnum = atoi(p);

				sprintf(msg, "Port %s", p);

				if (Sess->portmask & (1 << (portnum - 1)))
					SetPortMonLine(portnum, msg, 1, 1);
				else
					SetPortMonLine(portnum, msg, 1, 0);
			}
			free(ptr);
		}
		return;
	}
}

void QtTermTCP::selFont()
{
	bool ok;

	QSettings settings("QtTermTCP.ini", QSettings::IniFormat);

	QFont font= QFontDialog::getFont(&ok,
		QFont(settings.value("FontFamily", "Courier New").value<QString>(),
			settings.value("PointSize", 10).toInt(),
			settings.value("Weight", 50).toInt()),
		this, "Select Font", QFontDialog::MonospacedFonts);

	if (ok)
	{
		Ui_ListenSession * Sess;

		for (int i = 0; i < _sessions.size(); ++i)
		{
			Sess = _sessions.at(i);
		
//		for (Ui_ListenSession * Sess : _sessions)
//		{
			if (Sess->termWindow)
				Sess->termWindow->setFont(font);

			if (Sess->inputWindow)
				Sess->inputWindow->setFont(font);

			if (Sess->monWindow)
				Sess->monWindow->setFont(font);
		}
	
		settings.setValue("FontFamily", font.family());
		settings.setValue("PointSize", font.pointSize());
		settings.setValue("Weight", font.weight());
	}
}

void QtTermTCP::SetupHosts()
{
	QAction * Act = static_cast<QAction*>(QObject::sender());
	int i;

	for (i = 0; i < MAXHOSTS; i++)
	{
		if (Act == actSetup[i])
			break;
	}

	if (i > 15)
		return;

	ConfigHost = i;

	TabDialog tabdialog(0);
	tabdialog.exec();
}

Ui_AGWConnectDkg * AGWConSess;
QDialog * MUI;

void QtTermTCP::AGWConaccept()
{
	QVariant Q;

	int port = AGWConSess->RadioPorts->currentRow();
	char CallFrom[32];
	char CallTo[32];
	char Via[128];

	strcpy(CallFrom, AGWConSess->CallFrom->text().toUpper().toUtf8());
	strcpy(CallTo, AGWConSess->CallTo->currentText().toUpper().toUtf8());
	strcpy(Via, AGWConSess->Digis->text().toUpper().toUtf8());

	// Add CallTo if not already in list

	if (AGWToCalls.contains(CallTo))
		AGWToCalls.removeOne(CallTo);

	AGWToCalls.insert(-1, CallTo);

	Send_AGW_C_Frame(ActiveSession, port, CallFrom, CallTo, Via, strlen(Via));

	MUI->close();
}


void QtTermTCP::Connect()
{
	QMdiSubWindow * UI;
	Ui_ListenSession * Sess = nullptr;
	QAction * Act = static_cast<QAction*>(QObject::sender());

	int i;

	for (i = 0; i < MAXHOSTS; i++)
	{
		if (Act == actHost[i])
			break;
	}

	if (i == 16)
	{
		AGWConSess = new(Ui_AGWConnectDkg);
		MUI = new(QDialog);
		QString str;
		int count;

		char * Context;
		char * ptr;

		AGWConSess->setupUi(MUI);

		AGWConSess->CallFrom->setText(AGWTermCall);

		AGWConSess->CallTo->addItems(AGWToCalls);


		if (AGWPortList)
		{
			char * Temp = strdup(AGWPortList);		// Need copy as strtok messes with it

			count = atoi(Temp);

			ptr = strtok_s(Temp, ";", &Context);

			for (int n = 0; n < count; n++)
			{
				ptr = strtok_s(NULL, ";", &Context);
				new QListWidgetItem(ptr, AGWConSess->RadioPorts);
			}

			free(Temp);
		}
	

	//	AGWSess->beaconPorts->setText(AGWBeaconPorts);

	//	AGWSess->Host->setText(AGWHost);

		str.setNum(AGWPortNum);
	
		connect(AGWConSess->okButton, SIGNAL(pressed()), this, SLOT(AGWConaccept()));

		MUI->show();

		return;
	}


	SavedHost = i;				// Iast used

	if (TermMode == MDI)
	{
		UI = mdiArea->activeSubWindow();

		for (i = 0; i < _sessions.size(); ++i)
		{
			Sess = _sessions.at(i);

			if (Sess->sw == UI)
				break;
		}

		if (i == _sessions.size())
			return;
	}

	else if (TermMode == Tabbed)
	{
		Sess = (Ui_ListenSession *)tabWidget->currentWidget();
	}

	else if (TermMode == Single)
		Sess = _sessions.at(0);

	if ((Sess == nullptr || Sess->SessionType & Listen))
		return;

	Sess->CurrentHost = SavedHost;

	// Set Monitor Params for this host

	sscanf(MonParams[Sess->CurrentHost], "%x %x %x %x %x %x",
		&Sess->portmask, &Sess->mtxparam, &Sess->mcomparam,
		&Sess->MonitorNODES, &Sess->MonitorColour, &Sess->monUI);

	MonTX->setChecked(Sess->mtxparam);
	MonSup->setChecked(Sess->mcomparam);
	MonUI->setChecked(Sess->monUI);
	MonNodes->setChecked(Sess->MonitorNODES);
	MonColour->setChecked(Sess->MonitorColour);

	// Remove old Monitor menu

	for (i = 0; i < 32; i++)
	{
		SetPortMonLine(i, (char *)"", 0, 0);			// Set all hidden
	}

	delete(Sess->clientSocket);

	Sess->clientSocket = new myTcpSocket();
	Sess->clientSocket->Sess = Sess;

	connect(Sess->clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
	connect(Sess->clientSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(Sess->clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

	Sess->clientSocket->connectToHost(&Host[Sess->CurrentHost][0], Port[Sess->CurrentHost]);
	return;
}


void QtTermTCP::Disconnect()
{
	QMdiSubWindow * UI;
	Ui_ListenSession * Sess = nullptr;

	if (TermMode == MDI)
	{
		int i;

		UI = mdiArea->activeSubWindow();

		for (i = 0; i < _sessions.size(); ++i)
		{
			Sess = _sessions.at(i);

			if (Sess->sw == UI)
				break;
		}

		if (i == _sessions.size())
			return;
	}

	else if (TermMode == Tabbed)
	{
		Sess = (Ui_ListenSession *)tabWidget->currentWidget();
	}

	else if (TermMode == Single)
		Sess = _sessions.at(0);

	// if AGW send a d frame else disconnect TCP session

	if (Sess->AGWSession)
		Send_AGW_Ds_Frame(Sess->AGWSession);
	else
	{
		if (Sess && Sess->clientSocket)
			Sess->clientSocket->disconnectFromHost();
	}
	return;
}


void QtTermTCP::doYAPPSend()
{
	QFileDialog dialog(this);
	QStringList fileNames;
	dialog.setFileMode(QFileDialog::AnyFile);

	QMdiSubWindow * UI;
	Ui_ListenSession * Sess = nullptr;
	int i;

	if (TermMode == MDI)
	{
		UI = mdiArea->activeSubWindow();

		for (i = 0; i < _sessions.size(); ++i)
		{
			Sess = _sessions.at(i);

			if (Sess->sw == UI)
				break;
		}

		if (i == _sessions.size())
			return;
	}

	else if (TermMode == Tabbed)
	{
		Sess = (Ui_ListenSession *)tabWidget->currentWidget();
	}

	else if (TermMode == Single)
		Sess = _sessions.at(0);

	if (Sess == nullptr)
		return;

	if ((Sess->SessionType & Mon))
	{
		// Turn off monitoring

		setTraceOff(Sess);
	}

	if (dialog.exec())
	{
		char FN[256];

		fileNames = dialog.selectedFiles();
		if (fileNames[0].length() < 256)
		{
			strcpy(FN, fileNames[0].toUtf8());
			YAPPSendFile(Sess, FN);
		}
	}
}

void QtTermTCP::doYAPPSetRX()
{
	QFileDialog dialog(this);
	QStringList fileNames;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setDirectory(YAPPPath);

	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
		if (fileNames[0].length() < 256)
			strcpy(YAPPPath, fileNames[0].toUtf8());

		SaveSettings();
	}
}

void QtTermTCP::menuChecked()
{
	QAction * Act = static_cast<QAction*>(QObject::sender());

	int state = Act->isChecked();
	int newTermMode = TermMode;
	int newSingleMode = singlemodeFormat;

	if (Act == TabSingle || Act == TabBoth || Act == TabMon)
	{
		// Tebbed Window format had changed

		int i = tabWidget->currentIndex();

		if (Act == TabSingle)
			TabType[i] = Term;
		else if (Act == TabBoth)
			TabType[i] = Term + Mon;
		else
			TabType[i] = Mon;

		QMessageBox msgBox;
		msgBox.setText("Tab Types will change next time program is started.");
		msgBox.exec();

	}

	if (Act == singleAct || Act == singleAct2 || Act == MDIAct || Act == tabbedAct)
	{
		// Term Mode had changed

		if (singleAct->isChecked())
		{
			newTermMode = Single;
			newSingleMode = Mon + Term;
		}
		else if (singleAct2->isChecked())
		{
			newTermMode = Single;
			newSingleMode = Term;
		}
		else if (MDIAct->isChecked())
			newTermMode = MDI;
		else if (tabbedAct->isChecked())
			newTermMode = Tabbed;

		if (newTermMode != TermMode || newSingleMode != singlemodeFormat)
		{
			QSettings settings("QtTermTCP.ini", QSettings::IniFormat);
			settings.setValue("TermMode", newTermMode);
			settings.setValue("singlemodeFormat", newSingleMode);
			QMessageBox msgBox;
			msgBox.setText("Presentation Mode will change next time program is started.");
			msgBox.exec();
		}

		return;
	}


	if (Act == MonTX)
		ActiveSession->mtxparam = state;
	else if (Act == MonSup)
		ActiveSession->mcomparam = state;
	else if (Act == MonUI)
		ActiveSession->monUI = state;
	else if (Act == MonNodes)
		ActiveSession->MonitorNODES = state;
	else if (Act == MonColour)
		ActiveSession->MonitorColour = state;
	else if (Act == actChatMode)
		ChatMode = state;
	else if (Act == actBells)
		Bells = state;
	else
	{
		// look for port entry

		for (int i = 0; i < MAXPORTS; i++)
		{
			if (Act == MonPort[i])
			{
				unsigned int mask = 0x1 << (i - 1);

				if (state)
					ActiveSession->portmask |= mask;
				else
					ActiveSession->portmask &= ~mask;
				break;
			}
		}
	}

	// Get active Session
/*

	QMdiSubWindow *SW = mdiArea->activeSubWindow();

	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);

//	for (Ui_ListenSession * Sess : _sessions)
//	{
		if (Sess->sw == SW)
		{
		*/

	if (ActiveSession->clientSocket && ActiveSession->SessionType & Mon)
		SendTraceOptions(ActiveSession);
	
	return;
}




void QtTermTCP::LDisconnect(Ui_ListenSession * LUI)
{
	if (LUI->clientSocket)
		LUI->clientSocket->disconnectFromHost();
}


void QtTermTCP::LreturnPressed(Ui_ListenSession * Sess)
{

	QByteArray stringData = Sess->inputWindow->text().toUtf8();

	// if multiline input (eg from copy/paste) replace LF with CR

	char * ptr;
	char * Msgptr;
	char * Msg;

	QScrollBar *scrollbar = Sess->termWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	if (scrollbarAtBottom)
		Sess->termWindow->moveCursor(QTextCursor::End);					// So we don't get blank lines

	// Stack it


	Sess->StackIndex = 0;

	if (Sess->KbdStack[49])
		free(Sess->KbdStack[49]);

	for (int i = 48; i >= 0; i--)
	{
		Sess->KbdStack[i + 1] = Sess->KbdStack[i];
	}

	Sess->KbdStack[0] = qstrdup(stringData.data());

	stringData.append('\n');

	Msgptr = stringData.data();

	LastWrite = time(NULL);				// Stop initial beep

	if (Sess->AGWSession)
	{
		// Terminal is in AGWPE mode - send as AGW frame

		while ((ptr = strchr(Msgptr, '\n')))
		{
			*ptr++ = 0;

			Msg = (char *)malloc(strlen(Msgptr) + 10);
			strcpy(Msg, Msgptr);
			strcat(Msg, "\r");

			AGW_AX25_data_in(Sess->AGWSession, 240, (unsigned char *)Msg, strlen(Msg));

			WritetoOutputWindowEx(Sess, (unsigned char *)Msg, strlen(Msg),
				Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 0);		// Black

//			Sess->termWindow->insertPlainText(Msg);

			free(Msg);

			Msgptr = ptr;
		}
	}
	else if (Sess->clientSocket && Sess->clientSocket->state() == QAbstractSocket::ConnectedState)
	{
		while ((ptr = strchr(Msgptr, '\n')))
		{
			*ptr++ = 0;

			Msg = (char *)malloc(strlen(Msgptr) + 10);
			strcpy(Msg, Msgptr);
			strcat(Msg, "\r");

			Sess->clientSocket->write(Msg);

			WritetoOutputWindowEx(Sess, (unsigned char *)Msg, strlen(Msg),
				Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 0);		// Black

//			Sess->termWindow->insertPlainText(Msg);

			free(Msg);

			Msgptr = ptr;
		}
	}
	else
	{
			// Not Connected 

		if(Sess->SessionType != Listen)
		{
			char Msg[] = "Connecting....\r";

			WritetoOutputWindowEx(Sess, (unsigned char *)Msg, strlen(Msg),
					Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 81);		// Red

			delete(Sess->clientSocket);

			Sess->clientSocket = new myTcpSocket();
			Sess->clientSocket->Sess = Sess;

			connect(Sess->clientSocket,  SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
			
			connect(Sess->clientSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
			connect(Sess->clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

			Sess->clientSocket->connectToHost(&Host[Sess->CurrentHost][0], Port[Sess->CurrentHost]);
		}
		else
		{
			char Msg[] = "Incoming Session - Can't Connect\r";

			WritetoOutputWindowEx(Sess, (unsigned char *)Msg, strlen(Msg),
				Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 81);		// Red

		}
	}

	if (scrollbarAtBottom)
		Sess->termWindow->moveCursor(QTextCursor::End);

	Sess->inputWindow->setText("");
}


void QtTermTCP::displayError(QAbstractSocket::SocketError socketError)
{
	myTcpSocket* sender = static_cast<myTcpSocket*>(QObject::sender());

	switch (socketError)
	{
	case QAbstractSocket::RemoteHostClosedError:
		break;

	case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The host was not found. Please check the "
				"host name and port settings."));
		break;

	case QAbstractSocket::ConnectionRefusedError:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The connection was refused by the peer."));
		break;

	default:
		QMessageBox::information(this, tr("QtTermTCP"),
			tr("The following error occurred: %1.")
			.arg(sender->errorString()));
	}

	connectMenu->setEnabled(true);
}

void QtTermTCP::readyRead()
{
	int Read;
	unsigned char Buffer[4096];
	myTcpSocket* Socket = static_cast<myTcpSocket*>(QObject::sender());

	Ui_ListenSession * Sess = (Ui_ListenSession *)Socket->Sess;

	// read the data from the socket

	Read = Socket->read((char *)Buffer, 4095);

	if (Read > 0)
	{
		Buffer[Read] = 0;

//		if (InputMode == 'Y')			// Yapp
//		{
//			QString myString = QString::fromUtf8((char*)Buffer, Read);
//			QByteArray ptr = myString.toLocal8Bit();
//			memcpy(Buffer, ptr.data(), ptr.length());
//			Read = ptr.length();
//		}

		ProcessReceivedData(Sess, Buffer, Read);

		QString myString = QString::fromUtf8((char*)Buffer);
//		qDebug() << myString;
	}
}

extern "C" int SocketSend(Ui_ListenSession * Sess, char * Buffer, int len)
{
	myTcpSocket *Socket = Sess->clientSocket;
	
	if (Socket && Socket->state() == QAbstractSocket::ConnectedState)
		return Socket->write(Buffer, len);
	
	return 0;
}

extern "C" int SocketFlush(Ui_ListenSession * Sess)
{
	myTcpSocket* Socket = Sess->clientSocket;

	if (Socket->state() == QAbstractSocket::ConnectedState)
		return Socket->flush();

	return 0;
}

extern "C" void Sleep(int ms)
{
	QThread::msleep(ms);
}

extern "C" void SetPortMonLine(int i, char * Text, int visible, int enabled)
{
	MonPort[i]->setText(Text);
	MonPort[i]->setVisible(visible);
	MonPort[i]->setChecked(enabled);
}

bool scrollbarAtBottom = 0;

extern "C" void WritetoOutputWindowEx(Ui_ListenSession * Sess, unsigned char * Buffer, int len, QTextEdit * termWindow, int * OutputSaveLen, char * OutputSave, int Colour);

extern "C" void WritetoOutputWindow(Ui_ListenSession * Sess, unsigned char * Buffer, int len)
{
	WritetoOutputWindowEx(Sess, Buffer, len, Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 23);
}


extern "C" void WritetoOutputWindowEx(Ui_ListenSession * Sess, unsigned char * Buffer, int len, QTextEdit * termWindow, int * OutputSaveLen, char * OutputSave, int Colour)
{
	char Copy[8192];
	char * ptr1, *ptr2;
	char Line[512];
	int num;

	if (termWindow == NULL)
		return;


	time_t NOW = time(NULL);

	// Beep if no output for a while

	if (AlertInterval && (NOW - LastWrite) > AlertInterval)
	{
		if (AlertBeep)
			Beep();
	}

	LastWrite = NOW;

	// Mustn't mess with original buffer

	memcpy(Copy, Buffer, len);

	Copy[len] = 0;

	ptr1 = Copy;

	// Write a line at a time so we can action colour chars

//		Buffer[Len] = 0;

	if (*OutputSaveLen)
	{
		// Have part line - append to it
		memcpy(&OutputSave[*OutputSaveLen], Copy, len);
		*OutputSaveLen += len;
		ptr1 = OutputSave;
		len = *OutputSaveLen;
		*OutputSaveLen = 0;

		// part line was written to screen so remove it

//		termWindow->setFocus();
		QTextCursor storeCursorPos = termWindow->textCursor();
		termWindow->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
		termWindow->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		termWindow->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
		termWindow->textCursor().removeSelectedText();
//		termWindow->textCursor().deletePreviousChar();
		termWindow->setTextCursor(storeCursorPos);

	}
	else
	{
		QScrollBar *scrollbar = termWindow->verticalScrollBar();
		scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

		if (scrollbarAtBottom)
			termWindow->moveCursor(QTextCursor::End);
	}

lineloop:

	if (len <= 0)
	{
		if (scrollbarAtBottom)
			termWindow->moveCursor(QTextCursor::End);

		return;
	}

	//	copy text to control a line at a time	

	ptr2 = (char *)memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		if (len > 8000)
			len = 8000;			// Should never get lines this long

		memmove(OutputSave, ptr1, len);
		*OutputSaveLen = len;

		// Write part line to screen

		memcpy(Line, ptr1, len);
		Line[len] = 0;

		if (Line[0] == 0x1b)			// Colour Escape
		{
			if (Sess->MonitorColour)
				termWindow->setTextColor(Colours[Line[1] - 10]);

			termWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
		}
		else
		{
			termWindow->setTextColor(Colours[23]);
			termWindow->insertPlainText(QString::fromUtf8((char*)Line));
		}

		*OutputSaveLen = 0;		// Test if we need to delete part line

		if (scrollbarAtBottom)
			termWindow->moveCursor(QTextCursor::End);

		return;
	}

	*(ptr2++) = 0;

	if (Bells)
	{
		char * ptr;

		do {
			ptr = (char *)memchr(ptr1, 7, len);

			if (ptr)
			{
				*(ptr) = 32;
				Beep();
			}
		} while (ptr);
	}

	num = ptr2 - ptr1 - 1;

	//		if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	memcpy(Line, ptr1, num);
	Line[num++] = 13;
	Line[num] = 0;

	if (Line[0] == 0x1b)			// Colour Escape
	{
		if (Sess->MonitorColour)
			termWindow->setTextColor(Colours[Line[1] - 10]);

		termWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
	}
	else
	{
		termWindow->setTextColor(Colours[Colour]);
		termWindow->insertPlainText(QString::fromUtf8((char*)Line));
	}

	len -= (ptr2 - ptr1);

	ptr1 = ptr2;

	goto lineloop;

	termWindow->setTextColor(Colours[23]);

}

extern "C" void WritetoMonWindow(Ui_ListenSession * Sess, unsigned char * Msg, int len)
{
	char * ptr1 = (char *)Msg, * ptr2;
	char Line[512];
	int num;

	QScrollBar *scrollbar = Sess->monWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));

	// Write a line at a time so we can action colour chars
		
//		Buffer[Len] = 0;

	if (scrollbarAtBottom)
		Sess->monWindow->moveCursor(QTextCursor::End);

	if (Sess->MonSaveLen)
	{
		// Have part line - append to it
		memcpy(&Sess->MonSave[Sess->MonSaveLen], Msg, len);
		Sess->MonSaveLen += len;
		ptr1 = Sess->MonSave;
		len = Sess->MonSaveLen;
		Sess->MonSaveLen = 0;
	}

lineloop:

	if (len <= 0)
	{
		if (scrollbarAtBottom)
			Sess->monWindow->moveCursor(QTextCursor::End);

		return;
	}

	//	copy text to control a line at a time	

	ptr2 = (char *)memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		memmove(Sess->MonSave, ptr1, len);
		Sess->MonSaveLen = len;
		return;
	}

	*(ptr2++) = 0;

//		if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	num = ptr2 - ptr1 - 1;

	memcpy(Line, ptr1, num);
	Line[num++] = 13;
	Line[num] = 0;

	if (Line[0] == 0x1b)			// Colour Escape
	{
		if (Sess->MonitorColour)
		{
			if (Line[1] == 17)
				Sess->monWindow->setTextColor(qRgb(0, 0, 192));
			else
				Sess->monWindow->setTextColor(QColor(qRgb(192, 0, 0)));
		}
		else
			Sess->monWindow->setTextColor(QColor("black"));


		Sess->monWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
	}
	else
	{
		Sess->monWindow->insertPlainText(QString::fromUtf8((char*)Line));
	}
	len -= (ptr2 - ptr1);

	ptr1 = ptr2;

	goto lineloop;

}


void GetSettings()
{
	QByteArray qb;
	int i;
	char Key[16];
	char Param[256];

	QSettings settings("QtTermTCP.ini", QSettings::IniFormat);

	// Get saved session definitions

	sessionList = strdup(settings.value("Sessions", "1|3, 0, 5, 5, 600, 800|").toString().toUtf8());


	for (i = 0; i < MAXHOSTS; i++)
	{
		sprintf(Key, "HostParams%d", i);

		qb = settings.value(Key).toByteArray();

		DecodeSettingsLine(i, qb.data());
	}

	Split = settings.value("Split", 50).toInt();
	ChatMode = settings.value("ChatMode", 1).toInt();
	Bells = settings.value("Bells", 1).toInt();
	SavedHost = settings.value("CurrentHost", 0).toInt();
	strcpy(YAPPPath, settings.value("YAPPPath", "").toString().toUtf8());

	listenPort = settings.value("listenPort", 8015).toInt();
	listenEnable = settings.value("listenEnable", false).toBool();
	strcpy(listenCText, settings.value("listenCText", "").toString().toUtf8());

	TermMode = settings.value("TermMode", 0).toInt();
	singlemodeFormat = settings.value("singlemodeFormat", Term + Mon).toInt();

	AGWEnable = settings.value("AGWEnable", 0).toInt();
	strcpy(AGWTermCall, settings.value("AGWTermCall", "").toString().toUtf8());
	strcpy(AGWBeaconDest, settings.value("AGWBeaconDest", "").toString().toUtf8());
	strcpy(AGWBeaconPath, settings.value("AGWBeaconPath", "").toString().toUtf8());
	AGWBeaconInterval = settings.value("AGWBeaconInterval", 0).toInt();
	strcpy(AGWBeaconPorts, settings.value("AGWBeaconPorts", "").toString().toUtf8());
	strcpy(AGWHost, settings.value("AGWHost", "127.0.0.1").toString().toUtf8());
	AGWPortNum = settings.value("AGWPort", 8000).toInt();
	AGWPaclen = settings.value("AGWPaclen", 80).toInt();
	AGWToCalls = settings.value("AGWToCalls","").toStringList();

	strcpy(Param, settings.value("TabType", "1 1 1 1 1 1 1 2 2").toString().toUtf8());
	sscanf(Param, "%d %d %d %d %d %d %d %d %d %d",
		&TabType[0], &TabType[1], &TabType[2], &TabType[3], &TabType[4],
		&TabType[5], &TabType[6], &TabType[7], &TabType[8], &TabType[9]);

}

extern "C" void SaveSettings()
{
	QSettings settings("QtTermTCP.ini", QSettings::IniFormat);
	int i;
	char Param[512];
	char Key[16];


	for (i = 0; i < MAXHOSTS; i++)
	{
		sprintf(Key, "HostParams%d", i);
		EncodeSettingsLine(i, Param);
		settings.setValue(Key, Param);
	}

	settings.setValue("Split", Split);
	settings.setValue("ChatMode", ChatMode);
	settings.setValue("Bells", Bells);
	settings.setValue("CurrentHost", SavedHost);

	settings.setValue("YAPPPath", YAPPPath);
	settings.setValue("listenPort", listenPort);
	settings.setValue("listenEnable", listenEnable);
	settings.setValue("listenCText", listenCText);

	// Save Sessions

	char SessionString[1024];
	int SessStringLen;

	SessStringLen = sprintf(SessionString, "%d|", _sessions.size());

	if (TermMode == MDI)
	{
		for (int i = 0; i < _sessions.size(); ++i)
		{
			Ui_ListenSession * Sess = _sessions.at(i);

			QRect r = Sess->sw->geometry();

			SessStringLen += sprintf(&SessionString[SessStringLen],
				"%d, %d, %d, %d, %d, %d|", Sess->SessionType, Sess->CurrentHost, r.top(), r.left(), r.bottom(), r.right());
		}

		settings.setValue("Sessions", SessionString);
	}

	settings.setValue("AGWEnable", AGWEnable);
	settings.setValue("AGWTermCall", AGWTermCall);
	settings.setValue("AGWBeaconDest", AGWBeaconDest);
	settings.setValue("AGWBeaconPath", AGWBeaconPath);
	settings.setValue("AGWBeaconInterval", AGWBeaconInterval);
	settings.setValue("AGWBeaconPorts", AGWBeaconPorts);
	settings.setValue("AGWHost", AGWHost);
	settings.setValue("AGWPort", AGWPortNum);
	settings.setValue("AGWPaclen", AGWPaclen);
	settings.setValue("AGWToCalls", AGWToCalls);

	sprintf(Param, "%d %d %d %d %d %d %d %d %d %d",
		TabType[0], TabType[1], TabType[2], TabType[3], TabType[4], TabType[5], TabType[6], TabType[7], TabType[8], TabType[9]);

	settings.setValue("TabType", Param);

	settings.sync();
}


QtTermTCP::~QtTermTCP()
{
	if (_server.isListening())
		_server.close();

	QSettings mysettings("QtTermTCP.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", saveGeometry());
	mysettings.setValue("windowState", saveState());

	SaveSettings();
}


void QtTermTCP::MyTimerSlot()
{
	// Runs every 10 seconds
	
	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);
		
//	for (Ui_ListenSession * Sess : _sessions)
//	{
		if (Sess == NULL || Sess->clientSocket == NULL)
			continue;
		
		if (Sess && Sess->clientSocket && Sess->clientSocket->state() != QAbstractSocket::ConnectedState)
			continue;

		if (!ChatMode)
			continue;

		Sess->SlowTimer++;

		if (Sess->SlowTimer > 54)				// About 9 mins
		{
			Sess->SlowTimer = 0;
			Sess->clientSocket->write("\0", 1);
		}
	}

	if (AGWEnable)
		AGWTimer();

}

extern "C" void Beep()
{
	QApplication::beep();
}

Ui_ListenPort  * Sess;

void QtTermTCP::myaccept()
{
	QString val = Sess->portNo->text();
    QByteArray qb = val.toLatin1();
	char * ptr = qb.data();

	listenPort = atoi(ptr);
	listenEnable = Sess->Enabled->isChecked();
	strcpy(listenCText, Sess->CText->toPlainText().toUtf8());

	while ((ptr = strchr(listenCText, '\n')))
		*ptr = '\r';

	if (_server.isListening())
		_server.close();

	if (listenEnable)
		_server.listen(QHostAddress::Any, listenPort);

	SaveSettings();
	MUI->close();
}

void QtTermTCP::ListenSlot()
{
	// This runs the Listen Configuration dialog

	Sess = new(Ui_ListenPort);
	MUI = new(QDialog);
	char portnum[16];
	sprintf(portnum, "%d", listenPort);
	QString portname(portnum);

 	Sess->setupUi(MUI);

	Sess->portNo->setText(portname);
	Sess->Enabled->setChecked(listenEnable);
	Sess->CText->setText(listenCText);

	connect(Sess->buttonBox, SIGNAL(accepted()), this, SLOT(myaccept()));

	MUI->show();
}

Ui_AGWParams  * AGWSess;

void QtTermTCP::AGWaccept()
{
	QVariant Q;

	int OldEnable = AGWEnable;
	int OldPort = AGWPortNum;

//	QString val = Sess->portNo->text();A
//	QByteArray qb = val.toLatin1();
//	char * ptr = qb.data();

	AGWEnable = AGWSess->AGWEnable->isChecked();

	strcpy(AGWTermCall, AGWSess->TermCall->text().toUtf8().toUpper());
	strcpy(AGWBeaconDest, AGWSess->beaconDest->text().toUtf8().toUpper());

	Q = AGWSess->beaconInterval->text();
	AGWBeaconInterval = Q.toInt();

	strcpy(AGWHost, AGWSess->Host->text().toUtf8());

	Q = AGWSess->Port->text();
	AGWPortNum = Q.toInt();

	SaveSettings();

	if (AGWEnable != OldEnable || AGWPortNum != OldPort)
	{
		// (re)start connection

		if (OldEnable && AGWSock && AGWSock->ConnectedState == QAbstractSocket::ConnectedState)
		{
			AGWSock->disconnectFromHost();
			Status1->setText("AGW Disconnected");
		}
		
		// AGWTimer will reopen connection

	}
	MUI->close();
}


void QtTermTCP::AGWSlot()
{
	// This runs the Listen Configuration dialog

	AGWSess = new(Ui_AGWParams);
	MUI = new(QDialog);
	QString str;

	AGWSess->setupUi(MUI);

	AGWSess->AGWEnable->setChecked(AGWEnable);
	AGWSess->TermCall->setText(AGWTermCall);
	AGWSess->beaconDest->setText(AGWBeaconDest);
	AGWSess->beaconPath->setText(AGWBeaconPath);

	str.setNum(AGWBeaconInterval);
	AGWSess->beaconInterval->setText(str);

	AGWSess->beaconPorts->setText(AGWBeaconPorts);

	AGWSess->Host->setText(AGWHost);

	str.setNum(AGWPortNum);
	AGWSess->Port->setText(str);

	str.setNum(AGWPaclen);
	AGWSess->Paclen->setText(str);

	connect(AGWSess->okButton, SIGNAL(pressed()), this, SLOT(AGWaccept()));

	MUI->show();
}

// This handles incoming connections

void QtTermTCP::onNewConnection()
{
	myTcpSocket *clientSocket = (myTcpSocket *)_server.nextPendingConnection();

	clientSocket->Sess = NULL;

	Ui_ListenSession * S;
	Ui_ListenSession  * Sess = NULL;

	char Title[128];
	int i = 0;

	QByteArray Host = clientSocket->peerAddress().toString().toUtf8();

	if (TermMode == MDI)
	{
		// See if an old session can be reused

		for (int i = 0; i < _sessions.size(); ++i)
		{
			S = _sessions.at(i);

			//	for (Ui_ListenSession * S: _sessions)
			//	{
			if ((S->SessionType & Listen) && S->clientSocket == NULL)
			{
				Sess = S;
				break;
			}
		}

		// Create a window if none found, else reuse old

		if (Sess == NULL)
		{
			Sess = newWindow(this, Listen);
		}

		QByteArray Host = clientSocket->peerAddress().toString().toUtf8();
	}
	else
	{
		// Single or MDI - look for free session

		for (i = 0; i < _sessions.size(); ++i)
		{
			S = _sessions.at(i);

			if (S->clientSocket == NULL)
			{
				Sess = S;
				break;
			}
		}

		if (Sess == NULL)
		{
			// Clear connection

			clientSocket->disconnectFromHost();
			return;
		}
	}

	_sockets.push_back(clientSocket);

	clientSocket->Sess = Sess;

	// See if any data from host - first msg should be callsign

	clientSocket->waitForReadyRead(1000);

	QByteArray datas = clientSocket->readAll();

	datas.chop(2);
	datas.append('\0');

	sprintf(Title, "Inward Connect from %s:%d Call " + datas,
		Host.data(), clientSocket->peerPort());

	if (TermMode == MDI)
	{
		Sess->setWindowTitle(Title);
	}
	else if (TermMode == Tabbed)
	{
		tabWidget->setTabText(i, datas.data());
	}
	else if (TermMode == Single)
		this->setWindowTitle(Title);

	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

	Sess->clientSocket = clientSocket;

	if (Sess->sw == ActiveSubWindow)
	{
		// We need to set Connect and Disconnect as the window is already active

		setMenus(true);
	}

	// Send CText if defined

	if (listenCText[0])
		Sess->clientSocket->write(listenCText);

	Beep();
}

void QtTermTCP::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	myTcpSocket* sender = static_cast<myTcpSocket*>(QObject::sender());
	Ui_ListenSession * Sess = (Ui_ListenSession *)sender->Sess;

	if (socketState == QAbstractSocket::UnconnectedState)
	{
		char Msg[] = "Disconnected\r";

		WritetoOutputWindowEx(Sess, (unsigned char *)Msg, strlen(Msg),
			Sess->termWindow, &Sess->OutputSaveLen, Sess->OutputSave, 81);		// Red

		if (TermMode == MDI)
		{
			if (Sess->SessionType == Mon)			// Mon Only
				Sess->setWindowTitle("Monitor Session Disconnected");
			else
				Sess->setWindowTitle("Disconnected");
		}
		else if (TermMode == Tabbed)
		{
			if (Sess->SessionType == Mon)			// Mon Only
				tabWidget->setTabText(Sess->Tab, "Monitor");
			else
			{
				char Label[16];
				sprintf(Label, "Sess %d", Sess->Tab + 1);
				tabWidget->setTabText(Sess->Tab, Label);
			}
		}
		else if (TermMode == Single)
		{
			if (Sess->SessionType == Mon)			// Mon Only
				this->setWindowTitle("Monitor Session Disconnected");
			else
				this->setWindowTitle("Disconnected");
		}

		Sess->PortMonString[0] = 0;
		
//		delete(Sess->clientSocket);
		Sess->clientSocket = NULL;

		discAction->setEnabled(false);

		if ((Sess->SessionType & Listen))
			_sockets.removeOne(sender);
		else
		{
			connectMenu->setEnabled(true);
			YAPPSend->setEnabled(false);
		}
	}
	else if (socketState == QAbstractSocket::ConnectedState)
	{
		char Signon[256];
		char Title[128];

		// only seems to be triggered for outward connect

		sprintf(Signon, "%s\r%s\rBPQTERMTCP\r", UserName[Sess->CurrentHost], Password[Sess->CurrentHost]);

		Sess->clientSocket->write(Signon);

		discAction->setEnabled(true);
		YAPPSend->setEnabled(true);
		connectMenu->setEnabled(false);

		SendTraceOptions(Sess);

		Sess->InputMode = 0;
		Sess->SlowTimer = 0;
		Sess->MonData = 0;
		Sess->OutputSaveLen = 0;			// Clear any part line
		Sess->MonSaveLen = 0;			// Clear any part line

		if (Sess->SessionType == Mon)		// Mon Only
			sprintf(Title, "Monitor Session Connected to %s", Host[Sess->CurrentHost]);
		else
			sprintf(Title, "Connected to %s", Host[Sess->CurrentHost]);

		if (TermMode == MDI)
			Sess->setWindowTitle(Title);
		else if (TermMode == Tabbed)
			tabWidget->setTabText(tabWidget->currentIndex(), Host[Sess->CurrentHost]);
		else if (TermMode == Single)
			this->setWindowTitle(Title);
	}
}

void QtTermTCP::updateWindowMenu()
{
	if (TermMode == MDI)
	{
		windowMenu->clear();
		windowMenu->addAction(newTermAct);
		windowMenu->addAction(newMonAct);
		windowMenu->addAction(newCombinedAct);
		windowMenu->addSeparator();
		windowMenu->addAction(closeAct);
		windowMenu->addAction(closeAllAct);
		windowMenu->addSeparator();
		windowMenu->addAction(tileAct);
		windowMenu->addAction(cascadeAct);
		windowMenu->addSeparator();
		windowMenu->addAction(nextAct);
		windowMenu->addAction(previousAct);
		windowMenu->addAction(quitAction);
		windowMenu->addAction(windowMenuSeparatorAct);

		QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
		windowMenuSeparatorAct->setVisible(!windows.isEmpty());

		Ui_ListenSession * Sess;

		for (int i = 0; i < _sessions.size(); ++i)
		{
			Sess = _sessions.at(i);
			Sess->actActivate = windowMenu->addAction(Sess->sw->windowTitle());
			QAction::connect(Sess->actActivate, SIGNAL(triggered()), this, SLOT(actActivate()));
			Sess->actActivate->setCheckable(true);
			Sess->actActivate->setChecked(ActiveSubWindow == Sess->sw);
		}
	}
	else if (TermMode == Tabbed)
	{
		windowMenu->clear();

		Ui_ListenSession * Sess = (Ui_ListenSession *)tabWidget->currentWidget();

		QActionGroup * termGroup = new QActionGroup(this);

		delete(TabSingle);
		delete(TabBoth);
		delete(TabMon);

		TabSingle = setupMenuLine(nullptr, (char *)"Terminal Only", this, (Sess->SessionType == Term));
		TabBoth = setupMenuLine(nullptr, (char *)"Terminal + Monitor", this, (Sess->SessionType == Term + Mon));
		TabMon = setupMenuLine(nullptr, (char *)"Monitor Only", this, (Sess->SessionType == Mon));

		termGroup->addAction(TabSingle);
		termGroup->addAction(TabBoth);
		termGroup->addAction(TabMon);

		windowMenu->addAction(TabSingle);
		windowMenu->addAction(TabBoth);
		windowMenu->addAction(TabMon);

	}
}

Ui_ListenSession::~Ui_ListenSession()
{
//	subwindow = NULL;
}

extern "C" void setTraceOff(Ui_ListenSession * Sess)
{
	if ((Sess->SessionType & Mon) == 0)
		return;					// Not Monitor

	char Buffer[80];
	int Len = sprintf(Buffer, "\\\\\\\\0 0 0 0 0 0 0 0\r");

	SocketFlush(Sess);
	SocketSend(Sess, Buffer, Len);
	SocketFlush(Sess);
}


extern "C" void SendTraceOptions(Ui_ListenSession * Sess)
{
	if ((Sess->SessionType & Mon) == 0)
		return;					// Not Monitor
	
	char Buffer[80];
	int Len = sprintf(Buffer, "\\\\\\\\%x %x %x %x %x %x %x %x\r", Sess->portmask, Sess->mtxparam, Sess->mcomparam, 
		Sess->MonitorNODES, Sess->MonitorColour, Sess->monUI, 1, 1);

	strcpy(&MonParams[Sess->CurrentHost][0], &Buffer[4]);
	SaveSettings();
	SocketFlush(Sess);
	SocketSend(Sess, Buffer, Len);
	SocketFlush(Sess);
}

void QtTermTCP::doNewTerm()
{
	newWindow(this, Term);
}

void QtTermTCP::doNewMon()
{
	newWindow(this, Mon);
}

void QtTermTCP::doNewCombined()
{
	newWindow(this, Term + Mon);
}

void QtTermTCP::doCascade()
{
	// Qt Cascade Minimizes windows so do it ourselves

	int x = 0, y = 0;

	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);

		Sess->sw->move(x, y);
		x += 14;
		y += 30;
	}
}

void QtTermTCP::actActivate()
{
	QAction * sender = static_cast<QAction*>(QObject::sender());

	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);

		if (Sess->actActivate == sender)
		{
			mdiArea->setActiveSubWindow(Sess->sw);
			return;
		}
	}
}


void QtTermTCP::xon_mdiArea_changed()
{
	// This is triggered when the Active MDI window changes
	// and is used to enable/disable Connect, Disconnect and YAPP Send

	
	QMdiSubWindow *SW = mdiArea->activeSubWindow();

	// Dont waste time if not changed

	if (ActiveSubWindow == SW)
		return;

	ActiveSubWindow = SW;

	Ui_ListenSession * Sess;

	for (int i = 0; i < _sessions.size(); ++i)
	{
		Sess = _sessions.at(i);
		
//	for (Ui_ListenSession * Sess : _sessions)
//	{
		if (Sess->sw == SW)
		{
			ActiveSession = Sess;

			if (Sess->clientSocket && Sess->clientSocket->state() == QAbstractSocket::ConnectedState)
			{
				discAction->setEnabled(true);
				YAPPSend->setEnabled(true);
				connectMenu->setEnabled(false);
			}
			else
			{
				// Not connected

				discAction->setEnabled(false);
				YAPPSend->setEnabled(false);
				
				if ((Sess->SessionType & Listen))		// Listen Sessions can't connect
					connectMenu->setEnabled(false);
				else
					connectMenu->setEnabled(true);
			}

			// If a monitor Window, change Monitor config settings

			if (Sess->PortMonString[0])
			{
				char * ptr = (char *)malloc(1024);
				memcpy(ptr, Sess->PortMonString, 1024);

				int NumberofPorts = atoi((char *)&ptr[2]);
				char *p, *Context;
				char msg[80];
				int portnum;
				char delim[] = "|";

				// Remove old Monitor menu

				for (int i = 0; i < 32; i++)
				{
					SetPortMonLine(i, (char *)"", 0, 0);			// Set all hidden
				}

				p = strtok_s((char *)&ptr[2], delim, &Context);

				while (NumberofPorts--)
				{
					p = strtok_s(NULL, delim, &Context);
					if (p == NULL)
						break;

					portnum = atoi(p);

					sprintf(msg, "Port %s", p);

					if (Sess->portmask & (1 << (portnum - 1)))
						SetPortMonLine(portnum, msg, 1, 1);
					else
						SetPortMonLine(portnum, msg, 1, 0);
				}
				free(ptr);
			}

			return;
		}
	}
}
