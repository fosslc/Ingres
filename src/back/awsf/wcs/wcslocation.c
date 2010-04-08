/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcslocation.h>
#include <ddfhash.h>
#include <erwsf.h>
#include <wsf.h>
#include <wcsmo.h>

/*
**
**  Name: wcslocation.c - Locaiton memory management
**
**  Description:
**	This file contains the whole ice location memory management package
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      01-Oct-1998 (fanra01)
**          Modified terminate loop condition when location not found.
**          Correct setting of next element when a location is found.
**      07-Dec-1998 (fanra01)
**          Correct duplicate row error when trying to insert more than 10
**          ice locations.  Changed to use configured maximum.
**      24-Feb-1999 (fanra01)
**          Correct comparison in WCSLocFind.
**          relativePath has been modified to have its memory.
**          During storage of the OS path and the relative path into the
**          WCS_LOCTION structure separator characters are normalised.
**      09-Mar-1999 (fanra01)
**          For non-HTTP locations ensure the OS path has the correct
**          separator.
**      03-Mar-2000 (chika01)
**          Bug 100682
**          Update WCSLocDeleteExtension() to check for error before 
**          deleting an ICE location.
**      17-Jul-2000 (fanra01)
**          Bug 102116
**          Add check for attempt to remove a non-existant location.
**      25-Jul-2000 (fanra01)
**          Bug 102168
**          Add automatic setting of public http locations via request for
**          2.0 compatibility. Add WCSGetRootPath for http location
**          verification from wmo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-Mar-2001 (fanra01)
**          Bug 104261
**          Test for the existance of optional parameter extension before
**          attempting to use it. Also correct some compiler warnings.
**      23-Mar-2001 (fanra01)
**          Bug 104321
**          Ensure that a relative URL path is stored without the first
**          URL seperator.
**      11-Jun-2001 (fanra01)
**          Bug 104921
**          Ensure that only HTTP locations are stored in the repository
**          with no root forward slash character. 
**      12-Sep-2001 (fanra01)
**          Bug 105760
**          Ensure that the registration of a locations use is correctly
**          updated when it is removed from a business unit.
**      07-Nov-2002 (fanra01)
**          Bug 102195
**          Modified WCSGrabLoc to perform location id test if output
**          parameter is not supplied.
**      07-Jan-2003 (fanra01)
**          Bug 109399
**          Add test to returned value for html_root read from configuration
**          file.
**	27-Oct-2003 (hanch04)
**	    Moved definition of EXT_ACTION to wcslocation.h
**	31-Oct-2003 (bonro01)
**	    Fix obvious coding errors in while() comparison.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

typedef struct __WCS_EXTENSION 
{
	char								*name;
	WCS_LOCATION_LIST		*first;
} WCS_EXTENSION;

typedef struct __WCS_REG_LIST
{
	WCS_PREGISTRATION			reg;
	struct __WCS_REG_LIST	*next;
} WCS_REG_LIST, *WCS_PREG_LIST;

typedef struct __WCS_LOCATION_DEF 
{
	WCS_LOCATION				info;
	WCS_PREG_LIST				reg_list;
} WCS_LOCATION_DEF, *WCS_PLOCATION_DEF;

static DDFHASHTABLE			locations = NULL;
static DDFHASHTABLE			extensions = NULL;
static WCS_LOCATION_DEF	**locations_id = NULL;
static u_i4						wcs_location_max = 0;
static u_i2							wcs_mem_loc_tag;
static WCS_LOCATION*		http_root = NULL;
static WCS_REGISTRATION	public_locs;

/*
** Name: WCSLocGetRoot() - Return the default location structure
**
** Description:
**
** Inputs:
**
** Outputs:
**	WCS_LOCATION ** : location info
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
WCSLocGetRoot(
	WCS_LOCATION*	*root) 
{
	*root = http_root;
return(GSTAT_OK);
}

/*
** Name: WCSGetRootPath
**
** Description:
**      Returns a pointer to the OS path of the web server's document root.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      path    of web server's document root.
**      NULL    if not configured.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      25-Jul-2000 (fanra01)
**          Created to return the web server root document directory to
**          verify creation of public http directories.
*/
PTR
WCSGetRootPath()
{
    return(http_root->path);
}

