/*
** TRL.SC
**	This is the test routine library (TRL) for CTS/MTS. Each routine
**	represents a transaction that a CTS slave execute. You can find
**	out which routines are executed by a CTS test by looking at the
**	table test_index in the CTS database. This table contains an
**	index of how many slaves a test has and which are the routines
**	that each slave executes. The table test_reps has the information
**	about how many times each routine should be executed. This
**      table has one entry for each test and each slave in that test.
**
**      There are three important considerations when you plan to add
**	routines to this library.
**	1. If you are adding a routine to be executed by CTS you should
**	   put one transaction (SQT or MQT) per routine. This is necessary
**	   because CTS needs to keep track of the commit stamps of each
**	   transaction. If the routine is going to be excuted by MTS you 
**	   don't have this restriction. 
**      2. There are two parameters that are passed to each routine: the
**	   number of the slave that is executing the routine and the
**	   repetition number. If you need more information for the routine
**	   you can put that information in a table which is initialized by
**	   the coordinator or the slave.
**      3. To be able to execute a routine as part of a CTS test you need
**	   to add information to the table test_index and test_reps about
**	   the routine and the test. See the CTS documentation for more
**	   information on how to do that.
**
**	History:
**
**      14-may-1991 (lauraw)
**              Changed sqlprint() to CTS_errors().
**
**      05-jun-1991 (jeromef)
**              Updated trl with new routines 26, 27, 28, 29, and 30.  They are
**		new tests for testing the new Btree index compression routines.
**		There are specific comments in each routine.  Also cleaned up
**		some code and added form feed between each routine.
**      24-oct-1991 (jeromef)
**		Removed the rollback to savepoint statement since it was 
**		unnecessary and cause an extra error message to be reported.
**      18-may-1992 (jeromef)
**		Added test routines 31, 32 and 33 to test the SQL ROLLBACK
**		feature.
**      26-aug-1993 (dianeh)
**		Added #include <st.h>.
**      10-Dec-1993 (Jeromef)
**		Added an "if" clause to deal with a divide by zero problem.  
**		In tst26 through tst29, there is "mod" command whose result
**		is used for a division.  The "if" will change the result from
**		zero to the "mod" divisor value.
**	22-Jan-1994 (fredv)
**		Move the "if" after STcopy() in tst26 to tst29 because
**		there is no array[7]. Try to do STcopy(array[7], array_buffer)
**		will cause SEGV.
**	02-mar-1995 (canor01)
**		Change the "f4 = f4 * 3" to "f4 = f4 * 1.02" in tst6 to
**		prevent floating point overflow, which is now checked for
**      13-aug-1997 (stial01)
**              tst21: After UPDATE, INQUIRE_INGRES again.
**              We may deadlock trying to get the lock on the new page.
**              tst20,tst21,tst24,tst28: Increase MAXLOCKS so slaves don't get 
**              into escalate-deadlock loop.
**      10/26/99 (hweho01)
**              Added IILQint() function prototype. Without the 
**              explicit declaration, default int return value for a function 
**              will truncate a 64-bit address on ris_u64 platform. 
**      02/04/2000 (stial01)
**              Added a message to tst4 case 4, to explain row count
**              differences when running with lock level row, isolation level
**              cursor stability or repeatable read.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      31-jul-2001 (stial01)
**          Fixed deadlock handling in tst4, case 3 (b105392)
**	31-mar-2003 (somsa01)
**	    Changed return type of IILQint() to "void *".
**      03-mar-2003 (legru01)
**          Added two routines (tst34 & tst35) 
**          TST34: will be used to check 
**          a single sequence object operation when multiple slaves 
**          are executing the routine against that object. 
**          TST35: will be used to check
**          the operation of multiple sequence objects when multiple slaves
**          are executing the routine.
**	31-mar-2003 (somsa01)
**	    Changed return type of IILQint() to "void *".
*/

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL CTS_errors;	/* print the error out	*/
	
# include       <compat.h>
# include       <lo.h>
# include       <me.h>
# include       <mh.h>
# include       <si.h>
# include       <st.h>
# include       <tm.h>

# define	ERR_DEADLOCK	4700
# define	ERR_LOGFULL 	4706
# define	ERR_DROP	2753
# define	ERR_CREATE 	5102
# define	ERR_DUPLICATE	2010

# define	BRANCHS_PER_BANK	10
# define	TELLERS_PER_BRANCH	10
# define	ACCOUNTS_PER_BRANCH	1000
# define	LARGEST_DEPOSIT		1000
# define	MAX_SCAN		100
# define	TBLSIZE			5120		/* used for tst22 */

FUNC_EXTERN    VOID * IILQint();

extern		i4		traceflag;
EXEC SQL BEGIN DECLARE SECTION;
extern		int		ing_err;
EXEC SQL END DECLARE SECTION;

int	tst0();
int 	tst1();
int 	tst2();
int 	tst3();
int 	tst4();
int 	tst5();
int 	tst6();
int 	tst7();
int 	tst8();
int 	tst9();
int	tst10();
int	tst11();
int	tst12();
int	tst13();
int	tst14();
int	tst15();
int	tst16();
int	tst17();
int	tst18();
int	tst19();
int	tst20();
int	tst21();
int	tst22();
int	tst23();
int	tst24();
int	tst25();
int	tst26();
int	tst27();
int	tst28();
int	tst29();
int	tst30();
int	tst31();
int	tst32();
int	tst33();
int	tst34();
int     tst35();

GLOBALDEF	struct	trl_routine {
	int	(*func)();
} trl[] = {	
	tst0,
	tst1,
	tst2,
	tst3,
	tst4,
	tst5,
	tst6,
	tst7,
	tst8,
	tst9,
	tst10,
	tst11,
	tst12,
	tst13,
	tst14,
	tst15,
	tst16,
	tst17,
	tst18,
	tst19,
	tst20,
	tst21,
	tst22,
	tst23,
	tst24,
	tst25,
	tst26,
	tst27,
	tst28,
	tst29,
	tst30,
	tst31,
	tst32,
	tst33,
	tst34,
        tst35 
};

int 
trlsize()
{
      return ( (sizeof(trl)/sizeof(struct trl_routine)) - 1);
}
	
