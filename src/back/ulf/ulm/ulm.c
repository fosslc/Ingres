/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulmint.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <ex.h>
#include    <er.h>


#define			PAD_CHAR	    0x7F
#define		ULM_FREELIST_TOO_LONG	1000

/**
**
**  Name: ULM.C - Memory management functions for the server.
**
**  Description:
**    The server assigns a pool of memory for each facility to use.  These
**    functions manage memory within th.c. The entire amount of memory is
**    not necessarily allocated at startup time.  Instead, ULM gets large
**    "chunks" of contiguous memory from SCF on an "as needed" basis, until the
**    total size of these chunks has reached the size of the memory pool set at
**    startup time. The idea behind the memory management scheme of ULM, is that
**    memory is allocated from the pool in the same way that it is allocated in
**    MEalloc.  This is done by starting with the entire initial chunk as a node
**    in a free list.  Some of this code was stolen from ME.c in the
**    compatibility library and adapted for this purpose. 
**
**	The externally visible routines defines in this file are:
**	---------------------------------------------------------
**	    ulm_chunk_init - Initialize ULM server-wide chunk size default.
**	    ulm_startup - Get a pool of memory from SCF and initialize it.
**	    ulm_shutdown - Destroy a memory pool and return the memory to SCF.
**	    ulm_openstream - Create and open a memory stream.
**	    ulm_closestream - Close and destroy a memory stream returning its
**				memory to the pool.
**	    ulm_reinitstream - (*** NOT YET IMPLEMENTED ***) Reinitialize a
**				memory stream, leaving its memory allocated
**				to the stream, but resetting the "next piece"
**				pointer to the beginning of the stream.
**	    ulm_palloc - Allocate a memory piece from a memory stream.
**          ulm_mark - Mark a memory stream for reclaiming later.
**          ulm_reclaim - Reclaim a memory stream up to the supplied mark.
**	    ulm_mappool - Produce a memory map of the pool.
**          ulm_xcleanup - Does cleanup in a memory pool after an exception.
**	
**	The following routines are internal to ULM and should not be called
**	directly; in fact, they are declared static so as to prevent this:
**	-------------------------------------------------------------------
**          ulm_pinit - Initialize a memory pool.
**          ulm_nalloc - Allocate memory node from the pool.
**          ulm_free - Free memory node that was allocated from the pool.
**	    ulm_fadd - Used to add a node to the free list.
**	    ulm_challoc - Allocate another chunk, create node, add to free list.
**      called directly; in fact, they are declared static so as to prevent
**      this.  They differ from the above routines in that they (and calls to
**      them) are only compiled into the code if `xDEV_TEST' is #define'd.
**      The person compiling with `xDEV_TEST' should be aware that this will
**      cause a serious performance hit on the system; something on the order
**      of 25%.  Therefore, Therefore, it is recommended that `xDEV_TEST' not
**      normally be #define'd; only use it if there is a suspected problem:
**	---------------------------------------------------------------------
**	    ulm_conchk - Perform consistency checks on a given memory pool.
**
**
**	LINT NOTES: Since this code does quite a bit of pointer casting in order
**	==========  to hide the underlying data structures in memory pools from
**		    its clients, lint speaks up frequently with the following
**		    message:
**			"warning: unaligned pointer combination"
**		    ULM guarantees that EVERYTHING in the memory pool that
**		    gets a pointer assigned to it will be aligned on a
**		    sizeof(ALIGN_RESTRICT) byte boundry.  Therefore, ULM knows
**		    that the pointer casting that it does will not produce
**		    unaligned pointers.  Unfortunately, it seems impossible
**		    to convince lint of this.  Therefore, it is probably safe
**		    to ignore any lint warnings of this type.  (I hand checked
**		    each one when I placed this file into the ULF.OLB on
**		    April 21, 1986.)
**
**		    Another lint complaint that I can't seam to avoid is:
**			"warning: long assignment may lose accuracy"
**		    Most of these get reported for where I am mod'ing a
**                  i4 by a "small number" (e.g. "BIGNUM % 8") and
**                  assigning the result to a nat.  In fact, the result of such
**                  a mod operation will never be larger than the "small number"
**                  minus 1, so lint should not complain ... yet it does. This
**                  message also ocurrs when I am converting the requested size
**                  of a memory pool, in bytes (a i4), into the number of
**                  SCU_MPAGESIZE byte pages (a nat) to ask SCF for.  Ignore
**                  this one, also. 
**
**  History:    $Log-for RCS$
**      18-mar-86 (jeff)    
**          adapted from ME.c in the compatibility library
**	08-apr-86 (thurston)
**	    Made bug fixes to ulm_nalloc() and ulm_free().  (See history notes
**	    under those routines for details.)  Also, moved the typedefs for
**	    ULM_NODE, ULM_HEAD, and ULM_PHEAD into the ULMINT.H file.
**	17-apr-86 (thurston)
**	    Changed the name of ulm_alloc() (and all references to it,
**	    comments included) to ulm_nalloc(), so as to avoid posible
**	    conflict with ulm_palloc().  (i.e. If someone left the "p"
**	    out of ulm_palloc(), accidently.)
**	21-apr-86 (thurston)
**	    Added "LINT NOTES:" comments above for this file.
**	23-apr-86 (thurston)
**	    Changed the declarations of the ULM internal routines to be static
**	    so as to prevent outsiders from calling them.
**	20-jun-86 (daved)
**	    removed redundant assignment in ulm_startup
**	01-aug-86 (thurston)
**	    Upgraded to use SCU_MPAGESIZE instead of hard-wired 512.
**	01-jun-87 (thurston)
**	    In ulm_fadd(), if the block being added to the free list is adjacent
**	    to the last block in the free list it gets coalesced ... however,
**	    the last block pointer was not being adjusted.  This is now fixed.
**	03-jun-87 (thurston)
**	    Added the static function ulm_conchk(), calls to which gets
**	    conditionally compiled in (if xDEV_TEST is defined) at the beginning
**	    and end of ulm_openstream(), ulm_closestream(), and ulm_palloc().
**	    Note that compiling this in will cause a rather nasty performance
**	    hit.  However, the checks that are done should catch a lot of memory
**	    stomping early on.
**	21-jun-87 (thurston)
**	    Changed all uses of E_UL0003_BADPARM to be E_UL0014_BAD_ULM_PARM
**	    to avoid confusion with the ule_format() error code.
**	16-oct-87 (thurston)
**	    Added ulm_spalloc().  This change also dictated adding the `setsem'
**	    argument to the statis ulm_nalloc() routine, which of course meant
**	    that all calls to that routine had to have an extra argument added.
**	    This was all done to allow RDF to use ulm_spalloc() instead of
**	    ulm_palloc() when it may have multiple sessions updating the same
**	    memory stream.
**	29-oct-87 (thurston)
**	    Changes necessary to allow ULM to allocate memory from SCF for a
**	    pool in chunks instead of all at once.
**	02-nov-87 (thurston)
**	    Added `pnode' argument to ulm_fadd().
**	04-nov-87 (thurston)
**	    Added code to ulm_startup() that calculates sets the `preferred
**	    chunk size' with consideration for the supplied `preferred block
**	    size'.  Along with this, I added the pref_chunksize_pag argument
**	    to ulm_pinit().
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	12-jan-88 (thurston)
**          Added code to ulm_conchk() to check the backward pointer linkage in
**          both the free and allocated lists.  Also added code to check the
**          address ordering of the free list (that is, the "current" node in
**          the free list must always be at a higher address than the "previous"
**          node, it does not exist "inside" the previous node, and it is not
**          adjacent to the previous node). 
**	12-jan-88 (thurston)
**          Modified code in ulm_pinit(), ulm_nalloc(), ulm_free(), ulm_fadd(),
**          ulm_challoc(), and ulm_conchk() to use and maintain the backwards
**          pointers for the free list. 
**	12-jan-88 (thurston)
**	    Modified ulm_closestream() and ulm_reclaim() so that they now free
**	    up the stream's blocks begining with the last block in the
**	    stream.  This is done to fit in with the fact that when a free node
**	    is split to create an allocated node, we take the allocated piece
**	    from the end of the original free node.  We should get better
**	    performance by freeing the blocks (one node per block) backwards.
**	13-jan-88 (thurston)
**	    Modified code in ulm_openstream() and ulm_closestream() so that the
**	    first data block of a memory stream is always allocated contiguous
**	    to the stream header (read: out of the same node).
**	13-jan-88 (thurston)
**          Moved the level of semaphore locking up to the externally visible
**          routines.  This was done since, upon performance testing of the DBMS
**          server, it was found that the number of semaphore collisions is a
**          minute fraction of the total number of semaphore calls.  Therefore,
**          we seem to be spending a LOT more time setting and releasing
**          semaphores than we do actually waiting for them.  In making these
**          changes I removed the `setsem' argument from ulm_nalloc(), and
**          completely got rid of the ulm_stadd() and ulm_stremove() static
**          routines. 
**	25-jan-88 (thurston)
**          Fixed small bug introduced by recent performance mods.  In
**          ulm_closestream(), the memleft counter was not getting "credited"
**          for the size of the block header for the 1st block in the stream.
**          Thus, every ulm_openstream() - ulm_closestream() pair was
**          effectively reducing the size of the client's memory pool by
**          ULM_SZ_BLOCK bytes (20 bytes). 
**	11-feb-88 (thurston)
**	    Added extra arg to both ulm_nalloc() and ulm_challoc() for the
**	    err_data, and altered all routines to set it if NOMEM is detected.
**	    This could help during debugging.
**	12-feb-88 (thurston)
**	    Added the `failed_check' argument to ulm_conchk() so the caller can
**	    tell upon receiving the CORRUPT_POOL error why it was currupted.
**	    Also added the ulm_xcleanup() routine to be called by the ULM client
**	    if the client incurred an exception that may have happened during a
**	    ULM call.  This is required since that exception may have come when
**	    the session was holding the ULM pool semaphore.
**	15-mar-88 (thurston)
**	    Added the ulm_mappool() routine.
**      13-aug-91 (jrb)
**          Initialize pool semaphore using CSi_semaphore instead of MEfilling
**          with zeros.
**      01-sep-1992 (rog)
**          Prototype ULF.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	29-jun-1993 (rog)
**	    Include <tr.h> for the xDEBUGed TRdisplay()s.
**	27-aug-1993 (rog)
**	    Added ulm_chunk_init() to allow the chunk size to be configured at
**	    server startup.  Moved chunk macro definitions here from ulmint.h.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in ulm_pinit() and
**	    ulm_startup().
**	03-dec-93 (swm)
**	    Bug #58635
**	    In ulm_pinit() and ulm_openstream() cast powner and owner values
**	    to PTR in assignments to ulm_owner in the ULM_CHUNK type.
**	    Although the values are not pointers in these cases, ULM_CHUNK must
**	    declare ulm_owner to be PTR for consistency with the DM_SVCB
**	    standard DMF header.
**	04-mar-94 (swm)
**	    Bug #56449
**	    Corrected TRdisplay of pointers to use %p rather than %8x.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	02-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	05-Jun-1997 (shero03)
**	    Protect any changes to stream within the semaphored section
**	06-Aug-1997 (jenjo02)
**	    Added sanity checks of caller-supplied stream and pool
**	    pointers.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	    ulm_streamid_p, if present, points to the stream handle. If omitted,
**	    ulm_streamid will house the streamid.
**	15-Aug-1997 (jenjo02)
**	  o Introduced the notion of private and shareable streams. Private streams
**	    are assumed to be thread-safe while shareable streams are not. It
**	    is the stream creator's responsibility to define this characteristic
**	    when the stream is created by ulm_openstream(). ULM_SHARED_STREAM streams
**	    are mutex-protected; ULM_PRIVATE_STREAM streams are not.
**	    Stream marks and stream reclamation are permitted only in PRIVATE streams
**	    because of concurrency considerations.
**	    This change obviates the need for ulm_spalloc(), which has been removed.
**	  o Also added a piggyback function to ulm_openstream() by which one may
**	    allocate a piece of the stream during the same call by specifying
**	    the ULM_OPEN_AND_PALLOC flag in the ulm_rcb.
**	  o Modified inkdo01's 8-nov-95 change to ulm_palloc(). Instead of always 
**	    searching all blocks (even those which are full), keep track of the first 
**	    block with available space in a new stream field ulm_sblspa and begin
**	    the search from there.
** 	13-Apr-1998 (wonst02)
** 	    Added the URS facility name to the owner_names array.
**	26-may-1998 (canor01)
**	    Add newline to end of file to avoid compile error on Solaris.
**      27-oct-98 (stial01)
**          Exposed ulm_conchk as a tracepoint
**	14-Dec-2000 (jenjo02)
**	    ulm_challoc():
**	    If there are at least "needed" pages remaining in pool yet
**	    not enough to satisfy "preferred" number of pages, get
**	    all remaining pages rather than returning E_UL0005_NOMEM.
**      06-feb-2002 (stial01)
**          Add ULM memory padding, and filling, specified with dbms server
**          configuration parameter ulm_pad_bytes
**      28-feb-2003 (huazh01,stial01)
**          Added ulm_printpool, to be called at server shutdown
**          Time stamp ulm memory, to help find memory leaks
**      02-may-2003 (stial01)
**          TRdisplay ulm diagnostics only if Ulm_pad_bytes
**      06-may-2003 (stial01)
**          TRdisplay ulm diagnostics only if Ulm_pad_bytes, output 64 bytes
**      08-may-2003 (stial01)
**          ifdef some more ulm diagnostics
**      09-jul-2003 (stial01)
**          Changes to print the query when there is a ULM pad error.
**	31-Dec-2003 (schka24)
**	    TRdisplays are useful, do even if not padding
**	7-oct-2004 (thaju02)
**	    Define pool size as SIZE_TYPE.
**	6-Jan-2005 (schka24)
**	    Handle ulm_pad_bytes == 1 as turning on lfill but not padding.
**	    (Previously this would also set padding to 16, which hid a
**	    nasty little segv that I put into partitioning a year ago.)
**	31-May-2005 (schka24)
**	    Some fixes to xDEBUG debug checks and displays.
**      10-Jul-2006 (hanal04) Bug 116358
**          Removed requirement for Ulm_pad_bytes to be set in
**          ulm_print_pool().
**	9-feb-2007 (kibro01) b113972
**	    Add ulm_stream_marker to detect a twice-closed stream without
**	    crashing.
**	11-feb-2008 (smeke01) b113972
**	    Back out above change.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use SETDBERR, CLRDBERR to value DB_ERROR structures, clean up
**	    static functions to always pass ULM_RCB*.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes; use %ld for debug display sizes.
**       9-Nov-2009 (hanal04) Bug 122860
**          Extend pad overhead to include a second copy of the orig_len.
**          If the tail of the pad is corrupted we'll try to report pad errors
**          for the wrong memory locations and hit spurious exceptions.
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing prototypes
**/


/*
**  Forward and/or External function references.
*/

static DB_STATUS ulm_pinit( SIZE_TYPE  poolsize_pag, SIZE_TYPE  pref_chunksize_pag,
			    SIZE_TYPE  chunksize_pag, i4 pblksize,
			    PTR chunk, PTR *pool, ULM_RCB *ulm_rcb );
static DB_STATUS ulm_nalloc( PTR pool, i4 size, PTR *block, ULM_RCB *ulm_rcb );
static DB_STATUS ulm_free( PTR block, PTR pool, ULM_RCB *ulm_rcb );
static DB_STATUS ulm_fadd( ULM_NODE *node, i4  owner, ULM_HEAD *freehead,
			   ULM_RCB *ulm_rcb );
static DB_STATUS ulm_challoc( ULM_PHEAD *pool, i4 nodesize,
			      ULM_NODE **node, ULM_RCB *ulm_rcb );
static VOID ulm_pad_error(ULM_NODE *node, char *padchar);

static void ulm_printpool(PTR pool_ptr, char *facility);

static void ulm_countpool(ULM_PHEAD *pool_ptr,
			i4 *poolsum, i4 *poolnum,
			i4 *free_blks, i4 *free_tot);


/* Static definitions */
#define  ULM_CMB_CHUNK_MIN_BYTES    131072	/* 128 Kb */

/* Default ULM chunk size in bytes. */
static i4  Ulm_chunk_size = ULM_CMB_CHUNK_MIN_BYTES;

#define ULM_PAD_OVERHEAD ((sizeof(i4) * 2) + sizeof(SYSTIME))

#define ULM_PAD_BYTES_MIN  (ULM_PAD_OVERHEAD + 4)
static i4  Ulm_pad_bytes = 0; /* If set, MUST BE >= ULM_PAD_BYTES_MIN */

static bool Ulm_lfill = FALSE; /* Fill memory with 'FE' after freeing it */

static STATUS ex_handler(
	EX_ARGS	*ex_args);

/* CSp_semaphore modes: */
#define		SEM_SHARE	0
#define		SEM_EXCL	1

static char	    *owner_names[DB_MAX_FACILITY+1] =
	{
	    "<unknown>",
	    "CLF",
	    "ADF",
	    "DMF",
	    "OPF",
	    "PSF",
	    "QEF",
	    "QSF",
	    "RDF",
	    "SCF",
	    "ULF",
	    "DUF",
	    "GCF",
	    "RQF",
	    "TPF",
	    "GWF",
	    "SXF",
	    "URS"
	};

static DB_STATUS  ulm_conchk( ULM_PHEAD *pool, ULM_RCB *ulm_rcb );

static char	    *conchk_names[] =
	{
	    "(ALL OK)",
	    "(FIRST CHUNK)",
	    "(CHUNK LIST)",
	    "(LAST CHUNK)",
	    "(CHUNK COUNT)",
	    "(STREAM LIST)",
	    "(LAST STREAM)",
	    "(ALLOCATED NODE LIST)",
	    "(LAST ALLOCATED NODE)",
	    "(FREE NODE LIST)",
	    "(LAST FREE NODE)",
	    "(TOTAL NODE SIZE)"
	};


/*
** Name: ulm_countpool
**
** Description:
** 	Simple routine to gather pool stats for debug output
**	This routine should only be called when pool_ptr->ulm_psem is held.
**
*/
static void
ulm_countpool(ULM_PHEAD *pool_ptr, i4 *poolsum, i4 *poolnum, i4 *free_blks, i4 *free_tot)
{
    ULM_PHEAD   *pool = (ULM_PHEAD *)pool_ptr; 	/* current memory pool */
    ULM_NODE    *current = pool->ulm_pfrlist.ulm_hlast; /* free list */
    i4 len=0;
    i4 size=0;
    i4 free_cnt =0;
  
    if (pool_ptr == NULL) 
        return;


    for (current = pool->ulm_pfrlist.ulm_hlast; 
	    current != (ULM_NODE *)NULL;
		current = current->ulm_nprev)
    {
	free_cnt++;
	size += current->ulm_nsize;
    }
    *free_blks=free_cnt;
    *free_tot=size;


    size=0;
    if (free_cnt > pool->ulm_pchunks)
    {
	for (current = pool->ulm_pallist.ulm_hlast;
		current != (ULM_NODE *)NULL;
		    current = current->ulm_nprev)
	{
	    len++;
	    size += current->ulm_nsize;
	}
    }
    *poolnum = len;
    *poolsum = size;
}

