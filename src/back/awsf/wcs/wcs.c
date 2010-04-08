/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <compat.h>
#include <cm.h>
#include <mh.h>
#include <wcs.h>
#include <wcslocation.h>
#include <wcsfile.h>
#include <wcsdoc.h>
#include <wcsunit.h>
#include <ddfsem.h>
#include <erwsf.h>
#include <wsf.h>
#include <servermeth.h>
#include <dds.h>

#include <asct.h>

/*
**
**  Name: wcs.c - Web Cache Service
**
**  Description:
**	This service is in charge of the management about everything in relation
**	with the hard storage system (documents, files, locations, ...).
**	Through this module ICE improves performance concerning 
**	file accesses. In fact, that module garantees the unicity of a document 
**	onto the file system.
**	WCS allows the programmer to get temporary file with an unique name.
**
**  History:
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      01-Oct-1998 (fanra01)
**          Force the location to be a HTTP location if the file requested is
**          a facet.
**      15-Mar-99 (fanra01)
**          Add trace messages for initialisation.
**      29-Mar-1999 (fanra01)
**          Reordered the setup of wcs_extract_descr so that pre cached files
**          may be extracted in WCSInitialize.
**	27-Apr-1999 (ahaal01)
**	    Moved "include mh.h" before "include wcs.h".  The check for
**	    "ifdef EXCONTINUE" in wcs.h does no good if sys/context.h
**	    (via mh.h) is included after wcs.h.  Also added "include compat.h"
**	    before "include mh.h" to resolve errors caused by having mh.h
**	    included before wcs.h.
**      07-Jul-1999 (fanra01)
**          Bug 97836
**          Add check for file cache to determine if the file should be
**          released.  Permanent cache should not be removed.
**      30-Dec-1999 (fanra01)
**          Bug 99920
**          Modified WCSDisptachName to return a location of multiple
**          directories in a 2.0 url.
**      02-Mar-2000 (chika01)
**          Bug 100682 
**          Update WCSDisptachName to scan case where session group is 
**          included in the business unit.
**      25-Jul-2000 (fanra01)
**          Bug 102168
**          Add automatic setting of public http locations via request for
**          2.0 compatibility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      20-Mar-2001 (fanra01)
**          Correct function spelling for WCSDispatchName.
**	11-oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGKeySelect(), DDGExecute()
**	    and DDGDelete() such that we pass the addresses of int and float
**	    arguments.
**      23-Jan-2003 (hweho01)
**          For hybrid build 32/64 on AIX, the 64-bit shared lib   
**          oiddi needs to have suffix '64' to reside in the   
**          the same location with the 32-bit one, due to the alternate 
**          shared lib path is not available in the current OS.
**      20-Aug-2003 (fanra01)
**          Bug 110747
**          Add initialization of file pointer reference after it has been 
**          freed
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

GLOBALREF DDG_DESCR		wsf_descr;
GLOBALREF DDG_SESSION	wsf_session;	/* repository connection */
GLOBALDEF i4			request_file_timeout = 600;
static	  SEMAPHORE		wcs_semaphore;	/* garantees integrity of wcs location infos */
static	  SEMAPHORE		wcs_loc_semaphore;	/* garantees integrity of wcs location infos */
static	  SEMAPHORE		wcs_doc_semaphore;	/* garantees integrity of wcs document infos */
static	  SEMAPHORE		wcs_unit_semaphore;	/* garantees integrity of wcs unit infos */
static	  u_i4				generated_name = 0; /* counter for generated name */
static		DDG_DESCR		wcs_extract_descr;

