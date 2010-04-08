/*
** Copyright 1992, 2004 Ingres Corporation
**
** Name: CAIIRO.C
**
** Description:
**      Defines GetReadOnly routine to always return TRUE for read-only default
**      DLL.
**
** History:
**      07-mar-2003 (loera01)
**          Created.
**      19-mar-2003 (loera01)
**          Made compatible with Unix/Linux.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
*/
#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>
#include <sqlca.h>

BOOL GetReadOnly( char * dsn)
{
	return 1;
}
