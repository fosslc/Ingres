/*
**  Copyright (c) 1996, 1997 Ingres Corporation
**
**  Name: iquel.c -- Call the interactive terminal monitor for QUEL.
**
**  Usage:
** 	iquel dbname %1 %2 ...
**
**  Description:
**	This program provides the INGRES `isql' command on '95 by 
**	executing the full screen termainal monitor program (fstm), 
**	passing along the command line arguments.
** 
**  Exit Status:
**	OK	fstm succeeded.
**	FAIL	fstm failed.	
**
**  History:
**	01-may-1996 (emmag)
**		Created.  (based upon alterdb.c)
**      12-Nov-96 (fanra01)
**              Modified from using PCcmdline to use create process. As we need
**              more control over the behavior of children.
**      20-feb-97 (mcgem01)
**              Use GetCommand line instead of depending on argv as we're
**              losing some delimited quotes.  This way we wind up doing
**              a CreateProcess of exactly what was passed to the wrapper.
**	24-nov-1997 (canor01)
**		Allow quoted pathnames for executable to support embedded
**		spaces.
**	23-feb-1998 (canor01)
**		Fix typo in previous change.
**
*/

# include <compat.h>
# include <cm.h>
# include <er.h>
# include <lo.h>
# include <pc.h>
# include <st.h>


/*
**  Handler for console on Windows 95
**  Added since the passing of NULL to SetConsoleCtrlHandler causes exception
**  when ctrl-c pressed in windows 95.
*/
BOOL
HandlerRoutine(DWORD dwCtrlType)
{
    /* The exception has been handled do not exit */
    return(TRUE);
}

/*
**  Execute a command, blocking during its execution.
*/
static STATUS
execute( char *cmd )
{
CL_ERR_DESC err_code;
SECURITY_ATTRIBUTES sa;
STARTUPINFO     st;
PROCESS_INFORMATION pi;
HANDLE hProcess = 0;
HANDLE hFile = 0;
OSVERSIONINFO lpvers;
bool status = FALSE;
BOOL on_w95 = FALSE;
PHANDLER_ROUTINE pHandler = NULL;

    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    on_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;

    if (on_w95)
        pHandler = (PHANDLER_ROUTINE)HandlerRoutine;

    iimksec(&sa);
    /* Set up the STARTUPINFO structure for spawning the command. */
    st.cb = sizeof(st);
    st.lpReserved = NULL;
    st.lpReserved2 = NULL;
    st.cbReserved2 = 0;
    st.lpDesktop = NULL;
    st.lpTitle = NULL;
    st.dwX = st.dwY = 20;
    st.dwXSize = st.dwYSize = 40;
    st.dwFlags = STARTF_USESHOWWINDOW;  /* Start app according to */
    st.wShowWindow = SW_SHOWMINIMIZED;  /* wShowWindow setting */
    status = CreateProcess( NULL,
                            cmd,
                            &sa,
                            &sa,
                            TRUE,
                            NORMAL_PRIORITY_CLASS,
                            (LPVOID) NULL,
                            NULL,
                            &st,
                            &pi);

    SetConsoleCtrlHandler(pHandler,TRUE);   /* Turns off ctrl-c handling in  */
                                            /* this process                  */
    if(status == TRUE)
    {
        hProcess = pi.hProcess;
        WaitForSingleObject(pi.hProcess,INFINITE);
        CloseHandle(pi.hProcess);
        hProcess = 0;
    }
    return((status != TRUE) ? GetLastError(): 0);
}


void
main(int argc, char *argv[])
{
    char    buf[ 4096 ];
    LPSTR   lpCmdLine;
    char    *startquote;
    char    *endquote;

    STprintf(buf, ERx( "fstm" ));

    lpCmdLine = GetCommandLine();

    /*
    ** The current executable could have been invoked with the
    ** executable name in quotes.  If that is the case, skip
    ** over the quotes before looking for the arguments.
    */
    lpCmdLine = STskipblank( lpCmdLine, STlength(lpCmdLine) );
    if ( *lpCmdLine == '"' )
    {
        startquote = lpCmdLine;
        CMnext( startquote );
        if ( (endquote = STindex( startquote, "\"", 0 )) != NULL )
        {
            CMnext( endquote );
            lpCmdLine = endquote;
        }
    }
    if ( lpCmdLine = STindex(lpCmdLine, " ", 0) )
        STcat(buf, lpCmdLine);

    if ( execute(buf) != OK )
        PCexit(FAIL);

    PCexit(OK);
}
