/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0c.h>
#include    <dm0llctx.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <dm0p.h>
#include    <dm0pbmcb.h>
#include    <dmxe.h>
#include    <dmd.h>
/**
**
**  Name: DMFTRACE.C - DMF trace entry point.
**
**  Description:
**
**	Function prototypes defined in DMFTRACE.H.
**
**      This routine takes a DMF trace request and processes it.
**
**          dmf_trace - The DMF trace entry point.
**
**
**  History:
**      16-jun-1986 (Derek)    
**          Created for Jupiter.
**	31-oct-1988 (rogerk)
**	    Added calls to dump server control blocks.
**	10-apr-1989 (rogerk)
**	    Added trace point to toss pages from buffer manager.
**	    Added test trace point for fast commit test support.
**	    Added dmf_tst_point routine to execute test trace points.
**	19-apr-1990 (rogerk)
**	    Added DMZ_TST_MACRO(19) trace point to crash logging system.
**	25-sep-1991 (mikem) integrated following change:  4-feb-1991 (rogerk)
**	    Added DM1305 to force Consistency Point.
**	    Changed DM421 to toss all pages (including modified pages)
**	    from the buffer manager.  It used to skip modified pages.
**	    Added DM422 to behave like DM421 used to.
**	    Added DM425 to fix cache priority for a table.
**	    Created one call for session cb to be used by all trace
**	    points which need it.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Added archiver test trace points.
**	    Changed DM425 to take index_id as second trace argument.
**	    Changed dm0p_stbl_priority call to use a regular DB_TAB_ID.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (bryanp)
**	    Added dm615 <tblid> to print a Btree table.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (rogerk)
**	    Changed archiver test records to pass DM0L_JOURNAL flag.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	25-aug-1992 (ananth)
**	    Get rid of HP compiler warnings.
**	23-sep-1992 (bryanp)
**	    Prototyping LG and LK
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: added lock list arg to dm0p_stbl_priority.
**	04-nov-92 (jrb)
**	    Added "test trace point" DM1440 for ML Sorts; this tracepoint
**	    displays the current set of work locations being used to the trace
**	    output.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add trace point DM1434 to force the logfile to disk, for
**		    use by recovery testing which wishes to know when log recs
**		    are safely out.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Added a comment about the behavior of session trace points, because
**		it surprised me when I found it out.
**	24-may-1993 (rogerk)
**	    Added trace_immediate routine and new recovery test trace points.
**	21-jun-1993 (bryanp)
**	    Fixed bug in trace_immediate support for DM101.
**	    Added support for DM303 - VMS only; call LKalter(LK_CSP_DEBUG_SEND)
**	    Fixed bug in trace point DM1434.
**	21-june-1993 (jnash)
**	    Add DM1435 to mark database inconsistent for recovery testing.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Added DM1314 to signal an archive cycle.
**      26-jul-1993 (mikem)
**          - Add DM1436 to test performance of LG calls.
**          - Add DM1437 to test performance of LGwrite() calls.
**          - Add DM1438 to test performance of ii_nap() calls (unix only).
**          - Add DM1439 to test performance of system calls from server.
**          - Add DM1441 to test performance of cross process sem calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Trace point DM714 allows setting of svcb_page_size.
**      06-mar-1996 (stial01)
**          Only allow svcb_page_size to be set to page size for installation.
**	27-jan-97 (stephenb)
**	    Add code for DM102.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    DM425 no longer does anything. Table priority can now be set when
**	    table is created or modified.
**	    Added DM435 to notify dm0p that fixed Table cache priorities set in
**	    this manner are to be ignored.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages() calls.
**	26-may-1998 (nanpr01)
**	    Added DM1002, DM1003 trace point to change the exchange buffer
**	    size and number of exchange buffers.
**	13-Dec-1999 (jenjo02)
**	    Added DM309 to dump lock list information when deadlocks occur.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Replace calls to SCU_INFORMATION to get *DML_SCB with macro.
**	    Pass *DML_SCB to static functions.
**      01-feb-2001 (stial01)
**          Updated valid page types for DM721 (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      06-apr-2001 (stial01)
**          Added DM34: Display create table/index page type information
**	31-Jan-2003 (jenjo02)
**	    Added DM414 to trace Cache Flush activity.
**      16-sep-2003 (stial01)
**          Added DM722 to set svcb_etab_type (SIR 110923)
**      25-nov-2003 (stial01)
**          Added DM723 to set etab logging on/off
**      02-jan-2004 (stial01)
**          Removed temporary trace points DM722, DM723
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Aug-2004 (jenjo02)
**	    Added DM36 to set session's degree of parallelism.
**      01-dec-2004 (stial01)
**          Added DM722 back as blob tracepoint to validate coupons
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Added DM105 to start/stop/display DMF memory stats
**	    by "type".
**	21-Mar-2006 (jenjo02)
**	    Add DM1442, DM1443 for optimized log writes.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Use new form of uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**      21-Jan-2009 (horda03) Bug 121519
**          Add DM1444 to set the last allocated table ID for the
**          database.
**      05-Feb-2009 (horda03) Bug 121519
**          New prototype for dm0c_open/close introduced by SIR 120874.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DM405, 406 for MVCC tracing.
*/

/*
**  Definition of static variables and forward static functions.
*/

static	VOID	    trace_immediate(
			    DML_SCB	*scb,
			    DB_DEBUG_CB	    *debug_cb,
			    i4	    number);
static	VOID	    print_btree_table(
			    DML_SCB	*scb,
			    DB_TAB_ID	*tableid);    /* DM615 implementation */
static VOID 	    dmf_tst_point(
			    DML_SCB	*scb,
    			    i4      number, 
    			    DB_DEBUG_CB  *debug_cb);