/*
** Function TST0
** Purpose:   This routine is intended to be used as a test to verify that
**	      CTS/MTS are working.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst0(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL INSERT INTO data0(num,slave,routs,reps)
		values (100, :s, 0, :c);
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
		return;
	}
}

	
/*
** Function TST1
** Purpose: Insert one row according to the slave number, the repetition
**	    number and the total number of slaves.  This routine should 
**	    be executed by several slaves such that they insert different 
**	    rows in ascending order.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst1(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int value;
	int num = 0;
EXEC SQL END DECLARE SECTION;


  EXEC SQL SELECT num_kids 
	INTO :num
	FROM num_slaves;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  
  value = s + (num * c);
  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 1, pass %d, inserting %d \n",s,c, value);
	SIflush (stdout);
  }

  EXEC SQL REPEATED INSERT INTO data1 (tbl_key , rout , slave , cnt )
	VALUES (:value, 1, :s, :c);

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }
}

	
/*
** Function TST2
** Purpose: This routine inserts values in descending order.  It should be used
**          with a btree to test page splits and sorting.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst2(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	value;
	int	num;
EXEC SQL END DECLARE SECTION;


  EXEC SQL SELECT num_kids 
	INTO :num
	FROM num_slaves;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);

  value = s + (500 - (num * c));  
  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 2, pass %d, inserting %d \n",s,c, value);
	SIflush (stdout);
  }
  EXEC SQL REPEATED INSERT INTO data1 (tbl_key , rout , slave , cnt )
	VALUES (:value, 2, :s, :c);
  
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }
}

	
/*
** Function TST3
** Purpose: insert one row using the passed number as the key.  In this test
**          the slaves would be inserting rows with the same key such that
**          the table will have several duplicate keys.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst3(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 3, pass %d, inserting %d \n",s,c,c);
	SIflush (stdout);
  }
  EXEC SQL REPEATED INSERT INTO data1 (tbl_key , rout , slave , cnt )
	VALUES (:c, 3, :s, :c);
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  
  if (ing_err  != 0)
  {
	return;
  }
}

	
/*
** TST4
** Purpose: This routine uses a random number generator to decide which
**	    transaction to execute.  The seed for the random number is
**	    placed in the table seeds by the coordinator, and corresponds
**	    to the time of day.  Note that only one transaction will be
**	    executed in each pass.  Most transactions use repeat queries
**	    and perform operations that require exclusive locks.
**	    Note also the use of PCsleep (mainly for the VAX/VMS systems)
**	    to prevent deadlock thrashing.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst4(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	num, r, m, i, seed;
	int	qry_end;
	int	i2_var;
	int	i4_var;
	int	trans;
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  EXEC SQL REPEATED SELECT seed_val
  INTO :seed
  FROM seeds;

  MHsrand((i4)((seed + s) * c)); 

  r   = (i4) ((MHrand() * ( MHrand() * (f8) (32767))) +1);
  m   = r%6;

  if (traceflag)
  {	
	SIprintf ("Running SLAVE %d: Rout 4, pass %d, using case %d \n",
        s, c, m );
	SIflush (stdout);
  }

  switch (m)
  {
  case 0:
	if (traceflag)
	{
		SIprintf ("\t\t inserting tbl_key = %d \n", r);
		SIflush (stdout);
	}

	EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
				  char61a, char100)
	VALUES (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999',
	        'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

	break;			

  case 1:
	if (traceflag)
	{
		SIprintf("\t\t inserting & deleting tbl_key = %d \n",r);
		SIflush(stdout);
	}

	EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
				  char61a, char100)
	VALUES (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999',
	        'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
		break;
	}

	if (traceflag)
	{
		SIprintf ("\t\t case 1 delete WHERE data2.i2 = %d \n", r);
		SIflush (stdout);
	}

	EXEC SQL REPEATED DELETE FROM data2
	WHERE data2.i2 = :r;

	break;

  case 2:
	if (traceflag)
	{
		SIprintf("\t\t updating tbl_key = %d \n",r);
		SIflush(stdout);
	}

	EXEC SQL UPDATE data2
	  SET i2 = :r + 1, i4 = i4 + 1000
	  WHERE i2 = :r and i4 > :r;

	break;

  case 3:
	if (traceflag)
	{
		SIprintf("\t\t select all the rows with readlock=nolock, tbl_key=%d \n",
		    r);
		SIflush(stdout);
	}

	EXEC SQL COMMIT;
	EXEC SQL SET LOCKMODE ON data2 WHERE READLOCK=NOLOCK;

	EXEC SQL DECLARE c_tst4_3 CURSOR FOR
	    SELECT i2 FROM data2; 

	EXEC SQL OPEN c_tst4_3 FOR READONLY;
	EXEC SQL INQUIRE_INGRES (:qry_end = endquery, :ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK)
	    break;

	do
	{
	  EXEC SQL FETCH c_tst4_3 
	    INTO :i2_var;
	  EXEC SQL INQUIRE_INGRES (:qry_end = endquery, :ing_err = DBMSERROR);
	  if (ing_err == ERR_DEADLOCK)
	      break;
	  if (ing_err != 0)
	  {
		EXEC SQL CLOSE c_tst4_3;
		EXEC SQL COMMIT;
		EXEC SQL SET LOCKMODE ON data2 WHERE READLOCK=SHARED;
		break;
	  }
	} while (qry_end == 0);

	if (ing_err == ERR_DEADLOCK)
	    break;
	EXEC SQL CLOSE c_tst4_3;
	EXEC SQL COMMIT;
	EXEC SQL SET LOCKMODE ON data2 WHERE READLOCK=SHARED;

	break;

  case 4:
	if (traceflag)
	{
		SIprintf("\t\t insert, update and delete, tbl_key=%d \n", r);
		SIflush(stdout);
	}

	m = m + 6;
	EXEC SQL INSERT INTO data2 (i1, i2, i4, f4, f8, date, char61a, char100)
	    VALUES (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999',
		'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
		break;
	}

	m = m + 7;
	EXEC SQL UPDATE data2 
	    SET i2 = 1, i4 = 1000
	    WHERE i2 = :r;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
		break;
	}

	if (traceflag)
	{
		SIprintf ("\t\t case 4 delete WHERE data2.i2 = %d \n", r);
		SIflush (stdout);
	}

	m = m + 8;
	EXEC SQL DELETE FROM data2 
	    WHERE i2 = :r;

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == 0 && sqlca.sqlerrd[2] != 0)
	{
	    SIprintf ("\t\t TST4 case 4 delete WHERE data2.i2 = %d deleted %d rows\n",
	    r, sqlca.sqlerrd[2]);
	    SIprintf ("\t\t A phantom has been inserted between the UPDATE&DELETE\n");
	    SIprintf ("\t\t This is OK only if isolation level is not SERIALIZABLE\n");
	    SIprintf ("\t\t The coordinator table will have %d more rows\n",
	    sqlca.sqlerrd[2]);
	    SIflush (stdout);
	}
	break;

  case 5:
  default:
	if (traceflag)
	{
		SIprintf("\t\t inserting %d rows, tbl_key=%d \n", c*2, r);
		SIflush(stdout);
	}

	for (i=1; i<c*2; i++)
	{
	  EXEC SQL INSERT INTO data2 (i1, i2, i4, f4, f8, date)
	    VALUES (:c, :i, :r*2, :c*5, :c*10, '11-may-1958');
 	  if (traceflag)
	  {
		SIprintf ("\t\t case 5 inserting i=%d and r*2=%d \n", i, r*2);
		SIflush (stdout);
	  }
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	  if (ing_err != 0)
	  {
		break;
	  }
	}

	break;

  }	/* end of select	*/

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
	PCsleep( (u_i4)30000 );
	return;
  }

  return;
}

	
/*
** Function TST5
** Purpose:	This routine inserts 128 rows in a MQT to the table data3
**		the row added will be 200 + (slave_no * 128)
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst5(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  EXEC SQL BEGIN DECLARE SECTION;
	int	i;
	int	base;
	int	first;
	int	last;
	int	v_i2;
	int	v_f4;
	int	v_f8;
	int	v_dollar;
  EXEC SQL END DECLARE SECTION;

  base		= 200;
  first		= base + (s + c - 2) * 128 + 1;
  last		= base + (s + c - 1) * 128 + 1;

/*	Set lockmode to 35 because we are going to insert 128 rows which 
**	takes 32 pages.  In this way we can avoid a deadlock when escalating
**	to table level lock.  If we don't change the value of maxlocks, they
**	stay in a deadlock loop when trying to escalate to table level when
**	10 pages have been locked (10 is the default for maxlocks).
*/

  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 5, pass %d, MAXLOCKS = 35 \n",
        s, c);
    SIflush (stdout);
  }

  for (i=first; i>last; i++)
  {
    if (traceflag)
    {
      SIprintf ("\t\t inserting tuple where tbl_key = %d:\n", i); 
      SIflush (stdout);
    }
    v_i2 = i;
    v_f4 = s + i;
    v_f8 = c * i;
    v_dollar = i;

    EXEC SQL REPEATED INSERT INTO data3 ( i2, f4, f8, dollar, day, c6, txt58, 
				txt98one, txt98two, txt98three, txt98four)
		VALUES (:v_i2, :v_f4, :v_f8, :v_dollar, '1/1/1987', 'abcedf',
			'tst5_txt58_insert_value_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx');

    EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
    if (ing_err != 0)
    {
      return;
    }

  }	/* end of FOR loop	*/
  
}

	
/*
** Function TST6
** Purpose:	This routine updates rows in data3 where the key is
**		50 < i2 < 80.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst6(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 6, pass %d, update on data3 \n",
        s, c);
    SIflush (stdout);
  }

/*	Increase maxlocks so it doesn't try to escalate to table level lock */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 25;

  EXEC SQL REPEATED UPDATE data3 
	SET	f4 = f4 * 1.02, 
		txt58 = 'tst6_txt58_update_value', 
		dollar = dollar + 1.0
	WHERE i2 > 50 and i2 < 80;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }
}

	
/*
** Function TST7
** Purpose:	This routine updates rows that match the following requirement
**		200 < i2 < 225 in the table data3.  This function should be
**		used together with tst6.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst7(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;

{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 7, pass %d, update on data3 where 200<i2<225 \n",
        s, c);
    SIflush (stdout);
  }

/*	Increase maxlocks so it doesn't try to escalate to table level lock */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 25;

/*	  EXEC SQL REPEATED UPDATE data3	JNF test	*/
  EXEC SQL UPDATE data3 
	SET	f4 = f4 - 100, 
		txt58 = 'tst7_txt58_update_value' 
	WHERE i2 > 200 and i2 < 225;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST8
** Purpose:	This routine delete rows in data3 using a exact match.  It 
**		chooses the key for the exact match by multiplying the slave
**		number by 2 and the repetition pass number by 3 then adding
**		the results together.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst8(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 8, pass %d, delete on data3 \n",
        s, c);
    SIflush (stdout);
  }

/*	Increase maxlocks so it doesn't try to escalate to table level lock */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 20;

  EXEC SQL REPEATED DELETE FROM data3
	WHERE i2 = (:s*2) + (:c*3);

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }
}

	
/*
** Function TST9
** Purpose:	This routine inserts 1 row in each pass to the table data3. 
**		Each slave will insert the same key in each pass such that
**		we get a lot of concurrency and some duplicate keys.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst9(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  EXEC SQL BEGIN DECLARE SECTION;
	int	tbl_key;
	int	base;
	int	v_i2;
	int	v_f4;
	int	v_f8;
	int	v_dollar;
  EXEC SQL END DECLARE SECTION;

/*	Set lockmode to 35 because we are going to insert 128 rows which 
**	takes 32 pages.  In this way we can avoid a deadlock when escalating
**	to table level lock.  If we don't change the value of maxlocks, they
**	stay in a deadlock loop when trying to escalate to table level when
**	10 pages have been locked (10 is the default for maxlocks).
*/

/*	  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;	JNF test */

  base	= 200;
  tbl_key	= base + (c * 2);
  v_i2	= tbl_key;
  v_f4	= s * 2;
  v_f8	= tbl_key * 20;
  v_dollar = c;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 9, pass %d, insert 128 rows \n",
        s, c);
    SIflush (stdout);
  }

  if (traceflag)
  {
      SIprintf ("\t\t inserting tuple where tbl_key = %d:\n", tbl_key); 
      SIflush (stdout);
  }

  EXEC SQL REPEATED INSERT INTO data3 ( i2, f4, f8, dollar, day, c6, txt58, 
				txt98one, txt98two, txt98three, txt98four)
		VALUES (:v_i2, :v_f4, :v_f8, :v_dollar, 
			'1/1/1987',
			'abcedf',
			'tst9_txt58_insert_value_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx');

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }
}

	
/*
** Function TST10
** Purpose:	This routine selects 128 rows into a temporary table.  Since
**		there are 4 tuples per page, 128 rows will have 32 pages
**		which is enough to fill up the AM buffer.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst10(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 10, pass %d, create with select (128 rows)  \n",
        s, c);
    SIflush (stdout);
  }

/*EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = TABLE;	JNF test	*/

  EXEC SQL DROP data3_tmp;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0 && ing_err != ERR_DROP)
  {
/*    EXEC SQL COMMIT;	JNF test	*/
/*    EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = PAGE;	JNF test	*/
    return;
  }

  EXEC SQL CREATE TABLE data3_tmp AS SELECT * FROM data3 WHERE i2 < 128;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
/*    EXEC SQL COMMIT;	JNF test	*/
/*    EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = PAGE;	JNF test	*/
    return;
  }

