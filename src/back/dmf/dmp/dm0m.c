/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <me.h>
#include    <di.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <dmftrace.h>

/**
**
**  Name: DM0M.C - DMF memory manager.
**
**  Description:
**      The module implements the dynamic memory manager for DMF.  It supports
**	allocation of variable length objects, verification of pool consistency,
**	protecting and unprotecting the pool.
**
**      The pool of memory managed by these routines is implemented as an
**	sequence of variable length objects that have the same type of header.
**	Each object has a type and a length field in a specific offset from
**	the beginning of the object.  Objects are used to represent free as
**	well as allocated objects.  Free objects are linked together in sorted
**	order by memory address.  Allocated objects are not linked onto the
**	free list and can be found by looking at the type of the object.
**	All allocation requests are rounded up to the to a multiple of
**	32 bytes currently.  The size stored in the object is the actual
**	allocation size.
**
**	The memory routines include function dm0m_destroy to deallocate the
**	memory pools back to SCF.
**
**	These routines do not assume that SCF will give us memory pools of
**	ever-increasing addresses.  Also took out assumptions that
**	new allocated memory will have addresses greater than the
**	pool and free list headers.
** 
**      The routines in this file are:
**	    dm0m_allocate - Allocate memory.
**	    dm0m_check - Check legality of identifier.
**			 only available if built with xDEBUG
**          dm0m_deallocate - Deallocate memory.
**	    dm0m_destroy - Destroy the memory manager database.
**	    dm0m_search - Search pool for objects,
**	    dm0m_verify - Verify the pool integrity.
**	    dm0m_info - Return information about allocated memory.
**
**  History:
**      07-oct-1985 (derek)    
**          Created new for Jupiter.
**	01-feb-1991 (mikem)
**	    Added include of <me.h> as ME routines are used by this file (also
**	    allows those routines to be changed to macro's).
**	25-feb-1991 (rogerk)
**	    Added dm0m_info routine.
**	    This was added to allow error handling routines to give better
**	    error information after memory allocate failures.
**	    This was added as part of the Archiver Stability project changes.
**	25-mar-1991 (bryanp)
**	    Fixed a bug in the roundup code in expand_pool and updated the
**	    comments to reflect the actual expand_pool behavior.
**	14-jun-1991 (Derek)
**	    Make expand pool allocate multiples of EXPAND_INCREMENT so
**	    that the pool verify code will still work.
**	09-oct-91 (jrb)
**	    Integrated 6.4 changes into 6.5.
**	20-jan-1992 (bryanp)
**	    Clean up error handling in expand_pool().
**	21-may-1992 (jnash)
**	    Add additional traceback errors in expand_pool() and
**	    dm0m_allocate().
**	07-jul-92 (jrb)
**	    Prototyped DMF (protos in DM0M.H).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      13-sep-93 (smc)
**          Removed truncating casts now obj_owner is a PTR.
**	07-oct-93 (johnst)
**	    Bug #56442
**	    Changed type of dm0m_search() argument "arg" from (i4) to
**	    (i4 *) to match the required usage by its callers from dmdcb.c.
**	    For 64-bit pointer platforms (axp_osf), cannot get away with 
**	    overloading 64-bit pointers into 32-bit i4s!
**      06-mar-1996 (stial01 for bryanp)
**	    Added some new routines in support of long pages and long tuples:
**		dm0m_tballoc, dm0m_tbdealloc -- allocate and free a tuple
**		    buffer, to avoid allocating tuple buffers on the stack.
**		dm0m_lcopy, like MEcopy, but doesn't have a u_i2 copy length.
**		dm0m_lfill, like MEfill, but doesn't have a u_i2 fill length.
**      06-mar-1996 (stial01)
**          dm0m_allocate() Implement best fit to reduce dmf memory 
**          fragmentation, particularly the fragmentation of large sort buffers.
**      28-Nov-96 (fanra01)
**          Modifed overlap check in dm0m_deallocate to allow for deallocation
**          of the first object in the free list.
**	27-Jan-1997 (jenjo02)
**	    o Don't create fragments which can never be allocated.
**   	    o MEfill only the storage requested by the caller, not the extra.
**      10-mar-1997 (stial01)
**          dm0m_tballoc() Added tuple_size parameter
**	21-may-1997 (shero03)
**	    Add additional checking by painting free memory
**	04-jun-1997 (hanch04)
**	    Fix comment that caused compile error
**	02-dec-1998 (nanpr01)
**	    Pass the mlock flag for private cache.
**	06-jan-1999 (nanpr01)
**	    Donot squish the memory if the size goes beyond MAXI4. This
**	    needs to be revisited for 64-bit port.
**	08-jan-1999 (nanpr01)
**	    Pad memory with character to catch memory overruns.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Oct-2002 (hanch04)
**	    Changed MAXI4 to MAX_SIZE for 64-bit memory.
**	    Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**	01-Nov-2002 (hweho01)
**	    Renamed MAX_SIZE to MAX_SIZE_TYPE, avoid conflicts with
**          the existing definitions in source files of fe/rep/repmgr.
**	5-Mar-2004 (schka24)
**	    Fix up memory debug stuff a little.
**      18-feb-2004 (stial01)
**          Redefine tuple_size for dm0m_tballoc.
**      30-mar-2005 (stial01)
**          Added support for svcb_pad_bytes (dbms config param dmf_pad_bytes)
**      25-Jun-2005 (horda03)
**          Supplement to "03-may-2005 (hayke02)" change. ON VMS di.h
**          is not included by jf.h, so added it and tr.h to the 
**          #includes above.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	    Also back out subsequent change 478045.
**	15-Aug-2005 (jenjo02)
**	    Redesigned to support multiple LongTerm (Server) pool
**	    lists, and ShortTerm (Session) pool lists.
**	16-Jun-2006 (kschendel)
**	    Fix parameter size for 9425 error so it logs properly.
**	16-Apr-2008 (kschendel)
**	    Needs adf.h to compile; fix a couple %x to %p in trdisplays.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *, use
**	    new form uleFormat.
*/

/*
**  Constants.
*/

#define			EXPAND_INCREMENT    512 * 1024
#define			PAD_BYTE	    64
#define			PAD_CHAR	    0x7F

/* Computed once by dm0m_object_align, the MinimalFragment size */
static i4  MinFrag = 0;

static DB_STATUS expand_pool(
	DML_SCB		*scb,
	i4		MyPool,
	DM_OBJECT	**ret_obj,
	SIZE_TYPE	size,
	i4		flag,
	DB_ERROR	*dberr);

static i4  dm0m_object_align();

static VOID dm0m_fmt_cb(
	i4             *flag_ptr,
	DM_OBJECT      *obj);

static VOID dm0m_pad_error(
	DM_OBJECT 	*obj, 
	char 		*padchar);

static VOID dm0m_search_pools(
    	DM_OBJECT	*pool,
    	i4		type,
    	VOID		(*function)(),
    	i4		*arg);

static VOID dm0m_pool_info(
	DM_OBJECT	*pool,
	SIZE_TYPE	*total_memory,
	SIZE_TYPE	*allocated_memory,
	SIZE_TYPE	*free_memory);


/*{
** Name: dm0m_allocate	- Allocate memory.
**
** Description:
**      Allocate memory from the dynamic pool maintained by DMF.
**	The allocation is rounded to a multiple of the first power of 2
**	greater than sizeof(DM_OBJECT), and a search of the free list
**      is started. It was observed that if we take the first object
**      that fits that we will unecessarily fragment large pieces of
**      memory that were allocated for sorting. Since memory requests  
**      can be small or very large, we need to do some kind of best fit
**      algorithm when allocating memory.
**	Optional checks to validate the memory pool are compiled in
**	if xDEBUG is used.
**
** Inputs:
**      size                            The number of bytes to allocate.
**	type				The type of object to allocate,
**					taken from defines in dm.h
**	flag				DM0M_ZERO to zero allocated
**					memory.
**					DM0M_SHORTTERM to explictly
**					request ShortTerm Session
**					memory.
**					DM0M_LONGTERM to explictly
**					request LongTerm Server
**					memory.
**	tag				Tag to store in control block.
**	owner				The owner to store in the control block.
**
** Outputs:
**      block                           Address to return allocated block.
**	err_code			Error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-oct-1985 (derek)
**          Created new for Jupiter.
**      21-may-1992 (jnash)
**          Add additional traceback errors.
**	02-nov-93 (swm)
**	    Bug #56450
**	    Initialisation of adjusted_size offset assumed that
**	    sizeof(DM_OBJECT) was a power of 2 in its masking operation,
**	    and function comment assumed this value to be 32 (which is what
**	    it happens to be on sun4). Changed initialisation to calculate
**	    first power of 2 greater than or equal to sizeof(DM_OBJECT),
**	    so that masking will work.
**	05-nov-1993 (smc)
**	    Bug #58635
**          Changed initiation of unused obj_maxaddr (this is now obj_unused).
**	06-mar-1996 (stial01 for bryanp)
**	    Use dm0m_lfill rather than MEfill to clear >64K memory objects.
**      06-mar-1996 (stial01)
**          dm0m_allocate() Implement best fit to reduce dmf memory
**          fragmentation, particularly the fragmentation of large sort buffers.
**	27-Jan-1997 (jenjo02)
**	    o Don't create fragments which can never be allocated.
**   	    o MEfill only the storage requested by the caller, not the extra.
**	02-dec-1998 (nanpr01)
**	    Pass the mlock flag for private cache.
**	15-Aug-2005 (jenjo02)
**	    Added support for ShortTerm/LongTerm memory,
**	    compute MinFrag (MinimalFragment) as 
**	    dm_obj_align**2.
**	24-Sep-2008 (smeke01) b120941
**	    Release the mutex AFTER updating the object, especially the
**	    obj_size as this is used by other functions (under mutex
**	    protection) to traverse the pool.
**      15-Jan-2010 (hanal04) Bug 114184
**          Correct dmf_pad_bytes implementation. The pad was hard coded to
**          PAD_BYTE (64) instead of using the value svcb_pad_bytes. 
**          Make sure size + svcb_pad_bytes gets the correct alignment
**          adjustment.
*/

