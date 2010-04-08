/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      01-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WPS_REPORT_INCLUDED
#define WPS_REPORT_INCLUDED

#include <actsession.h>
#include <wcs.h>

GSTATUS
WPSGetReportParameters (
	ACT_PSESSION act_session,
	char*    *pszParams);

GSTATUS
WPSGetOutputDirLoc (
	ACT_PSESSION act_session,
	WPS_PFILE *ploOut,
	WPS_PFILE *ploErr);

GSTATUS
WPSGetReportCommand (
	char*	*command,
	u_i4	*ixParam,
	char*	*apszParam,
	ACT_PSESSION act_session,
	char*	dbname,
	char*	name,
	char*	location,
	char*	params,
	LOCATION*	ploOut);

GSTATUS
WPSReportError (
	ACT_PSESSION	act_session,
	WPS_PBUFFER	 buffer,
	char*		command,
	char*		*apszParam,
	WPS_FILE	*pploErr);

GSTATUS
WPSReportSuccess (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name,
	char* location,
	WPS_FILE*   ploOut);

GSTATUS
HasTags (
	char*	pszBuf, 
	bool	*ret);

#endif
