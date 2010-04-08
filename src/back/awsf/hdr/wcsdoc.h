/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef WCS_DOC_INCLUDED
#define WCS_DOC_INCLUDED

#include <ddfcom.h>

GSTATUS 
WCSPerformDocumentId (
	u_i4 *id);

GSTATUS 
WCSDocAdd (
	u_i4	id,
	u_i4	unit,
	char*	name, 
	char*	suffix, 
	i4 flag,
	u_i4	owner,
	u_i4	ext_loc,
	char*	ext_file,
	char*	ext_suffix);

GSTATUS
WCSDocGet (
	u_i4 unit,
	char *name,
	char *suffix,
	u_i4 *id);

GSTATUS
WCSDocSetFile (
	u_i4 id,
	WCS_FILE *file);

GSTATUS
WCSDocGetFile (
	u_i4 id,
	WCS_FILE **file);

GSTATUS
WCSDocGetExtFile (
	u_i4	id,
	u_i4	*location_id,
	char*	*filename,
	char*	*suffix);

GSTATUS 
WCSDocUpdate (
	u_i4 id,
	u_i4 unit,
	char *name, 
	char *suffix, 
	i4 flag,
	u_i4 owner,
	u_i4	ext_loc,
	char*	ext_file,
	char*	ext_suffix);

GSTATUS 
WCSDocDelete (
	u_i4	id);

GSTATUS WCSGetDocFromId (
	u_i4 id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix);

GSTATUS WCSDocBrowse (
	PTR *cursor,
	u_i4	**id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix);

GSTATUS WCSDocInitialize ();

GSTATUS WCSDocTerminate ();

#endif
