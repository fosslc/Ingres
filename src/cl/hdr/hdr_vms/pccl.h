/*
** Copyright (c) 1985, 2003 Ingres Corporation
*/

/**
** Name: pccl.h - Global definitions for the PC compatibility library.
**
** Description:
**      The file contains the type used by PC and the definition of the
**      PC functions.  These are used for process control.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	09-jan-86 (jeff)
**	    Corrected definition of PC_WAIT.  There was no # on the define.
**	24-may-1993 (bryanp)
**	    Add prototye for PCforce_exit();
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	05-aug-1997 (teresak)
**	    Integration of Unix CL changes for change 427955
**    	    14-Nov-96 (gordy)
**            Added PCtid() macro.
**	01-jul-1998 (kinte01)
**	    Changed PC_NOWAIT to PC_NO_WAIT which is the way it is defined
**	    in Unix.
**	30-dec-1999 (kinte01)
**	    Added PCspawn
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	13-Feb-2003 (hanje04)
**	    BUG 109555
**	    Add PCmpid() to specifically get the master process ID for a multi-
**	    threaded program on Linux using getpgrp(). On all other platforms
**	    this will just be defined at PCpid().
**	17-sep-2003 (abbjo03)
**	    The VMS process ID is an unsigned longword.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**/

/* 
** Defined Constants
*/

/* PC return status codes. */

#define                 PC_OK           0

/* PC constants. */

# define	PIPE		i4

# define	PID		u_i4

/*
**Macro for obtaining master PID on Linux.
** Same as PCpid() for other platforms
*/

#if defined(int_lnx) || defined(ibm_lnx) || defined(axp_lnx) || \
    defined(a64_lnx) || defined(i64_lnx) || defined(int_rpl)
# define	PCmpid(pid)	*pid = getpgrp()
#else
# define	PCmpid(pid)	PCpid(pid)
# endif

/*
** Defines macro to return current thread ID.
** For none threaded systems, return fixed value.
*/

# ifdef OS_THREADS_USED

# ifdef POSIX_THREADS

# define        PCtid()         (i4)pthread_self()

# endif

# ifdef SYS_V_THREADS

# define        PCtid()         (i4)thr_self()

# endif

# ifdef NT_GENERIC

# define        PCtid()         (i4)GetCurrentThreadId()

# endif

# else /* OS_THREADS_USED */

# define        PCtid()         ((i4)0)
 
# endif /* OS_THREADS_USED */

# define	PC_WAIT			0
# define	PC_NO_WAIT		1


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS PCspawn(
#ifdef CL_PROTOTYPED
	i4 		argc,
	char		**argv,
	i1		wait,
	LOCATION	*in_name,
	LOCATION	*out_name,
	PID		*pid
#endif
);
