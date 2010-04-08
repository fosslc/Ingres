/*
**  Copyright (c) 1995, 2006 Ingres Corporation
*/

/*
**  Name: servproc.c
**
**  Description:
**
**	Used by the NT Service Manager to start Ingres server using
**	ingstart.exe and to stop Ingres using instop.exe
**
**	The service runs as a daemon using the ingres login user account 
**	without the user being logged in as ingres.
**
**  History:
**	15-jul-95 (emmag)
**	    Created.
**	09-aug-95 (emmag)
**	    ingstart.exe and ingstop.exe have been moved to the utility
**	    directory rather than the bin.
**      11-aug-95 (emmag)
**          Run ingstart and ingstop as high priority processes to 
**	    improve performance when starting up and shutting down
**          the service..
**	16-dec-95 (emmag)
**	    Registry entries don't always take effect until the machine
**	    is rebooted.  This will mean that we won't necessarily have
**	    ingres\bin and ingres\utility in the path.  Previously we
**	    got around this by appending the utility directory to the
**	    path for this process, now we append both bin and utility.
**	17-dec-95 (emmag)
**	    Update the status regularly during shutdown. Clean up
**	    compiler warnings.
**	31-mar-97 (somsa01)
**	    Fixed bug 81337; in StartIngresServer(), we now allocate space for
**	    the PATH variable dynamically.
**      14-may-97 (mcgem01)
**          Clean up compiler warnings.
**	08-jan-1998 (somsa01)
**	    Added the necessary code to implement NT's version of setuid.
**	    (Bug #87751)
**	29-jan-98 (mcgem01)
**	    Clean up compiler warnings.
**	12-feb-1998 (canor01)
**	    Quote pathname, since it may contain embedded spaces.
**	16-feb-98 (mcgem01)
**	    If Ingstart or Ingstop fail for any reason, then we shouldn't show
**	    the service as started or stopped successfully.   Add an extra
**	    parameter to ReportIngresEvents so that we can report informative
**	    as well as failure messages.
**	19-feb-1998 (somsa01)
**	    We also need to pass the current working directory of the
**	    process who runs the setuid stuff.  (Bug #89006)
**	25-feb-1998 (somsa01)
**	    We now have an input file for the spawned process' stdin.
**	21-mar-98 (mcgem01)
**	    Include the error message header for reporting messages
**	    to the event log.
**	10-apr-98 (mcgem01)
**	    Product name change from OpenIngres to Ingres.
**	28-apr-1999 (mcgem01)
**	    Updated for the different product project.
**	11-jun-1999 (somsa01)
**	    Added functionality which will allow ingstart to bind different
**	    pieces of Ingres (if used to start them separately) with the
**	    Ingres service (if it is started).
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	03-sep-1999 (mcgem01)
**	    When shutting down the different product service,
**	    hard shut down the comms server regardless of whether it has
**	    active connections or not.
**	19-oct-99 (fanra01, cucjo01)
**	    (X-Integration from Bug #99211, Change #443543)
**	    Changed behavior of stopping the Ingres Service.  Originally
**	    the stop was implemented in the Control Handler, but in
**	    Windows 2000, if we didn't return in 30 seconds, the Control
**	    Handler would time out and the service would fail to stop.
**	    To work properly, the StopServer() function is now started as
**	    a thread and the Control Handler returns in a timely manner.
**	22-jan-2000 (somsa01)
**	    Append the Installation Identifier to the end of the service
**	    name. Also, add conditional running of 'ingstart' and 'ingstop'.
**	27-jan-2000 (somsa01)
**	    When running 'ingstart', pass the '-service' flag.
**	27-jan-2000 (somsa01)
**	    Fixed typo in StartIngresServer().
**	30-jan-2000 (mcgem01)
**	    Back out Sam's change to pass the '-service' flag.
**	10-jun-2000 (mcgem01)
**	    Set the path to the servproc process for all products and add
**	    product path equivalents.
**	22-dec-2000 (somsa01)
**	    In RunProcessAsIngres(), convert the result of GetUserName()
**	    to all lower case, as is done by IDname().
**	29-jan-2001 (somsa01)
**	    Added the ability to run ntrcpcfg.exe .
**	30-mar-2001 (somsa01)
**	    Added closing of process and thread handles created during
**	    execution of various CreateProcess() and CreateThread()
**	    functions to suppress handle leaks.
**	30-mar-2001 (somsa01)
**	    Added closing of process and thread handles created during
**	    execution of various CreateProcess() and CreateThread()
**	    functions to suppress handle leaks.
**	17-apr-2001 (somsa01)
**	    Added execution of the post installation executable,
**	    iipostinst.exe, if PostInstallationNeeded is set in the
**	    registry.
**	19-may-2001 (somsa01)
**	    Corrected previous cross-integration.
**	24-may-2001 (somsa01)
**	    Corrected when to retrieve II_INSTALLATION from the registry
**	    as well as the setting of II_TEMPORARY.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be created in the global name
**	    space.
**	07-jul-2001 (somsa01)
**	    In RunProcessAsIngres(), we now properly handle embedded spaced
**	    II_SYSTEM for startup commands.
**	15-aug-2001 (somsa01)
**	    In GetRegValue(), make sure we properly set dwSize before
**	    using it in RegQueryValueEx().
**	25-aug-2001 (somsa01)
**	    Instead of using 0xFFFFFFFF, use INVALID_HANDLE_VALUE.
**      29-Nov-2001 (fanra01)
**          Bug 106529
**          Change scope of include for ingmsg.h to reflect its change of
**          location.
**	17-jan-2002 (somsa01)
**	    When running "ingstart", pass the "-insvcmgr" flag.
**	14-feb-2002 (somsa01)
**	    In RunProcessAsIngres(), added a Sleep() call to give the
**	    CPU a break while we're waiting.
**	15-feb-2002 (somsa01)
**	    Changed registry entry used in GetRegValue() to conform to
**	    the CA standard.
**	20-mar-2002 (somsa01)
**	    In RunProcessAsIngres(), set the exit code of the child process
**	    in setuid.ExitCode.
**	26-mar-2002 (somsa01)
**	    In StartIngresServer(), remove II_TEMPORARY initially from our
**	    environment, since it may be left over from a previous
**	    installation.
**	29-mar-2002 (somsa01)
**	    In RunProcessAsIngres(), only set the exit code of the child
**	    process in setuid.ExitCode if it actually ran the child
**	    process; otherwise, it is -1.
**	06-dec-2002 (mcgem01)
**	    Register for the system shutdown message and attempt to 
**	    stop Ingres before the system shuts down.
**	27-Jan-2003 (fanar01)
**	    Bug 109284
**	    Register a handler function for the console control events.
**	    See MS article Q182561 since service control events don't
**	    seem to be arriving.
**      12-Feb-2003 (fanra01)
**          Bug 109284
**          During system shutdown if another service holds sessions
**          open then the dbms server does not shutdown and is reopened.
**          Add an ingstop option to terminate the server.
**	13-feb-2003 (somsa01)
**	    In StartIngresServer(), before starting Ingres, set up the
**	    setuid stuff.
**	11-mar-2003 (somsa01)
**	    The registry location for 64-bit Ingres is "Advantage Ingres".
** 	01-mar-2004 (wonst02)
** 	    Check the server status periodically (for High Availability
**	    support).
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, Cleaned-up warnings.
**	26-Jul-2004 (lakvi01)
**	    Backed-out the above change to keep the open-source stable.
**	    Will be revisited and submitted at a later date. 
** 	26-Jul-2004 (penga03)
** 	    Changed the registry location to 
**	    HKLM\Software\ComputerAssociates\Ingres.
**	29-Sep-2004 (drivi01)
**	    Added NEEDLIBS hint for generating Jamfile
**  	08-Mar-2005 (lakvi01)
**	    Added error checking in GetRegValue() for invalid registry handle
**	    of tree "Software\\ComputerAssociates\\Ingres".
**	29-mar-2005 (rigka01) bug 114109, INGSRV3211
**	    privileged instruction exception during Ingres shutdown or
**	    startup via Services
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SERVICE_PRODUCT_NAME with SERVICE_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**	28-Jun-2006 (drivi01)
**	    SIR 116381
**	    iipostinst.exe is renamed to ingconfig.exe. Fix all calls to
**	    iipostinst.exe to call ingconfig.exe instead.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	15-Apr-2009 (drivi01)
**          Move clusapi.h higher in the list of header includes.
*/

