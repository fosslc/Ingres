/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: ULMINT.H - Internal typedefs and defines needed by ULM.
**
** Description:
**      This file contains definitions of all of the structures and
**      symbolic constants needed by the Utility Library Memory
**      Management Module (ULM).  This file depends on the following
**	files already being #included:
**		dbms.h
**		cs.h	(for the CS_SEMAPHORE in ULM_PHEAD)
**
**      This file defines the following typedefs:
**	    ULM_NODE - Allocation node
**	    ULM_HEAD - Head for allocated and free node chains
**	    ULM_STREAM - Memory stream
**	    ULM_CHUNK - Memory stream chunk (large, contiguous section
**			of a memory pool)
**	    ULM_BLOCK - Memory stream block
**	    ULM_PHEAD - Memory pool header
**	    ULM_I_SMARK - Stream mark; internal representation.
**
** History: $Log-for RCS$
**	08-apr-86 (thurston)
**          Initial creation.  Some typedefs taken from Jeff Lichtman's
**          ULM.C file.
**	01-aug-86 (thurston)
**	    In the ULM_PHEAD typedef, I changed the name of member
**	    .ulm_psize_512 to be .ulm_psize_pag, since we are now using
**	    the constant SCU_MPAGESIZE instead of 512.
**	17-jul-87 (thurston)
**	    Added ULM_I_SMARK.
**	29-oct-87 (thurston)
**	    Added ULM_CHUNK, and associated constants, and fields in the
**	    ULM_PHEAD structure.
**      02-nov-87 (thurston)
**	    Added the standard structure header to the ULM_CHUNK typedef.
**	04-nov-87 (thurston)
**	    Added the .ulm_pchsize_pag field to the ULM_PHEAD typedef.
**	28-nov-87 (thurston)
**	    Changed the SCF_SEMAPHORE to a CS_SEMAPHORE in ULM_PHEAD typedef,
**	    to enable easier usage of CS{p,v}_semaphore() routines instead of
**	    going through SCF.
**	12-jan-88 (thurston)
**	    Added the .ulm_nprev pointer to the ULM_NODE typedef.
**	17-may-1993 (rog)
**	    Increase the default chunk size to 32K.
**	07-Aug-1997 (jenjo02)
**	    Added ulm_self to ULM_PHEAD and ULM_STREAM so ULM can sanity
**	    check these structures.
**	14-Aug-1997 (jenjo02)
**	    Added ulm_sblspa to ULM_STREAM to track first block with space.
**	15-Aug-1997 (jenjo02)
**	    Introduced the notion of private and shareable streams. Private streams
**	    are assumed to be thread-safe while shareable streams are not. It
**	    is the stream creator's responsibility to define this characteristic
**	    when the stream is created by ulm_openstream(). Unless defined explicitly
**	    as ULM_PRIVATE_STREAM, all streams will be assumed to be shareable
**	    and will be mutex protected; private streams do not incur the overhead
**	    of mutex protection.
**	    For this, added ulm_ssem, ulm_flags to ULM_STREAM.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Apr-2004 (stial01)
**          Define ulm_length as SIZE_TYPE.
**	9-feb-2007 (kibro01) b113972
**	    Add ulm_stream_marker to detect a twice-closed stream without
**	    crashing.
**	11-feb-2008 (smeke01) b113972
**	    Backed out above change.
**/


/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _ULM_BLOCK ULM_BLOCK;


/*}
** Name: ULM_NODE - Allocation node
**
** Description:
**      One of these structures will exist at the head of every piece of
**	memory allocated with ulm_alloc().
**
** History:
**	18-mar-86 (jeff)
**          adapted from ME.c
**	08-apr-86 (thurston)
**          Extracted from ULM.C.
**	12-jan-88 (thurston)
**	    Added the .ulm_nprev pointer.
*/
typedef struct _ULM_NODE
{
    struct _ULM_NODE  *ulm_nnext;       /* pointer to next node in chain */
    struct _ULM_NODE  *ulm_nprev;       /* pointer to prev node in chain */
    i4         ulm_nsize;          /* size of node (including head) */
    i4		    ulm_nowner;		/* Facility code of memory owner */
}   ULM_NODE;


