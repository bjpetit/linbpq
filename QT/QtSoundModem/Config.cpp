
#include <QSettings>

#include "UZ7HOStuff.h"

extern "C" void get_exclude_list(char * line, TStringList * list);
extern "C" void get_exclude_frm(char * line, TStringList * list);

extern "C" int RX_SR;
extern "C" int TX_SR;

QSettings* settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);

// This makes geting settings for more channels easier

char Prefix[16] = "AX25_A";

void GetPortSettings(int Chan);

QVariant getAX25Param(const char * key, QVariant Default)
{
	char fullKey[64];

	sprintf(fullKey, "%s/%s", Prefix, key);

	return settings->value(fullKey, Default);
}

void getAX25Params(int chan)
{
	Prefix[5] = chan + 'A';
	GetPortSettings(chan);
}


void GetPortSettings(int Chan)
{
	tx_hitoneraisedb[Chan] = getAX25Param("HiToneRaise", 0).toInt();

	maxframe[Chan] = getAX25Param("Maxframe", 3).toInt();
	fracks[Chan] = getAX25Param("Retries", 15).toInt();
	frack_time[Chan] = getAX25Param("FrackTime", 5).toInt();

	idletime[Chan] = getAX25Param("IdleTime", 180).toInt();
	slottime[Chan] = getAX25Param("SlotTime", 100).toInt();
	persist[Chan] = getAX25Param("Persist", 128).toInt();
	resptime[Chan] = getAX25Param("RespTime", 1500).toInt();
	TXFrmMode[Chan] = getAX25Param("TXFrmMode", 1).toInt();
	max_frame_collector[Chan] = getAX25Param("FrameCollector", 6).toInt();
	//	exclude_callsigns[Chan]= getAX25Param("ExcludeCallsigns/");
	//	exclude_APRS_frm[Chan]= getAX25Param("ExcludeAPRSFrmType/");
	KISS_opt[Chan] = getAX25Param("KISSOptimization", false).toInt();;
	dyn_frack[Chan] = getAX25Param("DynamicFrack", false).toInt();;
	recovery[Chan] = getAX25Param("BitRecovery", 0).toInt();
	NonAX25[Chan] = getAX25Param("NonAX25Frm", false).toInt();;
	//	MEMRecovery[Chan]= getAX25Param("MEMRecovery", 200).toInt();
	IPOLL[Chan] = getAX25Param("IPOLL", 80).toInt();

	strcpy(MyDigiCall[Chan], getAX25Param("MyDigiCall", "").toString().toUtf8());

	fx25_mode[Chan] = getAX25Param("FX25", FX25_MODE_RX).toInt();

	soundChannel[Chan] = getAX25Param("soundChannel", 1).toInt();
}