/*
**
NEEDLIBS = COMPATLIB CLUSAPI
**
*/
#define _WIN32_WINNT 0x0500

#include <compat.h>
#include <clusapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <cs.h>
#include <st.h>
#include <me.h>
#include <nm.h>
#include <gl.h>
#include <pc.h>
#include <cv.h>
#include <cm.h>
#include "servproc.h"
#include <ingmsg.h>


/*
** Definitions for ingstop options
*/
# define STOP_SERVER        0x0001
# define SERVER_SERVICE     0x0002
# define SERVER_KILL        0x0004
# define SERVER_FORCE       0x0008
# define SERVER_IMMED       0x0010
# define SERVER_SVC_KILL    0x0020
# define SERVER_CHECK	    0x0040
# define STOP_SERVICE       0x1000

/*
** ingstop options ordered according to bit mask position.
*/
char* stop_opt[16] = { "", "-service ", "-kill ", "-force ", "-immediate ", "-servicekill ", "-check", "", NULL };

# define SERVPROC_LOG   "II_SERVPROC_LOG"

void InitializeGlobals(void);
static void GetRegValue(char *strKey, char *strValue, BOOL CloseKey);
# ifdef xDEBUG
void ServProcTrace( char* fmt, ... );
# define SERVPROCTRACE  ServProcTrace
# else
# define SERVPROCTRACE( x, y )
# endif

TCHAR tchServiceName[255] = "Ingres_Database"; /* overridden below */

char	*ObjectPrefix;
DWORD dwClusterState = ClusterStateNotInstalled;
CRITICAL_SECTION critStopServer;

char *validSetuidCmds[] = {
	"dmfjsp",
	"ntlockst",
	"ntlogdmp",
	"ntlogst",
	"ntrcpst",
	"ntrcpcfg",
	"ntvrfydb",
	"ntupgrdb",
	NULL
};

char *validStartupCmds[] = {
	"iirun",
	"jasrun",
	NULL
};

/*
** Name: CtrlHandlerRoutine
**
** Description:
**      Console control handler function.  This function is called by the
**      OS to signal the following events:
**          CTRL_C_EVENT
**          CTRL_BREAK_EVENT
**          CTRL_CLOSE_EVENT
**          CTRL_LOGOFF_EVENT
**          CTRL_SHUTDOWN_EVENT
**
** Inputs:
**      dwCtrlType      Event from the OS
**
** Outputs:
**      None
**
** Return:
**      TRUE            Event handled in the handler routine
**      FALSE           Event not handled and should be passed to next handler
**
** History:
**      27-Jan-2003 (fanra01)
**          Created.
*/
BOOL
CtrlHandlerRoutine(DWORD dwCtrlType)
{
    DWORD StopType = 0;
    BOOL status = FALSE;
    switch(dwCtrlType)
    {
        case CTRL_CLOSE_EVENT:
            /*
            ** If a true is returned for this event and the service
            ** interacts with the desktop then a dialog to end the
            ** application is displayed.
            */
            status = TRUE;
            break;

        case CTRL_SHUTDOWN_EVENT:
            SERVPROCTRACE( "%s\n", "ServProc - CtrlHandlerRoutine shutdown" );
            /*
            ** StopType is now a set of bit flags for enabling ingstop
            ** command options.
            */
            StopType = (STOP_SERVER | SERVER_SERVICE | SERVER_FORCE);
            status = StopServer( (LPVOID)StopType ) == OK ? FALSE : TRUE;
            break;
    }
    /*
    ** If the StopServer was successful return false to indicate
    ** default handler action otherwise return true to indicate
    ** event has been handled.
    */
    return(status);
}

/*
** Name: main
**
** Description:
**	Called when the Ingres service is started by the NT Service
**	Manager.  Report error in the EventLog if it fails to start.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**	None.
**
** History:
**	15-jul-95 (emmag)
**	    Created.
**	22-jan-2000 (somsa01)
**	    Append II_INSTALLATION to the service name.
**	24-may-2001 (somsa01)
**	    Corrected setting of II_INSTALLATION.
*/

