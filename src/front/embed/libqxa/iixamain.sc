/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<cm.h>
# include	<st.h>
# include	<si.h>			/* needed for iicxfe.h */
# include       <xa.h>
# include       <iixa.h>
# include       <iixagbl.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iicxxa.h>
# include       <iixamain.h>
# include       <iilibqxa.h>
# include       <iixaapi.h>
# include       <iixainit.h>
# include       <generr.h>
# include       <erlq.h>
# include	<eqrun.h>	/* for IIsqconSysGen */
# include       <pc.h>
# include       <me.h>
# include       <iixashm.h>
# include	<tm.h>
# include	<cv.h>

GLOBALDEF   IITUX_LCB             IItux_lcb = {0,0,0,0};

/* Structures that are shared across modules */
    EXEC SQL INCLUDE SQLCA;

/* note that include for iilibq.h must *follow* the include for sqlca */
# include       <iilibq.h> 


/*
**  Name: iixamain.sc  - Contains main entrypoints called from LIBQ and from
**                       the TP system.
**
**  Description:
**	IIxa_open(), IIxa_close(), etc are called through the Ingres
**      xa_switch_t structure (exported via iixa.h, and defined in iixaswch.c)
**      by the TP system.
**
**  Defines:
**      IIxa_open        - Open an RMI, registered earlier w/the TP system.
**      IIxa_close       - Close an RMI.
**      IIxa_start       - Start/Resume binding a specific thread w/an XID.
**      IIxa_end         - End/Suspend the binding of a thread and an XID.
**      IIxa_rollback    - Propagate a rollback of a local xn bound to an XID.
**      IIxa_prepare     - Prepare a local xn bound to an XID.
**      IIxa_commit      - Commit a local xn bound to an XID.
**      IIxa_recover     - Recover a set of prepared XIDs for a specific RMI.
**      IIxa_forget      - a no-op routine, for now. (Ingres65)
**      IIxa_complete    - a no-op routine, for now. (Ingres65)
**      
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	08-sep-1992	- First written (mani)
**	19-nov-1992 (larrym)
**	    Removed functions that are called by libq, and placed them in
**	    another module (iilbqhdl.c).  The functions removed were:
**		IIxa_check_if_xa,
**		IIxa_get_and_set_xa_sid, and
**		IIxa_check_set_ingres_state.
**	09-mar-1993 (larrym)
**	    added connection name support
**	07-apr-1993 (larrym,mani)
**	    modified tracking of thread association to transaction branch
**      25-apr-1993 (mani)
**          added separate entry in state tables for IIxa_start(TMJOIN)
**	15-jun-1993 (larrym)
**	    moved tracking of tranaction branch association to a thread from
**	    xa_xn_thrd_cb to the xa_xn_cb (i.e., separate assocaition for
**	    each xid,rmid pair).
**	06-jul-1993 (larrym)
**	    fixed bug in lock_parse_open_shme; need case insensitive search
**	    for OPTIONS=
**	06-Aug-1993 (larrym)
**	    added a new assoc state IICX_XA_T_ASSOC_SUSP_MIG and updated
**	    the IIxa_xn_thrd_next_state to use it
**	23-Aug-1993 (larrym) 
** 	    Numerous changes:  Fixed bugs 53414, 54097, 54098, 54308, 54328,
**	    54330, 54372, 54373, 54374, 54379 in this integration.
**	    Cleaned up IIxa_xn_thrd_next_state table.  Modified check_fe_thread
**	    state to check, if passed an XID, its association with an
**	    xa_xn_thrd_cb againts the one just passed in.  
**	24-Aug-1993 (larrym)
**	    Fixed a couple of nit's and a bug.  Added a line-of-hypens to the
**	    trace output of IIxa_recover.  Modified IIxa_recover to use
**	    system generated sessions/connection_names.  Fixed "statement
**	    not reached error" in check_xa_xn_thrd_state
**	07-Sep-1993 (larrym)
**	    Fixed bug in parameter in call to IICXcompare_id in 
**	    check_fe_thread_state.
**      23-Sep-1993
**          The following changes are a result of the additional two members
**          which now form a part of the extended XID structure. This
**          structure contains the previous XA XID structure in addition to
**          these two members: branch_seqnum and branch_flag. The changes
**          primarily effect the xa_prepare, xa_rollback and xa_commit
**          functions and all the assocaited functions called with them.
**          connect_to_dbms() will now accept two additional parameters
**          both of type nat: branch_seqnum and branch_flag. The function
**          calls IICXformat_xa_extd_xid() function right after the call to
**          IICXformat_xa_xid() function. The xid_str now has the branch_seqnum
**          and branch_flag appended to the end of it. 
**	01-Oct-1993 (mhughes)
**	    Added in TUXEDO macros to hook in tuxedo bridge functionality.
**	    Macros defined in tuxedo main header iituxapi.h
**	    Moved IIXA_CALL_TYPE definitions (and all other global consts) to
**          iixagbl.h
**	06-Oct-1993 (larrym)
**	    Fixed prototype mismatch in STgetwords call in 
**	    lock_parse_xa_open_string().
**	12-Oct-1993 (larrym)
**	    Fixed bug in connect_to_iidbdb_setup_cursor where
**	    we weren't parsing the nodename correctly for 
**	    recover over ingres net.
**	12-Oct-1993 (larrym)
**	    Fixed bug in do_2phase_operation where we were trying to 
**	    reference the xa_xn_thrd_cx_cb unconditionally; this cb is
**	    not defined during recovery operations so it would core dump.
**	05-Nov-1993 (larrym)
**	    Modified lock_parse_tux_xa_open_string to put tmgroup into
**	    fe_thread_cb_p_p instead of xa_rmi_cb_p.
**	10-Nov-1993 (mhughes)
**	    Modified lock_parse_tux_xa_open_string to allow open string to 
**	    consist of 5 words.
**      11-nov-1993 (larrym)
**          integrated iyer's changes into this branch.
**	11-nov-1993 (larrym)
**	    modified IICX_XA_RMI_CB:  made connection_name a pointer
**	    into the usr_argv array.  Modified lock_parse* to deal with it.
**	    Also added field dbname_p to IICX_XA_RMI_CB, modified lock_parse*
**	    and various connect_to* to deal with it.  No more usr_argv[1]
**	    as the database name.
**	30-Nov-1993
**	    Moved first jump into IItux_main to be after ALL the open string
**	    parsing, and beefed up group name checking.
**	16-Dec-1993 (mhughes)
**	    Modified output_trace_header for different types of tracing.
**	17-Dec-1993 (mhughes)
**	    Modified IIxa_open & lock_parse_xa_open_string to take account
**	    of return statuses from any tuxedo functions called.
**	22-Dec-1993 (larrym)
**	    added rmid to connect_to_iidbdb_setup_cursor so it could be
**	    passed to tuxedo.
**	24-Dec-1993 (mhughes)
**	    Pass rmi cb and rmid from lock_parse_xa_open_string into
**	    lock_parse_tux_xa_open_string for ICAS recovery processing.
**	27-Jan-1994 (larrym)
**	    fixed bugs 59180, 59181, 59182
**	09-mar-1994 (larrym)
**	    fixed bug 59639 in lock_parse_tux_xa_open_string. We were
**	    requiring a minimum of 4 chars for TUXEDO group name.
**	09-mar-1994 (larrym)
**	    fixed bug 59645 in xa_open. We were returning
**	    an "Invalid Open String" when there was an internal error
**	09-mar-1994 (larrym)
**	    split up connect_to_iidbdb_setup_cursor into IIxa_connect_to_iidbdb
**	    and setup_recovery_cursor so TUXEDO could call 
**	    IIxa_connect_to_iidbdb without opening the cursor.  Moved 
**	    IIxa_connect_to_iidbdb declaration to iixagbl.h as it is now
**	    called from outside this file.  Also removed rmid from 
**	    IIxa_conect_to_iidbdb as it's not needed now.  Moved the call to 
**	    IItux_tms_recover from connect_to_iidbdb to IIxa_recover where 
**	    it belongs as IItux_tms_recover can now call connect_to_iidbdb 
**	    itself.
**	22-Apr-1994 (larrym)
*	    Fixed bugs 61137 and 61142.  
**	29-nov-95 (lewda02/stial02/thaju02)
**          Modified IIxa_start: added code for the association of 
**	    thread with global transaction.
**	    Modified do_2phase_esql: prepare/commit/rollback with xa_xid
**	    specified.
**      12-jan-1996 (stial01)
**          New INGRES-TUXEDO architecture:
**            The ICAS process is not needed anymore. We keep transaction
**            information in shared memory. The TMS can do transaction 
**            coordination via the following new (undocumented) SQL commands:
**                prepare to commit with xa_xid='...'
**                commit with xa_xid='...'
**                rollback with xa_xid='...'
**          Changes:
**          - IIxa_start: the AS will check to see if any of its transactions
**            were completed by the TMS and if so it will clean up the dbms
**            thread used for that transaction.
**            The AS will place XID information into shared memory.
**          - IIxa_end: the AS will update the state of the XID in shared mem.
**          - IIxa_do_2phase_operation: If XID isn't found by IICXfind_lock_cb, 
**            look for XID in shared memory.
**          - do_2phase_esql:
**               XA_PREPARE_CALL:  prepare to commit with xa_xid='...'
**               XA_COMMIT_CALL:   commit with xa_xid='...' 
**               XA_ROLLBACK_CALL: rollback with xa_xid='...' 
**          New routines added: 
**           IItux_shm_lock() Obtain EXCL lock on table 'tuxedo' in iidbdb.
**           IItux_shm_unlock() Release EXCL lock on table 'tuxedo' in iidbdb
**           IItux_find_xid() Find XID in the XN_ENTRYs for current AS
**           IItux_add_xid() Add XID to the XN_ENTRY array for this AS.
**           IItux_cleanup_threads() Cleanup threads if xn completed by TMS
**           IItux_tms_find_xid() Find entry for specified XID, branch sequence
**           IItux_max_seq() Return the entry with the max seqnum for this XID
**      25-jan-1996 (stial01)
**          Cleanup tuxedo changes...          
**          Fixed size of output buf in output_trace_header
**      30-may-1996 (stial01)
**          (B75073) When TM is Tuxedo, we need to get the server name
**          for each database connection made by an application server.
**          The TMS server must then connect to the same ingres server
**          to do the prepare/commit/rollback.
**          connect_to_dbms() Added server_in, server_out parameters
**          IItux_add_xid() Added server_name parameter
**          IIxa_start() Tuxedo: If new connection save server name
**          IIxa_do_2phase_operation() Tuxedo: Connect to same server 
**      29-jul-1996 (schte01)
**          For axp_osf, remove check for next_state in check_xa_xn_thread_
**      state. It will be the responsibility of the caller (IIxa_start,
**      IIxa_end, and do_2phase) to check the next_state. This is to allow
**      actions to be done against individual rmis prompted by Encina's
**      sequence of calls (...xa_end (rmid0), xa_prepare(rmid0), xa_end
**      (rmid1), xa_prepare(rmid1), etc.). There is no apparent reason to
**      group the state of rmis for the 2 phase operations and the spec
**      states that each rmi "supports independent transaction completion".
**      12-aug-1996 (stial01)
**          Store listen address in the XA XN CB with connection name (B78248)
**	15-aug-1997(angusm)
**	    Change validation in IIxa_do_2phase_operation() to allow for
**	    implicit or explicit rollback of transaction when TUXEDO
**	    timeout period is exceeded. (bug 83616). Also, add timestamp
**	    to trace output messages.
**	18-Dec-97 (gordy)
**	    LIBQ current session moved to thread-local-storage.
**	6-feb-1998(angusm)
**	    Back out previous change to validation in 
**	    IIxa_do_2phase_operation(): it doesn't help
**	6-feb-1998(angusm)
**	    bug 87706: allow use of server category in OPENSTRING.
**	    (change to connect_to_dbms(). Also trace name of database.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-apr-1999 (hanch04)
**	    Make same axp_osf changes for LP64
**	13-aug-2002 (stial01 for angusm)
**          Next version of fix for 83616: change IIxa_do_2phase_operation(), 
**          do_2phase_esql(). Cross integration of 433533 with minor changes
**          to make sure the xn_entries get cleared for reuse.
**      14-aug-2002 (stial01)
**          Fix session leak due to not properly clearing entries for reuse
**      21-aug-2002 (stial01)
**          IItux_cleanup_threads should return if there is an available 
**          XN_ENTRY. If not, iixa_start() can now handle this error.
**      22-oct-2002 (stial01)
**          connect_to_dbms: To start SHARED transaction correctly, 
**          set session with xa_xid MUST be first statement. Issue COMMIT
**          after select dbmsinfo(ima_server).
**          IItux_cleanup_threads() call validate_rollback
**          IIxa_start() send set session for every start (even TMJOIN)
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      18-Aug-2008 (hweho01)
**          To fix error XAER_INVAL from IICXvalidate_xa_xid() after     
**          receiving global transaction identifier from 64-bit TM 
**          (transaction manager), make the followings changes :   
**          1) Add IImap_xa_xid() routine to perform mapping in the 
**             the interface routines for 64-bit mode, so the ID can 
**             be recognized properly.   
**          2) To display the XA XID in 64-bit mode for tracing, a parm 
**             is added into output_trace_header routine, and a new   
**             routine IIformat_64_xa_xid is used for formatting in 64bit
**             mode exclusively.  
**	 4-Feb-2010 (wanfr01) Bug 123139
**	        Need to include cv.h for function definitions
*/

/* Global data structures */
/* Note that if next state goes to '0', then it's a protocol error. */

/* This maps to Table 6.1 in the X/Open XA Spec */

GLOBALDEF i4
IIxa_rmi_next_state[][IICX_XA_R_NUM_STATES + 1] =
{
  /* current XA call */     /* Current state */
  
  /* DUMMY    */  {0, 0, 0},
  /* xa_open  */  {0, IICX_XA_R_OPEN, IICX_XA_R_OPEN},
  /* xa_close */  {0, IICX_XA_R_CLOSED, IICX_XA_R_CLOSED}

};


GLOBALDEF i4  
IIxa_rmi_return_value[][IICX_XA_R_NUM_STATES + 1] =
{
  /* current XA call */     /* Current state */
  
  /* DUMMY       */  {XAER_PROTO,XAER_PROTO,XAER_PROTO},
  /* xa_open     */  {XAER_PROTO,     XA_OK,     XA_OK},
  /* xa_close    */  {XAER_PROTO,     XA_OK,     XA_OK},
  /* xa_start    */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_end      */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_rollback */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_prepare  */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_commit   */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_recover  */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_forget   */  {XAER_PROTO,XAER_PROTO,     XA_OK},
  /* xa_complete */  {XAER_PROTO,XAER_PROTO,     XA_OK},
};


/* This maps to Table 6.2 in the X/Open XA Spec (except there are 4 states) */

/* when we support migration, we'll have to update the 4th column */

GLOBALDEF i4
IIxa_xn_thrd_next_state[][IICX_XA_T_NUM_STATES] =
{
 /* current XA call */     /* Current state */
  
 /* DUMMY    */ 
	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC},

 /* xa_open  */ 
	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_ASSOC,	
	  IICX_XA_T_ASSOC_SUSP, IICX_XA_T_BAD_ASSOC},

 /* xa_close */ 
 	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_ASSOC_SUSP,	IICX_XA_T_BAD_ASSOC}, 