/*{
** Name: dmf_trace	- DMF trace entry point.
**
** Description:
**      This routine takes a standard trace control block as input, and
**	sets or clears the given trace point.  The trace points are either
**	specific to a session or server wide.
**
**	Actually, session trace points are sort of *BOTH* session specific AND
**	server-wide. Consider, for example, "set lock_trace" (DM0001). There
**	are two tracepoint bit vectors in the world, the server-wide tracepoint
**	bit vector (in the dmf_svcb), and the session-specific tracepoint bit
**	vector (in each session's SCB). Setting trace point DM1 causes your
**	session's tracepoint bit vector to have bit 1 set, AND ALSO CAUSES THE
**	SERVER-WIDE TRACEPOINT VECTOR TO HAVE BIT 1 SET! This is because the
**	code which is sprinkled throughout DMF to test DMZ_SES_MACRO(1) doesn't
**	want to be bothered looking up the session's control block. So what
**	happens is that as soon as you set lock_trace on in your session, all
**	sessions in the server begin calling dmd_lock(). dmd_lock, in turn,
**	looks up the session control block for each session and checks the value
**	of bit 1 in the session tracepoint vector. For all sessions except your
**	session, the bit is off, so dmd_lock just returns. For your session,
**	the bit is on, so dmd_lock does its thing.
**
**	When your session dies, or you do a "set notrace point dm1", we
**	really don't want the server forever-after calling the tracing
**	wrappers, doing get-SID calls, etc.  So we'll recompute the session
**	tracepoint section of the server trace vector.  This is the first
**	96 bits (although DMZ_POINTS is 100, we'll hardcode in 96 since
**	that way we can manage that part of the vector by hand as 3 i4's).
**	Recalculating this portion of the server trace vector is a PITA,
**	but it's better than inducing the server overhead indefinitely -
**	regardless of how minor that overhead might be.
**
** Inputs:
**      debug_cb                        A standard debug control block.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-1986 (Derek)
**          Created  for Jupiter.
**	31-oct-1988 (rogerk)
**	    Added calls to dump server control blocks.
**	10-apr-1989 (rogerk)
**	    Added trace point to toss pages from buffer manager.
**	    Added test trace point for fast commit test support.
**	25-sep-1991 (mikem) integrated following change:  4-feb-1991 (rogerk)
**	    Added DM1305 to force Consistency Point.
**	    Changed DM421 to toss all pages (including modified pages)
**	    from the buffer manager.  It used to skip modified pages.
**	    Added DM422 to behave like DM421 used to.
**	    Added DM425 to fix cache priority for a table.
**	    Created one call for session cb to be used by all trace
**	    points which need it.
**	25-sep-1991 (mikem) integrated following change: 22-mar-91 (jrb)
**	    Fixed unix compiler warning on MEcopy call.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Changed DM425 to take index_id as second trace argument.
**	    Changed dm0p_stbl_priority call to use a regular DB_TAB_ID.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (bryanp)
**	    Added dm615 <tblid> to print a Btree table.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: changes in buffer manager protocols
**	    required that the dm0p_stbl_priority call include a lock list arg.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Add trace point DM1434 to force the logfile to disk, for
**		    use by recovery testing which wishes to know when log recs
**		    are safely out.
**	24-may-1993 (rogerk)
**	    Added DM427 and DM428 to toss selected pages from the cache.
**	    Added DM713 to toss selected TCB's from the server.
**	    These are used to assist recovery testing.
**	    Moved the trace points that are executed immediately to a new
**	    routine (trace_immediate).  In addition to modularizing some of
**	    the code, this fixes the problem where test trace points could
**	    be executed unintentionally due to a different session being in
**	    the middle of a test trace point.  (If the trace point vector is
**	    turned on while the trace point is being executed then if another
**	    session calls dmf_trace it may accidentally execute the same
**	    test trace point).
**	21-jun-1993 (jnash)
**          Add xDEBUG DM1435 code to mark database inconsistent.
**	26-jul-1993 (rogerk)
**	    Added DM1314 to signal an archive cycle.
**      26-jul-1993 (mikem)
**          - Add DM1436 to test performance of LG calls.
**          - Add DM1437 to test performance of LGwrite() calls.
**          - Add DM1438 to test performance of ii_nap() calls (unix only).
**          - Add DM1439 to test performance of system calls from server.
**          - Add DM1441 to test performance of cross process sem calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Trace point DM714 allows setting of svcb_page_size.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    DM425 no longer does anything. Table priority can now be set when
**	    table is created or modified.
**	    Added DM435 to notify dm0p that fixed Table cache priorities set in
**	    this manner are to be ignored.
**	26-may-1998 (nanpr01)
**	    Added DM1002, DM1003 trace point to change the exchange buffer
**	    size and number of exchange buffers.
**	12-Aug-2004 (jenjo02)
**	    Added DM36 to set session's degree of parallelism.
**	22-Jul-2004 (schka24)
**	    Recalculate svcb session tracepoints after turning one off.
**	    Remove ability to set/clear all class tracepoints by specifying
**	    tracepoint xx00, doesn't really make any sense.
*/
DB_STATUS
dmf_trace(
DB_DEBUG_CB	    *debug_cb)
{
    DB_DEBUG_CB	    *d = debug_cb;
    DML_SCB	    *scb;
    CS_SID	    sid;
    u_i4	    *vector = dmf_svcb->svcb_trace;
    i4	    operation = (d->db_trace_point / 10000) % 10;
    i4	    class = (d->db_trace_point / 100) % 100;
    i4	    trace_point = (d->db_trace_point % 100);
    i4	    bitno = d->db_trace_point;
    i4	    i;

    if (operation >= DMZ_OPERATIONS || class >= DMZ_CLASSES)
	return (E_DB_ERROR);
    
    /* Get pointer to session control block */
    CSget_sid(&sid);
    scb = GET_DML_SCB(sid);

    /*
    ** If session trace point, then get session trace vector to set
    ** trace point in.
    */
    if (class == DMZ_SESCLASS)
    {

	/* Get the DMF SCB list mutex.  It's needed for setting to
	** make sure nobody is simultaneously recalc'ing the server
	** bits, and it's needed for clearing so that we can recalc
	** the server bits.
	*/

	dm0s_mlock(&dmf_svcb->svcb_sq_mutex);

	/*  Mark session trace enable in the server trace table. */

	if (d->db_trswitch == DB_TR_ON)
	    DMZ_SET_MACRO(vector, bitno);

	vector = scb->scb_trace;
	if (d->db_trswitch == DB_TR_ON)
	{
	    DMZ_SET_MACRO(vector, bitno);	/* In session vector too */
	}
	else
	{
	    DMZ_CLR_MACRO(vector, bitno);	/* From session vector */
	    dmf_trace_recalc();			/* Recompute server bits */
	}
	dm0s_munlock(&dmf_svcb->svcb_sq_mutex);
	/* Fall thru, we'll redundantly set the session bits, but we
	** need to take care of immediate tracepoints (and we don't want
	** to hold the mutex!)
	*/
    }

    if (d->db_trswitch == DB_TR_ON)
    {
	DMZ_SET_MACRO(vector, bitno);

#ifdef	xDEBUG
	if (DMZ_CLL_MACRO(12))
	{
	    dmd_timer(3, 0, 0);
	    DMZ_CLR_MACRO(vector, bitno);
	}
#endif

	/*
	** Check for Execute Immediate trace points.
	**
	**	DM36	- Set session's degree of parallelism.
	**	DM101	- Dump DMF memory (like dmd_check output).
	**	DM102	- Disable replication for this database.
	**	DM303	- Send SS$_DEBUG request to DMFCSP process (cluster)
	**	DM309	- Enable/disable deadlock debug dumping
	**	DM405	- Turn on/off MVCC CR log and jnl tracing for database
	**	DM406	- Turn on/off MVCC CR jnl-only tracing for database
	**	DM414	- Enable/disable Cache Flush tracing
	**	DM420	- Dump Buffer Manager Statistics.
	**	DM421	- Toss pages from buffer cache.
	**	DM422	- Toss unmodified pages from buffer cache.
	**	DM425	- Set table cache priority (obsolete)
	**	DM427	- Toss single page from buffer cache.
	**	DM428	- Toss pages for single table from buffer cache.
	**	DM615	- Print a Btree table to server trace log.
	**	DM713	- Toss table TCB from server TCB cache.
	**	DM714	- Set svcb_page_size.
	**      DM721   - Set svcb_page_type.
	**	DM1002  - Set the parallel index exchange buffer size(in rows).
	**	DM1003  - Set the number of parallel index exchange buffer.
	**	DM1305	- Force Consistency Point.
	**	DM1314	- Force Archive Cycle.
	**	DM1434  - Force log file to disk.
	**	DM1435  - Mark database inconsistent
	**	DM1442  - Toggle LG optimized writes.
	**	DM1443  - Toggle LG optimized write tracing.
        **      DM1444  - Set the next Table ID for the DB
	**
	** Rather than set a bit in the trace point bitmask to cause some
	** future action, these cause an immediate action.
	*/
	if ((DMZ_MEM_MACRO(1))  || (DMZ_BUF_MACRO(20)) || (DMZ_BUF_MACRO(21)) ||
	    (DMZ_BUF_MACRO(22)) || (DMZ_BUF_MACRO(25)) || (DMZ_BUF_MACRO(27)) ||
	    (DMZ_BUF_MACRO(28)) || (DMZ_TBL_MACRO(13)) || (DMZ_AM_MACRO(15)) ||
	    (DMZ_ASY_MACRO(5))  || (DMZ_ASY_MACRO(14)) || (DMZ_LCK_MACRO(3)) ||
	    (DMZ_TST_MACRO(34)) || (DMZ_TST_MACRO(35)) || (DMZ_TBL_MACRO(14)) ||
	    (DMZ_MEM_MACRO(2) || DMZ_UTL_MACRO(2) || DMZ_UTL_MACRO(3)) ||
	    (DMZ_LCK_MACRO(9)) || (DMZ_TBL_MACRO(21)) ||
	    (DMZ_SES_MACRO(34)) ||
	    (DMZ_BUF_MACRO(14)) ||
	    (DMZ_SES_MACRO(36)) ||
	    (DMZ_MEM_MACRO(5))  ||
	    (DMZ_TST_MACRO(42)) ||
	    (DMZ_TST_MACRO(43)) || (DMZ_TST_MACRO(44)) ||
	    (DMZ_BUF_MACRO(5))  || (DMZ_BUF_MACRO(6))
	    )
	{
	    trace_immediate(scb, d, bitno);
	    DMZ_CLR_MACRO(vector, bitno);
	    /* Note for session immediates, this leaves the bit on in the
	    ** server bit-vector, but these bits don't matter
	    */
	}

	/*
	** If test trace point, call test trace point routine.
	*/
	if (class == DMZ_TSTCLASS)
	{
	    dmf_tst_point(scb, trace_point, debug_cb);
	    DMZ_CLR_MACRO(vector, bitno);
	}

    }
    else if (d->db_trswitch == DB_TR_OFF && 
		(bitno == 102 || bitno == 309 || bitno == 414 ||
		 bitno == 105 || bitno == 1442 || bitno == 1443 ||
		 bitno == 405 || bitno == 406
		)
	    )
    {
	trace_immediate(scb, d, bitno);
	DMZ_CLR_MACRO(vector, bitno);
    }
    else
    {
	DMZ_CLR_MACRO(vector, bitno);
    }

    return (E_DB_OK);    
}

