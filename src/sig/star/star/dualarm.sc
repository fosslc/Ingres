/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** dualarm.sc - alarm checker: look for alarm entries to be executed
**
** History:
**	23-aug-1993 (dianeh)
**		Created (from ingres63p!generic!starutil!duf!dua!dualarm.sc);
**		updated for 6.5 -- most notably, since this stuff now goes in
**		$II_SYSTEM/sig/star, the calls to NMloc() had to be changed to
**		use NMgtAt()'s instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-apr-06 (toumi01)
**	    Check that the wakeup def file is a regular file and, if it is,
**	    that we can clear it.
**	05-apr-06 (drivi01) on behalf of toumi01.
**	    Port change to Windows by simplifying call to LOinfo to return
**	    only the basic file type.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	1-Dec-2010 (kschendel)
**	    Fix compiler warning.
*/
 
/* The scheduler will be running as the ingres SYSTEM administrator but any
   commands that you spawn must only run as the user otherwise you will have
   a big security hole. */
/* Need a quiet mode where only errors are printed, no status messages */
/* Need to print out ALL errors - so maybe SQLPRINT needs to be on? */
 
/*
DB names must be up to 256!
Try run/detach /input=x.com sys$system:dcl.exe
Do 'register as refresh' on all link objects
Cannot be applied to SQL Gateway since issues non-common SQL and
	uses -u flag (without password)
Can work on STAR except if CDB is SQL Gateway for same reasons as above.
Ability to specify command line flags to be used on the connection.
Use standard error message files for log printouts
*/
 
/*
NEEDLIBS = SQLCALIB TBACCLIB LIBQLIB LIBQGCALIB GCFLIB FDLIB FTLIB FEDSLIB FMTLIB UILIB UGLIB AFELIB ADFLIB CUFLIB COMPATLIB MALLOCLIB
 
UNDEFS = IIeqiqio
PROGRAM = scheduler
*/
 
 
/*
** NOTE ABOUT NAMING
**
** The word "alarm" was originally used to describe what this program dealt
** with. Later on the this was changed to the word "schedule".
**
** Therefore you will see a mixture of these ideas.  Currently, the product has 
** a working name of "SQL*SCHEDULER" and the idea is that this product is
** used for scheduling sets of SQL queries to be executed at various times.
**
** This product has been enhanced to schedule the replication of CDBs (where the
** STAR catalogs are kept).
**
** This product will be enhanced to schedule the running of some INGRES
** subsystems (like running reports every monday at 5am).
*/
 
/*
** NOTE ABOUT THE SCHEDULER CATALOGS
**
** The scheduler creates its own catalogs
** 
** The scheduler catalogs should be owned by $ingres.  However this did not work
** out due to permission problems.
**
** Basically, there should be one set of catalogs per database and each user
** must have access to only those rows in the catalogs that belong to them.
** So the catalog is owned by the DBA and there is no permission granted on
** this to anyone.  However, there is a view on top of the catalog with the
** qualification where alarm_user = dbmsinfo('username').
**
** This allows a user to access only those rows in the catalog which they own.
** This is the only way to do this in SQL (in QUEL, one could just define a
** permission like this, however in SQL, one must define a view to get this
** functionality).
**
** The view is given full access to everyone.  In SQL the permission on the
** view is automatically propogated to the base table (when the base table is
** accessed through the view).  HOWEVER, if the base table is owned by $ingres,
*  this permission propogation does not occur.  That is why the catalogs are
** owned by the DBA and not $ingres.
**
** An integrity with the same qualification as above is created on the catalog
** base table so that users may only add rows for themselves.
*/
 
/*
** NOTE ABOUT THE STRUCTURE OF THIS PROGRAM
**
** This program is logically several seperate items which have all been
** combined into one executable for ease of maintenance, porting, shipping, etc.
**
** There are basically three parts:
**	master
**	slave
**	utility
**
** There will typically be several instances of this program running on a system
** with each instance running in a different mode.
**
** The master is started at INGRES startup time.  It checks the master scheduler
** database (iialarmdb) and figures out how many slaves should be started and
** then starts each one.  If a slave dies it simply restarts it.  (Currently
** only one slave is started - the code for multiple slaves is here but not
** working since I could not figure out how to get a detached process to spawn
** subprocesses.)  The master must be run by the INGRES system administrator.
**
** A slave process connects to iialarmdb to figure out what databases it is
** supposed to service and then begins making its 'rounds'.  It checks each
** database for which it is responsible and performs the actions requested by
** any schedules needing servicing.  As it goes through each database it
** remembers the time of the next scheduled event for that database.  After
** servicing all databases the slave sleeps until the next scheduled event
** (unless a 'wakeup' occurs in which case the slave wakes up immediately at
** begins its rounds again).
**
** The slave runs as the INGRES system administrator but uses the '-u' flag
** whenever connecting to a user database.  In this way the the queries in a
** schedule are executed with the appropriate userid and permissions.
** 
** The utility mode of this program is used to review and modify schedules.
** This mode is run directly by the user.
*/
 
exec sql include sqlca;
 
# include	<compat.h>
# include	<dbms.h>
# include	<me.h>
# include	<st.h>
# include	<tm.h>
# include	<pc.h>
# include	<lo.h>
# include	<nm.h>
# include	<cm.h>
# include	<er.h>
# include	<tr.h>
# include	<si.h>
# include	<ut.h>
# include	<cv.h>
 
/* sqlca.sqlcode codes */
#define	ZERO_ROWS	100
#define	DEADLOCK	-4700 	/* use 4700 in osl/sql,
				   osl/quel & equel */
/* various subroutine return codes */
# define NOMOREALARMS	2
# define MOREALARMS	3
# define WAKEUP_EVENT	4
# define STOP_EVENT	5
# define ERROR_EVENT	6
# define NO_DATABASES	7
 
/* maximum number of alarm checker daemons (only one is currently implemented */
# define	ALARM_CHECKERS_MAX	256
struct alarm_chkrs {
	char		id[4];
	PID		pid;
};
struct alarm_chkrs	alarm_checker_arr[ALARM_CHECKERS_MAX];
 
i4	T_quiet;	/* quiet mode */
char	*T_node;	/* node of database currently being processed */
char	*T_dbms;	/* dbms of database currently being processed */
char	*T_alarm_id;	/* alarm currently being processed */
char	*T_alarm_desc;	/* description of alarm currently being processed */
int	*T_query_seq;	/* number of query within the current alarm */
 
char	Instid[10];	/* 2 letter installation id, null terminated */
char	T_curnode[65];	/* virtual node name of the node that this process is
			   currently running on */
bool	T_isterminal;	/* TRUE if stdin is a terminal (i.e., we are not running
			   as detatched or subprocess) */
 
exec sql begin declare section;
    char	*T_alarm_checker_id;	/* id of this alarm checker daemon */
    char	*T_user;		/* user on behalf of who this alarm checker
					   daemon is currently running */
    char	*T_database;	/* database currently being processed */
exec sql end declare section;
 
char	*ERget();
 
main(argc,argv)
int argc;
char **argv;
{
    char	*p;			/* tmp pointer */
    char	vnodelog[33];		/* virtual node name of the node we are
					   running on */
    char	*cnode;			/* tmp pointer */
    LOCATION	logfile;		/* where to log status and error messages */
    char	logfilebuf[MAX_LOC];	/* used with 'logfile' above */
    CL_SYS_ERR	clerrcode;		/* status return for TRset_file call */
    i4		retval;			/* generic "return-value" */
 
    /* needed for frontends */
    MEadvise( ME_INGRES_ALLOC );
 
    T_quiet = 0;	/* noisy */
    T_isterminal = SIterminal(stdin);
 
    /* get installation id and virtual node name to use for
       logfile naming purposes */
    NMgtAt("II_INSTALLATION", &p);
    if (p && *p)
    {
	STcopy(p, Instid);
	Instid[2]=0; /* in case installation ID was big */
    }
    else
	Instid[0] = 0;
 
    /* find out the current node */
    T_curnode[0] = 0;
    if (Instid && *Instid)
    {
	STcopy("II_GCNxx_LCL_VNODE", vnodelog);
	vnodelog[6] = Instid[0];
	vnodelog[7] = Instid[1];
    }
    else
	STcopy("II_GCN_LCL_VNODE", vnodelog);
    NMgtAt(vnodelog, &cnode);
    if (cnode && *cnode)
    {
	STcopy(cnode, T_curnode);
	CVlower(T_curnode);
    }
 
#ifdef hp9_mpe
    (void) STcopy("alarm.log", logfilebuf);
#else
    if (argc == 3 && STcompare(argv[1], "slave") == 0)
	(void) STpolycat(3, "alarm_", argv[2], ".log", logfilebuf);
    else
	(void) STcopy("alarm.log", logfilebuf);
#endif
    /* set up the output logging file */
    if (T_curnode[0])
	(void) STpolycat(3, logfilebuf, "_", T_curnode, logfilebuf);
 
    /* if a detached job then log in the files directory */
    if (T_isterminal == FALSE)
    {
	NMloc(FILES, FILENAME, logfilebuf, &logfile);
	LOcopy(&logfile, logfilebuf, &logfile);
    }
 
    if (TRset_file(TR_F_OPEN , logfilebuf,
	    STlength(logfilebuf), &clerrcode) != OK)
    	PCexit(1);
 
 
    /* figure out what to do and then call the appropriate service routine */
    if (argc == 2 && STcompare(argv[1], "wakeup") == 0)
	wakeup();
    else if (argc == 2 && STcompare(argv[1], "stop") == 0)
	stop();
    else if (argc == 2 && STcompare(argv[1], "master") == 0)
	master();
    else if (argc < 2 && (T_isterminal == FALSE))
	master();
#ifdef unix
    /* on UNIX a special program is used to start up "detached" processes.
       This program always passes one argument, the installationi id, so
       that a 'ps' will show the installation that the process belongs to.
       This is the first 'frontend' program that has been started as a
       daemon - we probably want to invent a CL way of dealing with this. */
    else if (argc == 2 && STcompare(argv[1], Instid) == 0)
	master();
#endif
    else if (argc < 2)
	usage();
    else if (STcompare(argv[1], "add") == 0)
	mastermaint(1, argc, argv);
    else if (STcompare(argv[1], "drop") == 0)
	mastermaint(2, argc, argv);
    else if (STcompare(argv[1], "enable") == 0)
	mastermaint(3, argc, argv);
    else if (STcompare(argv[1], "disable") == 0)
	mastermaint(4, argc, argv);
    else if (STcompare(argv[1], "show") == 0)
	mastermaint(5, argc, argv);
    else if (STcompare(argv[1], "slave") == 0)
    {
	/* 'slave' will return if it found a 'stop' or 'wakeup' event
	   file.   On a 'stop' event, this slave will stop.  On
	   a 'wakeup' event, the 'slave' should
	   go back to the iialarmdb and reread the 
	   information, and then start checking for alarms
	   right away */
	for (;;)
	{
	    if ((retval = slave(argc, argv)) == STOP_EVENT)
	    {
		err_print(0, "Slave 'def' has been stopped\n");
		PCexit(0);
	    }
	    else if (retval == WAKEUP_EVENT)
	    {
		err_print(0, "Slave 'def' has been awakened\n");
		continue;
	    }
	    else if (retval == NO_DATABASES)
	    {
		err_print(0, "Slave 'def' has no databases to service\n");
		err_print(0, "Sleep for 1 hour\n");
		if (goodsleep(60 * 60) == STOP_EVENT)
		{
		    err_print(0, "Slave 'def' has been stopped\n");
		    PCexit(0);
		}
		continue;
	    }
	    else
	    {
		err_print(1, "Slave 'def' had error, sleep for 5 minutes then try again\n");
		if (goodsleep(5 * 60) == STOP_EVENT)
		{
		    err_print(0, "Slave 'def' has been stopped\n");
		    PCexit(0);
		}
	    }
	}
    }
    else if (STcompare(argv[1], "init") == 0)
    {
	/* create the scheduler catalogs */
	PCexit(createtables(argc, argv));
    }
    else if (STcompare(argv[1], "update") == 0)
    {
	/* startup QBF to allow schedules to be browsed or deleted or updated */
	sched_update(argc, argv);
    }
    else
	usage();
 
    PCexit(FAIL);
}
 
