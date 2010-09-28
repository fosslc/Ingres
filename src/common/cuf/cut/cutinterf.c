/* Copyright (c) 1998, 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <me.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <cut.h>
#include    <cutint.h>
#include    <ulf.h>

/*
** Name: cutinterf.c	- Call interface to cut, thread coordination modules
**
** Description:
**	This file contains all of the interface routines that make up CUT.
**	CUT is a collection of service modules designed to provide thread
**	communications and coordination for cooperating threads within a 
**	single process. The following functions are defined in this file:
**
**		cut_create_buf	  - Create a thread communications buffers
**		cut_attach_buf	  - Attach to thread communications buffers
**		cut_detach_buf	  - Detach from thread communications buffers
**		cut_query_buf	  - Return communications buffer attributes
**		cut_read_buf	  - read from attached communication buffer
**		cut_write_buf	  - Write to attached communications buffer
**		cut_writer_done	  - Writer is done, won't write any more
**		cut_wakeup_waiters - Wake up read/write waiters
**		cut_signal_status - Signal status to cooperating threads
**		cut_event_wait    - Wait for specified CUT event
**		cut_local_gca	  - Send GCA type messages to cooperating
**				    threads
**		cut_thread_init	  - Init CUT thread structures
**		cut_thread_term	  - Remove thread from CUT
**		cut_read_complete - Complete DIRECT read
**		cut_write_complete- Complete DIRECT write
**
** History:
**	6-may-98 (stephenb)
**	    Initial creation as stubs
**	20-may-99 (stephenb)
**	    Replace nat and longnat with i4.
**      3-jun-99 (stephenb)
**	    Check return status from CS calls for interrupt status, which
**	    may be caused by a call to cut_signal_status() by another thread,
**	    in these cases we should return this status to the caller.
**	jun-1998 and may-jun-1999 (stephenb)
**	    Various fixes for problems uncovereed with CUT test suite
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	19-Apr-2004 (schka24)
**	    Implement hysteresis in buffer wakeup; various fixes.
**	25-Apr-2004 (schka24)
**	    Add wakeup-waiters function.  Add writer-done mechanism.
**	    Release semaphores before logging errors.
**	15-Jul-2004 (jenjo02)
**	    Heaps of changes to decrease semaphore blocks, fixes to
**	    make >1 row reads from a buffer work, semaphore leaks.
**	03-Sep-2004 (jenjo02)
**	    Added support for DIRECT (previously called "LOCATE")
**	    mode writes in addition to reads, "cut_async_status"
**	    to CUT RCB so bulk readers/writers can catch signals
**	    while off doing other things.
**	08-Sep-2007 (thaju02)
**	    Add memory barrier/fence call to prevent out-of-order
**	    execution; prevent increment of cut_buf_write_comps 
**	    before record has been copied to cut buffer. (b119100)
**	21-Nov-2007 (jonj)
**	    In cut_read_buf(), must wait if no cells to return
**	    and there are no writers yet and this is an Ingres
**	    threaded installation; the writers can only write when
**	    this thread yields to them.
**	    Add CutIsMT boolean.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    "log_error" function now conforms to new uleFormatFcn()
**	    prototype in which 1st parameter is DB_ERROR *.
**	    Repair use of obsolete (CL_SYS_ERR*)0 with (CL_ERR_DESC*)NULL.
*/

/*
** Static and/or forward references
*/
static	bool	CutIsMT;	/* CUT running with OS threads? */

/* 
** buffer master list
*/
static	CS_SEMAPHORE	Buf_list_sem;
static	CUT_INT_BUF	*Buf_list = NULL;

/*
** Thread master list
*/
static	CS_SEMAPHORE	Thread_list_sem;
static	CUT_TCB		*Thread_list = NULL;

/*
** memory pools
*/
static	PTR		cut_tcb_pool = NULL;
static	PTR		cut_buf_thread_pool = NULL;
static	PTR		cut_int_buf_pool = NULL;

/*
** functions
*/
static	DB_STATUS	buffer_sem(CUT_INT_BUF		*buffer,
				   bool			hold_list_sem,
				   char			*caller,
				   DB_ERROR		*error);

static	DB_STATUS	get_block(i4		type, 
				  PTR		*block, 
				  DB_ERROR	*error);

static	DB_STATUS	(*log_error)();

static  VOID		 ReadComplete(CUT_INT_BUF	 *cur_buf,
				  CUT_BUF_THREAD *my_thread);

static  VOID		 WriteComplete(CUT_INT_BUF	 *cur_buf,
				  CUT_BUF_THREAD *my_thread);


/*
** Name: cut_create_buf - create thread communications buffers
**
** Description:
**	This function creates the stipulated number of thread communications
**	buffers, each buffer will contain the stipulated number of cells,
**	each cell will be the stipulated size. A handle to each buffer is
**	returned which must be used in all other CUT calls to refference the 
**	buffer.
**
** Inputs:
**	num_bufs			Number of buffers to create
**	cut_rcb				Array of num_bufs rcbs to be created
**	    .cut_buf_name		Name for Buffer
**	    .cut_num_cells		Number of cells in buffer
**	    .cut_cell_size		Size of each cell
**	    .cut_buf_use		Buffer usage, CUT_BUF_READ or 
**					CUT_BUF_WRITE
**					Can include PARENT, TRACE flags
**
** Outputs:
**	cut_rcb.cut_buf_handle		Handle to created buffer
**	cut_rcb.cut_thr_handle		Thread's handle to buffer.
**	cut_rcb.cut_num_cells		Number of cells allocated.
**
** Returns:
**	E_DB_OK		Buffer was created
**	E_DB_ERROR	Error creating buffer
**
** Side effects:
**	Process virtual memory is expanded by size of buffer.
**
** History:
**	6-may-98 (stephenb)
**	    Created as stub
**	26-may-98 (stephenb)
**	    First cut of code
**	18-Apr-2004 (schka24)
**	    Semaphore protect get-block call.
**	26-Apr-2004 (schka24)
**	    Count attached writers.
**	16-Sep-1004 (schka24)
**	    Grab PARENT flag, set in buf-thread block.
*/
DB_STATUS
cut_create_buf(
	i4		num_bufs,
	CUT_RCB		**cut_rcb,
	DB_ERROR	*error)
{
    i4			i,j, erval;
    STATUS		cl_stat;
    CUT_INT_BUF		*cur_buf;
    CUT_TCB		*cur_thread;
    bool		thread_init = FALSE;
    CUT_BUF_THREAD	*thread_list;
    i4			erlog;
    DB_STATUS		status = E_DB_OK;
    i4			use;

    /*
    ** check parms
    */
    CLRDBERR(error);
    if (num_bufs < 1)
    {
	SETDBERR(error, 0, E_CU0101_BAD_BUFFER_COUNT);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
	    &erlog, 2, sizeof(i4), &num_bufs, 15, "cut_create_buf");
	return (E_DB_ERROR);
    }
    for (i = 0; i < num_bufs; i++)
    {
	if (cut_rcb[i] == NULL)
	{
	    SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 2, sizeof(i4), &i, 15, "cut_create_buf");
	    return (E_DB_ERROR);
	}

	/* For now, ignore the extra flags */
	use = cut_rcb[i]->cut_buf_use & ~(CUT_BUF_TRACE | CUT_BUF_PARENT);

	if ( use != CUT_BUF_READ && use != CUT_BUF_WRITE )
	{
	    SETDBERR(error, 0, E_CU0103_BAD_BUFFER_USE);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 3, sizeof(i4), &use,
		sizeof(i4), &i, 0, "cut_create_buf");
	    return (E_DB_ERROR);
	}
	if (cut_rcb[i]->cut_num_cells < 1 || 
	    cut_rcb[i]->cut_cell_size < 1)
	{
	    SETDBERR(error, 0, E_CU0104_BAD_CELLS);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 3, sizeof(i4), &cut_rcb[i]->cut_num_cells, 
		sizeof(i4), &cut_rcb[i]->cut_cell_size, 
		15, "cut_create_buf");
	    return (E_DB_ERROR);
	}
    }
    /*
    ** Check that thread is initialized in CUT, initialize if not
    */
    if (cut_thread_init((PTR *)&cur_thread, error) > E_DB_INFO)
    {
	/* message should already be logged, just retrurn */
	return(E_DB_ERROR);
    }
    /*
    ** grab and initialize each buffer
    */
    for(i = 0; i < num_bufs; i++)
    {
	do
	{
	    /* Iteratively ask for 1/2 of last attempt if no mem */
	    cut_rcb[i]->cut_buf_handle = MEreqmem(0, sizeof(CUT_INT_BUF) +
		(cut_rcb[i]->cut_cell_size * cut_rcb[i]->cut_num_cells), 
		FALSE, &cl_stat);
	} while ( cl_stat && (cut_rcb[i]->cut_num_cells >>= 1) > 8 );

        if (cut_rcb[i]->cut_buf_handle == NULL || cl_stat != OK)
        {
	    erval = sizeof(CUT_INT_BUF) +
		(cut_rcb[i]->cut_cell_size * cut_rcb[i]->cut_num_cells+1);
	    SETDBERR(error, 0, E_CU0105_NO_MEM);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 2, sizeof(i4), &erval,
		15, "cut_create_buf");
	    status = E_DB_ERROR;
	    break;
	}

	cur_buf = (CUT_INT_BUF *)cut_rcb[i]->cut_buf_handle;
	/* Clean it out */
	FREE_BLOCK(sizeof(CUT_INT_BUF), cur_buf);

	/* Prime caller's async_status */
	cut_rcb[i]->cut_async_status = E_DB_OK;

	/* init sem */
	cl_stat = CSw_semaphore(&cur_buf->cut_buf_sem, CS_SEM_SINGLE,
	    cut_rcb[i]->cut_buf_name);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU0106_INIT_SEM);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	/* init wait conditions */
	cl_stat = CScnd_init(&cur_buf->cut_read_cond);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU0107_INIT_READ_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	cl_stat = CScnd_init(&cur_buf->cut_write_cond);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU0108_INIT_WRITE_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	cl_stat = CScnd_init(&cur_buf->cut_event_cond);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU0109_INIT_EVENT_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	/* and name */
	cl_stat = CScnd_name(&cur_buf->cut_read_cond, 
	    cut_rcb[i]->cut_buf_name);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU010A_NAME_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;	    
	}
	cl_stat = CScnd_name(&cur_buf->cut_write_cond, 
	    cut_rcb[i]->cut_buf_name);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU010A_NAME_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	cl_stat = CScnd_name(&cur_buf->cut_event_cond, 
	    cut_rcb[i]->cut_buf_name);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU010A_NAME_COND);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
		&erlog, 1, sizeof(cut_rcb[i]->cut_buf_name),
		cut_rcb[i]->cut_buf_name);
	    status = E_DB_ERROR;
	    break;
	}
	/* attach this thread to buffer (first in list)*/
	CSp_semaphore(TRUE, &Buf_list_sem);
	status = get_block(CUT_BUF_THREAD_TYPE,
		(PTR *)&thread_list, error);
	if (status != E_DB_OK)
	{
	    CSv_semaphore(&Buf_list_sem);
	    /* error should be logged in function */
	    status = E_DB_ERROR;
	    break;
	}
	cur_buf->cut_thread_list = thread_list;

	/* Return thread's handle to this buffer */
	cut_rcb[i]->cut_thr_handle = (PTR)thread_list;

	thread_list->buf_next = thread_list->buf_prev = thread_list;
	thread_list->cut_int_buffer = cur_buf;
	thread_list->cut_tcb = cur_thread;

	/* Capture flags from usage, then store without flags */
	use = cut_rcb[i]->cut_buf_use;
	thread_list->cut_parent_thread = (use & CUT_BUF_PARENT) != 0;
	cur_buf->cut_trace = (use & CUT_BUF_TRACE) != 0;
	use &= ~(CUT_BUF_PARENT | CUT_BUF_TRACE);
	thread_list->cut_thread_use = use;
	thread_list->cut_async_status = &cut_rcb[i]->cut_async_status;
	cur_buf->cut_buf_info.cut_num_threads = 1;
	cur_buf->cut_buf_info.cut_hwm_threads = 1;
	/* Other fields zeroed by MEreqmem call */
	/* If we're a writer, remember that */
	if (use == CUT_BUF_WRITE)
	{
	    cur_buf->cut_buf_writers++;
	    cur_buf->cut_writers_attached = TRUE;
	}
	/* setup static buffer data */
	STRUCT_ASSIGN_MACRO(*cut_rcb[i], cur_buf->cut_ircb);
	cur_buf->cut_ircb.cut_thr_handle = NULL;

	/* add thread buffer to thread list - note reuse of thread_list */
	if (thread_list = cur_thread->cut_buf_list)
	{
	    cur_buf->cut_thread_list->thread_next = thread_list;
	    cur_buf->cut_thread_list->thread_prev = thread_list->thread_prev;
	    thread_list->thread_prev->thread_next = cur_buf->cut_thread_list;
	    thread_list->thread_prev = cur_buf->cut_thread_list;
	}
	else
	{
	    /* list is empty */
	    cur_thread->cut_buf_list = cur_buf->cut_thread_list->thread_next =
		cur_buf->cut_thread_list->thread_prev = 
		cur_buf->cut_thread_list;
	}
	/*
	** add buffer to buffer list
	*/
	if (Buf_list)
	{
	    cur_buf->buf_prev = Buf_list->buf_prev;
	    cur_buf->buf_next = Buf_list;
	    Buf_list->buf_prev->buf_next = cur_buf;
	    Buf_list->buf_prev = cur_buf;
	}
	else
	    Buf_list = cur_buf->buf_next = cur_buf->buf_prev = cur_buf;
	CSv_semaphore(&Buf_list_sem);
    }
    if (status != E_DB_OK)
    {
	/* clean up */
	for (j = 0; j <= i; j++)
	{
	    if ( cur_buf = (CUT_INT_BUF*)(cut_rcb[j]->cut_buf_handle) )
	    {
		/* Don't leak conditions, mutexes */
		CScnd_free(&cur_buf->cut_read_cond);
		CScnd_free(&cur_buf->cut_write_cond);
		CScnd_free(&cur_buf->cut_event_cond);
		CSr_semaphore(&cur_buf->cut_buf_sem);
		/* free memory */
		MEfree(cut_rcb[j]->cut_buf_handle);
	    }
	}
    }

    return(status);
}

