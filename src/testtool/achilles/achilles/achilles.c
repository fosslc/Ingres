/**
 ** achilles.c
 **
 ** Main program for the achilles test driver. Handles command line parsing
 ** and initialization, and sets up signal handlers. From there, goes into an
 ** infinite loop of waiting for a signal. Most globals are declared here.
 **
 **
 ** History:
 **	26-Sept-89 (GordonW)
 **		added maxkids for setting an optional number of children.
 **	27-Sept-89  (gordonW)
 **		use a portable version of MEreqmem.
 **	13-NOV-89   (bls)
 **		modify stderr handling, add time and date stamp to start and 
 **		end, misc bug fixes.
 **	08-Feb-91   (dufour)
 **		Set string's pid value to global achpid when used in 
 **		creation of name for output directory.
 **	11-Jun-91   (donj)
 **		Merged unix and vms versions of achilles.c.
 **     05-Jul-91   (donj)
 **             Resolve last conflict between VMS and UNIX. The '-s' flag, in 
 **             VMS, is means te the server var 'II_DBMS_SERVER' and in UNIX 
 **             it means set the 'silent' flag where Achilles won't record
 **             normal stdout from children, only stderr. VMS, since it uses 
 **             batch queues for running the children, has no use for a
 **             'silent' flag.
 **	23-jul-1991 (donj)
 **		Modify needlibs to allow for building on UNIX with MING.
 **		
 **	29-apr-92  (purusho)
 **		Added amd_us5 specific changes 
 **	08-jun-92  (donj)
 **		Took out extraneous LIBS out of NEEDLIBS. Made tdfile
 **		bigger.
 **	10-Jul-92 (GordonW)
 **		Added ming hints: OWNER and MODE.
 **	13-Aug-92 (GordonW)
 **		To make the user's ps happy, fork and setuid so that
 **		this process will show up as envokers uid.
 **	18-Aug-92 (GordonW)
 **		Moved the above logic out to a subroutine.
 **     20-aug-1992 (kevinm)
 **             Added MALLOCLIB as one of the libs needed.  On dg8_us5
 **             the C routine ctime calls malloc.   This causes the memory
 **		allocated through the ME_INGRES_ALLOC to get trashed unless
 **		we use INGRES malloc lib.
 **	14-Oct-92 (GordonW)
 **		We shouldn't have added MALLOCLIB above. We should have removed
 **		reefrences to ME_INGRES_ALLOC. Achilles should be built
 **		like user's application, using Unix malloc.
**	23-Nov-92 (GordonW)
**		Dobn't setup itimers if no IN/KILL enabled.
**		Add a Achilles SUSPENDED message.
**      23-Jul-93 (jeffr)
**		added -i interactive and -a time flags for Version 2.
**	17-Aug-93 (jeffr)
**		move count of total children to this file
**
**	10-Jan-94 (jeffr)
**		added total count on startup for Report
**	10-Jan-94 (jeffr)
**		turned on interrupt flag so that we always call handler 
**		for -a option
**	12-Jan-96 (bocpa01)
**		added NT support (NT_GENERIC)		
**	25-Mar-98 (kinte01)
**		Rename optarg to ach_optarg as symbol is already in use
**		by the DEC C shared library.
**	09-Jun-98 (kinte01)
**		Back out previous change as optarg is a predefined symbol on
**		Unix and I inadvertantly broke the achilles build on Unix
**		platforms. Instead replace the globaldef optarg in 
**		achilles_vms!accompat.c with an	include of unistd.h which
**		will also correct the problem
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define	ACH_VERSION "ACHILLES v2.0"

/*
PROGRAM = achilles
**
NEEDLIBS = ACHILLESLIB LIBINGRES
**
MODE = SETUID
**
OWNER = ROOT
*/

#include <achilles.h>

#define	N_DBMS	10
#define	N_KIDS	20
GLOBALDEF FILE *fptr;

