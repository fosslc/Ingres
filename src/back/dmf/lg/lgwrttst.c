/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGWRTTST.C - Implements the automated testing of the logging system
**
**  Description:
**	This module contains the code which implements the automated test
**	instrumentation for LGwrite. This code is only needed in a system which
**	is instrumented for automated test.
**	
**
**  History:
**	16-feb-1993 (bryanp)
**	    Updates for the new portable logging system.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	31-jan-1994 (bryanp) B56538, B56721, B56769, B56784
**	    Fix dual logging trace points.
**	29-Jan-1996 (jenjo02)
**	    Added lgd_mutex_is_locked (TRUE,FALSE) parm to 
**	    LG_signal_event() calls as part of mutex granularity
**	    project.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifdef LG_DUAL_LOGGING_TESTBED
/*
** Name: LG_check_write_test_1	- part 1 of automated write_block testing
**
** Description:
**	This routine examines the automated write testing testpoints, and
**	takes appropriate action if indicated.
**
**	We simulate the following types of faults:
**	1) An ordinary data block can't be written to, or gets corrupted
**	    once written. (Since VMS does 'bad block re-vectoring', this
**	    is unlikely on VMS; we simulate it anyway for completeness of
**	    testing).
**	2) A power failure occurs after writing a block to one of the
**	    logs but before the block is written to the other log (1 written
**	    correctly, 1 not at all).
**	3) A power failure occurs after writing a block to one of the logs
**	    but during the write to the other log (1 written correctly, 1
**	    written partially).
**	4) A power failure occurs during the write to the first log (1 written
**	    correctly, 1 not written at all).
**	5) A log header can't be written to. In this case, we simulate both
**	    copies becoming unreadabkle, but both copies on the other log are
**	    fine (with 4 copies of the log header, this is perhaps a VERY
**	    unlikely scenario)
**	6) A power failure occurs after writing the headers to one of the logs
**	    but before the headers are written to the other logs (primary's
**	    headers written correctly, dual's not at all).
**	7) A power failure occurs after writing the headers to one of the logs
**	    and during the writing of the headers to the other log (primary's
**	    headers written correctly, dual's written partially).
**	8) A power failure occurs during the header write to the primary log.
**
**	This routine is called before either I/O has been started.
**
** Inputs:
**	lgd	    - the LGD
**	lbb	    - the buffer being written
**	size	    - the I/O size in bytes.
**	writing_primary	    - are we actually writing the dual log or not?
**
** Outputs:
**	lbb		    - buffer may be intentionally corrupted.
**	size		    - may be modified to simulate partial write
**	write_primary	    - set to TRUE if primary will be written
**			      set to FALSE otherwise
**	write_dual	    - set to TRUE if dual will be written
**			      set to FALSE otherwise
**	qio_fail_primary    - set to TRUE if primary's QIO will fail.
**			      set to FALSE otherwise
**	qio_fail_dual	    - set to TRUE if dual's QIO will fail.
**			      set to FALSE otherwise
**	save_checksum	    - a copy of the buffer's checksum.
**	save_size	    - a copy of the I/O size
**	sync_write	    - if the primary write should be sync, set to TRUE
**
** History:
**	30-aug-1990 (bryanp)
**	    Created.
**	31-jan-1994 (bryanp) B56721, B56769, B56784
**	    Add writing_primary argument, fix dual logging trace points.
**	29-Jan-1996 (jenjo02)
**	    Added lgd_mutex_is_locked (TRUE,FALSE) parm to 
**	    LG_signal_event() calls as part of mutex granularity
**	    project.
**	06-Jul-2010 (jonj) SIR 122696
**	    Last 4 bytes on non-header pages reserved for la_sequence,
**	    excluded from lbh_used.
*/
VOID
LG_check_write_test_1(
LGD	    *lgd,
LBB	    *lbb,
i4	    writing_primary,
i4	    *size,
bool	    *write_primary,
bool	    *write_dual,
bool	    *qio_fail_primary,
bool	    *qio_fail_dual,
i4	    *save_checksum,
i4	    *save_size,
bool	    *sync_write)
{
    LG_HEADER	    *lgh;
    LBH		    *lbh;
    i4	    block_distance;
    LFB		    *lfb;

    *write_primary = *write_dual = TRUE;
    *qio_fail_primary = *qio_fail_dual = FALSE;
    *sync_write = FALSE;
    lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
    if (lbb->lbb_lga.la_offset != 0)
    {
	*save_checksum = lbh->lbh_checksum;
    }
    *save_size     = *size;

    block_distance = ((lbb->lbb_lga.la_offset >> 9) - lgd->lgd_test_badblk);
    if (block_distance < 0)
	block_distance = 0 - block_distance;

#if 0
    if (lgd->lgd_test_badblk)
	TRdisplay("LCWT1: Page <%d,%d,%d> is %d from %d (psize=%d)\n",
		    lbb->lbb_lga.la_sequence,
		    lbb->lbb_lga.la_offset / lfb->lfb_header.lgh_size,
		    lbb->lbb_lga.la_offset & (lfb->lfb_header.lgh_size - 1),
		    block_distance, lgd->lgd_test_badblk,
		    (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) );
#endif

    if (BTtest(LG_T_DATAFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbh->lbh_checksum++;
	    *sync_write = TRUE;
	    BTclear(LG_T_DATAFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec);
	    TRdisplay("LCWT1: Forcing bad checksum on page <%d,%d,%d>\n",
		    lbb->lbb_lga.la_sequence,
		    lbb->lbb_lga.la_offset / lfb->lfb_header.lgh_size,
		    lbb->lbb_lga.la_offset & (lfb->lfb_header.lgh_size - 1));
	}
    }
    if (BTtest(LG_T_DATAFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    /* dual will get corrupted, so write primary synchronously so
	    ** it will get written correctly
	    */
	    *sync_write = TRUE;
	}
    }
    if (BTtest(LG_T_PWRBTWN_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_dual = FALSE;
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN, 
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PWRBTWN_DUAL,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_primary = FALSE;
	    }
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PARTIAL_AFTER_PRIMARY,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    TRdisplay("LCWT1: Forcing partial write after page <%d,%d,%d>\n",
		    lbb->lbb_lga.la_sequence,
		    lbb->lbb_lga.la_offset / lfb->lfb_header.lgh_size,
		    lbb->lbb_lga.la_offset & (lfb->lfb_header.lgh_size - 1));
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PARTIAL_AFTER_DUAL,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *size /= 2;	    /* simulate partial write to primary */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	    TRdisplay("LCWT1: Forcing partial write after page <%d,%d,%d>\n",
		    lbb->lbb_lga.la_sequence,
		    lbb->lbb_lga.la_offset / lfb->lfb_header.lgh_size,
		    lbb->lbb_lga.la_offset & (lfb->lfb_header.lgh_size - 1));
	}
    }
    if (BTtest(LG_T_PARTIAL_DURING_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_dual = FALSE;
	    *size /= 2;	    /* simulate partial write to primary */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PARTIAL_DURING_DUAL,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_primary = FALSE;
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PARTIAL_DURING_BOTH,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *size /= 2;	    /* simulate partial write to primary */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_HDRFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    lgh = (LG_HEADER *)lbh;
	    *save_checksum = lgh->lgh_checksum;
    	    lgh->lgh_checksum++;	/* scramble first copy */
	    lgh = (LG_HEADER *)(((char *)lbh) + 1024);
    	    lgh->lgh_checksum++;	/* scramble second copy */
	    *sync_write = TRUE;
	}
    }
    if (BTtest(LG_T_HDRFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    *sync_write = TRUE;
	}
    }
    if (BTtest(LG_T_HDRFAIL_BETWEEN,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_dual = FALSE;
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	    *sync_write = TRUE;
	}
    }
    if (BTtest(LG_T_HDR_PARTIAL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    lbb->lbb_resume_cnt = 1;	/* only 1 write will be done */
	    *write_dual = FALSE;
	    *size = 64;	    /* only 1st 64 bytes of header get written */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_QIOFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *qio_fail_primary = TRUE;
	    BTclear(LG_T_QIOFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_QIOFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (!writing_primary)
	{
	    if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	    {
		*qio_fail_dual = TRUE;
		BTclear(LG_T_QIOFAIL_DUAL,(char *)lgd->lgd_test_bitvec);
	    }
	}
    }
    if (BTtest(LG_T_QIOFAIL_BOTH,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *qio_fail_primary = TRUE;
	    *qio_fail_dual = TRUE;
	    /*
	    ** B56769: Don't clear the testpoint here. We are setting both
	    ** qio_fail_primary and qio_fail_dual, but in fact the logwriter
	    ** which is calling us is only writing one of these logs, and we
	    ** want the test point to remain on until the second logwriter
	    ** arrives to write the second log. Once that has happened, the
	    ** system will immediately come down, so it doesn't hurt to leave
	    ** the test point on (it's cleared when the system comes down).
	    */
	}
    }
    if (BTtest(LG_T_QIOFAIL_HDRPRIM,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    *qio_fail_primary = TRUE;
	    BTclear(LG_T_QIOFAIL_HDRPRIM,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_QIOFAIL_HDRDUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (!writing_primary)
	{
	    if (lbb->lbb_lga.la_offset == 0)
	    {
		*qio_fail_dual = TRUE;
		BTclear(LG_T_QIOFAIL_HDRDUAL,(char *)lgd->lgd_test_bitvec);
	    }
	}
    }
    if (BTtest(LG_T_PARTIAL_SPAN,(char *)lgd->lgd_test_bitvec))
    {
	if ((lbh->lbh_used == lfb->lfb_header.lgh_size-LG_BKEND_SIZE) &&
	    ((lbh->lbh_address.la_offset &
		(lfb->lfb_header.lgh_size - 1))  == 0) )
	{
	    /*
	    ** This page is full, and contains no complete record.
	    ** Simulate a power failure.
	    */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }
    if (BTtest(LG_T_PARTIAL_LAST,(char *)lgd->lgd_test_bitvec))
    {
	if ((lbh->lbh_used == lfb->lfb_header.lgh_size-LG_BKEND_SIZE) &&
	    ((lbh->lbh_address.la_offset &
		(lfb->lfb_header.lgh_size - 1)) <
	     (lfb->lfb_header.lgh_size-LG_BKEND_SIZE - sizeof(i4))) )
	{
	    /*
	    ** This page is full, and the last record on it is incomplete.
	    ** Simulate a power failure.
	    */
	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	}
    }

    return;
}

/*
** Name: LG_check_write_test_2	- part 2 of automated write_block testing
**
** Description:
**	This routine is similar to check_write_test_1, but runs after the
**	first I/O has been queued and before the 2nd one has.
**
**	In this routine, any simulated failures which apply to the DUAL log
**	are processed. Also, any damage which was done to primary but not
**	to dual is 'repaired' here so that dual is written correctly.
**
** Inputs:
**	lgd	    - the LGD
**	lbb	    - the buffer being written
**	size	    - the I/O size in bytes.
**	save_checksum	    - a copy of the buffer's checksum.
**	save_size	    - a copy of the I/O size
**	write_primary	    - are we actually writing the dual log or not?
**
** Outputs:
**	size	    - may be altered to simulate a partial write
**	lbb	    - may be intentionally corrupted.
**
** History:
**	30-aug-1990 (bryanp)
**	    Created.
**	31-jan-1994 (bryanp) B56538
**	    Added write_primary to allow this code to function properly in the
**		world of logwriter threads, where this routine is sometimes
**		called by a logwriter which wrote the primary log, but not the
**		dual log, and hence must only take certain actions.
*/
VOID
LG_check_write_test_2(
LGD	    *lgd,
LBB	    *lbb,
i4	    write_primary,
i4	    *size,
i4	    *save_checksum,
i4	    *save_size )
{
    LG_HEADER	    *lgh;
    LBH		    *lbh;
    i4	    block_distance;
    LFB		    *lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);

    block_distance = ((lbb->lbb_lga.la_offset >> 9) - lgd->lgd_test_badblk);
    if (block_distance < 0)
	block_distance = 0 - block_distance;

    lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
    if (lbb->lbb_lga.la_offset != 0)
	lbh->lbh_checksum = *save_checksum;
    *size = *save_size;

    if (BTtest(LG_T_DATAFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (!write_primary)
	{
	    if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	    {
		lbh->lbh_checksum++;
		BTclear(LG_T_DATAFAIL_DUAL,(char *)lgd->lgd_test_bitvec);
	    }
	}
    }
    if (BTtest(LG_T_PARTIAL_AFTER_PRIMARY,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *size /= 2;	    /* simulate partial write */
	}
    }
    if (BTtest(LG_T_PARTIAL_DURING_DUAL,   (char *)lgd->lgd_test_bitvec) ||
	BTtest(LG_T_PARTIAL_DURING_BOTH,   (char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    *size /= 2;	    /* simulate partial write */
	}
    }
    if (BTtest(LG_T_HDRFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    /* repair the damaged checksum so the dual headers are ok */
	    lgh = (LG_HEADER *)lbh;
	    lgh->lgh_checksum = *save_checksum;
	    lgh = (LG_HEADER *)(((char *)lbh) + 1024);
	    lgh->lgh_checksum = *save_checksum;
	}
    }
    if (BTtest(LG_T_HDRFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (!write_primary)
	{
	    if (lbb->lbb_lga.la_offset == 0)
	    {
		lgh = (LG_HEADER *)lbh;
		lgh->lgh_checksum++;	/* scramble first copy */
		lgh = (LG_HEADER *)(((char *)lbh) + 1024);
		lgh->lgh_checksum++;	/* scramble second copy */
	    }
	}
    }
    if (BTtest(LG_T_HDR_PARTIAL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (!write_primary)
	{
	    if (lbb->lbb_lga.la_offset == 0)
	    {
		*size = 64;	    /* only 1st 64 bytes of header get written */
		LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN,
				LGD_START_SHUTDOWN,
				FALSE);
	    }
	}
    }
    return;
}

/*
** Name: LG_check_write_complete_test	- Handle simulated failure in I/O
**
** Description:
**	This routine is called by write_complete to see if this I/O should
**	have a simulated I/O failure (forced by setting the IOSB to an
**	obviously bad I/O completion status.
**
** Inputs:
**	lgd	    - the LGD
**	lbb	    - the buffer being written
**
** Outputs:
**	lbb	    - may be intentionally corrupted.
**
** History:
**	20-sep-1990 (bryanp)
**	    Created.
*/
VOID
LG_check_write_complete_test(
LGD	    *lgd,
LBB	    *lbb)
{
    i4	block_distance;
    LFB		*lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);

    block_distance = ((lbb->lbb_lga.la_offset >> 9) - lgd->lgd_test_badblk);
    if (block_distance < 0)
	block_distance = 0 - block_distance;

    if (BTtest(LG_T_IOSBFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_io_status = LG_WRITEERROR;
	    BTclear(LG_T_IOSBFAIL_PRIMARY,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_IOSBFAIL_BOTH,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_io_status = LG_WRITEERROR;
	    if (lbb->lbb_resume_cnt == 1)
		BTclear(LG_T_IOSBFAIL_BOTH,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_IOSBFAIL_HDRPRIM,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    lbb->lbb_io_status = LG_WRITEERROR;
	    BTclear(LG_T_IOSBFAIL_HDRPRIM,(char *)lgd->lgd_test_bitvec);
	}
    }
    return;
}

/*
** Name: LG_check_dual_complete_test	- Handle simulated failure in I/O
**
** Description:
**	This routine is called by dual_write_complete to see if this I/O should
**	have a simulated I/O failure (forced by setting the IOSB to an
**	obviously bad I/O completion status.
**
** Inputs:
**	lgd	    - the LGD
**	lbb	    - the buffer being written
**
** Outputs:
**	lbb	    - may be intentionally corrupted.
**
** History:
**	20-sep-1990 (bryanp)
**	    Created.
*/
VOID
LG_check_dual_complete_test(
LGD	    *lgd,
LBB	    *lbb)
{
    i4	block_distance;
    LFB		*lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);

    block_distance = ((lbb->lbb_lga.la_offset >> 9) - lgd->lgd_test_badblk);
    if (block_distance < 0)
	block_distance = 0 - block_distance;

    if (BTtest(LG_T_IOSBFAIL_DUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_dual_io_status = LG_WRITEERROR;
	    BTclear(LG_T_IOSBFAIL_DUAL,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_IOSBFAIL_BOTH,(char *)lgd->lgd_test_bitvec))
    {
	if (block_distance < (lfb->lfb_header.lgh_size / VMS_BLOCKSIZE) )
	{
	    lbb->lbb_dual_io_status = LG_WRITEERROR;
	    if (lbb->lbb_resume_cnt == 1)
		BTclear(LG_T_IOSBFAIL_BOTH,(char *)lgd->lgd_test_bitvec);
	}
    }
    if (BTtest(LG_T_IOSBFAIL_HDRDUAL,(char *)lgd->lgd_test_bitvec))
    {
	if (lbb->lbb_lga.la_offset == 0)
	{
	    lbb->lbb_dual_io_status = LG_WRITEERROR;
	    BTclear(LG_T_IOSBFAIL_HDRDUAL,(char *)lgd->lgd_test_bitvec);
	}
    }
    return;
}
#endif /* LG_DUAL_LOGGING_TESTBED */
