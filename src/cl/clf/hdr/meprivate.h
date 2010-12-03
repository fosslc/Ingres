/*
**  Copyright (c) 2004 Ingres Corporation
**
*/
# ifndef	ME_PRIVATE_INCLUDED
# define	ME_PRIVATE_INCLUDED 1
# include 	<compat.h>	/* compatability library header */
# include	<cs.h>		/* for threads structures */
# include	<lo.h>		/* for MAX_LOC value */
# include	<me.h>		/* include def of LIFOBUF */
# include	<qu.h>		/* for QUEUE structure */
# ifdef LNX 
# include	<sys/ipc.h>
# endif /* Linux */
# ifdef xCL_069_UNISTD_H_EXISTS
# ifdef axp_osf
# include	<time.h>
# endif /* axp_osf */
# include 	<unistd.h>      /* for sbrk() definition on some machines */
# endif /* xCL_069_UNISTD_H_EXISTS */
# include	<systypes.h>    /* for sbrk() definition on some machines */


/*
**
** meprivate.h
**
** History
**	09-apr-1986 (daveb)
**	    Increase max obj size as in 5.0 to > 512k.
**
**	16-Dec-1986 (daveb)
**	    Rework for layering on malloc.  Get rid of useless
**	    debugging stuff, add some variables.  Note that
**	    the args to many ME routines changed in the CL
**	    spec to take PTR args, as of spec revision 4.
**	    Renamed this from "ME.h" to "meprivate.h" to disambiguate
**	    from clhdr/ME.h.
**
**	19-apr-89 (markd)
**	    Added ME_SEG_IN_BREAK and SYS_V_SHMEM
**	    for sqs_us5.
**
**      15-aug-1989 (fls-sequent)
**	    Added ME_INGRES_SHMEM_ALLOC Meadvise flag for MCT.
**
**	02-feb-1990 (jkb - comment added by mikem)
**	    Added definitions to allow sequent to use MMAP rather the system V
**	    shared memory.
**
**	08-feb-1990 (mikem)
**	    The above change broke the shared memory for all machines other than
**	    sequent.  This change adds defines to support the sun3 and sun4 by
**	    defining SYS_V_SHMEM (in 6.4 we will probably switch over to using
**	    mmap()).  Note that all new machines must add a define for either
**	    MMAP or SYS_V_SHMEM.
**	16-apr-1990 (kimman)
**	    Adding ds3_ulx to SYS_V_SHMEM.
**	14-may-90 (achiu)
**	    Adding pyr_u42  and pyr_us5 to SYS_V_SHMEM.
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Force error if <clconfig.h> has not been not included;
**		Add #ifdef xCL_075_SYS_V_IPC_EXISTS to the list of symbols
**		for which SYS_V_SHMEM gets defined.
**	4-sep-90 (jkb)
**	    define ME_SEG_IN_BREAK and MMAP, and undefine SYS_V_SHMEM for
**	    sqs_ptx
**	4-oct-90 (jkb)
**	    define MECOPY_WRAPPED_MEMCPY, MECMP_WRAPPED_MEMCPY and 
**	    MEFILL_WRAPPED_MEMSET, and undefine SYS_V_SHMEM for sqs_ptx
**	19-nov-90 (jkb) define shbrk for ptx/mct
**      21-Jan-91 (jkb)
**          Change ME_INGRES_SHMEM_ALLOC to ME_INGRES_THREAD_ALLOC
**	    remove the *defines for ME_USER_ALLOC, ME_INGRES_ALLOC and
**	    ME_INGRES_THREAD_ALLOC.  These are defined in me.h which is
** 	    included in this file.  Change MCT specific ifdef to MCT rather
**	    than listing each platform.
**	4/91 (Mike S)
**	    Use pad space to keep statistics.
**	2-aug-1991 (bryanp)
**	    Add key and flags fields to ME_SEG_INFO as part of adding support
**	    for DI slave I/O directly to/from shared memory.
**	    Added #ifdef wrapper to protect this file from multiple inclusion.
**	21-feb-1992 (jnash)
**	    Revamp configuration to have sun4 use MMAP.
**	06-nov-1992 (mikem)
**	    Change defines to use MMAP on solaris port (as was done for 6.5
**	    generic sun4).
**	15-dec-1992 (mikem)
**	    Fixed typo in above submission.  Use MMAP in the su4_us5 port.
**	08-jan-1993 (mikem)
**	    su4_us5 port.  sbrk() returns different types on different systems
**	    causing a redefiniton compile error on solaris 2.1.  Changed to
**	    include unistd.h and systypes.h which seem to define it on all
**	    machines I could check.  Also cast the usage to sbrk() return to
**	    be a char * to take care of any machine that might not define it
**	    in the included hdrs.
**	20-nov-92 (pearl)
**	    Add prototyped function declarations.  MEerror() is now declared
**	    in me.h.
**	04-feb-93 (smc)
**	    Added include of <time.h> for axp_osf so the stucture tm is
**	    declared for <unistd.h>.
**	02-mar-93 (swm)
**          Use MMAP in axp_osf port in preference to SHMEM and define
**	    ME_SEG_IN_BREAK axp_osf since memory is allocated within the
**	    break region.
**	16-feb-93 (pearl)
**	    Moved declaration of MEmove over to me.h.  Add non-prototype
**	    function declarations for functions that already have prototyped
**	    declarations.  Delete declaration of forcestruct(); this has been
**	    changed to a static function.
**	26-apr-1993 (fredv)
**	    sbrk() isn't defined in any system header of RS6000. We have
**	    to define it for ris_us5.
**	8-jun-93 (ed)
**	    changed to use CL_PROTOTYPED name
**	9-aug-93 (robf)
**          Add su4_cmw
**	11-aug-93 (ed)
**	    unconditional prototypes
**	26-aug-1993 (bryanp)
**	    Moved ME_ALIGN out of this file into mecl.h. All MEget_pages masks,
**		whether visible to the mainline or not, are now in mecl.h, which
**		may not be ideal but it at least reduces the likelihood that
**		two masks will have the same value (until this change ME_ALIGN
**		and ME_NOTPERM_MASK accidentally had the same value).
**	28-oct-93 (swm)
**	    Bug #56448
**	    Added comments to guard against passing a non-varargs function
**	    pointer in the (*fmt) function pointer parameter to MExdump().
**	25-nov-93 (swm)
**	    Bug #58884
**	    Memory is mapped above the brk region on axp_osf so remove
**	    ME_SEG_IN_BREAK definition.
**	11-feb-94 (swm)
**	    Bug #59613
**	    On axp_osf mmap() written pages are flushed from the kernel
**	    buffer cache via the Unix update process (usually every 30
**	    seconds). Avoid this problem by using SHMEM instead of MMAP.
**	24-apr-1994 (jnash)
**	    Solaris has same problem described by swm in last change, 
**	    so also change it back to using SHMEM.
**	23-may-1994 (mikem)
**	    Added FUNC_EXTERN of ME_lock_sysv_mem(), as part of adding 
**	    support to lock memory when system V interface (SHMEM) is used to
**	    allocate memory.
**	25-aug-1995 (rambe99)
**	    Removed sqs_ptx defines for MECOPY_WRAPPED_MEMCPY, etc. because
**	    these are now done in mecl.h.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	20-apr-1995 (emmag)
**	    Desktop porting changes.
**	19-apr-1995 (canor01)
**	    disabled the MCT shared memory functions
**      12-Dec-95 (fanra01)
**          Added IMPORT_DLL_DATA section for references by statically built
**          code to data in DLL.
**	03-jun-1996 (canor01)
**	    Added function prototype for ME_mo_reg().
**	22-nov-1996 (canor01)
**	    Moved "ME_tls..." function declarations here to prevent conflict
**	    with similar GL functions.
**	26-feb-1997 (canor01)
**	    If system TLS functions can be used, #define the equivalents
**	    to the ME_tls functions.
**	07-mar-1997 (canor01)
**	    Cast parameters in ME_tls_get macro to avoid compiler warnings.
**	25-mar-1997 (canor01)
**	    Add TLS initialization and cleanup functions for NT.
**	03-apr-1997 (canor01)
**	    Pad the ME_HEAD structure to match ME_NODE in size, since 
**	    the freelist maintenance functions may sometimes cast it 
**	    to ME_NODE.
**	11-apr-1997 (canor01)
**	    Change CS_tls... functions to CSMT_tls... as part of merging
**	    os-threaded and ingres-threaded servers.
**	13-may-1997 (muhpa01)
**	    Added POSIX_DRAFT_4 versions of tls routines
**      10-jun-1997 (canor01)
**          Allow multiple calls to CSMT_tls_init(), in case initial access
**          comes in from an unexpected path.
**	20-Aug-1997 (kosma01)
**	    Corrent typos in posix thread syntax for ME_tls_createkey.
**	03-nov-1997 (canor01)
**	    Increase size of MEaskedfor from u_12 to i4 to match maximum
**	    amount of memory that can be requested.
**	24-nov-1997 (muhpa01)
**	    Previous change for MEaskedfor caused the size of ME_NODE to
**	    not be a power of 2 (on some machines such as HP) due to
**	    implicit padding between i2 & i4 felds.  Changed the order of
**	    fields in ME_NODE & ME_HEAD to get rid of implicit padding &
**	    made MEpad to be a portable pad to 32 bytes.
**	10-Feb-1998 (bonro01)
**		Add prototype for MEget_seg() function.
**	18-feb-1998 (toumi01)
**	    Add include of <ipc.h> for Linux (lnx_us5) to define key_t.
**	15-Jun-1998 (allmi01)
**	    Added DG specific versions of POSIX calls in the CSMT_tls macros.
**	21-aug-1998 (canor01)
**	    When destroying NT tls, set it zero for consistency.
**	02-dec-1998 (muhpa01)
**	    Remove POSIX_DRAFT_4 code for HP - obsolete.
**	10-may-1999 (walro03)
**	    Remove obsolete version string pyr_u42, sqs_u42, sqs_us5.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      08-Dec-1999 (podni01)
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs
**          this code for some reason removed all over the place.
**      07-Mar-2000 (hweho01)
**          Put back the #else preprocessor directive for ME_tls_createkey
**          definition, it was accidentally dropped by the above change. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Sep-2000 (hanje04)
**	    Add include of <ipc.h> for Alpha Linux (axp_lnx) to define key_t.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	07-Dec-1999 (hanch04)
**	    Remove define for REPORT_ERR, moved it to mexdump.c
**	14-Jun-2000 (hanje04)
**	    Changed #ifdef for inclusion of <ipc.h> for int_lnx to if defined
**	    to add ibm_lnx to platforms which inclued it.
**	07-feb-2001 (somsa01)
**	    On IA64, the calculation of the size of padding would result in
**	    a negative number, as size_t is a 64 bit type. Therefore, the
**	    padding has been increased to start from 64. Also, migrated
**	    NT's "melocal.h" into here.
**	25-oct-2001 (somsa01)
**	    Changed "int_wnt" to "NT_GENERIC".
**	04-Dec-2001 (hanje04)
**	    Added support for IA64 Linux (i64_lnx)
**	17-Dec-2001 (hanje04)
**	   Redefine MEpad to be dependent on sizeof(PTR) so that it is the 
**	   correct size on both 32 and 64 bit platforms. 
**	06-mar-2002 (somsa01)
**	    Generisized the definition of MEpad in ME_HEAD and _me_node.
**	22_mar_2002 (xu$we01)
**	    Remove "(dgi_us5)" to make pthread_getspecific function argument
**	    compatible with that in pthread.h.
**	5-May-2002 (bonro01)
**	    Increase size of padding for all 64-bit platforms by adding test
**	    for LP64.
**	    SGI would fail to compile. AIX and Tru64 would compile without
**	    errors, but would generate incorrect code that cause overlays.
**	09-May-2002 (xu$we01)
**	    Provide proper definition for ME_tls_get() on dgi_us5.
**      28-sep-2002 (devjo01)
**          Remove declare of ME_get_shared.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	31-oct-2002 (devjo01)
**	    Modify proto for ME_align.
**	25-Sep-2003 (hanje04)
**	    Remove declaration of ME_get_shared reintroduced by bad 
**  	    x-integ.
**      08-Jan-2004 (horda03) Bug 111658/INGSRV2680
**          Added declaration for MEclearalloc() and MEALLOCTAG.
**          Also use SIZE_TYPE as the type of paramters referring to
**          a page number.
**      17-Nov-2004 (hanje04)
**          BUG 113487/ISS 13680685
**        We now use posix_memalign() to request memory on linux instead
**        of sbrk() which causes memory to be allocated from an entirely
**        separate address space. As OUT_OF_DATASPACE uses sbrk(0) to
**        verify whether "address" is a valid pointer, it is
**        no longer a valid check for Linux. Further more there doesn't
**        seem to be a valid check at all for this case so we just skip it.
**	30-Nov-2004 (drivi01)
**	  Updated i4 to SIZE_TYPE to port change #473049 to windows.
**	25-Feb-2005 (schka24)
**	    Apply J's change to all "sys_page_map" platforms, not just linux.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	29-Mar-2010 (drivi01)
**	    Update pages to be of type SIZE_TYPE for consistency with
**          function definitions.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
*/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before meprivate.h"
# endif