/*
** Name: cut_attach_buf - Attach to specified buffer
** 
** Description:
**	Attaches the calling thread to the specified buffer. Threads
**	that create buffers are automatically attached to them, all other
**	threads that wish to communicate, or cooperate with each other through
**	the buffer need to explicitly attach to it first.
**
**	Currently a thread can only attach to a buffer for CUT_BUF_READ or
**	CUT_BUF_WRITE, but not both (although both statuses use seperate bits
**	and can therefore be set concurrently for future expansion).
**
**	The architecture is such that a thread can attach to a buffer
**	at any time during the life of the buffer. The thread's current 
**	position in the buffer will be the position of the buffer tail at the 
**	time the thread attaches.
**
** Inputs:
**	cut_rcb			Buffer to attach to:
**	    .cut_buf_use	Use for this buffer, CUT_BUF_READ or 
**				CUT_BUF_WRITE
**	    .cut_buf_handle	Buffer handle
** Outputs:
**	error			Returned error code (if any)
**
** Returns:
**	DB_STATUS
**
** Side effects:
**	None
**
** History:
**	22-may-98 (stephenb)
**	    Created as stub
**	26-may-98 (stephenb)
**	    First cut of code
**	3-aug-98 (stephenb)
**	    Alter interface to take only one buffer at a time, there's no point
**	    in having more and it only complicated the code.
**	26-Apr-2004 (schka24)
**	    Count attached writers.
**	17-May-2004 (schka24)
**	    Hold buffer list sem for get-block, else two threads can end
**	    up using the same buf-thread block - ugh.
**	17-Sep-2004 (jenjo02)
**	    Track high water mark of attaches.
**      21-jul-2010 (huazh01)
**          assign the return status of cut_thread_init() into 'status' 
**          before testing it with E_DB_INFO. (124094)
*/
DB_STATUS
cut_attach_buf(
	CUT_RCB		*cut_rcb,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    CS_SID		sid;
    CUT_INT_BUF		*check_buf;
    bool		valid_buf = FALSE;
    CUT_TCB		*cur_thread;
    bool		thread_attached;
    DB_STATUS		status = E_DB_OK;
    CUT_BUF_THREAD	*new_thread;
    CUT_BUF_THREAD	*thread_list;
    STATUS		cl_stat;
    i4			erlog, erval;
    bool		trace;
    bool		parent;

    /*
    ** check parms
    */
    CLRDBERR(error);

    if (cut_rcb == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 2, sizeof(i4), &erval, 15, "cut_attach_buf");
	return (E_DB_ERROR);
    }

    /* Capture, then discard, trace and parent flags */
    /* Note: no check is made for multiple threads claiming to be a
    ** buffer's parent.  This might be something nice to add "someday",
    ** but it's a fairly blatant caller error.
    ** (CUT doesn't care;  parentage is only of interest to abnormal
    ** thread termination cleanup.)
    */
    trace = (cut_rcb->cut_buf_use & CUT_BUF_TRACE) != 0;
    parent = (cut_rcb->cut_buf_use & CUT_BUF_PARENT) != 0;
    cut_rcb->cut_buf_use &= ~(CUT_BUF_TRACE | CUT_BUF_PARENT);

    /* Prime async_status in caller's RCB */
    cut_rcb->cut_async_status = E_DB_OK;

    if (cut_rcb->cut_buf_use != CUT_BUF_READ && 
	cut_rcb->cut_buf_use != CUT_BUF_WRITE)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0103_BAD_BUFFER_USE);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 3, sizeof(i4), &erval, sizeof(i4), 
	    &cut_rcb->cut_buf_use, 15, "cut_attach_buf");
	return (E_DB_ERROR);
    }

    /* 
    ** check thread is initialized in cut, init if not
    */
    if ((status = cut_thread_init((PTR *)&cur_thread, error)) > E_DB_INFO)
    {
	cur_thread = (CUT_TCB*)NULL;
	/* error is already logged, keep going */
    }

    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    
    /*
    ** check for valid buffer and grab sem.
    ** Tell buffer-sem to keep and not release the Buf-list-sem,
    ** to properly protect the get-block call that we're about to do.
    */
    if (buffer_sem(cur_buf, TRUE, "cut_attach_buf", error) != E_DB_OK)
    {
	/* error already logged */
	return(E_DB_ERROR);
    }

    /* Up HWM of attach attempts */
    cur_buf->cut_buf_info.cut_hwm_threads++;

    if ( !cur_thread )
    {
	/* Couldn't init thread, bail */
	CSv_semaphore(&cur_buf->cut_buf_sem);
	CSv_semaphore(&Buf_list_sem);
	return(status);
    }

    /* check that thread is not already attached to this buffer */
    if ( thread_list = cur_thread->cut_buf_list ) 
    {
	do
	{
	    if ( thread_list->cut_int_buffer == cur_buf )
	    {
		cur_buf->cut_buf_info.cut_hwm_threads--;
		CSv_semaphore(&cur_buf->cut_buf_sem);
		CSv_semaphore(&Buf_list_sem);
		SETDBERR(error, 0, W_CU0301_THREAD_ATTACHED);
		return(E_DB_WARN);
	    }
	    thread_list = thread_list->thread_next;
	} while ( thread_list != cur_thread->cut_buf_list );
    }

    /* get block */
    if (get_block(CUT_BUF_THREAD_TYPE, (PTR*)&new_thread, error) != E_DB_OK)
    {
	/* error already logged */
	CSv_semaphore(&cur_buf->cut_buf_sem);
	CSv_semaphore(&Buf_list_sem);
	return(E_DB_ERROR);
    }
    /* That's all we need Buf_list_sem for */
    CSv_semaphore(&Buf_list_sem);

    /* Return thread's handle to this buffer */
    cut_rcb->cut_thr_handle = (PTR)new_thread;

    /* attach thread to buffer list */
    thread_list = cur_buf->cut_thread_list;
    new_thread->buf_prev = thread_list->buf_prev;
    new_thread->buf_next = thread_list;
    thread_list->buf_prev->buf_next = new_thread;
    thread_list->buf_prev = new_thread;
    /* bump stats */
    cur_buf->cut_buf_info.cut_num_threads++;
    /* and then to thread list */
    if (thread_list = cur_thread->cut_buf_list)
    {
	new_thread->thread_prev = thread_list->thread_prev;
	new_thread->thread_next = thread_list;
	thread_list->thread_prev->thread_next = new_thread;
	thread_list->thread_prev = new_thread;
    }
    else
    {
	cur_thread->cut_buf_list = new_thread->thread_next =
	    new_thread->thread_prev = new_thread;
    }
    new_thread->cut_int_buffer = cur_buf;
    new_thread->cut_tcb = cur_thread;
    new_thread->cut_thread_use = cut_rcb->cut_buf_use;
    new_thread->cut_parent_thread = parent;
    new_thread->cut_thread_state = 0;
    new_thread->cut_flags = 0;
    new_thread->cut_num_cells = 0;
    new_thread->cut_async_status = &cut_rcb->cut_async_status;
    if ( cut_rcb->cut_buf_use == CUT_BUF_READ )
    {
	new_thread->cut_current_cell = cur_buf->cut_buf_info.cut_buf_tail;
	new_thread->cut_cell_ops = cur_buf->cut_buf_info.cut_cell_reads;
    }
    else
    {
	new_thread->cut_current_cell = 0;
	new_thread->cut_cell_ops = 0;
	cur_buf->cut_buf_writers++;
	cur_buf->cut_writers_attached = TRUE;
    }

    /* If turning on tracing, set the bit (but don't turn it off) */
    if ( trace )
	cur_buf->cut_trace = TRUE;

    /*
    ** signal waiting threads 
    */
    CScnd_broadcast(&cur_buf->cut_event_cond);
    /*
    ** release buffer sem
    */
    CSv_semaphore(&cur_buf->cut_buf_sem);

    /*
    ** Cleanup
    */
    return(E_DB_OK);
}

/*
** Name: cut_detach_buf - Detach from list of buffers
**
** Description:
**	Detaches the calling thread from the provided list of buffers, the
**	thread must have previously called cut_attach_buf() to attach to
**	each of the buffers specified.
**
**	The architecture is such that a thread can detach from a buffer
**	at any time during the life of the buffer, but if the thread's
**	current location is anything other than the buffer head location
**	a warning is returned. The last thread to detach from the buffer will
**	automatically cause the buffer to be destroyed, in this instance
**	the thread will not be allowed to detach when there are outstanding
**	cells to be read.  (But see the "unconditional" flag below.)
**
**
** Inputs:
**	num_bufs			Number of buffers to detach from
**	cut_rcb.cut_buf_handle		Handle of buffer to detach from
**	unconditional			TRUE to detach unconditionally,
**					even if we are the last reader with
**					more to read (typically for error
**					handling/recovery situations)
**
** Outputs:
**	error				Returned error code (if any)
**
** Returns:
**	E_DB_OK		Detach succeeded
**	E_DB_WARN	Detach succeeded, but thread still had data to read
**	E_DB_ERROR	Detach failed
**
** Side effects:
**	The detach may automatically move the buffer tail because there
**	are pending reads that need to be read only by this thread
**
**	The detach may automatically destroy the buffer if there are no
**	other threads currently attached.
**
** History:
**	22-may-98 (stephenb)
**	    Created as a stub.
**	27-may-98 (stephenb)
**	    First cut of code.
**	19-Apr-2004 (schka24)
**	    Added unconditional-detach flag for error situations.
**	    Properly unlink last buffer attached to thread.
**	26-Apr-2004 (schka24)
**	    Add last-writer wakeup.
**	18-Aug-2004 (jenjo02)
**	    During last-writer wakeup, inform readers that
**	    there are no more writes coming.
**	04-Sep-2004 (jenjo02)
**	    Added calls to ReadComplete/WriteComplete if
**	    detaching thread has incomplete reads/writes.
*/
DB_STATUS
cut_detach_buf(
	CUT_RCB		*cut_rcb,
	bool		unconditional,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    i4			tail_dif = 0;
    STATUS		cl_stat;
    CS_SID		my_sid;
    DB_STATUS		status = E_DB_OK;
    CUT_BUF_THREAD	*cur_thread;
    CUT_BUF_THREAD	*thread_list;
    bool		thread_attached;
    i4			erlog, erval;
    i4			num_readers = 0;

    /*
    ** Check params
    */
    CLRDBERR(error);

    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 15, "cut_detach_buf");
	return (E_DB_ERROR);
    }
    CSget_sid(&my_sid);
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    /*
    ** check for valid buffer and grab sem.
    ** Hold the master list semaphore too.
    */
    if (buffer_sem(cur_buf, TRUE, "cut_detach_buf", error) != E_DB_OK)
    {
	/* error already logged */
	return (E_DB_ERROR);
    }
    /*
    ** find thread
    */
    cur_thread = NULL;
    thread_list = cur_buf->cut_thread_list;
    thread_attached = FALSE;
    do
    {
	if (thread_list->cut_tcb->cut_thread_id == my_sid)
	{
	    cur_thread = thread_list;

	    /* Check for incomplete writes or reads */
	    if ( cur_thread->cut_thread_state == CUT_WRITE )
	    {
		cur_thread->cut_flags |= CUT_RW_FLUSH;
		WriteComplete(cur_buf, cur_thread);
	    }
	    else if ( cur_thread->cut_thread_state == CUT_READ )
		ReadComplete(cur_buf, cur_thread);
	}
	else if ( thread_list->cut_thread_use == CUT_BUF_READ &&
	          thread_list->cut_current_cell != 
			cur_buf->cut_buf_info.cut_buf_tail )
	{
	    i4	dif;

	    dif = thread_list->cut_current_cell - 
		    cur_buf->cut_buf_info.cut_buf_tail;
	    if (dif < 0)
		dif += cur_buf->cut_ircb.cut_num_cells;
	    if (dif > tail_dif)
		tail_dif = dif;
	}
	if (thread_list->cut_thread_use == CUT_BUF_READ)
	    num_readers++;
	thread_list = thread_list->buf_next;
    } while (thread_list != cur_buf->cut_thread_list);
    if (!cur_thread)
    {
	SETDBERR(error, 0, W_CU0302_THREAD_NOT_ATTACHED);
	CSv_semaphore(&cur_buf->cut_buf_sem);
	CSv_semaphore(&Buf_list_sem);
	return(E_DB_WARN);
    }

    /* If we care, check various thread a buffer states */
    if ( !unconditional )
    {
	/* check thread status */
	if (cur_thread->cut_tcb->cut_thread_status != E_DB_OK)
	{
	    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    CSv_semaphore(&Buf_list_sem);
	    return(cur_thread->cut_tcb->cut_thread_status);
	}
	/* check state */
	if (cur_thread->cut_thread_state != 0)
	{
	    SETDBERR(error, 0, E_CU0201_BAD_THREAD_STATE);
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    CSv_semaphore(&Buf_list_sem);
	    return(E_DB_ERROR);
	}
	/* Check if there is unread buffer data, unless we don't care */
	if ( cur_thread->cut_thread_use == CUT_BUF_READ &&
	     cur_thread->cut_current_cell != 
		cur_buf->cut_buf_info.cut_buf_head )
	{
	    /* something still to read, are we the last */
	    if (num_readers <= 1)
	    {
		SETDBERR(error, 0, E_CU0202_READ_CELLS);
		CSv_semaphore(&cur_buf->cut_buf_sem);
		CSv_semaphore(&Buf_list_sem);
		return(E_DB_ERROR);
	    }
	    else
	    {
		status = E_DB_WARN;
		SETDBERR(error, 0, W_CU0303_UNREAD_CELLS);
		/* move tail */
		cur_buf->cut_buf_info.cut_buf_tail = 
		    (cur_buf->cut_buf_info.cut_buf_tail + tail_dif) %
		    cur_buf->cut_ircb.cut_num_cells;
	    }
	}
    }
    /* If we were a writer, count down buffer writers, and wake up
    ** readers if we're the last writer.
    */
    if (cur_thread->cut_thread_use == CUT_BUF_WRITE)
    {
	if (--cur_buf->cut_buf_writers <= 0
	  && cur_buf->cut_buf_info.cut_num_threads > 1)
	{
	    CUT_BUF_THREAD	*reader = thread_list;

	    /* Inform readers that no more writes are coming */
	    do
	    {
		if (reader->cut_thread_use == CUT_BUF_READ)
		    reader->cut_thread_flush = TRUE;

		reader = reader->buf_next;
	    } while (reader != cur_buf->cut_thread_list);

	    cur_buf->cut_buf_rwakeup = 0;
	    CScnd_broadcast(&cur_buf->cut_write_cond);
	}
    }
    /*
    ** detach from buffer (thread_list is cur_buf->cut_thread_list)
    */
    if (cur_thread == thread_list)
    {
	if (cur_thread == cur_thread->buf_next)
	    cur_buf->cut_thread_list = NULL;
	else
	    cur_buf->cut_thread_list = cur_thread->buf_next;
    }
    /* (This is bogus linking if last entry on list, but no harm done) */
    cur_thread->buf_prev->buf_next = cur_thread->buf_next;
    cur_thread->buf_next->buf_prev = cur_thread->buf_prev;
    /*
    ** and then from thread list
    */
    thread_list = cur_thread->cut_tcb->cut_buf_list;
    if (cur_thread == thread_list)
    {
	if (cur_thread == cur_thread->thread_next)
	    cur_thread->cut_tcb->cut_buf_list = NULL;
	else
	    cur_thread->cut_tcb->cut_buf_list = cur_thread->thread_next;
    }
    /* (This is bogus linking if last entry on list, but no harm done) */
    cur_thread->thread_next->thread_prev = cur_thread->thread_prev;
    cur_thread->thread_prev->thread_next = cur_thread->thread_next;
    FREE_BLOCK(sizeof(CUT_BUF_THREAD), cur_thread);
    if (--cur_buf->cut_buf_info.cut_num_threads > 0)
    {
	/*
	** signal any threads waiting on detach
	*/
	CScnd_broadcast(&cur_buf->cut_event_cond);
	CSv_semaphore(&cur_buf->cut_buf_sem);
	CSv_semaphore(&Buf_list_sem);
    }
    else
    {
	/* Buffer is completely orphaned, remove buffer from list */
	/* Note:
	** Just in case another thread sneaks in while we are doing this,
	** we always check that num_threads is non-zero when obtaining the
	** buffer semaphore elsewhere
	*/
	if (Buf_list == cur_buf)
	{
	    if (cur_buf->buf_next == cur_buf && 
		cur_buf->buf_prev == cur_buf)
	    {
		/* only buffer in the list */
		Buf_list = NULL;
	    }
	    else
	    {
		Buf_list = cur_buf->buf_next;
		cur_buf->buf_prev->buf_next = Buf_list;
		cur_buf->buf_next->buf_prev = cur_buf->buf_prev;
	    }
	}
	else
	{
	    cur_buf->buf_next->buf_prev = cur_buf->buf_prev;
	    cur_buf->buf_prev->buf_next = cur_buf->buf_next;
	}
	CSv_semaphore(&cur_buf->cut_buf_sem);
	CSv_semaphore(&Buf_list_sem);
	/*
	** Note: timing issue here, semaphore is released before freeing
	** memory (because it's part of the block), someone could try to
	** attach to the buffer if we get switched out right at this line
	** of code. We avoid this by removeing the buffer from this list
	** before releasing sem. The attach code checks the list for the 
	** buffer before attaching the thread
	*/

	/* Clobber those things which we use to verify buffers */
	cur_buf->cut_in_use = FALSE;
	cur_buf->cut_ircb.cut_buf_handle = NULL;

	/* Don't leak conditions, mutexes */
	CScnd_free(&cur_buf->cut_read_cond);
	CScnd_free(&cur_buf->cut_write_cond);
	CScnd_free(&cur_buf->cut_event_cond);
	CSr_semaphore(&cur_buf->cut_buf_sem);
	MEfree((PTR)cur_buf);
    }
    return(status);
}