VOID	
main(int argc, char *argv[])
{
	TCHAR	tchFailureMessage[ 256];
	char	*BinaryName, *cptr;

	SERVICE_TABLE_ENTRY	lpServerServiceTable[] = 
	{
	    { tchServiceName, (LPSERVICE_MAIN_FUNCTION) OpenIngresService},
	    { NULL, NULL }
	};

# ifdef IIDEV_DEBUG
	/*
	** Uncomment the DebugBreak line below to enter the debugger
	** when the service is started.
	*/
	/*DebugBreak();*/
# endif

	GVshobj(&ObjectPrefix);

	InitializeCriticalSection(&critStopServer);

	SetConsoleCtrlHandler( (PHANDLER_ROUTINE)&CtrlHandlerRoutine, TRUE );

	/*
	** Let's get II_SYSTEM from the service executable path which
	** is passed in as an argument via the SCM. This is so that we
	** can run NMgtAt() on II_INSTALLATION.
	*/
	BinaryName = (char *)MEreqmem(0, STlength(argv[0])+1, TRUE, NULL);
	STcopy(argv[0], BinaryName);
	cptr = STstrindex(BinaryName, "\\ingres\\bin\\servproc.exe", 0, TRUE);
	*cptr = '\0';
	STcopy(BinaryName, tchII_SYSTEM);
	SetEnvironmentVariable(SYSTEM_LOCATION_VARIABLE, tchII_SYSTEM);

	NMgtAt("II_INSTALLATION", &cptr);

	if (cptr[0] != '\0')
	    STcopy(cptr, tchII_INSTALLATION);
	else
	    GetRegValue("installationcode", tchII_INSTALLATION, FALSE);

	GetRegValue("PostInstallationNeeded", PostSetupNeeded, FALSE);

        sprintf( tchServiceName, "%s_Database_%s", SYSTEM_SERVICE_NAME,
		 tchII_INSTALLATION );
	/*
	** If StartServiceCtrlDispatcher succeeds, it connects the calling 
	** thread to the service control manager and does not return until all 
	** running services in the process have terminated. 
	*/
	if (!StartServiceCtrlDispatcher( lpServerServiceTable ))
	{
	    LPVOID lpMsgBuf;
	    FormatMessage(
  		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
  		NULL,	                    /* no module handle */
  		GetLastError(),
		MAKELANGID(0, SUBLANG_ENGLISH_US),
  		&lpMsgBuf,
		0,			    /* minimum size of memory to allocate*/
  		NULL );

	    sprintf( tchFailureMessage, "Failed to start %s Server. %s", 
			SYSTEM_SERVICE_NAME, (LPCTSTR) lpMsgBuf);

	    LocalFree( lpMsgBuf );          /* Free the buffer. */
	    StopService(tchFailureMessage);
	}

	DeleteCriticalSection(&critStopServer);
	return;
}


/*
** Name: OpenIngresService	
**
** Description:
**	Register the control handling function and start the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**	15-jul-95 (emmag)
**	    Created.
**	22-jan-2000 (somsa01)
**	    If "NoIngstart" is passed in, do not run ingstart via
**	    StartIngresServer().
**	17-apr-2001 (somsa01)
**	    If PostSetupNeeded is set, do not run ingstart via
**	    StartIngresServer().
*/
VOID WINAPI
OpenIngresService(DWORD dwArgc, LPTSTR *lpszArgv)
{
	DWORD		dwWFS;
	DWORD		dwTimeoutms = INFINITE;  /*timeout in milliseconds*/
	DWORD 		dwStatus;
	DWORD 		StopType;

	/*
	** If "HAScluster" is passed to the Ingres service, it means that the 
	** Ingres service was defined to the Windows cluster service, so we 
	** use the GetNodeClusterState function to determine whether the 
	** Cluster service is installed and running on the local node.
	*/
    	if (dwArgc > 1)
    	{
            if (stricmp( lpszArgv[1], "HAScluster" ) == 0)
            {
		GetNodeClusterState( NULL, &dwClusterState );
            }
	}

	if (dwClusterState == ClusterStateRunning)
	    /* The Cluster service is installed, configured, and running on the node. */
	    dwTimeoutms = 30000;            /*30 second timeout*/

	/* 
	** Register contorl handling function with the control dispatcher.
	*/
	sshStatusHandle = RegisterServiceCtrlHandler(
					tchServiceName,
					(LPHANDLER_FUNCTION) IngresCtrlHandler);

	if ( !sshStatusHandle )
		goto cleanup;

	ssServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	ssServiceStatus.dwServiceSpecificExitCode = 0;

	/* 
	** Update status of SCM to SERVICE_START_PENDING	
	*/
	if (!ReportStatusToSCMgr(
		SERVICE_START_PENDING,
		NO_ERROR,
		2,
		300000))
		goto cleanup;

	/* 
	** Create an event that will handle termination.	
	*/
	hServDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

	if ( hServDoneEvent == NULL )
	    goto cleanup;
	
	if ((dwArgc == 2 &&
	    !STbcompare(lpszArgv[1], 10, "NoIngstart", 10, FALSE)) ||
	    strcmp(_strlwr(PostSetupNeeded), "yes") == 0)
	{
	    dbstatus = StartIngresServer(FALSE);
	}
	else
	    dbstatus = StartIngresServer(TRUE);

	if (!dbstatus)
		goto cleanup;
    
	/* 
	** Update status of SCM to SERVICE_RUNNING		
	*/
	if (!ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0, 0))
	    goto cleanup;

	/* 
	** Wait for terminating event or timeout period.
	*/
	while ((dwWFS = WaitForSingleObject( hServDoneEvent, dwTimeoutms )) == WAIT_TIMEOUT)
	{
	    /* Do not check for servers unless current state is "running". */
	    if (ssServiceStatus.dwCurrentState != SERVICE_RUNNING)
	    	continue;

	    /* If critical section is not available, continue waiting. */
	    if (! TryEnterCriticalSection(&critStopServer))
	    	continue;

	    /* Check to see that all the started servers are still running. */
            StopType = (SERVER_CHECK);
	    ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 1, 10000);
            dwStatus = StopServer( (LPVOID)StopType );
	    if (dwStatus == OK)
	    {
		LeaveCriticalSection(&critStopServer);
	    	continue;            /* continue waiting for another period */
	    }

	    /*
	    ** At this point, some server has ended or is not responding;
	    ** the Ingres installation should be shut down.
	    */
	    ReportIngresEvents ("Ingres installation failed. Please refer to the error log for further information.", FAIL);
	    
	    if (dwGlobalErr == 0)
	    	dwGlobalErr = dwStatus;
            SERVPROCTRACE( "%s\n", "ServProc - OpenIngresService shutdown" );
            StopType = (STOP_SERVER | SERVER_SERVICE | SERVER_FORCE);
	    ReportStatusToSCMgr(SERVICE_STOP_PENDING, dwGlobalErr, 1, 300000);
            dwStatus = StopServer( (LPVOID)StopType );
	    if (dwStatus != OK)
	    { 
		ReportIngresEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
		ReportStatusToSCMgr (SERVICE_RUNNING, dwGlobalErr, 0, 0);
	    }
	    LeaveCriticalSection(&critStopServer);
	} /* while WaitForSingleObject */

