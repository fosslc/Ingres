/*
** Copyright (c) 1988, 2008 Ingres Corporation
**
*/

# include <prtdef.h>
# include <ssdef.h>
# include <psldef.h>
# include <secdef.h>
# include <vadef.h>
# include <compat.h>
# include <gl.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <si.h>
# include <nm.h>
# include <tr.h>
# include <starlet.h>
# include <builtins.h>

/**
**
**  Name: MEPAGES.C - A Page based memory allocator
**
**  Description:
**	MEPAGES grew out of the need for a page based allocator which would
**	allow the exploitation of shared memory and other "special" memory.
**	This abstraction also supports returning pages to the operating
**	system (if conditions allow.)
**
**	NOTICE: These routines are not necessarily re-entrant.  Semaphores
**	must be added to make that so.
**
**	Shared Memory Issues:
**	This file provides definitions for shared memory operations.  Since
**	Shared memory operations will only be available on a subset of machines
**	on which ingres runs, one must only use these routines to improve
**	performance rather than add functionality.
**
**	Mainline clients of these routines must provide the same functionality
**	if these features are not available.  An example of this is the global
**	cache.  On machines with shared memory the global cache will be 
**	implemented.  On machines which lack this feature the global cache will
**	not be implemented but the interface to the i/o system will be the
**	same except that it may run slower.
**
**	OUTSTANDING ISSUES:
**	
**	1) Permissions on the shared memory segment.  (UNIX Example)
**
**	   On unix all shared memory interfaces provided, require permission
**	   on the segment to map to normal unix file permissions.  This is not
**	   a current problem with shared memory usage in the dbms server, since
**	   permission on these segments will be made 600 (only readable and
**	   writable by the creator).  The creator in this case will be programs
**	   which have been made setuid to ingres.
**
**	   The problem will show up if shared memory is used to communicate
**	   between front end and dbms server.  The (unpleasant) options
**	   include:  (1) Set permissions to be world readable and writable
**	   so that both a random user and ingres can read and write to the
**	   segment. (2) Kludge the CL by making the server setuid root
**	   (which has always been an unwelcome option to our users).
**	   The security problem can be handled by a user enabled option in the 
**	   main-line to encode the data.
**
**	2) ME_USER_ALLOC and shared memory
**
**	   Because we allow user applications to use their system provided
**	   stream based memory allocator, some difficulties will appear if
**	   we wish to map shared memory into the user's address space.  This
**	   problem is extremely environment dependent; on UNIX, this is
**	   probably the most non-portable issue possible.  Fundamentally,
**	   the problem is how to prevent the user memory allocator from
**	   interference with foreign shared memory being added to a process'
**	   address space.  Neither the current INGRES product set nor this
**	   proposal require the configuration of shared memory into a user
**	   process' address space, hence we simply disallow MEget_pages,
**	   MEfree_pages, MEprot_pages, and MEsmdestroy during
**	   MEadvise(ME_USER_ALLOC).  The other ME allocation routines
**	   can be built on the user allocator without using MEget_pages.
**
**	This file defines:
**		MEadvise()      - Advise if user allocation nessasary
**		MEget_pages()	- Allocate shared or unshared memory pages.
**		MEfree_pages()	- return allocated pages to the free page pool.
**		MEprot_pages()	- Set protection attributes on a page range.
**		MEsmdestroy()	- Destroy an existing shared memory segment.
**		MEshow_pages()	- Show shared memory segments.
**
**  History:
**      23-feb-88 (anton)
**	    Created.
**	03-May-88 (anton)
**	    Clarifications.
**	13-jul-88 (anton)
**	    Integrated into ME
**	22-feb-89 (greg)
**	    CL_ERR_DESC member error must be initialized to OK
**	28-feb-89 (rogerk)
**	    Implemented shared memory routines for VMS.  Added MEshow_pages
**	    routine.  Added ME_NOTPERM_MASK option to MEget_pages.
**	27-mar-89 (rogerk)
**	    Took out LOpurge when create file in ME_makekey.
**	15-may-89 (rogerk)
**	    Folded in changes according to the approved CL spec for shared
**	    memory interface changes.  Added allocated_pages parameter to
**	    MEget_pages and changed shared memory key to be a character string
**	    instead of a LOCATION pointer.
**	19-may-89 (rogerk)
**	    When calculated bytes allocated from the ending memory address of
**	    the allocated segment, add one to last byte address to get correct
**	    byte count.
**	18-aug-1989 (greg)
**	    MEadvise returns STATUS.  Do so, even though a NO-OP on VMS.
**	02-mar-1990 (neil)
**	    Supress status in MEsmdestroy (to allow cleanup to continue).
**	    Took out LOpurge from ME_destroykey.
**      18-mar-92 (jnash)
**          In MEget_pages, add ability to lock shared memory.
**	20-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototypes
**	01-may-95 (albany)
**	    Integrated markg's and ralph's changes from 6.4 AXP/VMS CL:
**		10-nov-1994 (markg)
**	    	    Put in Alpha specific change when allocating shared memory 
**	            so that the expected number of allocated pages are returned.
**		26-aug-1992 (ralph)
**	    	    Bug 40070: Make MEget_pages() allocate shared segments with
**	    	    permissions of S:RWED,O:RWED,G:,W:  (change #267170)
**      22-Aug-2000 (horda03)
**         Provide facility for allocating memory from the P1 space.
**         (102291)
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   forward external references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	19-may-2005 (devjo01)
**	    Shared memory key files go into II_SYSTEM:[files.memory.<host>]
**	    if clustered.
**	13-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Replace uses of
**	    CL_SYS_ERR by CL_ERR_DESC.
**      08-mar-2009 (stegr01)
**          Use VMS defined constants for VA defs
**          Add dummy function MEdetach to satisfy linker
**          Note that this would only be called from csev if a null address
**          had been supplied - so is currently redundant for all practical purposes
**      16-Oct-2009 (horda03) Bug 122742
**          If memory request from P0 space fails, try the P1 space. That is
**          nearly double the memory that Ingres can use. Each space is 1GB.
**          When 64bit pointers available, then the much larger P2 space can 
**          be utilised.
**/

