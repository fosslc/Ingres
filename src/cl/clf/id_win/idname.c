/*
** Copyright (c) 1995, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<id.h>
# include	<cv.h>
# include	<gccl.h>
# include	<gl.h>
# include	<nm.h>
# include	<systypes.h>
# include	<cs.h>
# include	<er.h>
# include	<me.h>
# include	<pc.h>
# include	<pm.h>
# include	<st.h>

FUNC_EXTERN BOOL	GVosvers(char *OSVersionString);

/*
** NAME: IDname
**
** Description:
** Set the passed in argument 'name' to contain the name
** of the user who is executing this process.
**
** History:
**	14-apr-95 (emmag) 
**	    Use GetUserName API call
**	01-feb-1996 (canor01)
**	    The API call is expensive, so cache the user name.
**	15-nov-97 (mcgem01)
**	    When running the evaluation release, always return
**	    "ingres".
**	08-jan-1998 (somsa01)
**	    If we are running this process through the setuid setup,
**	    find out the "real" userid. (Bug #87751)
**	12-nov-1998 (canor01)
**	    Include cv.h for CVal() declaration.
**	08-feb-1999 (mcgem01)
**	    For a Unicenter installation the user is always ingres.
**	02-aug-2000 (mcgem01)
**	    Change the name of the parameter we're checking for from
**	    tng_user to embed_user.  Also, key the forcing of the
**	    username to 'ingres' off this variable so that AHD
**	    can continue to use schema qualifiers.
**      18-sep-2000 (rodjo04)
**          Logic was wrong after cross integration. Fixed.
**	28-sep-2000 (mcgem01)
**	    Integration of Sam's change left the evaluation release code
**	    somewhere in the middle of the function, I've restored it
**	    to its rightful place.
**	24-oct-2000 (somsa01)
**	    Added IDname_service() to retrieve the user who will start
**	    the Ingres service.
**	15-dec-2000 (somsa01)
**	    In IDname_service(), if the login id of the Ingres service
**	    is "LocalSystem", change this to "system" to match what would
**	    be returned by GetUserName().
**	27-dec-2000 (somsa01)
**	    In IDname(), use a private context. This is because IDname()
**	    can be called from PMload(); since PMload() will hold the
**	    context0 semaphore, a PMload() here will fail.
**	03-jan-2001 (somsa01)
**	    Moved PMmFree() out one level; it should get executed if
**	    PMmInit() works but PMmLoad() fails.
**	15-feb-2001 (somsa01)
**	    In IDname_service(), obtain the size needed for the data
**	    parameter to QueryServiceConfig() dynamically.
**	19-may-2001 (somsa01)
**	    Removed extra definition of TNGInstallation from previous
**	    cross-integration.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	15-nov-2001 (zhayu01) Bug 106398
**	    On Win9x, if a user does not log on to a domain or machine by
**	    clicking on the Cancel button in the logon dialog box, the 
**	    GetUserName() in IDname() will fail and the szUserName will
**	    not be set. When IDname() gets called again from PMmLoad(),
**	    it will cause infinite calling loop between the two in the
**	    following sequence:
**	      PMmLoad,yyparse,yyresource_list,yylex,yywrap,IDname,PMmLoad
**	    This process will not stop until the stack overflows.
**	    Set szUserName to "nologinusername" if GetUserName() fails and 
**	    the szUserName has not been set.
**	24-jul-2002 (somsa01)
**	    In IDname_service(), lowercase the userid before returning
**	    it, just as is done in IDname().
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers by allowing up to GC_USERNAME_MAX
**	    username's rather than just 24 bytes.
*/

