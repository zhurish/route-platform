/* mpls_xc.c :- The XC Table for Dataplane Programming.
**
** This file is part of GNU zMPLS (http://zmpls.sourceforge.net)
**
** Copyright (C) 2003 Pranjal Kumar Dutta <prdutta@users.sourceforge.net>
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
/*-------------------------------------------------------------------
 * MODULE DESCRIPTION: This module provides the a generic abstraction 
 *                     of switching/forwarding database(XC) for FECs 
 *                     signalled by LDP. The XCs are programmed by 
 *                     ldp_fec module. This module inturn sends the 
 *                     the XCs into zmplsd via messages. zmplsd 
 *                     programs the XCs into teh kernel.
 *-----------------------------------------------------------------*/
#include <zebra.h>

#include "linklist.h"

#include "memory.h"
#include "linklist.h"
#include "if.h"
#include "command.h"
#include "vty.h"
#include "prefix.h"

#include "avl.h"
#include "rsvpd/rsvpd.h"

#include "mpls_xc.h"

/*-----------------------------------------------------------------
 * The mpls_xc table.
 *---------------------------------------------------------------*/
struct avl_table *xc_table; 
/*-----------------------------------------------------------------
 * Function : xc_avl_table_cmp_fn 
 * Input    : key   = Pointer to the key.
 *          : data  = Pointer to the data.
 *          : param = always NULL.
 * Output   : Returns 0 of key == data, -1 if key < data , else 1.
 * Synopsis : This function is required by the AVL table libray used
 *          : to maintain the database of XCs.
 * Callers  : Callback function internally used by avl library.
 *---------------------------------------------------------------*/ 
static int 
xc_avl_table_cmp_fn (const void *key,
		     const void *data,
                     void       *param)
{
   void *user_handle1 = ((struct xc *)key)->user_handle;   
   void *user_handle2 = ((struct xc *)data)->user_handle;

   if ((unsigned int)user_handle1 == (unsigned int)user_handle2)
      return 0;
   else if ((unsigned int)user_handle1 < (unsigned int)user_handle2)
      return -1;
   else
      return 1;
}

/*-------------------------------------------------------------------
 * Function : xc_free
 * Input    : None.
 * Output   : Returns struct xc.
 * Synopsis : This function frees a XC.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
static void 
xc_free (struct xc *xc)
{
   struct listnode *node;
   void            *data;
   /*------------------------------------------
    * Free up the xc_ilm and xc_nhlfe elements
    *-----------------------------------------*/
   if (xc->xc_ilm_list)
   {
      if (listcount (xc->xc_ilm_list))
      {
         LIST_LOOP (xc->xc_ilm_list, data, node)
	 //for (ALL_LIST_ELEMENTS_RO (xc->xc_ilm_list, data, node))
            mpls_xc_ilm_free ((struct xc_ilm *)data);
      }
      list_free (xc->xc_ilm_list);
   }
  
   if (xc->xc_nhlfe_list)
   { 
      if (listcount (xc->xc_nhlfe_list))
      {
         LIST_LOOP (xc->xc_nhlfe_list, data, node)
            mpls_xc_nhlfe_free ((struct xc_nhlfe *)data);
      }
      list_free (xc->xc_nhlfe_list);
   }
   /*-----------------------------------------
    * Free up the XC itself.
    *----------------------------------------*/
   XFREE (MTYPE_MPLS_XC, xc);
   return;
}

