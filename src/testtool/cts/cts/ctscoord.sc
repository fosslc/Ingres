/*
**	coord.sc
**
**	This program is the coordinator for CTS
**
**	History:
**	14-may-1991 (lauraw)
**		Changed sqlprint() to CTS_errors().
**	03-jun-1991 (jeromef)
**		save high bit from test_log and set autocommit to on
**	24-Oct-1991 (jeromef)
**		Turned off logging when the delete is executed between the
**		concurrent and serialize phase.  When deleting a large file,
**		the log file may become full, turning off logging will get 
**		around this problem.  
**	30-Oct-1991 (jeromef)
**		added commit statement before executing "ingdiff" to release
**		locks.
**	18-may-1992 (jeromef)
**		Added error routines for cursor M1 and M2 to allow the 
**		coordinator to exit gracefully.  
**	11-Oct-1993 (jeromef)
**		remove debug message, accidentally left in from last submit.
**
**	08-Apr-1997 (toumi01)
**      Changed long to i4 to account for size differences on 
**      axp_osf.
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**	10-sep-99 (inkdo01)
**	    Inserted trace point op134 to inhibit transform introduced by 
**	    implementation of Sir 98399. 
**
NEEDLIBS = CTSLIB LIBINGRES
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/
EXEC SQL INCLUDE SQLCA;
EXEC SQL INCLUDE SQLDA;

# include       <compat.h>
# include       <lo.h>
# include       <me.h>
# include       <mh.h>
# include       <pc.h>
# include       <si.h>
# include       <st.h>
# include	"trl.h"
# include	<cv.h>

# define	ERR_DESTROY	5202
# define	ERR_RETINTO	2135
# define	ERR_LOGFULL	4706
# define	ERR_DEADLOCK	4700
# define	ERR_CREATE	2010

/*
**	traceflag is a variable that is set by the user when he/she desires to
**	see status information about the slaves.  When traceflag is not set all
**	information that might show differences with the canons is filtered.
**	This flag should be set when the test is not executed as part of the
**	test suite.
*/

GLOBALDEF	i4		traceflag;

EXEC SQL BEGIN DECLARE SECTION;
GLOBALDEF	int		ing_err;
EXEC SQL END DECLARE SECTION;
  
/*
**	serial_list is the data structure for the list of routines the coord 
**	will execute. Each list member contains the routine number, the number
**	of the slave which executed it, the routine's count, and a pointer to 
**	the next member in the list.
*/
struct serial_list {
	i4	number;
	i4	slave;
	i4	count;
	struct	serial_list	*next;
	};
	
/*
**	tbl_list is used by the dumpNdiff routines to automatically "diff" the
**      tables the cctest uses as indicated by the test_tables relation.  The
**	structure stores a relation (or attribute) name, and a pointer to the
**	next name in the list.
**
**	The type field tells us whether the table is initially empty or
**	needs to be loaded. the field is unused in other routines. 
*/
struct tbl_list {
	char	tbl[20];
	char	type[2];
	struct	tbl_list	*next;
	};

/*
**	col_list is used by the dumpNdiff routines to automatically "diff" the
**      tables the cctest uses as indicated by the test_tables relation.  The
**	structure stores a relation (or attribute) column name, a numeric 
**	representation of the column data type, and a pointer to the
**	next name in the list.
**
**	The type field tells us whether the table is initially empty or
**	needs to be loaded. the field is unused in other routines. 
*/
struct col_list {
	char	tbl[20];
	int	data_type;
	struct	col_list	*next;
	};

