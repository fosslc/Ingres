/*
** Copyright (c) 1988, 2002 Ingres Corporation
**
**
*/

/*
**  Name: MUTEX.C - generic initialization, P and V operations on
**		    semaphores of the type CS_SEMAPHORE.
**
**  Description:  The generic operations are designed to accept and use
**		  user specified routines to initialize, P and V the
**		  semaphores.  The dummy functions provided will function
**		  correctly in a non-shared, non-threaded environment.
**		  No action need be take by the user to initialize the
**		  semaphore functions.  If the user is running in a
**		  more dynamic environment then the functions to be used
**		  to initialize, P and V the semaphores should be passed
**		  into gen_seminit() as parameters.  These functions will
**		  then be used for all subsequent semaphore operations.
**
**		  The following functions are included herein:
**
**			gen_Pdummy( sem ) - dummy P function.
**			gen_Vdummy( sem ) - dummy V function.
**			gen_Idummy( )	  - dummy initialization function.
**			gen_Ndummy( )	  - dummy initialization function.
**			gen_seminit( psem, vsem, isem, nsem ) - initialize
**				and name the semaphore functions.
**			gen_Psem( &sem )  - P a semaphore.
**			gen_Vsem( &sem )  - V a semaphore.
**			gen_Isem( &sem )  - initialize a semaphore.
**			gen_Nsem( &sem, name )  - name a semaphore.
**
**  History:
**	26-aug-89 (rexl)
**	    written.
**	22-Nov-89 (anton)
**	    added simple semaphore test routines
**	22-feb-90 (fls-sequent)
**	    Changed gen_useCSsems to take the CS semaphore routines as
**	    parameters instead of hard coding them.  This fixes problems
**	    with undefined references in libingres.  Also because MT_mutexes
**	    are no longer allocated dynamically, changed gen_useCSsems to 
**	    no longer require the stream allocator to be initialized before
**	    activating the semaphore system.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	22-may-1995 (canor01)
**	    initialize ME_page_sem
**      09-jun-1995 (lawst01)
**          initailize PM_misc_sem
**	24-jul-95 (canor01)
**	    NT uses malloc and free so no need for ME_stream_sem.
**	08-aug-1995 (canor01)
**	    NT does need protection for its global queues (ME_tag_sem)
**	22-aug-1995 (canor01)
**	    NT: Add protection for shared memory qlobal queue (ME_segpool_sem)
**	    and remove the higher level ME_page_sem.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	10-feb-1996 (canor01)
**	    restore name of TR_context_sem for Windows NT
**	23-sep-1996 (canor01)
**	    Move global data definitions to hdydata.c.
**	10-apr-1997 (canor01)
**	    Correct function declaration for gen_useCSsems().
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**	12-Jun-98 (rajus01)
**	    Initialize GC_associd_sem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-jun-2002 (somsa01)
**	    For Windows, removed usage of ME_tag_sem.
**	19-jan-2005 (devjo01)
**	    Remove redundant init of ME_page_sem in gen_useCSsems.
**	    Retain rename to minimize behavior change.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototypes.
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <st.h>
#include   <cs.h>
# ifdef NT_GENERIC
#include   <io.h>
# endif /* NT_GENERIC */


/* TABLE OF CONTENTS */
STATUS gen_Pdummy(
	i4 exclusive,
	CS_SEMAPHORE *sem);
STATUS gen_Vdummy(
	CS_SEMAPHORE *sem);
STATUS gen_Idummy(
	CS_SEMAPHORE *sem,
	i4 type);
VOID    gen_Ndummy(
	CS_SEMAPHORE *sem,
	char *name);
static void genstr(
	char *s);
STATUS gen_Ptest(
	i4 exclusive,
	CS_SEMAPHORE *sem);
STATUS gen_Vtest(
	CS_SEMAPHORE *sem);
STATUS gen_Itest(
	CS_SEMAPHORE *sem,
	i4 type);
VOID gen_seminit(
	STATUS (*psem)(i4 excl, CS_SEMAPHORE *sem),
	STATUS (*vsem)(CS_SEMAPHORE *sem),
	STATUS (*isem)(CS_SEMAPHORE *sem, i4 type),
	VOID   (*nsem)(CS_SEMAPHORE *sem, char *string));
STATUS iigen_Psem(
	CS_SEMAPHORE *sem);
STATUS iigen_Vsem(
	CS_SEMAPHORE *sem);
STATUS iigen_Isem(
	CS_SEMAPHORE *sem);
STATUS iigen_Nsem(
	CS_SEMAPHORE *sem,
	char *name);
VOID iigen_useCSsems(
	STATUS (*psem)(i4 excl, CS_SEMAPHORE *sem),
	STATUS (*vsem)(CS_SEMAPHORE *sem),
	STATUS (*isem)(CS_SEMAPHORE *sem, i4 type),
	VOID   (*nsem)(CS_SEMAPHORE *sem, char *string));

static	STATUS	(*genP_semaphore)() = gen_Pdummy;
static	STATUS	(*genV_semaphore)() = gen_Vdummy;
static	STATUS	(*genI_semaphore)() = gen_Idummy;
static	VOID	(*genN_semaphore)() = gen_Ndummy;

