/*
** Copyright (c) 1987, 2005 Ingres Corporation
**
**
NO_OPTIM=dr6_us5 dgi_us5 int_lnx int_rpl ibm_lnx i64_aix a64_lnx
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <ci.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <me.h>
#include    <st.h>
#include    <cv.h>
#include    <cs.h>
#include    <lk.h>
#include    <lo.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cui.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>
#include    <usererror.h>
#include    <sxf.h>

#include    <qefmain.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <rdf.h>

#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sc0a.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sceshare.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>


/**
**
**  Name: scsdbfcn.c - Database manipulation functions used by scs
**
**  Description:
**      This file contains the routines used by the sequencer to perform 
**      direct database manipulations.
**
**          scs_dbopen - open a database
**          scs_dbclose - close a database
**	    scs_dbms_task - call specialized dbms task on behalf of session
**	    scs_dbadduser - inrement no of database users in dbcb
**	    scs_dbdeluser - decrement no of users in dbcb
**	    scs_atfactot  - initiate a factotum thread
**
**
**  History:
**      23-Jun-1986 (fred)
**          Created on Jupiter.
**      02-jan-1987 (fred)
**          Modified interface to scs_dbopen() to allow for finding dbcb
**          only.
**      15-Jun-1988 (rogerk)
**	    Added scs_dbms_task routine to process specialized server threads.
**	30-sep-1988 (rogerk)
**	    Added write behind thread support.
**      23-Mar-1989 (fred)
**          Added server initialization thread support to scs_dbms_task
**	31-Mar-1989 (ac)
**	    Added 2PC support.
**	12-may-1989 (anton)
**	    Local collation support
**      03-apr-1990 (fred)
**          Added setting of activity when aborting xact.
**	10-dec-1990 (neil)
**	    Alerters: Add support for the event handling thread
**	20-dec-1990 (rickh)
**	    Forbid the opening of an INGRES db by an RMS server and vice versa.
**	03-may-1991 (rachael)
**	    Bug 33328 - add to scs_dbclose the release of
**	    Sc_main_cb.sc_kdbl.kdb_semaphore immediately after the MEcmp
**	    and add the regaining  of the semaphore prior to removing the dbcb
**	    from the scb
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**	22-sep-1992 (bryanp)
**	    Added support for new logging & locking special threads.
**	18-jan-1993 (bryanp)
**	    Added support for CPTIMER special thread.
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**	15-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Use STbcompare instead of MEcmp when searching for database names.
**	08-jun-1993 (jhahn)
**	    Added initialization of qe_cb->qef_no_xact_stmt in scs_dbclose.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	09-oct-93 (swm)
**	    Bugs #56437 #56447
**	    Put session id. (cs_self) into new dmc_session_id instead of
**	    dmc_id.
**	    Eliminate compiler warning by removing PTR cast of CS_SID
**	    value.
**	25-oct-93 (robf)
**	    Add terminator thread
**      05-nov-1993 (smc)
**          Fixed up a number of casts.
**      31-jan-1994 (mikem)
**          Reordered #include of <cs.h> to before <lk.h>.  <lk.h> now
**          references a CS_SID and needs <cs.h> to be be included before
**          it.
**      08-mar-1994 (rblumer)
**          Always return dbcb in dbcb_loc if dbcb_loc is non-null, rather than
**          only doing it when SCV_NOOPEN_MASK is set.  (in scs_dbopen)
**      24-Apr-1995 (cohmi01)
**          In scs_dbms_task, add startup for write-along/read-ahead threads.
**	11-jan-1995 (dougb)
**	    Remove code which should not exist in generic files. (Replace
**	    printf() calls with TRdisplay().)
**	    ??? Note:  This should be replaced with new messages before the
**	    ??? IOMASTER server type is documented for users.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	5-jun-96 (stephenb)
**	    Add functions scs_dbadduser and scs_dbdeluser for calls from
**	    DMF replication queue manager thread, also
**	    add SCS_REP_QMAN case to scs_dbms_task.
**	13-jun-96 (nick)
**	    LINT directed changes.
**	08-aug-1996 (canor01)
**	    Clean up various windows of opportunity to return without
**	    releasing the Sc_main_cb->sc_kdbl.kdb_semaphore semaphore.
**      01-aug-1996 (nanpr01 for ICL keving) 
**          scs_dbms_task can now start an LK Callback Thread if required.
**	15-Nov-1996 (jenjo02)
**	    Start a Deadlock Detector thread in the recovery server.
**	12-dec-1996 (canor01)
**	    Allow startup of Sampler thread.
**	07-feb-1997 (canor01)
**	    Overload terminator thread with handling the RDF cache
**	    synchronization event.  When other servers in installation
**	    signal an RDF invalidate event, process it here.
**	26-feb-1997 (canor01)
**	    If the event system has not been initialized, do not wait
**	    for events.
** 	08-apr-1997 (wonst02)
** 	    Update the scb's ics_db_access_mode from the dmc.
**	13-aug-1997 (wonst02)
**	    Remove 'if' so that dmc_db_access_mode is always primed from scb.
**      13-jan-1998 (hweho01)
**          Added NO_OPTIM dgi_us5 to this files. 
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	06-may-1998 (canor01)
**	    Add support for license manager.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started
**	    as factotum threads.
**	27-may-1998 (canor01)
**	    Initialize status variable to prevent spurious License Thread
**	    Shutdown Error messages.
**	01-Jun-1998 (jenjo02)
**	    Pass database name to sc0e_put() during E_US0010_NO_SUCH_DB error.
**      13-sep-98 (mcgem01)
**          The Star Server license thread should be checking the CI_DBE
**          license bit.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	27-jan-99 (toumi01)
**	    NO_OPTIM for lnx_us5 to prevent SIGSEGV during shutdown.  To
**	    reproduce this, just install and then ingstart/ingstop.  The
**	    errlog.log will show "Segmentation Violation (SIGSEGV)" and
**	    "E_CL2514_CS_ESCAPED_EXCEPTION".
**      15-Mar-1999 (hanal04)
**          - Clear information error E_DM0011, to prevent this being mistaken
**          for an error in scs_sequencer(). 
**          - scd_dbadd() may timeout on config file lock requests.
**          to prevent deadlock situations with the dblist mutex.
**          If this happens retry scd_dbadd(). b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788. To ensure
**          CNF lock / dblist mutes deadlock does not occur take the 
**          CNF lock prior to taking the dblist mutex. b97083.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	27-jan-99 (toumi01)
**	    NO_OPTIM for lnx_us5 to prevent SIGSEGV during shutdown.  To
**	    reproduce this, just install and then ingstart/ingstop.  The
**	    errlog.log will show "Segmentation Violation (SIGSEGV)" and
**	    "E_CL2514_CS_ESCAPED_EXCEPTION".
**	20-Aug-1999 (hanch04)
**	    Fix logic for conflicting change.
**      08-sep-1999 (devjo01)
**          Set SC_LIC_VIOL_REJECT in sc_capabilities if BACKEND check fails.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      08-Feb-2001 (horda03) Bug 103927
**          When removing events registered for a session, clear the
**          event fields introduced for bug 103596.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx.
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scs_dbadduser(), scs_dbdeluser(), scs_declare(),
**	    scs_clear(), scs_disable(), scs_enable(),
**	    scs_atfactot().
**	10-Jan-2001 (jenjo02)
**	    Deleted qef_dmf_id, use qef_sess_id instead.
**	19-Jan-2001 (jenjo02)
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**	22-Feb-2001 (jenjo02)
**	    Callers of RDF must provide session id.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	11-may-2001 (devjo01)
**	    Add CSP threads ( SCS_CSP_MAIN, SCS_CSP_MSGMON, SCS_CSP_MSGTHR ).
**	    Also added some headers to include missing function prototypes,
**	    and made local functions scs_secaudit_writer_thread and
**	    scs_open_iomdblist static for clean compile.
**	01-nov-2001 (devjo01)
**	    Per new licensing policy, only check once per week.  Rearrange
**	    code to check first, then sleep, so 1st check occurs immediately.
**	12-nov-2001 (devjo01)
**	    Amend above to once/day. Add TRdisplay. (b106246)
**      12-mar-2002 (chash01)
**          Bug 107317, upgradedb hangs when trying upgrade database from
**          6.4 to 2.0.  The cause is missing parenthesis on sizeof argument.
**      22-mar-2001 (chash01)
**          Bug 107317 is not caused by missing paren around sizeof argument.
**          Add paren any way, its better coding practice.
**	17-Jan-2003 (hanje04)
**	    Added NO_OPTIM for a64_lnx to stop SEGV on server shutdown.
**	06-Jun-2003 (hanje04)
**	    Bug 110370
**	    Removed check for session not being in any facility when marking 
**	    idle as most IDLE sessions will infact be in CLF.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	22-Sep_2004 (bonro01)
**	    Initialize status to prevent invalid E_SC0341_TERM_THREAD_EXIT
**	    errors.
**	09-may-2005 (devjo01)
**	    Add support for distributed deadlock thread.
**	21-jun-2005 (devjo01)
**	    Add support for GLC support thread.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new forms of sc0ePut(), uleFormat().
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          When opening a DB check MustLog_DB_Lst entries and flag the open
**          with DMC2_MUST_LOG if this DBMS must log all operations on the
**          DB. SET NOLOGGING will be blocked.
**       16-Mar-2010 (frima01) SIR 121619
**          Removed define for IIDBDB_ID, now located in dmf.h.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Delete sca.h include. Function prototypes moved to
**	    scf.h for exposure to DMF.
**/

/*
** Static declarations
*/
static DB_STATUS scs_check_term_thread(
    SCD_SCB	*scb);

static STATUS scs_chkterm_func(
    SCD_SCB	*scb,
    PTR		arg);

static STATUS scs_handle_alert( 
    SCD_SCB *scb, 
    char *alert_name );

static DB_STATUS scs_reg_alert( 
    SCD_SCB *scb, 
    char *alert_name );

static DB_STATUS
scs_secaudit_writer_thread( SCD_SCB *scb );

static DB_STATUS
scs_open_iomdblist(SCD_SCB *scb,
		DMC_CB *dmc, 
		char *dbname, 
		DB_ERROR *error);


/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB *Sc_main_cb;      /* core structure of scf */
GLOBALREF DU_DATABASE dbdb_dbtuple;    /* database tuple for iidbdb */
GLOBALREF SCS_IOMASTER  IOmaster;      /* database list etc.    */
GLOBALREF SCS_DBLIST MustLog_DB_Lst;   /* database list etc.    */


