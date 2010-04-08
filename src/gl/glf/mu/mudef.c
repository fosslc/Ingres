/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <mu.h>

/**
** Name:	mudef.c	- default semaphores for MU.
**
** Description:
**
**    This is the callable client-interface for MU semaphores,
**    described in <mu.h>.
**
**    This file defines:
**
**    public:
**		MUdefault_sems	- locate the default actions.
**
**    private:
**		MU_d_isem	- default init sem
**		MU_d_nsem	- name default semaphores
**		MU_d_rsem	- default sem remove fucntion
**		MU_d_psem	- lock default semaphore.
**		MU_d_vsem	- release a defulat semaphore
**		MU_d_sid 	- return "self ID" of caller.
**
** History:
**	10-oct-92 (daveb)
**	    Created.
**	12-oct-92 (daveb)
**	    mu_i_sem does not have a type arg anymore.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
**	30-Nov-00 (loera01) Bug 103318 NT_GENERIC only
**          In MU_d_psem(), added vsem operation if E_GL5005_MUDEF_P_DEADLOCK
**          error is encountered. 
**          
**	21-jan-1999 (canor01)
**	    Remove erglf.h and generated-file dependencies.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	13-jun-2002 (devjo01)
**	    Add SID stuff for b108015.
**/

/* 
** Forward references 
*/

static MU_I_SEM_FUNC MU_d_isem;
static MU_N_SEM_FUNC MU_d_nsem;
static MU_R_SEM_FUNC MU_d_rsem;
static MU_P_SEM_FUNC MU_d_psem;
static MU_V_SEM_FUNC MU_d_vsem;
static MU_SID_FUNC   MU_d_sid;

/* 
** Default function descriptions 
*/

static MU_SEMAPHORE_FUNCS MU_default_funcs =
{
    MU_d_isem,
    MU_d_nsem,
    MU_d_rsem,
    MU_d_psem,
    MU_d_vsem,
    MU_d_sid
};

/*
** CL private interface.
*/

static MU_CL_SEM_FUNCS	*MU_cl_funcs = NULL;
static bool		initialized = FALSE;



/*
** Name: MUdefault_sems
**
** Description:
**	Returns structure containing the default semaphore functions.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	MU_SEMAPHORE_FUNCS *	Default semaphore functions.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
*/

MU_SEMAPHORE_FUNCS *
MUdefault_sems(void)
{
    return( &MU_default_funcs );
}


/*{
** Name:	MU_d_isem	- default sem init function
**
** Description:
**	Initialize a default semaphore.  Return error if it
**	appears to be initialized or isn't a default semaphore.
**
** Inputs:
**	sem		sem to initialize.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if sem was initialize.
**	other		reportable status if something prevented
**			initialization.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

static STATUS
MU_d_isem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if ( ! initialized )
    {
	/*
	** Check to see if CL is providing
	** multi-threading support.
	*/
	MU_cl_funcs = MU_CL_SEM_INIT();
	initialized = TRUE;
    }

    if ( sem->mu_state == MU_ACTIVE )
    {
	cl_stat = MUDEF_SEM_INITIALIZED;
    }
    else
    {
	sem->mu_state = MU_ACTIVE;
	sem->mu_active = &MU_default_funcs;
	sem->mu_name[0] = EOS;

	if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_isem )  
	    cl_stat = (*MU_cl_funcs->mucl_isem)( &sem->mu_cl_sem );
    }

    return( cl_stat );
}