/*
** Name: WCSInitialize() - Initialization of the WCS module
**
** Description:
**	Intialize variables and semaphore.
**	Load repository information into the memory.
**
** Inputs:
**
** Outputs:
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
**      29-Mar-1999 (fanra01)
**          Reordered the setup of wcs_extract_descr so that pre cached files
**          may be extracted.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

GSTATUS
WCSInitialize ()
{
    GSTATUS err = GSTAT_OK;

    bool            exist = FALSE;
    u_i4           *id    = NULL;
    u_i4           *unit  = NULL;
    char            *name = NULL;
    u_i4           *type = NULL;
    i4         *flag = NULL;
    char            *path = NULL;
    char            *extensions= NULL;
    char            *suffix    = NULL;
    u_i4           *owner = NULL;
    u_i4           *ext_loc = NULL;
    char            *ext_suffix = NULL;
    WCS_FILE        *tmp = NULL;
    WCS_LOCATION    *location = NULL;
    char*           filename = NULL;
    u_i4           status;
    char            szConfig[CONF_STR_SIZE];

    err = WCSLocInitialize();
    if (err == GSTAT_OK)
        err = WCSFileInitialize();
    if (err == GSTAT_OK)
        err = WCSDocInitialize();
    if (err == GSTAT_OK)
        err = WCSUnitInitialize();
    if (err == GSTAT_OK)
        err = DDFSemCreate(&wcs_semaphore, "WCS");
    if (err == GSTAT_OK)
        err = DDFSemCreate(&wcs_loc_semaphore, "WCS for locations");
    if (err == GSTAT_OK)
        err = DDFSemCreate(&wcs_doc_semaphore, "WCS for documents");
    if (err == GSTAT_OK)
        err = DDFSemCreate(&wcs_unit_semaphore, "WCS for units");

    /*
    ** Initialise extract database connection before we attempt to load
    ** documents.  As pre-cached documents will need it.
    */
    if (err == GSTAT_OK)
    {
        char*    dictionary_name = NULL;
        char*    node = NULL;
        char*    svrClass = NULL;
        char*    dllname = NULL;
        char*    hostname = NULL;
        char*    size = NULL;
        hostname = PMhost();
        CVlower(hostname);

        STprintf (szConfig, CONF_REPOSITORY, hostname);
        if (PMget( szConfig, &dictionary_name ) != OK && dictionary_name == NULL)
            err = DDFStatusAlloc(E_WS0042_UNDEF_REPOSITORY);

#if defined(any_aix) && defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
        STprintf (szConfig, CONF_DLL64_NAME, hostname);
#else
        STprintf (szConfig, CONF_DLL_NAME, hostname);
#endif  /* aix */
        if (err == GSTAT_OK && PMget( szConfig, &dllname ) != OK && dllname == NULL)
            err = DDFStatusAlloc(E_WS0043_UNDEF_DLL_NAME);

        STprintf (szConfig, CONF_NODE, hostname);
        if (err == GSTAT_OK)
            PMget( szConfig, &node );

        STprintf (szConfig, CONF_CLASS, hostname);
        if (err == GSTAT_OK)
            PMget( szConfig, &svrClass );

        STprintf (szConfig, CONF_WCS_TIMEOUT, hostname);
        if (err == GSTAT_OK && PMget( szConfig, &size ) == OK && size != NULL)
            CVan(size, &request_file_timeout);

        if (err == GSTAT_OK)
            err = DDGInitialize(2, dllname, node, dictionary_name, svrClass, NULL, NULL, &wcs_extract_descr);
    }

    if (err == GSTAT_OK)
    {
        /* loading repository information about LOCATION, UNIT and DOCUMENT */
        err = DDGSelectAll(&wsf_session, WCS_OBJ_LOCATION);
        while (err == GSTAT_OK)
        {
            err = DDGNext(&wsf_session, &exist);
            if (err != GSTAT_OK || exist == FALSE)
                break;
            err = DDGProperties( &wsf_session,
                "%d%s%d%s%s",
                &id,
                &name,
                &type,
                &path,
                &extensions);

            if (err == GSTAT_OK)
                err = WCSLocAdd(*id, name, *type, path, extensions);
            asct_trace ( ASCT_INIT )( ASCT_TRACE_PARAMS,
                "Location add id=%d name=%s type=%v path=%s ext=%s error=%08x",
                *id, name, "PUBLIC,HTTP,ICE", *type, path, extensions, err);
        }
        CLEAR_STATUS(DDGClose(&wsf_session))
    }

    if (err == GSTAT_OK)
    {
        err = DDGSelectAll(&wsf_session, WCS_OBJ_UNIT);
        while (err == GSTAT_OK)
        {
            err = DDGNext(&wsf_session, &exist);
            if (err != GSTAT_OK || exist == FALSE)
                break;
            err = DDGProperties(&wsf_session, "%d%s%d", &id, &name, &owner);
            if (err == GSTAT_OK)
                err = WCSUnitAdd(*id, name, *owner);
            asct_trace ( ASCT_INIT )( ASCT_TRACE_PARAMS,
                "Unit add id=%d name=%s ownerid=%d error=%08x",
                *id, name, *owner, err);
        }
        CLEAR_STATUS(DDGClose(&wsf_session))
    }

    if (err == GSTAT_OK)
    {
        err = DDGSelectAll(&wsf_session, WCS_OBJ_UNIT_LOC);
        while (err == GSTAT_OK)
        {
            err = DDGNext(&wsf_session, &exist);
            if (err != GSTAT_OK || exist == FALSE)
                break;
            err = DDGProperties(&wsf_session, "%d%d", &unit, &id);
            if (err == GSTAT_OK)
                err = WCSMemUnitLocAdd(*unit, *id);
            asct_trace ( ASCT_INIT )( ASCT_TRACE_PARAMS,
                "Unit id=%d locationid=%d error=%08x",
                *unit, *id, err);
        }
        CLEAR_STATUS(DDGClose(&wsf_session))
    }

    if (err == GSTAT_OK)
    {
        err = DDGSelectAll(&wsf_session, WCS_OBJ_DOCUMENT);
        while (err == GSTAT_OK)
        {
            err = DDGNext(&wsf_session, &exist);
            if (err != GSTAT_OK || exist == FALSE)
                break;
            err = DDGProperties( &wsf_session,
                "%d%d%s%s%d%d%d%s%s",
                &id,
                &unit,
                &name,
                &suffix,
                &flag,
                &owner,
                &ext_loc,
                &filename,
                &ext_suffix);

            if (err == GSTAT_OK)
                err = WCSDocAdd(        *id,
                    *unit,
                    name,
                    suffix,
                    *flag,
                    *owner,
                    *ext_loc,
                    filename,
                    ext_suffix);
            asct_trace ( ASCT_INIT )( ASCT_TRACE_PARAMS,
                "Document add id=%d unit=%d name=%s%c%s flag=%v owner=%d extloc=%d file=%s%c%s error=%08x",
                *id, *unit, name, (suffix && *suffix)? '.' : '\0',
                (suffix && *suffix) ? suffix : "",
                "PUBLIC,PRE  ,PERM ,SESS ,EXT  ,PAGE ,FACET", *flag, *owner,
                *ext_loc,
                (filename && *filename) ? filename : "",
                (ext_suffix && *ext_suffix) ? '.' : '\0',
                (ext_suffix && *ext_suffix) ? ext_suffix : "", err);

            if (err == GSTAT_OK)
            {    /* request a file if the document has to be pre-cached */
                if (err == GSTAT_OK &&
                     ((*flag) & WCS_PRE_CACHE))
                {
                    err = WCSPerformFileName(*unit, *id, &filename);
                    if (err == GSTAT_OK)
                        err = G_ME_REQ_MEM(0, tmp, WCS_FILE, 1);

                    if (err == GSTAT_OK)
                    {
                        if (err == GSTAT_OK)
                            if (*flag & WSF_PAGE)
                                err = WCSUnitLocFind (0, suffix, *unit, &location);
                            else
                                err = WCSUnitLocFind (WCS_HTTP_LOCATION,
                                                                            suffix,
                                                                            *unit,
                                                                            &location);

                        if (err == GSTAT_OK)
                        {
                            tmp->doc_id = *id;
                            err = WCSFileRequest(
                                    location,
                                    filename,
                                    suffix,
                                    &tmp->info,
                                    &status);

                            if (err == GSTAT_OK)
                            {
                                bool load = FALSE;
                                err = WCSDocSetFile (*id, tmp);

                                if (err == GSTAT_OK)
                                {
                                    LOfroms(PATH & FILENAME, tmp->info->path, &tmp->loc);
                                    if (status == WCS_ACT_LOAD)
                                    {
                                        bool is_external = TRUE;
                                        err = WCSDocCheckFlag (tmp->doc_id, WCS_EXTERNAL, &is_external);

                                        if (err == GSTAT_OK && is_external == FALSE)
                                            err    = WCSExtractDocument (tmp);

                                        if (err == GSTAT_OK)
                                            load = TRUE;
                                    }
                                }

                                if (status == WCS_ACT_LOAD)
                                    CLEAR_STATUS(WCSLoaded(tmp, load));
                            }
                        }
                    }
                }
            }
        }
        CLEAR_STATUS(DDGClose(&wsf_session));
    }

    if (err == GSTAT_OK)
        err = DDGCommit(&wsf_session);
    else
        CLEAR_STATUS(DDGRollback(&wsf_session));

    MEfree((PTR)type);
    MEfree((PTR)flag);
    MEfree((PTR)path);
    MEfree((PTR)name);
    MEfree((PTR)suffix);
    MEfree((PTR)extensions);
    MEfree((PTR)id);
    MEfree((PTR)unit);
    MEfree((PTR)owner);
    MEfree((PTR)filename);
    MEfree((PTR)ext_loc);
    MEfree((PTR)ext_suffix);
return(err);
}

