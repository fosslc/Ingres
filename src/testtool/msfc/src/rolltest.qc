/*
**  ROLL_TEST	    - run fast commit test
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
**	16-aug-1999 (somsa01)
**		Changed nat and longnat to i4.
**	07-dec-1999 (somsa01)
**		Include me.h .
**      13-Jan-98 (fanra01)
**              Add me header for MEcopy.
**      22-sep-2000 (mcgem01)
**              More nat and longnat to i4 changes.
**
**
NEEDLIBS = LIBINGRES
**
*/

#include        <compat.h>
#include        <me.h>
#include        <tr.h>
#include        <pc.h>
#include        <st.h>
#include        <me.h>

main(argc, argv)
int	    argc;
char	    *argv[];
##{
##  char	*dbname;
##  int		base_num = -1;
##  int		sess_num = -1;
##  int		test_num = -1;
##  char	copy_file[40];
##  char	table_name[32];
    CL_SYS_ERR	syserr;

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

    if (base_num < 0 || test_num < 0 || test_num > 28 || sess_num < 0)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    if (test_num == 0)
    {
	roll_canon(dbname, base_num, sess_num);
	PCexit(0);
    }

##  ingres dbname

##  set printqry

    switch(test_num)
    {
      case  1:  roll_test1(base_num, sess_num); break;
      case  2:  roll_test2(base_num, sess_num); break;
      case  3:  roll_test3(base_num, sess_num); break;
      case  4:  roll_test4(base_num, sess_num); break;
      case  5:  roll_test5(base_num, sess_num); break;
      case  6:  roll_test6(base_num, sess_num); break;
      case  7:  roll_test7(base_num, sess_num); break;
      case  8:  roll_test8(base_num, sess_num); break;
      case  9:  roll_test9(base_num, sess_num); break;
      case 10:  roll_test10(base_num, sess_num); break;
      case 11:  roll_test11(base_num, sess_num); break;
      case 12:  roll_test12(base_num, sess_num); break;
      case 13:  roll_test13(base_num, sess_num); break;
      case 14:  roll_test14(base_num, sess_num); break;
      case 15:  roll_test15(base_num, sess_num); break;
      case 16:  roll_test16(base_num, sess_num); break;
      case 17:  roll_test17(base_num, sess_num); break;
      case 18:  roll_test18(base_num, sess_num); break;
      case 19:  roll_test19(base_num, sess_num); break;
      case 20:  roll_test20(base_num, sess_num); break;
      case 21:  roll_test21(base_num, sess_num); break;
      case 22:  roll_test22(base_num, sess_num); break;
      case 23:  roll_test23(base_num, sess_num); break;
      case 24:  roll_test24(base_num, sess_num); break;
      case 25:  roll_test25(base_num, sess_num); break;
      case 26:  roll_test26(base_num, sess_num); break;
      case 27:  roll_test27(base_num, sess_num); break;
      case 28:  roll_test28(base_num, sess_num); break;
      default:
	TRdisplay("Unknown test number - %d\n", test_num);
    }

    /*
    ** Create file so driver knows we are ready for server crash.
    */
    STprintf(table_name, "roll_temp_tab_%d_%d", base_num, sess_num);
    STprintf(copy_file, "ROLL_DATA:roll_copy_%d_%d.in", base_num, sess_num);
##  destroy table_name
##  create table_name (a=c30,b=c20)
##  append table_name (a=table_name, b=dbname)
##  copy table_name (a=c30, b=c20, d=d0nl) into copy_file
##  destroy table_name

    /*
    ** Wait for sync lock - driver will release this when all fast commit
    ** tests have reached this point.
    */
##  set trace point DM1420

    /*
    ** Crash server
    */
##  sleep 5
##  set trace point SC908

##  exit
    PCexit(0);
##}

roll_usage()
{
    TRdisplay("USAGE: ROLL_TEST dbname test base session\n");
    TRdisplay("    dbname - database.\n");
    TRdisplay("    test   - test number to execute.\n");
    TRdisplay("    base   - base # of driver.\n");
    TRdisplay("    session - which slave this is.\n");
    TRdisplay("\n");
}

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

##  delete rtab_name
##  append rtab_name (c=1)
##  append rtab_name (c=2)
##  append rtab_name (c=3)
##  append rtab_name (c=4)
##  append rtab_name (c=5)
##  append rtab_name (c=6)
##  append rtab_name (c=7)
##  append rtab_name (c=8)
##  append rtab_name (c=9)
##  append rtab_name (c=10)
##  append rtab_name (c=11)
##  append rtab_name (c=12)
##  append rtab_name (c=13)
##  append rtab_name (c=14)
##  append rtab_name (c=15)
##  append rtab_name (c=16)
##  append rtab_name (c=17)
##  append rtab_name (c=18)
##  append rtab_name (c=19)
##  append rtab_name (c=20)

##  delete rtab2_name
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)
##  append rtab2_name (a=0)