/*}
** Name: ULM_HEAD - Head for allocated and free node chains
**
** Description:
**	This structure is used to keep track of the lists of allocated and
**	free nodes.  These list heads are put at the beginning of each pool
**	(i.e.  in the pool header).
**
**      NOTE: ulm_last is used to add nodes to the end of the allocated
**	chain.  The free chain doesn't use it because nodes are inserted
**	so that successive nodes start at increasing memory locations.
**
** History:
**	18-mar-86 (jeff)
**          adapted from ME.c
*/
typedef struct _ULM_HEAD
{
    ULM_NODE        *ulm_h1st;           /* Pointer to 1st node in chain */
    ULM_NODE	    *ulm_hlast;          /* Pointer to last node in chain */
}   ULM_HEAD;


/*}
** Name: ULM_STREAM - Memory stream
**
** Description:
**      One of these structures will exist for each memory stream in the
**      memory pool.  It is used to hold a list of blocks allocated to the
**      stream, as well as keeping certain statistics about the stream.
**
** History:
**	08-apr-86 (thurston)
**          Initial creation.
**	07-Aug-1997 (jenjo02)
**	    Added ulm_self to ULM_PHEAD and ULM_STREAM so ULM can sanity
**	    check these structures.
**	14-Aug-1997 (jenjo02)
**	    Added ulm_sblspa to ULM_STREAM to track first block with space.
**	15-Aug-1997 (jenjo02)
**	    Introduced the notion of private and shareable streams. Private streams
**	    are assumed to be thread-safe while shareable streams are not. It
**	    is the stream creator's responsibility to define this characteristic
**	    when the stream is created by ulm_openstream(). Unless defined explicitly
**	    as ULM_PRIVATE_STREAM, all streams will be assumed to be shareable
**	    and will be mutex protected; private streams do not incur the overhead
**	    of mutex protection.
**	    For this, added ulm_ssem, ulm_flags to ULM_STREAM.
*/
typedef struct _ULM_STREAM
{
    struct _ULM_PHEAD  *ulm_spool;	/* Pointer to pool stream belongs to */
    struct _ULM_STREAM *ulm_self;	/* Self-pointer for validation */
    struct _ULM_STREAM *ulm_sstnext;    /* Next stream in pool */
    struct _ULM_STREAM *ulm_sstprev;    /* Previous stream in pool */
    struct _ULM_BLOCK  *ulm_sbl1st;     /* 1st block in stream */
    struct _ULM_BLOCK  *ulm_sbllast;    /* Last block in stream */
    struct _ULM_BLOCK  *ulm_sblcur;     /* Current "use" block in stream.  This
					** will differ from last block ONLY if
					** a ulm_reinitstream() call has been
					** on this stream.
					*/
    struct _ULM_BLOCK  *ulm_sblspa;	/* First block, in addition to ulm_sblcur,
					** with available space. */
    i4         ulm_snblks;         /* # blocks in stream */
    i4         ulm_sblsize;        /* Prefered size of data blocks for this
					** stream, in bytes.  This overrides the
					** pools preferred size, and can be
					** overridden by a request for a piece
					** that is larger than this block size.
					*/
    i4		    ulm_flags;		/* Flags describing this stream. */
					/* (see defines in ulm.h)        */
    CS_SEMAPHORE    ulm_ssem;		/* Shareable stream mutex, present
					** only in shareable streams */
}   ULM_STREAM;


/*}
** Name: ULM_CHUNK - Memory chunk
**
** Description:
**      One of these structures will exist at the beginning of each large,
**	contiguous chunk of memory allocated from SCF for the memory pool.
**
** History:
**	29-oct-87 (thurston)
**          Initial creation.
**      02-nov-87 (thurston)
**	    Added the standard structure header to this typedef.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _ULM_CHUNK
{
/********************   START OF STANDARD HEADER   ********************/

    struct _ULM_CHUNK  *ulm_cchnext;    /* Next chunk in pool */
    struct _ULM_CHUNK  *ulm_cchprev;    /* Previous chunk in pool */
    SIZE_TYPE           ulm_length;     /* Length of this structure: */
    i2              ulm_type;		/* Type of structure.  For now, 3245 */
