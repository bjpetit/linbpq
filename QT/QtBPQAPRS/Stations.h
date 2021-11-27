
#ifndef UI_STATIONS_H
#define UI_STATIONS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

class StationDialog : public QDialog
{
	Q_OBJECT

private:
	void closeEvent(QCloseEvent *event);

public:
	explicit StationDialog(QWidget *parent = 0);
	~StationDialog();
	
	QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QTableWidget *tableWidget;

	void setupUi(QDialog *Dialog)
	{

		if (Dialog->objectName().isEmpty())
			Dialog->setObjectName(QString::fromUtf8("Dialog"));
		Dialog->resize(786, 614);
		scrollArea = new QScrollArea(Dialog);
		scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
		scrollArea->setGeometry(QRect(2, 2, 776, 606));
		scrollArea->setWidgetResizable(true);
		scrollAreaWidgetContents = new QWidget();
		scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
		scrollAreaWidgetContents->setGeometry(QRect(0, 0, 774, 604));
		tableWidget = new QTableWidget(scrollAreaWidgetContents);
		if (tableWidget->columnCount() < 7)
			tableWidget->setColumnCount(7);
		if (tableWidget->rowCount() < 100)
			tableWidget->setRowCount(100);
		tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
		tableWidget->setGeometry(QRect(2, 2, 766, 586));
		tableWidget->setRowCount(100);
		tableWidget->setColumnCount(7);
		tableWidget->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
		tableWidget->horizontalHeader()->setStretchLastSection(true);
		tableWidget->verticalHeader()->setVisible(false);
		scrollArea->setWidget(scrollAreaWidgetContents);

	} // setupUi

};



#endif // UI_STATIONS_H