##  delete ctab1_name
##  begin transaction
##  append ctab1_name (a=1)
##  append ctab1_name (a=2)
##  append ctab1_name (a=ctab1_name.a+2)
##  append ctab1_name (a=ctab1_name.a+4)
##  append ctab1_name (a=ctab1_name.a+8)
##  append ctab1_name (a=ctab1_name.a+16)
##  append ctab1_name (a=ctab1_name.a+32)
##  append ctab1_name (a=ctab1_name.a+64)
##  append ctab1_name (a=ctab1_name.a+128)
##  append ctab1_name (a=ctab1_name.a+256)
##  append ctab1_name (a=ctab1_name.a+512)

    STprintf(copy_file, "ROLL_DATA:ctab_%d_%d.in", base_num, sess_num);
##  copy ctab1_name () into copy_file

##  end transaction
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

    /*
    ** Append rows that will cause additions to ovfl leaf chains of btree
    */
##  delete btab1_name
##  modify btab1_name to btree on a
##  append btab1_name (c=1)
##  append btab1_name (c=2)
##  append btab1_name (c=3)
##  append btab1_name (c=4)
##  append btab1_name (c=5)
##  append btab1_name (c=6)
##  append btab1_name (c=7)
##  append btab1_name (c=8)
##  append btab1_name (c=9)
##  append btab1_name (c=10)
##  append btab1_name (c=11)
##  append btab1_name (c=12)
##  append btab1_name (c=13)
##  append btab1_name (c=14)
##  append btab1_name (c=15)
##  append btab1_name (c=16)
##  append btab1_name (c=17)
##  append btab1_name (c=18)
##  append btab1_name (c=19)
##  append btab1_name (c=20)
##  append btab1_name (c=1)
##  append btab1_name (c=2)
##  append btab1_name (c=3)
##  append btab1_name (c=4)
##  append btab1_name (c=5)
##  append btab1_name (c=6)
##  append btab1_name (c=7)
##  append btab1_name (c=8)
##  append btab1_name (c=9)
##  append btab1_name (c=10)

    /*
    ** Delete rows on ovfl leaf chains
    */
##  delete btab2_name
##  modify btab2_name to btree on a
##  append btab2_name (c = rtab_name.c)
##  delete btab2_name where btab2_name.a=0

    /*
    ** Replace rows on ovfl leaf chains
    */
##  delete btab3_name
##  modify btab3_name to btree on a
##  append btab3_name (c = rtab_name.c)
##  replace btab3_name (b = "NEW") where btab3_name.a=0
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

##  delete btab9_name
##  modify btab9_name to btree on a
##  append btab9_name (c = rtab_name.c)
##  replace btab9_name (c = btab9_name.c + 400) where btab9_name.c > 0

##  delete btab10_name
##  modify btab10_name to btree on a
##  append btab10_name (c = rtab_name.c)
##  replace btab10_name (a = btab10_name.c) where btab10_name.c > 0
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

    /*
    ** Do same queries with HASH, ISAM, HEAP tables
    */
##  delete htab1_name
##  modify htab1_name to hash on a
##  append htab1_name (c = rtab_name.c)

##  delete htab2_name
##  modify htab2_name to hash on a
##  append htab2_name (c = htab1_name.c)
##  delete htab2_name where htab2_name.a=0

##  delete htab3_name
##  begin transaction
##  modify htab3_name to hash on a
##  append htab3_name (c = htab1_name.c)
##  replace htab3_name (b = "NEW") where htab3_name.a=0
##  end transaction

##  delete htab9_name
##  modify htab9_name to hash on a
##  append htab9_name (c = rtab_name.c)
##  replace htab9_name(c = htab9_name.c + 400) where htab9_name.c > 0

##  delete htab10_name
##  modify htab10_name to hash on a
##  append htab10_name (c = rtab_name.c)
##  replace htab10_name (a = htab10_name.c) where htab10_name.c > 0
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

##  delete itab1_name
##  modify itab1_name to isam on a
##  append itab1_name (c = rtab_name.c)

##  delete itab2_name
##  begin transaction
##  modify itab2_name to isam on a
##  append itab2_name (c = htab1_name.c)
##  delete itab2_name where itab2_name.a=0
##  end transaction

##  delete itab3_name
##  modify itab3_name to isam on a
##  append itab3_name (c = htab1_name.c)
##  replace itab3_name (b = "NEW") where itab3_name.a=0

##  delete itab9_name
##  modify itab9_name to isam on a
##  append itab9_name (c = rtab_name.c)
##  replace itab9_name (c = itab9_name.c + 400) where itab9_name.c > 0

##  delete itab10_name
##  modify itab10_name to isam on a
##  append itab10_name (c = rtab_name.c)
##  replace itab10_name (a = itab10_name.c) where itab10_name.c > 0
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

##  delete stab1_name
##  modify stab1_name to heap
##  append stab1_name (c = rtab_name.c)

