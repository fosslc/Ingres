/*
** Copyright (c) 1988, 2010 Ingres Corporation
**
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <meprivate.h>
# include   <cs.h>
# include   <st.h>
# include   <me.h>
# include   <er.h>
# include   <pm.h>
# include   <pc.h>
# include   <lk.h>
# include   <cx.h>

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

# ifdef SYS_V_SHMEM
/* For SEQUENT Dynix 3 bsd, use specially imported shmops */
/*# include   <sys/ipc.h>*/
# include   <clipc.h>
# include   <sys/shm.h>
# endif

# if defined(xCL_077_BSD_MMAP) && !defined(SYS_V_SHMEM)
# ifdef xCL_007_FILE_H_EXISTS
# include <sys/file.h>
# endif
# include <sys/stat.h>
# include <sys/mman.h>		/* this may change on different systems */
# endif

# include   <errno.h>
# include   <er.h>
# include   <lo.h>
# include   <si.h>
# include   <tr.h>
# include   <nm.h>
# ifdef		USE_MEMSYSCALL
# include   <syscall.h>
# endif

/**
**
**  Name: MESHARED.C - System shared memory interface calls
**
**  Description:
**	This file contains procedures to abstract whatever shared memory
**	facilities a paticular dialect of UNIX might provide.
**
**	This is the SUN 3/xxx 3.x implementation.
**
**	NOTES ON IMPLEMENTATION:
**	    This code is based examination of the object file shmsys.o
**		in /lib/libc.a.  We can't use the standard shared memory
**		calls because they force the sun malloc environment to
**		be loaded which we don't want.  To avoid this we use
**		the indirect system call interface syscall(3) to get to
**		the sys5 shared memory called directly in the kernel.
**		The SUN 3/2x0 cpus have a special cache which requires
**		that shared segments be aligned to 128k boundries.
**		The SUN 3.x versions of these calls differ from SVID
**		The differences are as follows:
**			shmat must have an address specified that will
**				allow the segment to reside within the
**				break region.  Failure to do this will
**				result in a system crash. <<<NOTE
**			shmalign returns the required segment alignment
**
**
**
**  History:
**	3-22-89 (markd)
**	   Moved ftok to handy
**	1-30-88 (markd)
**	   Changed to include Sequent Sys. V. header file
**	   Also added ftok function for those system without ftok(3).
**	 2-dec-88 (anton)
**	    massive portability fixs
**	12-jul-88 (anton)
**	    added to ME.
**      25-feb-88 (anton)
**          Created.
**	28-Feb-1989 (fredv)
**	    Include <clconfig.h> and <systypes.h>.
**	12-jun-1989 (rogerk)
**	    Integrated changes for Terminator I - shared buffer manager.
**	    Changed user supplied shared memory key to be a character string
**	    filename that identifies a file in the installation's memory
**	    directory, rather than a LOCATION pointer.
**	    Added MEshow_pages routine.
**	 5-Jul-1989 (rogerk)
**	    Return 'allocated_pages' value from ME_get_shared when creating 
**	    a new segment.  Just set to the number of pages requested.
**	 7-jul-1989 (rogerk)
**	    In MEseg_free, call ME_rem_seg to remove segment entries from
**	    the segment pool.  Reset page count to 0 when shared segment
**	    address is outside of the local memory address range so that
**	    in MEfree_pages we don't test for existence of the pages in
**	    the local memory address space.
**	    In ME_get_shared, return ME_NO_SUCH_SEGMENT if ENOENT returned
**	    from attempt to attach to shared memory segment.
**	15-aug-1989 (fls-sequent)
**	    Treat MEadvice as a flag word instead of an integer.
**	20-sep-1989 (rexl,daveb)
**	    mmap work begun
**	3-Oct-1989 (anton)
**	    mmap support finished
**	24-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Replace MMAP with xCL_077_BSD_MMAP;
**		Add dr5_us5 specific code to prevent multiple shared 
**		memory mappings;
**		Add code for dr6_us5, which objects to incorrect argument
**		count to shmdt();
**		Add the correct xCL_007... around sys/file.h so that this
**		file will compile.
**	31-may-90 (blaise)
**	    Added include <me.h> to pick up redefinition of MEfill().
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		On EINVAL from shmget to create a shared memory segment in
**		ME_get_shared() return ME_OUT_OF_MEM; also return internal to
**		the CL_ERR_DESC the newly defined error
**		E_CL1226_ME_SHARED_ALLOC_FAILURE;
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**	11-june-90 (blaise)
**	    Replaced #ifdef xCL_077_BSD_MMAP with #if defined(xCL_077...) &&
**	    !! defined(SYS_V_SHMEM). This is to prevent boxes which have both
**	    BSD mmap and SysV shmem from attempting to use both. Note that this
**	    means that SysV shmem is the default.
**	26-jul-90 (kirke)
**	    Added back NMloc() line to ME_destroykey() that got lost during
**	    integration.
**	4-sep-90 (jkb)
**	    Add sqs_ptx to the appropriate lists of ifdefs
**      21-Jan-91 (jkb)
**          Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**          "if( MEadvice == ME_USER_ALLOC )" so it works with
**          ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**	2-aug-1991 (bryanp)
**	    Update ME_attach and ME_alloc_shared to add new "key" argument,
**	    and pass the "key" and "flags" arguments to ME_reg_seg. This is
**	    being done in order to add support for DI slaves performing I/O
**	    directly to/from shared memory, without needing to copy buffers.
**      2-mar-92 (jnash)
**          In ME_attach, propagate ME_ADDR_SPEC to mmap call flags parameter.
**	    This was necessary to get mmap to work properly on Sun.
**	22-may-1992 (jnash)
**	    xDEBUG'd TRdisplays after expected function call errors.
**	06-jul-92 (swm)
**	    Added dr6_uv1 and dra_us5 to dr6_us5 code which specifies correct
**	    argument count to shmdt() to get around compiler objections.
**	02-dec-92 (mikem)
**	    su4_us5 6.5 port.  Added ifdef's to call shmdt() with correct
**	    number of arguments.
**	12-dec-92 (terjeb)
**	    Added hp8_us5 to list of machines with one argument to shmdt().
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**              these are now all in either me.h or meprivate.h.
**	24-feb-93 (swm)
**		Combine TRdisplay calls that follow failures so that
**		the first does not cause overwrite of errno before errno
**		is displayed by the second.
**	29-mar-93 (sweeney)
**      	Use new xCL_SHMDT_ONE_ARG. One fewer porting steps.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	20-sep-1993 (kwatts)
**	    Integrated a 6.4 fix from gordonw:
**          Fix a long standing problem with some systems where when
**          freeing shared memory, which isn't attached at high address space,
**          MEfree_pages() fail  because there is persumedly more pages to
**          free locally. This isn't the case. ME doesn't handle shared
**          memory attached at the middle of a process's address space
**          very well, because there are tests spread around testing
**          'MElimit' or 'MEbase', which has nothing to do with shared memory.
**	10-feb-1994 (ajc)
**	    Added hp8_bls specific entries based on hp8*. Also came in line 
**	    with dr5_us5 to prevent multiple shared memory mappings.
**	10-mar-1994 (ajc)
**	    Although coming in line with dr5_us5 fixes csphil, this causes all
**	    sorts of problems with the dbms, so I have backed out this change. 
**              [End of speech]
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in ME_alloc_shared(),
**	    and ME_attach().
**      23-may-1994 (mikem)
**          Bug #63113
**          Added support for locking shared memory allocated with shmget().
**	    New routine ME_lock_sysv_mem() added.
**	24-may-1994 (mikem)
**	    The above change did not work on HP.  Added ifdef's based on the
**	    existence of xCL_SETEUID_EXISTS define.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	01-jun-1995 (canor01)
**	    replaced errno with errnum
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	23-dec-1996 (canor01)
**	    Add hp8_us5 to dr5_us5 and other platforms that cannot re-map
**	    shared memory.
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
**	29-may-1997 (canor01)
**	    Clean up some compiler warnings.
**      29-aug-1997 (musro02)
**          For sqs_ptx, do not declare mmap
**	05-nov-1997 (canor01)
**	    Let all platforms maintain the shm_ids structure, for use by
**	    MEget_seg().  Add MEget_seg() function.
**	23-dec-1997 (muhpa01)
**	    Move shm_ids[] & shm_addrs[] arrays from ME_attach to global area.
**	    Modified MEseg_free() to clear entires in shm_ids & shm_addrs
**	    when seg is freed.  This allows the seg to be subsequently
**	    reattached - for dr5_us5, ib1_us5, & hp8_us5 only.
**	03-mar-1998 (canor01)
**	    The shms_ids_mutex is only used in an OS threads environment,
**	    so put it inside "#ifdef OS_THREADS_USED" so other environments
**	    can still build.
**	03-mar-1998 (somsa01)
**	    Overlayed the meshared.c file from main to oping20. In recent
**	    submissions to oping20, this file went out of whack.
**      07-aug-98 (hweho01)
**          Fixed the initial setting error in MEdetach(),  so the
**          the search operation can run through the queue.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-may-1999 (walro03)
**	    Remove obsolete version string cvx_u42, dr6_ues, dr6_uv1, dra_us5,
**	    sqs_u42, sqs_us5.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (devjo01)
**	    - Sir 103715 - UNIX clusters.  Check if system configured
**	    for clusters, and if so, add node name to path.
**	30-may-2001 (devjo01)
**	    Rename SHMSEG to ME_SHMSEGS to avoid symbol name conflict
**	    with OS header.
**      08-Oct-2002 (hanch04)
**          Cast pcount to a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	05-nov-2002 (devjo01)
**	    Change type of ME_align argument 'pgsize' from 'i4' to 'SIZE_TYPE'.
**	27-oct-2003 (devjo01)
**	    Check returned errno from CXnuma_shmget for ENOSYS which tells
**	    us that this is not supported, in which case we simply do a
**	    regular shared memory allocation.
**       3-Mar-2004 (hanal04) Bug 111888
**          Added static prototype for ME_get_shared() to resolve
**          2.6/0305 (axp.osf/00) mark 2637 build errors.
**	05-Jan-2005 (hanje04) 
**	    Correct prototype for ME_get_shared() to match function.
**	07-Oct-2005 (hweho01) 
**          On Tru64 release 5.1B, shm_segsz is defined as 4 byte (int  
**          type) in sys/shm.h file if _XOPEN_SOURCE is less than 500 
**          and _KERNEL is not defined. That is not sufficient to address 
**          the large shared memory. To overcome the problem, need to  
**          access the address of shm_segsz and retrieve the 8 byte data.     
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**/