/*{
** Name: dmf_trace_recalc - Recalculate serverwide session tracepoints
**
** Description:
**	After a session tracepoint is cleared, or a session exits, we
**	want to recalculate the session trace bits in the server trace
**	vector.  We can't just clear the bits there, because someone
**	else might have the same tracepoint on.  And we don't want to
**	do nothing, because then the server is forever after calling
**	trace wrappers which check session bits, wasting time.
**
**	In order to operate more conveniently with i4's, the number
**	of session tracepoints has been limited to 96 instead of the
**	usual 100.  That's 3 32-bit i4's.  I will just hardcode this
**	in (ugh) because at this stage nobody is going to massively
**	reshuffle all the tracepoint classes.
**
**	Call with the dmf_svcb->svcb_sq_mutex held, so that we can
**	traverse the DMF SCB list safely.
**
** Inputs:
**	None.  (Need svcb_sq_mutex.)
**
** Outputs:
**	None.
**	The first 96 bits of the svcb trace vector are recalculated.
**
** History:
**	22-Jul-2004 (schka24)
**	    Written.
*/

void
dmf_trace_recalc(void)
{
    DML_SCB *scb;
    u_i4 *server_bits, *session_bits;

    /* Clear the existing session bits and OR in all the session's bits.
    ** This is sort of brute force, but simple and pretty quick.
    ** 96 bits == 3 i4's, [0..2]
    */

    /* Compiler warning desired if trace vector not defined as i4 */
    server_bits = &dmf_svcb->svcb_trace[0];
    server_bits[0] = server_bits[1] = server_bits[2] = 0;

    scb = dmf_svcb->svcb_s_next;
    while (scb != (DML_SCB *) &dmf_svcb->svcb_s_next)
    {
	session_bits = &scb->scb_trace[0];
	server_bits[0] |= session_bits[0];
	server_bits[1] |= session_bits[1];
	server_bits[2] |= session_bits[2];
	scb = scb->scb_q_next;
    }
} /* dmf_trace_recalc */

