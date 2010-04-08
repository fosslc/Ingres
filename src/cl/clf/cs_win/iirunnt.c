/*
** Copyright (c) 1992, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <cm.h>
#include    <ep.h>
#include    <er.h>
#include    <gl.h>
#include    <me.h>
#include    <nm.h>
#include    <lo.h>
#include    <st.h>
#include    <si.h>
#include    <pc.h>
#include    <errno.h>

/**
**
** Name: iirunnt.c - Startup a server as a process on NT.
**
** Description:
**      This file contains the routines necessary to startup any
**	server process on NT.
**
**  History:
**      14-jun-1995 (reijo01)
**          Created.
**      15-jul-95 (emmag)
**          Allocate a NULL DACL and use it for security when creating
**          objects that need to be accessed by more than one user.
**      20-jul-95 (sarjo01)
**	    Start non-iidbms server processes with DETACHED_PROCESS flag
**	    so no console is created.
**      02-aug-95 (sarjo01)
**	    Check for dmfrcp as an iidbms server (see above)
**      08-aug-95 (reijo01)
**	    Same check as previous change but for iistar.
**	    That is a star server should be treated like an iidbms server
**	    in that it should not be detached from the console when the
** 	    process is created.
**	11-aug-95 (emmag)
**	    Create INGRES processes with the HIGH_PRIORITY_CLASS bit set
**	    for a significant performance improvement.
**	06-sep-95 (emmag)
**	    Those less fortunate than myself don't have an SMP machine
**	    so the previous change chokes their machines.  For now, we'll
**	    just run the DBMS, STAR and UTILITY servers at the
**	    highest priority and everything else at normal priority.
**	14-dec-1995 (canor01)
**	    Add delays on startup to allow DLLs to initialize.
**	15-dec-1995 (canor01)
**	    Increase delay on dmfrcp for slower machines.
**	29-Feb-1996 (wonst02)
**	    Add iigws, the OpenINGRES Desktop gateway server.
**	04-apr-1997 (canor01)
**	    Use generic system names from gv.c to allow program to be
**	    used in both Jasmine and OpenIngres.
**      08-apr-1997 (hanch04)
**          Added ming hints
**	05-may-1997 (canor01)
**	    Create all processes as detached processes. Use the
**	    STARTF_USESTDHANDLES flag to be able to read the server
**	    id from standard io.
**  	20-may-97 (mcgem01)
**          Clean up compiler warnings.
**	10-oct-1997 (canor01)
**	    For Jasmine, explicitly open the null device for standard 
**	    output to be inherited by children, since we may be the child 
**	    of a non-console application.
**	13-nov-97 (mcgem01)
**	    In setting all processes to run detached, the process
**	    priority was over-written.
**	23-nov-97 (mcgem01)
**	    Add support for the protocol bridge.
**	24-nov-1997 (canor01)
**	    Quote the pathname to the executable to support embedded spaces.
**	25-mar-98 (mcgem01)
**	    Add support for the gateway listeners.
**      13-Aug-98 (fanra01)
**          Add support for ice server.
**	28-apr-1999 (mcgem01)
**	    Replaced SystemCfgPrefix with SystemAltExecName to differentiate
**	    between different product binaries.
**	29-jul-1999 (somsa01)
**	    If the process we are forking is iigwstart, let's wait until it
**	    exits before returning control to our caller. This is because
**	    iigwstart is itself a startup program (like ingstart) for the
**	    Gateways.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	30-mar-2000 (somsa01)
**	    Added iijdbc.
**	14-jun-2002 (mofja02)
**	    Added db2 udb gw.
**	27-Mar-2003 (wansh01)
**	    Added iigcd DAS server. 	
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, Cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	13-Jul-2005 (drivi01)
**          Expanded PROG_LIST structure to include two additional
**          fields: startup priority and runtime priority of the process.
**          Modified CreateProcess function to use startup priority of the
**          structure for initialization of iigcn and reset it to
**          runtime priority once initialization completed.
**	22-Jul-2005 (drivi01)
**	    Reset timeout in svc_parms to -1.  Properly cleanup after
**	    connections initiated with GCrequest.
**	25-Jul-2005 (drivi01)
**	    Removed code for setting name server priority back to normal
**	    in this file.  Priority will be set back to normal by Name Server.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**	25-Jul-2007 (drivi01)
**	    Added additional error handling for Vista OS.
**	    This program will recognize ERROR_ELEVATION_REQUIRED and
**	    ERROR_ACCESS_DENIED errors as Vista's lack of privileges
**	    to run the command specified.
**/

