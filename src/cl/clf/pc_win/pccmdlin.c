/*****************************************************************************
**
** Copyright (c) 1989, 2001 Ingres Corporation
**
*****************************************************************************/
#define 	W32SUT_32
#include       <stdio.h>
#include       <conio.h>
#include       <io.h>

#include       <compat.h>
#include       <cm.h>
#include       <lo.h>
#include       <er.h>
#include       <pc.h>
#include       <me.h>
#include       <nm.h>
#include       <st.h>
#include       <si.h>
#include       <iiwin32s.h>
#include       <clconfig.h>
#include       <PCerr.h>
#include	"pclocal.h"

#ifndef _INC_STRING
#include <string.h>
#endif

static i2 showproperty[] = { SW_HIDE,
                             SW_SHOWNORMAL,
                             SW_SHOWMINIMIZED,
                             SW_SHOWMAXIMIZED,
                             SW_SHOWNOACTIVATE,
                             SW_SHOW,
                             SW_MINIMIZE,
                             SW_SHOWMINNOACTIVE,
                             SW_SHOWNA,
                             SW_RESTORE,
                             SW_SHOWDEFAULT };

static char *findspace( char *string );
static bool isshellcommand( char *command );
static char *shellcommand[] = {
    "echo", "cd", "date", "time", "rmdir", "mkdir", NULL };

/*
**
**      Name:
**              PCcmdline.c
**
**      Function:
**              PCcmdline
**
**      Arguments:
**              LOCATION        *interpreter;
**              char            *cmdline;
**              i4              wait;
**              LOCATION        *err_log;
**              CL_ERR_DESC     *err_code;
**
**      Result:
**              Execute command line.
**
**              Invoke the specified interpreter to execute the command line.
**                      A NULL argument means use the default.
**
**              Returns:
**                      OK              -- success
**                      FAIL            -- general failure
**
**      Side Effects:
**              None
**  History
**
**      28-Jun-95 (fanra01)
**          Modified the CreateFile flags to correctly open a file in the
**          append case.
**          Also added FILE_SHARE_READ flag to creation of stderr replacement,
**          since it is the same file, the share flags must match those
**          originally used for creation.
**      20-jul-95 (reijo01)
**          Fix SETCLOS32ERR macro
**	02-apr-1996 (canor01)
**	    Set an error in err_code only if the process failed to execute.
**	01-may-96 (emmag)
**	    On Windows '95, because we are using anonymous pipes
**	    we should call the application directly and not call
**	    the command interpreter passing it the application name
**	    and argument list.   This change is being applied for
**	    '95 only.   
**	17-dec-1996 (donc)
**	    Blend OpenROAD versions of these routines into this module
**	    so that both the traditional, character-based algorithms work,
**	    in addition to newer GUI-based ones.  The traditional behavior
**	    occurs by default. 
**	20-feb-97 (fraser)
**	    In PCcmdline_or removed test for Win32s because we no longer support Win32s.
**	    Also removed the function PC32scmdline which was used only for Win32s.
**	21-feb-1997 (canor01)
**	    Since we may be called from a DLL with no console and no
**	    inheritance on standard I/O handles, if PCcmdline() is redirecting
**	    its output, specify the standard handles in the STARTUPINFO
**	    structure.
**	22-feb-97 (mcgem01)
**	    Remove include of w32sut.h.  Also, added include of nm.h to 
**	    silence compiler warning.
**	27-mar-97 (trevor)
**		Modify PCwntcmdline so that we try to start commands
**      directly rather than use a command interpreter unless
**      absolutely necessary. Fixes 79166, 79782.
**		Modify pumpMsgLoop so that OpenROAD can redraw its
**		windows while blocked by call system. Fixes 80271.
**	05-may-1997 (canor01)
**	    Move fix from PCdocmdline() to PCdowindowcmdline() to check 
**	    process's exit code for failure as well as failure of 
**	    CreateProcess().
**	25-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	02-jun-97 (mcgem01)
**	    Allow for a command line greater than 1024 characters.
**	15-jun-97 (mcgem01)
**	   Need to allow room for the ComSpec when a NULL command line
**	   is passed to PCdocmdlin or PCdowindowncmdline.
**      30-Oct-97 (fanra01)
**          Updated to use test when an interpreter should be prepended.
**          Updated to default to inherit parent console and to set the
**          DETACHED_PROCESS flag if the I/O handles are redirected or
**          an output file is specified.
**          Also removed unused function PCdowindowcmdline.
**	24-nov-1997 (canor01)
**	    Allow path to executable to contain spaces.
**	14-oct-98 (mcgem01)
**	    ComSpec should override any value set for SHELL on NT.  If
**	    ComSpec isn't specified, then we'll use the value for SHELL.
**	17-mar-1999 (crido01) Bug 94020
**	    Have PCdocmdline check to see if it is being invoked from
**	    within OpenROAD, and if so call PCwntcmdline instead of doing
**	    what it normally would do.  Also make the OpenROAD-exclusive
**	    PCwntcmdline routine enable handle inheritance when doing
**	    the CreateProcess, otherwise the shared database support doesn't
**	    work.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      27-Jan-2001 (fanra01)
**          Bug 104248
**          Ensure that a command  intepreter is prepended to a command that
**          is recognised as not an .exe type for execution.
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	08-May-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008, clean up warnings.
*/

