/* Procedures and Routines for RSVP Softstate Module
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
#include "vty.h"
#include "prefix.h"
#include "avl.h"
#include "linklist.h"
#include "jhash.h"
#include "hash.h"
#include "if.h"
#include "command.h"
#include "lsp.h"
#include "stream.h"


#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_obj.h"
#include "rsvpd/rsvp_packet.h"
#include "rsvpd/rsvp_state.h"

/*--------------------------------------------------------------------------
 * Function : rsvp_psb_create.
 * Input    : None.
 * Output   : Returns pointer to struct rsvp_psb.
 * Synopsis : This is an internal utility function to create an object of 
 *            type struct rsvp_psb.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
static struct rsvp_psb *
rsvp_psb_create (void)
{
   struct rsvp_psb *rsvp_psb;

   rsvp_psb = XCALLOC (MTYPE_RSVP_PSB, sizeof (struct rsvp_psb));
  
   return rsvp_psb;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_psb_free
 * Input    : Pointer to object of type struct rsvp_psb.
 * Output   : None.
 * Synopsis : This is a utility function to free an object of type 
 *            struct rsvp_psb.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
void
rsvp_psb_free (struct rsvp_psb *rsvp_psb)
{
   /* Sanity Check.*/
   if (!rsvp_psb)
      return;

   XFREE (MTYPE_RSVP_PSB, rsvp_psb);
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_rsb_create 
 * Input    : None.
 * Output   : Returns pointer to struct rsvp_rsb.
 * Synopsis : This is an internal utility function to create an object of 
 *            type struct rsvp_rsb.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static struct rsvp_rsb *
rsvp_rsb_create (void)
{
   struct rsvp_rsb *rsvp_rsb;

   rsvp_rsb = XCALLOC (MTYPE_RSVP_RSB, sizeof (struct rsvp_rsb));
   
   return rsvp_rsb;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_rsb_free
 * Input    : Pointer to object of type struct rsvp_rsb.
 * Output   : None.
 * Synopsis : This is a utility function to free an object of type 
 *            struct rsvp_rsb.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static void
rsvp_rsb_free (struct rsvp_rsb *rsvp_rsb)
{
   /* Sanity Check.*/
   if (!rsvp_rsb)
      return;

   XFREE (MTYPE_RSVP_RSB, rsvp_rsb);
   return;
} 

/*--------------------------------------------------------------------------
 * Function : psb_session_avl_cmp_fn
 * Input    : tun_id_in = Pointer to u_int16_t containing Tunnel ID.
 *          : tun_id_entry = Pointer to u_int16_t containing tunnel id in 
 *                           the AVL tree Entry.
 *            null_param = Always NULL.
 * Output   : Returns the comparsion value.
 * Synopsis : This function is registered while creating the psb_session_avl.
 *            and is used to compare each entry in the avl_table.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
int 
psb_session_avl_cmp_fn (const void *tun_id_in,
                        const void *tun_id_entry,
                        void *null_param)
{
   return (*((u_int16_t *)tun_id_in) - *((u_int16_t *)tun_id_entry));
}

/*---------------------------------------------------------------------------
 * Function : psb_session_avl_create
 * Input    : None.
 * Output   : Pointer to object of type struct avl_table of created.
 * Synopsis : This internal function creates a struct avl_table. This is a 
 *            wrapper around the avl_table library function.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static struct avl_table *
psb_session_avl_create (void)
{  
   struct avl_table *psb_session_avl;

   /* Create and initialize the AVL tree.*/
   psb_session_avl = avl_create (psb_session_avl_cmp_fn,
                                 NULL,
                                 &avl_allocator_default);
   return psb_session_avl;
}

/*----------------------------------------------------------------------------
 * Function : psb_session_avl_delete
 * Input    : psb_session_avl = Pointer to the struct avl_table
 * Output   : None 
 * Synopsis : This function deletes a AVL Table. Make sure that it is called 
 *            only after deleting all nodes, because this only frees up a 
 *            struct avl_table.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
void
psb_session_avl_delete (struct avl_table *psb_session_avl)
{
   /* Sanity Check.*/
   if (!psb_session_avl)
      return;

   avl_destroy (psb_session_avl, NULL); 
   
   return;
}

