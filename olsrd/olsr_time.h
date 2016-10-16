/*
 * OLSR Rout(e)ing protocol
 *
 * Copyright (C) 2005        Tudor Golubenco
 *                           Polytechnics University of Bucharest 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public Licenseas published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 * more details.

 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef _ZEBRA_OLSR_TIME_H
#define _ZEBRA_OLSR_TIME_H

/* Macros. */
#define FLOAT_2_MSECS(a) (unsigned int)((a) * 1000)

#define TV_2_MSECS(a) ((a).tv_sec * 1000 + (a).tv_usec / 1000)

/* Why aren't this in thread.h? */
#define THREAD_SANDS(X) ((X)->u.sands)
#define THREAD_TIMER_MSEC_ON(master,thread,func,arg,time)       \
  do {                                                          \
    if (! thread)                                               \
      thread = thread_add_timer_msec (master, func, arg, time); \
  } while (0)

#define MAXJITTER 500
#define JITTER(msecs) ((msecs) - rand() % MAXJITTER)

#define OLSR_TIMER_ON(thread, func, arg, time)                 \
  THREAD_TIMER_MSEC_ON (olm->master, thread, func, arg,        \
			JITTER (FLOAT_2_MSECS (time)))

/* Prototypes. */
struct timeval float2tv (float a);
struct timeval
tv_add (struct timeval a, struct timeval b);
struct timeval
tv_sub (struct timeval a, struct timeval b);
int
tv_cmp (struct timeval a, struct timeval b);
struct timeval 
tv_max (struct timeval a, struct timeval b);




#endif /* _ZEBRA_OLSR_TIME_H */
