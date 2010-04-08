/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <me.h>
#include    <nm.h>
#include    <lo.h>
#include    <st.h>
#include    <si.h>
#include    <rusage.h>
#include    <pc.h>
#include    <clsigs.h>
#include    <clnfile.h>
#include    <errno.h>
#include    <cs.h>
#include    <cx.h>
#if defined(xCL_018_TERMIO_EXISTS)
#include    <termio.h>
#elif defined(xCL_TERMIOS_EXISTS)
#include    <termios.h>
#include    <sys/ioctl.h>
#endif
#ifdef xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif
#ifdef  xCL_006_FCNTL_H_EXISTS
# include       <fcntl.h>
#endif  /* xCL_006_FCNTL_EXISTS */

/**
**
**  Name: IIRUN.C - Startup an Ingres daemon process on UNIX
**
**  Description:
**      This file contains the routines necessary to startup any
**      Ingres daemon process on UNIX
**
**          main() - The main routine -- argument processing
**
**
**  History:
**      21-Jun-1988 (mmm)
**          Created.
**      21-Jan-1989 (mmm)
**          changed to full TS's supportablity requests.  ps output of the
**          rcp and acp will now have the full path name to the executable
**          (thus identifying what II_SYSTEM they were started with; and the
**          second argument will be the II_INSTALLATION value.
**      15-feb-1989 (mmm)
**          Added message reporting if the dmfrcp changes II_LG_MEMSIZE.
**	01-Mar-1989 (fredv)
**          Added ifdef xCL_xxx from clconfig.h.
**          Use <rusage.h> instead of <time.h> and <sys/resource.h>
**          Use <clsigs.h> instead of <signal.h>
**          Use PCfork instead of vfork.
**          Use PCexit instead of exit and _exit.
**          Use PCreap instead of wait3 because wait3 is not portable.
**	13-Mar-1989 (fredv)
**          Moved the setpgrp() call to the child process so that the
**          child process will disconnect from the terminal monitor.
**          This will work for System V. Need to test if it works for
**          BSD as well.
**	15-Mar-89 (GordonW)
**          setpgrp() should have the correct pid passed. Also on execve
**          call, set argv[0] equal to only the basename of image, for
**          easier identification thru a ps.
**	19-Apr-89 (markd)
**          Removed unecessary inclusion of socket.h.
**	29-Jun-89 (GordonW)
**          allow other programs besides "dmfrcp" and "dmfacp" to use this
**	10-Jul-89 (GordonW)
**          sleep for 20 seconds before starting up dmfacp, needed to
**          let dmfrcp quiet down first. Also allow parameters to be passed
**          along to requested process.
**	12-Jul-89 (GordonW)
**          It should tell us an error if the fork fails.
**	14-Sep-1989 (fredv)
**	    In order to have the child process disconnected from the pseudo 
**	    tty-line completely for IBM RT, we must close stdin, stdout,
**	    and stderr for the child process.
**	16-sep-89 (russ)
**	    Call PCreap *before* resetting the signal in reaper().
**	25-Sep-1989 (Paul Houtz)
**	    Added a short (2 sec) delay after parent forks off a child.
**	    This should give the child a chance to 'exec()' before the
**	    parent returns.  Was getting timing problems where startup
**	    scripts use "ps" to see if the child was created, but "ps"
**	    executes before the child reaches teh 'exec()' call.
**	10-Oct-1989 (fredv)
**	    Backed out my change at 14-Sep-1989. We don't want to close
**	    stdin, stdout, and stderr in iirun because we won't be able to
**	    get any error messages from the child processes. Closing those
**	    three files are done in the child processes now.
**	28-Nov-89 (GordonW)
**	    Revamped to easily configure differen waits and such when
**	    forking off INGRES servers.
**	17-jan-90 (mikem)
**	    Took porting version with one change:
**          Changed iirun back to printing full path in ps.  This makes
**          it much easier to determine which installations on a multiple
**          installation machine the executable is associated with.
**	18-feb-1990 (greg)
**          NMgtAt is a VOID.
**	27-Feb-90 (anton)
**          Be sure file descriptors 0, 1, and 2 are safe to write on
**          and that we don't inherit any others.
**	28-Mar-90 (anton)
**          Add support for SunOS 4.1 use of RLIMIT for more file
**          descriptors.
**	18-may-90 (blaise)
**          Integrated changes from 61 and ingresug:
**	    Include fcntl.h if xCL_006_FCNTL_EXISTS is true, so that O_RDWR
**	    is defined.
**	4-june-90 (blaise)
**          Integrated changes from termcl code line:
**	    Add proc_ctl for sqs_us5 to prevent the servers from being
**	    niced.
**      26-sep-1991 (jnash) merged 22-Oct-90 (pholman)
**          Add the Security Audit Process to the list of valid programs to be
**          started.
**      28-jul-1992 (terjeb)
**          setpgrp() on the HP9000 has no argument.
**      01-apr-1993 (smc)
**          Modified above change to rely on xCL_086_SETPGRP_0_ARGS
**          so it will work for axp_osf also.
**	15-jul-93 (ed)
**          adding <gl.h> after <compat.h>
**	01-feb-94 (vijay)
**          reduce all the unnecessary sleeps. These are no longer needed
**          because a) ingstart now waits for rcp to go on line, 
**	    b) startup programs don't use "ps" 
**	05-jan95 (sweeney)
**          start iidbms from iirun -- that way we start as a daemon
**          process, detach from our control terminal, inherit more file
**          descriptors, etc ad nauseam. It should have been this way
**          from day one.
**	08-feb-95 (sweeney)
**          Add iistar as a valid program to start.
**	08-feb-95 (sweeney)
**          Deleted some dead code that was troubling the compiler.
**	21-feb-1995 (canor01)
**          don't reset signal in reaper() (B66962)
**	28-mar-1995 (canor01)
**          add NO_OPTIM = rs4_us5
**	19-may-1995 (sweeney)
**          Back out Orlan's change of 21-feb -- the real problem is
**          that we should be calling PCfork() and not fork() --
**          PCfork initialises a global data structure; PCreap()
**          will return without calling wait() to clear SIGC[H]LD
**          if this structure isn't initialized, and when we replant
**          our handler, the SIGC[H]LD is still outstanding. So we
**          call our handler...
**	24-may-1995 (sweeney)
**          Substantially rewritten. Inlined some singleton functions
**          for clarity, implemented simple signal IPC for intermediate
**          fork to inform parent about child process(es), moved some
**          statics/forward refs to the relevant sections of the file.
**	29-may-1995 (sweeney)
**          remove the forking_rcp logic -- it doesn't work in OpenINGRES
**      14-jun-1995 (qiuji01)
**          added a declaration for fd(it got complained as undefined symbol 
**          on Sun OS)
**	18-jul-1995 (thoro01)         for hp8_us5 and dg8_us5 systems ....
**	    Randomly, ingstart would report that either the archiver, name
**	    or the net server would fail to start when in reality, they 
**          were started without any errors.  In a nutshell, the intermediate
**          process/pid was exiting too quickly.  The grandchild process
**          did not have time to issue its SIGUSR1 signal to the parent.
**          When the intermediate process/pid exited via the PCexit(), a
**          SIGCHLD was issued to the parent.  This set the stillborn flag
**          and when the parent(iirun) returned to ingstart.c, stillborn and
**          offspring were both == 1.  Thus iirun returned a 5 when in fact
**          all was working find.  This is a timing issue.  This error did not
**          happen in the OpenIngres 1.1/03 iirun.c. The circumvention is to
**          put the intermediate process/pid to sleep for 5 seconds so that
**          the parent task can complete normally.  If a child has an error,
**          the error SIGCHLD is still reported properly to iirun/the parent.
**          I was advised by development that this is to be a temporary 
**          solution.  This code will be rewritten to take this problem into 
**          account.
**	10-jan-1995 (sweeney)
**	    Remove signal IPC. If unreliable signals are implemented
*	    faithfully we have a gross race condition as seen above:
**	    In the protocol implemented, the stillborn counter was meant
**	    to be set by the SIGCHLD from the intermediate exit, but
**	    could potentially stomp either of the SIGUSR1 & SIGUSR2
**	    depending on timing. The parent's intermediate & offspring
**	    counts weren't being set reliably, hence the initial bug
**	    and workround described above. Moving to the reliable signal
**	    interface provided by the CL EX module revealed a much
**	    smaller timing window where a signal could be handled inbetween
**	    testing for its arrival and calling pause(), causing iirun
**	    to hang. So give up on trying to propagate the SIGCHLD from
**	    early exec failure through the intermediate process, since
**	    we cannot do so reliably, but ensure that the file and permissions
**	    of the program we are being asked to start are good before
**	    calling exec, which should trap most ocurrences.
**	 1-aug-1995 (allst01)
**	    Reapplied robf's su4_cmw change from 9-aug-93
**	31-may-1996 (sweeney)
**	    Use a more sensible umask. Partial fix for bug # 76162.
**       08-Feb-1996 (rajus01)
**          Added "iigcb" (Protocol Bridge) program in prog_list.
**	18-oct-96 (mcgem01)
**	    Add the set of Jasmine executables to the prog_list
**	16-dec-1993 (reijo01)
**            Ifdef the Jasmine process names and use generic system variable 
**	      where applicable.
**	25-mar-98 (mcgem01)
**	    Add support for the gateway listeners.
**	26-Aug-1998 (hanch04)
**	    Added rmcmd
**	03-Dec-1998 (hanch04)
**	    Set RLIMIT_DATA to maximum.
**	13-Jan-1999 (peeje01)
**	    Added icesvr to the prog_list
**	    Added check for icesvr as a legal name
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_us5.
**	27-oct-1999 (somsa01)
**	    Changed timeout for icesvr to zero. iirunice will wait for the
**	    II_DBMS_SERVER printout anyway, as does iirundbms for the DBMS.
**       24-Feb-2000 (rajus01)
**          Added iijdbc.
**	21-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products. Also, added get_sys_dependencies(), which will
**	    properly name binaries by product.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (hweho01)
**          1) Setup the appropriate parameter of EXsetsig() for ris_u64   
**             platform.
**          2) Child process also calls PCfork() if the BSD version  
**             of setpgrp() is used.
**	24-Apr-2001 (ahaal01)
**	    Added rs4_us5 platform to 17-Apr-2001 change by hweho01.
**      Jul-31-2001 (hweho01)
**          To detect the error after setpgrp function call (BSD flavour),   
**          -1 should be used to compare with the returned value.   
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	26-sep-2002 (devjo01)
**	    Add NUMA support (-rad=radid argument, & consistency checks).
**	04-Oct-2002 (mofja02)
**	    Added startup changes for the db2 udb gateway.
**      24-Feb-2003 (wansh01)
**          Added iigcd DAS server.
**	07-may-2003 (devjo01)
**	    Add a short sleep before intermediate child process exits.
**	    This addresses bug 110204 in which iirun incorrectly returns
**	    a status code indicating failure.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	05-Nov-2004 (hweho01)
**	    Removed rs4_us5 from NO_OPTIM list, it is no longer needed.
**	18-Jun-2005 (hanje04)
**	    Add xCL_TERMIOS_EXISTS as newer OS's don't have TERMIO.
**      04-Feb-2006 (hweho01)
**          Enable xCL_034_GETRLIMIT_EXISTS for AIX, but not use
**          the max number returned from getrlimit, set it to the
**          OS limit OPEN_MAX.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**/