/*{
** Name: trace_immediate	- Execute immediate trace point
**
** Description:
**	This routine implements the dmf execute immediate trace points.
**
**	These are trace points that cause the server to take some immediate
**	action rather than actually set a trace point. There are called by
**	issuing the command "Set trace point DMXXX" - where XXX is the
**	trace point number.
**
**	Generally these are undocumented and used for testing puposes.
**
**	Trace points implemented:
**
**	DM36	- Set session's degree of parallelism.
**	DM101	- Dump DMF memory (like dmd_check output).
**	DM303	- Send SS$_DEBUG request to DMFCSP process (cluster only)
**	DM309	- Turn on/off LK deadlock dumping.
**	DM405	- Turn on/off MVCC CR log and jnl tracing for database
**	DM406	- Turn on/off MVCC CR jnl-only tracing for database
**	DM414	- Turn on/off Cache Flush tracing for database
**	DM420	- Dump Buffer Manager Statistics.
**	DM421	- Toss pages from buffer cache.
**	DM422	- Toss unmodified pages from buffer cache.
**	DM425	- Set table cache priority (obsolete)
**	DM427	- Toss single page from buffer cache.
**	DM428	- Toss pages for single table from buffer cache.
**	DM615	- Print a Btree table to server trace log.
**	DM713	- Toss table TCB from server TCB cache.
**	DM714	- Set svcb_page_size.
**      DM721   - Set svcb_page_type.
**	DM1002  - Set the parallel index exchange buffer size(in rows).
**	DM1003  - Set the number of parallel index exchange buffer.
**	DM1305	- Force Consistency Point.
**	DM1434  - Force log file to disk.
**	DM1442  - Toggle LG optimized writes.
**	DM1443  - Toggle LG optimized write tracing.
**
** Inputs:
**      debug_cb    - the debug control block.
**      trace_point - trace point number.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    May vary depending on test trace number.
**
** History:
**	24-may-1993 (rogerk)
**          Moved here from general dmf_trace routine.
**	21-jun-1993 (bryanp)
**	    Fixed bug in trace_immediate support for DM101.
**	    Added support for DM303 - VMS only; call LKalter(LK_CSP_DEBUG_SEND)
**	    Fixed bug in trace point DM1434.
**	21-jun-1993 (jnash)
**          Add xDEBUG DM1435 code to mark database inconsistent.
**	26-jul-1993 (rogerk)
**	    Added DM1314 to signal an archive cycle.
**	06-mar-1996 (stial01 for bryanp)
**	    Trace point DM714 allows setting of svcb_page_size.
**      06-mar-1996 (stial01)
**          Only allow svcb_page_size to be set to page size for installation.
**	27-jan-97 (stephenb)
**	    Add code for DM102.
**	01-Apr-1997 (jenjo02)
**	    Defeated DM425 (set table cache priority).
**	26-may-1998 (nanpr01)
**	    Added DM1002, DM1003 trace point to change the exchange buffer
**	    size and number of exchange buffers.
**	13-Dec-1999 (jenjo02)
**	    Added DM309 to dump lock list info when deadlocks occur.
**	12-Aug-2004 (jenjo02)
**	    Added DM36 to set session's degree of parallelism.
**	24-Jan-2007 (kschendel)
**	    Allow dm713 to run outside of a transaction.  In that case
**	    it's OK to use the session lock list and the default BM log ID.
**      21-Jan-2009 (horda03) Bug 121519
**          Aded DM1444 to set DB's table ID.
**      27-Mar-2009 (hanal04) Bug 121857
**          With xDEBUG defined we failed to build because of undeclared
**          variable references to cnf.
*/
static VOID
trace_immediate(
DML_SCB	       *scb,
DB_DEBUG_CB    *debug_cb,
i4		trace_point)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DB_DEBUG_CB	    *d = debug_cb;
    DMP_DCB	    *dcb;
    DML_XCB	    *xcb;
    LG_LSN	    nlsn;
    DB_TAB_ID	    table_id;
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    page_number;
    i4	    tabid;
    i4	    item;
    i4	    error;
    DB_ERROR	    local_dberr;


    /* scb = *DML_SCB of session */

    /*
    **	DM34	- Display create table/index page type information.
    */
    if (trace_point == 34)
    {
	dmd_page_types();
    }

    if ( trace_point == 36 && d->db_value_count == 1 )
	scb->scb_dop = d->db_vals[0];

    /*
    **	DM101	- Dump DMF memory (like dmd_check output).
    */
    if (trace_point == 101)
    {
	dmd_dmp_pool(1);
    }

    /*
    ** DM102	- Disable/enable  replication for a database
    */
    if (trace_point == 102)
    {
	if (d->db_trswitch == DB_TR_ON)
	{
	    scb->scb_oq_next->odcb_dcb_ptr->dcb_status &= ~DCB_S_REPLICATE;
	}
	else
	{
	    if (scb->scb_oq_next->odcb_dcb_ptr->rep_regist_tab.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->rep_regist_tab_idx.db_tab_base
		&& scb->scb_oq_next->odcb_dcb_ptr->rep_regist_col.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->rep_input_q.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->rep_dd_paths.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->rep_dd_db_cdds.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->rep_dist_q.db_tab_base &&
		scb->scb_oq_next->odcb_dcb_ptr->dcb_rep_db_no)
		scb->scb_oq_next->odcb_dcb_ptr->dcb_status |= DCB_S_REPLICATE;
	}
    }

    /*
    **  DM105	- DM0M memory stats by type
    */
    if ( trace_point == 105 )
	dm0m_stats_by_type((d->db_trswitch == DB_TR_ON) ? TRUE : FALSE);

    /*
    **	DM303	- Ask LKalter to place CSP into the debugger.
    */
    if (trace_point == 303)
    {
	if (status = LKalter(LK_A_CSP_ENTER_DEBUGGER, 0L, &sys_err))
	{
	    (VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	}
    }

    /*
    **  DM309	- Turn deadlock dumping on/off
    */
    if ( trace_point == 309 )
    {
	i4	flag = (d->db_trswitch == DB_TR_ON);

	if ( status =  LKalter(LK_A_SHOW_DEADLOCKS, 
			(d->db_trswitch == DB_TR_ON) ? 1 : 0,
			&sys_err))
	{
	    (VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	}
    }

    /*
    **  DM405-406 - Manage MVCC tracing in connected database.
    **
    **     DM405: turn on/off MVCC tracing
    **     DM406: turn on/off only jnl tracing
    */
    if ( trace_point == 405 || trace_point == 406 )
    {
	/* Ignore if not MVCC-enabled database */
        if ( (dcb = scb->scb_oq_next->odcb_dcb_ptr) &&
	      dcb->dcb_status & DCB_S_MVCC )
	{
	    /* Keep out the riffraff while we fiddle with dcb_status */
	    dm0s_mlock(&dcb->dcb_mutex);

	    if ( d->db_trswitch == DB_TR_ON )
	    {
	        if ( trace_point == 405 )
		{
		    dcb->dcb_status &= ~DCB_S_MVCC_JTRACE;
		    dcb->dcb_status |= DCB_S_MVCC_TRACE;
		}
		else if ( trace_point == 406 )
		{
		    dcb->dcb_status &= ~DCB_S_MVCC_TRACE;
		    dcb->dcb_status |= DCB_S_MVCC_JTRACE;
		}
	    }
	    else
	    {
	        if ( trace_point == 405 )
		    dcb->dcb_status &= ~DCB_S_MVCC_TRACE;
		else if ( trace_point == 406 )
		    dcb->dcb_status &= ~DCB_S_MVCC_JTRACE;
	    }

	    dm0s_munlock(&dcb->dcb_mutex);
	}
    }



    /*
    **  DM414	- Turn CacheFlush tracing on/off
    **		  for all cache-connected servers
    */
    if ( trace_point == 414 )
    {
	DMP_LBMCB		*lbmcb;
	DM0P_BM_COMMON          *bm_common;

	if ( dmf_svcb && (lbmcb = dmf_svcb->svcb_lbmcb_ptr) &&
	     (bm_common = lbmcb->lbm_bm_common) )
	{
	    CSp_semaphore(1, &bm_common->bmcb_status_mutex.bms_semaphore);
	    if ( d->db_trswitch == DB_TR_ON )
		bm_common->bmcb_status |= BMCB_CFA_TRACE;
	    else
		bm_common->bmcb_status &= ~BMCB_CFA_TRACE;
	    CSv_semaphore(&bm_common->bmcb_status_mutex.bms_semaphore);
	}
    }

    /*
    **	DM420	- Dump Buffer Manager Statistics.
    */
    if (trace_point == 420)
    {
	dmd_buffer();
    }

    /*
    **	DM421	- Toss pages from buffer cache.
    */
    if (trace_point == 421)
    {
	dm0p_toss_pages(0, 0, scb->scb_lock_list, (i4)0, 1);
    }

    /*
    **	DM422	- Toss unmodified pages from buffer cache.
    */
    if (trace_point == 422)
    {
	dm0p_toss_pages(0, 0, scb->scb_lock_list, (i4)0, 0);
    }

    /*
    ** DM425	- Set table cache priority
    **
    ** Set table's cache priority - trace point arguments required:
    **
    **	db_vals[0]  - base table_id
    **	db_vals[1]  - index table_id (optional).
    **
    ** Make sure session has database open.
    ** Sets table's cache priority to 6 (cache priority range is 1-8).
    **
    ** (jenjo02) - Priority may now be set when the table/index
    **		   is created or modified instead of via this
    **		   trace point.
    */

    /*
    ** DM427	- Toss single page from buffer cache.
    **
    ** Purge single page from cache
    **
    **	db_vals[0]  - base table table_id
    **	db_vals[1]  - page number
    **
    ** Only works with base tables (no way to specify index id)
    */
    if ((trace_point == 427) && (d->db_value_count == 2) &&
	(scb->scb_oq_next) && (scb->scb_oq_next->odcb_dcb_ptr))
    {
	table_id.db_tab_base = d->db_vals[0];
	table_id.db_tab_index = 0;
	page_number = d->db_vals[1];

	dm0p_1toss_page(scb->scb_oq_next->odcb_dcb_ptr->dcb_id, 
			&table_id, page_number, scb->scb_lock_list);
    }

    /*
    ** DM428	- Toss pages for single table from buffer cache.
    **
    ** Purge table's pages from cache
    **
    **	db_vals[0]  - base table_id
    **
    ** Only works with base tables - secondary index pages tossed as well.
    */
    if ((trace_point == 428) && (d->db_value_count > 0) &&
	(scb->scb_oq_next) && (scb->scb_oq_next->odcb_dcb_ptr))
    {
	tabid = d->db_vals[0];

	/*
	** Call buffer manager to toss pages.
	*/
	dm0p_toss_pages(scb->scb_oq_next->odcb_dcb_ptr->dcb_id, 
			tabid, scb->scb_lock_list, (i4)0, 1);
    }

    /*
    ** DM615	- Print a Btree table to server trace log.
    **
    ** Print the Btree table by table ID
    **
    **    db_vals[0] - table_id
    **    db_vals[1] - table_index ID (optional, defaults to 0)
    */
    if ((trace_point == 615) && (d->db_value_count > 0))
    {
	if (d->db_value_count == 1)
	{
	    /* only gave base table id */
	    table_id.db_tab_base = d->db_vals[0];
	    table_id.db_tab_index = 0;
	}
	else
	{
	    /* gave both base table id and index id */
	    table_id.db_tab_base = d->db_vals[0];
	    table_id.db_tab_index = d->db_vals[1];
	}
	print_btree_table(scb, &table_id);
    }

    /*
    ** DM713	- Toss table TCB from server TCB cache.
    **
    ** Purge TCB.
    **
    **	db_vals[0]  - base table_id
    **
    ** Only works with base tables - secondary index TCBs tossed as well.
    ** Also bumps cache lock to invalidate TCB's in other caches.
    */
    if ((trace_point == 713) && (d->db_value_count > 0) &&
	(scb->scb_oq_next) && (scb->scb_oq_next->odcb_dcb_ptr))
    {
	i4 lock_id, log_id;

	table_id.db_tab_base = d->db_vals[0];
	table_id.db_tab_index = 0;

	dcb = scb->scb_oq_next->odcb_dcb_ptr;
	xcb = scb->scb_x_next;
	if (xcb != (DML_XCB *) &scb->scb_x_next)
	{
	    lock_id = xcb->xcb_lk_id;
	    log_id = xcb->xcb_log_id;
	}
	else
	{
	    /* If no transaction, use session list and no log id. */
	    lock_id = scb->scb_lock_list;
	    log_id = 0;
	}
	(VOID) dm2t_purge_tcb(dcb, &table_id, DM2T_PURGE, log_id,
	    lock_id, NULL, DM2D_IX, &local_dberr);
    }

    /*
    **	DM714	    - Set svcb_page_size
    **
    ** Sets svcb_page_size to the specified value, if it's valid.
    **
    **    db_vals[0] - page size.
    */
    if ((trace_point == 714) && (d->db_value_count == 1))
    {
	if (d->db_vals[0] == 2048  || d->db_vals[0] == 4096  ||
	    d->db_vals[0] == 8192  || d->db_vals[0] == 16384 ||
	    d->db_vals[0] == 32768 || d->db_vals[0] == 65536)
	{
	    if (dm0p_has_buffers(d->db_vals[0]))
	    {
		svcb->svcb_page_size = d->db_vals[0];
		svcb->svcb_etab_tmpsz = d->db_vals[0];
		svcb->svcb_etab_pgsize = d->db_vals[0];
	    }
	}
    }

    /*
    **	DM721	    - Set svcb_page_type
    **
    ** Sets svcb_page_type to the specified value, if it's valid.
    **
    **    db_vals[0] - page type.
    */
    if ((trace_point == 721) && (d->db_value_count == 1))
    {
	if (DM_VALID_PAGE_TYPE(d->db_vals[0]))
	    svcb->svcb_page_type = d->db_vals[0];
    }

    /*
    **	DM1002	    - Set svcb_pind_bsize
    **
    ** Sets svcb_pind_bsize to the specified value, if it's valid.
    **
    **    db_vals[0] - no of rows in exchange buffer.
    */
    if ((trace_point == 1002) && (d->db_value_count == 1))
    {
	if (d->db_vals[0] >= 1)
	  svcb->svcb_pind_bsize = d->db_vals[0];
    }

    /*
    **	DM1003	    - Set svcb_pind_nbuffers 
    **
    ** Sets svcb_pind_nbuffers to the specified value, if it's valid.
    **
    **    db_vals[0] - no of exchange buffers.
    */
    if ((trace_point == 1003) && (d->db_value_count == 1))
    {
	if (d->db_vals[0] >= 1)
	  svcb->svcb_pind_nbuffers = d->db_vals[0];
    }

    /*
    **	DM1305	- Force Consistency Point.
    */
    if (trace_point == 1305)
    {
	item = 1;
	(VOID) LGalter(LG_A_CPNEEDED, (PTR)&item, sizeof(item), &sys_err);
    }

    /*
    **	DM1314	- Force Archive Cycle.
    */
    if (trace_point == 1314)
    {
	(VOID) LGalter(LG_A_ARCHIVE, (PTR)0, 0, &sys_err);
    }

    /*
    **	DM1434	- Force the Log
    */
    if (trace_point == 1434)
    {
	status = OK;

	if (dmf_svcb && dmf_svcb->svcb_lctx_ptr &&
	    dmf_svcb->svcb_lctx_ptr->lctx_lxid != 0)
	{
	    status = LGforce(LG_LAST, dmf_svcb->svcb_lctx_ptr->lctx_lxid,
				 (LG_LSN *)0, &nlsn, &sys_err);
	}
	else if (dmf_svcb && dmf_svcb->svcb_lbmcb_ptr &&
		dmf_svcb->svcb_lbmcb_ptr->lbm_bm_lxid[0] != 0)
	{
	    status = LGforce(LG_LAST, dmf_svcb->svcb_lbmcb_ptr->lbm_bm_lxid[0],
				 (LG_LSN *)0, &nlsn, &sys_err);
	}
	if (status)
	    TRdisplay("Error Forcing Log file (%x).\n", status);
    }

    /*
    ** DM1435	- Mark database inconsistent
    **
    */
#ifdef xDEBUG
    if ((trace_point == 1435) && (scb->scb_oq_next) && 
	(scb->scb_oq_next->odcb_dcb_ptr))
    {
        DM0C_CNF *cnf;

	dcb = scb->scb_oq_next->odcb_dcb_ptr;

	/*
	** Open config file, mark db inconsistent, close config file. 
	*/
	status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list,
	    &cnf, &local_dberr);
	if (status != E_DB_OK)
	    return;

	cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
	cnf->cnf_dsc->dsc_inconst_time = TMsecs();
	cnf->cnf_dsc->dsc_inconst_code = DSC_TRACE_INCONS;

	status = dm0c_close(cnf, DM0C_UPDATE, &local_dberr);

	TRdisplay("Database %~t marked inconsistent via DM1435.\n",
	    sizeof(dcb->dcb_name), &dcb->dcb_name);
    }
#endif

    /*
    **	DM1442	- Turn Log optimized writes on/off
    */
    if (trace_point == 1442)
    {
	(VOID)LGalter(LG_A_OPTIMWRITE, (PTR)NULL,
			(d->db_trswitch == DB_TR_ON) ? 1 : 0,
			&sys_err);
    }

    /*
    **	DM1443	- Turn Log optimized write tracing on/off
    */
    if (trace_point == 1443)
    {
	(VOID)LGalter(LG_A_OPTIMWRITE_TRACE, (PTR)NULL,
			(d->db_trswitch == DB_TR_ON) ? 1 : 0,
			&sys_err);
    }

    /* DM1444  - Set DB's Table ID value
    **
    **  db_vals[0]  - New setting. Can only be set forward.
    */

    if ((trace_point == 1444) && (d->db_value_count == 1) &&
        (scb->scb_oq_next) &&
        (scb->scb_oq_next->odcb_dcb_ptr))
    {
        DM0C_CNF *cnf;

        dcb = scb->scb_oq_next->odcb_dcb_ptr;

        /*
        ** Open config file, set table_id, close config file.
        */
        status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list,
            &cnf, &local_dberr);
        if (status != E_DB_OK)
            return;

        if ( d->db_vals[0] > cnf->cnf_dsc->dsc_last_table_id)
           cnf->cnf_dsc->dsc_last_table_id = d->db_vals[0];

        status = dm0c_close(cnf, DM0C_UPDATE, &local_dberr);

        TRdisplay("Database %~t Max Table ID now %d\n",
            sizeof(dcb->dcb_name), &dcb->dcb_name, 
            cnf->cnf_dsc->dsc_last_table_id);
    }

    return;
}