void getSettings()
{
	int snd_ch;

	QSettings* settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
	settings->sync();

	RX_SR = settings->value("Init/RXSampleRate", 12000).toInt();
	TX_SR = settings->value("Init/TXSampleRate", 12000).toInt();

	strcpy(CaptureDevice, settings->value("Init/SndRXDeviceName", "hw:1,0").toString().toUtf8());
	strcpy(PlaybackDevice, settings->value("Init/SndTXDeviceName", "hw:1,0").toString().toUtf8());

	raduga = settings->value("Init/DispMode", DISP_RGB).toInt();

	strcpy(PTTPort, settings->value("Init/PTT", "").toString().toUtf8());

	DualPTT = settings->value("Init/DualPTT", 1).toInt();
	TX_rotate = settings->value("Init/TXRotate", 0).toInt();

	rx_freq[0] = settings->value("Modem/RXFreq1", 1700).toInt();
	rx_freq[1] = settings->value("Modem/RXFreq2", 1700).toInt();

	rcvr_offset[0] = settings->value("Modem/RcvrShift1", 30).toInt();
	rcvr_offset[1] = settings->value("Modem/RcvrShift2", 30).toInt();
	speed[0] = settings->value("Modem/ModemType1", SPEED_1200).toInt();
	speed[1] = settings->value("Modem/ModemType2", SPEED_1200).toInt();

	RCVR[0] = settings->value("Modem/NRRcvrPairs1", 0).toInt();;
	RCVR[1] = settings->value("Modem/NRRcvrPairs2", 0).toInt();;

	soundChannel[0] = settings->value("Modem/soundChannel1", 0).toInt();
	soundChannel[1] = settings->value("Modem/soundChannel2", 0).toInt();

	DualChan = settings->value("Init/DualChan", 0).toInt();
	SCO = settings->value("Init/SCO", 0).toInt();

	dcd_threshold = settings->value("Modem/DCDThreshold", 40).toInt();
		
	AGWServ = settings->value("AGWHost/Server", TRUE).toBool();
	AGWPort = settings->value("AGWHost/Port", 8000).toInt();
	KISSServ = settings->value("KISS/Server", TRUE).toBool();
	KISSPort = settings->value("KISS/Port", 8105).toInt();

	RX_Samplerate = RX_SR + RX_SR * 0.000001*RX_PPM;
	TX_Samplerate = TX_SR + TX_SR * 0.000001*TX_PPM;

	emph_all[0] = settings->value("Modem/PreEmphasisAll1", TRUE).toBool();
	emph_all[1] = settings->value("Modem/PreEmphasisAll2", TRUE).toBool();
	emph_db[0] = settings->value("Modem/PreEmphasisDB1", 0).toInt();
	emph_db[1] = settings->value("Modem/PreEmphasisDB2", 0).toInt();

	Firstwaterfall = settings->value("Window/Waterfall1", TRUE).toInt();
	Secondwaterfall = settings->value("Window/Waterfall2", TRUE).toInt();

	txdelay[0] = settings->value("Modem/TxDelay1", 250).toInt();
	txdelay[1] = settings->value("Modem/TxDelay2", 250).toInt();

	getAX25Params(0);
	getAX25Params(1);

	// Validate and process settings

	if (soundChannel[1])
		DualChan = 1;		// DualChan means two modems, not always L and R channels
	else
		DualChan = 0;

	if (DualChan && (soundChannel[0] != soundChannel[1]))
	{
		// Different so need both sides 

		UsingBothChannels = 1;
		UsingLeft = 1;
		UsingRight = 1;
	}
	else
	{
		UsingBothChannels = 0;

		if (soundChannel[0] == RIGHT)
		{
			UsingLeft = 0;
			UsingRight = 1;
		}
		else
		{
			UsingLeft = 1;
			UsingRight = 0;
		}
	}

	if (soundChannel[0] == RIGHT)
		modemtoSoundLR[0] = 1;
	else
		modemtoSoundLR[0] = 0;

	if (soundChannel[1] == RIGHT)
		modemtoSoundLR[1] = 1;
	else
		modemtoSoundLR[1] = 0;

	for (snd_ch = 0; snd_ch < 2; snd_ch++)
	{
		tx_hitoneraise[snd_ch] = powf(10.0f, -abs(tx_hitoneraisedb[snd_ch]) / 20.0f);



		if (IPOLL[snd_ch] < 0)
			IPOLL[snd_ch] = 0;
		else if (IPOLL[snd_ch] > 65535)
			IPOLL[snd_ch] = 65535;

		//	if MEMRecovery[snd_ch] < 1 then MEMRecovery[snd_ch]= 1;
		//	if MEMRecovery[snd_ch] > 65535 then MEMRecovery[snd_ch]= 65535;

		/*
		if resptime[snd_ch] < 0 then resptime[snd_ch]= 0;
			if resptime[snd_ch] > 65535 then resptime[snd_ch]= 65535;
			if persist[snd_ch] > 255 then persist[snd_ch]= 255;
			if persist[snd_ch] < 32 then persist[snd_ch]= 32;
			if fracks[snd_ch] < 1 then fracks[snd_ch]= 1;
			if frack_time[snd_ch] < 1 then frack_time[snd_ch]= 1;
			if idletime[snd_ch] < frack_time[snd_ch] then idletime[snd_ch]= 180;
		*/

		if (emph_db[snd_ch] < 0 || emph_db[snd_ch] > nr_emph)
			emph_db[snd_ch] = 0;
		/*
		if not (Recovery[snd_ch] in[0..1]) then Recovery[snd_ch]= 0;
			if not (TXFrmMode[snd_ch] in[0..1]) then TXFrmMode[snd_ch]= 0;
			if not (max_frame_collector[snd_ch] in[0..6]) then max_frame_collector[snd_ch]= 6;
			if not (maxframe[snd_ch] in[1..7]) then maxframe[snd_ch]= 3;

			if not (qpsk_set[snd_ch].mode in[0..1]) then qpsk_set[snd_ch].mode= 0;
			init_speed(snd_ch);
			end;
			TrackBar1.Position= DCD_threshold;
			*/
	}

	delete(settings);
}



