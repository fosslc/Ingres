/*
**Copyright (c) 2004 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>

#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <lgkparms.h>
#include    <lgdstat.h>

#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>

#include    <dm.h>
 
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
 
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
**  Name: DMFPRFTST.C - performance tests to be run in the server.
**
**  Description:
**	Performance tests to be executed in the server.  These tests are 
**	triggered by "set trace point dm14XX" and are immediately executed.
**
**	external procedures:
**	    dmf_perftst() - external procedure called from dmftrace.c
**
**	internal procedure:
**	    dmftst_lg_call() - dm1431: time "iteration" calls to LGshow().
**
**  History:
**      26-jul-1993 (mikem)
**	    Created.
**	27-jul-1993 (mikem)
**	   PCpid() returns void, so don't look for status.
**      11-aug-93 (ed)
**          added missing includes
**	23-aug-93 (mikem)
**	   Call LGreserve() in dmftst_lgwrite_call() to reserve space to mimic 
**	   actual calls made by server when doing LGwrite().  Minor changes to
**	   some of the debugging TRdisplay()'s.
**	20-sep-93 (jnash)
**	   Add flag param to LGreserve() call.
**	31-jan-94 (mikem)
**	   bug #58506
**	   Fix unitialized status errors found by running lint.  
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Get session's *DML_SCB with macro intead of SCU_INFORMATION.
**	18-jul-2002 (somsa01)
**	    Enabled dmftst_ii_nap() for Windows.
**	21-Aug-2002 (devjo01)
**	    Use CSswitch rather than II_nap in support of DM1438.
**	    This will allow us to test context switch overhead on all
**	    platforms, and will resolve build failure introduced
**	    by inclusion of iinap.h header file.  II_nap was a
**	    poor choice for measuring context switching performance,
**	    since the effect of a sleep time will overshadow any
**	    reasonable context switch time.   Renamed dmftst_ii_nap
**	    to dmftst_context_switch to reflect this.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**/


/*
**  Forward and/or External function references.
*/
static i4 dmftst_lg_call(
    i4            	iterations);

static i4 dmftst_lgwrite_call(
    i4         	iterations,
    i4		buf_size);

static i4 dmftst_context_switch(
    i4            	iterations);

static i4 dmftst_syscall(
    i4            	iterations);

static i4 dmftst_cspsem(
    i4            	iterations);

/*{
** Name: dmf_perftst()	- support for performance test trace points.
**
** Description:
**
**	Routines called when the following trace points are executed.
**          - DM1436 to test performance of LG calls.
**          - DM1437 to test performance of LGwrite() calls.
**          - DM1438 to test performance of context switches.
**          - DM1439 to test performance of system calls from server.
**          - DM1441 to test performance of cross process sem calls.
**
** Inputs:
**	tracepoint_num			tracepoint number
**	iterations			number of loops to execute.
**	arg				argument interpreted by test.
**
** Outputs:
**	none.
**
**	Returns:
**	    E_DB_OK
**
** History:
**      26-jul-93 (mikem)
**          Created.
*/
i4
dmf_perftst(
i4		tracepoint_num,
i4		iterations,
i4		arg)
{
    i4	status = FAIL;

    switch (tracepoint_num)
    {
	case 36:
	{
	    /* lg throughput test */
	    status = dmftst_lg_call(iterations);
	    break;
	}
	case 37:
	{
	    /* lg throughput test */
	    status = dmftst_lgwrite_call(iterations, arg);
	    break;
	}
	case 38:
	{
	    /* lg throughput test */
	    status = dmftst_context_switch(iterations);
	    break;
	}
	case 39:
	{
	    /* lg throughput test */
	    status = dmftst_syscall(iterations);
	    break;
	}
	case 41:
	{
	    status = dmftst_cspsem(iterations);
	}

	default:
	    status = FAIL;
	    break;
    }

    return(status);
}