/*
** Name: WCSTerminate() - Terminate of the WCS module
**
** Description:
**	Frees memory and destroys semphore.
**
** Inputs:
**
** Outputs:
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
WCSTerminate() 
{
	CLEAR_STATUS( WCSLocTerminate());
	CLEAR_STATUS( WCSFileTerminate());
	CLEAR_STATUS( WCSDocTerminate());
	CLEAR_STATUS( WCSUnitTerminate());
	CLEAR_STATUS( DDGTerminate(&wcs_extract_descr));
	CLEAR_STATUS( DDFSemDestroy(&wcs_semaphore) );
	CLEAR_STATUS( DDFSemDestroy(&wcs_loc_semaphore) );
	CLEAR_STATUS( DDFSemDestroy(&wcs_doc_semaphore) );
	CLEAR_STATUS( DDFSemDestroy(&wcs_unit_semaphore) );
return(GSTAT_OK);
}

/*
** Name: WCSDispatchName()	- find unit, location, filename and suffix
**
** Description:
**	That method get into a string information about a wcs file.
**	define the ICE file syntaxe.
**
**      e.g
**          /unit[location/name.suffix] or
**          /xyz/unit[location/name.suffix] or
**          /loca/tion/name.suffix      or
**          /name.suffix
**
** Inputs:
**	char *message : source string (file definition)
**
** Outputs:
**	char** : unit name (optional)
**	char** : location name (optional)
**	char** : file name
**	char** : suffix (optional)
**
** Returns:
**	GSTATUS	: GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      30-Dec-1999 (fanra01)
**          Bug 99920
**          Modifed the query parsing to handle locations with multiple
**          directories for ice 2.0 compatibility.
**      02-Mar-2000 (chika01)
**          Add scan case where session group is included in the 
**          business unit.
**      20-Mar-2001 (fanra01)
**          Correct function spelling of WCSDispatchName.
**          
*/

GSTATUS
WCSDispatchName (
    char*    message,
    char*    *unit,
    char*    *location,
    char*    *name,
    char*    *suffix)
{
    GSTATUS err = GSTAT_OK;
    char    urlstr[2];
    char    unitbeg[2];
    char    unitend[2];
    char*   p = NULL;
    char*   start = message;

    *unit = NULL;
    *location = NULL;
    *name = NULL;
    *suffix = NULL;

    urlstr[0] = CHAR_URL_SEPARATOR;
    urlstr[1] = EOS;

    unitbeg[0] = WSS_UNIT_BEGIN;
    unitbeg[1] = EOS;

    unitend[0] = WSS_UNIT_END;
    unitend[1] = EOS;

    if ((message != NULL) && (*message != EOS))
    {
        /*
        ** skip the first delimiter character if there is one
        */
        if (CMcmpnocase( message, urlstr ) == 0)
        {
            CMnext( message );
        }

        /*
        ** if unit is specified
        */
        if ((p = STrindex( message, unitbeg, 0 )) != NULL)
        {
            /*
            ** terminate the unit string, skip EOS and save page name
            */
            *p = EOS;
            CMnext( p );
            *name = p;

            /*
            ** find first url delimiter following unit start delimiter
            */
            if ((p = STindex( *name, urlstr, 0 )) != NULL)
            {
                /*
                ** terminate the location, skip the EOS and save location
                ** update name
                */
                *p = EOS;
                CMnext( p );
                *location = *name;
                *name = p;
            }
            else
            {
                if ((p = STrindex( message, urlstr, 0 )) != NULL)
                {
                    CMnext( p );
                    *unit = p;
                }
                else
                {
                    *unit = message;
                }
            }

            if ((*name != NULL) &&
                ((p = STrindex( *name, unitend, 0 )) != NULL))
            {
                /*
                ** remove terminating unit delimiter
                */
                *p = EOS;
            }
        }
        else
        {
            /*
            ** find first url delimiter preceeding page
            */
            if ((p = STrindex( message, urlstr, 0 )) != NULL)
            {
                /*
                ** terminate the location string, skip the EOS and save page
                */
                *p = EOS;
                CMnext( p );
                *name = p;
            }
            /*
            ** if no delimiter found or found the first one then only have
            ** a page set the name otherwise name should be set and location
            ** is setup
            */
            if ((p != NULL) && (p != start))
            {
                *location = message;
            }
            else
            {
                *name = message;
            }
        }
        if ((p = STrindex( *name, STR_PERIOD, 0 )) != NULL)
        {
            /*
            ** terminate the page string, skip the EOS and save extension
            */
            *p = EOS;
            CMnext( p );
            *suffix = p;
        }

        if (*name == NULL)
        {
            *name = message;
            *suffix = STindex( message, STR_PERIOD, 0);
        }
    }
    return(GSTAT_OK);
}

/*
** Name: WCSPerformFileName() - Perform a document name from his 
**								unit and file id.
**
** Description:
**	That function will allocate memory if there is a need
**
** Inputs:
**	u_i4 : unit id
**	u_i4 : document id
**
** Outputs:
**	char* : generated name
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
*/

GSTATUS
WCSPerformFileName(
	u_i4 unit,
	u_i4 doc, 
	char **name) 
{
	GSTATUS err = GSTAT_OK;
	err = GAlloc((PTR*)name, 20, FALSE);
	if (err == GSTAT_OK)
		STprintf(*name, "%d_%d", unit, doc);
return(err);
}

/*
** Name: WCSFindLocation() - Find the appropriate location
**
** Description:
**	that function return a good location for a specific document
**	and the associated file name
**
**	file. Two cases:
**		1. you furnish the location name
**		2. the location is based on the unit definition
**
** Inputs:
**	u_i4			: unit id
**	char*			: file name
**	char*			: suffix name
**
** Outputs:
**	char*			: associated filename
**	WCS_LOCATION	: location information
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
WCSFindLocation(
	u_i4			loc_type,
	u_i4			unit_id,
	char			*name,
	char			*suffix,
	WCS_LOCATION	**location)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
	if (err == GSTAT_OK)
	{
		if (unit_id != 0 && (name == NULL || name[0] == EOS))
			err = WCSUnitLocFind (loc_type, suffix, unit_id, location);
		else
		{
			err = WCSUnitLocFindFromName (loc_type, name, unit_id, location);
			if (err == GSTAT_OK && *location == NULL)
				err = DDFStatusAlloc(E_WS0072_WCS_BAD_LOCATION);
		}
		CLEAR_STATUS(DDFSemClose(&wcs_loc_semaphore));
	}
return(err);
}

/*
** Name: WCSRequest() - Request a wcs file to use it.
**
** Descr2iption:
**	That function precises to wcs that the caller wants 
**	to use this file and would like information.
**	That file can be 
**		1. a generated file (name == NULL)
**		2. a document
**		3. a filename
** Moreover, that function return the file status (AVAILABLE, LOAD)
** if the status is load the caller has to laod the file (example 
** extract from the database) and then calls WCSLoaded which makes
** available the file for others.
**
** Inputs:
**	u_i4	: location type
**	char*	: location name
**	u_i4	: unit id
**	char*	: file name
**	char*	: suffix name
**
** Outputs:
**	WCS_PFILE*	: file information
**	bool*		: file status. precise if the file has to bas loaded.
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
**      01-Oct-1998 (fanra01)
**          Force the location to be a HTTP location if the file requested is
**          a facet.
**      25-Jul-2000 (fanra01)
**          Add creation of a location specified in a 2.0 mode request.
**          If the location has not been created add it to the repository.
*/