/*{
** Name: ulm_chunk_init	- Initialize ULM server-wide default chunk size.
**
** Description:
**      This function is called at server startup time to initialize the 
**	default ULM server-wide chunk size.  This function must be called
**	before any of the other ULM functions.
**
**	If the chunk size passed in is less than the minimum, we set the
**	size to the minimum.  If it is too big, then the pool startup
**	functions will prevent it from trying to use a size bigger than
**	the actual pool size.
**
** Inputs:
**      chunk_size			Value to set the chunk size to.
**
** Outputs:
**	None
**
** Returns:
**	OK				Chunk size set.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	27-aug-1993 (rog)
**	    Created.
**	12-may-1994 (rog)
**	    Named correctly: should be called ulm_chunk_init().
*/

DB_STATUS
ulm_chunk_init( i4  chunk_size )
{
    if (chunk_size < ULM_CMB_CHUNK_MIN_BYTES)
	chunk_size = ULM_CMB_CHUNK_MIN_BYTES;

    Ulm_chunk_size = chunk_size;

    return(OK);
}

DB_STATUS
ulm_pad_bytes_init( i4  pad_bytes)
{
    i4 mod;

    if (pad_bytes <= 0)
    {
	TRdisplay("ULM Ignoring illegal ulm_pad_bytes %d\n", pad_bytes);
	return (OK);
    }

    /* Temporary way to turn on Ulm_lfill, if pad_bytes is odd */
    if ((pad_bytes & 1) != 0)
    {
	TRdisplay("Turning ULM lfill ON\n");
	Ulm_lfill = TRUE;
	pad_bytes &= ~1;
	/* Allow pad-bytes of 1 to mean lfill, but no padding */
	if (pad_bytes == 0)
	    return (OK);
    }

    /* Round up to nearest DB_ALIGN_SZ */
    if (mod = pad_bytes % DB_ALIGN_SZ) 
	pad_bytes += DB_ALIGN_SZ - mod;

    /* recommended pad is 64 bytes, but it must be at least 8 bytes */
    if (pad_bytes < ULM_PAD_BYTES_MIN)
	Ulm_pad_bytes = ULM_PAD_BYTES_MIN;
    else
	Ulm_pad_bytes = pad_bytes;

    /*
    ** Note this routine gets called from back!scf!scd scdopt.c
    ** when the processing the dbms config parameter ulm_pad_bytes
    */
    TRdisplay("Setting ULM pad bytes = %d\n", Ulm_pad_bytes);

    return(OK);
}


/*{
** Name: ulm_startup	- Get a pool of memory from SCF and initialize it.
**
** Description:
**      This procedure is used to allocate a memory pool on behalf of the client
**	routine.  This memory is allocated from SCF in large, contiguous
**	"chunks" on an "as needed" basis.  (That is, all of the memory for pool
**	may not be allocated initially, but the total size of the memory pool
**	will never exceed the requested size supplied to this routine.
**      Typically, this call will be used at server startup time to allocate 
**      large pools of memory to be used throughout the lifetime of the server. 
**      This routine will return a "pool id" which will be a pointer, but
**      the user of ULM should only consider this as an abstraction used to
**      identify the pool for later operations.  (i.e. The user should not use
**      this pointer to reference memory directly.)
**
**      The ULM routines will be entirely reentrant.  (i.e. No memory management
**      structures will be contained outside the pool.)
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Memory must be bound to a particular
**                                      facility ... this is the facility code
**                                      used to identify the facility.  (Use
**					constants found in DBMS.H.)
**          .ulm_sizepool               Number of bytes requested for pool ...
**                                      this may be rounded up slightly to be an
**                                      even multiple of SCU_MPAGESIZE bytes.
**          .ulm_blocksize              This will set the "preferred block size"
**                                      for the memory pool; which is to say,
**                                      that ULM will create data blocks of this
**                                      size from the pool unless overridden for
**                                      a particular stream at ulm_openstream()
**                                      time, or if a piece request requires a
**                                      larger block.  If 0 then ULM will choose
**                                      the preferred block size for the pool.
**
** Outputs:
**      ulm_rcb
**          .ulm_poolid                 This is an id which the user of ULM must
**                                      use to identify the pool in order to
**                                      open memory streams ... the user of ULM
**                                      will not be allowed to see the structure
**                                      of what this pointer points to ... since
**                                      it should only be used as a "pool id".
**          .ulm_error			Standard DB_ERROR structure.
**		.err_code		If the return status is not E_DB_OK ...
**		    E_UL0014_BAD_ULM_PARM   Bad parameter to this function.
**		    E_UL0005_NOMEM	    Couldn't get the memory from SCF.
**
**	Returns:
**          E_DB_OK, E_DB_WARNING, E_DB_ERROR, or E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory from SCF.
**
** History:
**	26-mar-86 (seputis)
**          initial creation
**	17-apr-86 (thurston)
**	    Finished coding.
**	01-aug-86 (thurston)
**	    Upgraded to use SCU_MPAGESIZE instead of hard-wired 512.
**	29-oct-87 (thurston)
**	    Modified to allow memory for a pool to be allocated from SCF in
**	    large, contiguous "chunks" as opposed to all at startup.
**	04-nov-87 (thurston)
**	    Added code that calculates sets the `preferred chunk size' with
**	    consideration for the supplied `preferred block size'.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-rog-1993 (rog)
**	    Simplified the calculation for the default chunk size.
**	14-Aug-1997 (jenjo02)
**	    Initialize unused portions of caller's ulm_rcb so that residual
**	    values don't inadvertantly get picked if the same ulm_rcb is reused.
**	13-Sep-2006 (wanfr01)
**	    Bug 116632 - added ULM diagnostics for E_UL0005
*/

DB_STATUS
ulm_startup( register ULM_RCB *ulm_rcb )
{
    i4		bsize;
    SIZE_TYPE		tot_psize_pag;	/* Total pool requested size */
    SIZE_TYPE		pbl_psize_pag;	/* Chunk size req. by pref. blk size */
    SIZE_TYPE		def_psize_pag;	/* Default chunk size (in pages) */
    SIZE_TYPE		pch_psize_pag;	/* Preferred chunk size */
    SIZE_TYPE		act_psize_pag;	/* Actual size of 1st chunk from SCF */
    PTR			chunk_ptr;
    PTR			pool_ptr;
    DB_STATUS		status;
    i4		err_code;
    SCF_CB              scf_cb;
    char		*facility;


    if ((bsize = ulm_rcb->ulm_blocksize) == 0)
	bsize = ULM_BLSIZE_DEFAULT;

    /*
    ** Set default values in caller's ulm_rcb.
    ** Set the default stream type to SHARED.
    */
    ulm_rcb->ulm_streamid_p = (PTR*)NULL;
    ulm_rcb->ulm_streamid = (PTR)NULL;
    ulm_rcb->ulm_pptr = (PTR)NULL;
    ulm_rcb->ulm_psize = 0;
    ulm_rcb->ulm_flags = ULM_SHARED_STREAM;
    ulm_rcb->ulm_smark = (ULM_SMARK*)NULL;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Check for bad parameter */
    if ((ulm_rcb->ulm_sizepool <= 0) || (bsize < 0))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
	return (E_DB_ERROR);
    }

    /* Prepare to get memory from SCF. */

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = ulm_rcb->ulm_facility;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_scm.scm_functions = 0;

    /*
    ** Calculate preferred CHUNK size, in SCU_MPAGESIZE byte pages.
    */
    tot_psize_pag = (ulm_rcb->ulm_sizepool - 1)/SCU_MPAGESIZE + 1;
    pbl_psize_pag = ((bsize + ULM_SZ_CHUNK + ULM_SZ_PHEAD + 2*ULM_SZ_NODE +
			ULM_SZ_STREAMS + ULM_SZ_BLOCK) - 1)/SCU_MPAGESIZE + 1;
    /*
    ** def_psize_pag is the default number of SCU_MPAGESIZE byte pages that 
    ** will be used per each chunk, unless the preferred block size for
    ** the pool would require something larger, or unless a piece request
    ** would require more, or unless the memory pool does not have that
    ** many left, or SCF can only give a smaller amount.
    */
    def_psize_pag = ((Ulm_chunk_size - 1)/SCU_MPAGESIZE + 1);

    pch_psize_pag = min(tot_psize_pag, max(def_psize_pag, pbl_psize_pag));

    /*
    ** If preferred chunk size > 1/2 of total pool request, change preferred
    ** to equal total pool request and allocate entire pool all at once.
    */
    if (pch_psize_pag*2 > tot_psize_pag)
	pch_psize_pag = tot_psize_pag;

    /* Now ask SCF for the memory. */
    scf_cb.scf_scm.scm_in_pages = pch_psize_pag;
    if (scf_call(SCU_MALLOC, &scf_cb) != E_DB_OK)
    {
	TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
			__FILE__, __LINE__, ulm_rcb->ulm_poolid,
			ulm_rcb->ulm_facility);

	SETDBERR(&ulm_rcb->ulm_error, 1, E_UL0005_NOMEM);
	return (E_DB_ERROR);
    }
    act_psize_pag = scf_cb.scf_scm.scm_out_pages;
    chunk_ptr = (PTR) scf_cb.scf_scm.scm_addr;

    /* Did we get enough? */
    /* ------------------ */
    if (act_psize_pag < pbl_psize_pag)
    {
	TRdisplay("DIAG E_UL0005_NOMEM: %s:%d act_psize_pag = %d, pbl_psize_pag = %d\n",
			__FILE__, __LINE__, act_psize_pag, pbl_psize_pag);
	TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
			__FILE__, __LINE__, ulm_rcb->ulm_poolid,
			ulm_rcb->ulm_facility);

	/*
	** No, we did not.  Before returning with
	** an error, release memory chunk to SCF.
	*/
	_VOID_ scf_call(SCU_MFREE, &scf_cb);

	ulm_rcb->ulm_poolid = (PTR) NULL;
	SETDBERR(&ulm_rcb->ulm_error, 2, E_UL0005_NOMEM);
	return (E_DB_ERROR);
    }

    /* We have memory pool allocated from SCF, now initialize it. */
    status = ulm_pinit(tot_psize_pag, pch_psize_pag, act_psize_pag,
					bsize, chunk_ptr, &pool_ptr, ulm_rcb);
    if (DB_FAILURE_MACRO(status))
    {
	/* Before returning with error, release memory chunk to SCF. */
	_VOID_ scf_call(SCU_MFREE, &scf_cb);

	return (status);
    }
    else    /* Prepare to return normally. */
    {

	/* The poolid TRdisplay is particularly useful, always output */
	if (ulm_rcb->ulm_facility != DB_ULF_ID)
	{
	    if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
		ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
		facility = owner_names[ulm_rcb->ulm_facility];
	    else
		facility = "unknown";
	    TRdisplay("%@ ULM %s Started up pool at 0x%p\n",
		    facility, pool_ptr);
	    TRdisplay("%@ ULM %s Requested pool size:%6d pages\n", 
		    facility, tot_psize_pag);
	    TRdisplay("%@ ULM %s Preferred chunk size:%6d pages\n", 
		    facility, pch_psize_pag);
	    TRdisplay("%@ ULM %s Allocate chunk 1 %d pages at 0x%p\n",
		    facility, act_psize_pag, chunk_ptr);
	}

	ulm_rcb->ulm_poolid = pool_ptr;
	return (E_DB_OK);
    }
}


/*{
** Name: ulm_shutdown	- Destroy a memory pool and return the memory to SCF.
**
** Description:
**      This routine will return the memory allocated to the pool by
**	ulm_startup() to the SCF.  All streams within this pool will
**	therefore be inaccessible. 
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .ulm_poolid                 Pool id to identify the memory pool
**                                      to be returned to SCF.
**
** Outputs:
**      ulm_rcb
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    All streams associated with the pool will be inaccessible, as
**	    the pool's memory will be returned to SCF.
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	17-apr-86 (thurston)
**	    Finished coding.
**	29-oct-87 (thurston)
**	    Modified to allow memory for a pool to be allocated from SCF in
**	    large, contiguous "chunks" as opposed to all at startup.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	15-Aug-1997 (jenjo02)
**	    Remove shareable stream semaphores.
*/

