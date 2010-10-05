#include <compat.h>
#include <clconfig.h>
#include <systypes.h>
#include <clnfile.h>
#include <lo.h>
#include <pc.h>
#include <er.h>
#include <ep.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <cs.h>
#include <me.h>

#include <sepdefs.h>
#include <fndefs.h>

#include <errno.h>

#include <stdlib.h>

/*
** History:
**
**	##-sep-1995 (somsa01)
**		Created. Used only on NT_GENERIC platforms with sepspawn.
**	26-Jan-1996 (somsa01)
**		Changed "nullfile.tmp" to "nul".
**      14-Jan-97 (fanra01)
**              Modified to use CreateProcess and to close handles not needed
**              by this process.  Previously preventing file deletion.
**      03-Feb-97 (fanra01)
**              Modified error handling from failed creation of console
**              handles.  Instead of exiting  try and continue with default
**              handle.
**	26-jul-1999 (somsa01)
**		When spawning a process, standard error for the child gets
**		redirected to standard out on UNIX. Let's follow suite here
**		on NT.
**      22-sep-2000 (mcgem01)
**              More nat and longnat to i4 changes.
**	27-feb-2001 (somsa01)
**		Make sure we don't re-close handles.
**	12-Nov-2007 (drivi01)
**		Add additional routines to handle handoffqa on Vista.
**		This file is compiled into sepchild.exe which is
**		responsible for running individual commands of a sep
**		test.  sep will run with elevated privileges to be
**		able to successfully create/attach to shared 
**		memory segments, but the privileges will be stripped in
**		this file to ensure individual commands run with lower
**		privileges.
**	07-Apr-2010 (drivi01)
**		Update the script to work on x64, by adding routines
**		to get around shell execution errors, updating 
**		allocated_pages datatype to SIZE_TYPE to be consistent
**		with ME parameters and cleanup the general warnings.
**	25-Jun-2010 (drivi01)
**		Add code to recognize different Token types.  
**		The existing code assumes that there will always be
**		a linked token attached to the current token
**		which is not true for Windows 2008.
**		The new code routines will determine if the
**		linked token exists and if it doesn't it will
**		the current token to initiate sub process.
**		On Windows 2008, the process is always elevated
**		there's no way to launch a process with stripped
**		token.
**	01-Oct-2010 (drivi01)
**		Added temporary fix on Windows 2008 to stop the hang.
**		The new change will bypass Vista code and execute
**		all commands in Administrative mode.
*/
/*
DEST = TOOLS

NEEDLIBS = COMPATLIB
**
*/
# define MAX_STD_HANDLES    4
# define MAX_CMD_LENGTH     1024

bool batchMode;

struct shm_struct {
  bool batchMode;
  i4  stderr_action;
  char prog_name[64];
  char in_name[256];
  char out_name[256];
  bool child_spawn;
};

static struct shm_struct stParams;

char *vistaException[] = {
	"sep.exe",
	"qasetuser.exe",
	"qasetusertm.exe",
	"qasetuserfe.exe",
	"rpserver.exe",
	"cp ", 
	NULL
};

char *vistaParamExcept[] = {
	"mklocdir.bat",
	"utl42mkloc.bat",
	NULL
};

char *win64Except[] = {
	"sh",
	"sh.exe",
	NULL
};

