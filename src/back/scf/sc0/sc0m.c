/*
**Copyright (c) 2004 Ingres Corporation
**
*/
#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <ex.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <cs.h>
#include    <cv.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <gca.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <ulm.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc0e.h>
#include    <sc0m.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scfcontrol.h>

#ifdef VMS
#include <ssdef.h>
#include <starlet.h>
#include <astjacket.h>
#endif

/**
**
**  Name: SC0M.C - Internal Memory Manager for SCF
**
**  Description:
**      This file contains the code for the SCF memory manager. 
**      The memory is organized into pools for different sized 
**      objects.  All objects are in multiples of sc0m_obj_align
**	bytes, where sc0m_obj_align is sizeof(SC0M_OBJECT) rounded
**	up to the next power of 2.
**
**
**	    sc0m_add_pool - expand scf's memory (static)
**          sc0m_allocate - return memory to scf function
**          sc0m_check - check consistency of memory arena (xDEBUG only)
**          sc0m_deallocate - free memory
**	    sc0m_free_pages - free a sc0m_get_pages'd area of memory
**	    sc0m_get_pages - get contiguous area of memory for a sub facility
**          sc0m_initialize - initialize SCF's memory pool(s)
**	    sc0m_poolhdr_align - calculate SC0M_POOL alignment factor
**	    sc0m_object_align - calculate SC0M_OBJECT alignment factor
**
**
**  History:
**	05-Aug-88 (anton)
**	    Changes to use ME page requests.
**      12-Mar-86 (fred)    
**          Created on Jupiter
**	05-jan-89 (mmm)
**	    finish memory bootstrap fix.  Must not use Sc_main_cb anywhere in
**	    sc0m_get_pages() if we are allocating it.
**	15-may-89 (rogerk)
**	    Added out_pages parameter to MEget_pages call in sc0m_get_pages.
**	23-may-89 (jrb)
**	    Change ule_format calls to conform to new interface.
**	07-mar-90 (neil)
**	    Zero out GCA memory allocated.
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**      06-apr-93 (smc)
**          Commented out text after ifdef.
**	29-Jun-1993 (daveb)
**	    include <tr.h>
**	2-Jul-1993 (daveb)
**	    prototyped, move func externs to <sc0m.h>
**	8-Jul-1993 (daveb)
**	    Get pages can't check &Sc_main_cb to see if it's allocating
**	    the main cb.  Instead, check Sc_main_cb for NULL before
**	    dereferencing it, which is a better test anyway.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	    Change casts in pointer compares from u_i4 to PTR.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in sc0m_check().
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      07-apr-1995 (kch)
**          The function sc0m_allocate() now returns status if status is true,
**          and similarly, the function sc0m_free_pages() now returns error if
**          error is true. The variables status and error are both set by
**          calls to the function CSp_semaphore().
**          Previously, if status and error were true the functions would
**          return E_SC0028_SEMAPHORE_DUPLICATE regardless of the value of
**          status or error.
**          This change fixes bug 65445.
**	04-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	03-jul-95 (allmi01)
**	    Turn off optimization for this module only to prevent 
**	    dgi_us5 cc abort.
**	06-mar-1996 (stial01 for bryanp)
**	    Reject allocation requests greater than SC0M_MAXALLOC (formerly,
**		we would loop for a LONG time in sc0m_allocate, each time
**		adding another not-quite-big-enough pool element, until
**		eventually MEget_pages would fail and we would then return
**		bad-size-expand).
**      06-mar-1996 (stial01)
**          sc0m_allocate: Write error to log if E_SC0107_BAD_SIZE error occurs.
**          When this error occurs, the session is not added but the client
**          hangs. At least put an error in the log.
**      22-mar-1996 (stial01)
**          Changes to support SCF memory allocations > 64k
**          (necessary for 64k tuple support)
**          sc0m_allocate: changed i4  length to i4 length
**          SC0M_POOL: u_i2 scp_size_allocated -> u_i2 scp_xtra_bytes
**          SC0M_OBJECT: u_i2 sco_size_allocated -> u_i2 sco_xtra bytes
**	13-jun-96 (nick)
**	    LINT directed changes.
**	19-jul-1996 (sweeney)
**	    Bulletproof the xDEBUG code.
**	06-sep-1996 (canor01)
**	    sc0m_deallocate: release semaphore on a duplicate block error.
**	27-Jan-1997 (jenjo02)
**	    o When fragmenting a piece of storage, don't create fragments
**	      that are too small to ever be used.
**	    o Do Mefill (if we must) after we've released sc0m_semaphore.
**	25-Jun-1997 (jenjo02)
**	    o Prototyped static function sc0m_add_pool().
**	    o Removed acquisition of sc0m_semaphore in sc0m_get/free_pages().
**	      When called from within sc0m, the semaphore is already
**	      held to protect the pool list; when called from other
**	      sources, the semaphore is not needed as sc0m's pool
**	      list is not involved.
**	    o Mark sc0m_status SC0M_ADD_POOL and release sc0m_semaphore
**	      when adding a new pool. Lets concurrent threads continue to
**	      operate on existing pools while the memory is being allocated.
**	12-Sep-1997 (jenjo02)
**	      Defined a semaphore per pool. Use sc0m_semaphore to lock the
**	      list of pools and use it shared when searching for space.
**	      Reorganized a bunch of code to improve concurrency.
**	24-Oct-1997 (muhpa01, from canor01)
**	    Change to sc0m_allocate() to check CS_is_mt() before acquiring
**	    or releasing the sc0m_semaphore.  With this change, ingres-threaded
**	    servers will not call the semaphore routines.  This cures a fatal
**	    SEGV for the idle thread which can come into conflict with the
**	    Cs_incomp flag check in CSp_semaphore (HP-UX port).
**	05-nov-1998 (shust01)
**	    Added internal diagnostic for memory 'coloring'.
**	09-Apr-1998 (jenjo02)
**	    Modified muhpa01 change, above. During initialization, SC_IS_MT flag
**	    is set in Sc_main_cb->sc_capabilities if OS threads in use. This
**	    avoids having to call CS_is_mt() constantly.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	    Added padding to memory allocation with coloring to check for
**	    memory overruns.
**      18-jan-1999 (matbe01)
**          Added NO_OPTIM for NCR (nc4_us5).
**	21-jan-1999 (nanpr01)
**	    Release the semaphore before returning error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-feb-2001 (somsa01)
**	    Fixed compiler warnings.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      18-sep-2001 (stial01)
**          sc0m_deallocate() Maintain free list in ASCENDING order (b105792)
**      09-Sep-2002 (horda03) Bug 108672
**          On VMS, the use of IMA queries will cause memory leaks. This is
**          due to sc0m_gcdealloc() not releasing memory if invoked from an
**          AST. Added ssdef.h for VMS to provide definition of SS$_WASSET
**          used in this fix to determine if ASTs were enabled or not.
**          Added new variable (sc0m_ast_object) for VMS to maintain a link list
**          of SC0M_OBJECTs that sc0m_gcdealloc() couldn't delete because it
**          was invokded via an AST. Also, added a new VMS only function
**          (sc0m_release_objects) to deallocate entries in the sc0m_ast_object
**          list.
**	25-feb-2003 (abbjo03)
**	    Fix above change to reflect 14-jan-1999 change to sc0m_deallocate.
**      18-feb-2004 (stial01)
**          Changed SC0M_OBJECT and sc0m_allocate to be like dmf DM_object
**          and dm0m_allocate. Object size is SIZE_TYPE. No need to maintain
**          sco_xtra_bytes = (adjusted size - requested size)
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool related vars/params.
**      09-nov-2004 (stial01)
**          Added diagnostic for SC0107
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**      29-jan-2009 (smeke01) b121565
**          Improve SC0M main semaphore handling to reduce likelihood 
**          of contention.
**/