/*
** Definition of all global variables used by this file.
*/

GLOBALREF	QUEUE		ME_segpool;
GLOBALREF	char		*MEbase, *MElimit;

/*
** Definition of all global variables owned by this file.
*/

/*
**  Definition of static variables and forward static functions.
*/

/*
** II_FILES subdirectory that is used by ME to create files to associate
** with shared memory segemnts.  These files are used by MEshow_pages to
** track existing shared memory segments.
*/
# define	ME_SHMEM_DIR	"memory"

# ifdef	USE_MEMSYSCALL
# define	SHMAT	0
# define	SHMCTL	1
# define	SHMDT	2
# define	SHMGET	3
# define	MEMCALL(op, a, b, c)	syscall(SYS_shmsys, op, a, b, c)
# else
# define	SHMAT	shmat
# define	SHMCTL	shmctl
# define	SHMDT	shmdt
# define	SHMGET	shmget
# define	MEMCALL(op, a, b, c)	op(a, b, c)
# endif

# define      ME_SHMSEGS  128
typedef struct   _SHM_IDS
{
    int		id;
    char	key[MAX_LOC];
    char	*addr;
    SIZE_TYPE	len;
} SHM_IDS;

static SHM_IDS  shm_ids[ME_SHMSEGS] ZERO_FILL;
# ifdef OS_THREADS_USED
static CS_SYNCH shm_ids_mutex ZERO_FILL;
static bool     shm_ids_init = FALSE;
# endif /* OS_THREADS_USED */
static STATUS build_memory_loc( LOCATION *location, char loc_buf[]);

/*
** Forward declarations.
*/
static STATUS build_memory_loc( LOCATION *location, char loc_buf[]);
static STATUS ME_get_shared(i4              create_flag,
		            char            *key,
         		    SIZE_TYPE       pcount,
         		    ME_SEGID        *segid,
         		    SIZE_TYPE       *allocated_pages,
         		    CL_ERR_DESC     *err_code);


/*{
** Name: ME_alloc_shared	- allocate shared memory
**
** Description:
**	Maps a shared memory segment into the process, keyed by LOCATION.
**
** Inputs:
**	flag			ME_CREATE_MASK to create the segment.
**				ME_ADDR_SPEC if to a fixed location.
**	memory			place we want it to go, if ME_ADDR_SPEC.
**	id			file key identifying the segment
**	pages			number of pages to create.
**
** Outputs:
**	pages			number of pages attached.
**	memory			place it ended up.
**	err_code		error description
**
**	Returns:
**	    OK or status from called routines.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    File described by the location may be created or opened.
**	    On mmap() systems, the segid is an open file descriptor.
**
** History:
**	??? (anton)
**	    created.
**      20-sep-89 (daveb)
**          Documented.
**	2-aug-1991 (bryanp)
**	    Pass "key" argument through to ME_attach()
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	16-sep-2002 (devjo01)
**	    Add support for ME_LOCAL_RAD.
*/

