/*
** Copyright (C) 1996, 2001 Ingres Corporation All Rights Reserved.
*/
/*
** Name:    pctask.c
**
** Description:
**      Enumerate the process list.
**
** History:
**      27-Oct-97 (fanra01)
**          Modified to get process list in Windows 95.
**	26-mar-1998 (somsa01)
**	    Needed to initialize dwLimit in GetProcessList() to MAX_TASKS.
**      14-Apr-98 (fanra01)
**          During read of performance key remember the size of memory used.
**          The RegQueryValueEx call corrupts the value when reading
**          HKEY_PERFORMANCE_DATA and returning ERROR_MORE_DATA.
**      10-Jul-98 (fanra01)
**          Reset lStatus after buffer extend otherwise the loop exits too
**          soon.
**      31-aug-1998 (canor01)
**          Remove other CL dependencies so the object can be linked into
**          arbitrary programs without requiring the full CL.
**	01-apr-1999 (somsa01)
**	    Added PCsearch_Pid, which takes a pid and searches the task
**	    list to see if it is there.
**	29-jun-1999 (somsa01)
**	    In GetProcessList(), we were doing a RegCloseKey() on
**	    HKEY_PERFORMANCE_DATA even though we never had this key
**	    specifically opened. This call was what caused this function
**	    to take an enormous amount of time, and also caused stack
**	    corruption. Sometimes (specifically when Oracle and Microsoft
**	    SQL Server were installed on the same machine), this would
**	    eventually lead a process that used PCget_PidFromName() to
**	    give an Access Violation on a printf (or, SIprintf) statement.
**	13-sep-1999 (somsa01)
**	    In GetProcessList(), initialize dwNumTasks to zero so that, in
**	    case of errors, we do not return a bogus value.
**	03-nov-1999 (somsa01)
**	    Put back the RegCloseKey() on HKEY_PERFORMANCE_DATA in
**	    GetProcessList(). Fixes elswhere have properly fixed 97704.
**	28-jun-2001 (somsa01)
**	    For OS Version, use GVosvers().
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	20-oct-2001 (somsa01)
**	    Added inclusion of systypes.h.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	07-May-2008 (whiro01)
**	    Made more common code routines.
*/

# include <stdio.h>
# include <windows.h>
# include <winperf.h>
# include <tlhelp32.h>
# include <compat.h>
# include <systypes.h>

//
// manifest constants
//
#define MAX_TASKS           256

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  "Counters"
#define PROCESS_COUNTER     "process"
#define PROCESSID_COUNTER   "id process"
#define UNKNOWN_TASK        "unknown"
#define TITLE_SIZE          64
#define PROCESS_SIZE        16

typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

TASK_LIST tlist[MAX_TASKS];

static DWORD GetProcessList (PTASK_LIST  pTask);
static DWORD GetProcessListNT (PTASK_LIST  pTask);
static DWORD GetProcessList95 (PTASK_LIST  pTask);
static int   Is_w95 = -1;

static OFFSET_TYPE (FAR WINAPI *pfnCreateToolhelp32Snapshot)() = NULL;
static OFFSET_TYPE (FAR WINAPI *pfnProcess32First)() = NULL;
static OFFSET_TYPE (FAR WINAPI *pfnProcess32Next)() = NULL;

extern BOOL GVosvers(char *OSVersionString);

/*
** Name: PCget_PidFromName
**
** Description:
**      Returns the process id of the specified process
**
** Inputs:
**      pszProcess      name of the process
**
** Outputs:
**      None.
**
** Returns:
**      dwProcessId
**      0               if not found
**
** History:
**      01-Oct-97 (fanra01)
**          Created.
**      28-Oct-97 (fanra01)
**          Add Windows 95 specific calls to get process list.  Load the
**          functions from the DLL otherwise builds will break.
**          Modified string compare.  Since the format of string is different
**          between NT and W95.
**	07-May-2008 (whiro01)
**	    Made more common code routines.
*/
DWORD
PCget_PidFromName (void *pszProcessName)
{
    DWORD dwNumProc;
    unsigned int i;

    strlwr(pszProcessName);

    dwNumProc = GetProcessList (tlist);

    if (dwNumProc != 0)
    {
        for (i=0; i < dwNumProc; i++)
        {
            if (strstr(strlwr(tlist[i].ProcessName), pszProcessName)
                != NULL)
            {
                return (tlist[i].dwProcessId);
            }
        }
    }
    return (0);
}