/*
**  Forward and/or External function references.
NO_OPTIM=dgi_us5 nc4_us5 i64_aix
*/

static i4  sc0m_poolhdr_align(void);
static i4  sc0m_object_align(void);
static i4 sc0m_add_pool(i4 flag);

#ifdef VMS
static void sc0m_release_objects();
static SC0M_OBJECT *sc0m_ast_object = 0;
#endif

GLOBALREF SC_MAIN_CB	*Sc_main_cb;       /* Main controlling structure */

/* defines for semaphore functions */
#define		SEM_SHARE	0
#define		SEM_EXCL	1

#define		PAD_BYTE	64
#define		PAD_CHAR	0x7F


/*{
** Name: sc0m_add_pool	- Expand SCF's pool of memory
**
** Description:
**      This function adds a pool of memory to scf's memory arena. 
**      The pool header is extracted built in the new pool, and the free 
**      list is constructed of the remainder of the list
**
** Inputs:
**	flag			SC0M_NOSMPH_MASK if sc0m_semaphore
**				is not used.
**
** Outputs:
**	Returns:
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-Aug-88 (anton)
**	    Added NOSMPH_MASK to call of sc0m_get_pages.
**	13-Mar-86 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    made static, prototyped.
**	25-Jun-1997 (jenjo02)
**	    Always allocates to sc0m_pool_next/prev pool list, 
**	    so removed pool_list as an input parm.
**	    Set SC0M_ADD_POOL status and release sc0m_semaphore while we're
**	    off allocating a new pool. This allows other threads to search
**	    existing pools at the same time. Should a pool allocation be in 
**	    progress when this function is called, return OK; the caller 
**	    should retry the operation, eventually finding the newly-allocated pool.
**      29-jan-2009 (smeke01) b121565
**          Moved sc0m_semaphore locking so that it is held only for
**          the final update of .sc0m_pool_next.
*/
static i4
sc0m_add_pool(i4 flag)
{
    SC0M_POOL	    *pool;
    SC0M_OBJECT	    *object;
    SC0M_POOL	    *addr;
    STATUS	    status;
    SIZE_TYPE	    page_count;
    i4		    sc0m_pool_align = sc0m_poolhdr_align();
    char	    sem_name[ CS_SEM_NAME_LEN ];


    Sc_main_cb->sc_sc0m.sc0m_status |= SC0M_ADD_POOL;

    status = sc0m_get_pages(flag, (i4) SC0M_EXPAND_SIZE,
					&page_count, (PTR *) &addr);

    if (status != E_SC_OK)
    {
	Sc_main_cb->sc_sc0m.sc0m_status &= ~SC0M_ADD_POOL;
	return(status);
    }
	
    pool = addr;
    object = (SC0M_OBJECT *) ((char *)addr + sc0m_pool_align);
    pool->scp_start = (PTR) addr;
    pool->scp_end = ((char *) addr + (page_count * SCU_MPAGESIZE) - 1);
    pool->scp_psize = page_count * SCU_MPAGESIZE;
    pool->scp_status = E_SC_OK;
    pool->scp_free_list = (SC0M_OBJECT *) &pool->scp_nfree;
			/* the queue hdr */
    pool->scp_nfree = pool->scp_pfree = object;
    pool->scp_tag = SC_POOL_TAG;
    pool->scp_pool_hdr = pool;
    pool->scp_type = SC0M_PTYPE;
    pool->scp_owner = SCF_MEM;
    pool->scp_status = SC0M_INIT;
    pool->scp_length = sc0m_pool_align;
    pool->scp_s_reserved = 0;
    pool->scp_flags = 0;
    CSw_semaphore(&pool->scp_semaphore, CS_SEM_SINGLE,
	STprintf(sem_name, "SC0M pool %x-%x", pool->scp_start, pool->scp_end));
    
    object->sco_next = object->sco_prev = pool->scp_free_list;
	    /* point them both at the queue header */
    object->sco_pool_hdr = pool;
    /* ignore long -> i4  inaccuracies */
    /*NOSTRICT*/
    object->sco_size = pool->scp_psize - sc0m_pool_align;
    object->sco_s_reserved = 0;
    object->sco_tag = SC_FREE_TAG;
    object->sco_type = SC0M_FTYPE;
    object->sco_owner = SCF_MEM;

    /* Count another pool allocation */
    Sc_main_cb->sc_sc0m.sc0m_pools++;

    /* Link new pool to head of pool list */
    pool->scp_next = Sc_main_cb->sc_sc0m.sc0m_pool_next;
    pool->scp_prev = (SC0M_POOL*)NULL;
    /* 
    ** Note that the prev pointers are not relied on anywhere else. 
    ** If this changes, then the fact that they are currently not 
    ** protected by mutex would need to be reviewed.
    */
    if (Sc_main_cb->sc_sc0m.sc0m_pool_next == (SC0M_POOL*)NULL)
    {
	Sc_main_cb->sc_sc0m.sc0m_status |= SC0M_INIT;
	Sc_main_cb->sc_sc0m.sc0m_pool_prev = pool;
    }
    else
    {
	Sc_main_cb->sc_sc0m.sc0m_pool_next->scp_prev = pool;
    }

    /* 
    ** Note: 
    ** 1. This is the only place (after initialisation) that we update 
    **    the field .sc0m_pool_next.
    ** 2. This single update to .sc0m_pool_next is what makes the pool visible so 
    **    that it can be allocated from, hence it is performed after all other 
    **    updates to the pool.
    */

    if ((flag & SC0M_NOSMPH_MASK) == 0)
	CSp_semaphore(SEM_EXCL, &Sc_main_cb->sc_sc0m.sc0m_semaphore);

    Sc_main_cb->sc_sc0m.sc0m_pool_next = pool;

    if ((flag & SC0M_NOSMPH_MASK) == 0)
	CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore);

    /* Let others know we're done adding a pool */
    Sc_main_cb->sc_sc0m.sc0m_status &= ~SC0M_ADD_POOL;

    return(E_SC_OK);
}

