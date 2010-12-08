/*
** Copyright (c) 1988, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<st.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<adf.h>
#include	<lo.h>
#include	<er.h>
#include	<pc.h>
#include	<si.h>
#include	<duerr.h>
#include	<duvfiles.h>
#include	<dudbms.h>
#include	<duve.h>
#include	<duustrings.h>
#include	<cs.h>
#include	<lg.h>
#include	<lk.h>
#include	<dm.h>
#include	<dmf.h>
#include	<dmp.h>
#include	<duucatdef.h>

    exec sql include SQLCA;	/* Embedded SQL Communications Area */

/**
**
**  Name: DUVECHK.SC - Embedded SQL for VERIFYDB
**
**  Description:
**      verifydb performs serveral functions.  One of them is to check/fix
**	the dbms system catalogs:  (iirelation, iirel_idx, iiattribute,
**	iidevices, iiindex, iiqrytext, iitree, iiprotect, iiintegrity,  
**	iidbdepends, iixdbdepends, iihistogram, iistatistics.)   This
**	function is evoked via a operation code of "dbms_catalogs"  Example:
**
**	    verifydb -mreport -sdbname "database_name" -odbms_catalogs
**
**	The system catalog tests are documented in the verifydb product spec.
**
**	Another verifydb function is to patch an inconsistent database to
**	appear consistent to the DBMS.  This is specified with an operation
**	code of "force_consistent"  Example:
**
**	    verifydb -mrun -sdbname "database_name" -oforce_consistent
**
**	Another verifydb function is to drop a table from the dbms system
**	catalogs (without checking for the existance of an associated data
**	file).  This quick and dirty drop does not clean up associated views
**	integrities, permits, etc.  To clean these up, run verifydb's system
**	catalog check/repair option.  ( -odbms_catalogs).
**
**	The ESQL routines to open/close a database, handle INGRES errors,
**	and perform the system catalogs checks (with or without their associated
**	system catalog  patches) is contained here.
**
**	All of the functions in this module rely on data contained in the
**	verifydb control block (duve_cb).  This control block contains the
**	following information:
**	    information about input parameters:
**		mode	(RUN, RUNSILENT, RUNINTERACTIVE, REPORT)
**		scope	(DBNAME, DBA, INSTALLATION)
**		lists of databases to operate on
**		operation (DBMS_CATALOGS, FORCE_CONSISTENT, etc)
**		user code/name (-U followed by user name)
**	        special (undocumented) flags (-r,-d)
**	    information about the verifydb task environment:
**		name of user who evoked task
**		name of utility (VERIFYDB)
**		error message block
**	    ESQL specific information:
**		name of current open database
**	    system catalog check specific information:
**		pointer to duverel array
**		duvedpnds structure
**		number of elements of duverel actually containing valid data
**		DUVEREL element contains iirelation info:
**		    table id (base and index id)
**		    table name
**		    owner name
**		    table's status word
**		    # attributes in table
**		    flag indicating if this table is to be dropped from database
**
**	Special ESQL include files are used to describe each dbms system
**	catalog.  These include files are generated via the DCLGEN utility,
**	and require a dbms server to be running.  The following commands create
**	the include files in the format required by verifydb:
**	    dclgen c iidbdb iiattribute duveatt.sh att_tbl
**	    dclgen c iidbdb iidevices duvedev.sh  dev_tbl
**	    dclgen c iidbdb iiindex duveindex.sh index_tbl
**	    dclgen c iidbdb iirelation duverel.sh rel_tbl
**	    dclgen c iidbdb iirel_idx duveridx.sh ridx_tbl
**
**	DUVECHK.SC contains the following functions:
**          duve_opendb   - opens a database
**          duve_close    - closes a database
**          duve_force    - forces a database to appear consistent to dbms
**          fixrelstat    - clears the TCB_ZOPTSTAT bit from iirelation's relstat
**			    if there is not any optimizer statistics associated
**			    with this table
**          duve_catalogs - performs system catalog checks on all dbms catalogs
**	    duve_hcheck	  - verify that it is possible to connect to specified dbs
**	    duve_tbldrp   - drops tuples from the dbms system catalogs for the
**			    specified table.
**          ckrel	  - performs system catalog checks on iirelation table
**          ckrelidx	  - performs system catalog checks on iirel_idx table
**          ckatt	  - performs system catalog checks on iiattribute table
**          ckindex	  - performs system catalog checks on iiindex table
**          ckdevices	  - performs system catalog checks on iidevices table
**	    Clean_up	  - INGRES error handling routine
**	    test_1	  - performs verifydb iirelation test 1
**	    test_2	  - performs verifydb iirelation tests 2a and 2b
**	    test_3	  - performs verifydb iirelation test 3
**	    test_4	  - performs verifydb iirelation test 4
**	    test_5	  - performs verifydb iirelation tests 5a and 5b
**	    test_6	  - performs verifydb iirelation test 6
**	    test_7	  - performs verifydb iirelation test 7
**	    test_8	  - performs verifydb iirelation test 8
**	    test_9	  - performs verifydb iirelation test 9
**	    test_10	  - performs verifydb iirelation test 10
**	    test_11	  - performs verifydb iirelation test 11
**	    test_12	  - performs verifydb iirelation test 12
**	    test_13	  - performs verifydb iirelation test 13
**	    test_14	  - performs verifydb iirelation test 14
**	    test_15	  - performs verifydb iirelation test 15
**	    test_16	  - performs verifydb iirelation tests 16 and 17
**	    test_18	  - performs verifydb iirelation test 18
**	    test_19	  - performs verifydb iirelation test 19
**	    test_20	  - performs verifydb iirelation test 20
**	    test_21	  - performs verifydb iirelation test 21
**	    test_22	  - performs verifydb iirelation test 22,23,24 & 25
**	    test_26	  - performs verifydb iirelation test 26
**	    test_27	  - performs verifydb iirelation test 27 	
**	    test_28	  - performs verifydb iirelation test 28
**	    test_29	  - Performs verifydb iirelation test 29
**	    test_30_and_31 - - Performs verifydb iirelation test2 30 and 31
**                   tests 32-93 are performed elsewhere
**	    test_95       - Performs verifydb iirelation test 95
**
**  History:
**      15-aug-88 (teg)
**          initial creation for verifydb phase 1
**	2-Nov-88 (teg)
**	    added drop_tbl operation (for bug 2344)
**	8-Feb-89 (teg)
**	    Moved duve_dbdb() to module duvedbdb.sc (for VERIFYDB PHASE II).
**	    Also changed duve_catalogs() to call logic for iidbdb checks/fixes.
**	16-Jun-89 (teg)
**	    added #include dudbms.h to get constant DU_FORCED_CONSISTENT, which
**	    was removed from duve.h to avoid unix compiler warnings.
**      15-feb-90 (teg)
**          added TABLE/XTABLE operation support.
**	30-apr-90 (teg)
**	    ingres6202p 131098 complained of nonexistent duve_fixrelstat() in
**	    static declarations.  Changed to correct routine name of
**	    fixrelstat()
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**`	    calls have ().
**	16-apr-92 (teresa)
**	    pick up ingres63p change 265521 for OS2 port:
**		errno -> errnum, NULL -> 0
**	17-may-92 (ananth)
**	    Increasing IIrelation tuple size project.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks,
**	    also added support for a few new routines: ckrule, ckproc, etc.
**	16-jun-93 (anitap)
**	    Added test_28() to check for schema for the relation/view.
**	    Added test 3 to ckatt() for FIPS constraints.	
**	26-jul-1993 (bryanp)
**	    Add includes of <lk.h> and <pc.h> to support include of dmp.h. I
**	    would prefer that this file did NOT include dmp.h. Hopefully someday
**	    that will be changed.
**	3-aug-93 (stephenb)
**	    Added calls to ckgw06att() and ckgw06rel() in duve_catalogs(), also
**	    added new routine test_29() for iirelation test #29
**	8-aug-93 (ed)
**	    unnest dbms.h
**	18-sep-93 (teresa)
**	    Added iidbms_comments catalog to list of catalogs verifydb checks.
**	10-oct-93 (anitap)
**	    Added check in ckindex() for corresponding iidbdepends tuple.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (anitap)
**	    Use repeat queries and ANY instead of COUNT() to improve performance
**	    in ckatt(), test_28(). Also added corrective actions to create
**	    schemas for orphaned objects and alter table drop/add constraint.
**	    use STtrmwhite() instead of STzapblank() which gets rid of embedded
**	    blanks also. Change for delimited identifiers in duve_catalogs()
**	    and ckdevices().
**	08-jan-94 (anitap)
**	    Corrected typos in duve_d_cascade(), int_qryid1 to intqryid1 and
**	    int_qryid2 to intqryid2.
**	    use S_DU0318_DROP_INDEX & S_DU0368_DROP_INDEX instead of 
**	    S_DU0302_DROP_TABLE and S_DU0352_DROP_TABLE in ckindex() test 4
**	    since we are dropping indexes and not base tables.
**      03-feb-95 (chech02)
**          After 'select count(*) into :cnt from table' stmt, We need to 
**          check cnt also.
**	05-apr-1996 (canor01)
**	    test_29()--must pass iirelation.relid.db_tab_name instead of just
**	    iirelation.relid, since NT compiler pushes structure on
**	    the stack, not just a pointer.
**	05-apr-1996 (canor01)
**	    Set duve_cb->duvetree to NULL after MEfree() in duve_close().
**      22-Aug-1997 (wonst02)
**          Added parentheses-- '==' has higher precedence than bitwise '&':
**	    if ( (iirelation.relstat & TCB_INDEX) == 0), not:
**	    if ( iirelation.relstat & TCB_INDEX == 0)
**      28-Jul-1998 (carsu07)
**          When verifying the location name, check if the name is blank. If so
**          then this is a dummy row in iidevices, caused by modifying a table
**          over multiple locations followed by a modify over a reduced number
**          of locations. (Bug 90146)
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**    13-Mar-2000 (fanch01)
**        Change iidevice query in test_20() to count active devloc entries
**        correctly for bug number 100875.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-may-2004 (stial01)
**          Fixed up references to tid host variables. (b112259, b112266)
**	19-jan-2005 (abbjo03)
**	    Change duvecb to GLOBALREF.
**      23-feb-2005 (stial01)
**          Explicitly set isolation level after connect
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Test 15 removed.  tests for existence of view if TCB_VBASE is set.
**          TCB_VBASE not being cleared even when view is dropped and not tested
**          else where other than verifydb tests.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      12-oct-2009 (joea)
**          Add cases for DB_DEF_ID_FALSE/TRUE in ckatt.
**      10-Feb-2010 (maspa05) b122651
**          Added test_94 - test that all mandatory relstat, relstat2 flags are 
**          set as defined in dub_mandflags. 
**          Added test_95 - test that if TCB2_PHYSLOCK_CONCUR is set that the
**          structure is hash.
**	3-May-2010 (kschendel) SIR 123639
**	    Pull test 94, we're leaving it up to DMF now.
**	30-Nov-2010 (kschendel)
**	    Fix for new spatial catalogs which have externally prescribed
**	    names (bah, humbug!) and don't start with ii.
**/


/*
**  Forward and/or External typedef/struct references.
*/

    GLOBALREF DUVE_CB duvecb;
 
/*
[@type_reference@]...
*/

/*
**  Forward and/or External function references.
*/
    /* external */
FUNC_EXTERN i2	    duve_basecnt();	/* counts # secondary indexes for a
					** specified base table. */
FUNC_EXTERN i4 duve_findreltid();  /* return duverel element offset that
					** matches specified table id */
FUNC_EXTERN DU_STATUS duve_log();	/* write string to log file */
FUNC_EXTERN DU_STATUS duve_banner();	/* write test banner to log file */
FUNC_EXTERN i4 duve_relidfind();	/* return duverel element element that
					**  matches specified table name */
FUNC_EXTERN DU_STATUS duve_talk();      /* verifydb communications routine */
FUNC_EXTERN VOID init_stats(DUVE_CB *duve_cb, i4  test_level);
FUNC_EXTERN VOID printstats(DUVE_CB *duve_cb, i4  test_level, char *id);

    /* forward */
FUNC_EXTERN DU_STATUS ckcomments();	/* check/fix iidbms_comments */
FUNC_EXTERN DU_STATUS ckdbdep();	/* check/fix iidbdepends */
FUNC_EXTERN DU_STATUS ckxpriv( 		/* check/fix iixpriv */
			      DUVE_CB *duve_cb);
FUNC_EXTERN DU_STATUS ckpriv(		/* check/fix iipriv */
			     DUVE_CB *duve_cb);
FUNC_EXTERN DU_STATUS ckxevent(		/* check/fix iixevent */
			       DUVE_CB *duve_cb);
FUNC_EXTERN DU_STATUS ckxproc(  	/* check/fix iixproc */
			      DUVE_CB *duve_cb);
FUNC_EXTERN DU_STATUS ckdefault();	/* check/fix iidefault */
FUNC_EXTERN DU_STATUS ckevent();	/* check/fix iievent */	
FUNC_EXTERN DU_STATUS ckgw06att();	/* check/fix iigw06_attribute */
FUNC_EXTERN DU_STATUS ckgw06rel();	/* check/fix iigw06_relation */
FUNC_EXTERN DU_STATUS ckhist();		/* check/fix iihistogram */
FUNC_EXTERN DU_STATUS ckintegs();	/* check/fix iiintegrity */
FUNC_EXTERN DU_STATUS ckkey();		/* check/fix iikey */
FUNC_EXTERN DU_STATUS ckproc();		/* check/fix iiprocedure */
FUNC_EXTERN DU_STATUS ckprotups();	/* check/fix iipermit */
FUNC_EXTERN DU_STATUS ckqrytxt();	/* check/fix iiqrytext */
FUNC_EXTERN DU_STATUS ckrule();		/* check/fix iirule */
FUNC_EXTERN DU_STATUS ckstat();		/* check/fix iistatistics */
FUNC_EXTERN DU_STATUS cksynonym();	/* check/fix iisynonym */
FUNC_EXTERN DU_STATUS cktree();		/* check/fix iitree */
FUNC_EXTERN DU_STATUS ckxdbdep();	/* check/fix iixdbdepends */
FUNC_EXTERN DU_STATUS cksecalarm();	/* check/fix iisecalarm */
FUNC_EXTERN VOID Clean_Up();		/* handle INGRES error */
FUNC_EXTERN DU_STATUS duve_catalogs();	/* executive for sys catalog check */
FUNC_EXTERN DU_STATUS duve_tbldrp();	/* executive to drop specified table*/
FUNC_EXTERN VOID duve_close();		/* close database */
FUNC_EXTERN DU_STATUS duve_force();     /* force database consistent */
FUNC_EXTERN DU_STATUS duve_hcheck();	/* check that specified db is healthy*/
FUNC_EXTERN DU_STATUS duve_opendb();    /* open database */
FUNC_EXTERN VOID idxchk();		/* handle relidxcount if delete index */

static VOID
duve_d_cascade(
	     DUVE_CB	*duve_cb,
	     u_i4	base,
	     u_i4	idx);
static DU_STATUS ckatt();		/* check iiattribute catalog */
static DU_STATUS ckdevices();		/* check iidevices catalog */
static DU_STATUS ckindex();		/* check iiindex catalog */
static DU_STATUS ckrel();		/* check iirelation catalog */
static DU_STATUS ckrelidx();		/* check iirel_idx catalog */
static DU_STATUS fixrelstat();		/* clear TCB_ZOPTSTAT from relstat */
static DU_STATUS test_1();		/* verifydb iirelation test */
static DU_STATUS test_2();		/* verifydb iirelation test */
static DU_STATUS test_3();		/* verifydb iirelation test */
static DU_STATUS test_4();		/* verifydb iirelation test */
static DU_STATUS test_5();		/* verifydb iirelation test */
static DU_STATUS test_6();		/* verifydb iirelation test */
static DU_STATUS test_7();		/* verifydb iirelation test */
static DU_STATUS test_8();		/* verifydb iirelation test */
static DU_STATUS test_9();		/* verifydb iirelation test */
static DU_STATUS test_10();		/* verifydb iirelation test */
static DU_STATUS test_11();		/* verifydb iirelation test */
static DU_STATUS test_12();		/* verifydb iirelation test */
static DU_STATUS test_13();		/* verifydb iirelation test */
static DU_STATUS test_14();		/* verifydb iirelation test */
static DU_STATUS test_15();		/* verifydb iirelation test */
static DU_STATUS test_16();		/* verifydb iirelation test */
static DU_STATUS test_18();		/* verifydb iirelation test */
static DU_STATUS test_19();		/* verifydb iirelation test */
static DU_STATUS test_20();		/* verifydb iirelation test */
static DU_STATUS test_21();		/* verifydb iirelation test */
static DU_STATUS test_22();		/* verifydb iirelation test */
static DU_STATUS test_26();		/* verifydb iirelation test */
static DU_STATUS test_27();		/* verifydb iirelation test */
static DU_STATUS test_28();		/* verifydb iirelation test */
static DU_STATUS test_29();		/* verifydb iirelation test */
static DU_STATUS test_30_and_31();	/* verifydb iirelation test 30 & 31 */
static DU_STATUS test_95();		/* verifydb iirelation test */
static void create_schema();		/* create schema for orphaned object */
static DU_STATUS check_cons();		/* check if constraints present in sch
				  	** schema.
					*/

/* 
**  LOCAL CONSTANTS
*/
#define        CMDBUFSIZE      512