sched_update(argc, argv)
int	argc;
char	**argv;
{
    STATUS	status;
    CL_ERR_DESC	clerrv;
    char	*p;
 
    exec sql begin declare section;
	long	tmp;
	char	*dbname;
    exec sql end declare section;
 
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    T_alarm_checker_id = "update";
 
    if (argc != 3)
	usage();
 
    dbname = argv[2];
 
    exec sql connect :dbname;
 
    if (sqlca.sqlcode < 0)
    {
	err_print(1, "Error connecting to %s to check username\n", dbname);
	wakeup();
	return (FAIL);
    }
 
    tmp = 0;
    exec sql select 1 into :tmp from iidbconstants
	    where dbmsinfo('username') = dbmsinfo('dba');
    if (sqlca.sqlcode < 0)
	err_print(1, "Error checking username/dba(1)\n");
 
    if (tmp == 0)
    {
    err_print(1, "Only the dba for this database may run update the catalogs\n");
    err_print(1, "This restriction will probably be removed in the near future\n");
	exec sql disconnect;
	wakeup();
	return (FAIL);
    }
 
    exec sql disconnect;
 
    status = UTexe(UT_WAIT, NULL, NULL, NULL, "qbf", &clerrv,
	    "database = %S, entry = %S", 2, dbname, "iialarm");
    if (status)
    {
	err_print(1, "Error encountered while calling QBF to update catalogs\n");
	p = ERget(status);
	err_print(1, "%s\n", p);
    }
    wakeup();
}
 
createtables(argc, argv)
int	argc;
char	**argv;
{
    int		isstar = 0;
    STATUS	status;
    LOCATION	formfile;
    char	formfilebuf[MAX_LOC];
    char	*p;
    CL_ERR_DESC clerr;
    char	*ptr ;
    char	buf[MAX_LOC] ;


    exec sql begin declare section;
	char	*dbname;
	long	tmp;
	char	dbms_type[33];
    exec sql end declare section;
 
    /*
    ** Create catalogs for either the iialarmdb master database or the
    ** user's database.  
    **
    ** All catalogs are created as owned by the dba (not $ingres).  This
    ** is so the (non-dba) user's can update the catalog for rows that
    ** they own.  If the catalogs were owned by $ingres then "update-system
    ** catalog" permission is required even if the user had permission on
    ** an overlying view.
    */
 
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    T_alarm_checker_id = "init";
 
    if (argc == 3 && STcompare(argv[2], "master") == 0)
    {
 
	err_print(0, "Connecting to master database iialarmdb...\n");
	exec sql connect 'iialarmdb';
 
	if (sqlca.sqlcode < 0)
	{
err_print(1, "Error connecting to iialarmdb (do 'createdb iialarmdb' first)\n");
	    return (FAIL);
	}
 
	tmp = 0;
	exec sql select 1 into :tmp from iidbconstants
	    where dbmsinfo('username') = dbmsinfo('dba');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error checking username/dba(2)\n");
 
	if (tmp == 0)
	{
	  err_print(1, "Only the dba for this database may create the catalogs\n");
	    exec sql disconnect;
	    return (FAIL);
	}
 
	crtabs(1, 0, 1);
 
	exec sql disconnect;

	return (OK);
    }
    else if (argc == 4 && STcompare(argv[2], "user") == 0)
    {
	/*
	** The following two tables are created in any database that wishes to
	** have alarm service.  Each user in a database must have their own
	** set of the following two tables.  This is to ensure security (to
	** prevent one user from entering alarm queries for another)
	*/
 
	dbname = argv[3];
	err_print(0, "Connecting to user database %s...\n", dbname);
	exec sql connect :dbname;
 
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error connecting to database:%s, exiting\n", dbname);
	    return (FAIL);
	}
 
	tmp = 0;
	exec sql select 1 into :tmp from iidbconstants
	    where dbmsinfo('username') = dbmsinfo('dba');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error checking username/dba(3)\n");
 
	if (tmp == 0)
	{
	    err_print(1, "Only the dba for this database may create the catalogs\n");
	    exec sql disconnect;
	    return (FAIL);
	}
 
	dbms_type[0] = 0;
	exec sql select cap_value into :dbms_type from iidbcapabilities
	    where cap_capability = 'DBMS_TYPE';
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error checking dbms_type\n");
 
	if (STcompare(dbms_type, "STAR                            ") == 0)
	    isstar++;
 
	if (isstar)
	{
	    err_print(0, "This is a Distributed Database\n");
	    exec sql commit;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error commiting before direct connect\n");
	    exec sql direct connect;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error doing direct connect\n");
	}
 
	crtabs(0, 0, 1);
 
	/* insert some examples (disabled) */
	err_print(0, "Load examples...\n");
 
	/* first a snapshot copy example */
	exec sql insert into i_user_alarm_settings values (
	    dbmsinfo('username'), 'Snapshot Copy Example',
'Snapshot of onsale is onsale_newyork (which is in another LDB). Do once/week on Sunday at 3:00pm GMT with up to 10 retrys once/hr',
	    '', 0, '1989_01_01 00:00:00 GMT', '1989_06_11 15:00:00 GMT',
	    '', 1, 'week', '1989_06_12 00:00:00 GMT', '', 0, 10, 1, 'hour',
	    '', '', 0, '', 'N');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Could not load examples (master)\n");
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Snapshot Copy Example', 0, 'S', 'S', '', 0,
	    'delete from onsale_newyork', 0, 0);
	if (sqlca.sqlcode < 0)
	    err_print(1, "Could not load examples (detail)\n");
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Snapshot Copy Example', 1, 'S', 'S', '', 0,
'insert into onsale_newyork select * from onsale where state = ''NY''', 0, 0);
 
	/* next, a data rollup example */
	exec sql insert into i_user_alarm_settings values (
	    dbmsinfo('username'), 'Data Rollup Example',
'Rollup worldwide sales data on last day of month (28,29,30 or 31 depending on month). On error, retry indefintely every 4 hours.',
	    '', 0, '1989_01_01 00:00:00 GMT', '1989_01_31 17:00:00 GMT',
	    '', 1, 'month', '1989_06_12 00:00:00 GMT', '', 0, 1000, 4, 'hours',
	    '', '', 0, '', 'N');
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Data Rollup Example', 0, 'S', 'S', '', 0,
	    'drop companysales', 0, 0);
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Data Rollup Example', 1, 'S', 'S', '', 0,
	    'create table companysales as select * from eastcoastsales', 0, 0);
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Data Rollup Example', 2, 'S', 'S', '', 0,
	    'insert into companysales select * from westcostsales', 0, 0);
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Data Rollup Example', 3, 'S', 'S', '', 0,
	    'insert into companysales select * from europeansales', 0, 0);
 
	/* next a replicated CDB example */
	exec sql insert into i_user_alarm_settings values (
	    dbmsinfo('username'), 'Replicated CDB Example',
'Replicate Star Catalogs into "NY_DDB" (with CDB "iiny_ddb"). Replicate every midnight (GMT) with up to 4 retrys once/half-hr',
	    '', 0, '1989_01_01 00:00:00 GMT', '1989_06_12 00:00:00 GMT',
	    '', 1, 'day', '1989_06_12 00:00:00 GMT', '', 0, 4, 30, 'minutes',
	    '', '', 0, '', 'N');
	exec sql insert into i_user_alarm_query_text values (
	    dbmsinfo('username'), 'Replicated CDB Example', 0, 'S', 'B', '', 0,
	    'replicate cdb newyork iiny_ddb ingres', 0, 0);
	exec sql commit;	/* was a bug in STAR that direct disconnect
				   did not commit */
 
	if (isstar)
	{
	    exec sql direct disconnect;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error doing direct disconnect\n");
	    exec sql register i_alarm_settings as link;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error registering i_alarm_settings\n");
	    exec sql register i_user_alarm_settings as link;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error registering i_user_alarm_settings\n");
	    exec sql register i_alarm_query_text as link;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error registering i_alarm_query_text\n");
	    exec sql register i_user_alarm_query_text as link;
	    if (sqlca.sqlcode < 0)
		err_print(1, "Error registering i_user_alarm_query_text\n");
	}
 
	exec sql disconnect;
 
	err_print(0, "Load data entry and update forms...\n");

/* iialarm.frm now lives in II_SYSTEM/ingres/sig/star -- use NMgtAt */
	NMgtAt( ERx( "II_SYSTEM" ), &ptr ) ;
	if( !ptr || !*ptr )
	{
	    SIprintf(ERx("\nII_SYSTEM must be set to the full pathname of\n"));
	    SIprintf(ERx("\nthe directory where INGRES is installed.\n "));
	    SIprintf(ERx("\nPlease set II_SYSTEM and re-run this program.\n"));
	    PCexit( FAIL ) ;
	}
	buf[0] = 0 ;
	STpolycat( 2, ptr, ERx( "ingres/sig/star/iialarm.frm" ), buf ) ;
	LOfroms( PATH&FILENAME, buf, &formfile ) ;

	LOtos(&formfile, &p);
	STcopy(p, formfilebuf);
 
	status = UTexe(UT_WAIT, NULL, (PTR)NULL, (PTR)NULL, 
		       "copyform", &clerr,
	               "inflag = %S, replace = %S, database = %S, file = %S",
	               4, "-i", "-r", dbname, formfilebuf);
	if (status)
	{
err_print(1, "Error encountered while loading data entry and update form\n");
	    p = ERget(status);
	    err_print(1, "%s\n", p);
	}
    }
    else
	usage();
}
 