#define                 ULCHK_TYPE      3245	/* for no particular reason */
    i2              ulm_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             ulm_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             ulm_owner;          /* Owner for internal mem mgmt. */
    i4              ulm_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 ULCHK_ASCII_ID  CV_C_CONST_MACRO('U', 'L', 'C', 'H')

/*********************   END OF STANDARD HEADER   *********************/

    struct _ULM_PHEAD  *ulm_cpool;	/* Pointer to pool chunk belongs to */
    SIZE_TYPE          ulm_cpages;         /* Size of this chunk,
					** in SCU_MPAGESIZE byte pages
					*/
}   ULM_CHUNK;


/*}
** Name: ULM_BLOCK - Memory stream block
**
** Description:
**      One of these structures will exist at the head of each data block
**	in a memory stream.  It will serve as a mechanism to chain all of
**      the blocks for a stream, and will also be used to keep track of how
**      much of the associated data block has been allocated via ulm_palloc(),
**      and how much remains to be allocated.
**
** History:
**	08-apr-86 (thurston)
**          Initial creation.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _ULM_BLOCK
{
    struct _ULM_BLOCK  *ulm_bblnext;    /* Next block in stream */
    struct _ULM_BLOCK  *ulm_bblprev;    /* Previous block in stream */
    ULM_STREAM      *ulm_bstream;       /* Stream block belongs to */
    i4         ulm_bblsize;        /* Actual size of the associated 
					** (read, "following") data block,
					** in bytes
					*/
    i4         ulm_bremain;        /* # bytes remaining free in the
					** associated data block
					*/
}   /*ULM_BLOCK*/;


/*}
** Name: ULM_PHEAD - Memory pool header
**
** Description:
**      This structure appears at the head of every memory pool.  It contains
**	the free list, allocated list, and other goodies.
**
** History:
**	20-mar-86 (jeff)
**          written
**	08-apr-86 (thurston)
**	    Extracted from ULM.H.  Also, added the ulm_pst1st and ulm_pstlast
**	    members; used to keep track of all memory streams in the pool.
**	    Also added the ulm_pblsize member; used to set a preferential size
**	    to make data blocks for the pool in hopes of preventing too much
**	    fragmentation.
**	01-aug-86 (thurston)
**	    Changed the name of member .ulm_psize_512 to be .ulm_psize_pag,
**	    since we are now using the constant SCU_MPAGESIZE instead of 512.
**	29-oct-87 (thurston)
**	    Added the fields needed to support memory pools that do not get
**	    fully allocated at startup time; those dealing with "chunks":
**				.ulm_pages_alloced
**				.ulm_pchunks
**				.ulm_pch1st
**				.ulm_pchlast
**	04-nov-87 (thurston)
**	    Added the .ulm_pchsize_pag field.
**	28-nov-87 (thurston)
**	    Changed the SCF_SEMAPHORE to a CS_SEMAPHORE, to enable easier usage
**	    of CS{p,v}_semaphore() routines instead of going through SCF.
**	07-Aug-1997 (jenjo02)
**	    Added ulm_self to ULM_PHEAD and ULM_STREAM so ULM can sanity
**	    check these structures.
**	22-May-2007 (wanfr01)
**	    Bug 118380
**	    Track smallest size request for this facility.
*/
typedef struct _ULM_PHEAD
{
    i4              ulm_powner;         /* Facility code of pool owner */
    struct _ULM_PHEAD *ulm_self;	/* Self-pointer for validation */
    SIZE_TYPE       ulm_psize_pag;      /* Total size of pool (max size),
					** in SCU_MPAGESIZE byte pages
					*/
    SIZE_TYPE       ulm_pages_alloced;  /* Total number of pages allocated for,
					** the pool, in SCU_MPAGESIZE byte pages
					*/
    i4		    ulm_minreq;		/* Smallest size request for this pool */
    i4		    ulm_pchunks;	/* # of chunks allocated for pool */
    SIZE_TYPE       ulm_pchsize_pag;    /* Prefered size of chunks for this
					** pool, in SCU_MPAGESIZE byte pages.
					*/
    i4         ulm_pblsize;        /* Prefered size of data blocks for
					** this pool, in bytes.  This can be
					** overridden for any stream when
					** that stream is opened with a
					** ulm_openstream() call.
					*/
#define                 ULM_BLSIZE_DEFAULT 512	/* This will be used as the   */
						/* default preferred size of  */
						/* data blocks, if the caller */
						/* of ulm_startup() specifies */
						/* zero as the preferred size.*/

    CS_SEMAPHORE    ulm_psem;		/* Mutex semaphore to avoid corruption*/
    ULM_HEAD        ulm_pallist;        /* Allocated node list */
    ULM_HEAD        ulm_pfrlist;        /* Free node list */
    ULM_STREAM      *ulm_pst1st;        /* 1st stream in pool's stream list */
    ULM_STREAM      *ulm_pstlast;       /* Last stream in pool's stream list */
    ULM_CHUNK       *ulm_pch1st;        /* 1st chunk in pool's chunk list */
    ULM_CHUNK       *ulm_pchlast;       /* Last chunk in pool's chunk list */
}   ULM_PHEAD;


