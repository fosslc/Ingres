/*
** Copyright (c) 1988, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<bt.h>
#include	<st.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<adf.h>
#include	<er.h>
#include	<lo.h>
#include	<pc.h>
#include	<si.h>
#include	<duerr.h>
#include	<duvfiles.h>
#include	<duve.h>
#include	<duustrings.h>

#include	<cs.h>
#include	<lg.h>
#include	<lk.h>
#include	<dm.h>
#include	<dmf.h>
#include	<dmp.h>
#include	<cv.h>

    exec sql include SQLCA;	/* Embedded SQL Communications Area */

    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvevent.sh>;
	exec sql include <duveproc.sh>;
	exec sql include <duvepro.sh>;
	exec sql include <duveprv.sh>;
    EXEC SQL END DECLARE SECTION;

/**
**
**  Name: DUVECHK2.SC - Verifydb System Catalog checks for non-core dbms
**		       catalogs.
**
**  Description:
**      This module contains the ESQL to execute the tests/fixed on the system
**	catalogs specified by the Verifydb Product Specification for:
**	    iidbms_comments
**	    iidbdepends
**	    iidefault
**	    iievent
**	    iigw06_attribute
**	    iigw06_relation
**	    iihistogram
**	    iiintegrity
**	    iikey	
**	    iiprocedure
**	    iiprotect
**	    iiqrytext
**	    iirule
**	    iistatistics
**	    iisynonym
**	    iitree
**	    iixdbdepends
**	    iisecalarm
**  
**      duvechk2.sc uses the verifydb control block to obtain information
**	about the verifydb system catalog check environment and to access
**	cached information about the iirelation and iidevices catalogs.
**	If duvechk wishes to drop a user table from the DBMS, it marks the
**	table for drop by setting the drop_table flag in the cached relinfo
**	(information about iirelation).
**
**	    ckcomments - check / fix iidbms_comments system catalog
**	    ckdbdep - check / fix iidbdepends system catalog
**	    ckdefault - check /fix iidefault system catalog
**	    ckevent - check / fix iievent system catalog
**	    ckgw06att - check / fix iigw06_attribute catalog
**	    ckgw06rel - check / fix iigw06_relation calalog
**	    ckhist - check / fix iihistogram system catalog
**          ckintegs - check / fix iiintegrity system catalog
**	    ckkey - check / fix iikey system catalog
**	    ckproc - check /fix iiprocedure catalog
**	    ckprotups - check/fix iiprotect catalog
**	    ckqrytxt - check / fix iiqrytext system catalog
**	    ckrule - check / fix iirule system catalog
**	    ckstat - check / fix iistatistic system catalog
**	    cksynonym - check /fix iisynonym system catalog
**	    cktree - check / fix iitree system catalog
**	    ckxdbdep - check / fix iixdbdepends system catalog
**	    cksecalarm  - check / fix iisecalarm system catalog
**	    
**	Special ESQL include files are used to describe each dbms system
**	catalog. These include files are generated via the DCLGEN utilility
**	and require a dbms server to be running. The following commands create
**	the include files in the format required by verifydb:
**	    dclgen c iidbdb iidbms_commnt duvecmnt.sh cmnt_tbl
**	    dclgen c iidbdb iidbdepends duvedpnd.sh  dpnd_tbl
**	    dclgen c iidbdb iidefault duvedef.sh default_tbl
**	    dclgen c iidbdb iievent duvevent.sh evnt_tbl
**	    dclgen c iidbdb iihistogram duvehist.sh hist_tbl
**	    dclgen c iidbdb iiintegrity duveint.sh int_tbl
**	    dclgen.c iidbdb iikey duvekey.sh key_tbl 
**	    dclgen c iidbdb iiprocedure duveproc.sh proc_tbl
**	    dclgen c iidbdb iiprotect duvepro.sh pro_tbl
**	    dclgen c iidbdb iiqrytext duveqry.sh qry_tbl
**	    dclgen c iidbdb iirule duverule.sh rule_tbl
**	    dclgen c iidbdb iistatistics duvestat.sh stat_tbl
**	    dclgen c iidbdb iisynonym duvesyn.sh syn_tbl
**	    dclgen c iidbdb iitree duvetree.sh tree_tbl
**	    dclgen c iidbdb iixdbdepends duvexdp.sh xdpnd_tbl
**	    dclgen c iidbdb iixpriv duvexprv.sh xprv_tbl
**          dclgen c iidbdb iipriv duveprv.sh prv_tbl
**	    dclgen c iidbdb iixevent duvexev.sh xev_tbl
**	    dclgen c iidbdb iixprocedure duvexdbp.sh xdbp_tbl
**	    dclgen c iidbdb iisecalarm duvealm.sh alarm_tbl
**
** History:
**	25-Aug-88 (teg)
**	    initial creation
**      04-Apr-89 (neil)
**          Added checks for rule trees and text.  At this time this was only
**          added to suppress failures and not to actually test the
**          relationships between iitree, iiqrytext, iirelation and iirule.
**      03-Jan-89 (neil)
**          Added checks for event text, permits and trees.  As with rules
**          these checks were added to suppress failures and not to actually
**          test the relationships between the qrymod tables and iievent.
**      12-Jun-90 (teresa)
**          add logic to check iisynonym catalog
**      15-feb-91 (teresa)
**          fix bug 35940 (assumed there was only 1 tree entry per table id,
**          which is invalid assumption.  ex: define multiple QUEL permits on
**          the same table.)
**      06-jan-92 (rickh)
**          Constraints don't have query trees so please forgive
**          them.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Include <lg.h> to pick up logging system typedefs
**	16-jun-93 (anitap)
**	    Added new tests to cksynonmym, ckproc() and ckevent() for CREATE 
**	    SCHEMA project.
**	    Added new routines ckdefault() and ckkey() and tests to ckintegs()
**          and ckdbdep() for FIPS constraints.
**	26-jul-1993 (bryanp)
**	    Add includes of <lk.h> and <pc.h> in support of dmp.h. I would
**	    prefer that this file did not include dmp.h; maybe someday that
**	    will be changed.
**	30-jul-1993 (stephenb)
**	    Added routines ckgw06rel() and ckgw06att() for verifydb
**	    dbms_catalogs checks of iigw06_relation and iigw06_attribute.
**	 4-aug-93 (shailaja)
**	    Unnested comments.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	18-sep-93 (teresa)
**	    Add routine ckcomments() and add new tests to ckdatabase().
**	10-oct-93 (anitap)
**	    Added new tests to ckrule() and ckproc() for FIPS
**	    constraints' project. Also added support to delete tuples from
**	    iiintegrity if corresponding tuples not present in iirule, 
**	    iiprocedure, iiindex in ckintegs().
**	    Added new routine create_cons() for FIPS constraints' project.
**	    Corrected spelling of message S_DU0320_DROP_FROM_IIINTERGRITY to
**	    S_DU0320_DROP_FROM_IIINTEGRITY in ckintegs().
**	17-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (robf)
**          Add cksecalarm() for security alarms.
**	01-jan-94 (anitap)
**	    performance enhancement for verifydb. Use repeat queries wherever 
**	    possible and use ANY instead of count. Use iidbdepends cache and
**	    iirelation cache wherever possible.
**	    If more than one referential constraint dependent on an unique
**	    constraint, use loop to create all the referential constraints
**	    after dropping the unique constraint. 
**	25-jan-94 (andre)
**	    removed duplicate definition of treeclean()
**	08-mar-94 (anitap)
**	    Rick has added another dbproc for referential constraint. Fix for
**	    the same in ckintegs().
**      03-feb-95 (chech02) 
**          After 'select count(*) into :cnt from table' stmt, We need to  
**          check cnt also.
**	 2-feb-1996 (nick)
**	    X-ref dependency check for events was querying 'iidbevent' ; this
**	    is a typo and should be 'iievent'.  The query condition was broken
**	    too ( though it never got that far ! ). #74424
**	05-apr-1996 (canor01)
**	    after freeing duve_cb->duvestat, set it to NULL to prevent it
**	    from being freed again later.
**	23-may-96 (nick)
**	    X-ref dependency check for synonyms broken. #76673
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Jul-2002 (inifa01)
**	    Bug 106821.  In ckintegs() do not verify existence of an index in 
**	    Test 10 for constraint where base table is index.
**	09-Apr-2003 (inifa01) bug 110019 INGSRV 2204
**	    in ckkey() don't return error if dealing with referential constraint
**	    column.
**      03-may-2004 (stial01)
**          Select int4(tid) (since tid is now i8) (b112259)
**          (Temporary fix until we add support for i8 host variable)
**      04-may-2004 (stial01)
**          Fixed up references to tid host variables. (b112259, b112266)
**	29-Dec-2004 (schka24)
**	    Remove iixprotect.
**	19-jan-2005 (abbjo03)
**	    Change duvecb to GLOBALREF.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      12-oct-2009 (joea)
**          Add cases for DB_DEF_ID_FALSE/TRUE in ckdefault.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	27-Apr-2010 (kschendel) SIR 123639
**	    Change bitmap columns to BYTE for easier expansion.
**/


/*
**  Forward and/or External typedef/struct references.
*/
	GLOBALREF DUVE_CB duvecb;


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS ckcomments();	  /* check/repair iidbms_comments */
FUNC_EXTERN DU_STATUS ckdbdep();          /* check/repair iidbdepends */
FUNC_EXTERN DU_STATUS ckdefault();	  /* check/repair iidefault */
FUNC_EXTERN DU_STATUS ckevent();          /* check/repair iievent */
FUNC_EXTERN DU_STATUS ckgw06att();	  /* check/repair iigw06_attribute */
FUNC_EXTERN DU_STATUS ckgw06rel();	  /* check/repair iigw06_relation */
FUNC_EXTERN DU_STATUS ckhist();           /* check/repair iihistogram */
FUNC_EXTERN DU_STATUS ckintegs();         /* check/repair iiintegrity */
FUNC_EXTERN DU_STATUS ckkey();		  /* check/repair iikey */
FUNC_EXTERN DU_STATUS ckproc();           /* check/repair iiprocedure */
FUNC_EXTERN DU_STATUS ckprotups();        /* check/repair iiprotect */
FUNC_EXTERN DU_STATUS ckqrytxt();         /* check iiqrytext */
FUNC_EXTERN DU_STATUS ckrule();           /* check/repair iirule */
FUNC_EXTERN DU_STATUS ckstat();           /* check/repair iistatistic */
FUNC_EXTERN DU_STATUS cksynonym();        /* check/repair iisynonym */
FUNC_EXTERN DU_STATUS cktree();           /* check/repair iitree */
FUNC_EXTERN DU_STATUS ckxdbdep();         /* check/repair iixdbdepends */
FUNC_EXTERN DU_STATUS duve_d_dpndfind();  /* find dependent object info element 
					  ** in IIDBDEPENDS cache */
FUNC_EXTERN DU_STATUS duve_i_dpndfind();  /* find independent object info
                                          ** element in IIDBDEPENDS cache */
FUNC_EXTERN void      duve_obj_type();    /* produce a string describing object
                                          ** type based on object type code */
FUNC_EXTERN DU_STATUS cksecalarm();       /* check/repair iisecalarm */
FUNC_EXTERN VOID treeclean();             /* clean up after drop of tree tuple*/

/* Statistics gathering/reporting utility routines for Debug mode */
FUNC_EXTERN VOID get_stats(i4 *bio, i4 *dio, i4 *ms);
FUNC_EXTERN VOID printstats(DUVE_CB *duve_cb, i4  test_level, char *id);
FUNC_EXTERN VOID init_stats(DUVE_CB *duve_cb, i4  test_level);

/* local functions */
static DU_STATUS create_cons();		  /* cache iintegrity info to drop and
					  ** add create constraint
					  ** for ckintegs()
					  */
static DU_STATUS alloc_mem();		  /* allocate memory and cache query 
					  ** text for creating constraint.
					  */
static VOID
duve_check_privs(
		 DB_TAB_ID	*objid, 
		 i4		privneeded, 
		 char		*privgrantee, 
		 u_i4		*attrmap, 
		 bool		*satisfied);
/*
**  LOCAL CONSTANTS
*/
#define        CMDBUFSIZE      512

