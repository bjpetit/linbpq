/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundBridge

QtSoundBridge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundBridge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundBridge.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ



#include "QtSoundBridge.h"
#include <QtWidgets/QApplication>
#include "UZ7HOStuff.h"


extern void getSettings();
extern void saveSettings();
extern int Closing;

workerThread *t;
mynet m1;

QCoreApplication * a;		

QtSoundBridge * w;

int main(int argc, char *argv[])
{
	char Title[128];

	sprintf(Title, "QtSoundBridge Version %s Running in GUI Mode", VersionString);

	qDebug() << Title;

	a = new QApplication(argc, argv);			// GUI version

	getSettings();

	t = new workerThread;

	w = new QtSoundBridge();
	w->show();

	QObject::connect(&m1, SIGNAL(HLSetPTT(int)), &m1, SLOT(doHLSetPTT(int)), Qt::QueuedConnection);


	t->start();				// This runs init

	m1.start();				// Start TCP 

	return a->exec();

}