/* Ming hints
PROGRAM = (PROG1PRFX)run
NO_OPTIM = i64_aix
NEEDLIBS = COMPATLIB MALLOCLIB
*/

typedef	struct _PROG_LIST
{
	char	*name;			/* progname */
	i4	flagword;		/* startup flags */
	STATUS	(*checkit)();		/* check startup routine */
	i4	bwait;			/* wait amount before fork */
	i4	await;			/* wait amount after fork */
} PROG_LIST;


/*
**  Forward and/or External function references.
*/
static STATUS pollTmpFile(char *tmpFile);
static STATUS pollChildPassOrFail(int pid, char *tmpFile);


/*
**  Definition of static variables and forward static functions.
*/

static STATUS	check_dummy(PROG_LIST *p);
static VOID	process_flags( PROG_LIST *p );
static VOID	handler(i4 sig);
static VOID	usage_and_die(char *prog);
static VOID	get_sys_dependencies(void);

/* some flags for the signal handler */

static i4  stillborn;

/* flags for "flagword" */

#define	C_STDIN		0x01	/* close stdin */
#define	C_STDOUT	0x02	/* close stdout */
#define	C_STDERR	0x04	/* close stderr */
#define	C_ALL		(C_STDIN | C_STDOUT | C_STDERR)
#define	C_EXPAND	0x08	/* expand file descriptors */
#define	C_AFFINE	0x10	/* Set process RAD affinity */
#define	C_NEEDRAD	0x20	/* Must supply -rad arg if NUMA clusters */

