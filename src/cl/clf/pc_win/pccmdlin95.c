/*****************************************************************************
**
** Copyright (c) 1997 Ingres Corporation 
** All rights reserved.
**
*****************************************************************************/
#define 	W32SUT_32
#include       <stdio.h>
#include       <conio.h>
#include       <io.h>

#include       <compat.h>
#include       <lo.h>
#include       <er.h>
#include       <pc.h>
#include       <me.h>
#include       <nm.h>
#include       <st.h>
#include       <si.h>
#include       <iiwin32s.h>
#include       <clconfig.h>
#include       <cm.h>
#include	"pclocal.h"

#ifndef _INC_STRING
#include <string.h>
#endif

/*
**      File: pccmdlin95.c 
**
**      Description: Windows 95 version of pccmdlin.c. This version allows 
**                   any application to execute on a Win95 system with the
**                   Win95 command interpreter.
**      History:
**          18-jun-97 (harpa06)
**              Created.
**
**              Lifted from pccmdlin.c. Modified to include command line
**              interpeter. Added "95" to the end of each function name.
**              Original history of pccmdlin.c is left intact.
**	    14-oct-98 (mcgem01)
**		ComSpec should override any value set for SHELL.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

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

/*
**      Function:
**              PCcmdline95
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
**	08-May-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008, clean up warnings.
*/


STATUS	PCw95cmdline(LOCATION *interpreter, char *cmdline,
		i4 wait,i4 append, LOCATION *err_log, CL_ERR_DESC *err_code);

STATUS
PCcmdline95(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    bool 	isOpenROAD;
    bool 	PCgetisOpenROAD_PM();
    STATUS 	PCdocmdline95();
    STATUS 	PCcmdline_or95();

    isOpenROAD = PCgetisOpenROAD_PM();
    if (!isOpenROAD )	
    {
        /*
        ** Someday the added functionality of appended output files may be part
        ** PCcmdline95.
        ** Why are we waiting?
        */
        return PCdocmdline95(interpreter,cmdline,wait,FALSE,err_log,err_code);
    }
    else
    {
	/* OpenROAD style */
        PCcmdline_or95(interpreter, cmdline, wait, err_log, err_code);
    }
}