/*
** Name: WCSParseExtensions() - parse the extension list and execute an action 
**				for each extension found
**
** Description:
**	the format of the list has to be ext,ext,ext,...
**
** Inputs:
**	WCS_LOCATION *	: location
**	char*			: extension list
**	EXT_ACTION		: action
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
WCSParseExtensions(
	WCS_PREGISTRATION	reg,
	u_i4							id,
	EXT_ACTION				action) 
{
	GSTATUS err = GSTAT_OK;
	char*		exts = locations_id[id]->info.extensions;

	if (exts != NULL && exts[0] != EOS ) 
	{
		char *tmp, *begin;
		for (tmp = exts, begin = exts; err == GSTAT_OK && tmp != NULL && tmp[0] != EOS; tmp++)
		{
			if (tmp[0] == ',')
			{
				tmp[0] = EOS;
				err = action(begin, reg, &locations_id[id]->info);
				begin = tmp+1;
			}
		}
		err = action(begin, reg, &locations_id[id]->info);
	}
return(err);
}

/*
** Name: WCSLocAddExtension() - Add extension support into a location
**
** Description:
**
** Inputs:
**	char*			: extension name
**	WCS_LOCATION *	: location
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
WCSLocAddExtension(
	char*							text,
	WCS_PREGISTRATION	reg, 
	WCS_PLOCATION			loc)
{
	GSTATUS				err = GSTAT_OK;
	WCS_EXTENSION *extension = NULL;
	char*					name = NULL;
	
	err = G_ME_REQ_MEM(0, name, char, NUM_MAX_ADD_STR + STlength(text) + 2);
	if (err == GSTAT_OK)
	{
		STprintf(name, "%x_%s", reg, text);
		err = DDFgethash(extensions, name, HASH_ALL, (PTR*) &extension);
		if (err == GSTAT_OK && extension == NULL)
		{
			err = G_ME_REQ_MEM(wcs_mem_loc_tag, extension, WCS_EXTENSION, 1);
			if (err == GSTAT_OK) 
			{
				extension->name = name;
				name = NULL;
				err = DDFputhash(extensions, extension->name, HASH_ALL, (PTR) extension);
			}
		}
		MEfree((PTR)name);
	}
	if (err == GSTAT_OK)
	{
		WCS_LOCATION_LIST *tmp;
		err = G_ME_REQ_MEM(wcs_mem_loc_tag, tmp, WCS_LOCATION_LIST, 1);
		if (err == GSTAT_OK)
		{
			tmp->location = loc;
			if (extension->first == NULL)
			{
				tmp->next = tmp;
				tmp->previous = tmp;
			}
			else
			{
				tmp->next = extension->first;
				tmp->previous = extension->first->previous;
				extension->first->previous->next = tmp;
				extension->first->previous = tmp;
			}
			extension->first = tmp;
		}
	}
return(err);
}

/*
** Name: WCSLocDeleteExtension() - delete extension support from a location
**
** Description:
**
** Inputs:
**	char*			: extension name
**	WCS_LOCATION *	: location
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
**      03-Mar-2000 (chika01)
**          Check for error before attempting to delete an ICE location.
*/

GSTATUS
WCSLocDeleteExtension(
	char*							text,
	WCS_PREGISTRATION	reg, 
	WCS_PLOCATION			loc)
{
	GSTATUS err = GSTAT_OK;
	WCS_EXTENSION *extension;
	WCS_LOCATION_LIST *tmp;
	char*					name = NULL;
	
	err = G_ME_REQ_MEM(0, name, char, NUM_MAX_ADD_STR + STlength(text) + 2);
	if (err == GSTAT_OK)
	{
		STprintf(name, "%x_%s", reg, text);
		err = DDFgethash(extensions, name, HASH_ALL, (PTR*) &extension);
		if (err == GSTAT_OK && extension != NULL)
		{
			tmp = extension->first;
			while (tmp != NULL && tmp->location != loc)
				tmp = tmp->next;

			if (tmp == tmp->next)
				extension->first = NULL;
			else 
			{
				tmp->previous->next = tmp->next;
				tmp->next->previous = tmp->previous;
				if (tmp == extension->first)
					extension->first = tmp->next;
			}
		}
		MEfree((PTR)name);
	}
	if (err == GSTAT_OK && extension != NULL && extension->first == NULL)
	{
		err = DDFdelhash(extensions, text, HASH_ALL, (PTR*) &extension);
		MEfree((PTR) extension);
	}
return(err);
}

/*
** Name: WCSRegisterLocation() - Add a new location into the passed registry
**
** Description:
**
** Inputs:
**	WCS_PREGISTRATION	: registry
**	u_i4				: location id
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
WCSRegisterLocation (
	WCS_PREGISTRATION	reg,
	u_i4				id) 
{
	GSTATUS err = GSTAT_OK;
	WCS_LOCATION_LIST *tmp;
	
	G_ASSERT(locations_id == NULL || 
			 id < 1 ||
			 id >= wcs_location_max ||
			 locations_id[id] == NULL,
			 E_WS0050_WCS_BAD_NUMBER);

	err = G_ME_REQ_MEM(wcs_mem_loc_tag, tmp, WCS_LOCATION_LIST, 1);
	if (err == GSTAT_OK)
	{
		tmp->location = &locations_id[id]->info;
		if (reg->locations == NULL)
		{
			tmp->next = tmp;
			tmp->previous = tmp;
		}
		else
		{
			tmp->next = reg->locations;
			tmp->previous = reg->locations->previous;
			reg->locations->previous->next = tmp;
			reg->locations->previous = tmp;
		}
		reg->locations = tmp;
	}	

	if (err == GSTAT_OK)
			err = WCSParseExtensions(reg, id, WCSLocAddExtension);

	if (err == GSTAT_OK)
	{
		WCS_REG_LIST *tmp = NULL;
		err = G_ME_REQ_MEM(wcs_mem_loc_tag, tmp, WCS_REG_LIST, 1);
		if (err == GSTAT_OK)
		{
			tmp->reg = reg;
			tmp->next = locations_id[id]->reg_list;
			locations_id[id]->reg_list = tmp;
		}
	}
return(err);
}

/*
** Name: WCSUnRegisterLocation() - delete a location from the passed registry
**
** Description:
**
** Inputs:
**	WCS_PREGISTRATION	: registry
**	u_i4				: location id
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
**      12-Sep-2001 (fanra01)
**          The location information is maintained within multiple lists
**          so that location use can be identified.  When the location is
**          removed from the unit it should also be removed from the
**          the registered list.  Made the removal from the registered list
**          independent of the removal from the unit list.
*/
GSTATUS
WCSUnRegisterLocation (
    WCS_PREGISTRATION   reg,
    u_i4                id)
{
    GSTATUS err = GSTAT_OK;
    WCS_LOCATION_LIST *tmp = reg->locations;

    while (tmp != NULL && tmp->location->id != id)
    {
        tmp = tmp->next;
    }
    if (tmp != NULL)
    {
        CLEAR_STATUS(WCSParseExtensions(reg, id, WCSLocDeleteExtension));

        if (tmp == tmp->next)
            reg->locations = NULL;
        else
        {
            tmp->previous->next = tmp->next;
            tmp->next->previous = tmp->previous;
            if (tmp == reg->locations)
                reg->locations = tmp->next;
        }
        if (err == GSTAT_OK)
        {
            WCS_REG_LIST *tmp_reg = locations_id[id]->reg_list;
            if (tmp_reg->reg == reg)
                locations_id[id]->reg_list = locations_id[id]->reg_list->next;
            else
            {
                while (tmp_reg->next != NULL && tmp_reg->next->reg != reg)
                    tmp_reg = tmp_reg->next;

                if (tmp_reg->next != NULL)
                {
                    tmp_reg->next = tmp_reg->next->next;
                    tmp_reg = tmp_reg->next;
                }
            }
            MEfree((PTR)tmp_reg);
        }
        MEfree((PTR)tmp);
    }
    return(err);
}