STATUS
ME_alloc_shared(
	i4		flag,
	SIZE_TYPE	pages,
	char		*key,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    STATUS		status;
    ME_SEGID		segid;

    status = ME_get_shared(flag, key, pages, &segid, allocated_pages, err_code);
#ifdef xDEBUG
    TRdisplay("ME_get_shared %d %d/0x%x\n", segid, status, status);
    if (status)
	TRdisplay("   ME_alloc_shared(%d, %d, %s, %p, %p, %x)\n",
			flag, pages, key, memory, allocated_pages, err_code);
#endif
#ifdef ME_SEG_IN_BREAK
    if (status == OK)
    {
	status = ME_alloc_brk(ME_ALIGN|flag, *allocated_pages, memory,
			      allocated_pages, err_code);
#ifdef xDEBUG
	TRdisplay("ME_alloc_brk %d/0x%x\n", status, status);
#endif
	flag |= ME_ADDR_SPEC;
    }
#endif
    if (status == OK)
    {
	status = ME_attach(flag, *allocated_pages, segid, memory, key,
				err_code);
#ifdef xDEBUG
	TRdisplay("ME_attach %d/0x%x\n", status, status);
#endif
#ifdef ME_SEG_IN_BREAK
	if (status)
	{
	    CL_ERR_DESC junk;
	    _VOID_ MEfree_pages(*memory, *allocated_pages, &junk);
	}
#endif
    }
    return(status);
}

/*{
** Name: ME_get_shared	- locate a shared memory segment by id.
**
** Description:
**	Locates or creates a shared memory segment.
**
** Inputs:
**	create_flags		TRUE if segment should be created.
**	id			LOCATION defining the segment key.
**	pcount			pages to create in a new segment.
**
** Outputs:
**	allocated_pages		size of the segment.
**	segid			id used by other calls
**	err_code		output error description
**
**	Returns:
**	    OK,
**	    ME_ALREADY_EXISTS	if create specified and seg exists.
**	    ME_NO_SUCH_SEGMENT	if non-create and seg doesn't exist.
**	    ME_OUT_OF_MEM       if create specified and segment can't be
**                              allocated do to EINVAL indicating that the
**                              size of the segment exceeds system imposed
**                              limits.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    File described by the location may be created or opened.
**	    On mmap() systems, the segid is an open file descriptor
**	    and the file is grown to the length of the segment.
**
** History:
**	??? (anton)
**	    created.
**      20-sep-89 (daveb)
**          Documented, modified for mmap.
**	3-Oct-89 (anton)
**	    Fixed handleing of allocated_pages
**	28-Feb-90 (jkb)
**	    For MMAP ifdef make sure the file descriptors for shared-
**	    memory segments are greater than 2 so shared memory isn't
**	    corrupted by csreport, cscleanup, etc.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		On EINVAL from shmget to create a shared memory segment in
**		ME_get_shared() return ME_OUT_OF_MEM; also return internal to
**		the CL_ERR_DESC the newly defined error
**		E_CL1226_ME_SHARED_ALLOC_FAILURE;
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**	22-may-1992 (jnash)
**	    xDEBUG'd TRdisplays after expected function call errors.
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
**	16-sep-2002 (devjo01)
**	    Change "bool create_flag" parameter to "i4 flag" as part of adding 
**	    support for ME_LOCAL_RAD.  If ME_LOCAL_RAD set use CXnuma_shmget
**	    to allocate storage.
**	21-Feb-2005 (thaju02)
**	     Modified err_code moreinfo for E_CL1226 to display unsigned size.
**	08-May-2007 (wanfr01)
**	     SIR 118278 - Add details when failing to create shared memory
**	07-Jun-2007 (hanje04)
**	    Bug 118466
**	    Don't cast "segment size" to int when requesting memory in
**	    ME_get_shared() otherwise it's limited to 2Gb. Use SIZE_TYPE.
**	     
*/
static STATUS
ME_get_shared(
	i4		 flag,
	char		*key,
	SIZE_TYPE	pcount,
	ME_SEGID	*segid,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    i4          perms;
    i4		sverrno;
    SIZE_TYPE   *shm_segsz_ptr;

#ifdef SYS_V_SHMEM
    key_t	shmem_key;
#endif

#if defined(xCL_077_BSD_MMAP) && !defined(SYS_V_SHMEM)
    auto char *str;
    auto struct stat sb;
    register STATUS ret_val = OK;
    LOCATION	location;
    LOCATION	temp_loc;
    char	loc_buf[MAX_LOC + 1];
#endif


    CL_CLEAR_ERR( err_code );

    if (key == NULL || *key == '\0')
    {
	return(ME_BAD_PARAM);
    }
#ifdef	SYS_V_SHMEM
# define got1
    if (ME_CREATE_MASK & flag)
    {
	if ( STncmp( key, "IIB.", 4 ) == 0 )
	     perms = 0766;
        else
	     perms = 0700;
	/*
	** Check for already existing filename.  It is OK for one to exist,
	** just reuse it.  It is not OK if a real shared segment already
	** exists.
	*/
	shmem_key = ME_getkey(key);
	if (shmem_key == (key_t)-1)
	{
	    if (ME_makekey(key) != OK)
	    {
    		if (ME_VERBOSE_MASK & flag)
		    SIprintf ("Failure making shared memory file\n");
		return (FAIL);
	    }
	    shmem_key = ME_getkey(key);
	}
	if (shmem_key == (key_t)-1)
	{
    	    if (ME_VERBOSE_MASK & flag)
	    	SIprintf ("Failed to create a key for shared memory\n");
	    return(FAIL);
	}
	if (ME_LOCAL_RAD & flag)
	{
	    *segid = (ME_SEGID)CXnuma_shmget( (i4)shmem_key,
	       (i4)pcount * ME_MPAGESIZE, (i4)(perms|IPC_CREAT|IPC_EXCL),
	       &sverrno );
	    if ( (*segid < 0) && (ENOSYS == sverrno) )
	    {
		*segid = MEMCALL(SHMGET, shmem_key,
				  (SIZE_TYPE)pcount * ME_MPAGESIZE,
				  perms|IPC_CREAT|IPC_EXCL);
		sverrno = errno;
	    }
	}
	else
	{
	    *segid = MEMCALL(SHMGET, shmem_key,
			      (SIZE_TYPE)pcount * ME_MPAGESIZE,
			      perms|IPC_CREAT|IPC_EXCL);
	    sverrno = errno;
	}
	if (*segid < 0)
	{
	    SETCLERR(err_code, 0, ER_shmget);
	    if (sverrno == EEXIST)
	    {
    	    	if (ME_VERBOSE_MASK & flag)
	            SIprintf ("Shared memory already exists for key %x\n",shmem_key);
		return(ME_ALREADY_EXISTS);
	    }
            else if (sverrno == EINVAL)
            {
                /* according to the man page EINVAL can mean either of the
                ** following:
                **
                **     size is less than the system-imposed minimum or greater
                **     than the system imposed maximum.
                **
                **     A shared memory identifier exists for key but
                **     the size of the segment associ- ated with it is less
                **     than size and size is not equal to zero.
                */
                err_code->intern = E_CL1226_ME_SHARED_ALLOC_FAILURE;
                err_code->moreinfo[0].size = sizeof(SIZE_TYPE);
                err_code->moreinfo[0].data._size_type = pcount * ME_MPAGESIZE;

    	    	if (ME_VERBOSE_MASK & flag)
	            SIprintf ("Error EINVAL returned when creating shared memory for key %x\n",shmem_key);
                return(ME_OUT_OF_MEM);
            }
	    else
	    {
    	    	if (ME_VERBOSE_MASK & flag)
	            SIprintf ("Error %d creating shared memory for key %x\n",sverrno,shmem_key);
		return(FAIL);
	    }
	}
	*allocated_pages = pcount;
	return(OK);
    }
    shmem_key = ME_getkey(key);
    if (shmem_key == -1)
    {
	return(ME_NO_SUCH_SEGMENT);
    }
    if ((*segid = MEMCALL(SHMGET, shmem_key, 0, 0)) < 0)
    {
	sverrno = errno;
	SETCLERR(err_code, 0, ER_shmget);

	if ((sverrno == EINVAL) || (sverrno == ENOENT))
	{
	    return(ME_NO_SUCH_SEGMENT);
	}
	else
	{
	    return(FAIL);
	}
    }
    if (allocated_pages)
    {
	struct shmid_ds shmds;
	
	/* get the size of the segment */
	if (MEMCALL(SHMCTL, *segid, IPC_STAT, &shmds) < 0)
	{
	    SETCLERR(err_code, 0, ER_shmctl);
	    return(FAIL);
	}
#if defined(axp_osf)
        shm_segsz_ptr = (SIZE_TYPE *) &(shmds.shm_segsz); 
	*allocated_pages = (*shm_segsz_ptr) / ME_MPAGESIZE;
#else
	*allocated_pages = shmds.shm_segsz / ME_MPAGESIZE;
#endif
    }
    return(OK);
# endif				/* SYS_V_SHMEM */

# if defined(xCL_077_BSD_MMAP) && !defined(got1)
# define got1

    ret_val = build_memory_loc(&temp_loc, loc_buf);
    if (ret_val != OK)
    {
	return (ret_val);
    }

    LOcopy(&temp_loc, loc_buf, &location);
    LOfstfile(key, &location);

    if (ME_CREATE_MASK & flag)
    {
	char buf[ ME_MPAGESIZE ];
	int i;

	if((*segid = (ME_SEGID) open(location.string,
				       O_RDWR|O_CREAT|O_EXCL, 0600 )) < 0 )
	{
	    SETCLERR(err_code, 0, ER_open);
#ifdef xDEBUG
	    TRdisplay("Can't create shared memory file %s.\n",location.string);
	    TRdisplay("%s already exists.\n",location.string);
#endif
	    ret_val = ME_ALREADY_EXISTS;
	}
	else
	{
	    /* make sure shared memory segment and stdio are different */
	    if ((i4) *segid < 3 && (i4) *segid >= 0)
	    {
	        i4		fd_tmp;
    
    	        fd_tmp = fcntl ((i4)*segid,F_DUPFD,3);
	        if(fd_tmp > 0)
		{
	            (void)close(*segid);
	            *segid = (ME_SEGID) fd_tmp;
	        }
	    }

	    /* Make the file that big */
	    MEfill( sizeof buf, 0, buf );
	    for( i = 0; i < pcount; i++ )
	    {
	        if( write( *segid, buf, sizeof buf ) != sizeof buf )
	        {
		    SETCLERR( err_code, 0, ER_write );
		    TRdisplay("Can't init %d pages of shared memory file %s.\n",
			      pcount, location.string);
		    ret_val = FAIL;
		    break;
	        }
	    }
	    *allocated_pages = pcount;
	}
    }
    else
    {
	/* open presumably existing file */
	
	if( (*segid = (ME_SEGID) open( location.string, O_RDWR, 0600 )) < 0 )
	{
#ifdef xDEBUG
	    TRdisplay("Can't open shared memory file %s.\n",location.string);
#endif 
	    SETCLERR(err_code, 0, ER_open);
	    ret_val = ME_NO_SUCH_SEGMENT;
	}
	else if (allocated_pages)
	{
	    /* This can't fail, since we do have an open fd, but... */
	    if( fstat( *segid, &sb ) )
	    {
		SETCLERR(err_code, 0, ER_fstat);
		*allocated_pages = 0;
		ret_val = FAIL;
	    }
	    else
		*allocated_pages = sb.st_size / ME_MPAGESIZE;
	}
	/* make sure shared memory segment and stdio are different */
	if ((i4) *segid < 3 && (i4) *segid >= 0)
	{
	    i4		fd_tmp;

    	    fd_tmp = fcntl ((i4)*segid,F_DUPFD,3);
	    if(fd_tmp > 0)
	    {
	        (void)close(*segid);
	        *segid = (ME_SEGID) fd_tmp;
	    }
	}
    }
    return( ret_val );
# endif				/* xCL_077_BSD_MMAP !SYS_V_SHMEM */

#ifndef got1
    return(FAIL);
#endif
}