/*{
** Name: dmf_tst_point	- DMF test trace point.
**
** Description:
**	This routine implements the dmf test trace points.
**
**	These are trace points that cause the server to take some immediate
**	action rather than actually set a trace point. There are called by
**	issuing the command "Set trace point DM14XX" - where XX is the
**	trace point number.
**
**	Generally these are undocumented and used for testing puposes.
**
**	Trace points implemented:
**
**	    20	    - Fast commit test trace point - used in fast commit test
**		      driver to allow multi-server, multi-database fast commit
**		      tests to stay in sync with each other.
**	    40	    - Work location trace point - used to list current set of
**		      work locations being used for this session.
**
** Inputs:
**      number	    - trace point number.
**      debug_cb    - A standard debug control block.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    May vary depending on test trace number.
**
** History:
**	10-apr-1989 (rogerk)
**          Created for Terminator.
**	19-apr-1990 (rogerk)
**	    Added DMZ_TST_MACRO(19) trace point to crash logging system.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Added archiver test trace points.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (rogerk)
**	    Changed archiver test records to pass DM0L_JOURNAL flag.
**	    Changed check for xact in progress to test the non-existence
**	    of an XCB correctly.
**	04-nov-92 (jrb)
**	    Added new trace point for displaying current set of work locations
**	    being used.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Writes a journaled BT record if not already done so.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
*/
static VOID
dmf_tst_point(
DML_SCB	    *scb,
i4	    number,
DB_DEBUG_CB *debug_cb)
{
    DB_STATUS	status;
    STATUS	cl_status;
    i4	error;
    CL_ERR_DESC	sys_err;
    DB_ERROR	local_dberr;

    switch (number)
    {
      case 19:
      {
	/*
	** Logging system recovery test trace point.
	**
	** This trace point crashes the logging system.  This is used by
	** some internal tests to test system recovery.  The system is
	** brought down through an LGalter call.
	*/
	i4	    action = 0;	    /* zero action implies immediate shutdown */

	uleFormat(NULL, E_DM9395_SYSTEM_SHUTDOWN_TEST, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	cl_status = LGalter(LG_A_SHUTDOWN, (PTR)&action, sizeof(action),
			    &sys_err);

	/* Suspend for a while until the server is actually killed. */
	CSsuspend(CS_TIMEOUT_MASK, 30, 0);

	break;
      }

      case 20:
      {
	/*
	** Fast commit test trace point.
	**
	** This trace point is used to help support the multi server fast
	** commit test suite.  It requests an exclusive sync lock that
	** allows each fast commit session, regardless of server or database
	** being used to sync up with the fast commit test driver.
	**
	** The lock is keyed on the buffer manager id, so this lock will
	** be shared by all test programs connect to the same shared cache.
	*/
	LK_LOCK_KEY	lockkey;
	LK_LLID		lock_list;
	LK_LKID		lockid;

	/* scb = *DML_SCB of session */

	/* Get session lock list to make request on. */
	lock_list = (LK_LLID)scb->scb_lock_list;

	/* Request lock, return when granted */
	lockkey.lk_type = 99;
	lockkey.lk_key1 = dm0p_bmid();
	MEfill(sizeof(LK_LKID), 0, &lockid);
	MEmove(20, (PTR)"FASTCOMMIT TEST LOCK", ' ',
	    (5 * sizeof(lockkey.lk_key2)), (PTR)&lockkey.lk_key2);
	cl_status = LKrequest(
	    (LK_STATUS | LK_PHYSICAL | LK_NODEADLOCK | LK_MULTITHREAD),
	    lock_list, &lockkey, LK_X, (LK_VALUE *)NULL,
	    &lockid, (i4)0, &sys_err);
	break;
      }

      case 21:
      case 22:
      case 23:
      case 24:
      case 25:
      case 26:
      case 27:
      case 29:
      {
	    LG_LSN		lsn;
	    DML_XCB		*xcb;
	    i4		journal_flag;
	    i4		test_number;

	    /*
	    ** Logging and Journaling System test trace points.  These trace 
	    ** points cause log records to be written which will the journaling
	    ** system to perform some test action.
	    **
	    ** The session must be in an active transaction to use these flags.
	    */

	    /* scb = *DML_SCB of session */

	    /*
	    **
	    ** We will leach off of this session's current transaction
	    ** in order to write our test log records.
	    */
	    xcb = scb->scb_x_next;

	    /*
	    ** If not in a transaction, then can't execute the test.
	    */
	    if (xcb == (DML_XCB *)&scb->scb_x_next)
		break;

            /*
            ** If this is the first write operation for this transaction,
            ** then we need to write the begin transaction record.
            */
            if (xcb->xcb_flags & XCB_DELAYBT)
            {
                /* Still need to write a BT record, this could
                ** mean that a non-journaled BT record has been written
                ** but that doesn't matter here, as we want to start
                ** journaling (see below).
                */
                status = dmxe_writebt(xcb, TRUE, &local_dberr);
                if (status != E_DB_OK)
                {
                    xcb->xcb_state |= XCB_TRANABORT;
                    break;
                }
            }

	    /*
	    ** Write the test log record.
	    */
	    journal_flag = DM0L_JOURNAL;
	    switch (number)
	    {
		case 21:
		    test_number = TST101_ACP_TEST_ENABLE;
		    break;

		case 22:
		    test_number = TST102_ACP_DISKFULL;
		    break;

		case 23:
		    test_number = TST103_ACP_BAD_JNL_REC;
		    break;

		case 24:
		    test_number = TST104_ACP_ACCESS_VIOLATE;
		    break;

		case 25:
		    test_number = TST105_ACP_COMPLETE_WORK;
		    break;

		case 26:
		    test_number = TST106_ACP_BACKUP_ERROR;
		    break;

		case 27:
		    test_number = TST107_ACP_MEMORY_FAILURE;
		    break;

		case 29:
		    test_number = TST109_ACP_TEST_DISABLE;
		    break;

		default:
		    journal_flag = 0;
		    test_number = 0;
	    }

	    status = dm0l_test(xcb->xcb_log_id, journal_flag, test_number, 
		    0, 0, 0, 0, (char *)0, 0, (char *)0, 0, &lsn, &local_dberr);
	    break;
      }

      case 36:
      case 37:
      case 38:
      case 39:
      case 41:
      {
	    status =
		dmf_perftst(number, debug_cb->db_vals[0], debug_cb->db_vals[1]);
	    break;
      }

    case 40:
      {
	/* This trace point is intended as a temporary measure.  It will list
	** all work locations which the issuing session is currently using for
	** sorting or whatever else work locations are used for.  Obviously a
	** trace point isn't the most friendly way to return this information,
	** but there isn't any other easy way to do it at this time.  Once the
	** config file is turned into a table, or at least a gateway to the
	** config file which looks like a table is constructed, then this trace
	** point will be superseded.
	*/
	DML_LOC_MASKS	*lm;
	DMP_LOC_ENTRY	*locs;
	i4		next_bit = -1;
	i4		i;

	/* scb = *DML_SCB of session */

	/* get location masks from scb and get extent array from dcb.
	** (Note: we assume only one odcb connected to the scb; although
	** this assumption may not be correct, it is unclear what should
	** be done to access the extent array without this assumption.)
	*/

	lm = scb->scb_loc_masks;
	locs = scb->scb_oq_next->odcb_dcb_ptr->dcb_ext->ext_entry;
	TRdisplay("\n=== List of Current Work Locations for this Session ===");
	TRdisplay("\n| ");

	for (i=0;;i++)
	{
	    /* get bits from location mask until end (rtns -1 at end) */
	    next_bit = BTnext(next_bit, (char *) lm->locm_w_use, lm->locm_bits);
	    if (next_bit == -1)
		break;
		
	    if (i % (132 / (sizeof(DB_LOC_NAME)+10)) == 0)
		TRdisplay("\n| ");
		
	    TRdisplay("%t | ", sizeof(DB_LOC_NAME),
				    locs[next_bit].logical.db_loc_name);
	}
	TRdisplay("\n|\n=== End of List ===\n");
	break;
      }

      default:
	break;
    }

    return;
}

VOID
dmf_scc_trace(
char               *msg_buffer)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)STlength(msg_buffer);
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d while attempting to send info from DMF\n",
	    scf_cb.scf_error.err_code);
    }
}

