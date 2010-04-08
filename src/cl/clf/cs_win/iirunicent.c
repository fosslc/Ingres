/*
** Copyright (c) 1992, 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <ep.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <pc.h>
# include <pm.h>
# include <si.h>
# include <st.h>


static void error(char *, char *);
static void get_sys_dependencies( void );

#define COMPARE_LENGTH 9 /*  length of i/ingres/ */
#ifndef ERROR_ELEVATION_REQUIRED
#define	ERROR_ELEVATION_REQUIRED	740
#endif
/*
** Name: iirunicent.c -- Start an ice server.
**
** Description:
**	Starts an icesvr. Resolves the full pathname of the executable by
**      getting the image name from config.dat file.
**	Create a anonymous pipe and change stdout to point to the write
**	end of the pipe. Start program iirun which will start the server.
**	Issue a read against the pipe which should return the startup message
**	from the server. If the server did not startup correctly, give the
**	user some advice.
**	NOTE - the executable name will be iirunice so the output
**		when an error occurs uses this name instead of iirunicent.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	Type of server to start, [dbms,star,gcn,gcc,
**	???
**	Any parameters that should be passed to the server for it's startup.
**		
** Outputs:
**	The message from the server and if the server fails to start, some
**	advice.	
**
** Returns:
**	OK
**	FAIL
**
** History:
**      18-Sep-98 (fanra01)
**          Created base on iirundbmsnt.c
**      01-Oct-98 (fanra01)
**          Fix default image name to be icesvr.
**      10-Jun-1999 (fanra01)
**          Set both stdout and stderr handles of iirun to use the pipe handle.
**          The read should not fail if iirun fails to start icesvr.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	04-sep-2000 (somsa01)
**	    Added code to allow the printing of the server id.
**	26-sep-2001 (somsa01)
**	    Use "STstrindex" to check for the "PASS" or "FAIL" strings,
**	    as we could read this message from the pipe as part of
**	    another message which is printable to the end-user.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	25-Jul-2007 (drivi01)
**	    Added additional error handling for Vista OS.
**	    This program will recognize ERROR_ELEVATION_REQUIRED
**	    and ERROR_ACCESS_DENIED errors as Vista's lack of
**	    privileges.
*/

/*
**      mkming hints

NEEDLIBS =      COMPATLIB 

PROGRAM =       iirunice

*/

char	bad_msg[GL_MAXNAME] 	= ERx("\nFAIL\n");
char	iirun[GL_MAXNAME] 	= ERx("iirun.exe");
char	iirundbms[GL_MAXNAME] 	= ERx("iirundbms");


