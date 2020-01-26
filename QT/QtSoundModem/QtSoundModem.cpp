#include "QtSoundModem.h"
#include <qheaderview.h>

int ModemA = 2;
int ModemB = 2;
int FreqA = 1500;
int FreqB = 1500;
int DCD = 50;

void QtSoundModem::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	QRect r = geometry();

	int A, B, W;

	A = r.height() - 200;
	B = r.height() - 100;
	W = r.width();

	// Calc Positions of Waterfalls

	ui.WaterfallA->setGeometry(QRect(0, A, W, 80));
	ui.WaterfallB->setGeometry(QRect(0, B, W, 80));
	m_pTableWidget->setGeometry(QRect(0, A - 150, W, 150));



}
QtSoundModem::QtSoundModem(QWidget *parent): QMainWindow(parent)
{
	ui.setupUi(this);

	m_pTableWidget = new QTableWidget(this);
	m_pTableWidget->setGeometry(QRect(0, 250, 770, 200));

	m_pTableWidget->verticalHeader()->setVisible(false);
	m_pTableWidget->verticalHeader()->setDefaultSectionSize(20);
	m_pTableWidget->horizontalHeader()->setDefaultSectionSize(68);
	m_pTableWidget->setRowCount(1);
	m_pTableWidget->setColumnCount(11);
	m_TableHeader << "MyCall" << "DestCall" << "Status" << "Sent pkts" << "Sent Bytes" << "Rcvd pkts" << "Rcvd bytes" << "Rcvd FC" << "CPS TX" << "CPS RX" << "Direction";


	m_pTableWidget->setHorizontalHeaderLabels(m_TableHeader);

	ui.modeA->addItem("New item");

	// Inline handlers as they are pretty small

	connect(ui.modeA, QOverload<int>::of(&QComboBox::currentIndexChanged),
	[=](int index)
	{
		ModemA = ui.modeA->currentIndex();
	});


	connect(ui.modeB, QOverload<int>::of(&QComboBox::currentIndexChanged),
	
	[=]	(int index)
	{
		ModemB = ui.modeA->currentIndex();
	});

	connect(ui.centerA, QOverload<int>::of(&QSpinBox::valueChanged),
		
	[=](int i)
	{
		if (i > 300)
			FreqA = i;
	});

	connect(ui.centerB, QOverload<int>::of(&QSpinBox::valueChanged),

	[=](int i)
	{
		if (i > 300)
			 FreqB = i;
	});

	connect(ui.DCDSlider, QOverload<int>::of(&QSlider::sliderMoved),

		[=](int i)
	{
		DCD = i;
	});


}
