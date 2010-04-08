/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <id.h>
#include    <tm.h>
#include    <cv.h>
#include    <pc.h>
#include    <cm.h>
#include    <st.h>
#include    <nm.h>
#include    <sl.h>
#include    <cs.h>
#include    <ck.h>
#include    <jf.h>
#include    <tr.h>
#include    <si.h>
#include    <lo.h>
#include    <er.h>
#include    <ex.h>
#include    <pm.h>
#include    <me.h>
#include    <di.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <cui.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dm0m.h>
#include    <sxf.h>
#include    <ulx.h>
#include    <dma.h>
#include    <dmxe.h>
#include    <dmpp.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2r.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm0j.h>
#include    <dmfjsp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0s.h>
#include    <dmfinit.h>
#include    <dm0d.h>

/**
**
**  Name: DMRFPSUP.C - DMF Rollforward Support Utilities
**
**  Description:
**
**	This file contains support utilities for the partial backup
**	and recovery project to handle transaction processing
**	on system catalogs such as iirelation and iirel_idx.
**
**	dmf_rfp_abort_transaction	- Abort a Transaction
**	dmf_rfp_commit_transaction	- Commit a Transaction
**	dmf_rfp_begin_transaction	- Begin a Transaction
**
** History:
**	16-nov-1994 (andyw)
**	    Created for Partial Backup/Recovery Project
**	    to support transaction requests
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_?, dmf_rfp_? functions converted to DB_ERROR *
*/

/*
**  Definition of static variables and forward static functions.
*/
static  DML_XCB     dmf_rfp_xcb;
/*
** Prototype Declarations
*/

DB_STATUS dmf_rfp_abort_transaction(
				    DML_ODCB            *odcb,
				    DMP_DCB             *dcb,
				    DB_TRAN_ID          *tran_id,
				    i4             log_id,
				    i4             lock_id,
				    i4             flag,
				    i4             sp_id,
				    LG_LSN              *savepoint_lsn,
				    DB_ERROR            *dberr);

DB_STATUS dmf_rfp_begin_transaction(
				    i4             mode,
				    i4             flags,
				    DMP_DCB             *dcb,
				    i4             dcb_id,
				    DB_OWN_NAME         *user_name,
				    i4             related_lock,
				    i4             *log_id,
				    DB_TRAN_ID          *tran_id,
				    i4             *lock_id,
				    DB_ERROR            *dberr);

DB_STATUS dmf_rfp_commit_transaction(
				    DB_TRAN_ID          *tran_id,
				    i4             log_id,
				    i4             lock_id,
				    i4             flag,
				    DB_TAB_TIMESTAMP    *ctime,
				    DB_ERROR            *dberr);


/*
** Name: dmf_rfp_abort_transaction() - Abort an RFP transaction
**
** Description:
**	This utility aborts a transaction which has been handled
**	by the Rollforward process in table level mode
**	This is basically a wrapper to the dmxe_abort() function 
**
** Inputs:
**      odcb                      - Address of this session's Open Database
**                                  Control Block. If dmxe_abort is
**                                  being called outside of session
**                                  context, and no ODCB is available,
**                                  this pointer may be null.
**      dcb                       - Address of the Database Control Block,
**                                  if known. If not known, pass NULL.
**      tran_id                   - The transaction id to commit.
**      log_id                    - The log_id to use.
**      lock_id                   - The lock_id to use.
**      flag                      - Special flags: DMXE_JOURNAL | DMXE_ROTRAN |
**				    DMXE_WILLING_COMMIT.
**      sp_id                     - The savepoint id to look for.
**      savepoint_lsn             - The Log Sequence Number of the
**                                  savepoint to which we are aborting 
**
**
** Outputs:
**      err_code                - Detailed error message if error occurs.
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      16-nov-1994 (andyw)
**	    Created for partial backup/recovery project
*/
DB_STATUS
dmf_rfp_abort_transaction(
DML_ODCB            *odcb,
DMP_DCB             *dcb,
DB_TRAN_ID          *tran_id,
i4             log_id,
i4             lock_id,
i4             flag,
i4             sp_id,
LG_LSN              *savepoint_lsn,
DB_ERROR             *dberr)
{
    DB_STATUS status;
 
    /* Call DMXE to perform abort */
    status = dmxe_abort(odcb, dcb, tran_id, log_id, lock_id, flag,
                	sp_id, savepoint_lsn, dberr);
 
    /* Update local transaction info */
    dmf_rfp_xcb.xcb_state=XCB_FREE;
    dmf_rfp_xcb.xcb_x_type= 0;
 
    return status;
}