GSTATUS
WCSRequest (
    u_i4        loc_type,
    char        *location_name,
    u_i4        unit_id,
    char        *name,
    char        *suffix,
    WCS_PFILE   *file,
    bool        *status)
{
    GSTATUS err = GSTAT_OK;
    char *filename = NULL;
    u_i4 document_id = SYSTEM_ID;
    u_i4 status1 = WCS_ACT_AVAILABLE;
    u_i4 status2 = WCS_ACT_AVAILABLE;
    bool valid_name = TRUE;
    char *doc_name = NULL;
    WCS_PFILE tmp = NULL;
    WCS_LOCATION *location = NULL;
    i4    preserve = TRUE;

    *file = NULL;

    if (name == NULL || name[0] == EOS)
    {    /* request a generated filename */
        err = G_ME_REQ_MEM(
                   0,
                   filename,
                   char,
                   LO_FPREFIX_MAX + 1);
        if (err == GSTAT_OK)
        {
            STprintf (filename, "%x%d%d", file, generated_name++, TMsecs());
            err = WCSFindLocation(
                      loc_type,
                      unit_id,
                      location_name,
                      suffix,
                      &location);
            preserve = FALSE;
        }
    }
    else
    {
        bool is_facet;
        bool is_external;
        bool has_to_be_cached;
        err = WCSDocGet (unit_id, name, suffix, &document_id);

        if (err == GSTAT_OK)
        {
            if (document_id == SYSTEM_ID)
            {    /* FAT file */
                err = WCSFindLocation(
                              loc_type,
                              unit_id,
                              location_name,
                              suffix,
                              &location);
                if ((err != GSTAT_OK) &&
                    (err->number == E_WS0072_WCS_BAD_LOCATION))
                {
                    i4 id = 0;

                    CLEAR_STATUS( err );
                    err = WCSCreateLocation( &id, location_name,
                        WCS_HTTP_LOCATION | WCS_PUBLIC, location_name,
                        ERx("") );
                }
                if (err == GSTAT_OK)
                    err = G_ST_ALLOC(filename, name);
            }
            else
            {    /* document */
                err = WCSDocCheckFlag( document_id, WSF_FACET, &is_facet );
                if (err == GSTAT_OK && is_facet)
                    loc_type = WCS_HTTP_LOCATION;

                err = WCSDocCheckFlag( document_id, WCS_EXTERNAL,
                    &is_external );
                if (err == GSTAT_OK)
                {
                    if (is_external == FALSE)
                    { /* internal */
                        err = WCSPerformFileName(unit_id, document_id,
                            &filename);
                        if (err == GSTAT_OK)
                        {
                            err = WCSDocCheckFlag (document_id,
                                WCS_PERMANENT_CACHE | WCS_PRE_CACHE,
                                &has_to_be_cached);
                        }
                        if (err == GSTAT_OK)
                            preserve = FALSE;
                    }
                    else
                    { /* external */
                        u_i4 loc_id;
                        has_to_be_cached = FALSE;
                        err = WCSDocGetExtFile (document_id, &loc_id,
                            &filename, &suffix);
                        if (err == GSTAT_OK)
                            err = WCSGrabLoc(loc_id, &location);
                    }
                }
                if (err == GSTAT_OK && has_to_be_cached == TRUE)
                {
                    err = DDFSemOpen(&wcs_semaphore, TRUE);
                    if (err == GSTAT_OK)
                    {    /* get the pre cache file definition */
                        err = WCSDocGetFile (document_id, &tmp);
                        if (err == GSTAT_OK && tmp == NULL)
                        {
                            err = G_ME_REQ_MEM(0, tmp, WCS_FILE, 1);
                            if (err == GSTAT_OK && location == NULL)
                            {
                                err = WCSFindLocation(
                                              loc_type,
                                              unit_id,
                                              location_name,
                                              suffix,
                                              &location);
                            }
                            if (err == GSTAT_OK)
                                err = WCSFileRequest(
                                             location,
                                             filename,
                                             suffix,
                                             &tmp->info,
                                             &status1);

                            if (err == GSTAT_OK)
                                err = WCSDocSetFile (document_id, tmp);
                        }
                        else
                            location = tmp->info->location;

                        CLEAR_STATUS(DDFSemClose(&wcs_semaphore));
                    }
                }

                if (err == GSTAT_OK && location == NULL)
                {
                    err = WCSFindLocation(
                              loc_type,
                              unit_id,
                              location_name,
                              suffix,
                              &location);
                }
            }
        }
    }

    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(0, tmp, WCS_FILE, 1);
        if (err == GSTAT_OK)
        {
            i4 timeout_end = TMsecs() + request_file_timeout;
            do
            {    /* loop while the file is unavailable */
                err = DDFSemOpen(&wcs_semaphore, TRUE);
                if (err == GSTAT_OK)
                {
                    err = WCSFileRequest(location, filename, suffix,
                        &tmp->info, &status2);
                    if (err == GSTAT_OK)
                    if (status1 == WCS_ACT_LOAD)
                        status2 = WCS_ACT_LOAD;
                    CLEAR_STATUS(DDFSemClose(&wcs_semaphore));
                }
            }
            while (err == GSTAT_OK &&
               status2 == WCS_ACT_WAIT &&
               timeout_end > TMsecs());

            if (err == GSTAT_OK && status2 == WCS_ACT_WAIT)
                err = DDFStatusAlloc ( E_WS0091_WCS_TIMEOUT );

            if (err == GSTAT_OK)
            {
                tmp->info->counter++;
                tmp->info->exist = preserve;

                LOfroms(PATH & FILENAME, tmp->info->path, &tmp->loc);
                tmp->doc_id = document_id;
                *file = tmp;
                *status = status2;
            }
            else
                MEfree((PTR)tmp);
        }
    }
    MEfree(filename);
    return(err);
}

