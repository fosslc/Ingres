/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wpsfile.h>
#include <wcs.h>
#include <dds.h>
#include <wsf.h>
#include <erwsf.h>
/*
**
**  Name: wsmfile.c - Requested file for active and user sessions
**
**  Description:
**		the goal of these functions is to memorize every file opened
**		by a session. Like than, it will be possible to release then
**		later.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      01-Oct-1998 (fanra01)
**          Change location request type to 0 (don't care).
**          WCSRequest has been changed to force to WCS_HTTP_LOCATION when
**          returning a facet.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add additional scheme parameter to WPSPerformURL.
**          Add additional scheme parameter to WPSRequest.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
static DDFHASHTABLE		wpsfiles = NULL;
static WPS_PFILE			oldest = NULL; 
static WPS_PFILE			youngest = NULL; 
GLOBALDEF SEMAPHORE		requested_pages_Semaphore;
GLOBALDEF i4			wps_timeout = 600;

STATUS wps_cache_define (void);
STATUS wps_cache_attach (WPS_PFILE wps_file);
VOID wps_cache_detach (WPS_PFILE wps_file);

/*
** Name: WPSFileRemoveTimeout() - Remove the session from the timeout list
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
WPSFileRemoveTimeout(
	WPS_PFILE file)
{
	GSTATUS err = GSTAT_OK;

	if (file == oldest)
	{
		oldest = file->next;
		if (oldest != NULL)
			oldest->previous = NULL;
	}
	else if (file->previous != NULL)
		file->previous->next = file->next;

	if (file== youngest)
	{
		youngest = file->previous;
		if (youngest != NULL)
			youngest->next = NULL;
	}
	else if (file->next != NULL)
		file->next->previous = file->previous;

	file->next = NULL;
	file->previous = NULL;
return(err);
}

/*
** Name: WPSFileSetTimeout() - Set the timeout system
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
WPSFileSetTimeout(
	WPS_PFILE	file,
	i4		time) 
{
	GSTATUS err = GSTAT_OK;

	file->timeout_end = time;
	if (youngest == NULL)
	{
		youngest = file;
		youngest->next = NULL;
		oldest = file;
		oldest->previous = NULL;
	}
	else
	{
		if (time == 0)
		{
			youngest->next = file;
			file->previous = youngest;
			youngest = file;
			youngest->next = NULL;
		}
		else
		{
			WPS_PFILE tmp = youngest;
			while (tmp != NULL && 
						 (tmp->timeout_end == 0 || 
						  tmp->timeout_end > time))
				tmp = tmp->previous;

			if (tmp == NULL)
			{
				oldest->previous = file;
				file->next = oldest;
				oldest = file;
				oldest->previous = NULL;
			}
			else
			{
				if (tmp == youngest)
				{
					youngest->next = file;
					file->previous = youngest;
					youngest = file;
					youngest->next = NULL;
				}
				else
				{
					file->next = tmp->next;
					tmp->next->previous = file;
				}
				tmp->next = file;
				file->previous = tmp;
			}
		}
	}
return(err);
}

/*
** Name: WPSPerformURL() - Perform a url name from his 
**						   host, location, file and suffix name
**
** Description:
**	That function will allocate memory if there is a need
**
** Inputs:
**	char*			: host name
**	WCS_LOCATION	: location information
**	char*			: file name
**	char*			: suffix name
**
** Outputs:
**	char*			: generated url
**
** Returns:
**	GSTATUS	:	E_DF0001_OUT_OF_MEMROY
**				GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSPerformURL.  Use scheme when
**          creating a url.
*/

GSTATUS
WPSPerformURL(
    char*           scheme,
    char*           host,
    char*           port,
    WCS_LOCATION    *location,
    char*           filename,
    char*           *url)
{
    GSTATUS err = GSTAT_OK;
    char*   pszRelPath = NULL;
    u_i4    lenURL = 0;

    if (location != NULL)
    {
        if (location->type & WCS_HTTP_LOCATION)
            pszRelPath = location->relativePath;
        else
            return (GSTAT_OK);
    }
    if (host != NULL && host[0] != EOS)
    {
        /*
        ** Add 3 characters to compensate for the :// in the url following
        ** scheme.  Add 1 character for end-of-string
        ** http://machine \0
        */
        lenURL += STlength( scheme ) + STlength( host ) + 3 + 1;
    }
    if (port != NULL && port[0] != EOS)
    {
        /*
        ** Add extra char to adjust for :portnum in url
        */
        lenURL += STlength (port) + sizeof (char);
    }
    if (pszRelPath != NULL && pszRelPath[0] != EOS)
    {
        lenURL += STlength (pszRelPath) + sizeof (char);
    }
    lenURL +=  STlength (filename) + 2 * sizeof (char);

    err    = GAlloc((PTR*)url, lenURL, FALSE);

    if (err == GSTAT_OK)
    {
        (*url)[0] = EOS;
        if (host != NULL && host[0] != EOS)
            STprintf(*url, "%s://%s",
                            scheme,
                            host);

        if (port != NULL && port[0] != EOS)
            STprintf(*url, "%s%s%s",
                            *url,
                            STR_COLON,
                            port);

        if (pszRelPath != NULL && pszRelPath[0] != EOS)
            STprintf(*url, "%s%c%s",
                            *url,
                            CHAR_URL_SEPARATOR,
                            pszRelPath);

        STprintf(*url, "%s%c%s",
                            *url,
                            CHAR_URL_SEPARATOR,
                            filename);

        pszRelPath = *url;
        while (pszRelPath[0] != EOS)
        {
            if (pszRelPath[0] == CHAR_PATH_SEPARATOR)
                pszRelPath[0] = CHAR_URL_SEPARATOR;
            pszRelPath++;
        }
    }
    return(err);
}