/*
** COORDINATOR -
**	This is the coordinating program of the Concurrency Test System.
**  Its algorithm:
**
**	process arguments (# of slaves, cc_test number)
**	initialize data and log tables
**	sychronize slaves to start them
**	sleep; synchonrize to awaken when all have ended
**      dump the database; restore the tables to their initial state
**      sort the test_log table
**      run the test routines serialized
**      dump the database; compare the two dumps
**
**	Parameters -
**		argc -	argument count
**		argv -	argument array
**		 [0] -	points to the directory spec of this executable
**		 [1] -	database name
**		 [2] -	concurrency test number
**		 [3] -  number of slave programs to manage
**		 [4] -  traceflag
*/
main(argc, argv)
i4		argc;
char		*argv[];
{
EXEC SQL BEGIN DECLARE SECTION;
	CHAR	*dbname=argv[1];
EXEC SQL END DECLARE SECTION;

	i4	num_kids; 		/* number of slaves to manage */
	i4	cctest_num;		/* which concurrency test this is */
	struct	tbl_list	*datalist;  /* holds list of data tables used */
	i4	baderror;		/* flag to denote bad errors in slaves */

	int	errproc();		/* routine to handle INGRES errors */
		

/* 	PROCESS ARGUMENTS	 */

	proc_args(argc,argv,&num_kids,&cctest_num,&traceflag);
	
	SIprintf ("\nCOORDINATOR: Opening database...\n");
	SIflush(stdout);

	EXEC SQL WHENEVER SQLERROR STOP;	/* stop if error occurs while opening db*/
	
	EXEC SQL CONNECT :dbname;

	EXEC SQL WHENEVER SQLERROR CALL CTS_errors;	/* print error message when certain errors occur*/

	EXEC SQL COMMIT;
	EXEC SQL SET AUTOCOMMIT ON;		/* commit each transaction after it's done*/


/*	INITIALIZE LOG TABLES		*/

	init_logs(num_kids);

/*	SYNCHRONIZE SLAVES TO START THEM	*/

	SIprintf ("\nCOORDINATOR: Going to synchronize %d kids now\n",num_kids);
	SIflush(stdout);

	/* get the coord and all slaves starting at the same time */
	coord_sync(num_kids);

	SIprintf ("\nCOORDINATOR: waiting until all the slaves finish...\n");
	SIflush(stdout);


/*	SYNCHRONIZE TO AWAKEN WHEN ALL SLAVES ARE FINISHED	*/

	awaken_sync();

	SIprintf("\nCOORDINATOR: checking for errors in the concurrent run ...\n");
	SIflush(stdout);

/*	LOOK AT THE TABLE ERRLOG_LOG TO SEE IF THERE ARE CRITICAL BUGS THAT
**	OCCURRED WHILE THE SLAVES WERE RUNNING CONCURRENTLY.
*/
	baderror = FALSE;
	check_errors(&baderror);
	if (baderror)
	{	
		SIprintf("\t\t aborting due to bad errors, check slave result files \n");
		SIflush(stdout);
	}

	else
	{
		SIprintf("\t\t no critical errors found \n");
		SIflush(stdout);
	}

/*	DUMP THE DATABASE; RESTORE THE TABLES TO THEIR INITIAL STATE */

		SIprintf("\nCOORDINATOR: restoring the database...\n");
		SIflush(stdout);

		dumpNrestore(cctest_num,&datalist);

/*	SORT THE TEST_LOG TABLE -- RUN THE TEST ROUTINES SERIALIZED  */

	SIprintf ("\nCOORDINATOR: serializing... \n");
	SIflush(stdout);

	serialize();

/*	DUMP THE RESULTS OF THE SERIAL RUN AND COMPARE THE CONCURRENT AND
**	SEARIAL RUNS.
*/
	SIprintf("\nCOORDINATOR: dumping and diffing...\n");
	SIflush(stdout);

	dumpNdiff(datalist);		

	SIprintf ("\nCOORDINATOR: cleaning up and exiting ...\n");
	SIflush(stdout);

	
	cleanup(num_kids);

	if (traceflag)
	{
		SIprintf ("COORDINATOR: testing completed.\n");
		SIflush(stdout);
	}


	EXEC SQL DISCONNECT;

	PCexit(OK);
}

/*
** PROC_ARGS
**	Process the arguments. Find out if the correct number of arguments 
**	present, and how many slaves we have. 
**
**	Parameters -
**		argcount  - 	number of arguments passed to main
**		argvector - 	element [3] has the number of slaves
**		slaves    -	holds the number of slaves 
**		cctest	  -	to hold the cctest number we're executing
**		trace     -	traceflag status
*/
proc_args(argcount,argvector,slaves,cctest,trace)
i4	argcount;
char	*argvector[];
i4	*slaves;
i4	*cctest;
i4 *trace;
{
	i4	status; 	/* error status	*/

	if (argcount != 5)
	{
		SIprintf("The syntax is: coord db_name cc_test num_o_kids traceflag\n");
		SIflush(stdout);
		PCexit(-1);
	}


	if (( status=CVal(argvector[2],cctest) ) != OK)
	{
		SIprintf("%s is an invalid concurrency test number \n",
							        argvector[2]);
		SIflush(stdout);
		PCexit(-1);
	}

	if (( status=CVal(argvector[3],slaves) ) != OK)
	{
		SIprintf("%s is an invalid number of slave processes\n",
							        argvector[3]);
		SIflush(stdout);
		PCexit(-1);
	}

	if (( status=CVal(argvector[4],trace) ) != OK )
	{
		SIprintf("%s is an invalid trace flag \n", argvector[4]);
		SIflush(stdout);
		PCexit(-1);
	}
} /* end proc_args */

