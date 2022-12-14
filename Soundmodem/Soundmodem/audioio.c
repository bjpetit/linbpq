/*****************************************************************************/

/*
 *      audioio.c  --  Audio I/O.
 *
 *      Copyright (C) 1999-2000
 *        Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Please note that the GPL allows you to use the driver, NOT the radio.
 *  In order to use the radio, you need a license from the communications
 *  authority of your country.
 *
 */

/*****************************************************************************/

#define _GNU_SOURCE
#define _REENTRANT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "soundio.h"
#include "audioio.h"

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/audioio.h>
#endif
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

/* ---------------------------------------------------------------------- */

#define AUDIOIBUFSIZE 4096

struct audioio_unix {
	struct audioio audioio;
	char audiopath[64];
	unsigned int samplerate;
       	int audiofd;
	unsigned int fragsize;
	unsigned int flags;
	unsigned int ptr;
	u_int16_t ptime;
	int16_t ibuf[AUDIOIBUFSIZE];
};

#if defined(HAVE_SYS_SOUNDCARD_H)
#define AUDIOPATH "/dev/dsp"
#else
#define AUDIOPATH "/dev/audio"
#endif

struct modemparams ioparams_soundcard[];

#undef AUDIOPATH

#define CAP_HALFDUPLEX   0x100

#define FLG_READING      0x1000
#define FLG_HALFDUPLEXTX 0x2000
#define FLG_TERMINATERX  0x4000

#ifndef INFTIM
#define INFTIM (-1)
#endif

/* ---------------------------------------------------------------------- */

static void iorelease(struct audioio *aio);
static void iowrite(struct audioio *aio, const int16_t *samples, unsigned int nr);
static void ioread(struct audioio *aio, int16_t *samples, unsigned int nr, u_int16_t tim);
static u_int16_t iocurtime(struct audioio *aio);
static void iotransmitstart(struct audioio *aio);
static void iotransmitstop(struct audioio *aio);
static void ioterminateread(struct audioio *aio);

/* ---------------------------------------------------------------------- */

static int iomodetofmode(unsigned int flags)
{
	switch (flags & IO_RDWR) {
	default:
	case IO_RDONLY:
		return O_RDONLY;

	case IO_WRONLY:
		return O_WRONLY;

	case IO_RDWR:
		return O_RDWR;
	}
}

/* ---------------------------------------------------------------------- */
/*
 * Linux OSS audio
 */

#if defined(HAVE_SYS_SOUNDCARD_H)

struct audioio *ioopen_soundcard(unsigned int *samplerate, unsigned int flags, const char *params[])
{
        int sndparam, i;
	const char *audiopath = params[0];
        audio_buf_info abinfoi, abinfoo;
	struct audioio_unix *audioio;