/*
** Name: WCSPerformLocationId() - Generate a unique location id
**
** Description:
**
** Inputs:
**
** Outputs:
**	u_nat*	: id
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
**      07-Dec-1998 (fanra01)
**          Return 0 if limit is reached.
**      25-Jul-2000 (fanra01)
**          Add check of id list size on entry and extend the table if
**          required.  This is required for the automatic addition of http
**          locations used in 2.0 mode.
*/

GSTATUS 
WCSPerformLocationId (
    u_i4 *id)
{
    GSTATUS err = GSTAT_OK;
    u_i4    i = 1;

    for (i = 1; i < wcs_location_max && locations_id[i] != NULL; i++) ;
    if (i < wcs_location_max)
    {
        *id = i;
    }
    else
    {
        WCS_LOCATION_DEF**   tmp = locations_id;

        err = G_ME_REQ_MEM( wcs_mem_loc_tag, locations_id,
            WCS_LOCATION_DEF*, i + WCS_DEFAULT_LOCATION );
        if (err == GSTAT_OK && tmp != NULL)
        {
            MECOPY_CONST_MACRO( tmp,
                sizeof(WCS_LOCATION_DEF*)*wcs_location_max, locations_id );
        }
        if (err == GSTAT_OK)
        {
            MEfree( (PTR)tmp );
            wcs_location_max = i + WCS_DEFAULT_LOCATION;
            *id = i;
        }
        else
            locations_id = tmp;

        if (err != GSTAT_OK)
        {
            err = DDFStatusAlloc( E_WS0094_WCS_LOCATION_EXCEEDED );
            *id = 0;
        }
    }
    return(err);
}

/*
** Name: WCSGrabLoc() - Retrieve the location structure through his number
**
** Description:
**
** Inputs:
**	u_i4	: location id
**
** Outputs:
**	WCS_LOCATION ** : location info
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
**      07-Nov-2002 (fanra01)
**          Add output parameter check so that the location can be
**          verified if no output is specified.
*/

GSTATUS 
WCSGrabLoc( u_i4 id, WCS_LOCATION **loc )
{
    G_ASSERT(locations_id == NULL ||
        id < 1 ||
        id >= wcs_location_max ||
        locations_id[id] == NULL,
        E_WS0072_WCS_BAD_LOCATION);

    if (loc != NULL)
    {
       *loc = &locations_id[id]->info;
    }
    return(GSTAT_OK);
}

/*
** Name: WCSLocGet() - Retrieve the location structure through his name
**
** Description:
**
** Inputs:
**	char*	: location name
**
** Outputs:
**	WCS_LOCATION ** : location info
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
WCSLocGet(
	char*					name,
	WCS_LOCATION	**loc) 
{
	GSTATUS err = GSTAT_OK;
	*loc = NULL;
	err = DDFgethash(locations, name, HASH_ALL, (PTR*) loc);

	if (loc == NULL)
		err = DDFStatusAlloc( E_WS0072_WCS_BAD_LOCATION );
return(err);
}

/*
** Name: WCSLocFind() - Find a usable location from a specific registry
**
** Description:
**	search the appropriate location correspoding to the desired extension.
**	if there is no desired extension or no location was found, the function 
**	return the default location.
**
** Inputs:
**	char*	: desired extension 
**	WCS_PREGISTRATION	: registry
**
** Outputs:
**	WCS_LOCATION ** : location info
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
**      01-Oct-1998 (fanra01)
**          Modified terminate loop condition when a location not found.
**          Correct setting of next element when a location is found.
**      24-Feb-1999 (fanra01)
**          Correct assign to comparison.
*/

