/* Configuration generator.
   Copyright (C) 2000 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include <zebra.h>

#include "command.h"
#include "linklist.h"
#include "memory.h"
#include <lib/version.h>

#ifdef IMISH_IMI_MODULE

#include "imi-cli/imi-sh.h"

static vector imish_configvec = NULL;


struct imish_config
{
  /* Configuration node name. */
  char *name;

  /* Configuration string line. */
  struct list *line;

  /* Configuration can be nest. */
  struct imish_config *config;

  /* Index of this config. */
  u_int32_t index;
};

static struct list *imish_config_top = NULL;

static int
line_cmp (char *c1, char *c2)
{
  return strcmp (c1, c2);
}

static void
line_del (char *line)
{
  XFREE (MTYPE_VTYSH_CONFIG_LINE, line);
}

static struct imish_config *
config_new ()
{
  struct imish_config *config;
  config = XCALLOC (MTYPE_VTYSH_CONFIG, sizeof (struct imish_config));
  return config;
}

static int
config_cmp (struct imish_config *c1, struct imish_config *c2)
{
  return strcmp (c1->name, c2->name);
}

static void
config_del (struct imish_config* config)
{
  list_delete (config->line);
  if (config->name)
    XFREE (MTYPE_VTYSH_CONFIG_LINE, config->name);
  XFREE (MTYPE_VTYSH_CONFIG, config);
}

static struct imish_config *
config_get (int index, const char *line)
{
  struct imish_config *config;
  struct imish_config *config_loop;
  struct list *mlist;
  struct listnode *node, *nnode;

  config = config_loop = NULL;

  mlist = vector_lookup_ensure (imish_configvec, index);

  if (! mlist)
    {
      mlist = list_new ();
      mlist->del = (void (*) (void *))config_del;
      mlist->cmp = (int (*)(void *, void *)) config_cmp;
      vector_set_index (imish_configvec, index, mlist);
    }
  
  for (ALL_LIST_ELEMENTS (mlist, node, nnode, config_loop))
    {
      if (strcmp (config_loop->name, line) == 0)
	config = config_loop;
    }

  if (! config)
    {
      config = config_new ();
      config->line = list_new ();
      config->line->del = (void (*) (void *))line_del;
      config->line->cmp = (int (*)(void *, void *)) line_cmp;
      config->name = XSTRDUP (MTYPE_VTYSH_CONFIG_LINE, line);
      config->index = index;
      listnode_add (mlist, config);
    }
  return config;
}

static void
config_add_line (struct list *config, const char *line)
{
  listnode_add (config, XSTRDUP (MTYPE_VTYSH_CONFIG_LINE, line));
}

static void
config_add_line_uniq (struct list *config, const char *line)
{
  struct listnode *node, *nnode;
  char *pnt;

  for (ALL_LIST_ELEMENTS (config, node, nnode, pnt))
    {
      if (strcmp (pnt, line) == 0)
	return;
    }
  listnode_add_sort (config, XSTRDUP (MTYPE_VTYSH_CONFIG_LINE, line));
}

