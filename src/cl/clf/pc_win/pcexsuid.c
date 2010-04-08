/*
**  Copyright (c) 1998, 2004 Ingres Corporation
**
**  Name: PCEXSUID.C  - implementation of the "setuid" bit from UNIX
**
**  Description:
**	This file contains the function which, in conjunction with the
**	Ingres service, runs commands as the ingres userid. This is
**	the NT implementation of the setuid bit from UNIX.
**
**	Internal routines.
**
**	    PCexec_suid - Execute the provided command as the ingres
**			  user.
** 
**  History:
**	08-jan-1998 (somsa01)
**	    Created.
**	29-jan-98 (mcgem01)
**	    Change CA-OpenIngres to OpenIngres.
**	06-feb-1998 (canor01)
**	    Clean up compiler warnings.
**	19-feb-1998 (somsa01)
**	    We need to pass to the service the current working directory
**	    as well.  (Bug #89006)
**	25-feb-1998 (somsa01)
**	    We now have an input file for the process' stdin which runs
**	    through the OpenIngres service.
**	14-mar-1998 (somsa01)
**	    In the case of the "-silent" flag (rcpstat), do not print out
**	    the error in error_exit(). Also, if the user has access to
**	    the server shared memory segment, then do not run the child
**	    process through the OpenIngres service.
**	10-apr-98 (mcgem01)
**	    Product name change from OpenIngres to Ingres.
**	19-jun-1998 (somsa01)
**	    Use SYSTEM_PRODUCT_NAME for the name of the service.
**	2-jul-1998 (kitch01)
**		Put in the missing s from the above change.
**	10-jul-1998 (kitch01)
**		Bug 91362. If user is 'system' run through OpenIngres service
**		despite having access to server shared memory 'system' does not
**		have required privilege to access semaphores/mutexes.
**	11-jun-1999 (somsa01)
**	    If the command is a startup command, then it is always run through
**	    the Ingres service.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
** 
**	03-nov-1999 (somsa01)
**	    A failure from ControlService() should be treated as a severe
**	    error which should not let us continue.
** 	22-jan-2000 (somsa01)
**	    In PCexec_suid, return the exit code of the spawned process.
**	    Also, if the files exist, truncate them. The service name is
**	    now keyed off of II_INSTALLATION.
**	05-jun-2000 (somsa01)
**	    The Ingres installation may be started as the SYSTEM account,
**	    in which the 'ingres' user will not automatically have access
**	    to the shared memory segments. Therefore, even if the real
**	    user is 'ingres', check to see if he has access.
**	14-jun-2000 (mcgem01)
**	    The different product service name is <product>_Client_<inst id>
**	24-oct-2000 (somsa01)
**	    In PCexec_suid(), removed the check on shared memory access.
**	    Access to the shared memory segment does not necessarily mean
**	    that the user running the process does not need to run the
**	    specified process as the Ingres owner. Also, generalized the
**	    check of the user with IDname_service().
**	18-dec-2000 (somsa01)
**	    In PCexec_suid(), modified the cases to run the command
**	    "as is" without the Ingres service.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	20-mar-2002 (somsa01)
**	    In PCexec_suid(), if all is well, return the exit code of
**	    the child process that was executed.
**	29-mar-2002 (somsa01)
**	    In PCexec_suid(), properly return the child process exit code.
**	11-apr-2003 (somsa01)
**	    In PCexec_suid(), while waiting for "pending" to not be set,
**	    give some CPU back to the OS. Also, in ex_handler(), reset
**	    "pending" to FALSE to indicate that the setuid service is ready
**	    to go again.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-jun-2004 (rigka01) bug #108772
**	    In PCexec_suid() use II_TEMPORARY in place of GetCurrentDirectory
**	    for setuid.WorkingDirectory
**	11-ocg-2004 (somsa01)
**	    Backed out rigka01's change for bug #108772, it breaks fastload
**	    when using the "-f" flag as a user who did not start the Ingres
**	    service.
**	28-Feb-2005 (drivi01)
**		Modified code to set setuid.WorkingDirectory to II_TEMPORARY if 
**		GetCurrentDirectory returns a directory on mounted drive.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**      12-Oct-2007 (drivi01)
**          tchII_INSTALLATION variable is being used before instance id is
**          retrieved. Move the NMgtAt function to the top of PCexec_suid.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    DBA Tools service was renamed to Ingres_DBATools_<instance id>
**	    and this file was updated to check for this service name
**	    if Ingres service isn't found.
**	10-Nov-2009 (drivi01)
**	    Process should be run by a service if it wasn't launched by the
**	    Admin user.  This primarily applies to Vista and above.
**	    
*/