/*	  EXEC SQL COMMIT;	*/
/*	  EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = PAGE;	*/
/*	  EXEC SQL COMMIT;	*/

}

	
/*
** Function TST11
** Purpose:	This routine is the same as tst10 but with readlock = nolock.  
**	        This routine selects 128 rows into a temporary table.  Since
**		there are 4 tuples per page, 128 rows will have 32 pages
**		which is enough to fill up the AM buffer.  Do not execute 
**		this test in 5.0 with btree because there is a bug that 
**		causes a syserr when using nolock on btrees.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst11(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 11, pass %d, tst10 with readlock=nolock \n",
        s, c);
    SIflush (stdout);
  }

  EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = NOLOCK;

  EXEC SQL DROP data3_tmp;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0 && ing_err != ERR_DROP)
  {
    return;
  }

  EXEC SQL CREATE TABLE data3_tmp AS SELECT * FROM data3 WHERE i2 < 128;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

  EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = SHARED;
}

	
/*
** Function TST12
** Purpose:	Modify data3 to other structures, should get an Xclusive table 
**		lock.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst12(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 12, pass %d, modify data3 structures \n",
        s, c);
    SIflush (stdout);
  }

/*  EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = PAGE;	JNF test	*/

  switch (c)
  {
  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying datat3 to btree \n" );
	  SIflush (stdout);
	}
	EXEC SQL MODIFY data3 TO BTREE ON i2 WITH FILLFACTOR=60, INDEXFILL=60;
	break;
  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying datat3 to isam \n" );
	  SIflush (stdout);
	}
	EXEC SQL MODIFY data3 TO ISAM ON i2 WITH FILLFACTOR=95;
	break;
  case '3':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying datat3 to chash \n" );
	  SIflush (stdout);
	}
	EXEC SQL MODIFY data3 TO CHASH ON i2, f4, f8 WITH FILLFACTOR=90;
	break;
  case '4':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying datat3 to cheap\n" );
	  SIflush (stdout);
	}
	EXEC SQL MODIFY data3 TO CHEAP;
	break;
  default:
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying datat3 to btree \n" );
	  SIflush (stdout);
	}
	EXEC SQL MODIFY data3 TO BTREE ON i2 WITH FILLFACTOR=80, INDEXFILL=85;
	break;
  }

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST13
** Purpose:	Create secondary indices for data3.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst13(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 13, pass %d, create secondary indices for data3 \n",
        s, c);
    SIflush (stdout);
  }

  switch (c)
  {
  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tcreating index with f8, dollar, day, c6 and txt98four \n" );
	  SIflush (stdout);
	}
	EXEC SQL CREATE INDEX data3ind ON data3(f8, dollar, day, c6, txt98four);
	break;
  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tcreating index with txt58, txt98one, txt98two and txt98three  \n" );
	  SIflush (stdout);
	}
	EXEC SQL CREATE INDEX data3ind ON 
		data3(txt58, txt98one, txt98two, txt98three);
	break;
  default:
	if (traceflag)
	{
	  SIprintf ( "\t\tcreating index with i2, f4 and f8 \n" );
	  SIflush (stdout);
	}
	EXEC SQL CREATE INDEX data3ind ON data3(i2, f4, f8);
	break;
  }

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}
/*
**
**	The purpose of tests 14 thru 18 is to exercise the system catalogs in
**	a concurrent environment by doing creates, drops, modifies, and by
**	defining permits.
**
*/

	
/*
** Function TST14
** Purpose:	This test creates a table called data4_x where x is the slave 
**		number
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst14(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[13];
	char	string[20];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  STcopy ("data4_", table_name);
  CVla (s, string);
  STcat (table_name, string);

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 14, pass %d, creating table %s\n",
	s, c, table_name);
    SIflush (stdout);
  }

  STcopy ("CREATE TABLE ", prepare_buffer);
  STcat (prepare_buffer, table_name);
  STcat (prepare_buffer, " ( id i2, c20 c20, ii i1, i2 i2, i4 i4, ");
  STcat (prepare_buffer, "f4 f4, f8 f8, money money, date date,  ");
  STcat (prepare_buffer, "c1 c1, char1 text(1), c50 c50, ");
  STcat (prepare_buffer, "char50 text(50) )" );

  EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err == ERR_CREATE || ing_err == ERR_DUPLICATE)
  {
    ing_err = 0; 
  }
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST15
** Purpose:	This test modifies table data4_x to other structures (btree,
**		isam, chash, cbtree, cheap).  The table is dependent on the 
**		slave that is executing the function (data4_x where x = s) and 
**		the structure of the table is dependent on the pass (c).
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst15(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[13];
	char	string[20];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  STcopy ("data4_", table_name);
  CVla (s, string);
  STcat (table_name, string);

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 15, pass %d, modifying table %s\n",
	s, c, table_name);
    SIflush (stdout);
  }

  switch (c)
  {
  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying %s to btree \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO BTREE UNIQUE WITH FILLFACTOR=95,");  
	STcat (prepare_buffer, " INDEXFILL=50, maxindexfill=90");

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying %s to isam \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO ISAM WITH FILLFACTOR=25 ");  

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '3':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying %s to chash \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO CHASH ON f4, f8 WITH FILLFACTOR=90,  ");  
	STcat (prepare_buffer, " MAXPAGES=100, MINPAGES=30");

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '4':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying %s to cbtree \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO CBTREE ");  

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '5':
	if (traceflag)
	{
	  SIprintf ( "\t\tmodifying %s to cheap \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO CHEAP ");  

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  default:
	if (traceflag)
	{
	  SIprintf ( "\t\tdefault modifying datat3 to btree \n", table_name);
	  SIflush (stdout);
	}
	STcopy ("MODIFY ", prepare_buffer);
	STcat (prepare_buffer, table_name);
	STcat (prepare_buffer, " TO BTREE WITH FILLFACTOR=30,");  
	STcat (prepare_buffer, " INDEXFILL=50, maxindexfill=80");

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  }

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST16
** Purpose:	This routine creates indices for the table data4_x where x is 
**		the slave_number.  It should be executed after routine tst14
**		is executed because tst14 creates the tables.  
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst16(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[13];
	char	string[20];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  STcopy ("data4_", table_name);
  CVla (s, string);
  STcat (table_name, string);

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 16, pass %d, creating indices for %s\n",
	s, c, table_name);
    SIflush (stdout);
  }

  switch ( c % 3 )
  {
  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tcreating indices for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "CREATE INDEX ind%s ON %s  (i4, date, money, f8) ",
		table_name, table_name);

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tcreating multiple indices for %s \n", table_name);
	  SIflush (stdout);
	}

        STprintf(prepare_buffer, "CREATE INDEX ind%s1 ON %s (i4)",
                table_name, table_name);
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
	  return;
	}

        STprintf(prepare_buffer, "CREATE INDEX ind%s2 ON %s (i2, f4, date) WITH  STRUCTURE = BTREE ",
                table_name, table_name);
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
	  return;
	}

        STprintf(prepare_buffer, "CREATE INDEX ind%s3 ON %s (tin) WITH STRUCTURE = HASH ",
                table_name, table_name);
 	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err != 0)
	{
	  return;
	}

	STprintf(prepare_buffer, "CREATE INDEX ind%s4 ON %s (char50) WITH STRUCTURE = ISAM ", 
		table_name, table_name);
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  default:
	if (traceflag)
	{
	  SIprintf ( "\t\tdefault creating indices for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "CREATE INDEX ind%s ON %s (i4, date, money) ",
		table_name, table_name);

	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  }

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }
}

	
/*
** Function TST17
** Purpose:	This routine defines permits on the table data4_x where x is
**		the slave_number.  It should be executed after tst14 is
**		executed because tst14 creates the table.  Also remember
**		that this routine should be executed by the DBA who is the
**		only one that can define permits on a table.
**
** Parameters:	s = slave number.
**		c = number of repetitions
**
** Returns:
*/