/*
** Name: cut_info - Return buffer attributes
**
** Description:
**	Returns useful information about CUT. Valid operators are:
**
**	CUT_INFO_BUF - Information about the provided buffer handle.
**
**		Buffer The information is in two parts, the first part is 
**		static (according to this thread) information which will 
**		remain constant for the life of the buffer. The second part is 
**		dynamic and only guarunteed to be correct at the time of 
**		the call.
**
**	CUT_ATT_BUFS - List of buffers attached to given thread
**
**		The current thread is used if none is provided, this is
**		the only option supported currently. A possible
**		future addition will be to provide a list for the
**		thread(s) stipulated in cut_buf_info.cut_thread. 
**
**		The list is returned one buffer at a time. The entire list is 
**		returned though many calls to cut_info(). If cut_rcb is NULL
**		then the first buffer in the list is returned, otherwhise the
**		next buffer after cut_rcb is provided.
**
**		The list is not guarunteed to be static between calls, if the
**		thread attaches to another buffer between calls it may, or
**		may not show up in the list (depending on the current position
**		of the caller). This is an issue that needs to be thought about
**		when providing lists for other threads.
**
**	CUT_INFO_THREAD - Information about thread status in provided buffer
**
**		Provides information about the current thread in the provided
**		buffer (thread status, current position, etc...)
**
**	CUT_ATT_THREADS - List of threads attached to a given buffer
**
** Inputs:
**	cut_rcb.cut_buf_handle		Buffer handle
**	cut_buf_info			Place to put info
**
** Outputs:
**	cut_rcb				Static buffer information
**	cut_buf_info			Dynamic buffer information
**
** Returns:
**	DB_STATUS
**
** History:
**	28-may-98 (stephenb)
**	    First written.
**	15-Sep-2004 (schka24)
**	    Teach cut-att-bufs query to return buf, thread handles.
**	    Also invent, return parent-thread indicator.
*/
DB_STATUS
cut_info(
	i4		operator,
	CUT_RCB		*cut_rcb,
	PTR		info_buf,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    CUT_INT_BUF		*check_buf;
    bool		valid_buf = FALSE;
    STATUS		cl_stat;
    CS_SID		sid;
    CUT_BUF_THREAD	*cur_thread;
    i4			erlog, erval;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(error);

    switch (operator)
    {
	case CUT_INFO_BUF:
	{
	    CUT_BUF_INFO	*cut_buf_info = (CUT_BUF_INFO *)info_buf;

    	    /* check parms */
    	    if (cut_rcb == NULL || cut_buf_info == NULL ||
		cut_rcb->cut_buf_handle == NULL)
    	    {
		erval = 0;
		SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 2, sizeof(i4), &erval, 9, "cut_info");
		return (E_DB_ERROR);
    	    }
    	    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
	    /* check for valid buffer and grab sem */
	    if (buffer_sem(cur_buf, FALSE, "cut_info(CUT_INFO_BUF)", error) != E_DB_OK)
	    {
		/* error already logged */
		return(E_DB_ERROR);
	    }
    	    /* consistency check */
    	    if (cur_buf->cut_ircb.cut_buf_handle != 
		cut_rcb->cut_buf_handle)
    	    {
		CSv_semaphore(&cur_buf->cut_buf_sem);
		SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 2, sizeof(PTR),&cut_rcb->cut_buf_handle ,
		    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
		return (E_DB_ERROR);
    	    }
    	    /* fill out info */
    	    STRUCT_ASSIGN_MACRO(cur_buf->cut_ircb, *cut_rcb);
    	    STRUCT_ASSIGN_MACRO(cur_buf->cut_buf_info, *cut_buf_info);

    	    /* release sem */
    	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    break;
	}
	case CUT_ATT_BUFS:
	{
	    bool	att_buf = FALSE;

	    /* get session id */
	    CSget_sid(&sid);

	    cl_stat = CSp_semaphore(TRUE, &Buf_list_sem);
	    if (cl_stat != OK)
	    {
		SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 1, 0, "Buffer List semaphore");
		return (E_DB_ERROR);

	    }
	    if (Buf_list == NULL)
	    {
		SETDBERR(error, 0, W_CU0304_NO_BUFFERS);
		CSv_semaphore(&Buf_list_sem);
		return(E_DB_WARN);
	    }
	    if (cut_rcb == NULL)
	    {
		CSv_semaphore(&Buf_list_sem);
		erval = 0;
		SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 2, sizeof(i4), &erval, 9, "cut_info");
		return (E_DB_ERROR);
	    }
	    if (cut_rcb->cut_buf_handle)
	    {
		/* get next */
	        cur_buf = 
		    ((CUT_INT_BUF *)cut_rcb->cut_buf_handle)->buf_next;
		/* check for last buffer */
		if (cur_buf == Buf_list)
		{
		    SETDBERR(error, 0, I_CU0401_LAST_BUFFER);
		    CSv_semaphore(&Buf_list_sem);
		    return(E_DB_INFO);
		}
	        /*
	        ** valid buffer ?
	        */
	        if (check_buf = Buf_list)
	        {
		    do
		    {
		        if (check_buf == cur_buf)
		        {
			    valid_buf = TRUE;
			    break;
		        }
		        check_buf = check_buf->buf_next;
		    } while (check_buf != Buf_list);
	        }
	        if (!valid_buf)
	        {
		    CSv_semaphore(&Buf_list_sem);
		    SETDBERR(error, 0, E_CU010D_BUF_NOT_IN_LIST);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
			0, 0, 0, &erlog, 2, sizeof(PTR), 
			&cut_rcb->cut_buf_handle , 9, "cut_info");
		    return (E_DB_ERROR);

	        }
	    }
	    else
		cur_buf = Buf_list;
	    do
	    {
		bool	connected;

		cl_stat = CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
		if (cl_stat != OK)
		{
		    CSv_semaphore(&Buf_list_sem);
		    SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
			0, 0, 0, &erlog, 1, 
			sizeof(cur_buf->cut_ircb.cut_buf_name), 
			cur_buf->cut_ircb.cut_buf_name);
		    return (E_DB_ERROR);
		}
		/* check for me */
		cur_thread = cur_buf->cut_thread_list;
		connected = FALSE;
		do
		{
		    if (cur_thread->cut_tcb->cut_thread_id == sid)
		    {
			connected = TRUE;
			break;
		    }
		    cur_thread = cur_thread->buf_next;
		} while (cur_thread != cur_buf->cut_thread_list);
		if (connected)
		{
		    /* I'm connected to this buffer */
		    att_buf = TRUE;
		    break;
		}
		CSv_semaphore(&cur_buf->cut_buf_sem);
		cur_buf = cur_buf->buf_next;
	    } while (cur_buf != Buf_list);
	    if (att_buf)
	    {
		/* I'm attach to cur_buf, return info into RCB */
		cut_rcb->cut_buf_handle = (PTR) cur_buf;
		cut_rcb->cut_thr_handle = (PTR) cur_thread;
		cut_rcb->cut_buf_use = cur_thread->cut_thread_use;
		if (cur_thread->cut_parent_thread)
		    cut_rcb->cut_buf_use |= CUT_BUF_PARENT;
		CSv_semaphore(&cur_buf->cut_buf_sem);
	    }
	    else
	    {
		SETDBERR(error, 0, I_CU0401_LAST_BUFFER);
		CSv_semaphore(&Buf_list_sem);
		return(E_DB_INFO);
	    }
	    CSv_semaphore(&Buf_list_sem);
	    break;
	}
	case CUT_INFO_THREAD:
	{
	    CUT_THREAD_INFO	*thread_info = (CUT_THREAD_INFO *)info_buf;
	    bool		thread_found = FALSE;

	    /* check buffer */
	    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
	    {
		erval = 0;
		SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 2, sizeof(i4), &erval, 9, "cut_info");
		return (E_DB_ERROR);
	    }

	    /* find my thread block */
	    CSget_sid(&sid);
	    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;

	    /* Sanity check before trying for mutex */
	    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
	    {
		SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
		return(E_DB_ERROR);
	    }
	    cl_stat = CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	    if (cl_stat != OK)
	    {
		CSv_semaphore(&cur_buf->cut_buf_sem);
		SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		    0, 0, 0, &erlog, 1, 
		    sizeof(cur_buf->cut_ircb.cut_buf_name), 
		    cur_buf->cut_ircb.cut_buf_name);
		return (E_DB_ERROR);
	    }

	    cur_thread = cur_buf->cut_thread_list;
	    do
	    {
		if (cur_thread->cut_tcb->cut_thread_id == sid)
		{
		    thread_found = TRUE;
		    break;
		}
		cur_thread = cur_thread->buf_next;
	    } while (cur_thread != cur_buf->cut_thread_list);
	    if (!thread_found)
	    {
		SETDBERR(error, 0, W_CU0302_THREAD_NOT_ATTACHED);
		status = E_DB_WARN;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(cur_buf->cut_ircb, *cut_rcb);
		cut_rcb->cut_thr_handle = (PTR)cur_thread;

		/* Only return info if caller wants it */
		if ( thread_info )
		{
		    thread_info->cut_thread_use = cur_thread->cut_thread_use;
		    thread_info->cut_thread_state = cur_thread->cut_thread_state;
		    thread_info->cut_current_cell = cur_thread->cut_current_cell;
		    thread_info->cut_thread_status = 
			cur_thread->cut_tcb->cut_thread_status;
		}
		if (cur_thread->cut_tcb->cut_thread_status != E_DB_OK)
		{
		    status = cur_thread->cut_tcb->cut_thread_status;
		    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
		}
	    }
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    break;
	}
    }
    return(status);
}