cleanup:
	if (hServDoneEvent != NULL)
	{	
	    CloseHandle(hServDoneEvent);
	    hServDoneEvent = NULL;
	}
	if (sshStatusHandle != 0)
		(VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
	return;
}


/*
** Name: IngresCtrlHandler
**
** Description:
**     Handles control requests for OpenIngres Service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**	15-jul-95 (emmag)
**	    Created.
**	08-jan-1998 (somsa01)
**	    Added the option to spawn a command as ingres. (Bug #87751)
**	16-jul-01 (rodjo04) bug 105249
**	    Increased timeout value to 300000 because timeout value was too 
**	    short for the thread to complete. 
**	22-jan-2000 (somsa01)
**	    Added the option to shutdown the service without running ingstop.
**	06-dec-2002 (mcgem01)
**		Handle the system shutdown message.
*/

VOID WINAPI
IngresCtrlHandler(DWORD dwCtrlCode)
{
	DWORD	dwState = SERVICE_RUNNING;
	DWORD	nret = 0;
	DWORD	dwThreadId;
	HANDLE  hThread;
	DWORD	StopType = 0;

	switch(dwCtrlCode) 
	{
	    case RUN_COMMAND_AS_INGRES:
		hThread = CreateThread(&sa,
			     0,
			     (LPTHREAD_START_ROUTINE) RunProcessAsIngres,
			     NULL,
			     0,
			     &dwThreadId);
		CloseHandle(hThread);
		break;

	    case SERVICE_CONTROL_PAUSE:
		break;

	    case SERVICE_CONTROL_CONTINUE:
		break;

	    case SERVICE_CONTROL_SHUTDOWN:
            /*
            ** Enable kill flag during system shutdown
            */
            StopType |= SERVER_FORCE;
	    case SERVICE_CONTROL_STOP:
            SERVPROCTRACE( "ServProc - IngresCtrlHandler %s\n",
                (dwCtrlCode == SERVICE_CONTROL_SHUTDOWN) ? "SERVICE_CONTROL_SHUTDOWN" : 
                    "SERVICE_CONTROL_STOP" );
		StopType |= (STOP_SERVER | SERVER_SERVICE);
		ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 1, 300000);
		hThread = CreateThread( NULL, 0, StopServer, (LPVOID)StopType,
					0, &dwThreadId);
		if (hThread == NULL)
		{ ReportIngresEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
		  ReportStatusToSCMgr (SERVICE_RUNNING, nret, 0, 0);
		}
		else
		    CloseHandle(hThread);
		return;

	    case SERVICE_CONTROL_STOP_NOINGSTOP:
		StopType = STOP_SERVICE;
		ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 1, 300000);
		hThread = CreateThread( NULL, 0, StopServer, (LPVOID)StopType,
					0, &dwThreadId);
		if (hThread == NULL)
		{ ReportIngresEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
		  ReportStatusToSCMgr (SERVICE_RUNNING, nret, 0, 0);
		}
		else
		    CloseHandle(hThread);
		return;

	    case SERVICE_CONTROL_INTERROGATE:
		break;

	    default:
		break;
	}
	ReportStatusToSCMgr (dwState, NO_ERROR, 0, 0);
}


/*
** Name: StartIngresServer
**
** Description:
**	Runs ingstart.exe which starts all the ingres processes 
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
**	15-jul-95 (emmag)
**	    Created.
**	31-mar-97 (somsa01)
**	    Fixed bug 81337; we now allocate space for the PATH variable
**	    dynamically.
**	08-jan-1998 (somsa01)
**	    Added the necessary code to run "setuid" programs. (Bug #87751)
**	22-jan-2000 (somsa01)
**	    Run 'ingstart' based upon StartIngres.
**	27-jan-2000 (somsa01)
**	    When running 'ingstart', pass the '-service' flag.
**	27-jan-2000 (somsa01)
**	    Fixed typo.
**	17-apr-2001 (somsa01)
**	    If PostSetupNeeded is set, kick off the post installation
**	    executable.
**	24-may-2001 (somsa01)
**	    Corrected setting of II_TEMPORARY.
**	26-mar-2002 (somsa01)
**	    Remove II_TEMPORARY initially from our environment, since it
**	    may be left over from a previous installation.
**	13-feb-2003 (somsa01)
**	    In StartIngresServer(), before starting Ingres, set up the
**	    setuid stuff.
*/

