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
#include <termios.h>
#include <sys/ioctl.h>

#include "UZ7HOStuff.h"

#define VOID void

extern  Closing;


//#define SHARECAPTURE		// if defined capture device is opened and closed for each transission

#define HANDLE int

void gpioSetMode(unsigned gpio, unsigned mode);
void gpioWrite(unsigned gpio, unsigned level);
int WriteLog(char * msg, int Log);
int _memicmp(unsigned char *a, unsigned char *b, int n);
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2);
int gpioInitialise(void);
HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits);
int CloseSoundCard();
int PackSamplesAndSend(short * input, int nSamples);
void displayLevel(int max);
BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite);
VOID processargs(int argc, char * argv[]);
void PollReceivedSamples();

int initdisplay();

extern BOOL blnDISCRepeating;
extern BOOL UseKISS;			// Enable Packet (KISS) interface

extern unsigned short * DMABuffer;


BOOL UseLeft = TRUE;
BOOL UseRight = TRUE;
char LogDir[256] = "";

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	WriteDebugLog(LOGDEBUG, Mess);

	return;
}


void Sleep(int mS)
{
	usleep(mS * 1000);
	return;
}


// Windows and ALSA work with signed samples +- 32767
// STM32 and Teensy DAC uses unsigned 0 - 4095

short buffer[2][1200 * 2];			// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[2][1200 * 2];		// Two Transfer/DMA buffers of 0.1 Sec

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "plughw:0,0";
char PlaybackDevice[80] = "plughw:0,0";

char * CaptureDevices = CaptureDevice;
char * PlaybackDevices = CaptureDevice;

int CaptureIndex = 0;
int PlayBackIndex = 0;

void InitSound();

int Ticks;

int LastNow;

extern int Number;				// Number waiting to be sent

snd_pcm_sframes_t MaxAvail;

#include <stdarg.h>

FILE *logfile[3] = {NULL, NULL, NULL};
char LogName[3][256] = {"ARDOPDebug", "ARDOPException", "ARDOPSession"};

#define DEBUGLOG 0
#define EXCEPTLOG 1
#define SESSIONLOG 2

FILE *statslogfile = NULL;

void printtick(char * msg)
{
	Debugprintf("%s %i", msg, Now - LastNow);
	LastNow = Now;
}

struct timespec time_start;

unsigned int getTicks()
{	
	struct timespec tp;
	
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (tp.tv_sec - time_start.tv_sec) * 1000 + (tp.tv_nsec - time_start.tv_nsec) / 1000000;
}

void PlatformSleep(int mS)
{
	Sleep(mS);
}

// PTT via GPIO code

#ifdef __ARM_ARCH

#define PI_INPUT  0
#define PI_OUTPUT 1
#define PI_ALT0   4
#define PI_ALT1   5
#define PI_ALT2   6
#define PI_ALT3   7
#define PI_ALT4   3
#define PI_ALT5   2

// Set GPIO pin as output and set low

int pttGPIOPin;
BOOL pttGPIOInvert;
BOOL useGPIO = FALSE;
BOOL RadioControl = FALSE;

void SetupGPIOPTT()
{
	if (pttGPIOPin == -1)
	{
		WriteDebugLog(LOGALERT, "GPIO PTT disabled"); 
		RadioControl = FALSE;
		useGPIO = FALSE;
	}
	else
	{
		if (pttGPIOPin < 0) {
			pttGPIOInvert = TRUE;
			pttGPIOPin = -pttGPIOPin;
		}

		gpioSetMode(pttGPIOPin, PI_OUTPUT);
		gpioWrite(pttGPIOPin, pttGPIOInvert ? 1 : 0);
		WriteDebugLog(LOGALERT, "Using GPIO pin %d for PTT", pttGPIOPin); 
		RadioControl = TRUE;
		useGPIO = TRUE;
	}
}
#endif


static void sigterm_handler(int sig)
{
	printf("terminating on SIGTERM\n");
	Closing = TRUE;
}

static void sigint_handler(int sig)
{
	printf("terminating on SIGINT\n");
	Closing = TRUE;
}

char * PortString = NULL;