static void imish_config_parse_line (const char *line)
{
  char c;
  static struct imish_config *config = NULL;

  if (! line)
    return;

  c = line[0];

  if (c == '\0')
    return;

  /* printf ("[%s]\n", line); */

  switch (c)
    {
    case '!':
    case '#':
      break;
    case ' ':
      /* Store line to current configuration. */
      if (config)
	{
	  if (strncmp (line, " address-family vpnv4",
	      strlen (" address-family vpnv4")) == 0)
	    config = config_get (BGP_VPNV4_NODE, line);
/*	  else if (strncmp (line, " address-family vpn6",
	      strlen (" address-family vpn6")) == 0)
	    config = config_get (BGP_VPNV6_NODE, line);
	  else if (strncmp (line, " address-family encapv6",
	      strlen (" address-family encapv6")) == 0)
	    config = config_get (BGP_ENCAPV6_NODE, line);
	  else if (strncmp (line, " address-family encap",
	      strlen (" address-family encap")) == 0)
	    config = config_get (BGP_ENCAP_NODE, line);*/
	  else if (strncmp (line, " address-family ipv4 multicast",
		   strlen (" address-family ipv4 multicast")) == 0)
	    config = config_get (BGP_IPV4M_NODE, line);
	  else if (strncmp (line, " address-family ipv6",
		   strlen (" address-family ipv6")) == 0)
	    config = config_get (BGP_IPV6_NODE, line);
	  else if (config->index == RMAP_NODE ||
	           config->index == INTERFACE_NODE ||
		   config->index == VTY_NODE)
	    config_add_line_uniq (config->line, line);
	  else
	    config_add_line (config->line, line);
	}
      else
	config_add_line (imish_config_top, line);
      break;
    default:
      if (strncmp (line, "interface", strlen ("interface")) == 0)
	config = config_get (INTERFACE_NODE, line);
      else if (strncmp (line, "router-id", strlen ("router-id")) == 0)
	config = config_get (ZEBRA_NODE, line);
      else if (strncmp (line, "router rip", strlen ("router rip")) == 0)
	config = config_get (RIP_NODE, line);
      else if (strncmp (line, "router ripng", strlen ("router ripng")) == 0)
	config = config_get (RIPNG_NODE, line);
      else if (strncmp (line, "router ospf", strlen ("router ospf")) == 0)
	config = config_get (OSPF_NODE, line);
      else if (strncmp (line, "router ospf6", strlen ("router ospf6")) == 0)
	config = config_get (OSPF6_NODE, line);
      else if (strncmp (line, "router bgp", strlen ("router bgp")) == 0)
	config = config_get (BGP_NODE, line);
      else if (strncmp (line, "router isis", strlen ("router isis")) == 0)
  	config = config_get (ISIS_NODE, line);
      else if (strncmp (line, "router bgp", strlen ("router bgp")) == 0)
	config = config_get (BGP_NODE, line);
      else if (strncmp (line, "route-map", strlen ("route-map")) == 0)
	config = config_get (RMAP_NODE, line);
      else if (strncmp (line, "access-list", strlen ("access-list")) == 0)
	config = config_get (ACCESS_NODE, line);
      else if (strncmp (line, "ipv6 access-list",
	       strlen ("ipv6 access-list")) == 0)
	config = config_get (ACCESS_IPV6_NODE, line);
      else if (strncmp (line, "ip prefix-list",
	       strlen ("ip prefix-list")) == 0)
	config = config_get (PREFIX_NODE, line);
      else if (strncmp (line, "ipv6 prefix-list",
	       strlen ("ipv6 prefix-list")) == 0)
	config = config_get (PREFIX_IPV6_NODE, line);
      else if (strncmp (line, "ip as-path access-list",
	       strlen ("ip as-path access-list")) == 0)
	config = config_get (AS_LIST_NODE, line);
      else if (strncmp (line, "ip community-list",
	       strlen ("ip community-list")) == 0)
	config = config_get (COMMUNITY_LIST_NODE, line);
      else if (strncmp (line, "ip route", strlen ("ip route")) == 0)
	config = config_get (IP_NODE, line);
      else if (strncmp (line, "ipv6 route", strlen ("ipv6 route")) == 0)
   	config = config_get (IP_NODE, line);
      else if (strncmp (line, "key", strlen ("key")) == 0)
	config = config_get (KEYCHAIN_NODE, line);
      else if (strncmp (line, "line", strlen ("line")) == 0)
	config = config_get (VTY_NODE, line);
      else if ( (strncmp (line, "ipv6 forwarding",
		 strlen ("ipv6 forwarding")) == 0)
	       || (strncmp (line, "ip forwarding",
		   strlen ("ip forwarding")) == 0) )
	config = config_get (FORWARDING_NODE, line);
      else if (strncmp (line, "service", strlen ("service")) == 0)
	config = config_get (SERVICE_NODE, line);
      else if (strncmp (line, "debug", strlen ("debug")) == 0)
	config = config_get (DEBUG_NODE, line);
	
/* 2016年7月2日 22:14:16 zhurish: 扩展路由协议增加客户端路由信息 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
      else if (strncmp (line, "router olsr", strlen ("router olsr")) == 0)
  	config = config_get (OLSR_NODE, line);
      else if (strncmp (line, "router hsls", strlen ("router hsls")) == 0)
	config = config_get (HSLS_NODE, line);
      else if (strncmp (line, "router frp", strlen ("router frp")) == 0)
	config = config_get (FRP_NODE, line);
      else if (strncmp (line, "router icrp", strlen ("router icrp")) == 0)
	config = config_get (ICRP_NODE, line);
#endif /* HAVE_EXPAND_ROUTE_PLATFORM */
/* 2016年7月2日 22:14:16  zhurish: 扩展路由协议增加客户端路由信息 */

#ifndef IMISH_IMI_MODULE
    else if (strncmp (line, "password", strlen ("password")) == 0
	       || strncmp (line, "enable password",
			   strlen ("enable password")) == 0)
	    config = config_get (AAA_NODE, line);
#else /* IMISH_IMI_MODULE */
/*
      else if (strncmp (line, "vrf", strlen ("vrf")) == 0)
	    config = config_get (VRF_NODE, line);
*/
#endif/* IMISH_IMI_MODULE */
      else if (strncmp (line, "ip protocol", strlen ("ip protocol")) == 0)
	config = config_get (PROTOCOL_NODE, line);
      else
	{
#ifdef  IMISH_IMI_MODULE 	
	  if ( strncmp (line, "hostname", strlen ("hostname")) == 0 ||
	       strncmp (line, "password", strlen ("password")) == 0 || 
	       strncmp (line, "enable password", strlen ("enable password")) == 0 ||
	       strncmp (line, "log", strlen ("log")) == 0 ||
	       strncmp (line, "no log", strlen ("no log")) == 0  )
#else/* IMISH_IMI_MODULE */
	  if (strncmp (line, "log", strlen ("log")) == 0 ||
	       strncmp (line, "hostname", strlen ("hostname")) == 0
	     )
#endif/* IMISH_IMI_MODULE */
	    config_add_line_uniq (imish_config_top, line);
	  else
	    config_add_line (imish_config_top, line);
	  config = NULL;
	}
      break;
    }
}

