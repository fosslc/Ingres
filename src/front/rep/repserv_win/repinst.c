/*
** Copyright (c) 1994, 2001 Ingres Corporation
*/
# ifndef NT_GENERIC
# error This should only be built under Windows NT
# endif
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <winsvc.h>
# include <compat.h>
# include <nm.h>
# include <st.h>
# include <gl.h>
# include <si.h>
# include <cm.h>
# include <cv.h>
# include <er.h>
# include <gl.h>
# include <me.h>
# include <pc.h>
# include <pm.h>
# include <ep.h>
# include <iicommon.h>
# include "repntsvc.h"

/**
** Name:	repinst.c - install Replicator service
**
** Description:
**	Defines
**		main		- install or remove Replicator service
**		rep_install	- install Replicator service
**		rep_remove	- remove Replicator service
**
** History:
**	13-jan-97 (joea)
**		Created based on repinst.c in replicator library.
**	23-jun-97 (mcgem01)
**		Integrate Sam's service code change that allows 
**		services to be invoked by domain users as well as 
**		workgroup users.
**      25-Jun-97 (fanra01)
**              Add include of gl.h and iicommon.h now requied for repntsvc.h.
**              Modified to use standalone service process.
**	22-mar-98 (mcgem01)
**		Replace RepServer executable name and location.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	28-oct-98 (abbjo03)
**		Remove testing of DD_RSERVERS.
**	09-aug-1999 (mcgem01)
**		Replace nat with i4.
**	28-jan-2000 (somsa01)
**		Services are now keyed off of the installation identifier.
**	02-jul-2001 (somsa01)
**		Cleaned up some holes whereby we were not closing the
**		NT service handles.
**	19-sep-2001 (somsa01)
**		Corrected sizes of certain variables.
**	25-Jul-2007 (drivi01)
**		This program will exit on Vista if user isn't
**		running with elevated privileges. Elevation
**		is required for this program.
**	16-Oct-2008 (drivi01)
**		Added routine to change username/password configuration
**		of the replicator service.  The usage has changed to take
**		additional config parameter along with server number, 
**		username and password.
**/

static TCHAR	*keynames[] =
{
	TEXT("SYSTEM"),
	TEXT("CurrentControlSet"),
	TEXT("Services"),
	TEXT("EventLog"),
	TEXT("Application")
};
static TCHAR	svc_name[32];

static STATUS rep_install(i4 num_servers);
static STATUS rep_remove(void);
static void abort_msg(DWORD err, char *where);


/*{
** Name:	main - install or remove Replicator service
**
** Description:
**	Installs or remove Replicator as a Windows NT service.
**
** Inputs:
**	argc and argv from the command line
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
i4
main(
i4	argc,
char	*argv[])
{
	i4	num_servers = DEFAULT_NUM_SERVERS;
	char uname[MAX_PATH];
	char pwd[MAX_PATH];

	/* Check if this is Vista and if elevation is required */
	ELEVATION_REQUIRED();
	if (argc == 2)
	{
		if (CMdigit(argv[1]))
		{
			CVan(argv[1], &num_servers);
			num_servers = min(max(1, num_servers), MAX_SERVERS);
			PCexit(rep_install(num_servers));
		}
		else if (STbcompare(argv[1], 0, ERx("remove"), 0, TRUE) == 0)
		{
			PCexit(rep_remove());
		}
	}
	if (argc == 5)
	{
		if (STbcompare(argv[1], 0, ERx("config"), 0, TRUE)==0)
		{
			if (CMdigit(argv[2]))
			{
				CVan(argv[2], &num_servers);
				num_servers = min(max(1, num_servers), MAX_SERVERS);
				if (STrstrindex(argv[3], "-username:", 0, FALSE))
					STcopy(STrstrindex(argv[3], ":", 0, FALSE) + 1, uname);
				if (STrstrindex(argv[4], "-password:", 0, FALSE))
					STcopy(STrstrindex(argv[4], ":", 0, FALSE)+1, pwd);
				if (*uname && *pwd)
					PCexit(rep_config(num_servers, uname, pwd));
			}		
		}
	}

	SIprintf("usage: repinst [num_servers | remove | config <num_servers> -username:<username> -password:<password>]\n");
	PCexit(FAIL);
}


