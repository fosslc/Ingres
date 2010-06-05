/*
**	mtsslave.sc
**
**	This program executes the routines for each slave in MST
**
*/

/*
**	modification history
**
**	who	when		what
**	---	----		------------------------------------------
**	jds	2/14/92		- add a pcsleep BEFORE checking for START table
**				- check for ERR_SELECT and ERR_NOT_OWNER as returns
**				from trying to read the START table - sleep only
**				for those.
**
**
**
**      jds     3/4/92          drop ref to me.h - never gets used, not
**                              thre anyway...
**
**	bh 	6/25/92		added libraries for ming. NEEDLIBS
**
** 02/03/93 (erniec)
** 	added ERR_CATDEADLK, ERR_QUERYABORT, NO_ERROR, and ERR_RQF_TIMEOUT
**		as acceptable, expected errors
** 	add variable levels of tracing information, traceflag>1 for debugging
** 	move comments for readability
**
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**
** slave_sync: 
**	Increase sleep waiting for start table to 60 reps * 60000
** 	Add PCsleep(5000) to "do while" deadlock loops
**	disable merr error handling on "select num from start"
**		this was suppressing messages for non-start table errors
** run_tests:
**	modify error handling to switch statement, add additional errors
**	create a error status type for each error:
**		[A-Z] for terminating errors, [a-z] for expected errors
**	add char errstatus[2] to record the error encountered
**	add i4  okerror - flag for terminating and acceptable errors
**	add a variable sleep after an acceptable error is encountered
**	if okerror encountered, repeat the repetition
**	change rollback to commit - to terminate transaction and add_msg
**		which logs the errors encountered in table err_msg
** 	Add PCsleep(5000) to "do while" deadlock loops
** add_msg:
** 	Add PCsleep(5000) to "do while" deadlock loops
** cleanup:
** 	Add PCsleep(5000) to "do while" deadlock loops
**	if non-deadlock error removing slave_num, print message 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** History:
**	26-aug-1993 (dianeh)
**		Added #include of <st.h>.
**	27-Aug-1993 (fredv)
**		Included <me.h>.
**      23-dec-1997 (hanch04)
**          Added FAIL to PCexit
**  30-Sep-1998 (merja01)
**      Change sql long types to i4 to make test protable to axp_osf.
**  13-jun-2000 (stephenb)
**     Add code to check for Star connections. In this case we need to unset
**     ddl concurrency to prevent hangs (bug 100282). Note: this change
**     can be removed when the bug is fixed.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

/* ming hints 
**
NEEDLIBS = MTSLIB LIBINGRES
**
*/

exec sql include sqlca;

# include	<time.h>
# include       <compat.h>
# include       <si.h>
# include       <me.h>
# include       <st.h>
# include       <mh.h>
# include       <lo.h>
# include       <pc.h>
# include       "trl.h"
# include	<cv.h>

GLOBALDEF 	i4	traceflag;  	/*flag to set trace on/off */

exec sql begin declare section;
# define	ERR_DEADLOCK		-4700	
# define        ERR_TIMEOUT     	-4702
# define        ERR_OUT_OF_LOCKS 	-4705
# define	ERR_LOGFULL 		-4706	
# define 	ERR_SELECT		-2117
# define	ERR_NOT_OWNER		-2119
# define        USER_PASSWORD           -786481
# define        GE_SERIALIZATION        -49900
# define        GE_OTHER_ERROR          -39100
# define        NO_ERROR          	0
# define        ERR_CATDEADLK		-196674
# define        ERR_QUERYABORT          -4708
# define        ERR_RQF_TIMEOUT     	-394566

exec sql end declare section;



/*
**	SLAVE - 
**	This is the slave executable for the MTS. Its algorithm follows:
**
**	process args (which slave it is, which cc_test is run)
**	retrieve test routine list and number of reps information
**	synchronize with the other slaves to start
**	for as many times as reps indicates to loop though the routine list
**	{
**	   (if we get a deadlock, pop up to here, try the routine again)
**		for each routine in the list do
**		{
**			execute the TRL routine
**			(if we get a syserr, there's a bug, break from loops)
**			write to its internal data structure
**				the routine number
**		}
**		
**	}
**	sleep (to calm down the machine)
**	append the internal data structure to test_log table, 
**		including its slave number
**	synchronize with the coordinator to end
**
**	Parameters -
**		argc -	argument count
**		argv -	argument array
**		 [0] -	points to the directory spec of this executable
**		 [1] -	database name
**		 [2] -	concurrency test number
**		 [3] -  which slave program this is
**		 [4] -  traceflag.
*/

