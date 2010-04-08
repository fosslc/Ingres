/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# ifndef MU_HDR_INCLUDED
# define MU_HDR_INCLUDED

/**
** Name:	mu.h	- public interface for MU mutex functions.
**
** Description:
**	Defines the public interface for the MU mutex module of
**	the GL.  MU provides MU_SEMAPHORE objects that may be used
**	in any program to syncronize access to variables that might
**	be corrupted in a multi-threaded environment.  When used
**	in a single-threaded application environment, these do 
**	nothing except some deadlock detection.  When the same code 
**	is linked in to a program that is CSinitiate-ed, then the 
**	same calls actually use CS_SEM_SINGLE semaphores for access 
**	control.  The CL may also provide semaphore routines for
**	support of multi-threaded application environments.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	04-jun-1996 (canor01)
**	    Add prototype for MUw_semaphore().  Changed MU_SEM_NAME_LEN
**	    to 32 to correspond with other semaphore names.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
**	21-jan-1999 (canor01)
**	    Add definitions for standard MU error codes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-jun-2002 (devjo01)
**	    Add SID support to resolve b108015.
**/

typedef struct II_MU_SEMAPHORE		MU_SEMAPHORE;
typedef struct II_MU_CL_SEMAPHORE	MU_CL_SEMAPHORE;

typedef	PTR				MU_SID;

# define MU_SEM_NAME_LEN	32

/*
** STATUS values of various functions, from <erglf.h>
*/
# define E_GL_MASK              0xD50000
# define E_MU_MASK              0x5000
# define MUDEF_SEM_INITIALIZED	(E_GL_MASK + E_MU_MASK + 0x02)
# define MUDEF_REM_NON_DEF	(E_GL_MASK + E_MU_MASK + 0x03)
# define MUDEF_P_NON_DEF	(E_GL_MASK + E_MU_MASK + 0x04)
# define MUDEF_P_DEADLOCK	(E_GL_MASK + E_MU_MASK + 0x05)
# define MUDEF_V_NON_DEF	(E_GL_MASK + E_MU_MASK + 0x06)
# define MUDEF_V_BAD_VAL	(E_GL_MASK + E_MU_MASK + 0x07)
# define MUCS_SEM_INITIALIZED	(E_GL_MASK + E_MU_MASK + 0x08)
# define MUCS_SEM_OVER_CHANGE	(E_GL_MASK + E_MU_MASK + 0x09)
# define MUCS_P_SEM_BAD_STATE	(E_GL_MASK + E_MU_MASK + 0x0a)
# define MUCS_V_SEM_BAD_STATE	(E_GL_MASK + E_MU_MASK + 0x0b)
# define MUCS_R_SEM_BAD_STATE	(E_GL_MASK + E_MU_MASK + 0x0c)


/* ----------------------------------------------------------------
**   
** These are function types used in switching actions around,
** and handed as arguments to MUset_funcs.
*/

/*}
** Name:	MU_I_SEM_FUNC	- initialize function for MUset_funcs.
**
** Description:
**	This is a function given to MUset_funcs.  If current mu_state
**	MU_UNKNOWN or MU_REMOVED, then it's an error.
**
** Inputs:
**	sem	Sem to initialize.
**
** Outputs:
**	sem	is initialized.
**
** Returns:
**	OK	if sucessful.
**	other	error status if a problem is encountered, such as
**		already being initialized.  Don't test for these,
**		just look them up and report.  They are always
**		programming errors.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

typedef STATUS	MU_I_SEM_FUNC( MU_SEMAPHORE *sem );
typedef STATUS	MU_CL_I_SEM_FUNC( MU_CL_SEMAPHORE *sem );

/*}
** Name:	MU_N_SEM_FUNC	 - name semaphore function for MUset_funcs.
**
** Description:
**	Give the MU_SEMAPHORE a name.  If mu_state is was CS_USER, then
**	initialize in new way and name the approriate way too.
**
** Inputs:
**	sem	Sem to name
**	name	Name for semaphore
**
** Outputs:
**	sem	is named
**
** Returns:
**	void
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

typedef void	MU_N_SEM_FUNC( MU_SEMAPHORE *sem, char *name );
typedef void	MU_CL_N_SEM_FUNC( MU_CL_SEMAPHORE *sem, char *name );


/*}
** Name:	MU_R_SEM_FUNC	-remove sem function given to MUset_funcs.
**
** Description:
**	Remove a semaphore.  If semaphore is already held or in a bad
**	state to do this, return an error.
**
** Inputs:
**	sem	Sem to remove.
**
** Outputs:
**	sem	is marked removed, if it was.
**
** Returns:
**	OK	if sem was removed.
**	other	reportable status if something was wrong, like it
**		was still held by someone.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

typedef STATUS	MU_R_SEM_FUNC( MU_SEMAPHORE *sem );
typedef STATUS	MU_CL_R_SEM_FUNC( MU_CL_SEMAPHORE *sem );


/*}
** Name:	MU_P_SEM_FUNC	- p function given to MUset_funcs.
**
** Description:
**	Lock a semaphore.  It's an error to ask for it if you already
**	hold it.
**
** Inputs:
**	sem	Sem to lock.
**
** Outputs:
**	sem	is modified.
**
** Returns:
**	OK		if sem was locked for you.
**	CSp_semaphore	return values
**	other		reportable status if something was wrong,
**			like you already hold it, or an interrupt
**			broke your blocking request.
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

typedef STATUS  MU_P_SEM_FUNC( MU_SEMAPHORE *sem );
typedef STATUS	MU_CL_P_SEM_FUNC( MU_CL_SEMAPHORE *sem );

/*}
** Name:	MU_V_SEM_FUNC	- v function given to MUset_funcs.
**
** Description:
**	Release a semaphore.  It's an error to release a semaphore
**	you don't hold.
**
** Inputs:
**	sem	Sem to release.
**
** Outputs:
**	sem	is modified.
**
** Returns:
**	OK	if sem was removed.
**	other	reportable status if something was wrong, like it
**		was still held by someone.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

typedef STATUS	MU_V_SEM_FUNC( MU_SEMAPHORE *sem );
typedef STATUS	MU_CL_V_SEM_FUNC( MU_CL_SEMAPHORE *sem );

/*}
** Name:	MU_SID_FUNC	- Self ID function given to MUset_funcs.
**
** Description:
**	Return a unique persistent identifier for the current thread.
**	Use for identity comparison only.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	Unique persistent identifier for current thread.
**
** History:
**	13-jun-2002 (devjo01)
**	    created.
*/

