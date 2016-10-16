/* The Main RSVP-TE Daemon Procedures.
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

#include "prefix.h"
#include "thread.h"
#include "lsp.h"
#include "stream.h"
#include "command.h"
#include "sockunion.h"
#include "network.h"
#include "memory.h"
#include "log.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "ternary.h"
#include "avl.h"
#include "labelmgr.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_zebra.h"
#include "rsvpd/rsvp_network.h"
#include "rsvpd/rsvp_vty.h"
#include "rsvpd/rsvp_obj.h"
#include "rsvpd/rsvp_packet.h"
#include "rsvpd/rsvp_state.h"
#include "rsvpd/rsvp_lsp.h"
#include "rsvpd/rsvp_route.h"

/*--------------------------------------------------------------------------
 * RSVPD Process wide configuration.
 *------------------------------------------------------------------------*/
static struct rsvp_master rsvp_master;
static struct rsvp_tunnel_if_obj rsvp_tunnel_if_obj;
/*-------------------------------------------------------------------------
 * RSVPD Processwide pointer to export.
 *------------------------------------------------------------------------*/
struct rsvp_master *rm;

/*-------------------------------------------------------------------------
 * Function : rsvp_master_init
 * Input    : None
 * Output   : None
 * Synopsis : RSVP Master Initialization.
 * Callers  : main() in rsvp_main.c 
 *-----------------------------------------------------------------------*/
void
rsvp_master_init ()
{
   memset (&rsvp_master, 0, sizeof (struct rsvp_master));
   rm = &rsvp_master;
   rm->master = thread_master_create ();
   assert (rm->master);
   rm->start_time = time (NULL);

   return;
}
/*-------------------------------------------------------------------------
 * Function : rsvp_init
 * Input    : None
 * Output   : None
 * Synopsis : This procedure initializes the complete RSVP-TE Stack. All other
 *            sub-modules of RSVP-TE Daemon would be initialized from this. 
 * Callers  : main() in rsvp_main.c
 *------------------------------------------------------------------------*/
void
rsvp_init ()
{

   /* Create and initialize the RSVP Object First. */
   rm->rsvp = rsvp_create ("RSVP-TE");

   /* If RSVP Creation is not successful then stop here.*/
   if (!rm->rsvp)
   {  
      printf ("Stopping the RSVP-TE Daemon!\n");
      return;
   }
   /* Initializes the rsvp_if module. */
   rsvp_if_init ();
  /* Initialize RSVP-TE interface with Zebra, i.e the zclient. It is hooked
     to rm->rsvp within this init function */
   rsvp_zebra_init ();

  /* Initialize the Route Module of RSVP. */
   rsvp_route_table_init ();

  /* Initialize the RSVP Softstate Module.*/
   rsvp_state_init ();
  /* Initialize the RSVP LSP Module. */
   rsvp_lsp_init ();
  
   /* Install the commands supported bt rsvpd.c module. */
   rsvp_vty_init ();

   /* Initialize the label manager for RSVP-TE Daemon. */
   rm->rsvp->labelmgr = labelmgr_init (LM_START_LABEL_RSVP);

   return;
};

/*----------------------------------------------------------------------------
 * Function : rsvp_init_set_complete
 * Input    : None
 * Output   : None
 * Synopsis : This function sets that the RSVP Module initialization complete.
 * Callers  : main() in rsvpd_main.c
 *--------------------------------------------------------------------------*/