DB_STATUS
dm0m_allocate(
    SIZE_TYPE	    size,
    i4		    flag,
    i4		    TypeWithClass,
    i4		    tag,
    char	    *owner,
    DM_OBJECT	    **block,
    DB_ERROR	    *dberr)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DM_OBJECT	    *next;
    DM_OBJECT	    *pool;
    SIZE_TYPE	    dm_obj_align = dm0m_object_align();
    SIZE_TYPE	    adjusted_size;
    DM_OBJECT	    *obj;
    DM_OBJECT       *best_obj = 0;
    DB_STATUS	    status;
    i4	    	    loc_err_code;
    CS_SID	    sid;
    DML_SCB	    *scb;
    i4		    MyPool;
    i4		    class;
    i4		    type;

    CLRDBERR(dberr);
    
    /* svcb->svcb_pad_bytes will be zero unless dmf_pad_bytes is set */
    adjusted_size = (size + svcb->svcb_pad_bytes + (dm_obj_align - 1)) & 
                                ~(dm_obj_align - 1);

    /*  Reject bad allocation requests. */

    if (adjusted_size == 0)
    {
#ifdef xDEBUG
	SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	return (E_DB_ERROR);
#endif
	adjusted_size = dm_obj_align;
    }

    /*
    ** If SINGLERUSER or Ingres threads, only use
    ** LONGTERM memory, as SHORTTERM is there to
    ** assist concurrency in multithreaded servers.
    **
    ** If overriding class present in "flag", use it,
    ** otherwise use default class of requested "type",
    ** forcing LONGTERM if none.
    */
    if ( svcb->svcb_status & SVCB_SINGLEUSER ||
       !(svcb->svcb_status & SVCB_IS_MT) ||
        (!(class = flag & (DM0M_SHORTTERM | DM0M_LONGTERM)) &&
	 !(class = TypeWithClass & (DM0M_SHORTTERM | DM0M_LONGTERM))) )
    {
	/* This really ought to return an error, as should
	** a bad "type" code.
	*/
	class = DM0M_LONGTERM;
    }
	     
    /* 
    ** ShortTerm memory is allocated on the Session's SCB
    ** pool list, no mutex required.
    */
    if ( class & DM0M_SHORTTERM )
    {
	/*
	** In single-user or pre-session startup, a DML_SCB
	** won't be returned, dropping us into LONGTERM.
	** Ditto if a thread started without the DMF facility
	** yet is making DMF allocate calls here...
	** 
	*/
	CSget_sid(&sid);
	if ( scb = GET_DML_SCB(sid) )
	{
	    obj = &scb->scb_free_list;
	    pool = &scb->scb_pool_list;
	    MyPool = 0;
	}
	else
	    class = DM0M_LONGTERM;
    }

    /*
    ** LongTerm memory is allocated on one of the Server's
    ** pool lists, mutex required.
    */
    if ( class & DM0M_LONGTERM )
    {
	/* Pick a pool list, simple round-robin */
	while ( (MyPool = svcb->svcb_next_pool++) >= svcb->svcb_pools )
	    svcb->svcb_next_pool = 0;
	obj = &svcb->svcb_free_list[MyPool];
	pool = &svcb->svcb_pool_list[MyPool];
	scb = (DML_SCB*)NULL;

	/*  Mutex this pool list */
	dm0s_mlock(&svcb->svcb_mem_mutex[MyPool]);
    }

    /* DM105 stats collection turned on? */
    if ( svcb->svcb_dm0m_stat[0].tag )
    {
	/* Isolate "type" as int */
	type = TypeWithClass & ~(DM0M_SHORTTERM | DM0M_LONGTERM);
	
	if ( type <= 0 || type > DM0M_MAX_TYPE )
	{
	    TRdisplay("%@ dm0m_allocate: Invalid type %d\n", type);
	    CS_breakpoint();
	}
	else
	{
	    DM0M_STAT	*Type = &svcb->svcb_dm0m_stat[type];
	    Type->tag = tag;
	    if ( class == DM0M_SHORTTERM )
		Type->shortterm++;
	    else
		Type->longterm++;
	    if ( ++Type->alloc_now > Type->alloc_hwm )
		Type->alloc_hwm = Type->alloc_now;
	    if ( adjusted_size > Type->max )
		Type->max = adjusted_size;
	    if ( adjusted_size < Type->min )
		Type->min = adjusted_size;
	}
    }

    /*
    **  Search the list of free object for the first free object 
    **  that has enough space to satisfy the request.
    */

    for (;;)
    {
	if (obj->obj_size < adjusted_size)
	{
	    if ( obj = obj->obj_next )
		continue;
	    if ( obj = best_obj )
		break;
	}
	else
	{
	    /* Stop if exact fit */
	    if (obj->obj_size == adjusted_size)
		break;

	    /*
	    ** Best fit compromise so we don't always do exhaustive search
	    */
	    if (obj->obj_size <= adjusted_size + MinFrag)
		break;

	    if (!best_obj || (obj->obj_size < best_obj->obj_size))
		best_obj = obj;

	    if ( obj = obj->obj_next )
		continue;

	    obj = best_obj;
	    break;
	}


	/* On return obj will point to newly allocated object */

	status = expand_pool(scb, MyPool, &obj, adjusted_size, (flag & (DM0M_LOCKED)),
				dberr);
	if ( status )
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &loc_err_code, 0);
	    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	    if ( class & DM0M_LONGTERM )
		dm0s_munlock(&svcb->svcb_mem_mutex[MyPool]);
	    return (status);
	}
    }

    /*  See if this was an exact fit or not. */

    /*
    ** If what's left is less than a minimal fragment
    ** take the whole object instead of creating a useless
    ** fragment (and lengthening the free queue).
    */
    if ( (obj->obj_size - adjusted_size) < MinFrag )
    {
	adjusted_size = obj->obj_size;

	/*  Unlink this block from the free queue. */

	if (obj->obj_prev->obj_next = obj->obj_next)
	    obj->obj_next->obj_prev = obj->obj_prev;
    }
    else
    {
	/* Fragment this block leaving tail on free queue. */
	next = (DM_OBJECT *)((char *)obj + adjusted_size);
	obj->obj_prev->obj_next = next;
	if (next->obj_next = obj->obj_next)
	    next->obj_next->obj_prev = next;
	next->obj_prev = obj->obj_prev;
	next->obj_size = obj->obj_size - adjusted_size;
	next->obj_type = OBJ_FREE;
	next->obj_tag = OBJ_T_FREE;
    }

    if ( scb )
    {
	/* Incr memory now allocated, track hwm */
	scb->scb_mem_alloc += adjusted_size;
	if ( scb->scb_mem_alloc > scb->scb_mem_hwm )
	    scb->scb_mem_hwm = scb->scb_mem_alloc;
	svcb->svcb_stat.ses_mem_alloc++;
    }
    else
    {
	svcb->svcb_stat.mem_alloc[MyPool]++;
    }

    /*  Set return values and return. */

#ifdef xDEBUG
    TRdisplay("dm0m_allocate: Alloc %d (%d) bytes %.4s/%x (%p)\n    %p .. %p\n",
		size, adjusted_size, (char *)&tag, tag, owner,
		obj, (char *)obj + adjusted_size - 1);
#endif

    /*
    **  Initialize the allocated object.  Zero the whole object if
    **  caller desired.  Store the type and the length.
    */

    obj->obj_size = adjusted_size;
    obj->obj_type = (u_i2)TypeWithClass;
    obj->obj_tag = (i4)tag;
    obj->obj_owner = owner;

    obj->obj_next = 0;
    obj->obj_prev = 0;
    obj->obj_pool = MyPool;
    obj->obj_scb = (PTR)scb;
    obj->obj_pad2 = 0;

    /*
    ** Release the mutex AFTER updating the object, especially the 
    ** obj_size as this is used by other functions (under mutex 
    ** protection) to traverse the pool. (b120941).
    */
    if ( !scb )
    {
	/* Release the pool mutex */
	dm0s_munlock(&svcb->svcb_mem_mutex[MyPool]);
    }

#ifdef xDEBUG
    dm0m_verify(DM0M_POOL_CHECK);
#endif

    *block = obj;

    if ((flag & DM0M_ZERO))
    {
        /* Zero the caller's portion of the object . */ 
        MEfill(size - sizeof(DM_OBJECT), 0, (char *)&obj[1]);
    }
    else if (DMZ_MEM_MACRO(3))
    {
	/*
	** SET TRACEPOINT DM103 makes sure dmf is 
	** really initializing the memory before using it
	*/
	MEfill(size - sizeof(DM_OBJECT), 0xFE, (char *)&obj[1]);
    }

    if (svcb->svcb_pad_bytes)
    {
	MEfill(obj->obj_size - size, PAD_CHAR, (PTR)((char *) obj) + size);
	*(i4 *)(((char *)obj) + adjusted_size - sizeof(i4)) = size;
    }

    return (E_DB_OK);
}

