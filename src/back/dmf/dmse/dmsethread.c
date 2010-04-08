/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <sr.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <dmccb.h>
#include    <adf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dmst.h>
#include    <dmftrace.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmd.h>

/**
**
**  Name: DMSETHREAD.C - Parallel Sort Child Thread Driver
**
**  Description:
**      This file contains the functions which drive the execution of 
**	the child thread code for parallel sorts.
**
**          dmse_child_thread - manage contents of thread exchange buffer
**
**
**  History:
**      19-feb-98 (inkdo01)
**	    Written (for parallel sort threads project).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Aug-2002 (hanal04) Bug 108509 INGSRV 1860
**          Prevent the srt_sem mutex being removed until child
**          threads are finished using it. This prevents CSMTp_semaphore()
**          errors.
**	04-Apr-2003 (jenjo02)
**	    Refined parent/child sync and mutex protection while
**	    testing/changing status bits. Added child_suspend()
**	    static function culled from duplicated code.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization.
**	28-Jan-2006 (kschendel)
**	    Fix sync problems (segv's, condition waits) caused by glitches
**	    in flag handling, and a mistaken assumption that cbix/pbix
**	    are mutex protected (they aren't).
**	22-aug-07 (hayke02 & kibro01)
**	    Add memory barrier/fence calls (CS_membar()) before ste_cbix
**	    is incremented. This prevents the increment taking place before
**	    other instructions are executed. This change fixes bug 118591. 
**	23-aug-07 (kibro01)
**	    Change CS_membar to CS_MEMBAR_MACRO
**	25-Nov-2008 (jonj)
**	    SIR 120874: dmse_? functions converted to DB_ERROR *
*/
 
/*
**  Definition of static variables and forward static functions.
*/
static STATUS dmse_handler(EX_ARGS *ex_args);

static VOID
dmse_child_put(DM_STECB *step,
	       DB_ERROR	*dberr);

static VOID
dmse_child_get(DM_STECB *step,
	       DB_ERROR	*dberr);

static VOID
child_suspend(DM_STECB  *step,
	       DB_ERROR	*dberr);

static VOID
child_presume(DM_STECB *step, bool mutex_here);

static VOID
dmse_tdiff(DM_STECB *step,
	float	*total);


