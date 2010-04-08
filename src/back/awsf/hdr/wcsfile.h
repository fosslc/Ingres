/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef WCS_FILE_INCLUDED
#define WCS_FILE_INCLUDED

#include <ddfcom.h>
#include <wcslocation.h>

typedef struct __WCS_FILE_INFO 
{
	char					*key;
	char					*name;
	char					*path;
	WCS_LOCATION	*location;
	u_i4					status;
	u_i4					counter;
	bool					exist;
} WCS_FILE_INFO;

GSTATUS
WCSPerformPath(
	u_i4	location_type,
	char*	location_path,
	char*	filename,
	char*	*path);

GSTATUS 
WCSFileRequest(
	WCS_LOCATION	*location,
	char			*name,
	char			*suffix,
	WCS_FILE_INFO	**info,
	u_i4			*status);

GSTATUS 
WCSFileLoaded(
	WCS_FILE_INFO *info,
	bool		  status);

GSTATUS 
WCSFileRelease(
	WCS_FILE_INFO *info);

GSTATUS WCSFileInitialize ();

GSTATUS WCSFileTerminate ();

GSTATUS
WCSOpenCache(
	PTR *cursor);

GSTATUS
WCSGetCache(
	PTR			*cursor,
	char*		*key,
	char*		*file_name,
	char*		*location_name,
	u_i4*	*status,
	u_i4*	*count,
	bool*		*exist);

GSTATUS
WCSCloseCache(
	PTR *cursor);

#endif
