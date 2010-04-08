/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      01-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
*/
#ifndef WSS_PARS_INCLUDED
#define WSS_PARS_INCLUDED

#include <wsfparser.h>
#include <actsession.h>
#include <wsf.h>

GSTATUS 
ParseHTMLVariables(
		ACT_PSESSION act_session, 
		char *var);

#endif