/*
**      mkming hints

NEEDLIBS =      COMPATLIB 


PROGRAM =       iirun

*/
#ifndef ERROR_ELEVATION_REQUIRED
#define	ERROR_ELEVATION_REQUIRED	740
#endif

typedef	struct _PROG_LIST
{
	char	*name;			/* progname */
	i4 	flagword;		/* startup flags */
	STATUS	(*checkit)();		/* check startup routine */
	i4 	bwait;			/* wait amount before fork */
	i4 	await;			/* wait amount after fork */
	u_i4	start_priority;		/* startup priority of the process */
	u_i4	run_priority;		/* post startup priority of the process */
} PROG_LIST;


/*
**  Forward and/or External function references.
*/


/*
**  Definition of static variables and forward static functions.
*/

static STATUS	check_dummy(PROG_LIST *p);
static VOID	usage_and_die(char *prog);
static VOID     get_sys_dependencies( void );



/* flags for "flagword" */

#define	C_STDIN		0x01	/* close stdin */
#define	C_STDOUT	0x02	/* close stdout */
#define	C_STDERR	0x04	/* close stderr */
#define	C_ALL		(C_STDIN | C_STDOUT | C_STDERR)
#define	C_EXPAND	0x08	/* expand file descriptors */

/* legal programs to start */

static char dmfrcp[GL_MAXNAME]	= ERx("dmfrcp");
static char dmfacp[GL_MAXNAME]	= ERx("dmfacp");
static char iigcn[GL_MAXNAME]	= ERx("iigcn");
static char iigws[GL_MAXNAME]	= ERx("iigws");
static char iigcc[GL_MAXNAME]	= ERx("iigcc");
static char iigcd[GL_MAXNAME]	= ERx("iigcd");
static char iigcb[GL_MAXNAME]	= ERx("iigcb");
static char iidbms[GL_MAXNAME]	= ERx("iidbms");
static char iistar[GL_MAXNAME]	= ERx("iistar");
static char rmcmd[GL_MAXNAME]	= ERx("rmcmd");
static char iigwinfd[GL_MAXNAME]  = ERx("iigwinfd");
static char iigworad[GL_MAXNAME]  = ERx("iigworad");
static char iigwsybd[GL_MAXNAME]  = ERx("iigwsybd");
static char iigwmssd[GL_MAXNAME]  = ERx("iigwmssd");
static char iigwodbd[GL_MAXNAME]  = ERx("iigwodbd");
static char iigwudbd[GL_MAXNAME]  = ERx("iigwudbd");
static char iigwstart[GL_MAXNAME] = ERx("iigwstart");
static char icestart[GL_MAXNAME] = ERx("icesvr");