/*
**  Forward and/or External typedef/struct references.
*/
STATUS ME_getkey();
STATUS ME_makekey();
STATUS ME_destroykey();
/*
** Address to specify when mapping global section in order to locate the
** section in program (P0) space or control (P1) space.
*/
#define	    ME_P0_SPACE	    0x00000200
#define     ME_P1_SPACE     VA$M_P1 | 0x0200

/*
** II_FILES subdirectory that is used by ME to create files to associate
** with shared memory segments.  These files are used by MEshow_pages to
** track existing shared memory segments.
*/
#define	    ME_SHMEM_DIR    "memory"

/*
** Maximum length of shared memory segment name.
*/
#define	    ME_SECNAME_MAX  25

/*
** Definition of all global variables used by this file.
*/

/*
** Definition of all global variables owned by this file.
*/

/*
**  Definition of static variables and forward static functions.
*/

/*
** Implementation Notes:
**
**	Implementation of Shared Memory routines on VMS:
**
**	    Shared Memory Segments on VMS are identified by a character
**	    string name.  This name is derived from the user supplied
**	    character string key.
**
**	    To build a vms key, we prepend "II_", then the 2-letter installation
**	    code, then "_" and then the user key.
**
**	    Since there is no way on VMS for a program to query the system to
**	    find out what shared segments exist, ME must keep track of all
**	    memory segments it creates in order for MEshow_pages to work.
**
**	    Each time a shared segment is created, a file will be created in
**	    a subdirectory of II_CONFIG (II_CONFIG:[MEMORY]) that identifies
**	    that shared segment.  The filename will be the user supplied key
**	    name.  MEshow_pages will be able to list the contents of this
**	    directory to find existing memory segments.
**
**	    ME routines should note that the existence of such a file does not
**	    guarantee the existence of a corresponding shared memory segment
**	    (however the reverse is guaranteed to be true).
*/

/*
**      Function
**              MEadvise
**
**      Arguments
**              i4     mode;
**
**      Result
**              supposed to set allocator to use malloc() or ingres allocator
**              mechanism.  NO-OP on VMS for now.
*/
STATUS
MEadvise(
	i4 mode)
{
    return(OK);
}