/*
** Name: cut_read_buf - read from attached buffer
**
** Description:
**	Reads the requested number of cells from the provided thread 
**	communications buffer. The data is returned in the location passed
**	and allocated by the caller. The caller must ensure that enough memory
**	has been allocated to contain the requested amount of data. The
**	caller must have previously attached to the buffer using the
**	cut_attach_buf call.
**
**	If there is not enough data in the buffer to provide the caller
**	with the cells, the caller will be suspended until the data becomes
**	available. The suspend can be over-ridden using the wait flag. If
**	the caller does not choose to wait, the amount of cells currently
**	available are returned in the num_cells parameter.
**
**	If there are no writers attached, the CUT_RW_WAIT flag is
**	ignored;  thus, the read will always return with a success or
**	warning indication.  (Assuming that there's no outstanding error.)
**	This happens even in the middle of a read.  So, if we're blocking
**	on a read, and all the writing threads declare that they are
**	done (or detach), the read will return.
**	Readers should use some mechanism (e.g. cut_event_wait) to assure
**	that there are some writers attached before reading starts.  If
**	this is not done, a read loop will spin very fast until a writer
**	manages to get attached!
**
** Inputs:
**	num_cells			Number of cells to read
**					(output only if DIRECT mode).
**	cut_rcb.cut_thr_handle		Thread's handle
**	cut_rcb.cut_buf_handle		Buffer handle
**	output_buf			Return address for data
**	flags				CUT_RW_NOFLAGS = no special action
**					CUT_RW_WAIT = wait if no data ready
**					CUT_RW_FLUSH = wakeup waiting writers
**					now, without waiting for wakeup point
**					CUT_RW_SYNC = wait even if no
**					writers have arrived yet.
**					CUT_RW_DIRECT = Return pointer
**					to first readable cell with
**					num_cells containing the
**					number of readable cells.
**
** Outputs:
**	num_cells			Number of cells read
**	output_buf			Returned data
**	error				Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	22-may-98 (stephenb)
**	    Created as stub.
**	28-may-98 (stephenb)
**	    First cut of code
**	3-jun-99 (stephenb)
**	    Check for interrupt return from CScnd_wait()
**	15-Jan-2004 (jenjo02)
**	    Add (CS_SID)NULL parm to CScnd_signal().
**	19-Apr-2004 (schka24)
**	    Allow buffer to empty partway before waking writers.
**	26-Apr-2004 (schka24)
**	    Don't block when there are no writers.
**	10-Aug-2004 (jenjo02)
**	    ...unless CUT_RW_SYNC. In that case, the reader
**	    is ahead of the writer(s) and wants to wait until
**	    one or more writers do something.
**	18-Aug-2004 (jenjo02)
**	    Implemented DIRECT mode.
**	29-Mar-2006 (kschendel)
**	    If we don't have-sem on the first trip thru the loop, AND
**	    if the scheduler decides to dislike us in the midst of the
**	    should-we-wait IF test,  AND if all writers finish up in that
**	    time period, we can return EOF when in fact there is still stuff
**	    that needs to be returned from the buffer.  Split up the IF
**	    test so that we for-sure check both avail and writers whilst
**	    semaphore protected.
**	21-Nov-2007 (jonj)
**	    Must wait reader to dispatch writer(s) if Ingres-threaded.
*/
DB_STATUS
cut_read_buf(
	i4		*num_cells,
	CUT_RCB		*cut_rcb,
	PTR		*output_buf,
	i4		flags,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    STATUS		cl_stat;
    i4			avail_cells;
    PTR			start_addr;
    PTR			buf_loc;
    i4			offset;
    CS_SID		sid;
    CUT_BUF_THREAD	*my_thread = NULL;
    CUT_BUF_THREAD	*cur_thread;
    i4			bytes_to_copy;
    DB_STATUS		status = E_DB_OK;
    i4			erlog, erval;
    i4			have_sem = FALSE;

    CLRDBERR(error);

    /*
    ** DIRECT mode reads return a pointer to the first CELL
    ** in "*output_buf" and the number of cells in "*num_cells".
    **
    ** non-DIRECT mode (move mode) caller must supply 
    ** pointer to where the cell data is to be returned 
    ** in "*output_buf" and the maximum number of those
    ** cells in "*num_cells". Note that *num_cells will be
    ** set to the number of cells actually read.
    */
    
    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 13, "cut_read_buf");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;

    /* consistency check */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }

    /* 
    ** consistency check for connected threads, this ensures
    ** that we didn't slip in after the last user has diconnected
    ** and the buffer is about to be removed from the list
    */
    if (cur_buf->cut_buf_info.cut_num_threads == 0)
    {
	SETDBERR(error, 0, E_CU0118_BAD_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
	    0, 0, 0, &erlog, 0); 
	return(E_DB_ERROR);
    }

    if (num_cells == NULL || 
	    (!(flags & CUT_RW_DIRECT) && *num_cells < 1))
    {
	SETDBERR(error, 0, E_CU010E_NO_CELLS);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 0);
	return(E_DB_ERROR);
    }

    if (output_buf == NULL || (*output_buf == NULL &&
				!(flags & CUT_RW_DIRECT)) )
    {
	SETDBERR(error, 0, E_CU010F_NO_OUTPUT_BUF);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 0);
	return(E_DB_ERROR);
    }

    /* If more cells requested than there are, reduce */
    if ( !(flags & CUT_RW_DIRECT) &&
        *num_cells > cur_buf->cut_ircb.cut_num_cells)
    {
	*num_cells = cur_buf->cut_ircb.cut_num_cells;
    }

    /*
    ** Match thread handle with buffer.
    ** If we can't, find myself in attached threads.
    */
    CSget_sid(&sid);

    if ( !(my_thread = (CUT_BUF_THREAD*)cut_rcb->cut_thr_handle) ||
	  my_thread->cut_int_buffer != cur_buf ||
	  my_thread->cut_tcb->cut_thread_id != sid )
    {
	CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;
	my_thread = NULL;
	cur_thread = cur_buf->cut_thread_list;
	do
	{
	    if (cur_thread->cut_tcb->cut_thread_id == sid)
	    {
		my_thread = cur_thread;
		break;
	    }
	    cur_thread = cur_thread->buf_next;
	} while (cur_thread != cur_buf->cut_thread_list);

	if (my_thread == NULL)
	{
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
		cur_buf->cut_ircb.cut_buf_name);
	    return(E_DB_ERROR);
	}
	/* Return thread handle so next time it'll be right */
	cut_rcb->cut_thr_handle = (PTR)my_thread;
    }

    /* If previous read was a DIRECT, advance the markers */
    if ( my_thread->cut_thread_state == CUT_READ )
    {
	/* Need mutex for ReadComplete */
	if ( !have_sem )
	    CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;
	ReadComplete(cur_buf, my_thread);
    }

    /* check thread status after ReadComplete */
    if ( status = my_thread->cut_tcb->cut_thread_status )
    {
	if ( have_sem )
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
	return(status);
    }

    /* set state */
    my_thread->cut_thread_state = CUT_READ;

    /* Copy this request's flags, clear cells read */
    my_thread->cut_flags = flags;
    my_thread->cut_num_cells = 0;


    /*
    ** Check for occupied cells
    */
    for (;;)
    {
	/* Only completed writes are eligible */
	avail_cells = FULL_CELLS(my_thread->cut_cell_ops, 
	    cur_buf->cut_buf_info.cut_cell_writes_comp);

	/* DIRECT mode means "all available cells" */
	if ( flags & CUT_RW_DIRECT )
	    *num_cells = avail_cells;

	
	/* If there's something to read... */
	if ( avail_cells > 0 && have_sem )
	{
	    /* ...and the buffer is being flushed */
	    if ( my_thread->cut_thread_flush )
	    {
		/* ...and the caller has more room than cells */
		if ( *num_cells > avail_cells )
		{
		    /* ...then we'll complete the flush in this call */
		    my_thread->cut_thread_flush = FALSE;
		    /* WARNING: less cells than requested */
		    SETDBERR(error, 0, W_CU0306_LESS_CELLS_READ);
		    status = E_DB_WARN;
		    *num_cells = avail_cells;
		}
		/* ...otherwise, return what we can, stay in flush */
	    }
	}
	
	/*
	** If nothing to read yet, wait if wait flags set.  We'll need
	** to make sure we have the buffer sem before we make the final
	** decision to wait or not.
	** (If the caller doesn't care about waiting, presumably it has
	** something clever in mind, so don't worry about the semaphore.)
	*/
	if ( (avail_cells == 0 || avail_cells < *num_cells) &&
   	  (flags & (CUT_RW_SYNC | CUT_RW_WAIT)) )
	{
	    if ( !have_sem )
	    {
		/* Take sem, check again */
		CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
		have_sem = TRUE;
		continue;
	    }
	    /* Now that we're sure that avail and writers are consistent,
	    ** check wait condition further.  Don't wait if there are no
	    ** writers, unless CUT_RW_SYNC.  (The latter being a shortcut
	    ** for waiting until a writer attaches.)
	    **
	    ** But we must wait if Ingres-threaded to give the
	    ** writer(s) a chance to run.
	    **
	    ** If no writers have ever attached and Ingres threads,
	    ** then must wait regardless of CUT_RW_SYNC setting.
	    */
	    if ( (cur_buf->cut_buf_writers == 0 && 
		  (flags & CUT_RW_SYNC || 
		      (!CutIsMT && !cur_buf->cut_writers_attached)))
		||
		 (cur_buf->cut_buf_writers > 0 &&
		  (flags & CUT_RW_WAIT || !CutIsMT)) )
	    {
		cur_buf->cut_buf_info.cut_read_waits++;
		/*
		** release sem and wait for cells to be written
		** Set reader wakeup point to 3/4 of the buffer (hardwired
		** for now).  In case of multiple readers, be careful to
		** not move the wakeup point forward if it's already set.
		*/
		if (cur_buf->cut_buf_rwakeup == 0)
		{
		    /*
		    ** If some cells are available but not as many as
		    ** the caller wants, the set the wakeup to what
		    ** the caller wants.
		    */
		    if ( avail_cells > 0 )
			cur_buf->cut_buf_rwakeup = 
			    my_thread->cut_cell_ops + *num_cells;
		    else
		    {
			offset = (cur_buf->cut_ircb.cut_num_cells * 3) / 4;
			if (offset == 0)
			    offset = 1;
			cur_buf->cut_buf_rwakeup =
				my_thread->cut_cell_ops + offset;
		    }
		}
		/* Note that this thread is waiting for cells */
		SETDBERR(error, 0, W_CU0305_NO_CELLS_READ);
		cl_stat = CScnd_wait(&cur_buf->cut_write_cond, 
			    &cur_buf->cut_buf_sem);
		CLRDBERR(error);
		if ( cl_stat == E_CS000F_REQUEST_ABORTED ||
		     my_thread->cut_tcb->cut_thread_status != E_DB_OK )
		{
		    /* caused by an interrupt (possibly the result of a
		    ** cut_signal_status(), return async status */
		    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
		    my_thread->cut_thread_state = 0;
		    my_thread->cut_thread_flush = FALSE;
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    return( (my_thread->cut_tcb->cut_thread_status)
			   ? my_thread->cut_tcb->cut_thread_status
			   : E_DB_ERROR );
		}
		else if (cl_stat != OK)
		{
		    my_thread->cut_thread_state = 0;
		    my_thread->cut_thread_flush = FALSE;
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    SETDBERR(error, 0, E_CU0112_BAD_WRITE_CONDITION);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
			    0, 0, 0, &erlog, 1, 
			    sizeof(cur_buf->cut_ircb.cut_buf_name),
			    cur_buf->cut_ircb.cut_buf_name);
		    return(E_DB_ERROR);
		}
		/* Keep cut_buf_sem, loop back to check again */
	    }
	    else
		break;
	}
	else
	    break;
    } /* for (;;) */

    if (avail_cells == 0)
    {
	/* nothing to read, release sem and return */
	SETDBERR(error, 0, W_CU0305_NO_CELLS_READ);
	*num_cells = 0;
	my_thread->cut_thread_state = 0;
	my_thread->cut_thread_flush = FALSE;
	if ( have_sem )
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	return(E_DB_WARN);
    }
    else if ( avail_cells < *num_cells )
    {
	/* WARNING: less cells than requested */
	SETDBERR(error, 0, W_CU0306_LESS_CELLS_READ);
	status = E_DB_WARN;
	*num_cells = avail_cells;
    }

    /* Remember number of cells read this call */
    my_thread->cut_num_cells = *num_cells;

    if ( have_sem && (*num_cells > 1 || flags & CUT_RW_DIRECT) )
    {
	/* Release buf_sem while we copy the cells */
	have_sem = FALSE;
	CSv_semaphore(&cur_buf->cut_buf_sem);
    }

    /*
    ** O.K. so now we have enough to complete the read
    */
    start_addr = (char*)cur_buf + sizeof(CUT_INT_BUF) +
	(my_thread->cut_current_cell * cur_buf->cut_ircb.cut_cell_size);

    /*
    ** If DIRECT read, return pointer to first readable cell
    ** and number of cells.
    */
    if ( flags & CUT_RW_DIRECT )
    {
	/*
	** If read wraps around buffer, only return up to
	** end of buffer.
	*/
	if ( my_thread->cut_current_cell + *num_cells >
	     cur_buf->cut_ircb.cut_num_cells )
	{
	    *num_cells = cur_buf->cut_ircb.cut_num_cells -
			my_thread->cut_current_cell;
	    my_thread->cut_num_cells = *num_cells;
	}

	/*
	** Note that this thread's "cut_current_cell" does
	** not get advanced until the next cut_read_buf,
	** signalling that the reader has finished with
	** all the cells.
	*/
	*output_buf = start_addr;
	/* thread_state still set to CUT_READ */
	return(status);
    }

    if (my_thread->cut_current_cell + *num_cells
	> cur_buf->cut_ircb.cut_num_cells)
    {
	/* read wraps around buffer, copy first chunk */
	bytes_to_copy = (cur_buf->cut_ircb.cut_num_cells - 
	    my_thread->cut_current_cell) *
	    cur_buf->cut_ircb.cut_cell_size;
	MEcopy(start_addr, bytes_to_copy, *output_buf);
	buf_loc = *output_buf + bytes_to_copy;
	start_addr = (char*)cur_buf + sizeof(CUT_INT_BUF);
	bytes_to_copy = (*num_cells * cur_buf->cut_ircb.cut_cell_size)
	    - bytes_to_copy;
    }
    else
    {
	bytes_to_copy = *num_cells * cur_buf->cut_ircb.cut_cell_size;
	buf_loc = *output_buf;
    }

    /* copy the rest */
    MEcopy(start_addr, bytes_to_copy, buf_loc);

    /* Need buf_sem for ReadComplete */
    if ( !have_sem )
	CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
 
    ReadComplete(cur_buf, my_thread);
    /*
    ** release sem
    */
    CSv_semaphore(&cur_buf->cut_buf_sem);

    return(status);
}

/*
** Name: cut_read_complete - External wrapper to ReadComplete
**
** Description:
**	Concludes a direct-mode read by advancing the read
**	markers in the CUT buffer.
**
** Inputs:
**	cut_rcb.cut_thr_handle		Thread's handle
**	cut_rcb.cut_buf_handle		Buffer handle
**
** Outputs:
**	error				Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	02-Sep-2004 (jenjo02)
**	    Created for read DIRECT
*/
DB_STATUS
cut_read_complete(
	CUT_RCB		*cut_rcb,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    CS_SID		sid;
    CUT_BUF_THREAD	*my_thread = NULL;
    CUT_BUF_THREAD	*cur_thread;
    DB_STATUS		status = E_DB_OK;
    i4			erlog, erval;
    i4			have_sem = FALSE;

    CLRDBERR(error);
   /* check parms */
    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 17, "cut_read_complete");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;

    /* consistency check */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }

    /* 
    ** consistency check for connected threads, this ensures
    ** that we didn't slip in after the last user has diconnected
    ** and the buffer is about to be removed from the list
    */
    if (cur_buf->cut_buf_info.cut_num_threads == 0)
    {
	SETDBERR(error, 0, E_CU0118_BAD_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
	    0, 0, 0, &erlog, 0); 
	return(E_DB_ERROR);
    }

    /*
    ** Match thread handle with buffer.
    ** If we can't, find myself in attached threads.
    */
    CSget_sid(&sid);

    if ( !(my_thread = (CUT_BUF_THREAD*)cut_rcb->cut_thr_handle) ||
	  my_thread->cut_int_buffer != cur_buf ||
	  my_thread->cut_tcb->cut_thread_id != sid )
    {
	CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;
	my_thread = NULL;
	cur_thread = cur_buf->cut_thread_list;
	do
	{
	    if (cur_thread->cut_tcb->cut_thread_id == sid)
	    {
		my_thread = cur_thread;
		break;
	    }
	    cur_thread = cur_thread->buf_next;
	} while (cur_thread != cur_buf->cut_thread_list);
	if (my_thread == NULL)
	{
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
		cur_buf->cut_ircb.cut_buf_name);
	    return(E_DB_ERROR);
	}
	/* Return thread handle so next time it'll be right */
	cut_rcb->cut_thr_handle = (PTR)my_thread;
    }

    /* If last read incomplete, advance the markers */
    if ( my_thread->cut_thread_state == CUT_READ )
    {
	/* Need mutex for ReadComplete */
	if ( !have_sem )
	    CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;
	ReadComplete(cur_buf, my_thread);

	/* check thread status after ReadComplete */
	if ( status = my_thread->cut_tcb->cut_thread_status )
	    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
    }

    if ( have_sem )
	CSv_semaphore(&cur_buf->cut_buf_sem);

    return(status);
}


/*
** Name: ReadComplete - Update markers after cut_buf_read
**		        completion.
**
** Description:
**	Updates current thread and its buffers after 
**	a DIRECT or move-mode read.
**
** Inputs:
**	cur_buf				The buffer read from.
**	my_thread			The involved thread.
**
** Outputs:
**	none.
**
** Returns:
**	VOID
**
** History:
**	18-Aug-2004 (jenjo02)
**	    Extracted from cut_read_buf for DIRECT mode reads.
*/
static VOID
ReadComplete(
CUT_INT_BUF	*cur_buf,
CUT_BUF_THREAD	*my_thread)
{
    CUT_BUF_THREAD	*cur_thread;
    i4			offset, tail_offset;

    /* Caller must hold cur_buf->cut_buf_sem */
 
    /* Update my_thread's current cell, cell_ops */
    my_thread->cut_current_cell = (my_thread->cut_current_cell + 
				   my_thread->cut_num_cells) % 
			    cur_buf->cut_ircb.cut_num_cells;
    my_thread->cut_cell_ops += my_thread->cut_num_cells;

    /* This thread's read is complete */
    my_thread->cut_thread_state = 0;

    /*
    ** can we move the tail forward
    */
    cur_thread = cur_buf->cut_thread_list;
    tail_offset = MAXI4;

    do
    {
	if ( cur_thread == my_thread || 
		cur_thread->cut_thread_use == CUT_BUF_READ )
	{
	    offset = CELL_OFFSET(cur_buf->cut_buf_info.cut_cell_reads,
		cur_thread->cut_cell_ops);
	    if (offset < tail_offset)
		tail_offset = offset;
	    if (tail_offset == 0 &&
		    (my_thread->cut_flags & CUT_RW_FLUSH) == 0 )
		break;			/* No point in looking further */
	}
	/* Not sure why a reader would want to flush and tell the writers... */
	else if ( my_thread->cut_flags & CUT_RW_FLUSH &&
		    cur_thread->cut_thread_use == CUT_BUF_WRITE )
	    cur_thread->cut_thread_flush = TRUE;

	cur_thread = cur_thread->buf_next;
    } while (cur_thread != cur_buf->cut_thread_list);

    if (tail_offset && tail_offset != MAXI4)
    {
        cur_buf->cut_buf_info.cut_buf_tail = 
	    (cur_buf->cut_buf_info.cut_buf_tail + tail_offset) % 
	    cur_buf->cut_ircb.cut_num_cells;
	cur_buf->cut_buf_info.cut_cell_reads += tail_offset;

        /*
	** If it's time to wake waiting writers, do so.
	** Wake 'em all up and let them rip.  (More than one can be
	** waiting, and there's only one wakeup point, so it's essential
	** to wake them all.)
        */
	if ((cur_buf->cut_buf_wwakeup > 0
	    && cur_buf->cut_buf_info.cut_cell_reads >= cur_buf->cut_buf_wwakeup)
	  || my_thread->cut_flags & CUT_RW_FLUSH)
	{
	    cur_buf->cut_buf_wwakeup = 0;
	    CScnd_broadcast(&cur_buf->cut_read_cond);
	}
    }
    else
	/* ...should never happen */
	CS_breakpoint();
    /* Returns with cut_buf_sem held */
    return;
}

