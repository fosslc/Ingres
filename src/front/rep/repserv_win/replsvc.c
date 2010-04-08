/*
** Copyright (c) 1994, 2000 Ingres Corporation
*/

# ifndef NT_GENERIC
# error This should only be built under Windows NT
# endif

/**
** Name:	replsvc.c - Replicator service
**
** Description:
**      Process to load replicator servers as a service.
**      main		    main routine for the Replicator service
**      ReplicatorService   initial service entry point
**      RepCtrlHandler      handler for events from service control manager
**      StartRepServer      starts the replication process
**      StopRepServer       stops the replication process
**      RefreshProcessEnv   updates the environment for the current process
**      ReportRepEvent      reports events into system event log
**      ReportStatusToSCMgr reports status to the service control manager
**      StopService         terminates the service
**	SignalRepTermEvent  signal termination event
**
**      This module serves as the interface between the service control manager
**      (SCM) and the replicator process.
**
**      The replproc will now report that the service runnning and wait for
**      either a termination signal from the SCM or a process termination
**      signal from the replicator process.  Either of these events will allow
**      replproc to report the the SCM that the service has stoped.
**
**      Stopping the replicator service via the SCM will open a session to the
**      replictor database and raise a dbevent to dd_stop_server#. Allowing
**      the replicator to perform its normal shutdown.
**
** History:
**	13-jan-97 (joea)
**		Created based on replsvc.c in replicator library.
**      23-Jun-97 (fanra01)
**          Modified based on service start programs servproc.c and replsvc.c.
**      06-Aug-97 (fanra01)
**          If the environment does not exist then indicate that the service
**          has stopped.  This will enable the parent process to detect a
**          service status change and exit.
**      14-Aug-97 (fanra01)
**          Reduced wait hint times to reflect time to update next service
**          status.
**	22-mar-98 (mcgem01)
**	    Replace repserver executable name and location.
**	28-oct-98 (abbjo03)
**	    Merge SignalRepTermEvent from repterm.sc and convert to OpenAPI.
**	16_dec-98 (stoli02)
**	    Create the location by calling LOfroms() before concatenating
**	    the server path using LOfaddpath().
**	23-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters by IIAPI_GETEINFOPARM
**		error parameter blocks.
**	22-apr-99 (abbjo03)
**		Add envHandle parameter to IIsw_terminate.
**	09-aug-1999 (mcgem01)
**	    Replace nat with i4.
**	28-jan-2000 (somsa01)
**	    Services are now keyed off of the installation identifier.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**	29-nov-2004 (rigka01) bug 111394, problem INGSRV2624
**	    Cross the part of change 466624 from main to ingres26 that
**	    applies to bug 111394 (change 466196). 
**/

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <cv.h>
# include <nm.h>
# include <er.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>

# include "repntsvc.h"
# include "repmsg.h"

FUNC_EXTERN STATUS iimksec(SECURITY_ATTRIBUTES *sa);
VOID SignalRepTermEvent(POPTVALS povRepFlags, i4 nServerNum);

VOID ReplicatorService(DWORD dwArgc, LPSTR *lpszArgv);
VOID RepCtrlHandler(DWORD dwCtrlCode);
VOID ReportRepEvent(LPTSTR lpszMsg, STATUS status);
BOOL RefreshProcessEnv(void);
BOOL StopRepServer(void);
int  StartRepServer (HANDLE * hChild, LPSTR lpszName);
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                         DWORD dwCheckPoint, DWORD dwWaitHint);
VOID StopService(LPTSTR lpszMsg);
STATUS LoadOpt(POPTVALS povRepFlags);

OPTVALS ovRepFlags;
SERVICE_STATUS_HANDLE   sshStatusHandle;
SERVICE_STATUS          ssServiceStatus;
DWORD                   dwGlobalErr;
DWORD                   dwGlobalStatus;
HANDLE                  hServDoneEvent;
HANDLE                  hRepDoneEvent;
BOOL                    isstarted;
STARTUPINFO             siStartInfo;
PROCESS_INFORMATION     piProcInfo;
CHAR                    chMsg[256];

