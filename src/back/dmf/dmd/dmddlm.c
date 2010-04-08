/*
** Copyright (c) 2005, 2008 Ingres Corporation
*/
#include <compat.h>
#include <pc.h>
#include <cs.h>
#include <nm.h>
#include <me.h>
#include <st.h>
#include <tr.h>
#include <cx.h>
#include <lk.h>

/*
** Name: dmddlm.c - Debug print function for cluster DLM locks
**
** Description:
**	This is used solely by cluster installations to print information
**	about Distributed Lock Manager locks, obtained directly from the DLM.
**
**	Resources are displayed in key order, followed by the locks held on the
**	resource.  Granted locks are listed 1st, then converters, then
**	finally waiters.
**
** History:
**	??-aug-2005 (devjo01)
**	    Original version (as standalone idlmdump.c).
**	21-feb-2008 (joea)
**	    Created.
**	02-sep-2008 (joea)
**	    Fix handling of installation ID.
*/

#if defined(conf_CLUSTER_BUILD)

typedef struct
{
    u_i4  node_number;
    i4    lock_count;
    char  node_name[16];
} NODEINFO;

static i4 node_count;

static NODEINFO	nodes[CX_MAX_NODES];

static char *system_level_inst_id = "AA";

void
node_format(u_i4 node_number, char **buf)
{
    int i;
    bool found;

    found = FALSE;
    for (i = 0; i < node_count; ++i)
    {
	if (nodes[i].node_number == node_number)
	{
	    *buf = nodes[i].node_name;
	    ++nodes[i].lock_count;
	    found = TRUE;
	    break;
	}
    }
    if (!found)
    {
	nodes[node_count].node_number = node_number;
	++nodes[node_count].lock_count;
	CXnode_name_from_id(node_number, nodes[node_count].node_name);
	*buf = nodes[node_count].node_name;
	++node_count;
    }
}

void
dmd_dlm_lock_info(
    bool waiters_only)
{
    CX_LOCK_LIST *llist, *pll;
    char *install_id;
    char inst_id[3];
    i4 count;
    NODEINFO *node;

    NMgtAt("II_INSTALLATION", &install_id);
    if (install_id && *install_id)
	STlcopy(install_id, inst_id, 2);
    else
	inst_id[0] = '\0';

    TRdisplay("\n---------------%@ DLM Locks");
    if (*inst_id)
	TRdisplay(" taken by Ingres instance %s", inst_id);
    else
	STcopy(system_level_inst_id, inst_id);
    TRdisplay(" ---------------\n\n");

    if (CXdlm_lock_info(inst_id, waiters_only, &llist) != OK)
	return;

    TRdisplay("Locks by Resource");
    if (waiters_only)
	TRdisplay("s having blocked lock(s)");
    TRdisplay("\n  RqstNode          PID      Lock ID    Q GR RQ (MastNode:MastID)\n");
    for (pll = llist; pll; pll = pll->cxll_next)
    {
	char buffer[128];
	LK_LOCK_KEY key;
	CX_LOCK_INFO *pli;

	/* Display using Ingres format */
	MEcopy(&pll->cxll_resource, sizeof(LK_LOCK_KEY), &key);
	key.lk_type &= 0x0FFFF;
	LKkey_to_string(&key, buffer);
	TRdisplay("%s\n", buffer);

	/* Display locks */
	for (count = pll->cxll_lock_count, pli = pll->cxll_locks; count > 0;
	     --count, ++pli)
	{
	    char *buf1, *buf2;

	    node_format(pli->cxlk_csid, &buf1);
	    node_format(pli->cxlk_mast_csid, &buf2);
	    TRdisplay("  %15s %08x%11x - %1s %2s %2s (%s:%x)\n",
		      /* Identity of lock as NODE PID LOCKID */
		      buf1, pli->cxlk_pid, pli->cxlk_lkid,
		      /* Status of lock as QUEUE, GRANT mode, REQUEST mode */
		      pli->cxlk_queue, pli->cxlk_grant_mode, pli->cxlk_req_mode,
		      /* Info as to which node is mastering lock */
		      buf2, pli->cxlk_mast_lkid);
	}
	TRdisplay("\n");
    }
    TRdisplay("\n");
    TRdisplay("\nCluster Nodes seen\n");
    TRdisplay("  Node Name\tNode Number\tLock Count\n");

    for (count = 0, node = nodes; count < node_count; ++count, ++node)
	TRdisplay("%11s\t%11x\t%10d\n", node->node_name, node->node_number,
                  node->lock_count);

    TRdisplay("\n");
}

#else /* !defined(conf_CLUSTER_BUILD) */

void
dmd_dlm_lock_info(bool waiters_only)
{
}

#endif
