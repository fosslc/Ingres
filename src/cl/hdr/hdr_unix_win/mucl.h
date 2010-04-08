/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <systypes.h>
# include <cs.h>

/**
** Name:	mucl.h	- CL private interface for MU mutex functions.
**
** Description:
**	Defines the CL private interface to support the GL public
**	interface of the MU mutex module.  This interface permits
**	semaphore support in multi-threaded application environments.
**	Support for multi-threaded servers is provided through CS.
**
** History:
**	14-Nov-96 (gordy)
**	    Created.
**	12-dec-1996 (canor01)
**	    Make slightly more generic using existing CL macros for
**	    ease in porting.
**	20-dec-1996 (kch)
**	    Move include of cs.h from here to mucl.c. This prevents csnormal.h
**	    being included twice in those files that include cs.h and mu.h
**	    (eg. csinterface.c, moutil.c).
**	 7-jan-1997 (kch)
**	    Undo previous change - cs.h will now only be included once.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-aug-2009 (Bruce Lunsford)  Sir 122501
**	    On Windows, convert the underlying OS synchronization object from
**	    a mutex to a CriticalSection, which has less overhead.
**/

/*
** Datatype used by the CL semaphore functions.  This
** must be a structure to permit forward referencing
** in the MU public header file for definition of the
** CL private interface.  On systems which do not
** support multi-threaded applications, this may have
** a single, unused element.
*/

struct II_MU_CL_SEMAPHORE
{

# ifdef OS_THREADS_USED

#  ifdef NT_GENERIC
    CRITICAL_SECTION	mu_synch;
#  else
    CS_SYNCH		mu_synch;
#  endif /* NT_GENERIC */

# else /* OS_THREADS_USED */

    i4 		mu_unused;	/* Unused - threads not supported */

# endif /* OS_THREADS_USED */

};

/*
** Macro function to provide access to the CL semaphore 
** functions data structure.  On non-threaded systems, 
** the macro may simply return NULL.  The macro may also 
** resolve to an actual function call, in which case the 
** function must match the following prototype:
**
** FUNC_EXTERN MU_CL_SEM_FUNCS *MUcl_init( VOID );
*/

# ifdef OS_THREADS_USED

FUNC_EXTERN	MU_CL_SEM_FUNCS		*MUcl_init( VOID );
# define	MU_CL_SEM_INIT()	MUcl_init()

# else /* OS_THREADS_USED */

# define 	MU_CL_SEM_INIT()	((MU_CL_SEM_FUNCS *)NULL)

# endif /* OS_THREADS_USED */