GSTATUS
WCSLocFind(
    u_i4               loc_type,
    char                *ext,
    WCS_PREGISTRATION   reg,
    WCS_LOCATION        **loc)
{
    GSTATUS err = GSTAT_OK;

    *loc = NULL;

    if (reg == NULL)
        reg = &public_locs;

    if (reg != NULL)
    {
        WCS_EXTENSION *extension = NULL;
        if (ext != NULL && ext[0] != EOS)
        {
            char*    name = NULL;

            err = G_ME_REQ_MEM(0, name, char, NUM_MAX_ADD_STR +
                STlength(ext) + 2);
            if (err == GSTAT_OK)
            {
                STprintf(name, "%x_%s", reg, ext);
                err = DDFgethash(extensions, name, HASH_ALL, (PTR*)&extension);
                MEfree((PTR)name);
            }
        }

        if (extension != NULL && extension->first != NULL)
        {
            WCS_LOCATION_LIST *first = extension->first;
            if (loc_type == 0 || first->location->type & loc_type)
            {
                *loc = first->location;
                extension->first = first->next;
            }
            else
                while (first->next != extension->first)
                {
                    if (loc_type == 0 ||
                            first->next->location->type & loc_type)
                    {
                        *loc = first->next->location;
                        extension->first = first->next->next;
                        break;
                    }
                    first = first->next;
                }
        }

        if (*loc == NULL && reg != NULL && reg->locations != NULL)
        {
            WCS_LOCATION_LIST *first = reg->locations;
            if (loc_type == 0 || first->location->type & loc_type)
            {
                *loc = first->location;
                reg->locations = first->next;
            }
            else
                while (first->next != reg->locations)
                {
                    if (loc_type == 0 ||
                            first->next->location->type & loc_type)
                    {
                        *loc = first->next->location;
                        reg->locations = first->next->next;
                        break;
                    }
                    first = first->next;
                }
        }
    }

    if (err == GSTAT_OK && *loc == NULL)
        err = DDFStatusAlloc(E_WS0046_LOCATION_UNAVAIL);
    return(err);
}

/*
** Name: WCSLocFindFromName() - Find a usable location from a specific registry
**
** Description:
**	search the appropriate location correspoding to the desired extension.
**	if there is no desired extension or no location was found, the function 
**	return the default location.
**
** Inputs:
**	char*	: desired extension 
**	WCS_PREGISTRATION	: registry
**
** Outputs:
**	WCS_LOCATION ** : location info
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
WCSLocFindFromName(
	u_i4			loc_type,
	char			*name,
	WCS_PREGISTRATION	reg,
	WCS_LOCATION **loc) 
{
	GSTATUS err = GSTAT_OK;

	*loc = NULL;

	if (reg == NULL)
		reg = &public_locs;

	if (reg != NULL && reg->locations != NULL)
	{
		WCS_LOCATION *tmp = NULL;
		WCS_LOCATION_LIST *first = reg->locations;

		err = WCSLocGet(name, &tmp);
		if (err == GSTAT_OK && tmp != NULL)
			do
			{
				if (tmp == first->location)
				{
					if (loc_type == 0 || tmp->type & loc_type)
						*loc = tmp;
					else
						err = DDFStatusAlloc(E_WS0090_WSS_BAD_LOC_TYPE);
					break;
				}
				first = first->next;
			}
			while (first != reg->locations);
	}
return(err);
}

/*
** Name: WCSLocAdd() - 
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
**      24-Feb-1999 (fanra01)
**          relativePath has been modified to have its memory.
**          During storage of the OS path and the relative path into the
**          WCS_LOCTION structure separator characters are normalised.
**      09-Mar-1999 (fanra01)
**          For non-HTTP locations ensure the OS path has the correct
**          separator.
**      25-Jul-2000 (fanra01)
**          Add check of id list size on entry and extend the table if
**          required.  This is required for the automatic addition of http
**          locations used in 2.0 mode.
**      07-Mar-2001 (fanra01)
**          Test existence of extension pointer before use.
**      23-Mar-2001 (fanra01)
**          Don't save the starting URL path seperator as generation of the
**          relative path will include it.
**      08-Jun-2001 (fanra01)
**          Ensure that URL path separator is only removed for HTTP locations.
*/