/*{
** Name: MEget_pages  - add (possibly shared) memory to the client
**
** Description:
**
**      This routine adds pages to the virtual address space of the requesting
**	program. This routine is intended to be used by the dbms server for all
**	dynamic memory allocation.  It adds pages which are ME_MPAGESIZE bytes 
**	in length, and are guaranteed to be aligned on ME_MPAGESIZE byte 
**	boundaries.
**
**	Implementor's of the ME module can assume that MEget_pages will be
**	the exclusive allocator of memory when MEadvise(ME_INGRES_ALLOC).
**
**      This routine allocates some number of pages.  The memory 
**      allocated here is considered to be long-term memory (on the order
**      of session or server length of time).  All memory returned by this 
**      service is page (ME_MPAGESIZE bytes) aligned, and the unit of measuring
**      of this memory is pages (i.e. requests come in for number of pages, 
**      not number of bytes).  Note that this page size must be variable
**	from environment to environment.  This is necessary to adjust for
**	environmental performance differences in paging activity.
**
**	When allocating shared memory, even more restrictive conditions
**	may apply, e.g.
**(	  1) the allocation size being greater
**	  2) the alignment boundaries are further apart
**	  3) only specific regions of the address space	can support shared
**	     segments.
**)
**	The various restrictions on shared memory are derived from a machine's
**	hardware architecture and the operating system's allocation primitives.
**	Because these restrictions are so varied, MEget_pages is not allowed
**	during MEadvise(ME_USER_ALLOC).  See 'OUTSTANDING ISSUES' above.
**
**	Attaching to a shared segment is performed by specifying either
**	ME_MSHARED_MASK or ME_SSHARED_MASK in the flags argument and by not
**	specifying ME_CREATE_MASK.  When attaching to an already existing
**	shared segment the user supplied 'pages' argument is ignored, the
**	user recieves all the pages that belong to the shared segment. This
**	number is returned in the 'allocated_pages' parameter.
**
**	When creating or attaching to shared memory segments, a key parameter
**	must be specified which uniquely identifies this shared segment within
**	the current Ingres Installation.  The key parameter is a character
**	string that must follow the Ingres file naming conventions: 
**	"prefix.suffix" where prefix is at most 8 characters and suffix is
**	at most 3.  All characters must be printable.  ME may use this key
**	to create a file to identify the shared memory segment, although this
**	is up to the particular implementation of ME.
**
** Inputs:
**      flag                            Flag specifying special operations on
**					allocated memory. (advisorys)
**					Can be or'ed together.
**		    ME_NO_MASK		None of the other flags apply.
**                  ME_MZERO_MASK       zero the memory before returning
**		    ME_MERASE_MASK	"erase" the memory (DoD) 
**					 ...on systems where applicable
**		    ME_IO_MASK		Memory used for I/O
**		    ME_SPARSE_MASK	Memory used `sparsely' so should
**					be paged in small chunks if applicable
**		    ME_LOCKED_MASK	Memory should be locked down to inhibit
**					paging -- i.e. buffer caches.
**		    ME_SSHARED_MASK	Memory shared across processes on a
**					single machine
**		    ME_MSHARED_MASK	Memory shared with other processors.
**		    ME_CREATE_MASK	Create a new shared memory segment.
**					(Usually specified with ME_MZERO_MASK)
**					It is an error if the specified segment
**					already exists.
**		    ME_NOTPERM_MASK	Create temporary shared memory segment
**					rather than a permanent one (on systems
**					where this is supported).
**
**		    The flags ME_SSHARED_MASK, ME_MSHARED_MASK cannot both be
**		    specified in an MEget_pages call.  One of these flags must
**		    be specified in order for ME_CREATE_MASK or ME_NOTPERM_MASK
**		    flags to be used.
**
**      pages                           number of pages requested
**					NOTE: if attaching to an already
**					    existing shared memory segment,
**					    the pages argument is ignored.
**
**	key				A character string identifier for the
**					a shared memory segment.  This argument
**					is only requried with shared memory
**					operations - when not needed, use NULL.
**
**					The key identifier should be a character
**					string that follows the Ingres file
**					naming conventions: 8 characters max
**					prefix name and 3 characters max suffix.
**
** Outputs:
**
**	memory				Upon success the address of the
**					allocated memory.
**	allocated_pages			Number pages of shared memory returned.
**					Normally, this will always match the
**					'pages' input parameter.  If attaching
**					to an existing shared memory segment,
**					this will give the actual size of the
**					attached segment.
**	err_code			System specific error information.
**
**	Returns:
**		OK			If all went well.
**					Otherwise, set to: 
**		ME_OUT_OF_MEM		Could not allocate required amount of 
**					memory.
**
**		ME_BAD_PARAM		Illegal parameter passed to routine
**					(pages <= 0, ...)
**
**		ME_ILLEGAL_USAGE	Combination of attributes is invalid
**
**		ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**
**		ME_ALREADY_EXISTS	shared memory identifier already exists
**
**		ME_NO_SUCH_SEGMENT	specified shared memory segment does
**					not exist
**		ME_NO_SHARED		system can not support shared memory
**
**	Exceptions:
**	    none
**
** Side Effects:
**	File may be created in ME_SHMEM_DIR directory to identify shared segment
**
** History:
**	21-Jan-88 (mmm)
**          Proposed.
**	23-feb-88 (anton)
**	    combined shared memory spec and first cut at implementing.
**	11-jul-88 (anton)
**	    integrate as part of ME
**	30-jan-89 (rogerk)
**	    Added shared memory implementation.  Added ME_NOTPERM_MASK option.
**	15-may-89 (rogerk)
**	    Added allocated_pages parameter and changed memory segement key
**	    from LOCATION ptr to character string.
**	19-may-89 (rogerk)
**	    When calculated bytes allocated from the ending memory address of
**	    the allocated segment, add one to last byte address to get correct
**	    byte count.
**	18-mar-92 (jnash)
**	    Add ability to lock shared memory.
**	01-may-95 (albany)
**	    Integrated markg's changes from 6.4 AXP/VMS CL:
**		10-nov-1994 (markg)
**	    	    Put in Alpha specific change when allocating shared memory 
**	            so that the expected number of allocated pages are returned.
**      22-Aug-2000 (horda03)
**         Handle new flag (ME_USE_P1_SPACE) which indates that the pages requested
**         should be obtained from the control (P1) space.
**         (102291)
*/
STATUS
MEget_pages(
	i4		flag,
	SIZE_TYPE	pages,
	char		*key,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    PTR		map_in_addr[2];
    PTR		map_out_addr[2];
    PTR		lock_out_addr[2];
    STATUS	status;
    u_i4	vms_flags;
    char	os_key[ME_SECNAME_MAX];
    struct
    {
	long    gsd_name_length;	
	char    *gsd_name_value;
    } gsd_desc;

    /*
    ** Should outline implementation details here - rogerk
    */

    CL_CLEAR_ERR(err_code);

    /*
    ** If flag does not specify shared memory then just allocate new
    ** local memory to this process.
    */
    if ((flag & (ME_SSHARED_MASK | ME_MSHARED_MASK)) == 0)
    {
        i4 region;

        /* Make memory request in approprate region, if no memory available
        ** try the next (if one exists).
        */
        for (region = (flag & ME_USE_P1_SPACE) ? 1 : 0; region <= 1; region++)
        {
         status = sys$expreg(pages, map_out_addr, 0, region);

           if (status & 1) break;
        }

	if ((status & 1) == 0)
	{
	    err_code->error = status;
	    return(ME_OUT_OF_MEM);
	}

        if (region == VA$C_P1)
        {
           /* Expanding Control region; so reverse returned address */ 
           PTR i_p = map_out_addr[0];

           map_out_addr[0] = map_out_addr[1];
           map_out_addr[1] = i_p;
        }

	*memory = map_out_addr[0];
	*allocated_pages = (map_out_addr[1] - map_out_addr[0] + 1) / 
								ME_MPAGESIZE;
   
	return(OK);
    }
    else
    {
	/*
	** Allocate shared memory segment (or just connect to already
	** existing one).
	*/

	/*
	** Fill in vms descriptors needed to create or map shared memory
	** segment.  The map_in_addr argument specifies to map the memory
	** segment into program space.
	*/
        if (flag & ME_USE_P1_SPACE)
        {
           map_in_addr[0] = ME_P1_SPACE;
           map_in_addr[1] = ME_P1_SPACE;
        }
        else
        {
	   map_in_addr[0] = ME_P0_SPACE;
	   map_in_addr[1] = ME_P0_SPACE;
        }

	gsd_desc.gsd_name_value = os_key;

	/*
	** If the CREATE flag is not specified, then just map the already
	** existing segment into this process.
	*/
	if ((flag & ME_CREATE_MASK) == 0)
	{
	    status = ME_getkey(key, os_key, ME_SECNAME_MAX, err_code);
	    if (status != OK)
		return (ME_NO_SUCH_SEGMENT);

	    gsd_desc.gsd_name_length = STlength(os_key);
	    vms_flags = (SEC$M_WRT | SEC$M_SYSGBL | SEC$M_EXPREG);

	    status = sys$mgblsc(map_in_addr, map_out_addr, PSL$C_USER,
		vms_flags, &gsd_desc, 0, 0);
	    if ((status & 1) == 0)
	    {
		err_code->error = status;
		return (ME_NO_SUCH_SEGMENT);
	    }
	}
	else
	{
	    /*
	    ** Create a new shared memory segment.  It is an ME error to
	    ** specify the CREATE flag when the segment already exists.
	    **
	    ** Unless the ME_NOTPERM_MASK is specified, make the memory
	    ** segment permanent.
	    */

	    /*
	    ** Check for an already existing key for this shared segment.
	    ** We should reuse the old key when the segment is lost due
	    ** to a system crash.
	    **
	    ** If there is not one, then create a new one.
	    */
	    status = ME_getkey(key, os_key, ME_SECNAME_MAX, err_code);
	    if (status != OK)
	    {
		status = ME_makekey(key, os_key, ME_SECNAME_MAX, err_code);
		if (status != OK)
		    return (ME_BAD_PARAM);
	    }

	    gsd_desc.gsd_name_length = STlength(os_key);
	    vms_flags = (SEC$M_GBL | SEC$M_WRT | SEC$M_SYSGBL |
		SEC$M_EXPREG | SEC$M_PAGFIL);
	    if ((flag & ME_NOTPERM_MASK) == 0)
		vms_flags |= SEC$M_PERM;

	    status = sys$crmpsc(map_in_addr, map_out_addr, PSL$C_USER,
		vms_flags, &gsd_desc, 0, 0, 0, pages, 0,
		(u_i4)0x0000ff00,	/* Bug 40070 - S:RWED,O:RWED,G:,W: */
		0);

	    /*
	    ** Check completion status.
	    ** If VMS returns successfull but not CREATED, then the memory
	    ** segment already existed.  This is an error.  Unmap the memory
	    ** segment.
	    */
	    if (status != SS$_CREATED)
	    {
		if (status & 1)
		{
		    status = sys$deltva(map_out_addr, 0, 0);
		    err_code->error = status;
		    return (ME_ALREADY_EXISTS);
		}
		else
		{
		    err_code->error = status;
		    return (ME_OUT_OF_MEM);
		}
	    }
	}

	/* Shared memory should be locked if requested */
	if (flag & ME_LOCKED_MASK)
	{
	    status = sys$lckpag(map_out_addr, lock_out_addr, vms_flags);
	    if ((status & 1) == 0)
	    {
		TRdisplay("MEget_pages: sys$lckpag request failed, status:%d\n", 
			status);
		err_code->error = status;
		return (ME_LCKPAG_FAIL);
	    }

	    TRdisplay("MEget_pages: Locked %d pages in memory\n", 
		  (lock_out_addr[1] - lock_out_addr[0] + 1) / ME_MPAGESIZE);
	}

	*memory = map_out_addr[0];
	*allocated_pages = (map_out_addr[1] - map_out_addr[0] + 1) / 
								   ME_MPAGESIZE;
	/* 
	** On AXP VMS the value for allocated pages can be greater than the 
	** number of pages that were requested because of O/S page 
	** alignment. This screws up checks that are made in DMF and
	** causes shared cache servers to fail.
	**
	** On AXP VMS, if the allocated pages is larger than the
	** requested pages, set allocated to requested.
	*/
	if (*allocated_pages > pages)
	    *allocated_pages = pages;

	return (OK);
    }
}

