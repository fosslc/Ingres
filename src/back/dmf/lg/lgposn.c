/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGPOSN.C - Implements the LGposition function of the logging system
**
**  Description:
**	This module contains the code which implements LGposition.
**	
**	    LGposition -- Establish position for reading the logfile.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-sep-1993 (bryanp)
**	    Clear the CL error descriptor to avoid junk in error messages.
**	18-oct-1993 (rmuth)
**	    Pprototype.
**	31-may-1994 (bryanp) B56858
**	    Detect repositioning an already positioned logfile context to a log
**		address which is already on the currently mapped page, and in
**		this one important special case (used in particular by RCP undo
**		recovery) don't throw away the currently mapped page.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly 
**	    named when calling LKMUTEX functions. Singular lgd_mutex
**	    augmented with many more.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed 
**          for support of logs > 2 gig
**	24-Aug-1999 (jenjo02)
**	    Removed mutexing of lfb_cp_mutex, lfb_current_lbb_mutex,
**	    both of which have been removed.
**	15-Dec-1999 (jenjo02)
**	    Add support for shared log transactions (LXB_SHARED/LXB_SHARED_HANDLE).
**      03-Mar-2000 (stial01)
**          LG_position Fixed condition for special case (B100719, star 8768498)
**	14-Mar-2000 (jenjo02)
**	    Removed static LG_position() function and the unnecessary level of
**	    indirection it induced.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/


