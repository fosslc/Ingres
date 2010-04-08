/*
**  ROLL_CREATE	    - run fast commit test
**
**  This program executes the fast commit tests.
**  It will execute only one test at a time, as the server is crashed at the
**  end of each test.
**
**  It will request an installation lock using the DMF trace point DM1420
**  before crashing in order to sync up with the test driver.
**
**  Before requesting the DM1420 trace point, it will create a copy file
**  in the ROLL_DATA directory identifying itself as ready to crash.
**
**  Test 0 indicates to run all the tests without causing crash in order
**  to generate a canon file.
**
**  Inputs:
**	dbname	    - database to use
**	test_num    - fast commit test to run (0-28)
**	base_num    - test driver identifier
**	sess_num    - session this is (multiple sessions may be running)
**
**      History:
**
**      DD-MMM-YYYY     Unknown
**              Created Multi-Server Fast Commit test.
**      01-Mar-1991     Jeromef
**              Modified Multi-Server Fast Commit test to include ckpdb and
**              rollforwarddb.
**      21-Nov-1991     Jeromef
**              Add rollforward/msfc test to piccolo library
**      20-Jan-1994 (huffman)
**              Correct include files (xxcl.h) should look at the
**              common version (xx.h) for compiling.
**      17-feb-1994 (donj)
**              Get rid of "trigraph replaced warning"
**      22-sep-2000 (mcgem01)
**              More nat and longnat to i4 changes.
**
**
NEEDLIBS = LIBINGRES
**
*/

#include        <compat.h>
#include        <tr.h>
#include        <pc.h>
#include        <si.h>
#include        <st.h>

#define		ARRAY_SIZE	100

typedef struct roll_struct
	{
		int	(*func)();
	} ROLL_LIST;

int	roll_test0();
int	roll_test1();
int	roll_test2();
int	roll_test3();
int	roll_test4();
int	roll_test5();
int	roll_test6();
int	roll_test7();
int	roll_test8();
int	roll_test9();
int	roll_test10();
int	roll_test11();
int	roll_test12();
int	roll_test13();
int	roll_test14();
int	roll_test15();
int	roll_test16();
int	roll_test17();
int	roll_test18();
int	roll_test19();
int	roll_test20();
int	roll_test21();
int	roll_test22();
int	roll_test23();
int	roll_test24();
int	roll_test25();
int	roll_test26();
int	roll_test27();
int	roll_test28();


static ROLL_LIST roll_array[ARRAY_SIZE] =
		{
			roll_test0,
			roll_test1,
			roll_test2,
			roll_test3,
			roll_test4,
			roll_test5,
			roll_test6,
			roll_test7,
			roll_test8,
			roll_test9,
			roll_test10,
			roll_test11,
			roll_test12,
			roll_test13,
			roll_test14,
			roll_test15,
			roll_test16,
			roll_test17,
			roll_test18,
			roll_test19,
			roll_test20,
			roll_test21,
			roll_test22,
			roll_test23,
			roll_test24,
			roll_test25,
			roll_test26,
			roll_test27,
			roll_test28
		};

main(argc, argv)
int	    argc;
char	    *argv[];
##{
##  char	*dbname;
##  int		base_num = -1;
##  int		sess_num = -1;
##  int		test_num = -1;
##  int		start_num = -1;
##  char	copy_file[40];
##  char	table_name[32];
    CL_SYS_ERR	syserr;
int	i;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &syserr);
#endif
#ifdef UNIX
    TRset_file(TR_T_OPEN, "stdio", 5, &syserr);
#endif

    if (argc < 5)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    dbname = argv[1];
    CVal(argv[2], &test_num);
    CVal(argv[3], &base_num);
    CVal(argv[4], &sess_num);
    if (argc == 6)
    {
	CVal(argv[5], &start_num);
    }

    if (base_num <= 0 || test_num <= 0 || test_num > 28 || sess_num <= 0
	 || start_num > test_num)	
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    SIprintf ("start num %d, test_num %d, argc %d \n ", 
	start_num, test_num, argc);
    SIflush (stdout);

    if (start_num <= 0 || start_num > 29)
    {
	SIprintf ("changing start_num \n ");
	SIflush (stdout);
	start_num = 1;
    }

##  ingres dbname

