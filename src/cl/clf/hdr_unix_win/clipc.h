/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** CLIPC.H -- include ipc things that are needed
**
** Separated because some systems may not have/want/need the SV IPC.
**
** History:
**	27-Oct-1988 (daveb)
**	    Created.
**	15-Apr-89 (arana)
**	    Added define for semun if it doesn't exist.
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Added ifdef for SysV IPC in case someone doesn't want it.
*/

# ifndef CLCONFIG_H_INCLUDED
error "You didn't include clconfig.h before including clipc.h"
# endif

/* right now, everybody gets SV IPC */

# ifdef xCL_075_SYS_V_IPC_EXISTS
# include    <sys/ipc.h>
# include    <sys/sem.h>
# endif

# ifndef xCL_002_SEMUN_EXISTS
    union semun
    {
	int val;
	struct semid_ds *buf;
	ushort *array;
    } arg;
# endif