void platformInit()
{
	struct timespec tp;
	struct sigaction act;

//	Sleep(1000);	// Give LinBPQ time to complete init if exec'ed by linbpq

	// Get Time Reference
		
	clock_gettime(CLOCK_MONOTONIC, &time_start);
	LastNow = getTicks();

	// Trap signals

	memset (&act, '\0', sizeof(act));
 
	act.sa_handler = &sigint_handler; 
	if (sigaction(SIGINT, &act, NULL) < 0) 
		perror ("SIGINT");

	act.sa_handler = &sigterm_handler; 
	if (sigaction(SIGTERM, &act, NULL) < 0) 
		perror ("SIGTERM");

	act.sa_handler = SIG_IGN; 

	if (sigaction(SIGHUP, &act, NULL) < 0) 
		perror ("SIGHUP");

	if (sigaction(SIGPIPE, &act, NULL) < 0) 
		perror ("SIGPIPE");

}

void txSleep(int mS)
{
	// called while waiting for next TX buffer or to delay response.
	// Run background processes

	// called while waiting for next TX buffer. Run background processes

	while (mS > 50)
	{
		PollReceivedSamples();			// discard any received samples

		Sleep(50);
		mS -= 50;
	}

	Sleep(mS);

	PollReceivedSamples();			// discard any received samples
}
 
// ALSA Code 

#define true 1
#define false 0

snd_pcm_t *	playhandle = NULL;
snd_pcm_t *	rechandle = NULL;

int m_playchannels = 2;
int m_recchannels = 2;


char SavedCaptureDevice[256];	// Saved so we can reopen
char SavedPlaybackDevice[256];

int Savedplaychannels = 2;

int SavedCaptureRate;
int SavedPlaybackRate;

char CaptureNames[16][256] = { "" };
char PlaybackNames[16][256] = { "" };

int PlaybackCount = 0;
int CaptureCount = 0;

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

	Debugprintf("Playback Devices\n");

	CloseSoundCard();

	// free old struct if called again

//	while (PlaybackCount)
//	{
//		PlaybackCount--;
//		free(PlaybackNames[PlaybackCount]);
//	}