/*----------------------------------------------------------------------------
 * Function : psb_session_avl_entry_insert.
 * Input    : psb_session_avl = Pointer to the struct avl_table.
 *          : tun_id = Tunnel ID Value of type u_int16_t.
 * Output   : Returns the Pointer to the object of type 
 *            struct session_avl_entry created.
 * Synopsis : This function created a avl_node with tun_id as key and iserts
 *            it into the session_avl.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static struct psb_session_avl_entry *
psb_session_avl_entry_insert (struct avl_table *psb_session_avl,
                              u_int16_t tun_id)
{
   struct psb_session_avl_entry *entry = NULL;
   
   /* Allocate a AVL node. */
   entry = XCALLOC (MTYPE_RSVP_PSB_SESSION_AVL_ENTRY, 
                    sizeof (struct psb_session_avl_entry));
   /* Check malloc failure if any.*/
   if (!entry)
      return NULL;
   /* Copy the Tunnel ID.*/
   entry->tun_id = tun_id;  
   /* Create the rsvp_psb list.*/
   if ((entry->psb_list = list_new ()) == NULL)
   {
      XFREE (MTYPE_RSVP_PSB_SESSION_AVL_ENTRY, entry);
      return NULL;
   }
   /* Insert the Entry into the AVL Tree.*/
   avl_insert (psb_session_avl, entry);
   return entry;
}

/*--------------------------------------------------------------------------
 * Function : psb_session_avl_entry_lookup
 * Input    : psb_session_avl = Pointer to struct avl_table.
 *          : tun_id = Tunnel ID of type u_int16_t.
 * Output   : Returns the Pointer to the object of struct psb_session_avl_entry
 * Synopsis : This function looks up the entry in psb_session_avl 
 *            corresponding to the input tun_id.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
static struct psb_session_avl_entry *
psb_session_avl_entry_lookup (struct avl_table *psb_session_avl,
                              u_int16_t tun_id)
{
   struct psb_session_avl_entry *entry = NULL;

   /* Sanity Test.*/
   if (!psb_session_avl)
      return NULL;
   /* Get the entry from AVL Table.*/ 
   entry = (struct psb_session_avl_entry *) avl_find (psb_session_avl, 
                                                      &tun_id); 
   return entry;
}          

/*--------------------------------------------------------------------------
 * Function : psb_session_avl_entry_delete
 * Input    : psb_session_avl = Pointer to struct avl_table.
 *          : tun_id = The input key as Tunnel IID of type u_int16_t.
 * Output   : Returns 0 if deleted successfully, else returns -1 if the entty
 *            is not found.
 * Synopsis : This function deletes an avl_node corresponding to the input key
 *            tun_id.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static int 
psb_session_avl_entry_delete (struct avl_table *psb_session_avl,
                              u_int16_t tun_id)
{
   struct psb_session_avl_entry *entry = NULL;

   /* Sanity Check.*/
   if (!psb_session_avl)
      return 0;
   /* Delete the entry from AVL Table.*/
   entry = (struct psb_session_avl_entry *) avl_delete (psb_session_avl,
                                                        &tun_id);
   /* If the entry is not found.*/
   if (!entry)
      return -1;
   /* Free the rsvp_psb list. */
   list_free (entry->psb_list);
   
   XFREE (MTYPE_RSVP_PSB_SESSION_AVL_ENTRY, entry);
   return 0;
}  
                      
/*--------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_key_fn
 * Input    : sender_addr = Pointer to object of type struct in_addr.
 * Output   : Returns the hash key .
 * Synopsis : This utility function creates a hash key from the ipv4 sender_addr
 *            as input key. This function is used by hash library.
 * Callers  : (TBD).  
 *------------------------------------------------------------------------*/
static unsigned int
psb_ipv4_sender_hash_key_fn (struct in_addr *sender_addr)
{
   return (hashword (&sender_addr->s_addr, 1, 13) & PSB_SENDER_HASH_MASK);    
}
 
/*-------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_cmp_fn
 * Input    : entry       = Pointer to struct psb_session_hash_entry.
 *          : sender_addr = Pointer to struct in_addr.            
 * Output   : Returns 1 if match is found, else returns 0.
 * Synopsis : This is the hash comparison function that is used to 
 *            search the matched entry in a hashed list.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
int
psb_ipv4_sender_hash_cmp_fn (struct psb_ipv4_sender_hash_entry *entry,
                             struct in_addr *sender_addr)
{
   if (memcmp (&entry->sender_addr, sender_addr, sizeof (struct in_addr)))  
      return 0;
   else
      return 1;
}

/*--------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_entry_alloc 
 * Input    : psb_ipv4_sender_hash = Pointer to struct hash.
 *          : sender_addr = Pointer to struct in_addr of the PSB's sender. 
 * Output   : Returns as void pointer to object of type 
 *            struct psb_sender_hash_entry.
 * Synopsis : Internal utility function is used to create a hash_entry of type
 *            struct psb_ipv4_sender_hash_entry.
 * Callers  : This function is used in hash_create 
 *-------------------------------------------------------------------------*/