/*{
** Name: LGposition	- Position for reading log file.
**
** Description:
**      This routine initializes and positions a read context buffer
**	for a call to LGread().
**
**	The caller is given the following positioning options to start with:
**	    LG_P_PAGE - Position the file for access by page number.
**	    LG_P_LGA  - Position file by given log address.
**	    LG_P_FIRST - Position to first record in log file.
**	    LG_P_LAST - Position to last record in log file.
**	    LG_P_TRANS - Position to last record for given transaction.
**
**	The caller is given the following direction options:
**	    LG_D_FORWARD - Read forward for next object.
**	    LG_D_BACKWARD - Read backward for next object.
**	    LG_D_PREVIOUS - Read backward by previous log address.
**
**	When the position is LG_P_PAGE, the log addresses given have
**	the la_sequence field set to zero.
**
** Inputs:
**      lx_id                           Transaction identifier.
**      position                        Read position. One of LG_P_*.
**      direction                       Read direction. One of LG_D_*.
**      lga                             Optional log record address.
**	context				Pointer to context.
**	l_context			Length of context.
**
** Outputs:
**      err_code                         Reason for error return status.
**      sys_err                         Reason for error return status.
**	Returns:
**	    LG_I_NORMAL
**	    LG_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-oct-1993 (rmuth)
**	    prototype.
**	10-oct-93 (swm)
**	    Bug #56439
**	    Now that lgc_check has changed from i4 to PTR, context is
**	    stored directly in lgc_check; t can be checked with a direct
**	    pointer comparison rather with a (i4) checksum.
**	31-may-1994 (bryanp) B56858
**	    Detect repositioning an already positioned logfile context to a log
**		address which is already on the currently mapped page, and in
**		this one important special case (used in particular by RCP undo
**		recovery) don't throw away the currently mapped page.
**	25-Jan-1996 (jenjo02)
**	    Singular lgd_mutex augmented with several more.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED log transactions.
**	02-May-2003 (jenjo02)
**	    Mutex current_lbb while fetching lgh_end
**	    to ensure consistency.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Make sure I/O buffer is aligned.
**	16-Dec-2009 (kschendel) SIR 122757
**	    Above needs to guard against no-alignment.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC:
**	    Add ability to bulk-read. What remains in the context memory
**	    after the usual stuff is carved out becomes an array of contiguous
**	    log blocks available to LGread. This is useful for things like the
**	    Archiver, or any other client that reads the log, with the intent
**	    of reducing the interference on the physical log file between
**	    log writers and log readers.
**	08-Apr-2010 (jonj)
**	    SIR 121619 MVCC: Change lgc_bufid from i4 to *i4.
*/
STATUS
LGposition(
LG_LXID		external_lx_id,
i4		position,
i4		direction,
LG_LA		*lga,
PTR		external_context,
i4		l_context,
CL_ERR_DESC	*sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB    *lxb, *slxb;
    register LFB    *lfb;
    SIZE_TYPE	    *lxbb_table;
    i4		    align;
    i4	    	    err_code;
    STATUS	    status;
    LG_I4ID_TO_ID   lx_id;
    LGC		    *context = (LGC*)external_context;

    LG_WHERE("LGposition")

    CL_CLEAR_ERR(sys_err);

    /*	Check the lg_id. */

    lx_id.id_i4id = external_lx_id;
    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA408_LGPOSN_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
    {
	uleFormat(NULL, E_DMA409_LGPOSN_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id.id_lgid.id_instance);
	return (LG_BADPARAM);
    }

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);

    if (position == 0 || position > LG_P_MAX ||
	direction == 0 || direction > LG_D_MAX ||
	l_context < sizeof(LGC) + LG_MAX_RSZ + lfb->lfb_header.lgh_size)
    {
	uleFormat(NULL, E_DMA40A_LGPOSN_BAD_PARM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, position, 0, LG_P_MAX, 0, direction, 0, LG_D_MAX,
		    0, l_context,
		    0, (sizeof(LGC) + LG_MAX_RSZ + lfb->lfb_header.lgh_size));
	return (LG_BADPARAM);
    }

    /*	Check position type. */

    if (position == LG_P_PAGE)
    {
	if (lga)
	{
	    if (lga->la_offset & (sizeof(i4) - 1))
	    {
		uleFormat(NULL, E_DMA40B_LGPOSN_BAD_LGA, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, position,
			    0, lga->la_sequence, 0, lga->la_offset);
		return (LG_BADPARAM);
	    }
	    context->lgc_lga = *lga;
	}
	else
	{
	    uleFormat(NULL, E_DMA40C_LGPOSN_NULL_LGA, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			    0, position);
	    return (LG_BADPARAM);
	}
    }
    else
    {
	if (position == LG_P_LGA)
	{
	    /*	Remember the LGA. */

	    if (lga)
	    {
		if (lga->la_offset & (sizeof(i4) - 1))
		{
		    uleFormat(NULL, E_DMA40B_LGPOSN_BAD_LGA, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
				0, position,
				0, lga->la_sequence, 0, lga->la_offset);
		    return (LG_BADPARAM);
		}
		context->lgc_lga = *lga;
	    }
	    else
	    {
		uleFormat(NULL, E_DMA40C_LGPOSN_NULL_LGA, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
				0, position);
		return (LG_BADPARAM);
	    }
	}    
	else
	{
	    /*	Set either first or last. */

	    if (position == LG_P_FIRST)
	    {
		context->lgc_lga = lfb->lfb_header.lgh_begin;
	    }
	    else if (position == LG_P_LAST)
	    {
		LBB	*lbb;
		/* lgh_end is valid only when CURRENT lbb mutexed */
		while (lbb = (LBB*)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
		{
		   if ( status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex) )
		       return(status);

                   if ( lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
                   {
		      /* current_lbb is still the "CURRENT" lbb. */
                      break;
                   }

                   (VOID)LG_unmutex(&lbb->lbb_mutex);

#ifdef xDEBUG
                   TRdisplay( "LGposn(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                              lfb->lfb_header.lgh_end.la_sequence,
                              lfb->lfb_header.lgh_end.la_block,
                              lfb->lfb_header.lgh_end.la_offset);
#endif
                }

		context->lgc_lga = lfb->lfb_header.lgh_end;
		(VOID)LG_unmutex(&lbb->lbb_mutex);
	    }
	    else if (position == LG_P_TRANS)
	    {
		/* If handle to SHARED transaction, fetch it */
		if ( lxb->lxb_status & LXB_SHARED_HANDLE )
		{
		    if ( lxb->lxb_shared_lxb == 0 )
		    {
			LG_I4ID_TO_ID xid;
			xid.id_lgid = lxb->lxb_id;
			uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, xid.id_i4id,
			    0, lxb->lxb_status,
			    0, "LG_position");
			return(LG_BADPARAM);
		    }

		    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		    if ( slxb->lxb_type != LXB_TYPE ||
			(slxb->lxb_status & LXB_SHARED) == 0 )
		    {
			LG_I4ID_TO_ID xid1, xid2;
			xid1.id_lgid = slxb->lxb_id;
			xid2.id_lgid = lxb->lxb_id;
			uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			    0, slxb->lxb_type,
			    0, LXB_TYPE,
			    0, xid1.id_i4id,
			    0, slxb->lxb_status,
			    0, xid2.id_i4id,
			    0, "LG_position");
			return(LG_BADPARAM);
		    }

		    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
		    
		    /* Set txn's last LGA from the SHARED lxb */
		    context->lgc_lga = slxb->lxb_last_lga;

		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		}
		else
		    context->lgc_lga = lxb->lxb_last_lga;
	    }

	    /*	If low is zero then the file is empty. */

	    if (context->lgc_lga.la_block == 0)
		return (LG_ENDOFILE);
	}
    }

    if ((context->lgc_status & LGC_VALID) != 0 &&
	context->lgc_check == (PTR)context &&
	context->lgc_direction == direction && direction == LG_D_PREVIOUS &&
	context->lgc_position == position && position == LG_P_LGA &&
	context->lgc_record == (LRH *)((char *)context + sizeof(LGC)) &&
	context->lgc_buffer != NULL &&
	context->lgc_lga.la_sequence == 
			    context->lgc_buffer->lbh_address.la_sequence &&
	context->lgc_lga.la_block ==
			    context->lgc_buffer->lbh_address.la_block &&
	(context->lgc_current_lbb ||
	 LGchk_sum((PTR)context->lgc_buffer, context->lgc_size) ==
			    context->lgc_buffer->lbh_checksum) )
    {
	/*
	** This is an important special case worth optimizing. We are asking
	** to position an LGC which is already positioned for the same direction
	** processing and already has the correct page mapped in. Rather than
	** re-reading the same page, we'll just use the page which is already
	** mapped. This arises in dmr_undo_pass, which commonly performs an
	** alternating sequence of LGposition/LGread/LGposition/LGread calls
	** moving backwards through the log file often positioning itself to
	** another log record located earlier on the same log page.
	*/
	context->lgc_current_lga = context->lgc_lga;
	context->lgc_offset = context->lgc_current_lga.la_offset;

	return (OK);
    }

    /*	Initialize the context for input to LGread. */

    context->lgc_status = LGC_VALID;
    context->lgc_check = (PTR)context;
    context->lgc_current_lga.la_sequence = 0;
    context->lgc_current_lga.la_block    = 0;
    context->lgc_current_lga.la_offset   = 0;
    context->lgc_size = lfb->lfb_header.lgh_size;
    context->lgc_count = lfb->lfb_header.lgh_count;

    /* Number of LBB's in this configuration */
    context->lgc_bufcnt = lfb->lfb_buf_cnt;
    /* bufid may be supplied by LGread */
    context->lgc_bufid = NULL;

    context->lgc_min = 1;
    context->lgc_max = lfb->lfb_header.lgh_count - 1;
    context->lgc_direction = direction;
    context->lgc_position = position;
    context->lgc_record = (LRH *)((char *)context + sizeof(LGC));
    context->lgc_offset = context->lgc_lga.la_offset;
    context->lgc_lga.la_offset = 0;
    if (context->lgc_offset < sizeof(LBH) - sizeof(i4))
	context->lgc_offset = sizeof(LBH) - sizeof(i4);

    /* Figure out where the (aligned) log file block pool begins */
    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    context->lgc_blocks = (char *)context->lgc_record + LG_MAX_RSZ;
    if (align > 0)
	context->lgc_blocks = ME_ALIGN_MACRO(context->lgc_blocks, align);

    /* Point lgc_buffer to first block in pool */
    context->lgc_buffer = (LBH*)context->lgc_blocks;
    context->lgc_curblock = 0;
    context->lgc_firstblock = 0;
    context->lgc_readblocks = 0;
    context->lgc_current_lbb = FALSE;

    /*
    ** Determine number of blocks in pool.
    **
    ** Note that we don't do bulk reads if the log
    ** file is partitioned.
    */
    if ( lfb->lfb_header.lgh_partitions > 1 )
        context->lgc_numblocks = 1;
    else
	context->lgc_numblocks = (l_context - (sizeof(LGC) + LG_MAX_RSZ + align)) /
				    context->lgc_size;

    return (OK);
}