/*-------------------------------------------------------------------
 * Function : mpls_xc_req_free
 * Input    : req = Pointer to struct xc_req
 * Output   : None
 * Synopsis : This function frees an object of type struct xc_req.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
void 
mpls_xc_req_free (struct xc_req *req)
{
   struct listnode *node;
   void            *data;

   if (req->xc_ilm_add_list)
   {
      LIST_LOOP (req->xc_ilm_add_list, data, node)
         mpls_xc_ilm_free ((struct xc_ilm *)data);
      list_free (req->xc_ilm_add_list);
   }
  
   if (req->xc_ilm_del_list)
   {
      LIST_LOOP (req->xc_ilm_del_list, data, node)
         mpls_xc_ilm_free ((struct xc_ilm *)data);
      list_free (req->xc_ilm_del_list);
   }

   if (req->xc_nhlfe_add_list)
   {
      LIST_LOOP (req->xc_nhlfe_add_list, data, node)
         mpls_xc_nhlfe_free ((struct xc_nhlfe *)data);
      list_free (req->xc_nhlfe_add_list);
   }

   if (req->xc_nhlfe_del_list)
   {
      LIST_LOOP (req->xc_nhlfe_del_list, data, node)
         mpls_xc_nhlfe_free ((struct xc_nhlfe *)data);
      list_free (req->xc_nhlfe_del_list);
   }
 
   XFREE (MTYPE_MPLS_XC_REQ, req);
   return;
}

/*-------------------------------------------------------------------
 * Function : mpls_xc_req_new
 * Input    : None.
 * Output   : Returns pointer to struct xc_req.
 * Synopsis : This function allocates an object of type struct xc_req
 *          : and returns it.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
struct xc_req *
mpls_xc_req_new (void)
{
   struct xc_req *req = NULL;

   if (!(req = XCALLOC (MTYPE_MPLS_XC_REQ, sizeof (struct xc_req))))
      goto fail;
   if (!(req->xc_ilm_add_list = list_new ())) 
      goto fail;
   if (!(req->xc_ilm_del_list = list_new ()))
      goto fail;
   if (!(req->xc_nhlfe_add_list = list_new ()))
      goto fail;
   if (!(req->xc_nhlfe_del_list = list_new ()))
      goto fail;
   
   return req;

fail:
   if (req)
      mpls_xc_req_free (req);
   return NULL;  
}

/*-------------------------------------------------------------------
 * Function : xc_new
 * Input    : None.
 * Output   : Returns struct xc.
 * Synopsis : This function allocates a XC.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/ 
static struct xc *
xc_new (void)
{
   struct xc *xc = NULL;

   if (!(xc = XCALLOC (MTYPE_MPLS_XC, sizeof (struct xc))))
      goto fail;
   if (!(xc->xc_ilm_list = list_new ()))
      goto fail;
   if (!(xc->xc_nhlfe_list = list_new ()))
      goto fail;  

   return xc;
 
fail:
   if (xc)
      xc_free (xc);
   return NULL;
}

/*-------------------------------------------------------------------
 * Function : mpls_xc_ilm_new
 * Input    : None
 * Output   : Returns pointer to xc_ilm
 * Synopsis : This function allocates an object of type xc_ilm.
 * Callers  : TBD.
 *----------------------------------------------------------------*/
struct xc_ilm *
mpls_xc_ilm_new (void)
{
   return XCALLOC (MTYPE_MPLS_XC_ILM, sizeof (struct xc_ilm));
}

/*-------------------------------------------------------------------
 * Function : mpls_xc_ilm_free
 * Input    : ilm = Pointer to struct xc_ilm.
 * Output   : None.
 * Synopsis : This function frees up an object of type xc_ilm.
 * Callers  : TBD.
 *-----------------------------------------------------------------*/
void
mpls_xc_ilm_free (struct xc_ilm *ilm)
{
   XFREE (MTYPE_MPLS_XC_ILM, ilm);
}

/*------------------------------------------------------------------
 * Function : mpls_xc_nhlfe_new
 * Input    : None.
 * Output   : xc_nhlfe = Pointer to struct xc_nhlfe.
 * Synopsis : This function allocates an object of type 
 *          : struct xc_nhlfe.
 * Callers  : TBD.
 *---------------------------------------------------------------*/ 
struct xc_nhlfe *
mpls_xc_nhlfe_new (void)
{
   return XCALLOC (MTYPE_MPLS_XC_NHLFE, sizeof (struct xc_nhlfe));
}

/*----------------------------------------------------------------
 * Function : mpls_xc_nhlfe_free
 * Input    : nhlfe = Pointer to struct nhlfe.
 * Output   : None.
 * Synopsis : This function frees up an object of type struct 
 *          : nhlfe.
 * Callers  : TBD.
 *--------------------------------------------------------------*/
void
mpls_xc_nhlfe_free (struct xc_nhlfe *nhlfe)
{
   XFREE (MTYPE_MPLS_XC_NHLFE, nhlfe);
}

