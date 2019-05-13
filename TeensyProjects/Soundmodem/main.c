/*****************************************************************************/

/*
 *      main.c  --  Soundmodem main.
 *
 *      Copyright (C) 1999-2001, 2003, 2010
 *        Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "soundio.h"
#include "simd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>


// This sets default TXDELAY, PERSIST, SLOTTIME, DUP, TAIL

struct state state = {
	NULL, NULL, NULL, {250, 64, 100, 0, 0}};


void InitSound(int SampleRate, int Report);
void DemodAFSKinit(void *state);
void DemodFSKinit(void *state);
void mainLoop();

char VersionString[] = "Teensy Packet TNC by G8BPQ Version 1.1 May 2019\r\n"
						"based on Soundmodem by Thomas Sailer";

int VersionNo = 11;		

int Baud = 1200;
BOOL AFSK = TRUE;
BOOL FSK = FALSE;
BOOL PSK = FALSE;
int samplerate;
int centreFreq = 1700;

//int Baud = 1200;
//BOOL AFSK = TRUE;

int TXLevel;				// 300 mV p-p Used on Teensy
int RXLevel;				// Configured Level - zero means auto tune
int autoRXLevel;			// calculated level
int logcheck(int x)
{
	return 1;
}

void audiowrite(struct modemchannel *chan, const int16_t *samples, unsigned int nr)
{
	while (nr--)
		SampleSink(*(samples++));
}

short PSKSamples[5000];
unsigned int PSKSampleCount = 0;

int maxpsk = 0;

void audioread(struct modemchannel *chan, int16_t *samples, unsigned int nr, u_int16_t tim)
{
	// This is called by psk decode and I can't see an easy way to restructure
	// this. So in PSK modes this becomes the main background loop, polling 
	// for new samples or host input

	if (nr > 3000)
		WriteDebugLog(7, "NR > 3000 %d", nr);

	while(PSKSampleCount < nr)
	{
		mainLoop();
		Sleep(1);
#ifdef TEENSY
		PlatformSleep();
#ifdef i2cSlaveSupport
		i2cloop();
#endif
#endif
	}

	memcpy(samples, PSKSamples, nr * 2);
	PSKSampleCount -= nr;

	if (PSKSampleCount)
		memmove(PSKSamples, &PSKSamples[nr], PSKSampleCount * 2);
}



u_int16_t audiocurtime(struct modemchannel *chan)
{
        struct audioio *audioio = chan->state->audioio;

        if (!audioio || !audioio->curtime)
                return 0;
        return audioio->curtime(audioio);
}


struct modulator *modchain = &afskmodulator;
struct demodulator *demodchain = &afskdemodulator;

void pktProcessNewSamples(short * buf, int count)
{
	if (AFSK)
	{
		// downsample to 12000

		if (samplerate != 12000)
		{
			int n = samplerate / 12000;
			int i, j = 0;

			for (i = 0; i < count; i += n)		// use every n samples
				buf[j++] = buf[i];

			count = j;

		}
		DemodAFSK(buf, count);

	}
	else if (FSK)
		DemodFSK(buf, count);
/*	else
	{
		memcpy(&PSKSamples[PSKSampleCount], buf, count * 2);
		PSKSampleCount += count;
		if (PSKSampleCount > maxpsk)
		{
			WriteDebugLog(7, "Max PSK Samples %d", PSKSampleCount);
			maxpsk =PSKSampleCount;
		}
	}
*/
}

int wavmain(int argc, char *argv[]);

void mainLoop()
{
	PollReceivedSamples();
	HostPoll();
	pkttransmitloop(&state);
	Sleep(1);
}

void SetDefaultKISSParams(unsigned int txdelay, unsigned int ppersist,
						  unsigned int slottime, unsigned int fullduplex, unsigned int txtail)
{
	state.chacc.txdelay = txdelay;
	state.chacc.ppersist = ppersist;
	state.chacc.slottime = slottime;
	state.chacc.fullduplex = fullduplex;
	state.chacc.txtail =txtail;
}