/*{
** Name: expand_pool	- Expand the size of the DMF memory pool.
**
** Description:
**      More memory is request.  The request memory is coalesed into
**	the pool list or is linked onto the pool list depending on
**	whether it was contiguous with another pool.
**
**	A new free object is placed on the free list.  If the new free object
**	is contiguous with another object, the two are coelesed into one.
**
**	The newly created/expanded free object is returned in the 'obj'
**	parameter.
**    
**	This routine does not assume that the newly allocated
**	memory pool could always be put at the end of the pool list and
**	the newly created free object at the end of the free list.
**	It also does not assume that allocated memory addresses will be
**	greater than the list headers.
**
**	If there are absolutely no free objects in the current pool (i.e., this
**	is the first time we've called expand_pool or every free pool object
**	is in use -- the latter is highly unlikely), then we adjust the new
**	pool object's size upward by EXPAND_INCREMENT (256K) in addition to
**	the object size requested.
**
**	We then round the new pool element's size up to the next
**	EXPAND_INCREMENT boundary, with the effect that the first pool object
**	allocated is always at least 2 * EXPAND_INCREMENT in size and all
**	subsequent pool objects are at least EXPAND_INCREMENT in size, and all
**	pool objects are sized in multiples of EXPAND_INCREMENT.
**	
** Inputs:
**      size                            The number of bytes to add to the pool.
**
** Outputs:
**      object                          New memory object with sufficient size.
**	err_code			Error code.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-oct-1985 (derek)
**          Created new for Jupiter.
**	    SCF to allocate memory.
**	25-mar-1991 (bryanp)
**	    Updated comments and fixed bug in roundup code.
**	20-jan-1992 (bryanp)
**	    Clean up error handling in expand_pool(). Added err_code argument
**	    and changed code to set err_code, then return E_DB_ERROR, instead
**	    of trying to return an error code in the status field.
**      21-may-1992 (jnash)
**          Add additional traceback errors.
**	02-dec-1998 (nanpr01)
**	    Pass the mlock flag for private cache.
**	15-Aug-2005 (jenjo02)
**	    Support ShortTerm Session-owned pools.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Track total DMF memory allocation for stats.
**	15-Sep-2010 (maspa05) b124388
**	    Make sure obj_pool is initialised.
*/
static DB_STATUS
expand_pool(
    DML_SCB	*scb,
    i4		MyPool,
    DM_OBJECT	**ret_obj,
    SIZE_TYPE	size,
    i4		flag,
    DB_ERROR	*dberr)
{
    DM_SVCB	        *svcb = dmf_svcb;
    DM_OBJECT		*pool, *pool_list;
    DM_OBJECT		*obj, *free_list;
    DM_OBJECT		*new_pool;
    DM_OBJECT		*new_obj;
    PTR			low_address;
    SIZE_TYPE		adjusted_size;
    DB_STATUS		status;
    STATUS		mestatus;
    SCF_CB		scf_cb;
    i4			loc_err_code;
    SIZE_TYPE	 	dm_obj_align = dm0m_object_align();
    i4			expand_pages;
    i4			hint_pages;
    i4			page_bucket;

    CLRDBERR(dberr);

    /* Nothing allocated, yet */
    scf_cb.scf_scm.scm_addr = NULL;

    /* Expanding Session ShortTerm memory? */
    if ( scb )
    {
	pool = pool_list = &scb->scb_pool_list;
	obj = free_list = &scb->scb_free_list;

	/* Compute requested size in units of DM0M_PageSize */
	adjusted_size = (size + dm_obj_align + (DM0M_PageSize - 1)) &
			~(DM0M_PageSize - 1);
	expand_pages = adjusted_size / DM0M_PageSize;
	
	/* If first pool of session, check the hint */
	if ( scb->scb_mem_pools++ == 0 )
	{
	    /* Extract hint for session type */
	    hint_pages = svcb->svcb_st_hint
			[(scb->scb_s_type & SCB_S_FACTOTUM)
				? DM0M_Factotum
				: DM0M_User]
			[DM0M_Hint];

	    /* Allocate the hint, if larger */
	    if ( hint_pages > expand_pages )
	    {
		expand_pages = hint_pages;
		adjusted_size = hint_pages * DM0M_PageSize;
	    }
	}

	/* If MEreqmem fails, fall through to give SCF a try */
	scf_cb.scf_scm.scm_out_pages = adjusted_size / SCU_MPAGESIZE;
	scf_cb.scf_scm.scm_addr = 
		MEreqmem(0, adjusted_size, FALSE, &mestatus);

	/* If MEreqmem succeeded mark the new pool as having been 
	 * allocated via MEreqmem */
        if ( scf_cb.scf_scm.scm_addr )
	   ((DM_OBJECT *) scf_cb.scf_scm.scm_addr)->obj_pool = OBJ_P_MEREQ; 
    }
    else
    {
	/* LongTerm Server memory, use the designated pool list */
	pool = pool_list = &svcb->svcb_pool_list[MyPool];
	obj = free_list = &svcb->svcb_free_list[MyPool];
	if (pool->obj_next == 0)
	    size += EXPAND_INCREMENT;
	adjusted_size = (size + dm_obj_align + (EXPAND_INCREMENT - 1)) &
			~(EXPAND_INCREMENT - 1);
#if (EXPAND_INCREMENT % SCU_MPAGESIZE) != 0
	/*
	** Must additionally round up to the SCF pagesize to allocate in SCF pages:
	*/
	adjusted_size = (adjusted_size + (SCU_MPAGESIZE - 1)) & ~(SCU_MPAGESIZE -1);
#endif
    }

    /*
    ** Get memory from SCF if LongTerm pool or if
    ** Session MEreqmem failed (mestatus != OK)
    */
    if ( scf_cb.scf_scm.scm_addr == NULL )
    {
	/*	Call SCF to allocate memory. */

	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_scm.scm_functions = flag;
	scf_cb.scf_scm.scm_in_pages = adjusted_size / SCU_MPAGESIZE;
	status = scf_call(SCU_MALLOC, &scf_cb);
	if (status != OK)
	{
	    if (scf_cb.scf_error.err_code != E_SC0005_LESS_THAN_REQUESTED)
	    {
		uleFormat(&scf_cb.scf_error, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &loc_err_code, 0);
		uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, 0, ULE_LOG, NULL, 
		    NULL, 0, NULL, 
		    &loc_err_code, 1, sizeof(adjusted_size), &adjusted_size);
		SETDBERR(dberr, 0, E_DM9429_EXPAND_POOL);
		return (E_DB_ERROR);
	    }
	}

	/* Display LT expansion */
	if ( !scb && svcb->svcb_stat.mem_expand[MyPool]++ )
	    TRdisplay("%@ DMF Expand(%d,%d) %d LT page pool at 0x%p (%d)\n",
		    MyPool, svcb->svcb_stat.mem_expand[MyPool],
		    scf_cb.scf_scm.scm_out_pages,
		    scf_cb.scf_scm.scm_addr,
		    svcb->svcb_stat.mem_alloc[MyPool]);
		
    }

    /*	Calculate the size of region that was returned. */

    adjusted_size = scf_cb.scf_scm.scm_out_pages * SCU_MPAGESIZE;
    low_address = (PTR) scf_cb.scf_scm.scm_addr;
    new_pool = (DM_OBJECT *) low_address;
    svcb->svcb_stat.mem_totalloc += adjusted_size;

    /*
    ** Find spot in memory pool list to insert the new pool.  Search down
    ** list till find pool with address greater than the newly allocated
    ** one.
    */

    while ( pool->obj_next && pool->obj_next < new_pool )
	pool = pool->obj_next;

    /*
    ** If we have pool immediately preceeding the new pool.  
    ** If the new pool is contiguous with this one, squish 
    ** them into one pool.
    **
    ** Note there is no squishing of SCB pools, as they are
    ** individually freed when the session terminates.
    */
    if ( !scb &&
        (pool != pool_list) &&
	(((char *)pool + pool->obj_size) == low_address) &&
	((pool->obj_size + adjusted_size) <= MAX_SIZE_TYPE) &&
	((pool->obj_size + adjusted_size) > 0))
    {
    	pool->obj_size += adjusted_size;
	new_obj = (DM_OBJECT *) low_address;
    }
    else
    {
	/*
	** Add new memory pool to front of pool list.
	** If new pool is added at end of list, point the list header's
	** previous pointer to it.
	*/
	new_pool->obj_next = pool->obj_next;
	new_pool->obj_size = adjusted_size;
	new_pool->obj_type = OBJ_POOL;
	new_pool->obj_tag = OBJ_T_POOL;
	new_pool->obj_scb = (PTR)scb;
	/* If SCB pool, note if mem came from SCF, not MEreqmem */
	if ( scb && mestatus )
	    new_pool->obj_pool = OBJ_P_SCF;

	pool->obj_next = new_pool;
	if ((pool_list->obj_prev == (DM_OBJECT *)NULL) ||
	    (pool_list->obj_prev == pool))
	{
	    pool_list->obj_prev = new_pool;
	}
	adjusted_size -= dm_obj_align;
	new_obj = (DM_OBJECT *)((char *)low_address + dm_obj_align);
    }

    /*
    ** Look for spot on the free list to insert the new free object.
    ** Search down the free list till find an object with address greater
    ** than the newly allocated one.
    */

    while ( obj->obj_next && obj->obj_next < new_obj )
	obj = obj->obj_next;

    /*
    ** We have free object immediately preceeding the new obj.  If the new
    ** object is contiguous with this one, squish them into one free object.
    */
    if ((obj != free_list) &&
	((DM_OBJECT *)((char *)obj + obj->obj_size) == new_obj) &&
	((obj->obj_size + adjusted_size) <= MAX_SIZE_TYPE) &&
	((obj->obj_size + adjusted_size) > 0))
    {
    	obj->obj_size += adjusted_size;
	*ret_obj = obj;
    }
    else
    {
	/*
	** Add new free object into free list.
	*/

	new_obj->obj_next = obj->obj_next;
	new_obj->obj_prev = obj;
	new_obj->obj_size = adjusted_size;
	new_obj->obj_type = OBJ_FREE;
	new_obj->obj_tag = OBJ_T_FREE;
	*ret_obj = new_obj;
    }
    
    return (E_DB_OK);
}