/*{
** Name: ME_align	- report segment size and alignment restrictions
**
** Description:
**	Report alignment restrictions.  This is a procedure because some
**	machines may require a system call to determine these restrictions.
**
** Inputs:
**	none
**
** Outputs:
**	size				size multiple
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	 2-dec-88 (anton)
**	    portability changes
**      25-feb-88 (anton)
**          Created.
**	05-nov-2002 (devjo01)
**	    Change type of 'pgsize' from 'i4' to 'SIZE_TYPE'.
*/
STATUS
ME_align(
	SIZE_TYPE	*pgsize)
{
    *pgsize = ME_MPAGESIZE;
    return(OK);
}

/*{
** Name: MEattach	- Attach a segment
**
** Description:
**	Attach a shared segment via whatever facilities this UNIX
**	provides.
**
** Inputs:
**	flags				flags
**	pages				number of pages in segment
**	segid				segment id to attach
**	addr				address to attach to
**	key				shared memory segment key (just passed
**					through to ME_reg_seg())
**	err_code			system specific error code
**
** Outputs:
**	addr				address where segment was attached
**
**	Returns:
**	    OK
**	    !OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	 2-dec-88 (anton)
**	    portability changes
**      25-feb-88 (anton)
**          Created.
**	20-sep-89 (daveb)
**	    added mmap support
**	3-Oct-89 (anton)
**	    Only specify MAP_FIXED on ME_SEG_IN_BREAK systems.
**	22-Oct-89 (fls-sequent)
**	    Don't specify MAP_FIXED for Sequent.
**	2-aug-1991 (bryanp)
**	    Update ME_attach and ME_alloc_shared to add new "key" argument,
**	    and pass the "key" and "flags" arguments to ME_reg_seg. This is
**	    being done in order to add support for DI slaves performing I/O
**	    directly to/from shared memory, without needing to copy buffers.
**	2-mar-92 (jnash)
**	    Propagate ME_ADDR_SPEC to mmap call flags parameter.
**	20-sep-93 (kwatts)
**	    GordonW fix from 6.4. Copy addr to a local so that the
**	    TRdisplays later on use the real value, not -1.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	10-jan-94 (swm)
**	    Bug #58646
**	    Can attach to address 0 on axp_osf.
**	20-mar-95 (smiba01)
**	    Added support for dr6_ues (ICL secure OS).
**	    Removed confusing entry for dr6_uv1.
*/
STATUS
ME_attach(
	i4		flags,
	SIZE_TYPE	pages,
	ME_SEGID	segid,
	PTR		*addr,
	char		*key,
	CL_ERR_DESC	*err_code)
{
    int		tmp_flag;
    PTR         laddr =  *addr;
    int         i;

# if defined(sqs_ptx)
  /* do nothing */
# else
    caddr_t mmap();
# endif /* sqs */

    CL_CLEAR_ERR( err_code );

# ifdef OS_THREADS_USED
    if ( shm_ids_init == FALSE )
    {
	shm_ids_init = TRUE;
	CS_synch_init( &shm_ids_mutex );
    }
# endif /* OS_THREADS_USED */

# if !(defined(any_hpux) || defined(axp_osf) || defined(hp8_bls))
    if ((flags & ME_ADDR_SPEC) && *addr == 0)
    {
	return(ME_BAD_PARAM);
    }
# endif /* hpux axp_osf */

# ifndef ME_SEG_IN_BREAK
    if (!(flags & ME_ADDR_SPEC))
    {
	*addr = 0;
    }
# endif

        /*
        ** Look for this segment in the already mapped list. If it is here then
        ** return its address. This is required bacause the DRS 500 does not
        ** support the multiple mapping of the same segment within a process.
        ** Ingres seems to want to do this because CS_create_sys_segment creates
        ** and then maps the shared memory segment. CS_map_sys_segment then trys
        ** to remap the same segment !
        */
# ifdef OS_THREADS_USED
    CS_synch_lock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
        for (i = 0; i < ME_SHMSEGS; ++i)
        {
                if (shm_ids[i].id == segid && shm_ids[i].addr > (char *)0)
                {
                        *addr = shm_ids[i].addr;   /* Return previous address */
# ifdef OS_THREADS_USED
    			CS_synch_unlock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
                        return (OK);
                }
        }
# ifdef OS_THREADS_USED
    CS_synch_unlock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */

# ifdef	SYS_V_SHMEM
# define got2
    if ((long)(*addr = (PTR)MEMCALL(SHMAT, segid, *addr, 0)) == -1l)
    {
	SETCLERR(err_code, 0, ER_shmat);
	TRdisplay("shmat for id %d at address %p fails\nerrno = %d\n",
		segid, laddr, err_code->errnum);

	return(FAIL);
    }
    else
    {
        /*
        ** Add the mapped segment into the list ready to be unmapped in the
        ** future. See comment above for further details.
        */
# ifdef OS_THREADS_USED
    CS_synch_lock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
        for (i = 0; i < ME_SHMSEGS; ++i)
        {
                if (shm_ids[i].id == 0)
                {
                        shm_ids[i].id = segid;
			STcopy(key, shm_ids[i].key);
                        shm_ids[i].addr = *addr;
			shm_ids[i].len = pages;
                        break;
                }
        }
# ifdef OS_THREADS_USED
    CS_synch_unlock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
    }

    if (ME_reg_seg(*addr, segid, pages, key, flags))
    {
	/* Ugh.  After all that work a simple reqmem fails.  Recover and
	   undo the attach - if this detach fail we are in bad trouble */
# if defined(xCL_SHMDT_ONE_ARG) || defined(any_hpux)
	SHMDT(*addr);
# else
	MEMCALL(SHMDT, *addr, 0, 0);
# endif	/* dr6_us5 */
	return(FAIL);
    }

# endif	/* SYS_V_SHMEM */

# if defined(xCL_077_BSD_MMAP) && !defined(got2)
# define got2
    /*
    ** Figure any system dependant "flags" for mmap in SHARE_FLAGS
    */

# ifndef SHARE_FLAGS
# ifdef ME_SEG_IN_BREAK
# if defined(sqs_ptx)
# define SHARE_FLAGS 0
# else	/* sqs */
# define SHARE_FLAGS MAP_FIXED
# endif /* sqs */
# else
# define SHARE_FLAGS 0
# endif
# endif

    /* If your mmap isn't
    **
    **   caddr_t mmap(addr, len, prot, flags, fd, off)
    **   caddr_t addr;
    **   int len, prot, flags, fd;
    **   off_t off;
    ** 
    ** then you'll need to ifdef this too.
    */
# if defined(sqs_ptx)
    if( mmap( *addr, pages * ME_MPAGESIZE, PROT_READ|PROT_WRITE,
	       SHARE_FLAGS|MAP_SHARED, segid, (off_t)0) == -1 )
# else

    /* Propagate ME_ADDR_SPEC to mmap, may already
    ** be done above, but not always
    */
    tmp_flag = (flags & ME_ADDR_SPEC) ? MAP_FIXED : 0;
    if( (long)(*addr = mmap( *addr, pages * ME_MPAGESIZE,
			    PROT_READ|PROT_WRITE,
			    SHARE_FLAGS|MAP_SHARED|tmp_flag, 
			    segid, (off_t)0)) == -1L )
# endif
    {
	SETCLERR(err_code, 0, ER_mmap)
	TRdisplay("mmap for fd %d at 0x%p fails\nerrno = %d\n",
		segid, addr, errno);
	return(FAIL);
    }

    if (ME_reg_seg(*addr, segid, pages, key, flags))
    {
	/* Ugh.  After all that work a simple reqmem fails.  Recover and
	   undo the attach - if this fails we are in trouble */
	_VOID_ munmap( *addr, pages );
	_VOID_ close( segid );
	return(FAIL);
    }

# endif	/* xCL_077_BSD_MMAP !SYS_V_SHMEM */

# ifdef got2
    return(OK);
# else
    return( FAIL );
# endif
}

