/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected case of usrsession to match piccolo.
*/
#ifndef WPSMO_INCLUDED
#define WPSMO_INCLUDED

#include <usrsession.h>

STATUS
wps_define (void);

STATUS
wps_context_attach (
	PTR pszName, 
	USR_PSESSION pWpsContext);

VOID
wps_context_detach (
	PTR pszName);

#endif