/*{
** Name: MEfree_pages()	- Free pages back to OS
**
** Description:
**	Routine will, if possible, return pages to OS.  Memory to be freed
**	must have been allocated using the MEget_pages() call.  Memory to
**	be freed need not be exact extent allocated by a MEget_pages() call
**	(for example one MEfree_pages() call can be used to free memory 
**	allocated by two MEget_pages() calls assuming the memory was 
**	contiguous; or one could free half of the memory allocated by a single
**	MEfree_pages() call.)
**
**	Note: The memory might not be freed immediately.  On Unix, memory
**	must be contiguous and freed in LIFO order.  Thus the memory will not
**	returned to the OS until all memory above it is also freed.
**
**	Shared Memory Note: Shared memory often must be released in an
**	all-or-nothing manner so again the actual release may be postponed.
**
** Inputs:
**      address                         address of the 1st allocated block of 
**					memory to be freed.
**      num_of_pages                    number of pages to be freed.
**
** Outputs:
**      err_code                        system dependent error code.
**
**	Returns:
**	    OK				Free operation succeeded.
**	    ME_BAD_PARAM		Parameter is invalid
**	    ME_NOT_ALLOCATED		Pages to be freed were not allocated
**	    ME_OUT_OF_MEM		should never happen (OS
**						doesn't want memory back)
**	    ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-feb-88 (anton)
**	    combined shared memory spec and first cut at implementing.
**      21-jan-87 (mmm)
**          proposed
*/
STATUS
MEfree_pages(
	PTR		address,
	SIZE_TYPE	num_of_pages,
	CL_ERR_DESC	*err_code)
{
    PTR		bounds[2];
    i4	status;

    CL_CLEAR_ERR(err_code);
    if (num_of_pages < 0)
    {
	return(ME_BAD_PARAM);
    }
    bounds[0] = address;
    bounds[1] = (char *)bounds[0] + num_of_pages * ME_MPAGESIZE - 1;
    status = sys$deltva(bounds, 0, 0);
    if ((status & 1) == 0)
    {
	err_code->error = status;
	return(ME_OUT_OF_MEM);
    }
    return(ME_OK);
}