int
StartIngresServer(BOOL StartIngres)
{
    LPTSTR	tchBuf, tchBuf2;
    TCHAR	tchServiceImage[ _MAX_PATH];
    TCHAR	tchCommand[ _MAX_PATH];
    CHAR	temp = '\0';
    DWORD	nret;
    DWORD	dwWait, dwExitCode = 0;
    int     	len = 0;
    static CHAR	SetuidPipeName[32];
    char	*cptr;

    SetuidShmHandle = NULL;
    Setuid_Handle = NULL;

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.lpReserved = NULL;
    siStartInfo.lpDesktop = NULL;
    siStartInfo.lpTitle = NULL;
    siStartInfo.lpReserved2 = NULL;
    siStartInfo.cbReserved2 = 0;
    siStartInfo.dwFlags = 0;
    
    nret = GetEnvironmentVariable("PATH", &temp, 1);
    if (nret > 0)
    {
	tchBuf = malloc(nret);
	GetEnvironmentVariable("PATH", tchBuf, nret);
	nret += 1024;  /* This is to make room for the add-ons later. */
    }
    else
    {
	nret = 1024;
	tchBuf = malloc(nret);  /* Just make room for the add-ons. */
	GetSystemDirectory(tchBuf, 1024);  /* Also add the System path. */
    }
    tchBuf2 = malloc(nret);

    /*
    ** We do not set II_TEMPORARY in the environment anymore. Therefore,
    ** remove a prior old one from our environment before proceeding.
    */
    SetEnvironmentVariable("II_TEMPORARY", NULL);

    NMgtAt("II_TEMPORARY", &cptr);

    if (cptr[0] != '\0')
	STcopy(cptr, tchII_TEMPORARY);
    else
    {
	GetRegValue("II_TEMPORARY", tchII_TEMPORARY, TRUE);

	if (tchII_TEMPORARY[0] == '\0')
	{
	    STprintf(tchII_TEMPORARY, "%s\\%s\\temp", tchII_SYSTEM,
		     SYSTEM_LOCATION_SUBDIRECTORY);
	}
    }
	

    SetEnvironmentVariable("II_TEMPORARY", tchII_TEMPORARY);

    /* 
    ** Strip II_SYSTEM of any \'s that may have been tagged onto 
    ** the end of it.					     
    */

    len = (i4)strlen (tchII_SYSTEM);

    if (tchII_SYSTEM [len-1] == '\\')
	tchII_SYSTEM [len-1] = '\0';

    /* 
    ** Append "%II_SYSTEM%\inges\{bin, utility};" to the beginning of
    ** the path and set the PATH environment variable. This is applicable
    ** only to this process.
    */

    strcpy(tchBuf2, tchII_SYSTEM);	
    strcat(tchBuf2, "\\");
    strcat(tchBuf2, SYSTEM_LOCATION_SUBDIRECTORY);
    strcat(tchBuf2, "\\utility;");
    strcat(tchBuf2, tchII_SYSTEM);	
    strcat(tchBuf2, "\\");
    strcat(tchBuf2, SYSTEM_LOCATION_SUBDIRECTORY);
    strcat(tchBuf2, "\\bin;");
    strcat(tchBuf2, tchBuf);

    nret = SetEnvironmentVariable(SYSTEM_LOCATION_VARIABLE, tchII_SYSTEM);
    nret = SetEnvironmentVariable("PATH", tchBuf2);
    free(tchBuf);
    free(tchBuf2);

    /*
    ** Create the shared memory segment needed for running setuid
    ** programs, and the named pipe to communicate with client
    ** programs.
    */
    iimksec(&sa);
    STprintf(SetuidShmName, "%s%sSetuidShm", ObjectPrefix, tchII_INSTALLATION);

    if ((SetuidShmHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
					     &sa,
					     PAGE_READWRITE,
					     0,
					     sizeof(struct SETUID_SHM),
					     SetuidShmName)) == NULL)
    {
	dwGlobalStatus = 0;
	ReportIngresEvents("A shared memory segment which allows the Ingres service to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
    }
    else
    {
	if ((SetuidShmPtr = MapViewOfFile(SetuidShmHandle,
					  FILE_MAP_WRITE | FILE_MAP_READ,
					  0,
					  0,
					  sizeof(struct SETUID_SHM))) == NULL)
	{
	    CloseHandle(SetuidShmHandle);
	    dwGlobalStatus = 0;
	    ReportIngresEvents("A shared memory segment which allows the Ingres service to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
	}
	else
	{
	    SetuidShmPtr->pending = FALSE;

	    STprintf(SetuidPipeName, "\\\\.\\PIPE\\INGRES\\%s\\SETUID",
		     tchII_INSTALLATION);
	    if ((Setuid_Handle = CreateNamedPipe(SetuidPipeName,
						PIPE_ACCESS_DUPLEX,
						0,
						PIPE_UNLIMITED_INSTANCES,
						sizeof(struct SETUID),
						sizeof(struct SETUID),
						0,
						&sa)) == INVALID_HANDLE_VALUE)
	    {
		UnmapViewOfFile(SetuidShmPtr);
		CloseHandle(SetuidShmHandle);
		dwGlobalStatus = 0;
		ReportIngresEvents("A pipe which allows the Ingres service to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
	    }
	}
    }

    if (StartIngres)
    {
	STprintf(tchServiceImage, "%s\\%s\\utility\\%sstart.exe",
		 tchII_SYSTEM, SYSTEM_LOCATION_SUBDIRECTORY,
		 SYSTEM_EXEC_NAME);

	sprintf(tchCommand, "%sstart.exe -insvcmgr", SYSTEM_EXEC_NAME);

	isstarted = CreateProcess(
		tchServiceImage, 
		tchCommand,	
		NULL,		
		NULL,		
		TRUE,		
		HIGH_PRIORITY_CLASS,		
		NULL,	
		NULL,		
		&siStartInfo,
		&piProcInfo);

	if (!isstarted)
		goto cleanup;

	dwWait = WaitForSingleObject( piProcInfo.hProcess, INFINITE);

        GetExitCodeProcess ( piProcInfo.hProcess, &dwExitCode);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);


	if (dwExitCode)
	    goto cleanup;
    }
    else if (strcmp(_strlwr(PostSetupNeeded), "yes") == 0)
    {
	/*
	** Kick off the post installation executable.
	*/
	STprintf(tchServiceImage, "%s\\%s\\bin\\%spostinst.exe",
		 tchII_SYSTEM, SYSTEM_LOCATION_SUBDIRECTORY,
		 SYSTEM_CFG_PREFIX);
	STprintf(tchCommand, "%spostinst.exe", SYSTEM_CFG_PREFIX);

	isstarted = CreateProcess(
		tchServiceImage, 
		tchCommand,	
		NULL,		
		NULL,		
		TRUE,		
		HIGH_PRIORITY_CLASS,		
		NULL,	
		NULL,		
		&siStartInfo,
		&piProcInfo);

	if (!isstarted)
		goto cleanup;

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
    }
    else
	isstarted = 1;
	
    return isstarted;

cleanup:
    if (dwExitCode)
    {
	ReportIngresEvents ("Failed to start Ingres installation. Please refer to the error log for further information.", FAIL);
    }
	
    if (hServDoneEvent != NULL)
    {	
	CloseHandle(hServDoneEvent);
	hServDoneEvent = NULL;
    }

    if (sshStatusHandle != 0)
	(VOID)ReportStatusToSCMgr(
			SERVICE_STOPPED,
			dwGlobalErr,
			0,
			0);
    return 0;
}



/*
** Name: StopServer
**
** Description:
**      Issue the ingstop command to stop the OpenIngres server.
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
**      15-jul-95 (emmag)
**          Created.
**	17-dec-95 (emmag)
**	    Stopping the service may take quite a while.  Notify the
**	    service manager that we're still shutting down so that 
**	    he doesn't think we've hung.
**	08-jan-1998 (somsa01)
**	    Get rid of the setuid setup stuff as well. (Bug #87751)
**  22-dec-2000 (rodjo04)
**      If ingstop should return an error, we must tell the service 
**      manager that the service is still running and then report
**      an error to the event log.
*/

DWORD WINAPI
StopServer(LPVOID param)
{
    char	cmdline [256];
    i4		checkpoint = 2;
    DWORD	dwExitStatus = 0;
    DWORD	StopType = (DWORD)param;
    DWORD 	dwCurrentState = SERVICE_STOP_PENDING;
    i4      opt;
    i4      i;

    EnterCriticalSection(&critStopServer);
    SERVPROCTRACE( "ServProc - StopServer entry %#40x\n", StopType );
    /*
    ** Issue an ingstop if STOP_SERVICE flag not set
    */
    if (StopType && ((StopType & STOP_SERVICE) == 0))
    {
	STprintf(cmdline, "\"%s\\ingres\\utility\\ingstop.exe\" ",
		tchII_SYSTEM);
	/*
	** Scan the StopType for enabled bit flags.
	*/
	for(i=0, opt=0; (i < BITS_IN(i4)) && (stop_opt[i] != NULL);
	i+=1, opt=(( StopType >> i)& 0x0001))
	{
	    /*
	    ** If the bit is set and there is an equivalent
	    ** option string append it to the command line.
	    */
	    if ((opt & 0x0001) && (*stop_opt[i] != '\0'))
	    {
        	STcat( cmdline, stop_opt[i]);
	    }
	}
	if (StopType & SERVER_CHECK)
	    dwCurrentState = SERVICE_RUNNING;  /* not stopping--just checking */ 

	SERVPROCTRACE( "ServProc - StopServer cmd %s\n", cmdline );

	isstarted = CreateProcess(NULL,
				cmdline,
				NULL,
				NULL,
				TRUE,
				HIGH_PRIORITY_CLASS,
				NULL,
				NULL,
				&siStartInfo,
				&piProcInfo2);
        SERVPROCTRACE( "ServProc - StopServer CreateProcess %d\n", isstarted );
	/* Report status to service control manager every 2 seconds (2000 ms) */
        while(WaitForSingleObject( piProcInfo2.hProcess, 2000) == WAIT_TIMEOUT)
        {
            ReportStatusToSCMgr( dwCurrentState,
                NO_ERROR,
                checkpoint,
                2000);
                checkpoint++;
        }

	GetExitCodeProcess (piProcInfo2.hProcess, &dwExitStatus);
	SERVPROCTRACE( "ServProc - StopServer GetExitCodeProcess %d\n", dwExitStatus );
	CloseHandle(piProcInfo2.hProcess);
	CloseHandle(piProcInfo2.hThread);

	if (StopType & SERVER_CHECK)
	{
	    LeaveCriticalSection(&critStopServer);
	    return dwExitStatus;
	}    			      
	if (dwExitStatus)
	{
	    ReportIngresEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
	    ReportStatusToSCMgr (SERVICE_RUNNING, dwExitStatus, 0, 0);
	    LeaveCriticalSection(&critStopServer);
	    return dwExitStatus;
	}
    }    

    /*
    ** Free the setuid shared memory, and close out the setuid pipe
    ** handle.
    */
    UnmapViewOfFile(SetuidShmPtr);
    if (SetuidShmHandle)
    	CloseHandle(SetuidShmHandle);
    if (Setuid_Handle)
    	CloseHandle(Setuid_Handle);

    SetEvent(hServDoneEvent);

    LeaveCriticalSection(&critStopServer);
    return(dwExitStatus);
}



/*
** Name: ReportIngresEvents
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
**	15-jul-95 (emmag)
**	    Created.
**
*/

VOID
ReportIngresEvents(LPTSTR lpszMsg, STATUS status)
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
                        E_ING_FAILED,
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
                        E_ING_SUCCEDED,
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
**      15-jul-95 (emmag)
**          Created.
*/

VOID
StopService(LPTSTR lpszMsg)
{
	HANDLE	hEventSource;
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
**      15-jul-95 (emmag)
**          Created.
*/

BOOL
ReportStatusToSCMgr(DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwCheckPoint,
		DWORD dwWaitHint)

{
	BOOL fResult;

	EnterCriticalSection(&critStopServer);
	if (dwCurrentState == SERVICE_START_PENDING) 
	    lpssServiceStatus->dwControlsAccepted = 0;
	else
	    lpssServiceStatus->dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;

	ssServiceStatus.dwCurrentState  = dwCurrentState;
	ssServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ssServiceStatus.dwCheckPoint    = dwCheckPoint;
	ssServiceStatus.dwWaitHint      = dwWaitHint;

	if (!(fResult = SetServiceStatus( sshStatusHandle, lpssServiceStatus)))
	{
	    StopService("SetServiceStatus");
	}
	
	LeaveCriticalSection(&critStopServer);
	return fResult;
}


/*
** Name: RunProcessAsIngres
**
** Description:
**	Implements NT's version of "setuid" on UNIX.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	08-jan-1998 (somsa01)
**	    Created.
**	19-feb-1998 (somsa01)
**	    We also need to pass to the spawned process the current
**	    working directory of the client.  (Bug #89006)
**	25-feb-1998 (somsa01)
**	    We now have an input file for the spawned process' stdin.
**	14-jun-1999 (somsa01)
**	    Check for valid startup commands as well.
**	22-dec-2000 (somsa01)
**	    Convert the result of GetUserName() to all lower case, as
**	    is done by IDname().
**	07-jul-2001 (somsa01)
**	    We now properly handle embedded spaced II_SYSTEM for startup
**	    commands.
**	14-feb-2002 (somsa01)
**	    Added a Sleep() call to give the CPU a break while we're
**	    waiting.
**	20-mar-2002 (somsa01)
**	    Set the exit code of the child process in setuid.ExitCode.
**	29-mar-2002 (somsa01)
**	    Only set the exit code of the child process in setuid.ExitCode
**	    if it actually ran the child process; otherwise, it is -1.
**	25-Jul-2007 (drivi01)
**	    Instead of using SE_DEBUG privilege to monitor the process
**          and get its exit code, service will now wait for process to die
**	    and retrieve exit code.
*/

VOID
RunProcessAsIngres()
{
    char	cmdline[2048];
    char	RealUserId[25];
    DWORD	szRealUserId = sizeof(RealUserId);
    char	pidstr[32];
    char	InfileName[256], OutfileName[256];
    HANDLE	InFile, OutFile;
    int		i;
    int		cmdlen;
    BOOL	SetuidCmd = FALSE, StartupCmd = FALSE;

    /*
    ** Connect to the communication pipe, and retrieve the
    ** information from the client, including his/her userid.
    */
    if (Setuid_Handle == INVALID_HANDLE_VALUE) return;
    ConnectNamedPipe (Setuid_Handle, NULL);
    if (!ReadFile (Setuid_Handle, &setuid, sizeof(struct SETUID),
			&BytesRead, NULL))
    {
	DisconnectNamedPipe(Setuid_Handle);
	return;
    }
    ImpersonateNamedPipeClient(Setuid_Handle);
    GetUserName(RealUserId, &szRealUserId);
    _strlwr(RealUserId);
    RevertToSelf();

    setuid.ExitCode = -1;

    /*
    ** Only run this command if it is a valid setuid or startup command.
    */
    cmdlen = (i4)STlength( setuid.cmdline );
    for ( i = 0; validSetuidCmds[i] ; i++ )
    {
	if ( STbcompare( setuid.cmdline, cmdlen,
			 validSetuidCmds[i], (i4)STlength(validSetuidCmds[i]),
			 FALSE ) == 0 )
	{
	    SetuidCmd = TRUE;
	    break;
	}
    }
    if (!SetuidCmd)
    {
	/*
	** See if this is a valid startup command.
	*/
	for ( i = 0; validStartupCmds[i] ; i++ )
	{
	    if ( STbcompare( setuid.cmdline, cmdlen,
			     validStartupCmds[i],
			     (i4)STlength(validStartupCmds[i]),
			     FALSE ) == 0 )
	    {
		StartupCmd = TRUE;
		break;
	    }
	}
    }
    if (SetuidCmd)
    {
	char *cp;

	if ( (cp = STindex( setuid.cmdline, " ", 0 )) != NULL )
	{
	    *cp = EOS;
	}
	STprintf( cmdline, "\"%s\\ingres\\bin\\%s\"", 
		  tchII_SYSTEM, setuid.cmdline );
	if ( cp )
	{
	   CMnext( cp );
	   STcat( cmdline, " " );
	   STcat( cmdline, cp );
	}
    }
    else if (StartupCmd)
    {
	char	*cp, tmpbuf[2048], tmpbuf2[2048];

	if ((cp = STindex(setuid.cmdline, " ", 0)) != NULL)
	{
	    STcopy (cp, tmpbuf2);
	    *cp = EOS;
	}

	STprintf( tmpbuf, "\"%s\\ingres\\utility\\%s", tchII_SYSTEM,
		  setuid.cmdline );

	STprintf( cmdline, "%s\"%s", tmpbuf, tmpbuf2 );
    }
    else
    {
	setuid.CreatedProcID = -3;
	goto cleanup_setuid;
    }

    /*
    ** Attach to the output file where the spawned process will send
    ** its output.
    */
    STprintf(OutfileName, "%s\\%sstdout.tmp", tchII_TEMPORARY,
	    setuid.ClientProcID);
    if ( (OutFile = CreateFile (OutfileName,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				&sa,
				OPEN_EXISTING,
				FILE_FLAG_WRITE_THROUGH,
				NULL)) == INVALID_HANDLE_VALUE )
    {
	DisconnectNamedPipe(Setuid_Handle);
	return;
    }

    /*
    ** Attach to the input file where the spawned process (if
    ** applicable) will get its input.
    */
    STprintf(InfileName, "%s\\%sstdin.tmp", tchII_TEMPORARY,
	    setuid.ClientProcID);
    if ( (InFile = CreateFile ( InfileName,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				&sa,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL)) == INVALID_HANDLE_VALUE )
    {
	CloseHandle(OutFile);
	DisconnectNamedPipe(Setuid_Handle);
	return;
    }

    ZeroMemory (&siStartInfo2, sizeof(STARTUPINFO));
    siStartInfo2.cb = sizeof(STARTUPINFO);
    siStartInfo2.dwFlags = STARTF_USESTDHANDLES;
    siStartInfo2.hStdInput = InFile;
    siStartInfo2.hStdOutput = OutFile;
    siStartInfo2.hStdError = OutFile;

    /*
    ** Create the process as suspended, so that we can obtain the process
    ** id first. This is to be used in the shared memory segment name.
    */
    if (CreateProcess ( NULL,
			cmdline,
			&sa,
			&sa,
			TRUE,
			NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED,
			NULL,
			setuid.WorkingDirectory,
			&siStartInfo2,
			&piProcInfo2) )
	setuid.CreatedProcID = piProcInfo2.dwProcessId;
    else
	setuid.CreatedProcID = -1;

    if (setuid.CreatedProcID != -1)
    {
	/*
	** Create the shared memory segment in the system pagefile (for
	** security) for the child process to pick up the real userid.
	*/
	CVla(piProcInfo2.dwProcessId, pidstr);
        STprintf(RealuidShmName, "%s%s%sRealuidShm", ObjectPrefix, pidstr,
		     tchII_INSTALLATION);

	if ( (RealuidShmHandle = CreateFileMapping (INVALID_HANDLE_VALUE,
						    &sa,
						    PAGE_READWRITE,
						    0,
						    sizeof(struct REALUID_SHM),
						    RealuidShmName)) == NULL )
	{
	    dwGlobalStatus = 0;
	    ReportIngresEvents("A shared memory segment vital for executing a task was unable to be created. This task will not be executed.", FAIL);
	    setuid.CreatedProcID = -2;
	    goto cleanup_setuid;
	}
	if ( (RealuidShmPtr = MapViewOfFile (RealuidShmHandle,
					     FILE_MAP_WRITE | FILE_MAP_READ,
					     0,
					     0,
					     sizeof(struct REALUID_SHM))) ==
	     NULL )
	{
	    CloseHandle(RealuidShmHandle);
	    dwGlobalStatus = 0;
	    ReportIngresEvents("A shared memory segment vital for executing a task was unable to be created. This task will not be executed.", FAIL);
	    setuid.CreatedProcID = -2;
	    goto cleanup_setuid;
	}
	RealuidShmPtr->InfoTaken = FALSE;
	STcopy(RealUserId, RealuidShmPtr->RealUserId);

	/*
	** Wait until the child process has made its first call to
	** IDname() to retrieve the real userid from the shared
	** memory segment. This first call will also cache the real
	** userid so that we can then destroy the shared memory
	** segment.
	*/
	ResumeThread(piProcInfo2.hThread);
	/* Wait for a process to die to get its exit code */
	while ( PCis_alive(setuid.CreatedProcID) )
	{
	    Sleep(100);
	}

	GetExitCodeProcess(piProcInfo2.hProcess, &setuid.ExitCode);

	/* Destroy the temporary shared memory segment. */
	UnmapViewOfFile(RealuidShmPtr);
	CloseHandle(RealuidShmHandle);
    }

cleanup_setuid:
    /*
    ** Send information back to the client, and then
    ** disconnect from the client.
    */
    CloseHandle(InFile);
    CloseHandle(OutFile);
    CloseHandle(piProcInfo2.hProcess);
    CloseHandle(piProcInfo2.hThread);
    WriteFile (Setuid_Handle, &setuid, sizeof(struct SETUID),
		&BytesWritten, NULL);
    FlushFileBuffers(Setuid_Handle);
    DisconnectNamedPipe(Setuid_Handle);
    if (setuid.CreatedProcID == -2)
	TerminateProcess(piProcInfo2.hProcess, 1);
}

static void
GetRegValue(char *strKey, char *strValue, BOOL CloseKey)
{
    HKEY		hKey;
    static HKEY		SoftwareKey = NULL;
    int			ret_code = 0, i;
    TCHAR		KeyName[MAX_PATH + 1];
    TCHAR		tchValue[MAX_PATH];
    unsigned long	dwSize;
    DWORD		dwType;

    if (!SoftwareKey)
    {
	/*
	** We have to figure out the right registry software key that
	** matches the install path.
	*/
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     "Software\\IngresCorporation\\Ingres",
		     0, KEY_ALL_ACCESS, &hKey))
	{
		for (i = 0; ret_code != ERROR_NO_MORE_ITEMS; i++)
		{
			ret_code = RegEnumKey(hKey, i, KeyName, sizeof(KeyName));
			if (!ret_code)
			{
				RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &SoftwareKey);
				dwSize = sizeof(tchValue);
				RegQueryValueEx(SoftwareKey, SYSTEM_LOCATION_VARIABLE,
						NULL, &dwType, (unsigned char *)&tchValue,
						&dwSize);

				if (!strcmp(tchValue, tchII_SYSTEM))
					break;
				else
					RegCloseKey(SoftwareKey);
			}
		}
	}
	RegCloseKey(hKey);
    }

    /*
    ** Now that we know where to look, let's get the value that we need
    ** from the registry.
    */
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(SoftwareKey, strKey, NULL, &dwType,
			(unsigned char *)&tchValue, &dwSize) == ERROR_SUCCESS)
    {
	strcpy(strValue, tchValue);
    }
    else
	strcpy(strValue, "");

    if (CloseKey)
    {
	RegCloseKey(SoftwareKey);
	SoftwareKey = NULL;
    }
}