# include <compat.h>
# include <cs.h>
# include <cv.h>
# include <ex.h>
# include <id.h>
# include <me.h>
# include <nm.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <st.h>
# include <gl.h>

SC_HANDLE		schSCManager, OpIngSvcHandle;
struct SETUID_SHM	*SetuidShmPtr;
HANDLE			SetuidShmHandle;
HANDLE			ProcHandle;
HANDLE			Setuid_Handle;
HANDLE			InFile, OutFile;
CHAR			InfileName[256], OutfileName[256];
BOOL			SilentMode = FALSE;
BOOL			Is_Win2K = FALSE;

/*
** These are setuid Ingres commands which interact with at least
** one database.
*/
char *validSetuidDbCmds[] = {
	"dmfjsp",
	"ntvrfydb",
	"ntupgrdb",
	NULL
};

/*
** These are setuid Ingres commands which, if we are asked to run them,
** must go through the Ingres service.
*/
char *ServiceCommands[] = {
	"iirun",
	"jasrun",
	NULL
};

FUNC_EXTERN VOID	GVshobj(char **ObjectPrefix);

static VOID		error_exit(DWORD errcode);
static STATUS		ex_handler(EX_ARGS *ex_args);


/*
** Name: PCexec_suid - Execute a command as the ingres user.
**
** Description:
**	This procedure works with the Ingres service to run the given
**	command as the ingres user. It mimicks the "setuid" bit in UNIX.
**
** Inputs:
**	cmdbuf - command to execute as the ingres user
**
** Outputs:
**	none
**
** Returns:
**	OK
**	FAIL
**
** Side Effects:
**	none
**
** History:
**	08-jan-1998 (somsa01)
**	    Created.
**	19-feb-1998 (somsa01)
**	    We need to pass to the service the current working directory
**	    as well.  (Bug #89006)
**	25-feb-1998 (somsa01)
**	    We now have an input file for the process' stdin which
**	    runs through the OpenIngres service.
**	19-jun-1998 (somsa01)
**	    Use SYSTEM_PRODUCT_NAME for the name of the service.
**	10-jul-1998 (kitch01)
**		Bug 91362. If user is 'system' run through OpenIngres service
**		despite having access to server shared memory 'system' does not
**		have required privilege to access semaphores/mutexes.
**	11-jun-1999 (somsa01)
**	    If the command is a startup command, then it is always run through
**	    the Ingres service.
**	03-nov-1999 (somsa01)
**	    A failure from ControlService() should be treated as a severe
**	    error which should not let us continue.
** 	22-jan-2000 (somsa01)
**	    Return the exit code of the spawned process. Also, if the
**	    files exist, truncate them. The service name is now keyed off
**	    of II_INSTALLATION.
**	05-jun-2000 (somsa01)
**	    The Ingres installation may be started as the SYSTEM account,
**	    in which the 'ingres' user will not automatically have access
**	    to the shared memory segments. Therefore, even if the real
**	    user is 'ingres', check to see if he has access.
**	24-oct-2000 (somsa01)
**	    Removed the check on shared memory access. Access to the shared
**	    memory segment does not necessarily mean that the user running
**	    the process does not need to run the specified process as the
**	    Ingres owner. Also, generalized the check of the user with
**	    IDname_service().
**	18-dec-2000 (somsa01)
**	    Modified the cases to run the command "as is" without the Ingres
**	    service.
**	20-mar-2002 (somsa01)
**	    If all is well, return the exit code of the child process that
**	    was executed.
**	29-mar-2002 (somsa01)
**	    Properly return the child process exit code.
**	11-apr-2003 (somsa01)
**	    While waiting for "pending" to not be set, give some CPU back
**	    to the OS.
**      29-Jul-2005 (drivi01)
**	    Allow user to run the command if he/she owns a shared
**	    segment and ingres is not running as a service.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	    Added PCadjust_SeDebugPrivilege to allow quering of 
**	    System processes.
**	25-Jul-2007 (drivi01)
**		On Vista, PCexec_suid is unable to use SE_DEBUG Privilege
**	    to query process status and retireve its exit code.
**		The routine for monitoring a process and retrieving 
**	    its exit code has been moved to Ingres Service.
**	05-Nov-2009 (wanfr01) b122847
**	    Don't do a PCsleep unless you are waiting for more input
*/
STATUS
PCexec_suid(char *cmdbuf)
{
    EX_CONTEXT		context;
    SERVICE_STATUS	ssServiceStatus;
    LPSERVICE_STATUS	lpssServiceStatus = &ssServiceStatus;
    struct SETUID	setuid;
    DWORD		ProcID;
    HANDLE		SaveStdout;
    SECURITY_ATTRIBUTES	sa;
    CHAR		szRealUserID[25] = "";
    CHAR		*pszRealUserID = szRealUserID;
    CHAR		szServiceUserID[25] = "";
    CHAR		*pszServiceUserID = szServiceUserID;
    DWORD		BytesWritten, BytesRead = 0;
    CHAR		*inst_id;
    CHAR		SetuidShmName[64];
    CHAR		*temp_loc;
    CHAR		InBuf[256], OutBuf[256];
    static CHAR		SetuidPipeName[32];
    CL_ERR_DESC		err_code;
    CHAR		ServiceName[255];
    DWORD		ExitCode = 0;
    CHAR		tchII_INSTALLATION[3];
    BOOL		SetuidDbCmd = FALSE, ServiceCommand = FALSE;
    int			i, cmdlen;
    char		*ObjectPrefix;
	u_i4		drType;
    SC_HANDLE		schSCManager, OpIngSvcHandle;
    BOOL		bServiceStarted = FALSE;

    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }
    NMgtAt("II_INSTALLATION", &inst_id);
    STcopy(inst_id, tchII_INSTALLATION);

    /*
    ** See if this is a command that MUST be run through the Ingres
    ** service.
    */
    cmdlen = (i4)STlength(cmdbuf);
    for (i = 0; ServiceCommands[i] ; i++)
    {
	if (STbcompare( cmdbuf, cmdlen, ServiceCommands[i],
			(i4)STlength(ServiceCommands[i]), FALSE ) == 0)
	{
	    ServiceCommand = TRUE;
	    break;
	}
    }

    /*
    ** If the user is the same as the user who started the Ingres
    ** service, just spawn the command.
    */
    if (!ServiceCommand)
    {
	IDname(&pszRealUserID);
	if (!IDname_service(&pszServiceUserID) &&
	    STcompare(pszServiceUserID, pszRealUserID) == 0 && 
		PCisAdmin())
	{
	    /*
	    ** Attempt to just execute the command.
	    */
	    return( PCcmdline( (LOCATION *) NULL, cmdbuf, PC_WAIT,
		    (LOCATION *) NULL, &err_code) );
	}
	else
	{
	    /*
	    ** If current user is not the same as service user and ingres is not
	    ** running as a service, check if shared memory segment is owned
	    ** by current user, if user has access to shared segment allow him
	    ** to run the command.
	    */
	    PTR	shmem;
	    i4	allocated_pages;
	    STATUS status;

	    if((status = MEget_pages(ME_MSHARED_MASK, 1, "lglkdata.mem",
	                 &shmem, &allocated_pages, &err_code)) == OK)
	    {
	        STprintf(ServiceName, "%s_Database_%s", SYSTEM_SERVICE_NAME, 
		tchII_INSTALLATION);
	        if ((schSCManager = OpenSCManager(NULL, NULL, 
					SC_MANAGER_CONNECT)) != NULL)
	        {
		    if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
					SERVICE_QUERY_STATUS)) != NULL)
		    {
		       if (QueryServiceStatus(OpIngSvcHandle,lpssServiceStatus))
		       {
		    	if (ssServiceStatus.dwCurrentState != SERVICE_STOPPED)
			     bServiceStarted = TRUE;
		        }
		    }
	        }	
	        if (!bServiceStarted)
		    return(PCcmdline( (LOCATION *) NULL, cmdbuf, PC_WAIT,
			(LOCATION *) NULL, &err_code) );
	    }

	}

	/*
	** See if this command is an Ingres command which needs to interact
	** with at least one database.
	*/
	for (i = 0; validSetuidDbCmds[i] ; i++)
	{
	    if (STbcompare( cmdbuf, cmdlen, validSetuidDbCmds[i],
			    (i4)STlength(validSetuidDbCmds[i]), FALSE ) == 0)
	    {
		SetuidDbCmd = TRUE;
		break;
	    }
	}

	/*
	** If the user has access to the Ingres shared memory segment,
	** just spawn the command provided that it is not in the
	** validSetuidDbCmds list.
	*/
	if (!SetuidDbCmd)
	{
	    PTR	shmem;
	    i4	allocated_pages;
	    STATUS	status;

	    if (((status = MEget_pages(ME_MSHARED_MASK, 1, "lglkdata.mem",
		&shmem,
		&allocated_pages, &err_code)) == OK) ||
		(status == ME_NO_SUCH_SEGMENT))
	    {
		if (status != ME_NO_SUCH_SEGMENT)
		    MEfree_pages(shmem, allocated_pages, &err_code);
		return( PCcmdline( (LOCATION *) NULL, cmdbuf, PC_WAIT,
			(LOCATION *) NULL, &err_code) );
	    }
	}
    }

    /*
    ** We must run the command through the Ingres service.
    */

    if ( STstrindex(cmdbuf, "-silent", 0, FALSE ) )
	SilentMode = TRUE;

    iimksec(&sa);


	GVshobj(&ObjectPrefix);
	STprintf(SetuidShmName, "%s%sSetuidShm", ObjectPrefix, tchII_INSTALLATION);

    if ( (SetuidShmHandle = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
					    FALSE,
					    SetuidShmName)) == NULL )
    {
	error_exit(GetLastError());
	return(FAIL);
    }
    if ( (SetuidShmPtr = MapViewOfFile(SetuidShmHandle,
				       FILE_MAP_WRITE | FILE_MAP_READ,
				       0,
				       0,
				       sizeof(struct SETUID_SHM))) == NULL )
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    /* Set up the information to send to the service. */
    STcopy(cmdbuf, setuid.cmdline);
    GetCurrentDirectory(sizeof(setuid.WorkingDirectory),
			setuid.WorkingDirectory);
	NMgtAt("II_TEMPORARY", &temp_loc);
	drType = GetDriveType(NULL);
	if (drType == DRIVE_REMOTE)
	{
		STcopy(temp_loc, setuid.WorkingDirectory);
	}
    SaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CVla(GetCurrentProcessId(), setuid.ClientProcID);
    STprintf(SetuidPipeName, "\\\\.\\PIPE\\INGRES\\%s\\SETUID", inst_id);

    /* Set up the stdout file for the command. */
    STprintf(OutfileName, "%s\\%sstdout.tmp", temp_loc, setuid.ClientProcID);
    if ( (OutFile = CreateFile(OutfileName,
			       GENERIC_READ | GENERIC_WRITE,
			       FILE_SHARE_READ | FILE_SHARE_WRITE,
			       &sa,
			       CREATE_ALWAYS,
			       FILE_ATTRIBUTE_NORMAL,
			       NULL)) == INVALID_HANDLE_VALUE )
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    /* Set up the stdin file for the command. */
    STprintf(InfileName, "%s\\%sstdin.tmp", temp_loc, setuid.ClientProcID);
    if ( (InFile = CreateFile(InfileName,
			      GENERIC_READ | GENERIC_WRITE,
			      FILE_SHARE_READ | FILE_SHARE_WRITE,
			      &sa,
			      CREATE_ALWAYS,
			      FILE_FLAG_WRITE_THROUGH,
			      NULL)) == INVALID_HANDLE_VALUE )
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    /* Wait until the service is ready to process our request. */
    while (SetuidShmPtr->pending == TRUE)
	PCsleep(100);
    SetuidShmPtr->pending = TRUE;

    /* Trigger the "setuid" event of the service. */
    if ( (schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)) == NULL)
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    STprintf(ServiceName, "%s_Database_%s", SYSTEM_SERVICE_NAME,
	     tchII_INSTALLATION );
	OpIngSvcHandle = OpenService(schSCManager, ServiceName,
						SERVICE_USER_DEFINED_CONTROL);
	if (OpIngSvcHandle == NULL)
	{
		STprintf(ServiceName, "%s_DBATools_%s", SYSTEM_SERVICE_NAME,
	     tchII_INSTALLATION );
		OpIngSvcHandle = OpenService(schSCManager, ServiceName,
						SERVICE_USER_DEFINED_CONTROL);
	}
    if ( OpIngSvcHandle == NULL)
    {
	error_exit(GetLastError());
	return(FAIL);
    }
    if (!ControlService(OpIngSvcHandle, RUN_COMMAND_AS_INGRES,
			lpssServiceStatus))
    {
	error_exit(GetLastError());
	CloseServiceHandle(schSCManager);
	return(FAIL);
    }

    WaitNamedPipe(SetuidPipeName, NMPWAIT_WAIT_FOREVER);

    /* Send the information to the service. */
    if ( (Setuid_Handle = CreateFile(SetuidPipeName,
				     GENERIC_READ|GENERIC_WRITE,
				     FILE_SHARE_READ|FILE_SHARE_WRITE,
				     &sa,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL)) == INVALID_HANDLE_VALUE )
    {
	error_exit(GetLastError());
	return(FAIL);
    }
    if (!WriteFile(Setuid_Handle, &setuid, sizeof(struct SETUID),
		   &BytesWritten, NULL))
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    /*
    ** Retrieve information back from the service, and then
    ** disconnect from the pipe.
    */
    if (!ReadFile(Setuid_Handle, &setuid, sizeof(struct SETUID),
		  &BytesRead, NULL))
    {
	error_exit(GetLastError());
	return(FAIL);
    }

    ProcID = setuid.CreatedProcID;
    SetuidShmPtr->pending = FALSE;

    UnmapViewOfFile(SetuidShmPtr);
    SetuidShmPtr = NULL;
    CloseHandle(SetuidShmHandle);

    if ( (ProcID != -1) && (ProcID != -2) )
    {
	/*
	** Wait for the "spawned" process to exit, reading its output
	** from the stdout file.
	*/
	for (;;)
	{
	    if ( ((!ReadFile(OutFile, OutBuf, sizeof(OutBuf), &BytesRead, NULL)
		  || (BytesRead == 0)) && setuid.ExitCode != STILL_ACTIVE ))
		break;

	    if ( BytesRead &&
		 (!WriteFile(SaveStdout, OutBuf, BytesRead, &BytesWritten,
		  NULL)) && setuid.ExitCode != STILL_ACTIVE)
		break;
	    else if (BytesRead < sizeof(OutBuf))
		PCsleep(200);

	    /*
	    ** Currently, the only DBA program which can require
	    ** user input is verifydb. Therefore, when it spits out
	    ** the appropriate messages asking for user input, get
	    ** it from the end user and pass it along to the spawned
	    ** process.
	    */
	    if ( (STrstrindex(OutBuf, "S_DU04FF_CONTINUE_PROMPT", 0, FALSE)
			!= NULL) ||
		 (STrstrindex(OutBuf, "S_DU0300_PROMPT", 0, FALSE) != NULL) )
	    {
		SIflush(stdout);
		MEfill(sizeof(OutBuf), ' ', &OutBuf);
		MEfill(sizeof(InBuf), ' ', &InBuf);
		SIgetrec(InBuf, 255, 0);
		WriteFile(InFile, InBuf, sizeof(OutBuf), &BytesWritten, NULL);
	    }
	}

	ExitCode = setuid.ExitCode;
	CloseHandle(Setuid_Handle);
	CloseHandle(InFile);
	DeleteFile(InfileName);
	CloseHandle(OutFile);
	DeleteFile(OutfileName);
	CloseServiceHandle(OpIngSvcHandle);
	CloseServiceHandle(schSCManager);
        return(ExitCode);
    }
    else
    {
	error_exit(GetLastError());
	return(FAIL);
    }
}

static STATUS
ex_handler(EX_ARGS *ex_args)
{
    if (SetuidShmPtr)
    {
	SetuidShmPtr->pending = FALSE;
	UnmapViewOfFile(SetuidShmPtr);
    }
    if (InFile)
    {
	CloseHandle(InFile);
	DeleteFile(InfileName);
    }
    if (OutFile)
    {
	CloseHandle(OutFile);
	DeleteFile(OutfileName);
    }
    if (Setuid_Handle)   CloseHandle(Setuid_Handle);
    if (SetuidShmHandle) CloseHandle(SetuidShmHandle);
    if (OpIngSvcHandle)  CloseServiceHandle(OpIngSvcHandle);
    if (schSCManager)    CloseServiceHandle(schSCManager);

    return (EXDECLARE);
}

static VOID
error_exit(DWORD errcode)
{
	ex_handler(NULL);
    if (!SilentMode)
	SIprintf("\nERROR : The execution of this command failed. Please verify with your\n	Ingres System Administrator that the Ingres service\n	was started successfully. The operating system error was %d.\n", errcode);
}
