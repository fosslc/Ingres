/*
** Copyright (c) 1985, 2003 Ingres Corporation
*/

/**
** Name: ME.H - Global definitions for the ME compatibility library
**
** Description:
**      The file contains the type used by ME and the definition of the
**      ME functions.  These are used for memory manipulation.
**
** History:
**	89/05/31  wong
**	Added MECOPY macros (as covers.)
**	21-Jul-1988 (anton)
**	    Added page extentions for DBMS use.
**      17-sep-1985 (derek)
**          Updated to codinng standard for 5.0.
**	28-feb-1989 (rogerk)
**	    Added ME_NOTPERM_MASK flag for shared memory allocations.
**	16-oct-90 (andre)
**	    added ME_H_INCLUDE to ela with other header files including this
**	    file.
**	07-Jan-1991 (anton)
**	    Add ME_INGRES_THREAD_ALLOC support
**	14-apr-1992 (jnash)
**	    Add ME_NO_SHMEM_LOCK, ME_NO_MLOCK, ME_LOCK_FAIL and ME_LCKPAG_FAIL.
**	30-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    declarations.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      24-nov-1992 (walt)
**          Brought ME_NOT_ALIGN_MACRO over from UNIX me.h to
**          satisfy a use in qef!qee.c.  This hasn't been needed
**          before on VMS because VMS on VAX doesn't have
**          BYTE_ALIGN defined.  However, the Alpha VMS version
**          runs with BYTE_ALIGN defined and now needs this macro.
**	15-jan-1998 (teresak)
**	   Add a MEcopylng macro wrapper
**      22-Aug-2000 (horda03)
**         Add definition indcating memory to be taken from P1 space.
**         (102291)
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	25-oct-2001 (somsa01)
**	    The return type of MEactual() is void.
**	19-sep-2001 (kinte01)
**	    added MECOPY_CONST_#_MACROs for 2,4 byte copy
**	06-nov-2001 (kinte01)
**	    forgot to add the M1 macro definition for previous change
**	23-Oct-2002 (hanch04)
**	    Make pagesize variables of type long (L)
**	05-sep-2003 (abbjo03)
**	    Add include of string.h.
**	23-oct-2003 (devjo01)
**	    Add ME_LOCAL_RAD to advise page allocator to inform underlaying
**	    OS to allocate the request from memory physically on the
**	    home Resource Affinity Domain (RAD) of the calling process.
**	    This is only of concern for Non-Uniform Memory Architecture
**	    (NUMA) machines, and will be ignored on non-NUMA boxes.
**      08-oct-2008 (stegr01)
**          Add define for ME_TUXEDO_ALLOC
**/

#ifndef	ME_H_INCLUDE	/* this MUST be the FIRST line */
#define ME_H_INCLUDE	1
#include <string.h>	/* for prototypes of memcpy, memcmp, etc. */

/*}
** Name: LIFOBUF - Type used for LIFO memory allocation
**
** Description:
**      This structure is used to control allocation and deallocation
**      from a LIFO buffer.
**
** History:
**     17-sep-1985 (derek)
**          Upgraded to coding standard for 5.0.
*/
typedef struct _LIFOBUF
{
	i4	lb_nleft;		/* bytes left */
	i4	lb_err_num;		/* error code on overflow */
	i4	(*lb_err_func)();	/* error function on overflow */
	char	*lb_xfree;		/* next free byte */
	char	lb_buffer[4];		/* beginning of buffer area */
}	LIFOBUF;

/*
**  Forward and/or External function references. 
*/
FUNC_EXTERN STATUS MEcalloc(
#ifdef CL_PROTOTYPED
        i4      num,
        i4      size,
        i4      *block
#endif
);

FUNC_EXTERN STATUS MEtalloc(
#ifdef CL_PROTOTYPED
        i2      tag,
        i4      num,
        i4      size,
        i4      *block
#endif
);

FUNC_EXTERN STATUS MEtcalloc(
#ifdef CL_PROTOTYPED
        i2      tag,
        i4      num,
        i4      size,
        i4      *block
#endif
);


FUNC_EXTERN void MEactual(
#ifdef CL_PROTOTYPED
        SIZE_TYPE    *user,
        SIZE_TYPE    *actual
#endif
);

FUNC_EXTERN STATUS MEneed(
#ifdef CL_PROTOTYPED
        LIFOBUF *buf,
        i4              nbytes,
        char            **buf_ptr
#endif
);