/*{
** Name: ckcomments - Perform checks/fixes on iidbms_comments system catalog
**
** Description:
**      this routine performs system catalog tests on iidbms_comment
**	system catalog.  The specific tests are:
** (by comtabbase, comtabidx, comattid, text_sesquence in ascending order)
** 1.      Verify that the catalog receiving the comment really exists
** TEST:   find a tuple in iirelation where
**               iirelation.reltid = iidbms_comments.comtabbase and
**               iirelation.reltidx = iidbms_comments.comtabidx
** FIX:    delete the tuple from iidbms_comments
**
** 2.    Verify that the catalog receiving the comment knows there are comments
**       on it.
** TEST: for the iirelation tuple found above, verify that the TCB_COMMENT bit
**       of iirelation.relstat is set.
** FIX:  Set iirelation.relstat.TCB_COMMENT for this tuple.
**
** 3.    Verify internal integrity of the comment type
** TEST: If iidbms_comments.comattid is zero, verify that bit DBC_T_TABLE
**       is set and bit DBC_T_COLUMN is not set in iidbms_comments.comtype.
**       If iidbms_comments.comattid is nonzero, verify that bit DBC_T_COLUMN
**       is set and bit DBC_T_TABLE s not set in iidbms_comments.comtype.
** FIX:  Delete this tuple from iidbms_comments and clear TCB_COMMENT bit in
**       iirelation.relstat
**
** 4.    Verify that the attribute receiving the comment really exists if this
**       is a column comment (as oppossed to a table comment)
** TEST: if iidbms_comments.comattid is nonzero, verify that it is less than or
**       equal to iirelation.relatts.
** FIX:  Delete this tuple from iidbms_comments and clear TCB_COMMENT bit in
**       iirelation.relstat.
**
** 5.    If this comment is a long remark that spands more than 1 tuple, verify
**       that the sequence numbers are correct.
** TEST: Collect all tuples for this iidbms_comments.comtabbase,
**       iidbms_comments.comtabidx and iidbms_comments.comattid, ordering by
**       iidbms_comments.text_sequence.  Verify that text_sequence starts with
**       zero and ordinally increases (i.e., 0, then 1, then 2, etc).
** FIX:  Delete this tuple from iidbms_comments and clear TCB_COMMENT bit in
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**		du_stat				relstat bits may be set/cleared
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    Table or Column Comments may be dropped from database
**	    iirelation tuples may be modified
**
** History:
**      18-sep-93 (teresa)
**          initial creation
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
[@history_template@]...
*/
DU_STATUS
ckcomments(duve_cb)
DUVE_CB		*duve_cb;
{
        
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvecmnt.sh>;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i4	id;
	i4	relstat;
	i4 cnt;
    EXEC SQL END DECLARE SECTION;
    DB_TAB_ID	cmnt_id;
    i2		cmnt_att;
    i4		base = DUVE_DROP;
    i4	seq=0;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE cmnt_cursor CURSOR FOR
	select * from iidbms_comment
	order by comtabbase, comtabidx, comattid, text_sequence;
    EXEC SQL open cmnt_cursor;

    /* loop for each tuple in iidbms_comments */
    for (cmnt_id.db_tab_base= -1,cmnt_id.db_tab_index= -1, cmnt_att = -1;;)
    {
	/* get tuple from table */
	EXEC SQL FETCH cmnt_cursor INTO :cmnt_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in table */
	{
	    EXEC SQL CLOSE cmnt_cursor;
	    break;
	}

	if ( (cmnt_tbl.comtabbase != cmnt_id.db_tab_base) ||
	     (cmnt_tbl.comtabidx != cmnt_id.db_tab_index) ||
	     (cmnt_att != cmnt_tbl.comattid) )
	{	
	    /* test 1 - verify base table really exists */
	    cmnt_id.db_tab_base = cmnt_tbl.comtabbase;
	    cmnt_id.db_tab_index  = cmnt_tbl.comtabidx;
	    seq = 0;
	    base = duve_findreltid ( &cmnt_id, duve_cb);
	    if (base == DUVE_DROP)
		continue;
	    if (base == DU_INVALID)
	    {
		if (duve_banner( DUVE_IIDBMS_COMMENT, 1, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk( DUVE_MODESENS, duve_cb, S_DU169E_MISSING_COM_BASE, 4,
			   sizeof(cmnt_id.db_tab_base), &cmnt_id.db_tab_base,
			   sizeof(cmnt_id.db_tab_index), &cmnt_id.db_tab_index);
		if (duve_talk( DUVE_ASK, duve_cb, S_DU034F_DROP_IIDBMS_COMMENT,
			   4, sizeof(cmnt_tbl.comtabbase), &cmnt_tbl.comtabbase,
			   sizeof(cmnt_tbl.comtabidx), &cmnt_tbl.comtabidx)
		== DUVE_YES)
		{

		    EXEC SQL delete from iidbms_comment where
			comtabbase = :cmnt_tbl.comtabbase and
			comtabidx = :cmnt_tbl.comtabidx;
		    duve_talk( DUVE_MODESENS, duve_cb,
			      S_DU039F_DROP_IIDBMS_COMMENT, 4,
			      sizeof(cmnt_tbl.comtabbase), &cmnt_tbl.comtabbase,
			      sizeof(cmnt_tbl.comtabidx), &cmnt_tbl.comtabidx);
		}
		continue;
	    }

	    /* test 2 - verify base knows it has comments */

	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_COMMENT) == 0 )
	    {
		if (duve_banner( DUVE_IIDBMS_COMMENT, 2, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);

		duve_talk(DUVE_MODESENS, duve_cb, S_DU169F_NO_COMMENT_RELSTAT,4,
			  0,duve_cb->duverel->rel[base].du_tab,
			  0,duve_cb->duverel->rel[base].du_own);
		if ( duve_talk( DUVE_ASK, duve_cb, S_DU0240_SET_COMMENT, 0)
		    == DUVE_YES)
		{
		    relstat = duve_cb->duverel->rel[base].du_stat | TCB_COMMENT;
		    duve_cb->duverel->rel[base].du_stat = relstat;
		    basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		    basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		    EXEC SQL update iirelation set relstat = :relstat where
			reltid = :basetid and reltidx = :basetidx;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0440_SET_COMMENT, 0);
		}

	    } /* end test 2 */

	    /* test 3 - verify internal integrity of the comment type */
	    if ( ( (cmnt_tbl.comattid )
		   &&
		   ( (cmnt_tbl.comtype & DBC_T_TABLE) ||
		     ((cmnt_tbl.comtype & DBC_T_COLUMN)==0)
		   )
		 )
		 ||
		 ( (cmnt_tbl.comattid==0 )
		    &&
		    ( ((cmnt_tbl.comtype & DBC_T_TABLE)==0) ||
		      (cmnt_tbl.comtype & DBC_T_COLUMN)
		    )
		 )
	       )	   
	    {

		if (duve_banner( DUVE_IIDBMS_COMMENT, 3, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS) DUVE_BAD);
		if (cmnt_tbl.comattid)
		    duve_talk(	DUVE_MODESENS, duve_cb,
				S_DU16A0_BAD_COL_COMMENT, 6,
				sizeof(cmnt_tbl.comattid), &cmnt_tbl.comattid,
				0,duve_cb->duverel->rel[base].du_tab,
				0,duve_cb->duverel->rel[base].du_own);
		else
		    duve_talk(	DUVE_MODESENS, duve_cb,
				S_DU16A1_BAD_TBL_COMMENT, 4,
				0,duve_cb->duverel->rel[base].du_tab,
				0,duve_cb->duverel->rel[base].du_own);
		if (duve_talk( DUVE_ASK, duve_cb, S_DU0241_DROP_THIS_COMMENT,0)
		== DUVE_YES)
		{
		    EXEC SQL delete from iidbms_comment where
			comtabbase = :cmnt_tbl.comtabbase and
			comtabidx = :cmnt_tbl.comtabidx and
			comattid = :cmnt_tbl.comattid;
		    duve_talk( DUVE_MODESENS, duve_cb,
			      S_DU0441_DROP_THIS_COMMENT, 0);
		}
		continue;
	    } /* end test 3 */

	    /* test 4 - verify col id is valid if its a column comment */

	    if (duve_cb->duverel->rel[base].du_attno < cmnt_tbl.comattid)
	    {
		if (duve_banner( DUVE_IIDBMS_COMMENT, 4, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);

		if (cmnt_tbl.comattid)
		    duve_talk(	DUVE_MODESENS, duve_cb,
				S_DU16A0_BAD_COL_COMMENT, 6,
				sizeof(cmnt_tbl.comattid), &cmnt_tbl.comattid,
				0,duve_cb->duverel->rel[base].du_tab,
				0,duve_cb->duverel->rel[base].du_own);
		else
		    duve_talk(	DUVE_MODESENS, duve_cb,
				S_DU16A1_BAD_TBL_COMMENT, 4,
				0,duve_cb->duverel->rel[base].du_tab,
				0,duve_cb->duverel->rel[base].du_own);
		if (duve_talk( DUVE_ASK, duve_cb, S_DU0241_DROP_THIS_COMMENT, 0)
		== DUVE_YES)
		{
		    EXEC SQL delete from iidbms_comment where
			comtabbase = :cmnt_tbl.comtabbase and
			comtabidx = :cmnt_tbl.comtabidx and
			comattid = :cmnt_tbl.comattid;
		    duve_talk( DUVE_MODESENS, duve_cb,
			      S_DU0441_DROP_THIS_COMMENT, 0);
		}
		continue;
	    } /* end test 4 */
	} /* endif this is the first time we've seen this col #, table id */

	/* test 5 - verify sequence number is correct */
	if ( cmnt_tbl.text_sequence == seq )
	    seq++;			/* all is well */
	else if (cmnt_tbl.text_sequence < seq)
	{
	    /* duplicate dbms comment sequence tuple */

	    if (duve_banner( DUVE_IIDBMS_COMMENT, 5, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU16A2_COMMENT_DUP_ERR, 8,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own,
			sizeof(cmnt_tbl.comattid), &cmnt_tbl.comattid,
			sizeof(seq), &seq);
	    if (duve_talk( DUVE_ASK, duve_cb, S_DU0241_DROP_THIS_COMMENT, 0)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iidbms_comment where
		    comtabbase = :cmnt_tbl.comtabbase and
		    comtabidx = :cmnt_tbl.comtabidx;
		duve_talk( DUVE_MODESENS, duve_cb,S_DU0441_DROP_THIS_COMMENT,0);
	    }
	    else
		seq = cmnt_tbl.text_sequence + 1;
	}
	else
	{
	    /* missing dbms comments sequence tuple */
	    if (duve_banner( DUVE_IIDBMS_COMMENT, 5, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU16A3_COMMENT_SEQ_ERR, 8,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own,
			sizeof(cmnt_tbl.comattid), &cmnt_tbl.comattid,
			sizeof(seq), &seq);
	    if (duve_talk( DUVE_ASK, duve_cb, S_DU0241_DROP_THIS_COMMENT, 0)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iidbms_comment where
		    comtabbase = :cmnt_tbl.comtabbase and
		    comtabidx = :cmnt_tbl.comtabidx;
		duve_talk( DUVE_MODESENS, duve_cb,S_DU0441_DROP_THIS_COMMENT,0);
	    }
	    else
		seq = cmnt_tbl.text_sequence + 1;
	}
    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckcomments");

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckdbdep - Perform system catalog checks/fixes on iidbdepends catalog
**
** Description:
**      this routine performs system catalog tests on iidbdpends 
**	system catalog, caching information that will be useful to tests
**	on iixdbdepends to enhance performance by reducing disk IOs.
**
**      It allocates the memory for the cache based on the number of tuples
**	currently in the iidbdepends table.  (This memory is deallocated by
**	duve_close when closing the database.) 
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	    duvedpnds			    cached iidbdepends information
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    views/permits may be dropped from database
**	    iirelation tuples may be modified
**
** History:
**      05-sep-88 (teg)
**          initial creation
**	31-oct-90 (teresa)
**	    change to stop doing iidbdepends checks if either deid or inid
**	    do not really exist.
**	2-nov-90 (teresa)
**	    fix bug where test 57 set TCB_VBASE instead of TCB_VIEW.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	16-jun-93 (anitap)
**	    Added test for FIPS constraints project. Added switch statement
**	    to include dependent objects like rules and procedures.
**	14-jul-93 (teresa)
**	    Fixed FIPS constraints tests for procedures and rules.  Cannot
**	    select count(deid1) from iirule and iiprocedure catalogs, as those
**	    catalogs do not contain column deid1.
**	06-dec-93 (andre)
**	    IIDBDEPENDS cache will contain both dependent and independent 
**	    object info (with independent object info list hanging off a 
**	    dependent object info element); before checking for existence of a 
**	    given dependent or independent object (by querying a catalog), we 
**	    will check whether the description of the object has been added to 
**	    the IIDBDEPENDS cache 
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	15-dec-93 (andre)
**	    iidbdepends cache will represent even those tuples which may have 
**	    been deleted because the independent (or dependent) object mentioned
**	    in them does not exist.
**	11-jan-94 (andre)
**	    added code to handle unexpected dependent/independent object types
**	 2-feb-1996 (nick)
**	    query to check if a reference db event exists was hosed ( in two
**	    places ). #74424
**	23-may-96 (nick)
**	    The query to check synonyms was hosed also ... #76673
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set bit has been removed ,
**          removeing references to TCB_VBASE here.
**      04-Aug-2009 (coomi01) b122349
**          Add DB_SEQUENCE as an object type, and check sequence dependency.
[@history_template@]...
*/
DU_STATUS
ckdbdep(duve_cb)
DUVE_CB		*duve_cb;
{
    /*
    ** independent and dependent may contain the following values:
    **  DUVE_BAD_TYPE	- tuple contained unexpected dependent/independent 
    **			  object type;  this means that either ckdbdpnds() needs
    **			  to be updated or that a catalog has been corrupted
    **	DUVE_DROP	- object is marked to be dropped
    **	DU_INVALID	- object does not exist
    **	DU_NOTKNOWN	- "dependent" will be set to this value if dependent
    **			  object D exists and is not marked for destruction and 
    **			  either D's description was found in the dependent 
    **			  object info list or D is not a view or index 
    **			  "independent" will be set to this value if the 
    **			  independent object I exists and is not marked for 
    **			  destruction and the dependent object is not a view and
    **			  either I's description was found in the independent 
    **			  object info list or I is not a table, view, or index
    **  non-negative #	- offset into duverel for a table/view/index
    */
    i4		independent;
    i4		dependent;

    u_i4		size;
    STATUS		mem_stat;
    bool		tuple_deleted;
        
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvedpnd.sh>;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i4	type,id;
	i4	relstat;
	char	obj_name[DB_MAXNAME + 1];
	char	sch_name[DB_MAXNAME + 1];
	i4 cnt;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* 
    ** allocate memory to hold IIDBDEPENDS cache
    */
    EXEC SQL select count(inid1) into :cnt from iidbdepends;
    if ( (sqlca.sqlcode == 100) || (cnt == 0))
    {	
	/* its unreasonable to have an empty iidbdepends table, as the system
	** catalogs include some views 
	*/
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU5022_EMPTY_IIDBDEPENDS, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }

    size = cnt * sizeof(DU_I_DPNDS);
    duve_cb->duvedpnds.duve_indep = 
	(DU_I_DPNDS *) MEreqmem(0, size, TRUE, &mem_stat);
    if (mem_stat != OK || duve_cb->duvedpnds.duve_indep == NULL)
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }
    duve_cb->duvedpnds.duve_indep_cnt = 0;

    /* 
    ** number of dependent objects described by iidbdepends will most likely 
    ** be lower than the total number of rows - to avoid allocating excessive 
    ** amounts of memory, we will separately compute number of distinct 
    ** dependent objects described in IIDBDEPENDS
    */

    exec sql declare global temporary table session.dbdep as 
	select distinct deid1, deid2, dtype, qid from iidbdepends
	on commit preserve rows with norecovery;

    exec sql inquire_sql(:cnt = rowcount);

    exec sql drop session.dbdep;

    size = cnt * sizeof(DU_D_DPNDS);
    duve_cb->duvedpnds.duve_dep = 
	(DU_D_DPNDS *) MEreqmem(0, size, TRUE, &mem_stat);
    if (mem_stat != OK || duve_cb->duvedpnds.duve_dep == NULL)
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }
    duve_cb->duvedpnds.duve_dep_cnt = 0;

    EXEC SQL DECLARE dpnd_cursor CURSOR FOR
	select * from iidbdepends;
    EXEC SQL open dpnd_cursor;
    
    /*
    ** (as of 12/06/93) IIDBDEPENDS is expected to describe the following types 
    ** of dependencies:
    **   - view depends on underlying objects
    **   - dbproc depends on underlying objects (tables/views, dbevents, other 
    **     dbprocs, synonyms)
    **   - permit depends on a table (non-GRANT compat. perms)
    **   - permit depends on a privilege descriptor contained in IIPRIV
    **   - rules created to enforce a constraint depend on that constraint
    **   - dbprocs created to enforce a constraint depend on that 
    **	   constraint
    **   - indices created to enforce a constraint depend on that constraint
    **   - REFERENCES constraints depend on UNIQUE constraints on the 
    **	   <referencing columns>
    **   - REFERENCES constraints depend on REFERENCES privilege on 
    **	   <referenced columns> (if the owner of the <referenced table> is
    **	   different from that of the <referencing table>)
    */

    /* loop for each tuple in iidbdepends */
    for (;;)  
    {
	DU_D_DPNDS	*dep;
	DU_I_DPNDS	*indep;
	DU_STATUS	dep_offset;
	DU_STATUS	indep_offset;

	/* get tuple from table */
	EXEC SQL FETCH dpnd_cursor INTO :dpnd_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in table */
	{
	    EXEC SQL CLOSE dpnd_cursor;
	    break;
	}
	
	tuple_deleted = FALSE;

	/* 
	** check whether we have already created a dependent object info 
	** element for this object;  if so, we will not populate another 
	** dependent object info element and can skip checking whether it 
	** exists, but will have to populate a new independent object info 
	** element and link it into the list associated with this dependent 
	** object; otherwise, we will create new dependent and independent 
	** object info elements and link them together + we will have to check 
	** whether the dependent object exists
	*/

	dep_offset = duve_d_dpndfind(dpnd_tbl.deid1, dpnd_tbl.deid2, 
	    dpnd_tbl.dtype, dpnd_tbl.qid, duve_cb);

	/*
	** if a dependent object info element for this object is yet to be 
	** created, we will check if the independent object info list contains 
	** its description;  we will NOT do it for views and indices since 
	** duverel contains descriptions of all views and indices and we might 
	** as well try to look it up there; for all other objects, a binary 
	** search through independent object info list may be far more 
	** efficient than a query
	*/
	if (   dep_offset == DU_INVALID
	    && dpnd_tbl.dtype != DB_VIEW
	    && dpnd_tbl.dtype != DB_INDEX)
	{
	    indep_offset = duve_i_dpndfind(dpnd_tbl.deid1, dpnd_tbl.deid2, 
		dpnd_tbl.dtype, dpnd_tbl.qid, duve_cb, (bool) FALSE);
	}
	else
	{
	    /* 
	    ** we haven't even tried to look for independent object - set 
	    ** indep_offset to DU_INVALID
	    */
	    indep_offset = DU_INVALID;
	}

	if (dep_offset != DU_INVALID)
	{
	    dep = duve_cb->duvedpnds.duve_dep + dep_offset;

	    /*
	    ** if this dependent object has been marked for destruction, 
	    **     set dependent to DUVE_DROP (this will result in us asking 
	    **     user whether he wants the tuple deleted, checking whether 
	    **	   the independent object exists and skipping remaining tests 
	    **	   on this tuple)
	    ** else if we have determined that the dependent object does not 
	    **         exist
	    **	   set dependent to DU_INVALID (this will result in an error 
	    **	   message and asking user whether he wants the tuple deleted)
	    ** else if the dependent object type was unexpected
	    **	   set dependent to DUVE_BAD_TYPE (this will result in an error
	    **	   message and asking user whether he wants the tuple deleted)
	    ** otherwise
	    **	   set "dependent" to DU_NOTKNOWN to indicate that the 
	    **	   description of dependent object was found in IIDBDEPENDS 
	    **	   cache; we will link the new independent object info element 
	    **	   (once it is populated) into the list associated with this 
	    **	   dependent object; 
	    */
	    if (dep->du_dflags & DU_DEP_OBJ_WILL_BE_DROPPED)
	    {
		dependent = DUVE_DROP;
	    }
	    else if (dep->du_dflags & DU_NONEXISTENT_DEP_OBJ)
	    {
		dependent = DU_INVALID;
	    }
	    else if (dep->du_dflags & DU_UNEXPECTED_DEP_TYPE)
	    {
		dependent = DUVE_BAD_TYPE;
	    }
	    else
	    {
		/*
		** set dependent to DU_NOTKNOWN to indicate that we looked up
		** a description of dependent object in IIDBDEPENDS cache
		*/
		dependent = DU_NOTKNOWN;
	    }
	}
	else
	{
	    /* populate a new dependent object info element */
	    dep = 
		duve_cb->duvedpnds.duve_dep + duve_cb->duvedpnds.duve_dep_cnt++;

	    dep->du_deid.db_tab_base  = dpnd_tbl.deid1;
	    dep->du_deid.db_tab_index = dpnd_tbl.deid2;
	    dep->du_dtype             = dpnd_tbl.dtype;
	    dep->du_dqid              = dpnd_tbl.qid;
	    dep->du_dflags 	      =	0;

	    if (indep_offset != DU_INVALID)
	    {
	        DU_I_DPNDS	    *existing_indep;
    
	        /*
	        ** if the existing independent object info element indicates 
		**        that this dependent object has been marked for 
		**        destruction, 
	        **     set dependent to DUVE_DROP (this will result in us asking
	        **     user whether he wants the tuple deleted, checking whether
	        **     the independent object exists and skipping remaining 
		**     tests on this tuple)
	        ** else if the existing independent object info element 
		**        indicates that this dependent object does not exist
	        **     set dependent to DU_INVALID (this will result in an 
		**     error message and asking user whether he wants the 
		**     tuple deleted)
	        ** otherwise
		**     if the dependent object type is one of those which we 
		**             expect
	        **         set "dependent" to DU_NOTKNOWN to indicate that the 
	        **         description of dependent object was found in 
		**	   IIDBDEPENDS cache; we will link the new independent 
		**	   object info element (once it is populated) into the 
		**	   list associated with this dependent object; 
		**     else
	        **	   set dependent to DUVE_BAD_TYPE (this will result in 
		**         an error message and asking user whether he wants 
		**	   the tuple deleted)
	        */
    
	        existing_indep = duve_cb->duvedpnds.duve_indep + indep_offset;
    
	        if (existing_indep->du_iflags & DU_INDEP_OBJ_WILL_BE_DROPPED)
	        {
		    dependent = DUVE_DROP;
	        }
	        else if (existing_indep->du_iflags & DU_NONEXISTENT_INDEP_OBJ)
	        {
		    dependent = DU_INVALID;
	        }
		else
		{
		    switch (existing_indep->du_itype)
		    {
			case DB_VIEW:
			case DB_INDEX:
			case DB_DBP:
			case DB_RULE:
			case DB_PROT:
			case DB_CONS:
			    /*
			    ** set dependent to DU_NOTKNOWN to indicate that we
			    ** looked up a description of dependent object in 
			    ** IIDBDEPENDS cache
			    */
			    dependent = DU_NOTKNOWN;
			    break;
			default:
			    dependent = DUVE_BAD_TYPE;
			    break;
		    }
		}
	    }
	    else
	    {
	        /* test 1 - verify dependent object really exists */
	        switch(dpnd_tbl.dtype)
                {
                    case DB_VIEW:
                    case DB_INDEX:
                        dependent = duve_findreltid(&dep->du_deid, duve_cb);
                        break;
    
                    case DB_DBP:
                        EXEC SQL REPEATED select any(dbp_id) into :cnt 
		            from iiprocedure
                            where 
				    dbp_id  = :dpnd_tbl.deid1 
			        and dbp_idx = :dpnd_tbl.deid2;
    
		       dependent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
                       break;
    
                    case DB_RULE:
                        EXEC SQL REPEATED select any(rule_id1) into :cnt 
		            from iirule
                            where 
				    rule_id1 = :dpnd_tbl.deid1 
			        and rule_id2 = :dpnd_tbl.deid2;
    
		       dependent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
	               break;
    	
		    case DB_PROT:
		        EXEC SQL REPEATED select any(1) into :cnt
			    from iiprotect 
			    where
				    protabbase = :dpnd_tbl.deid1 
			        and protabidx  = :dpnd_tbl.deid2
			        and propermid  = :dpnd_tbl.qid;
    
		        dependent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		        break;
	    	
		    case DB_CONS:
		        EXEC SQL REPEATED select any(1) into :cnt
			    from iiintegrity 
			    where
				    inttabbase = :dpnd_tbl.deid1
			        and inttabidx  = :dpnd_tbl.deid2
			        and intnumber  = :dpnd_tbl.qid
			        and consflags  != 0;
    
		        dependent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		        break;

	            default:
		         /* 
		         ** this means that either ckdbdep() code is out of 
			 ** date or the catalog got corrupted - we'll give 
			 ** ourselves a benefit of a doubt and blaim it on the 
			 ** catalog
		         */
		         dependent = DUVE_BAD_TYPE;
		         break;
                }
	    }
	}

        if (dependent == DUVE_DROP)
        {
	    /* 
	    ** dependent object has been marked for destruction - mark the 
	    ** dependent object info element accordingly
	    */
	    dep->du_dflags |= DU_DEP_OBJ_WILL_BE_DROPPED;
	}
        else if (dependent == DU_INVALID)
        {
	    char	obj_type[50];
	    i4		type_len;

	    /* 
	    ** dependent object does not exist - mark the dependent object info
	    ** element accordingly
	    */
	    dep->du_dflags |= DU_NONEXISTENT_DEP_OBJ;

            if (duve_banner( DUVE_IIDBDEPENDS, 1, duve_cb) == DUVE_BAD) 
	        return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_obj_type((i4) dpnd_tbl.dtype, obj_type, &type_len);

            duve_talk(DUVE_MODESENS, duve_cb, S_DU16C0_NONEXISTENT_DEP_OBJ, 10,
		       type_len, 	       obj_type,
		       sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
	               sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2,
		       sizeof(dpnd_tbl.dtype), &dpnd_tbl.dtype,
		       sizeof(dpnd_tbl.qid),   &dpnd_tbl.qid);
            if (duve_talk(DUVE_ASK, duve_cb, 
	                   S_DU0322_DROP_IIDBDEPENDS_TUPLE, 8, 
	                   sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
	                   sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
	                   sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
	                   sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2)
                == DUVE_YES)
            {
	        EXEC SQL delete from iidbdepends where current of dpnd_cursor;
	        duve_talk( DUVE_MODESENS, duve_cb, 
		        S_DU0372_DROP_IIDBDEPENDS_TUPLE, 8, 
		        sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
		        sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		        sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
		        sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2);

		/* remember that we have already deleted the current tuple */
		tuple_deleted = TRUE;
            }
        }
        else if (dependent == DUVE_BAD_TYPE)
        {
	    /* 
	    ** dependent object type found in the tuple was unexpected - mark 
	    ** the dependent object info element accordingly
	    */
	    dep->du_dflags |= DU_UNEXPECTED_DEP_TYPE;

            if (duve_banner( DUVE_IIDBDEPENDS, 1, duve_cb) == DUVE_BAD) 
	        return ( (DU_STATUS) DUVE_BAD);
	    
            duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU16C2_UNEXPECTED_DEP_OBJ_TYPE, 8,
		sizeof(dpnd_tbl.dtype), &dpnd_tbl.dtype,
		sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
	        sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2,
		sizeof(dpnd_tbl.qid),   &dpnd_tbl.qid);
            if (duve_talk(DUVE_ASK, duve_cb, 
	                   S_DU0322_DROP_IIDBDEPENDS_TUPLE, 8, 
	                   sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
	                   sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
	                   sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
	                   sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2)
                == DUVE_YES)
            {
	        EXEC SQL delete from iidbdepends where current of dpnd_cursor;
	        duve_talk( DUVE_MODESENS, duve_cb, 
		        S_DU0372_DROP_IIDBDEPENDS_TUPLE, 8, 
		        sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
		        sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		        sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
		        sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2);

		/* remember that we have already deleted the current tuple */
		tuple_deleted = TRUE;
            }
        }
	    
	/*
	** populate the next independent element (note that we will not 
	** increament duve_indep_cnt until we are done looking up a description
	** of independent object in the independent object info list - this 
	** ensures that we don't find this new element while searching for an 
	** existing element)
	*/
	indep = 
	    duve_cb->duvedpnds.duve_indep + duve_cb->duvedpnds.duve_indep_cnt;

	indep->du_inid.db_tab_base  = dpnd_tbl.inid1;
	indep->du_inid.db_tab_index = dpnd_tbl.inid2;
	indep->du_itype             = dpnd_tbl.itype;
	indep->du_iqid              = dpnd_tbl.i_qid;
	indep->du_iflags	    = 0;

	/* 
	** if we had to populate a new dependent object info element, link new 
	** dependent and independent object info elements; otherwise, link new 
	** independent element into a list associated with the existing 
	** dependent element
	*/
	if (dep_offset == DU_INVALID)
	{
	    dep->du_indep = indep;
	    indep->du_iflags |= DU_LAST_INDEP;
	    indep->du_next.du_dep = dep;
	}
	else
	{
	    indep->du_next.du_inext = dep->du_indep;
	    dep->du_indep = indep;
	}

	/* test 2 - verify independent object really exists */

	/*
	** if the dependent object is a view, we will look up independent object
	** description in duverel because test 3 expects "independent" to be an 
	** offset into IIRELATION cache;
	** otherwise, we will attempt to locate existing description of the 
	** independent object in the independent object info list
	** if that fails we will try to locate existing description of the
	** independent object in the dependent object info list unless the
	** independent object is a table, view, or index in which case a binary
	** search of duverel will be much more efficient
	*/
	if (dpnd_tbl.dtype == DB_VIEW)   
	{
	    indep_offset = dep_offset = DU_INVALID;
	}
	else
	{
	    /*
	    ** First try to look up the object description in the independent
	    ** object info list 
	    */

	    indep_offset = duve_i_dpndfind(dpnd_tbl.inid1, dpnd_tbl.inid2,
		dpnd_tbl.itype, dpnd_tbl.i_qid, duve_cb, (bool) FALSE);

	    if (indep_offset != DU_INVALID)
	    {
		DU_I_DPNDS          *existing_indep;

		/*
                ** if the existing independent object info element indicates
                **    that this independent object has been marked for
                **    destruction,
                **        set independent to DUVE_DROP (this will result in us
                **        skipping remaining tests on this tuple)
                ** else if the existing independent object info element
                **         indicates that this independent object does not exist
                **    set independent to DU_INVALID (this will result in an
                **    error message and asking user whether he wants the
                **    tuple deleted)
	        ** else if the independent object type was unexpected
	        **     set independent to DUVE_BAD_TYPE (this will result in an
		**     error message and asking user whether he wants the tuple
		**     deleted)
		** otherwise
		**    set independent to DU_NOTKNOWN to indicate that we  
		**    looked up a description of independent object in 
		**    IIDBDEPENDS cache
		*/

                existing_indep = duve_cb->duvedpnds.duve_indep + indep_offset;

                if (existing_indep->du_iflags & DU_INDEP_OBJ_WILL_BE_DROPPED)
                {
                    independent = DUVE_DROP;
                }
                else if (existing_indep->du_iflags & DU_NONEXISTENT_INDEP_OBJ)
                {
                    independent = DU_INVALID;
                }
	        else if (existing_indep->du_iflags & DU_UNEXPECTED_INDEP_TYPE)
	        {
		    independent = DUVE_BAD_TYPE;
	        }
	        else
	        {
		    /*
		    ** set independent to DU_NOTKNOWN to indicate that we 
		    ** looked up a description of independent object in 
		    ** IIDBDEPENDS cache
		    */
		    independent = DU_NOTKNOWN;
	        }

		/* 
		** set dep_offset to DU_INVALID to indicate that we haven't 
		** even tried to look up description of this independent object 
		** in the dependent object info list
		*/
		dep_offset = DU_INVALID;
	    }
	    else if (   dpnd_tbl.itype == DB_TABLE
	             || dpnd_tbl.itype == DB_VIEW
	             || dpnd_tbl.itype == DB_INDEX)
	    {
		/* 
		** binary search of duverel is more efficient than a scan of the
		** dependent object info list - don't bother calling 
		** duve_d_dpndfind() if independent object is a table, view, or 
		** index
		*/
		dep_offset = DU_INVALID;
	    }
	    else
	    {
		dep_offset = duve_d_dpndfind(dpnd_tbl.inid1, dpnd_tbl.inid2,
		    dpnd_tbl.itype, dpnd_tbl.i_qid, duve_cb);
		
		if (dep_offset != DU_INVALID)
		{
		    DU_D_DPNDS		*existing_dep;
    
                    /*
                    ** if the existing dependent object info element indicates
                    **    that this independent object has been marked for
                    **    destruction,
                    **        set independent to DUVE_DROP (this will result in
		    **	      us skipping remaining tests on this tuple)
                    ** else if the existing dependent object info element
                    **    indicates that this independent object does not exist
                    **        set independent to DU_INVALID (this will result in
		    **	      an error message and asking user whether he wants 
		    **	      the tuple deleted)
		    ** otherwise
		    **     if the independent object type is one of those which 
		    **             we expect
	            **         set "independent" to DU_NOTKNOWN to indicate that
		    **	       the description of independent object was found 
		    **         in IIDBDEPENDS cache
		    **     else
	            **	       set independent to DUVE_BAD_TYPE (this will 
		    **	       result in an error message and asking user 
		    **         whether he wants the tuple deleted)
                    */
    
                    existing_dep = duve_cb->duvedpnds.duve_dep + dep_offset;
    
                    if (existing_dep->du_dflags & DU_DEP_OBJ_WILL_BE_DROPPED)
                    {
                        independent = DUVE_DROP;
                    }
                    else if (existing_dep->du_dflags & DU_NONEXISTENT_DEP_OBJ)
                    {
                        independent = DU_INVALID;
                    }
	            else
	            {
			switch (existing_dep->du_dtype)
			{
			    case DB_TABLE:
			    case DB_VIEW:
			    case DB_INDEX:
			    case DB_EVENT:
			    case DB_SYNONYM:
			    case DB_PRIV_DESCR:
			    case DB_DBP:
			    case DB_CONS:
		                /*
		                ** set independent to DU_NOTKNOWN to indicate 
				** that we looked up a description of 
				** independent object in IIDBDEPENDS cache
		                */
		                independent = DU_NOTKNOWN;
				break;
			    default:
				independent = DUVE_BAD_TYPE;
				break;
			}
	            }
		}
	    }
	}

	/*
	** if we haven't found a description of the independent object in the 
	** IIDBDEPENDS cache, try to look elsewhere
	*/
	if (dep_offset == DU_INVALID && indep_offset == DU_INVALID)
	{
            switch(dpnd_tbl.itype)
            {
	        case DB_TABLE:
                case DB_VIEW:
                case DB_INDEX:
                    independent = duve_findreltid(&indep->du_inid, duve_cb);
                    break;
    
	        case DB_EVENT:
		    EXEC SQL REPEATED select any(1) into :cnt
		        from iievent
		        where
			        event_idbase = :dpnd_tbl.inid1
			    and event_idx    = :dpnd_tbl.inid2;
	    	
		    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		    break;
    
	        case DB_SYNONYM:
		    EXEC SQL REPEATED select any(1) into :cnt
		        from iisynonym
		        where
			        synid  = :dpnd_tbl.inid1
			    and synidx = :dpnd_tbl.inid2;
    
		    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		    break;
    
	        case DB_PRIV_DESCR:
		    EXEC SQL REPEATED select any(1) into :cnt
		        from iipriv
		        where
			        d_obj_id  = :dpnd_tbl.inid1
			    and d_obj_idx = :dpnd_tbl.inid2
			    and d_priv_number = :dpnd_tbl.i_qid;
	    	
		    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		    break;
    
                case DB_DBP:
                    EXEC SQL REPEATED select any(1) into :cnt 
		        from iiprocedure
                        where 
			        dbp_id  = :dpnd_tbl.inid1 
		            and dbp_idx = :dpnd_tbl.inid2;
    
		    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
                    break;
    
	        case DB_CONS:
	            EXEC SQL REPEATED select any(1) into :cnt
		        from iiintegrity 
		        where
		    	        inttabbase = :dpnd_tbl.inid1
		            and inttabidx  = :dpnd_tbl.inid2
		            and intnumber  = :dpnd_tbl.i_qid
		            and consflags  != 0;
        
		    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
		    break;
		    	
		case DB_SEQUENCE:
                    EXEC SQL REPEATED select any(1) into :cnt
                    from  iisequence
                    where seq_idbase = :dpnd_tbl.inid1
                    and   seq_idx    = :dpnd_tbl.inid2;

                    independent = (cnt == 0) ? DU_INVALID : DU_NOTKNOWN;
                    break;
		    	
	        default:
		    /* 
		    ** this means that either ckdbdep() code is out of 
		    ** date or the catalog got corrupted - we'll give 
		    ** ourselves a benefit of a doubt and blaim it on the 
		    ** catalog
		    */
		    independent = DUVE_BAD_TYPE;
		    break;
            }
	}

	/* 
	** we have populated the new independent object info element, but have 
	** not incremented duve_cb->duvedpnds.duve_indep_cnt to make sure that 
	** duve_i_dpndfind() does not accidently "find" it; now, however, is a 
	** good time to update duve_cb->duvedpnds.duve_indep_cnt
	*/
	duve_cb->duvedpnds.duve_indep_cnt++;

	if (independent == DUVE_DROP)
	{
	    /*
	    ** independent object has been marked for destruction - mark the
	    ** independent object info element accordingly + propagate this 
	    ** information into the dependent object info element
	    */
	    indep->du_iflags |= DU_INDEP_OBJ_WILL_BE_DROPPED;
	    dep->du_dflags |= DU_MISSING_INDEP_OBJ;

	}
	else if (independent == DU_INVALID)
	{
	    char	obj_type[50];
	    i4		type_len;

	    /*
	    ** independent object does not exist - mark the independent object 
	    ** info element accordingly + propagate this information into the
	    ** dependent object info element
	    */
	    indep->du_iflags |= DU_NONEXISTENT_INDEP_OBJ;
	    dep->du_dflags |= DU_MISSING_INDEP_OBJ;

	    if (duve_banner( DUVE_IIDBDEPENDS, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);

	    duve_obj_type((i4) dpnd_tbl.itype, obj_type, &type_len);

            duve_talk(DUVE_MODESENS, duve_cb, S_DU16C1_NONEXISTENT_INDEP_OBJ, 
		       10,
		       type_len, 	       obj_type,
		       sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
	               sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		       sizeof(dpnd_tbl.itype), &dpnd_tbl.itype,
		       sizeof(dpnd_tbl.i_qid), &dpnd_tbl.i_qid);

	    /* 
	    ** delete tuple unless it was already deleted because the dependent 
	    ** object does not exist
	    */
	    if (!tuple_deleted)
	    {
	        if (duve_talk( DUVE_ASK, duve_cb, 
			S_DU0322_DROP_IIDBDEPENDS_TUPLE, 8, 
			sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
		        sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		        sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
		        sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2)
	        == DUVE_YES)
	        {
		    EXEC SQL delete from iidbdepends 
			where current of dpnd_cursor;
		    duve_talk( DUVE_MODESENS, duve_cb, 
			    S_DU0372_DROP_IIDBDEPENDS_TUPLE, 8, 
			    sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
			    sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
			    sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
			    sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2);

		    /* remember that we deleted the current tuple */
		    tuple_deleted = TRUE;
	        }
	    }
	}
	else if (independent == DUVE_BAD_TYPE)
	{
	    /*
	    ** independent object type found in this tuple was unexpected - mark
	    ** the independent object info element accordingly + propagate this
	    ** information into the dependent object info element
	    */
	    indep->du_iflags |= DU_UNEXPECTED_INDEP_TYPE;
	    dep->du_dflags |= DU_MISSING_INDEP_OBJ;

	    if (duve_banner( DUVE_IIDBDEPENDS, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);

            duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU16C3_UNEXPECTED_IND_OBJ_TYPE, 8,
		   sizeof(dpnd_tbl.itype), &dpnd_tbl.itype,
		   sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
	           sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		   sizeof(dpnd_tbl.i_qid), &dpnd_tbl.i_qid);

	    /* 
	    ** delete tuple unless it was already deleted because the dependent 
	    ** object does not exist or was of unexpected type
	    */
	    if (!tuple_deleted)
	    {
	        if (duve_talk( DUVE_ASK, duve_cb, 
			S_DU0322_DROP_IIDBDEPENDS_TUPLE, 8, 
			sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
		        sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
		        sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
		        sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2)
	        == DUVE_YES)
	        {
		    EXEC SQL delete from iidbdepends 
			where current of dpnd_cursor;
		    duve_talk( DUVE_MODESENS, duve_cb, 
			    S_DU0372_DROP_IIDBDEPENDS_TUPLE, 8, 
			    sizeof(dpnd_tbl.inid1), &dpnd_tbl.inid1,
			    sizeof(dpnd_tbl.inid2), &dpnd_tbl.inid2,
			    sizeof(dpnd_tbl.deid1), &dpnd_tbl.deid1,
			    sizeof(dpnd_tbl.deid2), &dpnd_tbl.deid2);

		    /* remember that we deleted the current tuple */
		    tuple_deleted = TRUE;
	        }
	    }
	}

	/* 
	** skip the remaining tests if the dependent object does not exist, 
	** is slated to be dropped or is of unexpected type
	*/
	if (dep->du_dflags & 
		(  DU_DEP_OBJ_WILL_BE_DROPPED 
		 | DU_NONEXISTENT_DEP_OBJ
		 | DU_UNEXPECTED_DEP_TYPE))
	{
	    /* 
	    ** in the past we used to skip test 2 as well, but since we decided 
	    ** that IIDBDEPENDS will represent all IIDBDEPENDS tuples, I wanted
	    ** to perform test 2 in case the independent object did not exist
	    ** or was marked for destruction
	    */
	    continue;
	}

	/*
	** if the independent object does not exist, is marked for destruction,
	** or is of unexpected type, we will skip the remaining tests.  
	** If the dependent object is a view or an index, set du_tbldrop in 
	** duverel entry corresponding to it to DUVE_DROP to indicate that it 
	** should be dropped; otherwise add an entry to the "fixit" list 
	** describing the object to be dropped (given the time, I would modify 
	** code marking tables, views, and indices for destruction to also add 
	** the description to the "fixit" list and have that list be the only 
	** source of information about objects to be dropped - as it is I will 
	** settle for using duverel as a source of info about 
	** tables/views/indices to be dropped and the "fixit" list as a source 
	** of info on all other objects to be dropped)
	*/
	if (dep->du_dflags & DU_MISSING_INDEP_OBJ)
	{
	    /* 
	    ** if we got here, dependent object exists and has not yet been
	    ** marked for destruction
	    */
	    if (   dpnd_tbl.dtype == DB_VIEW 
		|| dpnd_tbl.dtype == DB_INDEX
		|| dpnd_tbl.dtype == DB_PROT)
	    {
		i4		tbl_offset;

	        /*
	        ** note that if IIDBDEPENDS indicates that a permit depends on a
	        ** privilege descriptor and the privilege descriptor is no 
		** longer there, rather than trying to drop the permit and 
		** launch the full-blown REVOKE processing, we will attempt to 
		** drop the object (a view) on which the permit was defined
	        */

		/*
		** if we looked up its description in the IIDBDEPENDS cache,
		** look it up in IIRELATION cache first
		*/
		if (dependent == DU_NOTKNOWN)
		{
		    tbl_offset = duve_findreltid(&dep->du_deid, duve_cb);
		}
		else
		{
		    tbl_offset = dependent;
		}

		if (tbl_offset == DU_INVALID || tbl_offset == DUVE_DROP)
		{
		    /* 
		    ** tbl_offset may get set to DU_INVALID if a view on which a
		    ** permit was defined no longer exists - this condition 
		    ** will be checked in ckprot();
		    ** tbl_offset may get set to DUVE_DROP if a view on which a
		    ** permit was defined was has been marked for destruction
		    */
		    continue;
		}

		if (dpnd_tbl.dtype == DB_PROT)
		{
		     duve_talk(DUVE_MODESENS, duve_cb, 
			 S_DU16C5_MISSING_PERM_INDEP_OBJ, 6,
			 sizeof(dpnd_tbl.qid), &dpnd_tbl.qid,
			 0, duve_cb->duverel->rel[tbl_offset].du_own,
			 0, duve_cb->duverel->rel[tbl_offset].du_tab);
		}
		else
		{
		    char    obj_type[50];
		    i4      type_len;

		    duve_obj_type((i4) dpnd_tbl.dtype, obj_type, &type_len);

		    duve_talk(DUVE_MODESENS, duve_cb,
			S_DU16C4_MISSING_INDEP_OBJ, 6,
			type_len, obj_type,
			0, duve_cb->duverel->rel[tbl_offset].du_own,
			0, duve_cb->duverel->rel[tbl_offset].du_tab);
		}

		if (duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
			0, duve_cb->duverel->rel[tbl_offset].du_tab,
			0, duve_cb->duverel->rel[tbl_offset].du_own)
		    == DUVE_YES)
		{
		    idxchk(duve_cb, dpnd_tbl.deid1, dpnd_tbl.deid2);
		    duve_cb->duverel->rel[tbl_offset].du_tbldrop = DUVE_DROP;
		    duve_talk( DUVE_MODESENS, duve_cb,
		        S_DU0352_DROP_TABLE, 4,
		        0, duve_cb->duverel->rel[tbl_offset].du_tab,
		        0, duve_cb->duverel->rel[tbl_offset].du_own);
    
		    dep->du_dflags |= DU_DEP_OBJ_WILL_BE_DROPPED;
		}
	    }
	    else if (dpnd_tbl.dtype == DB_RULE) 
	    { 
		i4         tbl_offset;
		DB_TAB_ID	rule_tab_id;

		EXEC SQL select rule_name, rule_owner, rule_tabbase, rule_tabidx
		    into :obj_name, :sch_name, :tid, :tidx
		    from iirule
		    where 
			    rule_id1 = :dpnd_tbl.deid1 
			and rule_id2 = :dpnd_tbl.deid2;

		rule_tab_id.db_tab_base = tid;
		rule_tab_id.db_tab_index = tidx;

		tbl_offset = duve_findreltid(&rule_tab_id, duve_cb);

		if (tbl_offset == DU_INVALID || tbl_offset == DUVE_DROP)
		{
		    /* 
		    ** tbl_offset may get set to DU_INVALID if a table on which
		    ** a rule was defined no longer exists - this condition 
		    ** will be checked in ckrule();
		    ** tbl_offset may get set to DUVE_DROP if a table on which a
		    ** rule was defined was has been marked for destruction
		    */
		    continue;
		}

		duve_talk(DUVE_MODESENS, duve_cb,
		    S_DU16C4_MISSING_INDEP_OBJ, 6,
		    sizeof("RULE") - 1, "RULE", 0, sch_name, 0, obj_name);

		if (duve_talk(DUVE_IO, duve_cb, S_DU0345_DROP_FROM_IIRULE, 4,
			0, obj_name, 0, sch_name)
		    == DUVE_YES)
		{
		    duve_talk(DUVE_MODESENS, duve_cb,
		        S_DU0395_DROP_FROM_IIRULE, 4,
		        0, obj_name, 0, sch_name);
    
		    dep->du_dflags |= DU_DEP_OBJ_WILL_BE_DROPPED;
		}
	    }
	    else if (dpnd_tbl.dtype == DB_DBP)
	    { 
		EXEC SQL select dbp_name, dbp_owner 
		    into :obj_name, :sch_name
		    from iiprocedure
		    where 
			    dbp_id 	= :dpnd_tbl.deid1 
			and dbp_idx = :dpnd_tbl.deid2;

		duve_talk(DUVE_MODESENS, duve_cb,
		    S_DU16C4_MISSING_INDEP_OBJ, 6,
		    sizeof("PROCEDURE") - 1, "PROCEDURE", 
		    0, sch_name, 0, obj_name);

		if (duve_talk(DUVE_IO, duve_cb, S_DU0251_MARK_DBP_DORMANT, 0)
		    == DUVE_YES)
		{
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU0451_MARKED_DBP_DORMANT, 0);
    
		    dep->du_dflags |= DU_DEP_OBJ_WILL_BE_DROPPED;
		}
	    }
	    else if (dpnd_tbl.dtype == DB_CONS)
	    { 
		i4         tbl_offset;
		DB_TAB_ID	cons_tab_id;

		EXEC SQL select consname into :obj_name
		    from iiintegrity
		    where 
			    inttabbase = :dpnd_tbl.deid1
			and inttabidx  = :dpnd_tbl.deid2
			and intnumber  = :dpnd_tbl.qid
			and consflags  != 0;

		cons_tab_id.db_tab_base = dpnd_tbl.deid1;
		cons_tab_id.db_tab_index = dpnd_tbl.deid2;

		tbl_offset = duve_findreltid(&cons_tab_id, duve_cb);

		if (tbl_offset == DU_INVALID || tbl_offset == DUVE_DROP)
		{
		    /* 
		    ** tbl_offset may get set to DU_INVALID if a table on which
		    ** a constraint was defined no longer exists - this 
		    ** condition will be checked in ckinteg();
		    ** tbl_offset may get set to DUVE_DROP if a table on which a
		    ** constraint was defined was has been marked for 
		    ** destruction
		    */
		    continue;
		}

		duve_talk(DUVE_MODESENS, duve_cb,
		    S_DU16C4_MISSING_INDEP_OBJ, 6,
		    sizeof("CONSTRAINT") - 1, "CONSTRAINT", 
		    0, duve_cb->duverel->rel[tbl_offset].du_own, 
		    0, obj_name);

		if (duve_talk(DUVE_IO, duve_cb, S_DU0247_DROP_CONSTRAINT, 4,
			0, duve_cb->duverel->rel[tbl_offset].du_own,
			0, obj_name)
		    == DUVE_YES)
		{
		    duve_talk(DUVE_MODESENS, duve_cb,
		        S_DU0447_DROP_CONSTRAINT, 4,
		        0, duve_cb->duverel->rel[tbl_offset].du_own,
			0, obj_name);
    
		    dep->du_dflags |= DU_DEP_OBJ_WILL_BE_DROPPED;
		}
	    }

	    if (   dep->du_dflags & DU_DEP_OBJ_WILL_BE_DROPPED
	        && dpnd_tbl.dtype != DB_VIEW 
		&& dpnd_tbl.dtype != DB_INDEX
		&& dpnd_tbl.dtype != DB_PROT)
	    {
		DUVE_DROP_OBJ		*drop_obj;

		/* 
		** user decided to drop the object - add its description to the 
		** "fixit" list (we don't do it if the independent object was 
		** a view, index, or permit because in those cases we use the 
		** iirelation cache to store that information)
		*/

	        drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
	        if (mem_stat != OK || drop_obj == NULL)
	        {
		    duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
		    return ( (DU_STATUS) DUVE_BAD);
	        }

		drop_obj->duve_obj_id.db_tab_base  = dpnd_tbl.deid1;
		drop_obj->duve_obj_id.db_tab_index = dpnd_tbl.deid2;
		drop_obj->duve_obj_type            = dpnd_tbl.dtype;
		drop_obj->duve_secondary_id	   = dpnd_tbl.qid;
		drop_obj->duve_drop_flags	   = 0;

		/* link this description into the existing list */
		drop_obj->duve_next = duve_cb->duvefixit.duve_objs_to_drop;
		duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
	    }

	    continue;
	}

	/* 
	** a couple of tests if the tuple described dependence of a view on some
	** object used in its definition
	*/
	if (dpnd_tbl.dtype == DB_VIEW)
	{
	    /* test 3 - verify independent table knows its a view base.
	    ** Removed this test.  TCB_VBASE is no longer set when a 
	    ** view is created.  b111211
	    */

	    /* 
	    ** test 4 - verify dependent view is really a view - this test need
	    ** to be run only when we first populate a dependent object info 
	    ** element 
	    */
	    if (   dependent != DU_NOTKNOWN 
	        && (duve_cb->duverel->rel[dependent].du_stat & TCB_VIEW) == 0)
	    {
	        if (duve_banner( DUVE_IIDBDEPENDS, 4, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS)DUVE_BAD);

	        duve_talk(DUVE_MODESENS, duve_cb, S_DU1643_DEPENDENT_NOT_VIEW, 
		    4,
		    0,duve_cb->duverel->rel[dependent].du_tab,
		    0,duve_cb->duverel->rel[dependent].du_own);
	        if (duve_talk( DUVE_IO, duve_cb, S_DU031D_SET_VIEW, 0) 
			== DUVE_YES)
	        {
		    relstat = 
			duve_cb->duverel->rel[dependent].du_stat | TCB_VIEW;
		    basetid = dpnd_tbl.deid1;
		    basetidx = dpnd_tbl.deid2;
		    duve_cb->duverel->rel[dependent].du_stat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat 
			where reltid = :basetid and reltidx = :basetidx;
		    duve_talk(DUVE_MODESENS, duve_cb,S_DU036D_SET_VIEW, 0);
		}
	    } /* end test 4 */
	} /* end tests run if the dependent object is a view */

	/* test 5 - verify there is an entry for this table in index table */

	id =	dpnd_tbl.qid;
	type =	dpnd_tbl.dtype;
	tid =	dpnd_tbl.deid1;
	tidx =	dpnd_tbl.deid2;

	exec sql REPEATED select any(deid1) into :cnt from iixdbdepends where
	    deid1 = :tid and deid2 = :tidx and dtype = :type and qid = :id;

	if (cnt==0)
	{  /* The entry for iidbdepends is missing from iixdbdepends */
	    
	    if (duve_banner( DUVE_IIDBDEPENDS, 5, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1644_MISSING_IIXDBDEPENDS, 8,
		       0, duve_cb->duverel->rel[independent].du_tab,
		       0, duve_cb->duverel->rel[independent].du_own,
		       0, duve_cb->duverel->rel[dependent].du_tab,
		       0, duve_cb->duverel->rel[dependent].du_own);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU0324_CREATE_IIXDBDEPENDS, 0)
	       == DUVE_YES )
	    {
		tid  = dpnd_tbl.deid1;
		tidx = dpnd_tbl.deid2;
		type = dpnd_tbl.dtype;
		id   = dpnd_tbl.qid;
		/* Note although tid is i8, tidp is still i4 */
		exec sql select int4(tid) into :cnt from iidbdepends where
		    deid1 = :tid and deid2 = :tidx and dtype = :type and
		    qid = :id;
		exec sql insert 
		    into iixdbdepends(deid1,deid2,dtype,qid,tid)
		    values (:tid, :tidx, :type, :id, :cnt);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU0374_CREATE_IIXDBDEPENDS,
			  0);
	    }

	} /* end test 5 */

    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckdbdep"); 

    return ( (DU_STATUS) DUVE_YES);
}


/*{
** Name: ckxevent - run verifydb check on iixevent dbms catalog
**
** Description:
**	This function will ensure that IIXEVENT is in sync with IIEVENT.  This 
**	will be accomplished in two steps:
**	- first we will open a cursor to read tuples in iixevent for which 
**	  there are no corresponding tuples in iievent; if one or more such
**	  tuples are found, we will ask for permission to drop and recreate the 
**	  index
**	- then we will open a cursor on iievent to read tuples for which there 
**	  is no corresponding iixevent tuple and insert appropriate iixevent 
**	  tuples
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**	none.
**
** Returns:
**    	DUVE_YES
**      DUVE_BAD
**
** Exceptions:
**	none
**
** Side Effects:
**	tuples may be inserted into / deleted from iixevent
**
** History:
**      19-jan-94 (andre)
**	    written
[@history_template@]...
*/
DU_STATUS
ckxevent(
	DUVE_CB		*duve_cb)
{
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvexev.sh>;
	u_i4		tid_ptr;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE xev_tst_curs_1 CURSOR FOR
	select * from iixevent xev where not exists 
	    (select 1 from iievent ev 
	    	where
		        xev.tidp 		= ev.tid
		    and xev.event_idbase 	= ev.event_idbase
		    and xev.event_idx 		= ev.event_idx);
    
    EXEC SQL open xev_tst_curs_1;

    /*
    ** test 1: for every IIXEVENT tuple for which there is no corresponding 
    ** IIEVENT tuple ask user whether we may delete the offending IIXEVENT
    ** tuple
    */
    for (;;)
    {
        EXEC SQL FETCH xev_tst_curs_1 INTO :xev_tbl;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xev_tst_curs_1;
	    break;
        }

	/* there is no tuple in IIEVENT corresponding to this IIXEVENT tuple */
        if (duve_banner(DUVE_IIXEVENT, 1, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16C9_EXTRA_IIXEVENT, 6,
            sizeof(xev_tbl.event_idbase), &xev_tbl.event_idbase,
	    sizeof(xev_tbl.event_idx), &xev_tbl.event_idx,
	    sizeof(xev_tbl.tidp), &xev_tbl.tidp);
	if (duve_talk( DUVE_ASK, duve_cb, S_DU024A_DELETE_IIXEVENT, 0) 
	    == DUVE_YES)
	{
	    exec sql delete from iixevent where current of xev_tst_curs_1;
			
	    duve_talk(DUVE_MODESENS,duve_cb, S_DU044A_DELETED_IIXEVENT, 0);
	    
	    break;
	}
    }

    /* end test 1 */

    EXEC SQL DECLARE xev_tst_curs_2 CURSOR FOR
	select ev.*, ev.tid from iievent ev where not exists
	    (select 1 from iixevent xev
		where
			xev.tidp                = ev.tid
		    and xev.event_idbase        = ev.event_idbase
		    and xev.event_idx           = ev.event_idx
		    /* 
		    ** this last comparison ensures that we get the qualifying 
		    ** rows - otherwise (I suspect) OPF chooses to join index 
		    ** to itself and finds nothing
		    */
		    and event_name		= event_name);

    EXEC SQL open xev_tst_curs_2 FOR READONLY;

    /*
    ** test 2: for every IIEVENT tuple for which there is no corresponding 
    ** IIXEVENT tuple ask user whether we may insert a missing tuple into 
    ** IIXEVENT
    */
    for (;;)
    {
        EXEC SQL FETCH xev_tst_curs_2 INTO :evnt_tbl, :tid_ptr;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xev_tst_curs_2;
	    break;
        }

	/* there is no tuple in IIXEVENT corresponding to this IIEVENT tuple */
        if (duve_banner(DUVE_IIXEVENT, 2, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16CA_MISSING_IIXEVENT, 6,
	    sizeof(evnt_tbl.event_idbase), &evnt_tbl.event_idbase,
	    sizeof(evnt_tbl.event_idx), &evnt_tbl.event_idx,
	    sizeof(tid_ptr), &tid_ptr);
	if (duve_talk( DUVE_ASK, duve_cb, S_DU024B_INSERT_IIXEVENT_TUPLE, 0) 
	    == DUVE_YES)
	{
	    exec sql insert into 
		iixevent(event_idbase, event_idx, tidp)
		values (:evnt_tbl.event_idbase, :evnt_tbl.event_idx, :tid_ptr);
			
	    duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU044B_INSERTED_IIXEVENT_TUPLE, 0);
	}
    }

    /* end test 2 */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckxevent"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckevent - run verifydb tests to check system catalog iievent.
**
** Description:
**      This routine opens a cursor to walk iievent one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iievent system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iievent.
**
** History:
**      27-may-1993 (teresa)
**          initial creation
**	16-jun-93 (anitap)
**	    Added test for CREATE SCHEMA project.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (anitap)
**	    Use repeat query in test 3 and ANY instead of COUNT to improve
**	    performance. Also cache the schema names to be created.
**	07-jan-94 (teresa)
**	    if there are no tuples in iievent, then dont open the cursor.
**	28-jan-94 (andre)
**	    if the dbevent's query text is missing (test #1), rather than 
**	    deleting the IIEVENT tuple, add a record to the "fix it" list 
**	    reminding us to drop description of the dbevent and mark dormant 
**	    any dbprocs that depend on it.
[@history_template@]...
*/
DU_STATUS
ckevent(duve_cb)
DUVE_CB            *duve_cb;
{

    DB_TAB_ID		event_id;
    DU_STATUS		base;
    u_i4		size;
    STATUS		mem_stat;

    EXEC SQL BEGIN DECLARE SECTION;
	i4 cnt;
	i2	mode;
        char	schname[DB_MAXNAME + 1];	
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory to hold add commands */	
    EXEC SQL select count(event_name) into :cnt from iievent;

    duve_cb->duve_ecnt = 0;

    if (cnt == 0)
	return DUVE_YES;
    else	
        size = cnt * sizeof(DU_EVENT);

    duve_cb->duveeves = (DUVE_EVENT *) MEreqmem(0, size, TRUE, 
			&mem_stat);

    if ( (mem_stat != OK) || (duve_cb->duveeves == 0))
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
        return( (DU_STATUS) DUVE_BAD);
    }	

    EXEC SQL DECLARE event_cursor CURSOR FOR
	select * from iievent;
    EXEC SQL open event_cursor;
		
    /* loop for each tuple in iievent */
    for (;;)  
    {
	/* get tuple from iievent */
	EXEC SQL FETCH event_cursor INTO :evnt_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iievent */
	{
	    EXEC SQL CLOSE event_cursor;
	    break;
	}

	event_id.db_tab_base = evnt_tbl.event_idbase;
	event_id.db_tab_index = evnt_tbl.event_idx;

	/* test 1 - verify query text used to define event exists */

	mode = DB_EVENT;
	EXEC SQL repeated select any(txtid1) into :cnt from iiqrytext where
	        txtid1 = :evnt_tbl.event_qryid1
		and txtid2 = :evnt_tbl.event_qryid2
		and mode = :mode;
	if (cnt == 0)
	{
	    /* missing query text that defined dbevent */
            if (duve_banner( DUVE_IIEVENT, 1, duve_cb)
	        == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
		
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU167E_NO_TXT_FOR_EVENT, 4,
			0, evnt_tbl.event_name, 0, evnt_tbl.event_owner);
	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0343_DROP_EVENT, 0)
	    == DUVE_YES)
	    {
		DUVE_DROP_OBJ	*drop_obj;

		drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		if (mem_stat != OK || drop_obj == NULL)
		{
		    duve_talk (DUVE_ALWAYS, duve_cb, 
			E_DU3400_BAD_MEM_ALLOC, 0);
		    return ((DU_STATUS) DUVE_BAD);
		}

		/*
		** add a record to the "fix it" list reminding us to drop this 
		** dbevent 
		*/
		drop_obj->duve_obj_id.db_tab_base  = evnt_tbl.event_idbase;
		drop_obj->duve_obj_id.db_tab_index = evnt_tbl.event_idx;
		drop_obj->duve_obj_type            = DB_EVENT;
		drop_obj->duve_secondary_id	   = 0;
		drop_obj->duve_drop_flags	   = 0;

		/* link this description into the existing list */
		drop_obj->duve_next = 
		    duve_cb->duvefixit.duve_objs_to_drop;

		duve_cb->duvefixit.duve_objs_to_drop = drop_obj;

		duve_talk(DUVE_MODESENS, duve_cb, S_DU0393_DROP_EVENT, 0);
	    }
	    continue;
	} /* endif event definition does not exist */	    

	/* test 2 -- verify event identifier is unique */
	base = duve_findreltid ( &event_id, duve_cb);
	if (base != DU_INVALID)
	{
	    /* this id is in iirelation, so drop from iievent */

	    if (duve_banner( DUVE_IIEVENT, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU167F_EVENTID_NOT_UNIQUE, 8,
			0, evnt_tbl.event_name, 0, evnt_tbl.event_owner,
			sizeof(evnt_tbl.event_idbase), &evnt_tbl.event_idbase,
			sizeof(evnt_tbl.event_idx), &evnt_tbl.event_idx);
	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0343_DROP_EVENT, 0)
	    == DUVE_YES)
	    {
		exec sql delete from iiqrytext
		    where txtid1 = :evnt_tbl.event_qryid1 and
			  txtid2 = :evnt_tbl.event_qryid2;
		/*
		** FIX_ME_ANDRE - instead of deleting the tuple, we need to
		** either add an entry to a drop'rm list or find all independent
		** descriptors referring to this dbevent and mark them
		** accordingly
		*/
		EXEC SQL delete from iievent where current of event_cursor;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU0393_DROP_EVENT, 0);
	    }
	    continue;
	} /* endif identifier is not unique */


	/* test 3 -- verify event belongs to a schema */

        /* first, take 32 char event owner and force into
        ** string DB_MAXNAME char long.
        */
        MEcopy( (PTR)evnt_tbl.event_owner, DB_MAXNAME, (PTR)schname);
        schname[DB_MAXNAME] ='\0';
        (void) STtrmwhite(schname);

        /* now search for iischema tuple for owner match */
        EXEC SQL repeated select any(schema_id) into :cnt from iischema
 		where schema_name = :schname;

        if (cnt == 0)
        {
           /* corresponding entry missing in iischema */

           if (duve_banner(DUVE_IIEVENT, 3, duve_cb)
                   == DUVE_BAD)
              return ( (DU_STATUS) DUVE_BAD);
           duve_talk (DUVE_MODESENS, duve_cb, S_DU168D_SCHEMA_MISSING, 2,
                       0, evnt_tbl.event_owner);
	   
	   if ( duve_talk(DUVE_IO, duve_cb, S_DU0245_CREATE_SCHEMA, 0)
			== DUVE_YES)
	   {	

              /* cache info about this tuple */
              MEcopy( (PTR)evnt_tbl.event_owner, DB_MAXNAME,
           	(PTR) duve_cb->duveeves->events[duve_cb->duve_ecnt].du_schname);

	      duve_cb->duve_ecnt++;

              duve_talk(DUVE_MODESENS, duve_cb, S_DU0445_CREATE_SCHEMA, 0);
	   }

	   continue;
        } /* end test 3 */

    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckevent"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckgw06att - Perform verifydb checks/repairs on iigw06_attribute
