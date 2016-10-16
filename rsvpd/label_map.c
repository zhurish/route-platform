/* label_map.c :- THe library of MPLS Label Mapping.
**
** This file is part of GNU zMPLS (http://zmpls.sourceforge.net).
**
** Copyright (C) 2002-2006 Pranjal Kumar Dutta <prdutta@users.sourceforge.net>
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
#include "linklist.h"
#include "avl.h"
#include "vty.h"
#include "label_map.h"

/*--------------------------------------------------------------------
 * Function : peer_handle_avl_cmp_fn 
 * Input    : key   = The key object.
 *          : data  = The data object.
 *          : param = Always NULL.
 * Outout   : Returns 1 of key>data, 0 if key == data , else returns 
 *          : -1.
 * Synopsis : This function is required for the maintenance of peer
 *          : handle avl in a label_map. The assumption is that both 
 *          : key and data wuld be pointers to storage that carries 
 *          : a peer_handle.
 * Callers  : Used by the AVL library as callback.
 *------------------------------------------------------------------*/
static int
peer_handle_avl_cmp_fn (const void *key,
                        const void *data,
                        void       *param)
{
   return memcmp (key, data, sizeof (unsigned int));
}

/*--------------------------------------------------------------------
 * Function : label_map_new
 * Input    : label_info_size = The size of label_info in label_map.
 * Output   : Returns pointer to struct label_map
 * Synopsis : This module allocates a label_map_entry in dunamic 
 *          : memory (heap) and returns to caller.
 * Callers  : TBD.
 *------------------------------------------------------------------*/
struct label_map *
label_map_new (u_int16_t label_info_size)
{
   struct label_map *label_map = NULL;

   if (!(label_map = XCALLOC (MTYPE_LABEL_MAP, 
                       sizeof (struct label_map) + label_info_size)))
      return NULL;
   /*------------------------------------------------
    * Allocate an AVL tree of peer_handles.
    *----------------------------------------------*/
   if (!(label_map->peer_handle_avl = 
	avl_create (peer_handle_avl_cmp_fn, NULL, 
		    &avl_allocator_default)))
   {
      label_map_free (label_map);
      return NULL;
   }
   return label_map; 
}

/*-----------------------------------------------------------------------
 * Function : label_map_free
 * Input    : label_map = Pointer to struct label_map.
 * output   : None
 * Synopsis : Frees and returns a dynamic memory block of type struct 
 *            label_map to heap.
 * Callers  : TBD.
 *---------------------------------------------------------------------*/
void
label_map_free (struct label_map *label_map)
{
   /*--------------------------------------
    * Free the list of peer_handles.
    *------------------------------------*/
   avl_destroy (label_map->peer_handle_avl, NULL);
   XFREE (MTYPE_LABEL_MAP, label_map);
   return;
}

/*-------------------------------------------------------------------
 * The object oriented functions.
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : label_map_add_peer
 * Input    : label_map   = Pointer to struct label_map.
 *          : peer_handle = Pointer to the peer handle.
 * Output   : None.
 * Synopsis : This function associates a peer with a label map. The 
 *          : peer is added to the list of peer handles in the label 
 *          : map. Before adding make sure that the peer handle 
 *          : doesn't exist earlier in the label_map.
 * Callers  : TBD.
 *----------------------------------------------------------------*/ 
void
label_map_add_peer (struct label_map *label_map,
		    void             *peer_handle)
{
   avl_insert (label_map->peer_handle_avl, peer_handle);
   return;
}

/*-------------------------------------------------------------------
 * Function : label_map_delete_peer.
 * Input    : label_map   = Pointer to struct label_map.
 *          : peer_handle = The handle of peer to be deleted.
 * Output   : This function deletes the peer_handle from a label_map.
 *          : Before calling this function make sure that the peer
 *          : exists in the label_map.
 *-----------------------------------------------------------------*/ 
void
label_map_delete_peer (struct label_map *label_map,
                       void             *peer_handle)
{
   avl_delete (label_map->peer_handle_avl, peer_handle);
   return;
}

/*-----------------------------------------------------------------
 * Function : label_map_has_peer
 * Input    : label_map   = Pointer to struct label_map.
 *          : peer_handle = The handle of the peer.
 * Output   : Returns 1 if peer_handle is found with label_map, else
 *          : returns 0.
 * Synopsis : This function looks up the peer_handle in the label_map
 *          : and returns boolean.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
int
label_map_has_peer (struct label_map *label_map,
                    void             *peer_handle)
{
   if (avl_find (label_map->peer_handle_avl, peer_handle))
      return 1;
   return 0;
}

/*-------------------------------------------------------------------
 * Function : label_map_get_first_peer
 * Input    : label_map = Pointer to struct label_map.
 * Output   : Returns the first the peer_handle from the AVL tree 
 *          : of label_map->peer_handle_avl.
 * Synopsis : -DO-
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
void *
label_map_get_first_peer (struct label_map *label_map)
{
   struct avl_table *avl = label_map->peer_handle_avl;
   return avl->avl_root->avl_data;
}

/*------------------------------------------------------------------
 * Function : label_map_get_peer_count
 * Input    : label_map = Pointer to struct label_map.
 * Output   : Returns number of peer_handles the label_map is 
 *          : associated with.
 * Synopsis : -DO-
 * Callers  : TBD.
 *-------------`:w---------------------------------------------------*/
unsigned int
label_map_get_peer_count (struct label_map *label_map)
{
   return avl_count (label_map->peer_handle_avl);
}   

/*-----------------------------------------------------------------
 * Function : label_map_debug_vty
 * Input    : vty       = Pointer to struct vty.
 *          : label_map = Pointer to struct label_map.
 * Output   : None.
 * Synopsis : This function dumps a label_map structure to vty.
 * Callers  : From cli command "debug mpls ldp label-map handle xxx"
 *----------------------------------------------------------------*/
void
label_map_debug_vty (struct vty       *vty,
                     struct label_map *label_map)
{
   void *peer_handle = NULL;
   vty_out (vty, "%slabel-map handle : %x%s", VTY_NEWLINE, 
	    (unsigned int)label_map, VTY_NEWLINE);
   vty_out (vty, "------------------------%s", VTY_NEWLINE);
   vty_out (vty, "fec-handle : %x%s", 
	    (unsigned int)label_map->fec_handle, 
	    VTY_NEWLINE);
   if (CHECK_FLAG (label_map->flags, LABEL_MAP_FLAG_RELEASE_PENDING))
      vty_out (vty, "flag : Release Pending%s", VTY_NEWLINE);
   else
      vty_out (vty, "flag : None%s", VTY_NEWLINE);
#if 0
   vty_out (vty, "label : %d%s", label_map->label, VTY_NEWLINE);   
#endif
   /*--------------------------------------------------------------
    * Dump the peer_handles with which the label_map is associated
    *-------------------------------------------------------------*/
   vty_out (vty, "peer_handles:%s", VTY_NEWLINE);
  
   FOR_ALL_PEER_IN_LABEL_MAP_START (label_map, peer_handle)
   {
      vty_out (vty, "   %x%s", (unsigned int) peer_handle,
 	       VTY_NEWLINE); 
   } FOR_ALL_PEER_IN_LABEL_MAP_END (); 
   return;
}