VOID
IDname(name)
char	**name;
{
    static char		szUserName[GC_USERNAME_MAX + 1] = "";
    DWORD		dwUserNameLen = sizeof(szUserName);
    char		*inst_id;
    struct REALUID_SHM 	*RealuidShmPtr;
    HANDLE		RealuidShmHandle;
    char		RealuidShmName[32];
    bool		RealUserIdSet = FALSE;
    char		pidstr[6];
    char		*host;
    char		*value = NULL;
    char		config_string[256];
    static bool         TNGInstallation = FALSE;
    PM_CONTEXT		*context_idname;

# ifdef EVALUATION_RELEASE
        strcpy(*name, "ingres");
# else

    if (szUserName[0] == '\0')
    {
	char	*ObjectPrefix;

	/*
	** See if there is a shared memory segment set up for us to
	** retrieve the real userid.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	CVla((GetCurrentProcessId() & 0x0ffff), pidstr);

	GVshobj(&ObjectPrefix);
    sprintf(RealuidShmName, "%s%s%sRealuidShm", ObjectPrefix, 
		pidstr, inst_id);

	RealuidShmHandle = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
					   FALSE,
					   RealuidShmName);
	if (RealuidShmHandle != NULL)
	{
	    RealuidShmPtr = MapViewOfFile(RealuidShmHandle,
					  FILE_MAP_READ | FILE_MAP_WRITE,
					  0,
					  0,
					  sizeof(struct REALUID_SHM));
	    if (RealuidShmPtr != NULL)
	    {
		if ((RealuidShmPtr->InfoTaken == FALSE) &&
		    (strcmp(RealuidShmPtr->RealUserId, "")))
		{
		    strcpy(szUserName, RealuidShmPtr->RealUserId);
		    RealUserIdSet = TRUE;
		    RealuidShmPtr->InfoTaken = TRUE;
		}
		UnmapViewOfFile(RealuidShmPtr);
	    }
	    CloseHandle(RealuidShmHandle);
	}

	if (!RealUserIdSet)
	/*
	** Since there was no shared memory segment, retrieve the
	** the real userid in the usual way.
	*/
	    if (GetUserName(szUserName, &dwUserNameLen)) 
	    {
		_strlwr(szUserName);
	    }
		else
		{
		if (szUserName[0] == '\0')
			strcpy(szUserName, "nologinusername");
		}
	/*
	** Load PM resource values from the standard location.  PM will
	** have already been initialized and loaded in most cases, but
	** there are some utilities which use ID which didn't previously
	** initalize PM, so we're catering for them here. 
	*/

	if (PMmInit(&context_idname) == OK)
	{
	    if (PMmLoad(context_idname, NULL, (PM_ERR_FUNC *)NULL) == OK)
	    {
		host = PMmHost(context_idname);

		STprintf(config_string, ERx("%s.%s.setup.embed_user"), 
			 SystemCfgPrefix, host);
		/*
		** Go search config.dat for a match on the string
		** we just built.
		*/
		PMmGet(context_idname, config_string, &value);
		if (value != NULL && STbcompare(value, 0, "ON", 0, TRUE) == 0)
		    TNGInstallation = TRUE;
	    }

	    PMmFree(context_idname);
	}
	else
	{
	    TNGInstallation = TRUE;
	}
    }

    if (TNGInstallation == FALSE)
    {
	STcopy(szUserName, *name);
    }
    else
    {
	STcopy("ingres", *name);
    }
# endif /* EVALUATION_RELEASE */
}


/*
**  Name: IDname_service()
**
**  Description:
**	Set the passed in argument 'name' to contain the name
**	of the user who will start the Ingres service.
**
**  History:
**	24-oct-2000 (somsa01)
**	    Created.
**	15-dec-2000 (somsa01)
**	    If the login id of the Ingres service is "LocalSystem",
**	    change this to "system" to match what would be returned
**	    by GetUserName().
**	15-feb-2001 (somsa01)
**	    Obtain the size needed for the data parameter to
**	    QueryServiceConfig() dynamically.
**	24-jul-2002 (somsa01)
**	    Lowercase the userid before returning it, just as is done
**	    in IDname().
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers by allowing up to GC_USERNAME_MAX
**	    username's rather than just 24 bytes.
*/

STATUS
IDname_service(name)
char	**name;
{
    static char			szSvcUserName[GC_USERNAME_MAX + 1] = "";
    SC_HANDLE			schSCManager, OpIngSvcHandle;
    LPQUERY_SERVICE_CONFIG 	lpServiceConfig;
    int				ServiceInfoSize;
    CHAR			ServiceName[255], *cptr;
    CHAR			tchII_INSTALLATION[3], *inst_id;
    DWORD			BytesNeeded;

    if (szSvcUserName[0] == '\0')
    {
	if ((schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ))
	    == NULL)
	{
	    return(FAIL);
	}

	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);

	STprintf(ServiceName, "%s_Database_%s", SYSTEM_SERVICE_NAME,
		 tchII_INSTALLATION);

	if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
					  SERVICE_QUERY_CONFIG)) == NULL)
	{
            return(FAIL);
	}

	/*
	** Get the size needed of the data buffer.
	*/
	QueryServiceConfig(OpIngSvcHandle, NULL, 0, &ServiceInfoSize);

	lpServiceConfig = (QUERY_SERVICE_CONFIG *)MEreqmem( 0,
							    ServiceInfoSize,
							    TRUE,
							    NULL );
	if (!QueryServiceConfig(OpIngSvcHandle, lpServiceConfig,
				ServiceInfoSize, &BytesNeeded))
	{
	    DWORD status = GetLastError();

	    printf("Error = %d\n", status);
	    MEfree((PTR)lpServiceConfig);
	    CloseServiceHandle(OpIngSvcHandle);
	    CloseServiceHandle(schSCManager);
	    return(FAIL);
	}

	cptr = STindex(lpServiceConfig->lpServiceStartName, "\\", 0);
	cptr = (cptr ? cptr+1 : lpServiceConfig->lpServiceStartName);
	STcopy(cptr, szSvcUserName);
	MEfree((PTR)lpServiceConfig);

	if (!STbcompare(szSvcUserName, 0, "LocalSystem", 0, TRUE))
	    STcopy("system", szSvcUserName);

	CloseServiceHandle(OpIngSvcHandle);
	CloseServiceHandle(schSCManager);
    }

    _strlwr(szSvcUserName);
    STcopy(szSvcUserName, *name);

    return(OK);
}
