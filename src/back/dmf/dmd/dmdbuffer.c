/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dm0pbmcb.h>
#include    <dm0l.h>
#include    <dmd.h>

/**
**
**  Name: DMDBUFFER.C - Buffer manager trace routines.
**
**  Description:
**      This file contains routines for buffer manager tracing.
**
**
**  History:
**      01-oct-86 (derek)
**	28-feb-89 (rogerk)
**	    Added support for shared buffer manager.  Added new bmcb fields.
**	    Use lbmcb pointer to print stats, not bmcb.
**	28-mar-90 (greg)
**	    Change %3d to %5d, buffer sizes bigger than 999 being chopped
**	    Could have cache bigger than 20MB so not %4d
**	07-july-1992 (jnash)
**	    Add DMF Function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Changes for new BM control blocks.
**	25-mar-96 (nick)
**	    Print FCwait statistic.
**	01-Apr-1997 (jenjo02)
**	    Table priority project:
**	    Added new stats for fixed priority tables.
**	01-May-1997 (jenjo02)
**	    Added new stat for group waits.
**	03-Apr-1998 (jenjo02)
**	    Added new stats for per-cache WriteBehind threads.
**	01-Sep-1998 (jenjo02)
**	  o Moved BUF_MUTEX waits out of bmcb_mwait, lbm_mwait, into DM0P_BSTAT where
**	    mutex waits by cache can be accounted for.
**	24-Aug-1999 (jenjo02)
**	    Added BMCB_FHDR, BMCB_FMAP page types, stat collection by
**	    page type.
**	10-Jan-2000 (jenjo02)
**	    Added stats by page type for groups.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	4-Feb-2002 (rigka01) Bug #106435 
**	    Use new TRdisplay datatype 'u' to display unsigned data; using 
**	    'd' causes large numbers to show up as negative when 
**	    for "set trace point dm420" output such as FIX CALLS.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**/