/*
** Name: WCSLoaded() - Checked the WCS_FILE loaded
**
** Description:
**
** Inputs:
**	WCS_PFILE	: file
**
** Outputs:
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
WCSLoaded(
	WCS_PFILE	file,
	bool		isIt)
{
	GSTATUS err = GSTAT_OK;
	if (file != NULL && file->info != NULL)
	{
		err = DDFSemOpen(&wcs_semaphore, TRUE);
		if (err == GSTAT_OK)
		{
			err = WCSFileLoaded(file->info, isIt);
			CLEAR_STATUS(DDFSemClose(&wcs_semaphore));
		}
	}
return(err);
}

/*
** Name: WCSDelete() - The file will be deleted as soon as possible
**
** Description:
**
** Inputs:
**	WCS_PFILE	: file
**
** Outputs:
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
WCSDelete(
	WCS_PFILE	file)
{
	GSTATUS err = GSTAT_OK;
	if (file != NULL && file->info != NULL)
		file->info->exist = FALSE;
return(err);
}

/*
** Name: WCSRelease() - Release a file Request
**
** Description:
**	Release file, decrease counter and delete the physical file if necessary
**
** Inputs:
**	WCS_PFILE*	: file
**
** Outputs:
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
**      07-Jul-1999 (fanra01)
**          Bug 97836
**          Add check for file cache to determine if the file should be
**          released.
**      08-Jul-2003 (fanra01)
**          Remove reference to the info object when it has been freed.
*/

GSTATUS 
WCSRelease ( WCS_PFILE *file )
{
    GSTATUS err = GSTAT_OK;
    bool    iscached = FALSE;

    if ((*file) != NULL)
    {
        err = WCSDocCheckFlag( (*file)->doc_id,
            WCS_PERMANENT_CACHE, &iscached );
        if ((err == GSTAT_OK) &&
            (iscached == FALSE) &&
            ((*file)->info != NULL))
        {
            err = DDFSemOpen( &wcs_semaphore, TRUE );
            if (err == GSTAT_OK)
            {
                err = WCSFileRelease( (*file)->info );
                (*file)->info = NULL;
                CLEAR_STATUS(DDFSemClose( &wcs_semaphore ));
            }
        }
        MEfree((PTR)*file);
        *file = NULL;
    }
    return(err);
}

/*
** Name: WCSCreateLocation() - Create a location 
**
** Description:
**	update the repository
**	allocate and declare a new location into the ICE memory
**
** Inputs:
**	char*	: location name
**	u_i4	: location type
**	char*	: physical path
**	char*	: extensions supported by this location
**
** Outputs:
**	u_nat*	: location id;
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
WCSCreateLocation(
	u_i4	*id,
	char	*name,
	u_i4	type,
	char	*path, 
	char	*extensions) 
{
	GSTATUS err = GSTAT_OK;

	if (err == GSTAT_OK) 
		err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = WCSPerformLocationId(id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&wsf_session, 
					WCS_OBJ_LOCATION,
					"%d%s%d%s%s", 
					id, 
					name,
					&type,
					path,
					extensions);

			if (err == GSTAT_OK)
				err = WCSLocAdd(
						*id, 
						name, 
						type, 
						path, 
						extensions);
		}

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_loc_semaphore));
	}
return(err);
}

/*
** Name: WCSUpdateLocation() - Update a location 
**
** Description:
**	update the repository
**	update the ICE memory 
**
** Inputs:
**	u_i4	: location id;
**	char*	: location name
**	u_i4	: location type
**	char*	: physical path
**	char*	: extensions supported by this location
**
** Outputs:
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
WCSUpdateLocation (
	u_i4	id,
	char	*name,
	u_i4	type,
	char	*path, 
	char	*extensions) 
{
	GSTATUS err = GSTAT_OK;

	if (err == GSTAT_OK) 
		err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGUpdate(
				&wsf_session, 
				WCS_OBJ_LOCATION,
				"%s%d%s%s%d", 
				name,
				&type,
				path,
				extensions,
				&id);

		if (err == GSTAT_OK)
			err = WCSLocUpdate(
					id, 
					name,
					type, 
					path, 
					extensions);

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_loc_semaphore));
	}
return(err);
}

/*
** Name: WCSCreateLocation() - Create a location 
**
** Description:
**	update the repository
**	free the ICE memory 
**
** Inputs:
**	u_i4	: location id;
**
** Outputs:
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
WCSDeleteLocation(
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGDelete(
				&wsf_session, 
				WCS_OBJ_LOCATION,
				"%d", 
				&id);

		if (err == GSTAT_OK)
			err = WCSLocDelete(id);	

		if (err == GSTAT_OK)
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_loc_semaphore));
	}
return(err);
}

/*
** Name: Method to list existing locations
**		WCSOpenLocation() - Open a cursor
**		WCSGetLocation() - Fetch the current location
**		WCSCloseLocation() - Close the cursor
**
** Description:
**	these three functions allow the user to list all the
**	declared location. They garantee the integrity of the 
**	information through a semaphore.
*/

