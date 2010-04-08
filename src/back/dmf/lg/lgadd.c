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
#include    <cx.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGADD.C - Implements the LGadd function of the logging system
**
**  Description:
**	This module contains the code which implements LGadd.
**	
**	    LGadd -- add a database to the logging system
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (rogerk)
**	    Added CL_CLEAR_ERR call.
**	    Removed LG_WILLING_COMMIT flag - now only LG_ADDONLY is used
**	    during recovery processing.  Add ldb_j_last_la, ldb_d_last_la
**	    fields.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed LG_ADDONLY flag.  Recovery processing now adds db with
**		a normal LGadd call and alters it via LG_A_DBCONTEXT to
**		reestablish its context.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp/andys)
**	    Cluster 6.5 Project I
**		Renamed stucture members of LG_LA to new names. This means
**		    replacing lga_high with la_sequence, and lga_low with
**		    la_offset.
**		Add ldb_sback_lsn field to the LDB.
**		Make sure that lpd_type is set so that LPDs can be deallocated
**		    properly upon error.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    When adding a database which is associated with a remote log file,
**		do not signal a local opening of the database.
**	    Include <tr.h>
**	    When adding the notdb again, increment the ldb_lpd_count even if
**		the ldb_buffer info doesn't match. The notdb is always the notdb
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in LG_add().
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Calls to LKMUTEX.C must now
**	    explicitly name the semaphore. Added several new
**	    semaphores to reduce blocking on single lgd_mutex.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	30-Jan-1996 (jenjo02)
**	    Reorganized LG_add() such that if NOTDB is wanted, 
**	    the search of the ldb queue is bypassed; after all,
**	    we know it's buried in the LGD and easy to find.
**	11-Sep-1996 (jenjo02)
**	    Fix a bug in LG_add() search of lgd_ldb_q which was looping
**	    if more that 2 LDBs were extant.
** 	13-jun-1997 (wonst02)
** 	    Added LG_READONLY and LDB_READONLY for readonly databases.
**	12-nov-1998 (kitch01)
**	    (hanje04) X-Integ of change 439570
**		Bug 90140. Add LDB_CLOSE_WAIT flag to wait the opening of a
**		database until its close is complete.
**	08-Dec-1997 (hanch04)
**	    Initialize new block field in LG_LA for support of logs > 2 gig
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. initialize the ldb_eback_lsn.
**	14-Mar-2000 (jenjo02)
**	    Removed static LG_add() function and the unnecessary level
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
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      05-may-2005 (stial01)
**          LGadd() get private LDB when flag LG_CSP_LDB.
**      02-jun-2005 (stial01)
**          Make previous changes cluster specific, changed TRdisplay
**      13-jun-2005 (stial01)
**          Modified previous changes - compare node name in lfb, ldb_lfb
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
*/

/*
** Forward declarations for static functions:
*/