/*{
** Name: dmd_buffer	- Debug buffer manager information.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History: 
**      01-oct-86 (derek)
**	28-feb-89 (rogerk)
**	    Added support for shared buffer manager.  Added new bmcb fields.
**	    Use lbmcb pointer to print stats, not bmcb.
**	28-mar-90 (greg)
**	    Change %3d to %5d, buffer sizes bigger than 999 being chopped
**	    Could have cache bigger than 20MB so not %4d
**      01-Nov-94 (Bin Li)
**       Fix bug 61055, trace point  dm420 output is wrong. "FAST COMMIT
**       FLUSHES" should be "CONSISTENCY POINT FLUSHES".
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Changes for new BM control blocks.
**	25-mar-96 (nick)
**	    Print FCwait statistic.
**	01-Apr-1997 (jenjo02)
**	    Table priority project:
**	    Added new stats for fixed priority tables.
**	01-May-1997 (jenjo02)
**	    Added new stat for group waits.
**	03-Apr-1998 (jenjo02)
**	    Added new stats for per-cache WriteBehind threads.
**	01-Sep-1998 (jenjo02)
**	  o Moved BUF_MUTEX waits out of bmcb_mwait, lbm_mwait, into 
**	    DM0P_BSTAT where mutex waits by cache can be accounted for.
**	24-Aug-1999 (jenjo02)
**	    Added BMCB_FHDR, BMCB_FMAP page types, stat collection by
**	    page type.
**	09-Apr-2001 (jenjo02)
**	    Added GatherWrite stats.
**      4-Feb-2002 (rigka01) Bug #106435
**          Use new TRdisplay datatype 'u' to display unsigned data; using
**          'd' causes large numbers to show up as negative when
**          for "set trace point dm420" output such as FIX CALLS.
**	31-Jan-2003 (jenjo02)
**	    bs_stat name changes for Cache Flush Agents.
**	01-may-2008 (toumi01) Bug #119033
**	    Relabel GREADS summary (which is count of **IO events**) and
**	    GREADS FHDR, FMAP, ROOT, etc. which are counts of **pages**).
**	    Using the same labels makes it appear that the details should
**	    add up to the total, but they are apples and oranges.
**	    New literals are GREADIOS and GREADPGS. Ditto for GWRITES,
**	    which are now GWRITEIOS and GWRITEPGS.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Additional cache stats.
**	19-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bs_lreadio for physical log reads.
**	15-Mar-2010 (smeke01) b119529
**	    Snipped out code for displaying DM0P_BSTAT structure into 
**	    separate function dmd_summary_statistics(). Replaced
**	    display format %d with %u where appropriate.
*/
VOID
dmd_buffer(VOID)
{
	DMP_LBMCB		*lbmcb = dmf_svcb->svcb_lbmcb_ptr;
	DM0P_BM_COMMON          *bm_common = lbmcb->lbm_bm_common;
	i4                     cache_ix;
	DM0P_BMCB               *bm;
	DM0P_BSTAT              *bs;
	i4			pid;
	u_i4			fcount, mcount, lcount;
	u_i4			gfcount, gmcount, glcount;

	/*
	** Note that this can be a shared buffer manager and the values
	** being dumped may be changed while we are writing them.
	** No locks are taken on the buffer manager while the control blocks
	** are dumped here.
	*/

	if (bm_common->bmcb_srv_count > 1)
	{
	    PCpid(&pid);
	    TRdisplay("Connected to Server Pid: %d\n", pid);
	}

	TRdisplay ("Buffer Status: %v\n",
	    "FCFLUSH,SHARED,PASS_ABORT,PREPASS_ABORT,IOMASTER,DMCM,MT", 
		bm_common->bmcb_status);
	TRdisplay("");
	TRdisplay(" Buffer Manager Id: %u  Connected servers %u\n",
	    bm_common->bmcb_id, bm_common->bmcb_srv_count);
	TRdisplay(" CP count: %u  CP index: %u  CP check: %u\n",
	    bm_common->bmcb_cpcount, bm_common->bmcb_cpindex, bm_common->bmcb_cpcheck);
	TRdisplay(" Database cache size: %u  Table cache size: %u\n",
	    bm_common->bmcb_dbcsize, bm_common->bmcb_tblcsize);
	TRdisplay(" Statistics %120*-\n");

	TRdisplay("  Lock reclaim: %u  CP flushes: %u\n",
	    lbmcb->lbm_lockreclaim, lbmcb->lbm_fcflush);

	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++) 
	{
	    bm = lbmcb->lbm_bmcb[cache_ix];

	    if (!bm || (bm->bm_status & BM_ALLOCATED) == 0)
		continue;

	    bs = &lbmcb->lbm_stats[cache_ix];

	    DM0P_COUNT_ALL_QUEUES(fcount, mcount, lcount);
	    DM0P_COUNT_ALL_GQUEUES(gfcount, gmcount, glcount);

	    TRdisplay(" Buffer Cache Configuration (%2dK) %97*-\n",
			bm->bm_pgsize/1024);

	    TRdisplay("  Buffer count: %u  Bucket count: %u Group count: %u Size: %u\n", 
		bm->bm_bufcnt, bm->bm_hshcnt, bm->bm_gcnt, bm->bm_gpages);
	    TRdisplay("  Free count: %u Limit: %u Modify count: %u Limit: %u\n",
		fcount, bm->bm_flimit, 
		mcount, bm->bm_mlimit); 

	    TRdisplay("  Free group count: %u Modify group count: %u\n",
		gfcount, gmcount); 
	    TRdisplay("  Fixed count: %u Group fixed count: %u\n", 
		lcount, glcount);

	    /* Only show WriteBehind stats if WB in use in this cache */
	    if (bm->bm_status & BM_WB_IN_CACHE)
	    {
		TRdisplay("  WB start limit: %u WB end limit: %u\n",
		    bm->bm_wbstart, bm->bm_wbend);
		TRdisplay("  WB flushes: %u, Agents cloned: %u\n",
		    bs->bs_cache_flushes, bs->bs_cfa_cloned);
		TRdisplay("  Agents active: %u, Agent hwm: %u\n",
		    bs->bs_cfa_active, bs->bs_cfa_hwm);
		TRdisplay("  WB pages flushed: %u WB groups flushed: %u\n",
		    bs->bs_wb_flushed, bs->bs_wb_gflushed);
	    }

	    dmd_summary_statistics(bs, bm->bm_pgsize, 2);
	}
}