##  delete stab2_name
##  begin transaction
##  modify stab2_name to heap
##  append stab2_name (c = rtab_name.c)
##  delete stab2_name where stab2_name.a=0
##  end transaction

##  delete stab3_name
##  modify stab3_name to heap
##  append stab3_name (c = rtab_name.c)
##  replace stab3_name (b = "NEW") where stab3_name.a=0

##  delete stab9_name
##  modify stab9_name to heap
##  append stab9_name (c = rtab_name.c)
##  replace stab9_name (c = stab9_name.c + 400) where stab9_name.c > 0

##  delete stab10_name
##  begin transaction
##  modify stab10_name to heap
##  append stab10_name (c = rtab_name.c)
##  replace stab10_name (a = stab10_name.c) where stab10_name.c > 0
##  end transaction
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
##  delete btab4_name
##  delete btab5_name
##  delete btab6_name
##  delete btab7_name
##  delete btab8_name

##  begin transaction
##  modify btab4_name to btree on a
##  append btab4_name (a = rtab_name.c)

##  modify btab5_name to btree on a
##  append btab5_name (a = rtab_name.c)
##  delete btab5_name where btab5_name.a > 0

##  modify btab6_name to btree on a
##  append btab6_name (a = rtab_name.c)
##  replace btab6_name (b = "NEW") where btab6_name.a > 0

##  modify btab7_name to btree on a
##  append btab7_name (a = rtab_name.c)
##  replace btab7_name (a = btab7_name.a + 400) where btab7_name.a > 0

##  modify btab8_name to btree on a
##  append btab8_name (a = rtab_name.c)
##  replace btab8_name (a = 400, c = btab8_name.a) where btab8_name.a > 0
##  end transaction
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

##  delete htab4_name
##  modify htab4_name to btree on a
##  append htab4_name (a = rtab_name.c)

##  delete htab5_name
##  modify htab5_name to btree on a
##  append htab5_name (a = rtab_name.c)
##  delete htab5_name where htab5_name.a > 0

##  delete htab6_name
##  modify htab6_name to btree on a
##  append htab6_name (a = rtab_name.c)
##  replace htab6_name (b = "NEW") where htab6_name.a > 0

##  delete htab7_name
##  modify htab7_name to btree on a
##  append htab7_name (a = rtab_name.c)
##  replace htab7_name (a = htab7_name.a + 400) where htab7_name.a > 0

##  delete htab8_name
##  modify htab8_name to btree on a
##  append htab8_name (a = rtab_name.c)
##  replace htab8_name (a = 400, c = htab8_name.a) where htab8_name.a > 0
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

##  delete itab4_name
##  modify itab4_name to btree on a
##  append itab4_name (a = rtab_name.c)

##  delete itab5_name
##  modify itab5_name to btree on a
##  append itab5_name (a = rtab_name.c)
##  delete itab5_name where itab5_name.a > 0

##  delete itab6_name
##  modify itab6_name to btree on a
##  append itab6_name (a = rtab_name.c)
##  replace itab6_name (b = "NEW") where itab6_name.a > 0

##  delete itab7_name
##  modify itab7_name to btree on a
##  append itab7_name (a = rtab_name.c)
##  replace itab7_name (a = itab7_name.a + 400) where itab7_name.a > 0

##  delete itab8_name
##  modify itab8_name to btree on a
##  append itab8_name (a = rtab_name.c)
##  replace itab8_name (a = 400, c = itab8_name.a) where itab8_name.a > 0
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

##  delete stab4_name
##  modify stab4_name to btree on a
##  append stab4_name (a = rtab_name.c)

##  delete stab5_name
##  modify stab5_name to btree on a
##  append stab5_name (a = rtab_name.c)
##  delete stab5_name where stab5_name.a > 0

##  delete stab6_name
##  modify stab6_name to btree on a
##  append stab6_name (a = rtab_name.c)
##  replace stab6_name (b = "NEW") where stab6_name.a > 0

##  delete stab7_name
##  modify stab7_name to btree on a
##  append stab7_name (a = rtab_name.c)
##  replace stab7_name (a = stab7_name.a + 400) where stab7_name.a > 0

##  delete stab8_name
##  modify stab8_name to btree on a
##  append stab8_name (a = rtab_name.c)
##  replace stab8_name (a = 400, c = stab8_name.a) where stab8_name.a > 0
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
##  delete btab11_name
##  modify btab11_name to btree on a
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)
##  append btab11_name (a=0)

##  delete btab12_name
##  modify btab12_name to btree on a
##  append btab12_name (a=rtab2_name.a)
##  delete btab12_name where btab12_name.c = 0

##  delete btab13_name
##  modify btab13_name to btree on a
##  append btab13_name (a=rtab2_name.a)
##  replace btab13_name (a = 10) where btab13_name.c = 0

##  delete btab14_name
##  modify btab14_name to btree on a
##  append btab14_name (a=rtab2_name.a)
##  replace btab14_name (a = btab14_name.tid) where btab14_name.c = 0
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
##  delete htab11_name
##  modify htab11_name to btree on a
##  append htab11_name (a=rtab2_name.a)