/*
**	INIT_LOGS -	Clear out the test_log, err_msg, num_slaves and seed
**			tables.
**      Parameters -
**              number of slaves for this test
**
*/
init_logs(num_kids)
EXEC SQL BEGIN DECLARE SECTION;
	i4  num_kids;
EXEC SQL END DECLARE SECTION;

{

	SIflush(stdout);
        do
	{
	  EXEC SQL DELETE FROM test_log;
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err  == ERR_DEADLOCK || ing_err == ERR_LOGFULL);

        do
	{
	  EXEC SQL DELETE FROM err_msg;
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err  == ERR_DEADLOCK || ing_err == ERR_LOGFULL);

        do
	{
	  EXEC SQL DELETE FROM num_slaves;
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err  == ERR_DEADLOCK || ing_err == ERR_LOGFULL);

        do
	{
	  EXEC SQL INSERT INTO num_slaves (num_kids)
		VALUES(:num_kids);
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	} while (ing_err  == ERR_DEADLOCK || ing_err == ERR_LOGFULL);

} /* end init_logs */



/*
**	COORD_SYNC -	Sync up the coord and all it's slaves, 
**			such that they all exit this routine
**					at the same time.
**
**	Parameters -
**		no_of_slaves	how many slaves we expect to sync up.
**
*/
coord_sync(no_of_slaves)
EXEC SQL BEGIN DECLARE SECTION;
	i4	no_of_slaves;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	i4	started_slaves;	   /* number of slaves already started up */
EXEC SQL END DECLARE SECTION;

/* 
**	Find out if all the slaves have gotten thier numbers and are ready to 
**	go. Each slave will append its number to slaves_up.
**	By counting how many rows the table has, the coordinator can tell
**	how many slaves are ready to go.
*/

	do 
	{ 
		EXEC SQL SELECT COUNT(*)
			INTO :started_slaves
			FROM slaves_up;

			if (traceflag)
			{
				SIprintf ("COORDINATOR: %d slaves out of %d are up\n",
						started_slaves, no_of_slaves);
				SIflush(stdout);
			}

		PCsleep( (u_i4)5000 );		
	} while (started_slaves != no_of_slaves);

/* FIRST CRUCIAL TIME POINT -- the coordinator knows the slaves are ready.*/

	EXEC SQL SET AUTOCOMMIT OFF;		/* start a multi query transaction */

/*
**	create the relation that tells the slaves that they can start
**	running.
*/
	EXEC SQL CREATE TABLE start (num i4);
	EXEC SQL INSERT INTO start (num)
		VALUES (0);

/* SECOND CRUCIAL TIME POINT -- the slaves know the coordinator is ready */

	SIprintf ("COORDINATOR: the slaves can start running now, the start table exists.\n");
	SIflush(stdout);

/*
** all the slaves are now ready and waiting to go. Releasing the lock
** on the start relation will allow them to run. 
*/
	EXEC SQL COMMIT;

	EXEC SQL SET AUTOCOMMIT ON;		/* start commiting all transactions after they are done */

}	/* end coord_sync */


/*
**	AWAKEN_SYNC --
**		Waits here until all the slaves have finished executing
**	their routines. Each slave deletes one row from the slaves_up
**	table when it is done so the coordinator can tell how many slaves 
**	are up. 
**
**	History:
**	28-Apr-2009 (wanfr01)
**	    Tune down sleep time
*/
awaken_sync()
{
EXEC SQL BEGIN DECLARE SECTION;
	i4 running_kids;	/* how many slaves are still running */
EXEC SQL END DECLARE SECTION;

	do 
	{ 
		EXEC SQL SELECT COUNT(*)
			INTO :running_kids
			FROM slaves_up;

			if (traceflag)
			{
				SIprintf ("COORDINATOR: %d slaves are STILL up\n",running_kids);
				SIflush(stdout);
			}

		if (running_kids > 0) 
			PCsleep( (u_i4)5000 );	
	} while (running_kids > 0);

} /* end awaken_sync */