/*{
** Name:	rep_install - install Replicator service
**
** Description:
**	Installs the Replicator as a Windows NT service.  This installs
**	num_servers Replicator Servers.
**
** Inputs:
**	num_servers	- number of servers to install
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
**
** 07-Aug-2000 (fanch01)
**   Changed format string for replicator display_name to
**   remove 99 service limitation.  Bug #102245.
**
**
** History:
**	28-jan-2000 (somsa01)
**	    Key the service name off of the isntlalation identifier.
*/
STATUS
rep_install(
i4	num_servers)
{
	TCHAR	subkeypath[128];
	HKEY	subkey;
	DWORD	disp;
	LONG	ret;
	DWORD	evt_types;
	LPTSTR	msg_file = TEXT("%II_SYSTEM%\\ingres\\bin\\repserv.exe");
	SC_HANDLE	scm;
	SC_HANDLE	svc;
	DWORD	err;
	TCHAR	display_name[32];
	TCHAR	start_name[128];
	i4 	i;
	i4 	n = 0;
	char	*p;
	char	exe_path[MAX_LOC+1];
	TCHAR	depend_str[255], service_name[255], dsp_name[255];
	LPCTSTR	depend = depend_str;
	LOCATION	loc;
	char	*cptr, tchII_INSTALLATION[3];

	char	szUserName[64]="";
	HANDLE	hProcess, hAccessToken;
	UCHAR	InfoBuffer[1000], szDomainName[64];
	PTOKEN_USER	pTokenUser=(PTOKEN_USER)InfoBuffer;
	DWORD	dwInfoBufferSize, dwDomainSize=64;
	SID_NAME_USE	snu;
	DWORD	dwUserNameLen=sizeof(szUserName);

	NMgtAt("II_INSTALLATION", &cptr);
	STcopy(cptr, tchII_INSTALLATION);
	STprintf(depend_str, "Ingres_Database_%s", tchII_INSTALLATION);
	depend_str[STlength(depend_str)+1] = '\0';
	STprintf(service_name, "%s_%s", SVC_NAME, tchII_INSTALLATION);
	STprintf(dsp_name, "%s (%s)", DISPLAY_NAME, tchII_INSTALLATION);

	SIprintf("Installing %s . . .\n", dsp_name);
	SIflush(stdout);

        STprintf(exe_path, ERx("%%II_SYSTEM%%\\%s\\bin\\replsvc.exe"),
                 SystemLocationSubdirectory);

	*subkeypath = EOS;
	for (i = 0; i < sizeof(keynames) / sizeof(keynames[0]); ++i)
	{
		STcat(subkeypath, keynames[i]);
		STcat(subkeypath, "\\");
	}
	STcat(subkeypath, service_name);
	if ((ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, subkeypath, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subkey,
			&disp)) != ERROR_SUCCESS)
		abort_msg(ret, "RegCreateKeyEx");
	if (disp == REG_CREATED_NEW_KEY)
	{
		if ((ret = RegSetValueEx(subkey, TEXT("EventMessageFile"), 0,
				REG_EXPAND_SZ, (CONST BYTE *)exe_path,
				STlength(exe_path) + 1)) != ERROR_SUCCESS)
			abort_msg(ret, "RegSetValueEx");

		evt_types = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
			EVENTLOG_INFORMATION_TYPE;
		if ((ret = RegSetValueEx(subkey, TEXT("TypesSupported"), 0,
				REG_DWORD, (CONST BYTE *)&evt_types,
				sizeof(DWORD))) != ERROR_SUCCESS)
			abort_msg(ret, "RegSetValueEx");

		RegCloseKey(subkey);
	}		

	scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (scm == NULL)
		abort_msg(GetLastError(), "OpenSCManager");

	NMgtAt("II_SYSTEM", &p);
	if (p == NULL || *p == EOS)
	{
		SIprintf("II_SYSTEM has not been defined\n");
		CloseServiceHandle(scm);
		PCexit(FAIL);
	}
        STprintf(exe_path, "%s\\%s\\bin", p, SystemLocationSubdirectory);
	err = (DWORD)LOfroms(PATH, exe_path, &loc);
	if (err)
	{
		CloseServiceHandle(scm);
		abort_msg(err, ERx("LOfroms"));
	}

	err = (DWORD)LOfstfile(ERx("replsvc.exe"), &loc);
	if (err)
	{
		CloseServiceHandle(scm);
		abort_msg(err, ERx("LOfstfile"));
	}

	LOtos(&loc, &p);
	STcopy(p, exe_path);


	/*
	** Get the domain name that this user is logged in to.  This could
	** also match the machien name.
	*/
        hProcess = GetCurrentProcess();
        OpenProcessToken(hProcess,TOKEN_READ,&hAccessToken);
        GetTokenInformation(hAccessToken, TokenUser, InfoBuffer,
			    sizeof(InfoBuffer), &dwInfoBufferSize);
        LookupAccountSid(NULL, pTokenUser->User.Sid, szUserName,
                         &dwUserNameLen, szDomainName, &dwDomainSize, &snu);
        strcpy(start_name, szDomainName);
        strcat(start_name, "\\");
	strcat(start_name, szUserName);
	
	for (i = 1, n = 0; i <= num_servers; ++i)
	{
		STprintf(svc_name, ERx("%s_%d"), service_name, i);
		STprintf(display_name, ERx("%s %2d"), dsp_name, i);
		svc = CreateService(scm, svc_name, display_name,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			exe_path, NULL, NULL, depend, start_name, NULL);
		if (svc == NULL)
		{
			if ((err = GetLastError()) == ERROR_SERVICE_EXISTS)
				SIprintf("%s service already installed\n",
					display_name);
			else
			{
				CloseServiceHandle(scm);
				abort_msg(err, "CreateService");
			}
		}
		else
		{
			++n;
		}
		CloseServiceHandle(svc);
	}
	CloseServiceHandle(scm);
	SIprintf("%s service:  %d server%s installed\n", display_name, n,
		n == 1 ? "" : "s");
	if (n)
		SIprintf("Please update the DBA passwords using the Control Panel Services option\n");

	return (OK);
}