/*{
** Name: LGadd	- Add Database.
**
** Description:
**	Add database to logging system for a process.
**
**      This routine adds a database to the logging system.  This service
**	is used to inform the logging system that records recorded in the log
**	file should be associated with this database.  A database can be 
**	marked as journaled by setting the LG_JOURNAL flag.  The fact that a
**	database is journaled is used by the logging system to recognize the
**	need to copy log records from the log file to a journal file.
**
** NOTE on adding databases that are being recovered:
**	When a database requires REDO recovery, the LDB for that database
**	is marked LDB_RECOVER.  This routine will return LG_DB_INCONSISTENT
**	(signifying that the database is inconsistent) if anyone tries
**	to add the db while it is being recovered.  This is not a very
**	on-line solution.
**
**	A better solution is to make sure that servers that want to open
**	a database that is currently being recovered are forced to wait
**	until the db is fully recovered, then they should be able to
**	proceed.
**
** Inputs:
**      lg_id                           Log identifier.
**	flag				Zero or
**					    LG_JOURNAL: if a journaled DB.
** 					    LG_NOTDB: not a DB; administrative
** 					    LG_PRETEND_CONSISTENT: used by verifydb
** 					    LG_FCT: fast commit
** 					    LG_READONLY: a readonly database
**	buffer				Database information buffer.
**      l_buffer                        Length of buffer.
**
** Outputs:
**      db_id                           Database identifier. Unique
**                                      identifier associated with this
**                                      instantiation of the logging/locking
**                                      server.  After logging/locking 
**                                      restarted, a database can have
**                                      a different id.
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK				Success.
**	    LG_BADPARAM			Bad parameters to call.
**	    LG_DB_INCONSISTENT		Inconsistent database.
**	    LG_EXCEED_LIMIT		Out of LDB's.
**	    LG_SHUTTING_DOWN		Shutdown has occured (or pending).
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (rogerk)
**	    Removed LG_WILLING_COMMIT flag - now only LG_ADDONLY is used
**	    during recovery processing.  Add ldb_j_last_la, ldb_d_last_la
**	    fields.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed LG_ADDONLY flag.  Recovery processing now adds db with
**		a normal LGadd call and alters it via LG_A_DBCONTEXT to
**		reestablish its context.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Add ldb_sback_lsn field to the LDB.
**		Make sure that lpd_type is set so that LPDs can be deallocated
**		    properly upon error.
**	26-jul-1993 (bryanp)
**	    When adding a database which is associated with a remote log file,
**		do not signal a local opening of the database. This occurs
**		when the CSP process on one node is recovering the work
**		performed by another node; in this case we do NOT wish to
**		signal to the RCP that a local open is being performed, since
**		in fact no local access is implied by adding this database.
**	    When adding the notdb again, increment the ldb_lpd_count even if
**		the ldb_buffer info doesn't match. The notdb is always the notdb
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new journal and dump log address fields.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	30-Jan-1996 (jenjo02)
**	    Reorganized LG_add() such that if NOTDB is wanted, 
**	    the search of the ldb queue is bypassed; after all,
**	    we know it's buried in the LGD and easy to find.
**	11-Sep-1996 (jenjo02)
**	    Fix a bug in LG_add() search of lgd_ldb_q which was looping
**	    if more that 2 LDBs were extant.
** 	13-jun-1997 (wonst02)
** 	    Added LG_READONLY and LDB_READONLY for readonly databases.
**	12-nov-1998 (kitch01)
**		Bug 90140. If the database is currently pending a close then
**		mark the open as in CLOSE_WAIT. This will ensure that the close
**		is processed before this open and prevent locking errors on the journals
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
        21-Jun-2006 (hanal04) Bug 116272
**          Take the lgd_mutex before the ldb_mutex in order to ensure
**          the acquisition order is consistent with LG_archive_complete()
**          and LG_event(). Flag LG_signal_event() that we already have the
**          lgd_mutex.
**	01-Nov-2006 (jonj)
**	    Use consistent ldb_q_mutex, ldb_mutex ordering thoughout the code.
**	    Don't put LDB on queue until it's completely initialized.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Initialize new ldb_active_lxbq.
*/
STATUS
LGadd(
LG_LGID             external_lg_id,
i4		    flag,
char		    *buffer,
i4		    l_buffer,
LG_DBID		    *external_db_id,
CL_ERR_DESC	    *sys_err)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB	*lpb;
    register LDB	*ldb;
    register LFB	*lfb;
    register LPD	*lpd;
    LDB			*next_ldb;
    LPD			*next_lpd;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		ldb_offset;
    SIZE_TYPE		*lpbb_table;
    SIZE_TYPE		*ldbb_table;
    i4			err_code;
    bool		initialize_ldb = FALSE;
    STATUS		status;
    LG_I4ID_TO_ID	lg_id;
    LG_ID		*db_id = (LG_ID*)external_db_id;
    LFB			*cur_db_lfb;
    i4			SignalEvent = 0;

    /*
    ** If the logging system is already in "shutdown" mode, then no new
    ** LGadd calls are permitted
    */
    LG_WHERE("LGadd")

    CL_CLEAR_ERR(sys_err);

    if ((lgd->lgd_status & (LGD_START_SHUTDOWN | LGD_IMM_SHUTDOWN)) != 0)
	return (LG_SHUTTING_DOWN);

    if (l_buffer == 0 ||
	l_buffer > sizeof(ldb->ldb_buffer))
    {
	uleFormat(NULL, E_DMA411_LGADD_BAD_LEN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, l_buffer, 0, sizeof(ldb->ldb_buffer));
	return (LG_BADPARAM);
    }

    /*	Check the lg_id. */

    lg_id.id_i4id = external_lg_id;
    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
    {
	uleFormat(NULL, E_DMA40F_LGADD_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count);
	return (LG_BADPARAM);
    }

    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

    if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	return(status);

    if (lpb->lpb_type != LPB_TYPE ||
	lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
    {
	(VOID)LG_unmutex(&lpb->lpb_mutex);
	uleFormat(NULL, E_DMA410_LGADD_BAD_PROC, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
		    0, lg_id.id_lgid.id_instance);
	return (LG_BADPARAM);
    }

    /*
    **  Allocate an LPD, causing lpd_type to be set to LPD_TYPE.
    */

    if ((lpd = (LPD *)LG_allocate_cb(LPD_TYPE)) == 0) 
    {
	(VOID)LG_unmutex(&lpb->lpb_mutex);
	return (LG_EXCEED_LIMIT);
    }

    /*
    ** CLEANUP: error returns after this point must free the lpd before
    **		returning! 
    */

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    /*
    ** If this isn't a real user database, but is instead the "NOTDB"
    ** database which is used by system processes such as the DMFRCP and
    ** DMFACP daemons, then it has a special reserved LDB slot and does not
    ** get located by its database information buffer, therefore we
    ** can skip locking and scanning the ldb queue.
    */
    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);

    /*
    ** When both the lgd_ldb_q and ldb must be mutexed, always take
    ** the lgd_ldb_q_mutex, then ldb_mutex.
    */

    /* Lock and hold the ldb queue mutex */
    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
	return(status);

    if (flag & LG_NOTDB)
    {
	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[1]);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	/*
	** IF the notdb has already been initialized, then we have some
	** caller who is adding the notdb with a different buffer, thus
	** we didn't match when we searched the database list for a
	** matching ldb_buffer field. Since we really don't care about the
	** ldb_buffer for the notdb (the notdb is the notdb, after all),
	** we'll treat this case as though the ldb buffer fields matched.
	*/
	if (ldb->ldb_type == LDB_TYPE)
	{
	    /*  Count new reference to LDB. */
	    ldb->ldb_lpd_count++;
	}
	else
	{
	    /*
	    ** first use of NOTDB; initialize it.
	    */
	    lgd->lgd_ldb_inuse++;
	    initialize_ldb = TRUE;
	}
    }
    else
    {
	/*
	** Scan database list to see if this database is already known. Each
	** database is identified by a "database information buffer", which DMF
	** passes in. This buffer contains items such as the database name, owner
	** name, etc. If the database information buffer passed to LGadd exactly
	** matches the database information buffer of an existing LDB, then this
	** database is already known (has already been added by another logging
	** system process).
	*/

	for (ldb_offset = lgd->lgd_ldb_next;
	    ldb_offset != end_offset;)
	{
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);

	    if (ldb->ldb_l_buffer != l_buffer ||
		MEcmp(ldb->ldb_buffer, buffer, l_buffer))
	    {
		ldb_offset = ldb->ldb_next;
		continue;
	    }

	    if ( CXcluster_enabled() )
	    {
	        /*
		** Node recovery must use distinct ldb context per node log file
		*/
		cur_db_lfb = (LFB *)LGK_PTR_FROM_OFFSET(ldb->ldb_lfb_offset);

		if ((lfb->lfb_l_nodename || cur_db_lfb->lfb_l_nodename) &&
		    (lfb->lfb_l_nodename != cur_db_lfb->lfb_l_nodename ||
		    MEcmp(lfb->lfb_nodename, cur_db_lfb->lfb_nodename,
				    lfb->lfb_l_nodename)))
		{
		    ldb_offset = ldb->ldb_next;
#ifdef xDEBUG
		    TRdisplay("%@ RCP-P1: Recovering %~t, ignore ldb for %~t %x\n",
			lfb->lfb_l_nodename, lfb->lfb_nodename,
			cur_db_lfb->lfb_l_nodename, cur_db_lfb->lfb_nodename,
			flag & LG_CSP_RECOVER);
#endif

		    continue;
		}
	    }
	    
	    if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
		return(status);

	    /*
	    ** Check again after semaphore wait.
	    ** If LDB is no longer a match (it was in the 
	    ** process of being eradicated while we waited for
	    ** the ldb_mutex), and start the search again from
	    ** the top of the queue.
	    */
	    if (ldb->ldb_type != LDB_TYPE ||
	        ldb->ldb_l_buffer != l_buffer ||
		MEcmp(ldb->ldb_buffer, buffer, l_buffer))
	    {
		(VOID)LG_unmutex(&ldb->ldb_mutex);
		ldb_offset = lgd->lgd_ldb_next;
		continue;
	    }
	    break;
	}

	if (ldb_offset != end_offset)
	{
	    /*
	    **  LDB exists. If the database is already known to be inconsistent,
	    **  then no new adds of the database are permitted, unless the caller
	    **  acknowledges that it "knows" that the database is inconsistent by
	    **  passing the "pretend consistent" flag (used by verifydb).
	    */

	    if (ldb->ldb_status & LDB_INVALID)
	    {
		if ( (flag & LG_PRETEND_CONSISTENT) == 0 )
		{
		    (VOID)LG_unmutex(&ldb->ldb_mutex);
		    (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);
		    LG_deallocate_cb(LPD_TYPE, (PTR)lpd);
		    (VOID)LG_unmutex(&lpb->lpb_mutex);
		    return (LG_DB_INCONSISTENT);	
		}
	    }

	    /*
	    ** If the database reference count is zero, then the database
	    ** must be opened by the RCP before the server can use it.
	    ** Mark the status opendb_pending - this will suspend any thread
	    ** making an LGwrite call on this database (note that the first
	    ** thing a server does after opening a database is to write an
	    ** OPENDB log record) until the RCP has finished opening it.
	    **
	    ** If the database reference count is not zero, but the database
	    ** is undergoing REDO recovery, then we cannot allow new servers
	    ** to access the database until recovery is complete.  Set the
	    ** database status to opendb_pending and opn_wait.
	    **
	    ** NOTE that if we begin to support READ-ONLY databases and servers
	    ** are able to open databases without writing an OPENDB record, then
	    ** we must come up with a new method of suspending database openers
	    ** until recovery is complete.
	    */
		/* Bug 90140. If the database is currently pending a close then
		** mark the open as in CLOSE_WAIT. This will ensure that the close
		** is processed before this open and prevent locking errors on the journals
		*/
	    if (ldb->ldb_lpd_count == 0)
	    {
		if ((ldb->ldb_status & LDB_PURGE) == 0)
		{
		    if ((ldb->ldb_status & LDB_OPENDB_PEND) == 0)
		    {
			ldb->ldb_status |= LDB_OPENDB_PEND;
			if (ldb->ldb_status & LDB_CLOSEDB_PEND)
				ldb->ldb_status |= LDB_CLOSE_WAIT;
			if (flag & LG_PRETEND_CONSISTENT)
			    ldb->ldb_status |= LDB_PRETEND_CONSISTENT;
	    		if (flag & LG_READONLY)
			    ldb->ldb_status |= LDB_READONLY;
			SignalEvent = LGD_OPENDB;
		    }
		}
		else
		    ldb->ldb_status &= ~(LDB_PURGE);
	    }
	    else if (ldb->ldb_status & LDB_RECOVER)
	    {
		/*
		** The database is open, but is being recovered.
		** Set the opendb_pending and opn_wait flags - this will 
		** prevent any new transactions from proceeding on this 
		** database until recovery is complete.  Marking this
		** database as OPENDB_PEND will not cause the database to
		** be processed in count_opens because of the opn_wait flag.
		*/
		ldb->ldb_status |= (LDB_OPENDB_PEND | LDB_OPN_WAIT);
	    }
	    /*  Count new reference to LDB. */

	    ldb->ldb_lpd_count++;
	}
	else
	{
	    /*
	    ** This database is NOT known.
	    **
	    ** If the caller has passed special flags indicating that they
	    ** require that the newly-added database must have a particular DB_ID
	    ** assigned to it, then ensure that the new LDB gets the right ID.
	    **
	    ** Otherwise, just pick the next LDB off the free list.

	    */
	    /*
	    **  Allocate a new LDB 
	    **  returning with the ldb_mutex held
	    **  and lgd_ldb_inuse incremented.
	    */
	    if ((ldb = (LDB *)LG_allocate_cb(LDB_TYPE)) == 0)
	    {
		LG_deallocate_cb(LPD_TYPE, (PTR)lpd);
		(VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);
		(VOID)LG_unmutex(&lpb->lpb_mutex);
		return (LG_EXCEED_LIMIT);
	    }
	    initialize_ldb = TRUE;
	}
    }