/*{
** Name: scs_dbopen	- open the requested database
**
** Description:
**      This routine opens the database which is specified in the 
**      presented session control block.  If the database is not already 
**      known to the server, it is added to the list of known databases. 
**      It is expected that the database control information has already 
**      been placed in the scb by the dbdb processing or is provided
**	via arguments. 
**
**
** Inputs:
**	db_name
**	db_owner
**	loc				optional to overide scb info
**	*db_tuple			override if necessary
**      scb                             session control block for this session
**      error                           pointer to area for error information
**	flag				Bit mask of flags specifying work to be
**					done:
**		    SCV_NOOPEN_MASK -	Find the dbcb only, return error if not
**					found.
**	dmc				pointer to dmc_cb to work with
**
** Outputs:
**      *error                          filled in with the appropriate
**					information
**	*dbcb_loc			Filled with pointer to dbcb of db in question.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none.
**
** History:
**      23-Jun-1986 (fred)
**          Created on Jupiter.
**      02-jan-1987 (fred)
**          Modified interface to scs_dbopen() to allow for finding dbcb
**          only.
**	12-may-1989 (anton)
**	    Move collation information into scb.
**	20-dec-1990 (rickh)
**	    Forbid the opening of an INGRES db by an RMS server and vice versa.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**	25-oct-93 (vijay)
**	    Correct type cast for qef_dmf_id which has been changed
**	    recently.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**          This results in owner within sc0m_allocate having to be PTR.
**      08-mar-1994 (rblumer)
**          Always return dbcb in dbcb_loc if dbcb_loc is non-null, rather than
**          only doing it when SCV_NOOPEN_MASK is set.  (part of B60387 fix)
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
** 	08-apr-1997 (wonst02)
** 	    Update the scb's ics_db_access_mode from the dmc.
**	13-aug-1997 (wonst02)
**	    Remove 'if' so that dmc_db_access_mode is always primed from scb.
**      15-Mar-1999 (hanal04)
**          - Clear information error E_DM0011, to prevent this being mistaken
**          for an error in scs_sequencer(). 
**          - Retry scd_dbadd() if timeout. b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788. Logic is
**          now as follows;
**            1) Take dblist mutex and decide whether we need to scd_dbadd().
**            2) If we do, drop the dblist mutex, take the cnf file lock and
**               re-acquire the dblist mutex.
**            3) Check the DB didn't get added after dropping the mutex.
**            4) If we still need to add, call scd_dbadd ()and drop the CNF 
**               file lock at the earliest opportunity.
**          Notify scd_dbadd() that we hold the CNF file lock. b97083.
**	26-mar-2001 (stephenb)
**	    Set Unicode collation info
**      12-mar-2002 (chash01)
**          Bug 107317, upgradedb hangs when trying upgrade database from
**          6.4 to 2.0.  The cause is missing parenthesis on sizeof argument.
**      22-mar-2001 (chash01)
**          Bug 107317 is not caused by missing paren around sizeof argument.
**          Add paren any way, its better coding practice.
**      22-sep-2004 (fanch01)
**          Remove meaningless LK_TEMPORARY flag from LKrequest call.
**	6-Jul-2006 (kschendel)
**	    DMF now returns unique (infodb) db id, store it for everyone.
**	13-May-2008 (kibro01) b120369
**	    Add flag to state this is a verifydb-style command which shouldn't
**	    check replication during the open-db.
**      09-aug-2010 (maspa05) b123189, b123960
**          Add DMC2_READONLYDB to indicate readonly database 
**
[@history_template@]...
*/
DB_STATUS
scs_dbopen(DB_DB_NAME *db_name,
	   DB_DB_OWNER *db_owner,
	   SCV_LOC *loc,
	   SCD_SCB *scb,
	   DB_ERROR *error,
	   i4  flag,
	   DMC_CB *dmc,
	   DU_DATABASE  *db_tuple,
	   SCV_DBCB  **dbcb_loc )
{
    DB_STATUS		dbop_status = 0;
    DB_STATUS           scf_status = 0;
    DB_STATUS		ret_val = E_DB_OK;
    DB_ERROR		save_error;
    SCV_DBCB		*dbcb;
    SCV_DBCB		*new_dbcb = 0;
    SCV_DBCB		*del_dbcb = 0;
    i4		        notfound;
    SCF_CB		scf_cb;
    i4			dbServerIncompat = 0;
    char		*dbType;
    char		*serverType;
    PTR			block;
    bool                cnf_locked = FALSE;
    bool                have_sem = FALSE;
    DB_STATUS		lk_status = 0;
    CL_SYS_ERR          sys_err;
    i4             local_error;
    i4             temp_lock_list = 0;
    LK_LKID             cnf_lkid;
    LK_LOCK_KEY         cf_lockkey;
    i4			i;
    char 		**vec;
    char  		dbname[DB_DB_MAXNAME + 1];

    if (db_name == 0)
    {
	db_name = &scb->scb_sscb.sscb_ics.ics_dbname;
	db_owner = (DB_DB_OWNER *) &scb->scb_sscb.sscb_ics.ics_dbowner;
	loc = scb->scb_sscb.sscb_ics.ics_loc;
    }

    for (;;)
    {
        /* b97083 - lock the CNF file, new_dbcb only true if notfound
        ** during first pass
        */
        if(new_dbcb)
        {
            /*  Create a temporary lock list. */
            lk_status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
                (i4)0, (LK_UNIQUE *)0, (LK_LLID *)&temp_lock_list, 0,
		&sys_err);
            if (lk_status != OK)
            {
                uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
                    (char *)0, 0L, (i4 *)0, &local_error, 0);
		SETDBERR(error, 0, E_SC0389_LOCK_CREATE);
                break;
            }

            MEcopy((PTR)&dmc->dmc_db_name, LK_KEY_LENGTH,
		(PTR)&cf_lockkey.lk_key1);
            cf_lockkey.lk_type = LK_CONFIG;

            cnf_lkid.lk_unique = 0;
	    cnf_lkid.lk_common = 0;

            lk_status = LKrequest(LK_PHYSICAL, temp_lock_list,
                    &cf_lockkey, LK_X, (LK_VALUE * )0,
                    (LK_LKID *)&cnf_lkid, 0L, &sys_err);
            if (lk_status != OK)
            {
                uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL, 0, 0, 0,
                    &local_error, 0);
		SETDBERR(error, 0, E_SC0390_BAD_LOCK_REQUEST);
                break;
            }

            cnf_locked = TRUE; 
            dmc->dmc_lock_list = (PTR)&temp_lock_list;
        }

	/*
	** Search the known database list for the database in question.
	** This is done by requesting a semaphore for the list,
	** and searching the list.
	*/

	if (scf_status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	{
	    SETDBERR(&scf_cb.scf_error, 0, scf_status);
	    break;
	}
        have_sem = TRUE;

	for ( dbcb = Sc_main_cb->sc_kdbl.kdb_next, notfound = 1;
		dbcb->db_next != Sc_main_cb->sc_kdbl.kdb_next;
		dbcb = dbcb->db_next)
	{
	    /*
	    ** List is maintained in sorted order.
	    ** Thus, the search can stop when a value is found
	    ** which is greater than or equal to the string in question.
	    */

	    if ((notfound = STbcompare((char *)&dbcb->db_dbname,
				sizeof(dbcb->db_dbname),
				(char *)db_name,
			        sizeof(DB_DB_NAME), TRUE)) >= 0)
		break;
	}
	
	if (flag & SCV_NOOPEN_MASK)
	{
            if(cnf_locked)
            {
                lk_status = LKrelease(LK_ALL, temp_lock_list,
                        (LK_LKID *)0, (LK_LOCK_KEY *)0,
                        (LK_VALUE *)0, &sys_err);
                if (lk_status != OK)
                {
                    uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
                        (char *)0, 0L, (i4 *)0, &local_error, 0);
		    SETDBERR(error, 0, E_SC0391_BAD_LOCK_RELEASE);
                }

            }

	    if (notfound)
	    {
		SETDBERR(error, 0, E_SC012D_DB_NOT_FOUND);
		return(E_DB_ERROR);
	    }
	    if (dbcb_loc)
	    {
		*dbcb_loc = dbcb;
	    }
	    return(E_DB_OK);
	}
		
	if (notfound)
	{
            /* Allocate new_dbcb only if this is the first pass */
            if(!(new_dbcb))
            {
	       scf_status = sc0m_allocate(0, (i4)sizeof(SCV_DBCB),
				DBCB_TYPE,
				(PTR) SCD_MEM,
				DBCB_TAG,
				&block);
	       new_dbcb = (SCV_DBCB *)block;
	       if (scf_status)
	       {
		   SETDBERR(&scf_cb.scf_error, 0, scf_status);
	 	   break;
	       }
	       del_dbcb = new_dbcb;
	       STRUCT_ASSIGN_MACRO(*((DB_OWN_NAME *)db_owner), new_dbcb->db_dbowner);  /* ONE LINE FOR IBM */
	       STRUCT_ASSIGN_MACRO(*db_name, new_dbcb->db_dbname); /* ONE LINE FOR IBM */

	       new_dbcb->db_ucount = 0;
	       new_dbcb->db_flags_mask =
	 	   (scb->scb_sscb.sscb_ics.ics_dmf_flags & ~DMC_NOWAIT);
	       new_dbcb->db_access_mode =
		    scb->scb_sscb.sscb_ics.ics_db_access_mode;
	       STRUCT_ASSIGN_MACRO(*loc, new_dbcb->db_location);
	       if (db_tuple)
		   STRUCT_ASSIGN_MACRO(*db_tuple, new_dbcb->db_dbtuple);
	       new_dbcb->db_state = SCV_IN_PROGRESS;

               /* b97083 - drop mutex, continue will take lock then mutex */
               if (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
	       {
		   SETDBERR(&scf_cb.scf_error, 0, scf_status);
	 	   break;
	       }
               else
               {
                   have_sem = FALSE;
                   continue;
               }
            }

	    /*
	    ** We only want to add one database at a time, to avoid race
	    ** conditions from adding the same one at the same time.
	    */

            /* b97083 - Now we have the CNF file locked tell scd_dbadd() */
            new_dbcb->db_flags_mask |= DMC_CNF_LOCKED;

	    dbop_status = scd_dbadd(new_dbcb, error, dmc, &scf_cb);
            if(dbop_status)
               break;

	    new_dbcb->db_next = dbcb;
	    new_dbcb->db_prev = dbcb->db_prev;
	    dbcb->db_prev->db_next = new_dbcb;
	    dbcb->db_prev = new_dbcb;
	    dbcb = new_dbcb;
	}

        /* 97083 - Release CNF file lock now or DMC_OPEN_DB will hang */
        if(cnf_locked)
        {
            if (lk_status = LKrelease(LK_ALL, temp_lock_list, (LK_LKID *)0, 
			(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err))
            {
                uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
                     (char *)0, 0L, (i4 *)0, &local_error, 0);
		SETDBERR(error, 0, E_SC0391_BAD_LOCK_RELEASE);
                break;
            }
            cnf_locked = FALSE;
        }

	/* return pointer to dbcb, if one was passed in
	 */
	if (dbcb_loc)
	{
	    *dbcb_loc = dbcb;
	}

	dbcb->db_ucount += 1;
	/*								    */
	/* We are unlocking the database control block so others may use    */
	/* it.  Thus, if we find an error in opening the db, if we are	    */
	/* not the thread that deletes the dbcb from the system, we cannot  */
	/* deallocate the dbcb.						    */

	del_dbcb = 0;

	/*
	** Before unlocking the database, make sure that we are allowed
	** to open this variety of db.  RMS servers may not access
	** INGRES databases and vice versa.
	**
	** IIDBDB is a special case.  It is the only INGRES database
	** that RMS servers may access.
	*/

	if ( dbcb->db_dbtuple.du_dbid != IIDBDB_ID )
	{
	    if ( ( Sc_main_cb->sc_capabilities & SC_INGRES_SERVER ) &&
	     ( dbcb->db_dbtuple.du_dbservice & DU_3SER_RMS ) )
	    {
		dbServerIncompat = 1;
		serverType = ERx( "INGRES" );
		dbType = ERx( "RMS" );
	    }
	    else if ( ( Sc_main_cb->sc_capabilities & SC_RMS_SERVER ) &&
	     !( dbcb->db_dbtuple.du_dbservice & DU_3SER_RMS ) )
	    {
		dbServerIncompat = 1;
		serverType = ERx( "RMS" );
		dbType = ERx( "INGRES" );
	    }
	}	/* end if du_dbid */



	/* if the database and server are compatible, then dmf-open the db */

	if ( !dbServerIncompat )
	{

	    /*
	    ** At this point we have a valid dbcb for the database in question.
	    ** Now, we open that database on behalf of the user in question.
	    */

	    if (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
	    {
	        SETDBERR(&scf_cb.scf_error, 0, scf_status);
	        break;
	    }

	    dmc->type = DMC_CONTROL_CB;
	    dmc->length = sizeof(DMC_CB);

	    dmc->dmc_flags_mask =
	        scb->scb_sscb.sscb_ics.ics_dmf_flags;

	    /* None of these applications want to allow replication to start */
	    switch (scb->scb_sscb.sscb_ics.ics_appl_code)
	    {
		case DBA_CREATEDB:		
		case DBA_SYSMOD:		
		case DBA_DESTROYDB:		
		case DBA_CONVERT:		
		case DBA_NORMAL_VFDB:		
		case DBA_PRETEND_VFDB:		
		case DBA_FORCE_VFDB:		
		case DBA_PURGE_VFDB:		
		case DBA_OPTIMIZEDB:		
			dmc->dmc_flags_mask2 |= DMC2_ADMIN_CMD;
			break;
	    }

            vec = (char **)MustLog_DB_Lst.dblist;
            if(MustLog_DB_Lst.dbcount)
            {
                STlcopy(db_name->db_db_name, dbname, DB_DB_MAXNAME);
                STtrmwhite(dbname);
            }
            for( i = 0 ; i < MustLog_DB_Lst.dbcount ; i++ )
            {
                if(STxcompare(vec[i],
                              STlength(vec[i]),
                              dbname, 
                              STlength(dbname),
                              TRUE, TRUE) == 0)
                    dmc->dmc_flags_mask2 |= DMC2_MUST_LOG;
            }

	    dmc->dmc_db_access_mode = scb->scb_sscb.sscb_ics.ics_db_access_mode;
	    dmc->dmc_db_location.data_address = (char *) &loc->loc_entry;
	    dmc->dmc_db_location.data_in_size = 
	        dbcb->db_location.loc_l_loc_entry; /* nbr locations * sizeof */
	    dmc->dmc_session_id = (PTR) scb->cs_scb.cs_self;
	    dmc->dmc_db_id = dbcb->db_addid;
	    dmc->dmc_lock_mode = scb->scb_sscb.sscb_ics.ics_lock_mode;
	    dmc->dmc_op_type = DMC_DATABASE_OP;

            if (scb->scb_sscb.sscb_ics.ics_dmf_flags2 & DMC2_READONLYDB)
                    dmc->dmc_flags_mask2 |= DMC2_READONLYDB;

	    dbop_status = dmf_call(DMC_OPEN_DB, dmc);

	    if (scf_status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	    {
	        SETDBERR(&scf_cb.scf_error, 0, scf_status);
	        break;
	    }

	}	/* end if database and server are compatible */

	if ( dbop_status || dbServerIncompat )
	{
	    dbcb->db_ucount -= 1;
	    save_error = dmc->error;
	    if (dbcb->db_ucount <= 0)
	    {
		(VOID) scd_dbdelete(dbcb, error, dmc, &scf_cb);
		dbcb->db_next->db_prev = dbcb->db_prev;
		dbcb->db_prev->db_next = dbcb->db_next;
		
		/* Mark that we can delete this dbcb */
		del_dbcb = dbcb;

		/* To fool error processing */

		new_dbcb = dbcb;
	    }
	    dmc->error = save_error;

	    break;
	}

	if (dmc->dmc_db_access_mode) 
	    scb->scb_sscb.sscb_ics.ics_db_access_mode = dmc->dmc_db_access_mode;
	scb->scb_sscb.sscb_ics.ics_opendb_id = dmc->dmc_db_id;
	/* Make sure everyone uses the same value for the database ID
	** number (the config file can differ from iidatabase).
	*/
	scb->scb_sscb.sscb_ics.ics_udbid = dmc->dmc_udbid;

	scb->scb_sscb.sscb_ics.ics_coldesc = dmc->dmc_coldesc;
	MEcopy(dmc->dmc_collation, sizeof(scb->scb_sscb.sscb_ics.ics_collation),
	    scb->scb_sscb.sscb_ics.ics_collation);
	scb->scb_sscb.sscb_ics.ics_ucoldesc = dmc->dmc_ucoldesc;
	MEcopy(dmc->dmc_ucollation, sizeof scb->scb_sscb.sscb_ics.ics_ucollation,
	    scb->scb_sscb.sscb_ics.ics_ucollation);
	
	/* "Forever" link scb <-> dbcb */

	scb->scb_sscb.sscb_dbcb = dbcb;
	
	break;
    }

    if(cnf_locked && (lk_status == E_DB_OK))
    {
        if (lk_status = LKrelease(LK_ALL, temp_lock_list, (LK_LKID *)0, 
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err))
        {
            uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
                 (char *)0, 0L, (i4 *)0, &local_error, 0);
	    SETDBERR(error, 0, E_SC0391_BAD_LOCK_RELEASE);
        }
    }

    /* if new one allocated, then that's where the error is */ 
    if (new_dbcb)
	dbcb = new_dbcb;	

    if (lk_status || dbop_status || scf_status)
    {
        if(lk_status)
        {
           ret_val = lk_status;
        }
        else
        {
	    if (scf_status)
	    {
	        ret_val = scf_status;
	        STRUCT_ASSIGN_MACRO(scf_cb.scf_error, *error);
	    }
	    else 
	    {
	        ret_val = dbop_status;
	        STRUCT_ASSIGN_MACRO(dmc->error, *error);
	    }
        }

	if (dmc->error.err_code != E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    sc0ePut(NULL, E_SC0121_DB_OPEN, NULL, 4,
		     sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		     sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner,
		     sizeof(dbcb->db_access_mode), (PTR)&dbcb->db_access_mode,
		     sizeof(dbcb->db_flags_mask), (PTR)&dbcb->db_flags_mask);

	    {
		sc0ePut(NULL, E_SC010D_DB_LOCATION, 0, 3,
			 sizeof(dbcb->db_location.loc_entry.loc_name),
			 (PTR)&dbcb->db_location.loc_entry.loc_name,
			 dbcb->db_location.loc_entry.loc_l_extent,
			 (PTR)dbcb->db_location.loc_entry.loc_extent,
			 sizeof(dbcb->db_location.loc_entry.loc_flags),
			 (PTR)&dbcb->db_location.loc_entry.loc_flags);
	    }
	}
    }

    if (have_sem &&
        (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore)))
    {
	sc0ePut(NULL, E_SC0121_DB_OPEN, NULL, 4,
		 sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		 sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner,
		 sizeof(dbcb->db_access_mode), (PTR)&dbcb->db_access_mode,
		 sizeof(dbcb->db_flags_mask), (PTR)&dbcb->db_flags_mask);
	/*
	** No reason to list locations here.  This error is not
	** related to the database.
	*/
	sc0ePut(NULL, E_SC0202_SEM_RELEASE, NULL, 0);
	sc0ePut(error, scf_status, NULL, 0);
	ret_val = scf_status;
    }

    if ( dbServerIncompat )
    {
	ret_val = E_DB_ERROR;
	SETDBERR(error, 0, E_US0051_DB_SERVER_MISMATCH);
	sc0ePut( NULL, E_US0050_DB_SERVER_INCOMPAT, NULL, 2,
		  ER_PTR_ARGUMENT, (PTR)serverType,
		  ER_PTR_ARGUMENT, (PTR)dbType);
    }

    if (ret_val != E_DB_OK)
    {
	if (del_dbcb)
	    scf_status = sc0m_deallocate(0, (PTR *)&del_dbcb);
    }
    scd_note(ret_val, DB_SCF_ID);
    return(ret_val);
}

