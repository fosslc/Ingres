/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfhash.h>
#include <erwsf.h>
#include <wsf.h>
#include <wcslocation.h>

/*
**
**  Name: wcsunit.c - Unit memory management
**
**  Description:
**	This file contains the whole ice unit memory management package
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      07-Apr-1999 (fanra01)
**          Add initialisation for a default location for ICE 2.0.  If this
**          has been configtured temporary files are created here.
**      14-Jun-2000 (fanra01)
**          Bug 102052
**          Unit name update modified to terminate name if shorter than
**          previous.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-Jul-2002 (fanra01)
**          Bug 102222
**          Add unit id validation function.
**/

typedef struct __WCS_UNIT
{
	u_i4				id;
	char				*name;
	u_i4				owner;
	WCS_REGISTRATION	loc_reg;
} WCS_UNIT;

static DDFHASHTABLE		units = NULL;
static WCS_UNIT			**units_id = NULL;
static u_i4			wcs_unit_max = 0;
static u_i2				wcs_mem_unit_tag;
static char*                    wcs_default_location = NULL;

GSTATUS 
WCSDocsUseUnit (u_i4 unit, bool *result);

/*
** Name: WCSUnitIsOwner() - Check if the user is the owner of the unit
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**	u_i4	: user id
**
** Outputs:
**	bool*	: result
**
** Returns:
**	GSTATUS	:	E_WS0051_WCS_MEM_NOT_OPEN
**				GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSUnitIsOwner (
	u_i4	id,
	u_i4	user,
	bool	*result) 
{
	G_ASSERT(units == NULL, E_WS0051_WCS_MEM_NOT_OPEN);	

	if (id > 0 &&
		id < wcs_unit_max &&
		units_id[id] != NULL && 
		units_id[id]->owner == user)
		*result = TRUE;
	else
		*result = FALSE;
return(GSTAT_OK);
}

/*
** Name: WCSPerformUnitId() - Generate a unit id
**
** Description:
**
** Inputs:
**
** Outputs:
**	u_nat*	: unit id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSPerformUnitId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i < wcs_unit_max && units_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: WCSUnitAdd() - Add a new unit in the memory space
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**	char*	: unit name
**	u_i4	: unit owner
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSUnitAdd (
	u_i4 id,
	char* name,
	u_i4 owner) 
{
	GSTATUS err = GSTAT_OK;
	WCS_UNIT *unit = NULL;

	G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 0, E_WS0050_WCS_BAD_NUMBER);
	G_ASSERT(id < wcs_unit_max && 
					 units_id[id] != NULL, E_WS0053_WCS_EXISTING_PROJ);

	if (id >= wcs_unit_max) 
	{
		WCS_UNIT* *tmp = units_id;
		err = G_ME_REQ_MEM(
					wcs_mem_unit_tag, 
					units_id, 
					WCS_UNIT*, 
					id + WCS_DEFAULT_UNIT);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(
				tmp, 
				sizeof(WCS_UNIT*)*wcs_unit_max, 
				units_id);

		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			wcs_unit_max = id + WCS_DEFAULT_UNIT;
		}
		else
			units_id = tmp;
	}

	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(wcs_mem_unit_tag, unit, WCS_UNIT, 1);
		if (err == GSTAT_OK) 
		{
			u_i4 length;
			length = STlength(name) + 1;
			err = G_ME_REQ_MEM(wcs_mem_unit_tag, unit->name, char, length);
			if (err == GSTAT_OK) 
			{
				MECOPY_VAR_MACRO(name, length, unit->name);
				unit->id = id;
				unit->owner = owner;
				units_id[id] = unit;
				err = DDFputhash(units, unit->name, HASH_ALL, (PTR) unit);
			}
		}
	}
return(err);
}

/*
** Name: WCSGetUnitId() - Retrieve the unit id from his name
**
** Description:
**
** Inputs:
**	char*	: unit name
**
** Outputs:
**	u_nat*	: unit id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSGetUnitId (
	char* name,
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	WCS_UNIT *unit = NULL;

	G_ASSERT(units == NULL, E_WS0051_WCS_MEM_NOT_OPEN);

	if (name == NULL || name[0] == EOS)
		*id = SYSTEM_ID;
	else
	{
		err = DDFgethash(units, name, HASH_ALL, (PTR*) &unit);

		if (err == GSTAT_OK && unit != NULL) 
			*id = unit->id;
		else 
			err = DDFStatusAlloc ( E_WS0050_WCS_BAD_NUMBER );
	}
return(err);
}

/*
** Name: WCSGetUnitName() - Retrieve the unit name from his id
**
** Description:
**	Allocate memories if necessary
**
** Inputs:
**	u_nat*	: unit id
**
** Outputs:
**	char*	: unit name
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      09-Jul-2002 (fanra01)
**          Add null check on returned name prior to use.
*/
GSTATUS
WCSGetUnitName(
    u_i4 id,
    char  **name )
{
    GSTATUS err = GSTAT_OK;
    u_i4 length;
    G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
    G_ASSERT(id < 0 ||
        id >= wcs_unit_max ||
        units_id[id] == NULL, E_WS0050_WCS_BAD_NUMBER);

    if (name != NULL)
    {
        length = STlength(units_id[id]->name) + 1;
        err = GAlloc((PTR*)name, length, FALSE);
        if (err == GSTAT_OK)
            MECOPY_VAR_MACRO(units_id[id]->name, length, *name);
    }
    return(err);
}

