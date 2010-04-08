/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfhash.h>
#include <erwsf.h>
#include <wsf.h>
#include <wcsfile.h>

#include <wcsmo.h>

/*
**
**  Name: wcsfile.c - System file management
**
**  Description:
**		Memorize all physical files requested.
**		The mechanism is:
**			When a file is requested either the file is avaliable or needs to be loaded.
**			Then, the request function will return the status. The function which requested
**			the file has to load the file and then call WCSFileFileLoaded. At this time 
**			the file could be used by others.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      08-Jun-1999 (fanra01)
**          Add semaphore protection to the access of the file hash table.
**      04-Jan-2000 (fanra01)
**          Bug 99923
**          Changed the scan loop in WCSFileTerminate to remove nested
**          opens.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  27-Mar-2003 (fanra01)
**      Bug 109934
**      Sharing violations during multiple concurrent access to the same
**      files.
**      Modified information order of key generation for requested files.
**   20-Aug-2003 (fanra01)
**      Bug 110747
**      Removed memory allocation dependency on memory tag.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

static SEMAPHORE    WCSFileSem;
static DDFHASHTABLE files = NULL;
static u_i2         wcs_mem_file_tag;

/*
** Name: WCSPerformPath() - Perform physical file 
**
** Description:
**
** Inputs:
**	u_i4		: location type
**	char*		: location path
**	char*		: filename
**
** Outputs:
**	char**		 : path
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
**      08-Jul-2003 (fanra01)
**          Remove dependancy on the wcs_mem_file_tag for memory as the info
**          object can be freed independently.
*/
GSTATUS
WCSPerformPath(
    u_i4    location_type,
    char*    location_path,
    char*    filename,
    char*    *path)
{
    GSTATUS        err = GSTAT_OK;
    u_i4            length = 0;
    char*            tmp = NULL;

    length = ((location_path == NULL) ? 0 : STlength(location_path)) +
                      ((filename == NULL) ? 0 : STlength(filename)) +
                     (sizeof(char) * 5);

    err = G_ME_REQ_MEM(0, tmp, char, length);
    tmp[0] = EOS;
    length = 0;
    if (err == GSTAT_OK)
    {
        if (location_path != NULL)
        {
            STcat(tmp, location_path);
            length += STlength(location_path);
        }

        if (filename != NULL)
        {

            int i;

            for (i = 0; filename[i] != EOS; i++)
                if (filename[i] == CHAR_PATH_SEPARATOR)
                {
                    length = 0;
                    *tmp = EOS;
                    break;
                }

            if (length != 0)
            {
                tmp[length++] = CHAR_PATH_SEPARATOR;
                tmp[length] = EOS;
            }
            STcat(tmp, filename);
        }

      for (length = 0; tmp[length] != EOS; length++)
            if (tmp[length] == CHAR_URL_SEPARATOR)
                tmp[length] = CHAR_PATH_SEPARATOR;

        if (*path != NULL)
            MEfree((PTR)*path);
        *path = tmp;
    }
    return(err);
}