//	if (PlaybackNames)
//		free(PlaybackNames);

	PlaybackCount = 0;

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max, ratemin, ratemax;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	
	card = -1;

	if (snd_card_next(&card) < 0)
	{
		Debugprintf("No Devices");
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
    
		Debugprintf("Card %d, ID `%s', name `%s'", card, snd_ctl_card_info_get_id(info),
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
		
			Debugprintf("  Device hw:%d,%d ID `%s', name `%s', %d subdevices (%d available)",
				card, dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);

			if (err)
			{
				Debugprintf("Error %d opening output device", err);
				goto nextdevice;
			}

			//	Get parameters for this device

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
			
			snd_pcm_hw_params_get_rate_min(pars, &ratemin, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &ratemax, NULL);

			if( min == max )
				if( min == 1 )
					Debugprintf("    1 channel,  sampling rate %u..%u Hz", ratemin, ratemax);
				else
					Debugprintf("    %d channels,  sampling rate %u..%u Hz", min, ratemin, ratemax);
			else
				Debugprintf("    %u..%u channels, sampling rate %u..%u Hz", min, max, ratemin, ratemax);

			// Add device to list

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

			strcpy(PlaybackNames[PlaybackCount++], NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:
			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }
		snd_ctl_close(handle);

nextcard:
			
		Debugprintf("");

		if (snd_card_next(&card) < 0)		// No more cards
			break;
	}

	return PlaybackCount;
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

	Debugprintf("Capture Devices\n");

	CaptureCount = 0;

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max, ratemin, ratemax;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	
	card = -1;

	if(snd_card_next(&card) < 0)
	{
		Debugprintf("No Devices");
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
    
		Debugprintf("Card %d, ID `%s', name `%s'", card, snd_ctl_card_info_get_id(info),
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
			Debugprintf("  Device hw:%d,%d ID `%s', name `%s', %d subdevices (%d available)",
				card, dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);
	
			if (err)
			{	
				Debugprintf("Error %d opening input device", err);
				goto nextdevice;
			}

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
			snd_pcm_hw_params_get_rate_min(pars, &ratemin, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &ratemax, NULL);

			if( min == max )
				if( min == 1 )
					Debugprintf("    1 channel,  sampling rate %u..%u Hz", ratemin, ratemax);
				else
					Debugprintf("    %d channels,  sampling rate %u..%u Hz", min, ratemin, ratemax);
			else
				Debugprintf("    %u..%u channels, sampling rate %u..%u Hz", min, max, ratemin, ratemax);

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

//			Debugprintf("%s", NameString);

			strcpy(CaptureNames[CaptureCount++], NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:
			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }
		snd_ctl_close(handle);
nextcard:

		Debugprintf("");
		if (snd_card_next(&card) < 0 )
			break;
	}
	return CaptureCount;
}

int OpenSoundPlayback(char * PlaybackDevice, int m_sampleRate, int channels, char * ErrorMsg)
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

	sprintf(buf1, "plug%s", PlaybackDevice);

	ptr = strchr(buf1, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name

	snd_pcm_hw_params_t *hw_params;
	
	if ((err = snd_pcm_open(&playhandle, buf1, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot open playback audio device %s (%s)",  buf1, snd_strerror(err));
		else
			Debugprintf("cannot open playback audio device %s (%s)",  buf1, snd_strerror(err));
		return false;
	}
		   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		Debugprintf("cannot allocate hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (playhandle, hw_params)) < 0) {
		Debugprintf("cannot initialize hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (playhandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			Debugprintf("cannot set playback access type (%s)", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (playhandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		Debugprintf("cannot setplayback  sample format (%s)", snd_strerror(err));
		return false;
	}

	if ((err = snd_pcm_hw_params_set_rate (playhandle, hw_params, m_sampleRate, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot set playback sample rate (%s)", snd_strerror(err));
		else
			Debugprintf("cannot set playback sample rate (%s)", snd_strerror(err));
		return false;
	}

	// Initial call has channels set to 1. Subequent ones set to what worked last time

	channels = 2;

	if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, channels)) < 0)
	{
		Debugprintf("cannot set play channel count to %d (%s)", channels, snd_strerror(err));
		
		if (channels == 2)
			return false;				// Shouldn't happen as should have worked before
		
		channels = 2;

		if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, 2)) < 0)
		{
			Debugprintf("cannot play set channel count to 2 (%s)", snd_strerror(err));
			return false;
		}
	}
	
	Debugprintf("Play using %d channels", channels);

	if ((err = snd_pcm_hw_params (playhandle, hw_params)) < 0) {
		Debugprintf("cannot set parameters (%s)", snd_strerror(err));
		return false;
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (playhandle)) < 0) {
		Debugprintf("cannot prepare audio interface for use (%s)", snd_strerror(err));
		return false;
	}

	Savedplaychannels = m_playchannels = channels;

	MaxAvail = snd_pcm_avail_update(playhandle);
