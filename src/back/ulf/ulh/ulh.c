/*
**Copyright (c) 2004 Ingres Corporation
**
*/
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulh.h>
#include    <ulhctrl.h>

/**
**
**  Name: ULH.C - Hash Table Management Module
**
**  Description:
**      This file contains the entire source code for the Hash Table 
**      Management Module (ULH).  The entire module was written in C.
**	The corresponding object module is cataloged in the ULF.OLB
**	library.  The object module must be linked to the executable
**	image of a calling module.
**
**	Accompanying the ULH.C file are two header files, ULH.H which
**	contains constants and data structures needed to use ULH services,
**	and ULHCTRL.H which contains constants and data structures used
**	internally by ULH.
**
**	ULH allows generic named objects to be cataloged in its table.
**	A user can create as many hash table as needed.  A hash table has
**	no name and is referenced only by a special pointer, called the
**	TABLE IDENTIFIER.  Objects cataloged in a hash table is represented
**	by object header blocks.  Only a portion of the object header is
**	visible by users.  An object header block resides in its
**	individual memory stream by itself.  The memory stream id and a
**	nul pointer are kept in user-visible portion so that the user can
**	allocate memory for data portion of the object from the same memory
**	stream.
**
**	ULH objects are divided into two types: Destroyable objects, and
**	fixed objects.  A destroyable object is one that ULH can delete
**	from its hash table when space is needed.  A fixed object is one
**	that remains fixed in memory until explicitly destroyed by the
**	calling module.  A hash table may contain both types of objects.
**
**	ULH also provides quick access to a set of objects which are
**	related to each other in a certain way.  This set of objects is
**	called a class.  Membership in a class is defined by users.  ULH
**	provides functions to add/delete an objects to/from a class
**	and to retrieve a list of objects in a class.
**
**	ULH use the LRU replacement algorithm to reclaim space when a
**	new object must be added to a full table.  Only destroyable objects
**	participate in the LRU queue.
**
**	(05-apr-90 andre)
**	We will now support multiple aliases on an object.
**
**	ULH provides the following services:
**
**	    ULH_INIT	    - Initialize a hash table
**	    ULH_CLOSE	    - Close and Destroy a hash table
**	    ULH_CREATE	    - Create, or if already exists, obtain access
**			      to an object in a hash table.
**	    ULH_DESTROY	    - Remove an object from a hash table.
**	    ULH_CLEAN	    - Remove all objects from a hash table.
**	    ULH_RENAME	    - Rename an object.
**	    ULH_ACCESS	    - Obtain access to an object in a hash table.
**	    ULH_RELEASE	    - Relinquish access to an object.
**	    ULH_GETMEMBER   - Obtain access to a free member in a class.  If
**			      none is available, create one.
**	    ULH_INFO	    - Obtain information about a hash table.
**
**
**  History:    $Log-for RCS$
**      28-apr-86 (daved)    
**          comments and headers. initial design
**      08-jan-87 (puree)    
**          Modified comment to reflect changes in design.
**	    initial implementation.
**	11-may-87 (puree)
**	    Modified ulh_destroy to be able to destroy an object at the
**	    head of an LRU queue.  Required by OPF.
**	04-aug-87 (puree)
**	    set error code if no objects returned in ulh_getclass.
**	18-sep-87 (puree)
**	    Restructured semaphore handling to avoid deadlock with DMF. 
**	    Basically, ULH now acquires semaphores for internal house-
**	    keeping only.  Users of ULH will have to synchronize accesses to 
**	    ULH objects among themselves.
**	02-nov-87 (puree)
**	    Implemented hashing by alias.
**	04-nov-87 (puree)
**	    Modified ulh_create to create/access-if-exists.
**	30-nov-87 (puree)
**	    Remove the internal function waitsem and sigsem.  
**	    Converted SCF_SEMAPHORE's to CSx_semaphore's.
**	14-jan-88 (puree)
**	    Modified ulh_getclass to return only free objects.
**	18-jan-88 (puree)
**	    Clean up ULH.  Implement ulh_getmember.  Remove ulh_getclass,
**	    ulh_addmember, ulh_delmember, and ulh_mrelease.   Implement
**	    the internal routine ulhi_create_obj and prefix all internal
**	    routines with ulhi_.
**	27-feb-88 (puree)
**	    Check for a valid name length.
**	29-feb-88 (puree)
**	    Modified ulh_define_alias to check if the object alias is being
**	    redefined.  ULH cannot support multiple aliases yet.
**	01-mar-88 (puree)
**	    Added debug code in ulh_define_alias to print information about
**	    object alias.
**	03-oct-88 (puree)
**	    Report alias redefined in ulh_define_alias only if the new alias
**	    is different from the old one.
**	06-dec-88 (puree)
**	    Fix bug in ulh_getmember.  The class header disappears while the
**	    new object is being included to the class.
**	04-apr-90 (andre)
**	    Made changes to support multiple synonyms.
**	04-apr-90 (andre)
**	    wrote ulhi_create_alias(), ulhi_mem_for_alias(),
**	    ulhi_remove_aliases().
**	    Changed interface for ulh_init.
**      14-aug-91 (jrb)
**          General cleaning up: use CSi_semaphore to init semaphores.  Also,
**          adjusted #includes and some external function declarations to
**          reflect reality.
**	12-mar-92 (teresa)
**	    implemented ulh_cre_class and ulh_dest_class to put back basic
**	    class functionality that seems to have disappeared from ULH. 
**	    ulh_cre_class - returns a pointer to an existing object in class
**			    or creates an empty object and puts it in the class.
**			    (If the class does not already exist, it also
**			    creates the class).
**	    ulh_dest_class - finds a class and destroys all objects in the
**			    class, then destroys the class.  This is the
**			    equivalent of doing a ulh_destroy for every member
**			    in the class.
**	31-aug-1992 (rog)
**	    Prototype ULF.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in ulh_init().
**	17-dec-1993 (rog)
**	    Add correct casts to ME calls to placate the compiler.
**	19-apr-1994 (andyw) Cross Integ from 6.4 (ramra01)
**	    Corrections to calls on ulhi_yankq() without checking for
**	    a valid foreward reference pointer within the circular scope.
**	    The affected functions are ulh_destroy(), ulh_clean(),
**	    ulh_rename(), ulh_getmember(), ulhi_rm_member().
**	01-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	17-Nov-1995 (reijo01)
**	    Added a CSr_semaphore to remove a tables's semaphore and 
**	    an object's semaphore and thus the semaphore's instance string
**	    from the MO_strings table when the object and/or table is destroyed.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm can
**	    destroy those handles when the memory is freed.
**	    Dinstinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where
**	    appropriate.
**	06-aug-1998 (nanpr01) 
**	    Backed out the change for andyw. If we get a segv from ulh,
**	    we must check the caller. Caller must be using a stale pointer.
**	    Checking for null pointer to prevent the segv is bad idea.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Sep-2008 (jonj)
**	    SIR 120874: Modify to use CLRDBERR, SETDBERR to value DB_ERROR.
**	    Deleted unneeded ulhi_chk_status() static function.
*/

/*
**  External Functions.
*/
FUNC_EXTERN DB_STATUS	ulh_openstream(); /* ULM services */

/*
**  Forward function references.
*/
static DB_STATUS    ulhi_create_obj( ULH_RCB *ulh_rcb, ULH_BUCKET *bp,
				     unsigned char *obj_name, i4  name_len,
				     i4  obj_type, i4 block_size );
static VOID	    ulhi_remove_aliases( ULH_QUEUE *alias_queue,
					 i4  *alias_count );
static DB_STATUS    ulhi_mem_for_alias( ULH_RCB *ulh_rcb, ULM_RCB *ulm_rcb,
					ULH_HOBJ *alias_obj );
static DB_STATUS    ulhi_create_alias( ULH_RCB *ulh_rcb, ULH_BUCKET *ap,
				       unsigned char *alias_name, i4  name_len,
				       ULH_HOBJ *alias_obj );
static i4	    ulhi_release_obj( ULH_RCB *ulh_rcb, ULH_TCB *tcb,
				      ULH_HOBJ *objp );
static VOID	    ulhi_destroy_obj( ULH_TCB *tcb, ULH_HOBJ *objp,
				      ULM_RCB *ulm_rcb );
static VOID	    ulhi_rm_member( ULH_TCB *tcb, ULH_HOBJ *objp );
static VOID	    ulhi_enqueue( ULH_QUEUE *queue, ULH_LINK *element );
static PTR	    ulhi_dequeue( ULH_QUEUE *queue );
static VOID	    ulhi_yankq( ULH_LINK *element );
static PTR	    ulhi_search( ULH_BUCKET *bp, unsigned char *obj_name,
				 i4  name_len );
static PTR	    ulhi_alias_search( ULH_BUCKET *ap,
				       unsigned char *alias_name,
				       i4  alias_len );
static i4	    ulhi_hash( unsigned char *obj_name, i4  name_len,
			       i4  tabsize );
static i4	    ulhi_notprime( i4  i );
static DB_STATUS    ulhi_alloc( ULH_RCB *ulh_rcb, ULM_RCB *ulm_rcb,
				i4 errcode, i4  flag );
/*
**  Defines of other constants.
*/
#define objptr		ulh_object->ulh_id
#define	objectname	ulh_obj.ulh_hname
#define	obj_namelen	ulh_obj.ulh_namelen
#define obj_streamid	ulh_streamid

#define classname	cl_unit.ulh_hname
#define cl_namelen	cl_unit.ulh_namelen
#define cl_sem		ulh_cp->cl_bptr->b_sem