/*
** Name: WCSOpenLocation() - Open a cursor
**
** Description:
**
** Inputs:
**	PTR*	: cursor pointer
**
** Outputs:
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
WCSOpenLocation (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_loc_semaphore, FALSE);
return(err);
}

/*
** Name: WCSGetLocation() - Fetch the current location
**
** Description:
**
** Inputs:
**	PTR	: cursor pointer
**
** Outputs:
**	u_nat**	: location id
**	char**	: location name
**	u_nat**	: type of the location (WCS_HTTP_LOCATION, WCS_II_LOCATION)
**	char**	: path
**	char**	: extensions
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
WCSGetLocation (
	PTR		*cursor,
	u_i4	**id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char	**relativePath,
	char	**extensions) 
{
return(WCSLocBrowse(
				cursor, 
				id, 
				name,
				type,
				path,
				relativePath,
				extensions));
}

/*
** Name: WCSCloseLocation() - Close the location cursor
**
** Description:
**
** Inputs:
**	PTR	: cursor pointer
**
** Outputs:
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
WCSCloseLocation (PTR		*cursor) 
{
	CLEAR_STATUS(DDFSemClose(&wcs_loc_semaphore));
return(GSTAT_OK);
}

/*
** Name: WCSRetrieveLocation() - Retrieve location information from its id.
**
** Description:
**	Either the location does not exist
**
** Inputs:
**	PTR	: cursor pointer
**
** Outputs:
**	u_nat**	: location id
**	char**	: location name
**	u_nat**	: type of the location (WCS_HTTP_LOCATION, WCS_II_LOCATION)
**	char**	: path
**	char**	: extensions
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
WCSRetrieveLocation (
	u_i4	id,
	char	**name,
	u_i4	**type,
	char	**path, 
	char	**relativePath,
	char	**extensions) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&wcs_loc_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = WCSGetLocFromId(
							id, 
							name,
							type,
							path,
							relativePath,
							extensions);
		CLEAR_STATUS(DDFSemClose(&wcs_loc_semaphore));
	}
return(err);
}

/*
** Name: WCSCreateDocument() - Create a new document into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_i4 :	unit id
**	char* : document name
**	char* : document suffix
**	u_i4 : document flag (PUBLIC, PRE_CACHE ... )
**	u_i4 :	owner
**	u_i4 :	external location (can be NULL)
**	char* : external file name (can be NULL)
**	char* : external suffix (can be NULL)
**
** Outputs:
**	u_nat* : document id
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
WCSCreateDocument (
	u_i4 *id,
	u_i4 unit,
	char* name,
	char* suffix,
	u_i4 flag,
	u_i4 owner,
	u_i4 ext_loc,
	char* ext_file,
	char* ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	
	err = DDFSemOpen(&wcs_doc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		char *file_path = ext_file;

		err = WCSPerformDocumentId(id);
		if (err == GSTAT_OK)
		{
			if ((flag & WCS_EXTERNAL) == FALSE)
			{
				ext_loc = SYSTEM_ID;
				ext_file = NULL;
				ext_suffix = NULL;
			} 

			err = DDGInsert(
					&wsf_session, 
					WCS_OBJ_DOCUMENT,
					"%d%d%s%s%d%d%d%s%s", 
					id, 
					&unit,
					name,
					suffix,
					&flag,
					&owner,
					&ext_loc,
					ext_file,
					ext_suffix);

			if (err == GSTAT_OK && (flag & WCS_EXTERNAL) == FALSE)
			{
					err = DDGInsert(
						&wsf_session, 
						WCS_OBJ_FILE,
						"%d%l", 
						id,
						file_path);
			}

			if (err == GSTAT_OK)
				err = WCSDocAdd(
						*id, 
						unit, 
						name, 
						suffix, 
						flag, 
						owner, 
						ext_loc,
						ext_file,
						ext_suffix);
		}

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_doc_semaphore));
	}
return(err);
}

/*
** Name: WCSRefresh() - Refresh the file
**
** Description:
**
** Inputs:
**	u_i4	: doc id
**
** Outputs:
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
WCSRefresh (
	u_i4	unit_id,
	char*	name,
	char*	suffix) 
{
    GSTATUS		err = GSTAT_OK;
    WCS_FILE	*file = NULL;
    bool		status;

    err = WCSRequest(	0,
			NULL,
			unit_id,
			name,
			suffix,
			&file,
			&status);
    if (err == GSTAT_OK)
    {
	WCSLoaded(file, FALSE);
	CLEAR_STATUS(WCSRelease(&file));
    }
    return(err);
}

/*
** Name: WCSUpdateDocument() - update a document into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_nat* : document id
**	u_i4 :	unit id
**	char* : document name
**	char* : document suffix
**	u_i4 : document flag (PUBLIC, PRE_CACHE ... )
**	u_i4 :	owner
**	u_i4 :	external location (can be NULL)
**	char* : external file name (can be NULL)
**	char* : external suffix (can be NULL)
**
** Outputs:
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
WCSUpdateDocument (
	u_i4	id,
	u_i4	unit,
	char*	name,
	char*	suffix,
	u_i4	flag,
	u_i4	owner,
	u_i4	ext_loc,
	char*	ext_file,
	char*   ext_suffix) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&wcs_doc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		char *file_path = ext_file;

		if ((flag & WCS_EXTERNAL) == FALSE)
		{
			ext_loc = SYSTEM_ID;
			ext_file = NULL;
			ext_suffix = NULL;
		}

		err = DDGUpdate(
				&wsf_session, 
				WCS_OBJ_DOCUMENT,
				"%d%s%s%d%d%d%s%s%d", 
				&unit,
				name,
				suffix,
				&flag,
				&owner,
				&ext_loc,
				ext_file,
				ext_suffix,
				&id);

		if (err == GSTAT_OK && 
			(flag & WCS_EXTERNAL) == FALSE &&
			file_path != NULL && 
			file_path[0] != EOS)
		{
				bool is_external;
				err = WCSDocCheckFlag (id, WCS_EXTERNAL, &is_external);
				if (err == GSTAT_OK && is_external == FALSE)
					err = DDGDelete(
							&wsf_session, 
							WCS_OBJ_FILE, 
							"%d", 
							&id);

				if (err == GSTAT_OK)
					err = DDGInsert(
						&wsf_session,
						WCS_OBJ_FILE,
						"%d%l", 
						&id,
						file_path);
		}
		if (err == GSTAT_OK)
			err = WCSDocUpdate(
					id, 
					unit, 
					name, 
					suffix, 
					flag, 
					owner, 
					ext_loc,
					ext_file,
					ext_suffix);

		if (err == GSTAT_OK) 
		{
			CLEAR_STATUS(WCSRefresh(unit, name, suffix));
			err = DDGCommit(&wsf_session);
		}
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_doc_semaphore));
	}
return(err);
}

/*
** Name: WCSExtractDocument() - extract the document from the repository
**
** Description:
**	Extract blob information from the repository and put it into the FAT file.
**
** Inputs:
**	WCS_PFILE	file information with document number
**
** Outputs:
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
WCSExtractDocument (
	WCS_PFILE	file)
{
	GSTATUS err = GSTAT_OK;
	DDG_SESSION session = {0};
	char	 *path = NULL;

	if (file->doc_id != SYSTEM_ID)
	{
		err = DDGConnect(&wcs_extract_descr, NULL, NULL, &session);
		if (err == GSTAT_OK)
		{
			err = DDGKeySelect(&session, 0, "%d%l",
					   &file->doc_id, &file->loc);

			if (err == GSTAT_OK)
				err = DDGCommit(&session);
			else
				CLEAR_STATUS(DDGRollback(&session));

			CLEAR_STATUS(DDGDisconnect(&session));
		}
	}
return(err);
}

/*
** Name: WCSRemoveDocument() - delete a document into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_nat* : document id
**
** Outputs:
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
WCSRemoveDocument (
	u_i4	id) 
{
	GSTATUS err = GSTAT_OK;
	char resource[WSF_STR_RESOURCE];
	STprintf(resource, "d%d", id);

	err = DDSDeleteResource(resource);
	if (err == GSTAT_OK)
	{
		err = DDGDelete(
				&wsf_session, 
				WCS_OBJ_DOCUMENT,
				"%d", 
				&id);

		if (err == GSTAT_OK)
		{
			bool is_external;

			err = WCSDocCheckFlag (id,	WCS_EXTERNAL, &is_external);
			if (err == GSTAT_OK && is_external == FALSE)
			{
					err = DDGDelete(
							&wsf_session, 
							WCS_OBJ_FILE, 
							"%d", 
							&id);
			}
		}

		if (err == GSTAT_OK)
			err = WCSDocDelete(id);	

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));
	}
return(err);
}

/*
** Name: WCSDeleteDocument() - delete a document into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_nat* : document id
**
** Outputs:
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
WCSDeleteDocument (
	u_i4	id) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&wcs_doc_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = WCSRemoveDocument(id);
		CLEAR_STATUS( DDFSemClose(&wcs_doc_semaphore));
	}
return(err);
}

/*
** Name: Method to list existing documents
**		WCSOpenDocument() - Open a cursor
**		WCSGetDocument() - Fetch the current location
**		WCSCloseDocument() - Close the cursor
**
** Description:
**	these three functions allow the user to list all the
**	declared declared. They garantee the integrity of the 
**	information through a semaphore.
*/

