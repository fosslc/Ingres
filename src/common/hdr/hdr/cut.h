/* Copyright (c) 1998, 2004 Ingres Corporation
**
**
*/

/*
** Name: cut.h - Header for CUT thread coordination services
**
** Description:
**	This file contains the external definitions of the thread coordination
**	services which can be found in common!cuf!cut
**
** History:
**	6-may-98 (stephenb)
**	    Initial cration.
**	aug-1998 (stephenb)
**	    Alter protos for cut_init() and cut_info(), add defs for 
**	    CUT_NOSTATE, CUT_READ, CUT_WRITE and CUT_ATTACH. Add strcture 
**	    CUT_THREAD_INFO. Add message E_CU0204_ASYNC_STATUS
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	    Created from spec.
**	19-Apr-2004 (schka24)
**	    Added "unconditional detach" arg to detach.
**	    Changed read/write wait arg to flags.
**	25-Apr-2004 (schka24)
**	    Added cut_wakeup_waiters prototype.
**	    Added CUT_DETACH, CUT_SYNC events and cut_writer_done prototype.
**	02-Jun-2004 (jenjo02)
**	    Renamed CUT_BUFFER to CUT_RCB to avoid confusion with 
**	    the real internal CUT buffer.
**	09-Aug-2004 (jenjo02)
**	    Added CUT_RW_SYNC modifier; wait even if there
**	    are no writers, yet.
**	18-Aug-2004 (jenjo02)
**	    Added support for DIRECT mode reads and writes.
**	20-Sep-2004 (jenjo02)
**	    Add cut_hwm_threads, CUT_HAVE_ATTACHED form of 
**	    cut_event_wait.
*/