/*{
** Name: scs_dbclose	- close a database
**
** Description:
**      This routine closes databases (via DMF) which were opened by
**      the sequencer.  If the appropriate criteria is met (at the moment, 
**      that criteria is that no users are using the database), then the 
**      database is deleted from the server (via a call to scd_dbdelete()).
**
** Inputs:
**      scb                             session control block for db in question
**      error                           DB_ERROR block
**	dmc				ptr to dmc to work with
**
** Outputs:
**      *error.err_code                 any appropriate error message
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    The database is closed.  It may also be deleted from the server.
**
** History:
**	26-Jun-1986 (fred)
**          Created on Jupiter.
**	31-Mar-1989 (ac)
**	    Added 2PC support.
**	03-may-1991 (rachael)
**	    Bug 33328 - add to scs_dbclose the release of
**	    Sc_main_cb.sc_kdbl.kdb_semaphore immediately after the MEcmp
**	    and add the regaining  of the semaphore prior to removing the dbcb
**	    from the scb
**	08-jun-1993 (jhahn)
**	    Added initialization of qe_cb->qef_no_xact_stmt.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	08-aug-1996 (canor01)
**	    Clean up window of opportunity to return without releasing 
**	    the Sc_main_cb->sc_kdbl.kdb_semaphore semaphore.
*/
DB_STATUS
scs_dbclose(DB_DB_NAME *db_name,
             SCD_SCB *scb,
             DB_ERROR *error,
             DMC_CB *dmc )
{
    SCV_DBCB            *dbcb;
    SCV_DBCB            *dbcb_delete = 0;
    QEF_RCB		*qe_ccb;
    DB_STATUS		scf_status;
    DB_STATUS		dbop_status = E_DB_OK;
    DB_STATUS		qef_status = E_DB_OK;
    DB_STATUS		ret_val = E_DB_OK;
    SCF_CB		scf_cb;

    CLRDBERR(error);

    if (!db_name)
	db_name = &scb->scb_sscb.sscb_ics.ics_dbname;

    for (;;)
    {

	dbcb = scb->scb_sscb.sscb_dbcb;
	if (!dbcb)	    /* must not have been opened */
	    return(E_DB_OK);

	if (scf_status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	{
	    SETDBERR(error, 0, scf_status);
	    break;
	}

	if (
		STbcompare((char *)&dbcb->db_dbname,
			sizeof(dbcb->db_dbname),
			(char *)db_name,
			sizeof(DB_DB_NAME), TRUE)
	    )
	{
	    SETDBERR(error, 0, E_SC010B_SCB_DBCB_LINK);
	    break;
	}

	if (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
	{
	    SETDBERR(error, 0, scf_status);
	    break;
	}
	
	/*
	** At this point we have a valid dbcb for the database in question.
	** Now, we close that database on behalf of the user in question.
	*/

	dmc->type = DMC_CONTROL_CB;
	dmc->length = sizeof(DMC_CB);

	dmc->dmc_session_id = (PTR) scb->cs_scb.cs_self;
	dmc->dmc_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
	dmc->dmc_flags_mask =
	    scb->scb_sscb.sscb_ics.ics_dmf_flags;
	dmc->dmc_op_type = DMC_DATABASE_OP;
	for (;;)
	{
	    dbop_status = dmf_call(DMC_CLOSE_DB, dmc);
	    if (dbop_status)
	    {
		if (dmc->error.err_code == E_DM0060_TRAN_IN_PROGRESS)
		{
		    /* then we need to abort the transaction */

		    MEcopy("Aborting on behalf of terminating session",
			    41, scb->scb_sscb.sscb_ics.ics_act1);
		    scb->scb_sscb.sscb_ics.ics_l_act1 = 41;
		    
		    qe_ccb = scb->scb_sscb.sscb_qeccb;
		    qe_ccb->qef_cb = scb->scb_sscb.sscb_qescb;
		    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		    qe_ccb->qef_flag = 0;
		    qe_ccb->qef_eflag = QEF_INTERNAL;
		    qe_ccb->qef_modifier = qe_ccb->qef_cb->qef_stat;
		    qe_ccb->qef_spoint = 0;
		    qe_ccb->qef_no_xact_stmt = FALSE;
		    qef_status = qef_call(QET_ABORT, qe_ccb);

		    MEcopy("Terminating Session",
			    19, scb->scb_sscb.sscb_ics.ics_act1);
		    scb->scb_sscb.sscb_ics.ics_l_act1 = 19;

		    if (qef_status)
		    {
			sc0ePut(&qe_ccb->error, 0, NULL, 0);
		    }
		    else
		    {
			continue;
		    }
		}
		*error= dmc->error;
		break;
	    }
	    break;
	}
	
	/* now remove dbcb from scb's knowledge */

	if (scf_status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	{
	    SETDBERR(error, 0, scf_status);
	    break;
	}
	scb->scb_sscb.sscb_dbcb = 0;
	scb->scb_sscb.sscb_ics.ics_opendb_id = 0;
	dbcb->db_ucount -= 1;

	if (dbcb->db_ucount == 0)
	{
	    /* Errors reported, but shouldn't override the close errors */
	    (VOID) scd_dbdelete(dbcb, error, dmc, &scf_cb);
	    if (dbop_status)
		dmc->error = *error;
	    dbcb->db_next->db_prev = dbcb->db_prev;
	    dbcb->db_prev->db_next = dbcb->db_next;
	    dbcb_delete = dbcb;
	}
	
	break;
    }

    if (scf_status || dbop_status || error->err_code)
    {
	if (ret_val = dbop_status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0122_DB_CLOSE, NULL, 2,
		     sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		     sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner);
	    sc0ePut(NULL, E_SC010D_DB_LOCATION, NULL, 3,
		     sizeof(dbcb->db_location.loc_entry.loc_name),
		     (PTR)&dbcb->db_location.loc_entry.loc_name,
		     dbcb->db_location.loc_entry.loc_l_extent,
		     (PTR)dbcb->db_location.loc_entry.loc_extent,
		     sizeof(dbcb->db_location.loc_entry.loc_flags),
		     (PTR)&dbcb->db_location.loc_entry.loc_flags);
	    *error = dmc->error;
	}
	else if (ret_val = scf_status)
	{
	    sc0ePut(NULL, E_SC0201_SEM_WAIT, NULL, 0);
	    sc0ePut(NULL, E_SC0122_DB_CLOSE, NULL, 2,
		     sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		     sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner);
	    sc0ePut(error, 0, NULL, 0);
	}
	else
	{
	    sc0ePut(error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0122_DB_CLOSE, NULL, 2,
		     sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		     sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner);
	    ret_val = E_DB_FATAL;
	}
    }
    if (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
    {
	SETDBERR(error, 0, scf_status);
	sc0ePut(NULL, E_SC0202_SEM_RELEASE, NULL, 0);
	sc0ePut(error, 0, NULL, 0);
	if (!ret_val)
	    ret_val = scf_status;
    }
    if (dbcb_delete)
    {
	scf_status = sc0m_deallocate(0, (PTR *)&dbcb_delete);
	if (scf_status)
	{
	    sc0ePut(NULL, scf_status, NULL, 0);
	    ret_val = E_DB_FATAL;
	}
    }
    scd_note(ret_val, DB_SCF_ID);
    return(ret_val);
}

/*{
** Name: scs_dbms_task	- process specialized server thread task.
**
** Description:
**	This routine is called by the sequencer when it is given a
**	thread with the SCS_SPECIAL state.  This state specifies that
**	the thread is a dedicated session to perform some server task.
**	This routine calls the appropriate task.
**
**	Tasks provided by this routine:
**	    SCS_SFAST_COMMIT	: Thread is a fast commit thread - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SRECOVERY   	: Thread is the recovery thread - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SLOGWRITER  	: Thread is the Logwriter thread - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SCHECK_DEAD  	: Thread is the dead process detector - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SGROUP_COMMIT  	: Thread is the group commit thread - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SFORCE_ABORT  	: Thread is the force abort thread - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SCP_TIMER	: Thread is the Consistency Point Timer - call
**		    DMF to actually do the work.  This thread should not
**		    return unless the server is being shutdown.
**	    SCS_SERVER_INIT	: Thread is the server initialization task.
**		    Call sca_add_datatypes to reset ADF, then release the
**		    semaphore which keeps user threads out.  This thread is then
**		    done and will return.
**	    SCS_SEVENT		: Event handling thread.
**		    Call sce_thread. This thread should not return unless the
**		    server is being shut down.
**	    SCS_SCHECK_TERM     : Session termination thread
**	            Call scs_check_term. This thread should not return 
**	            unless the server is being shut down.
**	    SCS_SSECAUD_WRITER  : Security Audit Writer thread
**	            Call scs_secaudit_writer. This thread should not return 
**	            unless the server is being shut down.
**          SCS_SLK_CALLBACK  : LK Blocking Callback Thread
**                  Call DMF to actually do the work. This thread should not
**                  return unless the server is being shut down. 
**	    SCS_SDEADLOCK	: Optimistic Deadlock Detector Thread
**	            Call LK via DMF to actually do the work. This thread must
**	   	    not return unless the recovery server is being shut down.
**	    SCS_SDISTDEADLOCK	: Distributed Deadlock Detector Thread
**	            Call LK via DMF to actually do the work. This thread must
**	   	    not return unless the recovery server is being shut down.
**		    Only used in conjunction with a DLM.
**	    SCS_SGLCSUPPORT  	: Group Lock Container support Thread
**	            Call CX via DMF to actually do the work. This thread must
**	   	    not return unless the server is being shut down.
**		    Only used in conjunction with a DLM without a native GLC.
**	    SCS_SSAMPLER	: Sampler Thread
**		    Call scs_sampler.  This thread can be shut down by
**		    the monitor.
**	    SCS_SFACTOTUM	: Factotum Thread.
**		    Call the function specified by the thread creator.
**
** Inputs:
**      scb				the session control block
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jun-1988 (rogerk)
**	    Created.
**	30-sep-1988 (rogerk)
**	    Added write behind thread support.
**      23-Mar-1989 (fred)
**          Added SCS_SERVER_INIT support
**	10-dec-1990 (neil)
**	    Alerters: Add support for the SCS_SEVENT thread
**	22-sep-1992 (bryanp)
**	    Added support for new logging & locking special threads.
**	23-nov-1992 (markg/robf)
**	    Added support for new audit thread.
**	18-jan-1993 (bryanp)
**	    Added support for CPTIMER special thread.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	7-Jul-1993 (daveb)
**	    Removed guts of server init thread; it isn't really
**	    used, and the sca_add_datatypes call there didn't do
**	    anything.  Also, pass correct err_code element instead of
**	    a whole DB_ERROR to sc0e_put.
**	14-apr-94 (robf)
**          Add Security  Audit Writer thread
**      24-Apr-1995 (cohmi01)
**          Add startup for write-along/read-ahead threads.
**	11-jan-1995 (dougb)
**	    Remove code which should not exist in generic files. (Replace
**	    printf() calls with TRdisplay().)
**	    ??? Note:  This should be replaced with new messages before the
**	    ??? IOMASTER server type is documented for users.
**	5-jun-96 (stephenb)
**	    Add case SCS_REP_QMAN for replicator queue management thread.
**      01-aug-1995 (nanpr01 for ICL keving) 
**          Add LK Callback Thread.
**	15-Nov-1996 (jenjo02)
**	    Start a Deadlock Detector thread in the recovery server.
**	3-feb-98 (inkdo01)
**	    Add parallel sort threads.
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	13-apr-98 (inkdo01)
**	    Drop parallel sort threads in favour of factotum threads.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started
**	    as factotum threads.
**	21-may-1998 (nanpr01)
**	    Save the error code and err_data.
**	04-Jan-2001 (jenjo02)
**	    Set proper sscb_cfac to make stat collection more meaningful.
**	11-may-2001 (devjo01)
**	    Add CSP threads ( SCS_CSP_MAIN, SCS_CSP_MSGMON, SCS_CSP_MSGTHR ).
[@history_template@]...
*/
DB_STATUS
scs_dbms_task(SCD_SCB *scb )
{
    DMC_CB	    *dmc;
    DB_STATUS	    status;
    QEF_RCB	    *qef_cb;

    /*
    ** Call the specific routine to handle the task.
    */

    switch (scb->scb_sscb.sscb_stype)
    {
      case SCS_SFAST_COMMIT:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_FAST_COMMIT, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0238_FAST_COMMIT_EXIT, NULL, 0);
	}

	/*
	** It would be nice to give statistics on I/O and CPU usage by the 
	** DBMS thread.  This may be the right place to do it.
	*/
	break;

      case SCS_REP_QMAN:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_REP_QMAN, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC037C_REP_QMAN_EXIT, NULL, 0);
	}
	break;

      case SCS_SRECOVERY:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_RECOVERY_THREAD, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC031E_RECOVERY_EXIT, NULL, 0);
	}
	break;

      case SCS_SLOGWRITER:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_LOGWRITER, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0320_LOGWRITER_EXIT, NULL, 0);
	}
	break;

      case SCS_SCHECK_DEAD:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_CHECK_DEAD, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0322_CHECK_DEAD_EXIT, NULL, 0);
	}
	break;

      case SCS_SGROUP_COMMIT:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_GROUP_COMMIT, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0324_GROUP_COMMIT_EXIT, NULL, 0);
	}
	break;

      case SCS_SFORCE_ABORT:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_FORCE_ABORT, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0326_FORCE_ABORT_EXIT, NULL, 0);
	}
	break;

      case SCS_SCP_TIMER:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_CP_TIMER, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0330_CP_TIMER_EXIT, NULL, 0);
	}
	break;

      case SCS_SERVER_INIT:
	break;
	
      case SCS_SEVENT:
	scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	status = sce_thread();
	break;

      case SCS_SAUDIT:
	scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	status = scs_audit_thread(scb);
	break;

      case SCS_SCHECK_TERM:
	scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	status = scs_check_term_thread(scb);
	break;

      case SCS_SSECAUD_WRITER:
	scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	status = scs_secaudit_writer_thread(scb);
	break;

      case SCS_SLK_CALLBACK:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
        dmc = scb->scb_sscb.sscb_dmccb;
        status = dmf_call(DMC_LK_CALLBACK, dmc);
        if (status)
        { 
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC037F_LK_CALLBACK_THREAD_EXIT, NULL, 0);
        }
        break; 

      case SCS_S2PC:
	/*
	** flag only used for development, the server will not do recovery
	** of DX transactions.
	*/
	/*
	** No need to protect this assignment, we only test for
	** SC_INBUSINESS before we accept a FE connection.
	*/
	Sc_main_cb->sc_state = SC_2PC_RECOVER;
	qef_cb = scb->scb_sscb.sscb_qeccb;
	qef_cb->qef_sess_id = scb->cs_scb.cs_self;
	status = qef_call(QED_P1_RECOVER, qef_cb);
	if (DB_SUCCESS_MACRO(status))
	{
	    Sc_main_cb->sc_state = SC_OPERATIONAL;
	    if (qef_cb->qef_r3_ddb_req.qer_d13_ctl_info & QEF_04DD_RECOVERY)
	    {
		status = qef_call(QED_P2_RECOVER, qef_cb);
	    }
	}
	if (status)
	{
	    /*
	    ** Set the state so that the sequencer will kill the server.
	    */
	    Sc_main_cb->sc_state = SC_2PC_RECOVER;
	    sc0ePut(&qef_cb->error, 0, NULL, 0);
	}
	break;


      case SCS_SWRITE_ALONG:  
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_WRITE_ALONG, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0366_WRITE_ALONG_EXIT, NULL, 0);
	}
	break;

      case SCS_SREAD_AHEAD:   
	dmc = scb->scb_sscb.sscb_dmccb;
	if( Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER )
	{
	    DB_ERROR error;
	    i4   ndb;
	    char *db;

	    /*
	    ** ??? Note:  This code is attempting to inform the administrator
	    ** ??? about the databases which are monitored by this server.
	    ** ??? The information here should be written to the ERRLOG.LOG
	    ** ??? file.  (That requires new messages and I don't have time.)
	    ** ??? Do not document for users 'til this is fixed.
	    */
	    if (! IOmaster.dbcount)
		TRdisplay(
		       "IOMASTER: No databases specified in database_list\n" );
	    else
	    {
		TRdisplay(
		    "IOMASTER: The following databases will be monitored:\n" );
	    	for (ndb = 0, db = IOmaster.dblist; 
	    	    ndb < IOmaster.dbcount;  ndb++, db += STlength(db) +1)
		{
		    status = scs_open_iomdblist(scb, dmc, db, &error);
		    if (status == E_DB_OK)
			TRdisplay( "\t'%s'\n", db );
		    else
			TRdisplay( "\tFailure opening '%s' for monitoring\n",
				  db );
		    /* error text was already output */
		}
	    }

	}

	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
	status = dmf_call(DMC_READ_AHEAD, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0364_READ_AHEAD_EXIT, NULL, 0);
	}
	break;

      case SCS_SDEADLOCK:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
        dmc = scb->scb_sscb.sscb_dmccb;
        status = dmf_call(DMC_DEADLOCK_THREAD, dmc);
        if (status)
        { 
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0372_DEADLOCK_THREAD_EXIT, NULL, 0);
        }
        break; 

      case SCS_SDISTDEADLOCK:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
        dmc = scb->scb_sscb.sscb_dmccb;
        status = dmf_call(DMC_DIST_DEADLOCK_THR, dmc);
        if (status)
        { 
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0397_DIST_DEADLOCK_THR_EXIT, NULL, 0);
        }
        break; 

      case SCS_SGLCSUPPORT:
	scb->scb_sscb.sscb_cfac = DB_DMF_ID;
        dmc = scb->scb_sscb.sscb_dmccb;
        status = dmf_call(DMC_GLC_SUPPORT_THR, dmc);
        if (status)
        { 
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC0399_GLC_SUPPORT_THR_EXIT, NULL, 0);
        }
        break; 

      case SCS_SSAMPLER:
	scb->scb_sscb.sscb_cfac = DB_SCF_ID;
        status = scs_sampler(scb);
        break; 

      case SCS_SFACTOTUM:
	/*
	** Invoke the specified thread function after first
	** creating a SCF_FTX containing:
	**
	**	o thread-creator-supplied data, if any
	**	o length of that data
	**	o the thread creator's thread_id
	**	o usable status and error for thread function
	*/
	if (scb->scb_sscb.sscb_factotum_parms.ftc_thread_entry)
	{
	    SCF_FTX	ftx;

	    ftx.ftx_data 	= scb->scb_sscb.sscb_factotum_parms.ftc_data;
	    ftx.ftx_data_length = scb->scb_sscb.sscb_factotum_parms.ftc_data_length;
	    ftx.ftx_thread_id 	= scb->scb_sscb.sscb_factotum_parms.ftc_thread_id;
	    ftx.ftx_error.err_code = 0;
	    ftx.ftx_error.err_data = 0;

	    /*
	    ** Note that the entry code has been called, and prepare
	    ** the FTC for the thread termination function.
	    **
	    ** Set the initial status to E_DB_ERROR in case the
	    ** thread execution code fails.
	    **
	    ** The (optional) thread_exit code will be invoked when the
	    ** thread is terminating (scs_terminate) instead of here 
	    ** to avoid missing the opportunity during an escaped exception.
	    */
	    scb->scb_sscb.sscb_factotum_parms.ftc_state = FTC_ENTRY_CODE_CALLED;
	    scb->scb_sscb.sscb_factotum_parms.ftc_status = E_DB_ERROR;
	    scb->scb_sscb.sscb_factotum_parms.ftc_error.err_code = 0;
	    scb->scb_sscb.sscb_factotum_parms.ftc_error.err_data = 0;
	    
	    scb->scb_sscb.sscb_cfac = DB_DMF_ID;

	    status = 
		(*(scb->scb_sscb.sscb_factotum_parms.ftc_thread_entry))( &ftx );
	    
	    /* Save the execution code status for passing on to the exit code */
	    scb->scb_sscb.sscb_factotum_parms.ftc_status = status;
	    scb->scb_sscb.sscb_factotum_parms.ftc_error.err_code = 
							ftx.ftx_error.err_code;
	    scb->scb_sscb.sscb_factotum_parms.ftc_error.err_data = 
							ftx.ftx_error.err_data;
	}
	break;

      case SCS_CSP_MAIN:
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_CSP_MAIN, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC02C1_CSP_MAIN_EXIT, NULL, 0);
	}
	break;

      case SCS_CSP_MSGMON:
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_CSP_MSGMON, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC02C3_CSP_MSGMON_EXIT, NULL, 0);
	}
	break;

      case SCS_CSP_MSGTHR:
	dmc = scb->scb_sscb.sscb_dmccb;
	status = dmf_call(DMC_CSP_MSGTHR, dmc);
	if (status)
	{
	    sc0ePut(&dmc->error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC02C5_CSP_MSGTHR_EXIT, NULL, 0);
	}
	break;

      default:
	sc0ePut(NULL, E_SC0239_DBMS_TASK_ERROR, NULL, 1,
		 sizeof(scb->scb_sscb.sscb_stype),
		 (PTR)&scb->scb_sscb.sscb_stype);
	status = E_DB_ERROR;
    }

    return (status);
}