main(argc, argv)
i4		argc;
 char		*argv[];
{
exec sql begin declare section;
	i4	test_num;	/* concurrency test number */
	i4	slave_num;	/* slave number */
	i4	reps;		/* how many times to execute this slave's routine list */
	char	*dbname=argv[1];
exec sql end declare section;


	i4	bomb;		/* flag to denote whether ingres bombs out */	
	struct	rout_list	*routines;	/* list of routines to execute*/

	FUNC_EXTERN 	int	trlsize();
	int	errproc();

	
/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

/*	PROCESS THE ARGUMENTS AND DO SOME INITIALIZATION	*/

	routines = NULL;

	proc_args(argc,argv,&test_num,&slave_num,&traceflag);

	SIprintf("SLAVE #%d: Opening database...\n",slave_num);
	SIflush(stdout);


	exec sql whenever sqlerror stop;	/* stop if error occurs while opening db*/
	exec sql connect :dbname;

	exec sql whenever sqlerror call merr;	/*print error message when error occurs*/
	
	exec sql commit;
	exec sql set autocommit on;		/* commit each transaction after it's done*/

	if (STbcompare("/star", 5, &dbname[STlength(dbname) - 5], 5, TRUE) == 0)
	{
	    /* this is a star connection */
	    exec sql set ddl_concurrency off;
	}

/*	RETRIEVE TEST ROUTINE LIST AND NUMBER OF REPS INFORMATION	*/

	db_info(test_num,slave_num,&reps,&routines);

	if (routines == NULL)
	{
		SIprintf("SLAVE #%d: routines are NULL; none retrieved from DB\n",slave_num);
		SIprintf("SLAVE #%d: Aborting execution...\n", slave_num);
		SIflush(stdout);
	}
	else
	{
		SIprintf("SLAVE #%d: Going to get synced up\n",slave_num);
		SIflush(stdout);


/*	    SYNCHRONIZE WITH THE OTHER SLAVES TO START	*/

	    	slave_sync(slave_num);

		SIprintf("\n");
		SIprintf("\n");
		SIprintf("SLAVE #%d: Finished getting synced up \n",slave_num);
		SIprintf("SLAVE #%d: Running routines.... \n",slave_num);
		SIprintf("\n");
		SIflush(stdout);


/*	    EXECUTE THE SPECIFIED TEST ROUTINES	*/

	    bomb = FALSE;

   	    run_tests(argv, routines, reps, slave_num, &bomb);

	    if (bomb)
	    {
		SIprintf("BOMBING NOW...\n");
		SIflush(stdout);
	    }

	}

	cleanup (slave_num);

	exec sql disconnect;
	PCexit(OK);
}


/*
** PROC_ARGS
**	processes the arguments and does some initialization.
**
**	Parameters -
**		routine_list -	points to the routine list pointer
**		argc	     -	how many arguments
**		argv	     -	element [2] is the cctest number
**			     -	element [3] is the slave number
**		cctest	     -	pointer to which concurrency test we're running
**		slave	     -	pointer to this slave's ID number
**		trace	     -  pointer to the traceflag
*/
proc_args(argc,argv,cctest,slave,trace)
i4	argc;
char	*argv[];
i4	*cctest;
i4	*slave;
i4 *trace;
{
	i4	status;

	if (argc != 5)
	{
		SIprintf("The syntax is: mtsslave db_name cc_test slave_num traceflag\n");
		SIflush(stdout);
		PCexit(FAIL);
	}

	if ( (status=CVal(argv[2],cctest)) != OK)
	{
		SIprintf("%s is an invalid Concurrency Test Number\n",argv[2]);
		SIflush(stdout);
		PCexit(FAIL);
	}

	if ( (status=CVal(argv[3],slave)) != OK)
	{
		SIprintf("%s is an invalid Slave Number\n",argv[3]);
		SIflush(stdout);
		PCexit(FAIL);
	}

	if ( (status=CVal(argv[4],trace)) != OK)
	{
		SIprintf("%s is an invalid trace flag \n",argv[4]);
		SIflush(stdout);
		PCexit(FAIL);
	}
}



/*
**	DB_INFO
**		loads in vital information from the database into the
**	slave. From the test_index table, the slave retrieves which test
**	routines it is supposed to execute, and build a linked list with this
**	info. It retrieves how many times to execute those routines from the
**	test_reps table.
**
**	Parameters -
**		test_num  -	which concurrency test we're running
**		slave_num -	this slave's ID number
**		reps_ptr  -	points to the number of repititions
**		rout_ptr  -	points to the list of routines to run
*/