main(argc,argv)
i4  argc;
char **argv;
{
register i4             i;
char                    shm_name[20];
struct shm_struct       *childenv_ptr;
SIZE_TYPE               allocated_pages;
CL_ERR_DESC             err_code;
STATUS                  mem_status;
char                    full_prog_name[256];
int                     exit_code;
HANDLE                  hConIO[MAX_STD_HANDLES];
char                    buf[64];
STARTUPINFO             si;
PROCESS_INFORMATION     pi;
SECURITY_ATTRIBUTES     sa;
char                    *pArgLine;
BOOL                    blStatus = FALSE;
DWORD                   dwAttrib = 0;
BOOL			bSkip = 0;
int			dwRet;
char *prog_name;

    /*
    **  Initialise security attributes for use with handle creation and
    **  duplication.
    */
    iimksec(&sa);
    /*
    **  Remove the shared memory name from the list of arguments.
    */
    strcpy (shm_name, argv[argc-1]);
    argv[argc-1]=NULL;
    argc=argc-1;

    ELEVATION_REQUIRED();
    /*
    **  Get the pointer tp shared memory and copy the contents into a local
    **  area.
    */
    mem_status = IIMEget_pages(ME_SSHARED_MASK,1,shm_name,(PTR *)&childenv_ptr,
                               &allocated_pages,&err_code);

    if (mem_status == OK)
    {
        MEcopy(childenv_ptr, sizeof(struct shm_struct), &stParams);
        childenv_ptr->child_spawn = TRUE;
        mem_status = IIMEfree_pages((PTR)childenv_ptr,1,&err_code);
    }
    else
    {
        _exit(509);
    }

    /*
    **  Allocate an area to reconstruct the command line from the argv ptrs.
    */
    pArgLine = MEreqmem (0,MAX_CMD_LENGTH,TRUE,NULL);
    if (pArgLine != NULL)
    {
        for (i=0; (i < argc) && (argv[i] != NULL); i++)
        {
            STcat(pArgLine,argv[i]);
            if (argc > (i+1))
                STcat(pArgLine," ");
        }
    }
    else
    {
        _exit(509);
    }

    /*
    **  Initialise the handle array which will take the created or duplicated
    **  handles for standard I/O.
    */
    for (i = 0; i < MAX_STD_HANDLES; i++)
    {
        hConIO[i] = 0;
    }

    /*
    **  Setup standard input handle for the child process.
    **  If input is redirected from a file open the file. If we're not in batch
    **  get the current handle.
    */
    if ((stParams.batchMode) || (stParams.in_name[0] != 0))
    {
        if (stParams.in_name[0] == 0)
        {
            STcopy((PTR)ERx("c:\\nul"),stParams.in_name);
        }
        dwAttrib = OPEN_ALWAYS;
    }
    else
    {                                   /* Batch mode with no std input */
        STcopy((PTR)ERx("CONIN$"),stParams.in_name);
        dwAttrib = OPEN_EXISTING;
    }
    hConIO[1] = CreateFile(
                            stParams.in_name,
                            GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            &sa,
                            dwAttrib,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                          );
    if (hConIO[1] == INVALID_HANDLE_VALUE)
    {
        hConIO[1] = NULL;
    }

    /*
    **  Setup standard output handle for the child process.
    **  If output is redirected from a file open the file. If we're not in
    **  batch mode get the current handle.
    */
    if ((stParams.batchMode) || (stParams.out_name[0] != 0))
    {
        if (stParams.out_name[0] == 0)
        {
            STcopy((PTR)ERx("c:\\nul"),stParams.out_name);
        }
        dwAttrib = OPEN_ALWAYS;
    }
    else
    {                                   /* Batch mode with no std input */
        STcopy((PTR)ERx("CONOUT$"),stParams.out_name);
        dwAttrib = OPEN_EXISTING;
    }
    hConIO[2] = CreateFile(
                            stParams.out_name,
                            GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            &sa,
                            dwAttrib,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                          );
    if (hConIO[2] == INVALID_HANDLE_VALUE)
    {
        hConIO[2] = NULL;
    }
    /*
    **  Setup standard error handle for the child process.
    */
    switch (stParams.stderr_action)
    {
        case 1:
	    hConIO[3] = CreateFile(
                                    "c:\\nul",
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    &sa,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                  );
            break;
        case 0:
        case 2:
	    /* Standard error goes to Standard out */
	    hConIO[3] = hConIO[2];
            break;
    }
    if (hConIO[3] == INVALID_HANDLE_VALUE)
    {
        hConIO[3] = NULL;
    }

    si.cb = sizeof (STARTUPINFO);
    si.lpReserved       = 0;
    si.lpDesktop        = NULL;
    si.lpTitle          = NULL;
    si.dwX              = 0;
    si.dwY              = 0;
    si.dwXSize          = 0;
    si.dwYSize          = 0;
    si.dwXCountChars    = 0;
    si.dwYCountChars    = 0;
    si.dwFillAttribute  = 0;
    si.dwFlags          = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    si.wShowWindow      = SW_HIDE;
    si.hStdInput        = (hConIO[1])?hConIO[1]:GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput       = (hConIO[2])?hConIO[2]:GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError        = (hConIO[3])?hConIO[3]:GetStdHandle(STD_ERROR_HANDLE);
    si.cbReserved2      = 0;
    si.lpReserved2      = 0;

    if (SearchPath (NULL, stParams.prog_name, ".com",
                    sizeof(full_prog_name),full_prog_name, NULL)==0)
		if (SearchPath (NULL, stParams.prog_name, ".bat",
                        sizeof(full_prog_name),full_prog_name, NULL)==0)
			if (SearchPath (NULL, stParams.prog_name, ".exe",
                            sizeof(full_prog_name),full_prog_name, NULL)==0)
                if (SearchPath (NULL, stParams.prog_name, ".cmd",
                                sizeof(full_prog_name),full_prog_name, NULL)==0)
    {
        /* Handle error condition */
        STprintf(buf, "SearchPath failed %d", GetLastError());
        SIfprintf(stderr, "%s\n", buf);
        SIflush(stderr);
        _exit(509);
    }

#ifdef WIN64
	/*
	** 05-Aug-2009 (drivi01)
	** Sep is having trouble executing sh.exe from cygwin on x64.
	** I suspect the errors are due to the fact that this program is 
	** compiled as 64-bit on x64 and sh.exe is 32-bit cygwin exe
	** and it's having trouble running 32-bit application from a
	** 64-bit application, but if the full program path is moved
	** to command line buffer instead as a 2nd argument to 
	** CreateProcess and first argument which is the app name set
	** to NULL, then the CreateProcess should succeed.
	** This bit of code moves the arguments to CreateProcess around
	** to have successfully executed shell program.
	*/
	for (i=0; win64Except[i]; i++)
		if (STstrindex (stParams.prog_name, win64Except[i], 0, TRUE) != NULL)
		{
			char *ptr;
			char tmp[MAX_CMD_LENGTH];

			ptr=pArgLine+STlength(win64Except[i]);
			MEcopy(ptr, MAX_CMD_LENGTH-STlength(win64Except[i]), &tmp);
			*pArgLine='\0';
			STpolycat(2, full_prog_name, tmp, pArgLine);
			*full_prog_name='\0';
			break;
		}
#endif //WIN64
	
	for (i = 0; vistaException[i]; i++)
		if (STstrindex (full_prog_name, vistaException[i], 0, TRUE) != NULL)
		{
			bSkip = TRUE;
			break;
		}
	for (i = 0; vistaParamExcept[i]; i++)
		if (STstrindex(pArgLine,vistaParamExcept[i], 0, TRUE) != NULL)
		{
			bSkip = TRUE;
			break;
		}
	/*
	** If this is Windows 2008, skip the Vista portion of the code for
	** now.  All the tests will be run with Admin privileges.
	** This is a temporary solution until the hang in the Vista
	** routines is fixed on Win 2008.
	*/ 
	if (GVvista())
	{
		OSVERSIONINFOEX	OSVers = { 0 };
		OSVers.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if (GetVersionEx((OSVERSIONINFOEX *)&OSVers) &&
			(OSVers.wProductType != VER_NT_WORKSTATION) &&
				OSVers.dwMajorVersion == 6)
			bSkip = TRUE;
	}
	prog_name = (*full_prog_name) ? full_prog_name : NULL;
	
	if (GVvista() && !bSkip)
	{
		DWORD errnum;
		char buf[64];
		HANDLE hToken, hToken2 = NULL;
		TOKEN_LINKED_TOKEN linked_token;
		TOKEN_ELEVATION_TYPE elevation_type;
		DWORD dwSize;

		if (OpenProcessToken( GetCurrentProcess(), 
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
			&hToken ) == TRUE)
		{
			if (!SetPrivilege(hToken, SE_TCB_NAME, TRUE))
			{
				errnum = GetLastError();
				STprintf(buf, "Can't set SE_TCB_NAME privilege, errno %d", errnum);
				SIfprintf(stderr, "%s\n", buf);
				SIflush(stderr);
				_exit(509);
			}
			if (!GetTokenInformation(hToken, TokenElevationType,
				(LPVOID)&elevation_type, sizeof(elevation_type), &dwSize))
			{
				STprintf(buf, "GetTokenInformation, TokenElevationType: (%d)\r\n", GetLastError());
				SIfprintf(stderr, "%s\n", buf);
				SIflush(stderr);
				_exit(509);
			}
			else
			{
				switch(elevation_type)
				{
				    case TokenElevationTypeFull:
				    {
					if (!GetTokenInformation(hToken, TokenLinkedToken, 
						(LPVOID)&linked_token, sizeof(linked_token), &dwSize))
					{
					    STprintf(buf, "GetTokenInfo: (%d)\r\n", GetLastError());
					    SIfprintf(stderr, "%s\n", buf);
					    SIflush(stderr);
					    _exit(509);
					}
					else
					    hToken2 = linked_token.LinkedToken;
					break;
				    }
				    case TokenElevationTypeLimited:
				    {
					STprintf(buf, "Sep tool should be run from Elevated cmd prompt.\r\nCurrent process has wrong elevation.\r\nRestart the script in the elevated command prompt.\r\n");
					SIfprintf(stderr, "%s\n", buf);
					SIflush(stderr);
					_exit(509);
				    }
				    case TokenElevationTypeDefault:
					hToken2 = hToken;  //If no linked token exist, just use the current token
					break;
				}
			}
		}

		if (hToken2)
		{
			dwRet = CreateProcessAsUser (hToken2, 
					prog_name, pArgLine, NULL, NULL, TRUE,
        		        	CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS,NULL,
					NULL,&si,&pi);
			if ( dwRet == TRUE)
			{  /* Child was created successfully */
				if (hConIO[1])
					CloseHandle(hConIO[1]);
				if (hConIO[2])
					CloseHandle(hConIO[2]);
				if (hConIO[3] && hConIO[3] != hConIO[2])
					CloseHandle(hConIO[3]);
				WaitForSingleObject(pi.hProcess, INFINITE);
				GetExitCodeProcess(pi.hProcess,&exit_code);
				CloseHandle(hToken2);
				_exit (exit_code);
			}
			else
			{
				DWORD rw;
				rw = GetLastError();
				STprintf(buf, "CreateProcess failed %d, command: %s", rw, full_prog_name);
				SIfprintf(stderr, "%s\n", buf);
				SIflush(stderr);
				CloseHandle(hToken2);
				if (hConIO[1])
					CloseHandle(hConIO[1]);
				if (hConIO[2])
					CloseHandle(hConIO[2]);
				if (hConIO[3] && hConIO[3] != hConIO[2])
					CloseHandle(hConIO[3]);
				if (rw == PC_ELEVATION_REQUIRED)
					exit(rw);
				_exit(509);
			}
		}
	}
	else
	{
	dwRet = CreateProcess (
                        prog_name,
                        pArgLine,
                        NULL,
                        NULL,
                        TRUE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &si,
                        &pi);
	if (dwRet==0)
    	{
        STprintf(buf, "CreateProcess failed %d", GetLastError());
        SIfprintf(stderr, "%s\n", buf);
        SIflush(stderr);
        if (hConIO[1])
            CloseHandle(hConIO[1]);
        if (hConIO[2])
            CloseHandle(hConIO[2]);
        if (hConIO[3] && hConIO[3] != hConIO[2])
            CloseHandle(hConIO[3]);
        _exit(509);
    }
    else
    {
        if (hConIO[1])
            CloseHandle(hConIO[1]);
        if (hConIO[2])
            CloseHandle(hConIO[2]);
        if (hConIO[3] && hConIO[3] != hConIO[2])
            CloseHandle(hConIO[3]);
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess,&exit_code);
        _exit (exit_code);
    }
	}
} /* sepchild */


/*
** Description: Enables/Disables sepcified privilege in a token. 
**
** History:
**	12-Nov-2007 (drivi01)
**		Added.
*/
BOOL 
SetPrivilege(
    HANDLE hToken,
    LPCTSTR Privilege,
    BOOL bEnablePrivilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious ZERO_FILL;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

    if (!LookupPrivilegeValue( NULL, Privilege, &luid )) 
	return (FALSE);

    /*
    ** first pass.  get current privilege setting
    */
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) 
	return (FALSE);

    /*
    ** second pass.  set privilege based on previous setting
    */
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if (bEnablePrivilege) 
    {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else 
    {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
                                           tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) 
	return (FALSE);

    return (TRUE);
}