/*{
** Name: scs_atfactot	- attach a Factotum thread
**
** Description:
**	
**	This function is called via scf_call(SCS_ATFACTOT).
**
**	Its purpose is to create and invoke a Factotum 
**	(fac - do, totum - all) thread. Factotum threads are intended
**	to be short-lived internal threads which have no connection to
**	a front end and endure only for the specific task for which they
**	were invoked.
**
** Inputs:
**      scf				the session control block
**					with an embedded
**		SCF_FTC
**			ftc_facilities	what facilities are needed by this thread?
**					If SCF_CURFACILITIES, the factotum
**					thread will assume the same facilities
**					as the creating thread.
**	 		ftc_data	optional pointer to thread-specific
**					input data.
**			ftc_data_length	optional length of that data. If present,
**					memory for "data" will be allocated with
**					the factotum thread's SCB and "data"
**					copied to that location. This should be
**					used for non-durable "data" that may
**					dissolve if the creating thread terminates
**					before the factotum thread.
**			ftc_thread_name pointer to a character string to identify
**					this thread in iimonitor, etc.
**			ftc_thread_entry thread execution function pointer.
**			ftc_thread_exit	optional function to be executed when
**					the factotum thread terminates. This
**					function will be called if and only if
**					ftc_thread_entry was invoked.
**			ftc_priority	priority at which thread is to run
**					If SCF_CURPRIORITY, the factotum thread
**					will assume the same priority as the
**					creating thread.
**
** Outputs:
**			ftc_thread_id   contains the new thread's SID.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Another thread is added to the server
**
** History:
**	09-Mar-1998 (jenjo02)
**	    Invented.
**      17-Feb-2010 (hanal04) Bug 123284
**          If the parent session has DB_GWF_ID set we could be
**          creating a factotum thread to break up an IMA select.
**          The child will also need GWF.
*/
STATUS
scs_atfactot(SCF_CB *scf, SCD_SCB *scb )
{
    SCF_FTC	*ftc = scf->scf_ptr_union.scf_ftc;
    STATUS	status;
    CL_ERR_DESC err;

    /* Use attaching threads's priority and/or facilities? */
    if (ftc->ftc_priority == SCF_CURPRIORITY ||
	ftc->ftc_facilities == SCF_CURFACILITIES)
    {
	if (ftc->ftc_priority == SCF_CURPRIORITY)
	    ftc->ftc_priority = scb->cs_scb.cs_priority;
	if (ftc->ftc_facilities == SCF_CURFACILITIES)
	    ftc->ftc_facilities = scb->scb_sscb.sscb_facility;
        else
        {
            if(scb->scb_sscb.sscb_facility & (1 << DB_GWF_ID))
                ftc->ftc_facilities |= (1 << DB_GWF_ID);
        }
    }

    return(CSadd_thread(ftc->ftc_priority, (PTR)ftc, SCS_SFACTOTUM, 
			&ftc->ftc_thread_id, &err));

}