	audioio = calloc(1, sizeof(struct audioio_unix));
	if (!audioio)
		return NULL;
	audioio->audioio.release = iorelease;
	if (flags & IO_RDONLY) {
		audioio->audioio.terminateread = ioterminateread;
		audioio->audioio.read = ioread;
		audioio->audioio.curtime = iocurtime;
	}
	if (flags & IO_WRONLY) {
		audioio->audioio.transmitstart = iotransmitstart;
		audioio->audioio.transmitstop = iotransmitstop;
		audioio->audioio.write = iowrite;
	}
	audioio->samplerate = *samplerate;
        pthread_cond_init(&audioio->iocond, NULL);
        pthread_mutex_init(&audioio->iomutex, NULL);
        audioio->flags = flags & IO_RDWR;
	audioio->ptr = audioio->ptime = 0;
	if (!audiopath)
		audiopath = "/dev/dsp";
	strncpy(audioio->audiopath, audiopath, sizeof(audioio->audiopath));
        logprintf(MLOG_DEBUG, "audio: starting \"%s\"\n", audioio->audiopath);
        if ((audioio->audiofd = open(audioio->audiopath, iomodetofmode(audioio->flags))) < 0) {
                logprintf(MLOG_ERROR, "audio: Error, cannot open \"%s\"\n", audioio->audiopath);
		free(audioio);
                return NULL;
        }
        if (ioctl(audioio->audiofd, SNDCTL_DSP_GETCAPS, &sndparam) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot get capabilities\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
	if (!((~audioio->flags) & IO_RDWR)) {
		if (params[1] && params[1][0] != '0') {
			audioio->flags |= CAP_HALFDUPLEX;
			logprintf(MLOG_INFO, "audio: forcing half duplex mode\n");
		} else if (!(sndparam & DSP_CAP_DUPLEX)) {
			audioio->flags |= CAP_HALFDUPLEX;
			logprintf(MLOG_INFO, "audio: Soundcard does not support full duplex, using half duplex mode\n");
		} else if (ioctl(audioio->audiofd, SNDCTL_DSP_SETDUPLEX, 0) == -1) {
			logprintf(MLOG_ERROR, "audio: Error, cannot set duplex mode\n");
			close(audioio->audiofd);
			free(audioio);
			return NULL;
		}
	}
        /* set fragment size so we have approx. 10-20ms wakeup latency */
        i = audioio->samplerate / 50;
        sndparam = 0xffff0000;
        while (i) {
                sndparam++;
                i >>= 1;
        }
	audioio->fragsize = sndparam;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFRAGMENT, &sndparam) == -1) 
                logprintf(MLOG_ERROR, "audio: Error, cannot set fragment size\n");
        sndparam = AFMT_S16_LE; /* we want 16 bits/sample signed */
        /* little endian; works only on little endian systems! */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFMT, &sndparam) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set sample size\n");
		free(audioio);
                return NULL;
        }
        if (sndparam != AFMT_S16_LE) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set sample size to 16 bits\n");
		free(audioio);
                return NULL;
        }
        sndparam = 0;   /* we want only 1 channel */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_STEREO, &sndparam) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set the channel number\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
        if (sndparam != 0) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set the channel number to 1\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
        sndparam = audioio->samplerate;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SPEED, &sndparam) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set the sample rate\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
	audioio->samplerate = sndparam;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_NONBLOCK, 0) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set nonblocking mode\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
	memset(&abinfoi, 0, sizeof(abinfoi));
	memset(&abinfoo, 0, sizeof(abinfoo));
        if ((flags & IO_RDONLY) && ioctl(audioio->audiofd, SNDCTL_DSP_GETISPACE, &abinfoi) == -1)
                logprintf(MLOG_ERROR, "audio: Error, cannot get input buffer parameters\n");
        else if ((flags & IO_WRONLY) && ioctl(audioio->audiofd, SNDCTL_DSP_GETOSPACE, &abinfoo) == -1)
                logprintf(MLOG_ERROR, "audio: Error, cannot get output buffer parameters\n");
        else {
                logprintf(MLOG_INFO, "audio: sample rate %u input fragsz %u numfrags %u output fragsz %u numfrags %u\n",
                          audioio->samplerate, abinfoi.fragsize, abinfoi.fragstotal, abinfoo.fragsize, abinfoo.fragstotal);
        }
	*samplerate = audioio->samplerate;
        return &audioio->audioio;
}

static inline void iotxend(struct audioio_unix *audioio)
{
	int sndparam;
	short s;

        fcntl(audioio->audiofd, F_SETFL, fcntl(audioio->audiofd, F_GETFL, 0) & ~O_NONBLOCK);
	if (ioctl(audioio->audiofd, SNDCTL_DSP_SYNC, 0))
		logerr(MLOG_ERROR, "ioctl: SNDCTL_DSP_SYNC");
        fcntl(audioio->audiofd, F_SETFL, fcntl(audioio->audiofd, F_GETFL, 0) | O_NONBLOCK);
	if (!(audioio->flags & CAP_HALFDUPLEX))
		return;
	/* the only reliable method seems to be to reopen the audio device :( */
	close(audioio->audiofd);
        if ((audioio->audiofd = open(audioio->audiopath, O_RDWR)) < 0)
                logprintf(MLOG_FATAL, "audio: Error, cannot open \"%s\"\n", audioio->audiopath);
        /* set fragment size so we have approx. 10-20ms wakeup latency */
	sndparam = audioio->fragsize;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFRAGMENT, &sndparam) == -1) 
                logprintf(MLOG_ERROR, "audio: Error, cannot set fragment size\n");
        sndparam = AFMT_S16_LE; /* we want 16 bits/sample signed */
        /* little endian; works only on little endian systems! */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFMT, &sndparam) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set sample size\n");
        if (sndparam != AFMT_S16_LE)
                logprintf(MLOG_FATAL, "audio: Error, cannot set sample size to 16 bits\n");
        sndparam = 0;   /* we want only 1 channel */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_STEREO, &sndparam) == -1) 
                logprintf(MLOG_FATAL, "audio: Error, cannot set the channel number\n");
        if (sndparam != 0)
                logprintf(MLOG_FATAL, "audio: Error, cannot set the channel number to 1\n");
        sndparam = audioio->samplerate;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SPEED, &sndparam) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set the sample rate\n");
        if (ioctl(audioio->audiofd, SNDCTL_DSP_NONBLOCK, 0) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set nonblocking mode\n");
	read(audioio->audiofd, &s, sizeof(s));
}