/*{
** Name: dmftst_lg_call()	- Determine performance of LG call.
**
** Description:
**	Determine the throughput limitation of making an LG call.  This is done
**	by measuring the cpu cost of making repeated calls to LGshow with 
**	one of it's most simple options (LG_A_HEADER).  This should give a
**	measurement of the best case time through the LG system.
**
** Inputs:
**	iterations			number of LG calls.
**
** Outputs:
**      none.
**
**	Returns:
**	    E_DB_OK
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	31-jan-94 (mikem)
**	   bug #58506
**	   Initialize status in all cases (lint found error).
*/
static i4
dmftst_lg_call(
i4            iterations)
{
    CL_ERR_DESC	sys_err;
    i4	length;
    LG_HEADER	head;
    i4	i;
    i4	status = OK;

    
    TRdisplay("Calling LGshow() %d times\n", iterations);
    for (i = 0; i < iterations; i++)
    {
        status = 
	    LGshow(LG_A_HEADER, (PTR) &head, sizeof(head), &length, &sys_err);
    }

    return(status);
}

/*{
** Name: dmftst_lgwrite_call()	- Determine performance of LGwrite call.
**
** Description:
**	Determine the throughput limitation of making an LG call.  This is done
**	by measuring the cpu cost of making repeated calls to LGshow with 
**	one of it's most simple options (LG_A_HEADER).  This should give a
**	measurement of the best case time through the LG system.
**
** Inputs:
**	iterations			number of loops.
**	buf_size			size of LGwrite to do.
**
** Outputs:
**	none.
**
**	Returns:
**	    E_DB_OK
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	23-aug-93 (mikem)
**	   Call LGreserve() to reserve space to mimic actual calls made by 
**	   server when doing LGwrite().
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**         Only need to write a non-jnl'd BT record if the TX hasn't already
**         been started.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: LG_LRI is output from LGwrite, no longer LG_LSN.
*/
static i4
dmftst_lgwrite_call(
i4         iterations,
i4		buf_size)
{
    CL_ERR_DESC	sys_err;
    i4	i;
    i4	status;
    LG_OBJECT	lg_object[2];
    LG_LRI	lri;
    DM0L_TEST	log;
    i4	log_data_size;
    LG_LXID	lx_id;
    DML_XCB     *xcb;
    DMX_CB	dmx_cb;
    bool	transaction_started_by_us = FALSE;
    DML_SCB	*scb;
    CS_SID	sid;
    i4	error;
    DB_ERROR	local_dberr;

    for (;;)
    {
	/* break on errors */

	if ((buf_size < 0) || (buf_size > 2048))
	{
	    status = FAIL;
	    break;
	}

	/*
	** Logging and Journaling System test trace points.  These trace 
	** points cause log records to be written which will the journaling
	** system to perform some test action.
	**
	** The session must be in an active transaction to use these flags.
	*/

	/*
	** Find session control block.
	**
	** We will leach off of the session's current transaction
	** in order to write our test log records.
	*/
	CSget_sid(&sid);
	scb = GET_DML_SCB(sid);

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
	    status = dmxe_writebt(xcb, FALSE, &local_dberr);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
            }
	}

	log.tst_header.length = ((char *)&log.tst_stuff6[0]) - ((char *)&log);
	log.tst_header.type = DM0LTEST;
	log.tst_header.flags = 0;
	log.tst_number = 0;
	log.tst_stuff1 = 1;
	log.tst_stuff2 = 2;
	log.tst_stuff3 = 3;
	log.tst_stuff4 = 4;
	
	lx_id = xcb->xcb_log_id;


	/* always make data portion at least one byte, even if the log header is
	** bigger than the test log size.
	*/

	log_data_size = max(1, (buf_size - log.tst_header.length));

	lg_object[0].lgo_size = log.tst_header.length;
	lg_object[0].lgo_data = (PTR) &log;
	lg_object[1].lgo_size = log_data_size; 
	lg_object[1].lgo_data = (PTR) &log.tst_stuff6[0];

	TRdisplay("Calling LGwrite() %d times with %d byte buffers\n", 
		iterations, lg_object[0].lgo_size + lg_object[1].lgo_size);

	for (i = 0; i < iterations; i++)
	{
	    if ((status = LGreserve(0, lx_id, 1, 
			    log_data_size + log.tst_header.length, &sys_err)))
		break;

	    if ((status = LGwrite(0, lx_id, (i4)2, lg_object, &lri, &sys_err)))
		break;
	}

	if (status)
	    TRdisplay("LGwrite() failed after the %d iteration - status %d\n", 
			i, status);

	if (transaction_started_by_us)
	{
	    /* The dmx_cb is all set up from the DMX_BEGIN. Just commit.  */
	    status = dmx_commit(&dmx_cb);
	}

	break;
    }

    return(status);
}