/*
** Name: print_btree_table	- Print the indicated Btree table
**
** Description:
**	This routine is called by trace point DM615 to print the contents of
**	a Btree table to the DBMS server trace log.
**
**	It opens the table and calls dmd_prall to do the work.
**
**	Note that we may not be in a transaction. For example, if this trace
**	point is issued from QUEL, we are typically NOT in a transaction at
**	this point. In this case, we must begin a transaction, and remember
**	that we did so, so that we can end it before returning.
**
** Inputs:
**	scb			- This session's session control block
**	tableid			- Table ID to dump
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (bryanp)
**	    Created as part of the Btree index compression project.
*/
static VOID
print_btree_table(
DML_SCB	    *scb,
DB_TAB_ID   *tableid)
{
    DB_STATUS		status;
    DMX_CB		dmx_cb;
    DML_XCB		*xcb;
    DMP_RCB		*rcb;
    i4		error;
    i4		db_lockmode;
    DB_TAB_TIMESTAMP	table_timestamp;
    bool		transaction_started_by_us = FALSE;
    DB_ERROR		local_dberr;

    for(;;)
    {
	/*
	** If database is not open, nothing to do
	*/
	if (scb->scb_oq_next == 0)
	    break;

	/*
	** Make sure session has database open and transation begun. If no
	** transaction has been started yet, start one.
	**
	** Open table and get an RCB to pass to the print function.
	** If an error occurs opening or closing the table, simply don't bother
	** to trace it -- it was probably a bogus table id.
	*/
	xcb = scb->scb_x_next;
	if (xcb == (DML_XCB*) &scb->scb_x_next)
	{
	    /* initialize dmx */
	    dmx_cb.type	= DMX_TRANSACTION_CB;
	    dmx_cb.length = sizeof(dmx_cb);
	    dmx_cb.dmx_flags_mask = DMX_READ_ONLY;    
	    dmx_cb.dmx_session_id = (PTR)scb->scb_sid;
	    dmx_cb.dmx_db_id = (PTR)scb->scb_oq_next;
	    dmx_cb.dmx_option = 0;

	    /* make the call and process status */
	    status = dmx_begin(&dmx_cb);

	    if (status != E_DB_OK)
		break;

	    transaction_started_by_us = TRUE;
	    xcb = scb->scb_x_next;
	}

	if ((scb->scb_oq_next) && (scb->scb_oq_next->odcb_dcb_ptr))
	{
	    db_lockmode = DM2T_S;
	    if (scb->scb_oq_next->odcb_lk_mode == ODCB_X)
		db_lockmode = DM2T_X;

	    status = dm2t_open(scb->scb_oq_next->odcb_dcb_ptr, tableid,
			    DM2T_X, DM2T_UDIRECT, DM2T_A_READ,
			    (i4)0 /*timeout*/, (i4)20 /*maxlocks*/,
			    xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id,
			    (i4)0 /* sequence */,
			    (i4)0 /* lock_duration */, db_lockmode,
			    &xcb->xcb_tran_id, &table_timestamp,
			    &rcb, (DML_SCB *)0, &local_dberr);
	    if (status)
		break;

	    if (rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE)
		dmd_prall(rcb);

	    status = dm2t_close(rcb, (i4)0, &local_dberr);
	    if (status)
		break;
	}
	break;
    }

    if (transaction_started_by_us)
    {
	/*
	** The dmx_cb is all set up from the DMX_BEGIN. Just commit this
	** read-only transaction.
	*/
	status = dmx_commit(&dmx_cb);
	if (status != E_DB_OK)
	    return;
    }

    return;
}