/** Name: dmse_child_thread - coordinate execution of child sort threads.
**
** Description:
**      This function coordinates the child processing in a parallel sort.
**	It calls dmse_begin to build a sort control block for this portion 
**	of the sort, then loops over the exchange buffer processing requests
**	as they arrive. It returns back directly to the SCF/CS code which
**	allocated the thread.
**
** Inputs:
	ftxcb			Pointer to thread execution block
**		.ftx_data	Pointer to thread's DM_STECB. Contains parent/
**				child sort cbs, thread ids, flags, exchange
**				buffer addr, etc. In short, everything needed
**				to coordinate parent/child processing.
**
** Outputs:
**	none
**
** Returns:
**	E_DB_OK
**	E_DB_FATAL
**
** History:
**	19-feb-98 (inkdo01)
**	    Written (for parallel sort threads project).
**	21-jul-98 (inkdo01)
**	    Changes to wrkloc processing (vector is defnitely longer than the
**	    32 bits originally assumed).
**	24-dec-98 (inkdo01)
**	    Use semaphore to wait 'til parent thread has init'ed all children.
**	20-mar-2002 (thaju02)
**	    Do not report E_DM9715, for sorts terminated by dmse_end(). 
**	    (s105576)
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin() prototype.
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization.
**	    Added EXdeclare() in case the unexpected
**	    happens.
**	30-Apr-2004 (jenjo02)
**	    Plug end-of-sort race condition whereby parent
**	    may immediately be dispatched and free its
**	    "psort" and our "step" memory before we call
**	    dmse_end_serial.
**	18-Apr-2005 (jenjo02)
**	    Added srt_c_count to dmse_begin_serial() prototype.
**	24-Jun-2009 (hweho01)
**          The length of ste_ctid would be 4 or 8, depends on the build
**          mode; use %p to handle the differences in trace display. 
*/
DB_STATUS
dmse_child_thread(
SCF_FTX		*ftxcb)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_STECB	*step = (DM_STECB *)ftxcb->ftx_data;
    DMS_SRT_CB	*psort = step->ste_pcb;	/* parent thread sort cb */
    DMS_SRT_CB	*s = (DMS_SRT_CB*)NULL;
    DMS_SRT_CB	*temp_s;

    i4		flags;
    SYSTIME	systime;
    DB_STATUS	status;
    i4		err_code;
    PTR		rowp;
    i4		i;
    EX_CONTEXT	context;
    DB_ERROR	local_dberr;

    CLRDBERR(&local_dberr);

    psort = step->ste_pcb;		/* parent thread sort cb */

    if ( EXdeclare(dmse_handler, &context) )
    {
	status = E_DB_ERROR;
	SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);

	CSp_semaphore(1, &psort->srt_cond_sem);

	/*
	** If we crashed before dmse_begin_serial
	** completed, we haven't done this...
	*/
	if ( s == (DMS_SRT_CB*)NULL )
	{
	    psort->srt_threads++;
	    psort->srt_init_active--;
	}
    }
    else
    {
	if (DMZ_SRT_MACRO(1))
	{
	    TMnow(&systime);		/* init timing info */
	    step->ste_secs = systime.TM_secs;
	    step->ste_msecs = systime.TM_msecs; 
	}

	step->ste_cbix = 0;			/* init for PUT phase */
	step->ste_exec = 0.0;
	step->ste_wait = 0.0;

	/* Identify ourselves to the Parent */
	CSget_sid(&step->ste_ctid);

	/* Initialize our exchange buffer: */

	/* Start of exchange buffer array */
	step->ste_xbufp = (char**)&step->ste_misc[1];

	/* 1st row buffer right after exchange buff array */
	rowp = (PTR)&step->ste_xbufp[step->ste_bsize];

	/* Init exchange buffer entries */
	for ( i = 0; i < step->ste_bsize; i++ )
	{
	    step->ste_xbufp[i] = rowp;
	    rowp += psort->srt_iwidth;
	}


	flags = 0;
	if (psort->srt_state & SRT_ELIMINATE) 
	    flags |= DMSE_ELIMINATE_DUPS;
	if (psort->srt_state & SRT_CHECK_KEY) 
	    flags |= DMSE_CHECK_KEYS;

	status = dmse_begin_serial(flags | DMSE_CHILD, 
			    &psort->srt_table_id,
			    psort->srt_atts, 
			    psort->srt_a_count, 
			    psort->srt_k_count,
			    psort->srt_c_count,
			    psort->srt_array_loc, 
			    step->ste_wrkloc, 
			    psort->srt_mask_size,
			    &psort->srt_spill_index, 
			    psort->srt_width,
			    step->ste_records,
			    psort->srt_uiptr,
			    psort->srt_lk_id, 
			    psort->srt_log_id,
			    &step->ste_ccb,
			    psort->srt_adfcb.adf_collation,
			    psort->srt_adfcb.adf_ucollation,
			    &local_dberr);

	/* Signal Parent that we're up and running */
	CSp_semaphore(1, &psort->srt_cond_sem);
	psort->srt_threads++;
	psort->srt_init_active--;
	if (psort->srt_pstate & SRT_PWAIT)
	    CScnd_signal(&psort->srt_cond, psort->srt_sid);

	if ( status == E_DB_OK )
	{
	    s = step->ste_ccb;

	    /* Initialization is now complete. Now progress through the stages
	    ** of the child sort. dmse_child_put receives all rows to be sorted
	    ** from the parent and loads them into this thread's sort. 
	    ** dmse_child_input_end passes on the input_end call for the child sort. 
	    ** dmse_child_get passes all sorted rows back to the parent through 
	    ** the exchange buffer (reversing the flow of dmse_child_put). 
	    ** dmse_child_end closes down the child thread structures.
	    */

	    CSv_semaphore(&psort->srt_cond_sem);

	    /* Do the PUTs, end_input */
	    dmse_child_put(step, &local_dberr);

	    /* Reset for GETs */
	    step->ste_cbix = 0;
	    step->ste_prstrt = step->ste_bsize * 3 / 4;

	    /* Do the GETs */
	    dmse_child_get(step, &local_dberr);

	    CSp_semaphore(1, &psort->srt_cond_sem);
	}
    }


    if ( status )
    {
	psort->srt_pstate |= SRT_CERR;

	if ( psort->srt_pstatus == E_DB_OK )
	{
	    psort->srt_pstatus = status;
	    psort->srt_pdberr = local_dberr;
	}

	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
	uleFormat(NULL, E_DM9714_PSORT_CFAIL, 0, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
    }
    else if (DMZ_SRT_MACRO(1))
    {
	TRdisplay("%p DMSE_CHILD: Gets = %d, Puts = %d, Rows = %d, Cwaits = %d, Pwaits = %d\n",
	    step->ste_ctid,
	    step->ste_c_gets, step->ste_c_puts, s->srt_c_records,
	    step->ste_ncwaits, step->ste_npwaits);
	TRdisplay("          Cresumes = %d, Presumes = %d\n",
	    step->ste_ncres, step->ste_npres);
    }

    /* This may be on if we crashed after this */
    if ( (step->ste_cflag & STE_CTERM) == 0 )
    {
	/* Count one less thread */
	psort->srt_threads--;

	/* Tell Parent we've terminated AND counted ourselves off */
	step->ste_cflag |= STE_CTERM;
    }

    /* free work location mask, if computed */
    if (step->ste_pcb->srt_wloc_mask != step->ste_wrkloc && 
		step->ste_wrkloc)
    {
	(VOID)MEfree((PTR)step->ste_wrkloc);
	step->ste_wrkloc = (i4*)NULL;
    }

    /* Wake up all waiters on bad status or end of thread */
    CScnd_broadcast(&psort->srt_cond);

    CSv_semaphore(&psort->srt_cond_sem);

    /*
    ** Caution: now that we've awakened it, the parent may
    **          have freed our "step" memory along with
    **		everything else that doesn't belong to
    **		this thread, so dereferences are ill-advised.
    */
    psort = (DMS_SRT_CB*)NULL;
    step  = (DM_STECB*)NULL;

    /* End the sort, don't care about this status */
    if ( temp_s = s )
    {
	s = (DMS_SRT_CB*)NULL;
	(VOID)dmse_end_serial(temp_s, &local_dberr);
    }

    EXdelete();

    return(E_DB_OK);

}	/* end of dmse_child_thread */