/*
** Name: dmf_rfp_commit_transaction() - Commit a RFP transaction
**
** Description:
**      This utility commits a transaction which has been handled
**      by the Rollforward process in table level mode
**      This is basically a wrapper to the dmxe_commit() function
**
** Inputs:
**      tran_id                 - The transaction id to commit.
**      log_id                  - The log_id to use.
**      lock_id                 - The lock_id to use.
**      flag                    - Special flags.
**                                DMXE_JOURNAL - journaled transaction.
**                                DMXE_ROTRAN - readonly transaction.
**                                DMXE_WILLING_COMMIT - commit a willing
**                                commit transaction.
**      ctime                   - Commit time.
**
**
** Outputs:
**      err_code                - Detailed error message if error occurs.
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      16-nov-1994 (andyw)
**          Created for partial backup/recovery project
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
*/
DB_STATUS
dmf_rfp_commit_transaction(
DB_TRAN_ID          *tran_id,
i4             log_id,
i4             lock_id,
i4             flag,
DB_TAB_TIMESTAMP    *ctime,
DB_ERROR             *dberr)
{
        DB_STATUS status;
 
        status = dmxe_commit (tran_id, log_id, lock_id, flag, ctime, dberr);
 
        /* Update local transaction info */
        dmf_rfp_xcb.xcb_state=XCB_FREE;
        dmf_rfp_xcb.xcb_x_type=0;
        return status;
}

/*
** Name: dmf_rfp_begin_transaction() - Begin a RFP transaction
**
** Description:
**      This utility begins a transaction which has been handled
**      by the Rollforward process in table level mode
**      This is basically a wrapper to the dmxe_commit() function
**
** Inputs:
**      mode                    - The mode of the transaction. Must be
**                                DMXE_READ, DMXE_UPDATE.
**      flags                   - From the following:
**                                DMXE_INTERNAL - local transaction
**                                DMXE_JOURNAL - tran is journaled
**                                DMXE_DELAYBT - don't write begin
**                                   transaction record. It will be
**                                   written when the first write
**                                   operation is performed.
**      dcb                     - dcb pointer.
**      dcb_id                  - The logging system database id.
**      user_name               - The user name of the session.
**      related_lock            - The related lock list.
**      tran_id                 - Optional distributed transaction id.
**
**
** Outputs:
**      tran_id                 - The assigned transaction_id.
**      log_id                  - The logging system id.
**      lock_id                 - The assigned lock id.
**      err_code                - The reason for error status.
**
** Side Effects:
**	None:
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      16-nov-1994 (andyw)
**          Created for partial backup/recovery project
*/
DB_STATUS
dmf_rfp_begin_transaction(
i4             mode,
i4             flags,
DMP_DCB             *dcb,
i4             dcb_id,
DB_OWN_NAME         *user_name,
i4             related_lock,
i4             *log_id,
DB_TRAN_ID          *tran_id,
i4             *lock_id,
DB_ERROR             *dberr)
{
        DB_STATUS status;

        status = dmxe_begin(mode, flags, dcb, dcb_id, user_name, related_lock,
                	    log_id, tran_id, lock_id, 
			    (DB_DIS_TRAN_ID *)NULL, dberr);
 
        if (status == E_DB_OK)
        {
                /* Update transaction info */
                dmf_rfp_xcb.xcb_tran_id= *tran_id;
                dmf_rfp_xcb.xcb_log_id= *log_id;
                dmf_rfp_xcb.xcb_lk_id = *lock_id;
                dmf_rfp_xcb.xcb_x_type=XCB_RONLY;
        }
        return status;
}
