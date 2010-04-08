/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <mu.h>

static MU_SEMAPHORE_FUNCS *MU_functions ZERO_FILL;

/*{
** Name:	MUset_funcs	- install new guts for MU_SEMAPHORES
**
** Description:
**
**	Install the functions contained in the argument structure
**	as the guts of the MU_SEMAPHORE system.  You only get to
**	do this once, to replace the default set.  At present, the
**	alternative is to work with CS semaphores.
**
** Re-entrancy:
**	No.  This is changing the global state of stuff critical
**	to mutual exclusion, and will need close examination on
**	truly threaded systems.
**
**	In truly threaded systems, it is important that this function
**	only be called during single-threaded startup, before
**	multi-threading actually begins.
**
** Inputs:
**	sem_funcs	pointer to a structure defining the new
**			functions to use, typically MU_cs_funcs.
**
** Outputs:
**	none
**
** Returns:
**	OK		if the change took effect.
**	other		reportable status if it couldn't happen.
**
** History:
**	1-oct-92 (daveb)
**	    created
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-Jul-1993 (daveb)
**	    Rewrite MUi_semaphore to make HP high-levels op optimizaiton
**	    happier.  It SEGVs otherwise.
**	03-jun-1996 (canor01)
**	    Add MUw_semaphore to correspond with similar CS functions.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	13-jun-2002 (devjo01)
**	    Add MUget_sid for b108015.
*/

MU_SEMAPHORE_FUNCS *
MUset_funcs( MU_SEMAPHORE_FUNCS *sem_funcs )
{
    MU_SEMAPHORE_FUNCS *old = MU_functions;

    MU_functions = sem_funcs;
    return( old );
}


STATUS
MUi_semaphore( MU_SEMAPHORE *sem )
{
	STATUS rv;

	if( MU_functions != NULL )
		rv = (*MU_functions->mu_isem)( sem );
	else
		rv = (*MUdefault_sems()->mu_isem)( sem );
	return( rv );		
}


void
MUn_semaphore( MU_SEMAPHORE *sem, char *name )
{
    if( MU_functions != NULL )
	(*MU_functions->mu_nsem)( sem, name );
    else
	(*MUdefault_sems()->mu_nsem)( sem, name );
}

STATUS
MUw_semaphore( MU_SEMAPHORE *sem, char *name )
{
    STATUS rv;

    if( MU_functions != NULL )
    {
	rv = (*MU_functions->mu_isem)( sem );
	if ( rv == OK )
	    (*MU_functions->mu_nsem)( sem, name );
    }
    else
    {
	rv = (*MUdefault_sems()->mu_isem)( sem );
	if ( rv == OK )
	    (*MUdefault_sems()->mu_nsem)( sem, name );
    }
    return ( rv );    
}

STATUS
MUr_semaphore( MU_SEMAPHORE *sem )
{
    return( MU_functions ?
	   (*MU_functions->mu_rsem)( sem ) :
	   (*MUdefault_sems()->mu_rsem)( sem ) );
}


STATUS
MUp_semaphore( MU_SEMAPHORE *sem )
{
    return( MU_functions ?
	   (*MU_functions->mu_psem)( sem ) :
	   (*MUdefault_sems()->mu_psem)( sem ) );
}


STATUS
MUv_semaphore( MU_SEMAPHORE *sem )
{
    return( MU_functions ?
	   (*MU_functions->mu_vsem)( sem ) :
	   (*MUdefault_sems()->mu_vsem)( sem ) );
	   
}

MU_SID
MUget_sid(void)
{
    return( MU_functions ?
	   (*MU_functions->mu_sid)( ) :
	   (*MUdefault_sems()->mu_sid)( ) );
}