/*{
** Name: sc0m_allocate	- allocate chunk of memory for SCF use
**
** Description:
**      This function allocates memory from the current free list.  Memory 
**      is maintained on a free list per pool, sorted by address.  A pool 
**      is a contiguous area allocated from the system.  The allocation 
**      algorithm simply looks down the free list, returning the first 
**      block which fits.  If the block is too large, then it is split.
**	Blocks are rounded up to the next power of 2 greater than
**	sizeof(SC0M_OBJECT) before allocation.
**	These routines make use of the reserved fields in the structure header
**	for internal management and corruption checking.
**
** Inputs:
**	flag				bit mask for options
**					such as zeroing memory, etc.
**      length                          amount of memory to allocate
**					    The maximum length is SC0M_MAXALLOC.
**      type                            type of the entry
**      owner                           owning module for information
**					    and tuning purposes
**      tag                             tag field for dump analysis
**      block                           ptr to vble to receive new address
**
** Outputs:
**      *block                          address of allocated memory
**	Returns:
**	    E_DB_OK			all went fine
**	    E_DB_ERROR			block request is too small
**	    E_DB_FATAL			memory arena has been corrupted or
**					memory limit has been exceeded
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Mar-86 (fred)
**          Created on Jupiter
**	04-nov-1986 (fred)
**	    Fixed header fixup area to reset pool header after zeroing
**	2-Jul-1993 (daveb)
**	    prototyped.
**	02-nov-93 (swm)
**	    Bug #56450
**	    Initialisation of size offset assumed that sizeof(SC0M_OBJECT)
**	    was a power of 2 in its masking operation, and function comment
**	    assumed this value to be 32 (which is what it happens to be on
**	    sun4). Changed initialisation to calculate first power of 2
**	    greater than or equal to sizeof(SC0M_OBJECT), so that masking
**	    will work.
**      15-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	    This results in owner within this function changing type to PTR.
**      07-apr-1995 (kch)
**          This function now returns status if status is true. The variable
**          status is set by calls to the function CSpsemaphore(), in the
**          form:
**          status = CSp_semaphore(TRUE, &Sc_main_cb->sc_sc0m.sc0m_semaphore).
**          Previously, if status was true (ie. CSp_semaphore()
**          returned a true value - see unix!cl!clf!cs cssem.c for
**          list of return values for CSp_semaphore()) the function would
**          return E_SC0028_SEMAPHORE_DUPLICATE regardless of the value of
**          status.
**	06-mar-1996 (stial01 for bryanp)
**	    Reject allocation requests greater than SC0M_MAXALLOC (formerly,
**		we would loop for a LONG time in sc0m_allocate, each time
**		adding another not-quite-big-enough pool element, until
**		eventually MEget_pages would fail and we would then return
**		bad-size-expand).
**      06-mar-1996 (stial01)
**          sc0m_allocate: Write error to log if E_SC0107_BAD_SIZE error occurs.
**          When this error occurs, the session is not added but the client
**          hangs. At least put an error in the log.
**      22-mar-1996 (stial01)
**          Change length parameter to i4.
**	27-Jan-1997 (jenjo02)
**	    o When fragmenting a piece of storage, don't create fragments
**	      that are too small to ever be used.
**	    o Do Mefill (if we must) after we've released sc0m_semaphore.
**	12-Sep-1997 (jenjo02)
**	    Rewrote the pool search algorithm to use shared semaphores
**	    and skip over "busy" pools.
**	24-Oct-1997 (muhpa01, from canor01)
**	    Change to sc0m_allocate() to check CS_is_mt() before acquiring
**	    or releasing the sc0m_semaphore.  With this change, ingres-threaded
**	    servers will not call the semaphore routines.  This cures a fatal
**	    SEGV for the idle thread which can come into conflict with the
**	    Cs_incomp flag check in CSp_semaphore (HP-UX port).
**	27-oct-1997 (nanpr01)
**	    sc0m_0_put takes 2 parameter instead of 1.
**	11-nov-1998 (nanpr01)
**	    Donot fragment the memory unnecessarily. Same logic of 
**	    dm0m_allocate has been now implemented in sc0m.
**      09-Sep-2002 (horda03) Bug 108672
**          On VMS, when allocating memory outside of an AST, first release
**          any memory that was not deallocated because invoked via an AST.
**      29-jan-2009 (smeke01) b121565
**          Moved sc0m_semaphore locking so that it is held only when 
**          we need to read .sc0m_pool_next. Add new semaphore  
**          sc0m_addpool_semaphore to prevent concurrent update of 
**          the pool list.
*/
i4
sc0m_allocate(i4 flag,
	      SIZE_TYPE size,
	      u_i2 type,
	      PTR owner,
	      i4  tag,
	      PTR *block )
{
    register SC0M_POOL          *pool;
    register SC0M_POOL          *first_pool;
    register SC0M_POOL          *next_pool;
    register SC0M_OBJECT        *current;
    register SC0M_OBJECT	*best_object;
    register SC0M_OBJECT	*object;
    i4		        sc0m_obj_align = sc0m_object_align();
    SIZE_TYPE			adjusted_size;/* rounded up to next multiple of
					   sc0m_obj_align */
    i4		status = E_DB_OK;	/* we'll assume it worked */
    i4			busy_pools;

#ifdef VMS
    /* Before allocating memory, check to see if memory needs deallocating
    ** because it was deleted via an AST.
    */

    if ( (lib$ast_in_prog() == 0) && sc0m_ast_object)
    {
       sc0m_release_objects();
    }
#endif

    adjusted_size = (size + (sc0m_obj_align - 1)) & ~(sc0m_obj_align - 1);

#ifdef		DEV_MEFILL
    adjusted_size += PAD_BYTE;    
#endif

    /* Don't take semaphores unless OS threads */
    if ( (flag & SC0M_NOSMPH_MASK) == 0 &&
	 (Sc_main_cb->sc_capabilities & SC_IS_MT) == 0 )
	flag |= SC0M_NOSMPH_MASK;

    if (adjusted_size > SC0M_MAXALLOC)
    {
	sc0e_0_put(E_SC0107_BAD_SIZE_EXPAND, 0);
	TRdisplay("%@ sc0m_allocate: E_SC0107_BAD_SIZE_EXPAND: adjusted_size %d \n", 
		adjusted_size);
	return (E_SC0107_BAD_SIZE_EXPAND);
    }

#ifdef xDEBUG
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMCHECK, NULL, NULL))
    {
	status = sc0m_check(flag | SC0M_BLOCK_PRINT_MASK, "sc0m_allocate");
	if (status != E_DB_OK)
	    return(E_SC0102_MEM_CORRUPT);
    }