/*
** Name: WCSOpenDocument() - Open the document list
**
** Description:
**	reserve semaphore to garantee integrity
**
** Inputs:
**	PTR* : cursor
**
** Outputs:
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
WCSOpenDocument (PTR		*cursor) 
{
return(DDFSemOpen(&wcs_doc_semaphore, FALSE));
}

/*
** Name: WCSGetDocument() - Fetch the document
**
** Description:
**
** Inputs:
**	PTR* : cursor
**
** Outputs:
**	u_nat**	: document id
**	u_nat**	: unit id
**	char**	: document name
**	char**	: suffix
**	u_nat**	: flag
**	u_nat**	: owner
**	u_nat**	: ext_loc
**	char**	: ext_file
**	char**	: ext_suffix
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
	char	**ext_suffix) 
{
return(WCSDocBrowse(
			cursor,
			id,
			unit,
			name,
			suffix,
			flag,
			owner,
			ext_loc,
			ext_file,
			ext_suffix));
}

/*
** Name: WCSCloseDocument() - Close the cursor
**
** Description:
**	Release the semaphore
**
** Inputs:
**	PTR* : cursor
**
** Outputs:
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
WCSCloseDocument (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDGClose(&wsf_session);

	if (err == GSTAT_OK)
		err = DDGCommit(&wsf_session);
	else
		CLEAR_STATUS(DDGRollback(&wsf_session));
	CLEAR_STATUS(DDFSemClose(&wcs_doc_semaphore));
return(err);
}

/*
** Name: WCSRetrieveDocument() - Fetch the document
**
** Description:
**
** Inputs:
**	u_i4	: document id
**
** Outputs:
**	u_nat**	: unit id
**	char**	: document name
**	char**	: suffix
**	u_nat**	: flag
**	u_nat**	: owner
**	u_nat**	: ext_loc
**	char**	: ext_file
**	char**	: ext_suffix
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
WCSRetrieveDocument (
	u_i4	id,
	u_i4	**unit,
	char	**name,
	char	**suffix,
	u_i4	**flag,
	u_i4	**owner,
	u_i4	**ext_loc,
	char	**ext_file,
	char	**ext_suffix) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_doc_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = WCSGetDocFromId(
				id,
				unit,
				name,
				suffix,
				flag,
				owner,
				ext_loc,
				ext_file,
				ext_suffix);
		
		CLEAR_STATUS(DDFSemClose(&wcs_doc_semaphore));
	}
return(err);
}

/*
** Name: WCSCreateUnit() - Create a new unit into ICE
**
** Description:
**	Update the repository and the ICE server memory
**
** Inputs:
**	char*	: unit name
**	u_i4	: owner
**
** Outputs:
**	u_nat*	: unit id
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
WCSCreateUnit (
	u_i4 *id,
	char* name,
	u_i4 owner) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = WCSPerformUnitId(id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&wsf_session, 
					WCS_OBJ_UNIT,
					"%d%s%d", 
					id, 
					name,
					&owner);

			if (err == GSTAT_OK)
				err = WCSUnitAdd(*id, name, owner);
		}

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_unit_semaphore));
	}
return(err);
}

/*
** Name: WCSUpdateUnit() - update a unit into ICE
**
** Description:
**	Update the repository and the ICE server memory
**
** Inputs:
**	u_i4	: unit id
**	char*	: unit name
**	u_i4	: owner
**
** Outputs:
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
WCSUpdateUnit (
	u_i4 id,
	char* name,
	u_i4 owner) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGUpdate(
				&wsf_session, 
				WCS_OBJ_UNIT,
				"%s%d%d", 
				name,
				&owner,
				&id);

		if (err == GSTAT_OK)
			err = WCSUnitUpdate(id, name, owner);

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_unit_semaphore) );
	}
return(err);
}

/*
** Name: WCSDeleteUnit() - delete a unit into ICE
**
** Description:
**	Update the repository and the ICE server memory
**
** Inputs:
**	u_i4	: unit id
**
** Outputs:
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
WCSDeleteUnit (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		PTR browser = NULL;
		u_i4 *pid = NULL;
		u_i4 *punit = NULL;

		err = WCSOpenDocument(&browser);
		if (err == GSTAT_OK)
		{
			do
			{
				err = WCSGetDocument(&browser,
															&pid, 
															&punit,
															NULL,NULL,NULL,NULL,NULL,NULL,NULL);
				if (err == GSTAT_OK && 
						browser != NULL &&
						*punit == id)
						err = WCSRemoveDocument(*pid);
			}
			while (err == GSTAT_OK && browser != NULL);
			CLEAR_STATUS (WCSCloseDocument(&browser));
		}
		MEfree((PTR)pid);
		MEfree((PTR)punit);

		if (err == GSTAT_OK)
		{
			char resource[WSF_STR_RESOURCE];
			STprintf(resource, "p%d", id);
			err = DDSDeleteResource(resource);
		}

		if (err == GSTAT_OK)
		{
			err = DDGExecute(&wsf_session, WSS_DEL_UNIT_LOC, "%d",
					 &id);
			CLEAR_STATUS( DDGClose(&wsf_session) );

			if (err == GSTAT_OK)
				err = WCSUnitDelete(id);
			if (err == GSTAT_OK)
				err = DDGDelete(
						&wsf_session, 
						WCS_OBJ_UNIT,
						"%d", 
						&id);

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS( DDFSemClose(&wcs_unit_semaphore));
	}
return(err);
}

/*
** Name: Method to list existing units
**		WCSOpenUnit() - Open a cursor
**		WCSGetUnit() - Fetch the current location
**		WCSCloseUnit() - Close the cursor
**
** Description:
**	these three functions allow the user to list all the
**	declared unit. They garantee the integrity of the 
**	information through a semaphore.
*/

