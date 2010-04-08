/*
**Copyright (c) 2004 Ingres Corporation
*/
/* duvetest.sc - test verifydb's dbms_catalogs operation
**
**  NO_OPTIM=ris_us5
**
**  Description:
**	This is an esql driver to test verifydb's dbms_catalogs operation.
**
**	HOW TO WRITE TESTS:
**	===================
**
**	Global variables are:
**		dbname - name of database to run tests on.  This is visible to
**			 esql as well as to the test driver.
**		dba    - DBA of the database to run tests on.  This is visible
**			 to ESQL as well as to the test driver.
**		user   - a valid ingres user name that you can use for any test
**			 that requires a valid user name (like someone to grant
**			 permits to, etc).  This is visible to ESQL as well as
**			 to the test driver.
**		test_id - contains name of catalog currently being tested.
**		test_status - this is the status variable.  If any test fails
**			 so that subsequent testing should be aborted, the
**			 test_status variable is set to TEST_FAIL.  Otherwise
**			 it should be set to TEST_OK.  All utility routines will
**			 check test_status before performing any operations and
**			 will silently exit if test_status is set to TEST_ABORT.
**		logfile - name of log file to output this test to
**
**	The basic structure is:
**	MAIN calls each of the following routines for each catagory of catalogs:
**	    1. core_cats() - tests verifydb checks/repairs for core catalogs
**	    2. qrymod_cats() - tests verifydb checks/repairs for qrymod catalogs
**	    3. qp_cats() - tests verifydb checks/repairs for query processing
**		           catalogs
**	    4. opt_cats() - tests verifydb checks/repairs for optimizer catalogs
**	    5. dbdb_cats() - tests verifydb checks/repairs for iidbdb only
**			     catalogs
**	    6. debug_me() - special area to dump any debug specific code.
**
**	Each of the above routines calls a routine named ck_<catalog_name> for
**	each catalog it must check.  For example, the routine to check the
**	iiattribute catalog is named ck_iiattribute, the routine to check the
**	iirelation catalog is named ck_iirelation, etc.  When ever you add a new
**	dbms catalog, you must write a new driver (ck_<catalog_name>) for that
**	catalog.  Each driver has one or more tests in it, which correspond
**	to the verifydb checks/repairs specificied for that catalog.  These
**	checks/repairs are specified in the "VERIFYDB DBMS_CATALOG Operation
**	Specification."
**
**	Each routine should start by checking that test_status is not set to
**	TEST_ABORT before doing anything (and if set to TEST_ABORT, it should
**	just return).  Also, each utility routine should set global variable
**	test_id to the name of the catalog it is testing.  For example,
**	ck_iiattribute should STcopy("iiattribute",test_id).  If you develop
**	a new driver, be sure to add a call to it in the approparite routine
**	[core_cats() if it is a core catalog, qrymod_cats() if it is a qrymod
**	catalog, qp_cats() if it is a query processing catalog, opt_cats() if
**	it is an optimizer catalog or dbdb_cats if it is an iidbdb-only catalog]
**	Each ck_<catalog_name> routine is comprised of 1 or more tests on
**	verifydb checks/repairs.
**
**	Each test is comprised of three sections:
**		1.  setup
**		2.  running verifydb (once in report mode and once in
**		    test_runi mode)
**		3.  cleanup -- the test driver needs to undo whatever setup
**		    it did in case verifydb fails, so that the subsequent tests
**		    will not have their results messed up by leftovers from a
**		    failed test.
**	There are utility routines that perform most functions you'll need:
**		open_db -- connects to a DB as DBA with "+Y" so you can update
**			   system catalogs.
**		open_A9db -- connects to a DB as DBA with +Y and -A9 so that you
**			     can update systemt catalogs and mess with indices
**		open_db_as_user -- open database as a specified user
**		close_db -- disconnects from an open database
**		report_vdb -- runs verifydb on the database in report mode
**		run_test -- determines whether or not to run specific test,
**			    based on value of argv[5] (which is only meaningful
**			    if argv[4] is not "1" for "all tests").
**			    **If you add new catalog tests, you'll need to
**			      update this routine to handle them and also
**			      update the catalog name constants.
**		run_vdb	-- runs verifydb on the database in test_runi mode.
**			 
**	This is a template for a typical test:
**	    
	    ************
	    ** test # **
	    ************

	    ** setup **
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql <<statements to set up for test go here >>
	    close_db();

	    ** run test **
	    report_vdb();
	    run_vdb();

	    ** clean up **
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete <<statements to clean up from test go here>>
	    close_db();
**
**
**  Inputs:
**	argv[0] = program pathname (default of operating system)
**	argv[1] = database name
**	argv[2] = dba name
**	argv[3] = user name
**	argv[4] = test inst:
**		  NULL - defaults to all verifydb tests
**	          '1'   - all verifydb tests
**		  '2'  - core catalog tests only
**		  '3'  - qrymod catalog tests only
**		  '4'  - qp catalog tests only
**		  '5'  - optimizer catalog tests only
**		  '6'  - dbdb catalog tests only
**		 '10'  - debug
**		 '20'  - debug
**	    argv[5] = subtest inst:
**		  varies with each test instr, but indicates to only run one
**		  specific subtest under each test type:
**			CORE:   101 to 131 - run iirelation test 1 to 31
**				201 - run iirel_idx test 1
**				301 to 303 - run iiattribute test 1 to 3 
**				401 to 404 - run iiindex test 1 to 3
**				501 to 502 - run iidevices test 1 to 2
**			QRYMOD: 101 to 110 - run iiintegrity tests 1 to 10 
**			        201 to 212 - run iiprotect tests 1 to 12
**			        301 to 306 - run iiqrytext tests 1 to 6
**			        401 to 403 - run iisynonym tests 1 to 3 
**			        501 to 506 - run iitree tests 1 to 6
**				601 to 605 - run iidbms_comment tests 1 to 5
**				701	   - run iikey test 1
**				750 to 754 - run iisecalarm tests 1 to 4
**				801        - run iidefault test 1
**			QP:	101 to 104 - run iidbdepends tests 1 to 4
**			QP:	100 to 104 - run iidbdepends tests 1 to 4
**					    currently iidbdepepends test 5 is
**					    not runnable.
**			        201 to 203 - run iievent tests 1 to 3 
**			        301 to 308 - run iiprocedure tests 1 to 8
**			        401 to 406 - run iirule tests 1 to 6 
**			        501 - run iixdbdepends test 1
**					     currently above test isn't runable
**				601 to 603 - run iigw06_attribute tests 1 to 3
**				701 to 703 - run iigw06_relation test 1 to 3
**				801 to 802 - run iipriv tests 1 to 2
**				901 to 902 - run iixpriv tests 1 to 2
**			      1001 to 1002 - run iixevent tests 1 and 2
**			      1101 to 1102 - run iixprocedure tests 1 and 2
**			OPT:    101 to 105 - run iistatistics tests 1 to 5
**			        201 to 202 - run iihistogram tests 1 to 2
**			IIDBDB: 101 to 102 - run iicdbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        201 to 219 - run iidatabase tests 1 to 15
**			        301 to 302 - run iidbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        401 to 403 - run iidbpriv tests 1 to 3
**			        501 to 502 - run iiextend tests 1 to 2
**			        601 to 602 - run iilocations tests 1 to 2
**			        701 to 705 - run iistar_cdbs tests 1 to 5
**			        801 to 804 - run iiuser tests 1 to 4
**			        901 - run iiusergroup test 1
**	                        950 to 903 - run iiprofile tests 1 to 3
**	    argv[6] = "SEP" if you want to run in sep mode.
**  Outputs:
**  -  messages to STDOUT via SIprintf;
**  -  several files in II_CONFIG containing the results of each test.  All
**     files start with "ii_config:iivdb" and have the extension of the DBA
**     name.  The 6th letter of each file is a test code as follows:
**		a = iidatabase catalog
**		c = core catalogs
**		d = dbdb catalogs
**		m = query mod catalogs
**		o = optimizer catalogs
**		p = query processing catalogs
**		r = iirelation catalog
**
PROGRAM =	testvdb

NEEDLIBS =      DBUTILLIB SQLCALIB LIBQLIB LIBQGCALIB GCFLIB \
                UGLIB FMTLIB AFELIB ADFLIB ULFLIB CUFLIB COMPATLIB MALLOCLIB
**
**  History :
**	04-jun-93 (teresa)
**	    initial creation
**	25-jun-93 (tomm)
**	    Added NEEDLIBS for ming.
**    	28-jun-93 (teresa)
**	    cleaned up the tests to assure they all correctly set up the
**	    test conditions.  Also added a feature to permit specificiation of
**	    and individual test via argv[5].
**	13-jul-93 (teresa)
**	    added support for sep flag.
**	2-aug-93 (stephenb)
**	    Added routines ck_iigw06_relation() and ck_iigw06_attribute().
**	    and updated other modules to fit them in, also added test #29
**	    to ck_iirelation()
**	18-sep-93 (teresa)
**	    Added routine ck_iidbms_comment and added new tests to
**	    ck_iidatabase.
**	22-dec-93 (robf)
**          Added routines ck_iisecalarm(), ck_iiprofile.
**	10-jan-94 (andre)
**	    remove redeclarations of DB_MAXNAME, DU_NAME_UPPER, DU_DELIM_UPPER,
**	    DU_DELIM_MIXED, and DU_REAL_USER_MIXED
**      11-jan-94 (anitap)
**          Corrected typos in open_db(), open_dbdb(), open_db_as_user().
**	    Added tests for FIPS' constraints and CREATE SCHEMA projects.
**	27-jan-94 (andre)
**	    redefined catalog constants to facilitate derivation of the first 
**	    valid test number for a given catalog.  Since redefining test 
**	    numbers would be too much work, I took a different approach:
**		catalogs in each group will be within the same 10000 group
**		(e.g. all core catalogs will be between 0 and 9999); to derive 
**		the first valid test number, one will subtract catalog number 
**		base (in case of core catalog - 0) from teh catalog constant 
**		and add 1
**
**	    defined IIXPROTECT, IIPRIV, IIXPRIV, IIXEVENT, IIXPROCEDURE
**	02-feb-94 (huffman) && (mhuishi)
**	    A conditional was_too_complex for the AXP C compiler to optimize.
**	    This is the programming 101 fix around it.
**	20-feb-96 (mosjo01/moojo03)
**	    NO_OPTIM=ris_us5 needed to resolve the following error:
**	    c89: 1501-229 Compilation ended due to lack of space.
**	    Error ocurred on AIX 3.2.5 using compiler xlc 1.3
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Dec-2004 (schka24)
**	    remove iixprotect tests.
**	19-jan-2005 (abbjo03)
**	    Remove declaration of dmf_svcb.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were turning on iirelation.relstat TCB_VBASE bit.
**          TCB_VBASE not tested anywhere in 2.6 and not turned off when view
**          dropped qeu_cview() call to dmt_alter() to set bit has been removed,
**          removeing references to TCB_VBASE here also.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
*/

    /**********************************/
    /* Global variables and constants */
    /**********************************/

exec sql include SQLCA;	/* Embedded SQL Communications Area */
#include        <compat.h>
#include        <gl.h>
#include        <iicommon.h>
#include        <dbdbms.h>
#include	<adf.h>
#include	<er.h>
#include        <me.h>
#include        <si.h>
#include        <st.h>
#include        <pc.h>
#include	<dudbms.h>
#include	<duerr.h>
#include	<cs.h>
#include	<lg.h>
#include	<lk.h>
#include	<dm.h>
#include	<dmf.h>
#include	<dmp.h>


    /* constants to process input parameters */
#define		INVALID_OPCODE	    0
#define		MAX_DIGITS	    4
#define		BIT1		    0x0001
#define		BIT2		    0x0002
#define		BIT3		    0x0004
#define		BIT4		    0x0008
#define		BIT5		    0x0010
#define		BIT6		    0x0020
#define		BIT10		    0x0040
#define		BIT20		    0x0080
#define         IIDBDB_DB	    "iidbdb"

    /* file codes for name_log */
#define		EXT_POS		    "."
#define		BASE_LOG	    "iivdb"
#define		DTBASE_CODE	    "a"
#define		CORE_CODE	    "c"
#define		DBDB_CODE	    "d"
#define		QP_CODE		    "p"
#define		QRYMOD_CODE	    "m"
#define		STAT_CODE	    "s"
#define		REL_CODE	    "r"
#define		ID_00		    "00"
#define		ID_01		    "01"
#define		ID_02		    "02"
#define		ID_03		    "03"
#define		ID_04		    "04"
#define		ID_05		    "05"
#define		ID_06		    "06"
#define		ID_07		    "07"
#define		ID_08		    "08"
#define		ID_09		    "09"
#define		ID_0A		    "0A"
#define		ID_0B		    "0B"
#define		ID_0C		    "0C"
#define		ID_0D		    "0D"
#define		ID_0E		    "0E"
#define		ID_0F		    "0F"
#define		ID_10		    "10"
#define		ID_11		    "11"
#define		ID_12		    "12"
#define		ID_13		    "13"
#define		ID_14		    "14"
#define		ID_15		    "15"
#define		ID_16		    "16"
#define		ID_17		    "17"
#define		ID_18		    "18"
#define		ID_19		    "19"
#define		ID_1A		    "1A"
#define		ID_1B		    "1B"
#define		ID_1C		    "1C"
#define		ID_1D		    "1D"
#define		ID_1E		    "1E"
#define		ID_1F		    "1F"
#define         ID_6A               "6A"
#define         ID_9A               "9A"
#define         ID_9B               "9B"
#define         ID_28A              "28A"

/* codes for run_test() */
    /* dummy */
#define		II		    0
#define		IIDB_ACCESS	    0
    /* core catalogs */
#define		CORE_CATALOGS	    0
#define		IIRELATION	    100
#define		IIREL_IDX	    200
#define		IIATTRIBUTE	    300
#define		IIINDEX		    400
#define		IIDEVICES	    500
    /* qrymod catalogs */
#define		QRYMOD_CATALOGS	    10000
#define		IIINTEGRITY	    10100
#define		IIPROTECT	    10200
#define		IIQRYTEXT	    10300
#define		IISYNONYM	    10400
#define		IITREE		    10500
#define		IIDBMS_COMMENT	    10600
#define         IIKEY               10700
#define		IISECALARM 	    10750
#define         IIDEFAULT           10800
    /* query processing catalogs */
#define		QP_CATALOGS	    20000
#define		IIDBDEPENDS	    20100
#define		IIEVENT	    	    20200
#define		IIPROCEDURE	    20300
#define		IIRULE		    20400
#define		IIXDBDEPENDS	    20500
#define		IIGW06_ATTRIBUTE    20600
#define		IIGW06_RELATION     20700
#define		IIPRIV		    20800
#define		IIXPRIV		    20900
#define		IIXEVENT	    21000
#define		IIXPROCEDURE	    21100
    /* optimizer catalogs */
#define		OPTIMIZER_CATALOGS  30000
#define		IISTATISTICS	    30100
#define		IIHISTOGRAM	    30200
    /* iidbdb only catalogs */
#define		IIDBDB_CATALOGS	    40000
#define		IICDBID_IDX	    40100
#define		IIDATABASE	    40200
#define		IIDBID_IDX	    40300
#define		IIDBPRIV	    40400
#define		IIEXTEND	    40500
#define		IILOCATIONS	    40600
#define		IISTAR_CDBS	    40700
#define		IIUSER		    40800
#define		IIUSERGROUP	    40900
#define		IIPROFILE	    40950

/* misc relstat bits -- matches values in dmp.h */
#define			PRTUPS		    0x00000004L
#define			INTEGS		    0x00000008L
#define                 SCONCUR		    0x00000010L
#define                 VIEW		    0x00000020L
#define                 VBASE		    0x00000040L
#define			INDEX		    0x00000080L
#define                 IDXD		    0x00020000L
#define                 ZOPTSTAT	    0x00100000L
#define			MULTIPLE_LOC	    0x00400000L
#define			RULE		    0x01000000L
#define			COMMENT_BIT	    0x08000000L

/* relstat2 bits */
#define			ALARM_BIT	    0x01000000L

/* misc dbservice bits -- matches values in dudbms.qsh */

    /* global variables */
    EXEC SQL begin declare section;
	char	dbname[DB_MAXNAME+1];
	char	dba[DB_MAXNAME+1];
	char	test_database[DB_MAXNAME+1];
	char	user[DB_MAXNAME+1];
    EXEC SQL end declare section;
    char	    logfile[DB_MAXNAME+1];
    char	    test_id[DB_MAXNAME+1];
    i4		    test_status;
#define			TEST_OK		    0
#define			TEST_FAIL	    1
#define			TEST_ABORT	    3
    i4		    subtest=0;
    int		    testmap;
    i4		    sep = FALSE;
    DB_STATUS	    suppress;
#define	DO_NOT_SUPPESS	0	/*  suppress no msgs */
#define SUPPRESS_ALL	-1	/*  suppress all msgs */

/*
** main -- main driver for verifydb dbms_catalogs operation tests
**
** Description:
**	Parses the input parameters, initializes global variables and
**	calls the appropriate processing routines based on input parameter
**	"test instruction", as follows:
**	    value:	    routine called:
**	    NULL or 1	    core_cats(), qrymod_cats(), qp_cats(), opt_cats(),
**			    and dbdb_cats()
**	    2		    core_cats()
**	    3		    qrymod_cats()
**	    4		    qp_cats()
**	    5		    opt_cats()
**	    6		    dbdb_cats()
**	    10		    debug_me()
**
** Inputs:
**	argc	    number of input parameters
**	argv[],	    Input parameters
**	    argv[0] = program pathname (default of operating system)
**	    argv[1] = database name
**	    argv[2] = dba name
**	    argv[3] = user name
**	    argv[4] = test inst:
**		  NULL - defaults to all verifydb tests
**	          '1'   - all verifydb tests
**		  '2'  - core catalog tests only
**		  '3'  - qrymod catalog tests only
**		  '4'  - qp catalog tests only
**		  '5'  - optimizer catalog tests only
**		  '6'  - dbdb catalog tests only
**		 '10'  - debug
**	    argv[5] = subtest inst:
**		  varies with each test instr, but indicates to only run one
**		  specific subtest under each test type:
**			CORE:   101 to 129 - run iirelation test 1 to 29
**				201 - run iirel_idx test 1
**				301 to 303 - run iiattribute test 1 to 3 
**				401 to 404 - run iiindex test 1 to 4 
**				501 to 502 - run iidevices test 1 to 2
**			QRYMOD: 101 to 110 - run iiintegrity tests 1 to 10 
**			        201 to 204 - run iiprotect tests 1 to 4
**			        301 to 306 - run iiqrytext tests 1 to 6
**			        401 to 403 - run iisynonym tests 1 to 3 
**			        501 to 506 - run iitree tests 1 to 6
**				601 to 605 - run iidbms_comment tests 1 to 5
**				701	   - run iikey test 1
**				751 to 754 - run iisecalarm test 1 to 4
**				801 	   - run iidefault test 1	
**			QP:	101 to 104 - run iidbdepends tests 1 to 4
**					    currently iidbdepepends test 5 is
**					    not runnable.
**			        201 to 203 - run iievent tests 1 to 3 
**			        301 to 304 - run iiprocedure tests 1 to 4 
**			        401 to 406 - run iirule tests 1 to 6 
**			        501 - run iixdbdepends test 1
**					     currently above test isn't runable
**			OPT:    101 to 105 - run iistatistics tests 1 to 5
**			        201 to 202 - run iihistogram tests 1 to 2
**			IIDBDB: 101 to 102 - run iicdbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        201 to 215 - run iidatabase tests 1 to 15
**			        301 to 302 - run iidbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        401 to 40~ - run iidbpriv tests 1 to ~
**			        501 to 50~ - run iiextend tests 1 to ~
**			        601 to 60~ - run iilocations tests 1 to ~
**			        701 to 70~ - run iistar_cdbs tests 1 to ~
**			        801 to 80~ - run iiuser tests 1 to ~
**			        901 to 90~ - run iiusergroup tests 1 to ~
**	                        950 to 95~- run iiprofile tests 1 to ~
**	    argv[6] = "SEP" if you want to run in sep mode.
**			    
**
** Outputs;
**	None.
** History:
**	08-jun-93 (teresa)
**	    initial creation.
**	22-jun-93 (teresa)
**	    added subtest parameter to enable user to indicate that a specific
**	    test should be run.
**	13-jul-93 (teresa)
**	    added support for sep flag.
**	15-jul-93 (teresa)
**	    fixed unix compiler warning.
**	10-nov-93 (anitap)
**	    Corrected inttabbase to protabbase in tests 3 & 4 in ck_iiprotect().
**	    Corrected ii dbdepends to iidbdepends and indi1 to inid1 and indid2
**	    to inid2 in test 5 in ck_iitree().
**	26-jan-94 (andre)
**	    added new tests for IIRELATION, IIPROCEDURE, and IIPROTECT;
**	    added new drivers for IIPRIV, IIXPRIV, IIXPROCEDURE, IIXEVENT, and
**	    IIXPROTECT
*/
main(argc, argv)
int    argc;
char   *argv[];
{
	int	test_instr = 0;
	int	i;
	int	ret_stat;

    exec sql whenever sqlmessage call sqlprint;
    exec sql whenever sqlerror continue;

    MEadvise(ME_INGRES_ALLOC);

    test_status = TEST_OK;

    do		    /* control loop */
    {
	if (argc < 2)
	{
	    printargs();
	    SIprintf ("retry with database name\n");
	    break;
	}

	if (argc < 3)
	{
	    printargs();
	    SIprintf ("retry with DBA name\n");
	    break;
	}

	if (argc < 4)
	{
	    printargs();
	    SIprintf ("retry with user name\n");
	    break;
	}

	/* copy database name */
	for (i=0; i <= DB_MAXNAME; i++)
	{
	    if (argv[1][i])
		test_database[i] = argv[1][i];
	    else
	    {
		test_database[i] = '\0';
		break;
	    }
	}

	/* copy DBA name */
	for (i=0; i <= DB_MAXNAME; i++)
	{
	    if (argv[2][i])
		dba[i] = argv[2][i];
	    else
	    {
		dba[i] = '\0';
		break;
	    }
	}

	/* copy user name */
	for (i=0; i <= DB_MAXNAME; i++)
	{
	    if (argv[3][i])
		user[i] = argv[3][i];
	    else
	    {
		user[i] = '\0';
		break;
	    }
	}

	/* trim names to 32 characters to protect against idocy! */
	test_database[DB_MAXNAME] = '\0';
	STcopy(test_database,dbname); /*assume that user input db will be used--
				      ** this will not be true if user specifies
				      ** iidbdb only tests */
	dba[DB_MAXNAME]= '\0';
	user[DB_MAXNAME] = '\0';

	if (argc >= 6)
	    xlate_string(argv[5], &subtest);

	if (argc >=7 )
	   if (  ( (argv[6][0]=='s') || (argv[6][0]=='S') ) &&
		 ( (argv[6][1]=='e') || (argv[6][1]=='E') ) &&
		 ( (argv[6][2]=='p') || (argv[6][2]=='P') ) )
			sep = TRUE;

	/* now see what test case we have been asked to do */
	testmap = INVALID_OPCODE;
	if (argc == 4)
	{
	    testmap = BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
	    subtest = 0;
	}
	else
	{
	    /* translate op_code to decimal and assure it is valid */
	    xlate_string(argv[4], &test_instr);
	    switch (test_instr)
	    {
		case 1:			    /* all verifydb tests */
		    testmap |= BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
		    subtest = 0;
		    break;
		case 2:			    /* core catalog tests only */
		    testmap |= BIT2;
		    break;
		case 3:			    /* qrymod catalog tests only */
		    testmap |= BIT3;
		    break;
		case 4:			    /* qp catalog tests only */
		    testmap |= BIT4;
		    break;
		case 5:			    /* optimizer catalog tests only */
		    testmap |= BIT5;
		    break;
		case 6:			    /* iidbdb catalog tests only */
		    testmap |= BIT6;
		    break;
		case 10:		    /* debug */
		    testmap |= BIT10;
		    break;
		case 20:		    /* debug */
		    testmap |= BIT20;
		    break;
		default:
		    /* this op code is currently not implemented */
		    break;
	    }
	}


	if ( testmap == INVALID_OPCODE )
	{
	    printargs();
	    SIprintf ("invalid test instr input parameter.\n");
	    SIprintf ("please retry with a valid test instruction code.\n");
	    break;
	}

	if ( (testmap & BIT2) && (test_status != TEST_ABORT) )
	    core_cats();
	if ( (testmap & BIT3) && (test_status != TEST_ABORT) )
	    qrymod_cats();
	if ( (testmap & BIT4) && (test_status != TEST_ABORT) )
	    qp_cats();
	if ( (testmap & BIT5) && (test_status != TEST_ABORT) )
	    opt_cats();
	if ( (testmap & BIT6) && (test_status != TEST_ABORT) )
	{
	    /* force test to run on iidbdb database */
	    STcopy(IIDBDB_DB,dbname);		
	    /* run tests on iidbdb catalogs */
	    dbdb_cats();
	    /* restore name of test db incase there is any futher testing */
	    STcopy(test_database,dbname); 
	}
	if ( (testmap & BIT10) && (test_status != TEST_ABORT) )
	    debug_me();
	if ( (testmap & BIT20) && (test_status != TEST_ABORT) )
	    debug_me2();

    } while (0);	/* end of control loop */
    if (test_status == TEST_ABORT)
    {
	SIprintf ("Verifydb Tests aborted on catalog %s\n", test_id);
	ret_stat = FAIL;
    }
    else
    {
	SIprintf ("Verifydb Tests completed.\n");
	ret_stat = OK;
    }
    PCexit(ret_stat);
}