/*{
** Name:	rep_remove - remove Replicator service
**
** Description:
**	Removes the Replicator as a Windows NT service.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
**
** History:
**	28-jan-2000 (somsa01)
**	    For backward compatibility, remove the old-style service as well.
*/
STATUS
rep_remove()
{
	SC_HANDLE	svc, scm;
	DWORD		err;
	SERVICE_STATUS	status;
	i4 		i, n;
	HKEY		parentkey, subkey;
	LONG		ret;
	char		*cptr, *tchII_INSTALLATION;
	TCHAR		service_name[255], dsp_name[255];

	NMgtAt("II_INSTALLATION", &cptr);
	tchII_INSTALLATION = STalloc(cptr);
	STprintf(service_name, "%s_%s", SVC_NAME, tchII_INSTALLATION);
	STprintf(dsp_name, "%s (%s)", DISPLAY_NAME, tchII_INSTALLATION);

	SIprintf("Removing %s . . .\n", dsp_name);
	SIflush(stdout);

	scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (scm == NULL)
		abort_msg(GetLastError(), "OpenSCManager");

	/*
	** First, delete the old-style services, if they exist.
	*/
	for (i = 1, n = 0; i <= MAX_SERVERS; ++i, ++n)
	{
	    STprintf(svc_name, ERx("%s%d"), SVC_NAME, i);
	    svc = OpenService(scm, svc_name, SERVICE_ALL_ACCESS | DELETE);
	    if (svc)
	    {
		if (!QueryServiceStatus(svc, &status))
		    abort_msg(GetLastError(), "QueryServiceStatus");
		if (status.dwCurrentState != SERVICE_STOPPED)
		{
		    SIprintf("Stopping server %d ...\n", i);
		    if (!ControlService(svc, SERVICE_CONTROL_STOP, &status))
			abort_msg(GetLastError(), "ControlService");
		}

		if (!DeleteService(svc))
		    abort_msg(GetLastError(), "DeleteService");
		CloseServiceHandle(svc);
	    }
	    else
	    {
		if ((err = GetLastError()) == ERROR_SERVICE_DOES_NOT_EXIST)
		    break;
		else
		{
		    CloseServiceHandle(scm);
		    abort_msg(err, "OpenService");
		}
	    }
	}
	
	for (i = 1, n = 0; i <= MAX_SERVERS; ++i, ++n)
	{
		STprintf(svc_name, ERx("%s_%d"), service_name, i);
		svc = OpenService(scm, svc_name, SERVICE_ALL_ACCESS | DELETE);
		if (svc == NULL)
		{
			if ((err = GetLastError()) == ERROR_SERVICE_DOES_NOT_EXIST)
				break;
			else
			{
				CloseServiceHandle(scm);
				abort_msg(err, "OpenService");
			}
		}
	
		if (!QueryServiceStatus(svc, &status))
		{
			CloseServiceHandle(svc);
			CloseServiceHandle(scm);
			abort_msg(GetLastError(), "QueryServiceStatus");
		}
	
		if (status.dwCurrentState != SERVICE_STOPPED)
		{
			SIprintf("Stopping server %d ...\n", i);
			if (!ControlService(svc, SERVICE_CONTROL_STOP, &status))
			{
				CloseServiceHandle(svc);
				CloseServiceHandle(scm);
				abort_msg(GetLastError(), "ControlService");
			}
		}
	
		if (!DeleteService(svc))
		{
			CloseServiceHandle(svc);
			CloseServiceHandle(scm);
			abort_msg(GetLastError(), "DeleteService");
		}

		CloseServiceHandle(svc);
	}
	if (n)
		SIprintf("%s service:  %d server%s removed\n", dsp_name, n,
			n == 1 ? "" : "s");
	else
		SIprintf("%s service:  No servers to remove\n", dsp_name);
	CloseServiceHandle(scm);

	parentkey = HKEY_LOCAL_MACHINE;
	for (i = 0; i < sizeof(keynames) / sizeof(keynames[0]); ++i)
	{
		ret = RegOpenKeyEx(parentkey, keynames[i], 0, KEY_QUERY_VALUE,
			&subkey);
		if (ret != ERROR_SUCCESS)
			abort_msg(ret, "RegOpenKeyEx");
		parentkey = subkey;
	}

	if (RegOpenKeyEx(parentkey, SVC_NAME, 0, KEY_WRITE, &subkey)
		== ERROR_SUCCESS)
	{
		ret = RegDeleteKey(parentkey, SVC_NAME);
		if (ret != ERROR_SUCCESS)
			abort_msg(ret, "RegDeleteKey");
	}
	if (RegOpenKeyEx(parentkey, service_name, 0, KEY_WRITE, &subkey)
		== ERROR_SUCCESS)
	{
		ret = RegDeleteKey(parentkey, service_name);
		if (ret != ERROR_SUCCESS)
			abort_msg(ret, "RegDeleteKey");
	}

	return (OK);
}


