/*****************************************************************************/

/*
 *      rrctime.c  --  Square Root Raised Cosine Function (Time Domain).
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
 * see Huber: Trelliscodierung, S. 15, or ptolemy source code
 *
 * [ (4 alpha t) cos( pi (1+alpha) t) + sin (pi (1 - alpha)t) ] / [ (pi t) (1 - (4 alpha t)^2) ]
 */

float root_raised_cosine_time(float time, float alpha)
{
        float opap, omap, at4, omat4sq;

	if (fabs(alpha) < 1e-8)
		return sinc(time);
	if (fabs(time) < 1e-8)
		return ((4.0f/ M_PI) - 1.0f) * alpha + 1.0f;
	opap = (1.0f + alpha) * M_PI;
	omap = (1.0f - alpha) * M_PI;
	at4 = alpha * time * 4;
	omat4sq = 1 - at4 * at4;
	if (fabs(omat4sq) < 1e-8)
		return ((4.0f * alpha) * cosf(opap * time) - at4 * opap * sinf(opap * time) + omap * cosf(omap * time)) / (1 - 3 * at4 * at4) * (1.0f / M_PI);
	return (4.0f / M_PI) * alpha / omat4sq * (cosf(opap * time) + sinf(omap * time) / at4);
}
