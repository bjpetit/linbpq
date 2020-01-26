#include "QtTermTCP.h"
#include "TabDialog.h"
#include "QMessageBox"
#include "QTimer"
#include "QSettings"
#include "QThread"
#include <QFontDialog>
#include <QScrollBar>

#define VersionString "0.0.0.5"

#define MAXHOSTS 16
#define MAXPORTS 32

extern "C" char Host[MAXHOSTS + 1][100];
extern "C" int Port[MAXHOSTS + 1];
extern "C" char UserName[MAXHOSTS + 1][80];
extern "C" char Password[MAXHOSTS + 1][80];
extern "C" char MonParams[MAXHOSTS + 1][80];

extern "C" void EncodeSettingsLine(int n, char * String);
extern "C" void DecodeSettingsLine(int n, char * String);
extern "C" void WritetoOutputWindow(unsigned char * Buffer, int Len);
extern "C" void WritetoMonWindow(char * Buffer, int Len);
extern "C" void ProcessReceivedData(unsigned char * Buffer, int len);
extern "C" void SendTraceOptions();
extern "C" void SetPortMonLine(int i, char * Text, int visible, int enabled);
extern "C" void SaveSettings();

extern "C" int portmask;
extern "C" int mtxparam;
extern "C" int mcomparam;
extern "C" int monUI;
extern "C" int MonitorNODES;
extern "C" int MonitorColour;
extern "C" int ChatMode;

extern "C" int InputMode;
extern "C" int SlowTimer;
extern "C" int MonData;

void menuChecked(QAction * Act);

void GetSettings();

// These widgets defined here as they are accessed from outside the framework

QTextEdit *monWindow;
QTextEdit *termWindow;
QLineEdit *inputWindow;
QTcpSocket *tcpSocket = nullptr;

QAction *actHost[16];
QAction *actSetup[16];

QMenu *monitorMenu; 
QAction *MonTX;
QAction *MonSup;
QAction *MonNodes;
QAction *MonUI;
QAction *MonColour;
QAction *MonPort[32];
QAction *actChatMode;
QAction *actFonts;
QAction *actSplit;

extern "C" int CurrentHost;

int ConfigHost = 0;

char * KbdStack[50] = { NULL };
int StackIndex = 0;

int Split = 50;				// Mon/Term size split

int termX;

void resizeWindow(QRect r)
{
	int H, Mid, monHeight, termHeight, Width;

	H = r.height() - 20;
	Width = r.width() - 10;

	// Calc Positions of window

	Mid = (H * Split) / 100;

	monHeight = Mid - 38;
	termX = Mid;
	termHeight = H - Mid - 23;

	Mid += 7;

	monWindow->setGeometry(QRect(5, 38, Width, monHeight));
	termWindow->setGeometry(QRect(5, Mid, Width, termHeight));
	inputWindow->setGeometry(QRect(5, H - 9, Width, 25));

	//	splitter->setGeometry(QRect(10, 10, A * 2	 - 20, W));

}

void QtTermTCP::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	resizeWindow(geometry());

}


bool QtTermTCP::eventFilter(QObject* obj, QEvent *event)
{
	if (obj == inputWindow)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
	
			if (keyEvent->key() == Qt::Key_Up)
			{
				// Scroll up

				if (KbdStack[StackIndex] == NULL)
					return true;

				inputWindow->setText(KbdStack[StackIndex]);
				inputWindow->cursorForward(strlen(KbdStack[StackIndex]));
	
				StackIndex++;
				if (StackIndex == 50)
					StackIndex = 49;

				return true;
			}
			else if (keyEvent->key() == Qt::Key_Down)
			{
				// Scroll down

				if (StackIndex == 0)
				{
					inputWindow->setText("");
					return true;
				}

				StackIndex--;

				if (KbdStack[StackIndex - 1])
				{
					inputWindow->setText(KbdStack[StackIndex - 1]);
					inputWindow->cursorForward(strlen(KbdStack[StackIndex - 1]));
				}
				else
					inputWindow->setText("");

				return true;
			}
		}
		return false;
	}
	return QMainWindow::eventFilter(obj, event);
}
	
