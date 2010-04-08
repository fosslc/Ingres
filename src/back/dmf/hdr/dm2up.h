/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2UP.H - Definitions for parallel create index utility routines.
**
** Description:
**      This file decribes the routines that creates the parallel index
**	table structures.
**
** History:
**      10-jan-1998 (nanpr01)
**          Created Parallel Index Build.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-dec-2005 (stial01)
**          Increase number of bits in exch_visit so we can create > 32 indices
**          concurrently (b115572)
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
*/

/*
**  Forward typedef references.
*/
typedef struct  _CHILD_CB	CHILD_CB;
typedef struct  _EXCH_BUF_HDR   EXCH_BUF_HDR;
typedef struct  _EXCH_BUF 	EXCH_BUF;


/*}
** Name: exch_buf
**
** Description:
**      This structure is used to describe the individual exchange buffers.
**	This contains its size, the number in its list and a count field
**	which all readers increment when that buffer is being read and 
**	decremented when done.
**
** History:
**      10-apr-1998 (nanpr01)
**          Created for Parallel Index Build.
**      05-may-2000 (stial01)
**          Added rec_cnt to prototype for dm2u_pload_table
*/
struct _EXCH_BUF {
    CS_SEMAPHORE        exch_buf_mutex;	    /* Mutex to protect sim access */
    i4             exch_bufno;	    /* buffer number for the exch buf */
    char                *exch_buffer;	    /* actual exchange buffer      */
    i4	 	exch_noofrec;	    /* no of record holding        */
    CS_SEMAPHORE        exch_cnt_mutex;	    /* mutex to protect count      */
    i4             exch_usr_cnt;	    /* no of readers to read from 
					    ** this buffer
					    */
    char	exch_visit[512/BITSPERBYTE];/* Who visited this buffer
					    ** b115572: increase max concurrent
					    ** indexes to 512
					    */
};

/*}
** Name: exch_hdr
**
** Description:
**      This structure is used to describe the exchange block. It describes 
**	how many exchange block is there and its beginning address. Also
**	the header keeps track of the status of the parent thread should
**	a wakeup is needed.
**
** History:
**      10-apr-1998 (nanpr01)
**          Created for Parallel Index Build.
*/
struct _EXCH_BUF_HDR
{
    i4      ehdr_noofbufs;		/* No of exchange buffers           */
    i4      ehdr_rec_capacity;	/* capacity of each exchange buffer */
    i4      ehdr_size;		/* Size of each exchange buffer     */
    EXCH_BUF     *ehdr_exch_buffers;
    i4      ehdr_parent_status;	/* if this is non-zero, parent is 
					** waiting on that buffer */
    CS_SEMAPHORE ehdr_cwait_mutex;	/* mutex for parent to synchro-
					** nize children execution
					*/
    i4	 ehdr_cwait_count;	/* Contains wait count */
    i4	 ehdr_error_flag;
#define	PARENT_ERROR			0x00000001L
#define	CHILD_ERROR			0x00000002L
};

/*}
** Name: child_cb 
**
** Description:
**      This structure is used to describe the control block passed to the
**	child. In this control block, mxcb address and the exchange buffer
**	header is passed
**
** History:
**      10-apr-1998 (nanpr01)
**          Created for Parallel Index Build.
*/
struct _CHILD_CB {
	i4			ccb_noofchild;	/* No of fac. thread created */
	EXCH_BUF_HDR		*ccb_exch_hdr;	/* Exchange Buffer header    */
	DM2U_MXCB		*ccb_mxcb;	/* mxcb for each create ind  */
	i4			ccb_childno;	/* This contains the children
						** number
					 	*/
	i4			ccb_thread_id;  /* child thread id     */
	DB_STATUS		ccb_status;	/* child status        */
	DB_ERROR		ccb_error;	/* child error array   */
};


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN DB_STATUS   dm2u_pindex(DM2U_INDEX_CB   *index_cbs);
FUNC_EXTERN DB_STATUS 	dm2u_pload_table(DM2U_MXCB *mxcbs, i4 *rec_cnt, 
					DB_ERROR *dberr);