void
rsvp_init_set_complete (void)
{
   /* Make sure that RSVP Main Module is initialized.*/
   assert (rm->rsvp);
   /* Setting this flag indicates that the initialization of all submodules
      are complete now.*/
   rm->rsvp->init_flag = 1;

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_create 
 * Input    ; String as name of the RSVP Instance
 * Output   : struct rsvp = new allocated rsvp instance
 * Synopsis : This function allocates a new rsvp instance during stack
 *            initialization.
 * Callers  : rsvp_master_init in rsvpd.c. It is called during RSVP-TE Stack
 *            initialization.  
 *--------------------------------------------------------------------------*/
struct rsvp *
rsvp_create (char *name)
{
   struct rsvp *rsvp;

   /* Allocate a new RSVP data structure. */
   rsvp = XCALLOC (MTYPE_RSVP_TOP, sizeof (struct rsvp));

   if (rsvp == NULL)
      return NULL;

   /* Create the read/write socket for this Daemon. */
   rsvp->sock = rsvp_sock_init ();

   if (rsvp->sock >=0)
      rsvp->t_read = thread_add_read (master, rsvp_read, rsvp, rsvp->sock);
    
   /* The RSVP Socket couldn't be opened. */ 
   else
   {
      /* Free up the allocated rsvp object and return error. */
      XFREE (MTYPE_RSVP_TOP, rsvp);
      return NULL;
   }

   /* Initialize the linked list for rsvp_ip_paths. */
   if (!(rsvp->ip_path_list = list_new ()))
       assert (0);

   /* Initialize the tunnel_if_obj. */
   if (!(rsvp->tunnel_if_obj = rsvp_tunnel_if_obj_init ()))
      assert (0);
 
   rsvp->rsvp_if_write_q = list_new ();
   rsvp->name = name;
  
   return rsvp;   
}

/*--------------------------------------------------------------------------
 * Function : rsvp_ip_hop_create
 * Input    : hop_type = The type of hop in int.
 *            hop = Pointer to struct prefix.
 * Output   : Returns pointer to struct rsvp_ip_hop if successful, else
 *            returns NULL.
 * Synopsis : This function is utility for creating a struct rsvp_ip_hop.
 *------------------------------------------------------------------------*/
struct rsvp_ip_hop *
rsvp_ip_hop_create (int hop_type,
                    struct prefix *hop)
{
   struct rsvp_ip_hop *rsvp_ip_hop;
 
   rsvp_ip_hop = XCALLOC (MTYPE_RSVP_IP_HOP, sizeof (struct rsvp_ip_hop));

   if (!rsvp_ip_hop)
      return NULL;

   rsvp_ip_hop->type = hop_type;
   memcpy (&rsvp_ip_hop->hop, hop, sizeof (struct rsvp_ip_hop));

   return rsvp_ip_hop;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_ip_path_create
 * Input    : name = Pointer to string for the path.
 * Output   : Returns the new created path if successful, else returns NULL.
 * Synopsis : This is a utility function to create a new object of type
 *            struct rsvp_ip_path.
 * Callers  : CLI handlers of this module in rsvp_vty.c
 *------------------------------------------------------------------------*/
struct rsvp_ip_path *
rsvp_ip_path_create (char *name)
{ 
   struct rsvp_ip_path *rsvp_ip_path;
   struct rsvp *rsvp = rm->rsvp;

   rsvp_ip_path = XCALLOC (MTYPE_RSVP_IP_PATH, sizeof (struct rsvp_ip_path));

   if (!rsvp_ip_path) 
      return NULL;

   strncpy (rsvp_ip_path->name , name, RSVP_IP_PATH_NAME_LEN);
   /* Create the list of rsvp_ip_hops. */
   rsvp_ip_path->ip_hop_list = list_new ();

   /* Insert the path into the global rsvp->ip_path_obj. */
   ternary_insert ((ternary_tree *)&rsvp->ip_path_obj, name, rsvp_ip_path, 0);     listnode_add ((struct list *)rsvp->ip_path_list, rsvp_ip_path);

   return rsvp_ip_path;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_ip_path_search
 * Input    : name = String (char *) that identifies the name of the path.
 * Output   : If the path matching the name is found the it returns the
 *            corresponding object of type struct rsvp_ip_path. If not found
 *            it returns NULL. 
 * Synopsis : This utility function parses through the 
 *            rsvp_ip_paths organized as ternary serach tries in the 
 *            rm-rsvp->ip_path_obj hook. 
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
struct rsvp_ip_path *
rsvp_ip_path_search (char *name)
{
   ternary_tree ip_path_tree;
   struct rsvp_ip_path *rsvp_ip_path;

   /* Get the ternary serach trie. */
   ip_path_tree = (ternary_tree)rm->rsvp->ip_path_obj;

   rsvp_ip_path = (struct rsvp_ip_path *)ternary_search (ip_path_tree, name);

   return rsvp_ip_path;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_ip_path_delete 
 * Input    : name = String that identifies the rsvp_ip_path.
 * Output   : Returns 0 if deletion sucessful, else returns 0.
 * Synopsis : This is utility function to delete a rsvp_ip_path from the
 *            ternary search trie that is hooked at rm->rsvp->ip_path_obj.
 * Callers  : no_ip_explicit_path,no_explicit_path in rsvp_vty.c
 *--------------------------------------------------------------------------*/ 
int 
rsvp_ip_path_delete (char *name)
{
   //struct rsvp_ip_path *rsvp_ip_path;
   //ternary_tree ip_path_tree = (ternary_tree)rm->rsvp->ip_path_obj;

   /* Hang on till I clarify the deletion process from the ternary serach trie
      . I am still not able to locate function. */
#if 0
   listnode_delete ((struct list *)rm->rsvp->ip_path_list, rsvp_ip_path);
#endif
   return 0;
}
 
/*---------------------------------------------------------------------------
 * Function : rsvp_ip_path_insert_hop
 * Input    : rsvp_ip_path = Pointer to object of type struct rsvp_ip_path.
 *          : hop_type = Type of the hop
 *          : hop = Pointer to object of type struct prefix.
 * Output   : Returns 0 if added successfully, else return -1.
 * Synopsis : This is a utility function to be called to insert a rsvp_ip_hop
 *            into a rsvp_ip_path.
 * Callers  : CLI Handlers defined in this module in rsvp_vty.c    
 *-------------------------------------------------------------------------*/
int
rsvp_ip_path_insert_hop (struct rsvp_ip_path *rsvp_ip_path,
                         int hop_type,
                         struct prefix *hop)
{
   struct rsvp_ip_hop *rsvp_ip_hop;

   /* Sanity Check. */
   if (!rsvp_ip_path || !hop)
      return -1;

   rsvp_ip_hop = rsvp_ip_hop_create (hop_type, hop);
   
   /* Sanity Check. */
   if(!rsvp_ip_hop)
      return -1;

   listnode_add (rsvp_ip_path->ip_hop_list, rsvp_ip_hop);
   rsvp_ip_path->num_hops++;
   return 0;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_ip_path_delete_hop
 * Input    : rsvp_ip_path = Pointer to object of type struct rsvp_ip_path.
 *          : hop_type = integer value of hop_type.
 *            hop = Pointer to object of type struct prefix.
 * Output   : Returns 0 if the hop is found and deleted, else returns -1.
 * Synopsis : This procedure deletes a hop from rsvp_ip_path object.
 * Callers  : CLI Handlers of this module in rsvp_vty.c
 *------------------------------------------------------------------------*/
int
rsvp_ip_path_delete_hop (struct rsvp_ip_path *rsvp_ip_path,
                         int hop_type,
                         struct prefix *hop)
{
   struct listnode *node;
   struct rsvp_ip_hop *rsvp_ip_hop;

   /* Sanity Check. */
   if (!rsvp_ip_path || !hop)
      return -1;

   /* Serach for the node that matches the prefix. */
   for (node = listhead (rsvp_ip_path->ip_hop_list); node; nextnode (node))
   {
      rsvp_ip_hop = (struct rsvp_ip_hop *)getdata (node);

      if ((rsvp_ip_hop->type == hop_type) && 
          (!memcmp (&rsvp_ip_hop->hop, hop, sizeof (struct prefix))))
      { 
         list_delete_node (rsvp_ip_path->ip_hop_list, node);
         return 0;
      }
   }
   return -1;  
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_avl_key_cmp
 * Input    : in_key = Pointer to input key
 *          : data_key = Pointer to existing key in data.
 *          : param = Optional.Not used here
 * Output   : Retuns 0 if matches, -1 if key < the node_key else returms +1.
 * Synopsis : This function is to be used internally by rsvp_tunnel_if_avl.
 *            This function is the comparison function for keys at each AVL
 *            node. The function is as per specified in GNU libavl.
 * Callers  : The libavl functions for this rsvp_tunnel_if_avl_table.
 *-------------------------------------------------------------------------*/ 
int
rsvp_tunnel_if_avl_key_cmp(const void *in_key,
                           const void *data,
                           void *param)
{
   /* RSVP Tunnel interfaces are kept with tunnel numbers as key.*/
   u_int32_t data_key = ((struct rsvp_tunnel_if *)data)->num;
   return (*(u_int32_t *)in_key - data_key);

}
 
/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_obj_init 
 * Input    : None
 * Output   : Void pointer to the rsvp_tunnel_if created as avl_tree.
 * Synopsis : This function creates the global AVL tree for holding all
 *            RSVP-TE Tunnel interfaces created and maintained by the system.
 *            The AVL tree is hooked into the rsvp->rsvp_tunnel_if object
 *            for global access. If failed then returns NULL.
 * Callers  : This function is called only once during initialization of 
 *            RSVP-TE daemon.
 *--------------------------------------------------------------------------*/
void *
rsvp_tunnel_if_obj_init (void)
{
   struct avl_table *rsvp_tunnel_if_avl_table;

   rsvp_tunnel_if_avl_table = avl_create (rsvp_tunnel_if_avl_key_cmp, NULL,
                                          &avl_allocator_default);
   
   /* If AVL Table is not allocated then crash. */
   if (!rsvp_tunnel_if_avl_table)
      assert (0);
   
   rsvp_tunnel_if_obj.rsvp_tunnel_if_avl_table = rsvp_tunnel_if_avl_table;
   /* Initialize the traverser. */
   avl_t_init (&rsvp_tunnel_if_obj.rsvp_tunnel_if_avl_table_traverser,
               rsvp_tunnel_if_avl_table);
   /* Return Pointer to the static data structure rsvp_tunnel_if_obj. */
   return &rsvp_tunnel_if_obj;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_create
 * Input    : Number of the Tunnel Interface.
 * Output   : Returns pointer to struct rsvp_tunnel_if_create if successful,
 *            else returns NULL.
 * Synopsis : This function creates a object of struct  rsvp_tunnel_if.
 *            Its a utility function.
 * Callers  : CLI Handler rsvp_tunnel_if in rsvp_vty.c
 *-------------------------------------------------------------------------*/  
struct rsvp_tunnel_if *
rsvp_tunnel_if_create (u_int32_t tunnel_if_num)
{
   struct rsvp_tunnel_if_obj *tunnel_if_obj;
   struct rsvp_tunnel_if *rsvp_tunnel_if;
   void *result;

   /* Allocated a tunnel interface object. */
   rsvp_tunnel_if = XCALLOC (MTYPE_RSVP_TUNNEL_IF, 
                             sizeof (struct rsvp_tunnel_if));
   if (rsvp_tunnel_if == NULL)
      return NULL;

   /* Assign the user defined tunnel interface number. */
   rsvp_tunnel_if->num = tunnel_if_num;
   rsvp_tunnel_if->num_paths = 0;
   /* By Default we always set Tunnel Interface as Admin UP. */
   RSVP_TUNNEL_IF_SET_ADMIN_UP (rsvp_tunnel_if);

   tunnel_if_obj = (struct rsvp_tunnel_if_obj *)rm->rsvp->tunnel_if_obj;        
   /* Add the rsvp_tunnel_if to rsvp->tunnel_if_obj AVL Tree. */
   result = 
       avl_insert ((struct avl_table *)tunnel_if_obj->rsvp_tunnel_if_avl_table, 
                   rsvp_tunnel_if);
   /* This function makes sure that the Tunnel is crated for first time.
      so we shouldn't see that we are replacing an existing tunnel.*/
   if (result)
      assert (0); 
   return rsvp_tunnel_if;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_search
 * Input    : tunnel_num = The number of the Tunnel.
 * Output   : Returns the pointer to the rsvp_tunnel_if object if found, else
 *            returns NULL.
 * Synopsis : This function searches a Tunnel interface from the Tunnel number
 *            given as input in the gloval AVL tree 
 *            rm->rsvp->rsvp_tunnel_if_objin which all tunnels are stored.
 * Callers  : rsvp_tunnel_if.. CLI Handlers in rsvp_vty.c
 *-------------------------------------------------------------------------*/
struct rsvp_tunnel_if *
rsvp_tunnel_if_search (u_int32_t tunnel_num)
{
   struct rsvp_tunnel_if_obj  *rsvp_tunnel_if_obj;
   struct rsvp_tunnel_if *rsvp_tunnel_if;
   /* Sanity Check. */
   assert (rm->rsvp);
   /* Get the AVL Tree hook. */
   rsvp_tunnel_if_obj = (struct rsvp_tunnel_if_obj *)rm->rsvp->tunnel_if_obj;
   /* Would return if not found. */
   rsvp_tunnel_if = 
   (struct rsvp_tunnel_if *)avl_find (
                                   rsvp_tunnel_if_obj->rsvp_tunnel_if_avl_table,                                   &tunnel_num);

  return rsvp_tunnel_if;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_traverse 
 * Input    : flag = if 1 means initialize to first element and then travserse. 
 * Output   : Returns the next rsvp_tunnel_if after each travserse.
 * Synopsis : This function is a wrapper on the avl traverse mechanism as
 *            in libavl. This function returns next element (rsvp_tunnel_if)
 *            from the traverser in rsvp_tunnel_if_obj.
 * Caller   : show_interface_tunnel in rsvp_vty.c
 *--------------------------------------------------------------------------*/
struct rsvp_tunnel_if *
rsvp_tunnel_if_traverse (int flag)
{
   struct rsvp_tunnel_if_obj *rsvp_tunnel_if_obj = 
                         (struct rsvp_tunnel_if_obj *)rm->rsvp->tunnel_if_obj;

   struct avl_traverser *traverser = 
                    &rsvp_tunnel_if_obj->rsvp_tunnel_if_avl_table_traverser;
   struct avl_table *table = rsvp_tunnel_if_obj->rsvp_tunnel_if_avl_table;

   /* If Flag is set initialize the traverser. */
   if (flag)
   {
      avl_t_init (traverser, table);
      /* Return the first element in avl_traverser. */
      return ((struct rsvp_tunnel_if *)avl_t_first (traverser, table));
   }   
   /* Return the next element. */
   return (avl_t_next (traverser));
}
 
/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_delete 
 * Input    : tunnel_num = Tunnel Number in Integer
 * Output   : Returns 0 if deleted successfully, else returns -1 if the tunnel
 *            number to be deleted is not found.
 * Synopsis : This function deletes the Tunnel  from the global AVL tree
 *            hooked at rm->rsvp->rsvp_tunnel_if_obj if the tunnel is found.
 * Callers  : The CLI handler no_rsvp_tunnel_if at rsvpd.c
 *-------------------------------------------------------------------------*/
int
rsvp_tunnel_if_delete (u_int32_t tunnel_num)
{
   struct rsvp_tunnel_if_obj  *rsvp_tunnel_if_obj;
   struct avl_table *rsvp_tunnel_if_avl_table;
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Sanity Check. */
   assert (rm->rsvp);
   rsvp_tunnel_if_obj = (struct rsvp_tunnel_if_obj *)rm->rsvp->tunnel_if_obj;
   rsvp_tunnel_if_avl_table = rsvp_tunnel_if_obj->rsvp_tunnel_if_avl_table;
   rsvp_tunnel_if = avl_delete (rsvp_tunnel_if_avl_table, &tunnel_num);

   /* Means the Tunnel Found and Deleted. */
   if (rsvp_tunnel_if)
   {
      /* Do all clean up for the rsvp tunnel. */
      // TBD;
      XFREE (MTYPE_RSVP_TUNNEL_IF, rsvp_tunnel_if);
      return 0;
   }
   else
      return -1;
}            

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_is_config_complete
 * Input    : rsvp_tunnel_if = Pointer to the struct rsvp_tunnel_if.
 * Output   : Returns 1 if complete else return 0 (It's true/false).
 * Synopsis : This function checks if all mandatory config. parameters are
 *            present to proceed for setting up LSP for this Tunnel.
 * Callers  :
 *-------------------------------------------------------------------------*/
int
rsvp_tunnel_if_is_config_complete (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return 0;

   /* We reach here if the config complete flag is not set, but can be set
      as the mandatory parameters are filled in recently. */
   if (RSVP_TUNNEL_IF_IS_RSVP_SET (rsvp_tunnel_if))
      return 0;

   if (RSVP_TUNNEL_IF_IS_DST_SET(rsvp_tunnel_if))
      return 0;

   if (rsvp_tunnel_if->num_paths == 0)
      return 0;

   return 1;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_set_rsvp
 * Input    : rsvp_tunnel_if = Pointer to object of struct rsvp_tunnel_if
 * Output   : None
 * Synopsis ; This is a utility function that sets a rsvp_tunnel_if for RSVP.
 *            As such there was no need of this function, but done to meet the
 *            same paradigm to manage parameters on a rsvp_tunnel_if. Anyway
 *            this is called from CLI, no high performance is required.
 * Callers  : tunnel_mode_mpls_traffoc_eng CLI handler in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_set_rsvp (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;
   /* Enable RSVP on this interface. */
   RSVP_TUNNEL_IF_SET_RSVP (rsvp_tunnel_if);

   /* Check if the rsvp_tunnel_if config was not complete earlier and now
      setting this parameter makes it complete. If yes, then set then
       rsvp_tunnel_if config as complete and start the tunnel. */
   if ((!RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      /* Set the config flag as complete. */
      RSVP_TUNNEL_IF_SET_COMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_start (rsvp_tunnel_if);
   }
   return;
}

/*----------------------------------------------------------------------------
 * Function ; rsvp_tunnel_if_unset_rsvp
 * Input    : rsvp_tunnel_if = Pointer to the object of type 
 *          : struct rsvp_tunnel_if.
 * Output   : None
 * Synopsis : This is a utility function provided to CLI (vty) that resets
 *            RSVP on this Tunnel Interface.
 * Callers  : no_tunnel_mode_mpls_traffic_eng CLI Handler in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_unset_rsvp (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;
   /* Disable RSVP on this interface. */
   RSVP_TUNNEL_IF_UNSET_RSVP (rsvp_tunnel_if);
   /* Check if earlier the config was complete on this rsvp_tunnel_if.If yes,
      then further check if unsetting this parameters makes the config 
      incomplete. If yes, then stop the tunnel. */
   if ((RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (!rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      RSVP_TUNNEL_IF_SET_INCOMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_stop (rsvp_tunnel_if);
   }
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_set_cspf
 * Input    : rsvp_tunnel_if = Pointer to object of type  struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This utility function enables CSPF on this rsvp_tunnel_if and
 *            takes the necessary actions required.
 * Callers  : tunnel_mode_mpls_traffic_eng_cspf CLI handler in rsvp_vty.c
 *-------------------------------------------------------------------------*/
void
rsvp_tunnel_if_set_cspf (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity check. */
   if (!rsvp_tunnel_if)
      return;
  
   /* Set CSPF on this interface. */
   RSVP_TUNNEL_IF_SET_CSPF (rsvp_tunnel_if);

   /* Check if the rsvp_tunnel_if config was not complete earlier and now
      setting this parameter makes it complete. If yes, then set then
      rsvp_tunnel_if config as complete and start the tunnel. */
   if ((!RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      /* Set the config flag as complete. */
      RSVP_TUNNEL_IF_SET_COMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_start (rsvp_tunnel_if);
   }
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_unset_cspf
 * Input    : rsvp_tunnel_if = Pointer to object of type rsvp_tunnel_if
 * Output   : None
 * Synopsis : This utility function disables CSPF for this tunnel interface
 *            and takes the necessary action.
 * Callers  : no_tunnel_mode_traffic_eng_cspf in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_unset_cspf (struct rsvp_tunnel_if *rsvp_tunnel_if)
{ 
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
     return;

   RSVP_TUNNEL_IF_UNSET_CSPF (rsvp_tunnel_if);
   /* Check if earlier the config was complete on this rsvp_tunnel_if.If yes,
      then further check if unsetting this parameters makes the config 
      incomplete. If yes, then stop the tunnel. */
   if ((RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (!rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      RSVP_TUNNEL_IF_SET_INCOMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_stop (rsvp_tunnel_if);
   }
   return;
}
/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_set_dst 
 * Input    : rsvp_tunnel_if = Pointer to struct rsvp_tunnel_if
 *            dst = Pointer to prefix identifying destination.
 * Output   : None.
 * Synopsis : This function sets the destination for a Tunnel.
 * Callers  : CLI Handlers of this module in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_set_dst (struct rsvp_tunnel_if *rsvp_tunnel_if,
                        struct prefix *dst)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if || !dst)
      return;
   /* Make the holder sane. */
   bzero (&rsvp_tunnel_if->destination, sizeof (struct prefix));

   /* Get the destination address into rsvp_tunnel_if from input. */
   memcpy (&rsvp_tunnel_if->destination, dst, sizeof (struct prefix));
   RSVP_TUNNEL_IF_SET_DST (rsvp_tunnel_if);
   /* Check if config is complete for this Tunnel. If No return else start 
      the Tunnel. */
   if((!RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
      (rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      RSVP_TUNNEL_IF_SET_COMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_start (rsvp_tunnel_if);
   } 
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_unset_dst 
 * Input    : rsvp_tunnel_if = Pointer to struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This function unsets the destination parameter for this
 *            rsvp_tunnel_if.
 * Callers  : CLI Handlers defined in this module in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void 
rsvp_tunnel_if_unset_dst (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;
   /* Make the destination holder contain 0. */ 
   bzero (&rsvp_tunnel_if->destination, sizeof (struct prefix));
   RSVP_TUNNEL_IF_UNSET_DST (rsvp_tunnel_if);

   /* Check if the tunnel config was complete earlier and if  unsetting this 
      parameter makes the config incomplete. If yes, then stop the tunnel and
      set the config on the tunnel as incomplete. */
   if ((RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (!rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      /* Stop the Tunnel. */
      rsvp_tunnel_if_stop (rsvp_tunnel_if);
      RSVP_TUNNEL_IF_SET_INCOMPLETE (rsvp_tunnel_if);
   }

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_set_path_option 
 * Input    : rsvp_tunnel_if    = Pointer to the struct rsvp_tunnel_if
 *            path_option_num   = The Path Option Number to be set
 *            path_option_type  = Type of the path option.
 *            standby_flag	= If Path needs to be standby
 *            path              = Pointer to struct rsvp_ip_path
 * Output   : None
 * Synopsis : This function sets a path_option to the rsvp_tunnel_if.
 * Callers  : CLI handler tunnel_mpls_traffic_eng_path_option in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_set_path_option (struct rsvp_tunnel_if *rsvp_tunnel_if,
                                int path_option_num,
                                int path_option_type,
                                int standby_flag,
                                struct rsvp_ip_path *path)
{
   struct rsvp_tunnel_if_path_option *path_option;
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;
 
   if (!path)
   {
      printf ("[rsvp_tunnel_if_set_path_option] path is NULL\n"); 
      return;
   }
   /* Get the path option pointer from the path_option_num. */ 
   path_option = rsvp_tunnel_if->path_option + path_option_num - 1;
   path_option->type = path_option_type;
   /* Increment the reference count of the path if its explicit. */
   if (path_option->type == RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT)
      path_option->path->ref_cnt++;
   /* If the path option is given as standby then set accordingly. */
   path_option->standby_flag = standby_flag;
   /* Set this path option as valid. */
   path_option->valid_flag = 1;
   /* Increment number of paths in Tunnel */
   rsvp_tunnel_if->num_paths++;

   /* Check if config is complete for this Tunnel. If No return else start 
      the Tunnel. */
   if((!RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
      (rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      RSVP_TUNNEL_IF_SET_COMPLETE (rsvp_tunnel_if);
      rsvp_tunnel_if_start (rsvp_tunnel_if);
   } 

   return; 
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_unset_path_option
 * Input    : rsvp_tunnel_if 	= Pointer to struct rsvp_tunnel_if
 *            path_opion_num    = The path number that is to be removed.
 * Output   : None
 * Synopsis : This functions unsets the path number specified from 
 *            rsvp_tunnel_if.
 * Callers  : no_mpls_traffic_eng_path_option in rsvp_vty.c
 *-------------------------------------------------------------------------*/
void
rsvp_tunnel_if_unset_path_option (struct rsvp_tunnel_if *rsvp_tunnel_if,
                                  int path_option_num)
{
   struct rsvp_tunnel_if_path_option *path_option;
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;

   /* Get the Pointer to the path option from the number. */
   path_option = rsvp_tunnel_if->path_option + path_option_num - 1 ;

   /* Check if the path option is having a LSP set up already. */
   if (path_option->rsvp_lsp_handle)
   {
      /* Stop the LSP First..Don't delete it and set the ingress_user_handle
         on the LSP as NULL. Deletion will be taken care automatically when the
         resv teardown is received and ingress_user_handle is found NULL.*/
      rsvp_lsp_stop (path_option->rsvp_lsp_handle);
      ((struct rsvp_lsp *)path_option->rsvp_lsp_handle)->ingress_user_handle =
      NULL;

      /* Further if this option is being used as the active path then we need
         to set alternate paths as active. */
      if (rsvp_tunnel_if->active_path == path_option_num)
         rsvp_tunnel_if_path_option_select_active (rsvp_tunnel_if);
   }
   
   /* Decrement the reference count if it is an explicit path. */
   if (path_option->type == RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT)
      path_option->path->ref_cnt--;

   /* Reset all path option. */
   bzero (path_option, sizeof (struct rsvp_tunnel_if_path_option));
   /* Set as invalid. */
   path_option->valid_flag = 0;
   /* Decrement the number of paths in Tunnel by one. */   
   rsvp_tunnel_if->num_paths--;

   /* Check if the tunnel config was complete earlier and if  unsetting this 
      parameter makes the config incomplete. If yes, then stop the tunnel and
      set the config on the tunnel as incomplete. */
   if ((RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if)) &&
       (!rsvp_tunnel_if_is_config_complete (rsvp_tunnel_if)))
   {
      /* Stop the Tunnel. */
      rsvp_tunnel_if_stop (rsvp_tunnel_if);
      RSVP_TUNNEL_IF_SET_INCOMPLETE (rsvp_tunnel_if);
   }

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_path_option_is_valid
 * Input    : rsvp_tunnel_if = Pointer to object of type struct rsvp_tunnel_if.
 *          : path_option_num = The path option number
 * Output   : Returns 1 if rsvp_tunnel_if contains valid path_option with the
 *            number given as input.
 * Synopsis : This is a utility function that returns the validity of a path
 *            option number on a tunnel interface.
 * Callers  :
 *--------------------------------------------------------------------------*/
int
rsvp_tunnel_if_path_option_is_valid (struct rsvp_tunnel_if *rsvp_tunnel_if,
                                     int path_option_num)
{
   /* Sanity Check. */
   if ((!rsvp_tunnel_if) || (path_option_num > 6))
      return 0;
  
   return  (rsvp_tunnel_if->path_option[path_option_num - 1].valid_flag);
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_path_option_select_active
 * Input    ; rsvp_tunnel_if = Pointer to object of type struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This procedure walks through the path-options in a rsvp_tunnel_if
 *            one by one and selects the active one.
 * Callers  : rsvp_tunnel_if_unset_path_option in rsvpd.c
 *---------------------------------------------------------------------------*/
void
rsvp_tunnel_if_path_option_select_active (struct rsvp_tunnel_if *rsvp_tunnel_if){
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;

   
   return;
} 
/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_admin_up
 * Input    : rsvp_tunnel_if = Pointer to the object of type 
 *            struct rsvp_tunnel_if.
 * Output   : None
 * Synopsis : This utility function sets the Interface Tunnel administratively
 *            up.
 * Callers  : no_interface_tunnel_shutdown CLI handler in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_admin_up (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;
 
   /* Not up so we need to make UP.*/
   RSVP_TUNNEL_IF_SET_ADMIN_UP (rsvp_tunnel_if);
   
   /* Check if the config is complete.If yes, then start the Tunnel. */
   if (RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if))
      rsvp_tunnel_if_start (rsvp_tunnel_if);
    
   return; 
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_admin_down 
 * Input    : rsvp_tunnel_if = Pointer to object of type struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This utility function sets Interface Tunnel administratively
 *            down.
 * Callers  : interface_tunnel_shutdown CLI handler defined in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_admin_down (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;

   /* Set the Interface Tunnel as administratively down. */
   RSVP_TUNNEL_IF_SET_ADMIN_DOWN (rsvp_tunnel_if);

   /* Check if the config on this Interface Tunnel is complete. If yes then
      it might be either ACTIBE or may be trying to become active. In both
      the cases stop the Tunnel completely. */
   if (RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if))
      rsvp_tunnel_if_stop (rsvp_tunnel_if);

   return; 
} 
/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_start
 * Input    : rsvp_tunnel_if = Pointer to struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This function starts signaling for a RSVP Tunnel Interface.
 * Callers  : CLI Handlers of this module in rsvp_vty.c.
 *-------------------------------------------------------------------------*/
void
rsvp_tunnel_if_start (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;

   /* If it is already ready the  do nothing. */
   if (RSVP_TUNNEL_IF_IS_ADMIN_UP (rsvp_tunnel_if) && 
       RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if))
      /* Do all necessary to start the tunnel.(TBD) */;
   /* Do all procedures for starting the Tunnel. */

   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_stop
 * Input    : rsvp_tunnel_if = Pointer to struct rsvp_tunnel_if
 * Output   : None
 * Synopsis : This function tears down an existing RSVP Tunnel Interface.
 * Callers  : CLI Handlers of this module in rsvp_vty.c.
 *-------------------------------------------------------------------------*/
void
rsvp_tunnel_if_stop (struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!rsvp_tunnel_if)
      return;

   /* Stop if the Tunnel is already ready for signaling or active. */
   if (RSVP_TUNNEL_IF_IS_ADMIN_UP (rsvp_tunnel_if) &&
       RSVP_TUNNEL_IF_IS_COMPLETE (rsvp_tunnel_if))
   {
      /* Do all procedures for stopping the Tunnel. */
      ;
   } 

   /* Else Returns and do nothing. */
   else
      return;
}
 