/* 
** On VMware machines with Unicenter Services set to
** automatic, Name server takes over 90 seconds to come
** up.  Name Server is set to a higher priority 
** to make sure that it comes up in allowed time
** frame and then reset to a NORMAL_PRIORITY for duration
** of its life.
*/
static	PROG_LIST prog_list[] =
{
	{iigcn, C_STDIN, check_dummy, 3000, 0, ABOVE_NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iigcc, C_STDIN|C_EXPAND, check_dummy, 3000, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iigcd, C_STDIN|C_EXPAND, check_dummy, 3000, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iigcb, C_STDIN|C_EXPAND, check_dummy, 3000, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iidbms, C_ALL|C_EXPAND, check_dummy, 3000, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{dmfrcp, C_ALL, check_dummy, 5300, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{dmfacp, C_ALL, check_dummy, 10, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iigws, C_STDIN, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{iistar, C_STDIN|C_EXPAND, check_dummy, 3000, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{rmcmd, C_STDIN, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{icestart, C_STDIN, check_dummy, 0, 10000, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwinfd, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigworad, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwsybd, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwmssd, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwodbd, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwudbd, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
        {iigwstart, C_STDIN|C_EXPAND, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS},
	{0, 0, check_dummy, 0, 0, NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS}
};

/*
** Name: main	- This is the entry point for this program.
**
** Description:
**      This program is used to start an Ingres server.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	Name of server to start. Could be a full pathname of the executable.
**	Any additional parameters to pass on to the server.
**		
** Outputs:
**	Error messages.
**
** Returns:
**	OK
**	FAIL
**
** History:
**	14-jun-95 (reijo01)
**	    Created.
**      15-jul-95 (emmag)
**          Allocate a NULL DACL and use it for security when creating
**          objects that need to be accessed by more than one user.
**      20-jul-95 (sarjo01)
**	    Start non-iidbms server processes with DETACHED_PROCESS flag
**	    so no console is created.
**	01-feb-96 (emmag)
**	    Only run the server at high priority in SMP environments.
**	05-may-1997 (canor01)
**	    Create all processes as detached processes. Use the
**	    STARTF_USESTDHANDLES flag to be able to read the server
**	    id from standard io.
*/


/* ARGSUSED */

main(argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
    	i4 			pid;
    	LOCATION		imageloc;
    	char			image[MAX_LOC + 1];
    	char			*inst;
    	char			*argv_child[20];
    	i4 			n = 0, x = 0;
    	i4 			search_length = 20;
    	bool			fullpath;     
    	PROG_LIST		*p;
	DWORD			error_code;
	STARTUPINFO 		si;
	PROCESS_INFORMATION 	ProcessInfo;
	BOOL 			Status;
	char			*command_line, *arguments;
	u_i4 			command_line_length;
	SECURITY_ATTRIBUTES	handle_security, child_security;
	DWORD			fdwCreate;
	char			*num_proc;
	char			tmp_buf[GL_MAXNAME];
	bool			havequote;
        char			*nulldevice = "NUL:";
        HANDLE			nullfile;

	handle_security.nLength = sizeof(handle_security);
        handle_security.lpSecurityDescriptor = NULL;
        handle_security.bInheritHandle = TRUE;

        child_security.nLength = sizeof(child_security);
        child_security.lpSecurityDescriptor = NULL;
        child_security.bInheritHandle = TRUE;

    	MEadvise(ME_INGRES_ALLOC); /* must be first */

	/* initialize global variables */
	get_sys_dependencies();

	/* if we're called with no args, show usage */

	if (argc < 2)
            usage_and_die(argv[0]);

	/*
	** We want to permit command lines like:
	** iirunnt iidbms
	** iirunnt iidbms.debug
	** iirunnt iidbms.test
	** iirunnt .\iidbms
	** iirunnt \build\65\ingres\bin\iidbms
	** iirunnt x:\build\65\ingres\bin\iidbms
	*/

	{ /* set up a couple of pointers for use in the switch */
	char	*s = argv[1];
	char	*e = image;

            switch (fullpath = 
		       (*argv[1] == '\\' || 
			*argv[1] == '.' || 
			STindex(argv[1],":",0)))
            {

            case 1:
                /* skip to the end of the string */
		s = STrindex(s,"\\",0);

                s++; /* we don't want the '\' */

		/* fall through */
	
            default:

		/* copy everything up to the period or NUL */
                while(*s != '.' && *s != '\0') *e++ = *s++;

                *e = '\0';
            }
	}

	/* check the process name */
	pid = 0;
	while( prog_list[pid].name != NULL &&
               STcompare( image, prog_list[pid].name ) )
		pid++;
	if( prog_list[pid].name == NULL )
            usage_and_die(argv[0]);

	p = &prog_list[pid]; /* we use this later */

	/* it's OK to reuse image -- p->name is now server name */
		
	STcopy(argv[1], image); 
	
	if (!fullpath)
	{
	char *s;

            NMloc(BIN, FILENAME, image, &imageloc);
            LOtos(&imageloc, &s);
            STcopy(s, image);
	}

	/* set argv[0] to full path name to make debugging multiple installation
	** machines easier.
	*/
	argv_child[n++] = image;

	/*
	** Only run the server at high priority in an SMP environment.
	*/

	STprintf( tmp_buf, "%s_NUM_OF_PROCESSORS", SystemVarPrefix );
	NMgtAt( tmp_buf, &num_proc );

 	/*
 	** start_priority is equal to NORMAL_PRIORITY_CLASS for most
 	** servers except for Name Server.  fdwCreate will be set
 	** to NORMAL_PRIORITY_CLASS for all servers except iigcn.
 	*/
	if (!(num_proc && *num_proc))
	{
	    fdwCreate = prog_list[pid].start_priority;
	}
	else
	{
	    if (atoi(num_proc) == 1)
		fdwCreate = prog_list[pid].start_priority;
	    else
		fdwCreate = HIGH_PRIORITY_CLASS;
	}

	/* if II_INSTALLATION is set, pass it to the child */

	STprintf( tmp_buf, "%s_INSTALLATION", SystemVarPrefix );
	NMgtAt( tmp_buf, &inst );

	fdwCreate |= DETACHED_PROCESS;

	if ( STcompare( p->name, iidbms ) &&
             STcompare( p->name, iistar ) &&
             inst && *inst )
		argv_child[n++] = inst;

	/* now copy over all the others */

	x = 2;
	while( x < argc )
		argv_child[n++] = argv[ x++ ];

	argv_child[n] = (char *)NULL;


	/* we've been passed a valid process name -- start it as a daemon.  */

	if(p->bwait)
            PCsleep( p->bwait );

       /*  execve(image, argv_child, envp); */



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
	arguments = STskipblank(arguments, search_length);
	while (*arguments != '\0' && *arguments != '\n')
	{
	    if ( *arguments == '"' )
		havequote = (havequote) ? FALSE : TRUE;
	    if ( CMwhite(arguments) && havequote == FALSE )
		break;
	    arguments++;
	}
 
	/* Uncomment following line to debug the server */
	/*	STcat(image,".exe");			*/

	/*
	** 	Put together a command line to create a process with.
	**	- image + arguments + 3 blank separators + 2 quotes + EOS
	*/
	command_line_length = STlength(arguments) + STlength(image) +
				STlength(ERx("   ")) + 2 + 1;
	if((command_line = (char *)
		MEreqmem(0, command_line_length, TRUE, NULL)) == NULL)
	{
		SIfprintf(stderr,ERx("Request for memory failed"));
		PCexit(FAIL);
	}

	STprintf(command_line, ERx("\"%s\" %s"),image,arguments);

	/*
	**	Initialize the startinfo structure.
	*/
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput  = GetStdHandle( STD_INPUT_HANDLE );
        si.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        si.hStdError  = GetStdHandle( STD_ERROR_HANDLE );

	/*
	**	Start the server.
	*/
	Status = CreateProcess( NULL,
				command_line, 
				&handle_security, 
				&child_security,
				TRUE,
				fdwCreate,
				NULL,
				NULL,
				&si,
				&ProcessInfo );
	if (Status == FALSE)
	{
		error_code = GetLastError();
		switch(error_code)
		{
			case ERROR_ACCESS_DENIED: 
				SIprintf(ERROR_REQ_PRIVILEGE);
				PCexit(FAIL);
			case ERROR_ELEVATION_REQUIRED:
				SIprintf(ERROR_DENIED_PRIVILEGE);
				SIprintf(ERROR_REQ_ELEVATION);
				PCexit(error_code);
			default:
				SIfprintf(stderr,ERx("CreateProcess error: %d\n"),error_code);
				PCexit(FAIL);
		}
	}

	/*
	** If this is iigwstart, wait for it to exit.
	*/
	if (STcompare(prog_list[pid].name, iigwstart) == 0)
	    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

	/*
	**	Close the thread handle since we don't need it.
	*/
	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);

	if(p->await)
            PCsleep( p->await );

	PCexit(OK);
}


static STATUS
check_dummy( PROG_LIST *p )
{
        return OK;
}

static VOID
usage_and_die( char *prog )
{
        i4  pid = 0;

        SIfprintf(stderr, "Usage: %s {", prog);

        while(prog_list[pid++].name != NULL)
                SIfprintf(stderr, "%s%s",
                        prog_list[pid-1].name,
                        prog_list[pid].name ? "," : "");
 
        SIfprintf(stderr, "} {argvs}\n");
	

        PCexit(FAIL);
}

/*
** Name: get_sys_dependencies -   Set up global variables
**
** Description:
**      Replace system-dependent names with the appropriate
**      values from gv.c.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Side effects:
**      initializes global variables
**
** History:
**      04-apr-1997 (canor01)
**          Created.
**	240-mar-98 (mcgem01)
**	    Add the gateway listeners.
*/
VOID
get_sys_dependencies()
{
    STprintf( iigcn, "%sgcn", SystemAltExecName );
    STprintf( iigws, "%sgws", SystemAltExecName );
    STprintf( iigcc, "%sgcc", SystemAltExecName );
    STprintf( iigcd, "%sgcd", SystemAltExecName );
    STprintf( iigcb, "%sgcb", SystemAltExecName );
    STprintf( iidbms, "%sdbms", SystemAltExecName );
    STprintf( iigwinfd, "%sgwinfd", SystemAltExecName );
    STprintf( iigworad, "%sgworad", SystemAltExecName );
    STprintf( iigwsybd, "%sgwsybd", SystemAltExecName );
    STprintf( iigwmssd, "%sgwmssd", SystemAltExecName );
    STprintf( iigwodbd, "%sgwodbd", SystemAltExecName );
    STprintf( iigwudbd, "%sgwudbd", SystemAltExecName );
    STprintf( iigwstart, "%sgwstart", SystemAltExecName );
}
