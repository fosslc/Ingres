/* History:
** --------
** 02/09/93 (erniec)
**      modify "key" fields to "key1", "key" is now a keyword in 6.5
**
**	create tst68-71 to do DSQL selects from pre-created views
**	these are new test routines for testcase 4 (xaa04s.sep)
**		instead of tst32-35 which did create view, select, drop view
**
**
** 02/03/93 (erniec)
**	removed unused ERR_DEADLOCK, ERR_OUT_OF_LOCKS, ERR_LOGFULL
**	all tst() test routines - add trace message if (sqlca.sqlcode < 0)
**	add variable levels of tracing information, traceflag>1 for debugging
**		traceflag = 1 prints messages on all errors encountered
**	tst26 - modify if statements on cursor opens
**	tst29 - modify error messages
**	tst32-35 - modify error handling: dropping views, freeing memory, and
**		logging errors with "goto", set lockmode timeout
**		generate specific view names for each routine
**	tst36-40 - set lockmode timeout for undetectable distributed deadlock
**	tst37 - remove 40X loop, we can control with testreps
**	tst52 - modify if statements on cursor opens
**
**
** 08/13/92 (erniec)
**	Corrected tst33 and tst34 to check the result column data type
**	If it is money (+-5) or date (+-3) "describe" sets sqllen to 0.
**	If it is either of these 2 types, set the calloc "len" such that
**	len = 8 for money and len = 25 for date.
**
**	Correct tst50 to open cursor c6b instead of c6.
**
**
**	06/10/92	Changed tst18() to call error handling routine
**			merr instead of sqlprint
**
**
**	add code for routine 99, a performance routine used in 
**	mtstest10-based tests using the HISTORY table.
**
**	Added entries for tests beyond 40, up to 99.
**
**	The entries are in the declaration and the GLOBALDEF,
**	and stub routines are in place to receive functional
**	code.
**		js 2/3/92
**
**	Renamed existing functions tst22() to tst28(), tst23() to tst29() and
**	tst24() to tst30(), then added new functions tst22() to tst27() 
**	inclusive.
**			Steve R. on 15-Aug-1990
**
**	26-aug-1993 (dianeh)
**		Added #include of <st.h>.
**
**	29-sep-1995 (somsa01)
**		Added definition of sleep(x) to be Sleep(x*1000) (Win32), since
**		sleep is not defined on NT_GENERIC platforms.
**	30-Sep-1998 (merja01)
**      Change sql long types to i4 to make test protable to axp_osf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

exec sql include sqlca;
exec sql include sqlda;
exec sql whenever sqlerror call merr;

# include	<compat.h>
# include	<si.h>
# include	<st.h>
# include 	<mh.h>
# include 	<lo.h>

# define	ERR_DROP 	-2753
# define	TBLSIZE		 5120	/* for tst28, i.e. the  old tst22 */
# define	BRANCHS_PER_BANK	10
# define	TELLERS_PER_BRANCH	10
# define	ACCOUNTS_PER_BRANCH	1000
# define	LARGEST_DEPOSIT		1000
# define	MAX_SCAN		100
# define	DATE_TYPE		3
# define	MONEY_TYPE		5
# define	DATE_LEN		25
# define	MONEY_LEN		8
# ifdef NT_GENERIC
# define	sleep(x)		Sleep(x*1000)
# endif

/*
**
**	each test routine must have an entry in the 
**	following list.  js 2/3/92
**
*/

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
int	tst35();
int	tst36();
int	tst37();
int	tst38();
int	tst39();

int	tst40();
int	tst41();
int	tst42();
int	tst43();
int	tst44();
int	tst45();
int	tst46();
int	tst47();
int	tst48();
int	tst49();

int	tst50();
int	tst51();
int	tst52();
int	tst53();
int	tst54();
int	tst55();
int	tst56();
int	tst57();
int	tst58();
int	tst59();

int	tst60();
int	tst61();
int	tst62();
int	tst63();
int	tst64();
int	tst65();
int	tst66();
int	tst67();
int	tst68();
int	tst69();

int	tst70();
int	tst71();
int	tst72();
int	tst73();
int	tst74();
int	tst75();
int	tst76();
int	tst77();
int	tst78();
int	tst79();

int	tst80();
int	tst81();
int	tst82();
int	tst83();
int	tst84();
int	tst85();
int	tst86();
int	tst87();
int	tst88();
int	tst89();

int	tst90();
int	tst91();
int	tst92();
int	tst93();
int	tst94();
int	tst95();
int	tst96();
int	tst97();
int	tst98();
int	tst99();


GLOBALREF     i4 traceflag;              /* flag to print detailed results */


/*
**	each test routine must have an entry in
**	the following GLOBALDEF.  js 2/3/92
**
*/


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
	tst35,
	tst36,
	tst37,
	tst38,
	tst39,

	tst40,
	tst41,
	tst42,
	tst43,
	tst44,
	tst45,
	tst46,
	tst47,
	tst48,
	tst49,

	tst50,
	tst51,
	tst52,
	tst53,
	tst54,
	tst55,
	tst56,
	tst57,
	tst58,
	tst59,

	tst60,
	tst61,
	tst62,
	tst63,
	tst64,
	tst65,
	tst66,
	tst67,
	tst68,
	tst69,

	tst70,
	tst71,
	tst72,
	tst73,
	tst74,
	tst75,
	tst76,
	tst77,
	tst78,
	tst79,

	tst80,
	tst81,
	tst82,
	tst83,
	tst84,
	tst85,
	tst86,
	tst87,
	tst88,
	tst89,

	tst90,
	tst91,
	tst92,
	tst93,
	tst94,
	tst95,
	tst96,
	tst97,
	tst98,
	tst99


};

/*
**
**	Be sure that last one does NOT have a 
**	comma at the end.  JS  2/3/92
**
*/
	
int 
trlsize()
{
	return ( (sizeof(trl)/sizeof(struct trl_routine)) - 1);
}
	

