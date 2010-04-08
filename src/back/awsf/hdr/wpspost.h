/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      01-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
**      21-Jun-2001 (fanra01)
**          Sir 103096
**          Add outtype to WPSExecuteQuery prototype.
*/
#ifndef WPS_POST_INCLUDED
#define WPS_POST_INCLUDED

#include <actsession.h>

GSTATUS
WPSExecuteProcedure (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION	act_session, 
	WPS_PBUFFER	 buffer,
	char		*name);

GSTATUS
WPSExecuteQuery (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* query,
        i4 outtype);

GSTATUS
WPSExecuteReport (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name,
	char* location);

GSTATUS
WPSExecuteApp (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char *pszApp);

#endif
