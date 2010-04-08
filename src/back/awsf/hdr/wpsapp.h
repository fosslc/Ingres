/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
*/
#ifndef WPS_APP_INCLUDED
#define WPS_APP_INCLUDED

#include <actsession.h>

GSTATUS
WPSAppAdditionalParams (
	ACT_PSESSION act_session,
	char*	*params);

GSTATUS
WPSAppParentDirReference (
	char *app,
	bool *ret);

GSTATUS
WPSAppBegin (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name);

GSTATUS
WPSAppEnd (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer);

#endif
