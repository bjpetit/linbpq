#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtSoundModem.h"
#include <QTableWidget>

class QtSoundModem : public QMainWindow
{
	Q_OBJECT

public:
	QtSoundModem(QWidget *parent = Q_NULLPTR);

protected:

	void resizeEvent(QResizeEvent *event) override;

private:
	Ui::QtSoundModemClass ui;
	QTableWidget* m_pTableWidget;
	QStringList m_TableHeader;

};