tst0(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql insert into test (num,slave,routs,s,c)
		values (100,0,0, :s, :c);

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("insert into test - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}


tst1(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int num;
exec sql end declare section;

	exec sql select key1
		into :num
		from data1
		where cnt = :c;
	exec sql begin;

	exec sql end;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("select key1 from data1 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}

tst2(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql insert into data2 (key1, slave, rout)
		values (:c, :s, 2);

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("insert into data2 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}


tst3(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql insert into data3 (i1,i2,i4,f4,f8,date,char61)
		values (:c, :c+10, :s, :c*5, :c*10, '31-dec-1999',
			 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx');

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("insert into data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}


tst4(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql delete from data3 where i1 = :c;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("delete from data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}


tst5(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql update data3 
		set 	i1 = i1+1,
			i4 = :s;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}

tst6(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int 	num;
	int 	r, m, iterations,i, seed,x;
exec sql end declare section;

	exec sql select seed_val
		into :seed
		from seeds;
		
	srand( seed+(s*c) );

	num = rand();
	r = num%(10000-1) + 1;
	m = r%5;


	if ( m == 0 )
	{
/*		SIprintf ("m is %d, appending %d\n",m,r); */
/*		SIflush(stdout); */

		exec sql repeated insert into data4 
				(i1, i2, i4, f4, f8, date, money, char61a, char100)
			values  (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999', 12.12,
		 		  'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		 		  'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=0: repeated insert into data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		if ( r%2 == 0 )
		{
/*			SIprintf ("also deleting %d\n",r); */
/*			SIflush(stdout); */

			exec sql repeated delete from data4 where i2 = :r;
			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
				{
					SIprintf("m=0: repeated delete from data4 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		};
	   	exec sql commit;
	};

	if ( m == 1 )
	{
/*		SIprintf ("m is %d, appending & deleting %d\n",m,r); */
/*		SIflush(stdout); */

		exec sql repeated insert into data4 
				(i1, i2, i4, f4, f8, date, money, char61a, char100)
			values  (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999', 17.23,
		 		  'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		 		  'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=1: repeated insert into data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql repeated delete from data4 
			where i2 >= :r and i2 < :r*2 and i4 > :r;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=1: repeated delete from data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
		exec sql commit;
	};


	if ( m == 2 )
	{
/*		SIprintf ("m is %d, replacing r %d\n",m,r); */
/*		SIflush(stdout); */
		exec sql repeated update data4 
			set i2=:r+1, i4=i4+1000
			where i2=:r and i4>:r;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=2: repeated update data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
		exec sql commit;
	};


	if ( m == 3 )
	{
/*		SIprintf ("m is %d, retrieving with readlock no lock\n",m); */
/*		SIflush(stdout); */
		exec sql commit;
		exec sql set lockmode on data4 where readlock=nolock;
		exec sql select i2
			into :r
			from data4;
		exec sql begin;
			/* SELECT loop, disgard the data */
		exec sql end;
		exec sql commit;
		exec sql set lockmode on data4 where readlock=shared;
	};


	if ( m == 4 )
	{
/*		SIprintf ("m is %d, retrieve, delete, append and replace %d\n",m,r); */
/*		SIflush(stdout); */
		exec sql repeated select i2
			into :num
			from data4
			where i2 = :r;
		exec sql begin;
			/* SELECT loop, disgard the data */
		exec sql end;

		exec sql repeated delete from data4
			where i2 = :r;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=4: repeated delete from data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql repeated insert into data4 
				(i1, i2, i4, f4, f8, date, money, char61a, char100)
			values  (:c, :r, :r*2, :c*5, :c*10, '31-dec-1999', 19.95,
		 		  'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
		 		  'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy');
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=4: repeated insert into data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql repeated update data4 
			set i2 = 1, i4=1000
			where i2 = :r;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("m=4: repeated update data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
		exec sql commit;
	};
}


tst7(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int i;
exec sql end declare section;


	for (i=1; i<c; i++)
	{
		exec sql repeated insert into data3 
				(i1, i2, i4, f4, f8, date, money, char61)
			values  (:c, :c+10, :i, :c*5, :c*10, '31-dec-1999',
					737.93,
		 		  'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx');
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("repeated insert into data3 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
	};

	exec sql commit;
}

tst8(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql delete from data3 
		where i1=:c and i2=:c+10 and f4=:c*5;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("delete from data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}

tst9(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql update data3
		set i2=i2-100, i4=i4*2
		where date = '31-dec-1999';

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;
}


tst10(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
	/* this routine appends 128 rows to the table width500 */
	/* rows added will be 1000 + (slave_no * 128) */

	int	i;
	int	base;
	int	first;
	int	last;

exec sql begin declare section;
	int	v_i2;
	int	v_f4;
	int	v_f8;
	int	v_dollar;
exec sql end declare section;

	base = 1000;
	first = base + (s + c - 2)*128 + 1;
	last = base + (s + c - 1)*128 + 1;


	for (i=first; i<last; i++)
	{
		v_i2 = i ;
 	  	v_f4 = i ;
	  	v_f8 = i ;
	  	v_dollar = i;

/*		SIprintf ("appending tuple where i2 = %d\n\n",i);	*/

		exec sql repeated insert into width500 
			(i2,f4,f8,dollar,day,c6,txt58,txt98one,txt98two,txt98three, txt98four)
			values (:v_i2, :v_f4, :v_f8, :v_dollar, '1/1/1987', 'abcedf', 
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
				);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("repeated insert into width500 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

	} /* end for loop*/
	exec sql commit;
}


tst11(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
	/* used with tst12, will lock 50 tuples, 12 pages of data */


	exec sql update width500 
		set f4 = f4*100, txt58 ='tst11_txt58_replaced_value'
		where i2 > 50 and i2 < 80;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update width500  where i2 > 50 and i2 < 80 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
}


tst12(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql update width500 
		set f4 = f4-100, txt58 = 'tst12_txt58_replaced_value'
		where i2 > 100 and i2 < 150;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update width500 where i2 > 100 and i2 < 150 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
}


tst13(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql update width500 
		set f4 = f4+500, txt58 = 'tst13_txt58_replaced_value'
		where i2 > 50 and i2 < 80; 

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update width500 where i2 > 50 and i2 < 80 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
}


tst14(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql update width500 
		set f4 = 1, txt58 = 'tst14_txt58_replaced_value'
		where i2 > 70 and i2 < 100; 

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update width500 where i2 > 70 and i2 < 100 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
}


tst15(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	/* Used with tst11 will lock the same 128 rows, 32 pages */

	/* retrieve into slave_no * 128 rows into a temp table */
	/* since there are 4 tuples per page, 128 rows will have 32 pages */
	/* which is enough to fill up the AM buffer. */


	exec sql create table ret_temp 
		as select * from width500
 		where i2 < :s*128;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("create table ret_temp from width500 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
}



tst16(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	/* Same as tst15 except set lockmode sessions where readlock=nolock */

	/* retrieve into slave_no * 128 rows into a temp table */
	/* since there are 4 tuples per page, 128 rows will have 32 pages */
	/* which is enough to fill up the AM buffer. */

	exec sql commit;
	exec sql set lockmode session where readlock = nolock;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("set lockmode readlock = nolock - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql create table ret_temp 
		as select * from width500
 		where i2 < :s*128;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("create table ret_temp from width500 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;
	exec sql set lockmode session where readlock = shared;
}

tst17(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

/*
**	***************************************
**	*                                     *
**	* This will NOT work with STAR..      *
**	*                                     *
**	* STAR does not permit modifying the  *
**	* structure of tables.                *
**	*                                     *
**      ***************************************
*/

	/* modify widht500 to other structures, should get Xclusive table lock */


	switch (c) {
	case '1':
               if (traceflag)
                {
		SIprintf ("Modifying to btree\n\n");
		SIflush (stdout);
		}

		exec sql modify width500 to btree 
			with fillfactor=95, indexfill = 85; 
		exec sql create index width500ind on width500 
			(f8,dollar,day,c6,txt98four) 
			with structure=hash; 
		break;
	case '2':
               if (traceflag)
                {
		SIprintf ("Modifying to isam\n\n");
		SIflush (stdout);
		}

		exec sql modify width500 to isam 
			with fillfactor = 95;
		exec sql create index width500ind on width500 
			(txt58,txt98one,txt98two,txt98three) 
			with structure=btree; 
		break;
	case '3':
               if (traceflag)
                {
		SIprintf ("Modifying to hash\n\n");
		SIflush (stdout);
		}

		exec sql modify width500 to chash on f4,f8 
			with fillfactor=90, maxpages = 100;
		break;
	case '4':
               if (traceflag)
                {
		SIprintf ("Modifying to cbtree\n\n");
		SIflush (stdout);
		}

		exec sql modify width500 to cbtree;
		break;
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;
}


tst18(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

	exec sql whenever sqlerror continue;
	exec sql drop table ret_temp;
	exec sql whenever sqlerror call merr;
	
	/* error -5202, failed to destroy due to non-exist table, it's OK */
	/* since the slave is not expecting this error juct change it to 0 */ 
	/* instead of the actual ingres error no. */
	if (sqlca.sqlcode == ERR_DROP)
	{
		sqlca.sqlcode = 0;
	}
	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;
}


/*
**	The purpose of tests 19, 20 and 21 is to exercise the system catalogs in
**	a concurrent environment by doing creates, destroys, modifies,
**	and define permits.
*/


tst19(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char	table_name[33];
	char	init_table[33];
	char	query[100];
exec sql end declare section;
	char	string[20];

/*	IN THIS TEST WE USE A DIFFERENT TABLE FOR EACH SLAVE.
**	THE DATABASE SHOULD HAVE TABLES LIKE DATA10_1, DATA10_2
**	DATA10_3, ETC.
*/
 
	CVla(s, string);
	STcopy("tbl", table_name);
	STcat(table_name, string);
	STcopy("data10_", init_table);
	STcat(init_table, string);

        if (traceflag)
        {
	SIprintf ("\nCreating the table %s\n",table_name);
	SIflush(stdout);
	}

 	STprintf(query,"create table %s as select * from %s",table_name,init_table);

	exec sql execute immediate :query;

	if (sqlca.sqlcode == 0)
		sqlca.sqlcode = 0;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	STprintf(query,"grant all on table %s to public",table_name);

	exec sql execute immediate :query;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}


tst20(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{


/*
**      ***************************************
**      *                                     *
**      * This will NOT work with STAR..      *
**      *                                     *
**      * STAR does not permit modifying the  *
**      * structure of tables.                *
**      *                                     *
**      ***************************************
*/

exec sql begin declare section;
	char	table_name[33];
	char	query[100];
	char	index_name[33];
	char	index1[33], index2[33], index3[33], index4[33];
exec sql end declare section;
	char	string[20];

	STcopy("tbl", table_name);
	CVla(s, string);
	STcat(table_name, string);
	STcopy("ind", index_name);
	STcat(index_name, table_name);

        if (traceflag)
        {
	SIprintf ("\nModifying the table %s\n",table_name);
	SIflush(stdout);
 	}

	switch (c) {
	case 1:
               if (traceflag)
                {
		SIprintf ("Modifying to btree unique, 1 index\n\n");
		SIflush(stdout);
		}
		STprintf(query, "modify %s to btree unique with fillfactor=95, indexfill = 50, maxindexfill = 90", 
			table_name);
		

		exec sql execute immediate :query;

		STprintf(query, "create index %s on %s (i4, varchar50, date, moneyn,t1n,f8) with structure=hash", 
			index_name, table_name);

		exec sql execute immediate :query;
		break;

	case 2:
               if (traceflag)
                {
		SIprintf ("Modifying to isam, 4 indeces\n\n");
		SIflush(stdout);
		}

		STcopy(index_name,index1);
		STcopy(index_name,index2);
		STcopy(index_name,index3);
		STcopy(index_name,index4);
		CVla(1, string);
		STcat(index1, string);
		CVla(2, string);
		STcat(index2, string);
		CVla(3, string);
		STcat(index3, string);
		CVla(4, string);
		STcat(index4, string);

		STprintf (query,"modify %s to isam with fillfactor = 25",table_name);


		exec sql execute immediate :query;

		STprintf (query,"create index %s on %s (i4)", index1, table_name);


		exec sql execute immediate :query;

		STprintf (query,"create index %s on %s (i2,f4,date) with structure=btree",
			 index2, table_name);


		exec sql execute immediate :query;

		STprintf (query,"create index %s on %s (t1n) with structure=hash", 
			index3, table_name);


		exec sql execute immediate :query;

		STprintf (query,"create index %s on %s (varchar50n,char50n) with structure=chash", 
			index4, table_name);

		exec sql execute immediate :query;
		break;

	case 3:
                if (traceflag)
                {
		SIprintf ("\nModifying to chash\n");
		SIflush(stdout);
		}
		STprintf (query,"modify %s to chash on f4,f8 with fillfactor = 90, maxpages=100, minpages=30", 
			table_name);


		exec sql execute immediate :query;
		break;

	case 4:
                if (traceflag)
                {
		SIprintf ("\nModifying to cbtree\n");
		SIflush(stdout);
		}
		STprintf (query,"modify %s to cbtree", table_name);

		exec sql execute immediate :query;
		break;


	case 5:
                if (traceflag)
                {
		SIprintf ("\nModifying to heap \n");
		SIflush(stdout);
		}
		STprintf (query,"modify %s to heap", table_name);

		exec sql execute immediate :query;
		break;
 	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}


tst21(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char	table_name[33];
	char 	query[100];
exec sql end declare section;
	char	string[20];


	STcopy("tbl", table_name);
	CVla(s, string);
	STcat(table_name, string);

        if (traceflag)
        {
	SIprintf ("\nDestroying the table %s\n",table_name);
	SIflush(stdout);
	}

	STprintf(query,"drop table %s", table_name); 


	exec sql execute immediate :query;
	
	if (sqlca.sqlcode == ERR_DROP)
	{
		sqlca.sqlcode = 0;
	}
	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}



tst22(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data5 table */
		int	d5key1;
		int	d5cnt;

		/* Declarations for data6 table */
		int	d6key1;
		float	d6float;
		int	d6cnt;

		/* Declarations for data7 table */
		int	d7key1;
		float  	d7money;

		/* Declarations for data8 table */
		int	d8key1;
		char 	d8char[66];

		/* Declarations for data9 table */
		int	d9key1;
		char 	d9date[26];

		/* Useful vars */
		int	done;		/* to signal end of cursor */
		int	sleeptime;	/* introduce a variable wait... */
	exec sql end declare section;


	/*  plain cursor, retrieves all rows of a single table  */
	exec sql declare c1 cursor 
			for select key1, cnt 
			from data5;

	/* with qualification and join*/
	exec sql declare c2 cursor 
			for select a.key1, a.cnt, b.floatv, f.money, d.charv,
									e.date
			from data5 a, data6 b, data7 f, data8 d, data9 e
			where a.cnt < 50
			and a.key1 = b.key1 
			and a.key1 = f.key1 
			and a.key1 = d.key1 
			and a.key1 = e.key1;

	/*  a join, and ORDER BY  */
	exec sql declare c3 cursor 
			for select a.key1, a.cnt, b.floatv, f.money, d.charv,
									e.date
			from data5 a, data6 b, data7 f, data8 d, data9 e
			where a.cnt > 20
			and a.key1 = b.key1
			and a.key1 = f.key1
			and a.key1 = d.key1
			and a.key1 = e.key1
			order by a.key1, a.cnt;

	/*  another  join, and ORDER BY  */
	exec sql declare c4 cursor 
			for select a.key1, a.cnt, b.cnt, b.floatv, f.money, 
								d.charv, e.date
			from data5 a, data6 b, data7 f, data8 d, data9 e
			where a.cnt > 10
			and b.cnt < 200
			and a.key1 = b.key1
			and a.key1 = f.key1
			and a.key1 = d.key1
			and a.key1 = e.key1
			order by a.cnt, b.cnt, f.money;

	/*  another plain retrieve */
	exec sql declare c5 cursor 
			for select a.key1, a.cnt, b.floatv, f.money, d.charv,
									e.date
			from data5 a, data6 b, data7 f, data8 d, data9 e
			where a.key1 = b.key1
			and a.key1 = f.key1
			and a.key1 = d.key1
			and a.key1 = e.key1;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c5 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return; 
	}


	exec sql commit;

	exec sql set lockmode on data5 where readlock=nolock;
	exec sql set lockmode on data6 where readlock=nolock;
	exec sql set lockmode on data7 where readlock=nolock;
	exec sql set lockmode on data8 where readlock=nolock;
	exec sql set lockmode on data9 where readlock=nolock;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("set lockmode readlock=nolock - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	}


/*
**	inserted and commented out; jds 19-SEP-1991
**	seems to be a real problem with locks here...
**
**	and put back 1-nov
*/


	sleeptime = 50 * s;
/*
**	adjust this from 1000 to 100 - 1000 was too long
**	for slave 30    jds   4-NOV-1991 16:14
**
**	and shrink it further to 50  12-nov-1991  jds
*/


	/*  GENERIC RETRIEVE BEGIN  */

        if (traceflag > 2)
        {
	SIprintf( "\nRetrieve data5 only (all)\n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c1 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c1 for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c1 into :d5key1, :d5cnt;
		
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf( "fetch c1 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

/*
**              if (done && !(sqlca.sqlcode < 0))
**              {
**                      SIprintf( "%d, %d\n\0", d5key1, d5cnt);
**                      SIflush(stdout);
**              }
*/
		PCsleep(sleeptime); 	/* wait about "s" seconds */
	}

	if (traceflag > 2)
	{
		SIprintf( "After fetch c1 loop: sqlcode=%d, done=%d\n\0", sqlca.sqlcode, done);
		SIflush(stdout);
	}


        if (traceflag > 2)
        {
	SIprintf( "Join all tables  where data5.cnt < 50\n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c2 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c2 for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c2 
				into :d5key1, :d5cnt, :d6float, 
					:d7money, :d8char, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf( "fetch c2 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

/*
**		if (done && !(sqlca.sqlcode < 0))
**		{
**			SIprintf( "%d, %d, %f, %f, %s, %s\n\0",
**			  	 d5key1, d5cnt,d6float,'.',d7money,'.',d8char,d9date );
**			SIflush(stdout);
**		}
*/
		PCsleep(sleeptime); 	/*	wait about "s" seconds	 */
	}

	if (traceflag > 2)
	{
		SIprintf( "After fetch c2 loop: sqlcode=%d, done=%d\n\0", sqlca.sqlcode, done);
		SIflush(stdout);
	}

        if (traceflag > 2)
        {
	SIprintf( "Join: all tables where data5.cnt > 10\n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c3 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c3 for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{

		exec sql fetch c3
				into :d5key1, :d5cnt, :d6float, 
					:d7money, :d8char, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf( "fetch c3 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

/*
**		if (done && !(sqlca.sqlcode < 0))
**		{
**			SIprintf( "%d, %d, %f, %f, %s, %s\n\0",
**			  	 d5key1, d5cnt,d6float,'.',d7money,'.',d8char,d9date );
**			SIflush(stdout);
**		}
*/

		PCsleep(sleeptime); 	/*	wait about "s" seconds	 */

	}

	if (traceflag > 2)
	{
		SIprintf( "After fetch c3 loop: sqlcode=%d, done=%d\n\0", sqlca.sqlcode, done);
		SIflush(stdout);
	}

	if (traceflag > 2)
        {
	SIprintf( "Join: All tables where data5.cnt>10 and data6.cnt<200nt<20 \n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c4 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c4 for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{

		exec sql fetch c4
				into :d5key1, :d5cnt, :d6cnt, :d6float,
					:d7money, :d8char, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf( "fetch c4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

/*
**		if (done && !(sqlca.sqlcode < 0))
**		{
**			SIprintf( "%d, %d, %f, %f, %s, %s\n\0",
**			  	 d5key1, d5cnt,d6float,'.',d7money,'.',d8char,d9date );
**			SIflush(stdout);
**		}
*/
		PCsleep(sleeptime); 	/*	wait about "s" seconds	 */

	}

	if (traceflag > 2)
	{
		SIprintf("After fetch c4 loop: sqlcode=%d, done=%d\n\0", sqlca.sqlcode, done);
		SIflush(stdout);
	}

        if (traceflag > 2)
        {
	SIprintf( "Join all tables \n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c5 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c5 for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c5
				into :d5key1, :d5cnt, :d6float, 
					:d7money, :d8char, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf( "fetch c5 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

/*
**		if (done && !(sqlca.sqlcode < 0))
**		{
**			SIprintf( "%d, %d, %f, %f, %s, %s\n\0",
**			  	 d5key1, d5cnt,d6float,'.',d7money,'.',d8char,d9date );
**			SIflush(stdout);
**		}
*/
		PCsleep(sleeptime); 	/*	wait about "s" seconds	 */

	}

	if (traceflag > 2)
	{
		SIprintf( "After fetch c5 loop: sqlcode=%d, done=%d\n\0", sqlca.sqlcode, done);
		SIflush(stdout);
	}

	exec sql commit;

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("\n COMMIT error %d\n\0", sqlca.sqlcode);
		SIflush(stdout);

		return;
	}

} /* End of tst22() */


tst23(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data5 table */
		int	d5key1;
		int	d5cnt;
		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;


	/*  direct update using join */
	exec sql declare c6 cursor
			for select key1, cnt
			from data5
			where key1 > 30 and key1 < 300
			for direct update of key1, cnt;

	if (traceflag > 2)
        {
	SIprintf( "\nDirect update where 30<data5.key1<300 \n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c6;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c6 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c6 into :d5key1, :d5cnt;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
       			{
				SIprintf("fetch c6 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);

		if (!done && !(sqlca.sqlcode < 0))
		{
/*			SIprintf("Direct   --> key1= %d, cnt=%d\n", d5key1, d5cnt);
**			SIflush(stdout);
*/
			exec sql update data5
					set key1 = key1 + 20
					where current of c6;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data5 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

} /* End of tst23() */




tst24(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data6 table */
		int	d6key1;
		float	d6float;
		int	d6cnt;

		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;


	/*  direct update on the whole table */
	/* Add WHERE clause that is always true to avoid bug 32481 */
	exec sql declare c7 cursor
			for select key1, floatv, cnt
			from data6
			where 1 = 1
			for direct update of key1, floatv;

        if (traceflag > 2)
        {
		SIprintf( "\nDirect update on data6 \n\0" );
		SIflush(stdout);
	}

	done = 0;

	exec sql open c7;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c7 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c7 into :d6key1, :d6float, :d6cnt;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c7 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
		{
			exec sql update data6
					set key1 = key1 - 10,
					floatv = floatv*2
					where current of c7;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data6 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}	/* End of tst24() */


tst25(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data7 table */
		int	d7key1;
		float	d7money;

		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;


	exec sql declare c8 cursor
			for select key1, money
			from data7
			where key1 < 200 
			and money > 10
			for deferred update of key1, money;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("declare c8 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

        if (traceflag > 2)
        {
	SIprintf( "\nDeferred update on data7 \n\0" );
	SIflush(stdout);
	}

	done = 0;


	exec sql open c8;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c8 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c8 into :d7key1, :d7money;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c8 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
		{
			exec sql update data7
					set key1 = key1*2
					where current of c8;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data7 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}	/* End of tst25() */


tst26(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data5 table */
		int	d5key1;
		int	d5cnt;

		/* Declarations for data8 table */
		int	d8key1;
		char 	d8char[66];

		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;

	exec sql declare c9 cursor
			for select a.key1, a.cnt
			from data5 a, data8 d
			where a.key1 = d.key1;

	exec sql declare c10 cursor
			for select key1, charv
			from data8
			where key1 > 10
			and key1 < 100
			for direct update of key1, charv;

        if (traceflag > 2)
        {
	SIprintf( "\nJoin data5 and data8 and replace data8 \n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c9 for readonly;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c9 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql open c10;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
       		{
			SIprintf("open c10 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c9 into :d5key1, :d5cnt;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c9 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
		{
/*
**			SIprintf("c9: %d, %d \n", d5key1, d5cnt);
**			SIflush (stdout);
*/
			exec sql fetch c10 into :d8key1, :d8char;
			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("fetch c10 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}

			exec sql inquire_ingres (:done = ENDQUERY);
			if (!done && !(sqlca.sqlcode < 0))
			{
/*				SIprintf("c10: %d \n", d8key1);
**				SIflush (stdout);
*/
				exec sql update data8
					set key1 = key1*2,
					charv = 'Test 26 replace '
					where current of c10;

				if (sqlca.sqlcode < 0)
				{
					if (traceflag)
        				{
						SIprintf("update data8 - FAILED: %d\n", sqlca.sqlcode);
						SIflush(stdout);
					}
					return;
				}
			}
		}
	}


	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}	/* End of tst26() */



tst27(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data9 table */
		int	d9key1;
		char 	d9date[26];

		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;


	/*  cursor for delete */
	exec sql declare c11 cursor
			for select key1, date
			from data9
			where key1 < 10
			or key1 > 300
			for direct update of key1;

	/* cursor for update */
	exec sql declare c12 cursor
			for select key1, date
			from data9
			where key1 > 10
			or key1 < 300
			for direct update of key1;


	/*  GENERIC RERIEVE BEGIN  */


        if (traceflag > 2)
        {
	SIprintf( "\nDelete data9 where key1 <10 or key1 > 300 \n\0" );
	SIflush(stdout);
	}

	done = 0;
	exec sql open c11;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c11 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c11 into :d9key1, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c11 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
		{
/*			SIprintf( "%d, %s\n\0", d9key1, d9date);
**			SIflush(stdout);
*/
			exec sql delete from data9
					where current of c11;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("delete from data9 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}

	}

	exec sql close c11;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("close c11 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}


        if (traceflag > 2)
        {
	SIprintf( "\nReplace data9 where 10<key1<300 \n\0" );
	SIflush(stdout);
	}

	done = 0;
	exec sql open c12;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c12 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c12 into :d9key1, :d9date;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
       			{
				SIprintf("fetch c12 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
   		{
/*			SIprintf( "%d, %s\n\0", d9key1, d9date);
**			SIflush(stdout);
*/
			exec sql update data9
					set key1 = key1 + 20
					where current of c12;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data9 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}
	}

	exec sql close c12;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("close c12 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;

}	/* End of tst27() */


tst28(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	i4	emp_no;
	i4	smallest_emp_no;
exec sql end declare section;
	


	exec sql select max(eno) into :emp_no from btbl_1;       /* select max */
	exec sql commit;
	exec sql set lockmode on btbl_1 where readlock = shared;

        emp_no++;

	exec sql insert 
                 into btbl_1 (eno, 
				deptno, 
				ename, 
				deptname, 
				manager, 
				avgsal, 
				sales)
                 values
                 	(:emp_no, 
			:emp_no,
			'xxxxxxxxxxxxxxxxxxxxxxxxxxxx',
                 	'dddddddddddddddddddddddddddddddd',
                  	'mmmmmmmmmmmmmmmmmmmmmmmmmmm',
                  	30000, 
			500000);

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("insert into btbl_1 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;

        smallest_emp_no = emp_no - TBLSIZE;

/*	SIprintf ("\n smallest_emp_no =%d\n",smallest_emp_no);	*/
/*	SIflush(stdout);					*/

	exec sql delete from btbl_1 where eno <= :smallest_emp_no;

	if (sqlca.sqlcode < 0)
	{
		if (sqlca.sqlcode < 0)
		{
			SIprintf("delete into btbl_1 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}
	exec sql commit;

}


tst29(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int		v_amount;
	int		account_no;
	int		branch_no;
	int		teller_no;
	float		persec;
        int		status;
	int		seed;
	int		time[2];
	int		type;
	int		selct;
exec sql end declare section;
	int     scale = 1;

	exec sql commit;   /* end previous transaction */

	EXEC SQL set lockmode on account where
		readlock = exclusive, level = page;

	/*	Begin the transaction. */
	/*  Pick a branch, teller, account, amount and type. */

	    branch_no = (i4)(MHrand() * (f8)BRANCHS_PER_BANK);
	    teller_no = (i4)(MHrand() * (f8)TELLERS_PER_BRANCH);
	    selct     = (i4)(MHrand() * (f8)MAX_SCAN);
	    account_no =(i4)(MHrand() * (f8)ACCOUNTS_PER_BRANCH);
	    v_amount  =  (i4)(MHrand() * (f8)(LARGEST_DEPOSIT));

	    /*  Change account record. */
          exec sql repeat update account
		set sequence = sequence + 1, balance = balance + :v_amount
              where (number = :account_no);

          if (sqlca.sqlcode < 0)
	  {
		if (traceflag)
        	{
			SIprintf("repeat update account - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	      	return;
	  }


	    /*  Change teller record. */

          exec sql repeat update  teller
		set balance = balance + :v_amount
              where (number = :teller_no);

          if (sqlca.sqlcode < 0)
	  {
		if (traceflag)
        	{
			SIprintf("repeat update  teller - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	      	return;
	  }
	    /*  Change branch record. */

          exec sql repeat update branch 
		set balance = balance + :v_amount
              where (number = :branch_no);

          if (sqlca.sqlcode < 0)
	  {
		if (traceflag)
        	{
			SIprintf("repeat update branch - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	      	return;
	  }

	    /*  Update history. */

	exec sql repeat insert into history(acc_number, br_number,
                       tel_number, amount, pad)
  		values (:account_no, :branch_no, :teller_no,
			:v_amount, '0123456789              ');

          if (sqlca.sqlcode < 0)
	  {
		if (traceflag)
        	{
			SIprintf("repeat insert into history - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	      	return;
	  }

         exec sql commit;
}


tst30(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int		v_amount;
	int		account_no;
	int		branch_no;
	int		teller_no;
	float		persec;
        int		status;
	int		seed;
	int		time[2];
	int		type;
	int		selct;
	int		return_value;
exec sql end declare section;
	int     scale = 1;

	EXEC SQL set lockmode on account where
		readlock = exclusive, level = page;

	/*	Begin the transaction. */
	/*  Pick a branch, teller, account, amount and type. */

	    branch_no = (i4)(MHrand() * (f8)BRANCHS_PER_BANK);
	    teller_no = (i4)(MHrand() * (f8)TELLERS_PER_BRANCH);
	    selct     = (i4)(MHrand() * (f8)MAX_SCAN);
	    account_no =(i4)(MHrand() * (f8)ACCOUNTS_PER_BRANCH);
	    v_amount  =  (i4)(MHrand() * (f8)(LARGEST_DEPOSIT));


/*  Call the stored procedure that updates account record, teller record,
**  branch record and inserts new history record.
*/
	    EXEC SQL execute procedure tp1 (acct_no = :account_no,
				            tell_no = :teller_no,
					    branch  = :branch_no,
					    amount  = :v_amount)
		     into :return_value;

          if (sqlca.sqlcode < 0)
	  {
		if (traceflag)
        	{
			SIprintf("execute procedure tp1 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
	      	return;
	  }

         exec sql commit;
}


tst31(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{
	sleep (10);
}


tst32(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s32 statement;

	int		i;	/* index into sqlvar */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view32_1_1", viewname);

	/* Make a unique viewname by appending the slave and count numbers */
	viewname[7] = (s % 10) + '0';
	viewname[9] = (c % 10) + '0';

	STcopy ("", sql_buff);   /* build SQL statement in buffer */
	STcat (sql_buff, "create view %s as ");
	STcat (sql_buff, "select a.key1, a.cnt, b.floatv, c.charv ");
	STcat (sql_buff, "from data1 a, data6 b, data8 c ");
	STcat (sql_buff, "where a.key1 = b.key1 and a.key1 = c.key1");
	STprintf (sql_stmt, sql_buff, viewname);   /* create view viewname */
	
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql commit;

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select key1, floatv, charv ");
	STcat (sql_buff, "from %s where cnt = %d");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select key1, floatv, charv from viewname where cnt = c */

	exec sql prepare s32 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s32 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql describe s32 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s32 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql declare c32 cursor for s32;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c32 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql open c32;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c32 - %s FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   if (traceflag > 4)
           {
		SIprintf("sqv->sqllen: %d\n", sqv->sqllen);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1,sqv->sqllen);

           if(!sqv->sqldata)
           {
                SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                goto close_cursor;
           }
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c32 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c32 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c32); \n");
                SIflush(stdout);
        }

	exec sql close c32;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c32 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}
drop_view:
        if (traceflag > 2)
        {
                SIprintf("Going to drop view %s \n", viewname);
                SIflush(stdout);
        }

	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "drop view %s", viewname);
	exec sql execute immediate :sql_stmt;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

	free ((char *)sqlda);   /* free sqlda memory */


	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, sqlcode is set to that error	*/
	/* sqlcode was getting reset to 0 by a successful drop_view	*/
	/* Note: the last error encountered will be returned		*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }
	
	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}


tst33(s,c)
exec sql begin declare section;
	i4 s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s33 statement;

	int		i;	/* index into sqlvar */
	int		len;	/* calloc length */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;
	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** SQLDA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view33_1_1", viewname);

	/* Make a unique viewname by appending the slave and count numbers */
	viewname[7] = (s % 10) + '0';
	viewname[9] = (c % 10) + '0';

	STcopy ("", sql_buff);   /* build SQL statement in buffer */
	STcat (sql_buff, "create view %s as select ");
	STcat (sql_buff, "b.key1, a.f8, a.money, a.date, a.char61, b.cnt ");
	STcat (sql_buff, "from data3 a, data5 b, data7 c ");
	STcat (sql_buff, "where a.money < c.money and b.key1 = c.key1");
	STprintf (sql_stmt, sql_buff, viewname);   /* create view viewname */
	
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}
	exec sql commit;

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select key1, money, date ");
	STcat (sql_buff, "from %s where cnt = %d");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select key1, money, date from viewname where cnt = c */

	exec sql prepare s33 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s33 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql describe s33 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s33 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql declare c33 cursor for s33;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c33 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql open c33;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c33 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   len = sqv->sqllen;

	   /* account for describe setting sqllen to 0 for money and date */

	   if ((len == 0) && (abs(sqv->sqltype) == DATE_TYPE))
		len = DATE_LEN;
	   else if ((len == 0) && (abs(sqv->sqltype) == MONEY_TYPE))
		len = MONEY_LEN;

	   if (traceflag > 4)
           {
		SIprintf("len: %d\n", len);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1, len);
	   if(!sqv->sqldata)
	   {
		SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
		SIflush(stdout);
		goto close_cursor; 
	   }
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c33 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c33 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
	if (traceflag > 2)
	{
		SIprintf("Going to free ((char *)sqv->sqldata); \n");
		SIflush(stdout);
	}

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
	if (traceflag > 2)
	{
		SIprintf("Going to close c33); \n");
		SIflush(stdout);
	}

	exec sql close c33;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c33 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

drop_view:
	if (traceflag > 2)
	{
		SIprintf("Going to drop view %s \n", viewname);
		SIflush(stdout);
	}

	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "drop view %s", viewname);
	exec sql execute immediate :sql_stmt;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
	if (traceflag > 2)
	{
		SIprintf("Going to free ((char *)sqlda) \n");
		SIflush(stdout);
	}

	free ((char *)sqlda);   /* free sqlda memory */
	
	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, sqlcode is set to that error	*/
	/* sqlcode was getting reset to 0 by a successful drop_view	*/
	/* Note: the last error encountered will be returned		*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst34(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s34 statement;

	int		i;	/* index into sqlvar */
	int		len;	/* calloc length */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view34_1_1", viewname);

	/* Make a unique viewname by appending the slave and count numbers */
	viewname[7] = (s % 10) + '0';
	viewname[9] = (c % 10) + '0';

	STcopy ("", sql_buff);   /* build SQL statement in buffer */
	STcat (sql_buff, "create view %s as ");
	STcat (sql_buff, "select i4, f4, money, date from data4 ");
	STcat (sql_buff, "where date in (select date from data9 ");
	STcat (sql_buff, "where key1 in (select key1 from data1)) ");
	STprintf (sql_stmt, sql_buff, viewname);   /* create view viewname */
	
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}
	exec sql commit;

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select f4, money, date ");
	STcat (sql_buff, "from %s where i4 >= %d + 500");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select f4, money, date from viewname where i4 >= c + 500 */

	exec sql prepare s34 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s34 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql describe s34 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s34 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql declare c34 cursor for s34;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c34 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
		}

	exec sql open c34;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c34 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   len = sqv->sqllen;

	   /* account for describe setting sqllen to 0 for money and date */

	   if ((len == 0) && (abs(sqv->sqltype) == DATE_TYPE))
		len = DATE_LEN;
	   else if ((len == 0) && (abs(sqv->sqltype) == MONEY_TYPE))
		len = MONEY_LEN;

	   if (traceflag > 4)
           {
		SIprintf("len: %d\n", len);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1, len);

           if(!sqv->sqldata)
           {
                SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                goto close_cursor;
           }
	}

	while (sqlca.sqlcode == 0)
	{
		exec sql fetch c34 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c34 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c32); \n");
                SIflush(stdout);
        }

	exec sql close c34;
	
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c34 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

drop_view:
        if (traceflag > 2)
        {
                SIprintf("Going to drop view %s \n", viewname);
                SIflush(stdout);
        }

	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "drop view %s", viewname);
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

        free ((char *)sqlda);   /* free sqlda memory */

	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, sqlcode is set to that error	*/
	/* sqlcode was getting reset to 0 by a successful drop_view	*/
	/* Note: the last error encountered will be returned		*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }
	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}


tst35(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s35 statement;

	int		i;	/* index into sqlvar */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/* Allocate memory for sqlda, enough for 1 column only */

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (1 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 1;   /* number of columns */

	STcopy ("view35_1_1", viewname);

	/* Make a unique viewname by appending the slave and count numbers */
	viewname[7] = (s % 10) + '0';
	viewname[9] = (c % 10) + '0';

	STcopy ("", sql_buff);   /* build SQL statement in buffer */
	STcat (sql_buff, "create view %s as ");
	STcat (sql_buff, "select key1, cnt from data1 union ");
	STcat (sql_buff, "select key1, cnt from data5");
	STprintf (sql_stmt, sql_buff, viewname);   /* create view viewname */
	
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}
	exec sql commit;

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "select key1 from %s where cnt > %d * 2", 
				viewname, c);

	/* select key1 from viewname where cnt > c x 2 */

	exec sql prepare s35 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s35 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql describe s35 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s35 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql declare c35 cursor for s35;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c35 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	exec sql open c35;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c35 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto drop_view;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];

	   if (traceflag > 4)
           {
		SIprintf("sqv->sqllen: %d\n", sqv->sqllen);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1,sqv->sqllen);
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c35 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c35 using descriptor - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c33); \n");
                SIflush(stdout);
        }

	exec sql close c35;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c35 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

drop_view:
        if (traceflag > 2)
        {
                SIprintf("Going to drop view %s \n", viewname);
                SIflush(stdout);
        }

	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "drop view %s", viewname);
	exec sql execute immediate :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("execute immediate - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

	free ((char *)sqlda);   /* free sqlda memory */

	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, sqlcode is set to that error	*/
	/* sqlcode was getting reset to 0 by a successful drop_view	*/
	/* Note: the last error encountered will be returned		*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst36(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		int	i, d7key1;   /* variables for update of data7 */
	exec sql end declare section;

	/* in case of undetectable distributed deadlock */
	exec sql commit;
	exec sql set lockmode session where timeout = 900; 

	exec sql declare c36 cursor for
		select key1 from data7
		where key1 >= 300 and key1 < 400
		for update of money;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c36 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql update width500
		set f4 = f4 * 3.142, txt58 = 'tst36_txt58_updated_value'
		where i2 > 40 and i2 <= 90;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update width500 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql update data4
		set money = money(999), date = date('15-dec-90'),
		char61a = 'tst36_char61_updated_value'
		where i4 > 5 and i4 <= 20;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update data4 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql update data3
		set money = money(:c + 100), f8 = f8 + (:c * 1000)
		where i4 > 200 and i4 <= 250;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update data3 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql update data9
		set date = date('4-mar-1991')
		where key1 > 350 and key1 <= 400;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("update data9 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql open c36;   /* cursor for data7 */
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c36 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql whenever not found goto closec36;  /* no more data -- exit */

	for (i = 0;; i++)
	{
		exec sql fetch c36 into :d7key1;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c36 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql update data7
			set money = money(:i * 1000)
			where current of c36;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("update data7 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
	}
closec36:
	exec sql whenever not found continue;   /* resume normal processing */

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst37(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
	exec sql begin declare section;
		int	i;
	exec sql end declare section;

		/* in case of undetectable distributed deadlock */
		exec sql commit;
		exec sql set lockmode session where timeout = 900; 

		exec sql insert into data2 (key1, slave, rout)
			values (:c + :i, :s, :i);
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data2 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql delete from data5
			where cnt = :c or key1 = :i;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("delete from data5 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}


		exec sql update data8
			set charv = 'tst37_charv_updated_value'
			where key1 = :i;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("update data8 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}


		exec sql delete from width500
			where i2 = :i + 500;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("delete from width500 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql update data6
			set cnt = (cnt + :i) * 100
			where key1 = :i + 40;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("update data6 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql commit;
		exec sql set lockmode session where timeout = SESSION;
}


tst38(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
	exec sql begin declare section;
		char	d3money[32], d3date[24], d9date[24];
		int	done1, done2, done3, d6cnt, d9key1;
		double	d6floatv;
	exec sql end declare section;

	/* delcare 3 cursors for updates of data3, data6, data9 */

	/* in case of undetectable distributed deadlock */
	exec sql commit;
	exec sql set lockmode session where timeout = 900; 

	exec sql declare c138 cursor for
		select char(money), char(date)
		from data3
		where i1 = 61
		for direct update of money, date;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c138 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql declare c238 cursor for
		select floatv, cnt
		from data6
		where key1 > 5 and key1 <= 25
		for direct update of floatv, cnt;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c238 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql declare c338 cursor for
		select key1, char(date)
		from data9
		where key1 > :s * 100 and key1 <= :s * 100 + 50
		for direct update of date;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c338 cursor - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql commit;   /* end last transaction */

	exec sql open c138;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c138 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql open c238;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c238 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql open c338;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c338 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	/* loop through all tables until data exhausted */

	do {
		exec sql fetch c138 into :d3money, :d3date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c138 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres(:done1 = ENDQUERY);
		if (!done1 && !(sqlca.sqlcode < 0))
		{
			exec sql update data3
				set money = money(56789),
				date = date('15-mar-1991')
				where current of c138;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data3 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}

		exec sql fetch c238 into :d6floatv, :d6cnt;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c238 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres(:done2 = ENDQUERY);
		if (!done2 && !(sqlca.sqlcode < 0))
		{
			exec sql update data6
				set floatv = floatv + :c * 5.5,
				cnt = cnt + :c + 100
				where current of c238;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data6 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}

		exec sql fetch c338 into :d9key1, :d9date;
		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
        		{
				SIprintf("fetch c338 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres(:done3 = ENDQUERY);
		if (!done3 && !(sqlca.sqlcode < 0))
		{
			exec sql update data9
				set date = date('30-apr-1991')
				where current of c338;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
        			{
					SIprintf("update data9 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}

	} while (!(done1 && done2 && done3));

	exec sql close c138;
	exec sql close c238;
	exec sql close c338;

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst39(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
		/* in case of undetectable distributed deadlock */
		exec sql commit;
		exec sql set lockmode session where timeout = 900; 

		exec sql update width500
			set f8 = 12.416 * :c
			where i2 > 100 and i2 <= 105;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("update width500 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data2 (key1, slave, rout)
			values (:c + 2000, :s + 2000, 39);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data2 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql delete from data4
			where money <= money(1000);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("delete from data4 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data7 (key1, money)
			values (:c + 1000, money(1000 * :c));

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data7 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql update data8
			set charv = 'tst39_charv_updated_value'
			where key1 > 100 and key1 <= 150;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("update data8 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

	exec sql commit;   /* commit all tables */
	exec sql set lockmode session where timeout = SESSION;
}

tst40(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
		/* in case of undetectable distributed deadlock */
		exec sql commit;
		exec sql set lockmode session where timeout = 900; 

		exec sql insert into width500
			(i2,f4,f8,dollar,day,c6,txt58,txt98one,txt98two,txt98three, txt98four)
			values (:c - 100, :c * 8.125, :c * 16.25, :c + 20000,
				'1/1/1992', 'tst040',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
				);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into width500 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data2 (key1, slave, rout)
			values (:c * 100, :s + 900, 40);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data2 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data5 (key1, cnt)
			values (:c * 500, :c + :s);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data5 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data7 (key1, money)
			values (:c * 1200, :c * 9.5);

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data7 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql insert into data8 (key1, charv)
			values (:c * 700, 'tst40_charv_inserted_value');

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("insert into data8 - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql commit;
		exec sql set lockmode session where timeout = SESSION;
}

tst41(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
	exec sql commit;   /* end previous transaction */

		exec sql insert into width500
			(i2,f4,f8,dollar,day,c6,txt58,txt98one,txt98two,txt98three, txt98four)
			values (:c - 100, :c * 8.125, :c * 16.25, :c + 20000,
				'1/1/1992', 'tst040',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
				'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
				);

		exec sql insert into data2 (key1, slave, rout)
			values (:c * 100, :s + 900, 40);

		exec sql insert into data5 (key1, cnt)
			values (:c * 500, :c + :s);

		exec sql insert into data7 (key1, money)
			values (:c * 1200, :c * 9.5);

		exec sql insert into data8 (key1, charv)
			values (:c * 700, 'tst40_charv_inserted_value');

	exec sql commit;
}

tst42(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst43(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst44(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst45(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst46(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst47(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst48(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst49(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}


tst50(s,c)
int	s;
int	c;
{
	exec sql begin declare section;
		/* Declarations for data5 table */
		int	d5key1;
		int	d5cnt;
		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;


	/*  direct update using join */
	exec sql declare c6b cursor
			for select key1, cnt
			from data5
			where key1 > 50 and key1 < 320
			for direct update of key1, cnt;

/*
**	SIprintf( "\nDirect update where 30<data5.key1<300 \n\0" );
**	SIflush(stdout);
*/

	done = 0;

	exec sql open c6b;

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c6b - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c6b into :d5key1, :d5cnt;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c6b into - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}
		exec sql inquire_ingres (:done = ENDQUERY);

		if (!done && !(sqlca.sqlcode < 0))
		{
/*			SIprintf("Direct   --> key1= %d, cnt=%d\n", d5key1, d5cnt);
**			SIflush(stdout);
*/
			exec sql update data5
					set key1 = key1 - 20
					where current of c6b;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
				{
					SIprintf("update data5 - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}
		}
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

} /* End of tst50() */



tst51(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data1_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

}


tst52(s,c)
int	s;
int	c;

/*
**		reverses the action of tst26  
**
**		jds 11/14/91
*/

{
	exec sql begin declare section;
		/* Declarations for data5 table */
		int	d5key1;
		int	d5cnt;

		/* Declarations for data8 table */
		int	d8key1;
		char 	d8char[66];

		/* Useful vars */
		int	done;		/* to signal end of cursor */
	exec sql end declare section;

	exec sql declare c9b cursor
			for select a.key1, a.cnt
			from data5 a, data8 d
			where a.key1 = d.key1;

	exec sql declare c10b cursor
			for select key1, charv
			from data8
			where key1 > 20
			and key1 < 200
			for direct update of key1, charv;


        if (traceflag > 2)
        {
	SIprintf( "\nJoin data5 and data8 and replace data8 \n\0" );
	SIflush(stdout);
	}

	done = 0;

	exec sql open c9b for readonly;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c9b for readonly - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}

	exec sql open c10b;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
        	{
			SIprintf("open c10b - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		return;
	}


	while (!done && !(sqlca.sqlcode < 0))
	{
		exec sql fetch c9b into :d5key1, :d5cnt;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c9b - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			return;
		}

		exec sql inquire_ingres (:done = ENDQUERY);
		if (!done && !(sqlca.sqlcode < 0))
		{
/*
**		SIprintf("c9: %d, %d \n", d5key1, d5cnt);
**		SIflush (stdout);
*/
			exec sql fetch c10b into :d8key1, :d8char;

			if (sqlca.sqlcode < 0)
			{
				if (traceflag)
				{
					SIprintf("fetch c10b - FAILED: %d\n", sqlca.sqlcode);
					SIflush(stdout);
				}
				return;
			}

			exec sql inquire_ingres (:done = ENDQUERY);
			if (!done && !(sqlca.sqlcode < 0))
			{
/*				SIprintf("c10: %d \n", d8key1);
**				SIflush (stdout);
*/
				exec sql update data8
					set key1 = key1/2,
					charv = 'Test 52 restore '
					where current of c10b;

				if (sqlca.sqlcode < 0)
				{
					if (traceflag)
					{
						SIprintf("fetch c10b - FAILED: %d\n", sqlca.sqlcode);
						SIflush(stdout);
					}
					return;
				}
			}
		}
	}

	if (sqlca.sqlcode < 0)
	{
		return;
	}

	exec sql commit;

}	/* End of tst52() */



tst53(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data3_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

}


tst54(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data4_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

}


tst55(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data5_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst56(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data6_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst57(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data7_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst58(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data8_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst59(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='data9_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst60(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d1d2_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst61(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d1d3_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst62(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d1d3d4_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst63(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d1d2d3d4d5_report',
		file='NLA0:',
		param='(Min_key=10, Max_key=20)',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(1200000);
	/*
	**	wait about 20 minutes
	*/
}


tst64(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d5d4d1_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst65(s,c)

/*
**	jds	 4-NOV-1991 16:26:35.30
**
**	add new test routine
**
**	3 table join in each of 5 ldbs, then
**	UNION ALL to unite them.
**
**	uses SELECT begin/end loop
**
**	has a 1/2 second sleep between each retrieve of the loop,
**	to emulate a user scanning data.
**
**	THIS is "star-ish"...
**
**	requires that data_4, data_8, data_9 all be registered from
**	each of MTSLDB1-5, with an under_score, dbnumber suffix, as
**	" data8_2 " - table data8 from MTSLDB2.
*/

exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;

{
	exec sql begin declare section;

		/*	for data4 table */
		char    d4money[32];

		/* Declarations for data8 table */
		int	d8key1;
		char 	d8char[66];

		/* Declarations for data9 table */
		int	d9key1;
		char 	d9date[26];

		/* Useful vars */
		int	sleeptime;	/* introduce a variable wait... */

	exec sql end declare section;



	sleeptime = (s * 1000);

	PCsleep(sleeptime);

/*
**	sleep 1 second per slave_number
*/

	exec sql select d8a.key1, d8a.charv, d9.date, d4.money 
		into :d8key1, :d8char, :d9date, :d4money
		from data8_1 d8a, data9_1 d9, data4_1 d4
		where d4.i2 = d8a.key1 and d9.key1 = d8a.key1
		union all
		select d8.key1, d8.charv, d9.date, d4.money 
		from data8_2 d8, data9_2 d9, data4_2 d4
		where d4.i2 = d8.key1 and d9.key1 = d8.key1
		union all
		select d8.key1, d8.charv, d9.date, d4.money 
		from data8_3 d8, data9_3 d9, data4_3 d4
		where d4.i2 = d8.key1 and d9.key1 = d8.key1
		union all
		select d8.key1, d8.charv, d9.date, d4.money 
		from data8_4 d8, data9_4 d9, data4_4 d4
		where d4.i2 = d8.key1 and d9.key1 = d8.key1
		union all
		select d8.key1, d8.charv, d9.date, d4.money 
		from data8_5 d8, data9_5 d9, data4_5 d4
		where d4.i2 = d8.key1 and d9.key1 = d8.key1
		order by d8a.key1
		;

	exec sql begin;

		PCsleep(300);	/* a pause to refresh */

	exec sql end;

/*
**	should return 1000 rows
*/


	if (sqlca.sqlcode < 0)
	{
		SIprintf ("select, union all - FAILED: %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

}


tst66(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{

exec sql call report (database='mtsddb1/star',
		report='d1d3d4_report',
		file='NLA0:',
		silent = '');

	if (sqlca.sqlcode < 0)
	{
		SIprintf ("Error from report call %d",sqlca.sqlcode); 
		SIflush(stdout); 

		return;
	}
	exec sql commit;

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/

	PCsleep(120000);
	/*
	**	wait about 2 minutes
	*/
}


tst67(s,c)
exec sql begin declare section;
	int	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	int num;
exec sql end declare section;

	exec sql select key1
		into :num
		from data1
		where cnt = :c;
	exec sql begin;
/*		SIprintf ("Slave %d retrieving %d\n", s, num); */

	exec sql end;

	if (sqlca.sqlcode < 0)
	{
		return;
	}
	exec sql commit;
}

tst68(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s68 statement;

	int		i;	/* index into sqlvar */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view68", viewname);

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select key1, floatv, charv ");
	STcat (sql_buff, "from %s where cnt = %d");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select key1, floatv, charv from viewname where cnt = c */

	exec sql prepare s68 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s68 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql describe s68 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s68 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql declare c68 cursor for s68;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c68 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql open c68;

	if (traceflag > 4)
	{
		SIprintf("open c68 - %s : %d\n", sql_stmt, sqlca.sqlcode);
		SIflush(stdout);
	}

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c68 - %s FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   if (traceflag > 4)
           {
		SIprintf("sqv->sqllen: %d\n", sqv->sqllen);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1,sqv->sqllen);

           if(!sqv->sqldata)
           {
                SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                goto close_cursor;
           }
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c68 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c68 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c68); \n");
                SIflush(stdout);
        }

	exec sql close c68;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c68 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

	free ((char *)sqlda);   /* free sqlda memory */


	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, tempcode is set to that error	*/
	/* At the end of the test, sqlcode is reset to tempcode		*/
	/* in case sqlcode may have been written over			*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }
	
	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}


tst69(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s69 statement;

	int		i;	/* index into sqlvar */
	int		len;	/* calloc length */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;
	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** SQLDA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view69", viewname);

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select key1, money, date ");
	STcat (sql_buff, "from %s where cnt = %d");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select key1, money, date from viewname where cnt = c */

	exec sql prepare s69 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s69 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql describe s69 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s69 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql declare c69 cursor for s69;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c69 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql open c69;

	if (traceflag > 4)
	{
		SIprintf("open c69 - %s : %d\n", sql_stmt, sqlca.sqlcode);
		SIflush(stdout);
	}

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c69 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   len = sqv->sqllen;

	   /* account for describe setting sqllen to 0 for money and date */

	   if ((len == 0) && (abs(sqv->sqltype) == DATE_TYPE))
		len = DATE_LEN;
	   else if ((len == 0) && (abs(sqv->sqltype) == MONEY_TYPE))
		len = MONEY_LEN;

	   if (traceflag > 4)
           {
		SIprintf("len: %d\n", len);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1, len);
	   if(!sqv->sqldata)
	   {
		SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
		SIflush(stdout);
		goto close_cursor; 
	   }
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c69 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c69 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
	if (traceflag > 2)
	{
		SIprintf("Going to free ((char *)sqv->sqldata); \n");
		SIflush(stdout);
	}

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
	if (traceflag > 2)
	{
		SIprintf("Going to close c69); \n");
		SIflush(stdout);
	}

	exec sql close c69;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c69 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
	if (traceflag > 2)
	{
		SIprintf("Going to free ((char *)sqlda) \n");
		SIflush(stdout);
	}

	free ((char *)sqlda);   /* free sqlda memory */
	
	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, tempcode is set to that error	*/
	/* At the end of the test, sqlcode is reset to tempcode		*/
	/* in case sqlcode may have been written over			*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst70(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s70 statement;

	int		i;	/* index into sqlvar */
	int		len;	/* calloc length */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/*
	** Allocate memory for sqlda, enough for 3 columns
	*/

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (3 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 3;   /* number of columns */

	STcopy ("view70", viewname);

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STcat (sql_buff, "select f4, money, date ");
	STcat (sql_buff, "from %s where i4 >= %d + 500");
	STprintf (sql_stmt, sql_buff, viewname, c);

	/* select f4, money, date from viewname where i4 >= c + 500 */

	exec sql prepare s70 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s70 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql describe s70 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s70 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql declare c70 cursor for s70;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c70 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
		}

	exec sql open c70;

	if (traceflag > 4)
	{
		SIprintf("open c70 - %s : %d\n", sql_stmt, sqlca.sqlcode);
		SIflush(stdout);
	}

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c70 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   len = sqv->sqllen;

	   /* account for describe setting sqllen to 0 for money and date */

	   if ((len == 0) && (abs(sqv->sqltype) == DATE_TYPE))
		len = DATE_LEN;
	   else if ((len == 0) && (abs(sqv->sqltype) == MONEY_TYPE))
		len = MONEY_LEN;

	   if (traceflag > 4)
           {
		SIprintf("len: %d\n", len);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1, len);

           if(!sqv->sqldata)
           {
                SIprintf("*** SQLDATA MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                goto close_cursor;
           }
	}

	while (sqlca.sqlcode == 0)
	{
		exec sql fetch c70 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c70 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c32); \n");
                SIflush(stdout);
        }

	exec sql close c70;
	
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c70 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

        free ((char *)sqlda);   /* free sqlda memory */

	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, tempcode is set to that error	*/
	/* At the end of the test, sqlcode is reset to tempcode		*/
	/* in case sqlcode may have been written over			*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }
	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}


tst71(s,c)
exec sql begin declare section;
	i4	s;
	int	c;
exec sql end declare section;
{
exec sql begin declare section;
	char		sql_stmt[300];
exec sql end declare section;

exec sql declare s71 statement;

	int		i;	/* index into sqlvar */
	IISQLVAR	*sqv;	/* pointer to sqlvar */
	IISQLDA		*sqlda = (IISQLDA *)0;
	char		viewname[10];
	char		sql_buff[300];
	i4		tempcode = 0;	/* retain sqlcode for error recording */

	exec sql commit;
	exec sql set lockmode session where timeout = 900;

	/* Allocate memory for sqlda, enough for 1 column only */

	sqlda = (IISQLDA *)
		calloc(1, IISQDA_HEAD_SIZE + (1 * IISQDA_VAR_SIZE));

        if (sqlda == (IISQLDA *)0)
        {
                SIprintf("*** MEMORY ALLOCATION ERROR ***\n");
                SIflush(stdout);
                return;
        }

	sqlda->sqln = 1;   /* number of columns */

	STcopy ("view71", viewname);

	/*
	** Execute a query that uses the view
	*/

	STcopy ("", sql_buff);
	STcopy ("", sql_stmt);
	STprintf (sql_stmt, "select key1 from %s where cnt > %d * 2", 
				viewname, c);

	/* select key1 from viewname where cnt > c x 2 */

	exec sql prepare s71 from :sql_stmt;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("prepare s71 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql describe s71 into :sqlda;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("describe s71 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql declare c71 cursor for s71;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("declare c71 - FAILED: %d\n", sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	exec sql open c71;

	if (traceflag > 4)
	{
		SIprintf("open c71 - %s : %d\n", sql_stmt, sqlca.sqlcode);
		SIflush(stdout);
	}

	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("open c71 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
		goto free_sqlda;
	}

	for (i=0; i < sqlda->sqld; i++)   /* allocate result areas */
	{
	   sqv = &sqlda->sqlvar[i];

	   if (traceflag > 4)
           {
		SIprintf("sqv->sqllen: %d\n", sqv->sqllen);
		SIflush(stdout);
	   }

	   sqv->sqldata = (char *) calloc(1,sqv->sqllen);
	}

	while (sqlca.sqlcode == 0)
	{
	   	exec sql fetch c71 using descriptor :sqlda;

		if (sqlca.sqlcode < 0)
		{
			if (traceflag)
			{
				SIprintf("fetch c71 using descriptor - FAILED: %d\n", sqlca.sqlcode);
				SIflush(stdout);
			}
			tempcode = sqlca.sqlcode;
			goto free_sqldata;
		}
	}

free_sqldata:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqv->sqldata); \n");
                SIflush(stdout);
        }

	for (i=0; i < sqlda->sqld; i++)   /* free result areas */
	{
	   sqv = &sqlda->sqlvar[i];
	   free ((char *)sqv->sqldata);
	}

close_cursor:
        if (traceflag > 2)
        {
                SIprintf("Going to close c33); \n");
                SIflush(stdout);
        }

	exec sql close c71;
	if (sqlca.sqlcode < 0)
	{
		if (traceflag)
		{
			SIprintf("close c71 - %s - FAILED: %d\n", sql_stmt, sqlca.sqlcode);
			SIflush(stdout);
		}
		tempcode = sqlca.sqlcode;
	}

free_sqlda:
        if (traceflag > 2)
        {
                SIprintf("Going to free ((char *)sqlda) \n");
                SIflush(stdout);
        }

	free ((char *)sqlda);   /* free sqlda memory */

	/* This is an attempt to simulate the normal error handling	*/
	/* If an error is encountered, tempcode is set to that error	*/
	/* At the end of the test, sqlcode is reset to tempcode		*/
	/* in case sqlcode may have been written over			*/

        if (tempcode < 0)
        {
		if (traceflag)
		{
			SIprintf("error - sqlcode = tempcode: %d\n", tempcode);
			SIflush(stdout);
		}
                sqlca.sqlcode = tempcode;
                return;
        }

	exec sql commit;
	exec sql set lockmode session where timeout = SESSION;
}

tst72(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst73(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst74(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst75(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst76(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst77(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst78(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst79(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst80(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst81(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst82(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst83(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst84(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst85(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst86(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst87(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst88(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst89(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst90(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst91(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst92(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst93(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst94(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst95(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst96(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst97(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst98(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{
        exec sql commit;   /* end previous transaction */

}

tst99(s,c)
exec sql begin declare section;
        int     s;
        int     c;
exec sql end declare section;
{

exec sql begin declare section;
        i4 running_kids;      /* how many slaves are still running */
exec sql end declare section;


 exec sql commit;   /* end previous transaction */


/*
**
**	Track the performance of these tests every twenty
**	minutes (or as close as the operating system will
**	track that time).
**
**	The tests which use tst29 append a row to the history
**	table with each successful transaction.  This routine
**	uses tables	history_count,
**			test_temp,
**			test_perf
**
**	history_count is a kluge to get the count into a
**	variable so I can use it.
**
**	test_temp retains information about the start of the
**	test and the previous number of transactions and
**	the total test minutes.
**
**	test_perf collects the performance data in units of
**	transactions per minute, both throughout the test so
**	far, and in the latest sampling period.
**
**	All 3 tables are created and initialized in MTSLDB3
**	with the initialization step of the test.
**
**
**	The test checks to see whether slaves_up still has
**	any rows in it (copied from mtscoord.sc) before it
**	tries the update/insert cycle.  If only 1, that's
**	THIS slave, and it will exit.
**
**	The routine uses DIRECT CONNECT, so it is suitable for
**	STAR only;  it also uses mtsnode3, so that must be defined
**	and set up correctly.
**
**
**	Why do it via direct connect?
**
**	(1) the STAR database is already open, and a session is established
**	to MTSLDB for the administrative tables; I need that to find out
**	when this test should exit;
**
**	(2) DIRECT CONNECT minimizes the impact on the STAR server at
**	the cost of an extra STAR session.
**
**		jds	2/3/92
**
*/


exec sql set lockmode on slaves_up where readlock=nolock;

/*
**	let me read the slaves_up table without messing
**	up the coordinator or the other slaves.
*/

do
 {


 exec sql direct connect with database=mtsldb3, node= mtsnode3;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

 exec sql set lockmode session where readlock = nolock;

 exec sql set autocommit on;


/*
**
**	a kluge to get this count into the temp area
**
*/

 exec sql delete from history_count;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

 exec sql insert into history_count (hcount) select count(acc_number) from history;


	if (sqlca.sqlcode < 0)
	{
		return;
	}


/*
**
**	Load the one-tuple table with
**
**		curmin - minutes into the test
**		curtrans - total successful transactions
**
*/

 exec sql update test_temp set 
	curmin = interval('minutes', (date('now') - starttime));

	if (sqlca.sqlcode < 0)
	{
		return;
	}

 exec sql update test_temp from history_count
	set curtrans = hcount;


	if (sqlca.sqlcode < 0)
	{
		return;
	}

/*
**
**	Calculate and store the changes from the last sample in
**	the one-tuple table.
**
**		elapmin - minutes since last sample
**		elaptran - number of transactions since last sample
*/

 exec sql update test_temp set
	elapmin = curmin - premin,
	elaptrans = curtrans - pretrans;


	if (sqlca.sqlcode < 0)
	{
		return;
	}

/*
**
**	Once those numers are there, calculate and store
**	the transaction rate
**
**		currate - number of new transactions divided by
**			number of minutes this sample.
*/

 exec sql update test_temp set
	currate = elaptrans / elapmin;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

/*
**
**	Save the results in the test history table
**
*/

 exec sql insert into test_perf
	 (sample_time, test_minutes, total_trans, test_rate, sample_min, sample_trans, sample_rate)
 	select
	'now',
	curmin, curtrans, curtrans / curmin, elapmin, elaptrans, currate
	from test_temp;

	if (sqlca.sqlcode < 0)
	{
		return;
	}

/*
**
**	Move the current minutes and transaction count to the 
**	"pre" fields to use next iteration
**
*/

 exec sql update test_temp set
	premin = curmin,
	pretrans = curtrans;

	if (sqlca.sqlcode < 0)
	{
		return;
	}


 exec sql commit;

	if (sqlca.sqlcode < 0)
	{
		return;
	}
	
 exec sql direct disconnect;


 exec sql select count(*)
      into :running_kids from slaves_up;


 if (running_kids > 1)
      PCsleep( 1200000 );

/*
**	sleep 20 minutes
*/


 } while (running_kids > 1);

}	/* end of routine 99 */

