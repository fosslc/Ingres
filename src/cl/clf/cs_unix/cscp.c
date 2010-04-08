/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <cs.h>
#include    <clipc.h>
#include    <fdset.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <sys/signal.h>
#include    <errno.h>

/*
NO_OPTIM = hp3_us5
*/

/**
**
**  Name: CSCP.C - Cross UNIX process semaphores
**
**  Description:
**	The file defines routines to handle semaphores that work across
**	UNIX processes.  Note that if these semaphores are used by a
**	UNIX process that is multi-threading such as the dbms server
**	then that whole process may block.
**
**  Functions Defined:
**	CScp_init	- initialize a cross process semaphore
**	CScp_Psem	- P a cross process semaphore
**	CScp_Vsem	- V a cross process semaphore
**	CS_sleep	- Sleep a UNIX process on a semaphore
**	CS_wakeup	- Wakeup a UNIX process from CS_sleep
**
**  History:
 * Revision 1.4  89/01/05  15:14:57  jeff
 * moved header file
 * 
 * Revision 1.3  88/10/17  15:17:51  jeff
 * fixed union argument passing
 * 
 * Revision 1.2  88/08/24  11:29:22  jeff
 * cs up to 61 needs
 * 
 * Revision 1.6  88/04/19  15:31:34  anton
 * new and fixed cross process semaphores
 * 
 * Revision 1.2  88/03/21  12:22:30  anton
 * Event system appears solid
**
**      01-mar-88 (anton)
**	    Created.
**	22-Feb-1989 (fredv)
**	    Integrated changes daveb made to beta1 code into beta3 code.
**	    o	Use new ifdefs from clconfig, handle no SEMUN case.
**	    o	Change include files to not grab system stuff directly.
**	15-apr-89 (arana)
**	    Moved handling of no SEMUN case into clipc.h so semun will
**	    always be defined.
**	19-Apr-89 (markd)
**  	    Changed ordering of inclusion of headers so compat.h is
**	    brought in before clipc.h	
**	22-jan-90 (fls-sequent)
**	    Fixed MCT re-entrancy problems by making sembuf sops variable
**	    local instead of static.
**	28-Mar-90 (anton)
**	    Move compat.h to before clconfig.h so clconfig.h can
**	    have the machine type information
**	    local instead of static.
**    4-june-90 (blaise)
**        Integrated changes from 61 and ingresug:
**	26-jun-90 (fls-sequent)
**	    Changed CS atomic locks to SHM atomic locks.
**	18-Oct-90 (fls-sequent)
**	    Integrated the following changes from the 63p CL:
**		Add escalating spin lock instead of always using the semaphore.
**		The number of spins is determined by looking at II_CSCP_SPINS.
**		If this is not set or "0", you get the old behaviour.  It only
**		makes sense to spin on multi-processors, like the sequent or
**		some pyramids.  An approriate value seems to be 5000, enough
**		to let somebody copy a page sized buffer and release the lock.
**		Add xCL_075_SYS_V_IPC_EXISTS around SysV specific shared memory
**		calls;
**		Add xCL_078_CVX_SEM to handle Convex semaphores;
**		Turn optimization off for hp3_us5.
**	15-apr-91 (vijay)
**		Added CS_slave_sleep(), which handles interrupts due to 
**		SIGDANGERs during semop.  esp. for ris_us5.
**	25-jul-91 (johnr)
**		Removed NO_OPTIM hp3_us5 for hp-ux 8.0
**      22-Oct-1992 (daveb)
**          Remove obsolete CScp stuff, add better diagnostics
**          (mostly TRdisplays).  These rely on some new erclf.msg
**          codes, and we include erclf.h to get them.
**	14-apr-93 (vijay)
**	    Fix typo, right error is SEMOP_GET.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	28-oct-93 (seiwald)
**	    Don't include erclf.h - it's a circular dependency.  Use 
**	    corresponding defines now in csnormal.h.
**/

/*
** Definition of all global variables and funtions used by this file.
*/

FUNC_EXTERN	STATUS		SHM_tas();
GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;



/*{
** Name: CS_sleep() - Sleep a UNIX process
**
** Description:
**	perform an operation on a system V semaphore to sleep a UNIX process
**
** Inputs:
**	semid		- sys V semaphore id
**	semnum		- sys V semaphore number
**
** Outputs:
**
**	Returns:
**	    E_DB_OK
**	    FAIL	- semaphore has been removed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**      01-mar-88 (anton)
**	    Created.
**	22-jan-90 (fls-sequent)
**	    Fixed MCT re-entrancy problems by making sembuf sops variable
**	    local instead of static.
**	26-jun-90 (fls-sequent)
**	    Changed CS_getspin to SHM_GETSPIN, CS_relspin to SHM_RELSPIN,
**	    and CS_TAS to SHM_TAS.
*/