/*
** Name: WCSOpenUnit() - Open cursor to list units
**
** Description:
**
** Inputs:
**	PTR*	: cursor
**
** Outputs:
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
WCSOpenUnit (PTR		*cursor) 
{
return(DDFSemOpen(&wcs_unit_semaphore, FALSE));
}

/*
** Name: WCSGetUnit() - fetch the unit
**
** Description:
**
** Inputs:
**	PTR*	: cursor
**
** Outputs:
**	u_i4	: unit id
**	char*	: unit name
**	u_i4	: unit owner
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
WCSGetUnit (
	PTR		*cursor,
	u_i4	**id, 
	char	**name,
	u_i4	**owner) 
{
return(WCSUnitBrowse(cursor, id, name, owner));
}

/*
** Name: WCSCloseUnit() - close the unit cusor
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
WCSCloseUnit (PTR		*cursor) 
{
	CLEAR_STATUS(DDFSemClose(&wcs_unit_semaphore));
return(GSTAT_OK);
}

/*
** Name: WCSRetrieveUnit() - Retrieve a unit 
**
** Description:
**
** Inputs:
**	u_i4	: id
**
** Outputs:
**	char**	: name
**	u_nat**	: owner
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
WCSRetrieveUnit (
	u_i4	id, 
	char	**name,
	u_i4	**owner) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = WCSGetProFromId(id, name, owner);
		CLEAR_STATUS(DDFSemClose(&wcs_unit_semaphore));
	}
return(err);
}

/*
** Name: WCSUnitLocAdd() - Declare a new association between
**							  a unit and a location
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
WCSUnitLocAdd (
	u_i4 unit,
	u_i4 location)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		if (err == GSTAT_OK)
			err = DDGInsert(
					&wsf_session, 
					WCS_OBJ_UNIT_LOC,
					"%d%d", 
					&unit,
					&location);

		if (err == GSTAT_OK)
		{
			err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
			if (err == GSTAT_OK) 
			{
				err = WCSMemUnitLocAdd(unit, location);
				CLEAR_STATUS( DDFSemClose(&wcs_loc_semaphore));
			}
		}

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_unit_semaphore));
	}
return(err);
}

/*
** Name: WCSUnitLocDel() - delete a new association between
**							  a unit and a location
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
WCSUnitLocDel (
	u_i4 unit,
	u_i4 location)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wcs_unit_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		if (err == GSTAT_OK)
			err = DDGDelete(
					&wsf_session, 
					WCS_OBJ_UNIT_LOC,
					"%d%d", 
					&unit,
					&location);

		if (err == GSTAT_OK)
		{
			err = DDFSemOpen(&wcs_loc_semaphore, TRUE);
			if (err == GSTAT_OK) 
			{
				err = WCSMemUnitLocDel(unit, location);
				CLEAR_STATUS( DDFSemClose(&wcs_loc_semaphore));
			}
		}

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS( DDFSemClose(&wcs_unit_semaphore));
	}
return(err);
}

/*
** Name: Method to list all the location associated to a unit
**		WCSOpenUnitLoc() - Open a cursor
**		WCSGetUnitLoc() - Fetch the current location
**		WCSCloseUnitLoc() - Close the cursor
**
** Description:
**	these three functions allow the user to list all the
**	association unit/locations. They garantee the integrity of the 
**	information through a semaphore.
*/

/*
** Name: WCSOpenUnitLoc() - Open cursor to list unit/locations
**
** Description:
**
** Inputs:
**	PTR*	: cursor
**
** Outputs:
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
WCSOpenUnitLoc (
	PTR		*cursor) 
{
return(DDFSemOpen(&wcs_unit_semaphore, FALSE));
}

/*
** Name: WCSGetUnitLoc() - fetch unit/locations
**
** Description:
**	For potential evolution
**
** Inputs:
**	PTR*	: cursor
**	u_i4	: unit
**
** Outputs:
**	u_nat*	: location
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
WCSGetUnitLoc (
	PTR		*cursor,
	u_i4	unit,
	u_i4	*location)
{
return(WCSUnitLocBrowse(cursor, unit, location));
}

/*
** Name: WCSCloseUnitLoc() - close unit/locations
**
** Description:
**	For potential evolution
**
** Inputs:
**	PTR*	: cursor
**	u_i4	: unit
**
** Outputs:
**	u_nat*	: location
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
WCSCloseUnitLoc (
	PTR		*cursor)
{
	CLEAR_STATUS (DDFSemClose(&wcs_unit_semaphore));
return(GSTAT_OK);
}

/*
** Name: 
**		WCSOpen() - SIfopen
**		WCSRead() - SIfread
**		WCSWrite()- SIfwrite
**		WCSClose()- SIfClose
**		WCSLength()- SIfClose
**
** Description:
**		Those following functions are equivalent to the
**		corresponding SI function. Those overwritten functions
**		merge SI functionalities and DDF error.
**
*/

GSTATUS 
WCSOpen(
	WCS_FILE	*file,
	char		*mode,
	i4			type,
	i4		length)
{
	GSTATUS err = GSTAT_OK;

	if (file->fd != NULL || SIfopen (&(file->loc), mode, type, length, &(file->fd)) != OK)
	{
		err = DDFStatusAlloc( E_WS0009_WPS_CANNOT_OPEN_FILE );
		CLEAR_STATUS( DDFStatusInfo(err, file->info->path));
	}
return(err);
}

GSTATUS 
WCSRead(
	WCS_FILE	*file,
	i4		numofbytes,
	i4		*actual_count,
	char		*pointer)
{
	GSTATUS err = GSTAT_OK;
	STATUS status;

	if (file->fd != NULL)
		status = SIread (file->fd, numofbytes, actual_count, pointer);
	
	if (file->fd == NULL || (status != OK && status != ENDFILE))
	{
		err = DDFStatusAlloc( E_WS0067_WCS_CANNOT_READ_FILE );
		CLEAR_STATUS( DDFStatusInfo(err, file->info->path));
	}
return(err);
}

GSTATUS 
WCSWrite(
	i4		typesize,
	char		*pointer,
	i4		*count,
	WCS_FILE	*file)
{
	GSTATUS err = GSTAT_OK;
	if (file->fd == NULL || SIwrite(typesize, pointer, count, file->fd) != OK)
	{
		err = DDFStatusAlloc( E_WS0068_WCS_CANNOT_WRITE_FILE );
		CLEAR_STATUS( DDFStatusInfo(err, file->info->path));
	}
return(err);
}

GSTATUS 
WCSClose(
	WCS_FILE	*file)
{
	GSTATUS err = GSTAT_OK;
	if (file->fd == NULL || SIclose(file->fd) != OK)
	{
		err = DDFStatusAlloc( E_WS0069_WCS_CANNOT_CLOSE_FILE );
		CLEAR_STATUS( DDFStatusInfo(err, file->info->path));
	}
	file->fd = NULL;
return(err);
}

GSTATUS 
WCSLength (
	WCS_FILE	*file,
	i4		*length)
{
	LOINFORMATION	loinf;
	i4						flagword;
	LOCATION			loc;

	LOfroms(PATH & FILENAME, file->info->path, &loc);
	flagword = (LO_I_SIZE);
	if (LOinfo(&loc, &flagword, &loinf) == OK)
		*length = loinf.li_size;
	else
		*length = 0;
return (GSTAT_OK);
}
