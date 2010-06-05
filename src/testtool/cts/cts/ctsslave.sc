/*
**	mtsslave.sc
**
**	This program executes the routines for each slave in MST
**
**      14-may-1991 (lauraw)
**              Changed sqlprint() to CTS_errors().
**      14-may-1991 (jeromef)
**		Added a new error routine to deal with a Btree Architectural
**		problem (bug #37417).  When you are splitting large btree 
**		leaf pages with very large keys, there are incidences where
**		the split does not create enough space for the new key.  By
**		re-splitting the leaf page again, there usually is enough 
**		space.  Error routine forces a retry.
**      24-oct-1991 (jeromef)
**		Added an order by sequence number in the select from 
**		test_index table since the test index needs to be sorted 
**		by both slave and sequence within each slave
**	15-Dec-1993 (jeromef)
**		Included <st.h>.
**	02-Mar-1995 (canor01)
**		Added checks for other forms of deadlock that may be reported
**      11-Nov-1996 (stial01)
**              run_routine() Moved COMMIT and INQUIRE_INGRES(highstamp,lowstamp
**      08-Apr-1997 (toumi01)
**        Changed long to i4 to account for size differences on 
**        axp_osf.
**	18-Aug-2009 (drivi01)
**	  Cleanup precedence warnings in effort to port ot Visual Studio 2008.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**
**
NEEDLIBS = CTSLIB LIBINGRES 
*/
EXEC SQL INCLUDE SQLCA;

# include       <compat.h>
# include       <lo.h>
# include       <me.h>
# include       <mh.h>
# include       <si.h>
# include       <st.h>
# include       "trl.h"
# include	<cv.h>

EXEC SQL BEGIN DECLARE SECTION;
# define	ERR_DEADLOCK	4700	
# define	ERR_LOGFULL 	4706	
# define	OK_LO_ERROR	2171
# define	OK_HI_ERROR	2173
# define 	ERR_RETRIEVE	2117

# define	SQL_DEADLOCK	-49900

GLOBALDEF	int		ing_err ;

EXEC SQL END DECLARE SECTION;

GLOBALDEF	i4		traceflag;

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
**		 [4] -  trace flag
*/

main(argc, argv)
i4		argc;
 char		*argv[];
{
EXEC SQL BEGIN DECLARE SECTION;
	i4	test_num;	/* concurrency test number		*/
	i4	slave_num;	/* slave number				*/
	i4	reps;		/* how many times to execute this	*/
				/* slave's routine list			*/
	char	*dbname=argv[1];
EXEC SQL END DECLARE SECTION;

	i4	bomb;		/* flag to denote whether ingres bombed out   */

	struct	rout_list	*routines;	/* list of routines to execute*/

	FUNC_EXTERN 	int	trlsize();
	int	errproc();

	
/*	PROCESS THE ARGUMENTS AND DO SOME INITIALIZATION	*/

	routines = NULL;

	proc_args(argc,argv,&test_num,&slave_num,&traceflag);

	SIprintf("\nSLAVE #%d: Opening database...\n",slave_num);
	SIflush(stdout);


	EXEC SQL WHENEVER SQLERROR STOP;	/* stop if error occurs */
	EXEC SQL CONNECT :dbname;		/* while opening db	*/


	EXEC SQL WHENEVER SQLERROR CALL CTS_errors;	/* print error message*/ 
	EXEC SQL COMMIT;
	EXEC SQL SET AUTOCOMMIT ON;		/* commit each transaction */
						/* after it's done	   */

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
	    SIprintf("\nSLAVE #%d: Going to get synced up\n",slave_num);
	    SIflush(stdout);

/*	    SYNCHRONIZE WITH THE OTHER SLAVES TO START	*/

	    slave_sync(slave_num);
 	    SIprintf("\n\nSLAVE #%d: Finished getting synced up \n",slave_num);
	    SIprintf("       Running routines.... \n\n");
	    SIflush(stdout);

/*	    EXECUTE THE SPECIFIED TEST ROUTINES	*/

	    bomb = FALSE;

 	    run_routines(routines, reps, slave_num, &bomb);

	    if (bomb)
	    {
		SIprintf("BOMBING NOW...\n");
		SIflush(stdout);
	    }

	}

