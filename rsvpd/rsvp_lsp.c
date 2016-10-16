/* RSVP LSP related procedures and routines.
**
** Copyright (C) 2003 Pranjal Kumar Dutta <prdutta@uers.sourceforge.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <zebra.h>

#include "memory.h"

#include "rsvpd/rsvp_lsp.h"


/*----------------------------------------------------------------------------
 * Function : rsvp_lsp_create 
 * Input    : None
 * Output   : Returns the pointer to struct rsvp_lsp if created successfully,
 *            else returns NULL.
 * Synopsis : Utility function to create a rsvp_lsp.
 * Callers  " (TBD..)
 *--------------------------------------------------------------------------*/
struct rsvp_lsp *
rsvp_lsp_create (void)
{
   struct rsvp_lsp *rsvp_lsp;
  
   rsvp_lsp = (struct rsvp_lsp *)XCALLOC (MTYPE_RSVP_LSP, 
                                          sizeof (struct rsvp_lsp));
   return rsvp_lsp;
}

void
rsvp_lsp_init (void)
{
   return;
}

void rsvp_lsp_start (struct rsvp_lsp *rsvp_lsp)
{
   return;
}

void rsvp_lsp_stop (struct rsvp_lsp *rsvp_lsp)
{
   return;
}

void rsvp_lsp_reoptimize (struct rsvp_lsp *rsvp_lsp)
{
   return;
}