STATUS
PCcmdline(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    bool 	isOpenROAD;
    bool 	PCgetisOpenROAD_PM();
    STATUS 	PCdocmdline();
    STATUS 	PCcmdline_or();

    isOpenROAD = PCgetisOpenROAD_PM();
    if (!isOpenROAD )	
    {
        /*
        ** Someday the added functionality of appended output files may be part
        ** PCcmdline.
        ** Why are we waiting?
        */
        return PCdocmdline(interpreter,cmdline,wait,FALSE,err_log,err_code);
    }
    else
    {
	/* OpenROAD style */
        return PCcmdline_or(interpreter, cmdline, wait, err_log, err_code);
    }
}

/*****************************************************************************
**
**  Name: GetFilespec
**
**  Description:
**      Return the path and filename from the command line.
**      Assumes that the filespec is the first space delimited string.
**
**  Inputs:
**      pszCmdline      pointer to command line.
**      nFilespeclen    pointer indicating size of pszFileSpec buffer.
**
**  Outputs:
**      pszFilespec     updated with path and filename of application.
**      pszFile         points to the character after the last '\'.
**	pszArgs		update with remainder of command line
**      nFilespeclen    number of characters in pszFilespec.
**
**  Returns:
**      STATUS           success or failure indication
**
** Side Effects:
**	Application name is quoted if it contains blanks.
**
** History:
**      30-Oct-97 (fanra01)
**          Created.
**	15-Aug-2005 (drivi01)
**	    BUG 115044:
**	    Added a check (SHGetFileInfo) to GetFilespec after first 
**      SearchPath to verify that whatever path was found contains 
**		a valid windows executable s.a. .exe, .com, or .bat.
*****************************************************************************/
static
STATUS
GetFilespec(  
	     char *pszCmdline, 
	     char *pszFilespec, 
	     char **pszFile,
             char *pszArgs, 
	     i4   *nFilespecLen
)
{
    STATUS status= OK;
    i4  nMaxLen = *nFilespecLen;
    char*   pWork;
    char*   pszCopy;
    char*   pTmp;
    i4      nDelim  = 0;
    bool    blFoundblank = FALSE;
    char    tmppath[MAX_LOC+1];
    char    tmpfname[MAX_LOC+1];
    DWORD   dwSstat;
    char    *pszAppFile;
    bool    blFoundexe = FALSE;
    bool    blFirstblank = TRUE;
	SHFILEINFO	fileInfo;

    *pszFilespec = '\0';
    *pszFile = NULL;

    if ((pszCopy = STalloc(pszCmdline)) != NULL)
    {
        pWork = pszCopy;
        while (CMwhite(pWork))
            CMnext(pWork);

        pTmp = pWork;
        if ( *pTmp == '\"' )
        {
            CMnext(pWork);
            pTmp = STindex( pWork, "\"", 0 );
            if ( pTmp != NULL )
            {
                *pTmp = EOS;
                STcopy( pWork, tmppath );
                dwSstat = SearchPath( NULL, tmppath, NULL,
				      MAX_LOC, tmpfname, &pszAppFile );
				if ( dwSstat != 0 )
				{
					dwSstat = SHGetFileInfo(tmpfname, (DWORD)NULL, &fileInfo, sizeof(fileInfo), SHGFI_EXETYPE);
				}
                if ( dwSstat == 0 )
                {
                    dwSstat = SearchPath( NULL, tmppath, ".EXE",
			                  MAX_LOC, tmpfname, &pszAppFile );
                }
                *pTmp = '"';
                if ( dwSstat > 0 )
                {
                    CMnext( pTmp );
                    STpolycat( 3, "\"", tmpfname, "\"", pszFilespec );
		    STcopy( pTmp, pszArgs );
                    blFoundexe = TRUE;
                }
            }
        }
        while ( blFoundexe == FALSE && pTmp &&
		(pTmp = findspace( pTmp )) != NULL )
        {
	    char savedchar = *pTmp;
            *pTmp = EOS;
            STcopy( pWork, tmppath);
	    if ( !isshellcommand( tmppath ) )
	    {
                dwSstat = SearchPath( NULL, tmppath, NULL,
			              MAX_LOC, tmpfname, &pszAppFile );
				if ( dwSstat != 0 )
				{
					dwSstat = SHGetFileInfo(tmpfname, (DWORD)NULL, &fileInfo, sizeof(fileInfo), SHGFI_EXETYPE);
				}
                if ( dwSstat == 0 )
                {
                    dwSstat = SearchPath( NULL, tmppath, ".EXE",
			                  MAX_LOC, tmpfname, &pszAppFile );
                }
                if ( dwSstat > 0 )
                {
                    if ( findspace( tmpfname ) == NULL )
                    {
                        STcopy( tmpfname, pszFilespec );
                    }
                    else
                    {
                        STpolycat( 3, "\"", tmpfname, "\"", pszFilespec );
                    }
		    CMnext( pTmp );
                    STcopy( pTmp, pszArgs );
                    blFoundexe = TRUE;
                    break;
                }
	    }
            *pTmp = savedchar;
            CMnext( pTmp );
            blFirstblank = FALSE;
        }
        /* there were no blanks so try complete string */
        if ( blFoundexe == FALSE && blFirstblank == TRUE )
        {
            STcopy( pWork, tmppath);
	    if ( !isshellcommand( tmppath ) )
	    {
                dwSstat = SearchPath( NULL, tmppath, NULL,
                                      MAX_LOC, tmpfname, &pszAppFile );
                if ( dwSstat == 0 )
                {
                    dwSstat = SearchPath( NULL, tmppath, ".EXE",
                                          MAX_LOC, tmpfname, &pszAppFile );
                }
                if ( dwSstat > 0 )
                {
                    if ( findspace( tmpfname ) == NULL )
                    {
                        STcopy( tmpfname, pszFilespec );
                    }
                    else
                    {
                        STpolycat( 3, "\"", tmpfname, "\"", pszFilespec );
                    }
                    blFoundexe = TRUE;
	            *pszArgs = EOS;
		}
            }
        }
        if (blFoundexe == TRUE)
        {
            *nFilespecLen = (i4)STlength( pszFilespec );
            if ((*pszFile = STrindex(pszFilespec, "\\", 0)) == NULL)
            {
                *pszFile = STrindex(pszFilespec, ":", 0);
            }
            if (*pszFile != NULL)
            {
                CMnext(*pszFile);
            }
            else
            {
                *pszFile = pszFilespec;
            }
        }
        MEfree(pszCopy);
    }
    if ( blFoundexe != TRUE )
	status = FAIL;

    return (status);
}