/*------------------------------------------------------------------
 * Function : mpls_xc_lookup 
 * Input    : user_handle = Pointer to the user handle of this XC.
 * Output   : Returns pointer to struct xc if found, else returns 
 *          : NULL.
 * Synopsis : This is the lookup function of a XC corresponding 
 *          : to the input user_handle.
 * Callers  : 
 *----------------------------------------------------------------*/
struct xc *
mpls_xc_lookup (void *user_handle)
{
   struct xc dummy;
   bzero (&dummy, sizeof (struct xc));
   dummy.user_handle = user_handle;
   return (struct xc *)avl_find (xc_table, &dummy);
} 

/*------------------------------------------------------------------
 * Function : mpls_xc_add
 * Input    : user_handle = The user of the XC, who adds this XC.
 *          :               This handle is used by user to later 
 *          :               refer to the corresponding XC.
 *          : req         = The XC Info to be requested/programmed  
 *          :               for this user_handle.
 *          : xc          = Returns the pointer to the xc handle 
 *          :               that is added to xc table.
 * Output   : Returns the E_XC_ADD_OK if XC is successfully added.
 *          : if XC already exists then returns E_XC_EXIST. If XC 
 *          : addition fails due to abnormal condition then return
 *          : E_XC_ADD_NOMEM.
 * Synopsis : This function creates a new XC and fills with relevant 
 *          : info. The XC is added to the AVL database.
 * Callers  : TBD.
 *----------------------------------------------------------------*/ 
int
mpls_xc_add (void          *user_handle,
             struct xc_req *req,
             struct xc     **xc) 
{
   struct listnode *node;
   void            *list_data1;
   void            *list_data2;
   int             ret = E_XC_OK;
   /*--------------------------------------------
    * We must have valid inputs.
    *------------------------------------------*/
   assert (user_handle && req && xc);
   /*--------------------------------------------
    * If we already find a xc with this handle 
    * then we return it saying that it already 
    * exists.
    *------------------------------------------*/
   if ((*xc = mpls_xc_lookup (user_handle)))
   {
      ret = E_XC_FOUND;
      goto fail;
   }
   /*--------------------------------------------
    * Allocate a new XC and check mem failure.
    *------------------------------------------*/
   if (!(*xc = xc_new ()))
   {
      ret= E_XC_NOMEM;
      goto fail;
   }
   /*--------------------------------------------
    * We assign the user handle of this XC.
    *------------------------------------------*/
   (*xc)->user_handle = user_handle;
   /*---------------------------------------------
    * Add the xc_ilms from corresponding ilm_list
    * to be added in req.
    *-----------------------------------------*/
   LIST_LOOP (req->xc_ilm_add_list, list_data1, node)
   {
      if (!(list_data2 = mpls_xc_ilm_new ()))
      {
         ret = E_XC_NOMEM;
         goto fail;
      }
      memcpy (list_data2, list_data1, sizeof (struct xc_ilm));
      listnode_add ((*xc)->xc_ilm_list, list_data2);
   } 
   /*---------------------------------------------
    * Add the xc_nhlfe from corresponding nhlfe 
    * list to be added in req.
    *-------------------------------------------*/
   LIST_LOOP (req->xc_nhlfe_add_list, list_data1, node)
   {
      if (!(list_data2 = mpls_xc_nhlfe_new ()))
      {
         ret = E_XC_NOMEM;
         goto fail;
      }
      memcpy (list_data2, list_data1, sizeof (struct xc_nhlfe));
      listnode_add ((*xc)->xc_nhlfe_list, list_data2);
   }
   /*---------------------------------------------
    * Add this XC to the XC Table.
    *-------------------------------------------*/
   avl_insert (xc_table, *xc);
   return ret;
 
fail:
   xc_free (*xc);
   return ret;
}

/*-----------------------------------------------------------------
 * Function : mpls_xc_update
 * Input    : xc_handle = The handle of the xc.
 *          : req       = Pointer to struct xc_req.
 *          : xc        + Pointer to the handle of xc.
 * Output   : Returns E_XC_OK if updated successfully.   
 * Synopsis : This functions updates an existing XC in the xc_table
 *          : from the req. The function returns the handle of the 
 *          : updated xc.
 *----------------------------------------------------------------*/