void saveSettings()
{
	QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);

	settings->setValue("Init/TXSampleRate", TX_SR);
	settings->setValue("Init/RXSampleRate", RX_SR);

	settings->setValue("Init/SndRXDeviceName", CaptureDevice);
	settings->setValue("Init/SndTXDeviceName", PlaybackDevice);

	settings->setValue("Init/DualChan", DualChan);
	settings->setValue("Init/SCO", SCO);
	settings->setValue("Init/DualPTT", DualPTT);
	settings->setValue("Init/TXRotate", TX_rotate);

	settings->setValue("Init/DispMode", raduga);

	settings->setValue("Init/PTT", PTTPort);

	// Don't save freq on close as it could be offset by multiple decoders

//	settings->setValue("Modem/RXFreq1", rx_freq[0]);
//	settings->setValue("Modem/RXFreq2", rx_freq[1]);

	settings->setValue("Modem/NRRcvrPairs1", RCVR[0]);
	settings->setValue("Modem/NRRcvrPairs2", RCVR[1]);

	settings->setValue("Modem/RcvrShift1", rcvr_offset[0]);
	settings->setValue("Modem/RcvrShift2", rcvr_offset[1]);

	settings->setValue("Modem/ModemType1", speed[0]);
	settings->setValue("Modem/ModemType2", speed[1]);

	settings->setValue("Modem/soundChannel1", soundChannel[0]);
	settings->setValue("Modem/soundChannel2", soundChannel[1]);

	settings->setValue("Modem/DCDThreshold", dcd_threshold);

	settings->setValue("AGWHost/Server", AGWServ);
	settings->setValue("AGWHost/Port", AGWPort);
	settings->setValue("KISS/Server", KISSServ);
	settings->setValue("KISS/Port", KISSPort);

	settings->setValue("AX25_A/Retries", fracks[0]);

	settings->setValue("AX25_A/soundChannel", soundChannel[0]);
	settings->setValue("AX25_B/soundChannel", soundChannel[1]);

	settings->setValue("Modem/PreEmphasisAll1", emph_all[0]);
	settings->setValue("Modem/PreEmphasisAll2", emph_all[1]);
	settings->setValue("Modem/PreEmphasisDB1", emph_db[0]);
	settings->setValue("Modem/PreEmphasisDB2", emph_db[1]);

	settings->setValue("Window/Waterfall1", Firstwaterfall);
	settings->setValue("Window/Waterfall2", Secondwaterfall);

	settings->setValue("Modem/TxDelay1", txdelay[0]);
	settings->setValue("Modem/TxDelay2", txdelay[1] );

	settings->setValue("AX25_A/FX25", fx25_mode[0]);
	settings->setValue("AX25_B/FX25", fx25_mode[1]);

	settings->sync();

	delete(settings);
}