master()
{
    i4		retval;
    i4		ac_index;
    i4		i;
    PID		pid;
    i4		sargc;
    char	*sargv[10];
    int		status;
    int		sqlca_sqlcode;
    char	*cp;
    char	alarm_id_buf[40];
    char	proc_name[30];
 
    exec sql begin declare section;
	char		alarm_checker_id[4];
	char		criteria_node[33];
	char		criteria_dbms[33];
	char		criteria_database[33];
	char		criteria_user[33];
	long		alarm_priority;
    exec sql end declare section;
 
    T_alarm_checker_id = "master";
 
    /* delete wakeup and stop event files if they were left hanging around
       from some previous incarnation of this program */
    wakeupexists();
    stopexists();
 
	exec sql WHENEVER NOT FOUND CONTINUE ;
	exec sql WHENEVER SQLWARNING CONTINUE;
	exec sql WHENEVER SQLERROR CONTINUE;
 
	/* see if we are the master.  The master enters iialarmdb
	   and figures out the alarm checker assignments and then
	   starts up a subprocess for each alarm checker.  If an
	   alarm checker dies for some reason then the master waits
	   for awhile and then restarts it */
	msg("Master Startup\n");
 
	sqlca.sqlcode = 0;
	err_print(0, "Connecting to iialarmdb...\n");
	exec sql CONNECT 'iialarmdb';
 
	if (sqlca.sqlcode < 0)
	{
		err_print(1, "Could not connect to iialarmdb, exiting\n");
		PCexit(FAIL);
	}
 
	/* initialize i_alarmed_dbs */
	exec sql UPDATE i_alarmed_dbs set alarm_checker_id = 'def';
	if (sqlca.sqlcode < 0)
	{
		err_print(1, "Error initializing i_alarmed_dbs\n");
		exec sql DISCONNECT;
		PCexit(FAIL);
	}
 
#ifdef NOTDEF
for now there will be only one alarm checker until we figure out
how to have multiple ones on VMS (see below).  I could not get
a detached process to spawn off suprocesses.  This can be done
but no VMS it is a lot of work (see PCcmdline code for an example
of how hard this is to do  in VMS land)
 
	exec sql DECLARE i_alarm_checkers_csr CURSOR FOR
	    SELECT DISTINCT alarm_checker_id, criteria_node, criteria_dbms,
		criteria_database, criteria_user, alarm_priority
	    FROM i_alarm_checkers
	    ORDER BY alarm_priority, alarm_checker_id;
 
	exec sql OPEN i_alarm_checkers_csr;
	sqlca_sqlcode = sqlca.sqlcode;
	if (sqlca.sqlcode < 0)
	{
err_print(1, "Error opening i_alarm_checkers cursor - will use default checker\n");
	}
 
	/* for each row in i_alarm_checkers assign this alarm checker
	   to all matching alarm tables.  The effect of this is that
	   the four 'criteria' are ANDed and multiple rows with the
	   same alarm_checker_id are ORed.  If two alarm_checker_id
	   could service the same alarm table, then the alarm_checker_id
	   with the highest priority number will be assigned. */
	for (; sqlca_sqlcode >= 0;)
	{
		exec sql FETCH i_alarm_checkers_csr
		    INTO :alarm_checker_id, :criteria_node, :criteria_dbms,
			:criteria_database, :criteria_user, :alarm_priority;
 
		if (sqlca.sqlcode == ZERO_ROWS)
			break;
		else if (sqlca.sqlcode < 0)
		{
			err_print(1, "Error fetching from i_alarm_checkers\n");
			break;
		}
 
		(void)STtrmwhite(alarm_checker_id);
		/* want to use this name as part of a file name so
		   get rid of any embedded spaces */
		for (cp = alarm_checker_id; *cp; cp++)
			if (*cp == ' ')
				*cp = '_';
		(void)STtrmwhite(criteria_node);
		(void)STtrmwhite(criteria_dbms);
		(void)STtrmwhite(criteria_database);
		(void)STtrmwhite(criteria_user);
 
		exec sql UPDATE i_alarmed_dbs set alarm_checker_id =
			:alarm_checker_id
		    WHERE
			alarm_node like :criteria_node and
			alarm_dbms like :criteria_dbms and
			alarm_database like :criteria_database and
			alarm_user like :criteria_user;
		if (sqlca.sqlcode < 0)
		{
			err_print(1, "Error updating i_alarmed_dbs\n");
			break;
		}
	}
 
	if (sqlca_sqlcode >= 0)
	{
	    exec sql CLOSE i_alarm_checkers_csr;
	}
 
	/* determine all the alarm checkers that need to be started.
	   This list may include the checker "iidefault_alarm_checker" */
	ac_index = 0;
	exec sql SELECT DISTINCT alarm_checker_id INTO :alarm_checker_id
		from i_alarmed_dbs
		where (alarm_enabled = 'y' or alarm_enabled = 'Y');
	exec sql BEGIN;
		if (ac_index >= ALARM_CHECKERS_MAX)
		{
		err_print(1, "More than %d alarm checkers, extras ignored\n");
			exec sql endselect;
		}
		STcopy(alarm_checker_id, alarm_checker_arr[ac_index].id);
		alarm_checker_arr[ac_index].pid = 0;
		ac_index++;
	exec sql END;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error selecting alarm_checker list\n");
#endif
 
	err_print(0, "Disconnecting from iialarmdb...\n");
	exec sql DISCONNECT;
 
	if (sqlca.sqlcode < 0)
	{
	err_print(1, "Error while disconnecting from iialarmdb, exiting\n");
		PCexit(FAIL);
	}
 
 
	sargv[0] = "iialarmchk";
	sargv[1] = "slave";
	sargv[2] = "def";
	for (;;)
	{
	    if ((retval = slave(3, sargv)) == STOP_EVENT)
	    {
		err_print(0, "Slave 'def' has been stopped\n");
		PCexit(0);
	    }
	    else if (retval == WAKEUP_EVENT)
	    {
		err_print(0, "Slave 'def' has been awakened\n");
		continue;
	    }
	    else if (retval == NO_DATABASES)
	    {
		err_print(0, "Slave 'def' has no databases to service\n");
		err_print(0, "Sleep for 1 hour\n");
		if (goodsleep(60 * 60) == STOP_EVENT)
		{
		    err_print(0, "Slave 'def' has been stopped\n");
		    PCexit(0);
		}
		continue;
	    }
	    else
	    {
		err_print(1, "Slave 'def' had error, sleep for 5 minutes then try again\n");
		if (goodsleep(5 * 60) == STOP_EVENT)
		{
		    err_print(0, "Slave 'def' has been stopped\n");
		    PCexit(0);
		}
	    }
	}
 
#ifdef WONT_WORK_ON_VMS
/*
basically it seems that a detached process may now spawn subprocesses.
The detached process could create other detached processes (I think).
*/
 
	/* now start up each alarm checker.  Periodically check to see if
	   any have died and if they have restart them. */
	for (;;)
	{
		for(i = 0; i < ac_index; i++)
		{
			pid = alarm_checker_arr[i].pid;
 
			if (pid) 
			{
				/* see if the process is still around */
				if (pid_probe(pid) == FAIL)
				{
					alarm_checker_arr[i].pid = pid = 0;
		msg("iialarmchk with id:%s failed, restarting\n");
				}
			}
 
			/* if 0 then start alarm checker */
			if (pid == 0)
			{
			    sargc = 3;
			    sargv[0] = "scheduler ";
			    sargv[1] = "slave ";
 
			    /* quote this arg to keep its case correct */
			    (void)STpolycat(3, "\"", alarm_checker_arr[i].id,
				"\" ", alarm_id_buf);
			    sargv[2] = alarm_id_buf;
 
			    /* process name will be 'iialarm_' +
			       installation_id + alarm_checker_id */
			    STcopy("iialarm_", proc_name);
			    if (Instid && *Instid)
				STpolycat(3, proc_name, Instid, "_", proc_name);
			    STcat(proc_name, alarm_checker_arr[i].id);
 
			    err_print(0, "Spawning alarm checker:%s%s%s\n",
				sargv[0], sargv[1], sargv[2]);
/*
sargv[0]="@ii_system:[ingres.utility]iirunscd ";
sargv[1]=alarm_id_buf;
sargc=2;
err_print(0, "Spawning %s%s\n", sargv[0], sargv[1]);
			    status = PCnspawn(sargc, sargv, (i1)1, (char *) 0,
				(char *) 0, &pid, proc_name);
*/
status = PCcmdline(NULL,"@ii_system:[ingres.utility]iirunscd dog");
			    if (status != OK)
			msg("Cannot spawn iialarmchk with id:%s\n",
				alarm_checker_arr[i].id);
			    else
				alarm_checker_arr[i].pid = pid;
			}
		}
		/* for now just break.  When pid_probe is written then
		   we can sleep here, and then periodically wake up to
		   see if any slaves have crashed
		goodsleep(3600L); */
	
		break;
	}
 
#endif
	PCexit(OK);
}
 
# define ALARM_TABLE_MAX	1024
struct alarm_tbls {
    char	alarm_node[33];
    char	alarm_dbms[33];
    char	alarm_database[33];
    char	alarm_user[33];
    long	next_time;
    int		alarm_table_enabled;
};
struct alarm_tbls	i_alarmed_dbs[ALARM_TABLE_MAX];
 
/* for each database for which this slave has responsibility (by 
   checking the alarm assignments in database iialarmdb), connect
   to the database and process any alarms.  When done, sleep for
   a reasonable period of time calculated by remembering the time
   of the next alarm for each of the databases */
slave(argc, argv)
int	argc;
char	**argv;
{
    i4		at_index;
    i4		i;
    long	next_time;
    long	min_time;
    long	next_alarm_seconds;
    SYSTIME	stime;
    i4		test_mode = 0;
    char	*test_id;
    i4		retval;
    i4		wakeup;
 
    exec sql begin declare section;
	long	alarm_sleep_seconds;
	char	alarm_node[33];
	char	alarm_dbms[33];
	char	alarm_database[33];
	char	alarm_user[33];
	char	dbname[100];
    exec sql end declare section;
 
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    if (argc < 3)
	usage();
 
    if (STcompare(argv[1], "test") == 0)
    {
	test_mode = 1;
	T_alarm_checker_id = "test";
    	if (argc != 7)
 	{
	    usage();
	    PCexit(FAIL);
	}
    }
    else if (STcompare(argv[1], "slave") == 0)
    {
	(void)STtrmwhite(argv[2]);
	T_alarm_checker_id = argv[2];
    }
    else
	usage();
 
    err_print(0, "Slave alarm checker startup...\n");
 
    if (test_mode)
    {
	STcopy(argv[5], i_alarmed_dbs[0].alarm_node);
	CVupper(argv[4]);
	STcopy(argv[4], i_alarmed_dbs[0].alarm_dbms);
	STcopy(argv[3], i_alarmed_dbs[0].alarm_database);
	STcopy(argv[2], i_alarmed_dbs[0].alarm_user);
	i_alarmed_dbs[0].next_time = 0;
	i_alarmed_dbs[0].alarm_table_enabled = 1;
	if (!CMnmstart(argv[2]))
	{
		err_print(1, "Illegal user name:%s, exiting\n", argv[2]);
		PCexit(FAIL);
	}
	else if (!CMnmstart(argv[3]))
	{
		err_print(1, "Illegal database name:%s, exiting\n", argv[3]);
		PCexit(FAIL);
	}
	else
		at_index = 1;
    }
    else
    {
 
    /* connect to iialarmdb and read in all entries in i_alarmed_dbs
       to be serviced by this alarm checker. */
 
    err_print(0, "Connecting to iialarmdb...\n");
    exec sql connect iialarmdb;
 
    if (sqlca.sqlcode < 0)
    {
	err_print(1, "Error connecting to iialarmdb, exiting\n");
	return (ERROR_EVENT);
    }
 
    at_index = 0;
    exec sql select alarm_node, alarm_dbms, alarm_database, alarm_user
	into :alarm_node, :alarm_dbms, :alarm_database, :alarm_user
	from i_alarmed_dbs
	where alarm_checker_id = :T_alarm_checker_id
	    and (alarm_enabled = 'y' or alarm_enabled = 'Y');
    exec sql begin;
	if (at_index >= ALARM_TABLE_MAX)
	{
		err_print(1, "More than %d alarm table entries, extras ignored\n",
			ALARM_TABLE_MAX);
		exec sql endselect;
	}
	(void)STtrmwhite(alarm_node);
	STcopy(alarm_node, i_alarmed_dbs[at_index].alarm_node);
 
	(void)STtrmwhite(alarm_dbms);
	CVupper(alarm_dbms);
	STcopy(alarm_dbms, i_alarmed_dbs[at_index].alarm_dbms);
 
	(void)STtrmwhite(alarm_database);
	STcopy(alarm_database, i_alarmed_dbs[at_index].alarm_database);
 
	(void)STtrmwhite(alarm_user);
	STcopy(alarm_user, i_alarmed_dbs[at_index].alarm_user);
	i_alarmed_dbs[at_index].next_time = 0;
	i_alarmed_dbs[at_index].alarm_table_enabled = 1;
 
	if (!CMnmstart(alarm_user))
		err_print(1, "Illegal user name:%s, skipping this entry\n");
	else if (!CMnmstart(alarm_database))
		err_print(1, "Illegal database name:%s, skipping this entry\n");
	else
	{
		err_print(0, "Will service %s::%s/%s, user:%s\n", alarm_node,
			alarm_database, alarm_dbms, alarm_user);
		at_index++;
	}
    exec sql end;
    if (sqlca.sqlcode < 0)
	err_print(1, "Error selecting alarms for this alarm_checker\n");
 
    err_print(0, "Exiting from iialarmdb...\n");
    exec sql disconnect;
    if (sqlca.sqlcode < 0)
    {
	err_print(1, "Error disconnecting from iialarmdb, exiting\n");
	return (ERROR_EVENT);
    }
    } /* end of 'else test_mode' */
 
    if (at_index == 0)
    {
	msg("No alarm tables for this alarm checker.  Exiting...\n");
	return (NO_DATABASES);
    }
 
    /* For each alarmed database connect to the database and then
       call alarm_check */
    for (;;)
    {
	exec sql WHENEVER NOT FOUND CONTINUE ;
	exec sql WHENEVER SQLWARNING CONTINUE;
	exec sql WHENEVER SQLERROR CONTINUE;
 
	TMnow(&stime);
	min_time = stime.TM_secs + (60 * 60);
	for (i = 0;i < at_index; i++)
	{
	    TMnow(&stime);
 
	    /* skip this database if there are no alarms */
	    if (!i_alarmed_dbs[i].alarm_table_enabled)
		continue;
		
	    /* skip this database if there are no alarms ready to be
	       services - but remember the next one in this database
	       to be serviced */
	    if ((next_time = i_alarmed_dbs[i].next_time) > stime.TM_secs)
	    {
		if (next_time < min_time)
		    min_time = next_time;
		continue;
	    }
 
	    /* set up some globals for error reporting */
	    T_node = i_alarmed_dbs[i].alarm_node;
	    T_dbms = i_alarmed_dbs[i].alarm_dbms;
	    T_database = i_alarmed_dbs[i].alarm_database;
	    T_user = i_alarmed_dbs[i].alarm_user;
	    T_alarm_id = (char *) 0;
	    T_alarm_desc = (char *) 0;
	    T_query_seq = 0;
 
	    /* If DBMS is blank or INGRES do not use '/dbms' suffix since
	       there is a bug in DBMS server.  Always use the "-uusername"
	       flag. */
	    dbname[0] = 0;
	    (void) STpolycat(2, T_node, "::", dbname);
	    (void) STcat(dbname, T_database);
	    if (T_dbms[0] && STcompare(T_dbms, "INGRES"))
		(void) STpolycat(3, dbname, "/", T_dbms, dbname);
 
	    sqlca.sqlcode = 0;
	    slave_print(0, "Connecting to '%s', user:%s...\n", dbname, T_user);
 
	    exec sql CONNECT :dbname IDENTIFIED BY :T_user;
	    if (sqlca.sqlcode < 0)
	    {
		slave_print(1, "Error connecting, skip this database\n");
		continue;
	    }
 
	    if (test_mode)
		test_id = argv[6];
	    else
		test_id = 0;
 
	    /* check alarms */
	    retval = alarm_check(&next_alarm_seconds, test_mode, test_id);
	    if (retval == NOMOREALARMS)
		i_alarmed_dbs[i].alarm_table_enabled = 0;
	    else
	    {
		/* remember when the next alarm will be ready */
		TMnow(&stime);
		i_alarmed_dbs[i].next_time = next_time = stime.TM_secs
		    + next_alarm_seconds;
 
		if (next_time < min_time)
		    min_time = next_time;
	    }
 
	    sqlca.sqlcode = 0;
	    slave_print(0, "Disconnecting from '%s', user:%s...\n",
		dbname, T_user);
	    exec sql DISCONNECT ;
	    if (sqlca.sqlcode < 0)
	    {
		slave_print(1, "Error disconnecting, exiting\n");
		return (retval);
	    }
	}
 
	if (test_mode)
	{
	    slave_print(0, "Testing complete, exiting\n");
	    return (STOP_EVENT);
	}
 
	/* if woken up by wakeup file then return and get called
	   from the top (go back and check iialarmdb */
	if (retval == WAKEUP_EVENT || retval == STOP_EVENT)
	    return (retval);
 
	/* now go to sleep until approximately the next alarm would
	   be fired */
	TMnow(&stime);
	min_time = min_time - stime.TM_secs;
	if (min_time > 0)
	{
	    sqlca.sqlcode = 0;
	    err_print(0, "Go to sleep for %d seconds...\n", min_time);
	    wakeup = goodsleep(min_time);
	    err_print(0, "Waking up from %d second sleep...\n", min_time);
 
	    /* if woken up by wakeup file then return and get called
	       from the top (go back and check iialarmdb */
	    if (wakeup == WAKEUP_EVENT || wakeup == STOP_EVENT)
		return (wakeup);
	}
    }
    return (WAKEUP_EVENT);
}
 
