/*
**  Copyright (c) 1996, 1997 Ingres Corporation
**
**  Name: sql.c -- Call the terminal monitor.
**
**  Usage:
** 	sql dbname [flags]
**
**  Description:
**	This program provides the INGRES `isql' command on '95 by 
**	executing the full screen termainal monitor program (tm), 
**	passing along the command line arguments.
** 
**  Exit Status:
**	OK	tm succeeded.
**	FAIL	tm failed.	
**
**  History:
**	30-jan-2003 (somsa01)
**		Created.
**	3-feb-2004 (drivi01)
**		Changed JAVA_HOME environment variable
**		to II_JRE_HOME.	
*/

# include <compat.h>
# include <cm.h>
# include <er.h>
# include <lo.h>
# include <nm.h>
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

    st.dwFlags |= STARTF_USESTDHANDLES;
    st.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    st.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    st.hStdError  = GetStdHandle (STD_ERROR_HANDLE);

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
        WaitForSingleObject(pi.hProcess,INFINITE);
        GetExitCodeProcess(pi.hProcess, &status);
        CloseHandle(pi.hProcess);
    }
    else
    {
        status = GetLastError();
    }
    return(status);
}


void
main(int argc, char *argv[])
{
    char    buf[ 4096 ];
    LPSTR   lpCmdLine;
    char    *startquote;
    char    *endquote;
    char    *iisystem, *javahome;

    NMgtAt("II_SYSTEM", &iisystem);
    NMgtAt("II_JRE_HOME", &javahome);

    if (iisystem && *iisystem && javahome && *javahome)
    {
	STprintf(buf, "%s\\bin\\java -jar -Djava.library.path=\"%s\\ingres\\bin\" \"%s\\ingres\\lib\\UTMproject.jar\"", javahome, iisystem, iisystem);

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
    else
	PCexit(FAIL);
}