##  delete htab12_name
##  modify htab12_name to btree on a
##  append htab12_name (a=rtab2_name.a)
##  delete htab12_name where htab12_name.c = 0

##  delete htab13_name
##  modify htab13_name to btree on a
##  append htab13_name (a=rtab2_name.a)
##  replace htab13_name (a = 10) where htab13_name.c = 0

##  delete htab14_name
##  modify htab14_name to btree on a
##  append htab14_name (a=rtab2_name.a)
##  replace htab14_name (a = htab14_name.tid) where htab14_name.c = 0
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

##  delete itab11_name
##  modify itab11_name to btree on a
##  append itab11_name (a=rtab2_name.a)

##  delete itab12_name
##  modify itab12_name to btree on a
##  append itab12_name (a=rtab2_name.a)
##  delete itab12_name where itab12_name.c = 0

##  delete itab13_name
##  modify itab13_name to btree on a
##  append itab13_name (a=rtab2_name.a)
##  replace itab13_name (a = 10) where itab13_name.c = 0

##  delete itab14_name
##  modify itab14_name to btree on a
##  append itab14_name (a=rtab2_name.a)
##  replace itab14_name (a = itab14_name.tid) where itab14_name.c = 0
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

##  delete stab11_name
##  modify stab11_name to btree on a
##  append stab11_name (a=rtab2_name.a)

##  delete stab12_name
##  modify stab12_name to btree on a
##  append stab12_name (a=rtab2_name.a)
##  delete stab12_name where stab12_name.c = 0

##  delete stab13_name
##  modify stab13_name to btree on a
##  append stab13_name (a=rtab2_name.a)
##  replace stab13_name (a = 10) where stab13_name.c = 0

##  delete stab14_name
##  modify stab14_name to btree on a
##  append stab14_name (a=rtab2_name.a)
##  replace stab14_name (a = stab14_name.tid) where stab14_name.c = 0
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
##  delete dtab1_name
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  append dtab1_name (a=0)
##  modify dtab1_name to hash
##  modify dtab1_name to hash unique
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
##  delete xtab1_name
##  modify xtab1_name to btree

##  index on xtab1_name is i1x1_name (a)
##  index on xtab1_name is i1x2_name (a)
##  index on xtab1_name is i1x3_name (a,b)
##  index on xtab1_name is i1x4_name (c)
##  index on xtab1_name is i1x5_name (c)
##  index on xtab1_name is i1x6_name (b)
##  index on xtab1_name is i1x7_name (b)

##  modify i1x1_name to btree
##  modify i1x2_name to isam
##  modify i1x3_name to hash on a,b
##  modify i1x4_name to btree
##  modify i1x5_name to isam
##  modify i1x6_name to btree
##  modify i1x7_name to hash

##  append xtab1_name (a=1, b=1)
##  append xtab1_name (a=2, b=2)
##  append xtab1_name (a=3, b=3)
##  append xtab1_name (a=4, b=4)
##  append xtab1_name (a=xtab1_name.a+4, b=xtab1_name.b)
##  append xtab1_name (a=xtab1_name.a+8, c=xtab1_name.a)
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

##  delete xtab2_name
##  modify xtab2_name to btree

##  index on xtab2_name is i2x1_name (a)
##  index on xtab2_name is i2x2_name (a)
##  index on xtab2_name is i2x3_name (a,b)
##  index on xtab2_name is i2x4_name (c)
##  index on xtab2_name is i2x5_name (c)
##  index on xtab2_name is i2x6_name (b)
##  index on xtab2_name is i2x7_name (b)

##  modify i2x1_name to btree
##  modify i2x2_name to isam
##  modify i2x3_name to hash on a,b
##  modify i2x4_name to btree
##  modify i2x5_name to isam
##  modify i2x6_name to btree
##  modify i2x7_name to hash

##  append xtab2_name (a=1, b=1)
##  append xtab2_name (a=2, b=2)
##  append xtab2_name (a=3, b=3)
##  append xtab2_name (a=4, b=4)
##  append xtab2_name (a=xtab2_name.a+4, b=xtab2_name.b)
##  append xtab2_name (a=xtab2_name.a+8, c=xtab2_name.a)
##  append xtab2_name (a=xtab2_name.a+16, b=xtab2_name.a+16)
##  append xtab2_name (a=xtab2_name.a+32, b=xtab2_name.a+32)
##  append xtab2_name (a=xtab2_name.a+64, b=xtab2_name.a+64)
##  append xtab2_name (a=xtab2_name.a+128, b=xtab2_name.a+128)
##  append xtab2_name (a=xtab2_name.a+512, b=xtab2_name.a+512)
##  append xtab2_name (a=xtab2_name.a+1024, b=xtab2_name.a+1024)
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

##  delete xtab3_name
##  modify xtab3_name to btree