/*{
** Name: dm0m_check	- Check object address.
**
** Description:
**      Check that the address given is within range and that the
**	type of control block matchs.
**
**	This routine is replaced by a macro, defined in dm.h, unless
**	the system is built with xDEBUG defined.
**
** Inputs:
**      object                          The address of the object to check.
**	type_id				The type of control block to check for.
**
** Outputs:
**	Returns:
**	    E_DB_OK			Address and type are OK.
**	    E_DB_ERROR			Address and/or type are invlid.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory database is not locked.
**
** History:
**	10-mar-1986 (derek)
**          Created for Jupiter.
**	1-oct-93 (robf)
**          Add check for object being NULL
**	15-Aug-2005 (jenjo02)
**	    Object may belong to either SCB pool or
**	    LongTerm server pool.
*/

# ifdef    xDEBUG

DB_STATUS
dm0m_check(
    DM_OBJECT	*obj,
    i4	TypeWithClass)
{
    DM_OBJECT		*pool;
    DML_SCB		*scb;
    i4	 		dm_obj_align = dm0m_object_align();

    if ( obj )
    {
	if ( scb = (DML_SCB*)obj->obj_scb )
	    pool = &scb->scb_pool_list;
	else
	    pool = &dmf_svcb->svcb_pool_list[obj->obj_pool];
	if (pool = pool->obj_next)
	{
	    /* See that object fits in pool and type matches */
	    if (obj >= (DM_OBJECT *)((char *)pool + dm_obj_align) &&
		    (DM_OBJECT *)((char *)obj + dm_obj_align)
                    <= (DM_OBJECT*)((char *)pool + pool->obj_size))
	    {
		if (obj->obj_type == TypeWithClass)
		    return (E_DB_OK);
	    }
	}
    }
    return (E_DB_ERROR);
}

# endif	    /* xDEBUG */


/*{
** Name: dm0m_deallocate	- Deallocate dynamic memory.
**
** Description:
**      Deallocate memory that was allocated from a previous call
**	to dm0m_allocate.  The free list is searched to find the position
**	in the free list that list block should be inserted in.  Adjacent
**	free block are coalesed into a larger free block.  If this is
**	compiled with xDEBUG extra checks are performed.
**	This routine does not assume that objects on the free list have memory
**	addresses greater than the address of the free list head.
**
** Inputs:
**      object                          The object to free.
**
** Outputs:
**      err_code                        Error code for operation.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-oct-1985 (derek)
**          Created new for Jupiter.
**	02-nov-93 (swm)
**	    Bug #56450
**	    Fixed xDEBUG sanity check that an object is aligned on a
**	    correct boundary. Existing code assumed that sizeof(DM_OBJECT)
**	    was a power of 2 in its masking operation, which happens to be
**	    to be true on sun4. Changed code to calculate an alignment
**	    check mask of the first power of 2 greater than or equal to
**	    sizeof(DM_OBJECT), so that masking will work.
**      28-Nov-96 (fanra01)
**          Add check for next to be unchanged from the address of the
**          svcb_free_list.  Overlap check does not allow for a deallocated
**          object being the first element in the free list.
**	21-may-1997 (shero03)
**	    Paint free memory with x'FE'
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer to dm0m_deallocate.
**	15-Aug-2005 (jenjo02)
**	    Object may be allocated on either ST SCB pool
**	    or LT pool; figure out which.
*/
VOID
dm0m_deallocate(DM_OBJECT **object)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DM_OBJECT	    *obj = *object;
    DM_OBJECT	    *next, *pool, *free_list;
    i4	 	    dm_obj_align = dm0m_object_align();
    i4		    type;
    DML_SCB	    *scb;
    i4		    MyPool;
    i4	    	    i, orig_len;
    char	    *padchar;

    /* Sanity check */
    if ( !obj )
	return;

#ifdef xDEBUG
    TRdisplay("dm0m_deallocate: Freeing %d bytes %.4s/%x (%p)\n    %p .. %p\n",
	    obj->obj_size, (char *)&obj->obj_tag, obj->obj_tag, obj->obj_owner,
	    obj, (char *)obj + obj->obj_size - 1);
    dm0m_verify(DM0M_POOL_CHECK);
#endif

    /* Session ShortTerm object? */
    if ( scb = (DML_SCB*)obj->obj_scb )
    {
	pool = &scb->scb_pool_list;
	next = free_list = &scb->scb_free_list;
	/* Reduce memory now allocated */
	scb->scb_mem_alloc -= obj->obj_size;
	/* No mutex required */
    }
    else
    {
	/* Extract LT pool from which object was allocated */
	MyPool = obj->obj_pool;
	pool = &svcb->svcb_pool_list[MyPool];
	next = free_list = &svcb->svcb_free_list[MyPool];

	/* Mutex this pool */
	dm0s_mlock(&svcb->svcb_mem_mutex[MyPool]);
	svcb->svcb_stat.mem_dealloc[MyPool]++;
    }

    /* Collecting DM105 stats? */
    if ( svcb->svcb_dm0m_stat[0].tag )
    {
	/* Count one less of these allocated now */
	type = obj->obj_type & ~(DM0M_SHORTTERM | DM0M_LONGTERM);
	if ( type > 0 && type <= DM0M_MAX_TYPE &&
		svcb->svcb_dm0m_stat[type].alloc_now-- == 0 )
	    svcb->svcb_dm0m_stat[type].alloc_now = 0;
	/*
	** Don't let "alloc_now" go negative. This can happen
	** because after the trace point is set, dm0m_deallocate()
	** may be done on objects that were allocated 
	** before the tracepoint came on.
	*/
    }


#ifdef xDEBUG
    {
	/*  Perform some sanity checks on the object.	*/

	/*  Search for the pool that this object was allocated in. */

	while (pool = pool->obj_next)
	    if (obj >= (DM_OBJECT *)((char *)pool + dm_obj_align) &&
		((char *)obj + obj->obj_size) <=
		((char *)pool + pool->obj_size))
		break;

	/*
	**  If this objects address wasn't within the range of an pool
	**	or
	**  the address was not a multiple of the alignment
	**	or
	**  the length is zero
	**	or
	**  the length is not a multiple of the alignment
	**	or
	**  the type is out of range.
	**
	**  Reject the deallocation request.
	*/

	if (pool == 0 ||
	    (int)obj & (dm_obj_align -1) ||
	    obj->obj_size == 0 ||
	    obj->obj_size & (dm_obj_align - 1))
	{
	    dmd_check(E_DMF008_DM0M_BAD_OBJECT);
	}
    }
#endif

    /*
    **  Search the free list looking for the spot to insert this object.
    **	The free list is sorted by address.
    */

    while ( next->obj_next && next->obj_next < obj )
	next = next->obj_next;

    if (svcb->svcb_pad_bytes)
    {
	orig_len = *(i4 *) (((char *)obj) + obj->obj_size - sizeof(i4));
	padchar = ((char *) obj) + orig_len;

	/* consistency check for pads */
	for (i = 0; i < obj->obj_size - orig_len - sizeof(i4); i++, padchar++)
	{
	    if (*padchar != PAD_CHAR)
	    {
		dm0m_pad_error(obj, padchar);
		break; /* generate pad error once */
	    }
	}
    }

    if (DMZ_MEM_MACRO(4))
    {
	/*
	** SET TRACEPOINT DM104 makes sure the caller is not 
	** referencing this memory after it is freed
	** USE THIS DIAGNOSTIC ONLY IN TEST ENVIRONMENTS
	*/
	MEfill(obj->obj_size - sizeof(DM_OBJECT), 0xFE,
	      (PTR)((char *) obj) + sizeof(DM_OBJECT));
    }

    /*
    **  If this object overlaps with the free object before this object,
    **  then either the object is bad, the object was deallocated twice,
    **	or the free list is bad.  Mark pool as invalid and return error.
    */
    if ( (next != free_list) &&
         ((DM_OBJECT *)((char *)next + next->obj_size) > obj))
    {
	dmd_check(E_DMF009_DM0M_FREE_OVERLAP_PREV);
    }

    /*	Set this object to point the next object. */

    if (obj->obj_next = next->obj_next)
    {
	/*  There was a next object, so update it's previous pointer. */

    	obj->obj_next->obj_prev = obj;

	/*  See the object overlaps with the next free object. */

    	if ((DM_OBJECT *)((char *)obj+ obj->obj_size) >
	    next->obj_next)
	{
	    dmd_check(E_DMF00A_DM0M_FREE_OVERLAP_NEXT);
	}
    }

    /*	Fixup the rest of the links. */

    next->obj_next = obj;
    obj->obj_prev = next;
    obj->obj_type = OBJ_FREE;
    obj->obj_tag = OBJ_T_FREE;

    /*  Coalese object with previous if contiguous. */

    if (((DM_OBJECT *)((char *)next + next->obj_size) == obj) &&
	((obj->obj_size + next->obj_size) <= MAX_SIZE_TYPE) &&
	((obj->obj_size + next->obj_size) > 0))
    {
	next->obj_size += obj->obj_size;
	if (next->obj_next = obj->obj_next)
		next->obj_next->obj_prev = next;
	obj= next;
    }

    /*  Coalese object with next if contiguous. */

    if ((DM_OBJECT *)((char *)obj+ obj->obj_size) == obj->obj_next)
    {
    	next = obj->obj_next;

	if (((obj->obj_size + next->obj_size) <= MAX_SIZE_TYPE) &&
	    ((obj->obj_size + next->obj_size) > 0))
	{
	    obj->obj_size += next->obj_size;
	    if (obj->obj_next = next->obj_next)
	        obj->obj_next->obj_prev = obj;
	}
    }

    *object = (DM_OBJECT *) NULL;

    /* Release LT pool mutex */

    if ( !scb )
	dm0s_munlock(&svcb->svcb_mem_mutex[MyPool]);

    return;
}