GLOBALREF	CS_SEMAPHORE	 CL_misc_sem;
GLOBALREF	CS_SEMAPHORE	 CL_acct_sem;
GLOBALREF	CS_SEMAPHORE	 FP_misc_sem;
GLOBALREF	CS_SEMAPHORE	 GC_associd_sem;
GLOBALREF	CS_SEMAPHORE	 GC_misc_sem;
GLOBALREF	CS_SEMAPHORE	 GC_trace_sem;
GLOBALREF	CS_SEMAPHORE	 TR_context_sem;
GLOBALREF	CS_SEMAPHORE	 NM_loc_sem;
GLOBALREF	CS_SEMAPHORE	 GCC_ccb_sem;
GLOBALREF	CS_SEMAPHORE	 LGK_misc_sem;
GLOBALREF	CS_SEMAPHORE	 ME_stream_sem;
GLOBALREF	CS_SEMAPHORE	 ME_segpool_sem;
GLOBALREF	CS_SEMAPHORE	 *ME_page_sem;
GLOBALREF	CS_SEMAPHORE	 DI_sc_sem;
GLOBALREF       CS_SEMAPHORE     PM_misc_sem;
GLOBALREF	CS_SEMAPHORE	 DMF_misc_sem;
GLOBALREF	CS_SEMAPHORE	 SCF_misc_sem;

STATUS
gen_Pdummy(exclusive, sem)
i4	exclusive;
CS_SEMAPHORE	*sem;
{
	return OK;
}

STATUS
gen_Vdummy(sem)
CS_SEMAPHORE	*sem;
{
	return OK;
}

STATUS
gen_Idummy(sem, type)
CS_SEMAPHORE	*sem;
i4		type;
{
	return	OK;
}

VOID
gen_Ndummy(sem, name)
CS_SEMAPHORE	*sem;
char		*name;
{
}

static VOID
genstr(s)
char	*s;
{
    i4	l;

    l = STlength(s);
    write(1, s, l);
}

STATUS
gen_Ptest(exclusive, sem)
i4	exclusive;
CS_SEMAPHORE	*sem;
{
    if (sem->cs_value == 0)
    {
	genstr("init on the fly\n");
	sem->cs_value = 652;
	sem->cs_count = 0;
    }
    if (sem->cs_value != 652)
    {
	genstr("sem corrupt\n");
	return FAIL;
    }
    if (sem->cs_count)
    {
	genstr("dup P\n");
	return FAIL;
    }
    sem->cs_count = 1;
    return OK;
}

STATUS
gen_Vtest(sem)
CS_SEMAPHORE	*sem;
{
    if (sem->cs_value != 652)
    {
	genstr("not init\n");
    }
    if (sem->cs_count == 0)
    {
	genstr("no P\n");
	return FAIL;
    }
    sem->cs_count = 0;
    return OK;
}

STATUS
gen_Itest(sem, type)
CS_SEMAPHORE	*sem;
i4		type;
{
    if (sem->cs_value == 652)
    {
	genstr("reinit\n");
    }
    sem->cs_value = 652;
    sem->cs_count = 0;
    return OK;
}

/*{
** Name: gen_seminit
**
** Description: Initialize the semaphore functions.
**
** Inputs:
**	STATUS	(*psem) ();	pointer to P function or NULL.
**	STATUS	(*vsem) ();	pointer to V function or NULL.
**	STATUS	(*isem) ();	pointer to init function or NULL.
**
** Outputs:
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	genP_semaphore, genV_semaphore and genI_semaphore get set.
**
** History:
**	26-aug-89 (rexl)
**	    created.
*/
VOID
gen_seminit(psem, vsem, isem, nsem)
STATUS	(*psem)(i4 excl, CS_SEMAPHORE *sem);
STATUS	(*vsem)(CS_SEMAPHORE *sem);
STATUS	(*isem)(CS_SEMAPHORE *sem, i4 type);
VOID	(*nsem)(CS_SEMAPHORE *sem, char *string);
{

	genP_semaphore = (psem) ? psem : gen_Pdummy;
	genV_semaphore = (vsem) ? vsem : gen_Vdummy;
	genI_semaphore = (isem) ? isem : gen_Idummy;
	genN_semaphore = (nsem) ? nsem : gen_Ndummy;
}

/*{
** Name: gen_Psem
**
** Description:  P a CS_SEMAPHORE using a user specified function.
**
** Inputs:
**	CS_SEMAPHORE	*sem;		pointer to a semaphore.
**
** Outputs:
**	Returns:
**	    FAIL	NULL pointer.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	26-aug-89 (rexl)
**	created.
*/
STATUS
gen_Psem(sem)
CS_SEMAPHORE	*sem;
{
	STATUS	status	= OK;

	if( !sem )
	{
	    status = FAIL;		/* pointer not set	*/
	}
	else if( sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
	    status = OK;
	}
	else if( status = (*genP_semaphore) (TRUE, sem) )
	{
	    /*
	    ***
	    TRdisplay( "Semaphore failure, server terminating\n" );
	    abort();
	    ***
	    */
	}

	return( status );
}


