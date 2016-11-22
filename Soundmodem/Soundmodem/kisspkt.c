/*****************************************************************************/

/*
 *      kisspkt.c  --  (M)KISS & HDLC packet IO.
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

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include "soundio.h"

#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* glibc2.0 does not have sockaddr_ax25, ifr.ifr_newname and SIOCSIFNAME */
#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>
#endif
#ifdef HAVE_LINUX_AX25_H
#include <linux/ax25.h>
#endif
#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#ifdef HAVE_MKISS
#include <arpa/inet.h>
#endif

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef HAVE_PTY_H
#include <pty.h>
#else
extern int openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct  winsize *winp);
#endif

int SerialSendData(unsigned char * Message,int MsgLen);
int KeyPTT(int blnPTT);
void SoundFlush();

/* ---------------------------------------------------------------------- */

#define KISS_FEND   ((unsigned char)0300)
#define KISS_FESC   ((unsigned char)0333)
#define KISS_TFEND  ((unsigned char)0334)
#define KISS_TFESC  ((unsigned char)0335)

#define KISS_CMD_DATA       0
#define KISS_CMD_TXDELAY    1
#define KISS_CMD_PPERSIST   2
#define KISS_CMD_SLOTTIME   3
#define KISS_CMD_TXTAIL     4
#define KISS_CMD_FULLDUP    5
#define KISS_CMD_HARDWARE   6
#define KISS_CMD_FECLEVEL   8
#define KISS_CMD_ACKMODE	12
#define KISS_CMD_RETURN     255

/* ---------------------------------------------------------------------- */
/*
 * the CRC routines are stolen from WAMPES
 * by Dieter Deyke
 */

const u_int16_t crc_ccitt_table[0x100] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* ---------------------------------------------------------------------- */

static u_int16_t calc_crc_ccitt(const u_int8_t *buffer, int len)
{
        u_int16_t crc = 0xffff;

        for (;len>0;len--)
                crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ *buffer++) & 0xff];
        crc ^= 0xffff;
	return crc;
}

static void append_crc_ccitt(u_int8_t *buffer, int len)
{
        u_int16_t crc = calc_crc_ccitt(buffer, len);
        buffer[len] = crc;
        buffer[len+1] = crc >> 8;
}

static int check_crc_ccitt(const u_int8_t *buffer, int len)
{
        u_int16_t crc = calc_crc_ccitt(buffer, len);
        return (crc & 0xffff) == 0x0f47;
}

/* ---------------------------------------------------------------------- */

/*
 * high performance HDLC encoder
 * yes, it's ugly, but generates pretty good code
 */

#define ENCODEITERA(j)                         \
do {                                           \
        if (!(notbitstream & (0x1f0 << j)))    \
                goto stuff##j;                 \
  encodeend##j:;                               \
} while (0)

#define ENCODEITERB(j)                                          \
do {                                                            \
  stuff##j:                                                     \
        bitstream &= ~(0x100 << j);                             \
        bitbuf = (bitbuf & (((2 << j) << numbit) - 1)) |        \
                ((bitbuf & ~(((2 << j) << numbit) - 1)) << 1);  \
        numbit++;                                               \
        notbitstream = ~bitstream;                              \
        goto encodeend##j;                                      \
} while (0)