/*{
** Name:	MU_d_nsem	- name default semaphores
**
** Description:
**	Give a name to a non-threaded default semaphore.
**	Does nothing if called on a semaphore that is not
**	known to be in default state.  The name string will
**	be truncated to CS_SEM_NAME_LEN (24).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		semaphore to name.
**
**	name		string to ascribe.
**
** Outputs:
**	sem		written.
**
** Returns:
**	none.	
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

static void
MU_d_nsem( MU_SEMAPHORE *sem, char *name )
{
    if ( sem->mu_state == MU_ACTIVE )
    {
	STncpy( sem->mu_name, name, sizeof( sem->mu_name ) - 1 );
	sem->mu_name[ sizeof( sem->mu_name ) - 1 ] = EOS;
	if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_nsem )
	    (*MU_cl_funcs->mucl_nsem)( &sem->mu_cl_sem, name );
    }

    return;
}


/*{
** Name:	MU_d_rsem	- default sem remove fucntion
**
** Description:
**	Mark a default semaphore as removed.  Return error if
**	it appears to be in use or isn't a default semaphore.
**
** Inputs:
**	sem		sem to remove.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if sem was removed.
**	other		reportable status if something prevented
**			removal.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

static STATUS
MU_d_rsem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if ( sem->mu_state != MU_ACTIVE || sem->mu_active != &MU_default_funcs )
    {
	cl_stat = MUDEF_REM_NON_DEF;
    }
    else
    {
	sem->mu_state = MU_REMOVED;
	sem->mu_value = 0;
	sem->mu_name[0] = EOS;
	sem->mu_active = NULL;

	if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_rsem )  
	    cl_stat = (*MU_cl_funcs->mucl_rsem)( &sem->mu_cl_sem );
    }

    return( cl_stat );
}


/*{
** Name:	MU_d_psem	- lock default semaphore.
**
** Description:
**	Get a thread-lock in a non-threaded process (a no-op).
**	Makes sure you don't already have the semaphore locked,
**	and that it's an initialised default semaphore.
**
** Inputs:
**	sem		sem to lock.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if the lock was obtained.
**	other		reportable error status.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
**	30-Nov-00 (loera01) Bug 103318 NT_GENERIC only
**          Perform vsem operation if E_GL5005_MUDEF_P_DEADLOCK error is 
**          encountered. 
*/

static STATUS
MU_d_psem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if ( sem->mu_state != MU_ACTIVE || sem->mu_active != &MU_default_funcs )
	cl_stat = MUDEF_P_NON_DEF;
    else
    {
	if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_psem )  
	    cl_stat = (*MU_cl_funcs->mucl_psem)( &sem->mu_cl_sem );

	if ( cl_stat == OK )
	{
	    if ( sem->mu_value != 0 )
            {
                cl_stat = MUDEF_P_DEADLOCK;
#ifdef NT_GENERIC
                if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_vsem )
                {
                /*
                **  For NT and 32-bit Windows: the p_sem has already been 
                **  locked, but the p_sem routine returns OK anyway because we
                **  are grabbing the mutex recursively (more than once from
                **  the same thread).  We therefore execute a v_sem operation
                **  to release the mutex in a last-ditch attempt to make 
                **  the CL layer match the GL layer.  Although the GL layer 
                **  acts as though only one owner of a semaphore is possible, 
                **  Windows/NT mutexes allow recursive ownership, and require 
                **  a v_sem operation for each p_sem previously executed.  
                */
                    (*MU_cl_funcs->mucl_vsem)( &sem->mu_cl_sem );
                }
#endif
            }
	    else
		sem->mu_value = 1;
	}
    }

    return( cl_stat );
}


/*{
** Name:	MU_d_vsem	- release a default semaphore
**
** Description:
**	Unlock a previously locked semaphore.  Returns
**	error if sem was not properly held.
**
** Inputs:
**	sem		held semaphore to unlock.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if the semphore was unlocked.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	14-Nov-96 (gordy)
**	    Added CL multi-threaded application support.
*/

static STATUS
MU_d_vsem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if ( sem->mu_state != MU_ACTIVE || sem->mu_active != &MU_default_funcs) 
	cl_stat = MUDEF_V_NON_DEF;
    else if( sem->mu_value != 1 )
	cl_stat = MUDEF_V_BAD_VAL;
    else
    {
	sem->mu_value = 0;

	if ( MU_cl_funcs  &&  MU_cl_funcs->mucl_vsem )  
	{
	    cl_stat = (*MU_cl_funcs->mucl_vsem)( &sem->mu_cl_sem );
	    if ( cl_stat != OK )  sem->mu_value = 1;
	}
    }

    return( cl_stat );
}


/*{
** Name:	MU_d_sid	- return a unique self ID.
**
** Description:
**	Return a unique persistent identifier for current thread.
**	This ID can be used for comparison for equality ONLY!
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	MU_SID for caller.
**
** History:
**	13-jun-2002 (devjo01)
**	    created.
*/

static MU_SID
MU_d_sid( void )
{
    MU_SID sid;

    if ( MU_cl_funcs && MU_cl_funcs->mucl_sid )  
    {
	sid = (*MU_cl_funcs->mucl_sid)( );
    }
    else
    {
	sid = (MU_SID)1;
    }

    return( sid );
}