GLOBALDEF HI_RES_TIME       achilles_start;
GLOBALDEF i4        time_to_run = 0;
GLOBALDEF i4	    time_set = 0;

GLOBALDEF char	     tdfile [MAX_LOC] = "config.ach"; /* Configuration file name */
GLOBALDEF char		    *outdir = NULL;
GLOBALDEF char 		    logfspec[MAX_LOC];

GLOBALDEF i4	     numtests;		     /* Number of test types */
GLOBALDEF TESTDESC **tests;		     /* Array of test types */
GLOBALDEF i4         tot_children;          /* Total children on startup */
GLOBALDEF char	    *testpath = NULL;	     /* Opt. location of frontends */
GLOBALDEF char	    *dbname   = NULL;	     /* Optional database name */
GLOBALDEF FILE	    *logfile  = NULL;	     /* Optional log file */
GLOBALDEF FILE	    *envfile  = NULL;	     /* Optional environment file */
#ifndef NT_GENERIC 
GLOBALDEF UID	     uid;		     /* Real user ID of this process */
#endif /* #ifndef NT_GENERIC */

GLOBALDEF char      *maxkids = NULL;         /* Optional number of children */
GLOBALDEF PID	    achpid;                  /* achilles process id  */
GLOBALDEF int	    n_forks = 0;
GLOBALDEF int	    n_dbname;
GLOBALDEF int       silentflg = 0;           /* run in "silent" mode         */
                                             /* discard front-end stdout     */
                                             /* 10-27-89, bls                */
GLOBALDEF int	    interflg = 0; 	    /* test to see if we are in 
					       background or not    */

GLOBALDEF char tembuf[256];

#ifdef NT_GENERIC
usage();
make_outdir ();
#endif /* END #ifdef NT_GENERIC */