##  index on xtab3_name is i3x1_name (a)
##  index on xtab3_name is i3x2_name (a)
##  index on xtab3_name is i3x3_name (a,b)
##  index on xtab3_name is i3x4_name (c)
##  index on xtab3_name is i3x5_name (c)
##  index on xtab3_name is i3x6_name (b)
##  index on xtab3_name is i3x7_name (b)

##  modify i3x1_name to btree
##  modify i3x2_name to isam
##  modify i3x3_name to hash on a,b
##  modify i3x4_name to btree
##  modify i3x5_name to isam
##  modify i3x6_name to btree
##  modify i3x7_name to hash

##  append xtab3_name (a=1, b=1)
##  append xtab3_name (a=2, b=2)
##  append xtab3_name (a=3, b=3)
##  append xtab3_name (a=4, b=4)
##  append xtab3_name (a=xtab3_name.a+4, b=xtab3_name.b)
##  append xtab3_name (a=xtab3_name.a+8, c=xtab3_name.a)
##  append xtab3_name (a=xtab3_name.a+16, b=xtab3_name.a+16)
##  append xtab3_name (a=xtab3_name.a+32, b=xtab3_name.a+32)
##  append xtab3_name (a=xtab3_name.a+64, b=xtab3_name.a+64)
##  append xtab3_name (a=xtab3_name.a+128, b=xtab3_name.a+128)
##  append xtab3_name (a=xtab3_name.a+512, b=xtab3_name.a+512)
##  append xtab3_name (a=xtab3_name.a+1024, b=xtab3_name.a+1024)

##  delete xtab3_name where xtab3_name.a > 23 and xtab3_name.a < 400
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

##  delete xtab4_name
##  modify xtab4_name to btree

##  index on xtab4_name is i4x1_name (a)
##  index on xtab4_name is i4x2_name (a)
##  index on xtab4_name is i4x3_name (a,b)
##  index on xtab4_name is i4x4_name (c)
##  index on xtab4_name is i4x5_name (c)
##  index on xtab4_name is i4x6_name (b)
##  index on xtab4_name is i4x7_name (b)

##  modify i4x1_name to btree
##  modify i4x2_name to isam
##  modify i4x3_name to hash on a,b
##  modify i4x4_name to btree
##  modify i4x5_name to isam
##  modify i4x6_name to btree
##  modify i4x7_name to hash

##  append xtab4_name (a=1, b=1)
##  append xtab4_name (a=2, b=2)
##  append xtab4_name (a=3, b=3)
##  append xtab4_name (a=4, b=4)
##  append xtab4_name (a=xtab4_name.a+4, b=xtab4_name.b)
##  append xtab4_name (a=xtab4_name.a+8, c=xtab4_name.a)
##  append xtab4_name (a=xtab4_name.a+16, b=xtab4_name.a+16)
##  append xtab4_name (a=xtab4_name.a+32, b=xtab4_name.a+32)
##  append xtab4_name (a=xtab4_name.a+64, b=xtab4_name.a+64)
##  append xtab4_name (a=xtab4_name.a+128, b=xtab4_name.a+128)
##  append xtab4_name (a=xtab4_name.a+512, b=xtab4_name.a+512)
##  append xtab4_name (a=xtab4_name.a+1024, b=xtab4_name.a+1024)

##  replace xtab4_name (a=24, b=xtab4_name.b-485, c=xtab4_name.a) 
##	where xtab4_name.a > 233 and xtab4_name.a < 501
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

##  delete ctab2_name
##  modify ctab2_name to isam
##  copy ctab2_name () from copy_file
##  append ctab2_name (a=5000)

##  delete ctab3_name
##  modify ctab3_name to hash
##  copy ctab3_name () from copy_file
##  append ctab3_name (a=5000)

##  delete ctab4_name
##  modify ctab4_name to btree
##  copy ctab4_name () from copy_file
##  append ctab4_name (a=5000)

##  delete ctab5_name
##  modify ctab5_name to heap
##  copy ctab5_name () from copy_file
##  append ctab5_name (a=5000)

##  delete ctab6_name
##  set ret_into hash
##  retrieve into ctab6_name (ctab1_name.all)
##  append ctab6_name (a=5000)

##  delete ctab7_name
##  set ret_into isam
##  retrieve into ctab7_name (ctab1_name.all)
##  append ctab7_name (a=5000)

##  delete ctab8_name
##  set ret_into btree
##  retrieve into ctab8_name (ctab1_name.all)
##  append ctab8_name (a=5000)

##  delete ctab9_name
##  set ret_into heap
##  retrieve into ctab9_name (ctab1_name.all)
##  append ctab9_name (a=5000)
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

##  delete ttab1_name
##  modify ttab1_name to hash on a

##  begin transaction
##	append ttab1_name (a=1)
##	append ttab1_name (a=2)
##	append ttab1_name (a=3)
##	append ttab1_name (a=4)
##  savepoint s1