/*{
** Name: dm0m_destroy	- Destroy Session/Server DMF memory
**
** Description:
**	This function checks each designated pool for 
**	unfreed objects, then destroys and frees the pool.
**
** Inputs:
**	scb			DML_SCB if destroying Session
**				ShortTerm pools, NULL otherwise.
**
** Outputs:
**	Returns:
**	    E_DB_OK			Address and type are OK.
**	    E_DB_ERROR			Address and/or type are invlid.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory manager database is destroyed.
**
** History:
**	10-mar-1986 (derek)
**          Created for Jupiter.
**      08-oct-1998 (stial01)
**          dm0m_destroy() TRdisplay unfreed DMF memory
**	15-Aug-2005 (jenjo02)
**	    Augmented to handle ShortTerm SCB memory,
**	    multiple LT pools.
*/
DB_STATUS
dm0m_destroy(
DML_SCB *scb, 
DB_ERROR *dberr)
{
    DM_SVCB	        *svcb = dmf_svcb;
    DM_OBJECT		*pool;
    DM_OBJECT		*delete_pool;
    DB_STATUS		status;
    SCF_CB		scf_cb;
    i4             	flag = DM0M_READONLY;
    SIZE_TYPE		total;
    SIZE_TYPE		allocated;
    SIZE_TYPE		free;
    i4			used_pages;
    i4			MyPool;
    i4			SCBType;
    i4 			i, j, k, type;
    CS_SEM_STATS 	sstat;
    u_i4		sem_requests;
    u_i4		sem_collisions;
    u_i4		alloc, dealloc, expand;
    SIZE_TYPE		pools_total, pools_alloc, pools_free;		

    CLRDBERR(dberr);

    if ( scb )
    {
	/* Destroy/free Session's pool(s), if used */
	if ( scb->scb_mem_pools )
	{
	    SCBType = (scb->scb_s_type & SCB_S_FACTOTUM) 
			? DM0M_Factotum : DM0M_User;

	    /* Compute hwm as number of pages */
	    used_pages = (scb->scb_mem_hwm + DM0M_PageSize -1)
			& ~(DM0M_PageSize-1);
	    used_pages /= DM0M_PageSize;
	    /* ... watch for too many */
	    if ( used_pages > DM0M_MaxPages )
		used_pages = DM0M_MaxPages;
	    /*
	    ** Update the number of SCBs using this number of
	    ** pages. If it's more than the old Initial
	    ** allocation hint, then this becomes the new
	    ** Initial hint.
	    ** Weight that number by the number of SCB
	    ** pools it took to contain those pages.
	    */
	    if ( (svcb->svcb_st_hint[SCBType][used_pages] +=
		  scb->scb_mem_pools) >
		    svcb->svcb_st_hint[SCBType]
			[svcb->svcb_st_hint[SCBType][DM0M_Hint]] )
	    {
		svcb->svcb_st_hint[SCBType][DM0M_Hint] = used_pages;
	    }


	    /*
	    ** There should be no memory allocated at end-of-session;
	    ** if there is, display it, then throw it away.
	    */

	    pool = &scb->scb_pool_list;

	    if ( scb->scb_mem_alloc )
		/*  Scan the pool(s) dumping control blocks. */
		dm0m_search_pools(pool, 0, dm0m_fmt_cb, &flag);

	    while (delete_pool = pool->obj_next)
	    {
		pool->obj_next = delete_pool->obj_next;

		/* Skip pool if preallocated part of SCB */
		if ( delete_pool->obj_pool == OBJ_P_STATIC )
		    continue;
		if ( delete_pool->obj_pool == OBJ_P_SCF )
		{
		    /* Memory allocated by SCF */
		    scf_cb.scf_type = SCF_CB_TYPE;
		    scf_cb.scf_length = sizeof(SCF_CB);
		    scf_cb.scf_session = DB_NOSESSION;
		    scf_cb.scf_facility = DB_DMF_ID;
		    scf_cb.scf_scm.scm_functions = 0;
		    scf_cb.scf_scm.scm_in_pages = delete_pool->obj_size / SCU_MPAGESIZE;
		    scf_cb.scf_scm.scm_addr = (char *)delete_pool;
		    status = scf_call(SCU_MFREE, &scf_cb);
		    if (status != E_DB_OK)
			return (status);
		}
		else
		    /* Allocated by MEreqmem */
		    MEfree((PTR)delete_pool);
	    }
	}
    }
    else
    {
	/* Server shutdown. Accumulate stats, destroy LT pools */

	sem_requests = 0;
	sem_collisions = 0;
	alloc = dealloc = expand = 0;
	pools_total = pools_alloc = pools_free = 0;

	TRdisplay("Searching and Destroying %d LT DMF Memory Pools...\n",
		    svcb->svcb_pools);

	for ( MyPool = 0; MyPool < svcb->svcb_pools; MyPool++ )
	{
	    pool = &svcb->svcb_pool_list[MyPool];

	    /* Skip unused pools */
	    if ( pool->obj_next )
	    {
		alloc += svcb->svcb_stat.mem_alloc[MyPool];
		dealloc += svcb->svcb_stat.mem_dealloc[MyPool];
		expand += svcb->svcb_stat.mem_expand[MyPool];

		dm0m_pool_info(pool, &total, &allocated, &free);
		pools_total += total;
		pools_alloc += allocated;
		pools_free  += free;

		/* Collect LT pool sem stats */
		if ((CSs_semaphore(CS_INIT_SEM_STATS, 
		    &svcb->svcb_mem_mutex[MyPool], 
		    &sstat, sizeof(sstat))) == OK)
		{
		    sem_requests += sstat.excl_requests + sstat.share_requests;
		    sem_collisions += sstat.excl_collisions + sstat.share_collisions;
		}

		/*  Scan the pool dumping control blocks. */
		dm0m_search_pools(pool, 0, dm0m_fmt_cb, &flag);

		while (delete_pool = pool->obj_next)
		{
		    pool->obj_next = delete_pool->obj_next;

		    scf_cb.scf_type = SCF_CB_TYPE;
		    scf_cb.scf_length = sizeof(SCF_CB);
		    scf_cb.scf_session = DB_NOSESSION;
		    scf_cb.scf_facility = DB_DMF_ID;
		    scf_cb.scf_scm.scm_functions = 0;
		    scf_cb.scf_scm.scm_in_pages = delete_pool->obj_size / SCU_MPAGESIZE;
		    scf_cb.scf_scm.scm_addr = (char *)delete_pool;
		    status = scf_call(SCU_MFREE, &scf_cb);
		    if (status != E_DB_OK)
			return (status);
		}
		MEfill(sizeof(DM_OBJECT), 0, &svcb->svcb_pool_list[MyPool]);
		MEfill(sizeof(DM_OBJECT), 0, &svcb->svcb_free_list[MyPool]);
	    }
	}

	/* Display collected LT stats */
	TRdisplay("DMF LT Pools memory total: %d, allocated: %d, free: %d\n",
			    pools_total, pools_alloc, pools_free);

	TRdisplay("DMF LT Pools memory alloc: %d, dealloc: %d, expand: %d\n",
			    alloc, dealloc, expand);
	      

	TRdisplay("DMF LT Pools semaphore requests %d, collisions %d\n",
		    sem_requests, sem_collisions);

	TRdisplay("DMF ST alloc %d, %d-byte pages, #pages(weight):\n",
		    svcb->svcb_stat.ses_mem_alloc, DM0M_PageSize);
	for ( i = 0; i <= DM0M_Hints; i++ )
	{
	    if ( svcb->svcb_st_hint[i][DM0M_Hint] )
	    {
		TRdisplay("%s Hint %d, ",
			    (i == DM0M_User) ? "User    " : "Factotum",
			    svcb->svcb_st_hint[i][DM0M_Hint]);
		for ( k = 0, j = 1; j <= DM0M_MaxPages; j++ )
		{
		    if ( svcb->svcb_st_hint[i][j] )
		    {
			TRdisplay("%d(%d) ",
			    j, svcb->svcb_st_hint[i][j]);
			if ( (++k % 8) == 0 )
			    TRdisplay("\n\t");
		    }
		}
		TRdisplay("\n");
	    }
	}

	/* If we've been collecting stats, display them */
	if ( svcb->svcb_dm0m_stat[0].tag )
	    dm0m_stats_by_type(TRUE);
    }

    return (E_DB_OK);
}