void imish_module_config_parse (char *line)
{
  char *begin;
  char *pnt;
  
  begin = pnt = line;

  while (*pnt != '\0')
    {
      if (*pnt == '\n')
	{
	  *pnt++ = '\0';
	  imish_config_parse_line (begin);
	  begin = pnt;
	}
      else
	{
	  pnt++;
	}
    }
}
void imish_module_config_format (const char *format, ...)
{
  int len = 0;
  char buf[1024];	
	va_list args;
  va_start (args, format);
  len = vsnprintf (buf, sizeof(buf), format, args);
  va_end (args);	  
	imish_module_config_parse (buf);
}
/* Macro to check delimiter is needed between each configuration line
 * or not. */
#define NO_DELIMITER(I)  \
  ((I) == ACCESS_NODE || (I) == PREFIX_NODE || (I) == IP_NODE \
   || (I) == AS_LIST_NODE || (I) == COMMUNITY_LIST_NODE || \
   (I) == ACCESS_IPV6_NODE || (I) == PREFIX_IPV6_NODE \
   || (I) == SERVICE_NODE || (I) == FORWARDING_NODE || (I) == DEBUG_NODE \
   || (I) == AAA_NODE)

/* Display configuration to file pointer. */
void imish_module_config_show (struct vty *vty)
{
  struct listnode *node, *nnode;
  struct listnode *mnode, *mnnode;
  struct imish_config *config;
  struct list *mlist;
  char *line;
  unsigned int i;

  for (ALL_LIST_ELEMENTS (imish_config_top, node, nnode, line))
    {
      vty_out (vty, "%s%s", line,VTY_NEWLINE);
    }
  vty_out (vty, "!%s",VTY_NEWLINE);

  for (i = 0; i < vector_active (imish_configvec); i++)
    if ((mlist = vector_slot (imish_configvec, i)) != NULL)
      {
	for (ALL_LIST_ELEMENTS (mlist, node, nnode, config))
	  {
	    vty_out (vty, "%s%s", config->name,VTY_NEWLINE);
	    for (ALL_LIST_ELEMENTS (config->line, mnode, mnnode, line))
	      {
		vty_out (vty, "%s%s", line,VTY_NEWLINE);
	      }
	    if (! NO_DELIMITER (i))
	      {
		vty_out (vty, "!%s",VTY_NEWLINE);
	      }
	  }
	if (NO_DELIMITER (i))
	  {
	    vty_out (vty, "!%s",VTY_NEWLINE);
	  }
      }

  for (i = 0; i < vector_active (imish_configvec); i++)
    if ((mlist = vector_slot (imish_configvec, i)) != NULL)
      {
	list_delete (mlist);
	vector_slot (imish_configvec, i) = NULL;
      }
  list_delete_all_node (imish_config_top);
}