main (argc, argv)
int  argc;
char *argv[];
{
    register i4     i,
		    j,
		    k
#ifndef NT_GENERIC
			,
		    pid,
		    oldmask
#endif /* END #ifndef NT_GENERIC */
			;

    i4		    c,
		    waittime = 10;

    HI_RES_TIME	    now;
    ACTIVETEST	    *curtest;
    int		    use_sigs = 0;

    GLOBALREF char  *optarg;
    LOCATION	    logloc
#ifndef NT_GENERIC
					,
					envloc
#endif /* END #ifndef NT_GENERIC */
					;
    char	    tmpfname[64],
		    *basename,
                    logfname[MAX_LOC],
		    timebuf [80];
#ifdef VMS

    char            serverstr[32],              /* Log file name */
                    envfname[MAX_LOC],  /* Environment file name */
                    *server   = NULL,
                    *envfspec = NULL;   /* Parsed environment fspec */

#define	OPTION_STRING "a:d:e:f:hlo:r:s:t:w:"

#else

    STATUS	    status;
    char	    *dbnames[N_DBMS];
    int		    x,
		    n_found;

#define	OPTION_STRING "a:n:d:f:ihlso:r:t:w:" 

#endif

    ACioInit();

    /* Get options from the command line. */
    while ((c = ACgetopt(argc, argv, OPTION_STRING )) != -1)
	switch (c)
	{
#ifdef VMS
	    case 'e':
		envfspec = optarg;
		break;
	    case 's':
		server = optarg;
		break;
#else
	    case 'n':
		maxkids = optarg;
		break;
	    case 's':
		silentflg = 1;
		break;
	     case 'i':
#ifndef NT_GENERIC
		interflg++;
#endif /* END #ifndef NT_GENERIC */
		break;
#endif
	    case 'd':
		dbname = optarg;
		break;
	    case 'f':
		STcopy(optarg,tdfile);
		break;
	    case 'l':
		logfile++;
		break;
	    case 'o':
		outdir = optarg;
		break;
	    case 'r':
		stat_char = *optarg;
		break;
	    case 't':
		testpath = optarg;
		break;
	    case 'w':
		CVan(optarg, &waittime);
		break;
             case 'a':
		time_set=1;
                CVan(optarg, &time_to_run);
                break;	
	    case 'h':
	    default:
		usage();
		break;
    }
#ifdef VMS

    /* If user asked for a specific server, make appropriate modifications
       to the environment variables which will get passed to spawned tests.
    */
    if (server)
	ACsetServer(server);

#else

	/* Make sure we have everything we need ...                 */
	/* 10-29-89, bls                                                    */

	if ( dbname == NULL )
	    usage();

#endif

    /* Make sure all output files will be readable by everyone. */
    PEumask("rwxrwx");
#ifndef NT_GENERIC
    /* Since we're going to be using the current UID fairly often, we'll
       stash it in a global rather than calling getuid() lots of times.
       also grab the pid for achilles logging -- 11/16/89, bls
    */
    
    uid = ACgetuid();
#endif /* #ifndef NT_GENERIC */
    PCpid(&achpid);
    /* Make the directory to hold the output files from the tests. */
    make_outdir(&outdir);

#ifdef VMS

    /* See what environment file has been specified */
    if (envfspec)
    {
	STcopy(envfspec, envfname);
/*** These lines don't create envfile such that acforkchild can use it? ***/
/***	LOfroms(PATH & FILENAME, envfname, &envloc);	***/
/***	if (SIopen(&envloc, "r", &envfile) != OK)	***/
	if ((envfile = fopen(envfname, "r")) == NULL) 
	{
	    SIprintf("Unable to open environment file.\n");
	    envfile = 0;
	} /*** else
	    ACchown(&envloc, uid);	***/

    }

#else
#endif

    /* Read the config file and initialize the array of test types */
    if ((numtests = readfile(tdfile, &tests)) <= 0)
	PCexit(!OK);

    /* If user wants to log to a file, open the file. */
    if (logfile)
    {
	/* If an output file template specified, use it as the
	   base for the log file name */
	if ( *tests[0]->t_outfile )
	{
	    STcopy( tests[0]->t_outfile, logfspec );
	    strcat( logfspec, ".log" );
	}
	else
	    STcopy( "log.ach", logfspec );
	STcopy(outdir, logfname);
	LOfroms(PATH, logfname, &logloc);
	LOfstfile( logfspec, &logloc );
	if (SIopen(&logloc, "w", &logfile) != OK)
	{
	    SIprintf("Unable to open log file.\n");
	    logfile = 0;
	}
#ifndef NT_GENERIC 
	else
	{
	    ACchown(&logloc, uid);
	}
#endif /* #ifndef NT_GENERIC */
    }
    /* set output file pointer to stdout if we're not using a logfile ...
       10-29-89, bls
    */
    else
    {
	logfile = stdout;
    }
    ACsetTerm();

/** always print this to stdout -jeffr **/
if (logfile != stdout) 
  {
    STprintf(tembuf,"%s",logfname);
    SIfprintf(stdout,"\n Note 1: Please -  tail -f  %s \n\t\t\t\t for  <output>\n",tembuf);
 }
#if ! defined VMS && ! defined NT_GENERIC
/** no run-time report on VMS yet **/
if (interflg)
    SIfprintf(stdout,"\n Note 2: Hit <CTRL %c> for Run-Time Report Status\n",'@'+ stat_char);

#endif
	PCsleep(2000);
    /* Set up signal handlers for exiting children and for abort signals. We
       won't set up the alarm handler until after all the initial tests have
       been started, and we'll block these immediately so we don't get
       interrupted partway through starting the tests.
    */

    ACsetupHandlers();

        ACblock(CHILD | STATRPT | ABORT);

    /* Initialize the random number generator which will be used for
       deciding which processes to interrupt/kill at interrupt/kill
       intervals.
    */
    ACinitRandom();

     
#ifdef	xNOT_NEEDED 
    SIfprintf(logfile, "\n     Time           Test Child  Iter.   PID   Action       Code Runtime\n\n");
#endif

    /* Start the tests - for each test type, make the array of test
       instances.  For each instance, figure out the output file name and
       start the first iteration.
       time and date stamp the achilles run -- 11/16/89, bls
    */

        ACgetTime (&now, (LO_RES_TIME *) NULL);
/** save our start time so we can quit if [-actual] specified - jeffr **/
	MEcopy(&now,sizeof(now),&achilles_start);


    ACtimeStr (&now, timebuf);

    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
	       timebuf, ACH_VERSION, achpid, "START");  

