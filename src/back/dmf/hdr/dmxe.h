/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMXE.H - Transaction interface definitions.
**
** Description:
**      This file describes the interface to the transaction level
**	services.
**
**      13-dec-1985 (derek)
**          Created new for Jupiter.
**      1-Feb-1989 (ac)
**          Added 2PC support.
**	25-mar-1991 (rogerk)
**	    Added DMXE_NOLOGGING for Set Nologging support.
**	7-Jul-1992 (walt)
**	    Prototyping DMF.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added dmxe_pass_abort routine.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed dmxe_abort output parameter used to return the log address
**		of the abortsave log record.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add DCB argument to dmxe_abort for callers who don't have an
**		    ODCB block handy.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added dmxe_xn_active() prototype
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID * to dmxe_begin prototype.
**      18-jan-2000 (gupsh01)
**          Added flag DMXE_NONJNLTAB to indicate the records which are a 
**          part of transactions involving non-journaled tables for a 
**          journaled database. 
**      01-feb-2001 (stial01)
**          Changed dmxe_xn_active args (Variable Page Type SIR 102955)
**      22-oct-2002 (stial01)
**          Added prototype for dmxe_xa_abort
**      20-sept-2004 (huazh01)
**          changed prototype for dmxe_commit(). 
**          INGSRV2643, b111502.
**	29-Jan-2004 (jenjo02)
**	    Added DMXE_CONNECT flag to dmxe_begin() for
**	    connecting to an existing txn context;
**	    used by QEF partitioning threads.
**	20-Apr-2004 (jenjo02)
**	    Added DMXE_LOGFULL_COMMIT flag for dmxe_commit().
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502, doesn't
**	    apply to r3.
**	26-Jun-2006 (jonj)
**	    Add prototypes for dmxe_xa_disassoc(), dmxe_xa_prepare(),
**	    dmxe_xa_commit(), dmxe_xa_rollback(), flags for
**	    XA_START.
**	01-Aug-2006 (jonj)
**	    Add lock_id to dmxe_xa_disassoc() prototype.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Added jornaled to dmxe_writebt, to indicate
**          whether the BT record is to be journaled.
**	17-May-2007 (kibro01) b118117
**	    Change DMXE flags to make them unique
**	28-Jun-2007 (kibro01) b118559
**	    Add in flag to allow lock release to take place after the
**	    commit, so that transaction temporary tables can keep their
**	    lock until they are destroyed.
**	20-Nov-2008 (jonj)
**	    SIR 120874: Convert functions to return DB_ERROR *dberr 
**	    instead of i4 *err_code.
**/

/*
**  Defines of other constants.
*/

/*
**      Constants used in calls to dmxe_begin() (mode):
*/
#define                 DMXE_READ       1L
#define			DMXE_WRITE	2L

/* Constants for dmxe_begin and dmxe_commit/dmxe_abort (kibro01) b118117 */
/* NOTE: These constants start at 0x10 rather than 0x01 to avoid any chance
** of being confused with DMXE_READ/DMXE_WRITE 
*/
#define                 DMXE_JOURNAL            0x0010L
#define                 DMXE_CONNECT            0x0020L
#define                 DMXE_ROTRAN             0x0040L
#define                 DMXE_INTERNAL           0x0080L
#define                 DMXE_DELAYBT            0x0100L
#define                 DMXE_WILLING_COMMIT     0x0200L
#define                 DMXE_NOLOGGING          0x0400L
#define                 DMXE_NONJNLTAB          0x0800L
#define                 DMXE_LOGFULL_COMMIT     0x1000L
#define         	DMXE_XA_START_XA 	0x2000L
#define         	DMXE_XA_START_JOIN 	0x4000L
#define			DMXE_DELAY_UNLOCK	0x8000L

/*
**  Forward and/or External function references.
*/

    /* Abort transaction */
FUNC_EXTERN DB_STATUS dmxe_abort(
    DML_ODCB		*odcb,
    DMP_DCB		*dcb,
    DB_TRAN_ID		*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    i4		sp_id,
    LG_LSN		*savepoint_lsn,
    DB_ERROR		*dberr);

    /* Begin a transaction. */
FUNC_EXTERN DB_STATUS dmxe_begin(
    i4		mode,
    i4		flags,
    DMP_DCB		*dcb,
    i4		dcb_id,
    DB_OWN_NAME		*user_name,
    i4		related_lock,
    i4		*log_id,
    DB_TRAN_ID		*tran_id,
    i4		*lock_id,
    DB_DIS_TRAN_ID	*dis_tran_id,
    DB_ERROR		*dberr);

    /* Commit a transaction. */
FUNC_EXTERN DB_STATUS dmxe_commit(
    DB_TRAN_ID	*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    DB_TAB_TIMESTAMP    *ctime,
    DB_ERROR		*dberr);

    /* Start a Savepoint. */
FUNC_EXTERN DB_STATUS dmxe_savepoint(
    DML_XCB		*xcb,
    DML_SPCB		*spcb,
    DB_ERROR		*dberr);

    /*  Write begin transaction record for already begun transaction. */
FUNC_EXTERN DB_STATUS dmxe_writebt(
    DML_XCB		*xcb_ptr,
    i4                  journaled,
    DB_ERROR		*dberr);

    /* Prepare to commit a transaction. */
FUNC_EXTERN DB_STATUS dmxe_secure(
    DB_TRAN_ID		*tran_id,
    DB_DIS_TRAN_ID	*dis_tran_id,
    i4		log_id,
    i4		lock_id,
    DB_ERROR		*dberr);

    /*
    ** Handle the disconnection of the FE/coordinator for the willing commit 
    ** slave transaction.
    */
FUNC_EXTERN DB_STATUS disconnect_willing_commit(
    DB_TRAN_ID          *tran_id,
    i4		lock_id,
    i4		log_id,
    DB_ERROR		*dberr);

    /* Resume a willing commit transaction. */
FUNC_EXTERN DB_STATUS dmxe_resume(
    DB_DIS_TRAN_ID	*dis_tran_id,
    DMP_DCB		*dcb,
    i4		*related_lock,
    i4		*log_id,
    DB_TRAN_ID		*tran_id,
    i4		*lock_id,
    DB_ERROR		*dberr);

    /* Check if transaction is in ABORT state. */
FUNC_EXTERN DB_STATUS dmxe_check_abort(
    i4		log_id,
    bool		*abort_state);

FUNC_EXTERN DB_STATUS dmxe_pass_abort(
			i4		    log_id,
			i4		    lock_id,
			DB_TRAN_ID	    *tran_id,
			i4		    db_id,
			DB_ERROR    	    *dberr);

FUNC_EXTERN bool dmxe_xn_active(
u_i4		tran_low);

FUNC_EXTERN DB_STATUS dmxe_xa_abort(
DB_DIS_TRAN_ID		*dis_tran_id,
i4			db_id,
DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmxe_xa_disassoc(
	DB_DIS_TRAN_ID		*dis_tran_id,
	i4			log_id,
	i4			lock_id,
	i4			txn_state_flags,
	DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmxe_xa_prepare(
	DB_DIS_TRAN_ID		*dis_tran_id,
	DMP_DCB			*dcb,
	DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmxe_xa_commit(
	DB_DIS_TRAN_ID		*dis_tran_id,
	DMP_DCB			*dcb,
	bool			OnePhaseCommit,
	DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmxe_xa_rollback(
	DB_DIS_TRAN_ID		*dis_tran_id,
	DMP_DCB			*dcb,
	DB_ERROR		*dberr);