/*{
** Name: duve_catalogs	- executive for checking dbms catalogs for consistency
**			  or patching them to be consistent
**
** Description:
**      duve_catalogs serves as a high level executive to assure that all of
**	the tests (with or without associated repairs) specified  in the
**	verifydb product specification are performed.  Each routine that it
**	calls returns DUVE_YES if it completes successfully and DUVE_BAD
**	if it encounters an error.  If an error is encountered, then testing
**	stops.  Otherwise testing continues.
**
**	If all of the testing completes successfully, then duve_catalogs
**	walks the duverel structure indicated by the verifydb control block and
**	drops any tables that are marked for deletion.  Since Verifydb operates
**	on databases that are not guarenteed to be healthy, it does not use the
**	ESQL drop statement -- the dbms may not be able to accomplish this 
**	without error because it assumes certain relationships between catalogs
**	that is always found in a healthy database.  Instead, it deletes the
**	appropriate tuple(s) from all catalogs one at a time.
**
** Inputs:
**      database                        name of database to check/fix catalogs
**      duve_cb                         verifydb control block contains
**	    duverel			    array of rel entries
**		rel				info about an iirelation tuple:
**		    du_tbldrop			    flag indicating to drop table
**		    du_id				    table base and index identifiers
**		    du_own			    table's owner
**		    du_tab			    name of table
**
** Outputs:
**	none.
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be deleted from any of the dbms catalogs, if verifydb
**	    processing decides that they should be.
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	6=-Feb-89 (teg)
**	    added logic to check iidbdb catalogs if we are checking iidbdb.
**	12-jun-90 (teresa)
**	    added logic to check iisynonym
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**`	    calls have ().
**	19-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Fix bug that caused a loop to traverse beyond the bounds of the
**	    array duve_cb->duverel->rel[].
**	2-aug-93 (stephenb)
**	    Added calls to ckgw06rel() and ckgw06att() for iigw06_relation
**	    and iigw06_attribute.
**	18-sep-93 (teresa)
**	    Added calls to ckcomments to check iidbms_comments catalog.
**	06-dec-93 (andre)
**	    moved calls to ckdbdep() and ckxdbdep() to BEFORE call to ckindex()
**	    since test #4 in ckindex() relies on iidbdepends cache being 
**	    populated
**	20-dec-93 (robf)
**          Added call to cksecalarm() to check iisecalarm catalog.
**	25-jan-94 (andre)
**	    moved cleanup code into a new function - duve_cleanup()
**	29-Dec-2004 (schka24)
**	    Remove iixprotect.
[@history_template@]...
*/
DU_STATUS
duve_catalogs(duve_cb, database)
DUVE_CB		*duve_cb;
char		*database;
{
    i4		ctr;
    EXEC SQL BEGIN DECLARE SECTION;
	char		    name[DB_MAXNAME+1];
	char		    owner[DB_MAXNAME+1];
	u_i4		    tid,tidx;
	char                rname[DB_MAXNAME+1];
	char                rowner[DB_MAXNAME+1];
	char                text[240];
	char                rtext[240];
	char                cons[DB_MAXNAME+1];
	i4             cnt;
	char                *statement_buffer, buf[CMDBUFSIZE];
    EXEC SQL END DECLARE SECTION;
 
    if ( duve_opendb(duve_cb, database) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);

    if (duve_cb->duve_relstat)
    {   /* this logic behaves as though mode = runsilent.  It is an
        **  undocumented feature for in house use only.  The same
	**  function is performed in the convert to 6.1 logic for the
	**  customers.
	*/
	fixrelstat();
	duve_close(duve_cb,FALSE);
        return ( (DU_STATUS) DUVE_YES);
    }

    if ( ckrel(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckrelidx(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckatt(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckdbdep (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckxdbdep (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if (ckxpriv(duve_cb) == DUVE_BAD)
	return((DU_STATUS) DUVE_BAD);
    if (ckpriv(duve_cb) == DUVE_BAD)
	return((DU_STATUS) DUVE_BAD);
    if ( ckindex(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckdevices(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( cktree (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckintegs (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckprotups (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( cksecalarm (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckqrytxt (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckstat (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckhist (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( cksynonym (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if (ckxevent(duve_cb) == DUVE_BAD)
	return((DU_STATUS) DUVE_BAD);
    if ( ckevent (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if (ckxproc (duve_cb) == DUVE_BAD)
	return((DU_STATUS) DUVE_BAD);
    if ( ckproc (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckrule(duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckkey (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    if ( ckdefault (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);				
    if ( ckcomments (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    /*
    ** routine ckgw06att() should be called before ckgw06rel() since
    ** it may delete rows from iigw06_attribute that invalidate rows in
    ** iigw06_relation, this will be checked for in ckgw06rel()
    */
    if ( ckgw06att (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);				
    if ( ckgw06rel (duve_cb) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);				

    /*
    ** now process objects marked for destruction and any objects dependent on
    ** them
    */
    duve_d_cascade(duve_cb, (u_i4) 0, (u_i4) 0);

    MEfree( (PTR) duve_cb->duverel);
    duve_cb->duverel = NULL;

    for (ctr = 0; ctr < duve_cb->duve_ecnt; ctr++)
    {	
        STcopy (duve_cb->duveeves->events[ctr].du_schname, 
			duve_cb->duve_schema);

        create_schema(duve_cb);
    }	
    
    MEfree( (PTR) duve_cb->duveeves);
    duve_cb->duveeves = NULL;

    for (ctr = 0; ctr < duve_cb->duve_pcnt; ctr++)
    {	
        STcopy (duve_cb->duveprocs->procs[ctr].du_schname,
		duve_cb->duve_schema);

        create_schema(duve_cb);
    }	
    
    MEfree( (PTR) duve_cb->duveprocs);
    duve_cb->duveprocs = NULL;
    
    for (ctr = 0; ctr < duve_cb->duve_sycnt; ctr++)
    {	
        STcopy (duve_cb->duvesyns->syns[ctr].du_schname, 
			duve_cb->duve_schema);

        create_schema(duve_cb);
    }	
    
    MEfree( (PTR) duve_cb->duvesyns);
    duve_cb->duvesyns = NULL;

    /*
    ** see if there are any constraints to be dropped. Drop the constraits
    ** and recreate them. We could not drop and create them while the    
    ** cursor was on them, so do so now.
    */
    for (ctr = 0; ctr < duve_cb->duve_intcnt; ctr++)
    {
        
	if (duve_cb->duveint->integrities[ctr].du_dcons == DUVE_DROP)
        {

           STcopy (duve_cb->duveint->integrities[ctr].du_ownname, owner);
           STcopy (duve_cb->duveint->integrities[ctr].du_tabname, name);
           STcopy (duve_cb->duveint->integrities[ctr].du_consname, cons);
          
 	   /*
           ** drop constraint.
           */
           exec sql commit;

           statement_buffer = STpolycat(4, "set session authorization ", "\"",
                        owner, "\"", buf);
           exec sql execute immediate :statement_buffer;

           statement_buffer = STpolycat(7, "alter table ", name,
               " drop constraint ", "\"", cons, "\"", " cascade", buf);
           exec sql execute immediate :statement_buffer;
	}
	
	if (duve_cb->duveint->integrities[ctr].du_acons == DUVE_ADD)
	{

           STcopy (duve_cb->duveint->integrities[ctr].du_ownname, owner);
           STcopy (duve_cb->duveint->integrities[ctr].du_tabname, name);
           STcopy (duve_cb->duveint->integrities[ctr].du_txtstring, text);

           /*
           ** add constraint
           */
           exec sql commit;

           statement_buffer = STpolycat(4, "set session authorization ", "\"",
                        owner, "\"", buf);
           exec sql execute immediate :statement_buffer;
	   statement_buffer = STpolycat(4, "alter table ",
                       name, " add ", text, buf);
           exec sql execute immediate :statement_buffer;
	}

        /* check if we need to add referential contraint */
        if (duve_cb->duveint->integrities[ctr].du_rcons == DUVE_ADD)
        {
           STcopy (duve_cb->duveint->integrities[ctr].du_rownname, rowner);
           STcopy (duve_cb->duveint->integrities[ctr].du_rtabname, rname);
           STcopy (duve_cb->duveint->integrities[ctr].du_rtxtstring, rtext);
           
	   exec sql commit;

           statement_buffer = STpolycat(4, "set session authorization ", "\"",
                       rowner, "\"", buf);
           exec sql execute immediate :statement_buffer;

           statement_buffer = STpolycat(4, "alter table ",
                        rname, " add ", rtext, buf);
           exec sql execute immediate :statement_buffer;
        }
    }

    MEfree( (PTR) duve_cb->duveint);
    duve_cb->duveint = NULL;


    exec sql commit;
    exec sql set session authorization initial_user;	

    /* if we are checking the iidbdb database, check the installation specific
    ** catalogs as well.  (These routines reside in duvedbdb.sc)
    */
    
    if (STcompare( duve_cb->du_opendb, DU_DATABASE_DATABASE) == DU_IDENTICAL)
    {
	if( ckiidbdb(duve_cb) == DUVE_BAD)
	    return ( (DU_STATUS) DUVE_BAD);
    }

    duve_close(duve_cb,FALSE);
    
    return (DUVE_YES);
}

/*{
** Name: duve_close	- close an open database
**
** Description:
**      This routine turns off the system catalog update privalege if it was
**	on.  It either commits or aborts the transaction, depending on the
**	value of the abort flag input parameter.  Finally, it does a disconnect
**	from the open database, which is how ESQL closes a database.  Finally,
**	it clears the du_opendb field in the duve control block to indicate that
**	there is not an open database.
**
**	If duve_close is called when there is not an open database, indicated
**	by a null in duve_cb->du_opendb, then this routine is a no op.
**
**	This routine will deallocate the memory that was used to cache system
**	catalog inforation when it closes the database.
**
** Inputs:
**      duve_cb                         verifydb control block, contains
**	    du_opendb			    name of open database
**	    duve_operation		    operation (FORCE_CONSISTENT, etc)
**	    duverel			    pointer to cache for iirelation
**	    duvedpnds			    structure describing cache for 
**					    iidbdepends
**	    duvefixit			    structure describing objects which
**					    need to be dropped and schemas which
**					    need to be created
**	    duvestat			    pointer to cache for iistatisitcs
**      abort_flag                      flag indicating if duve_close should
**					    commit (value=false) or abort
**					    (value=true) this transaction.
**
** Outputs:
**      duve_cb                         verifydb control block, contains
**	    du_opendb			    name of open database, set to null
**	Returns:
**	    none.
**	Exceptions:
**	    none.
**
** Side Effects:
**	    none.
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-sep-88 (teg)
**	    added logic to deallocate iirelation cache when closing db
**	07-dec-93 (andre)
**	    duve_cb->duvedpnds is now longer a pointer to an array of 
**	    structures - instead, it is a structure from which two lists may 
**	    be hanging
**	18-jan-94 (andre)
**	    added code to release memory used for the "fix it" cache
**	05-apr-1996 (canor01)
**	    Set duve_cb->duvetree to NULL after MEfree.
*/
VOID
duve_close(duve_cb, abort_flag)
DUVE_CB		*duve_cb;
bool		abort_flag;
{

    EXEC SQL WHENEVER SQLERROR continue;

    if (duve_cb->du_opendb != "")
    {
	if ( duve_cb->duverel != NULL)
	{
	    MEfree ( (PTR) duve_cb->duverel );
	    duve_cb->duverel = NULL;
	}

        if (duve_cb->duvedpnds.duve_indep != NULL)
        {
            MEfree((PTR) duve_cb->duvedpnds.duve_indep);
            duve_cb->duvedpnds.duve_indep = NULL;
            duve_cb->duvedpnds.duve_indep_cnt = 0;
        }

        if (duve_cb->duvedpnds.duve_dep != NULL)
        {
	    MEfree((PTR) duve_cb->duvedpnds.duve_dep);
	    duve_cb->duvedpnds.duve_dep = NULL;
	    duve_cb->duvedpnds.duve_dep_cnt = 0;
	}

	if (duve_cb->duvefixit.duve_schemas_to_add)
	{
	    DUVE_ADD_SCHEMA	*p, *nxt;

	    for (p = duve_cb->duvefixit.duve_schemas_to_add; p; p = nxt)
	    {
		nxt = p->duve_next;
		MEfree((PTR) p);
	    }

	    duve_cb->duvefixit.duve_schemas_to_add = NULL;
	}

	if (duve_cb->duvefixit.duve_objs_to_drop)
	{
	    DUVE_DROP_OBJ	*p, *nxt;

	    for (p = duve_cb->duvefixit.duve_objs_to_drop; p; p = nxt)
	    {
		nxt = p->duve_next;
		MEfree((PTR) p);
	    }

	    duve_cb->duvefixit.duve_objs_to_drop = NULL;
	}

	if ( duve_cb->duvestat != NULL)
	{
	    MEfree ( (PTR) duve_cb->duvestat);
	    duve_cb->duvestat = NULL;
	}
	if ( duve_cb->duvetree != NULL)
	{
	    MEfree ( (PTR) duve_cb->duvetree);
	    duve_cb->duvetree = NULL;
	}
	if ( duve_cb->duveeves != NULL)
	{
	    MEfree ( (PTR) duve_cb->duveeves);
	    duve_cb->duveeves = NULL;
	}
	if ( duve_cb->duveprocs != NULL)
	{
	    MEfree ( (PTR) duve_cb->duveprocs);
	    duve_cb->duveprocs = NULL;
	}
	if ( duve_cb->duvesyns != NULL)
	{
	    MEfree ( (PTR) duve_cb->duvesyns);
	    duve_cb->duvesyns = NULL;
	}
	if ( duve_cb->duveint != NULL)
	{
	    MEfree ( (PTR) duve_cb->duveint);
	    duve_cb->duveint = NULL;
	}
	if (duve_cb->duveusr != NULL)
	{
	    MEfree ( (PTR) duve_cb->duveusr );
	    duve_cb->duveusr = NULL;
	}
	if (duve_cb->duvedbinfo != NULL)
	{
	    MEfree ( (PTR) duve_cb->duvedbinfo);
	    duve_cb->duvedbinfo = NULL;
	}

	IIlq_Protect(FALSE);
	if (abort_flag)
	{
	    EXEC SQL ROLLBACK;
	}
	else
	{
	    EXEC SQL COMMIT;
	}
	EXEC SQL DISCONNECT;
	duve_cb->du_opendb[0] = '\0';
    }
}

/*{
** Name: duve_force	- force an inconsistent database to appear consistent
**
** Description:
**      This routine connects to the specified user database using the 
**	application code of DBA_FORCE_VFDB, which will cause the DBMS
**	to make an inconsistent database appear consistent.  It will not
**	have any effect on a consistent database.  As soon as the conncention
**	is accomplished, verifydb disconnects.  This leave the database
**	appearing consistent.  Then verifydb connects to the iidbdb (the
**	database database), and marks that it has forced the specified database
**	to appear consistent.
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_operation		    indicates the operation to perform
**					      (FORCE_CONSISTENT, etc) -- USED BY
**					      DUVE_OPENDB
**	    duve_debug			    flag indicating special informative
**					      messages should be output --
**					      USED BY DUVE_OPENDB
**	    duveowner			    owner of database
**	    duve_user			    owner of database if specified by
**					      -u flag to be different that 
**					      user who evoked verifydb
**      database                        name of database to force consistent
**
** Outputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_msg			    contains error info from INGRES if
**					    ingres error encountered
**	Returns:
**	    DUVE_YES -- operation successful
**	    DUVE_BAD -- operation failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-88 (teg)
**          initial creation
[@history_template@]...
*/
DU_STATUS
duve_force( duve_cb, database)
DUVE_CB		*duve_cb;
char		*database;
{
    char	*ownname;
    EXEC SQL BEGIN DECLARE SECTION;
	i4	access;
	char	dbname[DB_MAXNAME];
	char	dbown[DB_MAXNAME];
    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if ( duve_opendb(duve_cb, database) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
    duve_close(duve_cb,TRUE);

    duve_cb->duve_operation = DUVE_FORCED;
    if ( duve_opendb(duve_cb, DU_DATABASE_DATABASE) == DUVE_BAD)
	return ((DU_STATUS) DUVE_BAD);
    STcopy (database,dbname);
    if (duve_cb->duve_user[0] == '\0')
	STcopy ( duve_cb->duveowner, dbown);
    else
    {   
        ownname = duve_cb->duve_user;
	STcopy ( ownname, dbown);
    }
    EXEC SQL select dbservice into :access from iidatabase where 
		name = :dbname and own = :dbown;
    access |= DU_FORCED_CONST;
    EXEC SQL update iidatabase set dbservice = :access where
		name = :dbname and own = :dbown;
    duve_close (duve_cb,FALSE);
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: duve_hcheck	- check for healthy connect to specified DB as
**			  specified user.
**
** Description:
**	This routine opens the specified DB as the specifie user.  If it
**	encounters an error, then ESQL will cause routine Clean_up() to be
**	called to report the error.  RETSTAT will be set to DUVE_BAD if an 
**	error is encountered, or DUVE_YES if one is NOT.  Then duve_hcheck
**	will close the db.
**	
** Inputs:
**	user				name of user to connect as
**      database                        name of database to open
**
** Outputs:
**	    NONE
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	28-mar-1990 (teg)
**	    Created on request of Eurotech.
[@history_template@]...
*/
DU_STATUS
duve_hcheck(duve_cb, database, user)
DUVE_CB		*duve_cb;
char		*database;
char		*user;
{

    DU_STATUS	    retstat;

    EXEC SQL BEGIN DECLARE SECTION;
	char	username[DB_MAXNAME+1];
	char	dbname[DB_MAXNAME+1];
    EXEC SQL END DECLARE SECTION;


    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    STcopy(database, dbname);
    STcopy(user,username);

    exec sql connect :dbname identified by :username;
    if (sqlca.sqlcode < DU_SQL_OK)
	retstat = (DU_STATUS) DUVE_BAD;
    else
    {
	retstat = (DU_STATUS) DUVE_YES;
	EXEC SQL DISCONNECT;
    }

    return (retstat);
}

/*{
** Name: duve_opendb	- open database
**
** Description:
**      The ESQL connect statement is used to open a database for an ESQL
**      front end program.  The connect statement permits access to a specified
**      database.  It is also possible to specify a user code and to pass 
**	connection flags to SCF via this open statement.  Verifydb uses
**	its control block (DUVE_CB) to determine how to connect to the database,
**	as follows:
**	    OPERATION = FORCE_CONSISTENT
**		verifydb will connect as user $INGRES
**		verifydb will lock the database (-l flag)
**		verifydb will use application code of DBA_FORCE_VFDB to
**		    indicate that it is requsting the database be forced
**		    consistent
**	    OPERATION = DUVE_FORCED (set to this operation
**				by duve_force -- will access iidbdb only)
**		verifydb will connect as user $INGRES
**		verifydb will use application code of DBA_NORMAL_VFDB to
**		    indicate it does not require special acccess to inconsistent
**		    databases.
**	    OPERATION = DBMS_CATALOGS
**		verifydb will connect as user $INGRES
**		verifydb will lock the databasde (-l flag)
**		verifydb will use application code of DBA_PRETEND_VFDB to
**		    indicate that access should be permitted to the database
**		    even if it has been marked inconsistent
**	    OPERATION = TABLE
**		verifydb will connect as user who evokes it or as user indicated
**		    by -u flag
**		verifydb will not lock the database
**		verifydb will use application code of DBA_PURGE_VFDB to
**		    indicate it does require the ability to drop expired
**		    tables that the DBA does not own.
**	    OPERATION = EXPIRED_PURGE
**		verifydb will connect as user who evokes it or as user indicated
**		    by -u flag
**		verifydb will lock NOT the databasde
**		verifydb will use application code of DBA_PURGE_VFDB to
**		    indicate it does not require special acccess to inconsistent
**		    databases.
**	    OPERATION = any of the PURGES except (EXPIRED_PURGE)
**		verifydb will connect as user who evokes it or as user indicated
**		    by -u flag
**		verifydb will lock the databasde (-l flag)
**		verifydb will use application code of DBA_PURGE_VFDB to
**		    indicate it does not require special acccess to inconsistent
**		    databases.
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_operation		    indicates the operation to perform
**					    (FORCE_CONSISTENT, DBMS_CATALOGS,etc)
**	    duve_debug			    flag indicating if special informative
**					      messages should be output for debug
**	    duve_user			    user name specified with -u parameteer
**      database                        name of database to open
**
** Outputs:
**      duve_cb                         verifydb control block, contains:
**	    du_opendb			     name of opened database, null if
**					      database not opened
**          duve_msg                         Error code if DUVE_BAD RETURNED:
**		E_DU501A_CANT_CONNECT
**		E_DU501B_CANT_CONNECT_AS_USER
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    system catalog update permission is set true for some operations
**	    ERROR message is generated if DBMS returns error during connect
**
** History:
**      15-Aug-88 (teg)
**          initial creation
**	24-Aug-88 (teg)
**	    added logic for new operation of FORCED
**	2-Nov-88  (teg)
**	    added logic for drop_tbl operation
**	19-Jan-89 (teg)
**	    changed DUVE_FORCED logic to stop taking a lock on the database.
**	03-Mar-89 (teg)
**	    changed connect statements to all have system catalog update priv.
**	03-Aug-89 (teg)
**	    default (purges) connect was incorrect when -u flag was specified on
**	    verifydb command line.  Fixed by always connecting as $INGRES when
**	    opening for any of the purges.
**	14-Sep-89 (teg)
**	    Modified logic for DUVE_TABLE and add logic for DUVE_XTABLE
**	21-dec-90 (teresa)
**	    Add "set autocommit off" statement just incase someone has turned
**	    autocommit on via ing_set.
**	07-dec-93 (andre)
**	    duvedpnds is no longer a pointer - it is a structure describing 
**	    cached IIDBDEPENDS information - its initialization will be a bit 
**	    more complicated than just setting duvedpnds to NULL
**	04-jan-93 (teresa)
**	    Fix bug 42943 - no need to lock db on expired purge.
**	18-jan-94 (andre)
**	    added code to initialize "fix it" cache
**	1-mar-94 (robf)
**          added label_purge.
**      8-Sep-2005 (hanal04) Bug 107308
**          A verifydb against iidbdb may fail when connecting to the named
**          database because the GCA_RELEASE issued for the original connection
**          to iidbdb may not have been processed by the DBMS server.
**          As the GCA_SEND that sends the GCA_RELEASE is asynchronous
**          we need to take a nap to ensure we do not block ourself
**          when connecting to iidbdb for the second time.
[@history_template@]...
*/
DU_STATUS
duve_opendb(duve_cb, database)
DUVE_CB		*duve_cb;
char		*database;
{

	char	tmp_buf[15];

    EXEC SQL BEGIN DECLARE SECTION;
	char	username[DB_MAXNAME+4];
	char	dbname[DB_MAXNAME+4];
	char	duve_appl_flag[20];
    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR goto SQLerr;

    STcopy(database, dbname);

    if(STbcompare(dbname, STlength(dbname), DU_DBDBNAME,
                  STlength(DU_DBDBNAME), TRUE) == 0)
    {
        PCsleep(500);
    }

    switch (duve_cb->duve_operation)
    {  
      case DUVE_MKCONSIST:
	CVla((i4)DBA_FORCE_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
	     options = :duve_appl_flag, '-l';
	IIlq_Protect(TRUE);
	break;
      case DUVE_FORCED:
      	CVla((i4)DBA_NORMAL_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
	     options = :duve_appl_flag;
	IIlq_Protect(TRUE);
	break;
      case DUVE_DROPTBL:
	CVla((i4)DBA_NORMAL_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
	     options = :duve_appl_flag, '-l';
	IIlq_Protect(TRUE);
	break;
      case DUVE_CATALOGS:
	CVla((i4)DBA_PRETEND_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
	     options = :duve_appl_flag, '-l';
	IIlq_Protect(TRUE);
	break;
      case DUVE_TABLE:
      case DUVE_XTABLE:
	CVla((i4)DBA_PURGE_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
		options = :duve_appl_flag;
	IIlq_Protect(TRUE);
	break;
      case DUVE_EXPIRED:  /* expired purge */
	CVla((i4)DBA_PURGE_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
		options = :duve_appl_flag;
	IIlq_Protect(TRUE);
	break;
      case DUVE_TEMP:  /* temp purge or full purge */
      case DUVE_PURGE:
      case DUVE_LABEL_PURGE:
	CVla((i4)DBA_PURGE_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
		options = :duve_appl_flag, '-l';
	IIlq_Protect(TRUE);
	break;
      default:
	/* no other operations are expected, but if we get any,
	** open as user ingres with database lock, then give
	** self system catalog update priv.
	*/
      	CVla((i4)DBA_NORMAL_VFDB, tmp_buf);
	(VOID)STpolycat(2, "-A",tmp_buf, duve_appl_flag);
	exec sql connect :dbname identified by '$ingres'
		options = :duve_appl_flag, '-l';
	IIlq_Protect(TRUE);
	break;
    } /* endcase */

    duve_cb->duverel = NULL;

    duve_cb->duvedpnds.duve_indep_cnt = duve_cb->duvedpnds.duve_dep_cnt = 0;
    duve_cb->duvedpnds.duve_indep = (DU_I_DPNDS *) NULL;
    duve_cb->duvedpnds.duve_dep = (DU_D_DPNDS *) NULL;

    duve_cb->duvefixit.duve_schemas_to_add = NULL;
    duve_cb->duvefixit.duve_objs_to_drop = NULL;

    STcopy(database, duve_cb->du_opendb);
    EXEC SQL set session isolation level serializable;
    EXEC SQL set autocommit off;
    return ( (DU_STATUS) DUVE_YES);

SQLerr:
	/* if we get here, we couldn't open the database.  So, see if
	** we were attempting as a user or as self.  Then generate the
	** correct error message and abort verifydb
	*/
    if (duve_cb->duve_user[0]=='\0')
    	duve_talk(DUVE_ALWAYS, duve_cb, E_DU501A_CANT_CONNECT,2, 0, database);
    else
	duve_talk(DUVE_ALWAYS, duve_cb, E_DU501B_CANT_CONNECT_AS_USER,
		  4, 0, database, 0, duve_cb->duve_user);
    return ( (DU_STATUS) DUVE_BAD);

}

/*{
** Name: duve_tbldrp	- drop a table from the catalogs
**
** Description:
**      This routine obtains the table id from iirelation (knowning the
**	name)  It then drops tuples from each dbms catalog that have that
**	table id.
**
**	If the table is an independent table in iidbdepends, it will print a
**	warning that verifydb's system catalog check should be run.  It will
**	NOT automatically clean up views, permits, integrities, etc.  YOU MUST
**	RUN VERIFYDB again to do that:
**		
**		verifydb -mruni -sdbn xxxx -odbms
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_operation		    indicates the operation to perform
**					      (FORCE_CONSISTENT, etc) -- USED BY
**					      DUVE_OPENDB
**	    duveowner			    owner of database
**	    duve_user			    owner of database if specified by
**					      -u flag to be different that 
**					      user who evoked verifydb
**      database                        name of database containing table to
**					delete
**	table				name of table to delete
**
** Outputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_msg			    contains error info from INGRES if
**					    ingres error encountered
**	Returns:
**	    DUVE_YES -- operation successful
**	    DUVE_BAD -- operation failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-nov-88 (teg)
**          initial creation
**	6-aug-93 (teresa)
**	    fix bug 53152
**	07-jan-93 (teresa)
**	    drop query text for the table we are dropping.
**	26-jan-94 (andre)
**	    instead of using obsolete dropdbdepends() we will use 
**	    duve_d_cascade()
[@history_template@]...
*/
DU_STATUS
duve_tbldrp( duve_cb, database, droptable)
DUVE_CB		*duve_cb;
char		*database;
char		*droptable;
{
    char	*ownname;
    EXEC SQL BEGIN DECLARE SECTION;
	char	own[DB_MAXNAME];
	char	tblname[DB_MAXNAME];
	u_i4	tid,tidx;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    /* 1st open database and get table id for this table */
    if ( duve_opendb(duve_cb, database) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);
	
    STcopy ( droptable, tblname);

    if (duve_cb->duve_user[0] == '\0')
	STcopy ( duve_cb->duveowner, own);
    else
    {   
        ownname = duve_cb->duve_user;
	STcopy ( ownname, own);
    }

    exec sql select reltid,reltidx  into :tid, :tidx from iirelation where
	relid = :tblname and relowner = :own;
    if (sqlca.sqlcode == DU_SQL_NONE)
    {	duve_talk(DUVE_MODESENS, duve_cb, E_DU5025_NO_SUCH_TABLE, 4,
		  0, droptable, 0, own);
	return ( (DU_STATUS) DUVE_NO);
    }


    if (duve_talk (DUVE_ASK, duve_cb, S_DU032D_DROP_TABLE, 6, 0, tblname,
		    0, own, 0, database) == DUVE_YES)
    {

	/* now go clean up iidbdepends table.  If this is successful, complete
	** transaction.
	*/
	EXEC SQL delete from iidbdepends where deid1 = :tid and	deid2 = :tidx;

	duve_d_cascade(duve_cb, tid, tidx);

	duve_talk (DUVE_MODESENS, duve_cb, S_DU037D_DROP_TABLE, 6, 0, tblname,
	    0, own, 0, database);
	duve_close (duve_cb,FALSE);
	return ( (DU_STATUS) DUVE_YES);
    }

    duve_close (duve_cb,TRUE);
    return ( (DU_STATUS) DUVE_NO);
}

/*{
** Name: ckatt	- perform checks/fixes on iiattributes table
**
** Description:
**      ckatt gets a tuple from iiattribute.  It checks the table id to see
**	if it has already processed any other tuples for this table id.  If so,
**	and if tuples for this table are marked for deletion, it deletes this
**	tuple.  Otherwise, it runs verifydb tests on this tuple.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	MESSAGES TO USER and/or LOG FILE:
**	    S_DU1626_INVALID_ATTRELID,
**	    S_DU0317_DROP_ATTRIBUTES,
**	    S_DU0367_DROP_ATTRIBUTES,
**	    S_DU1627_INVALID_ATTFMT,
**	    S_DU0302_DROP_TABLE,
**	    S_DU0352_DROP_TABLE,
**	    S_DU1694_INVALID_DEFAULT
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**	    calls have ().
**	10-sep-92 (bonobo)
**	    Changed DB_LVC_TYPE to DB_LVCH_TYPE to fix compiler error.
**	08-apr-93 (teresa)
**	    remove test of attfrmt because it currently breaks UDTS.  May
**	    reinstate in 6.6 when UDTs are visible to FEs.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	16-jun-93 (anitap)
**	    Added test 3 for FIPS constraints.
**	15-jul-93 (teresa)
**	    moved declaration of cnt as bool (for test 3) out of SQL declare
**	    section because it redelcares earlier instances of cnt as a i4
**	    and breaks them.  This instance is not used by ESQL and should not
**	    be in that section.  This is a subtle thing, but all ESQLC declare
**	    variables are GLOBAL, even when they're declared from within a
**	    subroutine.
**	10-dec-93 (teresa)
**	    changed cnt to a more meaningful name (special_default).  Also
**	    rewrote query to iidefault to stop joining with iiattribute.  This
**	    is part of a project to speed up verifydb performance.  Also report
**	    performance statistics if debug flag is specified.
**	20-dec-93 (anitap)
**	    Added REPEATED keyword and ANY to test 3 to improve performance.
**	    Included cases DB_DEF_ID_NEEDS_CONVERTING and
**	    DB_DEF_ID_UNRECOGNIZED. Corrected number of parameters being
**	    passed to the error message in test 3.
[@history_template@]...
*/
static DU_STATUS
ckatt(duve_cb)
DUVE_CB		*duve_cb;
{
    i2		fmt;
    i4	base;
    DB_TAB_ID	reltid;
    DU_STATUS	permission = DUVE_NO;
    bool	marked_for_drop = FALSE;
    EXEC SQL BEGIN DECLARE SECTION;
	EXEC SQL INCLUDE <duveatt.sh>;
	i4 agg;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR continue;

    EXEC SQL DECLARE att_cursor CURSOR FOR 
	select * from iiattribute order by attrelid, attrelidx;
    EXEC SQL OPEN att_cursor;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    /* initialize for 1st pass thru loop */
    reltid.db_tab_base = DU_INVALID;
    reltid.db_tab_index = DU_INVALID;

    /* loop for each tuple in iiattribute */
    for ( ;; )
    {
	/*
	** get entry from iiattribute
	*/
	EXEC SQL FETCH att_cursor INTO :att_tbl;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE att_cursor;
	    break;
        }
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

        /* here we have a good tuple.  See if it has a table id that
	** we have already processed in a previous pass throught this loop
	*/
	if ( ! ((reltid.db_tab_base == att_tbl.attrelid) && 
	        (reltid.db_tab_index == att_tbl.attrelidx) ) )
	{
	    /* if we get here, then this the the start of attribute entries
	    ** for a table id we haven't checked for in iirelation yet. */
	    permission = DUVE_NO;
	    marked_for_drop = FALSE;
	    reltid.db_tab_base = att_tbl.attrelid;
	    reltid.db_tab_index = att_tbl.attrelidx;
	
	    base = duve_findreltid ( &reltid, duve_cb);
	    if ((base == DU_INVALID) || (base == DUVE_DROP))
	    {   if (base == DUVE_DROP)
		    continue;
	        else
	        {
	 	    /* this iiattribute tuple doesn't have a corresponding tuple
		    ** in iirelation. */
	            if (duve_banner( DUVE_IIATTRIBUTE, 1, duve_cb)
	            == DUVE_BAD)
		        return ( (DU_STATUS) DUVE_BAD);
	             duve_talk( DUVE_MODESENS, duve_cb, S_DU1626_INVALID_ATTRELID, 4,
			sizeof(att_tbl.attrelid), &att_tbl.attrelid,
			sizeof(att_tbl.attrelidx), &att_tbl.attrelidx);
	             /* see if we should delete this tuple */
	             permission = duve_talk( DUVE_IO, duve_cb, 
			   S_DU0317_DROP_ATTRIBUTES, 4,
			   sizeof(att_tbl.attrelid), &att_tbl.attrelid,
			   sizeof(att_tbl.attrelidx), &att_tbl.attrelidx);
		} /* endif no corresponding tuple in iirelation */
	    }  /* endif DROP or INVALID */
	}   /* endif 1st time we see this table id */

        if ( permission == DUVE_YES)
	{
		/* delete this tuple */
		exec sql delete from iiattribute
			where attrelid = :att_tbl.attrelid
			 and attrelidx = :att_tbl.attrelidx;
			 
		if (sqlca.sqlcode == DU_SQL_OK)
		    duve_talk( DUVE_MODESENS, duve_cb, S_DU0367_DROP_ATTRIBUTES, 4,
			sizeof(att_tbl.attrelid), &att_tbl.attrelid,
			sizeof(att_tbl.attrelidx), &att_tbl.attrelidx);
	}
	else if (marked_for_drop)
	    continue;
	else
	{
#if 0
	/* remove this test for now because it breaks on UDTs */
	    /* see if tuple has a valid format */
	    fmt = (att_tbl.attfrmt > 0) ? att_tbl.attfrmt 
				    : -att_tbl.attfrmt ;
	    if ( (fmt == DB_INT_TYPE) ||  (fmt == DB_FLT_TYPE) ||
	         (fmt == DB_CHR_TYPE) ||  (fmt == DB_TXT_TYPE) ||
	         (fmt == DB_DTE_TYPE) ||  (fmt == DB_MNY_TYPE) ||
	         (fmt == DB_DEC_TYPE) ||  (fmt == DB_CHA_TYPE) ||
	         (fmt == DB_VCH_TYPE) ||  (fmt == DB_LVCH_TYPE) ||
	         (fmt == DB_LOGKEY_TYPE) ||  (fmt == DB_TABKEY_TYPE) ||
	         (fmt == DB_SEC_TYPE) )
	     {
	         continue;
	     }
	     /* if we get here, tuple has invalid format */
	    if (duve_banner( DUVE_IIATTRIBUTE, 2, duve_cb)
	     == DUVE_BAD)
	     return ( (DU_STATUS) DUVE_BAD);
	     duve_talk( DUVE_MODESENS, duve_cb, S_DU1627_INVALID_ATTFMT, 8,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own,
			sizeof(att_tbl.attid), &att_tbl.attid,
			sizeof(fmt), &fmt);
	     /* see if we should delete this table */

	     if (duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own)
	     == DUVE_YES)
             {
		 /* delete this table */
		 idxchk (duve_cb, att_tbl.attrelid, att_tbl.attrelidx);
	         duve_cb -> duverel -> rel[base].du_tbldrop = DUVE_DROP;
	         duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);
		 marked_for_drop = TRUE;
             }
#endif
	}

	/* test 3 - check the default id. */

	/* check if canonical default id */
	switch(att_tbl.attdefid1)
        {
              case DB_DEF_NOT_DEFAULT:
              case DB_DEF_NOT_SPECIFIED:
              case DB_DEF_UNKNOWN:
              case DB_DEF_ID_0:
              case DB_DEF_ID_BLANK:
              case DB_DEF_ID_TABKEY:
              case DB_DEF_ID_LOGKEY:
              case DB_DEF_ID_NULL:
              case DB_DEF_ID_USERNAME:
              case DB_DEF_ID_CURRENT_DATE:
              case DB_DEF_ID_CURRENT_TIMESTAMP:
              case DB_DEF_ID_CURRENT_TIME:
              case DB_DEF_ID_SESSION_USER:
              case DB_DEF_ID_SYSTEM_USER:
              case DB_DEF_ID_INITIAL_USER:
              case DB_DEF_ID_DBA:
              case DB_DEF_ID_FALSE:
              case DB_DEF_ID_TRUE:
	      case DB_DEF_ID_NEEDS_CONVERTING:
	      case DB_DEF_ID_UNRECOGNIZED:		
                   /*
                   ** this is one of "special" defaults - no checking is
                   ** necessary
                   */
                   break;
              default:

	   	exec sql repeated select any(defid1) into :agg from iidefault
			where defid1 = :att_tbl.attdefid1 and
		     	defid2 = :att_tbl.attdefid2;
 
	   	if (agg == 0)
	   	{		/* default id not present in iidefault */
	      	   if (duve_banner(DUVE_IIATTRIBUTE, 3, duve_cb) == DUVE_BAD)
		      return ((DU_STATUS) DUVE_BAD);
	      	   duve_talk(DUVE_MODESENS, duve_cb,
			S_DU1694_INVALID_DEFAULT, 2,
			0, att_tbl.attname);
	   	} /* endif agg = 0 */
		break;
	}

    } /* end loop for each tuple in iiattribute */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckatt");	
    
    return( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckdevices  - perform system catalog check/fix for iidevices
**
** Description:
**      perform verifydb tests on iidevices from verifydb product specificaiton:
**
** 1.	verify that each unique table referenced in iidevices is also in
**	iirelation.
**TEST:	for each unique (devrelid, devrelidx) pair in iidevices, verify that
**	there is a row in iirelation where:  iirelation.reltid = 
**	iidevices.devrelid and iirelation.reltidx = iidevices.devrelidx.
**FIX:	delete all rows from iidevices with this (devrelid, devrelidx) pair.
**MSGS:	S_DU162B, S_DU031A, S_DU036A
**
** 2.   Verify that the location name is valid.
**TEST: Find the iirelation of the table that this tuple is for.  If that
**	does not indicate a gateway (iirelation.relstat.gateway = false), then
**	verify that the location name is known to the installation.
**FIX:  report error.  DBA should fix this offline.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	Messages to the User:
**	    S_DU162B_INVALID_DEVRELIDX,
**	    S_DU031A_DROP_DEVRELID,
**	    S_DU036A_DROP_DEVRELID
**	Returns:
**	    DUVE_YES,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be deleted from iidevices
**	    values in iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**	    calls have ().
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb - performance statistics
**	    reporting.
**	20-dec-93 (anitap)
**	    Support for delimited identifiers. Don't use STzapblank(). Use 
**	    STtrmwhite() instead which gets rid of the trailing blanks and not
**	    the embedded blanks.
**      27-Jul-1998 (carsu07)
**          When verifying the location name, check if the name is blank. If so 
**          then this is a dummy row in iidevices, caused by modifying a table 
**          over multiple locations followed by a modify over a reduced number
**          of locations. Cross integration from oping12. (Bug 90146)
**	23-Feb-2009 (kschendel) b122041
**	    Fix stupid blow-up if location name is DB_MAXNAME long.
[@history_template@]...
*/

static DU_STATUS
ckdevices(duve_cb)
DUVE_CB		*duve_cb;
{
    char dev_loc[DB_MAXNAME+1];
    DB_TAB_ID	reltid;
    i4	base, len;
    EXEC SQL BEGIN DECLARE SECTION;
	EXEC SQL INCLUDE <duvedev.sh>;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR continue;

    EXEC SQL DECLARE dev_cursor CURSOR FOR 
	select * from iidevices;
    EXEC SQL OPEN dev_cursor;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    for ( ;; )
    {

	/*
	** get entry from iidevices
	*/

	EXEC SQL FETCH dev_cursor INTO :dev_tbl;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE dev_cursor;
	    break;
        }
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

	reltid.db_tab_base = dev_tbl.devrelid;
	reltid.db_tab_index = dev_tbl.devrelidx;
	
	/* test 1 -- verify entry exists in iirelation for this iidevices tuple
	*/
	base = duve_findreltid ( &reltid, duve_cb);
	if( base == DU_INVALID)
	{    
	     /* this iidevices tuple doesn't have a corresponding tuple
	     ** in iirelation. */

	    if (duve_banner( DUVE_IIDEVICES, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	     duve_talk( DUVE_MODESENS, duve_cb, S_DU162B_INVALID_DEVRELIDX, 4,
		        sizeof(dev_tbl.devrelid), &dev_tbl.devrelid,
		        sizeof(dev_tbl.devrelidx), &dev_tbl.devrelidx);
	     /* see if we should delete this tuple */
	     if ( duve_talk( DUVE_ASK, duve_cb, S_DU031A_DROP_DEVRELID, 4,
		        sizeof(dev_tbl.devrelid), &dev_tbl.devrelid,
		        sizeof(dev_tbl.devrelidx), &dev_tbl.devrelidx)
	     == DUVE_YES)
	     {
		/* delete this tuple */
		exec sql delete from iidevices where current of dev_cursor;
		if (sqlca.sqlcode == DU_SQL_OK)
		    duve_talk( DUVE_MODESENS, duve_cb, S_DU036A_DROP_DEVRELID, 4,
		        sizeof(dev_tbl.devrelid), &dev_tbl.devrelid,
		        sizeof(dev_tbl.devrelidx), &dev_tbl.devrelidx);
	     }
	     continue;
	} /* endif tuple not in iirelation */


	/* dont bother checking location name if this tuple is going to be
	** dropped anyhow 
	*/
	if (base == DUVE_DROP)
		continue;

	/* test 2 -- verify location name known to this installation
	*/

	if (duve_cb->duverel->rel[base].du_stat & TCB_GATEWAY)
		continue;
        /*
        ** Skip rows with no devloc value.  These are considered
        ** dummy rows and are the result of spreading a table over
        ** multiple locations and then reducing the number of locs
        ** on which the table is defined.
        */
        if (dev_tbl.devloc[0] == ' ')
	        continue;

	len = STtrmwhite(dev_tbl.devloc);
	STcopy(dev_tbl.devloc, &dev_loc[0]);

	if ( duve_locfind( dev_loc, duve_cb) == DU_INVALID)
	{
	     /* this iidevices tuple doesn't have a valid location name.
	     */
	    if (duve_banner( DUVE_IIDEVICES, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	     duve_talk( DUVE_MODESENS, duve_cb, S_DU1620_INVALID_LOC, 6,
		  0, dev_loc, 0, duve_cb->duverel->rel[base].du_tab,
		  0, duve_cb->duverel->rel[base].du_own);
	}

    } /* end loop for each tuple in iidevices */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckdevices");	

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckindex	- perform check/repair on system catalog iiindex
**
** Description:
**      perform tests on iiindex from verifydb product specification. 
**
**
**  1.	    verify that is a tuple in iirelation for the index table
**  TEST:   get a row from iirelation where iirelation.reltid = iiindex.baseid
**	    and iirelation.reltidx = iiindex.indexid.
**  FIX:    drop the index relation from iiindex
**  MSGS:   S_DU1628, S_DU0318, S_DU0368
**
**  2.	    Verify that that index table entry in iirelation has the restat
**	    index table bit flag set.
**  TEST:   Verify that the iirelation tuple (retrieved in test 1) has the
**	    iirelation.relstat.INDEX bit set to true.
**  FIX:    Set the iirelation.relstat.INDEX bit in that iirelation tuple.
**  MSGS:   S_DU1629, S_DU0319, S_DU0369
**
**  3.	    Verify that the elements in the idom array are not unreasonable.
**  TEST:   verify that the content of idom1 through idom32 are not greater than
**	    the relatts from the iirelation tuple retrieved in test 1.
**	    (ie, iiindex.idom[x] =< iirelation.relatts)
**  FIX:    Print warning that this inconsistency occurred.
**		IF MODE = RUNINTERACTIVE THEN
**		    prompt user for permission to drop this table from catalogs
**		ELSE
**		    print request for user to fix manually or run verifydb in
**		    interactive mode
**		ENDIF
**
**  4.	    Verify that the index tuple has corresponding iidbdepends tuuple.
**  TEST:   verify that for each baseid, indexid pair in iiindex, there is a
**	    corresponding tuple in iidbdepends.	
**	    (i.e., iiindex.baseid = iidbdepends.deid1
**	    and iiindex.indexid = iidbdepends.deid2)
**  FIX:    Print warning that this inconsistency occurred.
**              IF MODE = UNINTERACTIVE THEN
**                  prompt user for permission to drop this table from catalogs
**              ELSE
**                  print request for user to fix manually or run verifydb in
**                  interactive mode
**              ENDIF
**  MSGS:   S_DU162A, S_DU0302, S_DU0352
**
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    duverel			    memory cache of iirelation tuple info
**		du_stat				memory cache of relstat
**		du_stat2			memory cache of relstat2
**		du_tab				name of table
**		du_own				name of owner
**		du_attno			number of columns in table
**
** Outputs:
**      duve_cb                         verifydb control block, contains:
**	    duverel			    memory cache of iirelation tuple info
**		du_stat				memory cache of relstat
**		du_tbldrop			flag indicating to drop table
**	Messages to User:
**	    S_DU1628_INVALID_BASEID,
**	    S_DU0318_DROP_INDEX,
**	    S_DU0368_DROP_INDEX,
**	    S_DU1629_IS_INDEX,
**	    S_DU0319_SET_INDEX,
**	    S_DU0369_SET_INDEX,
**	    S_DU161A_NO_BASE_FOR_INDEX,
**	    S_DU0302_DROP_TABLE,
**	    S_DU0352_DROP_TABLE
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    info read from iiindex
**	    tuple may be dropped from iiindex
**	    iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**	    initial creation
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**	    calls have ().
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	18-oct-93 (anitap)
**	    Added test #4 to check if the index tuple has corresponding
**	    iidbdepends tuple. 
**	03-dec-93 (andre)
**	    duverel now contains a copy of relstat2 (inside du_stat2).  
**	    Therefore, rather than checking IIDBDEPENDS for every index, we 
**	    will do it only for indices which were created to enforce a 
**	    constraint.  This should save us at least a few queries against 
**	    iidbdepends and iirelation.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb - performance statistics
**	    reporting.
**	13-jan-94 (anitap)
**	    Corrected test number in duve_banner() to 4.
**	02-feb-94 (anitap)
**	    use S_DU0318_DROP_INDEX & S_DU0368_DROP_INDEX instead of 
**	    S_DU0302_DROP_TABLE and S_DU0352_DROP_TABLE.
[@history_template@]...
*/
static DU_STATUS
ckindex(duve_cb)
DUVE_CB		*duve_cb;
{
#define		NUM_IDOMS	32	/* 32 entries in iiindex idom array */
    DB_TAB_ID	reltid;
    i4	base,index;
    EXEC SQL BEGIN DECLARE SECTION;
	EXEC SQL INCLUDE <duveindex.sh>;
	i4	relstat;
	u_i4	tid,tidx;
	i4 cnt;
	i4 stat;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR continue;

    EXEC SQL DECLARE index_cursor CURSOR FOR 
	select * from iiindex;
    EXEC SQL OPEN index_cursor;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    for ( ;; )
    {

	/*
	** get entry from iiindex
	*/

	EXEC SQL FETCH index_cursor INTO :index_tbl;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE index_cursor;
	    break;
        }
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

	reltid.db_tab_base = index_tbl.baseid;
	reltid.db_tab_index = index_tbl.indexid;
	
	/* test 1 -- entry in iirelation for this index table */
	index = duve_findreltid ( &reltid, duve_cb);
	if (index == DU_INVALID)
	{    
	     /* this iiindex tuple doesn't have a corresponding tuple
	     ** in iirelation. */

	    if (duve_banner( DUVE_IIINDEX, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	     duve_talk( DUVE_MODESENS, duve_cb, S_DU1628_INVALID_BASEID, 4,
			sizeof(index_tbl.baseid), &index_tbl.baseid,
			sizeof(index_tbl.indexid), &index_tbl.indexid);
	     /* see if we should delete this tuple */
	     if ( duve_talk( DUVE_ASK, duve_cb, S_DU0318_DROP_INDEX, 4,
			sizeof(index_tbl.baseid), &index_tbl.baseid,
			sizeof(index_tbl.indexid), &index_tbl.indexid)
	     == DUVE_YES)
	     {
		/* delete this tuple */
		exec sql delete from iiindex where current of index_cursor;
		if (sqlca.sqlcode == DU_SQL_OK)
		    duve_talk( DUVE_MODESENS, duve_cb, S_DU0368_DROP_INDEX, 4,
			sizeof(index_tbl.baseid), &index_tbl.baseid,
			sizeof(index_tbl.indexid), &index_tbl.indexid);
	     }
	} /* endif tuple not in iirelation */
	else if (index == DUVE_DROP)
	    continue;
	else /* test 2 -- verify iirelation for index table indicates that
	     ** it is secondary index 
	     */
	{   if ( (duve_cb->duverel->rel[index].du_stat & TCB_INDEX) == 0)
	    {	/* opps!.  The iirelation entry for index table doesn't
	        ** indicate that its an index table
		*/
                if (duve_banner( DUVE_IIINDEX, 2, duve_cb)
	        == DUVE_BAD) 
	  	    return ((DU_STATUS) DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU1629_IS_INDEX, 4,
			  0,duve_cb->duverel->rel[index].du_tab,
		          0,duve_cb->duverel->rel[index].du_own);
		if ( duve_talk(DUVE_ASK, duve_cb, S_DU0319_SET_INDEX, 0)
		     == DUVE_YES)
		{

		    tid = index_tbl.baseid;
	            tidx = index_tbl.indexid;
		    relstat = duve_cb->duverel->rel[index].du_stat | TCB_INDEX;
		    duve_cb->duverel->rel[index].du_stat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    if (sqlca.sqlcode == DU_SQL_OK)
		        duve_talk(DUVE_MODESENS, duve_cb, S_DU0369_SET_INDEX,0);
		}

	    } /* endif */
        } /* endif test 2 */
	
	/* test 3 -- verify elements in idom array not unreasonable */
	if ( (index != DU_INVALID) && (index != DUVE_DROP) )
	{   
	    bool	first=TRUE;
	    i4		i;
	    i2		*idom;

	    reltid.db_tab_index = 0;
	    base = duve_findreltid ( &reltid, duve_cb);
	    if (base == DU_INVALID)
	    {	/* this shouldn't happen, but somehow the base table
		** in iirelation doesn't exist, do delete index table
		** (test 16 should have caught this except in the rare
		** case where the base table was deleted after test 16
		*/
	        if (duve_banner( DUVE_IIINDEX, 3, duve_cb)
                == DUVE_BAD) 
	            return ( (DU_STATUS) DUVE_BAD);
	       duve_talk(DUVE_MODESENS, duve_cb, S_DU161A_NO_BASE_FOR_INDEX, 4,
		  0,duve_cb->duverel->rel[index].du_tab,
		  0,duve_cb->duverel->rel[index].du_own);
	       if ( duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[index].du_tab,
		  0,duve_cb->duverel->rel[index].du_own)
	       == DUVE_YES)
	       {
		   idxchk (duve_cb, index_tbl.baseid, index_tbl.indexid);
	           duve_cb->duverel->rel[index].du_tbldrop = DUVE_DROP;
		   duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
			0,duve_cb->duverel->rel[index].du_tab,
			0,duve_cb->duverel->rel[index].du_own);
		   continue;
	       }
	    }	/* endif base table missing */
	    else if (base == DUVE_DROP)
	    {
		/* the base table is marked for delete, which will drop
		** this index table.  The drops dont take place til the
		** end of the catalog checks, so skip proccessing this
		*/
		continue;
	    }
	    /* usually we will get here, and test to verify that all of the
	    ** elements in the key definition array (IDOM) are valid base
	    ** table column numbers
	    */
	    idom = &index_tbl.idom1;
	    for (i = 0; i < NUM_IDOMS; i++)
	    {
		if (idom[i] > duve_cb->duverel->rel[base].du_attno)
		{
		    if (first)
		    {
			if (duve_banner( DUVE_IIINDEX, 3, duve_cb)
			== DUVE_BAD) 
			    return ((DU_STATUS) DUVE_BAD);
			first = FALSE;
		    }
		    duve_talk( DUVE_MODESENS,duve_cb, 
			S_DU162A_INVALID_INDEXKEYS, 6,
			0,duve_cb->duverel->rel[index].du_tab,
			0,duve_cb->duverel->rel[index].du_own,
			sizeof(idom[i]), &idom[i]);
		    if (duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
				0, duve_cb->duverel->rel[index].du_tab,
				0, duve_cb->duverel->rel[index].du_own)
	            == DUVE_YES)
	            {
			/* delete this table */
		        idxchk (duve_cb, index_tbl.baseid, index_tbl.indexid);
			duve_cb -> duverel -> rel[index].du_tbldrop = DUVE_DROP;
			duve_talk( DUVE_MODESENS, duve_cb, 
			    S_DU0352_DROP_TABLE, 4,
			    0, duve_cb->duverel->rel[index].du_tab,
			    0, duve_cb->duverel->rel[index].du_own);
			break;
		    }  /* endif drop this index table */
		} /* endif this key is invalid */

	    } /* end loop */
	} 

	/* 
	** test 4 -- verify that if the index was created to enforce a 
	** constraint, then there is an entry in IIDBDEPENDS describing 
	** dependence of this index on a constraint
	*/
	if (duve_cb->duverel->rel[index].du_stat2 & TCB_SUPPORTS_CONSTRAINT)
	{
	    DU_STATUS	idx_dep_offset;

	    /* 
	    ** search IIDBDEPENDS cache for a dependent object info element 
	    ** describing this index
	    */
	    idx_dep_offset = duve_d_dpndfind(index_tbl.baseid, 
		index_tbl.indexid, (i4) DB_INDEX, (i4) 0, duve_cb);
	    if (idx_dep_offset == DU_INVALID)
	    { 
	        /* 
		** this index doesn't have a corresponding tuple 
		** in iidbdepends. 
		*/

	       if (duve_banner( DUVE_IIINDEX, 4, duve_cb) == DUVE_BAD)
                  return ( (DU_STATUS) DUVE_BAD);
	       duve_talk(DUVE_MODESENS, duve_cb, 
		   S_DU16A8_IIDBDEPENDS_MISSING, 4,
		   0,duve_cb->duverel->rel[index].du_tab,
		   0,duve_cb->duverel->rel[index].du_own);

	       if ( duve_talk( DUVE_ASK, duve_cb, S_DU0318_DROP_INDEX, 4,
			sizeof(index_tbl.baseid), &index_tbl.baseid,
			sizeof(index_tbl.indexid), &index_tbl.indexid)
	             == DUVE_YES)
	       {

		    /* delete this table */
		    idxchk (duve_cb, index_tbl.baseid, index_tbl.indexid);
		    duve_cb -> duverel -> rel[index].du_tbldrop = DUVE_DROP;
		    duve_talk( DUVE_MODESENS, duve_cb, S_DU0368_DROP_INDEX, 4,
			sizeof(index_tbl.baseid), &index_tbl.baseid,
			sizeof(index_tbl.indexid), &index_tbl.indexid);
               } 

	       continue;
	    } /* endif tuple not in iidbdepends */
	} /* endif index was created to enforce a constraint */
    } /* end loop for each tuple in iirel_idx */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckindex");	

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckrel	- perform system catalog checks/fixes on iirelation
**
** Description:
**      ckrel is an executive for the system catalog checks.  It detemines
**	how much memory to allocate for the duverel structure based on the
**	number of tuples in iirelation, then allocates the memory and assigns
**	it to the duverel structure.  Then ckrel opens a cursor and enters a
**	loop to read a tuple from iirelation, copy it the the DMP_RELATIION 
**	structure iirelation, and copies pertentent information to the duverel
**	structure.  Then ckrel runs the tests specified by the verifydb
**	product specification.  Each test has three possible return values:
**	    DUVE_YES   -  No errors, contintue testing this tuple
**	    DUVE_NO    -  No errors, stop testing this tuple and start
**		          next tuple
**	    DUVE_BAD   -  Error encountered.  Stop tests for all tuples.
**
**	CKREL ASSUMES THAT THE CALLING ROUTINE HAS ALREADY CONNECTED TO THE
**	DATABASE.
**
**	ckrel runs tests 1 throught test 28
**
** Inputs:
**      duvecb                          verifydb control block.
[@PARAM_DESCR@]...
**
** Outputs:
**      duve_cb                         verifydb control block, contains:
**	    duverel			    memory cache of information about
**					     each iirelation tuple
**	    duve_msg			    error message information if INGRES
**					     error encountered
**	    duve_cnt			    numver of duverel elements containing
**					     valid information
**	error msg:  E_DU501C_EMPTY_RELATION
**	Returns:
**	    DUVE_YES	-- completed all tests/repairs with no errors
**	    DUVE_BAD	-- errors encountered during processing
**	Exceptions:
**	    none
**
** Side Effects:
**	    iirelation structure filled with attributes from iirelation table
**	    contents of iirelation table may be modified as a result of processing
**	     controlled by this routine
**
** History:
**      15-aug-88 (teg)
**          initial creation.
**	09-feb-90 (teg)
**	    FIX AV that someone caused by changing relcmptlvl from 4 char array
**	    to I4.  (NOTE:  when duverel.sh is modified we will need to change
**	    this again.)
**	17-may-1992 (ananth)
**	    Increase IIrelation tuple size project.
**	    The columns relloccount, relgwid, relgwother, relhigh_logkey,
**	    rellow_logkey, relfhdr, relallocation and relextend are now
**	    visible to the front end. Copy them into the iirelation structure.
**	    Also uncomment test_20.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	16-jun-93 (anitap)
**	    Added call to test_28().
**	28-jun-93 (teresa)
**	    add calls to test_6 and test_7
**	03-dec-93 (andre)
**	    DU_RELINFO will now contain iirelation.relstat2 inside du_stat2
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb - performance statistics
**	    reporting.
**	24-jan-94 (andre)
**	    added call to test_30_and_31 
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
** 	    Test 15 removed.  tests for existence of view if TCB_VBASE is set.
** 	    TCB_VBASE not being cleared even when view is dropped and not tested
** 	    else where other than verifydb tests.
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now an integer.
*/
static DU_STATUS
ckrel(duve_cb)
DUVE_CB		*duve_cb;
{
    DMP_RELATION iirelation;
    DU_STATUS	status;
    i4	len;
    u_i4	size;
    STATUS	mem_stat;
    EXEC SQL BEGIN DECLARE SECTION;
	EXEC SQL INCLUDE <duverel.sh>;
	i4		    cnt;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR continue;

    EXEC SQL DECLARE rel_cursor CURSOR FOR 
	select * from iirelation order by reltid, reltidx;

    EXEC SQL OPEN rel_cursor;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    /*
    ** determine number of tuples and allocate memory for duverel
    */
    EXEC SQL select count(reltid) into :cnt from iirelation;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */
    else if (sqlca.sqlcode == DU_SQL_NONE || cnt == 0)
    {	
    	duve_talk (DUVE_ALWAYS, duve_cb, E_DU501C_EMPTY_IIRELATION, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }
    size = cnt * sizeof(DU_RELINFO);
    duve_cb->duverel = 
	(DUVE_RELINFO *) MEreqmem( 0, size, TRUE, &mem_stat);
    if ( (mem_stat != OK) || (duve_cb->duverel == NULL) )
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }

    for ( status = DUVE_YES, duve_cb->duve_cnt = 0; 
	  (duve_cb->duve_cnt < cnt);
	  duve_cb->duve_cnt++ )
    {

	if (status == DUVE_BAD)
	{	
	    duve_close(duve_cb,TRUE);
	    return DUVE_BAD;
	} /* endif */

	/*
	** get entry from iirelation table and put in iirelation
	** structure and in duverel array */

	EXEC SQL FETCH rel_cursor INTO :rel_tbl;
	if (sqlca.sqlcode == DU_SQL_NONE) 
	    goto Close_Cursor;
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

	iirelation.reltid.db_tab_base = rel_tbl.reltid;
	iirelation.reltid.db_tab_index = rel_tbl.reltidx;
	MEcopy( (PTR)rel_tbl.relid, sizeof(iirelation.relid.db_tab_name),
		(PTR)iirelation.relid.db_tab_name);
	MEcopy( (PTR)rel_tbl.relowner, sizeof(iirelation.relowner.db_own_name),
		(PTR)iirelation.relowner.db_own_name);
	iirelation.relatts = rel_tbl.relatts;
	iirelation.relwid = rel_tbl.relwid;
	iirelation.relkeys = rel_tbl.relkeys;
	iirelation.relspec = rel_tbl.relspec;
	iirelation.relstat = rel_tbl.relstat;
	iirelation.reltups = rel_tbl.reltups;
	iirelation.relpages = rel_tbl.relpages;
	iirelation.relprim = rel_tbl.relprim;
	iirelation.relmain = rel_tbl.relmain;
	iirelation.relsave = rel_tbl.relsave;
	iirelation.relstamp12.db_tab_high_time = rel_tbl.relstamp1;
	iirelation.relstamp12.db_tab_low_time = rel_tbl.relstamp2;

	len = STtrmwhite(rel_tbl.relloc);

	len = ( len > sizeof(iirelation.relloc.db_loc_name) ) ?
			sizeof(iirelation.relloc.db_loc_name) :
			len;
	rel_tbl.relloc[len] = '\0';
	MEcopy( (PTR)rel_tbl.relloc, sizeof(iirelation.relloc.db_loc_name),
		(PTR)iirelation.relloc.db_loc_name );
	iirelation.relcmptlvl = rel_tbl.relcmptlvl;
	iirelation.relcreate = rel_tbl.relcreate;
	iirelation.relqid1 = rel_tbl.relqid1;
	iirelation.relqid2 = rel_tbl.relqid2;
	iirelation.relmoddate = rel_tbl.relmoddate;
	iirelation.relidxcount = rel_tbl.relidxcount;
	iirelation.relifill = rel_tbl.relifill;
	iirelation.reldfill = rel_tbl.reldfill;
	iirelation.rellfill = rel_tbl.rellfill;
	iirelation.relmin = rel_tbl.relmin;
	iirelation.relmax = rel_tbl.relmax;
	iirelation.relloccount = rel_tbl.relloccount;
	iirelation.relgwid = rel_tbl.relgwid;
	iirelation.relgwother = rel_tbl.relgwother;
	iirelation.relhigh_logkey = rel_tbl.relhigh_logkey;
	iirelation.rellow_logkey = rel_tbl.rellow_logkey;
	iirelation.relfhdr = rel_tbl.relfhdr;
	iirelation.relallocation = rel_tbl.relallocation;
	iirelation.relextend = rel_tbl.relextend;
	iirelation.relstat2  = rel_tbl.relstat2;
	
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_id.db_tab_base = 
						iirelation.reltid.db_tab_base;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_id.db_tab_index = 
						iirelation.reltid.db_tab_index;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = iirelation.relstat;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat2 = iirelation.relstat2;

	len = STtrmwhite(rel_tbl.relid);
	STcopy(rel_tbl.relid,
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab);

	duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab[len] = '\0';

	len = STtrmwhite(rel_tbl.relowner);
	STcopy(rel_tbl.relowner,
			duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_own[len] = '\0';
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_attno = iirelation.relatts;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_qid1 = rel_tbl.relqid1;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_qid2 = rel_tbl.relqid2;
	duve_cb->duverel->rel[duve_cb->duve_cnt].du_idxcnt =rel_tbl.relidxcount;
	do	/* this loop is just for error control */
	{
	    /* NOTE: each verifydb system catalog  check returns DUVE_YES
			if testing should continue for this iirelation tuple,
			no permission if testing should stop for this iirelation
			tuple (but no verifydb error is encountered) and
			DUVE_BAD if verifydb encounters an error requires it
			stop processing on this database */


	    if ( (status = test_1( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_2( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_3( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_4( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_5( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_6( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_7( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_8( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_9( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_10( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_11( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_12( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_13( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_14( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    /*
	    ** Test 15 removed.  tests for existence of view if
	    ** iirelation.relstat.TCB_VBASE = true.  TCB_VBASE
	    ** not being cleared when view is dropped and 
	    ** not tested else where other than verifydb tests.
	    */
	    if ( (status = test_16( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    /* test 17 in test_16 for coding efficiency */
	    if ( (status = test_18( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_19( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_20( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_21( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_22( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    /* tests 23, 24 and 25 in test_22 for coding efficiency */
	    if ( (status = test_26( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_27( duve_cb, iirelation)) !=  DUVE_YES)
		break;
	    if ( (status = test_28( duve_cb, iirelation)) != DUVE_YES)
		break;
	    if ( (status = test_29( duve_cb, iirelation)) != DUVE_YES)
		break;
	    if ( (status = test_30_and_31( duve_cb, iirelation)) != DUVE_YES)
		break;
	    if ( (status = test_95( duve_cb, iirelation)) != DUVE_YES)
		break;

	} while (0); /* end error control loop */

    } /* end loop for each tuple in iirelation */

Close_Cursor:
    EXEC SQL CLOSE rel_cursor;

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckrel");
    
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckrelidx	- run system catalog check/fix on iirel_idx
**
** Description:
**      This ESQL routine gets a tuple out of iirel_idx, then searchs for a
**	tuple in iirelation that matches this tuple.  If no match is found,
**	the corrective action is to delete the tuple from iirel_idx.
**
**	ckrelidx does not actually querry the iirelation table, it uses the
**	cached version in duverel.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	MESSAGES TO USER and/or LOG FILE:
**	    S_DU1615_MISSING_IIRELATION,
**	    S_DU0316_CLEAR_IIRELIDX,
**	    S_DU0366_CLEAR_IIRELIDX
**
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirel_idx may be altered
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**	    calls have ().
**	09-oct-91 (teresa)
**	    Implement trace point 1 to work around a known iirel_idx.tidp
**	    corruption problem.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb - performance statistics
**	    reporting.
[@history_template@]...
*/
static DU_STATUS
ckrelidx(duve_cb)
DUVE_CB		*duve_cb;
{
    EXEC SQL BEGIN DECLARE SECTION;
	EXEC SQL INCLUDE <duveridx.sh>;
	char	*name,*own;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR continue;

    if (duve_cb->duve_trace[DUVE01_SKIP_IIRELIDX])
    {
	/* this trace silently skips operations on iirel_idx because of
	** a known 6.5 bug where the tidp is corrupted and generates error
	** "E_LQ000C Column 3 can not be converted into program variable."
	*/
	return ( (DU_STATUS) DUVE_YES);
    }


    EXEC SQL DECLARE ridx_cursor CURSOR FOR 
	select * from iirel_idx;
    EXEC SQL OPEN ridx_cursor FOR READONLY;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    for ( ;; )
    {

	/*
	** get entry from iirel_idx 
	*/

	EXEC SQL FETCH ridx_cursor INTO :ridx_tbl;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE ridx_cursor;
	    break;
        }
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

	/* null terminate table name and owner name */
	(VOID) STtrmwhite (ridx_tbl.relid);
	(VOID) STtrmwhite (ridx_tbl.relowner);

	if ( duve_relidfind( ridx_tbl.relid, ridx_tbl.relowner, duve_cb )
	== DU_INVALID)
	{    
	     /* this iirel_idx tuple doesn't have a corresponding tuple
	     ** in iirelation. */

	    if (duve_banner( DUVE_IIREL_IDX, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	     duve_talk( DUVE_MODESENS, duve_cb, S_DU1625_MISSING_IIRELATION, 4,
			0, ridx_tbl.relid, 0, ridx_tbl.relowner);
	     if (duve_talk( DUVE_ASK, duve_cb, S_DU0316_CLEAR_IIRELIDX, 0)
	     == DUVE_YES)
	     {
	        name = ridx_tbl.relid;
		own  = ridx_tbl.relowner;
		exec sql delete from iirel_idx where relid = :name and
			relowner = :own;
		if (sqlca.sqlcode == DU_SQL_OK)
		    duve_talk( DUVE_MODESENS, duve_cb, S_DU0366_CLEAR_IIRELIDX,
				0);
	     }
	} /* endif invalid iirel_idx tuple */
    } /* end loop for each tuple in iirel_idx */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckrelidx");	

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: Clean_Up	- ESQL error handler
**
** Description:
**      ESQL error handling procedure (print error, abort transaction and 
**	disconnect). 
**
** Inputs:
**	duvecb -- verifydb control block (as global)
**
** Outputs:
**	duvecb.duve_msg		    message control block
**	MESSAGE PRINTED TO LOG and STDOUT
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    INQUIRE INGRES called to get error msg text and error msg number
**
** History:
**      15-Aug-88 (teg)
**          initial creation
**	30-mar-90 (teg)
**          remove rollback command, as duve_exit will also issue the rollback
**	    command.
**	8-aug-90 (teresa)
**	    added logic to expect DUVE_FATAL (stop processing), DUVE_WARN
**	    (print warning message from DBMS) or DUVE_SPECIAL (duve_ingerr
**	    routine has already printed special case msg so exit silently.)
**	16-apr-92 (teresa)
**	    pick up ingres63p change 265521 for OS2 port:
**		errno -> errnum
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb - modify calls to duve_log()
**	    to handle new parameter to suppress non-significant diffs while
**	    executing run_all.
[@history_template@]...
*/
VOID
Clean_Up()
{

    DU_STATUS	stat;

    EXEC SQL WHENEVER SQLERROR continue;

    EXEC SQL BEGIN DECLARE SECTION;
	char errmsg[ER_MAX_LEN];
	i4 errnum;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL INQUIRE_INGRES (:errnum = ERRORNO);

    EXEC SQL INQUIRE_INGRES (:errmsg = ERRORTEXT);
    stat = duve_ingerr (errnum);
    if ( stat == DUVE_FATAL)
    {
	/* This is a fatal error.  We should NOT continue processing */
	SIprintf("Aborting because of error\n%s",errmsg);
	duve_log (&duvecb, 0, "Aborting because of error");
	duve_log( &duvecb, 0, errmsg);
	duve_exit(&duvecb);
    }
    else if (stat == DUVE_WARN)
    {
	/* print warning returned from the server, then continue processing */
	SIprintf("WARNING: %s",errmsg);
	duve_log( &duvecb, 0, errmsg);
    }
    else if (stat == DUVE_SPECIAL)
    {
	/* duve_ingerr has already output a special message, so do not
	** output message from dbms server.  Just exit. */
	duve_exit(&duvecb);
    }
}

/*{
** Name: fixrelstat	- clears optimizer statistics exist from iirelation's
**			  relstat if there are no corresponding statistics.
**
** Description:
**      this routine forces -MRUNSILENT mode.  It walks all tuples in
**	iirelation.  If iirelation indicates that statistics should exist, it
**	checks for corresponding entries in iistatistics.   If none are found,
**	it clears the iirelation status bit that indicates statistics exist.
**
** Inputs:
**      none
**
** Outputs:
**	none
**	Returns:
**	    DUVE_YES
**	Exceptions:
**	    none
**
** Side Effects:
**	    accesses/modifies tuples in iirelation via ESQL
**	    accesses tuples in iistatistics via ESQL
**
** History:
**      15-aug-88 (teg)
**          initial creation.
**	11-feb-91 (teresa)
**	    integrate INGRES6302P change 340465 to assure all Clean_up subroutin
**	    calls have ().
[@history_template@]...
*/
static DB_STATUS
fixrelstat()
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4		    status;
	i4		    numstat;
	u_i4		    tid,tidx;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL DECLARE r_cursor CURSOR FOR 
	select relstat, reltid, reltidx from iirelation
	FOR DEFERRED UPDATE OF relstat;
    EXEC SQL OPEN r_cursor;
    if (sqlca.sqlcode < DU_SQL_OK)
	Clean_Up();	/* Clean_Up will exit with error */

    for ( ;; )
    {
	/*
	** get entry from iirelation
	*/
	EXEC SQL FETCH r_cursor INTO :status, :tid, :tidx;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE r_cursor;
	    break;
        }
	else if (sqlca.sqlcode < DU_SQL_OK)
	    Clean_Up();

	if (status & TCB_ZOPTSTAT)
	{  /* it is a catalog.  Probably shouldn't have TCB_ZOPTSTAT set, but
	      check iistatistics, just to be sure */


	    exec sql repeated select any(stabbase) into :numstat
		from iistatistics 
	        where stabbase = :tid and stabindex = :tidx;

	    if (numstat)
	        continue;

	    /* if we get here, the table indicates it has statistics, but
	    ** doesn't really
	    */
	    status &= ~TCB_ZOPTSTAT;
	    EXEC SQL UPDATE iirelation set relstat = :status
	        WHERE CURRENT OF r_cursor;
	} /* endif iirelation indicates statistics exist */

    } /* end loop for each tuple in iirelation */
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: idxchk	- index check 
**
** Description:
**      checks to see if the table being dropped is an index table.  If so, 
**      then sees if the index's base table is valid.  If so, then decrements
**	index count for that table and, if index count hits zero, clears
**	TCB_IDXD bit from iirelation.relstat of the base table.
**
**	idxchk does not return any values.  If an error is encountered, it
**	will be trapped by the appropriate error handler.
**
** Inputs:
**      duve_cb                         verifydb control block
**	baseid				base portion of table id
**	indexid				index portion of table id
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    cached info about iirelation may be modified
**	    iirelation tuples may be modified
**
** History:
**      29-sep-1988 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/
VOID
idxchk(duve_cb, baseid, indexid)
DUVE_CB            *duve_cb;
u_i4		    baseid;
u_i4		    indexid;
{

    exec sql begin declare section;
	u_i4		tid;
	i4		relstat;
	i2		idxcnt;
    exec sql end declare section;   
    i4	base;
    DB_TAB_ID	tableid;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if (indexid == 0)
	return;		/* this is not an index table, so go home*/


    /* find base table in iirelation cache */
    tableid.db_tab_base = baseid;
    tableid.db_tab_index = 0;
    base = duve_findreltid( &tableid, duve_cb);

    /* if duverel pointer we have gotten is invalid, return */
    if (base < 0)
	return;

    /* decrement index count. If it goes to zero, clear tcb_idxd bit from
    ** iirelation.relstat.  If index count dips below zero, something is wrong.
    ** repair by setting index count to zero and clearing tcb_idxd bit
    */

    idxcnt = duve_cb->duverel->rel[base].du_idxcnt -1;
    if (idxcnt < 0)	
	idxcnt = 0;
    duve_cb->duverel->rel[base].du_idxcnt = idxcnt;

    relstat = (idxcnt) ? duve_cb->duverel->rel[base].du_stat :
			 duve_cb->duverel->rel[base].du_stat & ~TCB_IDXD;

    tid = tableid.db_tab_base;
    duve_cb->duverel->rel[base].du_stat = relstat;
    EXEC SQL update iirelation set relstat = :relstat, relidxcount = :idxcnt
	where reltid = :tid and reltidx = 0;

    return;
}

/*{
** Name: test_1	- Run verifydb test # 1
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**	1.	verify that the base table id is valid
**	TEST:	iirelation.reltid is nonzero
**	FIX:    If the tablename with the zero value reltid is a dmbs core 
**		catalog, then set reltid to the correct value.  Otherwise 
**		report the tablename for which the reltid is zero.
**		    IF MODE = RUNINTERACTIVE
**			request permission to delete this tuple from iirelation.
**		    ELSE
**			indicate that user must run interactive mode or 
**			 manually fix
**		    ENDIF
**	NOTES:	Deletion of this tuple is permitted only in interactive mode 
**		because removing the table from the system catalogs is 
**		effectively throwing away the data in that table.  Verifydb 
**		only permits destruction of data in interactive mode.  However, 
**		in this particular case, it is not possible for the dmbs to 
**		get to the user data since the filename is keyed off of reltid.
**	MSGS:	S_DU1600, S_DU0301, S_DU0351
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/


static DU_STATUS
test_1  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

	/* test 1 - verify base table id is valid */
	if (iirelation.reltid.db_tab_base == 0)		    
	{
	    /* output error message that table id is invalid */
	    if (duve_banner( DUVE_IIRELATION, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1600_INVALID_RELID, 4,
			0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab, 
			0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk (DUVE_IO, duve_cb,
			    S_DU0301_DROP_IIRELATION_TUPLE, 4,
			    0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab, 
			    0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own) 
		== DUVE_YES)
	    {
		/* do fix, which is to drop tuple from iirelation */
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
		duve_talk (DUVE_MODESENS, duve_cb,
			    S_DU0351_DROP_IIRELATION_TUPLE, 4,
			    0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab, 
			    0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		return ( (DU_STATUS)DUVE_NO);
	    } /* endif */
	    else
		/* 
		** no futher tests should be run on this tuple, cuz its useless 
		*/
		return ( (DU_STATUS)DUVE_NO);
	} /* endif */

	return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_2	- Run verifydb tests # 2a and 2b
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
** 2.	verify that the iiattributes table describes the columns in agreement
**	with the iirelation table
**TEST:	read all iiattribute tuples where iiattribute.attrelid=iirelation.reltid
**	and iiattribute.attrelidx = iirelation.reltidx, then run tests 2a and 2b
**FIX:	specified in each subtest.
**
**2a.	verify that the number of attributes for the table is correct
**TEST:	a) verify that each iiattribute.attid is unique, positive and less
**	   than or equal to DB_MAXCOLS
**	b) verify that the largest iiattribute.attid = iirelation.relatts
**FIX:	Report any mismatches.  Report any duplicate attids.  Report any
**	sequence errors (ie, attno=1, attno=2, attno=4 with no attno=3).
**	IF MODE = RUNINTERACTIVE
**	    request permission to drop this table from all system catalogs
**	ELSE
**	    indicate that user must run interactive mode or manually fix
**	ENDIF
**NOTES:It is not possible for verifydb to know if the iirelation.relatts is
**	correct and a tuple(s) is missing from iiattributes or if iiattributes
**	is correct.  It verifydb guesses wrong, the dmbs would not be able to
**	correctly retrieve data from the disk file.  Therefore the DBA has the
**	option of manually fixing this descrepancy in the catalogs or of
**	dropping the table that this iirelation tuple describes.
**MSGS:	S_DU1601, S_DU1602, S_DU1603, S_DU1604, S_DU0302, S_DU0352
**
** 2b.	verify that the column width is correct.
**TEST:	the sum of iiattribute.attfrml = iirelation.relwid
**FIX:	report that the widths do not match.
**	IF MODE = RUNINTERACTIVE
**	    request permission to replace iirelation.relwid with the sum of 
**	     iiattribute.attfrml
**	ELSE
**	    indicate that user must run interactive mode or manually fix
**	ENDIF
**NOTES:Its not possible for verifydb to know whether iirelation.relwid is
**	wrong or whether iiattribute has an incorrect attfrml or is missing
**	some tuples.   That is why the suggested repair can only be performed
**	in interactive mode.  If the fix that verifydb suggests is not the
**	correct fix, then the user must manually correct the system catalogs.
**MSGS:	S_DU1605, S_DU0303, S_DU0353
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
[@PARAM_DESCR@]...
**
** Outputs:
**      duve_cb                         verifydb control block
[@PARAM_DESCR@]...
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	06-mar-1997 (nanpr01)
**	    If table is altered, attributes will not match and we donot
**	    want to give an error. Same is true for width.
**	11-jan-04 (inkdo01)
**	    Exclude physical partition iirelation rows from iiattribute tests.
**	29-apr-2010 (miket) SIR 122403
**	    Fix verification of encrypted tables.
[@history_template@]...
*/

static DU_STATUS
test_2  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation ;
{

    i4		ctr;
    bool	drop_me = FALSE;
    exec sql begin declare section;
	i2	attid;
	i2	attver_dropped;
	i4 agg;
	i4 encagg;
	i2	null_agg=0;
	i2	null_encagg=0;
	u_i4	tid,tidx;
    exec sql end declare section;
    
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    tid = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    exec sql repeated select max(attid) into :attid:null_agg from iiattribute
	where attrelid  = :tid and attrelidx = :tidx;

    do		/* error control */
    {	
	/* 
	**  test 2a. verify that the number of attributes is correct
	*/

	if ((attid != iirelation.relatts) && 
	    ((iirelation.relstat2 & TCB2_ALTERED) == 0) &&
	    ((iirelation.relstat2 & TCB2_PARTITION) == 0))
	{
            if (duve_banner( DUVE_IIRELATION, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1601_INVALID_ATTID, 8,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		   sizeof(iirelation.relatts), &iirelation.relatts,
		   sizeof(attid), &attid);

	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0302_DROP_TABLE,
			   4, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
		== DUVE_YES)
	    {
		idxchk( duve_cb, iirelation.reltid.db_tab_base,
		        iirelation.reltid.db_tab_index);
	    	duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE,
			   4, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		return ( (DU_STATUS)DUVE_NO);
	    } /* endif */
	    else
		/*
		** stop running tests on this tuple, we already know it
		** is bad
		*/
		return ( (DU_STATUS)DUVE_NO);
	} /* endif */

	ctr = 0;
	EXEC SQL repeated SELECT attid, attver_dropped 
		INTO :attid, :attver_dropped FROM iiattribute 
		WHERE attrelid = :tid and attrelidx = :tidx ORDER BY attid;
	EXEC SQL BEGIN;
	    if (attver_dropped)
		continue;
	    ctr++;
	    if (sqlca.sqlcode < DU_SQL_OK)
		Clean_Up();
	    else if (attid < ctr)
	    {
                if (duve_banner( DUVE_IIRELATION, 2, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
	    	duve_talk (DUVE_MODESENS, duve_cb, S_DU1602_DUPLICATE_ATTIDS, 6,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
			   sizeof(attid), &attid);
		if ( duve_talk (DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
		     == DUVE_YES)
		{
		    drop_me = TRUE;
		    EXEC SQL endselect;
		} /* endif */
		ctr = attid;
	    } /* endif */
	    else if (attid > ctr)
	    {
                if (duve_banner( DUVE_IIRELATION, 2, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
	    	duve_talk (DUVE_MODESENS, duve_cb, S_DU1603_MISSING_ATTIDS, 6,
			   sizeof(ctr), &ctr,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		if ( duve_talk (DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
		     == DUVE_YES)
		{
		    drop_me = TRUE;
		    EXEC SQL endselect;
		} /* endif */
		ctr = attid;
	    } /* endif */
	EXEC SQL END;

	if (drop_me)
	{
	    /* have to do this here cuz idxchk does a query and that's not
	    ** allowed in a select loop
	    */
	    idxchk( duve_cb, iirelation.reltid.db_tab_base,
		    iirelation.reltid.db_tab_index);
	    duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE,
			   4, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    return ( (DU_STATUS)DUVE_NO);
	}


	/* 2b. verify that the column widths are correct */

	exec sql repeated select sum(attfrml) into :agg:null_agg
	    from iiattribute
	    where attrelid = :tid and attrelidx = :tidx
	    and attencflags = 0;
	
	exec sql repeated select sum(attencwid) into :encagg:null_encagg
	    from iiattribute
	    where attrelid = :tid and attrelidx = :tidx
	    and attencflags != 0;
	
	if (null_agg)	/* adjust agg if no tuples found to make aggregate */
	    agg = 0;
	if (null_encagg)/* adjust encagg if no tuples found to make aggregate */
	    encagg = 0;
	agg += encagg;	/* sum the unencrypted and encrypt widths */

	if ((agg != iirelation.relwid) &&
	    ((iirelation.relstat2 & TCB2_ALTERED) == 0) &&
	    ((iirelation.relstat2 & TCB2_PARTITION) == 0))
	{
	    /* output error message that column widths for table is wrong */
            if (duve_banner( DUVE_IIRELATION, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1605_INCORRECT_RELWID, 8,
		      sizeof(iirelation.relwid), &iirelation.relwid, 
		      sizeof(agg), &agg, 
		      0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		      0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);

	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0303_REPLACE_RELWID, 4,
			   sizeof(iirelation.relwid), &iirelation.relwid, 
			   sizeof(agg), &agg)
		== DUVE_YES)
	    {
		EXEC SQL update iirelation set relwid = :agg where
		    reltid = :tid and reltidx = :tidx;
		duve_talk( DUVE_MODESENS, duve_cb, S_DU0353_REPLACE_RELWID, 
			   4, sizeof(iirelation.relwid), &iirelation.relwid, 
			   sizeof(agg), &agg);
	    } /* endif */

	} /* endif */

    } while (0);	/* end error control loop */
	    
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_3	- Run verifydb test # 3
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**3.	verify that the number of columns comprising the key is correct
**TEST:	count the number of iiattribute tuples where (attkey = ISKEY).
**	Test that this count = iirelation.relkeys
**FIX:	replace iirelation.relkeys with this count.
**MSGS:	S_DU1606, S_DU0304, S_DU0354
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples are read from table iiattribute
**	    iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_3 ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation ;
{

    exec sql begin declare section;
	i4	agg;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;
    
	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	/* test 3 - verify # columns in the key is correct */
	exec sql repeated select count(attkdom) into :agg from iiattribute
	    where attrelid = :tid and attrelidx = :tidx and attkdom != 0;
	if (agg != iirelation.relkeys)
	{
	    /* output error message that # keys in table is wrong */
            if (duve_banner( DUVE_IIRELATION, 3, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1606_KEYMISMATCH, 8,
		      sizeof(iirelation.relkeys), &iirelation.relkeys,
		      sizeof(agg), &agg,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);

	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU0304_REPLACE_RELKEYS, 4,
			      sizeof(iirelation.relkeys), &iirelation.relkeys,
			      sizeof(agg), &agg)

		== DUVE_YES)
	    {
		EXEC SQL update iirelation set relkeys = :agg where
		  reltid = :tid and reltidx = :tidx;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU0354_REPLACE_RELKEYS, 4,
		          sizeof(iirelation.relkeys), &iirelation.relkeys,
			  sizeof(agg), &agg);

	    } /* endif */
	} /* endif */
	return ( (DU_STATUS)DUVE_YES);

}

/*{
** Name: test_4	- Run verifydb test #4
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**4.	verify that the table storage structure is valid
**TEST:	verify that iirelation.relspec is in (HEAP, ISAM, HASH, BTREE)
**FIX:	If the table in question is a dmbs core catalog, then set 
**	iirelation.relspec to the correct value for that table.  Otherwise,
**	report that the table storage structure is invalid, and request that
**	the user run patchlink, which converts table to a heap.  Then user may
**	use the "modify" command to convert table to whatever structure the
**	user desires.
**NOTE:	the inconsistancy will be fixed by patchlink, which will set the
**	iirelation.relspec to heap.
**MSGS:	S_DU1607
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    iirelation tuple contents may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/


static DU_STATUS
test_4 ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation ;
{
    exec sql begin declare section;
	i2	relspec;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;
    
	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	/* test 4 - verify table's storage structure is valid */
	if ( (iirelation.relspec != TCB_HEAP) && 
	     (iirelation.relspec != TCB_ISAM) && 
	     (iirelation.relspec != TCB_HASH) &&
	     (iirelation.relspec != TCB_BTREE) )
	{
	    if (iirelation.reltid.db_tab_base <= DM_SCONCUR_MAX)
	    { /* then this is a core dbms system catalog, so verifydb knows what the
	      ** storage structure should be, and can fix it. */
                if (duve_banner( DUVE_IIRELATION, 4, duve_cb)
	        == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU164D_INVAL_SYS_CAT_RELSPEC,
			  4, 0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			  sizeof( iirelation.relspec), &iirelation.relspec);
		{
		    if ( duve_talk(DUVE_ASK, duve_cb, S_DU032A_SET_RELSPEC_TO_HASH,
			 2, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab)
		       == DUVE_YES)
		    {
			 relspec = TCB_HASH;
			 exec sql update iirelation set relspec = :relspec
			   where reltid = :tid and reltidx = :tidx;
			 duve_talk(DUVE_MODESENS, duve_cb, 
			    S_DU037A_SET_RELSPEC_TO_HASH, 2, 0,
			    duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab);
	            } /* endif set to hash */
		}
	    }
	    else	/* this is not a core dbms catalog, so user must fix*/
	    {
	        /* output error message that table's storage structure is 
		** unknown */
                if (duve_banner( DUVE_IIRELATION, 4, duve_cb)
	        == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
	        duve_talk(DUVE_MODESENS, duve_cb, S_DU1607_INVALID_RELSPEC, 4,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    }

	}	     

	return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_5	- Run verifydb test # 5a and 5b
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**5.	verify that system catalogs are correctly marked as such.
**TEST:	run tests 5a and 5b
**FIX:	specified in each subtest.
**
**5a.	verify table is system catalog if it thinks it is
**TEST:	if iirelation.relstat.CATALOG bit = true then verify that
**	iirelation.relid starts with the letters "ii" and the owner is
**	"$INGRES"
**FIX:	clear iirelation.relstat.CATALOG bit
**MSGS:	S_DU1608, S_DU030A, S_DU035A
**
**5b.	verify that any system catalog is marked as one.
**TEST:	if the iirelation.relid starts with the letters "ii" then
**	verify that iirelation.relowner = "$INGRES" and that the
**	iirelation.relstat.CATALOG bit is set.
**FIX:	Report any table that starts with ii and is not owned by $INGRES
**MSGS:	S_DU1609
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation table may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
*/

/* Helper function to determine whether a table name is apparently a
** catalog name.
*/

static bool
is_catalog_name_own(DMP_RELATION *iirel)
{
    i4 len;

    len = cui_trmwhite(sizeof(iirel->relid.db_tab_name), iirel->relid.db_tab_name);

    return (len > 2
	    && (STncasecmp("ii", iirel->relid.db_tab_name, 2) == 0
		|| (len == 15 && STncasecmp("spatial_ref_sys", iirel->relid.db_tab_name,len) == 0)
		|| (len == 16 && STncasecmp("geometry_columns", iirel->relid.db_tab_name,len) == 0) )
	    && STncasecmp("$ingres", iirel->relowner.db_own_name,7) == 0);
}


static DU_STATUS
test_5  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	/* test 5 - verify that table correctly marked as system catalog */
	/* 5a */
	if ( ( iirelation.relstat & TCB_CATALOG ) || 
	     ( iirelation.relstat & TCB_EXTCATALOG) )
	{
	    if (! is_catalog_name_own(&iirelation))
	    {
                if (duve_banner( DUVE_IIRELATION, 5, duve_cb)
	        == DUVE_BAD) 
	  	    return ( (DU_STATUS)DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU1608_MARKED_AS_CATALOG, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		if ( duve_talk(DUVE_ASK, duve_cb, S_DU030A_CLEAR_CATALOG, 0)
		     == DUVE_YES)
		{

		    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			      & ~(TCB_CATALOG | TCB_EXTCATALOG);
		    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU035A_CLEAR_CATALOG,0);
		} /*  endif */
	    } /* endif */
	} /* endif */
	else /* 5b -- this is not a catalog, so it better not start with 'ii'
	     **	      and be owned by $ingres
	     */
	    if (is_catalog_name_own(&iirelation))
	    {
                if (duve_banner( DUVE_IIRELATION, 5, duve_cb)
	        == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
		/* output error message that table should be marked as a    
		    catalog and is not. */
		duve_talk(DUVE_MODESENS, duve_cb, S_DU1609_NOT_CATALOG, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    } /* endif   */

	return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_6	- Run verifydb test 6
**
** Description:
**      This routine runs verifydb check/repair #6, defined in the VERIFYDB
**	DBMS_CATALOGS product specification.
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation table may be modified
**
** History:
**      28-jun-93 (teresa)
**          initial creation
**	11-jan-04 (inkdo01)
**	    Exclude physical partition iirelation rows from index tests.
*/
static DU_STATUS
test_6 ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    /* test 6 - Verify secondary indexes are marked as such */
    
    if (iirelation.reltid.db_tab_index &&
	(iirelation.relstat2 & TCB2_PARTITION) == 0)
    {
	if ( (iirelation.relstat & TCB_INDEX) == 0)
	{
	    if (duve_banner( DUVE_IIRELATION, 6, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1695_NOT_MARKED_AS_IDX, 4,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		      0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU034C_SET_INDEX, 0)
		 == DUVE_YES)
	    {

		relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			  | TCB_INDEX;
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		iirelation.relstat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
		    reltid = :tid and reltidx = :tidx;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU039C_SET_INDEX,0);
	    } /*  endif */
	} /* endif */
    } /* endif */

    return ( (DU_STATUS)DUVE_YES);

}

/*{
** Name: test_7	- Run verifydb test 7
**
** Description:
**      This routine runs verifydb check/repair #7, defined in the VERIFYDB
**	DBMS_CATALOGS product specification.
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation table may be modified
**
** History:
**      28-jun-93 (teresa)
**          initial creation
**	11-jan-04 (inkdo01)
**	    Exclude physical partition iirelation rows from index tests.
*/
static DU_STATUS
test_7 ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	u_i4	tid,tidx;
	i4	agg;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    if (iirelation.reltid.db_tab_index &&
	(iirelation.relstat2 & TCB2_PARTITION) == 0)
    {
	exec sql repeated select any(baseid) into :agg from iiindex
	    where baseid = :tid and indexid = :tidx;
	if (agg==0)
	{
            if (duve_banner( DUVE_IIRELATION, 7, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    /* output error message that iiindex entry is missing */
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1696_IIINDEX_MISSING, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	    {
	        duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
	        duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	        return ( (DU_STATUS)DUVE_NO);
	    }

	}/* endif */

    }/* endif */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_8	- Run verifydb test #8
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**8.	verify that there are really protections specified for this table
**	if iirelation says that there are
**TEST:	if iirelation.relstat.PRTUPS bit set, then verify a nonzero count
**	of iiprotect tuples that meet this condition: iiprotect.protabbase =
**	iirelation.reltid and iiprotect.protabidx = iirelation.reltidx.
**FIX:	clear iirelation.relstat.PRTUPS bit.
**MSGS:	S_DU1611, S_DU0305, S_DU0355
**
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iiprotect
**	    contents of iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_8  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	i4	agg;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    /* test 8 - verify protections really defined on table if iirelation
		thinks they are */
    if (iirelation.relstat & TCB_PRTUPS)
    {
	exec sql repeated select any(protabbase) into :agg from iiprotect
	    where protabbase = :tid and protabidx = :tidx;
	if (agg==0)
	{
            if (duve_banner( DUVE_IIRELATION, 8, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    /* output error message that protections defined on table are lost*/
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1611_NO_PROTECTS, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if (duve_talk(DUVE_ASK, duve_cb, S_DU0305_CLEAR_PRTUPS, 0)
	        == DUVE_YES)
	    {	
		    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			      & ~TCB_PRTUPS;
		    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0355_CLEAR_PRTUPS,0);
	    } /* endif */

	}/* endif */

    }/* endif */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_9	- Run verifydb test # 9
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**9.	verify that there are really integrities specified for this table
**	if iirelation says that there are
**TEST:	if iirelation.relstat.INTEGS bit set, then verify a nonzero count
**	of iiintegrityu tuples that meet this condition: 
**	iiintegrity.inttabbase = iirelation.reltid and 
**	integrity.inttabidx = iirelation.reltidx.
**FIX:	clear iirelation.relstat.INTEGS bit.
**MSGS:	S_DU1612, S_DU0306, S_DU0356
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iiintegrity
**	    contents of iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_9  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	i4	agg;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    /* test 9 - verify integrities really defined on table if iirelation
	        thinks they are */

    if (iirelation.relstat & TCB_INTEGS)
    {
	exec sql repeated select any(inttabbase) into :agg from iiintegrity
	    where inttabbase = :tid and inttabidx = :tidx;
	if ( agg == 0) 
	{
	    /* output error message that integrities defined on table are lost*/
            if (duve_banner( DUVE_IIRELATION, 9, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1612_NO_INTEGS, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if (duve_talk(DUVE_ASK, duve_cb, S_DU0306_CLEAR_INTEGS, 0)
	        == DUVE_YES)
	    {	
		    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			      & ~TCB_INTEGS;
		    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0356_CLEAR_INTEGS, 0);
	    } /* endif */

	} /* endif */

    } /* endif */
 
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_10 - Run verifydb test # 10
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**10.	verify that this is a core catalog if the special locking bit is set.
**TEST:	if iirelation.relstat.CONCUR bit = true then iirelation.relid must be
**	one of (iirelation, iirel_idx, iiattributes, iiindex or iidevices).
**FIX:	clear iirelation.relstat.CONCUR bit.
**MSGS:	S_DU1613, S_DU0307, S_DU0357
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_10  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if (iirelation.relstat & TCB_CONCUR)
    {
	if ( (STncasecmp ("iirelation", iirelation.relid.db_tab_name,10)
	     ==DU_IDENTICAL) ||
	     (STncasecmp ("iirel_idx", iirelation.relid.db_tab_name,9)
	     ==DU_IDENTICAL) ||
	     (STncasecmp ("iiattribute", iirelation.relid.db_tab_name,11)
	     ==DU_IDENTICAL) ||
	     (STncasecmp ("iidevices", iirelation.relid.db_tab_name,9)
	     ==DU_IDENTICAL) ||
	     (STncasecmp ("iiindex", iirelation.relid.db_tab_name,7)
	     ==DU_IDENTICAL) )
	{
	    /* this is a dbms core catalog, so this bit should be set. */
	    return ( (DU_STATUS)DUVE_YES);
	}
	/* if we get here, then the CONCUR bit should not be set */
	if (duve_banner( DUVE_IIRELATION, 10, duve_cb)
        == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	duve_talk(DUVE_MODESENS, duve_cb, S_DU1613_BAD_CONCUR, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);

	if (duve_talk(DUVE_ASK, duve_cb, S_DU0307_CLEAR_CONCUR, 0) == DUVE_YES)
	{	
	    tid  = iirelation.reltid.db_tab_base;
	    tidx = iirelation.reltid.db_tab_index;
	    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
		      & ~TCB_CONCUR;
	    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
	    iirelation.relstat = relstat;
	    EXEC SQL update iirelation set relstat = :relstat where
		reltid = :tid and reltidx = :tidx;
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU0357_CLEAR_CONCUR, 0);

	}
    }

    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_11 - Run verifydb test # 11
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**11.	verify that core catalogs do have the special locking bit set.
**TEST:	if iirelation.relid is one of (iirelation, iirel_idx, iiattributes, 
**	iiindex or iidevices), verify that the iirelation.relstat.CONCUR 
**	bit = true.
**FIX:	set iirelation.relstat.CONCUR bit.
**MSGS:	S_DU1614, S_DU0308, S_DU0358
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_11  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    if ( (STncasecmp ("iirelation", iirelation.relid.db_tab_name,10)
         ==DU_IDENTICAL) ||
	 (STncasecmp ("iirel_idx", iirelation.relid.db_tab_name,9)
	 ==DU_IDENTICAL) ||
	 (STncasecmp ("iiattribute", iirelation.relid.db_tab_name,11)
	 ==DU_IDENTICAL) ||
	 (STncasecmp ("iidevices", iirelation.relid.db_tab_name,9)
	 ==DU_IDENTICAL) ||
	 (STncasecmp ("iiindex", iirelation.relid.db_tab_name,7)
	 ==DU_IDENTICAL) && (iirelation.reltid.db_tab_base <= DM_SCONCUR_MAX) )
    {
	    /* this is a dbms core catalog, so this bit should be set. */

	    if (iirelation.relstat & TCB_CONCUR)
	    {	/* the bit is set, so all is well */
	        return ( (DU_STATUS)DUVE_YES);
	    }

	/* if we get here, then the CONCUR bit should be set and is not */
	if (duve_banner( DUVE_IIRELATION, 11, duve_cb)
        == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	duve_talk(DUVE_MODESENS, duve_cb, S_DU1614_MISSING_CONCUR, 2,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab);

	if (duve_talk(DUVE_ASK, duve_cb, S_DU0308_SET_CONCUR, 0) == DUVE_YES)
	{	
	    tid  = iirelation.reltid.db_tab_base;
	    tidx = iirelation.reltid.db_tab_index;
	    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
		      | TCB_CONCUR;
	    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
	    iirelation.relstat = relstat;
	    EXEC SQL update iirelation set relstat = :relstat where
		reltid = :tid and reltidx = :tidx;
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU0358_SET_CONCUR, 0);

	}
    }
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_12 - Run verifydb test # 12
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**12.	verify that the view is defined if this entry indicates a view
**TEST:	if iirelation.relstat.VIEW bit = true then verify that there is
**	a nonzero count of iitree tuples that meet this condition:
**	iitree.treetabbase = iirelation.reltid and iitree.treetabidx =
**	iirelation.reltidx and iitree.treemode = VIEW.  If not, see if
**	a physical file exists for this table.
**FIX:	If the physical file exists, clear the iirelation.relstat.VIEW bit.
**	Otherwise drop this view definition from the system catalogs.
**NOTE:	If this was really a view, the view definition text can be retrieved
**	from iiqrytext and the view can be recreated.
**MSGS:	S_DU1615, S_DU1616, S_DU0309, S_DU030B, S_DU0359, S_DU035B
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    view may be dropped from the database
**	    contents of iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_12  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{

    /* test can not be implemented in its entirity cuz the DMF code to search
    ** for  a disk file does not exist 
    */

    exec sql begin declare section;
	i4	agg, mode;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    /* dont do this test unless this tuple describes a view */
    if ( iirelation.relstat & TCB_VIEW )
    {


	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;
	mode = DB_VIEW;

	exec sql repeated select any(treetabbase) into :agg from iitree where
	    treetabbase = :tid and treetabidx = :tidx and
	    treemode = :mode;

	/* if there are view tuples in iitree, all is well, so return */
	if ( agg )
	    return ( (DU_STATUS)DUVE_YES);
	
	/* if we get here, there is a problem */
	if (duve_banner( DUVE_IIRELATION, 12, duve_cb)
	== DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

	duve_talk(DUVE_MODESENS, duve_cb, S_DU1616_MISSING_VIEW_DEF, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	if ( duve_talk( DUVE_ASK, duve_cb, S_DU030B_DROP_VIEW, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	{
	    duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU035B_DROP_VIEW, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    return ( (DU_STATUS)DUVE_NO);
	}

    }	
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_13	- Run verifydb test # 13
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**13.	verify that the view is described in iidbdepends if this entry
**	indicates a view.
**TEST:	if iirelation.relstat.VIEW bit = true then verify that there is
**	a nonzero count of iidbdepends tuples that meet this condition:
**	iidbdepends.deid1 = iirelation.reltid and iidbdepends.deid2
**	= iirelation.reltidx and iidbdepends.type = view.
**FIX:	delete this tuple from iirelation and delete the tree tuple(s)
**	from iitree.
**NOTE:	The view definition text can be retrieved from iiqrytext and the view 
**	can be recreated.
**MSGS:	S_DU1617, S_DU030B, S_DU035B
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    view may be dropped from the database
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	29-mar-90 (teg)
**	    add logic to handle distributed constant views for bug 20970
**      22-jun-90 (teresa)
**          ii_ddb_finddbs -> iidd_ddb_finddbs
**	08-sep-92 (andre)
**	    before complaining about absence of description(s) of object(s) on
**	    which a view depends, check whether there are IIPRIV tuples
**	    describing privilegss on which a view depends.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	9-Sep-2004 (schak24)
**	    Users can define constant views too!  Downgrade this test
**	    to a warning, and never attempt to drop the view.
[@history_template@]...
*/

static DU_STATUS
test_13  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	agg, mode;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* dont do this test unless this tuple describes a view */
    if ( iirelation.relstat & TCB_VIEW )
    {
	mode = DB_VIEW;

	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	exec sql repeated select any(inid1) into :agg from iidbdepends where
	    deid1 = :tid and deid2 = :tidx and dtype = :mode;

	/* if there are view tuples in iidbdepends, all is well, so return */
	if ( agg )
	    return ( (DU_STATUS)DUVE_YES);

	/*
	** it is possible that the view depends ONLY on tables/views owned by
	** other user(s), so check in IIPRIV
	*/
	exec sql repeated select any(d_obj_id) into :agg from iipriv where
	    d_obj_id = :tid and d_obj_idx = :tidx and d_obj_type = :mode;

	/*
	** if there are tuples in IIPRIV describing privileges on which a view
	** depends, all is well, so return
	*/
	if ( agg )
	    return ( (DU_STATUS)DUVE_YES);
	
	/* if we get here, there is a potential problem  --
	** there is one view that will not be defined in iidbdepends:
	**	'iidbconstants' does not have an independent table, as it
	**			is defined from constants.  So are star views
	**			'iidd_ddb_finddbs' and 'iidd_dbconstants'
	*/

	 if (STncasecmp ("iidbconstants",iirelation.relid.db_tab_name,9)
	    ==DU_IDENTICAL)
	 {
	    return ( (DU_STATUS)DUVE_YES);
	 }

	 if (STncasecmp ("iidd_ddb_finddbs",iirelation.relid.db_tab_name,9)
	    ==DU_IDENTICAL)
	 {
	    return ( (DU_STATUS)DUVE_YES);
	 }

	 if (STncasecmp ("iidd_dbconstants",iirelation.relid.db_tab_name,9)
	    ==DU_IDENTICAL)
	 {
	    return ( (DU_STATUS)DUVE_YES);
	 }

	if (duve_banner( DUVE_IIRELATION, 13, duve_cb)
	== DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

	duve_talk(DUVE_MODESENS, duve_cb, S_DU1617_NO_DBDEPENDS, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	/* No automatic correction, as we don't bother parsing the view
	** text to see if perhaps it really is a constant view.
	*/
    }	
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_14	- Run verifydb test # 14
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**14.	verify that the view's definition text exists if this entry is a view.
**TEST:	if iirelation.relstat.VIEW bit = true then verify that there is
**	a nonzero count of iiqrytext tuples that meet this condition:
**	iiqrytext.txtid1 = iirelation.relqid1 and
**	iiqrytext.txtid2 = iirelation.relqid2 and iiqrytext.MODE = VIEW).
**FIX:	ISSUE WARNING THAT QUERRY TEXT IS MISSING, BUT DO NOT DELETE THIS
**	VIEW FROM THE DATA BASE.
**MSGS:	S_DU1618.
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_14  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{

    exec sql begin declare section;
	i4	agg, mode;
	i4	qid1,qid2;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* dont do this test unless this tuple describes a view */
    if ( iirelation.relstat & TCB_VIEW )
    {

	qid1 = iirelation.relqid1;
	qid2 = iirelation.relqid2;
	mode = DB_VIEW;

	exec sql repeated select any(txtid1) into :agg from iiqrytext where
	    txtid1 = :qid1 and txtid2 = :qid2 and mode = :mode;

	/* if there are view tuples in iiqrytext, all is well, so return */
	if ( agg )
	    return ( (DU_STATUS)DUVE_YES);
	
	/* if we get here, there is a problem */
	if (duve_banner( DUVE_IIRELATION, 14, duve_cb)
	== DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

	duve_talk(DUVE_MODESENS, duve_cb, S_DU1618_NO_QUERRYTEXT, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);

    }
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_16 - Run verifydb test #6, #7, # 16 and # 17
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**16.	verify that the base table exists if this is an index table
**TEST:	if iirelation.relstat.INDEX = true, then find the tuple in
**	iirelation where reltid = index_tables's.reltid and reltidx = 0.
**FIX:	If the base table doesn't exist, drop this index table
**MSGS:	S_DU161A, S_DU0302, S_DU0352
**
**17.	verify that the indexed bit is set in the base table
**TEST:	verify that iirelation.relstat.IDXD bit is set in the tuple
**	retrieved in step 16.
**FIX:	set the iirelation.relstat.IDXD bit in that tuple.
**MSGS:	S_DU161B, S_DU030D, S_DU035D
**
**6.    Verify that secondary index are marked as such.
**TEST: If iirelation.reltidx is non-zero, verify that iirelation.relstat.INDEX 
**	bit is set.  If iirelation.reltidx is zero, verify that the
**	iirelation.relstat.INDEX bit is not set.
**FIX:  Set or clear the iirelation.relstat.INDEX bit as appropriate.
**
**7.    Verify the the secondary index is known to the system.
**TEST: If this is a secondary index (iirelation.reltidx != 0), then verify that
**	there is an entry in iiindexes where: iiindex.baseid = iirelation.reltid
**	and iiindex.indexid = iirelation.reltidx.
**FIX:    Drop the iirelation tuple from the iirelation catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iiindex
**	    secondary index table may be dropped from database
**	    contents of iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	25-oct-90 (teresa)
**	    added two new tests:
**		16a: assures that secondary index a non-zero reltidx.  If not,
**		     it clears relstat.tcb_index.
**		17a: assures that secondary index is in iiindex.  If not, it
**		     marks the iirelation tuple for deletion.
**	02-apr-92 (teresa)
**	    do test 16a after test 16, not before.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

/* tests 16 and 17 in same routine for efficiency */
static DU_STATUS
test_16  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{

    DB_TAB_ID	index_id;
    i4     base;
    exec sql begin declare section;
	i4	agg;
	u_i4	tid,tidx;
	i4	relstat;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* dont do this test unless this tuple describes an index table */
    if ( iirelation.relstat & TCB_INDEX )
    {

	index_id.db_tab_base = iirelation.reltid.db_tab_base;
	index_id.db_tab_index = 0;
	if ( (base = duve_findreltid (&index_id, duve_cb)) == DU_INVALID)
	{
	    /* opps.  we have an index with no corresponding base. */
	    if (duve_banner( DUVE_IIRELATION, 16, duve_cb)
            == DUVE_BAD)
	        return ( (DU_STATUS)DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU161A_NO_BASE_FOR_INDEX, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	    {
		idxchk( duve_cb, iirelation.reltid.db_tab_base,
		        iirelation.reltid.db_tab_index);
	        duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
	        duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	        return ( (DU_STATUS)DUVE_NO);
	    }
	}	/* endif base table missing */

	/* test 6:
	** verify that reltidx is nonzero if this is marked as an index.
	** otherwise, clear TDB_INDEX bit from relstat and stop checking
	** this tuple as a secondary index. However, allow the verifydb
	** checking of this tuple to continue with other tests.
	*/
	if (iirelation.reltid.db_tab_index == 0)
	{
	    if (duve_banner( DUVE_IIRELATION, 6, duve_cb)
            == DUVE_BAD)
	        return ( (DU_STATUS)DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1671_NOT_INDEX, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU033D_CLEAR_TCBINDEX, 0)
	    == DUVE_YES)
	    {
		relstat = duve_cb->duverel->rel[base].du_stat & (~TCB_INDEX);
		duve_cb->duverel->rel[base].du_stat = relstat;
		iirelation.relstat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
		     reltid = :tid and reltidx = :tidx;
	        duve_talk( DUVE_MODESENS, duve_cb, S_DU038D_CLEAR_TCBINDEX, 0);
	    }
	    return ( (DU_STATUS)DUVE_YES);  /* its not a secondary index, so
					    ** skip remainding checks */
	}

	/* test 7:
	** Verify that the secondary index is described in catalog 
	** iiindex.  If not, drop this tuple. 
	*/
	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;
	exec sql repeated select any(baseid) into :agg from iiindex where
	    baseid = :tid and indexid=:tidx;
	if (agg==0)
	{
	    /* we have a problem because the iiindex tuple is missing.
	    ** we could attempt to reconstruct the iiindex tuple from
	    ** iiattribute information, but we have not yet verified that
	    ** the iiattribute info is valid.  So, mark this secondary
	    ** index for deletion if it is not in iiindex.  This may
	    ** leave iiattribute orphan tuple descriptors, but these
	    ** should be cleaned up by other verifydb tests.
	    */
	    if (duve_banner( DUVE_IIRELATION, 7, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1672_MISSING_IIINDEX, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	    {
		idxchk( duve_cb, iirelation.reltid.db_tab_base,
			iirelation.reltid.db_tab_index);
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
		duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		return ( (DU_STATUS)DUVE_NO);
	    }
	}

	/* test 17 - if base table exists for index table, assure that
	** it indicates it has a secondary index */
	if (base != DU_INVALID)
	{
	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_IDXD) == 0)
	    {	/* base table doesn't indicate that it has a secondary index */
		if (duve_banner( DUVE_IIRELATION, 17, duve_cb)
		== DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU161B_NO_2NDARY, 8,
			  0,duve_cb->duverel->rel[base].du_tab,
			  0,duve_cb->duverel->rel[base].du_own,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		if ( duve_talk( DUVE_ASK, duve_cb, S_DU030D_SET_IDXD, 4,
				0,duve_cb->duverel->rel[base].du_tab,
				0,duve_cb->duverel->rel[base].du_own)
		== DUVE_YES)
		{
		    tid  = iirelation.reltid.db_tab_base;
		    tidx = 0;
		    relstat = duve_cb->duverel->rel[base].du_stat
				| TCB_IDXD;
		    duve_cb->duverel->rel[base].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			 reltid = :tid and reltidx = :tidx;

		    if (sqlca.sqlcode == DU_SQL_OK)
			duve_talk( DUVE_MODESENS, duve_cb, S_DU035D_SET_IDXD, 4,
				0,duve_cb->duverel->rel[base].du_own);
		}
	    }
	} /* endif there is a base table for this index */


    } /* endif this is a secondary index */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_18	- Run verifydb test # 18
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**18.	verify that there really are indices on this table if relstat
**	indicates that there are.
**TEST:	get the count of the number of tuples in iiindex that meet this
**	condition:  iiindex.baseid = iirelation.reltid and iiindex.indexid = 
**	iirelation.reltidx.  If that count = iirelation.relidxcount then
**	all is well.
**FIX:	If the count is non-zero but less than iirelation.relidxcount, set
**	iirelation.relidxcount = the count.  If the count is zero, then
**	zero the iirelation.relidxcount and clear the iirelation.relstat.IDXD
**	bit.
**MSGS:	S_DU161C. S_DU161D, S_DU030E, S_DU030F, S_DU035E, S_DU035F
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iiindex
**	    contents of iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	25-oct-90 (teresa)
**	    set relloccount to 0, not to NULL.  This fixes a coding bug where
**	    the constant NULL was intended to imply zero, but was interpreted
**	    by ESQL as "put a null into this attribute".  This caused a bug
**	    since relloccount is a non-nullable attribute.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/


static DU_STATUS
test_18  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	agg;
	u_i4	tid,tidx;
	i4	relstat;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* dont do this test unless this tuple describes a base for an index */
    if ( iirelation.relstat & TCB_IDXD )
    {

	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	exec sql repeated select count (baseid) into :agg from iiindex where
	    baseid = :tid;

	/* if there is the correct # of tuples in iiindex , all is well, 
	** so return 
	*/
	if ( ( agg == iirelation.relidxcount ) && (iirelation.relidxcount) )
	    return ( (DU_STATUS)DUVE_YES);
	
	/* if we get here, there is a problem */
	if (duve_banner( DUVE_IIRELATION, 18, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	
	if (agg)
	{   /* we have the wrong index count */
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU161C_WRONG_NUM_INDEXES, 8,
		  sizeof(iirelation.relidxcount), &iirelation.relidxcount,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		  sizeof(agg), &agg);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU030E_FIX_RELIDXCOUNT, 4,
		  sizeof(agg), &agg,
		  sizeof(iirelation.relidxcount), &iirelation.relidxcount)
	    == DUVE_YES)
	    {	
		exec sql update iirelation set relidxcount = :agg
		   where reltid = :tid and reltidx = :tidx;
		duve_talk( DUVE_MODESENS, duve_cb, S_DU035E_FIX_RELIDXCOUNT, 4,
		  sizeof(agg), &agg,
		  sizeof(iirelation.relidxcount), &iirelation.relidxcount);
	    }
	} /* end if the wrong # of indexes */
	else /* there are none */
	{
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU161D_NO_INDEXS, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU030F_CLEAR_RELIDXCOUNT, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	    {	
		relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat 
			  & ~TCB_IDXD;
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
	        iirelation.relstat = relstat;
		EXEC SQL update iirelation set relstat = :relstat,
		     relidxcount = 0 where reltid = :tid and reltidx = :tidx;
	        duve_talk( DUVE_MODESENS, duve_cb, S_DU035F_CLEAR_RELIDXCOUNT, 4,
		           0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    }
	} /* endif no indexes defined */
    } /* endif this table is indexed */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_19 - Run verifydb test # 19
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**19.	verify that there is an entry in iistatistics if relstat indicates
**	that there is
**TEST:	If the iirelation.relstat.OPTSTAT bit = true, then verify that 
**	there is atleast one iistatistics row that meets this condition:  
**	iistatistics.stabbase = iirelation.reltid and iistatistics.stabindex 
**	= iirelation.reltidx.
**FIX:	clear iirelation.relstat.OPTSTAT bit.
**MSGS:	S_DU161E, S_DU0310, S_DU0360
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iistatistics
**	    contents of iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_19  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	i4	agg;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* dont do this test unless this tuple indicate statistics exist */
    if ( iirelation.relstat & TCB_ZOPTSTAT )
    {

	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	exec sql repeated select any(stabbase) into :agg from iistatistics
	    where stabbase = :tid and stabindex = :tidx;

	/* if there are tuples in iistatistics, all is well, so return */
	if ( agg )
	    return ( (DU_STATUS)DUVE_YES);
	
	/* if we get here, there is a problem */
	if (duve_banner( DUVE_IIRELATION, 19, duve_cb)
	== DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

	duve_talk(DUVE_MODESENS, duve_cb, S_DU161E_NO_STATISTICS, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	if ( duve_talk( DUVE_ASK, duve_cb, S_DU0310_CLEAR_OPTSTAT, 0)
	    == DUVE_YES)
	{
	    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
		      & ~TCB_ZOPTSTAT;
	    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
	    iirelation.relstat = relstat;
	    EXEC SQL update iirelation set relstat = :relstat where
		reltid = :tid and reltidx = :tidx;
	    duve_talk(DUVE_MODESENS, duve_cb,S_DU0360_CLEAR_OPTSTAT, 0);
	}
    } /* endif */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_20 - Run verifydb test # 20
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**20.	verify that the number of INGRES locations is correctly defined in
**	iirelation for all tables that are not views or core catalogs.
**TEST:	If the iirelation.relstat.MULTIPLE_LOC bit = true, then verify that
**	relloccount is > 1.  Then get the count
**	of the number of rows in iidevices that meet this condition:
**	iidevices.devrelid = iirelation.reltid and iidevices.devrelidx =
**	iirelation.reltidx.  That count should = (iirelation.relloccount - 1).
**	If this is a gateway table (TCB_GATEWAY), skip this test.
**FIX:	Report that there is a descrepancy between the location count and the
**	actual # of locations defined in iidevices.
**	IF MODE = RUNINTERACTIVE THEN
**	   If count > 0 but less than expected THEN
**	       prompt user for permisttion to set iirelation.relloccount = count
**	   ELSEIF count = 0 THEN 
**	       prompt user for permission to set iirelloccount = 1 and to clear
**	       	iirelation.relstat.MULTIPLE_LOC.
**	   ENDIF
**	ELSE
**	   Print message requesting user rerun verifydb with runinteractive mode
**	   or manually fix inconsistency in catalogs.
**	ENDIF
**NOTE:	Verifydb does not automatically make any fix because it cannot determine
**	if iirelation.relloccount is wrong or if a tuple is missing from 
**	iidevices.  The config file also contains location information.  At some
**	future point, verifydb may get this info out of the config file.
**MSGS:	S_DU161F, S_DU0311, S_DU0361
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iidevices
**	    contents of iirelation tuples may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	20-Oct-1988 (teg)
**	    added support for gateway.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	28-jun-93 (teresa)
**	    spell relloccount correctly.
**    13-Mar-2000 (fanch01)
**        Change iidevice query in test_20() to count active devloc entries
**        correctly for bug number 100875.
[@history_template@]...
*/


static DU_STATUS
test_20  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	agg;
	u_i4	tid,tidx;
	i4	relstat;

    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if ( (iirelation.relstat & TCB_MULTIPLE_LOC) && 
	((iirelation.relstat & TCB_GATEWAY) ==0) )
    {
	tid  = iirelation.reltid.db_tab_base;
	tidx = iirelation.reltid.db_tab_index;

	exec sql repeated select count (devrelid) into :agg from iidevices where
		devrelid = :tid and devrelidx = :tidx and devloc != '';

	/* if there is the correct # of tuples in iidevices, all is well, 
	** so return 
	*/
	if ( agg == (iirelation.relloccount-1) )
	    return ( (DU_STATUS)DUVE_YES);

	agg++;	
	/* if we get here, there is a problem */
	if (duve_banner( DUVE_IIRELATION, 20, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	
	duve_talk(DUVE_MODESENS, duve_cb, S_DU161F_WRONG_NUM_LOCS, 8,
		  sizeof(iirelation.relloccount), &iirelation.relloccount,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		  sizeof(agg), &agg);
        if ( duve_talk( DUVE_IO, duve_cb, S_DU0311_RESET_RELLOCOUNT, 4,
		  sizeof(iirelation.relloccount), &iirelation.relloccount,
		  sizeof(agg), &agg)
	== DUVE_YES)
	{
	    exec sql update iirelation set relloccount = :agg
		   where reltid = :tid and reltidx = :tidx;
	    if (agg<=1)
	    {   
		relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat 
			  & ~TCB_MULTIPLE_LOC;
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
	        iirelation.relstat = relstat;
		EXEC SQL update iirelation set relstat = :relstat,
		     relidxcount = 0 where reltid = :tid and reltidx = :tidx;
	        duve_talk( DUVE_MODESENS, duve_cb, S_DU030F_CLEAR_RELIDXCOUNT,4,
		           0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
			   0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    }
	}
    } 
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_21 - Run verifydb test # 21
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**21.	verify that the table's location is valid
**TEST:	verify that the location containted in this column matches one of the
**	locations known to the installation.
**FIX:	report error.  DBA should fix this offline.
**NOTE:	This test is related to test #7.
**MSGS:	S_DU1620
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	20-Oct-1988 (teg)
**	    enhanced location test to detect a location name with a valid
**	    format that is not known to this installation, and added logic
**	    to skip this test if we are a gateway table.
**      15-nov-90 (teresa)
**          fixed array out-of-bounds reference.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_21  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    char	loc[sizeof(iirelation.relloc)+1];

    if ( (iirelation.relstat & TCB_GATEWAY) != 0)
	return ( (DU_STATUS) DUVE_YES);

    if ( duve_locfind(&iirelation.relloc, duve_cb) == DU_INVALID)
    { 
	if (duve_banner( DUVE_IIRELATION, 21, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	STncpy( loc, iirelation.relloc.db_loc_name, sizeof(iirelation.relloc));
	loc[sizeof(iirelation.relloc)] = '\0';
	STtrmwhite (loc);

	duve_talk( DUVE_MODESENS, duve_cb, S_DU1620_INVALID_LOC, 6,
		  0, loc, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);

    }
    return ( (DU_STATUS)DUVE_YES);

}

/*{
** Name: test_22 - Run verifydb test # 22, 23, 24 and 25
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**22.	verify that the data page fill factor is valid for the heap storage type
**TEST:	If iirelation.relspec = HEAP, verify iirelation.reldfill is valid (>0)
**FIX:	set iirelation.reldfill to the INGRES default value for heap.
**MSGS:	S_DU1621, S_DU0312, S_DU0352
**
**23.	Verify that the data page fill factor is valid for the hash 
**	storage type.
**TEST:	If iirelation.relspec = HASH, verify iirelation.reldfill, 
**	is valid (nonzero)
**FIX:	Set to the INGRES default value for hash.
**MSGS:	S_DU1621, S_DU0312, S_DU0352
**
**24.	Verify that the data page fill factor is valid for the ISAM storage 
**	type.
**TEST:	Verify iirelation.reldfill is valid (>0).
**FIX:	If zero, set to the INGRES default value for ISAM.
**MSGS:	S_DU1621, S_DU0312, S_DU0352
**
**25.	Verify that the data, index and leaf page fill factors are valid for
**	the BTREE storage type.
**TEST:	Verify iirelation.reldfill, iirelation.rellfill and 
**	iirelation.relifill are valid (>0).
**FIX:	If either of these are zero, set them to the INGRES default value
**	for BTREE.
**MSGS:	S_DU1621, S_DU1622, S_DU1623, S_DU0312, S_DU0313, S_DU0314,
**	S_DU0362, S_DU0363, S_DU0364
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    contents of iirelation tuple may be modified
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_22  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    /* tests 22 - 25 */

    exec sql begin declare section;
	u_i4	tid,tidx;
	i2	fill;
    exec sql end declare section;   
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if ( (iirelation.reldfill <= 0) || (iirelation.reldfill > 100 ) )
    {
	/* invalid datafill factor */
	switch (iirelation.relspec)
	{
	    case TCB_HEAP:
		if (duve_banner( DUVE_IIRELATION, 22, duve_cb) 	== DUVE_BAD)
		    return ( (DU_STATUS)DUVE_BAD);
		if (iirelation.relstat & TCB_COMPRESSED)
		    fill = DM_F_CHEAP;
		else
		    fill = DM_F_HEAP;
		break;
	    case TCB_HASH:
		if (duve_banner( DUVE_IIRELATION, 23, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS)DUVE_BAD);
		if (iirelation.relstat & TCB_COMPRESSED)
		    fill = DM_F_CHASH;
		else
		    fill = DM_F_HASH;
		break;
	    case TCB_ISAM:
		if (duve_banner( DUVE_IIRELATION, 24, duve_cb) 	== DUVE_BAD)
		    return ( (DU_STATUS)DUVE_BAD);
		if (iirelation.relstat & TCB_COMPRESSED)
		    fill = DM_F_CISAM;
		else
		    fill = DM_F_ISAM;
		break;
	    case TCB_BTREE:
		if (duve_banner( DUVE_IIRELATION, 25, duve_cb) 	== DUVE_BAD)
		    return ( (DU_STATUS)DUVE_BAD);
		if (iirelation.relstat & TCB_COMPRESSED)
		    fill = DM_F_CBTREE;
		else
		    fill = DM_F_BTREE;
		break;
	} /* end case */
	duve_talk (DUVE_MODESENS, duve_cb, S_DU1621_INVALID_FILLFACTOR, 6,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		   sizeof(iirelation.reldfill), &iirelation.reldfill);
	if (duve_talk(DUVE_ASK, duve_cb, S_DU0312_FIX_RELDFILL, 0) == DUVE_YES)
	{
	    tid  = iirelation.reltid.db_tab_base;
	    tidx = iirelation.reltid.db_tab_index;
	    EXEC SQL update iirelation set reldfill = :fill
		     where reltid = :tid and reltidx = :tidx;
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU0362_FIX_RELDFILL, 0);
	}
	if (iirelation.relspec != TCB_BTREE)
	    return ( (DU_STATUS)DUVE_YES);
    }

    /* test 25 requires that leaf and index fill factors be checked for
    ** btrees
    */
	if ( (iirelation.relspec == TCB_BTREE) &&
	     (( iirelation.relifill <= 0) || ( iirelation.rellfill <= 0)||
	      ( iirelation.relifill > 100) || ( iirelation.rellfill > 100))  )
	{
	    /* need to output test header and initialize table id for join*/
	    if (duve_banner( DUVE_IIRELATION, 25, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS)DUVE_BAD);
	    if (iirelation.reldfill)
	    {
	        tid  = iirelation.reltid.db_tab_base;
	        tidx = iirelation.reltid.db_tab_index;
	    }

	    if ( (iirelation.rellfill == 0) || (iirelation.rellfill > 100) )
	    {
		/* leaf fill factor is bad */
		duve_talk (DUVE_MODESENS, duve_cb, S_DU1622_INVALID_LEAF_FILLFTR,
		   6, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		   sizeof(iirelation.rellfill), &iirelation.rellfill);
		if (duve_talk(DUVE_ASK, duve_cb, S_DU0313_FIX_RELLFILL, 0) 
		== DUVE_YES)
		{
		    fill = DM_FL_BTREE;
		    EXEC SQL update iirelation set rellfill = :fill
		         where reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0363_FIX_RELLFILL, 0);
		}
	    }

	    if ( (iirelation.relifill == 0) || (iirelation.relifill > 100) )
	    {
		/* index fill factor is bad */
		duve_talk (DUVE_MODESENS, duve_cb,S_DU1623_INVALID_INDEX_FILLFTR,
		   6, 0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		   sizeof(iirelation.relifill), &iirelation.relifill);
		if (duve_talk(DUVE_ASK, duve_cb, S_DU0314_FIX_RELIFILL, 0) 
		== DUVE_YES)
		{
		    fill = DM_FI_BTREE;
		    EXEC SQL update iirelation set relifill = :fill
		         where reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0364_FIX_RELIFILL,
				0);
		}
	    }

	}
	return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_26 - Run verifydb test #26
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**26.	Verify that there is an entry in iirel_idx for this iirelation tuple.
**TEST:	find a row in iirel_idx that meets the following condition:
**	iirel_idx.relid = iirelation.relid and iirel_idx.du_own =
**	iirelation.du_own.
**FIX:	Create an entry for iirel_idx and insert it into the table.
**MSGS:	S_DU1624, S_DU0315, S_DU0365
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    new tuples may be inserted into iirel_idx
**
** History:
**      15-aug-88 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
[@history_template@]...
*/

static DU_STATUS
test_26  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{

    exec sql begin declare section;
	u_i4		tid,tidx;
	i4		agg;
	char		trelid[DB_MAXNAME+1];
	char		trelowner[DB_MAXNAME+1];
    exec sql end declare section;   

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    STcopy ( duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab, trelid);
    STcopy ( duve_cb->duverel->rel[duve_cb->duve_cnt].du_own, trelowner);

    exec sql repeated select any(relid) into :agg from iirel_idx where
	relid = :trelid and relowner = :trelowner;

    if (agg==0)
    {  /* The entry for iirelation is missing from iirel_idx */
	
	if (duve_banner( DUVE_IIRELATION, 26, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);
	duve_talk( DUVE_MODESENS, duve_cb, S_DU1624_MISSING_IIRELIDX, 4,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	if ( duve_talk( DUVE_ASK, duve_cb, S_DU0315_CREATE_IIRELIDX, 0)
	   == DUVE_YES )
	{
	    tid  = iirelation.reltid.db_tab_base;
	    tidx = iirelation.reltid.db_tab_index;
	    /* Note although tid is i8, tidp is still i4 */
	    exec sql select int4(tid) into :agg from iirelation where
		reltid = :tid and reltidx = :tidx;
	    exec sql insert into iirel_idx (relid, relowner, tidp) values
		(:trelid, :trelowner, :agg);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU0365_CREATE_IIRELIDX, 0);
	}

    }
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_27	- Run verifydb test #27
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**8.	verify that there are really rules specified for this table
**	if iirelation says that there are
**TEST:	if iirelation.relstat.RULE bit set, then verify a nonzero count
**	of iirule tuples that meet this condition: iirule.rule_tabbase =
**	iirelation.reltid and iirule.rule_tabidx = iirelation.reltidx.
**FIX:	clear iirelation.relstat.PRTUPS bit.
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iiprotect
**	    contents of iirelation tuples may be modified
**
** History:
**	27-may-93 (teresa)
**	    Initial creation.
[@history_template@]...
*/

static DU_STATUS
test_27  ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	i4	agg;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    /* test 8 - verify rules really defined on table if iirelation
		thinks they are */
    if (iirelation.relstat & TCB_RULE)
    {
	exec sql repeated select any(rule_name) into :agg from iirule
	    where rule_tabbase = :tid and rule_tabidx = :tidx;
	if (agg==0)
	{
            if (duve_banner( DUVE_IIRELATION, 27, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    /* output error message that rules defined on table are lost*/
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU167D_NO_RULES, 4,
			  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		          0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	    if (duve_talk(DUVE_ASK, duve_cb, S_DU0342_CLEAR_RULE, 0)
	        == DUVE_YES)
	    {	
		    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			      & ~TCB_RULE;
		    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0392_CLEAR_RULE,0);
	    } /* endif */

	}/* endif */

    }/* endif */
    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: test_28 - Run verifydb test #28
**
** Description:
**      This rutine runs the following test defined in the verifydb product
**      spcification:
**
**28:   Verify that there is an entry in iischema for this iirelation tuple.
**TEST: find a row in iirelation that meets the folloing condition:
**      iirelation.relowner = iischema.schema_name
**      Select count(schema_id) from iirelation r, iischema s where
**      r.relowner = s.schema_name
**      If count is zero, then there is a problem.
**FIX:  Inform the user that he has an orphaned relation.
**MSGS: S_DU168D
**
** Inputs:
**      duve_cb                         verifydb control block
**      iirelation                      structure containing all attributes in
**                                      iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**      Returns
**          DUVE_YES,
**          DUVE_BAD
**      Exceptions:
**         none
**
** Side Effects:
**      tuples read from iischema
**
** Histry:
**      18-nov-92 (anitap)
**          initial creation.
**	20-dec-93 (anitap)
**	    Added REPEATED and ANY keywords to improve performance. 
**	    Also, new entry du_schadd in the iirelation cache which will set to
**	    DUVE_ADD for adding schemas for orphaned objects.
[@history_template@]...
*/
static DU_STATUS
test_28 ( duve_cb, iirelation )
DUVE_CB        *duve_cb;
DMP_RELATION   iirelation;
{

	DU_STATUS	base;

    exec sql begin declare section;
        i4         agg;
	char		trelowner[DB_MAXNAME + 1];
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    STcopy ( duve_cb->duverel->rel[duve_cb->duve_cnt].du_own, trelowner);

    exec sql repeated select any (schema_id) into :agg from iischema where
    	schema_name = :trelowner; 

    if (agg == 0)
    {  
	/* The entry for schema for the iirelation tuple is missing from
                iischema */

        if (duve_banner (DUVE_IIRELATION, 28, duve_cb)
        	== DUVE_BAD)
            return ( (DU_STATUS)DUVE_BAD);
        duve_talk( DUVE_MODESENS, duve_cb, S_DU168D_SCHEMA_MISSING, 2,
                   0, duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
	if ( duve_talk(DUVE_IO, duve_cb, S_DU0245_CREATE_SCHEMA, 0)
                        == DUVE_YES)
        {
           /* We want to check if there any existing constraints
           ** on tables owned by this user. If there are, we want
           ** the schema to have the schema_id of the constraint.
           */

           base = check_cons(duve_cb);

           if (base == DUVE_NO)
           {
              /* schema has not been created */

	      duve_cb->duverel->rel[duve_cb->duve_cnt].du_schadd = DUVE_ADD;
	   }
	
	   duve_talk(DUVE_MODESENS, duve_cb, S_DU0445_CREATE_SCHEMA, 0);
	}

     } /* endif */

     return ( (DU_STATUS)DUVE_YES);
}

/*
** Name: test_29 - run verifydb test #29
**
** Description:
**	This routine runs the following test as defined in the verifydb 
**	dbms_catalog operation specification:
**
** 29:	Verify that there is an entry in iigw06_relation for this iirelation
**	tuple if it is indicated as a security audit gateway table.
** TEST:Check that the row describes a security audit gateway table
**	(relgwid=6 and relstat TCB_GATEWAY bit is set), then find a row
**	in iigw06_relation that matches. If not then something is wrong.
** FIX:	Report error, DBA should fix this offline.
**
** Inputs:
**	duve_cb  	verifydb control block
**	iirelation	structure containing all attributes in iirelation tuple
**
** Outputs:
**	duve_cb		verifydb control block
**	Returns
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none.
**
** Side Effects:
** 	none.
**
** History:
**	5-aug-93 (stephenb)
**	    initial creation.
**	05-apr-1996 (canor01)
**	    must pass iirelation.relid.db_tab_name instead of just
**	    iirelation.relid, since NT compiler pushes structure on
**	    the stack, not just a pointer.
[@history_template@]
*/
static DU_STATUS
test_29 (DUVE_CB *duve_cb, DMP_RELATION iirelation)
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4 cnt;
	i4 reltid;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    reltid = iirelation.reltid.db_tab_base;

    if (iirelation.relstat & TCB_GATEWAY && iirelation.relgwid == 6)
    /* security audit gateway table */
    {
	EXEC SQL repeated SELECT any(reltid) INTO :cnt FROM iigw06_relation
	    WHERE reltid = :reltid;
	if (cnt == 0) /* no tuple in iigw06_relation */
	{
	    if (duve_banner( DUVE_IIRELATION, 29, duve_cb)
	    == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU169D_MISSING_IIGW06REL, 2,
		0, iirelation.relid.db_tab_name);
	}
    }
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: test_30_and_31 - Run verifydb IIRELATION tests #30 and #31
**
** Description:
**      This routine runs the following tests defined in the verifydb product
**	specification: 
**
**30.	If a tuple describes an always grantable view, verify that IIPRIV does
**	not contain a descriptor of privileges on which this view depends.
**TEST:	Check IIPRIV fpr existence of tuples describing privileges on which this
**	view depends.
**FIX:	clear iirelation.relstat.VGRANT_OK bit.
**MSGS:	S_DU16F4, S_DU0259, S_DU0459
**
**31.	If a tuple describes a view that is not always grantable, verify that
**	IIPRIV contains a descriptor of privileges on which this view depends.
**TEST:	Check IIPRIV for existence of tuples describing privileges on which this
**	view depends.
**FIX:	Mark view for destruction.
**MSGS:	S_DU16F5, S_DU0302, S_DU0352
**
**
** Inputs:
**      duve_cb                         verifydb control block
**	iirelation			structure containing all attributes in
**					  iirelation tuple
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES,
**	    DUVE_NO,
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iipriv
**	    contents of iirelation tuples may be modified
**
** History:
**      24-jan-94 (andre)
**          written
[@history_template@]...
*/
static DU_STATUS
test_30_and_31(duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation;
{
    exec sql begin declare section;
	i4	relstat;
	i4	cnt, mode;
	u_i4	tid,tidx;
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    tid  = iirelation.reltid.db_tab_base;
    tidx = iirelation.reltid.db_tab_index;

    if (iirelation.relstat & TCB_VIEW)
    {
	mode = DB_VIEW;

	exec sql repeated select any(1) into :cnt from iipriv
	    where d_obj_id = :tid and d_obj_idx = :tidx and
		  d_priv_number = 0 and d_obj_type = :mode;

        /* 
        ** test 30 - verify that a view marked as "always grantable" does not
        ** 		 depend on any privileges
        */
        if (cnt != 0 && iirelation.relstat & TCB_VGRANT_OK)
	{
            if (duve_banner( DUVE_IIRELATION, 30, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);

	    /* 
	    ** the view is marked "always grantable", but IIPRIV contains a 
	    ** description of privileges on which it depends - guess it is not 
	    ** "always grantable"
	    */
	    duve_talk( DUVE_MODESENS, duve_cb, 
		S_DU16F4_VIEW_NOT_ALWAYS_GRNTBL, 4,
		0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab);
	    if (duve_talk(DUVE_ASK, duve_cb, S_DU0259_CLEAR_VGRANT_OK, 0)
	        == DUVE_YES)
	    {	
		    relstat = duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat
			      & ~TCB_VGRANT_OK;
		    duve_cb->duverel->rel[duve_cb->duve_cnt].du_stat = relstat;
		    iirelation.relstat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :tid and reltidx = :tidx;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU0459_CLEARED_VGRANT_OK,0);
	    } /* endif */
	}/* endif */

	if (cnt == 0 && ~iirelation.relstat & TCB_VGRANT_OK)
	{
            if (duve_banner( DUVE_IIRELATION, 31, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);

	    /* 
	    ** the view is marked not "always grantable", but IIPRIV does
	    ** contain a description of privileges on which it depends - ask 
	    ** user for permission to drop it
	    */
	    duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU16F5_MISSING_VIEW_INDEP_PRVS, 4,
		0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own,
		0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab);
	    if ( duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own)
	    == DUVE_YES)
	    {
		idxchk( duve_cb, iirelation.reltid.db_tab_base,
			iirelation.reltid.db_tab_index);
		duve_cb->duverel->rel[duve_cb->duve_cnt].du_tbldrop = DUVE_DROP;
		duve_talk( DUVE_MODESENS, duve_cb, S_DU0352_DROP_TABLE, 4,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_tab,
		  0,duve_cb->duverel->rel[duve_cb->duve_cnt].du_own);
		return ( (DU_STATUS)DUVE_NO);
	    }
	}

    }/* endif */
    return ( (DU_STATUS)DUVE_YES);
}

/*
** Name: test_95 - run verifydb test #95
**
** Description:
**	This routine runs the following test:
**
** 95:	Verify that if this relation has TCB2_PHYSLOCK_CONCUR set that its
**      structure is hash
** TEST:Check the relstat2 field of the relation - if it includes 
**      TCB2_PHYSLOCK_CONCUR then relspec should be TCB_HASH
** FIX:	Modify the table to hash using its modify command 
**
** Inputs:
**	duve_cb  	verifydb control block
**	iirelation	structure containing all attributes in iirelation tuple
**
** Outputs:
**	duve_cb		verifydb control block
**	Returns
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none.
**
** Side Effects:
** 	none.
**
** History:
**	10-Feb-2010 (maspa05) bug 122651
**	    initial creation.
*/
static DU_STATUS
test_95 ( duve_cb, iirelation )
DUVE_CB		*duve_cb;
DMP_RELATION	iirelation ;
{
    char	relid[DB_MAXNAME+1];
 
    i2 l;

    STlcopy(iirelation.relid.db_tab_name, relid,DB_MAXNAME);
    l=STtrmwhite(relid);

    if ( (iirelation.relstat2 & TCB2_PHYSLOCK_CONCUR) &&  
           iirelation.relspec != TCB_HASH)
    {
       if (duve_banner( DUVE_IIRELATION, 95, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
       duve_talk( DUVE_MODESENS, duve_cb, S_DU1761_PHYSL_TBL_NOT_HASH, 2,
		0,relid);
       if (duve_talk(DUVE_ASK, duve_cb, S_DU025B_MAKE_PHYSL_TBL_HASH, 2,
                     0,relid)
	        == DUVE_YES)
       {	
	    /* modify the table to its default structure */
	    duc_modify_catalog(relid);

	    duve_talk(DUVE_MODESENS, duve_cb, S_DU045B_PHYSL_TBL_MADE_HASH,2,
		      0,relid);
       }

    }


    return ( (DU_STATUS)DUVE_YES);
}

/*{
** Name: create_schema - Create schema for the orphaned object
**
** Description:
**	This routine checks if the schema is already present, and if not,
**	creates the specified schema.
**
**	NOTE: May not be possible to create schema this way in post 6.5 when
**	      we add support for an user to have more than one schema.  It will
**	      be possible for a constraint on a table to belong to a schhema
**	      different from that table.	
** Inputs:
**	duve_cb			verifydb control block;
**
** Outputs:
**	duve_cb			verifydb control block
**	Returns
**	   none
**	Exceptions:
**	   none
**
** Side Effects:
**	tuples read from iischema
**
** History:
**	13-dec-93 (anitap)
**	    Created for CREATE SCHEMA project.
[@history_template@]...
*/	
static void 
create_schema(duve_cb)
DUVE_CB		*duve_cb;
{

	exec sql begin declare section;
	    i4	   cnt;
	    char	   sch_name[DB_MAXNAME + 1];
	    char           *statement_buffer, buf[CMDBUFSIZE];
	exec sql end declare section;

	u_i4 		   len;

	EXEC SQL WHENEVER SQLERROR call Clean_Up;

	STcopy ( duve_cb->duve_schema, sch_name);

	len = STtrmwhite(sch_name);

        /* check if this schema has already been created */
        exec sql repeated select any(schema_id) into :cnt
                from iischema
                where schema_name = :sch_name;

        if ( cnt == 0)
        {
           /* create the schema */
           exec sql commit;
           statement_buffer = STpolycat(4, "set session authorization ", "\"",
                       sch_name, "\"", buf);
           exec sql execute immediate :statement_buffer;
           statement_buffer = STpolycat(2, "create schema authorization ",
                        sch_name, buf);
           exec sql execute immediate :statement_buffer;
	}
}

/*{
** Name: check_cons - check if constraints present in the schema
**
** Description:
**      This routine checks if any constraints are present in the schema. If
**      there are, then verifydb will proceed to insert the schema into iischema
**      catalog with the schema_id obtained from iiintegrity.
**
**      If there are no constraints, verifydb will proceed to cache information
**      about the schema tuple and create the schema at the end.
**
** Inputs:
**      duve_cb                 verifydb control block
**
** Outputs:
**      duve_cb                 verifydb control block
**      Returns:
**              DUVE_YES,
**              DUVE_NO,
**              DUVE_BAD
**      Exceptions:
**              none.
**
** Side Effects:
**      new tuple may be inserted into iischema catalog.
**
** History:
**      20-dec-93 (anitap)
**          Created for CREATE SCHEMA project.
[@history_template@]...
*/

static DU_STATUS
check_cons(duve_cb)
DUVE_CB         *duve_cb;
{

        exec sql begin declare section;
                u_i4    sch_id1, sch_id2;
		u_i4	id1, id2;
                i4 cnt;
		char	schname[DB_MAXNAME + 1];
        exec sql end declare section;

        EXEC SQL WHENEVER SQLERROR call Clean_Up;

        STcopy (duve_cb->duverel->rel[duve_cb->duve_cnt].du_own, schname);

	id1 = duve_cb->duverel->rel[duve_cb->duve_cnt].du_id.db_tab_base;
	id2 = duve_cb->duverel->rel[duve_cb->duve_cnt].du_id.db_tab_index;

        /*
        ** Query iirelation and iiintegrity to find out
        ** if there are any constraints for the schema
        */
        EXEC SQL select any(consschema_id1), consschema_id1,
                        consschema_id2 into :cnt, :sch_id1, :sch_id2
                from iiintegrity
                where inttabbase = :id1
                and inttabidx = :id2
		group by consschema_id1, consschema_id2;

        if (cnt != 0)
        {
           EXEC SQL insert into iischema(schema_name,
                schema_owner, schema_id, schema_idx)
                values(:schname, :schname, :sch_id1, :sch_id2);

           return ((DU_STATUS) DUVE_YES);
        }
        else
           return ((DU_STATUS) DUVE_NO);
}

/*{
** Name: duve_d_cascade - perform cascading destruction of objects
**
** Description:
**	This function may be called at the end of DBMS_CATALOG operation or as a
**	part of DROPTBL operation.  
**
**	In the former case, some of the entries in the duverel cache 
**	may be marked for destruction + the "fix it" list may
**	contain descriptions of objects (other than table/view/indices) which
**	need to be dropped or, in case of user-defined dbprocs, marked dormant.
**
**	In the latter, caller will provide us with id of a table to drop.
**
**	We will proceed to store descriptions of objects to be destroyed into
**	the temp table to which we will then add descriptions of objects
**	dependent on those which we were asked to destroy.  Once the table has
**	been populated, we will proceed to delete tuples from various catalogs 
**	describing objects to be dropped.
**
**	Some additional processing needs to be performed for dbprocs and indices
**	which we process:
**	- user defined dbprocs must be marked dormant while system-defined ones
**	  will be destroyed
**	- relidxcnt for tables indices on which get dropped may need to be 
**	  recomputed + if it drops to 0 TCB_IDXD bit needs to be reset in 
**	  relstat
**
** Inputs:
**      duve_cb                 verifydb control block
**	base			if performing DROPTBL operation, this must 
**				contain base id of a table whose name was 
**				specified by the user; otherwise its value is 
**				disregarded
**	idx			if performing DROPTBL operation, this must 
**				contain index id of a table whose name was 
**				specified by the user; otherwise its value is 
**				disregarded
**
** Outputs:
**      duve_cb                 verifydb control block
**
** Returns:
**	none
**
** Exceptions:
**      none.
**
** Side Effects:
**      new tuples may be inserted into iischema catalog.
**	tuples describing DBMS objects may be deleted from various system 
**	catalogs
**
** History:
**      26-jan-94 (andre)
**          written
**	08-jan-94 (anitap)
**	    Corrected typos. Corrected int_qryid1 and int_qryid2 to intqryid1 
**	    & intqryid2.
**	27-Jul-1998 (nciph02)
**	    Bug 88961. verifydb is querying 'iirelation' using field name
**	    'relidxcnt' instead of 'relidxcount'.
[@history_template@]...
*/
static VOID
duve_d_cascade(
	     DUVE_CB	*duve_cb,
	     u_i4	base,
	     u_i4	idx)
{
    bool		objects_dropped;
    i4             ctr;
    i4		cnt_this_run;

    EXEC SQL BEGIN DECLARE SECTION;
	i4		cnt;
	i4		relstat;
	u_i4            tid;
	u_i4		tidx;
	u_i4		qtxtid1;
	u_i4		qtxtid2;
	i4		type;
	i4		sec_id;
	i4		idx_type;
	i4		view_type;
	i4		tbl_type;
	i4		dbp_type;
	i4		rule_type;
	i4		cons_type;
	i4		intg_type;
	i4		event_type;
	i4		perm_type;
	i4		dbp_mask;
	i4		cleanup_flag;
    EXEC SQL END DECLARE SECTION;

/*
** these bits may be set in session.cleanup_tbl.flags
*/
#define		DECREMENT_INDEX_COUNT	0x01
#define 	DROP_THIS_DBPROC	0x02

    objects_dropped = FALSE;

    /* create temp tables which will be used in the clean up effort */

    EXEC SQL WHENEVER SQLERROR CALL Clean_Up;

    exec sql declare global temporary table session.cleanup_tbl
	(obj_id		integer,
	 obj_idx	integer,
	 obj_type	integer,
	 secondary_id	integer,
	 flags  	integer)
	 on commit preserve rows with norecovery;

    /*
    ** if processing TBLDROP operation, append description of the table 
    ** specified by the user to our temp table;
    ** otherwise, we will populate the temp table with ids of 
    ** tables/views/indices marked for destruction + objects described in the 
    ** "fix it" list
    */

    if (duve_cb->duve_operation == DUVE_DROPTBL)
    {
	/* 
	** if user specified DROPTBL operation, id of the object is in 
	** base and idx
	*/
	
	/*
	** we assume that the caller has verified that the object exists, so 
	** we can set objects_dropped to TRUE
	*/
	objects_dropped = TRUE;

	tid = base;
	tidx = idx;
	type = DB_TABLE;
	cleanup_flag = DECREMENT_INDEX_COUNT;

	/*
	** note that we are setting DECREMENT_INDEX_COUNT in 
	** session.cleanup_tbl.flags because, if this object 
	** is an index, relidxcnt for its base table has not been decremented 
	** yet (and if it isn't an index, no harm done)
	*/
	exec sql insert into session.cleanup_tbl 
	    values(:tid, :tidx, :type, 0, :cleanup_flag);
    }
    else
    {
	DUVE_DROP_OBJ           *drop_obj;

	/*
	** duverel contains a list of tables/views/indices in the database and 
	** some of them may be marked for destruction
	*/

	cleanup_flag = 0;

	for (ctr = 0; ctr < duve_cb->duve_cnt; ctr++)
	{
	    if (duve_cb->duverel->rel[ctr].du_tbldrop == DUVE_DROP) 
	    {
		/*
		** remember that at least one object will be dropped
		*/
		objects_dropped = TRUE;

		tid = duve_cb->duverel->rel[ctr].du_id.db_tab_base;
		tidx = duve_cb->duverel->rel[ctr].du_id.db_tab_index;
		if (tidx)
		{
		    type = DB_INDEX;
		}
		else if (duve_cb->duverel->rel[ctr].du_stat & TCB_VIEW)
		{
		    type = DB_VIEW;
		}
		else
		{
		    type = DB_TABLE;
		}

		/*
		** note that we are NOT setting DECREMENT_INDEX_COUNT in 
		** session.cleanup_tbl.flags because this list only contains 
		** descirption of tables, view, and indices and if this object 
		** is an index, relidxcnt for its base table has already been 
		** decremented (and if it isn't an index, no harm done)
		*/
		exec sql REPEATED insert into session.cleanup_tbl
		    values(:tid, :tidx, :type, 0, :cleanup_flag);
	    }

	    if (duve_cb->duverel->rel[ctr].du_schadd == DUVE_ADD)
            {
               STcopy (duve_cb->duverel->rel[ctr].du_own, duve_cb->duve_schema);
	       create_schema(duve_cb);
            }
	}

	/*
	** "fix it" list may contain a list of object to be dropped or, in case
	** of database procedures, to be marked dormant; "fix it" list may
	** contain descriptions of
	**	- dbprocs,
	**	- rules,
	**	- constraints, and
	**	- dbevents
	*/
	for (drop_obj = duve_cb->duvefixit.duve_objs_to_drop; 
	     drop_obj; 
	     drop_obj = drop_obj->duve_next)
	{
	    /*
	    ** remember that at least one object will be dropped
	    */
	    objects_dropped = TRUE;

	    tid = drop_obj->duve_obj_id.db_tab_base;
	    tidx = drop_obj->duve_obj_id.db_tab_index;
	    type = drop_obj->duve_obj_type;
	    sec_id = drop_obj->duve_secondary_id;

	    /*
	    ** if the element describes a dbproc and indicates that it should 
	    ** be dropped, set DROP_THIS_DBPROC in session.cleanup_tbl.flags
	    */
	    cleanup_flag = 
		(type == DB_DBP && drop_obj->duve_drop_flags & DUVE_DBP_DROP)
		    ? DROP_THIS_DBPROC
		    : 0;

	    exec sql REPEATED insert into session.cleanup_tbl
		values(:tid, :tidx, :type, :sec_id, :cleanup_flag);
	}
    }

    if (!objects_dropped)
    {
	/*
	** no tuples have been entered into the temp table - there are no 
	** objects to destroy - we are done here
	*/
	return;
    }

    idx_type = DB_INDEX;
    tbl_type = DB_TABLE;
    view_type = DB_VIEW;
    dbp_type = DB_DBP;
    rule_type = DB_RULE;;
    cons_type = DB_CONS;
    intg_type = DB_INTG;
    event_type = DB_EVENT;
    perm_type = DB_PROT;

    /*
    ** At this point our temp table may contain descriptions of tables, views, 
    ** indexes, dbprocs, rules, constraints, and dbevents
    **
    ** Next we need to add to it descriptions of all objects that depend on 
    ** objects whose descriptions are already stored in the temp table.  
    **
    ** We will start up by looking up descriptions of indexes that depend on 
    ** tables whose descriptions are stored in the temp table.  
    **
    ** Finally we will do joins into IIDBDEPENDS and IIPRIV to pick up 
    ** descriptions of additional objects dependent on those whose descriptions 
    ** have been added to the temp table - note that this process will be 
    ** repeated until ALL objects dependent on those entered in the first step 
    ** have been entered into the temp table.
    */

    cleanup_flag = 0;

    /*
    ** note that we are NOT setting DECREMENT_INDEX_COUNT in 
    ** session.cleanup_tbl.flags because tables on which thiese indices were
    ** defined are being dropped and there is no point in worrying about their 
    ** relidxcount
    */
    exec sql insert into session.cleanup_tbl
	select reltid, reltidx, :idx_type, 0, :cleanup_flag
	from iirelation, session.cleanup_tbl
	where 
		obj_id = reltid 
	    and obj_idx = 0 
	    and obj_type = :tbl_type 
	    and reltidx != 0 
	    and not exists (select 1 
		 from session.cleanup_tbl 
		 where 
			 obj_id = reltid
		     and obj_idx = reltidx);
	    
    cleanup_flag = DECREMENT_INDEX_COUNT;

    do
    {
	cnt_this_run = 0;

	/*
	** now we will run joins into IIDBDEPENDS and IIPRIV looking for objects
	** dependent on those whose descriptions are already in the temp table
	** but whose own descriptions are yet to be entered
	**
	** query inserting values from IIDBDEPENDS may add descripotions of
	**	- views,
	**	- indexes,
	**	- dbprocs,
	**	- rules,
	**	- constraints, and
	**	- permits (QUEL-style permits refeencing a table other than the 
	**	  one on which it is defined)
	*/

	/*
	** note that we are setting DECREMENT_INDEX_COUNT in 
	** session.cleanup_tbl.flags because, if this object 
	** is an index, relidxcnt for its base table has not been decremented 
	** yet (and if it isn't an index, no harm done)
	*/
	exec sql REPEATED insert into session.cleanup_tbl
	    select deid1, deid2, dtype, qid, :cleanup_flag
	    from iidbdepends, session.cleanup_tbl
	    where
		    inid1 = obj_id
		and inid2 = obj_idx
					/* nothing can depend on rules */
		and obj_type != :rule_type	
		and (
					/* 
					** if the independent object is a
					** constraint, the IIDBDEPENDS tuple 
					** must match on all four keys
					*/
			(    obj_type = :cons_type 
		         and obj_type = itype 
		         and secondary_id = i_qid
			)

					/*
					** if the independent object type is
					** table/view/index (strictly speaking 
					** DB_TABLE would be enough, but one of
					** these days we may support constraints
					** on views), we want to pick up rules 
					** and dbprocs which depend on any 
					** constraint defined on this table + 
					** constraints defined on other tables 
					** which depend on some constraint 
					** defined on this table (we are not 
					** interested in indexes or constraints
					** on this table which depend on some 
					** constraint defined on this table 
					** because these get dropped 
					** automatically)
					*/
		     or (    (   obj_type = :tbl_type
			      or obj_type = :idx_type
			      or obj_type = :view_type
			     )
			 and itype = :cons_type
			 and (   dtype = :dbp_type
			      or dtype = :rule_type
			      or (    dtype = :cons_type
				  and (   deid1 != inid1
				       or deid2 != inid2
				      )
				 )
			     )
			)
					
					/*
					** finally, for objects other than 
					** constraints, the IIDBDEPENDS tuple 
					** must match on 3 keys (note that 
					** descriptions of independent 
					** tables/view/indices in IIDBDEPENDS 
					** have dtype==DB_TABLE which may 
					** disagree with our temp table which 
					** attempts to contain the real object 
					** type)
					*/

		     or (    itype != :cons_type
		         and (   itype = obj_type
		              or (    itype = :tbl_type
			          and (   obj_type = :idx_type 
				       or obj_type = :view_type
				      )
				 )
			     )
			)
		    )
		and not exists 
			(select 1
		             from session.cleanup_tbl 
		             where 
				     obj_id = deid1
				and obj_idx = deid2
				and obj_type = dtype
				and secondary_id = qid);

	exec sql inquire_sql (:cnt = rowcount);
	
	cnt_this_run += cnt;

	/*
	** now run a query joining IIPRIV with our temp table looking for views 
	** and dbprocs dependent on some privilege on object(s) being dropped 
	*/

	/*
	** note that we are NOT setting DECREMENT_INDEX_COUNT in 
	** session.cleanup_tbl.flags because IIPRIV does not contain 
	** descriptions of privileges on which indexes depend
	*/
	exec sql REPEATED insert into session.cleanup_tbl
	    select d_obj_id, d_obj_idx, d_obj_type, 0, 0
	    from iipriv, session.cleanup_tbl
	    where
		    i_obj_id = obj_id
		and i_obj_idx = obj_idx

					/* 
					** nothing can depend on privilege on a 
					** permit, rule, constraint 
					*/
		and obj_type != :perm_type
		and obj_type != :cons_type
		and obj_type != :rule_type
					
					/*
					** privileges on which views and dbprocs
					** depend reside in privilege descriptor
					** 0 - descriptors with number > 0 are 
					** of no interest to us
					*/
		and d_priv_number = 0
		and not exists 
			(select 1
		             from session.cleanup_tbl 
		             where 
				     obj_id = d_obj_id
				and obj_idx = d_obj_idx
				and obj_type = d_obj_type
				and secondary_id = 0);

	exec sql inquire_sql (:cnt = rowcount);
	
	cnt_this_run += cnt;

    } while (cnt_this_run != 0);

    /*
    ** our temp table contains descriptions of all objects to be dropped (or,
    ** in case of user-defined dbprocs, to be marked dormant);  the table may 
    ** contain descrioptions of objects of following types:
    **	- tables,
    **	- views,
    **	- indexes,
    **	- dbprocs,
    **	- rules,
    **	- constraints,
    **	- dbevents, and
    **	- permits (QUEL-style)
    **
    ** we will now proceed cleaning up catalogs for objects of each type 
    ** (tables, views, and indexes will be lumped together).  Following the
    ** general cleanup, additional processing will occur for database procedures
    ** and indexes:
    **	- we will scan the list of dbprocs in our temp table and for those of 
    **    them that are system-generated or marked to be destroyed (which is 
    **	  indicated by DROP_THIS_DBPROC being set in session.cleanup_tbl.flags),
    **	  will proceeed to destroy them (delete IIPROCEDURE, IIQRYTEXT, and 
    **	  IIPROCEDURE_PARAMETER tuples corresponding to them); 
    **	  for the rest, we will reset DB_DBPGRANT_OK, DB_ACTIVE_DBP, and 
    **	  DB_DBP_INDEP_LIST in dbp_mask1 and zero out dbp_ubt_id and dbp_ubt_idx
    ** 	- we will build a temp table containing number of indexes for every 
    **	  table on which some index has been deleted (and whose relidxcnt has 
    **	  not been altered to reflect it, which is indicated by 
    **    DECREMENT_INDEX_COUNT being set in session.cleanup_tbl.flags)
    **	  and then proceed to update relidxcnt and, if necessary, relstat of 
    **	  such tables
    */

    /*
    ** RULES
    */

    /* delete trees associated with rules */
    exec sql delete from iitree 
	where exists (select 1
			  from session.cleanup_tbl, iirule
			  where
				  obj_id = rule_id1
			      and obj_idx = rule_id2
			      and obj_type = :rule_type
			      and rule_tabbase = treetabbase
			      and rule_tabidx  = treetabidx
			      and rule_treeid1 = treeid1
			      and rule_treeid2 = treeid2
			      and treemode     = :rule_type);

    /* if performing DROPTBL operation, delete query text for rules as well */
    if (duve_cb->duve_operation == DUVE_DROPTBL)
    {
	exec sql delete from iiqrytext
	    where exists (select 1
			      from session.cleanup_tbl, iirule
			      where
				      obj_id = rule_id1
				  and obj_idx = rule_id2
				  and obj_type = :rule_type
				  and rule_qryid1 = txtid1
				  and rule_qryid2 = txtid2
				  and mode = :rule_type);
    }

    /*
    ** delete IIDBDEPENDS tuples describing dependency of rules on constraints
    */
    exec sql delete from iidbdepends
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = deid1
			      and obj_idx = deid2
			      and obj_type = :rule_type
			      and dtype = :rule_type);

    /* delete descriptions of rules */
    exec sql delete from iirule
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = rule_id1
			      and obj_idx = rule_id2
			      and obj_type = :rule_type);

    /*
    ** CONSTRAINTS
    */

    /* if performing DROPTBL operation, delete query text for constraints */
    if (duve_cb->duve_operation == DUVE_DROPTBL)
    {
	exec sql delete from iiqrytext
	    where exists (select 1
			      from session.cleanup_tbl, iiintegrity
			      where
				      obj_id = inttabbase
				  and obj_idx = inttabidx
				  and obj_type = :cons_type
				  and secondary_id = intnumber
				  and intqryid1 = txtid1
				  and intqryid2 = txtid2
						/* 
						** qtxt mode is DB_INTG - 
						** not DB_CONS 
						*/
				  and mode = :intg_type);
    }

    /* delete IIKEY records for UNIQUE/REF constraints */
    exec sql delete from iikey
	where exists (select 1
			  from session.cleanup_tbl, iiintegrity
			  where
				  obj_id = inttabbase
			      and obj_idx = inttabidx
			      and obj_type = :cons_type
			      and secondary_id = intnumber
			      and consid1 = key_consid1
			      and consid2 = key_consid2);
    
    /*
    ** delete IIDBDEPENDS tuples describing dependency of REF constraints
    ** on UNIQUE constraint on <referenced columns> or a privilege descriptor 
    ** representing a REFERENCES privilege on <referenced columns>
    */
    exec sql delete from iidbdepends
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = deid1
			      and obj_idx = deid2
			      and obj_type = :cons_type
			      and dtype = :cons_type
			      and secondary_id = qid);
    
    /* delete descriptions of constraints */
    exec sql delete from iiintegrity
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = inttabbase
			      and obj_idx = inttabidx
			      and obj_type = :cons_type
			      and secondary_id = intnumber);

    /*
    ** PERMITS (QUEL-style)
    */

    /* delete trees associated with permits */
    exec sql delete from iitree 
	where exists (select 1
			  from session.cleanup_tbl, iiprotect
			  where
				  obj_id = protabbase
			      and obj_idx = protabidx
			      and obj_type = :perm_type
			      and secondary_id = propermid
			      and protabbase = treetabbase
			      and protabidx  = treetabidx
			      and protreeid1 = treeid1
			      and protreeid2 = treeid2
			      and treemode   = :perm_type);

    /* if performing DROPTBL operation, delete query text for rules as well */
    if (duve_cb->duve_operation == DUVE_DROPTBL)
    {
	exec sql delete from iiqrytext
	    where exists (select 1
			      from session.cleanup_tbl, iiprotect
			      where
				      obj_id = protabbase
				  and obj_idx = protabidx
				  and obj_type = :perm_type
				  and secondary_id = propermid
				  and proqryid1 = txtid1
				  and proqryid2 = txtid2
				  and mode = :perm_type);
    }

    /* delete descriptions of permits */
    exec sql delete from iiprotect
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = protabbase
			      and obj_idx = protabidx
			      and obj_type = :perm_type
			      and secondary_id = propermid);

    /*
    ** TABLES/VIEWS/INDICES
    */

    /* delete descriptions of table/view/index attributes */
    exec sql delete from iiattribute 
	where exists (select 1 
			  from session.cleanup_tbl
			  where 
				  obj_id = attrelid
			      and obj_idx = attrelidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete index records for indices */
    exec sql delete from iiindex
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = baseid
			      and obj_idx = indexid
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete device records for tables/indices */
    exec sql delete from iidevices
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = devrelid
			      and obj_idx = devrelidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /*
    ** if performing DROPTBL operation, delete query text tuples for views,
    ** integrities, constraints, permits, security_alarms, and rules
    */
    if (duve_cb->duve_operation == DUVE_DROPTBL)
    {
	exec sql delete from iiqrytext where exists 
	    (select 1
	        from session.cleanup_tbl
		where
		        (   obj_type = :tbl_type
			 or obj_type = :view_type
			 or obj_type = :idx_type
			)

					/* query text of views */
		    and (   exists (select 1 from iirelation
					where 
						obj_id = reltid 
					    and obj_idx = reltidx
					    and relqid1 = txtid1
					    and relqid2 = txtid2
					    and mode = :view_type)

					/* 
					** query text of integrities/constraints
					*/
			 or exists (select 1 from iiintegrity
					where
						obj_id = inttabbase
					    and obj_idx = inttabidx
					    and intqryid1 = txtid1
					    and intqryid2 = txtid2
					    and mode = :intg_type)
					
					/* 
					** query text of permits/security_alarms
					*/
			 or exists (select 1 from iiprotect
					where
						obj_id = protabbase
					    and obj_idx = protabidx
					    and proqryid1 = txtid1
					    and proqryid2 = txtid2
					    and mode = :perm_type)

					/* query text of rules */
			 or exists (select 1 from iirule
					where
						obj_id = rule_tabbase
					    and obj_idx = rule_tabidx
					    and rule_qryid1 = txtid1
					    and rule_qryid2 = txtid2
					    and mode = :rule_type)
			)
	    );
    }

    /* delete integrity/constraint records for tables */
    exec sql delete from iiintegrity
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = inttabbase
			      and obj_idx = inttabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete permit/security_alarm records for tables/views */
    exec sql delete from iiprotect
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = protabbase
			      and obj_idx = protabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete tree records for views/permits/integrities/rules */
    exec sql delete from iitree
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = treetabbase
			      and obj_idx = treetabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete histogram records for tables/indices */
    exec sql delete from iihistogram
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = htabbase
			      and obj_idx = htabindex
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete statistics records for tables/indices */
    exec sql delete from iistatistics
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = stabbase
			      and obj_idx = stabindex
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete security alarm records for tables/views */
    exec sql delete from iisecalarm 
	where exists (select 1
			  from session.cleanup_tbl 
			  where
				  obj_id = obj_id1
			      and obj_idx = obj_id2
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete rule records for tables/views */
    exec sql delete from iirule
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = rule_tabbase
			      and obj_idx = rule_tabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete synonym records for tables/views/indices */
    exec sql delete from iisynonym
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = syntabbase
			      and obj_idx = syntabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /* delete comment records for tables/views/indices */
    exec sql delete from iidbms_comment
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = comtabbase
			      and obj_idx = comtabidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

	
    /* 
    ** delete IIDBDEPENDS records describing dependence of indices/views on 
    ** other objects, of permits on views and constraints on tables on 
    ** privilege descriptors, and of indexes and constraints on tables on other 
    ** constraints
    **
    ** NOTE that we are not concerned about deleting IIDBDEPENDS tuples 
    ** describing dependence of rules created to enforce a constraint on a 
    ** constraint because these were taken care of above (when we cleaned up 
    ** IIDBDEPENDS tuples describing dependence of rules on constraints)
    */
    exec sql delete from iidbdepends
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = deid1
			      and obj_idx = deid2
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));
    
    /*
    ** delete IIPRIV records comprising privilege descriptors representing 
    ** privileges on which views, permits on views, and constraints on tables
    ** depend
    */
    exec sql delete from iipriv
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = d_obj_id
			      and obj_idx = d_obj_idx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));
    
    /* delete IIRELATION records for tables, views, and indices */
    exec sql delete from iirelation
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = reltid
			      and obj_idx = reltidx
			      and (   obj_type = :tbl_type
				   or obj_type = :view_type
				   or obj_type = :idx_type));

    /*
    ** finally, we may need to update relidxcount for tables whose indices were
    ** dropped because they depended on some object marked for destruction and,
    ** for those tables whose relidxcount reaches zero, we need to reset 
    ** TCB_IDXD in relstat
    */

    /* 
    ** create and populate the temp table which will contain number of 
    ** remaining indices for affected tables 
    */
    exec sql declare global temporary table session.idx_cnt_tbl
	(tbl_id		integer,
	 tbl_idx	integer not null with default,
	 idxcnt		integer)
	 on commit preserve rows with norecovery;

    cleanup_flag = DECREMENT_INDEX_COUNT;

    exec sql insert into session.idx_cnt_tbl(tbl_id, idxcnt)
	select reltid, count(reltidx)
	from iirelation
	where
		reltidx > 0
	    and exists (select 1
			    from session.cleanup_tbl
			    where
				    obj_id = reltid
				and obj_idx > 0
				and obj_type = :idx_type
				and mod((flags/:cleanup_flag),(2)) = 1)
	group by reltid;
    
    exec sql inquire_sql (:cnt = rowcount);

    if (cnt > 0)
    {
	/* relidxcount needs to change for at least one table */

	exec sql update iirelation from session.idx_cnt_tbl
	    set relidxcount = idxcnt
	    where reltid = tbl_id and reltidx = tbl_idx;

	/*
	** finally, if relidxcnt in some table has dropped to 0, we need to 
	** reset TCB_IDXD bit in relstat
	*/
        EXEC SQL DECLARE tcb_idxd_cursor CURSOR FOR
	    select relstat
	        from iirelation
	        where 
		        relidxcount = 0 
		    and exists (select 1 
			        from session.idx_cnt_tbl
			        where
				      tbl_id = reltid
				  and tbl_idx = reltidx);

    	EXEC SQL open tcb_idxd_cursor;

    	for (;;)
    	{
	    EXEC SQL FETCH tcb_idxd_cursor into :relstat;
    
	    if (sqlca.sqlcode == 100)
	    {
	        EXEC SQL CLOSE tcb_idxd_cursor;
	        break;
	    }

	    if (relstat & TCB_IDXD)
	    {
		relstat &= ~TCB_IDXD;
		exec sql update iirelation 
		    set relstat = :relstat
			where current of tcb_idxd_cursor;
	    }
        }
    }

    /*
    ** DATABASE PROCEDURES
    */

    /* delete permit records for dbprocs */
    exec sql delete from iiprotect
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = protabbase
			      and obj_idx = protabidx
			      and obj_type = :dbp_type);

    /* 
    ** delete IIDBDEPENDS records describing dependence of dbprocs on 
    ** other objects
    */
    exec sql delete from iidbdepends
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = deid1
			      and obj_idx = deid2
			      and obj_type = :dbp_type);
    
    /*
    ** delete IIPRIV records comprising privilege descriptors representing 
    ** privileges on which dbprocs depend
    */
    exec sql delete from iipriv
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = d_obj_id
			      and obj_idx = d_obj_idx
			      and obj_type = :dbp_type);
    /*
    ** now we need to examine IIPROCEDURE tuples for dbprocs in our temp table:
    ** system-generated dbprocs and dbprocs which we were told must be dropped 
    ** (which will indicated by DROP_THIS_DBPROC being set in 
    ** session.cleanup_tbl.flags) need to be dropped (meaning that we need to 
    ** delete IIQRYTEXT, IIPROCEDURE_PARAMETER, IIPROCEDURE tuples) while the 
    ** rest need to be marked dormant (reset DB_DBPGRANT_OK, DB_ACTIVE_DBP, 
    ** DB_DBP_INDEP_LIST in dbp_mask1) and have the underlying base table id 
    ** zeroed out
    */
    EXEC SQL DECLARE dbp_cleanup_cursor CURSOR FOR
	select dbp_id, dbp_idx, dbp_txtid1, dbp_txtid2, dbp_mask1 
	    from iiprocedure
	    where exists (select 1 
			      from session.cleanup_tbl
			      where
				      obj_id = dbp_id
				  and obj_idx = dbp_idx
				  and obj_type = :dbp_type);
    EXEC SQL open dbp_cleanup_cursor;

    for (;;)
    {
	bool		dbproc_must_go;

	EXEC SQL FETCH dbp_cleanup_cursor 
	    into :tid, :tidx, :qtxtid1, :qtxtid2, :dbp_mask;

	if (sqlca.sqlcode == 100)
	{
	    EXEC SQL CLOSE dbp_cleanup_cursor;
	    break;
	}

	if (dbp_mask & DBP_SYSTEM_GENERATED)
	{
	    dbproc_must_go = TRUE;
	}
	else
	{
	    exec sql repeated select flags into :cleanup_flag
		from session.cleanup_tbl
		where 
			obj_id = :tid 
		    and obj_idx = :tidx 
		    and obj_type = :dbp_type;

	    dbproc_must_go = (cleanup_flag & DROP_THIS_DBPROC);
	}

	if (dbproc_must_go)
	{
	    /* 
	    ** system-generated dbproc or a dbproc which we were told to 
	    ** drop - must be deleted 
	    */
	    
	    /* delete descriptions of dbproc's parameters */
	    exec sql delete from iiprocedure_parameter 
		where pp_procid1 = :tid and pp_procid2 = :tidx;
	    
	    /* delete dbproc's query text */
	    exec sql delete from iiqrytext
		where txtid1 = :qtxtid1 and txtid2 = :qtxtid2;
	    
	    /* delete the description of the dbproc itself */
	    exec sql delete from iiprocedure 
		where current of dbp_cleanup_cursor;
	}
	else
	{
	    dbp_mask &= ~(DB_DBPGRANT_OK | DB_ACTIVE_DBP | DB_DBP_INDEP_LIST);
	    exec sql update iiprocedure 
		set dbp_mask1 = :dbp_mask, dbp_ubt_id = 0, dbp_ubt_idx = 0
		where current of dbp_cleanup_cursor;
	}
    }

    /*
    ** DBEVENTS
    */
    
    /* delete permit records for dbevents */
    exec sql delete from iiprotect
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = protabbase
			      and obj_idx = protabidx
			      and obj_type = :event_type);

    /* delete descriptions of dbevents */
    exec sql delete from iievent
	where exists (select 1
			  from session.cleanup_tbl
			  where
				  obj_id = event_idbase
			      and obj_idx = event_idx
			      and obj_type = :event_type);

    /*
    ** some objects were dropped - flush the RDF cache
    */
    exec sql set trace point rd2;
}

/*
[@function_definition@]...
*/