/*{
** Name: dm0m_search	- Search the object pool.
**
** Description:
**      Search the object pool for the specific object type that
**	the caller requested, or for all object types.  For each
**	object that qualifies call the caller function with the
**	address of the object and his optional argument.
**
** Inputs:
**	scb				DML_SCB if searching session
**					ShortTerm pools, NULL otherwise.
**      type                            The type of object to search for,
**					including DM0M_SHORTTERM,
**					DM0M_LONGTERM class bits.
**					Zero means all.
**      function                        Address of the function to call for
**					each qualifing function.
**      arg                             Optional arguments to pass to the 
**					qualifiying function.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory database is not locked.
**
** History:
**	08-oct-1985 (derek)
**          Created new for Jupiter.
**	07-oct-93 (johnst)
**	    Bug #56442
**	    Changed type of dm0m_search() argument "arg" from (i4) to
**	    (i4 *) to match the required usage by its callers from dmdcb.c.
**	    For 64-bit pointer platforms (axp_osf), cannot get away with 
**	    overloading 64-bit pointers into 32-bit i4s!
**	15-Aug-2005 (jenjo02)
**	    Tweaked to handle multiple pools, session ShortTerm pools.
*/
DB_STATUS
dm0m_search(
    DML_SCB	*scb,
    i4		TypeWithClass,
    VOID	(*function)(),
    i4		*arg,
    DB_ERROR	*dberr)
{
    i4		MyPool;

    CLRDBERR(dberr);

    /* If SCB, search session's ST pool list */
    if ( scb )
    {
	dm0m_search_pools(&scb->scb_pool_list,
			    TypeWithClass, function,
			    arg);
    }
    else for ( MyPool = 0; MyPool < dmf_svcb->svcb_pools; MyPool++ )
    {
	/* Search all LongTerm pools */
	dm0m_search_pools(&dmf_svcb->svcb_pool_list[MyPool],
			    TypeWithClass, function,
			    arg);
    }

    return(E_DB_OK);
}

static VOID
dm0m_search_pools(
    DM_OBJECT	*pool,
    i4		TypeWithClass,
    VOID	(*function)(),
    i4		*arg)
{
    DM_OBJECT	*obj;
    i4	 	dm_obj_align = dm0m_object_align();

    while (pool = pool->obj_next)
    {
	    obj = (DM_OBJECT *)((char *)pool + dm_obj_align);
	    while (obj < (DM_OBJECT *)((char *)pool + pool->obj_size))
	    {
		    if ( (obj->obj_type != OBJ_FREE 
			    && obj->obj_type != OBJ_POOL) &&
			(TypeWithClass == DM0M_ALL || 
			 TypeWithClass == obj->obj_type) )
		    {
			(*function)(arg, obj);
		    }
		    obj = (DM_OBJECT *)((char *)obj + obj->obj_size);
	    }
    }
    return;
}

/*{
** Name: dm0m_verify	- {@comment_text@}
**
** Description:
**      The routine scans the object allocation pools and performs
**	various checks on the integrity of the pool.
**	The following checks are performed:
**	    - Checks performed on the POOL
**		checks the links
**		checks for overlapping pool's
**		check for ordering of pool segments
**		check sizes of pools
**		check types of pools
**	    - Checks performed on the FREE list
**		checks the links
**		checks for overlapping free objects
**		checks for free object ordering
**		check size of free objects
**		check type of free objects
**	    - Scan the Pool's
**		check that each object has a legal size and type
**		check that there is no overlap of allocated objects with free
**
**	This routine does not assume that objects on the free list have memory
**	addresses greater than the address of the free list head.
** Inputs:
**      flag                            If DM0M_READONLY then write protect
**					the pool.  If DM0M_WRITEABLE then
**					make the pool writable again.
**					If DM0M_POOL_CHECK is set then also check
**					the pool.
** Outputs:
**      err_code                        Error return describing error found.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory databae is not locked.
**
** History:
**	09-oct-1985 (derek)
**          Created new for Jupiter.
**	02-nov-93 (swm)
**	    Bug #56450
**	    Fixed consistency check that an object is aligned on a
**	    correct boundary. Existing code assumed that sizeof(DM_OBJECT)
**	    was a power of 2 in its masking operation, which happens to be
**	    to be true on sun4. Changed code to calculate an alignment
**	    check mask of the first power of 2 greater than or equal to
**	    sizeof(DM_OBJECT), so that masking will work.
**	03-aug-2005 (sheco02)
**	    Fixed the x-integration 478041 mistake by putting #endif back.
*/
VOID
dm0m_verify(i4 flag)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_OBJECT	*pool;
    DM_OBJECT	*object;
    DM_OBJECT	*item;
    i4	 	dm_obj_align = dm0m_object_align();
    i4		MyPool;

    /*	See if we are trying to protect/unprotect the pool. */
    for ( MyPool = 0; MyPool < svcb->svcb_pools; MyPool++ )
    {
	pool = &svcb->svcb_pool_list[MyPool];

	if ((flag & DM0M_READONLY) || (flag & DM0M_WRITEABLE))
	{
	    while (pool->obj_next)
	    {
		DM_OBJECT	    *address_range[2];
		DB_STATUS	    status;

		address_range[0] = pool->obj_next;
		address_range[1] = (DM_OBJECT *)((char *)pool->obj_next 
					 + pool->obj_next->obj_size - 1);
#ifdef VMS
		status = sys$setprt(address_range, 0, 0, 
				    (flag & DM0M_READONLY) ? 15 : 4, 0);
		if ((status & 1) == 0)
		{
		    dmd_check(E_DMF00B_DM0M_PROTECT);
		}
#endif
		pool = pool->obj_next;
	    }
	}
    }
    if ((flag & DM0M_POOL_CHECK) == 0)
	return;

    /*	Scan the pool list performing consistency checks. */

#ifdef xDEBUG
    for ( MyPool = 0; MyPool < svcb->svcb_pools; MyPool++ )
    {
	pool = &svcb->svcb_pool_list[MyPool];
	object = &svcb->svcb_free_list[MyPool];
	dm0s_mlock(&dmf_svcb->svcb_mem_mutex[MyPool]);

	do
	{
	    /*  Pool must be aligned on a page (512) byte boundary. */

	    /* lint truncation warning if size of ptr > int, but code valid */
	    if (((int)pool->obj_next & 511))
	    {
		dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
	    }
	    if (pool != &svcb->svcb_pool_list[MyPool])
	    {
		/*
		**  The type of object should be TYPE_POOL and it's length
		**  should be a multiple of the pool EXPAND_INCREMENT.
		*/

		if (pool->obj_type != OBJ_POOL ||
		    pool->obj_size < EXPAND_INCREMENT ||
		    (pool->obj_size & (SCU_MPAGESIZE - 1)))
		{
		    dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		}

		if (pool->obj_next)
		{
		    /*
		    ** The pools should be linked in ascending address order
		    ** and they should not overlap.
		    */

		    if (pool->obj_next < pool ||
			((char *)pool + pool->obj_size) > (char *)pool->obj_next)
		    {
			dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		    }
		}
	    }
	} while (pool = pool->obj_next);

	/*	Scan the free list performing consistency checks. */

	do
	{
	    /*
	    **  The type of object should be OBJ_FREE, the next pointer address
	    **  should be a multiple of the alignment, and the previous address
	    ** should be a multiple of the alignment.
	    */

	    /* lint truncation warning if size of ptr > int, but code valid */
	    if (object->obj_type != OBJ_FREE ||
		((int)object->obj_next & (dm_obj_align - 1)) ||
		(object->obj_prev != &svcb->svcb_free_list[MyPool] &&
		    ((int)object->obj_prev & (dm_obj_align - 1))))
	    {
		dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
	    }	    
	    if (object != &svcb->svcb_free_list[MyPool])
	    {
		/*
		**  The free object should be linked correctly to the previous
		**  and the next.  The length should be non-zero and a multiple
		**  of the alignment.
		*/

		if ((object->obj_next && object->obj_next->obj_prev != object) ||
		    (object->obj_prev && object->obj_prev->obj_next != object) ||
		    object->obj_size == 0 ||
		    (object->obj_size & (dm_obj_align - 1)))
		{
		    dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		}

		/*
		**  The free object should be ordered by ascending memory addresses
		**  and should not overlap with the next free object.
		*/

		if ((object->obj_next && object > object->obj_next) ||
		    (object->obj_prev && object < object->obj_prev) ||
		    (object->obj_next &&
			((char *)object + object->obj_size) >
			     (char *)object->obj_next))
		{
		    dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		}

		/*
		** Check that the free object is contained within the bounds of
		** of one of the pools.
		*/

		pool = &svcb->svcb_pool_list[MyPool];
		while (pool = pool->obj_next)
		    if (object >= (DM_OBJECT *)((char *)pool + dm_obj_align) &&
			((char *)object + object->obj_size) <=
			((char *)pool + pool->obj_size))
		    {
			break;
		    }
		if (pool == 0)
		{
		    dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		}
	    }
	}	while (object = object->obj_next);

	/*	Now scan the pool checking the consistency of all objects. */


	object = svcb->svcb_free_list[MyPool].obj_next;
	pool = &svcb->svcb_pool_list[MyPool];

	while (pool = pool->obj_next)
	{
	    /*  Skip over the pool object at the beginning of a pool. */

	    item = (DM_OBJECT *)((char *)pool + dm_obj_align);

	    do
	    {
		if (item == object)
		{
		    /* Found next free object skip it and remeber next. */

		    object = object->obj_next;
		    item = (DM_OBJECT *)((char *)item + item->obj_size);
		}
		else
		{
		    /*
		    **  The object type should be within range and the length
		    **  should be non-zero and a multiple of the alignment and
		    **  the object shouldn't overlap with the next free object.
		    */

		    if (item->obj_type == OBJ_FREE || item->obj_type == OBJ_POOL ||
			item->obj_size == 0 ||
			(item->obj_size & (dm_obj_align - 1)) ||
			(object &&
			    ((DM_OBJECT *)((char *)item + item->obj_size)
			    > object)))
		    {
			dmd_check(E_DMF00C_DM0M_POOL_CORRUPT);
		    }
		    item = (DM_OBJECT*)((char *)item + item->obj_size);
		}
	    } while (item < (DM_OBJECT *)((char *)pool + pool->obj_size));
	}
	dm0s_munlock(&svcb->svcb_mem_mutex[MyPool]);
    }
