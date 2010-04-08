/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: wcslocation.h
**
** Descritpion:
**      File contains the structures and function prototypes for file
**      handling.
**
** History:
**      24-Feb-1999 (fanra01)
**          Add history.
**          Change WPSGetRelativePath to reflect that the relative path is
**          now maintained in a separate string buffer.
**      27-Oct-2003 (hanch04)
**          Add prototypes for missing functions.
**
*/
#ifndef WCS_LOC_INCLUDED
#define WCS_LOC_INCLUDED

#include <ddfcom.h>
#include <ddgexec.h>

typedef struct __WCS_LOCATION
{
	u_i4					id;
	char*					name;
	char*					path;
	char*					relativePath;
	u_i4					type;
	char*					extensions;
} WCS_LOCATION, *WCS_PLOCATION;

typedef struct __WCS_LOCATION_LIST
{
	WCS_LOCATION								*location;
	struct __WCS_LOCATION_LIST	*previous;
	struct __WCS_LOCATION_LIST	*next;
} WCS_LOCATION_LIST;

typedef struct __WCS_REGISTRATION
{
	WCS_LOCATION_LIST		*locations;
} WCS_REGISTRATION, *WCS_PREGISTRATION;

typedef GSTATUS (*EXT_ACTION) (char*, WCS_PREGISTRATION, WCS_PLOCATION);

GSTATUS 
WPSPathVerify(
	char*	path,
	char*	*delta);

GSTATUS
WPSGetRelativePath(
    WCS_LOCATION*   info,
    char**          path);

PTR
WCSGetRootPath();

GSTATUS
WCSParseExtensions(
WCS_PREGISTRATION	reg,
u_i4			id,
EXT_ACTION		action);

GSTATUS 
WCSLocGetRoot(
	WCS_LOCATION*	*root);

GSTATUS
WCSLocDeleteExtension(
	char*                  text,
	WCS_PREGISTRATION      reg,
	WCS_PLOCATION          loc);

GSTATUS
WCSLocAddExtension(
	char*                   text,
	WCS_PREGISTRATION       reg,
	WCS_PLOCATION           loc);


GSTATUS 
WCSRegisterLocation (
	WCS_PREGISTRATION	reg,
	u_i4				id);

GSTATUS 
WCSUnRegisterLocation (
	WCS_PREGISTRATION	reg,
	u_i4				id);

GSTATUS 
WCSPerformLocationId (
	u_i4 *id);

GSTATUS 
WCSGrabLoc(
	u_i4 id,
	WCS_LOCATION **loc);

GSTATUS 
WCSLocGet(
	char					*name,
	WCS_LOCATION	**loc);

GSTATUS 
WCSLocFind(
	u_i4			loc_type,
	char			*ext,
	WCS_PREGISTRATION	reg,
	WCS_LOCATION **loc);

GSTATUS
WCSLocFindFromName(
	u_i4			loc_type,
	char			*name,
	WCS_PREGISTRATION	reg,
	WCS_LOCATION **loc);

GSTATUS 
WCSLocAdd(
	u_i4 id,
	char *name,
	u_i4 type,
	char *path, 
	char *extensions);

GSTATUS
WCSLocUpdate(
	u_i4 id,
	char *name,
	u_i4 type,
	char *path, 
	char *extensions);

GSTATUS
WCSLocDelete(
	u_i4	id);

GSTATUS WCSGetLocFromId (
	u_i4	id,
	char	**name,
	u_i4	**type,
	char	**path,
	char  **relativePath,
	char	**extensions);

GSTATUS WCSLocBrowse (
	PTR *cursor,
	u_i4	**id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char	**relativePath,
	char	**extensions);


GSTATUS WCSLocInitialize ();

GSTATUS WCSLocTerminate ();


#endif
