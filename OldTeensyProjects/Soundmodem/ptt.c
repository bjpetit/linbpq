/*****************************************************************************/

/*
 *      ptt.c  --  PTT signalling.
 *
 *      Copyright (C) 1999-2000, 2002, 2014
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
 * Support for CM108 GPIO control of PTT by Andrew Errington ZL3AME May 2011
 * CM108/Hidraw detection by Thomas Sailer
 *
 */

/*****************************************************************************/

#define _GNU_SOURCE
#define _REENTRANT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "modem.h"
#include "pttio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <stdio.h> 


#ifdef HAVE_SYS_IOCCOM_H
#include <sys/ioccom.h>
#endif

#ifdef HAVE_LINUX_PPDEV_H
#include <linux/ppdev.h>
#elif defined(__FreeBSD__)
#include <dev/ppbus/ppi.h>
#include <dev/ppbus/ppbconf.h>
#else
#include "ppdev.h"
#endif


/* ---------------------------------------------------------------------- */
struct modemparams pttparams[];

/* ---------------------------------------------------------------------- */

int pttinit(struct pttio *state, const char *params[])
{
    return 0;
}

void pttsetptt(struct pttio *state, int pttx)
{
	unsigned char reg;

	if (!state)
		return;

	state->ptt = !!pttx;
	return;

}

void pttsetdcd(struct pttio *state, int dcd)
{

}

void pttrelease(struct pttio *state)
{
	if (!state)
		return;
}