/*
** Name: cut_write_buf - Write to attached buffer
**
** Description:
**	Wride data to the provided thread communications buffer. The caller
**	must have previously attached to the buffer using cut_attach_buf.
**	This function expects to see the data to write at the passed address
**	which is expected to be large enough to contain the data to write.
**
**	If there is not enough space in the buffer to write the requested
**	cells, the caller will suspend until the space becomes available. The
**	suspend may be over-ridden with the wait flag. If the caller has
**	requested not to wait, this function will return as many cells
**	as are available.
**
** Inputs:
**	num_cells			Number of cells to write
**	cut_rcb.cut_thr_handle		Thread's handle
**	cut_rcb.cut_buf_handle		Buffer handle
**	input_buf			Pointer to input buffer
**	flags				CUT_RW_NOFLAGS = no special action
**					CUT_RW_WAIT = wait if no space in buF
**					CUT_RW_FLUSH = wakeup waiting readers
**					now, without waiting for wakeup point
**					(use e.g. if this is last write)
**					CUT_RW_DIRECT = caller will
**					write directly in the CUT
**					buffer.
**
** Outputs:
**	num_cells			Number of cells written
**	error				Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	22-may-98 (stephenb) 
**	    Created as stub
**	11-jun-98 (stephenb)
**	    First cut of code
**	3-jun-99 (stephenb)
**	    Check for interrupt return from CScnd_wait()
**	19-Apr-2004 (schka24)
**	    Allow buffer to partly fill before waking readers.
**	18-Aug-2004 (jenjo02)
**	    If some, but not all of *num_cells available,
**	    set wakeup to what the caller wants.
**	04-Sep-2004 (jenjo02)
**	    Added concept of DIRECT mode writes in which
**	    the caller is returned a pointer to some number
**	    of writable cells.
*/
DB_STATUS
cut_write_buf(
	i4		*num_cells,
	CUT_RCB		*cut_rcb,
	PTR		*input_buf,
	i4		flags,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    STATUS		cl_stat;
    CS_SID		sid;
    CUT_BUF_THREAD	*my_thread, *cur_thread;
    i4			free_cells;
    i4			offset;
    DB_STATUS		status = E_DB_OK;
    PTR			start_addr;
    PTR			buf_loc;
    i4			bytes_to_copy;
    i4			erlog, erval;

    /*
    ** check parms
    */
    CLRDBERR(error);

    /* 
    ** For DIRECT mode writes, caller intends to write
    ** num_cells directly into the CUT buffer.
    ** For that, we allot that number, or less, contiguous
    ** cells to the thread and return a pointer to the
    ** first cell in "*input_buf".
    **
    ** When the caller has filled those cells, it must
    ** either issue another cut_write_buf or call
    ** cut_write_complete to release the cells to the
    ** reader(s).
    */

    if (*num_cells < 1)
    {
	SETDBERR(error, 0, E_CU010E_NO_CELLS);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 0);
	return(E_DB_ERROR);
    }
    if (input_buf == NULL || (*input_buf == NULL &&
				!(flags & CUT_RW_DIRECT)) )
    {
	SETDBERR(error, 0, E_CU0013_NO_INPUT_BUF);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 0);
	return(E_DB_ERROR);
    }
    
    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 14, "cut_write_buf");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    /*
    ** consistency check
    */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }

    if (*num_cells > cur_buf->cut_ircb.cut_num_cells)
	*num_cells = cur_buf->cut_ircb.cut_num_cells;

    /* 
    ** consistency check for connected threads, this ensures
    ** that we didn't slip in after the last user has diconnected
    ** and the buffer is about to be removed from the list
    */

    if (cur_buf->cut_buf_info.cut_num_threads == 0)
    {
	SETDBERR(error, 0, E_CU0118_BAD_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
	    0, 0, 0, &erlog, 0); 
	return(E_DB_ERROR);
    }

    /*
    ** Match thread handle with buffer.
    ** If we can't, find myself in attached threads.
    */
    CSget_sid(&sid);

    /* Blast. Every write is going to block right here... */
    CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);

    if ( !(my_thread = (CUT_BUF_THREAD*)cut_rcb->cut_thr_handle) ||
	  my_thread->cut_int_buffer != cur_buf ||
	  my_thread->cut_tcb->cut_thread_id != sid )
    {
	/*
	** find my slot in attached threads
	*/
	my_thread = NULL;
	cur_thread = cur_buf->cut_thread_list;
	do
	{
	    if (cur_thread->cut_tcb->cut_thread_id == sid)
	    {
		/* that's me */
		my_thread = cur_thread;
		break;
	    }
	    cur_thread = cur_thread->buf_next;
	} while (cur_thread != cur_buf->cut_thread_list);

	if ( (cut_rcb->cut_thr_handle = (PTR)my_thread) == NULL )
	{
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	    return(E_DB_ERROR);
	}
	/* Return thread handle so next time it'll be right */
	cut_rcb->cut_thr_handle = (PTR)my_thread;
    }

    /* If incomplete write, call WriteComplete */
    if ( my_thread->cut_thread_state == CUT_WRITE )
	WriteComplete(cur_buf, my_thread);

    /* check thread status */
    if ( status = my_thread->cut_tcb->cut_thread_status )
    {
	*num_cells = 0;
	SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
	CSv_semaphore(&cur_buf->cut_buf_sem);
	return(status);
    }

    /* set state */
    my_thread->cut_thread_state = CUT_WRITE;

    /* Copy this request's flags, clear cells written */
    my_thread->cut_flags = flags;
    my_thread->cut_num_cells = 0;

    /*
    ** check for free cells
    **
    ** Free cells occur between the last initiated write and
    ** the last completed read.
    */
    for (;;)
    {
	free_cells = EMPTY_CELLS(cur_buf->cut_ircb.cut_num_cells,
				 cur_buf->cut_buf_info.cut_cell_reads,
				 cur_buf->cut_buf_info.cut_cell_writes);

	if ( free_cells == 0 )
	{
	    /* nothing to write */
	    if (flags & CUT_RW_WAIT)
	    {
		cur_buf->cut_buf_info.cut_write_waits++;

		/* release sem and wait for cells to be read.
		** Set writer wakeup point to 3/4 of the buffer (hardwired
		** for now).  In case of multiple writers, be careful to
		** not move the wakeup point forward if it's already set.
		*/
		if (cur_buf->cut_buf_wwakeup == 0)
		{
		    /*
		    ** If some cells are available but not as many as
		    ** the caller wants, the set the wakeup to what
		    ** the caller wants.
		    */
		    if ( *num_cells > 1 )
			cur_buf->cut_buf_wwakeup = 
			    cur_buf->cut_buf_info.cut_cell_reads + *num_cells;
		    else
		    {
			offset = (cur_buf->cut_ircb.cut_num_cells * 3) / 4;
			if (offset == 0)
			    offset = 1;
			cur_buf->cut_buf_wwakeup =
			    cur_buf->cut_buf_info.cut_cell_reads + offset;
		    }
		}
		cl_stat = CScnd_wait(&cur_buf->cut_read_cond, 
		    &cur_buf->cut_buf_sem);
		if ( cl_stat == E_CS000F_REQUEST_ABORTED ||
		     my_thread->cut_tcb->cut_thread_status != E_DB_OK )
		{
		    /* caused by an interrupt (possibly the result of a
		    ** cut_signal_status(), return async status */
		    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
		    my_thread->cut_thread_state = 0;
		    *num_cells = 0;
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    return( (my_thread->cut_tcb->cut_thread_status)
			   ? my_thread->cut_tcb->cut_thread_status
			   : E_DB_ERROR );
		}
		else if (cl_stat != OK)
		{
		    SETDBERR(error, 0, E_CU0114_BAD_READ_CONDITION);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
			0, 0, 0, &erlog, 1, 
			sizeof(cur_buf->cut_ircb.cut_buf_name),
			cur_buf->cut_ircb.cut_buf_name);
		    my_thread->cut_thread_state = 0;
		    *num_cells = 0;
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    return(E_DB_ERROR);
		}
		/* Loop back to try again */
		continue;
	    }
	    else
	    {
		/* no space to write, just return without doing anything */
		/* WARNING: No writes done */
		*num_cells = 0;
		SETDBERR(error, 0, W_CU0307_NO_CELLS_WRITTEN);
		my_thread->cut_thread_state = 0;
		CSv_semaphore(&cur_buf->cut_buf_sem);
		return(E_DB_WARN);
	    }
	}
	else if ( free_cells < *num_cells )
	{
	    *num_cells = free_cells;
	    SETDBERR(error, 0, W_CU0308_LESS_CELLS_WRITTEN);
	    status = E_DB_WARN;
	}
	break;
    }

    /* Remember where this thread's write started */
    my_thread->cut_cell_ops = cur_buf->cut_buf_info.cut_cell_writes;
    /* ...and the number of cells */
    my_thread->cut_num_cells = *num_cells;

    /*
    ** O.K. So now we have enough free cells to write
    */
    start_addr = (char*)cur_buf + sizeof(CUT_INT_BUF) +
		    (cur_buf->cut_buf_info.cut_buf_head *
			cur_buf->cut_ircb.cut_cell_size);

    /*
    ** If DIRECT mode, return pointer to the first allotted
    ** cell and the number of cells.
    */
    if ( flags & CUT_RW_DIRECT )
    {
	if (cur_buf->cut_buf_info.cut_buf_head + *num_cells > 
	    cur_buf->cut_ircb.cut_num_cells)
	{
	    /* Cells wrap buffer, reduce num_cells */
	    *num_cells = cur_buf->cut_ircb.cut_num_cells -
			 cur_buf->cut_buf_info.cut_buf_head;
	    my_thread->cut_num_cells = *num_cells;
	}

	/* move buffer head */
	cur_buf->cut_buf_info.cut_buf_head = 
		(cur_buf->cut_buf_info.cut_buf_head + *num_cells) 
			% cur_buf->cut_ircb.cut_num_cells;

	/* Advance initiated writes */
	cur_buf->cut_buf_info.cut_cell_writes += *num_cells;

	*input_buf = start_addr;
	/* thread_state remains set to CUT_WRITE */
    }
    else
    {
	/* MOVE mode; copy caller's data to cells */
	if (cur_buf->cut_buf_info.cut_buf_head + *num_cells > 
		    cur_buf->cut_ircb.cut_num_cells)
	{
	    /* write wraps around buffer, copy first chunk */
	    bytes_to_copy = (cur_buf->cut_ircb.cut_num_cells -
				cur_buf->cut_buf_info.cut_buf_head) *
				cur_buf->cut_ircb.cut_cell_size;
	    MEcopy(*input_buf, bytes_to_copy, start_addr);
	    buf_loc = *input_buf + bytes_to_copy;
	    start_addr = (char*)cur_buf + sizeof(CUT_INT_BUF);
	    bytes_to_copy = (*num_cells * cur_buf->cut_ircb.cut_cell_size)
		- bytes_to_copy;
	}
	else
	{
	    bytes_to_copy = *num_cells * cur_buf->cut_ircb.cut_cell_size;
	    buf_loc = *input_buf;
	}

	/* copy the rest */
	MEcopy(buf_loc, bytes_to_copy, start_addr);

	/* move buffer head */
	cur_buf->cut_buf_info.cut_buf_head = 
		(cur_buf->cut_buf_info.cut_buf_head + *num_cells) 
			% cur_buf->cut_ircb.cut_num_cells;

	/* Advance initiated writes */
	cur_buf->cut_buf_info.cut_cell_writes += *num_cells;

	/* Our write's complete */
	WriteComplete(cur_buf, my_thread);
    }

    /*
    ** release sem
    */
    CSv_semaphore(&cur_buf->cut_buf_sem);

    return(status);
}

/*
** Name: cut_write_complete - External wrapper for WriteComplete
**
** Description:
**	Concludes a direct-mode write by advancing the write
**	markers in the CUT buffer.
**
** Inputs:
**	cut_rcb.cut_thr_handle		Thread's handle
**	cut_rcb.cut_buf_handle		Buffer handle
**
** Outputs:
**	error				Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	02-Sep-2004 (jenjo02)
**	    Created for write DIRECT
*/
DB_STATUS
cut_write_complete(
	CUT_RCB		*cut_rcb,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    CS_SID		sid;
    CUT_BUF_THREAD	*my_thread = NULL;
    CUT_BUF_THREAD	*cur_thread;
    DB_STATUS		status = E_DB_OK;
    i4			erlog, erval;
    i4			have_sem = FALSE;

    CLRDBERR(error);
   /* check parms */
    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 18, "cut_write_complete");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;

    /* consistency check */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }

    /* 
    ** consistency check for connected threads, this ensures
    ** that we didn't slip in after the last user has diconnected
    ** and the buffer is about to be removed from the list
    */
    if (cur_buf->cut_buf_info.cut_num_threads == 0)
    {
	SETDBERR(error, 0, E_CU0118_BAD_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
	    0, 0, 0, &erlog, 0); 
	return(E_DB_ERROR);
    }

    /*
    ** Match thread handle with buffer.
    ** If we can't, find myself in attached threads.
    */
    CSget_sid(&sid);

    if ( !(my_thread = (CUT_BUF_THREAD*)cut_rcb->cut_thr_handle) ||
	  my_thread->cut_int_buffer != cur_buf ||
	  my_thread->cut_tcb->cut_thread_id != sid )
    {
	CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;
	my_thread = NULL;
	cur_thread = cur_buf->cut_thread_list;
	do
	{
	    if (cur_thread->cut_tcb->cut_thread_id == sid)
	    {
		my_thread = cur_thread;
		break;
	    }
	    cur_thread = cur_thread->buf_next;
	} while (cur_thread != cur_buf->cut_thread_list);
	if (my_thread == NULL)
	{
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
		cur_buf->cut_ircb.cut_buf_name);
	    return(E_DB_ERROR);
	}
	/* Return thread handle so next time it'll be right */
	cut_rcb->cut_thr_handle = (PTR)my_thread;
    }

    /* If previous write was a DIRECT, advance the markers */
    if ( my_thread->cut_thread_state == CUT_WRITE )
    {
	/* Need mutex for WriteComplete */
	if ( !have_sem )
	    CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	have_sem = TRUE;

	/* Inform readers these writes are done */
	my_thread->cut_flags |= CUT_RW_FLUSH;
	WriteComplete(cur_buf, my_thread);

	/* check thread status after WriteComplete */
	if ( status = my_thread->cut_tcb->cut_thread_status )
	    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
    }

    if ( have_sem )
	CSv_semaphore(&cur_buf->cut_buf_sem);

    return(status);
}

/*
** Name: WriteComplete - Update markers after cut_write_buf
**		         completion.
**
** Description:
**	Moves "writes_comp" forward at the completion of
**	a DIRECT- or move-mode write when this write is
**	the earliest incomplete write.
**
** Inputs:
**	cur_buf				The buffer written to.
**	my_thread			The involved thread.
**
** Outputs:
**	none.
**
** Returns:
**	VOID
**
** History:
**	25-Aug-2004 (jenjo02)
**	    Created for DIRECT mode writes.
**      08-Sep-2007 (thaju02)
**          Add memory barrier/fence call. (b119100)
*/
static VOID
WriteComplete(
CUT_INT_BUF	*cur_buf,
CUT_BUF_THREAD	*my_thread)
{
    CUT_BUF_THREAD	*cur_thread;

    /* Caller must hold cur_buf->cut_buf_sem */
 
    /* Reset state, our write's complete */
    my_thread->cut_thread_state = 0;

    /* cut_cell_ops is "writes" when initiated */
    /* cut_num_cells is the number of cells written */

    if ( cur_buf->cut_buf_info.cut_cell_writes_comp ==
	     my_thread->cut_cell_ops )
    {
	/*
	** Then this is the write adjacent to the last-completed
	** write, move write_comp forward, releasing the
	** cells for readers.
	*/

	if ( cur_buf->cut_trace )
	    TRdisplay("%@ %x WriteComplete was %d now %d\n",
		my_thread->cut_tcb->cut_thread_id,
		cur_buf->cut_buf_info.cut_cell_writes_comp,
		cur_buf->cut_buf_info.cut_cell_writes_comp +
		    my_thread->cut_num_cells);

	CS_MEMBAR_MACRO();
	cur_buf->cut_buf_info.cut_cell_writes_comp +=
		my_thread->cut_num_cells;

	/*
	** If flushing, or it's time to wake waiting readers, do so.
	** Wake 'em all up and let them rip.  (More than one can be
	** waiting, and there's only one wakeup point, so it's essential
	** to wake them all.)
	*/
	if ( my_thread->cut_flags & CUT_RW_FLUSH ||
		(cur_buf->cut_buf_rwakeup > 0
		    && cur_buf->cut_buf_info.cut_cell_writes_comp >= 
			cur_buf->cut_buf_rwakeup) )
	{
	    /* If flushing, tell the reader(s) */
	    if ( my_thread->cut_flags & CUT_RW_FLUSH )
	    {
		cur_thread = cur_buf->cut_thread_list;
		do
		{
		    if (cur_thread->cut_thread_use == CUT_BUF_READ)
			cur_thread->cut_thread_flush = TRUE;
		    cur_thread = cur_thread->buf_next;
		} while ( cur_thread != cur_buf->cut_thread_list );
	    }
	    cur_buf->cut_buf_rwakeup = 0;
	    CScnd_broadcast(&cur_buf->cut_write_cond);
	}
    }
    else
    {
	/* 
	** writes_comp can't be moved forward until
	** the earliest writer completes, and we're not
	** the earliest writer.
	*/
	i4	found = FALSE;

	cur_thread = cur_buf->cut_thread_list;

	/*
	** Find the active writer that's just ahead of us
	** and give it our num_cells.
	**
	** Waiting writers' cut_num_cells will be zero,
	** which we'll skip over.
	*/

	do
	{
	    if ( cur_thread->cut_thread_state == CUT_WRITE &&
		 cur_thread->cut_num_cells > 0 &&
		 cur_thread->cut_cell_ops + cur_thread->cut_num_cells
		     == my_thread->cut_cell_ops )
	    {
		if ( cur_buf->cut_trace )
		    TRdisplay("%@ %x WriteComplete gave %d to %x (%d,%d)\n",
			my_thread->cut_tcb->cut_thread_id,
			my_thread->cut_num_cells,
			cur_thread->cut_tcb->cut_thread_id,
			cur_thread->cut_cell_ops,
			cur_thread->cut_num_cells + 
			    my_thread->cut_num_cells);

		found = TRUE;

		cur_thread->cut_num_cells += 
		    my_thread->cut_num_cells;
		break;
	    }
	    cur_thread = cur_thread->buf_next;
	} while (cur_thread != cur_buf->cut_thread_list);

	/* This should never happen... */
	if ( !found )
	{
	    TRdisplay("%@ %x WriteComplete prev not found (%d,%d) WC %d\n",
			my_thread->cut_tcb->cut_thread_id,
			my_thread->cut_cell_ops,
			my_thread->cut_num_cells,
			cur_buf->cut_buf_info.cut_cell_writes_comp);
	    cur_thread = cur_buf->cut_thread_list;
	    do
	    {
		if ( cur_thread->cut_thread_state == CUT_WRITE )
		{
		    TRdisplay("%@ %x CUT_WRITE (%d,%d)\n",
			cur_thread->cut_tcb->cut_thread_id,
			cur_thread->cut_cell_ops,
			cur_thread->cut_num_cells);
		}
		cur_thread = cur_thread->buf_next;
	    } while (cur_thread != cur_buf->cut_thread_list);
	    CS_breakpoint();
	}
    }

    /* Returns with cut_buf_sem held */
    return;
}