tst17(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[13];
	char	string[20];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  STcopy ("data4_", table_name);
  CVla (s, string);
  STcat (table_name, string);

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 17, pass %d, granting permits %s\n",
	s, c, table_name);
    SIflush (stdout);
  }

  switch (c)
  {
  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tgranting all to all for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "GRANT ALL ON %s TO PUBLIC ", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tgranting update to myrtle for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "GRANT UPDATE ON %s TO myrtle", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '3':
	if (traceflag)
	{
	  SIprintf ( "\t\tgranting insert to jupqatest for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "GRANT INSERT ON %s TO jupqatest", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  case '4':
	if (traceflag)
	{
	  SIprintf ( "\t\tgranting update, insert and delete for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "GRANT UPDATE ON %s TO myrtle", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}
	else if (sqlca.sqlcode < 0)
	{
	  break;
	}

	STprintf(prepare_buffer, "GRANT DELETE ON %s TO gladys", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}
	else if (sqlca.sqlcode < 0)
	{
	  break;
	}

	STprintf(prepare_buffer, "GRANT INSERT ON %s TO qatests", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	STprintf(prepare_buffer, "CREATE INTEGRITY ON %s IS i2 < 100", 
		table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;
	break;
  default:
	if (traceflag)
	{
	  SIprintf ( "\t\tdefault granting qatest insert and delete for %s \n", table_name);
	  SIflush (stdout);
	}
	STprintf(prepare_buffer, "GRANT DELETE, INSERT ON %s TO qatest\n", table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;

	STprintf(prepare_buffer, "CREATE INTEGRITY ON %s IS i2 > 100", 
		table_name);  
	EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;

	break;
  }
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST18
** Purpose:	This routine drops a table according to the slave number.  It 
**		should be executed after tst14 is executed because tst14 
**		creates the temporary tables.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst18(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[13];
	char	string[20];
	char	prepare_buffer[1000];
EXEC SQL END DECLARE SECTION;

  STcopy ("data4_", table_name);
  CVla (s, string);
  STcat (table_name, string);

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 18, pass %d, dropping table %s\n",
	s, c, table_name);
    SIflush (stdout);
  }

  STprintf(prepare_buffer, "DROP %s ", table_name);  

  EXEC SQL EXECUTE IMMEDIATE :prepare_buffer;

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0 && ing_err != ERR_DROP)
  {
    return;
  }

}

	
/*
** Function TST19
** Purpose:	This routine performs different transactions on data2 and then
**		aborts them.  It uses a random number generator to decide which
**		transcation to execute.  The seed for the random number should 
**		be placed in the table seeds and should be done before the 
**		CTS test that executes this routine is executed.  
**		Note that only one transaction will be excuted (aborted) in each
**		pass.  Most transactions use repeat queries and perform 
**		operations that require exclusive locks.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst19(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	num;
	int	r;
	int	m;
	int	i;
	int	seed;
	char	date[26];
	char	xxx61[62];
	char	yyy100[101];
EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT seed_val INTO :seed FROM seeds;
  MHsrand ( (f8)((seed + s) * c) );
  
  r   = (i4) ((MHrand() * ( MHrand() * (f8) (32767))) +1);
  m   = r % 6;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 19, pass %d, Testing Rollback with seed = %d\n",
	s, c, seed );
    SIflush (stdout);
  }

  STcopy ("31-dec-1999", date);
  STcopy ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", xxx61);
  STcopy ("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy", yyy100);

  switch (m)
  {
  case '0':
	if (traceflag)
	{
	  SIprintf ( "\t\tinserting tbl_key = %d ", r );
	  SIflush (stdout);
	}
	EXEC SQL REPEATED INSERT INTO data2 (i1, i2, i4, f4, f8, date, char61a, char100)
		values ( :c, :r, :r*2, :c*5, :c*10, :date, :xxx61, :yyy100);
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL ROLLBACK;
	break;

  case '1':
	if (traceflag)
	{
	  SIprintf ( "\t\tinserting and deleting tbl_key = %d ", r );
	  SIflush (stdout);
	}
	EXEC SQL REPEATED INSERT INTO data2 (i1, i2, i4, f4, f8, date, char61a, char100)
		values ( :c, :r, :r*2, :c*5, :c*10, :date, :xxx61, :yyy100);
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL REPEATED DELETE FROM data2 WHERE i2 = :r;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL ROLLBACK;
	break;

  case '2':
	if (traceflag)
	{
	  SIprintf ( "\t\tupdating tbl_key = %d ", r );
	  SIflush (stdout);
	}
	EXEC SQL REPEATED UPDATE data2 
		SET i2 = :r + 1, i4 = i4+1000
		WHERE i2 = :r AND i4 > :r;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL ROLLBACK;
	break;

  case '3':
	if (traceflag)
	{
	  SIprintf ( "\t\tselecting all the rows without readlock=nolock \n" );
	  SIflush (stdout);
	}
	EXEC SQL SET LOCKMODE ON data2 WHERE READLOCK = NOLOCK;
	EXEC SQL REPEATED SELECT i2 INTO :r FROM data2;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL COMMIT;
	EXEC SQL SET LOCKMODE ON data2 WHERE READLOCK = SHARED;
	break;

  case '4':
	if (traceflag)
	{
	  SIprintf ( "\t\tupdating, deleting, and inserting tbl_key = %d ", r );
	  SIflush (stdout);
	}
	EXEC SQL REPEATED UPDATE data2 
		SET i2 = 1, i4 = 1000
		WHERE i2 = :r;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL SAVEPOINT saveone;

	EXEC SQL REPEATED DELETE FROM data2 WHERE i2 = :r;
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL ROLLBACK TO saveone;

	EXEC SQL REPEATED INSERT INTO data2 (i1, i2, i4, f4, f8, date, char61a, char100)
		values ( :c, :r, :r*2, :c*5, :c*10, :date, :xxx61, :yyy100);
	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	{
	  return;
	}

	EXEC SQL ROLLBACK;

	break;

  case '5':
	if (traceflag)
	{
	  SIprintf ( "\t\tinserting %d rows in a MST statement \n", c*2 );
	  SIflush (stdout);
	}
	for (i=1; i<c*2; i++)
	{
	  EXEC SQL INSERT INTO data2 (i1, i2, i4, f4, f8, date, char61a, char100)
		values ( :c, :r, :r*2, :c*5, :c*10, :date, :xxx61, :yyy100);
	  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
 	  if (ing_err == ERR_DEADLOCK || ing_err == ERR_LOGFULL)
	  {
	    return;
	  }
	}

	EXEC SQL ROLLBACK;

	break;

  }
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err != 0)
  {
    return;
  }

}

	
/*
** Function TST20
** Purpose:	This routine will select data from 5 different tables.  The 
**		data will be a result simple table selects, table joins, and
**		the order by function.  This routine will also declare 5
**		cursors to handle the select transactions.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst20(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d5tbl_key;		/* delcarations FOR data5 table */
        int     d5cnt;

        int     d6tbl_key;		/* delcarations FOR data5 table */
        int     d6cnt;
	float	d6float;

        int     d7tbl_key;		/* delcarations FOR data5 table */
        float	d7money;

        int     d8tbl_key;		/* delcarations FOR data5 table */
        char	d8char[66];

        int     d9tbl_key;		/* delcarations FOR data5 table */
	char	d9date[26];

        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 20, pass %d Select with cursors \n",
        s, c);
    SIflush (stdout);
  }

  /*
  ** Increase maxlocks to avoid deadlock when escalating to table level lock.
  */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;

/* Plain and boring old select, get all rows from a single table */
  EXEC SQL DECLARE c1 CURSOR FOR 
	SELECT tbl_key, cnt FROM data5;

/* Qualify the select, and include join */
  EXEC SQL DECLARE c2 CURSOR FOR 
	SELECT a.tbl_key, a.cnt, b.floatv, f.money, d.charv, e.date
	  FROM data5 a, data6 b, data7 f, data8 d, data9 e
	  WHERE a.cnt < 50 AND a.tbl_key = b.tbl_key
	    AND a.tbl_key = f.tbl_key AND a.tbl_key = d.tbl_key 
	    AND a.tbl_key = e.tbl_key;
	
/* Getting more complicated, qualify the select, and also throw in a	*/
/*	join and a order by.						*/
  EXEC SQL DECLARE c3 CURSOR FOR 
	SELECT a.tbl_key, a.cnt, b.floatv, f.money, d.charv, e.date
	  FROM data5 a, data6 b, data7 f, data8 d, data9 e
	  WHERE a.cnt > 20 AND a.tbl_key = b.tbl_key
	    AND a.tbl_key = f.tbl_key AND a.tbl_key = d.tbl_key 
	    AND a.tbl_key = e.tbl_key
	  ORDER BY a.tbl_key, a.cnt;

/* Same as cursor c3, qualify the select, and also throw in a join and	*/
/*	a order by.  The order by also uses columns from 2 different	*/
/*	tables.								*/
  EXEC SQL DECLARE c4 CURSOR FOR 
	SELECT a.tbl_key, a.cnt, b.cnt, b.floatv, f.money, d.charv, e.date
	  FROM data5 a, data6 b, data7 f, data8 d, data9 e
	  WHERE a.cnt > 10 AND b.cnt < 200 
	    AND a.tbl_key = b.tbl_key AND a.tbl_key = f.tbl_key 
	    AND a.tbl_key = d.tbl_key AND a.tbl_key = e.tbl_key
	  ORDER BY a.tbl_key, b.cnt, f.money;

/* Another plain old select, but getting columns from different tables.	*/
  EXEC SQL DECLARE c5 CURSOR FOR 
	SELECT a.tbl_key, a.cnt, b.floatv, f.money, d.charv, e.date
	  FROM data5 a, data6 b, data7 f, data8 d, data9 e
	  WHERE a.tbl_key = b.tbl_key AND a.tbl_key = f.tbl_key 
	    AND a.tbl_key = d.tbl_key AND a.tbl_key = e.tbl_key;

  if (traceflag)
  {
    SIprintf ("\t\tSelect from data5 only (all)\n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c1;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
    {
    EXEC SQL FETCH c1 INTO :d5tbl_key, :d5cnt;
    EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err && traceflag)
    {
      SIprintf ("\t\tc1 got %d, %d \n\0", d5tbl_key, d5cnt);
      SIflush (stdout);
    }
  }

  if (ing_err != 0)
  {
    return;
  }

  EXEC SQL CLOSE c1;

  if (traceflag)
  {
    SIprintf ("\t\tJoin all tables where data5.cnt < 50 \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c2;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c2 INTO :d5tbl_key, :d5cnt, :d6float, :d7money, :d8char, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err && traceflag)
    {
      SIprintf ("\t\tc2 got %d, %d \n\0", d5tbl_key, d5cnt );
      SIflush (stdout);
    }
  } 

  if (ing_err != 0)
  {
    return;
  }

  EXEC SQL CLOSE c2;

  if (traceflag)
  {
    SIprintf ("\t\tJoin all tables where data5.cnt > 20 \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c3;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c3 INTO :d5tbl_key, :d5cnt, :d6float, :d7money, :d8char, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err && traceflag)
    {
      SIprintf ("\t\tc3 got %d, %d \n\0", d5tbl_key, d5cnt );
      SIflush (stdout);
    }
  }  

  if (ing_err != 0)
  {
    return;
  }
  EXEC SQL CLOSE c3;

  if (traceflag)
  {
    SIprintf ("\t\tJoin all tables where data5.cnt >10 and data6.cnt < 200 \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c4;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c4 INTO :d5tbl_key, :d5cnt, :d6cnt, 
	:d6float, :d7money, :d8char, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err && traceflag)
    {
      SIprintf ("\t\tc4 got %d, %d \n\0", d5tbl_key, d5cnt );
      SIflush (stdout);
    }
  }

  if (ing_err != 0)
  {
    return;
  }
  EXEC SQL CLOSE c4;

  if (traceflag)
  {
    SIprintf ("\t\tJoin all tables \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c5;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c5 INTO :d5tbl_key, :d5cnt, :d6float, :d7money, :d8char, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err && traceflag)
    {
      SIprintf ("\t\tc5 got %d, %d \n\0", d5tbl_key, d5cnt );
      SIflush (stdout);
    }
  }

  if (ing_err != 0)
  {
    return;
  }
  EXEC SQL CLOSE c5;

}


	
/*
** Function TST21
** Purpose:	This routine uses a cursor to select rows from data5 where 
**		30 < tbl_key < 300.  These rows are then updated in deferred mode.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst21(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d5tbl_key;		/* delcarations for data5 table */
        int     d5cnt;

        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 21, pass %d, deferred update using cursors where 30<tbl_key<300 \n",
        s, c);
    SIflush (stdout);
  }

  /*
  ** Increase maxlocks to avoid deadlock when escalating to table level lock.
  */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;

  EXEC SQL DECLARE c6 CURSOR FOR 
	SELECT tbl_key, cnt FROM data5
	  WHERE tbl_key > 30 AND tbl_key < 300
	  FOR UPDATE OF tbl_key, cnt;

  done = 0;
  EXEC SQL OPEN c6;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c6 INTO :d5tbl_key, :d5cnt;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {
      EXEC SQL UPDATE data5
	SET tbl_key = tbl_key + 20
	WHERE CURRENT OF c6;

      /*
      ** After UPDATE, INQUIRE_INGRES again.
      ** We may deadlock trying to get the lock on the new page.
      */
      EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );

      if (traceflag && !done && !ing_err)
      {
	SIprintf ("\t\tc6 Direct --> tbl_key = %d, cnt = %d \n", 
		d5tbl_key, d5cnt );
	SIflush (stdout);
      }
    }
  } 

  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR );
  if (ing_err != 0)
  {
    return;
  }
  EXEC SQL CLOSE c6;
}

	
/*
** Function TST22
** Purpose:	This routine uses a cursor to select rows from data6.  These 
**		rows are then updated in deferred mode.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst22(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d6tbl_key;		/* delcarations for data6 table */
        int     d6cnt;
	float	d6float;

        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 22, pass %d, deferred update using cursors on data6 \n",
        s, c);
    SIflush (stdout);
  }

  EXEC SQL DECLARE c7 CURSOR FOR 
	SELECT tbl_key, cnt, floatv FROM data6
	  FOR UPDATE OF tbl_key, cnt, floatv;

  done = 0;
  EXEC SQL OPEN c7;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);

  while (!done && !ing_err)
  {
    EXEC SQL FETCH c7 INTO :d6tbl_key, :d6cnt, :d6float;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {
      EXEC SQL UPDATE data6
	SET tbl_key = tbl_key - 10, floatv = floatv * 2
	WHERE CURRENT OF c7;
      if (traceflag)
      {
	SIprintf ("\t\tc7 Direct --> tbl_key = %d, cnt = %d, float = %f \n", 
		d6tbl_key, d6cnt, d6float );
	SIflush (stdout);
      }
    }
  } 

  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR );
  if (ing_err != 0)
  {
    return;
  }
  EXEC SQL CLOSE c7;
}
	