/*
** Name: PCsearch_Pid
**
** Description:
**	Searches the machine's process list to see if a particular pid
**	exists for a particular process name.
**
** Inputs:
**	pszProcess	name of the process
**	pid		pid to search for
**
** Outputs:
**	None.
**
** Returns:
**	1	if found
**	0	if not found
**
** History:
**	01-apr-1999 (somsa01)
**	    Created.
*/
DWORD
PCsearch_Pid (void *pszProcessName, DWORD pid)
{
DWORD dwNumProc;
unsigned int i;

    strlwr(pszProcessName);

    dwNumProc = GetProcessList (tlist);

    if (dwNumProc != 0)
    {
	for (i=0; i < dwNumProc; i++)
	{
	    if ((tlist[i].dwProcessId == pid) &&
		(strstr(strlwr(tlist[i].ProcessName), pszProcessName)))
	    {
		return (1);
	    }
	}
    }
    return (0);
}

/*
** Name:    GetProcessListNT
**
** Description:
**      Obtains a list of running processes.
**
** Inputs:
**      pTask           array of task list structures.
**
** Outputs:
**      none.
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**      01-Oct-97 (fanra01)
**          Created.
**	26-mar-1998 (somsa01)
**	    Needed to initialize dwLimit in GetProcessList() to MAX_TASKS.
**      14-Apr-98 (fanra01)
**          During read of performance key remember the size of memory used.
**          The RegQueryValueEx call corrupts the value when reading
**          HKEY_PERFORMANCE_DATA and returning ERROR_MORE_DATA.
**	29-jun-1999 (somsa01)
**	    We were doing a RegCloseKey() on HKEY_PERFORMANCE_DATA even
**	    though we never had this key specifically opened. This call was
**	    what caused this function to take an enormous amount of time, and
**	    also caused stack corruption. Sometimes (specifically when Oracle
**	    and Microsoft SQL Server were installed on the same machine),
**	    this would eventually lead a process that used
**	    PCget_PidFromName() to give an Access Violation on a printf (or,
**	    SIprintf) statement.
**	13-sep-1999 (somsa01)
**	    Initialize dwNumTasks to zero so that, in case of errors, we do
**	    not return a bogus value.
**	03-nov-1999 (somsa01)
**	    Put back the RegCloseKey() on HKEY_PERFORMANCE_DATA. Fixes
**	    elswhere have properly fixed 97704.
*/
static DWORD
GetProcessListNT (PTASK_LIST  pTask)
{
LANGID                      lid;        /* language id */
LONG                        lStatus;
char                        szSubKey[1024];
char                        szProcessName[MAX_PATH];
HKEY                        hKeyNames = 0;
DWORD                       dwLimit = MAX_TASKS;
DWORD                       dwType;
DWORD                       dwSize;
DWORD                       dwProcessIdTitle;
DWORD                       dwProcessIdCounter;
DWORD                       dwNumTasks = 0;
PPERF_DATA_BLOCK            pPerf;
PPERF_OBJECT_TYPE           pObj;
PPERF_INSTANCE_DEFINITION   pInst;
PPERF_COUNTER_BLOCK         pCounter;
PPERF_COUNTER_DEFINITION    pCounterDef;
char                        *pBuf = NULL;
unsigned int                i;

    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    sprintf( szSubKey, "%s\\%03x", REGKEY_PERF, lid );
    lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           szSubKey,
                           0,
                           KEY_READ,
                           &hKeyNames
                          );
    if (lStatus == ERROR_SUCCESS)
    {
        /*
        ** get the buffer size for the counter names
        */
        lStatus = RegQueryValueEx( hKeyNames,
                                   REGSUBKEY_COUNTERS,
                                   NULL,
                                   &dwType,
                                   NULL,
                                   &dwSize
                                 );
    }
    if (lStatus == ERROR_SUCCESS)
    {
        if ((pBuf = calloc(1,dwSize)) != NULL)
        {
            /*
            ** read the counter names from the registry
            */
            lStatus = RegQueryValueEx( hKeyNames,
                                       REGSUBKEY_COUNTERS,
                                       NULL,
                                       &dwType,
                                       pBuf,
                                       &dwSize
                                     );
            if (lStatus == ERROR_SUCCESS)
            {
            char* p;

                /*
                ** loop through counter names looking for counters:
                **
                **      1.  "Process"           process name
                **      2.  "ID Process"        process id
                **
                ** the buffer contains multiple null terminated strings and
                ** then finally null terminated at the end.  The strings
                ** are in pairs of counter number and counter name.
                */

                p = pBuf;
                while (*p)
                {
                    /* Skip the counter number for now, but remember where it is */
                    char *pNumber = p;
                    p += (strlen(p) + 1);
                    if (stricmp(p, PROCESS_COUNTER) == 0)
                    {
                        strcpy( szSubKey, pNumber );
                    }
                    else
                    if (stricmp(p, PROCESSID_COUNTER) == 0)
                    {
                        dwProcessIdTitle = atol( pNumber );
                    }
                    /*
                    ** next string
                    */
                    p += (strlen(p) + 1);
                }
            }
            free (pBuf);
            pBuf = NULL;
        }
    }
    if (lStatus == ERROR_SUCCESS)
    {
        DWORD   dwKeySize = INITIAL_SIZE;
        /*
        ** allocate the initial buffer for the performance data
        */
        dwSize = dwKeySize;
        if ((pBuf = calloc(1,dwSize)) != NULL)
        {
            while (lStatus == ERROR_SUCCESS)
            {
                lStatus = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                                           szSubKey,
                                           NULL,
                                           &dwType,
                                           pBuf,
                                           &dwSize
                                         );

                pPerf = (PPERF_DATA_BLOCK) pBuf;

                /*
                ** check for success and valid perf data block signature
                */
                if ((lStatus == ERROR_SUCCESS) &&
                    (dwSize > 0) &&
                    (pPerf)->Signature[0] == (WCHAR)'P' &&
                    (pPerf)->Signature[1] == (WCHAR)'E' &&
                    (pPerf)->Signature[2] == (WCHAR)'R' &&
                    (pPerf)->Signature[3] == (WCHAR)'F' )
                {
                    break;
                }

                /*
                ** if buffer is not big enough, reallocate and try again
                */
                if (lStatus == ERROR_MORE_DATA)
                {
                    free (pBuf);
                    dwKeySize += EXTEND_SIZE;
                    dwSize = dwKeySize;
		    lStatus = ERROR_SUCCESS;	
                    pBuf = calloc(1,dwSize);
                }
            }
            if (lStatus == ERROR_SUCCESS)
            {
                /*
                ** set the perf_object_type pointer
                */
                pObj = (PPERF_OBJECT_TYPE)((OFFSET_TYPE)pPerf +
					   pPerf->HeaderLength);

                /*
                ** loop thru the performance counter definition records looking
                ** for the process id counter and then save its offset
                */
                pCounterDef = (PPERF_COUNTER_DEFINITION)
                                  ((OFFSET_TYPE)pObj + pObj->HeaderLength);
                for (i=0; i<(DWORD)pObj->NumCounters; i++)
                {
                    if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
                        dwProcessIdCounter = pCounterDef->CounterOffset;
                        break;
                    }
                    pCounterDef++;
                }

                dwNumTasks = min( dwLimit, (DWORD)pObj->NumInstances );

                pInst = (PPERF_INSTANCE_DEFINITION)
                                  ((OFFSET_TYPE)pObj + pObj->DefinitionLength);

                /*
                ** loop thru the performance instance data extracting each process name
                ** and process id
                */
                for (i=0; i<dwNumTasks; i++)
                {
                char* p;
                    /*
                    ** pointer to the process name
                    */
                    p = (LPSTR) ((OFFSET_TYPE)pInst + pInst->NameOffset);

                    /*
                    ** convert it to ascii
                    */
                    lStatus = WideCharToMultiByte( CP_ACP,
                                                   0,
                                                   (LPCWSTR)p,
                                                   -1,
                                                   szProcessName,
                                                   sizeof(szProcessName),
                                                   NULL,
                                                   NULL
                                                 );

                    if (!lStatus)
                    {
            	        /*
                        ** if we cant convert the string then use a default
                        ** value
                        */
                        strcpy( pTask->ProcessName, UNKNOWN_TASK );
                    }
                    else
                    {
                        if (strlen(szProcessName)+4 <=
                                sizeof(pTask->ProcessName))
                        {
                            strcpy( pTask->ProcessName, szProcessName );
                            strcat( pTask->ProcessName, ".exe" );
                        }
                    }

                    /*
                    ** get the process id
                    */
                    pCounter = (PPERF_COUNTER_BLOCK) ((OFFSET_TYPE)pInst +
                                pInst->ByteLength);
                    pTask->flags = 0;
                    pTask->dwProcessId = *((LPDWORD) ((OFFSET_TYPE)pCounter +
						      dwProcessIdCounter));
                    if (pTask->dwProcessId == 0)
                    {
                        pTask->dwProcessId = (DWORD)-2;
                    }

                    /*
                    ** next process
                    */
                    pTask++;
                    pInst = (PPERF_INSTANCE_DEFINITION)
				((OFFSET_TYPE)pCounter + pCounter->ByteLength);
                }
            }
        }
    }
    if (pBuf)
        free(pBuf);
    if (hKeyNames)
        RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );
    return (dwNumTasks);
}

