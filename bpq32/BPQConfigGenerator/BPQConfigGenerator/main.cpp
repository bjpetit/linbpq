#include "BPQConfigGenerator.h"
#include <QtWidgets/QApplication>

char * pgm;

int main(int argc, char *argv[])
{
	pgm = argv[0];
	QApplication a(argc, argv);
	BPQConfigGenerator w;
	w.show();
	return a.exec();
}
