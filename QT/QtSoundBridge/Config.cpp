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

#include <QSettings>

#include "UZ7HOStuff.h"

extern "C" void get_exclude_list(char * line, TStringList * list);
extern "C" void get_exclude_frm(char * line, TStringList * list);

extern "C" int SoundMode; 
extern "C" int RX_SR = 0;
extern "C" int TX_SR = 0;
extern "C" int multiCore;

extern "C" word MEMRecovery[5];

extern int MintoTray;
extern int UDPPort;
extern int TXPort;

extern char UDPHost[64];

QSettings* settings = new QSettings("QtSoundBridge.ini", QSettings::IniFormat);


void getSettings()
{
	int snd_ch;

	QSettings* settings = new QSettings("QtSoundBridge.ini", QSettings::IniFormat);
	settings->sync();

	SoundMode = settings->value("Init/SoundMode", 0).toInt();
	UDPPort = settings->value("Init/UDPPort", 8884).toInt();
	TXPort = settings->value("Init/TXPort", UDPPort).toInt();
	strcpy(UDPHost, settings->value("Init/UDPHost", "192.168.1.255").toString().toUtf8());

	RX_SR = settings->value("Init/RXSampleRate", 12000).toInt();
	TX_SR = settings->value("Init/TXSampleRate", 12000).toInt();

	strcpy(CaptureDevice, settings->value("Init/SndRXDeviceName", "hw:1,0").toString().toUtf8());
	strcpy(PlaybackDevice, settings->value("Init/SndTXDeviceName", "hw:1,0").toString().toUtf8());

	multiCore = settings->value("Init/multiCore", 0).toInt();
	MintoTray = settings->value("Init/MinimizetoTray", 1).toInt();

	delete(settings);
}


void saveSettings()
{
	QSettings * settings = new QSettings("QtSoundBridge.ini", QSettings::IniFormat);

	settings->setValue("Init/SoundMode", SoundMode);
	settings->setValue("Init/UDPPort", UDPPort);
	settings->setValue("Init/TXPort", TXPort);
	settings->setValue("Init/UDPHost", UDPHost);

	settings->setValue("Init/TXSampleRate", TX_SR);
	settings->setValue("Init/RXSampleRate", RX_SR);

	settings->setValue("Init/SndRXDeviceName", CaptureDevice);
	settings->setValue("Init/SndTXDeviceName", PlaybackDevice);

	settings->setValue("Init/multiCore", multiCore);
	settings->setValue("Init/MinimizetoTray", MintoTray);

	// Don't save freq on close as it could be offset by multiple decoders

	settings->sync();

	delete(settings);
}
