//
//	Audio interface Routine

//	Passes audio samples to/from the sound interface

//	As this is platform specific it also has the main() routine, which does
//	platform specific initialisation before calling ardopmain()

//	This is ALSASound.c for Linux
//	Windows Version is Waveout.c
//	Nucleo Version is NucleoSound.c


#include <alsa/asoundlib.h>
#include <signal.h>

#include "ARDOPC.h"

void Sleep(int ticks)
{
	usleep(ticks * 1000);
	return;
}


// Windows/ALSA works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

short buffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "ARDOP";
char PlaybackDevice[80] = "ARDOP";

char * CaptureDevices = CaptureDevice;
char * PlaybackDevices = CaptureDevice;

void InitSound();

int Ticks;

int LastNow;

void printtick(char * msg)
{
	printf("%s %i\r\n", msg, Now - LastNow);
	LastNow = Now;
}

struct timespec time_start;

int getTicks()
{	
	struct timespec tp;
	
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (tp.tv_sec - time_start.tv_sec) * 1000 + (tp.tv_nsec - time_start.tv_nsec) / 1000000;
}

void main(int argc, char * argv[])
{
	struct timespec tp;

	if (argc > 1)
		port = atoi(argv[1]);

	printf("ARDOPC listening on port %d\n", port);

	// Get Time Reference
		
	clock_gettime(CLOCK_MONOTONIC, &time_start);
	LastNow = getTicks();

	printtick("First tick");
	printtick("Test Start");
	Sleep(1000);
	printtick("Test End");
	ardopmain();
}

// ALSA Code 

#define true 1
#define false 0

snd_pcm_t *	playhandle = NULL;
snd_pcm_t *	rechandle = NULL;

int m_playchannels = 1;
int m_recchannels = 1;

char SavedCaptureDevice[256];	// Saved so we can reopen
char SavedPlaybackDevice[256];

int SavedCaptureRate;
int SavedPlaybackRate;

// This rather convoluted process simplifies marshalling from Managed Code

char ** WriteDevices = NULL;
int WriteDeviceCount = 0;

char ** ReadDevices = NULL;
int ReadDeviceCount = 0;

// Routine to check that library is available

int CheckifLoaded()
{
	// Prevent CTRL/C from closing the TNC
	// (This causes problems if the TNC is started by LinBPQ)

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	return TRUE;
}