/*
** Name: WCSUnitUpdate() - Update a unit into the memory
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**	char*	: unit name
**	u_i4	: unit owner
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      14-Jun-2000 (fanra01)
**          Bug 102052
**          Changed memory copy to a string copy to ensure null termination
**          of the unit name string when updated.
*/
GSTATUS
WCSUnitUpdate(
    u_i4  id,
    char* name,
    u_i4  owner )
{
    GSTATUS err = GSTAT_OK;
    WCS_UNIT *unit = NULL;

    G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
    G_ASSERT(id < 0 || id >= wcs_unit_max || units_id[id] == NULL,
        E_WS0050_WCS_BAD_NUMBER);

    err = DDFdelhash(units, units_id[id]->name, HASH_ALL, (PTR*) &unit);
    if (err == GSTAT_OK && unit != NULL)
    {
        i4 namelen = STlength(name);
        err = GAlloc((PTR*)&units_id[id]->name, namelen+1, FALSE);
        if (err == GSTAT_OK)
        {
            units_id[id]->owner = owner;
            STlcopy(name, unit->name, namelen );
            err = DDFputhash(units, unit->name, HASH_ALL, (PTR) unit);
        }
    }
    return(err);
}

/*
** Name: WCSUnitDelete() - delete a unit from the memory
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSUnitDelete (
	u_i4 id) 
{
	GSTATUS		err = GSTAT_OK;
	WCS_UNIT	*unit = NULL;
	bool		used = FALSE;
	G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 0 || 
					 id >= wcs_unit_max || 
					 units_id[id] == NULL, E_WS0050_WCS_BAD_NUMBER);

	err = WCSDocsUseUnit (id, &used);

	if (err == GSTAT_OK)
	{
		G_ASSERT(used == TRUE, E_WS0064_WSF_UNIT_USED);

		unit = units_id[id];
		while (unit->loc_reg.locations != NULL)
			CLEAR_STATUS(WCSUnRegisterLocation(
							&(unit->loc_reg), 
							unit->loc_reg.locations->location->id));

		err = DDFdelhash(units, unit->name, HASH_ALL, (PTR*) &unit);
		if (err == GSTAT_OK && unit != NULL) 
		{
			units_id[id] = NULL;
			MEfree((PTR)unit->name);
			MEfree((PTR)unit);
		}
	}
return(err);
}

/*
** Name: WCSUnitCopy() - Copy a unit into some variables (internal)
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**	u_i4	: unit id
**
** Outputs:
**	char**	: unit name
**	u_nat**	: unit owner id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS WCSUnitCopy (
	u_i4	id,
	char	**name,
	u_i4	**owner) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;
	WCS_UNIT *pro = units_id[id];

	length = STlength(pro->name) + 1;
	err = GAlloc((PTR*)name, length, FALSE);
	if (err == GSTAT_OK)
		MECOPY_VAR_MACRO(pro->name, length, *name);

	if (err == GSTAT_OK)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)owner, length, FALSE);
		if (err == GSTAT_OK)
			**owner = pro->owner;
	}
return(err);
}

/*
** Name: WCSGetProFromId() - Retrieve a unit from his id and return its infos
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**	u_i4	: unit id
**
** Outputs:
**	char**	: unit name
**	u_nat**	: unit owner id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSGetProFromId (
	u_i4 id,
	char	**name,
	u_i4	**owner) 
{
	G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(id < 0 || 
					 id >= wcs_unit_max || 
					 units_id[id] == NULL, E_WS0050_WCS_BAD_NUMBER);

	return(WCSUnitCopy(id,
					 name,
					 owner));
}

/*
** Name: WCSUnitBrowse() - list exsiting units
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**
** Outputs:
**	u_nat**	: unit id
**	char**	: unit name
**	u_nat**	: unit owner id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GSTATUS 
WCSUnitBrowse (
	PTR		*cursor,
	u_i4	**id,
	char	**name,
	u_i4	**owner) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 pos = (u_i4)(SCALARP) *cursor;

	while (pos < wcs_unit_max &&
		   units_id[pos] == NULL) 
		   pos++;

	if (pos < wcs_unit_max &&
		units_id[pos] != NULL)
	{
		u_i4 length = sizeof(u_i4);
		err = GAlloc((PTR*)id, length, FALSE);
		if (err == GSTAT_OK)
			**id = pos;
		if (err == GSTAT_OK)
			err = WCSUnitCopy(pos,
							 name,
							 owner);
		pos++;
	}
	else
		pos = 0;

	*cursor = (PTR)(SCALARP) pos;
return(err);
}

/*
** Name: WCSMemUnitLocAdd() - Add into the unit memory space the association
**						  with a location
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**	u_i4	: location id
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSMemUnitLocAdd (
	u_i4 unit,
	u_i4 location)
{
	G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(unit < 0 || 
					 unit >= wcs_unit_max || 
					 units_id[unit] == NULL, E_WS0050_WCS_BAD_NUMBER);

	return(WCSRegisterLocation(&(units_id[unit]->loc_reg), location));
}

/*
** Name: WCSMemUnitLocDel() - delete from the unit memory space the association
**						  with a location
**
** Description:
**
** Inputs:
**	u_i4	: unit id
**	u_i4	: location id
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSMemUnitLocDel (
	u_i4 unit,
	u_i4 location)
{
	G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
	G_ASSERT(unit < 0 || unit >= wcs_unit_max, E_WS0050_WCS_BAD_NUMBER);
	G_ASSERT(units_id[unit] == NULL, E_WS0050_WCS_BAD_NUMBER);

	return(WCSUnRegisterLocation(&(units_id[unit]->loc_reg), location));
}

/*
** Name: WCSUnitLocBrowse() - list unit's locations
**
** Description:
**
** Inputs:
**	PTR*	: cursor
**	u_i4	: unit id
**
** Outputs:
**	u_nat*	: location id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSUnitLocBrowse (
	PTR		*cursor,
	u_i4	unit,
	u_i4	*location)
{
	GSTATUS err = GSTAT_OK;
	WCS_LOCATION_LIST *tmp;

	if (*cursor == NULL)
	{
		G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
		G_ASSERT(unit < 0 || unit >= wcs_unit_max, E_WS0050_WCS_BAD_NUMBER);
		G_ASSERT(units_id[unit] == NULL, E_WS0050_WCS_BAD_NUMBER);
		tmp = units_id[unit]->loc_reg.locations;
		if (tmp != NULL)
		{
			*location = tmp->location->id;
			*cursor = (PTR) tmp;
		}
	}
	else
	{
		tmp = (WCS_LOCATION_LIST*) *cursor;
		tmp = tmp->next;
		if (tmp != units_id[unit]->loc_reg.locations)
		{
			*location = tmp->location->id;
			*cursor = (PTR) tmp;
		}
		else
			*cursor = (PTR) NULL;

	}
return(err);
}

/*
** Name: WCSUnitLocFind() - Find a location which can be used.
**
** Description:
**	The location returned is either a location of the current unit
**	or the default location
**
** Inputs:
**	PTR*	: cursor
**	u_i4	: unit id
**
** Outputs:
**	u_nat*	: location id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
WCSUnitLocFind (
	u_i4			loc_type,
	char*			ext,
	u_i4			unit,
	WCS_LOCATION	**loc)
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(unit < 0, E_WS0050_WCS_BAD_NUMBER);
	if (unit == SYSTEM_ID)
		err = WCSLocFind(loc_type, ext, NULL, loc);
	else
	{
		G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
		G_ASSERT(unit < 0 || unit >= wcs_unit_max, E_WS0050_WCS_BAD_NUMBER);
		G_ASSERT(units_id[unit] == NULL, E_WS0050_WCS_BAD_NUMBER);

		err = WCSLocFind(loc_type, ext, &units_id[unit]->loc_reg, loc);
	}
return(err);
}

/*
** Name: WCSUnitLocFindFromName() - Find a location which can be used.
**
** Description:
**	The location returned is either a location of the current unit
**	or the default location
**
** Inputs:
**	PTR*	: cursor
**	u_i4	: unit id
**
** Outputs:
**	u_nat*	: location id
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Apr-1999 (fanra01)
**          If no location has been specified, try the default one first.
*/
GSTATUS
WCSUnitLocFindFromName (
    u_i4           loc_type,
    char*           name,
    u_i4           unit,
    WCS_LOCATION    **loc)
{
    GSTATUS err = GSTAT_OK;
    G_ASSERT(unit < 0, E_WS0050_WCS_BAD_NUMBER);

    if (name == NULL || name[0] == EOS)
    {
        /*
        ** If the default location has been configured, get it.
        */
        if (wcs_default_location != NULL)
        {
            err = WCSLocFindFromName( loc_type, wcs_default_location, NULL,
                loc );
        }
        /*
        ** If no default location found use the html root directory
        */
        if ((err == GSTAT_OK) && (*loc == NULL))
        {
            WCSLocGetRoot(loc);
        }
    }
    else
    {
        if (unit == SYSTEM_ID)
            err = WCSLocFindFromName(loc_type, name, NULL, loc);
        else
        {
            G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
            G_ASSERT(unit < 0 || unit >= wcs_unit_max, E_WS0050_WCS_BAD_NUMBER);
            G_ASSERT(units_id[unit] == NULL, E_WS0050_WCS_BAD_NUMBER);

            err = WCSLocFindFromName(loc_type, name, &units_id[unit]->loc_reg,
                loc);
        }
    }
    return(err);
}

