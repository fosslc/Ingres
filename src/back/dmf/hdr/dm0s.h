/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM0S.H - Definition for SCF interface.
**
** Description:
**      This module contains the decriptions of the SCF interface routines.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
**	28-feb-1989 (rogerk)
**	    Add DM0S_TCBWAIT_EVENT event type.
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	23-aug-1993 (bryanp)
**	    Redefine DM0S_TCBWAIT_EVENT so that it's different than the wait
**		codes used by dm0p.h. This probably doesn't fix any real bugs,
**		but it improves the ability of lockstat to be able to display
**		interesting lock event wait information (before this, it
**		couldn't distinguish an event wait for wbflush from an event
**		wait for tcbwait).
**	09-may-1995 (cohmi01)
**	    Define DM0S_READAHEAD_EVENT, ch dm0s_ewait to return status.
**	26-Oct-1996 (jenjo02)
**	    Added *name to dm0s_minit() prototype.
[@history_template@]...
**/

/*
** Event types used for dm0s event wait/signal protocols
*/
#define		DM0S_E_BUFFER	    	0
#define		DM0S_TCBWAIT_EVENT  	0x100000L
#define		DM0S_READAHEAD_EVENT 	0x900000L 


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID dm0s_minit(DM_MUTEX *mutex, char *name);
FUNC_EXTERN VOID dm0s_mlock(DM_MUTEX *mutex);
FUNC_EXTERN VOID dm0s_mrelease(DM_MUTEX *mutex);
FUNC_EXTERN VOID dm0s_munlock(DM_MUTEX *mutex);

FUNC_EXTERN VOID dm0s_eset(
			i4             lock_list,
			i4		    type,
			i4		    event);

FUNC_EXTERN DB_STATUS dm0s_elprepare(
			i4             lock_list,
			i4		    type,
			i4		    event);

FUNC_EXTERN VOID dm0s_elrelease(
			i4             lock_list,
			i4		    type,
			i4		    event);

FUNC_EXTERN VOID dm0s_elwait(
			i4             lock_list,
			i4		    type,
			i4		    event);

FUNC_EXTERN VOID dm0s_erelease(
			i4             lock_list,
			i4		    type,
			i4		    event);

FUNC_EXTERN DB_STATUS dm0s_ewait(
			i4             lock_list,
			i4		    type,
			i4		    event);