/*
** Function TST23
** Purpose:	This routine uses a cursor to select rows from data7.  It will
**		only select row where tbl_key < 200 and money > 10.  These 
**		rows are then updated in deferred mode.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst23(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d7tbl_key;		/* delcarations for data7 table */
	float	d7money;

        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 23, pass %d, deferred update using cursors on data7 \n",
        s, c);
    SIflush (stdout);
  }

  EXEC SQL DECLARE c8 CURSOR FOR 
	SELECT tbl_key, money FROM data7
	  WHERE tbl_key < 200 AND money > 10
	  FOR UPDATE OF tbl_key, money;

  done = 0;
  EXEC SQL OPEN c8;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c8 INTO :d7tbl_key, :d7money;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {
      EXEC SQL UPDATE data7
	SET tbl_key = tbl_key * 2
	WHERE CURRENT OF c8;

      if (traceflag)
      {
	SIprintf ("\t\tc8 Direct --> tbl_key = %d, %f \n", 
		d7tbl_key, d7money );
	SIflush (stdout);
      }
    }
  } 

  if (ing_err != 0)
  {
    return;
  }

  EXEC SQL CLOSE c8;

}

	
/*
** Function TST24
** Purpose:	This routine uses two cursors to select rows from table data5 
**		and data8.  The selects are used nested inside a while loop, 
**		with the inner select updating the data8 table.
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst24(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d5tbl_key;		/* delcarations for data5 table */
	int	d5cnt;

        int     d8tbl_key;		/* delcarations for data8 table */
	char 	d8char[66];

        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 24, pass %d, nested selects with cursors on data5 and data8 \n",
        s, c);
    SIflush (stdout);
  }

  /*
  ** Increase maxlocks to avoid deadlock when escalating to table level lock.
  */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;

  EXEC SQL DECLARE c9 CURSOR FOR 
	SELECT a.tbl_key, a.cnt FROM data5 a, data8 d
	  WHERE a.tbl_key = d.tbl_key;

  EXEC SQL DECLARE c10 CURSOR FOR 
	SELECT tbl_key, charv FROM data8 
	  WHERE tbl_key > 10 AND tbl_key < 100
	  FOR UPDATE OF tbl_key, charv;

  done = 0;
  EXEC SQL OPEN c10;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);

  if ( ing_err != 0 )
  {
    return;
  }

  EXEC SQL OPEN c9;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);

  if ( ing_err != 0 )
  {
    return;
  }

  while (!done && !ing_err)
  {
    EXEC SQL FETCH c9 INTO :d5tbl_key, :d5cnt;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {  
      if ( traceflag )
      {
	SIprintf ("\t\tc9: %d %d \n", d5tbl_key, d5cnt);
	SIflush (stdout);
      }

      EXEC SQL FETCH c10 INTO :d8tbl_key, :d8char;
      EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
      if ( !done && !ing_err )
      {
	if ( traceflag )
	{
	  SIprintf ("\t\tc10: %d %s \n", d8tbl_key, d8char );
	  SIflush (stdout);
	}

	EXEC SQL UPDATE data8
	  SET tbl_key = tbl_key * 2, charv = 'Test 24 update value '
	  WHERE CURRENT OF c10;
	EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
      }
    } 
  }

  if ( ing_err != 0 )
  {
    return;
  }

  EXEC SQL CLOSE c9;
  EXEC SQL CLOSE c10;
}

	
/* 
** Function TST25 
** Purpose:	This routine uses two cursors to select rows from the same 
**		data9 table with options for direct update.  The cursors will
**		be opened and closed one at a time, not concurrently. 
** Parameters: 
**          s = number of slaves. 
**	    c = number of repetitions.
** Returns: 
*/ 

tst25(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     d9tbl_key;		/* delcarations for data9 table */
	char	d9date[26];
        int     done;
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 25, pass %d, selects with 2 cursors on data9 \n",
        s, c);
    SIflush (stdout);
  }

  EXEC SQL DECLARE c11 CURSOR FOR 
	SELECT tbl_key, date FROM data9 
	  WHERE tbl_key < 10 OR tbl_key > 300
	  FOR UPDATE OF tbl_key;

  EXEC SQL DECLARE c12 CURSOR FOR 
	SELECT tbl_key, date FROM data9 
	  WHERE tbl_key > 10 AND tbl_key < 300
	  FOR UPDATE OF tbl_key;

  if (traceflag)
  {
    SIprintf ("\t\t delete data9 where tbl_key<10 or tbl_key>300 using cursors \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c11;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c11 INTO :d9tbl_key, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {
      EXEC SQL DELETE FROM data9
	WHERE CURRENT OF c11;

      if ( traceflag )
      {
	SIprintf ("\t\tc11 delete : %d %s \n", d9tbl_key, d9date );
	SIflush (stdout);
      }
    }
  }

  if (ing_err != 0)
  {
    EXEC SQL CLOSE C11;
    return;
  }

  EXEC SQL CLOSE C11;

  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR );
  if (ing_err != 0)
  {
    return;
  }

  if (traceflag)
  {
    SIprintf ("\t\t update data9 where 10<tbl_key<300 using cursors \n");
    SIflush (stdout);
  }

  done = 0;
  EXEC SQL OPEN c12;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
  while (!done && !ing_err)
  {
    EXEC SQL FETCH c12 INTO :d9tbl_key, :d9date;
    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :done = endquery );
    if ( !done && !ing_err )
    {
      EXEC SQL UPDATE data9
	SET tbl_key = (tbl_key * 3) / 2
	WHERE CURRENT OF c12;

      if ( traceflag )
      {
	SIprintf ("\t\tc12 update : %d %s \n", d9tbl_key, d9date );
	SIflush (stdout);
      }
    }
  } 

  if (ing_err != 0)
  {
    EXEC SQL CLOSE c12;
    return;
  }

  EXEC SQL CLOSE c12;

  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR );
  if (ing_err != 0)
  {
    return;
  }

}

	
/* 
** Function tst26 
** Purpose:	Test the index compressions with concurrency where the key 
**		ranges from very compressable, to keys where the compression 
**		would consist of no compression at all.  There would be four 
**		runs of this test with the first run on a old format 
**		un-compressed btree table, the second will be on a new 
**		compressed btree table with data and index compressions, 
**		third on a btree with index compression only, and the last 
**		depending on time, will be btree with data compression only.
**
**		The function which would be run 60 (?) times by each slave 
**		would follow the following pseudo code:
**
**		for (i=0; i <= repetition number; i++)
**		{
**		    insert into table row where numeric are (slave number ** i + i)
**			and character field are copies of predetermined strings
**			in array[ ((slave number *i) mod array size) ]
**		}
**
**		The array elements would be along the following lines of thought:
**		array[0] = 'a'
**		array[1] - 'abcde'
**		array[2] - 'abcdefghij'
**		array[3] - 'abcdefghijklmn'
**		array[4] - 'abcdefghijklmnopqr'
**		array[5] - 'abcdefghijklmnopqrstuvw'
**		array[6] - 'abcdefg... 90 characters ...stuvwxyz'
**
**		The table would be along the following lines of thought::
**	Column Name                      Type		     Length
**						     external	    internal
**	i2                               integer         2		 5
**	i2_1                             integer         2       	 5
**	i2_2                             integer         2       	 5
**	i2_3                             integer         2       	 5
**	f4                               float           4	 	 5
**	f8                               float           8	 	 9
**	dollar                           money           8            	 9
**	day                              date           12	 	13
**	day_1                            date           12	 	13
**	day_2                            date           12	 	13
**	c90                              c              90	 	91
**	txt1                             char           90	 	91
**	txt2                             vchar          90	 	93
**	txt3                             text           90		93
**       -----------------------------------------------------------------
**	text                             vchar         100 
**
**
** Parameters: 
**          s = slave number. 
**	    c = number of repetitions.
** Returns: 
*/ 