/*
** Name: WCSUnitInitialize() - set the unit memory space
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Apr-1999 (fanra01)
**          Add read of config parameter for default output location.
*/
GSTATUS 
WCSUnitInitialize () 
{
    GSTATUS err = GSTAT_OK;
    char*   size= NULL;
    char*   outdir= NULL;
    char    szConfig[CONF_STR_SIZE];
    i4      numberOfunit = 10;

    wcs_mem_unit_tag = MEgettag();

    STprintf (szConfig, CONF_UNIT_SIZE, PMhost());
    if (PMget( szConfig, &size ) == OK && size != NULL)
        CVan(size, &numberOfunit);

    err = DDFmkhash(numberOfunit, &units);
    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM( wcs_mem_unit_tag, units_id, WCS_UNIT*,
            WCS_DEFAULT_UNIT);
        wcs_unit_max = WCS_DEFAULT_UNIT;
    }
    if (err == GSTAT_OK)
    {
        if (PMget( CONF_OUTPUT_DEFAULT, &outdir ) == OK && (outdir != NULL))
        {
            wcs_default_location = STalloc( outdir );
        }
    }
    return(err);
}

/*
** Name: WCSUnitTerminate() - free the unit memory space
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSUnitTerminate () 
{
	GSTATUS err = GSTAT_OK;

	if (units_id != NULL)
	{
		u_i4 i;
		for (i = 0; i < wcs_unit_max; i++)
			if (units_id[i] != NULL)
				WCSUnitDelete(i);
	}

	CLEAR_STATUS (DDFrmhash(&units));
	MEfreetag(wcs_mem_unit_tag);
return(err);
}

/*
** Name: WCSvalidunit
**
** Description:
**      Tests the provided unit id for being in range and is in use.
**
** Inputs:
**	id      Unit id.
**
** Outputs:
**      valid   TRUE if unit is valid.
**              FALSE if invalid.
**              if NULL only the return status is given.
**
** Returns:
**	GSTAT_OK                    Unit id is valid
**      E_WS0051_WCS_MEM_NOT_OPEN   Unit list has not been initialized
**      E_WS0050_WCS_BAD_NUMBER     Unit id is not valid
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      09-Jul-2002 (fanra01)
**          Created.
*/
GSTATUS
WCSvalidunit( u_i4 id, bool* valid )
{
    GSTATUS err = GSTAT_OK;

    if (valid != NULL)
    {
        *valid = FALSE;
    }
    G_ASSERT(units_id == NULL, E_WS0051_WCS_MEM_NOT_OPEN);
    G_ASSERT(id < 0 ||
        id >= wcs_unit_max ||
        units_id[id] == NULL, E_WS0050_WCS_BAD_NUMBER);
    if (valid != NULL)
    {
        *valid = TRUE;
    }
    return(err);
}