#endif /* xDEBUG */

    if (!Sc_main_cb->sc_sc0m.sc0m_status & SC0M_INIT)
    {
	if (Sc_main_cb->sc_sc0m.sc0m_status == SC0M_UNINIT)
	    return(E_SC0101_NO_MEM_INIT);
	else
	    return(E_SC0102_MEM_CORRUPT);
    }

    for (;;)
    {
	if ((flag & SC0M_NOSMPH_MASK) == 0)
	{
	    /* 
	    ** Use exclusive sem to get the extent of the existing pool  
	    ** list that we can read safely without a semaphore.
	    */
	    if (status = CSp_semaphore(SEM_EXCL, &Sc_main_cb->sc_sc0m.sc0m_semaphore))
		return(status);
	}

	first_pool = Sc_main_cb->sc_sc0m.sc0m_pool_next;

	if ((flag & SC0M_NOSMPH_MASK) == 0)
	{
	    if (status = CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore))
		return(status);
	}

	current = (SC0M_OBJECT*)NULL;
	best_object = (SC0M_OBJECT*)NULL;
	busy_pools = 0;

	for (pool = first_pool;
	     pool != (SC0M_POOL*)NULL;
	     pool = next_pool)
	{

	    /* If pool has been found to be corrupted, go no furthur */
	    if (pool->scp_status == SC0M_CORRUPT)
	    {
		return(E_SC0104_CORRUPT_POOL);
	    }

	    next_pool = pool->scp_next;

	    /* If the pool is busy with another thread, skip it for now */
	    if (pool->scp_flags & SCP_BUSY)
	    {
		busy_pools++;
		continue;
	    }

	    /*
	    ** Pool is not busy.
	    ** Get pool sem and recheck
	    */
	    if ((flag & SC0M_NOSMPH_MASK) == 0)
	    {
		CSp_semaphore(SEM_EXCL, &pool->scp_semaphore);
	    }

	    if (pool->scp_flags & SCP_BUSY)
		busy_pools++;
	    else if (pool->scp_free_list != pool->scp_nfree)
	    {
		/* Search the free list for sufficient space */
		for (current = pool->scp_free_list->sco_next;
		     current != pool->scp_free_list;
		     current = current->sco_next)
		{
		    if (current->sco_size < adjusted_size)
		    {
			if (current->sco_next != pool->scp_free_list)
			    continue;
			else if (best_object)
			{
			    current = best_object;
			    break;
			}
		    }
		    else
		    {
			/* Stop if exact fit */
			if (current->sco_size == adjusted_size)
			    break;

			/*
			** Best fit comprimise so we don't always do 
			** exhaustive search
			*/
			if (current->sco_size <= adjusted_size + 4096)
			    break;

			if (!best_object || 
			    current->sco_size < best_object->sco_size)
			    best_object = current;

			/* Reached the end */
			if (current->sco_next == pool->scp_free_list)
			{
			    current = best_object;
			    break;
			}
		    }
		}
		/* If not at the end of the list, we found the block we want */
		if (current != pool->scp_free_list)
		    break;

	    }

	    current = (SC0M_OBJECT*)NULL;

	    if ((flag & SC0M_NOSMPH_MASK) == 0)
	    {
		CSv_semaphore(&pool->scp_semaphore);
	    }

	    /*
	    ** New pools are added to the front of the list.
	    ** If a new one has appeared, restart the search.
	    ** Note that we can use the fact that .sc0m_pool_next 
	    ** might have *changed* but if it has, we can't use 
	    ** it's value until we've re-read .sc0m_pool_next with 
	    ** the semaphore locked.
	    */
	    if (first_pool != Sc_main_cb->sc_sc0m.sc0m_pool_next)
	    {
		if ((flag & SC0M_NOSMPH_MASK) == 0)
		{
	    	    if (status = CSp_semaphore(SEM_EXCL, &Sc_main_cb->sc_sc0m.sc0m_semaphore))
			return(status);
		}

		first_pool = next_pool = Sc_main_cb->sc_sc0m.sc0m_pool_next;

		if ((flag & SC0M_NOSMPH_MASK) == 0)
		{
		    if (status = CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore))
			return(status);
		}
	    }
	}


	if (current == (SC0M_OBJECT*)NULL)
	{
	    if (busy_pools == 0)
	    {
		/*
		** If we're here we didn't find a pool with sufficient space so we
		** must check to see if a pool is being/has been added, and if not 
		** add one. Note that we can use the fact that .sc0m_pool_next might 
		** have *changed* but if it has, we can't use it's value until we've 
		** re-read .sc0m_pool_next with the semaphore locked. Note that 
		** .sc0m_semaphore *is* locked at the top of the outer loop.
		*/

		/* 
		** Is someone in the process of adding a pool or has someone completed 
		** the addition of a pool since we last looked?  First take a look 
		** without attempting a lock. 
		*/
		if ( (Sc_main_cb->sc_sc0m.sc0m_status & SC0M_ADD_POOL) || 
			(first_pool != Sc_main_cb->sc_sc0m.sc0m_pool_next))
		{
		    /* 
		    ** There will soon be, or is already by now, at least one more pool 
		    ** to look at, so go round the outer loop again. 
		    */
		    continue;
		}

		/* Take exclusive semaphore to ensure no concurrent adding of pools */
		if ((flag & SC0M_NOSMPH_MASK) == 0)
		    CSp_semaphore(SEM_EXCL, &Sc_main_cb->sc_sc0m.sc0m_addpool_semaphore);

		/* 
		** Has someone added a pool whilst we were contending for the semaphore...? 
		*/ 
		if (first_pool != Sc_main_cb->sc_sc0m.sc0m_pool_next)
		{
		    /* 
		    ** ...yes, so there's at least one more pool to look at, 
		    ** so go round the outer loop again.
		    */ 
		    if ((flag & SC0M_NOSMPH_MASK) == 0)
			CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_addpool_semaphore);
		    continue;
		}

		if (status = sc0m_add_pool(flag))
		{
		    /* Something went wrong in adding a pool */
		    if ((flag & SC0M_NOSMPH_MASK) == 0)
			CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_addpool_semaphore);

		    return(status);
		}

		if ((flag & SC0M_NOSMPH_MASK) == 0)
		    CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_addpool_semaphore);
	    }

	    /* Start pool search again from the top */
	    continue;
	}
	break;
    }

    /* At this point, current is guaranteed to point to the block we want */

    *block = (PTR) current;

    /* now update internal information */

    /* Let other threads know we're allocating from this pool */
    pool->scp_flags |= SCP_BUSY;

    /* 
    ** If extra space is less than or equal to the size 
    ** of an object header, don't create a useless fragment.
    */
    if ((current->sco_size - adjusted_size) <= sc0m_obj_align)
    {
	adjusted_size = current->sco_size;

    	if (current->sco_prev->sco_next = current->sco_next)
	    current->sco_next->sco_prev = current->sco_prev;
    }
    else
    {
	object = (SC0M_OBJECT *) ((char *) current + adjusted_size);
	/*NOSTRICT*/
	object->sco_size = current->sco_size - adjusted_size;
	object->sco_s_reserved = 0;
	object->sco_next = current->sco_next;
	object->sco_prev = current->sco_prev;
	object->sco_owner = SCF_MEM;
	object->sco_type = SC0M_FTYPE;
	object->sco_tag = SC_FREE_TAG;
	object->sco_pool_hdr = pool;	    /* current pool */

	/*
	** now link this new block into the free list
	*/ 

	object->sco_prev->sco_next = object;
	object->sco_next->sco_prev = object;
    }

    /* We're done with this pool. Let others know */
    pool->scp_flags &= ~(SCP_BUSY);
    if ((flag & SC0M_NOSMPH_MASK) == 0)
	CSv_semaphore(&pool->scp_semaphore);

    /* pool is now restored, fill information for user */

    /* remember, *block == current, so this is filling in the users info */

    current->sco_next = current->sco_prev = (SC0M_OBJECT*)NULL;
    current->sco_extra = 0;
    current->sco_size = adjusted_size;
    current->sco_owner = owner;
    current->sco_tag = tag;
    current->sco_type = type;
    current->sco_s_reserved = 0;
    current->sco_pool_hdr = pool;	/* fill in in case zero'd */

    /* Only clear the amount of memory that was requested */
    if (flag & SCU_MZERO_MASK)
	MEfill(size - sizeof(SC0M_OBJECT), 0, (char *)&current[1]);

#ifdef 	DEV_MEFILL
    MEfill(adjusted_size - size, PAD_CHAR, (PTR)((char *) current) + size);
    *(SIZE_TYPE *)(((char *)current) + adjusted_size - sizeof(SIZE_TYPE)) = length;
#endif
    return(E_DB_OK);
}