tst26(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	i;
	int	element;
	char	array_buffer[100]; 
static	char	*array[] = { "a", "123456789_", "abcdefghijklmnopqrstuvwxyz",
		"_9876543210__9876543210_9876543210", 
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		"99999999999999999999999999999999999999999999999999999999999999999999999",
    "abcdefgh_1_ijkomno_2_pqrstuv_3_xyzABCD_4_EFGHIJK_5_LMNOPQR_6_STUVWXY_7_Zabcdef_8_ghijkom_9"
			   };

EXEC SQL END DECLARE SECTION;

  EXEC SQL SAVEPOINT tst26save;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 26, pass %d, Insert into table data10 \n",
        s, c);
    SIflush (stdout);
  }

/*	  for (i=1; i <= 5; i++)		*/
/*	  {					*/
    i = s * c * 23;
    element = ((i*s) % 7);
    STcopy (array[element], array_buffer);
    if ( element == 0 ) { element = 7; }
    EXEC SQL REPEATED INSERT INTO data10(i2, i2_1, i2_2, i2_3, f4, f8, dollar, 
		day, day_1, day_2, c90, txt1, txt2, txt3, text) 
	VALUES (:i, :s, :c, :i*:c, :i*:s, :c*:s*:i/:element, :c*:s, 
		'15-Apr-1991', '16-Apr-1991', 
		'17-Apr-1991', :array_buffer, :array_buffer, 
		:array_buffer, :array_buffer, :array_buffer );

    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);

    if (ing_err != 0)
    {
      EXEC SQL ROLLBACK;
      return;
    }

    if (traceflag)
    {
      SIprintf ("\t\t inserting into data10 numeric %d and array element %d \n",
	i, element);
      SIflush (stdout);
    }
/*	  }					*/


}

	
/* 
** Function tst27 
** Purpose:	Test the index compressions with concurrency where the key 
**		ranges from very compressable, to keys where the compression 
**		would consist of no compression at all.  There would be four 
**		runs of this test with the first run on a old format 
**		un-compressed btree table, the second will be on a new 
**		compressed btree table with data and index compressions, 
**		third on a btree with index compression only, and the last 
**		depending on time, will be btree with data compression only.
**
**		The function which would be run 60 (?) times by each slave 
**		would follow the following pseudo code:
**
**		for (i=0; i <= repetition number; i++)
**		{
**		    insert into table row where numeric are (slave number ** i + i)
**			and character field are copies of predetermined strings
**			in array[ ((slave number *i) mod array size) ]
**		}
**
**		The table would be along the following lines of thought::
**	Column Name                      Type		     Length
**						     external	    internal
**	lt_name				vchar		50		53
**	ft_name				char		50		51
**	mid_init			c1		 1		 1
**	emp_num				integer		 2		 5
**	emp_titl			text		50		53
**	st_date 			date		12		13
**       -----------------------------------------------------------------
**	text                             vchar         100	       103
**
**
** Parameters: 
**          s = slave number. 
**	    c = number of repetitions.
** Returns: 
*/ 