/* xa_start */ 
	{ IICX_XA_T_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_end   */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_NO_ASSOC,  	
	  IICX_XA_T_NO_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_rollback - the spec doesn't say it has to be ended SUCCESS */ 
 	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_ASSOC_SUSP, IICX_XA_T_BAD_ASSOC},

 /* xa_prepare - must be ended SUCCESS */ 
	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_commit - must be ended SUCCESS */ 
 	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_recover     */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC,    
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_forget      */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_complete    */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC,  IICX_XA_T_BAD_ASSOC},

 /* xa_start_rb    */ 
 	{ IICX_XA_T_NO_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_start_res   */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_start_join  */ 
  	{ IICX_XA_T_ASSOC,	IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_start_res_rb*/ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_BAD_ASSOC, 	
	  IICX_XA_T_NO_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_end_rb      */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_NO_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_end_susp    */ 
  	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_ASSOC_SUSP,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_end_susp_rb */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_NO_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_end_fail    */ 
 	{ IICX_XA_T_BAD_ASSOC,	IICX_XA_T_NO_ASSOC,	
	  IICX_XA_T_NO_ASSOC, 	IICX_XA_T_BAD_ASSOC},

 /* xa_prepare_rmerr*/
 	{ IICX_XA_T_BAD_ASSOC, IICX_XA_T_BAD_ASSOC,	
	  IICX_XA_T_BAD_ASSOC, IICX_XA_T_BAD_ASSOC}
};


/* This maps to Table 6.4 in the X/Open XA Spec */

GLOBALDEF i4
IIxa_xn_branch_next_state[][IICX_XA_X_NUM_STATES] =
{
 /* current XA call */     /* indexed by Current state */
  
 /* DUMMY    */       {0, 0, 0, 0, 0, 0},
 /* xa_open  */       {IICX_XA_X_NONEXIST, IICX_XA_X_ACTIVE,
					   IICX_XA_X_IDLE,
                             		   IICX_XA_X_PREPARED, 
					   IICX_XA_X_ROLLBACK_ONLY,
                                           IICX_XA_X_HEUR_COMPLETE},
 /* xa_close */       {IICX_XA_X_NONEXIST, 0, 
					   IICX_XA_X_IDLE,
					   IICX_XA_X_PREPARED,
                           		   IICX_XA_X_ROLLBACK_ONLY, 
					   IICX_XA_X_HEUR_COMPLETE},
 /* xa_start */       {IICX_XA_X_ACTIVE, 0, IICX_XA_X_ACTIVE, 0, 0, 0},
 /* xa_end   */       {0, IICX_XA_X_IDLE, 0, 0, 0, 0},
 /* xa_rollback    */ {0, 0, IICX_XA_X_NONEXIST, 
					   IICX_XA_X_NONEXIST,
                                           IICX_XA_X_NONEXIST, 
					   IICX_XA_X_NONEXIST},
 /* xa_prepare     */ {0, 0,IICX_XA_X_PREPARED,0, 0, 0},
 /* xa_commit      */ {0, 0,IICX_XA_X_NONEXIST, IICX_XA_X_NONEXIST, 0, 0},
 /* xa_recover     */ {IICX_XA_X_NONEXIST, IICX_XA_X_ACTIVE,
					   IICX_XA_X_IDLE,
                                           IICX_XA_X_PREPARED,
					   IICX_XA_X_ROLLBACK_ONLY,
                                           IICX_XA_X_HEUR_COMPLETE},
 /* xa_forget      */ {0, 0, 0, 0, 0, IICX_XA_X_NONEXIST},
 /* xa_complete    */ {0, 0, 0, 0, 0, IICX_XA_X_NONEXIST},
 /* xa_start_rb    */ {0, 0, IICX_XA_X_ROLLBACK_ONLY, 0, 0, 0},
 /* xa_start_res   */ {0, IICX_XA_X_ACTIVE, 0, 0, 0, 0},
 /* xa_start_join  */ {0, 0, IICX_XA_X_ACTIVE, 0, 0, 0},
 /* xa_start_res_rb*/ {0, 0, IICX_XA_X_ROLLBACK_ONLY, 0, 0, 0},
 /* xa_end_rb      */ {0, IICX_XA_X_ROLLBACK_ONLY, 0, 0, 0, 0},
 /* xa_end_susp    */ {0, IICX_XA_X_ACTIVE, 0, 0, 0, 0},
 /* xa_end_susp_rb */ {0, IICX_XA_X_ROLLBACK_ONLY, 0, 0, 0, 0},
 /* xa_end_fail    */ {0, IICX_XA_X_ROLLBACK_ONLY, 0, 0, 0, 0},
 /* xa_prepare_rmerr*/{0, 0, 0, 0, 0,0}
};

GLOBALDEF i4
IIxa_rb_xn_branch_next_state[][IICX_XA_X_NUM_STATES] =
{
 /* current XA call */     /* Current state */
  
 /* DUMMY    */ {0,0,0,0,0,0},
 /* xa_open  */ {IICX_XA_X_NONEXIST,IICX_XA_X_ACTIVE,IICX_XA_X_IDLE,
                             IICX_XA_X_PREPARED,IICX_XA_X_ROLLBACK_ONLY,
                                                   IICX_XA_X_HEUR_COMPLETE},
 /* xa_close */ {IICX_XA_X_NONEXIST, 0,IICX_XA_X_IDLE,IICX_XA_X_PREPARED,
                           IICX_XA_X_ROLLBACK_ONLY,IICX_XA_X_HEUR_COMPLETE},
 /* xa_start */ {0,0,IICX_XA_X_ROLLBACK_ONLY,0,0,                  0},
 /* xa_end   */ {0,IICX_XA_X_ROLLBACK_ONLY,0,0,0,                  0},
 /* xa_rollback    */ {0,0,IICX_XA_X_NONEXIST,IICX_XA_X_NONEXIST,
                                     IICX_XA_X_NONEXIST,           0},
 /* xa_prepare     */ {0,0,IICX_XA_X_NONEXIST,0,        0,         0},
 /* xa_commit      */ {0,0,IICX_XA_X_NONEXIST,0, 	0,         0},
 /* xa_recover     */ {IICX_XA_X_NONEXIST,IICX_XA_X_ACTIVE,IICX_XA_X_IDLE,
                             IICX_XA_X_PREPARED,IICX_XA_X_ROLLBACK_ONLY,
                                                   IICX_XA_X_HEUR_COMPLETE},
 /* xa_forget      */ {0,0,0,0,0,                       IICX_XA_X_NONEXIST},
 /* xa_complete    */ {0,0,0,0,0,                       IICX_XA_X_NONEXIST},
 /* xa_start_rb    */ {0,0,IICX_XA_X_ROLLBACK_ONLY,0,0,                  0},
 /* xa_start_res   */ {0,0,IICX_XA_X_ROLLBACK_ONLY,0,0,                  0},
 /* xa_start_join  */ {0,0,IICX_XA_X_ROLLBACK_ONLY,0,0,                  0},
 /* xa_start_res_rb*/ {0,0,IICX_XA_X_ROLLBACK_ONLY,0,0,                  0},
 /* xa_end_rb      */ {0,IICX_XA_X_ROLLBACK_ONLY,0,0,0,                  0},
 /* xa_end_susp    */ {0,IICX_XA_X_ROLLBACK_ONLY,0,0,0,                  0},
 /* xa_end_susp_rb */ {0,IICX_XA_X_ROLLBACK_ONLY,0,0,0,                  0},
 /* xa_end_fail    */ {0,IICX_XA_X_ROLLBACK_ONLY,0,0,0,                  0},
 /* xa_prepare_rmerr*/{0,                      0,0,0,0,                  0}
};

/* This Table is for tracing XA routine invocation */ 

GLOBALDEF struct {
    i4		xa_call;
    char	*xa_call_str;
}


IIxa_trace_xa_call[] =
{
 /* XA call */     		/* string literal of call */
 {0,				"invalid call"},
 {IIXA_OPEN_CALL,		"XA_OPEN"},
 {IIXA_CLOSE_CALL,		"XA_CLOSE"},
 {IIXA_START_CALL,		"XA_START"},
 {IIXA_END_CALL,		"XA_END"},
 {IIXA_ROLLBACK_CALL,		"XA_ROLLBACK"},
 {IIXA_PREPARE_CALL,		"XA_PREPARE"},
 {IIXA_COMMIT_CALL,		"XA_COMMIT"},
 {IIXA_RECOVER_CALL,		"XA_RECOVER"},
 {IIXA_FORGET_CALL,		"XA_FORGET"},
 {IIXA_COMPLETE_CALL,		"XA_COMPLETE"},
 {IIXA_START_RB_CALL,		"XA_START (ROLLBACK)"},
 {IIXA_START_RES_CALL,		"XA_START (RESUME)"},
 {IIXA_START_JOIN_CALL,         "XA_START (JOIN)"},
 {IIXA_START_RES_RB_CALL,	"XA_START (RESUME|ROLLBACK)"},
 {IIXA_END_SUSP_CALL,		"XA_END (SUSPEND)"},
 {IIXA_END_SUSP_RB_CALL,	"XA_END (SUSPEND|ROLLBACK)"},
 {IIXA_END_FAIL_CALL,		"XA_END (FAIL)"},
 {IIXA_PREPARE_RMERR_CALL,	"XA_PREPARE (RMERR)"},
 {0,				"\0"}
};


/* 
**  This Table is for translating return values into text 
**  Don't forget to update II_NUM_XA_RET_VALS if you add to this list.
*/
GLOBALDEF struct {
    int		xa_ret_val;
    char	*xa_ret_str;
} IIxa_trace_ret_val[] =
{
 {-2,				"XA_ER_AYNC"},
 {-3,				"XA_ER_RMERR"},
 {-4,				"XA_ER_NOTA"},
 {-5,				"XA_ER_INVAL"},
 {-6,				"XA_ER_PROTO"},
 {-7,				"XA_ER_RMFAIL"},
 {-8,				"XA_ER_DUPID"},
 {-9,				"XA_ER_OUTSIDE"},
 {0,				"XA_OK"},
 {3,				"XA_RDONLY"},
 {4,				"XA_RETRY"},
 {5,				"XA_HEURMIX"},
 {6,				"XA_HEURRB"},
 {7,				"XA_HEURCOM"},
 {8,				"XA_HEURHAZ"},
 {9,				"XA_NOMIGRATE"},
 {100,				"XA_RBROLLBACK"},
 {101,				"XA_RBCOMMFAIL"},
 {102,				"XA_RBDEADLOCK"},
 {103,				"XA_RBINTEGRITY"},
 {104,				"XA_RBOTHER"},
 {105,				"XA_RBPROTO"},
 {106,				"XA_RBTIMEOUT"},
 {107,				"XA_RBTRANSIENT"}
# define	II_NUM_XA_RET_VALS	24
};

/*
**  This table is for translating FLAG values into text
**  Don't forget to update II_NUM_XA_FLAG_VALS if you add to this list.
*/
GLOBALDEF struct {
  long        xa_flag_value;
  char        *xa_flag_str;
} IIxa_trace_flag_val[] =
{
 {0x00000000L,			"TMNOFLAGS"},
 {0x00000001L,			"TMREGISTER"}, 
 {0x00000002L,			"TMNOMIGRATE"}, 
 {0x00000004L, 			"TMUSEASYNC"},
 {0x80000000L, 			"TMASYNC"}, 
 {0x40000000L, 			"TMONEPHASE"}, 
 {0x20000000L, 			"TMFAIL"}, 
 {0x10000000L, 			"TMNOWAIT"},
 {0x08000000L, 			"TMRESUME"},
 {0x04000000L, 			"TMSUCCESS"},
 {0x02000000L, 			"TMSUSPEND"},
 {0x01000000L, 			"TMSTARTRSCAN"},
 {0x00800000L, 			"TMENDRSCAN"}, 
 {0x00400000L, 			"TMMULTIPLE"}, 
 {0x00200000L, 			"TMJOIN"}, 
 {0x00100000L,			"TMMIGRATE"}
# define        II_NUM_XA_FLAG_VALS      16
};


/* Static routines */

static IITUX_XN_ENTRY *
IItux_tms_find_xid(i4 seqnum, 
		   DB_XA_DIS_TRAN_ID *xid
		   );

static IITUX_XN_ENTRY *
IItux_find_xid(DB_XA_DIS_TRAN_ID *xid);

static IITUX_XN_ENTRY *
IItux_add_xid(DB_XA_DIS_TRAN_ID *xid,
	      int rmid,
	      i4  branch_seqnum,
	      char *server_name
	      );

static VOID IItux_cleanup_threads(bool *xn_entry_available);

static int
validate_rollback ( IITUX_XN_ENTRY	*xn_entry);

/* Check the state of an RMI, at the time of this XA call. Return an */
/* XA error code if the RMI is not in the proper state.              */

static  int
check_xa_rmi_state( i4                 xa_call,
                    int                rmid,
                    IICX_CB            **rmi_cx_cb_p_p
                  );

/* 
** Check the state of the transaction-to-thread association, at the time 
** of this XA call. Return an XA error code if it is not in the proper 
** state. 
*/

static  int
check_xa_xn_thrd_state( i4                 *xa_call,
			DB_XA_DIS_TRAN_ID  *xa_xid_p,
			IICX_CB            **xa_xn_thrd_cx_cb_p_p,
			IICX_XA_XN_THRD_CB **xa_xn_thrd_cb_p_p,
			IICX_CB            **xa_xn_cx_cb_p_p,
                        i4                 *next_state
                      );

/* 
/* Check the state of this thread, at the time of this XA call. Return an */
/* XA error code if the thread is not in the proper state.                */

static  int
check_fe_thread_state( i4                 xa_call,
                       DB_XA_DIS_TRAN_ID  *xa_xid_p,
                       IICX_CB            **fe_thread_cx_cb_p_p,
                       IICX_FE_THREAD_CB  **fe_thread_cb_p_p
                     );


/* Check the state of a specific xn branch, at the time of this XA call. */
/* Return an XA error code if the branch is not in the proper state.     */

static  int
check_xn_branch_state( i4              xa_call,
                       IICX_XA_XN_CB   *xa_xn_cb_p, 
                       i4              *next_state
                     );

/* Check the assoc state of a specific xn branch, at the time of XA call. */
/* Return an XA error code if the branch is not in the proper assoc state.*/

static  int
check_xn_branch_assoc_state( i4              xa_call,
                             IICX_XA_XN_CB   *xa_xn_cb_p, 
                             i4              *next_assoc_state
                           );

/* parse an XA open string, and store it's contents/flags in an RMI CB.  */
/* the RMI CB is freshly allocated. It is then placed in the processwide */
/* linked list and locked.                                               */

static  int
lock_parse_xa_open_string( i4               rmid,
                           IICX_CB          **rmi_cx_cb_p_p,
                           char             *xa_info,
                           i4               xa_info_len
			  );

/* parse an XA open string, and store it's contents/flags in an RMI CB.  */
/* the RMI CB is freshly allocated. It is then placed in the processwide */
/* linked list and locked.                                               */

static  int
lock_parse_tux_xa_open_string( IICX_XA_RMI_CB  *xa_rmi_cb_p,
			       i4               rmid
                             );

/* connect to INGRES server. The actual ESQL connect is done here ! */

static  int
connect_to_dbms( i4                xa_call,
                 IICX_FE_THREAD_CB *fe_thread_cb_p,
		 IICX_XA_RMI_CB    *xa_rmi_cb_p,
                 IICX_XA_XN_CB     *xa_xn_cb_p,
                 DB_XA_DIS_TRAN_ID *xa_xid_p,
		 i4                branch_seqnum,
		 i4                branch_flag,
		 char              *server_in
               );


/* 
** this is called by IIxa_do_2phase_operation() (prototyped in iixagbl.h).  
** It encapsulates all interactions for the XA 2PC calls, i.e. 
** xa_prepare/xa_commit/xa_rollback, with LIBQ
*/

static  int
do_2phase_esql( i4                 xa_call,
                DB_XA_DIS_TRAN_ID  *xid,
                IICX_FE_THREAD_CB  *fe_thread_cb_p,
                IICX_XA_XN_CB      *xa_xn_cb_p,
		 i4                branch_seqnum,
		 i4                branch_flag,
		IITUX_XN_ENTRY     *xn_entry
               );


/* called by xa_recover after it has called IIxa_connect_to_iidbdb */
static  int
setup_recovery_cursor( IICX_FE_THREAD_CB *fe_thread_cb_p,
		       char              *dbname_p,
                       long              flags
                 );

/* process a recovery scan, for which a set_sql has been done to the proper */
/* session, and a cursor is ready for the 'fetch' operation.                */
static  int
process_recovery_scan( IICX_FE_THREAD_CB *fe_thread_cb_p,
                       IICX_XA_RMI_CB    *xa_rmi_cb_p,
                       XID		 *xids,
                       long              count,
                       long              flags
                     );

/*
** Check open string & Auth string and if requested setup as a Tuxedo XA 
** installation
**
*/

static void
check_setup_tuxedo ( IICX_XA_RMI_CB  *xa_rmi_cb_p 
                   );

/*
** Map XA transaction ID which is generate from TM into the format of  
** DB_XA_DIS_TRAN_ID.  
** The routine will be called when it is in 64-bit mode.
*/

#if defined(LP64)
static DB_STATUS
IImap_xa_xid( i4  xa_call,
	      XID *xa_xid_p,
	      DB_XA_DIS_TRAN_ID *db_xid_p
            ); 
#endif /*  defined(LP64) */  
/*
** Format the XA transaction ID, which is generated from TM,
** into the a character buffer.
** The routine will be called when it is in 64-bit mode.
*/
#if defined(LP64)
static VOID
IIformat_64_xa_xid( XID    *xa_xid_p,
                    char   *text_buffer);
#endif   /* defined(LP64) */

/*
** Converts the XA XID from a text format to an 64 XID structure format,
** it would be passed back to 64-bit TM ( transaction manager).
** The routine will be called from process_recovery_scan() when it is  
** in 64-bit mode.
*/
#if defined(LP64)
static DB_STATUS
IIconv_to_64_struct_xa_xid(
              char                 *xid_text,
              i4                   xid_str_max_len,
              XID	           *xa_xid_p,
              i4                   *branch_seqnum,
              i4                   *branch_flag); 
#endif   /* defined(LP64) */

/* output standard trace information to the trace file.  This prints out the
** xa_call, the flags passed in, XID (if not null) ,  XA_XID  (if not null), 
** and rmid (if not null).
** Other trace information is written out by calling IIxa_fe_qry_trace_handler
*/

static void
output_trace_header( 	i4		xa_call,
			long		flags,
			DB_XA_DIS_TRAN_ID *xid,
			XID             *xa_xid,
			int		*rmid
		   );

#define	OUTPUT_TRACE_HEADER(xc,fl,xid,xa_xid,rmid) \
	output_trace_header( xc, fl, xid, xa_xid, rmid )

/* output the return value to the trace file, then output a line of hyphens
** to make it all tidy
*/
static void
output_trace_return_val( int	xa_ret_val );

#define TRACED_RETURN(xrv) {\
	if((IIcx_fe_thread_main_cb) && \
	  (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)) \
	{ \
	    output_trace_return_val( xrv ); \
	}\
	return(xrv);}


/*{
**  Name: IIxa_open  - Open an RMI, registered earlier with the TP system.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string.
**      rmid       - RMI identifier, assigned by the TP system, unique to the
**                   AS process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. Currently, we don't
**                           support TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - various internal errors.
**          XAER_INVAL     - illegal input arguments.
**          XAER_PROTO     - if the RMI is in the wrong state. Enforces the 
**                           state tables in the XA spec. 
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	13-jan-1993 (larrym)
**	    moved startup code before anything else to ensure we
**	    always have an fe_main_cb before we check the trace flag. 
**	    added tracing functionality.
**	18-Aug-1993 (larrym)
**	    Fixed bug where multiple duplicate opens (which is valid) would 
**	    increment the num_open_rmi's.  Related to bug 54330.
**	09-mar-1994 (larrym)
**	    fixed bug 59645.  We were returning an "Invalid Open String" 
**	    when there was an internal error
*/

int
IIxa_open(char      *xa_info,
          int       rmid,
          long      flags)
{
    IICX_CB         *rmi_cx_cb_p;
    DB_STATUS       db_status;
    IICX_XA_RMI_CB  *xa_rmi_cb_p;
    i4              xa_info_len;
    int             xa_ret_value = XA_OK;

    if (!IIxa_startup_done)
    {
	/* 
	** can't do tracing here because IIcx_fe_thread_main_cb->fe_main_flags
	** might not exist yet
	*/
        db_status = IIxa_startup();
        if (DB_FAILURE_MACRO(db_status))
        {
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("xa_open_call: IIxa_startup failed."));
            return( XAER_RMERR );
        }
    }

    /* outputtrace header info if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        char	outbuf[300];
    	OUTPUT_TRACE_HEADER( IIXA_OPEN_CALL, flags,
		             (DB_XA_DIS_TRAN_ID *)0, (XID *)0, &rmid);
        /* output xa_info string */
        STpolycat(3, ERx("\tOPEN string is: "), xa_info, ERx("\n"), outbuf);
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }

    /* Unsupported options/flags */
    if (flags & TMASYNC)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_open_call: XAER_ASYNC."));
        TRACED_RETURN( XAER_ASYNC );
    }
    else 
    if (flags != TMNOFLAGS)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_open_call: invalid flags: XAER_PROTO."));
        TRACED_RETURN( XAER_PROTO );
    }


    xa_info_len = STnlength(MAXINFOSIZE, xa_info);

    /* check if input arguments are valid */
    if ((xa_info == NULL) || (*xa_info == '\0') || 
        (xa_info_len == MAXINFOSIZE))
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_open_call: invalid input arguments: XAER_INVAL."));
        TRACED_RETURN( XAER_INVAL );
    }

    /* check for valid state */ 
    if ((xa_ret_value = check_xa_rmi_state(IIXA_OPEN_CALL,
                                            rmid,&rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_open_call: invalid RMI state: XAER_PROTO."));
        TRACED_RETURN( xa_ret_value );
    }

    if (rmi_cx_cb_p == NULL)
    {
	/* 
	** lock_parse_xa_open_string checks for TUXEDO, and sets up switch 
	*/
        xa_ret_value = lock_parse_xa_open_string( 
                             rmid, &rmi_cx_cb_p, xa_info, xa_info_len );
        if (xa_ret_value != XA_OK)
        {
	    if (xa_ret_value == XAER_INVAL)
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                    ERx("xa_open_call: invalid xa open string"));
	    }
	    /* else, internal error, assume message already spewed */
            TRACED_RETURN( xa_ret_value );
        }


        /* LOCK THE MAIN RMI CX CB CHECK */
        IIcx_xa_rmi_main_cb->num_open_rmis += 1;
        /* UNLOCK THE MAIN RMI CX CB CHECK */
    }
    else 
    {
        IICXlock_cb( rmi_cx_cb_p );
    }
    xa_rmi_cb_p = rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;

    xa_rmi_cb_p->xa_state = (i4)
         IIxa_rmi_next_state[ IIXA_OPEN_CALL ][ xa_rmi_cb_p->xa_state ];

    /*
    ** now check the lgmo table to make sure all willing commit
    ** XID's are consistent with TUXEDO's view of them 
    */
    xa_ret_value = XA_OK;
    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                      	IIXA_OPEN_CALL, 
		        IITUX_ACTION_TWO, 
			(PTR) rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p );
    }

    IICXunlock_cb( rmi_cx_cb_p );

    TRACED_RETURN( xa_ret_value );

} /* IIxa_open */




/*{
+*  Name: IIxa_close   - Close an RMI.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string. Currently,
**                   we will ignore this argument (Ingres65.)
**      rmid       - RMI identifier, assigned by the TP system, unique to the 
**                   AS process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if the RMI/thread is in the wrong state for this 
**                           call. Enforces the state table rules in the spec.
**	Errors:
**	    None.
A
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	18-Aug-1993 (larrym)
**	    Fixed bug 54330 - We now allow multiple XA_CLOSE's per the XA_SPEC.
**	23-Aug-1993 (larrym)
**	    Fixed bug 54379 - We only decrement the num_reg_rmi variable on
**	    the first close of an RMI.
**	18-Oct-1993 (mhughes)
**	    Added hook into tuxedo lib for server shutdown.
*/

int
IIxa_close(char      *xa_info,
           int       rmid,
           long      flags)
{


    IICX_CB            *rmi_cx_cb_p;
    IICX_CB            *fe_thread_cx_cb_p;
    DB_STATUS          db_status;
    IICX_XA_RMI_CB     *xa_rmi_cb_p;
    IICX_FE_THREAD_CB  *fe_thread_cb_p;
    int                xa_ret_value = XA_OK;

    if (!IIxa_startup_done)
    {
	/* if startup is not done, then everything is closed, so just return */
        return( XA_OK );
    }

    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        char	outbuf[300];
        /* output trace header info if tracing on...*/
        OUTPUT_TRACE_HEADER( IIXA_CLOSE_CALL, flags,
		             (DB_XA_DIS_TRAN_ID *)0, (XID *)0, &rmid);
        /* output xa_info string */
        STpolycat(3, ERx("\tCLOSE string is: "), xa_info, ERx("\n"), outbuf);
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }

    /* Unsupported options/flags */
    if (flags & TMASYNC)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_close_call: XAER_ASYNC."));
        TRACED_RETURN( XAER_ASYNC );
    }
    else if (flags != TMNOFLAGS)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_close_call: invalid flags: XAER_PROTO."));
        TRACED_RETURN( XAER_PROTO );
    }

    if ((xa_ret_value = check_xa_rmi_state(IIXA_CLOSE_CALL,
                                            rmid, &rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_close_call: invalid RMI state: XAER_PROTO."));
        TRACED_RETURN( xa_ret_value );
    }

    /* check if input arguments are valid */
    /* 
    ** the contents of the xa_close_info string are ignored, as long as it's
    ** not too long.
    */

    if (STnlength(MAXINFOSIZE,xa_info) == MAXINFOSIZE)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_close_call: invalid xa close string"));
        TRACED_RETURN( XAER_INVAL );
    }

    /* check the state of this thread of control. There must be no active */
    /* transaction branch associated with this thread of control.         */  

    xa_ret_value = check_fe_thread_state( IIXA_CLOSE_CALL,
                    NULL,
                    &fe_thread_cx_cb_p,
                    &fe_thread_cb_p );

    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_close_call: invalid FE THREAD state."));
        TRACED_RETURN( xa_ret_value );
    }


    if (rmi_cx_cb_p == NULL)
    {
	/* 
	** we didn't find this rmi so it was either closed already,
	** or it was never opened, as long as everything else is in
	** the correct state, we don't care.
	*/
	TRACED_RETURN( XA_OK );
    }
    /* else, deal with the rmi cb */

    IICXlock_cb( rmi_cx_cb_p );

    xa_rmi_cb_p = rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;
    
    if (xa_rmi_cb_p->xa_state == IICX_XA_R_OPEN)
    {
        /* LOCK THE MAIN RMI CX CB CHECK */
        IIcx_xa_rmi_main_cb->num_open_rmis -= 1;
        /* UNLOCK THE MAIN RMI CX CB CHECK */
    }

    xa_rmi_cb_p->xa_state = (i4)
           IIxa_rmi_next_state[ IIXA_CLOSE_CALL ][ xa_rmi_cb_p->xa_state ];

    IICXunlock_cb( rmi_cx_cb_p );

    if (IIcx_xa_rmi_main_cb->num_open_rmis == 0)
    {
	/* Shutdown TUXEDO stuff if enabled */
		
	if ( IIXA_CHECK_IF_TUXEDO_MACRO )
        {
	    /* CHECK This carries on even if return Status is BAD */
	    xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                          	IIXA_CLOSE_CALL, 
				IITUX_ACTION_ONE, 
				(PTR) rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p );
	}

        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            IIxa_fe_qry_trace_handler( (PTR) 0, 
	ERx("\tLast XA_CLOSE, shutting down XA and freeing session cache\n"), 
	    TRUE);
	}

        db_status = IIxa_shutdown();
        if (DB_FAILURE_MACRO(db_status))
        {
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("xa_close_call: IIxa_shutdown failed."));
            TRACED_RETURN( XAER_RMERR );
        }
    }

  TRACED_RETURN( xa_ret_value );

} /* IIxa_close */


/*{
+*  Name: IIxa_start   -  Start/Resume binding a thread w/an XID.
**
**  Description: 
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                       TMJOIN/TMRESUME/TMNOWAIT/TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RETRY       - if TMNOWAIT is set in flags. We will *always* 
**                           assume a blocking call and return this (Ingres65).
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS svr.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_DUPID     - XID already exists, when it's not expected to be
**                           "known" at this AS, i.e. it's not TMRESUME/TMJOIN.
**          XAER_OUTSIDE   - RMI/thread is doing work "outside" of global xns.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal input arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	06-Aug-1993 (larrym)
**	    Added check to make sure we aren't already associated with 
**	    another XID in this thread.
**	10-Aug-1993 (larrym)
**	    added calls to check_xn_branch_state, and 
**          check_xn_branch_assoc_state.
**	23-Aug-1993 (larrym)
**	    Fixed bugs 54097 and 54373.  We now check for an existing 
** 	    association with an XID (so we don't allow xa_start (xid1) 
**	    followed by xa_start(xid2)) and we now correctly deal with 
**	    TMNOWAIT flag (although we'll have to change that slightly
**	    for tuxedo).  Also modified to deal with new element of 
**	    xa_xn_thrd_cb wich is an array of nat's indexed by possible
**	    association states.  See iicxxa.sc for more info.
**	01-nov-1993 (larrym)
**	    Added TUXEDO support.
**	01-dec-1993 (larrym)
**	    Fixed bug where we weren't switching to a session we were
**	    reusing.
**	28-jan-1994 (larrym)(
**	    fixed bug 59181; in the multi-rmi case, if this is a 
**	    start(TMRESUME) then we don't mark the thread ACTIVE_PENDING, 
**	    but rather, we SET_SQL (SESSION) the the session that was
**	    active at the time the thread was ENDed(TMSUSPEND).  This
**	    means the application programmer no longer has to issue
**	    a SET CONNECTION after a START(TMRESUME).
**	29-nov-1995 (lewda02/stial02/thaju02)
**	    Added code for the association of thread with global
**	    transaction.
**      12-jan-1995 (stial01)
**          New INGRES-TUXEDO architecture:
**          The AS will check to see if any of its transactions were completed
**          by the TMS and if so it will clean up the dbms thread used for that
**          transaction. The AS will place XID information into shared memory.
**      29-jul-1996 (schte01)
**          Added next_xn_thrd_state check after call to check_xa_xn_thrd_state
**          call for axp_osf. (See comments in check_xa_xn_thrd_state).
**      12-aug-1996 (stial01)
**          Check listen address in the XA XN CB(B78248)
**      19-sep-2006 (ashco01/stial01) - Bug 116438
**          On failure of "set session with xa_id" during iixa_start() BEA
**          recommend returning RMFAIL in place of RMERR to force TM to 
**          re-issue xa_open() request on alternate IIDBMS. Added check for 
**          'association failure' and now return RMFAIL if communications 
**          with DBMS has been lost (because it no longer exists). The 
**          current xid (for which no work has been performed) is cleared 
**          to allow re-use on subsequent xa_start().
*/

