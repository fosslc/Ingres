/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: wtsinit.h
**
** Description:
**      File contains definitions and function prototypes for wts.
**
** History:
**      15-Feb-1999 (fanra01)
**          Add prototype for cleanup thread function and add string for
**          thread description.
*/
#ifndef WTS_INIT_INCLUDED
#define WTS_INIT_INCLUDED

# define ICE_CLEANUPTHREAD_NAME " <Cleanup Checking Thread>      "

DB_STATUS
WTSInitialize(DB_ERROR *err);

DB_STATUS
WTSTerminate(DB_ERROR *err);

DB_STATUS
WTSCleanup(DB_ERROR *err);

#endif