static inline void iotxstart(struct audioio_unix *audioio)
{
	int sndparam;
	short s = 0;

	if (!(audioio->flags & CAP_HALFDUPLEX)) {
		write(audioio->audiofd, &s, sizeof(s));
		return;
	}
	/* the only reliable method seems to be to reopen the audio device :( */
	close(audioio->audiofd);
        if ((audioio->audiofd = open(audioio->audiopath, O_RDWR)) < 0)
                logprintf(MLOG_FATAL, "audio: Error, cannot open \"%s\"\n", audioio->audiopath);
        /* set fragment size so we have approx. 10-20ms wakeup latency */
	sndparam = audioio->fragsize;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFRAGMENT, &sndparam) == -1) 
                logprintf(MLOG_ERROR, "audio: Error, cannot set fragment size\n");
        sndparam = AFMT_S16_LE; /* we want 16 bits/sample signed */
        /* little endian; works only on little endian systems! */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SETFMT, &sndparam) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set sample size\n");
        if (sndparam != AFMT_S16_LE)
                logprintf(MLOG_FATAL, "audio: Error, cannot set sample size to 16 bits\n");
        sndparam = 0;   /* we want only 1 channel */
        if (ioctl(audioio->audiofd, SNDCTL_DSP_STEREO, &sndparam) == -1) 
                logprintf(MLOG_FATAL, "audio: Error, cannot set the channel number\n");
        if (sndparam != 0)
                logprintf(MLOG_FATAL, "audio: Error, cannot set the channel number to 1\n");
        sndparam = audioio->samplerate;
        if (ioctl(audioio->audiofd, SNDCTL_DSP_SPEED, &sndparam) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set the sample rate\n");
        if (ioctl(audioio->audiofd, SNDCTL_DSP_NONBLOCK, 0) == -1)
                logprintf(MLOG_FATAL, "audio: Error, cannot set nonblocking mode\n");
	sndparam = write(audioio->audiofd, &s, sizeof(s));
}

/* ---------------------------------------------------------------------- */
/*
 * Sun audio
 */

#elif defined(HAVE_SYS_AUDIOIO_H)

static inline int iomodetoflush(unsigned int flags)
{
	switch (flags & IO_RDWR) {
	default:
	case IO_RDONLY:
		return FLUSHR;

	case IO_WRONLY:
		return FLUSHW;

	case IO_RDWR:
		return FLUSHRW;
	}
}

struct audioio *ioopen_soundcard(unsigned int *samplerate, unsigned int flags, const char *params[])
{
	static int srtable[] = {
		8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000, -1
	};
	int *srptr = srtable;
	const char *audiopath = params[0];
        audio_info_t audioinfo;
        audio_info_t audioinfo2;
        audio_device_t audiodev;
	struct audioio_unix *audioio;