#if defined(sparc_sol) || defined(xCL_075_SYS_V_IPC_EXISTS)
#define SYS_V_SHMEM
#endif

# ifdef xCL_094_TLS_EXISTS
/*
** If system TLS exists, these macros allow their use, otherwise
** functions will be called
*/
#ifdef NT_GENERIC

#undef  ME_tls_set
#define ME_tls_set(key,value,status)    *status = \
                                           TlsSetValue(key,value)==TRUE?OK:FAIL;
#undef  ME_tls_get
#define ME_tls_get(key,value,status)    {*value = TlsGetValue(key);\
                                         *status=((value!=NULL||\
                                           GetLastError()==NO_ERROR)?OK:FAIL);}
#undef  ME_tls_destroy
#define ME_tls_destroy(key,status)      {PTR p=NULL;\
				  	 ME_tls_get(key,&p,status);\
					 if(p)MEfree(p);\
                                         ME_tls_set(key,0,status);}
# endif /* NT_GENERIC */

#if defined(SYS_V_THREADS)
#undef  ME_tls_createkey
#define ME_tls_createkey(key, status)   *status = \
					thr_keycreate(\
                                          (thread_key_t*)key,\
					  (void(*)(void*))MEfree)
#undef  ME_tls_set
#define ME_tls_set(key,value,status)    *status = \
                                           thr_setspecific((thread_key_t)key,\
							    value)