TCHAR                   tchServiceName[255] = ""; /* overridden below */
HANDLE                  hHeap = NULL;
LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
TCHAR			tchII_SYSTEM[ _MAX_PATH];

static i4 nServerNum = 0;

static DWORD	main_argc;
static LPTSTR	*main_argv;


/*{
** Name:	main - main routine for the Replicator service
**
** Description:
**	Main routine for the Replicator NT service.  Registers with the
**	Service Control Manager.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK on success
**      FAIL on failure
**
** History:
**	28-jan-2000 (somsa01)
**	    Services are now keyed off of the installation identifier.
*/
i4
main(int argc, char *argv[])
{
TCHAR   tchFailureMessage[ 256];
char	*BinaryName, *cptr;

SERVICE_TABLE_ENTRY     lpServerServiceTable[] =
{
    { tchServiceName,
      ( LPSERVICE_MAIN_FUNCTION) ReplicatorService},
    { NULL, NULL }
};

    /*
    ** Let's get II_SYSTEM from the service executable path which
    ** is passed in as an argument via the SCM. This is so that we
    ** can run NMgtAt() on II_INSTALLATION.
    */
    BinaryName = (char *)MEreqmem(0, STlength(argv[0])+1, TRUE, NULL);
    STcopy(argv[0], BinaryName);
    cptr = STstrindex(BinaryName, "\\ingres\\bin\\replsvc.exe", 0, TRUE);
    *cptr = '\0';
    STcopy(BinaryName, tchII_SYSTEM);
    SetEnvironmentVariable(SYSTEM_LOCATION_VARIABLE, tchII_SYSTEM);
    NMgtAt("II_INSTALLATION", &cptr);
    sprintf( tchServiceName, "%s_%s_%s_", SYSTEM_SERVICE_NAME,
             SYSTEM_COMPONENT_NAME, cptr );
    MEfree(BinaryName);

        if (!StartServiceCtrlDispatcher( lpServerServiceTable ))
        {
            sprintf( tchFailureMessage, "Failed to start %s %s Server",
                     SYSTEM_SERVICE_NAME, SYSTEM_COMPONENT_NAME);
            ReportRepEvent(tchFailureMessage, FAIL);
            return (FAIL);
        }
        return (OK);
}


/*
** Name: ReplicatorService
**
** Description:
**      Register the control handling function and start the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**      23-Jun-97 (fanra01)
**          Created.
**      07-Aug-97 (fanra01)
**          Check return of OK from StartRepServer.  Ensure that dwGlobalErr is
**          cleared on successful completion.
**		01-Aug-2000 (fanch01)
**			Add new optional service name parameter to StartRepServer() call.
**			Bug #102244.
*/