##  set printqry

    SIprintf ("database name : %s, %d, %d, %d\n", dbname, test_num, base_num,
	sess_num);
    SIflush (stdout);

    for (i = start_num; i <= test_num; i++)
    {

	SIprintf ("I'm in loop %d\n ", i);
	SIflush (stdout);

	(*roll_array[i].func)(base_num, sess_num);
    }

##  exit
    PCexit(0);
##}

roll_usage()
{
    TRdisplay("USAGE: ROLL_CREATE dbname test base session [start]\n");
    TRdisplay("    dbname - database.\n");
    TRdisplay("    test   - maximum test number to executed.\n");
    TRdisplay("    base   - base # of driver.\n");
    TRdisplay("    session - which slave this is.\n");
    TRdisplay("    start - optional: test number to start with (default is 1)\n");
    TRdisplay("\n");
}

roll_test0(base_num, sess_num)
int	base_num;
int	sess_num;
##{

    SIprintf ("dummy function should never be used\n");
    SIflush (stdout);

##}

roll_test1(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	rtab_name[16];
##  char	rtab2_name[16];
##  char	rtab3_name[16];
##  char	ctab1_name[16];
##  char	copy_file[40];

    /*
    ** Fast1
    */
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);
    STprintf(rtab2_name, "rtab2_%d_%d", base_num, sess_num);
    STprintf(ctab1_name, "ctab1_%d_%d", base_num, sess_num);

    SIprintf ("roll_test1: %s, %s, %s\n", rtab_name, rtab2_name,
	ctab1_name);
    SIflush (stdout);

##  destroy rtab_name
##  create rtab_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy rtab2_name
##  create rtab2_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy ctab1_name
##  create ctab1_name (a=i4,b=c20) with journaling
##}