DB_STATUS
ulm_shutdown( register ULM_RCB *ulm_rcb )
{
    		ULM_PHEAD	*pool_ptr;
    register	ULM_CHUNK	*chunk, *prev_chunk;
    register 	ULM_STREAM	*stream;
    		i4		chunk_size_pag;
    		DB_STATUS	status = E_DB_OK;
    		SCF_CB          scf_cb;
		i4		pchunks;
		EX_CONTEXT      context;
		bool		trace;
		char		*facility;

    /* Validate pool and owner */
    if ((pool_ptr = (ULM_PHEAD*)ulm_rcb->ulm_poolid) == (ULM_PHEAD*)NULL ||
	pool_ptr->ulm_self != pool_ptr ||
	pool_ptr->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    pchunks = pool_ptr->ulm_pchunks;

    /*
    ** Trace ulm shutdown in server shutdown
    ** Don't trace shutdown of ULF because server does that a lot
    ** shutdown from other facilities probably indicates server shutdown
    ** (Alternative would be to change all ulm_shutdown callers to 
    *  pass a flag in ULM_RCB indicating if it was server shutdown)
    */
    if (ulm_rcb->ulm_facility != DB_ULF_ID)
	trace = TRUE;
    else
	trace = FALSE;
	
    if (trace)
    {
	if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
	    ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
	    facility = owner_names[ulm_rcb->ulm_facility];
	else
	    facility = "unknown";

	TRdisplay("%@ ULM %s Shutting down pool at 0x%p\n", facility, pool_ptr);
	TRdisplay("%@ ULM %s Requested pool size:%6d pages\n",
		    facility, pool_ptr->ulm_psize_pag);
	TRdisplay("%@ ULM %s Allocated size:%6d pages in %d chunks\n",
		    facility, pool_ptr->ulm_pages_alloced, pchunks);
	/*
	** Turn off detailed trace of QSF pool since it is valid for QEF to
	** have unfreed memory at server shutdown
	*/
	if (ulm_rcb->ulm_facility == DB_QSF_ID)
	    trace = FALSE;
    }

    /* Remove shareable stream semaphores from remaining streams */
    for (stream = pool_ptr->ulm_pst1st;
	 stream != (ULM_STREAM*)NULL;
	 stream = stream->ulm_sstnext)
    {
	if (stream->ulm_flags & ULM_SHARED_STREAM)
	    CSr_semaphore(&stream->ulm_ssem);
    }

    pool_ptr->ulm_self = (ULM_PHEAD*)NULL;

    /* Stop leak! */
    CSr_semaphore( &pool_ptr->ulm_psem );

    /* Prepare to release memory pool to SCF. */

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = ulm_rcb->ulm_facility;
    scf_cb.scf_session = DB_NOSESSION;

    if (trace)
    {
	if (EXdeclare(ex_handler, &context) == OK)
	{
	    ulm_printpool((PTR)pool_ptr, facility);
	}
	EXdelete();
    }

    /*
    ** Now loop on the memory pool chunks, last to first, since the pool
    ** header is in the first chunk, and we want to deallocate this last
    */
    chunk = pool_ptr->ulm_pchlast;
    while (chunk != (ULM_CHUNK *) NULL)
    {
	prev_chunk = chunk->ulm_cchprev;
	chunk_size_pag = chunk->ulm_cpages;
	scf_cb.scf_scm.scm_in_pages = chunk_size_pag;
	scf_cb.scf_scm.scm_addr = (char *) chunk;

	/* Now ask SCF to free the memory. */
	if (scf_call(SCU_MFREE, &scf_cb) != E_DB_OK)
	{
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	    status = E_DB_ERROR;
	    break;
	}

#ifdef    xDEV_TEST
	TRdisplay("\n    Chunk %d released:%6d pages at 0x%p",
		    pchunks--, chunk_size_pag, chunk);
#endif /* xDEV_TEST */

	chunk = prev_chunk;

    }

    if (DB_SUCCESS_MACRO(status))
    {
	/* Prepare to return normally. */
	ulm_rcb->ulm_poolid = (PTR) NULL;
    }

#ifdef    xDEV_TEST
    if (pchunks)
    {
	TRdisplay("    *** Problem detected in ulm_shutdown() ...\n");
	TRdisplay("        ... released more/less chunks than should have.");
    }
    TRdisplay(" >>>\n");
#endif /* xDEV_TEST */

    return (status);
}


/*{
** Name: ulm_openstream	- Create and open a memory stream.
**
** Description:
**      This routine will create a stream of memory within the specified pool.
**      A stream of memory should be created for objects which have the same
**      "lifetime" characteristics.  Note, that the only way to deallocate
**      memory in ULM is to close (i.e. "delete") the stream; it is an all or
**      nothing deallocation.  A memory stream will not be contiguous
**      and will allocate memory from the pool which may be fragmented.
**      However, any one particular alloc-piece (gotten via ulm_palloc())
**	will be contiguous, of course.
**
**      When the internal structures are being updated, the critical areas of
**      code will be protected by semaphores; that is, multiple sessions can
**      create streams from the same pool.
**
**      Interrupts are not disabled, so that if the caller of the memory
**      management routines has interrupt handlers which signal exceptions
**      which clears the "call stack" then calls to these memory management
**      routines must be protected by disabling interrupts.
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .ulm_poolid                 Pool id (returned at ulm_startup()
**                                      time).  Used to identify the pool from
**                                      which the stream will be created.
**	    .ulm_blocksize		If non-zero, this "preferred block size"
**					will override the one set for the entire
**					pool, for data blocks allocated for this
**					stream.  Also note, that if a request to
**					allocate a piece larger than the block-
**					size, a block of an appropriate size
**					will be created and linked into the
**					stream.
**          .ulm_memleft                This will be a pointer to a "memory
**                                      counter."  This counter will be used as
**                                      a check to assure that the amount of
**                                      memory used for this operation will not
**                                      exceed "some" limit. This limit can be
**                                      imposed by the caller as needed.  (e.g.
**                                      Limit for a stream, limit for a set of
**                                      streams, limit for a session, limit for
**                                      the server, or whatever.) 
**	    .*ulm_streamid_p		Optional pointer to an address into which
**					the streamid is to be returned. If omitted,
**					the streamid will be placed in ulm_streamid.
**	    .ulm_flags			Either ULM_PRIVATE_STREAM or ULM_SHARED_STREAM.
**					Additionally, if ULM_OPEN_AND_PALLOC is
**					set, additional bytes will be allocated
**					and that piece returned to the caller.
**	    .ulm_psize			If ULM_OPEN_AND_PALLOC, the size of the
**					piece to allocate from the newly formed
**					stream.
**					
**
** Outputs:
**      ulm_rcb
**          .ulm_memleft                This routine will decrement the counter
**                                      pointed to by .ulm_memleft, since some
**                                      memory is required to store the stream
**                                      descriptor information.
**          .*ulm_streamid_p            "Stream id" used to identify the
**                                      stream when a piece of memory is
**					to be allocated via ulm_palloc().
**	    .ulm_pptr			If ULM_OPEN_AND_PALLOC, pointer to the
**					allocated memory piece.
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0014_BAD_ULM_PARM   Bad parameter to this function.
**		    E_UL0004_CORRUPT	    Pool is corrupted.
**		    E_UL0005_NOMEM	    Not enough memory.
**		    E_UL0009_MEM_SEMWAIT    Error waiting for exclusive access.
**		    E_UL000A_MEM_SEMRELEASE Error releasing exclusive access.
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, or E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will allocate the first block of the stream.  Thus,
**          allocating streams costs a small amount of memory, which is
**          why .ulm_memleft is passed into this routine.  To do this, it may
**	    require allocating another "chunk" of the memory pool from SCF.
**
** History:
**	26-mar-86 (seputis)
**          initial creation
**	17-apr-86 (thurston)
**	    Finished coding.
**	13-jan-88 (thurston)
**	    Now allocates the stream's first data block in the same node as the
**	    stream header itself.
**	13-jan-88 (thurston)
**	    Added semaphore protection around ulm_nalloc() and the former
**	    ulm_stadd() call since it has been removed from ulm_nalloc() and the
**	    code from ulm_stadd() was moved directly into this routine.  This
**	    avoids multiple semaphore calls, thus having a positive effect on
**	    performance.
**	06-Aug-1997 (jenjo02)
**	    Sanity check pool.
**	15-Aug-1997 (jenjo02)
**	    Introduced the notion of private and shareable streams. Private streams
**	    are assumed to be thread-safe while shareable streams are not. It
**	    is the stream creator's responsibility to define this characteristic
**	    when the stream is created by ulm_openstream(). ULM_SHARED_STREAM streams
**	    are mutex-protected; ULM_PRIVATE_STREAM streams are not.
**	13-Sep-2006 (wanfr01)
**	    Bug 116630
**	    Must mutex ulm_memleft to avoid corruption and E_UL0005 errors.
**	13-Sep-2006 (wanfr01)
**	    Bug 116632 - added ULM diagnostics for E_UL0005
**      12-jul-2010 (huazh01)
**          Cast output to i8 if using %ld in TRdisplay(). (b124066)
*/

DB_STATUS
ulm_openstream( register ULM_RCB *ulm_rcb )
{
    i4			psize = (ulm_rcb->ulm_flags & ULM_OPEN_AND_PALLOC)
				? ulm_rcb->ulm_psize
				: 0;
    register ULM_PHEAD	*pool;
    i4		sblsize;
    i4		req_size;
    register ULM_STREAM	*stream;
    PTR			*streamid_p = (ulm_rcb->ulm_streamid_p != (PTR*)NULL)
				     ? ulm_rcb->ulm_streamid_p
				     : &ulm_rcb->ulm_streamid;
    register ULM_BLOCK	*block;
    DB_STATUS		status;
    i4			mod;
    char		sem_name[ CS_SEM_NAME_LEN ];
    char		*facility;

    /* Nullify caller's streamid in case we fail */
    *streamid_p = (PTR)NULL;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Check for bad parameter */
    /* ----------------------- */
    if (ulm_rcb->ulm_blocksize < 0 ||
	(pool = (ULM_PHEAD*)ulm_rcb->ulm_poolid) == (ULM_PHEAD*)NULL ||
	ulm_rcb->ulm_memleft == (SIZE_TYPE*)NULL ||
	(ulm_rcb->ulm_flags & (ULM_PRIVATE_STREAM | ULM_SHARED_STREAM)) == 0 ||
	(ulm_rcb->ulm_flags & ULM_OPEN_AND_PALLOC && psize <= 0) )
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
	return (E_DB_ERROR);
    }

    /* Sanity check pool and owner */
    /* ----------------------------- */
    if (pool->ulm_self != pool ||
	 pool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }
    
#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_openstream    pool = 0x%p  owner = %s\n",
		pool, owner_names[ulm_rcb->ulm_facility]);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(pool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */

    /* Now check to see that memleft will not underflow */
    /* ------------------------------------------------ */
    sblsize = (ulm_rcb->ulm_blocksize) ? ulm_rcb->ulm_blocksize 
				       : pool->ulm_pblsize;
    sblsize = (sblsize > psize) ? sblsize 
				: psize;

    if (mod = sblsize % DB_ALIGN_SZ) 
	sblsize += DB_ALIGN_SZ - mod;

    /* Shared streams include a semaphore; private ones do not */
    req_size = (ulm_rcb->ulm_flags & ULM_SHARED_STREAM)
	     ?  sblsize + ULM_SZ_STREAMS + ULM_SZ_BLOCK
	     :  sblsize + ULM_SZ_STREAMP + ULM_SZ_BLOCK;

    if (req_size > *ulm_rcb->ulm_memleft)
    {
        TRdisplay ("E_UL0005 at point 3;  %d > (%p) %ld \n",req_size , ulm_rcb->ulm_memleft, (i8)*ulm_rcb->ulm_memleft);

        TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
       		__FILE__, __LINE__, ulm_rcb->ulm_poolid,
		   ulm_rcb->ulm_facility);
	SETDBERR(&ulm_rcb->ulm_error, 3, E_UL0005_NOMEM);
	status = E_DB_ERROR;
    }
    else
    {
	/* Now go and get exclusive access to the pool */
	if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
	{
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	    /* Return now, instead of attempting to do a v() */
	    return (E_DB_ERROR);
	}

	/* Now get one free node for the stream block and the 1st data block */
	/* ----------------------------------------------------------------- */
	status = ulm_nalloc((PTR) pool,
			    (i4) req_size,
			    streamid_p,
			    ulm_rcb);


	if ( status && ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
	{
	    TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
			    __FILE__, __LINE__, ulm_rcb->ulm_poolid,
			    ulm_rcb->ulm_facility);
	}

	if (DB_SUCCESS_MACRO(status))
	{
	    stream = (ULM_STREAM*)*streamid_p;

	    /* Now attach the stream to the list of streams for the pool */
	    /* --------------------------------------------------------- */
	    stream->ulm_sstnext = (ULM_STREAM *) NULL;
	    stream->ulm_sstprev = pool->ulm_pstlast;
	    pool->ulm_pstlast = stream;
	    if (pool->ulm_pst1st == (ULM_STREAM *) NULL)
	    {
		/* if stream is the 1st one in the pool ... */
		pool->ulm_pst1st = stream;
	    }
	    else
	    {
		/* else, adjust the former last stream's "next stream" pointer*/
	    	stream->ulm_sstprev->ulm_sstnext = stream;
	    }

	    *ulm_rcb->ulm_memleft -= req_size;

	    /* Release exclusive access to the pool */
	    CSv_semaphore(&pool->ulm_psem);


	    /* Initialize the stream, and set up the first data block */
	    /* ------------------------------------------------------ */
	    stream->ulm_self = stream;
	    stream->ulm_spool = pool;
	    stream->ulm_sblsize = sblsize;

	    /* Decide if this is a shareable or private stream */
	    stream->ulm_flags = ulm_rcb->ulm_flags;

	    /* Undo the piggyback function, forcing its explicit setting each time */
	    ulm_rcb->ulm_flags &= ~ULM_OPEN_AND_PALLOC;
	    ulm_rcb->ulm_psize = 0;

	    if (stream->ulm_flags & ULM_SHARED_STREAM)
	    {
		/* Shareable streams are semaphore protected */
		block = (ULM_BLOCK *) ((char *)stream + ULM_SZ_STREAMS);
		CSw_semaphore(&stream->ulm_ssem, CS_SEM_SINGLE, 
			STprintf(sem_name, "ULM stream (%s)",
					owner_names[ulm_rcb->ulm_facility]));
	    }
	    else
		block = (ULM_BLOCK *) ((char *)stream + ULM_SZ_STREAMP);

	    stream->ulm_sbl1st = stream->ulm_sbllast =
		stream->ulm_sblcur = stream->ulm_sblspa  = block;
	    stream->ulm_snblks = 1;
	    block->ulm_bblnext = block->ulm_bblprev = (ULM_BLOCK *) NULL;
	    block->ulm_bstream = stream;
	    block->ulm_bblsize = block->ulm_bremain = sblsize;

	    /* Prepare to return with a good stream */
	    /* ------------------------------------ */
	    *streamid_p = (PTR)stream;

	    /*
	    ** If piggybacking a palloc onto this call, allocate the 
	    ** memory from this block and return a pointer to it.
	    */
	    if (psize)
	    {
		ulm_rcb->ulm_pptr = (PTR)((char*)block + ULM_SZ_BLOCK);
		block->ulm_bremain -= psize;
		if ((block->ulm_bremain -= (block->ulm_bremain % DB_ALIGN_SZ)) == 0)
		    stream->ulm_sblspa = (ULM_BLOCK*)NULL;
	    }
	}
	else
	{
	    /* ulm_nalloc() error; release exclusive access to the pool */
	    CSv_semaphore(&pool->ulm_psem);
	}
    }

#ifdef    xDEV_TEST
    if ( status == E_DB_OK )
	status = ulm_conchk(pool, ulm_rcb);

    if (status != E_DB_OK)
    {
	if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
	    ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
	    facility = owner_names[ulm_rcb->ulm_facility];
	else
	    facility = "UNKNOWN";
	TRdisplay("%@ ULM %s openstream ERROR pool 0x%x psize %d memleft %d error %d\n",
		facility, pool, ulm_rcb->ulm_psize, *ulm_rcb->ulm_memleft, 
		ulm_rcb->ulm_error.err_code);
    }
#endif /* xDEV_TEST */

    return (status);
}


/*{
** Name: ulm_closestream - Close and destroy a memory stream returning its
**			    memory to the pool.
**
** Description:
**      This routine will cause memory to be deallocated from a stream and 
**      returned to the pool of memory. 
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .*ulm_streamid_p            Stream id of stream to be closed
**                                      and who's memory will be returned to
**                                      the pool ... the .ulm_streamid should
**                                      not be used for any further calls to
**                                      the ULM.
**          .ulm_memleft                A pointer to some memory limit counter.
**					The caller can use this counter as
**					needed.  (i.e.  Limit for a stream,
**                                      limit for a set of streams, limit for
**                                      a session, limit for the server, or
**					whatever.)
**
** Outputs:
**      ulm_rcb
**          .ulm_memleft                The memory left will be incremented
**                                      by the amount of memory which gets
**                                      deallocated from the stream closure.
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0014_BAD_ULM_PARM   Bad parameter to this function.
**		    E_UL0004_CORRUPT	    Memory pool is corrupted.
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**		    E_UL0006_BADPTR	    Bad pointer given as block to be
**					    freed.
**		    E_UL0007_CANTFIND	    Can't find block to be freed.
**		    E_UL0009_MEM_SEMWAIT    Error waiting for exclusive access.
**		    E_UL000A_MEM_SEMRELEASE Error releasing exclusive access.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	17-apr-86 (thurston)
**	    Finished coding.
**	12-jan-88 (thurston)
**	    Now frees up the stream's blocks begining with the last block in the
**	    stream.  This is done to fit in with the fact that when a free node
**	    is split to create an allocated node, we take the allocated piece
**	    from the end of the original free node.  We should get better
**	    performance by freeing the blocks (one node per block) backwards.
**	13-jan-88 (thurston)
**	    Now frees the first data block along with the stream header, since
**	    they were allocated out of the same node.
**	13-jan-88 (thurston)
**	    Added semaphore protection around ulm_free() and the former
**	    ulm_stremove() call since it has been removed from ulm_free() and
**	    the code from ulm_stremove() was moved directly into this routine.
**	    This avoids multiple semaphore calls, thus having a positive effect
**	    on performance.
**	25-jan-88 (thurston)
**	    Fixed small bug introduced by recent performance mods.  The memleft
**	    counter was not getting "credited" for the size of the block header
**	    for the 1st block in the stream.  Thus, every ulm_openstream() -
**	    ulm_closestream() pair was effectively reducing the size of the
**	    client's memory pool by ULM_SZ_BLOCK bytes (20 bytes).
**	06-Aug-1997 (jenjo02)
**	    Sanity check pool and stream.
**	15-Aug-1997 (jenjo02)
**	    Remove shareable stream's semaphore.
*/
DB_STATUS 
ulm_closestream( register ULM_RCB *ulm_rcb )
{
    register ULM_STREAM	*stream;
    PTR			*streamid_p = (ulm_rcb->ulm_streamid_p != (PTR*)NULL)
				     ? ulm_rcb->ulm_streamid_p
				     : &ulm_rcb->ulm_streamid;
    register ULM_PHEAD	*pool;
    register ULM_BLOCK	*block;
    i4		size;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Check for bad parameter */
    /* ----------------------- */
    if (ulm_rcb->ulm_memleft == (SIZE_TYPE*)NULL)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
	return (E_DB_ERROR);
    }

    /* Validate stream and pool owner */
    if ((stream = (ULM_STREAM*)*streamid_p) == (ULM_STREAM*)NULL ||
	stream->ulm_self != stream ||
	stream->ulm_spool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    pool = stream->ulm_spool;

#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_closestream   pool = 0x%p  owner = %s  stream = 0x%p\n",
		pool, owner_names[ulm_rcb->ulm_facility], stream);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(pool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */

    /* Now go and get exclusive access to the pool */
    if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	/* Return now, instead of attempting to do a v() */
	return (E_DB_ERROR);
    }

    /* Clobber caller's streamid */
    *streamid_p = (PTR)NULL;

    
    /* Remove the stream from the pool's list */
    /* -------------------------------------- */

    /* adjust backward pointers */
    if (pool->ulm_pstlast == stream)	/* if last stream in pool ... */
	pool->ulm_pstlast = stream->ulm_sstprev;
    else
	stream->ulm_sstnext->ulm_sstprev = stream->ulm_sstprev;

    /* adjust forward pointers */
    if (pool->ulm_pst1st == stream)	/* if 1st stream in pool ... */
	pool->ulm_pst1st = stream->ulm_sstnext;
    else
	stream->ulm_sstprev->ulm_sstnext = stream->ulm_sstnext;

    stream->ulm_self = stream->ulm_sstnext = stream->ulm_sstprev = 
				(ULM_STREAM *) NULL;

    /*
    ** Now free up all of the streams blocks, last to second -- the first
    ** block will be freed with the stream header since they were allocated
    ** out of the same node.
    */
    while ((block = stream->ulm_sbllast) != stream->ulm_sbl1st &&
	    DB_SUCCESS_MACRO(status))
    {
#ifdef    xDEBUG
	if (block == (ULM_BLOCK *) NULL)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0004_CORRUPT);
	    break;
	}
#endif /* xDEBUG */

	stream->ulm_sbllast = block->ulm_bblprev;
	size = block->ulm_bblsize + ULM_SZ_BLOCK;
	status = ulm_free((PTR)block, (PTR)pool, ulm_rcb);
	if (DB_SUCCESS_MACRO(status))
	    *ulm_rcb->ulm_memleft += size;
    }

    if (DB_SUCCESS_MACRO(status))   /*...free the stream and first data block */
    {

	if (stream->ulm_flags & ULM_SHARED_STREAM)
	{
	    /* Release shareable stream's semaphore */
	    CSr_semaphore(&stream->ulm_ssem);
	    size = ULM_SZ_STREAMS + block->ulm_bblsize + ULM_SZ_BLOCK;
	}
	else
	    size = ULM_SZ_STREAMP + block->ulm_bblsize + ULM_SZ_BLOCK;

	status = ulm_free((PTR)stream, (PTR)pool, ulm_rcb);
	if (DB_SUCCESS_MACRO(status))
	    *ulm_rcb->ulm_memleft += size;
    }

    /* Release exclusive access to the pool */
    if (CSv_semaphore(&pool->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
	return (E_DB_ERROR);
    }


#ifdef    xDEV_TEST
    if ( status == E_DB_OK )
	status = ulm_conchk(pool, ulm_rcb);
#endif /* xDEV_TEST */

    return (status);
}


/*{
** Name: ulm_reinitstream - Reinitialize a memory stream, leaving its memory
**			    allocated to the stream, but resetting the "next
**			    piece" pointer to the beginning of the stream.
**
**	*****************************************************
**	*****************************************************
**	*****                                           *****
**	*****   N O T   Y E T   I M P L E M E N T E D   *****
**	*****                                           *****
**	*****************************************************
**	*****************************************************
**
** Description:
**      The reinitialization of a stream means that a stream is still allocated 
**      but all memory in the stream is available for reuse for the ulm_palloc()
**      routine.  That is, the next piece to be allocated will be the first
**	piece from the first block of the stream.
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's emory.
**          .*ulm_streamid_p            Stream id to be reinitialization ...
**                                      this id was returned from the
**                                      ulm_openstream() call.
**
** Outputs:
**	ulm_rcb
**          .ulm_error                  Standard DB_ERROR structure.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    All memory in the stream will be considered available to be reused
**          for ulm_palloc() calls.
**
** History:
**	27-mar-86 (seputis)
**          initial creation
*/

/*DB_STATUS   
**ulm_reinitstream(ulm_rcb)
**ULM_RCB             *ulm_rcb;
**{
**    [@block_decl@]...
**
**    {@statement@}...
**}
*/


/*{
** Name: ulm_palloc	- Allocate a memory piece from a memory stream.
**
** Description:
**      This routine is the lowest level of the memory management routines
**      that are visible to the ULM user.  It is used to allocate the small
**      contiguous pieces of memory in much the same way as MEalloc() previously
**      operated.  Memory is allocated from a stream, which was opened earlier
**      by a ulm_openstream() call.
**        
**      The .ulm_memleft counter is decremented only if a new internal block
**      was allocated to the stream.  If this counter indicates that the memory
**      limit has been reached (i.e. turns negative) then no memory would be
**      allocated and an error will be returned.  The purpose of the
**	.ulm_memleft counter is to prevent an individual thread (or "some
**	individual entity") from consuming all the memory resources within
**	a particular facility. 
**
**      Note that all memory pool operations will be protected by semaphores 
**      as internal data structures are updated.  However, the user of the
**      ulm_palloc() routine should protect against any anticipated multi-thread
**      access.  At this time, it appears that there should not be any need to
**      have multiple threads allocate memory from the SAME stream. 
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .*ulm_streamid_p            Stream id returned from the
**                                      ulm_openstream() call ... used to
**                                      identify the stream from which to
**					allocate memory.
**          .ulm_psize                  Size of memory piece to allocate, in
**					bytes.  It is legal for this to be
**                                      larger than the "prefered block size"
**                                      for the stream.
**          .ulm_memleft                Pointer to a memory limit counter.  This
**                                      will represent the amount of memory
**                                      which can be allocated ... note that one
**                                      session may have several streams
**                                      associated with this counter, so they
**                                      may want to point to the same counter
**                                      ... this is why it needs to be passed as
**                                      a parameter each time.  If the request
**                                      for memory would mean that this counter
**                                      would become negative, no memory will be
**                                      allocated and an error will be returned.
**
** Outputs:
**      ulm_rcb
**          .ulm_memleft                This routine may decrement the counter
**                                      pointed to by .ulm_memleft, since some
**                                      memory will be required if another
**                                      internal block needs to be added to the
**                                      stream to satisfy the request. 
**          .ulm_pptr                   Pointer to allocated memory piece.  ULM
**                                      guarantees that this will point to a
**                                      memory location that is aligned on an
**                                      sizeof(ALIGN_RESTRICT) byte boundry. 
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0014_BAD_ULM_PARM   Bad parameter to this function.
**		    E_UL0004_CORRUPT	    Pool is corrupted.
**		    E_UL0005_NOMEM	    Not enough memory.
**		    E_UL0009_MEM_SEMWAIT    Error waiting for exclusive access.
**		    E_UL000A_MEM_SEMRELEASE Error releasing exclusive access.
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Another internal memory block might be allocated from the pool
**	    to the stream in order to satisfy the request.  To do this, it may
**	    require allocating another "chunk" of the memory pool from SCF.
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	17-apr-86 (thurston)
**	    Finished coding.
**	13-jan-88 (thurston)
**	    Added semaphore protection around ulm_nalloc() call so it could be
**	    removed from that routine.  This does not effect the performance of
**	    this routine, but will have a positive effect on performance of
**	    other ULM routines.
**	8-nov-95 (inkdo01)
**	    Changed to start search in 1st block, rather than current block.
**	05-Jun-1997 (shero03)
**	    Protect any changes to stream within the semaphored section
**	06-Aug-1997 (jenjo02)
**	    Sanity check stream pointer.
**	15-Aug-1997 (jenjo02)
**	    If shareable stream, protect it with its semaphore.
**	    Modified inkdo01's 8-nov-95 change. Instead of always searching all
**	    blocks (even those which are full), keep track of the first block 
**	    with available space in a new stream field ulm_sblspa and begin
**	    the search from there.
**	11-nov-1998 (nanpr01)
**	    Donot fragment the memory unnecessarily. Same logic of 
**	    dm0m_allocate has been now implemented in ulm.
**      13-Sep-2006 (wanfr01)
**          Bug 116630
**          Must mutex ulm_memleft to avoid corruption and E_UL0005 errors.
**	13-Sep-2006 (wanfr01)
**	    Bug 116632 - added ULM diagnostics for E_UL0005
**      12-jul-2010 (huazh01)
**          Cast output to i8 if using %ld in TRdisplay(). (b124066)
*/