#endif /* xDEBUG */
}

/*{
** Name: dm0m_info	- Return information about allocated memory.
**
** Description:
**	This routine returns information about memory allocated through
**	the dm0m_allocate call.
**
**	Currently, information returned includes:
**
**	    total_memory	- the total amount of memory allocated by
**				  DM0M module.
**	    allocated_memory	- the amount of 'total_memory' which is
**				  currently allocated to DMF routines.
**	    free_memory		- the amount of 'total_memory' which remains
**				  unallocated.
**
**	Output pointers may be passed in as NULL if information is not 
**	requested.
**
**	Note that allocated_memory and free_memory may not add up to
**	total_memory due to overhead used by DM0M memory management.
**
** Inputs:
**	none
**
** Outputs:
**	total_memory	 	- the total amount of memory allocated by 
**				  the DM0M module.
**	allocated_memory 	- the amount of 'total_memory' which is
**			   	  currently allocated to DMF routines.
**	free_memory	 	- the amount of 'total_memory' which remains 
**			   	  unallocated.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-feb-1991 (rogerk)
**          Created as part of Archiver Stability project.
**	15-Aug-2005 (jenjo02)
**	    Tweaked to handle multiple pools.
*/
VOID
dm0m_info(
    SIZE_TYPE	    *total_memory,
    SIZE_TYPE	    *allocated_memory,
    SIZE_TYPE	    *free_memory)
{
    DM_OBJECT	*pool;
    SIZE_TYPE	total_mem = 0;
    SIZE_TYPE	alloc_mem = 0;
    SIZE_TYPE	free_mem = 0;
    i4		MyPool;

    for ( MyPool = 0; MyPool < dmf_svcb->svcb_pools; MyPool++ )
    {
	/*
	** Calculate total memory in memory pools.
	*/
	pool = &dmf_svcb->svcb_pool_list[MyPool];
	dm0m_pool_info(pool, &total_mem, &alloc_mem, &free_mem);
	if ( total_memory )
	    *total_memory += total_mem;
	if ( allocated_memory )
	    *allocated_memory += alloc_mem;
	if ( free_memory )
	    *free_memory += free_mem;
    }
}
static VOID
dm0m_pool_info(
    DM_OBJECT	    *pool,
    SIZE_TYPE	    *total_memory,
    SIZE_TYPE	    *allocated_memory,
    SIZE_TYPE	    *free_memory)
{
    DM_OBJECT	*ppool = pool;
    DM_OBJECT	*obj;
    SIZE_TYPE	total_mem = 0;
    SIZE_TYPE	alloc_mem = 0;
    SIZE_TYPE	free_mem = 0;
    i4		dm_obj_align = dm0m_object_align();

    /*
    ** Calculate total memory in memory pools.
    */
    while (pool = pool->obj_next)
	total_mem += pool->obj_size;


    /*
    ** If free or allocated memory information is requested, then search
    ** scan through all allocated memory objects.  Since this is much
    ** more expensive, only do it if requested.
    */
    if (allocated_memory || free_memory)
    {
	pool = ppool;
	while (pool = pool->obj_next)
	{
	    for (obj = (DM_OBJECT *)((char *)pool + dm_obj_align);
		 obj < (DM_OBJECT *)((char *)pool + pool->obj_size);
		 obj = (DM_OBJECT *)((char *)obj + obj->obj_size))
	    {
		if (obj->obj_type == OBJ_FREE)
		    free_mem += obj->obj_size;
		else if (obj->obj_type != OBJ_POOL)
		    alloc_mem += obj->obj_size;
	    }
	}
    }

    /*
    ** Return requested information to the caller.
    */
    if (total_memory)
	*total_memory = total_mem;
    if (allocated_memory)
	*allocated_memory = alloc_mem;
    if (free_memory)
	*free_memory = free_mem;
}

/*{
** Name: dm0m_object_align	- Return DM_OBJECT alignment factor
**
** Description:
**	This routine returns the alignment factor to which DM_OBJECTs
**	are aligned. The alignment is the first power of 2 greater than
**	or equal to sizeof(DM_OBJECT).
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    DM_OBJECT alignment
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-nov-93 (swm)
**	    Bug #56450
**	    The DM0M memory allocator assumed that sizeof(DM_OBJECT) was a
**	    power of 2 in its alignment masking operations and that this
**	    value was 32 bytes (which is what it happens to be on sun4).
**	    Created this function to return the next power of two greater
**	    than or equal to sizeof(DM_OBJECT) so the result can be used
**	    in alignment mask operations.
**	15-Aug-2005 (jenjo02)
**	    Also calculate Minimum Fragment length as align**2
*/
static i4
dm0m_object_align()
{
	static i4  align = 0;

	/*
	** calculate the alignment factor once and save in static memory
	*/
	if (align == 0)
	{
    		for (align = 32; align < sizeof(DM_OBJECT); align *= 2);
		MinFrag = align * align;
	}

	return align;
}

/*
** Name: dm0m_tballoc		- tuple buffer allocator
**
** Description:
**	This routine allocates a tuple buffer for the caller. Tuple buffers
**	are always of size DM_MAXTUP, and must be freed by calling
**	dm0m_tbdealloc.
**
** Inputs:
**      tuple_size
**
** Outputs:
**	None
**
** Returns:
**	tuplebuf		pointer to a tuple buffer, or NULL if no mem.
**
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Created.
**      10-mar-1997 (stial01)
**          Added tuple_size parameter
**	15-Aug-2005 (jenjo02)
**	    Get LongTerm memory.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
char *
dm0m_tballoc(SIZE_TYPE tuple_size)
{
    DM_OBJECT	*mem_ptr;
    i4		err_code;
    DB_STATUS	status;
    DB_ERROR	local_dberr;

    tuple_size += sizeof(DMP_MISC);
    status = dm0m_allocate(tuple_size,
   	     (i4)DM0M_LONGTERM, (i4)MISC_CB, 
	     (i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, &local_dberr);
    if (status != OK)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1,
		sizeof(tuple_size), &tuple_size);
	return ((char *)NULL);
    }
    ((DMP_MISC*)mem_ptr)->misc_data = (char*)mem_ptr + sizeof(DMP_MISC);
    return ((char *)mem_ptr + sizeof(DMP_MISC));
    
}

/*
** Name: dm0m_tbdealloc		    - Frees a tuple buffer
**
** Description:
**	This routine frees a tuple buffer which was allocated by dm0m_tballoc.
**
**	For convenience, this routine checks to see if the passed in pointer
**	is NULL, and is a no-op if so. This relieves the umpteen callers from
**	needing to make this check.
**
** Inputs:
**	tuplebuf		    - tuple buffer.
**
** Outputs:
**	None, but the buffer is freed.
**
** Returns:
**	VOID
**
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Created.
*/
VOID
dm0m_tbdealloc(char **tuplebuf)
{
    DM_OBJECT	*mem_ptr;

    if (*tuplebuf)
    {
	mem_ptr = (DM_OBJECT *)(*tuplebuf - sizeof(DMP_MISC));
	(VOID) dm0m_deallocate(&mem_ptr);
    }
    *tuplebuf = (char *) NULL;
}

/*
** Name: dm0m_pgalloc		- page buffer allocator
**
** Description:
**	This routine allocates a page buffer for the caller. page buffers
**	are short term buffers used typically by the low level page accessor
**	for copying pages around during garbage collection. Page buffers must
**	be freed by calling dm0m_pgdealloc.
**
**	The returned memory buffer is guaranteed to be direct/raw aligned.
**	In order to be able to free the buffer, we'll hide a pointer to
**	the actual alloc'ed memory immediately in front of the buffer.
**
** Inputs:
**	page_size		- page size desired.
**
** Outputs:
**	None
**
** Returns:
**	pagebuf		pointer to a page buffer, or NULL if no mem.
**
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Created.
**	15-Aug-2005 (jenjo02)
**	    Changed to just MEreqmem, which gives better
**	    potential alignment for a "page".
**	10-Nov-2009 (kschendel) SIR 122757
**	    Buffers need to be properly aligned.  Rather than make the caller
**	    do (almost) everything, return an aligned buffer;  and stash
**	    a pointer to the unaligned memory area immediately in front
**	    (so that the memory can be freed).
*/
char *
dm0m_pgalloc(i4 page_size)
{
    STATUS	cl_stat;
    i4		err_code;
    i4		align, aligned_size;
    char	*mem, *amem;

    align = sizeof(PTR);
    if (align < dmf_svcb->svcb_directio_align)
	align = dmf_svcb->svcb_directio_align;
    if (dmf_svcb->svcb_rawdb_count > 0 && align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    aligned_size = page_size + sizeof(PTR) + align;
    if ( (mem = MEreqmem(0, aligned_size, FALSE, &cl_stat)) == NULL )
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1,
		0, page_size);
    amem = mem + sizeof(PTR);
    amem = ME_ALIGN_MACRO(amem, align);
    *(((PTR *)amem)-1) = mem;
    return(amem);
}