/*{
** Name: ULH_INIT	- Initialize a Hash Table
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                i4             facility;
**                PTR            pool_id;
**                i4        *memleft;
**                i4             max_obj, max_class, density;
**		  i4		 info_mask;
**		  i4	 max_alias;
**
**	status = ulh_init(ulh_rcb, facility, pool_id, memleft, max_obj,
**			  max_class, density, info_mask, max_alias);
**
** Description:
**                This function is used to set up and initialize  a  hash
**                table.   A  memory  stream  is  opened in the specified
**                memory pool.  An object table and  the  internal  table
**                control  block are allocated in this stream.  The table
**                size is computed below:
**
**                        table size = (max_obj/density) round up to  the
**                next prime number
**
**		  If aliases will be allowed (info_mask & ULH_ALIAS_OK),
**		  the alias table will also be allocated.  Number of 
**		  aliases will have to be specified by the caller.
**		  The alias table size will be computed as
**
**		    (max_alias/density) rounded up to the next prime number
**
**                If max_class is non-zero, signifying that the table  is
**                to  contain classes of objects, the class table and the
**                class headers are also allocated at this time from  the
**                same  memory  stream.  All the class headers are linked
**                to a free list (LIFO queue).  There are as many headers
**                as  there  are  classes  in the table.  The size of the
**                class table itself is computed from max_class with  the
**                same formula as for the object table.
**
**		  This routine must be called in a single thread manner.
**
** Inputs:
**                ulh_rcb             the pointer  to  an  empty  ULH_RCB
**                                    block.   The  ULH_RCB  block itself
**                                    must be in user's data space.
**                facility            facility code used for memory allo-
**                                    cation.
**                pool_id             memory pool  id.  used  for  memory
**                                    allocation.
**                memleft             pointer to memory counter. used for
**                                    memory allocation.
**                max_object          maximum number of  objects  in  the
**                                    table.   It  also implies the table
**                                    size.
**                max_class           maximum  number   of   classes   of
**                                    objects  in  this  table.  If zero,
**                                    the table will contain no classes.
**		  density	      table density factor.  A nominal 
**				      number of objects that collide into
**				      the same bucket.
**		  info_mask	      bit mask for special info
**			ULH_ALIAS_OK	    aliases allowed
**
**		  max_alias	      maximum number of aliases
**				      (meaningful only if aliases are allowed)
**
** Outputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash  table.
**                                    This  id is used for all subsequent
**                                    references to the table.
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0102_TABLE_SIZE	    Invalid ULH table size specified.
**                E_UL010D_TABLE_MEM	    Error in opening memory stream  for
**					    the hash table.
**                E_UL010E_TCB_MEM	    Error in allocating memory for  the
**					    table control block.
**                E_UL010F_BUCKET_MEM	    Error in allocating memory for hash
**					    buckets.
**                E_UL0110_CHDR_MEM	    Error  in  allocating  memory   for
**					    class headers.
**                E_UL0112_INIT_SEM	    Semaphore initialization failed.
**		  E_UL0118_NUM_ALIASES	    Invalid number of aliases.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	30-mar-87 (puree)
**	    implement semaphore queue for object semaphores
**	18-sep-87 (puree)
**	    remove all but table semaphores.
**	02-nov-87 (puree)
**	    added alias support.
**	04-apr-90 (andre)
**	    when looking for a prime number, there is no reason to check even
**	    numbers.
**	04-apr-90 (andre)
**	    Change interface: get rid of "alias", add "info_mask" and
**	    "num_alias".
**      14-aug-91 (jrb)
**          Use CSi_semaphore to initialize semaphores.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to tcb_owner which has changed
**	    type to PTR.
**	7-oct-2004 (thaju02)
**	    Change memleft to SIZE_TYPE.
*/
DB_STATUS
ulh_init( ULH_RCB *ulh_rcb, i4  facility, PTR pool_id, SIZE_TYPE *memleft,
	  i4  max_obj, i4  max_class, i4  density, i4  info_mask,
	  i4 max_alias )
{

    register ULH_TCB	*tcb;	    /* hash table control block */
    register ULH_BUCKET *bp, *ap;   /* hash and alias bucket pointer */
    register ULH_CHDR	*chdr, *nexthdr;    /* class header pointers */
    DB_STATUS	status;
    ULM_RCB	ulm_rcb;	    /* temp control block for ULM */
    i4		i;
    char	sem_name[ CS_SEM_NAME_LEN ];

    CLRDBERR(&ulh_rcb->ulh_error);
    
    /*
    **  Validate input arguments
    */
    ulh_rcb->ulh_hashid = (PTR) NULL;		/* zero out hash table id */
    if (max_obj == 0)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0102_TABLE_SIZE);
	return(E_DB_ERROR);
    }

    /*
    ** if aliases are allowed, check if the specified number is valid
    */
    if (info_mask & ULH_ALIAS_OK && max_alias <= 0)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0118_NUM_ALIASES);
	return(E_DB_ERROR);
    }

    /*
    **  Open a private memory stream to be used for the hash table.
    */
    ulm_rcb.ulm_facility = facility;
    ulm_rcb.ulm_poolid = pool_id;
    ulm_rcb.ulm_blocksize = (i4)0;		/* default block size */
    ulm_rcb.ulm_memleft = memleft;
    ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;

    /* Open streamid and allocate ULH_TCB with one effort */
    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(ULH_TCB);

    if (ulm_openstream(&ulm_rcb) != E_DB_OK)
    {
	SETDBERR(&ulh_rcb->ulh_error, ulm_rcb.ulm_error.err_code, E_UL010D_TABLE_MEM);
	return(E_DB_FATAL);			/* cannot open table stream */
    }
    
    tcb = (ULH_TCB *)(ulm_rcb.ulm_pptr);	/* set ULH_TCB pointer */

    /*
    **  Fill in standard server fields in TCB
    */
    tcb->next_tcb = (ULH_TCB *)NULL;
    tcb->prev_tcb = (ULH_TCB *)NULL;
    tcb->tcb_length = sizeof(ULH_TCB);
    tcb->tcb_type = ULH_TYPE;
    tcb->tcb_owner = (PTR)ULH_OWNER;
    tcb->tcb_ascii_id = (i4)ULH_ID;
    tcb->ulh_tcb = tcb;
    tcb->ulh_facility = facility;
    tcb->ulh_poolid = pool_id;
    tcb->ulh_streamid = ulm_rcb.ulm_streamid;
    tcb->ulh_memleft = memleft;
    /*
    **  Initialize table's semaphore.
    */
    if (CSw_semaphore(&tcb->ulh_sem, CS_SEM_SINGLE,
    			STprintf( sem_name, "ULH TCB %p", tcb )) != OK)
    {
    	(VOID) ulm_closestream(&ulm_rcb);
	return(E_DB_FATAL);	    /* semaphore initialization failed */
    }

    /*
    **  Round up size of object table to the next prime number and
    **  allocate memory for the table.
    */
    tcb->ulh_osize = max_obj/density;
    if (tcb->ulh_osize > 3)
    {
	/* no need to check if an even number is a prime */
	if (tcb->ulh_osize % 2 == 0)
	    ++tcb->ulh_osize;
        while (ulhi_notprime((tcb->ulh_osize += 2)))
	;
    }

    ulm_rcb.ulm_psize = tcb->ulh_osize * sizeof(ULH_BUCKET);
    if ((status = ulhi_alloc(ulh_rcb, &ulm_rcb, E_UL010F_BUCKET_MEM, 1)) !=
	    E_DB_OK)
	return(status);	/* table allocation failed */
    bp = tcb->ulh_otab = (ULH_BUCKET *)(ulm_rcb.ulm_pptr);
    /*
    **  Initiailize the object queue for each bucket in the
    **  object table.
    */
    for ( i = 0; i < tcb->ulh_osize; ++i)
    {
	bp->b_queue.head = (ULH_LINK *) &bp->b_queue;
	bp->b_queue.tail = (ULH_LINK *) &bp->b_queue;
	bp->b_count = 0;
	++bp;
    }
    /*
    **  Alocate space for alias table if required.
    */
    if (info_mask & ULH_ALIAS_OK)
    {
	/*
	**  Round up size of alias table to the next prime number and
	**  allocate memory for the table.
	*/
	tcb->ulh_asize = max_alias / density;
	if (tcb->ulh_asize > 3)
	{
	    /* no need to check if an even number is a prime */
	    if (tcb->ulh_asize % 2 == 0)
		++tcb->ulh_asize;
	    while (ulhi_notprime((tcb->ulh_asize += 2)))
	    ;
	}

	ulm_rcb.ulm_psize = tcb->ulh_asize * sizeof(ULH_BUCKET);
	if ((status = ulhi_alloc(ulh_rcb, &ulm_rcb, E_UL010F_BUCKET_MEM, 1)) !=
		E_DB_OK)
	    return(status);	/* table allocation failed */
	ap = tcb->ulh_alstab = (ULH_BUCKET *)(ulm_rcb.ulm_pptr);
	/*
	**  Initiailize the object queue for each bucket in the
	**  object table.
	*/
	for ( i = 0; i < tcb->ulh_asize; ++i)
	{
	    ap->b_queue.head = (ULH_LINK *) &ap->b_queue;
	    ap->b_queue.tail = (ULH_LINK *) &ap->b_queue;
	    ap->b_count = 0;
	    ++ap;
	}
    }
    else
    {
	max_alias = 0;	    /* aliases are not allowed */
	tcb->ulh_alstab = (ULH_BUCKET *)NULL;
    }
    /*
    **  If max_class is specified, allocate the class table in the same way
    **  as for the object table.
    */
    if (max_class > 0)
    {
	tcb->ulh_csize = max_class/density;
	if (tcb->ulh_csize > 3)
	{
	    /* no need to check if an even number is a prime */
	    if (tcb->ulh_csize % 2 == 0)
		++tcb->ulh_csize;
            while (ulhi_notprime((tcb->ulh_csize += 2)))
	    ;
	}

        ulm_rcb.ulm_psize = tcb->ulh_csize * sizeof(ULH_BUCKET);
	if ((status = ulhi_alloc(ulh_rcb, &ulm_rcb, E_UL010F_BUCKET_MEM, 1)) !=
		E_DB_OK)
	    return(status);	/* class table allocation failed */
        bp = tcb->ulh_ctab = (ULH_BUCKET *)(ulm_rcb.ulm_pptr);
        /*
        **  Initiailize the class header queue for each bucket
	**  in the class table.
        */
        for ( i = 0; i < tcb->ulh_csize; ++i)
        {
		bp->b_queue.head = (ULH_LINK *) &bp->b_queue;
		bp->b_queue.tail = (ULH_LINK *) &bp->b_queue;
		bp->b_count = 0;
		++bp;
        }
	/*
	**  Allocate memory for class header blocks.
	*/
        ulm_rcb.ulm_psize = max_class * sizeof(ULH_CHDR);
	if ((status = ulhi_alloc(ulh_rcb, &ulm_rcb, E_UL0110_CHDR_MEM, 1)) !=
		E_DB_OK)
	    return(E_DB_FATAL);	/* class header allocation failed */
        chdr = tcb->ulh_chq = (ULH_CHDR *)(ulm_rcb.ulm_pptr);
	/*
	**  Link the class header block together.
	*/
	for (i = 0; i < max_class; ++i)
	{
	    chdr->cl_queue.head = (ULH_LINK *)&chdr->cl_queue.head;
	    chdr->cl_queue.tail = (ULH_LINK *)&chdr->cl_queue.head;
	    chdr->cl_count = 0;
	    chdr->cl_unit.ulh_link.object = (PTR) chdr;
	    nexthdr = chdr + 1;
	    chdr->cl_unit.ulh_link.next = (ULH_LINK *)nexthdr;	
	    chdr = nexthdr;
	}
	--chdr;
	chdr->cl_unit.ulh_link.next = (ULH_LINK *) NULL;
    }
    else
    {
	tcb->ulh_chq = (ULH_CHDR *)NULL;
    }    
    /*
    **  Initialize the rest of fields in ULH_TCB structure
    */
    tcb->ulh_lru1.head = (ULH_LINK *) &tcb->ulh_lru1;
    tcb->ulh_lru1.tail = (ULH_LINK *) &tcb->ulh_lru1;
    tcb->lru1_limit = max_obj/10;
    tcb->ulh_lru2.head = (ULH_LINK *) &tcb->ulh_lru2;
    tcb->ulh_lru2.tail = (ULH_LINK *) &tcb->ulh_lru2;

    tcb->lru_cnt[0] = 0;
    tcb->lru_cnt[1] = 0;
    tcb->ulh_maxobj = max_obj;
    tcb->ulh_objcnt = 0;

    tcb->ulh_maxalias = max_alias;	/* maximum number of aliases */
    tcb->ulh_alias_count = 0;
    tcb->ulh_fixcnt = 0;
    tcb->ulh_maxclass = max_class;
    tcb->ulh_ccnt = 0;
    ulh_rcb->ulh_hashid = (PTR) tcb;	/* return the ptr to calling routine */

#ifdef xDEV_TEST
    tcb->ulh_max_objcnt = 0;
    tcb->ulh_b_avg = 0;
    tcb->ulh_b_max = 0;
    tcb->ulh_requests = 0;
    tcb->ulh_hits = 0;
    tcb->ulh_o_created = 0;
    tcb->ulh_o_destroyed = 0;
#endif /* xDEV_TEST */    

    return(E_DB_OK);
}

/*{
** Name: ULH_CLOSE	- Close and Destroy a Hash Table
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**
**                        status = ulh_close(ulh_rcb);
**
** Description:
**                The hash table specified by the ulh_rcb is removed from
**                the  system.   The  memory  stream  associated with the
**                table is closed.
**
**                The table  is  scanned sequentially  for  fixed/in-used
**                objects. If found, the function aborts and the table is
**                left intact.  Otherwise, objects will be  removed  from
**                the table and their memory stream closed.  Finally, the
**                table itself is closed (by closing  the  memory  stream
**                where the table resides).
**
**		  This routine must be called in a single thread manner.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the  hash  table
**                                    to  be  closed.   It must be the id
**                                    assigned by ULH when the table  was
**                                    created.
**
** Outputs:
**      none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0103_NOT_EMPTY  Table  contains  fixed  or   locked
**                                    objects.
**                E_UL0113_TABLE_SEM  Error in locking  the  table  sema-
**                                    phore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	27-mar-87 (puree)
**	    modify algorithm for faster execution.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	20-Nov-1995 (reijo01)
**	    Added a CSr_semaphore to remove a tables's semaphore and 
**	    an object's semaphore and thus the semaphore's instance string
**	    from the MO_strings table when the object and/or table is destroyed.
*/
DB_STATUS
ulh_close( ULH_RCB *ulh_rcb )
{
    register ULH_HOBJ	*objp;
    register ULH_TCB	*tcb;
    ULM_RCB		ulm_rcb;	    /* temp control block for ULM */

    CLRDBERR(&ulh_rcb->ulh_error);

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }

#ifdef	xDEV_TEST
    /*
    **	Report table statistic.
    */
    TRdisplay("\nULH Table Statistics:  Owner - ");
    switch (tcb->ulh_facility)
    {
	case DB_PSF_ID:
	    TRdisplay("PSF\n");
	    break;
	case DB_OPF_ID:
	    TRdisplay("OPF\n");
	    break;
	case DB_QEF_ID:
	    TRdisplay("QEF\n");
	    break;
	default:
	    TRdisplay("%d\n", tcb->ulh_facility);
	    break;
    }

    TRdisplay("\n\t    buckets  max.obj max.obj/bkt created  destroyed  search \
   hits   \n");
    TRdisplay("\t%8d %8d %10d %8d %9d %8d %8d\n",
	tcb->ulh_osize,
	tcb->ulh_max_objcnt,
	tcb->ulh_b_max,
	tcb->ulh_o_created,
	tcb->ulh_o_destroyed,
	tcb->ulh_requests,
	tcb->ulh_hits);
    /*
    **	Check that all objects are free or destroyed.
    */
    if (tcb->ulh_objcnt != (tcb->lru_cnt[0] + tcb->lru_cnt[1]))
    {
	TRdisplay("\t %d objects not released.\n\n",
	    (tcb->ulh_objcnt - tcb->lru_cnt[0] - tcb->lru_cnt[1]));
    }
#endif /* xDEV_TEST */
    /*
    **  Remove objects from the LRU queues and close their memory streams.
    */
    tcb->ulh_tcb = (ULH_TCB *) NULL;		/* invalidate self pointer */
    ulh_rcb->ulh_hashid = (PTR) NULL;		/* invalidate user's pointer */
    ulm_rcb.ulm_facility = tcb->ulh_facility;	/* set up ULM request block */
    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
    ulm_rcb.ulm_memleft = tcb->ulh_memleft;

    while (tcb->lru_cnt[0])
    {
	objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1); 
						    /* remove obj from LRU1 */
	--(tcb->lru_cnt[0]);			    /* adjust LRU1 count */
	CSr_semaphore(&objp->ulh_usem);		    /* remove semaphore   */
	ulm_rcb.ulm_streamid_p = &objp->obj_streamid;  /* destroy the object */
	(VOID) ulm_closestream(&ulm_rcb);
    }

    while (tcb->lru_cnt[1])
    {
	objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2); 
						    /* remove obj from LRU2 */
	--(tcb->lru_cnt[1]);			    /* adjust LRU2 count */
	CSr_semaphore(&objp->ulh_usem);		    /* remove semaphore   */
	ulm_rcb.ulm_streamid_p = &objp->obj_streamid;  /* destroy the object */
	(VOID) ulm_closestream(&ulm_rcb);
    }

    /*
    **  Remove table's semaphore.
    */
    CSr_semaphore(&tcb->ulh_sem);

    /*
    **  Now, destroy the table.
    */
    ulm_rcb.ulm_streamid_p = &tcb->ulh_streamid;
    (VOID) ulm_closestream(&ulm_rcb);		    /* destroy the hash table */

    return(E_DB_OK);
}