/*{
** Name: sc0m_check	- check the consistency of the SCF memory pools
**
** Description:
**      This routine performs a set of consistency checks on the memory 
**      pools in use by SCF.  If so requested, it will also perform 
**      a dump to the TR device of the memory pools as they are 
**      encountered. 
** 
**      The following checks are performed by this routine.  The list of pools 
**      are checked to see that the pools are linked in the correct order, 
**      and that all data items in the pool structure are consistent (greater 
**      than zero bytes, correct type & size, etc...).  Also, for each 
**      pool, the associated free list is checked to guarantee that the 
**      free blocks are sorted correctly, coalesced correctly, are of a 
**      reasonable (> 0 bytes, < pool size, and evenly divisible by the
**	first power of 2 >= sizeof(SC0M_OBJECT)) size, and uncorrupted. 
** 
**      If any inconsistency is found, a TR message is printed (if enabled) 
**      and a fatal error is returned.  The memory pool area is also marked 
**      as corrupted.  If the pool list is corrupted, then the entire memory 
**      arena is marked as corrupt.
**
**	If so asked, the protection on the memory arena is changed to disallow
**	access to unauthorized parties.
**
** Inputs:
**      flag                            Bit mask indicating what action to
**					perform.  Currently:
**					SC0M_BLOCK_PRINT_MASK to print out the
**					    blocks as encountered.
**					SC0M_READONLY_MASK: make the memory
**					    arena READONLY
**					SC0M_WRITEABLE_MASK: make the memory
**					    arena writeable
**
**	tag				String to print out on entry 
**					(typically name of calling routine.)
**
** Outputs:
**                                      Output is to TR
**	Returns:
**	    E_DB_OK, or E_SC0102_MEM_CORRUPT
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**	05-Aug-88 (anton)
**	    Removed commented out VMS code
**	17-Mar-86 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	25-Jun-1997 (jenjo02)
**	    Removed check for pools in ascending address order, which may
**	    have never been reliable, and now certainly isn't; pool
**	    order is constantly being shuffled depending on whether it
**	    contains free space or not.
*/
i4
sc0m_check( i4 flag, char *tag )
{
    SC0M_POOL           *pool = Sc_main_cb->sc_sc0m.sc0m_pool_next;
    SC0M_OBJECT		*current;
    i4			found_corruption = 0;
    i4		error;
    i4		status;
    i4		        sc0m_pool_align = sc0m_poolhdr_align();
    i4		        sc0m_obj_align = sc0m_object_align();

#ifdef xDEBUG
    if (flag & Sc_main_cb->sc_sc0m.sc0m_flag)
    {
	TRdisplay("*** sc0m_check with flags: %v, tag: %s ***\n",
	    "SC0M_BLOCK_PRINT,SC0M_READONLY_MASK,SC0M_WRITEABLE_MASK,OTHER",
	     flag, (char *)tag);
	TRdisplay("*** Main SCF memory control area: ***\n");
	TRdisplay("\tsc0m_flag: %x, sc0m_status: %w %<(%x)\n",
		    Sc_main_cb->sc_sc0m.sc0m_flag,
		    "SC0M_UNINIT,SC0M_INIT,SC0M_CORRUPT",
		    Sc_main_cb->sc_sc0m.sc0m_status);
	TRdisplay("\tsc0m_pool_next: %p, sc0m_pool_prev: %p\n",
		    Sc_main_cb->sc_sc0m.sc0m_pool_next,
		    Sc_main_cb->sc_sc0m.sc0m_pool_prev);
    }
#endif /* xDEBUG */

    if ((flag & SC0M_NOSMPH_MASK) == 0)
    {
	if (status = CSp_semaphore(SEM_SHARE, &Sc_main_cb->sc_sc0m.sc0m_semaphore))
	    return(status);
    }

    if ((Sc_main_cb->sc_sc0m.sc0m_status != SC0M_UNINIT) &&
	    (!Sc_main_cb->sc_sc0m.sc0m_status & SC0M_INIT))
    {
        if ((flag & SC0M_NOSMPH_MASK) == 0)
	    CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore);
	return(E_SC0102_MEM_CORRUPT);
    }
    while (pool)		/* check pool list consistency */
    {
#ifdef	xDEBUG
	if (flag & Sc_main_cb->sc_sc0m.sc0m_flag & SC0M_BLOCK_PRINT_MASK)
	{
	    TRdisplay("*** Pool Structure at 0x%p: ***\n", pool);
	    TRdisplay("\tscp_next: %p\tscp_type: %w %<(%d.)\n",
		pool->scp_next,
		"SC0M_PTYPE,SC0M_FTYPE", pool->scp_type);
	    TRdisplay("\tscp_prev: %p\tscp_tag: %4t\n",
		pool->scp_prev, 4, &pool->scp_tag);
	    TRdisplay("\tscp_length: %d.\t\tscp_owner: %w %<(%d.)\n",
		pool->scp_length,
		"SCF_MEM,SCU_MEM,SCC_MEM,SCS_MEM,SCD_MEM", pool->scp_owner);
	    TRdisplay("\tscp_pool_hdr: %p\n", pool->scp_pool_hdr);
	    TRdisplay("\tscp_start: %p\tscp_status: %w %<(%x)\n",
		pool->scp_start,
		"SC0M_UNINIT,SC0M_INIT,SC0M_CORRUPT", pool->scp_status);
	    TRdisplay("\tscp_end: %p\tscp_psize %x %<(%d.)\n",
		pool->scp_end, pool->scp_psize);
	    TRdisplay("\tscp_free_list: %p\n", pool->scp_free_list);
	}
#endif /* xDEBUG */

	if (  	(pool->scp_s_reserved != 0)
	    ||	(pool->scp_length != sc0m_pool_align)
	    ||	(pool->scp_type != SC0M_PTYPE)
	    ||	(pool->scp_tag != SC_POOL_TAG)
	    ||	(pool->scp_owner != SCF_MEM)
	    ||	(pool->scp_psize % SCU_MPAGESIZE != 0)
	  )
	{
	    pool->scp_status = SC0M_CORRUPT;
	    Sc_main_cb->sc_sc0m.sc0m_status |= SC0M_CORRUPT;
	    ule_format(E_SC0104_CORRUPT_POOL, 0, ULE_LOG, NULL, 0, 0, 0, 
					    &error, 1, sizeof(pool), &pool);
	    if ((flag & SC0M_NOSMPH_MASK) == 0)
		CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore);
	    return(E_SC0102_MEM_CORRUPT);
	}

	/*
	** Now that we "know" that the pool header is healthy,
	** let's go on and check out the free list
	*/

	current = pool->scp_free_list->sco_next;
	while (current != pool->scp_free_list)
	{
#ifdef	xDEBUG
	    if (flag & Sc_main_cb->sc_sc0m.sc0m_flag & SC0M_BLOCK_PRINT_MASK)
	    {
		TRdisplay("*** Free block at 0x%p: ***\n", current);
		TRdisplay("\tsco_next: %p\t\tsco_type: %w %<(%d.)\n",
		    current->sco_next,
		    "SC0M_PTYPE,SC0M_FTYPE", current->sco_type);
		TRdisplay("\tsco_prev: %p\t\tsco_tag: %4t\n",
		    current->sco_prev, 4, &current->sco_tag);
		TRdisplay("\tsco_size: %d.\t\tsco_owner: %w %<(%d.)\n",
		    current->sco_size,
		    "SCF_MEM,SCU_MEM,SCC_MEM,SCS_MEM,SCD_MEM",
		    current->sco_owner);
		TRdisplay("\tsco_pool_hdr: %p\n", current->sco_pool_hdr);
	    }
#endif	/* xDEBUG */
	    /* is this particular block bad */

	    if (	((current->sco_next <= current) &&
			(current->sco_pool_hdr) && /* don't deref if nil */
			(current->sco_next !=
			    current->sco_pool_hdr->scp_free_list))
		||	((current->sco_prev >= current) &&
			(current->sco_pool_hdr) && /* don't deref if nil */
			(current->sco_prev !=
			    current->sco_pool_hdr->scp_free_list))
		||	(current->sco_s_reserved != 0)
		||	(current->sco_type != SC0M_FTYPE)
		||	(current->sco_tag != SC_FREE_TAG)
		||	(current->sco_owner != SCF_MEM)
		||	((char *) current->sco_pool_hdr >= (char *) current)
		||	(current->sco_pool_hdr) && /* don't deref if nil */
		  	((current->sco_next !=
				current->sco_pool_hdr->scp_free_list) && 
			(current->sco_next <=
				(SC0M_OBJECT *) ((char *) current + current->sco_size))
		    )
	      )
	    {
		/*
		** in the case of a corrupted free list,
		** only mark the pool in question as bad
		*/
		pool->scp_status = SC0M_CORRUPT;
		found_corruption++;
		ule_format(E_SC0105_CORRUPT_FREE, 0, ULE_LOG, NULL, 0, 0, 0,
					&error, 1, sizeof(current), &current);
		break;
	    }
	    current = current->sco_next;
	}

	/* else there is no free list, ergo nothing to do */
	pool = pool->scp_next;
    }
    if ((flag & SC0M_NOSMPH_MASK) == 0)
	CSv_semaphore(&Sc_main_cb->sc_sc0m.sc0m_semaphore);
    if (found_corruption)
	return(E_SC0102_MEM_CORRUPT);
    else
	return(E_DB_OK);
}

