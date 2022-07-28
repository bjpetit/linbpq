#include "boatspeed.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//	a.setOrganizationDomain("g8bpq.net");
//	a.setApplicationName("boatspeed");

    BoatSpeed w;
    w.show();
    return a.exec();
}