/*
**	CHECK_ERRORS --
**	Look at ther error_log table in search of bad errors (deadlock
**	is not considered a bad error) and report them.
**
*/
check_errors (baderror)
i4	*baderror;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	letter[2];
	int	number;
	int	amount;
EXEC SQL END DECLARE SECTION;
	i4	status;

/*
**	Lets see how the test did.  Look at the error log table
**	and report all ingres "errors" (including deadlock and
**	force-abort-limit)
*/

EXEC SQL SELECT status, err_num, count(tid)
	INTO :letter, :number, :amount
	FROM err_msg
	GROUP BY err_num, status
	ORDER BY status;
EXEC SQL BEGIN; /* begin select loop */	

	if((status=STcompare(letter,"b")) == 0)
	{
		SIprintf("\t\t *** %d BAD INGRES ERRORS, error number %d\n",
							amount, number);
		SIflush(stdout);
		*baderror = TRUE;
	}
	else if ((status=STcompare(letter,"d")) == 0 && traceflag)
	{
		SIprintf("\t\t *** %d DEADLOCKS found, error number %d\n",
						amount, number);
		SIflush(stdout);
	}
EXEC SQL END; /* select loop */
} /* end check_errors */


/*
**	DUMPNRESTORE -
*/

dumpNrestore(cctest_num,dtlist)
EXEC SQL BEGIN DECLARE SECTION;
	i4	cctest_num;
EXEC SQL END DECLARE SECTION;
	struct tbl_list	**dtlist;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	tbl[33];		/* real tablename */
	char	itbl[33];		/* initial data table "i_tablename" */
	char	ctbl[33];	/* concurrent data table "c_tablename" */
	char	type[2];
	int	number;
	int	rcount;
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;
	struct	tbl_list	*temp;
	struct  tbl_list	*datatbls;

/*	Make a list of tables used in the list */

	temp = NULL;
	
EXEC SQL REPEATED SELECT tblname, init_type
	INTO :tbl, :type
	FROM test_tables t
	WHERE t.cc_test = :cctest_num;
EXEC SQL BEGIN; /* begin select loop */

	if ((datatbls = (struct tbl_list *) MEreqmem (0,sizeof(struct tbl_list), 1,NULL)) == NULL)
	{
		SIprintf("COORDINATOR: DumpNrestore bad allocation \n");
		SIflush (stdout);
	}

	STcopy(tbl, datatbls->tbl);
	STcopy(type,datatbls->type);
	datatbls->next = temp;
	temp = datatbls;
EXEC SQL END;  /* end select loop */


        EXEC SQL INQUIRE_INGRES (:rcount = rowcount);
	if (rcount == 0)
	{
		datatbls = NULL;
	}

	*dtlist = datatbls;

	/*
	** For each data table used append the c table with the contents
	** of the concurrent run.  Clear out the concurrent results.  If
	** this table requires initialization, do so.
	*/

	while (datatbls != NULL)
	{
	  STcopy(datatbls->tbl,tbl);
	  STcopy("c_",ctbl);
	  STcat(ctbl,tbl);

	 	do
		{
		  STprintf(prepare_buffer, "CREATE TABLE %s AS SELECT * FROM %s ", 
			ctbl, tbl);
		  EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
		  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);

		  if (ing_err == ERR_CREATE )  /* if the table already existed, 
						destroy it and create a new one */
		  {
		    STprintf(prepare_buffer, "DROP %s ", ctbl);

		    EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
		    EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);

		    STprintf(prepare_buffer, "CREATE TABLE %s AS SELECT * FROM %s ", 
			ctbl, tbl);
		    EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
  		    EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
		  }

		  if (ing_err == ERR_LOGFULL || ing_err == 4709)
		  {
			PCsleep( (u_i4)5000 );		
		  }
		} while (ing_err == ERR_LOGFULL || ing_err == 4709);