/*
** Name: dm0m_pgdealloc		    - Frees a page buffer
**
** Description:
**	This routine frees a page buffer which was allocated by dm0m_pgalloc.
**
**	For convenience, this routine checks to see if the passed in pointer
**	is NULL, and is a no-op if so. This relieves the umpteen callers from
**	needing to make this check.
**
** Inputs:
**	pagebuf		    - page buffer.
**
** Outputs:
**	None, but the buffer is freed.
**
** Returns:
**	VOID
**
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Created.
**	15-Aug-2005 (jenjo02)
**	    Changed to just MEreqmem, which gives better
**	    potential alignment for a "page".
**	10-Nov-2009 (kschendel) SIR 122757
**	    *pagebuf is now aligned, get original mem base from immediately
**	    in front.  See pgalloc.
*/
VOID
dm0m_pgdealloc(char **pagebuf)
{
    if (*pagebuf)
	MEfree(*((PTR *)(*pagebuf)-1));
    *pagebuf = (char *) NULL;
}

static VOID
dm0m_fmt_cb(
i4             *flag_ptr,
DM_OBJECT           *obj)
{
TRdisplay("\n%.4s Control Block @0x%p..0x%p owned by 0x%x for %d bytes.\n\n",
	    &obj->obj_tag, obj, (char *)obj + obj->obj_size - 1, 
            obj->obj_owner, obj->obj_size);
}

static VOID
dm0m_pad_error(DM_OBJECT *obj, char *padchar)
{
    i4		orig_len;
    i4		error;
    char	buf[256];

    /* This is a separate function so you can set a breakpoint at the error */
    orig_len = *(i4 *) (((char *)obj) + obj->obj_size - sizeof(i4));
    TRformat(NULL, 0, buf, sizeof(buf),
	"DMF pad error %x (o:%p n:%p p:%p %d %d) for (%.4s/%x)\n",
	*padchar, obj, obj->obj_next, obj->obj_prev, obj->obj_size, orig_len,
	(char *)&obj->obj_tag, obj->obj_tag);
    TRdisplay("%s", buf);
    uleFormat(NULL, E_UL0017_DIAGNOSTIC, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
	STlength(buf), buf);
}

/*
** Name: dm0m_stats_by_type
**
** Description:
**	Enable//disable/display statistics collected by type
**	of memory in response to trace point DM105.
**
**	set trace point DM105 turns them on and begins collecting,
**	but if already collecting, displays what's been accumulated
**	so far.
**
**	set notrace point DM105 displays anything collected, then
**	stops collection.
**
** Inputs:
**	turning_on	TRUE if set trace point DM105
**			FALSE if set notrace point DM105
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** History:
**	15-Aug-2005 (jenjo02)
**	    Created.
*/
VOID
dm0m_stats_by_type(
i4	turning_on
)
{
    DM_SVCB	*svcb = dmf_svcb;
    i4		i;
    DM0M_STAT	*Type;

    /*
    ** Note that we can't rely on the presence or absence
    ** of DMZ_MEM_MACRO(5) because it's an "immediate"
    ** trace point and gets turned "off" as soon as we
    ** return from this function.
    **
    ** Instead, we'll use the "tag" element of the 0th
    ** entry of the array (which will be otherwise unused
    ** as "type" is always > 0) as a boolean TRUE or FALSE.
    */

    /* If already tracing, display what we have */
    if ( svcb->svcb_dm0m_stat[0].tag )
    {
	TRdisplay("\nDMF memory allocations by type:\n");
	for ( i = 1; i <= DM0M_MAX_TYPE; i++ )
	{
	    Type = &svcb->svcb_dm0m_stat[i];
	    if ( Type->longterm || Type->shortterm )
		TRdisplay("%2d %.4s: LT %d ST %d, minLen %d maxLen %d now %d hwm %d \n", 
		    i, (char*)&Type->tag, 
		    Type->longterm,
		    Type->shortterm,
		    Type->min, 
		    Type->max,
		    Type->alloc_now,
		    Type->alloc_hwm);
	}
    }

    /* If turning on, start/continue collecting */
    if ( turning_on )
	svcb->svcb_dm0m_stat[0].tag = TRUE;
    else
    {
	/* If turning off, reset them all */
	MEfill(sizeof(svcb->svcb_dm0m_stat), 0, 
		(PTR)&svcb->svcb_dm0m_stat);
	for ( i = 1; i <= DM0M_MAX_TYPE; i++ )
	    svcb->svcb_dm0m_stat[i].min = MAX_SIZE_TYPE;
    }
    return;
}

/*
** Name: dm0m_init	- Prepare dm0m for use.
**
** Description:
**	Initializes DMF memory pool(s) for use.
**
** Inputs:
**	dmf_svcb
**	    svcb_status		If SVCB_SINGLEUSER, initializes
**				a single LongTerm pool, ShortTerm
**				SCB memory won't be used.
**	    svcb_pools		The number of LT pools to init
**				!(SVCB_SINGLEUSER)
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** History:
**	15-Aug-2005 (jenjo02)
**	    Created.
*/
VOID
dm0m_init(void)
{
    DM_SVCB	*svcb = dmf_svcb;
    i4		i;
    char	sem_name[CS_SEM_NAME_LEN];

    
    if ( svcb->svcb_status & SVCB_SINGLEUSER )
    {
	svcb->svcb_pools = 1;
	svcb->svcb_pad_bytes = 0;
    }

    for ( i = 0; i < svcb->svcb_pools; i++ )
    {
	MEfill(sizeof(DM_OBJECT), '\0', (PTR)&svcb->svcb_pool_list[i]);
	MEfill(sizeof(DM_OBJECT), '\0', (PTR)&svcb->svcb_free_list[i]);

	dm0s_minit(&svcb->svcb_mem_mutex[i],
	    STprintf(sem_name, "DMF pool list %d", i));
    }

    /* Start allocating from 1st LT pool */
    svcb->svcb_next_pool = 0;

    /* Clear out ShortTerm memory feedback area */
    MEfill(sizeof(svcb->svcb_st_hint), '\0', 
		(PTR)&svcb->svcb_st_hint);

    /* Init stats stuff in svcb */
    svcb->svcb_dm0m_stat[0].tag = FALSE;
    dm0m_stats_by_type(FALSE);

    return;
}

/*
** Name: dm0m_init_scb
**
** Description:
**	Prepares DML_SCB for ShortTerm memory use.
**	If configured, initialize preallocated memory
**	chunck as session's 1st ShortTerm pool.
**
** Inputs:
**	scb		Session's DML_SCB
**	dmf_svcb
**	    .svcb_st_ialloc
**			Number of bytes at the end of the
**			SCB pre-allocated by SCF for this
**			use, if any. Must be at least
**			MinFrag bytes.
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** History:
**	15-Aug-2005 (jenjo02)
**	    Created.
*/
VOID
dm0m_init_scb(
DML_SCB		*scb)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_OBJECT	*pool, *new_pool;
    DM_OBJECT	*obj, *new_obj;
    SIZE_TYPE	pool_size;
    i4	 	dm_obj_align = dm0m_object_align();


    /* Init mem-related stuff in SCB */
    scb->scb_mem_alloc = 0;
    scb->scb_mem_hwm = 0;
    scb->scb_mem_pools = 0;
    
    MEfill(sizeof(DM_OBJECT), 0, &scb->scb_pool_list);
    MEfill(sizeof(DM_OBJECT), 0, &scb->scb_free_list);

    /* If static (SCF-allocated) ST memory... */
    if ( (pool_size = svcb->svcb_st_ialloc) >= MinFrag )
    {
	/* Round down to multiples of MinFrag */
	pool_size = (pool_size / MinFrag) * MinFrag;

	pool = &scb->scb_pool_list;
	obj  = &scb->scb_free_list;

	/* Format preallocated chunk as a pool, all free */
	new_pool = (DM_OBJECT*)((char*)scb + sizeof(DML_SCB));
	new_pool->obj_next = pool->obj_next;
	new_pool->obj_size = pool_size;
	new_pool->obj_type = OBJ_POOL;
	new_pool->obj_tag = OBJ_T_POOL;
	new_pool->obj_scb = (PTR)scb;
	/* Mark pool as static (embedded), not freeable */
	new_pool->obj_pool = OBJ_P_STATIC;

	pool->obj_next = new_pool;
	pool->obj_prev = new_pool;

	/* Make it all one free object */
	new_obj = (DM_OBJECT *)((char *)new_pool + dm_obj_align);

	new_obj->obj_next = obj->obj_next;
	new_obj->obj_prev = obj;
	new_obj->obj_size = pool_size - dm_obj_align;
	new_obj->obj_type = OBJ_FREE;
	new_obj->obj_tag = OBJ_T_FREE;
	obj->obj_next = new_obj;

	scb->scb_mem_pools = 1;
    }

    return;
}