#ifdef TEENSY
int SoundModemInit()
#else
int main(int argc, char *argv[])
#endif
{
	struct modemchannel *chan;
	int sr;
	int P1 = 0;
	int P2 = 0;
	int P3 = 0;
	int P4 = 0;

#ifndef TEENSY
	
	wavmain(argc, argv);
	HostInit();

#endif

	afskmodulator.next = &fskmodulator;
	fskmodulator.next = &pskmodulator;
	afskdemodulator.next = &fskdemodulator;
	fskdemodulator.next = &pskdemodulator;

// Set up single channel

	if (!(chan = malloc(sizeof(struct modemchannel))))
		WriteDebugLog(MLOG_FATAL, "out of memory\n");

	memset(chan, 0, sizeof(struct modemchannel));
	chan->next = state.channels;
	chan->state = &state;

	if (FSK)
	{
		P1 = Baud;
		P2 = 0;
		samplerate = 48000;
		chan->mod = &fskmodulator;
		chan->demod = &fskdemodulator;
	}
	else if (AFSK)
	{
#ifdef TEENSY
		samplerate = 12000;
#else
		samplerate = 12000;
#endif
		P1 = Baud;

		if (Baud == 300)
		{
			P2 = centreFreq - 100;
			P3 = centreFreq + 100;
		}
		else if (Baud == 1200)
		{
			P2 = centreFreq - 500;
			P3 = centreFreq + 500;
		}
		else
		{
			P2 = 500;
			P3 = 2900;
			P2 = centreFreq - 1200;
			P3 = centreFreq + 1200;
		}
		chan->mod = &afskmodulator;
		chan->demod = &afskdemodulator;
	}
	else
	{
		// PSK

		samplerate = 9600;
		chan->mod = &pskmodulator;
		chan->demod = &pskdemodulator;
	}
	chan->modstate = NULL;
	chan->demodstate = NULL;

	pktinit(chan);

	chan->modstate = chan->mod->config(chan, &sr, P1, P2, P3);
 	chan->demodstate = chan->demod->config(chan, &sr, P1, P2, P3);

	state.channels = chan;

	// if 300 add more channels

	if (Baud == 300)
	{
		if (0)
		{
			if (!(chan = malloc(sizeof(struct modemchannel))))
				WriteDebugLog(MLOG_FATAL, "out of memory\n");

			memset(chan, 0, sizeof(struct modemchannel));
			chan->next = state.channels;
			chan->state = &state;

			chan->mod = &afskmodulator;
			chan->demod = &afskdemodulator;

			chan->modstate = NULL;
			chan->demodstate = NULL;

			pktinit(chan);

			chan->modstate = chan->mod->config(chan, &sr, P1, P2, P3);
		 	chan->demodstate = chan->demod->config(chan, &sr, P1, P2, P3);
			state.channels = chan;

		}
	}

	InitSound(samplerate, 1);

	// init channel(s)

	for (chan = state.channels; chan; chan = chan->next)
	{
		if (chan->demod)
			chan->demod->init(chan->demodstate, samplerate, &chan->rxbitrate);
  
		if (chan->mod)
			chan->mod->init(chan->modstate, samplerate);

		if (AFSK)
			DemodAFSKinit(chan->demodstate);		// G8BPQ 
		else if (FSK)
			DemodFSKinit(chan->demodstate);		// G8BPQ 
		else
		{
			DemodPSKinit(chan->demodstate);
#ifndef TEENSY
			chan->demod->demodulate(chan->demodstate);
#endif
		}
	}

	WriteDebugLog(7, "Centre Freq %d Baud %d AFSK %d FSK %d PSK %d Samplerate %d", centreFreq, Baud, AFSK, FSK, PSK, samplerate);

#ifndef TEENSY

	while (1)
		mainLoop();
#endif

	return 0;
}

RunPSKReceive()
{
	struct modemchannel *chan;

	for (chan = state.channels; chan; chan = chan->next)
	{
		if (PSK)
			chan->demod->demodulate(chan->demodstate);

	// Note this does not return!
	}
}