/*{
** Name: scs_ddbsearch - search the db list for given db name
**
** Description: The db list is searched for the given db name. The
**	structure of the db list is terminated by a list token that points
**	to the first token of the list. This is an artifact from the
**	back-end implementation. The search returns a boolean value but
**	always return the last examined token in the search. This allows
**	both returning of found token or sorted insertion.
**
** Inputs:
**      db_name		the search target database name
**	dbcb_list	the current list being searched
**	terminal	terminal token (the top of the list)
**	search_point	the search result
**
** Outputs:
**	search_point	the last token examined
**
**	Returns:
**	    TRUE	if db_name is found in the dbcb_list
**	    FALSE	if db_name is not found in the dbcb_list
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-Aug-1988 (alexh)
**          Created for STAR session startup.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
bool
scs_ddbsearch(char *db_name,
	       SCV_DBCB	*dbcb_list,
	       SCV_DBCB	*terminal,
	       SCV_DBCB	**search_point )
{
  i4	comp_result = -1;


  while (dbcb_list->db_next != terminal)
  {
      comp_result = STbcompare((char *)&dbcb_list->db_dbname,
			   sizeof(dbcb_list->db_dbname),
			   (char *)db_name,
			   sizeof(dbcb_list->db_dbname), TRUE);

      if ( Sc_main_cb->sc_prt_gcaf == TRUE )
      {
	  TRdisplay("%t\n%t Result %d\n",
			sizeof(dbcb_list->db_dbname), &dbcb_list->db_dbname,
			sizeof(dbcb_list->db_dbname), db_name, 
		    comp_result);
      }
      if (comp_result >= 0)
	  break;
      else
	/* search the rest of the list */
	dbcb_list = dbcb_list->db_next;
  }

  *search_point = dbcb_list;
  if (comp_result == 0)
      return (TRUE);
  else
      return (FALSE);
}

/*{
** Name: scs_ddbclose - indicate a db is closed by a user.
**
** Description:
**	This routines deletes the db token from db list.
**
** Inputs:
**      scb               session control block for db in question
**
** Outputs:
**      none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    The database is closed.  It may also be deleted from the server.
**
** History:
**	8-sept-88 (alexh)
**          create for STAR session termination
**	2-Jul-1993 (daveb)
**	    prototyped.
**	6-Jul-2006 (kschendel)
**	    Remove no-op rdf call.
*/
DB_STATUS
scs_ddbremove(SCV_DBCB *dbcb )
{
  DB_STATUS		scf_status;

  /* if use count is 0 then remove from the db list */
  if (--dbcb->db_ucount == 0)
    {

      dbcb->db_next->db_prev = dbcb->db_prev;
      dbcb->db_prev->db_next = dbcb->db_next;
      scf_status = sc0m_deallocate(0, (PTR *)&dbcb);
      if (scf_status)
	{
	  sc0ePut(NULL, scf_status, NULL, 0);
	  return(E_DB_FATAL);
	}
    }

  return(E_DB_OK);
}