int GetOutputDeviceCollection()
{
	// Get all the suitable devices and put in a list for GetNext to return

	snd_ctl_t *handle= NULL;
	snd_pcm_t *pcm= NULL;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_pcm_hw_params_t *pars;
	snd_pcm_format_mask_t *fmask;
	char NameString[256];

	printf("getWriteDevices\n");

	CloseSoundCard();

	// free old struct if called again

//	while (WriteDeviceCount)
//	{
//		WriteDeviceCount--;
//		free(WriteDevices[WriteDeviceCount]);
//	}

//	if (WriteDevices)
//		free(WriteDevices);

	WriteDevices = NULL;
	WriteDeviceCount = 0;

	//	Add virtual device ARDOP so ALSA plugins can be used if needed

	WriteDevices = realloc(WriteDevices,(WriteDeviceCount + 1) * 4);
	WriteDevices[WriteDeviceCount++] = strdup("ARDOP");

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	
	card = -1;

	if (snd_card_next(&card) < 0)
	{
		printf("No Devices\n");
		return 0;
	}

	if (playhandle)
		snd_pcm_close(playhandle);

	playhandle = NULL;

	while (card >= 0)
	{
		sprintf(hwdev, "hw:%d", card);
		err = snd_ctl_open(&handle, hwdev, 0);
		err = snd_ctl_card_info(handle, info);
    
		printf("Card %d, ID `%s', name `%s'\n", card, snd_ctl_card_info_get_id(info),
                snd_ctl_card_info_get_name(info));


		dev = -1;

		if(snd_ctl_pcm_next_device(handle, &dev) < 0)
		{
			// Card has no devices

			snd_ctl_close(handle);
			goto nextcard;      
		}

		while (dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
	
			err = snd_ctl_pcm_info(handle, pcminfo);

			
			if (err == -ENOENT)
				goto nextdevice;

			nsubd = snd_pcm_info_get_subdevices_count(pcminfo);
		
			printf("  Device %d, ID `%s', name `%s', %d subdevices (%d available)\n",
				dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);

			if (err)
			{
				printf("Error %d opening output device\n", err);
				goto nextdevice;
			}

			//	Get parameters for this device

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
			
			if( min == max )
				if(min == 1)
					printf("    1 channel, ");
				else
					printf("    %d channels, ", min);
			else
				printf("    %u..%u channels, ", min, max);
			
			snd_pcm_hw_params_get_rate_min(pars, &min, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &max, NULL);
			printf("sampling rate %u..%u Hz\n", min, max);

			// Add device to list

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

			printf("%s\n", NameString);

			WriteDevices = realloc(WriteDevices,(WriteDeviceCount + 1) * 4);
			WriteDevices[WriteDeviceCount++] = strdup(NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:

			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }

		snd_ctl_close(handle);

nextcard:

		if (snd_card_next(&card) < 0)		// No more cards
			break;
	}

	return WriteDeviceCount;
}

int GetNextOutputDevice(char * dest, int max, int n)
{
	if (n >= WriteDeviceCount)
		return 0;

	strcpy(dest, WriteDevices[n]);
	return strlen(dest);
}


int GetInputDeviceCollection()
{
	// Get all the suitable devices and put in a list for GetNext to return

	snd_ctl_t *handle= NULL;
	snd_pcm_t *pcm= NULL;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_pcm_hw_params_t *pars;
	snd_pcm_format_mask_t *fmask;
	char NameString[256];

	printf("getReadDevices\n");

	// free old struct if called again

//	while (ReadDeviceCount)
//	{
//ReadDeviceCount--;
//		free(ReadDevices[ReadDeviceCount]);
//	}

//	if (ReadDevices)
//		free(ReadDevices);

	ReadDevices = NULL;
	ReadDeviceCount = 0;

	//	Add virtual device ARDOP so ALSA plugins can be used if needed

	ReadDevices = realloc(ReadDevices,(ReadDeviceCount + 1) * 4);
	ReadDevices[ReadDeviceCount++] = strdup("ARDOP");

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	
	card = -1;

	if(snd_card_next(&card) < 0)
	{
		printf("No Devices\n");
		return 0;
	}

	if (rechandle)
		snd_pcm_close(rechandle);

	rechandle = NULL;

	while(card >= 0)
	{
		sprintf(hwdev, "hw:%d", card);
		err = snd_ctl_open(&handle, hwdev, 0);
		err = snd_ctl_card_info(handle, info);
    
		printf("Card %d, ID `%s', name `%s'\n", card, snd_ctl_card_info_get_id(info),
                snd_ctl_card_info_get_name(info));

		dev = -1;
			
		if (snd_ctl_pcm_next_device(handle, &dev) < 0)		// No Devicdes
		{
			snd_ctl_close(handle);
			goto nextcard;      
		}

		while(dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			err= snd_ctl_pcm_info(handle, pcminfo);
	
			if (err == -ENOENT)
				goto nextdevice;
	
			nsubd= snd_pcm_info_get_subdevices_count(pcminfo);
			printf("  Device %d, ID `%s', name `%s', %d subdevices (%d available)\n",
				dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);
	
			if (err)
			{	
				printf("Error %d opening input device\n", err);
				goto nextdevice;
			}

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
	
			if( min == max )
				if( min == 1 )
					printf("    1 channel, ");
				else
					printf("    %d channels, ", min);
			else
				printf("    %u..%u channels, ", min, max);
			
			snd_pcm_hw_params_get_rate_min(pars, &min, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &max, NULL);
			printf("sampling rate %u..%u Hz\n", min, max);

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

			printf("%s\n", NameString);

			ReadDevices = realloc(ReadDevices,(ReadDeviceCount + 1) * 4);
			ReadDevices[ReadDeviceCount++] = strdup(NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:
		
			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }

		snd_ctl_close(handle);

nextcard:

		if (snd_card_next(&card) < 0 )
			break;
	}

	return ReadDeviceCount;
}

int GetNextInputDevice(char * dest, int max, int n)
{
	if (n >= ReadDeviceCount)
		return 0;

	strcpy(dest, ReadDevices[n]);
	return strlen(dest);
}
int OpenSoundCard(char * CaptureDevice, char * PlaybackDevice, int c_sampleRate, int p_sampleRate, char * ErrorMsg)
{
	printf("Opening Playback Device %s Rate %d\n", PlaybackDevice, p_sampleRate);

	if (OpenSoundPlayback(PlaybackDevice, p_sampleRate, ErrorMsg))
	{
		printf("Opening Capture Device %s Rate %d\n", CaptureDevice, c_sampleRate, ErrorMsg);
		return OpenSoundCapture(CaptureDevice, c_sampleRate, ErrorMsg);
	}
	else
		return false;
}

int OpenSoundPlayback(char * PlaybackDevice, int m_sampleRate, char * ErrorMsg)
{
	int err = 0;

	char buf1[100];
	char * ptr;

	if (playhandle)
	{
		snd_pcm_close(playhandle);
		playhandle = NULL;
	}

	strcpy(SavedPlaybackDevice, PlaybackDevice);	// Saved so we can reopen in error recovery
	SavedPlaybackRate = m_sampleRate;

	strcpy(buf1, PlaybackDevice);

	ptr = strchr(buf1, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name

	snd_pcm_hw_params_t *hw_params;
	
	if ((err = snd_pcm_open(&playhandle, buf1, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot open playback audio device %s (%s)",  buf1, snd_strerror(err));
		else
			fprintf (stderr, "cannot open playback audio device %s (%s)\n",  buf1, snd_strerror(err));
		return false;
	}
		   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (playhandle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (playhandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			fprintf (stderr, "cannot set playback access type (%s)\n", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (playhandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot setplayback  sample format (%s)\n", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_rate (playhandle, hw_params, m_sampleRate, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot set playback sample rate (%s)\n", snd_strerror(err));
		else
			fprintf (stderr, "cannot set playback sample rate (%s)\n", snd_strerror(err));
		return false;
	}

	m_playchannels = 1;
	
	if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, 1)) < 0)
	{
		fprintf (stderr, "cannot set play channel count to 1 (%s)\n", snd_strerror(err));
		m_playchannels = 2;

		if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, 2)) < 0)
		{
			fprintf (stderr, "cannot play set channel count to 2 (%s)\n", snd_strerror(err));
				return false;
		}
		fprintf (stderr, "Play channel count set to 2 (%s)\n", snd_strerror(err));
	}
	
	if ((err = snd_pcm_hw_params (playhandle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		return false;
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (playhandle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return false;
	}

	return true;
}

int OpenSoundCapture(char * CaptureDevice, int m_sampleRate, char * ErrorMsg)
{
	int err = 0;

	char buf1[100];
	char * ptr;
	snd_pcm_hw_params_t *hw_params;

	if (rechandle)
	{
		snd_pcm_close(rechandle);
		rechandle = NULL;
	}

	strcpy(SavedCaptureDevice, CaptureDevice);	// Saved so we can reopen in error recovery
	SavedCaptureRate = m_sampleRate;

	strcpy(buf1, CaptureDevice);

	ptr = strchr(buf1, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name
	
	if ((err = snd_pcm_open (&rechandle, buf1, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot open capture audio device %s (%s)\n",  buf1, snd_strerror(err));
		else
			fprintf (stderr, "cannot open capture audio device %s (%s)\n",  buf1, snd_strerror(err));
		return false;
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate capture hardware parameter structure (%s)\n", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (rechandle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize capture hardware parameter structure (%s)\n", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (rechandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			fprintf (stderr, "cannot set capture access type (%s)\n", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (rechandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set capture sample format (%s)\n", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_rate (rechandle, hw_params, m_sampleRate, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot set capture sample rate (%s)\n", snd_strerror(err));
		else
			fprintf (stderr, "cannot set capture sample rate (%s)\n", snd_strerror(err));
		return false;
	}
	
	m_recchannels = 1;
	
	if ((err = snd_pcm_hw_params_set_channels (rechandle, hw_params, 1)) < 0)
	{
		fprintf (stderr, "cannot set rec channel count to 1 (%s)\n", snd_strerror(err));
		m_recchannels = 2;

		if ((err = snd_pcm_hw_params_set_channels (rechandle, hw_params, 2)) < 0)
		{
			fprintf (stderr, "cannot rec set channel count to 2 (%s)\n", snd_strerror(err));
			return false;
		}
		fprintf (stderr, "Record channel count set to 2 (%s)\n", snd_strerror(err));
	}
	
	if ((err = snd_pcm_hw_params (rechandle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		return false;
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (rechandle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return FALSE;
	}

	int i;
	short buf[256];

	for (i = 0; i < 10; ++i)
	{
		if ((err = snd_pcm_readi (rechandle, buf, 128)) != 128)
		{
			fprintf (stderr, "read from audio interface failed (%s)\n",
				 snd_strerror (err));
		}
	}

//	printf("Read got %d\n", err);

 	return TRUE;
}

int CloseSoundCard()
{
	if (rechandle)
	{
		snd_pcm_close(rechandle);
		rechandle = NULL;
	}

	if (playhandle)
	{
		snd_pcm_close(playhandle);
		playhandle = NULL;
	}
	return 0;
}


int SoundCardWrite(short * input, unsigned int nSamples)
{
	unsigned int i = 0, n;
	int ret, err, res;
	snd_pcm_sframes_t avail, maxavail;
	snd_pcm_status_t *status = NULL;

	if (playhandle == NULL)
		return 0;

	//	Stop Capture

	if (rechandle)
	{
		snd_pcm_close(rechandle);
		rechandle = NULL;
	}

	avail = snd_pcm_avail_update(playhandle);
//	printf("avail before play returned %d\n", (int)avail);

	if (avail < 0)
	{
		if (avail != -32)
			printf("Playback Avail Recovering from %d ..\n", (int)avail);
		snd_pcm_recover(playhandle, avail, 1);

		avail = snd_pcm_avail_update(playhandle);

		if (avail < 0)
			printf("avail play after recovery returned %d\n", (int)avail);
	}
	
	maxavail = avail;

//	printf("Tosend %d Avail %d\n", nSamples, (int)avail);

	while (avail < nSamples)
	{
		printf("Waiting for space - %d avail \n", (int)avail);
		txSleep(100);
		avail = snd_pcm_avail_update(playhandle);
	}

	ret = PackSamplesAndSend(input, nSamples);

	return ret;
}

int PackSamplesAndSend(short * input, int nSamples)
{
	unsigned short samples[256000];
	unsigned short * sampptr = samples;
	unsigned int n;
	int ret;
	snd_pcm_sframes_t avail;

	// Convert byte stream to int16 (watch endianness)

	if (m_playchannels == 1)
	{
		for (n = 0; n < nSamples; n++)
		{
			*(sampptr++) = input[0];
			input ++;
		}
	}
	else
	{
		int i = 0;
		for (n = 0; n < nSamples; n++)
		{
			*(sampptr) = input[0];
			*(sampptr + 1) = *(sampptr);		// same to both channels
			*(sampptr) += 2;
			input ++;
		}
	}

	ret = snd_pcm_writei(playhandle, samples, nSamples);

	if (ret < 0)
	{
//		printf("Write Recovering from %d ..\n", ret);
		snd_pcm_recover(playhandle, ret, 1);
		ret = snd_pcm_writei(playhandle, samples, nSamples);
//		printf("Write after recovery returned %d\n", ret);
	}

	avail = snd_pcm_avail_update(playhandle);
	return ret;

}

int SoundCardClearInput()
{
	short samples[65536];
	int n;
	int ret;
	int avail;

	if (rechandle == NULL)
		return 0;

	// Clear queue 
	
	avail = snd_pcm_avail_update(rechandle);

	if (avail < 0)
	{
		printf("Discard Recovering from %d ..\n", avail);
		if (rechandle)
		{
			snd_pcm_close(rechandle);
			rechandle = NULL;
		}
		OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);
		avail = snd_pcm_avail_update(rechandle);
	}

	while (avail)
	{
		if (avail > 65536)
			avail = 65536;

			ret = snd_pcm_readi(rechandle, samples, avail);
//			printf("Discarded %d samples from card\n", ret);
			avail = snd_pcm_avail_update(rechandle);

//			printf("Discarding %d samples from card\n", avail);
	}
	return 0;
}


int SoundCardRead(short * input, unsigned int nSamples)
{
	short samples[65536];
	int n;
	int ret;
	int avail;

	if (rechandle == NULL)
		return 0;

	avail = snd_pcm_avail_update(rechandle);

	if (avail < 0)
	{
		printf("Read Recovering from %d ..\n", avail);
		if (rechandle)
		{
			snd_pcm_close(rechandle);
			rechandle = NULL;
		}

		OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);
//		snd_pcm_recover(rechandle, avail, 0);
		avail = snd_pcm_avail_update(rechandle);
		printf("Read After recovery %d ..\n", avail);
	}

	if (avail < 1200)
		return 0;

	if (avail > nSamples)
	{
//		printf("ALSARead available %d\n", avail);
		avail = nSamples;
	}

//	printf("ALSARead available %d\n", avail);

	ret = snd_pcm_readi(rechandle, samples, avail);

	if (ret < 0)
	{
		printf("RX Error %d\n", ret);
		snd_pcm_recover(rechandle, avail, 0);
		return 0;
	}

	if (m_recchannels == 1)
	{
		for (n = 0; n < ret; n++)
		{
			*(input++) = samples[n];
		}
	}
	else
	{
		for (n = 0; n < (ret * 2); n+=2)			// return alternate
		{
			*(input++) = samples[n];
		}
	}

	return ret;
}




int PriorSize = 0;

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;				// DMA Buffer being used 0 or 1

BOOL DMARunning = FALSE;		// Used to start DMA on first write

short * SendtoCard(short * buf, int n)
{
	if (Loopback)
	{
		// Loop back   to decode for testing

		ProcessNewSamples(buf, 1200);		// signed
	}

	SoundCardWrite(&buffer[Index][0], n);

//	txSleep(10);				// Run buckground while waiting 

	return &buffer[Index][0];
}

short loopbuff[1200];		// Temp for testing - loop sent samples to decoder


//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);



void InitSound()
{

	GetInputDeviceCollection();
	GetOutputDeviceCollection();

	OpenSoundCard(CaptureDevice, PlaybackDevice, 12000, 12000, NULL);
}

PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	if (SoundCardRead(&inbuffer[0][0], 1200))
	{
		// returns 1200 or none


		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[0][0], 1200);
	}
}


void StopCapture()
{
	Capturing = FALSE;
//	printf("Stop Capture\n");
}

void StartCodec(char * strFault)
{
	strFault[0] = 0;
	OpenSoundCard(CaptureDevice, PlaybackDevice, 12000, 12000, strFault);
}

void StopCodec(char * strFault)
{
	strFault[0] = 0;
	CloseSoundCard();
}

void StartCapture()
{
	Capturing = TRUE;
	DiscardOldSamples();
	ClearAllMixedSamples();
	State = SearchingForLeader;

//	printf("Start Capture\n");
}
void CloseSound()
{ 
	CloseSoundCard();
}

#include <stdarg.h>

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	printf("%s", Mess);

	return;
}




VOID WriteSamples(short * buffer, int len)
{

#ifdef WIN32
	fwrite(buffer, 1, len * 2, wavfp1);
#endif
}

unsigned short * SoundInit()
{
	Index = 0;
	return &buffer[0][0];
}
	
//	Called at end of transmission

void SoundFlush(Number)
{
	// Append Trailer then send remaining samples

	snd_pcm_status_t *status = NULL;
	int err, res;

	AddTrailer();			// add the trailer.

	if (Loopback)
		ProcessNewSamples(&buffer[Index], Number);

	SendtoCard(&buffer[Index][0], Number);

	// Wait for tx to complete

	while (1)
	{
	//	printf("Waiting... \n");
		
		snd_pcm_status_alloca(&status);					// alloca allocates once per function, does not need a free

		if ((err=snd_pcm_status(playhandle, status))!=0)
		{
    		printf("snd_pcm_status() failed: %s",snd_strerror(err));
			break;
		}
	 
		res = snd_pcm_status_get_state(status);

		if (res != SND_PCM_STATE_RUNNING)				// If sound system is not running then it needs data
		{
			// Send complete - Restart Capture

			OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);	
			break;
		}
		usleep(50000);
	}

	SoundIsPlaying = FALSE;
	PlayComplete = TRUE;
	return;
}

VOID RadioPTT()	
{
	// May add PTT via GPIO for PI
}

//  Function to send PTT TRUE or PTT FALSE commanad to Host or if local Radio control Keys radio PTT 

const char BoolString[2][6] = {"FALSE", "TRUE"};

BOOL KeyPTT(BOOL blnPTT)
{
	// Returns TRUE if successful False otherwise

	if (blnLastPTT &&  !blnPTT)
		dttStartRTMeasure = Now;	 // start a measurement on release of PTT.

	if (!RadioControl)
		if (blnPTT)
			QueueCommandToHost("PTT TRUE");
		else
			QueueCommandToHost("PTT FALSE");

	else
		RadioPTT(blnPTT);

	if (DebugLog) Debugprintf("[Main.KeyPTT]  PTT-%s", BoolString[blnPTT]);

	blnLastPTT = blnPTT;
	return TRUE;
}







	