/*
** forward refs
*/
typedef struct		_CUT_RCB	CUT_RCB;
typedef struct		_CUT_BUF_INFO	CUT_BUF_INFO;
typedef union		_GCA_PARMLIST	*GCA_PARMLISTP;
/*
** prototypes
*/
FUNC_EXTERN	DB_STATUS	cut_create_buf(i4		num_bufs, 
					       CUT_RCB		**cut_rcb, 
					       DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_attach_buf(CUT_RCB		*cut_rcb, 
					       DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_detach_buf(CUT_RCB		*cut_rcb, 
					       bool		unconditional,
					       DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_info(i4			operator, 
					 CUT_RCB		*cut_rcb, 
					 PTR			info_buf, 
					 DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_read_buf(i4		*num_cells, 
					     CUT_RCB		*cut_rcb, 
					     PTR		*output_buf, 
					     i4			flags, 
					     DB_ERROR		*error);
FUNC_EXTERN	DB_STATUS	cut_read_complete(
					     CUT_RCB		*cut_rcb, 
					     DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS 	cut_write_buf(i4		*num_cells, 
					      CUT_RCB		*cut_rcb, 
					      PTR		*input_buf, 
					      i4		flags, 
					      DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_write_complete(
					     CUT_RCB		*cut_rcb, 
					     DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_writer_done(CUT_RCB		*cut_rcb,
						DB_ERROR	*error);

FUNC_EXTERN	DB_STATUS	cut_wakeup_waiters(CUT_RCB	*cut_rcb,
						   DB_ERROR	*error);

FUNC_EXTERN	DB_STATUS 	cut_signal_status(CUT_RCB	*cut_rcb, 
						  DB_STATUS	status, 
						  DB_ERROR	*error);

FUNC_EXTERN	DB_STATUS	cut_event_wait(CUT_RCB	*cut_rcb, 
					       u_i4	event, 
					       PTR		value, 
					       DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_local_gca(i4		gca_op_code, 
					      GCA_PARMLISTP	gca_parmlist, 
					      i4		gca_indicators, 
					      PTR		gca_async_id,
					      i4		gca_timeout, 
					      DB_ERROR		*error);

FUNC_EXTERN	DB_STATUS	cut_init(DB_STATUS (*)());

FUNC_EXTERN	DB_STATUS	cut_thread_init(PTR		*cut_thread, 
						DB_ERROR	*error);

FUNC_EXTERN	DB_STATUS	cut_thread_term(bool		cascade, 
						DB_ERROR	*error);
/*
** defines
*/

/* cut_info values */
#define		CUT_INFO_BUF	1 /* get info on passed buffer */
#define		CUT_ATT_BUFS	2 /* list buffers attached to current thread */
#define		CUT_ATT_THREADS	3 /* threads attached to given buffer */
#define		CUT_INFO_THREAD	4 /* information about current thread */

/* thread states */
#define		CUT_NOSTATE		0 /* no state */
#define		CUT_READ		1 /* thread is waiting to read */
#define		CUT_WRITE		2 /* thread is waiting to write*/
#define		CUT_ATTACH		3 /* thread is waiting for threads
					  ** to be attached */
#define		CUT_DETACH		4 /* thread is waiting for threads
					  ** to detach */
#define		CUT_SYNC		5 /* Thread rendezvous wait */
#define		CUT_HAVE_ATTACHED	6 /* Thread is waiting for threads
					  ** to have attached */

/* Read/write flags */
#define		CUT_RW_NONE		0 /* No flags */
#define		CUT_RW_WAIT		1 /* Wait if I/O blocked */
#define		CUT_RW_FLUSH		2 /* Force wakeup signal */
#define		CUT_RW_SYNC		4 /* Wait even if no writers */
#define		CUT_RW_DIRECT		8 /* Direct read/write */

/*
** Name: CUT_RCB - CUT Request Communications Block
**
** Description:
**	This structure contains all the information about a thread 
**	communications exchange buffer that is static (according to the 
**	current thread). It contains a handle to the CUT internal 
**	representation of the buffer and other information the is fixed
**	at buffer creation time. The control block is used to pass relavent
**	information to the CUT functions when creating the buffer, and return
**	information when the handle is used to access the buffer (currently
**	only with a cut_query_buf call).
**	
**	The buffer is implemented as a circular FIFO queue, each call to
**	write a cell to the buffer will cause the buffer head to move forward,
**	when all attached reading threads have read a cell, the buffer tail
**	moves forward. when the head or tail moves past the last cell, it
**	wraps back to the first cell. The buffer is empty when the head and 
**	tail are in the same place and full when the head is at the cell
**	immediately prior to the tail.
**
**	A note about the buffer usage state:  When attaching to a buffer,
**	a thread must declare its usage, either READ or WRITE.  The
**	"neither" state is not acceptable.  "Neither" state is reserved
**	for the situation where a writing thread has finished writing,
**	but does not yet want to detach from the buffer (to reap
**	status info, for example, or to sync via a cut_event_wait).
**
**	The "parent" flag defined below is for recording who-spawned-who.
**	During abnormal thread termination, when we're forcibly erroring-
**	out and cleaning up CUT attachments, we want parents to wait for
**	children -- not the other way around!  CUT per se doesn't care
**	about parentage.  (e.g. it's OK for a child thread to actually
**	create CUT buffers, and allow the parent to attach to them...)
**
** History:
**	6-may-98 (stephenb)
**	    Initial creation
**	26-Apr-2004 (schka24)
**	    Invent "neither" buffer-use state.
**	15-Jul-2004 (jenjo02)
**	    Renamed from CUT_BUFFER, added cut_thr_handle.
**	09-Sep-2004 (jenjo02)
**	    Added CUT_BUF_TRACE, cut_async_status
**	16-Sep-2004 (schka24)
**	    Added PARENT flag, mostly for cleanup.
**	27-Apr-2010 (kschendel)
**	    32 is plenty for an internal name
*/
struct _CUT_RCB
{
    char	cut_buf_name[DB_OLDMAXNAME_32 + 1]; /* null terminated 32char name */
    i4		cut_cell_size;
    i4		cut_num_cells;
    i4		cut_buf_use;
    DB_STATUS	cut_async_status;

#define		CUT_BUF_READ		0x0001 /* thread is attached for read */
#define		CUT_BUF_WRITE		0x0002 /* thread is attached for write*/
#define		CUT_BUF_NEITHER		0x0003 /* Thread is attached and done */
#define		CUT_BUF_PARENT		0x4000 /* Thread is parent for buf */
#define		CUT_BUF_TRACE		0x8000 /* Enable tracing */

    PTR		cut_buf_handle;
    PTR		cut_thr_handle;
};

/*
** Name: CUT_BUF_INFO - dynamic buffer information
**
** Description:
**	This structure describes information that may change during the life
**	of a buffer. It is returned from a cut_query_buf call and all 
**	information is only valid at the time of the call. The structure
**	provides a snapshot of the current state of the buffer. The caller 
**	should not assume that the information remains valid for any length of
**	time, and should avoid using the information to perform other actions.
**
** History:
**	6-may-98 (stephenb)
**	    Initial creation
**	09-Sep-2004 (jenjo02)
**	    Added cut_cell_writes_comp for DIRECT mode writes.
*/
struct _CUT_BUF_INFO
{
    i4		cut_num_threads; 		/* number of attached threads */
    i4		cut_hwm_threads; 		/* hwm of attached threads */
    i4		cut_buf_head;			/* cell # of buffer que head */
    i4		cut_buf_tail;			/* cell # of buffer que tail */
    i4		cut_cell_writes;		/* cumulative # initiated
						** writes to buffer */
    i4		cut_cell_writes_comp;		/* cumulative # completed
						** writes to buffer */
    i4		cut_cell_reads;			/* cumulative # completed
    						** reads from buffer */
    i4		cut_write_waits;		/* number of times we waited on
						** a write */
    i4		cut_read_waits;			/*
						** Number of times we waited on
						** a read */
};

/*
** Name: CUT_THREAD_INFO - thread information
**
** Description:
**	Contains user-visible thread information, used in cut_info() call
**
** History:
**	20-aug-98 (stephenb)
**	    Created.
*/
typedef struct _CUT_THREAD_INFO
{
    i4			cut_thread_use;	  /* buffer use for thread,
					  ** CUT_BUF_READ or CUT_BUF_WRITE */
    i4			cut_thread_state; /* current state in buffer */
    i4			cut_current_cell; /* current position of thread */
    DB_STATUS		cut_thread_status; /* thread's error status */
} CUT_THREAD_INFO;


/*
** CUT error numbers
*/
#define		E_CUF_OK				0
#define		E_CUF_INTERNAL				(E_CUF_MASK + 0x0201)

/*
** Fatal errors, both logged, returned from CUT, will cause process to exit
*/
/* NONE */

/* 
** errors both logged and returned from CUT
** 
** Return value:	E_DB_ERROR
** Range:		E_CU0101 - E_CU0200
*/
#define		E_CU0101_BAD_BUFFER_COUNT		(E_CUF_MASK + 0x0101)
#define		E_CU0102_NULL_BUFFER			(E_CUF_MASK + 0x0102)
#define		E_CU0103_BAD_BUFFER_USE			(E_CUF_MASK + 0x0103)
#define		E_CU0104_BAD_CELLS			(E_CUF_MASK + 0x0104)
#define		E_CU0105_NO_MEM				(E_CUF_MASK + 0x0105)
#define		E_CU0106_INIT_SEM			(E_CUF_MASK + 0x0106)
#define		E_CU0107_INIT_READ_COND			(E_CUF_MASK + 0x0107)
#define		E_CU0108_INIT_WRITE_COND		(E_CUF_MASK + 0x0108)
#define		E_CU0109_INIT_EVENT_COND		(E_CUF_MASK + 0x0109)
#define		E_CU010A_NAME_COND			(E_CUF_MASK + 0x010A)
#define		E_CU010B_CANT_GET_SEM			(E_CUF_MASK + 0x010B)
#define		E_CU010C_INCONSISTENT_BUFFER		(E_CUF_MASK + 0x010C)
#define		E_CU010D_BUF_NOT_IN_LIST		(E_CUF_MASK + 0x010D)
#define		E_CU010E_NO_CELLS			(E_CUF_MASK + 0x010E)
#define		E_CU010F_NO_OUTPUT_BUF			(E_CUF_MASK + 0x010F)
#define		E_CU0110_TO_MANY_CELLS			(E_CUF_MASK + 0x0110)
#define		E_CU0111_NOT_ATTACHED			(E_CUF_MASK + 0x0111)
#define		E_CU0112_BAD_WRITE_CONDITION		(E_CUF_MASK + 0x0112)
#define		E_CU0013_NO_INPUT_BUF			(E_CUF_MASK + 0x0113)
#define		E_CU0114_BAD_READ_CONDITION		(E_CUF_MASK + 0x0114)
#define		E_CU0115_INVALID_STATUS			(E_CUF_MASK + 0x0115)
#define		E_CU0116_BAD_EVENT			(E_CUF_MASK + 0x0116)
#define		E_CU0117_BAD_EVENT_CONDITION		(E_CUF_MASK + 0x0117)
#define		E_CU0118_BAD_BUFFER			(E_CUF_MASK + 0x0118)
#define		E_CU0119_BUF_NOT_IN_LIST		(E_CUF_MASK + 0x0119)
#define		E_CU011A_BAD_BLOCK_TYPE			(E_CUF_MASK + 0x011A)

/* 
** errors not logged, but returned from CUT (these problems can normally
** be recovered from by the caller).
**
** Return value:	E_DB_ERROR
** Range:		E_CU0201 - E_CU0300
*/
#define		E_CU0201_BAD_THREAD_STATE		(E_CUF_MASK + 0x0201)
#define		E_CU0202_READ_CELLS			(E_CUF_MASK + 0x0202)
#define		E_CU0203_TERM_ATTACHED			(E_CUF_MASK + 0x0203)
#define		E_CU0204_ASYNC_STATUS			(E_CUF_MASK + 0x0204)

/*
** Warnings, not logged but returned from CUT.
**
** Return value:	E_DB_WARN
** Range:		W_CU0301 - W_0400
*/
#define		W_CU0301_THREAD_ATTACHED		(E_CUF_MASK + 0x0301)
#define		W_CU0302_THREAD_NOT_ATTACHED		(E_CUF_MASK + 0x0302)
#define		W_CU0303_UNREAD_CELLS			(E_CUF_MASK + 0x0303)
#define		W_CU0304_NO_BUFFERS			(E_CUF_MASK + 0x0304)
#define		W_CU0305_NO_CELLS_READ			(E_CUF_MASK + 0x0305)
#define		W_CU0306_LESS_CELLS_READ		(E_CUF_MASK + 0x0306)
#define		W_CU0307_NO_CELLS_WRITTEN		(E_CUF_MASK + 0x0307)
#define		W_CU0308_LESS_CELLS_WRITTEN		(E_CUF_MASK + 0x0308)
#define		W_CU0309_TOO_FEW_THREADS		(E_CUF_MASK + 0x0309)

/*
** Informational messages, not logged, returned from CUT
**
** Return Value:	E_DB_INFO
** Range:		I_CU0401 - I_CU0500
*/

#define		I_CU0401_LAST_BUFFER			(E_CUF_MASK + 0x0401)
#define		I_CU0402_THREAD_INITIALIZED		(E_CUF_MASK + 0x0402)