/*****************************************************************************
**
** Name: PCdocmdline95
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
**      18-jun-97 (harpa06)
**          Yanked test for Win95 since this is the Win95 version of
**          PCdocmdline(). Made reference to "command.com" instead of "cmd.exe"
**
**          Added support for redirecting stdin.
**
*****************************************************************************/
STATUS
PCdocmdline95(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
i4 		append,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    char	*arglist;
    char        *input_file = NULL;
    char	comspec[MAX_LOC];
    char	*cp = NULL;
    DWORD	wait_status = 0;
    bool	stdout_redirected = 0;
    bool        stdin_redirected = 0;
    char hold[127];
    char *phold;
    OSVERSIONINFO lpvers;
    int		cmdlen;

    STATUS	status;

    PROCESS_INFORMATION ProcInfo;
    SECURITY_ATTRIBUTES handle_security;
    SECURITY_ATTRIBUTES child_security;
    HANDLE  SaveStdout, SaveStdin, SaveStderr;
    HANDLE 	newstdin, newstdout, newstderr;
    STARTUPINFO   StartUp;
    StartUp.cb = sizeof(StartUp);

    if (cmdline)
        cmdlen = STlength(cmdline) + MAX_LOC;
    else
	cmdlen = MAX_LOC;

    if((arglist = (char *)MEreqmem(0,cmdlen,TRUE,NULL)) == NULL)
    {
        SIprintf("PCdocmdlin95: Bad Memory Allocation\n");
        return (FAIL);
    }

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
    StartUp.wShowWindow = SW_SHOWMINIMIZED;  /* wShowWindow setting */

    /* Set up the secruity attribute structures. */

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;

    child_security.nLength = sizeof(child_security);
    child_security.lpSecurityDescriptor = NULL;
    child_security.bInheritHandle = TRUE;

    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);

    /* get location and name of command interpreter (usually command.com) */

    if (interpreter == NULL)
    {
      if (cmdline == NULL)
      {
         cp = getenv("SHELL");
      }
      if (cp == NULL)
      {
         cp = getenv("COMSPEC");
      }
      strcpy(comspec, cp ? cp : "command.com");
    }
    else
    {
       cp = comspec;
       LOtos(interpreter, &cp);
    }

    if (wait == PC_NO_WAIT)
    {
      StartUp.lpTitle = cmdline;
    }

    STcopy (comspec, arglist);

    /*
    ** If we have a command line, we must use it as an
    ** argument to the command interpreter.
    */
    if (cmdline != NULL)
    {
	   /* tell command.com a command line follows */
      STcat (arglist, " /c ");

      
      STcat (arglist, cmdline);
      if (wait == PC_NO_WAIT)
      {
         /* Generate a readable title for the created console... */
         sscanf(cmdline,"%s", hold);
         phold = STrindex(cmdline, "\\", 0);
         if (phold == NULL)
         {
            StartUp.lpTitle = hold;
         }
         else
         {
            phold++;
            StartUp.lpTitle = phold;
         }
      }
    }
    else
    {
      if (wait == PC_NO_WAIT)
      {
         /* Generate a readable title for the created console... */
         StartUp.lpTitle = comspec;
      }
    }

    /* Save our standard i/o handles. */
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE),
                    GetCurrentProcess(), &SaveStdout, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
                    GetCurrentProcess(), &SaveStdin, 0, FALSE, 
                    DUPLICATE_SAME_ACCESS);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE),
                    GetCurrentProcess(), &SaveStderr, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);

   
    /* See if stdin redirection is requested */
    if (input_file = STstrindex (arglist, ERx ("<"), 0, TRUE))
    {
        *input_file = EOS;
        CMnext (input_file);
        while (CMspace (input_file))
            CMnext (input_file);

        newstdin = CreateFile (input_file, GENERIC_READ, FILE_SHARE_READ,
                               &handle_security, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);

        if (newstdin == INVALID_HANDLE_VALUE)
        {
            status = GetLastError();
            SETWIN32ERR(err_code, status, ER_create);
            return(FAIL);
        }


        if (SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE)
        {
            status = GetLastError();
            SETWIN32ERR(err_code, status, ER_create);
            SetStdHandle(STD_ERROR_HANDLE, SaveStdin);
            return(FAIL);
        }

        stdin_redirected = 1;
    }


    /* See if stdout redirection is requested. */
    if (err_log != NULL)
    {
   	char   *cp;
        STATUS	status;

	/* Verify the validity of the log file name. */

	LOtos(err_log, &cp);

	/* now do the redirection */
        newstdout = CreateFile(cp, GENERIC_WRITE|GENERIC_READ,
                               FILE_SHARE_WRITE|FILE_SHARE_READ,
                               &handle_security, 
                               append ? OPEN_ALWAYS : CREATE_ALWAYS,
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

   	fflush(stdout);
   	fflush(stderr);
        stdout_redirected = 1;

    }

    StartUp.dwFlags |= STARTF_USESTDHANDLES;  
    StartUp.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    StartUp.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    StartUp.hStdError  = GetStdHandle (STD_ERROR_HANDLE);


    /*
    ** Execute the command.
    */

    status = CreateProcess( NULL, arglist, &child_security, &child_security, 
                            TRUE, 
             (wait == PC_WAIT) ? NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
            NULL, NULL, &StartUp, &ProcInfo);

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

       if (status == TRUE) status = 0;
    }
										
    if(stdout_redirected)
    {
       SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
       SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
       CloseHandle(newstdout);
       CloseHandle(newstderr);
    }

    if (stdin_redirected)
    {
       SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
       CloseHandle(newstdin);
    }

    MEfree (arglist);
    if(status || wait_status)
    {
	if (status)
	   SETWIN32ERR(err_code, status, ER_close);
	return FAIL;
    }
    return OK;
}