static void hdlc_encode(struct modemchannel *chan, unsigned char *pkt, unsigned int len, BOOL ackmode)
{
	unsigned bitstream, notbitstream, bitbuf, numbit;
	unsigned wr = chan->pkt.htx.wr;
	int lenptr;

	chan->pkt.stat.pkt_out++;

	if (ackmode)
		append_crc_ccitt(pkt + 2, len - 2);
	else
		append_crc_ccitt(pkt, len);

	len += 2;
	bitstream = 0;
	bitbuf = 0x7e;
	numbit = 8; /* opening flag */


	// I think I'll just buffer the frame as is, with length
	// and ACKMODE flag on front. I'll do bit stuffing in
	// tx loop

	chan->pkt.htx.buf[wr++] = ackmode;
	wr %= TXBUFFER_SIZE;

	// we can't put length in till end, but savepointer
	// and reserve space for it

	lenptr = wr;

	wr += 2;

	if (wr > TXBUFFER_SIZE)
		wr = wr % TXBUFFER_SIZE;

	if (ackmode)
	{
		// first two bytes are send unchanged
	
		chan->pkt.htx.buf[wr++] = *(pkt++);
		wr %= TXBUFFER_SIZE;

		chan->pkt.htx.buf[wr++] = *(pkt++);
		wr %= TXBUFFER_SIZE;

		len -= 2;
	}

	while (numbit >= 8) {
		chan->pkt.htx.buf[wr] = bitbuf;
		wr = (wr + 1) % TXBUFFER_SIZE;
		if (wr == chan->pkt.htx.rd)
			*(int *)0 = 0;  /* must not happen! */
		bitbuf >>= 8;
		numbit -= 8;
	}
 	for (; len > 0; len--, pkt++) {
		bitstream >>= 8;
		bitstream |= ((unsigned int)*pkt) << 8;
		bitbuf |= ((unsigned int)*pkt) << numbit;
		notbitstream = ~bitstream;
		ENCODEITERA(0);
		ENCODEITERA(1);
		ENCODEITERA(2);
		ENCODEITERA(3);
		ENCODEITERA(4);
		ENCODEITERA(5);
		ENCODEITERA(6);
		ENCODEITERA(7);
		goto enditer;
		ENCODEITERB(0);
		ENCODEITERB(1);
		ENCODEITERB(2);
		ENCODEITERB(3);
		ENCODEITERB(4);
		ENCODEITERB(5);
		ENCODEITERB(6);
		ENCODEITERB(7);
	enditer:
		numbit += 8;
		while (numbit >= 8) {
			chan->pkt.htx.buf[wr] = bitbuf;
			wr = (wr + 1) % TXBUFFER_SIZE;
			if (wr == chan->pkt.htx.rd)
				*(int *)0 = 0;  /* must not happen! */
			bitbuf >>= 8;
			numbit -= 8;
		}
	}
	bitbuf |= 0x7e7e << numbit;
	numbit += 16;
	while (numbit >= 8) {
		chan->pkt.htx.buf[wr] = bitbuf;
		wr = (wr + 1) % TXBUFFER_SIZE;
		if (wr == chan->pkt.htx.rd)
			*(int *)0 = 0;  /* must not happen! */
		bitbuf >>= 8;
		numbit -= 8;
	}
	chan->pkt.htx.wr = wr;

	// add length to packet

	len = wr - lenptr - 2;	// -2 for length fiwld
	if (len < 0 || len > 400)
		len %= TXBUFFER_SIZE;
	chan->pkt.htx.buf[lenptr++] = len & 0xff;
	lenptr %= TXBUFFER_SIZE;
	chan->pkt.htx.buf[lenptr] = len >> 8;
}

/* ---------------------------------------------------------------------- */

static void kiss_encodepkt(struct modemchannel *chan, unsigned char *data, unsigned dlen)
{
	unsigned char kbuf[768];
	unsigned char *bp = kbuf;
	int i, len;

	*bp++ = KISS_FEND;
	*bp++ = KISS_CMD_DATA;
	for (; dlen > 0; dlen--, data++) {
		if (*data == KISS_FEND) {
			*bp++ = KISS_FESC;
			*bp++ = KISS_TFEND;
		} else if (*data == KISS_FESC) {
			*bp++ = KISS_FESC;
			*bp++ = KISS_TFESC;
		} else 
			*bp++ = *data;
	}
	*bp++ = KISS_FEND;
	len = bp - kbuf;

	i = SerialSendData(kbuf, len);

	chan->pkt.stat.kiss_out++;
}

static void do_rxpacket(struct modemchannel *chan)
{
	if (chan->pkt.hrx.bufcnt < 15) 
		return;
	
	if (!check_crc_ccitt(chan->pkt.hrx.buf, chan->pkt.hrx.bufcnt))
	{
//		WriteDebugLog(7,"RX Packet Len = %d Bad CRC", chan->pkt.hrx.bufcnt);
		return;
	}
		
	WriteDebugLog(7,"RX Packet Len = %d Good CRC", chan->pkt.hrx.bufcnt);
	chan->pkt.stat.pkt_in++;
	kiss_encodepkt(chan, chan->pkt.hrx.buf, chan->pkt.hrx.bufcnt-2);
}