/*
** Name: cut_writer_done - Writer thread is done writing
**
** Description:
**	The "obvious" implementation of reading from a CUT buffer
**	has a problem, in that there's no way to get out of a blocking
**	read if EOF has been reached.  (Other than by stuffing some
**	kind of dummy record through the buffer, that says "I am EOF".)
**	Since blocking reads are of course very useful, a cleaner
**	solution to the EOF problem is desirable.
**
**	The solution is to take an idea from unix fifos.  A read on
**	a fifo will return something (0, as it happens) if there is
**	nobody connected to the fifo for writing -- even if the read
**	was originally issued in blocking mode.  So, that's what
**	cut-read-buf does.
**
**	Now, it turns out to be inconvenient to require that a writer
**	actually *detach* from the CUT buffer just to indicate that it's
**	done writing.  The CUT mechanisms are very useful for status
**	and statistics gathering (cut_info), as well as thread sync
**	(cut_event_wait).  A way for a writer to say "I am done writing
**	and will never ever ever write to this CUT buffer again" without
**	detaching would be useful.
**
**	That's what this routine does.  The caller must provide a buffer
**	handle, and the calling thread must be using the buffer in WRITE
**	use-mode.  We will decrement the number-of-writers count for
**	the buffer, and if it reaches zero we'll awaken all readers
**	who might be waiting on the buffer.  Those readers will notice
**	that nobody is left to write, and will do the proper thing
**	(return whatever rows are left, or return an E_DB_WARN if there's
**	nothing left to read.)
**
**	Note that cut_writer_done does not look at the buffer signal
**	status.  A writer-done will succeed even if there is a non-OK
**	status sitting on the buffer.
**
** Inputs:
**	cut_rcb				The cut request block
**	    .cut_buf_handle		Handle to the CUT buffer involved
**	error				An output
**
** Outputs:
**	Returns E_DB_xxx status
**	error				Set to CUT error status if any
**
** History:
**	26-Apr-2004 (schka24)
**	    Written.
**	26-Nov-2007 (jonj)
**	    If last writer, inform readers that nothing else is coming.
*/

DB_STATUS
cut_writer_done(CUT_RCB *cut_rcb, DB_ERROR *error)
{

    bool		thread_attached;
    CS_SID		sid;
    CUT_INT_BUF		*cur_buf;
    CUT_BUF_THREAD	*buf_thread;
    DB_STATUS		status;
    i4			erlog, erval;
    STATUS		cl_stat;

    /*
    ** check parms
    */
    CLRDBERR(error);
    CSget_sid(&sid);

    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 0, "cut_writer_done");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    /*
    ** consistency check
    */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }
    /* check buffer and grab sem */
    if (buffer_sem(cur_buf, FALSE, "cut_writer_done", error) != E_DB_OK)
    {
	/* error already logged */
	return(E_DB_ERROR);
    }
    /*
    ** find my slot in attached threads
    */
    buf_thread = cur_buf->cut_thread_list;
    do
    {
	if (buf_thread->cut_tcb->cut_thread_id == sid)
	{
	    thread_attached = TRUE;
	    /* that's me */
	    break;
	}
	buf_thread = buf_thread->buf_next;
    } while (buf_thread != cur_buf->cut_thread_list);
    if (!thread_attached)
    {
	CSv_semaphore(&cur_buf->cut_buf_sem);
	SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
	    cur_buf->cut_ircb.cut_buf_name);
	return(E_DB_ERROR);
    }

    /* If we're a "neither" thread, the caller already did this.  Don't
    ** do it again.  (This could be an error but I'll allow slack.)
    */
    if (buf_thread->cut_thread_use == CUT_BUF_NEITHER)
    {
	cl_stat = CSv_semaphore(&cur_buf->cut_buf_sem);
	return (E_DB_OK);
    }

    /* This is improper if we're a reading thread */
    if (buf_thread->cut_thread_use == CUT_BUF_READ)
    {
	cl_stat = CSv_semaphore(&cur_buf->cut_buf_sem);
	SETDBERR(error, 0, E_CU0103_BAD_BUFFER_USE);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 3, sizeof(i4), &buf_thread->cut_thread_use,
		sizeof(i4), 0, 0, "cut_writer_done");
	return (E_DB_ERROR);
    }

    /* Make our thread-use be "neither".  We're a zombie... */
    buf_thread->cut_thread_use = CUT_BUF_NEITHER;

    /* Decrement the writer count and wake up readers if there
    ** aren't any more writers.
    */
    if (--cur_buf->cut_buf_writers <= 0)
    {
	/* All writing is done.  Make sure readers are awake so that
	** they can read, and possibly return if there's nothing left.
	*/
	if ( cur_buf->cut_buf_info.cut_num_threads > 1)
	{
	    CUT_BUF_THREAD	*reader = cur_buf->cut_thread_list;

	    /* Inform readers that no more writes are coming */
	    do
	    {
		if (reader->cut_thread_use == CUT_BUF_READ)
		    reader->cut_thread_flush = TRUE;

		reader = reader->buf_next;
	    } while (reader != cur_buf->cut_thread_list);
	}
	cur_buf->cut_buf_rwakeup = 0;
	cl_stat = CScnd_broadcast(&cur_buf->cut_write_cond);
    }

    /*
    ** release sem
    */
    cl_stat = CSv_semaphore(&cur_buf->cut_buf_sem);

    return (E_DB_OK);

} /* cut_writer_done */

/*
** Name: cut_wakeup_waiters - Wake up waiting read/write threads
**
** Description:
**	The way CUT buffers data, readers may sleep until writers fill
**	the buffer to a certain level (and vice versa).  This works
**	great, unless no more writing is to be done!  If a writer knows
**	that it's the last write, a CUT_RW_FLUSH flag can be applied to
**	the last cut_write_buf call.  In other situations, it turns out
**	to be inconvenient to recognize "writing done" until later.
**	This routine was created so that a writer can poke all the
**	readers awake at any time, without having to write a dummy
**	record through the buffers.
**
**	For symmetry, the same function is implemented for readers who
**	want to wake up writers early for some reason.  The rule is
**	that if the calling thread is in READ use-mode, we wake up
**	WRITERS, and if the calling thread is in WRITE use-mod, we wake
**	up READERS.
**
**	Unlike actual reading or writing, this routine does not check
**	for "async status".  It only checks for parameter errors like
**	bad handle or thread not attached.
**
**	*NOTE* This routine was perhaps hastily conceived.  What you
**	may want is cut_writer_done instead, look there...
**	FIXME if nobody finds a use for this routine, delete it!
**	If you DO find a good use, please describe here, and remove this
**	comment... (Karl 28-apr-04)
**
** Inputs:
**	cut_rcb				The cut request block
**	    .cut_buf_handle		Handle to the CUT buffer involved
**	error				An output
**
** Outputs:
**	Returns E_DB_xxx status
**	error				Set to CUT error status if any
**
** History:
**	25-Apr-2004 (schka24)
**	    Written.
*/

DB_STATUS
cut_wakeup_waiters(CUT_RCB *cut_rcb, DB_ERROR *error)
{

    bool		thread_attached;
    CS_SID		sid;
    CUT_INT_BUF		*cur_buf;
    CUT_BUF_THREAD	*buf_thread;
    DB_STATUS		status;
    i4			erlog, erval;
    STATUS		cl_stat;

    /*
    ** check parms
    */
    CLRDBERR(error);
    CSget_sid(&sid);

    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 0, "cut_wakeup_waiters");
	return (E_DB_ERROR);
    }
    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    /*
    ** consistency check
    */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }
    /* check buffer and grab sem */
    if (buffer_sem(cur_buf, FALSE, "cut_wakeup_waiters", error) != E_DB_OK)
    {
	/* error already logged */
	return(E_DB_ERROR);
    }
    /*
    ** find my slot in attached threads
    */
    buf_thread = cur_buf->cut_thread_list;
    do
    {
	if (buf_thread->cut_tcb->cut_thread_id == sid)
	{
	    thread_attached = TRUE;
	    /* that's me */
	    break;
	}
	buf_thread = buf_thread->buf_next;
    } while (buf_thread != cur_buf->cut_thread_list);
    if (!thread_attached)
    {
	CSv_semaphore(&cur_buf->cut_buf_sem);
	SETDBERR(error, 0, E_CU0111_NOT_ATTACHED);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
	    cur_buf->cut_ircb.cut_buf_name);
	return(E_DB_ERROR);
    }

    /* Wake up readers or writers as appropriate */
    if (buf_thread->cut_thread_use == CUT_BUF_WRITE
      || buf_thread->cut_thread_use == CUT_BUF_NEITHER)
    {
	/* Wakeup readers */
	cur_buf->cut_buf_rwakeup = 0;
	cl_stat = CScnd_broadcast(&cur_buf->cut_write_cond);
    }
    else
    {
	/* Wakeup writers */
	cur_buf->cut_buf_wwakeup = 0;
	cl_stat = CScnd_broadcast(&cur_buf->cut_read_cond);
    }

    /*
    ** release sem
    */
    cl_stat = CSv_semaphore(&cur_buf->cut_buf_sem);

    return (E_DB_OK);

} /* cut_wakeup_waiters */

/*
** Name: cut_signal_status - signal status to cooperating threads
**
** Description:
**	Signal a status to other cooperating threads asynchronously. This
**	function is designed to signal a status to other threads that may
**	be cooperating with the caller. The status will normally be outside
**	of the standard flow of processing (caused by an unexpected error
**	in processing such as a SEGV). The function is primarily designed
**	to provide cross-thread error handling in the case where an error
**	in one thread affects the processing of others, but is can be used
**	to simply signal a status to any other thread in the same process
**	asynchronously. The act of signalling the status does not, in itself,
**	cause the target thread(s) to alter processing (except to change the
**	status value itself), the threads are expected to be coded to check 
**	the status at some quiescent point and perform the correct processing.
**
**	There is an implicit assumption in the design of this interface that
**	there will be a way for the calling and target thread(s) to communicate
**	under normal processing, we further assume that the method of
**	communication will be a CUT buffer, which means that the threads
**	to be signalled are attached to the buffer passed to this function.
**
**	A status value of E_DB_OK implies that the signalled status has been
**	dealt with and needs to be re-set. Note that all threads are autonomous
**	in CUT and each thread must deal with the signalled status individually,
**	this means that a status value of E_DB_OK will only cause the current
**	thread to be re-set.
**
** Inputs:
**	cut_rcb.cut_buf_handle		List of affected buffers or NULL
**	status				Status to signal, may be
**					E_DB_INFO, E_DB_WARN, E_DB_ERROR,
**					E_DB_FATAL, E_DB_OK
**
** Outputs:
**	error				Returned error status
** Returns
**	DB_STATUS
**
** Side effects:
**	Asynchronously sets a status in other threads that may cause
**	processing to alter.
**
** History:
**	22-may-98 (stephenb)
**	    Created as stub.
**	17-may-99 (stephenb)
**	    Allow status of E_DB_OK so that non-fatal indicators can be
**	    re-set.
**	16-Apr-2004 (schka24)
**	    Avoid segv when signalling all thread bufs and there aren't any.
**	    Don't CSattn with semaphore held, can cause sem deadlock.
**	22-Jun-2004 (jenjo02)
**	    Don't signal the calling thread, neither set its async
**	    status, so that it can continue using its buffers without
**	    having to trip over async status errors.
**	    Must also release buf_sem before calling CSattn as
**	    the signalled thread may be in a condition wait on the
**	    buffer and CSattn needs to acquire the condition's
**	    mutex (buf_mutex).
**	09-Sep-2004 (jenjo02)
**	    Set status in async_status as well as TCB.
**	17-Sep-2004 (jenjo02)
**	    Don't require that this thread is attached to CUT.
**	    For example, cut_thread_init() may fail and the
**	    caller may want to signal the failure to all
**	    others attached to a buffer.
**	14-Feb-2005 (jenjo02)
**	    Must hold thread's tcb_sem while signalling it
**	    to keep it from disappearing out from underneath
**	    us.
**	22-Mar-2007 (kschendel) b122040
**	    Mustn't.  That leads to reverse mutex ordering (hold tcb sem,
**	    take buf sem in CSattn, which is backwards) and mutex deadlock.
**	    Arrange for a flag that tells cut terminate thread that it's not
**	    allowed to go forward with the terminate until the flag goes off.
*/
DB_STATUS
cut_signal_status(
	CUT_RCB		*cut_rcb,
	DB_STATUS	status,
	DB_ERROR	*error)
{
    bool		all_bufs = FALSE;
    CUT_TCB		*cur_tcb;
    bool		thread_found;
    CUT_INT_BUF		*cur_buf;
    CUT_BUF_THREAD	*cur_thread;
    CUT_BUF_THREAD	*thread_list;
    STATUS		cl_stat;
    DB_STATUS		local_stat;
    i4			erlog, erval;
    bool		async_stat = FALSE;

    /* 
    ** check parms
    */
    CLRDBERR(error);

    if (status != E_DB_INFO && status != E_DB_WARN && status != E_DB_ERROR && 
	status != E_DB_FATAL && status != E_DB_OK)
    {
	SETDBERR(error, 0, E_CU0115_INVALID_STATUS);
	local_stat = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 1, sizeof(DB_STATUS), &status);
	return(E_DB_ERROR);
    }

    /* If just resetting status, none of this matters */
    if ( status != E_DB_OK )
    {
	if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
	    all_bufs = TRUE;
	else
	{
	    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
	    /* Sanity check passed buffer */
	    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
	    {
		SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
		    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
		return (E_DB_ERROR);
	    }
	}
    }
    /* 
    ** check thread is initialized in cut, init if not
    */
    if (cut_thread_init((PTR *)&cur_tcb, error) > E_DB_INFO)
    {
	if (error->err_code == E_CU0204_ASYNC_STATUS)
	    async_stat = TRUE;
	else
	    /* Keep going without a TCB */
	    cur_tcb = (CUT_TCB*)NULL;
    }
    if (status == E_DB_OK)
    {
	/* 
	** re-setting status after it has been dealt with,
	** effective only on the current thread
	*/
	if ( cur_tcb )
	    cur_tcb->cut_thread_status = E_DB_OK;
	/* re-set interrupt mask */
	CSintr_ack();
	return(E_DB_OK);
    }
    /*
    ** for each buffer, check attached threads and re-set status
    */
    if ( cur_tcb )
	thread_list = cur_tcb->cut_buf_list;
    else
	thread_list = (CUT_BUF_THREAD*)NULL;

    if (all_bufs)
    {
	/* If nothing to do in this thread, just return */
	if (thread_list == NULL)
	    return(E_DB_OK);
	cur_buf = thread_list->cut_int_buffer;
    }

    do /* each buffer attached by this thread (or passed buffer) */
    {
	cl_stat = CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
	if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name), 
		cur_buf->cut_ircb.cut_buf_name);
	    return (E_DB_ERROR);
	}
	cur_thread = cur_buf->cut_thread_list;
	for (;;) /* each thread attached to this buffer */
	{ 
	    /* It's silly, even bad, to signal ourselves */
	    if ( cur_thread->cut_tcb != cur_tcb )
	    {
		cl_stat = CSp_semaphore(TRUE, 
		    &cur_thread->cut_tcb->cut_tcb_sem);
		if (cl_stat != OK)
		{
		    char	erbuf[50];
		    CS_SID	sid;

		    CSget_sid(&sid);
		    SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
			0, 0, 0, &erlog, 1, sizeof(erbuf), 
			STprintf(erbuf, "TCB sem for thread %x", sid));
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    return (E_DB_ERROR);
		}

		/* 
		** Signal the thread if worse status than it
		** already knows about.
		*/
		if ( status > cur_thread->cut_tcb->cut_thread_status )
		{
		    cur_thread->cut_tcb->cut_thread_status = status;
		    *cur_thread->cut_async_status = status;
		    cur_thread->cut_tcb->being_signalled = TRUE;
		    CSv_semaphore(&cur_thread->cut_tcb->cut_tcb_sem);
		    /*
		    ** Must release the buf_sem. The thread we're
		    ** about to signal may be waiting on the
		    ** condition using buf_sem and CSattn will
		    ** try to take that sem.
		    **
		    ** Can't hold the tcb sem, leads to mutex deadlock.
		    ** Instead, light a flag (above) that tells terminating
		    ** threads that they can't leave yet.
		    ** If the terminator gets there first, it arranges to be
		    ** invisible to the above status check, closing the other
		    ** half of the potential race.
		    */
		    CSv_semaphore(&cur_buf->cut_buf_sem);
		    CSattn(CS_CTHREAD_EVENT, cur_thread->cut_tcb->cut_thread_id);
		    /* Retake the mutex, restart the thread list */
		    CSp_semaphore(TRUE, &cur_thread->cut_tcb->cut_tcb_sem);
		    cur_thread->cut_tcb->being_signalled = FALSE;
		    CSv_semaphore(&cur_thread->cut_tcb->cut_tcb_sem);
		    CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
		    cur_thread = cur_buf->cut_thread_list;
		    continue;
		}
		CSv_semaphore(&cur_thread->cut_tcb->cut_tcb_sem);
	    }

	    cur_thread = cur_thread->buf_next;
	    if ( cur_thread == cur_buf->cut_thread_list )
		break;
	}
	CSv_semaphore(&cur_buf->cut_buf_sem);
	if (thread_list == NULL || !all_bufs)
	    break;
	thread_list = thread_list->thread_next;
	cur_buf = thread_list->cut_int_buffer;
    } while (cur_tcb && thread_list != cur_tcb->cut_buf_list);

    if (async_stat)
	return(E_DB_ERROR);
    else
	return(E_DB_OK);
}