/*{
** Name: scs_ddbclose - indicate a ddb is closed by a user.
**
** Description:
**      This routines deletes the ddb token from ddb list.
**
** Inputs:
**      scb               session control block for ddb in question
**
** Outputs:
**      none
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          The database is closed.  It may also be deleted from the
**          server.
** History:
**	2-Jul-1993 (daveb)
**	    prototyped.
**	08-aug-1996 (canor01)
**	    Clean up window of opportunity to return without releasing 
**	    the Sc_main_cb->sc_kdbl.kdb_semaphore semaphore.
*/
DB_STATUS
scs_ddbclose(SCD_SCB  *scb )
{
    SCV_DBCB            *dbcb;
    DB_STATUS		scf_status;

    if (! ( dbcb = scb->scb_sscb.sscb_dbcb))   /* must not have been opened */
	    return(E_DB_OK);

    if (scf_status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
    {
	sc0ePut(NULL, E_SC0201_SEM_WAIT, NULL, 0);
	sc0ePut(NULL, E_SC0122_DB_CLOSE, NULL, 2,
		 sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		 sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner);
	return(E_DB_ERROR);
    }

    /* now remove dbcb from scb's knowledge */
    scb->scb_sscb.sscb_dbcb = 0;
    scb->scb_sscb.sscb_ics.ics_opendb_id = 0;

    if (scs_ddbremove(dbcb) != E_DB_OK)
    {
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return(E_DB_FATAL);
    }

    if (scf_status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
    {
	sc0ePut(NULL, E_SC0202_SEM_RELEASE, NULL, 0);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: scs_ddbadd - add a token to the db list
**
** Description: This routine allocates, initializes and adds a database
**	token to the db list at the specified postion.
**
** Inputs:
**	db_owner	the db owner
**      db_name		the database name
**	db_access	database access_mod
**	dbcb		position in the db list to insert
**
** Outputs:
**	None
**
**	Returns:
**	    the pointer of the added token
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-Aug-1988 (alexh)
**          Created for STAR session startup.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/
SCV_DBCB *
scs_ddbadd(PTR db_owner,
	   PTR db_name,
	   i4 access_mode,
	   SCV_DBCB *dbcb )
{
  SCV_DBCB	*new_dbcb;
  PTR		block;

  sc0m_allocate(SCU_MZERO_MASK, (i4)sizeof(SCV_DBCB),
		DBCB_TYPE,
		(PTR) SCD_MEM,
		DBCB_TAG,
		&block);
  new_dbcb = (SCV_DBCB *)block;

  /* ONE LINE FOR IBM */
  STRUCT_ASSIGN_MACRO(*((DB_OWN_NAME *)db_owner), new_dbcb->db_dbowner);  
  /* ONE LINE FOR IBM */
  STRUCT_ASSIGN_MACRO(*((DB_DB_NAME *)db_name), new_dbcb->db_dbname); 

  new_dbcb->db_ucount = 1;
  new_dbcb->db_access_mode = access_mode;
  new_dbcb->db_state = SCV_IN_PROGRESS;

  new_dbcb->db_next = dbcb;
  new_dbcb->db_prev = dbcb->db_prev;
  dbcb->db_prev->db_next = new_dbcb;
  dbcb->db_prev = new_dbcb;
  return(new_dbcb);
}

/*{
** Name: scs_ddbdb_info	- fetch information about the STAR user & database
**
** Description: 
** 	If the existence of the specified distributed database is
**	verified,information such as ownership, db type and capabilities,
**	user status are obtained and stored in the session control block
**	for later user. To minimize access to iidbdb and cdb's, all active
**	iidbdb and cdb info are stored and future reference. Therefore,
**	this db list is always searched first when iidbdb or a cdb
**	information is needed. RDF calls are made only when a db is not
**	currently known. This routine is basically analog to
**	scs_dbdb_info. It should be extended to cache known user list
**	because user status info requires access to iidbdb. This caching
**	can either be in the form of a dedicate iidbdb session or an
**	dynamicallly managed list.
**
** Inputs:
**      scb             session control block for the session
**	dbdb_name	dbdb name, Usually iidbdb.
**      error           pointer to a DB_ERROR struct to fill
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-Aug-1988 (alexh)
**          Created for STAR session startup.
**	14-jul-1990 (georgeg)
**	    get cluster info from RDF to QEF.
**	21-feb-1991 (fpang)
**	    Set dbcb->db_state to SCV_ADDED after cdb
**	    info has been filled in. Prevents other incoming
**	    sessions from getting incomplete cdb descriptors.
**	    Fixes B35094.
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	    
**	    the following fields in SCS_ICS have been renamed:
**		ics_sucode	--> ics_iucode
**	22-jan-1993 (fpang)
**	    For Star. Use the real user when retrieving permissions from 
**	    iiuser. Also, if effective user is '$ingres' turn on catalog
**	    update permission automatically.
**	24-mar-1993 (barbara)
**	    Delimited identifier support.  RDR_DD5_IIDBDB now queries iidbdb
**	    to get case translation semantics of iidbdb.  Identifiers are
**	    mapped for further queries to iidbdb (via RDR_DD1_DDBINFO and
**	    RDR_DD4_USRSTAT).  The case mapping is then done to set up
**	    for queries to the CDB.
**	15-apr-1993 (robf)
**	    Removed reference to SUPERUSER, replaced by SECURITY
**	2-Jul-1993 (daveb)
**	    prototyped.
**	23-sep-93 (teresa)
**	    Fixed SYS_CATALOG_UPDATE bug 53304.
**	01-Jun-1998 (jenjo02)
**	    Pass database name to sc0e_put() during E_US0010_NO_SUCH_DB error.
**      22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**          If the RDF_DDB fails it may not be because the database does
**          not exist. Use new E_SC0390_RDF_DDB_FAILURE message.
**          This also prevents ULE_FORMAT errors on old call which
**          used E_US0010 but did not supply a database name.
**	30-Aug-2004 (sheco02)
**	    Cross-integrating change 466017 to main and bump E_SC0390 to
**	    E_SC0393.
**	31-Aug-2004 (schka24)
**	    SC0395...
**[@history_template@]...
*/
DB_STATUS
scs_ddbdb_info(SCD_SCB *scb,
	       PTR dbdb_name,
	       DB_ERROR *error )
{
    i4		status;
    SCV_DBCB		*dbcb;
    SCV_DBCB		*dbdbcb;
    RDF_CB		rdf_cb;
    DD_DDB_DESC		*ddb_descp;
    QEF_RCB		*qef_cb;
    QEF_DDB_REQ		*qef_dcb;
    DD_USR_DESC		duser_desc;
    bool		incache;
    DB_STATUS		scf_status;

    CLRDBERR(error);

    /*
    ** Search the known database list for the database in question.
    ** This is done by requesting a semaphore for the list,
    ** and searching the list.
    */

    CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore);
    qef_cb = scb->scb_sscb.sscb_qeccb;

    rdf_cb.rdf_rb.rdr_session_id = scb->cs_scb.cs_self;
    rdf_cb.rdf_rb.rdr_r1_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;

    /* make sure dbdb info is available */
    if (scs_ddbsearch(dbdb_name,
		 Sc_main_cb->sc_kdbl.kdb_next,
		 Sc_main_cb->sc_kdbl.kdb_next,
		 &dbdbcb) == FALSE)
    {
	/* if dbdb is not already known then add */
	dbdbcb = scs_ddbadd(scb->scb_sscb.sscb_ics.ics_dbowner.db_own_name,
			    dbdb_name,
			    scb->scb_sscb.sscb_ics.ics_db_access_mode,
			    dbdbcb);
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD5_IIDBDB;
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d3_ddb_desc_p = &dbdbcb->db_ddb_desc;
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p = 
	  &dbdbcb->db_ddb_desc.dd_d3_cdb_info;
	if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
	{
	    /* bad iidbdb. remove it */
	    (VOID)scs_ddbremove(dbdbcb);
	    /* release semaphore */
	    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	    sc0ePut(error, E_US0014_DB_UNAVAILABLE, NULL, 0);
	    return(E_DB_ERROR);
	}

	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD9_CLUSTERINFO;
	status = rdf_call(RDF_DDB, (PTR)&rdf_cb);
	if (status == E_DB_OK)
	{
	    /*
	    ** pass what RQF wants
	    */
	    qef_cb->qef_r3_ddb_req.qer_d16_anything_p = 
		(PTR)rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d9_cluster_p;
	    qef_cb->qef_r3_ddb_req.qer_d17_anything = 
		(i4)rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d8_nodes;
	}

	if (Sc_main_cb->sc_no_star_cluster || status)
	{
	    qef_cb->qef_r3_ddb_req.qer_d16_anything_p = (PTR) NULL;
	    qef_cb->qef_r3_ddb_req.qer_d17_anything = 0;
	}

	status = qef_call(QED_3SCF_CLUSTER_INFO, qef_cb);

	/* should not see an error and hence ignore status */
    }

    /*
    ** if this is an orphan DX recovery thread no need to do any more.
    ** the user is star and we trust us...
    */
    if (scb->scb_sscb.sscb_stype == SCS_S2PC)
    {
	qef_cb = scb->scb_sscb.sscb_qeccb;
	qef_cb->qef_r3_ddb_req.qer_d2_ldb_info_p = 
		&dbdbcb->db_ddb_desc.dd_d3_cdb_info;
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return(E_DB_OK);
    }

    /*
    ** The case translation semantics of the iidbdb must be applied
    ** to session identifiers for further queries to the iidbdb.
    ** The only identifiers we need be concerned about for the queries
    ** are dbname and username; however scs_icsxlate translates the
    ** whole set of identifiers (dba, real & effective user, etc.)
    */
    ddb_descp = &dbdbcb->db_ddb_desc;
    scb->scb_sscb.sscb_ics.ics_dbserv = (i4)ddb_descp->dd_d6_dbservice;
    MEcopy(ddb_descp->dd_d2_dba_desc.dd_u1_usrname, sizeof(DD_OWN_NAME),
		&scb->scb_sscb.sscb_ics.ics_dbowner);
    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_dbowner,
		scb->scb_sscb.sscb_ics.ics_xdbowner);

    status = scs_icsxlate(scb, error);
    if (DB_FAILURE_MACRO(status))
    {
	SETDBERR(error, 0, E_SC0123_SESSION_INITIATE);
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return (E_DB_ERROR);
    }

    /* if it is iimonitor session then no need to verify ddb and its access */

    if (scb->scb_sscb.sscb_stype == SCS_SMONITOR)
    {
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d4_usr_desc_p = &duser_desc;
	MEcopy((PTR)scb->scb_sscb.sscb_ics.ics_eusername,
	   sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
	   (PTR)duser_desc.dd_u1_usrname);
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD8_USUPER;
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p = 
	  &dbdbcb->db_ddb_desc.dd_d3_cdb_info;

	if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
	{
	    /* bad iidbdb. remove it */
	    (VOID)scs_ddbremove(dbdbcb);
	    /* release semaphore */
	    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	    sc0ePut(error, E_US0014_DB_UNAVAILABLE, NULL, 0);
	    return(E_DB_ERROR);
	}

	scb->scb_sscb.sscb_ics.ics_rustat = 
		    rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d7_userstat;

	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return(E_DB_OK);
	 
    }

    incache = scs_ddbsearch(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
		 Sc_main_cb->sc_kdbl.kdb_next,
		 Sc_main_cb->sc_kdbl.kdb_next, &dbcb);

    ddb_descp = &dbcb->db_ddb_desc;

    if( incache == FALSE ) 
    {
	/* if db is not already known then add */
	dbcb = scs_ddbadd(scb->scb_sscb.sscb_ics.ics_dbowner.db_own_name,
			  scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
			  scb->scb_sscb.sscb_ics.ics_db_access_mode,
			  dbcb);

	/* get DDB info */
	ddb_descp = &dbcb->db_ddb_desc;
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d3_ddb_desc_p = ddb_descp;
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD1_DDBINFO;
	MEcopy((PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
	       sizeof(ddb_descp->dd_d1_ddb_name),
	       (PTR)ddb_descp->dd_d1_ddb_name);
	ddb_descp->dd_d4_iidbdb_p = &dbdbcb->db_ddb_desc.dd_d3_cdb_info;
	if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
	{
	    /* inaccessible ddb. remove it */
	    (VOID)scs_ddbremove(dbcb);
	    /* release semaphore */
            CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	    sc0ePut(error, E_SC0395_RDF_DDB_FAILURE, NULL, 1, 
		sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
		(PTR)&scb->scb_sscb.sscb_ics.ics_dbname);
	    return(E_DB_ERROR);
	}
	dbcb->db_state = SCV_ADDED;
    }
    else /* DDB cache hit */
    {
	++dbcb->db_ucount;
    }

    scb->scb_sscb.sscb_dbcb = dbcb;
    scb->scb_sscb.sscb_ics.ics_opendb_id = (PTR)(SCALARP)dbcb->db_ddb_desc.dd_d7_uniq_id;
    scb->scb_sscb.sscb_ics.ics_udbid  = dbcb->db_ddb_desc.dd_d7_uniq_id;

    /* get users DB acccess status */
    rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD4_USRSTAT;
    rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p = 
	    &dbdbcb->db_ddb_desc.dd_d3_cdb_info;
    rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d4_usr_desc_p = &duser_desc;
    MEcopy((PTR)scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
	   sizeof(scb->scb_sscb.sscb_ics.ics_rusername.db_own_name),
	   (PTR)duser_desc.dd_u1_usrname);
    /* set the CDB DBA name to the scb */
    MEcopy((PTR)ddb_descp->dd_d2_dba_desc.dd_u1_usrname,
	   sizeof(scb->scb_sscb.sscb_ics.ics_xdbowner),
	   (PTR)&scb->scb_sscb.sscb_ics.ics_xdbowner);

    if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
    {
	/* release semaphore */
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	sc0ePut(error, E_US000C_INVALID_REAL_USER, NULL, 0);
	return(E_DB_ERROR);
    }

    /*
    ** At this point we have a valid dbcb for the database in question.
    ** Now, we translate ids to obey the case translation semantics of
    ** the DDB before opening the privileged connection with the CDB.
    */
    scb->scb_sscb.sscb_ics.ics_dbserv = (i4)ddb_descp->dd_d6_dbservice;
    MEcopy((PTR)&ddb_descp->dd_d2_dba_desc.dd_u1_usrname, sizeof(DD_OWN_NAME),
		(PTR)&scb->scb_sscb.sscb_ics.ics_dbowner);
    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_dbowner,
		scb->scb_sscb.sscb_ics.ics_xdbowner);

    status = scs_icsxlate(scb, error);
    if (DB_FAILURE_MACRO(status))
    {
	SETDBERR(error, 0, E_SC0123_SESSION_INITIATE);
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return (E_DB_ERROR);
    }
    rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p = &ddb_descp->dd_d3_cdb_info;
    if (incache == FALSE)
    {
	/* get LDB dbname and capabilities */

	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD3_LDBPLUS;
	if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
	{
	    /* inaccessible ddb. remove it */
	    (VOID)scs_ddbremove(dbcb);
	    /* release semaphore */
	    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	    sc0ePut(error, E_US0041_DBDB_CONFIG_INCON, NULL, 0);
	    return(E_DB_ERROR);
	}
    }
    else
    {
	/*
	** if CDB in the cache the privileged CBD is not open yet.
	** Ask RQF to open it, the FE's require this because of
	** the GCA_MOD_ASSOC they send us, if we do not RQF gets cofused.
	*/ 
	rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD10_OPEN_CDBASSO;
	if (rdf_call(RDF_DDB, (PTR)&rdf_cb) != E_DB_OK)
	{
	    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	    return(E_DB_ERROR);
	}
    }

    /* release semaphore */
    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);

    scb->scb_sscb.sscb_ics.ics_rustat = 
      rdf_cb.rdf_rb.rdr_r2_ddb_req.rdr_d4_usr_desc_p->dd_u4_status;
    STRUCT_ASSIGN_MACRO((*scb->scb_sscb.sscb_ics.ics_eusername),
			scb->scb_sscb.sscb_ics.ics_iucode);
    /*
    ** SHUT DOWN THE IIDBDB ASSOCIATION WE DO NOT NEED IT
    */

    qef_cb = scb->scb_sscb.sscb_qeccb;
    qef_dcb = (QEF_DDB_REQ *)&qef_cb->qef_r3_ddb_req;

    if (ddb_descp->dd_d4_iidbdb_p == NULL)
    {
	sc0ePut(error, E_US0010_NO_SUCH_DB, NULL, 0);
	return(E_DB_ERROR);
    }	
    qef_dcb->qer_d2_ldb_info_p = ddb_descp->dd_d4_iidbdb_p;
    status = qef_call(QED_2SCF_TERM_ASSOC, qef_cb);
    if ( status != E_DB_OK )
    {
	return( E_DB_ERROR );
    }
 
    /*
    ** Since the update system catalog bit in iiuser is obsolete,
    ** we must handle this privilege with care. 
    ** -- this code is quite similar to code in scs_dbdb_info() added by Ralph 
    **    for B1.  We need to pick this up in star even though we do not support
    **	  B1 because the permissions bits in iiuser.status are modified and that
    **	  effects star.
    ** This fixes bug 53304.
    */

    if (scb->scb_sscb.sscb_ics.ics_rustat  & DU_USECURITY)
	scb->scb_sscb.sscb_ics.ics_rustat |= DU_UUPSYSCAT;
    else
	/* Disallow update system catalog requests */
	scb->scb_sscb.sscb_ics.ics_rustat &= ~DU_UUPSYSCAT;

    /*
    ** Having reached here,
    ** we know that we have read valid information.
    ** Now, we simply have to check that the database is available
    ** and is allowed to be used.
    */	

    if ( (scb->scb_sscb.sscb_ics.ics_appl_code < DBA_MIN_APPLICATION ||
	    scb->scb_sscb.sscb_ics.ics_appl_code > DBA_MAX_APPLICATION ||
	    scb->scb_sscb.sscb_ics.ics_appl_code == DBA_IIFE
	 ) &&
	 ( (scb->scb_sscb.sscb_ics.ics_requstat 
		& scb->scb_sscb.sscb_ics.ics_rustat
	    ) != scb->scb_sscb.sscb_ics.ics_requstat
	 )
	)
    {
	SETDBERR(error, 0, E_US0002_NO_FLAG_PERM);
	return(E_DB_ERROR);
    }

    /* If -u was specified, ensure the real user has permission to do so. */
    if (    MEcmp(scb->scb_sscb.sscb_ics.ics_eusername->db_own_name,
		    scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
			    sizeof(*scb->scb_sscb.sscb_ics.ics_eusername)) != 0
	&&
	    (scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY) == 0
	&&
	    MEcmp(scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
			scb->scb_sscb.sscb_ics.ics_dbowner.db_own_name,
				sizeof(scb->scb_sscb.sscb_ics.ics_dbowner)) != 0
	&&
	    (scb->scb_sscb.sscb_ics.ics_appl_code < DBA_MIN_APPLICATION ||
		scb->scb_sscb.sscb_ics.ics_appl_code > DBA_MAX_APPLICATION ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_IIFE
	    )
	)
    {
	SETDBERR(error, 0, E_US0002_NO_FLAG_PERM);
	return(E_DB_ERROR);
    }

    /*
    ** Since real user can use -u, see if the effective user is $ingres,
    ** and grant catalog update permission if true.  First, translate
    ** "$ingres" to appropriate case.
    */
    {
	u_i4   xlate_temp;
	u_i4       l_id;
	char        tempstr[DB_OWN_MAXNAME];
	u_i4       templen;

	l_id = sizeof(DB_INGRES_NAME)-1;
	templen = DB_OWN_MAXNAME;
	xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_REG
						    | CUI_ID_STRIP;
	MEfill(sizeof(tempstr), ' ', tempstr);
	status = cui_idxlate((u_char *)DB_INGRES_NAME,
			 &l_id, (u_char *)tempstr, &templen, xlate_temp,
			 (u_i4 *)NULL, error);

	/* Safe to ignore status? */

	if (MEcmp((PTR)scb->scb_sscb.sscb_ics.ics_eusername->db_own_name,
		(PTR)tempstr,
		sizeof(*scb->scb_sscb.sscb_ics.ics_eusername)) == 0)
		scb->scb_sscb.sscb_ics.ics_requstat |= DU_UUPSYSCAT;

    }
    scb->scb_sscb.sscb_ics.ics_loc = 0;

    return(E_DB_OK);
}

/*
** Name: scs_audit_thread - the DBMS audit thread.
**
** Description:
**	The audit thread will only be present in servers that are running
**	with security auditing enabled (i.e. C2 and B1 servers). One of its
**	main roles in life is to maintain the consistency of the SXF audit 
**	state cache, this is a cached copy of the iisecuritystate catalog in 
**	the  iidbdb. Because of this the iidbdb database has to be open
**	whenever the audit thread is active. It is for this reason that
**	the main body of the audit thread loop resides in SCF (SCF if the
**	facility that manages the opening and closing of databases).
**
**	Each time that the audit thread is activated the iidbdb is opened, it
**	is closed again before the thread suspends itself. If the request to
**	open the iidbdb does not complete successfully this is not necessarily
**	deemed to be a thread fatal error. The error code is passed to SXF
**	where it will be evaluated to determine if the thread should be
**	terminated.
**
** Inputs:
**	scb			session control block for the audit thread.
**
** Outputs:
**	None.
**
** Returns:
**	DB_STATUS
**
** History:
**	23-nov-92 (markg/robf)
**	    Initial creation.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	27-Feb-2007 (drivi01)
**	    STmove statement of DB_DBDB_NAME was causing a trashed
**	    stack on windows b/c it was copying one byte more than
**	    expected and overwriting top of the stack.  
**	    The size of the information being copied should be 
**	    the size of the destination buffer not source buffer.
*/
DB_STATUS
scs_audit_thread( SCD_SCB *scb )
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    DB_ERROR	local_err;
    DB_DB_NAME	db_name;
    DB_DB_OWNER	db_owner;
    SCV_LOC	db_loc;
    DMC_CB	*dmc = scb->scb_sscb.sscb_dmccb;
    SXF_RCB	*sxf_rcb = scb->scb_sscb.sscb_sxccb;
    bool	dbdb_open = FALSE;

    for (;;)
    {
	/* Open the iidbdb */
	STmove(DB_DBDB_NAME, ' ', 
		sizeof(DB_DB_NAME), 
		db_name.db_db_name);

	STmove(DB_INGRES_NAME, ' ', 
		DB_OWN_MAXNAME, db_owner.db_db_owner.db_own_name);

	STRUCT_ASSIGN_MACRO(*((SCV_LOC *) Sc_main_cb->sc_dbdb_loc), db_loc);

	for (;;)	/* The main audit thread loop */
	{
            MEcopy("Connecting to IIDBDB", 20,
		scb->scb_sscb.sscb_ics.ics_act1);
            scb->scb_sscb.sscb_ics.ics_l_act1 = 20;
	    status = scs_dbopen(&db_name, &db_owner, &db_loc, scb, 
			&local_err, 0, dmc, &dbdb_dbtuple, NULL);
	    if (status != E_DB_OK)
	    {
		/* 
		** If we didn't manage to open the iidbdb successfully
		** it is not necessarily an error. We should pass the
		** error code to SXF to handle.
		*/
		sxf_rcb->sxf_msg_id = local_err.err_code;
	    }
	    else
	    {
		sxf_rcb->sxf_msg_id = 0L;
		dbdb_open = TRUE;
	    }

            MEcopy("Refreshing Audit State", 22,
		scb->scb_sscb.sscb_ics.ics_act1);
            scb->scb_sscb.sscb_ics.ics_l_act1 = 22;
	    /* Call SXF to perform the audit thread tasks */
	    status = sxf_call(SXC_AUDIT_THREAD, sxf_rcb);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&sxf_rcb->sxf_error, 0, NULL, 0);
		if (dbdb_open &&
		    scs_dbclose(&db_name, scb, &local_err, dmc) != E_DB_OK)
		{
		    sc0ePut(&local_err, 0, NULL, 0);
		}
		break;
	    }

	    /* Close the iidbdb */
            MEcopy("Disconnecting from IIDBDB", 25,
		scb->scb_sscb.sscb_ics.ics_act1);
            scb->scb_sscb.sscb_ics.ics_l_act1 = 25;
	    status = scs_dbclose(&db_name, scb, &local_err, dmc);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&local_err, 0, NULL, 0);
		break;
	    }
	    dbdb_open = FALSE;

            MEcopy("Waiting for Refresh Event", 25,
		scb->scb_sscb.sscb_ics.ics_act1);
            scb->scb_sscb.sscb_ics.ics_l_act1 = 25;

	    cl_status = CSsuspend(CS_INTERRUPT_MASK, 0, NULL);
	    if (cl_status != OK)
	    {
		if (cl_status == E_CS0008_INTERRUPTED)
		{
		    /* server shutdown time */
		    break;
		}
		else
		{
		    status = cl_status;
		    break;
		}
	    }
	}
	break;

    }

    if (status != E_DB_OK)
	sc0ePut(NULL, E_SC032A_AUDIT_THREAD_EXIT, NULL, 0);

    return (status);
}

