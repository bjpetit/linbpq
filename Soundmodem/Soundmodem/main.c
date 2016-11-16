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


struct state state = {
	NULL, NULL, NULL};

void InitSound(int SampleRate, int Report);

int Baud = 9600;

int logcheck(int x)
{
	return 1;
}


void audioread(struct modemchannel *chan, int16_t *samples, unsigned int nr, u_int16_t tim)
{
        struct audioio *audioio = chan->state->audioio;

        if (!audioio || !audioio->read) {
                return;
        }
        audioio->read(audioio, samples, nr, tim);
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

void ProcessNewSamples(short * buf, int count)
{
	if (Baud == 1200)
		DemodAFSK(buf, count);
	else
		DemodFSK(buf, count);
}

int wavmain(int argc, char *argv[]);

void mainLoop()
{
	PollReceivedSamples();
	HostPoll();
	pkttransmitloop(&state);
	Sleep(1);
}


#ifdef TEENSY
int SoundModemInit()
#else
int main(int argc, char *argv[])
#endif
{
	struct modemchannel *chan;
	int samplerate, sr;
	char p1[] = "1200";
	char p2[] = "1200";
	char p3[] = "2200";
	char p4[] = "12000";


	char * modparams[] = {p1, p2, p3, p4};

#ifndef TEENSY
	
	wavmain(argc, argv);
	HostInit();

#endif

	afskmodulator.next = &fskmodulator;
	afskdemodulator.next = &fskdemodulator;

// Set up single channel

	if (!(chan = malloc(sizeof(struct modemchannel))))
		WriteDebugLog(MLOG_FATAL, "out of memory\n");

	memset(chan, 0, sizeof(struct modemchannel));
	chan->next = state.channels;
	chan->state = &state;

	if (Baud == 9600)
	{
		modparams[0] = "9600";
		modparams[1] = NULL;
		samplerate = 48000;
		chan->mod = &fskmodulator;
		chan->demod = &fskdemodulator;
	}
	else
	{
		samplerate = 12000;
		chan->mod = &afskmodulator;
		chan->demod = &afskdemodulator;
	}
	chan->modstate = NULL;
	chan->demodstate = NULL;

	pktinit(chan, "");

	chan->modstate = chan->mod->config(chan, &sr, modparams);

	if (sr > samplerate)
		samplerate = sr;

	sr = samplerate;
 	chan->demodstate = chan->demod->config(chan, &sr, modparams);
	if (sr > samplerate)
		samplerate = sr;

	state.channels = chan;

	InitSound(samplerate, 1);

	// init channel(s)

	for (chan = state.channels; chan; chan = chan->next)
	{
		if (chan->demod)
			chan->demod->init(chan->demodstate, samplerate, &chan->rxbitrate);
  
		if (chan->mod)
			chan->mod->init(chan->modstate, samplerate);
	}

	if (Baud == 1200)
		DemodAFSKinit(state.channels->demodstate);		// G8BPQ 
	else
		DemodFSKinit(state.channels->demodstate);		// G8BPQ 

#ifndef TEENSY

	while (1)
		mainLoop();
#endif

	return 0;
}