/*{
** Name: MEdetach()	- Disconnect shared memory segment from this process
**
** Description:
**	Detaches the shared memory segment associated with the shared memory 
**	identifier specified by shmid from the data segment of the calling 
**	process.  
**
**	Shmid must be a valid shared memory identifier used by a previous
**	MEcreate() call to create the shared memory segment, and a MEattach() 
**	call to attach the shared mem segment to the process.
**
**	On success beginning address of the shared memory segment
**	will be returned.  
**
** Theory:
**	Look up the segment in the segment pool and translate into an
**	address range for MEfree_pages.
**
** Inputs:
**	key				A character string identifier for the
**					a shared memory segment.  The key
**					identifier should be a character string
**					that follows the Ingres file naming
**					conventions: 8 characters max prefix
**					name and 3 characters max suffix.
**					This key should have been used earlier
**					in a successful MEget_pages call.
**	address				address to which the shared memory
**					segment is currently mapped.
** Outputs:
**	err_code 			System specific error information.
**
**	Returns:
**	    OK
**	    ME_NO_SUCH_SEGMENT		shared memory id does not exist
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-88 (mmm)
**         Proposed. 
**	12-jun-89 (rogerk)
**	    Changed shared memory key to be char string, not LOCATION ptr.
**      07-aug-98 (hweho01)
**          Fixed the initial setting error in MEdetach(),  so the
**          the search operation can run through the queue.
*/
STATUS
MEdetach(
	char		*key,
	PTR		address,
	CL_ERR_DESC	*err_code)
{
    register ME_SEG_INFO	*seginf;

    for (seginf = (ME_SEG_INFO *) ME_segpool.q_next;
	 (QUEUE *)seginf != &ME_segpool;
	 seginf = (ME_SEG_INFO *)seginf->q.q_next)
    {
	if (seginf->addr == address)
	{
	    return(MEfree_pages(address, seginf->npages, err_code));
	}
    }

    CL_CLEAR_ERR( err_code );
    return(ME_NO_SUCH_SEGMENT);
}

