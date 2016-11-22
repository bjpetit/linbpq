/*****************************************************************************/

/*
 *      modem.c  --  Linux Userland Soundmodem AFSK modem.
 *
 *      Copyright (C) 1998-2000, 2003
 *        Thomas Sailer <t.sailer@alumni.ethz.ch>
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
 *
 */

/*****************************************************************************/
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "..\modem.h"
#include "costab.h"

#undef M_PI
#define M_PI 3.1415926f

/* --------------------------------------------------------------------- */

struct modstate {
	struct modemchannel *chan;
	unsigned int bps, f0, f1, notdiff, maxbitlen;
	unsigned int bitinc, bitph;
	unsigned int dds, ddsinc[2];
	unsigned int bit;
};

static const struct modemparams modparam;

static void *modconfig(struct modemchannel *chan, unsigned int *samplerate, int P1, int P2, int P3)
{
	struct modstate *s;
	
	if (!(s = malloc(sizeof(struct modstate))))
		WriteDebugLog(MLOG_FATAL, "out of memory\n");

	memset(s, 0, sizeof(struct modstate));

	s->chan = chan;
	s->bps= P1;
	s->f0 = P2;
	s->f1 = P3;

	s->notdiff = 0;
	*samplerate = 8 * s->bps;
	return s;
}

static void modinit(void *state, unsigned int samplerate)
{
	struct modstate *s = (struct modstate *)state;

	s->maxbitlen = (samplerate + s->bps - 1) / s->bps;
	s->bitinc = (s->bps << 16) / samplerate;
	s->ddsinc[0] = (s->f0 << 16) / samplerate;
	s->ddsinc[1] = (s->f1 << 16) / samplerate;
	s->bit = 0;
}

static void modsendbits(struct modstate *s, unsigned int bits, unsigned int nrsyms)
{
	int16_t sbuf[512];
	int16_t *sptr = sbuf, *eptr = sbuf + sizeof(sbuf)/sizeof(sbuf[0]);
	int samplessent = 0;

	while (nrsyms > 0)
	{
		if (s->bitph >= 0x10000)
		{
			s->bitph &= 0xffff;
			s->bit = (~((s->bit | s->notdiff) ^ bits)) & 1;
			bits >>= 1;
			nrsyms--;
		}
		SampleSink(COS(s->dds));
		samplessent++;
		s->dds += s->ddsinc[s->bit];
        s->bitph += s->bitinc;
	}
}

static void modmodulate(void *state, unsigned int txdelay)
{
	struct modstate *s = (struct modstate *)state;
	int i;
	unsigned char ch;

	i = txdelay * s->bps / 1000;
	do {
		modsendbits(s, 0x7e, 8);
		i -= 8;
	} while (i > 0);
	while (pktget(s->chan, &ch, 1))
		modsendbits(s, ch, 8);
	modsendbits(s, 0x7e, 8);
	modsendbits(s, 0x7e, 8);
}

struct modulator afskmodulator = {
	NULL,
	"afsk",
	NULL, //modparams,
	modconfig,
	modinit,
	modmodulate,
	free
};

/* --------------------------------------------------------------------- */

#define max(a, b) (((a) > (b)) ? (a) : (b))

/* RxFilter */
#define WINDOWEXPAND  1.5f
#define RXFILTLEN       16
#define RXFILTOVERBITS   3
#define RXFILTOVER      (1<<(RXFILTOVERBITS))
#define RXFILTFIDX(x)   (((x)>>(16-(RXFILTOVERBITS)))&(RXFILTOVER-1))
#define RXFILTFSAMP(x)  ((x)>>16)

struct demodstate {
	struct modemchannel *chan;
	unsigned int bps, f0, f1, notdiff, firlen;
        unsigned int srate;
        unsigned int rxbits;
        unsigned int rxphase;
        unsigned int rxphinc;
        int dcdcnt;
        unsigned int dcdtim;