GSTATUS 
WCSLocAdd(
    u_i4    id,
    char*    name,
    u_i4    type,
    char*    path,
    char*    extensions)
{
    GSTATUS err = GSTAT_OK;
    WCS_LOCATION_DEF *loc = NULL;
    WCS_EXTENSION *list = NULL;

    G_ASSERT(path == NULL || path[0] == EOS, E_WS0055_WCS_BAD_PATH);

    if (id >= wcs_location_max)
    {
        WCS_LOCATION_DEF**   tmp = locations_id;

        err = G_ME_REQ_MEM( wcs_mem_loc_tag, locations_id,
            WCS_LOCATION_DEF*, id + WCS_DEFAULT_LOCATION );
        if (err == GSTAT_OK && tmp != NULL)
        {
            MECOPY_CONST_MACRO( tmp,
                sizeof(WCS_LOCATION_DEF*)*wcs_location_max, locations_id );
        }
        if (err == GSTAT_OK)
        {
            if (tmp != NULL)
            {
                MEfree( (PTR)tmp );
            }
            wcs_location_max = id + WCS_DEFAULT_LOCATION;
        }
        else
            locations_id = tmp;

        if (err != GSTAT_OK)
        {
            err = DDFStatusAlloc( E_WS0094_WCS_LOCATION_EXCEEDED );
        }
    }
    if (err == GSTAT_OK)
    {
        err = DDFgethash(locations, name, HASH_ALL, (PTR*) &loc);
    }
    if (err == GSTAT_OK && loc == NULL)
    {
        err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc, WCS_LOCATION_DEF, 1);
        if (err == GSTAT_OK)
        {
            u_i4 length;
            u_i4 plen;

            length = STlength(name) + 1;
            err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.name, char, length);
            if (err == GSTAT_OK)
            {
                char* normal;       /* platform normalised target path */
                char* pathstr;
                char* relpath;      /* url normalised relative path */

                MECOPY_VAR_MACRO(name, length, loc->info.name);
                plen = STlength(path);

                length = plen +
                    (((http_root != NULL) && (type & WCS_HTTP_LOCATION)) ?
                            (STlength(http_root->path) + 2) : 1);

                err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.path, char,
                    length);
                if (err == GSTAT_OK)

                {
                    /*
                    ** If the location is of type HTTP copy the HTTP root path
                    ** into the path string, replacing forward slash with the
                    ** OS path separator if it is different.
                    */
                    normal = loc->info.path;
                    if ((http_root != NULL) && (type & WCS_HTTP_LOCATION))
                    {
                        pathstr = http_root->path;
                        for (; *pathstr != EOS; CMnext(pathstr), CMnext(normal))
                        {
                            if (CHAR_PATH_SEPARATOR != CHAR_URL_SEPARATOR)
                            {
                                if (*pathstr == CHAR_URL_SEPARATOR)
                                    *normal = CHAR_PATH_SEPARATOR;
                                else
                                    CMcpychar(pathstr,normal);
                            }
                            else
                            {
                                CMcpychar (pathstr, normal);
                            }
                        }
                        /*
                        **  Append a separator
                        */
                        *normal = CHAR_PATH_SEPARATOR;
                        CMnext (normal);
                    }
                }

                err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.relativePath,
                    char, length);
                if (err == GSTAT_OK)
                {
                    /*
                    ** Save a copy of the relative path into the structure.
                    */
                    relpath = loc->info.relativePath;
                    pathstr = path;
                    if ((*pathstr == CHAR_URL_SEPARATOR) && 
                        (type & WCS_HTTP_LOCATION))
                    {
                        /*
                        ** First character is path separator skip it.  Don't
                        ** include it in the generation of the relative path.
                        */
                        CMnext(pathstr);
                    }
                    for (; *pathstr != EOS; CMnext(pathstr), CMnext(normal),
                        CMnext(relpath))
                    {
                        if (CHAR_PATH_SEPARATOR != CHAR_URL_SEPARATOR)
                        {
                            /*
                            ** Ensure that all directory separators are
                            ** changed to the OS directory separator.
                            */
                            if (*pathstr == CHAR_URL_SEPARATOR)
                            {
                                *normal = CHAR_PATH_SEPARATOR;
                            }
                            else
                            {
                                CMcpychar(pathstr,normal);
                            }
                            if (type & WCS_HTTP_LOCATION)
                            {
                                /*
                                ** This path is an HTTP location replace any
                                ** OS path separators with forward slashes into
                                ** the relative path
                                */
                                if (*pathstr == CHAR_PATH_SEPARATOR)
                                    *relpath = CHAR_URL_SEPARATOR;
                                else
                                    CMcpychar(pathstr,relpath);
                            }
                            else
                            {
                                CMcpychar(pathstr,relpath);
                            }
                        }
                        else
                        {
                            CMcpychar (pathstr, normal);
                            CMcpychar (pathstr, relpath);
                        }
                    }
                    *normal = EOS;
                    *relpath=EOS;
                }

                if(extensions != NULL)
                {
                    length = STlength(extensions) + 1;
                    err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.extensions, char, length);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(extensions, length, loc->info.extensions);
                    }
                }
                if (err == GSTAT_OK)
                {
                    loc->info.id = id;
                    loc->info.type = type;
                    if (err == GSTAT_OK)
                    {
                        err = DDFputhash(locations, loc->info.name, HASH_ALL, (PTR) loc);
                        if (err == GSTAT_OK)
                            wcs_floc_attach (loc->info.name, &loc->info);
                    }
                }
            }
        }
    }

    if (err == GSTAT_OK)
    {
        locations_id[id] = loc;
        if (type & WCS_PUBLIC)
            err = WCSRegisterLocation (&public_locs, id);
    }
return(err);
}