QAction * setupMenuLine(QMenu * Menu, char * Label, QObject * parent, int State)
{
	QAction * Mon = new QAction(Label, parent);
	Menu->addAction(Mon);

	Mon->setCheckable(true);
	if (State)
		Mon->setChecked(true);

//	parent->connect(Mon, SIGNAL(triggered()), parent, SLOT(menuChecked()));

	parent->connect(Mon, &QAction::triggered, parent, [=] { menuChecked(Mon); });

	return Mon;
}

QtTermTCP::QtTermTCP(QWidget *parent)
	: QMainWindow(parent)
{
	int i;

	ui.setupUi(this);

	tcpSocket = new QTcpSocket(this);

	GetSettings();

	connectMenu = ui.menuBar->addMenu(tr("Connect"));
	
	for (i = 0; i < MAXHOSTS; i++)
	{
		actHost[i] = new QAction(Host[i], this);
		connectMenu->addAction(actHost[i]);
		connect(actHost[i], &QAction::triggered, this, [=] { Connect(i); });
	}

	discAction = ui.menuBar->addAction("Disconnect");
	connect(discAction, SIGNAL(triggered()), this, SLOT(Disconnect()));

	discAction->setEnabled(false);

	setupMenu = ui.menuBar->addMenu(tr("Setup"));

	hostsubMenu = setupMenu->addMenu("Setup Hosts");

	for (i = 0; i < MAXHOSTS; i++)
	{
		actSetup[i] = new QAction(Host[i], this);
		hostsubMenu->addAction(actSetup[i]);
		connect(actSetup[i], &QAction::triggered, this, [=] {SetupHosts(i); });
	}

	actFonts = new QAction("Setup Fonts", this);
	setupMenu->addAction(actFonts);

	connect(actFonts, &QAction::triggered, this, [=] {selFont(); });

	actChatMode = setupMenuLine(setupMenu, "Chat Terminal Mode", this, ChatMode);

	monitorMenu = ui.menuBar->addMenu(tr("Monitor"));

	MonTX = setupMenuLine(monitorMenu, "Monitor TX", this, mtxparam);
	MonSup = setupMenuLine(monitorMenu, "Monitor Supervisory", this, mcomparam);
	MonUI = setupMenuLine(monitorMenu, "Only Monitor UI Frames", this, monUI);
	MonNodes = setupMenuLine(monitorMenu, "Monitor NODES Broadcasts", this, MonitorNODES);
	MonColour = setupMenuLine(monitorMenu, "Enable Colour", this, MonitorColour);

	for (i = 0; i < MAXPORTS; i++)
	{
		MonPort[i] = setupMenuLine(monitorMenu, "Port", this, 0);
		MonPort[i]->setVisible(false);
	}

//	QSplitter *splitter = new QSplitter(this);
//	splitter->setOrientation(Qt::Vertical);

//	splitter->setGeometry(QRect(10, 10, 770, 700));
	monWindow = new QTextEdit(this);
	monWindow->setReadOnly(1);

	monWindow->setGeometry(QRect(0, 0, 770, 300));

	termWindow = new QTextEdit(this);
	termWindow->setReadOnly(1);
	
	termWindow->setGeometry(QRect(0, 0, 770, 300));

	inputWindow = new QLineEdit(this);
	inputWindow->setGeometry(QRect(0, 0, 770, 650));
	inputWindow->installEventFilter(this);

	// Add Custom Menu to set Mon/Term Split

	monWindow->setContextMenuPolicy(Qt::CustomContextMenu);
	termWindow->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(monWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(showContextMenuM(const QPoint &)));

	connect(termWindow, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(showContextMenuT(const QPoint &)));

	connect(termWindow, SIGNAL(selectionChanged()),
		this, SLOT(ontermselectionChanged()));

	connect(monWindow, SIGNAL(selectionChanged()),
		this, SLOT(onmonselectionChanged()));

	actSplit= new QAction("Set Monitor/Output Split", this);
	
	QSettings settings("G8BPQ", "QT_TERM_TCP");

	QFont font = QFont(settings.value("FontFamily", "Courier").value<QString>(), settings.value("PointSize", 10).toInt());
	monWindow->setFont(font);
	termWindow->setFont(font);
	inputWindow->setFont(font);

	//splitter->addWidget(monWindow);
	//splitter->addWidget(termWindow);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(10000);

	connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &QtTermTCP::displayError);
	connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
	connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(tcpSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));

	connect(inputWindow, SIGNAL(textChanged(const QString &)), this, SLOT(inputChanged(const QString &)));
	connect(inputWindow, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

}
int splitY;					// Value when menu added