/*{
** Name: sc0m_deallocate	- free some memory allocated from SCF's pool
**
** Description:
**      This routines frees a previously sc0m_allocate'd piece of memory 
**      by returning it to the free list for the pool to which it belongs 
**      The free lists are maintained in sorted order by address of the block; 
**      this routine provides that sorting.  Adjacent blocks are coalesced.
**
** Inputs:
**      block                           addr of block to be freed
**	flag				special actions to take
**
** Outputs:
**	Returns:
**	    E_DB_OK,
**	    E_DB_ERROR			if memory does not belong to a pool
**	    E_SC0102_MEM_CORRUPT	if arena is corrupt
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Mar-86 (fred)
**          Created on Jupiter
**	06-jan-1987 (fred)
**	    Added flag parameter to control semaphore usage.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	06-sep-1996 (canor01)
**	    release semaphore on a duplicate block error.
**	28-may-1997 (shero03)
**	    Added memory debugging by filling a freed block with x'FE'
**	25-Jun-1997 (jenjo02)
**	    Sanity check pool vs returned block.
**	15-jan-1999 (nanpr01)
**	    Donot coalesce memory if we are overflowing in addition.
*/
i4
sc0m_deallocate( i4  flag, PTR *returned_block)
{
    register SC0M_POOL          *pool;
    register SC0M_OBJECT	*object;
    register SC0M_OBJECT	*block;
    STATUS		status;
#ifdef  DEV_MEFILL
    i4	         	i, orig_len;
    i4			sc0m_pool_align = sc0m_poolhdr_align();
#endif

    if (!Sc_main_cb->sc_sc0m.sc0m_status & SC0M_INIT)
    {
	if (Sc_main_cb->sc_sc0m.sc0m_status ==  SC0M_UNINIT)
	    return(E_SC0101_NO_MEM_INIT);
	else
	    return(E_SC0102_MEM_CORRUPT);
    }

    if ((block = (SC0M_OBJECT *) *returned_block) == 0)
	return(E_SC000D_BUFFER_ADDR);

#ifdef	xDEBUG
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMCHECK, NULL, NULL))
    {
	status = sc0m_check(flag | SC0M_BLOCK_PRINT_MASK, "sc0m_deallocate");
	if (status != E_DB_OK)
	    return(status);
    }
#endif	/* xDEBUG */

    /* Sanity check the pool header vs the object */
    if ((pool = block->sco_pool_hdr) == (SC0M_POOL*)NULL ||
	 pool->scp_pool_hdr != pool ||
	 pool->scp_tag != SC_POOL_TAG ||
	 pool->scp_start > *returned_block ||
	 pool->scp_end   < *returned_block)
    {
	return(E_SC0102_MEM_CORRUPT);
    }

    /* If pool has been found to be corrupted, go no furthur */
    if (pool->scp_status == SC0M_CORRUPT)
	return(E_SC0104_CORRUPT_POOL);

    /* Lock the pool and mark it busy */
    if ((flag & SC0M_NOSMPH_MASK) == 0)
    {
	if (status = CSp_semaphore(SEM_EXCL, &pool->scp_semaphore))
	    return(status);
    }

    pool->scp_flags |= SCP_BUSY;
#ifdef DEV_MEFILL
    orig_len = *(SIZE_TYPE *) (((char *)block) + block->sco_size - sizeof(SIZE_TYPE);
    if (orig_len > block->sco_size || 
	orig_len < block->sco_size - PAD_BYTE - (sc0m_pool_align * 3))
    {
    	if ((flag & SC0M_NOSMPH_MASK) == 0)
	    CSv_semaphore(&pool->scp_semaphore);
	return(E_SC0102_MEM_CORRUPT);
    }

    /* consistency check for pads */
    for (i = 0; i < block->sco_size - orig_len - sizeof(SIZE_TYPE); i++)
    {
	if ((((char *) block) + orig_len)[i] != PAD_CHAR)
	{
    	    if ((flag & SC0M_NOSMPH_MASK) == 0)
	        CSv_semaphore(&pool->scp_semaphore);
	    return(E_SC0102_MEM_CORRUPT);
	}
    }
    MEfill(block->sco_size - sizeof(SC0M_OBJECT), 0xFE,
		(char *)(block)+sizeof(SC0M_OBJECT));
#endif 	/* DEV_MEFILL */

    /* Find object's place within the pool */
    object = pool->scp_free_list;
    while (object->sco_next != pool->scp_free_list &&
	    object->sco_next < block)
	object = object->sco_next;
		

    if ((char *) block == (char *) object->sco_next)
    {
	pool->scp_flags &= ~SCP_BUSY;
	TRdisplay("SC0M_DEALLOCATE: Duplicate Block -- \
type %x(%<%d.), tag '%t', owner %x(%<%d.)\n",
		    block->sco_tag, 4, &block->sco_tag, block->sco_owner);
	if ((flag & SC0M_NOSMPH_MASK) == 0)
	    CSv_semaphore(&pool->scp_semaphore);
	return(E_SC0016_BAD_PAGE_ADDRESS);
    }

    block->sco_s_reserved = 0;
    block->sco_next = object->sco_next;
    block->sco_next->sco_prev = block;
    block->sco_prev = object;
    object->sco_next = block;

    if ((char *) block->sco_next == ((char *)block + block->sco_size))
    {
	if (((block->sco_size + block->sco_next->sco_size) <= MAXI4) ||
	    ((block->sco_size + block->sco_next->sco_size) > 0))
	{
	    /* coalesce these blocks */
	    block->sco_size += block->sco_next->sco_size; 
	    block->sco_next = block->sco_next->sco_next;
	    block->sco_next->sco_prev = block;
	}
    }
    if (    (block->sco_prev != pool->scp_free_list) && 
	    ( ((char *)block->sco_prev + block->sco_prev->sco_size)
				== (char *) block)
	)
    {
	if (((block->sco_size + block->sco_prev->sco_size) <= MAXI4) ||
	    ((block->sco_size + block->sco_prev->sco_size) > 0))
	{
	    /* coalesce these blocks */
	    block->sco_prev->sco_size += block->sco_size;
	    block->sco_prev->sco_next = block->sco_next;
	    block->sco_next->sco_prev = block->sco_prev;
	    block = block->sco_prev;
	}
    }
    block->sco_type = SC0M_FTYPE;
    block->sco_tag = SC_FREE_TAG;
    block->sco_owner = SCF_MEM;

    *returned_block = (PTR) NULL;

    /* Let others know were done with this pool */
    pool->scp_flags &= ~SCP_BUSY;

    if ((flag & SC0M_NOSMPH_MASK) == 0)
	CSv_semaphore(&pool->scp_semaphore);

    return(E_DB_OK);
}