int
IIxa_start(XID       *xa_xid,
           int       rmid,
           long      flags)
{
  IICX_ID            xa_xn_cx_id;
  IICX_ID            xa_xn_thrd_cx_id;
  IICX_CB            *rmi_cx_cb_p;
  IICX_CB            *fe_thread_cx_cb_p;
  IICX_CB	     fe_cx_cb;
  IICX_CB            *xa_xn_cx_cb_p;
  IICX_CB            *xa_xn_thrd_cx_cb_p;
  DB_STATUS          db_status;
  IICX_XA_RMI_CB     *xa_rmi_cb_p;
  IICX_XA_XN_CB      *xa_xn_cb_p;
  IICX_XA_XN_THRD_CB *xa_xn_thrd_cb_p;
  IICX_FE_THREAD_CB  *fe_thread_cb_p;
  IICX_FE_THREAD_CB  fe_cb;
  i4                 xid_found = FALSE;
  i4                 xa_call;
  int                xa_ret_value;
  int                xa_rb_value;
  i4                 next_xn_thrd_state;
  i4                 next_xn_branch_state;
  i4                 next_xn_branch_assoc_state;
  i4		     branch_seqnum;
  i4                 branch_flag;
  bool		     new_thrd_cb;
  DB_XA_DIS_TRAN_ID  db_xid;
  DB_XA_DIS_TRAN_ID  *xid;

  exec sql begin declare section;
    char   xid_str[IIXA_XID_STRING_LENGTH];
  exec sql end declare section;

#if defined(LP64)
    if( IImap_xa_xid( IIXA_START_CALL, (XID *)xa_xid, 
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
       return( XAER_INVAL );
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
         ERx("xa_start_call: LIBQXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO 
		&& !IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_start_call: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
	/* output trace header info if tracing on...*/
    	OUTPUT_TRACE_HEADER( IIXA_START_CALL, flags, xid, xa_xid,  &rmid);
    }

    /*
    ** check RMI state; basically, check that we got an OPEN for this RMID 
    */

    if ((xa_ret_value = check_xa_rmi_state(IIXA_START_CALL,
                                            rmid,&rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_start_call: invalid RMI state: XAER_PROTO."));
        TRACED_RETURN( xa_ret_value );
    }
  
    /* 
    ** check if input arguments are valid 
    */

    db_status = IICXvalidate_xa_xid((DB_XA_DIS_TRAN_ID *) xid);
    if (DB_FAILURE_MACRO( db_status ))
    {
        TRACED_RETURN( XAER_INVAL );
    }

    /* Unsupported options/flags */
    if (flags & TMASYNC)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_start_call: XAER_ASYNC."));
        TRACED_RETURN( XAER_ASYNC );
    }

    switch (flags & ~TMNOWAIT)
    {
      case TMNOFLAGS:
        xa_call = IIXA_START_CALL;
	break;
      case TMJOIN:
        xa_call = IIXA_START_JOIN_CALL;
	break;
      case TMRESUME:
        xa_call = IIXA_START_RES_CALL;
	break;
      default:
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_start_call: invalid flags: XAER_INVAL."));
        TRACED_RETURN( XAER_INVAL );
    }

    /* 
    ** check the thread state first (for OUTSIDE or APPL_ERROR)
    ** and check this thread's association to any transactions to make
    ** sure there is no conflict with the passed in XID.
    */

    xa_ret_value = check_fe_thread_state( xa_call,
                       			(DB_XA_DIS_TRAN_ID *)xid,
                                        &fe_thread_cx_cb_p,
                                        &fe_thread_cb_p );

    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_start_call: invalid FE THREAD state."));
        TRACED_RETURN( xa_ret_value );
    }

    /* Save in case of error and we want to restore */
    STRUCT_ASSIGN_MACRO(*fe_thread_cx_cb_p, fe_cx_cb);
    STRUCT_ASSIGN_MACRO(*fe_thread_cb_p, fe_cb);

    /* 
    ** check the XN THRD state next to make sure this XID is in the 
    ** correct state of associatedness (table 6.2).   (This routine 
    ** also locates (or creates, if appropriate) the xa_xn_thrd cb.)
    */

    xa_ret_value = check_xa_xn_thrd_state( &xa_call,
					   (DB_XA_DIS_TRAN_ID *)xid,
					   &xa_xn_thrd_cx_cb_p,
					   &xa_xn_thrd_cb_p,
					   &xa_xn_cx_cb_p,
                                           &next_xn_thrd_state );
#if defined(axp_osf) || defined(LP64)
    if (next_xn_thrd_state == IICX_XA_T_BAD_ASSOC)
		  xa_ret_value = XAER_RMERR;
#endif

    /* Remember if this is a new xa_xn_thrd_cb_p */
    if (xa_xn_thrd_cb_p->num_reg_rmis == 0)
	new_thrd_cb = TRUE;
    else
	new_thrd_cb = FALSE;

    /* Init xa_xn_thrd_cx_id in case we need to IICXdelete_cb */
    IICXsetup_xa_xn_thrd_id_MACRO( xa_xn_thrd_cx_id, (DB_XA_DIS_TRAN_ID *)xid);

    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_start_call: invalid XN THRD state."));

	/* 
	** free xa_xn_thrd_cx_id if the cb was just allocated, we have 
	** to check for newness, because it could be that a duplicate
	** xa_start came in, which is an error, but we don't want to 
	** delete the original, valid, cb.
	*/

        IICXunlock_cb( fe_thread_cx_cb_p );

	if ( (xa_call == IIXA_START_CALL) 
	  && (xa_xn_thrd_cb_p->num_reg_rmis == 0)) 
	{
            db_status = IICXdelete_cb( &xa_xn_thrd_cx_id );
	}
	else
	{
            IICXunlock_cb( xa_xn_thrd_cx_cb_p );
	}

        TRACED_RETURN( xa_ret_value );
    }

    /* 
    ** look for the specific xa_xn (xid,rmid) cb.  Sometimes we can find
    ** it by looking at the xa_xn_thrd cb, e.g., if this is a START(RESUME)
    ** so start there.
    */ 

    IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, (DB_XA_DIS_TRAN_ID *)xid, rmid );

    if (xa_xn_cx_cb_p != NULL)
    {
        IICXlock_cb( xa_xn_cx_cb_p );

        xid_found = !IICXcompare_id( &xa_xn_cx_id,
                                     &(xa_xn_cx_cb_p->cx_id));
    }

    if (!xid_found)
    {

	if ( IIXA_CHECK_IF_TUXEDO_MACRO )
	{
	    bool	xn_entry_available;
	    /*
	    ** Clean up any dbms threads whose transactions were
	    ** completed (commited/rolled back) by the TMS
	    **
	    ** If any threads are cleaned up, we will be able to reuse
	    ** a dbms connection for the transaction we are trying to
	    ** start now.
	    */
	    IItux_cleanup_threads(&xn_entry_available);
	    if (!xn_entry_available)
	    {
		IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
		   ERx("xa_start_call: LIBQTXXA Insufficient XN entries for this AS"));
		return( XAER_PROTO );
	    }
	}

	/* 
	** When we support association migration, then we need to try to 
	** find the xa_xn_cx_cb first to see if it was ended with suspend |
	** migrate in another thread.  If so, then TMRESUME is ok.  (CHECK)
	*/

        db_status = IICXfind_lock_cb( &xa_xn_cx_id, &xa_xn_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
	{
            IICXunlock_cb( xa_xn_cx_cb_p );
            TRACED_RETURN( XAER_RMERR );
	}
        
        xid_found = (xa_xn_cx_cb_p != NULL) ? TRUE : FALSE;

    }
        

    if (xa_call == IIXA_START_CALL)
    {
        if (xid_found)
	{
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_start_call: duplicate XID: XAER_DUPID"));
             IICXunlock_cb( xa_xn_cx_cb_p );
             TRACED_RETURN( XAER_DUPID );
	}
        else 
	{
            db_status = IICXcreate_lock_cb( &xa_xn_cx_id, 
                                                 NULL, &xa_xn_cx_cb_p );
            if (DB_FAILURE_MACRO( db_status ))
	    {
                IICXunlock_cb( xa_xn_cx_cb_p );
                TRACED_RETURN( XAER_RMERR );
	    }
     	 }

	 /* 
	 ** START (NOFLAGS) calls, initialize the branch states. 
	 */

	 next_xn_branch_state = IICX_XA_X_ACTIVE;
	 next_xn_branch_assoc_state = IICX_XA_X_ASSOC;
    }
    else if (!xid_found)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
            ERx("xa_start_call: XID expected, not found: XAER_NOTA"));
        IICXunlock_cb( xa_xn_cx_cb_p );
        TRACED_RETURN( XAER_NOTA );

    }
    else
    {
	/* 
	** a START(RESUME) call
        ** check the xn branch state, and the branch assoc state
	*/    

        xa_ret_value = check_xn_branch_state( xa_call,
                                            xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p,
                                            &next_xn_branch_state );
        if ((xa_ret_value >= XA_RBBASE) && (xa_ret_value <= XA_RBEND))
        {
            char	ebuf[30];

            xa_rb_value = xa_ret_value;

	    IICXunlock_cb( xa_xn_cx_cb_p );
	    CVna((i4) xa_ret_value, ebuf);
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D2_XA_XN_ROLLBACKONLY, 1, ebuf);
        }
        else if (xa_ret_value != XA_OK)
        {
	    IICXunlock_cb( xa_xn_cx_cb_p );
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
               ERx("xa_start_call: invalid XA XN branch state."));
	    TRACED_RETURN( xa_ret_value );
        }

        xa_ret_value = check_xn_branch_assoc_state( xa_call,
                                            xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p,
                                            &next_xn_branch_assoc_state );

    }


    /* 
    ** fire up a connection to the back-end, if this is a brand new connect 
    ** and if this is not a connection being re-used
    */
  
    xa_rmi_cb_p = rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;
    xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

    IICXlock_cb( fe_thread_cx_cb_p );
    if (xa_xn_cb_p->ingres_state == IICX_XA_X_ING_NEW)
    {
        branch_flag = branch_seqnum = 0;
        xa_ret_value = connect_to_dbms( IIXA_START_CALL,
                                      fe_thread_cb_p,
				      xa_rmi_cb_p, 
                                      xa_xn_cb_p,
                                      NULL,
                                      branch_seqnum,
				      branch_flag,
				      NULL);

        if (xa_ret_value != XA_OK)
        {
	    /* free xa_xn_cx_id */
            db_status = IICXdelete_cb( &xa_xn_cx_id );
            IICXunlock_cb( xa_xn_cx_cb_p );
            IICXunlock_cb( fe_thread_cx_cb_p );
            TRACED_RETURN( xa_ret_value );
        }
        xa_xn_cb_p->ingres_state = IICX_XA_X_ING_IN_USE;

	/* log the connection to the trace file */
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            char	outbuf[300];
	    STprintf(outbuf,
		ERx("\tconnected to %s (INGRES/OpenDTP session %d)\n"),
		xa_xn_cb_p->connect_name,
		xa_xn_cb_p->xa_sid);
	    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        }

    }
    else if (xa_xn_cb_p->ingres_state == IICX_XA_X_ING_IN_USE)
    {
        /* we are re-using a previously established DBMS connection/session */
        /* so the xa_xn_cb has already been updated with current info.      */
	/* check for rollback value (better not happen with XA_START_CALL)  */
        if (xa_xn_cb_p->xa_rb_value != 0)
	{
            if (xa_call != IIXA_START_CALL)
	    {
                xa_xn_cb_p->xa_state = IICX_XA_X_ROLLBACK_ONLY;
                IICXunlock_cb( xa_xn_cx_cb_p );
                IICXunlock_cb( fe_thread_cx_cb_p );
                TRACED_RETURN( xa_xn_cb_p->xa_rb_value );
	    }
	    else
	    {
		/* something's gone horribly wrong */
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
                    ERx("xa_start_call: xa_xn_cx_cb_p marked ROLLBACK ONLY."));
                IICXunlock_cb( xa_xn_cx_cb_p );
                IICXunlock_cb( fe_thread_cx_cb_p );
                TRACED_RETURN( XAER_RMERR );
	    }
	}
	else 
	{
	    /* need to switch to the re-use session */
	    EXEC SQL BEGIN DECLARE SECTION;
	    i4		reuse_session = xa_xn_cb_p->xa_sid;
	    EXEC SQL END DECLARE SECTION;
            i4     	save_ingres_state;
	    
            save_ingres_state = fe_thread_cb_p->ingres_state; 
            fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
            {
                char	outbuf[300];
	        /* don't need to do anything, except trace the connection */
	        STprintf(outbuf,
		   ERx("\treusing connection %s (INGRES/OpenDTP session %d)\n"),
		   xa_xn_cb_p->connect_name,
		   xa_xn_cb_p->xa_sid);
	        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	    }

	    EXEC SQL SET_SQL (SESSION = :reuse_session);
            fe_thread_cb_p->ingres_state = save_ingres_state;
	}
    }
    else
    { 
	char	ebuf[120];
        /* the ingres state for this XA XN CB is an illegal/unexpected state */
	STprintf(ebuf, 
	    ERx("xa_start_call: INVALID xa_xn_cb_p->ingres_state = %x."),
	    xa_xn_cb_p->ingres_state);
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1, ebuf);
        TRACED_RETURN( XAER_RMERR );
    }

    /* 
    ** set xa xn cb's xa state.  Actually, if we made it this far, it's
    ** going to be IICX_XA_X_ACTIVE, but just to be safe, we'll use the
    ** value returned from check_xn_branch_state.
    */

    xa_xn_cb_p->xa_state = next_xn_branch_state;

    /*
    ** set the xa xn cb's xa assoc state, again it will be IICX_XA_X_ASSOC
    ** if we got this far.
    */

    xa_xn_cb_p->xa_assoc_state = next_xn_branch_assoc_state;
    
    IICXunlock_cb( xa_xn_cx_cb_p );

    IICXlock_cb( xa_xn_thrd_cx_cb_p );

    /* 
    ** since this is an XA_START, we set the xa_xn_thrd_cb_p->num_reg_rmis
    ** equal to the number of OPEN'd RMI's (i.e., static registration).  When
    ** we support dynamic registration (via ax_reg) num_reg_rmis will get
    ** set there.
    */
    if( xa_xn_thrd_cb_p->num_reg_rmis == 0)
    {
        xa_xn_thrd_cb_p->num_reg_rmis = IIcx_xa_rmi_main_cb->num_open_rmis;
    }

    /* 
    ** set the xa_xn_thrd_cx_cb's state counter 
    */

    xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] += 1;

    if (xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] == 
	xa_xn_thrd_cb_p->num_reg_rmis)
    {
	/* 
	** all of the xa_start's for this xid have been called (one for each  
	** RMI) so now we can set the thread assoc state, and zero out the
        ** counters.
	*/

	xa_xn_thrd_cb_p->xa_assoc_state = next_xn_thrd_state;
	xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] = 0;

        if (xa_call == IIXA_START_RES_CALL)
	{
	    EXEC SQL BEGIN DECLARE SECTION;
	    i4		current_session;
	    i4		resume_session;
	    EXEC SQL END DECLARE SECTION;

	    /*
	    ** resume the last known current session for this XID 
	    */
	    resume_session = xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p->xa_sid;
	    xa_xn_cb_p->xa_sid;

	    if (resume_session == 0)
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
                  ERx("xa_start_res_call: Trying to resume to session 0")); 
		return(XAER_RMERR);
	    }

            fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

	    EXEC SQL SET_SQL (SESSION = :resume_session);
	    EXEC SQL INQUIRE_SQL (:current_session = SESSION);
	    if (current_session != resume_session)
	    {
		char outbuf[120];

		STprintf(outbuf, ERx("xa_start_res_call: Attempted to resume to session %d, current session is %d"), resume_session, current_session);

                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1, outbuf);
		return(XAER_RMERR);
	    }
	    
            fe_thread_cb_p->ingres_state = IICX_XA_T_ING_ACTIVE;

	    if ( (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	      && (xa_xn_thrd_cb_p->num_reg_rmis > 1))
            {
		char outbuf[80];

		STprintf(outbuf,
	         ERx("\tAll RMI's for this XID resumed. Resuming session %d\n"),
		 resume_session);

	        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
            }
	    
	}
	else
	{
            /* LOCK the XA RMI MAIN CB. CHECK !!! */

            /*
            ** Set the initial ingres_state of the thread.  If we have more than
            ** one RMI, then we don't know (yet) which one to make active, so
            ** we set the state to an "active, pending SET CONNECTION" state.
            */

            if (IIcx_xa_rmi_main_cb->num_open_rmis == 1)
            {
                fe_thread_cb_p->ingres_state = IICX_XA_T_ING_ACTIVE;
            }
            else 
            {
                fe_thread_cb_p->ingres_state = IICX_XA_T_ING_ACTIVE_PEND;
            }

            /* UNLOCK the XA RMI MAIN CB. CHECK !!! */
            /*
            **  Now link all the cb's together.
            */

            xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p = xa_xn_cx_cb_p;


	    if ( (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	      && (xa_xn_thrd_cb_p->num_reg_rmis > 1))
            {
	        IIxa_fe_qry_trace_handler( (PTR) 0, 
	         ERx(
	      "\tAll RMI's for this XID started.  THREAD state is now ACTIVE\n"
	         ), TRUE);
            }
	}
        fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p = xa_xn_thrd_cx_cb_p;
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	IITUX_XN_ENTRY    *xn_entry;
	IITUX_XN_ENTRY    *max_xn_entry;

	xn_entry = IItux_find_xid((DB_XA_DIS_TRAN_ID *)xid);
	branch_flag = 0;
	if (xn_entry)
	{
	    xn_entry->xn_state = 0;
	    branch_seqnum = xn_entry->xn_seqnum;
	}
	else
	{
	    /*
	    ** Association of thread with global transaction.
	    ** Need to use ESQL, but for now use libq calls.
	    ** EXEC SQL SET SESSION WITH XA_XID = ;
	    */
	    
	    max_xn_entry = IItux_max_seq((DB_XA_DIS_TRAN_ID *)xid);
	    if (max_xn_entry)
		branch_seqnum = max_xn_entry->xn_seqnum + 1;
	    else
		branch_seqnum = 1;
	}

	IICXformat_xa_xid((DB_XA_DIS_TRAN_ID *)xid, xid_str);
	IICXformat_xa_extd_xid(branch_seqnum, branch_flag, xid_str);
	IIsqInit(&sqlca);
	IIwritio(0,(short *)0,1,32,0,"set session with xa_xid=");
	IIvarstrio((short *)0,1,32,0,xid_str);
	IIsyncup((char *)0,0);
	if (sqlca.sqlcode < 0)
	{
            if (sqlca.sqlcode == -37000)
            {
                /* DBMS communication lost - DBMS may have exited */
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_start_call: set session failed - Association Failure - xid released"));

                /* Reset XA XN to non-existent state, this connection gone */
                xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;

		/* This xa_xn_cb has no connection */
		xa_xn_cb_p->ingres_state = IICX_XA_X_ING_NEW;

		STRUCT_ASSIGN_MACRO(fe_cx_cb, *fe_thread_cx_cb_p);
		STRUCT_ASSIGN_MACRO(fe_cb, *fe_thread_cb_p);

		if (new_thrd_cb)
		    db_status = IICXdelete_cb( &xa_xn_thrd_cx_id );
	
		if (xa_call == IIXA_START_CALL)
		    IICXdelete_cb( &xa_xn_cx_id );

                /* return RMFAIL, TM should re-issue xa_open() */
                TRACED_RETURN(XAER_RMFAIL);
            }
            else
            {
	        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
		    ERx("xa_start_call: set session failed"));
	        TRACED_RETURN(XAER_RMERR);
            }   
	}

	if (!xn_entry)
	{
	    xn_entry = IItux_add_xid((DB_XA_DIS_TRAN_ID *)xid,
				rmid, branch_seqnum, xa_xn_cb_p->listen_addr);
	    if (!xn_entry)
	    {
		IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
		   ERx("xa_start_call: max concurrent transactions for AS"));
		TRACED_RETURN( XAER_RMERR);
	    }
	}
    }

    IICXunlock_cb( xa_xn_thrd_cx_cb_p );
    IICXunlock_cb( fe_thread_cx_cb_p );

    TRACED_RETURN( XA_OK );

} /* IIxa_start */




/*{
+*  Name: IIxa_end   -  End/Suspend the binding of a thread w/an XID..
**
**  Description: 
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the
**                   AS process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                       TMSUSPEND/TMMIGRATE/TMSUCCESS/TMFAIL/TMASYNC
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_NOMIGRATE   - if TMSUSPEND|TMMIGRATE is specified. We won't
**                          have support for Association Migration in Ingres65.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	06-Aug-1993 (larrym)
**	    Added check to make sure we aren't already associated with 
**	    another XID in this thread.
**	10-Aug-1993 (larrym)
**	    added calls to check_xn_branch_state, and 
**          check_xn_branch_assoc_state.
**	23-Aug-1993 (larrym)
**	    Fixed bugs 54097 and 54373.  We now check for an existing 
** 	    association with an XID (so we don't allow xa_end (xid2) 
**	    after xa_start(xid1))  Also modified to deal with new element of 
**	    xa_xn_thrd_cb wich is an array of nat's indexed by possible
**	    association states.  See iicxxa.sc for more info.
**	27-Jan-1994 (larrym)
**	    Fixed bug 59183.  You can now XA_END(TMSUCCESS) an XID that
**	    was previously XA_END(TMSUSPEND) even if we're already working
**	    on a different XID.
**	25-Apr-1994 (larrym)
**	    Fixed bug 61142 (which is 59183 revisited).  While we allowed
**	    XA_END(TMSUCCESS) even if we were already assoc'd to another
**	    XID, we unconditionally marked the fe_thread ingres_state
**	    to IDLE, so if the user tried to do more work on the currently
**	    assoc'd XID, they got an OUTSIDE error.  So now we only
**	    mark the fe_thread ingres_state (and the corresponding
**	    fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p) if we're working
**	    on the XID we're assoc'd to.
**      12-jan-1995 (stial01)
**          The AS will update the state of the XID in shared memory.
**      29-jul-1996 (schte01)
**          Added next_xn_thrd_state check after call to check_xa_xn_thrd_state
**          call for axp_osf. (See comments in check_xa_xn_thrd_state).
*/