# define	QUERYBUF_SIZE	4096
# define	SQL_TYPE	0
# define	BUILTIN_TYPE	1
# define	REPORT_TYPE	2
 
/* check alarms in the alarm table in <T_node, T_dbms, T_database, T_user> */
alarm_check(next_in_seconds, test_mode, test_id)
long	*next_in_seconds;
int	test_mode;
char	*test_id;
{
    i4		stop_on_error;
    i4		alarm_aborted;
    i4		query_type;
 
    exec sql begin declare section;
	char	querybuf[QUERYBUF_SIZE + 1];	/* Accumulate query text here */
	char	alarm_id[33];
	char	talarm_id[33];
	char	alarm_desc[129];
	long	priority;
	long	tpriority;
	float	overdue;
	float	toverdue;
	float	fmin;
	int	query_seq;
	char	failure_action[2];
	char	text_dml[2];
	long	text_sequence;
	char	text_segment[1951];
	long	iialarmchk_error;
	long	sqlca_sqlcode;
	long	query_size;
	long	len;
	long	next_alarm_seconds;
	short	next_alarm_null;
	long	guess1;
	long	guess2;
	long	guess3;
	long	guess_interval_next;
	long	retry_enabled;
	long	tretry_enabled;
	long	retry_number;
	long	tretry_number;
	char	interval_units[10];
	long	interval_num;
	char	interval1[64];
	char	interval2[64];
	char	interval3[64];
	char	*ip;
	long	days;
	long	hours;
	long	minutes;
	long	seconds;
	long	secs_per_unit;
    exec sql end declare section;
 
	T_alarm_id = alarm_id;
	T_alarm_desc = alarm_desc;
	T_query_seq = &query_seq;
	
	alarm_id[0] = 0;
	alarm_desc[0] = 0;
	query_seq = 0;
 
	exec sql WHENEVER NOT FOUND CONTINUE ;
	exec sql WHENEVER SQLWARNING CONTINUE;
	exec sql WHENEVER SQLERROR CONTINUE;
 
# ifdef NOTDEF
STAR has bug with sort clause 'overdue'
/*	exec sql DECLARE i_user_alarm_settings_csr CURSOR FOR */
	    SELECT DISTINCT alarm_id, alarm_priority, retry_enabled,
		    retry_number, overdue =
		interval('seconds', date('now') -
		    date(ifnull(next_time, start_time)))
	    FROM i_user_alarm_settings
	    WHERE
		(alarm_enabled = 'Y' or alarm_enabled = 'y')
		and date('now') >= date(ifnull(next_time, start_time))
	    ORDER BY alarm_priority, overdue desc;
# endif
 
	exec sql update i_user_alarm_settings set next_time = start_time
		where next_time is null;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error changing null next_time's\n");
	    return (NOMOREALARMS);
	}
 
	/* for each alarm_id that needs to be executed */
	for (;;)
	{
	    if (wakeupexists())
		return (WAKEUP_EVENT);
	    else if (stopexists())
		return (STOP_EVENT);
 
		alarm_id[0] = 0;
		alarm_desc[0] = 0;
		query_seq = 0;
 
		exec sql WHENEVER NOT FOUND CONTINUE ;
		exec sql WHENEVER SQLWARNING CONTINUE;
		exec sql WHENEVER SQLERROR CONTINUE;
 
#ifdef NOTDEF
		if (!test_mode)
		{
/*		    exec sql OPEN i_user_alarm_settings_csr; */
		}
#endif
 
		if (!test_mode && sqlca.sqlcode < 0)
		{
	slave_print(1, "Error opening alarm_setting cursor, exit database\n");
			return (NOMOREALARMS);
		}
 
		if (test_mode)
		{
		    STcopy(test_id, alarm_id);
		}
		else
		{
		    fmin = 1e10;
 
		    exec sql SELECT DISTINCT alarm_id, alarm_priority, retry_enabled,
		        retry_number, overdue = interval('seconds', date('now') -
		        date(ifnull(next_time, start_time)))
		    into :talarm_id, :tpriority, :tretry_enabled, :tretry_number, :toverdue
		    FROM i_user_alarm_settings
		    WHERE
			(alarm_enabled = 'Y' or alarm_enabled = 'y')
			and date('now') >= date(ifnull(next_time, start_time))
		    ORDER BY alarm_priority;
		    exec sql begin;
			/* find minimum - normally there would just be an 'exec sql endselect;'
			   here and the orderby clause would be 'alarm_priority, overdue desc'.
			   However, the 'overdue desc' causes an AV in the STAR server. */
			if (toverdue < fmin)
			{
			    fmin = toverdue;
			    STcopy(talarm_id, alarm_id);
			    priority = tpriority;
			    retry_enabled = tretry_enabled;
			    retry_number = tretry_number;
			    overdue = toverdue;
			}
		    exec sql end;
 
/*		    exec sql FETCH i_user_alarm_settings_csr INTO :alarm_id,
			:priority, :retry_enabled, :retry_number, :overdue;*/
 
		    if (sqlca.sqlcode < 0)
		    {
	slave_print(1, "Error fetching alarm_setting cursor, exit database\n");
/*			exec sql close i_user_alarm_settings_csr; */
			return (NOMOREALARMS);
		    }
 
		    if (sqlca.sqlcode == ZERO_ROWS)
		    {
			exec sql commit work;
			if (sqlca.sqlcode < 0)
			slave_print(1, "Error committing when no more alarms\n");
slave_print(0, "No pending entries in i_user_alarm_settings: commit work and exit database\n");
			break ;
		    }
		}
		slave_print(0, "Begin processing this alarm\n");
 
		alarm_aborted = 0;
 
		/* process each query for this alarm_id */
		for (query_seq = 0; ;query_seq++)
		{
			/* get all pieces of the query */
			query_size = 0;
			iialarmchk_error = 0;
		        exec sql select distinct text_dml, query_seq,
				failure_action, text_sequence, text_segment
			    into
				:text_dml, :query_seq, :failure_action,
				:text_sequence, :text_segment
			    from i_user_alarm_query_text
			    where alarm_id = :alarm_id
				 and query_seq = :query_seq
			    order by text_sequence;
			exec sql begin;
			    len = STlength(text_segment);
 
			    /* leave room for trailing null */
			    if (len + query_size >= QUERYBUF_SIZE)
			    {
			     slave_print(1, "Query too large; limit is %d bytes\n",
				    QUERYBUF_SIZE);
				exec sql endselect;
			    }
			    MEcopy(text_segment, (u_i2)len,
				&querybuf[query_size]);
			    query_size += len;
			exec sql end;
 
			if (STcompare(text_dml, "S") == 0)
			    query_type = SQL_TYPE;
			else if (STcompare(text_dml, "B") == 0)
			    query_type = BUILTIN_TYPE;
			else if (STcompare(text_dml, "R") == 0)
			    query_type = REPORT_TYPE;
			else
			{
			    slave_print(1, "Can only handle S-SQL, B-Builtin, R-Report Writer\n");
			    iialarmchk_error = 2;
			}
 
			if (sqlca.sqlcode < 0)
			    slave_print(1, "Error retrieving query text\n");
			if (sqlca.sqlcode == ZERO_ROWS)
			    break;
 
			if (query_size == 0)
			{
				slave_print(1, "Zero length query\n");
				iialarmchk_error = 3;
			}
 
			stop_on_error = (failure_action[0] == 'S');
			sqlca_sqlcode = 0;
			if (!iialarmchk_error)
			{
			    querybuf[query_size] = 0;
			    if (query_type == BUILTIN_TYPE)
			    {
				slave_print(0, "Executing Builtin:%s\n", querybuf);
				iialarmchk_error =builtin(querybuf, query_size);
				if (iialarmchk_error)
				    slave_print(1, "Error executing builtin\n");
			    }
			    else if (query_type == REPORT_TYPE)
			    {
				slave_print(0, "Executing report command:%s\n", querybuf);
				iialarmchk_error = callreport(querybuf);
				if (iialarmchk_error)
				    slave_print(1, "Error executing report command\n");
			    }
			    else
			    {
				slave_print(0, "Executing:%s\n", querybuf);
				exec sql execute immediate :querybuf;
				sqlca_sqlcode = sqlca.sqlcode;
				if (sqlca.sqlcode < 0)
				    slave_print(1, "Error executing query\n");
			    }
			}
 
			if (stop_on_error && (iialarmchk_error ||
				sqlca_sqlcode))
			    alarm_aborted = 1;
 
			if (alarm_aborted)
			{
		slave_print(1, "Failure code is 'S'.  Aborting this alarm\n");
			    exec sql rollback;
			    if (sqlca.sqlcode < 0)
				slave_print(1, "Error executing rollback\n");
			}
 
			/* store the error code */
			exec sql update i_user_alarm_query_text
			    set
				iialarmchk_error = :iialarmchk_error,
				sqlca_sqlcode = :sqlca_sqlcode
			    where alarm_id = :alarm_id
				and query_seq = :query_seq
				and text_sequence = 0;
			if (sqlca.sqlcode < 0)
			    slave_print(1, "Error storing error codes\n");
 
			if (alarm_aborted)
			{
			    /* Alarm has failed.  If this was already a
			       retry from previous failure then bump retry
			       count.  Otherwise setup the first retry if the
			       user has specified retry information */
			    if (retry_enabled)
			    {
				retry_enabled++;
				exec sql update i_user_alarm_settings
				    set retry_enabled = :retry_enabled
				    where alarm_id = :alarm_id;
				if (sqlca.sqlcode < 0)
			    slave_print(1, "Error incrementing retry_enabled\n");
				else
				    slave_print(0, "Retry count incremented\n");
			    }
			    else if (retry_number)
			    {
				exec sql update i_user_alarm_settings
				    set
					save_start_time = start_time,
					save_interval_num = interval_num,
					save_interval_units = interval_units,
					retry_enabled = 1,
					start_time = date_gmt(date('now')),
					interval_num = retry_interval_num,
					interval_units = retry_interval_units
				    where alarm_id = :alarm_id;
				if (sqlca.sqlcode < 0)
				    slave_print(0, "Error enabling retry mode\n");
				else
				    slave_print(0, "Retry mode is now enabled\n");
			    }
			    break;
			}
		}
 
		/* Turn off retry mode if alarm was successful or if
		   retry count was exceeded */
		if (retry_enabled &&
		    (!alarm_aborted || (retry_enabled > retry_number)))
		{
		    exec sql update i_user_alarm_settings
			set
			    start_time = save_start_time,
			    interval_num = save_interval_num,
			    interval_units = save_interval_units,
			    retry_enabled = 0
			where alarm_id = :alarm_id;
		    if (sqlca.sqlcode < 0)
			slave_print(1, "Error disabling retry mode\n");
		    else if (alarm_aborted)
		slave_print(0, "Retry count exceeded, retry mode now disabled\n");
		    else
		slave_print(0, "Alarm successful, retry mode now disabled\n");
		}
 
		/* calculate the next time to schedule the alarm.  This
		   is tricky when the interval_unit is months. */
		exec sql select interval_units, interval_num,
		    guess_interval_next =
			int4
			(
			    interval
			    (
				interval_units,
				date('now') - date(start_time)
			    )
			    /
			    interval_num
			) + 1
		    into :interval_units, :interval_num, :guess_interval_next
		    from i_user_alarm_settings
		    where alarm_id = :alarm_id;
		if (sqlca.sqlcode < 0)
		   slave_print(1, "Error selecting initial next interval guess\n");
 
		/* this should never happen if units are months or greater - if months
		   or greater than this scheme would not work since months are not precise
		   and also updating start_time that originally was on jan 31 to a Feb 28
		   would then never again get 30th and 31st dates */
 
		exec sql select secs_per_unit =
			int4(interval('seconds', '1 ' + interval_units) + .1)
		    into :secs_per_unit
		    from i_user_alarm_settings
		    where alarm_id = :alarm_id;
		if (sqlca.sqlcode < 0)
		    slave_print(1, "Error computing secs_per_unit\n");
 
		build_interval((guess_interval_next-1) * interval_num,
		    secs_per_unit, interval1);
		build_interval((guess_interval_next) * interval_num,
		    secs_per_unit, interval2);
		build_interval((guess_interval_next+1) * interval_num,
		    secs_per_unit, interval3);
		    
		/* figure out the next time an alarm should occur */
		exec sql select
			guess1 = interval('seconds', (date(start_time) +
			    :interval1) - date('now')),
			guess2 = interval('seconds', (date(start_time) +
			    :interval2) - date('now')),
			guess3 = interval('seconds', (date(start_time) + 
			    :interval3) - date('now'))
		    into :guess1, :guess2, :guess3
		    from i_user_alarm_settings
		    where alarm_id = :alarm_id;
		if (sqlca.sqlcode < 0)
		   slave_print(1, "Error selecting three next interval guesses\n");
 
		/* use the smallest guess_interval_next value that yields
		   a next scheduled alarm time after right now */
		if (guess1 > 0)
		    ip = interval1;
		else if (guess2 > 0)  /* This should be the the case except for
					  some boundary cases for months and
					  years */
		    ip = interval2;
		else if (guess3 >= 0)
		    ip = interval3;
		else 
		    /* should probably disable this alarm */
		slave_print(1, "Cannot compute next sched interval:%d:%d:%d\n\n",
			guess1, guess2, guess3);
 
		/* update the next_time and last_time_run fields */
		exec sql update i_user_alarm_settings set
			last_time_run = date_gmt(date('now')),
			next_time = date_gmt(date(start_time) + :ip)
		    where alarm_id = :alarm_id;
		if (sqlca.sqlcode < 0)
		   slave_print(1, "Error setting next_time\n");
 
		/* we have complete this alarm_id so commit everything
		   (don't want to hold locks too long or have transactions
		   that are too big) */
		exec sql commit work;	
		if (sqlca.sqlcode < 0)
		   slave_print(1, "Error commiting for current alarm_id\n");
 
		/* Do not confuse user if alarm queries were aborted */
		query_seq = 0;
		if (!alarm_aborted)
		    slave_print(0, "WORK COMMITTED\n");
 
		alarm_id[0] = 0;   /* so does not come out in logging msgs*/
	
		if (test_mode)
			return (NOMOREALARMS);
	}
 
	if (wakeupexists())
	    return (WAKEUP_EVENT);
	else if (stopexists())
	    return (STOP_EVENT);
 
	/* find number of seconds until next alarm, could be negative
	   if there is an alarm that is ready to be serviced right now */
	exec sql SELECT DISTINCT next_alarm_seconds =
		min(interval('seconds', date(ifnull(next_time, start_time)) -
		    date('now')))
	    INTO :next_alarm_seconds:next_alarm_null	/* truncate float into integer is OK */
	    FROM i_user_alarm_settings
	    WHERE (alarm_enabled = 'Y' or alarm_enabled = 'y');
 
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error determining seconds to next alarm - use 1 hr\n");
	    next_alarm_seconds = 60 * 60;
	}
	/* notice that 'min' returns null if no rows meet the qualification */
	else if (sqlca.sqlcode == ZERO_ROWS || next_alarm_null == -1)
	{
slave_print(0, "No more alarms for this database, database checking disabled\n");
	    return(NOMOREALARMS);
	}
 
	*next_in_seconds = next_alarm_seconds;
	return (MOREALARMS);
}
 
