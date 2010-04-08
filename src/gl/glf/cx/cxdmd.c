/*
** Copyright (c) 2005, 2008 Ingres Corporation
*/
#include <compat.h>
#include <pc.h>
#include <cs.h>
#include <me.h>
#include <si.h>
#include <st.h>
#include <tr.h>
#include <cx.h>
#ifdef VMS
#include <starlet.h>
#include <efndef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <lkidef.h>
#include <ssdef.h>
#include <syidef.h>
#endif

/*
** Name: cxdmd.c - Debug print function for cluster DLM locks
**
** Description:
**	This is used solely by cluster installations to print information
**	about Distributed Lock Manager locks, obtained directly from the DLM.
**
** History:
**	??-aug-2005 (devjo01)
**	    Original version (as standalone idlmdump.c).
**	21-feb-2008 (joea)
**	    Created.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
*/

#if defined(conf_CLUSTER_BUILD)
#ifdef VMS

/* VMS lock modes */
static const char *modes[] = { "NL", "CR", "CW", "PR", "PW", "EX" };

static char *queues[] = { "G", "C", "W" };

STATUS
CXdlm_lock_info(
    char *inst_id,
    bool waiters_only,
    CX_LOCK_LIST **lock_list)
{
    u_i4 cursor;
    i4 grant, wait, conv;
    u_i4 lockid;	
    u_i2 reslen;
    char resname[32];
    ILE3 itmlist1[] =
        {
	    { sizeof(grant), LKI$_GRANTCOUNT, (char *)&grant, 0 },
	    { sizeof(conv), LKI$_CVTCOUNT, (char *)&conv, 0 },
	    { sizeof(wait), LKI$_WAITCOUNT, (char *)&wait, 0, },
	    { sizeof(lockid), LKI$_LOCKID, (char *)&lockid, 0 },
	    { 31, LKI$_RESNAM, (char *)resname, &reslen },
	    { 0, 0, }
        };
    IOSB	iosb;
    LKIDEF	*pli;
    CX_LOCK_LIST   *prsrc, **prevaddr;
    i4 status;
    i4 inst_mask = 0;

    /* Initialize */
    *lock_list = NULL;
    if (inst_id && inst_id[0] && inst_id[1])
        inst_mask = ((unsigned)(inst_id[0]) << 24)
		    + (((unsigned)inst_id[1]) << 16);

    /* Loop over locks, gathering info by resource. */
    cursor = 0;
    while (1)
    {
	int cmpres;
	int count;

	status = sys$getlkiw(EFN$C_ENF, &cursor, itmlist1, &iosb, 0, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if (status != SS$_NORMAL)
	    break;
	/* Normalize resource */
	MEfill(sizeof(resname) - reslen, EOS, resname + reslen);

	/* Filter out non-Ingres locks */
	if (inst_mask && (inst_mask != (~(0x0FFFF) & *(long *)resname)))
	    continue;
	
	/* Filter out resources without waiters if desired */
	if (waiters_only && !conv && !wait)
	    continue;

	/* Filter out resources already seen */
	prevaddr = lock_list;
	cmpres = -1;
	while (prsrc = *prevaddr)
	{
	    cmpres = MEcmp(resname, &prsrc->cxll_resource, sizeof(LK_LOCK_KEY));
	    if (cmpres <= 0)
		break;
	    prevaddr = &prsrc->cxll_next;
	}

	if (cmpres == 0)
	    /* We've seen this resource before */
	    continue;

	/* Allocate new entry */
	count = grant + conv + wait;
	for (;;)
	{
	    struct
	    {
		unsigned length:16;
		unsigned width:15;
		unsigned overflow:1;
	    } lkisiz;
	    ILE3 itmlist2[] =
		{
		    { 0, LKI$_LOCKS, (char *)0, (unsigned short *)&lkisiz },
		};
	    LKIDEF *lkilist;
	    int i;

	    itmlist2[0].ile3$w_length = count * sizeof(LKIDEF);
	    lkilist = (LKIDEF *)MEreqmem(0, itmlist2[0].ile3$w_length, FALSE,
					 &status);
	    if (status)
	    {
		TRdisplay("Insufficient memory to allocate lock lists\n");
		return FAIL;
	    }
	    itmlist2[0].ile3$ps_bufaddr = (char *)lkilist;
	    status = sys$getlkiw(EFN$C_ENF, &lockid, itmlist2, &iosb, 0, 0, 0);
	    if (status & 1)
		status = iosb.iosb$w_status;
	    if (status != SS$_NORMAL)
	    {
		/* Lock was freed while we were processing? */
		TRdisplay("\n\t\tMissed lock, status = 0x%x\n", status);
		MEfree((PTR)lkilist);
		break;
	    }

	    if (lkisiz.overflow)
		continue;

	    count = lkisiz.length / lkisiz.width;
	    prsrc = (CX_LOCK_LIST *)MEreqmem(0, sizeof(CX_LOCK_LIST)
		     + (count - 1) * sizeof(CX_LOCK_INFO), FALSE, &status);
	    if (status)
	    {
		TRdisplay("Insufficient memory to allocate lock descriptor\n");
		return FAIL;
	    }
	    MEcopy(resname, sizeof(resname), &prsrc->cxll_resource);
	    prsrc->cxll_lock_count = count;
	    prsrc->cxll_next = *prevaddr;

	    for (i = 0; i < count; ++i)
	    {
		prsrc->cxll_locks[i].cxlk_csid = lkilist[i].lki$l_csid;
		prsrc->cxll_locks[i].cxlk_pid = lkilist[i].lki$l_pid;
		prsrc->cxll_locks[i].cxlk_lkid = lkilist[i].lki$l_lkid;
		prsrc->cxll_locks[i].cxlk_queue =
					queues[1 - lkilist[i].lki$b_queue];
		prsrc->cxll_locks[i].cxlk_req_mode =
					modes[lkilist[i].lki$b_rqmode];
		prsrc->cxll_locks[i].cxlk_grant_mode =
					modes[lkilist[i].lki$b_grmode];
		prsrc->cxll_locks[i].cxlk_mast_csid = lkilist[i].lki$l_mstcsid;
		prsrc->cxll_locks[i].cxlk_mast_lkid = lkilist[i].lki$l_mstlkid;
	    }
	    *prevaddr = prsrc;
	    break;
	}
    }

    return OK;
}

#else /* !VMS */

STATUS
CXdlm_lock_info(char *inst_id, bool waiters_only, CX_LOCK_LIST **lock_list)
{
    TRdisplay("\n\tDLM Lock information not available on this platform\n");
    return FAIL;
}

#endif /* VMS */
#else /* !defined(conf_CLUSTER_BUILD) */

STATUS
CXdlm_lock_info(char *inst_id, bool waiters_only, CX_LOCK_LIST **lock_list)
{
    return FAIL;
}

#endif