int
IIxa_end(XID       *xa_xid,
         int       rmid,
         long      flags)
{

  IICX_ID            xa_xn_cx_id;
  IICX_ID            xa_xn_thrd_cx_id;
  IICX_CB            *rmi_cx_cb_p;
  IICX_CB            *fe_thread_cx_cb_p;
  IICX_CB            *xa_xn_thrd_cx_cb_p;
  IICX_CB            *xa_xn_cx_cb_p;
  DB_STATUS          db_status;
  IICX_XA_XN_CB      *xa_xn_cb_p;
  IICX_XA_XN_THRD_CB *xa_xn_thrd_cb_p;
  IICX_FE_THREAD_CB  *fe_thread_cb_p;
  i4                 xid_found = FALSE;
  i4                 xa_call;
  int                xa_ret_value = XA_OK;
  int                xa_rb_value = XA_OK;
  i4                 next_xn_thrd_state;
  i4                 next_xn_branch_state;
  i4                 next_xn_branch_assoc_state;
  DB_XA_DIS_TRAN_ID  db_xid;
  DB_XA_DIS_TRAN_ID  *xid;

#if defined(LP64)
    if( IImap_xa_xid( IIXA_END_CALL, (XID *)xa_xid, 
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
      return ( XAER_INVAL ); 
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: LIBQXA not initialized properly earlier."));
	return( XAER_PROTO );
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO 
		&& !IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }
    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
	/* output trace header info if tracing on...*/
	OUTPUT_TRACE_HEADER( IIXA_END_CALL, flags, xid, xa_xid,  &rmid);
    }


    if ((xa_ret_value = check_xa_rmi_state(IIXA_END_CALL,
                                            rmid,&rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid RMI state: XAER_PROTO."));
	TRACED_RETURN( xa_ret_value );
    }

    /* 
    ** check if input arguments are valid 
    */

    db_status = IICXvalidate_xa_xid((DB_XA_DIS_TRAN_ID *) xid);
    if (DB_FAILURE_MACRO( db_status ))
    {
	TRACED_RETURN( XAER_INVAL );
    }

    /* 
    ** check various flags, set xa_call type. 
    */
    switch (flags)
    {
    /* Unsupported options/flags */
    case TMASYNC:
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: XAER_ASYNC."));
	TRACED_RETURN( XAER_ASYNC );
    case TMNOFLAGS:
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid flags: XAER_PROTO."));
	TRACED_RETURN( XAER_PROTO );
    /* supported options/flags */ 
    case (TMSUSPEND | TMMIGRATE):
	/* 
	** supposed to CHECK that only one *xid is currently suspended with
	** TMMIGRATE (page 38, xa interface def).  But for now, we don't
	** support it.
	*/
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid flags.  Migration not supported (yet): XA_NOMIGRATE."));
	TRACED_RETURN( XA_NOMIGRATE );
    case TMSUSPEND:
	xa_call = IIXA_END_SUSP_CALL;
	break;
    case TMSUCCESS:
	xa_call = IIXA_END_CALL;
	break;
    case TMFAIL:
	xa_call = IIXA_END_FAIL_CALL;
	break;
    default:
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid flags: XAER_PROTO."));
	TRACED_RETURN( XAER_PROTO );
    }
       
    /* 
    ** check the thread state first 
    */

    if (xa_call == TMSUSPEND)
    {
	/* should only happen on the current xa_xn_thrd. */
        xa_ret_value = check_fe_thread_state( xa_call,
                         (DB_XA_DIS_TRAN_ID *)xid,
                         &fe_thread_cx_cb_p,
                         &fe_thread_cb_p );
    }
    else
    {
	/* you can end SUCCESS/FAIL a thread that was SUSPENDed earlier. */
        xa_ret_value = check_fe_thread_state( xa_call,
                         NULL,
                         &fe_thread_cx_cb_p,
                         &fe_thread_cb_p );
    }
    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid FE THREAD state."));
	TRACED_RETURN( xa_ret_value );
    }

    /* 
    ** check the XN THRD state next to make sure this XID is in the 
    ** correct state of associatedness (table 6.2).   (This routine 
    ** also locates (or creates, if appropriate) the xa_xn_thrd cb.)
    */

    xa_ret_value = check_xa_xn_thrd_state( &xa_call,
					   (DB_XA_DIS_TRAN_ID *)xid,
					   &xa_xn_thrd_cx_cb_p,
					   &xa_xn_thrd_cb_p,
					   &xa_xn_cx_cb_p,
                                           &next_xn_thrd_state );
#if defined(axp_osf) || defined(LP64)
    if (next_xn_thrd_state == IICX_XA_T_BAD_ASSOC)
			 xa_ret_value = XAER_RMERR;
#endif

    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid XA XN THRD state."));
	TRACED_RETURN( xa_ret_value );
    }

    IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, (DB_XA_DIS_TRAN_ID *)xid, rmid );

    if (xa_xn_cx_cb_p != NULL)
    {
        IICXlock_cb( xa_xn_cx_cb_p );

        xid_found = !IICXcompare_id( &xa_xn_cx_id,
                                     &(xa_xn_cx_cb_p->cx_id));
    }

    if (!xid_found)
    {
        db_status = IICXfind_lock_cb( &xa_xn_cx_id, &xa_xn_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            TRACED_RETURN( XAER_RMERR );
        
        xid_found = (xa_xn_cx_cb_p != NULL) ? TRUE : FALSE;

    }
        
    if (!xid_found)
    {
	TRACED_RETURN( XAER_NOTA );
    }

    xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

    /* 
    ** check the xn branch state 
    */
    xa_ret_value = check_xn_branch_state( xa_call,
                                          xa_xn_cb_p,
                                          &next_xn_branch_state );
    if ((xa_ret_value >= XA_RBBASE) && (xa_ret_value <= XA_RBEND))
    {
        char	ebuf[30];

        xa_rb_value = xa_ret_value;

	IICXunlock_cb( xa_xn_cx_cb_p );
	CVna((i4) xa_ret_value, ebuf);
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D2_XA_XN_ROLLBACKONLY, 1, ebuf);
    }
    else if (xa_ret_value != XA_OK)
    {
	IICXunlock_cb( xa_xn_cx_cb_p );
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid XA XN branch state."));
	TRACED_RETURN( xa_ret_value );
    }

    xa_ret_value = check_xn_branch_assoc_state( xa_call,
                                                xa_xn_cb_p,
                                                &next_xn_branch_assoc_state );
    if (xa_ret_value != XA_OK)
    {
	IICXunlock_cb( xa_xn_cx_cb_p );
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_end_call: invalid XA XN branch association state."));
	TRACED_RETURN( xa_ret_value );
    }

    /* set xn branch, xn thrd and fe thread states */

    xa_xn_cb_p->xa_state = next_xn_branch_state;
    xa_xn_cb_p->xa_assoc_state = next_xn_branch_assoc_state;

    IICXunlock_cb( xa_xn_cx_cb_p );

    IICXlock_cb( xa_xn_thrd_cx_cb_p );

    /* set the xa_xn_thrd_cx_cb's state */
    xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] += 1;

    if (xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] == 
	xa_xn_thrd_cb_p->num_reg_rmis)
    {
	/* 
	** all of the xa_end's for this xid have been called (one for each  
	** RMI) so now we can set the thread assoc state and reset the
	** counter.
	*/
        xa_xn_thrd_cb_p->assoc_state_by_rmi[next_xn_thrd_state] = 0;
	xa_xn_thrd_cb_p->xa_assoc_state = next_xn_thrd_state;

	/* 
	** This is to cover the case of:
	** XA_START (XID 1)
	** XA_END (XID 1, TMSUSPEND)
	** XA_START (XID 2)
	** XA_END (XID 1, TMSUCCESS)
	** Which, according to Transarc, is valid.  So if the XID we 
	** just ended (XID 1 in above example) is not the one pointed to by 
	** the fe_cb (XID 2 in above example) then don't disassociate the
	** fe_cb from its XID. 
	*/

        IICXlock_cb( fe_thread_cx_cb_p );
	if (fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p == 
	    xa_xn_thrd_cx_cb_p )
	{
            fe_thread_cb_p->ingres_state = IICX_XA_T_ING_IDLE;
            fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p = NULL;   
	}
        IICXunlock_cb( fe_thread_cx_cb_p );

	if((IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	   && (xa_xn_thrd_cb_p->num_reg_rmis > 1))
        {
            if (xa_call == IIXA_END_SUSP_CALL)
	    {
	        IIxa_fe_qry_trace_handler( (PTR) 0, 
	        ERx(
		"\tAll RMI's for this XID suspended.\n"),
		    TRUE);
	    }
	    else
	    {
	        IIxa_fe_qry_trace_handler( (PTR) 0, 
	        ERx(
		"\tAll RMI's for this XID ended.  THREAD state is now IDLE\n"),
		    TRUE);
	    }
        } /* if tracing on */
    }

    IICXunlock_cb( xa_xn_thrd_cx_cb_p );

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	IITUX_XN_ENTRY *xn_entry;

	xn_entry = IItux_find_xid((DB_XA_DIS_TRAN_ID *)xid);
	if (!xn_entry)
	{
	    IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
	    ERx("xa_end_call: cant find xid in shm"));
	    TRACED_RETURN( XAER_RMERR);
	}

	xn_entry->xn_state |= TUX_XN_END;

/* FIX ME
  If we want to do this... we need to switch to current session 
	EXEC SQL INQUIRE_SQL (:in_a_transaction = TRANSACTION);
*/
        xn_entry->xn_intrans = 1;   /* FIX ME */

	if ((xn_entry->xn_state & TUX_XN_ROLLBACK) &&
	    (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA))
	{
	    char outbuf[300];
	    STprintf(outbuf, ERx("\tXA_END after XA_ROLLBACK\n"));
	    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}
    }

    if ((xa_rb_value) && (xa_ret_value == XA_OK))
    {
       TRACED_RETURN( xa_rb_value );
    }
    else
    {
       TRACED_RETURN( xa_ret_value );
    }

} /* IIxa_end */






/*{
+*  Name: IIxa_rollback   -  Propagate rollback of a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the
**                   AS process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS 
**                             server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_HEURHAZ     - Work done in global xn branch may have been 
**                           heuristically completed.
**          XA_HEURCOM     - Work done may have been heuristically committed.
**          XA_HEURRB      - Work done was heuristically rolled back.
**          XA_HEURMIX     - Work done was partially committed and partially
**                           rolled back heuristically.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**      23-Sep-1993 (iyer)
**            The function should now pass two additional parameters to
**            do_2phase_operation. It should be disabled for all TUXEDO
**            AS processes. It should only be called from TUXEDO TMS.
**	01-nov-1993 (larrym)
**	     added tuxedo support.
**	11-nov-1993 (larrym)
**	    integrated iyer's changes into this branch.
**	10-jan-1994 (larrym)
**	    Temporarily added code to set branch flag to all flags so 
**	    recovery transactions in encina will work.  For now, when
**	    reconnecting to a transaction, you need the XID, the seqnum
**	    AND the flags it was prepared with.  We should change the
**	    server to ignore the flags.  For now it won't hurt to always
**	    set them this way since we only look at them when it's a 
**	    recovery situation.
*/

int
IIxa_rollback(XID       *xa_xid,
             int       rmid,
             long      flags)
{
    int			xa_ret_value;
    i4  		branch_seqnum;
    i4  		branch_flag;
    DB_XA_DIS_TRAN_ID   db_xid; 
    DB_XA_DIS_TRAN_ID   *xid;

#if defined(LP64)
    if( IImap_xa_xid( IIXA_ROLLBACK_CALL, (XID *)xa_xid,
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
       return( XAER_INVAL ); 
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                         	IIXA_ROLLBACK_CALL, 
				IITUX_ACTION_ONE, 
				(PTR) xid, (PTR) &rmid, (PTR) &flags);
    }
    else
    {
	branch_seqnum = 0;
	branch_flag  = (DB_XA_EXTD_BRANCH_FLAG_FIRST |
			DB_XA_EXTD_BRANCH_FLAG_LAST  |
			DB_XA_EXTD_BRANCH_FLAG_2PC );
        xa_ret_value = IIxa_do_2phase_operation( IIXA_ROLLBACK_CALL, xid, 
		    rmid, flags, branch_seqnum, branch_flag );
    }

    return( xa_ret_value );

} /* IIxa_rollback */





/*{
+*  Name: IIxa_prepare   -  Prepare a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_RDONLY      - the Xn branch was read-only, and has been
**                           committed.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**      23-Sep-1993 (iyer)
**            The function should now pass two additional parameters to
**            do_2phase_operation. It should be disabled for all TUXEDO
**            AS processes. It should only be called from TUXEDO TMS.
**	01-nov-1993 (larrym)
**	     added tuxedo support.
**      11-nov-1993 (larrym)
**          integrated iyer's changes into this branch.
**	10-jan-1994 (larrym)
**	    added code to set branch_flags to all flags, which is what
**	    it should be for Encina/CICS environments.
*/

int
IIxa_prepare(XID       *xa_xid,
             int       rmid,
             long      flags)
{
    int			xa_ret_value;
    i4  		branch_seqnum;
    i4  		branch_flag;
    DB_XA_DIS_TRAN_ID   db_xid; 
    DB_XA_DIS_TRAN_ID   *xid; 

#if defined(LP64)
    if( IImap_xa_xid( IIXA_PREPARE_CALL, (XID *)xa_xid,
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
       return( XAER_INVAL );
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                         	IIXA_PREPARE_CALL, 
				IITUX_ACTION_ONE, 
				(PTR) xid, (PTR) &rmid, (PTR) &flags);
    }
    else
    {
        branch_seqnum = 0;
	branch_flag  = (DB_XA_EXTD_BRANCH_FLAG_FIRST |
			DB_XA_EXTD_BRANCH_FLAG_LAST  |
			DB_XA_EXTD_BRANCH_FLAG_2PC );
        xa_ret_value = IIxa_do_2phase_operation( IIXA_PREPARE_CALL, xid,
                    rmid, flags, branch_seqnum, branch_flag );
    }

    return( xa_ret_value );

} /* IIxa_prepare */




/*{
+*  Name: IIxa_commit   -  Commit a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMNOWAIT/TMASYNC/TMONEPHASE/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RETRY       - if TMNOWAIT is set in flags.
**          XA_RB*         - if this XID is marked "rollback only". These are
**                           allowed *only* if this is a 1-phase Xn.
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_HEURHAZ     - Work done in global xn branch may have been 
**                           heuristically completed.
**          XA_HEURCOM     - Work done may have been heuristically committed.
**          XA_HEURRB      - Work done was heuristically rolled back.
**          XA_HEURMIX     - Work done was partially committed and partially
**                           rolled back heuristically.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**      23-Sep-1993 (iyer)
**            The function should now pass two additional parameters to
**            do_2phase_operation. It should be disabled for all TUXEDO
**            AS processes. It should only be called from TUXEDO TMS.
**	01-nov-1993 (larrym)
**	     added tuxedo support.
**      11-nov-1993 (larrym)
**          integrated iyer's changes into this branch
**	10-jan-1994 (larrym)
**	    Temporarily added code to set branch flag to all flags so 
**	    recovery transactions in encina will work.  For now, when
**	    reconnecting to a transaction, you need the XID, the seqnum
**	    AND the flags it was prepared with.  We should change the
**	    server to ignore the flags.  For now it won't hurt to always
**	    set them this way since we only look at them when it's a 
**	    recovery situation.
*/

int
IIxa_commit(XID       *xa_xid,
            int       rmid,
            long      flags)
{
    int			xa_ret_value;
    i4  		branch_seqnum;
    i4  		branch_flag;
    DB_XA_DIS_TRAN_ID  db_xid;
    DB_XA_DIS_TRAN_ID  *xid;

#if defined(LP64)
    if( IImap_xa_xid( IIXA_COMMIT_CALL, (XID *)xa_xid,
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
       return( XAER_INVAL );
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                         	IIXA_COMMIT_CALL, 
				IITUX_ACTION_ONE, 
				(PTR) xid, (PTR) &rmid, (PTR) &flags);
    }
    else
    {
        branch_seqnum = 0;
	branch_flag  = (DB_XA_EXTD_BRANCH_FLAG_FIRST |
			DB_XA_EXTD_BRANCH_FLAG_LAST  |
			DB_XA_EXTD_BRANCH_FLAG_2PC );
        xa_ret_value = IIxa_do_2phase_operation( IIXA_COMMIT_CALL, xid,
                    rmid, flags, branch_seqnum, branch_flag );
    }

    return( xa_ret_value );

} /* IIxa_commit */





/*{
+*  Name: IIxa_recover   -  Recover a set of prepared XIDs for a specific RMI.
**
**  Description: 
**
**  Inputs:
**      xids       - pointer to XID list.
**      count      - maximum number of XIDs expected by the TP system.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMSTARTRSCAN/TMENDRSCAN/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          >= 0           - number of XIDs returned.
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	06-Oct-1993 (larrym)
**	    fixed bug in tracing code.  Last call to IIxa_fe_qry_trace_handler 
**	    wasn't checking to see if tracing was on first so it would
**	    core dump if tracing wasn't on.
*/

int
IIxa_recover(XID       *xids,
             long      count,
             int       rmid,
             long      flags)
{
    IICX_CB            *rmi_cx_cb_p;
    int                xa_ret_value;
    IICX_CB            *fe_thread_cx_cb_p;
    IICX_FE_THREAD_CB  *fe_thread_cb_p;
    i4                 rmi_xid_count = 0;
    i4                 next_thread_state;
    char               *dbname_p;

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
            ERx("xa_recover_call: LIBQXA not initialized properly earlier."));
	return( XAER_PROTO );
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO 
		&& !IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_recover_call: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        char outbuf[80];

	/* output trace header info if tracing on...*/
	OUTPUT_TRACE_HEADER( IIXA_RECOVER_CALL, flags,
		             (DB_XA_DIS_TRAN_ID *)0, (XID *)0, &rmid);
	STprintf(outbuf, ERx("\txid_count = %d\n"), count);
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }

    if ((xa_ret_value = check_xa_rmi_state(IIXA_RECOVER_CALL,
                                            rmid,&rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_recover_call: invalid RMI state: XAER_PROTO."));
        TRACED_RETURN( xa_ret_value );
    }

    if (((xids == NULL) && (count > 0)) || (count < 0))
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_recover_call: invalid arguments."));
        TRACED_RETURN( XAER_INVAL );
    }

    switch( flags )
    {
       case TMNOFLAGS:
       case TMSTARTRSCAN:
       case TMENDRSCAN:
       case (TMSTARTRSCAN | TMENDRSCAN):
              break;
       default:
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                  ERx("xa_recover_call: invalid flags"));
            TRACED_RETURN( XAER_INVAL );
    }

    /* Now check the thread state, and make sure that a recovery scan has */
    /* already been marked 'open' in this thread, if the incoming flags   */
    /* argument is NOT a TMSTARTRSCAN  */

    /* check the thread state first */    
    xa_ret_value = check_fe_thread_state( IIXA_RECOVER_CALL,
                         NULL,
                         &fe_thread_cx_cb_p,
                         &fe_thread_cb_p );
    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_recover_call: invalid FE THREAD state."));
	TRACED_RETURN( xa_ret_value );
    }

    if ((!(flags & TMSTARTRSCAN)) &&
        (!fe_thread_cb_p->rec_scan_in_progress))
    {
       IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
          ERx("xa_recover_call: invalid flags: no scan in progress."));
       TRACED_RETURN( XAER_INVAL );
    }

    /* Now handle the de-generate case of an input 'count' arg of zero */
    if (count == 0)
    {
        fe_thread_cb_p->rec_scan_in_progress = FALSE;
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	{
	   IIxa_fe_qry_trace_handler( (PTR) 0, 
	   ERx("\tWARNING:  xid_count is 0.  Returning (nothing to do)\n"), 
	       TRUE); 
	}
	/* don't do a TRACED_RETURN as we're returning the xid_count */
        return(0);
    }

    /*
    ** If this is TUXEDO, then make sure the lgmo table is in a
    ** consistant state with TUXEDO's idea of things (see 
    ** front!embed!libqtxxa iitxtms.c for discussion) before we
    ** begin the recovery scan.
    */
 
    if ( (flags & TMSTARTRSCAN) && (!fe_thread_cb_p->rec_scan_in_progress)
       && ( IIXA_CHECK_IF_TUXEDO_MACRO ) )
    {
	/*
	** This connects to the iidbdb
	*/
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
			    IIXA_RECOVER_CALL,
			    IITUX_ACTION_ONE,
			    (PTR) &rmid,
			    (PTR) rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p);

	if( xa_ret_value != XA_OK)
	{
	    return( xa_ret_value );
	}
    }
    /* connect to the appropriate 'iidbdb'. If already connected, switch  */
    /* to the appropriate session                                         */
    

    xa_ret_value = IIxa_connect_to_iidbdb( 
                                    fe_thread_cb_p,
                                    rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p, 
                                    &dbname_p );
    if (xa_ret_value != XA_OK)
        TRACED_RETURN( xa_ret_value);

    /* close/open cursors as needed based on the input 'flags' argument.  */
    xa_ret_value = setup_recovery_cursor( fe_thread_cb_p,
					  dbname_p,
                                    	  flags );
    if (xa_ret_value != XA_OK)
        TRACED_RETURN( xa_ret_value);

    /* now do the complete/partial scan for XIDs to be returned */
    rmi_xid_count = process_recovery_scan( fe_thread_cb_p, 
                                           rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p,
                                           xids, 
                                           count,
                                           flags );
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
	char	outbuf[80];

	if (rmi_xid_count < count)
	{
	    STprintf(outbuf, 
	        ERx("\tfound %d XID's to recover.  Returning.\n"), 
	        rmi_xid_count);
	}
	else
	{
	    STprintf(outbuf, 
	        ERx("\tfound %d XID's to recover (so far).  Returning.\n"), 
	        rmi_xid_count);
	}
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }
    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        /* can't do TRACED_RETURN, because we're not returning a status here */
        IIxa_fe_qry_trace_handler( (PTR) 0, IIXATRCENDLN, TRUE);
    }

    return( rmi_xid_count );

} /* IIxa_recover */




/*{
+*  Name: IIxa_forget   -  Forget a heuristically completed xn branch.
**
**  Description: 
**        UNSUPPORTED IN Ingres65 - Hence unexpected !
**
**  Inputs:
**      xa_xid     - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

int
IIxa_forget(XID       *xa_xid,
            int       rmid,
            long      flags)
{
  DB_XA_DIS_TRAN_ID  db_xid;
  DB_XA_DIS_TRAN_ID  *xid;

#if defined(LP64)
    if( IImap_xa_xid( IIXA_FORGET_CALL, (XID *)xa_xid, 
                      (DB_XA_DIS_TRAN_ID *)&db_xid ) != XA_OK )
       return( XAER_INVAL );
    xid = &db_xid ;
#else
    xid = (DB_XA_DIS_TRAN_ID *) xa_xid ;
#endif  /* defined(LP64) */

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_forget_call: LIBQXA not initialized properly earlier."));
	return( XAER_PROTO );
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO 
		&& !IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_forget_call: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        /* output trace header info if tracing on...*/
        OUTPUT_TRACE_HEADER( IIXA_FORGET_CALL, flags, xid, xa_xid, &rmid);
	IIxa_fe_qry_trace_handler( (PTR) 0,
	    ERx("\tWARNING:  This call is not supported (yet)\n"), TRUE);
    }

    IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_forget_call: UNEXPECTED: XAER_PROTO."));

    TRACED_RETURN( XAER_PROTO );

} /* IIxa_forget */




/*{
+*  Name: IIxa_complete   -  Wait for an asynchronous operation to complete.
**
**  Description: 
**        UNSUPPORTED IN Ingres65 - Hence unexpected !
**
**  Inputs:
**      handle     - handle returned in a previous XA call.
**      retval     - pointer to the return value of the previously activated
**                   XA call.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMMULTIPLE/TMNOWAIT/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_RETRY       - TMNOWAIT was set in flags, no asynch ops completed.
**          XA_OK          - normal execution.
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended xns bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

int
IIxa_complete(int     *handle,
              int     *retval,
              int       rmid,
              long      flags)
{

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_complete_call: LIBQXA not initialized properly earlier."));
	return( XAER_PROTO );
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO 
		&& !IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_complete_call: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        /* output trace header info if tracing on...*/
        OUTPUT_TRACE_HEADER( IIXA_COMPLETE_CALL, flags,
		             (DB_XA_DIS_TRAN_ID *)0, (XID *)0, &rmid);
	IIxa_fe_qry_trace_handler( (PTR) 0,
	    ERx("\tWARNING:  This call is not supported (yet)\n"), TRUE);
    }

    IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_complete_call: UNEXPECTED: XAER_PROTO."));

    TRACED_RETURN( XAER_PROTO );

} /* IIxa_complete */




