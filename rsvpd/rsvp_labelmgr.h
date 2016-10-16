/*This is header for the Label Manager. This is common for LDP and RSVP.
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

/*----------------------------------------------------------------------------
 * What is a Label Manager? 
 * In MPLS we need to assign labels to set up LSPs. The Label Manager is 
 * responsible for distributing labels and taking back unused label. I did
 * a siple hash based implementation to randomize label distribution 
 * in a range of 8K starting from a configured start label. Each has entry is 
 * a bit map in 1 byte value.
 *-------------------------------------------------------------------------*/
#define LM_HASH_SIZE		1000
#define LM_START_LABEL_LDP	17
#define LM_START_LABEL_RSVP	8018
#define LM_PER_HASH_LABELS	8
#define LM_TOTAL_LABELS		(LM_HASH_SIZE*LM_PER_HASH_LABELS)

/*----------------------------------------------------------------------------
 * The Data Structure the defines  a Label Manager Entity.
 *--------------------------------------------------------------------------*/
struct labelmgr
{
  u_char     free;  /*The number of free labels remaining*/
  u_int32_t  offset;/* Start label for the range of labels. */
  u_int32_t  hash_curr;	 /* Hash bucket from where next label to be given.*/
  u_char     hash_bitmask[LM_HASH_SIZE];/*hash of bit map of labels in u_char*/
};
 
/*---------------------------------------------------------------------------
 * MACRO Functions.
 *--------------------------------------------------------------------------*/
#define LABELMGR_HAS_LABEL(X)  		 ((X)->free)
#define HASH_BITMASK_HAS_FREE_BIT(X)	 ((X) ^ 0xFF)
 
/*----------------------------------------------------------------------------
 * Function prototypes for this library.
 *--------------------------------------------------------------------------*/
struct labelmgr *labelmgr_init (u_int32_t);
u_int32_t labelmgr_alloc_label (struct labelmgr *);
void 	  labelmgr_free_label (struct labelmgr *,
                              u_int32_t); 