/* legal programs to start */

static char dmfrcp[GL_MAXNAME]    = "dmfrcp";
static char dmfacp[GL_MAXNAME]    = "dmfacp";
static char iigcn[GL_MAXNAME]     = "iigcn";
static char iigcc[GL_MAXNAME]     = "iigcc";
static char iigcd[GL_MAXNAME]     = "iigcd";
static char iigcb[GL_MAXNAME]     = "iigcb";
static char iidbms[GL_MAXNAME]    = "iidbms";
static char iistar[GL_MAXNAME]    = "iistar";
static char rmcmd[GL_MAXNAME]     = "rmcmd";
static char icestart[GL_MAXNAME]  = "icesvr";
static char iigwinfd[GL_MAXNAME]  = "iigwinfd";
static char iigworad[GL_MAXNAME]  = "iigworad";
static char iigwsybd[GL_MAXNAME]  = "iigwsybd";
static char iigwmssd[GL_MAXNAME]  = "iigwmssd";
static char iigwodbd[GL_MAXNAME]  = "iigwodbd";
static char iigwudbd[GL_MAXNAME]  = "iigwudbd";
static char iigwstart[GL_MAXNAME] = "iigwstart";

static	PROG_LIST prog_list[] =
{
	{iigcn, C_STDIN, check_dummy, 0, 0},
	{iigcc, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigcd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
 	{iigcb, C_STDIN|C_EXPAND, check_dummy, 0, 0},
	{iidbms, C_STDIN|C_EXPAND|C_AFFINE|C_NEEDRAD, check_dummy, 0, 0},
	{dmfrcp, C_ALL|C_AFFINE|C_NEEDRAD, check_dummy, 0, 0},
	{dmfacp, C_ALL|C_NEEDRAD, check_dummy, 0, 0},
	{iistar, C_STDIN|C_EXPAND|C_NEEDRAD, check_dummy, 0, 0},
        {rmcmd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
	{icestart, C_STDIN|C_EXPAND|C_NEEDRAD, check_dummy, 0, 0},
        {iigwinfd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigworad, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigwsybd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigwmssd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigwodbd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigwudbd, C_STDIN|C_EXPAND, check_dummy, 0, 0},
        {iigwstart, C_STDIN|C_EXPAND, check_dummy, 0, 0},
	{0, 0, check_dummy, 0, 0}
};

/*
** Name: main	- This is the entry point for this program.
**
** Description:
**      This program is used to start the Sun/UNIX Ingres ACP or RCP Server. 
**
** Inputs:
**      none
**
** Outputs:
**	none.
**
**	Returns:
**            Sun/UNIX Error code
**	Exceptions:
**            none
**
** Side Effects:
**            none
**
** History:
**      21-Jun-1988 (mmm)
**          Created.
**      21-Jan-1989 (mmm)
**          changed to full TS's supportablity requests.  ps output of the
**            rcp and acp will now have the full path name to the executable
**            (thus identifying what II_SYSTEM they were started with; and the
**            second argument will be the II_INSTALLATION value.
**	26-Mar-90 (anton)
**            Add SunOS 4.1 setrusage to get more file descriptors
**            Get more file descriptors for IIGCC
**	26-sep-1991 (jnash) merged 22-Oct-90 (pholman)
**            Add the Security Audit Process to the list of valid programs to be
**            started.
**	18-jan-1993 (pholman)
**            C2: The DMFSAP is now obsolete and is thus no longer a valid program
**	16-dec-1993 (reijo01)
**            Ifdef the Jasmine process names and use generic system variable 
**	      where applicable.
**	30-sep-1997 (canor01)
**	      Dup jasmine server stdout to /dev/null; get startup id
**	      from stderr.
**	21-apr-2000 (somsa01)
**	    Added execution of get_sys_dependencies().
**	23-sep-2002 (devjo01)
**	      Process NUMA args.
**	26-Jul-2005 (hanje04)
**	    BUG 114921; ISS 14279659
**	    handler() used to "make sure we [new server] aren't fazed" when the
**	    parent dies does nothing and if the server that is started doesn't 
**	    handle SIGHUP, receiving it will cause a shutdown. (e.g. IIACP)
**	    Also quiet kernel message on Linux caused by ignoring SIGCHLD
**	    and then calling wait().
**      30-Apr-2008 (coomi01) BUG 120312
**          IFF an pair of extra parameters "pollFile xxxx" are provided to
**          iirun, we execute in a synchronous mode, waiting for the child
**          to either write PASS/FAIL to file xxxx or quit. The case where
**          we have an unexpected exit, a SegV for example, is now caught.
**      14-Jul-2008 (coomi01) BUG 120312
**          A simple tidyup, put breaks at end of case blocks to prevent
**          fall-through in strange circumstances.
**      09-Feb-2009 (coomi01) b121587
**          Removed SIGCHLD handlers. We are now polling the child processes
**          waiting (sic) for them to exit. SysV 'auto reap' behaviour
**          defeats our calls to waitpid(), and was non-portable to other
**          UNIX systems anyway.
**      16-Jun-2009  (coomi01) b122163
**          If the user has an excessive limit on open file descriptors, 
**          since there is no portable way of requesting a list of open
**          descriptors we will then spend an excessive amount of time
**          attempting to close them - even though they are not open. 
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

/* ARGSUSED */

main(argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
        PID             firstChildPid;
        PID             secondChildPid;
    	i4		pid;
    	LOCATION	imageloc;
    	char		image[MAX_LOC + 1];
	LOINFORMATION	proginfo;
	i4		flags;
    	char		*inst;
    	char		*argv_child[20];
    	STATUS		status;
    	i4		n = 0, x = 0;
	i4             fd;
	i4		rad;
    	bool		fullpath;     
    	PROG_LIST	*p;
	char		dbms_name[LO_FILENAME_MAX];
	char		instbuf[8];
	char            *pollFile = NULLPTR;

    	MEadvise(ME_INGRES_ALLOC); /* must be first */

	/* initialize global variables */
	get_sys_dependencies();

	/* Establish NUMA context if required */
        (void)CXget_context( &argc, argv,
	  CX_NSC_CONSUME|CX_NSC_RAD_OPTIONAL|CX_NSC_SET_CONTEXT, NULL, 0 );

	/* if we're called with no args, show usage */

	if (argc < 2)
            usage_and_die(argv[0]);

	/*
	** We want to permit command lines like:
	** iirun iidbms
	** iirun iidbms.debug
	** iirun iidbms.test
	** iirun ./iidbms
	** iirun /build/65/ingres/bin/iidbms
	** &c.
	*/

	{ /* set up a couple of pointers for use in the switch */
	char	*s = argv[1];
	char	*e = image;

            switch (fullpath = (*s == '/' || *s == '.'))
            {

            case 1:
                /* skip to the end of the string */
                while(*s) s++;

                /* rewind to the last '/' */
                while(*s != '/') s--;

                s++; /* we don't want the '/' */

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
               STcompare(image, prog_list[pid].name) )
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
	else
	{
	    LOfroms(PATH & FILENAME, image, &imageloc); /* for LOinfo below */
	}

	/* set argv[0] to full path name to make debugging multiple installation
	** machines easier.
	*/
	n = 0;
	argv_child[n++] = image;

	/* if II_INSTALLATION is set, pass it to the child */

	NMgtAt("II_INSTALLATION", &inst);

	if ( ( p->flagword & C_NEEDRAD ) &&
	     ( 0 != ( rad = CXnuma_cluster_rad() ) ) )
	{
	    if ( !inst || 2 != STlength(inst) ) 
	    {
		/*
		** NUMA clusters is very fussy about this.
		** Bail if installation code is invalid.
		*/
		SIfprintf(stderr, 
		  "NUMA Clusters requires II_INSTALLATION (%s) be valid.\n",
		  inst ? inst : "<missing>");
		PCexit(FAIL);
	    }
	    STprintf(instbuf,"%s.%d", inst, rad);
	    inst = instbuf;

	    if ( p->flagword & C_AFFINE )
		(void)CXnuma_bind_to_rad(rad);
	}
	    
	STcopy(SystemCatPrefix, dbms_name);
	STcat(dbms_name, "dbms");

	if ( STcompare(p->name, dbms_name) &&
             STcompare(p->name, "iistar") &&
             STcompare(p->name, "icesvr") &&
             inst && *inst)
		argv_child[n++] = inst;

	/* now copy over all the others */

	x = 2;
	while( x < argc )
	{
	    /*
	    ** Check invocation to see if we are required
	    ** to poll for PASS/FAIL on a dedicated file
	    ** - Note, be careful to ensure we can consume two parms here
	    */
	    if ( 0 == STcompare(argv[ x ],"pollFile") ) 
	    {
		/*
		** Consume the introductory parm
		*/
		x += 1;

		/* 
		** Have we another parm ?
		*/
		if (x < argc)
		{
		    /*
		    ** Save the file name
		    */
		    pollFile = argv[x];

		    /*
		    ** Consume that parm too
		    */
		    x += 1;
		}
	    }
	    else
	    {
		argv_child[n++] = argv[ x++ ];
	    }
	}

	argv_child[n] = (char *)NULL;

	/*
	** now that we have the full command line, ensure the program
	** exists and is executable -- this is to (partially) make up
	** for the inability to catch early child failure on SYSV
	*/
        
	flags = (LO_I_TYPE | LO_I_PERMS);

	if (LOinfo(&imageloc, &flags, &proginfo) != OK)
	{
		SIprintf("%s: can't find %s\n", argv[0], imageloc.string);
		PCexit(-1);
	}
	else
	{

		if (!(flags & LO_I_TYPE))
		{
			SIprintf("%s: can't stat %s\n", argv[0],
					imageloc.string);
			PCexit(LO_I_TYPE);
		}
		if (proginfo.li_type != LO_IS_FILE)
		{
			SIprintf("%s: %s is not a regular file\n",
					argv[0], imageloc.string);
			PCexit(LO_IS_FILE);
		}
		if (!(proginfo.li_perms & LO_P_EXECUTE))
					/* don't need read on SVR4 */
		{
			SIprintf("%s: %s is not an executable\n",
					 argv[0], imageloc.string);
			PCexit(LO_P_EXECUTE);
		}
	}


	/* we've been passed a valid process name -- start it as a daemon.  */

	if(p->bwait)
            sleep( p->bwait );

	/*
	** Ignore Berkeley job control signals (where defined).
	*/

# ifdef SIGTTOU
	signal( SIGTTOU, SIG_IGN );
# endif
# ifdef SIGTTIN
	signal( SIGTTIN, SIG_IGN );
# endif
# ifdef SIGTSTP
	signal( SIGTSTP, SIG_IGN );
# endif

	/* don't inherit a bogus umask from the invoking shell.  */

	umask(022); /* no write for group, other */

	/* Set process-RAD affinity if desired. */
	/* fork (possibly twice) and exec */

	switch (firstChildPid = PCfork(&status))
	{
	case -1:
            /* can't fork */
            status = errno;
            SIprintf("Sorry, Can't fork %s\n", image);
            PCexit(status);
	    break;

	case 0:
            /* Child */
#if !defined xCL_086_SETPGRP_0_ARGS
            /* BSD flavour setpgrp */
            if (setpgrp(0, getpid()) == -1) 
            {
                status = errno;
		SIprintf("Sorry, Can't change process group for %s\n", image);
		PCexit(status);
            }

# ifdef TIOCNOTTY
            /* say goodbye to our control terminal */
            if ( (fd = open("/dev/tty", O_RDWR) ) >= 0 )
            {
		ioctl(fd, TIOCNOTTY, (char *) NULL);
		close(fd);
            }
# endif /* TIOCNOTTY */

#else
            /* SYSV flavour setpgrp */
            if(setpgrp() == -1) /* create new pgrp, lose control terminal */
            {
		status = errno;
		SIprintf("Sorry, Can't change process group for %s\n", image);
		PCexit(status);
            }

#endif

	    if ( rad && (C_AFFINE & p->flagword) )
	    {
		/* Set process affinity */
		(void)CXnuma_bind_to_rad(rad);
            }		

            /* fork again, so we can't reacquire a control terminal */

            EXsetsig(SIGHUP, SIG_IGN); /* make sure we aren't fazed when the
					parent process [group leader] dies */

            if ( (secondChildPid = PCfork(&status)) < 0 )
            {
		status = errno;
		SIprintf("Sorry, Can't fork again for %s\n", image);
		PCexit(status);
            }

            else if (secondChildPid > 0)
            {
		/* intermediate parent */
		if ( NULLPTR != pollFile )
		{
		    /* 
		    ** This will synchronous wait on the child pid
		    ** - Or Pass/Fail File
		    */
		    if ( OK == pollChildPassOrFail(secondChildPid, pollFile) )
		    {
			/*
			** Child has written Pass/Fail
			*/
			PCexit(OK);
		    }
		    else
		    {
			/*
			** Child has quit without writing Pass/Fail
			*/
			PCexit(FAIL);
		    }
		}
		else
		{
		    sleep(1);
		    PCexit(OK);
		}
            }

            process_flags(p); /* if C_EXPAND is set, max out fds */

            execve(image, argv_child, envp);
            status = errno;
            SIprintf("Sorry, Can't exec %s\n", image);
            PCexit(status);
	    break;

            default:
            {
                /* Top parent */
                if ( NULLPTR != pollFile )
                {
                    /* 
                    ** This will synchronous wait on the child pid
                    ** - But not the file
                    */
                    pollChildPassOrFail(firstChildPid, NULLPTR);

                    /*
                    ** Read result code from child
                    */
                    status = PCwait(firstChildPid);

                    /*
                    ** Return that result to our parent
                    */
                    PCexit (status);
                }
                break;
            }
        }

	if(p->await)
            sleep( p->await );

        /* exit fail if the daemon process died already */

	PCexit( stillborn ? 5 : 0 );
}

static VOID
process_flags( PROG_LIST *p )
{
# ifdef xCL_034_GETRLIMIT_EXISTS
	struct rlimit	res;
# endif
	i4	size;
	int	i;


	if ( p->flagword & C_STDIN )
		SIclose(stdin);
	if ( p->flagword & C_STDOUT )
		SIclose(stdout);
	if( p->flagword & C_STDERR )
		SIclose( stderr );

	/*
	** ANSI says that a new program can expect to be able to
	** write on stdin, stdout, stderr
	*/
	i = open("/dev/null", O_RDWR);
	while (i < 2 && i >= 0)
	{
		i = dup(i);
	}

	/* close all the others */

	i = CL_NFILE();

	/*
	** Look to optimise this where the high water mark is excessively large
	*/
	if ( i > 16384 )
	{
	    /*
	    ** Bug 122163
	    ** Get next available descriptor
	    ** - On the assumption the kernel will return first available
	    **   giving us a better estimate of how many open descriptors
	    **   are indeed active.
	    ** - There is a very very small risk that someone out there
	    **   using a daemon process without using close-on-exec will
	    **   now get caught out.
	    ** - Most will, however, start Ingres from an rc-init script
	    **    and will not.
	    ** - Those starting Ingres via a login shell are also not at risk. 
	    */
	    i = open("/dev/null", O_RDWR);
	    close(i);

	    /* Guarentee at least a minimum of 16k 
	     */
	    if (i < 16384 )
		i = 16384;
	}

	while (--i > 2) close(i);

	if ( p->flagword & C_EXPAND )
	{
		size = CL_NFILE();

# ifdef	xCL_062_SETDTABLESIZE

		do {
	            if( ((i=setdtablesize( size+1 )) < 0) || (i == size) )
			break;
	            size = i;
	            } while( TRUE );

# endif	/* xCL_062_SETDTABLESIZE */

# ifdef xCL_034_GETRLIMIT_EXISTS

# if defined(sun_u42) || defined(sparc_sol) || defined(su4_cmw)
# ifndef RLIMIT_NOFILE
# define RLIMIT_NOFILE	6 /* maximum descriptor index + 1 */
# endif
# endif

# ifdef	RLIMIT_NOFILE

		if (getrlimit(RLIMIT_NOFILE, &res) == 0 &&
	            res.rlim_cur < res.rlim_max)
		{
	            /* must get some more file descriptors */
#if defined(any_aix)
                   /* Set to the limit in AIX */
                    res.rlim_cur = OPEN_MAX ;
#else
	            res.rlim_cur = res.rlim_max;
#endif
	            _VOID_ setrlimit(RLIMIT_NOFILE, &res);
		}
# endif

# ifdef	RLIMIT_DATA

		if (getrlimit(RLIMIT_DATA, &res) == 0 &&
	            res.rlim_cur < res.rlim_max)
		{
	            /* must get some more file descriptors */
	            res.rlim_cur = res.rlim_max;
	            _VOID_ setrlimit(RLIMIT_DATA, &res);
		}
# endif

# endif /* xCL_034_GETRLIMIT_EXISTS */
	}
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

        SIfprintf(stderr, "Usage: %s [-rad=radid] {", prog);

        while(prog_list[pid++].name != NULL)
                SIfprintf(stderr, "%s%s",
                        prog_list[pid-1].name,
                        prog_list[pid].name ? "," : "");
 
        SIfprintf(stderr, "} {argvs}\n");
        PCexit(FAIL);
}

static VOID
handler( int sig )
{
    /*
    ** We must clear the SIGCHLD by calling PCreap (which calls wait())
    ** _before_ replanting our handler. This relies on having used PCfork()
    ** and not fork() or vfork() to start the child process.
    */
    switch(sig)
    { 
        case SIGCHLD:
            /* intermediate or final child death -- must call wait() to clear */
            PCreap();
            stillborn++;
            break;
	default:
            /* huh? */
            PCexit(FAIL);
    }
 
    /* replant the handler */
    EXsetsig( sig, handler );
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
**	21-apr-2000 (somsa01)
**	    Created from NT version.
*/
static VOID
get_sys_dependencies()
{
    STprintf( iigcn, "%sgcn", SystemAltExecName );
    STprintf( iigcc, "%sgcc", SystemAltExecName );
    STprintf( iigcb, "%sgcb", SystemAltExecName );
    STprintf( iigcd, "%sgcd", SystemAltExecName );
    STprintf( iidbms, "%sdbms", SystemAltExecName );
    STprintf( iigwinfd, "%sgwinfd", SystemAltExecName );
    STprintf( iigworad, "%sgworad", SystemAltExecName );
    STprintf( iigwsybd, "%sgwsybd", SystemAltExecName );
    STprintf( iigwmssd, "%sgwmssd", SystemAltExecName );
    STprintf( iigwodbd, "%sgwodbd", SystemAltExecName );
    STprintf( iigwudbd, "%sgwudbd", SystemAltExecName );
    STprintf( iigwstart, "%sgwstart", SystemAltExecName );
}


/*
** Name:
**      pollTmpFile
**
** Description:
**
**      Attempts to read contents of a temporary file
**      which will contain text PASS or FAIL on a new 
**      line boundry.
**
** Inputs:
**	tmpFile     :  The name of the file passed down
**
** Outputs:
**
**	OK   ~ PASS/FAIL contents were found
**      FAIL ~ otherwise
**
** Side effects:
**	None
**
** History:
**	30-apr-2008 (coomi01)
**	    Created.
**      20-May-2008 (coomi01)
**          Removed new-line from in front of PASS/FAIL strings
**          Some utilities put this string at end of startup message.
**
*/
static STATUS
pollTmpFile(char *tmpFile)
{
    char *fail = "FAIL";
    char *pass = "PASS";
    char     buffer[4096];
    FILE          *handle;
    i4             result;
    i4              count;
    LOCATION          loc;

    /* 
    ** Set up location struct
    */
    loc.string = tmpFile;

    result = FAIL;
    if ( OK == SIopen(&loc, "r", &handle) )
    {
        count = 0;
        SIread(handle, 4095, &count, buffer);
        if (count)
        {
            buffer[count] = 0;
            if ( strstr(buffer,pass) || strstr(buffer,fail) )
        	result = OK;
        }
        
        SIclose(handle);
    }

    return result;
}

/*
** Name:
**      pollChildPassOrFail
**
** Description:
**
**      Watches a child pid for exit,      
**      or waits to see if a PASS/FAIL was written 
**      it to a temp file whenever it is provided.
**
** Inputs:
**      pid         :  The process id of the child
**	tmpFile     :  The name of the file passed down
**                     - or null if no file is provided.
**
** Outputs:
**
**	OK   ~ PASS/FAIL contents were found
**      FAIL ~ otherwise
**
** Side effects:
**	None
**
** History:
**	30-apr-2008 (coomi01)
**	    Created.
**      29-Dec-2008 (coomi01) b121587
**          Change call on PCis_alive() to PCreap_withThisPid()
**          - PCis_alive uses a kill(0) to check the target process.
**            This is not fool proof, for it can cause SIGCHLD to
**            be masked. If this signal is missed, the child becomes
**            and remains a zombie - for we hooked the signal to reap
**            the result code. This leads to a hangup.
**          - Now we effectively call waitpid(). This will reap the
**            child if necessary. So we shall not hang any more.
**
*/
static STATUS
pollChildPassOrFail(PID pid, char *tmpFile)
{
    i4  status;
    i4  resultCode = 0;
    i4  loop=0;

    while (1)
    {
        /* 
        ** See if child has written PASS/FAIL to file 
        */
        if ( (NULLPTR != tmpFile) && (OK == pollTmpFile(tmpFile)) )
        {
            status = OK; 
            break;
        }

        /*
        ** See if child exited unexpectedly
        */
        if ( PCreap_withThisPid(pid) == pid )
        {
            status = FAIL;
            break;
        }

        /* 
        ** Snooze for a second
        */
        sleep(1);
    }

    return status;
}