slave_print(type, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
i4	type;
char	*fmt, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
    exec sql begin declare section;
		char	err_msg[257];
    exec sql end declare section;
 
    SYSTIME	stime;
    char	timestr[64];
    char	alarm_buf[33];
    char	*cp;
 
    /* if quiet mode and this is just a status message then ignore */
    if (T_quiet && type == 0)
	return;
 
    /* if both status and error messages are being displayed then
	print out an indicator to help differentiate the messages */
    if (!T_quiet && type)
	cp = " ***";
    else
	cp = "";
 
    TMnow(&stime);
    TMstr(&stime, timestr);
 
    TRdisplay("%s::[iischeduler_%s, '%s']: %s%s\n", T_curnode, Instid,
	T_alarm_checker_id, timestr, cp);
    TRdisplay("    %s::%s/%s, user:%s\n", T_node, T_database, T_dbms, T_user);
 
    if (T_alarm_id && *T_alarm_id)
    {
	(void)STcopy(T_alarm_id, alarm_buf);
	(void)STtrmwhite(alarm_buf);
	TRdisplay("    alarm_id:%s, query_seq:%d\n", alarm_buf, *T_query_seq);
    }
 
    if (sqlca.sqlcode < 0)
    {
	    exec sql COPY SQLERROR INTO :err_msg WITH 256 ;
	    err_msg[256] = (char)0;
	    msg("    %s", err_msg);
	    /* some error messages do not have terminating newlines */
	    if (err_msg[STlength(err_msg)-1] != '\n')
		msg("\n");
	    sqlca.sqlcode = 0;
    }
 
    msg("    ");
    msg(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
}
 
err_print(type, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
i4	type;
char	*fmt, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
    exec sql begin declare section;
		char	err_msg[257];
    exec sql end declare section;
 
    SYSTIME	stime;
    char	timestr[64];
    char	*cp;
 
    /* if quiet mode and this is just a status message then ignore */
    if (T_quiet && type == 0)
	return;
 
    TMnow(&stime);
    TMstr(&stime, timestr);
 
    /* if both status and error messages are being displayed then
	print out an indicator to help differentiate the messages */
    if (!T_quiet && type)
	cp = " ***";
    else
	cp = "";
 
    TRdisplay("%s::[iischeduler_%s, '%s']: %s%s\n", T_curnode, Instid,
	T_alarm_checker_id, timestr, cp);
 
    if (sqlca.sqlcode < 0)
    {
	    exec sql COPY SQLERROR INTO :err_msg WITH 256 ;
	    err_msg[256] = (char)0;
	    msg("    %s", err_msg);
	    /* some error messages do not have terminating newlines */
	    if (err_msg[STlength(err_msg)-1] != '\n')
		msg("\n");
	    sqlca.sqlcode = 0;
    }
 
    msg("    ");
    msg(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
}
 
/* When we figure out how to check for the existence of the given process
   id, then put it in this routine.  For now the master will not be able
   to restart a slave that crashes */
pid_probe(pid)
i4	pid;
{
    return (OK);
}
 
usage()
{
	msg("Usage: scheduler master\n");
/*	msg("       scheduler slave <alarm_checker_id>\n");*/
/*msg("       scheduler test <usr> <database> <dbms> <node> <alarm_id>\n");*/
	msg("       scheduler init master\n");
msg("       scheduler init user [<node>::]<database>[/<dbms>]\n");
msg("       scheduler update [<node>::]<database>[/<dbms>]\n");
/*
	msg("       scheduler wakeup <alarm_checker_id>\n");
	msg("       scheduler wakeupall\n");
	msg("       scheduler stop <alarm_checker_id>\n");
	msg("       scheduler stopall\n");
*/
	msg("       scheduler wakeup\n");
	msg("       scheduler stop\n");
	msg("       scheduler [add|drop|enable|disable] <node> <database> <dbms>\n");
	msg("       scheduler show\n");
	PCexit(FAIL);
}
 
msg(fmt, arg1, arg2, arg3, arg4, arg5, arg6)
char	*fmt, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	TRdisplay(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	TRflush();
	if (T_isterminal == TRUE)
	{
	    SIprintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	    SIflush(stdout);
	}
}
 
/* PCsleep has bug that it cannot sleep for more than 6
   minutes (or 3 minutes - I didn't check to see if it did
   unsigned arithmetic) handled .  Larger values cause it to overflow an 
   internal calculation resulting in shorter sleep periods */
/* return 1 if woken up by wakeup file, 0 otherwise */
goodsleep(seconds)
long	seconds;
{
    long	i;
    long	j;
 
    j = 0;
    for (i = seconds; i > 180; i -= 180)
    {
	/* check for the wakeup file every 9 minutes */
	if (((j++)%3) == 0 && wakeupexists())
	    return(WAKEUP_EVENT);
	if (((j++)%3) == 0 && stopexists())
	    return(STOP_EVENT);
	PCsleep(179000L); /* try to compensate for our wakeup and compute time*/
    }
 
    PCsleep(i * 1000);
    return (0);
}
 
/* if years or months, just build a simple date interval string.
   but if seconds, minutes or hours then 'num_units' might overflow
   so normalize to next higher unit.  For instance '40000 minutes'
   is an illegal interval.  Overflow occurs silently and one gets
   a negative number of minutes.  Similar problems for all the units,
   however it is not likely to be encountered except for seconds, 
   minutes and hours. */
build_interval(num_units, secs_per_unit, bp)
long	num_units;
long	secs_per_unit;
char	*bp;
{
    long	seconds;
    long	days;
    long	hours;
    long	minutes;
    char	daybuf[11];
    char	hourbuf[11];
    char	minutebuf[11];
    char	secondbuf[11];
 
    if (secs_per_unit > 60 * 60 * 24 * 32)
    {
	CVla(num_units, bp);
	STcat(bp, " years");
    }
    else if (secs_per_unit > 60 * 60 * 25)
    {
	CVla(num_units, bp);
	STcat(bp, " months");
    }
    else
    {
	seconds = num_units * secs_per_unit;
	days = seconds / (24 * 60 * 60);
	CVla(days, daybuf);
	seconds -= (days * (24 * 60 * 60));
	hours = seconds / (60 * 60);
	CVla(hours, hourbuf);
	seconds -= (hours * (60 * 60));
	minutes = seconds / (60);
	CVla(minutes, minutebuf);
	seconds -= (minutes * (60));
	CVla(seconds, secondbuf);
 
	STpolycat(8, daybuf, " days ", hourbuf, " hours ", minutebuf,
	    " minutes ", secondbuf, " seconds", bp);
    }
}
struct copytab {
    char	*table_name;
    char	*file_name;
};
struct copytab copytabarr[] = {
	{"ii_abfdependencies","ii_aa.tmp"},
	{"ii_abfobjects", "ii_ab.tmp"},
	{"ii_encoded_forms", "ii_ac.tmp"},
	{"ii_encodings", "ii_ad.tmp"},
	{"ii_fields", "ii_ae.tmp"},
	{"ii_forms", "ii_af.tmp"},
	{"ii_gropts", "ii_ag.tmp"},
	{"ii_id", "ii_ah.tmp"},
	{"ii_joindefs", "ii_ai.tmp"},
	{"ii_longremarks", "ii_aj.tmp"},
	{"ii_objects", "ii_ak.tmp"},
	{"ii_qbfnames", "ii_am.tmp"},
	{"ii_rcommands", "ii_an.tmp"},
	{"ii_reports", "ii_ao.tmp"},
	{"ii_trim", "ii_ap.tmp"},
	{"iidd_alt_columns", "ii_aq.tmp"},
	{"iidd_columns", "ii_ar.tmp"},
	{"iidd_dbcapabilities", "ii_as.tmp"},
	{"iidd_ddb_dbdepends", "ii_au.tmp"},
	{"iidd_ddb_ldb_columns", "ii_aw.tmp"},
	{"iidd_ddb_ldb_dbcaps", "ii_ax.tmp"},
	{"iidd_ddb_ldb_indexes", "ii_ay.tmp"},
	{"iidd_ddb_ldb_ingtables", "ii_az.tmp"},
	{"iidd_ddb_ldb_keycols", "ii_ba.tmp"},
	{"iidd_ddb_ldb_phytables", "ii_bb.tmp"},
	{"iidd_ddb_ldb_xcolumns", "ii_bc.tmp"},
	{"iidd_ddb_ldbids", "ii_bd.tmp"},
	{"iidd_ddb_long_ldbnames", "ii_be.tmp"},
	{"iidd_ddb_object_base", "ii_bf.tmp"},
	{"iidd_ddb_objects", "ii_bg.tmp"},
	{"iidd_ddb_tableinfo", "ii_bh.tmp"},
	{"iidd_ddb_tree", "ii_bi.tmp"},
	{"iidd_histograms", "ii_bj.tmp"},
	{"iidd_index_columns", "ii_bk.tmp"},
	{"iidd_indexes", "ii_bl.tmp"},
	{"iidd_integrities", "ii_bm.tmp"},
	{"iidd_multi_locations", "ii_bn.tmp"},
	{"iidd_permits", "ii_bo.tmp"},
	{"iidd_procedure", "ii_bq.tmp"},
	{"iidd_registrations", "ii_br.tmp"},
	{"iidd_stats", "ii_bs.tmp"},
	{"iidd_tables", "ii_bt.tmp"},
	{"iidd_views", "ii_bu.tmp"},
	{"i_alarm_settings", "ii_bv.tmp"},
	{"i_alarm_query_text", "ii_bw.tmp"}
    };
 
# define	WORDARRAYSIZE	10
 
builtin(query_buf, query_size)
char	*query_buf;
i4	query_size;
{
    i4		count = WORDARRAYSIZE;
    char	*wordarray[WORDARRAYSIZE];
    i4		i;
    char	locbuf[MAX_LOC];
    LOCATION	loc;
 
    exec sql begin declare section;
	char	*tablename;
	char	*filename;
	char	*node;
	char	*database;
	char	*dbms;
	int	tmp;
	char	dbms_type[33];
	char	orig_cdbname[33];
    exec sql end declare section;
 
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    query_buf[query_size] = 0;
    STgetwords(query_buf, &count, wordarray);
    CVlower(wordarray[0]);
    CVlower(wordarray[1]);
    if (count == 5 && STcompare(wordarray[0], "replicate") == 0 &&
	STcompare(wordarray[1], "cdb") == 0)
    {
	node = wordarray[2];
	database = wordarray[3];
	dbms = wordarray[4];
 
	/* scheme is:
	    direct connect to CDB
	    copy out catalogs from this CDB using exclusive table locks
		so we get a valid snapshot
	    direct connect to replicate CDB
	    create alarm catalogs if not already there
	    drop all data from catalogs
	    copy in catalogs
	    modify iidd_ddb_ldbids to make CDB entry be the new replicate CDB
 
	    Note:  user tables in the original CDB are not copied.  This means
	    that there will be danglink links to any user tables in the 
	    original CDB.  If the user wants the user tables copied then 
	    seperate snapshot copies must be set up.
	*/
 
	dbms_type[0] = 0;
	exec sql select cap_value into :dbms_type from iidbcapabilities
	    where cap_capability = 'DBMS_TYPE';
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error checking to see if DDB\n");
	    return (6);
	}
 
	if (STcompare(dbms_type, "STAR                            "))
	{
	    slave_print(1, "This command is only valid for a Distributed Database\n");
	    return (6);
	}
 
	/* copy out */
 
	/*  must commit before the direct connect */
	exec sql commit;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error commiting before direct connect for this builtin\n");
	    return (6);
	}
	exec sql direct connect;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot connect\n");
	    return(6);
	}
 
	/* any returns from this procedure after this point must do
	   a LOreset() */
	LOsave();
 
	exec sql set lockmode session where level=table, readlock=exclusive,
	    timeout=60;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot set lockmode\n");
	    goto errreturn1;
	}
 
	/* check to see that we don't have any links to the replicate CDB.
	   This will be disallowed since we will replace our the current
	   CDB information in iidd_ddb_ldbids with the replicate CDB and
	   we don't want two entries in iidd_ddb_ldbids for the same LDB */
	tmp = 0;
	exec sql select 1 into :tmp from iidd_ddb_ldbids
	    where ldb_node = :node and ldb_dbms = :dbms and ldb_database = :database;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error checking CDB for reference to replicate CDB\n");
	    goto errreturn1;
	}
	if (tmp == 1)
	{
	    slave_print(1, "Replicate CDB may not be referenced by this DDB, remove appropriate links\n");
	    goto errreturn1;
	}
 
	/* check that we are not trying to replicate onto ourselves */
	dbms_type[0] = 0;
	exec sql select dbmsinfo('database') into :orig_cdbname from
	    iidbconstants;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error checking if replicating CDB onto itself\n");
	    goto errreturn1;
	}
	/* For this error check, assume that the CDB is on the same node as
	   the STAR server and is of the same dbms type. This will handle most
	   cases.  Users can use node name aliases so not a fullproof check. */
	if (STcompare(database, orig_cdbname) == 0 &&
	    STcompare(dbms, T_dbms) == 0 &&
	    STcompare(node, T_node) == 0)
	{
	    slave_print(1, "Attempt to replicate a CDB onto itself\n");
	    goto errreturn1;
	}
 
	/* use user's current default directory if running from a terminal,
	   otherwise change to ingres/files/alarmtmp */
	if (T_isterminal == FALSE)
	{
	    STcopy("alarmtmp", locbuf);
	    NMloc(FILES, PATH, locbuf, &loc);
	    LOchange(&loc);
	}
 
	slave_print(0, "About to copy out STAR Catalogs from original CDB...\n");
	for (i = 0; i < sizeof(copytabarr) / sizeof (copytabarr[0]); i++)
	{
	    tablename = copytabarr[i].table_name;
	    filename = copytabarr[i].file_name;
 
	    /* this is what I really want to do but the ESQL precompiler will
	       not currently support it: */
	    /* exec sql copy table tablename () into :filename; */
	    {
		IIsqInit(&sqlca);
		IIwritio(0,(short *)0,1,32,0,"copy table ");
		IIwritio(0,(short *)0,1,32,0,tablename);
		IIwritio(0,(short *)0,1,32,0,"() into ");
		IIvarstrio((short *)0,1,32,0,filename);
		IIsyncup((char *)0,0);
	    }
	    if (sqlca.sqlcode < 0)
	    {
		slave_print(1, "Cannot copy table:%s\n", tablename);
		goto errreturn1;
	    }
	}
 
	exec sql commit;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot commit after copying tables out\n");
	    goto errreturn1;
	}
 
	exec sql set lockmode session where level=session, readlock=session,
	    timeout=session;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot reset lockmode\n");
	    goto errreturn1;
	}
 
	exec sql direct disconnect;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot disconnect after copying tables out\n");
	    goto errreturn2;
	}
 
	/* now copy back into the replicated CDB */
	exec sql direct connect with node=:node, database=:database, dbms=:dbms;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot connect to replicate CDB\n");
	    goto errreturn2;
	}
 
	IIlq_Protect(TRUE); /* allow updating of catalogs */
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot set catalog protection mode\n");
	    goto errreturn1;
	}
	exec sql commit;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot commit after setting catalog protection mode\n");
	    goto errreturn3;
	}
 
	exec sql set lockmode session where level=table, readlock=exclusive,
	    timeout = 60;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot set lockmode in replicate CDB\n");
	    goto errreturn3;
	}
 
	/* see if user is the dba of this database - if not disallow the command */
	tmp = 0;
	exec sql select 1 into :tmp from iidbconstants
	    where dbmsinfo('username') = dbmsinfo('dba');
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error checking dba of replicate CDB\n");
	    goto errreturn3;
	}
	if (tmp == 0)
	{
	    slave_print(1, "Must be dba of replicate CDB\n");
	    goto errreturn3;
	}
 
	/* alarm catalogs may not yet have been created in the replicate,
	   so create them now */
	crtabs(0, 1, 0); /* user database catalogs, drop first, not verbose */
 
	/* delete all data from all the catalogs */
	for (i = 0; i < sizeof(copytabarr) / sizeof (copytabarr[0]); i++)
	{
	    tablename = copytabarr[i].table_name;
	    /* this is what I really want to do but our ESQL precompilers do
		do not support it: */
	    /* exec sql delete from tablename; */
	    {
		IIsqInit(&sqlca);
		IIwritio(0,(short *)0,1,32,0,"delete from ");
		IIwritio(0,(short *)0,1,32,0,tablename);
		IIsyncup((char *)0,0);
	    }
	    if (sqlca.sqlcode < 0)
	    {
		slave_print(1, "Cannot delete from table:%s in replicate CDB\n",
		    tablename);
		goto errreturn3;
	    }
	}
 
	slave_print(0, "About to copy STAR Catalogs into Replicate CDB...\n");
 
	/* copy in the catalogs */
	for (i = 0; i < sizeof(copytabarr) / sizeof (copytabarr[0]); i++)
	{
	    tablename = copytabarr[i].table_name;
	    filename = copytabarr[i].file_name;
 
	    /* this is what I really want to do but our ESQL precompilers do not
   		currently allow it: */
	    /* exec sql copy table tablename () from :filename; */
	    {
		IIsqInit(&sqlca);
		IIwritio(0,(short *)0,1,32,0,"copy table ");
		IIwritio(0,(short *)0,1,32,0,tablename);
		IIwritio(0,(short *)0,1,32,0,"() from ");
		IIvarstrio((short *)0,1,32,0,filename);
		IIsyncup((char *)0,0);
	    }
	    if (sqlca.sqlcode < 0)
	    {
		slave_print(1, "Cannot copy table:%s into replicate CDB\n",
		    tablename);
		goto errreturn3;
	    }
	}
 
	exec sql update iidd_ddb_ldbids
	    set ldb_node = :node, ldb_dbms = :dbms, ldb_database = :database
	where ldb_id = 1;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Error updating iidd_ddb_ldbids with Replicate CDB name\n");
	    goto errreturn3;
	}
	if (sqlca.sqlcode == ZERO_ROWS)
	{
	    slave_print(1, "No rows updated when updating iidd_ddb_ldbids with Replicate CDB name\n");
	    goto errreturn3;
	}
 
	exec sql commit;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot commit the replicate CDB (1)\n");
	    goto errreturn3;
	}
 
	exec sql set lockmode session where level=session, readlock=session,
	    timeout = session;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot reset lockmode in replicate CDB\n");
	    goto errreturn3;
	}
 
	exec sql commit;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot commit the replicate CDB (2)\n");
	    goto errreturn3;
	}
 
	IIlq_Protect(FALSE); /* allow updating of catalogs */
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot disable catalog protect mode in replicate CDB\n");
	    goto errreturn1;
	}
 
	exec sql direct disconnect;
	if (sqlca.sqlcode < 0)
	{
	    slave_print(1, "Cannot disconnect from replicate CDB\n");
	    goto errreturn2;
	}
 
	delallcopytmps();
	LOreset();
	return (0);
 
    errreturn1:
	delallcopytmps();
	exec sql rollback;
	exec sql set lockmode session where level=session, readlock=session,
	    timeout = session;
	if (sqlca.sqlcode < 0)
	    slave_print(1, "Cannot reset lockmode error(1) return\n");
	exec sql direct disconnect;
	LOreset();
	return (6);
 
    errreturn2:
	delallcopytmps();
	exec sql rollback;
	LOreset();
	return (6);
 
    errreturn3:
	delallcopytmps();
	exec sql rollback;
	exec sql set lockmode session where level=session, readlock=session,
	    timeout = session;
	IIlq_Protect(FALSE);
	exec sql direct disconnect;
	LOreset();
	return (6);
    }
    else
    {
	slave_print(1, "Error - unrecognized command:%s\n", wordarray[0]);
	return (4);
    }
    return(5);
}
 
