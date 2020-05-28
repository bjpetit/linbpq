#include "BPQConfigGen.h"
#include <QtWidgets/QApplication>

char * pgm;
int restartingforupdate = 0;

int main(int argc, char *argv[])
{
	pgm = argv[0];

	if (argc > 1 && strcmp(argv[1], "adminrestart") == 0)
		restartingforupdate = 1;

	QApplication a(argc, argv);
	BPQConfigGen w;
	w.show();
	return a.exec();
}
