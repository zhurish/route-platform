/* The RSVP Label Manager Library. 
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


#include "labelmgr.h"

/*-----------------------------------------------------------------------------
 * The Static Label Manager in this module.
 *---------------------------------------------------------------------------*/
static struct labelmgr  labelmgr_static;

/*----------------------------------------------------------------------------
 * The Pointer to label manager exported by this module to RSVPD/LDPD
 *--------------------------------------------------------------------------*/
struct labelmgr *labelmgr;
 
/*-----------------------------------------------------------------------------
 * Function : labelmgr_init
 * Input    : start_label = The Labal Value on which the 8K range started.
 * Output   : Returns the pointer to struct lm initialized.
 * Synopsis; This is utility function that initializes a Label Manager.
 * Callers  : (TBD)..
 *---------------------------------------------------------------------------*/
struct labelmgr *
labelmgr_init (u_int32_t start_label)
{
   if (start_label <= 16)
   {
      printf ("[labelmgr_init] Fatal :Can't start with label %d\n", 
              start_label);
      assert(0);
   }
      
   bzero (&labelmgr_static, sizeof (struct labelmgr));
   labelmgr = &labelmgr_static;
   labelmgr->offset = start_label; 
   labelmgr->hash_curr = 0;
   labelmgr->free = LM_TOTAL_LABELS;
 
   /*-----------------------------------------------------
    * Initialize the bitmasks in each hash bucket as free
    *---------------------------------------------------*/
   memset (labelmgr->hash_bitmask, 0, LM_HASH_SIZE*sizeof (u_char));
   
   return labelmgr;
}
             
/*---------------------------------------------------------------------------
 * Function : hash_bitmask_alloc_bit 
 * Input    : hash_bitmask = A bitmask in u_char
 * Output   : Returns a free bit position in the bitmask.
 * Synopsis : This utility function seeks for the first free bit position in
 *            the input bitmask. If not free return -1.
 * Callers  : labelmgr_alloc_label in this module. Before calling this make 
 *            suree that that this node has free labels.
 *--------------------------------------------------------------------------*/
static u_char
hash_bitmask_alloc_bit (u_char *hash_bitmask)
{
   u_char bit_mask = 0x01;
   u_char bit_pos = 1;

   /* Search for first free bit in the label_bitmap.*/
   while (*hash_bitmask & bit_mask)
   {
      bit_mask = bit_mask << 1;
      bit_pos++;
   
      /* Search can't exceed more than 8 bites.*/
      if (bit_pos > 8)
         return -1;
   }
   /* Mark this bit_pos as allocated.*/
   *hash_bitmask |= bit_mask;
   /* Return the bit position allocated.*/ 
   return bit_pos;
} 
/*----------------------------------------------------------------------------
 * Function : labelmgr_get_label
 * Input    : labelmgr = Pointer to struct of type labelmgr
 * Output   : Returns a free MPLS Label. If full returns 0.
 * Synopsis ; This utility returns a free MPLS label from this label mgr.
 * Callers  : (TBD)..
 *--------------------------------------------------------------------------*/
u_int32_t
labelmgr_alloc_label (struct labelmgr *labelmgr)
{
   u_int32_t hash_bucket;
   u_char bit_pos;
   /* Sanity Check.*/
   assert (labelmgr);

   /* Check if the label manager in NOT FULL. */
   if (!LABELMGR_HAS_LABEL(labelmgr))
   {
      printf ("Label Manager is FULL\n");
      return 0;
   }

   hash_bucket = labelmgr->hash_curr;
   
   /* Get the next free hash bucket. */
   while (!HASH_BITMASK_HAS_FREE_BIT(labelmgr->hash_bitmask[hash_bucket]))
   {
      hash_bucket++;
      /* Check if this is the wraparound time for the hash.*/
      if (hash_bucket > 1000)
         hash_bucket = 0;
   }
   
   /* Get the first free bit position in the bit map of this hash bucket. */
   bit_pos = hash_bitmask_alloc_bit (labelmgr->hash_bitmask + hash_bucket);
   /* Set that one more label is allocated in the labelmgr. */
   labelmgr->free--;
   /* Increment the hash bucket. */
   labelmgr->hash_curr++;

   /* Check if this is time of wraparound of hash.*/
   if (labelmgr->hash_curr == 1000)
      labelmgr->hash_curr = 0;

   /* Return the label allocated */ 
   return (hash_bucket* LM_PER_HASH_LABELS + bit_pos + labelmgr->offset);
};

/*-----------------------------------------------------------------------------
 * Function : labelmgr_free_label 
 * Input    : labelmgr = Pointer to struct labelmgr.
 *          : label    = Value to label to be freed.
 * Output   : None
 * Synopsis : This function frees a label to Label Manager.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
void 
labelmgr_free_label (struct labelmgr *labelmgr,
                     u_int32_t label)
{
   int hash_bucket;
   int bit_mask = 1;
   /* Sanity Check. */
   if (!labelmgr)
      return;
   
   /* Check if the label freed is from reserved space. */
   if (label <= 16)
   {
      printf ("Can't free label %d - not permitted to free reserved labels\n",
               label);
      return;
   }
   /* Get the hash bucket corresponding to the label.*/
   hash_bucket = (label - labelmgr->offset)/LM_PER_HASH_LABELS;
   /* Get the bitmask of the label to be freed.*/
   bit_mask = bit_mask << ((label - labelmgr->offset) % LM_PER_HASH_LABELS);
   labelmgr->hash_bitmask[hash_bucket] &= ~bit_mask;
   /* Set one more label as free in label manager. */
   labelmgr->free++;
   return; 
}
