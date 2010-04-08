/*
** Copyright (c) 1983, 1999 Ingres Corporation
*/

/*
**
**  Name: pcpidq.h - defines related to the PID queue
**
**  Description:
**	This file contains the definition of the PID queue.
**
**  History:
**	3/82 (gb) written
**	21-may-91 (seng)
**	    Add int declaration in front of Pidq_init.
**	    RS/6000 compiler chokes on statement.
**	09-nov-1999 (somsa01)
**	    Tailored for NT. The PID queue now holds thread ids and their
**	    handles.
*/

/*
** Structure to store Thread ID for spawned process/thread and it's
** status.
*/
typedef struct _pidque
{   
    QUEUE 	pidq;
    PID		pid;
    HANDLE	hPid;
    STATUS	stat;
} PIDQUE;

GLOBALREF QUEUE		Pidq;
GLOBALREF bool		Pidq_init;
GLOBALREF CS_SYNCH	Pidq_mutex;