//	Debugprintf("Playback Buffer Size %d", (int)MaxAvail);

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

	sprintf(buf1, "plug%s", CaptureDevice);

	ptr = strchr(buf1, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name
	
	if ((err = snd_pcm_open (&rechandle, buf1, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot open capture audio device %s (%s)",  buf1, snd_strerror(err));
		else
			Debugprintf("cannot open capture audio device %s (%s)",  buf1, snd_strerror(err));
		return false;
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		Debugprintf("cannot allocate capture hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (rechandle, hw_params)) < 0) {
		Debugprintf("cannot initialize capture hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (rechandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			Debugprintf("cannot set capture access type (%s)", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (rechandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		Debugprintf("cannot set capture sample format (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_rate (rechandle, hw_params, m_sampleRate, 0)) < 0) {
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot set capture sample rate (%s)", snd_strerror(err));
		else
			Debugprintf("cannot set capture sample rate (%s)", snd_strerror(err));
		return false;
	}

//	m_recchannels = 1;

//	if (UseLeft == 0 || UseRight == 0)
		m_recchannels = 2;					// L/R implies stereo
	
	if ((err = snd_pcm_hw_params_set_channels (rechandle, hw_params, m_recchannels)) < 0)
	{
		if (ErrorMsg)
			sprintf (ErrorMsg, "cannot set rec channel count to %d (%s)" ,m_recchannels, snd_strerror(err));
		else
			Debugprintf("cannot set rec channel count to %d (%s)", m_recchannels, snd_strerror(err));
	
		if (m_recchannels  == 1)
		{
			m_recchannels = 2;

			if ((err = snd_pcm_hw_params_set_channels (rechandle, hw_params, 2)) < 0)
			{
				Debugprintf("cannot set rec channel count to 2 (%s)", snd_strerror(err));
				return false;
			}
			if (ErrorMsg)
				sprintf (ErrorMsg, "Record channel count set to 2 (%s)", snd_strerror(err));
			else
				Debugprintf("Record channel count set to 2 (%s)", snd_strerror(err));
		}
	}
	
	if ((err = snd_pcm_hw_params (rechandle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)", snd_strerror(err));
		return false;
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (rechandle)) < 0) {
		Debugprintf("cannot prepare audio interface for use (%s)", snd_strerror(err));
		return FALSE;
	}

	Debugprintf("Capture using %d channels", m_recchannels);

	int i;
	short buf[256];

	for (i = 0; i < 10; ++i)
	{
		if ((err = snd_pcm_readi (rechandle, buf, 128)) != 128)
		{
			Debugprintf("read from audio interface failed (%s)",
				 snd_strerror (err));
		}
	}

//	Debugprintf("Read got %d", err);

 	return TRUE;
}

int OpenSoundCard(char * CaptureDevice, char * PlaybackDevice, int c_sampleRate, int p_sampleRate, char * ErrorMsg)
{
	int Channels = 1;

	
	if (UseLeft == 0)
		printf("Using Right Channel of soundcard\n");
	if (UseRight == 0)
		printf("Using Left Channel of soundcard\n");

	Debugprintf("Opening Playback Device %s Rate %d", PlaybackDevice, p_sampleRate);

//	if (UseLeft == 0 || UseRight == 0)
	Channels = 2;						// L or R implies stereo

	if (OpenSoundPlayback(PlaybackDevice, p_sampleRate, Channels, ErrorMsg))
	{
#ifdef SHARECAPTURE

		// Close playback device so it can be shared
		
		if (playhandle)
		{
			snd_pcm_close(playhandle);
			playhandle = NULL;
		}
#endif
		Debugprintf("Opening Capture Device %s Rate %d", CaptureDevice, c_sampleRate);
		return OpenSoundCapture(CaptureDevice, c_sampleRate, ErrorMsg);
	}
	else
		return false;
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
//	Debugprintf("avail before play returned %d", (int)avail);

	if (avail < 0)
	{
		if (avail != -32)
			Debugprintf("Playback Avail Recovering from %d ..", (int)avail);
		snd_pcm_recover(playhandle, avail, 1);

		avail = snd_pcm_avail_update(playhandle);

		if (avail < 0)
			Debugprintf("avail play after recovery returned %d", (int)avail);
	}
	
	maxavail = avail;

//	Debugprintf("Tosend %d Avail %d", nSamples, (int)avail);

	while (avail < nSamples)
	{
		txSleep(10);
		avail = snd_pcm_avail_update(playhandle);
//		Debugprintf("After Sleep Tosend %d Avail %d", nSamples, (int)avail);
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
/*

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
//			if (UseLeft)
				*(sampptr++) = input[0];
	//		else
		//		*(sampptr++) = 0;

			//if (UseRight)
				*(sampptr++) = input[0];
//			else
	//			*(sampptr++) = 0;

			input ++;
		}
	}
*/
	ret = snd_pcm_writei(playhandle, input, nSamples);

	if (ret < 0)
	{
//		Debugprintf("Write Recovering from %d ..", ret);
		snd_pcm_recover(playhandle, ret, 1);
		ret = snd_pcm_writei(playhandle, samples, nSamples);
//		Debugprintf("Write after recovery returned %d", ret);
	}

	avail = snd_pcm_avail_update(playhandle);
	return ret;

}
/*
int xSoundCardClearInput()
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
		Debugprintf("Discard Recovering from %d ..", avail);
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
//			Debugprintf("Discarded %d samples from card", ret);
			avail = snd_pcm_avail_update(rechandle);

//			Debugprintf("Discarding %d samples from card", avail);
	}
	return 0;
}
*/

int SoundCardRead(short * input, unsigned int nSamples)
{
	short samples[65536];
	int n;
	int ret;
	int avail;
	int start;

	if (rechandle == NULL)
		return 0;

	avail = snd_pcm_avail_update(rechandle);

	if (avail < 0)
	{
		Debugprintf("avail Recovering from %d ..", avail);
		if (rechandle)
		{
			snd_pcm_close(rechandle);
			rechandle = NULL;
		}

		OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);
//		snd_pcm_recover(rechandle, avail, 0);
		avail = snd_pcm_avail_update(rechandle);
		Debugprintf("After avail recovery %d ..", avail);
	}

	if (avail < nSamples)
		return 0;

//	Debugprintf("ALSARead available %d", avail);

	ret = snd_pcm_readi(rechandle, samples, nSamples);

	if (ret < 0)
	{
		Debugprintf("RX Error %d", ret);
//		snd_pcm_recover(rechandle, avail, 0);
		if (rechandle)
		{
			snd_pcm_close(rechandle);
			rechandle = NULL;
		}

		OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);
//		snd_pcm_recover(rechandle, avail, 0);
		avail = snd_pcm_avail_update(rechandle);
		Debugprintf("After Read recovery Avail %d ..", avail);

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
		if (UseLeft)
			start = 0;
		else
			start = 1;

		for (n = start; n < (ret * 2); n+=2)			// return alternate
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

	if (playhandle)
		SoundCardWrite(buf, n);

//	txSleep(10);				// Run buckground while waiting 

	Index = !Index;
	return &buffer[Index][0];
}

short loopbuff[1200];		// Temp for testing - loop sent samples to decoder


//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

unsigned short * SoundInit();

void InitSound(BOOL Quiet)
{
	GetInputDeviceCollection();
	GetOutputDeviceCollection();
	
	OpenSoundCard(CaptureDevice, PlaybackDevice, 12000, 12000, NULL);

	printf("InitSound %s %s\n", CaptureDevice, PlaybackDevice);

	DMABuffer = SoundInit();
}

int min = 0, max = 0, lastlevelreport = 0, lastlevelGUI = 0;
UCHAR CurrentLevel = 0;		// Peak from current samples


void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	if (SoundCardRead(&inbuffer[0][0], ReceiveSize))
	{
		// returns ReceiveSize or none

		short * ptr = &inbuffer[0][0];
		int i;

		for (i = 0; i < ReceiveSize; i++)
		{
			if (*(ptr) < min)
				min = *ptr;
			else if (*(ptr) > max)
				max = *ptr;
			ptr++;
		}


		CurrentLevel = ((max - min) * 75) /32768;	// Scale to 150 max

		if ((Now - lastlevelGUI) > 2000)	// 2 Secs
		{
				lastlevelGUI = Now;

			if ((Now - lastlevelreport) > 10000)	// 10 Secs
			{
				char HostCmd[64];
				lastlevelreport = Now;

				sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);

				WriteDebugLog(LOGDEBUG, "Input peaks = %d, %d", min, max);
			}
			min = max = 0;							// Every 2 secs
		}

		ProcessNewSamples(&inbuffer[0][0], ReceiveSize);
	}
} 

void StopCapture()
{
	Capturing = FALSE;

#ifdef SHARECAPTURE

	// Stopcapture is only called when we are about to transmit, so use it to open plaback device. We don't keep
	// it open all the time to facilitate sharing.

	OpenSoundPlayback(SavedPlaybackDevice, SavedPlaybackRate, Savedplaychannels, NULL);
#endif
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

//	Debugprintf("Start Capture");
}
void CloseSound()
{ 
	CloseSoundCard();
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

void SoundFlush()
{
	// Append Trailer then send remaining samples

	snd_pcm_status_t *status = NULL;
	int err, res;
	char strFault[100] = "";


	if (Loopback)
		ProcessNewSamples(&buffer[Index][0], Number);

	SendtoCard(&buffer[Index][0], Number);

	// Wait for tx to complete

	while (1 && playhandle)
	{
//		snd_pcm_sframes_t avail = snd_pcm_avail_update(playhandle);

//		Debugprintf("Waiting for complete. Avail %d Max %d", avail, MaxAvail);

		snd_pcm_status_alloca(&status);					// alloca allocates once per function, does not need a free

		if ((err=snd_pcm_status(playhandle, status))!=0)
		{
    		Debugprintf("snd_pcm_status() failed: %s",snd_strerror(err));
			break;
		}
	 
		res = snd_pcm_status_get_state(status);

//		Debugprintf("PCM Status = %d", res);

		if (res != SND_PCM_STATE_RUNNING)				// If sound system is not running then it needs data
//		if (MaxAvail - avail < 100)	
		{
			// Send complete - Restart Capture

			OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, strFault);	
			break;
		}
		usleep(50000);
	}
	// I think we should turn round the link here. I dont see the point in
	// waiting for MainPoll

#ifdef SHARECAPTURE
	if (playhandle)
	{
		snd_pcm_close(playhandle);
		playhandle = NULL;
	}
#endif
	SoundIsPlaying = FALSE;

	StartCapture();
	return;
}


// GPIO access stuff for PTT on PI

#ifdef __ARM_ARCH

/*
   tiny_gpio.c
   2016-04-30
   Public Domain
*/
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GPSET0 7
#define GPSET1 8

#define GPCLR0 10
#define GPCLR1 11

#define GPLEV0 13
#define GPLEV1 14

#define GPPUD     37
#define GPPUDCLK0 38
#define GPPUDCLK1 39

unsigned piModel;
unsigned piRev;

static volatile uint32_t  *gpioReg = MAP_FAILED;

#define PI_BANK (gpio>>5)
#define PI_BIT  (1<<(gpio&0x1F))

/* gpio modes. */

void gpioSetMode(unsigned gpio, unsigned mode)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   gpioReg[reg] = (gpioReg[reg] & ~(7<<shift)) | (mode<<shift);
}

int gpioGetMode(unsigned gpio)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   return (*(gpioReg + reg) >> shift) & 7;
}