#undef  ME_tls_get
#define ME_tls_get(key,value,status)    *status=thr_getspecific(\
					   (thread_key_t)key,((void**)(value)))
#undef  ME_tls_destroy
#define ME_tls_destroy(key,status)      *status = 0
#undef  ME_tls_destroyall
#define ME_tls_destroyall(status)       *status = 0

GLOBALREF ME_TLS_KEY TlsIdxSCB;

#define CSMT_tls_init(count,status)       if(!TlsIdxSCB)\
                                            ME_tls_createkey(&TlsIdxSCB,status)
#define CSMT_tls_get(value,status)        ME_tls_get(TlsIdxSCB,value,status)
#define CSMT_tls_set(value,status)        ME_tls_set(TlsIdxSCB,value,status)
#define CSMT_tls_destroy(status)          *status = 0
# endif /* SYS_V_THREADS */

#if defined(POSIX_THREADS)
#undef  ME_tls_createkey
#if defined(DCE_THREADS)
#define ME_tls_createkey(key, status)   if ( pthread_keycreate(\
                                            (pthread_key_t*)key,\
                                            (void(*)(void*))MEfree))\
                                           *status=errno;\
                                        else\
                                           *status=OK
#else
#define ME_tls_createkey(key, status)   *status = \
                                           pthread_key_create(\
					    (pthread_key_t*)key,\
					    (void(*)(void*))MEfree)