/*	CLEAN UP BEFORE EXITING */

	cleanup (slave_num);

/*	DISCONNECT FROM DATABASE AND THEN EXIT	*/

	EXEC SQL DISCONNECT;
	PCexit(OK);
}


/*
** PROC_ARGS
**	processes the arguments and does some initialization.
**	insures the cctest number, slave number, and trace flag are all
**	numeric.
**
**	Parameters -
**		routine_list -	points to the routine list pointer
**		argc	     -	how many arguments
**		argv	     -	element [2] is the cctest number
**			     -	element [3] is the slave number
**		cctest	     -	pointer to which concurrency test we're running
**		slave	     -	pointer to this slave's ID number
*/
proc_args(argc,argv,cctest,slavenu,trace)
i4	argc;
char	*argv[];
i4	*cctest;
i4	*slavenu;
i4 *trace;
{
	i4	status;

	if (argc != 5)
	{
		SIprintf("The syntax is: slave db_name cc_test slave_num \n");
		SIflush(stdout);
		PCexit();
	}

	if ( (status=CVal(argv[2],cctest)) != OK)
	{
		SIprintf("%s is an invalid Concurrency Test Number\n",argv[2]);
		SIflush(stdout);
		PCexit();
	}

	if ( (status=CVal(argv[3],slavenu)) != OK)
	{
		SIprintf("%s is an invalid Slave Number\n",argv[3]);
		SIflush(stdout);
		PCexit();
	}

	if ( (status=CVal(argv[4],trace)) != OK)
	{
		SIprintf("%s is an invalid Trace Flagr\n",argv[4]);
		SIflush(stdout);
		PCexit();
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
EXEC SQL BEGIN DECLARE SECTION;
	i4	test_num;
	i4	slave_num;
EXEC SQL END DECLARE SECTION;
	i4	*reps_ptr;
	struct rout_list	**rout_ptr;
{

EXEC SQL BEGIN DECLARE SECTION;
	i4	reps_num;	/* gets repetition number from DB */
	i4	seq_num;	/* gets sequent number from DB */
	i4	rout;		/* gets routine number from DB */
EXEC SQL END DECLARE SECTION;

	struct	rout_list	*routines;
	struct 	rout_list	*temp;		/* building list of routines */
	struct	rout_list	*current;
	i4	status;				/* error status */

	FUNC_EXTERN 	int	trlsize();		/* check range of trl */

	current = NULL;
	temp = NULL;
	routines = NULL;

/*	GET THE NUMBER OF REPETITION THIS SLAVE WILL PERFORM FROM THE	*/
/*	TEST_REPS TABLE.						*/

	EXEC SQL SELECT reps
		INTO :reps_num
		FROM test_reps
		WHERE cc_test = :test_num AND slave_num = :slave_num;

	*reps_ptr = reps_num;
	
/*
** this is a select loop for the routine numbers. we build a link list
** of them in the classic manner. with each test routine retrieved,
**	Loop until all routine number are retrieved
**	{
**		make a new structure for it
**		fill it with data
**		if its the first one, 
**			then make it the head of the list
**			else add it to the end of the list
**	}
*/

	EXEC SQL SELECT rout_num, seq
		INTO :rout, :seq_num
		FROM test_index
		WHERE cc_test = :test_num AND slave_num = :slave_num
		ORDER BY seq;
	EXEC SQL BEGIN;
		/*	first make sure the routine specified is valid	*/
		if (rout > trlsize())
		{
			SIprintf("SLAVE %d: SlaveInfo: routine #%d ",
							slave_num, rout);
			SIprintf("exceeds TRL's max routine #%d\n\n",trlsize());
			SIprintf("Routine #%d will be skipped \n",rout);
			SIflush(stdout);
		}
		else
		{
                        if ((temp = (struct rout_list *) MEreqmem (0,sizeof(struct rout_list),1,NULL)) == NULL)
			{
				SIprintf ("SLAVE %d: SlaveInfo: bad allocation\n", slave_num);
				SIflush (stdout);
				PCexit();
			}
	
			SIprintf ("\nSLAVE #%d: should run routine %d\n",slave_num,rout);
			SIflush (stdout);

/*			store routine number in the structure and then	*/
/*			connect together the link list.			*/

			temp->number = rout;
			temp->next = NULL;
	
			if (routines == NULL)		/* first one ? */
				routines = temp;
			if (current != NULL)		/* add to end of list */
				current->next = temp;
			current = temp;			/* our new tail */

		} /* end else rout > max */
	EXEC SQL END;

	SIprintf ("\nSLAVE #%d: Run each routine %d times\n",slave_num,reps_num);
	SIflush (stdout);
	
	*rout_ptr = routines;

} /* end db_info */



/*	SLAVE_SYNC -	Sync up the coordinator and all it's slaves,
**			such that they all exit this routine
**			at the same time.
**
**	Parameters -
**		kid -		which slave number this is.
**
**
** 	14-may-1991 (lauraw)
**		Changed second start message so it doesn't look like
**		a duplicate of the first.
**	28-apr-2009 (wanfr01)
**		Tune down sleep time
*/

slave_sync(kid)
EXEC SQL BEGIN DECLARE SECTION;
	i4	kid;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	i4	start_num;
	char	err_mess[257];
	char	err_mes2[257];
EXEC SQL END DECLARE SECTION;


/* FIRST CRUCIAL TIME POINT */
	/* First, insert a row to slaves_up so the coordinator knows	*/
	/* this slave is ready						*/

        SIprintf("\nSLAVE #%d: is ready to start \n",kid);
        SIflush(stdout);

	do
	{
		EXEC SQL INSERT INTO slaves_up (slave_num) VALUES (:kid);
		EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err == ERR_DEADLOCK);

	if (traceflag)
	{
		SIprintf("SLAVE #%d: Has appended number %d\n",kid,kid);
		SIflush(stdout);
	}

/* SECOND CRUCIAL TIME POINT */
	/* Now wait until the start table has been created */

	do
	{
		EXEC SQL SELECT num 
			INTO :start_num
			FROM start;
		EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
		if (ing_err == ERR_RETRIEVE)
		{
			PCsleep( (u_i4)5000 );
		}
	} while (ing_err != 0);

	SIprintf("SLAVE #%d: is starting \n",kid);
	SIflush(stdout);

}


/*
** run_routines
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
**
**      History -
**      11-Nov-1996 (stial01)
**              run_routine() Moved COMMIT and INQUIRE_INGRES(highstamp,lowstamp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
run_routines(rlist, reps, slave, bombptr)
struct	rout_list	*rlist;
i4	reps;
EXEC SQL BEGIN DECLARE SECTION;
	i4	slave;
EXEC SQL END DECLARE SECTION;
i4	*bombptr;
{
EXEC SQL BEGIN DECLARE SECTION;	
	int 	et, cpu, dio, bio;
	char	lock_state[24];
	int 	cnt;
	int	routine;
	i4	hi_stamp;		/* save ingres high and low	*/
	i4	lo_stamp;		/* stamps for serialization	*/
	i4	bit31_hi_stamp;		/* later on			*/
	i4	bit31_lo_stamp;		/* bit31 version for database	*/
	int	lo_bit;
	i4     in_transaction;
EXEC SQL END DECLARE SECTION;

	i4	i;				/* for loop index */
	i4	status;				/* error status */
	i4	bombadd;			/* flag for add_msg routine*/
	struct	rout_list	*savelist;	
	int     xact;
	float	persec;

	void    add_msg();
	cnt = 1;
	*bombptr = FALSE;

/*	SET AUTOCOMMI OFF SO EACH ROUTINE CAN HANDLE ITS QUERIES SEPARATELY
**	SOME ROUTINES WILL HAVE MQT's AND OTHERS WILL HAVE MST's.
*/
	EXEC SQL SET AUTOCOMMIT OFF;

	savelist = rlist;		/* so we can cycle through again */
        if (reps <= 0)
		reps = 300000;

	EXEC SQL SELECT _et_sec(), _cpu_ms(), _dio_cnt(), _bio_cnt()
		 INTO :et, :cpu, :dio, :bio;

	EXEC SQL COMMIT;

	for (i=0; i< reps; i++)
	{
		while (rlist != NULL)			/* another routine? */
		{ 
			/* make sure that all transactions are committed */
			EXEC SQL INQUIRE_INGRES (:in_transaction = TRANSACTION);
			if (in_transaction)
			{
			    EXEC SQL COMMIT;
			}

			(*trl[rlist->number].func)(slave,cnt);	/* run it*/


			if (ing_err == 393341)
			{
				SIprintf("SLAVE #%d: Test #%d: put error try again \n",slave,rlist->number);
				SIflush(stdout);
				(*trl[rlist->number].func)(slave,cnt);	
				SIprintf("SLAVE #%d: Test #%d: ing_err: %d \n",slave,rlist->number,ing_err);
				SIflush(stdout);
				if (ing_err == 0)
				{
				  SIprintf("SLAVE #%d: Test #%d: put retry passed\n",slave,rlist->number);
				  SIflush(stdout);
				}
				else
				{
				  SIprintf("SLAVE #%d: Test #%d: put retry faile\n",slave,rlist->number);
				  SIflush(stdout);
				}

			}

			if (ing_err == 196674                 /* RDF deadlock */
                            || sqlca.sqlcode == SQL_DEADLOCK)
				ing_err = ERR_DEADLOCK;

			/*			
			** IF ERROR IS DEADLOCK, BREAK OUT OF LOOP WITHOUT
			** INCREMENTING THE LIST'S POINTER.
			*/
			if (ing_err == ERR_DEADLOCK)
			{
				if (traceflag)
				{
				  SIprintf("SLAVE #%d: Test #%d: DEADLOCK DETECTED\n",slave,rlist->number);
				  SIflush(stdout);
				}
				add_msg(slave,rlist->number,hi_stamp,lo_stamp,
					i,cnt,ERR_DEADLOCK,"d",&bombadd);

				if (bombadd)
				{
					*bombptr = TRUE;
					return;
				}

				continue;
			}


			/*			
			** IF ERROR IS LOGFULL, BREAK OUT OF LOOP WITHOUT
			** INCREMENTING THE LIST'S POINTER SO THE ROUTINE
			** IS EXECUTED AGAIN, BUT BEFORE WAIT FOR FEW SECONDS
			** TO ALLOW THE ARCHIVER TO CLEAN UP THE LOG FILE.
			*/

			if (ing_err == ERR_LOGFULL)
			{
				if (traceflag)
				{
				  SIprintf("SLAVE #%d: Test #%d: FORCE ABORT LIMIT REACHED\n",slave,rlist->number);
				  SIflush(stdout);
				}
				add_msg(slave,rlist->number,hi_stamp,lo_stamp,
					i,cnt,ing_err,"f",&bombadd);

				if (bombadd)
				{
					*bombptr = TRUE;
					return;
				}

				PCsleep( (u_i4)30000 );

				continue;
			}

			/*
			** IF ERROR IS ANYTHING ELSE, STOP PROCESSING, AND EXIT.
			** WE MUST ASSUME THAT SOMETHING WENT WRONG. WE
			** THEREFORE CLOSE OUR CURRENT INGRES SESSION AND WRITE
			** OUR ERROR LOGS TO THE ERR_MSG TABLE. 
			*/
			else if (ing_err != 0)
			{
				SIprintf("SLAVE #%d: Test #%d: Error %d: EXITING\n",slave,rlist->number,ing_err);
				SIflush(stdout);
				add_msg(slave,rlist->number,hi_stamp,lo_stamp,i,cnt,ing_err,"b",&bombadd);

				*bombptr = TRUE;
				return;			
			}

			EXEC SQL COMMIT;
			EXEC SQL INQUIRE_INGRES (:hi_stamp = highstamp,
				:lo_stamp = lowstamp);
			lo_bit = ((lo_stamp >> 31) & 0x01);
			bit31_hi_stamp = (hi_stamp & 0x7fffffff);
			bit31_lo_stamp = (lo_stamp & 0x7fffffff);
	
			do
			{
				routine = rlist->number;
				EXEC SQL REPEATED INSERT 
					INTO test_log (rout_number, slave, 
						hi_stamp, hi_bit_lstamp, 
						lo_stamp, counter)
					VALUES (:routine, :slave, 
						:bit31_hi_stamp, :lo_bit,
						:bit31_lo_stamp, :cnt);
				EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
				if (ing_err == 196674      /* RDF deadlock */
                            	    || sqlca.sqlcode == SQL_DEADLOCK)
					ing_err = ERR_DEADLOCK;
				if (sqlca.sqlcode < 0 && ing_err != ERR_DEADLOCK)
				{
				  	SIprintf("\t\t*******************************\n");
				  	SIprintf("SLAVE %d: error appending into test_log %d\n",
							slave,ing_err);
				  	SIprintf("\t\t*******************************\n");
				  	SIflush(stdout);
				  	*bombptr = TRUE;
				  	break;
				};
			} while (ing_err == ERR_DEADLOCK);

			cnt++;
			rlist = rlist->next;		/*point to next routine*/
		}	/* end of while loop	*/

		if (i > 200000)
		{
			EXEC SQL MODIFY btbl_1 TO MERGE;
	                i = 0;			/* reset i */
		}
		rlist = savelist;	/* done, recycle the list */
	}	/* end of for loop	*/
	
	if (traceflag)
	{
		EXEC SQL SELECT _et_sec(), _cpu_ms(), _dio_cnt(), _bio_cnt()
		    INTO :et, :cpu, :dio, :bio;
		persec = (float)reps / (float)et;
		SIprintf("number of transaction completed = %d; per/sec = %f\n",
			reps, persec);
		SIprintf("et: %d  cpu: %d ms, dio: %d, bio: %d\n",
			et, cpu, dio, bio);
	}

/*	NOW THAT ALL ROUTINES HAVE BEEN EXECUTED SET AUTOCOMMIT ON */
	EXEC SQL COMMIT;
	EXEC SQL SET AUTOCOMMIT ON;

} /* end run_routines*/