int
mpls_xc_update (void          *user_handle,
                struct xc_req *req,
                struct xc     **xc) 
{
   struct listnode *node1;
   struct listnode *node2;
   void            *data1;
   void            *data2;
   /*------------------------------------------------------------
    * Lookup the XC corresponding to the fec_handle.
    *----------------------------------------------------------*/ 
   if (!(*xc = mpls_xc_lookup (user_handle)))
      return E_XC_NOTFOUND; 
   /*-------------------------------------------------------------
    * Delete the ILMs requested to be deleted.
    *-----------------------------------------------------------*/ 
   LIST_LOOP (req->xc_ilm_del_list, data1, node1)
   {
      LIST_LOOP ((*xc)->xc_ilm_list, data2, node2)
      {
         if (!memcmp (data1, data2, sizeof (struct xc_ilm)))
            break;
      }
      LISTNODE_DELETE ((*xc)->xc_ilm_list, node2);
   }     
   /*------------------------------------------------------------
    * Delete the NHLFE entries requested to be deleted.
    *-----------------------------------------------------------*/
   LIST_LOOP (req->xc_nhlfe_del_list, data1, node1)
   {
      LIST_LOOP ((*xc)->xc_nhlfe_list, data2, node2)
      {
         if (!memcmp (data1, data2, sizeof (struct xc_nhlfe)))
            break;
      }
      LISTNODE_DELETE ((*xc)->xc_nhlfe_list, node2);
   }
   /*------------------------------------------------------------
    * Add the ILM entries to be added.
    *----------------------------------------------------------*/
   LIST_LOOP (req->xc_ilm_add_list, data1, node1)
   {
      if (!(data2 = mpls_xc_ilm_new ()))
         return E_XC_NOMEM;
      memcpy (data2, data1, sizeof (struct xc_ilm));
      listnode_add ((*xc)->xc_ilm_list, data2);   
   }
   /*-----------------------------------------------------------
    * Add the NHLFE Entries to be added.
    *----------------------------------------------------------*/
   LIST_LOOP (req->xc_nhlfe_del_list, data1, node1)
   {
      if (!(data2 = mpls_xc_nhlfe_new ()))
        return E_XC_NOMEM;
      memcpy (data2, data1, sizeof (struct xc_nhlfe));
      listnode_add ((*xc)->xc_nhlfe_list, data2);
   }
   return E_XC_OK;
}

/*------------------------------------------------------------------
 * Function : mpls_xc_delete
 * Input    : user handle = The user handle of this XC.
 * Output   : returns E_XC_OK if the xc is found and deleted. Else 
 *          : if the xc is not found for the input handle then 
 *          : returns E_XC_NOTFOUND.
 * Synopsis : This function deletes a XC corresponding to the 
 *          : user_handle from the XC table.
 * Callers  : TBD.
 *----------------------------------------------------------------*/
int
mpls_xc_delete (void *user_handle)
{
   struct xc       *xc = NULL;
   /*-------------------------------------------
    * Lookup up if the XC exists. If not there 
    * is nothing to delete.
    *-----------------------------------------*/
   if (!(xc = mpls_xc_lookup (user_handle)))
      return E_XC_NOTFOUND;
   avl_delete (xc_table, xc); 
   /*-------------------------------------------
    * Free the XC
    *-----------------------------------------*/
   xc_free (xc);
   return E_XC_OK;
}

/*------------------------------------------------------------------
 * Function : mpls_xc_init
 * Input    : None
 * Output   : returns pointer to struct avl_table.
 * Synopsis : This function initializes a XC table (AVL table ).
 * Callers  : This is a library and can be used in any zMPLS module.
 *----------------------------------------------------------------*/
int
mpls_xc_init (void)
{
   int ret = E_XC_OK;
   /*-------------------------------------------------------
    * Create AVL table to store the XCs.
    *-----------------------------------------------------*/
   if (!(xc_table =  avl_create (xc_avl_table_cmp_fn,
                                 NULL,
                                 &avl_allocator_default)))
      ret = E_XC_NOMEM;
   return ret;
}