/*
** close_db -- close an open database
**
** Description:
**	disconnects from a database.
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	08-jun-93 (teresa)
**	    initial creation.
*/
close_db()
{
    if (test_status == TEST_ABORT)
	return;    
    else
    {
#ifdef DEBUG
	SIprintf ("disconnecting from database\n");
#endif
        exec sql disconnect;
    }
}

/*
** ck_iiattribute - 
**
** Description:
**
**  This routine tests the verifydb iiattribute catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	08-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      23-nov-93 (anitap)
**          added test 3 to check that the default ids in iiattribute match
**          that in iidefault catalog.
*/
ck_iiattribute()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 id3;
	i4 id4;
    EXEC SQL end declare section;

    SIprintf ("..Testing verifydb checks/repairs for catalog iiattribute..\n");
    SIflush(stdout);
    STcopy("iiattribute",test_id);
    do
    {
	if ( run_test(IIATTRIBUTE, 1))
	{

	    SIprintf ("....iiattribute test 1..\n");
	    SIflush(stdout);

	    /***********/
	    /* test 1  */
	    /***********/

	    /* setup */
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_att1(a i4, b i4);
	    exec sql select reltid, reltidx into :id1, :id2 from iirelation
		where relid = 'duve_att1';
	    exec sql insert into iiattribute select 7, 7, attid, attxtra,
		    attname, attoff, attfrmt, attfrml, attfrmp, attkdom,
		    attflag, attdefid1, attdefid2
		from iiattribute where
		    attrelid = :id1 and attrelidx = :id2 and attname = 'b';
	    close_db();

	    /* run test */
	    report_vdb();
	    run_vdb();

	    /* clean up */
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirelation where reltid = :id1 and
		reltidx = :id2;
	    exec sql delete from iiattribute where
		attrelid = :id1 and attrelidx = :id2;	    
	    exec sql delete from iiattribute where attrelidx=7;
	    close_db();
	}

	if ( run_test(IIATTRIBUTE, 2))
	{
	    SIprintf ("....iiattribute test 2..\n");
	    SIflush(stdout);

	    /***********/
	    /* test 2  */
	    /***********/

	    /* setup */
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_att2(a i4, b i4);
	    exec sql select reltid, reltidx into :id1, :id2 from iirelation
		where relid = 'duve_att2';
	    exec sql update iiattribute set attfrmt = 12 where
		attrelid = :id1 and attrelidx = :id2 and attname = 'b';    
	    close_db();

	    /* run test */
	    report_vdb();
	    run_vdb();

	    /* clean up */
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirelation where reltid = :id1
		and reltidx = :id2;
	    exec sql delete from iiattribute where
		attrelid = :id1 and attrelidx = :id2;
	    close_db();
	}

	if ( run_test(IIATTRIBUTE, 3))
        {

            SIprintf ("....iiattribute test 3..\n");
            SIflush(stdout);

            /***********/
            /* test 3  */
            /***********/

            /* setup */
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_att3(i i);
            exec sql select reltid, reltidx into :id1, :id2
                from iirelation where relid = 'duve_att3'; 
	    exec sql select attdefid1, attdefid2 into :id3, :id4
                from iiattribute where attrelid = :id1 and attrelidx = :id2
			and attname = 'i';
	    exec sql update iiattribute set attdefid1 = 17
			where attrelid = :id1 and attrelidx = :id2 
			and attname = 'i' and attdefid1 = :id3 and
			attdefid2 = :id4;
            close_db();

            /* run test */
            report_vdb();
            run_vdb();

            /* clean up */
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql delete from iirelation where reltid = :id1 and
                reltidx = :id2;
            exec sql delete from iiattribute where
		attrelid = :id1 and attrelidx = :id2;
            exec sql delete from iiattribute where attrelid = :id1 and
                attrelidx = :id2 and attdefid1 = :id3 and attdefid2 = :id4
                and attname = 'i';
            close_db();
        }
	
    } while (0);	/* control loop */
}

