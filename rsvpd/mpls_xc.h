/* mpls_xc :- This module provides a generic dataplane abstraction for MPLS.
**            It provides libraries to create/add/delete XC (Cross Connect)
**            libraries.
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
#ifndef _ZMPLS_MPLS_XC_H
#define _ZMPLS_MPLS_XC_H
/*-------------------------------------------------------------------
 * MODULE DESCRIPTION: This module provides the a generic abstraction 
 *                     of switching/forwarding database(XC) for MPLS 
 *                     FECs.  
 *-----------------------------------------------------------------*/
/*------------------------------------------------------------------
 * Return codes for this library.
 *----------------------------------------------------------------*/
#define E_XC_NOMEM    0   
#define E_XC_OK       1
#define E_XC_FOUND    2
#define E_XC_NOTFOUND 3
/*------------------------------------------------------------------
 * An incoming label map for a LSP. It is represented with ilm+label
 * VPN ILMs are associated with loopback interface. So ifindex would
 * be of the loopback interface.
 *----------------------------------------------------------------*/
struct xc_ilm
{
   unsigned int ifindex;
   u_int32_t    label;
};

/*-----------------------------------------------------------------
 * A NHLFE Entry for a LSP.
 * ifindex = The outgoing interface index.
 * nhop    = The nexthop of the LSP
 * label   = The outgoing label for the LSP. 
 *----------------------------------------------------------------*/
struct xc_nhlfe
{
   unsigned int   ifindex;
   struct in_addr nhop;
   u_int32_t      label;
};

/*----------------------------------------------------------------
 * A Group of xc_ilms
 * - num = The number of xc_ilm entries in this group.
 * - ilm = The array of xc_ilms.
 *--------------------------------------------------------------*/
struct xc_ilm_grp
{
   unsigned int  num;
   struct xc_ilm ilm[0];
};

/*---------------------------------------------------------------
 * A Group of xc_nhlfes. This is used in xc_msg that is used 
 * as protocol packet for IPC to zmpls.
 *-------------------------------------------------------------*/
struct xc_nhlfe_grp
{
   /*----------------------------------------------------------
    * Number of xc_nhlfe in this group.
    *--------------------------------------------------------*/
   unsigned int    num;
   /*----------------------------------------------------------
    * The array of xc_nhlfe entries in this group.
    *--------------------------------------------------------*/
   struct xc_nhlfe nhlfe[0]; 
};

/*--------------------------------------------------------------
 * xc :- A Generic MPLS Cross Connect 
 *------------------------------------------------------------*/
struct xc
{
   /*----------------------------------------------------------
    * The user of the XC. Normally this is the fec_handle that
    * uses the XC.
    *--------------------------------------------------------*/
   void        *user_handle;
   /*----------------------------------------------------------
    * The list of xc_ilm(s) to be programmed for this XC.
    *--------------------------------------------------------*/
   struct list *xc_ilm_list;
   /*----------------------------------------------------------
    * The list of xc_nhlfe(s) to be programmed for this XC.
    *--------------------------------------------------------*/
   struct list *xc_nhlfe_list;
};

/*------------------------------------------------------------
 * XC Programming Request : This is the info used by users of  
 * the XC to program the XC.
 *----------------------------------------------------------*/
struct xc_req
{
   /*---------------------------------------------------------
    * The list of xc_ilm to be added to a XC.
    *-------------------------------------------------------*/
   struct list *xc_ilm_add_list;
   /*--------------------------------------------------------
    * The list of xc_ilm to be deleted from a XC.
    *-------------------------------------------------------*/
   struct list *xc_ilm_del_list;
   /*--------------------------------------------------------
    * The list of xc_nhlfe to be added to a XC.
    *-------------------------------------------------------*/
   struct list *xc_nhlfe_add_list;
   /*--------------------------------------------------------
    * The list of xc_nhlfe to be deleted from a XC.
    *------------------------------------------------------*/
   struct list *xc_nhlfe_del_list;
};
/*------------------------------------------------------------
 * XC Programming Message : This is a message sent to
 * zmpls for programming the XC.
 *----------------------------------------------------------*/
/*
 zhurish
struct xc_msg
{
 
  // struct fec fec_info;
   u_char xc_data[0];
};
*/

/*-----------------------------------------------------------
 * MACRO for parsing each XC.
 *---------------------------------------------------------*/
#define FOR_ALL_XC_START(xc) {\
	struct avl_traverser avl_t;\
        extern struct avl_table *xc_table;\
	\
	if (xc_table->avl_count) {\
	   avl_t_init (&avl_t, xc_table);\
	   \
	   while ((xc = avl_t_next (&avl_t))) {\

#define FOR_ALL_XC_END() }\
			}\
		      }	
/*-----------------------------------------------------------
 * Prototype exported by this library.
 *---------------------------------------------------------*/
struct xc_ilm *
mpls_xc_ilm_new (void);

void
mpls_xc_ilm_free (struct xc_ilm *);

struct xc_nhlfe *
mpls_xc_nhlfe_new (void);

void
mpls_xc_nhlfe_free (struct xc_nhlfe *);

struct xc_req *
mpls_xc_req_new (void);

void
mpls_xc_req_free (struct xc_req *);

int
mpls_xc_add (void 	   *,
             struct xc_req *,
             struct xc     **);
int
mpls_xc_update (void          *,
                struct xc_req *,
                struct xc     **);

struct xc *
mpls_xc_lookup (void          *);

int 
mpls_xc_delete (void *);

int
mpls_xc_init (void); 
#endif