/*{
** Name:	abort_msg - print error message and abort
**
** Description:
**	Prints a simple error message to stdout and exits
**
** Inputs:
**	msg_num - the identifier of the message
**	params - optional parameters to be filled into the using printf
**		conventions
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
abort_msg(
DWORD	err,
char	*where)
{
	SIprintf("Error %d in %s.\n", (i4)err, where);
	SIflush(stdout);
	PCexit(FAIL);
}

/*{
** Name:	rep_config - change password for replicator service
**
** Description:
**	Configures Replicator service.  This sets up
**	username, password for num_servers Replicator Server
**	specified.
**
** Inputs:
**	num_servers	- server number to configure
**	uname 		- configure service with this username
**	pwd		- configure service with this password
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
**
**
**
** History:
** 	07-Oct-2008 (drivi01)
**		Created.
*/
STATUS
rep_config(
i4   num_server,
char *uname,
char * pwd)
{
	char *cptr, tchII_INSTALLATION[3];
	char depend_str[255], service_name[255], dsp_name[255];
	SC_HANDLE	scm, svc;
	int ret=0;
	DWORD dwSize=0;

	NMgtAt("II_INSTALLATION", &cptr);
	STcopy(cptr, tchII_INSTALLATION);
	STprintf(service_name, "%s_%s_%d", SVC_NAME, tchII_INSTALLATION, num_server);
	STprintf(dsp_name, "%s (%s)", DISPLAY_NAME, tchII_INSTALLATION);

	SIprintf("Configuring %s . . .\n", dsp_name);
	SIflush(stdout);

	scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (scm == NULL)
		abort_msg(GetLastError(), "OpenSCManager");

	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	svc = OpenService (scm, service_name, SERVICE_ALL_ACCESS);
	if (svc == NULL)
		abort_msg(GetLastError(), "OpenService");

	dwSize=sizeof(dsp_name);
	if (!GetServiceDisplayName(scm, service_name, &dsp_name, &dwSize))
	{
		DWORD dwError = GetLastError();
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		abort_msg(dwError, "GetServiceDisplayName");
	}

	ret = ChangeServiceConfig(
		svc,
		SERVICE_NO_CHANGE,
		SERVICE_NO_CHANGE,
		SERVICE_NO_CHANGE,
		NULL,
		NULL,
		NULL,
		NULL,
		uname,
		pwd,
		dsp_name);
	
	if (!ret)
	{
		DWORD dwError = GetLastError();
		CloseServiceHandle(svc);
		CloseServiceHandle(scm);
		abort_msg(dwError, "ChangeServiceConfig");
	}
	CloseServiceHandle(svc);
	CloseServiceHandle(scm);

	SIprintf("Username/Password successfully configured for %s . . .\n", dsp_name);

	return (OK);
}