DB_STATUS
ulm_palloc( register ULM_RCB *ulm_rcb )
{
    register ULM_STREAM	*stream;
    PTR			*streamid_p = (ulm_rcb->ulm_streamid_p != (PTR*)NULL)
				     ? ulm_rcb->ulm_streamid_p
				     : &ulm_rcb->ulm_streamid;
    register ULM_PHEAD	*pool;
    register ULM_BLOCK	*cur_block;
    register ULM_BLOCK	*best_block = (ULM_BLOCK *) 0;
    PTR			cur_block_p;
    i4		data_size;
    i4		req_size;
    DB_STATUS		status;
    i4			mod;
    char		*facility;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Check for bad parameter */
    if (ulm_rcb->ulm_psize <= 0 ||
	ulm_rcb->ulm_memleft == (SIZE_TYPE*)NULL)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
	return (E_DB_ERROR);
    }

    /* Validate stream and pool owner */
    if ((stream = (ULM_STREAM*)*streamid_p) == (ULM_STREAM*)NULL ||
	stream->ulm_self != stream ||
	stream->ulm_spool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    pool = stream->ulm_spool;

#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_palloc        pool = 0x%p  owner = %s  stream = 0x%p\n",
		pool, owner_names[ulm_rcb->ulm_facility], stream);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(pool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */

    /*
    ** If shareable stream, lock it to prevent concurrent
    ** alterations to the stream's blocks.
    */
    if (stream->ulm_flags & ULM_SHARED_STREAM)
	CSp_semaphore(SEM_EXCL, &stream->ulm_ssem);

    /*
    ** Look for space in the current (last allocated block) first,
    ** then look for space starting with the first block with
    ** available space.
    */
    cur_block = stream->ulm_sblcur;
    status = E_DB_OK;

    if (cur_block->ulm_bremain < ulm_rcb->ulm_psize)
    {
	/* 
	** Look for space starting with first block with space.
	** If none can be found, allocate a new block.
	*/
	for (cur_block = stream->ulm_sblspa;
	     cur_block != (ULM_BLOCK*)NULL;
	     cur_block = cur_block->ulm_bblnext)
	{
	    if (cur_block->ulm_bremain < ulm_rcb->ulm_psize)
	    {
		/* Reached the end */
		if (cur_block->ulm_bblnext != (ULM_BLOCK *) NULL)
		    continue;
		else if (best_block)
		{
		    cur_block = best_block;
		    break;
		}
	    }
	    else
	    {
		/* Stop if exact fit */
		if (cur_block->ulm_bremain == ulm_rcb->ulm_psize)
		    break;

		/*
		** Best fit comprimise so we don't always do
		** exhaustive search
		*/
		if (cur_block->ulm_bremain <= ulm_rcb->ulm_psize + 512)
		    break;
		if (!best_block || 
		    cur_block->ulm_bremain < best_block->ulm_bremain) 
		    best_block = cur_block;

		/* Reached the end */
		if (cur_block->ulm_bblnext == (ULM_BLOCK *) NULL)
		{
		    cur_block = best_block;
		    break;
		}
	    }
	}
 
	/* If no block with sufficient space, get another */
	if (cur_block == (ULM_BLOCK*)NULL)
	{
	    data_size = (ulm_rcb->ulm_psize > stream->ulm_sblsize) 
			 ? ulm_rcb->ulm_psize 
			 : stream->ulm_sblsize;

	    if (mod = data_size % DB_ALIGN_SZ) data_size += DB_ALIGN_SZ - mod;
	    req_size = data_size + ULM_SZ_BLOCK;

	    /* Check to make sure memleft will not underflow */
	    if (req_size > *ulm_rcb->ulm_memleft)
	    {
		SETDBERR(&ulm_rcb->ulm_error, 4, E_UL0005_NOMEM);
		status = E_DB_ERROR;
	        if (ulm_rcb->ulm_facility != DB_OPF_ID) 
	        {
                    TRdisplay ("E_UL0005 at point 4: %d > %d\n",req_size,*ulm_rcb->ulm_memleft);

                    TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
                            __FILE__, __LINE__, ulm_rcb->ulm_poolid,
                            ulm_rcb->ulm_facility);
		}
	    }
	    else
	    {
		/* Must get exclusive access to the pool */
		if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
		{
		    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
		    /* Return now, instead of attempting to do a v() */
		    return (E_DB_ERROR);
		}
		status = ulm_nalloc((PTR) pool,
				    req_size,
				    &cur_block_p,
				    ulm_rcb);

		/* Release exclusive access to the pool */
		if (DB_SUCCESS_MACRO(status))
		{
		    *ulm_rcb->ulm_memleft -= req_size;
		}

		CSv_semaphore(&pool->ulm_psem);

		if ( status && ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM )
	        {
	            TRdisplay("DIAG: %s:%d ulm_poolid = %p ulm_facility = %d\n",
			            __FILE__, __LINE__, ulm_rcb->ulm_poolid,
			            ulm_rcb->ulm_facility);
	        }

		if (DB_SUCCESS_MACRO(status))
		{
		    cur_block = (ULM_BLOCK*)cur_block_p;
		    /*
		    ** Add this new block to the end of the stream
		    ** and initialize it
		    */
		    stream->ulm_snblks++;
		    cur_block->ulm_bblprev = stream->ulm_sbllast;
		    stream->ulm_sbllast->ulm_bblnext = cur_block;
		    stream->ulm_sbllast = stream->ulm_sblcur = cur_block;
		    cur_block->ulm_bblnext = (ULM_BLOCK *) NULL;
		    cur_block->ulm_bstream = stream;
		    cur_block->ulm_bblsize = cur_block->ulm_bremain = data_size;
		    if (stream->ulm_sblspa == (ULM_BLOCK*)NULL)
			stream->ulm_sblspa = cur_block;
		}
	    }
	}
    }

    if (DB_SUCCESS_MACRO(status))
    {
	/* Now calculate the address of the piece */
	ulm_rcb->ulm_pptr = (PTR) ((char *) cur_block +
		  (ULM_SZ_BLOCK + cur_block->ulm_bblsize - cur_block->ulm_bremain));
	cur_block->ulm_bremain -= ulm_rcb->ulm_psize;

	/* Round # bytes remaining in block down to a nice even number */
	if ((cur_block->ulm_bremain -= (cur_block->ulm_bremain % DB_ALIGN_SZ)) == 0 &&
	     cur_block == stream->ulm_sblspa)
	{
	    /*
	    ** This was the first block with space and now it has none.
	    ** Advance the stream's "first-block-with-space" pointer.
	    ** If no blocks have space, sblspa will end up NULL.
	    */
	    for (stream->ulm_sblspa = cur_block->ulm_bblnext;
		 stream->ulm_sblspa != (ULM_BLOCK*)NULL &&
		 stream->ulm_sblspa->ulm_bremain == 0;
		 stream->ulm_sblspa = stream->ulm_sblspa->ulm_bblnext);
	}
    }

    if (stream->ulm_flags & ULM_SHARED_STREAM)
	CSv_semaphore(&stream->ulm_ssem);

#ifdef    xDEV_TEST
    if ( status == E_DB_OK )
	status = ulm_conchk(pool, ulm_rcb);

    if (Ulm_pad_bytes && status != E_DB_OK)
    {
	if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
	    ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
	    facility = owner_names[ulm_rcb->ulm_facility];
	else
	    facility = "UNKNOWN";
	TRdisplay("%@ ULM %s alloc ERROR pool 0x%x psize %ld memleft %ld error %d\n",
		facility, pool, (i8)ulm_rcb->ulm_psize, (i8)*ulm_rcb->ulm_memleft, 
		ulm_rcb->ulm_error.err_code);
    }
#endif /* xDEV_TEST */

    return (status);
}

/*{
** Name: ulm_mark	- Mark a memory stream for reclaiming later.
**
** Description:
**      This routine creates a ULM_SMARK which represents a `stream mark'.  By
**	later passing this ULM_SMARK to the ulm_reclaim() routine, the ULM
**	client can free the memory allocated to that memory stream from the
**	end of the stream up to the `stream mark'.
**        
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .*ulm_streamid_p            Stream id returned from the
**                                      ulm_openstream() call ... used to
**                                      identify the stream from which to
**					allocate memory.
**          .ulm_smark                  Points to the ULM_SMARK structure that
**                                      will be filled in by this routine to
**                                      represent the `stream mark'.
**
** Outputs:
**      ulm_rcb
**          .ulm_smark                  The ULM_SMARK pointed to by this will be
**                                      filled in by this routine to represent
**                                      the `stream mark'.
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jul-87 (thurston)
**          Initial creation.
**	06-Aug-1997 (jenjo02)
**	    Sanity check stream.
**	    Return error if attempting to mark a shared stream.
*/

DB_STATUS
ulm_mark( register ULM_RCB *ulm_rcb )
{
    register ULM_STREAM	*stream;
    PTR			*streamid_p = (ulm_rcb->ulm_streamid_p != (PTR*)NULL)
				     ? ulm_rcb->ulm_streamid_p
				     : &ulm_rcb->ulm_streamid;
    register ULM_I_SMARK *smark;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Validate stream and pool owner */
    if ((stream = (ULM_STREAM*)*streamid_p) == (ULM_STREAM*)NULL ||
	stream->ulm_self != stream ||
	stream->ulm_spool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    /* Check for bad parameter */
    if ((smark = (ULM_I_SMARK*)ulm_rcb->ulm_smark) == (ULM_I_SMARK*)NULL ||
	 stream->ulm_flags & ULM_SHARED_STREAM)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
	return (E_DB_ERROR);
    }

#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_mark          pool = 0x%p  owner = %s  stream = 0x%p\n",
		stream->ulm_spool, owner_names[ulm_rcb->ulm_facility], stream);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(stream->ulm_spool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */

    smark->ulm_mstream = stream;
    smark->ulm_mblock  = stream->ulm_sblcur;
    smark->ulm_mremain = stream->ulm_sblcur->ulm_bremain;
    
#ifdef    xDEV_TEST
    if ( ulm_conchk(stream->ulm_spool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */

    return (E_DB_OK);
}


/*{
** Name: ulm_reclaim	- Reclaim a memory stream up to the supplied mark.
**
** Description:
**      This routine free memory from a memory stream to be reused.  The memory
**	freed is from the end of the stream up to the supplied `stream mark'.
**	This `stream mark' must have been built by a call to ulm_mark().
**        
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .ulm_memleft                A pointer to some memory limit counter.
**					The caller can use this counter as
**					needed.  (i.e.  Limit for a stream,
**                                      limit for a set of streams, limit for
**                                      a session, limit for the server, or
**					whatever.)
**          .ulm_smark                  Points to the ULM_SMARK structure that
**                                      represents the `stream mark'.
**
** Outputs:
**      ulm_rcb
**          .ulm_memleft                The memory left will be incremented
**                                      by the amount of memory which gets de-
**                                      allocated from the stream reclaimation.
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0004_CORRUPT	    Memory pool is corrupted
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**		    E_UL0009_MEM_SEMWAIT    Error waiting for exclusive access.
**		    E_UL000A_MEM_SEMRELEASE Error releasing exclusive access.
**		    E_UL0015_BAD_STREAM_MARK Stream mark supplied is bogus.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-jul-87 (thurston)
**          Initial creation.
**	12-jan-88 (thurston)
**	    Now frees up the stream's blocks begining with the last block in the
**	    stream.  This is done to fit in with the fact that when a free node
**	    is split to create an allocated node, we take the allocated piece
**	    from the end of the original free node.  We should get better
**	    performance by freeing the blocks (one node per block) backwards.
**	13-jan-88 (thurston)
**          Added semaphore protection around ulm_free() call since it has been
**          removed from that routine. This avoids multiple semaphore calls,
**          thus having a positive effect on performance. 
**	06-Aug-1997 (jenjo02)
**	    Sanity check stream.
**	15-Aug-1997 (jenjo02)
**	    Allow only in ULM_PRIVATE streams.
**      13-Sep-2006 (wanfr01)
**          Bug 116630
**          Must mutex ulm_memleft to avoid corruption and E_UL0005 errors.
*/

DB_STATUS
ulm_reclaim( register ULM_RCB *ulm_rcb )
{
    register ULM_I_SMARK *smark;
    register ULM_BLOCK	*cur_block, *block;
    register ULM_STREAM	*mstream;
    i4		size;
    ULM_PHEAD		*pool;
    DB_STATUS		status;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Check for an invalid stream mark */
    if (    (smark = (ULM_I_SMARK*)ulm_rcb->ulm_smark) == (ULM_I_SMARK *) NULL
	||  (cur_block = smark->ulm_mblock) == (ULM_BLOCK *) NULL
	||  (mstream = smark->ulm_mstream) == (ULM_STREAM *) NULL
	||  (mstream->ulm_self != mstream)
	||  (cur_block->ulm_bstream) != mstream
	||  (smark->ulm_mremain < cur_block->ulm_bremain)
	||  (ulm_rcb->ulm_memleft == (SIZE_TYPE*)NULL)
	||  (mstream->ulm_flags & ULM_SHARED_STREAM)
       )
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0015_BAD_STREAM_MARK);
	return (E_DB_ERROR);
    }

    /* Check to see if owner of pool */
    pool = mstream->ulm_spool;

    if (pool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }



#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_reclaim       pool = 0x%p  owner = %s  stream = 0x%p\n",
		pool, owner_names[ulm_rcb->ulm_facility], mstream);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(pool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */


    /*
    **  All looks kosher, so lets get rid of the trailing blocks in the stream:
    */
    status = E_DB_OK;
	
    /* If we're going to free any blocks, lock the pool */
    if (cur_block != mstream->ulm_sbllast)
    {
	if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
	{
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	    /* Return now, instead of attempting to do a v() */
	    return (E_DB_ERROR);
	}
	
	/* Now, loop through blocks from end of stream up to the mark */
	while ((block = mstream->ulm_sbllast) != cur_block && status == E_DB_OK)
	{
	    if (block == mstream->ulm_sblspa)
		mstream->ulm_sblspa = (ULM_BLOCK*)NULL;
	    mstream->ulm_sbllast = block->ulm_bblprev;
	    size = block->ulm_bblsize + ULM_SZ_BLOCK;
	    status = ulm_free((PTR)block, (PTR)pool, ulm_rcb);
	    if (status == E_DB_OK)
	    {
		*ulm_rcb->ulm_memleft += size;
		mstream->ulm_snblks--;
	    }
	}

	/* Release exclusive access to the pool */
	if (CSv_semaphore(&pool->ulm_psem))
	{
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
	    status = E_DB_ERROR;
	}
    }

    if (DB_SUCCESS_MACRO(status))
    {
	if (smark->ulm_mremain > cur_block->ulm_bremain)
	{
	    if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
	    {
		SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	        /* Return now, instead of attempting to do a v() */
	        return (E_DB_ERROR);
	    }

	    *ulm_rcb->ulm_memleft += 
		(smark->ulm_mremain - cur_block->ulm_bremain);

	    /* Release exclusive access to the pool */
	    if (CSv_semaphore(&pool->ulm_psem))
	    {
		SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
	        status = E_DB_ERROR;
	    }
	}
	cur_block->ulm_bremain = smark->ulm_mremain;
	cur_block->ulm_bblnext = (ULM_BLOCK *) NULL;
	mstream->ulm_sblcur = cur_block;
	if (mstream->ulm_sblspa == (ULM_BLOCK*)NULL &&
	    cur_block->ulm_bremain > 0)
	    mstream->ulm_sblspa = cur_block;
    }


#ifdef    xDEV_TEST
    if ( status == E_DB_OK )
	status = ulm_conchk(pool, ulm_rcb);
#endif /* xDEV_TEST */

    return (status);
}


/*{
** Name: ulm_mappool - Produce a memory map of the pool.
**
** Description:
**      This routine uses TRdisplay to produce a memory map of the supplied
**	memory pool.  Only a stub will be compiled unless xDEBUG is defined
**	for the compilation.
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .ulm_poolid                 Pool id for the memory pool to be
**                                      "cleaned up".
**
** Outputs:
**      ulm_rcb
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0009_MEM_SEMWAIT    Error waiting for exclusive access.
**		    E_UL000A_MEM_SEMRELEASE Error releasing exclusive access.
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-mar-88 (thurston)
**          Initial creation.
**	29-Nov-2000 (jenjo02)
**	    Removed xDEBUG ifdefs, always compile this code,
**	    to facilitate development/debugging, tolerate 
**	    semaphore failures.
**      17-Jul-2006 (hanal04) 116358
**          Make sure we do not try to dump a pool that has not been setup.
*/

