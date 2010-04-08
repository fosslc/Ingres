/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef WCS_DB_INCLUDED
#define WCS_DB_INCLUDED

#include <ddfcom.h>
#include <ddgexec.h>

typedef struct __WPS_DOC {
	DDG_SESSION		ddg_session;
	struct __WPS_DOC	*next;
} WPS_DOC, *WPS_PDOC;

GSTATUS 
WCSDBInitialize();

GSTATUS 
WCSDBDisconnect(
	WPS_PDOC session);

GSTATUS
WCSDBConnect(
	WPS_PDOC *session);

GSTATUS 
WCSDBTerminate();

#endif