static  int
check_xa_rmi_state( i4                 xa_call,
                    int                rmid,
                    IICX_CB            **rmi_cx_cb_p_p
                  )
{
    IICX_ID            rmi_cx_id;
    IICX_XA_RMI_CB     *xa_rmi_cb_p;
    DB_STATUS          db_status;
    int                xa_ret_value;

    IICXsetup_xa_rmi_id_MACRO( rmi_cx_id, rmid );
    db_status = IICXfind_lock_cb( &rmi_cx_id, rmi_cx_cb_p_p );

    if (DB_FAILURE_MACRO(db_status))
        return( XAER_RMERR );
   
    if (*rmi_cx_cb_p_p == NULL)
    {
	switch(xa_call)
	{
	  case IIXA_OPEN_CALL:
	  case IIXA_CLOSE_CALL:
	    return( XA_OK );
	  default:
	    return( XAER_RMERR );
	}
    }

    xa_rmi_cb_p = (*rmi_cx_cb_p_p)->cx_sub_cb.xa_rmi_cb_p;

    if (xa_rmi_cb_p->xa_state > IICX_XA_R_OPEN)
    {
       IICXunlock_cb( *rmi_cx_cb_p_p );
       return( XAER_RMERR );
    }

    xa_ret_value = (int) IIxa_rmi_return_value[ xa_call ]
					      [ xa_rmi_cb_p->xa_state ];

    IICXunlock_cb( *rmi_cx_cb_p_p );

    return( xa_ret_value );

} /* check_xa_rmi_state */



static  int
check_xa_xn_thrd_state( i4                 *xa_call,
			DB_XA_DIS_TRAN_ID  *xa_xid_p,
			IICX_CB            **xa_xn_thrd_cx_cb_p_p,
			IICX_XA_XN_THRD_CB **xa_xn_thrd_cb_p_p,
			IICX_CB            **xa_xn_cx_cb_p_p,
			i4                *next_state
		      )
{
   int        state_index;
   IICX_ID    cx_id;
   DB_STATUS  db_status;

   *xa_xn_cx_cb_p_p = NULL;	/* initialize in case of error */

   /* Part I: find the xa_xn_thrd_cb_p_p for this XID */

   IICXsetup_xa_xn_thrd_id_MACRO( cx_id, xa_xid_p );
   if (*xa_call == IIXA_START_CALL)
   {
       /*
       ** create_lock does a find_lock first and just returns it if found.
       */

       db_status = IICXcreate_lock_cb( &cx_id, NULL, xa_xn_thrd_cx_cb_p_p );

    }
    else
    {
        /* this has to find the xa_xn_thrd for this xid */
	db_status = IICXfind_lock_cb( &cx_id, xa_xn_thrd_cx_cb_p_p );
    }

    if (DB_FAILURE_MACRO( db_status ))
    {
	return( XAER_RMERR );
    }

    if (*xa_xn_thrd_cx_cb_p_p == NULL)
    {
	/* 
	** should only happen if find_lock failed.
	** If this is a tuxedo application, then this may be because 
	** this is the first involvement of this AS in a transaction
	** already in progress.  If so, then we'll just pretend it's
	** an IIXA_START_CALL, instead of an IIXA_START_JOIN_CALL
	*/
        if (( IIXA_CHECK_IF_TUXEDO_MACRO )
	  &&( *xa_call == IIXA_START_JOIN_CALL ) )
	{
            db_status = IICXcreate_lock_cb( &cx_id, NULL, 
					    xa_xn_thrd_cx_cb_p_p );
	    if (*xa_xn_thrd_cx_cb_p_p == NULL)
	    {
		/* if it's *still* NULL, then we have a problem */
	        return( XAER_RMERR );
	    }
	    /* else, all is cool */
	    *xa_call = IIXA_START_CALL;
	}
	else
	{
	    /* we don't know about this transaction */
            return (XAER_NOTA );
	}
    }

    *xa_xn_thrd_cb_p_p = (*xa_xn_thrd_cx_cb_p_p)->cx_sub_cb.xa_xn_thrd_cb_p;

    /* Part II: check the transaction-association-to-a-thread state */

    state_index = (*xa_xn_thrd_cb_p_p)->xa_assoc_state - 1;

    if (state_index < 0)
    {
        return( XAER_RMERR );
    }

    *next_state = IIxa_xn_thrd_next_state[*xa_call][state_index];
   
#if defined(axp_osf) || defined(LP64)
    if (*next_state == IICX_XA_T_BAD_ASSOC)
    {
        return( XAER_RMERR );
    }
#endif

    /* Part III: return the xa_xn_cx_cb_p if available (might be null) */

    *xa_xn_cx_cb_p_p = (*xa_xn_thrd_cb_p_p)->curr_xa_xn_cx_cb_p; 

    return( XA_OK );

} /* check_xa_xn_thrd_state */



static  int
check_fe_thread_state( i4                 xa_call,
                       DB_XA_DIS_TRAN_ID  *xa_xid_p,
                       IICX_CB            **fe_thread_cx_cb_p_p,
                       IICX_FE_THREAD_CB  **fe_thread_cb_p_p
                     )
{
   i4        thread_ingres_state;
   DB_STATUS db_status;
   int       xa_status = XA_OK;

   db_status = IICXget_lock_fe_thread_cb( NULL, fe_thread_cx_cb_p_p );
   if (DB_FAILURE_MACRO( db_status ))
   {
       return( XAER_RMERR );
   }

   *fe_thread_cb_p_p =
        (*fe_thread_cx_cb_p_p)->cx_sub_cb.fe_thread_cb_p;

   thread_ingres_state =  (*fe_thread_cb_p_p)->ingres_state;

   if (thread_ingres_state == IICX_XA_T_ING_OUTSIDE)
   {
	/* we don't generate an error here, the caller does upon return. */

        thread_ingres_state = IICX_XA_T_ING_IDLE;
 
        switch( xa_call )
        {
          case  IIXA_START_CALL:
          case  IIXA_START_RES_CALL:
                        xa_status = XAER_OUTSIDE;
                        break;
          case  IIXA_ROLLBACK_CALL:
			xa_status = XA_OK;
                        break;
          default:
                        xa_status = XAER_PROTO;
                        break;
        }
   }
   else if (thread_ingres_state == IICX_XA_T_ING_APPL_ERROR)
   {
        thread_ingres_state = IICX_XA_T_ING_IDLE;

        switch( xa_call )
        {
          case IIXA_END_FAIL_CALL:
          case IIXA_ROLLBACK_CALL:
	    /* TP system already knows there's a problem.  Let it continue */
            xa_status = XA_OK;
	    break;
	  default:
	    /* we don't generate an error here, the caller does upon return. */
            xa_status = XAER_PROTO;
	    break;
        }
   }

   (*fe_thread_cb_p_p)->ingres_state = thread_ingres_state;
   IICXunlock_cb( *fe_thread_cx_cb_p_p );
   if (xa_status != XA_OK)
   {
       return( xa_status );
   }

    /* 
    ** Now check thread association state:
    ** Here's the strategy - all of this is to handle the multiple RMI
    ** case:  When the first XA_START comes in, we create an xa_xn_thrd
    ** cb (one per XID) and an xa_xn cb (one per XID,RMID pair, so the 
    ** the relation ship between xa_xn_thrd and xa_xn is 1:n).  We set
    ** the FE thread cb to point to the current xa_xn_thrd cb to indicate
    ** this thread is associated (or will be associated) with this transaction.
    ** An XA_START or XA_END for a different XID will fail at this point 
    ** because we are obviously busy with the first XID.  However, until
    ** all the XA_START's for all of the RMI's come in, we are not fully
    ** associated with that transaction; i.e., we are not in the right
    ** state for any XA_ENDs on that XID.  So we wait till all the XA_STARTS
    ** for that XID come in, then we can change the state of the xa_xn_thrd
    ** cb's so that XA_END can succeed.
    ** if fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p is not null, then we're
    ** assoc'd to an XID.
    */
    if (xa_xid_p != NULL)
    {
	IICX_CB		*xa_xn_thrd_cx_cb_p;
	IICX_ID		xa_xn_thrd_cx_id;

        xa_xn_thrd_cx_cb_p = (*fe_thread_cb_p_p)->curr_xa_xn_thrd_cx_cb_p;

        if (xa_xn_thrd_cx_cb_p != NULL)
        {
	    /* 
	    ** we're currently associated with an XID.  Make sure the one
	    ** just passed in matches it.
	    */
            IICXsetup_xa_xn_thrd_id_MACRO( xa_xn_thrd_cx_id, xa_xid_p );

	    if ( IICXcompare_id( &xa_xn_thrd_cx_id, 
				 &xa_xn_thrd_cx_cb_p->cx_id ))
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                  ERx(
		   "This thread is current associated with another transaction."
		  ));
                xa_status = XAER_PROTO;
	    }
	    else
	    {
	        /* otherwise, it's cool. */
                xa_status = XA_OK;
	    }
	}
    }

   return( xa_status );

} /* check_fe_thread_state */





static  int
check_xn_branch_state( i4              xa_call,
                       IICX_XA_XN_CB   *xa_xn_cb_p, 
                       i4              *next_state
                     )
{
   i4   state_index;
   int  xa_ret_value = XA_OK;

   if (xa_xn_cb_p == NULL)
   {
       return( XAER_RMERR );
   }

   state_index = xa_xn_cb_p->xa_state - 1;

   if (state_index < 0)
       switch (xa_call)
       {
	   case IIXA_END_FAIL_CALL:
	   case IIXA_ROLLBACK_CALL:
	       *next_state = IICX_XA_T_NO_ASSOC;
	       return (XA_OK);
	   default:
	       *next_state = IICX_XA_T_BAD_ASSOC; /* in case it was garbage */
               return( XAER_RMERR );
       }

   if (xa_xn_cb_p->xa_rb_value != 0)
   {
       /* use rollback_only state table */
       *next_state = IIxa_rb_xn_branch_next_state[xa_call][state_index];
   }
   else
   {
       /* use normal state table */
       *next_state = IIxa_xn_branch_next_state[xa_call][state_index];
   }

    /* now handle possible xa_end(SUSPEND) + xa_end(SUCCESS) case */
    /* CHECK:  Do we need this or will the state tables take care of it? */
    if (!*next_state)
    {
	xa_ret_value = XAER_PROTO; /* most likely */
	switch( xa_call )
	{
            case IIXA_END_CALL:
            case IIXA_END_FAIL_CALL:
		/* if previously we were suspended... */
                if (xa_xn_cb_p->xa_assoc_state == IICX_XA_T_ASSOC_SUSP)
                {
                     *next_state = IICX_XA_X_IDLE;
	             xa_ret_value = XA_OK;
                }
                break;
	}
    }

    if (xa_xn_cb_p->xa_rb_value != 0)
    {
	/* rollback value takes precidence */
        xa_ret_value = xa_xn_cb_p->xa_rb_value;
    }

    return( xa_ret_value );

}

static  int
check_xn_branch_assoc_state( i4              xa_call,
                             IICX_XA_XN_CB   *xa_xn_cb_p, 
                             i4              *next_assoc_state
                           )
{
    i4   state_index;
    int  xa_ret_value = XA_OK;

    if (xa_xn_cb_p == NULL)
    {
        return( XAER_RMERR );
    }

    state_index = xa_xn_cb_p->xa_assoc_state - 1;

    if (state_index < 0)
    {
        return( XAER_RMERR );
    }

    *next_assoc_state = IIxa_xn_thrd_next_state[xa_call][state_index];

    if (*next_assoc_state == IICX_XA_X_BAD_ASSOC)
    {
        return( XAER_RMERR );
    }
    else
    {
        return( XA_OK );
    }

}

/*{
**  Name: lock_parse_xa_open_string
**
**  Description: 
**     parse an XA open string, and store it's contents/flags in an RMI CB. 
**     the RMI CB is freshly allocated. It is then placed in the processwide 
**     linked list and locked. 
**
**  Inputs:
**      rmid            - RMI identifier, assigned by the TP system, unique 
**			  to the
**      rmi_cx_cb_p_p	- pointer to an IICX_CB control block, see 
**			  front!embed!hdr iicx.h for description
**	xa_info		- The open string, as it's passed to xa_open
**	xa_info_len	- The length of the open string
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_RMERR     - various internal errors.
**          XAER_INVAL     - illegal input arguments.
**
**  Side Effects:
**	    Calls lock_parse_tux_xa_open_string if it encounters a TUXEDO
**	    style open string
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/
static  int
lock_parse_xa_open_string( i4               rmid,
                           IICX_CB          **rmi_cx_cb_p_p,
                           char             *xa_info,
                           i4               xa_info_len
                         )
{
    char             *str;
    DB_STATUS        db_status;
    STATUS	     cl_status;
    IICX_U_SUB_CB    xa_rmi_u_sub_cb;
    IICX_XA_RMI_CB   *xa_rmi_cb_p;
    IICX_ID          rmi_cx_id;
    i4               next_token = 0;
    int		     xa_ret_value;

    db_status =  IICXcreate_xa_rmi_sub_cb( &xa_rmi_cb_p );
    if (DB_FAILURE_MACRO( db_status ))
        return( XAER_RMERR );  

    STcopy( xa_info, xa_rmi_cb_p->open_string );   /* master copy of open str */
    STcopy( xa_info, xa_rmi_cb_p->usr_arg_buff );  /* will contain tokens     */
  
    str = (char *)STskipblank(xa_rmi_cb_p->usr_arg_buff, xa_info_len);

    /* This call tokenizes the open string. Count is properly set */
    STgetwords( str, &(xa_rmi_cb_p->usr_arg_count),
                                (char **)&(xa_rmi_cb_p->usr_argv) );

    if ( STbcompare( (xa_rmi_cb_p->usr_argv[0]), STlength( iixa_switch.name ), 
                      iixa_switch.name, STlength( iixa_switch.name ), 
                      TRUE ) != 0 )
    {
        /* Open String doesn't start with keyword INGRES, may be from 
        ** TUXEDO or may be invalid. Parse locally 'cause we don't have
        ** the tuxedo top-level switched on at this point. 
        */
        xa_ret_value = lock_parse_tux_xa_open_string( xa_rmi_cb_p, rmid );

        if ( xa_ret_value != XA_OK )
            return( xa_ret_value );
    }
    else
    {
        /* The open string is  supposed to be in the format:
        **  INGRES dbname [AS connect_name] 
        **         [options = "-Rroleid/passwd" "..." ]  
        */

        /* parse that puppy */
        /* weed out illegal argc's. */
        /* minimum number of args is 2 (INGRES dbname), and 3 is illegal */

        switch ( xa_rmi_cb_p->usr_arg_count )
        {
	case 0:
	case 1:
	case 3:
            return( XAER_INVAL );
        }

        /* we think we have a valid number of args now.  */

        /* second token must be the database name. */
        /* CHECK should do an isalpha here */ 
        if ( *(xa_rmi_cb_p->usr_argv[1]) == '\0' )  
        {
            return( XAER_INVAL );
        }
	xa_rmi_cb_p->dbname_p = xa_rmi_cb_p->usr_argv[1];

        /* see if we have a connection name */
        if ( (xa_rmi_cb_p->usr_arg_count > 3) && 
             !(STbcompare(xa_rmi_cb_p->usr_argv[2],0, ERx("as"),0,TRUE)) )
        {
	    /* we should have a connection name */
	    xa_rmi_cb_p->connect_name_p = xa_rmi_cb_p->usr_argv[3];
            next_token = 4; /* where the options (if any) will start */
        }
        else
        {
	    /* no connection name; maybe options though */ 
	    xa_rmi_cb_p->connect_name_p = xa_rmi_cb_p->usr_argv[1];
            next_token = 2;
        }

        /* check for OPTIONS now */
        if(  next_token < xa_rmi_cb_p->usr_arg_count)
        {
            /* looking for "options=" or "options" "=" */
            if (STbcompare(xa_rmi_cb_p->usr_argv[next_token], 0,
                           ERx("options="), 0, TRUE ) == 0)
	    {
	         xa_rmi_cb_p->flags_start_index = next_token + 1;
	    }
	    else if ((STbcompare(xa_rmi_cb_p->usr_argv[next_token], 0,
                                 ERx("options"), 0, TRUE ) == 0)
                  &&  (STcompare(xa_rmi_cb_p->usr_argv[next_token+1], 
                                 ERx("=")) == 0)
	          &&  (xa_rmi_cb_p->usr_arg_count > next_token + 2))
	    {
                xa_rmi_cb_p->flags_start_index = next_token + 2;
	    }
	    else 
	    {
	        return( XAER_INVAL );
	    }
        }        
    }   /* if user_argv[0] == INGRES */

    db_status = IICXcheck_if_unique_rmi( xa_rmi_cb_p->dbname_p,
                                         xa_rmi_cb_p->connect_name_p );
    if (DB_FAILURE_MACRO( db_status ))
        return( XAER_INVAL );

    /* Now create the actual XA RMI CB and link it into the process-wide */
    /* linked-list                                                       */
    xa_rmi_u_sub_cb.xa_rmi_cb_p = xa_rmi_cb_p;

    IICXsetup_xa_rmi_id_MACRO( rmi_cx_id, rmid ); 
    db_status = IICXcreate_lock_cb( &rmi_cx_id,
                                    &xa_rmi_u_sub_cb,
                                    rmi_cx_cb_p_p );
    if (DB_FAILURE_MACRO( db_status ))
        return( db_status );

    return( XA_OK );

} /* lock_parse_xa_open_string */


static  int
connect_to_dbms( i4                xa_call,
                 IICX_FE_THREAD_CB *fe_thread_cb_p,
		 IICX_XA_RMI_CB    *xa_rmi_cb_p,
                 IICX_XA_XN_CB     *xa_xn_cb_p,
                 DB_XA_DIS_TRAN_ID *xa_xid_p,
		 i4                branch_seqnum,
	         i4                branch_flag,
		 char              *server_in
               )
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4    internal_session = IIsqconSysGen; /* generate session id's */
	char   *dbname_p;
	char   *flags[12];
	char   err_msg[256];
	i4    err_number;
        char   xid_str[IIXA_XID_STRING_LENGTH];
	char   tmp_server_name[DB_MAXNAME * 2];
    EXEC SQL END DECLARE SECTION;
    i4         src_i;
    i4	       dest_i;
    i4	       old_ingres_state;
    i4	       xa_error_status;
    char       dbname_buf[100];
    char       outbuf[200];

    dbname_p = xa_rmi_cb_p->dbname_p;
    MEfill((u_i2)sizeof(outbuf), 0, outbuf);

    /* if there are flags in this open string, set them up for the connect */

    dest_i = 0;

    if (server_in)
    {
	PTR	u;
	char	t[DB_MAXNAME+1];

	u = STrchr(dbname_p, '/');
	if (u != NULL)
	    STlcopy(dbname_p, t, (i4)(u - dbname_p));
	else
	    STcopy(dbname_p, t);
	/* 
	** Use the server class method '/@<gca_id>' for direct connection
	** to a specific server.
	** This works properly with the Name server and the Comm server.
	*/
	dbname_p = dbname_buf;

	STprintf(dbname_buf, "%s/@%s", t, server_in);
    }
    STprintf(outbuf, "Connecting to database %s \n", dbname_p);

    if (src_i = xa_rmi_cb_p -> flags_start_index)
    {
       for ( ;
             ((src_i < xa_rmi_cb_p->usr_arg_count) && (dest_i < 12));
             dest_i++, src_i++)
       {
            flags[dest_i] = xa_rmi_cb_p->usr_argv[src_i];
       }
    }

    for (; dest_i < 12; dest_i++)
        flags[dest_i] = (char *)0;
    
    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;
	        
    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);

    if (xa_call == IIXA_START_CALL)
    {
      /* new transaction */
      xa_error_status = XAER_RMERR;

      EXEC SQL CONNECT :dbname_p SESSION :internal_session OPTIONS = 
         :flags[0], :flags[1], :flags[2], :flags[3], :flags[4], :flags[5],
         :flags[6], :flags[7], :flags[8], :flags[9], :flags[10], :flags[11]; 
    }
    else
    {

      /* recovery trans - if connect fails, we don't know about this XID */
      xa_error_status = XAER_NOTA;

      IICXformat_xa_xid( xa_xid_p, xid_str );

/*    Format branch_seqnum and branch_flag to end of xid_str */

      IICXformat_xa_extd_xid( branch_seqnum, branch_flag, xid_str ); 
      
      EXEC SQL CONNECT :dbname_p SESSION :internal_session OPTIONS = 
         :flags[0], :flags[1], :flags[2], :flags[3], :flags[4], :flags[5],
         :flags[6], :flags[7], :flags[8], :flags[9], :flags[10], :flags[11]
         WITH xa_xid = :xid_str;
    }

    EXEC SQL INQUIRE_SQL(:err_number = ERRORNO);

    if (err_number)
    {
        fe_thread_cb_p->ingres_state = old_ingres_state;

        /* XA XN is reset to non-existent state */
        xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;

        return( xa_error_status );
    }
    else
    {
       EXEC SQL INQUIRE_SQL(:internal_session = SESSION);
       if (internal_session == 0)
       {
	   /* this should never ever happen */
           fe_thread_cb_p->ingres_state = old_ingres_state;

           /* XA XN is reset to non-existent state */
           xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;

           return( XAER_RMERR );
	}
	else
	{
            xa_xn_cb_p->xa_sid = internal_session;
	}
    }


    fe_thread_cb_p->ingres_state = old_ingres_state;

    /* store the connection name in the XA XN CB. */
    STcopy(xa_rmi_cb_p->connect_name_p, xa_xn_cb_p->connect_name);

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	char       *nodename_p;
	char       *listen_address = NULL;
	char       *end;
	int        i;
	i4        save_ingres_state;

	save_ingres_state = fe_thread_cb_p->ingres_state; 
	fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

	EXEC SQL SELECT dbmsinfo('ima_server') INTO :tmp_server_name;
	if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	    IIxa_fe_qry_trace_handler( (PTR) 0, ERx("select dbmsinfo ...\n"),
			    TRUE);

	EXEC SQL INQUIRE_SQL(:err_number = ERRORNO);

	/*
	** Need to commit - since select ima_server started transaction
	** but to start XA transaction correctly (with shared logging context)
	** we need SET SESSION .. WITH XID= to be the first statement
	*/
	if (!err_number)
	{
	    EXEC SQL COMMIT; 
	    EXEC SQL INQUIRE_SQL(:err_number = ERRORNO);
	}

	fe_thread_cb_p->ingres_state = save_ingres_state;

	if (err_number)
	{
	    fe_thread_cb_p->ingres_state = old_ingres_state;

	    /* XA XN is reset to non-existent state */
	    xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;

	    return( xa_error_status );
	}

	/*
	** server_name returned as my_vnode::/@listen_address
	** See gwf/gwm/gwmutil.c
	** Isolate listen address
	*/

	tmp_server_name[DB_MAXNAME] = '\0';
	if ( ((nodename_p = STindex( tmp_server_name, ERx("::"), 0 )) == NULL)
	    || ((listen_address = STindex (nodename_p, ERx("@"), 0)) == NULL))
	{
	    fe_thread_cb_p->ingres_state = old_ingres_state;

	    /* XA XN is reset to non-existent state */
	    xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;

	    return( xa_error_status );
	}

	listen_address += 1;
	if ( (end = STindex (listen_address, ERx(" "), 0)) != NULL)
	    *end = '\0';

	/* Store the listen address in the XA XN CB. */
	STcopy(listen_address, xa_xn_cb_p->listen_addr);
    }

    return( XA_OK );

} /* connect_to_dbms */