/*
** Name: WPSRequest() - 
**
** Description:
**
** Inputs:
**	ACT_PSESSION : session,
**	char*		 : location name
**	u_i4		 : unit id
**	char*		 : file name
**	char*		 : file suffix
**
** Outputs:
**	WCS_PFILE*	 : file info
**	bool*		 : file status
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
**          Change location request type to 0 (don't care).
**          WCSRequest has been changed to force to WCS_HTTP_LOCATION when
**          returning a facet.
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter and use scheme in call to
**          WPSPerformURL.
*/
GSTATUS
WPSRequest (
    char*       key,
    char*       scheme,
    char*       host,
    char*       port,
    char*       location_name,
    u_i4       unit,
    char*       name,
    char*       suffix,
    WPS_PFILE   *cache,
    bool        *status)
{
    GSTATUS     err = GSTAT_OK;
    WCS_PFILE   file = NULL;
    WPS_PFILE   tmp = NULL;
    char*       index = NULL;
    u_i4       length;
    bool        existed = TRUE;

    err = DDFSemOpen(&requested_pages_Semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err= WCSRequest(0, location_name, unit, name, suffix, &file, status);

        if (err == GSTAT_OK && file != NULL)
        {
            err = G_ME_REQ_MEM(0, index, char,
                STlength(file->info->key) + ((key) ? STlength(key) : 0) + 2);
            if (err == GSTAT_OK)
            {
                STprintf (index, "%s%c%s", file->info->key,
                    CHAR_PATH_SEPARATOR, (key) ? key : "");
                err = DDFgethash(wpsfiles, index, HASH_ALL, (PTR*) &tmp);
                if (err == GSTAT_OK)
                    if (tmp == NULL)
                    {
                        err = G_ME_REQ_MEM(0, tmp, WPS_FILE, 1);
                        if (err == GSTAT_OK)
                        {
                            length = STlength(index) + 1;
                            err = G_ME_REQ_MEM(0, tmp->name, char, length);
                            if (err == GSTAT_OK)
                            {
                                MECOPY_VAR_MACRO(index, length, tmp->name);
                                tmp->key = tmp->name + STlength(file->info->key) + 1;
                                tmp->file = file;
                                err = DDFputhash(wpsfiles, tmp->name, HASH_ALL, (PTR) tmp);
                                if (err == GSTAT_OK)
                                {
                                    wps_cache_attach (tmp);
                                    existed = FALSE;
                                }
                            }
                        }
                    }
                    else
                        err = WPSFileRemoveTimeout(tmp);

                if (err == GSTAT_OK)
                    err = WPSFileSetTimeout(tmp, 0);
                MEfree((PTR)index);
            }
            if (err == GSTAT_OK)
            {
                tmp->used++;
                tmp->count++;
                err = WPSPerformURL( scheme, host, port,
                    file->info->location,
                    file->info->name,
                    &(tmp->url) );
            }

            if (err != GSTAT_OK || existed == TRUE)
            {
                CLEAR_STATUS(WCSRelease(&file));
            }
        }
        CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
    }

    if (err == GSTAT_OK)
        *cache = tmp;
    else
    {
        if (tmp != NULL)
        {
            MEfree((PTR)tmp->name);
            MEfree((PTR)tmp);
        }
        *cache = NULL;
    }
return(err);
}

/*
** Name: WPSRelease() - 
**
** Description:
**	Release all requested files
**
** Inputs:
**	char*		 : key
**	u_i4		 : level (WSM_ACTIVE or/and WSM_USER)
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
WPSRelease (	
	WPS_PFILE	*file) 
{
	GSTATUS		err = GSTAT_OK;
	WPS_PFILE	tmp = *file;

	*file = NULL;
	err = DDFSemOpen(&requested_pages_Semaphore, TRUE);
	if (err == GSTAT_OK)
	{
		tmp->used--;

		err = WPSFileRemoveTimeout(tmp);
		if (err == GSTAT_OK)
			err = WPSFileSetTimeout(tmp, TMsecs() + wps_timeout);
		CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
	}
return(err);
}

/*
** Name: WPSDeleteFile() - 
**
** Description:
**	Release all requested files
**
** Inputs:
**	char*		 : key
**	u_i4		 : level (WSM_ACTIVE or/and WSM_USER)
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
**      29-Jun-04 (gilta01)
**			Inserted a control structure so that memory is
**			freed only if the value of the variable != NULL.
**			Previously MEfree was being called regardless
**			of whether or not the variable contained a value.
**			(b112589)
*/
GSTATUS
WPSDeleteFile (	
	WPS_PFILE	*file) 
{
	GSTATUS err = GSTAT_OK;
	WPS_PFILE tmp = *file;
	WPS_PFILE tmp1 = NULL;
	*file = NULL;

	if (tmp != NULL && tmp->used == 0)
	{
		err = WPSFileRemoveTimeout(tmp);
		if (err == GSTAT_OK)
			err = DDFdelhash(wpsfiles, tmp->name, HASH_ALL, (PTR*) &tmp1);
		if (err == GSTAT_OK)
		{
			wps_cache_detach (tmp);
			err = WCSRelease(&(tmp->file));
		}
        if (tmp->name)
        {
            MEfree((PTR) tmp->name);
        }
        if (tmp->url)
        {
            MEfree((PTR) tmp->url);
        }
		MEfree((PTR) tmp);
	}
return(err);
}

