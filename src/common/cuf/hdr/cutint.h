/* Copyright (c) 1998, 2004 Ingres Corporation
**
**
*/

/*
** Name: cutint.h - CUT internal definitions
**
** Description:
**	This header file describes structures, defines and functions that 
**	are internal to CUT
**
**	Note on structure heirachcy:
**	The follwoing diagram depicts how the structures are linked together
**
**	                 ---------          ---------             ---------
**	                 |CUT_TCB|----------|CUT_TCB|-------------|CUT_TCB|
**	                 ---------          ---------             ---------
**	                     |                  |
**	-------------  ----------------  ----------------
**	|CUT_INT_BUF|--|CUT_BUF_THREAD|--|CUT_BUF_THREAD|
**	-------------  ----------------  ----------------
**	      |              |                  |
**	-------------  ----------------         |
**	|CUT_INT_BUF|--|CUT_BUF_THREAD|         |
**	-------------  ----------------         |
**	      |              |                  |
**	-------------  ----------------  ----------------
**	|CUT_INT_BUF|--|CUT_BUF_THREAD|--|CUT_BUF_THREAD|
**	-------------  ----------------  ----------------
**
** 	All CUT_INT_BUFs are linked together in a master buffer linked list,
**	there is one CUT_INT_BUF per active buffer in CUT.
**	
**	Each buffer contains a list of CUT_BUF_THREAD  structures, there is
**	one CUT_BUF_THREAD strcuture for each thread that is attached to each
**	buffer.
**
**	All CUT_TCBs are linked togehter in a master thread linked list, there
**	is one CUT_TCB for each thread that is active in CUT.
**
**	Each CUT_TCB contains a list of CUT_BUF_THREAD strcutures which describe
**	all of the buffers that thread is attached to. As the diagram shows,
**	these CUT_BUF_THREAD structures are the same as the ones hanging from
**	the CUT_INT_BUF, and the linked lists overlap.
**	
**	The CUT_BUF_THREAD structure contains a pointer to both the next
**	CUT_BUF_THREAD attached to the current buffer and the next 
**	CUT_BUF_THREAD attached to this thread (CUT_TCB).
**
** History:
**	26-may-98 (stephenb)
**	    Created.
**	jun-1998 (stephenb)
**	    alter macros CELL_OFFSET, EMPTY_CELLS, FULL_CELLS
**	20-may-99 (stephenb)
**	    replace nat and longnat with i4
**	19-Apr-2004 (schka24)
**	    Add wakeup counters to buffer structure.
**	26-Apr-2004 (schka24)
**	    Add writer-thread counter, rendezvous stuff to buffer.
**	15-Jul-2004 (jenjo02)
**	    CUT_BUFFER renamed to CUT_RCB for clarity.
*/

/*
** Forward refferences
*/
typedef struct	_CUT_TCB		CUT_TCB;
typedef struct	_CUT_INT_BUF		CUT_INT_BUF;
typedef struct	_CUT_BUF_THREAD		CUT_BUF_THREAD;

/*
** defines and macros
*/

/*
** Currently memory allocation for CUT structures simply grabs enough
** memory to contain CUT_ME_NUM_BLOCK structures, and then marks them used
** when they are in use. They never realy leave the "free list" as such, so
** freeing them again as simply a case of zeroing the memory, which unsets
** the used byte, THIS BYTE IS THE FIRTS BYTE IN EACH STRUCTURE MANAGED BY
** CUT, AND IT MUST REMAIN THAT WAY.
*/
#define		CUT_ME_NUM_BLOCKS	20 /* allocation size for CUT structs */

#define		FREE_BLOCK(size, block)		MEfill(size, 0, block)

/*
** offset of cell from tail in number of cells
*/
#define		CELL_OFFSET(cell_reads, thread_reads) \
			(thread_reads - cell_reads)
/*
** number of available cells in buffer
*/
#define		EMPTY_CELLS(num_cells, reads, writes) \
			(num_cells - (writes - reads))
/*
** number of occupied cells in buffer
*/
#define		FULL_CELLS(reads, writes) (writes - reads)
			