/*
** Name: WCSLocUpdate() - Update location into the location memory space
**
** Description:
**
** Inputs:
**	u_i4	location id
**	char*	location name
**	u_i4	location type
**	char*	location path
**	char*	extensions list supported
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
**      24-Feb-1999 (fanra01)
**          relativePath has been modified to have its memory.
**          During storage of the OS path and the relative path into the
**          WCS_LOCTION structure separator characters are normalised.
*/
GSTATUS
WCSLocUpdate(
    u_i4    id,
    char*    name,
    u_i4    type,
    char*    path,
    char*    extensions)
{
    GSTATUS err = GSTAT_OK;
    WCS_LOCATION_DEF *loc = NULL;
    WCS_EXTENSION *list = NULL;

    G_ASSERT(path == NULL || path[0] == EOS, E_WS0055_WCS_BAD_PATH);

    err = DDFdelhash(locations, locations_id[id]->info.name, HASH_ALL, (PTR*) &loc);
    if (err == GSTAT_OK && loc != NULL)
    {
        char* tmp_name = loc->info.name;
        char* tmp_ext = loc->info.extensions;
        char* tmp_path = loc->info.path;
        WCS_PREG_LIST    tmp_reg = loc->reg_list;
        u_i4 length;
        u_i4 plen;

        while (tmp_reg != NULL)
        {
            CLEAR_STATUS(WCSParseExtensions(tmp_reg->reg,
               id,
               WCSLocDeleteExtension));
            tmp_reg = tmp_reg->next;
        }

        loc->info.name = NULL;
        loc->info.path = NULL;
        loc->info.extensions = NULL;

        if ((loc->info.type & WCS_PUBLIC))
                err = WCSUnRegisterLocation (&public_locs, id);

        if (err == GSTAT_OK)
        {
            length = STlength(name) + 1;
            err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.name, char, length);
        }
        if (err == GSTAT_OK)
        {
            char* normal;
            char* pathstr;
            char* relpath;

            MECOPY_VAR_MACRO(name, length, loc->info.name);
            plen = STlength(path);

            length = plen +
                (((http_root != NULL) && (type & WCS_HTTP_LOCATION)) ?
                        (STlength(http_root->path) + 2) : 1);

            err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.path, char,
                length);
            if (err == GSTAT_OK)

            {
                /*
                ** If the location is of type HTTP copy the HTTP root path
                ** into the path string, replacing forward slash with the
                ** OS path separator if it is different.
                */
                normal = loc->info.path;
                if ((http_root != NULL) && (type & WCS_HTTP_LOCATION))
                {
                    pathstr = http_root->path;
                    for (; *pathstr != EOS; CMnext(pathstr), CMnext(normal))
                    {
                        if (CHAR_PATH_SEPARATOR != CHAR_URL_SEPARATOR)
                        {
                            if (*pathstr == CHAR_URL_SEPARATOR)
                                *normal = CHAR_PATH_SEPARATOR;
                            else
                                CMcpychar(pathstr,normal);
                        }
                        else
                        {
                            CMcpychar (pathstr, normal);
                        }
                    }
                    /*
                    **  Append a separator
                    */
                    *normal = CHAR_PATH_SEPARATOR;
                    CMnext (normal);
                }
            }

            err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.relativePath,
                char, length);
            if (err == GSTAT_OK)
            {
                /*
                ** Save a copy of the relative path into the structure.
                */
                relpath = loc->info.relativePath;
                pathstr = path;
                for (; *pathstr != EOS; CMnext(pathstr), CMnext(normal),
                    CMnext(relpath))
                {
                    if (CHAR_PATH_SEPARATOR != CHAR_URL_SEPARATOR)
                    {
                        if (type & WCS_HTTP_LOCATION)
                        {
                            /*
                            ** This path is an HTTP location replace any
                            ** forward slashes with OS path separator into
                            ** the full path
                            */
                            if (*pathstr == CHAR_URL_SEPARATOR)
                                *normal = CHAR_PATH_SEPARATOR;
                            else
                                CMcpychar(pathstr,normal);

                            /*
                            ** This path is an HTTP location replace any
                            ** OS path separators with forward slashes into
                            ** the relative path
                            */
                            if (*pathstr == CHAR_PATH_SEPARATOR)
                                *relpath = CHAR_URL_SEPARATOR;
                            else
                                CMcpychar(pathstr,relpath);
                        }
                        else
                        {
                            if (*pathstr == CHAR_URL_SEPARATOR)
                                *normal = CHAR_PATH_SEPARATOR;
                            else
                                CMcpychar(pathstr,normal);
                            CMcpychar(pathstr,relpath);
                        }
                    }
                    else
                    {
                        CMcpychar (pathstr, normal);
                        CMcpychar (pathstr, relpath);
                    }
                }
                *normal = EOS;
                *relpath=EOS;
            }
            length = STlength(extensions) + 1;
            err = G_ME_REQ_MEM(wcs_mem_loc_tag, loc->info.extensions, char, length);
            if (err == GSTAT_OK)
            {
                MECOPY_VAR_MACRO(extensions, length, loc->info.extensions);
                loc->info.id = id;
                loc->info.type = type;

                tmp_reg = loc->reg_list;
                while (err == GSTAT_OK && tmp_reg != NULL)
                {
                    err = WCSParseExtensions(tmp_reg->reg,
                              id,
                              WCSLocAddExtension);
                    tmp_reg = tmp_reg->next;
                }

                if (err == GSTAT_OK && (type & WCS_PUBLIC))
                        err = WCSRegisterLocation (&public_locs, id);

                if (err == GSTAT_OK)
                    err = DDFputhash(locations, loc->info.name, HASH_ALL, (PTR) loc);
            }
        }

        if (err == GSTAT_OK)
        {
            MEfree((PTR)tmp_name);
            MEfree((PTR)tmp_path);
            MEfree((PTR)tmp_ext);
        }
        else
        {
            MEfree((PTR)loc->info.path);
            MEfree((PTR)loc->info.name);
            MEfree((PTR)loc->info.extensions);
            loc->info.name = tmp_name;
            loc->info.path = tmp_path;
            loc->info.extensions = tmp_ext;

            tmp_reg = loc->reg_list;
            while (err == GSTAT_OK && tmp_reg != NULL)
            {
                err = WCSParseExtensions(tmp_reg->reg,
                    id, WCSLocAddExtension);
                tmp_reg = tmp_reg->next;
            }
            CLEAR_STATUS( DDFputhash(locations, loc->info.name, HASH_ALL, (PTR) loc) );
        }
    }
    return(err);
}