/*{
** Name: ULH_CREATE	- Add an object to a hash table.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *obj_name;
**                i4             name_len;
**                i4             obj_type;
**                i4        block_size;
**
**     status = ulh_create(ulh_rcb, obj_name, name_len, obj_type, block_size);
**
** Description:   The hash table is searched for an the existing object.
**		  If found, the object is returned.  If not found, a new
**                object is created and added to the  hash table.  A new
**                memory stream is opened with the allocation block  size
**                as  specified.  The memory stream id is returned to the
**                user in ulh_rcb.ulh_object->ulh_streamid.   The  object
**                header  itself  is  also  allocated  from  this  memory
**                stream.
**
**                If the max object count of the table is reached prior to
**		  creating the new object, or not enough space available
**		  in the memory pool, an unused object at the head of the 
**		  LRU queue will be destroyed.  This process repeats until
**		  there is enough memory space, or all the unused objects
**		  are destroyed.  If there still is not enough memory, an
**		  error is returned.
**
**                If the space is found, the  memory  stream  is opened
**		  and  the object header block is allocated.  The
**                object name is hashed to locate  the  appropriate  hash
**                bucket.  The object is then appended to the tail of the
**                list in the bucket.  Appropriate counters are  adjusted
**                accordingly.  A semaphore is initialized in the object
**		  header to be used by the users for synchronizing accesses
**		  to the user-visible part of the object.  The usage count 
**		  will be initialize to one indicating that it is being 
**		  referenced by the current user.  An object becomes free
**		  when its usage count becomes zero through ulh_release
**		  function calls.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                obj_name            The  pointer  to  the  object  name
**                                    string.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.
**                obj_type            Type  of  object  to  be   created:
**                                    ULH_FIXED | ULH_DESTROYABLE.
**                block_size          Used to  set  the  preferred  block
**                                    size  of  allocation  unit  in  the
**                                    memory stream for the object.
**
** Outputs:
**                ulh_rcb
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object just created.
**                ULH_OBJECT
**                    .ulh_objname    The character array containing  the
**                                    object name.
**                    .ulh_namelen    An integer indicating the length of
**                                    the name.
**                    .ulh_streamid   The id of the memory stream for the
**                                    object.
**		      .ulh_uptr	      NULL if the object is newly created.
**				      User's pointer, otherwise.
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0104_FULL_TABLE No more space in the table.
**                E_UL0111_OBJ_MEM    Error in allocating memory for  the
**                                    object.
**                E_UL0112_INIT_SEM   Semaphore initialization failed.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**                If the table is full at the time a new object is to  be
**                created,  an object at the head of an LRU queue will be
**                destroyed.  All user data  stored  in  the  same  meory
**                stream with the destroyed object are lost.
**
**                The object that is successfully created becomes "in-use"
**		  and has the usage count of 1.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	25-mar-87 (puree)
**	    check for the object was destroyed (no longer in a bucket linked
**	    list) before removing an object from its bucket.
**	30-mar-87 (puree)
**	    implement semaphore queue for object semaphores.
**	07-may-87 (puree)
**	    destroy an unused object if out of memory.
**	09-jul-87 (puree)
**	    also acquire the semaphore for the object in exclusive mode.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	02-nov-87 (puree)
**	    alias support.
**	04-nov-87 (puree)
**	    Implement create/access-if-exists approach.  If the object to be 
**	    created already exist in cache, access to the object is granted
**	    to the client.
**	18-jan-88 (puree)
**	    Implement ulhi_create_obj.
**	27-feb-88 (puree)
**	    Check for valid name length.
*/
DB_STATUS
ulh_create( ULH_RCB *ulh_rcb, unsigned char *obj_name, i4  name_len,
	    i4  obj_type, i4 block_size )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*bp;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (name_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */
    bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);
						/* compute hash bucket ptr */
    /*
    **  Lock the cache table
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **  Check for an existing object.  If found one, return it to the caller.
    */
    if ((objp = (ULH_HOBJ *)ulhi_search(bp, obj_name, name_len)) 
	    != (ULH_HOBJ *)NULL)
    {						/* duplicated name */
	/*
	**	Check object in-use status
	*/
	if (objp->ulh_ucount == 0)  /* object is currently not in-use */
	{
	    if (objp->ulh_llink.next)		/* if object in LRU */
	    {
		ulhi_yankq(&objp->ulh_llink);	/* remove object from LRU */
		--(tcb->lru_cnt[objp->ulh_lindx]);/* update LRU count */
		objp->ulh_flags = ULH_USED;	/* mark it multiple uses */
	    }
	}
	/*
	**	Adjust user count
	*/
	++objp->ulh_ucount;
#ifdef	xDEV_TEST
	++tcb->ulh_requests;		/* update count of cache search */
	++tcb->ulh_hits;		/* update count of hits */
#endif /* xDEV_TEST */
	/*
	**	Return object to the user
	*/
	ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;
	CSv_semaphore(&tcb->ulh_sem);
	return(E_DB_OK);
    }
    /*
    **	Create a brand new object.
    **  If the table is full of fixed objects, release the semaphore 
    **  and return error code.
    */
    status = ulhi_create_obj(ulh_rcb, bp, obj_name, name_len, obj_type,
		 block_size);
    if (status != E_DB_OK)
    {	/* error, err_code already set by ulhi_create_obj */
	CSv_semaphore(&tcb->ulh_sem);
	return(E_DB_ERROR);
    }

    CSv_semaphore(&tcb->ulh_sem);     /* release table semaphore */
    return(E_DB_OK);
}

/*{
** Name: ULH_DESTROY	- Remove an object from a hash table.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**		  unsigned char  *obj_name;
**		  i4		 name_len;
**
**		    status = ulh_destroy(ulh_rcb, obj_name, name_len);
**
** Description:
**                The specified object is removed from the hash table.
**                The object becomes inaccessible to all subsequent
**		  references.  If the object is fixed, it is converted
**                into a destroyable one.   If the object belongs to the
**                LRU and/or the class lists, it will be appropriately
**                removed from such lists. The object itself, however, is
**                not destroyrd until the reference count becomes zero.
**                When the object is actually destroyed, Its memory stream
**                is closed and hence, any user's data block allocated on
**		  this stream will be lost. 
**
**		  If the user already has access to the object, the
**                ulh_rcb.ulh_object must be set to the object.  In this
**                case, the obj_name and namelen input arguments are
**                ignored by ULH. This is a prefered method as no hashing
**                is needed.  However, a user can request an object to be
**                destroyed by name by by setting the ulh_rcb.ulh_object
**                field to a null and filling in the object name and
**                length fields.  This method is reserved for the users
**                who do not have access to the object only. 
**
**		  The calling routine can use this function to reclaim
**		  space from ULH cache by requesting that an unused object
**		  at the head of an LRU queue be destroyed.  This can be
**		  done by setting both the ulh_object ptr and the object
**		  name ptr to null.  If such an object is found, it will
**		  be immediately destroyed.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object to be destroyed. Required if
**				      the user already have access to the
**				      object.  If null, the object at the
**				      head of an LRU queue will be destroyed.
**                obj_name            The  pointer  to  the  object  name
**                                    string.  Required if the user does
**				      not have access to the object.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.  Required if the
**				      object is specified by name.
**
** Outputs:
**                none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0106_OBJECT_ID  Invalid object pointer.
**                E_UL0107_ACCESS     Invalid access to the object.
**		  E_UL0109_NFND	      Object not found.  If requested to
**				      destroy an object in LRU, this means
**				      there is no objects in the LRU queues.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**                E_UL0116_LRU_SEM    Error in locking  LRU  queue  sema-
**                                    phore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**                All user data stored in the same memory stream with the
**                destroyed object are lost.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	25-mar-87 (puree)
**	    check for a destroy-pending object.
**	30-mar-87 (puree)
**	    immediately destroy an object that is not in-use.
**	28-may-87 (puree)
**          return ok status for a request to destroy a non-existent object.
**	18-sep-87 (puree)
**	    Lock the table semaphore before doing anything.
**	27-feb-88 (puree)
**	    Check for a valid name length.
**	04-apr-90 (andre)
**	    validate name_len only if it will be used, i.e.
**	    ulh_rcb->ulh_object == NULL && obj_name != NULL.
**	    Also, since an object may have more than 1 alias, the algorithm for
**	    removing object's alias(es) has changed.
**	06-aug-1998 (stial01 & nanpr01)
**	    Concurrent threads must decrement the ucount before 
**	    exiting ulh_destroy.
*/
DB_STATUS
ulh_destroy( ULH_RCB *ulh_rcb, unsigned char *obj_name, i4  name_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*bp;
    DB_STATUS		status;
    STATUS		cs_status;
    ULM_RCB		ulm_rcb;

    CLRDBERR(&ulh_rcb->ulh_error);

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Partially set up ULM control blocks
    */
    ulm_rcb.ulm_facility = tcb->ulh_facility;
    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
    /*
    **  Lock the cache table
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    ** Request to destroy an unused object at the head of the LRU queue.
    */
    if (ulh_rcb->ulh_object == (ULH_OBJECT *)NULL && 
   	obj_name == (unsigned char *) NULL)
    {
	/*
	**  Remove an object from LRU1/LRU2 queue, if any.
	**  Then remove it from the object table bucket.
	**  Also, if the object belongs to a class, remove it from the class.
	*/
	if ((tcb->lru_cnt[1]) && (tcb->lru_cnt[0] <= tcb->lru1_limit))
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2);
						/* remove object from LRU2 */
	    --(tcb->lru_cnt[1]);		/* adjust LRU2 count */
	}
	else if (tcb->lru_cnt[0])
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1);
						/* remove object from LRU1 */
	    --(tcb->lru_cnt[0]);		/* adjust LRU1 count */
	}
	else
	{
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
	    return(E_DB_ERROR);
	}

	if (objp->ulh_clink.next)	    /* remove object from its class */
	    ulhi_rm_member(tcb, objp);

	ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

	if (objp->ulh_obj.ulh_link.next)    /* remove object from its hash */
	{				    /*  bucket */
	    ulhi_yankq(&objp->ulh_obj.ulh_link);
	    --objp->ulh_bp->b_count;	    /* adjust bucket count */
	}

	ulhi_destroy_obj(tcb, objp, &ulm_rcb);    /* destroy the object */

        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	return(E_DB_OK);
    }
    /*
    ** If requested by name, locate the object.
    */
    if (ulh_rcb->ulh_object == NULL)
    {
	if (name_len > ULH_MAXNAME)
	{
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	    return(E_DB_ERROR);
	}

	bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);
	/*
	**	Search the bucket list for exact match.
	*/
	if ((objp = (ULH_HOBJ *)ulhi_search(bp, obj_name, name_len))
		== (ULH_HOBJ *)NULL)
	{					/* object not found */
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    return(E_DB_OK);
	}

	if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
	{				    /* make it destroyable */
	    objp->ulh_type = ULH_DESTROYABLE;
	    --tcb->ulh_fixcnt;		    /*  also adjust fix count */
	}
    }
    else    /* request by id, must already have access to the object */
    {
	/*
	**  Validate object pointer
	*/
	if	((ulh_rcb->ulh_object == (ULH_OBJECT *) NULL) ||
		( (objp = (ULH_HOBJ *) ulh_rcb->objptr) == (ULH_HOBJ *)NULL ))
	{
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0106_OBJECT_ID);
	    return(E_DB_ERROR);
	}

	if (objp->ulh_obj.ulh_link.next == (ULH_LINK *) NULL)
	{
	    /*  Decrement user count */
	    if (objp->ulh_ucount)
	        --objp->ulh_ucount;

    	    /*
    	    **  If the object becomes free, destroy it immediately.
    	    */
    	    if (objp->ulh_ucount == 0)
    	    {
		if (objp->ulh_llink.next)	    /* if it is in LRU queue */
		{
	    	    ulhi_yankq(&objp->ulh_llink);	    /* remove it */
	    	    --(tcb->lru_cnt[objp->ulh_lindx]);
		}
		ulhi_destroy_obj(tcb, objp, &ulm_rcb);   /* destroy it */
    	    }
	    CSv_semaphore(&tcb->ulh_sem); 
	    return(E_DB_OK);
	}

	if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
	{				    /* make it destroyable */
	    objp->ulh_type = ULH_DESTROYABLE;
	    --tcb->ulh_fixcnt;		    /*  also adjust fix count */
	}

	/*  Decrement user count */

	if (objp->ulh_ucount)
	    --objp->ulh_ucount;
    }
    /*
    **	Remove the object from its hash bucket and class.
    */
    if (objp->ulh_clink.next)	    /* check if object in a class */
	    ulhi_rm_member(tcb, objp);	    /* remove object from its class */

    ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

    ulhi_yankq(&objp->ulh_obj.ulh_link); /* remove object from the bucket */
    --objp->ulh_bp->b_count;		    /* adjust bucket count */

    /*
    **  If the object becomes free, destroy it immediately.
    */
    if (objp->ulh_ucount == 0)
    {
	if (objp->ulh_llink.next)	    /* if it is in LRU queue */
	{
	    ulhi_yankq(&objp->ulh_llink);	    /* remove it */
	    --(tcb->lru_cnt[objp->ulh_lindx]);
	}
	ulhi_destroy_obj(tcb, objp, &ulm_rcb);   /* destroy it */
    }

    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}


/*{
** Name: ULH_CLEAN	- Remove all objects from a hash table.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**
**		    status = ulh_clean(ulh_rcb);
**
** Description:   All objects in the hash table are removed.  The objects
**	          become inaccessible to all subsequent references.  Fixed
**		  objects are converted to destroyable.  An object
**		  that belongs to the LRU and/or the class list will be
**		  removed from such lists.  The object itself, however,
**		  is not destroyed until the reference count becomes zero.
**                When the object is actually destroyed, Its memory stream 
**                is closed and hence, any user's data block allocated on
**                this stream will be lost. 
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**
** Outputs:
**                none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**                                    phore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**                All user data stored in the same memory stream with the
**                destroyed objects are lost.
**
** History:
**      02-mar-87 (puree)
**          initial version.
**	11-mar-87 (daved)
**	    set type to destroyable.
**	25-mar-87 (puree)
**	    check for a destroy-pending object.
**	30-mar-87 (puree)
**	    immediately destroy an object that is not in-use.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	04-apr-90 (andre)
**	    since an object may now have more than 1 alias, the algorithm for
**	    removing object's alias(es) was changed.
**	19-apr-1994 (andyw) Cross Integ from 6.4 (ramra01)
**	    Check that the ULH_HOBJ.ulh_obj.ulh_link has a forewardpointer
**	    before calling ulhi_yankq() and reduce the bucket count.
**	06-aug-1998 (nanpr01) 
**	    Backed out the change for andyw. If we get a segv from ulh,
**	    we must check the caller. Caller must be using a stale pointer.
**	    Checking for null pointer to prevent the segv is bad idea.
*/
DB_STATUS
ulh_clean( ULH_RCB *ulh_rcb )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*bp;
    register ULH_LINK	*linkp;
    DB_STATUS		status;
    STATUS		cs_status;
    i4			count;
    i4                  i, j;
    ULM_RCB		ulm_rcb;

    CLRDBERR(&ulh_rcb->ulh_error);

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Lock the cache table
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **  Scan the entire table, removing objects from their buckets.
    */
    bp = tcb->ulh_otab;			    /* start from the top */
    for (i = 0; i < tcb->ulh_osize; ++i)	    /* scan entire table */
    {
	if ((count = bp->b_count) != 0)
	{
	    linkp = bp->b_queue.head;	    /* get the head of bucket chain */
	    for (j = 0; j < count; ++j)	    /* follow the bucket chain */
	    {
		objp = (ULH_HOBJ *)linkp->object;   /* get an object */

		if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
		{				    /* make it destroyable */
		    objp->ulh_type = ULH_DESTROYABLE;
		    --tcb->ulh_fixcnt;		    /*  also adjust fix count */
		}
		/*
		**	Remove the object from its bucket and class.
		*/
		linkp = linkp->next;

		if (objp->ulh_clink.next)
		    ulhi_rm_member(tcb, objp);

		ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

		ulhi_yankq(&objp->ulh_obj.ulh_link);
		--objp->ulh_bp->b_count;

		/*
		**  If the object is free, destroy it immediately
		*/

		if (objp->ulh_ucount == 0)
		{			
		    if (objp->ulh_llink.next)	/* remove object from LRU */
		    {
			ulhi_yankq(&objp->ulh_llink);
			--(tcb->lru_cnt[objp->ulh_lindx]);

		    }
		    ulm_rcb.ulm_facility = tcb->ulh_facility;
		    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
		    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
		    ulhi_destroy_obj(tcb, objp, &ulm_rcb);
		}
	    }
	}
	++bp;
    }
    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}