int
IIxa_do_2phase_operation( 
			i4             xa_call,
                        DB_XA_DIS_TRAN_ID *xid,
                        int             rmid,
                        long            flags,
		        i4              branch_seqnum,
		        i4              branch_flag)
{
  IICX_ID            xa_xn_cx_id;
  IICX_CB            *rmi_cx_cb_p;
  IICX_CB            *fe_thread_cx_cb_p;
  IICX_CB            *xa_xn_thrd_cx_cb_p;
  IICX_CB            *xa_xn_cx_cb_p;
  DB_STATUS          db_status;
  IICX_XA_XN_THRD_CB *xa_xn_thrd_cb_p;
  IICX_XA_XN_CB      *xa_xn_cb_p;
  IICX_FE_THREAD_CB  *fe_thread_cb_p;
  i4                 xid_found = FALSE;
  i4                 local_xid = TRUE;
  int                xa_ret_value = XA_OK;
  int		     xa_rb_value = 0;
  i4                 next_xn_branch_state;
  IITUX_XN_ENTRY     *xn_entry = NULL;
  bool		     rollbackonly = FALSE;

    if (!IIxa_startup_done)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("xa_2pc_call: LIBQXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
    	OUTPUT_TRACE_HEADER( xa_call, flags, xid, (XID *)0, &rmid);
        /* output xa_info string */
    }

    /* Unsupported options/flags */
    if (flags & TMASYNC)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_2pc_call: XAER_ASYNC."));
        TRACED_RETURN( XAER_ASYNC );
    }

    /* Validate xa_call/flag combo */
    switch (xa_call)
    {
	case IIXA_COMMIT_CALL:
	    /* only TMNOWAIT, TMONEPHASE, or TMNOFLAGS should be set */
	    if (flags & (~(TMNOWAIT|TMONEPHASE|TMNOFLAGS)))
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                     ERx("xa_2pc_call: invalid flags."));
		TRACED_RETURN( XAER_PROTO );
	    }
	    break;
	case IIXA_ROLLBACK_CALL:
	case IIXA_PREPARE_CALL:
	    if (flags != TMNOFLAGS)
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                     ERx("xa_2pc_call: invalid flags."));
		TRACED_RETURN( XAER_PROTO );
	    }
	    break;
	default:
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("xa_2pc_call: invalid flags."));
	    TRACED_RETURN( XAER_PROTO );
    } /* switch */
		
    if ((xa_ret_value = check_xa_rmi_state(xa_call,rmid,&rmi_cx_cb_p)) != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_2pc_call: invalid RMI state: XAER_PROTO."));
        TRACED_RETURN( xa_ret_value );
    }

    /* check if input arguments are valid */

    db_status = IICXvalidate_xa_xid((DB_XA_DIS_TRAN_ID  *) xid);
    if (DB_FAILURE_MACRO( db_status ))
    {
        TRACED_RETURN( XAER_INVAL );
    }

    /* check the thread state first, for ROLLBACK, this is basically a no-op */
    xa_ret_value = check_fe_thread_state( xa_call,
                         NULL,
                         &fe_thread_cx_cb_p,
                         &fe_thread_cb_p );
    if (xa_ret_value != XA_OK)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
            ERx("xa_2pc_call: invalid FE THREAD state."));
        TRACED_RETURN( xa_ret_value );
    }

    /* 
    ** we used to check the fe_thread_cb_p->curr_xa_xn_cx_cb_p, to
    ** see if it pointed to the xa_xn_cb we wanted.  But now we use that
    ** field to tell us which XID we're actively associated with.  It
    ** either better be NULL, or it better point to a different XID than
    ** we want; otherwise we're trying to do a 2phase operation on an XID
    ** that hasn't been end'd yet.
    */
    IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, (DB_XA_DIS_TRAN_ID *)xid, rmid );
    db_status = IICXfind_lock_cb( &xa_xn_cx_id, &xa_xn_cx_cb_p );
    if (DB_FAILURE_MACRO( db_status ))
        TRACED_RETURN( XAER_RMERR );

    xid_found = (xa_xn_cx_cb_p != NULL) ? TRUE : FALSE;

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {

	if (!xid_found)
	    local_xid = FALSE;

	xn_entry = IItux_tms_find_xid(branch_seqnum, (DB_XA_DIS_TRAN_ID *)xid);

	if (xn_entry)
	    xid_found = TRUE;
	else
	    xid_found = FALSE;
    }

    if (!xid_found)
    {
	/* 
	** This is a recovery operation
	*/

	/*
	** Create the xa_xn_cb
	*/

        db_status = IICXcreate_lock_cb( &xa_xn_cx_id, NULL, &xa_xn_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            TRACED_RETURN( XAER_RMERR );

        IICXlock_cb( fe_thread_cx_cb_p );

        xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

	/*
	** We don't need an xa_xn_thrd_cb for a recover operation
	*/

	xa_xn_thrd_cb_p = NULL;
      
	/*
	** Now try to connect with the XID
	*/
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            char outbuf[200];

            STprintf(outbuf,
            ERx("\tRecovering connection.  Branch_seqnum = %d.  Branch_flags = %d\n"),
		branch_seqnum,branch_flag);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        }

        xa_ret_value = connect_to_dbms(xa_call,
                                  fe_thread_cb_p,
                                  rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p,
                                  xa_xn_cb_p,
                                  xa_xn_cx_id.u_cx_id.xa_xn_id.xid_p,
				  branch_seqnum,
				  branch_flag,
				  NULL);


        if (xa_ret_value != XA_OK)
        {
            IICXdelete_cb( &xa_xn_cx_id );
            IICXunlock_cb( fe_thread_cx_cb_p );
            TRACED_RETURN( xa_ret_value );
      
        }
	else
	{
	    /* 
	    ** need to make sure the new xa_xn_cx_cb_p is in the proper
	    ** state.  CHECK
	    */

	    /* 
	    ** we just connected to perform a recovery operation;  We
	    ** need libq to think its in a transaction so it will do 
	    ** the required commit or rollback
	    */ 

	    /* 
	    ** What we really want is for the server to tell us after a 
	    ** successful connect that we're now in a transaction.  Until 
	    ** that happens, we have to use the following kludge 
	    */ 

	    II_THR_CB	*thr_cb	= IILQthThread();
	    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
	    IIlbqcb->ii_lq_flags |= II_L_XACT;  /* we're in a transaction */ 
	}

        xa_xn_cb_p->ingres_state = IICX_XA_X_ING_IN_USE;
        xa_xn_cb_p->xa_state = IICX_XA_X_PREPARED;

        IICXunlock_cb( fe_thread_cx_cb_p );
    }
    else if (IIXA_CHECK_IF_TUXEDO_MACRO && !local_xid)
    {
	II_THR_CB	*thr_cb;
	II_LBQ_CB	*IIlbqcb;
	IICX_XA_RMI_CB  *xa_rmi_cb_p;

	/* Check the state */

	/* If TUXEDO transaction timeouts are in use and a leg of     */
	/* a transaction has failed due to a timeout, we may not have */
	/* done XA_END, so xn_state may still be as for XA_START      */
	/* allow this rollback through, even though we'll get message */
	/* later that there is nothing to rollback (bug 82316)        */

	if ( !(xn_entry->xn_state & TUX_XN_END) ) 
	{
	    if (xa_call == IIXA_ROLLBACK_CALL)
	    {
	        rollbackonly = TRUE;
		if (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
		{
		    char outbuf[200];
		    STprintf(outbuf, ERx("\tXA_ROLLBACK before XA_END\n"));
		    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
		}
	    }
	    else
	    {
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
                ERx("transaction demarcation call: invalid XN THRD state."));
	        TRACED_RETURN( XAER_RMERR );
	    }
	}
#ifdef xDEBUG
	if (xa_call == IIXA_ROLLBACK_CALL)
	{
	    char	outbuf[512];

	    STprintf(outbuf, "%@\tRollback: thread state is: %d\n", xn_entry->xn_state);
	    IIxa_fe_qry_trace_handler((PTR)0, outbuf, TRUE);
	}
#endif
	/*
	** Create the xa_xn_cb
	*/

	IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, (DB_XA_DIS_TRAN_ID *)xid, rmid );

	/* Specify server also */
	IICXset_xa_xn_server( &xa_xn_cx_id, xn_entry->xn_server_name);

	db_status = IICXcreate_lock_cb( &xa_xn_cx_id, NULL, &xa_xn_cx_cb_p );
	if (DB_FAILURE_MACRO( db_status ))
	    TRACED_RETURN( XAER_RMERR );

	/*
	** We don't need an xa_xn_thrd_cb for 2 phase operation
	*/
	xa_xn_thrd_cb_p = NULL;
	  
	/* 
	** fire up a connection to the back-end, if this is a brand new connect 
	** and if this is not a connection being re-used
	*/
      
	xa_rmi_cb_p = rmi_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;
	xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

	IICXlock_cb( fe_thread_cx_cb_p );

	if (xa_xn_cb_p->ingres_state == IICX_XA_X_ING_NEW)
	{
	    i4  conn_branch_flag = 0;
	    i4  conn_branch_seqnum = 0;

	    xa_ret_value = connect_to_dbms( IIXA_START_CALL,
					  fe_thread_cb_p,
					  xa_rmi_cb_p, 
					  xa_xn_cb_p,
					  NULL,
					  conn_branch_seqnum,
					  conn_branch_flag,
					  xn_entry->xn_server_name);
	    if (xa_ret_value != XA_OK)
	    {
		db_status = IICXdelete_cb( &xa_xn_cx_id );
		IICXunlock_cb( xa_xn_cx_cb_p );
		IICXunlock_cb( fe_thread_cx_cb_p );
		TRACED_RETURN( xa_ret_value );
	    }
	    xa_xn_cb_p->ingres_state = IICX_XA_X_ING_IN_USE;
	    IICXset_xa_xn_server( &xa_xn_cx_id, xn_entry->xn_server_name);
	    IICXset_xa_xn_server( &(xa_xn_cx_cb_p->cx_id),
					xn_entry->xn_server_name);

	    /* log the connection to the trace file */
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	    {
		char	outbuf[300];
		STprintf(outbuf,
		    ERx("\tconnected to %s (INGRES/OpenDTP session %d)\n"),
		    xa_xn_cb_p->connect_name,
		    xa_xn_cb_p->xa_sid);
		IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	    }

	}
	else if (xa_xn_cb_p->ingres_state == IICX_XA_X_ING_IN_USE)
	{
	    /*
	    ** we are re-using a previously established DBMS 
	    ** connection/session
	    */
	    if (xa_xn_cb_p->xa_rb_value != 0)
	    {
		/* something's gone horribly wrong */
                IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
                    ERx("xa_start_call: xa_xn_cx_cb_p marked ROLLBACK ONLY."));
                IICXunlock_cb( xa_xn_cx_cb_p );
                IICXunlock_cb( fe_thread_cx_cb_p );
                TRACED_RETURN( XAER_RMERR );
	    }
	    else
	    {
		/* need to switch to the re-use session */
		EXEC SQL BEGIN DECLARE SECTION;
		i4         reuse_session = xa_xn_cb_p->xa_sid;
		EXEC SQL END DECLARE SECTION;
		i4         save_ingres_state;
 
		save_ingres_state = fe_thread_cb_p->ingres_state;
		fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;
 
		if(IIcx_fe_thread_main_cb->fe_main_flags & 
			IICX_XA_MAIN_TRACE_XA)
		{
		    char    outbuf[300];
		    /* don't need to do anything, except trace the connection */
		    STprintf(outbuf,
			ERx("\treusing connection %s (INGRES/OpenDTP session %d)\n"),
			xa_xn_cb_p->connect_name, xa_xn_cb_p->xa_sid);
		    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
		}

		EXEC SQL SET_SQL (SESSION = :reuse_session);
		fe_thread_cb_p->ingres_state = save_ingres_state;

	    }
	}
	else
	{ 
	    char	ebuf[120];
	    /* the ingres state for this XA XN CB is an illegal/unexpected state */
	    STprintf(ebuf, 
		ERx("xa_start_call: INVALID xa_xn_cb_p->ingres_state = %x."),
		xa_xn_cb_p->ingres_state);
	    IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1, ebuf);
	    TRACED_RETURN( XAER_RMERR );
	}

	/* We need libq to think we're in a transaction */
	thr_cb = IILQthThread();
	IIlbqcb = thr_cb->ii_th_session;
	IIlbqcb->ii_lq_flags |= II_L_XACT;  /* we're in a transaction */
	xa_xn_cb_p->ingres_state = IICX_XA_X_ING_IN_USE;
	if (xa_call == IIXA_PREPARE_CALL)
	    xa_xn_cb_p->xa_state = IICX_XA_X_IDLE;
	else
            xa_xn_cb_p->xa_state = IICX_XA_X_PREPARED;

	IICXunlock_cb( fe_thread_cx_cb_p );
    }
    else   /* Not TUXedo, or Tuxedo but transaction is local */
    {
        IICX_CB            *dummy_xn_cx_cb_p;
	i4		   dummy_next_xn_thrd_state;

        xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

	/*
	** this is not a recover operation, so the xa_xn_thrd cb should
	** exist and it *must* be in the proper state.  We don't care
	** which xa_xn_cb is pointed to by xa_xn_thrd_cb_p, so we pass in
	** a dummy xa_xn_cx_cb_p (we don't care because we found the one
	** we're looking for earlier).  It's also not going to change
	** the state, so we use a dummy for that too.
        */

        xa_ret_value = check_xa_xn_thrd_state( &xa_call,
					   (DB_XA_DIS_TRAN_ID *)xid,
					   &xa_xn_thrd_cx_cb_p,
					   &xa_xn_thrd_cb_p,
					   &dummy_xn_cx_cb_p,
                                           &dummy_next_xn_thrd_state );
        if (xa_ret_value != XA_OK)
        {
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
               ERx("transaction demarcation call: invalid XN THRD state."));

	    /* free xa_xn_cx_id if call is XA_START. */

            IICXunlock_cb( fe_thread_cx_cb_p );
            IICXunlock_cb( xa_xn_cx_cb_p );
            TRACED_RETURN( xa_ret_value );
        }
    }

    /* 
    ** check the xn branch state first (table 6.4) 
    */
    xa_ret_value = check_xn_branch_state( xa_call,
                         xa_xn_cb_p,
                         &next_xn_branch_state );
    if ((xa_ret_value >= XA_RBBASE) && (xa_ret_value <= XA_RBEND))
    {
        xa_rb_value = xa_ret_value;
        xa_ret_value = XA_OK;
    }

    if (xa_ret_value != XA_OK)
    {
        IICXunlock_cb( xa_xn_cx_cb_p );
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
             ERx("xa_2pc_call: invalid XA XN branch state."));
        TRACED_RETURN( xa_ret_value );
    }

    IICXlock_cb( fe_thread_cx_cb_p );
    if (rollbackonly)
	xa_xn_cb_p->xa_state = IICX_XA_X_ROLLBACK_ONLY;
    xa_ret_value = do_2phase_esql( xa_call,
                                   xid,
                                   fe_thread_cb_p,
                                   xa_xn_cb_p,
				   branch_seqnum,
				   branch_flag,
				   xn_entry);

    if (xa_ret_value != XA_OK)
    {
        IICXunlock_cb( fe_thread_cx_cb_p );
        IICXunlock_cb( xa_xn_cx_cb_p );
        TRACED_RETURN( xa_ret_value );
    }
				

    /* 
    ** if this is a commit/rollback, delete the xn context 
    ** OR if this is a tuxedo tms process, not the recovery case.
    */

    if ((xa_call == IIXA_COMMIT_CALL) ||
	(xa_call == IIXA_ROLLBACK_CALL) || 
	(IIXA_CHECK_IF_TUXEDO_MACRO && xid_found))
    {
        db_status = IICXdelete_cb( &xa_xn_cx_id );
        if (DB_FAILURE_MACRO( db_status))
        {
            IICXunlock_cb( fe_thread_cx_cb_p );
            TRACED_RETURN( XAER_RMERR );
        }

	/* 
	** if this is the last one, delete the xa_xn_thrd context too
	** (if it exists).
	*/

	if ( (xa_xn_thrd_cb_p != NULL)
	  && (--(xa_xn_thrd_cb_p->num_reg_rmis) == 0) )
	{
	    IICX_ID	xa_xn_thrd_cx_id;
            IICXsetup_xa_xn_thrd_id_MACRO( xa_xn_thrd_cx_id, 
		(DB_XA_DIS_TRAN_ID *)xid );
	    db_status = IICXdelete_cb( &xa_xn_thrd_cx_id );
            if (DB_FAILURE_MACRO( db_status))
            {
                IICXunlock_cb( fe_thread_cx_cb_p );
                TRACED_RETURN( XAER_RMERR );
            }
	}
    }
    else
    {
        /* set xn branch state */

        xa_xn_cb_p->xa_state = next_xn_branch_state;

        IICXunlock_cb( xa_xn_cx_cb_p );
    }

    if (IIXA_CHECK_IF_TUXEDO_MACRO && xid_found)
    {
	/* Update transaction status in shared memory */
	if (xa_call == IIXA_COMMIT_CALL)
	{
	    xn_entry->xn_state |= TUX_XN_COMMIT; 
	}
        else if (xa_call == IIXA_ROLLBACK_CALL)
        {
	    xn_entry->xn_state |= TUX_XN_ROLLBACK; 
	}
	else /* PREPARE */
	{
	    xn_entry->xn_state |= TUX_XN_PREPARED;
	}

	/* FIX ME set rb_value */

    }

    /* 
    ** If TUXEDO and we just completed a transaction local to this
    ** server, we already cleaned up the transaction,
    ** now clean up the shared memory so that the connection can
    ** be reused immediately.
    ** Of course this would've gotten done the next time this 
    ** server does an xa_start(), however it should be done now 
    ** so this server can use the same connection to commit 
    ** the other branches of this transaction if there are any.
    */
    if (IIXA_CHECK_IF_TUXEDO_MACRO && xid_found && local_xid &&
		xa_call != IIXA_PREPARE_CALL)
    {
	/*
	** If rollbackonly... this is XA_ROLLBACK before XA_END
	** (probably because the transaction branch was waiting on a lock
	** and tuxedo timeout is shorter than ingres lock timeout.)
	** Do not clear the xn_entry unless the branch has been ended
	** because in this case after tuxedo sends the XA_ROLLBACK to the tms
	** server, it will send the XA_END to the application server
	*/
	if (xn_entry->xn_state & TUX_XN_END)
	{
	    xn_entry->xn_inuse = 0;
	    xn_entry->xn_state = 0;
	    xn_entry->xn_rb_value = 0;
	}
    }

    IICXunlock_cb( fe_thread_cx_cb_p );

    if ((xa_rb_value) && (xa_ret_value == XA_OK))
    {
       TRACED_RETURN( xa_rb_value );
    }
    else
    {
       TRACED_RETURN( xa_ret_value );
    }

} /* IIxa_do_2phase_operation */




static  int
do_2phase_esql( i4                xa_call,
                DB_XA_DIS_TRAN_ID *xid,
                IICX_FE_THREAD_CB *fe_thread_cb_p,
                IICX_XA_XN_CB     *xa_xn_cb_p,
	        i4                branch_seqnum,
	        i4                branch_flag,
		IITUX_XN_ENTRY    *xn_entry)
{

EXEC SQL BEGIN DECLARE SECTION;
    i4     current_session = 0;
    i4     save_session = 0;
    i4     save_ingres_state;
    i4     in_a_transaction;
    i4     err_number;
    char   xid_str[IIXA_XID_STRING_LENGTH];
    char   err_msg[256];
    int    xa_ret_value = XA_OK;
    char   area[10];
EXEC SQL END DECLARE SECTION;

    save_ingres_state = fe_thread_cb_p->ingres_state; 
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    EXEC SQL INQUIRE_SQL(:current_session = SESSION);
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
       char outbuf[200];

        STprintf(outbuf,
        ERx("\tcurrent INGRES/OpenDTP session: %d\n"),
		current_session);
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }

    if (current_session != xa_xn_cb_p->xa_sid)
    {
	save_session = current_session;
        current_session = xa_xn_cb_p->xa_sid;

        /* if tracing on...*/
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            char outbuf[200];

            STprintf(outbuf,
            ERx("\tsetting connection to %s (INGRES/OpenDTP session: %d)\n"),
		xa_xn_cb_p->connect_name,
		xa_xn_cb_p->xa_sid);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        }

	EXEC SQL SET_SQL(SESSION = :current_session);

	/* the most reliable way to check for error here */
	EXEC SQL INQUIRE_SQL(:current_session = SESSION);
        if (current_session != xa_xn_cb_p->xa_sid)
	{
            fe_thread_cb_p->ingres_state = IICX_XA_T_ING_IDLE;
	    return (XAER_RMERR);
	}
    }

    if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	in_a_transaction = xn_entry->xn_intrans;
    }
    else
    {
	EXEC SQL INQUIRE_SQL (:in_a_transaction = TRANSACTION);
    }

    if ( (!in_a_transaction) && 
	 (xa_xn_cb_p->xa_state != IICX_XA_X_ROLLBACK_ONLY))
    {
	/* 
	**  we could return an XA_RDONLY if this was a prepare call
	**  we would have to make sure the xa_xn*cbs got nuked tho
	**  CHECK, also, much of the code CHECKs for xa_ret_value !=
	**  XA_OK, which would fail on XA_RDONLY.
	*/
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            IIxa_fe_qry_trace_handler( (PTR) 0, 
	    ERx("\tWARNING - this session has no active transaction.\n"), 
	    TRUE);
        }
    }
    else if ( IIXA_CHECK_IF_TUXEDO_MACRO )
    {
	IICXformat_xa_xid((DB_XA_DIS_TRAN_ID *)xid, xid_str);
	IICXformat_xa_extd_xid(branch_seqnum, branch_flag, xid_str);
        if (xa_call == IIXA_PREPARE_CALL)
        {
	    /* 
	    ** this should be ESQL, not libq calls!  Need to change mgram
	    ** CHECK! 
	    ** IIsqInit(&sqlca);
	    ** IIsqXATPC( xid, branch_seqnum, branch_flag);
	    */

	    IIsqInit(&sqlca);
            IIwritio(0,(short *)0,1,32,0,"prepare to commit with xa_xid=");
            IIvarstrio((short *)0,1,32,0,xid_str);
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
		IIxa_fe_qry_trace_handler( (PTR) 0, ERx("prepare to commit\n"),
				TRUE);
        }
        else if (xa_call == IIXA_COMMIT_CALL)
        {
	    /*
	    ** this should be ESQL, but for now use libq calls.
	    ** EXEC SQL COMMIT WITH XA_XID = ;
	    */
	    IIsqInit(&sqlca);
	    IIwritio(0,(short *)0,1,32,0,"commit with xa_xid=");
	    IIvarstrio((short *)0,1,32,0,xid_str);
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
		IIxa_fe_qry_trace_handler( (PTR) 0, ERx("commit with ...\n"),
				TRUE);
        }
        else
        {
    	    if ( (!in_a_transaction) && 
	       (xa_xn_cb_p->xa_state == IICX_XA_X_ROLLBACK_ONLY))
	       EXEC SQL SET TRACE POINT qe40;

	    /*
	    ** this should be ESQL, but for now use libq calls.
	    ** EXEC SQL ROLLBACK WITH XA_XID = ;
	    */
	    IIsqInit(&sqlca);
	    IIwritio(0,(short *)0,1,32,0,"rollback with xa_xid=");
	    IIvarstrio((short *)0,1,32,0,xid_str);
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
		IIxa_fe_qry_trace_handler( (PTR) 0, ERx("rollback with ...\n"),
				TRUE);
        }
	IIsyncup((char *)0,0);
        if (sqlca.sqlcode < 0)
        {
	    xa_ret_value = XAER_RMERR;
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
		IIxa_fe_qry_trace_handler( (PTR) 0, ERx("ERROR do_2_phase\n"),
				TRUE);
        }
    }
    else  /* Not TUXEDO */
    {
        if (xa_call == IIXA_PREPARE_CALL)
        {
	    /* 
	    ** this should be ESQL, not libq calls!  Need to change mgram
	    ** CHECK! 
	    */

	    IIsqInit(&sqlca);
            IIsqXATPC( xid, branch_seqnum, branch_flag);

        }
        else if (xa_call == IIXA_COMMIT_CALL)
        {
	    EXEC SQL COMMIT;
        }
        else
        {
	    EXEC SQL ROLLBACK;
        }

        if (sqlca.sqlcode < 0)
        {
	    xa_ret_value = XAER_RMERR;
        }
    }

    /* restore the old session if necessary */
    if (save_session != 0)
    {
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            char outbuf[200];
            STprintf(outbuf,
            ERx("\trestoring previous INGRES/OpenDTP session: %d\n"),
		save_session);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}
	EXEC SQL SET_SQL(SESSION = :save_session);
        /* the most reliable way to check for error here */
        EXEC SQL INQUIRE_SQL(:current_session = SESSION);
        if (current_session != save_session)
        {
	    xa_ret_value = XAER_RMERR;
	}
    }

    fe_thread_cb_p->ingres_state = save_ingres_state;

    return( xa_ret_value );

} /* do_2phase_esql */