/*{
** Name: gen_Vsem
**
** Description: V a CS_SEMAPHORE using a user specified function.
**
** Inputs:
**	CS_SEMAPHORE	*sem;		pointer to a semaphore.
**
** Outputs:
**	Returns:
**	    FAIL	NULL pointer.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	26-aug-89 (rexl)
**	created.
*/
STATUS
gen_Vsem(sem)
CS_SEMAPHORE	*sem;
{
	STATUS	status	= OK;

	if( !sem )
	{
	    status = FAIL;
	}
	else if( sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
	    status = OK;
	}
	else if ((status = (*genV_semaphore) (sem)) != OK)
	{
	    /*
	    ***
	    TRdisplay( "Semaphore failure, server terminating\n");
	    abort();
	    ***
	    */
	}

	return( status );
}


/*{
** Name: gen_Isem
**
** Description:  Initialize a CS_SEMAPHORE using user specified function.
**
** Inputs:
**	CS_SEMAPHORE	*sem;		pointer to a semaphore.
**
** Outputs:
**	Returns:
**	    FAIL	NULL pointer.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	26-aug-89 (rexl)
**	created.
*/
STATUS
gen_Isem(sem)
CS_SEMAPHORE	*sem;
{
	STATUS	status = OK;

	if( !sem )
	{
	    status = FAIL;
	}
	else
	{
	    status = (*genI_semaphore) (sem, CS_SEM_SINGLE);
	}

	return( status );
}

/*{
** Name: gen_Nsem
**
** Description:  Name a CS_SEMAPHORE using user specified function.
**
** Inputs:
**	CS_SEMAPHORE	*sem;		pointer to a semaphore.
**	char		*name;		pointer to a name.
**
** Outputs:
**	Returns:
**	    FAIL	NULL pointer.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	13-jun-1995 (canor01)
**	created.
*/
STATUS
gen_Nsem(sem, name)
CS_SEMAPHORE	*sem;
char		*name;
{
	STATUS	status = OK;

	if( !sem )
	{
	    status = FAIL;
	}
	else if( sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
	    status = OK;
	}
	else
	{
	    (*genN_semaphore) (sem, name);
	    status = OK;
	}

	return( status );
}


VOID
gen_useCSsems(
            STATUS (*psem)(i4 excl, CS_SEMAPHORE *sem),
            STATUS (*vsem)(CS_SEMAPHORE *sem),
            STATUS (*isem)(CS_SEMAPHORE *sem, i4  type),
            VOID   (*nsem)(CS_SEMAPHORE *sem, char *string)  )
{
    gen_seminit( psem, vsem, isem, nsem );
    gen_Isem( &CL_misc_sem );
    gen_Isem( &CL_acct_sem );
    gen_Isem( &FP_misc_sem );
    gen_Isem( &TR_context_sem );
    gen_Isem( &NM_loc_sem );
    gen_Isem( &GC_associd_sem );
    gen_Isem( &GC_misc_sem );
# ifndef NT_GENERIC
    gen_Isem( &GC_trace_sem );
# endif /* NT_GENERIC */
    gen_Isem( &GCC_ccb_sem  );
    gen_Isem( &LGK_misc_sem );
# ifndef NT_GENERIC
    gen_Isem( &ME_stream_sem );
# else  /* NT_GENERIC */
    gen_Isem( &ME_segpool_sem );
# endif /* NT_GENERIC */
    gen_Isem( &DI_sc_sem );
    gen_Isem( &PM_misc_sem );
    gen_Isem( &DMF_misc_sem );
    gen_Isem( &SCF_misc_sem );
    gen_Nsem( &CL_misc_sem, "CL misc sem" );
    gen_Nsem( &CL_acct_sem, "CL acct sem" );
    gen_Nsem( &FP_misc_sem, "FP misc sem" );
    gen_Nsem( &TR_context_sem, "TR context sem" );
    gen_Nsem( &SCF_misc_sem, "SCF misc sem" );
    gen_Nsem( &GC_associd_sem, "GC associd sem" );
    gen_Nsem( &GC_misc_sem, "GC misc sem" );
# ifndef NT_GENERIC
    gen_Nsem( &GC_trace_sem, "GC trace sem" );
# endif /* NT_GENERIC */
    gen_Nsem( &GCC_ccb_sem, "GCC ccb sem" );
    gen_Nsem( &LGK_misc_sem, "LGK misc sem" );
# ifndef NT_GENERIC
    gen_Nsem( &ME_stream_sem, "ME stream sem" );
    gen_Nsem( ME_page_sem, "ME page sem" );
# else  /* NT_GENERIC */
    gen_Nsem( &ME_segpool_sem, "ME segpool sem" );
# endif /* NT_GENERIC */
    gen_Nsem( &DI_sc_sem, "DI slave sem" );
    gen_Nsem( &PM_misc_sem, "PM misc sem" );
    gen_Nsem( &DMF_misc_sem, "DMF misc sem" );
    gen_Nsem( &NM_loc_sem, "NMloc sem" );

}