/*{
** Name: ULH_RENAME	- Rename an object.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *new_name;
**                i4             name_len;
**
**                status = ulh_rename(ulh_rcb, new_name, name_len);
**
** Description:
**                The table is searched for an object with the same name.
**                If found, the function is aborted and the table is left
**                intact.  If no duplication, the object (ULH part) is 
**		  given  a  new name  and is rehashed to a new bucket.
**		  The user-visible part and all other attributes of the 
**		  object are preserved.  The user must have accessto the
**		  object prior to issueing this functiona call. The object
**		  remains accessible to all current users.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object to be renamed.
**                new_name            The pointer to the new name string.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.
**
** Outputs:
**                none.
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0106_OBJECT_ID  Invalid object pointer.
**                E_UL0108_DUPLICATE  Duplicated object name.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**                                    phore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	25-mar-87 (puree)
**	    check for a destroy-pending object.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	27-feb-88 (puree)
**	    check for a valid name.
**	19-apr-1994 (andyw) Cross Integ from 6.4 (ramra01)
**	    Check that the ULH_HOBJ.ulh_obj.ulh_link has a forewardpointer
**	    before calling ulhi_yankq() and reduce the bucket count.
**	06-aug-1998 (nanpr01) 
**	    Backed out the change for andyw. If we get a segv from ulh,
**	    we must check the caller. Caller must be using a stale pointer.
**	    Checking for null pointer to prevent the segv is bad idea.
*/
DB_STATUS
ulh_rename( ULH_RCB *ulh_rcb, unsigned char *obj_name, i4  name_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*bp;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (name_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Validate object pointer
    */
    if	( (ulh_rcb->ulh_object == (ULH_OBJECT *) NULL) ||
	    ( (objp = (ULH_HOBJ *) ulh_rcb->objptr) == (ULH_HOBJ *)NULL ) )
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0106_OBJECT_ID);
	return(E_DB_ERROR);
    }
    bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);
					/* compute new hash bucket ptr */
    /*
    **  Lock the hash table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **  Check if the object was destroyed.
    */
    if (objp->ulh_obj.ulh_link.next == (ULH_LINK *) NULL)
    {						/* object was destroyed */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
	return(E_DB_ERROR);
    }
    /*
    **  Check for duplicated name.
    */
    if (ulhi_search(bp, obj_name, name_len) != (PTR)NULL)
    {						/* duplicated name, */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0108_DUPLICATE);
	return(E_DB_ERROR);
    }
    /*
    **	Remove the object from the old bucket.
    */
    ulhi_yankq(&objp->ulh_obj.ulh_link);/* remove object from old bucket */
    --objp->ulh_bp->b_count;		/* adjust old bucket count */

    /*
    **	Put object to the tail of the new bucket
    */
    ulhi_enqueue(&bp->b_queue, &objp->ulh_obj.ulh_link);
					/* put object in new bucket */
    objp->ulh_bp = bp;			/* store bucket back ptr */
    ++bp->b_count;			/* update new bucket count */
    /*
    **	Move new name into object header.
    */
    objp->obj_namelen = name_len;
    MEcopy((PTR)obj_name, name_len, &objp->objectname[0]);

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}

/*{
** Name: ULH_ACCESS	- Acquire accessibility to an object in a hash table.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *obj_name;
**                i4             name_len;
**
**         status = ulh_access(ulh_rcb, obj_name, name_len);
**
** Description:
**                The object name is hashed to locate  the  hash  bucket.
**                The  bucket  list is then searched to locate the object
**                with the exact name match.
**
**                If the object is not currently in used, implying that it
**		  must be in an LRU queue,  the object is removed from the
**		  LRU queue and marked as a multiple-use object.
**
**                When an object is made available to the user, its usage
**                count is updated  and  the  object becomes "in-use".
**                The object must be released  through the ulh_release or
**		  destroyed through the ulh_destroy function.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                obj_name            The  pointer  to  the  object  name
**                                    string.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.
** Outputs:
**                ulh_rcb
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object just created.
**                ULH_OBJECT
**                    .ulh_objname    The character array containing  the
**                                    object name.
**                    .ulh_namelen    An integer indicating the length of
**                                    the name.
**                    .ulh_streamid   The id of the memory stream for the
**                                    object.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0107_ACCESS     Invalid access to the object.
**                E_UL0109_NFND       Requested class/object not found in
**                                    the table.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	02-mar-87 (puree)
**	    implement wait option.
**	30-mar-87 (puree)
**	    implement semaphore queue for object semaphores.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	27-feb-88 (puree)
**	    check for a valid name length.
*/
DB_STATUS
ulh_access( ULH_RCB *ulh_rcb, unsigned char *obj_name, i4  name_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*bp;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (name_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */
    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
#ifdef	xDEV_TEST
    ++tcb->ulh_requests;	    /* update count of cache searches */
#endif /* xDEV_TEST */
    /* compute hash bucket ptr */

    bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);

    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **	Search the bucket list for exact match.
    */
    if ((objp = (ULH_HOBJ *)ulhi_search(bp, obj_name, name_len)) == (ULH_HOBJ *)NULL)
    {					/* object not found */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
	return(E_DB_ERROR);
    }
    /*
    **	Check object in-use status
    */
    if (objp->ulh_ucount == 0)  /* object is currently not in-use */
    {
	if (objp->ulh_llink.next)		/* if object in LRU */
	{
	    ulhi_yankq(&objp->ulh_llink);	/* remove object from LRU */
	    --(tcb->lru_cnt[objp->ulh_lindx]);	/* update LRU count */
	    objp->ulh_flags = ULH_USED;		/* mark it multiple uses */
	}
    }
    /*
    **	Adjust user count
    */
    ++objp->ulh_ucount;

#ifdef	xDEV_TEST
    ++tcb->ulh_hits;	    /* update count of hits */
#endif /* xDEV_TEST */

    ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}

/*{
** Name: ULH_RELEASE	- Relinquish usage on a single object.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**
**                        status = ulh_release(ulh_rcb);
**
** Description:
**                The user  count is  updated.  The object becomes a free 
**		  object if the user count becomes zero.  A destroyable 
**		  object that becomes free, joins the LRU queue.  If the
**		  object was accessed for the first time, it goes to the
**		  single-use  LRU  queue.  Otherwise, it will go to the 
**		  multiple-use LRU queue.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object to be released.
**
** Outputs:
**                none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0106_OBJECT_ID  Invalid object pointer.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**                A destroyable object that becomes free will be appended
**                to the tail of the appropriate LRU queue.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	30-mar-87 (puree)
**	    if a destroy-pending object becomes free, destroy it now.
**	18-sep-87 (puree)
**	    modify semaphore handling.
*/
DB_STATUS
ulh_release( ULH_RCB *ulh_rcb )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);


    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Validate object pointer
    */
    if	( (ulh_rcb->ulh_object == (ULH_OBJECT *) NULL) ||
	    ( (objp = (ULH_HOBJ *) ulh_rcb->objptr) == (ULH_HOBJ *)NULL ) )
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0106_OBJECT_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Lock the cache table
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */

    if (ulhi_release_obj(ulh_rcb, tcb, objp))	/* release object */
    {
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	return(E_DB_FATAL);
    }

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}


/*{
** Name: ULH_DEFINE_ALIAS	- Define an alias for an object.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *alias;
**                i4             alias_len;
**
**               status = ulh_define_alias(ulh_rcb, alias, alias_len);
**
** Description:
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object to be added to the specified
**                                    class.
**                alias		      The pointer to the alias name string.
**                alias_len           The length of the alias name, up to
**                                    ULH_MAXNAME.
**
** Outputs:
**      none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID		Invalid ULH table id.
**                E_UL0106_OBJECT_ID		Invalid object pointer.
**                E_UL0113_TABLE_SEM		Error in locking table
**						semaphore.
**		  E_UL0120_DUP_ALIAS		Alias already defined.
**		  E_UL0121_NO_ALIAS		No alias support.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-nov-87 (puree)
**	    written for alias support
**	27-feb-88 (puree)
**	    check for a valid name length.
**	29-feb-88 (puree)
**	    check for alias being redefined.
**	03-oct-88 (puree)
**	    report alias redefined only if it's different from the old one.
**	04-apr-90 (andre)
**	    use the newly created ulhi_create_alias() to create a new alias.
*/
DB_STATUS
ulh_define_alias( ULH_RCB *ulh_rcb, unsigned char *alias, i4  alias_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*alias_obj;
    register ULH_BUCKET	*ap;
    ULH_HOBJ		*objp;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (alias_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Error if the table does not support aliases.
    */
    if (tcb->ulh_alstab == (ULH_BUCKET *) NULL)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0121_NO_ALIAS);
	return(E_DB_ERROR);
    }

    /*
    **  Validate object pointer.
    */
    if	(ulh_rcb->ulh_object			    == (ULH_OBJECT *) NULL ||
	 (alias_obj = (ULH_HOBJ *) ulh_rcb->objptr) == (ULH_HOBJ *)NULL)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0106_OBJECT_ID);
	return(E_DB_ERROR);
    }

    ap = tcb->ulh_alstab + ulhi_hash(alias, alias_len, tcb->ulh_asize);
						/* compute alias bucket ptr */
    /*
    **  Lock the cache table
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }

    /*
    **  Check for duplicate alias.  If a duplicate alias has already been
    **	defined on this object, just return, otherwise, complain.
    */
    if ((objp = (ULH_HOBJ *) ulhi_alias_search(ap, alias, alias_len)) !=
        (ULH_HOBJ *) NULL)
    {						/* duplicated alias */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */

	/* if alias has been defined on the same object as now, return OK */
	if (objp == alias_obj)
	{
	    return(E_DB_OK);
	}
	else
	{
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0120_DUP_ALIAS);
	    return(E_DB_ERROR);
	}
    }
    
    /*
    ** Check if the object has a pending destroy.
    */
    if (alias_obj->ulh_obj.ulh_link.next == (ULH_LINK *)NULL)
    {					    /* object was destroyed */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	return(E_DB_OK);
    }

    /* now we create a new alias */
    status = ulhi_create_alias(ulh_rcb, ap, alias, alias_len, alias_obj);

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(status);
}


/*{
** Name: ULH_GETALIAS	- Obtain access to an object by its alias.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *alias;
**                i4             alias_len;
**
**               status = ulh_getalias(ulh_rcb, alias, alias_len);
**
** Description:
**                The object alias is hashed to locate  the  hash  bucket.
**                The  bucket  list is then searched to locate the object
**                with the exact alias.
**
**                If the object is not currently in used, implying that it
**		  must be in an LRU queue,  the object is removed from the
**		  LRU queue and marked as a multiple-use object.
**
**                When an object is made available to the user, its usage
**                count is updated  and  the  object becomes "in-use".
**                The object must be released  through the ulh_release or
**		  destroyed through the ulh_destroy function.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                alias		      The  pointer  to  the alias string.
**                alias_len           The length of the alias string,  up
**                                    to ULH_MAXNAME.
** Outputs:
**                ulh_rcb
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object just created.
**                ULH_OBJECT
**                    .ulh_objname    The character array containing  the
**                                    object name.
**                    .ulh_namelen    An integer indicating the length of
**                                    the name.
**                    .ulh_streamid   The id of the memory stream for the
**                                    object.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0107_ACCESS     Invalid access to the object.
**                E_UL0109_NFND       Requested class/object not found in
**                                    the table.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**		  E_UL0121_NO_ALIAS   The table does not support alias.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-nov-87 (puree)
**	    written for alias support
**	27-feb-88 (puree)
**	    check for a valid name length.
*/
DB_STATUS
ulh_getalias( ULH_RCB *ulh_rcb, unsigned char *alias, i4  alias_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_BUCKET	*ap;
    DB_STATUS		status;
    STATUS		cs_status;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (alias_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }

#ifdef	xDEV_TEST
    ++tcb->ulh_requests;		/* update count of cache search */
#endif /* xDEV_TEST */    
    /*
    **  Error if the table does not support aliases.
    */
    if (tcb->ulh_alstab == (ULH_BUCKET *) NULL)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0121_NO_ALIAS);
	return(E_DB_ERROR);
    }
    /* compute alias bucket ptr */

    ap = tcb->ulh_alstab + ulhi_hash(alias, alias_len, tcb->ulh_asize);

    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **	Search the alias bucket for exact match.
    */
    if ((objp = (ULH_HOBJ *)ulhi_alias_search(ap, alias, alias_len)) == 
		    (ULH_HOBJ *) NULL)	/* object not found */
    {					
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
	return(E_DB_ERROR);
    }
    /*
    **	Check object in-use status
    */
    if (objp->ulh_ucount == 0)  /* object is currently not in-use */
    {
	if (objp->ulh_llink.next)		/* if object in LRU */
	{
	    ulhi_yankq(&objp->ulh_llink);	/* remove object from LRU */
	    --(tcb->lru_cnt[objp->ulh_lindx]);	/* update LRU count */
	    objp->ulh_flags = ULH_USED;		/* mark it multiple uses */
	}
    }
    /*
    **	Adjust user count
    */
    ++objp->ulh_ucount;

#ifdef	xDEV_TEST
    ++tcb->ulh_hits;		    /* update count of hits */
#endif /* xDEV_TEST */

    ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);
}


