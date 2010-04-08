/*
**      Copyright (c) 2004, Ingres Corporation
*/

/**
**
** Name:  QEFEXCH.H   Typedefs for QEF parallel query processing.
**
** Description:
**
**	This file contains the control block that is used to preserve data
**	across calls to the exchange node during parallel processing.
**	After the exchange node launches children to handle query execution
**	of the outer tree branch, it returns so that other query processing
**	can continue. This block, stored in the DSH_CBS, allows the node
**	state to persist across calls.
**    
** History:
** 
**      9-feb-04 (toumi01)
**          Written.
**	8-apr-04 (toumi01)
**	    Add bool to track eating of first FUNC_RESET.
**	    Delete function and parent_thread as no longer needed.
**	28-apr-04 (toumi01)
**	    Add fields for 1:n parallelism.
**	19-may-04 (toumi01)
**	    Add debugging trace field for QE95.
**	02-Jun-2004 (jenjo02)
**	    Renamed CUT_BUFFER to CUT_RCB for clarity.
**	    Added cut_attached.
**	18-Aug-2004 (jenjo02)
**	    Implemented CUT DIRECT mode for Parent reads
**	    of DATA buffer.
**	09-Sep-2004 (jenjo02)
**	    Added cut_trace boolean, set with QE97.
**	    Deleted parent_cut_wait.
**	17-Sep-2004 (jenjo02)
**	    Change cut_attached from boolean to integer.
**	04-Apr-2006 (jenjo02)
**	    Moved BUFF_HDR, CHILD_DATA struture defines here
**	    from qenexch.c.
**	    Modified all structures to accomodate one CUT
**	    data buffer per child.
*/

typedef struct _EXCH_CB	EXCH_CB;
typedef struct _BUFF_HDR BUFF_HDR;
typedef struct _CHILD_DATA CHILD_DATA;

/* Prefixes each row in data buffer */
struct _BUFF_HDR
{
    DB_STATUS	status;
    i4		err_code;
};

/* One of these for each attached Child thread */
struct _CHILD_DATA
{
    i4		threadno;		/* Child's identification, 1-n */
    EXCH_CB	*exch_cb;		/* Pointer to EXCH_CB */
					/* The remaining elements are for
					** the sole use of the Parent thread
					*/
    DB_STATUS	status;			/* Status from BUFF_HDR */
    i4		row_count;		/* Rows returned by this Child */
    i4		cells_remaining;	/* Cells not yet read by Parent */
    PTR		next_cell;		/* Next cell to be read by Parent */
    CUT_RCB	data_rcb;		/* Parent's data buffer rcb */
};

/* Structure shared by Parent and Kids */
struct _EXCH_CB
{
    QEN_NODE		*node;
    QEF_RCB		*rcb;
    void		*dsh;		/* DSH orignal from root node */
    i4			ascii_id;             
#define			EXCH_ASCII_ID	CV_C_CONST_MACRO('#', 'E', 'X', 'C')
    CS_SID		parent_sid;	/* Session ID of Parent */
    DB_STATUS		high_status;	/* higest child status */
    i4			high_err_code;	/* ... and associated err_code */
    i4			cchild_running;	/* count of running children */
    i4			cut_attached;	/* count of expected attaches*/
    bool		trace;		/* debugging trace */
    bool		cut_trace;	/* CUT debugging trace */
    bool		eat_reset;	/* tracks eating of first reset */
    CHILD_DATA		*child_data;	/* Array of thread-safe Kid data */
    i4			curr_child;	/* Which child parent is working on */
    CUT_RCB		comm_rcb;	/* Parent->Kid communications */
};