static void *
psb_ipv4_sender_hash_entry_alloc (struct in_addr *sender_addr)
{
   struct psb_ipv4_sender_hash_entry *entry = NULL;
   /* Sanity Check.*/
   if (!sender_addr)
      return NULL;

   /* Allocate the entry.*/
   entry = XCALLOC (MTYPE_RSVP_PSB_IPV4_SENDER_HASH_ENTRY, 
                    sizeof (struct psb_ipv4_sender_hash_entry));
   /* Check for malloc failure.*/
   if (!entry)
      return NULL;

   /* Fill the sender_addr in the entry.*/
   memcpy (&entry->sender_addr, sender_addr , sizeof (struct in_addr));
   /* Initialize the AVL Tree to store the sessions for this source.*/ 
   if ((entry->session_avl = psb_session_avl_create ()) == NULL)
   {
      /* If malloc failure then free it.*/
      XFREE (MTYPE_RSVP_PSB_IPV4_SENDER_HASH_ENTRY, entry);
      return NULL;
   }
   return entry;      
};

/*--------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_entry_insert 
 * Input    : psb_ipv4_sender_hash = Pointer to struct hash.
 *          : sender_addr = Pointer to struct in_addr of the PSB's sender. 
 * Output   : Returns 0 if insertion successful, else returns -1.
 * Synopsis : Internal utility function is used to create a hash_entry of type
 *            struct psb_ipv4_sender_hash_entry.
 * Callers  : This function is used in hash_create 
 *-------------------------------------------------------------------------*/
static int
psb_ipv4_sender_hash_entry_insert (struct hash *psb_ipv4_sender_hash,
                                   struct in_addr *sender_addr)
{
   struct psb_ipv4_sender_hash_entry *entry = NULL;
   /* Sanity Check.*/
   if (!sender_addr || !psb_ipv4_sender_hash)
      return -1;
 
   entry = 
   (struct psb_ipv4_sender_hash_entry *)hash_get (psb_ipv4_sender_hash,
                                                  sender_addr,
                                            psb_ipv4_sender_hash_entry_alloc);
   return 0;      
};

/*--------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_entry_lookup
 * Input    : psb_sender_hash = Pointer to object of type struct hash.
 *          : sender_addr     = Pointer to object of type struct in_addr.
 * Output   : Returns the pointer to struct psb_sender_hash_entry if matched
 *            entry found, else returns NULL.
 * Synopsis : This function looks up a hash entry that matches the sender_addr
 *            as input key.
 * Callers  : rsvp_state_psb_lookup in rsvp_state.c
 *-------------------------------------------------------------------------*/
static struct psb_ipv4_sender_hash_entry *
psb_ipv4_sender_hash_entry_lookup ( struct hash *psb_ipv4_sender_hash,
                                    struct in_addr *sender_addr)
{
   struct psb_ipv4_sender_hash_entry *entry = NULL;

   /* Sanity Check on input parameters.*/
   if (!psb_ipv4_sender_hash || !sender_addr)
      return NULL;
   /* Get the hash entry corresponding to the sender_addr.*/
   entry = hash_get (psb_ipv4_sender_hash, sender_addr, NULL);

   return entry;
}

/*---------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_entry_delete 
 * Input    : psb_sender_hash = Pointer to object of type struct hash.
 *          : sender_addr = Pointer to object of type struct in_addr.
 * Output   : None.
 * Synopsis : This function deletes a hash entry that matches the sender_addr
 *            as key.
 *-------------------------------------------------------------------------*/
static void
psb_ipv4_sender_hash_entry_delete (struct hash *psb_ipv4_sender_hash,
                                   struct in_addr *sender_addr)
{
   struct psb_ipv4_sender_hash_entry *entry;
   /* Sanity check for input arguments.*/
   if (!psb_ipv4_sender_hash || !sender_addr)
      return;

   entry = hash_release (psb_ipv4_sender_hash, sender_addr);

   if (entry)
      XFREE (MTYPE_RSVP_PSB_IPV4_SENDER_HASH_ENTRY, entry);
   return;
}

/*---------------------------------------------------------------------------
 * Function : psb_ipv4_sender_hash_create
 * Input    : hash_size = The size of the hash.
 * Output   : Pointer to object of type struct hash.
 * Synopsis : This function creates a hash of psb sender as key.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static struct hash *
psb_ipv4_sender_hash_create (unsigned int hash_size)
{
   struct hash *hash;

   /* Create the Hash.*/
   hash = hash_create_size (PSB_SENDER_HASH_SIZE, 
                            psb_ipv4_sender_hash_key_fn,
                            psb_ipv4_sender_hash_cmp_fn);
   return hash;
}

#ifdef HAVE_IPV6 
/*--------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_key_fn
 * Input    : sender_addr = Pointer to object of type struct in6_addr.
 * Output   : Returns the hash key.
 * Synopsis : This utility function creates a hash key from the IPV6 sender
 *            addr as input key. This function is used by hash library. 
 * Callers  : (TBD.)
 *------------------------------------------------------------------------*/
