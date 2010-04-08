/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      11-Sep-1998 (fanra01)
**          Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      20-Mar-2001 (fanra01)
**          Correct function spelling of WCSDispatchName.
**      09-Jul-2002 (fanra01)
**          Bug 102222
**          Add prototype for WCSvalidunit.
**      09-Jul-2002 (fanra01)
**          Bug 108207
**          Add prototype for WCSvaliddoc.
*/
#ifndef WCS_INCLUDED
#define WCS_INCLUDED

#include <ddfcom.h>
#include <ddginit.h>
#include <wcsfile.h>

typedef struct _WCS_FILE {
	u_i4						doc_id;
	WCS_FILE_INFO		*info;
	LOCATION				loc;
	FILE						*fd;
} WCS_FILE, *WCS_PFILE;

GSTATUS 
WCSInitialize ();

GSTATUS 
WCSTerminate();

GSTATUS 
WCSDispatchName (
	char*	message,
	char*	*unit,
	char*	*location,
	char*	*name,
	char*	*suffix);

GSTATUS
WCSPerformFileName(
	u_i4 unit,
	u_i4 doc, 
	char **name);

GSTATUS
WCSRequest (
	u_i4		loc_type,
	char		*location_name,
	u_i4		unit_id,
	char		*name, 
	char		*suffix,
	WCS_PFILE	*file,
	bool		*status);

GSTATUS 
WCSLoaded(
	WCS_PFILE	file,
	bool		isIt);

GSTATUS 
WCSDelete(
	WCS_PFILE	file);
		  
GSTATUS 
WCSRelease (
	WCS_PFILE	*file);

GSTATUS 
WCSCreateLocation(
	u_i4	*id,
	char*	name,
	u_i4	type,
	char*	path, 
	char*	extensions);

GSTATUS 
WCSUpdateLocation(
	u_i4	id,
	char*	name,
	u_i4	type,
	char*	path, 
	char*	extensions);

GSTATUS
WCSDeleteLocation(
	u_i4	id);

GSTATUS
WCSOpenLocation (PTR		*cursor);

GSTATUS
WCSFindLocation(
	u_i4			loc_type,
	u_i4			unit_id,
	char			*name,
	char			*suffix,
	WCS_LOCATION	**location);

GSTATUS 
WCSRetrieveLocation (
	u_i4	id,
	char	**name,
	u_i4	**type,
	char	**path,
	char	**relativePath,
	char	**extensions);

GSTATUS 
WCSGetLocation (
	PTR		*cursor,
	u_i4	**id,
	char	**name,
	u_i4	**type,
	char	**path,
	char	**relativePath,
	char	**extensions);

GSTATUS 
WCSCloseLocation (PTR		*cursor);

GSTATUS 
WCSDocIsOwner (
	u_i4	id,
	u_i4	user,
	bool	*result);

GSTATUS 
WCSUnitIsOwner (
	u_i4	id,
	u_i4	user,
	bool	*result);

GSTATUS
WCSDocGetUnit (
	u_i4	id,
	u_i4	*unit_id);

GSTATUS
WCSDocGetName (
	u_i4	id,
	char*	*name);

GSTATUS 
WCSCreateDocument (
	u_i4 *id,
	u_i4 unit,
	char* name,
	char* suffix,
	u_i4 flag,
	u_i4 owner,
	u_i4 ext_loc,
	char* ext_file,
	char* ext_suffix);

GSTATUS 
WCSUpdateDocument (
	u_i4 id,
	u_i4 unit,
	char* name,
	char* suffix,
	u_i4 flag,
	u_i4 owner,
	u_i4 ext_loc,
	char* ext_file,
	char* ext_suffix);

GSTATUS 
WCSExtractDocument (
	WCS_PFILE	file);

GSTATUS
WCSDeleteDocument (
	u_i4	id);

GSTATUS
WCSOpenDocument (PTR		*cursor);

GSTATUS 
WCSRetrieveDocument (
	u_i4	id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix);

GSTATUS 
WCSGetDocument (
	PTR		*cursor,
	u_i4	**id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix);

GSTATUS 
WCSCloseDocument (PTR		*cursor);

GSTATUS
WCSDocCheckFlag (
	u_i4 id,
	i4 flag,
	bool *result);

GSTATUS 
WCSCreateUnit (
	u_i4 *id,
	char* name,
	u_i4 owner);

GSTATUS 
WCSGetUnitId (
	char* name,
	u_i4 *id);

GSTATUS 
WCSGetUnitName (
	u_i4 id,
	char  **name);

GSTATUS 
WCSUpdateUnit (
	u_i4 id,
	char* name,
	u_i4 owner);

GSTATUS 
WCSDeleteUnit (
	u_i4 id);

GSTATUS
WCSOpenUnit (PTR		*cursor);

GSTATUS 
WCSRetrieveUnit (
	u_i4	id, 
	char	**name,
	u_i4	**owner);

GSTATUS 
WCSGetUnit (
	PTR		*cursor,
	u_i4	**id, 
	char	**name,
	u_i4	**owner);

GSTATUS 
WCSCloseUnit (PTR		*cursor);

GSTATUS 
WCSUnitLocAdd (
	u_i4 unit,
	u_i4 location);

GSTATUS 
WCSUnitLocDel (
	u_i4 unit,
	u_i4 location);

GSTATUS
WCSOpenUnitLoc (
	PTR		*cursor);

GSTATUS 
WCSGetUnitLoc (
	PTR		*cursor,
	u_i4	unit,
	u_i4	*location_id);

GSTATUS 
WCSCloseUnitLoc (
	PTR				*cursor);

GSTATUS 
WCSOpen(
	WCS_FILE	*file,
	char			*mode,
	i4				type,
	i4		length);

GSTATUS 
WCSRead(
	WCS_FILE	*file,
	i4		numofbytes,
	i4		*actual_count,
	char			*pointer);

GSTATUS 
WCSWrite(
	i4		size,
	char			*pointer,
	i4		*count,
	WCS_FILE	*file);

GSTATUS 
WCSClose(
	WCS_FILE	*file);

GSTATUS 
WCSLength (
	WCS_FILE	*file,
	i4		*length);

GSTATUS
WCSvalidunit( u_i4 id, bool* valid );

GSTATUS
WCSvaliddoc( u_i4 id, bool* valid );

#endif