/*{
** Name: ME_getkey()	- convert a character string key to an ipc key
**
** Description:
**	Convert a user supplied shared memory key to a UNIX key.  This
**	is done by looking up a file corresponding to this user key in
**	the installation's memory directory (II_CONFIG/memory).  This
**	file should have been created by ME_makekey() through MEget_pages().
**
**	The ipc key is created using ftok(3) with the specified file.
**
** Inputs:
**	key			character key identifying shared segment.
**
** Outputs:
**	return value
**
**	Returns:
**	    sys V ipc key
**		-1 if there is no file 
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-jul-88 (jaa)
**          created.
**	12-jun-89 (rogerk)
**	    Changed user key to be a character string instead of a LOCATION.
**	    Changed to look up file in MEMORY directory.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**	13-sep-1995 (canor01)
**	    When creating the shared memory key, add the installation id
**	    to try for a more unique key.
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-feb-1997 (canor01/funka01)
**	    Key creation was only adding in the first character of the
**	    installation id.  Modified to use all characters.
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
**	5-Apr-2006 (kschendel)
**	    Force ingres symbol table for II_INSTALLATION, don't allow local
**	    shell env override lest we end up with the wrong ID.
*/
key_t
ME_getkey(
	char		*user_key)
{
    LOCATION	location;
    LOCATION	temp_loc;
    char	loc_buf[MAX_LOC + 1];
    auto char	*file_str;
    key_t	ret_val = 0;
    char	*installation;
    i4		inst_id;
    int		i;

    /*
    ** Build UNIX key name from user key.
    ** Use user key as a filename identifying a file in the installation's
    ** MEMORY directory.  Check that the file exists, and get its inode
    ** to use as a shared memory key.
    */

    /*
    ** Get location of ME files for shared memory segments.
    */
    if (build_memory_loc(&temp_loc, loc_buf))
    {
	ret_val = -1;
    }
    else
    {
	LOcopy(&temp_loc, loc_buf, &location);
        LOfstfile(user_key, &location);

	/*
	** Check existence of file indicated by user key.
	*/
	if (LOexist(&location) != OK)
	{
            ret_val = -1;
        }
        else
        {
	    /*
	    ** The ftok() function creates a unique key based on
	    ** the device plus the last 3 hex digits of the inode number
	    ** Since large file systems can have duplicates within this
	    ** range, salt in the installation id. (canor01)
	    */
            LOtos(&location, &file_str);
	    NMgtIngAt( "II_INSTALLATION", &installation );
	    if (installation && *installation)
		for ( inst_id = 0, i = 0; i < STlength(installation); i++)
		    inst_id = (inst_id << (i*4)) + *(installation+i);
	    else
		inst_id = 'I';
            ret_val = ftok(file_str, inst_id);
        }
    }

    return(ret_val);
}
#ifdef SYS_V_SHMEM
/*{
** Name: ME_makekey()	- Make UNIX shared memory key from user key.
**
** Description:
**	Create a file to identify a shared memory segment.  The file's inode
**	will be used as a shared memory key.  The file's existence will be
**	used to track shared segments on the system.
**
** Inputs:
**	key			character key identifying shared segment.
**
** Outputs:
**	return value
**
**	Returns:
**	    OK			key created.
**	    FAIL		key could not be created.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    File created in installation's memory directory.
**
** History:
**      12-jun-89 (rogerk)
**          created.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
*/
STATUS
ME_makekey(
	char		*user_key)
{
    LOCATION	location;
    LOCATION	temp_loc;
    char	loc_buf[MAX_LOC + 1];
    FILE	*dummy;
    STATUS	ret_val = OK;

    /*
    ** Build location to file in MEMORY directory.
    ** Get location of ME files for shared memory segments.
    */

    if (!(ret_val = build_memory_loc(&temp_loc, loc_buf)))
    {
	LOcopy(&temp_loc, loc_buf, &location);
        LOfstfile(user_key, &location);

	/*
	** Create file to identify the shared segment.  The file's inode
	** will be used as a shared segment key.  This will also allow
	** MEshow_pages to tell what shared segments have been created.
	*/
	if (SIopen(&location, "w", &dummy) != OK)
	    ret_val = FAIL;
	else
	    SIclose(dummy);
    }
    return (ret_val);
}
# endif	/* SYS_V_SHMEM */