tst27(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	i;
	int	j;
	int	k;
	int	element;
	char	First_Name[51]; 
	char	Last_Name[51]; 
	char	Middle_Init[2]; 
	int	Emp_Num; 
	char	Emp_Title[51]; 
static	char	*f_name[] = { " ", "a", "123456789_", 
		"abcdefghijklmnopqrstuvwxyz",
		"_9876543210__9876543210_9876543210", 
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 
		"Steven", "John", "Donald", "Krzysztof", "Hamilton", 
		"Jerry", "Saundra", "Steve", "Kelly", "Richard", "Roy",
		"Jennifer", "Claire"
			   };
static	char	*l_name[] = { " ", "b", "b23456789_", 
		"bcdefghijklmnopqrstuvwxyza",
		"b9876543210__9876543210_9876543210", 
		"cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		"Pennebaker", "Simutis", "Johnson", "Stec", "Whitener",
		"Dufour", "Flaggs", "Doan", "Park", "Lamberty", "Tessler",
		"Johnson", "Leachman", "Brussels"
			   };
static	char	*titles[] = { " ", "c", "123456789_", 
		"cdefghijklmnopqrstuvwxyzab",
		"c9876543210__9876543210_9876543210", 
		"cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		"student", "Member of Technical Staff", "Manager", 
		"Director", "Vice President", "Exective Vice President",
		"CEO"
			   };

static	char    *middle[]= { "A", "B", "C", "D", "E", "F", "G", "H", "I",
                            "J", "K", "L", "M", "N", "O", "P", "Q", "R",
                            "S", "T", "U", "V", "W", "X", "Y", "Z", " "
                           };
 
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 27, pass %d, Insert into table data21 \n",
        s, c);
    SIflush (stdout);
  }

  EXEC SQL SAVEPOINT tst27save;

  for (i=1, j=c, k=s; i <= 5; i++, j++, k++)
  {
    element = ((i*s) % 19);
    STcopy (f_name[element], First_Name);
    if ( element == 0 ) { element = 19; }
    element = ((j*s) % 20);
    STcopy (l_name[element], Last_Name);
    if ( element == 0 ) { element = 20; }
    element = (( i*j*k ) % 27);
    STcopy (middle[element], Middle_Init);
    if ( element == 0 ) { element = 27; }
    Emp_Num = (i*j*k % 32761);
    element = ((k*s) % 13);
    STcopy (titles[element], Emp_Title);
    if ( element == 0 ) { element = 13; }

    if (traceflag)
    {
      SIprintf ("\t about to inserting into DATA11 numeric %d and element %d \n",
	i, element);
      SIflush (stdout);
    }

    EXEC SQL REPEATED INSERT INTO DATA11 (lt_name, ft_name, mid_init,
		emp_num, emp_titl, st_date, text) 
	VALUES (:Last_Name, :First_Name, :Middle_Init, :Emp_Num, :Emp_Title,
		'15-Apr-1991', :Last_Name);

    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
    if (ing_err != 0)
    {
      EXEC SQL ROLLBACK;
      return;
    }
    if (traceflag)
    {
      SIprintf ("\t done inserting into DATA11 numeric %d and array element %d \n",
	i, element);
      SIprintf ("\t\t row = %s, %s %s - %d - %s \n",
	Last_Name, First_Name, Middle_Init, Emp_Num, Emp_Title);
      SIflush (stdout);
    }
  }


}

	
/*
** Function tst28
** Purpose:	Test the new btree compressed index's ability to deal with 
**		numerous overflow pages.  The test would create row with 
**		similiar keys.  Again, there would be four runs of this test 
**		like the test above.  The number of tid's in an overflow page 
**		is 250+, so the number of times each slave need to run to 
**		create an overflow needs to be larger than 250.
**
**		The function which would be run 600 (?) times by each slave 
**		would follow the following pseudo code:
**
**	for (i=0; i <= repetition number; i++)
**	{
**		insert into table row where numeric are (i)
**			and character field are copies of predetermined strings
**			in array[ i mod array size ]
**	}
**
**		The array elements and the table used would be similiar to 
**		those defined in test number 1.  At the end of the test with 
**		60 repetition, keys with numeric value equal to 1 would have 
**		600 duplicate rows, the key with numeric value equal to 2 
**		would have 599 duplicate rows, etc.
**
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst28(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	i;
	int	element;
	char	Last_Name[51]; 
	char	First_Name[51]; 
	char	Middle_Init[2]; 
	int	Emp_Num; 
	char	Emp_Title[51]; 
	char	array_buffer[100]; 
static	char	key_buffer[30] = "tst28 string field data"; 
EXEC SQL END DECLARE SECTION;

static	char	*l_name[] = { "Pennebaker", "Simutis", "Johnson", "Stec", 
		"Whitener", "Dufour", "Flaggs", "Doan", "Park", "Lamberty", 
		"Tessler", "Johnson", "Leachman"
			   };

static	char	*f_name[] = { "Steven", "John", "Donald", "Krzysztof", 
		"Hamilton", "Jerry", "Saundra", "Steve", "Kelly", "Richard", 
		"Roy", "Jennifer", "Claire"
			   };

static	char	*titles[] = { "Supervisor", "Member of Technical Staff", "Manager", 
		"Director", "Vice President", "Exective Vice President",
		"CEO"
			   };

static	char    *middle[]= { "A", "B", "C", "D", "E", "F", "G", "H", "I",
                            "J", "K", "L", "M"
                           };

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 28, pass %d, Insert into table data10 \n",
        s, c);
    SIflush (stdout);
  }

  STprintf(array_buffer, "tst28 non_key data for each row, slave %d, reps %d, loop_counter %d",
                s, c, i);

  /*
  ** Increase maxlocks to avoid deadlock when escalating to table level lock.
  */
  EXEC SQL SET LOCKMODE SESSION WHERE MAXLOCKS = 35;

  EXEC SQL SAVEPOINT tst28save;

  for (i=1; i <= c+1; i++)
  {
    element = (s % 13);
    STcopy (l_name[element], Last_Name);
    STcopy (f_name[element], First_Name);
    STcopy (middle[element], Middle_Init);
    if ( element == 0 ) { element = 13; }
    Emp_Num = (s % 13);
    if (element > 7) element = element % 7;
    STcopy (titles[element], Emp_Title);

    EXEC SQL REPEATED INSERT INTO DATA11 (lt_name, ft_name, mid_init,
		emp_num, emp_titl, st_date, text) 
	VALUES (:Last_Name, :First_Name, :Middle_Init, :Emp_Num, :Emp_Title,
		'15-Apr-1991', :key_buffer); 

    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
    if (ing_err != 0)
    {
      EXEC SQL ROLLBACK;
      return;
    }
    if (traceflag)
    {
      SIprintf ("\t\t inserting into data11 similiar row with key value %d \n",
	i );
      SIflush (stdout);
    }
  }


}
  

	
/*
** Function tst29
** Purpose:	Test the new btree compressed index's ability to deal with 
**	page splits.  The test will create rows with key values that will 
**	increment with values that leaves gaps for future insertations.  An 
**	example would be slave number 1 will insert rows 1, 6, 11, and 16; 
**	slave number 2 would insert rows 2, 7, 12, and 17; slave number 3 
**	would insert rows 3, 8, 13, and 18.  There would be 5 slave and in the 
**	end, the random insertions should fill in all the rows, and the thought 
**	of limiting ? number of keys on a page would cause the pages to split.  
**	Another factor that can be used is the fill factor values. 
**
**	The function which would be run 60 times by each slave would follow 
**	the following pseudo code:
**
**	for (i=0; i <= repetition number; i++)
**	{
**		insert into table row where numeric are (i) except for the 
**			first key value which would be (slave number * i * 5)
**			and character field is a copy of a predetermined string
**	}
**	
**	The table used would be very similiar to the table defined in test 
**	number 25.
**
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst29(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int	i;
	int	j;
	int	k;
	int	insert_value;
	int	element;
	char	First_Name[51]; 
	char	Last_Name[51]; 
	char	Middle_Init[2]; 
	int	Emp_Num; 
	char	Emp_Title[51]; 
	char	array_buffer[100]; 
static	char	key_buffer[30] = "tst29 string field data"; 
EXEC SQL END DECLARE SECTION;

static	char	*f_name[] = { " ", "a", "123456789_", 
		"abcdefghijklmnopqrstuvwxyz",
		"_9876543210__9876543210_9876543210", 
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 
		"Steven", "John", "Donald", "Krzysztof", "Hamilton", 
		"Jerry", "Saundra", "Steve", "Kelly", "Richard", "Roy",
		"Jennifer", "Claire"
			   };
static	char	*l_name[] = { " ", "b", "b23456789_", 
		"bcdefghijklmnopqrstuvwxyza",
		"b9876543210__9876543210_9876543210", 
		"cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		"Pennebaker", "Simutis", "Johnson", "Stec", "Whitener",
		"Dufour", "Flaggs", "Doan", "Park", "Lamberty", "Tessler",
		"Johnson", "Leachman", "Brussels"
			   };
static	char	*titles[] = { " ", "c", "123456789_", 
		"cdefghijklmnopqrstuvwxyzab",
		"c9876543210__9876543210_9876543210", 
		"cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		"student", "Member of Technical Staff", "Manager", 
		"Director", "Vice President", "Exective Vice President",
		"CEO"
			   };

static	char    *middle[]= { "A", "B", "C", "D", "E", "F", "G", "H", "I",
                            "J", "K", "L", "M", "N", "O", "P", "Q", "R",
                            "S", "T", "U", "V", "W", "X", "Y", "Z", " "
                           };

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 29, pass %d, Insert into table data11 \n",
        s, c);
    SIflush (stdout);
  }

  STprintf(array_buffer, "tst29 non_key data for each row, slave %d, reps %d, loop_counter %d",
                s, c, i);

  EXEC SQL SAVEPOINT tst29save;

  for (i=1, j=c, k=s; i <= c+1; i++, j++, k++)
  {
    Emp_Num = (i*s*5);
    element = ((i*s) % 19);
    STcopy (f_name[element], First_Name);
    if ( element == 0 ) { element = 19; }
    element = ((j*s) % 20);
    STcopy (l_name[element], Last_Name);
    if ( element == 0 ) { element = 20; }
    element = (( i*j*k ) % 27);
    STcopy (middle[element], Middle_Init);
    if ( element == 0 ) { element = 27; }
    element = ((k*s) % 13);
    STcopy (titles[element], Emp_Title);
    if ( element == 0 ) { element = 13; }

    EXEC SQL REPEATED INSERT INTO DATA11 (lt_name, ft_name, mid_init,
		emp_num, emp_titl, st_date, text) 
	VALUES (:Last_Name, :First_Name, :Middle_Init, :Emp_Num, :Emp_Title,
		'15-Apr-1991', :key_buffer); 

    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR);
    if (ing_err != 0)
    {
      EXEC SQL ROLLBACK;
      return;
    }
    if (traceflag)
    {
      SIprintf ("\t\t inserting into data11 similiar row with key value %d \n",
	i );
      SIflush (stdout);
    }
  }


}
  

	
/*
** Function tst30
** Purpose:	Test the Update function with the new btree.  The reason for 
**	the special test is that Update internally, is two functions.  
**	Internally, to update a row, the system must first Delete the row, then 
**	it will re-insert the row with the new values.  
**
**	The function which would be run 60 times by each slave would follow 
**	the following pseudo code: 
**
**	for (i=0; i <= repetition number; i++)
**	{
**		switch( ((i * slave number) MOD 3) )
**		case 0:
**		{
**			update rows where key < lower range 
**				key = key * 3
**		}
**		case 1:
**		{
**			update rows where lower range < key < upper range
**				key = key * 4 - 2047
**		}
**		case 2:
**		{
**			update rows where key > upper range 
**				key = key / 2
**		}
**		default:
**		{
**			update rows everything 
**				key = key + 4095
**		}
**	}
**
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst30(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{

EXEC SQL BEGIN DECLARE SECTION;
# define	range_1		392
# define	range_2		1536
# define	range_3		3700
# define	range_4		7260
# define	range_5		12717
# define	range_6		20748
# define	range_7		32758

	int	i;
	int	j;
	int	key;
	int	row_count;
	int	sleep_value;
	char	First_Name[51]; 
	char	Last_Name[51]; 
	char	Middle_Init[2]; 
	int	Emp_Num; 
	char	Emp_Title[51]; 
	char	array_buffer[100]; 
static	char	key_buffer[30] = "tst30 string field data"; 
EXEC SQL END DECLARE SECTION;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 30, pass %d, Insert into table data11 \n",
        s, c);
    SIflush (stdout);
  }

    EXEC SQL SAVEPOINT tst30save;

    sleep_value = (s * s) * 100; 
    PCsleep( (u_i4)sleep_value);

    STcopy (array_buffer, " ");
    j = (((c * 11) * (s * 13)) % 7);
    switch( j )
    {
      case 0:
      {
	STprintf(array_buffer, "tst30 text data for case 0, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num*20+400, text = :array_buffer
	  WHERE Emp_Num < :range_1;

	break;
      }
      case 1:
      {
	STprintf(array_buffer, "tst30 text data for case 1, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num*4-50, text = :array_buffer
	  WHERE Emp_Num > :range_1 AND Emp_Num < :range_2;

	break;
      }
      case 2:
      {
	STprintf(array_buffer, "tst30 text data for case 2, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num*9+1024, text = :array_buffer
	  WHERE Emp_Num > :range_2 AND Emp_Num < :range_3;

	break;
      }
      case 3:
      {
	STprintf(array_buffer, "tst30 text data for case 3, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num / 10 - 100, text = :array_buffer
	  WHERE Emp_Num > :range_3 AND Emp_Num < :range_4;

	break;
      }
      case 4:
      {
	STprintf(array_buffer, "tst30 text data for case 4, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num + 8507, text = :array_buffer
	  WHERE Emp_Num > :range_4 AND Emp_Num < :range_5;

	break;
      }
      case 5:
      {
	STprintf(array_buffer, "tst30 text data for case 2, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num / 9 - 1024, text = :array_buffer
	  WHERE Emp_Num > :range_5 AND Emp_Num < :range_6;

	break;
      }
      case 6:
      {
	STprintf(array_buffer, "tst30 text data for case 2, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num+5000, text = :array_buffer
	  WHERE Emp_Num > :range_6 AND Emp_Num < :range_7;

	break;
      }
      default:
      {
	STprintf(array_buffer, "tst30 text data for case default, slave %d",
                s);
	EXEC SQL REPEATED UPDATE data11 
	  SET Emp_Num = Emp_Num / 62 * 5 - 255, text = :array_buffer
	   WHERE Emp_Num > :range_7;

	break;
      }
    }

    EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :row_count=rowcount);
    if (ing_err != 0)
    {
      EXEC SQL ROLLBACK;
      return;
    }
    if (traceflag)
    {
      SIprintf ("\t\t inserting into data11 similiar row with key value %d \n",
	key);
      SIflush (stdout);
    }

}
  

	
/*
** Function TST31
** Purpose: Rollback test with btree inserts.  The test will insert various
**		rows into the data2 table.  The rows that will be inserted are 
**		determined by a calculated value.  The calculated value is 
**		the number of slaves times the iteration number, plus the 
**		slave number.  After the insertions are done, the same 
**		calculated value is used again to determine if a rollback will 
**		be done.  If the calculated value is even, a rollback will be 
**		performed to take out the inserted rows.  The insertions are 
**		committed if the value is odd.  
**		
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst31(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int value;
	int num = 0;
EXEC SQL END DECLARE SECTION;


  EXEC SQL SELECT num_kids 
	INTO :num
	FROM num_slaves;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  
  value = s + (num * c);
  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 31, pass %d, inserting %d \n",s,c, value);
	SIflush (stdout);
  }

  EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
			  char61a, char100)
  VALUES (:c, :s, :value, :c*:s, :value, '10-dec-1989',
	        'x1-1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }

  EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
			  char61a, char100)
  VALUES (:c, :s, :value, :c*:s, :value+1, '11-dec-1989',
	        'x2-2xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }

  EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
			  char61a, char100)
  VALUES (:c, :s, :value, :c*:s, :value+2, '12-dec-1989',
	        'x3-3xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }

  EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
			  char61a, char100)
  VALUES (:c, :s, :value, :c*:s, :value+3, '13-dec-1989',
	        'x4-4xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }

  EXEC SQL REPEATED INSERT INTO data2(i1, i2, i4, f4, f8, date,
			  char61a, char100)
  VALUES (:c, :s, :value, :c*:s, :value+4, '14-dec-1989',
	        'x5-5xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');


  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  if (ing_err  != 0)
  {
	return;
  }

  if ((value % 2) == 0)
  {
    EXEC SQL ROLLBACK; 
  }
}

	
/*
** Function TST32
** Purpose: Rollback test with btree deletes.  The test will delete various
**		rows from the data2 table.  The rows that will be deleted is 
**		determined by a calculated value.  The calculated value is 
**		the number of slaves times the iteration number, plus the 
**		slave number.  After the delete is done, the same calculated 
**		value is used again to determine if a rollback will be done.  
**		If the calculated value is even, a rollback will be performed 
**		to restore the deleted rows.  The deletions are committed 
**		if the value is odd.  
**		
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst32(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int value;
	int num = 0;
	int temp;
EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT num_kids 
	INTO :num
	FROM num_slaves;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  
  value = s * (num * c);
  temp = value % 3;

  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 32, pass %d, inserting %d \n",s,c, value);
	SIflush (stdout);
  }

  switch (temp)
  {
    case 0: 
    {
	temp = value % 127;
	EXEC SQL DELETE FROM data2 WHERE i1 = :temp AND i2 < 0; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
    case 1: 
    {
	temp = value % 127;
	EXEC SQL DELETE FROM data2 WHERE i1 = :temp AND i2 > 0 AND i2 < 10000; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
    default: 
    {
	temp = value % 127;
	EXEC SQL DELETE FROM data2 WHERE i1 = :temp AND i2 > 10000; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
  }

  if ((value % 2) == 0)
  {
    EXEC SQL ROLLBACK; 
  }

}

	
/*
** Function TST33
** Purpose: Rollback test with btree updates.  The test will update various
**		rows from the data2 table.  The rows that will be updated is 
**		determined by a calculated value.  The calculated value is 
**		the number of slaves times the iteration number, plus the 
**		slave number.  After the updates are done, the same calculated 
**		value is used again to determine if a rollback will be done.  
**		If the calculated value is even, a rollback will be performed 
**		to restore the updated rows.  The updates are committed 
**		if the value is odd.  
**		
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst33(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
	int value;
	int num = 0;
	int temp;
EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT num_kids 
	INTO :num
	FROM num_slaves;
  EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
  
  value = s * (num * c);
  temp = value % 3;

  if (traceflag)
  {
	SIprintf ("Running SLAVE %d: Rout 33, pass %d, inserting %d \n",s,c, value);
	SIflush (stdout);
  }

  switch (temp)
  {
    case 0: 
    {
	temp = value % 127;
	EXEC SQL UPDATE data2 SET i2 = i2 + 1, date = '03-jan-1992' 
		WHERE i1 = :temp AND i2 < 0; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
    case 1: 
    {
	temp = value % 127;
	EXEC SQL UPDATE data2 SET i2 = i2 + 2, date = '04-jan-1992' 
		WHERE i1 = :temp AND i2 < 0; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
    default: 
    {
	temp = value % 127;
	EXEC SQL UPDATE data2 SET i2 = i2 + 3, date = '05-jan-1992' 
		WHERE i1 = :temp AND i2 < 0; 

	EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
	if (ing_err  != 0)
	{
		return;
	}

	break;
    }
  }

  if ((value % 2) == 0)
  {
    EXEC SQL ROLLBACK; 
  }

}

	
/*
** Function TST126
** Purpose:	This routine performs a TP1 transaction in which the relations 
**		account, teller branch and history get updated in a MQT. It
**		uses a seed which should be put in the seeds table before 
**		running the test. 
** Parameters:
**          s = slave number.
**	    c = number of repetitions.
** Returns:
*/

