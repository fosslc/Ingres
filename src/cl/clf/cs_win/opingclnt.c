/*
**  Copyright (c) 1995, 1998 Ingres Corporation
**
**  Name: opingclnt.c
**
**  Description:
**
**	This program installs an Ingres client as a service which 
**	can be seen by using 
**		REGEDT32 HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
**
**  History:
**      18-oct-95 (emmag)
**          Created based on opingclnt.
**	21-mar-96 (emmag)
**	    Add warning message about password expiration.       
**      07-jun-1996 (emmag)
**          If the service fails to start it should be a normal and
**          not a severe error.  This is the difference between getting
**          a message on the screen telling you that your password has
**          expired etc. and winding up with the machine constantly
**          rebooting.
**      08-apr-1997 (hanch04)
**          Added ming hints
**      14-may-97 (mcgem01)
**          Clean up compiler warnings.
**	10-apr-98 (mcgem01)
**	    Product name change to Ingres.
**	14-sep-98 (mcgem01)
**	    Update the copyright notice.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, Cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS 
*/

/*
**      mkming hints

NEEDLIBS =      COMPATLIB 


PROGRAM =       opingclient

*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <compat.h>
#include <si.h>

SC_HANDLE	schSCManager;
void InstallIngresService (VOID);
void RemoveIngresService  (VOID);
void PrintUsage		  (VOID);


/*
** Name: main
**
** Description:
**      Called to install Ingres client service on NT.
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
*/

VOID
main(int argc, char *argv[])
{
    SIprintf("Ingres Client Installer. Copyright (c) 1995,1998 ");
    SIprintf("Ingres Corporation\n\n");

    if (argc != 2) 
    {
	PrintUsage();
    }

    if (!stricmp(argv[1], "remove"))
        RemoveIngresService();
    else if (!stricmp(argv[1], "install"))
        InstallIngresService();
    else
	PrintUsage();

    CloseServiceHandle(schSCManager);
}



/*
** Name: PrintUsage
**
** Description: 
**     Print the usage message.
**
** History:
*/

VOID PrintUsage(void)
{
        SIprintf("usage:\topingclient install\n");
        SIprintf("\t\tto install the Ingres Client as a service.\n");
        SIprintf("\topingclient remove\n");
        SIprintf("\t\tto remove the Ingres Client as a service.\n");
        exit(1);
}


/*
** Name: InstallIngresService.
**
** Description:
**      Called to install Ingres client on NT.
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
*/

VOID
InstallIngresService()
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
	HKEY		hKey1;
	HKEY		hKey2;
	DWORD		cbData = _MAX_PATH;
	DWORD		dwReserved = 0;
	DWORD		nret;
	DWORD		dwDisposition;
	int		len = 0;


	/* 
	** Get II_SYSTEM from the environment.
	*/

	GetEnvironmentVariable ("II_SYSTEM", tchII_SYSTEM, _MAX_PATH);
	
        /*
        ** Strip II_SYSTEM of any \'s that may have been tagged onto
        ** the end of it.
        */

        len = strlen (tchII_SYSTEM);

        if (tchII_SYSTEM [len-1] == '\\')
            tchII_SYSTEM [len-1] = '\0';

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
		    SIprintf("opingclient: Failed to connect to SCManager.\n");
		    SIprintf("\t OpenSCManager (%d)\n", GetLastError());
                    return;
            }
	}

	nret = RegCreateKeyEx(
			HKEY_CLASSES_ROOT,
			"Ingres_Client",
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey1,
			&dwDisposition);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("opingclient: Cannot open/create Registry Key: %d\n",nret);
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
	    SIprintf("opingclient: Cannot open/create Registry Key: %d\n",nret);
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
	    SIprintf("Cannot create Registry value for II_SYSTEM: %d\n",nret);
	    return;
	}

	strcpy( tchSysbuf, tchII_SYSTEM );
	strcat( tchSysbuf, "\\ingres\\bin\\clntproc.exe");

	lpszServiceName  = "Ingres_Client";
	lpszDisplayName  = "Ingres Client";
	fdwDesiredAccess = SERVICE_START || SERVICE_STOP;
	fdwServiceType   = SERVICE_WIN32_OWN_PROCESS;
	fdwStartType     = SERVICE_DEMAND_START;
	fdwErrorControl  = SERVICE_ERROR_NORMAL;
	lpszBinaryPathName = tchSysbuf;
	lpszLoadOrderGroup = NULL;
	lpdwTagID = NULL;
	lpzDependencies = NULL;
	lpszServiceStartName = NULL;
	lpszPassword = NULL;


	/* 
	** regedt32 can check this on 
	**	HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	*/

	lpszServiceName = "Ingres_Client";
	schService = OpenService(schSCManager, 
				lpszServiceName, 
				SERVICE_ALL_ACCESS);

	if (schService != NULL)
        {
	   SIprintf("opingclient: The service \"%s\" already exists.\n",
				lpszServiceName);
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
	    SIprintf ("Failed to create the Ingres_Client Service.\n");
	    SIprintf ("\t CreateService (%d)\n", GetLastError());
	    return;
	}

	SIprintf("The path to the Ingres Client service program ");
	SIprintf("\"clntproc.exe\" \nbased upon the Registry is:\n\n");
	SIprintf("\t   %s\n\n",tchSysbuf);
	SIprintf("To view the value stored in the Microsoft Windows NT ");
	SIprintf("registry\nfor II_SYSTEM, use the Registry Editor ");
	SIprintf("\"REGEDT32.EXE\" using\n");
	SIprintf("the key \"HKEY_CLASSES_ROOT on Local Machine\" and ");
	SIprintf("the subkeys\n");
	SIprintf("\"Ingres_Client\" and \"shell\".\n\n");
	SIprintf("\nIngres Client Service installed ");
	SIprintf("successfully.\n\n");
	SIprintf("WARNING: If you choose to start the Ingres Client ");
	SIprintf("Service \nautomatically, you must make certain that the ");
	SIprintf("Password Never \nExpires option is specified in the User ");
	SIprintf("Manager for the required user.\n");
}


/*
** Name: RemoveIngresService
**
** Description:
**      Remove Ingres service on NT.
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
*/

VOID
RemoveIngresService()
{
	BOOL    	ret;
	SC_HANDLE	schService;
	SC_HANDLE	schSCManager;
	LPTSTR		lpszMachineName; 
	LPCTSTR		lpszServiceName;
	LPTSTR		lpszDatabaseName;
	DWORD		fdwDesiredAccess;
	STATUS		errnum;

	lpszMachineName  = NULL;		/* Local machine	*/
	lpszDatabaseName = NULL;		/* ServicesActive 	*/
	fdwDesiredAccess = SC_MANAGER_CREATE_SERVICE;

	schSCManager = OpenSCManager( lpszMachineName,
				      lpszDatabaseName,
				      fdwDesiredAccess);

	if ( schSCManager == NULL)
	{
           switch (GetLastError())
           {
               case ERROR_SERVICE_DOES_NOT_EXIST:
                  SIprintf("The %s service does not exist.\n", lpszServiceName);
                  break;

               case ERROR_ACCESS_DENIED:
                  SIprintf("Only an administrator may remove a service.\n");
                  break;

               default:
                   SIprintf("failure: OpenService() \n");
                   break;
           }
           return;
	}

	lpszServiceName = "Ingres_Client";
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
	    return;
	}

	ret = DeleteService(schService);

	if (ret)
	    SIprintf("Ingres Client service removed.\n");
	else
	    SIprintf("DeleteService failed with error code (%d)\n", 
				GetLastError());
}