db_info(test_num,slave_num,reps_ptr,rout_ptr)
exec sql begin declare section;
	i4	test_num;
	i4	slave_num;
exec sql end declare section;
	i4	*reps_ptr;
	struct rout_list	**rout_ptr;
{

exec sql begin declare section;
	i4	reps_num;	/* gets repetition number from DB */
	i4	rout;		/* gets routine number from DB */
exec sql end declare section;

	struct	rout_list	*routines;
	struct 	rout_list	*temp;		/* building list of routines */
	struct	rout_list	*current;
	i4	status;				/* error status */

	FUNC_EXTERN 	int	trlsize();		/* check range of trl */

/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

	current = NULL;
	temp = NULL;
	routines = NULL;

	exec sql select reps
		into :reps_num
		from test_reps
		where cc_test = :test_num and slave_num = :slave_num;

	*reps_ptr = reps_num;
	
/*
** this is a select loop for the routine numbers. we build a link list
** of them in the classic manner. with each test routine retrieved,
**	make a new structure for it
**	fill it with data
**	if its the first one, make it the head of the list
**	if its not, add it to the end of the list
*/

	exec sql select rout_num
		into :rout
		from test_index
		where cc_test = :test_num and slave_num = :slave_num;
	exec sql begin;
		if (rout > trlsize())
		{
			SIprintf("SLAVE %d: SlaveInfo: routine #%d ",
							slave_num, rout);
			SIprintf("exceeds TRL's max routine #%d\n\n",trlsize());
			SIprintf("Routine #%d will be skipped\n",rout);
			SIflush(stdout);
		}
		else
		{
                        if ((temp = (struct rout_list *) MEreqmem (0,sizeof(struct rout_list),1,NULL)) == NULL)
			{
				SIprintf ("SLAVE %d: SlaveInfo: bad allocation\n", slave_num);
				SIflush (stdout);
				PCexit(FAIL);
			}

			SIprintf ("SLAVE #%d: should run routine %d\n",slave_num,rout);
			SIflush (stdout);
	
			temp->number = rout;
			temp->next = NULL;
	
			if (routines == NULL)		/* first one ? */
				routines = temp;
			if (current != NULL)		/* add to end of list */
				current->next = temp;
			current = temp;			/* our new tail */

		} /* end else rout > max */
	exec sql end;

	SIprintf ("\n");
	SIprintf ("SLAVE #%d: run these routines %d times\n",slave_num,reps_num);
	SIflush (stdout);
	
	*rout_ptr = routines;

} /* end db_info */



/*	SLAVE_SYNC -	Sync up the coordinator and all it's slaves,
**			such that they all exit this routine
**			at the same time.
**
**	Parameters -
**		kid -		which slave number this is.
*/

slave_sync(kid)
exec sql begin declare section;
	i4	kid;