/*
** Name: scs_check_term_thread - the DBMS check_terminate thread
**
** Description:
**	The check terminate thread checks session to see which ones
**	need to be terminated, then does so.
**
** Inputs:
**	scb			session control block for the check_terminate
**				thread
**
** Outputs:
**	None.
**
** Returns:
**	DB_STATUS
**
** History:
**	25-oct-93 (robf)
**	    Initial creation.
**	25-nov-93 (robf)
**          Allow lowest timeout of 1 sec (per LRC suggestion)
**	30-dec-93 (robf)
**          Changed name to session_check_interval, and a value of
**	    0 disables processing.
**	6-jan-94 (robf)
**          Option processing now in scdopt.c, so use value stored in
**	    Sc_main_cb for timeout interval.
**	07-feb-1997 (canor01)
**	    Overload terminator thread with handling the RDF cache
**	    synchronization event.  When other servers in installation
**	    signal an RDF invalidate event, process it here.
**	26-feb-1997 (canor01)
**	    If the event system has not been initialized, do not wait
**	    for events.
**	22-Sep_2004 (bonro01)
**	    Initialize status to prevent invalid E_SC0341_TERM_THREAD_EXIT
**	    errors.
**      11-mar-2009 (huazh01)
**          'timeout' now becomes a static i4.
**          if check term thread was interrupted, adjust timeout value
**          and possibly do a scs_scan_scbs() before going back to sleep.
**          (bug 121776)
*/
static DB_STATUS
scs_check_term_thread( SCD_SCB *scb )
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    DB_ERROR	local_err;
    static i4	timeout; 
    char 	*pmvalue;
    SCF_CB	scf_cb;
    SCF_ALERT   scfa;
    DB_ALERT_NAME aname;
    bool	event_wait;
    i4          time_now, time_prev;

    /*
    ** Check for alternate time
    */
    timeout=Sc_main_cb->sc_session_chk_interval;
    if(timeout<=0)
	return E_DB_OK;

    status = scs_reg_alert( scb, RDF_INV_ALERT_NAME );

    if ( status == E_DB_OK )
	event_wait = TRUE;
    else
	event_wait = FALSE;

    if ( event_wait == TRUE )
        scb->scb_sscb.sscb_astate = SCS_ASWAITING;

    for (status = E_DB_OK ;;)
    {

	MEcopy( "Waiting for next check time", 27,
	        scb->scb_sscb.sscb_ics.ics_act1 );
	scb->scb_sscb.sscb_ics.ics_l_act1 = 27;

        time_prev = TMsecs();

	cl_status = CSsuspend(CS_INTERRUPT_MASK|CS_TIMEOUT_MASK, 
			timeout, NULL);

	/* expected results are timeout and interrupt */
	if ( cl_status != E_CS0009_TIMEOUT &&
	     cl_status != E_CS0008_INTERRUPTED )
	{
	    status = cl_status;
	    break;
	}

	if ( cl_status == E_CS0008_INTERRUPTED )
	{

	    /* either processing an event, or shutting down */
	    if ( event_wait == TRUE )
	    {
	        cl_status = scs_handle_alert( scb, RDF_INV_ALERT_NAME );

	        /* 
	        ** if we processed an event, go back to sleep and
	        ** wait for next timeout/event
	        ** otherwise, shut down
	        */
	        if ( cl_status == OK )
                {
                    time_now = TMsecs(); 
                    timeout = timeout - (time_now - time_prev);

                    if (timeout <= 0)
                    {
                       MEcopy("Checking for sessions to Terminate", 34,
                               scb->scb_sscb.sscb_ics.ics_act1);
                       scb->scb_sscb.sscb_ics.ics_l_act1 = 34;
                       scs_scan_scbs(scs_chkterm_func, NULL);

                       timeout = Sc_main_cb->sc_session_chk_interval;
                    }
                    continue; 
                }
	    }
	    break;
	}
	else
	{

            timeout = Sc_main_cb->sc_session_chk_interval;

	    MEcopy("Checking for sessions to Terminate", 34,
		    scb->scb_sscb.sscb_ics.ics_act1);
	    scb->scb_sscb.sscb_ics.ics_l_act1 = 34;
	    /*
	    ** Loop over all sessions, checking for any which need to
	    ** be terminated
	    */
	    scs_scan_scbs(scs_chkterm_func, NULL);
        }
    }

    if (status != E_DB_OK)
	sc0ePut(NULL, E_SC0341_TERM_THREAD_EXIT, NULL, 0);

    return (status);
}

/*
** Name: scs_chkterm_func
**
** Description:
**	This function is called once for each known session, it checks
**	to see if it should be terminated, and if so calls scs_remove_sess
**	to drop the connection, so causing termination
**
** Input:
**	scb - scb to process
**
**	arg - currently unused, but needed from scs_scan_scbs
**	      interface
**
** Outputs:
**	none
** 
** History
**	25-oct-93 (robf)
**	   Created
**	30-dec-93 (robf)
**         Security audit dropping sessions, in the context of the session
**	   being dropped.
*/
static STATUS 
scs_chkterm_func(
	SCD_SCB *scb,
	PTR     arg
)
{
    STATUS status=OK;
    SYSTIME time_now;
    DB_ERROR dberror;

    /*
    ** Idle processing first
    */
    for(;;)
    {
	/*
	** Check if session has idle limit
	*/
        if(scb->scb_sscb.sscb_ics.ics_cur_idle_limit<=0)
	{
	   /*
	   ** No idle limit
	   */
	   scb->scb_sscb.sscb_is_idle=FALSE;
	   break;
	}
	/*
	** Check if still idle from previous scan
	*/
	if(scb->scb_sscb.sscb_is_idle)
	{
	   i4 length;
	    /*
	    ** Check if idle time has expired
	    */
	    TMnow(&time_now);
	    if(scb->scb_sscb.sscb_last_idle.TM_secs+
	       scb->scb_sscb.sscb_ics.ics_cur_idle_limit >
	       time_now.TM_secs)
	    {
		/*
		** Not idle long enough yet
		*/
		break;
	    }
	    length= cus_trmwhite(sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
		 	(char*)&scb->scb_sscb.sscb_ics.ics_rusername);
	    sc0ePut(NULL, I_SC0344_SESSION_DISCONNECT, NULL, 3,
                 sizeof(scb->scb_sscb.sscb_ics.ics_sessid),
                 	(PTR)scb->scb_sscb.sscb_ics.ics_sessid,
		 length, (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		 sizeof("IDLE_TIME")-1,(PTR)"IDLE_TIME");
	    /*
	    ** Idle time expired so drop the session
	    */
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
		&& DB_FAILURE_MACRO(sc0a_write_audit(
		    scb,
		    SXF_E_RESOURCE,	/* Type */
		    SXF_A_FAIL|SXF_A_LIMIT,  /* Action */
		    "IDLE_LIMIT",
		    sizeof("IDLE_LIMIT")-1,
		    NULL,			/* Object Owner */
		    I_SX2744_SESSION_IDLE_LIMIT,	/* Mesg ID */
		    FALSE,			/* Force record */
		    0,
		    &dberror		/* Error location */
		)) )
            {
	        /*
	        **  sc0a_write_audit should already have logged
	        **  any lower level errors
	        */
	        sc0ePut(NULL, E_SC023C_SXF_ERROR, NULL, 0);
            }
	    scs_remove_sess(scb);
	    break;
	}
	/*
	** If not doing anything mark as idle, 
	** a session is marked as idle when waiting on user input,
	*/
	if(scb->scb_sscb.sscb_state==SCS_INPUT)
	{
		scb->scb_sscb.sscb_is_idle=TRUE;
		TMnow(&scb->scb_sscb.sscb_last_idle);
		break;
	}
	break;
    }
    /*
    ** Now do connect time processing
    */
    for(;;)
    {
	/*
	** Check if session has connect limit
	*/
        if(scb->scb_sscb.sscb_ics.ics_cur_connect_limit<=0)
	{
	   /*
	   ** No connect limit
	   */
	   break;
	}

        TMnow(&time_now);
	/*
	** Check if connect time past limit
	*/
	if(time_now.TM_secs > scb->scb_sscb.sscb_ics.ics_cur_connect_limit+
	   scb->scb_sscb.sscb_connect_time.TM_secs)
	{
	   i4 length;
	   length= cus_trmwhite(sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
		 	(char*)&scb->scb_sscb.sscb_ics.ics_rusername);
	    /*
	    ** Past connect limit, so drop session
	    */
	    sc0ePut(NULL, I_SC0344_SESSION_DISCONNECT, NULL, 3,
                 sizeof(scb->scb_sscb.sscb_ics.ics_sessid),
                 	(PTR)scb->scb_sscb.sscb_ics.ics_sessid,
		 length, (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		 sizeof("CONNECT_TIME")-1,(PTR)"CONNECT_TIME");

	    /*
	    ** Security audit resource limit
	    */
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
		&& DB_FAILURE_MACRO(sc0a_write_audit(
		    scb,
		    SXF_E_RESOURCE,	/* Type */
		    SXF_A_FAIL|SXF_A_LIMIT,  /* Action */
		    "CONNECT_LIMIT",
		    sizeof("CONNECT_LIMIT")-1,
		    NULL,			/* Object Owner */
		    I_SX2745_SESSION_CONNECT_LIMIT,	/* Mesg ID */
		    FALSE,			/* Force record */
		    0,
		    &dberror		/* Error location */
		)) )
            {
	        /*
	        **  sc0a_write_audit should already have logged
	        **  any lower level errors
	        */
	        sc0ePut(NULL, E_SC023C_SXF_ERROR, NULL, 0);
            }
	    /*
	    ** Drop the connection
	    */
            scs_remove_sess(scb);
	}
	break;
    }
    return status;
}
/*
** Name: scs_secaudit_writer_thread - the DBMS audit writer thread.
**
** Description:
**	The audit writer thread will only be present in servers that are running
**	with security auditing enabled (i.e. C2 and B1 servers). Its main
**	role in life is to help flush the SXAP security audit queue, which
**	is maintained inside SXF. 
**
** Inputs:
**	scb			session control block for the audit thread.
**
** Outputs:
**	None.
**
** Returns:
**	DB_STATUS
**
** History:
**	14-apr-94 (robf)
**	    Initial creation.
*/
static DB_STATUS
scs_secaudit_writer_thread( SCD_SCB *scb )
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    DB_ERROR	local_err;
    SXF_RCB	*sxf_rcb = scb->scb_sscb.sscb_sxccb;

    for (;;)
    {
	    MEcopy("Writing security audit records", 30,
	    scb->scb_sscb.sscb_ics.ics_act1);
	    scb->scb_sscb.sscb_ics.ics_l_act1 = 30;

	    /* Call SXF to perform the audit thread task */
	    status = sxf_call(SXC_AUDIT_WRITER_THREAD, sxf_rcb);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&sxf_rcb->sxf_error, 0, NULL, 0);
		break;
	    }
	    MEcopy("Waiting for wakeup", 18,
	    scb->scb_sscb.sscb_ics.ics_act1);
	    scb->scb_sscb.sscb_ics.ics_l_act1 = 18;

	    /* Wait for next wakeup */
	    cl_status = CSsuspend(CS_INTERRUPT_MASK, 0, NULL);
	    if (cl_status != OK)
	    {
		if (cl_status == E_CS0008_INTERRUPTED)
		{
		    /* server shutdown time */
		    break;
		}
		else
		{
		    status = cl_status;
		    break;
		}
	    }
    }

    if (status != E_DB_OK)
	sc0ePut(NULL, E_SC0361_SEC_WRITER_THREAD_EXIT, NULL, 0);

    return (status);
}