static unsigned int
psb_ipv6_sender_hash_key_fn (struct in6_addr *sender_addr)
{
   return (hashword (&sender_addr->s6_addr32, 4, 13) & PSB_SENDER_HASH_MASK);   
}

/*--------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_cmp_fn
 * Input    : entry      = Pointer to struct psb_ipv6_sender_hash_entry.
 *          : sender_addr= Pointer to IPV6 address on the sender.
 * Output   : Returns 1 if hash entry matches, else returns 0.
 * Synopsis : This function is the hash comparison function for IPV6 Sender.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
int
psb_ipv6_sender_hash_cmp_fn (struct psb_ipv6_sender_hash_entry *entry,
                             struct in6_addr *sender_addr)
{
   if (memcmp (&entry->sender_addr, sender_addr, sizeof (struct in6_addr)))
      return 0;
   else
      return 1;
}

/*--------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_entry_alloc 
 * Input    : sender_addr = Pointer to struct in6_addr of the PSB's sender. 
 * Output   : Returns as void pointer to object of type 
 *            struct psb_ipv6_sender_hash_entry.
 * Synopsis : Internal utility function is used to create a hash_entry of type
 *            struct psb_ipv4_sender_hash_entry.
 * Callers  : This function is used in hash_create 
 *-------------------------------------------------------------------------*/
static void *
psb_ipv6_sender_hash_entry_alloc (struct in6_addr *sender_addr)
{
   struct psb_ipv6_sender_hash_entry *entry = NULL;
   /* Sanity Check.*/
   if (!sender_addr)
      return NULL;

   /* Allocate the entry.*/
   entry = XCALLOC (MTYPE_RSVP_PSB_IPV6_SENDER_HASH_ENTRY, 
                    sizeof (struct psb_ipv6_sender_hash_entry));
   /* Check for malloc failure.*/
   if (!entry)
      return NULL;

   /* Fill the sender_addr in the entry.*/
   memcpy (&entry->sender_addr, sender_addr , sizeof (struct in6_addr));
   /* Initialize the AVL Tree to store the sessions for this source.*/ 
   if ((entry->session_avl = psb_session_avl_create ()) == NULL)
   {
      /* If malloc failure then free it.*/
      XFREE (MTYPE_RSVP_SENDER_HASH_ENTRY, entry);
      return NULL;
   }
   return entry;      
};

/*--------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_entry_insert 
 * Input    : sender_addr = Pointer to struct in6_addr of the PSB's sender. 
 * Output   : Returns 0 if inserted , else returns -1. 
 * Synopsis : Internal utility function is used to create a hash_entry of type
 *            struct psb_ipv4_sender_hash_entry.
 * Callers  : This function is used in hash_create 
 *-------------------------------------------------------------------------*/
static int
psb_ipv6_sender_hash_entry_insert (struct hash *psb_ipv6_sender_hash,
                                   struct in6_addr *sender_addr)
{
   struct psb_ipv6_sender_hash_entry *entry = NULL;
   /* Sanity Check.*/
   if (!sender_addr || !psb_ipv6_sender_hash)
      return -1;

   entry = (struct psb_ipv6_sender_hash_entry *)hash_get (psb_ipv6_sender_hash,
                                                          sender_addr,
                                             psb_ipv6_sender_hash_entry_alloc);
   return 0;      
};

/*--------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_entry_lookup
 * Input    : psb_sender_hash = Pointer to object of type struct hash.
 *          : sender_addr     = Pointer to object of type struct in6_addr.
 * Output   : Returns the pointer to struct psb_sender_hash_entry if matched
 *            entry found, else returns NULL.
 * Synopsis : This function looks up a hash entry that matches the sender_addr
 *            as input key.
 * Callers  : rsvp_state_psb_lookup in rsvp_state.c
 *-------------------------------------------------------------------------*/
static struct psb_ipv6_sender_hash_entry *
psb_ipv6_sender_hash_entry_lookup ( struct hash *psb_ipv6_sender_hash,
                                    struct in6_addr *sender_addr)
{
   struct psb_ipv6_sender_hash_entry *entry = NULL;

   /* Sanity Check on input parameters.*/
   if (!psb_ipv6_sender_hash || !sender_addr)
      return;
   /* Get the hash entry corresponding to the sender_addr.*/
   entry = hash_get (psb_ipv6_sender_hash, sender_addr, NULL);

   return entry;
}

/*---------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_entry_delete 
 * Input    : psb_sender_hash = Pointer to object of type struct hash.
 *          : sender_addr = Pointer to object of type struct in6_addr.
 * Output   : None.
 * Synopsis : This function deletes a hash entry that matches the sender_addr
 *            as key.
 *-------------------------------------------------------------------------*/