FUNC_EXTERN STATUS MEinitbuf(
#ifdef CL_PROTOTYPED
        LIFOBUF *buf,
        i4                      size,
        i4                      err_num,
        i4                      (*err_func)()
#endif
);

FUNC_EXTERN STATUS MEfbuf(
#ifdef CL_PROTOTYPED
        LIFOBUF *buf,
        i4                      bytes
#endif
);

FUNC_EXTERN i4 MEmarkbuf(
#ifdef CL_PROTOTYPED
        LIFOBUF         *buf
#endif
);

FUNC_EXTERN VOID MEseterr(
#ifdef CL_PROTOTYPED
        LIFOBUF                 *buf,
        i4                      errnum,
        i4                      (*err_func)()
#endif
);

FUNC_EXTERN i4 MEuse(
#ifdef CL_PROTOTYPED
        VOID
#endif
);

FUNC_EXTERN STATUS MEalloc(
#ifdef	CL_PROTOTYPED
	i4	num,
	i4	size,
	i4	*block
#endif
);


/* 
** Defined Constants
*/

/*
**      Thread-local storage
*/
# define        ME_MAX_TLS_KEYS 32      /* Maximum TLS unique keys */
# define        ME_TLS_KEY      i4      /* Thread-local storage key */


/* ME return status codes. */

# define	 ME_OK           0
# define	 ME_GOOD	(E_CL_MASK + E_ME_MASK + 0x01)
	/*  ME routine: The Status returned was good  */
# define	 ME_BD_CHAIN	(E_CL_MASK + E_ME_MASK + 0x02)
	/*  MEdump: correct parameter value must be one of ME_D_ALLOC, 
        **  ME_D_FREE, ME_D_BOTH  */
# define	 ME_BD_CMP	(E_CL_MASK + E_ME_MASK + 0x03)
	/*  MEcmp: number of bytes to compare must be > 0  */
# define	 ME_BD_COPY	(E_CL_MASK + E_ME_MASK + 0x04)
	/*  MEcopy: number of bytes to copy must be > 0  */
# define	 ME_BD_FILL	(E_CL_MASK + E_ME_MASK + 0x05)
	/*  MEfill: number of bytes to fill must be > 0  */
# define	 ME_BD_TAG	(E_CL_MASK + E_ME_MASK + 0x06)
	/*  MEt[alloc, free]: tags must be > 0  */
# define	 ME_ERR_PROGRAMMER	(E_CL_MASK + E_ME_MASK + 0x07)
	/*  MEfree: There is something wrong with the way this 
        **  routine was programmed.  Sorry.  */
# define	 ME_FREE_FIRST	(E_CL_MASK + E_ME_MASK + 0x08)
	/*  ME[t]free: can't free a block before any blocks have 
        **  been allocated  */
# define	 ME_GONE	(E_CL_MASK + E_ME_MASK + 0x09)
	/*  ME[t]alloc: system can't allocate any more memory for 
        **  this process  */
# define	 ME_NO_ALLOC	(E_CL_MASK + E_ME_MASK + 0x0A)
	/*  ME[t]alloc: request to allocate a block with zero (or less) 
        **  bytes was ignored  */
# define	 ME_NO_FREE	(E_CL_MASK + E_ME_MASK + 0x0B)
	/*  MEfree: can't find a block with this address in the free 
        **  list  */
# define	 ME_NO_TFREE	(E_CL_MASK + E_ME_MASK + 0x0C)
	/*  MEtfree: process hasn't allocated any memory with this tag  */
# define	 ME_00_PTR	(E_CL_MASK + E_ME_MASK + 0x0D)
	/*  ME[t]alloc: passed a null pointer  */
# define	 ME_00_CMP	(E_CL_MASK + E_ME_MASK + 0x0E)
	/*  MEcmp: passed a null pointer  */
# define	 ME_00_COPY	(E_CL_MASK + E_ME_MASK + 0x0F)
	/*  MEcopy: passed a null pointer  */
# define	 ME_00_DUMP	(E_CL_MASK + E_ME_MASK + 0x10)
	/*  MEdump: passed a null pointer  */
# define	 ME_00_FILL	(E_CL_MASK + E_ME_MASK + 0x11)
	/*  MEfill: passed a null pointer  */
# define	 ME_00_FREE	(E_CL_MASK + E_ME_MASK + 0x12)
	/*  MEfree: passed a null pointer  */
# define	 ME_TR_FREE	(E_CL_MASK + E_ME_MASK + 0x13)
	/*  MEfree: the memory has been corrupted  */