VOID
ReplicatorService(DWORD dwArgc, LPSTR *lpszArgv)
{
DWORD                   dwWFS;
STATUS                  dbstatus;
HANDLE hChildProcess = NULL;

    /*
    ** Register contorl handling function with the control dispatcher.
    */

    sshStatusHandle = RegisterServiceCtrlHandler (
                                    tchServiceName,
                                    (LPHANDLER_FUNCTION) RepCtrlHandler);

    if (sshStatusHandle )
    {
        ssServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
        ssServiceStatus.dwServiceSpecificExitCode = 0;

        /*
        ** Update status of SCM to SERVICE_START_PENDING
        */
        if (ReportStatusToSCMgr (SERVICE_START_PENDING, NO_ERROR, 2, 10000))
        {
            /*
            **  Get handle to process heap
            */
            hHeap = GetProcessHeap();

            /*
            **  Get the server number from the command line
            */
            main_argc = dwArgc;
            main_argv = lpszArgv;
            CVan(*lpszArgv + strlen(tchServiceName) , &nServerNum);
            if (!nServerNum)
            {
                ReportRepEvent (TEXT("Server number not specified"), FAIL);
                (VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
                return;
            }

            /*
            ** Create an event that will handle termination.
            */
            if ((hServDoneEvent=CreateEvent (NULL, TRUE, FALSE, NULL)) != NULL)
            {
                dbstatus = StartRepServer(&hChildProcess, main_argc < 2 ? NULL : main_argv[1]);

                if (dbstatus == OK)
                {
                    /*
                    ** Update status of SCM to SERVICE_RUNNING
                    */
                    if (ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR,
                                             0, 0))
                    {
                    HANDLE hObjects[2];
                            hObjects[0] = hServDoneEvent;
                            hObjects[1] = hChildProcess;
                        STprintf (chMsg, ERx("Replicator Server %d started"),
                                  nServerNum);
                        ReportRepEvent (chMsg,OK);
                        /*
                        ** Wait for terminating event.
                        */
                        dwWFS = WaitForMultipleObjects( 2,
                                                        hObjects,
                                                        FALSE,
                                                        INFINITE);
                        if (dwWFS > 0)
                        {
                            switch (dwWFS - WAIT_OBJECT_0)
                            {
                                case 1:
                                STprintf (chMsg,
                                ERx("Replicator Server %d process exited."),
                                nServerNum);
                                   ReportRepEvent (chMsg,OK);
                                dwGlobalErr = 0;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        ReportRepEvent (TEXT("Register service failed"), FAIL);
    }
    if (hServDoneEvent != NULL)
        CloseHandle(hServDoneEvent);
    if (sshStatusHandle != 0)
        (VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
    if (hHeap)
        CloseHandle(hHeap);
    return;
}


/*
** Name: RepCtrlHandler
**
** Description:
**     Handles control requests for OpenIngres Replicator Service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**      23-Jun-97 (fanra01)
**         Created.
**
*/
VOID
RepCtrlHandler(DWORD dwCtrlCode)
{
DWORD dwState = SERVICE_RUNNING;
DWORD nRet;

    switch(dwCtrlCode)
    {
        case SERVICE_CONTROL_PAUSE:
            break;

        case SERVICE_CONTROL_CONTINUE:
            break;

        case SERVICE_CONTROL_STOP:
            dwState = SERVICE_STOP_PENDING;
            ReportStatusToSCMgr( SERVICE_STOP_PENDING, NO_ERROR, 1, 20000);
            nRet = StopRepServer();
            SetEvent(hServDoneEvent);
            return;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            break;
    }
    ReportStatusToSCMgr (dwState, NO_ERROR, 0, 0);
}

/*
** Name: StartRepServer
**
** Description:
**      Runs repserv.exe.
**
** Inputs:
**      None
**
** Outputs:
**      hChild      Handle of the created child process.
**
** Returns:
**      OK
**      !OK
**
** History:
**      23-Jun-97 (fanra01)
**          Created.
**      06-Aug-97 (fanra01)
**          If the environment does not exist then indicate that the service
**          has stopped.  This will enable the parent process to detect a
**          service status change and exit.
**          Clear the sshStatusHandle to invalidate it.
**	01-Aug-2000 (fanch01)
**    	    Add optional replication service name argument as override to
**	    default replicator name.  Bug #102244.
**	28-jan-2000 (somsa01)
**	    Set the PATH variable for the service, just in case we would
**	    need it.
*/
int
StartRepServer (HANDLE * hChild, LPSTR lpszName)
{
    TCHAR	tchServiceImage[ _MAX_PATH];
    char	dir[MAX_LOC+1];
    char	subdir[16];
    DWORD	nret;
    char	*p;
    LOCATION	reploc;
    LOCATION	binloc;
    CHAR	temp = '\0';
    LPTSTR	tchBuf;

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.lpReserved = NULL;
    siStartInfo.lpDesktop = NULL;
    siStartInfo.lpTitle = NULL;
    siStartInfo.lpReserved2 = NULL;
    siStartInfo.cbReserved2 = 0;
    siStartInfo.dwFlags = 0;

    /*
    **  Set the PATH for the service.
    */
    nret=GetEnvironmentVariable("PATH", &temp, 1);
    if (nret>0)
    {
	nret += 1024;  /* This is to make room for the add-ons later. */
	tchBuf = (char *)MEreqmem(0, nret, TRUE, NULL);
	GetEnvironmentVariable("PATH", tchBuf, nret);
    }
    else
    {
	/* Just make room for the add-ons. */
	tchBuf = (char *)MEreqmem(0, 1024, TRUE, NULL);
	GetSystemDirectory(tchBuf, 1024);  /* Also add the System path. */
    }

    STcat( tchBuf, ";" );
    STcat( tchBuf, tchII_SYSTEM);
    STcat( tchBuf, "\\" );
    STcat( tchBuf, SYSTEM_LOCATION_SUBDIRECTORY );
    STcat( tchBuf, "\\utility;");
    STcat( tchBuf, tchII_SYSTEM);
    STcat( tchBuf, "\\");
    STcat( tchBuf, SYSTEM_LOCATION_SUBDIRECTORY );
    STcat( tchBuf, "\\bin");

    SetEnvironmentVariable("PATH", tchBuf);
    MEfree(tchBuf);

    NMgtAt(ERx("DD_RSERVERS"), &p);
    if (p == NULL || *p == EOS)
    {
    	/* look up II_SYSTEM\ingres\rep */
	if (NMloc(SUBDIR, PATH, ERx("rep"), &reploc) != OK)
	{
	    ReportRepEvent(TEXT("II_SYSTEM environment variable not set"),
	    	FAIL);
	    if (sshStatusHandle != 0)
	    {
		(VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
		sshStatusHandle = 0;
	    }
	    return(FAIL);
	}
	LOtos(&reploc, &p);
    }
    STcopy(p, dir);
    LOfroms(PATH, dir, &reploc);
    LOfaddpath(&reploc, ERx("servers"), &reploc);
    STprintf(subdir, ERx("server%d"), nServerNum);
    LOfaddpath(&reploc, subdir, &reploc);
    if (LOchange(&reploc) == OK)
    {
        if (LoadOpt(&ovRepFlags) == OK)
        {
	    NMloc(BIN, PATH, NULL, &binloc);
	    LOtos(&binloc, &p);
            STprintf( tchServiceImage, "%s\\%s", p, COMPONENT_REP_PROCESS );

            if ((isstarted = CreateProcess(
                                            tchServiceImage,
                                            NULL,
                                            NULL,
                                            NULL,
                                            TRUE,
                                            NORMAL_PRIORITY_CLASS,
                                            NULL,
                                            NULL,
                                            &siStartInfo,
                                            &piProcInfo)) == TRUE)
            {
                *hChild = piProcInfo.hProcess;
                return (OK);
            }
            else
            {
                dwGlobalErr = GetLastError();
            }
        }
    }
    else
    {
        ReportRepEvent (TEXT("Error changing directory"), FAIL);
    }

    if (hServDoneEvent != NULL)
        CloseHandle(hServDoneEvent);

    if (sshStatusHandle != 0)
    {
        (VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
        sshStatusHandle = 0;
    }

    return (FAIL);
}

/*
** Name: StopRepServer
**
** Description:
**      Sets the event which causes the OpenIngres replicator server to stop.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      TRUE success
**
** History:
**      24-Jun-97 (fanra01)
**          Created.
*/

BOOL
StopRepServer()
{
    SignalRepTermEvent(&ovRepFlags, nServerNum);
    STprintf (chMsg, ERx("Replicator Server %d stopped"), nServerNum);
    ReportRepEvent (chMsg,OK);
    SetEvent (hRepDoneEvent);
    return(TRUE);
}

/*{
** Name:	RefreshProcessEnv - initialize the environment
**
** Description:
**	Recreates the environment by querying the registry.  This is
**	necessary because NT services do not have the environment of the
**	user they log on as.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**      TRUE    successful
**      FAIL    unsuccessful
*/
BOOL
RefreshProcessEnv()
{
HKEY    envkey;
TCHAR   class[MAX_PATH];
DWORD   classlen = MAX_PATH;
DWORD   num_subkeys, max_subkey, max_class;
DWORD   num_values, max_value_name, max_value_data;
DWORD   sec_desc;
FILETIME    filetime;
LPTSTR  name;
BYTE    *data;
DWORD   name_len, data_len;
DWORD   i;
DWORD   value_type;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), 0,
                     KEY_QUERY_VALUE, &envkey) != ERROR_SUCCESS)
    {
        ReportRepEvent(TEXT("HKEY_LOCAL_MACHINE Environment"), FAIL);
        return (FALSE);
    }

    if (RegQueryInfoKey( envkey, class, &classlen, NULL,
                         &num_subkeys, &max_subkey, &max_class,
                         &num_values, &max_value_name,
                         &max_value_data, &sec_desc,
                         &filetime) != ERROR_SUCCESS)
    {
        ReportRepEvent(TEXT("Environment subkey"), FAIL);
        return (FALSE);
    }

    if ((name = HeapAlloc(hHeap, 0, ++max_value_name)) == NULL)
    {
        ReportRepEvent(TEXT("HeapAlloc"), FAIL);
        return (FALSE);
    }

    if ((data = HeapAlloc(hHeap, 0, max_value_data)) == NULL)
    {
        ReportRepEvent(TEXT("HeapAlloc"), FAIL);
        return (FALSE);
    }

    for (i = 0; i < num_values; ++i)
    {
        name_len = max_value_name;
        data_len = max_value_data;
        if (RegEnumValue(envkey, i, name, &name_len, NULL, &value_type,
                         data, &data_len) != ERROR_SUCCESS)
        {
            ReportRepEvent(TEXT("Environment enumeration"), FAIL);
            return (FALSE);
        }
        if (!SetEnvironmentVariable(name, (LPTSTR)data))
        {
            ReportRepEvent(name, FAIL);
            return (FALSE);
        }
    }
    return (TRUE);
}

/*
** Name: ReportRepEvent
**
** Description:
**      Write events to the event log.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**      24-Jun-97 (fanra01)
**	    Created.
**      07-Aug-97 (fanra01)
**          Only report error if status indicates failure.
**
*/

VOID
ReportRepEvent(LPTSTR lpszMsg, STATUS status)
{
HANDLE	hEventSource;
LPTSTR	lpszStrings[2];
CHAR	chMsg[256];

    dwGlobalErr = (status == FAIL) ? GetLastError() : 0;

    hEventSource = RegisterEventSource( NULL, tchServiceName );

    sprintf( chMsg, "%s error: %d %d",
	     SYSTEM_SERVICE_NAME, dwGlobalErr, dwGlobalStatus );

    lpszStrings[0] = chMsg;
    lpszStrings[1] = lpszMsg;

    if (hEventSource != NULL)
    {
        if (status == FAIL)
        {
            ReportEvent(
                        hEventSource,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        E_REP_FAILED,
                        NULL,
                        2,
                        0,
                        lpszStrings,
                        NULL);
        }
        else
        {
            ReportEvent(
                        hEventSource,
                        EVENTLOG_INFORMATION_TYPE,
                        0,
                        E_REP_SUCCEDED,
                        NULL,
                        1,
                        0,
                        &lpszStrings[1],
                        NULL);
        }
        DeregisterEventSource(hEventSource);
    }
    return;
}

/*
** Name: ReportStatusToSCMgr
**
** Description:
**     Report the status to the service control manager.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
**      24-Jun-97 (fanra01)
**          Created.
*/

BOOL
ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint, DWORD dwWaitHint)
{
    BOOL fResult;

    if (dwCurrentState == SERVICE_START_PENDING)
        lpssServiceStatus->dwControlsAccepted = 0;
    else
        lpssServiceStatus->dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ssServiceStatus.dwCurrentState  = dwCurrentState;
    ssServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssServiceStatus.dwCheckPoint    = dwCheckPoint;
    ssServiceStatus.dwWaitHint      = dwWaitHint;

    if (!(fResult = SetServiceStatus( sshStatusHandle, lpssServiceStatus)))
    {
        StopService("SetServiceStatus");
    }

    return fResult;
}

/*
** Name: StopService
**
** Description:
**      Stop the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
**      24-Jun-97 (fanra01)
**          Created.
*/

VOID
StopService(LPTSTR lpszMsg)
{
    HANDLE  hEventSource;
    LPTSTR	lpszStrings[2];

    dwGlobalErr = GetLastError();

    hEventSource = RegisterEventSource( NULL, tchServiceName );

    sprintf(chMsg, "%s error: %d", SYSTEM_SERVICE_NAME, dwGlobalErr);
    lpszStrings[0] = chMsg;
    lpszStrings[1] = lpszMsg;

    if (hEventSource != NULL)
    {
        ReportEvent(hEventSource,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    0,
                    NULL,
                    2,
                    0,
                    lpszStrings,
                    NULL);
        (VOID) DeregisterEventSource(hEventSource);
    }
    SetEvent(hServDoneEvent);
}

/*{
** Name:        LoadOpt         - Load the runrepl.opt flags for scanning
**
** Description:
**      Opens the runrepl.opt file in the current working directory.
**      Reads each line in turn and passes it to the checking function.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
STATUS
LoadOpt(POPTVALS povRepFlags)
{
STATUS status = OK;
FILE            *pFile;
LOCATION        loLoc;
char            szFilename[MAX_LOC+1];
char            szOption[80];
i4              nLen;
i4              i;

    STcopy(ERx("runrepl.opt"),szFilename);
    LOfroms(FILENAME, szFilename, &loLoc);
    if (SIfopen(&loLoc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &pFile) != OK)
    {
        ReportRepEvent(TEXT("Open option file failed"), FAIL);
        status = FAIL;
    }
    else
    {
        while ((status == OK) &&
               (SIgetrec(szOption, sizeof(szOption), pFile) != ENDFILE))
        {
            STtrmwhite(szOption);
            if ((nLen = STlength(szOption)) < 4)
            {
                ReportRepEvent(TEXT("Open option file failed"), FAIL);
                status = FAIL;
            }
            else
            {
            char* p;
            char* pNode;

                for (i = 0, p = szOption; *p != EOS && i < 4; CMnext(p), i++);

                if (STbcompare(szOption, nLen, ERx("-IDB"), 4, 1) == 0)
                {
                    STtrmwhite(p);
                    pNode = STindex(p, ERx(":"), 0);
                    if (pNode == NULL)
                    {
                        *povRepFlags->szNode = EOS;
                        STlcopy (p, povRepFlags->szDBname, DB_MAXNAME);
                    }
                    else
                    {
                        *pNode = EOS;
                        STlcopy(p, povRepFlags->szNode, DB_MAXNAME);
                        CMnext(pNode);
                        CMnext(pNode);
                        STlcopy(pNode, povRepFlags->szDBname, DB_MAXNAME);
                    }
                }
            }
        }
        SIclose(pFile);
    }
    return (status);
}


/*
** Name:	SignalRepTermEvent - signal termination event
**
** Description:
**      Function to signal the replicator termination dbevent.
**      First must parse the runrepl.opt file for the database name then
**      connect to the database an send the termination event.
**
** History:
**      23-Jun-97 (fanra01)
**          Created.
**	28-oct-98 (abbjo03)
**		Changed to use OpenAPI.
**	26-jan-2004 (penga03)
**	    Added an argument for IIsw_initialize.    
**
*/
VOID
SignalRepTermEvent(
POPTVALS	povRepFlags,
i4 		nServerNum)
{
    char	szFullNode[DB_MAXNAME * 2 + 3];
    char	evtName[DB_MAXNAME+1];
    II_PTR	connHandle = NULL;
    II_PTR	tranHandle = NULL;
    II_PTR	stmtHandle;
    IIAPI_GETEINFOPARM	errParm;

    if (*povRepFlags->szNode == EOS)
        STprintf(szFullNode, "%s", povRepFlags->szDBname);
    else
        STprintf(szFullNode, "%s::%s", povRepFlags->szNode,
	    povRepFlags->szDBname);

    if (IIsw_initialize(NULL,0) != IIAPI_ST_SUCCESS)
	return;
    if (IIsw_dbConnect(szFullNode, NULL, NULL, &connHandle, NULL, &errParm) ==
    	IIAPI_ST_SUCCESS)
    {
	STprintf(evtName, ERx("dd_stop_server%d"), nServerNum);
	IIsw_raiseEvent(connHandle, &tranHandle, evtName, NULL, &stmtHandle,
	    &errParm);
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&tranHandle, &errParm);
	IIsw_disconnect(&connHandle, &errParm);
    }
    IIsw_terminate(NULL);
}