##	append ttab1_name (a=5)
##	append ttab1_name (a=6)
##  abort

##  begin transaction
##	append ttab1_name (a=10)
##	append ttab1_name (a=11)
##	append ttab1_name (a=12)

##  savepoint s2
##	append ttab1_name (a=13)
##	append ttab1_name (a=14)
##	append ttab1_name (a=15)
##	delete ttab1_name where ttab1_name.a=12
##	replace ttab1_name (a=40) where ttab1_name.a=11
##  abort to s2

##	append ttab1_name (a=16)
##	append ttab1_name (a=17)
##  savepoint s3

##	delete ttab1_name where ttab1_name.a = 31
##	replace ttab1_name (a=90) where ttab1_name.a=32
##	append ttab1_name (a=18)
##  savepoint s4

##	append ttab1_name (a=19)
##  savepoint s5

##	append ttab1_name (a=20)
##  abort to s4

##  end transaction
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

##  delete mtab1_name
##  delete mtab2_name
##  delete mtab3_name

##  begin transaction
##  append mtab1_name (a=1, b=1)
##  append mtab1_name (a=2, b=1)
##  append mtab1_name (a=3, b=1)
##  append mtab1_name (a=4, b=1)
##  append mtab2_name (a=mtab1_name.a)
##  append mtab3_name (b=mtab1_name.b)
##  replace mtab1_name (a=19) where mtab1_name.a=2
##  delete mtab3_name where mtab3_name.b=1
##  end transaction
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
    MEfill(1000, ' ', blah_data);
    MEcopy("LARGE_ROW   ", 9, blah_data);
    large_data1 = blah_data;

##  delete comptab1_name
##  modify comptab1_name to cheap

##  begin transaction
##  append comptab1_name (a=1,  b="one",      c="ROW 1", d="row_one")
##  append comptab1_name (a=2,  b="two",      c="ROW 2")
##  append comptab1_name (a=3,  b="three",    c="ROW 3", d="row_three")
##  append comptab1_name (a=4,  b="four",     c="ROW 4")
##  append comptab1_name (a=5,  b="five",     c="ROW 5", d="row_five")
##  append comptab1_name (a=6,  b="six",      c="ROW 6")
##  append comptab1_name (a=7,  b="seven",    c="ROW 7", d="row_seven")
##  append comptab1_name (a=8,  b="eight",    c="ROW 8")
##  append comptab1_name (a=9,  b="nine",     c="ROW 9", d="row_nine")
##  append comptab1_name (a=10, b="ten",      c="ROW 10")
##  append comptab1_name (a=11, b="eleven",   c="ROW 11", d="row_eleven")
##  append comptab1_name (a=12, b="twelve",   c="ROW 12")
##  append comptab1_name (a=13, b="thirteen", c="ROW 13", d="row_thirteen")
##  append comptab1_name (a=14, b="fourteen", c="ROW 14")
##  append comptab1_name (a=15, b="fifteen",  c="ROW 15", d="row_fifteen")
##  append comptab1_name (a=16, b="sixteen",  c="ROW 16")
##  append comptab1_name (a=17, b="seventeen",c="ROW 17", d="row_seventeen")
##  append comptab1_name (a=18, b="eighteen", c="ROW 18")
##  append comptab1_name (a=19, b="nineteen", c="ROW 19", d="row_nineteen")
##  append comptab1_name (a=20, b="twenty",   c="ROW 20")
##  replace comptab1_name (d = large_data1) where comptab1_name.a = 1
##  replace comptab1_name (d = large_data1) where comptab1_name.a = 2
##  replace comptab1_name (c = comptab1_name.c + "_new")
##  replace comptab1_name (d = "NOT_NULL") where comptab1_name.a = 18
##  delete comptab1_name where comptab1_name.a > 5 and comptab1_name.a < 10
##  end transaction

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
    MEfill(1000, ' ', blah_data);
    MEcopy("LARGE_ROW   ", 9, blah_data);
    large_data2 = blah_data;

##  delete comptab2_name
##  modify comptab2_name to chash