/*		Modify the table to btree to make the comparison with
**		the serialized run more efficient.
*/

		do
		{
		  STprintf(prepare_buffer, "MODIFY %s TO BTREE", ctbl);

		  EXEC SQL EXECUTE IMMEDIATE :prepare_buffer ;
		  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);

		}  while (ing_err == ERR_LOGFULL);

		do
		{
		  EXEC SQL COMMIT;
		  EXEC SQL SET NOLOGGING;

		  STprintf (prepare_buffer, "DELETE FROM %s ", tbl);

		  EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
		  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);

		} while (ing_err == ERR_LOGFULL);

		  EXEC SQL COMMIT;
		  EXEC SQL SET LOGGING;

		if (!STcompare("i",datatbls->type))
		{
			STcopy("i_",itbl);
			STcat(itbl,tbl);
			do
			{
				STprintf (prepare_buffer,
					"INSERT INTO %s SELECT * FROM %s",
					tbl, itbl);
				EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
				EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR,
							 :number = rowcount);
			} while (ing_err == ERR_LOGFULL);
						
		 SIprintf("\nCOORDINATOR: %s initialized ", tbl);
		 SIprintf("with %d rows from %s\n", number,itbl);
		 SIflush(stdout);
		} 
		datatbls = datatbls->next;
	} /*end while */	


} /* end dumpNrestore */



/*
** SERIALIZE --
**	Selects the log table into its own internal data structure, then
**	runs each routine.  The data table has already been cleared, and
**	saved.
**	This is the SERIALIZED run of the CTS.
**
**	The serial_list data structure contains a routine number
**	and a pointer to the next element in the list, and is
**	defined (along with trl) in trl.h
*/
serialize()
{
EXEC SQL BEGIN DECLARE SECTION;
	i4	rout;		/* gets routine number from DB */
	float	high;		/* to sort routine number by stamps.  */
	int	bit;		/*	"	"		*/
	float	low;		/*	"	"		*/
	i4	slave;		/* slave number */
	i4	counter;	/* and counter, for diagnostics	*/
EXEC SQL END DECLARE SECTION;

	struct	serial_list	*routines;
	struct  serial_list	*temp;		/* building list of routines */
	struct  serial_list	*current;
	i4	status;				/* error status */

	current = NULL;
	temp    = NULL;
	routines = NULL;

/*
**	This is a select loop for the routine numbers.  We build a link list
**	of them in the classic manner - with each test routine selected,
**	1. make a new structure for it.
**	2. fill it with data.
**	3. if this routine is the first one, make it the head of the list.
**	4. if not the first routine,  add it to the end.
**
*/

EXEC SQL REPEATED SELECT rout_number, hi_stamp, hi_bit_lstamp, lo_stamp, slave, counter
	  INTO :rout, :high, :bit, :low, :slave, :counter
	  FROM test_log
	  ORDER BY hi_stamp, hi_bit_lstamp, lo_stamp;
EXEC SQL BEGIN;	/* begin select loop */

	  if ((temp = (struct serial_list *) MEreqmem (0,sizeof(struct serial_list), 1,NULL)) == NULL)
	  {
		SIprintf("COORDINATOR: Serialize: bad allocation\n");
		SIflush (stdout);
		PCexit(-1);
	  }

	  temp->number = rout;
	  temp->slave  = slave;
	  temp->count  = counter;
	  temp->next   = NULL;

	  if (routines == NULL)		/* first one? */
		routines = temp;
	  if (current != NULL)		/* add to end of list */
		current->next = temp;
	  current = temp;		/* our new tail */
EXEC SQL END;	/* end select loop */
	  		
	  status = 0;		/* counter for future diagnostics	   */

	EXEC SQL SET AUTOCOMMIT OFF;	/* Turn off autocommit 	*/

	  while (routines != NULL)	/* another routine? */
	  {
		EXEC SQL COMMIT;	/* commit each transaction after */
					/* it's done		 	 */

		(*trl[routines->number].func)(routines->slave, routines->count);
		routines = routines->next;	/* point to next one */
		status++;		/* counter for future diagnostics */
	  } /* end while */

} /* end serialize */

