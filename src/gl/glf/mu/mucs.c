/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <me.h>
# include <mu.h>
# include <pc.h>
# include <cs.h>

/**
** Name:	mucs.c	- CS interface for MU semaphores.
**
** Description:
**
**	This file defines the things that let MU_SEMAPHORES
**	work in a threaded CS environment.  It defines a number
**	of static functions that interpret the MU_SEMs, turning
**	them into appropriate CS_xsem calls.  It packages them
**	up into the GLOBALDEF-ed MU_cs_funcs array so they may
**	be seen by CSdispatch.  It's CSdispatches job to call
**
**		MUset_funcs( MUCS_Sems() );
**
**	at the right time.
**
**	Defines the following public interfaces:
**
**		MU_SEMAPHORE_FUNCS MU_cs_funcs(void);
**
**	and the following private functions:
**
**		MU_cs_isem	- init CS sem for MU
**		MU_cs_nsem	- name a CS sem for MU
**		MU_cs_rsem	- remove a CS sem for MU.
**		MU_cs_psem	- lock a CS sem for MU
**		MU_cs_vsem	- unlock CS sem for MU.
**		MU_cs_sid	- return "Self" ID.
**
** History:
**	1-oct-92 (daveb)
**	    created.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  
**	02-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
**	21-jan-1999 (canor01)
**	    Remove erglf.h and generated-file dependencies.
**	28-apr-1999 (hanch04)
**	    Replaced STlcopy with STncpy
**	13-jun-2002 (devjo01)
**	    Added MU_cs_sid for b108015.
**/

static MU_I_SEM_FUNC MU_cs_isem;
static MU_N_SEM_FUNC MU_cs_nsem;
static MU_R_SEM_FUNC MU_cs_rsem;
static MU_P_SEM_FUNC MU_cs_psem;
static MU_V_SEM_FUNC MU_cs_vsem;
static MU_SID_FUNC   MU_cs_sid;

static MU_SEMAPHORE_FUNCS MU_cs_funcs =
{
    MU_cs_isem,
    MU_cs_nsem,
    MU_cs_rsem,
    MU_cs_psem,
    MU_cs_vsem,
    MU_cs_sid
};

MU_SEMAPHORE_FUNCS *
MUcs_sems(void)
{
    return( &MU_cs_funcs );
}



/*{
** Name:	MU_cs_isem	- init CS sem for MU
**
** Description:
**	Initialize the semaphore.  If it is not UNKNOWN or
**	REMOVED, it's an active semaphore, so complain.
**	It's not legal to re-init an active semaphore without
**      removing it first.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		semaphore to initialize.
**
** Outputs:
**	sem		written.
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.
**	    Allocate it here with MEreqmem.
**	02-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
*/
static STATUS
MU_cs_isem( MU_SEMAPHORE *sem )
{
    STATUS		cl_stat;

    if( sem->mu_state == MU_ACTIVE )
    {
	cl_stat = MUCS_SEM_INITIALIZED;
    }
    else 
    {
	sem->mu_cs_sem = MEreqmem( 0, sizeof( CS_SEMAPHORE ), 0, &cl_stat );
	if( sem->mu_cs_sem != NULL )
	{
	    if (sem->mu_state == MU_CONVERTING)
	    {
	        cl_stat = CSw_semaphore( (CS_SEMAPHORE *)sem->mu_cs_sem,
				        CS_SEM_SINGLE, sem->mu_name );
	    }
	    else
	    {
	        sem->mu_name[0] = EOS;
	        cl_stat = CSw_semaphore( (CS_SEMAPHORE *)sem->mu_cs_sem,
					CS_SEM_SINGLE, "MU sem" );
	    }
	    sem->mu_state = MU_ACTIVE;
	    sem->mu_active = &MU_cs_funcs;
	}
    }
    return( cl_stat );
}


/*{
** Name:	MU_cs_nsem	- name a CS sem for MU
**
** Description:
**	Give the semaphore a name.  If this is an unpromoted
**	DEFAULT semaphore, copy the name into the mu_name
**	field.  If it's a CS_SEM now, just call CS_nsemaphore.
**
**	The strategy here is to defer promoting the semaphore
**	until it is actually used with a P or V operation.
**
**	Name will be truncated to CS_SEM_NAME_LEN (24) characters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		semaphore to name.
**
**	name		name to give.
**
** Outputs:
**	
**
** Returns:
**	<function return values>
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
*/
static void
MU_cs_nsem( MU_SEMAPHORE *sem, char *name )
{
    if( sem->mu_state == MU_ACTIVE )
    {
	STncpy( sem->mu_name, name, sizeof( sem->mu_name ) - 1 );
	sem->mu_name[sizeof( sem->mu_name ) - 1 ] = '\0';
	if( sem->mu_active == &MU_cs_funcs )
	    (void) CSn_semaphore( (CS_SEMAPHORE *)sem->mu_cs_sem, name );
    }
}