/*****************************************************************************
**
** Name: PCdocmdline
**
**  Arguments:
**      LOCATION        *interpreter;
**      char            *cmdline;
**      i4              wait;
**      i4              append,
**      LOCATION        *err_log;
**      CL_ERR_DESC     *err_code;
**
**  Returns:
**      STATUS           success or failure indication
**
** History:
**	01-may-96 (emmag)
**	    On Windows '95 call the application directly, rather
**	    than calling the command interpreter.   This is because
**	    the '95 implementation of GC uses anonymous pipes and
**	    the pipe handles are not inheritable by children of the
**	    command interpreter.   This change will be backed out
**	    once named pipes are implemented on '95 so that we can
**	    comply with the CL spec on '95.
**	21-feb-1997 (canor01)
**	    Since we may be called from a DLL with no console and no
**	    inheritance on standard I/O handles, if PCcmdline() is redirecting
**	    its output, specify the standard handles in the STARTUPINFO
**	    structure.
**      30-Oct-97 (fanra01)
**          Updated to use test when an interpreter should be prepended.
**          Behavior defaults to inherit parent console and to set the
**          DETACHED_PROCESS flag if the I/O handles are redirected or
**          an output file is specified.
**      27-Jan-2001 (fanra01)
**          Update the setup of the new command line when a script or a non
**          .exe command is issued so that the command intepreter is
**          prepended to the command.
**
*****************************************************************************/
STATUS
PCdocmdline(
		LOCATION	*pLOinterp,
		char		*pszCmdline,
		i4              nWait,
		i4              nAppend,
		LOCATION	*pLOerrlog,
		CL_ERR_DESC	*pErrCode
)
{
    STATUS	status = OK;
    char*   pszAppSpec = NULL;
    char*   pszApp = NULL;
    char*   pszArglist = NULL;
    char    *pszNewcmdline = NULL;
    char*   pszInterp = NULL;
    char*   pszStdin  = NULL;
    char*   pszStdout = NULL;
    DWORD   dwCreateFlags;
    WORD    wWindowFlags;
    DWORD	wait_status = 0;
    i4 	nCmdlen;
    i4      nApplen;
    bool    blRedirected = FALSE;
    bool    isOpenROAD   = FALSE;
    bool    PCgetisOpenROAD_PM();
    bool    blComspec = FALSE;


    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES ProcessSec;
    SECURITY_ATTRIBUTES ThreadSec;
    HANDLE  SaveStdout, SaveStdin, SaveStderr;
    HANDLE 	newstdout = 0;
    HANDLE  newstdin = 0;
    HANDLE  newstderr = 0;
    STARTUPINFO     si;
    bool blGotConsole;
    HANDLE  hStdOut;

    /* 
    ** OpenROAD handles things differently
    */
    isOpenROAD = PCgetisOpenROAD_PM();
    if (isOpenROAD)
	return PCcmdline_or( pLOinterp, pszCmdline, nWait, pLOerrlog, pErrCode);

    GetStartupInfo (&si);

    nCmdlen = (pszCmdline) ? (i4)STlength(pszCmdline) + MAX_LOC : MAX_LOC;
    nApplen = nCmdlen;
    pszArglist = (char *) MEreqmem(0, nCmdlen, TRUE, &status);
    pszAppSpec = (char *) MEreqmem(0, nCmdlen, TRUE, &status);
    pszNewcmdline = (char *) MEreqmem(0, nCmdlen + MAX_LOC, TRUE, &status);

    if ( pszArglist == NULL || pszAppSpec == NULL || pszNewcmdline == NULL )
    {
        if ( pszArglist ) 
	    MEfree( pszArglist );
        if ( pszAppSpec ) 
	    MEfree( pszAppSpec );
	if ( pszNewcmdline )
	    MEfree( pszNewcmdline );
        SIprintf("PCdocmdlin: Bad Memory Allocation\n");
        return (PC_CM_MEM);
    }
    /*
    **  Setup for prepending interpreter. Are we starting a process?
    **  If yes, no cmd necessary.
    */
    if ( ( GetFilespec( pszCmdline, pszNewcmdline,
		        &pszApp, pszArglist, &nApplen ) != OK ) ||
         ( (STstrindex( pszApp, ".exe", 0, TRUE ) == NULL) &&
           (STstrindex( pszApp, ".EXE", 0, TRUE ) == NULL) ) )
    {
        /*
        ** Script extension or unknown command.  Attempt to find the command
        ** interpreter if not specified in the function call.
        */
        if (pLOinterp == NULL)
        {
            /*
            ** Check the ComSpec environment as NT interpreter is CMD.
            */
            if (pszInterp == NULL)
            {
                /*
                ** Get comspec environment.
                */
                pszInterp = getenv("ComSpec");
	        if ( pszInterp != NULL && *pszInterp != EOS )
	            blComspec = TRUE;
            }
            if (pszInterp == NULL)
            {
                /*
                ** No ComSpec get SHELL just in case using a different
                ** interpreter.
                */
                pszInterp = getenv("SHELL");
            }
        }
        else
        {
            /*
            ** Command interpreter specified
            */
            LOtos(pLOinterp, &pszInterp);
        }
        /*
        ** If comspec interpreter append a /c flag for interpreter to
        ** terminate after command executed.
        */
        if (blComspec == TRUE && pszCmdline != NULL)
        {
            sprintf( pszAppSpec, "%s /c %s",
                pszInterp ? pszInterp : "cmd.exe", pszNewcmdline );
        }
        else
        {
            /*
            ** If interpreter specified then use it otherwise as a last
            ** resort use a default of cmd.
            */
            sprintf( pszAppSpec, "%s %s",
                pszInterp ? pszInterp : "cmd.exe /c", pszNewcmdline );
        }
    }
    else
    {
        /*
        ** Command is recognised as an executable and has a valid path
        */
        strcpy( pszAppSpec, pszNewcmdline );
    }
    if (pszCmdline != NULL)
    {
	if ( pszApp == NULL )
            STcat( pszArglist, pszCmdline );

        if (nWait == PC_NO_WAIT)
        {
            si.lpTitle = pszApp;
        }
    }
    else
    {
        if (nWait == PC_NO_WAIT)
        {
            /* Generate a readable title for the created console... */
            si.lpTitle = pszApp ? pszApp : pszInterp;
        }
    }
    /*
    **  Determine what creation flags are required.
    **
    **  Assume that default behaviour is to use parents console.
    **
    **  Has our I/O been redirected.  If yes, we should run detached.
    */
    dwCreateFlags = NORMAL_PRIORITY_CLASS;
    wWindowFlags  = SW_HIDE;

    blGotConsole = ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE))
                        != INVALID_HANDLE_VALUE) ? TRUE : FALSE;

    if ((pLOerrlog != NULL) ||
        (nWait == PC_NO_WAIT) ||
        (blGotConsole == FALSE))
    {
        dwCreateFlags |= DETACHED_PROCESS;
    }

    /*
    ** Set up the STARTUPINFO structure for spawning process.
    */
    si.cb           = sizeof(si);
    si.lpReserved   = NULL;
    si.lpReserved2  = NULL;
    si.cbReserved2  = 0;
    si.lpDesktop    = NULL;
    si.dwX          = si.dwY = 20;
    si.dwXSize      = si.dwYSize = 40;
    si.dwFlags      = STARTF_USESHOWWINDOW;  /* Start app according to */
    si.wShowWindow  = wWindowFlags;  /* wShowWindow setting */

    /* Set up the secruity attribute structures. */

    ProcessSec.nLength = sizeof(ProcessSec);
    ProcessSec.lpSecurityDescriptor = NULL;
    ProcessSec.bInheritHandle = TRUE;

    ThreadSec.nLength = sizeof(ThreadSec);
    ThreadSec.lpSecurityDescriptor = NULL;
    ThreadSec.bInheritHandle = TRUE;

    /* See if redirection is requested. */
    pszStdin  = STrindex(pszArglist, "<", 0);
    pszStdout = STrindex(pszArglist, ">", 0);

    if ((pLOerrlog != NULL) || (pszStdin != NULL) || (pszStdout != NULL))
    {
        char*   cp = NULL;

        if (pszStdin != NULL)
        {
            *pszStdin = EOS;
            CMnext (pszStdin);
            while(CMwhite(pszStdin))
                CMnext(pszStdin);
        }

	/* Verify the validity of the log file name. */

        if (pLOerrlog != NULL)
        {
            LOtos(pLOerrlog, &cp);
        }
        if (pszStdout != NULL)
        {
            *pszStdout = EOS;
            CMnext (pszStdout);
            while(CMwhite(pszStdout))
                CMnext(pszStdout);
            if (cp == NULL)
                cp = pszStdout;
        }

	/* now do the redirection */

        /* Save our standard i/o handles. */
        DuplicateHandle ( GetCurrentProcess(),
                          GetStdHandle(STD_OUTPUT_HANDLE),
                          GetCurrentProcess(), &SaveStdout, 0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS);

        DuplicateHandle ( GetCurrentProcess(),
                          GetStdHandle(STD_INPUT_HANDLE),
                          GetCurrentProcess(), &SaveStdin, 0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS);

        DuplicateHandle ( GetCurrentProcess(),
                          GetStdHandle(STD_ERROR_HANDLE),
                          GetCurrentProcess(), &SaveStderr, 0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS);

        if (pszStdin != NULL)
        {
            newstdin =  CreateFile(  pszStdin, GENERIC_READ,
                                     FILE_SHARE_READ,
                                     &ProcessSec,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, NULL );

            if (newstdin == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                return(FAIL);
            }

            if (SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
                return(FAIL);
            }
        }
        if (cp != NULL)
        {
            newstdout = CreateFile ( cp, GENERIC_WRITE|GENERIC_READ,
                                     FILE_SHARE_WRITE|FILE_SHARE_READ,
                                     &ProcessSec,
                                     nAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL, NULL);

            if (newstdout == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                return(FAIL);
            }

            newstderr = CreateFile ( cp, GENERIC_WRITE,
                                     FILE_SHARE_WRITE|FILE_SHARE_READ,
                                     &ProcessSec, OPEN_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL, NULL);

            if (newstderr == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                return(FAIL);
            }

            if (SetStdHandle(STD_OUTPUT_HANDLE, newstdout) == FALSE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
                return(FAIL);
            }

            if (SetStdHandle(STD_ERROR_HANDLE, newstderr) == FALSE)
            {
                status = GetLastError();
                SETWIN32ERR(pErrCode, status, ER_create);
                SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
                return(FAIL);
            }

            fflush(stdout);
            fflush(stderr);
        }

        blRedirected = TRUE;

        SetFilePointer( newstdout, 0L, NULL, FILE_END );
        SetFilePointer( newstderr, 0L, NULL, FILE_END );

    }
    if ((blRedirected) || (dwCreateFlags & DETACHED_PROCESS))
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
        si.hStdError  = GetStdHandle (STD_ERROR_HANDLE);
    }

    /*
    ** Execute the command.
    */

    STpolycat( 3, pszAppSpec, " ", pszArglist, pszNewcmdline );
    status = CreateProcess( NULL, 
			    pszNewcmdline, &ProcessSec, &ThreadSec, 
			    TRUE, dwCreateFlags, NULL, NULL, &si, &pi);

    if (status != TRUE)
    {
          status = GetLastError();
          TerminateProcess(pi.hProcess, 0);
    }

    if (status == TRUE)
    {
        if (nWait == PC_WAIT)
        {
            wait_status = WaitForSingleObject(pi.hProcess, INFINITE);
            status =  (wait_status == WAIT_FAILED) ? GetLastError() :
                      GetExitCodeProcess(pi.hProcess, &wait_status);

            if (status != TRUE)
            {
                status=GetLastError();
            }
            else
            {
                /*
                **  If we started a command interpreter
                */
                if (pszInterp && *pszInterp != EOS)
                {
                    /*
                    **  Determining the exit code of a command passed to a
                    **  command interpreter could return an undocumented
                    **  value. Assume success in this case.
                    */
                    if (wait_status == 0x000000FF) 
			wait_status = 0;
                }
            }
        }
	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );

        if (status == TRUE) 
	    status = 0;
    }
										
    if (blRedirected)
    {
        if (newstdin != 0)
        {
            SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
            CloseHandle(newstdin);
        }
        if (newstdout != 0)
        {
            SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
            CloseHandle(newstdout);
        }
        if (newstderr != 0)
        {
            SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
            CloseHandle(newstderr);
        }
    }

    if ( pszArglist ) 
	MEfree( pszArglist );
    if ( pszAppSpec ) 
	MEfree( pszAppSpec );
    if ( pszNewcmdline )
	MEfree( pszNewcmdline );

    if (status || wait_status)
    {
	if (status)
	{
	    SETWIN32ERR(pErrCode, status, ER_close);
	    return (FAIL);
	}
	return ( wait_status );
    }
    return (OK);
}
/*****************************************************************************
**
**  PCdowindowcmdline
**
**  Description:
**      This function has been added to allow a command line to be started
**      from a different process.  The calling procedure can control the
**      properties of the display of the child process.
**
**  Arguments:
**      LOCATION        *interpreter;
**      char            *cmdline;
**      i4              wait;
**      i4              append,
**      i4              property;
**      LOCATION        *err_log;
**      CL_ERR_DESC     *err_code;
**
**  Results:
**      Start a process using the command interpreter.  If no interpreter is
**      supplied then the default is used.
**      The display window can be controlled using the properties flag.
**      If the interpreter argument points to an empty string a command
**      interpreter is not prepended to the command line.  Instead the
**      process is started with the OS default.
**
**  Side Effects:
**
**  History
**      23-Jul-95 (fanra01)
**          Created.
**	05-may-1997 (canor01)
**	    Move fix from PCdocmdline() to check process's exit code for
**	    failure as well as failure of CreateProcess().
**	09-oct-1998 (somsa01)
**	    If the child returns an error code (even though it was
**	    successfully spawned), return it (just like in PCdocmdline()).
**
*****************************************************************************/
STATUS
PCdowindowcmdline(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
i4 		append,
i4 		property,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
     char	*cp = NULL;
    DWORD   wait_status = 0;
    bool	stdout_redirected = 0;
    DWORD       createflags = 0;
    STATUS	status;
    char*   pszAppSpec = NULL;
    char*   pszApp = NULL;
    char*   pszArglist = NULL;
    char    *pszNewcmdline = NULL;
    char*   pszInterp = NULL;
    char*   pszStdin  = NULL;
    char*   pszStdout = NULL;
    i4 	nCmdlen;
    i4      nApplen;
    bool    blComspec = FALSE;

    PROCESS_INFORMATION ProcInfo;
    SECURITY_ATTRIBUTES handle_security;
    SECURITY_ATTRIBUTES child_security;
    HANDLE  SaveStdout, SaveStdin, SaveStderr;
    HANDLE 	newstdout, newstderr;
    STARTUPINFO   StartUp;

    /* Set up the STARTUPINFO structure for spawning the iidbms. */
    StartUp.cb = sizeof(StartUp);
    StartUp.lpReserved = NULL;
    StartUp.lpReserved2 = NULL;
    StartUp.cbReserved2 = 0;
    StartUp.lpDesktop = NULL;
    StartUp.lpTitle = NULL;
    StartUp.dwX = StartUp.dwY = 20;
    StartUp.dwXSize = StartUp.dwYSize = 40;
    StartUp.dwFlags = STARTF_USESHOWWINDOW;  /* Start app according to */
    StartUp.wShowWindow = showproperty[property];  /* wShowWindow setting */

    /* Set up the secruity attribute structures. */

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;

    child_security.nLength = sizeof(child_security);
    child_security.lpSecurityDescriptor = NULL;
    child_security.bInheritHandle = TRUE;

    nCmdlen = (cmdline) ? (i4)STlength(cmdline) + MAX_LOC : MAX_LOC;
    nApplen = nCmdlen;
    pszArglist = (char *) MEreqmem(0, nCmdlen, TRUE, &status);
    pszAppSpec = (char *) MEreqmem(0, nCmdlen, TRUE, &status);
    pszNewcmdline = (char *) MEreqmem(0, nCmdlen + MAX_LOC, TRUE, &status);

    if ( pszArglist == NULL || pszAppSpec == NULL || pszNewcmdline == NULL )
    {
        if ( pszArglist ) 
	    MEfree( pszArglist );
        if ( pszAppSpec ) 
	    MEfree( pszAppSpec );
	if ( pszNewcmdline )
	    MEfree( pszNewcmdline );
        SIprintf("PCdocmdlin: Bad Memory Allocation\n");
        return (PC_CM_MEM);
    }
    /*
    **  Setup for prepending interpreter. Are we starting a process?
    **  If yes, no cmd necessary.
    */
    if ( ( GetFilespec( cmdline, pszNewcmdline, &pszApp,
                        pszArglist, &nApplen ) != OK ) || 
         ( (STstrindex( pszApp, ".exe", 0, TRUE ) == NULL) &&
	   (STstrindex( pszApp, ".EXE", 0, TRUE ) == NULL) ) ) 
    { 
        if (interpreter == NULL)
        {
            if (pszInterp == NULL)
            {
               pszInterp = getenv("ComSpec");
	       if ( pszInterp != NULL && *pszInterp != EOS )
	           blComspec = TRUE;
            }
            if (pszInterp == NULL)
            {
               pszInterp = getenv("SHELL");
            }
        }
        else
        {
           LOtos(interpreter, &pszInterp);
        }
        /*
        ** If comspec interpreter append a /c flag for interpreter to
        ** terminate after command executed.
        */
        if (blComspec == TRUE && cmdline != NULL)
        {
            sprintf( pszAppSpec, "%s /c %s",
                pszInterp ? pszInterp : "cmd.exe", pszNewcmdline );
        }
        else
        {
            /*
            ** If interpreter specified then use it otherwise as a last
            ** resort use a default of cmd.
            */
            sprintf( pszAppSpec, "%s %s",
                pszInterp ? pszInterp : "cmd.exe /c", pszNewcmdline );
        }
    }
    else
    {
        /*
        ** Command is recognised as an executable and has a valid path
        */
        strcpy( pszAppSpec, pszNewcmdline );
    }
    if (cmdline != NULL)
    {
	if ( pszApp == NULL )
            STcat( pszArglist, cmdline );

        if (wait == PC_NO_WAIT)
        {
            StartUp.lpTitle = pszApp;
        }
    }
    else
    {
        if (wait == PC_NO_WAIT)
        {
            /* Generate a readable title for the created console... */
            StartUp.lpTitle = pszApp ? pszApp : pszInterp;
        }
    }

    /* See if redirection is requested. */
    if (err_log != NULL)
    {
   	char   *cp;
      STATUS	status;

	/* Verify the validity of the log file name. */

	LOtos(err_log, &cp);

	/* now do the redirection */
   	

      /* Save our standard i/o handles. */
      DuplicateHandle(GetCurrentProcess(),
         GetStdHandle(STD_OUTPUT_HANDLE),
         GetCurrentProcess(), &SaveStdout, 0,
         FALSE,
         DUPLICATE_SAME_ACCESS);

      DuplicateHandle(GetCurrentProcess(),
         GetStdHandle(STD_INPUT_HANDLE),
         GetCurrentProcess(), &SaveStdin, 0,
         FALSE,
         DUPLICATE_SAME_ACCESS);

      DuplicateHandle(GetCurrentProcess(),
         GetStdHandle(STD_ERROR_HANDLE),
         GetCurrentProcess(), &SaveStderr, 0,
         FALSE,
         DUPLICATE_SAME_ACCESS);

      newstdout = CreateFile(cp, GENERIC_WRITE|GENERIC_READ,
         FILE_SHARE_WRITE|FILE_SHARE_READ,
         &handle_security, append ? OPEN_ALWAYS : CREATE_ALWAYS,
         FILE_ATTRIBUTE_NORMAL, NULL);

      if (newstdout == INVALID_HANDLE_VALUE)
      {
         status = GetLastError();
         SETWIN32ERR(err_code, status, ER_create);
         return(FAIL);
      }

      newstderr = CreateFile(cp, GENERIC_WRITE,
         FILE_SHARE_WRITE|FILE_SHARE_READ,
         &handle_security, OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL, NULL);

      if (newstderr == INVALID_HANDLE_VALUE)
      {
         status = GetLastError();
         SETWIN32ERR(err_code, status, ER_create);
         return(FAIL);
      }

      if (SetStdHandle(STD_OUTPUT_HANDLE, newstdout) == FALSE)
      {
         status = GetLastError();
         SETWIN32ERR(err_code, status, ER_create);
         SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
         return(FAIL);
      }

      if (SetStdHandle(STD_ERROR_HANDLE, newstderr) == FALSE)
      {
         status = GetLastError();
         SETWIN32ERR(err_code, status, ER_create);
         SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
         return(FAIL);
      }

   	stdout_redirected = 1;

        SetFilePointer( newstdout, 0L, NULL, FILE_END );
        SetFilePointer( newstderr, 0L, NULL, FILE_END );

        StartUp.dwFlags |= STARTF_USESTDHANDLES;
        StartUp.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
        StartUp.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
        StartUp.hStdError  = GetStdHandle (STD_ERROR_HANDLE);
    }
    else
	   stdout_redirected = 0;

    /*
    ** Execute the command.
    */

    if (wait == PC_WAIT)
    {
        createflags |= NORMAL_PRIORITY_CLASS;
    }
    else
    {
        createflags |= (NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE);
    }
    STpolycat( 3, pszAppSpec, " ", pszArglist, pszNewcmdline );
    status = CreateProcess( NULL, 
			    pszNewcmdline,
			    &child_security, &child_security, TRUE,
			    createflags,
			    NULL, NULL,
			    &StartUp, &ProcInfo);

    if (status != TRUE)
    {
          status = GetLastError();
          TerminateProcess(ProcInfo.hProcess, 0);
    }

    if (status == TRUE)
    {
       if (wait == PC_WAIT)
       {
         wait_status = WaitForSingleObject(ProcInfo.hProcess, INFINITE);
         status = GetExitCodeProcess(ProcInfo.hProcess, &wait_status);
         if (status != TRUE)
         {
            status=GetLastError();
         }
       }
       CloseHandle(ProcInfo.hThread);
       CloseHandle(ProcInfo.hProcess);

       if (status == TRUE) status = 0;
    }
										
    if(stdout_redirected)
    {
       SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
       SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
       CloseHandle(newstdout);
       CloseHandle(newstderr);
    }

    if ( pszArglist ) 
	MEfree( pszArglist );
    if ( pszAppSpec ) 
	MEfree( pszAppSpec );
    if ( pszNewcmdline )
	MEfree( pszNewcmdline );

    if(status || wait_status)
    {
	if (status)
	{
	    SETWIN32ERR(err_code, status, ER_close);
	    return (FAIL);
	}
	return ( wait_status );
    }
    return OK;
}

