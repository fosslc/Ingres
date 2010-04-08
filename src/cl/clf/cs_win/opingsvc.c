/*
**  Copyright (c) 1995, 2004 Ingres Corporation
**
**  Name: opingsvc.c
**
**  Description:
**
**	This program installs OpenIngres as a service which can be seen 
**	by using 
**		REGEDT32 HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
**
**  History:
**      15-jul-95 (emmag)
**          Created.
**	21-mar-96 (emmag)
**	    Add password expiration warning message.
**      07-jun-1996 (emmag)
**          If the service fails to start it should be a normal and
**          not a severe error.  This is the difference between getting
**          a message on the screen telling you that your password has
**          expired etc. and winding up with the machine constantly
**          rebooting.
**	04-apr-1997 (canor01)
**	    Use generic system names from gv.c to allow program to be
**	    used in both Jasmine and OpenIngres.
**	11-apr-1997 (somsa01)
**	    It will now be possible to install the service when the user is
**	    logged in as the domain ingres user. The service will live on the
**	    local machine. (Bug 75063)
**	12-may-97 (mcgem01)
**	    Modify the warning and error messages for use by Jasmine and
**	    Ingres.  Also, fix a typo when setting up the domain\\username.
**      14-may-97 (mcgem01)
**          Clean up compiler warnings.
**	08-jan-1998 (somsa01)
**	    Added SERVICE_USER_DEFINED_CONTROL as a flag to the creation of
**	    the service, and also added setting II_INSTALLATION and
**	    II_TEMPORARY in the registry for the service. (Bug #87751)
**	12-feb-1998 (canor01)
**	    Quote the pathname, since it may contain embedded spaces.
**	14-sep-98 (mcgem01)
**	    Update the copyright notice.
**	10-nov-98 (mcgem01)
**	    Allow for the installation and removal of Ingres, OpenIngres 
**	    and CA-OpenIngres services.
**	26-apr-1999 (mcgem01)
**	    The TNG team have requested that the Ingres service be started
**	    from the Local System account rather than by the ingres User.
**	29-aug-2000 (rodjo04)
**	    Removed TNG_EDITION from source code. Now we check the config.dat 
**	    file for the value of II.*.setup.embed_installation.
**	06-oct-2000 (rodjo04)
**	    Changed II_INSTALLATION parameter to SystemCfgPrefix when checking
**	    for %s.%s.setup.embed_installation.
**	28-apr-1999 (mcgem01)
**	    Updated for the different product project.
**	28-sep-1999 (somsa01)
**	    In InstallIngresService(), after the different product changes, an
**	    "ifdef <product>" was left off of the retrieval of II_TEMPORARY
**	    and II_INSTALLATION.
**	22-jan-2000 (somsa01)
**	    Key the service name off of II_INSTALLATION.
**	29-jan-2000 (mcgem01)
**	    Add the ability to install the service under the localsystem
**	    account without a password for client installations.
**	29-jan-2000 (mcgem01)
**	    Overlooked the fact that the product name is optional in the
**	    previous change.
**	18-aug-2000 (mcgem01)
**	    If we're installing this service for an Embedded Ingres 
**	    installation, it should be installed under the localsystem
**	    account.
**	18-dec-2000 (somsa01)
**	    In RemoveIngresService(), update any services which depend on
**	    us with the new Ingres service name format before removing the
**	    old service.
**	02-jul-2001 (somsa01)
**	    Cleaned up some holes whereby we were not closing the NT
**	    service handles.
**      29-Nov-2001 (fanra01)
**          Bug 106529
**          Add registry key creation to enable EventLog to retrieve messages.
**          HKEY_LOCAL_MACHINE\
**              SYSTEM\CurrentControlSet\Services\EventLog\Application\<ingsvc>
**                  EventMessageFile    REG_EXPAND_SZ
**                  TypesSupported      REG_DWORD
**	13-mar-2002 (somsa01)
**	    For the SDK, the account to use for service creation will be
**	    "LocalSystem".
**	09-apr-2002 (penga03)
**	    Correct the error occurred while setting the service account name.
**	13-jan-2003 (somsa01)
**	    Added option to "install" to specify a password.
**	21-jan-2004 (penga03)
**	    Added a check before removing the old Ingres service that if it 
**	    belongs to the same instance being installed/removed. 
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	07-sep-2004 (penga03)
**	    Install Ingres service using Local System account by default.
**	16-sep-2004 (penga03)
**	    Backed out the last change (471628).
**	20-sep-2004 (somsa01)
**	    Cleaned up message printout for "ingres" user.
**	07-May-2005 (drivi01)
**	    Modified fdwDesiredAccess on Ingres Service being created
**	    to include SERVICE_CHANGE_CONFIG.  Replaced '||' symbols
**	    with '|' symbol for desired access on Ingres service.
**	    Added routine to fill in Description field of Ingres 
**	    Service after its creation.
**	27-Jun-2005 (drivi01)
**	    Slightly modified description as well as fixed description 
**	    pointers.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**	25-Jul-2007 (drivi01)
**	    Updated error handling for routines that are responsible for
**	    service installation.  On Vista, when user is running with
**	    a restricted token, they will get ERROR_ACCESS_DENIED errors.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    DBA Tools service was renamed to Ingres_DBATools_<instance id>
**	    and a new parameter was added to this file to tell opingsvc
**	    when to install a service with a new name.
**	    Parameter name is DBATools and to install service user would
**	    need to specify "opingsvc install DBATools client" and
**	    to remove it would be "opingsvc remove DBATools".
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <compat.h>
#include <nm.h>
#include <st.h>
#include <gl.h>
#include <si.h>
#include <er.h>
#include <gl.h>
#include <me.h>
#include <pm.h>

SC_HANDLE	schSCManager;
TCHAR		tchServiceName[ 255 ];
TCHAR		tchOldServiceName[ 255 ];
TCHAR		tchDisplayName[ 255 ];
TCHAR		tchDescription[ 255 ];
TCHAR		tchServiceStartName[ 255 ];
TCHAR		tchServiceInstallName[ 255 ];
CHAR		ServicePassword[255] = "";

void InstallIngresService (BOOL);
void RemoveIngresService  (VOID);
void PrintUsage		  (char *);
void InitializeGlobals    (char *);


/*
** Name: main
**
** Description:
**      Called to install CA-OpenIngres service on NT.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
**      15-jul-95 (emmag)
**          Created.
**	13-jan-2003 (somsa01)
**	    Added option to "install" to specify a password.
*/