/*}
** Name: ULM_I_SMARK - Stream mark; internal representation.
**
** Description:
**      This is what a ULM_SMARK "really" looks like.
**
** History:
**	17-jul-87 (thurston)
**	    Initial creation.
*/
typedef struct _ULM_I_SMARK
{
    ULM_STREAM      *ulm_mstream;       /* pointer to stream this mark is for */
    ULM_BLOCK       *ulm_mblock;        /* pointer to current block for mark  */
    i4	    ulm_mremain;        /* # bytes remaining in current block */
}   ULM_I_SMARK;


/*
**  Defines of other constants.
**
** History:
**	17-may-1993 (rog)
**	    Increase the default chunk size to 32K.
**	27-aug-1993 (rog)
**	    Removed definition of ULM_CMB_CHUNK_MIN_BYTES and ULM_EXTENT_PAGES.
**	15-Aug-1997 (jenjo02)
**	    Added ULM_SX_SP, ULM_SX_SS, etc, to distinguish between SHARED
**	    streams (which contain a semaphore) and PRIVATE streams (which do not).
*/

/*
**      These constants are used to assure that the memory allocated to all
**      structs that ULM can possibly store in the memory pool are divisible
**      by sizeof(ALIGN_RESTRICT) (which is also known as, DB_ALIGN_SZ).
*/
#define  ULM_SX_P      sizeof(ULM_PHEAD)
#define  ULM_SX_N      sizeof(ULM_NODE)
#define  ULM_SX_SP     (sizeof(ULM_STREAM) - sizeof(CS_SEMAPHORE))
#define  ULM_SX_SS     sizeof(ULM_STREAM)
#define  ULM_SX_C      sizeof(ULM_CHUNK)
#define  ULM_SX_B      sizeof(ULM_BLOCK)

#define  ULM_MD_P      (ULM_SX_P % DB_ALIGN_SZ)
#define  ULM_MD_N      (ULM_SX_N % DB_ALIGN_SZ)
#define  ULM_MD_SP     (ULM_SX_SP % DB_ALIGN_SZ)
#define  ULM_MD_SS     (ULM_SX_SS % DB_ALIGN_SZ)
#define  ULM_MD_C      (ULM_SX_C % DB_ALIGN_SZ)
#define  ULM_MD_B      (ULM_SX_B % DB_ALIGN_SZ)

#define  ULM_SZ_PHEAD  (ULM_MD_P ? ULM_SX_P + DB_ALIGN_SZ - ULM_MD_P : ULM_SX_P)
#define  ULM_SZ_NODE   (ULM_MD_N ? ULM_SX_N + DB_ALIGN_SZ - ULM_MD_N : ULM_SX_N)
#define  ULM_SZ_STREAMS (ULM_MD_SS ? ULM_SX_SS + DB_ALIGN_SZ - ULM_MD_SS : ULM_SX_SS)
#define  ULM_SZ_STREAMP (ULM_MD_SP ? ULM_SX_SP + DB_ALIGN_SZ - ULM_MD_SP : ULM_SX_SP)
#define  ULM_SZ_CHUNK  (ULM_MD_C ? ULM_SX_C + DB_ALIGN_SZ - ULM_MD_C : ULM_SX_C)
#define  ULM_SZ_BLOCK  (ULM_MD_B ? ULM_SX_B + DB_ALIGN_SZ - ULM_MD_B : ULM_SX_B)
