/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/* Multiple-inclusion protection: */
#ifndef INCLUDE_ULM_H
#define INCLUDE_ULM_H

/**
** Name: ULM.H - control block for ULF memory management routines
**
** Description:
**      This file contains the request control block to be used to make calls
**      to the "Utility Library Facility's Memory Management" routines (ULM).
**
** History: $Log-for RCS$
**      27-mar-86 (seputis)
**          initial creation
**	10-apr-86 (thurston)
**	    Changed the name of ULM_CB to ULM_RCB.  Also, many fields of the
**	    ULM_RCB added or modified.  Nothing terribly important.
**	17-jul-87 (thurston)
**	    Added ULM_SMARK, and extended ULM_RCB by adding .ulm_smark member.
**	28-aug-92 (rog)
**	    Prototype ULF.
**	15-oct-1993 (rog)
**	    Add prototype for ulm_copy().
**	19-nov-93 (swm)
**	    Bug #58637
**	    The ULM_SMARK type is treated as a ULM_I_SMARK internally by
**	    the ulf facility (by casting). Therefore, we must ensure that
**	    sizeof(ULM_SMARK) >= sizeof(ULM_I_SMARK). Since ULM_I_SMARK
**	    contains pointers (which are bigger than i4s on platforms
**	    such as axp_osf), change type of ulm_sbuf array elements from
**	    i4 to PTR.
**	12-may-1994 (rog)
**	    Add prototype for ulm_chunk_init().
**	11-Aug-1997 (jenjo02)
**	    Added ulm_streamid_p which caller can supply to point to the
**	    address of a streamid. When the stream is deallocated, this
**	    address will be nullified by ULM.
**	15-Aug-1997 (jenjo02)
**	    Introduced the notion of private and shareable streams. Private streams
**	    are assumed to be thread-safe while shareable streams are not. It
**	    is the stream creator's responsibility to define this characteristic
**	    when the stream is created by ulm_openstream(). Shareable streams
**	    will be mutex-protected by ULM, private streams will not.
**	    To this end, added ulm_flags and defines to ULM_RCB.
**	    This change obviates the need for ulm_spalloc(), which has been removed.
**	    Also added a piggyback function to ulm_openstrem() by which one may
**	    allocate a piece of the stream during the same call by specifying the
**	    flag ULM_OPEN_AND_PALLOC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Jan-2004 (schka24)
**	    Add standard double-include protection
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      17-Jul-2206 (hanal04) Bug 116358
**          Prototype ulm_print_pool(). Quiets the compiler warnings in
**          QSF and QEF.
**/

/*}
** Name: ULM_SMARK - Stream mark.
**
** Description:
**      This structure will be filled in by ulm_mark() and represent a memory
**	stream (marked at the current stream position).  The ULM client keeps
**	this filled in structure around, and may later pass it to ulm_reclaim()
**	to reclaim memory allocated to that stream subsequent to the mark.
**
** History:
**	17-jul-87 (thurston)
**	    Initial creation.
*/
typedef struct _ULM_SMARK
{
    PTR         ulm_sbuf[4];		/* Buffer for ULM routines to create
					** and manage a stream mark in.
					*/
}   ULM_SMARK;

/*}
** Name: ULM_RCB - request control block used to interface with ULM
**
** Description:
**      This control block is passed to each of the memory management routines.
**      This is done since most of the parameters are passed repetitively
**      and it is less error prone and moves less data if this information
**      is passed in a control block.
**
** History:
**      27-mar-86 (seputis)
**          initial creation
**	10-apr-86 (thurston)
**	    Changed the name of ULM_CB to ULM_RCB.  Also, many fields of the
**	    ULM_RCB added or modified.  Nothing terribly important.
**	17-apr-86 (thurston)
**	    Changed "char *" 's to be "PTR" 's for poolid, streamid, and pptr.
**	17-jul-87 (thurston)
**	    Extended this structure by adding the .ulm_smark member.
**	11-Aug-1997 (jenjo02)
**	    Added ulm_streamid_p which caller can supply to point to the
**	    address of a streamid. When the stream is deallocated, this
**	    address will be nullified by ULM.
**	15-Aug-1997 (jenjo02)
**	    Added ulm_flags and defines for private and shared streams.
*/
typedef struct _ULM_RCB
{
    PTR             ulm_poolid;         /* Pool id used to identify pool from
                                        ** which streams will be allocated.
                                        */
    PTR             *ulm_streamid_p;    /* Pointer to Stream id used to identify stream
                                        ** from which pieces will be allocated.
                                        */
    PTR		    ulm_streamid;	/* A handy placeholder for streamid */
    PTR             ulm_pptr;           /* Pointer to piece of memory which
                                        ** has been allocated by ulm_palloc().
					** ULM guarantees this memory will be
                                        ** aligned on a sizeof(ALIGN_RESTRICT)
                                        ** byte boundry. 
                                        */
    SIZE_TYPE       *ulm_memleft;	/* Pointer to counter to be decremented
                                        ** every time memory is assigned from
                                        ** the pool to a stream, and incremented
                                        ** every time memory is released from a
                                        ** stream (i.e. a stream is closed).  A
                                        ** pointer is used since many streams
                                        ** may need to point to the same
                                        ** counter. 
                                        */
    SIZE_TYPE  ulm_sizepool;       /* Size in bytes of pool to create.
					*/
    i4         ulm_blocksize;      /* Used to set the preferred size
					** of data blocks in the pool (at
                                        ** ulm_startup() time), or for a
                                        ** specific stream (at ulm_openstream()
                                        ** time).  If zero at ulm_startup()
                                        ** time, ULM will choose a size for the
                                        ** pool; if zero at ulm_openstream()
                                        ** time, ULM will set the stream's
                                        ** block size to equal that of the pool.
                                        */
    i4              ulm_facility;       /* Facility which owns the pool.
					** (Use constants in DBMS.H.)
					*/
    i4              ulm_psize;          /* Size of piece being requested from
                                        ** memory stream via ulm_palloc().
                                        */
    DB_ERROR        ulm_error;          /* Standard DB_ERROR structure.
					*/
    ULM_SMARK       *ulm_smark;         /* Ptr to a `stream mark' structure.
                                        ** This structure is used only by
                                        ** ulm_mark(), which fills it in with
                                        ** the current stream mark, and by
                                        ** ulm_reclaim(), which reclaims memory
                                        ** from a stream up to the mark. 
					*/
    i4		    ulm_flags;		/* Flags for ulm_openstream: */
#define		ULM_PRIVATE_STREAM	0x0001  /* Private, thread-safe stream */
#define		ULM_SHARED_STREAM	0x0002  /* Shared, non-thread-safe stream */
#define		ULM_OPEN_AND_PALLOC	0x0004  /* Open stream and allocate memory */
}   ULM_RCB;


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS ulm_chunk_init( i4  chunk_size );
FUNC_EXTERN DB_STATUS ulm_closestream( ULM_RCB *ulm_rcb );
FUNC_EXTERN VOID      ulm_copy( PTR source, i4 nbytes, PTR dest );
FUNC_EXTERN DB_STATUS ulm_mark( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_mappool( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_print_pool( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_openstream( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_palloc( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_reclaim( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_shutdown( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_startup( ULM_RCB *ulm_rcb );
FUNC_EXTERN DB_STATUS ulm_xcleanup( ULM_RCB *ulm_rcb );

#ifdef NOT_YET_READY
FUNC_EXTERN DB_STATUS ulm_szstream( PTR stream_id, i4 *nbytes,
				    i4 *nnodes );
#endif

/* Inclusion protection for entire file */
#endif
