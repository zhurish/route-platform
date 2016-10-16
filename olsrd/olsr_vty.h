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



#ifndef _ZEBRA_OLSR_VTY_H
#define _ZEBRA_OLSR_VTY_H


/* Macros. */
#ifndef VTY_GET_IPV4_PREFIX
#define VTY_GET_IPV4_PREFIX(NAME,V,STR)                                       \
{                                                                             \
  int retv;                                                                   \
  retv = str2prefix_ipv4 ((STR), &(V));                                       \
  if (retv <= 0)                                                              \
    {                                                                         \
      vty_out (vty, "%% Invalid %s value%s", NAME, VTY_NEWLINE);              \
      return CMD_WARNING;                                                     \
    }                                                                         \
}
#endif

/* Prototypes */
int olsr_write_interface(struct vty *vty);
int olsr_router_config_write (struct vty *vty);

void olsr_vty_init ();

#endif /* _ZEBRA_OLSR_VTY_H */