/*
** Name: cut_event_wait - Wait for a specified event to occur
**
** Description:
**	This function causes the thread to sleep until a specified CUT
**	event has occurred. Possible events are:
**
**		CUT_ATTACH		Wait for a specified number of threads
**					to be attached to buffer
**		CUT_HAVE_ATTACHED	Wait for a specified number of threads
**					to have attached to buffer
**		CUT_DETACH		Wait until only a specified number of
**					threads are attached to buffer
**		CUT_SYNC		Wait until the specified number of
**					threads have all done an event-wait
**					for CUT_SYNC
**
**	The attach and detach events are inverses.  The ATTACH wait will
**	wait until at least N threads are attached.  The DETACH wait will
**	wait until no more than N threads are attached.
**
**	If the thread creating a buffer is a writer, typically a
**	CUT_ATTACH wait in that thread suffices to sync up all
**	participating threads.  The writer thread does a CUT_ATTACH
**	wait for the reader to show up;  meanwhile the reader attaches
**	and blocks on a buffer read.
**
**	If however the creating/driving thread is a reader, CUT_ATTACH
**	is insufficient.  A writer thread could start, perform a few
**	writes, and exit before the reader/driver can reach its CUT_ATTACH
**	wait.  Thus the reader would wait forever.  The CUT_SYNC mechanism
**	was created for this situation.  CUT_SYNC is appropriate when the
**	number of participating threads is known in advance, and all threads
**	want to reach a rendezvous point before proceeding.
**
**	Finally, CUT_DETACH is useful when threads are shutting down, and
**	a driver thread wants to ensure that all other participants have
**	disconnected before proceeding.
**
**	Usage detail for CUT_SYNC:  if someone issues a cut_signal_status
**	and kicks threads out of a rendezvous, whether before or during
**	its satisfaction, the rendezvous data is left in an indeterminate
**	state.  The assumption is that something bad has happened, and
**	threads will be detaching from CUT rather than trying to go forward.
**
** Inputs:
**	cut_rcb.cut_buf_handle		Buffer event is relevant to
**	event				Event to wait for
**	value				Event value
**
** Outputs:
**	error				Returned error code
**
** Returns:
**	DB_STATUS
**
** History:
**	26-may-98 (stephenb)
**	    Written as stub
**	3-jun-99 (stephenb)
**	    Check for interrupt return from CScnd_wait()
**	26-Apr-2004 (schka24)
**	    Detach is to shutdown as attach is to startup.
**	    Invent sync-wait for a group rendezvous, useful when the
**	    reading end wants to manage cut buffer creation.
**	4-May-2004 (schka24)
**	    Fix stupid typo in rendezvous wait.
**	17-Sep-2004 (jenjo02)
**	    Added CUT_HAVE_ATTACHED-type wait.
*/
DB_STATUS
cut_event_wait(
	CUT_RCB		*cut_rcb,
	u_i4		event,
	PTR		value,
	DB_ERROR	*error)
{
    CUT_INT_BUF		*cur_buf;
    i4			*attached = (i4 *)value;
    STATUS		cl_stat;
    i4			erlog, erval;
    DB_STATUS		status = E_DB_OK;

    /*
    ** Check parms
    */
    CLRDBERR(error);

    if (event != CUT_ATTACH && event != CUT_DETACH && 
	event != CUT_SYNC && event != CUT_HAVE_ATTACHED)
    {
	SETDBERR(error, 0, E_CU0116_BAD_EVENT);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
	    0, 0, 0, &erlog, 1, sizeof(u_i4), &event);
	return(E_DB_ERROR);
    }
    /* Attach and sync only make sense with 2 or more.
    ** Detach is typically called with 1.
    */
    if ( ((event == CUT_ATTACH || event == CUT_SYNC || event == CUT_HAVE_ATTACHED)
		&& *attached < 2)
      || (event == CUT_DETACH && *attached < 1) )
    {
	SETDBERR(error, 0, W_CU0309_TOO_FEW_THREADS);
	return (E_DB_WARN);
    }
    if (cut_rcb == NULL || cut_rcb->cut_buf_handle == NULL)
    {
	erval = 0;
	SETDBERR(error, 0, E_CU0102_NULL_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(i4), &erval, 15, "cut_event_wait");
	return (E_DB_ERROR);
    }

    cur_buf = (CUT_INT_BUF *)cut_rcb->cut_buf_handle;
    /* Sanity check passed buffer */
    if (cur_buf->cut_ircb.cut_buf_handle != (PTR)cur_buf)
    {
	SETDBERR(error, 0, E_CU010C_INCONSISTENT_BUFFER);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 2, sizeof(PTR), &cut_rcb->cut_buf_handle,
	    sizeof(PTR), &cur_buf->cut_ircb.cut_buf_handle);
	return (E_DB_ERROR);
    }

    /*
    ** grab sem
    */
    cl_stat = CSp_semaphore(TRUE, &cur_buf->cut_buf_sem);
    if (cl_stat != OK)
    {
	SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
	    cur_buf->cut_ircb.cut_buf_name);
	return (E_DB_ERROR);
    }

    /* If this is a rendezvous, there is some extra work to do... */
    if (event == CUT_SYNC)
    {
	/* First, make sure that any prior rendezvous has completed.
	** "satisfied" will be FALSE, except when threads are in the
	** process of leaving a (prior) rendezvous.
	*/
	while (cur_buf->cut_sync_satisfied)
	{
	    cl_stat = CScnd_wait(&cur_buf->cut_event_cond, &cur_buf->cut_buf_sem);
	    if (cl_stat == E_CS000F_REQUEST_ABORTED)
	    {
		/* caused by an interrupt (possibly the result of a
		** cut_signal_status(), return async status */
		SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
		CSv_semaphore(&cur_buf->cut_buf_sem);
		return(E_DB_ERROR);
	    }
	    else if (cl_stat != OK)
	    {
		SETDBERR(error, 0, E_CU0117_BAD_EVENT_CONDITION);
		status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		    0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
		    cur_buf->cut_ircb.cut_buf_name);
		CSv_semaphore(&cur_buf->cut_buf_sem);
		return(E_DB_ERROR);
	    }
	}

	/* The rendezvous is available, note that we are joining up */
	++ cur_buf->cut_buf_syncers;
    }

    /* Wait for proper event (at least N if attaching, no more than N if
    ** detaching, exactly N if syncing).
    ** while (event not satisfied) ...
    */
    while ((event == CUT_ATTACH && cur_buf->cut_buf_info.cut_num_threads < *attached)
	  || (event == CUT_DETACH && cur_buf->cut_buf_info.cut_num_threads > *attached)
	  || (event == CUT_SYNC && ! (cur_buf->cut_sync_satisfied
				    || cur_buf->cut_buf_syncers == *attached))
	  || (event == CUT_HAVE_ATTACHED && cur_buf->cut_buf_info.cut_hwm_threads < *attached) )
    {
	cl_stat = CScnd_wait(&cur_buf->cut_event_cond, &cur_buf->cut_buf_sem);
	if (cl_stat == E_CS000F_REQUEST_ABORTED)
	{
	    /* caused by an interrupt (possibly the result of a
	    ** cut_signal_status(), return async status */
	    SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    return(E_DB_ERROR);
	}
	else if (cl_stat != OK)
	{
	    SETDBERR(error, 0, E_CU0117_BAD_EVENT_CONDITION);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0,
		0, 0, 0, &erlog, 1, sizeof(cur_buf->cut_ircb.cut_buf_name),
		cur_buf->cut_ircb.cut_buf_name);
	    CSv_semaphore(&cur_buf->cut_buf_sem);
	    return(E_DB_ERROR);
	}
    }

    /* If this was a rendezvous, it was satisfied.  Wake up the other
    ** guys, if any are left;  count us out;  and reset the rendezvous
    ** system if we're the last guy out of the rendezvous.
    */
    if (event == CUT_SYNC)
    {
	if (--cur_buf->cut_buf_syncers == 0)
	{
	    cur_buf->cut_sync_satisfied = FALSE;
	}
	else
	{
	    cur_buf->cut_sync_satisfied = TRUE;
	    /* Wake up next guy */
	    CScnd_signal(&cur_buf->cut_event_cond, (CS_SID) 0);
	}
    }

    CSv_semaphore(&cur_buf->cut_buf_sem);
    
    return(E_DB_OK);
}

/*
** Name: cut_local_gca - send GCA type messages to cooperating threads
**
** Description:
**	This function is designed to immitate the IIGCa_call() function
**	for local threads. It realy represents an abstraction layer from
**	the standard CUT functions, and, for most operations, will simly
**	call a combination of other CUT functions to perform the task. The
**	idea is that the caller should be able to replace calls to
**	IIGCa_call() with calls to cut_local_gca(), where the the peer 
**	thread is in the local process. This should save the overhead of
**	creating a communications channel and asking the operating system to
**	pass data through it, when it is un-neccesary in this instance.
**
**	Not all GCA calls result in data being sent through the communications
**	channel, and where this is the case, this function will not seek to
**	immitate the GCA operations (since GCA can simply be called to perform
**	the operation itself).
**
**	The ultimate aim is that a thread should be able to decide that
**	a peer conection is required to perform some GCA related task
**	(i.e. run an SQL statement), then at some point an assesment will be
**	made as to whether the task can be completed locally, and a thread
**	may be used as opposed to a remote connection. This function should
**	allow the caller to use the same calling semantics no matter where the
**	thread actually exists, thus allowing us to build a location 
**	independent communications protocol for task threads. This will
**	ultimately allow us to run the same thread code on a single CPU
**	machine, a standard SMP machine, or a shared-nothing MPP machine.
**
**	For the initial implimentation, this function will not support 
**	asynchronous operations, or timeouts on operations. If the caller
**	requests an operation to be performed asynchronously, then the
**	operation will be performed synchronously anyway, whithout error
**	(to remain consistent with GCA). If an anychronous completion function
**	is passed, it will be executed at the synchronous completion of the
**	task.
**
** Inputs:
**	gca_op_code	Requested GCA operation (only some are supported)
**	gca_parmlist	GCA type service parameters
**	gca_indicators	GCA type flags
**	gca_async_id	Ignored
**	gca_time_out	Ignored
**
** Outpus:
**	error		Error code
**
** Returns:
**	DB_STATUS
**
** History:
**	26-may-98 (stephenb)
**	    Created as a stub
*/
DB_STATUS
cut_local_gca(i4		gca_op_code, 
	      GCA_PARMLIST	*gca_parmlist, 
	      i4		gca_indicators, 
	      PTR		gca_async_id, 
	      i4		gca_timeout, 
	      DB_ERROR		*error)
{
    return(E_DB_OK);
}

/*
** Name: cut_init - Initialize CUT
**
** Description:
**	Initialize environment for CUT, currently this simply involves
**	initializing and naming the buffer master list semaphore and the
**	thread master list semaphore.
**
** Inputs:
**	erfunc
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** History:
**	2-jun-98 (stephenb)
**	    Created.
**	21-Nov-2007 (jonj)
**	    Init CutIsMT static - are we running with Ingres or
**	    OS threads?
*/
DB_STATUS
cut_init(DB_STATUS	(*erfunc)())
{
    DB_STATUS		status;

    /* check parms */
    if (erfunc == NULL)
    {
	return(E_DB_ERROR);
    }

    /* Init sems */
    status = CSw_semaphore(&Thread_list_sem, 
			   CS_SEM_SINGLE, "CUT Thread Master");
    if (status != OK)
	return(status);
    status = CSw_semaphore(&Buf_list_sem, 
			 CS_SEM_SINGLE, "CUT Buffer Master");

    log_error = erfunc;

    /* Note threading model */
    CutIsMT = CS_is_mt();

    return(status);
}