DB_STATUS 
ulm_mappool( ULM_RCB *ulm_rcb )
{
    DB_STATUS		status = E_DB_OK;
    i4                  owner = ulm_rcb->ulm_facility;
    ULM_PHEAD		*pool = (ULM_PHEAD *) ulm_rcb->ulm_poolid;
    i4                  powner;
    ULM_HEAD		*freelist;
    ULM_NODE		*node;
    ULM_NODE		*freenode;
    ULM_STREAM		*stream;
    ULM_BLOCK		*block;
    ULM_CHUNK		*chunk;
    char		*chunkend;
    i4		tot_pool_size = 0;
    i4		tot_chunk_size = 0;
    i4		num_chunks = 0;
    i4		sz_chunks = 0;
    i4		num_streams = 0;
    i4		sz_streams = 0;
    i4		num_blocks = 0;
    i4		sz_blocks = 0;
    i4		num_free = 0;
    i4		sz_free = 0;
    i4		num_items = 0;
    i4		sz_items = 0;
    i4		tnum_streams = 0;
    i4		tsz_streams = 0;
    i4		tnum_blocks = 0;
    i4		tsz_blocks = 0;
    i4			i;
    STATUS	sem_was_held;

    CLRDBERR(&ulm_rcb->ulm_error);

    if(pool == (ULM_PHEAD*)NULL)
    {
        TRdisplay("ulm_mappool: exiting due to NULL pool pointer\n");
        return(E_DB_OK);
    }
    powner = pool->ulm_powner;
    freelist = &pool->ulm_pfrlist;

#ifdef    xDEV_TEST
#ifdef    xTR_OUTPUT
    TRdisplay("ulm_mappool       pool = 0x%p  owner = %s\n",
		pool, owner_names[powner]);
#endif /* xTR_OUTPUT */
    if ( ulm_conchk(pool, ulm_rcb) )
	return (E_DB_ERROR);
#endif /* xDEV_TEST */


    /* Check to see if owner of pool */
    if (owner != powner)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    /* Now go and get exclusive access to the pool */
    sem_was_held = CSp_semaphore(SEM_EXCL, &pool->ulm_psem);


    TRdisplay("\n\n=================================");
    TRdisplay("===============================\n");
    TRdisplay("BEGIN MEMORY POOL MAP FOR MEMORY POOL:  0x%p  owned by %s\n",
		(PTR) pool, owner_names[powner]);


		    /* Produce the "GENERAL POOL INFO" */
		    /* ------------------------------- */

    TRdisplay("\n\n                         <<< GENERAL POOL INFO >>>\n\n");
    TRdisplay("                               ");
    TRdisplay("# alloc'ed     pref chunk      pref block\n");
    TRdisplay(" Max # bytes    alloc bytes      chunks      ");
    TRdisplay("size (bytes)    size (bytes)\n");
    TRdisplay(" -----------    -----------    ----------    ");
    TRdisplay("------------    ------------\n");
    TRdisplay("%11d.%13d.%11d.%16d.%14d.\n",
		(i4) pool->ulm_psize_pag * SCU_MPAGESIZE,
		(i4) pool->ulm_pages_alloced * SCU_MPAGESIZE,
		(i4) pool->ulm_pchunks,
		(i4) pool->ulm_pchsize_pag * SCU_MPAGESIZE,
		(i4) pool->ulm_pblsize);


		    /* Produce the "POOL MAP" */
		    /* ---------------------- */

    TRdisplay("\n\n                            <<< POOL MAP >>>\n\n");
    TRdisplay("Start addr     End addr      # Bytes    ");
    TRdisplay("Item           Associated stream ID\n");
    TRdisplay("---------------------------------------");
    TRdisplay("-------------------------------------\n");

    chunk = pool->ulm_pch1st;
    while (chunk != (ULM_CHUNK *) NULL)
    {
	tot_chunk_size = (i4)chunk->ulm_cpages * SCU_MPAGESIZE;
	tot_pool_size += tot_chunk_size;
	chunkend = (char *)chunk + tot_chunk_size;
	TRdisplay("0x%p    0x%p%11d.    CHUNK_HDR\n",
		    (PTR) chunk,
		    (PTR) ((char *)chunk + ULM_SZ_CHUNK - 1),
		    (i4) ULM_SZ_CHUNK);
	num_chunks++;
	sz_chunks += ULM_SZ_CHUNK;
	node = (ULM_NODE *) ((char *)chunk + ULM_SZ_CHUNK);

	if (chunk == pool->ulm_pch1st)
	{
	    TRdisplay("0x%p    0x%p%11d.    POOL_HDR\n",
			(PTR) pool,
			(PTR) ((char *)pool + ULM_SZ_PHEAD - 1),
			(i4) ULM_SZ_PHEAD);
	    node = (ULM_NODE *) ((char *)pool + ULM_SZ_PHEAD);
	}

	while ((PTR)node < (PTR)chunkend)
	{
	    TRdisplay("0x%p    0x%p%11d.    NODE(",
			(PTR) node,
			(PTR) ((char *)node + node->ulm_nsize - 1),
			(i4) node->ulm_nsize);

	    /* First, see if node is on free list */
	    for (freenode = freelist->ulm_h1st;
		    freenode != (ULM_NODE *) NULL  &&  freenode != node;
			freenode = freenode->ulm_nnext)
		;

	    if (freenode != (ULM_NODE *) NULL)
	    {
		TRdisplay("FREE)\n");
		num_free++;
		sz_free += node->ulm_nsize;
	    }
	    else
	    {
		/* ... else, is this a stream header node?
		** To check this, it is currently sufficient to look at the
		** ulm_spool member of the `would-be' stream header.  If it is
		** equal to `pool', then this must be a stream header, because
		** no other structure enveloped in a node will have the pool
		** address at that offset.
		*/
		stream = (ULM_STREAM *) ((char *)node + ULM_SZ_NODE);
		if (stream->ulm_spool == pool)
		{
		    TRdisplay("STREAM)   for stream 0x%p\n",
				(PTR) stream);
		    num_streams++;
		    sz_streams += node->ulm_nsize;
		}
		else
		{
		    /* It must be a block */
		    block = (ULM_BLOCK *) ((char *)node + ULM_SZ_NODE);
		    TRdisplay("BLOCK)    for stream 0x%p\n",
				(PTR) block->ulm_bstream);
		    num_blocks++;
		    sz_blocks += node->ulm_nsize;
		}
	    }

	    node = (ULM_NODE *) ((char *)node + node->ulm_nsize);
	}
	
	TRdisplay("---------------------------------------");
	TRdisplay("-------------------------------------\n");
	TRdisplay("%35d. (Total bytes for CHUNK #%d)\n\n",
		    tot_chunk_size, num_chunks);
	chunk = chunk->ulm_cchnext;
    }
    TRdisplay("%35d. (Total bytes for entire POOL)\n", tot_pool_size);


		    /* Produce the "ITEM SUMMARY" */
		    /* -------------------------- */

    TRdisplay("\n\n                <<< ITEM SUMMARY >>>\n\n");
    TRdisplay("Item              Count   Total bytes       Avg\n");
    TRdisplay("------------------------------------------------\n");
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"POOL_HDR    ",
		(i4) 1,
		(i4) ULM_SZ_PHEAD,
		(i4) ULM_SZ_PHEAD);
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"CHUNK_HDR   ",
		(i4) num_chunks,
		(i4) sz_chunks,
		(i4) (num_chunks ? (sz_chunks / num_chunks) : 0));
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"NODE(STREAM)",
		(i4) num_streams,
		(i4) sz_streams,
		(i4) (num_streams ? (sz_streams / num_streams) : 0));
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"NODE(BLOCK) ",
		(i4) num_blocks,
		(i4) sz_blocks,
		(i4) (num_blocks ? (sz_blocks / num_blocks) : 0));
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"NODE(FREE)  ",
		(i4) num_free,
		(i4) sz_free,
		(i4) (num_free ? (sz_free / num_free) : 0));
    TRdisplay("------------------------------------------------\n");
    num_items = 1 + num_chunks + num_streams + num_blocks + num_free;
    sz_items = ULM_SZ_PHEAD + sz_chunks + sz_streams + sz_blocks + sz_free;
    TRdisplay("%12s%11d.%11d.%11d.\n",
		"    Totals  ",
		(i4) num_items,
		(i4) sz_items,
		(i4) (sz_items / num_items));


		    /* Produce the "STREAM SUMMARY" */
		    /* ---------------------------- */

    TRdisplay("\n\n                                                 ");
    TRdisplay("<<< STREAM SUMMARY >>>\n\n");
    TRdisplay("                      NODE(STREAM) Totals");
    TRdisplay("                 NODE(BLOCK) Totals");
    TRdisplay("                   All NODE Totals\n");
    TRdisplay(" Stream ID");
    for (i = 0; i < 3; i++)
	TRdisplay("       Count     # bytes        Avg ");
    TRdisplay("\n----------");
    for (i = 0; i < 3; i++)
	TRdisplay("       -----------------------------");

    sz_streams = 0;
    tnum_streams = 0;
    tsz_streams = 0;
    tnum_blocks = 0;
    tsz_blocks = 0;
    stream = pool->ulm_pst1st;
    while (stream != (ULM_STREAM *) NULL)
    {
	/* Generate "NODE(STREAM) Totals" for this stream */
	node = (ULM_NODE *) ((char *)stream - ULM_SZ_NODE);
	TRdisplay("\n0x%p", (PTR) stream);
	TRdisplay("%11d.%11d.%11d.",
		    (i4) 1,
		    (i4) node->ulm_nsize,
		    (i4) node->ulm_nsize);
	sz_streams = node->ulm_nsize;
	tnum_streams++;
	tsz_streams += node->ulm_nsize;

	/* Generate "NODE(BLOCK) Totals" for this stream */
	num_blocks = 0;
	sz_blocks = 0;
	block = stream->ulm_sbl1st;
	while ((block = block->ulm_bblnext) != (ULM_BLOCK *) NULL)
	{
	    node = (ULM_NODE *) ((char *)block - ULM_SZ_NODE);
	    num_blocks++;
	    sz_blocks += node->ulm_nsize;
	}
	TRdisplay("%11d.%11d.%11d.",
		    (i4) num_blocks,
		    (i4) sz_blocks,
		    (i4) (num_blocks ? (sz_blocks / num_blocks) : 0));
	tnum_blocks += num_blocks;
	tsz_blocks += sz_blocks;

	/* Generate "All NODE Totals" for this stream */
	TRdisplay("%11d.%11d.%11d.",
		    (i4) 1 + num_blocks,
		    (i4) sz_streams + sz_blocks,
		    (i4) ((sz_streams + sz_blocks) / (1 + num_blocks)));

	stream = stream->ulm_sstnext;
    }
    TRdisplay("\n----------");
    for (i = 0; i < 3; i++)
	TRdisplay("       -----------------------------");
    TRdisplay("\n    Totals");
    TRdisplay("%11d.%11d.%11d.",
		(i4) tnum_streams,
		(i4) tsz_streams,
		(i4) (tnum_streams ? (tsz_streams / tnum_streams) : 0));
    TRdisplay("%11d.%11d.%11d.",
		(i4) tnum_blocks,
		(i4) tsz_blocks,
		(i4) (tnum_blocks ? (tsz_blocks / tnum_blocks) : 0));
    TRdisplay("%11d.%11d.%11d.",
		(i4) tnum_streams + tnum_blocks,
		(i4) tsz_streams + tsz_blocks,
		(i4) ((tnum_streams + tnum_blocks)
				? ((tsz_streams + tsz_blocks)
					/ (tnum_streams + tnum_blocks))
				: 0));
    TRdisplay("\n\n    # streams   Total bytes    Avg bytes\n");
    TRdisplay(    "    ---------   ------------   ----------\n");
    TRdisplay("%11d.%15d.%12d.\n",
		(i4) tnum_streams,
		(i4) tsz_streams + tsz_blocks,
		(i4) (tnum_streams
				? ((tsz_streams + tsz_blocks) / tnum_streams)
				: 0));


		    /* END OF OUTPUT */
		    /* ------------- */

    TRdisplay("\n\nEND MEMORY POOL MAP FOR MEMORY POOL:  0x%p  owned by %s\n",
		(PTR) pool, owner_names[powner]);
    TRdisplay("=================================");
    TRdisplay("===============================\n\n\n");
    
    /* Release exclusive access to the pool */
    if ( !(sem_was_held) )
        CSv_semaphore(&pool->ulm_psem);


#ifdef    xDEV_TEST
    if ( status == E_DB_OK )
        status = ulm_conchk(pool, ulm_rcb);
#endif /* xDEV_TEST */


    return (status);
}


/*{
** Name: ulm_xcleanup - Does cleanup in a memory pool after an exception.
**
** Description:
**      This routine may need to do some cleanup work in a memory pool, such as
**	releasing the pool semaphore, if an exception was detected by the ULM
**	client.  If an exception is detected, and it was possible that ULM was
**	being used, then the client's exception handler should call this routine
**	before even attempting to release any ULM resources.
**
** Inputs:
**      ulm_rcb
**          .ulm_facility               Facility code of the calling facility.
**					(Use constants in DBMS.H.)  This will
**					be used to assure that only the owner
**					of the pool is using the pool's memory.
**          .ulm_poolid                 Pool id for the memory pool to be
**                                      "cleaned up".
**
** Outputs:
**      ulm_rcb
**          .ulm_error                  Standard DB_ERROR structure.
**		.err_code		If return status is not E_DB_OK ...
**		    E_UL0004_CORRUPT	    Pool is corrupted.
**		    E_UL000B_NOT_ALLOCATED  Pool was not allocated by you.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Releases the pool's semaphore.
**
** History:
**	12-feb-88 (thurston)
**          Initial creation.
**	06-Aug-1997 (jenjo02)
**	    Sanity check pool.
*/

DB_STATUS 
ulm_xcleanup( ULM_RCB *ulm_rcb )
{
    ULM_PHEAD		*pool;
    DB_STATUS		status = E_DB_OK;

    /* Sanity check pool and owner */
    if ((pool = (ULM_PHEAD*)ulm_rcb->ulm_poolid) == (ULM_PHEAD*)NULL ||
	 pool->ulm_self != pool ||
	 pool->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    /* Now just free the pool semaphore, and ignore errors */
    CSv_semaphore(&pool->ulm_psem);


#ifdef    xDEV_TEST
    status = ulm_conchk(pool, ulm_rcb);
#endif /* xDEV_TEST */

    return (status);
}


#ifdef    NOT_YET_READY
/*{
** Name: ulm_szstream - How many bytes/nodes of memory pool does a stream use?
**
** Description:
**	Calculates the number of bytes and nodes allocated to the given memory
**	stream.
**
** Inputs:
**      stream_id	Memory stream ID.
**
** Outputs:
**      nbytes		Number of bytes currently allocated to this stream.
**	nnodes		Number of nodes currently allocated to this stream.
**
**	Returns:
**	    E_UL0000_OK			All is fine in memory land.
**	    E_UL0004_CORRUPT		Memory pool is hosed.
**	    E_UL0009_MEM_SEMWAIT	Couldn't set semaphore.
**	    E_UL000A_MEM_SEMRELEASE	Couldn't release semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-mar-88 (thurston)
**          Initial creation.
*/

i4
ulm_szstream( PTR stream_id, i4 *nbytes, i4 *nnodes )
{
    i4		err = E_UL0000_OK;
    ULM_NODE		*node;
    ULM_STREAM		*stream = (ULM_STREAM *) stream_id;
    ULM_POOL		*pool = (ULM_PHEAD *) stream->ulm_spool;
    ULM_BLOCK		*block = stream->ulm_sbl1st;


    /* First, wait to get exclusive access to the pool */
    /* ----------------------------------------------- */
    if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
    {
	err = E_UL0009_MEM_SEMWAIT;
	/* Return now, instead of attempting to do a v() */
	return (err);
    }

    *nnodes = stream->ulm_snblks;

    node = (ULM_NODE *) ((char *)stream - ULM_SZ_NODE);
    nbytes = node->ulm_nsize - ULM_SZ_NODE;

    block;
    while ((block = block->ulm_bblnext) != (ULM_BLOCK *) NULL)
    {
	node = (ULM_NODE *) ((char *)block - ULM_SZ_NODE);
	nbytes += (node->ulm_nsize - ULM_SZ_NODE);
    }

    /* Release exclusive access to the pool */
    /* ------------------------------------ */
    if (CSv_semaphore(&pool->ulm_psem))
    {
	err = E_UL000A_MEM_SEMRELEASE;
    }

    return (err);
}
#endif /* NOT_YET_READY */


/**
** Name: ulm_pinit	- Initialize a memory pool.
**
** Description:
**      Given a pointer to an initial "chunk" of contiguous memory for a memory
**      pool, the total (or maximum) size of the pool, the size of the initial
**      chunk, and a couple other assorted goodies, this function will
**      initialize the pool for use by the ulm_nalloc() and ulm_free(). 
**
** Inputs:
**      poolsize_pag                    Max # of SCU_MPAGESIZE byte pages to
**                                      allow for pool.
**      pref_chunksize_pag              Preferred # of SCU_MPAGESIZE byte pages
**					to allocated for each chunk of the pool.
**      chunksize_pag                   # of SCU_MPAGESIZE byte pages allocated
**                                      for the initial chunk of the pool.
**	owner				Facility code for memory owner.
**	pblksize			Preferred size of data blocks for pool.
**					If zero, ULM will choose.
**      chunk                           Pointer to initial chunk of memory.
**      pool                            Place to put pointer to memory pool.
**	err_code			Place to put error code.
**
** Outputs:
**      pool                            Pointer to memory pool is placed here.
**                                      Beginning of pool is filled with header
**					information.
**	err_code			Filled in with error code on error.
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-mar-86 (jeff)
**          adapted from ME.c
**	10-apr-86 (thurston)
**	    Added the pblksize parameter, and also intialized the pool's
**	    stream list.
**	23-apr-86 (thurston)
**	    Changed the declaration of this routine to be static so as to
**	    prevent any routine outside of this file to call it.
**	01-aug-86 (thurston)
**	    Upgraded to use SCU_MPAGESIZE instead of hard-wired 512.
**	29-oct-87 (thurston)
**	    Modified to allow memory for a pool to be allocated from SCF in
**	    large, contiguous "chunks" as opposed to all at startup.
**	04-nov-87 (thurston)
**	    Added the pref_chunksize_pag argument.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	12-jan-88 (thurston)
**	    Modified code to maintain the backward pointers in the free and
**	    allocated lists.
**      13-aug-91 (jrb)
**          Initialize pool semaphore using CSi_semaphore instead of MEfilling
**          with zeros.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	03-dec-93 (swm)
**	    Bug #58635
**	    Cast owner to PTR in assignment to ulm_owner in ULM_CHUNK type.
**	    Although the value is not a pointer in this case, ULM_CHUNK must
**	    declare ulm_owner to be PTR for consistency with the DM_SVCB
**	    standard DMF header.
**	06-Aug-1997 (jenjo02)
**	    Added ulm_self to sanity check pool.
**	22-May-2007 (wanfr01)
**	    Bug 118380
**	    Initialize ulm_minreq
*/

static DB_STATUS
ulm_pinit( SIZE_TYPE  poolsize_pag, SIZE_TYPE  pref_chunksize_pag, 
	   SIZE_TYPE  chunksize_pag,
	   i4 pblksize, PTR chunk, PTR *pool,
	   ULM_RCB *ulm_rcb )
{
    ULM_CHUNK		*chunkhead = (ULM_CHUNK *) chunk;
    ULM_PHEAD		*poolhead = (ULM_PHEAD *)((char *)chunk + ULM_SZ_CHUNK);
    ULM_HEAD            *listhead = &poolhead->ulm_pallist;
    ULM_HEAD		*freehead = &poolhead->ulm_pfrlist;
    ULM_NODE		*freenode = (ULM_NODE *) ((char *) poolhead +
								ULM_SZ_PHEAD);
    i4		*pblsize  = &poolhead->ulm_pblsize;
    i4			mod;
    char		sem_name[ CS_SEM_NAME_LEN ];

    /* Set up initial chunk */
    chunkhead->ulm_cchnext = (ULM_CHUNK *) NULL;
    chunkhead->ulm_cchprev = (ULM_CHUNK *) NULL;
    chunkhead->ulm_length = sizeof(ULM_CHUNK);
    chunkhead->ulm_type = ULCHK_TYPE;
    chunkhead->ulm_owner = (PTR)(SCALARP)ulm_rcb->ulm_facility;
    chunkhead->ulm_ascii_id = ULCHK_ASCII_ID;
    chunkhead->ulm_cpool = poolhead;
    chunkhead->ulm_cpages = chunksize_pag;
    
    /* Initialize the pool semaphore */
    if (CSw_semaphore(&poolhead->ulm_psem, CS_SEM_SINGLE,
	    STprintf( sem_name, "ULM pool (%s)", owner_names[ulm_rcb->ulm_facility])) != OK)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	return (E_DB_ERROR);
    }

    /*
    ** Put the owner code, pool size, and preferred
    ** data block size, etc. at the beginning of the pool.
    */
    poolhead->ulm_self = poolhead;
    poolhead->ulm_powner = ulm_rcb->ulm_facility;
    poolhead->ulm_psize_pag = poolsize_pag;
    poolhead->ulm_pages_alloced = chunksize_pag;
    poolhead->ulm_pchsize_pag = pref_chunksize_pag;
    poolhead->ulm_pchunks = 1;
    poolhead->ulm_pch1st = chunkhead;
    poolhead->ulm_pchlast = chunkhead;
    poolhead->ulm_minreq = MAXI4;
    *pblsize = (pblksize ? pblksize : ULM_BLSIZE_DEFAULT);
    if (mod = *pblsize % DB_ALIGN_SZ) *pblsize += DB_ALIGN_SZ - mod;

    /* Start out with nothing in the allocated list */
    listhead->ulm_h1st = (ULM_NODE *) NULL;
    listhead->ulm_hlast = (ULM_NODE *) NULL;

    /* Start out with one huge node in the free list */
    freehead->ulm_h1st = freenode;
    freehead->ulm_hlast = freenode;

    /* Initialize the one huge node */
    freenode->ulm_nnext = (ULM_NODE *) NULL;
    freenode->ulm_nprev = (ULM_NODE *) NULL;
    /*
    ** The amount of memory in the node is the initial chunk size,
    ** minus the chunk header, minus the pool header.
    */
    freenode->ulm_nsize = (chunksize_pag * SCU_MPAGESIZE) -
			  (ULM_SZ_CHUNK + ULM_SZ_PHEAD);
    freenode->ulm_nowner = ulm_rcb->ulm_facility;

    /* Start out with nothing in the stream list */
    poolhead->ulm_pst1st = (ULM_STREAM *) NULL;
    poolhead->ulm_pstlast = (ULM_STREAM *) NULL;


    *pool = (PTR) poolhead;

    return (E_DB_OK);
}