/*
** ADD_MSG 
**	takes the info passed to it and appends it to err_msg
*/
void
add_msg(slaven,routine,histamp,lostamp,reps,cnt,err_num,status,bombflag)
EXEC SQL BEGIN DECLARE SECTION;
	i4	slaven;
	int	routine;
	i4	reps;
	int 	cnt;
	int	histamp;
	int	lostamp;
	int	err_num;
	char	status[2];
EXEC SQL END DECLARE SECTION;
i4	*bombflag;
{

	*bombflag = FALSE;
	do
	{
		EXEC SQL REPEATED INSERT INTO err_msg 
			(trl, slave,hi_stamp,lo_stamp,reps,counter,err_num,status)
			VALUES (:routine, :slaven, :histamp, :lostamp, :reps, :cnt, :err_num, :status);
		EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
		if (ing_err == 196674      /* RDF deadlock */
               	    || sqlca.sqlcode == SQL_DEADLOCK)
			ing_err = ERR_DEADLOCK;
		if (ing_err != 0 && ing_err != ERR_DEADLOCK)
		{
			SIprintf("\t\t*******************************\n");
			SIprintf("SLAVE %d: error appending into err_msg %d\n",
							slaven,ing_err);
			SIprintf("\t\t*******************************\n");
			SIflush(stdout);
			*bombflag = TRUE;
			return; 
		};
	} while (ing_err == ERR_DEADLOCK);

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
EXEC SQL BEGIN DECLARE SECTION;
	i4	slave_number;
EXEC SQL END DECLARE SECTION;
{

	do
	{
		EXEC SQL DELETE FROM slaves_up 
			WHERE slave_num = :slave_number;
		EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err == ERR_DEADLOCK);

	SIprintf("\nSLAVE #%d: Terminating... \n", slave_number);
	SIflush(stdout);

} /* end cleanup */