# define	 ME_TR_SIZE	(E_CL_MASK + E_ME_MASK + 0x14)
	/*  MEsize: the memory has been corrupted  */
# define	 ME_TR_TFREE	(E_CL_MASK + E_ME_MASK + 0x15)
	/*  MEtfree: the memory has been corrupted  */
# define	 ME_OUT_OF_RANGE	(E_CL_MASK + E_ME_MASK + 0x16)
	/*  ME routine: address is out of process's data space, 
        **  referencing will cause bus error  */
# define	 ME_BF_OUT	(E_CL_MASK + E_ME_MASK + 0x17)
	/* MEneed: 'buf' doesn't have 'nbytes' left to allocate */
# define	 ME_BF_ALIGN	(E_CL_MASK + E_ME_MASK + 0x18)
	/* MEinitbuf: 'buf' not aligned */
# define	 ME_BF_FALIGN	(E_CL_MASK + E_ME_MASK + 0x19)
	/* MEfbuf: 'buf' not aligned */
# define	 ME_BF_PARAM	(E_CL_MASK + E_ME_MASK + 0x1A)
	/* MEfbuf: 'bytes' argument must come from call to MEfbuf() */
# define	 ME_TOO_BIG	(E_CL_MASK + E_ME_MASK + 0x1B)
	/* MEreqmem/MEreqlng/ME[t][c]alloc: Object too big */
# define         ME_OUT_OF_MEM	(E_CL_MASK + E_ME_MASK + 0x1C)
	/* MEget_pages: Can not expand memory size */
# define         ME_BAD_PARAM	(E_CL_MASK + E_ME_MASK + 0x1D)
	/* Bad Paramater to ME routines */
# define         ME_ILLEGAL_USAGE (E_CL_MASK + E_ME_MASK + 0x1E)
	/* Combination of arguments to an ME routine is invalid */
# define         ME_BAD_ADVICE	(E_CL_MASK + E_ME_MASK + 0x1F)
	/* An ME routine will not function with the given MEadvice) */
# define         ME_ALREADY_EXISTS (E_CL_MASK + E_ME_MASK + 0x20)
	/* A shared memory segment to be created already exists */
# define         ME_NO_SUCH_SEGMENT (E_CL_MASK + E_ME_MASK + 0x21)
	/* A shared memory segment to be manipulated does not exist */
# define         ME_NO_SHARED	(E_CL_MASK + E_ME_MASK + 0x22)
	/* This CL does not support shared memory */
# define         ME_NOT_ALLOCATED (E_CL_MASK + E_ME_MASK + 0x23)
	/* The memory specified for some ME operation was not all allocated */
# define         ME_NOT_SUPPORTED (E_CL_MASK + E_ME_MASK + 0x24)
	/* The ME operation requested is not supported in this CL */
# define         ME_NO_PERM	(E_CL_MASK + E_ME_MASK + 0x25)
	/* Insufficient permission for some ME operation */

# define         ME_NO_SHMEM_LOCK  (E_CL_MASK + E_ME_MASK + 0x27)
	/* Cannot lock DMF cache on sys V shmem configuration */
# define         ME_NO_MLOCK       (E_CL_MASK + E_ME_MASK + 0x28)
	/* Cannot lock DMF cache if without mlock() function */
# define         ME_LOCK_FAIL      (E_CL_MASK + E_ME_MASK + 0x29)
	/* Cache Lock operation failed  */
# define	 ME_LCKPAG_FAIL	   (E_CL_MASK + E_ME_MASK + 0x2A)
	/* Cache lock operation failed (VMS) */

/* codes for MEadvise */
# define                 ME_INGRES_ALLOC	1
	    /* ME_INGRES_THREAD_ALLOC is same as ME_INGRES_ALLOC for this CL */
# define		 ME_INGRES_THREAD_ALLOC	1
# define                 ME_USER_ALLOC		2

# define		ME_MPAGESIZE	512L
# define		ME_MAX_ALLOC	(1 << 16)

/*
**	The following symbols are for calling MEget_pages.
*/

# define	ME_NO_MASK	0	/* None of the below flags apply */
# define	ME_MZERO_MASK	0x01	/* Zero the memory before returning */
# define	ME_MERASE_MASK	0x02	/* Erase the memory (DoD specific) */
# define	ME_IO_MASK	0x04	/* memory used for I/O */
# define	ME_SPARSE_MASK	0x08	/* memory used `sparsely' so it should
					   be pages in small chunks */
# define	ME_LOCKED_MASK	0x10	/* memory should be locked down to
					   inhibit paging -- i.e. caches */