/*
**  Name: IIxa_connect_to_iidbdb
**
**  Description: 
**	Open a recovery session to the iidbdb for this RMI.  
**      Then open a 
**	cursor to the the lgmo table containing the list of willing_commit 
**	transactions.
**
**  Inputs:
**      fe_thread_cx_cb_p       - thread control block, can be NULL
**      rmid       		- RMI identifier, assigned by the TP system, 
**				  unique to the AS process w/which LIBQXA is 
**				  linked in.
**  Outputs:
**	dbname_p_p		- name of database (without nodename)
**	
**
**  History
**
**      22-Dec-1993 (larrym)
**          Added call to IItux_tms_recover for TUXEDO support
**	06-Jan-1994 (larrym)
**	    added "AND xa_seqnum = 0" to DECLARE CURSOR statement so we'll
**	    only return one XID (in TUXEDO, we can multiple XID's in the 
**	    lgmo table for the same XA transaction).
**	09-mar-1994 (larrym)
**	    split connect_to_iidbdb_setup_cursor into this function and
**	    setup_recovery_cursor so TUXEDO could call this function without
**	    openning a cursor.  Also renamed it IIxa_connect_to_iidbdb
**	    as modules outside this file call it.
*/

int
IIxa_connect_to_iidbdb(IICX_FE_THREAD_CB *fe_thread_cb_p,
       		               IICX_XA_RMI_CB    *xa_rmi_cb_p,
        		       char              **dbname_p_p
                              )
{
    EXEC SQL BEGIN DECLARE SECTION;
        i4     current_rec_session;
        i4     save_rec_session;
	i4    rec_session;
	char   iidbdbname[64];
	char   err_msg[256];
        i4     err_number;
    EXEC SQL END DECLARE SECTION;
    int	    xa_ret_value;
    char    *nodename_p, *iidbdbname_p;
    i4       old_ingres_state;

    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    EXEC SQL INQUIRE_SQL(:save_rec_session = SESSION);

    /* we either need to switch to the recovery session, or start a new */
    /* recovery session */
   
    if (xa_rmi_cb_p->rec_sid)
    {
        /* recovery session already exists ! Just set_sql to it, if necessary */
        /* if tracing on...*/
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            char outbuf[200];

            STprintf(outbuf,
              ERx(
	      "\tcontinuing recovery scan for %s (INGRES/OpenDTP session: %d \n"
	      ),
	      xa_rmi_cb_p->connect_name_p,
	      xa_rmi_cb_p->rec_sid);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        }

        if (save_rec_session != xa_rmi_cb_p->rec_sid)
        {
            rec_session = xa_rmi_cb_p->rec_sid;
   
	    EXEC SQL SET_SQL(SESSION = :rec_session);

	    EXEC SQL INQUIRE_SQL(:current_rec_session = SESSION);
            if (current_rec_session != rec_session)
	    {
		/* CHECK:  Should set to error state */
                fe_thread_cb_p->ingres_state = old_ingres_state;
	        return (XAER_RMERR);
	    }
        }

        /* now we are ready to perform cursor operations. */
    }
    else
    {
        /* we need to get a new connection going to the appropriate iidbdb */

        *dbname_p_p = xa_rmi_cb_p->dbname_p;
        iidbdbname_p = iidbdbname;

        if ((nodename_p = STindex( *dbname_p_p, ERx("::"), 0 )) != NULL)
        {
            /* This is of the format 'nodename::iidbdb' */
 
            STlcopy(*dbname_p_p, iidbdbname_p, nodename_p - *dbname_p_p + 2);
            *(iidbdbname_p + (nodename_p - *dbname_p_p + 3)) = '\0';
            STcat(iidbdbname_p, ERx("iidbdb"));
 	    /* dbname_p_p should not have the vnode in it */
 	    *dbname_p_p = nodename_p + 2;
        }
        else 
        {
            *iidbdbname_p = '\0';
            STcat(iidbdbname_p, ERx("iidbdb"));
        }

	rec_session = IIsqconSysGen;  /* have LIBQ generate a session ID */
        EXEC SQL CONNECT :iidbdbname SESSION :rec_session;

        EXEC SQL INQUIRE_SQL(:err_number = ERRORNO);
        if (err_number)
        {
	    /* error should have been printed by libq */
            fe_thread_cb_p->ingres_state = old_ingres_state;

            return( XAER_RMERR );
        }

        EXEC SQL INQUIRE_SQL(:rec_session = SESSION);

        xa_rmi_cb_p->rec_sid = rec_session;

        /* if tracing on...*/
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
           char outbuf[200];

           STprintf(outbuf,
         ERx("\tbeginning recovery scan for %s (INGRES/OpenDTP session: %d \n"),
	   xa_rmi_cb_p->connect_name_p,
	   xa_rmi_cb_p->rec_sid);
           IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        }



    } /* new connection started to the iidbdb */

    fe_thread_cb_p->ingres_state = old_ingres_state;

    return( XA_OK );

} /* IIxa_connect_to_iidbdb */



/*
**  Name: setup_recovery_cursor
**
**  Description: 
**      open a cursor to the the lgmo table containing the list of 
**	willing_commit transactions, if appropriate.
**
**  Inputs:
**      fe_thread_cx_cb_p       - thread control block, can be NULL
**	dbname_p		- database we're trying to recover
**      flags      		- TP system-specified flags, one of
**                          		TMSTARTRSCAN/TMENDRSCAN/TMNOFLAGS.
**
**  History
**
**	09-Mar-1994 (larrym)
**	    written (from connect_to_iidbdb_setup_cursor)
*/
static  int
setup_recovery_cursor(IICX_FE_THREAD_CB *fe_thread_cb_p,
		      char		*dbname_p,
                      long              flags)

{
EXEC SQL BEGIN DECLARE SECTION;
char	*tmp_dbname_p = dbname_p;
EXEC SQL END DECLARE SECTION;

    i4       old_ingres_state;

    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;
    /* this doesn't generate any code */
    /* declare the cursor thru which XIDs will be retrieved */
    EXEC SQL DECLARE IIXA_REC_CURSOR CURSOR FOR
             SELECT trim(xa_dis_tran_id) FROM lgmo_xa_dis_tran_ids
                    WHERE xa_database_name = :tmp_dbname_p
		    AND   xa_seqnum = 0;

    if( flags & TMSTARTRSCAN)
    {
	if (fe_thread_cb_p->rec_scan_in_progress)
	{
	    /* start the scan over */
	    EXEC SQL CLOSE IIXA_REC_CURSOR;
	}

        /* open a cursor here */
        EXEC SQL OPEN IIXA_REC_CURSOR FOR READONLY;

        if (sqlca.sqlcode < 0)
        {
	    fe_thread_cb_p->rec_scan_in_progress = FALSE;
            fe_thread_cb_p->ingres_state = old_ingres_state;
            return( XAER_RMERR );
        }
    }
    fe_thread_cb_p->ingres_state = old_ingres_state;
    return (XA_OK);
}


static  int
process_recovery_scan( IICX_FE_THREAD_CB *fe_thread_cb_p,
                       IICX_XA_RMI_CB    *xa_rmi_cb_p,
                       XID		 *xids,
                       long              count,
                       long              flags
                     )
{

EXEC SQL BEGIN DECLARE SECTION;
    char   xid_str[IIXA_XID_STRING_LENGTH];
    char   err_msg[256];
EXEC SQL END DECLARE SECTION;
XID       *next_xid;
long      xid_count;
i4	  old_ingres_state;
DB_STATUS db_status = E_DB_OK;
i4        branch_flag;
i4        branch_seqnum;         /* These fields would be checked to */
                                 /* determine uniqueness of XID      */

    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    /* Now retrieve XIDs, convert from text to structure form and place in */
    /* the buffer supplied by the TP library via the xa_recover() call.    */

    for ( next_xid = xids, xid_count = 0;
          (xid_count < count);
          xid_count++, next_xid++ )
    {
          EXEC SQL FETCH IIXA_REC_CURSOR INTO :xid_str;

          if (sqlca.sqlcode != 0)
              break;
          
          if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
          {
	      char outbuf[200];
	      STprintf(outbuf, ERx("\tfound %s\n"), xid_str);
	      IIxa_fe_qry_trace_handler((PTR) 0, outbuf, TRUE);
	  }

/* Call now returns the branch_seqnum and branch_flag */
/* XA IDs with inappropriate flags should be processed here */
/* the rest would be passed back to TP monitor              */

#if defined(LP64)
          db_status = IIconv_to_64_struct_xa_xid( xid_str, 
                                (i4) STlength( xid_str ),
                                (XID *)next_xid,
				&branch_seqnum,
				&branch_flag);
#else
          db_status = IICXconv_to_struct_xa_xid( xid_str, 
                                (i4) STlength( xid_str ),
                                (DB_XA_DIS_TRAN_ID *)next_xid,
				&branch_seqnum,
				&branch_flag);
#endif     /* defined(LP64) */


          if (DB_FAILURE_MACRO( db_status ))
          {
              break;
          }

              

    }

    if ((sqlca.sqlcode < 0) || (DB_FAILURE_MACRO( db_status )))
    {
        if (sqlca.sqlcode < 0)
        {
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("xa_recover: error scanning for  transactions."));
            EXEC SQL DISCONNECT;
            xa_rmi_cb_p->rec_sid = 0;
        }
        fe_thread_cb_p->ingres_state = old_ingres_state;
	return (XAER_RMERR);
    }

    if (sqlca.sqlcode == 100)
    {
        EXEC SQL CLOSE IIXA_REC_CURSOR;

        EXEC SQL DISCONNECT;

        xa_rmi_cb_p->rec_sid = 0;
    }

    fe_thread_cb_p->ingres_state = old_ingres_state;

    return( xid_count );

} /* process_recovery_scan */

static void
output_trace_header( 	i4		xa_call,
			long		flags,
			DB_XA_DIS_TRAN_ID *xid,
			XID               *xa_xid,
			int		*rmid
		   )
{
        char 	outbuf[IIXA_XID_STRING_LENGTH + 100];
	char	xidstr[IIXA_XID_STRING_LENGTH];
	i4	i;
	long 	origflags = flags;
	SYSTIME		time;
	char		buf[32];

	/* If we are using application-wide trace then output xa call/PID & 
        ** process type IF setup 
	*/
	if ( *(IIcx_fe_thread_main_cb->process_id) != EOS )
	{
	    STprintf( outbuf, 
		      ERx("%s call:\n\tProcess ID: %s"), 
		      IIxa_trace_xa_call[xa_call].xa_call_str,
		      IIcx_fe_thread_main_cb->process_id
	            );

	    if ( *(IIcx_fe_thread_main_cb->process_type) != EOS )
	    {
		STcat( outbuf, ERx("\n\tProcess Type: ") );
		STcat( outbuf, IIcx_fe_thread_main_cb->process_type );
	    }

	    STcat( outbuf, ERx("\n\tflags: ") );
	}
	else
	{
	    /* first get the xa_call */
	    STcopy( IIxa_trace_xa_call[xa_call].xa_call_str, outbuf);
	    STcat(outbuf, ERx(" call:\n\tflags: "));
	}

	/* and now process the flags */
	if (flags == TMNOFLAGS)
        {
	    STcat(outbuf, ERx("TMNOFLAGS\n"));
        }
	else
	{
	    for(i = 1; (i < II_NUM_XA_FLAG_VALS) && (flags != 0 ); i++)
	    if (flags & IIxa_trace_flag_val[i].xa_flag_value)
	    {
		if( flags != origflags )
		    STcat(outbuf, ERx(" | "));
		STcat(outbuf, IIxa_trace_flag_val[i].xa_flag_str);
		flags &= ~IIxa_trace_flag_val[i].xa_flag_value;
	    }
	    if(flags)
	    {
		/* invalid flags found */
		char tmpchar[17];

		STcat(outbuf, "| (UNKNOWN VALUE) 0x");
		CVlx(flags, tmpchar);
		STcat(outbuf, tmpchar);
	    }
	    STcat(outbuf, ERx("\n"));
	}
	/* this does an SIfprintf, so it's ok to put format strings in it */
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);

#if defined(LP64)
        /* display the original 64bit XA XID before mapping. */
	if (xa_xid != NULL)
	{
	    IIformat_64_xa_xid((XID *)xa_xid, xidstr );  
	    STpolycat(4, ERx("\tIImap_xa_xid() is called for 64bit XID "), 
			 ERx("processing, the XID before mapping: "), xidstr,
			 ERx("\n"), outbuf ); 
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}
#endif /* defined(LP64) */

        /* and now the XID, if present.  This could be 140 chars! */
	if (xid != NULL)
	{
	    IICXformat_xa_xid((DB_XA_DIS_TRAN_ID *)xid, xidstr );  
	    STpolycat (3 ,ERx("\tXID: "), xidstr, ERx("\n"), outbuf);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}

        /* and now the rmid, if present.  */
	if (rmid != NULL)
	{
	    char   tmpchar[17];

	    CVna(*rmid, tmpchar);
	    STpolycat(3, ERx("\trmid: "), tmpchar, ERx("\n"), outbuf);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}
    TMnow(&time);
    TMstr(&time, buf); 
    STprintf(outbuf, "\tdate/time: %s\n", buf);
    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);

    return;
}

static void
output_trace_return_val( int	xa_ret_val )
{
    i4		i;
    char	outbuf[80];
    char	retvalstr[25];
    SYSTIME	time;
    char	buf[32];

    /* find matching index into flag array */
    for (i=0; (i < II_NUM_XA_RET_VALS) && 
	      (xa_ret_val != IIxa_trace_ret_val[i].xa_ret_val); i++);

    /* if match was not found, just return the string image of value */
    if (i >= II_NUM_XA_RET_VALS)
    {
	CVna(xa_ret_val, retvalstr);
    }
    else
    {
	STcopy(IIxa_trace_ret_val[i].xa_ret_str, retvalstr);
    }

    /* build the puppy */
    STpolycat(3, ERx("\treturn value: "), retvalstr, ERx("\n"), outbuf);

    /* output the stuff, followed by tidy-up line of hyphens */
    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    TMnow(&time);
    TMstr(&time, buf); 
    STprintf(outbuf, "\tdate/time: %s\n", buf);
    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    IIxa_fe_qry_trace_handler( (PTR) 0, IIXATRCENDLN, TRUE);

    return;
}

/*
** Name: check_setup_tuxedo
** 
** Description: Check open string contents to see if we are being accessed by
**              TUXEDO, if we are and the auth bit for tuxedo is set then set
**              fe_thread_main->tuxedo_switch to point to the tuxedo switch
**	        routine.
**
** History:
**	11-Oct-93 (mhughes)
**	    First version
*/
static VOID
check_setup_tuxedo( IICX_XA_RMI_CB 	*xa_rmi_cb_p )
{
    i4  	index;
    bool	tuxedo = FALSE;


    /* Check if we were called by tuxedo */

    for( index=0; index<xa_rmi_cb_p->usr_arg_count; index++ )
    {
        if ( ( !STbcompare( xa_rmi_cb_p->usr_argv[index], 0, 
                            ERx("with"), 0, TRUE ) ) &&
             ( !STbcompare( xa_rmi_cb_p->usr_argv[index+1], 0, 
                            ERx("tmgroup"), 0, TRUE ) ) )
	{
		tuxedo = TRUE;
		break;
	}
    }

    return;
} /* check_setup_tuxedo */


/*{
**  Name: lock_parse_tux_xa_open_string
**
**  Description: 
**     parse an XA open string, and store it's contents/flags in an RMI CB. 
**     the RMI CB is freshly allocated. It is then placed in the processwide 
**     linked list and locked. 
**
**  Inputs:
**      xa_rmi_cb_p	- pointer to an IICX_XA_RMI_CB control block, see 
**			  front!embed!hdr iicxxa.h for description
**      rmid            - RMI identifier, assigned by the TP system, unique 
**			  to the
**                   AS process w/which LIBQXA is linked in.
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_RMERR     - various internal errors.
**          XAER_INVAL     - illegal input arguments.
**
**  Side Effects:
**	    Calls into libqtxxa to initialize DTP for TUXEDO code.
**	
**  History:
**	08-sep-1993  -  First written (mhughes)
**	09-mar-1994 (larrym)
**	    fixed bug 59639.
*/
static  int
lock_parse_tux_xa_open_string( IICX_XA_RMI_CB  *xa_rmi_cb_p, i4  rmid )
{
    char	     *next_char;
    i4               next_token = 0;
    i4               server_group_index=0;
    STATUS	     cl_status;
    int		     xa_ret_value = XA_OK;


    /* The open string is  supposed to be in the format:             
    ** dbname [AS connect_name] WITH TMGROUP aaa  
    **        [options = "-Rroleid/passwd" "..." ] 
    */

    /* Check for illegal argc's. 
    ** Minimum number of args is 4 (dbname with tmgroup aaaa), 
    ** 5 (dbname as with tmgroup aaa) is illegal
    */
    switch ( xa_rmi_cb_p->usr_arg_count )
    {
	case 0:
	case 1:
	case 2:
	case 3:
            return( XAER_INVAL );
    }

    /* first token must be the database name. */
    /* CHECK should do an isalpha here */

    if (*(xa_rmi_cb_p->usr_argv[0]) == '\0')	
    {
        return( XAER_INVAL );
    }
    xa_rmi_cb_p->dbname_p = xa_rmi_cb_p->usr_argv[0];

    /* See if we have a connection name.
    ** First check for 'as' clause which must be word #2 then expect a
    ** name as word #3
    */

    if ( !(STbcompare( xa_rmi_cb_p->usr_argv[1], 0, ERx("as"), 0, TRUE )) )
    {
        xa_rmi_cb_p->connect_name_p = xa_rmi_cb_p->usr_argv[2];
        next_token = 3; 
    }
    else
    {
        xa_rmi_cb_p->connect_name_p = xa_rmi_cb_p->usr_argv[0];
        next_token = 1;
    }

    /* We MUST have a "with tmgroup aaa" clause next */

    if ( ( STbcompare( xa_rmi_cb_p->usr_argv[next_token], 0,  
                       ERx("with"), 0, TRUE) ) ||
         ( STbcompare( xa_rmi_cb_p->usr_argv[next_token+1], 0,
		         ERx("tmgroup"), 0, TRUE ) ) )
    {
        return( XAER_INVAL );
    }
    else
    {
	next_token += 2;

	/* If the next word isn't null, isn't too long and isn't 'options', 
        ** then grab it as it should be server group id.
	*/
        if ( ( xa_rmi_cb_p->usr_argv[next_token] == '\0' ) 	||
             ( STlength( xa_rmi_cb_p->usr_argv[next_token] ) >
               IITUX_MAX_SGID_NAME_LEN ) 			||
             ( STbcompare( xa_rmi_cb_p->usr_argv[next_token], 0 ,
 			    ERx( "options" ), 0, TRUE ) == 0 ) )	
        {
	    return( XAER_INVAL );
	}

	/* Check the server group name's made up of valid ingres
	** name characters
	*/
	server_group_index=next_token;

	for ( next_char  = xa_rmi_cb_p->usr_argv[next_token];
	      *next_char != EOS;
	      CMnext(next_char) )
	{
	    if ( !CMnmchar(next_char))
		return( XAER_INVAL );
	}

    } /* looks like tuxedo */

    /* check for OPTIONS now */

    next_token++;
    if( next_token < xa_rmi_cb_p->usr_arg_count )
    {
        /* looking for "options=" or "options" "=" */
        if ( STbcompare( xa_rmi_cb_p->usr_argv[next_token], 0,
                         ERx("options="), 0, TRUE ) == 0 )
	{
	    xa_rmi_cb_p->flags_start_index = next_token + 1;
	}
	else if ( ( STbcompare( xa_rmi_cb_p->usr_argv[next_token], 0,
                                ERx("options"), 0, TRUE ) == 0 )
             && ( STcompare(xa_rmi_cb_p->usr_argv[next_token+1], ERx("=")) == 0)
	     && ( xa_rmi_cb_p->usr_arg_count > next_token + 2))
	{
             xa_rmi_cb_p->flags_start_index = next_token + 2;
	}
	else 
	{
	    return( XAER_INVAL );
	}
    }

    /*
    ** It's probably TUXEDO.  Make sure and then do tuxedo start up stuff 
    */
    if (IIXA_CHECK_IF_TUXEDO_MACRO)
    {
	check_setup_tuxedo( xa_rmi_cb_p );
        /*
	** Does setup for various process types & Initialises svn into 
        ** Tuxedo global structures.  Only does it for the first 
	** IIxa_open call; additional ones are noops
	*/
	xa_ret_value = (*IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)( 
                          	IIXA_OPEN_CALL, 
				IITUX_ACTION_ONE,  
				(PTR) xa_rmi_cb_p->usr_argv[server_group_index],
				xa_rmi_cb_p,
				&rmid );

    } /* if first OPEN call */
    return( xa_ret_value );

} /* lock_parse_tux_xa_open_string */