/*
** Name CUT_INT_BUF - CUT internal description of a buffer
**
** Description:
**	This structure contains the information about a communications buffer
**	that is internal to the CUT functions, this includes the semaphore
**	used to access the buffer, and other statistical information.
**	a generic pointer to this structure will be returned to the user
**	in the cut_buf_handle field in the CUT_RCB. There will be one of
**	these structures for each active buffer in CUT
**
**	Design Note:
**	The conditions on buffers fall broadly into two catagories, a read
**	condition (wait for tail to move forward, or buffer full wait), and
**	a write condition (wait for head to move forward, or buffer empty
**	wait). Since a condition free action may only wake a single thread,
**	and read threads can do nothing to help write threads (and vice-versa),
**	except, perhaps, re-signal the condition, it was decided to have
**	two conditions within the buffer. This way a thread may wait on the 
**	read condition or the write condition, and may signal either
**	indipendently. This means that we will not wake a thread waiting for
**	a read condition when we have completed a write action (and 
**	vice-versa), thus ensuring we don't get stuck by only waking a 
**	thread that can't help. Note that both conditions use the same
**	semaphore (is this legal ?).
**
**	Typically in a reader/writer setup, one end will be consistently
**	faster than the other.  When this happens, the faster end will
**	end up sleeping and being awakened on every cell, which is slow.
**	The reader/writer wakeup points reduce this thrashing by
**	deferring wakeup until the buffer cell read or write count has
**	reached a specified level.
**
**	Note on Semaphores:
**	There are two semaphores of interest involved in maintaining buffers,
**	the fisrt is the buffer list semaphore, which protects the linked list
**	of buffers (this is a static variable in cutinterf.c). The second
**	semaphore protects the buffer itself. To avoid semaphore deadlocks
**	and other mutex related inconsistencies, the following protocol should
**	be followed before working on any buffer:
**
**		 - Grab buffer list sem
**		 - Check for buffer in list
**		 - Grab buffer sem
**		 - Release buffer list sem
** History:
**	09-Sep-2004 (jenjo02)
**	    Added cut_trace boolean for some primitive r/w tracing.
**	27-Nov-2007 (jonj)
**	    Add cut_writers_attached boolean.
*/
#define	CUT_INT_BUF_TYPE	1
struct _CUT_INT_BUF
{
    bool		cut_in_use;	  /* buffer is in use, must be the first
					  ** field in the struct, (altered by
					  ** memory routines only) */
    bool		cut_sync_satisfied;  /* Rendezvous completed */
    bool		cut_trace;	/* Enable TRdisplay tracing */
    bool		cut_writers_attached;/* One or more writers have 
					     ** attached, may be gone now */
    CUT_INT_BUF		*buf_prev;
    CUT_INT_BUF		*buf_next;
    CS_SEMAPHORE	cut_buf_sem; /* mutex for this buffer */
    CS_CONDITION	cut_read_cond; /* wait for read condition */
    CS_CONDITION	cut_write_cond; /* wait for write condition */
    CS_CONDITION	cut_event_cond; /* general event condition */
    CUT_BUF_THREAD	*cut_thread_list; /* attached threads */
    CUT_RCB		cut_ircb; /* user external static buffer view */
    CUT_BUF_INFO	cut_buf_info; /* user external dynamic buffer info */
    i4			cut_buf_rwakeup;  /* Wake readers if this many writes */
    i4			cut_buf_wwakeup;  /* Wake writers if this many reads */
    i4			cut_buf_writers;  /* Number of attached writers */
    i4			cut_buf_syncers;  /* Number of CUT_SYNC waiters */
};