	unsigned int div, divcnt;
	unsigned int pllinc, pll, pllthresh, pllcorr;
	unsigned int shreg, lastbit;
        int dcd_sum0, dcd_sum1, dcd_sum2;
        unsigned int dcd_time;
	u_int32_t raws, rawb;
	unsigned char dcd;

	union {
		struct {
			int32_t f0r[RXFILTOVER][RXFILTLEN];
			int32_t f0i[RXFILTOVER][RXFILTLEN];
			int32_t f1r[RXFILTOVER][RXFILTLEN];
			int32_t f1i[RXFILTOVER][RXFILTLEN];
		} f32;
		struct {
			int16_t f0r[RXFILTOVER][RXFILTLEN];
			int16_t f0i[RXFILTOVER][RXFILTLEN];
			int16_t f1r[RXFILTOVER][RXFILTLEN];
			int16_t f1i[RXFILTOVER][RXFILTLEN];
		} f16;
	} f;
};

/* --------------------------------------------------------------------- */

static const struct modemparams demodparams[1];

static void *demodconfig(struct modemchannel *chan, unsigned int *samplerate, int P1, int P2, int P3)
{
	struct demodstate *s;
	unsigned int f;

	if (!(s = malloc(sizeof(struct demodstate))))
		WriteDebugLog(MLOG_FATAL, "out of memory\n");

	memset(s, 0, sizeof(struct modstate));

	s->chan = chan;
	s->bps= P1;
	s->f0 = P2;
	s->f1 = P3;

	s->notdiff = 0;

	f = s->f0;
	if (s->f1 > f)
		f = s->f1;
	f += s->bps/2;
	f = (2*f) + (f >> 1);
	*samplerate = f;
	return s;
}

static int demfilter(struct demodstate *state, const int16_t *val, unsigned int phase)
{
        unsigned int fidx = RXFILTFIDX(phase);
        const int *c0r = state->f.f32.f0r[fidx];
        const int *c0i = state->f.f32.f0i[fidx];
        const int *c1r = state->f.f32.f1r[fidx];
        const int *c1i = state->f.f32.f1i[fidx];
        const int16_t *v1, *v2;
        unsigned int i;
        int st = 0, s;

        v1 = val + RXFILTFSAMP(phase);
        for (v2 = v1, s = 0, i = 0; i < RXFILTLEN; i++, v2++, c1r++)
                s += (*v2) * (*c1r);
        s >>= 16;
        st = s * s;
        for (v2 = v1, s = 0, i = 0; i < RXFILTLEN; i++, v2++, c1i++)
                s += (*v2) * (*c1i);
        s >>= 16;
        st += s * s;
        for (v2 = v1, s = 0, i = 0; i < RXFILTLEN; i++, v2++, c0r++)
                s += (*v2) * (*c0r);
        s >>= 16;
        st -= s * s;
        for (v2 = v1, s = 0, i = 0; i < RXFILTLEN; i++, v2++, c0i++)
                s += (*v2) * (*c0i);
        s >>= 16;
        st -= s * s;
        return st;
}