/* execute an arbitrary OS command using PCcmdline */
callreport(query_buf)
char	*query_buf;
{
    STATUS	retval;
 
    exec sql begin declare section;
    char	tmp2[36];
    exec sql end declare section;
 
    (void) STpolycat(2, "-u", T_user, tmp2);
 
    exec sql call report (database = :T_database, report = '', flags = :tmp2);
 
    /* need to check and return status */
    return (0);
}
 
crtabs(master, dropfirst, verbose)
i4	master;
i4	dropfirst;
i4	verbose;
{
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    if (master)
    {
	if (dropfirst)
	{
	    exec sql drop i_alarm_checkers;
	    exec sql drop i_alarmed_dbs;
	    /* don't check error status on the above */
	}
 
	if (verbose)
	    err_print(0, "Creating table 'i_alarm_checkers'...\n");
	exec sql create table i_alarm_checkers (
	    alarm_checker_id	varchar(3)
		not null not default,	/* Alarm checker unique id.
					   This id is used a a file
					   suffix.  It is varchar to
					   fix problems caused by
					   blank padding in char's. */
	    alarm_align		char(3)
		not null with default,	/* Keep things aligned */
	    criteria_node	varchar(32)
		not null not default,	/* This alarm checker will 
					   service alarm tables meeting
					   these criteria.  The criteria
					   in this row are ANDed
					   together.  Multiple rows with
					   the same alarm_checker_id are
					   ORed. A criteria may include
					   the % and ? pattern matching
					   characters. */
	    criteria_dbms	varchar(32)
		not null not default,	/* DBMS-type: INGRES, DB2, ...*/
	    criteria_database	varchar(32)
		not null not default,	/* Database name */
	    criteria_user	varchar(32)
		not null not default,	/* User name */
	    alarm_priority	integer
		not null with default	/* Order in which to apply
					   the above criteria.  The
					   highest priority alarm
					   checker whose criteria match
					   an alarm table will service
					   that alarm table */
	    );
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_alarm_checkers\n");
	exec sql modify i_alarm_checkers to btree on alarm_checker_id;
	if (sqlca.sqlcode < 0)
	    err_print(1, "errprint && Error modifying i_alarm_checkers\n");
 
	/*
	** These databases need to be checked for alarms by the specified alarm
	** checker.
	*/
	if (verbose)
	    err_print(1, "Creating table 'i_alarmed_dbs'...\n");
	exec sql create table i_alarmed_dbs (
	    alarm_node		varchar(32)
		not null not default,	/* Node of alarm table.  Default
					   value implies current node */
	    alarm_dbms		varchar(32)
		not null not default,	/* DBMS type of alarm table.
					   Default value implies default
					   dbms type (usually INGRES) */
	    alarm_database	varchar(32)
		not null not default,	/* Database of alarm table */
	    alarm_user		varchar(32)
		not null not default,	/* User name of alarm table */
	    alarm_conarg0	varchar(8)
		not null with default,	/* Args to use on connection */
	    alarm_conarg1	varchar(8)
		not null with default,
	    alarm_conarg2	varchar(8)
		not null with default,
	    alarm_conarg3	varchar(8)
		not null with default,
	    alarm_conarg4	varchar(8)
		not null with default,
	    alarm_conarg5	varchar(8)
		not null with default,
	    alarm_conarg6	varchar(8)
		not null with default,
	    alarm_conarg7	varchar(8)
		not null with default,
	    alarm_conarg8	varchar(8)
		not null with default,
	    alarm_conarg9	varchar(8)
		not null with default,
	    alarm_checker_id	varchar(3)
		with null,		/* Alarm checker assigned to this
					   alarm table.  This assignment is 
					   made at INGRES installation startup
					   time by the 'iialarm' process (or 
					   which can be started by the 
					   'runalarm' command).  'iialarm' 
					   applies the criteria in the alarm_
					   checkers table to this
					   alarm_checker_id assignment column. 
					   The highest priority assignment
					   takes priority. */
	    alarm_enabled	varchar(1)
		not null with default	/* 'Y' if this one is currently
					   enabled*/
	);
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_alarmed_dbs\n");
	exec sql modify i_alarmed_dbs to btree unique on alarm_node,
	    alarm_dbms, alarm_database, alarm_user;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error modifying i_alarmed_dbs\n");
 
	/* make sure user only adds entries for themself */
	exec sql create integrity on i_alarmed_dbs is
	    alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating integrity on i_alarmed_dbs\n");
 
	/* make sure user only accesses/modifies their own entries */
	if (verbose)
	    err_print(1, "Creating table 'i_user_alarmed_dbs'...\n");
	exec sql create view i_user_alarmed_dbs as select * from i_alarmed_dbs
	    where alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating view i_user_alarmed_dbs\n");
	exec sql create permit all on i_user_alarmed_dbs to all;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating permit on i_user_alarmed_dbs\n");
 
	/*
	** After 'iialarm' sets the alarm_checker_id's in i_alarmed_dbs, it 
	** spawns one process for each alarm checker.  Each alarm checker is
	** given its alarm_checker_id.  Using this id each alarm checker 
	** selects all matching rows from i_alarmed_dbs.  For each matching
	** row the alarm checker connects to the specified database as the
	** specified user and then checks the alarm table for alarms that need
	** to be serviced. After all alarms have been serviced for that alarm
	** table, the next database is accessed and its alarm table is
	** serviced.  When a complete cycle through all alarm tables is made
	** with no alarms to be serviced, then the alarm checker sleeps for
	** the amount of time specified in alarm_sleeptime.  After waking up,
	** the cycle repeats.
	*/
    }
    else /* create user database tables */
    {
	/*
	** The following two tables are created in any database that wishes to
	** have alarm service.  Each user in a database must have their own
	** set of the following two tables.  This is to ensure security (to
	** prevent one user from entering alarm queries for another)
	*/
 
	if (dropfirst)
	{
	    exec sql drop i_alarm_settings;
	    exec sql drop i_alarm_query_text;
	    /* don't check error status on the above */
	}
 
	if (verbose)
	    err_print(1, "Creating table 'i_alarm_settings'...\n");
	exec sql create table i_alarm_settings (
	    alarm_user	varchar(32)
		not null not default,	/* User that this alarm applies to */
	    alarm_id	varchar(32)
		not null not default,	/* Unique user-chosen name for this
					   alarm entry */
	    alarm_desc	varchar(128)
		not null with default,	/* Description for this alarm entry */
	    alarm_align1	char(2)
		not null with default,
	    alarm_priority	integer
		not null with default,	/* Overdue alarms are executed in
					   order of priority and then by the
					   amount of time overdue */
	    last_time_run	char(25)
		not null with default,	/* Last time this alarm entry was 
					   successfully executed - standard
					   catalog format */
	    start_time		char(25)
		not null not default,	/* First time that this alarm entry
					   should be executed - standard catalog
					   format */
	    alarm_align2	char(2)
		not null with default,
	    interval_num	integer
		not null not default,	/* Number of interval_units between
					   executing this alarm entry.  If ...
					   then this alarm entry
					   needs to be executed. */
	    interval_units	char(8)
		not null not default,	/* Interval units - 'seconds',
					   'minutes', 'hours', 'days', 'months',
					   'years' */
	    next_time		char(25)
		with null,		/* Used by alarm checker to schedule
					   next alarm */
	    alarm_align3	char(2)
		not null with default,
	    retry_enabled	integer
		not null with default,	/* 0 means 'retry' is not in effect,
					   >= 1 means next this is the next
					   retry scheduled.  Incremented after
					   each retry.  When > retry_number
					   then reset to 0 and save_* columns
					   copied back into original columns */
	    retry_number	integer
		not null with default,	/* number of times to retry - 0 means
					   do not retry, just wait until next
					   scheduled time */
	    retry_interval_num	integer
		not null with default,	/* number of interval_units to wait
					   until retry */
	    retry_interval_units char(8)
		not null with default,	/* how long to wait until a retry.
					   for example: retry_number=3,
					   retry_interval_num=10,
					   retry_interval_units='minutes' would
					   mean on query failure, retry every
					   ten minutes up to three times. */
	    save_start_time	char(25)
		not null with default,	/* original start_time, while
					   retry_enabled is true */
	    alarm_align4	char(1)
		not null with default,
	    save_interval_num	integer
		not null with default,	/* original interval_num, while
					   retry_enabled is true */
	    save_interval_units	char(8)
		not null with default,	/* original interval_units, while
					   retry_enabled is true */
	    alarm_enabled	char(1)
		not null not default	/* "Y" means this alarm is enabled.
					   Other values mean this alarm is
					   not enabled. */
	    );
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_alarm_settings\n");
 
	exec sql modify i_alarm_settings to btree unique on alarm_id;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error modifying i_alarm_settings\n");
 
	/* make sure user only adds entries for themself */
	exec sql create integrity on i_alarm_settings is
	    alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating integrity on i_alarm_settings\n");
 
	/* make sure user only accesses/modifies their own entries */
	if (verbose)
	    err_print(1, "Creating table 'i_user_alarm_settings'...\n");
	exec sql create view i_user_alarm_settings as select * from i_alarm_settings
	    where alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_user_alarm_settings\n");
 
	exec sql create permit all on i_user_alarm_settings to all;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating permit on i_user_alarm_settings\n");
 
	if (verbose)
	    err_print(1, "Creating table 'i_alarm_query_text'...\n");
	exec sql create table i_alarm_query_text (
	    alarm_user	varchar(32)
		not null not default,	/* User that this alarm applies to */
	    alarm_id	varchar(32)
		not null not default,	/* Unique user-chosen name */
	    query_seq	integer
		not null with default,	/* The query number for this alarm_id;
					   starting at 0.  There may be multiple
					   queries for this alarm.  The queries
					   are executed in query_seq order. */
	    failure_action	char(1)
		not null not default,	/* Action to take if query has sqlca.
					   sqlcode < 0. 'C' means continue with
					   remaining queries. 'S' means stop
					   working on this alarm_id and commit
					   transaction */
	    text_dml		char(1)
		not null not default,	/* "S" for SQL, "B" for Builtin (like
					   "replicate cdb ...") */
	    alarm_align1	char(2)
		not null with default,
	    text_sequence	integer
		not null with default,	/* The sequence number of the
					   text_segment field; starting at 0.
					   Needed when the query text is larger
					   than text_segment */
	    text_segment	varchar(1900)
		not null with default,	/* SQL query. Legal queries are those
					   that can be executed with EXECUTE
					   IMMEDIATE */
	    sqlca_sqlcode	integer
		not null with default,	/* Result code last time this query
					   was run - only set in rows where
					   text_sequence is 0 */
	    iialarmchk_error	integer
		not null with default	/* Error code generated by alarm
					   checker if it was unable to even
					   attempt to run the query - only set
					   in rows where text_sequence is 0 */
	    );
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_alarm_query_text\n");
 
	exec sql modify i_alarm_query_text to cbtree unique on alarm_id,
	    query_seq, text_sequence;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error modifying i_alarm_query_text\n");
 
	/* make sure user only adds entries for themself */
	exec sql create integrity on i_alarm_query_text is
	    alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating integrity on i_alarm_query_text\n");
 
	/* make sure user only accesses/modifies their own entries */
	if (verbose)
	    err_print(1, "Creating table 'i_user_alarm_query_text'...\n");
	exec sql create view i_user_alarm_query_text as select * from i_alarm_query_text
	    where alarm_user=dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating i_user_alarm_query_text\n");
 
	exec sql create permit all on i_user_alarm_query_text to all;
	if (sqlca.sqlcode < 0)
	    err_print(1, "Error creating permit on i_user_alarm_query_text\n");
    }
}
 