static	void	pumpMsgLoop(void);

/*
**  History:
**
**	20-feb-97 (fraser)
**	    Removed test for Win32s because we don't support Win32s now.  Also removed
**	    the function PC32scmdline which was used for Win32s.
*/

STATUS
PCcmdline_or(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    /*
    ** Someday the added functionality of appended output files may be part
    ** PCcmdline.
    */

	return PCwntcmdline(interpreter,cmdline,wait,FALSE,err_log,err_code);
}

/* Save the Thunk entry point here */

UT32PROC pfnUTProc = 0;		/* THIS MUST NOT BE A GLOBALDEF! */

VOID
PCsave_pfnUTProc(UT32PROC pfn)
{
	pfnUTProc = pfn;
}

UT32PROC
PCget_pfnUTProc()
{
	return(pfnUTProc);
}



/* PCcmdline version for Win/NT */

STATUS
PCwntcmdline(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
i4              append,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    char	arglist[1024];
    char	comspec[MAX_LOC];
    char	*argptr;
	char	*cmdonly = NULL;
    char	*cp;
    bool	stdout_redirected = 0;
    int		savestdout;	/* file dscriptor for current stdout */
    int		savestderr;	/* file descriptor for current stderr */

    ULONG 	status;

    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO   StartUp;

    /* rionc A07 w4gl2-3200072
    ** startup info needs to be initialized
    */
    StartUp.cb = sizeof(StartUp);
    StartUp.lpReserved = NULL;
    StartUp.lpDesktop = NULL;
    StartUp.dwFlags = (DWORD)NULL;
    StartUp.cbReserved2 = (WORD)NULL;
    StartUp.lpReserved2 = NULL;
    StartUp.lpTitle = NULL;

    /* get location and name of command interpreter (usually cmd.exe) */

   	if ((interpreter == NULL) || (cmdline != NULL))
   	{
	   cp = getenv("ComSpec");
	   strcpy(comspec, cp ? cp : "cmd.exe");
 	   if ((cmdline == NULL) ||
		   (strlen(cmdline) == 0)) /* (trevor) fix 79782- no interpreter & no cmdline */
		   cmdonly = comspec;
  	}
   	else
   	{
	   cp = comspec;
	   LOtos(interpreter, &cp); 
 	   /* (trevor)fix 79782 - interpreter and no cmdline */
       cmdonly = comspec;
  	}
   	strcpy(arglist, comspec);
   	argptr = arglist+strlen(arglist);

   	/*
   	** If we have a command line, we must use it as an
   	** argument to the command interpreter.
   	*/

    if (cmdline != NULL)
    {
	   /* tell command.com a command line follows */
	   memcpy(argptr, " /c ", 4);
	   argptr += 4;
	   strcpy(argptr, cmdline);
	   argptr += strlen(cmdline)+1;
    }


    /* See if redirection is requested. */
    if (err_log != NULL)
    {
   	char   *cp;
   	FILE 	*newstdout, *newstderr;

   	LOtos(err_log, &cp);
   	/* now do the redirection */

	/* Save stdout and stderr. */
	savestdout = dup(fileno(stdout));
	savestderr = dup(fileno(stderr));

	newstdout = freopen(cp, append ? "a" : "w", stdout);
	if (newstdout == NULL)
	{
          SETCLERR(err_code, 0,0);
          dup2(savestdout, fileno(stdout));
          return FAIL;
   	}

	newstderr = freopen(cp, "a", stderr);
	if (newstderr == NULL)
	{
          SETCLERR(err_code, 0,0);
          dup2(savestdout, fileno(stderr));
          return FAIL;
   	}

   	SIflush(stdout);
   	SIflush(stderr);
   	stdout_redirected = 1;
    }
    else
	   stdout_redirected = 0;

    /*
    ** Execute the command.
    */
	if (cmdonly) /*(trevor) 79782 - put up a command window */
		status = CreateProcess(/*comspec*/NULL, 
    						cmdonly, 
    						(LPSECURITY_ATTRIBUTES) NULL,
    						(LPSECURITY_ATTRIBUTES) NULL,
						TRUE, 0, 
    						(LPVOID) NULL, 
    						(LPSTR) NULL,
    						&StartUp, 
    						&ProcInfo);
	else
	{
	/*(trevor) try to execute directly without "cmd.exe" and only use
	           cmd if we get 'file not found'. This will retain
			   previous behaviour for internal commands and batch
			   files - fixes 79166
	*/
       status = CreateProcess(NULL, 
    			      cmdline, 
    			      (LPSECURITY_ATTRIBUTES) NULL,
    			      (LPSECURITY_ATTRIBUTES) NULL, TRUE, 0, 
    			      (LPVOID) NULL, 
    			      (LPSTR) NULL,
    			      &StartUp, 
    			      &ProcInfo);

       if (!status)
       {
    	    status = GetLastError();
            if (status == ERROR_FILE_NOT_FOUND)
		        status = CreateProcess(NULL, 
    						arglist, 
    						(LPSECURITY_ATTRIBUTES) NULL,
    						(LPSECURITY_ATTRIBUTES) NULL, 0, 0, 
    						(LPVOID) NULL, 
    						(LPSTR) NULL,
    						&StartUp, 
    						&ProcInfo);
       }
	}


    if (!status)
    {
	   	status = GetLastError();
    }
    else
    {
    	if (wait & PC_WAIT)
    	{
			do
			{
				pumpMsgLoop();
	    		Sleep(500);
				WaitForSingleObject(ProcInfo.hProcess, (DWORD)NULL);
			} while (GetExitCodeProcess(ProcInfo.hProcess, &status) 
				  && (status == STILL_ACTIVE));
		}
		else
		{
			pumpMsgLoop();
			status = 0;
		}
		CloseHandle(ProcInfo.hThread);
		CloseHandle(ProcInfo.hProcess);
    }
									
    if (stdout_redirected)
    {
       dup2(savestdout, fileno(stdout));
       dup2(savestderr, fileno(stderr));
    }

    if (status)
    {
	   SETCLOS2ERR(err_code, status, 0);
	   return FAIL;
    }
    return OK;
}