/*{
** Name: ULH_GETMEMBER	- Obtain access to a free object in a class.  If none
**			  is available, create a new one.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *class_name;
**                i4             class_len;
**		  unsigned char	 *obj_name;
**		  i4		 name_len;
**		  i4		 obj_type;
**                i4        block_size;
**		
**           status = ulh_getmember(ulh_rcb, class_name, class_len, 
**			obj_name, name_len, obj_type, block_size);
**
** Description:
**                The list of objects in the specified class is searched
**                to  obtain an object that is  available to the user.
**		  In this function, an object is avaliable if it is not
**		  currently referenced by any user.
**
**                The object acquired through the ulh_getmember can be
**		  released through the ulh_release.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                class_name          The  pointer  to  the  class   name
**                                    string.
**                class_len            The length of the class name, up to
**                                    ULH_MAXNAME.
**                obj_name            The  pointer  to  the  object  name
**                                    string.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.
**                obj_type            Type  of  object  to  be   created:
**                                    ULH_FIXED | ULH_DESTROYABLE.
**                block_size          Used to  set  the  preferred  block
**                                    size  of  allocation  unit  in  the
**                                    memory stream for the object.
**
** Outputs:
**                ulh_rcb
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object just created.
**                ULH_OBJECT
**                    .ulh_objname    The character array containing  the
**                                    object name.
**                    .ulh_namelen    An integer indicating the length of
**                                    the name.
**                    .ulh_streamid   The id of the memory stream for the
**                                    object.
**		      .ulh_uptr	      NULL if the object is newly created.
**				      User's pointer, otherwise.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0104_FULL_TABLE No more space in the table.
**		  E_UL0105_FULL_CLASS No more space to make a new class.
**                E_UL0109_NFND       Requested class/object not found in
**                                    the table.
**                E_UL0111_OBJ_MEM    Error in allocating memory for  the
**                                    object.
**                E_UL0112_INIT_SEM   Semaphore initialization failed.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jan-88 (puree)
**	    Implemented.
**	27-feb-88 (puree)
**	    check for a valid  name length.
**	06-dec-88 (puree)
**	    fix bug for disappearing class header while the new object is
**	    being created.  When an existing object is not found in a class,
**	    a new object will be created and inserted into the class.  If
**	    there is memory shortage, one or more unused objects will be
**	    destroyed to yield space for the new one.  In the process, the
**	    target class header could be destroyed if the unused object 
**	    destroyed is the only member in the class.  Fixed by search for
**	    the class header again before inserting a new object into a class.
**	19-apr-1994 (andyw) Cross Integ from 6.4 (ramra01)
**	    Check that the ULH_HOBJ.ulh_obj.ulh_link has a forewardpointer
**	    before calling ulhi_yankq() and reduce the bucket count.
**	06-aug-1998 (nanpr01) 
**	    Backed out the change for andyw. If we get a segv from ulh,
**	    we must check the caller. Caller must be using a stale pointer.
**	    Checking for null pointer to prevent the segv is bad idea.
*/
DB_STATUS
ulh_getmember( ULH_RCB *ulh_rcb, unsigned char *class_name, i4  class_len,
	       unsigned char *obj_name, i4  name_len, i4  obj_type,
	       i4 block_size )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_CHDR	*chdr;
    register ULH_BUCKET	*cbp, *bp;
    DB_STATUS		status;
    STATUS		cs_status;
    i4			i;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (name_len > ULH_MAXNAME || class_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */

#ifdef	xDEV_TEST
    ++tcb->ulh_requests;			/* update count of requests */
#endif /* xDEV_TEST */

    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **	Search for the specified class.
    */
    cbp = tcb->ulh_ctab + ulhi_hash(class_name, class_len, tcb->ulh_csize);
    if ((chdr = (ULH_CHDR *)ulhi_search(cbp, class_name, class_len))
						 != (ULH_CHDR *)NULL)
    {
	/*
	**  The specified class already exists, get an object from it.
	*/
	objp = (ULH_HOBJ *)chdr->cl_queue.head->object;
	for (i = 0; i < chdr->cl_count; i++)
	{
	    if (objp->ulh_ucount == 0)		/* found a free object */
	    {
		if (objp->ulh_llink.next)	/* if object in LRU */
		{
		    ulhi_yankq(&objp->ulh_llink);   /* remove object from LRU */
		    --(tcb->lru_cnt[objp->ulh_lindx]);	/* update LRU count */
		    objp->ulh_flags = ULH_USED; /* mark it multiple uses */
		}
		ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;
		++objp->ulh_ucount;		/* adjust user count */
		break;				/* break the for loop */
	    }
	    objp = (ULH_HOBJ *)objp->ulh_clink.next->object;
					    /* get next object in the class */
	}				    /* end of for loop */

	/*  If an object is found, return */

	if (ulh_rcb->ulh_object != (ULH_OBJECT *) NULL)
	{
#ifdef	xDEV_TEST
	    ++tcb->ulh_hits;		    /* update count of hits */
#endif /* xDEV_TEST */
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    return(E_DB_OK);
	}
    }
    /*
    ** We come to here either because there is no such class, or there
    ** is no available object in the class.  Either case, a new object
    ** will be created.
    **
    ** Search and destroy an existing object of the same name.
    */

    bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);
					    /* compute hash bucket ptr */
    if ((objp = (ULH_HOBJ *)ulhi_search(bp, obj_name, name_len)) 
	    != (ULH_HOBJ *)NULL)
    {
	if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
	{				    /* make it destroyable */
	    objp->ulh_type = ULH_DESTROYABLE;
	    --tcb->ulh_fixcnt;		    /*  also adjust fix count */
	}
	/*
	**  Remove the object from its hash bucket and class.
	*/
	if (objp->ulh_clink.next)	    /* if the object in a class */
	    ulhi_rm_member(tcb, objp);	    /* remove it from the class */

	ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

	ulhi_yankq(&objp->ulh_obj.ulh_link); /* remove object from the bucket */
	--objp->ulh_bp->b_count;	    /* adjust bucket count */

	/*
	**  If the object becomes free, destroy it immediately.
	*/
	if (objp->ulh_ucount == 0)
	{
	    ULM_RCB	ulm_rcb;

	    if (objp->ulh_llink.next)	    /* if it is in LRU queue */
	    {
		ulhi_yankq(&objp->ulh_llink);    /* remove it */
		--(tcb->lru_cnt[objp->ulh_lindx]);
	    }
	    ulm_rcb.ulm_facility = tcb->ulh_facility;
	    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
	    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
	    ulhi_destroy_obj(tcb, objp, &ulm_rcb);   /* destroy it */
	}
    }
    /*
    **  Create a brand new object.
    */
    status = ulhi_create_obj(ulh_rcb, bp, obj_name, name_len, obj_type,
		 block_size);
    if (status != E_DB_OK)
    {	/* error, err_code already set by ulhi_create_obj */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	return (status);
    }
    objp = (ULH_HOBJ *) (ulh_rcb->objptr);

    /*
    **	Search for the specified class again.  Although we previous located
    **  the class header, the class may be destroyed when an unused object
    **  is destroyed in the process of creating a new object.
    */
    cbp = tcb->ulh_ctab + ulhi_hash(class_name, class_len, tcb->ulh_csize);
    chdr = (ULH_CHDR *)ulhi_search(cbp, class_name, class_len);
    if (chdr == (ULH_CHDR *)NULL)
    {
	/*
	**  Get an empty class header from the table.
	*/
	if (tcb->ulh_ccnt == tcb->ulh_maxclass)
	{				    /* no space for a new class */
	    ULM_RCB	ulm_rcb;

	    ulhi_yankq(&objp->ulh_obj.ulh_link);
					    /* remove object from the bucket */
	    --objp->ulh_bp->b_count;	    /* adjust bucket count */

	    ulm_rcb.ulm_facility = tcb->ulh_facility;
	    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
	    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
	    ulhi_destroy_obj(tcb, objp, &ulm_rcb); /* destroy the object */
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0105_FULL_CLASS);
	    return(E_DB_ERROR);
	}
	chdr = tcb->ulh_chq;		    /* get next available header */
	tcb->ulh_chq  = (ULH_CHDR *)chdr->cl_unit.ulh_link.next;    
					    /* update chain */
        ++tcb->ulh_ccnt;		    /* update class count */
	/*
	**	Fill in fields of the new class header and chain it in
	**	the class hash bucket (computed earlier).
	*/
        chdr->cl_namelen = class_len;
        MEcopy((PTR)class_name, class_len, (PTR)&chdr->classname[0]);
	/*
	**  Insert the class header to the tail of the bucket list.
	*/
        ulhi_enqueue(&cbp->b_queue, &chdr->cl_unit.ulh_link);
					    /* put it in bucket list */
        chdr->cl_bptr = cbp;		    /* store bucket back ptr */
        ++cbp->b_count;			    /* update bucket count */
    }
    /*
    **  Add the object to the class.
    */
    ulhi_enqueue(&chdr->cl_queue, &objp->ulh_clink);
					    /* put object in list */
    objp->ulh_cp = chdr;		    /* set class back ptr */
    ++chdr->cl_count;			    /* update counter */

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;
    return(E_DB_OK);
}

/*{
** Name: ULH_CRE_CLASS	- Obtain access to an object in a class.  If the object
**			  does not exist, create a new one.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *class_name;
**                i4             class_len;
**		  unsigned char	 *obj_name;
**		  i4		 name_len;
**		  i4		 obj_type;
**                i4        block_size;
**		  i4		 uniq_id;
**		
**           status = ulh_cre_class(ulh_rcb, class_name, class_len, 
**			obj_name, name_len, obj_type, block_size, uniq_id);
**
** Description:
**                The list of objects in the specified class is searched
**                for an existing object.  If found, the object is returned.
**		  If the object does not exist, it is created and placed into
**		  this class.  If there are no other members in this class, then
**		  the class is also created.
**
**                The object acquired through the ulh_cre_class can be
**		  released through the ulh_release.  All objects in the class
**		  can be destroyed via ulh_dest_class.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                class_name          The  pointer  to  the  class   name
**                                    string.
**                class_len            The length of the class name, up to
**                                    ULH_MAXNAME.
**                obj_name            The  pointer  to  the  object  name
**                                    string.
**                name_len            The length of the object  name,  up
**                                    to ULH_MAXNAME.
**                obj_type            Type  of  object  to  be   created:
**                                    ULH_FIXED | ULH_DESTROYABLE.
**                block_size          Used to  set  the  preferred  block
**                                    size  of  allocation  unit  in  the
**                                    memory stream for the object.
**		  uniq_id	      Unique identifier for this class member.
**
** Outputs:
**                ulh_rcb
**                    .ulh_object     The pointer to  ULH_OBJECT  of  the
**                                    object just created.
**                ULH_OBJECT
**                    .ulh_objname    The character array containing  the
**                                    object name.
**                    .ulh_namelen    An integer indicating the length of
**                                    the name.
**                    .ulh_streamid   The id of the memory stream for the
**                                    object.
**		      .ulh_uptr	      NULL if the object is newly created.
**				      User's pointer, otherwise.
**		      .ulh_uniq_id    Unique identifier for this object
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0104_FULL_TABLE No more space in the table.
**		  E_UL0105_FULL_CLASS No more space to make a new class.
**                E_UL0109_NFND       Requested class/object not found in
**                                    the table.
**                E_UL0111_OBJ_MEM    Error in allocating memory for  the
**                                    object.
**                E_UL0112_INIT_SEM   Semaphore initialization failed.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    if there is an object on the cache but not in this class with the
**	    same obj_name input for this call, the earlier object on the cache
**	    will be destroyed (if use count is 0) or marked for destroy if use
**	    count > 0.
**
** History:
**	12-mar-92 (teresa)
**	    Initial creation from ulh_getmember for sybil.
**	29-may-92 (teresa)
**	    Modify to use ULH_HOBJ.ulh_uniq_id to find desired member of class.
*/
DB_STATUS
ulh_cre_class( ULH_RCB *ulh_rcb, unsigned char *class_name, i4  class_len,
	       unsigned char *obj_name, i4  name_len, i4  obj_type,
	       i4 block_size, i4  uniq_id )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_CHDR	*chdr;
    register ULH_BUCKET	*cbp, *bp;
    DB_STATUS		status;
    STATUS		cs_status;
    i4			i;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (name_len > ULH_MAXNAME || class_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */

    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **	Search for the specified class.
    */
    cbp = tcb->ulh_ctab + ulhi_hash(class_name, class_len, tcb->ulh_csize);
    if ((chdr = (ULH_CHDR *)ulhi_search(cbp, class_name, class_len))
						 != (ULH_CHDR *)NULL)
    {
	/*
	**  The class already exists, see if specified object exists.
	*/
	objp = (ULH_HOBJ *)chdr->cl_queue.head->object;
	for (i = 0; i < chdr->cl_count; i++)
	{
	    if (objp->ulh_obj.ulh_namelen == name_len)
	    {
		if ( MEcmp((PTR)obj_name, &objp->ulh_obj.ulh_hname[0], name_len)
		   == 0)
		    /* found member of class.  See if its the one we want */
		    if (MEcmp((PTR)&uniq_id, (PTR)&objp->ulh_uniq_id, 
			      sizeof(uniq_id))
		       == 0)
		    {
			if (objp->ulh_llink.next)	/* if object in LRU */
			{
			    ulhi_yankq(&objp->ulh_llink);   /* remove object from LRU */
			    --(tcb->lru_cnt[objp->ulh_lindx]);	/* update LRU count */
			    objp->ulh_flags = ULH_USED; /* mark it multiple uses */
			}
			ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;
			++objp->ulh_ucount;		/* adjust user count */
			break;
		    }
	    }
	    objp = (ULH_HOBJ *)objp->ulh_clink.next->object;
					    /* get next object in the class */
	}				    /* end of for loop */

	/*  If an object is found, return */
	if (ulh_rcb->ulh_object != (ULH_OBJECT *) NULL)
	{
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    return(E_DB_OK);
	}
    }
    /*
    ** We come to here either because there is no such class, or there
    ** is no available object in the class.  Either case, a new object
    ** will be created.
    **
    ** There should not be any objects with the same name.  However, guard
    ** against the possibility that someone created the object using ulh_create
    ** instead of ulh_cre_class, which would allow for an object with the same
    ** name to be on the cache.  In this case, remove the existing object and
    ** go ahead and re-create it as part of this class.
    **/
    bp = tcb->ulh_otab + ulhi_hash(obj_name, name_len, tcb->ulh_osize);
					    /* compute hash bucket ptr */
    if ((objp = (ULH_HOBJ *)ulhi_search(bp, obj_name, name_len)) 
	    != (ULH_HOBJ *)NULL)
    {
	if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
	{				    /* make it destroyable */
	    objp->ulh_type = ULH_DESTROYABLE;
	    --tcb->ulh_fixcnt;		    /*  also adjust fix count */
	}
	/*
	**  Remove the object from its hash bucket and class.
	*/
	if (objp->ulh_clink.next)	    /* if the object in a class */
	    ulhi_rm_member(tcb, objp);	    /* remove it from the class */

	ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

	ulhi_yankq(&objp->ulh_obj.ulh_link); /* remove object from the bucket */
	--objp->ulh_bp->b_count;	    /* adjust bucket count */
	/*
	**  If the object becomes free, destroy it immediately.
	*/
	if (objp->ulh_ucount == 0)
	{
	    ULM_RCB	ulm_rcb;

	    if (objp->ulh_llink.next)	    /* if it is in LRU queue */
	    {
		ulhi_yankq(&objp->ulh_llink);    /* remove it */
		--(tcb->lru_cnt[objp->ulh_lindx]);
	    }
	    ulm_rcb.ulm_facility = tcb->ulh_facility;
	    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
	    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
	    ulhi_destroy_obj(tcb, objp, &ulm_rcb);   /* destroy it */
	}
    }

    /*
    **  Create a brand new object.
    */
    status = ulhi_create_obj(ulh_rcb, bp, obj_name, name_len, obj_type,
		 block_size);
    if (status != E_DB_OK)
    {	/* error, err_code already set by ulhi_create_obj */
        CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	return (status);
    }
    objp = (ULH_HOBJ *) (ulh_rcb->objptr);

    /*
    **	Search for the specified class again.  Although we previous located
    **  the class header, the class may be destroyed when an unused object
    **  is destroyed in the process of creating a new object.
    */
    cbp = tcb->ulh_ctab + ulhi_hash(class_name, class_len, tcb->ulh_csize);
    chdr = (ULH_CHDR *)ulhi_search(cbp, class_name, class_len);
    if (chdr == (ULH_CHDR *)NULL)
    {
	/*
	**  Get an empty class header from the table.
	*/
	if (tcb->ulh_ccnt == tcb->ulh_maxclass)
	{				    /* no space for a new class */
	    ULM_RCB	ulm_rcb;

	    ulhi_yankq(&objp->ulh_obj.ulh_link);
					    /* remove object from the bucket */
	    --objp->ulh_bp->b_count;	    /* adjust bucket count */
	    ulm_rcb.ulm_facility = tcb->ulh_facility;
	    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
	    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
	    ulhi_destroy_obj(tcb, objp, &ulm_rcb); /* destroy the object */
	    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0105_FULL_CLASS);
	    return(E_DB_ERROR);
	}
	chdr = tcb->ulh_chq;		    /* get next available header */
	tcb->ulh_chq  = (ULH_CHDR *)chdr->cl_unit.ulh_link.next;    
					    /* update chain */
        ++tcb->ulh_ccnt;		    /* update class count */
	/*
	**	Fill in fields of the new class header and chain it in
	**	the class hash bucket (computed earlier).
	*/
        chdr->cl_namelen = class_len;
        MEcopy((PTR)class_name, class_len, (PTR)&chdr->classname[0]);
	/*
	**  Insert the class header to the tail of the bucket list.
	*/
        ulhi_enqueue(&cbp->b_queue, &chdr->cl_unit.ulh_link);
					    /* put it in bucket list */
        chdr->cl_bptr = cbp;		    /* store bucket back ptr */
        ++cbp->b_count;			    /* update bucket count */
    }
    /*
    **  Add the object to the class.
    */
    objp->ulh_uniq_id = uniq_id;
    ulhi_enqueue(&chdr->cl_queue, &objp->ulh_clink);
					    /* put object in list */
    objp->ulh_cp = chdr;		    /* set class back ptr */
    ++chdr->cl_count;			    /* update counter */

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;
    return(E_DB_OK);
}