static void
psb_ipv6_sender_hash_entry_delete (struct hash *psb_ipv6_sender_hash,
                                   struct in6_addr *sender_addr)
{
   struct psb_ipv6_sender_hash_entry *entry;
   /* Sanity check for input arguments.*/
   if (!psb_ipv6_sender_hash || !sender_addr)
      return;

   entry = hash_release (psb_ipv6_sender_hash, sender_addr);

   if (!entry)
      return;
   XFREE (MTYPE_RSVP_PSB_IPV6_SENDER_HASH_ENTRY, entry);
   return;
}

/*---------------------------------------------------------------------------
 * Function : psb_ipv6_sender_hash_create
 * Input    : hash_size = The size of the hash.
 * Output   : Pointer to object of type struct hash.
 * Synopsis : This function creates a hash of psb sender as key.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
static struct hash *
psb_ipv6_sender_hash_create (unsigned int hash_size)
{
   struct hash *hash;

   /* Create the Hash.*/
   hash = hash_create_size (PSB_SENDER_HASH_SIZE, 
                            psb_ipv6_sender_hash_key_fn,
                            psb_ipv6_sender_hash_cmp_fn);
   return hash;
}   

#endif /* HAVE_IPV6 */ 

/*---------------------------------------------------------------------------
 * Function : rsvp_state_init
 * Input    : None.
 * Output   : None.
 * Synopsis : This function initializes the RSVP Softstate Module.
 * Callers  : rsvp_init in rsvpd.c
 *-------------------------------------------------------------------------*/