#define DECODEITERA(j)                                                        \
do {                                                                          \
        if (!(notbitstream & (0x0fc << j)))              /* flag or abort */  \
                goto flgabrt##j;                                              \
        if ((bitstream & (0x1f8 << j)) == (0xf8 << j))   /* stuffed bit */    \
                goto stuff##j;                                                \
  enditer##j:;                                                                \
} while (0)

#define DECODEITERB(j)                                                                 \
do {                                                                                   \
  flgabrt##j:                                                                          \
        if (!(notbitstream & (0x1fc << j))) {              /* abort received */        \
                state = 0;                                                             \
                goto enditer##j;                                                       \
        }                                                                              \
        if ((bitstream & (0x1fe << j)) != (0x0fc << j))   /* flag received */          \
                goto enditer##j;                                                       \
        if (state)                                                                     \
                do_rxpacket(chan);                                                     \
        chan->pkt.hrx.bufcnt = 0;                                                      \
        chan->pkt.hrx.bufptr = chan->pkt.hrx.buf;                                      \
        state = 1;                                                                     \
        numbits = 7-j;                                                                 \
        goto enditer##j;                                                               \
  stuff##j:                                                                            \
        numbits--;                                                                     \
        bitbuf = (bitbuf & ((~0xff) << j)) | ((bitbuf & ~((~0xff) << j)) << 1);        \
        goto enditer##j;                                                               \
} while (0)
  

static void hdlc_receive(struct modemchannel *chan, const unsigned char *data, unsigned nrbytes)
{
	unsigned bits, bitbuf, notbitstream, bitstream, numbits, state;

	/* start of HDLC decoder */
	numbits = chan->pkt.hrx.numbits;
	state = chan->pkt.hrx.state;
	bitstream = chan->pkt.hrx.bitstream;
	bitbuf = chan->pkt.hrx.bitbuf;
	while (nrbytes > 0)
	{
		bits = *data++;
		nrbytes--;
		bitstream >>= 8;
		bitstream |= ((unsigned int)bits) << 8;
		bitbuf >>= 8;
		bitbuf |= ((unsigned int)bits) << 8;
		numbits += 8;
		notbitstream = ~bitstream;

//
		DECODEITERA(0);
		DECODEITERA(1);
		DECODEITERA(2);
		DECODEITERA(3);
		DECODEITERA(4);
		DECODEITERA(5);
		DECODEITERA(6);
		DECODEITERA(7);
		goto enddec;
		DECODEITERB(0);
		DECODEITERB(1);
		DECODEITERB(2);
		DECODEITERB(3);
		DECODEITERB(4);
		DECODEITERB(5);
		DECODEITERB(6);
		DECODEITERB(7);
	enddec:

		// ?? if 
		
		while (state && numbits >= 8) {
			if (chan->pkt.hrx.bufcnt >= RXBUFFER_SIZE) {
				state = 0;
			} else {
				*(chan->pkt.hrx.bufptr)++ = bitbuf >> (16-numbits);
				chan->pkt.hrx.bufcnt++;
				numbits -= 8;
			}
		}
	}
        chan->pkt.hrx.numbits = numbits;
	chan->pkt.hrx.state = state;
	chan->pkt.hrx.bitstream = bitstream;
	chan->pkt.hrx.bitbuf = bitbuf;
}

/* ---------------------------------------------------------------------- */