typedef MU_SID	MU_SID_FUNC( void );


/*}
** Name:	MU_SEMAPHORE_FUNCS	- struct handed to MUset_funcs.
**
** Description:
**	One of these is handed to MUset_funcs to change the routines
**	that are working on MU_SEMAPHORES.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
**	13-jun-2002 (devjo01)
**	    Added mu_sid & mucl_sid.
*/

typedef struct
{

    MU_I_SEM_FUNC	*mu_isem;
    MU_N_SEM_FUNC	*mu_nsem;
    MU_R_SEM_FUNC	*mu_rsem;
    MU_P_SEM_FUNC	*mu_psem;
    MU_V_SEM_FUNC	*mu_vsem;
    MU_SID_FUNC		*mu_sid;
} MU_SEMAPHORE_FUNCS;

typedef struct
{

    MU_CL_I_SEM_FUNC	*mucl_isem;
    MU_CL_N_SEM_FUNC	*mucl_nsem;
    MU_CL_R_SEM_FUNC	*mucl_rsem;
    MU_CL_P_SEM_FUNC	*mucl_psem;
    MU_CL_V_SEM_FUNC	*mucl_vsem;
    MU_SID_FUNC		*mucl_sid;
} MU_CL_SEM_FUNCS;

/*
** Now include the CL level MU header file to define
** the II_MU_CL_SEMAPHORE data structure and a macro
** function, MU_CL_SEM_INIT(), to provide access to
** the CL semaphore data structure.  On non-threaded
** systems, the macro can simply return NULL.  The
** macro may also resolve to an actual function call,
** in which case the function must match the following
** prototype:
**
** FUNC_EXTERN MU_CL_SEM_FUNCS *MUcl_init( VOID );
*/

# include <mucl.h>


/*}
** Name:	MU_SEMAPHORE	- user semaphore
**
** Description:
**	This is the visible MU_SEMAPHORE structure.  Users 
**	peek inside this at their own peril.
**
** History:
**	30-sep-92 (daveb)
**	    recreated.
*/
struct II_MU_SEMAPHORE
{
    i4			mu_state;

    /* other types may be added, but must be documented
       in the specification . */

#   define		MU_UNKNOWN	0
#   define		MU_REMOVED	1
#   define		MU_ACTIVE	2
#   define		MU_CONVERTING	3

    i4			mu_value;	/* 0 when not held, 1 when held */
    char		mu_name[ MU_SEM_NAME_LEN ];
    MU_SEMAPHORE_FUNCS	*mu_active;	/* Provider functions */
    PTR			mu_cs_sem;	/* CS semaphore */
    MU_CL_SEMAPHORE	mu_cl_sem;	/* CL semaphore */

};


/* user functions */

FUNC_EXTERN STATUS	MUi_semaphore( MU_SEMAPHORE *sem );
FUNC_EXTERN void	MUn_semaphore( MU_SEMAPHORE *sem, char *name );
FUNC_EXTERN STATUS	MUw_semaphore( MU_SEMAPHORE *sem, char *name );
FUNC_EXTERN STATUS	MUr_semaphore( MU_SEMAPHORE *sem );
FUNC_EXTERN STATUS	MUp_semaphore( MU_SEMAPHORE *sem );
FUNC_EXTERN STATUS	MUv_semaphore( MU_SEMAPHORE *sem );
FUNC_EXTERN MU_SID	MUget_sid(void);

/* provider functions */

FUNC_EXTERN MU_SEMAPHORE_FUNCS *MUset_funcs( MU_SEMAPHORE_FUNCS *funcs );

/* provided services, for providers */

FUNC_EXTERN MU_SEMAPHORE_FUNCS *MUcs_sems(void);
FUNC_EXTERN MU_SEMAPHORE_FUNCS *MUdefault_sems(void);

# endif /* MU_HDR_INCLUDED */