delallcopytmps()
{
    i4		i;
    LOCATION	loc;
    char	locbuf[MAX_LOC];
 
    for (i = 0; i < sizeof(copytabarr) / sizeof (copytabarr[0]); i++)
    {
	STcopy(copytabarr[i].file_name, locbuf);
	LOfroms(FILENAME, locbuf, &loc);
	LOdelete(&loc);
    }
}
 
stop()
{
    LOCATION	stop_loc;
    LOCATION	save_loc;
    char	stop_file[MAX_LOC];
    char	save_file[MAX_LOC];
 
    /* first delete stop file if it exists - this avoids getting multiple
       stop files created in VMS */
    stopexists();
 
    (void) STpolycat(2, "alarmstp.", /*T_alarm_checker_id*/ "def", stop_file);
    NMloc(FILES, FILENAME, stop_file, &stop_loc);
    LOcopy(&stop_loc, save_file, &save_loc);
    if (LOcreate(&save_loc) != OK)
    {
	msg("Unable to create stop file - probably lacking appropriate permissions.\n");
	msg("Only the INGRES system administrator is allowed to run this command.\n");
    }
}
 
/* we cannot just create the wakup file since this program does not have
   permission to create the wakeup file in the 'files' directory.  This
   program cannot run suid (on unix) since it needs to run as the user
   in order for the correct database permissions, and userid, to apply
   when checking and updating schedules.  (On VMS we could just install
   this program with SYSPRV but this is not a portable solution). Also note that
   by NOT giving this program excess permissions, the 'stop' command above
   works as desired, namely only the INGRES system administrator (or someone
   with write access to the ../files directory) can perform a stop. */