# endif /* DCE_THREADS */
#undef  ME_tls_set
# if defined(DCE_THREADS)
#define ME_tls_set(key,value,status)    if (pthread_setspecific(\
                                             (pthread_key_t)key,\
                                             (pthread_addr_t)value))\
                                           *status=errno;\
                                        else\
                                           *status=OK;
#else
#define ME_tls_set(key,value,status)    *status = \
                                           pthread_setspecific(\
					     (pthread_key_t)key,value)
# endif /* DCE_THREADS */
#undef  ME_tls_get
# if defined(DCE_THREADS)
#define ME_tls_get(key,value,status)    { if (pthread_getspecific(\
                                             (pthread_key_t)key,\
                                             (pthread_addr_t*)value))\
                                             *status=errno;\
                                          else\
                                             *status=OK;}
# else
#define ME_tls_get(key,value,status)    {*((void**)(value))=\
					   pthread_getspecific(\
					          (pthread_key_t)key);\
					 *status=OK;}
# endif /* DCE_THREADS */
#undef  ME_tls_destroy
#define ME_tls_destroy(key,status)      *status = 0
#undef  ME_tls_destroyall
#define ME_tls_destroyall(status)       *status = 0
 
GLOBALREF ME_TLS_KEY TlsIdxSCB;
 