# ifdef xDEBUG
static char logfilename[MAX_PATH + 1] ZERO_FILL;    /* path to log file */
static bool loginit = FALSE;                        /* initialization flag */
/*
** Name: ServProcTrace
**
** Description:
**      Function to log messages to a file.
**      Used during shutdown processing when the eventlog service is shutdown
**      before us.
**
** Inputs:
**      fmt         Formating character string.  See printf.
**      ...         variable argument list
**
** Outputs:
**      None
**
** Return:
**      None
**
** History:
**      27-Jan-2003 (fanra01)
**          Created.
*/
void
ServProcTrace( char* fmt, ... )
{
    char    lbuf[256];
    char    *log = NULL;
    FILE* fd = NULL;
    va_list p;

    if (loginit == FALSE)
    {
        NMgtAt( SERVPROC_LOG, &log );
        if (log && *log)
        {
            strcpy( logfilename, log );
            loginit = TRUE;
        }
    }
    /*
    ** If initialized and able to open file then write a log message.
    ** Flush the message and close the file just in case OS decides to
    ** terminate this process.
    */
    if ((loginit == TRUE) && ((fd=fopen( logfilename, "a+")) != NULL))
    {
        va_start( p, fmt );
        sprintf(lbuf, "%08d %s", GetCurrentThreadId(), fmt);
        vfprintf( fd, lbuf, p );
        fflush( fd );
        fclose( fd );
    } 
    return;
}
# endif