/** we will always turn on so that -a flag will work **/
    use_sigs = 1;

    for (i = 0 ; i < numtests ; i++)
    {
     tot_children +=tests[i]->t_nkids;
#ifdef VMS
	if (MEalloc(tests[i]->t_nkids, sizeof(ACTIVETEST),
	    (PTR *) &tests[i]->t_children) != OK)
	    SIprintf("ERROR - MEalloc(%d, %d, ptr) returned %d.\n",
		     tests[i]->t_nkids, sizeof(ACTIVETEST));
#else
	tests[i]->t_children = (ACTIVETEST *) MEreqmem(0,
	  tests[i]->t_nkids*sizeof(ACTIVETEST), TRUE, &status);
	if (status != OK )
	{
	    SIprintf("Can't MEreqmem children\n");
	    PCexit(FAIL);
	}
#endif
	if (*tests[i]->t_outfile)
	    basename = tests[i]->t_outfile;
	else
	    basename = "output";

	for (j = 0 ; j < tests[i]->t_nkids ; j++)
	{
	    char *p;

	    curtest = &(tests[i]->t_children[j]);
	    curtest->a_iters = 1;
	    STcopy(outdir, curtest->a_outfstr);
	    LOfroms(PATH, curtest->a_outfstr, &curtest->a_outfloc);
	    STprintf(tmpfname, "%s_%d_%d", basename, i+1, j+1);
	    LOfstfile(tmpfname, &curtest->a_outfloc);

	    /* The following line _should_ do nothing, but on VAX
	       Ultrix the output file ends up in the current
	       directory (instead of the output directory) without it.
	    */
	    LOtos(&curtest->a_outfloc, &p);

	    /* Copy the generic argv from the parent test type,
	       expanding the <CHILDNUM> variable if necessary.
	    */
	    k = 0;
	    do
	    {
		if ((tests[i]->t_argv[k]) 
		    && (STcompare(tests[i]->t_argv[k],"<CHILDNUM>") == 0))
		{
		    STprintf(curtest->a_childnum, "%d", j);
		    curtest->a_argv[k] = curtest->a_childnum;
		}
		else
		{
#ifdef VMS
		    curtest->a_argv[k] = tests[i]->t_argv[k];
#else
		    n_dbname = -1;
		    if( k == n_dbname )
		    {
			n_found=N_DBMS;
			STgetwords(dbname,&n_found, dbnames);
			if(!n_found)
			    n_found=1;
			for(x=n_found; x<N_DBMS; x++)
			    dbnames[x]=dbnames[x-1];
			x = (tests[i]->t_niters * j)/N_KIDS;
			curtest->a_argv[k]=dbnames[x];
			tests[i]->t_argv[k]=dbnames[x];
		    }
		    else
			curtest->a_argv[k] = tests[i]->t_argv[k];
#endif
		}

	    } while (tests[i]->t_argv[k++]);

	    start_child(i, j, 1);

/** set time to zero in case -a == 0  - jeffr **/
/** made it 0.0F to avoid compiler warning - bocpa01 **/
	   tests[i]->t_ctime = 0.0F;
/** add to total children running - vers 2 - jeffr **/
	    tests[i]->t_ctotal++;
	}
	/* Determine the time to the first interrupt and kill. */
	ACinitTimes(i);
/*
	if( ACintEnabled(i) || ACkillEnabled(i) )
		use_sigs++;

*/
		
    }
    ACreleaseChildren();
    /* Unblock SIGCHLD and the abort signals. */