##  begin transaction
##  append comptab2_name (a=1,  b="one",      c="ROW 1", d="row_one")
##  append comptab2_name (a=2,  b="two",      c="ROW 2")
##  append comptab2_name (a=3,  b="three",    c="ROW 3", d="row_three")
##  append comptab2_name (a=4,  b="four",     c="ROW 4")
##  append comptab2_name (a=5,  b="five",     c="ROW 5", d="row_five")
##  append comptab2_name (a=6,  b="six",      c="ROW 6")
##  append comptab2_name (a=7,  b="seven",    c="ROW 7", d="row_seven")
##  append comptab2_name (a=8,  b="eight",    c="ROW 8")
##  append comptab2_name (a=9,  b="nine",     c="ROW 9", d="row_nine")
##  append comptab2_name (a=10, b="ten",      c="ROW 10")
##  append comptab2_name (a=11, b="eleven",   c="ROW 11", d="row_eleven")
##  append comptab2_name (a=12, b="twelve",   c="ROW 12")
##  append comptab2_name (a=13, b="thirteen", c="ROW 13", d="row_thirteen")
##  append comptab2_name (a=14, b="fourteen", c="ROW 14")
##  append comptab2_name (a=15, b="fifteen",  c="ROW 15", d="row_fifteen")
##  append comptab2_name (a=16, b="sixteen",  c="ROW 16")
##  append comptab2_name (a=17, b="seventeen",c="ROW 17", d="row_seventeen")
##  append comptab2_name (a=18, b="eighteen", c="ROW 18")
##  append comptab2_name (a=19, b="nineteen", c="ROW 19", d="row_nineteen")
##  append comptab2_name (a=20, b="twenty",   c="ROW 20")
##  replace comptab2_name (d = large_data2) where comptab2_name.a = 1
##  replace comptab2_name (d = large_data2) where comptab2_name.a = 2
##  replace comptab2_name (c = comptab2_name.c + "_new")
##  replace comptab2_name (d = "NOT_NULL") where comptab2_name.a = 18
##  delete comptab2_name where comptab2_name.a > 5 and comptab2_name.a < 10
##  end transaction

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
    MEfill(1000, ' ', blah_data);
    MEcopy("LARGE_ROW   ", 9, blah_data);
    large_data3 = blah_data;

##  delete comptab3_name
##  modify comptab3_name to cisam

##  begin transaction
##  append comptab3_name (a=1,  b="one",      c="ROW 1", d="row_one")
##  append comptab3_name (a=2,  b="two",      c="ROW 2")
##  append comptab3_name (a=3,  b="three",    c="ROW 3", d="row_three")
##  append comptab3_name (a=4,  b="four",     c="ROW 4")
##  append comptab3_name (a=5,  b="five",     c="ROW 5", d="row_five")
##  append comptab3_name (a=6,  b="six",      c="ROW 6")
##  append comptab3_name (a=7,  b="seven",    c="ROW 7", d="row_seven")
##  append comptab3_name (a=8,  b="eight",    c="ROW 8")
##  append comptab3_name (a=9,  b="nine",     c="ROW 9", d="row_nine")
##  append comptab3_name (a=10, b="ten",      c="ROW 10")
##  append comptab3_name (a=11, b="eleven",   c="ROW 11", d="row_eleven")
##  append comptab3_name (a=12, b="twelve",   c="ROW 12")
##  append comptab3_name (a=13, b="thirteen", c="ROW 13", d="row_thirteen")
##  append comptab3_name (a=14, b="fourteen", c="ROW 14")
##  append comptab3_name (a=15, b="fifteen",  c="ROW 15", d="row_fifteen")
##  append comptab3_name (a=16, b="sixteen",  c="ROW 16")
##  append comptab3_name (a=17, b="seventeen",c="ROW 17", d="row_seventeen")
##  append comptab3_name (a=18, b="eighteen", c="ROW 18")
##  append comptab3_name (a=19, b="nineteen", c="ROW 19", d="row_nineteen")
##  append comptab3_name (a=20, b="twenty",   c="ROW 20")
##  replace comptab3_name (d = large_data3) where comptab3_name.a = 1
##  replace comptab3_name (d = large_data3) where comptab3_name.a = 2
##  replace comptab3_name (c = comptab3_name.c + "_new")
##  replace comptab3_name (d = "NOT_NULL") where comptab3_name.a = 18
##  delete comptab3_name where comptab3_name.a > 5 and comptab3_name.a < 10
##  end transaction

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
    MEfill(1000, ' ', blah_data);
    MEcopy("LARGE_ROW   ", 9, blah_data);
    large_data4 = blah_data;

##  delete comptab4_name
##  modify comptab4_name to cbtree

##  begin transaction
##  append comptab4_name (a=1,  b="one",      c="ROW 1", d="row_one")
##  append comptab4_name (a=2,  b="two",      c="ROW 2")
##  append comptab4_name (a=3,  b="three",    c="ROW 3", d="row_three")
##  append comptab4_name (a=4,  b="four",     c="ROW 4")
##  append comptab4_name (a=5,  b="five",     c="ROW 5", d="row_five")
##  append comptab4_name (a=6,  b="six",      c="ROW 6")
##  append comptab4_name (a=7,  b="seven",    c="ROW 7", d="row_seven")
##  append comptab4_name (a=8,  b="eight",    c="ROW 8")
##  append comptab4_name (a=9,  b="nine",     c="ROW 9", d="row_nine")
##  append comptab4_name (a=10, b="ten",      c="ROW 10")
##  append comptab4_name (a=11, b="eleven",   c="ROW 11", d="row_eleven")
##  append comptab4_name (a=12, b="twelve",   c="ROW 12")
##  append comptab4_name (a=13, b="thirteen", c="ROW 13", d="row_thirteen")
##  append comptab4_name (a=14, b="fourteen", c="ROW 14")
##  append comptab4_name (a=15, b="fifteen",  c="ROW 15", d="row_fifteen")
##  append comptab4_name (a=16, b="sixteen",  c="ROW 16")
##  append comptab4_name (a=17, b="seventeen",c="ROW 17", d="row_seventeen")
##  append comptab4_name (a=18, b="eighteen", c="ROW 18")
##  append comptab4_name (a=19, b="nineteen", c="ROW 19", d="row_nineteen")
##  append comptab4_name (a=20, b="twenty",   c="ROW 20")
##  replace comptab4_name (d = large_data4) where comptab4_name.a = 1
##  replace comptab4_name (d = large_data4) where comptab4_name.a = 2
##  replace comptab4_name (c = comptab4_name.c + "_new")
##  replace comptab4_name (d = "NOT_NULL") where comptab4_name.a = 18
##  delete comptab4_name where comptab4_name.a > 5 and comptab4_name.a < 10
##  end transaction

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

