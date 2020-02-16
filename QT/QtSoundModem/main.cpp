//
//	 My port of UZ7HO's Soundmodem
//

#include "QtSoundModem.h"
#include <QtWidgets/QApplication>
#include "UZ7HOStuff.h"

extern "C" int nonGUIMode;

extern void getSettings();

workerThread *t;
mynet m1;

QCoreApplication * a;		

QtSoundModem * w;

int main(int argc, char *argv[])
{
	char Title[128];

	if (argc > 1 && strcmp(argv[1], "nogui") == 0)
		nonGUIMode = 1;

	if (nonGUIMode)
		sprintf(Title, "QtSoundModem Version %s Running in non-GUI Mode", VersionString);
	else
		sprintf(Title, "QtSoundModem Version %s Running in GUI Mode", VersionString);

	qDebug() << Title;

	if (nonGUIMode)
		a = new QCoreApplication(argc, argv);
	else
		a = new QApplication(argc, argv);			// GUI version

	getSettings();

	t = new workerThread;

	if (nonGUIMode == 0)
	{
		w = new QtSoundModem();
		w->show();
	}

	t->start();				// This runs init

	m1.start();				// Start TCP 

	return a->exec();
}