/*{
** Name: ULH_DEST_CLASS	- Destroy all objects in a class & destroy that class
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                unsigned char	 *class_name;
**                i4             class_len;
**		
**           status = ulh_dest_class (ulh_rcb, class_name, class_len)
**
** Description:
**                The list of objects in the specified class is walked,
**		  and each object in the class is destroyed.  When the last
**		  object is destroyed, then the class header is also destroyed.
**
** Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                class_name          The  pointer  to  the  class   name
**                                    string.
**                class_len            The length of the class name, up to
**                                    ULH_MAXNAME.
**
** Outputs:
**	none.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0106_OBJECT_ID  Invalid object pointer.
**                E_UL0107_ACCESS     Invalid access to the object.
**		  E_UL0109_NFND	      Object not found.  If requested to
**				      destroy an object in LRU, this means
**				      there is no objects in the LRU queues.
**                E_UL0112_INIT_SEM   Semaphore initialization failed.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**                E_UL0116_LRU_SEM    Error in locking  LRU  queue  sema-
**                                    phore.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-12-92 (teresa)
**	    Initial creation from ulh_getmember for SYBIL.
*/
DB_STATUS
ulh_dest_class( ULH_RCB *ulh_rcb, unsigned char *class_name, i4  class_len )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    register ULH_HOBJ	*next_objp;
    register ULH_CHDR	*chdr;
    register ULH_BUCKET	*cbp, *bp;
    DB_STATUS		status;
    STATUS		cs_status;
    i4			i;
    bool		found = FALSE;

    CLRDBERR(&ulh_rcb->ulh_error);

    if (class_len > ULH_MAXNAME)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0123_NAME_TOO_LONG);
	return(E_DB_ERROR);
    }

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    ulh_rcb->ulh_object = (ULH_OBJECT *) NULL;	/* zero out object pointer */

    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }
    /*
    **	Search for the specified class.
    */
    cbp = tcb->ulh_ctab + ulhi_hash(class_name, class_len, tcb->ulh_csize);
    if ( (chdr = (ULH_CHDR *)ulhi_search(cbp, class_name, class_len))
         != (ULH_CHDR *)NULL
       )
    {
	i4  cl_cnt = chdr->cl_count;
	/*
	**  The specified class exists, so get an object from it.
	*/
	found = TRUE;
	next_objp = (ULH_HOBJ *)chdr->cl_queue.head->object;
	
	while (cl_cnt--)
	{
	    objp = next_objp;

	    /* destroy that object */
	    if (objp->ulh_type == ULH_FIXED)    /* if fixed object, */
	    {				    /* make it destroyable */
		objp->ulh_type = ULH_DESTROYABLE;
		--tcb->ulh_fixcnt;		    /*  also adjust fix count */
	    }

	    /* save ptr to next object on queue, since ulhi_rm_member call
	    ** will wipe out the desired ptr 
	    */
	    next_objp = (ULH_HOBJ *)objp->ulh_clink.next->object;

	    /*
	    **  Remove the object from its hash bucket and class.
	    */
	    ulhi_rm_member(tcb, objp);	/* remove it from the class */
	    ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count); /* remove
									alias */
	    ulhi_yankq(&objp->ulh_obj.ulh_link); /* remove object from bucket */
	    --objp->ulh_bp->b_count;		 /* adjust bucket count */

	    /*
	    **  If the object becomes free, destroy it immediately.
	    */
	    if (objp->ulh_ucount == 0)
	    {
		ULM_RCB	ulm_rcb;

		if (objp->ulh_llink.next)	    /* if it is in LRU queue */
		{
		    ulhi_yankq(&objp->ulh_llink);    /* remove it */
		    --(tcb->lru_cnt[objp->ulh_lindx]);
		}
		ulm_rcb.ulm_facility = tcb->ulh_facility;
		ulm_rcb.ulm_poolid = tcb->ulh_poolid;
		ulm_rcb.ulm_memleft = tcb->ulh_memleft;
		ulhi_destroy_obj(tcb, objp, &ulm_rcb);   /* destroy it */
	    }

	}  /* end of for loop */
    }
    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    if (found)
    {
        return(E_DB_OK);
    }
    else
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
        return(E_DB_ERROR);
    }	
}

/*{
** Name: ULH_INFO	- Obtain information about a hash table.
**
** Synopsis:
**                DB_STATUS      status;
**                ULH_RCB        *ulh_rcb;
**                i4             ulh_icode;
**                i4             access_mode;
**                PTR            *bufptr;
**
**                   status = ulh_info(ulh_rcb, ulh_icode, bufptr);
**
** Description:
**                This function returns specific information about a hash
**                table  to  the user.  The information to be returned by
**                ULH  depends  on  the  information  code  specified  in
**                ulh_icode.   In  all  cases,  the user must provide the
**                buffer to receive  the  information.   The  information
**                codes are described below:
**
**	ULH_TABLE
**                Obtain hash table attributes as described by the struc-
**                ture  ULH_ATTR.   The  information included the current
**                counts of classes and objects.
**
**	    Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                ulh_icode           ULH_TABLE, information about  table
**                                    attributes.
**                bufptr              The pointer to the ULH_ATTR  struc-
**                                    ture in the user's data space.
**
**	    Outputs:
**                ULH_ATTR            See ULH_ATTR in External  Interface
**                                    Specification.
**
**	ULH_ROSTER
**                Obtain information about classes  in  the  table.   The
**                information  returned  are the names of the classes and
**                the count of objects in each class.
**
**	    Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                ulh_icode           ULH_ROSTER,    information    about
**                                    classes.
**                bufptr              The pointer to the ULH_CLIST struc-
**                                    ture in the user's data space.
**                ULH_CLIST
**                    .ulh_ccount     Count of classes to get information
**                                    from.
**                    .ulh_cstat[]    Array of class information records.
**
**	    Outputs:
**                ULH_CLIST
**                    .ulh_rcount     Count of class  information  record
**                                    returned in .ulh_cstat[] array.
**                    .ulh_cstat[]    Array of class information  records
**                                    returned  by  ULH.   See  ULH_CSTAT
**                                    structure in the External Interface
**                                    Description.
**
**	ULH_MEMCOUNT
**                Obtain count of objects in a specific class.
**
**	    Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                ulh_icode           ULH_MEMCOUNT
**                bufptr              The pointer to the ULH_CSTAT struc-
**                                    ture in the user's data space.
**                ULH_CSTAT
**                    .ulh_cname[]    The name of the class in question.
**                    .ulh_namelen    The length of the name string.
**
**	    Outputs:
**                ULH_CSTAT
**                    .ulh_count      Count of objects in this class.
**
**	ULH_STAT
**                Obtain the distribution of objects and classes  in  the
**                object and the class table.  This function goes through
**                all buckets in the object table and returns  the  count
**                of  objects  in  each bucket.  Then it goes through the
**                class table and returns the count of  classes  in  each
**                class  bucket.  The user must provide an interger (i4)
**                array to contain the numbers returned by this function.
**                The size of the array is:
**
**	    array_size  =  ulh_attr.ulh_bcount  + ulh_attr.ulh_cbuckets
**
**	    Inputs:
**                ulh_rcb
**                    .ulh_hashid     The internal id of the hash table.
**                ulh_icode           ULH_STAT
**                bufptr              The pointer to an integer array  in
**                                    user's data space.
**
**	    Outputs:
**                bufprt              An  integer  array  specifying  the
**                                    lengths of hash table buckets.  The
**                                    first         "ulh_attr.ulh_bcount"
**                                    integers  are the counts of objects
**                                    in object buckets.   The  remaining
**                                    "ulh_attr.ulh_cbuckets"   are   the
**                                    counts of classes in classes  buck-
**                                    ets.
**
**	Returns:
**	    Status:
**		  E_DB_OK, E_DB_ERROR, E_DB_FATAL
**
**          Error Codes:
**                E_UL0101_TABLE_ID   Invalid ULH table id.
**                E_UL0113_TABLE_SEM  Error in locking table semaphore.
**                E_UL010C_UNKNOWN    Unknown information code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**       9-Nov-2009 (hanal04) Bug 122861
**          Release the ulh_sem before returning from the default error case.
*/
DB_STATUS
ulh_info( ULH_RCB *ulh_rcb, i4  ulh_icode, PTR bufptr )
{
    register ULH_TCB	*tcb;
    register ULH_ATTR	*ap;
    register ULH_CLIST	*clp;
    register ULH_CSTAT	*cstat;
    register ULH_BUCKET	*bp;
    register ULH_CHDR	*chdr;
    DB_STATUS		status;
    STATUS		cs_status;
    i4			*iptr;
    i4			i, bcnt, count;

    CLRDBERR(&ulh_rcb->ulh_error);
    

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    if (tcb->ulh_tcb != tcb)			/* validate the pointer */
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0101_TABLE_ID);
	return(E_DB_ERROR);
    }
    /*
    **  Lock the cache table.
    */
    if (cs_status = CSp_semaphore(ULH_EXCLUSIVE, &tcb->ulh_sem))
    {
	SETDBERR(&ulh_rcb->ulh_error, cs_status, E_UL0113_TABLE_SEM);
	return(E_DB_ERROR);			 
    }

    switch(ulh_icode)
    {
	case ULH_TABLE:
	    ap = (ULH_ATTR *) bufptr;
	    ap->ulh_facility = tcb->ulh_facility;
	    ap->ulh_poolid   = tcb->ulh_poolid;
	    ap->ulh_streamid = tcb->ulh_streamid;
	    ap->ulh_bcount   = tcb->ulh_osize;
	    ap->ulh_maxobj   = tcb->ulh_maxobj;
	    ap->ulh_fcount   = tcb->ulh_fixcnt;
	    ap->ulh_dcount   = tcb->ulh_objcnt - tcb->ulh_fixcnt;
	    ap->ulh_cbuckets = tcb->ulh_csize;
	    ap->ulh_maxclass = tcb->ulh_maxclass;
	    ap->ulh_ccount   = tcb->ulh_ccnt;
	    ap->ulh_l1count  = tcb->lru_cnt[0];
	    ap->ulh_l2count  = tcb->lru_cnt[1];
	    break;

	case ULH_ROSTER:
	    clp = (ULH_CLIST *) bufptr;
	    cstat = &clp->ulh_cstat[0];
	    bp = tcb->ulh_ctab;
	    count = 0;
	    for ( i = 0; (i < tcb->ulh_csize && count < clp->ulh_ccount); ++i)
	    {
		if ((bcnt = bp->b_count) != 0)
		{
		    chdr = (ULH_CHDR *)bp->b_queue.head->object;
		    while (bcnt && count < clp->ulh_ccount)
		    {
			cstat->ulh_count = chdr->cl_count;
			cstat->ulh_namelen = chdr->cl_namelen;
		        MEcopy((PTR)&chdr->classname[0], chdr->cl_namelen,
			       (PTR)&cstat->ulh_cname[0]);
			chdr = (ULH_CHDR *)chdr->cl_unit.ulh_link.next->object;
			++cstat;
			++count;
			--bcnt;
		    }
		}
		++bp;
	    }
	    clp->ulh_rcount = count;		/* return actual count */
	    break;
	    
	case ULH_MEMCOUNT:
	    cstat = (ULH_CSTAT *) bufptr;
	    bp = tcb->ulh_ctab + ulhi_hash(cstat->ulh_cname,
			cstat->ulh_namelen, tcb->ulh_csize);
					    /* compute class table bucket */
	    if ((chdr = (ULH_CHDR *)ulhi_search(bp, cstat->ulh_cname,
		    cstat->ulh_namelen)) == (ULH_CHDR *)NULL)
	    {
		SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0109_NFND);
	        CSv_semaphore(&tcb->ulh_sem);
		return(E_DB_ERROR);
	    }
	    cstat->ulh_count = chdr->cl_count;
	    break;

	case ULH_STAT:
	    iptr = (i4 *) bufptr;
	    bp = tcb->ulh_otab;
	    for (i = 0; i < tcb->ulh_osize; ++i)
	    {
		*iptr = bp->b_count;
		++iptr;
		++bp;
	    }
	    bp = tcb->ulh_ctab;
	    for (i = 0; i < tcb->ulh_csize; ++i)
	    {
		*iptr = bp->b_count;
		++iptr;
		++bp;
	    }
	    break;

	default:
            CSv_semaphore(&tcb->ulh_sem);
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL010C_UNKNOWN);
	    return(E_DB_ERROR);
    }

    CSv_semaphore(&tcb->ulh_sem);    /* release table sem */
    return(E_DB_OK);	
}
/****************************************************************************/
/*                     Internal Subroutines                                 */
/****************************************************************************/