# ifdef xCL_075_SYS_V_IPC_EXISTS
# define gotsleep
CS_sleep(semid, semnum)
int	semid, semnum;
{
    STATUS cl_stat;
    struct sembuf sops[1];

    sops[0].sem_num = semnum;
    sops[0].sem_op = -1;
    sops[0].sem_flg = 0;

    cl_stat = semop(semid, sops, 1) < 0 ? E_CS0050_CS_SEMOP_GET: OK;
    return( cl_stat );
}
# endif	/* xCL_075_SYS_V_IPC_EXISTS */

# ifdef xCL_078_CVX_SEM
# define gotsleep
CS_sleep (sem)
struct semaphore *sem;
{
    STATUS cl_stat;
    mset (sem,0);
    cl_stat = mset(sem, 1) < 0 ? E_CS0052_CS_MSET : OK;
    return( cl_stat );
}
# endif	/* xCL_078_CVX_SEM */

# ifndef gotsleep
CS_sleep(semid, semnum)
int     semid, semnum;
{
    return( E_CS0053_CS_SLEEP_BOTCH );
}
# endif	/* !gotsleep */

# ifdef SIGDANGER


/*{
** Name: CS_slave_sleep() -similar to CS_sleep() used only by slave which need
**		to handle SIGDANGERs.
**
** Description:
**      perform an operation on a system V semaphore to sleep a UNIX process.
**      Handle SIGDANGER interrupts.
**
** Inputs:
**      semid           - sys V semaphore id
**      semnum          - sys V semaphore number
**
** Outputs:
**
**      Returns:
**          E_DB_OK
**          FAIL        - semaphore has been removed
**
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**      02-apt-91 (vijay)
**          Created.
**      01-mar-88 (anton)
**	    Created.
**	22-jan-90 (fls-sequent)
**	    Fixed MCT re-entrancy problems by making sembuf sops variable
**	    local instead of static.
*/

CS_slave_sleep(semid, semnum)
int     semid, semnum;
{
    GLOBALREF   bool    rcvd_danger;
    STATUS      status;

    struct sembuf sops[1];

    sops[0].sem_num = semnum;
    sops[0].sem_op = -1;
    sops[0].sem_flg = 0;

    rcvd_danger = FALSE;
    while ((status = semop(semid, sops, 1)) < 0)
    {
        if ((status == -1) && (errno == EINTR) && (rcvd_danger))
        {
           rcvd_danger = FALSE;
           continue;
        }
        return( E_CS0050_CS_SEMOP_GET );
    }
    return(OK);
}

# endif /* SIGDANGER */


/*{
** Name: CS_wakeup - Wake a UNIX process sleeping on a sys V semaphore
**
** Description:
**	Wake a UNIX process sleeping on a sys V semaphore.
**
** Inputs:
**	semid		- sys V semaphore id
**	semnum		- sys V semaphore number
**
** Outputs:
**
**	Returns:
**	    E_DB_OK
**	    FAIL	- semaphore has been removed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-mar-88 (anton)
**	    Created.
**	22-jan-90 (fls-sequent)
**	    Fixed MCT re-entrancy problems by making sembuf sops variable
**	    local instead of static.
*/

# ifdef xCL_075_SYS_V_IPC_EXISTS
# define gotwakeup
CS_wakeup(semid, semnum)
int	semid, semnum;
{
    STATUS cl_stat;
    struct sembuf sops[1];

    sops[0].sem_num = semnum;
    sops[0].sem_op = 1;
    sops[0].sem_flg = 0;

    cl_stat = semop(semid, sops, 1) < 0 ? E_CS0053_CS_SEMOP_RELEASE : OK;
	
    return( cl_stat );
}
# endif	/* xCL_075_SYS_V_IPC_EXISTS */

# ifdef xCL_078_CVX_SEM
# define gotwakeup
CS_wakeup (sem)
struct semaphore *sem;
{
    STATUS cl_stat;
    cl_stat = mclear(sem) < 0 ? E_CS0054_CS_MCLEAR : OK;
    return( cl_stat );
}
# endif	/* xCL_078_CVX_SEM */

# ifndef gotwakeup
CS_wakeup(semid, semnum)
int     semid, semnum;
{
    return( E_CS0055_CS_WAKEUP_BOTCH );
}
# endif	/* !gotwakeup */