/**
** Name: ulm_nalloc	- Allocate some memory out of a memory pool.
**
** Description:
**      This function allocates memory from a memory pool.  The algorithm
**	was stolen from MEdoalloc in ME.c, with minor modifications.  The
**	memory pool must be initialized by calling ulm_pinit() before anything
**	is allocated from it.
**
**	*** ---- ***
**	*** NOTE ***  This routine assumes that the pool semaphore has been set!
**	*** ---- ***
**
** Inputs:
**      pool                            Pointer to memory pool.
**      size                            Number of bytes to allocate.
**      block                           Place to put pointer to allocated
**					memory.
**	err_code			Place to put error code.
**	err_data			Place to put additional error info.
**
** Outputs:
**      block                           Filled in with pointer to allocated mem.
**	err_code			Filled in with error code on error ...
**	    E_UL0014_BAD_ULM_PARM	Bad parameter to this function.
**	    E_UL0004_CORRUPT		Pool is corrupted.
**	    E_UL0005_NOMEM		Not enough memory.
**	err_data			Aditional info about the error may be
**					filled in here.
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory from the pool.  Fools with free list and allocated
**	    list.
**
** History:
**	18-mar-86 (jeff)
**          adapted from MEdoalloc() in ME.c
**	08-apr-86 (thurston)
**	    Fixed three bugs:
**	    (1) Several pointer initializations were being done BEFORE the
**	    pool semaphore was set.  This could have caused problems when
**	    twho or more threads were attempting to use the pool concurrently.
**	    (2) Calculating the size of a node header was using the statement,
**	       "size += sizeof(i4) - (sizeof(i4) % size)"
**	    to do rounding up to the nearest i4.  This statement has been
**	    changed to,
**	       "if (mod = size % DB_ALIGN_SZ) size += DB_ALIGN_SZ - mod"
**	    which does the correct thing (i.e. nothing) when size is already
**	    divisible by DB_ALIGN_SZ.  (*** Note the use of "DB_ALIGN_SZ"
**	    instead of "sizeof(i4)".  This will be more portable.)
**	    (3) When splitting a free node because the size of the block being
**	    asked for was smaller than the free node that will be used, the
**	    left over chunk of the node gets put back on the free list as a
**	    smaller node.  This was being done incorrectly.
**	17-apr-86 (thurston)
**	    Changed the name of this routine from ulm_alloc() to ulm_nalloc().
**	23-apr-86 (thurston)
**	    Changed the declaration of this routine to be static so as to
**	    prevent any routine outside of this file to call it.
**	17-mar-87 (daved)
**	    adjust last freenode pointer
**	16-oct-87 (thurston)
**	    Added the `setsem' argument so ulm_spalloc() can set the pool
**	    semaphore itself, and then use this routine without having it
**	    attempt to set the semaphore and deadlock itself.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	12-jan-88 (thurston)
**	    Modified code to use and maintain the backward pointers in the free and
**	    allocated lists.
**	13-jan-88 (thurston)
**	    Removed the semaphore code from this routine.  This routine will now
**	    assume that it is semaphore protected.  (Also removed the `setsem'
**	    argument since it is no longer meaningfull.)  By doing this,
**	    routines that do several calls to ulm_nalloc() or other `semaphore
**	    setters' will run faster by only having to set the semaphore once.
**	11-feb-88 (thurston)
**	    Added extra arg to ulm_nalloc() for the err_data.
**	13-Sep-2006 (wanfr01)
**	    Bug 116632 - added ULM diagnostics for E_UL0005
**	22-May-2007 (wanfr01)
**	    Bug 118380 - Do not leave blocks in free pool if they are smaller
**	    than your smallest memory request.  They won't be used and will
**	    just add to fragmentation.
**	08-Jun-2007 (wanfr01)
**	    Bug 118476 - Trim down ULM diags
**	    E_UL0005 can happens internally on big systems when QEF/QSF 
**	    pools get full.
**      27-Mar-2009 (hanal04) Bug 121857
**          Correct #if which should have been #ifdef
**	08-Jul-2009 (kibro01) b122284
**	    Addition to bug 118380 - do not leave blocks in the free pool if:
**		1 - The list is longer than ULM_FREELIST_TOO_LONG, which is an
**		    estimate of how long it can be before it starts taking a
**		    performance-degrading time to work through it for every
**		    allocation/free.
**		2 - The amount left in this block is smaller than the item
**		    currently being allocated.
**		3 - The amount left in this block is smaller than half the
**		    difference between the list length and the trigger for
**		    list length (ULM_FREELIST_TOO_LONG), so it gets less and
**		    less likely to leave memory as the list grows longer.
**	    The worst case for this amendment is that a medium-size block with
**	    a little less remaining at the end of a legitimately-long chain
**	    would use almost double the memory, but bear in mind that is only
**	    the final element of an originally-allocated chunk, so many of them
**	    will already have been allocated by this point.
*/

static DB_STATUS
ulm_nalloc( register PTR pool, i4 size, register PTR *block, ULM_RCB *ulm_rcb )
{
    ULM_PHEAD		*poolhead = (ULM_PHEAD *) pool;
    ULM_HEAD            *listhead;
    ULM_HEAD		*freehead;
    register ULM_NODE   *next, *prev;
    ULM_NODE   		*current, *alloc_ptr;
    i4		remain;
    i4			mod;
    i4	orig_size = size;
    DB_STATUS		status = E_DB_OK;
    i4  freecnt =1;
    i4  freetot =0;
    i4  listlen = 0;

    listhead = &poolhead->ulm_pallist;
    freehead = &poolhead->ulm_pfrlist;
    current = (ULM_NODE *) freehead->ulm_h1st;


    if (block == (PTR *) NULL || size <= 0)
    {
	status = E_DB_ERROR;
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
    }
    else
    {
	/* Add in size of node header */
	size += ULM_SZ_NODE;
	if (Ulm_pad_bytes)
	    size += Ulm_pad_bytes;
	/* Round up to nearest DB_ALIGN_SZ */
	if (mod = size % DB_ALIGN_SZ) size += DB_ALIGN_SZ - mod;

	if (size < poolhead->ulm_minreq)
	{
	    poolhead->ulm_minreq=size;
	}

        /*
	** Look on free list for first node big enough to hold request.
	*/

	while (current != (ULM_NODE *) NULL && current->ulm_nsize < size)
	{
#ifdef    xDEBUG
	    if (current->ulm_nowner != poolhead->ulm_powner)
	    {
		status = E_DB_ERROR;
		SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0004_CORRUPT);
		break;
	    }
#endif /* xDEBUG */

	    current = current->ulm_nnext;
	    listlen++;
	}

	if (DB_SUCCESS_MACRO(status)  &&  current == (ULM_NODE *) NULL)
	{
	    i4 poolsum, poolnum;
	    /*
	    ** Attempt to allocate and add another chunk to the pool,
	    ** make it into one big node, add to free list, and return
	    ** pointer to that node.
	    */

#ifdef xDEBUG
	    ulm_countpool(poolhead,&poolsum,&poolnum,&freecnt, &freetot);
	    if (freecnt > 0)
	        TRdisplay("\n0x%p Free chain:  %d blocks, %d tot, %d avg\n",poolhead,freecnt, freetot, freetot/freecnt);
	    else
	        TRdisplay("\n0x%p Free chain:  %d blocks, %d tot, --- avg\n",poolhead,freecnt, freetot);
	    TRdisplay("0x%p Total Used pool: %d blocks, %d total  \n",poolhead, poolnum, poolsum);
	    TRdisplay("0x%p pool stats: limit: %d blocks, alloced  %d \n",poolhead,poolhead->ulm_psize_pag, poolhead->ulm_pages_alloced);
#endif
	    status = ulm_challoc(poolhead, size, &current, ulm_rcb);

            if ( status && ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
	    {
		TRdisplay("DIAG: %s:%d E_UL0005_NOMEM\n", __FILE__, __LINE__);
	    }
	}

	if (DB_SUCCESS_MACRO(status))
	{
	    next = current->ulm_nnext;
	    prev = current->ulm_nprev;

	    /*
	    ** If current is correct size or would leave useless
	    ** node in chain:
	    **	    just move node to allocated list
	    ** else:
	    **	    grab what is needed from end of 'current' node
	    **	    and then update 'current'
	    ** Extra calculation of useless, based on the length of the 
	    ** existing free chain - if we couldn't fit another of these
	    ** items into it, and the free chain has already grown long,
	    ** don't make the free chain grow forever (kibro01) b122284
	    */
	    remain = current->ulm_nsize - size;
	    if (remain <= poolhead->ulm_minreq ||
	      (remain <= (listlen - ULM_FREELIST_TOO_LONG)/2 && remain < size))
	    {
		alloc_ptr = current;
		size += remain;

		/* adjust forward pointers */
		if (prev == (ULM_NODE *) NULL)	/* if this was the first     */
		{				/* node in the free list ... */
		    freehead->ulm_h1st = next;
		}
		else
		{
		    prev->ulm_nnext = next;
		}
		
		/* adjust backward pointers */
		if (next == (ULM_NODE *) NULL)	/* if this was the last      */
		{				/* node in the free list ... */
		    freehead->ulm_hlast = prev;
		}
		else
		{
		    next->ulm_nprev = prev;
		}
	    }
	    else
	    {
		alloc_ptr = (ULM_NODE *) ((char *) current + remain);
		current->ulm_nsize = remain;
	    }

	    /*
	    ** Set values in node holding block to be returned.
	    */
	    alloc_ptr->ulm_nowner = poolhead->ulm_powner;
	    alloc_ptr->ulm_nsize = size;
	    alloc_ptr->ulm_nnext = (ULM_NODE *) NULL;
	    alloc_ptr->ulm_nprev = listhead->ulm_hlast;

	    /* Put node into allocated list */
	    if (listhead->ulm_hlast == (ULM_NODE *) NULL)
	    {
		listhead->ulm_h1st = alloc_ptr;
	    }
	    else
	    {
		listhead->ulm_hlast->ulm_nnext = alloc_ptr;
	    }

	    listhead->ulm_hlast = alloc_ptr;
	    *block = (PTR) (((char *) alloc_ptr) + ULM_SZ_NODE);

#ifdef xDEBUG
	    TRdisplay("ULM alloc %x nnext %x nprev %x nsize %d orig_size %d \n",
		alloc_ptr, alloc_ptr->ulm_nnext, alloc_ptr->ulm_nprev, 
		alloc_ptr->ulm_nsize,  orig_size);
#endif

	    if (Ulm_pad_bytes)
	    {
		i4	*ip;
		char	*cp;
		SYSTIME stime;

		/* Pad buffer with PAD_CHAR for Ulm_pad_bytes */
		MEfill(Ulm_pad_bytes, PAD_CHAR, (PTR)((char *) *block) + orig_size);

		/* Put the original size requested in the last 4 pad bytes */
                /* and a copy in the preceding 4 pad  bytes */
		ip = (i4 *)(((char *)alloc_ptr) + alloc_ptr->ulm_nsize - sizeof(i4));
		*ip = orig_size; 
                ip--; /* four bytes */
                *ip = orig_size;

		/* Put the SYSTIME before the original size */
		cp = (char *)ip - sizeof(SYSTIME);
		TMnow(&stime);
		MEcopy(&stime, sizeof(SYSTIME), cp);
	    }
	}
    }

    return (status);
}


/**
** Name: ulm_free	- Free memory within a memory pool
**
** Description:
**      This function puts memory from a memory pool back on a free list
**	and, via ulm_fadd() coalesces adjacent nodes on the free list.
**
**	*** ---- ***
**	*** NOTE ***  This routine assumes that the pool semaphore has been set!
**	*** ---- ***
**
** Inputs:
**      block                           The block to be freed
**      pool                            Pool the block was allocated from
**	err_code			Pointer to place to put error code
**
** Outputs:
**	err_code			Set to error code on error
**	    E_UL0014_BAD_ULM_PARM	Bad parameter to this function
**	    E_UL0004_CORRUPT		Memory pool is corrupted
**	    E_UL0006_BADPTR		Bad pointer given as block to be freed
**	    E_UL0007_CANTFIND		Can't find block to be freed
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Returns block to the free list, possibly coalescing nodes on the
**	    free list.
**
** History:
**	18-mar-86 (jeff)
**          adapted from MEfree() in ME.c
**	08-apr-86 (thurston)
**	    Fixed a bug:  Several pointer initializations were being done
**	    BEFORE the pool semaphore was set.  This could have caused
**	    problems when two or more threads were attempting to use the pool
**	    concurrently.  Also, checked the block pointer to see if it was
**	    less than the first possible block in the pool.  (Checking was
**	    already being done for greater than the last possible block in
**	    the pool.)
**	23-apr-86 (thurston)
**	    Changed the declaration of this routine to be static so as to
**	    prevent any routine outside of this file to call it.
**	01-aug-86 (thurston)
**	    Upgraded to use SCU_MPAGESIZE instead of hard-wired 512.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	12-jan-88 (thurston)
**	    Modified code to use and maintain the backward pointers in the free and
**	    allocated lists.
**	13-jan-88 (thurston)
**	    Removed the semaphore code from this routine.  This routine will now
**	    assume that it is semaphore protected.  By doing this, routines that
**	    do several calls to ulm_free() or other `semaphore setters' will run
**	    faster by only having to set the semaphore once.
**	14-jul-1993 (rog)
**	    Unnested some comments.
*/

static DB_STATUS
ulm_free( register PTR block, register PTR pool, ULM_RCB *ulm_rcb )
{
    ULM_PHEAD		*poolhead = (ULM_PHEAD *) pool;
    ULM_HEAD		*listhead;
    ULM_HEAD		*freehead;
    register ULM_NODE	*node, *next, *prev;
    DB_STATUS		status;


    status = E_DB_OK;

    listhead = &poolhead->ulm_pallist;
    freehead = &poolhead->ulm_pfrlist;

    if (block == (PTR) NULL)
    {
	status = E_DB_ERROR;
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0014_BAD_ULM_PARM);
    }
/*******************************************************************************
** (Removed this check since a pool may be made up of several non-contiguous
** chunks of memory.  Perhaps we will eventually add a BAD_PTR check that checks
** the pointer against each chunk.) -- Gene Thurston, 30-oct-87
********************************************************************************
**  else if ((block > (PTR) ((char *) pool + (poolhead->ulm_psize_pag *
**								SCU_MPAGESIZE)))
**	 ||  (block < (PTR) ((char *) pool + ULM_SZ_PHEAD + ULM_SZ_NODE)))
**  {
**	status = E_DB_ERROR;
**      SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0006_BADPTR);
**  }
*******************************************************************************/
    else
    {
	/* Remove the node containing this block from the allocated list */
	/* ------------------------------------------------------------- */
	node = (ULM_NODE *) ((char *) block - ULM_SZ_NODE);
	next = node->ulm_nnext;
	prev = node->ulm_nprev;

	/* adjust forward pointers */
	if (prev == (ULM_NODE *) NULL)	    /* if node was first one */
	{				    /* in allocated list ... */
	    listhead->ulm_h1st = next;
	}
	else
	{
	    prev->ulm_nnext = next;
	}

	/* adjust backward pointers */
	if (next == (ULM_NODE *) NULL)	    /* if node was last one  */
	{				    /* in allocated list ... */
	    listhead->ulm_hlast = prev;
	}
	else
	{
	    next->ulm_nprev = prev;
	}

	/* Now add this node back to the free list */
	/* --------------------------------------- */
	status = ulm_fadd(node, poolhead->ulm_powner, freehead, ulm_rcb);
    }

    return (status);
}


/**
** Name: ulm_fadd	- Add a node to the free list.
**
** Description:
**      This function adds a node to the free list for a memory pool.
**	The free list is kept in ascending order of node address, and
**	adjacent nodes will be merged together whenever possible.
**
**	*** ---- ***
**	*** NOTE ***  This routine assumes that the pool semaphore has been set!
**	*** ---- ***
**
** Inputs:
**      node                            Pointer to the node
**	owner				Owner of the memory pool
**      freehead                        Pointer to the head of the free list
**	err_code			Pointer to place to put error code
**
** Outputs:
**	err_code			Filled in with error code on error
**	    E_UL0004_CORRUPT		Memory pool is corrupted
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Puts the node back on the free list.
**
** History:
**	18-mar-86 (jeff)
**          adapted from MEf_add() in ME.c
**	23-apr-86 (thurston)
**	    Changed the declaration of this routine to be static so as to
**	    prevent any routine outside of this file to call it.
**	18-mar-87 (daved)
**	    If adding a node past the current last node, adjust
**	    last node pointer.
**	01-jun-87 (thurston)
**	    If the node being added is adjacent to the last node in the free
**	    list it gets coalesced ... however, the last node pointer was not
**	    being adjusted.  This is now fixed.
**	02-nov-87 (thurston)
**	    Added the `pnode' argument.
**	12-jan-88 (thurston)
**	    Modified code to use and maintain the backward pointers in the free and
**	    allocated lists.
**	12-jan-88 (thurston)
**	    Removed the `pnode' argument.  It is no longer needed since the free
**	    and allocated lists are now doubly linked.
**	27-may-1997 (shero03)
**	    Added debugging code to fill a freed storage piece to a unusable
**	    pattern
*/