#define CSMT_tls_init(count,status)       if(!TlsIdxSCB)\
                                            ME_tls_createkey(&TlsIdxSCB,status)
#define CSMT_tls_get(value,status)        ME_tls_get(TlsIdxSCB,value,status)
#define CSMT_tls_set(value,status)        ME_tls_set(TlsIdxSCB,value,status)
#define CSMT_tls_destroy(status)          *status = 0
# endif /* POSIX_THREADS */

# endif /* xCL_094_TLS_EXISTS */


/*
**	structure of nodes in allocated and free chain
**	The sizeof a header node should be a power of two.
**	The lists are maintined with the QU routines.
**
**	NOTE: if this structure changes, similar changes must
**	be made in the ME_HEAD structure below.
*/

typedef struct _me_node ME_NODE;

struct _me_node
{
    ME_NODE 	*MEnext;	/* These form a QUEUE */
    ME_NODE 	*MEprev;

    SIZE_TYPE	MEsize;		/* Including size of this ME_NODE. */

    SIZE_TYPE	MEaskedfor;	/* Number of bytes requested */

    i2		MEtag;		/* user tag for allocated memory block */
    char	MEpad[(sizeof(PTR) * BITSPERBYTE) - ((2 * sizeof(PTR)) + (2 * sizeof(SIZE_TYPE)) + sizeof(i2))];
};

/*
**  Structure of the allocated and free list headers.  This is
**  A QUEUE structure maintained with the QU routines.
**  It is padded to match ME_NODE in size, since the freelist
**  maintenance functions may sometimes cast it to ME_NODE.
*/
typedef struct
{
    ME_NODE	*MEfirst;	/* these form a QUEUE */
    ME_NODE	*MElast;
    SIZE_TYPE	MEsize;         /* dummy to match ME_NODE */

    SIZE_TYPE	MEaskedfor;	/* dummy to match ME_NODE */

    i2          MEtag;          /* dummy to match ME_NODE */
    char	MEpad[(sizeof(PTR) * BITSPERBYTE) - ((2 * sizeof(PTR)) + (2 * sizeof(SIZE_TYPE)) + sizeof(i2))];
} ME_HEAD;