/*
**  History:
**
**	27-mar-97 (trevor)
**		Add WM_PAINT to PeekMessage so we can redraw our windows (80271)
**
*/

static
void
pumpMsgLoop(void)
{
	MSG	msg;

	while (PeekMessage((LPMSG) &msg,NULL,WM_ACTIVATE,WM_ACTIVATE,PM_REMOVE)
		|| PeekMessage((LPMSG) &msg,NULL,WM_ACTIVATEAPP,WM_ACTIVATEAPP,PM_REMOVE)
		|| PeekMessage((LPMSG) &msg,NULL,WM_PAINT,WM_PAINT,PM_REMOVE))
	{
		TranslateMessage((LPMSG) &msg);
		DispatchMessage((LPMSG) &msg);
	}
}

static 
char *
findspace( char *string )
{
    char *found = NULL;

    while ( *string != EOS )
    {
	if ( CMspace( string ) )
	{
	    found = string;
	    break;
	}
	CMnext( string );
    }
    return (found);
}

static 
bool 
isshellcommand( char *command )
{
    int i;
    bool retval = FALSE;

    for ( i = 0; shellcommand[i] != NULL; i++ ) 
    {
        if ( STbcompare( command, 0, shellcommand[i], 0, TRUE ) == OK )
	{
	    retval = TRUE;
	    break;
	}
    }
    return (retval);
}