VOID
main(int argc, char *argv[])
{

    BOOL IsClient = FALSE;

    SIprintf( "%s Service Installer. Copyright (c) 2005-2006 ",
	      SystemProductName );
    SIprintf("Ingres Corporation\n");

    if (argc < 2 || argc > 4) 
    {
	PrintUsage( argv[0] );
    }

    InitializeGlobals( argv[0] );

    if (argc == 3 && stricmp(argv[1], "remove"))
    {
	if ((!stricmp(argv[2], "Ingres")) || 
	    (!stricmp(argv[2], "OpenIngres")) || 
	    (!stricmp(argv[2], "CA-OpenIngres")))
	{
		STprintf ( tchServiceName, "%s_Database", argv[2] );
		STprintf ( tchOldServiceName, "%s_Database", argv[2] );
		STprintf ( tchDisplayName, "%s Intelligent Database", argv[2]);
		STprintf ( tchDescription, "Manages the startup and shutdown of an Ingres Instance.");
	}
	else if (STrstrindex(argv[2], "-password:", 0, FALSE))
	    STcopy(STrstrindex(argv[2], ":", 0, FALSE) + 1, ServicePassword);
	else
	{
	    if (!stricmp(argv[2], "client")) 
		IsClient = TRUE;
	    else
		PrintUsage( argv[0] );
	}
    }
    if (argc == 4)
    {
	if (!stricmp(argv[3], "client"))
	{
	    IsClient = TRUE;
		if (!stricmp(argv[2], "DBATools"))
		{
		STprintf ( tchServiceName, "Ingres_DBATools", argv[2] );
		STprintf ( tchOldServiceName, "Ingres_DBATools", argv[2] );
		STprintf ( tchDisplayName, "Ingres DBA Tools", argv[2] );
		STprintf ( tchDescription, "Manages the startup and shutdown of an Ingres DBA Tools client.");
		}
	}
	else if (STrstrindex(argv[3], "-password:", 0, FALSE))
	    STcopy(STrstrindex(argv[3], ":", 0, FALSE) + 1, ServicePassword);
	else
	    PrintUsage(argv[0]);
    }

    if (argc == 2 && !stricmp(argv[1], "remove"))
        RemoveIngresService();
    else if(argc == 3 && !stricmp(argv[1], "remove") && !stricmp(argv[2], "DBATools"))
    {
		STprintf ( tchServiceName, "Ingres_DBATools" );
		RemoveIngresService();
    } 
    else 
    {
	if (!stricmp(argv[1], "install"))
                InstallIngresService(IsClient);
        else
	    PrintUsage( argv[0] );
    }
    CloseServiceHandle(schSCManager);
}



