/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: clntproc.h
**
**  History:
**
*/

BOOL    ReportStatusToSCMgr (DWORD dwCurrentState,
                                DWORD dwWin32ExitCode,
                                DWORD dwCheckPoint,
                                DWORD dwWaitHint);
VOID    OpenIngresService (DWORD dwArgc, LPSTR *lpszArgv);
int     StartIngresServer ();
WINAPI  IngresCtrlHandler (DWORD);
VOID    ReportIngresEvents (LPTSTR lpszMsg);
VOID    StopService (LPTSTR lpszMsg);
BOOL    StopServer  ();

SERVICE_STATUS_HANDLE   sshStatusHandle;
HANDLE                  hServDoneEvent = NULL;
SERVICE_STATUS          ssServiceStatus;
LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
BOOL                    isstarted;
PROCESS_INFORMATION     piProcInfo;
PROCESS_INFORMATION     piProcInfo2;
STARTUPINFO             siStartInfo;
CHAR                    chMsg[256];
TCHAR   		tchII_SYSTEM[ _MAX_PATH];
char    		*iisystem;
CL_ERR_DESC     	err_code;
DWORD    		dwGlobalErr;
DWORD    		dwGlobalStatus;
STATUS      		dbstatus;