/** Name: dmse_child_put	- drive put processing in child sort thread.
**
** Description:
**	This function loops on PUT requests asynchronously placed in the
**	exchange buffer array by the parent thread. Each is passed to 
**	the DMF sort PUT routine to be sorted into heap sequence.
**
** Inputs:
**	step		DM_STECB for child thread.
**	 .ste_cbix	index to exchange array entry at which processing 
**			is to begin.
**	 .ste_xbufp	Ptr to exchange buffer array.
**
** Outputs:
**	step
**	 .ste_cflag	Indicates current child thread status.
**
** Returns:
**	nothing.
**
** History:
**	24-feb-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	25-nov-98 (inkdo01)
**	    Add semaphore synchronization protection.
**	01-Apr-2004 (jenjo02)
**	    Loop back to retest condition after child_suspend.
**	19-Apr-2004 (schka24)
**	    Also, recheck condition with semaphore protection before suspend.
**	31-Jan-2006 (kschendel)
**	    Don't sleep if the parent is in the process of going to sleep.
**	    Above changes mistakenly thought that cbix/pbix were semaphore
**	    protected.
*/

static VOID
dmse_child_put(
DM_STECB 	*step,
DB_ERROR	*dberr)

{
    DMS_SRT_CB	*psort = step->ste_pcb;	/* parent thread sort cb */
    char	*xbp;
    i4		cbix;
    DB_STATUS	status = E_DB_OK;
    i4		err_code;

    CLRDBERR(dberr);


    /* Simply stay in loop until all rows have been received from the 
    ** parent thread. Each received row, in turn, is passed to the serial
    ** "put" function. */

    /* loop on PUT's until there ain't no more */

    while ( status == E_DB_OK && 
	    (psort->srt_pstate & SRT_PCERR) == 0 )
    {
	if ( step->ste_cbix >= step->ste_pbix )
	{
	    /* Child has caught parent, wait for Parent to catch up */
	    CSp_semaphore(1, &psort->srt_cond_sem);

	    /* Recheck, although this isn't much help since parent counts
	    ** pbix without the mutex.  It does however set flags under
	    ** protection.
	    */
	    if (step->ste_cbix >= step->ste_pbix)
	    {
		if (psort->srt_pstate & SRT_PEOF)
		{
		    /* We've PUT all of Parent's records, so stop putting */
		    CSv_semaphore(&psort->srt_cond_sem);
		    break;
		}
		else if (step->ste_cflag & STE_PRSTRT)
		{
		    /* Parent is about to wait for us!  Child must have
		    ** zoomed ahead while parent was trying to decide
		    ** if it should wait.  Resume the parent if it's
		    ** actually waiting, then loop again.
		    */
		    child_presume(step, FALSE);
		}
		else
		{
		    /* Parent isn't at EOF or waiting, so really sleep */
		    step->ste_crstrt = step->ste_cbix + 
				    step->ste_bsize * 3 / 4;
		    child_suspend(step, dberr);
		}
	    }
	    CSv_semaphore(&psort->srt_cond_sem);
	    /* loop back to retest condition */
	}
	else
	{
	    /* We ought to have a row here. Compute location in exchange
	    ** buffer and process it. */
	    cbix = step->ste_cbix % step->ste_bsize;
	    xbp = step->ste_xbufp[cbix];

	    /* Count another PUT by this Child */
	    step->ste_c_puts++;

	    status = dmse_put_record_serial(step->ste_ccb, 
					    xbp, 
					    dberr);
	    CS_MEMBAR_MACRO();
	    if ( status == E_DB_OK &&
		 ++step->ste_cbix >= step->ste_prstrt &&
		 step->ste_cflag & STE_PRSTRT)
	    {
		/*
		** Parent had wrapped around and caught child 
		** in exchange buffer, requiring it to suspend 
		** itself to allow child to catch up. 
		** Child has now caught up and can restart parent.
		*/
		child_presume(step, TRUE);
	    }
	}
    }	/* keep looping */

    if ( status == E_DB_OK && (psort->srt_pstate & SRT_PCERR) == 0 )
    {
	/* Good status and some rows assigned to this thread? */
	if ( step->ste_c_puts )
	{
	    if (DMZ_SRT_MACRO(1))
	    {
		dmse_tdiff(step, &step->ste_exec);
		TRdisplay("%p DMSE_TIMEC1: exec - %f, wait - %f, pw %d, pr %d, cw %d, cr %d\n",
		step->ste_ctid,
		step->ste_exec, step->ste_wait, 
		step->ste_npwaits, step->ste_npres, step->ste_ncwaits, step->ste_ncres);
		step->ste_texec = step->ste_exec;
		step->ste_twait = step->ste_wait;
		step->ste_exec = step->ste_wait = 0.0;
	    }

	    /* Finish our slice of the sort */
	    status = dmse_input_end_serial(step->ste_ccb, dberr);
	
	    if ( status == E_DB_OK )
	    {
		if (DMZ_SRT_MACRO(1))
		{
		    dmse_tdiff(step, &step->ste_exec);
		    step->ste_texec += step->ste_exec;
		    step->ste_twait += step->ste_wait;
		    TRdisplay("%p DMSE_TIMEC2: exec - %f, wait - %f, texec - %f, twait - %f, pw %d, pr %d, cw %d, cr %d\n",
		    step->ste_ctid,
		    step->ste_exec, step->ste_wait, step->ste_texec, step->ste_twait, 
		    step->ste_npwaits, step->ste_npres, step->ste_ncwaits, step->ste_ncres);
		    step->ste_exec = step->ste_wait = 0.0;
		}
	    }
	}
    }

    /* Done with puts and input_end, ready for gets */

    CSp_semaphore(1, &psort->srt_cond_sem);

    /* One less Child doing PUTs */
    psort->srt_puts_active--;
    /* Reset parent's getter pointer since we're in control */
    step->ste_pbix = 1;

    if ( status )
    {
	psort->srt_pstate |= SRT_CERR;

	if ( psort->srt_pstatus == E_DB_OK )
	{
	    psort->srt_pstatus = status;
	    psort->srt_pdberr = *dberr;
	}

	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
	uleFormat(NULL, E_DM9714_PSORT_CFAIL, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
    }

    /* Signal state change, error to all waiters */
    CScnd_broadcast(&psort->srt_cond);

    CSv_semaphore(&psort->srt_cond_sem);

    return;
}

/** Name: dmse_child_get	- drive get processing in child sort thread.
**
** Description:
**	This function loops over dmse_get calls for child sort fragment in an
**	attempt to asynchronously feed parent get processing for final merge
**	of total sort.
**
** Inputs:
**	step		DM_STECB for child thread.
**	 .ste_cbix	Index to exchange array entry at which processing 
**			is to begin.
**	 .ste_xbufp	Ptr to exchange buffer array.
**
** Outputs:
**	step
**	 .ste_xbufp	Exchange buffer array with sorted result rows.
**	 .ste_cflag	Current status of child thread.
**
** Returns:
**	nothing.
**
** History:
**	24-feb-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	25-nov-98 (inkdo01)
**	    Add semaphore synchronization protection.
**	01-Apr-2004 (jenjo02)
**	    Loop back to retest condition after child_suspend.
**	19-Apr-2004 (schka24)
**	    Also, recheck condition with semaphore protection before suspend.
**	31-Jan-2006 (kschendel)
**	    Don't sleep if the parent is in the process of going to sleep.
**	    Above changes mistakenly thought that cbix/pbix were semaphore
**	    protected.
*/
static VOID
dmse_child_get(
DM_STECB 	*step,
DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    i4		err_code;
    char	*xbp;
    char	*record;
    i4		cbix;
    DMS_SRT_CB	*psort = step->ste_pcb;	/* parent sort cb */
    DMS_SRT_CB  *s = step->ste_ccb;	/* Child thread sort cb */

    CLRDBERR(dberr);

    /* Get routine reverses the flow of data between parent and child. It
    ** loops over exchange array, issuing get calls to retrieve sorted rows
    ** from this child's fragment into array for parent thread's final merge
    ** processing. Unlike PUT processing, in which parent puts rows into 
    ** exchange buffer and child picks them up, child GET processing puts 
    ** the sorted rows into the exchange buffer for the parent to pick them
    ** up. */

    /* but first, if this thread had nothing to sort, just return.
    ** We signaled the parent when this was discovered, no need to do
    ** it again.
    */
    if (step->ste_c_puts == 0)
	return;

    /* Get until error or end-of-gets */

    while ( status == E_DB_OK &&
	    (psort->srt_pstate & SRT_PCERR) == 0 )
    {
	cbix = step->ste_cbix % step->ste_bsize;

	/* Next available exchange array entry */
	xbp = step->ste_xbufp[cbix];

	status = dmse_get_record_serial(s, &record, dberr);

	if ( DB_FAILURE_MACRO(status) )
	{
	    if ( dberr->err_code == E_DM0055_NONEXT )
	    {
	     	/* end of gets in child sort fragment.  Tell parent
		** and break out of big get loop.
		*/
		status = E_DB_OK;
		CLRDBERR(dberr);
		break;
	    }
	}
	else
	{
	    /* E_DB_INFO if duplicate */
	    status = E_DB_OK;

	    /* Copy record to exchange row buffer */
	    MEcopy((PTR)record, s->srt_iwidth, (PTR)xbp);

	    /* If first get, or if parent is waiting and it's time to
	    ** restart it, wake up parent.  Note that presume takes the
	    ** mutex, so if the parent is deciding to wait, we allow
	    ** it to get all the way into wait state so we can wake it.
	    */
	    if ( step->ste_c_gets++ == 0 ||
		  (step->ste_cflag & STE_PRSTRT &&
	           step->ste_cbix >= step->ste_prstrt) )
	    {
		/* Get Parent moving */
		child_presume(step, TRUE);
	    }

	    /* Advance to next exch buffer row */
	    CS_MEMBAR_MACRO();
	    step->ste_cbix++;

	    while ( (step->ste_cbix - step->ste_pbix) 
			    >= step->ste_bsize-1
		   && (psort->srt_pstate & SRT_PCERR) == 0 )
	    {
		/*
		** We've caught parent, or filled the
		** exchange buffer - time to wait.
		** However be careful not to wait if the parent races
		** ahead and decides to wait for the child.
		** Remember cbix/pbix is not protected, but flags are.
		*/
		CSp_semaphore(1, &psort->srt_cond_sem);

		if (step->ste_cbix - step->ste_pbix >= step->ste_bsize-1
		  && (psort->srt_pstate & SRT_PCERR) == 0
		  && (step->ste_cflag & STE_PRSTRT) == 0 )
		{
		    step->ste_crstrt = step->ste_cbix - step->ste_bsize / 4;

		    child_suspend(step, dberr);
		}
		CSv_semaphore(&psort->srt_cond_sem);
	    }
	}
    }	/* end of big GET loop */

    CSp_semaphore(1, &psort->srt_cond_sem);

    if ( status == E_DB_OK )
    {
	/* Tell parent about normal EOF */
	step->ste_cflag |= STE_CEOG;
    }
    else
    {
	psort->srt_pstate |= SRT_CERR;

	if ( psort->srt_pstatus == E_DB_OK )
	{
	    psort->srt_pstatus = status;
	    psort->srt_pdberr = *dberr;
	}

	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
	uleFormat(NULL, E_DM9714_PSORT_CFAIL, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
    }

    /* Signal state change, error to all waiters */
    CScnd_broadcast(&psort->srt_cond);

    CSv_semaphore(&psort->srt_cond_sem);

    return;
}

/** Name: child_presume	- resume parent thread
**
** Description:
**	This function resumes the parent thread for which this thread is
**	sorting.
**
** Inputs:
**	step		DM_STECB for child thread.
**	mutex_here	TRUE if we need to take/release the semaphore here
**
** Outputs:
**	none 
**
** Returns:
**	nothing.
**
** History:
**	19-mar-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	25-nov-98 (inkdo01)
**	    Add semaphore synchronization protection.
**	04-Apr-2003 (jenjo02)
**	    Awaken parent only if waiting on this child.
**	    Return with srt_sem held.
**	28-Jan-2006 (kschendel)
**	    Awaken parent only if it's waiting for something.
**	02-Oct-2007 (jonj)
**	    Always reset STE_PRSTRT, even if parent isn't now
**	    waiting. Another child thread may have come through
**	    here and awakened the parent, but we still need
**	    to reset our STE_PRSTRT to avoid looping, especially
**	    if running with Ingres threads.
*/
static VOID
child_presume(DM_STECB *step, bool mutex_here)
{
    DMS_SRT_CB	*psort = step->ste_pcb;	/* parent thread sort cb */

    if (mutex_here)
	CSp_semaphore(1, &psort->srt_cond_sem);

    if (psort->srt_pstate & SRT_PWAIT)
    {
	/* Count wakeups by this child */
	step->ste_npres++;
	CScnd_signal(&psort->srt_cond, psort->srt_sid);
    }

    /* Turn off "prstrt might mean something" flag so we don't bash
    ** at the parent over and over.
    */
    step->ste_cflag &= ~STE_PRSTRT;

    if (mutex_here)
	CSv_semaphore(&psort->srt_cond_sem);

    return;
}

/** Name: dmse_tdiff	- utility to roll elapsed time diffs into
**	an accumulated total.
**
*/
static VOID
dmse_tdiff(DM_STECB *step,
	float	*total)
{
    SYSTIME	systime;
    
    TMnow(&systime);		/* latest time */
    *total += (float)(systime.TM_secs - step->ste_secs);
    *total += ((float)(systime.TM_msecs - step->ste_msecs))/1000000.;

    step->ste_secs = systime.TM_secs;
    step->ste_msecs = systime.TM_msecs;
}

/*{
** Name: child_suspend		- Coordinate suspension of child thread
**
** Description:
**	Waits for parent to get to Child's restart point
**	and wake it up.
**
** Inputs:
**	step				The sort thread control block
**
**		Caller must hold srt_cond_sem.
**
** Outputs:
**	srt_pstate			SRT_CERR if interrupted with
**      srt_perr_code                   E_DM0065_USER_INTR
** Exceptions:
**      none
**
** Side Effects:
**	None
**
** History:
**	11-Apr-2003 (jenjo02)
**	    Created.
**	28-Jan-2006 (kschendel)
**	    Clear the child-is-waiting flag when we wake up.  Otherwise
**	    we might wake up from a broadcast and exit, leaving the
**	    flag set, and then the parent might try to twiddle with an
**	    exited child (has led to segv's).
*/
static VOID
child_suspend(
DM_STECB	  *step,
DB_ERROR	  *dberr)
{
    i4		local_err;
    DMS_SRT_CB	*psort = step->ste_pcb;	/* parent thread sort cb */

    /* Don't wait if parent/child error */
    if ( (psort->srt_pstate & SRT_PCERR) == 0 )
    {
	if (DMZ_SRT_MACRO(1)) 
	    dmse_tdiff(step, &step->ste_exec);

	step->ste_ncwaits++;

	/* Tell the parent that we are indeed sleeping */
	step->ste_cflag |= STE_CWAIT;

	if ( CScnd_wait(&psort->srt_cond, &psort->srt_cond_sem) )
	{
	    /* Then we were interrupted */
	    psort->srt_pstate |= SRT_CERR;

	    if ( psort->srt_pstatus == E_DB_OK )
	    {
		psort->srt_pstatus == E_DB_ERROR;
		SETDBERR(&psort->srt_pdberr, 0, E_DM0065_USER_INTR);

		uleFormat(&psort->srt_pdberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &local_err, 0);
		uleFormat(NULL, E_DM9714_PSORT_CFAIL, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &local_err, 0);
	    }
	}
	/* Don't allow parent to think we are still suspended */
	step->ste_ncres++;
	step->ste_cflag &= ~STE_CWAIT;

	if (DMZ_SRT_MACRO(1)) 
	    dmse_tdiff(step, &step->ste_wait);
    }

    return;
}

/*{
** Name: dmse_handler	- Child's exception handler
**
** Description:
**	Catches exceptions that occur in Child threads.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-Mar-2004 (jenjo02)
**	    Added.
*/
static STATUS
dmse_handler(
EX_ARGS		    *ex_args)
{
    i4	    err_code;
    
    if ( ex_args->exarg_num == EX_DMF_FATAL )
	err_code = E_DM904A_FATAL_EXCEPTION;
    else
	err_code = E_DM9049_UNKNOWN_EXCEPTION;

    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    return (EXDECLARE);
}