/*
** Name: PrintUsage
**
** Description: 
**     Print the usage message.
**
** History:
**     15-jul-95 (emmag)
**	   Created.
*/

VOID PrintUsage( char *exe )
{
        SIprintf("usage:\t%s install [ProductName] [client | -password:<ServicePassword>]\n", exe);
        SIprintf("\t\tto install the %s server as a service.\n",
		 SystemProductName);
        SIprintf("\t%s remove <ProductName>\n", exe);
        SIprintf("\t\tto remove the %s server as a service.\n",
		 SystemProductName);
        exit(1);
}


/*
** Name: InstallIngresService.
**
** Description:
**      Called to install OpenIngres service on NT.
**
** Inputs:
**      client - if TRUE indicates that this is a client installation
**		and the service should be owned by the localsystem account.
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
**      15-jul-95 (emmag)
**          Created.
**	11-apr-1997 (somsa01)
**	    The service can now be installed if the user is logged in as the
**	    domain ingres user.
**	08-jan-1998 (somsa01)
**	    Added SERVICE_USER_DEFINED_CONTROL as a flag to the creation of
**	    the service, and also added setting II_INSTALLATION and
**	    II_TEMPORARY in the registry for the service. (Bug #87751)
**	28-sep-1999 (somsa01)
**	    After the different product changes, an "ifdef <product>" was left
**	    off of the retrieval of II_TEMPORARY and II_INSTALLATION.
**      28-Nov-2001 (fanra01)
**          Add creation of registry key required for EventLog messages.
**	13-jan-2003 (somsa01)
**	    Added usage of service password.
*/