exec sql end declare section;
{
exec sql begin declare section;
	i4	start_num;
exec sql end declare section;

exec sql begin declare section;
        char    errmsg[256];
exec sql end declare section;

	i4		loop = 0;

/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

/* FIRST CRUCIAL TIME POINT */
	/* First, insert a row to slaves_up so the coordinator knows this */
	/* slave is ready */

	do
	{
		exec sql insert into slaves_up (slave_num) values (:kid);

		if (sqlca.sqlcode == ERR_DEADLOCK)
			PCsleep(5000);  /* give deadlocks a chance to clear */

	} while (sqlca.sqlcode == ERR_DEADLOCK);

	SIprintf("SLAVE #%d: Has appended number %d\n",kid,kid);
	SIflush(stdout);

/* SECOND CRUCIAL TIME POINT */
	/* Now wait until the start table has been created */

	/*
	**
	**	Interestingly, as previously written, the slave
	**	looks _immediately_ for table start.
	**
	**	That's wrong; it can't exist until _all_ the
	**	slaves have appended their slave_number rows
	**	to slaves_up.
	**
	**	Add a 60-second sleep BEFORE the first check -
	**	recent experimentation show that the slaves
	**	may run faster than the coordinator.
	**
	**	jds	2/14/92
	**
	*/

	PCsleep (60000);

	do
	{
		exec sql whenever sqlerror continue;

		exec sql select num 
			into :start_num
			from start;

		if ((sqlca.sqlcode < 0) && 
		   ((sqlca.sqlcode == ERR_SELECT) || 
		    (sqlca.sqlcode == ERR_NOT_OWNER)))
		{
                          PCsleep(60000);
		}

		if ((sqlca.sqlcode < 0) && (traceflag > 1))
		{	
			SIprintf("select num from start - FAILED: %d\n",sqlca.sqlcode);
			SIflush(stdout);
		}
        	exec sql whenever sqlerror call merr;
/*
**
**	add this test to get past an oddity/bug
**
**	in high-concurrency, multi-STAR server applications, 
**	sometimes we get -2119 ERR_NOT_OWNER on the start
**	table.  STAR queries local with an <owner>.<table>
**	so we get -2119 back when the table does not exist
**	yet.
**
**	In either case, we want the slave to kick back and
**	wait for the START table, not panic.  OTHER errors
**	require a panic...
**
**	js	feb 12, 1992
**
*/

	} while ((sqlca.sqlcode < 0) && (loop++ < 60));

	if (loop < 60)
	{
		SIprintf("SLAVE #%d: Knows the start table is up \n",kid);
		SIflush(stdout);
	}
	else
	{
		do
		{
			PCsleep(5000);  /* give deadlocks a chance to clear */
			exec sql delete from slaves_up where slave_num = :kid;

		} while (sqlca.sqlcode == ERR_DEADLOCK);

		exec sql commit;
		exec sql disconnect;
		SIprintf("SLAVE #%d: Terminating because the START table does not exist\n", kid);
		SIflush(stdout);
		SIprintf("SLAVE #%d: Row %d has been deleted from SLAVES_UP\n",kid, kid);
		SIflush(stdout);

		exit(1);
	}

}



/*
** RUN_TESTS
**	This executes the list of routines (specified by rlist), reps
**	number of times. 
**
**	for as many times as reps indicates to loop though the routine list
**	{
**	   (if we get a deadlock, pop up to here, try the routine again)
**		for each routine in the list do
**		{
**			execute the TRL routine
**			(if we get a syserr, there's a bug, break from loops)
**			write to its internal data structure
**				the routine number
**		}
**		
**	}
**	Parameters - 
**		rlist -	list of routines to run. will cycle through reps times.
**		reps  -	how many times to cycle through rlist of routines.
**		slave -	which slave this is.
*/
run_tests(argv, rlist, reps, slave, bombptr)
char	*argv[];
struct	rout_list	*rlist;
i4	reps;
exec sql begin declare section;
	i4	slave;