#ifdef xDEBUG
    /*
    ** For a while, we were having problems with corruption of the LFB/LDB
    ** large block queues, and this debugging code helped to track those
    ** problems down.
    */
    if (ldb->ldb_id.id_id == 0)
    {
	TRdisplay("%@ LGadd: args were:(%d,%d).%x.%p.%x.%p\n",
		lg_id.id_lgid.id_id, lg_id.id_lgid.id_instance,
		flag, buffer, l_buffer, db_id);
	LG_debug_wacky_ldb_found(lgd, ldb);
	return (LG_BADPARAM);
    }
#endif

    /*
    ** NOTE: Be careful about adding error returns after this point,
    ** because any such error return must first free up BOTH the LPD AND
    ** the LDB, if an LDB was actually allocated.
    */

    /*
    **  Initialize the LDB, if one was allocated
    **  or if first use of NOTDB LDB.
    */
    if (initialize_ldb)
    {
	MEcopy((PTR)buffer, l_buffer, (PTR)ldb->ldb_buffer);
	ldb->ldb_l_buffer = l_buffer;
	ldb->ldb_type = LDB_TYPE;
	ldb->ldb_status = LDB_ACTIVE;
	ldb->ldb_stat.read = 0;
	ldb->ldb_stat.write = 0;
	ldb->ldb_stat.begin = 0;
	ldb->ldb_stat.wait = 0;
	ldb->ldb_stat.force = 0;
	ldb->ldb_stat.end = 0;
	ldb->ldb_lxbo_count = 0;
	ldb->ldb_lxb_count = 0;
	ldb->ldb_lpd_count = 1;
	ldb->ldb_lfb_offset = lpb->lpb_lfb_offset;
	ldb->ldb_j_first_la.la_sequence = 0;
	ldb->ldb_j_first_la.la_block    = 0;
	ldb->ldb_j_first_la.la_offset   = 0;
	ldb->ldb_j_last_la.la_sequence  = 0;
	ldb->ldb_j_last_la.la_block     = 0;
	ldb->ldb_j_last_la.la_offset    = 0;
	ldb->ldb_d_first_la.la_sequence = 0;
	ldb->ldb_d_first_la.la_block    = 0;
	ldb->ldb_d_first_la.la_offset   = 0;
	ldb->ldb_d_last_la.la_sequence  = 0;
	ldb->ldb_d_last_la.la_block     = 0;
	ldb->ldb_d_last_la.la_offset    = 0;
	ldb->ldb_sbackup.la_sequence    = 0;
	ldb->ldb_sbackup.la_block       = 0;
	ldb->ldb_sbackup.la_offset      = 0;
	ldb->ldb_sback_lsn.lsn_high     = 0;
	ldb->ldb_sback_lsn.lsn_low      = 0;
	ldb->ldb_eback_lsn.lsn_high     = 0;
	ldb->ldb_eback_lsn.lsn_low      = 0;

	/*
	** Assume no simulated MVCC journal writes.
	**
	** This may be changed by LGalter(LG_A_JFIB)
	*/
	MEfill(sizeof(ldb->ldb_jfib), 0, &ldb->ldb_jfib);

	/*
	** Set last_commit, last_lsn, and first_la to
	** the current values from the header.
	*/
	ldb->ldb_last_commit = lfb->lfb_header.lgh_last_lsn;
	ldb->ldb_last_lsn = lfb->lfb_header.lgh_last_lsn;
	ldb->ldb_first_la = lfb->lfb_header.lgh_end;

	/*
	** Initialize active transaction queue to empty.
	*/
	ldb->ldb_active_lxbq.lxbq_next = ldb->ldb_active_lxbq.lxbq_prev
		= LGK_OFFSET_FROM_PTR(&ldb->ldb_active_lxbq.lxbq_next);

	ldb->ldb_lgid_low = 0;
	ldb->ldb_lgid_high = 0;

	/*
	** Extract the external Database Id from the info buffer to
	** put in an accessable place of the ldb.
	*/
	I4ASSIGN_MACRO(ldb->ldb_buffer[2*DB_MAXNAME], ldb->ldb_database_id);

	if (flag & LG_NOTDB)
	{
	    ldb->ldb_status |= LDB_NOTDB;
	}
	else
	{
	    if (flag & LG_JOURNAL)
		ldb->ldb_status |= LDB_JOURNAL;
	    if (flag & LG_PRETEND_CONSISTENT)
		ldb->ldb_status |= LDB_PRETEND_CONSISTENT;
	    if (flag & LG_READONLY)
		ldb->ldb_status |= LDB_READONLY;
	}

	if ((ldb->ldb_status & LDB_NOTDB) == 0)
	{
	    if ((lfb->lfb_status & LFB_USE_DIIO) == 0)
	    {
		/*
		** signal to the RCP that local use of this database is
		** beginning. The database remains in pending-open state
		** until the RCP acknowledges the open.
		*/
		ldb->ldb_status |= LDB_OPENDB_PEND;
		SignalEvent = LGD_OPENDB;
	    }
	}
    }

    /*
    ** The LPD (Logging system Process-Database connection block) contains
    ** pointers to its associated database and process blocks, and contains
    ** a list of all transactions which this process has begun within this
    ** database:
    */

    lpd->lpd_ldb = LGK_OFFSET_FROM_PTR(ldb);
    lpd->lpd_lpb = LGK_OFFSET_FROM_PTR(lpb);

    lpd->lpd_lxbq.lxbq_next = lpd->lpd_lxbq.lxbq_prev =
	    LGK_OFFSET_FROM_PTR(&lpd->lpd_lxbq.lxbq_next);

    lpd->lpd_lxb_count = 0;

    /*	Change various counters. */

    lpb->lpb_lpd_count++;
    lgd->lgd_stat.add++;

    /*	Queue LPD to the LPB. */

    lpd->lpd_next = lpb->lpb_lpd_next;
    lpd->lpd_prev = LGK_OFFSET_FROM_PTR(&lpb->lpb_lpd_next);

    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpb->lpb_lpd_next);
    next_lpd->lpd_prev    = 
	    lpb->lpb_lpd_next = LGK_OFFSET_FROM_PTR(lpd);

    /*
    ** If the adding process uses fast commit, then mark the database
    ** as open with FC protocols.  Should a crash occur, all updates to
    ** this db since the last Consistency Point will need to be redone.
    */
    if ((flag & LG_FCT) && (lpb->lpb_status & LPB_FCT))
	ldb->ldb_status |= LDB_FAST_COMMIT;

    /* If opener wants MVCC, ensure that it is on, found or not */
    if ( flag & LG_MVCC )
	ldb->ldb_status |= LDB_MVCC;

    /*	Return identifier. */

    *db_id = lpd->lpd_id;

    if ( initialize_ldb )
    {
	/* Lastly, insert LDB on the active queue. */

	ldb->ldb_next = lgd->lgd_ldb_next;
	ldb->ldb_prev = end_offset;
	next_ldb = (LDB *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldb_next);
	next_ldb->ldb_prev    = 
	    lgd->lgd_ldb_next = LGK_OFFSET_FROM_PTR(ldb);
    }

    /*
    ** Unwind the mutexes
    */
    (VOID)LG_unmutex(&ldb->ldb_mutex); 
    (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex); 
    (VOID)LG_unmutex(&lpb->lpb_mutex); 

    /* If any events to signal, do so */
    if ( SignalEvent )
	LG_signal_event(SignalEvent, 0, FALSE);

    return (OK);
}