VOID
InstallIngresService(BOOL client)
{

	SC_HANDLE	schService;
	LPCTSTR		lpszServiceName;
	LPCTSTR		lpszDisplayName;
	DWORD		fdwDesiredAccess;
	DWORD		fdwServiceType;	
	DWORD		fdwStartType;	
	DWORD		fdwErrorControl; 
	LPCTSTR		lpszBinaryPathName;
	LPCTSTR		lpszLoadOrderGroup;
	LPDWORD		lpdwTagID;
	LPCTSTR		lpzDependencies;
	LPCTSTR		lpszServiceStartName;
	LPCTSTR		lpszPassword;
	TCHAR		tchSysbuf[ _MAX_PATH];
	TCHAR		tchII_SYSTEM[ _MAX_PATH];
	TCHAR		tchII_INSTALLATION[3];
	TCHAR		tchII_TEMPORARY[ _MAX_PATH];
	CHAR		*inst_id;
	CHAR		*temp_loc;
	HKEY		hKey1;
	HKEY		hKey2;
	DWORD		cbData = _MAX_PATH;
	DWORD		dwReserved = 0;
	DWORD		nret;
	DWORD		dwDisposition;
	int		len = 0;
	static char	szUserName[1024] = "";
	HANDLE		hProcess, hAccessToken;
	UCHAR		InfoBuffer[1000], szDomainName[1024], szAccountName[1024];
	PTOKEN_USER	pTokenUser = (PTOKEN_USER)InfoBuffer;
	DWORD		dwInfoBufferSize, dwDomainSize=1024, dwAccountSize=1024;
	SID_NAME_USE	snu;
	DWORD		dwUserNameLen=sizeof(szUserName);
	BOOL 		IsEmbedded = FALSE;
	char    	config_string[256];
	char    	*value, *host;
	SERVICE_DESCRIPTION svcDescription;
	


	/* 
	** Get II_SYSTEM from the environment.
	*/

	GetEnvironmentVariable( SystemLocationVariable, 
				tchII_SYSTEM, _MAX_PATH );
	
        /*
        ** Strip II_SYSTEM of any \'s that may have been tagged onto
        ** the end of it.
        */

        len = strlen (tchII_SYSTEM);

        if (tchII_SYSTEM [len-1] == '\\')
            tchII_SYSTEM [len-1] = '\0';

	/*
	** Get II_INSTALLATION and II_TEMPORARY from the symbol table.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);
	NMgtAt("II_TEMPORARY", &temp_loc);
	STcopy(temp_loc, tchII_TEMPORARY);
	len = STlength (tchII_TEMPORARY);
	if (tchII_TEMPORARY [len-1] == '\\')
	    tchII_TEMPORARY [len-1] = '\0';

	/*
	** Append the installation identifier to the service name.
	*/
	STprintf(tchServiceName, "%s_%s", tchServiceName, tchII_INSTALLATION);
	STprintf(tchDisplayName, "%s [%s]", tchDisplayName,
		 tchII_INSTALLATION);

#ifdef EVALUATION_RELEASE
	strcpy(szUserName, "LocalSystem");
#else
	/*
	** Get the domain name that this user is logged on to. This could
	** also be the local machine name.
	*/
	hProcess = GetCurrentProcess();
	OpenProcessToken(hProcess,TOKEN_READ,&hAccessToken);
	GetTokenInformation(hAccessToken, TokenUser, InfoBuffer, 1000,
			    &dwInfoBufferSize);
	LookupAccountSid(NULL, pTokenUser->User.Sid, szAccountName,
			 &dwAccountSize, szDomainName, &dwDomainSize, &snu);
	strcpy(szUserName, szDomainName);
	strcat(szUserName, "\\");
	strcat(szUserName, szAccountName);

#endif  /* EVALUATION_RELEASE */

	/* 
	** Connect to service control manager on the local machine and 
	** open the ServicesActive database.
	*/

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

	
	if ( schSCManager == NULL )
	{
            switch (GetLastError ())
            {
                case ERROR_ACCESS_DENIED:
                    SIprintf ("You must have administrator privileges to "\
                        "install or remove a service.\n");
                    return;

                default:
			SIprintf("%s: Failed to connect to the Service Control ",
				tchServiceInstallName);
			SIprintf("manager.\n\t OpenSCManager (%d)\n", GetLastError());
			return;
            }
	
	}

	nret = RegCreateKeyEx(
			HKEY_CLASSES_ROOT,
			tchServiceName,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey1,
			&dwDisposition);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("%s: Cannot open/create Registry Key: %d\n",
					tchServiceInstallName, nret);
	    return;
	}

	nret = RegCreateKeyEx (
		hKey1,
		"shell", 
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,	
		&hKey2,
		&dwDisposition);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("%s: Cannot open/create Registry Key: %d\n",
					tchServiceInstallName, nret);
	    return;
	}

	cbData = strlen(tchII_SYSTEM) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_SYSTEM",
		0,
		REG_SZ,
		tchII_SYSTEM,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf( "Cannot create Registry value for %s: %d\n",
		      SystemLocationVariable, nret);
	    return;
	}

	cbData = STlength(tchII_INSTALLATION) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_INSTALLATION",
		0,
		REG_SZ,
		tchII_INSTALLATION,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("Cannot create Registry value for II_INSTALLATION: %d\n",
			nret);
	    return;
	}

	cbData = STlength(tchII_TEMPORARY) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_TEMPORARY",
		0,
		REG_SZ,
		tchII_TEMPORARY,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("Cannot create Registry value for %s_TEMPORARY: %d\n",
			SystemVarPrefix, nret);
	    return;
	}

	STprintf(tchSysbuf, "\"%s\\ingres\\bin\\servproc.exe\"", tchII_SYSTEM);

	lpszServiceName  = tchServiceName;
	lpszDisplayName  = tchDisplayName;
	fdwDesiredAccess = SERVICE_START | SERVICE_STOP |
				SERVICE_USER_DEFINED_CONTROL | 
				SERVICE_CHANGE_CONFIG;
	fdwServiceType   = SERVICE_WIN32_OWN_PROCESS;
	fdwStartType     = SERVICE_DEMAND_START;
	fdwErrorControl  = SERVICE_ERROR_NORMAL;
	lpszBinaryPathName = tchSysbuf;
	lpszLoadOrderGroup = NULL;
	lpdwTagID = NULL;
	lpzDependencies = NULL;


	/*
	** Get the host name, formally used iipmhost.
	*/
	host = PMhost();

	/*
	** Build the string we will search for in the config.dat file.
	*/
	STprintf( config_string,
	          ERx("%s.%s.setup.embed_installation"),
	          SystemCfgPrefix, host );

	if( (PMinit() == OK) &&
	    (PMload( NULL, (PM_ERR_FUNC *)NULL ) == OK) )
	{
	    if( PMget( config_string, &value ) == OK )
	        if ((value[0] != '\0') &&
	            (STbcompare(value,0,"on",0,TRUE) == 0) )
	            IsEmbedded = TRUE;
	}


	if (client || IsEmbedded )
	{
		lpszServiceStartName = NULL;
		lpszPassword = NULL;
	}
	else
	{
		lpszServiceStartName = szUserName;
		lpszPassword = ServicePassword;
	}

	/* 
	** regedt32 can check this on 
	**	HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	*/

	schService = OpenService(schSCManager, 
				lpszServiceName, 
				SERVICE_ALL_ACCESS);

	if (schService != NULL)
        {
	   SIprintf("%s: The service \"%s\" already exists.\n", 
				tchServiceInstallName, lpszServiceName);
	   CloseServiceHandle(schService);
	   return;
	}

	/* 
	** Only create the service if you know that it doesn't already exist.
	*/
	schService = CreateService(     
		schSCManager,
		lpszServiceName,
		lpszDisplayName,
		fdwDesiredAccess,
		fdwServiceType,
		fdwStartType,
		fdwErrorControl,
		lpszBinaryPathName,
		lpszLoadOrderGroup,
		lpdwTagID,
		lpzDependencies,
		lpszServiceStartName,
		lpszPassword);
	
	if ( schService == NULL )
	{
	    SIprintf( "Failed to create the %s Service.\n",
		      lpszServiceName );
	    SIprintf( "\t CreateService (%d)\n", GetLastError() );
	    return;
	}

	svcDescription.lpDescription = tchDescription;
	if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, 
		&svcDescription))
	{
		SIprintf("Failed to update Description of the Ingres Service");
		SIprintf("\n\t Failed with system error (%d)\n", GetLastError());
		return;
	}


        STprintf( tchSysbuf,
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
            lpszServiceName );
        if ((nret = RegCreateKeyEx( HKEY_LOCAL_MACHINE, tchSysbuf, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey1,
            &dwDisposition )) != ERROR_SUCCESS)
        {
	    SIprintf( "Failed to create the %s key for EventLog messages.\n",
		      lpszServiceName );
	    SIprintf( "\t RegCreateKeyEx (%d)\n", GetLastError() );
	    return;
        }
        if (dwDisposition == REG_CREATED_NEW_KEY)
        {
            STprintf( tchSysbuf, "%s\\ingres\\bin\\servproc.exe", tchII_SYSTEM );
            if ((nret = RegSetValueEx( hKey1, TEXT("EventMessageFile"), 0,
                REG_EXPAND_SZ, (CONST BYTE *)tchSysbuf,
                STlength(tchSysbuf) + 1 )) != ERROR_SUCCESS)
            {
                SIprintf( "Failed to set EventMessageFile value.\n" );
                SIprintf( "\t RegSetValueEx (%d)\n", GetLastError() );
                RegCloseKey( hKey1 );
                return;
            }
            else
            {
                DWORD   evt_types = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                    EVENTLOG_INFORMATION_TYPE;
                if ((nret = RegSetValueEx( hKey1, TEXT("TypesSupported"), 0,
                    REG_DWORD, (CONST BYTE *)&evt_types,
                    sizeof(DWORD) )) != ERROR_SUCCESS)
                {
                    SIprintf( "Failed to set TypesSupported value.\n" );
                    SIprintf( "\t RegSetValueEx (%d)\n", GetLastError() );
                    RegCloseKey( hKey1 );
                    return;
                }
            }

            RegCloseKey( hKey1 );
        }

	SIprintf( "The path to the %s service program ",
		  SystemProductName );
	SIprintf("\"servproc.exe\" \nbased upon the Registry is:\n\n");
	SIprintf("\t   %s\n\n",tchSysbuf);
	SIprintf("To view the value stored in the Microsoft Windows NT ");
	SIprintf( "registry\nfor %s, use the Registry Editor ",
		  SystemLocationVariable );
	SIprintf("\"REGEDT32.EXE\" using\n");
	SIprintf("the key \"HKEY_CLASSES_ROOT on Local Machine\" and ");
	SIprintf("the subkeys\n");
	SIprintf("\"%s\" and \"shell\".\n\n", lpszServiceName);
