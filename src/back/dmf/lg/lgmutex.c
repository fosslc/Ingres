/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGMUTEX.C - Implements the mutex functions of the logging system
**
**  Description:
**	This module contains the code which implements the mutual exclusion
**	primitives used by LG.
**	
**	    LG_imutex  -- Initialize a LG mutex
**	    LG_mutex   -- Lock a LG mutex.
**	    LG_unmutex -- unlock a LG mutex.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	13-oct-1992 (bryanp)
**	    Temporarily remove the EXinterrupt() calls since they goof up VMS.
**	    In particular, EXinterrrupt disables AST's, and that causes all
**	    sorts of grief (for example, CSp_semaphore doesn't work right).
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-may-1994 (bryanp) B58498
**	    After releasing mutex, check to see if an interrupt was
**		received; if so, exit gracefully.
**	26-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Mutex pointer must now be
**	    passed to each function. Removed sys_err function parm
**	    which wasn't being used anyway.
**	11-Dec-1997 (jenjo02)
**	  o If CSp_semaphore(), CSv_semaphore() fails with 
**	    E_CS0029_SEM_OWNER_DEAD and the requestor is the
**	    RCP, ignore the error to facilitate recovery.
**	  o Removed LG_rmutex() function, which has never been used.
**	  o Removed call to CSa_semaphore() within LG_imutex().
**	02-Feb-2000 (jenjo02)
**	    Inline CSp|v_semaphore calls. LG_mutex, LG_unmutex are
**	    now macros calling LG_mutex_fcn(), LG_unmutex_fcn() iff
**	    a semaphore error occurs.
**	    Added mutex name, module name and line number to DMA42E, DMA42F
**	    messages to aid debugging.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

static i4	LG_interrupt_received = 0;

/*{
** Name: LG_mutex_fcn()	- Get a specific LG mutex.
**
** Description:
**      Get mutex on an LG object.
**
** Inputs:
**	mode				SEM_SHARE or
**					SEM_EXCL
**	mutex				semaphore to grab
**	file				name of source module making
**					this call
**	line				line number within that module
**
** Outputs:
**	none.
**
**	Returns:
**	    OK				- success
**	    other non-zero return	- failure.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Mutex pointer must now be
**	    passed to each function. Removed sys_err function parm
**	    which wasn't being used anyway.
**	11-Dec-1996 (jenjo02)
**	    If semaphore owner is dead and this is Master, return OK
**	    to facilitate recovery.
**	02-Feb-2000 (jenjo02)
**	    Renamed from LG_mutex, added "file" and "line" parms.
**	    Modified E_DMA42E to show mutex name, file, and line,
**	    removed now-redundant TRdisplays.
*/
STATUS
LG_mutex_fcn(i4	mode,
	 CS_SEMAPHORE *mutex, 
	 char	*file,
	 i4 	line)
{
    STATUS	status;

#ifdef LGLK_NO_INTERRUPTS
    EXinterrupt(EX_OFF);
#endif

    if (status = CSp_semaphore(mode, mutex))
    {
	if (status == E_CS0029_SEM_OWNER_DEAD)
	{
	    LGD         *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
	    LPB		*lpb;

	    if (lgd->lgd_master_lpb)
	    {
		lpb = (LPB*)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

		if (lpb->lpb_pid == LG_my_pid)
		    status = OK;
	    }
	}

	if (status)
	{
	    i4	        err_code;
	    CS_SEM_STATS    	sstat;

	    /* 
	    ** Use CSs_semaphore to get the name of the mutex with
	    ** the error, if we can.
	    */
	    if ( CSs_semaphore(CS_INIT_SEM_STATS, mutex, &sstat, sizeof(sstat)) )
		STprintf(sstat.name, "-unknown-");

#ifdef LGLK_NO_INTERRUPTS
	    EXinterrupt(EX_ON);
#endif
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA42E_LG_MUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			    0, sstat.name,
			    0, file,
			    0, line);
	}
    }

    return(status);
}

/*{
** Name: LG_unmutex_fcn()	- release a LG mutex.
**
** Description:
**	Release a LG mutex.
**
** Inputs:
**	mutex				semaphore to release
**	file				name of source module making
**					this call
**	line				line number within that module
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	23-may-1994 (bryanp) B58498
**	    After releasing mutex, check to see if an interrupt was
**		received; if so, exit gracefully.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Mutex pointer must now be
**	    passed to each function. Removed sys_err function parm
**	    which wasn't being used anyway.
**	11-Dec-1996 (jenjo02)
**	    If Master, ignore errors to facilitate recovery.
**	02-Feb-2000 (jenjo02)
**	    Renamed from LG_unmutex, added "file" and "line" parms.
**	    Modified E_DMA42F to show mutex name, file, and line,
**	    removed now-redundant TRdisplays.
*/
STATUS
LG_unmutex_fcn(CS_SEMAPHORE *mutex,
	char	*file,
	i4 	line)
{
    STATUS      status;

    status = CSv_semaphore(mutex);

#ifdef LGLK_NO_INTERRUPTS
    EXinterrupt(EX_ON);
#endif

    if (status)
    {
	LGD             *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
	LPB		*lpb;

	if (lgd->lgd_master_lpb)
	{
	    lpb = (LPB*)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

	    if (lpb->lpb_pid == LG_my_pid)
		status = OK;
	}

	if (status)
	{
	    i4	 	 err_code;
	    CS_SEM_STATS 	 sstat;

	    /* 
	    ** Use CSs_semaphore to get the name of the mutex with
	    ** the error, if we can.
	    */
	    if ( CSs_semaphore(CS_INIT_SEM_STATS, mutex, &sstat, sizeof(sstat)) )
		STprintf(sstat.name, "-unknown-");

	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA42F_LG_UNMUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			    0, sstat.name,
			    0, file,
			    0, line);
	}
    }

    if (LG_interrupt_received)
    {
	LKintrack();
	LGintrack();
	PCexit(FAIL);
    }

    return(status);
}

/*{
** Name: LG_imutex()	- Initialize a cross-process LG mutex.
**
** Description:
**	Initialize and name a LG mutex.
**
** Inputs:
**	mutex				semaphore to initialize
**	name				what to call it
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	26-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Mutex pointer must now be
**	    passed to each function. Removed sys_err function parm
**	    which wasn't being used anyway.
**	    Added CSa_semaphore() on successful mutex init.
**	11-Dec-1997 (jenjo02)
**	    Removed CSa_semaphore() call. Semaphores are no longer
**	    attached to MO.
*/
STATUS
LG_imutex(CS_SEMAPHORE *mutex, char *mutex_name)
{
    STATUS      status;

    if (status = CSw_semaphore(mutex, CS_SEM_MULTI, mutex_name))
    {
	i4	err_code;

	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DMA430_LG_IMUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }
    return(status);
}

void
LGintrcvd(void)
{
    LG_interrupt_received = 1;
}
void
LGintrack(void)
{
    LG_interrupt_received = 0;
}
