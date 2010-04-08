/*
**  Copyright (c) 1996 Ingres Corporation
**
**  Name: esqlcc.c -- Call the interactive terminal monitor.
**
**  Usage:
** 	esqlcc [flags] [files]
**
**  Description:
**	This program provides the INGRES `esqlcc' command on NT & '95 by 
**	executing the embedded SQL/C++ pre-compiler, passing along 
**	the command line arguments.
** 
**  Exit Status:
**	OK	esqlcc succeeded.
**	FAIL	esqlcc failed.	
**
**  History:
**	10-jun-1996 (emmag)
**		Created.  (based upon isql.c)
**      12-Nov-96 (fanra01)
**              Modified from using PCcmdline to use create process. As we need
**              more control over the behavior of children.
**	28-aug-97 (mcgem01)
**		Change the default extension to .cpp on NT and '95.
**
*/

# include <compat.h>
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
#define 	MAXBUF	4095

	char 	buf[ MAXBUF+1 ];
	int		iarg, ibuf, ichr;

	/*
	 *	Put the program name and first parameter in the buffer.
	 */
	STprintf(buf, ERx( "esqlc -cplusplus -extension=cpp" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		esqlc -cplusplus -extension=EXT [files]
		 */
	    default:
		ibuf = sizeof(ERx("esqlc -cplusplus -extension=cpp")) - 1;
		for (iarg = 1; (iarg < argc) && (ibuf < MAXBUF); iarg++) {
		    buf[ibuf++] = ' ';
		    for (ichr = 0; 
		    	 (argv[iarg][ichr] != '\0') && (ibuf < MAXBUF); 
		    	 ichr++, ibuf++)
		    	buf[ibuf] = argv[iarg][ichr];
		}
		buf[ibuf] = '\0';
	}

	/*
	 *	Execute the command.
	 */
	if( execute(buf) != OK )
	    PCexit(FAIL);

	PCexit(OK);
}