/*
** Name: WCSFileExist() - Check if the file exists
**
** Description:
**
** Inputs:
**	char*		 : path
**
** Outputs:
**	bool*		 : result (TRUE or FALSE)
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
WCSFileExist (
	char			*path,
	bool			*exist)
{
	LOINFORMATION	loinf;
	i4				flagword;
	LOCATION		loc;

	LOfroms(PATH & FILENAME, path, &loc);
	flagword = (LO_I_TYPE | LO_I_PERMS);
	*exist = (LOinfo(&loc, &flagword, &loinf) == OK);
return (GSTAT_OK);
}

/*
** Name: WCSFileRequest() - Request access to a physical file
**
** Description:
**
** Inputs:
**	WCS_LOCATION*	: location
**	char*			: name
**	char*			: suffix
**
** Outputs:
**	WCS_FILE_INFO**	: info,
**	u_nat*			: status of the file (AVAILABLE, LOADED)
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
**          Add the creation of a file info object instance for ima.
**      08-Jun-1999 (fanra01)
**          Semaphore protect the reading from the hash table.
**      10-Jun-1999 (fanra01)
**          Change indexing to include thread id for concurrent file access.
**      27-Mar-2003 (fanra01)
**          Modified information order for key generation.
**      08-Jul-2003 (fanra01)
**          Remove dependancy on the wcs_mem_file_tag for memory as the info
**          object can be freed independently.
*/
GSTATUS 
WCSFileRequest(
    WCS_LOCATION    *location,
    char            *name,
    char            *suffix,
    WCS_FILE_INFO   **info,
    u_i4            *status)
{
    GSTATUS         err = GSTAT_OK;
    WCS_FILE_INFO   *object = NULL;
    char*           key = NULL;
    u_i4            length = NUM_MAX_ADD_STR + 2;
    u_i4            name_offset;
    CS_SID          sessid;

    G_ASSERT(files == NULL, E_WS0004_WCS_MEM_NOT_OPEN);
    G_ASSERT(name == NULL, E_WS0047_BAD_FILE_NAME);

    length += STlength(name) + ((suffix) ? (STlength(suffix) + 1) : 0);

    err = G_ME_REQ_MEM(0, key, char, length);
    if (err == GSTAT_OK)
    {
        CSget_sid(&sessid);
        STprintf (key, "%x%x%c", sessid, location, CHAR_PATH_SEPARATOR );
        name_offset = STlength(key);

        if (suffix == NULL)
            STprintf (key, "%s%s", key, name);
        else
            STprintf (key, "%s%s%c%s", key, name, CHAR_FILE_SUFFIX_SEPARATOR, suffix);

        if ((err == GSTAT_OK) &&
            ((err = DDFSemOpen( &WCSFileSem, TRUE )) == GSTAT_OK))
        {
            err = DDFgethash(files, key, HASH_ALL, (PTR*) &object);
            if (err == GSTAT_OK)
            {
                if (object == NULL)
                {
                    err = G_ME_REQ_MEM(0, object, WCS_FILE_INFO, 1);
                    if (err == GSTAT_OK)
                    {
                        object->key = key;
                        key = NULL;
                        object->name = object->key + name_offset;
                        object->location = location;
                        object->status = WCS_ACT_LOAD;

                        if (location == NULL)
                            err = WCSPerformPath(0, NULL, object->name, &object->path);
                        else
                            err = WCSPerformPath(location->type, location->path, object->name, &object->path);

                        if (err == GSTAT_OK)
                            err = WCSFileExist(object->path, &object->exist);

                        if (err == GSTAT_OK)
                        {
                            *status = WCS_ACT_LOAD;
                            err = DDFputhash(files, object->key, HASH_ALL, (PTR) object);
                        }
                        /*
                        ** Only free object if it is being created.
                        */
                        if (err != GSTAT_OK)
                        {
                            if (object != NULL)
                                MEfree((PTR) object->key);
                            MEfree((PTR)object);
                            *info = NULL;
                        }
                    }
                }
                else
                {
                    /*
                    ** Key used to find this object is not needed as there is 
                    ** already one in the key field of the object.
                    ** Moved free of search key here if object already exists.
                    */
                    MEfree((PTR)key);
                    switch (object->status)
                    {
                    case WCS_ACT_DELETE:
                            object->status = WCS_ACT_LOAD;
                            *status = WCS_ACT_LOAD;
                            break;
                    case WCS_ACT_LOAD:
                            *status = WCS_ACT_WAIT;
                            break;
                    default:;
                        *status = WCS_ACT_AVAILABLE;
                    }
                }
            }
            CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
        }
    }
    /*
    ** Assign output if successful
    */
    if (err == GSTAT_OK)
    {
        *info = object;
    }
    return(err);
}