/*
**	DUMPNDIFF --
**	In its new and improved form, this procedure looks at the
**	data in the concurrent run and in the serial run, and via ingres,
**	determines if they are different.  The general algorithm is:
**	1. Build a list of all data tables used by this cc_test.
**	2. for each table:
**		Build a list of attributes for that table.
**		Send it off to "ingdiff" to be diffed.
**	Since the serial run's data table is of the form "c_tablename", the
**	"ingdiff" routine need only know the tablename and its attributes to
**	perform the diff.  Briefly, ingdiff checks the number of rows in each
**	table, and makes sure all rows in the concurrent table exist in the
**	serial one, and vice versa.
*/
dumpNdiff(t)
struct tbl_list	*t;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	tbl[33];
	char	attcol[33];
	int	col_typ;
EXEC SQL END DECLARE SECTION;
	struct	col_list	*old_ptr;
	struct	col_list	*col_name_lst;

/*	Use the list of all data relations this test uses passed in.
**
**	With each relation, make a list of its attributes.  Pass all
**	of this information down to ingdiff, which will actually do
**	the comparing.
*/

	while (t != NULL)
	{
		old_ptr = NULL;
		STcopy(t->tbl,tbl);

		/* tbl = t->tbl; loads table name into tbl for the ingres
		select */

		EXEC SQL REPEATED SELECT attname, attfrmt
		INTO :attcol, :col_typ
		FROM iiattribute, iirelation
		WHERE iirelation.relid = :tbl and iirelation.reltid =
		      iiattribute.attrelid and iiattribute.attrelidx = 0;
		EXEC SQL BEGIN;	/* select loop begin */

		    SIflush(stdout);
		    if ((col_name_lst = (struct col_list *) MEreqmem (0,sizeof(struct col_list), 1,NULL)) == NULL)
		    {
			SIprintf("COORDINATOR: Col Name Dump: bad allocation\n");
			SIflush(stdout);
			EXEC SQL ENDSELECT; /* exit select loop */
			PCexit(-1);
		    }
		    STtrmwhite(attcol);	/* trim if attrib is < 12 chars	long */
		    STcopy(attcol,col_name_lst->tbl);	/* col_name_lst->tbl = atbl; */
		    col_name_lst->data_type = col_typ; /* save column type */
		    col_name_lst->next = old_ptr;
		    old_ptr = col_name_lst;
		EXEC SQL END; /* select loop end */

		EXEC SQL COMMIT; 

		ingdiff (tbl,col_name_lst);	/* perform the diff */

		t = t->next;		/* next table */

	} /* end while t not NULL */	 
		      
	
} /* end dumpNdiff */

/*
**	INGDIFF --
**	This procedure does 3 selects to determine if two tables
**	are different.  The serialized data is in table(s) of the
**	form "tablename"; the concurrent data is in table(s) of
**	of the form "c_tablename".
**	First, it checks to see if the number of rows in each of the
**	tables is the same.  Then it checks if each row in the concurrent
**	table has a corresponding row in the serialized table.  Finally,
**	it checks if each row in the serialized table has a corresponding
**	row in the concurrent table.  It uses procedure "makequal" to create
**	the somewhat complex where clause, crucial to the latter two retrieves.
**
**	Parameters:
**		tbl   = name of the serialized data table
**		alist = list of attributes for that table
**
*/
ingdiff(tbl,alist)

EXEC SQL BEGIN DECLARE SECTION;
	char	tbl[33];
EXEC SQL END DECLARE SECTION;
	struct	col_list *alist;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	scount;
	int	ccount;
	int	rowcount;
	int	qry_end;
	char	*qual;
	char	ctbl[33];
	char	which[2];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;
	IISQLDA	_sqlda;                  /* define dynamic storage  */
	IISQLDA	*sqlda = &_sqlda;

	sqlda->sqln = IISQ_MAX_COLS;     /* define the size */

	STcopy("c_",ctbl);
	STcat(ctbl,tbl);

	SIprintf("\nCOORDINATOR: comparing table %s\n",tbl);
	SIprintf("\t Looking at the rowcount: ");
	SIflush(stdout);

	STprintf (prepare_buffer, "SELECT count(c.tid) FROM %s c", ctbl);
	EXEC SQL DECLARE cnt CURSOR FOR :prepare_buffer;
	EXEC SQL OPEN cnt;
	EXEC SQL FETCH cnt INTO :ccount;
	EXEC SQL CLOSE cnt;

	STprintf (prepare_buffer, "SELECT count(s.tid) FROM %s s", tbl);
	EXEC SQL DECLARE scnt CURSOR FOR :prepare_buffer;
	EXEC SQL OPEN scnt;
	EXEC SQL FETCH scnt INTO :scount;
	if(ccount == scount)
	{
		SIprintf(" Same number of rows...\n");
		SIflush(stdout);
	}
	else
	{
		SIprintf("Row count is DIFFERENT table1=%d table2=%d ...\n", 
			ccount, scount);
		SIflush(stdout);
	}		
        EXEC SQL CLOSE scnt;