/*
** Name: buffer_sem - Check for valid buffer and grab sem
**
** Description:
**	This routine checks that the passed buffer is valid (by searching the
**	list for it), and then grabs the buffer semaphore before returning
**	to the caller.
**
**	This code is written to conform to the following protocol
**
**		 - Grab buffer list sem
**		 - Check for buffer in list
**		 - Grab buffer sem
**		 - (Optional) Release buffer list sem
**
**	Note that if "hold-list-sem" is false, all we really need to do
**	is to grab the buffer semaphore.  The rest of the work done here
**	is preventative, and could possibly be ifdef'ed or otherwise
**	turned off once we gain more experience and confidence with using CUT.
**	If we are keeping the list mutex, it's probably attach/detach,
**	and it's probably a good idea to validate the buffer itself
**	(especially for attach).
**
** Inputs:
**	buffer		Buffer to mutex
**	hold_list_sem	TRUE to keep the buffer list sem, FALSE to release.
**			(Held only on OK return.)
**			Attach/detach need to maintain the list mutex.
**	caller		Caller string for error messages
**
** Outputs:
**	buffer		In a mutexed state
**	error		Returned error
**
** History:
**	9-jun-98 (stephenb)
**	    Initial creation
**	4-May-2004 (schka24)
**	    Pass in caller "where" string for errmsgs.
**	17-May-2004 (schka24)
**	    Need to hold list sem on attach to protect attach's get-block.
*/
static DB_STATUS
buffer_sem(CUT_INT_BUF		*buffer,
	   bool			hold_list_sem,
	   char			*caller,
	   DB_ERROR		*error)
{
    STATUS		cl_stat;
    CUT_INT_BUF		*check_buf;
    CUT_INT_BUF		*cur_buf;
    i4		erlog;
    DB_STATUS		status = E_DB_OK;
    
    /* grab list sem */
    cl_stat = CSp_semaphore(TRUE, &Buf_list_sem);
    if (cl_stat != OK)
    {
	SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 1, 22, "Buffer List semaphore");
	return (E_DB_ERROR);
    }
    /* check for buffer */
    if (check_buf = Buf_list)
    {
	do
	{
	    if (check_buf == buffer)
	    {
		/* buffer is in list, grab sem */
		cl_stat = CSp_semaphore(TRUE, &buffer->cut_buf_sem);
		if (cl_stat != OK)
		{
		    SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
			0, 0, 0, &erlog, 1, 
			sizeof(buffer->cut_ircb.cut_buf_name), 
			buffer->cut_ircb.cut_buf_name);
		    CSv_semaphore(&Buf_list_sem);
		    return (E_DB_ERROR);
		}
		/* 
		** consistency check for connected threads, this ensures
		** that we didn't slip in after the last user has diconnected
		** and the buffer is about to be removed from the list
		*/
		if (buffer->cut_buf_info.cut_num_threads == 0)
		{
		    CSv_semaphore(&buffer->cut_buf_sem);
		    CSv_semaphore(&Buf_list_sem);
		    SETDBERR(error, 0, E_CU0118_BAD_BUFFER);
		    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
			0, 0, 0, &erlog, 0); 
		    return(E_DB_ERROR);
		}
		/* release list sem */
		if (! hold_list_sem)
		    CSv_semaphore(&Buf_list_sem);
		return(E_DB_OK);
	    } 
	    check_buf = check_buf->buf_next;
	} while (check_buf != Buf_list);
    }
    /*
    ** Buffer not in list 
    */
    SETDBERR(error, 0, E_CU0119_BUF_NOT_IN_LIST);
    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 
	0, 0, 0, &erlog, 1, 0, caller); 
    /* Release the list mutex */
    CSv_semaphore(&Buf_list_sem);
    return (E_DB_ERROR);
}

/*
** Name: cut_thread_init - Initialize CUT for a thread
**
** Description:
**	Initilaize CUT for the given thread. Currently, the function simply
**	allocates a CUT_TCB for the thread and adds it to the global list
**
** Inputs:
**	None
**
** Outpus:
**	cut_thread		- Thread handle for CUT
**
** Returns:
**	DB_STATUS
**
** History:
**	11-jun-97 (stephenb)
**	    Created
**	26-aug-98 (stephenb)
**	    Added code to check thread status in CUT.
**	4-mar-04 (toumi01)
**	    Expand sem_name by one byte to avoid mem overlay mayhem.
**	16-Apr-2004 (schka24)
**	    Going to need more than that on 64-bit machines...
*/
DB_STATUS 
cut_thread_init(PTR		*cut_thread,
		DB_ERROR	*error)
{
    CUT_TCB	*check_thread;
    CS_SID	my_sid;
    STATUS	cl_stat;
    bool	thread_found = FALSE;
    char	sem_name[30];
    i4	erlog;
    DB_STATUS	status = E_DB_OK;

    CLRDBERR(error);

    /* Grab sem */
    cl_stat = CSp_semaphore(TRUE, &Thread_list_sem);
    if (cl_stat != E_DB_OK)
    {
	SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 1, 22, "Thread List semaphore");
	return (E_DB_ERROR);
    }

    CSget_sid(&my_sid);

    /* check list for thread */
    if (check_thread = Thread_list)
    {
	do
	{
	    if (check_thread->cut_thread_id == my_sid) 
	    {
		thread_found = TRUE;
		break;
	    }
	    check_thread = check_thread->thread_next;
	} while (check_thread != Thread_list);
	if (thread_found)
	{
    	    *cut_thread = (PTR)check_thread;
	    if (check_thread->cut_thread_status != E_DB_OK)
	    {
		SETDBERR(error, 0, E_CU0204_ASYNC_STATUS);
		status = check_thread->cut_thread_status;
	    }
	    else
	    {
		SETDBERR(error, 0, I_CU0402_THREAD_INITIALIZED);
		status = E_DB_INFO;
	    }
	    CSv_semaphore(&Thread_list_sem);
	    return(status);
	}
    }
    /* add thread to list */
    if (get_block(CUT_TCB_TYPE, (PTR *)&check_thread, error))
    {
	/* error logged already */
	CSv_semaphore(&Thread_list_sem);
	return(E_DB_ERROR);
    }
    if (Thread_list)
    {
	/* list exists */
	check_thread->thread_next = Thread_list;
	check_thread->thread_prev = Thread_list->thread_prev;
	Thread_list->thread_prev->thread_next = check_thread;
	Thread_list->thread_prev = check_thread;
    }
    else
	check_thread->thread_next = check_thread->thread_prev = 
	    Thread_list = check_thread;

    /* init TCB */
    check_thread->cut_thread_id = my_sid;
    check_thread->cut_thread_status = E_DB_OK;
    check_thread->cut_buf_list = NULL;
    check_thread->being_signalled = FALSE;
    CSw_semaphore(&check_thread->cut_tcb_sem, CS_SEM_SINGLE, 
		  STprintf(sem_name, "CUT_TCB Sem %x",check_thread));


    *cut_thread = (PTR)check_thread;

    /* release sem */
    CSv_semaphore(&Thread_list_sem);

    return(E_DB_OK);
}

/*
** Name: cut_thread_term - Terminate thread in CUT
**
** Description:
**	This function cleans up the CUT resources associated with a thread
**
**	Please note that even if cascade (below) is set, this routine can
**	return with error and ASYNC_STATUS if the thread is still holding
**	a non-OK cut-tcb status.  Thus, the proper way to unconditionally
**	detach/clean up a thread is something like:
**		(void) cut_signal_status(NULL, E_DB_OK, &error);
**		status = cut_thread_term(TRUE, &error);
**
** Inputs:
**	Cascade		Boolean used to determin whether to cascade the thread
**			delete to attached buffers. if cascade is set, and
**			the thread is still attached to any buffers, the
**			thread will be detached from all buffers and a warning
**			will be returned to the caller. If cascase is not set
**			the function will return an error if the thread is still
**			attached to any buffers.
** 
** Outputs:
**	error		Returned error
**
** Returns:
**	DB_STATUS
**
** History:
**	11-jun-98 (stephenb)
**	    Created
**	4-mar-04 (toumi01)
**	    Fix == vs. = typo for NULLing Thread_list.
**	18-Apr-2004 (schka24)
**	    Do unconditional buffer detach.
**	14-Feb-2004 (jenjo02)
**	    Before commencing with detaches, set TCB's status
**	    to E_DB_FATAL (highest possible) to keep
**	    signallers away from this thread. Do that while
**	    holding tcb_sem.
**	22-Mar-2007 (kschendel) b122040
**	    Add to Jon's previous change to make sure we don't exit
**	    prematurely when a signaller has already chosen us by the
**	    time we hike up our status to fatal.
*/
DB_STATUS
cut_thread_term(bool		cascade,
		DB_ERROR	*error)
{
    STATUS		cl_stat;
    CUT_TCB		*my_tcb;
    CS_SID		my_sid;
    bool		tcb_found = FALSE;
    CUT_BUF_THREAD	*buf_thread;
    CUT_RCB		*cut_rcb;
    DB_STATUS		status = E_DB_OK;
    i4		erlog;

    CLRDBERR(error);

    /* grab TCB list sem */
    cl_stat = CSp_semaphore(TRUE, &Thread_list_sem);
    if (cl_stat != OK)
    {
	SETDBERR(error, 0, E_CU010B_CANT_GET_SEM);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
	    0, &erlog, 1, 22, "Thread List semaphore");
	return (E_DB_ERROR);
    }
    if (my_tcb = Thread_list)
    {
	/* find myself in the list */
	CSget_sid(&my_sid);
	do
	{
	    if (my_tcb->cut_thread_id == my_sid)
	    {
		CSv_semaphore(&Thread_list_sem);
		CSp_semaphore(TRUE, &my_tcb->cut_tcb_sem);
		tcb_found = TRUE;
	    	break;
	    }
	    my_tcb = my_tcb->thread_next;
	} while (my_tcb != Thread_list);
	if (tcb_found)
	{
	    /* check for attached buffers */
	    if (buf_thread = my_tcb->cut_buf_list)
	    {
		/* still attached to buffers */
		if (cascade)
		{
		    /* Thwart any signals by setting our status to the max.
		    ** That's only half the battle; we also need to ensure
		    ** that someone didn't choose to signal us just as we
		    ** grabbed the tcb sem.  (It's not possible for signallers
		    ** to simply hold the tcb sem, that causes out-of-order
		    ** mutexing and a mutex deadlock.)
		    **
		    ** This business of sleeping is awkward;  it really ought
		    ** to be a condition variable wait, but there's no
		    ** convenient CV to use and this should be rare anyway.
		    */

		    my_tcb->cut_thread_status = E_DB_FATAL;
		    while (my_tcb->being_signalled)
		    {
			CSv_semaphore(&my_tcb->cut_tcb_sem);
			CSms_thread_nap(10);		/* Delay .01 second */
			CSp_semaphore(TRUE, &my_tcb->cut_tcb_sem);
		    }
		    CSv_semaphore(&my_tcb->cut_tcb_sem);

		    /* detach from buffers */
		    do
		    {
			cut_rcb = &buf_thread->cut_int_buffer->cut_ircb;
			buf_thread = buf_thread->thread_next;
			/* Note:
			** This call will grab both the buffer list master sem
			** and the buffer sem for the buffer.
			** Unconditional detach here...
			*/
			status = cut_detach_buf(cut_rcb, TRUE, error);
			if (status != E_DB_OK)
			{
			    /* error already logged */
			    my_tcb->cut_thread_status = E_DB_OK;
			    return(E_DB_ERROR);
			}
		    } while (my_tcb->cut_buf_list != NULL);
		}
		else
		{
		    SETDBERR(error, 0, E_CU0203_TERM_ATTACHED);
		    CSv_semaphore(&my_tcb->cut_tcb_sem);
		    return (E_DB_ERROR);
		}
	    }
	    else
		CSv_semaphore(&my_tcb->cut_tcb_sem);

	    /* remove from (mutexed) list */
	    CSp_semaphore(TRUE, &Thread_list_sem);
	    if (my_tcb == Thread_list)
	    {
		if (my_tcb->thread_next == my_tcb &&
		    my_tcb->thread_prev == my_tcb)
		    /* only one in list */
		    Thread_list = NULL;
		else
		{
		    Thread_list = my_tcb->thread_next;
		    Thread_list->thread_prev = my_tcb->thread_prev;
		    my_tcb->thread_prev->thread_next = Thread_list;
		}
	    }
	    else
	    {
		my_tcb->thread_next->thread_prev =
		    my_tcb->thread_prev;
		my_tcb->thread_prev->thread_next =
		    my_tcb->thread_next;
	    }
	    /* Don't leak mutexes */
	    CSr_semaphore(&my_tcb->cut_tcb_sem);
	    /* and release mem */
	    FREE_BLOCK(sizeof(CUT_TCB), my_tcb);
	}
    }
    /* release sem */
    CSv_semaphore(&Thread_list_sem);

    return(E_DB_OK);
}

/*
** Name: get_block - get memory for a CUT control structure
**
** Description:
**	This routine maintains a free list of CUT control block structures,
**	this allows us to get memory for a block without having to go
**	to ME, which maintains a more generic free list leading to posible
**	fragmentation. Requests to this routine are for a fixed sized
**	control block which will be satisfied from the relevant free list, or
**	by a call to ME if there are none free. Calls to ME will grab
**	enough for CUT_ME_NUM_BLOCKS structures at once to increase effeciency
**
**	THE FIRST FIELD IN ALL STRUCTURES MANAGED BY THIS ROUTINE IS THE "USED"
**	BYTE. THIS BYTE IS SET ONLY BY THIS ROUITNE, AND WILL BE SET BEFORE
**	RETURNING THE BLOCK FOR USE. A block without this byte set will be 
**	considered free, used blocks never realy leave the "free list" as
**	such. This means that a "free" operation simply zero's the "used" byte
**	and it defined as a macro that executes MEfill().
**
**	It is assumed that all callers of this function have first obtained the
**	mutex that controls the list that the block is being used in. These are:
**
**		CUT_INT_BUF		Buf_list_sem
**		CUT_BUF_THREAD		Buf_list_sem
**		CUT_TCB			Thread_list_sem
**
** Inputs:
**	type		type of structure needed
**	block		Pointer to fill with returned address
**
** Outputs:
**	block		Address of allocated struct (zero'd)
**	error		Returned error code
**
** Returns:
**	DB_STATUS
**
** History:
**	18-jun-98 (stephenb)
**	    Created
*/
static DB_STATUS
get_block(i4		type,
	  PTR		*block,
	  DB_ERROR	*error)
{
    i4		size;
    bool	*used;
    i4		i;
    STATUS	cl_stat;
    PTR		*next_ptr;
    i4		erlog, erval;
    DB_STATUS	status = E_DB_OK;
    PTR		*pool;

    /*
    ** check block type
    */
    switch (type)
    {
	case CUT_INT_BUF_TYPE:
	{
	    size = sizeof(CUT_INT_BUF);
	    *block = cut_int_buf_pool;
	    pool = &cut_int_buf_pool;
	    break;
	}
	case CUT_BUF_THREAD_TYPE:
	{
	    size = sizeof(CUT_BUF_THREAD);
	    *block = cut_buf_thread_pool;
	    pool = &cut_buf_thread_pool;
	    break;
	}
	case CUT_TCB_TYPE:
	{
	    size = sizeof(CUT_TCB);
	    *block = cut_tcb_pool;
	    pool = &cut_tcb_pool;
	    break;
	}
	default:
	{
	    SETDBERR(error, 0, E_CU011A_BAD_BLOCK_TYPE);
	    status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0,
		0, &erlog, 0);
	    return (E_DB_ERROR);
	}
    }
    /*
    ** check list for free object
    */
    while(*block)
    {
        for (i = 0; i < CUT_ME_NUM_BLOCKS; i++)
        {
	    used = *block;
	    if (*used)
	    {
	        *block += size;
	        continue;
	    }
	    /* block not used, return it */
	    *used = TRUE;
	    return(E_DB_OK);
        }
	/* 
	** at the end of the loop, *block points at the PTR sized chunk of 
	** memory at the end of this block, which contains the pointer to the
	** next block, or NULL if there is none
	*/
	next_ptr = (PTR *)*block;
	*block = *next_ptr;
    }
    /*
    ** if we got to here, then were out of slots, grab some memory
    */
    *block = MEreqmem(0, (size * CUT_ME_NUM_BLOCKS) + sizeof(PTR), 
			    TRUE, &cl_stat);
    if (*block == NULL || cl_stat != OK)
    {
	erval = size * CUT_ME_NUM_BLOCKS + sizeof(PTR);
	SETDBERR(error, 0, E_CU0105_NO_MEM);
	status = (*log_error)(error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 0, 0, 0, 0,
	    &erlog, 2, sizeof(i4), &erval,
	    0, "get_block");
    }
    /* chain it to the last block */
    if (*pool) /* pool exists */
	*next_ptr = *block;
    else /* this is the first block */
	*pool = *block;

    used = *block;
    *used = TRUE;

    return(E_DB_OK);
}