void QtTermTCP::setSplit()
{
	QRect Size = geometry();
	int y = Size.height() - 50;

	// y is height of whole windom. splitX is new split position
	// So split = 100 * splitx/x

	qDebug() << splitY;
	qDebug() << y;

	Split = (splitY * 100) / y;
	qDebug() << Split;

	if (Split < 10)
		Split = 10;
	else if (Split > 90)
		Split = 90;

	resizeWindow(geometry());

}

void QtTermTCP::ontermselectionChanged()
{
	termWindow->copy();
}


void QtTermTCP::onmonselectionChanged()
{
	monWindow->copy();
}


void QtTermTCP::showContextMenuT(const QPoint &pt)				// Term Window
{
	QMenu *menu = termWindow->createStandardContextMenu();
	menu->addAction(actSplit);

	splitY = pt.y() + termX;

	qDebug() << "Term";
	qDebug() << splitY;

	connect(actSplit, &QAction::triggered, this, [=] {setSplit(); });
	menu->exec(termWindow->mapToGlobal(pt));
	delete menu;
}

void QtTermTCP::showContextMenuM(const QPoint &pt)				// Mon Window
{
	QMenu *menu = monWindow->createStandardContextMenu();
	menu->addAction(actSplit);

	splitY = pt.y();

	qDebug() << "Mon";
	qDebug() << splitY;

	connect(actSplit, &QAction::triggered, this, [=] {setSplit(); });
	menu->exec(monWindow->mapToGlobal(pt));
	delete menu;
}


void QtTermTCP::selFont()
{
	bool ok;
	QFont font= QFontDialog::getFont(&ok, monWindow->font(), this, "Title", QFontDialog::MonospacedFonts);

	if (ok)
	{
		monWindow->setFont(font);
		termWindow->setFont(font);
		inputWindow->setFont(font);
	
		QSettings settings("G8BPQ", "QT_TERM_TCP");

		settings.setValue("FontFamily", font.family());
		settings.setValue("PointSize", font.pointSize());
	}
}

void QtTermTCP::SetupHosts(int i)
{
	ConfigHost = i;

	TabDialog tabdialog(0);
	tabdialog.exec();
}

void QtTermTCP::Connect(int i)
{
	CurrentHost = i;

	// Set Monitor Params for this host

	sscanf(MonParams[CurrentHost], "%x %x %x %x %x %x",
		&portmask, &mtxparam, &mcomparam, &MonitorNODES, &MonitorColour, &monUI);

	MonTX->setChecked(mtxparam);
	MonSup->setChecked(mcomparam);
	MonUI->setChecked(monUI);
	MonNodes->setChecked(MonitorNODES);
	MonColour->setChecked(MonitorColour);

	// Remove old Monitor menu

	for (i = 0; i < 32; i++)
	{
		SetPortMonLine(i, "", 0, 0);			// Set all hidden
	}

	tcpSocket->abort();
	tcpSocket->connectToHost(Host[CurrentHost], Port[CurrentHost]);
}