/*	EXEC SQL SET QEP;			*/
/*	EXEC SQL SET TRACE POINT QE90;		*/
	EXEC SQL SET NOTRACE POINT OP132;
	EXEC SQL SET TRACE POINT OP133;
	EXEC SQL SET TRACE POINT OP134;
/*	SIprintf("\n\tSet NOTrace OP132 \n\tSet Trace op133 \n\n");	*/

	STcopy("c",which);		/* we want C compared to S */
	makequal (which,&qual,alist,tbl, ctbl);	/* build the qual clause */

	SIprintf("\nChecking for non-matching 'concurrent' tids...\n");
	SIflush(stdout);

/*	result should be "SELECT c.tid INTO scount FROM ctbl c WHERE qual"   */
	STcopy ("SELECT c.tid FROM ", prepare_buffer); 
	STcat (prepare_buffer, ctbl);
	STcat (prepare_buffer, " c WHERE "); 
	STcat (prepare_buffer, qual);
	scount = 0;

	if (traceflag)			/* let's see what we're checking */
	{
		SIprintf("\n%s\n", prepare_buffer);
		SIflush(stdout);
	}

	EXEC SQL DECLARE m1 CURSOR FOR :prepare_buffer;
	EXEC SQL OPEN m1 FOR READONLY;

	do
	{
		EXEC SQL FETCH m1 INTO :ccount;
	        EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR,
					 :qry_end = endquery, 
					 :rowcount = rowcount);
		if (rowcount == 1 )
		{
			scount = scount+rowcount;
			SIprintf("\t\t non concurrent tid = %d, rowcount=%d\n", 
				ccount, scount);
		}
		SIflush(stdout);

		if (ing_err != OK)
		{
		  SIprintf("\nCursor M1 error occurred\n");
		  SIflush(stdout);
		  return;
		}

	} while (qry_end == 0);	/* endquery = 1 if fetch is beyond last row */
	
	EXEC SQL CLOSE m1;

	if (scount == 0)			/* if all rows match */
	{
		SIprintf("\tAll 'concurrent' tids match 'serialized' data\n");
		SIflush(stdout);
	}

	STcopy ("s", which);			     /* now compare S to C */
	makequal (which, &qual, alist, tbl, ctbl);   /* build the qual clause */

	SIprintf("\nChecking for non-matching 'serialized' tids...\n");
	SIflush(stdout);

/*	result should be "SELECT s.tid INTO scount FROM tbl s WHERE qual"   */
	STcopy ("SELECT s.tid FROM ", prepare_buffer); 
	STcat (prepare_buffer, tbl);
	STcat (prepare_buffer, " s WHERE "); 
	STcat (prepare_buffer, qual);
	ccount = 0;

	if (traceflag)			/* let's see what we're checking */
	{
		SIprintf("\n%s\n", prepare_buffer);
		SIflush(stdout);
	}

	EXEC SQL DECLARE m2 CURSOR FOR :prepare_buffer;
	EXEC SQL OPEN m2 FOR READONLY;

	do
	{
		EXEC SQL FETCH m2 INTO :scount;
	        EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR,
					 :qry_end = endquery,
					 :rowcount = rowcount);
		if (rowcount == 1) 
		{
			ccount = ccount + rowcount;
			SIprintf("non serialized tid = %d, rowcount=%d\n", 
				scount, ccount);
			SIflush(stdout);
		}

		if (ing_err != OK)
		{
		  SIprintf("\nCursor M2 error occurred\n");
		  SIflush(stdout);
		  return;
		}

	} while (qry_end == 0);	/* endquery = 1 if fetch is beyond last row */

	if (ccount == 0)
	{
		SIprintf("\tAll 'serialized' tids match 'concurrent' data\n");
		SIflush(stdout);
	}

	EXEC SQL CLOSE m2;

} /* end ingdiff */