/*{
**  Name: IItux_find_xid  - Find XID in the XN_ENTRYs for current AS.
**
**  Description: 
**      We look in the subset of the XN_ENTRY array that was reserved for
**      this AS. We will only find the XID if this AS has participated
**      in the global transaction specified by XID.
**
**  Inputs:
**      xid                - pointer to XID
**
**  Outputs:
**	Returns:
**          NULL           - XID not found
**          !null          - pointer to XN_ENTRY for this XID, this AS
**
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**	
**  History:
**      01-jan-1996  -  Written (stial01,thaju02)
*/
static IITUX_XN_ENTRY *
IItux_find_xid(DB_XA_DIS_TRAN_ID *xid)
{
    i4  i;
    IITUX_LCB       *lcb = &IItux_lcb;
    IITUX_AS_DESC   *cur_as_desc = lcb->lcb_as_desc;
    IITUX_XN_ENTRY  *cur_xn_array = lcb->lcb_cur_xn_array;

    for (i = 0; i < cur_as_desc->as_xn_cnt; i++) 
    {
	if (!cur_xn_array[i].xn_inuse)
	    continue;
	if (!IICXcompare_xa_xid(xid, &cur_xn_array[i].xn_xid)) 
	    return (&cur_xn_array[i]);
    }
    return (NULL);
}

/*{
**  Name: IItux_add_xid- Add XID to the XN_ENTRY array for this AS.
**
**  Description: 
**      We add XID to the XN_ENTRY array that was reserved for this AS.
**      NOTE that during initialization for this AS we reserved entries
**      in the XN_ENTRY array for this AS. We did this so that this AS
**      can update the shared memory without locking it.
**
**  Inputs:
**      xid                - pointer to XID
**      rmid               - resource manager id
**      branch_seqnum      - branch sequence number
**      server_name        - Name of ingres server we connected to
**
**  Outputs:
**	Returns:
**          NULL           - XID not added
**          !null          - pointer to new XN_ENTRY
**
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**	
**  History:
**      01-jan-1996  -  Written (stial01,thaju02)
*/
static IITUX_XN_ENTRY *
IItux_add_xid(DB_XA_DIS_TRAN_ID *xid,
	     int rmid, 
	     i4  branch_seqnum,
	     char *server_name)
{
    i4  i;
    IITUX_LCB       *lcb = &IItux_lcb;
    IITUX_AS_DESC   *as_desc = lcb->lcb_as_desc;
    IITUX_XN_ENTRY  *cur_xn_array = lcb->lcb_cur_xn_array;

    for (i = 0; i < as_desc->as_xn_cnt; i++)
    {
	if (!cur_xn_array[i].xn_inuse)
	    break;
    }

    if (i >= as_desc->as_xn_cnt)
	return (NULL);
    
    MEcopy((char *)xid, sizeof (*xid), &cur_xn_array[i].xn_xid);
    MEcopy(server_name, DB_MAXNAME, cur_xn_array[i].xn_server_name);
    cur_xn_array[i].xn_rmid = rmid;
    cur_xn_array[i].xn_inuse = 1;
    cur_xn_array[i].xn_state = 0;
    cur_xn_array[i].xn_intrans = 0;
    cur_xn_array[i].xn_rb_value = 0;
    cur_xn_array[i].xn_seqnum = branch_seqnum;

    return(&cur_xn_array[i]);
}

/*{
**  Name: IItux_cleanup_threads: Cleanup threads if transaction completed by TMS
**
**  Description: 
**      Search the subset of the XN_ENTRY array reserved for this AS.
**      Check to see if any transactions have been completed for this AS by
**      the TMS. When the TMS completes a transaction, it will update the
**      state of that transaction in shared memory. When the AS sees the
**      transaction is completed, he should clean up the thread, so that
**      it may be used again.
**      After the AS cleans up the thread, it deletes the XID from his
**      XN_ENTRY array.
**
**  Inputs:
**      None
**
**  Outputs:
**      None
**
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**	
**  History:
**      01-jan-1996  -  Written (stial01,thaju02)
*/
static VOID IItux_cleanup_threads(bool *xn_entry_available)
{
    i4  i;
    IITUX_LCB           *lcb = &IItux_lcb;
    IITUX_AS_DESC       *as_desc = lcb->lcb_as_desc;
    IITUX_XN_ENTRY      *cur_xn_array = lcb->lcb_cur_xn_array;
    DB_STATUS           db_status;
    IICX_XA_XN_THRD_CB *xa_xn_thrd_cb_p;
    IICX_ID             xa_xn_thrd_cx_id;
    IICX_ID             xa_xn_cx_id;
    IICX_CB            *xa_xn_thrd_cx_cb_p;
 
    *xn_entry_available = FALSE;
    for (i = 0; i < as_desc->as_xn_cnt; i++)
    {
	if (!cur_xn_array[i].xn_inuse)
	{
	    *xn_entry_available = TRUE;
	    continue;
	}

	if (cur_xn_array[i].xn_state & TUX_XN_ROLLBACK) 
	{
	    /* 
	    ** Switch to the rollback session - and make sure it is ready
	    ** We have to acknowledge any abort - force abort messages here.
	    ** Whenever we see abort errors, xa_xn_cb_p->xa_rb_value gets set.
	    ** We need to acknowledge abort errors here, before we
	    ** reclaim the session (which resets xa_xn_cb_p->xa_rb_value)
	    */
	    db_status = validate_rollback(&cur_xn_array[i]);
	    if (db_status != XA_OK)
		continue;
	}

	/* 
	** cleanup thread if it has committed
	** or (ended AND rolled back)
	*/
	if ((cur_xn_array[i].xn_state & TUX_XN_COMMIT) ||
		    ((cur_xn_array[i].xn_state & TUX_XN_END) && 
		    (cur_xn_array[i].xn_state & TUX_XN_ROLLBACK)))
	{
	    IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, &cur_xn_array[i].xn_xid,
                                                cur_xn_array[i].xn_rmid);
	    db_status = IICXdelete_cb( &xa_xn_cx_id );
	    if (db_status == E_DB_OK)
	    {
		*xn_entry_available = TRUE;
		cur_xn_array[i].xn_inuse = 0;
		cur_xn_array[i].xn_state = 0;
		cur_xn_array[i].xn_rb_value = 0;
	    }

	    /*
            ** if this is the last one, delete the xa_xn_thrd context too
            ** (if it exists).
            */
	    IICXsetup_xa_xn_thrd_id_MACRO( xa_xn_thrd_cx_id, 
					    &cur_xn_array[i].xn_xid);
	    db_status = IICXfind_lock_cb( &xa_xn_thrd_cx_id, 
					    &xa_xn_thrd_cx_cb_p);
	    if (xa_xn_thrd_cx_cb_p)
	    {
		xa_xn_thrd_cb_p = xa_xn_thrd_cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p;
		if ( (xa_xn_thrd_cb_p != NULL)
		  && (--(xa_xn_thrd_cb_p->num_reg_rmis) == 0) )
		{
		    db_status = IICXdelete_cb( &xa_xn_thrd_cx_id );
		}
	    }
	}
    }
}

/*{
**  Name: IItux_tms_find_xid: Find entry for specified XID, branch sequence  
**
**  Description: 
**      Search the entire XN_ENTRY array for the specified XID, branch sequence
**
**  Inputs:
**      xid                - pointer to XID
**      seqnum             - branch sequence number
**
**  Outputs:
**          NULL           - entry not found
**          !null          - pointer to XN_ENTRY for this XID, branch sequence
**
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**	
**  History:
**      01-jan-1996  -  Written (stial01,thaju02)
*/
static IITUX_XN_ENTRY *
IItux_tms_find_xid(i4 seqnum, DB_XA_DIS_TRAN_ID *xid) 
{
    IITUX_LCB       *lcb = &IItux_lcb;
    IITUX_MAIN_CB   *tux_cb = lcb->lcb_tux_main_cb;
    IITUX_XN_ENTRY  *xn_entry;
    i4          xn_cnt;
    i4          i;

    xn_entry = 
         (IITUX_XN_ENTRY *)((char *)tux_cb + tux_cb->tux_xn_array_offset);

    for (i = 0; i < tux_cb->tux_xn_cnt; i++, xn_entry++) 
    {
	if (!xn_entry->xn_inuse)
	    continue;

	if (!IICXcompare_xa_xid(xid, &xn_entry->xn_xid) && 
		xn_entry->xn_seqnum == seqnum)
	{
	    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	    {
		char	outbuf[300];
		STprintf(outbuf,
		    ERx("\ttms_find_xid branch_seqnum %d ix %d)\n"),
		    seqnum, i);
		IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	    }

	    return (xn_entry);
	}
    }

    return (NULL);
}

/*{
**  Name: IItux_max_seq - Return the entry with the max seqnum for this XID
**
**  Description: 
**      Search the entire XN_ENTRY array, looking for all entries for XID.
**      Return the maximum sequence number found in those entries with  
**      the specified XID.
**
**      The branch sequence number ranges from 1 to n, where n is the number
**      of AS's involved in a tuxedo transaction. It has two purposes:
**      First, together with the xa_xid, it provides uniqueness of an
**      XA transactions's thread in the dbms server, and is used in 
**      reconnecting to a suspended, willing-commit thread. The second use
**      is to track whether a 2PC operation has completed on all dbms threads
**      involved in a tuxedo transaction. By examining the rows in the 
**      lgmo table one can, with some additional information, determine
**      the exact state of a tuxedo transaction; or at least enough state
**      to guarantee database consistency.
**
**      During IIxa_start() the maximum sequence number is calculated,
**      so that a new sequence number may be assigned to the transaction
**      being started.
**
**      During IIxa_prepare(), IIxa_commit(), IIxa_rollback(), the maximum 
**      sequence number is used to determine the correct branch_flags to be 
**      used for the current transaction branch.
**
**  Inputs:
**      xid                - pointer to XID
**
**  Outputs:
**	Returns:
**          max_xn_entry   - pointer to entry with max seq number for the
**                           specified XID.
**
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**	
**  History:
**      12-jan-1996  -  Written (stial01)
*/
IITUX_XN_ENTRY *
IItux_max_seq(DB_XA_DIS_TRAN_ID *xid)
{
    IITUX_LCB       *lcb = &IItux_lcb;
    IITUX_MAIN_CB   *tux_cb = lcb->lcb_tux_main_cb;
    IITUX_XN_ENTRY  *xn_entry;
    IITUX_XN_ENTRY  *max_xn_entry = NULL;
    i4         i;

    xn_entry = 
         (IITUX_XN_ENTRY *)((char *)tux_cb + tux_cb->tux_xn_array_offset);

    for (i = 0; i < tux_cb->tux_xn_cnt; i++, xn_entry++) 
    {
	if (!xn_entry->xn_inuse)
	    continue;

	if (!IICXcompare_xa_xid(xid, &xn_entry->xn_xid) && 
	     (!max_xn_entry || xn_entry->xn_seqnum > max_xn_entry->xn_seqnum))
	    max_xn_entry = xn_entry;
    }

    return (max_xn_entry);
}


/*{
**  Name: validate_rollback: Make sure thread marked ROLLBACK is ready to reuse 
**
**  Description: 
**      Make sure FE thread associated with input xid that has been 
**      rolled back, is ready to reuse
**
**  Inputs:
**      xn_entry	  
**
**  Outputs:
**          XA_OK	   - rollback validated
**          Other	   - error
**
**  Side Effects:
**	    None.
**	
**  History:
**      22-oct-2002  -  Written (stial01)
*/
static int
validate_rollback( IITUX_XN_ENTRY *xn_entry)
{
  IICX_ID            xa_xn_cx_id;
  IICX_CB            *fe_thread_cx_cb_p;
  IICX_CB            *xa_xn_thrd_cx_cb_p;
  IICX_CB            *xa_xn_cx_cb_p;
  DB_STATUS          db_status;
  IICX_XA_XN_THRD_CB *xa_xn_thrd_cb_p;
  IICX_XA_XN_CB      *xa_xn_cb_p;
  IICX_FE_THREAD_CB  *fe_thread_cb_p;
  i4                 xid_found = FALSE;
  i4                 local_xid = TRUE;
  int                xa_ret_value = XA_OK;
EXEC SQL BEGIN DECLARE SECTION;
i4		reuse_session;
char		transaction_state[DB_MAXNAME * 2];
EXEC SQL END DECLARE SECTION;
i4     	save_ingres_state;

    /* check if input arguments are valid */
    db_status = IICXvalidate_xa_xid((DB_XA_DIS_TRAN_ID  *) &xn_entry->xn_xid);
    if (DB_FAILURE_MACRO( db_status ))
    {
	TRACED_RETURN( XAER_INVAL );
    }

    xa_ret_value = check_fe_thread_state( IIXA_ROLLBACK_CALL,
			 NULL,
			 &fe_thread_cx_cb_p,
			 &fe_thread_cb_p );
    if (xa_ret_value != XA_OK)
    {
	IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
	    ERx("validate_rollback: invalid FE THREAD state."));
	TRACED_RETURN( xa_ret_value );
    }

    IICXsetup_xa_xn_id_MACRO( xa_xn_cx_id, 
	(DB_XA_DIS_TRAN_ID *)&xn_entry->xn_xid, xn_entry->xn_rmid );
    db_status = IICXfind_lock_cb( &xa_xn_cx_id, &xa_xn_cx_cb_p );
    if (DB_FAILURE_MACRO( db_status ))
    {
	TRACED_RETURN( XAER_RMERR );
    }

    xid_found = (xa_xn_cx_cb_p != NULL) ? TRUE : FALSE;

    if (!xid_found)
    {
	TRACED_RETURN( XAER_RMERR );
    }

    xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
	char	outbuf[300];

	STprintf(outbuf,
       ERx("\tChecking rollback for %s (INGRES/OpenDTP session %d) xa_rb_value %d\n"),
	   xa_xn_cb_p->connect_name,
	   xa_xn_cb_p->xa_sid, xa_xn_cb_p->xa_rb_value);
	IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
    }

    if (xa_xn_cb_p->xa_rb_value != 0)
    {
	return (XA_OK);
    }
    reuse_session = xa_xn_cb_p->xa_sid;

    save_ingres_state = fe_thread_cb_p->ingres_state; 
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    IICXlock_cb( fe_thread_cx_cb_p );
    EXEC SQL SET_SQL (SESSION = :reuse_session);

    /*
    ** Unfortunately this starts a transaction if there isn't one
    EXEC SQL SELECT dbmsinfo('transaction_state') INTO :transaction_state;
    */

    /*
    ** set notrace point will not start transaction
    ** but will force acknowledgement of abort or force abort
    */


    EXEC SQL SET NOTRACE POINT qe40;
    fe_thread_cb_p->ingres_state = save_ingres_state;

    IICXunlock_cb( fe_thread_cx_cb_p );
    IICXunlock_cb( xa_xn_cx_cb_p );
    if (sqlca.sqlcode == 0 || (xa_xn_cb_p->xa_rb_value != 0))
    {
	TRACED_RETURN( XA_OK );
    }
    else
    {
	TRACED_RETURN( XA_RBTRANSIENT);
    }
}



/*
**   Name: IImap_xa_xid()
**
**   Description:
**       Mapping the X/Open transaction identifier from TM (transaction  
**       manager) into the format of DB_XA_DIS_TRAN_ID which is used to   
**       identify the transaction in the front and back end routines in 
**       Ingres. This routine will be called only if the process is in 
**       64-bit mode. 
**
**   Struct :  
**       XID defined in xa.h
**       DB_XA_DIS_TRAN_ID defined in iicommon.h
**
**   Parameters :
**       xa_call   - integer to indicate the caller 
**       xa_xid_p  - pointer to XID.
**       db_xid_p  - pointer to DB_XA_DIS_TRAN_ID.
**
**
**   History:
**       25-Jul-2008 (hweho01)
**         Initial version.
*/
#if defined(LP64)
DB_STATUS
IImap_xa_xid( i4 xa_call, XID *xa_xid_p, DB_XA_DIS_TRAN_ID *db_xid_p )
{

   MEfill( (u_i2)sizeof(DB_XA_DIS_TRAN_ID), '\0', (char *) db_xid_p );
   if ( xa_xid_p != NULL )
   {
     if( xa_call == IIXA_START_CALL )
     {   
       if( xa_xid_p->formatID > MAXI4 || xa_xid_p->formatID == -1 )
       {
           IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1, 
	     ERx("IImap_xa_xid: Invalid or unsupported data in XA XID.") );  
           return( XAER_INVAL );
       }
     }
     if( (db_xid_p->formatID = xa_xid_p->formatID ) != -1 ) 
     {   
	db_xid_p->gtrid_length = xa_xid_p->gtrid_length ;
	db_xid_p->bqual_length = xa_xid_p->bqual_length ;
	MEcopy( (char *)xa_xid_p->data, XIDDATASIZE,
		(char *)db_xid_p->data );
     }
   }
   return( XA_OK ); 
}
#endif  /* defined(LP64) */
/*
**   Name: IIformat_64_xa_xid()
**
**   Description: 
**       Formats the XA XID that is from the 64-bit TM.
**
**   Inputs:
**       xa_xid_p       - pointer to the XA XID.
**       text_buffer    - pointer to a text buffer that will contain the text
**                        form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**       25-Aug-2008 (hweho01) 
**          Written by modifying IICXformat_xa_xid.
*/
#if defined(LP64)
static
VOID
IIformat_64_xa_xid( XID    *xa_xid_p,
                    char   *text_buffer)
{
  char     *cp = text_buffer;
  i4       data_lword_count = 0;
  i4       data_byte_count  = 0;
  u_char   *tp;                  /* pointer to xid data */
  u_i4 unum;                /* unsigned i4 work field */
  i4       i;
  CVlx8(xa_xid_p->formatID,cp);
  STcat(cp, ERx(":"));
  cp += STlength(cp);
  CVla((i4)(xa_xid_p->gtrid_length),cp); /* length 1-64 from gtrid_length */
  STcat(cp, ERx(":"));
  cp += STlength(cp);
  CVla((i4)(xa_xid_p->bqual_length),cp); /* length 1-64 from bqual_length */
  cp += STlength(cp);
  data_byte_count = (i4)(xa_xid_p->gtrid_length + xa_xid_p->bqual_length);
  data_lword_count = (data_byte_count + sizeof(u_i4) - 1) / sizeof(u_i4);
  tp = (u_char*)(xa_xid_p->data);  /* tp -> B0B1B2B3 xid binary data */
  for (i = 0; i < data_lword_count; i++)
  {
     STcat(cp, ERx(":"));
     cp++;
     unum = (u_i4)(((i4)(tp[0])<<24) |   /* watch out for byte order */
                        ((i4)(tp[1])<<16) | 
                        ((i4)(tp[2])<<8)  | 
                         (i4)(tp[3]));
     CVlx(unum, cp);     /* build string "B0B1B2B3" in byte order for all platforms*/
     cp += STlength(cp);
     tp += sizeof(u_i4);
  }
  STcat(cp, ERx(":XA"));
} 
#endif  /* defined(LP64) */

/*
**   Name: IIconv_to_64_struct_xa_xid()
**
**
**   Description: 
**       Converts the XA XID from a text format to an 64 XID structure format,
**       it would be passed back to 64-bit TM ( transaction manager). 
**
**   Inputs:
**       xid_text       - pointer to a text buffer that has the XID in text
**                        format. 
**       xid_str_max_len- max length of the xid string.
**       xa_xid_p       - address of an 64-bit XA XID structure.
**
**   Outputs:
**       Returns: the 64-bit XA XID structure, appropriately filled in.
**
**   History:
**       25-Aug-2008 (hweho01)
**          Written by modifying IICXconv_to_struct_xa_xid().
*/
#if defined(LP64)
static
DB_STATUS
IIconv_to_64_struct_xa_xid(
              char                 *xid_text,
              i4                   xid_str_max_len,
              XID	           *xa_xid_p,
              i4                   *branch_seqnum,
              i4                   *branch_flag)
{
   char        *cp, *next_cp, *prev_cp;
   u_char      *tp;
   char        c;
   i4          j;
   i4          num_xid_words;
   i4     num;            /* work i4 */
   u_i4        unum;
   STATUS      cl_status;

   if ((cp = STindex(xid_text, ":", xid_str_max_len)) == NULL)
   {
       IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
       return( XAER_RMERR );
   }

   c = *cp;
   *cp = '\0';

   if ((cl_status = CVuahxl8(xid_text,(long *)(&xa_xid_p->formatID))) != OK)
   {
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
        ERx("IIconv_to_64_struct_xa_xid:formatID field conversion failed."));
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
      return( XAER_RMERR );
   }

   *cp = c;    

   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
      return( XAER_RMERR );
   }

   c = *next_cp;
   *next_cp = '\0';

   if ((cl_status = CVal(cp + 1, &num)) != OK)
   {
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
      ERx("IIconv_to_64_struct_xa_xid:gtrid_length field conversion failed."));
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
      return( XAER_RMERR );
   }
   xa_xid_p->gtrid_length = num;  /* 32 or 64 bit long gtrid_length = i4 num */

   *next_cp = c;    

   if ((cp = STindex(next_cp + 1, ":", 0)) == NULL)
   {
       IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
       return( XAER_RMERR );
   }

   c = *cp;
   *cp = '\0';

   if ((cl_status = CVal(next_cp + 1, &num)) != OK)
   {
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
      ERx("IIconv_to_64_struct_xa_xid:bqual_length field conversion failed."));
      IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
      return( XAER_RMERR );
   }
   xa_xid_p->bqual_length = num;  /* 32 or 64 bit long bqual_length = i4 num */

   *cp = c;    

    /* Now copy all the data                            */
    /* We now go through all the hex fields separated by */
    /* ":"s.  We convert it back to bytes and store it in */
    /* the data field of the distributed transaction id  */
    /*     cp      - points to a ":"                       */
    /*     prev_cp - points to the ":" previous to the     */
    /*               ":" cp points to, as we get into the  */
    /*               loop that converts the XID data field */
    /*     tp    -   points to the data field of the       */
    /*               distributed transaction id            */

    prev_cp = cp;
    tp = (u_char*) (&xa_xid_p->data[0]);
    num_xid_words = (xa_xid_p->gtrid_length+
            xa_xid_p->bqual_length+
            sizeof(u_i4)-1)/sizeof(u_i4);
    for (j=0;
         j< num_xid_words; 
         j++)
    {

         if ((cp = STindex(prev_cp + 1, ":", 0)) == NULL)
         {
             IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
             return( XAER_RMERR );
         }
         c = *cp;
         *cp = '\0';
         if ((cl_status = CVuahxl(prev_cp + 1,&unum)) != OK)
         {
             IIxa_error(GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
             ERx("IIconv_to_64_struct_xa_xid:xid_data field conversion failed."));
             IIxa_error(GE_LOGICAL_ERROR, E_LQ00D4_XA_BAD_VALUE, 1, xid_text);
             return(  XAER_RMERR );
         }
         tp[0] = (u_char)((unum >> 24) & 0xff);  /* put back in byte order */
         tp[1] = (u_char)((unum >> 16) & 0xff);
         tp[2] = (u_char)((unum >>  8) & 0xff);
         tp[3] = (u_char)( unum        & 0xff);
         tp += sizeof(u_i4);
         *cp = c;
         prev_cp = cp;
    }

   return( XA_OK );

} /* IIconv_to_64_struct_xa_xid */
#endif  /* defined(LP64) */