/* Values for pull-ups/downs off, pull-down and pull-up. */

#define PI_PUD_OFF  0
#define PI_PUD_DOWN 1
#define PI_PUD_UP   2

void gpioSetPullUpDown(unsigned gpio, unsigned pud)
{
   *(gpioReg + GPPUD) = pud;

   usleep(20);

   *(gpioReg + GPPUDCLK0 + PI_BANK) = PI_BIT;

   usleep(20);
  
   *(gpioReg + GPPUD) = 0;

   *(gpioReg + GPPUDCLK0 + PI_BANK) = 0;
}

int gpioRead(unsigned gpio)
{
   if ((*(gpioReg + GPLEV0 + PI_BANK) & PI_BIT) != 0) return 1;
   else                                         return 0;
}
void gpioWrite(unsigned gpio, unsigned level)
{
   if (level == 0)
	   *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else
	   *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;
}

void gpioTrigger(unsigned gpio, unsigned pulseLen, unsigned level)
{
   if (level == 0) *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else            *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;

   usleep(pulseLen);

   if (level != 0) *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else            *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;
}

/* Bit (1<<x) will be set if gpio x is high. */

uint32_t gpioReadBank1(void) { return (*(gpioReg + GPLEV0)); }
uint32_t gpioReadBank2(void) { return (*(gpioReg + GPLEV1)); }

