/*
**  Copyright (c) 1995, 2001 Ingres Corporation
**
**  Name: servproc.h
**
**  History:
**      15-jul-95 (emmag)
**          Created.
**	17-dec-95 (emmag)
**	    Service control handler is of type VOID.
**	08-jan-1998 (somsa01)
**	    Added the necessary stuff to implement NT's version of setuid.
**	    (Bug #87751)
**	18-feb-98 (mcgem01)
**	    Add an extra status parameter to ReportIngresEvents so that
**	    we're not limited to reporting failure events.
**	19-oct-99 (fanra01, cucjo01)
**	    Changed return type of StopService() function since it now
**	    stops as a thread to avoid timeouts.
**	22-jan-2000 (somsa01)
**	    Added another parameter to StartIngresServer() for conditional
**	    running of 'ingstart'.
**	17-apr-2001 (somsa01)
**	    Added PostSetupNeeded variable.
**	28-jun-2001 (somsa01)
**	    Changed size of SetuidShmName and RealuidShmName to 64.
**    29-mar-2005 (rigka01) bug 114109, INGSRV3211
**        privileged instruction exception during Ingres shutdown or
**        startup via Services
**    12-Dec-2006 (drivi01)
**	  Export function GVshobj which retrieves the prefix to
**	  shared object names.
*/

BOOL    ReportStatusToSCMgr (DWORD dwCurrentState,
                             DWORD dwWin32ExitCode,
                             DWORD dwCheckPoint,
                             DWORD dwWaitHint);
VOID    WINAPI OpenIngresService (DWORD dwArgc, LPTSTR *lpszArgv);
int     StartIngresServer (BOOL StartIngres);
VOID    WINAPI IngresCtrlHandler (DWORD);
VOID    ReportIngresEvents (LPTSTR lpszMsg, STATUS status);
VOID    StopService (LPTSTR lpszMsg);
DWORD WINAPI    StopServer  ( LPVOID param );
VOID	RunProcessAsIngres ();

FUNC_EXTERN BOOL	GVosvers(char *OSVersionString);
FUNC_EXTERN VOID	GVshobj(char **shPrefix);

SERVICE_STATUS_HANDLE	sshStatusHandle;
HANDLE                  hServDoneEvent = NULL;
SERVICE_STATUS          ssServiceStatus;
LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
BOOL                    isstarted;
TCHAR			PostSetupNeeded[6];
PROCESS_INFORMATION     piProcInfo;
PROCESS_INFORMATION     piProcInfo2;
STARTUPINFO             siStartInfo;
STARTUPINFO             siStartInfo2;
SECURITY_ATTRIBUTES	sa;
CHAR                    chMsg[256];
TCHAR   		tchII_SYSTEM[ _MAX_PATH];
TCHAR   		tchII_INSTALLATION[3];
TCHAR   		tchII_TEMPORARY[ _MAX_PATH];
DWORD    		dwGlobalErr;
DWORD    		dwGlobalStatus;
STATUS      		dbstatus;
struct SETUID		setuid;
HANDLE			Setuid_Handle;
CHAR			SetuidShmName[64];
struct SETUID_SHM	*SetuidShmHandle;
struct SETUID_SHM	*SetuidShmPtr;
CHAR			RealuidShmName[64];
struct REALUID_SHM	*RealuidShmHandle;
struct REALUID_SHM	*RealuidShmPtr;
DWORD			BytesRead, BytesWritten;