tst126(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int	s;
	int	c;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
        int     seed;
	int	scale;
	int	account_no;
	int	teller_no;
	int	branch_no;
	double	v_amount;
	int	row_cnt;
	int	rnum;
EXEC SQL END DECLARE SECTION;

	i4	type;

  EXEC SQL SELECT seed_val INTO :seed FROM seeds;
  EXEC SQL SELECT scale_val INTO :scale FROM tp1_scale;

  if (traceflag)
  {
    SIprintf ("Running SLAVE %d: Rout 126 (TP1), pass %d, seed %d \n",
        s, c, seed);
    SIflush (stdout);
  }

  MHsrand (seed + (s * c));
  rnum = MHrand () / 10000;

  teller_no = rnum * TELLERS_PER_BRANCH + scale + 1;

  if (rnum * 100 > 85)
    branch_no = (i4)((rnum * scale * BRANCHS_PER_BANK + 1));
  else 
    branch_no = (teller_no - 1) / BRANCHS_PER_BANK + 1;

  account_no = (branch_no + rnum * ACCOUNTS_PER_BRANCH) / 100 + (s * c);
  if (account_no > 1000000)
    account_no = account_no / 10 + (s * c);
  else if (account_no < 100000)
    account_no = account_no * 10 + (s * c);

  type = rnum * 2 + 1;
  if (type == 2)
    v_amount = -.01;
  else
    v_amount = 1.0;

			/*  Change account record  */
  EXEC SQL REPEATED UPDATE account 
	SET sequence = sequence + 1, balance = balance - :v_amount
	WHERE number = :account_no AND (balance - :v_amount) >= 0;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :row_cnt = rowcount );
  if ( traceflag )
  {
    SIprintf ("\t\tchange account record, account_no = %d rowcount = %d \n",
	account_no, row_cnt );
    SIflush (stdout);
  }

  if (ing_err != 0)
  {
    return;
  }

			/*  Change teller record  */
  EXEC SQL REPEATED UPDATE teller 
	SET balance = balance + :v_amount
	WHERE number = :teller_no;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :row_cnt = rowcount );
  if ( traceflag )
  {
    SIprintf ("\t\tchange teller record, teller_no = %d rowcount = %d \n",
	teller_no, row_cnt );
    SIflush (stdout);
  }

  if (ing_err != 0)
  {
    return;
  }

			/*  Change branch record  */
  EXEC SQL REPEATED UPDATE branch
	SET balance = balance + :v_amount
	WHERE number = :branch_no;
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :row_cnt = rowcount );
  if ( traceflag )
  {
    SIprintf ("\t\tchange BRANCH record, branch_no = %d rowcount = %d \n",
	branch_no, row_cnt );
    SIflush (stdout);
  }

  if (ing_err != 0)
  {
    return;
  }

			/*  Insert an history record  */
  EXEC SQL REPEATED INSERT INTO history 
	( acc_number, br_number, tel_number, amount, pad )
	VALUES (:account_no, :branch_no, :teller_no, :v_amount, '0123456789');
  EXEC SQL INQUIRE_INGRES ( :ing_err = DBMSERROR, :row_cnt = rowcount );
  if ( traceflag )
  {
    SIprintf ("\t\tinserted history record, amount = %d rowcount = %d \n",
	v_amount, row_cnt );
    SIflush (stdout);
  }

  if (ing_err != 0)
  {
    return;
  }

}
/*
** Function TST34
** Purpose: Inserts one row in ascending order. The inserted value will be
**          generated from the sequence object. Multiple slaves will be 
**          responsible for increasing the value of a single sequence object 
**          and inserting that value into the table.
** Parameters:
**          s = slave number.
**          c = number of repetitions.
** Returns:
**
*/
tst34(s,c)
EXEC SQL BEGIN DECLARE SECTION;
        int     s;
        int     c;
EXEC SQL END DECLARE SECTION;
{

	EXEC SQL BEGIN DECLARE SECTION;
		int inserting = 0;
	EXEC SQL END DECLARE SECTION;

	int slaveSleep = (s * c) * 100;
        PCsleep((u_i4)slaveSleep);

        EXEC SQL SELECT tst34seq.nextval INTO :inserting;
        EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
        if(ing_err != 0) { return; }

        if (traceflag)
        {
        SIprintf ("Running SLAVE %d: Rout 34, pass %d,inserting %d \n",s,c,inserting);
        SIflush (stdout);
        }
  
	EXEC SQL REPEATED INSERT INTO data1 (tbl_key , rout , slave , cnt )
        	VALUES (:inserting, 34, :s, :c);
        EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
        if (ing_err != 0) { return; }
}
/*
** Function TST35
** Purpose: Multiple slaves will increase or decrease the value of 
**          multiple sequence objects, then each slave will insert its
**          value into the table data1. This routine will check a subset 
**          of options used with sequence objects. 
** Parameters:
**          s = slave number.
**          c = number of repetitions.
** Returns:
**
*/
tst35(s,c)
EXEC SQL BEGIN DECLARE SECTION;
	int s;
	int c;
EXEC SQL END DECLARE SECTION;
{

           EXEC SQL BEGIN DECLARE SECTION;
		int inserting;
                char prepare_buff[250];
	   EXEC SQL END DECLARE SECTION;

	        int slaveSleep = (s * c )* 100;
                PCsleep((u_i4)slaveSleep);

                STprintf(prepare_buff,"select tst35seq%d.nextval",s);
                 
                EXEC SQL PREPARE stmt FROM :prepare_buff;
                EXEC SQL DECLARE seq_cur CURSOR for stmt;
                EXEC SQL open seq_cur;
                EXEC SQL fetch seq_cur into :inserting;
                EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
                if (ing_err != 0) { return; }
                EXEC SQL close seq_cur;

if (traceflag){
SIprintf ("Running SLAVE %d: Rout 35,pass %d, inserting %d \n", s,c,inserting);} 
           EXEC SQL REPEATED INSERT INTO data1 (tbl_key , rout , slave , cnt )
               VALUES (:inserting, 35, :s, :c);

           EXEC SQL INQUIRE_INGRES (:ing_err = DBMSERROR);
           if (ing_err != 0) { return; }
	
}