/*{
** Name: MEprot_pages()	- Set protection attributes on a page range
**
** Description:
**	For host OS' which support it, modify access attributes on a page
**	range.  Possible uses of this routine include stack overflow warning,
**	facility and/or session verification traces of valid memory use, 
**	protection from dynamically linked user code in INGRES proprietary
**	processes, ...
**
**	Mainline (user accessible) functionality can not depend on this
**	interface; improved error handling, enhanced debugging capabilities,
**	etc. are the intended clients.  Clients are not expected to ifdef
**	use of this interface.
**
**	Setting attributes on a shared segment only affects the client
**	that calls MEprot_pages.
**
** Inputs:
**	addr				start address of page range
**	pages				number of pages in range
**	protection			protections requested:
**			ME_PROT_READ
**			ME_PROT_WRITE
**			ME_PROT_EXECUTE
**			ME_PROT_NONE
**
** Outputs:
**	return status
**
**	Returns:
**	    OK				Protections could be set.
**	    ME_NOT_ALLOCATED		Pages are not allocated.
**	    ME_NOT_SUPPORTED		OS can not comply with request
**	    ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**	    ME_NO_PERM			No permission to make request.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Memory accesses which violate set permissions will generate
**	OS specific exceptions.
**
** History:
**      03-may-88 (anton)
**          Created.
*/
STATUS
MEprot_pages(
	PTR		addr,
	SIZE_TYPE	pages,
	i4		protection)
{
    PTR		bounds[2];
    i4	status;
    i4	prot;

    bounds[0] = addr;
    bounds[1] = (char *)bounds[0] + pages * ME_MPAGESIZE - 1;
    prot = PRT$C_NA;
    switch(protection)
    {
    case ME_PROT_READ:
    case ME_PROT_EXECUTE:
	prot = PRT$C_UR;
	break;
    case ME_PROT_WRITE:
	prot = PRT$C_UW;
	break;
    }
    status = sys$setprt(bounds, 0, 0, prot, 0);
    if  ((status & 1) == 0)
    {
	if (status == SS$_LENVIO)
	    return(ME_NOT_ALLOCATED);
	return(ME_NO_PERM);
    }
    return(ME_OK);
}