main(int argc, char **argv)
{
	char 			*value = NULL; 
	char			*host, *server_type, *command_line; 
	char 			*server_location, *env, *arguments;
	u_i4			command_line_length;
	char 			config_string[256];
	char 			icesvr[256];
	STARTUPINFO 		si;
	PROCESS_INFORMATION 	ProcessInfo;
	BOOL 			Status;
	SECURITY_ATTRIBUTES     sa;
	HANDLE			hRead, hWrite;
	HANDLE			hStderr;
	HANDLE			hStdout;
	char			buffer[512];
	DWORD			bufferl = sizeof(buffer);
	DWORD			pipe_size = 0;	/* Use default buffer size */
	DWORD			datal = 0;
	bool			havequote, failed = FALSE;


	MEadvise( ME_INGRES_ALLOC );

	switch (argc)
	{

	case 1:
		server_type = ERx("icesvr");
		server_location = ERx("*");
		break;

	case 2:
		server_type = argv[1];
		server_location = ERx("*");
		break;

	default:
		server_type = argv[1];
                server_location = argv[2];
		break;

	}
	
	get_sys_dependencies();

	/*
	**	Get the host name, formally used iipmhost.
	*/
	host = PMhost();

	/*
	**	Build the string we will search for in the config.dat file.
	*/
	STprintf( config_string,
		  ERx("%s.%s.%s.%s.image_name"),
		  SystemCfgPrefix, host, server_type, server_location );

	/*
	**	Get set up for the PMget call.
	*/
	PMinit();
	if( PMload( NULL, (PM_ERR_FUNC *)NULL ) != OK )
		PCexit( FAIL );

	/*
	**	Go search config.dat for a match on the string we just built.
	*/
	PMget( config_string, &value );

	if ( value == NULL )
	{
		NMgtAt( SystemLocationVariable, &env );
		STprintf( icesvr, ERx("%s\\%s\\bin\\icesvr"),
			  env, SystemLocationSubdirectory );
	}
	else if ( *value == '/' || 
		  *value == '\\' ||
		  (*(value+1) == ':' && *(value+2) == '\\') )
	{
		/* Must be a qualified path name */
		STcopy( value, icesvr );
	}
	else 
	{
		NMgtAt( SystemLocationVariable, &env );
		STprintf( icesvr, ERx("%s\\%s\\bin\\%s"),
			  env, SystemLocationSubdirectory, value );
	}
	
	/*
 	** 	Initialize the startinfo structure.	
	*/
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	/*
	**	Get the original command line.
	*/
	arguments = GetCommandLine();
        havequote = FALSE;
        while (*arguments != '\0' && *arguments != '\n')
        {
            if ( *arguments == '"' )
                havequote = (havequote) ? FALSE : TRUE;
            if ( CMwhite(arguments) && havequote == FALSE )
                break;
            arguments++;
        }

	/*
	** 	Put together a command line to create a process with.
	**      - 4 blank separators, quotes, null termination 
	*/
	command_line_length = STlength(arguments) + STlength(icesvr) +
				STlength(iirun) + 6 + 1; 
	if((command_line = (char *)
		MEreqmem(0, command_line_length, TRUE, NULL)) == NULL)
	{
		error(ERx("Request for memory failed"),NULL);
	}

	STprintf( command_line, "%s \"%s\" %s", iirun, icesvr, arguments );

	/*
	**	Save standard out's handle, to be restored later.
	*/
	hStderr = GetStdHandle(STD_ERROR_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

 	/*
	**      Initialize the security attributes structure that will
	**      be used to make the pipe handles inheritable.
	*/
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;       /* Make object inheritable */

	/*
	**	Define a anonymous pipe to catch the server's startup message.
	*/
	if((Status = CreatePipe(&hRead,&hWrite,&sa,pipe_size)) != TRUE)
	{
		error(ERx("CreatePipe failed"),ERx("error code = "));
	}

	SetStdHandle(STD_ERROR_HANDLE,hWrite);
	SetStdHandle(STD_OUTPUT_HANDLE,hWrite);

 	/*
	**      Initialize the security attributes structure that will
	**      be used to make the pipe handles inheritable.
	*/
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;       /* Make object inheritable */

	/*
	**	Initialize the startup information structure.
	*/
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	/* 
	**	Start it up.
	*/
	/*
	**	Start iirun which will start the server.
	*/
	if ((Status = CreateProcess(NULL,command_line,&sa,NULL,TRUE,
		HIGH_PRIORITY_CLASS, NULL,
	     NULL,&si,&ProcessInfo)) != TRUE)
	{
		DWORD dwError = GetLastError();
		switch(dwError)
		{
		     case ERROR_ACCESS_DENIED:
			error(ERROR_REQ_PRIVILEGE, ERx(""));
			break;
		     case ERROR_ELEVATION_REQUIRED:
			error(ERROR_DENIED_PRIVILEGE, ERROR_REQ_ELEVATION);
			break;
		     default:				
			error(ERx("CreateProcess failed"),ERx("error code = "));
			break;
		}
	}

	SetStdHandle(STD_ERROR_HANDLE,hStderr);
	SetStdHandle(STD_OUTPUT_HANDLE,hStdout);

	for (;;)
	{
	    char	*tmpptr;

	    if((Status = ReadFile(hRead,&buffer,bufferl,&datal,NULL)) != TRUE)
	    {
		error(ERx("ReadFile failed"),ERx("error code = "));
	    }

	    buffer[datal] = '\0';
	    if ((tmpptr = STstrindex(buffer, "PASS\n", 0, FALSE)))
	    {
		*tmpptr = '\0';
		if (STlength(buffer))
		    SIfprintf(stdout, "%s", buffer);

		break;
	    }

	    SIfprintf(stdout, "%s", buffer);

	    if ((tmpptr = STstrindex(buffer, bad_msg, 0, FALSE)))
	    {
		*tmpptr = '\0';
		if (STlength(buffer))
		    SIfprintf(stdout, "%s", buffer);

		failed = TRUE;
		break;
	    }
	}

	/*
	**	Close handles since we don't need them anymore.
	*/
	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);
	CloseHandle(hRead);
	CloseHandle(hWrite);

	if (failed)
	{
		SIfprintf( stderr,"\n%s: server would not start.\n",
			   iirundbms );

		SIfprintf(stderr,
		" %s must be set in your environment.\n", 
		    SystemLocationVariable );

		SIfprintf(stderr,
		" Check the file '%%%s%%\\%s\\files\\errlog.log'\n",
		    SystemLocationVariable, SystemLocationSubdirectory );

		SIfprintf(stderr,
		" for more details concerning internal errors.\n");

		SIfprintf(stderr,
		" See your Installation and Operation Guide for more\n");

		SIfprintf(stderr,
		" information concerning server startup.\n");

		PCexit(FAIL);
	}
	else
	{
            PCexit(OK);
	}
}
/*
** Name: error
**
** Description:
**	Prints an error message to stderr. If two error strings are passed
**	to the function, the function will make a win32 api call to obtain
**	a detailed error code.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	s1 - mandatory error message
**	s2 - optional error message(should only be used if a win32 api call
**		failed.
**		
** Outputs:
**	An error message.
**
** Returns:
**	To operating system a FAIL return code.
**
** Side effects:
**	Causes process to terminate.
**
** History:
**	14-jun-95 (reijo01)
**	    Created.
*/
void
error(char *s1, char *s2)
{
    DWORD 	error_code;
    if (s2)
    {
	error_code = GetLastError();
	SIfprintf( stderr, "%s: %s %s %d\n", iirundbms, s1, s2, error_code );
    }
    else
    {
	SIfprintf( stderr, "%s: %s %s\n", iirundbms, s1, s2 );
    }
    PCexit(FAIL);
}

/*
** Name: get_sys_dependencies -   Set up global variables
**
** Description:
**	Replace system-dependent names with the appropriate
**	values from gv.c.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Side effects:
**	initializes global variables
**
** History:
**	04-apr-1997 (canor01)
**	    Created.
*/
void
get_sys_dependencies()
{
    STprintf( iirun, "%srun.exe", SystemCfgPrefix );
    STprintf( iirundbms, "%srunice", SystemCfgPrefix );
}