roll_test2(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	btab1_name[16];
##  char	btab2_name[16];
##  char	btab3_name[16];
##  char	rtab_name[16];

    /*
    ** Fast2
    */

    STprintf(btab1_name, "btab1_%d_%d", base_num, sess_num);
    STprintf(btab2_name, "btab2_%d_%d", base_num, sess_num);
    STprintf(btab3_name, "btab3_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

    SIprintf ("roll_test2: %s, %s, %s, %s\n", btab1_name, btab2_name,
	btab3_name, rtab_name);
    SIflush (stdout);

##  destroy btab1_name
##  create btab1_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy btab2_name
##  create btab2_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy btab3_name
##  create btab3_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test3(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	btab9_name[16];
##  char	btab10_name[16];
##  char	rtab_name[16];

    /*
    ** Fast3
    */
    STprintf(btab9_name, "btab9_%d_%d", base_num, sess_num);
    STprintf(btab10_name, "btab10_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

    SIprintf ("roll_test3: %s, %s, %s\n", btab9_name, btab10_name,
	rtab_name);
    SIflush (stdout);

##  destroy btab9_name
##  create btab9_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy btab10_name
##  create btab10_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test4(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	htab1_name[16];
##  char	htab2_name[16];
##  char	htab3_name[16];
##  char	htab9_name[16];
##  char	htab10_name[16];
##  char	rtab_name[16];

    /*
    ** Fast4
    */
    STprintf(htab1_name, "htab1_%d_%d", base_num, sess_num);
    STprintf(htab2_name, "htab2_%d_%d", base_num, sess_num);
    STprintf(htab3_name, "htab3_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);
    STprintf(htab9_name, "htab9_%d_%d", base_num, sess_num);
    STprintf(htab10_name, "htab10_%d_%d", base_num, sess_num);

    SIprintf ("roll_test4: %s, %s, %s, %s, %s, %s\n", htab1_name, htab2_name,
	htab3_name, htab9_name, htab10_name, rtab_name);
    SIflush (stdout);

    /*
    ** Do same queries with HASH, ISAM, HEAP tables
    */
##  destroy htab1_name
##  create htab1_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab2_name
##  create htab2_name(a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab3_name
##  create htab3_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab9_name
##  create htab9_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab10_name
##  create htab10_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test5(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	itab1_name[16];
##  char	itab2_name[16];
##  char	itab3_name[16];
##  char	itab9_name[16];
##  char	itab10_name[16];
##  char	htab1_name[16];
##  char	rtab_name[16];

    /*
    ** Fast5
    */

    STprintf(itab1_name, "itab1_%d_%d", base_num, sess_num);
    STprintf(itab2_name, "itab2_%d_%d", base_num, sess_num);
    STprintf(itab3_name, "itab3_%d_%d", base_num, sess_num);
    STprintf(itab9_name, "itab9_%d_%d", base_num, sess_num);
    STprintf(itab10_name, "itab10_%d_%d", base_num, sess_num);
    STprintf(htab1_name, "htab1_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

##  destroy itab1_name
##  create itab1_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab2_name
##  create itab2_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab3_name
##  create itab3_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab9_name
##  create itab9_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab10_name
##  create itab10_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test6(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	stab1_name[16];
##  char	stab2_name[16];
##  char	stab3_name[16];
##  char	stab9_name[16];
##  char	stab10_name[16];
##  char	rtab_name[16];

    /*
    ** Fast6
    */

    STprintf(stab1_name, "stab1_%d_%d", base_num, sess_num);
    STprintf(stab2_name, "stab2_%d_%d", base_num, sess_num);
    STprintf(stab3_name, "stab3_%d_%d", base_num, sess_num);
    STprintf(stab9_name, "stab9_%d_%d", base_num, sess_num);
    STprintf(stab10_name, "stab10_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

##  destroy stab1_name
##  create stab1_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab2_name
##  create stab2_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab3_name
##  create stab3_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab9_name
##  create stab9_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab10_name
##  create stab10_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test7(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	btab4_name[16];
##  char	btab5_name[16];
##  char	btab6_name[16];
##  char	btab7_name[16];
##  char	btab8_name[16];
##  char	rtab_name[16];

    /*
    ** Fast7
    */
    STprintf(btab4_name, "btab4_%d_%d", base_num, sess_num);
    STprintf(btab5_name, "btab5_%d_%d", base_num, sess_num);
    STprintf(btab6_name, "btab6_%d_%d", base_num, sess_num);
    STprintf(btab7_name, "btab7_%d_%d", base_num, sess_num);
    STprintf(btab8_name, "btab8_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

    /*
    ** Try non - duplicate key
    */
##  destroy btab4_name
##  destroy btab5_name
##  destroy btab6_name
##  destroy btab7_name
##  destroy btab8_name

##  create btab4_name (a=i4, b=c20, c=i4, d=c180) with journaling
##  create btab5_name (a=i4, b=c20, c=i4, d=c180) with journaling
##  create btab6_name (a=i4, b=c20, c=i4, d=c180) with journaling
##  create btab7_name (a=i4, b=c20, c=i4, d=c180) with journaling
##  create btab8_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test8(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	htab4_name[16];
##  char	htab5_name[16];
##  char	htab6_name[16];
##  char	htab7_name[16];
##  char	htab8_name[16];
##  char	rtab_name[16];

    /*
    ** Fast8
    */
    STprintf(htab4_name, "htab4_%d_%d", base_num, sess_num);
    STprintf(htab5_name, "htab5_%d_%d", base_num, sess_num);
    STprintf(htab6_name, "htab6_%d_%d", base_num, sess_num);
    STprintf(htab7_name, "htab7_%d_%d", base_num, sess_num);
    STprintf(htab8_name, "htab8_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

##  destroy htab4_name
##  create htab4_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab5_name
##  create htab5_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab6_name
##  create htab6_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab7_name
##  create htab7_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy htab8_name
##  create htab8_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test9(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	itab4_name[16];
##  char	itab5_name[16];
##  char	itab6_name[16];
##  char	itab7_name[16];
##  char	itab8_name[16];
##  char	rtab_name[16];

    /*
    ** Fast9
    */
    STprintf(itab4_name, "itab4_%d_%d", base_num, sess_num);
    STprintf(itab5_name, "itab5_%d_%d", base_num, sess_num);
    STprintf(itab6_name, "itab6_%d_%d", base_num, sess_num);
    STprintf(itab7_name, "itab7_%d_%d", base_num, sess_num);
    STprintf(itab8_name, "itab8_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

##  destroy itab4_name
##  create itab4_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab5_name
##  create itab5_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab6_name
##  create itab6_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab7_name
##  create itab7_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy itab8_name
##  create itab8_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test10(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	stab4_name[16];
##  char	stab5_name[16];
##  char	stab6_name[16];
##  char	stab7_name[16];
##  char	stab8_name[16];
##  char	rtab_name[16];

    /*
    ** Fast10
    */
    STprintf(stab4_name, "stab4_%d_%d", base_num, sess_num);
    STprintf(stab5_name, "stab5_%d_%d", base_num, sess_num);
    STprintf(stab6_name, "stab6_%d_%d", base_num, sess_num);
    STprintf(stab7_name, "stab7_%d_%d", base_num, sess_num);
    STprintf(stab8_name, "stab8_%d_%d", base_num, sess_num);
    STprintf(rtab_name, "rtab_%d_%d", base_num, sess_num);

##  destroy stab4_name
##  create stab4_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab5_name
##  create stab5_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab6_name
##  create stab6_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab7_name
##  create stab7_name (a=i4, b=c20, c=i4, d=c180) with journaling

##  destroy stab8_name
##  create stab8_name (a=i4, b=c20, c=i4, d=c180) with journaling
##}

roll_test11(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	btab11_name[16];
##  char	btab12_name[16];
##  char	btab13_name[16];
##  char	btab14_name[16];
##  char	rtab2_name[16];

    /*
    ** Fast11
    */
    STprintf(btab11_name, "btab11_%d_%d", base_num, sess_num);
    STprintf(btab12_name, "btab12_%d_%d", base_num, sess_num);
    STprintf(btab13_name, "btab13_%d_%d", base_num, sess_num);
    STprintf(btab14_name, "btab14_%d_%d", base_num, sess_num);
    STprintf(rtab2_name, "rtab2_%d_%d", base_num, sess_num);

    /*
    ** Try duplicate rows
    */
##  destroy btab11_name
##  create btab11_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy btab12_name
##  create btab12_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy btab13_name
##  create btab13_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy btab14_name
##  create btab14_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling
##}

roll_test12(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	htab11_name[16];
##  char	htab12_name[16];
##  char	htab13_name[16];
##  char	htab14_name[16];
##  char	rtab2_name[16];

    /*
    ** Fast12
    */
    STprintf(htab11_name, "htab11_%d_%d", base_num, sess_num);
    STprintf(htab12_name, "htab12_%d_%d", base_num, sess_num);
    STprintf(htab13_name, "htab13_%d_%d", base_num, sess_num);
    STprintf(htab14_name, "htab14_%d_%d", base_num, sess_num);
    STprintf(rtab2_name, "rtab2_%d_%d", base_num, sess_num);

    /*
    ** Try duplicate rows - HASH, ISAM, HEAP
    */
##  destroy htab11_name
##  create htab11_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy htab12_name
##  create htab12_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy htab13_name
##  create htab13_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy htab14_name
##  create htab14_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling
##}

roll_test13(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	itab11_name[16];
##  char	itab12_name[16];
##  char	itab13_name[16];
##  char	itab14_name[16];
##  char	rtab2_name[16];

    /*
    ** Fast13
    */
    STprintf(itab11_name, "itab11_%d_%d", base_num, sess_num);
    STprintf(itab12_name, "itab12_%d_%d", base_num, sess_num);
    STprintf(itab13_name, "itab13_%d_%d", base_num, sess_num);
    STprintf(itab14_name, "itab14_%d_%d", base_num, sess_num);
    STprintf(rtab2_name, "rtab2_%d_%d", base_num, sess_num);

##  destroy itab11_name
##  create itab11_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy itab12_name
##  create itab12_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy itab13_name
##  create itab13_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy itab14_name
##  create itab14_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling
##}

roll_test14(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	stab11_name[16];
##  char	stab12_name[16];
##  char	stab13_name[16];
##  char	stab14_name[16];
##  char	rtab2_name[16];

    /*
    ** Fast14
    */
    STprintf(stab11_name, "stab11_%d_%d", base_num, sess_num);
    STprintf(stab12_name, "stab12_%d_%d", base_num, sess_num);
    STprintf(stab13_name, "stab13_%d_%d", base_num, sess_num);
    STprintf(stab14_name, "stab14_%d_%d", base_num, sess_num);
    STprintf(rtab2_name, "rtab2_%d_%d", base_num, sess_num);

##  destroy stab11_name
##  create stab11_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy stab12_name
##  create stab12_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy stab13_name
##  create stab13_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling

##  destroy stab14_name
##  create stab14_name (a=i4, b=c20, c=i4, d=c180) with duplicates, journaling
##}

roll_test15(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	dtab1_name[16];

    /*
    ** Fast15
    */
    STprintf(dtab1_name, "dtab1_%d_%d", base_num, sess_num);

    /*
    ** append dups, then modify to unique and make sure that
    ** transactions don't attempt to redo.
    */
##  destroy dtab1_name
##  create dtab1_name (a=i4) with journaling
##}

roll_test16(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	xtab1_name[16];
##  char	i1x1_name[16];
##  char	i1x2_name[16];
##  char	i1x3_name[16];
##  char	i1x4_name[16];
##  char	i1x5_name[16];
##  char	i1x6_name[16];
##  char	i1x7_name[16];

    /*
    ** Fast16
    */
    STprintf(xtab1_name, "xtab1_%d_%d", base_num, sess_num);
    STprintf(i1x1_name, "i1x1_%d_%d", base_num, sess_num);
    STprintf(i1x2_name, "i1x2_%d_%d", base_num, sess_num);
    STprintf(i1x3_name, "i1x3_%d_%d", base_num, sess_num);
    STprintf(i1x4_name, "i1x4_%d_%d", base_num, sess_num);
    STprintf(i1x5_name, "i1x5_%d_%d", base_num, sess_num);
    STprintf(i1x6_name, "i1x6_%d_%d", base_num, sess_num);
    STprintf(i1x7_name, "i1x7_%d_%d", base_num, sess_num);

    /*
    ** Secondary index tests.
    */
##  destroy xtab1_name
##  create xtab1_name (a=i4,b=i4,c=i4) with journaling

##  set journaling
##  index on xtab1_name is i1x1_name (a)
##  index on xtab1_name is i1x2_name (a)
##  index on xtab1_name is i1x3_name (a,b)
##  index on xtab1_name is i1x4_name (c)
##  index on xtab1_name is i1x5_name (c)
##  index on xtab1_name is i1x6_name (b)
##  index on xtab1_name is i1x7_name (b)
##}

roll_test17(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	xtab2_name[16];
##  char	i2x1_name[16];
##  char	i2x2_name[16];
##  char	i2x3_name[16];
##  char	i2x4_name[16];
##  char	i2x5_name[16];
##  char	i2x6_name[16];
##  char	i2x7_name[16];

    /*
    ** Fast17
    */
    STprintf(xtab2_name, "xtab2_%d_%d", base_num, sess_num);
    STprintf(i2x1_name, "i2x1_%d_%d", base_num, sess_num);
    STprintf(i2x2_name, "i2x2_%d_%d", base_num, sess_num);
    STprintf(i2x3_name, "i2x3_%d_%d", base_num, sess_num);
    STprintf(i2x4_name, "i2x4_%d_%d", base_num, sess_num);
    STprintf(i2x5_name, "i2x5_%d_%d", base_num, sess_num);
    STprintf(i2x6_name, "i2x6_%d_%d", base_num, sess_num);
    STprintf(i2x7_name, "i2x7_%d_%d", base_num, sess_num);

##  destroy xtab2_name
##  create xtab2_name (a=i4,b=i4,c=i4) with journaling

##  set journaling
##  index on xtab2_name is i2x1_name (a)
##  index on xtab2_name is i2x2_name (a)
##  index on xtab2_name is i2x3_name (a,b)
##  index on xtab2_name is i2x4_name (c)
##  index on xtab2_name is i2x5_name (c)
##  index on xtab2_name is i2x6_name (b)
##  index on xtab2_name is i2x7_name (b)

##}

roll_test18(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	xtab3_name[16];
##  char	i3x1_name[16];
##  char	i3x2_name[16];
##  char	i3x3_name[16];
##  char	i3x4_name[16];
##  char	i3x5_name[16];
##  char	i3x6_name[16];
##  char	i3x7_name[16];

    /*
    ** Fast18
    */
    STprintf(xtab3_name, "xtab3_%d_%d", base_num, sess_num);
    STprintf(i3x1_name, "i3x1_%d_%d", base_num, sess_num);
    STprintf(i3x2_name, "i3x2_%d_%d", base_num, sess_num);
    STprintf(i3x3_name, "i3x3_%d_%d", base_num, sess_num);
    STprintf(i3x4_name, "i3x4_%d_%d", base_num, sess_num);
    STprintf(i3x5_name, "i3x5_%d_%d", base_num, sess_num);
    STprintf(i3x6_name, "i3x6_%d_%d", base_num, sess_num);
    STprintf(i3x7_name, "i3x7_%d_%d", base_num, sess_num);

##  destroy xtab3_name
##  create xtab3_name (a=i4,b=i4,c=i4) with journaling

##  set journaling
##  index on xtab3_name is i3x1_name (a)
##  index on xtab3_name is i3x2_name (a)
##  index on xtab3_name is i3x3_name (a,b)
##  index on xtab3_name is i3x4_name (c)
##  index on xtab3_name is i3x5_name (c)
##  index on xtab3_name is i3x6_name (b)
##  index on xtab3_name is i3x7_name (b)

##}

roll_test19(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	xtab4_name[16];
##  char	i4x1_name[16];
##  char	i4x2_name[16];
##  char	i4x3_name[16];
##  char	i4x4_name[16];
##  char	i4x5_name[16];
##  char	i4x6_name[16];
##  char	i4x7_name[16];

    /*
    ** Fast19
    */
    STprintf(xtab4_name, "xtab4_%d_%d", base_num, sess_num);
    STprintf(i4x1_name, "i4x1_%d_%d", base_num, sess_num);
    STprintf(i4x2_name, "i4x2_%d_%d", base_num, sess_num);
    STprintf(i4x3_name, "i4x3_%d_%d", base_num, sess_num);
    STprintf(i4x4_name, "i4x4_%d_%d", base_num, sess_num);
    STprintf(i4x5_name, "i4x5_%d_%d", base_num, sess_num);
    STprintf(i4x6_name, "i4x6_%d_%d", base_num, sess_num);
    STprintf(i4x7_name, "i4x7_%d_%d", base_num, sess_num);

##  destroy xtab4_name
##  create xtab4_name (a=i4,b=i4,c=i4) with journaling

##  set journaling
##  index on xtab4_name is i4x1_name (a)
##  index on xtab4_name is i4x2_name (a)
##  index on xtab4_name is i4x3_name (a,b)
##  index on xtab4_name is i4x4_name (c)
##  index on xtab4_name is i4x5_name (c)
##  index on xtab4_name is i4x6_name (b)
##  index on xtab4_name is i4x7_name (b)

##}

roll_test20(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	ctab1_name[16];
##  char	ctab2_name[16];
##  char	ctab3_name[16];
##  char	ctab4_name[16];
##  char	ctab5_name[16];
##  char	ctab6_name[16];
##  char	ctab7_name[16];
##  char	ctab8_name[16];
##  char	ctab9_name[16];
##  char	copy_file[40];

    /*
    ** Fast20
    */
    STprintf(ctab1_name, "ctab1_%d_%d", base_num, sess_num);
    STprintf(ctab2_name, "ctab2_%d_%d", base_num, sess_num);
    STprintf(ctab3_name, "ctab3_%d_%d", base_num, sess_num);
    STprintf(ctab4_name, "ctab4_%d_%d", base_num, sess_num);
    STprintf(ctab5_name, "ctab5_%d_%d", base_num, sess_num);
    STprintf(ctab6_name, "ctab6_%d_%d", base_num, sess_num);
    STprintf(ctab7_name, "ctab7_%d_%d", base_num, sess_num);
    STprintf(ctab8_name, "ctab8_%d_%d", base_num, sess_num);
    STprintf(ctab9_name, "ctab9_%d_%d", base_num, sess_num);
    STprintf(copy_file, "ROLL_DATA:ctab_%d_%d.in", base_num, sess_num);

##  destroy ctab2_name
##  create ctab2_name (a=i4,b=c20) with journaling

##  destroy ctab3_name 
##  create ctab3_name (a=i4,b=c20) with journaling

##  destroy ctab4_name
##  create ctab4_name (a=i4,b=c20) with journaling

##  destroy ctab5_name
##  create ctab5_name (a=i4,b=c20) with journaling

##  destroy ctab6_name
##  set journaling
##  set ret_into hash
##  retrieve into ctab6_name (ctab1_name.all)

##  destroy ctab7_name
##  set ret_into isam
##  retrieve into ctab7_name (ctab1_name.all)

##  destroy ctab8_name
##  set ret_into btree
##  retrieve into ctab8_name (ctab1_name.all)

##  destroy ctab9_name
##  set ret_into heap
##  retrieve into ctab9_name (ctab1_name.all)
##}

roll_test21(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	ttab1_name[16];

    /*
    ** Fast21
    */
    STprintf(ttab1_name, "ttab1_%d_%d", base_num, sess_num);

##  destroy ttab1_name
##  create ttab1_name (a=i4,b=c20) with journaling
##}

roll_test22(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	mtab1_name[16];
##  char	mtab2_name[16];
##  char	mtab3_name[16];

    /*
    ** Fast22
    */
    STprintf(mtab1_name, "mtab1_%d_%d", base_num, sess_num);
    STprintf(mtab2_name, "mtab2_%d_%d", base_num, sess_num);
    STprintf(mtab3_name, "mtab3_%d_%d", base_num, sess_num);

##  destroy mtab1_name
##  destroy mtab2_name
##  destroy mtab3_name
##  create mtab1_name (a=i4, b=i4) with journaling
##  create mtab2_name (a=i4, b=i4) with journaling
##  create mtab3_name (a=i4, b=i4) with journaling

##}

roll_test23(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	comptab1_name[16];
##  char	*large_data1;
    char	blah_data[1001];

    /*
    ** Fast23
    */
    STprintf(comptab1_name, "comptab1_%d_%d", base_num, sess_num);

##  destroy comptab1_name
##  create comptab1_name (a=i4, b=char(50) with null, c=varchar(25), 
##	d=varchar(1000) with null) with journaling

##}

roll_test24(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	comptab2_name[16];
##  char	*large_data2;
    char	blah_data[1001];

    /*
    ** Fast24
    */
    STprintf(comptab2_name, "comptab2_%d_%d", base_num, sess_num);

##  destroy comptab2_name
##  create comptab2_name (a=i4, b=char(50) with null, c=varchar(25), 
##	d=varchar(1000) with null) with journaling

##}

roll_test25(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	comptab3_name[16];
##  char	*large_data3;
    char	blah_data[1001];

    /*
    ** Fast25
    */
    STprintf(comptab3_name, "comptab3_%d_%d", base_num, sess_num);

##  destroy comptab3_name
##  create comptab3_name (a=i4, b=char(50) with null, c=varchar(25), 
##	d=varchar(1000) with null) with journaling

##}

roll_test26(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	comptab4_name[16];
##  char	*large_data4;
    char	blah_data[1001];

    /*
    ** Fast26
    */
    STprintf(comptab4_name, "comptab4_%d_%d", base_num, sess_num);

##  destroy comptab4_name
##  create comptab4_name (a=i4, b=char(50) with null, c=varchar(25), 
##	d=varchar(1000) with null) with journaling

##}

roll_test27(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	concurtab1_name[16];
##  char	concurtab2_name[16];
##  char	concurtab3_name[16];
##  char	concurtab4_name[16];
##  char	concurtab5_name[16];
##  char	concurtab6_name[16];

    /*
    ** Fast27
    */
    STprintf(concurtab1_name, "concurtab1_%d", base_num);
    STprintf(concurtab2_name, "concurtab2_%d", base_num);
    STprintf(concurtab3_name, "concurtab3_%d", base_num);
    STprintf(concurtab4_name, "concurtab4_%d", base_num);
    STprintf(concurtab5_name, "concurtab5_%d", base_num);
    STprintf(concurtab6_name, "concurtab6_%d", base_num);


##  destroy concurtab1_name
##  destroy concurtab2_name
##  destroy concurtab3_name
##  destroy concurtab4_name
##  destroy concurtab5_name
##  destroy concurtab6_name

##  create concurtab1_name (a=i4,b=i4,c=c100) with journaling
##  create concurtab2_name (a=i4,b=i4,c=c100) with journaling
##  create concurtab3_name (a=i4,b=i4,c=c100) with journaling
##  create concurtab4_name (a=i4,b=i4,c=c100) with journaling
##  create concurtab5_name (a=i4,b=i4,c=c100) with journaling
##  create concurtab6_name (a=i4,b=i4,c=c100) with journaling

##}

roll_test28(base_num, sess_num)
int	base_num;
int	sess_num;
##{
##  char	concrtab1_name[16];
##  char	concrtab2_name[16];
##  char	concrtab3_name[16];
##  char	concrtab4_name[16];
##  char	concrtab5_name[16];
##  char	concrtab6_name[16];
##  long	num;
    i4     	i;

    /*
    ** Fast28
    */
    STprintf(concrtab1_name, "concurtab1_%d", base_num);
    STprintf(concrtab2_name, "concurtab2_%d", base_num);
    STprintf(concrtab3_name, "concurtab3_%d", base_num);
    STprintf(concrtab4_name, "concurtab4_%d", base_num);
    STprintf(concrtab5_name, "concurtab5_%d", base_num);
    STprintf(concrtab6_name, "concurtab6_%d", base_num);


##}