/*{
** Name: dmftst_context_switch()	- Determine context switch performance.
**
** Description:
**	Try to determine the cost of a unix context switch.  This is done
**	by making a number of CSswitch() calls and averaging over the calls.
**	To make this measurement more reasonable one might set the server's
**	priority much higher than all other processes on the machine and
**	also have enough low priority cpu hogging processes to eat all 
**	available cpu time on the machine.
**
**	devjo01 note:  This seems a pretty lame test.  "performance"
**	measured will be highly dependent on what other execution threads
**	are present.  It probably would have been better to measure this
**	with a stand-alone program similar to csphil.  Plus unless
**	'iterations' is very high, TRdisplay calls will skew the results.
**
** Inputs:
**      iterations                      number of loops.
**
** Outputs:
**	none.
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	21-Aug-2002 (devjo01)
**	   Changed name, and replaced II_nap with CSswitch.  Killed 'usec'
**	   parameter.
*/
static i4
dmftst_context_switch( i4 iterations )
{
    i4	i;

    TRdisplay("Calling CSswitch() %d times\n", iterations);

    for (i = 0; i < iterations; i++)
    {
        CSswitch();
    }

    return(OK);
}

/*{
** Name: dmftst_syscall()	- Determine performance of a "cheap" syscall.
**
** Description:
**	Try to determine the cost of a OS system call.This is done
**	by making a number of PCpid() calls and averaging over the calls.
**
** Inputs:
**      iterations			number of loops.
**
** Outputs:
**	none.
**
**	Returns:
**	    E_DB_OK
**
** History:
**      23-jul-93 (mikem)
**         Created.
**	27-jul-93 (mikem)
**	   PCpid() returns void, so don't look for status.
**	31-jan-94 (mikem)
**	   bug #58506
**	   Always return OK.  Uninitialized status was returned.  PCpid always
**	   succeeds so no failure condition.
*/
static i4
dmftst_syscall(
i4            iterations)
{
    i4	i;
    i4	status = OK;
    PID		pid;

    
    TRdisplay("Calling PCpid() %d times\n", iterations);
    for (i = 0; i < iterations; i++)
    {
        PCpid(&pid);
    }

    return(OK);
}

/*{
** Name: dmftst_cspsem()	- Determine performance of cross process sem.
**
** Description:
**	Measure cost of CSp_semaphore()/CSv_semaphore() calls where there is
**	priority much higher than all other processes on the machine and
**
** Inputs:
**	iterations			number of iterations
**
** Outputs:
**	none.
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	31-jan-94 (mikem)
**	   bug #58506
**	   Always initialize status.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
*/
static i4
dmftst_cspsem(
i4            iterations)
{
    i4		i;
    i4		status = OK;
    CS_SEMAPHORE	sem;

    CSw_semaphore(&sem, CS_SEM_MULTI, "dmftst_cspsem" );
    for (i = 0; i < iterations; i++)
    {
        if (status = CSp_semaphore(TRUE, &sem))
	    break;
        if (status = CSv_semaphore(&sem))
	    break;
    }
    TRdisplay("Called CSp_semaphore()/CSv_semaphore %d times\n", i);
    CSr_semaphore( &sem );
    return(status);
}