static void kiss_process_pkt(struct modemchannel *chan, u_int8_t *pkt, unsigned int len)
{
	if (len < 2)
		return;

	if (len > 360)
		return;

	chan->pkt.stat.kiss_in++;
	switch (pkt[0])
	{
	case KISS_CMD_DATA:
		hdlc_encode(chan, pkt+1, len-1, 0);
		return;

	case KISS_CMD_ACKMODE:
		hdlc_encode(chan, pkt+1, len-1, 1);
		return;

	case KISS_CMD_TXDELAY:
		state.chacc.txdelay = pkt[1]*10;
		WriteDebugLog(MLOG_INFO, "kiss: txdelay = %ums\n", pkt[1]*10);
		return;
                        
	case KISS_CMD_TXTAIL:
		WriteDebugLog(MLOG_INFO, "kiss: txtail = %ums\n", pkt[1]*10);
		return;

	case KISS_CMD_PPERSIST:
		state.chacc.ppersist = pkt[1];
		WriteDebugLog(MLOG_INFO, "kiss: ppersist = %u/256\n", pkt[1]);
		return;

	case KISS_CMD_SLOTTIME:
		state.chacc.slottime = pkt[1]*10;
		WriteDebugLog(MLOG_INFO, "kiss: slottime = %ums\n", pkt[1]*10);
		return;

	case KISS_CMD_FULLDUP:
		state.chacc.fullduplex = !!pkt[1];
		WriteDebugLog(MLOG_INFO, "kiss: %sduplex\n", pkt[1] ? "full" : "half");
		return;
	
	default:
		WriteDebugLog(MLOG_INFO, "unknown kiss packet: 0x%02x 0x%02x\n", pkt[0], pkt[1]);
		return;
	}
}

/* ---------------------------------------------------------------------- */

static unsigned short random_num(void)
{
	static unsigned short random_seed;

        random_seed = 28629 * random_seed + 157;
        return random_seed;
}

/* ---------------------------------------------------------------------- */

static unsigned int terminate = 0;

static int globaldcd(struct state *state)
{
	struct modemchannel *chan;

	for (chan = state->channels; chan; chan = chan->next)
		if (chan->pkt.dcd)
			return 1;
	return 0;
}

int pktget(struct modemchannel *chan, unsigned char *data, unsigned int len)
{
	unsigned int i, j, n = len;

	i = (chan->pkt.htx.rd - chan->pkt.htx.wr - 1) % TXBUFFER_SIZE;
	if (chan->pkt.inhibittx || chan->pkt.htx.rd == chan->pkt.htx.wr)
		return 0;
	while (n > 0) {
		if (chan->pkt.htx.wr >= chan->pkt.htx.rd)
			j = chan->pkt.htx.wr - chan->pkt.htx.rd;
		else
			j = TXBUFFER_SIZE - chan->pkt.htx.rd;
		if (j > n)
			j = n;
		if (!j)
			break;
		memcpy(data, &chan->pkt.htx.buf[chan->pkt.htx.rd], j);
		data += j;
		n -= j;
		chan->pkt.htx.rd = (chan->pkt.htx.rd + j) % TXBUFFER_SIZE;
	}
	if (n > 0)
		memset(data, 0, n);
	return (len - n);
}

void pktput(struct modemchannel *chan, const unsigned char *data, unsigned int len)
{
	hdlc_receive(chan, data, len);
}

#define NRPFD 8