/* Structure to keep track of memory pages allocated */

typedef struct _MEALLOCTAB MEALLOCTAB;

struct _MEALLOCTAB
{
   MEALLOCTAB *next;
   char       *alloctab;
};

/*
 * Structure to keep track of attached shared memory
 */

typedef	int	ME_SEGID;

typedef struct _ME_SEG_INFO	ME_SEG_INFO;

/*}
** Name: ME_SEG_INFO - structure to keep track of attached shared memory.
**
** Description:
**	Structure that maintains information about discrete segments of memory
**	allocated.  This structure is linked in "address" order to a queue of
**	like structure, thus building a "sparse" allocation bitmap.
**
** History:
**      14-feb-89 (mikem)
**	    added comments about the dynamic length of the allocvec member of
**	    this structure.  
**	2-aug-1991 (bryanp)
**	    Add key and flags fields to ME_SEG_INFO as part of adding support
**	    for DI slave I/O directly to/from shared memory.
*/
struct _ME_SEG_INFO
{
    QUEUE	    q;			/* queue these segment descs together */
    ME_SEGID	    segid;		/* Segment id */
    char	    *addr, *eaddr;	/* beginning and end address of seg */
    SIZE_TYPE	    npages;		/* number of pages in segment */
#ifdef NT_GENERIC
    HANDLE	    mem_handle;		/* The WIN32 NT handle to the
					** associated shared memory.
					*/
    HANDLE	    file_handle;	/* The WIN32 NT handle to the
					** associated shared memory map file.
					*/
#endif  /* NT_GENERIC */
    char	    key[MAX_LOC];	/* the shared memory segment key
					** which this process handed to
					** MEget_pages when it attached
					*/
    i4		    flags;		/* the flags which were passed to
					** MEget_pages when we attached
					*/
    char	    allocvec[4];	/* this is actuall a dummy hdr for a
					** variable length bit map.  When the
					** structure is actually allocated we
					** will tack on enough bytes to the end
					** to handle a bit per "npage".
					*/
};

# if defined(NT_GENERIC)  && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	QUEUE		ME_segpool;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF	QUEUE		ME_segpool;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/*
** How to get more memory and find the end of the current process
*/

# if defined(xMCT) 
extern char	*shsbrk();
# endif

# define	SBRK_FAIL	((char *)-1)

/* This is a performance constant, denoting the size block we get
 * from the OS.  It's best if this is a page or file system block
 * multiple.  Presently 8k.  NOTE:  This must match ME_MPAGESIZE in me.h
 */
# if defined(NT_ALPHA) || defined(NT_IA64)
# define	BLOCKPOWER	17
# else
# define	BLOCKPOWER	18
# endif  /* NT_ALPHA || NT_IA64 */
# define	BLOCKSIZE	(1 << BLOCKPOWER)
# define	BLOCKMASK	(BLOCKSIZE - 1)

/*
** These macros are supposed to simplify reading the code.
*/

/* Return the address of the next node as an i4 */
# define NEXT_NODE(node)	(ME_NODE *)((char *)node + node->MEsize)

/* amount to round a ptr up to get an aligned node */
# define ROUND			(sizeof(ME_NODE) - 1)

/* round size to a node aligned quantity */
# define SIZE_ROUND(A)		(((A) + sizeof(ME_NODE) + ROUND) & ~ROUND)

/* Is this pointer valid? */
# if defined(xMCT) 
# define OUT_OF_DATASPACE(addr)	( (char *)addr >= \
    ((MEadvice == ME_INGRES_THREAD_ALLOC) ? shsbrk(0) : (char *) sbrk(0)) )
