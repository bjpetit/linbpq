/*****************************************************************************/

/*
 *      rctime.c  --  Raised Cosine Function (Time Domain).
 *
 *      Copyright (C) 2001, 2002
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
 *
 */

/*****************************************************************************/

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>

#undef M_PI
#define M_PI 3.1415926f

#include "raisedcosine.h"

/* --------------------------------------------------------------------- */

/*
 * sinc(pi t) cos(pi alpha t) / [ 1 - (2 alpha t)^2 ]
 */

float raised_cosine_time(float time, float alpha)
{
        float f1, f2, alphatime;
	
	if (fabs(alpha) < 1e-8)
		return sinc(time);
	alphatime = time * alpha;
	f1 = 1 - 4 * alphatime * alphatime;
	if (fabs(f1) < 1e-10)
		f2 = M_PI / 4.0f;
	else
		f2 = cosf(M_PI * alphatime) / f1;
	return sinc(time) * f2;
}