/* To clear gpio x bit or in (1<<x). */

void gpioClearBank1(uint32_t bits) { *(gpioReg + GPCLR0) = bits; }
void gpioClearBank2(uint32_t bits) { *(gpioReg + GPCLR1) = bits; }

/* To set gpio x bit or in (1<<x). */

void gpioSetBank1(uint32_t bits) { *(gpioReg + GPSET0) = bits; }
void gpioSetBank2(uint32_t bits) { *(gpioReg + GPSET1) = bits; }

unsigned gpioHardwareRevision(void)
{
   static unsigned rev = 0;

   FILE * filp;
   char buf[512];
   char term;
   int chars=4; /* number of chars in revision string */

   if (rev) return rev;

   piModel = 0;

   filp = fopen ("/proc/cpuinfo", "r");

   if (filp != NULL)
   {
      while (fgets(buf, sizeof(buf), filp) != NULL)
      {
         if (piModel == 0)
         {
            if (!strncasecmp("model name", buf, 10))
            {
               if (strstr (buf, "ARMv6") != NULL)
               {
                  piModel = 1;
                  chars = 4;
               }
               else if (strstr (buf, "ARMv7") != NULL)
               {
                  piModel = 2;
                  chars = 6;
               }
               else if (strstr (buf, "ARMv8") != NULL)
               {
                  piModel = 2;
                  chars = 6;
               }
            }
         }

         if (!strncasecmp("revision", buf, 8))
         {
            if (sscanf(buf+strlen(buf)-(chars+1),
               "%x%c", &rev, &term) == 2)
            {
               if (term != '\n') rev = 0;
            }
         }
      }

      fclose(filp);
   }
   return rev;
}