# elif defined(xCL_SYS_PAGE_MAP)
/*
** If we're using system mem map functions then there's
** no valid way to check this on Linux so don't try.
*/
# define OUT_OF_DATASPACE(addr)  OK  
# else
# define OUT_OF_DATASPACE(addr)	( (char *)addr >= (char *) sbrk(0) )
# endif

extern	ME_HEAD	MElist;		/* head of allocated list */
extern	ME_HEAD	MEfreelist;	/* head of free list */
extern	bool	MEsetup;	/* Alloc & free lists set up? */
extern	STATUS	MEstatus;	/* Return for errors to caller */
extern	i2	ME_pid;		/* set in MEinitLists for MEid field */
extern	bool	MEgotadvice;	/* MEadvise() called yet? */
extern	i4	MEadvice;	/* advice last given */
extern	char *	MEbrkloc;	/* where the break is now */

/*
**	ME special status code
*/
# define	ME_CORRUPTED		4

/* The following defines are for manageing the allocation bitmap */

# define	ME_BITS_PER_BYTE	8
# define	ME_BYTE_FROM_PAGE(p)	((p) / ME_BITS_PER_BYTE)
# define	ME_BIT_FROM_PAGE(p)	((p) % ME_BITS_PER_BYTE)

# define	ME_LOWCNT(c)	((c) & 07)
# define	ME_MAXCNT(c)	(((c) >> 3) & 07)
# define	ME_UPCNT(c)	(((c) >> 6) & 07)
# define	ME_ALLSET	01000
# define	ME_ALLCLR	02000

/* function declarations - prototyped */
FUNC_EXTERN void
MEactual(
	SIZE_TYPE	*user,
	SIZE_TYPE	*actual);
FUNC_EXTERN bool
MEisalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages,
	bool	isalloc);
FUNC_EXTERN void
MEsetalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages);
FUNC_EXTERN void
MEclearalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages);
FUNC_EXTERN STATUS
ME_reg_seg(
	PTR	addr,
#ifdef NT_GENERIC
	SIZE_TYPE	pages,
	HANDLE	sh_mem_handle,
	HANDLE	file_handle);
#else
	ME_SEGID segid,
	SIZE_TYPE	pages,
	char	*key,
	i4	flags);
#endif  /* NT_GENERIC */
FUNC_EXTERN ME_SEG_INFO *
ME_find_seg(
	char	*addr,
	char	*eaddr,
	QUEUE	*segq);
FUNC_EXTERN QUEUE *
ME_rem_seg(
	ME_SEG_INFO	*seginf);
FUNC_EXTERN STATUS
ME_offset_to_addr(
	char		*seg_key,
	SIZE_TYPE	offset,
	PTR		*addr);
FUNC_EXTERN STATUS
MEnewalloctab(
	MEALLOCTAB	*alloctab);
FUNC_EXTERN bool
MEfindpages(
	SIZE_TYPE	pages,
	SIZE_TYPE       *pageno);
FUNC_EXTERN VOID
MEsetpg(
	char	*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	 pages);
FUNC_EXTERN VOID
MEclearpg(
	char	*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	 pages);
FUNC_EXTERN bool
MEalloctst(
	char	*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages,
	bool	allalloc);
FUNC_EXTERN STATUS
MEconsist(
		i4 (*printf)());
FUNC_EXTERN STATUS
MEdoAlloc(
	i2	tag,
	i4	num,
	SIZE_TYPE size,
	PTR	*block,
	bool	zero);
FUNC_EXTERN STATUS
MEdump(
	i4	chain);
FUNC_EXTERN STATUS
MEfadd(
	ME_NODE	*block,
	i4 releasep);
FUNC_EXTERN STATUS
MEinitLists(
	VOID);