/*
** Name: WPSCleanFilesTimeout() - 
**
** Description:
**	Release all requested files
**
** Inputs:
**	char*		 : key
**	u_i4		 : level (WSM_ACTIVE or/and WSM_USER)
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
WPSCleanFilesTimeout (	
	char*	key) 
{
	GSTATUS err = GSTAT_OK;
	WPS_PFILE tmp = oldest;
	WPS_PFILE next = NULL;
	i4 limit = TMsecs();

	err = DDFSemOpen(&requested_pages_Semaphore, TRUE);
	if (err == GSTAT_OK)
	{
		while (err == GSTAT_OK &&
			   tmp != NULL)
		{
			next = tmp->next;
			if (tmp->timeout_end < limit ||
				(key != NULL && STcompare(key, tmp->key) == 0))
				err = WPSDeleteFile(&tmp);
			else
				if (key == NULL)
					break;
			tmp = next;
		}
		CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
	}
return(err);
}

/*
** Name: WPSBrowseFiles() - 
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
WPSBrowseFiles(
	PTR		*cursor,
	WPS_PFILE *file)
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&requested_pages_Semaphore, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) oldest;
			*file = oldest;
		}
	}
	else
	{
		*file = (WPS_PFILE) *cursor;
		*file = (*file)->next;
		*cursor = (PTR) *file;
	}

	if (*file == NULL)
		CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
return(err);
}

/*
** Name: WPSRemoveFile() - 
**
** Description:
**	Release all requested files
**
** Inputs:
**	char*		 : key
**	u_i4		 : level (WSM_ACTIVE or/and WSM_USER)
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
WPSRemoveFile (	
	WPS_PFILE *file,
	bool	  checkAdd) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&requested_pages_Semaphore, TRUE);
	if (err == GSTAT_OK)
	{
		if (checkAdd == TRUE)
		{
			WPS_PFILE tmp = oldest;
			while (tmp != NULL && tmp != *file)
				tmp = tmp->next;
			if (tmp == NULL)
				err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
		}
	
		if (err == GSTAT_OK)
			err = WPSDeleteFile(file);

		CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
	}
return(err);
}

/*
** Name: WPSRefreshFile() - 
**
** Description:
**	Release all requested files
**
** Inputs:
**	char*		 : key
**	u_i4		 : level (WSM_ACTIVE or/and WSM_USER)
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
WPSRefreshFile (	
	WPS_PFILE file,
	bool	  checkAdd) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&requested_pages_Semaphore, TRUE);
	if (err == GSTAT_OK)
	{
		if (checkAdd == TRUE)
		{
			WPS_PFILE tmp = oldest;
			while (tmp != NULL && tmp != file)
				tmp = tmp->next;
			if (tmp == NULL)
				err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
		}

		if (err == GSTAT_OK)
			err = WCSLoaded(file->file, FALSE);

		CLEAR_STATUS(DDFSemClose(&requested_pages_Semaphore));
	}
return(err);
}

/*
** Name: WPSFileInitialize() - 
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
*/
GSTATUS 
WPSFileInitialize () 
{
	GSTATUS err = GSTAT_OK;
	char*	size= NULL;
	char	szConfig[CONF_STR_SIZE];
	i4	numberOfFile	= 50;

	STprintf (szConfig, CONF_FILE_SIZE, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &numberOfFile);

	STprintf (szConfig, CONF_WPS_TIMEOUT, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &wps_timeout);

	err = DDFmkhash(numberOfFile, &wpsfiles);
  wps_cache_define ();
return(err);
}

/*
** Name: WPSFileTerminate() - 
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
WPSFileTerminate ()
{
	GSTATUS err = GSTAT_OK;
	DDFHASHSCAN sc;
	WPS_PFILE *object = NULL;

	DDF_INIT_SCAN(sc, wpsfiles);
	while (DDF_SCAN(sc, object) == TRUE && object != NULL)
		CLEAR_STATUS(WPSDeleteFile(object));

	CLEAR_STATUS (DDFrmhash(&wpsfiles));
return(err);
}