void QtTermTCP::Disconnect()
{
	tcpSocket->disconnectFromHost();
}

void menuChecked(QAction * Act)
{
	int state = Act->isChecked();
	
	if (Act == MonTX)
		mtxparam = state;
	else if (Act == MonSup)
		mcomparam = state;
	else if (Act == MonUI)
		monUI = state;
	else if (Act == MonNodes)
		MonitorNODES = state;
	else if (Act == MonColour)
		MonitorColour = state;
	else if (Act == actChatMode)
		ChatMode = state;
	else
	{
		// look for port entry

		for (int i = 0; i < MAXPORTS; i++)
		{
			if (Act == MonPort[i])
			{
				unsigned int mask = 0x1 << (i - 1);

				if (state)
					portmask |= mask;
				else
					portmask &= ~mask;
				break;
			}
		}
	}

	SendTraceOptions();
}

void QtTermTCP::inputChanged(const QString &)
{
}

void QtTermTCP::returnPressed()
{
	QByteArray stringData = inputWindow->text().toUtf8();

	// Stack it

	StackIndex = 0;

	if (KbdStack[49])
		free(KbdStack[49]);

	for (int i = 48; i >= 0; i--)
	{
		KbdStack[i + 1] = KbdStack[i];
	}

	KbdStack[0] = qstrdup(stringData.data());

	stringData.append('\r');

	QScrollBar *scrollbar = monWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));
	int scrollbarPrevValue = scrollbar->value();
	int max = scrollbar->maximum();

	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);					// So we don't get blank lines

	if (tcpSocket->state() == QAbstractSocket::ConnectedState)
	{
		tcpSocket->write(stringData);
		termWindow->setTextColor(QColor("black"));
		termWindow->insertPlainText(stringData);
	}
	else
	{
		// Not Connected 

		termWindow->setTextColor(QColor("red"));
		termWindow->insertPlainText("Connecting....\r");

		tcpSocket->abort();
		tcpSocket->connectToHost(Host[CurrentHost], Port[CurrentHost]);
	}
	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);

	inputWindow->setText("");
}



void QtTermTCP::displayError(QAbstractSocket::SocketError socketError)
{
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
			.arg(tcpSocket->errorString()));
	}

	connectMenu->setEnabled(true);
}

void QtTermTCP::connected()
{
	char Signon[256];
	char Title[128];

	sprintf(Signon, "%s\r%s\rBPQTERMTCP\r", UserName[CurrentHost], Password[CurrentHost]);

	tcpSocket->write(Signon);

	discAction->setEnabled(true);
	connectMenu->setEnabled(false);

	SendTraceOptions();

	InputMode = 0;
	SlowTimer = 0;
	MonData = 0;

	sprintf(Title, "QtTermTCP Version %s - Connected to %s", VersionString, Host[CurrentHost]);

	this->setWindowTitle(Title);
}

void QtTermTCP::disconnected()
{
	char Title[128];

	termWindow->setTextColor(QColor("red"));
	termWindow->append("*** Disconnected\r");
	termWindow->moveCursor(QTextCursor::End); 
	termWindow->moveCursor(QTextCursor::End);
	connectMenu->setEnabled(true);

	sprintf(Title, "QtTermTCP Version %s - Disonnected", VersionString);

	this->setWindowTitle(Title);
}

void QtTermTCP::bytesWritten(qint64 bytes)
{
	//qDebug() << bytes << " bytes written...";
}

void QtTermTCP::readyRead()
{
	int Read;
	unsigned char Buffer[4096];

	// read the data from the socket

	Read = tcpSocket->read((char *)Buffer, 4095);

	if (Read > 0)
	{
		Buffer[Read] = 0;

		ProcessReceivedData(Buffer, Read);

//		QString myString = QString::fromUtf8((char*)Buffer);
//		qDebug() << myString;
	}
}