static DB_STATUS
ulm_fadd( register ULM_NODE *node, i4  owner, register ULM_HEAD *freehead, ULM_RCB *ulm_rcb )
{
    register ULM_NODE	*current = freehead->ulm_h1st;
    register ULM_NODE	*prev = (ULM_NODE *) NULL;
    PTR			node_end;	/* 1st address after end of 'node' */
    DB_STATUS		status = E_DB_OK;
    i4			orig_len, orig_len2;
    i4			i;
    char		*padchar;

    node_end = (PTR) (((char *) node) + node->ulm_nsize);

#ifdef xDEBUG
	TRdisplay("ULM free  %x nnext %x nprev %x nsize %d \n",
	    node, node->ulm_nnext, node->ulm_nprev, node->ulm_nsize);
#endif

    if (Ulm_pad_bytes && !(node->ulm_nnext == NULL && node->ulm_nprev == NULL))
    {
	orig_len = *(i4 *) (((char *)node) + node->ulm_nsize - sizeof(i4));
        orig_len2 = *(i4 *) (((char *)node) + node->ulm_nsize - (sizeof(i4) * 2));
        if(orig_len != orig_len2)
        {
            padchar = (((char *)node) + node->ulm_nsize - Ulm_pad_bytes);
            ulm_pad_error(node, padchar);
        }
        else
        {
	    padchar = (((char *) node) + orig_len + ULM_SZ_NODE);
 
	    /* Consistency check for pads */ 
	    for (i = 0; i < Ulm_pad_bytes - ULM_PAD_OVERHEAD; i++, padchar++) 
	    { 
	        if (*padchar != PAD_CHAR)
	        {
		    ulm_pad_error(node, padchar);
		    break; /* generate pad error once */
	        }
	    }
        }
    }

    /* If padding memory, invalidate the memory when it is freed */
    if (Ulm_lfill)
    {
	MEfill(node->ulm_nsize - sizeof(ULM_NODE), 0xFE,
		  (PTR)((char *) node) + sizeof(ULM_NODE));
    }

    /*
    ** Search free list for place to put node.
    */
    while ((current != (ULM_NODE *) NULL)  &&  ((PTR) current < node_end))
    {
#ifdef    xDEBUG
	if (current->ulm_nowner != owner)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0004_CORRUPT);
	    break;
	}
#endif /* xDEBUG */

	prev = current;
	current = current->ulm_nnext;
    }

    if (DB_SUCCESS_MACRO(status))
    {
	/*
	** If prev node is adjacent
	**	coalesce
	** else
	**	put node into chain
	*/

	if ((prev != (ULM_NODE *) NULL) &&
	    ((ULM_NODE *) ((char *) prev + prev->ulm_nsize)) == node)
	{
	    prev->ulm_nsize += node->ulm_nsize;
	}
	else
	{
	    /* set up the node's pointers */
	    node->ulm_nnext = current;
	    node->ulm_nprev = prev;

	    /* adjust forward pointers */
	    if (prev == (ULM_NODE *) NULL)	/* if the node will go at the */
	    {					/* top of the free list ...   */
		freehead->ulm_h1st = node;
	    }
	    else
	    {
		prev->ulm_nnext = node;
	    }

	    /* adjust backward pointers */
	    if (current == (ULM_NODE *) NULL)	/* if the node will go at the */
	    {					/* end of the free list ...   */
		freehead->ulm_hlast = node;
	    }
	    else
	    {
		current->ulm_nprev = node;
	    }

	    prev = node;
	}

	/*
	** If next node is adjacent
	**	coalesce
	**
	** NOTE: prev now points to 'node' or a node containing it
	*/
	if ( (ULM_NODE *) (((char *) prev) + prev->ulm_nsize) == current)
	{
	    prev->ulm_nsize += current->ulm_nsize;
	    prev->ulm_nnext = current->ulm_nnext;

	    if (current->ulm_nnext == (ULM_NODE *)NULL)	/* if this is now last*/
	    {						/* node in free list  */
		freehead->ulm_hlast = prev;
	    }
	    else
	    {
		current->ulm_nnext->ulm_nprev = prev;
	    }
	}
    }

    return (status);
}


/**
** Name: ulm_challoc	- Allocate another chunk, create node, add to free list
**
** Description:
**	This routine allocates another chunk of memory from SCF for the memory
**	pool, adds it to the chunk list, makes one big node out of it, and adds
**	that node to the list of free nodes.  The address of this node is
**	returned to the caller, and the pool header information is brought up
**	to date. 
**
**	Chunks will be allocated in SCU_MPAGESSIZE byte pages, and the size of
**	each chunk will be Ulm_chunk_size bytes except in the following
**	situations:
**	    (a) When the pool was initialized, the preferred block size
**		specified would require larger chunks.  In this situation, the
**		chunk size selected will be large enough to hold one block.
**	    (b) `nodesize' (which is supplied in bytes) would require it to be
**		bigger.  In this case, the smallest number of pages that can
**		contain a node of `nodesize' bytes will be used.
**	    (c) The memory pool limit would be exceeded.  In this case, the
**		chunk will be smaller, and the number of pages left to allocate
**		will be used.
**	    (d) SCF would not give this much memory.  In this case, if the
**		amount of memory is sufficient for a node of size `nodesize',
**		then it will be used.  If not, the memory will be handed back to
**		SCF, and the E_UL0005_NOMEM error will be generated.
**
**	*** ---- ***
**	*** NOTE ***  This routine assumes that the pool semaphore has been set!
**	*** ---- ***
**
** Inputs:
**      pool                            Pointer to the memory pool.
**      nodesize			Need a node at least this big.
**	node				Place to put ptr to the node created.
**      err_code                        Pointer to place to put error code
**					if an error happened.
**      err_data                        Pointer to place to put additional info
**					about the error.
**
** Outputs:
**	*node				Set to point to the node created.
**					This will be set to NULL if there
**					was an error.
**      *err_code                       If an error ocurred (i.e. the routine
**					returns a status other than E_DB_OK),
**					then a more meaningful error code will
**					be returned here.  Most likely,
**					    E_UL0005_NOMEM.
**      *err_data                       If an error ocurred additional info
**                                      about the error may be placed here.
**
**	Returns:
**	    E_DB_OK, E_DB_WARNING, E_DB_ERROR, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Another large, contiguous chunk of memory will be allocated by SCF.
**	    Pointers in the pool header, and other fields therein will be
**	    altered, as well as the free list.
**
** History:
**	03-oct-87 (thurston)
**          Initial creation.
**	12-jan-88 (thurston)
**	    Modified code to maintain the backward pointers in the free and
**	    allocated lists.
**	12-jan-88 (thurston)
**	    Removed the `prev' argument.  It is no longer needed since the free
**	    and allocated lists are now doubly linked.
**	11-feb-88 (thurston)
**	    Added extra arg to ulm_challoc() for the err_data.
**	03-dec-93 (swm)
**	    Bug #58635
**	    Cast powner to PTR in assignment to ulm_owner in ULM_CHUNK type.
**	    Although the value is not a pointer in this case, ULM_CHUNK must
**	    declare ulm_owner to be PTR for consistency with the DM_SVCB
**	    standard DMF header.
**	14-Dec-2000 (jenjo02)
**	    If there are at least "needed" pages remaining in pool yet
**	    not enough to satisfy "preferred" number of pages, get
**	    all remaining pages rather than returning E_UL0005_NOMEM.
**	24-sep-04 (toumi01)
**	    Add diagnostics for memory allocation failure.
**      09-nov-04 (stial01)
**          More diagnostics for memory allocation failure.
**	13-Sep-2006 (wanfr01)
**	    Bug 116632 - added ULM diagnostics for E_UL0005
**      12-jul-2010 (huazh01)
**          Cast output to i8 if using %ld in TRdisplay(). (b124066)
*/

static DB_STATUS
ulm_challoc( register ULM_PHEAD *pool, i4 nodesize, ULM_NODE **node, ULM_RCB *ulm_rcb )
{
    DB_STATUS 		status;
    SIZE_TYPE		premain_pag;
    SIZE_TYPE		need_pag;
    SIZE_TYPE		csize_pag;
    ULM_CHUNK		*chunk_ptr;
    ULM_NODE		*node_ptr;
    ULM_HEAD		*freelist = &pool->ulm_pfrlist;
    i4			powner = pool->ulm_powner;
    char		*facility;
    i4			error;
    SCF_CB		scf_cb;


    /* Calculate size of chunk needed */
    /* ------------------------------ */
    premain_pag = pool->ulm_psize_pag - pool->ulm_pages_alloced;
    need_pag = ((nodesize + ULM_SZ_CHUNK) - 1)/SCU_MPAGESIZE + 1;

    if (powner >= DB_MIN_FACILITY &&
	powner <= DB_MAX_FACILITY)
	facility = owner_names[powner];
    else
	facility = "unknown";

    /*
    ** Allocate the greater of needed and preferred number
    ** of pages.
    **
    ** If that number is greater than the number of remaining
    ** pages and there are at least as many remaining pages
    ** as the number needed, get all remaining pages,
    ** otherwise the pool is out of memory.
    */
    if ( (csize_pag = max(need_pag, pool->ulm_pchsize_pag)) > premain_pag &&
	 (csize_pag = premain_pag) < need_pag )
    {
	SETDBERR(&ulm_rcb->ulm_error, 6, E_UL0005_NOMEM);
	*node = (ULM_NODE *) NULL;
	return (E_DB_ERROR);
    }

    /* Now, prepare to get memory from SCF */
    /* ----------------------------------- */
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = powner;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = csize_pag;

    /* Now ask SCF for the memory */
    /* -------------------------- */
    if (scf_call(SCU_MALLOC, &scf_cb) != E_DB_OK)
    {
	TRdisplay("DIAG E_UL0005_NOMEM:%s:%d\n", __FILE__, __LINE__);

	TRdisplay("%@ ULM %s Alloc failed: psize_pag %d, pages_alloc %d remain %ld\n",
		facility, pool->ulm_psize_pag, pool->ulm_pages_alloced, 
		(i8)premain_pag);
	TRdisplay("%@ ULM %s Alloc failed: nodesize %d  needed %ld csize %ld\n",
		facility, nodesize, (i8)need_pag, (i8)csize_pag);

#ifdef xDEBUG
	/* UL0017 .. and query printed only if ULM diagnostics configured */
	if (Ulm_lfill)
	    ule_format(E_UL0017_DIAGNOSTIC, (CL_SYS_ERR *)0, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &error, 1,
		24, "ULM chunk allocate error");
#endif

	SETDBERR(&ulm_rcb->ulm_error, 7, E_UL0005_NOMEM);
	*node = (ULM_NODE *) NULL;
	return (E_DB_ERROR);
    }
    csize_pag = scf_cb.scf_scm.scm_out_pages;
    chunk_ptr = (ULM_CHUNK *) scf_cb.scf_scm.scm_addr;

    /* Did we get enough? */
    /* ------------------ */
    if (csize_pag < need_pag)
    {
	/*
	** No, we did not.  Before returning with
	** an error, release memory chunk to SCF.
	*/
	_VOID_ scf_call(SCU_MFREE, &scf_cb);

	TRdisplay("%@ ULM %s Alloc insufficient: psize_pag %d, pages_alloc %d remain %d\n",
		facility, pool->ulm_psize_pag, pool->ulm_pages_alloced, 
		premain_pag);
	TRdisplay("%@ ULM %s Alloc insufficient: nodesize %d  needed %ld csize %ld\n",
		facility, nodesize, (i8)need_pag, (i8)csize_pag);
	
	/* UL0017 .. and query printed only if ULM diagnostics configured */
#ifdef xDEBUG
	if (Ulm_lfill)
	    ule_format(E_UL0017_DIAGNOSTIC, (CL_SYS_ERR *)0, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &error, 1,
		31, "ULM chunk allocate insufficient");
#endif

	SETDBERR(&ulm_rcb->ulm_error, 8, E_UL0005_NOMEM);
	*node = (ULM_NODE *) NULL;
	return (E_DB_ERROR);
    }


    /* We have chunk allocated from SCF, now initialize it */
    /* --------------------------------------------------- */
    chunk_ptr->ulm_cchnext = (ULM_CHUNK *) NULL;
    chunk_ptr->ulm_cchprev = pool->ulm_pchlast;
    chunk_ptr->ulm_length = sizeof(ULM_CHUNK);
    chunk_ptr->ulm_type = ULCHK_TYPE;
    chunk_ptr->ulm_owner = (PTR)(SCALARP)powner;
    chunk_ptr->ulm_ascii_id = ULCHK_ASCII_ID;
    chunk_ptr->ulm_cpool = pool;
    chunk_ptr->ulm_cpages = csize_pag;
    
    /* Update the chunk list and pool header */
    /* ------------------------------------- */
    pool->ulm_pchlast->ulm_cchnext = chunk_ptr;
    pool->ulm_pages_alloced += csize_pag;
    pool->ulm_pchunks++;
    pool->ulm_pchlast = chunk_ptr;  /* Note that this routine will never be used
				    ** used to allocate the 1st chunk.  That is
				    ** done by the ulm_startup()/ulm_pinit()
				    ** combo.
				    */

    /* Now set up the one big node */
    /* --------------------------- */
    node_ptr = (ULM_NODE *) ((char *) chunk_ptr + ULM_SZ_CHUNK);
    node_ptr->ulm_nnext = (ULM_NODE *) NULL;
    node_ptr->ulm_nprev = (ULM_NODE *) NULL;
    node_ptr->ulm_nsize = (csize_pag * SCU_MPAGESIZE) - ULM_SZ_CHUNK;
    node_ptr->ulm_nowner = powner;

    /* ... and add it to the free list */
    /* ------------------------------- */
    status = ulm_fadd(node_ptr, powner, freelist, ulm_rcb);

    
    if (DB_SUCCESS_MACRO(status))
    {
	TRdisplay("%@ ULM %s Extend pool at 0x%p to %d pages\n", facility, pool,pool->ulm_pages_alloced);
	TRdisplay("%@ ULM %s Allocate chunk %d %ld pages at 0x%p\n",
		    facility, pool->ulm_pchunks, (i8)csize_pag, chunk_ptr);

	*node = node_ptr;
    }
    else
    {
	TRdisplay("%@ ULM %s ERROR Can't Extend pool at 0x%p\n", facility, pool);
	TRdisplay("%@ ULM %s ERROR Can't Allocate chunk %d %ld pages at 0x%p\n",
		    facility, pool->ulm_pchunks, (i8)csize_pag, chunk_ptr);

	*node = (ULM_NODE *) NULL;
    }
    
    return (status);
}


/**
** Name: ulm_trace - Perform consistency check on memory pool, print allocated.
**                   memory blocks
**
** Description:
**	Does several consistency checks on the memory pool.
**
** Inputs:
**      pool                            Pointer to the memory pool.
**
** Outputs:
**      none
**
**	Returns:
**	    E_UL0000_OK			All is fine in memory land.
**	    E_UL0004_CORRUPT		Memory pool is hosed.
**	    E_UL0009_MEM_SEMWAIT	Couldn't set semaphore.
**	    E_UL000A_MEM_SEMRELEASE	Couldn't release semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-oct-98 (stial01)
**          Exposed ulm_conchk as a tracepoint
*/
DB_STATUS
ulm_trace( register ULM_RCB *ulm_rcb )
{
    register ULM_STREAM	*stream;
    PTR			*streamid_p = (ulm_rcb->ulm_streamid_p != (PTR*)NULL)
				     ? ulm_rcb->ulm_streamid_p
				     : &ulm_rcb->ulm_streamid;
    register ULM_PHEAD  *pool;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Do a consistency check now */
    stream = (ULM_STREAM*)*streamid_p;
    if (stream && stream->ulm_self == stream &&
	stream->ulm_spool->ulm_powner == ulm_rcb->ulm_facility)
    {
	TRdisplay("Performing ULM consistency check %@.\n");
	pool = stream->ulm_spool;
	(VOID) ulm_conchk(pool, ulm_rcb);
    }
    else
	status = E_DB_ERROR;

    return (status);
}


/**
** Name: ulm_conchk	- Perform consistency checks on a given memory pool.
**
** Description:
**	Does several consistency checks on the memory pool.
**
** Inputs:
**      pool                            Pointer to the memory pool.
**
** Outputs:
**      none
**
**	Returns:
**	    E_UL0000_OK			All is fine in memory land.
**	    E_UL0004_CORRUPT		Memory pool is hosed.
**	    E_UL0009_MEM_SEMWAIT	Couldn't set semaphore.
**	    E_UL000A_MEM_SEMRELEASE	Couldn't release semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-jun-87 (thurston)
**          Initial creation.
**	30-oct-87 (thurston)
**	    Modified to handle pool allocations in chunks.  Also added extra
**	    checks to check the chunk list.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	12-jan-88 (thurston)
**	    Added code to check the backward pointer linkage in both the free
**	    and allocated lists.  Also added code to check the address ordering
**          of the free list (that is, the "current" node in the free list must
**          always be at a higher address than the "previous" node, it does not
**	    exist "inside" the previous node, and it is not adjacent to the
**	    previous node).
**	12-feb-88 (thurston)
**	    Added the failed_check argument.  Callers of this routine will
**	    probably send in the address of the err_data field.
*/