void 
rsvp_state_init (void)
{
   /* Make sure that rm, rsvp are initialized.*/
   assert (rm->rsvp);
   
   /* Initialize the global psb_ipv4_obj.*/
   rm->rsvp->psb_ipv4_obj = psb_ipv4_sender_hash_create (PSB_SENDER_HASH_SIZE);
   
   if (!rm->rsvp->psb_ipv4_obj)
      assert (0);

#ifdef HAVE_IPV6 
   /* Initailize the global psb_ipv6_obj.*/
   rm->rsvp->psb_ipv6_obj = psb_ipv6_sender_hash_create (PSB_SENDER_HASH_SIZE);
   
   if (!rm->rsvp->psb_ipv6_obj)
      assert (0);
#endif /* HAVE_IPV6.*/

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_psb_lookup_create
 * Input    : session_info = Pointer to struct rsvp_session_info.
 *          : st_info      = Pointer to struct rsvp_st_info.
 *          : rsvp_psb     = Pointer to struct rsvp_psb found or created.
 * Output   : If found returns 1 else returns 0 after creating the rsvp_psb.
 *            Returns -1 if some error occurs while processing it.
 * Synopsis : This function looks up the rsvp_psb that matches all the keys 
 *            in input arguments. If match found then returns the pointer to
 *            input arg rsvp_psb. If not found then the rsvp_psb is created.
 *            and the pointer is returned to input arg rsvp_psb.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_psb_lookup_create (struct rsvp_session_info *session_info,
                        struct rsvp_st_info	 *st_info,
                        struct rsvp_psb	         *rsvp_psb)
{
   struct hash *psb_sender_hash;
   struct psb_ipv4_sender_hash_entry *ipv4_hash_entry;
#ifdef HAVE_IPV6
   struct psb_ipv6_sender_hash_entry *ipv6_hash_entry;
#endif /* HAVE_IPV6.*/
   struct avl_table *psb_session_avl;
   struct psb_session_avl_entry *avl_entry;
   struct listnode *node;
   int create_flag = 0;

   /* Sanity Check for inputs.*/
   if (!session_info || !st_info)
      return -1;

   /* Make sure that rsvp main module is initialized.*/
   assert (rm->rsvp);
   /* Get the sender hash.*/
   if (st_info->type == ST_INFO_LSP_TUN_IPV4)
   {
      struct rsvp_st_info_lsp_tun_ipv4 *st_info_ipv4 = 
                                             &st_info->st_info_lsp_tun_ipv4_t;

      psb_sender_hash = (struct hash *)rm->rsvp->psb_ipv4_obj;
      assert (psb_sender_hash);
      ipv4_hash_entry = psb_ipv4_sender_hash_entry_lookup (psb_sender_hash, 
                                                    &st_info_ipv4->sender_addr);
      /* If not found then create it.*/
      if (!ipv4_hash_entry)
      {
         ipv4_hash_entry = psb_ipv4_sender_hash_entry_insert (psb_sender_hash,
                                                   &st_info_ipv4->sender_addr);
         create_flag = 1;
      }

      psb_session_avl = ipv4_hash_entry->session_avl;
   }
#ifdef HAVE_IPV6
   else
   {
      struct rsvp_st_info_lsp_tun_ipv6 *st_info_ipv6 =
                                            &st_info->st_info_lsp_tun_ipv6_t;

      psb_sender_hash = (struct hash *)rm->rsvp->psb_ipv6_obj;
      assert (psb_sender_hash);
      ipv6_hash_entry = psb_ipv6_sender_hash_entry_lookup (psb_sender_hash,
                                                    &st_info_ipv6->sender_addr);
      /* If not found then create the hash entry for this sender.*/
      if (!ipv6_hash_entry)
      {
         ipv6_hash_entry = psb_ipv6_sender_hash_entry_insert (psb_sender_hash,
                                                   &st_info_ipv6->sender_addr);
         create_flag = 1;
      }
      psb_session_avl = ipv6_hash_entry->session_avl;
   } 
#endif /* HAVE_IPV6.*/

   /* Get the AVL Node (session) that corresponds to the input Tunnel ID for 
      this sender.*/
   if (session_info->type == SESSION_INFO_LSP_TUN_IPV4)
   {
      struct rsvp_session_info_lsp_tun_ipv4 *session_info_ipv4= 
                                   &session_info->session_info_lsp_tun_ipv4_t;
      /* Check if sender lookup had been successful earlier, then lookup 
         its session_avl_tree.*/
      if (!create_flag)
      {
         avl_entry = psb_session_avl_entry_lookup (psb_session_avl, 
                                                session_info_ipv4->tun_id);
         /* If Tunnel ID not found then create a new entry corresponding to 
            the Tunnel ID.*/ 
         if (!avl_entry) 
         {
            avl_entry = psb_session_avl_entry_insert (psb_session_avl,
                                                  session_info_ipv4->tun_id);
            create_flag = 1;
         }
      }
      /* Else no need for lookup in avl_tree as sender hash entry itself was 
         created.*/
      else
         avl_entry = psb_session_avl_entry_insert (psb_session_avl,
                                               session_info_ipv4->tun_id);
   }
#ifdef HAVE_IPV6
   else
   {
      struct rsvp_session_info_lsp_tun_ipv6 *session_info_ipv6 =
                                  &session_info->session_info_lsp_tun_ipv6_t;

      /* Check if the sender_hash entry for this sender was created earlier.*/
      if (!create_flag)
      {
         avl_entry = psb_session_avl_entry_lookup (psb_session_avl,
                                                session_info_ipv6->tun_id);
         /** If the Tunnel ID is not found then create one.*/
         if (!avl_entry)
         {
            avl_entry = psb_session_avl_entry_insert (psb_session_avl,
                                                session_info_ipv6->tun_id);
            create_flag = 1;
         }
      }
      /* Else the sender_hash_entry itself was created.*/
      else
         avl_entry = psb_session_avl_entry_insert (psb_session_avl,
                                               session_info_ipv6->tun_id);
   }
#endif /* HAVE_IPV6.*/

   /* Parse the linked list of rsvp_psbs in the avl_node and match the 
      lsp_id, tunnel destination address and ext tunnel id.*/
   if (!create_flag)
   {
      for (node = listhead (avl_entry->psb_list); node; nextnode (node))
      {
         rsvp_psb = (struct rsvp_psb *) getdata (node);

         /* Check if the sender template and session info are same.*/
         if (!memcmp (&rsvp_psb->st_info, st_info, sizeof(struct rsvp_st_info))
             && !memcmp (&rsvp_psb->session_info, session_info, 
                         sizeof (struct rsvp_session_info)))
            return 1;
      }  
   }
   
   /* Create rsvp_psb and add to the list of the session_avl_entry.*/
   rsvp_psb = rsvp_psb_create ();
   listnode_add (avl_entry->psb_list, rsvp_psb);
   return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_psb_lookup
 * Input    : session_info = Pointer to struct rsvp_session_info.
 *          : st_info      = Pointer to struct rsvp_st_info.
 * Output   : Returns pointer to object of type struct rsvp_psb if a matched
 *            entry found, else returns NULL.
 * Synopsis : This function looks up the rsvp_psb that matches all the keys 
 *            in input arguments.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
struct rsvp_psb *
rsvp_psb_lookup (struct rsvp_session_info *session_info,
                 struct rsvp_st_info	  *st_info)
{
   struct rsvp_psb *rsvp_psb = NULL;
   struct hash *psb_sender_hash;
   struct psb_ipv4_sender_hash_entry *ipv4_hash_entry;
#ifdef HAVE_IPV6
   struct psb_ipv6_sender_hash_entry *ipv6_hash_entry;
#endif /* HAVE_IPV6.*/
   struct avl_table *psb_session_avl;
   struct psb_session_avl_entry *avl_entry;
   struct listnode *node;

   /* Sanity Check for inputs.*/
   if (!session_info || !st_info)
      return NULL;

   /* Make sure that rsvp main module is initailized.*/
   assert (rm->rsvp);
   /* Get the sender hash.*/
   if (st_info->type == ST_INFO_LSP_TUN_IPV4)
   {
      struct rsvp_st_info_lsp_tun_ipv4 *st_info_ipv4 = 
                                             &st_info->st_info_lsp_tun_ipv4_t;

      psb_sender_hash = (struct hash *)rm->rsvp->psb_ipv4_obj;
      assert (psb_sender_hash);
      ipv4_hash_entry = psb_ipv4_sender_hash_entry_lookup (psb_sender_hash, 
                                                   &st_info_ipv4->sender_addr);
      if (!ipv4_hash_entry)
         return NULL;

      psb_session_avl = ipv4_hash_entry->session_avl;
   }
#ifdef HAVE_IPV6
   else
   {
      struct rsvp_st_info_lsp_tun_ipv6 *st_info_ipv6 =
                                            &st_info->st_info_lsp_tun_ipv6_t;

      psb_sender_hash = (struct hash *)rm->rsvp->psb_ipv6_obj;
      assert (psb_sender_hash);
      ipv6_hash_entry = psb_ipv6_sender_hash_entry_lookup (psb_sender_hash,
                                                    &st_info_ipv6->sender_addr);
      if (!ipv6_hash_entry)
         return NULL;

      psb_session_avl = ipv6_hash_entry->session_avl;
   } 
#endif /* HAVE_IPV6.*/

   /* Get the AVL Node (session) that corresponds to the input Tunnel ID for 
      this sender.*/
   if (session_info->type == SESSION_INFO_LSP_TUN_IPV4)
   {
      struct rsvp_session_info_lsp_tun_ipv4 *session_info_ipv4= 
                                   &session_info->session_info_lsp_tun_ipv4_t;
      avl_entry = psb_session_avl_entry_lookup (psb_session_avl, 
                                                session_info_ipv4->tun_id);
   }
#ifdef HAVE_IPV6
   else
   {
      struct rsvp_session_info_lsp_tun_ipv6 *session_info_ipv6 =
                                  &session_info->session_info_lsp_tun_ipv6_t;
      avl_entry = psb_session_avl_entry_lookup (psb_session_avl,
                                                session_info_ipv6->tun_id);
   }
#endif /* HAVE_IPV6.*/
   /* Return as not found if the session number is not found.*/
   if (!avl_entry)
      return NULL;

   /* Parse the linked list of rsvp_psbs in the avl_node and match the 
      lsp_id, tunnel destination address and ext tunnel id.*/
   for (node = listhead (avl_entry->psb_list); node; nextnode (node))
   {
      rsvp_psb = (struct rsvp_psb *) getdata (node);

      /* Check if the sender template and session info are same.*/
      if (!memcmp (&rsvp_psb->st_info, st_info, sizeof (struct rsvp_st_info)) &&
          !memcmp (&rsvp_psb->session_info, session_info, 
                   sizeof (struct rsvp_session_info)))
         return rsvp_psb;  
   }
          
   return NULL;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_psb_dump_vty
 * Input    : vty 	= Pointer to object of type struct vty
 *          : rsvp_psb 	= Pointer to object of type struct rsvp_psb
 * Output   : None
 * Synopsis : This utility function prints a psb info into vty.
 * Callers  : (TBD)
 *-------------------------------------------------------------------------*/
void
rsvp_psb_dump_vty (struct vty *vty,
                   struct rsvp_psb *rsvp_psb)
{
   return;
}

/*----------------------------------------------------------------------------
 * Utility Functions used in processing for different types of RSVP Messages.
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Function : rsvp_rro_detect_loop
 * Input    : rro_info = Pointer to the object of type struct rsvp_rro_info.
 * Output   : Returns 1 if loop is found in RRO received from Path or Resv 
 *          : message. If no loop is found then return 0.
 * Synopsis : This function is as per RRO Applicability in Section 4.4.2
 *            in RFC 3209. RRO object can be used by Path and Resv messages 
 *            as a part of loop detection mechanism. This function checks
 *            if RRO objcet info received from a message indicates a loop.
 * Callers  : rsvp_state_recv_path in rsvp_state.c
 *            rsvp_state_recv_resv in rsvp_state.c
 *-------------------------------------------------------------------------*/
int
rsvp_rro_info_detect_loop (struct rsvp_rro_info *rro_info)
{


   return 1;
}
/*-----------------------------------------------------------------------------
 * The Receive Routines from RSVP Packet Module.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_path
 * Input    : ip_hdr_info   = Pointer to struct rsvp_ip_hdr_info 
 *          : path_msg_info = Pointer to struct rsvp_path_msg_info 
 *          : rsvp_if_in    = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Path Message received from rsvp_packet
 *            layer. It does all the processing related to Path States or 
 *            Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_path (struct rsvp_ip_hdr_info   *ip_hdr_info,
                      struct rsvp_path_msg_info *path_msg_info,
                      struct rsvp_if 	        *rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_resv
 * Input    : ip_hdr_info   = Pointer to struct rsvp_ip_hdr_info 
 *          : resv_msg_info = Pointer to struct rsvp_resv_msg_info 
 *          : rsvp_if_in    = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Resv Message received from rsvp_packet
 *            layer. It does all the processing related to Path States or 
 *            Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_resv (struct rsvp_ip_hdr_info   *ip_hdr_info,
                      struct rsvp_resv_msg_info *resv_msg_info,
                      struct rsvp_if		*rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_path_err
 * Input    : ip_hdr_info       = Pointer to struct rsvp_ip_hdr_info 
 *          : path_err_msg_info = Pointer to struct rsvp_path_msg_info 
 *          : rsvp_if_in        = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Path Err Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_path_err (struct rsvp_ip_hdr_info       *ip_hdr_info,
                          struct rsvp_path_err_msg_info *path_err_msg_info,
                          struct rsvp_if		*rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_resv_err
 * Input    : ip_hdr_info       = Pointer to struct rsvp_ip_hdr_info 
 *          : resv_err_msg_info = Pointer to struct rsvp_resv_err_msg_info 
 *          : rsvp_if_in        = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Resv Err Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_resv_err (struct rsvp_ip_hdr_info 	*ip_hdr_info,
                          struct rsvp_resv_err_msg_info *resv_err_msg_info,
                          struct rsvp_if 		*rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_path_tear
 * Input    : ip_hdr_info        = Pointer to struct rsvp_ip_hdr_info 
 *          : path_tear_msg_info = Pointer to struct rsvp_path_tear_msg_info 
 *          : rsvp_if_in         = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Path Tear Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_path_tear (struct rsvp_ip_hdr_info	  *ip_hdr_info,
                           struct rsvp_path_tear_msg_info *path_tear_msg_info,
                           struct rsvp_if	          *rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_resv_tear
 * Input    : ip_hdr_info   	 = Pointer to struct rsvp_ip_hdr_info 
 *          : resv_tear_msg_info = Pointer to struct rsvp_resv_tear_msg_info 
 *          : rsvp_if_in         = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Resv Tear Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void 
rsvp_state_recv_resv_tear (struct rsvp_ip_hdr_info	  *ip_hdr_info,
                           struct rsvp_resv_tear_msg_info *resv_tear_msg_info,
                           struct rsvp_if		  *rsvp_if_in)
{

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_resv_conf
 * Input    : ip_hdr_info        = Pointer to struct rsvp_ip_hdr_info 
 *          : resv_conf_msg_info = Pointer to struct rsvp_resv_conf_msg_info 
 *          : rsvp_if_in         = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Resv Conf Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_resv_conf (struct rsvp_ip_hdr_info	  *ip_hdr_info,
                           struct rsvp_resv_conf_msg_info *resv_conf_msg_info,
                           struct rsvp_if		  *rsvp_if_in)
{
   return;
}  	

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_hello_req
 * Input    : ip_hdr_info    = Pointer to struct rsvp_ip_hdr_info 
 *          : hello_msg_info = Pointer to struct rsvp_hello_msg_info 
 *          : rsvp_if_in     = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Hello Message received from rsvp_packet
 *            layer. It does all the processing related to Path States or 
 *            Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_hello_req (struct rsvp_ip_hdr_info 	  *ip_hdr_info,
                           struct rsvp_hello_msg_info     *hello_msg_info,
                           struct rsvp_if		  *rsvp_if_in)
{
   return;

}

/*----------------------------------------------------------------------------
 * Function : rsvp_state_recv_hello_ack
 * Input    : ip_hdr_info    = Pointer to struct rsvp_ip_hdr_info 
 *          : hello_msg_info = Pointer to struct rsvp_hello_msg_info 
 *          : rsvp_if_in     = Pointer to the struct rsvp_if
 * Output   : None
 * Synopsis : This function processes a Hello Ack Message received from 
 *            rsvp_packet layer. It does all the processing related to Path 
 *            States or Resv States.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_state_recv_hello_ack (struct rsvp_ip_hdr_info	  *ip_hdr_info,
                           struct rsvp_hello_msg_info     *hello_msg_info,
                           struct rsvp_if		  *rsvp_if_in)
{
   return;

}	
