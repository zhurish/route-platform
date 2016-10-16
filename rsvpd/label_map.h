/* label_map.h :- The definitions of a generic MPLS Label Map Entry.
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
#ifndef _ZMPLS_LABEL_MAP_H
#define _ZMPLS_LABEL_MAP_H

/*-------------------------------------------------------
 * Label Descriptor for ATM LSR.
 *-----------------------------------------------------*/
struct label_atm
{
   u_int16_t vpi;
   u_int16_t vci;
}__attribute__((aligned(8)));

/*-------------------------------------------------------
 * Label Descriptor for FR LSR.
 *-----------------------------------------------------*/
struct label_fr
{
   u_char    len;
   u_int32_t dlci;   	
}__attribute__((aligned(8)));

/*-------------------------------------------------------
 * The Label Descriptor for all Label Types. This is 
 * maintained internally.
 *-----------------------------------------------------*/
struct label
{
   u_char type;
#define LABEL_TYPE_GEN	0
#define LABEL_TYPE_ATM	1
#define LABEL_TYPE_FR	2
   union
   {
      u_int32_t         gen;
      struct label_atm  atm;
      struct label_fr   fr;
   } u;
#define label_gen       u.gen
#define label_atm       u.atm
#define label_fr        u.fr
};
/*-------------------------------------------------------
 * Generic Label Map Entry. The entry can be local label
 * binding or remote label binding for any type of fec
 *------------------------------------------------------*/
struct label_map
{
   /*----------------------------------------------------------------
    * AVL tree of peer_handles with which the label_map is associated
    * with. It can be either remote binding received from peer or 
    * local binding advertised to peer. Generally a label map is 
    * always received from one peer. In that case the avl will have 
    * only one peer. Local binding may be sent out to one or more 
    * peers-then there would be ordered list of peers. We can have 
    * more than 100 peers. So AVL tree is necessary for fast llokups. 
    *--------------------------------------------------------------*/
   struct avl_table  *peer_handle_avl;
   /*----------------------------------------------------------------
    * The pointer to the fec to which the label_map is associated  
    *--------------------------------------------------------------*/
   void      	     *fec_handle;
   /*----------------------------------------------------------------
    * Flags identify characteristics of this entry
    *--------------------------------------------------------------*/
   u_char    	     flags;
#define LABEL_MAP_FLAG_RELEASE_PENDING   0x01
   /*---------------------------------------------------------------
    * The local or remote Label for the fec_handle. It is kept 
    * generic so that the label value could be a Generic Label, ATM 
    * Label or FR Label. It acts as the label key for the label_map. 
    * The label_info should be allocated by the specific protocol 
    * based on use case.
    *--------------------------------------------------------------*/
   u_char	     label_info[0];
}__attribute__((aligned(8)));

/*-------------------------------------------------------------------
 * The following marocs provide parsing facility of each peer in 
 * the label map.
 *-----------------------------------------------------------------*/
#define FOR_ALL_PEER_IN_LABEL_MAP_START(label_map, peer_handle) {\
        struct avl_traverser avl_t;\
        struct avl_table *avl = label_map->peer_handle_avl;\
        if (avl->avl_count) {\
           avl_t_init (&avl_t, avl);\
           \
           while ((peer_handle = avl_t_next (&avl_t))) {\

#define FOR_ALL_PEER_IN_LABEL_MAP_END() }\
                                   }\
                                }
/*-------------------------------------------------------------------
 * Prototypes exported by this module.
 *-----------------------------------------------------------------*/
struct label_map *
label_map_new (u_int16_t label_info_size);

void
label_map_free (struct label_map *);

void
label_map_add_peer (struct label_map *,
                    void             *);

void
label_map_delete_peer (struct label_map *,
                       void             *);

void *
label_map_get_first_peer (struct label_map *);

unsigned int
label_map_get_peer_count (struct label_map *);

int
label_map_has_peer (struct label_map *,
                    void             *);
#endif