/*
**	MAKEQUAL --
**	This builds the qual clause for our selects that checks rows
**	against each other.  This clause will end up looking like:
**		not exists (select * from c_tbl c
**				where s.col1 = c.col1
**				  and s.col2 = c.col2
**					     .
**					     .
**					     .
**				  and s.coln = c.coln );	
**
**	Parameters:
**		ch	= range variable for the target table (c or s)
**		qual	= pointer passed back, points to the qual string
**		atts	= list of attributes for these tables
*/
makequal(ch, qual, atts, tbl, ctbl)
char	ch[];
char	**qual;
struct	col_list	*atts;
char	tbl[];
char	ctbl[];
{
	static	char	qualspace[1000];
	struct	col_list	*attholder;
	char	alt[2];
	char	compare_tbl[33];

	attholder = atts;	/* save a copy of the head of the list */

	/*
	** For the two range variables, c and s: if the target table
	** being compared is c, then the alternate is s; and vice-versa.
	** This keeps the range variables straight.
	*/
	if (!STcompare("c",ch))
	{
		STcopy("s",alt);
		STcopy(tbl, compare_tbl);
	}
	else
	{
		STcopy("c",alt);
		STcopy(ctbl, compare_tbl);
	}

/*	This is the beginning of the any clause; any(s.tid sort by s.tid
*/

	STprintf(qualspace, " %s.%s not in ( select %s.%s from %s %s ",
		ch, atts->tbl, alt, atts->tbl, compare_tbl, alt);

	/*
	** Add the where clause now;
	**	any (s.tid sort by s.tid where s.col1=c.col1...)
	*/

	STcat(qualspace, " where");

	/*
	** With each attribute, add s.attributename = c.attributename to
	** the where clause.
	** At the end of this loop, the where clause is complete:
	**      any(s.tid sort by s.tid where s.col1 = c.col1 and s.col2=
	**	c.col2..)
	*/

	while (atts != NULL)
	{
	  if (atts->data_type > 0)
	  {
		STprintf(qualspace,"%s %s.%s = %s.%s", qualspace,ch,atts->tbl,
								alt,atts->tbl);
	  }
	  else
	  {
	    switch (atts->data_type)
	    {
	    case -30: 
	    case -31: 
	    case -5:
			STprintf(qualspace,"%s IFNULL(%s.%s,-1) = IFNULL(%s.%s,-1)",
				qualspace, ch, atts->tbl, alt, atts->tbl);
			break;
	    case -32: 
	    case -37: 
	    case -20: 
	    case -21:
			STprintf(qualspace,"%s IFNULL(%s.%s,\'NULL\') = IFNULL(%s.%s,\'NULL\')",
				qualspace, ch, atts->tbl, alt, atts->tbl);
			break;
	    case -3:
			STprintf(qualspace,"%s IFNULL(%s.%s,\'01-jan-1882\') = IFNULL(%s.%s,\'01-jan-1882\')",
				qualspace, ch, atts->tbl, alt, atts->tbl);
			break;
	    default:
			SIprintf("\tUndefined data type defined for column %s\n", 
				atts->tbl);
			SIflush(stdout);
			break;
	    }	/* end switch	*/
	  }
	  atts =atts->next;
	  if (atts != NULL)
	    STcat(qualspace," and");
	}

	/*
	** Close up the statement, yeilding;
	**	any (s.tid sort by s.tid s.col1=c.col1 and s.col2=...)=0;
	*/

	STcat(qualspace,")" );

	*qual = qualspace;		/* return the whole mess */
	
} /* end makequal */	

			
/*
** CLEANUP -
**	destroys the start table which is used for synchronization purposes
**	and deletes any rows in the slaves_up table.
**
**	Parameters -
**		slaves	- how many slaves there are
*/
cleanup(slaves)
i4	slaves;
{

	/* First destroy the table that has the number of running slaves */

	EXEC SQL DROP TABLE start; 
	EXEC SQL DELETE FROM slaves_up;
	
} /* end cleanup */