/*{
** Name: sc0m_free_pages	- return sc0m_get_pages'd area to the system
**
** Description:
**      This function returns pages to the system for use by other portions.
**      It may keep free'd pages around to be reused later on.
**
** Inputs:
**      page_count                      The number of pages to free
**      addr                            address of the first page to free.
**
** Outputs:
**      none
**	Returns:
**	    E_SC_OK or lower level error code
**	Exceptions:
**	    none
**
** Side Effects:
**	    The process's virtual address space may be shrunk.
**
** History:
**	25-Mar-1986 (fred)
**          Created on Jupiter.
**	03-Jul-1986 (fred)
**	    Changed calculation for ending bounds range so that it
**	    correctly calculates the last page instead of the page
**	    after the last page in the range.
**	08-Aug-1988 (anton)
**	    Added semaphores and changed direct to OS calls into calls
**		to new ME with page allocation
**	2-Jul-1993 (daveb)
**	    prototyped.
**	8-sep-93 (swm)
**	    Added comment about valid pointer truncation warning on code
**	    that checks for a bad page adress.
**	25-Jun-1997 (jenjo02)
**	    o Removed acquisition of sc0m_semaphore in sc0m_get/free_pages().
**	      When called from within sc0m, the semaphore is already
**	      held to protect the pool list; when called from other
**	      sources, the semaphore is not needed as sc0m's pool
**	      list is not involved.
*/
i4
sc0m_free_pages(SIZE_TYPE page_count, PTR *addr)
{
    i4		error;
    CL_SYS_ERR		sys_err;

    if (page_count <= 0)
	return(E_SC0015_BAD_PAGE_COUNT);
    /* lint truncation warning if size of ptr > int, but code valid */
    if (((SIZE_TYPE) *addr & (SCU_MPAGESIZE - 1)) != (SIZE_TYPE)0)
	return(E_SC0016_BAD_PAGE_ADDRESS);
	
    error = MEfree_pages(*addr, page_count, &sys_err);

    if (error != OK)
    {
	ule_format(E_SC0106_BAD_SIZE_REDUCE, &sys_err, ULE_LOG, NULL, 0, 0, 0,
		&error, 0);
	return(E_SC0106_BAD_SIZE_REDUCE);
    }
    *addr = (PTR) NULL;
    return(E_SC_OK);
}

/*{
** Name: sc0m_get_pages	- add some pages to the virtual addr space of the server
**
** Description:
**      This routine adds pages to the virtual address space of the server. 
**      It is used primarily by the scu memory services when so requested by 
**      one of the client facilities.  It adds pages which are SCU_MPAGESIZE bytes in 
**      length, and are guaranteed to be aligned on SCU_MPAGESIZE byte boundaries.
**
** Inputs:
**      flag                            flag specifying special operations
**					    (see sc0m_allocate)
**      in_pages                        number of pages requested
**
** Outputs:
**      out_pages                       number of pages actually requested
**      addr                            where did we put those pages
**	Returns:
**	    E_SC_OK if all went well
**	    lower level error code if not.
**	Exceptions:
**
**
** Side Effects:
**	    The process' virtual address space is expanded
**
** History:
**	05-Aug-88 (anton)
**	    Added semaphores and changed directt calls to the OS into new
**		ME page allocation routines
**	25-Mar-86 (fred)
**          Created on Jupiter
**	08-sep-88 (mmm)
**	    fix memory bootstrap problem in ult_check_macro.
**	05-jan-89 (mmm)
**	    finish memory bootstrap fix.  Must not use Sc_main_cb anywhere in
**	    this function if we are allocating it.
**	15-may-89 (rogerk)
**	    Added out_pages parameter to MEget_pages call.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	8-Jul-1993 (daveb)
**	    Get pages can't check &Sc_main_cb to see if it's allocating
**	    the main cb.  Instead, check Sc_main_cb for NULL before
**	    dereferencing it, which is a better test anyway.
**	25-Jun-1997 (jenjo02)
**	    o Removed acquisition of sc0m_semaphore in sc0m_get/free_pages().
**	      When called from within sc0m, the semaphore is already
**	      held to protect the pool list; when called from other
**	      sources, the semaphore is not needed as sc0m's pool
**	      list is not involved.
**	02-dec-1998 (nanpr01)
**	    Pass the SCU_LOCKED_MASK flag if set.
*/
i4
sc0m_get_pages(i4 flag,
	       SIZE_TYPE  in_pages,
	       SIZE_TYPE  *out_pages,
	       PTR *addr )
{
    i4	    error;
    CL_SYS_ERR	    sys_err;

    /* If we are allocating the Sc_main_cb with this call, we had better not
    ** check it's members.  
    */
    if ( Sc_main_cb != NULL &&
	(ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_GUARD_PAGES, NULL, NULL)))
    {
	    in_pages += 2;
    }

    /* 
    ** NOTE: SCU_{MZERO,MERASE,LOCKED}_MASK must match 
    ** ME_{MZERO,MERASE,LOCKED}_MASK 
    */
    error = MEget_pages((flag&(SCU_MZERO_MASK|SCU_MERASE_MASK|SCU_LOCKED_MASK)),
			in_pages, 0, addr, out_pages, &sys_err);

    if (error != OK)
    {
	ule_format(E_SC0107_BAD_SIZE_EXPAND, &sys_err, ULE_LOG, NULL, 0, 0, 0,
				&error, 0);
	TRdisplay("%@ sc0m_get_pages: E_SC0107_BAD_SIZE_EXPAND: pages %d \n", 
			in_pages);
	return(E_SC0107_BAD_SIZE_EXPAND);
    }

    /* If we are allocating the Sc_main_cb with this call, we had better not
    ** check it's members until it is initialized.  
    */
    if ( Sc_main_cb != NULL &&
        (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_GUARD_PAGES, NULL, NULL)))
    {
	(VOID) MEprot_pages(*addr, 1, 0);
	(VOID) MEprot_pages(*addr + SCU_MPAGESIZE * (in_pages - 1), 1, 0);
	*addr = ((char *) *addr + SCU_MPAGESIZE);
	in_pages -= 2;
    }	
    *out_pages = in_pages;
    return(E_SC_OK);
}

/*{
** Name: sc0m_initialize	- initialize the memory arena
**
** Description:
**      This routine allocates the initial pool of memory, 
**      and sets it up accordingly.  It also marks the server 
**      control block as initialized so that other memory services 
**      can run.  Otherwise, all other services will return fatal errors, 
**      indicating that they are unable to run.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    SCF memory arena is initialized.
**
** History:
**	13-Mar-86 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      29-jan-2009 (smeke01) b121565
**          Added initialisation of new .sc0m_addpool_semaphore.
*/
i4
sc0m_initialize(void)
{
    i4	    status;

    if (Sc_main_cb->sc_sc0m.sc0m_status != SC0M_UNINIT)
    {
	return(E_SC0100_MULTIPLE_MEM_INIT);
    }

    Sc_main_cb->sc_sc0m.sc0m_pool_next = 
	Sc_main_cb->sc_sc0m.sc0m_pool_prev = (SC0M_POOL*)NULL;
    Sc_main_cb->sc_sc0m.sc0m_pools = 0;
    
    status = CSw_semaphore( &Sc_main_cb->sc_sc0m.sc0m_semaphore,
			  CS_SEM_SINGLE, "SC0M main" );

    if( status != OK )
    {
	TRdisplay( "sc0m_initalize semaphore SC0M main init error %x\n", status );
	return( E_DB_ERROR );
    }

    status = CSw_semaphore( &Sc_main_cb->sc_sc0m.sc0m_addpool_semaphore,
			  CS_SEM_SINGLE, "SC0M add pool" );

    if( status != OK )
    {
	TRdisplay( "sc0m_initalize semaphore SC0M add pool init error %x\n", status );
	return( E_DB_ERROR );
    }

    if (status = sc0m_add_pool(SC0M_NOSMPH_MASK))
	return(status);
#ifdef	xDEBUG
    /*
    ** Let's check that we did it right
    */
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMCHECK, NULL, NULL))
    {
	status = sc0m_check(SC0M_BLOCK_PRINT_MASK, "sc0m_initialize");
	if (status != E_DB_OK)
	    return(status);
    }
