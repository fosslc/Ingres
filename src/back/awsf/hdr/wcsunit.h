/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998
**	    Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WCS_UNIT_INCLUDED
#define WCS_UNIT_INCLUDED

#include <ddfcom.h>

GSTATUS 
WCSPerformUnitId (
	u_i4 *id);

GSTATUS 
WCSUnitAdd (
	u_i4 id,
	char* name,
	u_i4 owner);

GSTATUS 
WCSGetUnitId (
	char* name,
	u_i4 *id);

GSTATUS 
WCSUnitUpdate (
	u_i4 id,
	char* name,
	u_i4 owner) ;

GSTATUS 
WCSUnitDelete (
	u_i4 id);

GSTATUS 
WCSGetProFromId (
	u_i4 id,
	char	**name,
	u_i4	**owner);

GSTATUS 
WCSUnitBrowse (
	PTR		*cursor,
	u_i4	**id,
	char	**name,
	u_i4	**owner);

GSTATUS WCSUnitInitialize ();

GSTATUS WCSUnitTerminate ();

GSTATUS 
WCSMemUnitLocAdd (
	u_i4 unit,
	u_i4 location);

GSTATUS 
WCSMemUnitLocDel (
	u_i4 unit,
	u_i4 location);

GSTATUS 
WCSUnitLocBrowse (
	PTR		*cursor,
	u_i4	unit,
	u_i4	*location_id);

GSTATUS 
WCSUnitLocFind (
	u_i4			loc_type,
	char*			ext,
	u_i4			unit,
	WCS_LOCATION	**loc);

GSTATUS 
WCSUnitLocFindFromName (
	u_i4			loc_type,
	char*			name,
	u_i4			unit,
	WCS_LOCATION	**loc);

#endif
