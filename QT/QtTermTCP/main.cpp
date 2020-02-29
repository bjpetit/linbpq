#include "QtTermTCP.h"
#include <QApplication>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QtTermTCP w;
	w.show();

	return a.exec();
}