#endif	/* xDEBUG */
    return(E_SC_OK);
}

/*{
** Name: sc0m_gcalloc	- Allocate memory for use by GCF
**
** Description:
**      This routine simply provides a MEalloc style interface to the 
**      SCF memory manager, as required by GCF.  It allocates the quantity 
**      of memory requested, but puts the appropriate header information 
**      onto the front so as not to screw things up too bad.
**
** Inputs:
**      num                             Number of objects to allocate
**                                      space for.
**      size                            Size of each object.
**      place				Ptr to place to put object(s) ptr.
**
** Outputs:
**      *place                          Filled with object ptr.
**	Returns:
**	    OK or error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-Jul-1987 (fred)
**          Created.
**	07-mar-90 (neil)
**	    Zero out GCA memory allocated.  This may be modified in GCA
**	    if they have a better way to do this.  This avoids bugs that
**	    turn up in GCA where they use dirty memory.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
DB_STATUS
sc0m_gcalloc(i4 num, i4 size, PTR *place)
{
    DB_STATUS	status;
    PTR		block;

    if (num <= 0 || size < 0)
	return(FAIL);

    status = sc0m_allocate(SCU_MZERO_MASK,
			    (i4)((num * size) + sizeof(SC0M_OBJECT)),
			    DB_GCF_ID,
    			    (PTR) DB_GCF_ID,
			    CV_C_CONST_MACRO('g','c','a','f'),
			    &block);
    if (status)
	return(status);

    *place = (PTR) ((char *) block + sizeof(SC0M_OBJECT));
    return(E_DB_OK);
}

/*{
** Name: sc0m_gcdealloc	- Deallocate GCF memory
**
** Description:
**      This routine provides an MEfree() style cover 
**      for sc0m_deallocate.  It is provided for use by GCF 
**      for managing its DBMS server memory.
**
** Inputs:
**      block                           Pointer to block to deallocate
**
** Outputs:
**      none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-Jul-1987 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      09-Sep-2002 (horda03) Bug 108672
**          Memory leak on VMS. Don't just ignore the memory deallocation if
**          this function is invokded via an AST. Rather add the SC0m_OBJECT
**          to the list of objects waiting deallocation (the next time
**          sc0m_allocate() invoked outside of an AST). We can't release the
**          memory here, as Semaphores may not be acquired from an AST - we
**          can't be certain that another (non-ast) session isn't manipulating
**          the sc0m memory queues.
[@history_template@]...
*/
DB_STATUS
sc0m_gcdealloc( PTR block )
{
    SC0M_OBJECT		*object;
    DB_STATUS           status;

    if (!block)
	return(E_SC000D_BUFFER_ADDR);
    object = (SC0M_OBJECT *) ((char *) block - sizeof(SC0M_OBJECT));
    if (    (object->sco_type != DB_GCF_ID)
	||  (object->sco_owner != (PTR) DB_GCF_ID)
	||  (object->sco_tag != CV_C_CONST_MACRO('g','c','a','f')))
	return(E_DB_OK);
#ifdef	VMS
    if (lib$ast_in_prog() == 0)
    {
#endif
    status = sc0m_deallocate(0, (PTR *)&object);

    return(status);
#ifdef	VMS
    }
    else
    {
        /* In an AST, so add the SC0M_OBJECT to the list of objects that
        ** will be deallocated the next time sc0m_allocate() is invoked
        ** (not in an AST).
        */

        object->sco_next = sc0m_ast_object;

        sc0m_ast_object  = object;

	return(OK);
    }
#endif
}

/*{
** Name: sc0m_poolhdr_align	- Return SC0M_POOL alignment factor
**
** Description:
**	This routine returns the alignment factor to which a SC0M_POOL header
**	is aligned. The alignment is the first power of 2 greater than
**	or equal to sizeof(SC0M_POOL)
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    SC0M_POOL alignment
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-nov-93 (swm)
**	    Bug #56450
**	    The SC0M memory allocator assumed that sizeof(SC0M_POOL) was a
**	    power of 2 in its alignment masking operations and that this
**	    value was 32 bytes (which is what it happens to be on sun4).
**	    Created this function to return the next power of two greater
**	    than or equal to sizeof(SC0M_POOL) so the result can be used
**	    in alignment mask operations.
*/
static i4
sc0m_poolhdr_align()
{
	static i4  align = 0;

	/*
	** calculate the alignment factor once and save in static memory
	*/
	if (align == 0)
	{
    		for (align = 32; align < sizeof(SC0M_POOL); align *= 2);
	}

	return align;
}

/*{
** Name: sc0m_object_align	- Return SC0M_OBJECT alignment factor
**
** Description:
**	This routine returns the alignment factor to which SC0M_OBJECTs
**	are aligned. The alignment is the first power of 2 greater than
**	or equal to sizeof(SC0M_OBJECT).
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    SC0M_OBJECT alignment
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-nov-93 (swm)
**	    Bug #56450
**	    The DM0M memory allocator assumed that sizeof(SC0M_OBJECT) was a
**	    power of 2 in its alignment masking operations and that this
**	    value was 32 bytes (which is what it happens to be on sun4).
**	    Created this function to return the next power of two greater
**	    than or equal to sizeof(SC0M_OBJECT) so the result can be used
**	    in alignment mask operations.
*/
static i4
sc0m_object_align()
{
	static i4  align = 0;

	/*
	** calculate the alignment factor once and save in static memory
	*/
	if (align == 0)
	{
    		for (align = 32; align < sizeof(SC0M_OBJECT); align *= 2);
	}

	return align;
}


#ifdef VMS
/*{
** Name: sc0m_release_objects   - Release SC0M_OBJECTs outside of AST
**
** Description:
**      This routine releases all SC0M_OBJECTS that were released inside
**      an AST. Memory can't be released inside an AST because the AST
**      cannot acquire Semaphores and hence there is no way of ensuring
**      integrity of the SC0M_OBJECT free queue.
**
**      To ensure that the sc0m_ast_object link list is not updated while
**      this function is in use, AST delivery is disabled.
**
** Inputs:
**      none
**
** Outputs:
**      Returns:
**          None
**      Exceptions:
**          none
**
** Side Effects:
**          SC0M_OBJECTS added to free queue.
**
** History:
**      09-Sep-2002 (horda03)
**          Created.
**	25-feb-2003 (abbjo03)
**	    Change call to sc0m_deallocate to reflect 14-jan-1999 change.
*/
void
sc0m_release_objects()
{
   /* Disable ASTs to prevent any new ASTs from altering the list. */
   i4          ast_enable = (sys$setast(0) != SS$_WASCLR);
   SC0M_OBJECT *object;
       
   for(object = sc0m_ast_object; object; object = sc0m_ast_object)
   {
      sc0m_ast_object = sc0m_ast_object->sco_next;

      sc0m_deallocate(0, (PTR *)&object);
   }

   if (ast_enable) sys$setast(1);
}
#endif