exec sql end declare section;
i4	*bombptr;
{
exec sql begin declare section;	
	char	*dbname=argv[1];
	int 	et1, cpu1, dio1, bio1, pf1;	/* Initial values */
	int 	et2, cpu2, dio2, bio2, pf2;	/* Final values */
	int 	cnt;
	int	routine;
exec sql end declare section;

	i4 	i;				/* for loop index */
	i4	status;				/* error status */
	i4	bombadd;			/* flag for add_msg routine*/
	struct	rout_list	*savelist;	
	int     xact;
	float	persec;
	char	errstatus[2];			/* record type of error */
	i4	okerror;			/* flag for acceptable or
						   terminating errors */

	void    add_msg();

/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

	cnt = 1;			/* how many times we have attempted */
					/* to execute this test routine     */
					/* note: if error, cnt++, but i--   */
					/* to repeat the repetition. 	    */
	*bombptr = FALSE;

/*	SET AUTOCOMMIT OFF SO EACH ROUTINE CAN HANDLE ITS QUERIES SEPARATELY
**	SOME ROUTINES WILL HAVE MQT's AND OTHERS WILL HAVE MST's.
*/

/*
**	this "set " better succeed...
*/

	exec sql set autocommit off;

	if (sqlca.sqlcode < 0)
	{
		SIprintf("SLAVE #%d: Error %d: EXITING\n",slave,sqlca.sqlcode);

		SIflush(stdout);
		add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"A",&bombadd);

		*bombptr = TRUE;
		return;			
	}


	savelist = rlist;		/* so we can cycle through again */
        if (reps <= 0)
		reps = 300000;

	for (i=0; i< reps; i++)		/* do this "reps" times */
	{

		if (traceflag > 1)
		{	
			SIprintf("----------- EXECUTING REPETITION # %d -----------\n",i);
			SIflush(stdout);
		}

		while (rlist != NULL)		/* do until no more routines */
		{ 


			exec sql commit;	/* make sure that all transactions are committed */
						/* before a new routine is started */

			exec sql select int4(dbmsinfo('_et_sec')),
					int4(dbmsinfo('_cpu_ms')),
					int4(dbmsinfo('_dio_cnt')),
					int4(dbmsinfo('_bio_cnt')),
					int4(dbmsinfo('_pfault_cnt'))
					into :et1, :cpu1, :dio1, :bio1, :pf1;

			(*trl[rlist->number].func)(slave,cnt);	/* run it*/

			/*			
			** If no error, continue on
			** 
			** If an error has occurred, see if it is an acceptable error
			** - if so, record it & start the next rep of the test routine
			** - if not, record it and terminate
			** - if error rocording error, terminate
			*/

			if ((sqlca.sqlcode < 0) && (traceflag > 5))
			{
				SIprintf("ERROR after trl(): %d\n",sqlca.sqlcode);
				SIflush(stdout);
			}

			okerror = FALSE;	/* assume terminating error */

			switch (sqlca.sqlcode)
			{
			case ERR_DEADLOCK:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: DEADLOCK DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"a",&bombadd);
				break;

			case GE_SERIALIZATION:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: GE_SERIALIZATION DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"b",&bombadd);
				break;

			case ERR_OUT_OF_LOCKS:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_OUT_OF_LOCKS DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"c",&bombadd);
				break;

			case ERR_LOGFULL:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_LOGFULL DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"d",&bombadd);
				break;

			case GE_OTHER_ERROR:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: GE_OTHER_ERROR DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"e",&bombadd);
				break;

			case ERR_TIMEOUT:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_TIMEOUT DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"f",&bombadd);
				break;

			case USER_PASSWORD:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: USER_PASSWORD DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"g",&bombadd);
				break;

			case ERR_QUERYABORT:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_QUERYABORT DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"h",&bombadd);
				break;

			case ERR_CATDEADLK:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_CATDEADLK DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"i",&bombadd);
				break;

			case ERR_RQF_TIMEOUT:
				if (traceflag)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: ERR_RQF_TIMEOUT DETECTED\n",slave,rlist->number);
					SIflush(stdout);
				}
				okerror = TRUE;
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"j",&bombadd);
				break;

			case NO_ERROR:	/* no error encountered, continue on */
				if (traceflag > 5)
				{
					SIprintf("SLAVE #%d: CC_TEST #%d: NO ERRORS \n",slave,rlist->number);
					SIflush(stdout);
				}
				break;	

			default:
				SIprintf("SLAVE #%d: TRL_NO #%d: TERMINATING ERROR %d: EXITING\n",slave,rlist->number,sqlca.sqlcode);
				SIflush(stdout);
				add_msg(slave,rlist->number,i,cnt,sqlca.sqlcode,"C",&bombadd);
				*bombptr = TRUE;
				return;			

			}	/* end of switch */


			if (okerror == TRUE)
			{
				if (traceflag > 3)
				{
					SIprintf("okerror set to TRUE: %d\n",sqlca.sqlcode);
					SIflush(stdout);
				}

				if (bombadd)	/* if add_msg fails */
                        	{
					SIprintf("Bombadd set attempting to log an okerror\n");
					SIflush(stdout);
                        		*bombptr = TRUE;
                        		return;
                        	}

                                PCsleep(30000); /* Wait 30 secs. */

                                exec sql commit;/* cleanup current transaction*/
                                                /* commit add_msg log of error*/

                                if (sqlca.sqlcode < 0)
                                {
                                        SIprintf("SLAVE #%d: Commit error after okerror encountered: %d: EXITING\n",slave,sqlca.sqlcode);
                                        SIflush(stdout);
                                        add_msg(slave,rlist->number,i,cnt,sqlca.
sqlcode,"B",&bombadd);
                                        *bombptr = TRUE;
                                        return;
                                }

				i-- ;			/* repeat the test routine */
                                PCsleep(((cnt % 10) + 1) * 3000); 	/* Wait variable secs. */

				if (traceflag > 3)
				{
					SIprintf("i--: repeat repetition and wait: %d\n", ((cnt % 10) + 1) * 3000 );
					SIflush(stdout);
				}
			}