void pkttransmitloop(struct state *state)
{
	struct modemchannel *chan = state->channels;

	BOOL AckMode;
	unsigned char AckVal1;	// ackmode value to return
	unsigned char AckVal2;	// ackmode value to return
	int len, savewr, newwr;
	int rd;

	// TO support ackmode I've added an ackmode flag and two 
	// byte length field on the front of each packet.

	// I think we should discard anything that can't be
	// sent for more than say 60 secs. ax.25 can't really 
	// survive with longer waits than this, so I'll add 
	// another two bytes for timestamp

	// or do we use a cyclic buffer??? - yes, 4K for now

	if (chan->pkt.htx.rd != chan->pkt.htx.wr)
	{
		if (!state->chacc.fullduplex)
		{
			if (!globaldcd(state) && (random_num() & 0xff) <= state->chacc.ppersist)
			{
				// Lost CSMA try again after slot time

				Sleep(state->chacc.slottime);
				return;
			}
		}

		KeyPTT(1);

		// This sends flags for txd 

		chan->mod->modulate(chan->modstate, state->chacc.txdelay);

		// See if more packets to send
		
		while (chan->mod && chan->pkt.htx.rd != chan->pkt.htx.wr)
		{
			rd = chan->pkt.htx.rd;

			AckMode = chan->pkt.htx.buf[rd++];
			rd = rd % TXBUFFER_SIZE;

			len = chan->pkt.htx.buf[rd++];
			rd = rd % TXBUFFER_SIZE;
			len += chan->pkt.htx.buf[rd++] << 8;
			rd = rd % TXBUFFER_SIZE;

			if (len > 256)
				len = 256;


			if (AckMode)
			{
				// fist two bytes have to be returned when frame sent

				AckVal1 = chan->pkt.htx.buf[rd++];
				rd = rd % TXBUFFER_SIZE;
				AckVal2 = chan->pkt.htx.buf[rd++];
				rd = rd % TXBUFFER_SIZE;

				len -= 2;
			}

			// modulate will send till buffer is empty,
			// but we want to stop at end of each packet
			// to send ackmode response if needed.
			// TO do this change wr temporaily

			savewr = chan->pkt.htx.wr;
			newwr = rd + len;
			if (newwr > TXBUFFER_SIZE)
				newwr -= TXBUFFER_SIZE; 

			chan->pkt.htx.rd = rd;

			if (chan->pkt.htx.wr != newwr)
				chan->pkt.htx.wr = newwr;
			
			chan->pkt.inhibittx = 0;
			chan->mod->modulate(chan->modstate, 0);
			chan->pkt.inhibittx = 1;

			if (AckMode)
			{
				UCHAR kbuf[5];
				UCHAR * kptr = kbuf;

				*(kptr++) = KISS_FEND;
				*(kptr++) = KISS_CMD_ACKMODE;
				*(kptr++) = AckVal1;
				*(kptr++) = AckVal2;
				*(kptr++) = KISS_FEND;

				SerialSendData(kbuf, 5);
			}
			chan->pkt.htx.wr = savewr;
		}
		SoundFlush();
		KeyPTT(0);
	}
}

void pktsetdcd(struct modemchannel *chan, int dcd)
{
	unsigned int newdcd =  !!dcd;
		
	if (chan->pkt.dcd != newdcd)
	{
		WriteDebugLog(250, "DCD: %s", dcd ? "on" : "off");
		displayDCD(dcd);
	}

	chan->pkt.dcd = newdcd;

}

void pktrelease(struct modemchannel *chan)
{
	close(chan->pkt.kiss.fd);
	if (chan->pkt.kiss.fdmaster != -1)
		close(chan->pkt.kiss.fdmaster);
	chan->pkt.kiss.fd = chan->pkt.kiss.fdmaster = -1;
}

struct modemparams pktkissparams[];

void pktinit(struct modemchannel *chan, const char *params[])
{
    memset(&chan->pkt, 0, sizeof(chan->pkt));
	chan->pkt.inhibittx = 1;
	chan->pkt.kiss.ibufptr = 0;
	chan->pkt.dcd = 0;
}


#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

extern struct state state;

UCHAR kissMsg[512];

UCHAR * kissPtr = kissMsg;

BOOL ESCFLAG = FALSE;

void ProcessKISSMessage(UCHAR *kissMsg, int Length)
{
	WriteDebugLog(7, "sending KISS frame len %d", Length);
	kiss_process_pkt(state.channels, kissMsg, Length);
}


void ProcessKISSPacket(unsigned char * Packet, int Length)
{
	UCHAR c;

	if (Length < 0 || Length >360)
		return;

	while (Length)
	{
		Length--;

		c = *(Packet++);

		if (ESCFLAG)
		{
			//
			//	FESC received - next should be TFESC or TFEND

			ESCFLAG = FALSE;

			if (c == TFESC)
				c=FESC;
	
			if (c == TFEND)
				c=FEND;

		}
		else
		{
			switch (c)
			{
			case FEND:		
	
				//
				//	Either start of message or message complete
				//
				
				if (kissPtr == &kissMsg[0])
					continue;				// Null Frame

				ProcessKISSMessage(kissMsg, kissPtr - &kissMsg[0]);
				kissPtr = &kissMsg[0];

				continue;

			case FESC:
		
				ESCFLAG = TRUE;
				continue;

			}
		}
		
		//
		//	Ok, a normal char
		//

		*(kissPtr++) = c;
	}

	if (kissPtr > &kissMsg[500])		// too long
		kissPtr = &kissMsg[0];
	
 	return;
}
	