#if !defined (INGRES4UNI)
	if (!client)
	{
	    if (STcompare(ServicePassword, "") == 0)
	    {
	SIprintf("Before using the service you must set the password for the ");
	SIprintf("\"%s\"\naccount in the Service Startup dialog.\n\n",
		 szUserName);
	    }
        SIprintf("WARNING: If you choose to start the %s Database ",
		 SystemProductName);
        SIprintf("Service \nautomatically, you must make certain that the ");
        SIprintf("Password Never \nExpires option is specified in the User ");
        SIprintf("Manager for the \"%s\"\nuser.\n", szUserName);
	}
#endif
	SIprintf("\n%s Service installed ", lpszDisplayName);
	SIprintf("successfully.\n");
	CloseServiceHandle(schService);
}


/*
** Name: RemoveIngresService
**
** Description:
**      Remove OpenIngres service on NT.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
**      15-jul-95 (emmag)
**          Created.
**	18-dec-2000 (somsa01)
**	    Update any services which depend on us with the new Ingres
**	    service name format before deleting the old service.
**      28-Nov-2001 (fanra01)
**          Add removal of EventLog registry key, including keys added for
**          old style service names.
*/

VOID
RemoveIngresService()
{
	BOOL    	ret;
	SC_HANDLE	schService, schDepService;
	SC_HANDLE	schSCManager;
	LPTSTR		lpszMachineName; 
	LPCTSTR		lpszServiceName;
	LPCTSTR		lpszOldServiceName;
	LPCTSTR		lpszDisplayName;
	LPTSTR		lpszDatabaseName;
	DWORD		fdwDesiredAccess;
	STATUS		errnum;
	CHAR		*inst_id;
	TCHAR		tchII_INSTALLATION[3];
	TCHAR		tchSysbuf[ _MAX_PATH];
        HKEY            pkey = NULL;
        HKEY            skey = NULL;
	SC_HANDLE	schOldService;
	BOOL		bSame;

	/*
	** Append the installation identifier to the service name.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);
	STprintf(tchServiceName, "%s_%s", tchServiceName, tchII_INSTALLATION);
	STprintf(tchDisplayName, "%s [%s]", tchDisplayName,
		 tchII_INSTALLATION);

	lpszMachineName  = NULL;		/* Local machine	*/
	lpszDatabaseName = NULL;		/* ServicesActive 	*/
	fdwDesiredAccess = SC_MANAGER_CREATE_SERVICE;

	schSCManager = OpenSCManager( lpszMachineName,
				      lpszDatabaseName,
				      fdwDesiredAccess);

	if ( schSCManager == NULL)
	{
	    SIprintf("%s: OpenSCManager (%d)\n", tchServiceInstallName, 
					GetLastError());
	    return;
	}

	/*
	** Before delete the old service name, check if it belongs to the 
	** same instance.
	*/
	bSame=0;
	schOldService = OpenService(schSCManager, 
				tchOldServiceName, 
				SERVICE_QUERY_CONFIG);
	if (schOldService)
	{
		QUERY_SERVICE_CONFIG *lpOldServiceConfig=NULL;
		DWORD BufSize, BytesNeeded;
		TCHAR tchII_SYSTEM[_MAX_PATH], tchBuf[_MAX_PATH];
		int len;

		GetEnvironmentVariable(SystemLocationVariable, tchII_SYSTEM, _MAX_PATH );
		len = strlen(tchII_SYSTEM);
		if (tchII_SYSTEM[len - 1] == '\\') 
			tchII_SYSTEM[len - 1] = '\0';		
		STprintf(tchBuf, "\"%s\\ingres\\bin\\servproc.exe\"", tchII_SYSTEM);

		BufSize=0;
		if (!QueryServiceConfig(schOldService, lpOldServiceConfig, BufSize, &BytesNeeded) 
			&& GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			BufSize = BytesNeeded + 8;
			lpOldServiceConfig = (QUERY_SERVICE_CONFIG *)MEreqmem(0, BufSize, TRUE, NULL);
			QueryServiceConfig(schOldService, lpOldServiceConfig, BufSize, &BytesNeeded);
		}
		
		SIprintf("old service: %s\n", lpOldServiceConfig->lpBinaryPathName);
		SIprintf("cur service: %s\n", tchBuf);

		if (!_stricmp(lpOldServiceConfig->lpBinaryPathName, tchBuf))
			bSame=1;
		
		CloseServiceHandle(schOldService);
	}

	/*
	** Delete the old service name, if it exists.
	*/
	lpszOldServiceName = tchOldServiceName;
	schService = OpenService(schSCManager, 
				lpszOldServiceName, 
				SERVICE_ALL_ACCESS);
	if (schService && bSame)
	{
	    ENUM_SERVICE_STATUS		*lpServices=NULL;
	    QUERY_SERVICE_CONFIG	*lpServiceConfig=NULL;
	    DWORD			cbBufSize=0, BytesNeeded;
	    DWORD			ServicesReturned, status;

	    /*
	    ** Before deleting the service, update any other services
	    ** which depend on us with the new Ingres service name format.
	    */
	    status = EnumDependentServices( schService,
					    SERVICE_STATE_ALL,
					    lpServices,
					    cbBufSize,
					    &BytesNeeded,
					    &ServicesReturned );
	    if (!status)
	    {
		int	i;
		char	*lpData=NULL, *p, *q;
		DWORD	size;

		cbBufSize = BytesNeeded + 8;
		lpServices = (ENUM_SERVICE_STATUS *)MEreqmem(0, cbBufSize,
							     TRUE, NULL);
		EnumDependentServices(schService,
				      SERVICE_STATE_ALL,
				      lpServices,
				      cbBufSize,
				      &BytesNeeded,
				      &ServicesReturned);

		for (i=0; i<(int)ServicesReturned; i++)
		{
		    schDepService = OpenService(schSCManager,
						lpServices[i].lpServiceName,
						SERVICE_QUERY_CONFIG |
						SERVICE_CHANGE_CONFIG);

		    if (schDepService)
		    {
			cbBufSize = 0;
			if (!QueryServiceConfig(schDepService, lpServiceConfig,
						cbBufSize, &BytesNeeded) &&
			    GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
			    cbBufSize = BytesNeeded + 8;
			    lpServiceConfig = (QUERY_SERVICE_CONFIG *)
					      MEreqmem( 0, cbBufSize, TRUE,
							NULL );

			    QueryServiceConfig( schDepService, lpServiceConfig,
						cbBufSize, &BytesNeeded );
	
			    p = lpServiceConfig->lpDependencies;
			    size = 0;
			    while (*p != '\0' || *(p+1) != '\0')
			    {
				size++;
				p++;
			    }
			    size += 8;

			    lpData = (char *)MEreqmem(0, size, TRUE, NULL);
			    p = lpServiceConfig->lpDependencies;
			    q = lpData;

			    /*
			    ** Search for "Ingres_Database" and replace it
			    ** with "Ingres_Database_<Installation Identifier>"
			    */
			    while (*p != '\0')
			    {
				if (STbcompare( p, 0, lpszOldServiceName, 0,
						TRUE ) == 0)
				{
				    MEcopy( tchServiceName,
					    STlength(tchServiceName)+1,
					    q );
				    q += STlength(tchServiceName)+1;
				    p += STlength(p)+1;
				}
				else
				{
				    MEcopy(p, STlength(p)+1, q);
				    q += STlength(p)+1;
				    p += STlength(p)+1;
				}
			    }

			    ChangeServiceConfig(schDepService,
						SERVICE_NO_CHANGE,
						SERVICE_NO_CHANGE,
						SERVICE_NO_CHANGE,
						NULL,
						NULL,
						NULL,
						lpData,
						NULL,
						NULL,
						lpServiceConfig->lpDisplayName);

			    MEfree(lpData);
			    lpData = NULL;
			    CloseServiceHandle(schDepService);
			    schDepService = NULL;
			}
		    }
		}

		MEfree((PTR)lpServices);
		lpServices = NULL;
	    }
            STprintf( tchSysbuf,
                "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application" );
            errnum = RegOpenKeyEx( HKEY_LOCAL_MACHINE, tchSysbuf, 0, KEY_QUERY_VALUE, &skey );
            if (errnum != ERROR_SUCCESS)
            {
                SIprintf("%s: RegOpenKeyEx (%d)\n", tchServiceInstallName,
                    errnum);
            }
            else
            {
                pkey = skey;
                if (RegOpenKeyEx( pkey, lpszOldServiceName, 0, KEY_WRITE, &skey )
                    == ERROR_SUCCESS)
                {
                    errnum = RegDeleteKey( pkey, lpszOldServiceName );
                    if (errnum != ERROR_SUCCESS)
                    {
                        SIprintf("%s: RegDeleteKey (%d)\n", tchServiceInstallName,
                            errnum);
                    }
                }
            }

	    /* Now, delete the old Ingres service. */
		if (DeleteService(schService))
			SIprintf("%s service removed.\n", lpszOldServiceName);
		else
			SIprintf("DeleteService failed with error code (%d)\n", GetLastError());
	    CloseServiceHandle(schService);
	}

	lpszServiceName = tchServiceName;
	schService = OpenService(schSCManager, 
				lpszServiceName, 
				SERVICE_ALL_ACCESS);

	if (schService == NULL)
	{
	    errnum = GetLastError();
	    switch (errnum)
	    {
	 	case ERROR_SERVICE_DOES_NOT_EXIST:
		    SIprintf ("The service \"%s\" does not exist.\n", 
					lpszServiceName);
		    break;

		case ERROR_ACCESS_DENIED:
		    SIprintf ("The service control manager does not have ");
		    SIprintf ("access to the service %s.\n", lpszServiceName);
		    break;

		default:
	            SIprintf("Failed to remove service %s.\n",lpszServiceName);
		    break;
	    }

	    CloseServiceHandle(schSCManager);
	    return;
	}

        STprintf( tchSysbuf,
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application" );
        errnum = RegOpenKeyEx( HKEY_LOCAL_MACHINE, tchSysbuf, 0, KEY_QUERY_VALUE, &skey );
        if (errnum != ERROR_SUCCESS)
        {
            SIprintf("%s: RegOpenKeyEx (%d)\n", tchServiceInstallName,
                errnum);
        }
        else
        {
            pkey = skey;
            if (RegOpenKeyEx( pkey, tchServiceName, 0, KEY_WRITE, &skey )
                == ERROR_SUCCESS)
            {
                errnum = RegDeleteKey( pkey, tchServiceName );
                if (errnum != ERROR_SUCCESS)
                {
                    SIprintf("%s: RegDeleteKey (%d)\n", tchServiceInstallName,
                        errnum);
                }
            }
        }

	ret = DeleteService(schService);
	CloseServiceHandle(schService);

	lpszDisplayName = tchDisplayName;
	if (ret)
	    SIprintf("%s service removed.\n", lpszDisplayName);
	else
	    SIprintf("DeleteService failed with error code (%d)\n", 
				GetLastError());

	CloseServiceHandle(schSCManager);
}

/*
** Name: InitializeGlobals
**
** Description:
**      Initialize global names based on system names in gv.c
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** Side effects:
**	global variables are initialized
**
** History:
**      04-apr-1997 (canor01)
**          Created.
*/
void
InitializeGlobals( char *exename )
{
	sprintf( tchServiceName, "%s_Database", SystemServiceName );
	sprintf( tchOldServiceName, "%s_Database", SystemServiceName );

	if ( strcmp( "ing", SystemExecName ) == 0 )
	    sprintf( tchDisplayName, "%s Intelligent Database",
		     SystemServiceName );
	else
	    sprintf( tchDisplayName, "%s", SystemServiceName );

	sprintf( tchServiceStartName, ".\\%s", SystemAdminUser );

	sprintf( tchServiceInstallName, exename );
	
	sprintf ( tchDescription, "Manages the startup and shutdown of an Ingres Instance.");
}