/*{
** Name: dmd_summary_statistics
**
** Description:
**
**	Print to dbms log the values in the supplied DM0P_BSTAT statistics
**	structure pointed to by parameter bs. Print the page size in Kb, 	
**	supplied in bytes by the parameter pgsize. Indent the printed lines 
**	by between 0 and 5 characters, controlled by the value supplied in 
**	parameter indent. The indentation logic is needed to allow this 
**	function to be called from dmc_stop_server() and dmd_fmt_cb() as 
**	well as dmd_buffer(). This eliminates duplication of code and helps 
**	standardise output layout for the same information.
**
** Inputs:
**
**	bs	pointer to DM0P_BSTAT structure to be printed.
**	pgsize 	page size in bytes, for display in Kb.
**	indent	control over left hand indentation for heading and data
**		lines. Valid values are trimmed to between 0 to 5 inclusive.
**
** Outputs:
**	Returns:
**	    None.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	15-Mar-2010 (smeke01) b119529
**	    First version. Created from lines cut from dmd_buffer().
**	27-Aug-2010 (thaju02) B124324
**	    Changed TRdisplay from 'u' to 'lu' since DM0P_BMSTAT members 
**	    are now u_i8.
*/
VOID
dmd_summary_statistics(DM0P_BSTAT *bs, i4 pgsize, i4 indent)
{
    i4	i;
    i4	indent_heading, trailer_heading;
    i4	indent_data;

    /* 
    ** There's at most 5 spaces spare for indentation.
    ** Indent data more than heading if possible. 
    */
    if (indent > 5)
	indent = 5;
    else
    if (indent < 0)
	indent = 0;

    indent_heading = indent;
    if (indent < 5)
    	indent_data = indent + 1;
    else
	indent_data = 5;

    /* TRdisplay max line length is 132 including the initial '!' character */
    trailer_heading = (132 - strlen(" Summary Statistics (nnK) ")) - indent_heading;
    TRdisplay("%#* Summary Statistics (%2dK) %#*-\n", indent_heading, pgsize/1024, trailer_heading);

    /*
    ** The longest title string is "UNFIX CALLS", which is 11 characters.
    ** The title strings are left-padded with spaces to 20 characters.
    */
    TRdisplay("%#* %5(%20s %)\n",
	indent_data, "               GWAIT", "               GSYNC", "            FREEWAIT", "            GW PAGES", "              GW I/O");
    TRdisplay("%#* %5(%20lu %)\n",
	indent_data, bs->bs_gwait, bs->bs_gsyncwr, bs->bs_fwait,
	bs->bs_gw_pages, bs->bs_gw_io);
    TRdisplay("%#* %6(%20s %)\n",
	indent_data, "           FIX CALLS", "                HITS", "               CHECK", "             REFRESH", "                READ", "                TOSS");
    TRdisplay("%#* %6(%20lu %)\n",
	indent_data, bs->bs_fix[BMCB_PTYPES], bs->bs_hit[BMCB_PTYPES],
		bs->bs_check[BMCB_PTYPES], bs->bs_refresh[BMCB_PTYPES],
		bs->bs_reads[BMCB_PTYPES], bs->bs_toss[BMCB_PTYPES]);
    TRdisplay("%#* %6(%20s %)\n",
	indent_data, "         UNFIX CALLS", "               DIRTY", "               FORCE", "               WRITE", "              IOWAIT", "                SYNC");
    TRdisplay("%#* %6(%20lu %)\n",
	indent_data, bs->bs_unfix[BMCB_PTYPES], bs->bs_dirty[BMCB_PTYPES],
		bs->bs_force[BMCB_PTYPES], bs->bs_writes[BMCB_PTYPES],
		bs->bs_iowait[BMCB_PTYPES], bs->bs_syncwr[BMCB_PTYPES]);
    TRdisplay("%#* %5(%20s %)\n",
	indent_data, "               MWAIT", "               PWAIT", "              FCWAIT", "             RECLAIM", "             REPLACE");
    TRdisplay("%#* %5(%20lu %)\n",
	indent_data, bs->bs_mwait[BMCB_PTYPES], bs->bs_pwait[BMCB_PTYPES], bs->bs_fcwait[BMCB_PTYPES],
		bs->bs_reclaim[BMCB_PTYPES], bs->bs_replace[BMCB_PTYPES]);
    TRdisplay("%#* %2(%20s %)\n",
	indent_data, "            GREADIOS", "           GWRITEIOS");
    TRdisplay("%#* %2(%20lu %)\n",
	indent_data, bs->bs_greads[BMCB_PTYPES], bs->bs_gwrites[BMCB_PTYPES]);
    /* Don't show CR stats if none */
    if ( bs->bs_crreq[BMCB_PTYPES] )
    {
	TRdisplay("%#* %6(%20s %)\n",
	    indent_data, 
	    "               CRREQ", "              CRALOC", "               CRMAT", "                NOCR", "              CRTOSS", "               RTOSS");
	TRdisplay("%#* %6(%20lu %)\n",
	    indent_data, bs->bs_crreq[BMCB_PTYPES], bs->bs_craloc[BMCB_PTYPES],
		bs->bs_crmat[BMCB_PTYPES], bs->bs_nocr[BMCB_PTYPES], 
		bs->bs_crtoss[BMCB_PTYPES], bs->bs_roottoss[BMCB_PTYPES]);
	TRdisplay("%#* %4(%20s %)\n",
	    indent_data, 
	    "               CRHIT", "               LREAD", "             LREADIO", "               LUNDO");
	TRdisplay("%#* %4(%20lu %)\n",
	    indent_data, bs->bs_crhit[BMCB_PTYPES], bs->bs_lread[BMCB_PTYPES], 
		bs->bs_lreadio[BMCB_PTYPES], bs->bs_lundo[BMCB_PTYPES]);
	TRdisplay("%#* %3(%20s %)\n",
	    indent_data, 
	    "               JREAD", "               JUNDO", "                JHIT");
	TRdisplay("%#* %3(%20lu %)\n",
	    indent_data, bs->bs_jread[BMCB_PTYPES], bs->bs_jundo[BMCB_PTYPES],
		bs->bs_jhit[BMCB_PTYPES]);
    }

    /* Show stats by page type: */
    for (i = 0; i < BMCB_PTYPES; i++)
    {
	if (bs->bs_fix[i] || bs->bs_reclaim[i])
	{
	    TRdisplay("%#* Stats for %w pages:\n",
		indent_heading,
			"FHDR,FMAP,ROOT,INDEX,LEAF,DATA", i);
	    TRdisplay("%#* %6(%20s %)\n",
		indent_data,
		"           FIX CALLS", "                HITS", "               CHECK", "             REFRESH", "                READ", "                TOSS");
	    TRdisplay("%#* %6(%20lu %)\n",
		indent_data, bs->bs_fix[i], bs->bs_hit[i],
			bs->bs_check[i], bs->bs_refresh[i],
			bs->bs_reads[i], bs->bs_toss[i]);
	    TRdisplay("%#* %6(%20s %)\n",
		indent_data,
		"         UNFIX CALLS", "               DIRTY", "               FORCE", "               WRITE", "              IOWAIT", "                SYNC");
	    TRdisplay("%#* %6(%20lu %)\n",
		indent_data, bs->bs_unfix[i], bs->bs_dirty[i],
			bs->bs_force[i], bs->bs_writes[i],
			bs->bs_iowait[i], bs->bs_syncwr[i]);
	    TRdisplay("%#* %5(%20s %)\n",
		indent_data, 
		"               MWAIT", "               PWAIT", "              FCWAIT", "             RECLAIM", "             REPLACE");
	    TRdisplay("%#* %5(%20lu %)\n",
		indent_data, bs->bs_mwait[i], bs->bs_pwait[i], bs->bs_fcwait[i],
			bs->bs_reclaim[i], bs->bs_replace[i]);
	    TRdisplay("%#* %2(%20s %)\n",
		indent_data, 
		"            GREADIOS", "           GWRITEIOS");
	    TRdisplay("%#* %2(%20lu %)\n", 
		indent_data, bs->bs_greads[i], bs->bs_gwrites[i]);

	    /* Don't show CR stats if none */
	    if ( bs->bs_crreq[i] )
	    {
		TRdisplay("%#* %6(%20s %)\n",
		    indent_data, 
		    "               CRREQ", "              CRALOC", "               CRMAT", "                NOCR", "              CRTOSS", "               RTOSS");
		TRdisplay("%#* %6(%20lu %)\n",
		    indent_data, bs->bs_crreq[i], bs->bs_craloc[i],
			bs->bs_crmat[i], bs->bs_nocr[i], 
			bs->bs_crtoss[i], bs->bs_roottoss[i]);
		TRdisplay("%#* %4(%20s %)\n",
		    indent_data, 
		    "               CRHIT", "               LREAD", "             LREADIO", "               LUNDO");
		TRdisplay("%#* %4(%20lu %)\n",
		    indent_data, bs->bs_crhit[i], bs->bs_lread[i], 
		        bs->bs_lreadio[i], bs->bs_lundo[i]);
		TRdisplay("%#* %3(%20s %)\n",
		    indent_data, 
		    "               JREAD", "               JUNDO", "                JHIT");
		TRdisplay("%#* %3(%20lu %)\n",
		    indent_data, bs->bs_jread[i], bs->bs_jundo[i],
		bs->bs_jhit[i]);
	    }
	}
    }
    return;
}
