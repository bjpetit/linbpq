#include "QtSoundModem.h"
#include <QtWidgets/QApplication>

extern "C" int nonGUIMode;

int main(int argc, char *argv[])
{
	qDebug() << "Start";

	QApplication a(argc, argv);

	qDebug() << "Stage 0";

	QtSoundModem w;

	qDebug() << "Stage 2";

	if (argc > 1 && strcmp(argv[1], "nogui") == 0)
		nonGUIMode = 1;

	if (nonGUIMode == 0)
		w.show();

	qDebug() << "Stage 3";

	return a.exec();
}
