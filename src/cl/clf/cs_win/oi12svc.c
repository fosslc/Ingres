/*
**  Copyright (c) 1995, 2004 Ingres Corporation
**
**  Name: oi12svc.c
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
**	11-apr-1997 (somsa01)
**	    It will now be possible to install the service when the user is
**	    logged in as the domain ingres user. The service will live on the
**	    local machine. (Bug 75063)
**	15-jun-97 (mcgem01)
**	    The purpose of this executable is solely to remove the
**	    CA-OpenIngres 1.2 database service from the service
**	    control database.
**	20-dec-2000 (somsa01)
**	    In RemoveIngresService(), update any services which depend on
**	    us with the new Ingres service name format before removing the
**	    old service.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <compat.h>
#include <sicl.h>
#include <me.h>
#include <nm.h>
#include <st.h>

SC_HANDLE	schSCManager;
TCHAR		tchNewServiceName[255];

void RemoveIngresService  (VOID);
void PrintUsage		  (VOID);


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
*/

VOID
main(int argc, char *argv[])
{
    SIprintf("OpenIngres Service Removal Utility.\nCopyright (c) 1995, 1997 ");
    SIprintf("Ingres Corporation\n");

    if (argc != 2) 
    {
	PrintUsage();
    }

    if (!stricmp(argv[1], "remove"))
        RemoveIngresService();
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
**     15-jul-95 (emmag)
**	   Created.
*/

VOID PrintUsage(void)
{
        SIprintf("\noi12svc remove\n");
        SIprintf("\tto remove the CA-OpenIngres 1.2 server as a service.\n");
        exit(1);
}


/*
** Name: RemoveIngresService
**
** Description:
**      Remove CA-OpenIngres service on NT.
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
**	20-dec-2000 (somsa01)
**	    Update any services which depend on us with the new Ingres
**	    service name format before deleting the old service.
*/

VOID
RemoveIngresService()
{
	BOOL    		ret;
	SC_HANDLE		schService, schDepService;
	SC_HANDLE		schSCManager;
	LPTSTR			lpszMachineName; 
	LPCTSTR			lpszServiceName;
	LPCTSTR			lpszNewServiceName;
	LPTSTR			lpszDatabaseName;
	DWORD			fdwDesiredAccess;
	STATUS			errnum;
	CHAR			*inst_id;
	TCHAR			tchII_INSTALLATION[3];
	ENUM_SERVICE_STATUS	*lpServices=NULL;
	QUERY_SERVICE_CONFIG	*lpServiceConfig=NULL;
	DWORD			cbBufSize=0, BytesNeeded;
	DWORD			ServicesReturned, status;

	/*
	** Append the installation identifier to the new service name.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);
	STprintf(tchNewServiceName, "Ingres_Database_%s", tchII_INSTALLATION);
	lpszNewServiceName = tchNewServiceName;

	lpszMachineName  = NULL;		/* Local machine	*/
	lpszDatabaseName = NULL;		/* ServicesActive 	*/
	fdwDesiredAccess = SC_MANAGER_CREATE_SERVICE;

	schSCManager = OpenSCManager( lpszMachineName,
				      lpszDatabaseName,
				      fdwDesiredAccess);

	if ( schSCManager == NULL)
	{
	    SIprintf("oi12svc: OpenSCManager (%d)\n", GetLastError());
	    return;
	}

	lpszServiceName = "CA-OpenIngres_Database";
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
	    int		i;
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
					  MEreqmem(0, cbBufSize, TRUE, NULL);

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
			** Search for "CA-OpenIngres_Database" and replace it
			** with "Ingres_Database_<Installation Identifier>"
			*/
			while (*p != '\0')
			{
			    if (STbcompare(p, 0, lpszServiceName, 0, TRUE) ==
				0)
			    {
				MEcopy( tchNewServiceName,
					STlength(tchNewServiceName)+1,
					q );
				q += STlength(tchNewServiceName)+1;
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

	ret = DeleteService(schService);

	if (ret)
	    SIprintf("CA-OpenIngres Database service removed.\n");
	else
	    SIprintf("DeleteService failed with error code (%d)\n", 
				GetLastError());
}
