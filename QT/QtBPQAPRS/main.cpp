#include "QtBPQAPRS.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtBPQAPRS w;
    w.show();
    return a.exec();
}
