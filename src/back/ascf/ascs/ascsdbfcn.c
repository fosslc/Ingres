/*
**Copyright (c) 2004 Ingres Corporation
NO_OPTIM=dr6_us5 dgi_us5 i64_aix
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <ci.h>
#include    <si.h>
#include    <sl.h>
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
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sca.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>

#include    <wtsinit.h>
#include    <asct.h>


/**
**
**  Name: scsdbfcn.c - Database manipulation functions used by scs
**
**  Description:
**      This file contains the routines used by the sequencer to perform 
**      direct database manipulations.
**
**          ascs_dbopen - open a database
**          ascs_dbclose - close a database
**	    ascs_dbms_task - call specialized dbms task on behalf of session
**	    ascs_dbadduser - inrement no of database users in dbcb
**	    ascs_dbdeluser - decrement no of users in dbcb
**	    ascs_atfactot  - initiate a factotum thread
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
**          Cast length passed to sc0m_allocate to longnat.
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
**      08-Feb-1999 (fanra01)
**          Renamed scs_dbms_task and scs_check_license to ascs versions.
**          Removed unused functions and removed unused cases from 
**          ascs_dbms_task.
**      04-Mar-1999 (fanra01)
**          Add ice tracing thread to flush memory buffers to disk.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	12-Nov-2001 (devjo01)
**	    Change check period in ascs_check_license to 1 / day.
**	28-Nov-2001 (kinte01)
**	    Add include of si.h
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      16-Mar-2010 (frima01) SIR 121619
**          Removed define for IIDBDB_ID, now located in dmf.h.
[@history_template@]...
**/

/*
** Static declarations
*/
static DB_STATUS ascs_periodic_clean( SCD_SCB *scb );
static DB_STATUS ascs_tracelog( SCD_SCB *scb );


/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB *Sc_main_cb;      /* core structure of scf */
GLOBALREF DU_DATABASE dbdb_dbtuple;    /* database tuple for iidbdb */
GLOBALREF SCS_IOMASTER  IOmaster;      /* database list etc.    */


/*{
** Name: ascs_dbms_task	- process specialized server thread task.
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
**	    SCS_SSAMPLER	: Sampler Thread
**		    Call scs_sampler.  This thread can be shut down by
**		    the monitor.
**	    SCS_SFACTOTUM	: Factotum Thread.
**		    Call the function specified by the thread creator.
**	    SCS_SLICENSE 	: License Thread.
**		    Call scs_check_license.  This thread should not return
**                  unless the server is being shut down. 
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
**      08-Feb-1999 (fanra01)
**          Renamed to ascs version and removed unneeded cases.
**      04-Mar-1999 (fanra01)
**          Add ice tracing thread to flush memory buffers to disk.
[@history_template@]...
*/
DB_STATUS
ascs_dbms_task(SCD_SCB *scb )
{
    DB_STATUS	    status;

    /*
    ** Call the specific routine to handle the task.
    */
    switch (scb->scb_sscb.sscb_stype)
    {
      case SCS_SEVENT:
	status = sce_thread();
	break;

      case SCS_PERIODIC_CLEAN:
        status = ascs_periodic_clean( scb );
        break;

      case SCS_TRACE_LOG:
        status = ascs_tracelog( scb );
        break;

      default:
	sc0e_put(E_SC0239_DBMS_TASK_ERROR, 0, 1,
		 sizeof(scb->scb_sscb.sscb_stype),
		 (PTR)&scb->scb_sscb.sscb_stype,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: ascs_periodic_clean - check ICE cleanup
**
** Description:
**      The periodic cleanup thread checks the sessions in the ice server
**      and disconnects timedout ones.
**
** Inputs:
**      scb                     session control block
**
** Outputs:
**      None.
**
** Returns:
**      DB_STATUS
**
** History:
**      04-Feb-1999 (fanra01)
**          Created.
*/
static DB_STATUS
ascs_periodic_clean( SCD_SCB *scb )
{
    DB_STATUS   status = E_DB_OK;
    STATUS	cl_status;

    for (;;)
    {

	MEcopy( "Waiting for next check time", 27,
	        scb->scb_sscb.sscb_ics.ics_act1 );
	scb->scb_sscb.sscb_ics.ics_l_act1 = 27;

	/* 
	** license API states license must be checked at least 
	** 60 times per minute and at most 240 times per minute
	*/
	cl_status = CSsuspend(CS_INTERRUPT_MASK|CS_TIMEOUT_MASK, 
			30, NULL);

	if ( cl_status == E_CS0008_INTERRUPTED )
	    break;

	/* expected results are timeout and interrupt only */
	if ( cl_status != E_CS0009_TIMEOUT )
	{
	    status = cl_status;
	    break;
	}

	/* call the check, but don't exit the server on violation */
        if (Sc_main_cb->sc_capabilities & SC_ICE_SERVER)
        {
            DB_ERROR dberr;

            status = WTSCleanup(&dberr);
            if (status != E_DB_OK)
            {
                sc0e_0_put(dberr.err_code, 0);
            }
        }
    }

    if (status != E_DB_OK)
	sc0e_0_put(E_SC0385_CLEANUP_THREAD_EXIT, 0);

    return (status);

}

/*
** Name: ascs_tracelog - trace log writer
**
** Description:
**      Periodically flushes the trace buffers to disk.
**
**
** Inputs:
**      scb                     session control block
**
** Outputs:
**      None.
**
** Returns:
**      DB_STATUS
**
** History:
**      04-Feb-1999 (fanra01)
**          Created.
*/
static DB_STATUS
ascs_tracelog( SCD_SCB *scb )
{
    DB_STATUS   status = E_DB_OK;
    STATUS	cl_status;

    asct_session_set (scb->cs_scb.cs_self);

    asct_trace (ASCT_LOG)(ASCT_TRACE_PARAMS, "Trace started ....");

    for (;;)
    {

	MEcopy( "Waiting for next check time", 27,
	        scb->scb_sscb.sscb_ics.ics_act1 );
	scb->scb_sscb.sscb_ics.ics_l_act1 = 27;

	/* 
	*/
	cl_status = CSsuspend(CS_INTERRUPT_MASK|CS_TIMEOUT_MASK, 
			asctlogtimeout, NULL);

	if ( cl_status == E_CS0008_INTERRUPTED )
	    break;

	/* expected results are ok, timeout and interrupt only */
        /* ok from resume on thread */
	if ( cl_status != OK && cl_status != E_CS0009_TIMEOUT )
	{
	    status = cl_status;
	    break;
	}

	/* call the check, but don't exit the server on violation */
        if (Sc_main_cb->sc_capabilities & SC_ICE_SERVER)
        {
            DB_ERROR dberr;

            status = asct_flush(&dberr, ASCT_POLLED_FLUSH);
            if (status != E_DB_OK)
            {
                sc0e_0_put(dberr.err_code, 0);
            }
        }
    }

    asct_terminate ();

    if (status != E_DB_OK)
	sc0e_0_put(E_SC0387_LOGTRC_THREAD_EXIT, 0);

    return (status);

}