int gpioInitialise(void)
{
   int fd;

   piRev = gpioHardwareRevision(); /* sets piModel and piRev */

   fd = open("/dev/gpiomem", O_RDWR | O_SYNC) ;

   if (fd < 0)
   {
      fprintf(stderr, "failed to open /dev/gpiomem\n");
      return -1;
   }

   gpioReg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

   close(fd);

   if (gpioReg == MAP_FAILED)
   {
      fprintf(stderr, "Bad, mmap failed\n");
      return -1;
   }
   return 0;
}


	
#endif



int stricmp(const unsigned char * pStr1, const unsigned char *pStr2)
{
    unsigned char c1, c2;
    int  v;

	if (pStr1 == NULL)
	{
		if (pStr2)
			Debugprintf("stricmp called with NULL 1st param - 2nd %s ", pStr2);
		else
			Debugprintf("stricmp called with two NULL params");

		return 1;
	}


    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* The casts are necessary when pStr1 is shorter & char is signed */
        v = tolower(c1) - tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') );

    return v;
}

char Leds[8]= {0};
unsigned int PKTLEDTimer = 0;



static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};


HANDLE OpenCOMPort(VOID * Port, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	;
	char buf[100];

	//	Linux Version.

	int fd;
	int hwflag = 0;
	u_long param = 1;
	struct termios term;
	struct speed_struct *s;

	// As Serial ports under linux can have all sorts of odd names, the code assumes that
	// they are symlinked to a com1-com255 in the BPQ Directory (normally the one it is started from

	if ((fd = open(Port, O_RDWR | O_NDELAY)) == -1)
	{
		if (Quiet == 0)
		{
			perror("Com Open Failed");
			sprintf(buf, " %s could not be opened", (char *)Port);
			Debugprintf(buf);
		}
		return 0;
	}

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)
		if (s->user_speed == speed)
			break;

	if (s->user_speed == -1)
	{
		fprintf(stderr, "tty_speed: invalid speed %d", speed);
		return FALSE;
	}

	if (tcgetattr(fd, &term) == -1)
	{
		perror("tty_speed: tcgetattr");
		return FALSE;
	}

	cfmakeraw(&term);
	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	Debugprintf("LinBPQ Port %s fd %d", Port, fd);

	return fd;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	//	Some systems seem to have a very small max write size

	int ToSend = BytesToWrite;
	int Sent = 0, ret;

	while (ToSend)
	{
		ret = write(fd, &Block[Sent], ToSend);

		if (ret >= ToSend)
			return TRUE;

		if (ret == -1)
		{
			if (errno != 11 && errno != 35)					// Would Block
				return FALSE;

			usleep(10000);
			ret = 0;
		}

		Sent += ret;
		ToSend -= ret;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	close(fd);
}

VOID COMSetDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &status);
}

VOID COMClearDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &status);
}

VOID COMSetRTS(HANDLE fd)
{
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1)
		perror("ARDOP PTT TIOCMGET");
	status |= TIOCM_RTS;
	if (ioctl(fd, TIOCMSET, &status) == -1)
		perror("ARDOP PTT TIOCMSET");
}

VOID COMClearRTS(HANDLE fd)
{
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1)
		perror("ARDOP PTT TIOCMGET");
	status &= ~TIOCM_RTS;
	if (ioctl(fd, TIOCMSET, &status) == -1)
		perror("ARDOP PTT TIOCMSET");
}