/*
** Name: WCSFileLoaded() - Precise the file is loaded
**
** Description:
**
** Inputs:
**	WCS_FILE_INFO*	: info
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
**      08-Jun-1999 (fanra01)
**          Semaphore protect the reading from the hash table.
*/
GSTATUS 
WCSFileLoaded(
    WCS_FILE_INFO   *info,
    bool            status)
{
    GSTATUS         err = GSTAT_OK;
    WCS_FILE_INFO   *object = NULL;

    G_ASSERT(files == NULL, E_WS0004_WCS_MEM_NOT_OPEN);
    G_ASSERT(info == NULL, E_WS0047_BAD_FILE_NAME);

    if ((err = DDFSemOpen( &WCSFileSem, TRUE )) == GSTAT_OK)
    {
        err = DDFgethash(files, info->key, HASH_ALL, (PTR*) &object);
        CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
    }
    if (err == GSTAT_OK && object != NULL)
        object->status = (status == TRUE) ? WCS_ACT_AVAILABLE : WCS_ACT_DELETE;
    return(GSTAT_OK);
}

/*
** Name: WCSFileRelease() - Release a request
**
** Description:
**	if the number of request is 0 and the file didn't exist at the first
**	request the file will be deleted.
**
** Inputs:
**	WCS_FILE_INFO*	: info
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
**          Add removal of the file info object instance for ima.
**      08-Jun-1999 (fanra01)
**          Semaphore protect the hash enrty removal.
*/
GSTATUS 
WCSFileRelease(
    WCS_FILE_INFO   *info)
{
    GSTATUS err = GSTAT_OK;

    G_ASSERT(files == NULL, E_WS0004_WCS_MEM_NOT_OPEN);
    G_ASSERT(info == NULL, E_WS0047_BAD_FILE_NAME);

    if (err == GSTAT_OK && info->counter > 0)
        info->counter--;

    if (err == GSTAT_OK && info->counter == 0)
    {
        if (info->exist == FALSE)
        {
            i4  flagword;
            LOINFORMATION loinf;
            LOCATION loc;
            LOfroms(PATH & FILENAME, info->path, &loc);

            flagword = (LO_I_TYPE);
            if ((LOinfo(&loc, &flagword, &loinf) == OK) && loinf.li_type == LO_IS_FILE)
                LOdelete(&loc);
        }
        if ((err = DDFSemOpen( &WCSFileSem, TRUE )) == GSTAT_OK)
        {
            err = DDFdelhash(files, info->key, HASH_ALL, (PTR*) &info);
            CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
        }
        if (info != NULL)
        {
            MEfree((PTR)info->key);
            MEfree((PTR)info->path);
            MEfree((PTR)info);
            info = NULL;
        }
    }
    return(err);
}

/*
** Name: WCSOpenCache() - Open cursor to list cache files
**
** Description:
**
** Inputs:
**
** Outputs:
**	PTR*	: cursor
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
WCSOpenCache( PTR *cursor )
{
    GSTATUS err = GSTAT_OK;
    DDFHASHSCAN *sc = NULL;

    err = G_ME_REQ_MEM(0, sc, DDFHASHSCAN, 1);
    if (err == GSTAT_OK)
    {
        DDF_INIT_SCAN(*sc, files);
    }
    *cursor = (PTR) sc;
    return(GSTAT_OK);
}

/*
** Name: WCSGetCache() - fetch
**
** Description:
**
** Inputs:
**	PTR*	: cursor
**
** Outputs:
**	char**	: key
**	char**	: file_name
**	char**	: location_name
**	u_nat*	: request
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
**      08-Jun-1999 (fanra01)
**          Semaphore protrect the reading of the files hash table.
*/
GSTATUS
WCSGetCache(
    PTR     *cursor,
    char*   *key,
    char*   *file_name,
    char*   *location_name,
    u_i4*  *status,
    u_i4*  *count,
    bool*   *exist)
{
    GSTATUS         err = GSTAT_OK;
    DDFHASHSCAN     *sc;
    WCS_FILE_INFO   *file;
    u_i4           length;

    sc = (DDFHASHSCAN*) *cursor;

    if ((err = DDFSemOpen( &WCSFileSem, TRUE )) == GSTAT_OK)
    {
        if (DDF_SCAN(*sc, file) == TRUE && file != NULL)
        {
            length = STlength(file->key) + 1;
            err = GAlloc((PTR*)key, length, FALSE);
            if (err == GSTAT_OK)
            {
                MECOPY_VAR_MACRO(file->key, length, *key);
                length = STlength(file->name) + 1;
                err = GAlloc((PTR*)file_name, length, FALSE);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(file->name, length, *file_name);
                    length = STlength(file->location->name) + 1;
                    err = GAlloc((PTR*)location_name, length, FALSE);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(file->location->name, length,
                            *location_name);
                        if (err == GSTAT_OK)
                        {
                            err = GAlloc((PTR*)status, sizeof(file->status),
                                FALSE);
                        }
                        if (err == GSTAT_OK)
                        {
                            **status = file->status;
                            if (err == GSTAT_OK)
                            {
                                err = GAlloc((PTR*)status,
                                    sizeof(file->counter), FALSE);
                            }
                            if (err == GSTAT_OK)
                            {
                                **count = file->counter;
                                if (err == GSTAT_OK)
                                {
                                    err = GAlloc((PTR*)status,
                                    sizeof(file->exist), FALSE);
                                }
                                if (err == GSTAT_OK)
                                    **exist = file->exist;
                            }
                        }
                    }
                }
            }
        }
        CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
    }

    if (err != GSTAT_OK || file == NULL)
    {
        MEfree((PTR)sc);
        *cursor = NULL;
    }
    return(err);
}