/*
** anyone executing beyond this point is because:
** - no error was encountered, OR
** - an okerror encountered, AND add_msg log of the error into err_msg succeeded
**
** everyone else failed to reach this point because
** - set autocommit off failed, OR
** - a terminating (not okerror) was encountered, OR
** - an okerror encountered, but add_msg into err_msg failed, OR
** - an okerror encountered, but commit failed
*/


			exec sql select int4(dbmsinfo('_et_sec')),
					int4(dbmsinfo('_cpu_ms')),
					int4(dbmsinfo('_dio_cnt')),
					int4(dbmsinfo('_bio_cnt')),
					int4(dbmsinfo('_pfault_cnt'))
					into :et2, :cpu2, :dio2, :bio2, :pf2;

			do
			{
				routine = rlist->number;
				exec sql insert into test_log 
					values (:dbname, 'now',
						:routine, :slave, :cnt, 
						:et2 - :et1, :cpu2 - :cpu1,
						:dio2 - :dio1, :bio2 - :bio1,
						:pf2 - :pf1);
				if (sqlca.sqlcode < 0 && sqlca.sqlcode != ERR_DEADLOCK)
				{
				  	SIprintf("\t\t*******************************\n");
				  	SIprintf("SLAVE %d: error appending into test_log %d\n",
							slave,sqlca.sqlcode);
				  	SIprintf("\t\t*******************************\n");
				  	SIflush(stdout);
				  	*bombptr = TRUE;
				  	return;
				};

				if (sqlca.sqlcode == ERR_DEADLOCK)
					PCsleep(5000);  /* give deadlocks a chance to clear */

			} while (sqlca.sqlcode == ERR_DEADLOCK);

			cnt++;			/* inc count of attempts to */
						/* execute the test routine */

			rlist = rlist->next;	/* point to next routine */

		}	/* bottom of  " while (rlist != NULL) " loop */

		if (i > 200000)
		{
			exec sql modify btbl_1 to merge;
	                i = 0;			/* reset i */
		}
		rlist = savelist;	/* done, recycle the list */
	}
	
	
/*	NOW THAT ALL ROUTINES HAVE BEEN EXECUTED SET AUTOCOMMIT ON */
	exec sql commit;
	exec sql set autocommit on;

} /* end run_tests*/

/*
** ADD_MSG 
**	takes the info passed to it and appends it to err_msg
*/
void
add_msg(slaven,routine,reps,cnt,err_num,status,bombflag)
exec sql begin declare section;
	i4	slaven;
	int	routine;
	i4	reps;
	int 	cnt;
	int	err_num;
	char	status[2];
exec sql end declare section;
i4	*bombflag;
{

/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

	*bombflag = FALSE;
	do
	{
		exec sql insert into err_msg (trl, slave,reps,counter,err_num,status)
			values (:routine, :slaven, :reps, :cnt, :err_num, :status);

		if (sqlca.sqlcode < 0 && sqlca.sqlcode != ERR_DEADLOCK)
		{
			SIprintf("\t\t*******************************\n");
			SIprintf("SLAVE %d: error appending into err_msg %d\n",
							slaven,sqlca.sqlcode);
			SIprintf("\t\t*******************************\n");
			SIflush(stdout);
			*bombflag = TRUE;
			return; 
		};

		if (sqlca.sqlcode == ERR_DEADLOCK)
			PCsleep(5000);  /* give deadlocks a chance to clear */

	} while (sqlca.sqlcode == ERR_DEADLOCK);

} /* end addmsg */



/*
** CLEANUP -
**	deletes a row from slaves_up to notify the coordinator that this
**	slave is done.
**
**	Parameters -
**		slave 	- this slave number
*/
cleanup(slave_number)
exec sql begin declare section;
	i4	slave_number;
exec sql end declare section;
{

/*	Use DBMS specific and NOT generic error messages */
	exec sql set_ingres (ERRORTYPE = 'dbmserror');

	do
	{
		exec sql delete from slaves_up where slave_num = :slave_number;

		if (sqlca.sqlcode < 0 && sqlca.sqlcode != ERR_DEADLOCK)
		{
			SIprintf("\n");
			SIprintf("SLAVE %d: Error removing slave# from slaves_up: %d\n", slave_number ,sqlca.sqlcode);
			SIprintf("SLAVE %d: May need to be removed manually\n");
			SIprintf("\n");
			SIflush(stdout);
			return; 
		}

		if (sqlca.sqlcode == ERR_DEADLOCK)
			PCsleep(5000);  /* give deadlocks a chance to clear */

	} while (sqlca.sqlcode == ERR_DEADLOCK);


	SIprintf("\n");
	SIprintf("SLAVE #%d: Terminating... \n", slave_number);
	SIprintf("SLAVE #%d: Row %d has been deleted\n", slave_number, slave_number);
	SIflush(stdout);

} /* end cleanup */