static DB_STATUS
ulm_conchk( ULM_PHEAD *pool, ULM_RCB *ulm_rcb )
{
    ULM_HEAD            *listhead;
    ULM_HEAD            *freehead;
    ULM_NODE		*current;
    ULM_NODE		*prev;
    ULM_STREAM		*scurrent;
    ULM_STREAM		*sprev;
    ULM_CHUNK		*ccurrent;
    ULM_CHUNK		*cprev;
    i4		numstreams;
    i4		listnodes;
    i4		freenodes;
    i4		listsize;
    i4		freesize;
    SIZE_TYPE		poolbytes;
    SIZE_TYPE		chunkbytes;
    SIZE_TYPE		tot_nodebytes;
    i4		pchunks;
    i4		numchunks;
    i4		prev_size;
    i4             alloc_count = 0;
    i4             alloc_size = 0;


    /* First, wait to get exclusive access to the pool */
    /* ----------------------------------------------- */
    if (CSp_semaphore(SEM_EXCL, &pool->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	/* Return now, instead of attempting to do a v() */
	return (E_DB_ERROR);
    }


    poolbytes  = pool->ulm_psize_pag * SCU_MPAGESIZE;
    chunkbytes = pool->ulm_pages_alloced * SCU_MPAGESIZE;
    pchunks = pool->ulm_pchunks;
    tot_nodebytes = chunkbytes - (ULM_SZ_PHEAD + (pchunks * ULM_SZ_CHUNK));


    /* check the chunk list */
    /* -------------------- */
    cprev = (ULM_CHUNK *) NULL;
    ccurrent = pool->ulm_pch1st;
    numchunks = 0;

    if (ccurrent != (ULM_CHUNK *) ((char *) pool - ULM_SZ_CHUNK))
    {
	/*
	** First chunk is not directly in front of pool header.
	** ----------------------------------------------------
	*/
	SETDBERR(&ulm_rcb->ulm_error, 1, E_UL0004_CORRUPT);
    }

    while (ccurrent != (ULM_CHUNK *) NULL)
    {
	numchunks++;

	if (    ccurrent->ulm_cchprev != cprev
	    ||  ccurrent->ulm_cpool != pool
	   )
	{
	    /*
	    ** Either linked list of chunks is bad, or
	    ** chunk does not point to the proper pool.
	    ** ----------------------------------------
	    */
	    SETDBERR(&ulm_rcb->ulm_error, 2, E_UL0004_CORRUPT);
	    break;
	}

	cprev = ccurrent;
	ccurrent = ccurrent->ulm_cchnext;
    }

    if (!ulm_rcb->ulm_error.err_code  &&  cprev != pool->ulm_pchlast)
    {
	/*
	** Last chunk ptr does not point to the last chunk in list.
	** --------------------------------------------------------
	*/
	SETDBERR(&ulm_rcb->ulm_error, 3, E_UL0004_CORRUPT);
    }

    if (!ulm_rcb->ulm_error.err_code  &&  numchunks != pchunks)
    {
	/*
	** Number of chunks in list does not
	** match chunk counter in pool header.
	** -----------------------------------
	*/
	SETDBERR(&ulm_rcb->ulm_error, 4, E_UL0004_CORRUPT);
    }


    if (!ulm_rcb->ulm_error.err_code)
    {
	/* check the stream list */
	/* --------------------- */
	sprev = (ULM_STREAM *) NULL;
	scurrent = pool->ulm_pst1st;
	numstreams = 0;

	while (scurrent != (ULM_STREAM *) NULL)
	{
	    numstreams++;

	    if (    scurrent->ulm_sstprev != sprev
		||  scurrent->ulm_spool != pool
	       )
	    {
		/*
		** Either linked list of streams is bad, or
		** stream does not point to the proper pool.
		** -----------------------------------------
		*/
		SETDBERR(&ulm_rcb->ulm_error, 5, E_UL0004_CORRUPT);
		break;
	    }

	    sprev = scurrent;
	    scurrent = scurrent->ulm_sstnext;
	}

	if (!ulm_rcb->ulm_error.err_code  &&  sprev != pool->ulm_pstlast)
	{
	    /*
	    ** Last stream ptr does not point to the last stream in list.
	    ** ----------------------------------------------------------
	    */
	    SETDBERR(&ulm_rcb->ulm_error, 6, E_UL0004_CORRUPT);
	}
    }


    if (!ulm_rcb->ulm_error.err_code)
    {
	/* check the allocated list */
	/* ------------------------ */
	listhead = &pool->ulm_pallist;
	prev = (ULM_NODE *) NULL;
	current = listhead->ulm_h1st;
	listsize = 0;
	listnodes = 0;

	while (current != (ULM_NODE *) NULL)
	{
	    listnodes++;
	    listsize += current->ulm_nsize;
	    TRdisplay("current %x size %d next %x prev %x owner %d\n", 
			current, current->ulm_nsize, current->ulm_nnext, 
			current->ulm_nprev,
			current->ulm_nowner);
	    alloc_count++;
	    alloc_size += current->ulm_nsize;


	    if (    current->ulm_nsize <= 0
		||  listsize > tot_nodebytes
		||  current->ulm_nowner != pool->ulm_powner
		||  current->ulm_nprev != prev
	       )
	    {
		/*
		** Either node size is zero or negative, running sum of node
                ** sizes is bigger than possible, the node owner is different
                ** from the pool owner, or the previous pointer does not point
                ** to the previous node in the allocated list. 
		** -----------------------------------------------------------
		*/
		SETDBERR(&ulm_rcb->ulm_error, 7, E_UL0004_CORRUPT);
		break;
	    }

	    prev = current;
	    current = current->ulm_nnext;
	}

	TRdisplay("SUMMARY Allocated pieces %d total size %d at %@. \n", 
	alloc_count, alloc_size);

	if (!ulm_rcb->ulm_error.err_code  &&  prev != listhead->ulm_hlast)
	{
	    /*
	    ** Last allocated node ptr does not point
	    ** to the last allocated node in list.
	    ** --------------------------------------
	    */
	    SETDBERR(&ulm_rcb->ulm_error, 8, E_UL0004_CORRUPT);
	}
    }


    if (!ulm_rcb->ulm_error.err_code)
    {
	/* check the free list */
	/* ------------------- */
	freehead = &pool->ulm_pfrlist;
	prev = (ULM_NODE *) NULL;
	prev_size = 0;
	current = freehead->ulm_h1st;
	freesize = 0;
	freenodes = 0;

	while (current != (ULM_NODE *) NULL)
	{
	    freenodes++;
	    freesize += current->ulm_nsize;

	    if (    current->ulm_nsize <= 0
		||  listsize + freesize > tot_nodebytes
		||  current->ulm_nowner != pool->ulm_powner
		||  current->ulm_nprev != prev
		||  current <= (ULM_NODE *) ((char *)prev + prev_size)
	       )
	    {
		/*
		** Either node size is zero or negative, running sum of node
                ** sizes is bigger than possible, the node owner is different
                ** from the pool owner, the previous pointer does not point to
                ** the previous node in the free list, or the current node is
		** not at a higher address than the previous node plus its size.
		** -------------------------------------------------------------
		*/
		SETDBERR(&ulm_rcb->ulm_error, 9, E_UL0004_CORRUPT);
		break;
	    }

	    prev = current;
	    prev_size = prev->ulm_nsize;
	    current = current->ulm_nnext;
	}

	if (!ulm_rcb->ulm_error.err_code  &&  prev != freehead->ulm_hlast)
	{
	    /*
	    ** Last free node ptr does not point
	    ** to the last free node in list.
	    ** ---------------------------------
	    */
	    SETDBERR(&ulm_rcb->ulm_error, 10, E_UL0004_CORRUPT);
	}
    }


    if (!ulm_rcb->ulm_error.err_code)
    {
	/* Now check to see that the sizes of the allocated nodes and   */
	/* the free nodes plus the size of the pool header adds up to   */
	/* the size of all of the chunks allocated so far for the pool. */
	/* ------------------------------------------------------------ */
	if ((listsize + freesize) != tot_nodebytes)
	{
	    /*
	    ** Total of node sizes does not jive with total chunk size.
	    ** --------------------------------------------------------
	    */
	    SETDBERR(&ulm_rcb->ulm_error, 11, E_UL0004_CORRUPT);
	}
    }


    if (ulm_rcb->ulm_error.err_code)
    {
	TRdisplay("******************************************************\n");
	TRdisplay("************* ULM MEMORY POOL IS CORRUPT *************\n");
	TRdisplay("****          --------------------------          ****\n");
	TRdisplay("****  which_check = %2d %27s****\n",
		    ulm_rcb->ulm_error.err_data, 
		    conchk_names[ulm_rcb->ulm_error.err_data]);
	TRdisplay("******************************************************\n");
    }


    /* Release exclusive access to the pool */
    /* ------------------------------------ */
    if (CSv_semaphore(&pool->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
    }

    return( (ulm_rcb->ulm_error.err_code) ? E_DB_ERROR : E_DB_OK );
}


static VOID
ulm_pad_error(ULM_NODE *node, char *padchar)
{
    i4		orig_len, orig_len2;
    i4		error;

     /* This is a separate function so you can set a breakpoint at the error */
    orig_len = *(i4 *) (((char *)node) + node->ulm_nsize - sizeof(i4));
    orig_len2 = *(i4 *) (((char *)node) + node->ulm_nsize - (sizeof(i4) * 2));
    TRdisplay("ULM pad error %x %x (%x %x %x %d %d %d)\n", 
	padchar, *padchar,
	node, node->ulm_nnext, node->ulm_nprev, node->ulm_nsize, 
	orig_len, orig_len2);
    ule_format(E_UL0017_DIAGNOSTIC, (CL_SYS_ERR *)0, ULE_LOG, NULL, 
	(char *)0, (i4)0, (i4 *)0, &error, 1,
	13, "ULM pad error");
}

#define PRINT_MAX	10	

/* given a pool pointer, print out the freelist in this pool */
static void
ulm_printpool(PTR pool_ptr, char *facility)
{
    ULM_PHEAD   *pool = (ULM_PHEAD *)pool_ptr; 	/* current memory pool */
    ULM_NODE    *current = pool->ulm_pfrlist.ulm_hlast; /* free list */
    i4          free_cnt;	/* # of element in the free list */
    i4		alloc_cnt;	/* # elements in the allocated list */
    i4		print_cnt;	/* # elements hex dumped */
    i4		print_max;      /* max number of hex dumps */
    i4		i;
    bool        reset = FALSE; 
    char	*cp;
    i4		nsize;
    char	stimestr[TM_SIZE_STAMP + 80];
    bool	pad_error;

    /* Print the free list */
    free_cnt = 0;
    for (current = pool->ulm_pfrlist.ulm_hlast; 
	    current != (ULM_NODE *)NULL;
		current = current->ulm_nprev)
    {
	TRdisplay("ULM %s Freelist 0x%p,0x%p Size %d\n", facility,
	    current, (ULM_NODE *)((char *) current + current->ulm_nsize),
	    current->ulm_nsize);
	free_cnt++;
    }

    TRdisplay("%@ ULM %s Freelist count %d\n", facility, free_cnt);

    /* Print allocated blocks */
    alloc_cnt = 0;
    if (free_cnt > pool->ulm_pchunks)
    {
	MEfill(sizeof(stimestr), '\0', stimestr);
	stimestr[0] = '\0';
	TRdisplay("%@ ULM %s Allocated List Last %p 1st %p\n",
		facility, pool->ulm_pallist.ulm_hlast,
		pool->ulm_pallist.ulm_h1st);

	for (current = pool->ulm_pallist.ulm_hlast;
		current != (ULM_NODE *)NULL;
		    current = current->ulm_nprev)
	{
	    alloc_cnt++;

	    pad_error = FALSE;
	    if (Ulm_pad_bytes)
	    {
		SYSTIME		*stime;
		char		*padchar;
		i4		orig_len, orig_len2;

		stime = (SYSTIME *)
		(((char *)current) + current->ulm_nsize - ULM_PAD_OVERHEAD);
                orig_len = *(i4 *) (((char *)current) + current->ulm_nsize -
                           sizeof(i4)); 
		orig_len2 = *(i4 *) (((char *)current) + current->ulm_nsize - 
                            (sizeof(i4) * 2));
		/*
		** Note not all extra bytes are padded
		** (ulm_nalloc may have added extra bytes if < ULM_SZ_NODE
		** remaining)
		*/
		TMstr(stime, stimestr);
                if(orig_len != orig_len2)
                {
                    padchar = (((char *) current) + current->ulm_nsize 
                                                  - Ulm_pad_bytes);
                    pad_error = TRUE;
                }
                else
                {
		    padchar = (((char *) current) + orig_len + ULM_SZ_NODE);
		    if (*padchar != PAD_CHAR)
		        pad_error = TRUE;
                }
	    }
	    TRdisplay("ULM %s Size %12d 0x%p,0x%p %s %s\n", facility,
		current->ulm_nsize, current,
		(ULM_NODE *)((char *) current + current->ulm_nsize), stimestr,
		pad_error ? "PadErr" : "");
	}

	TRdisplay("%@ ULM %s Allocated count %d\n", facility, alloc_cnt);

	/* If it looks like there is a memory leak, hex dump some records */
	if (alloc_cnt < 10)
	    print_max = 0;
	else
	    print_max = PRINT_MAX;

	/* 
	** Output ULM memory 32 bytes per output row
	** TRformat requires aligned storage.
	** Lucky for us ULM always returns aligned memory
	** Make sure, just in case
	**
	** Dump the LAST PRINT_MAX blocks
	*/
	current = pool->ulm_pallist.ulm_hlast;
	print_cnt = 0;
	while ( current != (ULM_NODE *)NULL && print_cnt++ < print_max)
	{
	    TRdisplay("ULM %s Dumping Allocated 0x%p,0x%p Size %d\n", facility, 
		current, (ULM_NODE *)((char *) current + current->ulm_nsize),
		current->ulm_nsize);

	    /* 
	    ** Output ULM memory 32 bytes per output row
	    ** TRformat requires aligned storage.
	    ** Lucky for us ULM always returns aligned memory
	    ** Make sure, just in case
	    */
	    cp = ME_ALIGN_MACRO((char *)current, sizeof(PTR));
	    if ( cp == (char *)current)
	    {
		nsize = current->ulm_nsize;
		if (nsize > 128)
		    nsize = 128;  /* don't print more than 128 bytes */
		for (i = 0; i < nsize; i += 32)
		{
		    TRdisplay("\t[%4.1d]: %8.4{%x %}%2< >%32.1{%c%}<\n", 
			i, cp+i, 0);
		}
	    }

	    current = current->ulm_nprev;
	}

	/* Dump the 1st PRINT_MAX blocks */
	current = pool->ulm_pallist.ulm_h1st;
	print_cnt = 0;
	while ( current != (ULM_NODE *)NULL && print_cnt++ < print_max)
	{
	    TRdisplay("ULM %s Dumping Allocated 0x%p,0x%p Size %d\n", facility,
		current, (ULM_NODE *)((char *) current + current->ulm_nsize),
		current->ulm_nsize);

	    /* 
	    ** Output ULM memory 32 bytes per output row
	    ** TRformat requires aligned storage.
	    ** Lucky for us ULM always returns aligned memory
	    ** Make sure, just in case
	    */
	    cp = ME_ALIGN_MACRO((char *)current, sizeof(PTR));
	    if ( cp == (char *)current)
	    {
		nsize = current->ulm_nsize;
		if (nsize > 128)
		    nsize = 128;  /* don't print more than 128 bytes */
		for (i = 0; i < nsize; i += 32)
		{
		    TRdisplay("\t[%4.1d]: %8.4{%x %}%2< >%32.1{%c%}<\n", 
			i, cp+i, 0);
		}
	    }

	    current = current->ulm_nnext;
	}
    }

    return;
}


/*{
** Name: ex_handler	- ULM exception handler.
**
** Description:
**	Exception handler for ULM.
**
**	An error message including the exception type and PC will be written
**	to the log file.
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
**	EXDECLARE causes us to return execution to where the handler was 
**      defined.
**
** History:
**      28-feb-2003 (stial01)
**          Created.
*/
static STATUS
ex_handler(
EX_ARGS	    *ex_args)
{
    i4		err_code;

    TRdisplay("Exception in ULM\n");

    /*
    ** Returning with EXDECLARE will cause execution to return to the
    ** top of the place where the handler was declared.
    */
    return (EXDECLARE);
}

DB_STATUS
ulm_print_pool( register ULM_RCB *ulm_rcb)
{
    		ULM_PHEAD	*pool_ptr;
    register	ULM_CHUNK	*chunk, *prev_chunk;
    register 	ULM_STREAM	*stream;
    		i4		chunk_size_pag;
    		DB_STATUS	status = E_DB_OK;
		i4		pchunks;
		EX_CONTEXT      context;
		bool		trace = 1;
		char		*facility;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Validate pool and owner */
    if ((pool_ptr = (ULM_PHEAD*)ulm_rcb->ulm_poolid) == (ULM_PHEAD*)NULL ||
	pool_ptr->ulm_self != pool_ptr ||
	pool_ptr->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    /* First, wait to get exclusive access to the pool */
    /* ----------------------------------------------- */
    if (CSp_semaphore(SEM_EXCL, &pool_ptr->ulm_psem))
    {
	/* Return now, instead of attempting to do a v() */
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	return (E_DB_ERROR);
    }

    pchunks = pool_ptr->ulm_pchunks;

    if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
	ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
	facility = owner_names[ulm_rcb->ulm_facility];
    else
	facility = "UNKNOWN";
    TRdisplay("%@ ULM %s pool at 0x%p\n", facility, pool_ptr);
    TRdisplay("ULM %s Requested pool size:%6d pages\n", 
		facility, pool_ptr->ulm_psize_pag);
    TRdisplay("ULM %s Allocated size:%6d pages in %d chunks\n",
		facility, pool_ptr->ulm_pages_alloced, pchunks);

    if (trace)
    {
	if (EXdeclare(ex_handler, &context) == OK)
	{
	    ulm_printpool((PTR)pool_ptr, facility);
	}
	EXdelete();
    }

    /* Release exclusive access to the pool */
    /* ------------------------------------ */
    if (CSv_semaphore(&pool_ptr->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
	return (E_DB_ERROR);
    }

    return (status);
}


#ifdef xDEBUG
DB_STATUS
ulm_print_mem( register ULM_RCB *ulm_rcb, PTR mem, i4 memsize)
{
    		ULM_PHEAD	*pool_ptr;
    register	ULM_CHUNK	*chunk, *prev_chunk;
    register 	ULM_STREAM	*stream;
    		i4		chunk_size_pag;
    		DB_STATUS	status = E_DB_OK;
		i4		pchunks;
		EX_CONTEXT      context;
		bool		trace;
		char		*facility;
		bool		found = FALSE;
		i4		tot_chunk_size;
		i4		nsize;
		i4		i;
		char		*cp;

    CLRDBERR(&ulm_rcb->ulm_error);

    /* Validate pool and owner */
    if ((pool_ptr = (ULM_PHEAD*)ulm_rcb->ulm_poolid) == (ULM_PHEAD*)NULL ||
	pool_ptr->ulm_self != pool_ptr ||
	pool_ptr->ulm_powner != ulm_rcb->ulm_facility)
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000B_NOT_ALLOCATED);
	return (E_DB_ERROR);
    }

    /* First, wait to get exclusive access to the pool */
    /* ----------------------------------------------- */
    if (CSp_semaphore(SEM_EXCL, &pool_ptr->ulm_psem))
    {
	/* Return now, instead of attempting to do a v() */
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL0009_MEM_SEMWAIT);
	return (E_DB_ERROR);
    }

    pchunks = pool_ptr->ulm_pchunks;

    if (ulm_rcb->ulm_facility >= DB_MIN_FACILITY &&
	ulm_rcb->ulm_facility <= DB_MAX_FACILITY)
	facility = owner_names[ulm_rcb->ulm_facility];
    else
	facility = "UNKNOWN";

    if (trace)
    {
	if (EXdeclare(ex_handler, &context) == OK)
	{
	    chunk = pool_ptr->ulm_pch1st;
	    while (chunk != (ULM_CHUNK *) NULL && !found)
	    {
		tot_chunk_size = (i4)chunk->ulm_cpages * SCU_MPAGESIZE;
		if (mem >= (PTR)chunk 
				&& mem+memsize <= (PTR)chunk+tot_chunk_size)
		{
		    TRdisplay("ULM %s found %x in chunk %x... printing\n",
			facility, mem, chunk);

		    /* 
		    ** Output ULM memory 32 bytes per output row
		    ** TRformat requires aligned storage.
		    ** Lucky for us ULM always returns aligned memory
		    ** Make sure, just in case
		    */
		    cp = ME_ALIGN_MACRO((char *)mem, sizeof(PTR));
		    if ( cp == mem)
		    {
			nsize = memsize;
			for (i = 0; i < nsize; i += 32)
			{
			    TRdisplay("\t[%4.1d]: %8.4{%x %}%2< >%32.1{%c%}<\n", 
				i, cp+i, 0);
			}
		    }
		    else
			TRdisplay("ULM memory must be aligned to be printed\n");
		}
		chunk = chunk->ulm_cchnext;
	    }
	}
	EXdelete();
    }

    /* Release exclusive access to the pool */
    /* ------------------------------------ */
    if (CSv_semaphore(&pool_ptr->ulm_psem))
    {
	SETDBERR(&ulm_rcb->ulm_error, 0, E_UL000A_MEM_SEMRELEASE);
	return (E_DB_ERROR);
    }

    if (found)
	return (E_DB_OK);
    else
	return (E_DB_ERROR);
}
#endif