/*
** Name: WCSCloseCache() - Close
**
** Description:
**
** Inputs:
**	PTR*	: cursor
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
WCSCloseCache(
	PTR *cursor)
{
	if (*cursor != NULL)
	{
		DDFHASHSCAN *sc;
		sc = (DDFHASHSCAN*) *cursor;
		MEfree((PTR)sc);
		*cursor = NULL;
	}
return(GSTAT_OK);
}

/*
** Name: WCSFileInitialize() - 
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
**      06-Jul-98 (fanra01)
**          Add file information ima object.
**      08-Jun-1999 (fanra01)
**          Create semaphore to protect the file cache hash table.
*/
GSTATUS 
WCSFileInitialize () 
{
    GSTATUS err = GSTAT_OK;
    char*    size= NULL;
    char    szConfig[CONF_STR_SIZE];
    i4  numberOfFile = 50;

    wcs_mem_file_tag = MEgettag();

    STprintf (szConfig, CONF_FILE_SIZE, PMhost());
    if (PMget( szConfig, &size ) == OK && size != NULL)
        CVan(size, &numberOfFile);

    if ((err = DDFSemCreate(&WCSFileSem, "WCSFiles")) == GSTAT_OK)
    {
        err = DDFmkhash(numberOfFile, &files);
    }
    return(err);
}

/*
** Name: WCSFileTerminate() - 
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
**      08-Jun-1999 (fanra01)
**          Sempahore protect the release of entries in the file hash table.
**          Add free of file semaphore.
**      04-Jan-2000 (fanra01)
**          Bug 99923
**          Changed scan loop to release the semaphore after each iteration.
**          The WCSFileRelease function will reaquire it.
*/
GSTATUS 
WCSFileTerminate () 
{
    GSTATUS         err = GSTAT_OK;
    DDFHASHSCAN     sc;
    WCS_FILE_INFO   *object = NULL;

    DDF_INIT_SCAN(sc, files);
    do
    {
        if ((err = DDFSemOpen( &WCSFileSem, TRUE )) == GSTAT_OK)
        {
            if (DDF_SCAN(sc, object) == TRUE && object != NULL)
            {
                /*
                ** Release the semaphore as WCSFileRelease will reaquire it
                */
                CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
                CLEAR_STATUS(WCSFileRelease(object));
            }
            else
            {
                object = NULL;
                CLEAR_STATUS(DDFSemClose( &WCSFileSem ));
            }
        }
    }
    while( object != NULL );
    CLEAR_STATUS (DDFrmhash(&files));
    CLEAR_STATUS (DDFSemDestroy(&WCSFileSem));
    MEfreetag(wcs_mem_file_tag);
    return(err);
}