/*{
** Name: MEsmdestroy()	- Destroy a shared memory segment
**
** Description:
**	Remove the shared memory segment specified by shared memory identifier
**	"key" from the system and destroy any system data structure associated 
**	with it.
**
**	Note: The shared memory pages are not necessarily removed from
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
**	    ME_BAD_ADVICE	call was made during ME_USER_ALLOC
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
**	12-jun-89 (rogerk)
**	    Changed key to character string from LOCATION pointer.
**	15-aug-89 (fls-sequent)
**	    Treat MEadvice as a flag word instead of an integer.
**	20-sep-89 (daveb)
**		Added mmap.
*/
STATUS
MEsmdestroy(
	char		   *key,
	CL_ERR_DESC	   *err_code)
{
    STATUS      ret_val = OK;

#ifdef	SYS_V_SHMEM
    /* system dependent variables */
    key_t       shmem_key;
    int         shmid;
#endif

#if defined(xCL_077_BSD_MMAP) && !defined(SYS_V_SHMEM)
    LOCATION	location;
    char	loc_buf[MAX_LOC + 1];
#endif

    CL_CLEAR_ERR( err_code );

    if ( MEadvice == ME_USER_ALLOC)
        return(ME_BAD_ADVICE);

    /* map inputs to system dependent variables */

#ifdef	SYS_V_SHMEM
    shmem_key = ME_getkey(key);
    if (shmem_key == -1)
        return(ME_NO_SUCH_SEGMENT);

    if ((shmid = MEMCALL(SHMGET, shmem_key, 0, 0)) == -1)
    {
	SETCLERR(err_code, 0, ER_shmget);

        switch (errno)
        {
        case EINVAL:
            ret_val = ME_NO_SUCH_SEGMENT;
            break;
        case EACCES:
            ret_val = ME_NO_PERM;
            break;
        case ENOENT:
            break;
        default:
            ret_val = FAIL;
        }
    }
    else if (MEMCALL(SHMCTL, shmid, IPC_RMID, 0) < 0)
    {
	SETCLERR(err_code, 0, ER_shmctl);
        ret_val = FAIL;
        if (errno == EPERM)
            ret_val = ME_NO_PERM;
    }
#endif				/* SYS_V_SHMEM */

    if (ret_val == OK)
    {
        ret_val = ME_destroykey(key);
    }

    return(ret_val);
}
/*{
** Name: MEseg_free()	- Free shared memory
**
** Description:
**	Free a region of shared memory and return new region for potential
**	futher freeing of the break region.
**
**	If there is attached shared memory in the region being
**	freed then those pages should only be marked free if the whole
**	segment can be detached.
**
**	There is a small table of attached segments which is scanned.
**
** Inputs:
**      addr				address of region
**	pages				number of pages to check
**
** Outputs:
**	addr				new address of region
**	pages				new number of pages
**	err_code			CL_ERR_DESC
**
**	Returns:
**	    OK
**	    ME_NOT_ALLOCATED
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-jul-88 (anton)
**          created
**	 7-jul-1989 (rogerk)
**	    Call ME_rem_seg to remove segment entries from the segment pool.
**	    Set pages argument to zero if shared segment is outside of local
**	    memory address space.
**	20-sep-1993 (kwatts)
**	    Bring forward 6.4 change from GordonW. Added a TRdisplay for
**	    an error path and also improved shared memory checks.
*/
STATUS
MEseg_free(
	PTR		*addr,
	SIZE_TYPE	*pages,
	CL_ERR_DESC	*err_code)
{
    STATUS			status = OK;
    register ME_SEG_INFO	*seginf;
    char			*lower, *upper, *last;
    SIZE_TYPE			off, len;
    QUEUE			*next_queue;
    i4                          i;

    CL_CLEAR_ERR( err_code );

    lower = (char *)*addr;
    upper = lower + ME_MPAGESIZE * *pages;
    last = NULL;
    for (seginf = ME_find_seg(lower, upper, &ME_segpool);
	 seginf && status != FAIL;
	 seginf = ME_find_seg(lower, upper, next_queue))
    {
	next_queue = &seginf->q;
	if (last && last != seginf->eaddr)
	{
	    status = ME_NOT_ALLOCATED;
	}
	last = seginf->addr;
	off = 0;
	len = seginf->npages;
	if (lower > seginf->addr)
	{
	    off = (lower - seginf->addr) / ME_MPAGESIZE;
	    len -= off;
	}
	if (upper < seginf->eaddr)
	{
	    len -= (seginf->eaddr - upper) / ME_MPAGESIZE;
	}
	if (MEalloctst(seginf->allocvec, off, len, TRUE))
	{
	    status = ME_NOT_ALLOCATED;
	}
	MEclearpg(seginf->allocvec, off, len);
	if (!MEalloctst(seginf->allocvec, 0, seginf->npages, FALSE))
	{
	    /* detach segment */
#if defined(xCL_077_BSD_MMAP) && !defined(SYS_V_SHMEM)
	    if( munmap( seginf->addr, seginf->npages * ME_MPAGESIZE ) )
	    {
		SETCLERR( err_code, 0, ER_munmap );
		status = FAIL;
	    }
	    (void)close( seginf->segid );
#endif
#ifdef	SYS_V_SHMEM

# if defined(xCL_SHMDT_ONE_ARG) || defined(any_hpux)
	    if (SHMDT(seginf->addr))
# else
	    if (MEMCALL(SHMDT, seginf->addr, 0, 0))
# endif	/* dr6_us5 */
	    {
		SETCLERR(err_code, 0, ER_shmdt);
		status = FAIL;
		TRdisplay("MEseg_free: Can't shmdt %p, errno %d\n",
				*addr, err_code->errnum);
	    }
# endif
# ifdef OS_THREADS_USED
            CS_synch_lock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
	    for ( i = 0; i < ME_SHMSEGS; i++ )
	    {
		if ( shm_ids[i].addr == seginf->addr )
		{
		    shm_ids[i].id = 0;
		    shm_ids[i].addr = (PTR)0;
		    break;
	        }
	    }
# ifdef OS_THREADS_USED
            CS_synch_unlock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */

	    if( status == OK )
	    {
	        /*
                ** first check for an exact match,
                ** we do this to help on systems that
                ** have shared memory attached anywhere
                ** within the process's address space,
                ** (not just a high space)
                */
                if(seginf->addr == (char *)*addr &&
                   seginf->npages >= *pages)
                {
                    *pages -= seginf->npages;
                }

		/* add to return region */
		if (seginf->addr < (char *)*addr)
		{
		    *pages += ((char *)*addr - seginf->addr) / ME_MPAGESIZE;
		    *addr = (PTR)seginf->addr;
		}
		if (seginf->eaddr > upper)
		{
		    *pages += (seginf->eaddr - upper) / ME_MPAGESIZE;
		}
		next_queue = ME_rem_seg(seginf);
	    }
	}
    }

    /*
    ** If the shared segment address is out of the range of the local memory
    ** address space, then return that there are zero pages for the local
    ** memory manager to unmap.  Otherwise, return the number of pages of
    ** this segment that do reside in the local memory address space.
    */
    if ((char *)*addr > MElimit)
    {
	*pages = 0;
    }
    else if ((char *)*addr + ME_MPAGESIZE * *pages > MElimit)
    {
	*pages = (MElimit - (char *)*addr) / ME_MPAGESIZE;
    }

    return(status);
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
**	none
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      30-jun-89 (rogerk)
**          Created for terminator project.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
*/
STATUS
ME_destroykey(
	char		*user_key)
{
    LOCATION	    location;
    LOCATION	    temp_loc;
    char	    loc_buf[MAX_LOC + 1];
    STATUS          ret_val = OK;

    /*
    ** Get location of ME files for shared memory segments.
    */
    if (!(ret_val = build_memory_loc(&temp_loc, loc_buf)))
    {
	LOcopy(&temp_loc, loc_buf, &location);
        LOfstfile(user_key, &location);

	if (LOexist(&location) != OK)
	    ret_val = FAIL;
	else
	    LOdelete(&location);
    }
    return (ret_val);
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
**          i4         *arg_list;       Pointer to argument list. 
**          char       *key;		Shared segment key.
**          CL_SYS_ERR *err_code;       Pointer to operating system
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
**      12-jun-89 (rogerk)
**          Created for Terminator.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Check error return from NMloc - some AV's in ME have been
**		caused by not checking these error returns, especially in cases
**		where II_SYSTEM is not defined.
**      30-may-1996 (stial01)
**          Added ME_TUXEDO_ALLOC, build_memory_loc() (B75073)
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
*/
STATUS
MEshow_pages(
	STATUS		(*func)(),
	PTR		*arg_list,
	CL_SYS_ERR	*err_code)
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

    /*
    ** Get location ptr to directory to search for memory segment names.
    */
    status = build_memory_loc(&locoutptr, loc_buf1);
    if (status != OK)
    {
	return (status);
    }

    LOcopy(&locoutptr, loc_buf1, &locinptr);

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

/*{
** Name: ME_lock_sysv_mem()	- Lock shared memory on SYSV shared mem machine.
**
** Description:
**	Cause memory associated with a system V shared memory segment to be
**	"locked", ie. always physically resident in real memory.  Only machines
**	with SHM_LOCK option to shmctl() can support this feature.
**
** Inputs:
**	key				character key to identify shared mem.
**
** Outputs:
**	err_code			errors.
**
**	Returns:
**	    ME_LOCK_FAIL
**	    ME_NO_SUCH_SEGMENT
**	    ME_NO_SHMEM_LOCK
**	    OK
**	    FAIL
**
** History:
**      23-may-94 (mikem)
**          Created.
**	24-may-1994 (mikem)
**	    Added ifdef's to make routine build/runtime-error on machines with
**	    no seteuid() support.  
*/
STATUS
ME_lock_sysv_mem(
char		*key,
CL_ERR_DESC	*err_code)
{
#if defined(SYS_V_SHMEM) && defined(SHM_LOCK)

    struct shmid_ds	dummy_buf;
    int			segid;
    key_t		shmem_key;
    int			saveuid;
    STATUS		status = OK;

# ifndef xCL_SETEUID_EXISTS
    /* This implementation, using shmctl to lock memory, requires root
    ** access to execute the shmctl() call.  Systems which support 
    ** seteuid() allow processes started as root to which effective id back
    ** and forth from root.
    */
    {
        TRdisplay("MEget_pages: Memory locking requires seteuid() fn call\n");
	CL_CLEAR_ERR( err_code );
        return(ME_NO_MLOCK);
    }
# else

    saveuid = geteuid();
    if ( seteuid((uid_t)0) == -1 )
    {
	SETCLERR(err_code, 0, ER_setuid);
	return(ME_LOCK_FAIL);
    }

    shmem_key = ME_getkey(key);
    if (shmem_key == -1)
    {
	status = ME_NO_SUCH_SEGMENT;
    }
    else if ((segid = MEMCALL(SHMGET, shmem_key, 0, 0)) < 0)
    {
        SETCLERR(err_code, 0, ER_shmget);

        if ((errno == EINVAL) || (errno == ENOENT))
        {
	    status = ME_NO_SUCH_SEGMENT;
        }
        else
        {
	    status = FAIL;
        }
    }
    else if (MEMCALL(SHMCTL, segid, SHM_LOCK, &dummy_buf) < 0)
    {
	SETCLERR(err_code, 0, ER_shmctl);
	status = FAIL;
    }

    if ( seteuid((uid_t)saveuid) == -1 )
    {
	SETCLERR(err_code, 0, ER_setuid);
	status = ME_LOCK_FAIL;
    }

    return(status);    

# endif /* xCL_SETEUID_EXISTS */

#else

    TRdisplay("MEget_pages: Memory locking requires mmap() usage\n");
    CL_CLEAR_ERR( err_code );
    return(ME_NO_SHMEM_LOCK);

#endif /* defined(SYS_V_SHMEM) && defined(SHM_LOCK) */
}

/*{
** Name: build_memory_loc()	- Build location of shared memory file.
**
** Description:
**      Build location where shared memory file will be created
**
** Inputs:
**	key				character key to identify shared mem.
**
** Outputs:
**      None
**
**	Returns:
**	    OK
**          status
**
** History:
**      30-may-96 (stial01)
**          Created. (B75073)
**	03-apr-1997 (reijo01)
**          Bug #81346
**	    Bug fix, build_memory_loc would return a LOCATION which would
**	    have a pointer to an out of scope variable, loc_buf, which had
**	    been defined in build_memory_loc.
**	07-mar-2001 (devjo01)
**	    Sir 103715 - UNIX clusters.  Check if system configured
**	    for clusters, and if so, add node name to path.
*/
static STATUS
build_memory_loc(
	LOCATION *location,
	char	 loc_buf[])
{
    LOCATION	temp_loc;
    char        *tux_file_ptr;            /* points to environment space */
    STATUS      ret_val;

    if (MEadvice & ME_TUXEDO_ALLOC)
    {
	NMgtAt( ERx("II_TUXEDO_LOC"), &tux_file_ptr);
	if (tux_file_ptr == (char *)0 || *tux_file_ptr == '\0')
	    NMgtAt( ERx("II_TEMPORARY"), &tux_file_ptr);
	if (tux_file_ptr == (char *)0 || *tux_file_ptr == '\0')
	    ret_val = FAIL;
	else
	    ret_val = LOfroms(PATH, tux_file_ptr, location);
    }
    else
    {
	if ((ret_val = NMloc(ADMIN, PATH, (char *)NULL, &temp_loc)) == OK)
	{
	    LOcopy(&temp_loc, loc_buf, location);
	    LOfaddpath(location, ME_SHMEM_DIR, location);
	    if ( CXcluster_configured() )
	    {
		LOfaddpath(location, CXnode_name(NULL), location);
	    }
	}
    }
    return (ret_val);
}

/*{
** Name: MEget_seg() - return information about mapped shared memory
**
** Description:
**      This routine is used by clients of the shared memory allocation
**      routines to show what shared memory segments a particular 
**      memory address is in.
**
** Inputs:
**	addr		address that (supposedly) is in shared memory
**
** Outputs:
**	key		pointer to name of shared memory segment
**	base		base address of shared memory segment
**
** Returns:
**	OK		
**	FAIL		address not in a known shared memory segment
**
** History:
**	05-nov-1997 (canor01)
**	    Created for DG/UX mutex reclamation project.
**      11-aug-1998 (hweho01)
**          Included the starting location of a shared
**          segment when search its key and base address.
**
*/
STATUS
MEget_seg( 
	char	*addr,
	char	**key,
	void	**base )
{
    register int	i;
    STATUS		status = FAIL;

# ifdef OS_THREADS_USED
    if ( shm_ids_init == FALSE )
    {
	shm_ids_init = TRUE;
	CS_synch_init( &shm_ids_mutex );
    }

    CS_synch_lock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */
    for (i = 0; i < ME_SHMSEGS; ++i)
    {
        if ( addr >= shm_ids[i].addr && 
             addr < (shm_ids[i].addr + (shm_ids[i].len * ME_MPAGESIZE)) )
        {
	    *key = shm_ids[i].key;
	    *base = shm_ids[i].addr;
            status = OK;
	    break;
        }
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock(&shm_ids_mutex);
# endif /* OS_THREADS_USED */

    return( status );
}