/*{
** Name: MEsmdestroy()	- Destroy a shared memory segment
**
** Description:
**	Remove the shared memory segment specified by shared memory identifier
**	"key" from the system and destroy any system data structure associated 
**	with it.
**
**	Note: The shared memory pages are not nessasarily removed from
**	processes which have the segment mapped.  It is up to the clients to
**	detach the segment via MEfree_pages prior to destroying it.
**
**	Protection Note: The caller of this routine must have protection to
**	destroy the shared memory segment.  Protections are enforced by the
**	underlying Operating System.  In general, this routine can only be
**	guaranteed to work when executed by a user running with the same
**	effective privledges as the user who created the shared memory segment.
**
** Inputs:
**	key			identifier which was previosly 
**				used in a successful MEget_pages() call 
**				(not necessarily a call in this process)
**
** Outputs:
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_NO_PERM		No permission to destroy shared memory
**				segment.
**	    ME_NO_SUCH_SEGMENT	indicated shared memory segment does not
**				exist.
**	    ME_NO_SHARED	No shared memory in this CL.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-Jun-88 (anton)
**          Created.
**	15-may-89 (rogerk)
**	    Changed memory segement key from LOCATION ptr to character string.
**	02-mar-1990 (neil)
**	    If MEfree_pages is called first the system may return that the
**	    segment is missing (though we still want to destroy the key).
*/
STATUS
MEsmdestroy(
	char		*key,
	CL_ERR_DESC	*err_code)
{
    PTR		    bounds[2];
    char	    os_key[ME_SECNAME_MAX];
    i4	    status, xstat;
    CL_ERR_DESC	    error;
    struct
    {
	long    gsd_name_length;	
	char    *gsd_name_value;
    } gsd_desc;

    CL_CLEAR_ERR(err_code);

    /*
    ** Get segment name from LOCATION id.
    ** Ignore NO_SUCH_SEGMENT return code - this indicates that the key was
    ** valid, but there is no ME knowledge of it.  Try to destroy shared
    ** segment anyway.
    */
    status = ME_getkey(key, os_key, ME_SECNAME_MAX, &error);
    if (status != OK && status != ME_NO_SUCH_SEGMENT)
    {
	err_code->error = SS$_NOSUCHSEC;
	return (ME_NO_SUCH_SEGMENT);
    }

    /*
    ** Destroy shared memory segment.
    */
    gsd_desc.gsd_name_length = STlength(os_key);
    gsd_desc.gsd_name_value = os_key;

    status = sys$dgblsc(SEC$M_SYSGBL, &gsd_desc, 0);
    /*
    ** If this fails then it may be because the system freed the
    ** segment in MEfree_pages.  Still make sure we get rid of the
    ** key.  If we got rid of the key then ignore the system
    ** status. (neil)
    */
    if ((status & 1) == 0)
    {
	xstat = ME_destroykey(key, &error);
	if (xstat != OK)
	{
	    err_code->error = status;
	    if (status == SS$_NOPRIV)
		return (ME_NO_PERM);
	    return (ME_NO_SUCH_SEGMENT);
	}
    }
    else
    {
	status = ME_destroykey(key, err_code);
    }
    return(OK);
}