	audioio = calloc(1, sizeof(struct audioio_unix));
	if (!audioio)
		return NULL;
	audioio->audioio.release = iorelease;
	if (flags & IO_RDONLY) {
		audioio->audioio.terminateread = ioterminateread;
		audioio->audioio.read = ioread;
		audioio->audioio.curtime = iocurtime;
	}
	if (flags & IO_WRONLY) {
		audioio->audioio.transmitstart = iotransmitstart;
		audioio->audioio.transmitstop = iotransmitstop;
		audioio->audioio.write = iowrite;
	}
	audioio->samplerate = *samplerate;
        pthread_cond_init(&audioio->iocond, NULL);
        pthread_mutex_init(&audioio->iomutex, NULL);
        audioio->flags = flags & IO_RDWR;
	audioio->ptr = audioio->ptime = 0;
	if (!audiopath)
		audiopath = "/dev/audio";
        logprintf(MLOG_DEBUG, "audio: starting \"%s\"\n", audiopath);
        if ((audioio->audiofd = open(audiopath, iomodetofmode(audioio->flags))) < 0) {
                logprintf(MLOG_ERROR, "audio: Error, cannot open \"%s\"\n",  audiopath);
		free(audioio);
                return NULL;
        }
        if (ioctl(audioio->audiofd, AUDIO_GETDEV, &audiodev) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot get audio dev\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
        logprintf(MLOG_DEBUG, "audio: Audio device: name %s, ver %s, config %s\n",
		  audiodev.name, audiodev.version, audiodev.config);
        AUDIO_INITINFO(&audioinfo);
	while (srptr[0] < audioio->samplerate && srptr[1] != -1)
		srptr++;
	if (audioio->flags & IO_WRONLY) {
		audioinfo.play.sample_rate = srptr[0];
		audioinfo.play.channels = 1;
		audioinfo.play.precision = 16;
		audioinfo.play.encoding = AUDIO_ENCODING_LINEAR;
	}
	if (audioio->flags & IO_RDONLY) {
		audioinfo.record.sample_rate = srptr[0];
		audioinfo.record.channels = 1;
		audioinfo.record.precision = 16;
		audioinfo.record.encoding = AUDIO_ENCODING_LINEAR;
		//audioinfo.record.gain = 0x20;
		audioinfo.record.port = AUDIO_LINE_IN;
		//audioinfo.monitor_gain = 0;
	}
        if (ioctl(audioio->audiofd, AUDIO_SETINFO, &audioinfo) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot set audio params\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
        if (ioctl(audioio->audiofd, I_FLUSH, iomodetoflush(audioio->flags)) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot flush\n");
		close(audioio->audiofd);
 		free(audioio);
		return NULL;
        }
        if (ioctl(audioio->audiofd, AUDIO_GETINFO, &audioinfo2) == -1) {
                logprintf(MLOG_ERROR, "audio: Error, cannot get audio params\n");
		close(audioio->audiofd);
		free(audioio);
                return NULL;
        }
	audioio->samplerate = audioinfo2.record.sample_rate;
        if (params[1] && params[1][0] != '0') {
		audioio->flags |= CAP_HALFDUPLEX;
                logprintf(MLOG_INFO, "audio: forcing half duplex mode\n");
	}
        fcntl(audioio->audiofd, F_SETFL, fcntl(audioio->audiofd, F_GETFL, 0) | O_NONBLOCK);
        logprintf(MLOG_INFO, "audio: sample rate %u input buffer %u output buffer %u\n",
                  audioio->samplerate, audioinfo2.record.buffer_size, audioinfo2.play.buffer_size);
	*samplerate = audioio->samplerate;
        return &audioio->audioio;
}

static inline void iotxend(struct audioio_unix *audioio)
{
	if (ioctl(audioio->audiofd, AUDIO_DRAIN, 0))
		logerr(MLOG_ERROR, "ioctl: AUDIO_DRAIN");
}

static inline void iotxstart(struct audioio_unix *audioio)
{
        short s = 0;

        write(audioio->audiofd, &s, sizeof(s));
}

#endif

/* ---------------------------------------------------------------------- */

void ioinit_soundcard(void)
{
}

static void iorelease(struct audioio *aio)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
	free(audioio);
}

static void iowrite(struct audioio *aio, const int16_t *samples, unsigned int nr)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
}

static void ioread(struct audioio *aio, int16_t *samples, unsigned int nr, u_int16_t tim)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
}

static u_int16_t iocurtime(struct audioio *aio)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
}

static void iotransmitstart(struct audioio *aio)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
	iotxstart(audioio);
}

static void iotransmitstop(struct audioio *aio)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;
	short sbuf[256];
	unsigned int i, j;

}

static void ioterminateread(struct audioio *aio)
{
	struct audioio_unix *audioio = (struct audioio_unix *)aio;

   audioio->flags |= FLG_TERMINATERX;
}

/* ---------------------------------------------------------------------- */