extern "C" int SocketSend(char * Buffer, int len)
{
	if (tcpSocket->state() == QAbstractSocket::ConnectedState)
		tcpSocket->write(Buffer, len);
	
	return 0;
}

extern "C" void Sleep(int ms)
{
//	this->msleep(ms);
}

extern "C" void SetPortMonLine(int i, char * Text, int visible, int enabled)
{
	MonPort[i]->setText(Text);
	MonPort[i]->setVisible(visible);
	MonPort[i]->setChecked(enabled);
}

extern "C" void WritetoOutputWindow(unsigned char * Buffer, int Len)
{
	char Copy[4097];

	// Mustn't mess with original buffer

	memcpy(Copy, Buffer, Len);

	Copy[Len] = 0;

	QScrollBar *scrollbar = termWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));
	int scrollbarPrevValue = scrollbar->value();
	int max = scrollbar->maximum();


	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);
	termWindow->setTextColor(QColor("blue"));
	termWindow->insertPlainText(QString::fromUtf8(Copy));
	if (scrollbarAtBottom)
		termWindow->moveCursor(QTextCursor::End);
}

char MonSave[2000];
int MonSaveLen;


extern "C" void WritetoMonWindow(char * Msg, int len)
{
	char * ptr1 = Msg, * ptr2;
	char Line[512];
	int num;

	QScrollBar *scrollbar = monWindow->verticalScrollBar();
	bool scrollbarAtBottom = (scrollbar->value() >= (scrollbar->maximum() - 4));
	int scrollbarPrevValue = scrollbar->value();
	int max = scrollbar->maximum();

	// Write a line at a time so we can action colour chars
		
//		Buffer[Len] = 0;

	if (scrollbarAtBottom)
		monWindow->moveCursor(QTextCursor::End);	

	if (MonSaveLen)
	{
		// Have part line - append to it
		memcpy(&MonSave[MonSaveLen], Msg, len);
		MonSaveLen += len;
		ptr1 = MonSave;
		len = MonSaveLen;
		MonSaveLen = 0;
	}

lineloop:

	if (len <= 0)
	{
		if (scrollbarAtBottom)
			monWindow->moveCursor(QTextCursor::End);

		return;
	}

	//	copy text to control a line at a time	

	ptr2 = (char *)memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		memmove(MonSave, ptr1, len);
		MonSaveLen = len;
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
		if (MonitorColour)
		{
			if (Line[1] == 17)
				monWindow->setTextColor(QColor("blue"));
			else
				monWindow->setTextColor(QColor("red"));
		}
		else
			monWindow->setTextColor(QColor("black"));


		monWindow->insertPlainText(QString::fromUtf8((char*)&Line[2]));
	}
	else
	{
		monWindow->insertPlainText(QString::fromUtf8((char*)Line));
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

	QSettings settings("G8BPQ", "QT_TERM_TCP");

	for (i = 0; i < MAXHOSTS; i++)
	{
		sprintf(Key, "HostParams%d", i);

		qb = settings.value(Key).toByteArray();

		DecodeSettingsLine(i, qb.data());
	}

	Split = settings.value("Split", 50).toInt();
	ChatMode = settings.value("ChatMode", 1).toInt();
	CurrentHost = settings.value("CurrentHost", 0).toInt();
}

extern "C" void SaveSettings()
{
	QSettings settings("G8BPQ", "QT_TERM_TCP");
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
	settings.setValue("CurrentHost", CurrentHost);

}


QtTermTCP::~QtTermTCP()
{
	SaveSettings();
}


void QtTermTCP::MyTimerSlot()
{
	// Runs every 10 seconds

	if (tcpSocket->state() == !QAbstractSocket::ConnectedState)
		return;

	if (!ChatMode)
		return;

	SlowTimer++;

	if (SlowTimer > 54)				// About 9 mins
	{
		SlowTimer = 0;
		tcpSocket->write("\0", 1);
	}
}