/*{
** Name:	MU_cs_rsem	- remove a CS sem for MU.
**
** Description:
**	Remove a semaphore.  If it's not a CS_SEM, complain.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		to delete.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if semaphore is deleted.
**	other		reportable error status.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.  Free
**	    it here with MEfree.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
*/
static STATUS
MU_cs_rsem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if( sem->mu_state != MU_ACTIVE || sem->mu_active != &MU_cs_funcs )
    {
	cl_stat = MUCS_R_SEM_BAD_STATE;
    }
    else
    {
	cl_stat = CSr_semaphore( (CS_SEMAPHORE *)sem->mu_cs_sem );
	if( cl_stat == OK )
	{
	    MEfree( sem->mu_cs_sem );
	    sem->mu_state = MU_REMOVED;
	    sem->mu_active = NULL;
	    sem->mu_name[0] = EOS;
	}
    }
    return( cl_stat );
}


/*{
** Name:	MU_cs_psem	- lock a CS sem for MU
**
** Description:
**	Try to lock a semaphore.  If it's a DEFAULT sem, now
**	is the time to promote it; init it and copy it's name.
**	It's an error to be holding a DEFAULT semaphore and
**	be converting it now.  If it's not a DEFAULT or CS sem, complain.
**
**	May return any CS_psemaphore error, esp. INTERRUPTED,
**	in which case the semaphore is not held.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		sem to lock.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if semaphore is locked.
**	any CS_psemaphore value
**	other		reportable error status.
**
** History:
**	1-Oct-92 (daveb)
**	    created.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
*/
static STATUS
MU_cs_psem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat = OK;

    if( sem->mu_state == MU_CONVERTING )
    {
	if( sem->mu_value != 0 )
	    cl_stat = MUDEF_P_DEADLOCK;
	else
	    sem->mu_value = 1;
    }
    else if ( sem->mu_state != MU_ACTIVE )
    {
	cl_stat = MUCS_P_SEM_BAD_STATE;
    }
    else if( sem->mu_active == MUdefault_sems() )
    {
	if( sem->mu_value != 0 )
	{
	    cl_stat = MUCS_SEM_OVER_CHANGE;
	}
	else
	{
	    /* handle recursion while converting this one */
	    sem->mu_state = MU_CONVERTING;
	    cl_stat = MU_cs_isem( sem );
	}
    }

    if( cl_stat == OK )
    {
	if ( sem->mu_active != &MU_cs_funcs )
	{
	    cl_stat = MUCS_P_SEM_BAD_STATE;
	}
	else
	{
	    cl_stat = CSp_semaphore( TRUE, (CS_SEMAPHORE *)sem->mu_cs_sem);
	    if( cl_stat == OK )
		sem->mu_value = 1;
	}
    }

    return( cl_stat );
}


/*{
** Name:	MU_cs_vsem	- unlock CS sem for MU.
**
** Description:
**	Release the semaphore.  It's an error for this not
**	to be a CS semaphore that is properly held.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		sem to unlock.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		if the sempahore was unlocked.
**	other		reportable error status.
**
** History:
**	1-Oct-92 (daveb)
**	    created, fixed _psem call.
**	12-oct-92 (daveb)
**	    mu_sem is now a PTR to a CS_SEM, not one directly.
**	14-Nov-96 (gordy)
**	    Renamed mu_sem to mu_cs_sem and added mu_cl_sem.
*/
static STATUS
MU_cs_vsem( MU_SEMAPHORE *sem )
{
    STATUS cl_stat;
    
    if( sem->mu_state != MU_ACTIVE && sem->mu_state != MU_CONVERTING
       || sem->mu_value != 1 )
    {
	cl_stat = MUCS_V_SEM_BAD_STATE;
    }
    else if( sem->mu_active == MUdefault_sems() )
    {
	if( sem->mu_state == MU_CONVERTING )
	    sem->mu_value = 0;
	else
	    cl_stat = MUCS_SEM_OVER_CHANGE;
    }
    else if ( sem->mu_active != &MU_cs_funcs )
    {
	cl_stat = MUCS_V_SEM_BAD_STATE;
    }
    else
    {
	sem->mu_value = 0;
	cl_stat = CSv_semaphore( (CS_SEMAPHORE *)sem->mu_cs_sem);
	if( cl_stat != OK )
	    sem->mu_value = 1;
    }
    return( cl_stat );
}


/*{
** Name:	MU_cs_sid	- Return self ID.
**
** Description:
**	Return a unique persistent identifier for calling
**	thread which can be used for equality comparisons
**	with other MU_SID variables ONLY!.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	MU_SID
**
** History:
**	13-jun-2002 (devjo01)
**	    created.
*/
static MU_SID
MU_cs_sid( void )
{
    CS_SID	sid;

    CSget_sid(&sid);
    return (MU_SID)sid;
}

/* end of mucs.c */