# define	ME_SSHARED_MASK	0x20	/* memory shared across process on a
					   single machine */
# define	ME_MSHARED_MASK	0x40	/* memory shared with other machines */
# define	ME_CREATE_MASK	0x80	/* If shared memory doen't yet exist
					   it may be created.  (Usually
					   specified with ME_ZERO_MASK)
					   If the shared memory alread exists
					   it is an error to create it. */
# define	ME_ADDR_SPEC	0x0100	/* memory must be allocated at this
					   address (UNIX CL use only) */
# define	ME_NOTPERM_MASK	0x0200	/* memory segment should not be created
					   permanent */
# define	ME_USE_P1_SPACE 0x0400  /* memory should be allocated in P1
                                           space. (VMS CL use only) */
# define	ME_LOCAL_RAD	0x1000  /* Memory to be allocatted out of
					   physical memory local to the
					   processes Resource Affinity Domain.
					   (NUMA only, otherwise ignored)*/

# define        ME_TUXEDO_ALLOC  4

/*
**	The following symbols are for calling MEprot_pages.
*/
# define	ME_PROT_READ	0x4	/* pages should be readable */
# define	ME_PROT_WRITE	0x2	/* pages should be writable */
# define	ME_PROT_EXECUTE	0x1	/* pages should be executable */



/*:
** Name:	MECOPY_MACRO
**
** Description:
**	Generalized MEcopy macros to generate in-line copy code or
**	a function call to MEcopy depending upon size of copy buffer.
**
** History:
**	05/89 (jhw) -- Implemented as covers on VAX/VMS.
*/

/* ================ MECOPY_CONST_MACRO ================ */

# define MECOPY_CONST_MACRO( s, n, d ) memcpy(d,s,n)

/*
** Unaligned one byte move with offset
** s = source, o = offset, d = destination
*/
# define M1(s,o,d) (((char*)(d))[o]=((char*)(s))[o])

/*
** The following MECOPY_CONST_N_MACROs were adapted from an old
** version of MECOPY_CONST_MACRO which was found to have no performance
** improvements, probably due to the length tests being done in the
** original macro. These versions are coded for specific lengths.
*/
# define MECOPY_CONST_2_MACRO( s, d ) \
{ M1(s,0,d); M1(s,1,d); }

# define MECOPY_CONST_4_MACRO( s, d ) \
{ M1(s,0,d); M1(s,1,d); M1(s,2,d); M1(s,3,d); }

/* ================ MECOPY_VAR_MACRO ================ */

# define MECOPY_VAR_MACRO( s, n, d ) memcpy(d,s,n)


#define MEcopy(source, n, dest)         memcpy(dest, source, n)
#define MEmemcpy                        memcpy
#define MEmemccpy                       memccpy
#define MEfill(n,v,s)                   memset(s, v, n)
#define MEfilllng(n,v,s)                memset(s, v, n)
#define MEcmp(s1,s2,n)                  memcmp(s1, s2, n)
#define MEmemcmp                        memcmp
#define MEmemmove                       memmove


/*}
** Name: ME_NOT_ALIGNED_MACRO - is a ptr aligned to worst cast boundary.
**
** Description:
**  ME_NOT_ALIGNED_MACRO() is to be used to determine if a ptr
**      is aligned to the worst case boundary for the machine.  This is the
**  standard test used throughout the backend when the datatype of the
**  item in question is unknown.  More work could be done in this area
**  to save unecessary alignment work with support from adf since all most
**  of the code usually knows is the datatype id of the item in question.
**
**  This macro will always evaluate to FALSE if the machine is not
**  a BYTE_ALIGN architecture.  On some non-BYTE_ALIGN machines
**  ALIGN_RESTRICT has been defined to be long rather than char.  What
**  is really desired is some version of "ALIGN_RESTRICT_DESIRED" and
**  "ALIGN_RESTRICT_REQUIRED" values so that the code can align data
**  for performance if it has to move the data anyway, but can decide
**  not do some work on non-BYTE_ALIGN machines.
**
** History:
**     16-feb-1990 (mikem)
**          Created
**		24-nov-1992 (walt)
**			Brought from UNIX me.h 
*/
# ifdef BYTE_ALIGN
# define  ME_NOT_ALIGNED_MACRO(ptr) (((i4) ptr) & (sizeof(ALIGN_RESTRICT) - 1))
# else
# define  ME_NOT_ALIGNED_MACRO(ptr) (0)
# endif

#endif	    /* ME_H_INCLUDE - this MUST be the LAST line */
