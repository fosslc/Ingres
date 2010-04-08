/*
** Copyright (c) 1999, 2003 Ingres Corporation
*/

/*
** Name: digather.h - defines to be used with gatherWrite
**
** Description:
**      This file contains the definitions of datatypes used with 
**      gatherWrite.
**
** History:
**	05-nov-1999 (somsa01)
**	    Created from the UNIX version.
**	30-apr-2001 (somsa01)
**	    Added gio_gw_pages, gio_io_count to DI_GIO.
**	24-sep-2003 (somsa01)
**	    Removed EVCOMP_PARAM. Changed WaitArray[] to AsynchIOWaitArray[].
*/

/*
**  Defines of other constants.
*/
typedef	struct	_DI_TGIO	DI_TGIO;
typedef	struct	_DI_GIO		DI_GIO;

/* Max number of queued writes before force */
#define		GIO_MAX_QUEUED	256


/*}
** Name: DI_TGIO - Thread's GatherWrite CB.
**
** Description:
**	Each thread doing GatherWrite is allocated one of these, 
** 	and it's anchored in the thread's CS_SCB.
**	As requests are received, they are queued into tgio_todo
**	until GIO_MAX_QUEUED is reached, at which time the
**	writes are forced. The queued writes may be forced at
**	other times as well.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
**	09-Jul-1999 (jenjo02)
**	    Added TGIO_UNORDERED flag.
**	10-dec-1999 (somsa01)
**	    Added WaitArray[].
**	24-sep-2003 (somsa01)
**	    Renamed WaitArray[] to AsynchIOWaitArray[].
*/
struct _DI_TGIO
{
    DI_TGIO 	    *tgio_next;		/* Ptr to next DI_TGIO */
    CS_SCB	    *tgio_scb;		/* Owning thread's SCB */
    i4		    tgio_state;	        /* State of the list: */
#define		TGIO_INACTIVE       0x00000 /* not in use, available */
#define		TGIO_ACTIVE	    0x00001 /* gatherwrite in progress*/
#define		TGIO_UNORDERED	    0x00002 /* Queue must be sorted */
    i4		    tgio_queued;	/* Number of GIOs queued */
    i4		    *tgio_uqueued;	/* Pointer to caller's queued count */
    DI_GIO	    *tgio_queue[GIO_MAX_QUEUED];
    HANDLE	    AsynchIOWaitArray[MAXIMUM_WAIT_OBJECTS];
};

/*}
** Name: DI_GIO - GatherWrite I/O request.
**
** Description:
**	This structure contains the information for a single
**	gatherWrite request.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
**	30-apr-2001 (somsa01)
**	    Added gio_gw_pages, gio_io_count.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
struct _DI_GIO
{
    DI_IO	*gio_f;
    VOID	(*gio_evcomp)();	/* I/O completion function */
    PTR		gio_data;		/* Callback data */
    OFFSET_TYPE gio_offset;		/* File offset to write */
    OFFSET_TYPE	gio_len;		/* Length to write */
    char	*gio_buf;		/* Buffer to be written */
    u_i8	*gio_gw_pages;		/* Buffers written using GW */
    u_i8	*gio_io_count;		/* Physical I/O counter */

};