**
** Description:
**	This routine opens a cursor on iigw06_attribute and walks the table,
**	one tuple at a time, performing verifydb checks and repairs on it.
**
**	NOTE: This routine should be run before ckgw06rel() since it may
**	remove tuples that subsequently make a row in iigw06_relation invalid
**	e.g. a table no longer has any attributes. ckgwo6rel() will check for
**	this and delete the row. See also note in ckgw06rel().
**
** Inputs:
**	duve_cb			verifydb control block
**
** Outputs:
**	duve_cb			verifydb control block
**
**	Returns:
**	    DUVE_BAD
**	    DUVE_YES
**	Exceptions:
**	    None.
**
** Side effects:
**	May delete tuples in iigw06_attribute, and iiattribute after prompting
**	user for confirmation.
**
** History:
**	30-jul-93 (stephenb)
**	    Initial creation
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
[@history_template@]
*/
DU_STATUS
ckgw06att (DUVE_CB *duve_cb)
{
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvegwat.sh>;
	i4 cnt = 0;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE gwatt_cursor CURSOR FOR
	select * from iigw06_attribute;
    EXEC SQL OPEN gwatt_cursor;

    /* loop for each tuple in iigw06_attribute */
    for (;;)
    {
	/* get tuple from iigw06_attribute */
	EXEC SQL FETCH gwatt_cursor INTO :gwatt_tbl;
	if (sqlca.sqlcode == 100) /* no more tuples */
	{
	    EXEC SQL CLOSE gwatt_cursor;
	    break;
	}
	/*
	** Test 1 -- Find a tuple in iigw06_relation.
	*/
	EXEC SQL repeated SELECT any(reltid) INTO :cnt FROM iigw06_relation
	    WHERE reltid = :gwatt_tbl.reltid;
	if (cnt == 0) /* no iigw06_relation */
	{
	    if (duve_banner (DUVE_IIGW06_ATTRIBUTE, 1, duve_cb)
	    == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU169A_MISSING_GW06REL, 2,
		0, gwatt_tbl.attname);
	    if (duve_talk (DUVE_IO, duve_cb, S_DU034E_DROP_IIGW06_ATTRIBUTE, 0)
	    == DUVE_YES)
	    {
		/*
		** Fisrt delete row in iiattribute, iigw06_attribute
		*/
		EXEC SQL DELETE FROM iiattribute 
		    WHERE attrelid = :gwatt_tbl.reltid
		      AND attid = :gwatt_tbl.attid;
		EXEC SQL DELETE FROM iigw06_attribute
		    WHERE CURRENT OF gwatt_cursor;
		duve_talk (DUVE_MODESENS, duve_cb, 
		    S_DU039E_DROP_IIGW06_ATTRIBUTE, 0);
	    }
	    continue;
	}
	/*
	** Test 2 -- Find a tuple in iiattribute.
	*/
	EXEC SQL repeated SELECT any(attrelid) INTO :cnt FROM iiattribute
	    WHERE attrelid = :gwatt_tbl.reltid
	    AND attid = :gwatt_tbl.attid;
	if (cnt == 0) /* no iiattribute */
	{
	    if (duve_banner (DUVE_IIGW06_ATTRIBUTE, 2, duve_cb)
	    == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU169B_MISSING_IIATTRIBUTE,
		2, 0, gwatt_tbl.attname);
	    if (duve_talk (DUVE_IO, duve_cb, S_DU034E_DROP_IIGW06_ATTRIBUTE, 0)
	    == DUVE_YES)
	    {
		/*
		** delete row from iigw06_attribute
		*/
		EXEC SQL DELETE FROM iigw06_attribute
		    WHERE CURRENT OF gwatt_cursor;
		duve_talk (DUVE_MODESENS, duve_cb, 
		    S_DU039E_DROP_IIGW06_ATTRIBUTE, 0);
	    }
	    continue;
	}
	/*
	** Test 3 -- check that reltidx=0 (indexes are not supported)
	*/
	if (gwatt_tbl.reltidx != 0)
	{
	    if (duve_banner (DUVE_IIGW06_ATTRIBUTE, 3, duve_cb) == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU169C_NONZERO_GW06ATTIDX,
		2, 0, gwatt_tbl.attname);
	    if (duve_talk (DUVE_IO, duve_cb, S_DU034E_DROP_IIGW06_ATTRIBUTE, 0)
	    == DUVE_YES)
	    {
		/*
		** first delete from iiattribute then iigw06_attribute.
		*/
		EXEC SQL DELETE FROM iiattribute
		    WHERE attrelid = :gwatt_tbl.reltid
		      AND attrelidx = :gwatt_tbl.reltidx
		      AND attid = :gwatt_tbl.attid;
		EXEC SQL DELETE FROM iigw06_attribute
		    WHERE CURRENT OF gwatt_cursor;
		duve_talk ( DUVE_MODESENS, duve_cb, 
		    S_DU039E_DROP_IIGW06_ATTRIBUTE, 0);
	    }
	} /* end of test 3 */
    } /* end of loop for each row in iigw06_attribute */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckgw06att"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckgw06rel - Perform verifydb checks/repairs on iigw06_relation
** 
** Description:
**	This routine opens a cursor on iigw06_relation and walks the table,
**	one tuple at a time, performing verifydb checks and repairs on it.
**
**	NOTE: the tables iigw06_relation and iigw06_attribute are fully
**	dependent on eachother and on iirelation and iiattribute, i.e. a
**	row in iigw06_relation must have a corresponding row in iirelation
**	and a row in iigw06_attribute must have a corresponding row in
**	iiattribute and in iigw06_relation and in iirelation (by inference)
**	so removing rows from any of these tables means that we need to perform
**	cascading deletes to maintain consistency, we can't always rely on
**	DROP REGISTRATION since the kind of corruption we are checking for
**	may prevent this statement from working properly, so we have to perform
**	all the deletes by hand. Currently tables are joined only on
**	reltid, since reltidx is always zero (there is a check for this)
**	reltid provides a unique key, this should be changed if the
**	code is ever updated to support indexes.
**
** Inputs:
**	duve_cb				verifydb control block
**
** Outputs:
**	duve_cb				verifydb control block
**
**	Returns:
**	    DUVE_BAD
**	    DUVE_YES
**	Exceptions:
**	    None
**
** Side Effects:
**	May delete rows in iirelation, iigw06_relation, iiattribute and 
**	iigw06_attribute after prompting for user confirmation.
**
** History
**	27-jul-93 (stephenb)
**	    Initial creation
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
[@history_template@]
*/
DU_STATUS
ckgw06rel(DUVE_CB *duve_cb)
{
    i4 ctr = 0;
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvegwrl.sh>;
	i4	cnt = 0;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE gwrel_cursor CURSOR FOR
	select * from iigw06_relation;
    EXEC SQL OPEN gwrel_cursor;

    /* loop for each tuple in iigw06_relation */
    for (;;)
    {
	/* get tuple from iigw06_relation */
	EXEC SQL FETCH gwrel_cursor INTO :gwrel_tbl;
	if (sqlca.sqlcode == 100) /* no more tuples */
	{
	    EXEC SQL CLOSE gwrel_cursor;
	    break;
	}
	/*
	** Test 1 -- Verify that a correspondig tuple exists in iirelation
	*/
	/* traverse the array containg iirelation tuples looking for a 
	** value that matches, break out when found
	*/
	for (ctr = 0; ctr < duve_cb->duve_cnt; ctr++)
	{
	    if (duve_cb->duverel->rel[ctr].du_id.db_tab_base 
		== gwrel_tbl.reltid) /* found a tuple that corresponds */
		break;
	}
	if (ctr == duve_cb->duve_cnt) /* no value found */
	{
	    if (duve_banner( DUVE_IIGW06_RELATION, 1, duve_cb) == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1697_MISSING_RELTUP, 2,
		0, gwrel_tbl.audit_log);
	    if ( duve_talk(DUVE_IO, duve_cb, S_DU034D_DROP_IIGW06_RELATION, 0)
	    == DUVE_YES)
	    {
		/* 
		** first delete rows in iigw06_attribute and then
		** iiattribute and finaly the offendig row in iigw06_relation
		*/
		EXEC SQL DELETE FROM iigw06_attribute 
		    WHERE reltid = :gwrel_tbl.reltid;
		EXEC SQL DELETE FROM iiattribute
		    WHERE attrelid = :gwrel_tbl.reltid;
		EXEC SQL DELETE FROM iigw06_relation WHERE 
		    CURRENT OF gwrel_cursor;
		duve_talk (DUVE_MODESENS, duve_cb, 
		    S_DU039D_DROP_IIGW06_RELATION, 0);
	    }
	    continue;
	}
	/*
	** Test 2 -- Verify there is at least one corresponding attribute
	**           in iigw06_attribute.
	*/
	EXEC SQL repeated SELECT any(reltid) INTO :cnt FROM iigw06_attribute
	    WHERE reltid = :gwrel_tbl.reltid;
	if (cnt == 0) /* no iigw06_attribute */
	{
	    if (duve_banner (DUVE_IIGW06_RELATION, 2, duve_cb)
	    == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1698_MISSING_GW06ATT, 4,
		0, duve_cb->duverel->rel[ctr].du_tab, 0, gwrel_tbl.audit_log);
	    if (duve_talk( DUVE_IO, duve_cb, S_DU034D_DROP_IIGW06_RELATION, 0)
	    == DUVE_YES)
	    {
		/* 
		** First delete from iiattribute, then iirelation and finaly
		** from iigw06_relation
		*/
		EXEC SQL DELETE FROM iiattribute
		    WHERE attrelid = :gwrel_tbl.reltid;
		EXEC SQL DELETE FROM iirelation
		    WHERE reltid = :gwrel_tbl.reltid;
		EXEC SQL DELETE FROM iigw06_relation 
		    WHERE CURRENT OF gwrel_cursor; 
		duve_talk (DUVE_MODESENS, duve_cb, 
		    S_DU039D_DROP_IIGW06_RELATION, 0);
	    }
	    continue;
	}
	/*
	** Test 3 -- Verify that the value in reltidx=0 (we do not support
	** indexes in this gateway
	*/
	if (gwrel_tbl.reltidx != 0)
	{
	    if (duve_banner ( DUVE_IIGW06_RELATION, 3, duve_cb)
	    == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU1699_NONZERO_GW06IDX, 4,
		0, duve_cb->duverel->rel[ctr].du_tab, 0, gwrel_tbl.audit_log);
	    if (duve_talk( DUVE_IO, duve_cb, S_DU034D_DROP_IIGW06_RELATION, 0)
	    == DUVE_YES)
	    {
	    /*
	    ** first delete corresponding iigw06_attribue then iiattribute
	    ** then iirelation and finally iigw06_relation
	    */
	    	EXEC SQL DELETE FROM iigw06_attribute
		    WHERE reltid = :gwrel_tbl.reltid
		      AND reltidx = :gwrel_tbl.reltidx;
	    	EXEC SQL DELETE FROM iiattribute
		    WHERE attrelid = :gwrel_tbl.reltid
		      AND attrelidx = :gwrel_tbl.reltidx;
	    	EXEC SQL DELETE FROM iirelation
		    WHERE reltid = :gwrel_tbl.reltid
		      AND reltidx = :gwrel_tbl.reltidx;
	    	EXEC SQL DELETE FROM iigw06_relation 
		    WHERE CURRENT OF gwrel_cursor;
		duve_talk (DUVE_MODESENS, duve_cb, 
		    S_DU039D_DROP_IIGW06_RELATION, 0);
	    }
	} /* end of test 3 */
    } /* end of loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckgw06rel"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckhist	- perform verifydb checks/repairs on iihistogram
**
** Description:
**      This routine opens a cursor on iihistogram and walks the table,
**	one tuple at a time, performing verifydb checks and repairs on it. 
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**	duve_cb				verifydb control block
**	    duvestat			cache of statistics entries
**		stats[]			statistics cache entry
**
**	Returns:
**	    DUVE_BAD
**	    DUVE_YES
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      nn-nn-nn (teresa)
**          Initial creation
**	15-jul-93 (teresa)
**	    Fix MEfree statement -- was passing address of duve_cb->duvedpnds
**	    instead of just duve_cb->duvedpnds.
**      18-oct-93 (teresa)
**          fix bug 50840 -- was releasing the wrong memory (duve_cb->duvedpnds
**          instead of duve_cb->duvestat).  This caused an AV on UNIX but
**          appeared to slip by on VMS.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	05-apr-1996 (canor01)
**	    after freeing duve_cb->duvestat, set it to NULL to prevent it
**	    from being freed again later.
**
[@history_template@]...
*/
DU_STATUS
ckhist(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_TAB_ID	   histid;
    DU_STATUS	   base,ptr;
    i4	   seq;
    i2		   hist_att;
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvehist.sh>;
	u_i4	tid, tidx;
	i2	att;
	i2	cells;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if ( duve_cb ->duve_scnt == 0)
    {
	/* there were no statistics, so there should be no tuples */
	EXEC SQL select any(htabbase) into :att from iihistogram;
	if (att != 0)
	{
	    if (duve_banner( DUVE_IIHISTOGRAM, 1, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU1651_INVALID_IIHISTOGRAM, 0);
	    if (duve_talk (DUVE_ASK, duve_cb, S_DU032C_EMPTY_IIHISTOGRAM, 0)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iihistogram;
		duve_talk(DUVE_MODESENS, duve_cb,S_DU037C_EMPTY_IIHISTOGRAM, 0);
	    }
	}
	return ( (DU_STATUS) DUVE_YES);
    } /* endif iistatistics is empty */


    EXEC SQL DECLARE hist_cursor CURSOR FOR
	select * from iihistogram order by htabbase, htabindex, hatno,hsequence;
    EXEC SQL open hist_cursor;

    /* loop for each tuple in iihistogram */
    for (histid.db_tab_base= -1,histid.db_tab_index= -1,base= -1;;)
    {
	/* get tuple from table */
	EXEC SQL FETCH hist_cursor INTO :hist_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in table */
	{
	    EXEC SQL CLOSE hist_cursor;
	    break;
	}

	/* determine if this is the 1st tuple for this particular histogram
	** entry
	*/
	if ( (hist_tbl.htabbase != histid.db_tab_base) ||
	     (hist_tbl.htabindex != histid.db_tab_index) ||
	     (hist_att != hist_tbl.hatno) )
	{

	    histid.db_tab_base  = hist_tbl.htabbase;
	    histid.db_tab_index = hist_tbl.htabindex;
	    hist_att = hist_tbl.hatno;
	    base = duve_statfind ( &histid, hist_tbl.hatno, &base, duve_cb);
	    seq = 0;

	    /* test 1 - make sure that statistics entry exists that references
	    **		this histogram
	    */

	    if ( base == DU_INVALID)
	    {
		if (duve_banner( DUVE_IIHISTOGRAM, 1, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS)DUVE_BAD);
		    
		duve_talk( DUVE_MODESENS, duve_cb, 
			    S_DU164A_NO_STATS_FOR_HISTOGRAM, 4,
			    sizeof(histid.db_tab_base), &histid.db_tab_base,
			    sizeof(histid.db_tab_index), &histid.db_tab_index);
		if ( duve_talk (DUVE_ASK, duve_cb, 
			    S_DU0329_DROP_FROM_IIHISTOGRAM, 6,
			    sizeof(histid.db_tab_base), &histid.db_tab_base,
			    sizeof(histid.db_tab_index), &histid.db_tab_index,
			    sizeof(hist_tbl.hatno), &hist_tbl.hatno)
		== DUVE_YES)
		{
		    EXEC SQL delete from iihistogram where 
		        htabbase = :hist_tbl.htabbase and 
			htabindex = :hist_tbl.htabindex;
		    base = DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0379_DROP_FROM_IIHISTOGRAM,
			    6, sizeof(histid.db_tab_base), &histid.db_tab_base,
			    sizeof(histid.db_tab_index), &histid.db_tab_index,
			    sizeof(hist_tbl.hatno), &hist_tbl.hatno);
		    continue;
		}

	    }	/* end test 1 */

	}   /* end 1st tuple in this histogram entry */

	/* if we are dropping the statistic tuple, dont bother
	** checking the histogram
	*/
	if ( base == DUVE_DROP)
	    continue;

	/* test 2 - verify sequence of multiple tuple histogram entry is valid
	*/
			
	ptr = duve_cb->duvestat->stats[base].du_rptr;
	if ( hist_tbl.hsequence == seq )
	    seq++;			/* all is well */
	else if (hist_tbl.hsequence < seq)
	{
	    /* duplicate histogram sequence tuple */
            if (duve_banner( DUVE_IIHISTOGRAM, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1652_HISTOGRAM_DUP_ERR, 8,
			0, duve_cb->duverel->rel[ptr].du_tab,
			0, duve_cb->duverel->rel[ptr].du_own,
			sizeof(hist_tbl.hatno), &hist_tbl.hatno,
			sizeof(hist_tbl.hsequence), &hist_tbl.hsequence);
	    if ( duve_talk (DUVE_ASK, duve_cb, S_DU0329_DROP_FROM_IIHISTOGRAM,
		    6,sizeof(histid.db_tab_base), &histid.db_tab_base,
		    sizeof(histid.db_tab_index), &histid.db_tab_index,
		    sizeof(hist_tbl.hatno), &hist_tbl.hatno)
	    == DUVE_YES)
	    {
	    	    duve_cb->duvestat->stats[base].du_hdrop = DUVE_DROP;
		    base = DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			    S_DU0379_DROP_FROM_IIHISTOGRAM,
			    6, sizeof(histid.db_tab_base), &histid.db_tab_base,
			    sizeof(histid.db_tab_index), &histid.db_tab_index,
			    sizeof(hist_tbl.hatno), &hist_tbl.hatno);
		    continue;
	    }
	    else
		seq = hist_tbl.hsequence + 1;
	}
	else
	{
	    /* missing tree sequence tuple */
            if (duve_banner( DUVE_IIHISTOGRAM, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb,S_DU164B_HISTOGRAM_SEQUENCE_ERR, 
			8, 0, duve_cb->duverel->rel[ptr].du_tab,
			0, duve_cb->duverel->rel[ptr].du_own,
			sizeof(hist_tbl.hatno), &hist_tbl.hatno,
			sizeof(seq), &seq);
	    if ( duve_talk (DUVE_ASK, duve_cb, S_DU0329_DROP_FROM_IIHISTOGRAM,6,
		    sizeof(histid.db_tab_base), &histid.db_tab_base,
		    sizeof(histid.db_tab_index), &histid.db_tab_index,
		    sizeof(hist_tbl.hatno), &hist_tbl.hatno)
	    == DUVE_YES)
	    {
	    	    duve_cb->duvestat->stats[base].du_hdrop = DUVE_DROP;
		    base = DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			    S_DU0379_DROP_FROM_IIHISTOGRAM,
			    6, sizeof(histid.db_tab_base), &histid.db_tab_base,
			    sizeof(histid.db_tab_index), &histid.db_tab_index,
		    sizeof(hist_tbl.hatno), &hist_tbl.hatno);
		    continue;
	    }
	    else
		seq = hist_tbl.hsequence + 1;
	}

    }  /* end loop for each tuple */

    /* now that each tuple in iistatistics and iihistogram has been examined,
    ** open a cursor and loop to see if any modification should be made to the
    ** table
    */

    for (base=0; base < duve_cb->duve_scnt; base++)
    {

	if (duve_cb->duvestat->stats[base].du_rptr == DUVE_DROP)
	    continue;
	else if (duve_cb->duvestat->stats[base].du_sdrop == DUVE_DROP)
	{   
	    tid = duve_cb->duvestat->stats[base].du_sid.db_tab_base;
	    tidx = duve_cb->duvestat->stats[base].du_sid.db_tab_index;
	    att = duve_cb->duvestat->stats[base].du_satno;
	    exec sql delete from iistatistics where stabbase = :tid and
		stabindex = :tidx and satno = :att;
	    exec sql delete from iihistogram where htabbase = :tid and
		htabindex = :tidx and hatno = :att;
	    continue;
	}
	else if (duve_cb->duvestat->stats[base].du_hdrop == DUVE_DROP)
	{
	    tid = duve_cb->duvestat->stats[base].du_sid.db_tab_base;
	    tidx = duve_cb->duvestat->stats[base].du_sid.db_tab_index;
	    att = duve_cb->duvestat->stats[base].du_satno;
	    exec sql update iistatistics set snumcells = 0 where 
		stabbase = :tid and stabindex = :tidx and satno = :att;
	    exec sql delete from iihistogram where htabbase = :tid and
		htabindex = :tidx and hatno = :att;
	    continue;
	}
	else if (duve_cb->duvestat->stats[base].du_numcel != 0)
	{
    	    tid = duve_cb->duvestat->stats[base].du_sid.db_tab_base;
	    tidx = duve_cb->duvestat->stats[base].du_sid.db_tab_index;
	    att = duve_cb->duvestat->stats[base].du_satno;
	    cells = duve_cb->duvestat->stats[base].du_numcel;
	    exec sql update iistatistics set snumcells = :cells where 
		stabbase = :tid and stabindex = :tidx and satno = :att;
	    continue;
	}
	
    } /* end loop for each entry in iistatistics cache */

    MEfree ( (PTR) duve_cb->duvestat);
    duve_cb->duvestat = NULL;

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckhist"); 

    return ( (DU_STATUS) DUVE_YES );
}

/*{
** Name: ckintegs - run verifydb tests to check system catalog iiintegrity.
**
** Description:
**      This routine opens a cursor to walk iiintegrity one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iiintegrity system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped or entered from/into table iiintegrity
**	    tuples may be modified or dropped from iirelation
**	    tuples may be entered into iirule
**	    tuples may be entered into iiprocedure
**	    tuples may be entered into iiindex
**
** History:
**      29-Aug-1988 (teg)
**          initial creation
**	21-Feb-1989 (teg)
**	    Stopped caching qryid for integrity in relinfo cache.
**	6-jan-92 (rickh)
**	    Constraints don't have query trees so please forgive
**	    them.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	16-jun-93 (anitap)
**	    Added tests for FIPS constraints project.
**	10-oct-93 (anitap)
**	    If matching iikey, iirule, iiprocedure, iiindex tuples not 
**	    present for the constraint, delete the constraint and create a SQL
**	    file to recreate the constraint.
**	    Added call to new function to drop the constraint and create SQL 
**	    file to create the constraint.
**	    Corrected spelling of message S_DU0320_DROP_FROM_IIINTERGRITY to
**	    S_DU0320_DROP_FROM_IIINTEGRITY.
**	10-dec-93 (teresa)
**	    Changed test 3 to query iitree on its keys so the query does not
**	    need to be a scan.  This is part of an effort to improve performance
**	    for verifydb.  Also added performance statistics reporting.
**	20-dec-93 (anitap)
**	    Cache the integrity tuples for which constraint needs to be dropped
**	    and recreated. Use iidbdepends cache in test 8 to check if 
**	    rule present for CHECK constraint. Also use iidbdepends cache in
**	    test 9 to check for procedures for referential constarints.
**	    Added new test 6 to check if the unique constraint is present
**	    for the referential constraint. If not, we continue with the next
**	    iiintegrity tuple.
**	    Do not call duve_obj_type() as we know the independent object type.
**	21-dec-93 (andre,teresa)
**	    Fix error handling for S_DU16A8_IIDBDEPENDS_MISSING, which did a
**	    query of two parameters into a single esqlc variable and used the
**	    wrong parameters to the error message.
**	07-dec-94 (teresa)
**	    remove erronous abort if iiintegrity is empty.  Do not open cursor
**	    if there are no tuples in iiintegrity.
**	08-mar-94 (anitap)
**	    Rick has added another dbproc for referential constraint. Fix for
**	    the same.
**	24-Jul-2002 (inifa01
**	    Bug 106821.  Do not verify existence of an index in Test 10 for 
**	    constraint where 'index = base table structure'. 
**	12-Sep-2004 (schka24)
**	    Another base table structure bug, test 7 this time.
*/
DU_STATUS
ckintegs(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_INTEGRITY	iiintegrity;	
    DB_TAB_ID		int_id;
    DU_STATUS		base;
    DU_STATUS		status;
    i4	 	integrity = DU_SQL_OK;
    u_i4		size;
    STATUS              mem_int;
    i4		len;
    DU_STATUS 	        dep_offset;
    i4			dtype;
    i4 		ind_offset;
    i4		ind_type = DB_CONS; 

    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duveint.sh>;
	i4 cnt = 0;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i4	id1, id2;
	char	name[DB_MAXNAME + 1];
        char	owner[DB_MAXNAME + 1];
	i4	relstat;
	i2	mode;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory to hold add/drop commands */
    EXEC SQL select count(inttabbase) into :cnt from iiintegrity;

    if (cnt == 0)
	return DUVE_YES;
    else	
        size = cnt * sizeof(DU_INTEGRITY);

    duve_cb->duveint = (DUVE_INTEGRITY *) MEreqmem(0, size, TRUE,
                               &mem_int);

    duve_cb->duve_intcnt = 0;

    if ( (mem_int != OK) || (duve_cb->duveint == 0) )
    {
        duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }

    EXEC SQL DECLARE int_cursor CURSOR FOR
	select * from iiintegrity;
    EXEC SQL open int_cursor;
    
    /* loop for each tuple in iiintegrity */
    for (;;)  
    {
	/* get tuple from iiintegrity */
	EXEC SQL FETCH int_cursor INTO :int_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iiintegrity */
	{
	    EXEC SQL CLOSE int_cursor;
	    break;
	}

	/* Put entry from iiintegrity table in iiintegrity
	** structure */
	iiintegrity.dbi_tabid.db_tab_base = int_tbl.inttabbase;
	iiintegrity.dbi_tabid.db_tab_index = int_tbl.inttabidx;
	iiintegrity.dbi_txtid.db_qry_high_time = int_tbl.intqryid1;
	iiintegrity.dbi_txtid.db_qry_low_time = int_tbl.intqryid2;
     	iiintegrity.dbi_tree.db_tre_high_time = int_tbl.inttreeid1;
	iiintegrity.dbi_tree.db_tre_low_time = int_tbl.inttreeid2;
	iiintegrity.dbi_resvar = int_tbl.intresvar;
	iiintegrity.dbi_number = int_tbl.intnumber;
	iiintegrity.dbi_seq = int_tbl.intseq;
	iiintegrity.dbi_cons_id.db_tab_base = int_tbl.consid1;
	iiintegrity.dbi_cons_id.db_tab_index = int_tbl.consid2;
	iiintegrity.dbi_consschema.db_tab_base = int_tbl.consschema_id1;
	iiintegrity.dbi_consschema.db_tab_index = int_tbl.consschema_id2;
	iiintegrity.dbi_consflags = int_tbl.consflags;

	len = STtrmwhite(int_tbl.consname);

	STcopy(int_tbl.consname, iiintegrity.dbi_consname.db_constraint_name);

	iiintegrity.dbi_consname.db_constraint_name[len] = '\0';

	int_id.db_tab_base = int_tbl.inttabbase;
	int_id.db_tab_index = int_tbl.inttabidx;
	tid = int_tbl.inttabbase;
	tidx = int_tbl.inttabidx;


	/* test 1 -- verify table receiving integrity exists */
	base = duve_findreltid ( &int_id, duve_cb);
	if (base == DU_INVALID)
	{
	    /* this tuple is not in iirelation, so drop from iiintegrity */

            if (duve_banner( DUVE_IIINTEGRITY, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1634_NO_BASE_FOR_INTEG, 4,
			sizeof(tid), &tid, sizeof(tidx), &tidx);
	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
		 4, sizeof(tid), &tid, sizeof(tidx), &tidx)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iiintegrity where current of int_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,S_DU0370_DROP_FROM_IIINTEGRITY, 
			  4, sizeof(tid), &tid, sizeof(tidx), &tidx);
	    }
	    continue;
	} /* endif base table not found */
	else 
	{
	    if ( base == DUVE_DROP)
		continue;	/* if table is marked for drop, stop now */

	}
	/* 
	**  test 2 - verify base table indicates integrities are defined on it
	*/
	if ( (duve_cb->duverel->rel[base].du_stat & TCB_INTEGS) == 0 )
	{
	    /* the relstat in iirelation needs to be modified */
            if (duve_banner( DUVE_IIINTEGRITY, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1632_NO_INTEGS_RELSTAT, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);

	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU031F_SET_INTEGS, 0)
	    == DUVE_YES)
	    {
		basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		relstat = duve_cb->duverel->rel[base].du_stat | TCB_INTEGS;
		duve_cb->duverel->rel[base].du_stat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
			    reltid = :basetid and reltidx = :basetidx;
		if (sqlca.sqlcode == 0)
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU036F_SET_INTEGS, 0);
	    }  /* endif fix relstat */

	} /* endif relstat doesnt indicate integrities */

	/* test 3 -- verify internal representation of integrity exists */

	/* integrities should have trees unless they are constraints! */

	if ( ( int_tbl.consflags & ( CONS_UNIQUE | CONS_CHECK | CONS_REF ) )
		== 0 )
	{
	    mode = DB_INTG;
	    EXEC SQL repeated select any(treetabbase) into :cnt
		from iitree where
			treetabbase = :tid and treetabidx = :tidx and
			treemode = :mode and treeid1 = :int_tbl.inttreeid1 and
			treeid2 = :int_tbl.inttreeid2;
	    if (cnt == 0)
	    {
	        /* missing internal representation of integrity */
                if (duve_banner( DUVE_IIINTEGRITY, 3, duve_cb)
	        == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
	        
	        duve_talk ( DUVE_MODESENS, duve_cb, S_DU1635_NO_TREE_FOR_INTEG, 8,
			    sizeof(tid), &tid, sizeof(tidx), &tidx,
			    0, duve_cb->duverel->rel[base].du_tab,
			    0, duve_cb->duverel->rel[base].du_own);

	        if ( duve_talk(DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
			   4, sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	        {
		    EXEC SQL delete from iiintegrity where current of int_cursor;
		    duve_talk(DUVE_MODESENS,duve_cb,S_DU0370_DROP_FROM_IIINTEGRITY, 
			  4, sizeof(tid), &tid, sizeof(tidx), &tidx);
		    continue;
	        }
	    }
	}	/* end if not a constraint */

	/* test 4 - verify query text used to define integrity exists */

	mode = DB_INTG;
	EXEC SQL repeated select any(txtid1) into :cnt from iiqrytext where
	        txtid1 = :int_tbl.intqryid1 and txtid2 = :int_tbl.intqryid2
		and mode = :mode;
	if (cnt == 0)
	{
	    /* missing query text that defined integrity */
            if (duve_banner( DUVE_IIINTEGRITY, 4, duve_cb)
	        == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1636_NO_INTEG_QRYTEXT, 8,
			sizeof(tid), &tid, sizeof(tidx), &tidx,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);
	}

	/* test 5 - verify that the integrity which is a constraint has
        ** enough tuples in iikey.
        */

        if (int_tbl.consflags == CONS_UNIQUE)
	{
           EXEC SQL repeated select any(key_attid) into :cnt from iikey
                where key_consid1 = :int_tbl.consid1 and 
			key_consid2 = :int_tbl.consid2;

           if (cnt < 1)
           {
              if (duve_banner(DUVE_IIINTEGRITY, 5,  duve_cb)
               		== DUVE_BAD)
                 return ((DU_STATUS) DUVE_BAD);
              duve_talk(DUVE_MODESENS, duve_cb,
                S_DU1690_MISSING_KEYTUP, 2, 0, int_tbl.consname);

	       if ( duve_talk(DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
			   4, sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	      {	
   	         /* call routine which drops and creates SQL file to 
	         ** create the constraint */
	   
	         duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_dcons =
                               DUVE_DROP;
	     
 	         if ((status = create_cons(duve_cb, &iiintegrity, base)) 
			!= DUVE_YES)
	            return(status);

                 continue;
	      }	
           } /* endif corresponding tuple not in iikey */
	} /* end test 5 */

	/* Test 6 - Check that for referential constraint, we have the
	** the corresponding UNIQUE constraint.
	*/

	if (int_tbl.consflags == CONS_REF)
	{
	   DU_STATUS	depend_offset;
	   DU_D_DPNDS	*depend;
	   i4		dep_type = DB_CONS;

	   depend_offset = duve_d_dpndfind(int_tbl.inttabbase,
				int_tbl.inttabidx, dep_type,
				int_tbl.intnumber, duve_cb);

	   if (depend_offset != DU_INVALID)
	   {
	      depend = duve_cb->duvedpnds.duve_dep + depend_offset;

	      if (depend->du_dflags & DU_MISSING_INDEP_OBJ)
	      {
	         char	obj_type[50];
	         i4	type_len;

            	 if (duve_banner(DUVE_IIINTEGRITY, 6, duve_cb)
                       == DUVE_BAD)
                    return ((DU_STATUS) DUVE_BAD);

		 STcopy("CONSTRAINT", obj_type);
		 type_len = sizeof("CONSTRAINT") - 1;
                
                 duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU16C1_NONEXISTENT_INDEP_OBJ, 
		       	10,
		       	type_len, 	       obj_type,
		       	sizeof(int_tbl.inttabbase), &int_tbl.inttabbase,
	               	sizeof(int_tbl.inttabidx), &int_tbl.inttabidx,
			sizeof(dep_type), &dep_type,
		       	sizeof(int_tbl.intnumber), &int_tbl.intnumber);
	        continue;
	      } /* endif unique constraint missing */
	  } /* end if depend_offset != DU_INVALID */
	} /* end test 6 */
		

        /* Test 7 - check that we have enough tuples in iidbdepends 
        ** for the constraint, i.e., 1 for unique or check and 8/9 for
        ** referential (the index for referential constraint is user droppable)
	** FIXME eliminate this check for UNIQUE because the CONS_BASEIX
	** flag is not reliable - it's not set prior to late 2.6 patches,
	** and there's no convenient way I could think of to get
	** upgradedb to set it without a lot of overhead or work.
        */ 

        if ( /* ** removed (int_tbl.consflags == CONS_UNIQUE) || ** */
	  (int_tbl.consflags == CONS_CHECK) || 
	  (int_tbl.consflags == CONS_REF))
        {	

          integrity = DU_SQL_OK; 
	  ind_offset = DU_INVALID; 

          dtype = DU_INVALID;

          dep_offset = duve_d_cnt(ind_offset, int_tbl.inttabbase, 
					int_tbl.inttabidx,
					ind_type, int_tbl.intnumber,
					dtype, duve_cb);
          switch (int_tbl.consflags)
          {
	     case CONS_UNIQUE:
	     case CONS_CHECK:

	       	    if (dep_offset < 1)
	       	    {
		  	integrity = DU_INVALID;
	       	    }
	       	    break;

	     case CONS_REF:

			if (dep_offset < 8)
			{
			   integrity = DU_INVALID;
			}
			break;
	     default:
			break;
         }

         if (integrity == DU_INVALID)
         {

            if (duve_banner(DUVE_IIINTEGRITY, 7, duve_cb)
                       == DUVE_BAD)
               return ((DU_STATUS) DUVE_BAD);
            duve_talk(DUVE_MODESENS, duve_cb, S_DU16A8_IIDBDEPENDS_MISSING, 4,
			0, duve_cb->duverel->rel[base].du_tab, 0, 
			duve_cb->duverel->rel[base].du_own);

            if ( duve_talk(DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
			   4, sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	    {
	       /* call routine to drop and create the SQL file to 
	       ** recreate constraint 
	       */ 

               duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_dcons =
                               DUVE_DROP;

	       if ((status = create_cons(duve_cb, &iiintegrity, base)) 
			!= DUVE_YES)
		  return(status);

	       continue;
	    } /* endif drop constraint */
      
         } /* endif enough iidbdepends tuples not present */

     } /* end test 7 */

     /* test 8 - verify that there are enough rule tuples for
     ** the constraint i.e., 1 for check and 4 for referential.
     */

     integrity = DU_SQL_OK;
	
     switch (int_tbl.consflags)
     {
	    case CONS_CHECK:	
		{	
			bool		first = FALSE;
    			DU_STATUS	indep_offset;
    			DU_I_DPNDS	*indep;
		
			indep_offset = duve_i_dpndfind(int_tbl.inttabbase,
					int_tbl.inttabidx, ind_type, 
					int_tbl.intnumber, duve_cb, first);

			if (indep_offset != DU_INVALID)
			{
			   indep = duve_cb->duvedpnds.duve_indep 
					+ indep_offset; 

			   /* use iidbdepends cache to check if rule(dependent
			   ** object) present for the check constraint.
			   */
			   if (indep->du_next.du_dep->du_dflags &
					DU_NONEXISTENT_INDEP_OBJ)

	      		      integrity = DU_INVALID;

			} /* endif indep_offset != DU_INVALID */
			break;	
		 }

	    case CONS_REF:	
		 {	
			dtype = DB_RULE;

		        ind_offset = DU_INVALID;

			dep_offset = duve_d_cnt(ind_offset,
				int_tbl.inttabbase, int_tbl.inttabidx,
				ind_type, int_tbl.intnumber,
				dtype, duve_cb);
					
			if (dep_offset < 4)	
	                   integrity = DU_INVALID;
			
			break;
	      	  }

	      default:
			break;	
	}

	if (integrity == DU_INVALID)
	{
           if (duve_banner(DUVE_IIINTEGRITY, 8, duve_cb)
                       == DUVE_BAD)
               return ((DU_STATUS) DUVE_BAD);
           duve_talk(DUVE_MODESENS, duve_cb,
                        S_DU1691_MISSING_RULETUP, 2, 0, int_tbl.consname);
	
           if ( duve_talk(DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
			   4, sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	   {
   	      /* call routine which drops and creates the SQL file to create
	      ** SQL file to create the constraint 
	      */

              duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_dcons =
                               DUVE_DROP;

	      if ((status = create_cons(duve_cb, &iiintegrity, base)) 
			!= DUVE_YES)
	         return(status);

	      continue;
	   } /* endif drop constraint */

        } /* end if corresponding tuple not present in iirule */

        /* Test 9 - check that we have enough tuples in iiprocedure
        ** for the constraint - i.e., 4 for referential.
        */

        if (int_tbl.consflags == CONS_REF)
        {
	   dtype = DB_DBP;
	   ind_offset = DU_INVALID;

	   dep_offset = duve_d_cnt(ind_offset,
				int_tbl.inttabbase, int_tbl.inttabidx,
				ind_type, int_tbl.intnumber,
				dtype, duve_cb);
					
           if (dep_offset < 4)
           {
              if (duve_banner(DUVE_IIINTEGRITY, 9, duve_cb)
                       == DUVE_BAD)
                 return ((DU_STATUS) DUVE_BAD);
              duve_talk(DUVE_MODESENS, duve_cb,
                       S_DU1692_MISSING_DBPROCTUP, 2, 0, int_tbl.consname);

              if (duve_talk (DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
		 	4, sizeof(tid), &tid, sizeof(tidx), &tidx)
			== DUVE_YES)
	      {	
                 duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_dcons =
                               DUVE_DROP;

	         if ((status = create_cons(duve_cb, &iiintegrity, base)) 
			!= DUVE_YES)
	         return(status);
	    
	         continue;
	      } /* endif drop constraint */
           } /* endif corresponding tuple not present in iiprocedure */
       } /* endif constraint type = CONS_REF */

       /* Test 10 - check that we have enough tuples in iiindex
       ** for the constraint i.e., 1 for unique.
       ** We do not check the index for referential as the user can drop the
       ** index, also we do not check for index if base table is index.		
	** FIXME removed this test because CONS_BASEIX is not set
	** reliably for older (ie pre-late-2.6-patch) databases.
       */

#if 0
       if ( (int_tbl.consflags & (CONS_UNIQUE | CONS_PRIMARY)) &&
	    !(int_tbl.consflags & CONS_BASEIX) )
       {
          EXEC SQL repeated select any(indexid) into :cnt
           from iiindex
               where baseid = :int_tbl.inttabbase;

          if (cnt != 1)
          {
            if (duve_banner(DUVE_IIINTEGRITY, 10, duve_cb)
                        == DUVE_BAD)
                 return ((DU_STATUS) DUVE_BAD);
               duve_talk(DUVE_MODESENS, duve_cb,
                        S_DU1693_MISSING_INDEXTUP, 2, 0,
                                int_tbl.consname);

               if (duve_talk (DUVE_ASK, duve_cb, S_DU0320_DROP_FROM_IIINTEGRITY,
		 	4, sizeof(tid), &tid, sizeof(tidx), &tidx)
			== DUVE_YES)
	       {	

	          /* if the index is missing for the unique constraint,
	          ** we cannot drop the constraint because DMF will complain.
	          ** This is because there is a corresponding entry in
		  ** iirelation for the index. So we need to hand delete the 
		  ** corresponding tuples in iikey, iiintegrity.
	          ** The corresponding iidbdepends tuple would have been deleted
	          ** by ckdbdep(). The corresponding tuple in iirelation is 
		  ** deleted by ckrel() and the corresponding tuples in 
		  ** iiattribute by ckatt(). 
	          */ 

	          if ((status = create_cons(duve_cb, &iiintegrity, base)) 
			!= DUVE_YES)
	             return(status);
	
	          EXEC SQL delete from iikey 
			where key_consid1 = :int_tbl.consid1;
	          EXEC SQL delete from iiintegrity 
			where inttabbase = :int_tbl.inttabbase
			and consid1 = :int_tbl.consid1;
	          continue;     

	      } /* endif drop constraint */

         } /* endif corresponding tuple not present in iiindex */  
      } /* endif CONS_UNIQUE */
#endif

    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckintegs"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckxproc - run verifydb check on iixprocedure dbms catalog
**
** Description:
**	This function will ensure that IIXPROCEDURE is in sync with IIPROCEDURE.
**	This will be accomplished in two steps:
**	- first we will open a cursor to read tuples in iixprocedure for which 
**	  there are no corresponding tuples in iiprocedure; if one or more such
**	  tuples are found, we will ask for permission to drop and recreate the 
**	  index
**	- then we will open a cursor on iiprocedure to read tuples for which 
**	  there is no corresponding iixprocedure tuple and insert appropriate 
**	  iixprocedure tuples
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**	none.
**
** Returns:
**    	DUVE_YES
**      DUVE_BAD
**
** Exceptions:
**	none
**
** Side Effects:
**	tuples may be inserted into / deleted from iixprocedure
**
** History:
**      19-jan-94 (andre)
**	    written
[@history_template@]...
*/
DU_STATUS
ckxproc(
	DUVE_CB		*duve_cb)
{
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvexdbp.sh>;
	u_i4		tid_ptr;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE xdbp_tst_curs_1 CURSOR FOR
	select * from iixprocedure xdbp where not exists 
	    (select 1 from iiprocedure dbp 
	    	where
		        xdbp.tidp 	= dbp.tid
		    and xdbp.dbp_id 	= dbp.dbp_id
		    and xdbp.dbp_idx 	= dbp.dbp_idx);
    
    EXEC SQL open xdbp_tst_curs_1;

    /*
    ** test 1: for every IIXPROCEDURE tuple for which there is no corresponding 
    ** IIPROCEDURE tuple ask user whether we may delete the offending tuple from
    ** IIXPROCEDURE
    */
    for (;;)
    {
        EXEC SQL FETCH xdbp_tst_curs_1 INTO :xdbp_tbl;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xdbp_tst_curs_1;
	    break;
        }

	/* 
	** there is no tuple in IIPROCEDURE corresponding to this IIXPROCEDURE 
	** tuple 
	*/
        if (duve_banner(DUVE_IIXPROCEDURE, 1, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16CB_EXTRA_IIXPROCEDURE, 6,
            sizeof(xdbp_tbl.dbp_id), &xdbp_tbl.dbp_id,
	    sizeof(xdbp_tbl.dbp_idx), &xdbp_tbl.dbp_idx,
	    sizeof(xdbp_tbl.tidp), &xdbp_tbl.tidp);
	if (duve_talk( DUVE_ASK, duve_cb, S_DU024C_DELETE_IIXPROC, 0) 
	    == DUVE_YES)
	{
	    exec sql delete from iixprocedure where current of xdbp_tst_curs_1;

	    duve_talk(DUVE_MODESENS,duve_cb, S_DU044C_DELETED_IIXPROC, 0);
	    
	    break;
	}
    }

    /* end test 1 */

    EXEC SQL DECLARE xdbp_tst_curs_2 CURSOR FOR
	select dbp.*, dbp.tid from iiprocedure dbp where not exists
	    (select 1 from iixprocedure xdbp
		where
			xdbp.tidp	= dbp.tid
		    and xdbp.dbp_id     = dbp.dbp_id
		    and xdbp.dbp_idx    = dbp.dbp_idx
		    /* 
		    ** this last comparison ensures that we get the qualifying 
		    ** rows - otherwise (I suspect) OPF chooses to join index 
		    ** to itself and finds nothing
		    */
		    and dbp_name	= dbp_name);

    EXEC SQL open xdbp_tst_curs_2 FOR READONLY;

    /*
    ** test 2: for every IIPROCEDURE tuple for which there is no corresponding 
    ** IIXPROCEDURE tuple ask user whether we may insert a missing tuple into 
    ** IIXPROCEDURE
    */
    for (;;)
    {
        EXEC SQL FETCH xdbp_tst_curs_2 INTO :proc_tbl, :tid_ptr;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xdbp_tst_curs_2;
	    break;
        }

	/* 
	** there is no tuple in IIXPROCEDURE corresponding to this IIPROCEDURE 
	** tuple 
	*/
        if (duve_banner(DUVE_IIXPROCEDURE, 2, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16CC_MISSING_IIXPROCEDURE, 6,
	    sizeof(proc_tbl.dbp_id), &proc_tbl.dbp_id,
	    sizeof(proc_tbl.dbp_idx), &proc_tbl.dbp_idx,
	    sizeof(tid_ptr), &tid_ptr);
	if (duve_talk( DUVE_ASK, duve_cb, S_DU024D_INSERT_IIXPROC_TUPLE, 0) 
	    == DUVE_YES)
	{
	    exec sql insert into 
		iixprocedure(dbp_id, dbp_idx, tidp)
		values (:proc_tbl.dbp_id, :proc_tbl.dbp_idx, :tid_ptr);
			
	    duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU044D_INSERTED_IIXPROC_TUPLE, 0);
	}
    }

    /* end test 2 */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckxproc"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckproc - run verifydb tests to check system catalog iiprocedure.
**
** Description:
**      This routine opens a cursor to walk iiprocedure one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iiprocedure system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iiprocedure.
**
** History:
**      27-may-1993 (teresa)
**          initial creation
**	16-jun-93 (anitap)
**	    Added test for CREATE SCHEMA project.
**	10-oct-93 (anitap)
**	    Added test for FIPS constraints project. 
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	13-dec-93 (anitap)
**	    Corrected typos. Use ANY instead of COUNT to improve performance.
**	    Corrected test number in duve_banner() to 4.
**	21-jan-94 (andre)
**	    added code to implement newly defined tests 5-8
**	28-jan-94 (andre)
**	    if the dbproc text is missing (test #1), 
**	    dbp_id is not unique (test #2), or 
**	    IIDBDEPENDS tuple is missing for a system-generated dbproc (test #4)
**	    then rather than deleting IIPROCEDURE tuple we will add a record to
**	    the "fix it" list reminding us to drop this dbproc and mark dormant
**	    any dbprocs dependent on it.
[@history_template@]...
*/
DU_STATUS
ckproc(duve_cb)
DUVE_CB            *duve_cb;
{

    DB_TAB_ID		proc_id;
    DU_STATUS		base;
    DU_STATUS   	proc_dep_offset = DU_INVALID;
    u_i4		size;
    DUVE_DROP_OBJ	*drop_obj;
    STATUS		mem_stat;

    EXEC SQL BEGIN DECLARE SECTION;
	i4 cnt;
	i2	mode;
        char	schname[DB_MAXNAME + 1];
	char	*statement_buffer, buf[CMDBUFSIZE];
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory to hold add commands */	
    EXEC SQL select count(dbp_name) into :cnt from iiprocedure;

    if ( (sqlca.sqlcode == 100) || (cnt == 0))
    {
       /* its unreasonable to have an empty iiprocedure table, as procedures 
       ** "iierror" and "iiqef_alter_extension" should be created by createdb. 
       */
       duve_talk (DUVE_ALWAYS, duve_cb, E_DU503B_EMPTY_IIPROCEDURE, 0);
       return DUVE_BAD;
    }

    duve_cb->duve_prcnt = 0;
    size = cnt * sizeof(DU_PROCEDURE);
    duve_cb->duveprocs = (DUVE_PROCEDURE *) MEreqmem(0, size, TRUE, 
			&mem_stat);

    if ( (mem_stat != OK) || (duve_cb->duveprocs == 0))
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
        return( (DU_STATUS) DUVE_BAD);
    }	

    EXEC SQL DECLARE proc_cursor CURSOR FOR
	select * from iiprocedure;
    EXEC SQL open proc_cursor;

    mode = DB_DBP;

    /* loop for each tuple in iiprocedure */
    for (;;)  
    {
	/* get tuple from iiprocedure */
	EXEC SQL FETCH proc_cursor INTO :proc_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iiprocedure */
	{
	    EXEC SQL CLOSE proc_cursor;
	    break;
	}

	proc_id.db_tab_base = proc_tbl.dbp_id;
	proc_id.db_tab_index = proc_tbl.dbp_idx;

	STtrmwhite(proc_tbl.dbp_owner);
	STtrmwhite(proc_tbl.dbp_name);

	/* test 1 - verify query text used to define procedure exists */

	EXEC SQL repeated select any(txtid1) into :cnt from iiqrytext where
	        txtid1 = :proc_tbl.dbp_txtid1
		and txtid2 = :proc_tbl.dbp_txtid2
		and mode = :mode;
	if (cnt == 0)
	{
	    /* missing query text that defined procedure */
            if (duve_banner( DUVE_IIPROCEDURE, 1, duve_cb)
	        == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
		
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1680_NO_TXT_FOR_DBP, 4,
			0, proc_tbl.dbp_name, 0, proc_tbl.dbp_owner);
	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0344_DROP_DBP, 0) == DUVE_YES)
	    {
		drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		if (mem_stat != OK || drop_obj == NULL)
		{
		    duve_talk (DUVE_ALWAYS, duve_cb, 
			E_DU3400_BAD_MEM_ALLOC, 0);
		    return ((DU_STATUS) DUVE_BAD);
		}

		/*
		** add a record to the "fix it" list reminding us to drop this 
		** database procedure
		*/
		drop_obj->duve_obj_id.db_tab_base  = proc_tbl.dbp_id;
		drop_obj->duve_obj_id.db_tab_index = proc_tbl.dbp_idx;
		drop_obj->duve_obj_type            = DB_DBP;
		drop_obj->duve_secondary_id	   = 0;
		drop_obj->duve_drop_flags	   = DUVE_DBP_DROP;

		/* link this description into the existing list */
		drop_obj->duve_next = 
		    duve_cb->duvefixit.duve_objs_to_drop;

		duve_cb->duvefixit.duve_objs_to_drop = drop_obj;

		duve_talk(DUVE_MODESENS, duve_cb, S_DU0394_DROP_DBP, 0);
	    }
	    continue;
	} /* endif procedure definition does not exist */	    

	/* test 2 -- verify procedure identifier is unique */
	base = duve_findreltid ( &proc_id, duve_cb);
	if (base != DU_INVALID)
	{
	    /* this id is in iirelation, so drop from iiprocedure */

	    if (duve_banner( DUVE_IIPROCEDURE, 2, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1681_DBPID_NOT_UNIQUE, 8,
			0, proc_tbl.dbp_name, 0, proc_tbl.dbp_owner,
			sizeof(proc_tbl.dbp_id), &proc_tbl.dbp_id,
			sizeof(proc_tbl.dbp_idx), &proc_tbl.dbp_idx);
	    if ( duve_talk(DUVE_IO, duve_cb, S_DU0344_DROP_DBP, 0)
	    == DUVE_YES)
	    {
		drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		if (mem_stat != OK || drop_obj == NULL)
		{
		    duve_talk (DUVE_ALWAYS, duve_cb, 
			E_DU3400_BAD_MEM_ALLOC, 0);
		    return ((DU_STATUS) DUVE_BAD);
		}

		/*
		** add a record to the "fix it" list reminding us to drop this 
		** database procedure
		*/
		drop_obj->duve_obj_id.db_tab_base  = proc_tbl.dbp_id;
		drop_obj->duve_obj_id.db_tab_index = proc_tbl.dbp_idx;
		drop_obj->duve_obj_type            = DB_DBP;
		drop_obj->duve_secondary_id	   = 0;
		drop_obj->duve_drop_flags	   = DUVE_DBP_DROP;

		/* link this description into the existing list */
		drop_obj->duve_next = 
		    duve_cb->duvefixit.duve_objs_to_drop;

		duve_cb->duvefixit.duve_objs_to_drop = drop_obj;

		duve_talk(DUVE_MODESENS, duve_cb, S_DU0394_DROP_DBP, 0);
	    }
	    continue;
	} /* endif identifier is not unique */

	/* test 3 -- verify procedure belongs to a schema */

        /* first, take 32 char procedure owner and force into
        ** string DB_MAXNAME char long.
        */
        MEcopy( (PTR)proc_tbl.dbp_owner, DB_MAXNAME, (PTR)schname);
        schname[DB_MAXNAME]='\0';
        (void) STtrmwhite(schname);

        /* now search for iischema tuple or owner match */
        EXEC SQL repeated select any(schema_id) into :cnt from iischema
		where schema_name = :schname;

        if (cnt == 0)
        {
           /* corresponding entry missing in iischema */
           if (duve_banner(DUVE_IIPROCEDURE, 3, duve_cb)
                   == DUVE_BAD)
                return ( (DU_STATUS) DUVE_BAD);
           duve_talk (DUVE_MODESENS, duve_cb, S_DU168D_SCHEMA_MISSING, 2,
                       0, proc_tbl.dbp_owner);
	   if ( duve_talk(DUVE_IO, duve_cb, S_DU0245_CREATE_SCHEMA, 0)
			== DUVE_YES)
	   {	

 	      /* cache info about this tuple */
              MEcopy( (PTR)proc_tbl.dbp_owner, DB_MAXNAME,
                (PTR) duve_cb->duveprocs->procs[duve_cb->duve_prcnt].du_schname);

	      duve_cb->duve_prcnt++;
              
	      duve_talk(DUVE_MODESENS, duve_cb, S_DU0445_CREATE_SCHEMA, 0);
	   }

	  continue;
        } /* end test 3 */

	/* test 4 -- verify iidbdepends tuple present for procedure */

        /* Special check needed for procedures supporting
	** constraints.
	*/

	if ((proc_tbl.dbp_mask1 & DBP_CONS) != 0)
	{
	   /*
	   ** search IIDBDEPENDS cache for a dependent object info element
	   ** describing this procedure
	   */

	   proc_dep_offset = duve_d_dpndfind(proc_tbl.dbp_id, proc_tbl.dbp_idx,
				(i4) DB_DBP, (i4) 0, duve_cb);

	   if (proc_dep_offset == DU_INVALID) 
	   {  
	      /* missing dbdepends tuple for the procedure */
	      if (duve_banner( DUVE_IIPROCEDURE, 4, duve_cb)
                	== DUVE_BAD)
                 return ( (DU_STATUS) DUVE_BAD); 

	       duve_talk(DUVE_MODESENS, duve_cb, S_DU16A8_IIDBDEPENDS_MISSING, 4,
			0, proc_tbl.dbp_name, 0, proc_tbl.dbp_owner);

	      if ( duve_talk(DUVE_IO, duve_cb, S_DU0344_DROP_DBP, 0)
            	== DUVE_YES)
	      {
		 drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		     sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		 if (mem_stat != OK || drop_obj == NULL)
		 {
		     duve_talk (DUVE_ALWAYS, duve_cb, 
			 E_DU3400_BAD_MEM_ALLOC, 0);
		     return ((DU_STATUS) DUVE_BAD);
		 }

		 /*
		 ** add a record to the "fix it" list reminding us to drop this 
		 ** database procedure
		 */
		 drop_obj->duve_obj_id.db_tab_base  = proc_tbl.dbp_id;
		 drop_obj->duve_obj_id.db_tab_index = proc_tbl.dbp_idx;
		 drop_obj->duve_obj_type            = DB_DBP;
		 drop_obj->duve_secondary_id	    = 0;
		 drop_obj->duve_drop_flags	    = DUVE_DBP_DROP;

		 /* link this description into the existing list */
		 drop_obj->duve_next = 
		     duve_cb->duvefixit.duve_objs_to_drop;

		 duve_cb->duvefixit.duve_objs_to_drop = drop_obj;

                 duve_talk(DUVE_MODESENS, duve_cb, S_DU0394_DROP_DBP, 0);
	      } 
            
	      continue;
	   } /* endif iidbdepends tuple not present for procedure */ 
	} /* end test 4 */
	else
	{
	    /* 
	    ** need to reset proc_dep_offset to DU_INVALID to indicate that we 
	    ** have not yet tried to look up the dbproc in the dependent object
	    ** info list
	    */
	    proc_dep_offset = DU_INVALID;
	}

	/*
	** tests 5 and 6: make sure that an independent object/privilege list 
	**		  for a dbproc exists iff the current record contains an
	**		  indication to that effect
	*/
	if (proc_dep_offset == DU_INVALID)
	{
	    proc_dep_offset = duve_d_dpndfind(proc_tbl.dbp_id, proc_tbl.dbp_idx,
		(i4) DB_DBP, (i4) 0, duve_cb);
	}

	if (proc_dep_offset == DU_INVALID)
	{
	    /* 
	    ** IIDBDEPENDS does not contain any records describing objects on 
	    ** which this dbproc depends - see if you can find any records
	    ** describing privileges on which this dbproc depends in IIPRIV
	    */
	    exec sql REPEATED SELECT any(1) into :cnt 
		from iipriv
		where
			d_obj_id 	= :proc_tbl.dbp_id
		    and d_obj_idx 	= :proc_tbl.dbp_idx
		    and d_priv_number 	= 0
		    and d_obj_type 	= :mode;
	}
	else
	{
	    /*
	    ** init cnt to 0 so that we don't mistakenly assume that we found
	    ** IIPRIV tuples describing privileges on which this dbproc depends
	    */
	    cnt = 0;
	}

	if (   proc_tbl.dbp_mask1 & DB_DBP_INDEP_LIST
	    && proc_dep_offset == DU_INVALID
	    && cnt == 0)
	{
	    /*
	    ** IIPROCEDURE tuple indicates that there is a list of 
	    ** objects/privileges on which the dbproc depends, but none can be 
	    ** found.  Ask user for permission to mark this database procedure 
	    ** dormant
	    */

            if (duve_banner(DUVE_IIPROCEDURE, 5, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
		
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16E5_MISSING_INDEP_LIST, 4,
		0, proc_tbl.dbp_owner,
		0, proc_tbl.dbp_name);
	    if (duve_talk(DUVE_IO, duve_cb, S_DU0251_MARK_DBP_DORMANT, 0)
		== DUVE_YES)
	    {

		/* 
		** user decided to allow us to mark the dbproc 
		** dormant - add its description to the "fixit" list 
		*/

		drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		if (mem_stat != OK || drop_obj == NULL)
		{
		    duve_talk (DUVE_ALWAYS, duve_cb, 
			E_DU3400_BAD_MEM_ALLOC, 0);
		    return ((DU_STATUS) DUVE_BAD);
		}

		drop_obj->duve_obj_id.db_tab_base  = proc_tbl.dbp_id;
		drop_obj->duve_obj_id.db_tab_index = proc_tbl.dbp_idx;
		drop_obj->duve_obj_type            = DB_DBP;
		drop_obj->duve_secondary_id	   = 0;
		drop_obj->duve_drop_flags	   = 0;

		/* link this description into the existing list */
		drop_obj->duve_next = 
		    duve_cb->duvefixit.duve_objs_to_drop;

		duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
		duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU0451_MARKED_DBP_DORMANT, 0);
	    }
	}
	else if (   ~proc_tbl.dbp_mask1 & DB_DBP_INDEP_LIST
		 && (proc_dep_offset != DU_INVALID || cnt != 0))
	{
	    /*
	    ** IIPROCEDURE tuple indicates that there is no list of 
	    ** objects/privileges on which the dbproc depends, but we found 
	    ** record(s) in IIDBDEPENDS and/or IIPRIV describing objects and/or
	    ** privileges on which this database procedure depends.  Ask user 
	    ** for permission to set DB_DBP_INDEP_LIST in IIPROCEDURE.dbp_mask1
	    */

            if (duve_banner(DUVE_IIPROCEDURE, 6, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
		
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16E6_UNEXPECTED_INDEP_LIST, 4,
		0, proc_tbl.dbp_owner,
		0, proc_tbl.dbp_name);
	    if (duve_talk(DUVE_IO, duve_cb, S_DU0252_SET_DB_DBP_INDEP_LIST, 0)
		== DUVE_YES)
	    {
		proc_tbl.dbp_mask1 |= DB_DBP_INDEP_LIST;

		exec sql update iiprocedure set dbp_mask1 = :proc_tbl.dbp_mask1
		    where current of proc_cursor;
		
		duve_talk(DUVE_MODESENS, duve_cb,
		    S_DU0452_SET_DB_DBP_INDEP_LIST, 0);
	    }
	}

	/* end of tests 5 and 6 */

	/*
	** test 7: if the IIPROCEDURE tuple contains a description of the 
	**	   dbproc's underlying base table, verify that it exists and 
	**	   is not a catalog.
	*/
	if (proc_tbl.dbp_ubt_id || proc_tbl.dbp_ubt_idx)
	{
	    DB_TAB_ID		ubt_id;

	    ubt_id.db_tab_base = proc_tbl.dbp_ubt_id;
	    ubt_id.db_tab_index = proc_tbl.dbp_ubt_idx;

	    base = duve_findreltid (&ubt_id, duve_cb);

	    if (base == DU_INVALID)
	    {
		/*
		** dbproc's underlying base table no longer exists
		*/

                if (duve_banner( DUVE_IIPROCEDURE, 7, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		
	        duve_talk(DUVE_MODESENS, duve_cb, S_DU16E7_MISSING_UBT, 8,
		    sizeof(proc_tbl.dbp_ubt_id), &proc_tbl.dbp_ubt_id,
		    sizeof(proc_tbl.dbp_ubt_idx), &proc_tbl.dbp_ubt_idx,
		    0, proc_tbl.dbp_owner,
		    0, proc_tbl.dbp_name);

	        if (duve_talk(DUVE_IO, duve_cb, S_DU0251_MARK_DBP_DORMANT, 0)
		    == DUVE_YES)
	        {

		    /* 
		    ** user decided to allow us to mark the dbproc 
		    ** dormant - add its description to the "fixit" list 
		    */

		    drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		        sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		    if (mem_stat != OK || drop_obj == NULL)
		    {
		        duve_talk (DUVE_ALWAYS, duve_cb, 
			    E_DU3400_BAD_MEM_ALLOC, 0);
		        return ((DU_STATUS) DUVE_BAD);
		    }

		    drop_obj->duve_obj_id.db_tab_base  = proc_tbl.dbp_id;
		    drop_obj->duve_obj_id.db_tab_index = proc_tbl.dbp_idx;
		    drop_obj->duve_obj_type            = DB_DBP;
		    drop_obj->duve_secondary_id	       = 0;
		    drop_obj->duve_drop_flags	       = 0;
    
		    /* link this description into the existing list */
		    drop_obj->duve_next = 
		        duve_cb->duvefixit.duve_objs_to_drop;
    
		    duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
		    duve_talk(DUVE_MODESENS, duve_cb, 
		        S_DU0451_MARKED_DBP_DORMANT, 0);
	        }
	    }
	    else if (   base != DUVE_DROP
		     && duve_cb->duverel->rel[base].du_stat & TCB_CATALOG
		     && ~duve_cb->duverel->rel[base].du_stat & TCB_EXTCATALOG)
	    {
		/*
		** dbproc's underlying base table is a DBMS catalog
		*/

		if (duve_banner( DUVE_IIPROCEDURE, 7, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS) DUVE_BAD);
		
		duve_talk(DUVE_MODESENS, duve_cb, S_DU16E8_UBT_IS_A_CATALOG, 8,
		    sizeof(proc_tbl.dbp_ubt_id), &proc_tbl.dbp_ubt_id,
		    sizeof(proc_tbl.dbp_ubt_idx), &proc_tbl.dbp_ubt_idx,
		    0, proc_tbl.dbp_owner,
		    0, proc_tbl.dbp_name);

	        if (duve_talk(DUVE_IO, duve_cb, 
			S_DU0253_UBT_IS_A_DBMS_CATALOG, 0)
		    == DUVE_YES)
	        {
		    proc_tbl.dbp_ubt_id = proc_tbl.dbp_ubt_idx = 0;

		    exec sql update iiprocedure 
			set dbp_ubt_id = :proc_tbl.dbp_ubt_id,
			    dbp_ubt_idx = :proc_tbl.dbp_ubt_idx
		    where current of proc_cursor;
		
		    duve_talk(DUVE_MODESENS, duve_cb,
		        S_DU0453_CLEARED_DBP_UBT_ID, 0);
	        }
	    }
	}

	/* end of test 7 */

	/*
	** test 8: If the procedure is not marked grantable, make sure there 
	**	   are no permits were granted on it
	*/
	if (~proc_tbl.dbp_mask1 & DB_DBPGRANT_OK)
	{
	    exec sql REPEATED select any(1) into :cnt 
		from iiprotect
		where
			protabbase = :proc_tbl.dbp_id
		    and protabidx  = :proc_tbl.dbp_idx;
	    
	    if (cnt != 0)
	    {
		/*
		** dbproc is marked non-grantable, but there are still some 
		** permits defined on it
		*/

                if (duve_banner( DUVE_IIPROCEDURE, 8, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		
	        duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU16E9_UNEXPECTED_DBP_PERMS, 4,
		    0, proc_tbl.dbp_owner,
		    0, proc_tbl.dbp_name);

	        if (duve_talk(DUVE_IO, duve_cb, S_DU0251_MARK_DBP_DORMANT, 0)
		    == DUVE_YES)
	        {

		    /* 
		    ** user decided to allow us to mark the dbproc 
		    ** dormant - add its description to the "fixit" list 
		    */

		    drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
		        sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
		    if (mem_stat != OK || drop_obj == NULL)
		    {
		        duve_talk (DUVE_ALWAYS, duve_cb, 
			    E_DU3400_BAD_MEM_ALLOC, 0);
		        return ((DU_STATUS) DUVE_BAD);
		    }

		    drop_obj->duve_obj_id.db_tab_base  	= proc_tbl.dbp_id;
		    drop_obj->duve_obj_id.db_tab_index 	= proc_tbl.dbp_idx;
		    drop_obj->duve_obj_type            	= DB_DBP;
		    drop_obj->duve_secondary_id	   	= 0;
		    drop_obj->duve_drop_flags	        = 0;
    
		    /* link this description into the existing list */
		    drop_obj->duve_next = 
		        duve_cb->duvefixit.duve_objs_to_drop;
    
		    duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
		    duve_talk(DUVE_MODESENS, duve_cb, 
		        S_DU0451_MARKED_DBP_DORMANT, 0);
	        }
	    }
	}

    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckproc"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckprotups - run verifydb tests to check system catalog iiprotect.
**
** Description:
**      This routine opens a cursor to walk iiprotect one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iiprotect system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iiprotect
**	    tuples may be modified in iirelation
**
** History:
**      29-Aug-1988 (teg)
**          initial creation
**	5-Dec-1988 (teg)
**	    removed test for iitree tuple for permit.
**	21-Feb-1989 (teg)
**	    stopped caching permit qry text id in relinfo
**	18-aug-89  (teg)
**	    fixed bug 7547 - stop assuming permits are only for tables, and also
**	    search iiprocedure for table ids before removing permit.
**	03-jan-89 (neil)
**	    Added checks for event permits.  This is only added to suppress
**	    failures and not to test relationships.  Made sure to include in
**	    fix for bug 7547.
**	02-feb-90 (teg)
**	    if dbp or even permission, then there is no iirelation tuple, so
**	    skip permit checks that test the relationship between iirelation
**	    and iiprotect.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	28-jun-93 (teresa)
**	    Handle special quel objects owned by DBA, which have entries in
**	    iiprotect with txtid1=0 and txtid2=0.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	21-jan-94 (andre)
**	    Added tests 5-12    
**	21-mar-94 (robf)
**          Allow DB_COPY_INTO and DB_COPY_FROM as legal grants for tables
**      02-Jan-1997 (merja01)
**          Changed long to i4 to fix bug 55186 E_LQ000A Conversion error
**          while doing a verifydb on axp.osf.
**	15-mar-2002 (toumi01)
**	    add prodomset11-33 to support 1024 cols/row
**	12-Sep-2004 (schka24)
**	    Don't complain about sequence rows.
**	3-Mar-2008 (kibro01) b119427
**	    Don't suggest dropping the whole of iiprotect if a permit is
**	    found wrong for a database table - just note it (like with
**	    S_DU163D).
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap columns.
*/
DU_STATUS
ckprotups(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_TAB_ID		pro_id;
    DU_STATUS		base;
    long		valid_privs;
    char		*schname_p, *objname_p;

    EXEC SQL BEGIN DECLARE SECTION;
	i4 cnt;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i4	relstat;
	i4	qtxt_status;
	i4	id1, id2;
	char	obj_name[DB_MAXNAME + 1];
	char	sch_name[DB_MAXNAME + 1];
	i2	mode;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE pro_cursor CURSOR FOR
	select * from iiprotect;
    EXEC SQL open pro_cursor;

    /* loop for each tuple in iiprotect */
    for (;;)  
    {
	/* get tuple from iiprotect */
	EXEC SQL FETCH pro_cursor INTO :pro_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iiprotect */
	{
	    EXEC SQL CLOSE pro_cursor;
	    break;
	}

	pro_id.db_tab_base = pro_tbl.protabbase;
	pro_id.db_tab_index = pro_tbl.protabidx;
	tid = pro_tbl.protabbase;
	tidx = pro_tbl.protabidx;

	/*
	** test 1 -- verify that the type of object on which a permit was
	**	     created is one of those we expect + verify that the 
	**	     the object on which the permit was created exists
	*/

	switch (pro_tbl.probjtype[0])
	{
	    case DBOB_TABLE:
	    case DBOB_VIEW:
		base = duve_findreltid(&pro_id, duve_cb);

		if (base != DU_INVALID && base != DUVE_DROP)
		{
		    schname_p = duve_cb->duverel->rel[base].du_own;
		    objname_p = duve_cb->duverel->rel[base].du_tab;
		}

		break;
	    case DBOB_DBPROC:
		EXEC SQL repeated select dbp_name, dbp_owner 
		    into :obj_name, :sch_name 
		    from iiprocedure
		    where dbp_id = :pro_tbl.protabbase and
		          dbp_idx = :pro_tbl.protabidx;

		EXEC SQL inquire_sql(:cnt = rowcount);

		base = (cnt == 0) ? DU_INVALID : DU_SQL_OK;

		if (base != DU_INVALID)
		{
		    STtrmwhite(schname_p = sch_name);
		    STtrmwhite(objname_p = obj_name);
		}

		break;
	    case DBOB_EVENT:
		EXEC SQL repeated SELECT event_name, event_owner 
		    INTO :obj_name, :sch_name
		    FROM iievent
		    WHERE event_idbase = :pro_tbl.protabbase AND
		          event_idx = :pro_tbl.protabidx;

		EXEC SQL inquire_sql(:cnt = rowcount);

		base = (cnt == 0) ? DU_INVALID : DU_SQL_OK;

		if (base != DU_INVALID)
		{
		    STtrmwhite(schname_p = sch_name);
		    STtrmwhite(objname_p = obj_name);
		}

		break;
	    case DBOB_SEQUENCE:
		EXEC SQL repeated SELECT seq_name, seq_owner 
		    INTO :obj_name, :sch_name
		    FROM iisequence
		    WHERE seq_idbase = :pro_tbl.protabbase AND
		          seq_idx = :pro_tbl.protabidx;

		EXEC SQL inquire_sql(:cnt = rowcount);

		base = (cnt == 0) ? DU_INVALID : DU_SQL_OK;

		if (base != DU_INVALID)
		{
		    STtrmwhite(schname_p = sch_name);
		    STtrmwhite(objname_p = obj_name);
		}

		break;
	    default:
		base = DUVE_BAD_TYPE;
		break;
	}
	
	if (base == DU_INVALID)
	{
	    char	obj_type[50];
	    i4		type_len;

	    /* this tuple is not in iirelation, iiprocedure or iievent
	    ** so drop from iiprotect */

	    switch (pro_tbl.probjtype[0])
	    {
		case DBOB_TABLE:
		    mode = DB_TABLE; break;
		case DBOB_VIEW:
		    mode = DB_VIEW;  break;
		case DBOB_DBPROC:
		    mode = DB_DBP;   break;
		case DBOB_SEQUENCE:
		    mode = DB_SEQUENCE; break;
		default:
		    mode = DB_EVENT; break;
	    }

	    duve_obj_type((i4) mode, obj_type, &type_len);

	    if (duve_banner( DUVE_IIPROTECT, 1, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);

	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16EB_NO_OBJECT_FOR_PERMIT, 8,
		type_len, obj_type,
		sizeof(pro_tbl.protabbase), &pro_tbl.protabbase, 
		sizeof(pro_tbl.protabidx), &pro_tbl.protabidx, 
		sizeof(pro_tbl.propermid), &pro_tbl.propermid);

	    if (duve_talk(DUVE_ASK, duve_cb,
			S_DU0321_DROP_FROM_IIPROTECT, 4,
			sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	    {
		EXEC SQL delete from iiprotect where current of
			pro_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0371_DROP_FROM_IIPROTECT, 4,
			  sizeof(tid), &tid, sizeof(tidx), &tidx);
	    }
	    continue;
	} /* endif object not found */
	else if (base == DUVE_BAD_TYPE)
	{
	    /*
	    ** type of object on which a permit was defined is not one of those
	    ** we would expect to find
	    */

	    if (duve_banner( DUVE_IIPROTECT, 1, duve_cb) == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU16EA_UNEXPECTED_PERMOBJ_TYPE, 8,
		0, pro_tbl.probjtype,
		sizeof(pro_tbl.protabbase), &pro_tbl.protabbase,
		sizeof(pro_tbl.protabidx), &pro_tbl.protabidx,
		sizeof(pro_tbl.propermid), &pro_tbl.propermid);

	    if (duve_talk(DUVE_ASK, duve_cb,
			S_DU0321_DROP_FROM_IIPROTECT, 4,
			sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	    {
		EXEC SQL delete from iiprotect where current of pro_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0371_DROP_FROM_IIPROTECT, 4,
			  sizeof(tid), &tid, sizeof(tidx), &tidx);
	    }

	    continue;
	}
	else if ( base == DUVE_DROP)
	{
	    continue;	/* if table is marked for destruction, stop now */
	}

	/* 
	**  test 2 - if the permit was defined on a table or view, verify that
	**	     IIRELATION indicates that permits are defined on it
	*/
	if (   (   pro_tbl.probjtype[0] == DBOB_TABLE 
		|| pro_tbl.probjtype[0] == DBOB_VIEW)
	    && (duve_cb->duverel->rel[base].du_stat & TCB_PRTUPS) == 0 )
	{
	    /* the relstat in iirelation needs to be modified */
            if (duve_banner( DUVE_IIPROTECT, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1631_NO_PROTECT_RELSTAT, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);

	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU031E_SET_PRTUPS, 0)
	    == DUVE_YES)
	    {
		basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		relstat = duve_cb->duverel->rel[base].du_stat | TCB_PRTUPS;
		duve_cb->duverel->rel[base].du_stat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
				    reltid = :basetid and reltidx = :basetidx;
		if (sqlca.sqlcode == 0)
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU036E_SET_PRTUPS, 0);
	    }  /* endif fix relstat */

	} /* endif relstat doesnt indicate permits */

/*
**	test 3 (reported error if there was not an entry in iitree for each
**	entry in iipermit) was removed because there will not be an iitree
**	entry unless there is a "where" clause.	  SQL has been modified to
**	stop using create permit and now uses GRANT (which does not permit a
**	where clause.  The usual case is that there will not be an iitree entry.
*/

	/*********************************************************/
	/* test 4 verify query text used to define permit exists */
	/*********************************************************/
	mode = DB_PROT;
	/* if both qryid1 and qryid2 are zero, this is a special DBA owned
	** quel object protect that PSF has put into iiprotect without any
	** qrytext.  If this special case, skip check #4
	**/
	if ( (pro_tbl.proqryid1==0) && (pro_tbl.proqryid2==0))
	{
	    cnt = 1; /* fake cnt to keep from checking this tuple */
	}
	else
	{
	    id1 = pro_tbl.proqryid1;
	    id2 = pro_tbl.proqryid2;
	    EXEC SQL repeated select status into :qtxt_status 
		from iiqrytext 
		where 
		        txtid1 = :id1 and txtid2 = :id2 and mode = :mode 
		    and seq =0;
	    EXEC SQL inquire_sql(:cnt = rowcount);
	}
	if (cnt == 0) 
	{
	    /* missing query text that defined permit */
            if (duve_banner( DUVE_IIPROTECT, 4, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1639_NO_PERMIT_QRYTEXT, 8,
			sizeof(tid), &tid, sizeof(tidx), &tidx,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);
	}

	/*
	** test 5: verify that name of the object and the schema to which it
	**	   belongs which are stored in IIPROTECT match those in the 
	**	   catalog describing the object itself
	*/

	/*
	** need to get rid of trailing blanks in pro_tbl.probjowner and
	** pro_tbl.probjname to make the following comparison meaningful
	*/
	STtrmwhite(pro_tbl.probjowner);
	STtrmwhite(pro_tbl.probjname);
	if (   STcompare(schname_p, pro_tbl.probjowner)
	    || STcompare(objname_p, pro_tbl.probjname))
	{
	    char	obj_type[50];
	    i4		type_len;

	    switch (pro_tbl.probjtype[0])
	    {
		case DBOB_TABLE:
		    mode = DB_TABLE; break;
		case DBOB_VIEW:
		    mode = DB_VIEW;  break;
		case DBOB_DBPROC:
		    mode = DB_DBP;   break;
		case DBOB_SEQUENCE:
		    mode = DB_SEQUENCE; break;
		default:
		    mode = DB_EVENT; break;
	    }

	    duve_obj_type((i4) mode, obj_type, &type_len);

	    /*
	    ** schema and/or object name stored in IIPROTECT do not match thoses
	    ** found in the catalog describing the object itself.
	    */
	    if (duve_banner(DUVE_IIPROTECT, 5, duve_cb) == DUVE_BAD)
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16EC_NAME_MISMATCH, 10,
		type_len, obj_type,
		0, pro_tbl.probjowner,
		0, pro_tbl.probjname,
		0, schname_p,
		0, objname_p);

	    if (duve_talk( DUVE_ASK, duve_cb, S_DU0254_FIX_OBJNAME_IN_IIPROT, 0)
	        == DUVE_YES)
	    {
		STcopy(schname_p, pro_tbl.probjowner);
		STcopy(objname_p, pro_tbl.probjname);
		exec sql update iiprotect 
		    set probjowner = :pro_tbl.probjowner,
			probjname  = :pro_tbl.probjname
		    where current of pro_cursor;

		duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU0454_FIXED_OBJNAME_IN_IIPROT, 0);
	    }
	}

	/* end of test 5 */

	/*
	** test 6 and 7: verify that the privileges specified in proopset and 
	**	         proopctl are valid for the type of object to which this
	**	  	 permit or security_alarm applies.
	*/
        switch (pro_tbl.probjtype[0])
        {
	    case DBOB_TABLE:
	    case DBOB_VIEW:
		/* 
		** bits that may be set depend on whether this tuple represents
		** a permit or a security_alarm
		*/
		if (pro_tbl.proopset & DB_ALARM)
		{
		    valid_privs =   DB_RETRIEVE | DB_REPLACE | DB_DELETE 
				  | DB_APPEND | DB_TEST | DB_AGGREGATE
				  | DB_ALARM | DB_ALMFAILURE | DB_ALMSUCCESS;
		}
		else
		{
		    valid_privs =   DB_RETRIEVE | DB_REPLACE | DB_DELETE
				  | DB_APPEND | DB_TEST | DB_AGGREGATE
				  | DB_COPY_INTO | DB_COPY_FROM  
				  | DB_GRANT_OPTION | DB_CAN_ACCESS; 
		    
		    if (pro_tbl.probjtype[0] == DBOB_TABLE)
			valid_privs |= DB_REFERENCES;
		}

		break;

	    case DBOB_DBPROC:
	        valid_privs = DB_EXECUTE | DB_GRANT_OPTION;
		break;

	    case DBOB_SEQUENCE:
		valid_privs = DB_NEXT | DB_GRANT_OPTION;
		break;

	    default:
	        valid_privs = DB_EVREGISTER | DB_EVRAISE | DB_GRANT_OPTION; 
		break;
        }

	/*
	** pro_tbl.proopset and valid_privs must have more in common than
	** DB_GRANT_OPTION, DB_ALARM, DB_ALMFAILURE, or DB_ALMSUCCESS + 
	** pro_tbl.proopset must be a subset of valid_privs
	*/
	if (   !(  pro_tbl.proopset 
		 & valid_privs 
		 & ~(DB_GRANT_OPTION | DB_ALARM | DB_ALMFAILURE | DB_ALMSUCCESS)
		)
	    || (pro_tbl.proopset & ~valid_privs)
	   )
	{
	    char	obj_type[50];
	    i4		type_len;

	    /*
	    ** proopset specifies privilege which is illegal for the type of 
	    ** object to which this permit or security_alarm applies.
	    */

	    switch (pro_tbl.probjtype[0])
	    {
		case DBOB_TABLE:
		    mode = DB_TABLE; break;
		case DBOB_VIEW:
		    mode = DB_VIEW;  break;
		case DBOB_DBPROC:
		    mode = DB_DBP;   break;
		case DBOB_SEQUENCE:
		    mode = DB_SEQUENCE; break;
		default:
		    mode = DB_EVENT; break;
	    }

	    duve_obj_type((i4) mode, obj_type, &type_len);

	    if (duve_banner( DUVE_IIPROTECT, 6, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);

	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16ED_INVALID_OPSET_FOR_OBJ, 8,
		sizeof(pro_tbl.proopset), &pro_tbl.proopset,
		type_len, obj_type,
		0, schname_p,
		0, objname_p);

	    if (duve_talk(DUVE_ASK, duve_cb,
			S_DU0321_DROP_FROM_IIPROTECT, 4,
			sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	    {
		EXEC SQL delete from iiprotect where current of
			pro_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0371_DROP_FROM_IIPROTECT, 4,
			  sizeof(tid), &tid, sizeof(tidx), &tidx);

	        continue;
	    }
	}

	/*
	** pro_tbl.proopctl and valid_privs must have more in common than
	** DB_GRANT_OPTION, DB_ALARM, DB_ALMFAILURE, or DB_ALMSUCCESS + 
	** pro_tbl.proopctl must be a subset of valid_privs
	*/
	if (   !(  pro_tbl.proopctl 
		 & valid_privs 
		 & ~(DB_GRANT_OPTION | DB_ALARM | DB_ALMFAILURE | DB_ALMSUCCESS)
		)
	    || (pro_tbl.proopctl & ~valid_privs)
	   )
	{
	    char	obj_type[50];
	    i4		type_len;

	    /*
	    ** proopctl specifies privilege which is illegal for the type of 
	    ** object to which this permit or security_alarm applies.
	    */

	    switch (pro_tbl.probjtype[0])
	    {
		case DBOB_TABLE:
		    mode = DB_TABLE; break;
		case DBOB_VIEW:
		    mode = DB_VIEW;  break;
		case DBOB_DBPROC:
		    mode = DB_DBP;   break;
		case DBOB_SEQUENCE:
		    mode = DB_SEQUENCE; break;
		default:
		    mode = DB_EVENT; break;
	    }

	    duve_obj_type((i4) mode, obj_type, &type_len);

	    if (duve_banner( DUVE_IIPROTECT, 7, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);

	    duve_talk(DUVE_MODESENS, duve_cb, S_DU16EE_INVALID_OPCTL_FOR_OBJ, 8,
		sizeof(pro_tbl.proopctl), &pro_tbl.proopctl,
		type_len, obj_type,
		0, schname_p,
		0, objname_p);

	    if (duve_talk(DUVE_ASK, duve_cb,
			S_DU0321_DROP_FROM_IIPROTECT, 4,
			sizeof(tid), &tid, sizeof(tidx), &tidx)
	        == DUVE_YES)
	    {
		EXEC SQL delete from iiprotect where current of
			pro_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0371_DROP_FROM_IIPROTECT, 4,
			  sizeof(tid), &tid, sizeof(tidx), &tidx);

	        continue;
	    }
	}

	/* end of tests 6 and 7 */

	/*
	** tests 8 and 9: verify that info regarding the language used to 
	**		  specify the permit is correct.
	*/
	if (   ~pro_tbl.proopset & DB_ALARM
	    && (pro_tbl.proqryid1 != 0 || pro_tbl.proqryid2 != 0))
	{
	    /* this a permit other than an access permit */
	    if (~qtxt_status & DBQ_SQL && pro_tbl.proflags & DBP_SQL_PERM)
	    {
	        char		obj_type[50];
	        i4		type_len;
    
	        switch (pro_tbl.probjtype[0])
	        {
		    case DBOB_TABLE:
		        mode = DB_TABLE; break;
		    case DBOB_VIEW:
		        mode = DB_VIEW;  break;
		    case DBOB_DBPROC:
		        mode = DB_DBP;   break;
		    case DBOB_SEQUENCE:
			mode = DB_SEQUENCE; break;
		    default:
		        mode = DB_EVENT; break;
	        }
    
	        duve_obj_type((i4) mode, obj_type, &type_len);
    
	        /*
	        ** IIQRYTEXT indicates that the permit was created using QUEL, 
		** but IIPROTECT tuple indicates that it was created using 
		** SQL - assume that IIQRYTEXT knows best
	        */
	        if (duve_banner(DUVE_IIPROTECT, 8, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS) DUVE_BAD);
	        
	        duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU16EF_QUEL_PERM_MARKED_AS_SQL, 8,
		    sizeof(pro_tbl.propermid), &pro_tbl.propermid,
		    type_len, obj_type,
		    0, schname_p,
		    0, objname_p);
    
	        if (duve_talk( DUVE_ASK, duve_cb, 
			S_DU0255_SET_PERM_LANG_TO_QUEL, 0)
	            == DUVE_YES)
	        {
		    pro_tbl.proflags &= ~DBP_SQL_PERM;
		    exec sql update iiprotect 
		        set proflags = :pro_tbl.proflags
		        where current of pro_cursor;
    
		    duve_talk(DUVE_MODESENS, duve_cb, 
		        S_DU0455_SET_PERM_LANG_TO_QUEL, 0);
	        }
	    }
	    else if (qtxt_status & DBQ_SQL && ~pro_tbl.proflags & DBP_SQL_PERM)
	    {
	        char		obj_type[50];
	        i4		type_len;
    
	        switch (pro_tbl.probjtype[0])
	        {
		    case DBOB_TABLE:
		        mode = DB_TABLE; break;
		    case DBOB_VIEW:
		        mode = DB_VIEW;  break;
		    case DBOB_DBPROC:
		        mode = DB_DBP;   break;
		    case DBOB_SEQUENCE:
			mode = DB_SEQUENCE; break;
		    default:
		        mode = DB_EVENT; break;
	        }
    
	        duve_obj_type((i4) mode, obj_type, &type_len);
    
	        /*
	        ** IIQRYTEXT indicates that the permit was created using SQL, 
		** but IIPROTECT tuple indicates that it was created using 
		** QUEL - assume that IIQRYTEXT knows best
	        */
	        if (duve_banner(DUVE_IIPROTECT, 9, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS) DUVE_BAD);
	        
	        duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU16F0_SQL_PERM_MARKED_AS_QUEL, 8,
		    sizeof(pro_tbl.propermid), &pro_tbl.propermid,
		    type_len, obj_type,
		    0, schname_p,
		    0, objname_p);
    
	        if (duve_talk( DUVE_ASK, duve_cb, 
			S_DU0256_SET_PERM_LANG_TO_SQL, 0)
	            == DUVE_YES)
	        {
		    pro_tbl.proflags |= DBP_SQL_PERM;
		    exec sql update iiprotect 
		        set proflags = :pro_tbl.proflags
		        where current of pro_cursor;
    
		    duve_talk(DUVE_MODESENS, duve_cb, 
		        S_DU0456_SET_PERM_LANG_TO_SQL, 0);
	        }
	    }
	}

	/* end of tests 8 and 9 */

	/*
	** test 10: if a description of a permit indicates that it was created 
	** 	    after 6.4, verify that it specifies exactly one privilege
	** test 11: if a description of a permit indicates that it was created 
	**	    in 6.4 or prior, warn the user that he should consider 
	**	    running UPGRADEDB over his database
	** test 12: if the grantor of a permit is not the owner of the object,
	**	    verify that he/she possesses GRANT OPTION FOR privilege 
	**	    being granted
	*/
	if (!(pro_tbl.proopset & (DB_ALARM | DB_CAN_ACCESS)))
	{
	    if (pro_tbl.proflags & DBP_65_PLUS_PERM)
	    {
		long		priv;

		/* 
		** we want to make sure that the permit conveys exactly one 
		** privilege - turn off extraneous bits whicxh may be set in 
		** the permit tuple
		*/
		priv =   pro_tbl.proopset 
		       & ~(DB_GRANT_OPTION | DB_TEST | DB_AGGREGATE);
		
		if (BTcount((char *) &priv, BITS_IN(priv)) > 1)
		{
	            char	obj_type[50];
	            i4		type_len;
        
	            switch (pro_tbl.probjtype[0])
	            {
		        case DBOB_TABLE:
		            mode = DB_TABLE; break;
		        case DBOB_VIEW:
		            mode = DB_VIEW;  break;
		        case DBOB_DBPROC:
		            mode = DB_DBP;   break;
			case DBOB_SEQUENCE:
			    mode = DB_SEQUENCE; break;
		        default:
		            mode = DB_EVENT; break;
	            }
        
	            duve_obj_type((i4) mode, obj_type, &type_len);
        
	            /*
	            ** IIQRYTEXT indicates that the permit was created 
		    ** after 6.4, but it conveys more than one privilege which 
		    ** would not happen in 6.5/1.0
	            */
	            if (duve_banner(DUVE_IIPROTECT, 10, duve_cb) == DUVE_BAD)
		        return ( (DU_STATUS) DUVE_BAD);
	            
	            duve_talk(DUVE_MODESENS, duve_cb, S_DU16F1_MULT_PRIVS, 8,
		        sizeof(pro_tbl.propermid), &pro_tbl.propermid,
		        type_len, obj_type,
		        0, schname_p,
		        0, objname_p);
        
	            if (duve_talk( DUVE_ASK, duve_cb, 
			    S_DU0257_PERM_CREATED_BEFORE_10, 0)
	                == DUVE_YES)
	            {
		        pro_tbl.proflags &= ~DBP_65_PLUS_PERM;
		        exec sql update iiprotect 
		            set proflags = :pro_tbl.proflags
		            where current of pro_cursor;
        
		        duve_talk(DUVE_MODESENS, duve_cb, 
		            S_DU0457_PERM_CREATED_BEFORE_10, 0);
	            }
		}
	    }

	    /* end of test 10 */

	    /* test 11 */

	    if (~pro_tbl.proflags & DBP_65_PLUS_PERM)
	    {
	        char	obj_type[50];
	        i4	type_len;
        
	        switch (pro_tbl.probjtype[0])
	        {
		    case DBOB_TABLE:
		        mode = DB_TABLE; break;
		    case DBOB_VIEW:
		        mode = DB_VIEW;  break;
		    case DBOB_DBPROC:
		        mode = DB_DBP;   break;
		    case DBOB_SEQUENCE:
			mode = DB_SEQUENCE; break;
		    default:
		        mode = DB_EVENT; break;
	        }
        
	        duve_obj_type((i4) mode, obj_type, &type_len);
        
	        /*
	        ** IIPROTECT indicates that this permit was created prior to 6.5
	        */
	        if (duve_banner(DUVE_IIPROTECT, 11, duve_cb) == DUVE_BAD)
		    return ( (DU_STATUS) DUVE_BAD);
	            
	        duve_talk(DUVE_MODESENS, duve_cb, S_DU16F2_PRE_10_PERMIT, 8,
		    sizeof(pro_tbl.propermid), &pro_tbl.propermid,
		    type_len, obj_type,
		    0, schname_p,
		    0, objname_p);
	    }

	    /* end of test 11 */

	    /* test 12 */

	    /*
	    ** need to trim trailing blanks from pro_tbl.prograntor if the 
	    ** following comparison is to work correctly
	    */
	    STtrmwhite(pro_tbl.prograntor);

	    if (STcompare(pro_tbl.prograntor, pro_tbl.probjowner))
	    {
	        u_i4		attr_map[DB_COL_WORDS];
	        STATUS          mem_stat;
	        bool		satisfied;

		/* 
		** grantor of the permit does not own the object itself - verify
		** that he possesses GRANT OPTION FOR privilege conveyed by the 
		** permit
		**
		** If he does not possess them, we will report this fact to the
		** user and ask for permission to deal with offending object(s)
		** as follows:  
		**   - if the permit was granted on a table, view, or dbevent,
		**     we will mark that object for destruction;
		**   - if the permit was granted on a dbproc, that dbproc will 
		**     be marked dormant
		*/

		MEcopy(&pro_tbl.prodomset[0], sizeof(attr_map), (char *) &attr_map[0]);

		duve_check_privs(&pro_id, 
		    (i4) (pro_tbl.proopset | DB_GRANT_OPTION),
		    pro_tbl.prograntor, attr_map, &satisfied);

		if (!satisfied)
		{
	            char	obj_type[50];
	            i4		type_len;
            
		    /*
		    ** grantor of the permit lacks GRANT OPTION FOR privilege 
		    ** conveyed in this permit; if the permit was granted on 
		    ** table, view, or dbevent, that object will be marked for 
		    ** destruction; if the permit was granted on a dbproc, the 
		    ** dbproc will be marked dormant
		    */

	            switch (pro_tbl.probjtype[0])
	            {
		        case DBOB_TABLE:
		            mode = DB_TABLE; break;
		        case DBOB_VIEW:
		            mode = DB_VIEW;  break;
		        case DBOB_DBPROC:
		            mode = DB_DBP;   break;
			case DBOB_SEQUENCE:
			    mode = DB_SEQUENCE; break;
		        default:
		            mode = DB_EVENT; break;
	            }
            
	            duve_obj_type((i4) mode, obj_type, &type_len);
        
	            if (duve_banner(DUVE_IIPROTECT, 12, duve_cb) == DUVE_BAD)
		        return ( (DU_STATUS) DUVE_BAD);
	            
	            duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU16F3_LACKING_GRANT_OPTION, 10,
			0, pro_tbl.prograntor,
		        sizeof(pro_tbl.propermid), &pro_tbl.propermid,
		        type_len, obj_type,
		        0, schname_p,
		        0, objname_p);

		    if (mode == DB_TABLE || mode == DB_VIEW)
		    {
			char tmp_tabname[DB_MAXNAME*2 + 2];
			STprintf(tmp_tabname,ERx("%s.%s"),
				duve_cb->duverel->rel[base].du_own,
				duve_cb->duverel->rel[base].du_tab);
			duve_talk( DUVE_MODESENS, duve_cb, S_DU16F6_INVALID_IIPROTECT, 10,
				sizeof(pro_tbl.protabbase),&pro_tbl.protabbase,
				sizeof(pro_tbl.protabidx),&pro_tbl.protabidx,
				0, pro_tbl.prouser,
				sizeof(pro_tbl.proopset),&pro_tbl.proopset,
				0, tmp_tabname);
		    }
		    else if (mode == DB_DBP)
		    { 
			DUVE_DROP_OBJ		*drop_obj;

			/*
			** before asking user whether we may mark the dbproc 
			** dormant, check the last entry added to the "fix it" 
			** list - if it describes the current procedure, we 
			** will avoid issuing redundant messages
			*/
			if (   !(drop_obj = 
				     duve_cb->duvefixit.duve_objs_to_drop)
			    || drop_obj->duve_obj_id.db_tab_base   != 
				   pro_tbl.protabbase
			    || drop_obj->duve_obj_id.db_tab_index  != 
				   pro_tbl.protabidx
			    || drop_obj->duve_obj_type 		   != DB_DBP
			   )
			{
			    if (duve_talk(DUVE_IO, duve_cb, 
				    S_DU0251_MARK_DBP_DORMANT, 0)
				== DUVE_YES)
			    {

				/* 
				** user decided to allow us to mark the dbproc 
				** dormant - add its description to the "fixit" 
				** list 
				*/

				drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0,
				    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
				if (mem_stat != OK || drop_obj == NULL)
				{
				    duve_talk (DUVE_ALWAYS, duve_cb, 
					E_DU3400_BAD_MEM_ALLOC, 0);
				    return ((DU_STATUS) DUVE_BAD);
				}

				drop_obj->duve_obj_id.db_tab_base  = 
				    pro_tbl.protabbase;
				drop_obj->duve_obj_id.db_tab_index = 
				    pro_tbl.protabidx;
				drop_obj->duve_obj_type            = DB_DBP;
				drop_obj->duve_secondary_id	   = 0;
				drop_obj->duve_drop_flags	   = 0;

				/* 
				** link this description into the existing list 
				*/
				drop_obj->duve_next = 
				    duve_cb->duvefixit.duve_objs_to_drop;

				duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
				duve_talk(DUVE_MODESENS, duve_cb, 
				    S_DU0451_MARKED_DBP_DORMANT, 0);
			    }
			}
		    }
		    else if (mode == DB_EVENT)
		    { 
			DUVE_DROP_OBJ		*drop_obj;

			/*
			** before asking user whether we may drop the dbevent,
			** check the last entry added to the "fix it" list - if
			** it describes the current dbevent, we will avoid 
			** issuing redundant messages
			*/
			if (   !(drop_obj = 
				     duve_cb->duvefixit.duve_objs_to_drop)
			    || drop_obj->duve_obj_id.db_tab_base   != 
				   pro_tbl.protabbase
			    || drop_obj->duve_obj_id.db_tab_index  != 
				   pro_tbl.protabidx
			    || drop_obj->duve_obj_type 		   != DB_EVENT
			   )
			{
			    if (duve_talk(DUVE_IO, duve_cb, 
				    S_DU0258_DROP_DBEVENT, 0)
				== DUVE_YES)
			    {

				/* 
				** user decided to allow us to drop the 
				** dbevent - add its description to the "fixit"
				** list 
				*/

				drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0,
				    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
				if (mem_stat != OK || drop_obj == NULL)
				{
				    duve_talk (DUVE_ALWAYS, duve_cb, 
					E_DU3400_BAD_MEM_ALLOC, 0);
				    return ((DU_STATUS) DUVE_BAD);
				}

				drop_obj->duve_obj_id.db_tab_base  = 
				    pro_tbl.protabbase;
				drop_obj->duve_obj_id.db_tab_index = 
				    pro_tbl.protabidx;
				drop_obj->duve_obj_type            = DB_EVENT;
				drop_obj->duve_secondary_id	   = 0;
				drop_obj->duve_drop_flags	   = 0;

				/* 
				** link this description into the existing list 
				*/
				drop_obj->duve_next = 
				    duve_cb->duvefixit.duve_objs_to_drop;

				duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
				duve_talk(DUVE_MODESENS, duve_cb, 
				    S_DU0458_DROPPED_DBEVENT, 0);
			    }
			}
		    }
		}
	    }
	    /* end of test 12 */
	}
    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckprotups"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckqrytxt - run verifydb checks on the iiqrytext dbms catalog
**
** Description:
**      This routine opens a cursor and examines each tuple in iiqrytext. 
**      It reports errors, but does not attempt to correct them (in any mode).
**	The logic is that the text used to create views, integrities, and
**	permits is valuable.  Even a partial qrytext entry may provide the
**	DBA with enough information to create a view, permit, integrity. 
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**	none
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
** Side Effects:
**	    none
**
** History:
**      5-Sep-1988 (teg)
**          initial creation
**	20-Oct-1988 (teg)
**	    added check to test 49 for REGISTER, LINK and GATEWAY
**	21-Feb-1989 (teg)
**	    Fixed logic in tests 51 & 52 which used to assume information they
**	    needed was in the RELINFO cache.  This was not adequate because
**	    it had an invalid underlaying assumption that only one
**	    integrity or permit would be defined on a table.
**	04-Apr-1989 (neil)
**	    Added checks for rule text.  At this time this is only
**	    added to suppress failures and not to actually test the
**	    relationships between iiqrytext, and iirule.
**	03-Jan-89 (neil)
**	    Added checks for event text.  This is only added to suppress
**	    failures and not to test relationships.
**	12-Oct-90 (teresa)
**	    Pick Up GW change: 23-May-90 (linda)
**          Changed checks of q.mode in iiqrytext table to reflect changes of
**          constant names for "create...as link", "register...as link" and
**          "register...as import" statements.  *NOTE* when integrating STAR:
**          constant DB_REG_LINK used to be DB_REGISTER; it was changed to
**          reflect is real meaning which is "register...as link".
**	21-feb-91 (teresa)
**	    fix incomplete ifdeffed code for rules and events for test 50.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	27-may-93 (teresa)
**	    Added handling for DBPs (test 8)
**	01-nov-93 (andre)
**	    since we stopped using timestamps in attributes used to join 
**	    IIQRYTEXT/IITREE tuples to tuples in catalogs describing objects 
**	    with which they are affiliated, it is now possible to have a tuple 
**	    containing integrity text to have the same qryid as the tuple 
**	    containing permit text (for the same table).  This should not be a 
**	    problem because the two will have different "mode".  This test will
**	    be modified to NOT treat as error occurrences of text tuples with 
**	    identical qryid if they were created for DIFFERENT object types.
**	    Got rid of a couple of unused variables; folded IF statement into
**	    the switch statement to make things a tad more efficient
**	10-dec-93 (teresa)
**	    rewrite cursor as an outer join to improve performance as this has
**	    been determined to be a very costly portion of verifydb.  Rework
**	    switch statement significantly to handle outer join change.  This
**	    is part of an effort to improve performance for verifydb.  Also
**	    added logic to report peformance statistics
**	20-dec-93 (robf)
**          Add support for security alarms (DB_ALM)
[@history_template@]...
*/
DU_STATUS
ckqrytxt(duve_cb)
DUVE_CB            *duve_cb;
{
#define		IS_NULL		-1
	char		*test;
	i4		qid1,qid2;
	i4		mode;
	i4		msg,qseq;
        DU_STATUS	base;

    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duveqry.sh>;
	i4 cnt;
	u_i8	tid8;
	i2	vmode;
	i2	null_i;
    EXEC SQL END DECLARE SECTION;


    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    
    
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* make a union all join of all the objects for which query text may exist.
    ** note that this union view does NOT include views because views are
    ** already cached on the iirelation cache, so way waste the I/Os querying
    ** iirelation for them again...
    */
    exec sql declare global temporary table session.vdb_qry (id1, id2, vmode) as
            select proqryid1, proqryid2, 19 from iiprotect
	union all
	    select event_qryid1, event_qryid2, 33 from iievent
	union all
	    select intqryid1, intqryid2, 20 from iiintegrity
	union all
	    select dbp_txtid1, dbp_txtid2, 87 from iiprocedure
	union all
	    select rule_qryid1, rule_qryid2, 110 from iirule
	on commit preserve rows with norecovery;

    EXEC SQL DECLARE qry_cursor CURSOR FOR
	select distinct
	    q.tid, q.txtid1, q.txtid2, q.mode, q.seq, q.status, v.vmode 
	from (iiqrytext q left join session.vdb_qry v on q.txtid1 = v.id1 and
						 q.txtid2 = v.id2 and
						 q.mode = v.vmode)
        order by q.txtid1, q.txtid2, q.mode, q.seq;
    EXEC SQL open qry_cursor;

    /* loop for each tuple in iiqrytext */
    for (qid1 = qid2 = mode = -1;;)  
    {
	/* get tuple from table */
	EXEC SQL FETCH qry_cursor INTO :tid8, :qry_tbl.txtid1, :qry_tbl.txtid2,
	    :qry_tbl.mode, :qry_tbl.seq, :qry_tbl.status, :vmode:null_i;
	    
	if (sqlca.sqlcode == 100) /* then no more tuples in iiqrytext */
	{
	    EXEC SQL CLOSE qry_cursor;
	    break;
	}

	/* determine if this is a new query text id */
	if (   (qry_tbl.txtid1 != qid1) 
	    || (qry_tbl.txtid2 != qid2) 
	    || (qry_tbl.mode != mode))
	{
        /* this is the first tuple for this query text entry  */
	    qid1 = qry_tbl.txtid1;
	    qid2 = qry_tbl.txtid2;
	    mode = qry_tbl.mode;
	    qseq = 0;

	    /* test 1 - assure text id is valid */
	    if (qid1 == 0  &&  qid2 == 0)
	    {
		if (duve_banner( DUVE_IIQRYTEXT, 1, duve_cb)
		== DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb,S_DU163B_INVALID_QRYTEXT_ID,0);
	    }

	    /* tests 2, 3, 4, 5, 7, 8, 9
	    **  -- assure there is recepient for query text
	    */
	    if (qry_tbl.mode == DB_VIEW)
	    {
		base = duve_qidfind( qid1, qid2, duve_cb);
		if (base == DU_INVALID)
		{   
		    /* test 3 - view corresponding to qrytext does not exist */
		    if (duve_banner( DUVE_IIQRYTEXT, 3, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU163C_QRYTEXT_VIEW_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		}
	    }
	    else if (null_i == (i2) IS_NULL)
	    {
		/* there exists a query text without a corresponding object,
		** so figure out which one and report it -- this works because
		** of the outer join -- if there is a corresponding tuple from
		** the various tables (iiprotect, iievent, iiintegrity,
		** iiprocedure, iirule) then vmode will have the same value as
		** qry_tbl.mode.   If there is no corresponding tuple in the
		** above list of catalogs, then vmode will have a value of NULL.
		** Note that views are NOT handled in this particular query
		** because relation information has already been cached, so we
		** look for views on the relation cache -- vmode will always
		** be null when qry_tbl.mode is DB_VIEW (17).
		*/
		/* NOTE: esqlc puts nulls into a separate indicator, null_i in
		**	 this case.  The value of null_i is set to -1 if there
		**	 is a null indicator and is not guaranteed (except that
		**	 it is NOT -1) if there is not a null indicator.  The
		**	 value of vmode is also not guarenteed from platform to
		**	 platform -- it would be nice if it were guarenteed to
		**	 be zero when there are nulls, or some nice known value.
		**	 Ideally, we'd like to test for
		**		if (qry_tbl.mode != vmode)
		**	 but we cannot do that because of possible FE
		**	 inconsistencies on some platforms, we instead test
		**	 for the null indicator
		*/

		switch (qry_tbl.mode){
		  case DB_PROT:
		    /* test 4 - permit corresponding to qrytext does not exist*/
		    if (duve_banner( DUVE_IIQRYTEXT, 4, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU163D_QRYTEXT_PERMIT_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		    break;
		  case DB_INTG:
		    /* test 5-integrity corresponding to qrytext does not exist */
		    if (duve_banner( DUVE_IIQRYTEXT, 5, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU163E_QRYTEXT_INTEG_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		    break;
		  case DB_DBP:
		    /* test 8 - DBP corresponding to qrytext does not exist */
		    if (duve_banner( DUVE_IIQRYTEXT,8, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU167C_QRYTEXT_DBP_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		    break;
	          case DB_RULE:
		    if (duve_banner( DUVE_IIQRYTEXT, 9, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU1674_QRYTEXT_RULE_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		    break;
	          case DB_ALM:
		    if (duve_banner( DUVE_IIQRYTEXT, 9, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
			        S_DU16D0_QRYTEXT_ALARM_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
			break;
		 case DB_EVENT:
		    /* test 7 - event corresponding to qrytext does not exist */
		    if (duve_banner( DUVE_IIQRYTEXT, 7, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk ( DUVE_MODESENS, duve_cb, 
				S_DU1675_QRYTEXT_EVENT_MISSING, 4, 
				sizeof(qid1), &qid1, sizeof(qid2), &qid2);
		    break;
		 case DB_CRT_LINK:
		 case DB_REG_LINK:
		 case DB_REG_IMPORT:
		    /* looks like no checking is done for these objects. */
		    break;
		 default:
		    /* test 2 -- text mode is NOT recognizable to dbms */
		    if (duve_banner( DUVE_IIQRYTEXT, 2, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU163A_UNKNOWN_QRYMODE,
				6, sizeof(qid1), &qid1, sizeof(qid2), &qid2,
				   sizeof(qry_tbl.mode), &qry_tbl.mode);
		    break;
		} /* end case */
	    } /* endif the object of the query text does not exist */

	}  /* endif start of new text entry */

	/*
	**  the following tests are performed on all tuples that comprise a
	**  query text entry.
	*/

	/* test 2 -- assure text mode is recognizable to dbms */
	if ( (qry_tbl.mode != DB_VIEW) && (qry_tbl.mode != DB_PROT) &&
	     (qry_tbl.mode != DB_CRT_LINK) && (qry_tbl.mode != DB_REG_LINK) &&
	     (qry_tbl.mode != DB_REG_IMPORT) &&
	     (qry_tbl.mode != DB_ALM) &&
	     (qry_tbl.mode != DB_INTG) && (qry_tbl.mode != DB_DBP ) &&
	     (qry_tbl.mode != DB_RULE) && (qry_tbl.mode != DB_EVENT) )
	{
	    if (duve_banner( DUVE_IIQRYTEXT, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU163A_UNKNOWN_QRYMODE, 6,
			    sizeof(qid1), &qid1, sizeof(qid2), &qid2,
			    sizeof(qry_tbl.mode), &qry_tbl.mode);
	}

        /* test 6 -- check for sequence errors */
	if ( qry_tbl.seq == qseq )
	    qseq++;			/* all is well */
	else if (qry_tbl.seq > qseq)
	{
	    /* missing tree sequence tuple */
            if (duve_banner( DUVE_IIQRYTEXT, 6, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU164F_QRYTEXT_SEQUENCE_ERR, 
			6, sizeof(qid1), &qid1, sizeof(qid2), &qid2,
			sizeof(qseq), &qseq);
	    qseq = qry_tbl.seq + 1;
	}
	else
	{
	    /* duplicate tree sequence tuple */
            if (duve_banner( DUVE_IIQRYTEXT, 6, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1650_QRYTEXT_DUP_ERR, 6,
			sizeof(qid1), &qid1, sizeof(qid2), &qid2,
			sizeof(qry_tbl.seq), &qry_tbl.seq);
	    qseq = qry_tbl.seq + 1;
	}
    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckqrytxt");
    
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckrule - run verifydb tests to check system catalog iirule.
**
** Description:
**      This routine opens a cursor to walk iirule one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iirule system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iirule
**	    tuples may be modified in iirelation
**
** History:
**	27-May-93 (teresa)
**	    Initial creation
**	23-jun-93 (teresa)
**	    Fixed test 5 to look for all blanks as well as '\0'
**      10-oct-93 (anitap)
**          Added test to check if corresponding iidbdepends tuple
**          present for the rule tuple. If not present, delete the
**          corresponding rule tuple from iirule.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (anitap)
**	    Use iidbdepends cache for test 6 to improve performance.
**	    Corrected test number to 6 in duve_banner().
[@history_template@]...
*/
DU_STATUS
ckrule(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_TAB_ID		rule_id;
    DU_STATUS		base;

    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duverule.sh>;
	i4 cnt;
	u_i4	basetid, basetidx;
	i4	relstat;
	i4	id1, id2;
	i2	mode;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE rule_cursor CURSOR FOR
	select * from iirule;
    EXEC SQL open rule_cursor;

    /* loop for each tuple in iirule */
    for (;;)  
    {
	/* get tuple from iirule */
	EXEC SQL FETCH rule_cursor INTO :rule_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iirule */
	{
	    EXEC SQL CLOSE rule_cursor;
	    break;
	}

	rule_id.db_tab_base = rule_tbl.rule_tabbase;
	rule_id.db_tab_index = rule_tbl.rule_tabidx;

	/* test 1 -- verify table receiving rule exists */
	base = duve_findreltid ( &rule_id, duve_cb);
	if ( (base == DU_INVALID) || ( base == DUVE_DROP) )
	{
	    /* this tuple is not in iirelation so drop from iirule */

	    if (duve_banner( DUVE_IIRULE, 1, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb,
			S_DU1682_NO_BASE_FOR_RULE, 4,
			0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
	    if ( duve_talk(DUVE_IO, duve_cb,
			S_DU0345_DROP_FROM_IIRULE, 4,
			0, rule_tbl.rule_name, 0, rule_tbl.rule_owner)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iirule where current of
			rule_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0395_DROP_FROM_IIRULE, 4,
			  0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
	    }
	    continue;
	} /* endif base table not found or is marked for delete */

	/* 
	**  test 2 - verify base table indicates rules are defined on it
	*/
	if ( (duve_cb->duverel->rel[base].du_stat & TCB_RULE) == 0 )
	{
	    /* the relstat in iirelation needs to be modified */
            if (duve_banner( DUVE_IIRULE, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1683_NO_RULE_RELSTAT, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);

	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU0346_SET_RULE, 0)
	    == DUVE_YES)
	    {
		basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		relstat = duve_cb->duverel->rel[base].du_stat | TCB_PRTUPS;
		duve_cb->duverel->rel[base].du_stat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
				    reltid = :basetid and reltidx = :basetidx;
		if (sqlca.sqlcode == 0)
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0396_SET_RULE, 0);
	    }  /* endif fix relstat */

	} /* endif relstat doesnt indicate permits */

	/* test 3 verify query text used to define rule exists */
	mode = DB_RULE;
	id1 = rule_tbl.rule_qryid1;
	id2 = rule_tbl.rule_qryid2;
	EXEC SQL repeated select any(txtid1) into :cnt from iiqrytext where
	    txtid1 = :id1 and txtid2 = :id2 and mode = :mode;
	if (cnt == 0)
	{
	    /* missing query text that defined rule */
	    if (duve_banner( DUVE_IIRULE, 3, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1684_NO_RULE_QRYTEXT, 8,
			sizeof(id1), &id1, sizeof(id2), &id2,
			0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
	    if ( duve_talk(DUVE_IO, duve_cb,
			S_DU0345_DROP_FROM_IIRULE, 4,
			0, rule_tbl.rule_name, 0, rule_tbl.rule_owner)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iirule where current of
			rule_cursor;
		duve_talk(DUVE_MODESENS,duve_cb,
			  S_DU0395_DROP_FROM_IIRULE, 4,
  			  0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
	    }
	    continue;
	}

	/* test 4 -- verify referenced procedure really exists */
	if (rule_tbl.rule_dbp_name[0])
	{
	    exec sql repeated select any(dbp_name) into :cnt from iiprocedure
	        where dbp_name = :rule_tbl.rule_dbp_name and
		      dbp_owner = :rule_tbl.rule_dbp_owner;
	    if (cnt== 0)
	    {
		if (duve_banner( DUVE_IIRULE, 4, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU1685_NO_DBP_FOR_RULE, 8,
			   0, rule_tbl.rule_dbp_name, 0,rule_tbl.rule_dbp_owner,
			   0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);

		/* procedure does not exist, so drop rule */
		if ( duve_talk(DUVE_IO, duve_cb,
			    S_DU0345_DROP_FROM_IIRULE, 4,
			    0, rule_tbl.rule_name, 0, rule_tbl.rule_owner)
		== DUVE_YES)
		{
		    EXEC SQL delete from iirule where current of
			    rule_cursor;
		    duve_talk(DUVE_MODESENS,duve_cb,
			      S_DU0395_DROP_FROM_IIRULE, 4,
			      0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
		}
		continue;
	    }  /* endif referenced procedure does not exist */
	}  

	/* test 5 - if timebased rule, verify that a time is specified */
	if (rule_tbl.rule_type == DBR_T_TIME)
	{
	    if ( ( (rule_tbl.rule_time_date[0] == '\0')	      &&
		   (rule_tbl.rule_time_int[0] == '\0')		 )  ||
		 ( (STcompare(rule_tbl.rule_time_date,
			      "                         ")==0) &&
		   (STcompare(rule_tbl.rule_time_int,
			      "                         ")==0)    )
	       )
	    {
		/* time interval and date both missing from a time based
		** rule so it will never fire */
		if (duve_banner( DUVE_IIRULE, 5, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk ( DUVE_MODESENS, duve_cb,
			    S_DU1686_NO_TIME_FOR_RULE, 4,
			    0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
		if ( duve_talk(DUVE_IO, duve_cb,
			    S_DU0345_DROP_FROM_IIRULE, 4,
			    0, rule_tbl.rule_name, 0, rule_tbl.rule_owner)
		== DUVE_YES)
		{
		    EXEC SQL delete from iirule where current of
			    rule_cursor;
		    duve_talk(DUVE_MODESENS,duve_cb,
			      S_DU0395_DROP_FROM_IIRULE, 4,
			      0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
		}
		continue;
	    }
	} /* endif time/interval missing */

	/* test 6 -- verify iidbdepends tuple exists for the
        ** rule tuple.
        */

	/* check for WCO rule by checking rule_flags.
	** Rule for WCO does not have entry in iidbdepends.
	** So do not delete that rule.
	** Rules supporting constraints are system generated and are not
	** user droppable.
	** Rules created by users do not have corresponding tuples in
	** iidbdepends.
 	*/
	if (((rule_tbl.rule_flags & 
		DBR_F_CASCADED_CHECK_OPTION_RULE) == 0) &&
		((rule_tbl.rule_flags & DBR_F_NOT_DROPPABLE) != 0) && 
                ((rule_tbl.rule_flags &	DBR_F_SYSTEM_GENERATED) != 0))
	{

	   DU_STATUS 	rule_dep_offset;

	   rule_dep_offset = duve_d_dpndfind(rule_tbl.rule_id1,
				rule_tbl.rule_id2, (i4) DB_RULE, 
				(i4) 0, duve_cb);	

	   if (rule_dep_offset == DU_INVALID)
	   {
	       /* this tuple is not in iidbdepends so drop from iirule */
	
	       if (duve_banner( DUVE_IIRULE, 6, duve_cb) == DUVE_BAD)
                  return ( (DU_STATUS) DUVE_BAD);
	       duve_talk( DUVE_MODESENS, duve_cb, S_DU16A8_IIDBDEPENDS_MISSING,
		 4, 0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);

	       if ( duve_talk(DUVE_IO, duve_cb,
		S_DU0345_DROP_FROM_IIRULE, 4,
                        0, rule_tbl.rule_name, 0, rule_tbl.rule_owner)
            		== DUVE_YES)
               {
                  EXEC SQL delete from iirule where current of
                       rule_cursor;
                  duve_talk(DUVE_MODESENS, duve_cb,
                         S_DU0395_DROP_FROM_IIRULE, 4,
                         0, rule_tbl.rule_name, 0, rule_tbl.rule_owner);
               }
               continue;
	    } /* endif corresponding iidbdepends tuple does not exist */
	} /* end test 6 */

    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckrule"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckstat	- run verifydb tests to check system catalog iistatistics
**	         
**
** Description:
**      This routine opens a cursor to walk iistatistics one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iistatistics system catalog.
**
**	It allocates memory to keep track of statistic and histogram info
**	in order to reduct the number of disk accesses required for checking
**	the catalogs.  It operates under the assumption that the catalog will
**	normally be healthy.  Thus it gathers info from iistatistics, then
**	test ckhist gathers info from iihistogram.  If any tuples are to be
**	deleted, this is cached, and deletion occurs after all tuples from
**	both tables are examined.  Function ckhist may cause iistatistics
**	tuples to be updated (snumcels modified) or to delete tuples
**	from iistatistics.  Function ckhist may also delete tuples from
**	iihistogram.
**
**	Since there are several error exist possible, and all of them exit
**	through duve_close, duve_close will be responsible for deallocating
**	any memory that was allocated by ckstat.
**
**
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	    duvestat			    cached iistatistics information
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iistatistics may be modified
**	    tuples in iirelation may be modified
**	    view/prot/etc that tree defines may be dropped from database
**
** History:
**      29-aug-88   (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	07-jan-94 (teresa)
**	    dont open cursor until we're sure there are tuples in iistatistics
**	26-jun-2000 (thaju02)
**	    Upon entry, initialize duve_cb->duve_scnt to zero. (b101942)
[@history_template@]...
*/

DU_STATUS
ckstat(duve_cb)
DUVE_CB            *duve_cb;
{
  
    DB_TAB_ID	   statid;
    i4	   repeat;
    DU_STATUS	   base;
    i2		   atno = -1;
    u_i4		size;
    STATUS		mem_stat;
    
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvestat.sh>;
	i4 cnt;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i2	satno;
	i4	relstat;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    duve_cb -> duve_scnt = 0;	/* will be incremented before it is used */

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory for duvestat */
    EXEC SQL select count(stabbase) into :cnt from iistatistics;
    if ( (sqlca.sqlcode == 100) || (cnt == 0) )
    {	
	return ( (DU_STATUS) DUVE_NO);
    }
    size = cnt * sizeof(DU_STAT);
    duve_cb->duvestat = (DUVE_STAT *) MEreqmem(0, size, TRUE, &mem_stat);
    if ( (mem_stat != OK) || (duve_cb->duvestat == 0) )
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }
    
    EXEC SQL DECLARE stat_cursor CURSOR FOR
	select * from iistatistics order by stabbase, stabindex, satno;
    EXEC SQL open stat_cursor;

    /* loop for each tuple in iistatistics */
    for (statid.db_tab_base= -1,statid.db_tab_index= -1;;
	 duve_cb->duve_scnt++, repeat++)
    {
	/* get tuple from table */
	EXEC SQL FETCH stat_cursor INTO :stat_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in table */
	{
	    EXEC SQL CLOSE stat_cursor;
	    break;
	}

	/* determine if this is the 1st tuple of statistics for this particular
	** iirelation entry -- of course, there can be multiple statistics on
	** a single table, one for each attribute 
	*/
	if ( (stat_tbl.stabbase != statid.db_tab_base) ||
	     (stat_tbl.stabindex != statid.db_tab_index) ||
	     (stat_tbl.satno != atno) )
	{
	    /* this is the first statistics tuple for this table */
	    statid.db_tab_base  = stat_tbl.stabbase;
	    statid.db_tab_index = stat_tbl.stabindex;
	    atno = stat_tbl.satno;
	    base = duve_findreltid ( &statid, duve_cb);
	    repeat = 0;

	    /* 
	    **  copy pertentent info to the cache 
	    */
	    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_rptr = base;
	    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_satno = stat_tbl.satno;
	    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_sid.db_tab_base =
							    stat_tbl.stabbase;
	    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_sid.db_tab_index =
							    stat_tbl.stabindex;

	}

	/* if table receiving statistics is to be dropped, then dont bother
	** checking statistics on it 
	*/
	if (base == DUVE_DROP)
	    continue;

	if (! repeat)
	{
	    /* test 1 - verify table receiving statistics really exists */
	    if (base == DU_INVALID)
	    {   
		if (duve_banner( DUVE_IISTATISTICS, 1, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk ( DUVE_MODESENS, duve_cb, S_DU164C_NO_BASE_FOR_STATS, 
			4, sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex);
		if ( duve_talk(DUVE_ASK, duve_cb, 
			S_DU0326_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex)
		== DUVE_YES)
		{
		    /* indicate to delete when done with cursor */
		    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_sdrop=
			DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU0376_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex);
		    base = DUVE_DROP;
		}
		continue;
	    }

	    /* test 2 - verify table recieving statistics is not index */
	    if (stat_tbl.stabindex != 0)
	    {
		if (duve_banner( DUVE_IISTATISTICS, 2, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU1649_STATS_ON_INDEX, 8,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex);
		if ( duve_talk( DUVE_ASK, duve_cb,
			S_DU0326_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex)
		== DUVE_YES)
		{
		    /* indicate to delete when done with cursor */
		    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_sdrop=
			DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU0376_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex);
		    base = DUVE_DROP;
		    continue;
		}

	    }

	    tid = statid.db_tab_base;
	    tidx= statid.db_tab_index;

	    /* test 3 - verify base table knows it has optimizer statistics */
	    if ( ( duve_cb->duverel->rel[base].du_stat & TCB_ZOPTSTAT) == 0)
	    {
		if (duve_banner( DUVE_IISTATISTICS, 3, duve_cb)
		== DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk ( DUVE_MODESENS, duve_cb, S_DU1647_NO_OPSTAT_IN_BASE,
			    4, 0, duve_cb->duverel->rel[base].du_tab,
			    0, duve_cb->duverel->rel[base].du_own);
		if ( duve_talk( DUVE_ASK, duve_cb, S_DU0327_SET_OPTSTAT, 0)
		== DUVE_YES)
		{
		    basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		    basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		    relstat = duve_cb->duverel->rel[base].du_stat |TCB_ZOPTSTAT;
		    duve_cb->duverel->rel[base].du_stat = relstat;
		    EXEC SQL update iirelation set relstat = :relstat where
					    reltid = :tid and reltidx = :tidx;
		    if (sqlca.sqlcode == 0)
			duve_talk(DUVE_MODESENS, duve_cb,S_DU0377_SET_OPTSTAT,0);
		}
	    } /* end test 3 */

	} /* endif 1st statistic tuple for this table */


	/* test 4 -  verify statistic is for a valid column */
	if ( (stat_tbl.satno > duve_cb->duverel->rel[base].du_attno) ||
	     (stat_tbl.satno == 0) )
	{
		if (duve_banner( DUVE_IISTATISTICS, 4, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU1646_INVALID_STATISTIC_COL, 8,
			   0, duve_cb->duverel->rel[base].du_tab,
			   0, duve_cb->duverel->rel[base].du_own,
			   sizeof(stat_tbl.satno), &stat_tbl.satno,
			   sizeof(duve_cb->duverel->rel[base].du_attno),
			   &duve_cb->duverel->rel[base].du_attno  );
		if (duve_talk(DUVE_ASK, duve_cb,
			S_DU0326_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex)
		== DUVE_YES)
		{
		    /* indicate to delete when done with cursor */
		    duve_cb->duvestat->stats[duve_cb->duve_scnt].du_sdrop=
			DUVE_DROP;
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU0376_DROP_FROM_IISTATISTICS, 4,
			sizeof(stat_tbl.stabbase), &stat_tbl.stabbase,
			sizeof(stat_tbl.stabindex), &stat_tbl.stabindex);
		    base = DUVE_DROP;
		    continue;
		}
	}

	/* test 5 - verify there really is a histogram if one is expected */
	EXEC SQL repeated select any(hatno) into :cnt from iihistogram where
	    htabbase = :tid and htabindex = :tidx and hatno = :stat_tbl.satno;

	if ( (cnt == 0) && (stat_tbl.snumcells) )
	{
	    if (duve_banner( DUVE_IISTATISTICS, 5, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);	    
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1648_MISSING_HISTOGRAM,6,
		   0, duve_cb->duverel->rel[base].du_tab,
		   0, duve_cb->duverel->rel[base].du_own,
		   sizeof(stat_tbl.satno), &stat_tbl.satno);
	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU0328_FIX_SNUMCELLS, 10,
		   sizeof(stat_tbl.snumcells), &stat_tbl.snumcells,
		   sizeof(cnt), &cnt,
		   0, duve_cb->duverel->rel[base].du_tab,
		   0, duve_cb->duverel->rel[base].du_own,
		   sizeof(stat_tbl.satno), &stat_tbl.satno) == DUVE_YES)
	    {
	        /* clean snumcells */
		exec sql update iistatistics set snumcells = :cnt where
			stabbase = :stat_tbl.stabbase and 
			stabindex = :stat_tbl.stabindex;
	        duve_talk(DUVE_MODESENS, duve_cb, S_DU0378_FIX_SNUMCELLS,10,
		   sizeof(stat_tbl.snumcells), &stat_tbl.snumcells,
		   sizeof(cnt), &cnt,
		   0, duve_cb->duverel->rel[base].du_tab,
		   0, duve_cb->duverel->rel[base].du_own,
		   sizeof(stat_tbl.satno), &stat_tbl.satno) ;
	    }

	}

    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckstat"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: cksynonym - run verifydb tests to check system catalog iisynonym.
**
** Description:
**      This routine opens a cursor to walk iisynonym one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iisynonym system catalog.
**
**	TEST 1 : Verify that there are not any tables in iirelation with the
**		 same name and owner as the synonym
**	        TEST:   Select count(relid) from iirelation r, iisyonym s where
**			r.relid = s.synonym_name and
**			r.relowner = s.synonym_owner.
**			If count is non-zero, then we have detected error.
**	        FIX:    Delete the synonym tuple.
**
**	TEST 2 : Verify that there really is an iirelation object (table, view
**		 or index) that the synonym resolves to.
**	        TEST:   select count(relid) from iirelation r, iisynonym s where
**			r.reltid = s.syntabbase and r.reltidx = s.syntabidx.
**			If count is not exactly 1, then there is a problem.
**	        FIX:    Delete the synonym tuple.
**
**	TEST 3: Verify that there is a corresponding schema for the synonym
**              owner.
**              TEST:   Select repeat any(schema_id)
**			from iischema s, iisynonym sy
**                      where sy.synonym_owner = s.schema_name.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**	none
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iisynonym
**	    tuples will be read from iischema	
**
** History:
**      12-jun-90 (teresa)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**      16-jun-93 (anitap)
**          Added test for CREATE SCHEMA project.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (anitap)
**	    Cache schema information to create schemas for orphaned objects.
**	07-jan-94 (teresa)
**	    change logic to 1) exit if there are no tuples (since above chagne
**	    now counts tuples) and 2) don't open cursor until after memory
**	    is allocated.
[@history_template@]...
*/
DU_STATUS
cksynonym(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_TAB_ID		int_id;
    DU_STATUS		base;
    char		tabid[DB_MAXNAME+1];
    u_i4		size;
    STATUS		mem_stat;
    
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvesyn.sh>;
	i4 cnt;
	u_i4	tid, tidx;
        char	tabowner[DB_MAXNAME+1];
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory to hold add commands */	
    EXEC SQL select count(synonym_name) into :cnt from iisynonym;

    duve_cb->duve_sycnt = 0;

    if (cnt == 0)
	return DUVE_NO;
    else
        size = cnt * sizeof(DU_SYNONYM);

    duve_cb->duvesyns = (DUVE_SYNONYM *) MEreqmem(0, size, TRUE, 
			&mem_stat);

    if ( (mem_stat != OK) || (duve_cb->duvesyns == 0))
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
        return( (DU_STATUS) DUVE_BAD);
    }	

    EXEC SQL DECLARE syn_cursor CURSOR FOR
	select * from iisynonym;
    EXEC SQL open syn_cursor;
    
    /* loop for each tuple in iisynonym */
    for (;;)  
    {
	/* get tuple from iisynonym */
	EXEC SQL FETCH syn_cursor INTO :syn_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iisynonym */
	{
	    EXEC SQL CLOSE syn_cursor;
	    break;
	}

	/* test 1 -- verify synonym name is unique */

	/* first, take 32 char synonym name, synonym owner and force into string
	** DB_NAXNAME char long.
	**
	** FIXME -- if DB_MAXNAME is extended to exceed 32 char and the
	**	    iisynonym names are not likewise extended, this will cause
	**	    an AV here.  Of course, that is very unlikely to occur.
	*/
	MEcopy( (PTR)syn_tbl.synonym_name, DB_MAXNAME, (PTR)tabid);
	MEcopy( (PTR)syn_tbl.synonym_owner, DB_MAXNAME, (PTR)tabowner);
	tabid[DB_MAXNAME]='\0';
	tabowner[DB_MAXNAME]='\0';
	STtrmwhite(tabid);
	STtrmwhite(tabowner);
	/* now search cache of iirelation tuple info for name,owner match */
	base = duve_relidfind (tabid, tabowner, duve_cb);
	if (base != DU_INVALID)
	{
	    /* this name,owner pair is in iirelation, so drop from iisynonym */

            if (duve_banner( DUVE_IISYNONYM, 1, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU166F_NONUNIQUE_SYNONYM, 4,
			0, syn_tbl.synonym_name, 0, syn_tbl.synonym_owner);
	    
	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU033C_DROP_IISYNONYM, 0)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iisynonym where current of syn_cursor;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU038C_DROP_IISYNONYM, 0);
		continue;
	    }
	} /* endif base table not found */

	/* test 2 -- verify table for synonym exists */
	int_id.db_tab_base = syn_tbl.syntabbase;
	int_id.db_tab_index = syn_tbl.syntabidx;
	
	base = duve_findreltid ( &int_id, duve_cb);
	if ( (base == DU_INVALID) || ( base == DUVE_DROP) )
	{
	    /* this tuple is not in iirelation, so drop from iisynonym */

            if (duve_banner( DUVE_IISYNONYM, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1670_INVALID_SYNONYM, 8,
			0, syn_tbl.synonym_name, 0, syn_tbl.synonym_owner,
			sizeof(syn_tbl.syntabbase), &syn_tbl.syntabbase,
			sizeof(syn_tbl.syntabidx), &syn_tbl.syntabidx);
	    if ( duve_talk(DUVE_ASK, duve_cb, S_DU033C_DROP_IISYNONYM, 0)
	    == DUVE_YES)
	    {
		EXEC SQL delete from iisynonym where current of syn_cursor;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU038C_DROP_IISYNONYM, 0);
	    }
	    continue;
	} /* endif base table not found */

	/* test 3 -- verify there is a corresponding schema in iischema */

        /* now search for iischema tuple for owner match */
        EXEC SQL repeated select any(schema_id) into :cnt
		 from iischema 
		 where schema_name = :tabowner; 

        if (cnt == 0)
        {
           /* corresponding entry missing in iischema */
           
	   if (duve_banner(DUVE_IISYNONYM, 3, duve_cb)
                  == DUVE_BAD)
                return ( (DU_STATUS) DUVE_BAD);
           duve_talk (DUVE_MODESENS, duve_cb, S_DU168D_SCHEMA_MISSING, 2,
                        0, syn_tbl.synonym_owner);

	   if ( duve_talk(DUVE_IO, duve_cb, S_DU0245_CREATE_SCHEMA, 0)
			== DUVE_YES)
	   {	

              /* cache info about this tuple */
              MEcopy( (PTR)syn_tbl.synonym_owner, DB_MAXNAME,
           	(PTR) duve_cb->duvesyns->syns[duve_cb->duve_sycnt].du_schname);

	      duve_cb->duve_sycnt++;

              duve_talk(DUVE_MODESENS, duve_cb, S_DU0445_CREATE_SCHEMA, 0);
	   }

	   continue;
        } /* end test 3 */

    }  /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "cksynonym"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: cktree	- run verifydb tests to check system catalog iitree
**
** Description:
**      This routine opens a cursor to walk iitree one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iitree system catalog.
**
**	It allocates memory to keep track of any iitree tuples that should be
**	deleted from the iitree table (identified by table id).  After it has
**	finished walking the table via cursor, it deletes any iitree tuples
**	marked for deletion.  If the deleted tree entry defines a view, it marks
**	that for deletion also.  cktree is a good housekeeper, it deallocates
**	the memory it was using once it is done with it.
**
**	Function cktree is able to efficiently handle tree entries that span
**	across more than 1 iitree tuple.  It does this by:
**	    1. keeping track of whether or not it has seen this tree's table
**		id before.  If so, it skips cross referencing it against the
**		cached iirelation table.
**	    2. Skipping all futher tests on the tree entry once it has been
**		marked for deletion.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples read from iitree
**	    tuples in iirelation may be modified
**	    view/prot/etc that tree defines may be dropped from database
**
** History:
**      29-aug-88   (teg)
**          initial creation
**	21-feb-89   (teg)
**	    fix bug where verifydb unable to handle two different trees on the
**	    same table (where more than one permit defined for same table, or
**	    more than one integrity), etc.
**	    Also, stop caching information about stored procedures, as the
**	    trees for stored procedures are no longer put into iitree.
**	04-apr-89 (neil)
**	    Added checks for rule trees.  At this time this is only
**	    added to suppress failures and not to actually test the
**	    relationships between iitree, and iirule.
**	03-jan-89 (neil)
**	    Added checks for event trees.  At this time there are really no
**	    event trees (these are expected with event enhancements).  The
**	    checks were added to suppress failures and not to test.
**	26-oct-90 (teresa)
**	    Skip all other iitree checks if test 35 fails but the fix option
**	    is NOT selected.
**	30-oct-90 (teresa)
**	    fix bug where S_DU036C_DEL_TREE was reported with DUVE_ASK message
**	    mode instead of DUVE_MODESENS.
**	11-feb-91 (teresa)
**	    Implement ingres6302p change 340465
**	15-feb-91 (teresa)
**	    fix bug 35940 (assumed there was only 1 tree entry per table id,
**	    which is invalid assumption.  ex: define multiple QUEL permits on
**	    the same table.)
**	01-oct-91 (teresa)
**	    fixed compiler warning where duve_cb->duvetree was being cast to
**	    the wrong type.
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	26-may-93 (teresa)
**	    Changed to handle events, procedures, rules not being in iirelation,
**	    which also implements new tests 6, 7, 8.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	07-jan-94 (teresa)
**	    do NOT open cursor until after memory is allocated.
[@history_template@]...
*/
DU_STATUS
cktree (duve_cb)
DUVE_CB            *duve_cb;
{
    i4		dropcnt = 0;
    DU_STATUS		base;
    i4		seq;
    i4		diag_msgno,fix_msgno,done_msgno,bit_val;
    DUVE_TREE		tree;	  /* iitree.treetabbase,iitree.treetabidx */
    u_i4		size;
    STATUS		mem_stat;
    char		*name;
    char		*own;
    bool		badtree;
    i4			testid;
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvetree.sh>;
	i4 cnt;
	u_i4	tid, tidx;
	i4	id1, id2;
	u_i4	basetid, basetidx;
	i4	relstat;
	char	ownname[DB_MAXNAME + 1];
	char	objname[DB_MAXNAME + 1];
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* allocate memory to hold drop commands */
    EXEC SQL select count(treeseq) into :cnt from iitree where treeseq = 0;
    if (sqlca.sqlcode == 100 || cnt ==0)
    {	
	/* its unreasonable to have an empty iitree table, as the system
	** catalogs include some views 
	*/
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU5021_EMPTY_IITREE, 0);
	return DUVE_BAD;
    }
    size = cnt * sizeof(tree);
    duve_cb->duvetree = (DUVE_TREE *) MEreqmem(0, size, TRUE, &mem_stat);
    if ( (mem_stat != OK) || (duve_cb->duvetree == 0) )
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	return ( (DU_STATUS) DUVE_BAD);
    }
    
    /* loop for each tuple in iitree */

    EXEC SQL DECLARE tree_cursor CURSOR FOR
	select * from iitree order by
		treetabbase, treetabidx, treeid1, treeid2, treeseq;
    EXEC SQL open tree_cursor;
    
    tree.tabid.db_tab_base = -1;
    tree.tabid.db_tab_index = -1;
    tree.treeid.db_tre_high_time = -1;
    tree.treeid.db_tre_low_time = -1;
    for (;;)  
    {
	/* get tuple from iitree */
	EXEC SQL FETCH tree_cursor INTO :tree_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iitree */
	{
	    EXEC SQL CLOSE tree_cursor;
	    break;
	}

	/* determine if this is the 1st tuple for this tree entry */
	if ( (tree_tbl.treetabbase != tree.tabid.db_tab_base) ||
	     (tree_tbl.treetabidx != tree.tabid.db_tab_index) ||
	     (tree_tbl.treeid1 != tree.treeid.db_tre_high_time) ||
	     (tree_tbl.treeid2 != tree.treeid.db_tre_low_time) )
	{
	    /* this is the first tuple for this tree */
	    tree.tabid.db_tab_base  = tree_tbl.treetabbase;
	    tree.tabid.db_tab_index = tree_tbl.treetabidx;
	    tree.treeid.db_tre_high_time = tree_tbl.treeid1;
	    tree.treeid.db_tre_low_time = tree_tbl.treeid2;
	    seq = 0;
	    badtree = FALSE;

	    /* test 1 - verify tree has corresponding iirelation entry
	    **	        (or iirule for rules or iievent for events,
	    **		which are tests 6 and 7)
	    */
	    if (tree.treemode == DB_EVENT)
	    {
		testid = 7;
		exec sql repeated select event_name, event_owner
		                    into :objname, :ownname
		    from iievent
		    where event_ibase = :tree_tbl.treetabbase and
			  event_idx   = :tree_tbl.treetabidx;
		if (sqlca.sqlcode == 100)
		    badtree = TRUE;
		else
		{
		    name = objname;
		    own = ownname;
		}
	    }
	    else if (tree.treemode == DB_RULE)
	    {
		testid = 6;
		exec sql repeated select rule_name, rule_owner
				    into :objname, :ownname
		    from iirule
		    where rule_treeid1 = :tree_tbl.treeid1 and 
			  rule_treeid2 = :tree_tbl.treeid2;
		if (sqlca.sqlcode == 100)
		    badtree = TRUE;
		else
		{
		    name = objname;
		    own = ownname;
		}
	    }
	    else 
	    {
		testid = 1;
		base = duve_findreltid ( &tree.tabid, duve_cb);
		if (base == DU_INVALID)
		    badtree = TRUE;
		else
		{
		    name = duve_cb->duverel->rel[base].du_tab;
		    own = duve_cb->duverel->rel[base].du_own;
		}
	    }

	    if (badtree)
	    {   
		if (duve_banner( DUVE_IITREE, testid, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk ( DUVE_MODESENS, duve_cb, S_DU162F_NO_TREEBASE, 4,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx);
		if ( duve_talk(DUVE_ASK, duve_cb, S_DU031C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2)
		== DUVE_YES)
		{
		    /* indicate to drop tree when done with cursor */
		    duve_cb->duvetree[dropcnt].tabid.db_tab_base =
					tree.tabid.db_tab_base;
		    duve_cb->duvetree[dropcnt].tabid.db_tab_index =
					tree.tabid.db_tab_index;
		    duve_cb->duvetree[dropcnt].treemode = tree.treemode;
		    duve_cb->duvetree[dropcnt].treeid.db_tre_high_time =
					tree.treeid.db_tre_high_time;
		    duve_cb->duvetree[dropcnt++].treeid.db_tre_low_time =
					tree.treeid.db_tre_low_time;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU036C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2);
		    continue;
		}
		continue; /* no sense doing other tests on this tree tuple */
	    } /* endif this tree missing corresponding entry from iirelation */

	}	    

	/* determine if this tree entry is marked for drop */
	if ( ( base == DUVE_DROP) ||
	     ((duve_cb->duvetree[dropcnt-1].tabid.db_tab_base ==
	       tree.tabid.db_tab_base) &&
	      (duve_cb->duvetree[dropcnt-1].tabid.db_tab_index ==
	       tree.tabid.db_tab_index) &&
	      (duve_cb->duvetree[dropcnt-1].treeid.db_tre_high_time ==
	       tree.treeid.db_tre_high_time) &&
	      (duve_cb->duvetree[dropcnt-1].treeid.db_tre_low_time ==
	       tree.treeid.db_tre_low_time)
	     )
	   )
	{
	    /* this tree entry is marked for deletion, so skip testing it */
	    continue;
	}

	
	/* test 2 -- verify tree mode is valid */
	if ( (tree_tbl.treemode != DB_VIEW) && 
	     (tree_tbl.treemode != DB_PROT) &&
	     (tree_tbl.treemode != DB_CRT_LINK) &&
	     (tree_tbl.treemode != DB_REG_LINK) && 
	     (tree_tbl.treemode != DB_REG_IMPORT) &&
	     (tree_tbl.treemode != DB_RULE) &&
	     (tree_tbl.treemode != DB_EVENT) &&
	     (tree_tbl.treemode != DB_INTG) )
	{
            if (duve_banner( DUVE_IITREE, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU162D_UNKNOWN_TREEMODE, 
			10,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			0, name, 0, own,
			sizeof(tree_tbl.treemode), &tree_tbl.treemode);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU031C_DEL_TREE, 8,
			    sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			    sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			    sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			    sizeof(tree_tbl.treeid2), &tree_tbl.treeid2)
	    == DUVE_YES)
	    {
		duve_cb->duvetree[dropcnt].tabid.db_tab_base =
				    tree.tabid.db_tab_base;
		duve_cb->duvetree[dropcnt].tabid.db_tab_index =
				    tree.tabid.db_tab_index;
		duve_cb->duvetree[dropcnt].treemode = tree.treemode;
		duve_cb->duvetree[dropcnt].treeid.db_tre_high_time =
				    tree.treeid.db_tre_high_time;
		duve_cb->duvetree[dropcnt++].treeid.db_tre_low_time =
				    tree.treeid.db_tre_low_time;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU036C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2);
		continue;		
	    }
	} /* endif bad tree mode */

	/* test 3 - verify tree id is valid */
	if ( (tree_tbl.treeid1 == 0) && (tree_tbl.treeid2 == 0) )
	{
	    /* invalid tree id */
            if (duve_banner( DUVE_IITREE, 3, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU162E_INVALID_TREEID, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			0, name, 0, own);
	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU031C_DEL_TREE, 8,
			    sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			    sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			    sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			    sizeof(tree_tbl.treeid2), &tree_tbl.treeid2)
	    == DUVE_YES)
	    {
		duve_cb->duvetree[dropcnt].tabid.db_tab_base =
				    tree.tabid.db_tab_base;
		duve_cb->duvetree[dropcnt].tabid.db_tab_index =
				    tree.tabid.db_tab_index;
		duve_cb->duvetree[dropcnt].treemode = tree.treemode;
		duve_cb->duvetree[dropcnt].treeid.db_tre_high_time =
				    tree.treeid.db_tre_high_time;
		duve_cb->duvetree[dropcnt++].treeid.db_tre_low_time =
				    tree.treeid.db_tre_low_time;
		duve_talk(DUVE_MODESENS, duve_cb, S_DU036C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2);
		continue;		
	    }
	} /* endif bad tree id */	    

	/*
	** test 4  - verify that iirelation tuple correctly reflects
	**	     tree mode (relstat)
	*/
	diag_msgno = 0;
	switch (tree_tbl.treemode)
	{
	case DB_VIEW:
	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_VIEW) == 0 )
	    {
		diag_msgno = S_DU1630_NO_VIEW_RELSTAT;
		fix_msgno  = S_DU031D_SET_VIEW;
		done_msgno = S_DU036D_SET_VIEW;
		bit_val = TCB_VIEW;
	    }
	    break;
	case DB_PROT:
	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_PRTUPS) == 0 )
	    {
		diag_msgno = S_DU1631_NO_PROTECT_RELSTAT;
		fix_msgno  = S_DU031E_SET_PRTUPS;
		done_msgno = S_DU036E_SET_PRTUPS;
		bit_val = TCB_PRTUPS;
	    }
	    break;
	case DB_INTG:
	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_INTEGS) == 0 )
	    {
		diag_msgno = S_DU1632_NO_INTEGS_RELSTAT;
		fix_msgno  = S_DU031F_SET_INTEGS;
		done_msgno = S_DU036F_SET_INTEGS;
		bit_val = TCB_INTEGS;
	    }
	    break;
	case DB_DBP:
	    break;
	case DB_RULE:
	    if ( (duve_cb->duverel->rel[base].du_stat & TCB_RULE) == 0 )
	    {
		diag_msgno = S_DU1673_NO_RULES_RELSTAT;
		fix_msgno  = S_DU033E_SET_RULES;
		done_msgno = S_DU038E_SET_RULES;
		bit_val = TCB_RULE;
	    }
	    break;
	case DB_EVENT:
	    break;
	default:
	    break;
	}
	if (diag_msgno != 0)
	{
	    /* the relstat in iirelation needs to be modified to indicate mode*/
            if (duve_banner( DUVE_IITREE, 4, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, diag_msgno, 4,
			0, name, 0, own);
	    if ( duve_talk( DUVE_ASK, duve_cb, fix_msgno, 0)
	    == DUVE_YES)
	    {
		basetid = duve_cb->duverel->rel[base].du_id.db_tab_base;
		basetidx= duve_cb->duverel->rel[base].du_id.db_tab_index;
		relstat = duve_cb->duverel->rel[base].du_stat | bit_val;
		duve_cb->duverel->rel[base].du_stat = relstat;
		EXEC SQL update iirelation set relstat = :relstat where
				    reltid = :basetid and reltidx = :basetidx;
		if (sqlca.sqlcode == 0)
		    duve_talk(DUVE_MODESENS, duve_cb, done_msgno, 0);
	    }  /* endif fix relstat */

	} /* endif relstat doesnt reflect mode */

	/* test 5  -- verify sequence is correct for iitree entries that
	**	      span more than 1 tuple */

	if ( tree_tbl.treeseq == seq )
	    seq++;			/* all is well */
	else if (tree_tbl.treeseq < seq)
	{
	    /* duplicate tree sequence tuple */
            if (duve_banner( DUVE_IITREE, 5, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU164E_DUPLICATE_TREE_SEQ, 10,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			0, name, 0, own,
			sizeof(tree_tbl.treeseq), &tree_tbl.treeseq);
	    if ( duve_talk (DUVE_ASK, duve_cb, S_DU031C_DEL_TREE, 8,
			    sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			    sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			    sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			    sizeof(tree_tbl.treeid2), &tree_tbl.treeid2)
	    == DUVE_YES)
	    {
		duve_cb->duvetree[dropcnt].tabid.db_tab_base =
				    tree.tabid.db_tab_base;
		duve_cb->duvetree[dropcnt].tabid.db_tab_index =
				    tree.tabid.db_tab_index;
		duve_cb->duvetree[dropcnt].treemode = tree.treemode;
		duve_cb->duvetree[dropcnt].treeid.db_tre_high_time =
				    tree.treeid.db_tre_high_time;
		duve_cb->duvetree[dropcnt++].treeid.db_tre_low_time =
				    tree.treeid.db_tre_low_time;
		treeclean(base, &tree, tree_tbl.treemode, duve_cb);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU036C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2);
		continue;
	    }
	    else
		seq = tree_tbl.treeseq + 1;
	}
	else
	{
	    /* missing tree sequence tuple */
            if (duve_banner( DUVE_IITREE, 5, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU1633_MISSING_TREE_SEQ, 10,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			0, name, 0, own,
			sizeof(tree_tbl.treeseq), &tree_tbl.treeseq);
	    if ( duve_talk (DUVE_ASK, duve_cb, S_DU031C_DEL_TREE, 8,
			    sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			    sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			    sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			    sizeof(tree_tbl.treeid2), &tree_tbl.treeid2)
	    == DUVE_YES)
	    {
		duve_cb->duvetree[dropcnt].tabid.db_tab_base =
				    tree.tabid.db_tab_base;
		duve_cb->duvetree[dropcnt].tabid.db_tab_index =
				    tree.tabid.db_tab_index;
		duve_cb->duvetree[dropcnt].treemode = tree.treemode;
		duve_cb->duvetree[dropcnt].treeid.db_tre_high_time =
				    tree.treeid.db_tre_high_time;
		duve_cb->duvetree[dropcnt++].treeid.db_tre_low_time =
				    tree.treeid.db_tre_low_time;
		treeclean(base, &tree, tree_tbl.treemode, duve_cb);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU036C_DEL_TREE, 8,
			sizeof(tree_tbl.treetabbase), &tree_tbl.treetabbase,
			sizeof(tree_tbl.treetabidx), &tree_tbl.treetabidx,
			sizeof(tree_tbl.treeid1), &tree_tbl.treeid1,
			sizeof(tree_tbl.treeid2), &tree_tbl.treeid2);
		continue;
	    }
	    else
		seq = tree_tbl.treeseq + 1;
	}

    }	/* end loop for each iitree tuple */

    /* delete any iitree tuples marked for delete
    ** then deallocate the memory used by duve_cb->duvetree
    */
    while (dropcnt > 0)
    {
	/* drop from iitree */
	tid  = duve_cb->duvetree[--dropcnt].tabid.db_tab_base;
	tidx = duve_cb->duvetree[dropcnt].tabid.db_tab_index;
	id1 = duve_cb->duvetree[dropcnt].treeid.db_tre_high_time;
	id2 = duve_cb->duvetree[dropcnt].treeid.db_tre_low_time;
	EXEC SQL delete from iitree where 
	    treetabbase = :tid and treetabidx = :tidx and treeid1 = :id1 and
	    treeid2 = :id2;
	if (duve_cb->duvetree[dropcnt].treemode == DB_EVENT)
	{
	    /* Drop from iievent */
	    exec sql delete from iievent
	      where event_ibase = :tid and event_idx = :tidx;
    	}
	if (duve_cb->duvetree[dropcnt].treemode == DB_RULE)
	{
	    /* Drop from iirule */
	    exec sql delete from iirule
	      where rule_treeid1 = :id1 and rule_treeid2 = :id2;
	}
	else
	{
	    /* if view, drop from iirelation */
	    base = duve_findreltid ( &tree.tabid, duve_cb);
	    if ( base >= 0 )
		if (duve_cb->duverel->rel[base].du_stat & TCB_VIEW)
		    duve_cb->duverel->rel[base].du_tbldrop = DUVE_DROP;
	}
    }
    /*
    ** deallocate duve_cb->duvetree cache and indicate in control block that
    ** it has been dropped.
    */
    MEfree ( (PTR) duve_cb->duvetree);
    duve_cb->duvetree = 0;

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "cktree"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckxdbdep - run verifydb check on iixdbdepends dbms catalog
**
** Description:
**      This routine opens a cursor and examines each tuple in iixdbdepends.
**	It checks the tuple against the cached iidbdepends table.  If it
**	cannot find a match with tuples in this table, it reports an error
**	and possible (mode dependent) deletes that tuple from iixdbdepends.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duvedpnds			    cached iidbdepends information
**
** Outputs:
**	none.
**
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
** Side Effects:
**	    tuples may be deleted from iixdbdepends
**
** History:
**      6-Sep-1988 (teg)
**          initial creation
**	06-May-93 (teresa)
**	    Changed interface to duve_banner() and renumbered verifydb checks.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
[@history_template@]...
*/
DU_STATUS
ckxdbdep(duve_cb)
DUVE_CB            *duve_cb;
{
    DU_STATUS		base;


    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvexdp.sh>;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE xdpnd_cursor CURSOR FOR
	select * from iixdbdepends;
    EXEC SQL open xdpnd_cursor FOR READONLY;

    /* loop for each tuple in iixdbdepends */
    for (;;)
    {
	/* get tuple from table */
	EXEC SQL FETCH xdpnd_cursor INTO :xdpnd_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iixdbdepends */
	{
	    EXEC SQL CLOSE xdpnd_cursor;
	    break;
	}

	/* test 1 - see if there is corresponding tuple in iidbdepends */
	base = duve_d_dpndfind( xdpnd_tbl.deid1, xdpnd_tbl.deid2, 
	    xdpnd_tbl.dtype, xdpnd_tbl.qid, duve_cb);
	if (base == DU_INVALID)
	{
	    if (duve_banner( DUVE_IIXDBDEPENDS, 1, duve_cb)
	    == DUVE_BAD) 
		return ( (DU_STATUS)DUVE_BAD);
	    duve_talk( DUVE_MODESENS, duve_cb, S_DU1645_MISSING_IIDBDEPENDS, 4,
		       sizeof(xdpnd_tbl.deid1), &xdpnd_tbl.deid1,
		       sizeof(xdpnd_tbl.deid2),	&xdpnd_tbl.deid2);
	    if (duve_talk( DUVE_ASK, duve_cb, S_DU0325_REDO_IIXDBDEPENDS, 0)
	    ==DUVE_YES)
	    {
	        EXEC SQL CLOSE xdpnd_cursor;
		EXEC SQL drop iixdbdepends;
		EXEC SQL create index iixdbdepends on iidbdepends (deid1, deid2,
			dtype,qid) with structure = btree;
			
		duve_talk(DUVE_MODESENS,duve_cb, S_DU0375_REDO_IIXDBDEPENDS, 0);
	    }
	} /* end test 1 */

    } /* end loop for each tuple in table */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckxdbdep"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckxpriv - run verifydb check on iixpriv dbms catalog
**
** Description:
**	This function will ensure that IIXPRIV is in sync with IIPRIV.  This 
**	will be accomplished in two steps:
**	- first we will open a cursor to read tuples in iixpriv for 
**	  which there are no corresponding tuples in iipriv; if one or more such
**	  tuples are found, we will ask for permission to drop and recreate the 
**	  index
**	- then we will open a cursor on iipriv to read tuples for which there 
**	  is no corresponding iixpriv tuple and insert appropriate iixpriv 
**	  tuples
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**	none.
**
** Returns:
**    	DUVE_YES
**      DUVE_BAD
**
** Exceptions:
**	none
**
** Side Effects:
**	tuples may be inserted into / deleted from iixpriv
**
** History:
**      19-jan-94 (andre)
**	    written
[@history_template@]...
*/
DU_STATUS
ckxpriv(
	DUVE_CB		*duve_cb)
{
    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvexprv.sh>;
	u_i4		tid_ptr;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE xpriv_tst_curs_1 CURSOR FOR
	select * from iixpriv xpr where not exists 
	    (select 1 from iipriv pr 
	    	where
		        xpr.tidp 		= pr.tid
		    and xpr.i_obj_id 		= pr.i_obj_id
		    and xpr.i_obj_idx 		= pr.i_obj_idx
		    and xpr.i_priv 		= pr.i_priv
		    and xpr.i_priv_grantee 	= pr.i_priv_grantee);
    
    EXEC SQL open xpriv_tst_curs_1;

    /*
    ** test 1: for every IIXPRIV tuple for which there is no corresponding 
    ** IIPRIV tuple ask user whether we may delete the offending IIXPRIV tuple
    */
    for (;;)
    {
        EXEC SQL FETCH xpriv_tst_curs_1 INTO :xprv_tbl;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xpriv_tst_curs_1;
	    break;
        }

	/* there is no tuple in IIPRIV corresponding to this IIXPRIV tuple */
        if (duve_banner(DUVE_IIXPRIV, 1, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16C7_EXTRA_IIXPRIV, 10,
            sizeof(xprv_tbl.i_obj_id), &xprv_tbl.i_obj_id,
	    sizeof(xprv_tbl.i_obj_idx), &xprv_tbl.i_obj_idx,
	    sizeof(xprv_tbl.i_priv), &xprv_tbl.i_priv,
	    0, xprv_tbl.i_priv_grantee,
	    sizeof(xprv_tbl.tidp), &xprv_tbl.tidp);
	if (duve_talk(DUVE_ASK, duve_cb, S_DU0248_DELETE_IIXPRIV, 0) 
	    == DUVE_YES)
	{
	    exec sql delete from iixpriv where current of xpriv_tst_curs_1;

	    duve_talk(DUVE_MODESENS,duve_cb, S_DU0448_DELETED_IIXPRIV, 0);
	    
	    break;
	}
    }

    /* end test 1 */

    EXEC SQL DECLARE xpriv_tst_curs_2 CURSOR FOR
	select pr.*, pr.tid from iipriv pr where not exists
	    (select 1 from iixpriv xpr
		where
                        xpr.tidp                = pr.tid
                    and xpr.i_obj_id            = pr.i_obj_id
                    and xpr.i_obj_idx           = pr.i_obj_idx
                    and xpr.i_priv              = pr.i_priv
                    and xpr.i_priv_grantee      = pr.i_priv_grantee
		    /* 
		    ** this last comparison ensures that we get the qualifying 
		    ** rows - otherwise (I suspect) OPF chooses to join index 
		    ** to itself and finds nothing
		    */
		    and prv_flags		= prv_flags);

    EXEC SQL open xpriv_tst_curs_2 FOR READONLY;

    /*
    ** test 2: for every IIPRIV tuple for which there is no corresponding 
    ** IIXPRIV tuple ask user whether we may insert a missing tuple into IIXPRIV
    */
    for (;;)
    {
        EXEC SQL FETCH xpriv_tst_curs_2 INTO :prv_tbl, :tid_ptr;
        if (sqlca.sqlcode == 100) /* then no more tuples to read */
        {
            EXEC SQL CLOSE xpriv_tst_curs_2;
	    break;
        }

	/* there is no tuple in IIXPRIV corresponding to this IIPRIV tuple */
        if (duve_banner(DUVE_IIXPRIV, 2, duve_cb) == DUVE_BAD) 
	    return ( (DU_STATUS)DUVE_BAD);

	STtrmwhite(prv_tbl.i_priv_grantee);

        duve_talk( DUVE_MODESENS, duve_cb, S_DU16C8_MISSING_IIXPRIV, 10,
            sizeof(prv_tbl.i_obj_id), &prv_tbl.i_obj_id,
	    sizeof(prv_tbl.i_obj_idx), &prv_tbl.i_obj_idx,
	    sizeof(prv_tbl.i_priv), &prv_tbl.i_priv,
	    0, prv_tbl.i_priv_grantee,
	    sizeof(tid_ptr), &tid_ptr);
	if (duve_talk( DUVE_ASK, duve_cb, S_DU0249_INSERT_IIXPRIV_TUPLE, 0) 
	    == DUVE_YES)
	{
	    exec sql insert into 
		iixpriv(i_obj_id, i_obj_idx, i_priv, i_priv_grantee, tidp)
		values (:prv_tbl.i_obj_id, :prv_tbl.i_obj_idx, :prv_tbl.i_priv,
		    :prv_tbl.i_priv_grantee, :tid_ptr);
			
	    duve_talk(DUVE_MODESENS, duve_cb, 
		S_DU0449_INSERTED_IIXPRIV_TUPLE, 0);
	}
    }

    /* end test 2 */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckxpriv"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckpriv - Perform system catalog checks/fixes on iipriv catalog
**
** Description:
**      this routine performs system catalog tests on iipriv 
**	system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    iipriv tuples may get deletred, database objects may be marked for 
**	    destruction
**
** History:
**      19-jan-94 (andre)
**          written
**	9-Sep-2004 (schka24)
**	    1024 column expansion.
[@history_template@]...
*/
DU_STATUS
ckpriv(
       DUVE_CB		*duve_cb)
{
    DB_TAB_ID		obj_id;
    u_i4		priv_number, obj_type;
    DU_STATUS		view_offset;
    u_i4		attr_map[DB_COL_WORDS];
    DB_TAB_ID		indep_obj_id;
    i4			priv_needed;
    STATUS              mem_stat;
    bool		satisfied;
    bool		sel_perms_on_view;
    bool		perms_on_dbp;
        
    EXEC SQL BEGIN DECLARE SECTION;
	u_i4	tid, tidx;
	i4	qid;
	char	obj_name[DB_MAXNAME + 1];
	char	sch_name[DB_MAXNAME + 1];
	i4 cnt;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE priv_cursor CURSOR FOR
	select * from iipriv;
    EXEC SQL open priv_cursor;
    
    /*
    ** IIPRIV contains privilege descriptors for objects of 4 types:
    **	- views,
    **	- database procedures,
    **	- permits on views, and
    **  - constraints
    **
    ** descriptors for views and dbprocs will be assigned descriptor number 0
    ** descriptors for views and constraints will descriptor number > 0
    */

    /*
    ** for views and dbprocs, there may be multiple records in IIPRIV describing
    ** privileges on which these objects depend. In order to avoid redundant 
    ** checks for existence of an object, we will save object id and type and 
    ** the descriptor number found in the last row.
    */
    obj_id.db_tab_base = obj_id.db_tab_index = 0L;

    /* loop for each tuple in iipriv */
    for (;;)  
    {
	/* get tuple from table */
	EXEC SQL FETCH priv_cursor INTO :prv_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in table */
	{
	    EXEC SQL CLOSE priv_cursor;
	    break;
	}
	
	STtrmwhite(prv_tbl.i_priv_grantee);

	/*
	** test 1: verify that the object for which this descriptor has been 
	** 	   created exists; 
	**
	** if the descriptor contains privileges on which a view or a dbproc 
	** depends, we will verify that the object exists; if it contains 
	** privileges on which a permit or a constraint depends, we will verify
	** that there is a tuple in IIDBDEPENDS describing dependence of an 
	** object of specified type on this descriptor; in all cases, we will 
	** compare object id and type and descriptor number with those saved 
	** in obj_id, obj_type and priv_number - if they are the same, we have 
	** already checked for existence of the object and there is hardly a 
	** reason to do it again.
	*/
	if (   obj_id.db_tab_base 	!= prv_tbl.d_obj_id
	    || obj_id.db_tab_index 	!= prv_tbl.d_obj_idx
	    || priv_number		!= prv_tbl.d_priv_number
	    || obj_type 		!= prv_tbl.d_obj_type)
	{
	    bool	bad_obj_type;
	    bool	found;

	    /*
	    ** save object id and type and descriptor number found in this 
	    ** record
	    */
	    obj_id.db_tab_base = prv_tbl.d_obj_id;
	    obj_id.db_tab_index = prv_tbl.d_obj_idx;
	    priv_number = prv_tbl.d_priv_number;
	    obj_type = prv_tbl.d_obj_type;

	    bad_obj_type = found = FALSE;

	    switch (obj_type)
	    {
		case DB_VIEW:
		    /*
		    ** look up the view description in the relation cache;
		    ** will complain only if the object was not found; there is 		    ** no reason to complain even if it is marked for 
		    ** destruction because this tuple will then get destroyed 
		    ** as a part of destruction of the view itself
		    */
		    if ((view_offset = duve_findreltid(&obj_id, duve_cb)) 
			== DUVE_DROP)
		    {
			/* 
			** view is marked for destruction - no point in running 
			** checks on this tuple
			*/
			continue;
		    }

		    found = (view_offset != DU_INVALID);

		    if (found)
		    {
			EXEC SQL BEGIN DECLARE SECTION;
			    u_i4	sel;
			    u_i4	sel_wgo;
			EXEC SQL END DECLARE SECTION;

			sel = DB_RETRIEVE | DB_TEST | DB_AGGREGATE;
			sel_wgo = sel | DB_GRANT_OPTION;

			/*
			** if the view exists, determine whether there are 
			** select permits on it - if so, when checking whether 
			** the user possesses privileges described by this 
			** record, we will check for SELECT WITH GRANT OPTION 
			** as opposed to just SELECT
			*/
			exec sql repeated select any(1) into :cnt from iiprotect
			    where 
				    protabbase = :prv_tbl.d_obj_id
				and protabidx  = :prv_tbl.d_obj_idx
				and (proopset = :sel or proopset = :sel_wgo)
				and prograntor = :prv_tbl.i_priv_grantee;
			
			sel_perms_on_view = (cnt != 0);
		    }

		    break;

		case DB_DBP:
                    EXEC SQL REPEATED select any(dbp_id) into :cnt 
		        from iiprocedure
                        where 
			        dbp_id 	= :prv_tbl.d_obj_id 
			    and dbp_idx = :prv_tbl.d_obj_idx;
    
		    found = (cnt != 0);

		    if (found)
		    {
			EXEC SQL BEGIN DECLARE SECTION;
			    u_i4	exec_priv;
			    u_i4	exec_priv_wgo;
			EXEC SQL END DECLARE SECTION;

			exec_priv = DB_EXECUTE;
			exec_priv_wgo = exec_priv | DB_GRANT_OPTION;

			/*
			** if the dbproc exists, determine whether there are 
			** permits on it - if so, when checking whether 
			** the user possesses privileges described by this 
			** record, we will check for <priv> WITH GRANT OPTION 
			** as opposed to just <priv>
			*/
			exec sql repeated select any(1) into :cnt from iiprotect
			    where 
				    protabbase = :prv_tbl.d_obj_id
				and protabidx  = :prv_tbl.d_obj_idx
				and (   proopset = :exec_priv 
				     or proopset = :exec_priv_wgo)
				and prograntor = :prv_tbl.i_priv_grantee;
			
			perms_on_dbp = (cnt != 0);
		    }

		    break;

		case DB_PROT:
		case DB_CONS:
		    /*
		    ** make sure IIDBDEPENDS cache contains an entry describing 
		    ** dependence of an object of specified type on this 
		    ** privilege descriptor.
		    */
		    found = 
			(duve_d_cnt((i4) DU_INVALID, (i4) obj_id.db_tab_base,
			    (i4) obj_id.db_tab_index, (i4) DB_PRIV_DESCR,
			    (i4) priv_number, (i4) obj_type, duve_cb) > 0);

		    break;
		
		default:
		    bad_obj_type = TRUE;
		    break;
	    }

            if (bad_obj_type)
            {
	        /* 
	        ** type of object for which the privilege descriptor has been 
		** created is not one of those which we expected to find
		*/
                if (duve_banner(DUVE_IIPRIV, 1, duve_cb) == DUVE_BAD) 
	            return ( (DU_STATUS) DUVE_BAD);
	        
                duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU16CF_BAD_OBJ_TYPE_IN_IIPRIV, 8,
		    sizeof(prv_tbl.d_obj_type), &prv_tbl.d_obj_type,
		    sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	            sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
		    sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number);
                if (duve_talk(DUVE_ASK, duve_cb, 
	                S_DU0250_DELETE_IIPRIV_TUPLE, 8, 
	                sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	                sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
	                sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
	                sizeof(prv_tbl.d_obj_type), &prv_tbl.d_obj_type)
                    == DUVE_YES)
                {
	            EXEC SQL delete from iipriv where current of priv_cursor;

	            duve_talk( DUVE_MODESENS, duve_cb, 
		        S_DU0450_DELETED_IIPRIV_TUPLE, 8, 
	                sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	                sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
	                sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
	                sizeof(prv_tbl.d_obj_type), &prv_tbl.d_obj_type);
    
                }
		
		continue;
            }
            else if (!found)
            {
	        char	obj_type_str[50];
	        i4	type_len;
		i4	error_number;
    
	        /* 
	        ** object for which the descriptor has been created does not 
		** exist 
	        */
                if (duve_banner(DUVE_IIPRIV, 1, duve_cb) == DUVE_BAD) 
	            return ((DU_STATUS) DUVE_BAD);
	        
	        duve_obj_type((i4) prv_tbl.d_obj_type, obj_type_str, 
		    &type_len);
		
		if (obj_type == DB_VIEW || obj_type == DB_DBP)
		{
		    error_number = S_DU16E0_NONEXISTENT_OBJ;
		}
		else
		{
		    error_number = S_DU16E1_NO_OBJ_DEPENDS_ON_DESCR;
		}
    
                duve_talk(DUVE_MODESENS, duve_cb, error_number, 8,
		    type_len, 	       obj_type_str,
		    sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	            sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
		    sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number);
                if (duve_talk(DUVE_ASK, duve_cb, 
	                S_DU0250_DELETE_IIPRIV_TUPLE, 8, 
	                sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	                sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
	                sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
	                sizeof(prv_tbl.d_obj_type), &prv_tbl.d_obj_type)
                    == DUVE_YES)
                {
	            EXEC SQL delete from iipriv where current of priv_cursor;

	            duve_talk( DUVE_MODESENS, duve_cb, 
		        S_DU0450_DELETED_IIPRIV_TUPLE, 8, 
	                sizeof(prv_tbl.d_obj_id), &prv_tbl.d_obj_id,
	                sizeof(prv_tbl.d_obj_idx), &prv_tbl.d_obj_idx,
	                sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
	                sizeof(prv_tbl.d_obj_type), &prv_tbl.d_obj_type);
                }
		
		continue;
            }
	}

	/* end of test 1 */

	/*
	** test 2: verify that the user who created the object dependent on 
	** 	   privilege(s) described by this privilege descriptor still 
	**	   possesses them;
	**
	** If he does not possess them, we will report this fact to the user and
	** ask for permission to deal with offending object(s) as follows:  
	**   - if the privilege descriptor described privileges on which a view 
	**     depends, we will mark that view for destruction;
	**   - if the privilege descriptor described privileges on which a
	**     dbproc depends, that dbproc will be marked dormant
	**   - if the privilege descriptor described privileges on which 
	**     permit(s) on a view depended, we will mark that view for 
	**     destruction
	**   - if the privilege descriptor described privileges on which
	**     a constraint depended, we will mark that constraint for 
	**     destruction
	*/

	indep_obj_id.db_tab_base = prv_tbl.i_obj_id;
	indep_obj_id.db_tab_index = prv_tbl.i_obj_idx;

	/* 
	** permits depend of GRANT OPTION FOR privilege; 
	**
	** if the current record describes privilege on which a view depends
	** and there are some permits defined on it, we will check for 
	** SELECT WGO rather than SELECT to avoid having to check whether the 
	** owner of a view possesses GRANT OPTION FOR all privileges on which a
	** view depends when performing IIPROTECT tests 
	**
	** similarly, if the current record describes privilege on which a 
	** dbproc depends and there are some permits defined on it, we will 
	** check for <priv> WGO rather than <priv> to avoid having to check 
	** whether the owner of a dbproc possesses GRANT OPTION FOR all 
	** privileges on which a dbproc depends when performing IIPROTECT tests
	*/
	if (   prv_tbl.d_obj_type == DB_PROT
	    || (prv_tbl.d_obj_type == DB_VIEW && sel_perms_on_view)
	    || (prv_tbl.d_obj_type == DB_DBP && perms_on_dbp)
	   )
	{
	    priv_needed = prv_tbl.i_priv | DB_GRANT_OPTION;
	}
	else
	{
	    priv_needed = prv_tbl.i_priv;
	}

	MEcopy(&prv_tbl.i_priv_map[0], sizeof(attr_map), (char *) &attr_map[0]);

	duve_check_privs(&indep_obj_id, priv_needed, 
	    prv_tbl.i_priv_grantee, attr_map, &satisfied);

	if (!satisfied)
	{
            if (duve_banner(DUVE_IIPRIV, 2, duve_cb) == DUVE_BAD) 
	        return ( (DU_STATUS) DUVE_BAD);

	    /*
	    ** user specified in this IIPRIV tuple lacks privileges described by
	    ** the tuple - object will be marked for destruction
	    */
	    if (prv_tbl.d_obj_type == DB_VIEW || prv_tbl.d_obj_type == DB_PROT)
	    {
	        /*
	        ** note that if IIPRIV indicates that permit(s) depend on a
	        ** privilege and the grantor of the permit no longer possesses 
		** that privilege, rather than trying to drop the permit and 
		** launch the full-blown REVOKE processing, we will attempt to 
		** drop the object (a view) on which the permit was defined
	        */

		/* if the descriptor represents a privilege on which a permit
		** depends, lookup the definition of the view with which this 
		** permit is affiliated
		*/
		if (prv_tbl.d_obj_type == DB_PROT)
		{
		    view_offset = duve_findreltid(&obj_id, duve_cb);
		}

		if (view_offset == DU_INVALID || view_offset == DUVE_DROP)
		{
		    /* 
		    ** view_offset may get set to DU_INVALID if a view on which 
		    ** a permit was defined no longer exists - this condition 
		    ** will be checked in ckprot();
		    ** view_offset may get set to DUVE_DROP if a view on which a
		    ** permit was defined was has been marked for destruction
		    */
		    continue;
		}

		if (prv_tbl.d_obj_type == DB_PROT)
		{
		    duve_talk(DUVE_MODESENS, duve_cb, 
			S_DU16E3_LACKING_PERM_INDEP_PRIV, 8,
			0, prv_tbl.i_priv_grantee,
			sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
			0, duve_cb->duverel->rel[view_offset].du_own,
			0, duve_cb->duverel->rel[view_offset].du_tab);
		}
		else
		{
		    duve_talk(DUVE_MODESENS, duve_cb,
			S_DU16E2_LACKING_INDEP_PRIV, 10,
			0, prv_tbl.i_priv_grantee,
			sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
			sizeof("VIEW") - 1, "VIEW",
			0, duve_cb->duverel->rel[view_offset].du_own,
			0, duve_cb->duverel->rel[view_offset].du_tab);
		}

		if (duve_talk( DUVE_IO, duve_cb, S_DU0302_DROP_TABLE, 4,
			0, duve_cb->duverel->rel[view_offset].du_tab,
			0, duve_cb->duverel->rel[view_offset].du_own)
		    == DUVE_YES)
		{
		    idxchk(duve_cb, prv_tbl.d_obj_id, prv_tbl.d_obj_idx);
		    duve_cb->duverel->rel[view_offset].du_tbldrop = DUVE_DROP;
		    duve_talk( DUVE_MODESENS, duve_cb,
		        S_DU0352_DROP_TABLE, 4,
		        0, duve_cb->duverel->rel[view_offset].du_tab,
		        0, duve_cb->duverel->rel[view_offset].du_own);
		}
	    }
	    else if (prv_tbl.d_obj_type == DB_DBP)
	    { 
		DUVE_DROP_OBJ		*drop_obj;

		EXEC SQL select dbp_name, dbp_owner 
		    into :obj_name, :sch_name
		    from iiprocedure
		    where 
			    dbp_id  = :prv_tbl.d_obj_id 
			and dbp_idx = :prv_tbl.d_obj_idx;

		STtrmwhite(obj_name);
		STtrmwhite(sch_name);

		duve_talk(DUVE_MODESENS, duve_cb,
		    S_DU16E2_LACKING_INDEP_PRIV, 10,
		    0, prv_tbl.i_priv_grantee,
		    sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
		    sizeof("PROCEDURE") - 1, "PROCEDURE",
		    0, sch_name,
		    0, obj_name);

		/*
		** before asking user whether we may mark the dbproc dormant,
		** check the last entry added to the "fix it" list - if it 
		** describes the current procedure, we will avoid issuing 
		** redundant messages
		*/
		if (   !(drop_obj = duve_cb->duvefixit.duve_objs_to_drop)
		    || drop_obj->duve_obj_id.db_tab_base   != prv_tbl.d_obj_id
		    || drop_obj->duve_obj_id.db_tab_index  != prv_tbl.d_obj_idx
		    || drop_obj->duve_obj_type 		   != prv_tbl.d_obj_type
		   )
		{
		    if (duve_talk(DUVE_IO, duve_cb, 
			    S_DU0251_MARK_DBP_DORMANT, 0)
		        == DUVE_YES)
		    {

			/* 
			** user decided to allow us to mark the dbproc 
			** dormant - add its description to the "fixit" list 
			*/

			drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0, 
			    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
			if (mem_stat != OK || drop_obj == NULL)
			{
			    duve_talk (DUVE_ALWAYS, duve_cb, 
				E_DU3400_BAD_MEM_ALLOC, 0);
			    return ((DU_STATUS) DUVE_BAD);
			}

			drop_obj->duve_obj_id.db_tab_base  = prv_tbl.d_obj_id;
			drop_obj->duve_obj_id.db_tab_index = prv_tbl.d_obj_idx;
			drop_obj->duve_obj_type            = prv_tbl.d_obj_type;
			drop_obj->duve_secondary_id	   = 0;
			drop_obj->duve_drop_flags	   = 0;

			/* link this description into the existing list */
			drop_obj->duve_next = 
			    duve_cb->duvefixit.duve_objs_to_drop;

			duve_cb->duvefixit.duve_objs_to_drop = drop_obj;
		        duve_talk(DUVE_MODESENS, duve_cb, 
			    S_DU0451_MARKED_DBP_DORMANT, 0);
		    }
		}
	    }
	    else if (prv_tbl.d_obj_type == DB_CONS)
	    { 
	    	DU_STATUS	indep_offset;
		DU_STATUS       tbl_offset;
		DU_I_DPNDS      *indep;
		DU_D_DPNDS	*dep;

		tbl_offset = duve_findreltid(&obj_id, duve_cb);

		if (tbl_offset == DU_INVALID || tbl_offset == DUVE_DROP)
		{
		    /* 
		    ** tbl_offset may get set to DU_INVALID if a table on which
		    ** a constraint was defined no longer exists - this 
		    ** condition will be checked in ckinteg();
		    ** tbl_offset may get set to DUVE_DROP if a table on which a
		    ** constraint was defined was has been marked for 
		    ** destruction
		    */
		    continue;
		}

		duve_talk(DUVE_MODESENS, duve_cb, 
		    S_DU16E4_LACKING_CONS_INDEP_PRIV, 8,
		    0, prv_tbl.i_priv_grantee,
		    sizeof(prv_tbl.d_priv_number), &prv_tbl.d_priv_number,
		    0, duve_cb->duverel->rel[tbl_offset].du_own,
		    0, duve_cb->duverel->rel[tbl_offset].du_tab);

		/* 
		** find the first entry in the independent object info list of 
		** IIDBDEPENDS cache that refers to the current privilege 
		** descriptor
		*/
		indep_offset = duve_i_dpndfind((i4) obj_id.db_tab_base, 
		    (i4) obj_id.db_tab_index, (i4) DB_PRIV_DESCR, 
		    (i4) priv_number, duve_cb, (bool) TRUE);

		for (indep = duve_cb->duvedpnds.duve_indep + indep_offset;
		     indep_offset < duve_cb->duvedpnds.duve_indep_cnt;
		     indep++, indep_offset++)
		{
		    DU_I_DPNDS      *cur;

		    /*
		    ** if this independent object ino element does not describe 
		    ** our privilege descriptor, we are done
		    */
		    if (   obj_id.db_tab_base  	!= indep->du_inid.db_tab_base
			|| obj_id.db_tab_index 	!= indep->du_inid.db_tab_index
			|| indep->du_itype     	!= DB_PRIV_DESCR
			|| priv_number	       	!= indep->du_iqid)
		    {
			break;
		    }

		    /*
		    ** find the dependent object info element to which this 
		    ** independent object info element belongs
		    */
		    for (cur = indep;
			 ~cur->du_iflags & DU_LAST_INDEP;
			 cur = cur->du_next.du_inext)
		    ;

		    /*
		    ** cur now points at the last independent object info 
		    ** element in a ring associated with a given dependent 
		    ** object info element - cur->du_next.du_dep points at the 
		    ** dependent object info element 
		    ** (I would be very surprised if the dependent object type 
		    ** were not DB_CONS, but things may change, so let's check)
		    */
		    dep = cur->du_next.du_dep;

		    if (prv_tbl.d_obj_type == dep->du_dtype)
		    {
			DUVE_DROP_OBJ           *drop_obj;

			/*
			** before asking user whether we may drop this 
			** constraint, check the last entry added to the 
			** "fix it" list - if it describes the current 
			** constraint, we will avoid issuing redundant messages
			*/
			if (   !(drop_obj = 
				     duve_cb->duvefixit.duve_objs_to_drop)
			    || drop_obj->duve_obj_id.db_tab_base   != 
				   dep->du_deid.db_tab_base
			    || drop_obj->duve_obj_id.db_tab_index  != 
				   dep->du_deid.db_tab_index
			    || drop_obj->duve_obj_type 		   != 
				   dep->du_dtype
			    || drop_obj->duve_secondary_id         != 
				   dep->du_dqid
			   )
			{
			    tid = dep->du_deid.db_tab_base;
			    tidx = dep->du_deid.db_tab_index;
			    qid = dep->du_dqid;

			    EXEC SQL select consname into :obj_name
				from iiintegrity
				where 
					inttabbase = :tid
				    and inttabidx  = :tidx
				    and intnumber  = :qid
				    and consflags  != 0;

			    if (duve_talk(DUVE_IO, duve_cb, 
			            S_DU0247_DROP_CONSTRAINT, 4,
				    0, duve_cb->duverel->rel[tbl_offset].du_own,
				    0, obj_name)
				== DUVE_YES)
			    {

				/* 
				** user decided to drop the constraint - add its
				** description to the "fixit" list 
				*/

				drop_obj = (DUVE_DROP_OBJ *) MEreqmem(0,
				    sizeof(DUVE_DROP_OBJ), TRUE, &mem_stat);
				if (mem_stat != OK || drop_obj == NULL)
				{
				    duve_talk (DUVE_ALWAYS, duve_cb, 
					E_DU3400_BAD_MEM_ALLOC, 0);
				    return ((DU_STATUS) DUVE_BAD);
				}

				drop_obj->duve_obj_id.db_tab_base  = 
				    dep->du_deid.db_tab_base;
				drop_obj->duve_obj_id.db_tab_index = 
				    dep->du_deid.db_tab_index;
				drop_obj->duve_obj_type            = 
				    dep->du_dtype;
				drop_obj->duve_secondary_id	   = 
				    dep->du_dqid;
				drop_obj->duve_drop_flags	   = 0;

				/* 
				** link this description into the existing list 
				*/
				drop_obj->duve_next = 
				    duve_cb->duvefixit.duve_objs_to_drop;

				duve_cb->duvefixit.duve_objs_to_drop = drop_obj;

				duve_talk(DUVE_MODESENS, duve_cb,
				    S_DU0447_DROP_CONSTRAINT, 4,
				    0, duve_cb->duverel->rel[tbl_offset].du_own,
				    0, obj_name);
			    }
			}
		    }
		}
	    }
	}

	/* end of test 2 */
    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckpriv"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: duve_check_privs - determine whether a specified user possesses 
**			    required privilege on the specified object
**
** Description:
**      this routine will scan iiprotect trying to determine whether a specified
**      user (or PUBLIC) possess required privilege on the specified object
**
** Inputs:
**	objid			id of the object
**	privneeded		required privilege
**	privgrantee		user who must possess the required privilege
**	attrmap			map of attributes of a table or view on which 
**				the user must possess a specified privilege
**
** Outputs:
**      satisfied		TRUE if the specified user (or PUBLIC) possess 
**				required privilege on the specified object; 
**				FALSE otherwise
** Returns:
**  	none
**
** Exceptions:
**    	none
**
** Side Effects:
**	none
**
** History:
**      20-jan-94 (andre)
**          written
**	9-Sep-2004 (schka24)
**	    1024 column expansion
**	28-Feb-2008 (kibro01) b119427
**	    The DBA doesn't have the GRANT option in the bitmask in iiprotect,
**	    but they always have an implicit GRANT option to anything.
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap.
*/
static VOID
duve_check_privs( 
		 DB_TAB_ID	*objid, 
		 i4		privneeded, 
		 char		*privgrantee, 
		 u_i4		*attrmap, 
		 bool		*satisfied)
{
    EXEC SQL BEGIN DECLARE SECTION;
	u_i4	priv_tid;
	u_i4	priv_tidx;
	char	*priv_grantee;
	i4	priv;
	i4	priv_wgo;
	i4	priv_wogo;
	i4	user_grantee;
	i4	public_grantee;
	char	attr_bytes[DB_COL_BYTES];
	i4	cnt;
    EXEC SQL END DECLARE SECTION;

    *satisfied = FALSE;

    priv_tid = objid->db_tab_base;
    priv_tidx = objid->db_tab_index;
    priv_grantee = privgrantee;

    /* 
    ** if an object depends on [GRANT OPTION FOR] SELECT, we need to set 
    ** two more bits in priv_needed since a permit conveying SELECT 
    ** privilege has three bits set in iiprotect.proopset: DB_RETRIEVE, 
    ** DB_TEST, and DB_AGGREGATE
    */

    priv = (privneeded & DB_RETRIEVE) ? (privneeded | DB_TEST | DB_AGGREGATE)
				      : privneeded;
    priv_wgo = priv | DB_GRANT_OPTION;
    priv_wogo = priv & ~DB_GRANT_OPTION;
    user_grantee = DBGR_USER;
    public_grantee = DBGR_PUBLIC;

    /* 
    ** we need to make sure that the privilege applies to all columns 
    ** described in attrmap only if the privilege is on a table; otherwise we
    ** only need to ascertain that there is a tuple in IIPROTECT which grants
    ** specified privilege to the specified user (or public) on the specified 
    ** object
    */
    if (  privneeded 
	& (DB_RETRIEVE | DB_REPLACE | DB_DELETE | DB_APPEND | 
		DB_COPY_FROM | DB_COPY_INTO | DB_REFERENCES))
    {
        i4	i;
        u_i4	attrs_to_find[DB_COL_WORDS];

	/* 
	** make a copy of users attribute map since we plan to clear some (and
	** hope to clear all) bits in it
	*/

	for (i = 0; i < DB_COL_WORDS; i++)
	    attrs_to_find[i] = attrmap[i];

        EXEC SQL DECLARE check_priv_cursor CURSOR FOR
	    select prodomset
	    from iiprotect
	    where 
		    protabbase = :priv_tid
	        and protabidx  = :priv_tidx
	        and (   (progtype = :user_grantee and prouser = :priv_grantee)
		     or progtype = :public_grantee)
		and prograntor != :priv_grantee
	        and (proopset = :priv or proopset = :priv_wgo or 
		     (proopset = :priv_wogo and prouser = DBMSINFO('DBA')));

        EXEC SQL OPEN check_priv_cursor FOR READONLY;

	/* 
	** examine all tuples conveying specified privilege to the specified 
	** user (or public) on the specified object
	*/
        for (; !*satisfied; )  
        {
	    /* get tuple from iiprotect */
	    EXEC SQL FETCH check_priv_cursor 
		INTO :attr_bytes;
	    if (sqlca.sqlcode == 100) /* then no more tuples in table */
	    {
	        EXEC SQL CLOSE check_priv_cursor;
	        break;
	    }

	    /* be optimistic */
	    *satisfied = TRUE;

	    for (i = 0; i < DB_COL_WORDS; i++)
	    {
		if (attrs_to_find[i] &= ~*((u_i4 *)(&attr_bytes[0]) + i))
		    *satisfied = FALSE;
	    }
	}
	
	if (*satisfied)
	{
	    /*
	    ** we exited the loop because we determined that the specified 
	    ** user possesses required privilege on the specified table or 
	    ** view - must remember to close the cursor
	    */
	    EXEC SQL CLOSE check_priv_cursor;
	}
    }
    else
    {
	EXEC SQL REPEATED select any(protabbase) into :cnt
	    from iiprotect
	    where
		    protabbase = :priv_tid
		and protabidx  = :priv_tidx
		and (   (progtype = :user_grantee and prouser = :priv_grantee)
		     or progtype = :public_grantee)
		and (proopset = :priv or proopset = :priv_wgo or
		     (proopset = :priv_wogo and prouser = DBMSINFO('DBA')));
	*satisfied = (cnt != 0);
    }
    
    return;
}

/*{
** Name: ckkey - run verifydb test #1 to check system catalog iikey
**
** Description:
**      This routine opens a cursor to walk iikey one tuple at a time.
**	It performs checks on each tuple.
**
**      TEST 1: Verify that for each constraint id, all columns in its key
**                are legal columns (for the table) and that all positions
**                (1-n) in the key have columns assigned to them.
**              TEST:   Select count(attid) from iikey k, iiattribute a,
**                             iirelation r
**                      where k.key_attid = a.attid and
**                             a.attrelid = r.reltid.
**              If count(attid) is zero, then we have detected error.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      none.
**
**      Returns:
**          DUVE_YES
**          DUVE_BAD
**      Exceptions:
**          none
** Side Effects:
**          tuples read from iiattribute
**          tuples read from iirelation
**
** Histry:
**      18-nov-92 (anitap)
**          initial creation
**	04-nov-93 (anitap)
**	    Added null indicator variable 'keyind' to get rid of the following
**	    error: "E_LQ000E No null indicator supplied with program variable.
**	    Null data from column 3 can not be converted."
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	01-jan-94 (anitap)
**	    Performance enhancements. Use repeat query and ANY instead
**	    of COUNT.
**	09-april-2003 (inifa01) bug 110019 INGSRV 2204
**	    verifydb ... -odbms check on a referential constraint, where constraint 
**	    columns are not part of a key, returns error;
**	    S_DU168F_ILLEGAL_KEY Attribute with constraint id <ID> is not a legal
**	    column for the key. 
**	    Fix: Check if attkdom in iiattribute is not turned on b/cos we're dealing
**	    with a CONS_REF, then don't return error. 
[@history_template@]...
*/
DU_STATUS
ckkey (duve_cb)
DUVE_CB                 *duve_cb;
{

    DU_STATUS                base;

    EXEC SQL BEGIN DECLARE SECTION;
        exec sql include <duvekey.sh>;
        short keyind[4];	
        i4 cnt;
	i4 cons_type;
	i4 id1, id2;
    EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE key_cursor CURSOR FOR
        select * from iikey;
    EXEC SQL open key_cursor FOR READONLY;

    /* loop for each tuple in iikey */
    for (;;)
    {
        /* get tuple from table */
        EXEC SQL FETCH key_cursor INTO :key_tbl:keyind;
        if (sqlca.sqlcode == 100) /* then no more tuples in iikey */
        {
           EXEC SQL CLOSE key_cursor;
           break;
        }

        /* test 1 -- verify that for each constraint id, all columns in its
	**	     key are legal columns and that all positions (1-n)
	**	     in the key have columns assigned to them.
	*/

	EXEC SQL repeated select any(attid)
		into :cnt 
		from iiattribute, iiintegrity 
		where consid1 = :key_tbl.key_consid1
		and consid2 = :key_tbl.key_consid2
		and attkdom = :key_tbl.key_position
		and inttabbase = attrelid;

        if (cnt == 0)
        {
           /* The key tuple is not referenced in iiattribute
           */
           if (duve_banner(DUVE_IIKEY, 1, duve_cb)
                  == DUVE_BAD)
                return ( (DU_STATUS) DUVE_BAD);

	   /* (inifa01) could be no rows where returned b/cos 
	      attkdom == 0 != :key_tbl.key_position, which will be
	      the case for referential constraint columns not part
	      of a key eg CONS_REF created WITH NO INDEX.
	   */
	   EXEC SQL repeated select consflags into :cons_type from
		    iiintegrity where consid1=:key_tbl.key_consid1
		    and consid2 = :key_tbl.key_consid2;
	   if (!(cons_type & CONS_REF))
               duve_talk (DUVE_MODESENS, duve_cb, S_DU168F_ILLEGAL_KEY, 2,
                        0, key_tbl.key_consid1);
           continue;
        } /* end test 1 */

    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckkey"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ckdefault - run verifydb test #1 to check system catalog iidefault
**
** Description:
**      This routine opens a cursor to walk iidefault one tuple at a time.
**      It performs checks and gives user the option to delete the default
**      tuple.
**
**      TEST 1: Verify tht each non-canonical default tuple is referenced
**               by some attribute.
**              TEST:   Select count(defid1) from iidefault , iiattribute a
**                      where d.defid1 = a.attdefid1 and
**                              d.defid2 = a.attdefid2.
**              if any(defid1) is zero, then we have detected error.
**      FIX:    Offer the user the option to delete the default tuple.
**
** Inputs:
**     duve_cb                         verifydb control block
**
** Outputs:
**      none.
**
**      Returns:
**          DUVE_YES
**          DUVE_BAD
**      Exceptions
**          none
** Side Effects:
**          tuples read from iiattribute
**          tuples may be deleted from iidefault
**
** History:
**     18-nov-92 (anitap)
**         initial creation.
**     25-oct-93 (anitap)
**	   Added indicator variable for default_tbl.
**	10-dec-93 (teresa)
**	    performance enchancement for verifydb.  Includes performance 
**	    statistics reporting, changing many queries to repeated queries,
**	    modifying existing queries to be more efficient (such as using
**	    key columns in the qualification).  Also added logic to suppress
**	    some non-significant difs during run-all testing, including a
**	    change to duve_log parameters).
**	20-dec-93 (anitap)
**	    Use repeat query and ANY instead of COUNT to improve performance.
**	    Added the two cases DB_DEF_ID_NEEDS_CONVERTING & 
**	    DB_DEF_ID_UNRECOGNIZED to test 3.
**	    
[@history_template@]..
*/
DU_STATUS
ckdefault (duve_cb)
DUVE_CB                 *duve_cb;
{

   DU_STATUS                base;

   EXEC SQL BEGIN DECLARE SECTION;
       exec sql include <duvedef.sh>;
       short defind[3];	
       i4 	cnt;
   EXEC SQL END DECLARE SECTION;

    /* set up to gather performance statistics */
    init_stats(duve_cb, (i4) DUVE_TEST_PERF);    

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE default_cursor CURSOR FOR
        select * from iidefault;
    EXEC SQL open default_cursor;

    /* loop for each tuple in iidefault */
    for (;;)
    {
	/* get tuple from table */
        EXEC SQL FETCH default_cursor INTO :default_tbl:defind;
        if (sqlca.sqlcode == 100) /* then no more tuples in iidefault */
        {
          EXEC SQL CLOSE default_cursor;
          break;
        }


        /* test 1 -- verify each non-canonical default tuple is referenced
        ** by some attribute
        */
	/* check if canonical default */
	switch(default_tbl.defid1)
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
            {
                /* "special" default - no need to check IIDEFAULT */
                break;
            }
            default:
            {
	   	EXEC SQL repeated select any(attdefid1) into :cnt 
		    from iiattribute
		    where attdefid1 = :default_tbl.defid1 and
			  attdefid2 = :default_tbl.defid2;

	   	if (cnt == 0)
	   	{
	      	    /* The default tuple is not referenced in iiattribute */

	      	    if (duve_banner(DUVE_IIDEFAULT, 1, duve_cb) == DUVE_BAD)
		      return ( (DU_STATUS) DUVE_BAD);
	      	   duve_talk (DUVE_MODESENS, duve_cb,
			S_DU168E_UNREFERENCED_DEFTUPLE, 4,
			0, default_tbl.defid1, 0, default_tbl.defid2);
	      	        if (duve_talk(DUVE_ASK, duve_cb, S_DU034B_DELETE_IIDEFAULT, 
			    4, 0, default_tbl.defid1, 0, default_tbl.defid2)
				    == DUVE_YES) 
	           {
		      EXEC SQL delete from iidefault
			    where current of default_cursor;
		      duve_talk(DUVE_MODESENS, duve_cb,
			    S_DU039B_DELETE_IIDEFAULT, 4,
			    0, default_tbl.defid1, 0, default_tbl.defid2);
	           }
	           continue;
	       } /* endif cnt = null */ 
	       break;
	} 
      } /* end test 1 */	

    } /* end loop for each tuple */

    /* report performance statistics */
    printstats(duve_cb, (i4) DUVE_TEST_PERF, "ckdefault"); 

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: treeclean - clean up integrities, permits, views for which th tree
**		     is dropped.
**	         
**
** Description:
**      This routine drops view, integrity, permit and rule tuples for
**	which the tree is dropped, as follows:
**	  case tree_table.treemode
**	    DB_VIEW - mark in duverel as dropped
**	    DB_PROT - delete tuple from iiprotect
**	    DB_RULE - delete tuple from iirule
**	    DB_INTG - delete tuple from iiintegrity
**
**
** Inputs:
**	base				offset into duverel for this entry
**					  (only used for views)
**	tree				table id and tree id
**	mode				tree type (from tree_tbl.treemode)
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from iiprotect, iiintegrity or iirule. A
**	    view tuple in duverel cache may be marked for deletion
**
** History:
**      20-feb-91 (teresa)
**          initial creation for bug 35940
*/
VOID
treeclean( base, tree, mode, duve_cb)
DU_STATUS	   base;
DUVE_TREE	   *tree;
short		   mode;
DUVE_CB		   *duve_cb;
{
    EXEC SQL WHENEVER SQLERROR CONTINUE; 	/* ignore sql errors here */
    EXEC SQL BEGIN DECLARE SECTION;
	u_i4	tid, tidx;
	u_i4	tree1, tree2;
    EXEC SQL END DECLARE SECTION;


    switch (mode)
    {
    case DB_VIEW:
	/* mark this view as dropped */
	duve_cb->duverel->rel[base].du_tbldrop = DUVE_DROP;
	break;
    case DB_PROT:
	tid = tree->tabid.db_tab_base;
	tidx = tree->tabid.db_tab_index;
	tree1 = tree->treeid.db_tre_high_time;
	tree2 = tree->treeid.db_tre_low_time;
	exec sql delete from iiprotect where protabbase = :tid and
	    protabidx = :tidx and protreeid1 = :tree1 and protreeid2 = :tree2;
	break;
    case DB_INTG:
	tid = tree->tabid.db_tab_base;
	tidx = tree->tabid.db_tab_index;
	tree1 = tree->treeid.db_tre_high_time;
	tree2 = tree->treeid.db_tre_low_time;
	exec sql delete from iiintegrity where inttabbase = :tid and
	    inttabidx = :tidx and inttreeid1 = :tree1 and inttreeid2 = :tree2;
	break;
    case DB_RULE:
	tree1 = tree->treeid.db_tre_high_time;
	tree2 = tree->treeid.db_tre_low_time;
	exec sql delete from iirule where 
	    rule_treeid1 = :tree1 and rule_treeid2 = :tree2;
	break;
    default:
	break;
    }
}

/*{
** Name: create_cons - drop constraint and recreate the same from the qrytext
**			in iiqrytext catalog.
**
** Description:
**	This routine drops UNIQUE/CHECK/REFERENTIAL constraint on a table and
**	creates a SQL file to create the same. The constraint is dropped 
**	because one of the following database objects may have been missing: 
**	rule, procedure, index, iidbdepends tuple.
**
** Inputs:
**	duve_cb			verifydb control block
**	iiintegrity		ptr to structure containing all attributes in 
**				iiintegrity tuple
**	base			offset into iirelation cache for the table
**				on which the constraint needs to be 
**				dropped/added
**
** Outputs:
**	duve_cb			verifydb control block
**	Returns:
**		DUVE_YES,
**		DUVE_NO,
**		DUVE_BAD
**	Exceptions:
**		none
**
** Side Effects:
**	old tuples in iirule/iiprocedure/iiindex/iiintegrity/iidbdepends
**	/iiqrytext may be dropped.	
**	new tuples may be entered into iirule/iiprocedure/iiindex/iiintegrity/
**	iidbdepends/iiqrytext at the end of verifydb.
**
** History:
**	01-nov-93 (anitap)
**	    initial creation.
**	05-jan-94 (anitap)
**	    use iidbdepends cache to count the number of referential constraints
**	    dependent on the unique constraint (which is being dropped).
**	    Also take care of the condition when more than one referential 
**	    constraint is defined on a table.
**	    Allocate memory dynamically for cases where the query text sstring
**	    spans more than one tuple in iiqrytext catalog.	
[@history_template@]...
*/

static DU_STATUS
create_cons (duve_cb, iiintegrity, base)
DUVE_CB		*duve_cb;
DB_INTEGRITY	*iiintegrity;
DU_STATUS	base;
{

	exec sql begin declare section;
	    u_i4 	qryid1, qryid2;
	    u_i4	inum, rflag;
    	    u_i4	id1, id2;
	exec sql end declare section;

        u_i4		tid, tidx, flag;	
        u_i4 		len;	
        char		cons[DB_MAXNAME + 1];
	DB_TAB_ID	ref_id;
	DU_STATUS	rbase;
	DU_STATUS	indep_offset, status;
	DU_I_DPNDS	*indep;
	DU_D_DPNDS	*dep;
	DB_QRY_ID	qyid;
	i4		i;
	bool		ref = FALSE;
	bool		first = TRUE;
        i4		ind_type = DB_CONS; 
	
	EXEC SQL WHENEVER SQLERROR call Clean_Up;

	tid = iiintegrity->dbi_tabid.db_tab_base;
	tidx = iiintegrity->dbi_tabid.db_tab_index;
	inum = iiintegrity->dbi_number;
	flag = iiintegrity->dbi_consflags;
	qyid.db_qry_high_time = iiintegrity->dbi_txtid.db_qry_high_time;
	qyid.db_qry_low_time = iiintegrity->dbi_txtid.db_qry_low_time;

	STcopy(iiintegrity->dbi_consname.db_constraint_name, cons);

        len = STtrmwhite(cons);
	  
	/* cache info about this tuple */
        MEcopy( (PTR)cons, DB_MAXNAME,
         (PTR) duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_consname);

        MEcopy( (PTR)duve_cb->duverel->rel[base].du_own,
			DB_MAXNAME,           
	   (PTR)duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_ownname);

        MEcopy( (PTR)duve_cb->duverel->rel[base].du_tab, DB_MAXNAME,
           (PTR)duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_tabname);
      	   
	/* 
      	** drop constraint.
        */
      	duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU0370_DROP_FROM_IIINTEGRITY,
		 4, sizeof(tid), &tid, sizeof(tidx), &tidx);
	  
        if (duve_talk (DUVE_ASK, duve_cb, S_DU0246_CREATE_CONSTRAINT, 0)
			== DUVE_YES)
	{
           duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_acons =
                               DUVE_ADD;
	  
	   /* Get the query text for creating the constraint */
	   if ((status = alloc_mem(duve_cb, &qyid, 
	     &duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_txtstring))
				!= DUVE_YES)
	      return(status);

	   if (flag == CONS_UNIQUE)
	   {

	      /* If UNIQUE constraint, we can have a referential constraint
	      ** depending on this constraint. Dropping the UNIQUE constraint
	      ** will also drop the REFERENTIAL constraint.
	      ** 
	      ** We want to get the query text for the referential constraint
	      ** before dropping the UNIQUE constraint.
	      ** We can do this by querying the IIDBDEPENDS catalog.
	      */

	      indep_offset = duve_i_dpndfind( tid, tidx,
				ind_type, inum, duve_cb, first);

	      if (indep_offset != DU_INVALID)
	      {
	         indep = duve_cb->duvedpnds.duve_indep + indep_offset;

	         for (i = 0, dep = indep->du_next.du_dep;
			i < duve_cb->duvedpnds.duve_dep_cnt;
			i++, dep++)
	         {
		     if (dep->du_dtype == DB_CONS
			 && dep->du_indep->du_inid.db_tab_base ==
				indep->du_inid.db_tab_base)
		     {
		        id1 = dep->du_deid.db_tab_base;
		        id2 = dep->du_deid.db_tab_index;
			inum = dep->du_dqid;

		        /* We have a referential constraint depending on this
	                ** unique constraint.
		        ** Get the query text for the referential constraint.
		        */
		        /* We have a referential constraint depending on this
                        ** unique constraint.
                        */

                        /* Get the table name and owner on which
                        ** referential constraint
                        ** needs to be defined.
                        */

	                ref_id.db_tab_base = id1;
		        ref_id.db_tab_index = id2;

                        rbase = duve_findreltid ( &ref_id, duve_cb);

	                /* We do not want to recreate the referential
		        ** constraint on the table if it has been marked
			** for drop.
		        */
	 	        if (rbase != DUVE_DROP ||
			     rbase != DU_INVALID)
		        {

	                   /* cache info about the constraint text,
			   ** table name and owner
                    	   */

                   duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_rcons =
                                         DUVE_ADD;

                           MEcopy( (PTR)duve_cb->duverel->rel[rbase].du_own,
					DB_MAXNAME,
          (PTR)duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_rownname);

                   MEcopy( (PTR)duve_cb->duverel->rel[rbase].du_tab, DB_MAXNAME,
          (PTR)duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_rtabname);

		           rflag = CONS_REF;

		           EXEC SQL repeated select intqryid1, intqryid2
				into :qryid1, :qryid2
				from iiintegrity
				where inttabbase = :id1
				and inttabidx = :id2
				and consflags = :rflag
				and intnumber = :inum;

			   qyid.db_qry_high_time = qryid1;
			   qyid.db_qry_low_time = qryid2;

	   	           /* Get the query text for creating the 
		           ** referential constraint 
		           */
			   if ((status = alloc_mem(duve_cb, &qyid,
	  &duve_cb->duveint->integrities[duve_cb->duve_intcnt].du_rtxtstring))
				!= DUVE_YES)
			      return(status);

		} /* endif rbase != DU_INVALID or DUVE_DROP */

	        ref = TRUE; 
	        duve_cb->duve_intcnt++;
	      } /* endif referential constraint dependent on this unique cons*/	

	     } /* end of for loop */	
	    } /* endif indep_offset != DU_INVALID */

	   } /* endif consflags = CONS_UNIQUE */

	   if (ref == FALSE)
	      duve_cb->duve_intcnt++;
	   
      	   duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU0446_CREATE_CONSTRAINT, 0);

	} /* endif create constraint */

	return ((DU_STATUS)DUVE_YES);	
	      
}

/*{
** Name: alloc_mem - allocate memory dynamically to cache the query text to 
**			create the constraint and cache the query text.
**
** Description:
** 	If the integrity tuple spans more than one tuple, we need to
** 	dynamically allocate the memory for caching the query text
** 	string.
**
** 	We need to find the len of the total query text. We do thiis
** 	by calculating the length of the last tuple and adding to the
** 	product of the sequence and 240, i.e., len(last tuple) 
** 	+ (seq * 240).
**
** 	We find the length of the last tuple by counting the number of
** 	iiqrytext tuples and using count - 1 to match with sequence
** 	number.
**
**	This routine is called to allocate memory and cache query text for
**	both unique constraint and for referential constraint which depends
**	on the unique constraint.
**
** Inputs:
**	duve_cb		verifydb control block
**	iiqrytext	structure containing
**			query text id of text which needs to be cached
**	string		pointer to query text string where the query text will
**			be cached.
**
** Outputs:
**	duve_cb		verifydb control block
**	Returns:
**		DUVE_YES,
**		DUVE_BAD
**	Exceptions:
**		none
**
** Side Effects:
**	tuples will be read from iiqrytext.
**
** History:
**	09-jan-94 (anitap)
**	    Created for FIPS' constraints project.
**	    Corrected memory corruption bug. Was allocating correct memory.
**	    But was trying to MEcopy a larger chunk of test string. 
**	    Also null terminate the text string.
[@history_template@]...
*/
static DU_STATUS
alloc_mem(duve_cb, quyid, string)
DUVE_CB		*duve_cb;
DB_QRY_ID	*quyid;	
char		**string;
{

	exec sql begin declare section;
		i4	cnt = 0;
		u_i4	queryid1;
		u_i4	queryid2;
		char	text[240];
	exec sql end declare section;

	STATUS          mem_text;
	u_i4           size;
	u_i4		length, str_len;
	char		*save_ptr;
	char		*txt_ptr;
	i4		i;

	EXEC SQL WHENEVER SQLERROR call Clean_Up;

	queryid1 = quyid->db_qry_high_time;
	queryid2 = quyid->db_qry_low_time;

        EXEC SQL select count(txtid1) into :cnt
		from iiqrytext
		where txtid1 = :queryid1
		and txtid2 = :queryid2;

        cnt--;

	/* Get the length of the last text string */
        EXEC SQL select txt into :text from iiqrytext 
		where txtid1 = :queryid1
		and txtid2 = :queryid2
		and seq = :cnt;

        size = STlength(text) + (cnt * 240);

	/* Assign memory */ 

        txt_ptr	= (char *) MEreqmem(0, size, TRUE, &mem_text);
	   
	if (mem_text != OK || txt_ptr == NULL)
        {
           duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, 0);
	   return ( (DU_STATUS) DUVE_BAD);
    	}

	*string = txt_ptr;
   	save_ptr = txt_ptr;
	  
        for (i = 0, length = 0; i <= cnt; i++)
	{ 	
	    EXEC SQL repeated select txt into :text from iiqrytext where
			txtid1 = :queryid1 and
			txtid2 = :queryid2
			and seq = :cnt;


            /* cache info about the constraint text
	    */

	    str_len = STlength(text);

            MEcopy( (PTR)text, str_len, (PTR)txt_ptr);

            length = length + STlength(text);

	    txt_ptr += length;
	 }

	 /* Null terminate the text string */
	 *txt_ptr = '\0';

	 /* Restore the pointer to the beginning of the text string */
	 txt_ptr = save_ptr;

	 return DUVE_YES;
}

/*{
** Name: DEBUG UTILITY ROUTINES:
**	    get_stats()	 - get _bio_cnt, _cpu_ms, _dio_cnt via dbms info
**	    init_stats() - Initialize gathering of performance statistics
**	    printstats() - Complete performanc statistics gather and report them
**
** Description:
**      These utilities are meant to allow verifydb (in debug mode only) to
**	gather and report statistics for elapsed CPU time, DIO and BIO count.
**	The sequence is as follows:
**	    call init_stats() to initialize and gather "before" statistics"
**	    call printstats() to gather "after" statistics, calculate actual
**		statistics as AFTER - BEFORE.  printstats() also writes the
**		statistics to the verifydb log file.
**	NOTE:
**	    routine get_stats() is a utility to support init_stats() and 
**	    printstats(), and should never be called except by those two 
**	    routines.
**
** Inputs:
**	duve_cb		    ptr to verifydb control block.
**	id		    ASCII identifier string (null terminated)
** Outputs:
**	duve_cb		    ptr to verifydb control block.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    An entry is written to the verifydb log file.
**
** History:
**      10-dec-1993 (teresa)
**          initial creation
[@history_template@]...
*/
VOID
get_stats(i4 *bio, i4 *dio, i4 *ms)
{
    exec sql begin declare section;
	char bio_str[DB_MAXNAME+1],dio_str[DB_MAXNAME+1],ms_str[DB_MAXNAME+1];
    exec sql end declare section;

    EXEC SQL WHENEVER SQLERROR CONTINUE; 	/* ignore sql errors here */

    /* get _bio_cnt, _cpu_ms, _dio_cnt via dbms info */
    exec sql select dbmsinfo('_bio_cnt'), dbmsinfo('_dio_cnt'),
		    dbmsinfo('_cpu_ms')  into :bio_str, :dio_str, :ms_str;
    if (sqlca.sqlcode < 0)
    {
	*bio = 0;
	*dio = 0;
	*ms = 0;
    }
    else
    {
	/* convert ascii to integer */
	CVal (bio_str, bio);
	CVal (dio_str, dio);
	CVal (ms_str, ms);

    }
}

VOID
init_stats(DUVE_CB *duve_cb, i4  test_level)
{

    if (duve_cb->duve_debug)
    {
	DUVE_PERF   *perf_info;

	if (test_level == DUVE_TEST_PERF)
	    perf_info = &duve_cb->duve_test_perf_numbers;
	else
	    perf_info = &duve_cb->duve_qry_perf_numbers;

	perf_info->bio_after = 0;
	perf_info->dio_after = 0;
	perf_info->ms_after = 0;
	get_stats(&perf_info->bio_before, &perf_info->dio_before,
			&perf_info->ms_before);
    }
}

VOID
printstats(DUVE_CB *duve_cb, i4  test_level, char *id)
{
    i4 bio, dio, ms;
    char msg_buffer[500];


    if (duve_cb->duve_debug)
    {
	DUVE_PERF   *perf_info;

	if (test_level == DUVE_TEST_PERF)
	    perf_info = &duve_cb->duve_test_perf_numbers;
	else
	    perf_info = &duve_cb->duve_qry_perf_numbers;

	/* get "after" stats */
	get_stats(&perf_info->bio_after, &perf_info->dio_after,
	    &perf_info->ms_after);

	/* calculate delta of after minus before */
	bio = perf_info->bio_after - perf_info->bio_before;
	dio = perf_info->dio_after - perf_info->dio_before;
	ms = perf_info->ms_after - perf_info->ms_before;

	/* handle any esqlc failure */
	if ( (perf_info->bio_after == 0) || (perf_info->bio_before < 0) )
	    bio = 0;
	if ( (perf_info->dio_after == 0) || (perf_info->dio_before < 0) )
	    dio = 0;
	if ( (perf_info->ms_after == 0) || (perf_info->ms_before < 0) )
	    ms = 0;

	/* output a message to the verifydb log file */
	if (id)
	    STprintf( msg_buffer, "debug: %s -- CPU_MS: %d, BIO: %d, DIO: %d\n",
		      id, ms, bio, dio);
	else 
	    STprintf( msg_buffer,
		      "debug: %XXXX -- CPU_MS: %d, BIO: %d, DIO: %d\n",
		      ms, bio, dio);
	duve_log (duve_cb, 0, msg_buffer);

    } /* endif debug */
}
/*{
** Name: cksecalarm - run verifydb tests to check system catalog iisecalarm.
**
** Description:
**      This routine opens a cursor to walk iisecalarm one tuple at a time.
**      It performs checks and associated repairs (mode permitting) on each 
**      tuple to assure the consistency of the iisecalarm system catalog.
**
** Inputs:
**      duve_cb                         verifydb control block
**	    duverel                          cached iirelation information
**
** Outputs:
**      duve_cb                         verifydb control block
**	    duverel                         cached iirelation information
**	        du_tbldrop			flag indicating to drop table
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be dropped from table iisecalarm
**	    tuples may be modified in iirelation
**
** History:
**      20-dec-93  (robf)
**          initial creation
[@history_template@]...
*/
DU_STATUS
cksecalarm(duve_cb)
DUVE_CB            *duve_cb;
{
    DB_TAB_ID		alm_id;
    DU_STATUS		base;

    EXEC SQL BEGIN DECLARE SECTION;
	exec sql include <duvealm.sh>;
	i4 cnt;
	u_i4	tid, tidx;
	u_i4	basetid, basetidx;
	i4	relstat;
	i4	id1, id2;
	i2	mode;
    EXEC SQL END DECLARE SECTION;
	bool	found;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    EXEC SQL DECLARE alm_cursor CURSOR FOR
	select * from iisecalarm;
    EXEC SQL open alm_cursor;

    /* loop for each tuple in iisecalarm */
    for (;;)  
    {
	/* get tuple from iisecalarm */
	EXEC SQL FETCH alm_cursor INTO :alarm_tbl;
	if (sqlca.sqlcode == 100) /* then no more tuples in iisecalarm */
	{
	    EXEC SQL CLOSE alm_cursor;
	    break;
	}

	alm_id.db_tab_base = alarm_tbl.obj_id1;
	alm_id.db_tab_index = alarm_tbl.obj_id2;
	tid = alarm_tbl.obj_id1;
	tidx = alarm_tbl.obj_id2;

	/* test 1 -- verify table receiving alarm exists */
	base = duve_findreltid ( &alm_id, duve_cb);
	if (base == DU_INVALID)
	{
	    /*
	    ** Not found, so check if its a database or installation
	    ** alarm.
	    */
	    found=FALSE;
	    if(alarm_tbl.alarm_flags&DBA_ALL_DBS)
		found=TRUE;
	    else if(*alarm_tbl.obj_type==DBOB_DATABASE)
	    {
		EXEC SQL SELECT COUNT (name) 
			 INTO :cnt
			 FROM iidatabase
			 WHERE name=:alarm_tbl.obj_name;
				
		if(cnt)
			found=TRUE;
	    }
	    if (!found)
	    {
		/* this tuple is not in iirelation/iidatabase
		** so drop from iisecalarm */

		if (duve_banner( DUVE_IISECALARM, 1, duve_cb) == DUVE_BAD) 
		    return ( (DU_STATUS) DUVE_BAD);
		duve_talk ( DUVE_MODESENS, duve_cb,
			    S_DU16D1_NO_BASE_FOR_ALARM, 6,
			    sizeof(tid), &tid, sizeof(tidx), &tidx,
			    0, alarm_tbl.alarm_name);
	        if ( duve_talk( DUVE_ASK, duve_cb, S_DU16D2_DROP_FROM_IISECALARM, 2,
			0, alarm_tbl.alarm_name)
			== DUVE_YES)
		{
		    mode = DB_ALM;
		    EXEC SQL DELETE FROM iiqrytext
			     WHERE  txtid1=:alarm_tbl.alarm_txtid1
			     AND    txtid2=:alarm_tbl.alarm_txtid2
			     AND    mode=:mode;

		    EXEC SQL delete from iisecalarm where current of
			    alm_cursor;
		    duve_talk(DUVE_MODESENS,duve_cb,
			      S_DU16D3_IISECALARM_DROP, 2,
			      0, alarm_tbl.alarm_name);
		}
	        continue;
	    }
	} /* endif base table not found */
	else 
	{
	    if ( base == DUVE_DROP)
		continue;	/* if table is marked for drop, stop now */
	}
	/* 
	**  test 2 - verify base table indicates alarms are defined on it
	*/
	if (*alarm_tbl.obj_type==DBOB_TABLE &&
	   (duve_cb->duverel->rel[base].du_stat2 & TCB_ALARM) == 0 )
	{
	    /* the relstat in iirelation needs to be modified */
            if (duve_banner( DUVE_IISECALARM, 2, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU16D4_NO_ALARM_RELSTAT, 4,
			0, duve_cb->duverel->rel[base].du_tab,
			0, duve_cb->duverel->rel[base].du_own);

	    if ( duve_talk( DUVE_ASK, duve_cb, S_DU16D5_SET_ALMTUPS, 0)
		    == DUVE_YES)
	    {
		basetid =  duve_cb->duverel->rel[base].du_id.db_tab_base;
		basetidx =  duve_cb->duverel->rel[base].du_id.db_tab_index;
		relstat = duve_cb->duverel->rel[base].du_stat2 | TCB_ALARM;
		duve_cb->duverel->rel[base].du_stat2 = relstat;
		EXEC SQL update iirelation set relstat2 = :relstat where
				    reltid = :basetid and reltidx = :basetidx;
		if (sqlca.sqlcode == 0)
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU16D6_ALMTUPS_SET, 0);
	    }  /* endif fix relstat */

	} /* endif relstat doesnt indicate alarms */

	/*********************************************************/
	/* test 3 verify query text used to define alarm exists */
	/*********************************************************/
	mode = DB_ALM;
	id1 = alarm_tbl.alarm_txtid1;
	id2 = alarm_tbl.alarm_txtid2;
	EXEC SQL select count(txtid1) into :cnt from iiqrytext where
		txtid1 = :id1 and txtid2 = :id2 and mode = :mode;
	if (cnt == 0) 
	{
	    /* mising query text that defined permit */
            if (duve_banner( DUVE_IISECALARM, 3, duve_cb) == DUVE_BAD) 
		return ( (DU_STATUS) DUVE_BAD);
	    
	    duve_talk ( DUVE_MODESENS, duve_cb, S_DU16D7_NO_ALARM_QRYTEXT, 6,
			sizeof(id1), &id1, sizeof(id2), &id2,
			0, alarm_tbl.alarm_name);
	}
	/*********************************************************/
	/* test 4 if alarm fires dbevent make sure the event exists
	/*********************************************************/
	if(alarm_tbl.alarm_flags&DBA_DBEVENT)
	{
	    EXEC SQL SELECT COUNT(*) 
			 INTO :cnt
			 FROM iievent
			 WHERE event_idbase=:alarm_tbl.event_id1
			 AND   event_idx=:alarm_tbl.event_id2;
	    if(!cnt)
	    {
	        /* the event is unknown */
                if (duve_banner( DUVE_IISECALARM, 4, duve_cb) == DUVE_BAD) 
			return ( (DU_STATUS) DUVE_BAD);
	         duve_talk ( DUVE_MODESENS, duve_cb, S_DU16D8_NO_ALARM_EVENT, 6,
			sizeof(alarm_tbl.event_id1), &alarm_tbl.event_id1,
			sizeof(alarm_tbl.event_id2), &alarm_tbl.event_id2,
			0, alarm_tbl.alarm_name);

	        if ( duve_talk( DUVE_ASK, duve_cb, S_DU16D2_DROP_FROM_IISECALARM, 2,
			0, alarm_tbl.alarm_name)
		    == DUVE_YES)
	        {
		    mode = DB_ALM;
		    EXEC SQL DELETE FROM iiqrytext
			     WHERE  txtid1=:alarm_tbl.alarm_txtid1
			     AND    txtid2=:alarm_tbl.alarm_txtid2
			     AND    mode=:mode;
		    EXEC SQL DELETE FROM iisecalarm 
			     WHERE CURRENT OF alm_cursor;
		    if (sqlca.sqlcode == 0)
		        duve_talk(DUVE_MODESENS, duve_cb, 
				S_DU16D3_IISECALARM_DROP, 2,
				0, alarm_tbl.alarm_name
				);
	        }  
	    }
	}
    }  /* end loop for each tuple in iisecalarm */

    return ( (DU_STATUS) DUVE_YES);
}