/*
** Name: WCSLocDelete() - delete location from the location memory space
**
** Description:
**
** Inputs:
**	u_i4	location id
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
**      06-Jul-98 (fanra01)
**          Add the removal of the location ima object.
**      17-Jul-2000 (fanra01)
**          Add check for attempt to remove a non-existant location.
*/

GSTATUS
WCSLocDelete( u_i4 id )
{
    GSTATUS err = GSTAT_OK;
    WCS_LOCATION_DEF *loc = NULL;

    G_ASSERT( locations_id[id] == NULL, E_WS0099_WCS_UNDEF_LOCATION_ID );

    if ((locations_id[id]->info.type & WCS_PUBLIC))
        CLEAR_STATUS(WCSUnRegisterLocation (&public_locs, id));

    G_ASSERT(locations_id[id]->reg_list != NULL, E_WS0083_WSF_LOC_USED);

    DDFdelhash(locations, locations_id[id]->info.name, HASH_ALL, (PTR*) &loc);
    if (err == GSTAT_OK && loc != NULL)
    {
        wcs_floc_detach (loc->info.name);
        MEfree((PTR)loc->info.name);
        MEfree((PTR)loc->info.path);
        MEfree((PTR)loc->info.extensions);
        MEfree((PTR)loc);
        locations_id[id] = NULL;
    }
    return(err);
}

/*
** Name: WCSLocCopy() - copy locaiton information into variables
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**	u_i4	location id
**
** Outputs:
**	char**	location name
**	u_nat**	location type
**	char**	location path
**	char**	extensions list supported
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
WCSLocCopy (
	u_i4	id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char  **relativePath,
	char	**extensions)
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;
	WCS_LOCATION *loc = &locations_id[id]->info;

	if (name != NULL)
	{
		length = STlength(loc->name) + 1;
		err = GAlloc((PTR*)name, length, FALSE);
		if (err == GSTAT_OK)
			MECOPY_VAR_MACRO(loc->name, length, *name);
	}

	if (err == GSTAT_OK && type != NULL)
	{
		length = sizeof(u_i4);
		err = GAlloc((PTR*)type, length, FALSE);
		if (err == GSTAT_OK)
			**type = loc->type;
	}

	if (err == GSTAT_OK && path != NULL)
	{
		length = STlength(loc->path) + 1;
		err = GAlloc((PTR*)path, length, FALSE);
		if (err == GSTAT_OK)
			MECOPY_VAR_MACRO(loc->path, length, *path);
	}

	if (err == GSTAT_OK && relativePath != NULL)
	{
		length = STlength(loc->relativePath) + 1;
		err = GAlloc((PTR*)relativePath, length, FALSE);
		if (err == GSTAT_OK)
			MECOPY_VAR_MACRO(loc->relativePath, length, *relativePath);
	}

	if (err == GSTAT_OK && extensions != NULL)
	{
            if(loc->extensions != NULL)
            {
		length = STlength(loc->extensions) + 1;
		err = GAlloc((PTR*)extensions, length, FALSE);
		if (err == GSTAT_OK)
			MECOPY_VAR_MACRO(loc->extensions, length, *extensions);
            }
	}
return(err);
}

/*
** Name: WCSGetLocFromId() - retrieve location information through his number
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**	u_i4	location id
**
** Outputs:
**	char**	location name
**	u_nat**	location type
**	char**	location path
**	char**	extensions list supported
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
WCSGetLocFromId (
	u_i4	id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char  **relativePath,
	char	**extensions)
{
	G_ASSERT(locations_id == NULL || 
			 id < 1 ||
			 id >= wcs_location_max ||
			 locations_id[id] == NULL,
			 E_WS0072_WCS_BAD_LOCATION);

	return(WCSLocCopy(id,
										name,
										type,
										path,
										relativePath,
										extensions));
}

/*
** Name: WCSLocBrowse() - list exsiting locations 
**
** Description:
**	Allocate memory if necessary
**
** Inputs:
**	PTR*	: cursor
**
** Outputs:
**	u_i4	location id
**	char**	location name
**	u_nat**	location type
**	char**	location path
**	char**	extensions list supported
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
WCSLocBrowse (
	PTR *cursor,
	u_i4	**id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char	**relativePath, 
	char	**extensions)
{
	GSTATUS err = GSTAT_OK;
	u_i4 pos = (u_i4)(SCALARP) *cursor;

	while (pos < wcs_location_max &&
		   locations_id[pos] == NULL) 
		   pos++;

	if (pos < wcs_location_max &&
		locations_id[pos] != NULL)
	{
		u_i4 length = sizeof(u_i4);
		err = GAlloc((PTR*)id, length, FALSE);
		if (err == GSTAT_OK)
			**id = pos;

			if (err == GSTAT_OK)
				err = WCSLocCopy(	pos,
													name,
													type,
													path,
													relativePath,
													extensions);
		pos++;
	}
	else
		pos = 0;

	*cursor = (PTR)(SCALARP) pos;
return(err);
}

/*
** Name: WCSLocInitialize() - Initialize the location memory space
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
**      08-Jul-98 (fanra01)
**          Add location class definition for ima.
**      07-Dec-1998 (fanra01)
**          Initialise wcs_location_max with configured maximum.
**      07-Jan-2003 (fanra01)
**          Add test for returned value for html_root.
*/