/*{
** Name: MEshow_pages()	- show system's allocated shared memory segments.
**
** Description:
**	This routine is used by clients of the shared memory allocation
**	routines to show what shared memory segments are currently allocated
**	by the system.
**
**	The routine takes a function parameter - 'func' - that will be
**	called once for each existing shared memory segment allocated
**	by Ingres in the current installation.
**
**      The client specified function must have the following call format:
**(
**          STATUS
**          func(arg_list, key, err_code)
**          i4        *arg_list;       Pointer to argument list. 
**          char       *key;		Shared segment key.
**          CL_ERR_DESC *err_code;       Pointer to operating system
**                                      error codes (if applicable).
**)
**
**	The following return values from the routine 'func' have special
**	meaning to MEshow_pages:
**
**	    OK (zero)	- If zero is returned from 'func' then MEshow_pages
**			  will continue normally, calling 'func' with the
**			  next shared memory segment.
**	    ENDFILE	- If ENDFILE is returned from 'func' then MEshow_pages
**			  will stop processing and return OK to the caller.
**
**	If 'func' returns any other value, MEshow_pages will stop processing
**	and return that STATUS value to its caller.  Additionally, the system
**	specific 'err_code' value returned by 'func' will be returned to the
**	MEshow_pages caller.
**
** Inputs:
**	func			Function to call with each shared segment.
**	arg_list		Optional pointer to argument list to pass
**				to function.
**
** Outputs:
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_NO_PERM		No permission to show shared memory segment.
**	    ME_BAD_PARAM	Bad function argument.
**	    ME_NO_SHARED	No shared memory in this CL.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-Apr-89 (rogerk)
**          Created for Terminator.
*/
STATUS
MEshow_pages(
	STATUS		(*func)(),
	PTR		*arg_list,
	CL_ERR_DESC	*err_code)
{
    LOCATION	locinptr;
    LOCATION	locoutptr;
    STATUS	status;
    char	file_buf[LO_FILENAME_MAX + 1];
    char	dev[LO_DEVNAME_MAX + 1];
    char	path[LO_PATH_MAX + 1];
    char	fprefix[LO_FPREFIX_MAX + 1];
    char	fsuffix[LO_FSUFFIX_MAX + 1];
    char	version[LO_FVERSION_MAX + 1];
    char	loc_buf1[MAX_LOC + 1];
    char	loc_buf2[MAX_LOC + 1];

    CL_CLEAR_ERR(err_code);
    /*
    ** Get location ptr to directory to search for memory segment names.
    */
    NMloc(FILES, PATH, (char *)NULL, &locoutptr);
    LOcopy(&locoutptr, loc_buf1, &locinptr);
    LOfaddpath(&locinptr, ME_SHMEM_DIR, &locinptr);
    if ( CXcluster_configured() )
    {
	LOfaddpath(&locinptr, CXnode_name(NULL), &locinptr);
    }
    LOcopy(&locinptr, loc_buf2, &locoutptr);

    /*
    ** For each file in the memory location, call the user supplied function
    ** with the filename.  Shared memory keys are built from these filenames.
    */
    status = OK;
    while (status == OK)
    {
	status = LOlist(&locinptr, &locoutptr);
	if (status == OK)
	{
	    LOdetail(&locoutptr, dev, path, fprefix, fsuffix, version);
	    (VOID) STpolycat(3, fprefix, ".", fsuffix, file_buf);
	    status = (*func)(arg_list, file_buf, err_code);
	}
    }

    if (status == ENDFILE)
	return(OK);

    LOendlist(&locinptr);
    return (status);
}

/*
**  ME_getkey	- Get shared memory key from user key
**
** Description:
**	Build a shared memory key from a user-supplied key.
**
**	We build a VMS shared segment key by append "ii_XX_" (where XX is the
**	two letter ii_installation code) to the front of the user key.
**
**	Check for the existence of a ME file for the user supplied shared
**	memory key.  If one does not exist, return an error.  Even if we
**	return an error here, we must return the VMS_KEY that would describe
**	such a shared segment.
**
** Inputs:
**	user_key		user shared memory key
**	vms_key			buffer to build vms shared segment key into
**	length			length of vms_key buffer
**
** Outputs:
**	vms_key			filled in with shared segment key
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_BAD_PARAM	Bad user key name.
**	    ME_NO_SUCH_SEGMENT	ME file for key name does not exist.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jan-89 (rogerk)
**          Created for terminator project.
**	19-may-2005 (devjo01)
**	    Shared memory key files go into II_SYSTEM:[files.memory.<host>]
**	    if clustered.
*/
STATUS
ME_getkey(
	char		*user_key,
	char		*vms_key,
	i4		length,
	CL_ERR_DESC	*err_code)
{
    LOCATION	    location;
    LOCATION	    temp_loc;
    STATUS	    status;
    char	    *inst_code;
    char	    loc_buf[MAX_LOC + 1];

    CL_CLEAR_ERR(err_code);
    /*
    ** Build VMS key name from user key.  Key name is built from
    ** user key, "II", and the Ingres Installation Code.
    */
    NMgtAt("II_INSTALLATION", &inst_code);
    if (inst_code == NULL || *inst_code == '\0')
	inst_code = "";

    /*
    ** Check that we are not going to overflow the local buffer.
    ** We should be OK since installation code should be 2 bytes,
    ** the filename not more than 8 bytes and the postfix not more
    ** than 3 bytes.
    */
    if ((STlength(inst_code) + STlength(user_key) + 5) > length)
    {
	err_code->error = SS$_BUFFEROVF;
	return (ME_BAD_PARAM);
    }

    /* Build key name */
    STpolycat(4, "II_", inst_code, "_", user_key, vms_key);

    /*
    ** Get location of ME files for shared memory segments.
    */
    NMloc(FILES, PATH, (char *)NULL, &temp_loc);
    LOcopy(&temp_loc, loc_buf, &location);
    LOfaddpath(&location, ME_SHMEM_DIR, &location);
    if ( CXcluster_configured() )
    {
	LOfaddpath(&location, CXnode_name(NULL), &location);
    }
    LOfstfile(user_key, &location);

    /*
    ** Check for existence of file indicated by user key.
    */
    status = LOexist(&location);
    if (status != OK)
    {
	err_code->error = SS$_NOSUCHFILE;
	return (ME_NO_SUCH_SEGMENT);
    }

    return (OK);
}