/* Command execution over the imi interface. */
static int vtysh_config_from_file (struct vty *vty, FILE *fp)
{
  int ret = 0;
  //struct cmd_element *cmd;

  while (fgets (vty->buf, VTY_BUFSIZ, fp))
    {
      //ret |= imish_module_execute_file (vty, vty->buf);
      if( vty->buf[0]!='#' && vty->buf[0]!='!')
      ret |= imish_module_execute (vty, vty->buf, 2);
    }
  return ret;
}

/* Read up configuration file from file_name. */
void imish_execute_file (FILE *confp)
{
  int ret;
  struct vty *vty;

  vty = vty_new ();
  //vty->fd = 0;			/* stdout */
	vty->fd = STDOUT_FILENO;
  vty->fd = STDIN_FILENO;  
  vty->type = VTY_FILE;//VTY_TERM;
  vty->node = CONFIG_NODE;//CONFIG_NODE;VIEW_NODE
#if 1 
  vty->node = VIEW_NODE;//CONFIG_NODE;VIEW_NODE
  imish_module_execute (vty, "enable",2);
  imish_module_execute (vty, "configure terminal",2);
  /* Execute configuration file. */
  ret = vtysh_config_from_file (vty, confp);

  imish_module_execute (vty, "end",2);
  imish_module_execute (vty, "disable",2);
#else
  vty->node = CONFIG_NODE;//CONFIG_NODE;VIEW_NODE
  imish_module_client_execute(IMISH_ALL, "enable", vty, 0);
  imish_module_client_execute(IMISH_ALL, "configure terminal", vty, 0);

  /* Execute configuration file. */
  ret = vtysh_config_from_file (vty, confp);
  imish_module_client_execute(IMISH_ALL, "end", vty, 0);
  vty->node = ENABLE_NODE;
  imish_module_client_execute(IMISH_ALL, "disable", vty, 0);
#endif
  //vty_close (vty);

  if (ret != CMD_SUCCESS) 
    {
      switch (ret)
			{
				case CMD_ERR_AMBIGUOUS:
	  			fprintf (stderr, "Ambiguous command.\n");
	  			break;
				case CMD_ERR_NO_MATCH:
	  			fprintf (stderr, "There is no such command.\n");
	  			break;
			}
      fprintf (stderr, "Error occured during reading below line(ret:%d).\n%s\n", ret,vty->buf);
      //exit (1);
    }
    vty_close (vty);
}

/* Read up configuration file from config_default_dir. */
int imish_read_config (char *config_file)
{
  FILE *confp = NULL;
  confp = fopen (config_file, "r");
  if (confp == NULL)
  {
  	char imish_config_default[] = SYSCONFDIR IMISH_DEFAULT_CONFIG;
	  if (host.name)//
	    XFREE (MTYPE_HOST, host.name);
	  host.name = XSTRDUP (MTYPE_HOST, OEM_PACKAGE_NAME);
	  
	  if (host.enable)//设置enable的默认密码
    		XFREE (MTYPE_HOST, host.enable);
  	host.enable = XSTRDUP(MTYPE_HOST, IMISH_PASSWORD_DEFAULT);

  	if (host.enable_encrypt)
    		XFREE (MTYPE_HOST, host.enable_encrypt);
  	host.enable_encrypt = XSTRDUP(MTYPE_HOST, IMISH_PASSWORD_DEFAULT);
	  
	  if (host.password)//设置configure terminal默认密码
    		XFREE (MTYPE_HOST, host.password);
  	  host.password = XSTRDUP(MTYPE_HOST, IMISH_PASSWORD_DEFAULT);
  	if (host.password_encrypt)
    		XFREE (MTYPE_HOST, host.password_encrypt);
  	host.password_encrypt = XSTRDUP(MTYPE_HOST, IMISH_PASSWORD_DEFAULT);  
  	host_config_set (imish_config_default);	
    return (1);
  }
  imish_execute_file (confp);
  fclose (confp);
  host_config_set (config_file);
  return (0);
}

void imish_module_config_init ()
{
  imish_config_top = list_new ();
  imish_config_top->del = (void (*) (void *))line_del;
  imish_configvec = vector_init (1);
}
void imish_module_config_exit ()
{
	int i = 0;
	struct list *mlist;
  for (i = 0; i < vector_active (imish_configvec); i++)
    if ((mlist = vector_slot (imish_configvec, i)) != NULL)
      {
				list_delete (mlist);
				vector_slot (imish_configvec, i) = NULL;
      }
  list_delete_all_node (imish_config_top);
 	vector_free (imish_configvec);
}
#endif /* IMISH_IMI_MODULE */