GSTATUS 
WCSLocInitialize () 
{
    GSTATUS err = GSTAT_OK;
    char*    size= NULL;
    char    szConfig[CONF_STR_SIZE];
    char*    root = NULL;
    i4  numberOflocation = WCS_DEFAULT_LOCATION;
    i4  numberOfextensions = WCS_DEFAULT_LOCATION;

    wcs_mem_loc_tag = MEgettag();

    STprintf (szConfig, CONF_HTML_HOME, PMhost());
    if ((PMget( szConfig, &root ) == OK) && (root) && (*root))
    {
        err = G_ME_REQ_MEM(wcs_mem_loc_tag, http_root, WCS_LOCATION, 1);
        if (err == GSTAT_OK)
        {
            err = G_ME_REQ_MEM(wcs_mem_loc_tag, http_root->name, char, 1);
            if (err == GSTAT_OK)
            {
                u_i4 length = STlength(root);
                err = G_ME_REQ_MEM(wcs_mem_loc_tag,
                    http_root->path,
                    char,
                    length+1);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(root, length + 1, http_root->path);
                    http_root->relativePath = http_root->path + length;
                    http_root->type = WCS_HTTP_LOCATION;
                }
            }
        }
    }
    else
    {
        err = DDFStatusAlloc(E_WS0104_WCS_HTTP_ROOT);
        http_root = NULL;
    }

    if (err == GSTAT_OK)
    {
        STprintf (szConfig, CONF_LOCATION_SIZE, PMhost());
        if (PMget( szConfig, &size ) == OK && size != NULL)
            CVan(size, &numberOflocation);

        err = DDFmkhash(numberOflocation, &locations);

        STprintf (szConfig, CONF_EXTENSION_SIZE, PMhost());
        if (PMget( szConfig, &size ) == OK && size != NULL)
            CVan(size, &numberOfextensions);

        err = DDFmkhash(numberOfextensions, &extensions);
        if (err == GSTAT_OK)
        {
            err = G_ME_REQ_MEM(
                   wcs_mem_loc_tag,
                   locations_id,
                   WCS_PLOCATION_DEF,
                   numberOflocation);

            wcs_location_max = (numberOflocation != 0) ?
                numberOflocation : WCS_DEFAULT_LOCATION;
            wcs_floc_define ();
        }
    }
    return(err);
}

/*
** Name: WCSLocTerminate() - free the location memory space
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
WCSLocTerminate () 
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(locations == NULL, E_WS0004_WCS_MEM_NOT_OPEN);

	if (locations_id != NULL)
	{
		u_i4 i;
		for (i = 0; i < wcs_location_max; i++)
			if (locations_id[i] != NULL)
				WCSLocDelete(i);
	}

	CLEAR_STATUS(DDFrmhash(&locations));
	if (http_root != NULL)
	{
		MEfree((PTR)http_root->name);
		MEfree((PTR)http_root);
		http_root = NULL;
	}
	MEfreetag(wcs_mem_loc_tag);
return(err);
}

/*
** Name: WPSPathVerify() - Verify if the path match with the http root directory
**
** Description:
**	update the repository
**	allocate and declare a new location into the ICE memory
**
** Inputs:
**	char*	: path
**
** Outputs:
**	char*	: delta 
**
** Returns:
**	GSTATUS	:	...
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
WPSPathVerify(
	char*	path,
	char*	*delta)
{
	GSTATUS err = GSTAT_OK;
	char*	root = http_root->path;
	char*	tmp = path;

	if (tmp != NULL && root != NULL)
	{
		while (	tmp[0] != EOS && 
				root[0] != EOS &&
				tmp[0] == root[0])
		{
			tmp++;
			root++;
		}
		if (root[0] != EOS)
			err = DDFStatusAlloc(E_WS0084_WSF_INCORRECT_PATH);
		if (tmp[0] == CHAR_PATH_SEPARATOR)
			tmp++;
	}
	*delta = tmp;
return(err);
}

/*
** Name: WPSGetRelativePath() - 
**
** Description:
**      Function returns the relative path for a location.
**
** Inputs:
**      info    Pointer WCS_LOCATION structure where the relative path
**              string is stored.
**
** Outputs:
**      path    Pointer to the relative path string.
**
** Returns:
**      E_WS0055_WCS_BAD_PATH   NULL structure pointer passed.  Path is null.
**      GSTAT_OK                Path updated
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      24-Feb-1999 (fanra01)
**          Relative path is now separated from the path in its own string.
**          Update function to return pointer to relative path.
*/

GSTATUS
WPSGetRelativePath(
    WCS_LOCATION*   info,
    char**          path)
{
    GSTATUS err = GSTAT_OK;

    if (info != NULL)
    {
        *path = info->relativePath;
    }
    else
    {
        err = DDFStatusAlloc(E_WS0055_WCS_BAD_PATH);
        *path = NULL;
    }

    return(err);
}