/*
** Name:    GetProcessList95
**
** Description:
**      Obtains a list of running processes in Windows 95.
**
** Inputs:
**      pTask           array of task list structures.
**
** Outputs:
**      none.
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**      27-Oct-97 (fanra01)
**          Created.
*/
static DWORD
GetProcessList95 (PTASK_LIST  pTask)
{
HANDLE          hProcessSnap = NULL;
BOOL            bRet         = FALSE;
PROCESSENTRY32  pe32         = {0};
DWORD           dwNumTasks   = 0;

    hProcessSnap = (HANDLE)pfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == (HANDLE)-1)
        return (dwNumTasks);

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (pfnProcess32First(hProcessSnap, &pe32))
    {
        do
        {
            pTask->flags = 0;
            pTask->dwProcessId = pe32.th32ProcessID;
            strcpy( pTask->ProcessName, pe32.szExeFile );
            if (pTask->dwProcessId == 0)
            {
                pTask->dwProcessId = (DWORD)-2;
            }
            pTask++;
            dwNumTasks += 1;
        } while (pfnProcess32Next(hProcessSnap, &pe32) && dwNumTasks < MAX_TASKS);
    }
    CloseHandle (hProcessSnap);
    return (dwNumTasks);
}

/*
** Name:    GetProcessList
**
** Description:
**      Wrapper for one of the NT or 95 routines.
**
** Inputs:
**      pTask           pointer to array of task list structures.
**
** Outputs:
**      pTask           array entries filled in
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**	07-May-2008 (whiro01)
**	    Made more common code routines.
*/
static DWORD
GetProcessList(PTASK_LIST  pTask)
{
DWORD dwNumProc;

    if (Is_w95 == (int)-1)
    {
	char	VersionString[256];

	GVosvers(VersionString);
	Is_w95 = (strstr(VersionString, "Microsoft Windows 9") != NULL) ?
								TRUE : FALSE;
    }
    if (!Is_w95)
    {
	dwNumProc = GetProcessListNT (tlist);
    }
    else
    {
	HANDLE hKernel32 = LoadLibrary (TEXT("kernel32.dll"));

	if (hKernel32 != NULL)
	{
	    pfnCreateToolhelp32Snapshot=GetProcAddress (hKernel32,
					TEXT("CreateToolhelp32Snapshot"));
	    pfnProcess32First=GetProcAddress (hKernel32,
					TEXT("Process32First"));
	    pfnProcess32Next=GetProcAddress (hKernel32,
					TEXT("Process32Next"));
	    if (pfnCreateToolhelp32Snapshot && pfnProcess32First &&
		pfnProcess32Next)
	    {
		dwNumProc = GetProcessList95 (tlist);
	    }
	    FreeLibrary (hKernel32);
	    CloseHandle (hKernel32);
	}
    }
    return dwNumProc;
}
