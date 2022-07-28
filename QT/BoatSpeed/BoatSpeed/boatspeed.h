#ifndef BOATSPEED_H
#define BOATSPEED_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class BoatSpeed; }
QT_END_NAMESPACE

class BoatSpeed : public QMainWindow
{
    Q_OBJECT

public:
    BoatSpeed(QWidget *parent = nullptr);
    ~BoatSpeed();

	void ProcessRMC(char * msg);

private slots:

	void Reset();
	void MyTimerSlot();
	void readPendingDatagrams();
	void socketError();

private:
    Ui::BoatSpeed *ui;
	QImage *Graph;

};
#endif // BOATSPEED_H
