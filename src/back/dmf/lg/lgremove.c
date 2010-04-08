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
#include    <pc.h>
#include    <er.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGREMOVE.C - Implements the LGremove function of the logging system
**
**  Description:
**	This module contains the code which implements LGremove.
**
**	External Routines:
**
**	    LGremove -- Remove a database from the logging system
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
**	26-jul-1993 (bryanp)
**	    If the database was added for remote log processing, don't signal a
**		local close when the database is removed.
**	    Clear the CL_ERR_DESC so that error messages are reasonable.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	18-oct-1993 (rogerk)
**	    Fix arguments to ule_format call.
**	    Moved LG_remove prototype to lgdef.h and made the routine non-
**	    static so it can be called from LG_end_transaction.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions. Many new semaphores
**	    added to augment lone lgd_mutex.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	16-Oct-1996 (jenjo02)
**	    Release ldb_mutex before calling LG_signal_check() to avoid
**	    deadlocking with LG_archive_complete().
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed 
**          for support of logs > 2 gig
**	14-Mar-2000 (jenjo02)
**	    Removed static LG_remove() function and the unnecessary level
**	    of indirection it induced.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
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

/*
** Forward declarations of static functions:
*/

/*{
** Name: LGremove	- Remove database.
**
** Description:
**      This routine deletes a database description from the logging
**	system.  The calling process is stating that it will not be
**	logging to this database anymore.  When the last process deletes
**	a database the description is removed from the log system data
**	structures.  The flag value LG_RCP is used to inform the LG
**	code that special actions taken by the client now allow
**	the database to be removed.
**
**	When a transaction aborts abnormally, the database that it was
**	processing against is also in a state of flux.  Until the transaction
**	can be resolved, the database information must be maintained.  When
**	all aborted transactions have been resolved the setting of the LG_RCP
**	flag directs LG to delete information about the process that this
**	transaction was performed from.
**
** Inputs:
**      db_id                           Database identifier.
**
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    If the database was added for remote log processing, don't signal a
**		local close when the database is removed.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new journal log address fields.
**	18-oct-1993 (rogerk)
**	    Fix arguments to ule_format call.
**	25-Jan-1996 (jenjo02)
**	    Multiple new mutexes.
**	16-Oct-1996 (jenjo02)
**	    Release ldb_mutex before calling LG_signal_check() to avoid
**	    deadlocking with LG_archive_complete().
**	01-Nov-2006 (jonj)
**	    Using consistent ldb_q, ldb mutex ordering throughout the
**	    code means we can take and hold the ldb_q and mutex LDBs
**	    without deadlock danger.
*/
STATUS
LGremove(
LG_DBID		    external_db_id,
CL_ERR_DESC	    *sys_err)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LDB	*ldb;
    register LFB	*lfb;
    register LPD	*lpd;
    register LPB	*lpb;
    LPD			*next_lpd;
    LPD			*prev_lpd;
    LDB			*next_ldb;
    LDB			*prev_ldb;
    SIZE_TYPE		*lpdb_table;
    i4			err_code;
    STATUS		status;
    LG_I4ID_TO_ID	db_id;

    LG_WHERE("LGremove")

    CL_CLEAR_ERR(sys_err);
  
    /*	Check the db_id. */

    db_id.id_i4id = external_db_id;
    if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
    {
	uleFormat(NULL, E_DMA41A_LGREM_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count);
	return (LG_BADPARAM);
    }

    lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

    if (lpd->lpd_type != LPD_TYPE ||
	lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
    {
	uleFormat(NULL, E_DMA41B_LGREM_BAD_DB, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
		    0, db_id.id_lgid.id_instance);
	return (LG_BADPARAM);
    }

    /*	Check for outstanding transactions. */

    if (lpd->lpd_lxb_count)
    {
	uleFormat(NULL, E_DMA41C_LGREM_DB_ACTIVE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lpd->lpd_id.id_id, 0, lpd->lpd_id.id_instance,
		    0, lpd->lpd_lxb_count);
	return (LG_BADPARAM);
    }

    /*	Save some interesting pointers. */

    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	return(status);

    /*
    ** If this is the Checkpoint Process exiting, make sure we have
    ** cleaned up any backup processing state.
    */
    if ((lpb->lpb_status & LPB_CKPDB) && (ldb->ldb_status & LDB_BACKUP))
	LG_cleanup_checkpoint(lpb);

    /*	Remove LPD from LPB queue. */
    lpd->lpd_type = 0;

    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_next);
    prev_lpd  = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_prev);
    next_lpd->lpd_prev = lpd->lpd_prev;
    prev_lpd->lpd_next = lpd->lpd_next;

    /*
    ** Deallocating the LPD decrements lgd_lpd_inuse
    */
    LG_deallocate_cb(LPD_TYPE, (PTR)lpd);

    /*	Change various counters. */

    lpb->lpb_lpd_count--;

    (VOID)LG_unmutex(&lpb->lpb_mutex);

    /*
    ** When both the lgd_ldb_q and ldb must be mutexed, always take
    ** the lgd_ldb_q_mutex, then ldb_mutex.
    */

    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
	return(status);

    if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	return(status);

    /*	Check LDB reference count and journaling status. */

    if (--ldb->ldb_lpd_count == 0)
    {
	if ((ldb->ldb_status & LDB_NOTDB) == 0)
	{
	    if (ldb->ldb_status & LDB_JOURNAL &&
		ldb->ldb_j_first_la.la_block)
	    {
		ldb->ldb_status |= LDB_PURGE;
		(VOID)LG_unmutex(&ldb->ldb_mutex);
		ldb = (LDB *)NULL;
		LG_signal_check();
	    }
	    else
	    {
		if ((lfb->lfb_status & LFB_USE_DIIO) == 0)
		{
		    ldb->ldb_status |= LDB_CLOSEDB_PEND;
		    (VOID)LG_unmutex(&ldb->ldb_mutex);
		    ldb = (LDB *)NULL;
		    LG_signal_event(LGD_CLOSEDB, 0, FALSE);
		}
		else
		{
		    /*  Remove LDB from active queue. */

		    next_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_next);
		    prev_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_prev);
		    next_ldb->ldb_prev = ldb->ldb_prev;
		    prev_ldb->ldb_next = ldb->ldb_next;

		    /*
		    ** Deallocating the LDB decrements the inuse count
		    ** and frees the ldb_mutex.
		    */
		    LG_deallocate_cb(LDB_TYPE, (PTR)ldb);
		    /*
		    ** Note that the LDB has been vaporised
		    */
		    ldb = (LDB *)NULL;

		    /*  Change various counters. */

		    if (lgd->lgd_ldb_inuse == 1 &&
			(lgd->lgd_status & LGD_START_SHUTDOWN))
		    {
			LG_signal_event(LGD_ACP_SHUTDOWN, 0, FALSE);
		    }
		}
	    }
	}
    }

    if (ldb)
	(VOID)LG_unmutex(&ldb->ldb_mutex);

    (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);

    /*	Change various counters. */

    lgd->lgd_stat.remove++;

    return (OK);
}