/*****************************************************************************
**
**  PCdowindowcmdline95
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
**
*****************************************************************************/
STATUS
PCdowindowcmdline95(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
i4 		append,
i4 		property,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    char	*arglist;
    char	comspec[MAX_LOC];
    char	*cp = NULL;
    DWORD   wait_status;
    bool	stdout_redirected = 0;
    char hold[127];
    char *phold;
    DWORD       createflags = 0;
    STATUS	status;
    i4 		cmdlen;

    PROCESS_INFORMATION ProcInfo;
    SECURITY_ATTRIBUTES handle_security;
    SECURITY_ATTRIBUTES child_security;
    HANDLE  SaveStdout, SaveStdin, SaveStderr;
    HANDLE 	newstdout, newstderr;
    STARTUPINFO   StartUp;
    StartUp.cb = sizeof(StartUp);


    if (cmdline)
        cmdlen = STlength(cmdline) + MAX_LOC;
    else
	cmdlen = MAX_LOC;

    if((arglist = (char *)MEreqmem(0,cmdlen,TRUE,NULL)) == NULL)
    {
        SIprintf("PCdowindowcmdline95: Bad Memory Allocation\n");
        return (FAIL);
    }

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

    /* get location and name of command interpreter (usually command.com) */

    if (interpreter == NULL)
    {
      if (cp == NULL)
      {
         cp = getenv("ComSpec");
      }
      if (cmdline == NULL)
      {
         cp = getenv("SHELL");
      }
      strcpy(comspec, cp ? cp : "command.com");
    }
    else
    {
        cp = comspec;
        LOtos(interpreter, &cp);
        if(*cp == '\0')
        {
            createflags |= CREATE_NEW_CONSOLE;
        }
    }

    if (wait == PC_NO_WAIT)
    {
      StartUp.lpTitle = cmdline;
    }

    strcpy(arglist, comspec);

    /*
    ** If we have a command line, we must use it as an
    ** argument to the command interpreter.
    */
    if (cmdline != NULL)
    {
	   /* tell command.com a command line follows */
      if(*cp)
      {                     /* if there is no interpreter no need for option */
          strcat(arglist, " /c ");
      }
      strcat(arglist, cmdline);
      if (wait == PC_NO_WAIT)
      {
         /* Generate a readable title for the created console... */
         sscanf(cmdline,"%s", hold);
         phold = STrindex(cmdline, "\\", 0);
         if (phold == NULL)
         {
            StartUp.lpTitle = hold;
         }
         else
         {
            phold++;
            StartUp.lpTitle = phold;
         }
      }
    }
    else
    {
      if (wait == PC_NO_WAIT)
      {
         /* Generate a readable title for the created console... */
         StartUp.lpTitle = comspec;
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

   	fflush(stdout);
   	fflush(stderr);
   	stdout_redirected = 1;
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
    status = CreateProcess( NULL, arglist,
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

       if (status == TRUE) status = 0;
    }
										
    if(stdout_redirected)
    {
       SetStdHandle(STD_OUTPUT_HANDLE, SaveStdout);
       SetStdHandle(STD_ERROR_HANDLE, SaveStderr);
       CloseHandle(newstdout);
       CloseHandle(newstderr);
    }

    MEfree (arglist);
    if(status || wait_status)
    {
	if (status)
	   SETWIN32ERR(err_code, status, ER_close);
	return FAIL;
    }
    return OK;
}

static	void	pumpMsgLoop95(void);

/*
**  History:
**
**	20-feb-97 (fraser)
**	    Removed test for Win32s because we don't support Win32s now.  Also removed
**	    the function PC32scmdline which was used for Win32s.
*/

STATUS
PCcmdline_or95(
LOCATION *      interpreter,
char *          cmdline,
i4              wait,
LOCATION *      err_log,
CL_ERR_DESC *   err_code)
{
    /*
    ** Someday the added functionality of appended output files may be part
    ** PCcmdline95.
    */

	return PCw95cmdline(interpreter,cmdline,wait,FALSE,err_log,err_code);
}

/* Save the Thunk entry point here */

UT32PROC pfnUTProc95 = 0;	/* THIS MUST NOT BE A GLOBALDEF! */

VOID
PCsave_pfnUTProc95(UT32PROC pfn)
{
	pfnUTProc95 = pfn;
}

UT32PROC
PCget_pfnUTProc95()
{
	return(pfnUTProc95);
}



/* PCcmdline version for Win/95 */

STATUS
PCw95cmdline(
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

    /* get location and name of command interpreter (usually command.com) */

   	if ((interpreter == NULL) || (cmdline != NULL))
   	{
	   cp = getenv("ComSpec");
	   strcpy(comspec, cp ? cp : "command.com");
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
    						(LPSECURITY_ATTRIBUTES) NULL, 0, 0, 
    						(LPVOID) NULL, 
    						(LPSTR) NULL,
    						&StartUp, 
    						&ProcInfo);
	else
	{
	/* try to execute directly without "command.com" and only use
	   command if we get 'file not found'. This will retain
	   previous behaviour for internal commands and batch files
	*/
       status = CreateProcess(NULL, 
    			      cmdline, 
    			      (LPSECURITY_ATTRIBUTES) NULL,
    			      (LPSECURITY_ATTRIBUTES) NULL, 0, 0, 
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
				pumpMsgLoop95();
	    		Sleep(500);
				WaitForSingleObject(ProcInfo.hProcess, (DWORD)NULL);
			} while (GetExitCodeProcess(ProcInfo.hProcess, &status) 
				  && (status == STILL_ACTIVE));
		}
		else
		{
			pumpMsgLoop95();
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
pumpMsgLoop95(void)
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