FUNC_EXTERN STATUS
ME_alloc_brk(
	i4		flag,
	SIZE_TYPE		pages,
	PTR		*memory,
	SIZE_TYPE		*allocated_pages,
	CL_ERR_DESC	*err_code);
FUNC_EXTERN i4
MEptrdif(
	char	*ptr1,
	char	*ptr2);
FUNC_EXTERN char *
MEptradd(
	char	*ptr,
	i4	offset);
FUNC_EXTERN STATUS
ME_alloc_shared(
	i4		flag,
	SIZE_TYPE	pages,
	char		*key,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code);
FUNC_EXTERN STATUS
ME_align(
	SIZE_TYPE	*pgsize);
FUNC_EXTERN STATUS
ME_attach(
	i4		flags,
	SIZE_TYPE	pages,
	ME_SEGID	segid,
	PTR		*addr,
	char		*key,
	CL_ERR_DESC	*err_code);
FUNC_EXTERN STATUS
MEdetach(
	char		*key,
	PTR		address,
	CL_ERR_DESC	*err_code);
FUNC_EXTERN key_t
ME_getkey(
	char		*user_key);
#ifdef NT_GENERIC
FUNC_EXTERN HANDLE
ME_makekey(
	char		*user_key);
#else
FUNC_EXTERN STATUS
ME_makekey(
	char		*user_key);
#endif  /* NT_GENERIC */
FUNC_EXTERN STATUS
MEseg_free(
	PTR		*addr,
	SIZE_TYPE	*pages,
	CL_ERR_DESC	*err_code);
FUNC_EXTERN STATUS
ME_destroykey(
	char		*user_key);
FUNC_EXTERN STATUS
MEget_seg(
	char		*addr,
	char		**key,
	void		**base);
FUNC_EXTERN VOID
IIME_atAddTag(
	i4	tag,
	ME_NODE	*node);
FUNC_EXTERN STATUS
IIME_ftFreeTag(
	i4	tag);
FUNC_EXTERN i4
MEuse(
	VOID);
FUNC_EXTERN STATUS
MExdump(
	i4	chain,
	i4	(*fmt)());	/* must be a varargs function */
FUNC_EXTERN SIZE_TYPE
MEtotal(
	i4 which);
FUNC_EXTERN i4
MEobjects(
	i4 which);

FUNC_EXTERN STATUS
ME_lock_sysv_mem(
    char            *key,
    CL_ERR_DESC     *err_code);

FUNC_EXTERN VOID
ME_mo_reg(
     VOID );

#ifndef ME_tls_createkey
#define ME_tls_createkey IIME_tls_createkey
FUNC_EXTERN VOID
ME_tls_createkey(
    ME_TLS_KEY  *key,
    STATUS      *status
);
#endif

#ifndef ME_tls_set
#define ME_tls_set IIME_tls_set
FUNC_EXTERN VOID
ME_tls_set(
    ME_TLS_KEY  key,
    PTR  	value,
    STATUS      *status
);
#endif

#ifndef ME_tls_get
#define ME_tls_get IIME_tls_get
FUNC_EXTERN VOID
ME_tls_get(
    ME_TLS_KEY  key,
    PTR  	*value,
    STATUS      *status
);
#endif

#ifndef ME_tls_destroy
#define ME_tls_destroy IIME_tls_destroy
FUNC_EXTERN VOID
ME_tls_destroy(
    ME_TLS_KEY  key,
    STATUS	*status
);
#endif

#ifndef ME_tls_destroyall
#define ME_tls_destroyall IIME_tls_destroyall
FUNC_EXTERN VOID
ME_tls_destroyall( STATUS *status );
#endif

FUNC_EXTERN STATUS MEshared_free(
		PTR         *addr,
		SIZE_TYPE         *pages,
		CL_ERR_DESC *err_code
);

# endif		/* ME_PRIVATE_INCLUDED */
/* end of meprivate.h */