/*
**	scs_open_iomdblist  - Open databases for IOMASTER server
**
** History:
**	??? mists of time ???
**	    written
**	3-Feb-2006 (kschendel)
**	    param removed from scs-dbdb-info call, fix here.
*/
static DB_STATUS
scs_open_iomdblist(SCD_SCB *scb,
		DMC_CB *dmc, 
		char *dbname, 
		DB_ERROR *error)
{
    DB_STATUS		status;

    STmove(dbname, ' ', sizeof(scb->scb_sscb.sscb_ics.ics_dbname), 
	scb->scb_sscb.sscb_ics.ics_dbname.db_db_name);

    scb->scb_sscb.sscb_ics.ics_appl_code = DBA_NORMAL_VFDB;
    scb->scb_sscb.sscb_ics.ics_fl_dbpriv |= DBPR_DB_ADMIN;
    scb->scb_sscb.sscb_ics.ics_rustat |= DU_USECURITY;
    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_EINGRES;
    STmove("$ingres", ' ', sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
	scb->scb_sscb.sscb_ics.ics_rusername.db_own_name);

    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_rusername,
	scb->scb_sscb.sscb_ics.ics_xrusername);

    status =  scs_dbdb_info(scb, dmc, error);
    if (status)
    {
    	sc0ePut(&dmc->error, 0, NULL, 0);
    	sc0ePut(NULL, E_SC0365_IOMASTER_OPENDB, NULL, 0);
    }

    return (status);
}

/*
** Name: scs_dbadduser - add to the number of users in the dbcb
**
** Description:
**	This routine grabs the Sc_main_cb semaphore and then chains down
**	the list of dbcb's looking for the database stipulated in 
**	scf_cb->scf_dbname, and then adds to the user count (db_ucount).
**	The routine is implimented so that SCF can be made aware of database
**	opens performed in DMF internal system threads, this avoids an 
**	attempt to delete a database from the server that may still be opened
**	by one of these threads.
**
** Inputs:
**	scf_cb
**	      .scf_dbname	dbname to add to
**
** Outputs:
**	None.
**
** Side Effects:
**	Adds to db_ucount in a dbcb
**
** History:
**	4-jun-96 (stephenb)
**	    Initial cration for DBMS replication Phase 1
**	30-jul-96 (stephenb)
**	    Fill in scb dbname field.
**	24-jan-1998 (walro03)
**	    'notfound' defined as bool is char, which is unsigned on some
**	    platforms.  Redefined as i4  to allow for correct (signed) return
**	    code from STbcompare.
**	12-jan-98 (stephenb)
**	    Log error E_SC012D_DB_NOT_FOUND, it doesn't happen elsewhere.
*/
DB_STATUS
scs_dbadduser(SCF_CB	*scf_cb, SCD_SCB *scb )
{
    STATUS	cl_stat;
    SCV_DBCB	*dbcb;
    i4		notfound;

    CLRDBERR(&scf_cb->scf_error);

    /*
    ** grab semaphore
    */
    cl_stat = CSp_semaphore(TRUE, &Sc_main_cb->sc_kdbl.kdb_semaphore);
    if (cl_stat != OK)
    {
        SETDBERR(&scf_cb->scf_error, 0, cl_stat);
	return(E_DB_ERROR);
    }
    /*
    ** search list for database (list is in sorted order)
    */
    for (dbcb = Sc_main_cb->sc_kdbl.kdb_next, notfound = TRUE; dbcb->db_next 
	!= Sc_main_cb->sc_kdbl.kdb_next; dbcb = dbcb->db_next)
    {
	if ((notfound = STbcompare((char *)&dbcb->db_dbname,
	    sizeof(dbcb->db_dbname), scf_cb->scf_dbname->db_db_name, 
	    sizeof(DB_DB_NAME), TRUE)) >= 0)
	    break;
    }
    if (notfound)
    {
	sc0ePut(&scf_cb->scf_error, E_SC012D_DB_NOT_FOUND, NULL, 0);

	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return (E_DB_ERROR);
    }

    dbcb->db_ucount++;
    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
    /*
    ** set the session's dbname field
    */
    STRUCT_ASSIGN_MACRO(dbcb->db_dbname, scb->scb_sscb.sscb_ics.ics_dbname);

    return (E_DB_OK);
}

/*
** Name: scs_dbdeluser - decrement the number of users in the dbcb
**
** Description:
**	This routine grabs the Sc_main_cb semaphore and then chains down
**	the dbcb list looking for the given database, it then decrements the
**	number of users (db_ucount). If db_ucount reaches zero E_DB_WARN is
**	returned to indicate that the db should be deleted from the server.
**	The routine was implimented to keep SCF informed of database opens and 
**	closes performed in DMF internal system threads.
**
** Inputs:
**	scf_cb
**	      .scf_dbname	- name of db in which to decrement user count
**
** Outputs:
**	None.
**
** Returns:
**	DB_STATUS
**
** Side effects:
**	decrements db_ucount in dbcb and mat delete the db from the server.
**
** History:
**	4-may-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
**	24-jan-97 (stephenb)
**	    Remove dbcb from the linked list if we are the last user out.
**	24-jan-1998 (walro03)
**	    'notfound' defined as bool is char, which is unsigned on some
**	    platforms.  Redefined as i4  to allow for correct (signed) return
**	    code from STbcompare.
*/
DB_STATUS
scs_dbdeluser(SCF_CB	*scf_cb, SCD_SCB *scb )
{
    STATUS	cl_stat;
    SCV_DBCB	*dbcb;
    i4		notfound;

    CLRDBERR(&scf_cb->scf_error);

    /*
    ** grab semaphore
    */
    cl_stat = CSp_semaphore(TRUE, &Sc_main_cb->sc_kdbl.kdb_semaphore);
    if (cl_stat != OK)
    {
        SETDBERR(&scf_cb->scf_error, 0, cl_stat);
	return(E_DB_ERROR);
    }
    /*
    ** search list for database (list is in sorted order)
    */
    for (dbcb = Sc_main_cb->sc_kdbl.kdb_next, notfound = TRUE; dbcb->db_next 
	!= Sc_main_cb->sc_kdbl.kdb_next; dbcb = dbcb->db_next)
    {
	if ((notfound = STbcompare((char *)&dbcb->db_dbname,
	    sizeof(dbcb->db_dbname), scf_cb->scf_dbname->db_db_name, 
	    sizeof(DB_DB_NAME), TRUE)) >= 0)
	    break;
    }
    if (notfound)
    {
        SETDBERR(&scf_cb->scf_error, 0, E_SC012D_DB_NOT_FOUND);
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	return (E_DB_ERROR);
    }

    dbcb->db_ucount--;
    if (dbcb->db_ucount <= 0)
    {
	/* 
	** remove dbcb from list and reclaim memory
	*/
        dbcb->db_next->db_prev = dbcb->db_prev;
        dbcb->db_prev->db_next = dbcb->db_next;
    	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	(VOID)sc0m_deallocate(0, (PTR *)&dbcb);
	return (E_DB_WARN);
    }

    CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
    return (E_DB_OK);
}

/*
** Name: scs_reg_alert - register an alert with event system
**
** Description:
**      Registers an alert with the event system, so a system thread
**	can be awakened by an SCE_RAISE.
**
** Inputs:
**	scb			session control block for thread
**      alert_name              name of alert to register
**
** Outputs:
**      None.
**
** Returns:
**      DB_STATUS		return value from scf_call(SCE_REGISTER)
**
** History:
**	07-feb-1997 (canor01)
**	    Created.
**	26-feb-1997 (canor01)
**	    If the event system has not been initialized, return an
**	    appropriate status indicating this.
*/
static DB_STATUS
scs_reg_alert( SCD_SCB *scb, char *alert_name )
{
    DB_STATUS   status = E_DB_OK;
    SCF_CB      scf_cb;
    SCF_ALERT   scfa;
    DB_ALERT_NAME aname;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_SCF_ID;
    scf_cb.scf_session = scb->cs_scb.cs_self;
    scf_cb.scf_ptr_union.scf_alert_parms = &scfa;

    MEfill( sizeof(DB_ALERT_NAME), 0, &aname.dba_alert );
    STmove( alert_name, ' ', sizeof(DB_EVENT_NAME), aname.dba_alert.db_ev_name );
    scfa.scfa_name = &aname;
    scfa.scfa_text_length = 0;
    scfa.scfa_user_text = NULL;
    scfa.scfa_flags = 0;

    status = scf_call( SCE_REGISTER, &scf_cb );

    if ( status != E_DB_OK &&
         scf_cb.scf_error.err_code == E_SC0280_NO_ALERT_INIT )
    {
	status = E_SC0280_NO_ALERT_INIT;
    }

    return ( status );
}

/*
** Name: scs_handle_alert - handle the alerts given to a system thread
**
** Description:
**      If a system thread has been notified of an alert, take the
**	appropriate action.
**
** Inputs:
**	scb			session control block of thread
**      alert_name              name of alert to register
**
** Outputs:
**      None.
**
** Returns:
**      STATUS			OK if handling alert
**				FAIL if no alert
**
** History:
**      07-feb-1997 (canor01)
**          Created for rdf_invalidate alert.
**      07-feb-2000 (horda03) Bug 103927
**          Need to reset last Alert and Number of alerts
**          once all alerts have been processed.
**	14-Jun-2001 (jenjo02)
**	    Reinit rdf_sess_id after it gets clobbered
**	    by MEcopy.
**	6-Jul-2006 (kschendel)
**	    Preventive:  maintain non-NULL-ness of dmf db id, but make sure
**	    RDF dies if it actually tries to use it in this server.
**	9-Mar-2009 (wanfr01)
**	    Bug 121768
**	    rdf_cb.caller_ref needs to be set to NULL to match rdf_info_blk 
*/
static STATUS
scs_handle_alert( SCD_SCB *scb, char *alert_name )
{
    STATUS	status = OK;
    DB_STATUS   rdf_status;
    SCE_AINSTANCE *inst, *next_inst;
    RDF_CB      rdf_cb;
    PID         pid;
    SCE_HASH    *sce_hash;

    /* process the rdf_invalidate alert */
    if ( STcompare( alert_name, RDF_INV_ALERT_NAME ) == 0 )
    {

	sce_hash = Sc_main_cb->sc_alert;
	sce_mlock(&scb->scb_sscb.sscb_asemaphore);

	if ( scb->scb_sscb.sscb_astate != SCS_ASNOTIFIED )
	{
	    /* we have not been notified of an event */
	    sce_munlock(&scb->scb_sscb.sscb_asemaphore);
	    return ( FAIL );
	}
        rdf_cb.length = sizeof(RDF_CB);
        rdf_cb.type = RDF_CB_TYPE;
        rdf_cb.ascii_id = RDF_CB_ID;
        rdf_cb.owner = (PTR)scb;
        rdf_cb.rdf_info_blk = NULL;
        rdf_cb.caller_ref = NULL;
	/* session ID set after rdf_rb is copied in */

        inst = (SCE_AINSTANCE *)scb->scb_sscb.sscb_alerts;
        while (inst != NULL)
        {
	    /*
	    ** The passed structure contains:
	    **	  PID of process where raised
	    **	  RDR_RB passed from original rdf_invalidate
	    */
            MECOPY_CONST_MACRO( inst->scin_value, sizeof(PID), &pid );

	    /* if this server generated alert, do nothing */
            if ( pid != Sc_main_cb->sc_pid )
            {
                MEcopy( inst->scin_value + sizeof(PID),
                        inst->scin_lvalue,
                        &rdf_cb.rdf_rb );
		/* the RDI_FCB is a pointer from the other server.
		** So is the db id, but maintain its non-null-ness, because
		** RDF invalidate uses that to control what to do.
		*/
                rdf_cb.rdf_rb.rdr_fcb = NULL;
		rdf_cb.rdf_rb.rdr_session_id = scb->cs_scb.cs_self;
		if (rdf_cb.rdf_rb.rdr_db_id != NULL)
		    rdf_cb.rdf_rb.rdr_db_id = (PTR) 1;
                rdf_status = rdf_call( RDF_INVALIDATE, (PTR)&rdf_cb );
            }
            /* Point to next one and free current one */
            next_inst = inst->scin_next;
            _VOID_ sce_free_node(inst->scin_lvalue > 0,
                                 sce_hash->sch_nhashb,
                                 &sce_hash->sch_nainst_free,
                                 (SC0M_OBJECT **)&sce_hash->sch_ainst_free,
                                 (SC0M_OBJECT *)inst);
            inst = next_inst;
        }
        scb->scb_sscb.sscb_alerts = NULL;
        scb->scb_sscb.sscb_last_alert = NULL;
        scb->scb_sscb.sscb_num_alerts = 0;
        scb->scb_sscb.sscb_astate = SCS_ASWAITING;
        sce_munlock(&scb->scb_sscb.sscb_asemaphore);
    }
    else
    {
	/* unknown alert */
	status = FAIL;
    }
    return ( status );
}