static void demod8bits(struct demodstate *s, int16_t *samples)
{
	unsigned int phinc = s->rxphinc;
	unsigned int halfphinc = phinc >> 1;
	unsigned int dcdinterv = phinc >> 2;
	unsigned int phasecorr = phinc >> 4;
	unsigned int totsamp = ((12 * phinc) >> 16) + RXFILTLEN;
	unsigned int phase, nr;
	int oldv = 0, newv = 0, midv = 0, thv = 0;

 //       samples = alloca(totsamp * sizeof(samples[0]));
      
//		audioread(s->chan, samples, totsamp, s->rxphase >> 16);
	// a block of 120 samples are passed in 

	// each bit is 10 samples (12000/1200) so why 120??

	phase = s->rxphase;
//	s->rxphase &= ~0xffff;
	oldv = demfilter(s, samples, phase);

	for (nr = 0; nr < 8; nr++)
	{
//		if (logcheck(260))
//		{
//			unsigned int i;
//			for (i = 0; i < 8; i++)
//				WriteDebugLog(260, "afsk: rxv[%1u]: %6d\n", i,
//					demfilter(s, samples, phase + ((i * phinc) >> 3)) >> 16);
//		}
		phase += phinc;
		newv = demfilter(s, samples, phase);
		s->rxbits >>= 1;
		if (!((newv > 0) ^ ((oldv > 0) | s->notdiff)))
			s->rxbits |= 0x80;
		else
		{
			midv = demfilter(s, samples, phase - halfphinc);
			if ((oldv > 0) ^ (midv > 0))
			{
				thv  = demfilter(s, samples, phase - halfphinc - dcdinterv);
				phase -= phasecorr;
			}
			else
			{
				thv  = demfilter(s, samples, phase - halfphinc + dcdinterv);
				phase += phasecorr;
			}
			if ((midv > 0) ^ (thv > 0))
				s->dcdcnt++;
			else
				s->dcdcnt -= 2;
		}
//		WriteDebugLog(258, "afsk: rx: oldv %10d midv %10d newv %10d\n", oldv, midv, newv);
		oldv = newv;
		s->dcdtim++;
	}
	s->rxphase = phase;
}

// This is the rx thread 
static void demoddemodulate(void *state)
{
	struct demodstate *s = (struct demodstate *)state;
        int dcd0 = -2, dcd1 = -2, dcd2 = -2;
        unsigned char buf;
        char buf2[9];
        unsigned int i;

        s->dcdcnt = -2;
        s->dcdtim = 0;
        s->rxphase = audiocurtime(s->chan) << 16;
        for (;;) {
		demod8bits(s, NULL);
                buf = s->rxbits;
                pktput(s->chan, &buf, 1);
                if (logcheck(256)) {
                        for (i = 0; i < 8; i++)
                                buf2[i] = '0' + ((buf >> i) & 1);
                        buf2[8] = 0;
                        WriteDebugLog(256, "afskrx: %s\n", buf2);
                }
#if 0
                for (i = 0; i < 8; i++)
                        putchar('0' + ((buf >> i) & 1));
                fflush(stdout);
#endif
                if (s->dcdtim < 120)
                        continue;
                dcd0 = dcd1;
                dcd1 = dcd2;
                dcd2 = s->dcdcnt;
                s->dcdcnt = -2;
                s->dcdtim = 0;
                pktsetdcd(s->chan, (dcd0 + dcd1 + dcd2) > 0);
        }
}

static float sinc(float x)
{
        float arg = x * M_PI;

        if (arg == 0)
                return 1;
        return sinf(arg) / arg;
}

float hamming(float x)
{
        return 0.54f-0.46*cosf((2*M_PI)*x);
}