/*
** ck_iicdbid_idx - 
**
** Description:
**
**  This routine tests the verifydb iicdbid_idxcatalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iicdbid_idx()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iicdbid_idx..\n");
    SIflush(stdout);
    STcopy("iicdbid_idx",test_id);
    do
    {
	if ( run_test(IICDBID_IDX, 1))
	{
	    SIprintf ("....iicdbid_idx test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max (cdb_id) into :id1 from iicdbid_idx;
	    id1 += 2;
	    exec sql insert into iicdbid_idx values (:id1, 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iicdbid_idx where cdb_id = :id1;
	    close_db();
	}

	if ( run_test(IICDBID_IDX, 2))
	{

	    SIprintf ("....iicdbid_idx test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max (db_id) into :id2 from iidatabase;
	    exec sql select max (cdb_id) into :id1 from iicdbid_idx;
	    id1 += 3;	    
	    exec sql insert into iistar_cdbs values ('fake_ddb', 'fake_dba',
		'fake_cdb', 'fake_dba', 'fake_node', 'ingres', 'UNKNOWN', ' ',
		'Y', :id2, '');
	    exec sql select tidp into :val from iicdbid_idx where cdb_id = :id2;
	    val ++;
	    exec sql update iicdbid_idx set tidp = :val where cdb_id = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    val --;
	    exec sql update iicdbid_idx set tidp = :val where cdb_id = :id2;
	    exec sql delete from iistar_cdbs where cdb_id = :id2;
	    exec sql delete from iicdbid_idx where cdb_id = :id2;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidatabase - 
**
** Description:
**
**  This routine tests the verifydb iidatabase catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iidatabase()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4  val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidatabase..\n");
    SIflush(stdout);
    STcopy("iidatabase",test_id);
    do
    {
	if ( run_test(IIDATABASE, 1))
	{
	    name_log(DTBASE_CODE,ID_01);
	    SIprintf ("....iidatabase test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase (own,db_id)
		values ('$ingres',100000);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase
		where db_id = 100000 and own='$ingres';
	    close_db();
	}

	if ( run_test(IIDATABASE, 2))
	{
	    name_log(DTBASE_CODE,ID_02);
	    SIprintf ("....iidatabase test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb2','$ingres',
		'bad_default_dev','ii_checkpoint','ii_journal','ii_work',17,0,
		'6.4', 0,2,'ii_dump');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb2';
	    close_db();
	}

	if ( run_test(IIDATABASE, 3))
	{
	    name_log(DTBASE_CODE,ID_03);
	    SIprintf ("....iidatabase test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb3','fake_dba',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,3,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb3',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb3';
	    exec sql delete from iiextend where dname = 'duve_dbdb3';
	    close_db();
	}

	if ( run_test(IIDATABASE, 4))
	{
	    name_log(DTBASE_CODE,ID_04);
	    SIprintf ("....iidatabase test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iilocations
		values (8,'fake_dbloc2','fake_area');
	    exec sql insert into iidatabase values ('duve_dbdb4','$ingres',
		'fake_dbloc2','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,4,'ii_dump');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb4';
	    exec sql delete from iilocations where lname = 'fake_dbloc2';
	    exec sql delete from iiextend where lname = 'fake_dbloc2';
	    close_db();
	}

	if ( run_test(IIDATABASE, 5))
	{
	    name_log(DTBASE_CODE,ID_05);
	    SIprintf ("....iidatabase test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb5','$ingres',
		'ii_database','fake_checkpoint','ii_journal','ii_work',17,0,
		'6.4',0,5,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb5',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb5';
	    exec sql delete from iiextend where dname = 'duve_dbdb5';
	    close_db();
	}

	if ( run_test(IIDATABASE, 6))
	{
	    name_log(DTBASE_CODE,ID_06);
	    SIprintf ("....iidatabase test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb6','$ingres',
		'ii_database','ii_checkpoint','fake_journal','ii_work',17,0,
		'6.4',0,6,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb6',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb6';
	    exec sql delete from iiextend where dname = 'duve_dbdb6';
	    close_db();
	}

	if ( run_test(IIDATABASE, 7))
	{
	    name_log(DTBASE_CODE,ID_07);
	    SIprintf ("....iidatabase test 7..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 7  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb7','$ingres',
		'ii_database','ii_checkpoint','ii_journal','fake_sort',17,0,
		'6.4',0,7,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb7',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
     	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb7';
	    exec sql delete from iiextend where dname = 'duve_dbdb7';
	    close_db();
	}

	if ( run_test(IIDATABASE, 8))
	{
	    name_log(DTBASE_CODE,ID_08);
	    /*************/
	    /** test 8  **/
	    /*************/
	    /* this test is obsoleted */
	}

	if ( run_test(IIDATABASE, 9))
	{
	    name_log(DTBASE_CODE,ID_09);
	    SIprintf ("....iidatabase test 9..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 9  **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb9','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',4,0,'6.4',
		0,9,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb9',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb9';
	    exec sql delete from iiextend where dname = 'duve_dbdb9';
	    close_db();
	}

	if ( run_test(IIDATABASE, 10))
	{
	    name_log(DTBASE_CODE,ID_0A);
	    SIprintf ("....iidatabase test 10..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 10 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb10','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'7.6',
		0,987,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb10',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb10';
	    exec sql delete from iiextend where dname = 'duve_dbdb10';
	    close_db();
	}

	if (run_test(IIDATABASE, 11))
	{
	    name_log(DTBASE_CODE,ID_0B);
	    SIprintf ("....iidatabase test 11..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 11 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb11','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		3,11,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb11',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb11';
	    exec sql delete from iiextend where dname = 'duve_dbdb11';
	    close_db();
	}

	if ( run_test(IIDATABASE, 12) )
	{
	    name_log(DTBASE_CODE,ID_0C);
	    SIprintf ("....iidatabase test 12..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 12 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb12','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,0,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb12',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb12';
	    exec sql delete from iiextend where dname = 'duve_dbdb12';
	    close_db();
	}

#if 0
-- this is currently untestable, since INGRES does not permit inserts of
-- tuples with duplicate keys into iidatabase
	if ( run_test(IIDATABASE, 13))
	{
	    name_log(DTBASE_CODE,ID_0D);
	    SIprintf ("....iidatabase test 13..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 13 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb13a','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,13,'ii_dump');
	    exec sql insert into iidatabase values ('duve_dbdb13b','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,13,'ii_dump');
	    exec sql insert into iiextend values
		('ii_database','duve_dbdb13a',3);
	    exec sql insert into iiextend values
		('ii_database','duve_dbdb13b',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb13a';
	    exec sql delete from iidatabase where name = 'duve_dbdb13b';
	    exec sql delete from iiextend where dname = 'duve_dbdb13a';
	    exec sql delete from iiextend where dname = 'duve_dbdb13b';
	    close_db();
	}
#endif

	if ( run_test(IIDATABASE, 14) )
	{
	    name_log(DTBASE_CODE,ID_0E);
	    /*************/
	    /** test 14 **/
	    /*************/

	    /* currently this is not testable because the server will not permit
	    ** us to directly minipulate secondary indexes */

	}

	if ( run_test(IIDATABASE, 15) )
	{
	    name_log(DTBASE_CODE,ID_0F);
	    SIprintf ("....iidatabase test 15..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 15 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb15','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,15,'fake_dmp');
	    exec sql insert into iiextend values('ii_database','duve_dbdb15',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb15';
	    exec sql delete from iiextend where dname = 'duve_dbdb15';
	    close_db();
	}

	if ( run_test(IIDATABASE, 16) )
	{
	    name_log(DTBASE_CODE,ID_10);
	    SIprintf ("....iidatabase test 16..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 16 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    val = DU_NAME_UPPER;
	    exec sql insert into iidatabase values ('duve_dbdb16','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,
		:val, '7.0',0,16,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb16',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb16';
	    exec sql delete from iiextend where dname = 'duve_dbdb16';
	    close_db();
	}

	if ( run_test(IIDATABASE, 17) )
	{
	    name_log(DTBASE_CODE,ID_11);
	    SIprintf ("....iidatabase test 17..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 17 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    val = DU_DELIM_UPPER;
	    exec sql insert into iidatabase values ('duve_dbdb17','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,
		:val, '7.0',0,17,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb17',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb17';
	    exec sql delete from iiextend where dname = 'duve_dbdb17';
	    close_db();
	}	

	if ( run_test(IIDATABASE, 18) )
	{
	    name_log(DTBASE_CODE,ID_12);
	    SIprintf ("....iidatabase test 18..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 18 **/
	    /*************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    val = DU_NAME_UPPER | DU_DELIM_UPPER | DU_DELIM_MIXED;
	    exec sql insert into iidatabase values ('duve_dbdb18','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,
		:val, '7.0',0,18,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb18',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb18';
	    exec sql delete from iiextend where dname = 'duve_dbdb18';
	    close_db();
	}

	if ( run_test(IIDATABASE, 19) )
	{
	    name_log(DTBASE_CODE,ID_13);
	    SIprintf ("....iidatabase test 19..\n");
	    SIflush(stdout);

	    /***************/
	    /** test 19   **/
	    /***************/

	    /** setup **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    val = DU_REAL_USER_MIXED;
	    exec sql insert into iidatabase values ('duve_dbdb19','$ingres',
		'ii_database','ii_checkpoint','ii_journal','ii_work',17,
		:val, '7.0',0,19,'ii_dump');
	    exec sql insert into iiextend values('ii_database','duve_dbdb19',3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_dbdb();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_dbdb19';
	    exec sql delete from iiextend where dname = 'duve_dbdb19';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidbaccess - 
**
** Description:
**
**  This routine tests the verifydb iidbaccesscatalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iidbaccess()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidbaccess..\n");
    SIflush(stdout);
    STcopy("iidbaccess",test_id);
    do
    {
/* not used for 6.5, so not coded.  Should be coded if verifydb is retrofit
** to 6.4
*/
	if ( run_test(IIDB_ACCESS, 1) )
	{
	    SIprintf ("....iidb_access test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test #  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    /* exec sql <<statements to set up for test go here >> */
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    /* exec sql delete <<statements to clean up from test go here>> */
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidbdepends - 
**
** Description:
**
**  This routine tests the verifydb iidbdepends catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      24-nov-93 (anitap)
**          Added tests 1A & 1B to test if the dependent tuple exists for
**          procedures and rules supporting constraints.
**	    Also added tests 1C and 1D to test if the dependent tuple 
**	    exists for indexes and constraints.
**	    Corrected typo in test 3. Changed indid1 to inid1.
**	    Added i_qid, qid to insert statement in test 1 for views.
**	    Also changed the dtype for views to 17 from 0.
**	11-jan-94 (andre)
**	    corrected setup for tests 1 and 2 so that they test what they claim 
**	    to test. 
*/
ck_iidbdepends()
{
    EXEC SQL begin declare section;
	i4 id1, id2, id3, id4;
	i4 val;
	i4 fakeid;
	i4 txt1,txt2, txt3,txt4;
	i4 dep_type, indep_type;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidbdepends..\n");
    SIflush(stdout);
    STcopy("iidbdepends",test_id);
    do
    {
	if ( run_test(IIDBDEPENDS, 1) )
	{
	    SIprintf ("....iidbdepends test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    dep_type = DB_VIEW;
	    indep_type = DB_TABLE;

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_dbdep1(a i4);
	    exec sql select reltid into :id1 from iirelation
		where relid = 'duve_dbdep1';
	    fakeid = id1+1;
	    exec sql insert into iidbdepends 
		(inid1, inid2, itype, i_qid, qid, deid1, deid2, dtype)
		values (:id1, 0, :indep_type, 0, 0, :fakeid, 0, :dep_type);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_dbdep1;
	    exec sql delete from iidbdepends where deid1 = :fakeid;
	    close_db();

	    /* This test is actually part of test 1, but checks for
            ** dependent objects which are procedures for constraints.
            */
            SIprintf ("....iidbdepends test 1A..\n");
            SIflush(stdout);

            /*************/
            /** test 1A **/
            /*************/

	    indep_type = DB_CONS;
	    dep_type = DB_DBP;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_dbdep1a(a i4 not null unique);
            exec sql create table duve_dbdep1ar(b i4 references
                                duve_dbdep1a(a));
            exec sql select reltid,reltidx
                into :id1,:id2 from iirelation
                where relid = 'duve_dbdep1ar';
            fakeid = id1+1;
            exec sql insert into iidbdepends (inid1,inid2,itype,
			i_qid, qid, deid1,deid2,dtype)
                    values (:id1, :id2, :indep_type, 1, 0, :fakeid, 0, 
			    :dep_type);
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
		break;
            exec sql drop table duve_dbdep1ar;
            exec sql drop table duve_dbdep1a;
            exec sql delete from iitree 
		where treetabbase=:id1 and treetabidx=:id2;
            exec sql delete from iidbdepends where deid1 = :fakeid;
            close_db();

            /* This test is actually part of test 1, but checks for
            ** dependent objects which are rules for constraints.
            */

            SIprintf ("....iidbdepends test 1B..\n");
            SIflush(stdout);

            /*************/
            /** test 1B **/
            /*************/

	    indep_type = DB_CONS;
	    dep_type = DB_RULE;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_dbdep1b(a i4 check(a is not null));
            exec sql select reltid,reltidx
                into :id1,:id2 from iirelation
                where relid = 'duve_dbdep1b';
            fakeid = id1+1;
	    exec sql insert into iidbdepends(inid1, inid2, itype,
		i_qid, qid, deid1, deid2, dtype) 
		values(:id1, :id2, :indep_type, 1, 0, :fakeid, 0, :dep_type);   
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_dbdep1b;
            exec sql delete from iitree 
		where treetabbase=:id1 and treetabidx=:id2;
            exec sql delete from iidbdepends where deid1 = :fakeid;
            close_db();

            /* This test is actually part of test 1, but checks for
            ** dependent objects which is index for UNIQUE
            ** constraint.
            */

            SIprintf ("....iidbdepends test 1C..\n");
            SIflush(stdout);

            /*************/
            /** test 1C **/
            /*************/

	    indep_type = DB_CONS;
	    dep_type = DB_INDEX;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_dbdep1c(a i4 not null unique);
            exec sql select reltid,reltidx
                into :id1,:id2 from iirelation
                where relid = 'duve_dbdep1c';
            fakeid = id1+1;
	    exec sql select i_qid into :id3 from iidbdepends where
		inid1 = :id1 and dtype = :dep_type;
	    exec sql insert into iidbdepends(inid1, inid2, itype,
		i_qid, qid, deid1, deid2, dtype) 
		values(:id1, :id2, :indep_type, :id3, 0, :id1, :fakeid, 
		       :dep_type);   
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_dbdep1c;
            exec sql delete from iitree 
		where treetabbase=:id1 and treetabidx=:id2;
            exec sql delete from iidbdepends where deid1 = :fakeid;
            close_db();

            /* This test is actually part of test 1, but checks for
            ** dependent objects which is constraint for REFERENTIAL 
            ** constraint dependent on an UNIQUE constraint.
            */

            SIprintf ("....iidbdepends test 1D..\n");
            SIflush(stdout);

            /*************/
            /** test 1D **/
            /*************/

	    indep_type = DB_CONS;
	    dep_type = DB_CONS;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_dbdep1d(a i4 not null unique);
	    exec sql create table duve_dbdep1dr(b i4 
			references duve_dbdep1d(a));
            exec sql select reltid,reltidx
                into :id1,:id2 from iirelation
                where relid = 'duve_dbdep1d';
            fakeid = id1+1;
	    exec sql select i_qid into :id3 from iidbdepends where
		inid1 = :id1 and dtype = :dep_type;
	    exec sql insert into iidbdepends(inid1, inid2, itype,
		i_qid, qid, deid1, deid2, dtype) 
		values(:id1, :id2, :indep_type, :id3, 0, :fakeid, 0, :dep_type);   
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_dbdep1d;
	    exec sql drop table duve_dbdep1dr;
            exec sql delete from iitree 
		where treetabbase=:id1 and treetabidx=:id2;
            exec sql delete from iidbdepends where deid1 = :fakeid;
            close_db();
        }

	if ( run_test(IIDBDEPENDS, 2) )
	{
	    SIprintf ("....iidbdepends test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    dep_type = DB_VIEW;
	    indep_type = DB_TABLE;

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_dbdep2(a i4);
	    exec sql create view v_duve_dbdep2 as select * from duve_dbdep2;
	    exec sql select reltid, relqid1, relqid2 into :id1, :txt1, :txt2 
		from iirelation
		where relid = 'v_duve_dbdep2';
	    fakeid = id1 + 1;
	    exec sql insert into iidbdepends
		(inid1, inid2, itype, i_qid, qid, deid1, deid2, dtype)
		values (:fakeid, 0, :indep_type, 0, 0, :id1, 0, :dep_type);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop v_duve_dbdep2;
	    exec sql delete from iidbdepends where inid1 = :fakeid;
	    exec sql delete from iiqrytext 
		where txtid1 = :txt1 and txtid2 = :txt2;
	    exec sql drop table duve_dbdep2;
	    close_db();
	}

	if ( run_test(IIDBDEPENDS, 3) )
	{
	    SIprintf ("....iidbdepends test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_dbdep3(a i4);
	    exec sql create view v_duve_dbdep3 as select * from duve_dbdep3;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop view v_duve_dbdep3;
	    exec sql drop table duve_dbdep3;
	    close_db();
	}

	if ( run_test(IIDBDEPENDS, 4) )
	{
	    SIprintf ("....iidbdepends test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_dbdep4(a i4);
	    exec sql create view v_duve_dbdep4 as select * from duve_dbdep4;
	    exec sql select reltid,reltidx,relqid1,relqid2 
		into :id1,:id2,:txt1,:txt2 from iirelation
		where relid = 'duve_dbdep4';
	    exec sql select reltid,reltidx,relqid1,relqid2
		into :id3,:id4,:txt3,:txt4,:val from iirelation
		where relid = 'v_duve_dbdep4';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop view v_duve_dbdep4;
	    exec sql drop table duve_dbdep4;
	    exec sql delete from iitree
		where treetabbase=:id1 and treetabidx=:id2;
	    exec sql delete from iitree
		where treetabbase=:id3 and treetabidx=:id4;
	    exec sql delete from iiqrytext
		where txtid1 = :txt1 and txtid2 = :txt2;
	    exec sql delete from iiqrytext
		where txtid1 = :txt3 and txtid2 = :txt4;
	    exec sql delete from iiattribute
		where attrelid = :id1 and attrelidx = :id2;
	    exec sql delete from iiattribute
		where attrelid = :id3 and attrelidx = :id4;
	    exec sql delete from iirelation
		where reltid = :id1 and reltidx = :id2;
	    exec sql delete from iirelation
		where reltid = :id3 and reltidx = :id4;
	    exec sql delete from iidbdepends where
		inid1 = :id1 and inid2 = :id2 and deid1 = :id3 and deid2 = :id4;
	    close_db();
	}

	if ( run_test(IIDBDEPENDS, 5) )
	{
	    SIprintf ("....iidbdepends test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /* this is currently not testable, since there is no way to
	    ** corrupt the secondary index via esql.
	    */
	}
    } while (0);  /* control loop */
}

/*
** ck_iixpriv - exercise IIXPRIV tests
**
** Description:
**
**  This routine tests the verifydb iixpriv catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	27-jan-94 (andre)
**	    written
*/
ck_iixpriv()
{
    EXEC SQL begin declare section;
	i4 	id1;
	i4 	val;
	i4	priv;
	u_i4	tid;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iixpriv..\n");
    SIflush(stdout);
    STcopy("iixpriv",test_id);
    do
    {
	if ( run_test(IIXPRIV, 1) )
	{
	    SIprintf ("....iixpriv test 1..\n");
	    SIflush(stdout);
#if	0
	    /* 
	    ** skip this test - cleanup gets messy - dropping the index may 
	    ** result in a deadlock (it is used in qeu_d_cascade()) and cursor 
	    ** delete fails due to insufficient privileges - both can be 
	    ** addressed in BE, but not now
	    */

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;

	    /* 
	    ** insert into IIXPRIV a tuple for which there should be no 
	    ** match in IIPRIV
	    */
	    exec sql insert into iixpriv 
		(i_obj_id, i_obj_idx, i_priv, i_priv_grantee, tidp)
		values (0, 0, 0, 'andre', 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iixpriv 
		where i_obj_id = 0 and i_obj_idx = 0 and i_priv = 0;
	    close_db();
#else
	    SIprintf ("this test is currently skipped.\n");
	    SIflush(stdout);
#endif
	}

	if ( run_test(IIXPRIV, 2) )
	{
	    SIprintf ("....iixpriv test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    /*
	    ** create a view on iirelation and a dummy table to avoid iirelation
	    ** test 13, then delete the newly added IIXPRIV tuple
	    */
	    exec sql drop duve_iixpriv2;
	    exec sql drop duve_iixpriv2_t;
	    exec sql create table duve_iixpriv2_t(i i);
	    exec sql create view duve_iixpriv2 as 
		select * from iirelation, duve_iixpriv2_t;
	    close_db();

	    open_A9db();
	    exec sql select p.tid, i_obj_id, i_priv 
		into :tid, :id1, :priv from iipriv p, iirelation r
		where
			relowner = user
		    and relid = 'duve_iixpriv2'
		    and d_obj_id = reltid
		    and d_obj_idx = reltidx;
	    exec sql delete from iixpriv where tidp = :tid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select count(*) into :val
		from iixpriv
		where
			i_obj_id = :id1
		    and i_obj_idx = 0;
	    
	    if (val == 0)
	    {
		exec sql insert into iixpriv
		    (i_obj_id, i_obj_idx, i_priv, i_priv_grantee, tidp)
		    select :id1, 0, :priv, user, tid
		    from iipriv
		    where i_obj_id = :id1;
	    }

	    exec sql drop duve_iixpriv2;
	    exec sql drop duve_iixpriv2_t;

	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iipriv - test verifydb IIPRIV checks/repairs
**
** Description:
**
**  This routine tests the verifydb iipriv catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	27-jan-94 (andre)
**	    written
*/
ck_iipriv()
{
    EXEC SQL begin declare section;
	i4 id1, id2, id3, id4;
	i4 val;
	i4 fakeid;
	i4 txt1,txt2, txt3,txt4;
	i4 dep_type, indep_type;
	i4 priv;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iipriv..\n");
    SIflush(stdout);
    STcopy("iipriv",test_id);
    do
    {
	if ( run_test(IIPRIV, 1) )
	{
	    SIprintf ("....iipriv test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/

	    /*
	    ** insert a tuple representing a privilege descriptor for 
	    ** non-existant object and let verifydb find it
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    dep_type = DB_VIEW;
	    priv = DB_RETRIEVE | DB_TEST | DB_AGGREGATE;

	    exec sql insert into iipriv(d_obj_id, d_obj_idx, d_priv_number,
		d_obj_type, i_obj_id, i_obj_idx, i_priv, i_priv_grantee, 
		prv_flags, i_priv_map1, i_priv_map2, i_priv_map3, i_priv_map4,
		i_priv_map5, i_priv_map6, i_priv_map7, i_priv_map8, i_priv_map9,
		i_priv_mapa) 
		values
		    (0, 0, 0, 
		     :dep_type, 1, 1, :priv, 'andre', 
		     0, 1, 1, 1, 1,
		     1, 1, 1, 1, 1,
		     1);

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iipriv where d_obj_id = 0 and d_obj_idx = 0;
	    close_db();
	}

	if ( run_test(IIPRIV, 2) )
	{
	    SIprintf ("....iipriv test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a dbproc selecting from IIRELATION, update its privilege 
	    ** descriptor to make it look as if it depends on UPDATE privilege,
	    ** then let verifydb at it
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop procedure duve_iipriv2;
	    exec sql create procedure duve_iipriv2 as 
		declare cnt i;
		begin
		    select count(*) into :cnt from iirelation;
		end;
	    
	    dep_type = DB_DBP;
	    priv = DB_REPLACE;
	    exec sql update iipriv from iiprocedure
		set i_priv = :priv 
		    where
			    dbp_name = 'duve_iipriv2'
			and dbp_owner = user
			and d_obj_id = dbp_id
			and d_obj_idx = dbp_idx
			and d_obj_type = :dep_type;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop procedure duve_iipriv2;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidbid_idx - 
**
** Description:
**
**  This routine tests the verifydb iidbid_idx catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iidbid_idx()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidbid_idx..\n");
    SIflush(stdout);
    STcopy("iidbid_idx",test_id);
    do
    {
	if ( run_test(IIDBID_IDX, 1) )
	{
	    SIprintf ("....iidbid_idx test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max (db_id) into :val from iidatabase;
	    val++;
	    exec sql insert into iidbid_idx values (:val, 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbid_idx where db_id = :val;
	    close_db();
	}

	if ( run_test(IIDBID_IDX, 2) )
	{
	    SIprintf ("....iidbid_idx test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_dbdb2','$ingres',
		'ii_default_dev','ii_checkpoint','ii_journal','ii_work',17,0,'6.4',
		0,22,'ii_dump');
	    exec sql select db_id, tidp into :id1, :val from iidbid_idx;
	    id2 = val +1;
	    exec sql update iidbid_idx set tipd = :id2 where db_id = :id1;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where db_id = :id1;
	    exec sql delete from iidatabase where db_id = :id1;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidbms_comment() - 
**
** Description:
**
**  This routine tests the verifydb iidbms_comment catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	18-sep-93 (teresa)
**	    Initial Creation.
*/
ck_iidbms_comment()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidbms_comment..\n");
    SIflush(stdout);
    STcopy("iidbms_comment",test_id);
    do
    {
	if ( run_test(IIDBMS_COMMENT,1) )
	{
	    SIprintf ("....iidbms_comment test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_cmnt1(a i4);
	    exec sql select reltid,reltidx into :id1,:id2 from iirelation
		where relid = 'duve_cmnt1';
	    exec sql drop table duve_cmnt1;
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 0, 'hi',
		0, 'long remark');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbms_comment where comtabbase = :id1;
	    close_db();
	}

	if ( run_test(IIDBMS_COMMENT,2) )
	{
	    SIprintf ("....iidbms_comment test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_cmnt2(a i4);
	    exec sql select reltid,reltidx into :id1,:id2 from iirelation
		where relid = 'duve_cmnt2';
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 2, 'hi',
		0, 'long remark');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbms_comment where comtabbase = :id1;
	    exec sql drop table duve_cmnt2;
	    close_db();
	}

	if ( run_test(IIDBMS_COMMENT,3) )
	{
	    SIprintf ("....iidbms_comment test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_cmnt3(a i4);
	    exec sql select reltid,reltidx,relstat
		 into :id1,:id2,:val from iirelation
		where relid = 'duve_cmnt3';
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 0, 'hi',
		0, 'long remark');
	    val |= TCB_COMMENT;
	    exec sql update iirelation set relstat = :val
		where reltid = :id1 and reltidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbms_comment where comtabbase = :id1;
	    exec sql drop table duve_cmnt3;
	    close_db();
	}

	if ( run_test(IIDBMS_COMMENT,4) )
	{
	    SIprintf ("....iidbms_comment test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_cmnt4(a i4);
	    exec sql select reltid,reltidx,relstat
		 into :id1,:id2,:val from iirelation
		where relid = 'duve_cmnt4';
	    exec sql insert into iidbms_comment values (:id1, :id2, 2, 2, 'hi',
		0, 'long remark');
	    val |= TCB_COMMENT;
	    exec sql update iirelation set relstat = :val
		where reltid = :id1 and reltidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbms_comment where comtabbase = :id1;
	    exec sql drop table duve_cmnt4;
	    close_db();
	}

	if ( run_test(IIDBMS_COMMENT,5) )
	{
	    SIprintf ("....iidbms_comment test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_cmnt5(a i4);
	    exec sql select reltid,reltidx,relstat
		 into :id1,:id2,:val from iirelation
		where relid = 'duve_cmnt5';
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 2, 'hi',
		0, 'long remark1');
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 2, 'hi',
		1, 'long remark1');
	    exec sql insert into iidbms_comment values (:id1, :id2, 1, 2, 'hi',
		3, 'long remark1');
	    val |= TCB_COMMENT;
	    exec sql update iirelation set relstat = :val
		where reltid = :id1 and reltidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbms_comment where comtabbase = :id1;
	    exec sql drop table duve_cmnt5;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidbpriv - 
**
** Description:
**
**  This routine tests the verifydb iidbpriv catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iidbpriv()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidbpriv..\n");
    SIflush(stdout);
    STcopy("iidbpriv",test_id);
    do
    {
	if ( run_test(IIDBPRIV, 1) )
	{
	    SIprintf ("....iidbpriv test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidbpriv (dbname, grantee, control, flags)
		values ('duve_nonexistent_db','$ingres',0,0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbpriv where dbname = 'duve_nonexistent_db';
	    close_db();
	}

	if ( run_test(IIDBPRIV, 2) )
	{
	    SIprintf ("....iidbpriv test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidbpriv (dbname, grantee, control, flags)
		values ('iidbdb','duve_nonexistent_dba',0,0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidbpriv where grantee= 'duve_nonexistent_dba';
	    close_db();
	}

	if ( run_test(IIDBPRIV, 3 ))
	{
	    SIprintf ("....iidbpriv test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db1',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,2,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db1', 3);
	    exec sql insert into iidbpriv (dbname, grantee, control, flags)
		values ('duve_ficticious_db1','$ingres',15,31);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiextend where dname  = 'duve_ficticious_db1';
	    exec sql delete from iidatabase where name = 'duve_ficticious_db1';
	    exec sql delete from iidbpriv where dbname = 'duve_ficticious_db1';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iidefault -
**
** Description:
**
** This routine tests the verifydb iidefault catalog checks/repairs.
**
** Inputs:
**      None.
** Outputs:
**      global variable: test_status    - status of tests: TEST_OK, TEST_ABORT
** History:
**     23-nov-93 (anitap)
**         initial creation for FIPS constraints' project.
*/
ck_iidefault()
{
    EXEC SQL begin declare section;
        i4 id1, id2, id3;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidefault..\n");
    SIflush(stdout);
    STcopy("iidefault",test_id);
    do
    {
        if (run_test(IIDEFAULT, 1) )
        {
           SIprintf ("....iidefault test 1..\n");
           SIflush(stdout);

           /*************/
           /** test 1 **/
           /************/

           /** setup **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql create table duve_def1(a char(10) with default 'great');
           exec sql select reltid into :id1 from iirelation where
                relid = 'duve_def1';
           exec sql select attdefid1, attdefid2 into :id2, :id3 from iiattribute
                where attrelid = :id1 and attname = 'a';
           exec sql delete from iiattribute
                where attrelid = :id1 and attname = 'a';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql delete from iidefault where defid1 = :id2 and
                        defid2 = :id3;
           exec sql drop table duve_def1;
           close_db();
        }
    } while (0);        /* control loop */
}

/*
** ck_iidevices - 
**
** Description:
**
**  This routine tests the verifydb iidevices catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iidevices()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 stat;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iidevices..\n");
    SIflush(stdout);
    STcopy("iidevices",test_id);
    do
    {
	if ( run_test(IIDEVICES, 1) )
	{
	    SIprintf ("....iidevices test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidevices values (28,28,1,'ii_database');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidevices where devrelidx = 28 and devrelid=28;
	    close_db();
	}

	if ( run_test(IIDEVICES, 2) )
	{
	    SIprintf ("....iidevices test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_dev2(a i4);
	    exec sql select reltid, reltidx into :id1, :id2 from iirelation
		where relid = 'duve_dev2';
	    exec sql insert into iidevices
		values (:id1, :id2, 1, 'bogas_location');
	    exec sql update iirelation set relloccount = relloccount + 1 where
		relid = 'duve_dev2';
	    exec sql select relstat into :stat from iirelation where relid =
		'duve_dev2';
	    stat |= TCB_MULTIPLE_LOC;
	    exec sql update iirelation set relstat = :stat
		where relid ='duve_dev2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    stat &= ~TCB_MULTIPLE_LOC;
	    exec sql update iirelation set relstat = :stat
		where relid ='duve_dev2';
	    exec sql update iirelation set relloccount = 0
		where relid ='duve_dev2';
	    exec sql delete from iidevices where
		devrelid = :id1 and devrelidx = :id2;
	    exec sql drop table duve_dev2;
	    close_db();	exec sql create table duve_dev2(a i4);
	}
    } while (0);  /* control loop */
}

/*
** ck_iixevent - exercise IIXEVENT tests
**
** Description:
**
**  This routine tests the verifydb iixevent catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	27-jan-94 (andre)
**	    written
*/
ck_iixevent()
{
    EXEC SQL begin declare section;
	i4 	id1;
	i4 	val;
	u_i4	tid;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iixevent..\n");
    SIflush(stdout);
    STcopy("iixevent",test_id);
    do
    {
	if ( run_test(IIXEVENT, 1) )
	{
	    SIprintf ("....iixevent test 1..\n");
	    SIflush(stdout);

#if	0
	    /* 
	    ** skip this test - cleanup gets messy - dropping the index may 
	    ** result in a deadlock (it is used in qeu_d_cascade()) and cursor 
	    ** delete fails due to insufficient privileges - both can be 
	    ** addressed in BE, but not now
	    */

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;

	    /* 
	    ** insert into IIXEVENT a tuple for which there should be no 
	    ** match in IIEVENT
	    */
	    exec sql insert into iixevent 
		(event_idbase, event_idx, tidp)
		values (0, 0, 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iixevent 
		where event_idbase = 0 and event_idx = 0;
	    close_db();
#else
	    SIprintf ("this test is currently skipped.\n");
	    SIflush(stdout);
#endif
	}

	if ( run_test(IIXEVENT, 2) )
	{
	    SIprintf ("....iixevent test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;

	    /*
	    ** create a dbevent, then delete the newly added IIXEVENT tuple
	    */
	    exec sql drop dbevent duve_iixevent2;
	    exec sql create dbevent duve_iixevent2;
	    exec sql select tid, event_idbase
		into :tid, :id1 from iievent
		where
			event_owner = user
		    and event_name = 'duve_iixevent2';
	    exec sql delete from iixevent where tidp = :tid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select count(*) into :val
		from iixevent
		where
			event_idbase = :id1
		    and event_idx = 0;
	    
	    if (val == 0)
	    {
		exec sql insert into iixevent
		    (event_idbase, event_idx, tidp)
		    values (:id1, 0, :tid);
	    }

	    exec sql drop dbevent duve_iixevent2;

	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iievent - 
**
** Description:
**
**  This routine tests the verifydb iievent catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	18-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      23-nov-93 (anitap)
**          Added test 3 to check that schema is created for orphaned event.
*/
ck_iievent()
{
    EXEC SQL begin declare section;
	i4 id1, id2;
	i4 txt1, txt2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iievent..\n");
    SIflush(stdout);
    STcopy("iievent",test_id);
    do
    {
	if ( run_test(IIEVENT, 1) )
	{
	    SIprintf ("....iievent test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iievent
		(event_name,event_owner,event_qryid1,event_qryid2)
		values ('duve_event1','dummy', 100, 99);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iievent where event_name = 'duve_event1';
	    close_db();
	}
	
	if ( run_test(IIEVENT, 2) )
	{
	    SIprintf ("....iievent test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_event2(a i4);
	    exec sql create dbevent duve_event;
	    exec sql select reltid into :id1 from iirelation
		where relid = 'duve_event2';
	    exec sql select event_idbase, event_qryid1, event_qryid2
		into :id2, :txt1, :txt2 from iievent
		where event_name = 'duve_event';
	    exec sql update iievent set event_idbase = :id1
		where event_name = 'duve_event';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iievent set event_idbase = :id2
		where event_name = 'duve_event';
	    exec sql drop dbevent duve_event;
	    exec sql drop table duve_event2;
	    exec sql delete from iiqrytext where txtid1=:txt1 and txtid2=:txt2;
	    exec sql delete from iievent where event_name = 'duve_event';
	    close_db();
	}

	if (run_test (IIEVENT, 3) )
        {
           SIprintf("....iievent test 3..\n");
           SIflush(stdout);

           /*************/
           /** test 3  **/
           /*************/

           /** setup   **/
           open_db();
           if (test_status == TEST_ABORT)
               break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql create dbevent duve_event;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql drop dbevent duve_event;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();
        }

    } while (0);  /* control loop */
}

/*
** ck_iiextend - 
**
** Description:
**
**  This routine tests the verifydb iiextend catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iiextend()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiextend..\n");
    SIflush(stdout);
    STcopy("iiextend",test_id);
    do
    {
	
	if ( run_test(IIEXTEND, 1) )
	{
	    SIprintf ("....iiextend test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db2', 3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiextend where dname  = 'duve_ficticious_db2';
	    close_db();
	}

	if ( run_test(IIEXTEND, 2) )
	{
	    SIprintf ("....iiextend test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db3',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,33,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('duve_ficticious_loc_2','duve_ficticious_db3', 3);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiextend where dname  = 'duve_ficticious_db3';
	    exec sql delete from iidatabase where name = 'duve_ficticious_db3';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iigw06_attribute - 
**
** Description:
**
** This routine tests the verifydb iigw06_attribute catalog checks/repairs
**
** Inputs:
**	none.
** Outputs:
**	global variable test_status - status of tests: TEST_OK, TEST_ABORT
** History:
**	2-aug-93 (stephenb)
**	    Initial creation.
*/
ck_iigw06_attribute()
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4 reltid;
    EXEC SQL END DECLARE SECTION;

    SIprintf("..Testing verifydb checks/repairs for catalog iigw06_attribute..\n");
    SIflush(stdout);
    STcopy("iigw06_attribute",test_id);
    do
    {
	if (run_test(IIGW06_ATTRIBUTE, 1) )
	{
	    SIprintf ("....iigw06_attribute test 1..\n");
	    SIflush(stdout);

	    /************/
	    /** test 1 **/
	    /************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    /*
	    ** Put a good row in using register, then corrupt it.
	    */
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    close_db();
	}

	if (run_test(IIGW06_ATTRIBUTE, 2) )
	{
	    SIprintf("....iigw06_attribute test 2..\n");
	    SIflush(stdout);

	    /************/
	    /** test 2 **/
	    /************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    close_db();
	}

	if ( run_test(IIGW06_ATTRIBUTE, 3) )
	{
	    SIprintf ("....iigw06_attribute test 3..\n");
	    SIflush(stdout);

	    /************/
	    /** test 3 **/
	    /************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL UPDATE iigw06_attribute SET reltidx = 1
		WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    close_db();
	}
    } while (0); /* control loop */
}

/*
** ck_iigw06_relation
**
** Description:
**
** This routine tests the verifydb iigw06_relation checks/repairs
**
** Inputs:
**	none.
** Outputs:
**	global variable: test_status - status of tests: TEST_OK, TEST_ABORT
** History:
**	3-aug-93 (stephenb)
**	    Initial creation.
*/
ck_iigw06_relation()
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4 reltid;
    EXEC SQL END DECLARE SECTION;

    SIprintf("..Testing verifydb checks/repairs for catalog iigw06_relation..\n");
    SIflush(stdout);
    STcopy("iigw06_relation", test_id);
    do
    {
	if ( run_test(IIGW06_RELATION, 1) )
	{
	    SIprintf("....iigw06_relation test 1..\n");
	    SIflush(stdout);

	    /************/
	    /** test 1 **/
	    /************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    close_db();
	}

	if ( run_test(IIGW06_RELATION, 2) )
	{
	    SIprintf("....iigw06_relation test 2..\n");
	    SIflush(stdout);

	    /************/
	    /** test 2 **/
	    /************/

	    /** set up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    close_db();
	}

	if (run_test(IIGW06_RELATION, 3) )
	{
	    SIprintf("....iigw06_relation test 3..\n");
	    SIflush(stdout);

	    /************/
	    /** test 3 **/
	    /************/

	    /** set up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
	        AS IMPORT FROM 'current' WITH DBMS = SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL UPDATE iigw06_relation SET reltidx = 1 
		WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iiattribute WHERE attrelid = :reltid;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    close_db();
	}
    } while(0);  /* control loop */
}
 
/*
** ck_iihistogram - 
**
** Description:
**
**  This routine tests the verifydb iihistogram catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	18-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	18-oct-93 (teresa)
**	    changed test1 to assure that iistatistics is not empty, since an
**	    empty iistatistics yeilds S_DU1651_INVALID_IIHISTOGRAM instead of
**	    the desired S_DU164A_NO_STATS_FOR_HISTOGRAM
*/
ck_iihistogram()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;

    SIprintf ("..Testing verifydb checks/repairs for catalog iihistogram..\n");
    SIflush(stdout);
    STcopy("iiihistogram",test_id);
    do
    {
	if ( run_test(IIHISTOGRAM, 1) )
	{
	    SIprintf ("....iihistogram test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_hist1(a i4, b i4);
	    exec sql select reltid,reltidx,relstat into :id1, :id2, :val
		    from iirelation where relid = 'duve_hist1';
           exec sql insert into iistatistics(stabbase,stabindex,satno,snumcells)
		    values (:id1, :id2, 1, 0);	
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val
		where relid ='duve_hist1';
	    exec sql select max(reltid) into :id1;
	    id1++;
	    exec sql insert into iihistogram values (:id1,0,1,0,'hi there');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iihistogram where htabbase = :id1;
	    id1--;
	    exec sql drop table duve_hist1;
	    exec sql delete from iistatistics where stabbase = :id1 and
		stabindex = :id2;
	    close_db();
	}
	if ( run_test(IIHISTOGRAM, 2) )
	{	
	    SIprintf ("....iihistogram test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_hist2(a i4, b i4);
	    exec sql select reltid,reltidx,relstat into :id1, :id2, :val
		    from iirelation where relid = 'duve_hist2';
	   exec sql insert into iistatistics(stabbase,stabindex,satno,snumcells)
		    values (:id1, :id2, 1, 50);	
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val
		where relid ='duve_hist2';
	    exec sql insert into iihistogram values (:id1,:id2,1,0,'hi there');
	    exec sql insert into iihistogram values (:id1,:id2,1,1,'hi there');
	    exec sql insert into iihistogram values (:id1,:id2,1,3,'hi there');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iihistogram where htabbase = :id1;
	    exec sql delete from iistatistics where stabbase = :id1;
	    exec sql drop table duve_hist2;
	    close_db();
	}
    } while (0);  /* control loop */
}

/*
** ck_iiindex - 
**
** Description:
**
**  This routine tests the verifydb index catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	14-jan-94 (anitap)
**	    added logic to test check #4 in DUVECHK.SC for ckindex().
*/
ck_iiindex()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 stat, dep_type;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiindex..\n");
    SIflush(stdout);
    STcopy("index",test_id);
    do
    {
	if ( run_test(IIINDEX, 1) )
	{
	    SIprintf ("....iiindex test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiindex (baseid,indexid,sequence)
		values (30,30,0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiindex where baseid=30 and indexid=30;
	    close_db();
	}

	if ( run_test(IIINDEX, 2) )
	{
	    SIprintf ("....iiindex test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_indx2(a i4, b i4);
	    exec sql create index i_duve_indx2 on duve_indx2(a);
	    exec sql select relstat into :stat from iirelation
		where relid = 'i_duve_indx2';
	    stat &= ~TCB_INDEX;
	    exec sql update iirelation set relstat = :stat
		    where relid = 'i_duve_indx2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    stat |= TCB_INDEX;
	    exec sql update iirelation set relstat = :stat
		    where relid = 'i_duve_indx2';
	    exec sql drop duve_indx2;
	    close_db();
	}

	if ( run_test(IIINDEX, 3) )
	{
	    SIprintf ("....iiindex test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_indx3(a i4, b i4);
	    exec sql create index i_duve_indx3 on duve_indx3(a);
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'i_duve_indx3';
	    exec sql update iiindex set idom2 = 2, idom3 = 3 where baseid = :id1
		    and indexid = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iiindex set idom2 = 0, idom3 = 0 where baseid = :id1
		    and indexid = :id2;
	    exec sql drop index i_duve_indx3;
	    exec sql drop table duve_indx3;
	    close_db();
	}
	if ( run_test(IIINDEX, 4) )
	{
	    SIprintf ("....iiindex test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    dep_type = DB_INDEX;

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_indx4(a i4 not null unique);
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_indx4';
	    exec sql delete from iidbdepends where inid1 = :id1 and inid2 = :id2
		and dtype = :dep_type;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_indx4;
	    close_db();
	}
    } while (0);  /* control loop */
}

/*
** ck_iiintegrity - 
**
** Description:
**
**  This routine tests the verifydb iiintegrity catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	11-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      23-nov-93 (anitap)
**          Added tests 5-10 for FIPS constraints' project.
**	01-feb-94 (anitap)
**	    Get rid of iiqrytext tuples for procedures and rules and iitree
**	    tuples for rules associated with check and referential constraints.
*/
ck_iiintegrity()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 id3;
	i4 id4;
	i4 id5;
	i4 id6;
	i4 id7;
	i4 id8;
	i4 id9;
	i4 id10;
	i4 id11;
	i4 id12;
	i4 id13;
	i4 id14;
	i4 id15;
	i4 id16;
	i4 val;
	i4 qryid1, qryid2, treeid1, treeid2;
	i4 dep_type, indep_type, dep1_type;
	i4 stat_type, stat1_type, stat2_type;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiintegrity..\n");
    SIflush(stdout);
    STcopy("iiintegrity",test_id);
    do
    {
	if ( run_test(IIINTEGRITY, 1) )
	{
	    SIprintf ("....iiintegrity test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiintegrity (inttabbase, inttabidx)
		values (31,31);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiintegrity
		where inttabbase=31 and inttabidx=31;
	    close_db();
	}

	if ( run_test(IIINTEGRITY, 2) )
	{
	    SIprintf ("....iiintegrity test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_int2(a i4, b i4);
	    exec sql insert into duve_int2 values (0,0);
	    exec sql create integrity on duve_int2 is a < 100;
	    exec sql select relstat into :val from iirelation
		    where relid = 'duve_int2';
	    val &= ~TCB_INTEGS;
	    exec sql update iirelation set relstat = :val 	
		    where relid = 'duve_int2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val |= TCB_INTEGS;
	    exec sql update iirelation set relstat = :val 	
		    where relid = 'duve_int2';
	    exec sql drop table duve_int2;
	    close_db();
	}

	if ( run_test(IIINTEGRITY, 3) )
	{
	    SIprintf ("....iiintegrity test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_int3(a i4);
	    exec sql insert into duve_int3 values (1);
	    exec sql create integrity on duve_int3 is a > 0;
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_int3';
	    exec sql select inttreeid1,inttreeid2, intqryid1, intqryid2
		    into :treeid1, :treeid2, :qryid1, :qryid2 from iiintegrity
		    where inttabbase = :id1 and inttabidx = :id2;
	    exec sql delete from iitree where
		    treeid1 = :treeid1 and treeid2 = :treeid2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiintegrity
		    where inttabbase = :id1 and inttabidx = :id2;
	    exec sql delete from iiqrytext where
		    txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql drop table duve_int3;
	    close_db();
	}

	if ( run_test(IIINTEGRITY, 4) )
	{
	    SIprintf ("....iiintegrity test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_int4(a i4);
	    exec sql insert into duve_int4 values (1);
	    exec sql create integrity on duve_int4 is a > 0;
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_int4';
	    exec sql select inttreeid1,inttreeid2, intqryid1, intqryid2
		    into :treeid1, :treeid2, :qryid1, :qryid2 from iiintegrity
		    where inttabbase = :id1 and inttabidx = :id2;
	    exec sql delete from iiqrytext where
		    txtid1 = :qryid1 and txtid2 = :qryid2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiintegrity
		    where inttabbase = :id1 and inttabidx = :id2;
	    exec sql delete from iitree where
		    treeid1 = :treeid1 and treeid2 = :treeid2;
	    exec sql drop table duve_int4;
	    close_db();
	}	

	if ( run_test(IIINTEGRITY, 5) )
        {
            SIprintf ("....iiintegrity test 5..\n");
            SIflush(stdout);

            /*************/
            /** test 5  **/
            /*************/

            /* This test is part of test 5. Tests for keys
            ** for unique constraints.
            */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int5(a i4 not null unique);
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int5';
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iikey where
                    key_consid1 = :id3 and key_consid2 = :id4;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int5;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            close_db();
        }
	if ( run_test(IIINTEGRITY, 6) )
        {
            SIprintf ("....iiintegrity test 6..\n");
            SIflush(stdout);

            /*************/
            /** test 6  **/
            /*************/

	    stat_type = DBR_S_INSERT;
	    stat1_type = DBR_S_UPDATE;	
	    stat2_type = DBR_S_DELETE;

            /* This test is part of test 6. Tests that the unique
	    ** constraint on which the referential depends is 
	    ** present.	
            */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int6(a i4 not null unique);
	    exec sql create table duve_int6r(ar i4 references duve_int6(a));
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int6';
	    exec sql select reltid, reltidx into :id5, :id6 
		from iirelation
		where relid = 'duve_int6r';
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
	    exec sql select intqryid1, intqryid2 into :id7, :id8
		from iiintegrity
		where inttabbase = :id5 and inttabidx = :id6; 
	    exec sql select rule_qryid1, rule_qryid2 into :id9, :id10
		from iirule
		where rule_tabbase = :id1 and rule_tabidx = :id2
		and rule_statement = :stat2_type;	
	    exec sql select rule_qryid1, rule_qryid2 into :id11, :id12
		from iirule
		where rule_tabbase = :id1 and rule_tabidx = :id2
		and rule_statement = :stat1_type;	
	    exec sql select rule_qryid1, rule_qryid2 into :id13, :id14
		from iirule
		where rule_tabbase = :id5 and rule_tabidx = :id6
		and rule_statement = :stat1_type;	
	    exec sql select rule_qryid1, rule_qryid2 into :id15, :id16
		from iirule
		where rule_tabbase = :id5 and rule_tabidx = :id6
		and rule_statement = :stat_type;	
            exec sql delete from iiintegrity where
                    inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int6;
	    exec sql drop table duve_int6r;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            exec sql delete from iiqrytext where
                txtid1 = :id7 and txtid2 = :id8;
            exec sql delete from iiqrytext where
                txtid1 = :id9 and txtid2 = :id10;
            exec sql delete from iiqrytext where
                txtid1 = :id11 and txtid2 = :id12;
            exec sql delete from iiqrytext where
                txtid1 = :id13 and txtid2 = :id14;
            exec sql delete from iiqrytext where
                txtid1 = :id15 and txtid2 = :id16;
	    exec sql delete from iikey where key_consid1 = :id3
		and key_consid2 = :id4;	
            close_db();
        }
        if ( run_test(IIINTEGRITY, 7) )
        {
            SIprintf ("....iiintegrity test 7..\n");
            SIflush(stdout);

            /*************/
            /** test 7  **/
            /*************/

	    dep_type = DB_INDEX;

            /* Tests for iidbdepends for unique constraint */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int7(a i4 not null unique);
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int7';
            exec sql select indexid into :id3 from iiindex where baseid = :id1;
            exec sql delete from iidbdepends
                where inid1 = :id1 and deid2 = :id3 and dtype = :dep_type;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int7;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            exec sql delete from iikey where
                key_consid1 = :id3 and
                key_consid2 = :id4;
            close_db();

            /* This test is part of test 7. Tests for iidbdepends
            ** for check constraints.
            */
            SIprintf ("....iiintegrity test 7A..\n");
            SIflush(stdout);

            /*************/
            /** test 7A **/
            /*************/

	    dep_type = DB_RULE;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int7a(a i4 check(a is not null));
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int7a';
            exec sql select rule_id1, rule_id2, 
		rule_qryid1, rule_qryid2, rule_treeid1, rule_treeid2 
		into :id3, :id4, :id7, :id8, :id9, :id10
                 from iirule where rule_tabbase = :id1 and rule_tabidx = :id2;
            exec sql delete from iidbdepends
                where inid1 = :id1 and deid1 = :id3 and deid2 = :id4
                         and dtype = :dep_type;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id5, :id6
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int7a;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
	    exec sql delete from iitree where
		treeid1 = :id9 and treeid2 = :id10;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql delete from iiqrytext where
		txtid1 = :id7 and txtid2 = :id8; 
            close_db();

            /* This test is part of test 7. Tests for iidbdepends
            ** for referential constraints.
            */

            SIprintf ("....iiintegrity test 7B..\n");
            SIflush(stdout);

            /*************/
            /** test 7B **/
            /*************/

	    indep_type = DB_CONS;
	    dep_type = DB_DBP;
	    dep1_type = DB_INDEX;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int7b(a i4 not null unique);
            exec sql create table duve_int7br(b i4
                        references duve_int7b(a));
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int7br';
            exec sql select deid1, deid2, dbp_txtid1, dbp_txtid2 
			into :id3, :id4, :id7, :id8 
			from iidbdepends, iiprocedure
                        where inid1 = :id1 and inid2 = :id2 and
                        dtype = :dep_type and itype = :indep_type
			and deid1 = dbp_id and deid2 = dbp_idx
			and dbp_parameter_count = 2;
            exec sql delete from iidbdepends
                where inid1 = :id1 and inid2 = :id2
                         and dtype = :dep_type and itype = :indep_type
			and deid1 = :id3 and deid2 = :id4;
	    exec sql delete from iidbdepends 
		where inid1 = :id1 and inid2 = :id2
			and dtype = :dep1_type and itype = :indep_type;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id5, :id6
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int7b;
            exec sql drop table duve_int7br;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql delete from iiqrytext where
		txtid1 = :id7 and txtid2 = :id8;
            exec sql delete from iiprocedure where
                dbp_id = :id3 and dbp_idx = :id4;
            close_db();
        }
        if ( run_test(IIINTEGRITY, 8) )
        {
            SIprintf ("....iiintegrity test 8..\n");
            SIflush(stdout);

            /*************/
            /** test 8  **/
            /*************/

            /* This test is part of test 8. Tests for rules
            ** for check constraints.
            */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int8(a i4 check(a is not null));
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int8';
	    exec sql select rule_qryid1, rule_qryid2,
		rule_treeid1, rule_treeid2 
		into :id5, :id6, :id7, :id8 
		from iirule
		where rule_tabbase = :id1 and rule_tabidx = :id2;
            exec sql delete from iirule where rule_tabbase = :id1 and
                rule_tabidx = :id2;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int8;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
	    exec sql delete from iitree where
		treeid1 = :id7 and treeid2 = :id8;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql delete from iiqrytext where 
		txtid1 = :id5 and txtid2 = :id6;
            close_db();

            /* This test is part of test 8. Tests for rules
            ** for referential constraints.
            */
            SIprintf ("....iiintegrity test 8A..\n");
            SIflush(stdout);

            /*************/
            /** test 8A **/
            /*************/

	    stat_type = DBR_S_INSERT;
	
            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int8a(a i4 not null unique);
            exec sql create table duve_int8ar(b i4 references
                        duve_int8a(a));
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int8ar';
	    exec sql select rule_treeid1, rule_treeid2, rule_qryid1, rule_qryid2
		into :id7, :id8, :id9, :id10
		from iirule where rule_tabbase = :id1 and rule_tabidx = :id2
		and rule_statement = :stat_type;
            exec sql delete from iirule where rule_tabbase = :id1 and
			rule_tabidx = :id2 and rule_statement = :stat_type;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id5, :id6
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int8a;
            exec sql drop table duve_int8ar;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iitree where
                    treeid1 = :id7 and treeid2 = :id8;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            exec sql delete from iiqrytext where
                txtid1 = :id9 and txtid2 = :id10;
            close_db();
        }
        if ( run_test(IIINTEGRITY, 9) )
        {
            SIprintf ("....iiintegrity test 9..\n");
            SIflush(stdout);

            /*************/
            /** test 9  **/
            /*************/

	    dep_type = DB_DBP;
	    indep_type = DB_CONS;

            /* Tests for procedures for referential constraints */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int9(a i4 not null unique);
            exec sql create table duve_int9r(b i4 REFERENCES
                                        duve_int9(a));
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int9r';
            exec sql select deid1, deid2, dbp_txtid1, dbp_txtid2 
			into :id3, :id4, :id5, :id6 
			from iidbdepends, iiprocedure
                        where inid1 = :id1 and inid2 = :id2 and
                        dtype = :dep_type and itype = :indep_type
			and deid1 = dbp_id and deid2 = dbp_idx
			and dbp_parameter_count = 2;
	    exec sql select rule_qryid1, rule_qryid2, rule_treeid1, rule_treeid2
		into :id7, :id8, :id9, :id10
		from iirule, iiprocedure
		where rule_dbp_name = dbp_name
		and dbp_id = :id3
		and dbp_idx = :id4;
            exec sql delete from iiprocedure where dbp_id = :id3 and
                dbp_idx = :id4;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int9;
            exec sql drop table duve_int9r;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iitree where
                    treeid1 = :id9 and treeid2 = :id10;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            exec sql delete from iiqrytext where
                txtid1 = :id7 and txtid2 = :id8;
            exec sql delete from iiqrytext where
                txtid1 = :id5 and txtid2 = :id6;
            exec sql delete from iirule where
                        rule_tabbase = :id1 and rule_tabidx = :id2;
            exec sql delete from iikey where
                        key_consid1 = :id3 and
                        key_consid2 = :id4;
            close_db();
        }
        if ( run_test(IIINTEGRITY, 10) )
        {
            SIprintf ("....iiintegrity test 10..\n");
            SIflush(stdout);

            /*************/
            /** test 10  **/
            /*************/

            /* Tests for indexes for UNIQUE constraint */

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_int10(a i4 not null unique);
            exec sql select reltid,reltidx into :id1, :id2 from iirelation
                    where relid = 'duve_int10';
            exec sql delete from iiindex where baseid = :id1;
            exec sql select inttreeid1, inttreeid2, intqryid1, intqryid2,
                consid1, consid2 into :treeid1, :treeid2, :qryid1, :qryid2,
                        :id3, :id4
                    from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_int10;
            exec sql delete from iiintegrity
                    where inttabbase = :id1 and inttabidx = :id2;
            exec sql delete from iitree where
                    treeid1 = :treeid1 and treeid2 = :treeid2;
            exec sql delete from iiqrytext where
                txtid1 = :qryid1 and txtid2 = :qryid2;
            exec sql delete from iikey where
                key_consid1 = :id3 and
                key_consid2 = :id4;
            close_db();
        }
    } while (0);  /* control loop */
}

/* ck_iikey -
**
** Description:
**
** This routine tests the verifydb iikey catalog checks
**
** Inputs:
**      None.
** Outputs:
**      global variable: test_status    - status of tests: TEST_OK, TEST_ABORT
** History:
**      23-nov-93 (anitap)
**          initial creation for FIPS constraints and CREATE SCHEMA projects.
*/
ck_iikey()
{
    EXEC SQL begin declare section;
        i4 id1;
        i4 id2;
        i4 id3;
        i4 id4;
	i4 fakeid;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks for catalog iikey..\n");
    SIflush(stdout);
    STcopy("iikey", test_id);
    do
    {
        if (run_test(IIKEY, 1))
        {
           SIprintf ("....iikey test 1..\n");
           SIflush(stdout);

           /*************/
           /** test 1  **/
           /*************/

           /** setup **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql create table duve_key1(a i4 not null unique);
           exec sql select reltid, reltidx into :id1, :id2
                from iirelation where
                relid = 'duve_key1';
           exec sql select consid1, consid2 into :id3, :id4
                from iiintegrity
                where inttabbase = :id1 and
                        inttabidx = :id2;
	   fakeid = id3 + 1;
           exec sql update iikey set key_consid1 = :fakeid
		where key_consid1 = :id3
		and key_consid2 = :id4;
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql delete from iikey where key_consid1 = :fakeid and
                key_consid2 = :id4;
           exec sql drop table duve_key1;
           close_db();
        }

    } while (0);  /* control loop */
}

/*
** ck_iilocations - 
**
** Description:
**
**  This routine tests the verifydb iilocations catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iilocations()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iilocations..\n");
    SIflush(stdout);
    STcopy("iilocations",test_id);
    do
    {
	if ( run_test(IILOCATIONS, 1) )
	{
	    SIprintf ("....iilocations test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iilocations values
		(8,'_bad!loc','full_pathname_goes_here');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iilocations where lname = '_bad!loc';
	    close_db();
	}

	if ( run_test(IILOCATIONS, 2) )
	{
	    SIprintf ("....iilocations test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iilocations values
		(-1,'fake_loc_2','full_pathname_goes_here');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iilocations where lname = 'fake_loc_2';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iixprocedure - exercise IIXPROCEDURE tests
**
** Description:
**
**  This routine tests the verifydb iixprocedure catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	27-jan-94 (andre)
**	    written
*/
ck_iixprocedure()
{
    EXEC SQL begin declare section;
	i4 	id1;
	i4 	val;
	u_i4	tid;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iixprocedure..\n");
    SIflush(stdout);
    STcopy("iixprocedure",test_id);
    do
    {
	if ( run_test(IIXPROCEDURE, 1) )
	{
	    SIprintf ("....iixprocedure test 1..\n");
	    SIflush(stdout);

#if	0
	    /* 
	    ** skip this test - cleanup gets messy - dropping the index may 
	    ** result in a deadlock (it is used in qeu_d_cascade()) and cursor 
	    ** delete fails due to insufficient privileges - both can be 
	    ** addressed in BE, but not now
	    */

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;

	    /* 
	    ** insert into IIXPROCEDURE a tuple for which there should be no 
	    ** match in IIPROCEDURE
	    */
	    exec sql insert into iixprocedure 
		(dbp_id, dbp_idx, tidp)
		values (0, 0, 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iixprocedure 
		where dbp_id = 0 and dbp_idx = 0;
	    close_db();
#else
	    SIprintf ("this test is currently skipped.\n");
	    SIflush(stdout);
#endif
	}

	if ( run_test(IIXPROCEDURE, 2) )
	{
	    SIprintf ("....iixprocedure test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;

	    /*
	    ** create a dbproc, then delete the newly added IIXPROCEDURE tuple
	    */
	    exec sql drop procedure duve_iixprocedure2;
	    exec sql create procedure duve_iixprocedure2 as begin return; end;
	    exec sql select tid, dbp_id
		into :tid, :id1 from iiprocedure
		where
			dbp_owner = user
		    and dbp_name = 'duve_iixprocedure2';
	    exec sql delete from iixprocedure where tidp = :tid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_A9db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select count(*) into :val
		from iixprocedure
		where
			dbp_id = :id1
		    and dbp_idx = 0;
	    
	    if (val == 0)
	    {
		exec sql insert into iixprocedure
		    (dbp_id, dbp_idx, tidp)
		    values (:id1, 0, :tid);
	    }

	    exec sql drop procedure duve_iixprocedure2;

	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iiprocedure - 
**
** Description:
**
**  This routine tests the verifydb iiproccedure catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      23-nov-93 (anitap)
**          Added test 3 to check schema is created for orphaned procedure.
**          Also added test 4 to check that iidbdepends tuples are present
**          for procedures for referential constraints.
**	28-jan-94 (andre)
**	    added tests 5-8
**	29-jan-94 (andre)
**	    updated cleanup for tests 1, 2, and 4 since verifydb deletes 
**	    IIPROCEDURE tuple for the dbproc.
**	02-feb-94 (anitap)
**	    updated cleanup for tests 1,2 and 4 to drop the procedures.
*/
ck_iiprocedure()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 id3;
	i4 id4;
	i4 id5;
	i4 id6;
	i4 dbp_type;
	i4 dbp_mask, dep_type, indep_type;
    EXEC SQL end declare section;

    dbp_type = DB_DBP;

    SIprintf ("..Testing verifydb checks/repairs for catalog iiprocedure..\n");
    SIflush(stdout);
    STcopy("iiprocedure",test_id);
    do
    {
	if ( run_test(IIPROCEDURE, 1) )
	{
	    SIprintf ("....iiprocedure test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create procedure duve_proc1 as
		begin
		    message 'hi';
		end;
	    exec sql select dbp_txtid1,dbp_txtid2 into :id1, :id2
		from iiprocedure
		where dbp_name = 'duve_proc1';
	    exec sql update iiprocedure set dbp_txtid1 = 123, dbp_txtid2 = 456
		where dbp_name = 'duve_proc1';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiqrytext
		where txtid1 = :id1 and txtid2 = :id2;
	    close_db();
	}

	if ( run_test(IIPROCEDURE, 2) )
	{
	    SIprintf ("....iiprocedure test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create procedure duve_proc2 as
		begin
		    message 'hi';
		end;
	    exec sql create table duve_prottbl (a i4);
	    exec sql select dbp_txtid1,dbp_txtid2, dbp_id 
		into :id1, :id2, :id3 from iiprocedure
		where dbp_name = 'duve_proc2';
	    exec sql select reltid into :id4 from iirelation
		where relid = 'duve_prottbl';
	    exec sql update iiprocedure set dbp_id = :id4
                where dbp_name = 'duve_proc2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiqrytext
		where txtid1 = :id1 and txtid2 = :id2;
	    exec sql drop table duve_prottbl;
	    close_db();
	}

        if ( run_test(IIPROCEDURE, 3) )
        {
            SIprintf ("....iiprocedure test 3..\n");
            SIflush(stdout);

            /*************/
            /** test 3  **/
            /*************/

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
               break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql create procedure duve_proc3 as
                begin
                    message 'hi';
                end;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql drop procedure duve_proc3;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();
        }
        if ( run_test(IIPROCEDURE, 4) )
        {
            SIprintf ("....iiprocedure test 4..\n");
            SIflush(stdout);

            /*************/
            /** test 4  **/
            /*************/

	    dep_type = DB_DBP;
	    indep_type = DB_CONS;

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_proc (a i4 not null unique);
            exec sql create table duve_procr(b i4 references
                                duve_proc(a));
            exec sql select reltid, reltidx into :id1, :id2 
		from iirelation
                where relid = 'duve_procr';
            exec sql select deid1, deid2, dbp_txtid1, dbp_txtid2 
			into :id3, :id4, :id5, :id6 
			from iidbdepends, iiprocedure
                        where inid1 = :id1 and inid2 = :id2 and
                        dtype = :dep_type and itype = :indep_type
			and deid1 = dbp_id and deid2 = dbp_idx
			and dbp_parameter_count = 2;
            exec sql delete from iidbdepends where inid1 = :id1
			and inid2 = :id2
                        and dtype = :dep_type
			and deid1 = :id3
			and deid2 = :id4;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_proc;
            exec sql drop table duve_procr;
            exec sql delete from iiqrytext where
                txtid1 = :id5 and txtid2 = :id6;
            close_db();
        }

	if ( run_test(IIPROCEDURE, 5) )
	{
	    SIprintf ("....iiprocedure test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a dbproc dependent on SELECT privilege on iirelation, then
	    ** delete the IIPRIV tuple representing the privilege descriptor
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc5;

	    exec sql create procedure duve_proc5 as
		declare cnt i;
		begin
		    select count(*) into :cnt from iirelation;
		end;

	    exec sql delete from iipriv 
		where 
		    exists (select 1 
				from iiprocedure
			    	where
					dbp_name = 'duve_proc5'
				    and dbp_owner = user
				    and d_obj_id = dbp_id
				    and d_obj_idx = dbp_idx
				    and d_priv_number = 0
				    and d_obj_type = :dbp_type);
			
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc5;

	    close_db();
	}

	if ( run_test(IIPROCEDURE, 6) )
	{
	    SIprintf ("....iiprocedure test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a dbproc dependent on SELECT privilege on iirelation and 
	    ** on user's table, then update dbp_mask1 to indicate that there 
	    ** exists no independent object/privilege list
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc6;
	    exec sql drop duve_proc6_tbl1;

	    exec sql create table duve_proc6_tbl1 (i i);

	    exec sql create procedure duve_proc6 as
		begin
		    insert into duve_proc6_tbl1
			select count(*) from iirelation;
		end;

	    exec sql select dbp_mask1 into :dbp_mask
		from iiprocedure
		where dbp_name = 'duve_proc6' and dbp_owner = user;
	    
	    dbp_mask &= ~DB_DBP_INDEP_LIST;

	    exec sql update iiprocedure 
		set dbp_mask1 = :dbp_mask
		where dbp_name = 'duve_proc6' and dbp_owner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc6;
	    exec sql drop duve_proc6_tbl1;

	    close_db();
	}

	if ( run_test(IIPROCEDURE, 7) )
	{
	    SIprintf ("....iiprocedure test 7..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 7  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a dbproc dependent on SELECT privilege on iirelation, 
	    ** then update (dbp_ubt_id, dbp_ubt_idx) with (reltid, reltidx) of 
	    ** IIRELATION
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc7;

	    exec sql create procedure duve_proc7 as
		declare cnt i;
		begin
		    select count(*) into :cnt from iirelation;
		end;

	    exec sql update iiprocedure 
		set dbp_ubt_id = 1, dbp_ubt_idx = 0
		where dbp_name = 'duve_proc7' and dbp_owner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc7;

	    close_db();
	}

	if ( run_test(IIPROCEDURE, 8) )
	{
	    SIprintf ("....iiprocedure test 8..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 8  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a dbproc dependent on user's table, grant a permit on it,
	    ** then update dbp_mask1 to indicate that the dbproc is not 
	    ** grantable
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc8;
	    exec sql drop duve_proc8_tbl1;

	    exec sql create table duve_proc8_tbl1 (i i);

	    exec sql create procedure duve_proc8 as
		begin
		    insert into duve_proc8_tbl1 values(0);
		end;

	    exec sql grant all on procedure duve_proc8 to public;

	    exec sql select dbp_mask1 into :dbp_mask
		from iiprocedure
		where dbp_name = 'duve_proc8' and dbp_owner = user;
	    
	    dbp_mask &= ~DB_DBPGRANT_OK;

	    exec sql update iiprocedure 
		set dbp_mask1 = :dbp_mask
		where dbp_name = 'duve_proc8' and dbp_owner = user;
	    
	    exec sql select proqryid1, proqryid2 into :id1, :id2
		from iiprotect
		where probjname='duve_proc8' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop procedure duve_proc8;
	    exec sql delete from iiqrytext
		where txtid1 = :id1 and txtid2 = :id2;
	    exec sql drop duve_proc8_tbl1;

	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iiprotect - 
**
** Description:
**
**  This routine tests the verifydb iiprotect catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	11-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	10-nov-93 (anitap)
**	    Corrected inttabbase to protabbase in tests 3 & 4.
**	28-jan-94 (andre)
**	    added tests 5-12
*/
ck_iiprotect()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
	i4 qryid1, qryid2, treeid1, treeid2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiprotect..\n");
    SIflush(stdout);
    STcopy("iiprotect",test_id);
    do
    {
	if ( run_test(IIPROTECT, 1) )
	{
	    SIprintf ("....iiprotect test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_pro1(i i);
	    exec sql grant select on duve_pro1 to public;
	    exec sql select proqryid1, proqryid2 into :qryid1, :qryid2
		from iiprotect
		where probjname = 'duve_pro1' and probjowner = user;
	    exec sql update iiprotect
		set protabbase = 31, protabidx = 31
		where probjname = 'duve_pro1' and probjowner = user;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiqrytext 
		where txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql drop duve_pro1;
	    close_db();
	}

	if ( run_test(IIPROTECT, 2) )
	{
	    SIprintf ("....iiprotect test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_pro2(a i4, b i4);
	    exec sql grant select on duve_pro2 to '$ingres';
	    exec sql select relstat into :val from iirelation
		    where relid = 'duve_pro2';
	    val &= ~TCB_PRTUPS;
	    exec sql update iirelation set relstat = :val 	
		    where relid = 'duve_pro2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val |= TCB_PRTUPS;
	    exec sql update iirelation set relstat = :val 	
		    where relid = 'duve_pro2';
	    exec sql drop table duve_pro2;
	    close_db();
	}

	if ( run_test(IIPROTECT, 3) )
	{
	    SIprintf ("....iiprotect test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_pro3(a i4, b i4);
	    exec sql grant select on duve_pro3 to '$ingres';
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_pro3';
	    exec sql select protreeid1, protreeid2, proqryid1, proqryid2
		    into :treeid1, :treeid2, :qryid1, :qryid2 from iiprotect
		    where protabbase = :id1 and protabidx = :id2;
	    exec sql update iiprotect set protreeid1 = 321 
		    where protabbase = :id1 and protabidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiprotect
		    where protabbase = :id1 and protabidx = :id2;
	    exec sql delete from iitree where
		    treeid1 = :treeid1 and treeid2 = :treeid2;
	    exec sql delete from iiqrytext where
		    txtid1 = :qryid1 and txtid2 = :qryid2;
	    exec sql drop table duve_pro2;
	    close_db();
	}

	if ( run_test(IIPROTECT, 4) )
	{
	    SIprintf ("....iiprotect test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_pro3(a i4, b i4);
	    exec sql grant select on duve_pro3 to '$ingres';
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_pro3';
	    exec sql select protreeid1, protreeid2, proqryid1, proqryid2
		    into :treeid1, :treeid2, :qryid1, :qryid2 from iiprotect
		    where protabbase = :id1 and protabidx = :id2;
	    exec sql delete from iiqrytext where
		    txtid1 = :qryid1 and txtid2 = :qryid2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiprotect
		    where protabbase = :id1 and protabidx = :id2;
	    exec sql delete from iitree where
		    treeid1 = :treeid1 and treeid2 = :treeid2;
	    exec sql drop table duve_pro3;
	    close_db();	
	}	

	if ( run_test(IIPROTECT, 5) )
	{
	    SIprintf ("....iiprotect test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite probjname
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro5;
	    exec sql create table duve_pro5(i i);
	    exec sql grant select on duve_pro5 to public;
	    exec sql update iiprotect 
		set probjname='gibberish' 
		where probjname = 'duve_pro5' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro5;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 6) )
	{
	    SIprintf ("....iiprotect test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite proopset
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    val = DB_EXECUTE;

	    exec sql drop duve_pro6;
	    exec sql create table duve_pro6(i i);
	    exec sql grant select on duve_pro6 to public;
	    exec sql select proqryid1, proqryid2 into :id1, :id2
		from iiprotect
		where probjname = 'duve_pro6' and probjowner = user;
	    exec sql update iiprotect 
		set proopset = :val
		where probjname = 'duve_pro6' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql delete from iiprotect 
		where probjname = 'duve_pro6' and probjowner = user;
	    exec sql delete from iiqrytext 
		where txtid1 = :id1 and txtid2 = :id2;
	    exec sql drop duve_pro6;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 7) )
	{
	    SIprintf ("....iiprotect test 7..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 7  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite proopctl
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    val = DB_EXECUTE;

	    exec sql drop duve_pro7;
	    exec sql create table duve_pro7(i i);
	    exec sql grant select on duve_pro7 to public;
	    exec sql select proqryid1, proqryid2 into :id1, :id2
		from iiprotect
		where probjname = 'duve_pro7' and probjowner = user;
	    exec sql update iiprotect 
		set proopctl = :val
		where probjname = 'duve_pro7' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql delete from iiprotect 
		where probjname = 'duve_pro7' and probjowner = user;
	    exec sql delete from iiqrytext 
		where txtid1 = :id1 and txtid2 = :id2;
	    exec sql drop duve_pro7;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 8) )
	{
	    SIprintf ("....iiprotect test 8..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 8  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite 
	    ** IIQRYTEXT. status to make this look like a permit created 
	    ** using QUEL
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro8;
	    exec sql create table duve_pro8(i i);
	    exec sql grant select on duve_pro8 to public;
	    exec sql select iiqrytext.status into :val 
		from iiqrytext, iiprotect
		where
			probjname = 'duve_pro8'
		    and probjowner = user
		    and txtid1 = proqryid1
		    and txtid2 = proqryid2;

	    val &= ~DBQ_SQL;

	    exec sql update iiqrytext from iiprotect
		set status = :val
		where
			probjname = 'duve_pro8'
		    and probjowner = user
		    and txtid1 = proqryid1
		    and txtid2 = proqryid2;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro8;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 9) )
	{
	    SIprintf ("....iiprotect test 9..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 9  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite 
	    ** IIPROTECT.proflags to make this look like a permit created using 
	    ** QUEL
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro9;
	    exec sql create table duve_pro9(i i);
	    exec sql grant select on duve_pro9 to public;
	    exec sql select proflags into :val 
		from iiprotect
		where probjname = 'duve_pro9' and probjowner = user;

	    val &= ~DBP_SQL_PERM;

	    exec sql update iiprotect
		set proflags = :val
		where probjname = 'duve_pro9' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro9;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 10) )
	{
	    SIprintf ("....iiprotect test 10..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 10  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite 
	    ** IIPROTECT.proopset to make this look like a permit conveys
	    ** multiple privileges
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro10;
	    exec sql create table duve_pro10(i i);
	    exec sql grant select on duve_pro10 to public;
	    exec sql select proopset into :val 
		from iiprotect
		where probjname = 'duve_pro10' and probjowner = user;

	    val |= DB_REPLACE;

	    exec sql update iiprotect
		set proopset = :val
		where probjname = 'duve_pro10' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro10;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 11) )
	{
	    SIprintf ("....iiprotect test 11..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 11  **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a table, grant a permit on it, then overwrite 
	    ** IIPROTECT.proflags to make this look like a permit was created 
	    ** prior to 6.5
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro11;
	    exec sql create table duve_pro11(i i);
	    exec sql grant select on duve_pro11 to public;
	    exec sql select proflags into :val 
		from iiprotect
		where probjname = 'duve_pro11' and probjowner = user;

	    val &= ~DBP_65_PLUS_PERM;

	    exec sql update iiprotect
		set proflags = :val
		where probjname = 'duve_pro11' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_pro11;

	    close_db();	
	}	

	if ( run_test(IIPROTECT, 12) )
	{
	    SIprintf ("....iiprotect test 12..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 12  **/
	    /*************/

	    /** setup **/

	    /*
	    ** as user SUPER, create a view over IIRELATION (on which everyone 
	    ** has SELECT WGO), grant SELECT on that view, then overwrite 
	    ** prograntor to make it look like as if the permit has been granted
	    ** by a user who does not own the view and does not possess
	    ** GRANT OPTION FOR privilege being conveyed by the permit (i.e.
	    ** SELECT)
	    */

	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql commit;
	    exec sql set session authorization super;
	    exec sql create view duve_pro12 as select * from iirelation;
	    exec sql grant select on duve_pro12 to public;

	    /*
	    ** remember query text id for both the view and the permit since by
	    ** the time verifydb is done with them, the IIQRYTEXT tuples will be
	    ** the only thing left of them
	    */
	    exec sql select relqid1, relqid2 into :id1, :id2
		from iirelation
		where relid='duve_pro12' and relowner=user;
	    exec sql select proqryid1, proqryid2 into :qryid1, :qryid2
		from iiprotect
		where probjname='duve_pro12' and probjowner=user;

	    exec sql update iiprotect
		set prograntor = '$ingres'
		where probjname = 'duve_pro12' and probjowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql delete from iiqrytext 
		where (txtid1 = :id1 and txtid2 = :id2) or
		      (txtid1 = :qryid1 and txtid2 = :qryid2);

	    close_db();	
	}	

    } while (0);  /* control loop */
}
/*
** ck_iiqrytext - 
**
** Description:
**
**  This routine tests the verifydb iiqrytext catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	11-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iiqrytext()
{
    EXEC SQL begin declare section;
	i4 id1,id2;
	i4 id1_a,id2_a;
	i4 id1_b,id2_b;
	i4 id1_c,id2_c;
	i4 id1_d,id2_d;
	i4 id1_e,id2_e;
	i4 qry1, qry2;
	i4 qry1_a, qry2_a;
	i4 qry1_b, qry2_b;
	i4 qry1_c, qry2_c;
	i4 qry1_d, qry2_d;
	i4 qry1_e, qry2_e;
	i4 val, val2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiqrytext..\n");
    SIflush(stdout);
    STcopy("iiqrytext",test_id);
    do
    {
	if ( run_test(IIQRYTEXT, 1) )
	{
	    SIprintf ("....iiqrytext test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiqrytext values (0,0,17,0,0,'dummy entry');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiqrytext where txtid1=0 and txtid2=0;
	    close_db();
	}

	if ( run_test(IIQRYTEXT, 3) || run_test(IIQRYTEXT, 4) ||
	     run_test(IIQRYTEXT, 5) || run_test(IIQRYTEXT, 7) ||
	     run_test(IIQRYTEXT, 8) || run_test(IIQRYTEXT, 9) )
	{
	    SIprintf ("....iiqrytext tests 3,4,5,7,8,9..\n");
	    SIflush(stdout);

	    /********************/
	    /** tests 3-5, 7-9 **/
	    /********************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_qrytab(a i4, b i4, c i4);
	    exec sql create view duve_qryview_a as select a,b from duve_qrytab;
	    exec sql create view duve_qryview_b as select a,b from duve_qrytab;
	    exec sql create view duve_qryview_c as select a,b from duve_qrytab;
	    exec sql create view duve_qryview_d as select a,b from duve_qrytab;
	    exec sql create view duve_qryview_e as select a,b from duve_qrytab;
	    exec sql grant select on duve_qrytab to '$ingres';

	    exec sql select reltid, reltidx, relqid1, relqid2
		    into :id1_a,:id2_a,:qry1_a,:qry2_a
		    from iirelation where
		    relid = 'duve_qryview_a';
	    exec sql select reltid, reltidx, relqid1, relqid2
		    into :id1_b,:id2_b,:qry1_b,:qry2_b
		    from iirelation where
		    relid = 'duve_qryview_b';
	    exec sql select reltid, reltidx, relqid1, relqid2
		    into :id1_c,:id2_c,:qry1_c,:qry2_c
		    from iirelation where
		    relid = 'duve_qryview_c';
	    exec sql select reltid, reltidx, relqid1, relqid2
		    into :id1_d,:id2_d,:qry1_d,:qry2_d
		    from iirelation where
		    relid = 'duve_qryview_d';
	    exec sql select reltid, reltidx, relqid1, relqid2
		    into :id1_e,:id2_e,:qry1_e,:qry2_e
		    from iirelation where
		    relid = 'duve_qryview_e';
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation where
		    relid = 'duve_qrytab';
	    exec sql select proqryid1,proqryid2 into :qry1, :qry2 from iiprotect
		    where protabbase = :id1 and protabidx = :id2;
	    exec sql update iiqrytext set mode = 19 where
		    txtid1 = :qry1_a and txtid2 = :qry2_a;
	    exec sql update iiqrytext set mode = 20 where
		    txtid1 = :qry1_b and txtid2 = :qry2_b;
	    exec sql update iiqrytext set mode = 33 where
		    txtid1 = :qry1_c and txtid2 = :qry2_c;
	    exec sql update iiqrytext set mode = 87 where
		    txtid1 = :qry1_d and txtid2 = :qry2_d;
	    exec sql update iiqrytext set mode = 110 where
		    txtid1 = :qry1_e and txtid2 = :qry2_e;	
	    exec sql update iiqrytext set mode = 17 where 
		    txtid1 = :qry1 and txtid2 = :qry2;	
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iiqrytext set mode = 17 where
		    txtid1 = :qry1_a and txtid2 = :qry2_a;
	    exec sql update iiqrytext set mode = 17 where
		    txtid1 = :qry1_b and txtid2 = :qry2_b;
	    exec sql update iiqrytext set mode = 17 where
		    txtid1 = :qry1_c and txtid2 = :qry2_c;
	    exec sql update iiqrytext set mode = 17 where
		    txtid1 = :qry1_d and txtid2 = :qry2_d;
	    exec sql update iiqrytext set mode = 17 where
		    txtid1 = :qry1_e and txtid2 = :qry2_e;	
	    exec sql update iiqrytext set mode = 19 where 
		    txtid1 = :qry1 and txtid2 = :qry2;
	    exec sql drop view duve_qryview_a;
	    exec sql drop view duve_qryview_b;
	    exec sql drop view duve_qryview_c;
	    exec sql drop view duve_qryview_d;
	    exec sql drop view duve_qryview_e;
	    exec sql drop table duve_qrytab;
	    close_db();
	}

	if ( run_test(IIQRYTEXT, 2) )
	{
	    SIprintf ("....iiqrytext test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_qrytab2(a i4, b i4);
	    exec sql create view duve_qryview as select * from duve_qrytab2;
	    exec sql select reltid,reltidx,relqid1,relqid2
		    into :id1,:id2,:qry1,:qry2 
		    from iirelation where relid = 'duve_qryview';
	    exec sql update iiqrytext set mode = 11
		    where txtid1 =: qry1 and txtid2 = :qry2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iiqrytext set mode = 17
		    where txtid1 =: qry1 and txtid2 =: qry2;
	    exec sql drop view duve_qryview;
	    exec sql drop table duve_qrytab2;
	    close_db();
	}

	if ( run_test(IIQRYTEXT, 6) )
	{
	    SIprintf ("....iiqrytext test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_qrytab6(a i4, b i4);
	    exec sql create view duve_qryview as select * from duve_qrytab6;
	    exec sql select reltid,reltidx,relqid1,relqid2
		    into :id1,:id2,:qry1,:qry2 
		    from iirelation where relid = 'duve_qryview';
	    exec sql update iiqrytext set seq = 1
		    where txtid1 =: qry1 and txtid2 =: qry2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iiqrytext set seq = 0
		    where txtid1 =: qry1 and txtid2 =: qry2;
	    exec sql drop view duve_qryview;
	    exec sql drop table duve_qrytab6;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iirelation - 
**
** Description:
**
**  This routine tests the verifydb iirelation catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	4-aug-93 (stephenb)
**	    added test #29 to test that a row exists in iigw06_relation
**	    if we have a security audit gateway table.
**      23-nov-93 (anitap)
**          added test #28 to test that schema is created for orphaned
**          table.
**	28-jan-94 (andre)
**	    added tests 30 and 31
*/
ck_iirelation()
{
    EXEC SQL begin declare section;
	i4	id1, id2;
	i4	val, val2;
	i4	txt1, txt2;
	char	charval[DB_MAXNAME+1];
	i4	reltid;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iirelation..\n");
    SIflush(stdout);
    STcopy("iirelation",test_id);
    do
    {

#if 0
/* this test currently causes an AV in server due to a DMF bug ...
*/
	if ( run_test(IIRELATION,1) )
	{
	    SIprintf ("....iirelation test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_01);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set reltid = 0 where relid = 'iiintegrity';
	    exec sql insert into iirelation (reltid,reltidx,relid) values
		(0,0,'duve_rel1');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirelation where relid = 'duve_rel1';
	    exec sql update iirelation set reltid = 23 where relid = 'iiintegrity';
	    close_db();
	}
#endif

	if ( run_test(IIRELATION,2) )
	{
	    SIprintf ("....iirelation test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2a **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_02);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel2a(a i4, b i4, c i4);
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_rel2a';
	    exec sql update iiattribute set attid = 4 where attid=3 and
		    attrelid = :id1 and attrelidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
 		break;
	    exec sql update iiattribute set attid = 3 where attid=4 and
		    attrelid = :id1 and attrelidx = :id2;
	    exec sql drop table duve_rel2a;
	    close_db();

	    /*************/
	    /** test 2b **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel2b (a i4, b i4);
	    exec sql select relwid into :val from iirelation
		    where relid = 'duve_rel2b';
	    val += 20;
	    exec sql update iirelation set relwid = :val where relid = 'duve_rel2b';
	    val -= 20;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set relwid = :val where relid = 'duve_rel2b';
	    exec sql drop table duve_rel2b;
	    close_db();
	}

	if ( run_test(IIRELATION,3) )
	{
	    SIprintf ("....iirelation test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_03);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel3(a i4, b i4, c i4);
	    exec sql create index i_duve_rel3 on duve_rel3(a, b);
	    exec sql update iirelation set relkeys = 4
		where relid = 'i_duve_rel3';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set relkeys = 3
		where relid = 'i_duve_rel3';
	    exec sql drop index i_duve_rel3;
	    exec sql drop table duve_rel3;
	    close_db();
	}

	if ( run_test(IIRELATION,4) )
	{
	    SIprintf ("....iirelation test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_04);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel4(a i4);
	    exec sql select relspec into :val from iirelation where
		relid = 'duve_rel4';
	    exec sql update iirelation set relspec = 500
		where relid = 'duve_rel4';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set relspec = :val
		where relid = 'duve_rel4';
	    exec sql drop table duve_rel4;
	    close_db();
	}

	if ( run_test(IIRELATION,5) )
	{
	    SIprintf ("....iirelation test 5..\n");
	    SIflush(stdout);

	    /*********************/
	    /** tests 5a and 5b **/
	    /*********************/

	    /** setup **/
	    name_log(REL_CODE,ID_05);
	    open_db_as_user("$ingres");
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_5a1(a i4);
	    exec sql create table iiduve_5a2(a i4);
	    exec sql select relstat into :val from iirelation
		where relid = 'duve_5a1';
	    exec sql select relstat into :val2 from iirelation
		where relid = 'iiduve_5a2';
	    exec sql update iirelation set relstat = :val
		where relid= 'iiduve_5a2';
	    exec sql update iirelation set relstat = :val2
		where relid= 'duve_5a1';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db_as_user("$ingres");
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop duve_5a1;
	    exec sql drop iiduve_5a2;
	    close_db();
	}

	if ( run_test(IIRELATION,6) )
	{
	    SIprintf ("....iirelation test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_06);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel6(a i4, b i4);
	    exec sql create index i_duve_rel6 on duve_rel6(a);
	    exec sql select relstat into :val from iirelation
		    where relid = 'duve_rel6';
	    exec sql select relstat,reltid,reltidx
		    into :val2,:id1,:id2 from iirelation
		    where relid = 'i_duve_rel6';
	    val |= TCB_INDEX;
	    val2 &= ~TCB_INDEX;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel6';
	    exec sql update iirelation set relstat=:val2
		where relid ='i_duve_rel6';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val &= ~TCB_INDEX;
	    val2 |= TCB_INDEX;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel6';
	    exec sql update iirelation set relstat=:val2
		where relid ='i_duve_rel6';
	    exec sql drop index i_duve_rel6;
	    exec sql drop table duve_rel6;
	    exec sql delete from iiindex where baseid=:id1;
	    exec sql delete from iirelation where reltid = :id1;
	    exec sql delete from iiattribute where attrelid = :id1;
	    close_db();
	}

	if ( run_test(IIRELATION,7) )
	{
	    SIprintf ("....iirelation test 7..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 7  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_07);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel7(a i4, b i4);
	    exec sql select reltid,relstat into :id1,:val from iirelation
		    where relid = 'duve_rel7';
	    id2 = id1+1;
	    val |= TCB_INDEX;
	    exec sql insert into iirelation select :id1,:id2,'i_duve_rel7',
	        relowner,relatts,relwid,relkeys,relspec,:val,reltups,relpages,
		relprim,relmain,relsave,relstamp1,relstamp2,relloc,relcmptlvl,
		relcreate, relqid1,relqid2,relmoddate,relidxcount,relifill,
		reldfill, rellfill,relmin,relmax,relloccount,relgwid,relgwother,
		relhigh_logkey,rellow_logkey,relfhdr,relallocation,relextend,
		relcomptype,relpgtype,relstat2,relfree from iirelation
		where relid = 'duve_rel7';
	    exec sql insert into iiattribute select :id1, :id2,
		attid, attxtra, attname, attoff, attfrmt, attfrml, attfrmp,
		attkdom, attflag, attdefid1, attdefid2 from iiattribute
		where attrelid = :id1;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiindex where baseid = :id1;
	    exec sql delete from iirelation where reltid = :id1;
	    exec sql delete from iiattribute where attrelid = :id1;
	    close_db();
	}

	if ( run_test(IIRELATION,8) )
	{
	    SIprintf ("....iirelation test 8..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 8  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_08);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel8(a i4, b i4);
	    exec sql select relstat into :val from iirelation
		    where relid = 'duve_rel8';
	    val |= TCB_PRTUPS;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel8';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_rel8;
	    close_db();
	}

	if ( run_test(IIRELATION,9) )
	{
	    SIprintf ("....iirelation test 9..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 9  **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_09);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel9(a i4, b i4);
	    exec sql select relstat into :val from iirelation
		    where relid = 'duve_rel9';
	    val |= TCB_INTEGS;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel9';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_rel9;
	    close_db();
	}

	if ( run_test(IIRELATION,10) )
	{
	    SIprintf ("....iirelation test 10..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 10 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0A);
	    open_db_as_user("$ingres");
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table iiduve_rel10(a i4);
	    exec sql select relstat into :val from iirelation
		where relid = 'iiduve_rel10';
	    val |= TCB_CONCUR;
	    exec sql update iirelation set relstat = :val 
		where relid = 'iiduve_rel10';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val &= ~TCB_CONCUR;
	    exec sql update iirelation set relstat = :val 
		where relid = 'iiduve_rel10';
	    exec sql drop table iiduve_rel10;
	    close_db();
	}

	if ( run_test(IIRELATION,11) )
	{
	    SIprintf ("....iirelation test 11..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 11 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0B);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select relstat into :val from iirelation
		where relid = 'iidevices';
	    val &= ~TCB_CONCUR;
	    exec sql update iirelation set relstat = :val
		where relid = 'iidevices';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val |= TCB_CONCUR;
	    exec sql update iirelation set relstat = :val
		where relid = 'iidevices';
	    close_db();
	}

	if ( run_test(IIRELATION,12) )
	{
	    SIprintf ("....iirelation test 12..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 12 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0C);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel12(a i4);
	    exec sql create view v_duve_rel12 as select * from duve_rel12;
	    exec sql select reltid,reltidx,relqid1,relqid2
		into :id1,:id2,:val,:val2 from iirelation where
		relid = 'v_duve_rel12';
	    exec sql delete from iitree
		where treetabbase = :id1 and treetabidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiattribute
		where attrelid=:id1 and attrelidx = :id2;
	    exec sql delete from iirelation where reltid= :id1 and reltidx=:id2;
	    exec sql delete from iiqrytext where txtid1=:val and txtid2=:val2;
	    exec sql delete from iidbdepends where deid1=: id1 and deid2 =: id2;
	    exec sql delete from iitree
		where treetabbase = :id1 and treetabidx = :id2;
	    exec sql drop table duve_rel12;
	    close_db();
	}

	if ( run_test(IIRELATION,13) )
	{
	    SIprintf ("....iirelation test 13..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 13 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0D);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel13(a i4);
	    exec sql create view v_duve_rel13 as select * from duve_rel13;
	    exec sql select reltid, reltidx, relqid1, relqid2
		into :id1, :id2, :val, :val2 from iirelation
		    where relid = 'v_duve_rel13';
	    exec sql delete from iidbdepends where deid1=: id1 and deid2 =: id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iitree
		where treetabbase = :id1 and treetabidx = :id2;
	    exec sql delete from iiattribute
		where attrelid=:id1 and attrelidx = :id2;
	    exec sql delete from iirelation where reltid= :id1 and reltidx=:id2;
	    exec sql delete from iiqrytext where txtid1=:val and txtid2=:val2;
	    exec sql delete from iidbdepends where deid1=: id1 and deid2 =: id2;
	    exec sql drop table duve_rel13;
	    close_db();
	}

	if ( run_test(IIRELATION,14) )
	{
	    SIprintf ("....iirelation test 14..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 14 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0E);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel14(a i4);
	    exec sql create view v_duve_rel14 as select * from duve_rel14;
	    exec sql select reltid, reltidx, relqid1, relqid2
		into :id1, :id2, :val, :val2 from iirelation
		    where relid = 'v_duve_rel14';
	    exec sql delete from iiqrytext where txtid1=:val and txtid2=:val2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iitree
		where treetabbase = :id1 and treetabidx = :id2;
	    exec sql delete from iiattribute
		where attrelid=:id1 and attrelidx = :id2;
	    exec sql delete from iirelation where reltid= :id1 and reltidx=:id2;
	    exec sql delete from iiqrytext where txtid1=:val and txtid2=:val2;
	    exec sql delete from iidbdepends where deid1=: id1 and deid2 =: id2;
	    exec sql drop table duve_rel14;
	    close_db();
	}

	if ( run_test(IIRELATION,15) )
	{
	    SIprintf ("....iirelation test 15..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 15 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_0F);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel15(a i4);
	    exec sql select relstat into :val from iirelation
		where relid = 'duve_rel15';

	    /*
	    ** if we don't set the TCB_VGRANT_OK bit, test 31 will complain that
	    ** IIPRIV does not contain description of privileges on which this,
	    ** apparently not "always grantable" view, depends
	    */
	    val |= TCB_VIEW | TCB_VGRANT_OK;

	    exec sql update iirelation set relstat= :val
		where relid = 'duve_rel15';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val &= ~TCB_VIEW;
	    exec sql update iirelation set relstat= :val
		where relid = 'duve_rel15';
	    exec sql drop table duve_rel15;
	    close_db();
	}

	if ( run_test(IIRELATION,16) )
	{
	    SIprintf ("....iirelation test 16..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 16 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_10);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel16(a i4, tidp i4);
	    exec sql select reltid,relstat into :id1,:val from iirelation
		where relid = 'duve_rel16';
	    val |= TCB_INDEX;
	    id2 = id1 +1;
	    exec sql insert into iirelation select :id1,:id2,'i_duve_rel16',
	        relowner,relatts,relwid,relkeys,relspec,:val,reltups,relpages,
		relprim,relmain,relsave,relstamp1,relstamp2,relloc,relcmptlvl,
		relcreate, relqid1,relqid2,relmoddate,relidxcount,relifill,
		reldfill, rellfill,relmin,relmax,relloccount,relgwid,relgwother,
		relhigh_logkey,rellow_logkey,relfhdr,relallocation,relextend,
		relcomptype,relpgtype,relstat2,relfree from iirelation
		where relid = 'duve_rel16';
	    exec sql delete from iirelation where relid = 'duve_rel16';
	    exec sql insert into iiattribute select :id1,:id2,attid,attxtra,
		attname,attoff,attfrmt,attfrml,attfrmp,attkdom,attflag,
		attdefid1,attdefid2 from iiattribute
	    			where
			attrelid = :id1 and attrelidx = 0;
	    exec sql delete from iiattribute
		where attrelid=:id1 and attrelidx=0;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirelation where reltid = :id1;
	    exec sql delete from iiattribute where attrelid = :id1;
	    close_db();
	}

	if ( run_test(IIRELATION,17) )
	{
	    SIprintf ("....relation test 17..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 17 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_11);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel17(a i4, b i4);
	    exec sql create index i_duve_rel17 on duve_rel17(a);
	    exec sql select relstat into :val from iirelation
		where relid='duve_rel17';
	    val &= ~TCB_IDXD;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel17';
	    exec sql select reltid,reltidx,relqid1,relqid2
		into :id1, :id2, :txt1, :txt2 from iirelation
		where relid = 'i_duve_rel17';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val |= TCB_IDXD;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel17';
	    exec sql drop index duve_rel17;
	    exec sql drop table duve_rel17;
	    exec sql delete from iiindex where baseid = :id1;
	    exec sql delete from iiattribute where attrelid= :id1;
	    exec sql delete from iirelation where reltidx = :id1;
	    exec sql delete from iitree where treetabbase = :id1;
	    exec sql delete from iiqrytext where txtid1=:txt1 and txtid2=:txt2;
    	    close_db();
	}

	if ( run_test(IIRELATION,18) )
	{
	    SIprintf ("....iirelation test 18..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 18 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_12);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel18a(a  i4, b i4);
	    exec sql create table duve_rel18b(a  i4, b i4);
	    exec sql create index i_duve_rel18 on duve_rel18a (a);
	    exec sql select relidxcount into :val from iirelation
		where relid	= 'duve_rel18a';
	    exec sql select relidxcount,relstat into :val2, :id1 from iirelation
		where relid	= 'duve_rel18b';
	    val++;
	    val2++;
	    id1 |= TCB_IDXD;
	    exec sql update iirelation set relidxcount = :val
		where relid = 'duve_rel18a';
	    exec sql update iirelation set relidxcount = :val2, relstat = :id1
		where relid = 'duve_rel18b';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val--;
	    val2--;
	    id1 &= ~TCB_IDXD;
	    exec sql update iirelation set relidxcount = :val
		where relid = 'duve_rel18a';
	    exec sql update iirelation set relidxcount = :val2, relstat = :id1
		where relid = 'duve_rel18b';
	    exec sql drop index i_duve_rel18;
	    exec sql drop table duve_rel18a;
	    exec sql drop table duve_rel18b;
	    close_db();
	}

	if ( run_test(IIRELATION,19) )
	{
	    SIprintf ("....iirelation test 19..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 19 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_13);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel19(a i4, b i4);
	    exec sql select relstat into :val from iirelation
		where relid = 'duve_rel19';
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel19';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_rel19;
	    close_db();
	}

	if ( run_test(IIRELATION,20) )
	{
	    SIprintf ("....iirelation test 20..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 20 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_14);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel20(a i4, b i4);
	    exec sql select reltid,relstat,relloccount
		into :id1, :val,:val2 from iirelation
		where relid = 'duve_rel20';
	    val |= TCB_MULTIPLE_LOC;
	    val2++;
	    exec sql update iirelation set relstat = :val, relloccount = :val2
		where relid = 'duve_rel20';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val2--;
	    val &= ~TCB_MULTIPLE_LOC;
	    exec sql update iirelation set relstat = :val, relloccount = :val2
		where relid = 'duve_rel20';
	    exec sql drop table duve_rel20;
	    exec sql delete from iidatabase where reltid  = :id1;
	    exec sql delete from iiattribute where attrelid = :id1;
	    close_db();
	}

	if ( run_test(IIRELATION,21) )
	{
	    SIprintf ("....iirelation test 21..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 21 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_15);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel21(a i4);
	    exec sql select relloc into :charval from iirelation
		where relid = 'duve_rel21';
	    exec sql update iirelation set relloc = 'fake_location'
		where relid = 'duve_rel21';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set relloc = :charval
		where relid = 'duve_rel21';
	    exec sql drop table duve_rel21;
	    close_db();
	}

	if ( run_test(IIRELATION,22) )
	{
	    SIprintf ("....iirelation test 22..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 22 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_16);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel22(a i4, b i4);
	    exec sql select reldfill into :val from iirelation
		where relid = 'duve_rel22';
	    exec sql update iirelation set reldfill = 0
		where relid = 'duve_rel22';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set reldfill= :val
		where relid= 'duve_rel22';
	    exec sql drop table duve_rel22;
	    close_db();
	}

	if ( run_test(IIRELATION,23) )
	{
	    SIprintf ("....iirelation test 23..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 23 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_17);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel23(a i4, b i4);
	    exec sql modify duve_rel23 to hash on a;
	    exec sql select reldfill into :val from iirelation
		where relid = 'duve_rel23';
	    exec sql update iirelation set reldfill = 0
		where relid = 'duve_rel23';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set reldfill= :val
		where relid= 'duve_rel23';
	    exec sql drop table duve_rel23;
	    close_db();
	}

	if ( run_test(IIRELATION,24) )
	{
	    SIprintf ("....iirelation test 24..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 24 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_18);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel24(a i4, b i4);
	    exec sql modify duve_rel24 to isam on a;
	    exec sql select reldfill into :val from iirelation
		where relid = 'duve_rel24';
	    exec sql update iirelation set reldfill = 0
		where relid = 'duve_rel24';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set reldfill= :val
		where relid= 'duve_rel24';
	    exec sql drop table duve_rel24;
	    close_db();
	}

	if ( run_test(IIRELATION,25) )
	{
	    SIprintf ("....iirelation test 25..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 25 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_19);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel25(a i4, b i4);
	    exec sql modify duve_rel25 to btree on a;
	    exec sql select reldfill into :val from iirelation
		where relid = 'duve_rel25';
	    exec sql update iirelation set reldfill = 0
		where relid = 'duve_rel25';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iirelation set reldfill= :val
		where relid= 'duve_rel25';
	    exec sql drop table duve_rel25;
	    close_db();
	}

	    /*************/
	    /** test 26 **/
	    /*************/

		    /* this cannot safely be tested, so this test is skipped. */

	if ( run_test(IIRELATION,27) )
	{
	    SIprintf ("....iirelation test 27..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 27 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_1B);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rel27(a i4, b i4);
	    exec sql select relstat into :val from iirelation
		where relid = 'duve_rel27';
	    val |= TCB_RULE;
	    exec sql update iirelation set relstat = :val
		where relid = 'duve_rel27';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_rel27;
	    close_db();
	}

	if (run_test (IIRELATION, 28) )
        {
           SIprintf("....iirelation test 28..\n");
           SIflush(stdout);

           /*************/
           /** test 28 **/
           /*************/

           /* checks for schema for orphaned relation */

           /** setup   **/
           name_log(REL_CODE,ID_1C);
           open_db();
           if (test_status == TEST_ABORT)
               break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql create table duve_rel28(a i4, b i4);
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql drop table duve_rel28;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /* Orphaned views. Part A of test 1 */

           SIprintf("....iirelation test 28A..\n");
           SIflush(stdout);

           /**************/
           /** test 28A **/
           /**************/

           /** setup   **/
           open_db();
           if (test_status == TEST_ABORT)
               break;
           exec sql commit;
           exec sql set session authorization god;
           exec sql create table duve_rel28a(a i4, b i4);
           exec sql grant all on duve_rel28a to super;
           exec sql commit;
           exec sql set session authorization super;
           exec sql create view v_duve_rel28a as select *
                        from god.duve_rel28a;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql commit;
           exec sql set session authorization god;
           exec sql drop table duve_rel28a;
           exec sql commit;
           exec sql set session authorization super;
	   exec sql drop v_duve_rel28a;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           exec sql delete from iischema where schema_name = 'god';
           close_db();
        }

	if (run_test (IIRELATION, 29) )
	{
	    SIprintf("....iirelation test 29..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 29 **/
	    /*************/

	    /** setup **/
	    name_log(REL_CODE,ID_1D);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL REGISTER TABLE verifydb_test (database char(24) not null)
		AS IMPORT FROM 'current' WITH DBMS=SXA;
	    EXEC SQL SELECT reltid INTO :reltid FROM iirelation
		WHERE relid = 'verifydb_test';
	    EXEC SQL DELETE FROM iigw06_relation WHERE reltid = :reltid;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    EXEC SQL DELETE FROM iigw06_attribute WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iirelation WHERE reltid = :reltid;
	    EXEC SQL DELETE FROM iiattribute WHERE reltid = :reltid;
	    close_db();
	}

	if (run_test (IIRELATION, 30) )
	{
	    SIprintf("....iirelation test 30..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 30 **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a view that depends on SELECT on IIRELATION, then set
	    ** TCB_VGRANT_OK in relstat to make it look "always grantable"
	    */

	    name_log(REL_CODE,ID_1E);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_rel30;
	    exec sql create view duve_rel30 as select * from iirelation;

	    exec sql select relstat into :val 
		from iirelation
		where relid = 'duve_rel30' and relowner = user;
	    
	    val |= TCB_VGRANT_OK;

	    exec sql update iirelation 
		set relstat = :val
		where relid = 'duve_rel30' and relowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_rel30;

	    close_db();
	}

	if (run_test (IIRELATION, 31) )
	{
	    SIprintf("....iirelation test 31..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 31 **/
	    /*************/

	    /** setup **/

	    /*
	    ** create a view dependent on its owner's base table (which makes 
	    ** the view "always grantable"), then reset TCB_VGRANT_OK bit in 
	    ** relstat
	    */

	    name_log(REL_CODE,ID_1F);
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    exec sql drop duve_rel31_view1;
	    exec sql drop duve_rel31_tbl1;

	    exec sql create table duve_rel31_tbl1(i i);
	    exec sql create view duve_rel31_view1 as 
		select * from duve_rel31_tbl1;

	    exec sql select relstat, relqid1, relqid2 into :val, :txt1, :txt2
		from iirelation
		where relid = 'duve_rel31_view1' and relowner = user;
	    
	    val &= ~TCB_VGRANT_OK;

	    exec sql update iirelation 
		set relstat = :val
		where relid = 'duve_rel31_view1' and relowner = user;

	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;

	    /* 
	    ** code in duvechk.sc will delete catalog info pertaining to 
	    ** duve_rel31_view1 except for the query text - we need to clean 
	    ** it up to avoid hearing about this tuple throughout the remaining
	    ** tests.
	    */
	    exec sql delete from iiqrytext 
		where txtid1 = :txt1 and txtid2 = :txt2;

	    exec sql drop duve_rel31_tbl1;

	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iirel_idx - 
**
** Description:
**
**  This routine tests the verifydb iirel_idx catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iirel_idx()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;

    /** actually, at this time, it is not possible to update a secondary
    *** index, so we cannot really set up for this test.  The code is left
    *** here incase this ever changes.  At this point, this routine is a
    *** no-op and it is not possible to test this verifydb check/repair.
    ***/

    SIprintf ("..Testing verifydb checks/repairs for catalog iirel_idx..\n");
    SIflush(stdout);
    STcopy("iirel_idx",test_id);
    do
    {
	if ( run_test(IIREL_IDX, 1) )
	{
	    SIprintf ("....iirel_idx test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iirel_idx values('fake_relidx','fake_dba',100);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirel_idx where relid = 'fake_relidx';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iirule - 
**
** Description:
**
**  This routine tests the verifydb iirule catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	13-jan-94 (anitap)
**  	    Added logic to test check #6 in ckrule().
**	02-feb-94 (anitap)
**	    Updated cleanup of test check #6.	
*/
ck_iirule()
{
    EXEC SQL begin declare section;
	i4 id1, id2, id3;
	i4 val;
	i4 txt1, txt2;
	i4 dep_type;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iirule..\n");
    SIflush(stdout);
    STcopy("iirule",test_id);
    do
    {
	if ( run_test(IIRULE, 1) )
	{
	    SIprintf ("....iirule test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max(reltid) into :id1 from iirelation;
	    id1++;	
	    exec sql insert into iirule (rule_name, rule_owner, rule_type,
		rule_tabbase, rule_tabidx) values ('duve_rule1','$ingres',1,
		:id1, 0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iirule where rule_name = 'duve_rule1';
	    close_db();
	}

	if ( run_test(IIRULE, 2) )
	{
	    SIprintf ("....iirule test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rul2(a i4);
	    exec sql create procedure p_duve_rul2 as
		begin
		    message 'hi';
		end;
	    exec sql create rule r_duve_rul2 after insert on duve_rul2 execute
		procedure p_duve_rul2;
	    exec sql select reltid,relstat into :id1, :val from iirelation
		where relid = 'duve_rul2';
	    val &= ~TCB_RULE;
	    exec sql update iirelation set relstat=:val where relid='duve_rul2';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val |= TCB_RULE;
	    exec sql update iirelation set relstat = :val where relid = 'duve_rul2';
	    exec sql drop table duve_rul2;
	    exec sql drop procedure p_duve_rul2;
	    close_db();
	}

	if ( run_test(IIRULE, 3) )
	{
	    SIprintf ("....iirule test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rul3(a i4, b i4);
	    exec sql create procedure p_duve_rul3 as
		begin
		    message 'hi';
		end;
	    exec sql create rule r_duve_rul3 after update on duve_rul3
		execute procedure p_duve_rul3;
	    exec sql select rule_tabbase, rule_tabidx, rule_qryid1, rule_qryid2
		into :id1, :id2, :txt1, :txt2 from iirule
		where rule_name = 'r_duve_rul3';
	    exec sql delete from iiqrytext
		where txtid1 = :txt1 and txtid2 = :txt2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop procedure p_duve_rul3;
	    exec sql drop table duve_rul3;
	    exec sql delete from iitree
		where treetabbase=:id1 and treetabidx=:id2;
	    exec sql delete from iirule where rule_name='r_duve_rul3';
	    exec sql delete from iidbdepends where inid1 = :id1 and inid2= :id2;
	    close_db();
	}

	if ( run_test(IIRULE, 4) )
	{
	    SIprintf ("....iirule test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rul4(a i4, b i4);
	    exec sql create procedure p_duve_rul4 as
		begin
		    message 'hi';
		end;
	    exec sql create rule r_duve_rul4 after update on duve_rul4 execute
		procedure p_duve_rul4;
	    exec sql select dbp_txtid1, dbp_txtid2 into :txt1,:txt2
		from iiprocedure
		where dbp_name = 'p_duve_rul4';
	    exec sql delete from iiprocedure where dbp_name = 'p_duve_rul4';
	    exec sql delete from iiqrytext
		where txtid1 = :txt1 and txtid2 = :txt2;
	    exec sql select rule_tabbase, rule_tabidx, rule_qryid1, rule_qryid2
		into :id1, :id2, :txt1, :txt2 from iirule
		where rule_name = 'r_duve_rul4';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop rule r_duve_rul4;
	    exec sql drop table duve_rul4;
	    exec sql delete from iirule where rule_name='r_duve_rul4';
	    exec sql delete from iiqrytext
		where txtid1 = :txt1 and txtid2 = :txt2;
	    exec sql delete from iitree
		where treetabbase=:id1 and treetabidx=:id2;
	    close_db();
	}

	if ( run_test(IIRULE, 5) )
	{
	    SIprintf ("....iirule test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_rul5(a i4, b i4);
	    exec sql create procedure p_duve_rul5 as
		begin
		    message 'hi';
		end;
	    exec sql create rule r_duve_rul5 after update on duve_rul5 execute
		procedure p_duve_rul5;
	    exec sql select rule_tabbase, rule_tabidx, rule_qryid1, rule_qryid2
		into :id1, :id2, :txt1, :txt2 from iirule
		where rule_name = 'r_duve_rul5';
	    exec sql update iirule set rule_type = 2
		where rule_name = 'r_duve_rul5';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop rule r_duve_rul5;
	    exec sql delete from iirule where rule_name='r_duve_rul5';
	    exec sql delete from iiqrytext
		where txtid1 = :txt1 and txtid2 = :txt2;
	    exec sql delete from iitree
		where treetabbase=:id1 and treetabidx=:id2;
	    exec sql drop procedure p_duve_rul5;
	    exec sql drop table duve_rul5;
	    close_db();
	}

        if ( run_test(IIRULE, 6) )
        {
            SIprintf ("....iirule test 6..\n");
            SIflush(stdout);

            /*************/
            /** test 6  **/
            /*************/

	    dep_type = DB_RULE;	

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql create table duve_rul6(a i4 check(a is not null));
            exec sql select reltid into :id1 from iirelation
                where relid = 'duve_rul6';
	    exec sql select rule_treeid1, rule_treeid2, rule_qryid1, rule_qryid2
		into :id2, :id3, :txt1, :txt2 from iirule
		where rule_tabbase = :id1;
            exec sql delete from iidbdepends where inid1 = :id1 
		and dtype = :dep_type;
            close_db();

            /** run test **/
            report_vdb();
            run_vdb();

            /** clean up **/
            open_db();
            if (test_status == TEST_ABORT)
                break;
            exec sql drop table duve_rul6;
	    exec sql delete from iitree where
		treeid1 = :id2 and treeid2 = :id3;
	    exec sql delete from iiqrytext where
		txtid1 = :txt1 and txtid2 = :txt2; 	
            close_db();
        }

    } while (0);  /* control loop */
}

/*
** ck_iistar_cdbs - 
**
** Description:
**
**  This routine tests the verifydb iistar_cdbs catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iistar_cdbs()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iistar_cdbs..\n");
    SIflush(stdout);
    STcopy("iistar_cdbs ",test_id);
    do
    {
	if ( run_test(IISTAR_CDBS, 1) )
	{
	    SIprintf ("....iistar_cdbs test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db4',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,44,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db4', 3);
	    exec sql insert into iistar_cdbs (cdb_name,cdb_owner,ddb_name,
		ddb_owner, cdb_id)
		values ('duve_ficticious_db4','$ingres','idontexist',
			'$ingres',44);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_ficticious_db4';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db4';
	    exec sql delete from iistar_cdbs
		where cdb_name = 'duve_ficticious_db4';
	    close_db();
	}

	if ( run_test(IISTAR_CDBS, 2) )
	{
	    SIprintf ("....iistar_cdbs test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db5',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,55,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db5', 3);
	    exec sql insert into iistar_cdbs (ddb_name,ddb_owner,cdb_name,
		cdb_owner,cdb_id)
		values ('duve_ficticious_db5','$ingres','idontexist',
			'$ingres',55);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_ficticious_db5';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db5';
	    exec sql delete from iistar_cdbs
		where ddb_name = 'duve_ficticious_db5';
	    close_db();
	}

	if ( run_test(IISTAR_CDBS, 3) )
	{
	    SIprintf ("....iistar_cdbs test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db6',
		'$ingres', 'ii_database','ii_checkpoint',
		'ii_journal','ii_work',	17,0,'6.4',0,66,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db6', 3);
	    exec sql insert into iidatabase values ('duve_ficticious_db7',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,77,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db7', 3);
	    exec sql insert into iistar_cdbs (ddb_name,ddb_owner,cdb_name,
		cdb_owner, cdb_id)
		values ('duve_ficticious_db7','duve_ficticious_dba',
			'duve_ficticious_db6','$ingres',66);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_ficticious_db6';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db6';
	    exec sql delete from iidatabase where name = 'duve_ficticious_db7';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db7';
	    exec sql delete from iistar_cdbs
		where ddb_name = 'duve_ficticious_db7';
	    close_db();
	}

	if ( run_test(IISTAR_CDBS, 4) )
	{
	    SIprintf ("....iistar_cdbs test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db8',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,88,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db8', 3);
	    exec sql insert into iidatabase values ('duve_ficticious_db9',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,99,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db9', 3);
	    exec sql insert into iistar_cdbs (ddb_name,ddb_owner,cdb_name,
		cdb_owner, cdb_id)
		values ('duve_ficticious_db9','$ingres',
			'duve_ficticious_db8','duve_ficticious_dba',88);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_ficticious_db8';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db8';
	    exec sql delete from iidatabase where name = 'duve_ficticious_db9';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db9';
	    exec sql delete from iistar_cdbs
		where ddb_name = 'duve_ficticious_db9';
	    close_db();
	}

	if ( run_test(IISTAR_CDBS, 5) )
	{
	    SIprintf ("....iistar_cdbs test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iidatabase values ('duve_ficticious_db10',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,110,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db10', 3);
	    exec sql insert into iidatabase values ('duve_ficticious_db11',
		'$ingres', 'ii_database','ii_checkpoint','ii_journal','ii_work',
		17,0,'6.4',0,111,'ii_dump');
	    exec sql insert into iiextend values
	    	    ('ii_database','duve_ficticious_db11', 3);
	    exec sql insert into iistar_cdbs (ddb_name,ddb_owner,cdb_name,
		cdb_owner, cdb_id)
		values ('duve_ficticious_db11','$ingres',
			'duve_ficticious_db10','$ingres',66);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iidatabase where name = 'duve_ficticious_db10';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db10';
	    exec sql delete from iidatabase where name = 'duve_ficticious_db11';
	    exec sql delete from iiextend where dname = 'duve_ficticious_db11';
	    exec sql delete from iistar_cdbs
		where ddb_name = 'duve_ficticious_db11';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iistatistics - 
**
** Description:
**
**  This routine tests the verifydb iistatistics catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	17-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iistatistics()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iistatistics..\n");
    SIflush(stdout);
    STcopy("iistatistics",test_id);
    do
    {
	if ( run_test(IISTATISTICS, 1) )
	{
	    SIprintf ("....iistatistics test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max(reltid) into :val from iirelation;
	    val++;
	    exec sql insert into iistatistics (stabbase) values (:val);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iistatistics where stabbase = :val;
	    close_db();
	}

	if ( run_test(IISTATISTICS, 2) )
	{
	    SIprintf ("....iistatistics test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_stat2(a i4, b i4);
	    exec sql create index i_duve_stat2 on duve_stat2(a);
	    exec sql select reltid,reltidx,relstat into :id1, :id2, :val
		from iirelation where relid = 'i_duve_stat2';
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val where
		relid = 'i_duve_stat2';
	    exec sql insert into iistatistics (stabbase,stabindex)
		    values (:id1, :id2);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iistatistics where stabbase = :id1;
	    exec sql drop index i_duve_stat2;
	    exec sql drop table duve_stat2;
	    close_db();
	}

	if ( run_test(IISTATISTICS, 3) )
	{
	    SIprintf ("....iistatistics test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_stat3(a i4, b i4);
	    exec sql select reltid,reltidx into :id1, :id2 from iirelation
		    where relid = 'duve_stat3';
	    exec sql insert into iistatistics (stabbase, stabindex, satno)
		    values (:id1, :id2, 1);	
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iistatistics where stabbase = :id1;
	    exec sql drop table duve_stat3;
	    close_db();
	}

	if ( run_test(IISTATISTICS, 4) )
	{
	    SIprintf ("....iistatistics test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_stat4(a i4, b i4);
	    exec sql select reltid,reltidx,relstat into :id1, :id2, :val
		    from iirelation where relid = 'duve_stat4';
	    exec sql insert into iistatistics (stabbase,stabindex,satno)
		    values (:id1, :id2, 3);	
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val
		where relid ='duve_stat4';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iistatistics where stabbase = :id1;
	    exec sql drop table duve_stat4;
	    close_db();
	}

	if ( run_test(IISTATISTICS, 5) )
	{
	    SIprintf ("....iistatistics test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_stat5(a i4, b i4);
	    exec sql select reltid,reltidx,relstat into :id1, :id2, :val
		    from iirelation where relid = 'duve_stat5';
	    exec sql
		insert into iistatistics (stabbase,stabindex,satno,snumcells)
		    values (:id1, :id2, 1, 5);	
	    val |= TCB_ZOPTSTAT;
	    exec sql update iirelation set relstat = :val
		where relid ='duve_stat5';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iistatistics where stabbase = :id1;
	    exec sql drop table duve_stat5;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iisynonym - 
**
** Description:
**
**  This routine tests the verifydb iisynonym catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**      23-nov-93 (anitap)
**          added test 3 to check that schema is created for orphaned synonym.
*/
ck_iisynonym()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iisynonym..\n");
    SIflush(stdout);
    STcopy("iisynonym",test_id);
    do
    {
	if ( run_test(IISYNONYM, 1) )
	{
	    SIprintf ("....iisynonym test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_syn1(a i4, b i4);
	    exec sql select reltid,reltidx into :id1,:id2 from iirelation
		    where relid = 'duve_syn1';
	    exec sql insert into iisynonym (synonym_name,synonym_owner,
		syntabbase,syntabidx,synid,synidx)
		values ('duve_syn1',:dba,:id1,:id2,:id1,0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iisynonym
		where syntabbase=:id1 and syntabidx=:id2;
	    exec sql drop table duve_syn1;
	    close_db();
	}

	if ( run_test(IISYNONYM, 2) )
	{
	    SIprintf ("....iisynonym test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_syn2(a i4, b i4);
	    exec sql select reltid,reltidx into :id1,:id2 from iirelation
		    where relid = 'duve_syn2';
	    id1++;
	    exec sql insert into iisynonym (synonym_name,synonym_owner,
		syntabbase,syntabidx,synid,synidx)
		values ('duve_syn2',:dba,:id1,:id2,:id1,0);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    id1--;
	    exec sql delete from iisynonym
		where syntabbase=:id1 and syntabidx=:id2;
	    exec sql drop table duve_syn2;
	    close_db();
	}

        if ( run_test(IISYNONYM, 3) )
        {
            SIprintf ("....iisynonym test 3..\n");
            SIflush(stdout);

            /*************/
            /** test 3  **/
            /*************/

            /** setup **/
            open_db();
            if (test_status == TEST_ABORT)
               break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql create synonym duve_syn3 for iirelation;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();

           /** run test **/
           report_vdb();
           run_vdb();

           /** clean up **/
           open_db();
           if (test_status == TEST_ABORT)
              break;
           exec sql commit;
           exec sql set session authorization super;
           exec sql drop synonym duve_syn3;
           exec sql commit;
           exec sql set session authorization $ingres;
           exec sql delete from iischema where schema_name = 'super';
           close_db();
        }

    } while (0);  /* control loop */
}

/*
** ck_iitree - 
**
** Description:
**
**  This routine tests the verifydb iitree catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	09-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	10-nov-93 (anitap)
**	    Corrected ii dbdepends to iidbdepends and indi1 to inid1 and indid2
**	    to inid2 in test 5.
*/
ck_iitree()
{
    EXEC SQL begin declare section;
	i4 id1, id2;
	i4 id1_a, id2_a;
	i4 id1_b, id2_b;
	i4 id1_c, id2_c;
	i4 val1, val2, val3, val4;
	i4 qid1_a, qid2_a, qid1_b, qid2_b, qid1_c, qid2_c, qid1_d, qid2_d;
	i4 qid1_e, qid2_e, rid1_e, rid2_e;
	i4 rid1_a, rid2_a, rid1_b, rid2_b, rid1_c, rid2_c, rid1_d, rid2_d;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iitree..\n");
    SIflush(stdout);
    STcopy("iitree",test_id);
    do
    {
	if ( run_test(IITREE, 1) )
	{
	    SIprintf ("....iitree test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max(reltid) into :id1 from iirelation;
	    id1++;
	    exec sql insert into iitree values (:id1,0,100,101, 0, 17, 0, 'hi');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iitree where treetabbase = :id1;
	    close_db();
	}

	if ( run_test(IITREE, 2) )
	{
	    SIprintf ("....iitree test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_tree2(a i4);
	    exec sql create view duve_treeview2 as select * from duve_tree2;
	    exec sql select reltid,reltidx,relqid1,relqid2
		    into :id1,:id2,:id1_a,:id2_a from iirelation
		    where relid = 'duve_treeview2';
	    exec sql select treeid1, treeid2 into :id1_b, :id2_b from iitree
		    where treetabbase = :id1 and treetabidx = :id2;
	    exec sql update iitree set treemode = 11
		    where treetabbase = :id1 and treetabidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iitree set treemode = 17
		    where treetabbase = :id1 and treetabidx = :id2;
	    exec sql drop view duve_treeview2;
	    exec sql drop table duve_tree2;
	    exec sql delete from iitree where treeid1=:id1_b and treeid2=:id2_b;
	    exec sql delete from iiqrytext
		where txtid1 = :id1_a and txtid2= :id2_a;
	    exec sql delete from iidbdepends where deid1=:id1 and deid2 = :id2;
	    close_db();
	}

	if ( run_test(IITREE, 3) )
	{
	    /*************/
	    /** test 3  **/
	    /*************/

	    /* cant test this because it's caught by the iirelation tests */
	}

	if ( run_test(IITREE, 4) )
	{
	    SIprintf ("....iitree test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_tree4a( a i4, b i4);
	    exec sql create table duve_tree4b( a i4, b i4);
	    exec sql insert into duve_tree4b values (1,1);
	    exec sql create table duve_tree4c( a i4, b i4);
	    exec sql insert into duve_tree4c values (1,1);
	    exec sql create table duve_tree4d( a i4, b i4);
	    exec sql insert into duve_tree4d values (1,1);
	    exec sql create view v_duve_tree4a as select * from duve_tree4a;
	    exec sql create permit select on duve_tree4b to '$ingres'
		    where a > 5;
	    exec sql create integrity on duve_tree4c is b < 100;
	    exec sql create procedure p_duve_tree4 as
		    begin
			    message 'hi';
		    end;
	    exec sql create rule r_duve_tree4 after insert on duve_tree4d where
		    old.a > new.a execute procedure p_duve_tree4;
	    exec sql select reltid,reltidx,relqid1,relqid2 into
		    :rid1_a,:rid2_a,:qid1_a, :qid2_a from iirelation
		    where relid = 'duve_tree4a';
	    exec sql select reltid,reltidx,relqid1,relqid2 into
		    :rid1_b,:rid2_b,:qid1_b, :qid2_b from iirelation
		    where relid = 'duve_tree4b';
	    exec sql select reltid,reltidx,relqid1,relqid2 into
		    :rid1_c,:rid2_c,:qid1_c, :qid2_c from iirelation
		    where relid = 'duve_tree4c';
	    exec sql select reltid,reltidx,relqid1,relqid2 into
		    :rid1_d,:rid2_d,:qid1_d, :qid2_d from iirelation
		    where relid = 'duve_tree4d';
	    exec sql select relstat into :val1 from iirelation
		    where relid = 'duve_tree4a';
	    exec sql select relstat into :val2 from iirelation
		    where relid = 'duve_tree4b';
	    exec sql select relstat into :val3 from iirelation
		    where relid = 'duve_tree4c';
	    exec sql select relstat into :val4 from iirelation
		    where relid = 'duve_tree4d';
	    exec sql select relqid1,relqid2 into :id1, :id2 from iirelation
		    where relid = 'p_duve_tree4';
	    exec sql select reltid,reltidx, relqid1,relqid2 into
		    :rid1_e, :rid2_e, :qid1_e, :qid2_e from iirelation
		    where relid = 'v_duve_tree4a';
	    val1 &= ~TCB_VIEW;
	    val2 &= ~TCB_PRTUPS;
	    val3 &= ~TCB_INTEGS;
	    val4 &= ~TCB_RULE;
	    exec sql update iirelation set relstat= :val1
		where relid='duve_tree4a';
	    exec sql update iirelation set relstat= :val2
		where relid='duve_tree4b';
	    exec sql update iirelation set relstat= :val3
		where relid='duve_tree4c';
	    exec sql update iirelation set relstat= :val4
		where relid='duve_tree4d';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    val1 |= TCB_VIEW;
	    val2 |= TCB_PRTUPS;
	    val3 |= TCB_INTEGS;
	    val4 |= TCB_RULE;
	    exec sql update iirelation set relstat= :val1
		where relid='duve_tree4a';
	    exec sql update iirelation set relstat= :val2
		where relid='duve_tree4b';
	    exec sql update iirelation set relstat= :val3
		where relid='duve_tree4c';
	    exec sql update iirelation set relstat= :val4
		where relid='duve_tree4d';
	    exec sql drop view v_duve_tree4a;
	    exec sql drop procedure p_duve_tree;
	    exec sql drop table duve_tree4a;
	    exec sql drop table duve_tree4b;
	    exec sql drop table duve_tree4c;
	    exec sql drop table duve_tree4d;
	    exec sql delete from iitree
		    where treetabbase=:rid1_a and treetabidx=:rid2_a;
	    exec sql delete from iitree
		    where treetabbase=:rid1_b and treetabidx=:rid2_b;
	    exec sql delete from iitree
		    where treetabbase=:rid1_c and treetabidx=:rid2_c;
	    exec sql delete from iitree
		    where treetabbase=:rid1_d and treetabidx=:rid2_d;
	    exec sql delete from iitree
		    where treetabbase=:rid1_e and treetabidx=:rid2_e;
	    exec sql delete from iiqrytext
		where txtid1=:qid1_a and txtid2=:qid2_a;
	    exec sql delete from iiqrytext
		where txtid1=:qid1_b and txtid2=:qid2_b;
	    exec sql delete from iiqrytext
		where txtid1=:qid1_c and txtid2=:qid2_c;
	    exec sql delete from iiqrytext
		where txtid1=:qid1_d and txtid2=:qid2_d;
	    exec sql delete from iiqrytext
		where txtid1=:qid1_e and txtid2=:qid2_e;
	    exec sql delete from iiattribute
		    where attrelid=:rid1_a and attrelidx=:rid2_a;
	    exec sql delete from iiattribute
		    where attrelid=:rid1_b and attrelidx=:rid2_b;
	    exec sql delete from iiattribute
		    where attrelid=:rid1_c and attrelidx=:rid2_c;
	    exec sql delete from iiattribute
		    where attrelid=:rid1_d and attrelidx=:rid2_d;
	    exec sql delete from iiattribute
		    where attrelid=:rid1_e and attrelidx=:rid2_e;
	    exec sql delete from iirelation
		    where reltid=:rid1_a and reltidx=:rid2_a;
	    exec sql delete from iirelation
		    where reltid=:rid1_b and reltidx=:rid2_b;
	    exec sql delete from iirelation
		    where reltid=:rid1_c and reltidx=:rid2_c;
	    exec sql delete from iirelation
		    where reltid=:rid1_d and reltidx=:rid2_d;
	    exec sql delete from iirelation
		    where reltid=:rid1_e and reltidx=:rid2_e;
	    exec sql delete from iidbdepends where deid1=:rid1_e and deid2=:rid2_e;
	    close_db();
	}

	if ( run_test(IITREE, 5) )
	{
	    SIprintf ("....iitree test 5..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 5  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_tree5(a i4);
	    exec sql create view duve_vtree5 as select * from duve_tree5;
	    exec sql select reltid,reltidx,relqid1,relqid2
		    into :id1, :id2, :val1, :val2 from iirelation
		    where relid = 'duve_vtree5';
	    exec sql update iitree set treeseq = 1
		    where treetabbase = :id1 and treetabidx = :id2;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql update iitree set treeseq = 0
		    where treetabbase = :id1 and treetabidx = :id2;
	    exec sql drop view duve_vtree5;
	    exec sql delete from iirelation
		where reltid = :id1 and reltidx = :id2;
	    exec sql delete from iiattribute
		    where attrelid = :id1 and attrelidx = :id2;
	    exec sql delete from iitree where treeid1=:id1 and treeid2=:id2;
	    exec sql delete from iidbdepends where inid1=:id1 and inid2=:id2;
	    exec sql drop table duve_tree5;
	    exec sql delete from iiqrytext
		where txtid1 = :val1 and txtid2 = :val2;
	    close_db();
	}

	if ( run_test(IITREE, 6) )
	{
	    SIprintf ("....iitree test 6..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 6  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_tree6(a i4);
	    exec sql create procedure duve_treeproc =
		    begin
			    message 'hi';
		    end;
	    exec sql create rule duve_treerule after update on duve_tree6
		    where new.a > old.a
		    execute procedure duve_treeproc;
	    exec sql select reltid,reltidx into :rid1_a, :rid2_a from iirelation
		    where relid = 'duve_tree6';
	    exec sql select rule_qryid1, rule_qryid2 into :id1, :id2 from iirule
		    where rule_name = 'duve_treerule' and rule_owner = :dba;
	    exec sql delete from iirule where rule_name = 'duve_treerule' and
		    rule_owner = :dba;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiqrytext where txtid1 = :id1 and txtid2 = :id2;
	    exec sql drop procedure duve_treeproc;
	    exec sql drop table duve_tree6;
	    exec sql delete from iidbdepends
		where inid1 = :rid1_a and inid2 = :rid2_a;
	    exec sql delete from iitree
		where treetabbase = :rid1_a and treetabidx = :rid2_a;
	    close_db();
	}

	if ( run_test(IITREE, 7) )
	{
	    /*************/
	    /** test 7  **/
	    /*************/

	    /* not testable at this time because currently events do not cause
    	    ** tree tuples to be generated */
	}

    } while (0);  /* control loop */
}

/*
** ck_iiuser - 
**
** Description:
**
**  This routine tests the verifydb iiuser catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
**	15-Oct-93 (teresa)
**	    Remove test 1 since the verifydb functionality for test 1 has been
**	    commented out.  This was necessary because the Delimited Identifier
**          project changed the semantics for detecting valid user names and now
**          almost anything is a valid user name.
**	22-dec-93 (robf)
**          added test 4 (profile name)
*/
ck_iiuser()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiuser..\n");
    SIflush(stdout);
    STcopy("iiuser",test_id);
    do
    {
#if 0
	if ( run_test(IIUSER, 1) )
	{
	    SIprintf ("....iiuser test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiuser (name, status) values ('**bad**',35345);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiuser where name = '**bad**';
	    close_db();
	}   
#endif

	if ( run_test(IIUSER, 2) )
	{
	    SIprintf ("....iiuser test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiuser (name, status) values ('duve_user2',-1);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiuser where name = 'duve_user2';
	    close_db();
	}   

	if ( run_test(IIUSER, 3) )
	{
	    SIprintf ("....iiuser test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiuser (name, status,default_group) values
		('duve_user3',35345,'duve_nonexistent_default_group');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiuser where name = 'duve_user3';
	    close_db();
	}   
	if ( run_test(IIUSER, 4) )
	{
	    SIprintf ("....iiuser test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiuser (name, status,profile) values
		('duve_user4',35345,'duve_nonexistent_profile');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiuser where name = 'duve_user4';
	    close_db();
	}   

    } while (0);  /* control loop */
}

/*
** ck_iiusergroup - 
**
** Description:
**
**  This routine tests the verifydb iiusergroup catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iiusergroup()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiusergroup..\n");
    SIflush(stdout);
    STcopy("iiusergroup",test_id);
    do
    {
	if ( run_test(IIUSERGROUP, 1) )
	{   
	    SIprintf ("....iiusergroup test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiusergroup (groupid,groupmem) values
		('valid_group','                                ');
	    exec sql insert into iiusergroup (groupid,groupmem) values
		('valid_group','invalid_member');
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiusergroup where groupid= 'valid_group';
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_iiprofile - 
**
** Description:
**
**  This routine tests the verifydb iiprofile catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	22-dec-93 (robf)
**	    Initial creation.
*/
ck_iiprofile()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iiprofile..\n");
    SIflush(stdout);
    STcopy("iiprofile",test_id);
    do
    {
	if ( run_test(IIPROFILE, 1) )
	{
	    SIprintf ("....iiprofile test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiprofile (name, status) values ('**bad**',35345);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiprofile where name = '**bad**';
	    close_db();
	}   

	if ( run_test(IIPROFILE, 2) )
	{
	    SIprintf ("....iiprofile test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql insert into iiprofile (name, status) values ('duve_user2',-1);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiprofile where name = 'duve_user2';
	    close_db();
	}   

	if ( run_test(IIPROFILE, 3) )
	{
	    SIprintf ("....iiprofile test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iiprofile where name='';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	}   

    } while (0);  /* control loop */
}
/*
** ck_iixdbdepends - 
**
** Description:
**
**  This routine tests the verifydb iixdbdependscatalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	22-jun-93 (teresa)
**	    added logic to handle subtest parameter to each test.
*/
ck_iixdbdepends()
{
    EXEC SQL begin declare section;
	i4 id1, id2;
    EXEC SQL end declare section;

 
    SIprintf ("..Testing verifydb checks/repairs for catalog iixdbdepends..\n");
    SIflush(stdout);
    STcopy("iixddbdepends",test_id);
    do
    {
	if ( run_test(IIXDBDEPENDS, 1) )
	{
	    SIprintf ("....iixdbdepends test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql select max(reltid) into :id1 from iirelation;
	    id1 ++;
	    id2 = id1+1;
	    exec sql insert into iixdbdepends values (:id1, :id2, 17, 1, 100);
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql delete from iixdbdepends where deid1 = :id1;
	    close_db();
	}

    } while (0);  /* control loop */
}

/*
** ck_suppress() - 
**
** Description:
**
**  This routine determines whether or not to supply a suppress message
**  number value for a given test.  The suppress value only works when a
**  specified test is selected.  Example:
**
**	Error message S_DU167D_NO_RULES indicates that at one time there were
**	rules defined on a table.  However, it is perfectly valid for this
**	bit to be set even if there are no rules currently defined.  In such
**	a case, verifydb reports that the bit is set and recommends clearing it.
**	If we are running a test suite and there are some tables in the user's
**	database which used to have rules and no longer do, it will generate
**	some diffs that we do not expect -- but these will not be significant
**	diffs.  However, if we are running iitree test #6 (or iirelation test
**	#27), then we purposely set up error conditions where this message is
**	expected.
**
**	So, the trick is to suppress S_DU167D_NO_RULES unless we are running
**	a test where we expect it in the diffs (iirelation #27 or iitree #6).
**
**	This is a little more complicated than it may seem because there are
**	different ways that the user can specify to run these specific tests.
**	The following conditions may trigger iitree test 6 to be run (and this
**	is how the suppress logic works in each case):
**	1.  The user has specified only iitree test 6 - argv[4] = 3 and
**	    argv[5] = 506.  In this case, we will specify to suppress
**	    all messages but S_DU167D_NO_RULES.  (This is the usual run_all or
**	    run_seq case.)
**	2.  The user has specified all iitree tests be run -- argv[4] = 3 and
**	    argv[5] = 500.  In this case, ALL messages will be suppressed
**	    for all iitree tests except test 6, which will suppress all messages
**	    except S_DU167D_NO_RULES.
**	3.  The user has specified all qrymod tests be run -- argv[4] = 3 and
**	    argv[5] = 0.  In this case, no messages will be suppressed.
**	4.  The user has specified that all tests be run -- argv[4] = 1 or
**	    argv[4] is not supplied.  In this case, no messages will be
**	    suppressed.
**
**	This description section accurately describes suppressed messages as of
**	22-dec-1993.  But, the way developers develop, this will undoubtedly get
**	out of date.  So, if you want to know the details of what is currently
**	being suppressed, refer to routine duve_suppress() in duvetalk.c.
**	There is a lookup table named suppress_list[], which contains the
**	official list of all suppressed messages.  The current list is:
**
**	Message ID:		    Test that generates that message:
**	------------------------    ---------------------------------
**	S_DU1611_NO_PROTECTS	    [CORE] iirelation test 8
**	S_DU1612_NO_INTEGS	    [CORE] iirelation test 9
**	S_DU1619_NO_VIEW	    [CORE] iirelation test 13
**	S_DU161E_NO_STATISTICS	    [CORE] iirelation test 19
**	S_DU165E_BAD_DB		    [IIDBDB] iidatabase test 9
**	S_DU167A_WRONG_CDB_ID	    [IIDBDB] iistar_cdbs test 5
**	S_DU167D_NO_RULES	    [CORE] iirelation test 27 AND
**				    [QRYMOD] iitree test 3
**
**  Inputs:
**	Catalog			A numeric code specifying which catalog is
**				being operated on.
**	testid			a numeric code indaciting which test is being
**				run
**  Outputs:
**	global variable: suppress - suppress instruction:
**					DO_NOT_SUPPESS	- suppress no msgs
**				    or
**					SUPPRESS_ALL	- suppress all msgs
**				    or
**					specific value of msg to suppress
**
**  History:
**	22-dec-93 (teresa)
**	    Initial creation
**	27-jan-94 (andre)
**	    added cases for IIXPROTECT, IIPRIV, IIXPRIV, IIXEVENT, and 
*8	    IIXPROCEDURE
*/
ck_suppress(catalog, testid)
i4	    catalog;
i4	    testid;
{
    i4	retstat=FALSE;
    i4	map = BIT2 | BIT3 | BIT4 | BIT5 | BIT6 ;

    /* don't suppress any messages if running all tests or if debug mode
    ** has been selected
    */
    if (
	(testmap & BIT10)   /* debug */
       ||
	(testmap & BIT20)   /* debug */
       ||
       /* run all tests */
	( (testmap & map ) == map )
       )
	{
	    /* indicate we are not to suppress messages */
	    suppress = DO_NOT_SUPPESS;
	    return;
	}

    if (subtest == 0)
    {
	/* we have been asked to run all catalogs in a category, so do NOT
	** suppress any messages
	*/
	suppress = DO_NOT_SUPPESS;
	return;
    }

    /* if we get here, we are either running one specific test or we are
    ** running all tests for a given catalog.  In either case, we suppress
    ** all messages unless there is a specific test that requires a specific
    ** message NOT be suppressed.
    */    
    switch (catalog)
    {
	case IIATTRIBUTE:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IICDBID_IDX:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
    	case IIDATABASE:
	    if (testid == 9)
		suppress = S_DU165E_BAD_DB;
	    else
		/* there are no special messages to avoid suppressing, so
		** suppress all messages
		*/
		suppress = SUPPRESS_ALL;
	    break;
	case IIDBDEPENDS:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIDBID_IDX:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIDBMS_COMMENT:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIDBPRIV:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIDEVICES:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIDEFAULT:
	    /* there are no special messages to avoid suppressing, so suppress
            ** all messages
            */
            suppress = SUPPRESS_ALL;
            break;
	case IIEVENT:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIEXTEND:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIGW06_ATTRIBUTE:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIGW06_RELATION:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIHISTOGRAM:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIINDEX:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIINTEGRITY:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIKEY:
	    /* there are no special messages to avoid suppressing, so suppress
            ** all messages
            */
            suppress = SUPPRESS_ALL;
            break;	
	case IILOCATIONS:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIPROCEDURE:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIPROTECT:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIQRYTEXT:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIRELATION:
	    if (testid == 8)
		suppress = S_DU1611_NO_PROTECTS;
	    else if (testid == 9)
		suppress = S_DU1612_NO_INTEGS;
	    else if (testid == 13)
		suppress = S_DU1619_NO_VIEW;
	    else if (testid == 19)
		suppress = S_DU161E_NO_STATISTICS;
	    else if (testid == 27)
		suppress = S_DU167D_NO_RULES;
	    else
		/* there are no special messages to avoid suppressing, so
		** suppress all messages
		*/
		suppress = SUPPRESS_ALL;
	    break;
	case IIREL_IDX:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIRULE:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IISTAR_CDBS:
	    if (testid == 5)
		suppress = S_DU167A_WRONG_CDB_ID;
	    else
		/* there are no special messages to avoid suppressing, so
		** suppress all messages
		*/
		suppress = SUPPRESS_ALL;
	    break;
	case IISTATISTICS:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IISYNONYM:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IITREE:
	    if (testid == 6)
		suppress = S_DU167D_NO_RULES;
	    else
		/* there are no special messages to avoid suppressing, so
		** suppress all messages
		*/
		suppress = SUPPRESS_ALL;
	    break;
	case IIUSER:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIUSERGROUP:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIXDBDEPENDS:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIPRIV:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIXPRIV:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIXEVENT:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	case IIXPROCEDURE:
	    /* there are no special messages to avoid suppressing, so suppress
	    ** all messages
	    */
	    suppress = SUPPRESS_ALL;
	    break;
	default:
	    /* unexpected case, so don't suppress any messages. */
	    suppress = DO_NOT_SUPPESS;
	    break;
    }
}

/*
** core_cats() - this routine tests the verifydb core catalog checks/repairs
**
** Description:
**
** This routine tests the verifydb core catalog checks/repairs.  This includes
** checks/repairs on the following catalogs:
**	iiattribute
**	iidevices
**	iiindex
**	iirelation
**	iirel_idx
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**	Outputs to log files:   ii_config:iivdbc01.DBA to
**				ii_config:iivdbc04.DBA
**	    and
**				ii_config:iivdbr00.DBA to 
**				ii_config:iivdbr0f.DBA
**	    where DBA is the value of the DBA name input parameter
**  History:
**	08-jun-93 (teresa)
**	    Initial creation.
*/
core_cats()
{
    SIprintf("Testing Verifydb Check/Repairs On Core Catalogs\n");
    SIflush(stdout);
    if (test_status != TEST_ABORT)
    {
	name_log(CORE_CODE,ID_01);
	ck_iiattribute();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(CORE_CODE,ID_02);
	ck_iidevices();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(CORE_CODE,ID_03);
	ck_iiindex();
    }
    if (test_status != TEST_ABORT)
    {
	/* iirelation outputs several log files since there are so
	** many check/repair sets in it.  So it gets its own alpha character: r
	*/
	name_log(REL_CODE,ID_00);
	ck_iirelation();
    }
/* currently the server does not allow us to update secondry indees, so we
** cannot test this.
    if (test_status != TEST_ABORT)
    {
	name_log(CORE_CODE,ID_04);
	ck_iirel_idx();
    }
*/
}

/*
** dbdb_cats() - this routine tests the verifydb iidbdb-only catalog
**	       checks/repairs
**
** Description:
**
** This routine tests the verifydb iidbdb-only catalog checks/repairs.
** This includes checks/repairs on the following catalogs:
**	iicdbid_idx
**	iidatabase
**	iidbaccess
**	iidbid_idx
**	iidbprv
**	iiextend
**	iilocations
**	iistar_cdbs
**	iiuser
**	iiusergroup
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**	Outputs to log files:   ii_config:iivdbd01.DBA to
**				ii_config:iivdbd0A.DBA
**			     and iidatabase tests in
**				ii_config:iivdba01.DBA to
**				ii_config:iivdba0F.DBA
**	    where DBA is the value of the DBA name input parameter
**	    and ii_config:iivdbd02.DBA is currently not used.
**		
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
*/
dbdb_cats()
{
    SIprintf("Testing Verifydb Check/Repairs On iidbdb-Only Catalogs\n");
    SIflush(stdout);

/* we currently cannot test this because the server does not support direct
** updates on secondary indexes 
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_01);
	ck_iicdbid_idx();
    }
*/

    /* catalog iidatabase has so many tests that it gets its own code */
    if (test_status != TEST_ABORT)
    {
	name_log(DTBASE_CODE,ID_00);
	ck_iidatabase();
    }
/* This catalog does not exist in release 6.5, so no sense coding a test for it
** for release 6.5 (now called Series 2000, I1/2). 
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_03);
	ck_iidbaccess();
    }
*/
/* we currently cannot test this because the server does not support direct
** updates on secondary indexes 
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_04);
	ck_iidbid_idx();
    }
*/
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_05);
	ck_iidbpriv();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_06);
	ck_iiextend();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_07);
	ck_iilocations();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_08);
	ck_iistar_cdbs();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_09);
	ck_iiuser();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_0A);
	ck_iiusergroup();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(DBDB_CODE,ID_0B);
	ck_iiprofile();
    }
}

/*
** debug_me() - A debug work area
**
** Description:
**
** This routine is a place to cut-n-paste portions of a routine that you
** are trying to debug or test.  You can put whatever you like in here, from
** a call to a single ck_<catalog_name>()  routine to whatever you need to
** debug.
**
** You can skip all the verifydb tests and jump right to this rouitne by
** entering a test instruction of 10.  Example: use database "mydb" owned
** by "joe_programmer", with a valid installation user name of "samspade";
** just skip to this debug section and skip routines to check core catalogs,
** qrymod catalogs, etc:
**
**	test_vdb mydb joe_programmer samspade 10
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**  History:
**	Does not apply as this is a dynamic work area.
*/
debug_me()
{
	ck_iiattribute();
}

/*
** debug_me2() - A debug work area
**
** Description:
**
** This routine is a place to cut-n-paste portions of a routine that you
** are trying to debug or test.  You can put whatever you like in here, from
** a call to a single ck_<catalog_name>()  routine to whatever you need to
** debug.
**
** You can skip all the verifydb tests and jump right to this routne by
** entering a test instruction of 20.  Example: use database "mydb" owned
** by "joe_programmer", with a valid installation user name of "samspade";
** just skip to this debug section and skip routines to check core catalogs,
** qrymod catalogs, etc:
**
**	test_vdb mydb joe_programmer samspade 20
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**  History:
**	Does not apply as this is a dynamic work area.
*/
debug_me2()
{
}

/*
** name_log() - build name for verifydb output log file
**
** Description:
**
**  This routine builds a name for the verifydb log file and places that name
**  into global variable 'logfile'.  These are the nameing conventions:
**
**	filename.ext where:
**
**	1.  All file names do not exceed 8 characters.
**	2.  All file names start with "iivdb" (BASE_LOG)
**	3.  The 6th letter of each file name is a class code, as follows:
**		Core Catalogs (except iirelation)   'C'
**		iirelation core catalog		    'R'
**		iidbdb-only catalogs		    'D'
**		query processing catalogs	    'P'
**		query modification catalogs	    'M'
**		optimizer/statictics catalogs	    'S'
**	4.  The 7th and 8th characters of the filename are a numberic code
**	    ranging from 00 to 2F (defined via constants ID_00 to ID_2F).
**	5.  The extenstion is the dba name (input parameter ARG_V[2])
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: logfile
**  History:
**	17-jun-93 (teresa)
**	    Initial Creation.
*/
name_log(class_code,id)
char	*class_code;
char	*id;
{
    STpolycat( 5,
		BASE_LOG, class_code, id, EXT_POS, dba,
		logfile
	     );
}

/*
** open_db -- open a database
**
** Description:
**	connects to a database as the DBA with +Y priv.
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	08-jun-93 (teresa)
**	    initial creation.
**      11-jan-94 (anitap)
**          Corrected typo: conenct -> connect in SIprintf error message text.
**	05-mar-94 (teresa)
**	    "+Y" -> '+Y' because of change to FE that no longer accepts "+Y"
*/
open_db()
{

    if (test_status == TEST_ABORT)
	return;
    else
    {
#ifdef DEBUG
	SIprintf ("connecting to database %s\n as user %s\n", dbname, dba);
#endif
	exec sql connect :dbname identified by :dba options = '+Y';
	/* if connection fails, then quit */
	if (sqlca.sqlcode < 0)
	{
	    test_status = TEST_ABORT;
	    SIprintf (" FATAL ERROR: unable to connect to database.\n");
	}
    }
}

/*
** open_A9db -- open a database with +Y and -A9
**
** Description:
**	connects to a database as the DBA with +Y and -A9 flags.
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	31-jan-94 (andre)
**	    written
**	05-mar-94 (teresa)
**	    "+Y" -> '+Y' because of change to FE that no longer accepts "+Y"
**	    Also change "-A9" to '-A9'
*/
open_A9db()
{

    if (test_status == TEST_ABORT)
	return;
    else
    {
#ifdef DEBUG
	SIprintf ("connecting to database %s\n as user %s\n", dbname, dba);
#endif
	exec sql connect :dbname identified by :dba options = '+Y','-A9';
	/* if connection fails, then quit */
	if (sqlca.sqlcode < 0)
	{
	    test_status = TEST_ABORT;
	    SIprintf (" FATAL ERROR: unable to conenct to database.\n");
	}
    }
}

/*
** open_dbdb -- open iidbdb database
**
** Description:
**	connects to iidbdb as '$ingres' with +Y priv.
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	19-jun-93 (teresa)
**	    initial creation.
**      11-jan-94 (anitap)
**          Corrected typo: conenct -> connect in SIprintf error message text.
**	05-mar-94 (teresa)
**	    "+Y" -> '+Y' because of change to FE that no longer accepts "+Y"
*/
open_dbdb()
{

    if (test_status == TEST_ABORT)
	return;
    else
    {
#ifdef DEBUG
	SIprintf ("connecting to database iidbdb as user $ingres\n");
#endif
	exec sql connect iidbdb identified by '$ingres' options = '+Y';
	/* if connection fails, then quit */
	if (sqlca.sqlcode < 0)
	{
	    test_status = TEST_ABORT;
	    SIprintf (" FATAL ERROR: unable to connect to iidbdb database.\n");
	}
    }
}

/*
** open_db_as_user -- open a database as the specified user
**
** Description:
**	connects to a database as the specified user with +Y priv.
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	09-jun-93 (teresa)
**	    initial creation.
**      11-jan-94 (anitap)
**          Corrected typo: conenct -> connect in SIprintf error message text.
**	05-mar-94 (teresa)
**	    "+Y" -> '+Y' because of change to FE that no longer accepts "+Y"
*/
open_db_as_user(user)
char	*user;
{
    exec sql begin declare section;
	char	uname[DB_MAXNAME+1];
    exec sql end declare section;
    
    STcopy( user, uname);
    if (test_status == TEST_ABORT)
	return;
    else
    {

#ifdef DEBUG
	SIprintf ("connecting to database %s\n as user %s\n", dbname, dba);
#endif
	exec sql connect :dbname identified by :uname options = '+Y';
	/* if connection fails, then quit */
	if (sqlca.sqlcode < 0)
	{
	    test_status = TEST_ABORT;
	    SIprintf (" FATAL ERROR: unable to connect to database.\n");
	}
    }
}

/*
** opt_cats() - this routine tests the verifydb optimizer catalog checks/repairs
**
** Description:
**
** This routine tests the verifydb optimizer catalog checks/repairs.
** This includes checks/repairs on the following catalogs:
**	iistatistics
**	iihistogram
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**	Outputs to log files:   ii_config:iivdbs01.DBA to
**				ii_config:iivdbs02.DBA
**	    where DBA is the value of the DBA name input parameter
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
*/
opt_cats()
{
    SIprintf("Testing Verifydb Check/Repairs On Optimizer Catalogs\n");
    SIflush(stdout);

    if (test_status != TEST_ABORT)
    {
	name_log(STAT_CODE,ID_01);
    	ck_iistatistics();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(STAT_CODE,ID_02);
	ck_iihistogram();
    }
}

/*
** printargs -- print arguments for verifydb's test executive
**
** Description:
**	If the user does not give all required parameters, this routine
**	identifies what they should be 
** Inputs:
**	None.
** Outputs;
**	None.
** History:
**	08-jun-93 (teresa)
**	    initial creation.
*/
printargs()
{
    SIprintf("\n VERIFYDB TEST SUITE.  Arguments:\n");
    SIprintf("	argv[0] = program pathname (default of operating system)\n");
    SIprintf("	argv[1] = database name\n");
    SIprintf("	argv[2] = dba name\n");
    SIprintf("	argv[3] = user name\n");
    SIprintf("	argv[4] = test inst:\n");
    SIprintf("		  NULL - DEFAULTS to all verifydb tests\n");
    SIprintf("	          '1'  - all verifydb tests\n");
    SIprintf("		  '2'  - core catalog tests only\n");
    SIprintf("		  '3'  - qrymod catalog tests only\n");
    SIprintf("		  '4'  - qp catalog tests only\n");
    SIprintf("		  '5'  - optimizer catalog tests only\n");
    SIprintf("		  '6'  - dbdb catalog tests only\n\n");
    SIprintf("	          '10' - debug mode\n");
}

/*
** qp_cats() - this routine tests the verifydb query processing catalog
**	       checks/repairs
**
** Description:
**
** This routine tests the verifydb query processing catalog checks/repairs.
** This includes checks/repairs on the following catalogs:
**	iidbdepends
**	iievent
**	iiprocedure
**	iirule
**	iixdbdepends
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**	Outputs to log files:   ii_config:iivdbp01.DBA to
**				ii_config:iivdbp05.DBA
**	    where DBA is the value of the DBA name input parameter
**  History:
**	16-jun-93 (teresa)
**	    Initial creation.
**	27-jan-94 (andre)
**	    added calls to ck_iipriv(), ck_iixpriv(), ck_iixevent(), and
**	    ck_ooxprocedure()
*/
qp_cats()
{
    SIprintf("Testing Verifydb Check/Repairs On Query Processing Catalogs\n");
    SIflush(stdout);

    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_01);
	ck_iidbdepends();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_02);
	ck_iievent();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_03);
	ck_iiprocedure();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_04);
	ck_iirule();
    }
/* currently the server does not allow us to update secondary indexes, so
** we cannot test this
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_05);
	ck_iixdbdepends();
    }
*/
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_06);
	ck_iigw06_attribute();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_07);
	ck_iigw06_relation();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_08);
	ck_iipriv();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_09);
	ck_iixpriv();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_0A);
	ck_iixevent();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QP_CODE,ID_0B);
	ck_iixprocedure();
    }
}

/*
** qrymod_cats() - this routine tests the verifydb qrymod catalog checks/repairs
**
** Description:
**
** This routine tests the verifydb qrymod catalog checks/repairs.  This includes
** checks/repairs on the following catalogs:
**	iidefault
**	iiintegrity
**	iikey
**	iiprotect
**	iiqrytxt
**	iisynonym
**	iitree
**	iidbms_comment
**	iisecalarm
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**	    is set by subordinate routines.
**	Outputs to log files:   ii_config:iivdbm01.DBA to
**				ii_config:iivdbm06.DBA
**	    where DBA is the value of the DBA name input parameter
**  History:
**	08-jun-93 (teresa)
**	    Initial creation.
**	18-sep-93 (teresa)
**	    added support for iidbms_comment
**	22-dec-93 (robf)
**          add support for iisecalarm
**	13-jan-94 (anitap)
**	    added support for iikey and iidefault.
**	27-jan-94 (andre)
**	    added call to ck_iixprotect
**	30-Dec-2004 (schka24)
**	    remove same
*/
qrymod_cats()
{

    SIprintf("Testing Verifydb Check/Repairs On QryMod Catalogs\n");
    SIflush(stdout);

    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_01);
	ck_iiintegrity();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_02);
	ck_iiprotect();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_03);
	ck_iiqrytext();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_04);
	ck_iisynonym();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_05);
	ck_iitree();
    }
    if (test_status != TEST_ABORT)
    {
	name_log(QRYMOD_CODE,ID_06);
	ck_iidbms_comment();
    }
    if (test_status != TEST_ABORT)
    {
        name_log(QRYMOD_CODE,ID_07);
        ck_iikey();
    }
    if (test_status != TEST_ABORT)
    {
        name_log(QRYMOD_CODE,ID_08);
        ck_iidefault();
    }
}

/*
** report_vdb -- run verifydb (in report mode) via PCcmdline
**
** Description:
**	constructs a command line to run verifydb in report mode and to redirect
**	the output log file to ii_config:iitstvdb.<DBA>  Then calls PCcmdline to
**	execute that command.
** Inputs:
**	Global variable:    dbname  - name of database to open
**	Global variable:    dba	    - name of DBA
** Outputs;
**	Global variable:    test_status	- TEST_OK or TEST_ABORT
** History:
**	08-jun-93 (teresa)
**	    initial creation.
**	13-jul-93 (teresa)
**	    added support for sep flag.
**	23-dec-93 (teresa)
**	    added support for suppress flag.
*/
report_vdb()
{
    char	    cmd[200];
    CL_ERR_DESC	    errdesc;		/* used for PCcmdline() calls */
    char	    sup_cmd[30];
    char	    num_buf[15];
    
    if (test_status == TEST_ABORT)
	return;    

    if (suppress == DO_NOT_SUPPESS)
	STcopy (" ",sup_cmd);
    else if (suppress == SUPPRESS_ALL)
	STcopy (" -dbms_test",sup_cmd);
    else
    {
	CVlx((u_i4)(suppress-E_DUF_MASK), num_buf);
	STpolycat( 2, " -dbms_test", num_buf, sup_cmd);
    }
    
    if (sep)
	STpolycat( 4,
		    "verifydb -mreport -odbms -sdbn ",dbname,
		    " -sep", sup_cmd, cmd);
    else
	STpolycat( 5,
		    "verifydb -mreport -odbms -sdbn ",dbname,
		    " -test -lf", logfile, sup_cmd,
		    cmd);

    if (PCcmdline(NULL, cmd, (bool)PC_WAIT, NULL, &errdesc) != OK)
    {
	test_status = TEST_ABORT;
	SIprintf("...ERROR!  Unable to run verifydb.\n");
    }

}

/* run_test()	-  determine if specified test should be run.
**
**  Description:
**
**  Translates the value of argv[5] (subtest) from a numeric value as per
**  below table to see if it applies to this specific test.  Is given
**  an input of catalog (a code indicating which catalog) and test number.
**  It checks to see if the given test number should be run for the given
**  catalog based on the value of argv[5], which is stored in global variable
**  subtest.  If subtest is zero, then run all tests.  Otherwise run only
**  specified test.
**  Another enhancement: if an even multiple of 100 is chosen, it will run
**  all the tests for a given catalog.  If mod(subtest,100) is not zero, then it
**  runs only the specified test.  For example:
**	if argv[4] specifies qrymod tests and subtest is 200, then ALL
**	    the iiprotect tests will be run
**  However
**	if argv[4] specifies qrymod tests and subtest is 201, then ONLY
**	    iiprotect test #1 will be run
**  Futhermore
**	if argv[4] specifies qrymod tests and subtest is 0, then
**	    ALL iiprotect tests will be run
**
**			CORE:   101 to 127 - run iirelation test 1 to 29
**					   currently there is no test 26 or 28
**				201 - run iirel_idx test 1
**				301 to 302 - run iiattribute test 1 to 2
**				401 to 403 - run iiindex test 1 to 3
**				501 to 502 - run iidevices test 1 to 2
**			QRYMOD: 101 to 104 - run iiintegrity tests 1 to 4
**			        201 to 204 - run iiprotect tests 1 to 4
**			        301 to 306 - run iiqrytext tests 1 to 6
**			        401 to 402 - run iisynonym tests 1 to 2
**			        501 to 506 - run iitree tests 1 to 6
**			        601 to 605 - run iidbms_comment tests 1 to 5
**				701 to 704 - run iisecalarm tests 1 to 4
**			QP:	101 to 104 - run iidbdepends tests 1 to 4
**					    currently iidbdepepends test 5 is
**					    not runnable.
**			        201 to 202 - run iievent tests 1 to 2
**			        301 to 302 - run iiprocedure tests 1 to 2
**			        401 to 405 - run iirule tests 1 to 5
**			        501 - run iixdbdepends test 1
**					     currently above test isn't runable
**				601 to 603 - run iigw06_attribute tests 1 to 3
**				701 to 703 - run iigw06_relation tests 1 to 3
**			OPT:    101 to 105 - run iistatistics tests 1 to 5
**			        201 to 202 - run iihistogram tests 1 to 2
**			IIDBDB: 101 to 102 - run iicdbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        201 to 215 - run iidatabase tests 1 to 15
**			        301 to 302 - run iidbid_idx tests 1 to 2
**					     currently above test isn't runable
**			        401 to 40~ - run iidbpriv tests 1 to ~
**			        501 to 50~ - run iiextend tests 1 to ~
**			        601 to 60~ - run iilocations tests 1 to ~
**			        701 to 70~ - run iistar_cdbs tests 1 to ~
**			        801 to 80~ - run iiuser tests 1 to ~
**			        901 to 90~ - run iiusergroup tests 1 to ~
**
** Inputs:
**	Catalog			A numeric code specifying which catalog is
**				being operated on.
**	testid			a numeric code indaciting which test is being
**				run
** Outputs;
**	RETURNS:    TRUE	if test should be run
**		    FALSE	if test should be skipped
** History:
**	22-jun-93 (teresa)
**	    initial creation.
**	2-aug-93 (stephenb)
**	    Added checks for iigw06_relation and iigw06_attribute tests.
**	18-sep-93 (teresa)
**	    Added Support for iidbms_comment
**	22-dec-93 (teresa)
**	    added call to ck_suppress
**	27-jan-94 (andre)
**	    rewrote to use symbolic constants, added checks for IIXPRIV, IIPRIV,
**	    IIXEVENT, IIXPROCEDURE, IIXPROTECT
**	02-feb-94 (huffman) && (mhuishi)
**	    A conditional was_too_complex for the AXP C compiler to optimize.
**	    This is the programming 101 fix around it.
*/

run_test(catalog, testid)
i4	    catalog;
i4	    testid;
{
    bool	retstat;
    i4	test_base;
    
    switch (catalog)
    {
	case IIRELATION:
	case IIREL_IDX:
	case IIATTRIBUTE:
	case IIINDEX:
	case IIDEVICES:
	    test_base = catalog - CORE_CATALOGS;
	    break;
	case IIINTEGRITY:
	case IIPROTECT:
	case IIQRYTEXT:
	case IISYNONYM:
	case IITREE:
	case IIDBMS_COMMENT:
	case IIKEY:
	case IIDEFAULT:
	case IISECALARM:
	    test_base = catalog - QRYMOD_CATALOGS;
	    break;
	case IIDBDEPENDS:
	case IIEVENT:
	case IIPROCEDURE:
	case IIRULE:
	case IIXDBDEPENDS:
	case IIGW06_ATTRIBUTE:
	case IIGW06_RELATION:
	case IIPRIV:
	case IIXPRIV:
	case IIXEVENT:
	case IIXPROCEDURE:
	    test_base = catalog - QP_CATALOGS;
	    break;
	case IISTATISTICS:
	case IIHISTOGRAM:
	    test_base = catalog - OPTIMIZER_CATALOGS;
	    break;
	case IICDBID_IDX:
	case IIDATABASE:
	case IIDBID_IDX:
	case IIDBPRIV:
	case IIEXTEND:
	case IILOCATIONS:
	case IISTAR_CDBS:
	case IIUSER:
	case IIUSERGROUP:
	case IIPROFILE:
	    test_base = catalog - IIDBDB_CATALOGS;
	    break;
	default:
	    test_base = -1;
	    break;
    }

    /*
    ** test will be run if catalog id is one of those we know about, and one of 
    ** the following is true:
    **	- we were told to run all tests on catalogs withing this group 
    **	  (subtest == 0),
    **	- we were told to run all tests on this catalog (subtest == test_base),
    **	- we were told to run the specified test (subtest == test_base + testid)
    */
    retstat = FALSE;
    if (   test_base != -1 )
	 if (   subtest == 0
	     || subtest == test_base 
	     || subtest == test_base + testid)
	    retstat = TRUE;

    if (retstat)
	ck_suppress(catalog, testid);
    else
	suppress = DO_NOT_SUPPESS;

    return (retstat);
}

/*
** run_vdb -- run verifydb (in TEST_RUNI mode) via PCcmdline
**
** Description:
**	constructs a command line to run verifydb in test_runi mode and to
**	redirect the output log file to ii_config:iitstvdb.<DBA>  Then calls
**	PCcmdline to execute that command.
** Inputs:
**	Global variable:    dbname  - name of database to open
**	Global variable:    dba	    - name of DBA
** Outputs;
**	Global variable:    test_status	- TEST_OK or TEST_ABORT
** History:
**	08-jun-93 (teresa)
**	    initial creation.
**	13-jul-93 (teresa)
**	    added support for sep flag.
**	23-dec-93 (teresa)
**	    added support for suppress flag.
*/
run_vdb()
{
    char	    cmd[200];
    CL_ERR_DESC	    errdesc;		/* used for PCcmdline() calls */
    char	    sup_cmd[30];
    char	    num_buf[15];

    if (test_status == TEST_ABORT)
	return;    

    if (suppress == DO_NOT_SUPPESS)
	STcopy (" ",sup_cmd);
    else if (suppress == SUPPRESS_ALL)
	STcopy (" -dbms_test",sup_cmd);
    else
    {
	CVlx((u_i4)(suppress-E_DUF_MASK), num_buf);
	STpolycat( 2, " -dbms_test", num_buf, sup_cmd);
    }

    if (sep)
	STpolycat( 4,
		    "verifydb -mtest_runi -odbms -sdbn ",dbname,
		    " -sep", sup_cmd, cmd);
    else
	STpolycat( 5,
		    "verifydb -mtest_runi -odbms -sdbn ",dbname,
		    " -test -lf", logfile, sup_cmd,
		    cmd);

    if (PCcmdline(NULL, cmd, (bool)PC_WAIT, NULL, &errdesc) != OK)
    {
	test_status = TEST_ABORT;
	SIprintf("...ERROR!  Unable to run verifydb.\n");
    }

}

/* xlate_string	- translate ascii string to decimal
**
**  Description:
**	This routine translates a null terminated string from ascii to hex.
**	The string must not be more than MAX_DIGITS+1 long, where the last
**	character is a null termination.  All characters in the string must
**	be ascii representations of digits with values between '0' and '9'.
**  Inputs:
**	op_code		    Op code string to translate
**  Outputs:
**	none
**  History
**	08-jun-93 (teresa)
**	    Initial creation.
*/
xlate_string(num_string, number)
char	    *num_string;
int	    *number;
{
	int	value;
	int	num_digits;
	int	i;

	do 
	{
	    for (num_digits= 0; num_digits<= MAX_DIGITS; num_digits++)
	    {   
		if (num_string[num_digits] == '\0')
		    break;
	    }
	    
	    if ( 
		 (num_digits == MAX_DIGITS) && (num_string[num_digits] != '\0')
	       ||
		 num_digits <= 0
	       )
	    {
		/* op code is too long */
		value = INVALID_OPCODE;
		break;
	    }	
	
	    /* xlate ASCII to decimal */	
	    for (i=0, value=0; i < num_digits; i++)
	    {
		value *= 10;
		switch (num_string[i])
		{
		case '1':
		    value += 1;
		    break;
		case '2':
		    value += 2;
		    break;	    
		case '3':
		    value += 3;
		    break;
		case '4':
		    value += 4;
		    break;
		case '5':
		    value += 5;
		    break;
		case '6':
		    value += 6;
		    break;
		case '7':
		    value += 7;
		    break;
		case '8':
		    value += 8;
		    break;
		case '9':
		    value += 9;
		    break;
		}
	    }	    	    

	} while (0);  /* control loop */
	*number = value;    
}

/* this is the basis to build your routine on */

/*
** dummy() - 
**
** Description:
**
**  
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
*/
dummy()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog ii*****..\n");
    SIflush(stdout);
    STcopy("ii*****",test_id);
    do
    {
	if ( run_test(II,1) )
	{
	    SIprintf ("....ii***** test #..\n");
	    SIflush(stdout);

	    /*************/
	    /** test #  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    /* exec sql <<statements to set up for test go here >> */
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    /* exec sql delete <<statements to clean up from test go here>> */
	    close_db();
	}

    } while (0);  /* control loop */
}
/*
** ck_iisecalarm() - 
**
** Description:
**
**  This routine tests the verifydb iidbms_comment catalog checks/repairs.
**
**  Inputs:
**	None.
**  Outputs:
**	global variable: test_status	- status of tests: TEST_OK, TEST_ABORT
**  History:
**	22-dec-93  (robf)
**	    Initial Creation.
*/
ck_iisecalarm()
{
    EXEC SQL begin declare section;
	i4 id1;
	i4 id2;
	i4 val;
    EXEC SQL end declare section;


    SIprintf ("..Testing verifydb checks/repairs for catalog iisecalarm..\n");
    SIflush(stdout);
    STcopy("iisecalarm",test_id);
    do
    {
	if ( run_test(IISECALARM,1) )
	{
	    SIprintf ("....iisecalarm test 1..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 1  **/
	    /*************/

	    /** setup **/
	    open_db();
	    exec sql create table duve_secal1 (name c10);
	    exec sql create dbevent duve_secal1 ;
	    exec sql create security_alarm duve_secal1 on table duve_secal1
				raise dbevent duve_secal1;
	    exec sql update iisecalarm set obj_id1= -1, obj_id2= -1;
	    if (test_status == TEST_ABORT)
		break;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_secal1;
	    exec sql drop dbevent duve_secal1;
	    close_db();
	}

	if ( run_test(IISECALARM,2) )
	{
	    SIprintf ("....iisecalarm test 2..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 2  **/
	    /*************/

	    /** setup **/
	    open_db();
	    	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_secal1 (name c10);
	    exec sql create dbevent duve_secal1 ;
	    exec sql create security_alarm duve_secal1 on table duve_secal1
				raise dbevent duve_secal1;
	    id1=ALARM_BIT;
	    exec sql update iirelation set relstat2 = relstat2-:id1;
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_secal1;
	    exec sql drop dbevent duve_secal1;
	    close_db();
	}

	if ( run_test(IISECALARM,3) )
	{
	    SIprintf ("....iisecalarm test 3..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 3  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_secal1 (name c10);
	    exec sql create dbevent duve_secal1 ;
	    exec sql create security_alarm duve_secal1 on table duve_secal1
				raise dbevent duve_secal1;
	    exec sql delete from iiqrytext q where
		     exists (select *
			     from iisecalarm a
			     where a.alarm_name='duve_secal1'
			     and q.txtid1 = a.alarm_txtid1
			     and q.txtid2 = a.alarm_txtid2
		     );
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_secal1;
	    exec sql drop dbevent duve_secal1;
	    close_db();
	}

	if ( run_test(IISECALARM,4) )
	{
	    SIprintf ("....iisecalarm test 4..\n");
	    SIflush(stdout);

	    /*************/
	    /** test 4  **/
	    /*************/

	    /** setup **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql create table duve_secal1 (name c10);
	    exec sql create dbevent duve_secal1 ;
	    exec sql create security_alarm duve_secal1 on table duve_secal1
				raise dbevent duve_secal1;
	    exec sql update iisecalarm set event_id1= -1, event_id2= -1
		     where alarm_name='duve_secal1';
	    close_db();

	    /** run test **/
	    report_vdb();
	    run_vdb();

	    /** clean up **/
	    open_db();
	    if (test_status == TEST_ABORT)
		break;
	    exec sql drop table duve_secal1;
	    exec sql drop dbevent duve_secal1;
	    close_db();
	}

    } while (0);  /* control loop */
}