wakeup()
{
    /* Call the wakeup program */
 
    LOCATION	wakeup_loc;
    LOCATION	save_loc;
    char	wakeup_name[MAX_LOC];
    char	wakeup_cmd[MAX_LOC + 4];
    CL_ERR_DESC  clerr;
 
    /* the program name is 'wakeup' */
    /* wakeup now lives in II_SYSTEM/ingres/sig/star -- use NMgtAt */

    char *ptr ;
    char buf[MAX_LOC] ;

    NMgtAt( ERx( "II_SYSTEM" ), &ptr ) ;
    if( !ptr || !*ptr )
    {
      SIprintf( ERx( "\nII_SYSTEM must be set to the full pathname of\n " ) );
      SIprintf( ERx( "\nthe directory where INGRES is installed.\n " ) );
      SIprintf( ERx( "\nPlease set II_SYSTEM and re-run this program.\n " ) );
      PCexit( FAIL ) ;
    }
    buf[0] = 0 ; 
    STpolycat( 2, ptr, ERx( "ingres/sig/star/wakeup" ), buf ) ;
    LOfroms( PATH&FILENAME, buf, &wakeup_loc ) ;

    LOcopy(&wakeup_loc,wakeup_name,&save_loc);
 
    wakeup_cmd[0] = '\0';
    if (STindex(wakeup_name,"[",0) != NULL)
    {
	/* VMS */
	STcopy("RUN ",wakeup_cmd);
    }
    STcat(wakeup_cmd,wakeup_name);
    if (PCcmdline(NULL, wakeup_cmd, 0, 0, &clerr) != OK)
    {
	msg("Unable to create wakeup file.\n");
    }
}
 
/* only called in master or slave mode, never in utility mode */
stopexists()
{
    LOCATION	stop_loc;
    LOCATION	save_loc;
    char	stop_file[MAX_LOC];
    char	save_file[MAX_LOC];
 
    (void) STpolycat(2, "alarmstp.", /*T_alarm_checker_id*/ "def", stop_file);
    NMloc(FILES, FILENAME, stop_file, &stop_loc);
    LOcopy(&stop_loc, save_file, &save_loc);
    if (LOexist(&save_loc) == OK)
    {
	LOdelete(&save_loc);
	return (TRUE);
    }
    return (FALSE);
}
 
/* only called in master or slave mode, never in utility mode */
wakeupexists()
{
    LOCATION	wakeup_loc;
    LOCATION	save_loc;
    LOINFORMATION	loinf;
    char	wakeup_file[MAX_LOC];
    char	save_file[MAX_LOC];
    i4		flaginf;
 
    (void) STpolycat(2, "alarmwkp.", /*T_alarm_checker_id*/ "def", wakeup_file);
    NMloc(FILES, FILENAME, wakeup_file, &wakeup_loc);
    LOcopy(&wakeup_loc, save_file, &save_loc);
    flaginf = LO_I_TYPE ;
    if (LOinfo(&save_loc, &flaginf, &loinf) == OK)
    {
	if (!(flaginf & LO_I_TYPE))
	{
	    err_print(1,"Error detecting type of wakeup file\n");
	    PCexit(FAIL);
	}
	if (loinf.li_type != LO_IS_FILE)
	{
	    err_print(1,"Wakeup file is not a regular file\n");
	    PCexit(FAIL);
	}
	if (LOdelete(&save_loc) != OK)
	{
	    err_print(1,"Error clearing the wakeup file\n");
	    PCexit(FAIL);
	}
	return (TRUE);
    }
    return (FALSE);
}
 
mastermaint(function, argc, argv)
i4	function;
i4	argc;
char	**argv;
{
/* connect and use dbmsinfo('username') as the username */
    exec sql begin declare section;
	char	*database;
	char	*node;
	char	*dbms;
	char	databasebuf[33];
	char	nodebuf[33];
	char	dbmsbuf[33];
	char	userbuf[33];
	char	alarmbuf[4];
    exec sql end declare section;
 
    exec sql WHENEVER NOT FOUND CONTINUE ;
    exec sql WHENEVER SQLWARNING CONTINUE;
    exec sql WHENEVER SQLERROR CONTINUE;
 
    T_alarm_checker_id = "maint";
 
    node = argv[2];
    database = argv[3];
    dbms = argv[4];
 
    exec sql connect iialarmdb;
 
    if (sqlca.sqlcode < 0)
    {
	err_print(1, "Error connecting to iialarmdb, exiting\n");
	PCexit(1);
    }
    if (function == 1 && argc == 5) /* add */
    {
	exec sql insert into i_user_alarmed_dbs values (
	    :node,
	    :dbms,
	    :database,
	    dbmsinfo('username'),
	    '','','','','','','','','','','def','Y');
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error adding database, no database added\n");
	    exec sql rollback;
	}
	else if (sqlca.sqlcode == ZERO_ROWS)
	{
	    err_print(1, "No database added, probable duplicate database specified\n");
	}
	else
	{
	    err_print(1, "One database added\n");
	    wakeup();
	}
    }
    else if (function == 2 && argc == 5) /* drop */
    {
	exec sql delete from i_user_alarmed_dbs where
	    alarm_node = :node and
	    alarm_dbms = :dbms and
	    alarm_database = :database and
	    alarm_user = dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error droping database, no database dropped\n");
	    exec sql rollback;
	}
	else if (sqlca.sqlcode == ZERO_ROWS)
	{
	    err_print(1, "No database dropped, specified database not found\n");
	}
	else
	{
	    err_print(1, "One database dropped\n");
	    wakeup();
	}
    }
    else if (function == 3 && argc == 5) /* enable */
    {
	exec sql update i_user_alarmed_dbs set alarm_enabled = 'Y' where
	    alarm_node = :node and
	    alarm_dbms = :dbms and
	    alarm_database = :database and
	    alarm_user = dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error enabling database, no database enabled\n");
	    exec sql rollback;
	}
	else if (sqlca.sqlcode == ZERO_ROWS)
	{
	    err_print(1, "No database enabled, specified database not found\n");
	}
	else
	{
	    err_print(1, "One database enabled\n");
	    wakeup();
	}
    }
    else if (function == 4 && argc == 5) /* disable */
    {
	exec sql update i_user_alarmed_dbs set alarm_enabled = 'N' where
	    alarm_node = :node and
	    alarm_dbms = :dbms and
	    alarm_database = :database and
	    alarm_user = dbmsinfo('username');
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error disabling database, no database disabled\n");
	    exec sql rollback;
	}
	else if (sqlca.sqlcode == ZERO_ROWS)
	{
	    err_print(1, "No database disabled, specified database not found\n");
	}
	else
	{
	    err_print(1, "One database disabled\n");
	    wakeup();
	}
    }
    else if (function == 5 && argc == 2) /* show */
    {
	SIprintf("NODE              DATABASE          DBMS              USER              ENABLED\n");
	exec sql select alarm_node, alarm_dbms, alarm_database, alarm_user,
		alarm_enabled
	    into :nodebuf, :dbmsbuf, :databasebuf, :userbuf, :alarmbuf
	    from i_user_alarmed_dbs;
	exec sql begin;
	    SIprintf("%-18s%-18s%-18s%-18s%c\n", nodebuf, databasebuf, dbmsbuf,
		userbuf, alarmbuf[0]);
	exec sql end;
	if (sqlca.sqlcode < 0)
	{
	    err_print(1, "Error listing entries\n");
	    exec sql rollback;
	}
    }
    else
    {
	exec sql commit;
	exec sql disconnect;
	usage();
    }
 
    exec sql commit;
    exec sql disconnect;
}
 
