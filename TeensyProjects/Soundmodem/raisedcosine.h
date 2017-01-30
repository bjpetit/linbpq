/*****************************************************************************/

/*
 *      raisedcosine.h  --  Raised Cosine Functions.
 *
 *      Copyright (C) 2002
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _RAISEDCOSINE_H
#define _RAISEDCOSINE_H

#ifdef  __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------- */

extern float sinc(float x);
extern float hamming(float x);
extern float raised_cosine_time(float time, float alpha);
extern float raised_cosine_freq(float freq, float alpha);
extern float root_raised_cosine_time(float time, float alpha);
extern float root_raised_cosine_freq(float freq, float alpha);

/* --------------------------------------------------------------------- */
#ifdef  __cplusplus
}
#endif

#endif /* _RAISEDCOSINE_H */