static void demodinit(void *state, unsigned int samplerate, unsigned int *bitrate)
{
	struct demodstate *s = (struct demodstate *)state;
        float f0r[RXFILTLEN*RXFILTOVER];
        float f0i[RXFILTLEN*RXFILTOVER];
        float f1r[RXFILTLEN*RXFILTOVER];
        float f1i[RXFILTLEN*RXFILTOVER];
        float ph0, ph1, w, tscale;
        float max, max0, max1, max2, max3;
        unsigned int i, j;

	tscale = (float)s->bps / (float)samplerate / RXFILTOVER / WINDOWEXPAND;
        ph0 = 2.0f * M_PI * (float)s->f0 / (float)samplerate / RXFILTOVER;
        ph1 = 2.0f * M_PI * (float)s->f1 / (float)samplerate / RXFILTOVER;
        for (i = 0; i < RXFILTLEN * RXFILTOVER; i++) {
                w = i * tscale;
                if (w > 1)
                        w = 0;
                else
                        w = hamming(w);
                f0r[i] = w * cosf(ph0 * i);
                f0i[i] = w * sinf(ph0 * i);
                f1r[i] = w * cosf(ph1 * i);
                f1i[i] = w * sinf(ph1 * i);
        }
        /* determine maximum */
        max = 0;
        for (i = 0; i < RXFILTOVER; i++) {
                max0 = max1 = max2 = max3 = 0;
                for (j = i; j < RXFILTLEN * RXFILTOVER; j += RXFILTOVER) {
                        max0 += fabsf(f0r[j]);
                        max1 += fabsf(f0i[j]);
                        max2 += fabsf(f1r[j]);
                        max3 += fabsf(f1i[j]);
                }
                if (max0 > max)
                        max = max0;
                if (max1 > max)
                        max = max1;
                if (max2 > max)
                        max = max2;
                if (max3 > max)
                        max = max3;
        }
        w = 32767 / max;
        for (i = 0; i < RXFILTLEN * RXFILTOVER; i++) {
                f0r[i] *= w;
                f0i[i] *= w;
                f1r[i] *= w;
                f1i[i] *= w;
        }
	for (i = 0; i < RXFILTOVER; i++)
		for (j = 0; j < RXFILTLEN; j++) {
			s->f.f32.f0r[RXFILTOVER-1-i][j] = f0r[j*RXFILTOVER+i];
			s->f.f32.f0i[RXFILTOVER-1-i][j] = f0i[j*RXFILTOVER+i];
			s->f.f32.f1r[RXFILTOVER-1-i][j] = f1r[j*RXFILTOVER+i];
			s->f.f32.f1i[RXFILTOVER-1-i][j] = f1i[j*RXFILTOVER+i];
		}
        s->srate = samplerate;
        s->rxphinc = ((samplerate << 16) + s->bps / 2) / s->bps;
	*bitrate = s->bps;
}

struct demodulator afskdemodulator = {
	NULL,
	"afsk",
	demodparams,
	demodconfig,
	demodinit,
	demoddemodulate,
	free
};

/* --------------------------------------------------------------------- */

// AFSK Receive code

struct demodstate *s;
int dcd0 = -2, dcd1 = -2, dcd2 = -2;
unsigned char buf = 0;
char buf2[9];
unsigned int i;

void DemodAFSKinit(void *state)
{
	s = (struct demodstate *)state;
	s->dcdcnt = -2;
	s->dcdtim = 0;
	s->rxphase = audiocurtime(s->chan) << 16;
}

// we call demod8bits fo so long as we have enogh samples

short SavedSamples[600];	// AFSK only needs 136 but shared with PSK
int SavedSamplesCount = 0;

short Samples[1200 + 600];		// Change if we change receivesize

void DemodAFSK(short * buffer, int count)
{
	// Called when ADC has accumulated a block of samples

	// demod needs to align the sample pointer to get phase right.
	// It looks like demod needs 136 samples (at 12K sample rate)

	int needed = (12 * s->rxphinc >> 16) + RXFILTLEN; // varie with baud and sample rate
	int phase = s->rxphase;		// save so we know how many samples we consumed
	int used = 0;

//	WriteDebugLog(7, "Needed %d", needed);

	// We save unused samples and prepend to new ones

	memcpy(Samples, SavedSamples, SavedSamplesCount * 2);	// left over from last time
	memcpy(&Samples[SavedSamplesCount], buffer, count * 2); // new ones

	count += SavedSamplesCount;
	
	while ((count - used) > needed)
	{
		demod8bits(s, Samples);
		buf = s->rxbits;
		pktput(s->chan, &buf, 1);

		// demod pointer is in  s->rxphase

		used = (s->rxphase - phase)  >> 16;
	}

	// save count - used samples

	SavedSamplesCount =  count - used;
	memcpy(SavedSamples, &Samples[used], SavedSamplesCount * 2);
	s->rxphase &= 0xffff;	// remove the whole samples
	
//	WriteDebugLog(7, "used %d left %d phase %x",
//		used, SavedSamplesCount, s->rxphase); 

	if (s->dcdtim < 120)
		return;

	dcd0 = dcd1;
	dcd1 = dcd2;
	dcd2 = s->dcdcnt;
	s->dcdcnt = -2;	
	s->dcdtim = 0;
	pktsetdcd(s->chan, (dcd0 + dcd1 + dcd2) > 0);
}