static DB_STATUS
ulhi_create_obj( ULH_RCB *ulh_rcb, ULH_BUCKET *bp, unsigned char *obj_name,
		 i4  name_len, i4  obj_type, i4 block_size )
{
    register ULH_TCB	*tcb;
    register ULH_HOBJ	*objp;
    DB_STATUS		status;
    ULM_RCB		ulm_rcb;	    /* temp control block for ULM */

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */
    /*    
    **  Error if the table is full and all the objects are fixed (since we
    **  cannot destroy them).
    */
    if (tcb->ulh_fixcnt == tcb->ulh_maxobj)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0104_FULL_TABLE);
	return(E_DB_ERROR);
    }

    ulm_rcb.ulm_facility = tcb->ulh_facility;
    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
    ulm_rcb.ulm_blocksize = block_size;		/* block size as requested */

    /*
    **  If the table is full or has not enough memory for the new object,
    **  Destroy a destroyable object. Remove an object from LRU1/LRU2
    **  queue, if any.  Then remove it from the object table bucket.
    **  Also, if the object belongs to a class, remove it from the class.
    */
    if (tcb->ulh_objcnt == tcb->ulh_maxobj ||
	*(tcb->ulh_memleft) <= (sizeof(ULH_HOBJ)))
    {
	if ((tcb->lru_cnt[1]) && (tcb->lru_cnt[0] <= tcb->lru1_limit))
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2);
						/* remove object from LRU2 */
	    --(tcb->lru_cnt[1]);		/* adjust LRU2 count */
	}
	else if (tcb->lru_cnt[0])
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1);
						/* remove object from LRU1 */
	    --(tcb->lru_cnt[0]);		/* adjust LRU1 count */
	}
	else
        {
	    SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0104_FULL_TABLE);
	    return(E_DB_ERROR);
        }

	if (objp->ulh_clink.next)	    /* remove object from its class */
	    ulhi_rm_member(tcb, objp);

	ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

	if (objp->ulh_obj.ulh_link.next)    /* remove object from its hash */
	{				    /*  bucket */
	    ulhi_yankq(&objp->ulh_obj.ulh_link);
	    --objp->ulh_bp->b_count;	    /* adjust bucket count */
	}

	ulhi_destroy_obj(tcb, objp, &ulm_rcb);    /* destroy the object */
    }
    /*
    **  Create a new object.
    **	Open a shared stream for the object and allocate object header.
    **  Destroy an unused object if out of memory.
    */
    /* Open stream and allocate ULH_HOBJ with one effort */
    ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
    ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(ULH_HOBJ);

    while ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code != E_UL0005_NOMEM)
	{
	    SETDBERR(&ulh_rcb->ulh_error, ulm_rcb.ulm_error.err_code, E_UL0111_OBJ_MEM);
	    return(status);
	}
	if ((tcb->lru_cnt[1]) && (tcb->lru_cnt[0] <= tcb->lru1_limit))
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2);
						/* remove object from LRU2 */
	    --(tcb->lru_cnt[1]);		/* adjust LRU2 count */
	}
	else if (tcb->lru_cnt[0])
	{
	    objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1);
						/* remove object from LRU1 */
	    --(tcb->lru_cnt[0]);		/* adjust LRU1 count */
	}
	else
        {
	    SETDBERR(&ulh_rcb->ulh_error, ulm_rcb.ulm_error.err_code, E_UL0111_OBJ_MEM);
	    return(status);
        }

	if (objp->ulh_clink.next)	    /* remove object from its class */
	    ulhi_rm_member(tcb, objp);

	ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

	if (objp->ulh_obj.ulh_link.next)    /* remove object from its hash */
	{				    /*  bucket */
	    ulhi_yankq(&objp->ulh_obj.ulh_link);
	    --objp->ulh_bp->b_count;	    /* adjust bucket count */
	}

	ulhi_destroy_obj(tcb, objp, &ulm_rcb);    /* destroy the object */

	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof(ULH_HOBJ);
    }
    
    objp = (ULH_HOBJ *) (ulm_rcb.ulm_pptr);
    objp->obj_streamid = ulm_rcb.ulm_streamid;

    /* Initialize object's semaphore */

    if (CSw_semaphore(&objp->ulh_usem, CS_SEM_SINGLE,
    			"ULH sem" ) != OK)
    {
	(VOID) ulm_closestream(&ulm_rcb);
	return(E_DB_FATAL);	/* object semaphore initialization failed */
    }
    /*
    **	Fill in fields in the newly created object and chain it in
    **	the hash bucket (computed earlier).
    */
    objp->ulh_flags = ULH_NEW;
    objp->ulh_type = (unsigned char)obj_type & ULH_FIXED;
    objp->ulh_ucount = 1;
    objp->obj_namelen = name_len;
    MEcopy((PTR)obj_name, name_len, (PTR)&objp->objectname[0]);
    objp->ulh_llink.object = (PTR) objp;
    objp->ulh_llink.next = (ULH_LINK *) NULL;
    objp->ulh_llink.back = (ULH_LINK *) NULL;
    objp->ulh_clink.object = (PTR) objp;
    objp->ulh_clink.next = (ULH_LINK *) NULL;
    objp->ulh_clink.back = (ULH_LINK *) NULL;

    /* initialize queue of aliases defined for this object */
    objp->ulh_aq.head = objp->ulh_aq.tail = (ULH_LINK *) &objp->ulh_aq;

    objp->ulh_cp = (ULH_CHDR *)NULL;
    objp->ulh_obj.ulh_link.object = (PTR) objp;
    objp->ulh_uptr = (PTR) NULL;		    /* clear user ptr */

    ulhi_enqueue(&bp->b_queue, &objp->ulh_obj.ulh_link); 
						    /* put it in bucket list */
    objp->ulh_bp = bp;				    /* store bucket back ptr */
    ++bp->b_count;				    /* update bucket count */
    ++tcb->ulh_objcnt;				    /* update object count */
    if (obj_type == ULH_FIXED)			    /* update fix count */
	++tcb->ulh_fixcnt;

#ifdef	xDEV_TEST
    ++tcb->ulh_o_created;		/* update count of objects created */
    if (tcb->ulh_objcnt > tcb->ulh_max_objcnt) 
	++tcb->ulh_max_objcnt; 		/* update highest count of objects */
    if (bp->b_count > tcb->ulh_b_max)
	++tcb->ulh_b_max;		/* update highest count in a bucket */
#endif /* xDEV_TEST */

    ulh_rcb->ulh_object = (ULH_OBJECT *) &objp->uobject;	
					/* return ptr to user-visible part */
    return (E_DB_OK);
}

/*
** static VOID
** ulhi_remove_aliases()    - remove aliases associated with an object from
**			      their respective hash buckets and adjust counters
**			      accordingly.
**
**  Input:
**	alias_queue	    ptr to the list of object's aliases
**	alias_count	    ptr to the count of aliases
**
**  Output:
**	None
**
**  Side effects:
**	Alias hash bucket counts may be adjusted
**	Count of aliases may get adjusted
**
**  Returns:
**	None
**  
**  History:
**	06-apr-90 (andre)
**	    written
*/
static VOID
ulhi_remove_aliases( ULH_QUEUE *alias_queue, i4  *alias_count )
{
    register ULH_LINK   *linkp_cur, *linkp_last;

    /*
    ** since it is now possible to have multiple aliases, it is necessary
    ** to go down the list of this object's aliases and yank each of them of
    ** their respective hash bucket lists
    */

    for (linkp_cur = alias_queue->head, linkp_last = (ULH_LINK *) alias_queue;
	 linkp_cur != linkp_last;
	 linkp_cur = linkp_cur->next)
    {
	/* remove alias from its hash bucket */
	ulhi_yankq(&((ULH_HALIAS *) linkp_cur->object)->ulh_alink);

	/*
	** decrement number of aliases 
	*/
	--(*alias_count);

	/* adjust alias bucket count */
	--((ULH_HALIAS *) linkp_cur->object)->ulh_ap->b_count;  
    }
}

/*
** static DB_STATUS
** ulhi_mem_for_alias()  - try to destroy an existing object from one of LRU
**			   queues to make room for a new alias.  There is a
**			   stipulation that we may not destroy an object for
**			   which an alias is being created.
**
**  Input:
**	ulh_rcb		    request CB
**	ulm_rcb		    used for calling ULM routines.
**	    .ulm_facility   name of the facility
**	    .ulm_poolid	    memory pool id
**	    .ulm_memleft    ptr to the counter of available memory
**	alias_obj	    object for which an alias is being created
**
**
**  Returns:
**	E_DB_OK		    an object was found and destroyed
**	E_DB_ERROR	    could not find an object to destroy
**
**  Side effects:
**	ulm_rcb
**	    .ulm_memleft    amount of available memory may have increased
**
**  Returns:
**	E_DB_OK, E_DB_ERROR.
**
**  Error codes:
**	    E_UL0119_ALIAS_MEM
*/
static DB_STATUS
ulhi_mem_for_alias( ULH_RCB *ulh_rcb, ULM_RCB *ulm_rcb, ULH_HOBJ *alias_obj )
{
    register ULH_TCB    *tcb;
    register ULH_HOBJ   *objp;

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;      /* get pointer to ULH_TCB */

    objp = (ULH_HOBJ *) NULL;
    
    /*
    **  NOTE: when destroying objects from an LRU queue, we must avoid
    **        destroying the object for which the alias is being created
    */

    if ((tcb->lru_cnt[1]) && (tcb->lru_cnt[0] <= tcb->lru1_limit))
    {
	/*
	** Try to remove object from LRU2.
	** See if this is the object for which an alias is being defined;
	** if so, enqueue it;
	** if the object for which an alias is being defined is the only
	** object on the queue, we'll have to try our luck on the other
	** queue
	*/

	if ((objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2)) == alias_obj)
	{
	    ulhi_enqueue(&tcb->ulh_lru2, &objp->ulh_llink);

	    /*
	    ** if there is only one element on the queue, we'll have to try
	    ** to destroy an object on another queue
	    */
	    objp = (tcb->lru_cnt[1] < 2)
		? (ULH_HOBJ *) NULL
		: (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru2);
	}
    }

    if (objp != (ULH_HOBJ *) NULL)
    {
	/* we found the element to destroy, adjust LRU2 count */
	--(tcb->lru_cnt[1]);
    }
    else if (tcb->lru_cnt[0])
    {
	/*
	** Try to remove object from LRU1.
	** See if this is the object for which an alias is being defined;
	** if so, enqueue it;
	** if the object for which an alias is being defined is the only
	** object on the queue, we won't be able to find an object to
	** destroy.
	*/

	if ((objp = (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1)) == alias_obj)
	{
	    ulhi_enqueue(&tcb->ulh_lru1, &objp->ulh_llink);

	    /*
	    ** if there is only one element on the queue, there are no
	    ** objects for us to destroy.
	    */
	    objp = (tcb->lru_cnt[0] < 2)
		? (ULH_HOBJ *) NULL
		: (ULH_HOBJ *)ulhi_dequeue(&tcb->ulh_lru1);
	}

	if (objp != (ULH_HOBJ *) NULL)
	{
	    /* we found the element to destroy, adjust LRU1 count */
	    --(tcb->lru_cnt[0]);
	}
    }

    /* if we found no objects to destroy, return error here */
    if (objp == (ULH_HOBJ *) NULL)
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0119_ALIAS_MEM);
	return(E_DB_ERROR);
    }

    if (objp->ulh_clink.next)	    /* remove object from its class */
	ulhi_rm_member(tcb, objp);

    ulhi_remove_aliases(&objp->ulh_aq, &tcb->ulh_alias_count);

    if (objp->ulh_obj.ulh_link.next)    /* remove object from its hash bucket */
    {
	ulhi_yankq(&objp->ulh_obj.ulh_link);
	--objp->ulh_bp->b_count;	    /* adjust bucket count */
    }

    ulhi_destroy_obj(tcb, objp, ulm_rcb);    /* destroy the object */

    return(E_DB_OK);	/* object has been successfully destroyed */
}

/*
** static DB_STATUS
** ulhi_create_alias()	- allocate space for a new alias and attach it the the
**			  list of object's aliases + to the list of the hash
**			  bucket objects.
**  Input:
**	ulh_rcb->
**	    ulh_hashid->	pointer to ULH_TCB
**		ulh_alias_count	    number of aliases in the table
**		ulh_maxalias	    maximum number of aliases
**		ulh_fixcnt	    number of fixed objects
**		ulh_obj_cnt	    number of objects in the table
**	ap		alias hash bucket ptr
**	alias_name	name of the new alias
**	name_len	length of the name of the new alias
**	alias_obj	object for which an alias is being created
**  Output:
**	ulh_rcb->
**	    ulh_error           error block
**	    ulh_hashid->        pointer to ULH_TCB
**		ulh_alias_count	    current number of aliases
**		ulh_obj_cnt	    may change if any objects were destroyed
**  Returns:
**	E_DB_OK, E_DB_ERROR
**
**  Error codes:
**	    E_UL0104_FULL_TABLE
**	    E_UL0119_ALIAS_MEM
*/
static DB_STATUS
ulhi_create_alias( ULH_RCB *ulh_rcb, ULH_BUCKET *ap, unsigned char *alias_name,
		   i4  name_len, ULH_HOBJ *alias_obj )
{
    register ULH_TCB	*tcb;
    register ULH_HALIAS *alias;
    DB_STATUS		status;
    ULM_RCB		ulm_rcb;	    /* temp control block for ULM */

    tcb = (ULH_TCB *) ulh_rcb->ulh_hashid;	/* get pointer to ULH_TCB */

    ulm_rcb.ulm_facility = tcb->ulh_facility;
    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
    ulm_rcb.ulm_memleft = tcb->ulh_memleft;

    /*
    **  While there is not enough memory for a new alias object, or there are
    **	too many aliases, destroy a destroyable object. Remove an object from
    **	LRU1/LRU2 queue, if any.  Then remove it from the object table bucket.
    **  Also, if the object belongs to a class, remove it from the class.
    **	NOTE: we will not destroy an object for which an alias is being created.
    */
    
    /*
    ** return error if we will need to destroy at least 1 object but all objects
    ** are fixed (fixed objects can not be destroyed)
    */
    if (tcb->ulh_fixcnt == tcb->ulh_objcnt &&
	(*(tcb->ulh_memleft) <= sizeof(ULH_HALIAS) ||
	 tcb->ulh_alias_count >= tcb->ulh_maxalias))
    {
	SETDBERR(&ulh_rcb->ulh_error, 0, E_UL0104_FULL_TABLE);
	return(E_DB_ERROR);
    }

    while (*(tcb->ulh_memleft) <= sizeof(ULH_HALIAS) ||
	   tcb->ulh_alias_count >= tcb->ulh_maxalias)
    {
	/* try to destroy an object to free up room for a new alias */
	status = ulhi_mem_for_alias(ulh_rcb, &ulm_rcb, alias_obj);
	if (status != E_DB_OK)
	{
	    return(status);
	}
    }

    /*
    **  Create a new alias.
    **  Destroy existing objects(s) if out of memory
    */

    ulm_rcb.ulm_psize = sizeof(ULH_HALIAS);
    
    /* will use object's stream to allocate alias */
    ulm_rcb.ulm_streamid_p = &alias_obj->ulh_streamid;
    
    while ((status = ulhi_alloc(ulh_rcb, &ulm_rcb, E_UL0119_ALIAS_MEM, 0)) !=
	    E_DB_OK)
    {	
	if (ulm_rcb.ulm_error.err_code != E_UL0005_NOMEM)
	{
	    SETDBERR(&ulh_rcb->ulh_error, ulm_rcb.ulm_error.err_code, E_UL0119_ALIAS_MEM);
	    return(status);
	}

	/* try to destroy an object to free up room for a new alias */
	status = ulhi_mem_for_alias(ulh_rcb, &ulm_rcb, alias_obj);
	if (status != E_DB_OK)
	{
	    return(status);
	}

	ulm_rcb.ulm_streamid_p = &alias_obj->ulh_streamid;	/* restore stream id */
	ulm_rcb.ulm_psize = sizeof(ULH_HALIAS);	       /* restore memory size */
    }

    alias = (ULH_HALIAS *) ulm_rcb.ulm_pptr;

    /*
    **	Fill in fields in the newly created alias + place it on the list of the
    **	object's aliases + add it to the appropriate hash bucket
    */
    alias->ulh_ap = ap;
    alias->ulh_obj = alias_obj;
    alias->ulh_alias_len = name_len;
    MEcopy((PTR)alias_name, name_len, (PTR)alias->ulh_halias);

    /* make links point at the alias structure */
    alias->ulh_alink.object = alias->ulh_objlink.object = (PTR) alias;

    /* attach to the hash bucket list */
    ulhi_enqueue(&ap->b_queue, &alias->ulh_alink);
    /* attach to the list of object's aliases */
    ulhi_enqueue(&alias_obj->ulh_aq, &alias->ulh_objlink);

    ++ap->b_count;				    /* update bucket count */
    ++tcb->ulh_alias_count;			    /* update alias count */

    return (E_DB_OK);
}