##  begin transaction

##  delete concurtab1_name
##  delete concurtab2_name
##  delete concurtab3_name
##  delete concurtab4_name
##  delete concurtab5_name
##  delete concurtab6_name


##  modify concurtab1_name to isam
##  modify concurtab2_name to hash
##  modify concurtab3_name to btree
##  modify concurtab4_name to cisam
##  modify concurtab5_name to chash
##  modify concurtab6_name to cbtree

## append concurtab1_name (a = 1,b = 1)
## append concurtab1_name (a = 2,b = 1)
## append concurtab1_name (a = concurtab1_name.a +  2, b = 1)
## append concurtab1_name (a = concurtab1_name.a +  4, b = 1)
## append concurtab1_name (a = concurtab1_name.a +  8, b = 1)
## append concurtab1_name (a = concurtab1_name.a + 16, b = 1)
## append concurtab1_name (a = concurtab1_name.a + 32, b = 1)
 
## append concurtab2_name (a = concurtab1_name.a, b = concurtab1_name.b)
## append concurtab3_name (a = concurtab1_name.a, b = concurtab1_name.b)
## append concurtab4_name (a = concurtab1_name.a, b = concurtab1_name.b)
## append concurtab5_name (a = concurtab1_name.a, b = concurtab1_name.b)
## append concurtab6_name (a = concurtab1_name.a, b = concurtab1_name.b)

##  end transaction

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

/***    for (i = 0 ; i < 10000; i ++)	full test, but not enough disk space ***/
    for (i = 0 ; i < 1000; i ++)
    {
	/*
	** Use dbms_cpu as a sort of random number generator.
	** Use the divide-by-two because on some systems the
	** number always seems to be an even-number.
	*/
##	retrieve (num = int4(dbmsinfo("dbms_cpu")))
	num = ((num / 2) % 64) + 1;

##  	begin transaction

##	    replace concrtab1_name (b = concrtab1_name.b+1) 
##		where concrtab1_name.a = num
##	    replace concrtab2_name (b = concrtab2_name.b+1) 
##		where concrtab2_name.a = num
##	    replace concrtab3_name (b = concrtab3_name.b+1) 
##		where concrtab3_name.a = num
##	    replace concrtab4_name (b = concrtab4_name.b+1) 
##		where concrtab4_name.a = num
##	    replace concrtab5_name (b = concrtab5_name.b+1) 
##		where concrtab5_name.a = num
##	    replace concrtab6_name (b = concrtab6_name.b+1) 
##		where concrtab6_name.a = num

##  	end transaction
    }

##}

roll_canon(name, base_num, sess_num)
char	*name;
int	base_num;
int	sess_num;
{
##  char   *dbname;

    dbname = name;

##  ingres dbname

    roll_test1(base_num, sess_num);
    roll_test2(base_num, sess_num);
    roll_test3(base_num, sess_num);
    roll_test4(base_num, sess_num);
    roll_test5(base_num, sess_num);
    roll_test6(base_num, sess_num);
    roll_test7(base_num, sess_num);
    roll_test8(base_num, sess_num);
    roll_test9(base_num, sess_num);
    roll_test10(base_num, sess_num);
    roll_test11(base_num, sess_num);
    roll_test12(base_num, sess_num);
    roll_test13(base_num, sess_num);
    roll_test14(base_num, sess_num);
    roll_test15(base_num, sess_num);
    roll_test16(base_num, sess_num);
    roll_test17(base_num, sess_num);
    roll_test18(base_num, sess_num);
    roll_test19(base_num, sess_num);
    roll_test20(base_num, sess_num);
    roll_test21(base_num, sess_num);
    roll_test22(base_num, sess_num);
    roll_test23(base_num, sess_num);
    roll_test24(base_num, sess_num);
    roll_test25(base_num, sess_num);
    roll_test26(base_num, sess_num);
    roll_test27(base_num, sess_num);
    roll_test28(base_num, sess_num);

##  exit
}