#ifndef NT_GENERIC
    ACunblock();
#endif /* END #ifndef NT_GENERIC */
    /* Set up the alarm timer. It will initially wait 'waittime' seconds,
       and trigger every 1/4 second after that.
    */					      if( use_sigs )

    	ACinitAlarm(waittime);
    else
    	ACinitAlarm(0);

    /* show that we are waiting */
    ACgetTime (&now, (LO_RES_TIME *) NULL);
    ACtimeStr (&now, timebuf);
    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
               timebuf, "ACHILLES WAIT", achpid, "SUSPENDED");
    SIflush(logfile);
    /* Let the world come to us. */
    ACgetEvents();
#ifdef NT_GENERIC
	return 0;
#endif /* END #ifdef NT_GENERIC */
} /* main */

/*
 * usage() - prints a helpful list of options.
 */

usage ()
{
    SIprintf("Usage: achilles [options]\n");
    SIprintf("Valid options:\n");
    SIprintf("\t-d database_name        (to be passed to INGRES processes)\n");
    SIprintf("\t-f config_file_name\n");
    SIprintf("\t-h                      (print this message)\n");
    SIprintf("\t-l                      (log to machine-read file)\n");
    SIprintf("\t-o output_directory\n");
#ifdef VMS
    SIprintf("\t-s server_id\n");
#else
    SIprintf("\t-s silent_flag          (suppress stdout output)\n");
#endif
    SIprintf("\t-r status_report_char   (defaults to 's')\n");
    SIprintf("\t-t test_path            (location of INGRES test frontends)\n");
    SIprintf("\t-w wait_time            (seconds until first interrupt)\n");
    SIprintf("\t-a time_to_run          (amt. of time to run in hours)\n");
#ifndef VMS
    SIprintf("\t-i interactive		(sets interactive mode)\n");
#endif
    PCexit(!OK);
#ifdef NT_GENERIC
	return 0;
#endif /* END #ifdef NT_GENERIC */
} /* usage */

/*
 * make_outdir() - Invents a name for the output directory, if the user hasn't
 * already specified one. Makes the directory if it doesn't exist. Reports
 * problems, if any.
 */

make_outdir (outdir)
char **outdir;
{
    static char	oddata[32];
    i2		flag;
    char	odstr[MAX_LOC];
    STATUS	rval;
    LOCATION	odloc;

    if (!*outdir)
    {
	*outdir = oddata;
#ifdef VMS
	STprintf(*outdir, "Achilles%05d", ACpidFormat(achpid));
#else
	STprintf(*outdir, "Achilles%05d", achpid );
#endif
    }
    STcopy(*outdir, odstr);
    LOfroms(PATH, odstr, &odloc);
    if (LOisdir(&odloc, &flag) != OK)
    {
	if ((rval = LOcreate(&odloc)) != OK)
	    SIprintf("LOcreate returned %d.\n", rval);
#ifndef NT_GENERIC 
	ACchown(&odloc, uid);
#endif /* #ifndef NT_GENERIC */
    }
    else
    {
	if (flag != ISDIR)
	    printf("%s - not a directory.\n", *outdir);
    }
    SIprintf("Output from frontends is in the %s directory.\n", *outdir);
#ifdef NT_GENERIC 
	return 0;
#endif /* END #ifdef NT_GENERIC */
} /* make_outdir */
#ifndef NT_GENERIC
switch_user()
{
#ifdef	UNIX
    switch(fork())
    {
    case 0:		/* child (setuid) */
	setuid(getuid());
	break;
    case -1:		/* error (ignore) */
	break;
    default:		/* parent (good-bye) */ 
	exit(1);	
	break;
    }
#endif
}
#endif /* END #ifndef NT_GENERIC */