/*
**  ME_makekey	- build shared memory key
**
** Description:
**	Build the shared memory key and then create an ME file to mark
**	that this segment has been created.
**
**	The key is derived from the filename by appending "ii_XX_" (where
**	XX is the two letter ii_installation code) to the front of it.
**
** Inputs:
**	user_key		user supplied shared memory key
**	vms_key			pointer to buffer to fill in VMS segment key
**	length			length of key buffer
**
** Outputs:
**	vms_key			filled in with vms shared segment key.
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_BAD_PARAM	bad key name.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	File is created in MEMORY directory to identify shared segment.
**
** History:
**      30-jan-89 (rogerk)
**          Created for terminator project.
**	27-mar-89 (rogerk)
**	    Took out LOpurge when create file.
**	19-may-2005 (devjo01)
**	    Shared memory key files go into II_SYSTEM:[files.memory.<host>]
**	    if clustered.
*/
STATUS
ME_makekey(
	char		*user_key,
	char		*vms_key,
	i4		length,
	CL_ERR_DESC	*err_code)
{
    LOCATION	    location;
    LOCATION	    temp_loc;
    FILE	    *desc;
    char	    *inst_code;
    char	    loc_buf[MAX_LOC + 1];
    STATUS	    status;

    CL_CLEAR_ERR(err_code);
    /*
    ** Build VMS key name from user key.  Key name is built from
    ** user key, "II", and the Ingres Installation Code.
    */
    NMgtAt("II_INSTALLATION", &inst_code);
    if (inst_code == NULL || *inst_code == '\0')
	inst_code = "";

    /*
    ** Check that we are not going to overflow the local buffer.
    ** We should be OK since installation code should be 2 bytes,
    ** the filename not more than 8 bytes and the postfix not more
    ** than 3 bytes.
    */
    if ((STlength(inst_code) + STlength(user_key) + 5) > length)
    {
	err_code->error = SS$_BUFFEROVF;
	return (ME_BAD_PARAM);
    }

    /* Build key name */
    STpolycat(4, "II_", inst_code, "_", user_key, vms_key);

    /*
    ** Get location of ME files for shared memory segments.
    */
    NMloc(FILES, PATH, (char *)NULL, &temp_loc);
    LOcopy(&temp_loc, loc_buf, &location);
    LOfaddpath(&location, ME_SHMEM_DIR, &location);
    if ( CXcluster_configured() )
    {
	LOfaddpath(&location, CXnode_name(NULL), &location);
    }
    LOfstfile(user_key, &location);

    /*
    ** Create file to identify the shared segment.  This will allow
    ** MEshow_pages to tell what shared segments have been created.
    */
    status = SIopen(&location, "w", &desc);
    if (status != OK)
    {
	err_code->error = SS$_NOSUCHFILE;
	return (ME_BAD_PARAM);
    }
    SIclose(desc);

    return (OK);
}

/*
**  ME_destroykey	- destroy shared memory key
**
** Description:
**	Destroy the ME file used to identify the specified shared segment.
**
** Inputs:
**	user_key		user supplied shared memory key
**
** Outputs:
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_BAD_PARAM	bad key name.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      30-jan-89 (rogerk)
**          Created for terminator project.
**	02-mar-1990 (neil)
**	    Took out LOpurge from ME_destroykey - not server callable.
**	19-may-2005 (devjo01)
**	    Shared memory key files go into II_SYSTEM:[files.memory.<host>]
**	    if clustered.
*/
STATUS
ME_destroykey(
	char		*user_key,
	CL_ERR_DESC	*err_code)
{
    LOCATION	    location;
    LOCATION	    temp_loc;
    char	    loc_buf[MAX_LOC + 1];
    STATUS	    status;

    CL_CLEAR_ERR(err_code);
    /*
    ** Get location of ME files for shared memory segments.
    */
    NMloc(FILES, PATH, (char *)NULL, &temp_loc);
    LOcopy(&temp_loc, loc_buf, &location);
    LOfaddpath(&location, ME_SHMEM_DIR, &location);
    if ( CXcluster_configured() )
    {
	LOfaddpath(&location, CXnode_name(NULL), &location);
    }
    LOfstfile(user_key, &location);

    status = LOexist(&location);
    if (status != OK)
    {
	err_code->error = SS$_NOSUCHFILE;
	return(ME_BAD_PARAM);
    }

    /* LOpurge(&location, 1); -- not server callable */
    LOdelete(&location);

    return (OK);
}

/*
**  MEdetach       - Detached from shared memory
**
** Description:
**      This is a null function on VMS
**
** Inputs:
**      key                user supplied shared memory key
**      address            address of memory
**
** Outputs:
**      err_code                System specific error information.
**
**      Returns:
**          OK
**
**      Exceptions:
**          none
**
** Side Effects:
**      none
**
** History:
**      22-dec-2008 (stegr01)
**          Created.
*/
STATUS
MEdetach (char *key, PTR  address, CL_ERR_DESC *err_code)
{
   return (OK);
}