/*
** Name: CUT_BUF_THREAD - Buffer specific thread data
** 
** Description:
**	This structure contains buffer specific information about a thread that
**	is attached to a communications buffer. There will be one of these
**	structures for each attachment of a thread to each buffer.
**
** History:
**	6-may-98 (stephenb)
**	    Initial creation
**	15-Jul-2004 (jenjo02)
**	    Added cut_thread_flush boolean.
**	18-Aug-2004 (jenjo02)
**	    Added cut_flags, cut_num_cells, cut_async_status
**	    for DIRECT mode read/write. 
**	16-Sep-2004 (schka24)
**	    Added parent-thread indicator, for cleanup.
*/
#define	CUT_BUF_THREAD_TYPE	2
struct _CUT_BUF_THREAD
{
    bool		cut_in_use;	  /* buffer is in use, must be the first
					  ** field in the struct, (altered by
					  ** memory routines only) */
    bool		cut_thread_flush; /* Buffer is being flushed */
    bool		cut_parent_thread;  /* TRUE if thread is parent for
					  ** the buffer; mainly (only?) needed
					  ** for uncontrolled thread/CUT
					  ** termination */
    CUT_BUF_THREAD	*buf_next;	  /* next thread in this buffer */
    CUT_BUF_THREAD	*buf_prev;	  /* prev of above */
    CUT_BUF_THREAD	*thread_next;	  /* next buffer attached to thread */
    CUT_BUF_THREAD	*thread_prev;	  /* prev of above */
    CUT_INT_BUF		*cut_int_buffer;  /* buffer at head of buffer list */
    CUT_TCB		*cut_tcb;	  /* thread at head of thread list */
    i4			cut_thread_use;	  /* buffer use for thread,
    					  ** CUT_BUF_READ or CUT_BUF_WRITE
					  ** (does not include flags) */
    i4			cut_thread_state; /* current state in buffer */
    i4			cut_current_cell; /* current cell position of reader */
    i4			cut_cell_ops;	  /* reads or writes on buffer */
    i4			cut_flags;	  /* "flags" from last R/W */
    i4			cut_num_cells;	  /* Last num_cells read/writ */
    DB_STATUS		*cut_async_status; /* Pointer to async status */
};

/*
** Name: CUT_TCB - CUT Thread control block
**
** Description:
**	The structure contains all the CUT control information for a thread.
**	There will be one of these structure for each thread that is active
**	in CUT.
**
** 	Notes on semaphores: 
**	The data in this control block, and the list of CUT_THREAD control
**	blocks that it points to needs no semaphore protection because, by
**	definition, only one thread will be accessing it at a time. THIS
**	SITUATION CHANGES IF A THREAD CAN ACCESS ANOTHER THREADS DATA.
**	The only exception to this is the error case (when thread status is
**	being checked or set), at this time, this will be the only semaphore
**	held.
**
**	If thread master list semaphore, the buffer master list semaphore 
**	and a buffer semaphore are all required. In order to avoid dealocks
**	and other semaphore related problems, the caller must grab the
**	thread master list semaphore first, and then follow the semaphore
**	protocol describe in CUT_INT_BUF for the other semaphores.
**
**	Note on Session ID
**	In all of the CUT code, a CS_SID is assumed to be a single data-type
**	that has been defined or typedef'd in the CL (i.e. it is not a 
**	structure). This holds for all known platforms that Ingres runs on.
**	The values of CS_SID will be 32 bits or 64 bits on the relavent 
**	architectures with Ingres threads, or system returned thread number
**	when using OS threads (normaly an int). Either way, CS_SID's can be
**	comapred to eachother without having to use MEcmp(), we will assume
**	we can simply say "if (sid1 == sid2)".
**
** History:
**	11-jun-98 (stephenb)
**	    Created
**	22-Mar-2007 (kschendel) b122040
**	    Add being-signaled boolean to deal with signal/terminate race
*/
#define	CUT_TCB_TYPE		3
struct _CUT_TCB
{
    bool		cut_in_use;	/* buffer is in use, must be the first
					** field in the struct, (altered by
					** memory routines only) */
    bool		being_signalled;  /* Set TRUE when cut signaler has
					** decided to send an ATTN to this
					** thread.  A thread engaged in a
					** terminate will see this and wait
					** until this goes back FALSE before
					** actually terminating. */
    CUT_TCB		*thread_next;	/* next TCB in list */
    CUT_TCB		*thread_prev; 	/* previous TCB in list */
    CS_SEMAPHORE	cut_tcb_sem; 	/* used only in error case when 
					** setitng status */
    CS_SID		cut_thread_id;	/* session ID for thread */
    DB_STATUS		cut_thread_status; /* current status of thread */
    CUT_BUF_THREAD	*cut_buf_list;  /* pointer to list of buffers thread 
					** is attached to */
};