/*{
** Name: ulhi_release_obj	- Relinquish user's access to an object
**
** Description:
**      Relinquish user's access to a hash object by decrementing the
**      object user count.  An object becomes free (not "in-use')
**      when its user count becomes zero.
**       
**      A destroyable object that becomes free will be appended to
**	the LRU1 if it was accessed for the first time, otherwise,
**	it will be appended to the LRU2, 
**
** Inputs:
**      ulh_rcb				pointer to ULH_RCB
**      tcb                             pointer to ULH_TCB
**      objp                            pointer to the object to be released.
** Outputs:
**      none
**	Returns:
**	    0	    if the function completes successfully
**	    1	    if error, error code is set in ulh_rcb.ulh_error
**		     by the subfunction.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	30-mar-87 (puree)
**	    immediately destroy a free object that has a pending destroy.
**	18-sep-87 (puree)
**	    modify semaphore handling.
*/
static i4
ulhi_release_obj( ULH_RCB *ulh_rcb, ULH_TCB *tcb, ULH_HOBJ *objp )
{
    ULM_RCB	    ulm_rcb;

    /*
    **	Check user count.
    */
    if (objp->ulh_ucount == 0)		    /* object is already free */
    {
	return(0);			    /* just return */
    }
    else
    {
	--objp->ulh_ucount;		    /* decrement user count */
    }
    /*
    **  Check object type.
    */
    if (objp->ulh_type == ULH_FIXED)	    /* if fixed object, */
	return(0);			    /* done */
    /*
    **  If a destroyable object becomes free, put it in LRU queue.
    **  For the object that was destroy, its memory space is reclaimed
    **  immediately.
    */
    if (objp->ulh_ucount == 0)
    {
	if (objp->ulh_obj.ulh_link.next == (ULH_LINK *) NULL)
	{
	    ulm_rcb.ulm_facility = tcb->ulh_facility;
	    ulm_rcb.ulm_poolid = tcb->ulh_poolid;
	    ulm_rcb.ulm_memleft = tcb->ulh_memleft;
	    ulhi_destroy_obj(tcb, objp, &ulm_rcb);
	}
	else
	{
	    if (objp->ulh_flags == ULH_NEW)
	    {
		ulhi_enqueue(&tcb->ulh_lru1, &objp->ulh_llink);
		objp->ulh_lindx = 0;	    /* indicate object in the LRU 1 */
		++tcb->lru_cnt[0];		    /* update LRU count */
	    }
	    else
	    {
		ulhi_enqueue(&tcb->ulh_lru2, &objp->ulh_llink);
		objp->ulh_lindx = 1;	    /* indicate object in the LRU 2 */
		++tcb->lru_cnt[1];		    /* update LRU count */
	    }
	}
    }
    return(0);
}

/*{
** Name: ulhi_destroy_obj	- destroy an object
**
** Description:	
**      Destroy an object by:
**	    - return the object semaphore to the available queue.
**	    - close object memory stream.
**	    - decrement object count in the table.
**
** Inputs:
**      tcb                             pointer to ULH_TCB
**      objp                            pointer to the object to be destroyed
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-mar-87 (puree)
**          initial version.
**	18-sep-87 (puree)
**	    modify semaphore handling.
**	17-Nov-1995 (reijo01)
**	    Added a CSr_semaphore to remove an object's semaphore and thus
**	    the semaphore instance string from the MO_strings table when 
**	    the object is destroyed.
*/
static VOID
ulhi_destroy_obj( ULH_TCB *tcb, ULH_HOBJ *objp, ULM_RCB *ulm_rcb )
{
    
    /*
    **	Remove the object's semaphore.
    */
    CSr_semaphore(&objp->ulh_usem);

    /*  Close object memory stream */
	
    ulm_rcb->ulm_streamid_p = &objp->obj_streamid;
    (VOID) ulm_closestream(ulm_rcb);

    /* Decrement table count */

    --tcb->ulh_objcnt;

#ifdef	xDEV_TEST
    ++tcb->ulh_o_destroyed;	/* update count of objects destroyed */
#endif /* xDEV_TEST */
}

/*{
** Name: ulhi_rm_member	- remove a member object from a class
**
** Description:
**      This function removes an object from a class.  If the class 
**      becomes empty, remove the class header from the bucket and 
**      return it to the free header chain (ulh_chq in ULH_TCB).
** 
** Inputs:
**      tcb                             pointer to ULH_TCB
**      objp                            pointer to the object to be
**					 removed from its class.
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	19-apr-1994 (andyw) Cross Integ from 6.4 (ramra01)
**	    Check foreward link before yanking
**	06-aug-1998 (nanpr01) 
**	    Backed out the change for andyw. If we get a segv from ulh,
**	    we must check the caller. Caller must be using a stale pointer.
**	    Checking for null pointer to prevent the segv is bad idea.
*/
static VOID
ulhi_rm_member( ULH_TCB *tcb, ULH_HOBJ *objp )
{
    register ULH_CHDR	*chdr;			/* pointer to class header */
    register ULH_BUCKET	*bp;			/* pointer to class bucket */

    chdr = objp->ulh_cp;		/* get pointer to class header */
    ulhi_yankq(&objp->ulh_clink);	/* remove it from class */
    objp->ulh_cp = (ULH_CHDR *)NULL;
    if ( --chdr->cl_count == 0)		/* adjust count of objects */
    {					/* class becomes empty */
	bp = chdr->cl_bptr;		/* get class bucket ptr */
	ulhi_yankq(&chdr->cl_unit.ulh_link);
	chdr->cl_bptr = (ULH_BUCKET *)NULL;
					/* remove it from bucket */
	--bp->b_count;			/* adjust bucket count */
	chdr->cl_unit.ulh_link.next = (ULH_LINK *) tcb->ulh_chq;
					/* put header in the free list */
	tcb->ulh_chq = chdr;
	--tcb->ulh_ccnt;		/* adjust total class count */
    }
}

/*{
** Name: ulhi_enqueue	- append an element to the tail of a queue.
**
** Description:
**      Append an element to the tail of the specified queue.
**
** Inputs:
**	queue			       pointer to ULH_QUEUE of the queue.
**      element                        pointer to ULH_LINK of the element.
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87(puree)
**          initial version.
*/
static VOID
ulhi_enqueue( ULH_QUEUE *queue, ULH_LINK *element )
{
    element->next = (ULH_LINK *)queue;
    element->back = queue->tail;
    queue->tail = element;
    element->back->next = element;
}

/*{
** Name: ulhi_dequeue	- remove an object from the head of a queue
**
** Description:
**      The calling routine must ensure that there is at least one object 
**      in the specified queue. 
**
** Inputs:
**	queue			       pointer to ULH_QUEUE of the queue.
**
** Outputs:
**      none 
**	Returns:
**	    Pointer to the generic object.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version
*/
static PTR
ulhi_dequeue( ULH_QUEUE *queue )
{
    register ULH_LINK     *element;

    element = queue->head;
    queue->head = element->next;
    element->next->back = (ULH_LINK *)queue;
    element->next = (ULH_LINK *)NULL;
    element->back = (ULH_LINK *)NULL;
    return(element->object);
}

/*{
** Name: ulhi_yankq	- remove an object from any position in the queue.
**
** Description:
**      Given a queue linker (ULH_LINK) of an object, remove it from
**      the associated queue.
**
** Inputs:
**      element                         pointer to ULH_LINK of the object
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-nan-87 (puree)
**          initial version.
*/
static VOID
ulhi_yankq( ULH_LINK *element )
{
    register ULH_LINK	*fwd_element, *prev_element;

    fwd_element = element->next;
    prev_element = element->back;
    prev_element->next = fwd_element;
    fwd_element->back = prev_element;
    element->next = (ULH_LINK *)NULL;
    element->back = (ULH_LINK *)NULL;
}


/*{
** Name: ulhi_search	- search a bucket list
**
** Description:
**      Search through a bucket list for an object/class with the exact
**	name match.
**
** Inputs:
**      bp                              pointer to the bucket
**      obj_name                        pointer to the requested name string
**      name_len                        length of the requested name.
**
** Outputs:
**	    none
**	Returns:
**	    PTR                         the pointer to the object/class with
**					the exactname match.  A NULL if no
**					match is found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
static PTR
ulhi_search( ULH_BUCKET *bp, unsigned char *obj_name, i4  name_len )
{
    register ULH_UNIT	*unit;
    register i4	count;

    if ( name_len == 0 || (count = bp->b_count) == 0 )
	return(NULL);			/* don't search an empty bucket
					**  or a null string */

    unit = (ULH_UNIT *)bp->b_queue.head; /* get the first unit */

    while (count)
    {
	if (unit->ulh_namelen == name_len)
	{
	    if (MEcmp((PTR)obj_name, &unit->ulh_hname[0], name_len) == 0)
		return(unit->ulh_link.object);
	}
	unit = (ULH_UNIT *) unit->ulh_link.next;
	--count;
    }
    return(NULL);
}


/*{
** Name: ulhi_alias_search	- search an object by alias
**
** Description:
**      Search through an alias bucket for an object matching the given
**	alias.
**
** Inputs:
**      ap                              pointer to the alias bucket
**      alias				pointer to the requested alias string
**      alias_len			length of the alias string.
**
** Outputs:
**	    none
**	Returns:
**	    PTR                         the pointer to the object matching
**					the given alias.  NULL if no
**					match is found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-nov-87 (puree)
**          implemented for alias support.
**	06-apr-90 (andre)
**	    modify for multiple alias support (hash bucket list will contain
**	    ULH_HALIAS structures, not ULH_HOBJ)
*/
static PTR
ulhi_alias_search( ULH_BUCKET *ap, unsigned char *alias_name, i4  alias_len )
{
    register ULH_HALIAS	*alias;
    register i4	count;

    if ( alias_len == 0 || (count = ap->b_count) == 0 )
	return(NULL);			/* don't search an empty bucket
					**  or a null string */

    for (alias = (ULH_HALIAS *) ap->b_queue.head->object;   /* first alias */
	 count > 0;
	 alias = (ULH_HALIAS *) alias->ulh_alink.next->object, count--)
    {
	if (alias->ulh_alias_len == alias_len)
	{
	    if (MEcmp((PTR)alias_name, alias->ulh_halias, alias_len) == 0)
		return((PTR) alias->ulh_obj);
	}
    }
    return((PTR)NULL);
}


/*{
** Name: hash	- compute a hash key from given name
**
** Description:
**      Compute the hash key of a given name string.
**
** Inputs:
**      obj_name                        pointer to name string
**      name_len                        length of name
**      tabsize                         size of the hash table
**
** Outputs:
**      none 
**	Returns:
**	    i4				The hash key
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
#define		SHFCNT	    2
#define		KEYSIZE	    8*sizeof(i4)/SHFCNT
#define		HCOUNT	    (ULH_MAXNAME + KEYSIZE)/2
static i4
ulhi_hash( unsigned char *obj_name, i4  name_len, i4  tabsize )
{
    register i4     count1, count2;
    i4	    hash1, hash2;
    i4		    hashkey;

    if (name_len > HCOUNT)
    {
	count1 = HCOUNT;
	count2 = name_len - HCOUNT;
    }
    else
    {
	count1 = name_len;
	count2 = 0;
    }

    hash1 = 0L;
    hash2 = 0L;

    while (count1)
    {
	hash1 = (hash1 + (i4)(*obj_name)) << SHFCNT;
	hash2 += (i4)(*obj_name);
	++obj_name;
	--count1;
    }
    while(count2)
    {
	hash2 += (i4)(*obj_name);
	++obj_name;
	--count2;
    }

    hashkey = (i4)(abs((hash1 + hash2)  % tabsize));
    return(hashkey);
}

/*{
** Name: ulhi_notprime	- Internal routine for ULH.
**
** Description:
**      This function determines if the input argument is a prime
**	number or not.
**
** Inputs:
**      i   a positive integer number
**
** Outputs:
**	none
**
**	Returns:
**	    0 if the input argument is a prime number
**	    1 otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-dec-86   puree
**          Initial programming
*/
static i4
ulhi_notprime( i4  i )
{
    register i4     x, divisor;

    x = i;
    if ((x & 1) == 0)
	return(1);
    divisor = 3;
    while ((divisor * divisor) <= x)
    {
	if ((x % divisor) == 0)
	    return(1);
	divisor +=2;
    }
    return(0);
}

/*{
** Name: ulhi_alloc	- allocate a memory block
**
** Description:
**      This function allocates a block of memory from a given 
**      memory resource as specified in ulm_rcb.  The main purpose
**      of implementing this routine is to have error checking and 
**      for error code formating code in one place.
**
** Inputs:
**      ulh_rcb                         ULH_RCB pointer for error code
**					formating purpose.
**      ulm_rcb                         ULM_RCB pointer for interfacing
**					to ULM.
**      errcode                         error code to return in ulh_rcb.
**					 ulh_error.err_code if error occurs.
**      flag                            flag of set, close the memory stream
**					 in case of error.
**
** Outputs:
**	Returns:
**	    i4				0 if OK
**					1 if error occurs.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
\**
** History:
**      08-jan-87 (puree)
**          initial version
*/
static DB_STATUS
ulhi_alloc( ULH_RCB *ulh_rcb, ULM_RCB *ulm_rcb, i4 errcode, i4  flag )
{
    DB_STATUS	    status;

    if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
    {
	SETDBERR(&ulh_rcb->ulh_error, ulm_rcb->ulm_error.err_data, errcode);

	/* if flag is set, close memory stream */
	if (flag)
	    (VOID) ulm_closestream(ulm_rcb);
	return(status);
    }
    return(E_DB_OK);
}
