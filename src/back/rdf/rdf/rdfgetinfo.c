/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<pc.h>
#include	<st.h>
#include	<ddb.h>
#include	<ulf.h>
#include        <cs.h>
#include	<lk.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<adfops.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<tr.h>
#include	<psfparse.h>
#include	<rdftree.h>
#include	<rdftrace.h>
#include	<cui.h>

/*
 NO_OPTIM=dgi_us5 nc4_us5 i64_aix
*/

/**
**
**  Name: RDFGETINFO.C - Request qrymod and statistics information of a table.
**
**  Description:
**	This file contains the routine for requesting integrity, protection,
**	tree, rules, iistatistics and iihistogram information of a table.
**
**	rdf_getinfo - Request qrymod and statistics information of a table.
**	rdu_rqrytuple - Read the integrity/protection/rules tuples.
**	rdu_statistics - Read iistatistics and iihistogram tuples and convert
**		   these tuples into a form which can be directly used
**		   by OPF.
**	rdu_histogram - Read iihistogram tuples and convert these tuples into 
**		    a form which can be directly used by OPF.
**	rdu_rtree - Read the tree tuples and convert them to query tree nodes.
**	rdu_event - Read an event tuple.
**	rdu_evsqprotect - Read a set of event or sequence protection tuples.
**
**  History:
**      10-apr-86 (ac)    
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	 8-apr-87 (puree)
**	    add snull to RDD_HISTO.  Also change rdu_histogram to copy snull
**	    from DB_1ZOPTSTAT.db_nulls.
**	 9-jun-87 (puree)
**	    fix bug in rdu_types_chk for the call to rdu_qmod_chk
**	14-aug-87 (puree)
**	    convert QEU_ATRAN to QEU_ETRAN
**      5-nov-87 (seputis)
**          rewritten to handle resource management problems
**	09-mar-89 (neil)
**	    modified to read rules too.
**	21-apr-89 (neil)
**	    Fixed bug which didn't allow process rdu_qget correct for qrymod.
**	    The GETNEXT path was assigning qrym_cnt upon EOF.
**      29-sep-89 (teg)
**          added support for outter joins.
**      27-oct-89 (neil)
**          Rules - linted field names for rules.
**          Alerters - added event extraction/rdu_event.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**      22-may-92 (teresa)
**          added new parameter to rdu_xlock call for SYBIL.
**	29-jun-92 (andre)
**	    defined rdu_i_objects() and rdu_i_privileges() to obtain  a list of
**	    independent objects and privileges, respectively, for a specified
**	    object (e.g. dbproc, view) + added code to rdf_getinfo() to invoke
**	    these new functions when appropriate
**	16-jul-92 (teresa)
**	    prototypes
**	1-dec-92 (teresa)
**	    modified name sizes in internal procedures from 24 to 32.
**	26-apr-93 (vijay)
**	    Include st.h.
**	24-mar-93 (smc)
**	    Cast function parameters to match prototype declarations
**          and made forward declarations.
**	17-may-93 (rblumer)
**	    in rdu_view,
**	    check return status from rdu_rtree before call to semaphore routine
**	    overwrites it; otherwise don't catch QEF errors during tree lookup.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	09-nov-93 (swm)
**	    Bug #58633
**	    Catered for pointer alignment in memory allocation in rdu_qopen().
**	14-jan-94 (teresa)
**	    fix bug 56728 (assure semaphores are NOT held across QEF/DMF calls
**	    during RDF_READTUPLES operation).
**      11-nov-94 (cohmi01)
**          Various fixes to ensure that sys_infoblk gets re-checksumed.
**	16-jul-95 (popri01)
**	    Add NO_OPTIM for nc4_us5 to eliminate failing table alarms.
**	    As demonstrated in c2secure sep tests c2log01 and c2log02,
**	    table alarms would not fire under any condition.
**	01-Nov-95 (allmi01)
**	    Add NO_OPTIM for dgi_us5 to eliminate failing table alarms.
**	    As demonstrated in c2secure sep tests c2log01 and c2log02,
**	    table alarms would not fire under any condition.
**	08-oct-96 (pchang)
**	    Ensure that semaphore is not held across QEF/DMF calls when reading
**	    iistatistics and iihistogram tuples (B56369).
**	9-jan-96 (inkdo01)
**	    Support for new "f4repf" array in RDD_HISTO for per-cell 
**	    repetition factors.
**      23-jan-97 (cohmi01)
**          Remove checksum calls from here, now done before releasing mutex
**          to ensure that checksum is recomputed during same mutex interval
**          that the infoblock was altered in. (Bug 80245).
**	09-Apr-97 (kitch01)
**	    Amend rdu_qmopen check logic to ensure we check and store
**	    rdr_2_types. This allows concurrent access to those objects
**	    defined with the types2_mask, eg. procedure parameters.
**	    (Bug 81138).
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**      16-mar-1998 (rigka01)
**	    Cross integrate change 434645 for bug 89105
**          Bug 89105 - clear up various access violations which occur when
**	    rdf memory is low.  In rdu_rtree(), add DB_FAILURE_MACRO(status)
**	    call after each rdu_malloc call for which it is missing
**	    (in CASE PST_CUR_VSN).  In rdf_readtuples(), add
**	    DB_FAILURE_MACRO(status) call after each rdu_rqrytuple call.
**	22-jul-98 (shust01)
**	    Changed DB_FAILURE_MACRO(s_status) to DB_FAILURE_MACRO(status).
**	    Was checking the wrong variable after rdu_rsemaphore() call.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**      10-May-2000 (wanya01)
**          Bug 101306 - Allow rdu_procedure() read more than 65k iiqrytext 
**          info from system catalog.
**      
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb(),
**	    rdu_gadfcb() functions.
**      16-apr-2001 (stial01)
**          rdr_tree() Added PST_MOP and PST_OPERAND cases
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	1-Jan-2004 (schka24)
**	    Teach RDF about table partitioning.
**	    Move this history to the front where it belongs.
**	29-Dec-2004 (schka24)
**	    Add cversion to ii_update_config param list.
**      01-mar-2005 (rigka01) bug 112613, problem INGSRV 2875
**          In rdu_usrbuf_read(), when processing RDR_CLOSE, do not
**          require rdfcb->rdf_rb.rdr_qtuple_count to be non-zero.
**	6-Jul-2006 (kschendel)
**	    Don't mix the dmf db id and the unique (infodb) db id.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
**      04-nov-2008 (stial01)
**          Define internal procedure name and owner without blanks
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	18-Aug-2009 (drivi01)
**	    Cleanup precedence warnings and other warnings in
**	    efforts to port to Visual Studio 2008.
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now an integer, fix update-config proc defn here.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/* prototype definitions */

/* rdu_master_infodump - Figure if there are any requests to dump
**                             RDF table information then do so.
*/
VOID 
rdu_master_infodump(	RDR_INFO        *info,
			RDF_GLOBAL	*global,
			RDF_CALLER	caller,
			RDF_SVAR	request);

/* rdr_tuple - read data from IITREE catalog tuples */
FUNC_EXTERN DB_STATUS rdr_tuple(  
	    RDF_GLOBAL	*global,
	    i4		length,
	    PTR		buffer,
	    RDF_TSTATE  *tstate);

/* rdu_rstatistics - Read the iistatistics and iihistogram tuples and convert
**		    these tuples into a form which can be directly used
**		    by OPF.
*/
static DB_STATUS rdu_rstatistics(RDF_GLOBAL *global);

/* rdu_statistics - read histogram and statistics tuples for an attribute */
static DB_STATUS rdu_statistics(RDF_GLOBAL *global);

/* rdr_tree - build the tree from IITREE catalog tuples */
static DB_STATUS rdr_tree(  RDF_GLOBAL	*global,
			    PST_QNODE	**root,
			    RDF_TSTATE	*tstate);

/* rdu_rtree - Read the tree tuples and convert them to query tree nodes. */
static DB_STATUS rdu_rtree( RDF_GLOBAL	    *global,
			    PST_PROCEDURE   **qrytree,
			    i4              treetype);

/* rdu_view - create descriptor for view tree */
static DB_STATUS rdu_view(RDF_GLOBAL *global);

/* rdu_qtname	- get object name for query tree hash table */
static VOID rdu_qtname( RDF_GLOBAL	*global,
			i4		catalog);

/* rdu_qtree	- lookup or create the query mod tree */
static DB_STATUS rdu_qtree( RDF_GLOBAL	    *global,
			    RDD_QRYMOD	    **rddqrymodp,
			    RDF_SVAR	    *builds,
			    RDF_SVAR	    *finds,
			    i4		    catalog,
			    RDF_TYPES	    rdftypes,
			    bool	    *refreshed);

/* rdu_intprotrul - first time read of integrity/protection/rule tuples */
static DB_STATUS rdu_intprotrul( RDF_GLOBAL        *global,
				RDF_TABLE	   table,
				i4                extcattupsize,
				RDD_QDATA          **tuplepp,
				bool		   *eof_found);

/* rdu_qmopen	- open query mod table and read tuples */
static DB_STATUS rdu_qmopen(
	RDF_GLOBAL	*global,
	RDF_TYPES       types_mask,
	RDF_TYPES       types2_mask,
	RDF_RTYPES	operation,
	RDF_SVAR	*builds,
	RDF_SVAR	*finds,
	RDF_SVAR	*multiple,
	RDF_TABLE	table,
	RDD_QDATA	**sys_tuplepp,
	RDF_STATE       *sys_state,
	RDD_QDATA	**usr_tuplepp,
	RDF_STATE       *usr_state,
	i4		extcattupsize);

/* rdu_rqrytuple - Read the integrity/protection/rule tuples. */
static DB_STATUS rdu_rqrytuple(
	RDF_GLOBAL	*global,
	RDF_TYPES       types_mask,
	RDF_TYPES       types2_mask,
	RDF_RTYPES	operation,
	RDF_SVAR        *requests,
	RDF_SVAR        *builds,
	RDF_SVAR        *finds,
	RDF_SVAR        *multiple,
	RDF_TABLE	table,
	RDD_QDATA	**sys_tuplepp,
	RDF_STATE       *sys_state,
	RDD_QDATA	**usr_tuplepp,
	RDF_STATE       *usr_state,
	i4		extcattupsize);
	
/* rdu_procedure - read iiqrytext info for the procedure catalog */
static DB_STATUS rdu_procedure(	RDF_GLOBAL         *global,
				bool		   *refreshed);

/* rdu_event	- Read an event from iievent catalog */
static DB_STATUS rdu_event(RDF_GLOBAL *global);

/* rdu_evsqprotect - Read permit tuples for a specific event or sequence. */
static DB_STATUS rdu_evsqprotect(RDF_GLOBAL *global);

/* Read tuples for a specified object into user supplied memory. */
static	    DB_STATUS	    rdu_usrbuf_read(RDF_GLOBAL *global);

/* rdu_sequence	- Read a sequence from iisequence catalog */
static DB_STATUS rdu_sequence(RDF_GLOBAL *global);

/* Read IIDBDEPENDS tuples for a specified object. */
static	DB_STATUS   rdu_i_objects(RDF_GLOBAL *global);

/* recognized RDF generated interal procedures */
static	i4	    rdu_special_proc( char *prcname, char *owner, bool upcase);

/* read iiintegrityidx tuples for a constraint */
static  DB_STATUS   rdu_cnstr_integ(RDF_GLOBAL    *global);

#define	    RDF_FAKE_OPEN    111    /* a non-zero value to stuff into a ptr
				    ** to "fake" an open catalog to PSF
				    */
/*******************************************************/
/*						       */
/*  The following constants/tables are used to define  */
/*  any internal procedures that do not get defined in */
/*  The DBMS catalogs				       */
/*						       */
/*******************************************************/

/* ordinal position of operation in special_proc table */
#define	    NOPROC	    0
#define	    LISTFILE	    1
#define	    DROPFILE	    2
#define	    CREATE_DB	    3
#define	    DESTROY_DB	    4
#define	    ALTER_DB	    5
#define	    ADD_LOC	    6
#define	    CHECK_TABLE	    7
#define	    PATCH_TABLE	    8
#define	    READ_CONFIG	    9
#define	    FINDDBS	    10
#define	    UPD_CONFIG	    11
#define	    DEL_DUMP	    12


#define	    TXTLEN	    50
/*length of special_proc*/
#define	    SPECIAL_CNT	    (sizeof(special_proc)/sizeof(special_proc[0]))

/* INTERNAL PROCEDURES */
static const struct 
{
    char    *proc_name;	    /* name of special mapped internal procedure,
			    ** which should be DB_DBP_MAXNAME characters long */
    char    *proc_own;	    /* owner of procedure, which should be DB_OWN_MAXNAME 
			    ** characters long */
    char    *proc_txt[10];    /* array of 50 char substrings of qry */
    i4	    txt_cnt;	    /* number of strings in array */
    i4	    txt_len;	    /* number of characters in proc_txt */
    i4	    proc_id;	    /* index into special_proc table for this proc */
} special_proc [] =
{
    /* dummy 1st entry */
    {
	"",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
	{"                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	 1, 0, NOPROC
    },

    /* ii_list_file */
    {
	"ii_list_file",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_list_file( directory =char(256",
	 ") not null not default, table =char(32) not null n",
	 "ot default, owner =char(32) not null not default, ",
	 "column=char(32) not null not default) as begin exe",
	 "cute internal; end                                ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	5, 218, LISTFILE
    },

    /* ii_drop_file */
    {	
	"ii_drop_file",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_drop_file( directory =char(256",
	 ") not null not default, file = char(256) not null ",
	 "not default) as begin execute internal; end       ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	3, 143, DROPFILE
    },

    /* ii_create_db */
    {	
	"ii_create_db",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure   ii_create_db( dbname = char(32)",
	 " not null not default, db_loc_name = char(32) not ",
	 "null not default, jnl_loc_name = char(32) not null",
	 " not default, ckp_loc_name = char(32) not null not",
	 " default, dmp_loc_name = char(32) not null not def",
	 "ault, srt_loc_name = char(32) not null not default",
	 ", db_access = integer not null not default, collat",
	 "ion = char(32) not null not default, need_dbdir_fl",
	 "g =integer not null not default, db_service = inte",
	 "ger not null not default) = {execute internal;}   "},
	10, 497, CREATE_DB
    },

    /* ii_destroy_db */
    {	
	"ii_destroy_db",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_destroy_db(dbname = char(32) n",
	 "ot null not default) as begin execute internal;end",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	2, 100, DESTROY_DB
    },

    /* ii_alter_db */
    {
	"ii_alter_db",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_alter_db(   dbname = char(32) ",
	 "not null not default, access_on = integer not null",
	 " not default, access_off = integer not null not de",
	 "fault, service_on = integer not null not default, ",
	 "service_off = integer not null not default, last_t",
	 "able_id = integer not null not default)	    ",
	 "as begin execute internal; end                    ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	7, 330, ALTER_DB
    },

    /* ii_add_location */
    {
	"ii_add_location",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_add_location(database_name=cha",
	 "r(32) not null not default,location_name=char(32) ",
	 "not null not default, access = integer not null no",
	 "t default, need_dbdir_flg = integer not null not d",
	 "efault) as begin  execute internal; end           ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	5, 239, ADD_LOC
    },

    /* ii_check_table */
    {
	"ii_check_table",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_check_table( table_id i4 not n",
	 "ull not default, index_id i4 not null not default ",
	 ") as begin  execute internal; end                 ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	3, 133, CHECK_TABLE
    },

    /* ii_patch_table */
    {
	"ii_patch_table",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_patch_table( table_id i4 not n",
	 "ull not default, mode i4 not null not default) as ",
	 "begin  execute internal; end		            ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	3, 128, PATCH_TABLE
    },

    /* ii_read_config_value */
    {
	"ii_read_config_value",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_read_config_value(database_nam",
	 "e = char(32) not null not default, location_area =",
	 " char(256) not null not default, readtype = intege",
	 "r not null not default) AS begin execute internal;",
	 "end                                               ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	5, 203, READ_CONFIG
    },

    /* ii_finddbs */
    {
	"ii_finddbs",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create prcedure iiQEF_finddbs (loc_name= char(DB_",
	 "MAXNAME) not null not default, loc_area = char(DB_",
	 "MAXNAME) not null not default, codemap_exists= int",
	 "eger not null not default, verbose_flag = integer ",
	 "not null not default, priv_50dbs = integer not nul",
	 "l not default) as begin   execute internal; end   ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	6, 297, FINDDBS
    },

    /* ii_update_config */
    {
	"ii_update_config",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_update_config( database_name =",
	 " char(32) not null not default, location_area =   ",
	 "char(256) not null not default, update_map =      ",
	 "integer not null not default, status = integer,   ",
	 "cmptlvl = integer, cmptminor = integer, access =  ",
	 "integer, service = integer, cversion = integer) as",
	 " begin execute internal; end			    ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	7, 328, UPD_CONFIG
    },

    /* ii_del_dmp_config */
    {
	"ii_del_dmp_config",
	DB_INGRES_NAME,
        /*12345678901234567890123456789012345678901234567890  - 50 char/line */
        {"create procedure ii_del_dmp_config(  database_name",
	 " = char(32) not null not default, location_area = ",
	 "char(256) not null not default) AS begin execute  ",
	 "internal; end 			            ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  ",
	 "                                                  "},
	4, 163, DEL_DUMP
    }
};

/*
** The rdu_qopen utility needs to set up q QEU_CB describing the catalog
** to be read.  Some of the setup is catalog specific.  Rather than a
** very large and tedious switch statement, define an array that holds
** the setup info needed.
**
** *Note* This array is order sensitive!  It has to be in the same order
** as the RDF_TABLE #defines.  There's a "self index" member of each
** array entry just to verify that things are OK.
**
*/
#define QOPEN_MAXKEYS	3	/* Maximum number of key columns needed */

static const struct _QOPEN_INFO {
    DB_TAB_ID	tab_id;			/* The catalog's table ID numbers */
    i2		self_index;		/* RD_xxxx of this entry for checking */
    i2		nkeys;			/* Number of keys to position on */
    i2		attr_number[QOPEN_MAXKEYS];  /* Key attribute number */
    bool	key12_is_tabid;		/* TRUE if keys 1 & 2 are
					** rdf_rb->rdr_tabid */
    bool	key3_is_data;		/* TRUE if key 3 is passed-in "data" */
} qopen_info[] = {
	/* RD_IIDBDEPENDS */
	{ {DM_B_DEPENDS_TAB_ID, DM_I_DEPENDS_TAB_ID},
	  RD_IIDBDEPENDS,
	  0,			/* No keys, always accessed by tid */
	  {0, 0, 0},
	  FALSE, FALSE
	},
	/* RD_IIDBXDEPENDS */
	{ {DM_B_XDEPENDS_TAB_ID, DM_I_XDEPENDS_TAB_ID},
	  RD_IIDBXDEPENDS,
	  3,
	  {DM_1_XDEPENDS_KEY, DM_2_XDEPENDS_KEY, DM_3_XDEPENDS_KEY},
	  TRUE, TRUE
	},
	/* RD_IIEVENT */
	{ {DM_B_EVENT_TAB_ID, DM_I_EVENT_TAB_ID},
	  RD_IIEVENT,
	  2,			/* Sometimes used with 0 keys by tid too */
	  {DM_1_EVENT_KEY, DM_2_EVENT_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IIHISTOGRAM */
	{ {DM_B_HISTOGRAM_TAB_ID, DM_I_HISTOGRAM_TAB_ID},
	  RD_IIHISTOGRAM,
	  2,
	  {DM_1_HISTOGRAM_KEY, DM_2_HISTOGRAM_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIINTEGRITY */
	{ {DM_B_INTEGRITY_TAB_ID, DM_I_INTEGRITY_TAB_ID},
	  RD_IIINTEGRITY,
	  2,
	  {DM_1_INTEGRITY_KEY, DM_2_INTEGRITY_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIPRIV */
	{ {DM_B_PRIV_TAB_ID, DM_I_PRIV_TAB_ID},
	  RD_IIPRIV,
	  3,
	  {DM_1_PRIV_KEY, DM_2_PRIV_KEY, DM_3_PRIV_KEY},
	  TRUE, TRUE
	},
	/* RD_IIPROCEDURE */
	{ {DM_B_DBP_TAB_ID, DM_I_DBP_TAB_ID},
	  RD_IIPROCEDURE,
	  2,			/* Sometimes used with 0 keys by tid too */
	  {DM_1_DBP_KEY, DM_2_DBP_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IIPROTECT */
	{ {DM_B_PROTECT_TAB_ID, DM_I_PROTECT_TAB_ID},
	  RD_IIPROTECT,
	  2,
	  {DM_1_PROTECT_KEY, DM_2_PROTECT_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIQRYTEXT */
	{ {DM_B_QRYTEXT_TAB_ID, DM_I_QRYTEXT_TAB_ID},
	  RD_IIQRYTEXT,
	  2,
	  {DM_1_QRYTEXT_KEY, DM_2_QRYTEXT_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IIRULE */
	{ {DM_B_RULE_TAB_ID, DM_I_RULE_TAB_ID},
	  RD_IIRULE,
	  2,
	  {DM_1_RULE_KEY, DM_2_RULE_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IISTATISTICS */
	{ {DM_B_STATISTICS_TAB_ID, DM_I_STATISTICS_TAB_ID},
	  RD_IISTATISTICS,
	  2,
	  {DM_1_STATISTICS_KEY, DM_2_STATISTICS_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IISYNONYM */
	{ {DM_B_SYNONYM_TAB_ID, DM_I_SYNONYM_TAB_ID},
	  RD_IISYNONYM,
	  2,
	  {DM_1_SYNONYM_KEY, DM_2_SYNONYM_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IITREE */
	{ {DM_B_TREE_TAB_ID, DM_I_TREE_TAB_ID},
	  RD_IITREE,
	  3,
	  {DM_1_TREE_KEY, DM_2_TREE_KEY, DM_3_TREE_KEY},
	  TRUE, TRUE
	},
	/* RD_IIXEVENT */
	{ {DM_B_XEVENT_TAB_ID, DM_I_XEVENT_TAB_ID},
	  RD_IIXEVENT,
	  2,
	  {DM_1_XEVENT_KEY, DM_2_XEVENT_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIXPROCEDURE */
	{ {DM_B_XDBP_TAB_ID, DM_I_XDBP_TAB_ID},
	  RD_IIXPROCEDURE,
	  2,
	  {DM_1_XDBP_KEY, DM_2_XDBP_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIKEY */
	{ {DM_B_IIKEY_TAB_ID, DM_I_IIKEY_TAB_ID},
	  RD_IIKEY,
	  2,
	  {DM_1_IIKEY_KEY, DM_2_IIKEY_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIPROCEDURE_PARAMETER */
	{ {DM_B_IIPROC_PARAM_TAB_ID, DM_I_IIPROC_PARAM_TAB_ID},
	  RD_IIPROCEDURE_PARAMETER,
	  2,
	  {DM_1_IIPROC_PARAM_KEY, DM_2_IIPROC_PARAM_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIDEFAULT */
	{ {DM_B_IIDEFAULT_TAB_ID, DM_I_IIDEFAULT_TAB_ID},
	  RD_IIDEFAULT,
	  2,
	  {DM_1_IIDEFAULT_KEY, DM_2_IIDEFAULT_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IISCHEMA */
	{ {DM_B_IISCHEMA_TAB_ID, DM_I_IISCHEMA_TAB_ID},
	  RD_IISCHEMA,
	  1,
	  {DM_1_IISCHEMA_KEY, 0, 0},
	  FALSE, FALSE
	},
	/* RD_IIINTEGRITYIDX */
	{ {DM_B_INTEGRITYIDX_TAB_ID, DM_I_INTEGRITYIDX_TAB_ID},
	  RD_IIINTEGRITYIDX,
	  3,
	  {DM_1_INTEGRITYIDX_KEY, DM_2_INTEGRITYIDX_KEY, DM_3_INTEGRITYIDX_KEY},
	  TRUE, FALSE
	},
	/* RD_IISECALARM */
	{ {DM_B_IISECALARM_TAB_ID, DM_I_IISECALARM_TAB_ID},
	  RD_IISECALARM,
	  3,
	  {DM_1_IISECALARM_KEY, DM_2_IISECALARM_KEY, DM_3_IISECALARM_KEY},
	  FALSE, FALSE
	},
	/* RD_IISEQUENCE */
	{ {DM_B_SEQ_TAB_ID, DM_I_SEQ_TAB_ID},
	  RD_IISEQUENCE,
	  2,
	  {DM_1_SEQ_KEY, DM_2_SEQ_KEY, 0},
	  FALSE, FALSE
	},
	/* RD_IIDISTSCHEME */
	{ {DM_B_DSCHEME_TAB_ID, DM_I_DSCHEME_TAB_ID},
	  RD_IIDISTSCHEME,
	  2,			/* All rows for given master table ID */
	  {DM_1_DSCHEME_KEY, DM_2_DSCHEME_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIDISTCOL */
	{ {DM_B_DISTCOL_TAB_ID, DM_I_DISTCOL_TAB_ID},
	  RD_IIDISTCOL,
	  2,
	  {DM_1_DISTCOL_KEY, DM_2_DISTCOL_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIDISTVAL */
	{ {DM_B_DISTVAL_TAB_ID, DM_I_DISTVAL_TAB_ID},
	  RD_IIDISTVAL,
	  2,
	  {DM_1_DISTVAL_KEY, DM_2_DISTVAL_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IIPARTNAME */
	{ {DM_B_PNAME_TAB_ID, DM_I_PNAME_TAB_ID},
	  RD_IIPARTNAME,
	  2,
	  {DM_1_PNAME_KEY, DM_2_PNAME_KEY, 0},
	  TRUE, FALSE
	},
	/* RD_IICOLCOMPARE */
	{ {DM_B_COLCOMPARE_TAB_ID, DM_I_COLCOMPARE_TAB_ID},
	  RD_IICOLCOMPARE,
	  2,
	  {DM_1_COLCOMPARE_KEY, DM_2_COLCOMPARE_KEY, 0},
	  TRUE, FALSE
	},
};

/* Definitions for pre-r3.0.2 (version 9) parse trees.
**
** This definition is here, rather than in psfparse.h, because we're
** the only place that it's needed.  (If this turns out to be a lie,
** move the definition(s) to psfparse.h, or rdftree.h.)
**
** 3.0.2 added a db_collID to the DB_DATA_VALUE.  We need to declare
** the old DB_DATA_VALUE and an old-style PST_SYMBOL, so that we can
** read v9 parse trees without forcing trees to be rebuilt.
*/

struct _OLD_DB_DATA_VALUE {
    PTR db_data;
    i4 db_length;
    DB_DT_ID db_datatype;
    i2 db_prec;
};

typedef struct {
    i4 pst_type;
    struct _OLD_DB_DATA_VALUE pst_dataval;
    i4 pst_len;
    PST_SYMVALUE pst_value;
} PST_SYMBOL_V9;


/*{
** Name: rdd_streequery - send inquiry query to QEF for iidd_ddb_tree catalog.
**
** Description:
**      This routine generates query to inquire iidd_ddb_tree tuples.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	09-jul-90 (teresa)
**	    changed "select *" query to name each expected field instead.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_streequery(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    RQB_BIND		*bind_p = global->rdf_ddrequests.rdd_bindp;
    				    /* 8 columns */
    RDC_TREE		*tree = global->rdf_ddrequests.rdd_ttuple;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];

    /* 1. consistency check */
  
    if (global->rdf_ddrequests.rdd_types_mask != RDD_VTUPLE)
    {
  	rdu_ierror(global, E_DB_ERROR, E_RD0272_UNKNOWN_DDREQUESTS);
  	return(E_DB_ERROR);
    } 
  
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* 2. set up query 
    **
    **	select * from iidd_ddb_tree where 
    **	             treetabbase = obj_base and treetabidx = obj_idx
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p =
	(char *)STprintf(qrytxt, "%s %d and treetabidx = %d order by treeseq;",
	    "select treetabbase, treetabidx, treeid1, treeid2,treeseq,treemode,\
	     treevers, treetree from iidd_ddb_tree where treetabbase = ",
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_base,
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_index);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
    	return(status);
  
    /* init binding information */
  
    /* treetabbase */
    bind_p[0].rqb_addr = (PTR)&tree->treetabbase;
    bind_p[0].rqb_length = sizeof(tree->treetabbase);
    bind_p[0].rqb_dt_id = DB_INT_TYPE;
    /* treetabidx */
    bind_p[1].rqb_addr = (PTR)&tree->treetabidx;
    bind_p[1].rqb_length = sizeof(tree->treetabidx);
    bind_p[1].rqb_dt_id = DB_INT_TYPE;
    /* treeid1 */
    bind_p[2].rqb_addr = (PTR)&tree->treeid1;
    bind_p[2].rqb_length = sizeof(tree->treeid1);
    bind_p[2].rqb_dt_id = DB_INT_TYPE;
    /* treeid2 */
    bind_p[3].rqb_addr = (PTR)&tree->treeid2;
    bind_p[3].rqb_length = sizeof(tree->treeid2);
    bind_p[3].rqb_dt_id = DB_INT_TYPE;
    /* treeseq */
    bind_p[4].rqb_addr = (PTR)&tree->treeseq;
    bind_p[4].rqb_length = sizeof(tree->treeseq);
    bind_p[4].rqb_dt_id = DB_INT_TYPE;
    /* treemode */
    bind_p[5].rqb_addr = (PTR)&tree->treemode;
    bind_p[5].rqb_length = sizeof(tree->treemode);
    bind_p[5].rqb_dt_id = DB_INT_TYPE;
    /* treevers */
    bind_p[6].rqb_addr = (PTR)&tree->treevers;
    bind_p[6].rqb_length = sizeof(tree->treevers);
    bind_p[6].rqb_dt_id = DB_INT_TYPE;
    /* treetree */
    bind_p[7].rqb_addr = (PTR)tree->treetree;
    bind_p[7].rqb_length = sizeof(tree->treetree);
    bind_p[7].rqb_dt_id = DB_DBV_TYPE;
    
    return(status);
}

/*{
** Name: rdd_gtreetuple - retrieve tree tuples from iidd_ddb_tree.
**
** Description:
**      This routine calls QEF to fetch next set of tuples
**	from iidd_ddb_tree in CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_gtreetuple(	RDF_GLOBAL         *global,
		bool		   *eof_found)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    RQB_BIND            *bind_p = global->rdf_ddrequests.rdd_bindp;
    				    /* 8 columns */
    RDC_TREE		*tree = global->rdf_ddrequests.rdd_ttuple;
    DB_IITREE		*iitree;
    QEF_DATA		*qdata = global->rdf_qeucb.qeu_output;
    i4			tupleno = 0;

    for (;global->rdf_qeucb.qeu_count; qdata = qdata->dt_next, global->rdf_qeucb.qeu_count--)
    {
	
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_08_COL;

	status = rdd_fetch(global);
	if (status)
	    return(status);

        if (ddr_p->qer_d5_out_info.qed_o3_output_b)
        {  
	    iitree = (DB_IITREE *)qdata->dt_data;
  	    iitree->dbt_tabid.db_tab_base = tree->treetabbase;
  	    iitree->dbt_tabid.db_tab_index = tree->treetabidx;
  	    iitree->dbt_trid.db_tre_high_time = tree->treeid1;
  	    iitree->dbt_trid.db_tre_low_time = tree->treeid2;
  	    iitree->dbt_trseq = tree->treeseq;
  	    iitree->dbt_trmode = tree->treemode;
  	    iitree->dbt_version = tree->treevers;
  
	    /* clear the text field */

	    MEfill(DB_TREELEN, ' ', iitree->dbt_tree);

  	    /* the length of the text is in the first 2 bytes of treetree */

	    MEcopy((PTR)tree->treetree, sizeof(iitree->dbt_l_tree),
		    (PTR)&iitree->dbt_l_tree);

	    MEcopy((PTR)&tree->treetree[2], iitree->dbt_l_tree,
		    (PTR)iitree->dbt_tree);
	    tupleno++;
	    
	}
	else
	{
	    *eof_found = TRUE;
	    break;
	}
    };

    global->rdf_qeucb.qeu_count = tupleno;
    return(status);
}

/*{
** Name: rdu_qopen	- routine to open a QEF file
**
** Description:
**      Routine will initialize control block and make a QEU call necessary
**      to process the access to the extended system catalogs
**
** Inputs:
**      global                          ptr to RDF state variable
**      table				table identifier (type RDF_TABLE)
**	data				Any data to use for keying:
**					  RD_IIEVENT,RD_IIPROCEDURE - nonzero
**					    data[0] -> get by tid.
**					  RD_IITREE -> tree_type
**					  RD_IIQRYTXT -> query id
**					  RD_IIPRIV -> descriptor number
**					  RD_IIDBXDEPENDS -> type (view or 
**							     procedure)
**					  RD_IIINTEGRITYIDX -> schema id
**      extcattupsize		        tuple size for the extended system
**                                      catalog being looked up
**      datap                           ptr to block of memory to be used
**                                      as tuple buffer area
**      datasize                        size of block of memory pointed at
**                                      by datap, if 0 then datap can be
**                                      used directly to call QEF
**
** If datasize is zero, datap has to point to a properly set up RDD_QDATA
** structure, with the chain of initialized QEF_DATA entries filled in
** as well as qrym_alloc (the number of such entries).
** If datasize is nonzero, datap points to an uninitialized memory area,
** of size datasize.  We'll init the area into a QEF_DATA chain and a
** bunch of row buffers.  In order for the datap buffer to be as large as
** the caller expects it to be, a buffer like this is typically used:
**	struct {
**	    QEF_DATA qefdata[n];
**	    DB_IIblahblah rows[n];
**	} and the address and sizeof this struct passed to rdu_qopen.
** However if you use this convention DO NOT expect the rows to be where
** you declared "rows"!  The struct def is just a sizing convention;
** rdu_qopen will lay out the buffer to suit itself.
**
** Outputs:
**      global->rdf_qeucb               initialized and opened
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-oct-87 (seputis)
**          initial creation
**	09-mar-89 (neil)
**	    modified to check attributes 5 & 6 if iirule table.
**      18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**      19-apr-90 (teg)
**          correctly use new RDD_QDATA.qrym_alloc field to fix bug 8011.
**          (This bug was caused because RDD_QDATA.qrym_cnt was double-loaded.
**          It was used to indicate the amount of memory allocated to return
**          tuples in (which matched the #tuples requested on 1st call), and 
**          was also used to indicate the number of tuples that QEF found.)
**	01-apr-92 (teresa)
**	    Initialize timeout.
**	29-jun-92 (andre)
**	    add special handling for IIPRIV and IIXDBDEPENDS which both use keys
**	    consisting of 3 attributes.
**	29-jun-92 (andre)
**	    added special handling for IIDBDEPENDS which will be accessed by
**	    TID, so no keys will be passed
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    rewrote to know keying for target table
**	15-Oct-92 (teresa)
**	    put synonym name and synonym owner into GLOBAL, since these fields
**	    must be visible to the calling routine.
**	09-nov-92 (rganski)
**	    Added type casts for "assignment type mismatch" warnings.
**	28-jan-93 (teresa)
**	    Added support for iikey catalog.
**	05-mar-93 (teresa)
**	    added support for iischema and iiintegrityidx catalogs
**	14-apr-93 (teresa)
**	    change the ordering of the iiintegrityidx keys to match the catalog.
**	09-nov-93 (swm)
**	    Bug #58633
**	    Catered for pointer alignment in memory allocation in rdu_qopen().
**	26-nov-93 (robf)
**          Add support for security alarms.
**	17-nov-95 (markg)
**	    Change definition of almval to be static. Previously this
**	    variable was being declared on the stack, and was no longer
**	    valid by the time it reached DMF. (b69676)
**	14-Dec_1995 (murf)
**		Added change by Mark Greaves (markg).
**		Change definition of almval to be static. Previosly this
**          	variable was being declared on the stack, and was no longer
**          	valid by the time it reached DMF. (b69676).
**	18-mar-02 (inkdo01)
**	    Add iisequence for sequence support.
**	1-Jan-2004 (schka24)
**	    Add partitioning catalog support; try to table-drive a bit more.
**	22-Mar-2005 (thaju02)
**	    iidbdepends normally accessed by tid. Add special case to
**	    access iidbdepends by partial key (tab base, tab_index, 
**	    itype = DB_CONS). (B114000)
[@history_template@]...
*/
DB_STATUS
rdu_qopen(  RDF_GLOBAL	    *global,
	    RDF_TABLE	    table,
	    char	    *data,
	    i4		    extcattupsize,
	    PTR		    datap,
	    i4		    datasize)
{
    DB_STATUS	    status;
    RDR_RB          *rdf_rb = &global->rdfcb->rdf_rb;
    char	    **key1;		/* Pointers to key attr_value */
    char	    **key2;
    char	    **key3;
    DMR_ATTR_ENTRY  *key_ptr;		/* Pointer to global->rdf_keys */
    i4		    i;
    i4		    nkeys;		/* Number of keys to use this time */
    const struct _QOPEN_INFO *info_ptr;
    bool            dbdep_by_key = FALSE;
    i4              db_cons = DB_CONS;

    /* This has to be static so that the pointer to it is still valid after
    ** we return.  The value is constant so there's no MT issues.
    */
    static const char    almval_table = DBOB_TABLE;

    if (global->rdf_resources & RDF_RQEU)
    {	/* consistency check to see if some file is already opened */
	status = E_DB_SEVERE;
	rdu_ierror(global, status, E_RD0102_SYS_CATALOG);
	return(status);
    }
    if (table > RD_MAX_TABLE)
    {
	status = E_DB_SEVERE;			/* FIXME bogus message */
	rdu_ierror(global, status, E_RD0119_QOPEN_INFO);
	return(status);
    }

    /*
    ** normally iidbdepends is accessed by tid only (nkeys = 0)
    ** but this is a special case in which we need to access
    ** by partial key
    ** should only be called indirectly from psl_ref_cons_iobj()
    */
    if ( (table == RD_IIDBDEPENDS) &&
              (rdf_rb->rdr_2types_mask & RDR2_DBDEPENDS) )
	dbdep_by_key = TRUE;

    global->rdf_qeucb.qeu_key = &global->rdf_keyptrs[0];
    global->rdf_keyptrs[0] = &global->rdf_keys[0];
    global->rdf_keyptrs[1] = &global->rdf_keys[1];
    global->rdf_keyptrs[2] = &global->rdf_keys[2];

    info_ptr = &qopen_info[table-1];		/* Point to catalog's setup */
    if (info_ptr->self_index != table)
    {
	status = E_DB_SEVERE;			/* qopen_info is wrong! */
	rdu_ierror(global, status, E_RD0119_QOPEN_INFO);
	return(status);
    }

    /* initialize the generic portion of QEU control block */
    global->rdf_qeucb.qeu_db_id = rdf_rb->rdr_db_id;
    global->rdf_qeucb.qeu_d_id = rdf_rb->rdr_session_id;
    /* The QEU_SHOW_STAT flag simply says that the access is internal
    ** and need not be security-label-checked or audited.
    */
    global->rdf_qeucb.qeu_flag = QEF_READONLY | QEU_SHOW_STAT;
    global->rdf_qeucb.qeu_eflag = QEF_EXTERNAL;
    global->rdf_qeucb.qeu_lk_mode = DMT_IS;
    global->rdf_qeucb.qeu_access_mode = DMT_A_READ;
    global->rdf_qeucb.qeu_acc_id = NULL;
    global->rdf_qeucb.qeu_tup_length = extcattupsize;
    global->rdf_qeucb.qeu_qual = 0;
    global->rdf_qeucb.qeu_qarg = 0;
    STRUCT_ASSIGN_MACRO(info_ptr->tab_id, global->rdf_qeucb.qeu_tab_id);
    nkeys = info_ptr->nkeys;
    /* Check for special cases of access-by-tid */
    if ( (table == RD_IIEVENT || table == RD_IIPROCEDURE) && data[0] != '\0')
	nkeys = 0;
    else if (dbdep_by_key)
	nkeys = 3;   
    global->rdf_qeucb.qeu_klen = nkeys;
    global->rdf_qeucb.qeu_getnext = QEU_REPO;   /* in most cases, reposition */
    key_ptr = &global->rdf_keys[0];
    for (i = 0; i < nkeys; ++i)
    {
	key_ptr->attr_number = (!dbdep_by_key ? info_ptr->attr_number[i] :
				(DM_1_DEPENDS_KEY + i));
	key_ptr->attr_operator = DMR_OP_EQ;
	++key_ptr;
    }
    if (nkeys == 0)
    {
	/* No keys, must be access by tid */
	global->rdf_qeucb.qeu_key = (DMR_ATTR_ENTRY **) NULL;
	global->rdf_qeucb.qeu_getnext = QEU_NOREPO;
    }
    else
    {
	/* Save some typing... */
	key1 = &global->rdf_keys[0].attr_value;
	key2 = &global->rdf_keys[1].attr_value;
	key3 = &global->rdf_keys[2].attr_value;
	/* Very common case is key by table ID numbers */
	if (info_ptr->key12_is_tabid || dbdep_by_key)
	{
	    *key1 = (char *) (&rdf_rb->rdr_tabid.db_tab_base);
	    *key2 = (char *) (&rdf_rb->rdr_tabid.db_tab_index);
	}
	if (info_ptr->key3_is_data)
	    *key3 = data;
	else if (dbdep_by_key)
	    *key3 = (char *)&db_cons;
	    
	/* Other keys have to be handled manually */
	switch (table)
	{
	case RD_IIDEFAULT:
	{
	    /* "data" is a pointer to an attribute entry */
	    DMT_ATT_ENTRY *attptr = (DMT_ATT_ENTRY *) data;

	    *key1 = (char *) (&attptr->att_defaultID.db_tab_base);
	    *key2 = (char *) (&attptr->att_defaultID.db_tab_index);
	    break;
	}
	case RD_IIEVENT:
	{
	    /* Can't be the by-tid case, key is event name & owner */
	    *key1 = (char *) &rdf_rb->rdr_name.rdr_evname;
	    *key2 = (char *) &rdf_rb->rdr_owner;
	    break;
	}
	case RD_IIINTEGRITYIDX:
	{
	    /* "data" points to a DB_SCHEMA_ID */
	    DB_SCHEMA_ID *schema_id = (DB_SCHEMA_ID *) data;

	    *key1 = (char *) (&schema_id->db_tab_base);
	    *key2 = (char *) (&schema_id->db_tab_index);
	    *key3 = (char *) (&rdf_rb->rdr_name.rdr_cnstrname);
	    break;
	}
	case RD_IIPROCEDURE:
	    /* Can't be the by-tid case, key is proc name & owner */
	    *key1 = (char *) &rdf_rb->rdr_name.rdr_prcname;
	    *key2 = (char *) &rdf_rb->rdr_owner;
	    break;
	case RD_IISECALARM:
	    *key1 = (char *) &almval_table;
	    *key2 = (char *) (&rdf_rb->rdr_tabid.db_tab_base);
	    *key3 = (char *) (&rdf_rb->rdr_tabid.db_tab_index);
	    break;
	case RD_IIQRYTEXT:
	{
	    /* "data" points to a DB_QRY_ID */
	    DB_QRY_ID *qid = (DB_QRY_ID *) data;

	    *key1 = (char *) (&qid->db_qry_high_time);
	    *key2 = (char *) (&qid->db_qry_low_time);
	    break;
	}
	case RD_IISCHEMA:
	    *key1 = (char *) &rdf_rb->rdr_owner;
	    break;
	case RD_IISEQUENCE:
	    /* Key is seq name and owner */
	    *key1 = (char *) (&rdf_rb->rdr_name.rdr_seqname);
	    *key2 = (char *) (&rdf_rb->rdr_owner);
	    break;
	case RD_IISYNONYM:
	    /* build key for iisynonym tuple.  NOTE: the synonym_name and
	    **  synonym_owner fields may be longer than DB_MAXNAME, so blank
	    **  pad and then fill in the PSF supplied names, which are
	    **  guaranteed to be DB_TAB_MAXNAME, DB_OWN_MAXNAME.
	    ** (I think this comment is obsolete - synonym names are now
	    ** the same as a db_name - schka24)
	    */
	    MEfill( sizeof(DB_SYNNAME), ' ', (PTR) &global->rd_syn_name);
	    MEfill( sizeof(DB_SYNOWN), ' ', (PTR) &global->rd_syn_own);
	    MEcopy( (PTR) &rdf_rb->rdr_name.rdr_tabname, DB_TAB_MAXNAME,
		    (PTR)&global->rd_syn_name);

	    MEcopy( (PTR) &rdf_rb->rdr_owner,
		    DB_OWN_MAXNAME, (PTR) &global->rd_syn_own);
	    *key1 = (char *) &global->rd_syn_name;
	    *key2 = (char *) &global->rd_syn_own;
	    break;
	/* Other cases already set up... */
	} /* end switch statement */
    }

    if ( (!datasize) && (datap) && (((RDD_QDATA *)datap)->qrym_alloc) )
    {	/* datasize is zero, which means datap can be used directly
        ** to call QEF */
	RDD_QDATA	*temp_ptr;
	temp_ptr = (RDD_QDATA *)datap;
	global->rdf_qeucb.qeu_count = temp_ptr->qrym_alloc;
	global->rdf_qeucb.qeu_output = temp_ptr->qrym_data; 
	temp_ptr->qrym_cnt = temp_ptr->qrym_alloc;
    }
    else
    {   /* initialize structures used to obtain tuples from QEF */

	i4		    tupleno;
	QEF_DATA	    *datablk;
	QEF_DATA	    *prev_datablk;
	i4		    maxcount;
	i4		    size_align;

	if (rdf_rb->rdr_qtuple_count > 0)
	    maxcount = rdf_rb->rdr_qtuple_count;
	else
	    maxcount = datasize;	    /* use as much of buffer as
                                            ** as possible if no limit is
                                            ** required */
	prev_datablk = NULL;
	size_align = sizeof(QEF_DATA) + extcattupsize;
	size_align = DB_ALIGN_MACRO(size_align);
	for (tupleno = 0; tupleno < maxcount; tupleno++)
	{
	    if ( size_align > datasize)
	    {	/* check if enough data remains in buffer */
		if (!tupleno)
		{   /* not enough for one tuple */
		    status = E_DB_SEVERE;
		    rdu_ierror(global, status, E_RD0104_BUFFER);
		    return(status);
		}
		break;
	    }
	    datablk = (QEF_DATA *)datap;
	    datap = (PTR)&datablk[1];	/* allocate a QEF_DATA element */
	    datablk->dt_data = (PTR)datap;
	    datablk->dt_size = extcattupsize;
	    datap = (char *)datablk + size_align;
	    datablk->dt_next = prev_datablk;
	    prev_datablk = datablk;
	    datasize -= size_align;
	}
	global->rdf_qeucb.qeu_count = tupleno; 
	global->rdf_qeucb.qeu_output = prev_datablk; 
    }

    if (rdf_rb->rdr_r1_distrib & DB_3_DDB_SESS)
    {
	global->rdf_resources |= RDF_RQEU;
	status = E_DB_OK;
    }
    else
    {

	if (rdf_rb->rdr_2types_mask & RDR2_TIMEOUT)
	{	/* add the timeout information to the QEF call */
	    global->rdf_qeucb.qeu_timeout = RDF_TIMEOUT; /* set default
				    ** timeout value for system catalog
				    ** access */
	    global->rdf_qeucb.qeu_mask = QEU_TIMEOUT;
	}
	else
	    global->rdf_qeucb.qeu_mask = 0;

	status = qef_call(QEU_OPEN, ( PTR ) &global->rdf_qeucb);
	if (DB_FAILURE_MACRO(status))
	    rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0077_QEU_OPEN,0);
	else
	{
	    rdf_rb->rdr_rec_access_id = global->rdf_qeucb.qeu_acc_id;
	    global->rdf_resources |= RDF_RQEU;
						/* save the record access id
						** for future calls */
	}
    }
    return(status);
}

/*{
** Name: rdu_qget	- get the next set of tuples from extended catalog
**
** Description:
**      This routine will fetch the next set of tuple from the extended
**      system catalog.
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-nov-87 (seputis)
**          initial creation
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	30-jan-91 (teresa)
**	    add more info to consistency check e_rd0103_not_opened to gather
**	    information which may let us fix unduplicatable bugs 35513 and 30694
**	01-apr-92 (teresa)
**	    Initialize to no-timeout
**	16-jul-92 (teresa)
**	    prototypes
**	28-jan-93 (teresa)
**	    move RDR_QTUPLE_ONLY from RDF_RB to RDF_RTYPES.RDF_QTUPLE_ONLY.
[@history_template@]...
*/
DB_STATUS
rdu_qget(   RDF_GLOBAL	*global,
	    bool	*eof_found)
{
    DB_STATUS	    status;

    if (!(global->rdf_resources & RDF_RQEU))
    {	/* check to see if the file has been opened previously */
	RDF_CB	    *rdfcb;
	rdfcb = global->rdfcb;
	if (global->rdf_resources & RDF_QTUPLE_ONLY)
	{   /* this is a case in which a file remains open over successive
            ** RDF calls, so the control block needs to be initialized */
	    /* - initialize the QEU control block */
	    global->rdf_qeucb.qeu_acc_id 
		= rdfcb->rdf_rb.rdr_rec_access_id; /* this should be
					    ** the QEF id obtained when the
                                            ** the query mod file was opened
                                            ** for scanning */
	    global->rdf_qeucb.qeu_db_id = rdfcb->rdf_rb.rdr_db_id;
	    global->rdf_qeucb.qeu_d_id = rdfcb->rdf_rb.rdr_session_id;
	    global->rdf_qeucb.qeu_flag = QEF_READONLY;
	    global->rdf_qeucb.qeu_eflag = QEF_EXTERNAL;
	    global->rdf_qeucb.qeu_lk_mode = DMT_IS;
	    global->rdf_qeucb.qeu_access_mode = DMT_A_READ;
	    
	    global->rdf_qeucb.qeu_tab_id.db_tab_base = 0;
	    global->rdf_qeucb.qeu_tab_id.db_tab_index = 0;
	    global->rdf_qeucb.qeu_key = NULL;
	    global->rdf_qeucb.qeu_qual = 0;
	    global->rdf_qeucb.qeu_qarg = 0;
	    global->rdf_qeucb.qeu_getnext = QEU_NOREPO; /* on the initial call to
						    ** QEU_GET, this will position
						    ** DMF to be beginning of table */
	    if (!(global->rdf_init & RDF_IQDATA))
	    {
		status = E_DB_SEVERE;
		rdu_ierror(global, status, E_RD0105_NO_QDATA);
		return(status);
	    }
	}
	else
	{
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0109_NOT_QTUPLE_ONLY);
	    rdu_ierror(global, status, E_RD0103_NOT_OPENED);
	    return(status);
	}
    }
    {
	i4		    avail;		/* number of available blocks
						** for QEF to fill */

	avail = global->rdf_qeucb.qeu_count;

	global->rdf_qeucb.qeu_mask = 0;		/* indicate no timeout */
	status = qef_call(QEU_GET, ( PTR ) &global->rdf_qeucb);
	if (DB_FAILURE_MACRO(status))
	{
	    if (global->rdf_qeucb.error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		rdu_ferror(global, status, &global->rdf_qeucb.error,
		    E_RD0078_QEU_GET,0);
	    }
	    else
	    {
		status = E_DB_OK;
		*eof_found = TRUE;
	    }
	}
	else
	    *eof_found = (global->rdf_qeucb.qeu_count < avail); /* end of file
						** reached if not all blocks have
						** been read */
    }
    global->rdf_qeucb.qeu_getnext = QEU_NOREPO; /* on the subsequent calls to
					    ** QEU_GET, there should be no
					    ** repositioning of the table */
    return(status);
}

/*{
** Name: rdu_qclose	- close the extended system catalog
**
** Description:
**      This routine will close the extended system catalog which has been
**      opened.
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-nov-87 (seputis)
**          initial creation
**      30-nov-88 (seputis)
**          rd0103 used for ierror only not ferror
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	    ALSO fixed an rdu_ferror call that should have been an rdu_ierror
**	    call.
**	30-jan-91 (teresa)
**	    add more info to consistency check e_rd0103_not_opened to gather
**	    information which may let us fix unduplicatable bugs 35513 and 30694
**	01-apr-92 (teresa)
**	    Initialize to no-timeout
**	16-jul-92 (teresa)
**	    prototypes
**	28-jan-93 (teresa)
**	    move RDR_QTUPLE_ONLY from RDF_RB to RDF_RTYPES.RDF_QTUPLE_ONLY.
[@history_template@]...
*/
DB_STATUS
rdu_qclose(RDF_GLOBAL *global)
{
    DB_STATUS	    status;

    /* in distributed case, return ok */
    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	return(E_DB_OK);

    if (!(global->rdf_resources & RDF_RQEU))
    {	/* check to see if the file has been opened previously */
	if (global->rdf_resources & RDF_QTUPLE_ONLY)
	{   /* this is a case in which a file remains open over successive
            ** RDF calls, so the control block needs to be initialized */
	    /* - initialize the QEU control block */
	    global->rdf_qeucb.qeu_acc_id 
		= global->rdfcb->rdf_rb.rdr_rec_access_id; /* this should be
					    ** the QEF id obtained when the
                                            ** the query mod file was opened
                                            ** for scanning */
	    global->rdf_qeucb.qeu_db_id = global->rdfcb->rdf_rb.rdr_db_id;
	    global->rdf_qeucb.qeu_d_id = global->rdfcb->rdf_rb.rdr_session_id;
	    global->rdf_qeucb.qeu_flag = QEF_READONLY;
	    global->rdf_qeucb.qeu_eflag = QEF_EXTERNAL;
	    global->rdf_qeucb.qeu_lk_mode = DMT_IS;
	    global->rdf_qeucb.qeu_access_mode = DMT_A_READ;
	    
	    global->rdf_qeucb.qeu_tab_id.db_tab_base = 0;
	    global->rdf_qeucb.qeu_tab_id.db_tab_index = 0;
	    global->rdf_qeucb.qeu_count = 0;
	    global->rdf_qeucb.qeu_key = NULL;
	    global->rdf_qeucb.qeu_qual = 0;
	    global->rdf_qeucb.qeu_qarg = 0;
	    global->rdf_qeucb.qeu_getnext = QEU_NOREPO; /* on the initial call to
 						    ** QEU_GET, this will position
						    ** DMF to be beginning of table */
	}
	else
	{
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD010A_RQEU_NOT_QTUPLE);
	    rdu_ierror(global, status, E_RD0103_NOT_OPENED);
	    return(status);
	}
    }
    if (!global->rdf_qeucb.qeu_acc_id)
    {	/* if this field is NULL then the file was probably not opened */
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD010B_NO_QEU_ACC_ID);
	rdu_ierror(global, status, E_RD0103_NOT_OPENED);
	return(status);
    }
    global->rdf_qeucb.qeu_mask = 0;		/* indicate no timeout */
    status = qef_call(QEU_CLOSE, ( PTR ) &global->rdf_qeucb);
    global->rdf_resources &= (~RDF_RQEU);    /* release resource */
    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error, 
	    E_RD0079_QEU_CLOSE,0);
    }
    return(status);
}

/**
**	
** Name: rdu_rstatistics - Read the iistatistics and iihistogram tuples and convert
**		    these tuples into a form which can be directly used
**		    by OPF.
**
**	Internal call format:	rdu_rstatistics(rdf_cb)
**
** Description:
**      This function reads the iistatistics and iihistogram tuples and convert
**	these tuples into a form which can be directly used by OPF.
**
**	There is an obscure structure that needs to be documented:
**
** 	  histmap is a boolean array located at the end of the array of ptrs
** 	  to RDD_HISTOs. There is one element in the array for each attribute in
** 	  the table.  It is structured as follows;
**          RDR_INFO.rdr_histogram is array:
**                  [ptr #1][ptr #2]..[ptr #N][bool #1][bool #2]..[bool #N]
** 	  generally we address the hidden array of booleans as a two step 
**	  process.
**  	   	Step 1 - find the start of the boolean array as
**                  	 &RDR_INFO->rdr_histogram[last column +1]
**         	Step 2 - once we have the address of the start of the boolean 
**			 array, then index into it by the attribute number+1
**			 (+1 because an extra element is added at the -1 
**			 position for possible composite histogram).
**  	  We use this array of booleans to indicate whether or not RDF has 
**	  already tried to find statistics for a specific attribute of this 
**	  table.  There is no rule that a user has to provide statistics for ALL
**	  attributes in a table.  If the table has statistics, a specific 
**	  attribute may not have statistics.  If we look for statistics once and
**	  don't find them, then we set the boolean in this array.  Next time RDF
**	  is asked for statistics on this attribute, it will notice that the 
**	  boolean is set and know that there are no statistics for this 
**	  attribute.  This is a performance enhancement to keep RDF from trying 
**	  over and over to query iistatistics for tuples that do not exist.
**
** Inputs:
**	rdf_cb
**	usr_infoblk
** Outputs:
**	rdf_cb
**	    .rdr_info_blk
**		.rdr_historm
**	    .rdf_error
**		.err_code
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}	
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      15-Aug-86 (ac)    
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	14-aug-87 (puree)
**	    convert QEU_ATRAB to QEU_ETRAN
**      30-May-89 (teg)
**          db_atno changed from struct DB_ATT_ID to typedef DB_ATTNUM in
**          parent structures DB_1ZOPTSTAT and DB_2OPTSTAT.  Therefore, modified
**          references to it in this code accordingly.
**      18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    rework interface to rdu_qopen call.
**	09-nov-92 (rganski)
**	    Added allocation and initialization of new RDD_HISTO fields
**	    charnunique and chardensity. The memory is only allocated if the
**	    attribute is of a character type.
**	    For now, these fields are being initialized to constants that will
**	    retain OPF's current behavior. 
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project, continued.
**	    Added initializiation of new rdd_statp fields shistlength and
**	    sversion. If there is a version in sversion, cellsize gets set to
**	    shislength. Added consistency check for shistlength.
**	    Removed initialization of character set statistics arrays when
**	    there is a version ID. Instead, read the statistics from the
**	    catalog and copy them into the arrays. This involves extending the
**	    algorithm for copying the histogram tuples into the appropriate
**	    buffers.
**	30-apr-1993 (robf)
**	    Allocate charnunique/charndensity for security labels (which have
**	    character type histograms)
**	16-feb-94 (teresa)
**	    Fix race condition where sys_infoblk used to read att_type when
**	    usr_infoblk should be used. It is possible that the thread building
**	    the private infoblk somehow got ahead of the thread building the
**	    shared infoblk adn the att_type info is not in the shared infoblk
**	    (or sys_infoblk) yet.  Also added some comments to the description
**	    section to clarify the invisible array of booleans at the end of
**	    the array of PTRs to histograms (RDD_HISTO).
**	25-apr-94 (rganski)
**	    b62184: determine the histogram type of the attribute by checking
**	    new rdr_histo_type field of rdf_cb->rdf_rb.
**	08-oct-96 (pchang)
**	    Ensure that semaphore is not held across QEF/DMF calls when reading
**	    iistatistics and iihistogram tuples (B56369).
**	9-jan-96 (inkdo01)
**	    Support for new "f4repf" array in RDD_HISTO for per-cell 
**	    repetition factors.
**	11-nov-1998 (nanpr01)
**	    Allocate a single chunk rather than allocating small pieces of
**	    memory separately.
**	19-sep-00 (inkdo01)
**	    Changes for composite histograms - histogram array is one element 
**	    longer, to permit "-1" indexed entry.
**	14-may-03 (inkdo01)
**	    Fix copy from shared rdr_histogram to private. It used to misalign
**	    the target.
**	15-Mar-2004 (schka24)
**	    ULM takes care of stream semaphores, don't need mutex here when
**	    doing an rdu-malloc.
**	15-apr-05 (inkdo01)
**	    Tolerate 0 cells - for histograms with all null values.
*/

static DB_STATUS
rdu_rstatistics(RDF_GLOBAL *global)
{
    DB_STATUS	    status;		/* return status of operation */
    bool	    not_found;          /* TRUE if iistatistics tuple is not
                                        ** found */
    RDD_HISTO	    *rdd_statp;         /* ptr to RDF form of iistatistics tuple
                                        */
    i4		    cellcount;          /* number of cells in histogram */
    bool	    eof_found;          /* TRUE if no more tuples exist which
                                        ** in iistatistics or iihistogram */
    RDF_CB	    *rdfcb;
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    bool	    pcrepfs = TRUE;	/* TRUE if iihistogram contains 
					** per-cell repetition factors */
    bool	    has_version;        /* TRUE if iistatistics.sversion
					** contains version id */

    struct
    {	/* Allocate the space for the retrieve output tuples, if allocating
	** space on the stack is a problem then RDF_TUPLE_RETRIEVE can be
        ** made equal to 1, but this may require several calls to DMF */

	QEF_DATA           qdata[RDF_TUPLE_RETRIEVE]; /* allocate space for
					    ** temporary qefdata elements */
	union usr_data
	{
	    DB_1ZOPTSTAT    statistics[RDF_TUPLE_RETRIEVE]; /* buffer area for
                                            ** statistics tuples */
	    DB_2ZOPTSTAT    histograms[RDF_TUPLE_RETRIEVE]; /* buffer area for
					    ** histogram tuples */
	} tempbuffer;
    }   buffer;

    rdfcb = global->rdfcb;
    usr_infoblk = rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;

    if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	/* Check distributed flag for star */
	not_found = FALSE;
	global->rdf_ddrequests.rdd_types_mask = RDD_STATISTICS;

	/* retrieve statistics info from CDB */
	status = rdd_getobjinfo(global);
	if (status)
	    return(status);

	rdd_statp = global->rdf_ddrequests.rdd_statp;
	if (global->rdf_ddrequests.rdd_ddstatus & RDD_NOTFOUND)
	{
	    global->rdf_ddrequests.rdd_ddstatus  &= ~(RDD_NOTFOUND);
	    not_found = TRUE;
	}
	else
	{
	    cellcount = rdd_statp->snumcells;
	}
    }
    else
    {
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);	    /* release semaphore before opening table */

	/* Open the statistics table. */
	global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
	status = rdu_qopen(global, RD_IISTATISTICS, (char *)0, 
			   (i4)sizeof(DB_1ZOPTSTAT), (PTR)&buffer, 
			   (i4)sizeof(buffer)); /* open extended system cat */
	if (DB_FAILURE_MACRO(status))
	    return(status);

	not_found = TRUE;
	do
	{
	    i4	tuple_count;
	    QEF_DATA    *qefstatp;

	    status = rdu_qget(global, &eof_found);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    qefstatp = global->rdf_qeucb.qeu_output; 
	    for (tuple_count = global->rdf_qeucb.qeu_count; 
		(tuple_count--) > 0;
		qefstatp = qefstatp->dt_next)
	    {
		DB_1ZOPTSTAT	    *statp;
		statp = (DB_1ZOPTSTAT *)qefstatp->dt_data;
		if (statp->db_atno == rdfcb->rdf_rb.rdr_hattr_no)
		{
		    not_found = FALSE;

		    status = rdu_malloc(global, (i4)sizeof(RDD_HISTO),
			(PTR *)&rdd_statp);
		    if (DB_FAILURE_MACRO(status))
			return(status);

		    /* 
		    ** Assign the information to the histogram information
		    ** block.
		    */
		    rdd_statp->f4count 		= NULL;
		    rdd_statp->datavalue 	= NULL;
		    rdd_statp->charnunique 	= NULL;
		    rdd_statp->chardensity 	= NULL;
		    rdd_statp->snunique 	= statp->db_nunique;
		    rdd_statp->sreptfact 	= statp->db_reptfact;
		    rdd_statp->snumcells 	= statp->db_numcells;
		    rdd_statp->sunique 		= statp->db_unique;
		    rdd_statp->scomplete 	= statp->db_complete;
		    rdd_statp->sdomain 		= statp->db_domain;
		    rdd_statp->snull 		= statp->db_nulls;
		    rdd_statp->shistlength 	= statp->db_histlength;

		    MEcopy(statp->db_version.db_stat_version,
			   sizeof(DB_STAT_VERSION),rdd_statp->sversion);
		    rdd_statp->sversion[sizeof(DB_STAT_VERSION)] = '\0';

		    /* Determine length of histogram boundary values. If there
		    ** is a version in the sversion field of iistatistics,use
		    ** shistlength; otherwise use the one the caller provided.
		    */
		    if (STcompare(rdd_statp->sversion, DB_NO_STAT_VERSION))
		    {
			/* There is a version */
			has_version = TRUE;
			if (rdd_statp->shistlength <= 0)
			{
			    status = E_DB_ERROR;
			    rdu_ierror(global, status, E_RD010C_HIST_LENGTH);
			    return(status);
			}
		    }
		    else
		    {
			/* There is no version; use length caller provided */
			has_version = FALSE;
			rdd_statp->shistlength = (i2)rdfcb->rdf_rb.rdr_cellsize;
		    }

		    cellcount = statp->db_numcells;

		    /* check for inconsistent database info */		    
		    if (cellcount < 0)
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0106_CELL_COUNT);
			return(status);
		    }
		    break;
		}
	    }
	} while (not_found && !eof_found);
    }

    if (!not_found && rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {	
	/* iistatistics tuple was found so now read in the corresponding
        ** histogram tuples -- this is a distributed thread
	*/

	global->rdf_ddrequests.rdd_types_mask = RDD_HSTOGRAM;
	global->rdf_ddrequests.rdd_statp = rdd_statp;

	/* retrieve histo info from CDB */

	status = rdd_getobjinfo(global); 

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    if ( cellcount > 0 && (!not_found) && !(rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS) )
    {	
	/* iistatistics tuple was found so now read in the corresponding
	** histogram tuples -- this is a local thread 
	*/

	i4		    tuplecount, tuple1count;
	i4		    cellsize;
	i4		    seq;
	f4		    *f4count;
	f4		    *f4repf;
	i4		    *i4repf;
	i4		    f4countlength;
	i4		    f4repflength;
	PTR		    datavalue;
	i4		    histolength;
	i4		    *charnunique;
	f4		    *chardensity;
	i4		    charnlength = 0;
	i4		    chardlength = 0;
	i4		    f4countmax, f4repfmax, histomax;
	i4		    charnmax, chardmax;
	i4		    i;
	bool		    done;

	status = rdu_qclose(global);	/* close iistatistics catalog */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	/* Open the iihistogram table. */
	done = FALSE;
	seq = 0;
	global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
	status = rdu_qopen(global, RD_IIHISTOGRAM, (char *)0, 
			  (i4)sizeof(DB_2ZOPTSTAT), (PTR)&buffer, 
			  (i4)sizeof(buffer)); /* open extended system cat */
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* 
	** Allocate a histogram information block for the 
	** requested attribute number if doesn't exist.
	*/
	f4countlength = f4repflength = cellcount * sizeof(f4);

	status = rdu_malloc(global, (i4)(f4countlength+f4repflength), 
		(PTR *)&f4count);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	rdd_statp->f4count = f4count;
	f4repf  = &f4count[cellcount];	/* f4repf follows f4count */
	i4repf  = (i4 *)f4repf;		/* for later content check */
	rdd_statp->f4repf  = f4repf;

	cellsize = rdd_statp->shistlength;

	/* If the attribute is of a type for which the histogram is a character
	** type, allocate character statistics arrays. If there is no version
	** id, initialize with default character set statistics.
	*/
	{
	    if (rdfcb->rdf_rb.rdr_histo_type == DB_CHA_TYPE ||
		rdfcb->rdf_rb.rdr_histo_type == DB_BYTE_TYPE)
	    {
		charnlength = cellsize * sizeof(i4);
		chardlength = cellsize * sizeof(f4);
		status = rdu_malloc(global, (i4)(charnlength + chardlength),
				    (PTR *)&charnunique);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rdd_statp->charnunique = charnunique;
		chardensity = (f4 *)((char *) charnunique + charnlength); 
		rdd_statp->chardensity = chardensity;

		if (!has_version)
		{
		    /* There is no version id. Initialize with default
		    ** character set statistics, and set charnlength and
		    ** chardlength to 0 to indicate that there are no character
		    ** set statistics in the catalog.
		    */
		    i4		i;		/* Loop counter */

		    for (i = 0; i < cellsize; i++)
		    {
			charnunique[i] = (i4) DB_DEFAULT_CHARNUNIQUE;
			chardensity[i] = (f4) DB_DEFAULT_CHARDENSITY;
		    }
		    charnlength = chardlength = 0;
		}
	    }
	}
	
    	histolength = cellcount * cellsize;

	status = rdu_malloc(global, (i4)histolength, (PTR *)&datavalue);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	rdd_statp->datavalue = datavalue;

	tuplecount = (histolength + f4countlength + charnlength + chardlength +
		      DB_OPTLENGTH - 1) / DB_OPTLENGTH;
	tuple1count = (histolength + f4countlength + charnlength + chardlength +
		      f4repflength + DB_OPTLENGTH - 1) / DB_OPTLENGTH;

	/* The following are boundaries in an imaginary buffer containing
	** all the data for this attribute in iihistogram.
	** They are used when copying the data from tuples to the destination
	** structures.
	*/
	f4countmax 	= f4countlength;
	histomax	= f4countmax + histolength;
	charnmax	= histomax + charnlength;
	chardmax	= charnmax + chardlength;
	f4repfmax	= chardmax + f4repflength;

	do
	{
	    QEF_DATA	    *datablk;	/* ptr to linked list of histogram
					** data blocks read from QEF */
	    i4	    tupleno;    /* number of data blocks left to be
                                        ** scanned in the datablk list */

	    status = rdu_qget(global, &eof_found);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    /* Find the zopt2stat tuple of the requested attribute number. */
	    datablk = global->rdf_qeucb.qeu_output;

	    for (tupleno = global->rdf_qeucb.qeu_count; 
		(tupleno--) > 0;
		datablk = datablk->dt_next)
	    {
		if (((DB_2ZOPTSTAT *)(datablk->dt_data))->db_atno
		    == 
		    rdfcb->rdf_rb.rdr_hattr_no)
		{
		    i4		    	beginhis;
		    i4		    	maxhis;
		    i4			copy_amount, total_copied;
		    DB_2ZOPTSTAT	*histp;

		    histp = (DB_2ZOPTSTAT *)(datablk->dt_data);

		    /* 
		    ** Start the conversion of histogram data into a form
		    ** which can be directly used by OPF.
		    */

		    if (histp->db_sequence < 0 ||
			 histp->db_sequence >= tuple1count)
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0012_BAD_HISTO_DATA);
			return(status);
		    }

		    seq++;		    /* keep a count of the sequence
                                            ** number, note that two tuples
                                            ** may have the same sequence
                                            ** number which is an error which
                                            ** is not caught here FIXME */
		    beginhis = histp->db_sequence * DB_OPTLENGTH;
		    maxhis = beginhis + DB_OPTLENGTH;

		    copy_amount = 0;
		    total_copied =0;

		    /* Get the f4count values */
		    if (beginhis < f4countmax)
		    {
			copy_amount = min(maxhis, f4countmax) - beginhis;
			MEcopy((PTR) histp->db_optdata,
			       copy_amount,
			       (PTR) ((char *)f4count + beginhis));
			beginhis	+= copy_amount;
			total_copied 	+= copy_amount;
		    }
		    /* Get the histogram boundary values */
		    if (maxhis > f4countmax && beginhis < histomax)
		    {
			copy_amount = min(maxhis, histomax) - beginhis;
			MEcopy((PTR) (histp->db_optdata + total_copied),
			       copy_amount,
			       (PTR) ((char *)datavalue + (beginhis -
							   f4countmax)));
			beginhis	+= copy_amount;
			total_copied 	+= copy_amount;
		    }
		    /* Get the char set nunique values */
		    if (maxhis > histomax && beginhis < charnmax)
		    {
			copy_amount = min(maxhis, charnmax) - beginhis;
			MEcopy((PTR) (histp->db_optdata + total_copied),
			       copy_amount,
			       (PTR) ((char *)charnunique + (beginhis -
							     histomax)));
			beginhis	+= copy_amount;
			total_copied 	+= copy_amount;
		    }
		    /* Get the char set density values */
		    if (maxhis > charnmax && beginhis < chardmax)
		    {
			copy_amount = min(maxhis, chardmax) - beginhis;
			MEcopy((PTR) (histp->db_optdata + total_copied),
			       copy_amount,
			       (PTR) ((char *)chardensity + (beginhis -
							     charnmax)));
			beginhis	+= copy_amount;
			total_copied 	+= copy_amount;
		    }
		    /* Get the f4repf values */
		    if (maxhis > chardmax && beginhis < f4repfmax)
		    {
			copy_amount = min(maxhis, f4repfmax) - beginhis;
			MEcopy((PTR) (histp->db_optdata + total_copied),
			       copy_amount,
			       (PTR) ((char *)f4repf + (beginhis -
							     chardmax)));
			beginhis	+= copy_amount;
			total_copied 	+= copy_amount;
		    }
		}
		if (seq == tuple1count)
		{
		    /* FIXME - have a bitmap of tuples read so that a duplicate
                    ** tuple does not affect the result */
		    done = TRUE;
		    break;
		}
	    }
	}
	while    (!done && !eof_found);
	
	/* Check for repetition factor array (f4repf). If !done, or if all
	** f4repf values are (i4)0, it wasn't on the catalog and must be
	** set to the relation-wide repetition factor value here.
	*/
	if (!done) pcrepfs = FALSE;
	 else for (i = 0, pcrepfs = FALSE; i < cellcount && !pcrepfs; i++)
		if (i4repf[i] != 0) pcrepfs = TRUE;

	if (!pcrepfs)
	 for (i = 0; i < cellcount; i++) f4repf[i] = rdd_statp->sreptfact;
			/* not on catalog - force to sreptfact */
    }

    status = rdu_gsemaphore(global);
    if (DB_FAILURE_MACRO(status))
	return(status);			/* need to get a semaphore on
					** update of the histogram since
					** an MEcopy is used which may be
                                        ** a byte wise copy */
    if (!not_found)
    {
	if (global->rdf_resources & RDF_RUPDATE)
	{   /* if a update lock was obtained on the system descriptor then
	    ** this histogram can be placed into the system cache */
	    RDD_HELEMENT	*syshistmap;
	    sys_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_hattr_no] = rdd_statp;
	    syshistmap = (RDD_HELEMENT *)
		(&sys_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]);
	    syshistmap = (RDD_HELEMENT *)&syshistmap[1];
					/* get ptr to lookup map which is found
					** after the array of ptrs to histograms
					** (leaving -1 entry for composite hist.)
					*/
	    syshistmap[rdfcb->rdf_rb.rdr_hattr_no] = TRUE; 
					/* mark histogram as being looked up 
					** whether it was found or not
					*/
	}
	else if (usr_infoblk->rdr_histogram == sys_infoblk->rdr_histogram)
	{   /* if a shared copy of the histogram array is used, but
	    ** a private histogram is created then a copy of the
	    ** histogram array also needs to be made */
	    RDD_HISTO	    **histpp;
	    RDD_HISTO	    **hist1pp;
	    i4		    histsize;

	    /* if histogram descriptor is still not available while
	    ** object is protected by semaphores, then a new structure
	    ** needs to be added by this thread */

	    histsize = (rdfcb->rdf_rb.rdr_no_of_attr + 2) * 
		(sizeof(PTR) + sizeof(RDD_HELEMENT));
	    status = rdu_malloc(global, histsize, (PTR *)&histpp);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    hist1pp = &sys_infoblk->rdr_histogram[-1];
						    /* address the composite
						    ** hist entry (-1) */
	    MEcopy((PTR)hist1pp, histsize, (PTR)histpp);
	    usr_infoblk->rdr_histogram = &histpp[1]; /* save ptr after array has
						    ** been initialized (making 
						    ** space for -1 entry) */
	}
	usr_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_hattr_no] = rdd_statp;
    }

    {	/* set the map that this histogram has been looked up even though
	** it may not exist */
	RDD_HELEMENT	*histmap;
	histmap = (RDD_HELEMENT *)
		(&usr_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]);
	histmap = (RDD_HELEMENT *)&histmap[1];
					/* get ptr to lookup map which is found
					** after the array of ptrs to histograms
					** (leaving -1 entry for composite hist.)
					*/
	histmap[rdfcb->rdf_rb.rdr_hattr_no] = TRUE; 
					/* mark histogram as being looked up 
					** whether it was found or not 
					*/
	if ((sys_infoblk->rdr_histogram)
	    &&
	    ((global->rdf_resources & RDF_RUPDATE) || not_found))
	{
	    histmap = (RDD_HELEMENT *)
		(&sys_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]);
	    histmap = (RDD_HELEMENT *)&histmap[1];
					/* get ptr to lookup map which is found
					** after the array of ptrs to histograms
					** (leaving -1 entry for composite hist.)
					*/
	    histmap[rdfcb->rdf_rb.rdr_hattr_no] = TRUE; 
					/* mark histogram as being looked up 
					** whether it was found or not
					*/
	}
    }
    return(status);
}

/*{
** Name: rdu_statistics	- read histogram and statistics tuples for an attribute
**
** Description:
**      Read the statistics and histogram info for an attribute if
**      not in cache. 
**
**	There is an obscure structure that needs to be documented:
**
** 	  histmap is a boolean array located at the end of the array of ptrs
** 	  to RDD_HISTOs. There is one element in the array for each attribute in
** 	  the table.  It is structured as follows;
**          RDR_INFO.rdr_histogram is array:
**                  [ptr #1][ptr #2]..[ptr #N][bool #1][bool #2]..[bool #N]
** 	  generally we address the hidden array of booleans as a two step 
**	  process.
**  	   	Step 1 - find the start of the boolean array as
**                  	 &RDR_INFO->rdr_histogram[last column +1]
**         	Step 2 - once we have the address of the start of the boolean 
**			 array, then index into it by the attribute number+1.
**			 (+1 because an extra element is added at the -1 
**			 position for possible composite histogram).
**  	  We use this array of booleans to indicate whether or not RDF has 
**	  already tried to find statistics for a specific attribute of this 
**	  table.  There is no rule that a user has to provide statistics for ALL
**	  attributes in a table.  If the table has statistics, a specific 
**	  attribute may not have statistics.  If we look for statistics once and
**	  don't find them, then we set the boolean in this array.  Next time RDF
**	  is asked for statistics on this attribute, it will notice that the 
**	  boolean is set and know that there are no statistics for this 
**	  attribute.  This is a performance enhancement to keep RDF from trying 
**	  over and over to query iistatistics for tuples that do not exist.
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-nov-87 (seputis)
**          initial creation
**      5-oct-88 (seputis)
**          initialize "done" boolean to false
**	26-jan-90 (teg)
**	    fix concurency bug where it was possible for sys_infoblk.rdr_histogram
**	    to be null and still be used by rdu_rstatistics, causing an AV. (bug
**	    9953)
**      22-may-92 (teresa)
**          added new parameter to rdu_xlock call for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	12-mar-93 (teresa)
**	    moved rdf_shared_sessions increment logic out of rdu_xlock call.
**	16-feb-94 (teresa)
**	    update checksum whether or not tracepoint to skip checking
**	    the checksum is set.
**      11-nov-1994 (cohmi01) checksum sys block instead of usr block.
**          Even where the 'usr' variable is used, if it points to the
**          sys_infoblk, recompute the checksum.
**	16-may-00 (inkdo01)
**	    Changes for composite histograms.
**	19-sep-00 (inkdo01)
**	    Changes for composite histograms - histogram array is one element 
**	    longer, to permit "-1" indexed entry.
**	15-Mar-2004 (schka24)
**	    Mutex-protect sysinfo block state changes.
[@history_template@]...
*/
static DB_STATUS
rdu_statistics(RDF_GLOBAL *global)
{
    RDD_HELEMENT    *histmap;
    RDF_CB	    *rdfcb;
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    DB_STATUS	    status;
    bool	    comphist;

    status = E_DB_OK;
    rdfcb = global->rdfcb;
    comphist = ((rdfcb->rdf_rb.rdr_2types_mask & RDR2_COMPHIST) != 0);
    usr_infoblk = rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    if (!comphist && rdfcb->rdf_rb.rdr_hattr_no < 0 ||
	rdfcb->rdf_rb.rdr_cellsize == 0 ||
	rdfcb->rdf_rb.rdr_no_of_attr <= 0)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }		
    if ((!usr_infoblk->rdr_histogram)
	&&
	(usr_infoblk == sys_infoblk))
    {
	/* X-lock the master info block if that's what we're using, might
	** return with a private copy
	*/
	status = rdu_xlock(global, &usr_infoblk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    if (!usr_infoblk->rdr_histogram)
    {
	if (sys_infoblk->rdr_histogram)
	{   /* use system copy for as long as possible */
	    usr_infoblk->rdr_histogram = sys_infoblk->rdr_histogram;
	    usr_infoblk->rdr_types |= RDR_STATISTICS;
	}
	else
	{
	    RDD_HISTO	    **histpp;
	    i4		    histsize;


	    /* if histogram descriptor is still not available while
	    ** object is protected by semaphores, then a new structure
	    ** needs to be added by this thread */

	    histsize = (rdfcb->rdf_rb.rdr_no_of_attr + 2) * 
		(sizeof(PTR) + sizeof(RDD_HELEMENT));
	    status = rdu_malloc(global, histsize, (PTR *)&histpp);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    MEfill(histsize, (u_char)0, (PTR)histpp);
	    usr_infoblk->rdr_histogram = &histpp[1]; /* save ptr after array has
					** been initialized (leaving an entry for
					** comp histogram at -1 location) */
	    usr_infoblk->rdr_types |= RDR_STATISTICS;
	    if (global->rdf_resources & RDF_RUPDATE)
	    {   /* if an exclusive lock on the system cache is held then
		** place link to histogram in system descriptor */
		status = rdu_gsemaphore(global);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		sys_infoblk->rdr_histogram = &histpp[1]; /* save ptr after array has
					    ** been initialized */
		sys_infoblk->rdr_types |= RDR_STATISTICS;
		status = rdu_rsemaphore(global);
		if (DB_FAILURE_MACRO(status))
		    return (status);
	    }
	}
    }
    histmap = (RDD_HELEMENT *)(&usr_infoblk->
	rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]);
				    /* this map is used to determine if
				    ** if a lookup on a histogram as
				    ** occurred, so that repeated lookups
				    ** are not done if the histogram does
				    ** not exist */
    histmap = (RDD_HELEMENT *) &histmap[1];
				    /* leaving -1 entry for composite hist. */
    
    if (!histmap[rdfcb->rdf_rb.rdr_hattr_no] )
    {	
	if (!(global->rdf_resources & RDF_RUPDATE))
	{   
	    /* try to get an exclusive lock on descriptor */
	    status = rdu_xlock(global, &usr_infoblk); /* need to reserve
					** descriptor for update, and to
					** initialize memory manager, since
					** there may be duplicated histograms
					** in the system stream if this call
					** is not made here */
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    if (global->rdf_resources & RDF_RUPDATE)
	    {
		/* make sure that memory gets allocated from system memory 
		** stream from now on.  Since we just got a lock, it is possible
		** that we did not have RDF_UPDATE before the lock and were
		** just given that privilege.  If we did not have RDF_UPDATE,
		** we had been using a private memory stream to allocate memory.
		** Now we have RDF_UPDATE, and will be updating the REAL copy of
		** the sysinfoblk, so we must now use the system memory stream.
		*/
		global->rdf_init &= ~RDF_IULM; 
		if ( !(sys_infoblk->rdr_histogram) )
		{
		    /* this is an obscure case where we were working from a private
		    ** copy of the sysinfoblk because another thread had an UPDATE 
		    ** LOCK on the system copy.  Now that other thread has exited,
		    ** and we have been granted the UPDATE LOCK on the system copy.
		    **
		    ** If the system copy of sysinfoblk does not already have a 
		    ** pointer to histogram information, then allocate memory to
		    ** hold histogram info that will later be filled in by 
		    ** rdr_rstatistics.  Initialize the newly allocated memory to 0.
		    */
		    
		    RDD_HISTO	    **histptr;
		    i4		    h_size;

		    h_size = (rdfcb->rdf_rb.rdr_no_of_attr + 2) * 
			     (sizeof(PTR) + sizeof(RDD_HELEMENT));
		    status = rdu_malloc(global, h_size, (PTR *)&histptr);
		    if (DB_FAILURE_MACRO(status))
			return(status);
		    MEfill(h_size, (u_char)0, (PTR)histptr);
		    status = rdu_gsemaphore(global);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    sys_infoblk->rdr_histogram = &histptr[1];
		    sys_infoblk->rdr_types |= RDR_STATISTICS;
		    status = rdu_rsemaphore(global);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
	    }
	}
	if ((sys_infoblk->rdr_histogram != usr_infoblk->rdr_histogram)
	    &&
	    (sys_infoblk->rdr_histogram))
	{   /* if this is a private descriptor then see if there exists
	    ** a histogram in the system descriptor */
	    RDD_HELEMENT	    *syshistmap;

	    syshistmap =  (RDD_HELEMENT *)(&sys_infoblk->
		rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]);
	    syshistmap = (RDD_HELEMENT *)&syshistmap[1];
	    if (syshistmap[rdfcb->rdf_rb.rdr_hattr_no])
	    {
		histmap[rdfcb->rdf_rb.rdr_hattr_no] =
		    syshistmap[rdfcb->rdf_rb.rdr_hattr_no]; /* in case the user 
					    ** got a ptr to the system 
					    ** histogram, copy the lookup 
					    ** flag also */
		usr_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_hattr_no] = 
		    sys_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_hattr_no];
	    }
	}
    }
    histmap = (RDD_HELEMENT *)(&usr_infoblk->
	rdr_histogram[rdfcb->rdf_rb.rdr_no_of_attr + 1]); /* need to 
					    ** recalculate since a new
                                            ** usr_infoblk may have been created
				 	    ** (leaving -1 entry for composite hist)
                                            */
    histmap = (RDD_HELEMENT *)&histmap[1];
    if (!histmap[rdfcb->rdf_rb.rdr_hattr_no])
    {
	/* 
	** Read the iistatistics and iihistogram tuples and construct 
	** these tuples info a form which can be directly used by OPF.
        ** - note that the system descriptor should be reserved for
        ** update, or else a private copy will be used 
	*/
	status = rdu_rstatistics(global);
	Rdi_svcb->rdvstat.rds_b11_stat++;
    }
    else
	Rdi_svcb->rdvstat.rds_c11_stat++;
    if (DB_SUCCESS_MACRO(status)
	&&
	!usr_infoblk->rdr_histogram[rdfcb->rdf_rb.rdr_hattr_no])
    {
	status = E_DB_WARN;
	rdu_ierror(global, status, E_RD001B_HISTOGRAM_NOT_FOUND);
    }
    return(status);
}

/*{
** Name: rdu_bld_colcompare - loads column comparison stats for a table
**
** Description:
**      Read the column comparison statistics for a table if not in cache
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-jan-06 (dougi)
**          Cloned from rdu_statistics().
[@history_template@]...
*/
DB_STATUS
rdu_bld_colcompare(RDF_GLOBAL *global)
{
    RDF_CB	    *rdfcb;
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    DMT_COLCOMPARE  *rdd_colcp = NULL;
    DB_COLCOMPARE   *colcp;
    i4		    rowcount = 0;
    i4		    i, tupix, bufcount;
    bool	    reread, eof_found;
    QEF_DATA	    *qefcolcp;
    DB_STATUS	    status;

    struct
    {	/* Allocate the space for the retrieve output tuples, if allocating
	** space on the stack is a problem then RDF_TUPLE_RETRIEVE can be
        ** made equal to 1, but this may require several calls to DMF */

	QEF_DATA           qdata[RDF_TUPLE_RETRIEVE]; /* allocate space for
					    ** temporary qefdata elements */
	DB_COLCOMPARE	colstats[RDF_TUPLE_RETRIEVE];	/* space for stats */
    }   buffer;

    status = E_DB_OK;
    rdfcb = global->rdfcb;
    usr_infoblk = rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;

    if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	/* Not with Star for now. */
	return(E_DB_OK);
    }

    if ((!usr_infoblk->rdr_colcompare)
	&&
	(usr_infoblk == sys_infoblk))
    {
	/* X-lock the master info block if that's what we're using, might
	** return with a private copy
	*/
	status = rdu_xlock(global, &usr_infoblk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    if (!usr_infoblk->rdr_colcompare)
    {
	if (sys_infoblk->rdr_colcompare)
	{   /* use system copy for as long as possible */
	    usr_infoblk->rdr_colcompare = sys_infoblk->rdr_colcompare;
	    usr_infoblk->rdr_2_types |= RDR2_COLCOMPARE;
	}
	else
	{
	    /* colcompare stats have yet to be read for this table. 
	    ** Do it here. */
	    status = rdu_rsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);	    /* release semaphore before opening table */

	    /* Open the column comparison statistics table. */
	    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, RD_IICOLCOMPARE, (char *)0, 
			   (i4)sizeof(DB_COLCOMPARE), (PTR)&buffer, 
			   (i4)sizeof(buffer)); /* open extended system cat */
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    do
	    {
		/* Loop to read all iicolcompare tuples for table, so
		** they can be counted for later memory allocation. */

		status = rdu_qget(global, &eof_found);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rowcount += global->rdf_qeucb.qeu_count;
	    } while (!eof_found);

	    /* We now have a count and, if it is less than RDF_TUPLE_RETRIEVE,
	    ** the tuples themselves. Memory can now be allocated and the
	    ** tuples copied in. If there are >= RDF_TUPLE_RETRIEVE rows, 
	    ** we'll have to re-read them. */

	    status = rdu_malloc(global, sizeof(DMT_COLCOMPARE) + 
		(rowcount-1) * sizeof(DB_COLCOMPARE), (PTR *)&rdd_colcp);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    usr_infoblk->rdr_colcompare = rdd_colcp;
	    rdd_colcp->col_comp_cnt = rowcount;

	    if (rowcount >= RDF_TUPLE_RETRIEVE)
	    {
		/* More than one batch of tuples was read. Close table to 
		** reposition, open it and away we go. Otherwise, all the
		** tuples we want are already in "buffer". */
		status = rdu_qclose(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		status = rdu_qopen(global, RD_IICOLCOMPARE, (char *)0, 
			   (i4)sizeof(DB_COLCOMPARE), (PTR)&buffer, 
			   (i4)sizeof(buffer)); /* open extended system cat */
		if (DB_FAILURE_MACRO(status))
		    return(status);
		reread = TRUE;
	    }
	    else reread = FALSE;

	    bufcount = rowcount;
	    tupix = 0;

	    do {
		/* Outer loop drives the re-reading (if necessary). */

		if (reread && rowcount > 0)
		{
		    /* Read next chunk (if necessary). */
		    status = rdu_qget(global, &eof_found);
		    if (DB_FAILURE_MACRO(status))
			return(status);

		    bufcount = (rowcount < RDF_TUPLE_RETRIEVE) ? rowcount
						: RDF_TUPLE_RETRIEVE;
		}

		qefcolcp = global->rdf_qeucb.qeu_output; 
		for (i = 0; i < bufcount; i++, tupix++,
					qefcolcp = qefcolcp->dt_next)
		{
		    /* Loop over tuples, copying them to RDF memory. */
		    colcp = (DB_COLCOMPARE *)qefcolcp->dt_data;
		    STRUCT_ASSIGN_MACRO(colcp->cc_tabid1,
			rdd_colcp->col_comp_array[tupix].cc_tabid1);
		    STRUCT_ASSIGN_MACRO(colcp->cc_tabid2,
			rdd_colcp->col_comp_array[tupix].cc_tabid2);
		    rdd_colcp->col_comp_array[tupix].cc_count = 
					colcp->cc_count;
		    rdd_colcp->col_comp_array[tupix].cc_col1[0] =
					colcp->cc_col1[0];
		    rdd_colcp->col_comp_array[tupix].cc_col1[1] =
					colcp->cc_col1[1];
		    rdd_colcp->col_comp_array[tupix].cc_col1[2] =
					colcp->cc_col1[2];
		    rdd_colcp->col_comp_array[tupix].cc_col1[3] =
					colcp->cc_col1[3];
		    rdd_colcp->col_comp_array[tupix].cc_col2[0] =
					colcp->cc_col2[0];
		    rdd_colcp->col_comp_array[tupix].cc_col2[1] =
					colcp->cc_col2[1];
		    rdd_colcp->col_comp_array[tupix].cc_col2[2] =
					colcp->cc_col2[2];
		    rdd_colcp->col_comp_array[tupix].cc_col2[3] =
					colcp->cc_col2[3];
		    rdd_colcp->col_comp_array[tupix].cc_ltcount = 
					colcp->cc_ltcount;
		    rdd_colcp->col_comp_array[tupix].cc_eqcount = 
					colcp->cc_eqcount;
		    rdd_colcp->col_comp_array[tupix].cc_gtcount = 
					colcp->cc_gtcount;

		}

		rowcount -= RDF_TUPLE_RETRIEVE;
	    } while (rowcount > 0 && !eof_found);

	    status = rdu_qclose(global);	/* close iicolcompare catalog */

	}   /* end of colcompare build logic */
    }

    return(status);
}

/*
** Name: rdr_tuple - read data from IITREE catalog tuples
**
** Description:
**	This routine reads data from the IITREE catalog. It is called with
**  a request for an object (pst_header, pst_node) returns a buffer
**  with the object. This routine will get more
**  tree tuples if required.
**
**  Inputs:
**	global				ptr to RDF global state variable
**      length                          requested object size
**      tstate                          state of RDF during read of iitree
**  Outputs:
**      buffer                          ptr to memory for object
**  Return:
**	E_DB_{OK, WARN, ERROR, FATAL}
**
** Side Effects:
**		none
** History:
**	9-oct-86 (daved)
**	    written
**	14-feb-87 (daved)
**	    clear status if NO MORE ROWS
**      10-nov-87 (seputis)
**          rewritten for resource management
**	09-mar-89 (neil)
**	    added check for rule trees.
**      30-Jun-89 (teg)
**          changed DB_IITREE.dbt_tree references to reflect the fact that
**          this is defined as a u_i2 dbt_l_tree and char dbt_tree[1024]
**          for UNIX Portability instead of as a DB_TEXT_STRING dbt_tree struct.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdr_tuple(  RDF_GLOBAL	*global,
	    i4		length,
	    PTR		buffer,
	    RDF_TSTATE  *tstate)
{
    DB_STATUS	    status;
    QEF_DATA	    *qdata;		/* current QEF tuple which is being
					** read */
    DB_IITREE	    *tuple;             /* current iitree tuple attached to
                                        ** to qdata */
    i4		    remaining;		/* number of unread bytes remaining
                                        ** in the current tuple */

    qdata = tstate->rdf_qdata;
    if (qdata)
    {
	tuple = (DB_IITREE*) qdata->dt_data;
	remaining = tuple->dbt_l_tree - tstate->rdf_toffset;
    }
    else
    {
	tuple = NULL;
	remaining = 0;
    }

    /* try to satisfy request with current tuple */
    while ( length > 0 )
    {
	/* if no more space, get next tuple */	
	if (!remaining)
	{
	    DB_TREE_ID		*tree_id;
	    tree_id = &global->rdfcb->rdf_rb.rdr_tree_id;

	    while(!remaining		/* if no bytes remain in current tuple*/
		||
		!tuple			/* or if tuple has not been read */
		||
		(			/* or if not correct tree_id */
		    (	    (tree_id->db_tre_high_time 
			    != 
			    tuple->dbt_trid.db_tre_high_time)
			||
			    (tree_id->db_tre_low_time 
			    != 
			    tuple->dbt_trid.db_tre_low_time)
		    )
		    &&
		    (	(tstate->rdf_treetype == DB_INTG)
			||
			(tstate->rdf_treetype == DB_PROT) /* tree id
					    ** needed for integrities and
                                            ** permits, (but not views) */
			||
		    	(tstate->rdf_treetype == DB_RULE)
		    )
		))
	    {
		if (!tuple 
		    ||
		    (tstate->rdf_tcount == global->rdf_qeucb.qeu_count))
		{
		    /* get next batch of tuples */
		    if (tstate->rdf_eof)
		    {	/* if the previous call detected no more tuples
                        ** then some inconsistency exists */
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0107_QT_SEQUENCE);
			return(status);
		    }

		    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
		    {	/* get tree from distributed queries */
			status = rdd_gtreetuple(global, &tstate->rdf_eof);
		    }
		    else
		    {
			/* use local queries to get QT */
			status = rdu_qget(global, &tstate->rdf_eof);
		    }
		    if (DB_FAILURE_MACRO(status))
			return(status);
		    if (global->rdf_qeucb.qeu_count <= 0)
		    {	/* if the previous call detected no more tuples
                        ** then some inconsistency exists */
			status = E_DB_ERROR;
			if (tstate->rdf_seqno < 0)
			    rdu_ierror(global, status, E_RD0200_NO_TUPLES);
					    /* this special case is made since
                                            ** there is a possibility that
                                            ** table was deleted, but the
                                            ** iirelation tuple still exists
                                            ** PSF can retry or report table
                                            ** not found errors */
			else
			    rdu_ierror(global, status, E_RD0107_QT_SEQUENCE);
			return(status);
		    }
		    /* start at front of list */
		    tstate->rdf_tcount = 1;
		    qdata =
		    tstate->rdf_qdata = global->rdf_qeucb.qeu_output; /* get
					    ** beginning of the list of qef_data
					    ** blocks */
		}
		else
		{
		    /* get next row and tuple */
		    qdata = qdata->dt_next;
		    /* set the new tuple */
		    (tstate->rdf_tcount)++;
		}
		tuple = (DB_IITREE*) qdata->dt_data;
		remaining = tuple->dbt_l_tree;
		/* now loop until we get a tuple with the correct ID and
		** has a non-zero number of bytes */
	    }
	    /* at this point a valid new tuple has been found so check
            ** the sequence number for consistency */
	    tstate->rdf_seqno++;
	    tstate->rdf_qdata = qdata;
	    tstate->rdf_toffset = 0;
	    if (tstate->rdf_seqno != tuple->dbt_trseq)
	    {	/* out of sequence tuple, since it is assumed that BTREE
                ** will return tuples in order */
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0107_QT_SEQUENCE);
		return(status);
	    }
	}
	{
	    /* get size to move */
	    i4	    copy_length;

	    copy_length = length > remaining ? remaining : length;
	    /* move the data */
            MEcopy((PTR) &tuple->dbt_tree[tstate->rdf_toffset],
                    copy_length, (PTR)buffer);
	    tstate->rdf_toffset += copy_length;
	    length -= copy_length;
	    buffer = (PTR)&((char *)buffer)[copy_length];
	    remaining = tuple->dbt_l_tree - tstate->rdf_toffset;
	}
    }	
    return (E_DB_OK);
}

/*
** Name: rdr_tree - build the tree from IITREE catalog tuples
**
** Description:
**	This routine builds the query tree by calling rdr_tuple to get
**  tree objects and then assembling the objects into the tree. The
**  tree is created through a pre-order walk. The walk is right to left.
**
**  Inputs:
**	qeu				contains tree tuples
**	tree_id				used to qualify tuples for integrity
**					and permits.
**	ulm_rcb				memory stream in which tree objects
**	count				current row number (count from 0).
**	offset				offset within current tuple
**  Outputs:
**	count				new row number
**	offset				offset within current tuple
**	root				query tree.
**	error				location for any error messages.
**  Return:
**	E_DB_{OK, WARN, ERROR, FATAL}
**
** Side Effects:
**		none
** History:
**	10-oct-86 (daved)
**	    written
**	4-feb-87 (daved)
**	    allow for root nodes with unions.
**	14-may-87 (daved)
**	    resest the function description address.
**      16-feb-88 (seputis)
**          rewrote for resource handling, error recovery,
**          NOTE - PST_AOP can have 0 operands e.g. count(*)
**          so that the symbol type cannot be used to determine
**          if a left or right child exists, introduced
**          compression to storing symbols
**	26-aug-88 (seputis)
**	    highly recursive - runs out of stack space so placed structures
**	    in tstate
**      3-jan-89 (seputis)
**          fixed ADI_EXIST_OP problem
**	09-mar-89 (neil)
**	    added checks for rule trees.
**	29-sep-89 (teg)
**	    added support for Outter Joins.
**	01-Feb-90  (teg)
**	    fixed RDF caused ADF AV where RDF was calling adi_fidesc with a
**	    NULL adf_cb ptr.  ADF was AV if it encountered an error condition, 
**	    because it tried to deposit error information in the 
**	    adf_cb->adf_errcb but adf_cb was NULL.
**	12-feb-90   (teg)
**	    Removed old logic to deal with uncompressed trees, since they are
**	    no longer supported.
**	14-feb-90   (teg)
**	    made iterative on left side.  This should reduce stack usage
**	    drastically less for long resdom lists.
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	23-feb-93 (teresa)
**	    support enlarged PST_8_VSN PST_RSDM node in trees.
**	20-aug-93 (teresa)
**	    Simplified handling of pre-current release trees according to the
**	    following:	
**		Star views support PST_2_VSN, PST_3_VSN, PST_5_VSN, PST_8_VSN
**		ALL Non-Distributed trees Support only PST_8_VSN.
**	    (At the time of this work, PST_CUR_VSN is PST_8_VSN.)  Therefore, if
**	    tree level is PST_0_VSN, PST_1_VSN, PST_4_VSN, PST_6_VSN or 
**	    PST_7_VSN, generate a consistency Check (E_RD0146_ILLEGAL_TREEVSN).
**	17-jan-00 (inkdo01)
**	    Added PST_CASEOP, PST_WHLIST, PST_WHOP for case expressions.
**	6-dec-02 (inkdo01)
**	    Add support for PST_8_VSN versions of PST_ROOT_NODE and 
**	    PST_RNGENTRY before range table expansion.
**	14-dec-04 (inkdo01)
**	    PST_9_VSN has added db_collID field in DB_DATA_VALUE.
**	10-Jan-2005 (schka24)
**	    Adjust above fix for 64-bit v9 trees.
**	25-Jun-2008 (kiria01) SIR120473
**	    pst_isescape is now u_i2 pst_pat_flags.
[@history_template@]...
*/
static DB_STATUS
rdr_tree(   RDF_GLOBAL	*global,
	    PST_QNODE	**root,
	    RDF_TSTATE	*tstate)
{
    DB_STATUS		    status;
    PST_QNODE		    *pst_qnode;
    PST_QNODE		    **next;
    i4			    i;
    i4			    read_symsize;   /* size of symbol to read */
    i4			    write_symsize;  /* size of symbol to allocate memory
					    */
    i4			    prefix_length = sizeof(PST_SYMBOL) -
						sizeof(PST_SYMVALUE);
    RDF_CHILD		    children;

    /* If earlier than PST_10_VSN, copy old (shorter) DB_DATA_VALUE. */
    if (tstate->rdf_version <= PST_9_VSN)
	prefix_length = sizeof(PST_SYMBOL_V9) - sizeof(PST_SYMVALUE);
    
    /* set up loop to iterate on left side of tree */
    next = root;
    while (next)
    {
	/* new style way of storing the query tree, compress the PST_SYMVALUE
        ** component, and explicitly store the left,right children needed */

	/* get the child indicator */
	status = rdr_tuple(global, sizeof (RDF_CHILD), (PTR) &children,
	    tstate);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	status = rdr_tuple(global, 
	    prefix_length,
	    (PTR) &tstate->rdf_temp,
	    tstate);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* If earlier than PST_10_VSN splice in new db_collID field 
	** to DB_DATA_VALUE. */
	if (tstate->rdf_version <= PST_9_VSN)
	{
	    char *old_lenp;
	    PST_SYMBOL_V9 *old_symptr;

	    /* Copy old pst_len into position and fill in collID.
	    ** We haven't read pst_value yet, just move the length.
	    */
	    old_symptr = (PST_SYMBOL_V9 *) &tstate->rdf_temp;
	    old_lenp = (char *) &old_symptr->pst_len;
	    MEcopy(old_lenp, sizeof(tstate->rdf_temp.pst_len),
			(char *)&tstate->rdf_temp.pst_len);
	    tstate->rdf_temp.pst_dataval.db_collID = 0;
	}

	switch (tstate->rdf_temp.pst_type)
	{
	case PST_AND:
	case PST_OR:
	case PST_UOP:
	case PST_BOP:
	case PST_AOP:
	case PST_COP:
	case PST_MOP:
	{
	    switch (tstate->rdf_version)
	    {
	    /* versions 2, 3 and 5 required to support early versions of
	    ** star views.  However, all versions before PST_8_VSN are illegal
	    ** for post 6.4 NON-DISTRIBUTED tree processing.
	    */
	    case PST_4_VSN:
	    case PST_6_VSN:
	    case PST_7_VSN:
		/* this version of tree is illegal in post 6.4 databases,
		** so raise a consistency check 
		*/
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		return(status);
	    case PST_2_VSN:
		/* only used for old distributed views */
		read_symsize = sizeof(PST_2OP_NODE);
		break;
	    case PST_3_VSN:
	    case PST_5_VSN:
		/* only used for old distributed views */
		read_symsize = sizeof(PST_3OP_NODE);
		break;
	    case PST_8_VSN:
	    case PST_9_VSN:
	    case PST_CUR_VSN:
		read_symsize = sizeof(PST_OP_NODE);
		break;
	    default:
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		return(status);
	    }
	    write_symsize = sizeof(PST_OP_NODE);
	    break;
	}
	case PST_CONST:
	{   /* old versions of the query tree exist */
	    switch (tstate->rdf_version)
	    {
	    /* versions 2, 3 and 5 required to support early versions of
	    ** star views.  However, all versions before PST_8_VSN are illegal
	    ** for post 6.4 NON-DISTRIBUTED tree processing.
	    */
	    case PST_4_VSN:
	    case PST_6_VSN:
	    case PST_7_VSN:
		/* this version of tree is illegal in post 6.4 databases,
		** so raise a consistency check 
		*/
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		return(status);
	    case PST_2_VSN:
		/* only used for old distributed views */
		read_symsize = sizeof(PST_2CNST_NODE);
		break;
	    case PST_3_VSN:	/* only used for old distributed views */
	    case PST_5_VSN:	/* only used for old distributed views */
	    case PST_8_VSN:
	    case PST_9_VSN:
	    case PST_CUR_VSN:
		read_symsize = sizeof(PST_CNST_NODE);
		break;
	    default:
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		return(status);
	    }
	    write_symsize = sizeof(PST_CNST_NODE);
	    break;
	}
	case PST_RESDOM:
	case PST_BYHEAD:
	{   /* old versions of the query tree exist */
	    switch (tstate->rdf_version)
	    {
	    /* versions 2, 3 and 5 required to support early versions of
	    ** star views.  However, all versions before PST_8_VSN are illegal
	    ** for post 6.4 NON-DISTRIBUTED tree processing.
	    */
	    case PST_4_VSN:
	    case PST_6_VSN:
	    case PST_7_VSN:
		/* this version of tree is illegal in post 6.4 databases,
		** so raise a consistency check 
		*/
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		return(status);
	    case PST_2_VSN:
		/* only used for old distributed views */
		read_symsize = sizeof(PST_2RSDM_NODE);
		break;
	    case PST_3_VSN:
	    case PST_5_VSN:
		/* only used for old distributed views */
		read_symsize = sizeof(PST_7RSDM_NODE);
		break;
	    case PST_8_VSN:
	    case PST_9_VSN:
	    case PST_CUR_VSN:
		read_symsize = sizeof(PST_RSDM_NODE);
		break;
	    default:
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		return(status);
	    }
	    write_symsize = sizeof(PST_RSDM_NODE);
	    break;
	}
	case PST_VAR:
	    read_symsize =
	    write_symsize = sizeof(PST_VAR_NODE);
	    break;
	case PST_AGHEAD:
	case PST_ROOT:
	case PST_SUBSEL:
	    switch (tstate->rdf_version)
	    {
	    case PST_8_VSN:
		/* Precedes range table expansion. */
		read_symsize = sizeof(PST_8RT_NODE);
		break;
	    case PST_9_VSN:
	    case PST_CUR_VSN:
		read_symsize = sizeof(PST_RT_NODE);
		break;
	    }
	    write_symsize = sizeof(PST_RT_NODE);
	    break;
	case PST_QLEND:
	case PST_TREE:
	case PST_NOT:
	case PST_WHLIST:
	case PST_WHOP:
	case PST_OPERAND:
	    read_symsize =
	    write_symsize = 0;
	    break;
	case PST_CASEOP: 
	    read_symsize = write_symsize = sizeof(PST_CASE_NODE);
	    break;
	case PST_RULEVAR:
	    read_symsize = write_symsize = sizeof(PST_RL_NODE);
	    break;
	case PST_SORT:
	case PST_CURVAL:
	default:
	    /* unexpected node type */
	    rdu_ierror(global, E_DB_ERROR, E_RD0132_QUERY_TREE); /* unexpected node type */
	    return(E_DB_ERROR);
	}
	{
	    i4	    tempsize;
	    /* consistency check - is the symbol length correct */
	    if (tstate->rdf_temp.pst_dataval.db_data)
		tempsize = read_symsize + tstate->rdf_temp.pst_dataval.db_length;
	    else
		tempsize = read_symsize;
	    if ((tempsize != tstate->rdf_temp.pst_len)
		||
		(tstate->rdf_temp.pst_dataval.db_length < 0)
		)
	    {
		/* bad length stored */
		rdu_ierror(global, E_DB_ERROR, E_RD0132_QUERY_TREE); /* bad length stored */
		return(E_DB_ERROR);
	    }
	}
	status = rdu_malloc(global, 
	    (i4)(sizeof(PST_QNODE)-sizeof(PST_SYMVALUE)+write_symsize), 
	    (PTR *)&pst_qnode);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	*next = pst_qnode;
	MEcopy((PTR)&tstate->rdf_temp, sizeof(PST_SYMBOL)-sizeof(PST_SYMVALUE),
	    (PTR)&pst_qnode->pst_sym);
	if (read_symsize > 0)
	{
	    switch (tstate->rdf_temp.pst_type)
	    {
	    case PST_CONST:
	    {   /* old versions of the query tree exist */
		switch (tstate->rdf_version)
		{
		/* versions 2, 3 and 5 required to support early versions of
		** star views.  However, all versions before PST_8_VSN are 
		** illegal for post 6.4 NON-DISTRIBUTED tree processing.
		*/
		case PST_4_VSN:
		case PST_6_VSN:
		case PST_7_VSN:
		    /* this version of tree is illegal in post 6.4 databases,
		    ** so raise a consistency check 
		    */
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		    return(status);
		case PST_2_VSN:
		{
		    /* only used for old distributed views */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_2cnst_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no
			= tstate->rdf_2cnst_node.pst_parm_no;
		    pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_pmspec
			= tstate->rdf_2cnst_node.pst_pmspec;
		    pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_cqlang
			= tstate->rdf_2cnst_node.pst_cqlang;
		    pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt
			= NULL;
		    pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype
			= PST_USER;
		    if (pst_qnode->pst_sym.pst_dataval.db_data)
			pst_qnode->pst_sym.pst_len = write_symsize 
			    + pst_qnode->pst_sym.pst_dataval.db_length;
		    else
			pst_qnode->pst_sym.pst_len = write_symsize;
		    break;
		}
		case PST_3_VSN:	    /* only used for old distributed views */
		case PST_5_VSN:	    /* only used for old distributed views */
		case PST_8_VSN:
		case PST_9_VSN:
		case PST_CUR_VSN:
		{
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &pst_qnode->pst_sym.pst_value, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    if (pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt)
		    {   /* a null terminated text string exists so read this
			** since it will be useful when recreating the query
			** for distributed */
			DB_TEXT_STRING	    stringsize; /* use this to store
							** the length of the 
							** text string */
			char		    *textptr;	/* temp for string 
							** text */

                            /* read the length of the text string representing
                            ** the constant */
			status = rdr_tuple(global, 
					(i4)sizeof(stringsize.db_t_count), 
					(PTR) &stringsize.db_t_count, tstate);
			if (DB_FAILURE_MACRO(status))
			    return (status);
			status = rdu_malloc(global, 
					    (i4)(stringsize.db_t_count+1),
					    (PTR *)&textptr);
			if (DB_FAILURE_MACRO(status))
			    return(status);
			/* read the text string */
			status = rdr_tuple(global, (i4)stringsize.db_t_count, 
					    (PTR)textptr, tstate);
			if (DB_FAILURE_MACRO(status))
			    return (status);
			textptr[stringsize.db_t_count] = 0; /* null terminate the string */
			pst_qnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt = textptr;
		    }
		    break;
		}
		default:
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		    return(status);
		}
		break;
	    }
	    case PST_RESDOM:
	    case PST_BYHEAD:
	    {   /* old versions of the query tree exist */
		switch (tstate->rdf_version)
		{
		/* versions 2, 3 and 5 required to support early versions of
		** star views.  However, all versions before PST_8_VSN are 
		** illegal for post 6.4 NON-DISTRIBUTED tree processing.
		*/
		case PST_4_VSN:
		case PST_6_VSN:
		case PST_7_VSN:
		    /* this version of tree is illegal in post 6.4 databases,
		    ** so raise a consistency check 
		    */
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		    return(status);
		case PST_2_VSN:
		{
		    /* only used for old distributed views */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_2rsdm_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = 
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno
			= tstate->rdf_2rsdm_node.pst_rsno;
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt
			= tstate->rdf_2rsdm_node.pst_rsupdt;
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags
			= (tstate->rdf_2rsdm_node.pst_print ? PST_RS_PRINT : 0);
		    MEcopy((PTR)&pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0],
			sizeof(pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			(PTR)&tstate->rdf_2rsdm_node.pst_rsname[0]);
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;
		    if (pst_qnode->pst_sym.pst_dataval.db_data)
			pst_qnode->pst_sym.pst_len = write_symsize 
			    + pst_qnode->pst_sym.pst_dataval.db_length;
		    else
			pst_qnode->pst_sym.pst_len = write_symsize;
		    break;
		}
		case PST_3_VSN:
		case PST_5_VSN:
		{
		    /* only used for old distributed views */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_7rsdm_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    /* zero out the new fields */
		    MEfill(sizeof(*pst_qnode), (u_char)0, (PTR)pst_qnode);
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = 
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno
			= tstate->rdf_7rsdm_node.pst_rsno;
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype
			= tstate->rdf_7rsdm_node.pst_ttargtype;
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt
			= tstate->rdf_7rsdm_node.pst_rsupdt;
		    pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags
			= (tstate->rdf_7rsdm_node.pst_print ? PST_RS_PRINT : 0);
		    MEcopy((PTR)&pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0],
			sizeof(pst_qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			(PTR)&tstate->rdf_7rsdm_node.pst_rsname[0]);
		    break;
		}
		case PST_8_VSN:
		case PST_9_VSN:
		case PST_CUR_VSN:
		{
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &pst_qnode->pst_sym.pst_value, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    break;
		}
		default:
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		    return(status);
		}
		break;
	    }
	    case PST_AND:
	    case PST_OR:
	    case PST_UOP:
	    case PST_BOP:
	    case PST_AOP:
	    case PST_COP:
	    case PST_MOP:
	    {   /* old versions of the query tree exist */
		switch (tstate->rdf_version)
		{
		/* versions 2, 3 and 5 required to support early versions of
		** star views.  However, all versions before PST_8_VSN are 
		** illegal for post 6.4 NON-DISTRIBUTED tree processing.
		*/
		case PST_4_VSN:
		case PST_6_VSN:
		case PST_7_VSN:
		    /* this version of tree is illegal in post 6.4 databases,
		    ** so raise a consistency check 
		    */
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
		    return(status);
		case PST_2_VSN:
		{
		    /* only used for old distributed views */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_2op_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opno
			= tstate->rdf_2op_node.pst_opno;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opinst
			= tstate->rdf_2op_node.pst_opinst;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_distinct
			= tstate->rdf_2op_node.pst_distinct;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_retnull
			= tstate->rdf_2op_node.pst_retnull;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid
			= tstate->rdf_2op_node.pst_oplcnvrtid;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid
			= tstate->rdf_2op_node.pst_oprcnvrtid;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opparm
			= tstate->rdf_2op_node.pst_opparm;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_fdesc
			= tstate->rdf_2op_node.pst_fdesc;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opmeta
			= (u_i2)tstate->rdf_2op_node.pst_opmeta;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_escape = 0;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags 
			= AD_PAT_DOESNT_APPLY;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_joinid
			= (PST_J_ID) PST_NOJOIN;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0;

		    if (pst_qnode->pst_sym.pst_dataval.db_data)
			pst_qnode->pst_sym.pst_len = write_symsize 
			    + pst_qnode->pst_sym.pst_dataval.db_length;
		    else
			pst_qnode->pst_sym.pst_len = write_symsize;
		    break;
		}
		case PST_3_VSN:
		case PST_5_VSN:
		{   
		    /* only used for old distributed views */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_3op_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opno
			= tstate->rdf_3op_node.pst_opno ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opinst
			= tstate->rdf_3op_node.pst_opinst ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_distinct
			= tstate->rdf_3op_node.pst_distinct ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_retnull
			= tstate->rdf_3op_node.pst_retnull ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid
			= tstate->rdf_3op_node.pst_oplcnvrtid ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid
			= tstate->rdf_3op_node.pst_oprcnvrtid ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opparm
			= tstate->rdf_3op_node.pst_opparm ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_fdesc
			= tstate->rdf_3op_node.pst_fdesc ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opmeta
			= (u_i2)tstate->rdf_3op_node.pst_opmeta ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_escape
			= tstate->rdf_3op_node.pst_escape ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags
			= tstate->rdf_3op_node.pst_isescape ;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_joinid
			= (PST_J_ID) PST_NOJOIN;
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0;

		    if (pst_qnode->pst_sym.pst_dataval.db_data)
			pst_qnode->pst_sym.pst_len = write_symsize 
			    + pst_qnode->pst_sym.pst_dataval.db_length;
		    else
			pst_qnode->pst_sym.pst_len = write_symsize;
		    break;
		}
		case PST_8_VSN:
		case PST_9_VSN:
		case PST_CUR_VSN:
		{
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &pst_qnode->pst_sym.pst_value, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    break;
		}
		default:
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD012E_QUERY_TREE);
		    return(status);
		}
		break;
	    }
	    case PST_AGHEAD:
	    case PST_ROOT:
	    case PST_SUBSEL:
	    {   /* old versions of the query tree exist */
		switch (tstate->rdf_version)
		{
		case PST_8_VSN:
		{
		    /* Preceding range table expansion. */
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &tstate->rdf_8rt_node, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_tvrc
			= tstate->rdf_8rt_node.pst_tvrc;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_lvrc
			= tstate->rdf_8rt_node.pst_lvrc;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_mask1
			= tstate->rdf_8rt_node.pst_mask1;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_qlang
			= tstate->rdf_8rt_node.pst_qlang;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_rtuser
			= tstate->rdf_8rt_node.pst_rtuser;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_project
			= tstate->rdf_8rt_node.pst_project;
		    pst_qnode->pst_sym.pst_value.pst_s_root.pst_dups
			= tstate->rdf_8rt_node.pst_dups;
		    STRUCT_ASSIGN_MACRO(tstate->rdf_8rt_node.pst_union,
			pst_qnode->pst_sym.pst_value.pst_s_root.pst_union);
		    /* Init. the range table bit mask fields. */
		    for (i = 0; i < PSV_MAPSIZE; i++)
		    {
			pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_tvrm.pst_j_mask[i] = 0;
			pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_lvrm.pst_j_mask[i] = 0;
			pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_rvrm.pst_j_mask[i] = 0;
		    }
		    /* Copy old range table maps to new fields. */
		    pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_tvrm.pst_j_mask[0]
			= tstate->rdf_8rt_node.pst_tvrm;
		    pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_lvrm.pst_j_mask[0]
			= tstate->rdf_8rt_node.pst_lvrm;
		    pst_qnode->pst_sym.pst_value.pst_s_root.
				pst_rvrm.pst_j_mask[0]
			= tstate->rdf_8rt_node.pst_rvrm;

		    if (pst_qnode->pst_sym.pst_dataval.db_data)
			pst_qnode->pst_sym.pst_len = write_symsize 
			    + pst_qnode->pst_sym.pst_dataval.db_length;
		    else
			pst_qnode->pst_sym.pst_len = write_symsize;
		    break;
		}
		case PST_9_VSN:
		case PST_CUR_VSN:
		{
		    status = rdr_tuple(global, (i4)read_symsize, 
			(PTR) &pst_qnode->pst_sym.pst_value, tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    break;
		}
		}
		break;
	    }
	    case PST_VAR:
	    case PST_RULEVAR:
	    case PST_QLEND:
	    case PST_TREE:
	    case PST_NOT:
	    case PST_CASEOP:
	    case PST_WHLIST:
	    case PST_WHOP:
	    case PST_OPERAND:
	    {
		status = rdr_tuple(global, (i4)read_symsize, 
		    (PTR) &pst_qnode->pst_sym.pst_value, tstate);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		break;
	    }
	    case PST_SORT:
	    case PST_CURVAL:
	    default:
		/* unexpected node type */
		rdu_ierror(global, E_DB_ERROR, E_RD0132_QUERY_TREE); 
		return(E_DB_ERROR);
	    }
	}

	pst_qnode->pst_left = NULL;
	pst_qnode->pst_right = NULL;


	/* if data exists, this field has the address of the values when
	** put into the tree catalog.
	*/
	if (pst_qnode->pst_sym.pst_dataval.db_data)
	{
	    /* allocate room for the data */
	    DB_DATA_VALUE	    *data;

	    data = &pst_qnode->pst_sym.pst_dataval;
	    status = rdu_malloc(global, (i4)data->db_length, 
		               (PTR *)&data->db_data);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    status = rdr_tuple(global, (i4)data->db_length, data->db_data, 
			       tstate);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}

	if ((pst_qnode->pst_sym.pst_type == PST_ROOT)
	    &&
	    (pst_qnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	   )
	{
	    status = rdr_tree(global,
		&pst_qnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next, 
		tstate);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	if ((children == RDF_RCHILD) || (children == RDF_LRCHILD))
	{
	    status = rdr_tree(global, &pst_qnode->pst_right, tstate);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	if ((children == RDF_LCHILD) || (children == RDF_LRCHILD))
	    next = &pst_qnode->pst_left;
	else
	    next = (PST_QNODE **) NULL;

	switch (pst_qnode->pst_sym.pst_type)
	{   /* update the address ptr to the ADF function instance descriptor */
	    case PST_UOP:
		if ((pst_qnode->pst_sym.pst_value.pst_s_op.pst_opno 
		     == ADI_EXIST_OP)
		    ||
		    (pst_qnode->pst_sym.pst_value.pst_s_op.pst_opno 
		    == ADI_NXIST_OP))
			break;		/* special case - function instance
					** does not exist so NULL is used */
	    case PST_BOP:
	    case PST_COP:
	    case PST_AOP:
	    case PST_MOP:
	    {

		status = adi_fidesc(tstate->adfcb,
		    pst_qnode->pst_sym.pst_value.pst_s_op.pst_opinst,
		    &pst_qnode->pst_sym.pst_value.pst_s_op.pst_fdesc);
		if (DB_FAILURE_MACRO(status))
		{	
		    status = adi_fidesc(tstate->adfcb, 
			pst_qnode->pst_sym.pst_value.pst_s_op.pst_opinst,
			&pst_qnode->pst_sym.pst_value.pst_s_op.pst_fdesc);
		    if (DB_FAILURE_MACRO(status))
		    {
			DB_ERROR	adferror;
			if (tstate->adfcb->adf_errcb.ad_errclass 
			    == ADF_USER_ERROR)
			{   /* report the error message to the user */
			    SETDBERR(&adferror, 0, 
					    tstate->adfcb->adf_errcb.ad_usererr);
			}
			else
			{   /* report non-user error code */
			    SETDBERR(&adferror, 0, 
					    tstate->adfcb->adf_errcb.ad_errcode);
			}
			rdu_ferror(global, status, &adferror,
				   E_RD0108_ADI_FIDESC,0);
		    }
		}
	    }
	    default:
		;
	}
    } /* end while loop */
    return (status);
}

/**
**	
** Name: rdu_rtree - Read the tree tuples and convert them to query tree nodes.
**
**	Internal call format:	rdu_rtree(rdf_cb, scf_cb, qeu, sys_infoblk)
**
** Description:
**      This function reads the tree tuples and convert them to query tree nodes.
**	
** Inputs:
**	rdf_cb
**	qeu
** Outputs:
**	rdf_cb
**	    .rdr_info_blk
**		.rdr_procedure
**		.rdr_protect
**		.rdr_integrity
**		.rdr_view
**	    .rdf_error
**		.err_code
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}	
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      15-Aug-86 (ac)    
**          written
**      10-nov-87 (seputis)
**          re-written for cache, resource management
**	20-jan-88 (stec)
**	    Commented out STRUCT_ASSIGN_MACRO for resloc. Old trees
**	    and new trees (muliple locations) have different resloc 
**	    definitions, so this file would not compile. In case of trees 
**	    retrieved from catalogs this info is not used anyway, therefore 
**	    removing the statement is alright.
**	09-mar-89 (neil)
**	    Minor changes for rules.
**	29-sep-89 (teg)
**	    added support for outter joins.
**	01-feb-90 (teg)
**	    added logic to put ptr to adfcb into tstate before calling rdr_tree.
**	12-feb-90   (teg)
**	    Removed old logic to deal with uncompressed trees, since they are
**	    no longer supported.
**	26-feb-91 (teresa)
**	    Add E_RD0146_ILLEGAL_TREEVSN consistency check.
**	    added support for PST_6_VSN (part of improved diagnostics for bug)
**	    35862
**	12-NOV-91 (teresa)
**	    Added support for verion 4, 5 & 6 trees
**	06-dec-91 (teresa)
**	    fix uninitialized rngentry for reading PST_6_VSN trees
**	27-jan-92 (teresa)
**	    add support of v8 trees for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    change interface to rdu_qopen
**	30-dec-92 (andre)
**	    restore PST_AGG_IN_OUTERMOST_SUBSEL and
**	    PST_GROUP_BY_IN_OUTERMOST_SUBSEL
**	31-dec-92 (andre)
**	    restore PST_SINGLETON_SUBSEL
**	20-aug-93 (teresa)
**	    Simplified handling of pre-current release trees according to the
**	    following:	
**		Star views support PST_2_VSN, PST_3_VSN, PST_5_VSN, PST_8_VSN
**		ALL Non-Distributed trees Support only PST_8_VSN.
**	    (At the time of this work, PST_CUR_VSN is PST_8_VSN.)  Therefore, if
**	    tree level is PST_0_VSN, PST_1_VSN, PST_4_VSN, PST_6_VSN or 
**	    PST_7_VSN, generate a consistency Check (E_RD0146_ILLEGAL_TREEVSN).
**	17-nov-93 (andre)
**	    restore PST_CORR_AGGR, PST_SUBSEL_IN_OR_TREE, PST_ALL_IN_TREE, and
**	    PST_MULT_CORR_ATTRS
**      16-mar-1998 (rigka01)
**	    Cross integrate change 434645 for bug 89105
**          Bug 89105 - clear up various access violations which occur when
**	    rdf memory is low.  In rdu_rtree(), add DB_FAILURE_MACRO(status)
**	    call after each rdu_malloc call for which it is missing
**	    (in CASE PST_CUR_VSN). 
**	17-jul-00 (hayke02)
**	    Set PST_INNER_OJ in pst_mask1 if RDF_INNER_OJ has been set in
**	    version.mask.
**	6-dec-02 (inkdo01)
**	    Support expanded range table (pst_inner/outer_rel are now bigger).
**	2-feb-06 (dougi)
**	    Replace pst_rngtree integer flag with PST_RNGTREE single bit flag.
**	20-july-06 (dougi)
**	    Read range table entries one at a time looking for derived tables. 
**	    Then read the derived table parse tree before the next range table
**	    entry.
**	29-dec-2006 (dougi)
**	    Add support for WITH list elements, too.
**	10-dec-2007 (smeke01) b118405
**	    Added pst_mask1 handling for 3 new view properties: PST_AGHEAD_MULTI, 
**	    PST_IFNULL_AGHEAD and PST_IFNULL_AGHEAD_MULTI.
**	20-may-2008 (dougi)
**	    Add support for table procedures (load parse trees of its parm
**	    specs).
**	21-nov-2008 (dougi) Bug 121265
**	    Small change to tolerate parameter-less TPROC invocations in views.
*/
static DB_STATUS
rdu_rtree(  RDF_GLOBAL	    *global,
	    PST_PROCEDURE   **qrytree,
	    i4              treetype)
{
    PST_PROCEDURE	*pst_procedure;
    RDF_TSTATE          tstate;
    DB_STATUS		status;
    RDF_TRTYPE		request;
    i4			ctr;	    /* a generic counter */
    struct
    {	/* Allocate the space for the retrieve output tuples, if allocating
	** space on the stack is a problem then RDF_TUPLE_RETRIEVE can be
        ** made equal to 1 */

	QEF_DATA           qefdata[RDF_TUPLE_RETRIEVE]; /* allocate space for
					    ** temporary qefdata elements */
	DB_IITREE	    buffer[2];	    /* this is a large tuple so use
                                            ** a buffer of two for reading
                                            ** from DMF */
    }   buffer;

    request = (RDF_TRTYPE)treetype;			    /* need to make sure correct type
					    ** is used for keying */
    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
    status = rdu_qopen(global, RD_IITREE, (char *)&request,
		       sizeof(DB_IITREE), (PTR)&buffer, sizeof(buffer));
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	status = rdd_streequery(global);

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    tstate.rdf_qdata = NULL;
    tstate.rdf_seqno = -1;	    /* used to keep track of sequence numbers
				    */
    tstate.rdf_eof = FALSE;         /* end of file indicator */
    tstate.rdf_treetype = treetype;
    {
	RDF_QTREE       version;

	/* get the RDF_QTREE from the catalog */
	status = rdr_tuple(global, sizeof(version), (PTR)&version, &tstate);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	switch (version.vsn)
	{
	case PST_1_VSN:
	case PST_0_VSN:
	{
	    /* obsolete or illegal versions */

	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0133_OLD_QUERY_TREE); /* these old
					** query trees are no longer supported */
	    return(status);		/* function instances may not exist
                                        ** so query trees must be converted */
	}

	/* versions 2, 3 and 5 required to support early versions of Distributed
	** views.  However, all versions before PST_8_VSN are illegal for post 
	** 6.4 NON-DISTRIBUTED tree processing.
	*/
	case PST_4_VSN:
	case PST_6_VSN:
	case PST_7_VSN:
	    /* this version of tree is illegal in post 6.4 databases,
	    ** so raise a consistency check 
	    */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0146_ILLEGAL_TREEVSN);
	    return(status);

	case PST_2_VSN:	    /* only used for old distributed views */
	case PST_3_VSN:	    /* only used for old distributed views */
	case PST_5_VSN:	    /* only used for old distributed views */
	case PST_8_VSN:
	case PST_9_VSN:
	case PST_CUR_VSN:
	{
	    /* there are multiple versions supported here.  The differences of
	    ** the early (pre PST_8_VSN) are as follows:
	    **  version 2,3 -- only elements in RDF_QNODE are mode, vsn,
	    **	    mask, resvno and range_size.  Also, if star, pst_mask1 is
	    **	    in rdf_qtree but has alignment bug.  If we are going from
	    **	    PST_3_VSN to PST_5_VSN, then need to use RDF_V3_QTREE to 
	    **	    fix alignment bug, which is done only for STAR.  (pst_mask1
	    **	    is guarenteed to be zeros for non-star version 2/3, so the
	    **	    same transformation to v5+ may be done on non-star dbs.)
	    **	version 5 -- Identical to version 3 except that pst_mask1 is
	    **	    moved down to a 4-byte boundary.  So, numjoins is added as
	    **	    a space holder before pst_mask1, but is not used by version
	    **	    6.3 STAR, which is the only version that produces version 5
	    **	    trees.  Therefore, RDF_QTREE contains mode, vsn, mask, 
	    **	    resvno, range_size, numjoins and pst_mask1.  IE, version
	    **	    4 and version 5 look alike, but pst_numjoins is always zero
	    **	    in version 5.
	    **	version 8 -- pst_inner/outer_rel bit maps in range table 
	    **	    entries are i4, rather than i4[4] (after range table
	    **	    expansion).
	    **
	    **  You will notice version specific logic in this section.  The
	    **	above description should help you to understand it.
	    */

	    PST_PROCEDURE   *procedure;
	    PST_STATEMENT   *statement;
	    PST_QTREE	    *qtree;
	    PST_RNGENTRY    **rng = NULL;	/* array of ptrs to structs*/
	    PST_RNGENTRY    *rngentry;		/* array of structs */
	    i4		    rangesize;

	    tstate.rdf_version = version.vsn; /* use version number to determine
					** how to read in nodes */
	    status = rdu_malloc(global, (i4)(sizeof(PST_QTREE) + 
		sizeof(PST_STATEMENT) + sizeof(PST_PROCEDURE)), (PTR *)&qtree);
					/* allocate memory for current version 
					** of query tree */
	    if (DB_FAILURE_MACRO(status))
	       return (status);
	    MEfill(sizeof(PST_QTREE), (u_char)0, (PTR)qtree);
	    statement = (PST_STATEMENT *) ((char *) qtree + 
						sizeof(PST_QTREE));
	    MEfill(sizeof(PST_STATEMENT), (u_char)0, (PTR)statement);
	    procedure = (PST_PROCEDURE *) ((char *) statement + 
						sizeof(PST_STATEMENT));	
	    MEfill(sizeof(PST_PROCEDURE), (u_char)0, (PTR)procedure);
	    rangesize = (sizeof(PST_RNGENTRY)+sizeof(PTR)) * version.range_size;
	    if (rangesize)
	    {
		/* allocate memory for array of range table and ptrs */
		status = rdu_malloc(global, rangesize, (PTR *)&rng);	
	        if (DB_FAILURE_MACRO(status))
	           return (status);
		MEfill(rangesize, (u_char)0, (PTR)rng);
	    }
	    procedure->pst_stmts = statement;
	    statement->pst_next = NULL;
	    statement->pst_type = PST_QT_TYPE;
	    statement->pst_specific.pst_tree = qtree;
	    statement->pst_link = NULL;
	    statement->pst_after_stmt = NULL;
	    statement->pst_mode = version.mode;
	    pst_procedure = procedure;

	    /* first, synch up earlier versions in the "version" or TREE header
	    ** structure */
	    if (version.vsn <= PST_3_VSN)
	    {
		/* this is a case where the RDF_QTREE was changed to fix a 
		** distributed alignment bug. The change is that pst_mask1 is
		** moved down 2 bytes and pst_numjoins is introduced as a
		** placeholder. If this is a non-star 6.3, then this operation 
		** is a harmless no-op */

		RDF_V3_QTREE        *v3;
		RDF_QTREE           workversion;
		DB_TAB_ID	    basetab;

		v3 = (RDF_V3_QTREE *) &version;
		if (version.vsn >= PST_6_VSN)
		{
		    /* save table id from tree. Else use nulls */
		    basetab.db_tab_base = version.pst_1basetab_id;
		    basetab.db_tab_index = version.pst_2basetab_id;
		}
		else
		{
		    basetab.db_tab_base = (u_i4) 0;
		    basetab.db_tab_index = (u_i4) 0;
		}
		workversion.mode = v3->mode;
		workversion.vsn  = v3->vsn;
		workversion.mask = v3->mask;
		workversion.resvno = v3->resvno;
		workversion.range_size = v3->range_size;
		workversion.numjoins = 0;
		workversion.pst_mask1 = v3->pst_mask1;
		version.mode = workversion.mode;
		version.vsn = workversion.vsn;
		version.mask = workversion.mask;
		version.resvno = workversion.resvno;
		version.range_size = workversion.range_size;
		version.numjoins = workversion.numjoins;
		version.pst_mask1 = workversion.pst_mask1;
		version.pst_1basetab_id = basetab.db_tab_base;
		version.pst_2basetab_id = basetab.db_tab_index;
	    }
	    qtree->pst_mode = version.mode;
	    qtree->pst_restab.pst_resvno = version.resvno;
	    qtree->pst_vsn = PST_CUR_VSN;
	    qtree->pst_rngvar_count = version.range_size;
	    qtree->pst_agintree = (version.mask & RDF_AGINTREE) != 0;
	    qtree->pst_subintree = (version.mask & RDF_SUBINTREE) != 0;
	    qtree->pst_wchk = (version.mask & RDF_WCHK) != 0;
	    
	    if (version.mask & RDF_RNGTREE)
		qtree->pst_mask1 |= PST_RNGTREE;
	    if (version.mask & RDF_AGG_IN_OUTERMOST_SUBSEL)
		qtree->pst_mask1 |= PST_AGG_IN_OUTERMOST_SUBSEL;
	    if (version.mask & RDF_GROUP_BY_IN_OUTERMOST_SUBSEL)
		qtree->pst_mask1 |= PST_GROUP_BY_IN_OUTERMOST_SUBSEL;
	    if (version.mask & RDF_SINGLETON_SUBSEL)
		qtree->pst_mask1 |= PST_SINGLETON_SUBSEL;
	    if (version.mask & RDF_CORR_AGGR)
		qtree->pst_mask1 |= PST_CORR_AGGR;
	    if (version.mask & RDF_SUBSEL_IN_OR_TREE)
		qtree->pst_mask1 |= PST_SUBSEL_IN_OR_TREE;
	    if (version.mask & RDF_ALL_IN_TREE)
		qtree->pst_mask1 |= PST_ALL_IN_TREE;
	    if (version.mask & RDF_MULT_CORR_ATTRS)
		qtree->pst_mask1 |= PST_MULT_CORR_ATTRS;
	    if (version.mask & RDF_INNER_OJ)
		qtree->pst_mask1 |= PST_INNER_OJ;
	    if (version.mask & RDF_AGHEAD_MULTI)
		qtree->pst_mask1 |= PST_AGHEAD_MULTI;
	    if (version.mask & RDF_IFNULL_AGHEAD)
		qtree->pst_mask1 |= PST_IFNULL_AGHEAD;
	    if (version.mask & RDF_IFNULL_AGHEAD_MULTI)
		qtree->pst_mask1 |= PST_IFNULL_AGHEAD_MULTI;

		
	    qtree->pst_updtmode = PST_UNSPECIFIED;
	    qtree->pst_basetab_id.db_tab_base = version.pst_1basetab_id;
	    qtree->pst_basetab_id.db_tab_index = version.pst_2basetab_id;
	    qtree->pst_distr = (PST_DSTATE *) NULL;	/* fix_me: this may
							** need to change for
							** star trees. If so,
							** then we will need
							** to pack a value away
							** and unpack it into 
							** tstate. */

	    /*  numjoins is only present in version 4 or versions after 
	    **  PST_6_VSN. , if this tree version > PST_6_VSN, get number of
	    **  joins in this tree.  Otherwise, initialize it to something 
	    **  harmless.  Since only versions 2,3,5 and 8 (or above) are still
	    **  supported, this simplifies to: if below 8 use PST_NOJOIN and if
	    **	8 or above, use version.numjoins.
	    */ 
	    qtree->pst_numjoins = (version.vsn >= PST_8_VSN) ?
				    (PST_J_ID) version.numjoins : 
				    (PST_J_ID) PST_NOJOIN;

	    if (version.vsn <= PST_3_VSN)
	    {
		    /* need to add pst_inner_rel and pst_outer_rel and
		    ** pst_dd_obj_type.  Also, change the static array of 30 
		    ** range table entries into a dynamic array of pointers to 
		    ** range table entries.*/
		    
		    PST_3RNGTAB	pst_3qrange;
		    i4	    ctr;

		    /* version_3 and below (and version 5) had a max of PST_3NUMVARS [30]
		    ** entries.  Before we read this, assure that we are not
		    ** being asked to read more than PST_3NUMVARS entries.
		    */
		    if (version.range_size > PST_3NUMVARS)
		    {
			/* print a consistency check message and return
			** an error */
		        status = E_DB_ERROR;
		        rdu_ierror(global, status, E_RD013C_BAD_QRYTRE_RNGSIZ);
		        return(status);
		    }
		    else if (version.range_size == 0)
			break;

		    status = rdr_tuple(global, 
			(i4)(sizeof(pst_3qrange[0]) * version.range_size),
			(PTR)&pst_3qrange[0], &tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);

		    /*
		    ** calculate addr in allocated block where array
		    ** of structs starts
		    */
		    rngentry = (PST_RNGENTRY *)(rng + version.range_size);  

		    /* now loop for each entry in Range table */
		    for (ctr = 0; ctr < version.range_size; ctr++)
		    {
			/* copy each element from the old struct to the 
			** current Querry Tree Range Table entry.  Also,
			** update ptr to that struct for each element.
			*/
			rng[ctr] = rngentry+ctr;
			rngentry[ctr].pst_rgtype
					    = pst_3qrange[ctr].pst_rgtype;
			rngentry[ctr].pst_rgroot 
					    = pst_3qrange[ctr].pst_rgroot;
			rngentry[ctr].pst_rgparent
					    = pst_3qrange[ctr].pst_rgparent;
			rngentry[ctr].pst_dd_obj_type = DD_0OBJ_NONE;
			STRUCT_ASSIGN_MACRO( pst_3qrange[ctr].pst_rngvar,
					    rngentry[ctr].pst_rngvar);
			STRUCT_ASSIGN_MACRO( pst_3qrange[ctr].pst_timestamp,
					    rngentry[ctr].pst_timestamp);
			MEfill( sizeof(DB_TAB_NAME), (u_char)' ',
				(PTR)rngentry[ctr].pst_corr_name.db_tab_name);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_inner_rel);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_outer_rel);
		    }  /* end loop for each entry in query tree range table */
		    qtree->pst_rangetab = rng;
	    }
	    else if (version.vsn == PST_5_VSN)
	    {
		    /* need to add pst_inner_rel and pst_outer_rel and
		    ** pst_dd_obj_type.  Also, change the static array of 30 
		    ** range table entries into a dynamic array of pointers to 
		    ** range table entries.  */
		    
		    PST_5RNGTAB	pst_5qrange;
		    i4	    ctr;

		    /* version 5 had a max of PST_5NUMVARS [30]
		    ** entries.  Before we read this, assure that we are not
		    ** being asked to read more than PST_5NUMVARS entries.
		    */
		    if (version.range_size > PST_5NUMVARS)
		    {
			/* print a consistency check message and return
			** an error */
		        status = E_DB_ERROR;
		        rdu_ierror(global, status, E_RD013C_BAD_QRYTRE_RNGSIZ);
		        return(status);
		    }
		    else if (version.range_size == 0)
			break;

		    status = rdr_tuple(global, 
			(i4)(sizeof(pst_5qrange[0]) * version.range_size),
			(PTR)&pst_5qrange[0], &tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);

		    /*
		    ** calculate addr in allocated block where array
		    ** of structs starts
		    */
		    rngentry = (PST_RNGENTRY *)(rng + version.range_size);  

		    /* now loop for each entry in Range table */
		    for (ctr = 0; ctr < version.range_size; ctr++)
		    {
			/* copy each element from the old struct to the 
			** current Querry Tree Range Table entry.  Also,
			** update ptr to that struct for each element.
			*/
			rng[ctr] = rngentry+ctr;
			rngentry[ctr].pst_rgtype
					    = pst_5qrange[ctr].pst_rgtype;
			rngentry[ctr].pst_rgroot 
					    = pst_5qrange[ctr].pst_rgroot;
			rngentry[ctr].pst_rgparent
					    = pst_5qrange[ctr].pst_rgparent;
			rngentry[ctr].pst_dd_obj_type 
					    = pst_5qrange[ctr].pst_dd_obj_type;
			STRUCT_ASSIGN_MACRO( pst_5qrange[ctr].pst_rngvar,
					    rngentry[ctr].pst_rngvar);
			STRUCT_ASSIGN_MACRO( pst_5qrange[ctr].pst_timestamp,
					    rngentry[ctr].pst_timestamp);
			MEfill( sizeof(DB_TAB_NAME), (u_char)' ',
				(PTR)rngentry[ctr].pst_corr_name.db_tab_name);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_inner_rel);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_outer_rel);
		    }  /* end loop for each entry in query tree range table */
		    qtree->pst_rangetab = rng;
	    }
	    else if (version.vsn == PST_8_VSN)
	    {
		    /* need to expand pst_inner_rel and pst_outer_rel for range
		    ** table expansion.  */
		    
		    PST_8RNGTAB	pst_8qrange;
		    i4	    ctr;

		    /* version 8 had a max of PST_8NUMVARS [32]
		    ** entries.  Before we read this, assure that we are not
		    ** being asked to read more than PST_8NUMVARS entries.
		    */
		    if (version.range_size > PST_8NUMVARS)
		    {
			/* print a consistency check message and return
			** an error */
		        status = E_DB_ERROR;
		        rdu_ierror(global, status, E_RD013C_BAD_QRYTRE_RNGSIZ);
		        return(status);
		    }
		    else if (version.range_size == 0)
			break;

		    status = rdr_tuple(global, 
			(i4)(sizeof(pst_8qrange[0]) * version.range_size),
			(PTR)&pst_8qrange[0], &tstate);
		    if (DB_FAILURE_MACRO(status))
			return (status);

		    /*
		    ** calculate addr in allocated block where array
		    ** of structs starts
		    */
		    rngentry = (PST_RNGENTRY *)(rng + version.range_size);  

		    /* now loop for each entry in Range table */
		    for (ctr = 0; ctr < version.range_size; ctr++)
		    {
			/* copy each element from the old struct to the 
			** current Querry Tree Range Table entry.  Also,
			** update ptr to that struct for each element.
			*/
			rng[ctr] = rngentry+ctr;
			rngentry[ctr].pst_rgtype
					    = pst_8qrange[ctr].pst_rgtype;
			rngentry[ctr].pst_rgroot 
					    = pst_8qrange[ctr].pst_rgroot;
			rngentry[ctr].pst_rgparent
					    = pst_8qrange[ctr].pst_rgparent;
			rngentry[ctr].pst_dd_obj_type 
					    = pst_8qrange[ctr].pst_dd_obj_type;
			STRUCT_ASSIGN_MACRO( pst_8qrange[ctr].pst_rngvar,
					    rngentry[ctr].pst_rngvar);
			STRUCT_ASSIGN_MACRO( pst_8qrange[ctr].pst_timestamp,
					    rngentry[ctr].pst_timestamp);
			MEcopy( (PTR)pst_8qrange[ctr].pst_corr_name.db_tab_name,
				sizeof(DB_TAB_NAME),
				(PTR)rngentry[ctr].pst_corr_name.db_tab_name);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_inner_rel);
			MEfill( sizeof(PST_J_MASK),'\0', &rngentry[ctr].pst_outer_rel);
			rngentry[ctr].pst_inner_rel.pst_j_mask[0]
					    = pst_8qrange[ctr].pst_inner_rel;
							/* copy old mask to 1st word */
			rngentry[ctr].pst_outer_rel.pst_j_mask[0]
					    = pst_8qrange[ctr].pst_outer_rel;
		    }  /* end loop for each entry in query tree range table */
		    qtree->pst_rangetab = rng;
	    }
	    else /* PST_9_VSN or above */
	    {
		if (version.range_size > 0)
		{
		    /* the usual case, the current Query Tree Range Table.  This
		    ** consists of pst_rangetab, which is a pointer to an array
		    ** of pointers to PST_RNGENTRY structs.  So, this is a three
		    ** step process:  1) copy the QT Range Table into an array
		    ** of structure to contain it.  2) initialize the array of
		    ** pointers to point to each element in structure.  3) put 
		    ** address of array of ptrs in qtree
		    */

		    /*
		    ** calculate addr in allocated block where array
		    ** of structs starts
		    */
		    rngentry = (PST_RNGENTRY *)(rng + version.range_size);  

		    rangesize = sizeof(PST_RNGENTRY);
		    for (ctr = 0; ctr < version.range_size; ctr++)
		    {   
			/* initialize array of ptrs so that each element in ptr
			** array points to an element in the array of structures */
			status = rdr_tuple(global, rangesize, (PTR)rngentry,
					&tstate); /* read range var into array*/
			if (DB_FAILURE_MACRO(status))
			    return (status);

			/* If this is a derived table, we need to load its
			** parse tree now. */
			if (rngentry->pst_rgtype == PST_DRTREE ||
				rngentry->pst_rgtype == PST_WETREE ||
				rngentry->pst_rgtype == PST_TPROC)
			{
			    status = rdr_tree(global, &rngentry->pst_rgroot, 
				&tstate);
			    if (DB_FAILURE_MACRO(status))
				return(status);
			}

			/* Reset parameter-less TPROC if there was one. */
			if (rngentry->pst_rgtype == PST_TPROC_NOPARMS)
			    rngentry->pst_rgtype = PST_TPROC;

			rng[ctr] = rngentry;	
			rngentry++;
		    }
		    qtree->pst_rangetab = rng;
		}
	    }
	    break;
	}
        default:
            {
                /* the version field is garbage, so the whole tree is 
                ** unreadable */
                status = E_DB_ERROR;
                rdu_ierror(global, status, E_RD012E_QUERY_TREE);
                return(status);
            }
	} /* endcase */
    }

    /* 
    ** get the tree from the catalog 
    */
    /* initialize tstate->adf_cb with ADF control block ptr for rdr_tree */
	
    tstate.adfcb = (ADF_CB*)global->rdf_sess_cb->rds_adf_cb;

    status = rdr_tree(global, 
		    &pst_procedure->pst_stmts->pst_specific.pst_tree->pst_qtree,
		    &tstate);
    if (DB_SUCCESS_MACRO(status))
	*qrytree = pst_procedure;	    /* return root if retrieval
					    ** was successful */
    return(status);
}

/*{
** Name: rdu_view	- create descriptor for view tree
**
** Description:
**      Create a descriptor and read the parse tree for a view
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-nov-87 (seputis)
**          initial creation
**	01-feb-91 (teresa)
**	    improve statistics reporting.
**      22-may-92 (teresa)
**          added new parameter to rdu_xlock call for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	29-dec-92 (teresa)
**	    fix bug 48380, where RDF held a semaphore across QEF/DMF calls when
**	    reading a view.
**	12-mar-93 (teresa)
**	    moved rdf_shared_sessions increment logic out of rdu_xlock call.
**	11-may-1993 (teresa)
**	    fix bug where rdu_rtree() error status lost by retaking semaphore
**	    before checking status.
**      11-nov-1994 (cohmi01) checksum sys block instead of usr block.
**	15-Mar-2004 (schka24)
**	    Mutex-protect sysinfo block status updates.
**	30-Jul-2006 (kschendel)
**	    rdu-xlock isn't careful enough with the object mutex yet, so
**	    we have to be smarter about updating a user infoblk from system
**	    after xlock.
[@history_template@]...
*/
static DB_STATUS
rdu_view(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    RDD_VIEW		*viewp;
    RDR_INFO		*sys_infoblk;
    RDR_INFO		*usr_infoblk;
    RDF_CB		*rdfcb;

    if (!(global->rdfcb->rdf_rb.rdr_types_mask & RDR_QTREE))
    {   /* Consistency check, always expect a query tree for query mod */
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
    rdfcb = global->rdfcb;
    usr_infoblk = rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    viewp = usr_infoblk->rdr_view;
    if (viewp)
    {	/* view is already in cache */
	Rdi_svcb->rdvstat.rds_c4_view++;
	return(E_DB_OK);
    }
    status = rdu_xlock(global, &usr_infoblk);   /* reserve the object for 
					    ** update */
    if (DB_FAILURE_MACRO(status))
	return(status);
    /* It's possible (if unlikely) that the system copy will be updated
    ** in time for rdu-private to copy it to a newly created private
    ** info block.  If not, we will read our own copy of the view tree
    ** if we have a private info block.
    ** If we're still using the system one, see if it was filled in
    ** by someone else.
    ** Note that xlock returns holding the object mutex in the latter
    ** case.
    */
    if ( (usr_infoblk == sys_infoblk
	  && sys_infoblk->rdr_view != NULL && (sys_infoblk->rdr_types & RDR_VIEW))
      || (usr_infoblk != sys_infoblk
	  && usr_infoblk->rdr_view != NULL && (usr_infoblk->rdr_types & RDR_VIEW)) )
    {
	Rdi_svcb->rdvstat.rds_c4_view++;
	return(E_DB_OK);
    }
    status = rdu_malloc(global, (i4)sizeof(*viewp), (PTR *)&viewp);
    if (DB_FAILURE_MACRO(status))
	return(status);
    viewp->qry_root_node = NULL;

    /* In distributed view, we need to allocate buffers for binding 
    ** binding information and return tree tuple at higher level. This is
    ** because RQF stores tuple descriptor at caller's memory space and
    ** expects caller to return the same descriptor at subsequence calls.
    ** QEF should pass the binding pointer directly to RQF without any
    ** modifications. */
    
    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
    	RQB_BIND	rq_bind[RDD_08_COL]; /* 8 columns for
    					     ** distributed view 
    					     ** binding information */
    	RDC_TREE	treebuf;	     /* tree tuple buffer */
    
    	global->rdf_ddrequests.rdd_types_mask = RDD_VTUPLE;
    	global->rdf_ddrequests.rdd_bindp = rq_bind;
    	global->rdf_ddrequests.rdd_ttuple = &treebuf;
     
	/* fix_me: note: when we do sybil phase III, it will not be ok to hold
	**	   a semaphore across QEF calls, and this routine calls QEF.
	**	   we should release the semaphore before the call and retake
	**	   it afterwards.  I'm not putting that in now cuz I'm not sure
	**	   if there are any nasty sideeffects and its past code freeze,
	**	   so I dont want to destbilize the code -- teresa
	*/
     	status = rdu_rtree(global, (PST_PROCEDURE **)&viewp->qry_root_node, 
			   DB_VIEW);
    	
     }
     else
     {
	/* release the query tree semaphore */
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	status = rdu_rtree(global, (PST_PROCEDURE **)&viewp->qry_root_node, 
		           DB_VIEW);
	if (DB_FAILURE_MACRO(status))
	    return(status);

    }
    if (DB_FAILURE_MACRO(status))
	return(status);
    if (global->rdf_resources & RDF_RUPDATE)
    {	/* if the system stream has been reserved for update then save this
        ** descriptor in the system cache, otherwise memory was allocated in
        ** the private stream so it cannot be used in the system cache */
	if (sys_infoblk->rdr_view || (sys_infoblk->rdr_types & RDR_VIEW))
	{   /* consistency check - this should not be executed */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0111_VIEW);
	    return(status);	    
	}
	/* retake the query tree semaphore to update state */
	status = rdu_gsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	sys_infoblk->rdr_view = viewp;
	sys_infoblk->rdr_types |= RDR_VIEW;
	status = rdu_rsemaphore(global);
	Rdi_svcb->rdvstat.rds_b4_view++;
	return(status);
    }
    usr_infoblk->rdr_view = viewp;
    usr_infoblk->rdr_types |= RDR_VIEW;
    Rdi_svcb->rdvstat.rds_b4_view++;
    return(status);
}

/*{
** Name: rdu_qtclass	- get class name for query tree hash table
**
** Description:
**      Get the query tree class name for the ULH hash table ID.
**	This name is comprised of:
**		unique table identifier (i4)
**		tree_type (i4 - DB_RULE, DB_PROTECT, DB_INTG)
**		database id (i4)
**
** Inputs:
**      global                          ptr to RDF global state variable
**	    rdrcb			RDF Control block
**		rdf_rb			    RDF Request block
**		    rdr_unique_dbid		    unique table identifier
**		    rdr_treebase_id		    table id for base table
**	id				Unique table identifier (optional)
**	tree_type			tree type: DB_PROT = permit
**						   DB_INTG = integrity
**						   DB_RULE = rule
**	
** Outputs:
**      *name		    class name to be used by ULH
**	*len		    number of characters in *name
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-mar-92 (teresa)
**          initial creation for SYBIL
**	16-jul-92 (teresa)
**	    prototypes
**	19-jul-2002 (devjo01)
**	    Routine would leave gaps of uninitialized memory on 
**	    platforms where ptrs are larger than i4's.  Also
**	    changed dubious logic where ONLY db_tab_index was used
**	    if non-zero and 'id' NULL.   Instead, always use both the
**	    base table id and the index id, faking a zero index if 'id'
**	    is passed.
**	6-Jul-2006 (kschendel)
**	    Assume that the unique dbid is there, can't substitute the
**	    dmf db id which is completely different anyway.
[@history_template@]...
*/
VOID
rdu_qtclass(	RDF_GLOBAL  *global,
		u_i4	    id,
		i4	    tree_type,
		char	    *name,
		i4	    *len)
{
    DB_TAB_ID	    temp, *id_ptr;
    RDF_TRTYPE	    type;
    char	    *class_name = name;

    if (id)
    {
	temp.db_tab_base = id;
	temp.db_tab_index = 0;
	id_ptr = &temp;
    }
    else
    {
	id_ptr = &global->rdfcb->rdf_rb.rdr_tabid;
    }

    MEcopy( (PTR)id_ptr, sizeof(DB_TAB_ID), (PTR)class_name);
    class_name += sizeof(DB_TAB_ID);
    type = (RDF_TRTYPE)tree_type;
    MEcopy( (PTR)&type, sizeof(type), (PTR)class_name);
    class_name += sizeof(type);
    MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid, 
	sizeof(global->rdfcb->rdf_rb.rdr_unique_dbid), 
	(PTR)class_name);
    *len = sizeof(DB_TAB_ID) + sizeof(type) + 
      sizeof(global->rdfcb->rdf_rb.rdr_unique_dbid);
}

/*{
** Name: rdu_qtalias 	- get alias name for query tree hash table object
**
** Description:
**      Get the alias name for this query tree cache object.
**	This name is comprised of:
**	    for permit/integrity
**		unique table identifier (i4)
**		sequence number (i4)
**		tree_type (i4 - DB_RULE, DB_PROTECT, DB_INTG)
**		database id (i4)
**	    for rules:
**		rule name   (DB_RULE_MAXNAME)
**		rule owner  (DB_OWN_MAXNAME)
**		database id (i4)
**
** Inputs:
**      global                          ptr to RDF global state variable
**	    rdrcb			RDF Control Block
**		rdf_rb			    RDF Request Block
**		    rdr_unique_dbid		unique table identifier
**		    rdr_treebase_id		table id for table receiving
**						    permit or integrity
**		    rdr_sequence		sequence number for permit/
**						    integrity
**		    rdr_name.rdr_rlname		rule name
**		    rdr_owner			rule owner
**	tree_type				tree type: DB_PROT = permit
**							   DB_INTG = integrity
** Outputs:
**      *name		    class name to be used by ULH
**	*len		    number of characters in *name
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-mar-92 (teresa)
**          initial creation for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
VOID
rdu_qtalias(	RDF_GLOBAL  *global,
		i4	    tree_type,
		char	    *name,
		i4	    *len)
{
    RDF_TRTYPE	    type;
    char	    *alias_name = name;
    PTR		    id_ptr;

    if ( (tree_type == DB_INTG) || (tree_type == DB_PROT) )
    {
	/* this is an integrity or procedure */
	/* (schka24) I am pretty sure that this test for base-or-index
	** is formulaic;  one doesn't put integrities on indexes.
	** For darn sure they don't go on physical partitions.
	** It's also going to be wrong for sequences (index is a version).
	*/

	if (global->rdfcb->rdf_rb.rdr_tabid.db_tab_index > 0)
	    id_ptr = (PTR) &global->rdfcb->rdf_rb.rdr_tabid.db_tab_index;
	else
	    id_ptr = (PTR) &global->rdfcb->rdf_rb.rdr_tabid.db_tab_base;

	MEcopy (id_ptr, sizeof(PTR), (PTR) alias_name);
	alias_name += sizeof(PTR);
	*len = sizeof(PTR);

	type = (RDF_TRTYPE) tree_type;
	MEcopy((PTR)&type, sizeof(type), (PTR)alias_name);
	alias_name += sizeof(type);
	*len += sizeof(type);
	MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid, 
	    sizeof(i4), 
	    (PTR)alias_name);
	alias_name += sizeof(i4);
	*len += sizeof(i4);
	/* FIXME this wasn't doing anything because len isn't adjusted,
	** is this right or wrong or what?
	**MEcopy ((PTR) &global->rdfcb->rdf_rb.rdr_sequence,
	**	sizeof(global->rdfcb->rdf_rb.rdr_sequence),
	**	(PTR) alias_name);
	*/
    }
    else
    {
	/* this is a rule */
	MEcopy ((PTR) &global->rdfcb->rdf_rb.rdr_name, 
		DB_RULE_MAXNAME, (PTR) alias_name);
	alias_name += DB_RULE_MAXNAME;
	MEcopy ((PTR) &global->rdfcb->rdf_rb.rdr_owner,
		DB_OWN_MAXNAME, (PTR) alias_name);
	alias_name += DB_OWN_MAXNAME;
	MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid, 
	    sizeof(i4),
	    (PTR)alias_name);
	*len = DB_RULE_MAXNAME + DB_OWN_MAXNAME + sizeof(i4);

    }	/* endif */
}

/*{
** Name: rdu_qtname	- get object name for query tree hash table
**
** Description:
**      Get the query tree object name for the ULH hash table ID
**
** Inputs:
**      global                          ptr to RDF global state variable
**
** Outputs:
**      global->rdf_objname             object name to be used by ULH
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-nov-87 (seputis)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
static VOID
rdu_qtname( RDF_GLOBAL	*global,
	    i4		catalog)
{
    char	    *obj_name;	    /* ptr to ULH object name to be built */
    RDF_TRTYPE	    tree_type;
    
    tree_type = (RDF_TRTYPE) catalog;
    obj_name = (char *)&global->rdf_objname[0];
    /* DB_QRY_ID and DB_TREE_ID are both time stamps so this approach should
    ** work, currently DB_PROC will always request a text object only */
    switch (catalog)
    {
	case DB_DBP:
	{
	    MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_qtext_id, 
		sizeof(DB_QRY_ID), (PTR)obj_name);
	    obj_name += sizeof(DB_QRY_ID);
	    global->rdf_namelen = sizeof(DB_QRY_ID) + sizeof(i4) 
		+ sizeof(tree_type);
	    break;
	}
	default:
	{
	    MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_tree_id, 
		sizeof(DB_TREE_ID), (PTR)obj_name);
	    obj_name += sizeof(DB_TREE_ID);
	    global->rdf_namelen = sizeof(DB_TREE_ID) + sizeof(i4) 
		+ sizeof(tree_type);
	}
    }
    MEcopy((PTR)&tree_type, sizeof(tree_type), (PTR)obj_name);
    obj_name += sizeof(tree_type);
    MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid, 
	sizeof(i4),
	(PTR)obj_name);
    global->rdf_init |= RDF_INAME;	    /* indicates that the ULH name
                                            ** has been initialized */
}

/*{
** Name: rdu_qtree	- lookup or create the query mod tree
**
** Description:
**      This routine will lookup or create the query mod tree.  Note that
**      all query trees are put into the same memory stream as the table
**      descriptor, this may cause out of memory problems with user who
**      have lots of query mod information.  This can be fixed by creating
**      a separate stream for each query tree and managing usage counts OR
**      by using ULH and inserting the object by tree ID, since ULH manages
**      usage counts already.
**
** Inputs:
**      global                          ptr to RDF state variable
**      usr_qmod                        ptr to ptr to user linked list of
**                                      query mod objects
**      sys_qmod                        ptr to ptr to system linked list of
**                                      query mod objects
**      catalog                         either DB_PROT or DB_INTG or DB_RULE
**
** Outputs:
**	finds				ctr incremented if qtree found on cache
**	builds				ctr incremented if qtree is built and
**					 put on cache.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-nov-87 (seputis)
**          initial creation
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	15-dec-89 (teg)
**	    get RDF SVCB address from global instead of from GLOBALDEF Rdi_svcb
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	01-feb-91 (teresa)
**	    improve rdf statistics
**	23-jan-92 (teresa)
**	    SYBIL changes: rdi_poolid -> rdv_poolid; rdi_qthashid->rdv_qtree_hashid;
**			   rdv_c1_memleft->rdv_memleft
**	06-mar-92 (teresa)
**	    assure that semaphore is NOT held across QEF/DMF calls by releasing
**	    it before the call to rdu_rtree and retaking it afterwards to
**	    update rdf_qtulhobject->rdf_qtresource.  (Fixes bug 42922.)
**	12-mar-92 (teresa)
**	    changed to use ulh_cre_class instead of ulh_create.  Also build
**	    alias to tree object.
**	08-jun-92 (teresa)
**	    implemented rdr2_refresh logic for sybil.
**	16-jul-92 (teresa)
**	    prototypes
**	09-nov-92 (rganski)
**	    Put comment markers around "unterminated character constant"
**	21-may-93 (teresa)
**	    Fixed typecasts for calls to recently prototyped ulf routines.
[@history_template@]...
*/
static DB_STATUS
rdu_qtree(  RDF_GLOBAL	    *global,
	    RDD_QRYMOD	    **rddqrymodp,
	    RDF_SVAR	    *builds,
	    RDF_SVAR	    *finds,
	    i4		    catalog,
	    RDF_TYPES	    rdftypes,
	    bool	    *refreshed)
{
    DB_STATUS	status;
    RDD_QRYMOD  *rdf_qtulhobject;
    char	classname[ULH_MAXNAME];
    char	aliasname[ULH_MAXNAME];
    i4		class_len=0;
    i4		alias_len=0;

    if((    (global->rdfcb->rdf_rb.rdr_tree_id.db_tre_high_time == 0) 
	    &&
	    (global->rdfcb->rdf_rb.rdr_tree_id.db_tre_low_time == 0))
	||
	    !(global->rdfcb->rdf_rb.rdr_types_mask & RDR_QTREE)
       )
    {
	/*
	** Error if requesting tree information without a tree id on any 
	** object other than a view.  Note: Views don't have a tree id.
	*/
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
    /* Get the tree tuples and convert them to query tree nodes. */

    rdu_qtname(global, catalog);	    /* initialize the query tree name 
					    ** for the ULH object name */
    /* init class name */
    rdu_qtclass(global, 0, catalog, &classname[0], &class_len); 
    /* init alias name */
    rdu_qtalias(global, catalog, &aliasname[0], &alias_len); 

    /* obtain access to a query tree cache object */
    if (!(global->rdf_init & RDF_IULH))
    {	/* init the ULH control block if necessary, only need the server
        ** level hashid obtained at server startup */
	global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	global->rdf_init |= RDF_IULH;
    }
    status = ulh_cre_class(&global->rdf_ulhcb, (unsigned char *)classname, 
			   class_len, (unsigned char *)&global->rdf_objname[0], 
			   global->rdf_namelen, ULH_DESTROYABLE, 0,
			   (i4) global->rdfcb->rdf_rb.rdr_sequence);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
	    E_RD0045_ULH_ACCESS,0);
	return (status);		/* some other ULH error occurred so
					** report it and return */
    }
    else
    {
	/* update statistics */
	switch (rdftypes)
	{
	case RDR_INTEGRITIES:
	    Rdi_svcb->rdvstat.rds_f1_integrity++;
	    break;
	case RDR_RULE:
	    Rdi_svcb->rdvstat.rds_f4_rule++;
	    break;
	case RDR_PROTECT:
	    Rdi_svcb->rdvstat.rds_f3_protect++;
	    break;
	default:
	    break;
	}
#if 0
/*
    -- we can't decide if we want this logic for QTREE cache objects, so we're
    -- leaving the code incase we want to use it again.  If we refresh QTREE
    -- cache obejcts when we don't need to, this slows things down.  Right now
    -- we're not sure of any instances where we need to, but they could come
    -- up as this new code gets exercised.  The ifdeffed out code has already
    -- been tested.
*/

	/* if user has requested RDF refresh the cache, see if the object
	** should be destroyed and recreated 
	*/
	rdf_qtulhobject = (RDD_QRYMOD *) global->rdf_ulhcb.ulh_object->ulh_uptr;

    	if ( (global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH)
	     &&
		rdf_qtulhobject 
	     && 
		rdf_qtulhobject->qry_root_node 
	   )
	{
	    /* invalidate this cache entry and behave as though 
	    ** the entry is not found
	    */
	    *refreshed =  TRUE;
	    status = ulh_destroy(&global->rdf_ulhcb, (unsigned char *) NULL, 0);
	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror( global, status, 
			    &global->rdf_ulhcb.ulh_error, 
			    E_RD0040_ULH_ERROR,0);
	    }		    
	    /* recreate entry -- this time should be an empty entry */
	    status = ulh_cre_class(&global->rdf_ulhcb, 
				   (unsigned char *)classname, class_len,
				   (unsigned char *)&global->rdf_objname[0], 
				   global->rdf_namelen, ULH_DESTROYABLE, 0,
				   (i4) global->rdfcb->rdf_rb.rdr_sequence);
	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		    E_RD0045_ULH_ACCESS,0);
		return (status);	/* some other ULH error occurred so
					** report it and return */
	    }
	    /* update statistics */
	    switch (rdftypes)
	    {
	    case RDR_INTEGRITIES:
		Rdi_svcb->rdvstat.rds_f1_integrity++;
		break;
	    case RDR_RULE:
		Rdi_svcb->rdvstat.rds_f4_rule++;
		break;
	    case RDR_PROTECT:
		Rdi_svcb->rdvstat.rds_f3_protect++;
		break;
	    default:
		break;
	    }
	}
#endif
    }
    global->rdf_resources |= RDF_RULH; /* mark ULH object has being fixed */
    /* we have to reget this because earlier processing may have called
    ** ulh_destroy for rdr2_refresh logic */
    rdf_qtulhobject = (RDD_QRYMOD *) global->rdf_ulhcb.ulh_object->ulh_uptr;

    if ( rdf_qtulhobject && rdf_qtulhobject->qry_root_node &&
	 (rdf_qtulhobject->rdf_qtresource & RDF_SSHARED)
       )
	++(*finds);		/* update stats that object found on cache */
    else
    {	/* object does not exist so it needs to be created */
	PST_PROCEDURE		*root;
	status = rdu_gsemaphore(global);  /* get the query tree semaphore */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	rdf_qtulhobject = (RDD_QRYMOD *) global->rdf_ulhcb.ulh_object->ulh_uptr;
	if (!rdf_qtulhobject)
	{   /* if query tree still doesn't exist while be read under semaphore
            ** protection then it needs to be created */
	    if (!(global->rdf_init & RDF_IULM))
	    {	/* init the ULM control block for memory allocation */
		RDI_FCB	    *rdi_fcb;		/* ptr to facility control block 
						** initialized at server startup time
						*/
		ULM_RCB	    *ulm_rcb;
		rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
		ulm_rcb = &global->rdf_ulmcb;
		ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
		ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
		ulm_rcb->ulm_facility = DB_RDF_ID;
		ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
		/* get the stream id from the ulh descriptor */
		ulm_rcb->ulm_streamid_p 
		    = &global->rdf_ulhcb.ulh_object->ulh_streamid;
		global->rdf_init |= RDF_IULM;
	    }
	    status = rdu_malloc(global, (i4)sizeof(*rdf_qtulhobject),
		(PTR *)&rdf_qtulhobject);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    rdf_qtulhobject->rdf_qtresource = RDF_SNOACCESS; /* mark object as
					    ** being created */
	    rdf_qtulhobject->qry_root_node = NULL;
	    rdf_qtulhobject->rdf_ulhptr = (PTR)global->rdf_ulhcb.ulh_object;
	    global->rdf_ulhcb.ulh_object->ulh_uptr = (PTR)rdf_qtulhobject; /*
					    ** place descriptor into cache */
	}
	if (!rdf_qtulhobject->qry_root_node)
	{
	    switch(rdf_qtulhobject->rdf_qtresource)
	    {
	    case RDF_SUPDATE:
	    {   /* the object is being updated by some other thread, so a
		** private copy needs to be made.  Once we have made a private
		** copy, it will not be visible to other threads, so do not
		** worry about further semaphore protection. */
		status = rdu_cstream(global); /* create a private stream which
					    ** will be deleted when the caller
					    ** is finished with the tree */
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = rdu_malloc(global, (i4)sizeof(*rdf_qtulhobject),
		    (PTR *)&rdf_qtulhobject);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rdf_qtulhobject->rdf_qtresource = RDF_SPRIVATE;
		rdf_qtulhobject->rdf_qtstreamid 
		    = global->rdf_ulmcb.ulm_streamid;
		/* release the query tree semaphore */
		status = rdu_rsemaphore(global);  
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = rdu_rtree(global, &root, (i4)catalog);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rdf_qtulhobject->qry_root_node = root;
		++(*builds);	/* increment ctr that RDf built this object */
		break;
	    }
	    case RDF_SNOACCESS:
	    {
		rdf_qtulhobject->rdf_qtresource = RDF_SUPDATE; /* mark object as
						** being created */
		global->rdf_resources |= RDF_RQTUPDATE; /* mark query tree as being
						** in update mode */
		/* release the query tree semaphore */
		status = rdu_rsemaphore(global);  
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = rdu_rtree(global, &root, (i4)catalog);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		/* retake the query tree semaphore to update state */
		status = rdu_gsemaphore(global);  
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rdf_qtulhobject->qry_root_node = root;
		rdf_qtulhobject->rdf_qtresource = RDF_SSHARED; /* mark query tree
						** object as being shared */
		global->rdf_resources &= (~RDF_RQTUPDATE); /* release update lock
						** on query tree object */
		/* release the query tree semaphore */
		status = rdu_rsemaphore(global);  
		if (DB_FAILURE_MACRO(status))
		    return(status);
		++(*builds);	/* increment ctr that RDf built this object */
		break;
	    }
	    case RDF_SSHARED:
	    case RDF_SPRIVATE:
	    default:
		status = E_DB_SEVERE;
		rdu_ierror(global, status, E_RD0110_QRYMOD);
		return(status);		    /* unexpected state */
	    }
	}

	/* build an alias to the object */
	status = ulh_define_alias(&global->rdf_ulhcb, (u_char *)aliasname,
		alias_len);
	if (DB_FAILURE_MACRO(status))
	{
	    if ((global->rdf_ulhcb.ulh_error.err_code != E_UL0120_DUP_ALIAS)
		&&
		(global->rdf_ulhcb.ulh_error.err_code != E_UL0122_ALIAS_REDEFINED))
	    {
		rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		    E_RD0045_ULH_ACCESS,0);
		return(status);
	    }
	    else
		status = E_DB_OK;		/* reset status if object obtained */
	}
    } /* endif object must be created */
    *rddqrymodp = rdf_qtulhobject;	    /* return query tree to user */
    return(status);
}

/*{
** Name: rdu_intprotrul	- first time read of integrity/protection/rule/alarm
**			   tuples
**
** Description:
**      This routine reads integrity/protection/rule/alarm tuples for the 
**	first time for the relation descriptor and determines the type of 
**	sharing that can occur.
**
**  NOTE: this routine makes QEF calls that make DMF calls.  It assumes that
**	  RDF is NOT  holding any semaphores on the target cache object when
**	  this routine is called.  If there are semaphores held, this could
**	  result in undetected deadlock in concurrent situations.
**
**	  (as of today [ 14-jan-94], this is only called by rdu_qmopen during
**	  processing of the RDF_READTUPLES operation.)
**
** Inputs:
**      global                          ptr to RDF state variable
**	table				RDF_TABLE code indicating which table
**					to open: RD_IIRULE, RD_IIINTEGRITY,
**					RD_IIPROTECT, RD_IISECALARM
**      extcattupsize                   tuple size of extended system catalog
**                                      tuples associated with table_id
**
** Outputs:
**      tuplepp                         if E_DB_OK then contains a linked list
**                                      of extended system catalog tuples
**	eof_found                       if E_DB_OK then TRUE indicates all
**                                      tuples have been read
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-nov-87 (seputis)
**          initial creation
**	09-mar-89 (neil)
**	    modified name of routine to rdu_intprotrul to support rules too.
**	    modified to work with rules.
**      19-apr-90 (teg)
**          correctly use new RDD_QDATA.qrym_alloc field to fix bug 8011.
**          (This bug was caused because RDD_QDATA.qrym_cnt was double-loaded.
**          It was used to indicate the amount of memory allocated to return
**          tuples in (which matched the #tuples requested on 1st call), and 
**          was also used to indicate the number of tuples that QEF found.)
**	16-jul-92 (teresa)
**	    prototypes
**	14-sep-92 (teresa)
**	    replaced table_id interface with RDF_TABLE interface.
**	29-nov-93 (robf)
**          Add support for security alarms
[@history_template@]...
*/
static DB_STATUS
rdu_intprotrul( RDF_GLOBAL         *global,
		RDF_TABLE	   table,
		i4                extcattupsize,
		RDD_QDATA          **tuplepp,
		bool		   *eof_found)
{
    struct
    {	/* Allocate the space for the retrieve output tuples, if allocating
	** space on the stack is a problem then RDF_TUPLE_RETRIEVE can be
        ** made equal to 1 */

	QEF_DATA           qefdata[RDF_TUPLE_RETRIEVE]; /* allocate space for
					    ** temporary qefdata elements */
	union 
	{
		DB_INTEGRITY      integ[RDF_TUPLE_RETRIEVE]; /* allocate
					    ** space for some integrity tuples 
					    */
		DB_PROTECTION     prot[RDF_TUPLE_RETRIEVE]; /* allocate
					    ** space for some protection tuples
                                            */
		DB_IIRULE      	  rule[RDF_TUPLE_RETRIEVE]; /* for rules too */
		DB_SECALARM    	  alarm[RDF_TUPLE_RETRIEVE]; /* for alarms too */
	} usr_data;
    }   buffer;

    RDD_QDATA		*rddqdata;
    RDF_CB		*rdfcb;
    DB_STATUS		status;
    i4			tuples_needed;

    rdfcb = global->rdfcb;
    status = rdu_malloc(global, (i4)sizeof(*rddqdata), (PTR *)&rddqdata);
    if (DB_FAILURE_MACRO(status))
	return(status);
    status = rdu_qopen(global, table, (char *)NULL,
		       extcattupsize, (PTR)&buffer, 
		       sizeof(buffer)); /* open extended system catalog */
    if (DB_FAILURE_MACRO(status))
	return(status);

    rddqdata->qrym_cnt = 0;	    /* init the number of buffers with data */
    rddqdata->qrym_alloc = 0;       /* init the number of buffers allocated */
    rddqdata->qrym_data = NULL;	    /* init the linked list to be NULL
				    ** terminated */

    *eof_found = FALSE;		    /* FIXME - one small problem may be that
                                    ** if the caller asks for exactly the number
                                    ** of tuples which exist in the relation,
                                    ** then a shared cache may not
                                    ** be created because eof_found will remain
                                    ** false */
    for(tuples_needed = rdfcb->rdf_rb.rdr_qtuple_count; 
	(tuples_needed > 0) && (!*eof_found);)
    {
	if (global->rdf_qeucb.qeu_count > tuples_needed)
	    global->rdf_qeucb.qeu_count = tuples_needed; /* do not give caller more
				    ** tuples than are requested */
	status = rdu_qget(global, eof_found);	/* get next set of tuples 
				    ** from extended system catalog */
	if (DB_FAILURE_MACRO(status))
	    return(status);

	{   /* allocate memory in relation descriptor to contain 
	    ** qrymod tuples,... the advantage of this approach is that
            ** the exact amount of memory is used in the cache, the 
            ** disadvantage is that an extra memory move is needed */
	    i4		    tuple;
	    QEF_DATA	    *temp_qdata;

	    temp_qdata = global->rdf_qeucb.qeu_output;
	    tuple = global->rdf_qeucb.qeu_count;		
	    rddqdata->qrym_cnt += tuple;	
	    rddqdata->qrym_alloc = rddqdata->qrym_cnt;
	    tuples_needed -= tuple;	/* get number of tuples still needed
					** by the caller */
	    for (; tuple > 0; tuple--)
	    {
		QEF_DATA		*datablk;

		status = rdu_malloc(global, 
				(i4)(sizeof(*datablk) + extcattupsize),
		    		(PTR *)&datablk);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		datablk->dt_size = extcattupsize; /* allocate memory for the
					** result tuple */
		datablk->dt_data = (PTR) ((char *) datablk + sizeof(*datablk));
		MEcopy((PTR)temp_qdata->dt_data, extcattupsize,
		    (PTR)datablk->dt_data);
		temp_qdata = temp_qdata->dt_next; /* get next temp block */
		datablk->dt_next = rddqdata->qrym_data;
		rddqdata->qrym_data = datablk; /* link into list of QEF
					** block, note that tuples are
					** linked in "backwards" but this should
					** not matter */
	    }
	}
    }
    *tuplepp = rddqdata;
    return(status);
}

/*{
** Name: rdu_qmclose	- close query mod table
**
** Description:
**      Routine which will close a query mod file, and perform some
**      consistency checks
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-nov-87 (seputis)
**          initial creation
**	16-apr-92 (teresa)
**	    typecase rdr_rec_access_id to (char *) before doing arithmetic
**	    operations on it.
**	16-jul-92 (teresa)
**	    prototypes
**	14-jan-94 (teresa)
**	    fix bug 56728 (assure semaphores are NOT held across QEF/DMF calls
**	    during RDF_READTUPLES operation).
**	6-may-97 (inkdo01)
**	    Avoid real close on RDF_FAKE_OPEN under RDF_SUPDATE (causes
**	    bus error in DMF).
[@history_template@]...
*/
DB_STATUS
rdu_qmclose(	RDF_GLOBAL      *global,
		RDF_STATE       *usr_state,
		RDF_STATE	*sys_state)
{	
    /* Close the table */
    DB_STATUS	status;
    DB_STATUS	s_status= E_DB_OK;

    s_status = rdu_gsemaphore(global); /* do not use semaphore
				** unless necessary */
    if (DB_FAILURE_MACRO(s_status))
	    return(s_status);

    switch (*usr_state)
    {
      case RDF_SUPDATE:
      {	/* the system copy was exclusively locked by this thread
	** so a record ID must exist for QEF to be called */

	/* this is the case in which the query mod tuples were read
	** into memory entirely and are shared by many threads 
	** concurrently, so that a QEF table was never really opened.
	**
	** I.E. -- this can be either a public or private infolbk.  In either
	** case, all tuples were read into the cache buffer and EOF was reached
	** via rdu_qmopen processing. Therefore, rdu_qmopen() actually closed
	** the catalog ad we do not really close it here.  We just pretend
	** to close it to keep the interface clean.  NO QEF CALL IS MADE HERE.
	*/
    
	if (global->rdfcb->rdf_rb.rdr_rec_access_id == (PTR) RDF_FAKE_OPEN)
	{
	    /* parser expects a non-zero access ID, so RDF dummy assigns it
	    ** to "1" on open and sets back to NULL on close"
	    */
	    status = E_DB_OK;
	    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    break;
	}
	s_status = rdu_rsemaphore(global);  /* must not hold semaphores
					    ** accross QEF/DMF calls */
	if (DB_FAILURE_MACRO(s_status))
	    return(s_status);
	status = rdu_qclose(global);
	s_status = rdu_gsemaphore(global);  /* Try to retake the semaphore.
					    ** even if this fails, we must set 
					    ** the state for the infoblk so that
					    ** we do not have a memory leak or 
					    ** corrupted cache. */
	*usr_state = RDF_SNOACCESS; /* mark the resource as deallocated */
	if (usr_state != sys_state)
	    *sys_state = RDF_SNOACCESS; /* if usr has private info block
			    ** then release this query mod resource to
			    ** be available to other threads , note that
			    ** it is not safe to update *sys_state twice
			    ** since there is a window for concurrent
			    ** use */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	if (DB_FAILURE_MACRO(s_status))
	    return(s_status);
	break;
      }
      case RDF_SPRIVATE:
      { 
	/* this is a private set of query mod tuples created in
	** the user's stream, the memory will be released when
	** the table is unfixed --
	**
	**  i.e., to get here we are reading more than 20 tuples into a
	**  private infoblk.  Since it is private, there is no need for 
	**  subsequent semaphore protection.
	*/
	if (global->rdfcb->rdf_rb.rdr_rec_access_id == (PTR) RDF_FAKE_OPEN)
	{
	    /* parser expects a non-zero access ID, so RDF dummy assigns it
	    ** to "1" on open and sets back to NULL on close"
	    */
	    status = E_DB_OK;
	    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    break;
	}
	s_status = rdu_rsemaphore(global);  /* must not hold semaphores
					    ** accross QEF/DMF calls */
	if (DB_FAILURE_MACRO(s_status))
	    return(s_status);
	status = rdu_qclose(global);
	s_status = rdu_gsemaphore(global);  /* must not hold semaphores
					    ** accross QEF/DMF calls */
	*usr_state = RDF_SNOACCESS; /* mark the resource
			    ** as being deallocated */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	if (DB_FAILURE_MACRO(s_status))
	    return(s_status);
	break;
    }
    case RDF_SSHARED:
	/* this is the case in which the query mod tuples were read
	** into memory entirely and are shared by many threads 
	** concurrently, so that a QEF table was never really opened.
	**
	** I.E. -- this can be either a public or private infolbk.  In either
	** case, all tuples were read into the cache buffer and EOF was reached
	** via rdu_qmopen processing. Therefore, rdu_qmopen() actually closed
	** the catalog ad we do not really close it here.  We just pretend
	** to close it to keep the interface clean.  NO QEF CALL IS MADE HERE.
	*/
	if (global->rdfcb->rdf_rb.rdr_rec_access_id == (PTR) RDF_FAKE_OPEN)
	{
	    /* parser expects a non-zero access ID, so RDF dummy assigns it
	    ** to "1" on open and sets back to NULL on close"
	    */
	    status = E_DB_OK;
	}
	else
	    status = E_DB_ERROR;
	global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	break;
    case RDF_SNOACCESS:
    default:
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0072_NOT_OPEN);
	return(status);
    }
    if (DB_FAILURE_MACRO(status))
	/* check if the proper table is open and accessible */
	rdu_ierror(global, status, E_RD0072_NOT_OPEN);

    s_status = rdu_rsemaphore(global);  /* many paths to here (but not all)
					** hold a semaphore.  Assure that we
					** have released it on the way out.
					** not that rdu_rsemaphore() is a noop
					** if no semaphore is held.
					*/
    if (DB_FAILURE_MACRO(s_status))
	return(s_status);

    return(status);
}

/*{
** Name: rdu_qmopen	- open query mod table and read tuples
**
** Description:
**      This routine should be called to open the query mod file and get
**      the first set of tuples
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      30-aug-87 (seputis)
**          initial creation
**      30-aug-88 (seputis)
**          fix concurrency problem, use RDF_RUPDATE instead of RDF_SUPDATE
**      1-dec-88 (seputis)
**          return E_DB_WARN status instead of E_DB_OK
**	    also close QEF file for private stream if all tuples are read
**	09-mar-89 (neil)
**	    modified to work for rules as well.
**	21-apr-89 (neil)
**	    Fixed bug which didn't allow process rdu_qget correct for qrymod.
**	    After calling rdu_qget the # of rows on EOF are in qeu_count.
**	01-feb-91 (teresa)
**	    change statistics reporting.  This entails changing hits input
**	    parameter to become three: finds (ie, cache entry was built), 
**	    builds (ie, cache entry successfully built via this call) and
**	    multiple (ie, enough tuples that multiple calls are required).
**	16-apr-92 (teresa)
**	    typecase rdr_rec_access_id to (char *) before doing arithmetic
**	    operations on it.
**      22-may-92 (teresa)
**          added new parameter to rdu_xlock call for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	14-sep-92 (teresa)
**	    changed interface, specify table type instead of table id.
**	12-mar-93 (teresa)
**	    moved rdf_shared_sessions increment logic out of rdu_xlock call.
**	29-nov-93 (robf)
**          Add support for security alarms
**	14-jan-94 (teresa)
**	    fix bug 56728 (assure semaphores are NOT held across QEF/DMF calls
**	    during RDF_READTUPLES operation).
**	16-feb-94 (teresa)
**	    refresh checksum whether or not tracepoint to skip setting
**	    checksums is set.
**	09-Apr-97 (kitch01)
**	    To see if we have what the caller is asking for also check
**	    rdr_2_types against types2_mask as this may be an object
**	    defined on types2_mask and is currently in storage. Also
**	    store types2_mask back in the object to ensure concurrent
**	    access. (Bug 81138)
**	22-jul-98 (shust01)
**	    Changed DB_FAILURE_MACRO(s_status) to DB_FAILURE_MACRO(status).
**	    Was checking the wrong variable after rdu_rsemaphore() call.
**	07-nov-1998 (nanpr01)
**	    Rewrote the routine to use correct state variables in a concurrent
**	    environment to avoid segv's. When using usrinfoblk, by mistake
**	    it was updating the sysinfoblk's ptr, causing segv.
[@history_template@]...
*/
static DB_STATUS
rdu_qmopen(
	RDF_GLOBAL	*global,
	RDF_TYPES       types_mask,
	RDF_TYPES       types2_mask,
	RDF_RTYPES	operation,
	RDF_SVAR	*builds,
	RDF_SVAR	*finds,
	RDF_SVAR	*multiple,
	RDF_TABLE	table,
	RDD_QDATA	**sys_tuplepp,
	RDF_STATE       *sys_state,
	RDD_QDATA	**usr_tuplepp,
	RDF_STATE       *usr_state,
	i4		extcattupsize)
{   /* if the access_id is null then this indicates that the user
    ** wishes to have the extended catalog opened */
    RDF_CB	    *rdfcb;
    DB_STATUS	    status;
    DB_STATUS	    s_status = E_DB_OK;
    RDF_STATE	    temp_sys_state;
    RDR_INFO	    *usr_infoblk;
    RDR_INFO	    *sys_infoblk;
    bool	    new_blk_flag = FALSE;
    bool	    eof_found = FALSE;

    rdfcb = global->rdfcb;
    usr_infoblk = rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    status = E_DB_OK;
    status = rdu_gsemaphore(global); /* do not use semaphore
				** unless necessary */
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Check for either a match of rdr_types with types_mask or
    ** a match of rdr_2_types with types2_mask. The object can be either
    ** defined on types_mask or types2_mask. Also ensure we store the
    ** correct type to allow for concurrent access.
    */
    if (sys_infoblk == usr_infoblk)
    {
        if (!(sys_infoblk->rdr_types & types_mask)
	    && !(sys_infoblk->rdr_2_types & types2_mask))
        {
	    /* 
	    ** we do not have what the caller has asked for, so initialize for
	    ** reading that information from the catalogs 
	    */
	    /* 
	    ** check if other thread is reading into this if so
	    ** read in pvt blk
	    */
	    switch (*sys_state)
	    {
		case RDF_SUPDATE:
		    /* Other threads may be trying to read in intprotrul */
		    /* Because we gave up the semaphore			 */
		case RDF_SNOACCESS:
		    /* Cannot hold the tuples - So create a private one */
	    	    temp_sys_state = *sys_state;
	    	    break;

		case RDF_SNOTINIT:
		  *sys_tuplepp = NULL;
	    	  *sys_state = RDF_SNOACCESS;
	    	  temp_sys_state = RDF_SNOACCESS; 
	    	  /* We will use sysinfoblk */
	    	  global->rdf_resources |= RDF_RUPDATE;
		  break;

		case RDF_SSHARED:
		case RDF_SPRIVATE:
		default:
		    /* We should have type set for these */
	    	    status = E_DB_ERROR;
	    	    rdu_ierror(global, status, E_RD0076_OPEN); /* the resource
				** is opened but rec access id is not
				** available */
	    	    return(status);
		    break;
	    }	
        }
        else
        {
	    /* the cache object has the required information, so find out what
	    ** state it's currently in.  If the state is "busy" we will have to 
	    ** make a private copy and populate it from catalogs.
	    */
	    temp_sys_state = *sys_state;
        }
        switch (temp_sys_state)
        {
        case RDF_SUPDATE:
	    /* 
	    ** there is concurrent use of the system query mod buffer so a 
	    ** private buffer needs to be created for this thread  --
	    **
	    ** I.E, some other thread is currently updating this object.  We 
	    ** dare not risk any collisions, so we will make our own private 
	    ** private copy and work from it.
	    */
	    status = rdu_private(global, &usr_infoblk); 
						/* create private info block */
	    if (DB_FAILURE_MACRO(status))
	        return(status);
	    /* report statistics */
	    Rdi_svcb->rdvstat.rds_l0_private++;
	    if (Rdi_svcb->rdvstat.rds_l0_private == 0)
		Rdi_svcb->rdvstat.rds_l1_private++;
	    
	    new_blk_flag = TRUE;
	    break;
        case RDF_SNOACCESS:
        {	
	    /* this state implies that:
	    ** 1. It is a newly created object that is not yet populated 
	    **    from the catalogs
	    ** or 
	    ** 2. there are a large number of tuples available, so that several 
	    **    calls to RDF will be needed to scan all the tuples - this 
	    **    implies only one thread can be scanning the tuples using 
	    **    the system buffer at one time.  Thus, resource needs to be 
	    **    read under semaphore protection in order to avoid 
	    **    concurrency problems 
	    */
	    global->rdf_resources |= operation; /* mark query mod
				    		** resource as being used */
	    /* 
	    ** For case 1 : we will use sysinfo blk 
	    ** For case 2 : we will have *sys_tuplepp/*usr_tuplepp set 
	    */
	    *sys_state = RDF_SUPDATE;
	    break;
        }
        case RDF_SSHARED:
        {
	    ++(*finds);		    /* update already on cache ctr for stats */
	    if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
	    {
	        /* 
		** set to known nonzero constant to indicate to PSF that 
		** table is open 
		*/
	        rdfcb->rdf_rb.rdr_rec_access_id = (PTR) RDF_FAKE_OPEN;
	    }
	    else
	    {
	        /* report an error because someone is trying to open a table 
		** that has already been opened */
	        status = E_DB_ERROR;
	        rdu_ierror (global, status, E_RD0076_OPEN);
	        return status;
	    }
	    status = E_DB_WARN;
	    if ((*usr_tuplepp)->qrym_cnt != 0)
	        rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
	    else
	        rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	    return(status);
        }
        case RDF_SPRIVATE: 
		/* 
		** unexpected state since a system info block
		** can never be private 
		*/
        default:
	    /* 
	    ** if the table is already open then report inconsistency error
	    */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0076_OPEN); /* the resource
				** is opened but rec access id is not
				** available */
	    return(status);
        }
    }
    else
    {
        if (!(usr_infoblk->rdr_types & types_mask)
	    && !(usr_infoblk->rdr_2_types & types2_mask))
	{
	    /* 
	    ** Check if sysinfoblk contains the stuff like rule/protection etc
	    ** what we are looking for
	    */
            if (!(sys_infoblk->rdr_types & types_mask)
	        && !(sys_infoblk->rdr_2_types & types2_mask))
	    {
		/* 
		** Probably cannot be cached because contains more tuples than
		** requested by the caller or it is reading it(though this 
		** is less likely to happen but can happen). Donot bother 
		** to try updating sysinfo. Just work with private copy.
		*/
	        *usr_tuplepp = NULL;
	        *usr_state = RDF_SNOACCESS;
	    }
	    else {
		/*
		** Check if this can be copied from the caller.
		** If it can be copy it. Otherwise read on your own.
		*/
		switch (*sys_state)
		{
		    case RDF_SSHARED :
			*usr_tuplepp = *sys_tuplepp;
	        	*usr_state = RDF_SPRIVATE;
	    		++(*finds);		    
				/* update already on cache ctr for stats */
	    		if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
	    		{
	        	    /* 
			    ** set to known nonzero constant to indicate to PSF 
			    ** that table is open 
			    */
	        	    rdfcb->rdf_rb.rdr_rec_access_id = 
							(PTR) RDF_FAKE_OPEN;
	    		}
	    		else
	    		{
	        	    /* 
			    ** report an error because someone is trying to 
			    ** open a table that has already been opened 
			    */
	        	    status = E_DB_ERROR;
	        	    rdu_ierror (global, status, E_RD0076_OPEN);
	        	    return(status);
	    		}
	    		status = E_DB_WARN;
	    		if ((*usr_tuplepp)->qrym_cnt != 0)
	        	    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
	    		else
	        	    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	    		return(status);
			break;
		    case RDF_SUPDATE:
		    case RDF_SNOACCESS :
			/* Create one since System is busy or being built */
	        	*usr_tuplepp = NULL;
	        	*usr_state = RDF_SNOACCESS;
			break;
		    case RDF_SPRIVATE:
		    default :
	    		/* 
	    		** if the table is already open then report 
			** inconsistency error
	    		*/
	    		status = E_DB_ERROR;
	    		rdu_ierror(global, status, E_RD0076_OPEN); 
				/* 
				** the resource is opened but rec access id 
				** is not available 
				*/
	    		return(status);
		}	
	    }
	}
	else
	{
	    /* 
	    ** It is all there in our private blk and we should start 
	    ** using it. 
	    */
	    ++(*finds);		    /* update already on cache ctr for stats */
	    *usr_state = RDF_SPRIVATE;
	    if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
	    {
	        /* 
		** set to known nonzero constant to indicate to PSF that 
		** table is open 
		*/
	        rdfcb->rdf_rb.rdr_rec_access_id = (PTR) RDF_FAKE_OPEN;
	    }
	    else
	    {
	        /* report an error because someone is trying to open a table 
		** that has already been opened */
	        status = E_DB_ERROR;
	        rdu_ierror (global, status, E_RD0076_OPEN);
	        return status;
	    }
	    status = E_DB_WARN;
	    if ((*usr_tuplepp)->qrym_cnt != 0)
	        rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
	    else
	        rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	    return(status);
	}

    }

    if (new_blk_flag)
    {
	if (types_mask & RDR_PROTECT)
	{
	    usr_state = &((RDF_ULHOBJECT*)usr_infoblk->
		rdr_object)->rdf_sprotection;
	    usr_tuplepp = (RDD_QDATA **)&usr_infoblk->rdr_ptuples;
	}
	else if (types_mask & RDR_SECALM)
	{
	    usr_state = &((RDF_ULHOBJECT*)usr_infoblk->
		rdr_object)->rdf_ssecalarm;
	    usr_tuplepp = (RDD_QDATA **)&usr_infoblk->rdr_atuples;
	}
	else if (types_mask & RDR_INTEGRITIES)
	{
	    usr_state = &((RDF_ULHOBJECT*)usr_infoblk->
		rdr_object)->rdf_sintegrity;
	    usr_tuplepp = (RDD_QDATA **)&usr_infoblk->rdr_ituples;
	}
	else if (types_mask & RDR_RULE)
	{
	    usr_state = &((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->rdf_srule;
	    usr_tuplepp = (RDD_QDATA **)&usr_infoblk->rdr_rtuples;
	}
	else if (types2_mask & RDR2_PROCEDURE_PARAMETERS)
	{
	    usr_state = &((RDF_ULHOBJECT*)usr_infoblk->rdr_object)
			->rdf_sprocedure_parameter;
	    usr_tuplepp = (RDD_QDATA **)&usr_infoblk->rdr_pptuples;
	}
	else
	{	/* unexpected querymod resource error */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0110_QRYMOD);
	    return(status);
	}
    }

    /* Open the iiintegrity/iiprotection/iirule/iisecalarm table. */
    if (!(*usr_tuplepp))
    {	
	/* 
	** memory has not been allocated for the qdata list so
	** this routine will create one 
	*/
	RDD_QDATA   *tuplep = NULL;

	status = rdu_rsemaphore(global);  /* must not hold semaphores
					    ** accross QEF/DMF calls */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	status = rdu_intprotrul(global, table, extcattupsize,
	    &tuplep, &eof_found);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = rdu_gsemaphore(global);  /* retake semaphore to update state */
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (global->rdf_resources & RDF_RUPDATE)
	{   
	    /* since an exclusive lock has been obtained, the
	    ** system memory stream was used to create the qrymod
	    ** so it needs to be made visible */
	    if (*sys_tuplepp)
	    {	/* unexpected querymod resource error, system tuples
		** should  not be available */
		status = E_DB_SEVERE;
		rdu_ierror(global, status, E_RD0110_QRYMOD);
		return(status);
	    }
	    if (eof_found)
	    {   
		/* 
		** mark this set of querymod tuples as shared since
		** it can fit into the requested caller's buffer,  note
		** that this assumes that all requests are the same
		** number of qef_data tuples 
		*/
		status = rdu_rsemaphore(global);  /* must not hold semaphores
						  ** accross QEF/DMF calls */
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = rdu_qclose(global); /* close the file now since
					     ** this will become a shared
					     ** resource */
		if (DB_FAILURE_MACRO(status))
		    return(status);

		/* retake semaphore to update infoblk state */
		status = rdu_gsemaphore(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		/* 
		** This should be fine since if RDF_RUPDATE is set, we allocate
		** from sysinfo.  This memory will not be freed until 
		** sysinfoblk stream is closed.
		*/
		*sys_state = RDF_SSHARED;
		if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
		{
		    /* set to known nonzero constant to indicate to PSF that 
		    ** table is open */
		    rdfcb->rdf_rb.rdr_rec_access_id = (PTR) RDF_FAKE_OPEN;
		}
		else
		{
		    /* report an error because someone is trying to open a 
		    ** table that has already been opened */
		    status = E_DB_ERROR;
		    rdu_ierror (global, status, E_RD0076_OPEN);
		    return(status);
		}
	        if (types_mask)
	        {
		    sys_infoblk->rdr_types |= types_mask; /* mark the query
					** mod fields as being initialized
					** for this descriptor */
	        }
	        if (types2_mask)
	        {
		    sys_infoblk->rdr_2_types |= types2_mask; /* mark the query
					** mod fields as being initialized
					** for this descriptor */
	        }
	    }
	    *sys_tuplepp = tuplep;
	}
	else
	{   /* the user must have a private copy so update only
	    ** the user infoblock */
	    if ((usr_tuplepp == sys_tuplepp) /* the system copy should
				** not be updated */
		||
		*usr_tuplepp	/* not expected this field to be
				** initialized */
		)
	    {	/* unexpected querymod resource error, system tuples
		** should  not be available */
		status = E_DB_SEVERE;
		rdu_ierror(global, status, E_RD0110_QRYMOD);
		return(status);
	    }
	    if (eof_found)
	    {   /* mark this set of querymod tuples as shared since
		** it can fit into the requested caller's buffer,  note
		** that this assumes that all requests are the same
		** number of qef_data tuples */

		s_status = rdu_rsemaphore(global);  /* must not hold semaphores
						    ** accross QEF/DMF calls */
		/* we must close the catalog before we check for semaphore
		** release errors */

		status = rdu_qclose(global); /* close the file now since
					** this will become a shared
					** resource, not really shared but
					** the close file routine expects
					** this state, and subsequent "EOF" 
					** processing will not work unless 
					** the file is closed now */
		if (DB_FAILURE_MACRO(status))
		    return(status);
		if (DB_FAILURE_MACRO(s_status))
		    return(s_status);

		/* retake semaphore to update infoblk state */
		status = rdu_gsemaphore(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
		{
		    /* set to known nonzero constant to indicate to PSF that 
		    ** table is open */
		    rdfcb->rdf_rb.rdr_rec_access_id = (PTR) RDF_FAKE_OPEN;
		}
		else
		{
		    /* report an error because someone is trying to open a 
		    ** table that has already been opened */
		    status = E_DB_ERROR;
		    rdu_ierror (global, status, E_RD0076_OPEN);
		    return(status);
		}
	        if (types_mask)
		    usr_infoblk->rdr_types |= types_mask; 
					/* 
					** mark the query mod fields as being 
					** initialized for this descriptor 
					*/
	        if (types2_mask)
		    usr_infoblk->rdr_2_types |= types2_mask; 
					/* 
					** mark the query mod fields as being 
					** initialized for this descriptor 
					*/
	    }
	    *usr_state = RDF_SPRIVATE;
	    *usr_tuplepp = tuplep;
	}
    }
    else
    {
	/* use existing qdata linked list */
	status = rdu_rsemaphore(global);    /* must not hold semaphores
						** accross QEF/DMF calls */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	status = rdu_qopen(global, table, (char *)NULL,
			extcattupsize, (PTR)*usr_tuplepp, 
			 0); /* 0 indicates to use
				** existing tuplepp list for qeu_output
				*/
	if (DB_FAILURE_MACRO(status))
	     return(status);
	status = rdu_qget(global, &eof_found);
	if (DB_FAILURE_MACRO(status))
	     return(status);
	/* retake semaphore to update infoblk state */
	status = rdu_gsemaphore(global); 
	if (DB_FAILURE_MACRO(status))
	    return(status);
	if (eof_found)
	    (*usr_tuplepp)->qrym_cnt = global->rdf_qeucb.qeu_count;
    }

    if (eof_found)
    {   /* report the end of file condition to the caller, make sure that
        ** usr_tuplepp points to correct state variable, if new info
        ** block was created above */
	status = E_DB_WARN;
	if ((*usr_tuplepp)->qrym_cnt != 0)
	{
	    ++(*builds);	/* increment counter for statistics */
	    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
	}
	else
	    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
    }
    else
    {
	global->rdf_resources |= RDF_RMULTIPLE_CALLS; /* multiple calls
				** will be needed to complete
				** this request */
	++(*multiple);	    /* increment statistics counter */
    }
    return(status);
}

/**
**	
** Name: rdu_rqrytuple - Read the integrity/protection/rule/alarm tuples.
**
**	Internal call format:	rdu_rqrytuple(rdf_cb, operation)
**
** Description:
**      This function reads the integrity/protection/rule/alarm tuples.
**	
** Inputs:
**	rdf_cb
**	operation
**
** Outputs:
**	rdf_cb
**	    .rdr_info_blk
**		.rdr_ituples
**		.rdr_ptuples
**		.rdr_atuples
**	    .rdf_error
**		.err_code
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}	
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      15-Aug-86 (ac)    
**          written
**	14-aug-87 (puree)
**	    convert QEU_ATRAB to QEU_ETRAN
**      5-nov-87 (seputis)
**          rewritten for resource management, eliminate transactions,
**          caching small requests, reusing memory
**	09-mar-89 (neil)
**	    extended to work for rules too.
**	21-apr-89 (neil)
**	    Fixed bug which didn't allow process rdu_qget correct for qrymod.
**	    The GETNEXT path was assigning qrym_cnt upon EOF.
**	16-jul-92 (teresa)
**	    prototypes
**	14-sep-92 (teresa)
**	    change interface to rdu_rqrytupple to take table type instead of
**	    table id.
**	29-nov-93 (robf)
**          Add support for security alarms
**	14-jan-94 (teresa)
**	    fix bug 56728 (assure semaphores are NOT held across QEF/DMF calls
**	    during RDF_READTUPLES operation).
*/
static DB_STATUS
rdu_rqrytuple(
	RDF_GLOBAL	*global,
	RDF_TYPES       types_mask,
	RDF_TYPES       types2_mask,
	RDF_RTYPES	operation,
	RDF_SVAR        *requests,
	RDF_SVAR        *builds,
	RDF_SVAR        *finds,
	RDF_SVAR        *multiple,
	RDF_TABLE	table,
	RDD_QDATA	**sys_tuplepp,
	RDF_STATE       *sys_state,
	RDD_QDATA	**usr_tuplepp,
	RDF_STATE       *usr_state,
	i4		extcattupsize)
{
    RDF_CB	    *rdfcb;
    DB_STATUS	    status;

    rdfcb = global->rdfcb;

    switch (rdfcb->rdf_rb.rdr_update_op)
    {
    case RDR_CLOSE:
	/* close the query mod file */
	status = rdu_qmclose(global, usr_state, sys_state);
	break;
    case RDR_OPEN:
    {
	++(*requests);			/* update request for info stat */
	if (rdfcb->rdf_rb.rdr_rec_access_id == NULL)
	{   /* if the file is not opened then open it, allocate the tuple 
	    ** buffers and fetch the first block of tuples */
	    status = rdu_qmopen(global, types_mask, types2_mask, operation, 
				builds, finds, multiple, table, sys_tuplepp, 
				sys_state, usr_tuplepp, usr_state, 
				extcattupsize);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	else
	{   /* report error if file is already open */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0110_QRYMOD);
	    return(status);
	}
	break;
    }
    case RDR_GETNEXT:
    {
        /* table is already open and buffer space should already be
	** allocated so get next set of tuples */
	bool		eof_found;
	if (rdfcb->rdf_rb.rdr_qtuple_count <= 0) /* requested number of
					    ** qrymod tuples must be non-zero */
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0073_TUPLE_COUNT); 
	    return(status);
	}

	/* take semaphore to assure valid read of infoblk state */
	status = rdu_gsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	switch (*usr_state)
	{	/* consistency check */
	case RDF_SUPDATE:		/* use the system copy */
	case RDF_SPRIVATE:          /* user has a private copy */
	    break;			/* OK state */
	case RDF_SNOACCESS:
	case RDF_SSHARED:
	default:
	    {   /* unexpected querymod resource error */
		status = E_DB_SEVERE;
		rdu_ierror(global, status, E_RD0110_QRYMOD);
		return(status);
	    }
	}

	status = rdu_rsemaphore(global);  /* must not hold semaphores
					    ** accross QEF/DMF calls */
	if (DB_FAILURE_MACRO(status))
	    return(status);


	global->rdf_qeucb.qeu_tup_length = extcattupsize;
	global->rdf_qeucb.qeu_count = (*usr_tuplepp)->qrym_cnt; 
        global->rdf_qeucb.qeu_output = (*usr_tuplepp)->qrym_data; 
	global->rdf_init |= RDF_IQDATA;
	global->rdf_resources |= operation; /* mark resource as being held
				    ** in case of errors, so that the
				    ** file will be close */
	status = rdu_qget(global, &eof_found); /* if the file is already open then
				    ** just get the next set of tuples */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	if (eof_found)
	{   /* report the end of file condition to the caller */
	    status = E_DB_WARN;
	    (*usr_tuplepp)->qrym_cnt = global->rdf_qeucb.qeu_count;
	    if ((*usr_tuplepp)->qrym_cnt != 0)
		rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
	    else
		rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	}
	break;
    }	
    default:
	{   /* unexpected querymod resource error */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0110_QRYMOD);
	    return(status);
	}
    }
    return(status);
}

/*{
** Name: rdu_txtcopy	- copy iiqrytext data from read buffer to cache
**
** Description:
**      This routine is the equivalent of MEcopy, but is guarenteed NOT to
**	harm anything if its interrupted in the middle.  It is called because
**	RDF may be overwriting an existing db procedure query text (that another
**	thread is reading) with the identical text in high concurency
**	situations.  This routine assures that the copy will NOT harm the
**	data that is beinng overwritten (if it is a case where data is being
**	overwritten).  Since there is no such specification for MEcopy in the
**	CL spec, this is routine necessary.  This routine may be time sliced
**	out anytime and nothing will be harmed.
**
** Inputs:
**      from	-	address of character string to copy
**	len	-	number of bytes to copy
**	to	-	where to copy the character string to
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-mar-92 (teresa)
**	    initial creation. (Fixes bug 42922.)
**	16-jul-92 (teresa)
**	    prototypes
*/
VOID 
rdu_txtcopy(	u_char  *from,
		u_i2	len,
		u_char  *to)
{
while (len--)
    {
	*to++ = *from++;
    }
}

/*{
** Name: rdu_special_proc - Determine if this is a special procedure
**
** Description:
**
**      This routine determines if the specified procedure comes form a list
**	of procedures that RDF recognizes and builds from hardcoded constants
**	instead of one that RDF reads from catalogs.
**
** Inputs:
**	prcname
**	owner
**	upcase
**
** Outputs:
**	Returns:
**	    proc_id - ordinal position of procedure in lookup table
**		      special_proc; 0=> not found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-oct-92 (teresa)
**	    Initial Creation
**	20-aug-97 (nanpr01)
**	    sql92 upgrade problem. The static array is lower case whereas
**	    the prcname and owner passed to this routine is in upper case.
**	    So we have to do a case insensitive comparison to find
**	    a match.
*/

static i4  
rdu_special_proc( char *prcname, char *owner, bool upcase)
{
    i4	i;
    char	special_name[DB_DBP_MAXNAME];
    char	special_own[DB_OWN_MAXNAME];

    for (i=1; i < SPECIAL_CNT; i++)
    {
	STmove((PTR)special_proc[i].proc_name, ' ', DB_DBP_MAXNAME, special_name);
	STmove((PTR)special_proc[i].proc_own, ' ', DB_OWN_MAXNAME, special_own);
	if (upcase)
	{
	   if (STbcompare((PTR)prcname, DB_DBP_MAXNAME,
				special_name, DB_DBP_MAXNAME, 1))
	     continue;
	   if (STbcompare((PTR)owner, DB_OWN_MAXNAME, 
				special_own, DB_OWN_MAXNAME, 1))
	     continue;
	}
	else
	{
	   if (MEcmp((PTR)prcname, special_name, DB_DBP_MAXNAME))
	   	continue;
	   if (MEcmp((PTR) owner, special_own, DB_OWN_MAXNAME))
	        continue;
	}
	return ( special_proc[i].proc_id );
    }
    return (special_proc[0].proc_id);
}

/*{
** Name: rdu_procedure	- read iiqrytext info for the procedure catalog
**
** Description:
**      Routine which will read iiqrytext info for the procedure object
**
** Inputs:
**      global                          rdf state information
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-apr-88 (seputis)
**          initial creation
**      03-aug-88 (seputis)
**          return status was not reset if ULH alias error was detected,
**          this should be a recoverable error.
**      23-sep-88 (seputis)
**          implemented GRANT
**      13-dec-88 (seputis)
**          fix unix lint problem
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	15-Dec-89 (teg)
**	    get RDF SVCB address from global instead of from GLOBALDEF Rdi_svcb
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	06-apr-90 (teg)
**	    remove outdated if'def code to get iiprocedure via dmf_show call.
**	01-feb-91 (teresa)
**	    updated statistics reporting.
**	23-jan-92 (teresa)
**	    SYBIL changes: rdi_poolid->rdv_poolid; rdi_qthashid->rdv_qtree_hashid;
**			   rdv_c1_memleft->rdv_memleft
**	08-jun-92 (teresa)
**	    SYBIL:  implement rdr2_refresh.
**	30-jun-92 (andre)
**	    added support for requesting dbproc information by id
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    change interface for rdu_qopen
**	01-oct-92 (teresa)
**	    Add support for RDF generated internal procedures
**      02-nov-92 (teresa)
**          initialize global->rdfcb->rdf_rb.rdr_qtext_id with qry text id.
**	11-mar-93 (teresa)
**	    fix 'dbproc information by id bug' where status is not initialized
**	    before its used.
**	31-mar-93 (swm)
**	    Fixed typo in cast: "(ucha *)" to "(u_char *)".
**	19-apr-94 (teresa)
**	    fix bug 62560
**	8-jul-94 (robf)
**	    Initialize security label values for internal database 
**	    procedures. 
**	6-aug-97 (nanpr01)
**	    Memory overlay problem in upgradedb.
**	20-aug-97 (nanpr01)
**	    For sql92 upgrade, we need to pass in whether or not we need
**	    to do a case insensitive search. Since the static array
**	    contains lower case internal procedure names, we copy the
**	    the passed in procedure name and owner after a match is 
**	    found.
**      10-May-2000 (wanya01)
**          Bug 101306 - Allow rdu_procedure() read more than 65k iiqrytext 
**          info from system catalog.
**	17-april-2008 (dougi)
**	    Re-enable RDR_BY_ID for procedures until we figure out why it was
**	    disabled. It is very useful for table procedures.
**	27-May-2010 (wanfr01)
**	    Bug 123824 - rdf_shared_sessions needs to be initialized to 1
**	    during procedure initialization.
*/

static DB_STATUS
rdu_procedure(	RDF_GLOBAL         *global,
		bool		   *refreshed)
{
    DB_STATUS		status, loc_status;
    bool		eof_found;
    i4			special_id;
    struct {
	i4		db_id;
	DB_DBP_NAME	prc_name;
	DB_OWN_NAME	prc_owner;
    } obj_alias;		    /* structure used to define an alias
				    ** by name to the object */

    struct
    {	/* Allocate the space for the retrieve output tuples, if allocating
	** space on the stack is a problem then RDF_TUPLE_RETRIEVE can be
        ** made equal to 1, but this may require several calls to DMF */

	QEF_DATA           qdata[RDF_TUPLE_RETRIEVE]; /* allocate space for
					    ** temporary qefdata elements */
	union usr_data
	{
	    DB_PROCEDURE    procedure[RDF_TUPLE_RETRIEVE]; /* buffer area for
                                            ** procedure tuple */
	    DB_IIQRYTEXT    qrytext[RDF_TUPLE_RETRIEVE]; /* buffer area for
					    ** qrytext tuples */
	} tempbuffer;
    }   buffer;

    DB_PROCEDURE	    proc_tuple;
    RDF_CB		    *rdfcb;
    u_i4		    dbp_tidp;
    bool		    dbp_by_id;
    RDF_ULHOBJECT	    *rdf_ulhobject; /* root of procedure descriptor  
					    ** object dbp_by_idstored in ULH */

    rdfcb = global->rdfcb;
    dbp_by_id = (~rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME) != 0;
    /* for the time being we will NOT allow access by dbproc id */
    /* Not sure why this was disallowed - it has now been re-enabled because
    ** it very helpful for table procedures. */
    if (FALSE && dbp_by_id)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
    
    /* get access to the ULH object */
    if (rdfcb->rdf_info_blk != (RDR_INFO *) NULL)
    {   
	/* object is already available in the info block */

	rdf_ulhobject = (RDF_ULHOBJECT *)(rdfcb->rdf_info_blk->rdr_object);
	status = E_DB_OK;

	/* note: we intentionally do not check the RDR2_REFRESH request here.
	**       if the user already has the object fixed, we do NOT refresh it.
	*/
    }
    else
    {
	/* obtain access to a cache object */
	if (!(global->rdf_init & RDF_IULH))
	{	/* init the ULH control block if necessary, only need the server
	    ** level hashid obtained at server startup */
	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	    global->rdf_init |= RDF_IULH;
	}

	/* if caller specified name, try to get descriptor by alias */
	if (!dbp_by_id)
	{
	    obj_alias.db_id = rdfcb->rdf_rb.rdr_unique_dbid;
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_prcname, 
						    obj_alias.prc_name);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
						    obj_alias.prc_owner);
	    status = ulh_getalias(&global->rdf_ulhcb, 
				  (unsigned char *)&obj_alias, 
				  sizeof(obj_alias));
	}
	else
	{
	    /*
	    ** caller specified dbproc id - try to access an object using its
	    ** id
	    ** NOTE: this code will NOT be executed until we remove the check
	    ** above that insists that we must be given dbproc name and owner
	    */
	    (VOID) rdu_setname(global, &rdfcb->rdf_rb.rdr_tabid);
	    status = ulh_access(&global->rdf_ulhcb, 
		                (u_char *)global->rdf_objname, 
				global->rdf_namelen);
	}

	if (DB_FAILURE_MACRO(status))
	{
	    if((global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
		&&
		(global->rdf_ulhcb.ulh_error.err_code != E_UL0121_NO_ALIAS))
	    {
		rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		    E_RD012D_ULH_ACCESS,0);
		return(status);
	    }
	    rdf_ulhobject = NULL;   /* cannot find the ULH descriptor */
	}
	else
	{
	    /* update statistics */
	    Rdi_svcb->rdvstat.rds_f2_procedure++;   
	    rdf_ulhobject =
			(RDF_ULHOBJECT *)global->rdf_ulhcb.ulh_object->ulh_uptr;

	    /* if user has requested RDF refresh the cache, see if the object
	    ** should be destroyed and recreated 
	    */
	    if ( (global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH)
		&&
		  rdf_ulhobject
	       )
	    {
		/* invalidate this cache entry and behave as though 
		** the entry is not found
		*/
		*refreshed =  TRUE;
		status = ulh_destroy(&global->rdf_ulhcb, 
				     (unsigned char *) NULL, 0);
		if (DB_FAILURE_MACRO(status))
		{
		    rdu_ferror( global, status, 
				&global->rdf_ulhcb.ulh_error, 
				E_RD0040_ULH_ERROR,0);
		}		    
		rdf_ulhobject = (RDF_ULHOBJECT *) 0;
	    }
	    else
	    {
		/* found the object, object would should not have been created
		** unless the procedure tuple has been read */


		global->rdf_resources |= RDF_RULH; /* mark ULH object as being
					** fixed */
		rdf_ulhobject = 
		        (RDF_ULHOBJECT *)global->rdf_ulhcb.ulh_object->ulh_uptr;
		/* make the procedure as being "fixed" */
		rdfcb->rdf_info_blk = rdf_ulhobject->rdf_sysinfoblk; 
	    }
	}
    }

    if (rdf_ulhobject)
    {	/* if object has been obtained */
	global->rdfcb->rdf_rb.rdr_procedure = rdf_ulhobject->rdf_procedure;
	    /* this is the old interface, would be nice to have PSF access the 
	    ** procedure tuple directly in the RDF info block */
	Rdi_svcb->rdvstat.rds_c5_proc++;
	return(status);
    }

    /*
    * if caller specified access by dbproc id, we will read IIXPROCEDURE and do
    ** a TID-join into IIPROCEDURE to find the tuple.
    ** NOTE: this should never occur for procedures that RDF builds rather than
    **	     reads from catalogs
    */
    if (dbp_by_id)
    {
	global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
	status = rdu_qopen(global, RD_IIXPROCEDURE, (char *)0,
	    sizeof(DB_IIXPROCEDURE), (PTR)&buffer, sizeof(buffer));
	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = rdu_qget(global, &eof_found);

	/* close the file now since the IIXPROCEDURE tuple has been read */
	loc_status = rdu_qclose(global);
	if (DB_FAILURE_MACRO(loc_status) && loc_status > status)
	    status = loc_status;
	if (DB_FAILURE_MACRO(status))
	    return(status);

        switch (global->rdf_qeucb.qeu_count)
	{
	    case 0:
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0201_PROC_NOT_FOUND);
		return(status);
	    }
	    case 1:
	    {	/* exactly one tuple is found */
		QEF_DATA    *qefxprocp = global->rdf_qeucb.qeu_output;
		
		dbp_tidp = ((DB_IIXPROCEDURE *) qefxprocp->dt_data)->dbx_tidp;
		break;
	    }
	    default:
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0137_DUPLICATE_PROCS);
		return(status);
	}
    }

    /* see if this is a special internal procedure that will not be in the
    ** catalogs.  
    */
    if (special_id = 
	         rdu_special_proc((char *)(&rdfcb->rdf_rb.rdr_name.rdr_prcname),
			          (char *)(&rdfcb->rdf_rb.rdr_owner),
		  ((*global->rdf_sess_cb->rds_dbxlate & CUI_ID_REG_U) != 0))
       )
    {
	/* This is a special internal procedure that is NOT in DBMS catalogs.
	** Use a lookup table indexed by "special_id" to get the values instead
	** of using the iiprocedure catalog tuple
	*/
	/*
	STmove( (PTR) special_proc[special_id].proc_name, ' ', DB_DBP_MAXNAME,
		(PTR) &proc_tuple.db_dbpname);
	STmove( (PTR)  special_proc[special_id].proc_own, ' ', DB_OWN_MAXNAME,
		(PTR) &proc_tuple.db_owner);
	*/
	MEcopy( (PTR) rdfcb->rdf_rb.rdr_name.rdr_prcname.db_dbp_name, 
		DB_DBP_MAXNAME, (PTR) &proc_tuple.db_dbpname);
	MEcopy( (PTR) rdfcb->rdf_rb.rdr_owner.db_own_name, DB_OWN_MAXNAME,
		(PTR) &proc_tuple.db_owner);
	proc_tuple.db_txtlen = special_proc[special_id].txt_len;
	proc_tuple.db_mask[0] = DB_IPROC | DB_DBPGRANT_OK | DB_ACTIVE_DBP;
	proc_tuple.db_mask[1] = 0;
	proc_tuple.db_procid.db_tab_base = 10+special_id;   /* dummy value */
	proc_tuple.db_procid.db_tab_index = 20+special_id;  /* dummy value */
	proc_tuple.db_txtid.db_qry_high_time = 1000+special_id; /*dummy value*/
	proc_tuple.db_txtid.db_qry_low_time = 100+special_id; /* dummy value */
	STRUCT_ASSIGN_MACRO(proc_tuple.db_txtid,
			    global->rdfcb->rdf_rb.rdr_qtext_id);
    }
    else
    {
	char	    key[1];
	
	/*
	** if caller supplied dbproc name and owner, access IIPROCEDURE catalog
	** to get tuple by name which defines the qrytext ID; otherwise access
	** IIPROCEDURE by TID saved above (in dbp_tidp)
	*/
	
	global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	global->rdfcb->rdf_rb.rdr_qtuple_count = 0;

	/* if planning to access IIPROCEDURE by TID, no keys will be used */
	if (dbp_by_id)
	{
	    key[0]='T';
	}
	else
	{
	    key[0]='\0';
	}
	
	status = rdu_qopen(global, RD_IIPROCEDURE, key,
			   sizeof(DB_PROCEDURE), (PTR)&buffer, sizeof(buffer));
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (dbp_by_id)
	{
	    global->rdf_qeucb.qeu_flag = QEU_BY_TID;
	    global->rdf_qeucb.qeu_tid = dbp_tidp;
	}

	status = rdu_qget(global, &eof_found);

	/* close the file now since the procedure tuple has been read */
	loc_status = rdu_qclose(global); 
	if (DB_FAILURE_MACRO(status) && loc_status > status)
	    status = loc_status;

	if (DB_FAILURE_MACRO(status))
	    return(status);

        switch (global->rdf_qeucb.qeu_count)
	{
	    case 0:
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0201_PROC_NOT_FOUND);
		return(status);
	    }
	    case 1:
	    {	/* exactly one tuple is found */
		QEF_DATA    *qefprocp;
		
		qefprocp = global->rdf_qeucb.qeu_output;
		MEcopy((PTR)qefprocp->dt_data, sizeof(proc_tuple),
		    (PTR)&proc_tuple);
		if (dbp_by_id)
		{
		    STRUCT_ASSIGN_MACRO(proc_tuple.db_dbpname,
			rdfcb->rdf_rb.rdr_name.rdr_prcname);
		    STRUCT_ASSIGN_MACRO(proc_tuple.db_owner,
			rdfcb->rdf_rb.rdr_owner);
		}
		else
		{
		    rdfcb->rdf_rb.rdr_tabid.db_tab_base =
			proc_tuple.db_procid.db_tab_base;
		    rdfcb->rdf_rb.rdr_tabid.db_tab_index =
			proc_tuple.db_procid.db_tab_index;
		}
		break;
	    }
	    default:
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0137_DUPLICATE_PROCS);
		return(status);
	}
    }

    {	/* read the query text tuples from IIQRYTEXT catalog */
	char	    *textp;
	RDD_QRYMOD  *rdf_qtulhobject;

	if (!special_id)
	{
	    /* open the iiqrytext catalog unless this is a special internal
	    ** database procedure
	    */
	    global->rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    global->rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, RD_IIQRYTEXT, 
			      (char *)&proc_tuple.db_txtid,sizeof(DB_IIQRYTEXT),
			      (PTR)&buffer, sizeof(buffer));
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    STRUCT_ASSIGN_MACRO(proc_tuple.db_txtid, 
				global->rdfcb->rdf_rb.rdr_qtext_id);
	}

	rdu_qtname(global, DB_DBP);     /* initialize the DB_QRY_ID name
					** for the ULH object name */	
	status = ulh_create(&global->rdf_ulhcb, 
			    (unsigned char *)global->rdf_objname,
			    global->rdf_namelen, ULH_DESTROYABLE, 0);

	if (DB_SUCCESS_MACRO(status))
	{
	    global->rdf_resources |= RDF_RULH; /* mark ULH object as being
					** fixed */
	    /* update statistics */
	    Rdi_svcb->rdvstat.rds_f2_procedure++;
	}
	else
	{
	    rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		E_RD0045_ULH_ACCESS,0);
	    return (status);		/* some other ULH error occurred so
					** report it and return */
	}
	/* get the query tree/text semaphore */
	status = rdu_gsemaphore(global);  
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* at this point memory may need to be allocated, but the procedure 
	** text needs to be read into the rdf_procedure structure since 
	** concurrent activity may mean it is only allocated but not defined.  
	** Prior to making the shared structure visible to other threads, a 
	** semaphore is obtained for memory allocation.  If 2 threads are 
	** executing this code on the same procedure then one may allocate, but 
	** the other may initialize, etc.  the idea is that all memory is 
	** allocated under semaphore procedure without any windows, and
	** that the rdr_dbp tuple and query text tuple are initialized prior to
	** exiting this procedure.  This complication occurs since calls to QEF
	** cannot be semaphore protected due to deadlock problems. 
	*/
	rdf_ulhobject = (RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->ulh_uptr;
	if (!rdf_ulhobject)
	{

	    /* if this is not defined then this is the first user so allocate
            ** all the memory prior to calling QEF.  The semaphore will be
	    ** release prior to the QEF call.
	    */
	    if (!(global->rdf_init & RDF_IULM))
	    {	/* init the ULM control block for memory allocation */
		RDI_FCB	    *rdi_fcb;	/* ptr to facility control block 
					    ** initialized at server startup time
					    */
		ULM_RCB	    *ulm_rcb;
		rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
		ulm_rcb = &global->rdf_ulmcb;
		ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
		ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
		ulm_rcb->ulm_facility = DB_RDF_ID;
		ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
		/* get the stream id from the ulh descriptor */
		ulm_rcb->ulm_streamid_p
		    = &global->rdf_ulhcb.ulh_object->ulh_streamid;
		global->rdf_init |= RDF_IULM;
	    }
	    {	
		/* initialize the main ULH descriptor */
		global->rdf_resources |= RDF_RUPDATE; /* ULH object has been
				    ** successfully inited, to be updated by
				    ** this thread */
		status = rdu_malloc(global, (i4)(sizeof(RDF_ULHOBJECT) +
				sizeof(*rdf_ulhobject->rdf_sysinfoblk) +
				sizeof(DB_PROCEDURE)),
		    		(PTR *)&rdf_ulhobject);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		rdf_ulhobject->rdf_ulhptr = global->rdf_ulhcb.ulh_object; 
				    /* save
				    ** ptr to ULH object for unfix commands */
		rdf_ulhobject->rdf_state = RDF_SUPDATE; /* indicate that this
				    ** object is being updated */
		
		rdf_ulhobject->rdf_sintegrity = RDF_SNOTINIT;
		rdf_ulhobject->rdf_sprotection = RDF_SNOTINIT;	
		rdf_ulhobject->rdf_ssecalarm = RDF_SNOTINIT;
		rdf_ulhobject->rdf_srule = RDF_SNOTINIT;
		rdf_ulhobject->rdf_skeys = RDF_SNOTINIT;
		rdf_ulhobject->rdf_sprocedure_parameter = RDF_SNOTINIT;
		rdf_ulhobject->rdf_procedure = NULL;
		rdf_ulhobject->rdf_defaultptr = NULL;
		rdf_ulhobject->rdf_shared_sessions = 1;
		global->rdf_ulhobject = rdf_ulhobject; /* save ptr
				    ** to the resource which is to be updated */
		rdf_ulhobject->rdf_sysinfoblk = (RDR_INFO *)
			((char *)rdf_ulhobject + sizeof(RDF_ULHOBJECT));
		rdu_init_info(global, rdf_ulhobject->rdf_sysinfoblk,
		    global->rdf_ulhcb.ulh_object->ulh_streamid);
		rdf_ulhobject->rdf_sysinfoblk->rdr_object = (PTR)rdf_ulhobject;
				    /*
				    ** have this object point back to itself
				    ** initially, so it can be found if the user
				    ** calls RDF with this object again 
				    */
	        /* allocate memory for the procedure tuple and copy */
		rdf_ulhobject->rdf_sysinfoblk->rdr_dbp = (DB_PROCEDURE *)
			((char *) rdf_ulhobject->rdf_sysinfoblk + 
			 sizeof(*rdf_ulhobject->rdf_sysinfoblk));
		MEcopy((PTR)&proc_tuple, sizeof(DB_PROCEDURE),
		    (PTR)rdf_ulhobject->rdf_sysinfoblk->rdr_dbp);
	    }
	    status = rdu_malloc(global, 
		(i4)(sizeof(RDD_QRYMOD) + proc_tuple.db_txtlen + 1),
		(PTR *)&rdf_qtulhobject);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    rdf_ulhobject->rdf_procedure = rdf_qtulhobject;
	    textp = (char *)&rdf_qtulhobject[1];
	    rdf_qtulhobject->qrym_id = DB_DBP;
	    rdf_qtulhobject->qry_root_node = NULL; /* The root node of the query 
					    ** tree not used for procedure */
	    rdf_qtulhobject->rdf_qtstreamid = NULL; /* not a private stream */
	    rdf_qtulhobject->rdf_qtresource = RDF_SSHARED;
	    rdf_qtulhobject->rdf_ulhptr = (PTR)global->rdf_ulhcb.ulh_object;
	    rdf_qtulhobject->rdf_querytext = textp;
	    rdf_qtulhobject->rdf_l_querytext = proc_tuple.db_txtlen;
	    global->rdf_ulhcb.ulh_object->ulh_uptr = (PTR)rdf_ulhobject; 
				/* save this object after marking 
				** its' status as being updated,
				** IMPORTANT - this structure should be totally
				** initialized with the DB_PROCEDURE tuple and
				** memory allocation, prior to releasing the
				** semaphore so that it does not conflict
				** with concurrent threads, i.e. calls to QEF
				** will release the semaphore */
	}
	else
	{   /* if this is defined then this is a subsequent caller so
            ** check if the memory parameters are correct */
	    if (
		!rdf_ulhobject->rdf_procedure
		||
		(   rdf_ulhobject->rdf_procedure->rdf_l_querytext 
		    != 
		    proc_tuple.db_txtlen
		)
	       )
	    {	/* consistency check - mismatched length */
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0136_CONCURRENT_PROC);
		return(status);  /* fix bug 62560 */
	    }
	    rdf_qtulhobject = rdf_ulhobject->rdf_procedure;
	    textp = rdf_qtulhobject->rdf_querytext;
	    /* update buffer concurrently, since all memory has been allocated
            ** and it should contain the same data */
	}

	/* release the query tree/text semaphore -- its a no-no to hold 
	** semaphores across DMF calls because this can cause undetected
	** deadlocks and the subbordinate routine calls DMF 
	*/
	status = rdu_rsemaphore(global);  
	if (DB_FAILURE_MACRO(status))
	    return(status);
	if (special_id)
	{
	    i4  i, remaining = proc_tuple.db_txtlen;
	    i4  byte_copied;

	    /* copy querytext from special_proc. */
	    for (i=0; i<special_proc[special_id].txt_cnt; i++)
	    {
		if (remaining >= TXTLEN)
		{
		    MEcopy(special_proc[special_id].proc_txt[i], TXTLEN,
		       textp);
		    byte_copied = TXTLEN;
		}
		else
		{
		    MEcopy(special_proc[special_id].proc_txt[i], remaining, 
			textp);
		    byte_copied = remaining;
		}
		textp += byte_copied;
		remaining -= byte_copied;
	    }
	}
	else
	{
	    /* not a special hard coded internal procedure, so use data from
	    ** catalogs
	    */

	    i4		sequence;	    /* sequence number for the query
					    ** text tuples being read */
	    i4		remaining;	    /* number of bytes remaining to be
					    ** read */
	    i4		current;	    /* current byte to be read */

	    current = 0;
	    remaining = proc_tuple.db_txtlen;
	    sequence = 0;
	    eof_found = FALSE;

	    while    (!eof_found)
	    {
		/* warning -- this loop, in concurrent conditions, may be
		** overwriting a fully created cache object that another thread
		** is reading.  The assumption is that this is harmless, since
		** it is overwriting it with exactly the same contents as it
		** already contains.  It is difficult to tell whether or not  
		** this object was correctly built by another (concurrent) 
		** thread or whether that thread took an error exit.  So every
		** thread that tries to build the object at the same time will
		** build it in its entirity.  The object cannot disappear from
		** underneath the thread that is currently updating it because
		** ULH keeps a "use count" and will not release the object
		** until the use count goes to zero. That use count cannot go
		** to zero while this thread has this object fixed (which it
		** did via the ulh_create call).
		*/
		QEF_DATA	    *datablk;	/* ptr to linked list of iiqrytext
					    ** data blocks read from QEF */
		i4	    tupleno;    /* number of data blocks left to be
					    ** scanned in the datablk list */

		status = rdu_qget(global, &eof_found);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		datablk = global->rdf_qeucb.qeu_output; /* data block currently
					    ** being analyzed */

		for (tupleno = global->rdf_qeucb.qeu_count; 
		    (tupleno--)>0;
		    datablk = datablk->dt_next)
		{
		    DB_IIQRYTEXT	    *iiqrytextp;

		    iiqrytextp = (DB_IIQRYTEXT *)(datablk->dt_data);
		    if ((iiqrytextp->dbq_mode != DB_DBP)
			||
			(iiqrytextp->dbq_seq != (sequence++))
			||
			((u_i4)iiqrytextp->dbq_text.db_t_count > (u_i4)remaining)
		       )
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0138_QUERYTEXT);	/* inconsistent
					** iiqrytext tuple */
			return(status);
		    }
		    MEcopy( (PTR)iiqrytextp->dbq_text.db_t_text, 
			iiqrytextp->dbq_text.db_t_count, 
			(PTR)&textp[current]);
		    current += iiqrytextp->dbq_text.db_t_count;
		    remaining -= iiqrytextp->dbq_text.db_t_count;
		}
	    }
	    if (remaining)
	    {	/* the query text was not completely read so report error */
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0138_QUERYTEXT);	/* inconsistent
				** iiqrytext tuple */
		return(status);
	    }
	    /* close the open iiqrytext catalog */
	    status = rdu_qclose(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	Rdi_svcb->rdvstat.rds_b5_proc++;
	/* retake the semaphore to update the object's status */
	status = rdu_gsemaphore(global);  
	if (DB_FAILURE_MACRO(status))
	    return(status);
	/* indicate that this object has be successfully created and can be
	** shared by many threads 
	*/
	rdf_ulhobject->rdf_state = RDF_SSHARED; 
	/* now release the semaphore one final time. */
	status = rdu_rsemaphore(global);  
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* do not define alias until the procedure object has been completely
    ** initialized, thus if procedure object is successfully looked up by
    ** name then the object should be initialized */

    status = ulh_define_alias(&global->rdf_ulhcb, (u_char *)&obj_alias, 
			      sizeof(obj_alias)); /* define alias by name for
						  ** future lookups by PSF */
    if (DB_FAILURE_MACRO(status))
    {
	if ((global->rdf_ulhcb.ulh_error.err_code != E_UL0120_DUP_ALIAS)
	    &&
	    (global->rdf_ulhcb.ulh_error.err_code != E_UL0122_ALIAS_REDEFINED))
	{
	    rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		E_RD0045_ULH_ACCESS,0);
	    return(status);
	}
	else
	    status = E_DB_OK;		/* reset status if object obtained */
    }
    rdfcb->rdf_info_blk = rdf_ulhobject->rdf_sysinfoblk; /* this is used
					** to mark the procedure as being fixed */
    global->rdfcb->rdf_rb.rdr_procedure =
	((RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->ulh_uptr)->rdf_procedure;
    return(status);
}

/*{
** Name: rdu_event	- Read an event from iievent catalog
**
** Description:
**      This routine will read a single event into the user event tuple.
**	The method to retrieve the event tuple will be to use rdu_qopen (to
**	open the iievent catalog),  rdu_qget and rdu_qclose.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_EVENT | RDR_BY_NAME
**	     .rdr_evname		Event name
**	     .rdr_owner			Event owner
**	     .rdr_qtuple_count		Number of event tuples - must be 1.
**
** Outputs:
**      global.rdfcb
**	     .rdr_qrytuple		Must be allocated event tuple into
**					which result will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-dec-90 (neil)	(6.5: 27-oct-89)
**          Written for Terminator II/alerters
**	01-feb-91 (teresa)
**	    improved statistics reporting
**	01-jul-92 (andre)
**	    added support for requesting dbevent information by id
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    change interface for rdu_qopen
**	22-dec-93 (robf)
**          set evrdqef.qry_cnt to 1 to avoid QEF AV when looking up by EV_ID
*/
static DB_STATUS
rdu_event(RDF_GLOBAL *global)
{
    bool		eof_found;	/* To pass to rdu_qget */
    QEF_DATA		evqefdata;	/* For QEU extraction call */
    RDD_QDATA		evrdqef;	/* For rdu_qopen to use */
    RDF_CB		*rdfcb = global->rdfcb;
    u_i4		ev_tidp;
    DB_IIXEVENT		xevent;
    bool		ev_by_id;

    char		key[1];

    DB_STATUS		status, end_status;

    ev_by_id=(~rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME) != 0;
    /* Validate parameters to request */
    if (!rdfcb->rdf_rb.rdr_qrytuple || rdfcb->rdf_rb.rdr_qtuple_count != 1)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /*
    ** if caller requested dbevent info by id, we need to retrieve a tuple from
    ** IIXEVENT and get the tidp of the IIEVENT tuple we will be getting
    */
    if (ev_by_id)
    {
	evqefdata.dt_next = NULL;
	evqefdata.dt_size = sizeof(DB_IIXEVENT);
	evqefdata.dt_data = (PTR) &xevent;
	evrdqef.qrym_alloc = evrdqef.qrym_cnt = 1;
	evrdqef.qrym_data = &evqefdata;

	/* Access IIXEVENT catalog by id to extract tidp of IIEVENT tuple */
	rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	rdfcb->rdf_rb.rdr_qtuple_count = 0;
	status = rdu_qopen(global, RD_IIXEVENT, (char *) NULL,
			   sizeof(DB_IIXEVENT), (PTR) &evrdqef,
			   0 /* 0 datasize means RDD_QDATA is set up */);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	/* Fetch the tuple */
	status = rdu_qget(global, &eof_found);
	if (!DB_FAILURE_MACRO(status))
	{
	    if (global->rdf_qeucb.qeu_count == 0 || eof_found)
	    {
		status = E_DB_WARN;
		rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	    }
	    else
	    {
		ev_tidp = xevent.dbx_tidp;
	    }
	}
	end_status = rdu_qclose(global); 	/* Close file */
	if (end_status > status)
	    status = end_status;

	/*
	** if tuple was not found or some other error was encountered,
	** we cannot continue
	*/
	if (status != E_DB_OK)
	    return(status);
    }
    
    /* Use the user data with a local RDF/QEF data request block */
    evqefdata.dt_next = NULL;
    evqefdata.dt_size = sizeof(DB_IIEVENT);
    evqefdata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
    evrdqef.qrym_cnt  = 1;   /* Just 1 event */
    evrdqef.qrym_alloc = evrdqef.qrym_cnt;
    evrdqef.qrym_data = &evqefdata;

    /* Access iievent catalog to extract event tuple by name */
    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    rdfcb->rdf_rb.rdr_qtuple_count = 0;

    /* if planning to access IIEVENT by TID, no keys will be used */
    if (ev_by_id)
    {
	key[0]='T';
    }
    else
    {
	key[0] = '\0';
    }
    
    status = rdu_qopen(global, RD_IIEVENT, (char *) key,
		       sizeof(DB_IIEVENT),
		       (PTR) &evrdqef,
		       0 /* 0 datasize means RDD_QDATA is set up */);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (ev_by_id)
    {
	global->rdf_qeucb.qeu_flag = QEU_BY_TID;
	global->rdf_qeucb.qeu_tid = ev_tidp;
    }

    /* Fetch the tuple */
    status = rdu_qget(global, &eof_found);
    if (!DB_FAILURE_MACRO(status))
    {
	if (global->rdf_qeucb.qeu_count == 0 || eof_found)
	{
	    status = E_DB_WARN;
	    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	}
	else
	{
	    DB_IIEVENT	    *ev_tup = (DB_IIEVENT *) evqefdata.dt_data;

	    if (ev_by_id)
	    {
		STRUCT_ASSIGN_MACRO(ev_tup->dbe_name,
		    rdfcb->rdf_rb.rdr_name.rdr_evname);
		STRUCT_ASSIGN_MACRO(ev_tup->dbe_owner, rdfcb->rdf_rb.rdr_owner);
	    }
	    else
	    {
		rdfcb->rdf_rb.rdr_tabid.db_tab_base =
		    ev_tup->dbe_uniqueid.db_tab_base;
		rdfcb->rdf_rb.rdr_tabid.db_tab_index =
		    ev_tup->dbe_uniqueid.db_tab_index;
	    }
	}
    }
    end_status = rdu_qclose(global); 	/* Close file */
    if (end_status > status)
	status = end_status;
    if (DB_SUCCESS_MACRO(status))
	Rdi_svcb->rdvstat.rds_b7_event++;
    return(status);
} /* rdu_event */

/*{
** Name: rdu_sequence	- Read a sequence from iisequence catalog
**
** Description:
**      This routine will read a single sequence into the user sequence 
**	tuple. This will be done by successive calls to rdu_qopen (to open
**	the iisequence catalog), rdu_qget and rdu_qclose.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_BY_NAME
**	     .rdr_2types_mask		RDR2_SEQUENCE
**	     .rdr_seqname		Sequence name
**	     .rdr_owner			Sequence owner
**	     .rdr_qtuple_count		Number of sequence tuples - must be 1.
**
** Outputs:
**      global.rdfcb
**	     .rdr_qrytuple		Must be allocated event tuple into
**					which result will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-mar-02 (inkdo01)
**	    Written for sequence support.
*/
static DB_STATUS
rdu_sequence(RDF_GLOBAL *global)
{
    bool		eof_found;	/* To pass to rdu_qget */
    QEF_DATA		seqqefdata;	/* For QEU extraction call */
    RDD_QDATA		seqrdqef;	/* For rdu_qopen to use */
    RDF_CB		*rdfcb = global->rdfcb;

    char		key[1];

    DB_STATUS		status, end_status;

    /* Validate parameters to request */
    if (!rdfcb->rdf_rb.rdr_qrytuple || rdfcb->rdf_rb.rdr_qtuple_count != 1)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* Use the user data with a local RDF/QEF data request block */
    seqqefdata.dt_next = NULL;
    seqqefdata.dt_size = sizeof(DB_IISEQUENCE);
    seqqefdata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
    seqrdqef.qrym_alloc = seqrdqef.qrym_cnt = 1;	/* just 1 sequence */
    seqrdqef.qrym_data = &seqqefdata;

    /* Access iisequence catalog to extract sequence tuple by name */
    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    rdfcb->rdf_rb.rdr_qtuple_count = 0;
    key[0] = '\0';
    
    status = rdu_qopen(global, RD_IISEQUENCE, (char *) key,
		       sizeof(DB_IISEQUENCE),
		       (PTR) &seqrdqef,
		       0 /* 0 datasize means RDD_QDATA is set up */);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Fetch the tuple */
    status = rdu_qget(global, &eof_found);
    if (!DB_FAILURE_MACRO(status))
    {
	if (global->rdf_qeucb.qeu_count == 0 || eof_found)
	{
	    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
	    status = E_DB_ERROR;
	}
	else
	{
	    DB_IISEQUENCE    *seq_tup = (DB_IISEQUENCE *) seqqefdata.dt_data;

	    rdfcb->rdf_rb.rdr_tabid.db_tab_base =
		seq_tup->dbs_uniqueid.db_tab_base;
	    rdfcb->rdf_rb.rdr_tabid.db_tab_index =
		seq_tup->dbs_uniqueid.db_tab_index;
	}
    }
    end_status = rdu_qclose(global); 	/* Close file */
    if (end_status > status)
	status = end_status;
    if (DB_SUCCESS_MACRO(status))
	Rdi_svcb->rdvstat.rds_b21_seqs++;
    return(status);
} /* rdu_sequence */

/*{
** Name: rdu_evsqprotect	- Read permit tuples for a specific event or sequence.
**
** Description:
**      This routine will read a block of permit tuples associated with
**	a single event or sequence.  Note that we do not go through the normal permit
**	interface as table-related permit tuples may be cached and can
**	later be invalidated by checking the timestamp of the table.  Event/sequence-
**	related permit tuples must be retrieved and checked on each event/sequence
**	access as they are not related to any timestamped object at execution
**	time.  Since these tuples are not cached they will be extracted
**	through a simple interface that does not require RDF to allocate
**	an information block to be made available to others.
**
**	As with normal permit retrieval the this routine will use rdu_qopen (to
**	open iiprotect), rdu_qget (to extract the permit tuples, possibly with
**	more calls to RDR_GETNEXT) and then rdu_qclose.
**
**	NOTE: this was originally written as rdu_evprotect for event protections
**	only. But the logic to retrieve sequence protections is identical and the
**	function was extended to handle events and sequences.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_EVENT|RDR_QTUPLES_ONLY|RDR_PROTECT
**	     .rdr_2types_mask		RDR2_SEQUENCE
**	     .rdr_tabid			Unique event/sequence id
**	     .rdr_qtuple_count		Number of iiprotect tuples to retrieve
**
** Outputs:
**      global.rdfcb
**	     .rdr_qtuple_count		Number of iiprotect tuples actually
**					retrieved into rdr_qrytuple.  This will
**					be <= than the number requested on 
**					input.
**	     .rdr_qrytuple		Must be an allocated set of iiiprotect
**					tuple into which the result tuples
**					will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-dec-90 (neil)	(6.5: 27-oct-89)
**          Written for Terminator II/alerters
**	01-feb-91 (teresa)
**	    Modified use of RDF_RMULTIPLE_CALLS to only be set if the iiprotect
**	    table is being left open between RDF calls.  Also, upgraded RDF
**	    statistics reporting.
**	09-jul-92 (teresa)
**	    Update statistics with non-found counters for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	11-sep-92 (teresa)
**	    change interface for rdu_qopen
**	28-jan-93 (teresa)
**	    move RDR_QTUPLE_ONLY from RDF_RB to RDF_RTYPES.RDF_QTUPLE_ONLY.
**	6-may-02 (inkdo01)
**	    Extend support to retrieve either event or sequence protections.
*/
static DB_STATUS
rdu_evsqprotect(RDF_GLOBAL *global)
{
    bool		eof_found;	/* To pass to rdu_get */
    QEF_DATA		protqdata[RDF_TUPLE_RETRIEVE];	/* For QEU extraction */
    QEF_DATA		*qd, *prev_qd;	/* QEF_DATA initialization */
    DB_PROTECTION	*protups;	/* User protection tuples */
    i4			qnum;
    RDD_QDATA		protrdqef;	/* For rdu_qopen to use */
    RDF_CB		*rdfcb;
    DB_STATUS		status;
    bool		event_not_sequence;

    rdfcb = global->rdfcb;
    event_not_sequence = ((rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT) != 0);

    /* Validate parameters to request */
    if (   (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT) == 0
	|| (global->rdf_resources & RDF_QTUPLE_ONLY) == 0
        || (   (rdfcb->rdf_rb.rdr_update_op != RDR_CLOSE)
	    && (   (rdfcb->rdf_rb.rdr_qrytuple == NULL)
		|| (rdfcb->rdf_rb.rdr_qtuple_count < 1)
	       )
	   )
	|| event_not_sequence && rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE
	|| !event_not_sequence && !(rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* see which operation is requested and handle */
    if (rdfcb->rdf_rb.rdr_update_op == RDR_CLOSE)
	status = rdu_qclose(global); 	/* Close file */
    else 
    {
	/* this is RDF_OPEN or RDR_GETNEXT.  Processing for both is similar,
	** so they are handled together */

	if (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)    
	{
	    if (event_not_sequence)
		Rdi_svcb->rdvstat.rds_r13_etups++;
	    else Rdi_svcb->rdvstat.rds_r22_stups++;
	}
	else if (rdfcb->rdf_rb.rdr_update_op == RDR_GETNEXT)
	    global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	else
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	    return (status);
	}

	/* 
	** Set up the local set of QEF data request blocks to point at the
	** set of tuples that the user has provided.
	*/
	protups = (DB_PROTECTION *)rdfcb->rdf_rb.rdr_qrytuple;
	for (qnum = 0, prev_qd = NULL, qd = protqdata;
	     qnum < min(rdfcb->rdf_rb.rdr_qtuple_count, RDF_TUPLE_RETRIEVE);
	     qnum++, qd++)
	{
	    qd->dt_size = sizeof(DB_PROTECTION); 
	    qd->dt_data = (PTR)&protups[qnum];
	    if (prev_qd != NULL)
		prev_qd->dt_next = qd;
	    prev_qd = qd;
	}
	if (prev_qd != NULL)
	    prev_qd->dt_next = qd;
	protrdqef.qrym_cnt  = qnum;
	protrdqef.qrym_alloc = qnum;
	protrdqef.qrym_data = protqdata;

	if (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)
	{
	    /* Open iiprotect catalog to extract protect tuple by unique id */
	    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, RD_IIPROTECT, 
			       (char *)NULL,
			       sizeof(DB_PROTECTION),
			       (PTR)&protrdqef,
			       0 /* 0 datasize means RDD_QDATA is set up */);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	else			/* On GETNEXT restore some fields for QEU */
	{
	    global->rdf_qeucb.qeu_tup_length = sizeof(DB_PROTECTION);;
	    global->rdf_qeucb.qeu_count = protrdqef.qrym_cnt; 
	    global->rdf_qeucb.qeu_output = protrdqef.qrym_data; 
	    global->rdf_init |= RDF_IQDATA;	/* Inform rdu_qget of setup */
	}
	status = rdu_qget(global, &eof_found);	 /* Fetch the tuples */
	if (DB_SUCCESS_MACRO(status))
	{
	    if (eof_found)
	    {
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count == 0)
		{
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);

		    /* update not-found statistics */
		    if (event_not_sequence)
			Rdi_svcb->rdvstat.rds_z13_etups++;
		    else Rdi_svcb->rdvstat.rds_z22_stups++;
		}
		else
		{
		    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
		    if (event_not_sequence)
			Rdi_svcb->rdvstat.rds_b13_etups++;
		    else Rdi_svcb->rdvstat.rds_b22_stups++;
		}
	    }
	    else
	    {
		global->rdf_resources |= RDF_RMULTIPLE_CALLS;
		if (event_not_sequence)
		    Rdi_svcb->rdvstat.rds_m13_etups++;
		else Rdi_svcb->rdvstat.rds_m22_stups++;
	    }

	    /* Return to user the number of tuples retrieved */
	    rdfcb->rdf_rb.rdr_qtuple_count = global->rdf_qeucb.qeu_count;

	} /* endif the fetch was successful */
    }
    return (status);
} /* rdu_evsqprotect */

#if 0
/*{
** Name: rdu_i_privileges   - Read IIPRIV tuples for a specified object.
**
** Description:
**      This routine will read a block of IIPRIV tuples from the set of tuples
**	constituting a descriptor of privileges on which an object (dbproc or
**	view) depends.
**	Tuples will be returned inside a buffer supplied by the caller.
**	Since these tuples are not cached they will be extracted
**	through a simple interface that does not require RDF to allocate
**	an information block to be made available to others.
**
**	This routine will use rdu_qopen (to open IIPRIV), rdu_qget (to extract
**	IIPRIV tuples, possibley with more calls to RDR_GETNEXT) and then
**	rdu_qclose.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_PROCEDURE | RDR_QTUPLES_ONLY or
**					RDR_VIEW | RDR_QTUPLES_ONLY
**	     .rdr_2types_mask		RDR2_INDEP_PRIVILEGE_LIST
**	     .rdr_tabid			Unique dbproc or view id
**	     .rdr_qtuple_count		Number of IIPRIV tuples to retrieve
**
** Outputs:
**      global.rdfcb
**	     .rdr_qtuple_count		Number of IIPRIV tuples actually
**					retrieved into rdr_qrytuple.  This will
**					be <= than the number requested on 
**					input.
**	     .rdr_qrytuple		Must be an allocated set of IIPRIV
**					tuple into which the result tuples
**					will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jun-92 (andre)
**	    written for GRANT/REVOKE project
**	14-sep-92 (teresa)
**	    change interface to rdu_qopen
**	28-jan-93 (teresa)
**	    move RDR_QTUPLE_ONLY from RDF_RB to RDF_RTYPES.RDF_QTUPLE_ONLY.
*/
static DB_STATUS
rdu_i_privileges(RDF_GLOBAL	*global)
{
    bool		eof_found;	/* To pass to rdu_get */
    QEF_DATA		privqdata[RDF_TUPLE_RETRIEVE];	/* For QEU extraction */
    QEF_DATA		*qd, *prev_qd;	/* QEF_DATA initialization */
    DB_IIPRIV		*priv_tup;
    i4			qnum;
    i4			tups_to_get;
    RDD_QDATA		rdqef;	/* For rdu_qopen to use */
    RDF_CB		*rdfcb;
    DB_STATUS		status;

    rdfcb = global->rdfcb;

    /* Validate parameters to request */
    if (   (~rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE &&
	    ~rdfcb->rdf_rb.rdr_types_mask & RDR_VIEW)
        || ~global->rdf_resources & RDF_QTUPLE_ONLY
	|| ~rdfcb->rdf_rb.rdr_2types_mask & RDR2_INDEP_PRIVILEGE_LIST
        || (   (rdfcb->rdf_rb.rdr_update_op != RDR_CLOSE)
	    && (   (rdfcb->rdf_rb.rdr_qrytuple == NULL)
		|| (rdfcb->rdf_rb.rdr_qtuple_count < 1)
	       )
	   )
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* see which operation is requested and handle */
    if (rdfcb->rdf_rb.rdr_update_op == RDR_CLOSE)
	status = rdu_qclose(global); 	/* Close file */
    else 
    {
	/*
	** this is RDF_OPEN or RDR_GETNEXT.  Processing for both is similar,
	** so they are handled together
	*/

	if (rdfcb->rdf_rb.rdr_update_op == RDR_GETNEXT)
	    global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	else if (rdfcb->rdf_rb.rdr_update_op != RDR_OPEN)
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	    return (status);
	}

	/* 
	** Set up the local set of QEF data request blocks to point at the
	** set of tuples that the user has provided.
	*/
	priv_tup = (DB_IIPRIV *) rdfcb->rdf_rb.rdr_qrytuple;
	tups_to_get = min(rdfcb->rdf_rb.rdr_qtuple_count, RDF_TUPLE_RETRIEVE);

	for (qnum = 0, prev_qd = NULL, qd = privqdata;
	     qnum < tups_to_get;
	     qnum++, qd++, priv_tup++)
	{
	    qd->dt_size = sizeof(DB_IIPRIV); 
	    qd->dt_data = (PTR) priv_tup;
	    if (prev_qd != NULL)
		prev_qd->dt_next = qd;
	    prev_qd = qd;
	}
	qd->dt_next = NULL;
	
	rdqef.qrym_cnt  = rdqef.qrym_alloc = qnum;
	rdqef.qrym_data = privqdata;

	if (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)
	{
	    i2	    descr_no = 0;
	    
	    /*
	    ** Open IIPRIV catalog to extract tuple by object id and descriptor
	    ** number - for the time being, there will be at most one privilege
	    ** descriptor created for a dbproc - its number will be 0, but there
	    ** may be multiple descriptors created for a view - fortunately the
	    ** one that describes privileges on which the view depends will also
	    ** be numbered 0
	    */
	    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, RD_IIPRIV, 
			       (char *) &descr_no,
			       sizeof(DB_IIPRIV),
			       (PTR) &rdqef,
			       0 /* 0 datasize means RDD_QDATA is set up */);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	else			/* On GETNEXT restore some fields for QEU */
	{
	    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIPRIV);
	    global->rdf_qeucb.qeu_count = rdqef.qrym_cnt; 
	    global->rdf_qeucb.qeu_output = rdqef.qrym_data; 
	    global->rdf_init |= RDF_IQDATA;	/* Inform rdu_qget of setup */
	}
	status = rdu_qget(global, &eof_found);	 /* Fetch the tuples */
	if (DB_SUCCESS_MACRO(status))
	{
	    if (eof_found)
	    {
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count == 0)
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
		else
		{
		    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
		}
	    }
	    else
	    {
		global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	    }

	    /* Return to user the number of tuples retrieved */
	    rdfcb->rdf_rb.rdr_qtuple_count = global->rdf_qeucb.qeu_count;

	} /* endif the fetch was successful */
    }
    return (status);
} /* rdu_i_privileges */
#endif

/*{
** Name: rdu_i_objects   - Read IIDBDEPENDS tuples for a specified object.
**
** Description:
**      This routine will read a block of IIDBDEPENDS tuples from the set of
**	tuples describing DBMS objects on which a given object (dbproc or view)
**	depends. Tuples will be returned inside a buffer supplied by the caller.
**	Since IIDBDEPENDS is keyed on independent object id, we will have to
**	retrieve tuples from IIXDBDEPENDS and then perform TID-join into
**	IIDBDEPENDS.
**	
**	Since these tuples are not cached they will be extracted
**	through a simple interface that does not require RDF to allocate
**	an information block to be made available to others.
**
**	This routine will use rdu_qopen (to open IIXDBDEPENDS and IIDBDEPENDS),
**	rdu_qget (to extract IIXDBDEPENDS tuples by key, possibley with more
**	calls to RDR_GETNEXT and IIDBDEPENDS tuples by TID) and then rdu_qclose.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_PROCEDURE | RDR_QTUPLES_ONLY or
**					RDR_VIEW | RDR_QTUPLES_ONLY
**	     .rdr_2types_mask		RDR2_INDEP_OBJECT_LIST
**	     .rdr_tabid			Unique dbproc or view id
**	     .rdr_qtuple_count		Number of IIDBDEPENDS tuples to retrieve
**
** Outputs:
**      global.rdfcb
**	     .rdr_qtuple_count		Number of IIDBDEPENDS tuples actually
**					retrieved into rdr_qrytuple.  This will
**					be <= than the number requested on 
**					input.
**	     .rdr_qrytuple		Must be an allocated set of IIDBDEPENDS
**					tuples into which the result tuples
**					will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jun-92 (andre)
**	    written for GRANT/REVOKE project
**	14-sep-92 (teresa)
**	    change interface to rdu_qopen
**	28-jan-93 (teresa)
**	    move RDR_QTUPLE_ONLY from RDF_RB to RDF_RTYPES.RDF_QTUPLE_ONLY.
*/
static DB_STATUS
rdu_i_objects(RDF_GLOBAL    *global)
{
    DB_TAB_ID		dep_id;	    /* Id of IIDBDEPENDS table */
    bool		eof_found;	/* To pass to rdu_get */
    QEF_DATA		xdepqdata[RDF_TUPLE_RETRIEVE];	/* For QEU extraction */
    QEF_DATA		*qd, *prev_qd;	/* QEF_DATA initialization */
    DB_IIXDBDEPENDS     xdep_tups[RDF_TUPLE_RETRIEVE];	/* index tuples will be
							** placed here */
    DB_IIXDBDEPENDS	*xdeptuple;
    i4			qnum;
    i4			tups_to_get;
    RDD_QDATA		xdeprdqef;	/* For rdu_qopen to use */
    RDF_CB		*rdfcb;
    DB_STATUS		status, local_status;

    rdfcb = global->rdfcb;

    /* Validate parameters to request */
    if (   (~rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE &&
	    ~rdfcb->rdf_rb.rdr_types_mask & RDR_VIEW)
	|| ~global->rdf_resources & RDF_QTUPLE_ONLY
	|| ~rdfcb->rdf_rb.rdr_2types_mask & RDR2_INDEP_OBJECT_LIST
        || (   (rdfcb->rdf_rb.rdr_update_op != RDR_CLOSE)
	    && (   (rdfcb->rdf_rb.rdr_qrytuple == NULL)
		|| (rdfcb->rdf_rb.rdr_qtuple_count < 1)
	       )
	   )
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* see which operation is requested and handle */
    if (rdfcb->rdf_rb.rdr_update_op == RDR_CLOSE)
	status = rdu_qclose(global); 	/* Close file */
    else 
    {
	/*
	** this is RDF_OPEN or RDR_GETNEXT.  Processing for both is similar,
	** so they are handled together
	*/

	if (rdfcb->rdf_rb.rdr_update_op == RDR_GETNEXT)
	    global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	else if (rdfcb->rdf_rb.rdr_update_op != RDR_OPEN)
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	    return (status);
	}

	/* 
	** Set up the local set of QEF data request blocks to point at the
	** set of IIXDBDEPENDS tuples which were allocated on stack
	*/
	tups_to_get = min(rdfcb->rdf_rb.rdr_qtuple_count, RDF_TUPLE_RETRIEVE);

	for (qnum = 0, prev_qd = NULL, qd = xdepqdata, xdeptuple = xdep_tups;
	     qnum < tups_to_get;
	     qnum++, qd++, xdeptuple++)
	{
	    qd->dt_size = sizeof(DB_IIXDBDEPENDS); 
	    qd->dt_data = (PTR) xdeptuple;
	    if (prev_qd != NULL)
		prev_qd->dt_next = qd;
	    prev_qd = qd;
	}
	qd->dt_next = NULL;
	
	xdeprdqef.qrym_cnt = xdeprdqef.qrym_alloc = qnum;
	xdeprdqef.qrym_data = xdepqdata;

	if (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)
	{
	    /* will read independent objects on which an object depends */
	    i4		obj_type =
		(rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE) ? DB_DBP
							       : DB_VIEW;  
	    
	    /*
	    ** Open IIXDBDEPENDS catalog to extract tuple by object id and
	    ** object type (DB_DBP)
	    */
	    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, RD_IIDBXDEPENDS, 
			       (char *) &obj_type,
			       sizeof(DB_IIXDBDEPENDS),
			       (PTR) &xdeprdqef,
			       0 /* 0 datasize means RDD_QDATA is set up */);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	else			/* On GETNEXT restore some fields for QEU */
	{
	    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIXDBDEPENDS);
	    global->rdf_qeucb.qeu_count = xdeprdqef.qrym_cnt; 
	    global->rdf_qeucb.qeu_output = xdeprdqef.qrym_data; 
	    global->rdf_init |= RDF_IQDATA;	/* Inform rdu_qget of setup */
	}
	status = rdu_qget(global, &eof_found);	 /* Fetch the tuples */
	if (DB_SUCCESS_MACRO(status))
	{
	    if (eof_found)
	    {
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count == 0)
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
		else
		{
		    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
		}
	    }
	    else
	    {
		global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	    }

	    /* Return to user the number of tuples retrieved */
	    if (global->rdf_qeucb.qeu_count)
	    {
		/*
		** need to make a copy of global since we need to open the index
		** and the table at the same time and the index may have to
		** remain open across calls
		*/
		RDF_GLOBAL	dep_global;

		QEF_DATA	depqdata;   /* for TID-join into IIDBDEPENDS */
		DB_IIDBDEPENDS	*dep_tup;
		RDD_QDATA	deprdqef;   /* For rdu_qopen to use */

		
		STRUCT_ASSIGN_MACRO((*global), dep_global);

		/*
		** RDF_RQEU bit could be set to indicate that we have already opened
		** IIXDBDEPENDS and RDF_RMULTIPLE_CALLS could be set if we don't manage
		** to read all IIXDBDEPENDS tuples during one call
		*/
		dep_global.rdf_resources = 0;

		/*
		** now we will open IIDBDEPENDS and do a TID-join into it using
		** IIXDBDEPENDS tuples retrieved so far.  For each tuple, we
		** will verify that it represents some dbproc on which the
		** specified dbproc depends - if not, the tuple will be
		** discarded;  since we will retrieve by TID, we will be getting
		** one tuple at a time
		*/

		rdfcb->rdf_rb.rdr_qtuple_count = 0;
		
		depqdata.dt_size = sizeof(DB_IIDBDEPENDS);
		depqdata.dt_next = NULL;
		deprdqef.qrym_cnt = deprdqef.qrym_alloc = 1;
		deprdqef.qrym_data = &depqdata;

		/* now we open IIDBDEPENDS */
		dep_id.db_tab_base = DM_B_DEPENDS_TAB_ID;
		dep_id.db_tab_index = DM_I_DEPENDS_TAB_ID;

		dep_global.rdfcb->rdf_rb.rdr_rec_access_id = NULL;
		dep_global.rdfcb->rdf_rb.rdr_qtuple_count = 0;

		local_status = rdu_qopen(&dep_global, RD_IIDBDEPENDS,
					(char *) NULL,
					sizeof(DB_IIDBDEPENDS), 
					(PTR) &deprdqef, 0);
		if (DB_FAILURE_MACRO(local_status))
		{
		    status = local_status;

		    /*
		    ** caller does not know or care that we access IIDBDEPENDS
		    ** through IIXDBDEPENDS, so we must close IIDBDEPENDS
		    ** without caller's intervention
		    */
		    local_status = rdu_qclose(&dep_global);    /* Close file */
		    if (local_status > status)
			status = local_status;
		    return(status);
		}

		dep_tup = (DB_IIDBDEPENDS *) rdfcb->rdf_rb.rdr_qrytuple;
		dep_global.rdf_qeucb.qeu_flag = QEU_BY_TID;
		depqdata.dt_data = (PTR) dep_tup;

		for (qnum = 0, tups_to_get = global->rdf_qeucb.qeu_count,
		     xdeptuple = xdep_tups; 

		     tups_to_get > 0;

		     xdeptuple++, tups_to_get--)
		{
		    dep_global.rdf_qeucb.qeu_tid = xdeptuple->dbvx_tidp;
		    local_status = rdu_qget(&dep_global, &eof_found);   

		    /* 
		    ** we don't expect to get EOF since we are reading by tid - 
		    ** if tuple could not be found, QEF will return an error
		    */
		    if (DB_FAILURE_MACRO(local_status))
		    {
			status = local_status;
			local_status = rdu_qclose(&dep_global);   /* Close file */
			if (local_status > status)
			    status = local_status;
			return(status);
		    }

		    /* 
		    ** we are only interested in IIDBDEPENDS tuples describing
		    ** dbprocs on which the current dbproc depends
		    */
		    if (dep_tup->dbv_itype == DB_DBP)
		    {
			/* 
			** keep this tuple, read next tuple into next record
			** provided by the caller
			*/
			qnum++;
			depqdata.dt_data = (PTR) ++dep_tup;
		    }
		}

		global->rdf_qeucb.qeu_count = qnum;

		local_status = rdu_qclose(&dep_global);    /* Close file */
		if (local_status > status)
		    status = local_status;
	    }

	    /* Return to user the number of tuples retrieved */
	    rdfcb->rdf_rb.rdr_qtuple_count = global->rdf_qeucb.qeu_count;

	} /* endif the fetch was successful */
    }
    return (status);
} /* rdu_i_objects */

/*{
** Name: rdu_usrbuf_read   - Read catalog tuples into a user specifed buffer.
**
** Description:
**      This routine will read a block of tuples from a specified DBMS catalog.
**	Tuples will be returned inside a buffer supplied by the caller.
**	Since these tuples are not cached they will be extracted
**	through a simple interface that does not require RDF to allocate
**	an information block to be made available to others.
**
**	This routine will use rdu_qopen (to open the catalog), rdu_qget (to 
**	extract tuples, possibley with more calls to RDR_GETNEXT) and then
**	rdu_qclose.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		RDR_PROCEDURE | RDR_QTUPLES_ONLY or
**					RDR_VIEW | RDR_QTUPLES_ONLY
**	     .rdr_2types_mask		RDR2_INDEP_PRIVILEGE_LIST or 
**					RDR2_SYNONYM
**	     .rdr_tabid			Unique dbproc or view id
**	     .rdr_qtuple_count		Number of tuples to retrieve
**
** Outputs:
**      global.rdfcb
**	     .rdr_qtuple_count		Number of tuples actually
**					retrieved into rdr_qrytuple.  This will
**					be <= than the number requested on 
**					input.
**	     .rdr_qrytuple		Must be an allocated set of tuples into
**					which the result tuples are deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-feb-93 (teresa)
**	    Initial creation for constraints.
**	21-oct-93 (andre)
**	    added support for retrieving IISYNONYM tuple by synonym name and 
**	    owner
**	22-Mar-2005 (thaju02)
**	    Added support for retrieving IIDBDEPENDS tuple by tabid and 
**	    constraint type
*/
static DB_STATUS
rdu_usrbuf_read(RDF_GLOBAL	*global)
{
    bool		eof_found;	/* To pass to rdu_get */
    QEF_DATA		qdata[RDF_MAX_QTCNT];	/* For QEU extraction */
    QEF_DATA		*qd, *prev_qd;	/* QEF_DATA initialization */
    i4			qnum;
    i4			tups_to_get;
    RDD_QDATA		rdqef;		/* For rdu_qopen to use */
    RDF_CB		*rdfcb;
    DB_STATUS		status;
    PTR			tup_p;
    i4		structsize;		    
    RDF_TABLE		rdf_table;
    i4		special_param;

    rdfcb = global->rdfcb;

    /* Validate parameters to request    */
    if (
	((rdfcb->rdf_rb.rdr_2types_mask & 
	    (RDR2_INDEP_PRIVILEGE_LIST | RDR2_KEYS | RDR2_SYNONYM |
	    RDR2_DBDEPENDS))==0)
	    /* was not indep priv list, keys, or synonym request */
       ||
	    !rdfcb->rdf_rb.rdr_qrytuple	    /* buffer not specified */
       ||
	    (rdfcb->rdf_rb.rdr_update_op != RDR_CLOSE &&
	    !rdfcb->rdf_rb.rdr_qtuple_count) /* count not specified */
       ||
	    (!rdfcb->rdf_rb.rdr_tabid.db_tab_base && 
	     !rdfcb->rdf_rb.rdr_tabid.db_tab_index &&
	     rdfcb->rdf_rb.rdr_2types_mask & 
		 (RDR2_INDEP_PRIVILEGE_LIST | RDR2_KEYS))
	    /* constraint or view or permit id is missing */
       ||
	    (rdfcb->rdf_rb.rdr_update_op != RDR_OPEN) &&
	    (rdfcb->rdf_rb.rdr_update_op != RDR_GETNEXT) &&
	    (rdfcb->rdf_rb.rdr_update_op != RDR_CLOSE)
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_INDEP_PRIVILEGE_LIST)
    {
	/*
	** Open catalog to extract tuple by object id and descriptor
	** number - for the time being, there will be at most one privilege
	** descriptor created for a dbproc - its number will be 0, but there
	** may be multiple descriptors created for a view - fortunately the
	** one that describes privileges on which the view depends will also
	** be numbered 0
	*/
	special_param = 0;

	structsize = sizeof (DB_IIPRIV);
	rdf_table = RD_IIPRIV;
	if ((rdfcb->rdf_rb.rdr_types_mask & (RDR_PROCEDURE|RDR_VIEW))==0) 
	{
	   /* indep priv list but not view or procedure */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	    return (status);
	}
    }
    else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_KEYS)
    {
	structsize = sizeof (DB_IIKEY);
	special_param = 0;
	rdf_table = RD_IIKEY;
    }
    else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SYNONYM)
    {
	structsize = sizeof (DB_IISYNONYM);
	special_param = 0;
	rdf_table = RD_IISYNONYM;
    }
    else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_DBDEPENDS)
    {
	structsize=sizeof(DB_IIDBDEPENDS);
	special_param = 0;
	rdf_table = RD_IIDBDEPENDS;
    }
					    
    /* see which operation is requested and handle */
    if (rdfcb->rdf_rb.rdr_update_op == RDR_CLOSE)
	status = rdu_qclose(global); 	/* Close file */
    else 
    {
	/*
	** this is RDF_OPEN or RDR_GETNEXT.  Processing for both is similar,
	** so they are handled together
	*/

	if (rdfcb->rdf_rb.rdr_update_op == RDR_GETNEXT)
	    global->rdf_resources |= RDF_RMULTIPLE_CALLS;

	/* 
	** Set up the local set of QEF data request blocks to point at the
	** set of tuples that the user has provided.
	*/
	tups_to_get = min(rdfcb->rdf_rb.rdr_qtuple_count, RDF_TUPLE_RETRIEVE);
	tup_p = rdfcb->rdf_rb.rdr_qrytuple;	/* start of user buffer */

	for (qnum = 0, prev_qd = NULL, qd = qdata;
	     qnum < tups_to_get;
	     qnum++, qd++)
	{
	    qd->dt_next = NULL;
	    qd->dt_size = structsize; 
	    qd->dt_data = tup_p;
	    if (prev_qd != NULL)
		prev_qd->dt_next = qd;
	    prev_qd = qd;
	    tup_p += structsize;
	}
	rdqef.qrym_cnt  = rdqef.qrym_alloc = qnum;
	rdqef.qrym_data = qdata;

	if (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)
	{
	    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
	    rdfcb->rdf_rb.rdr_qtuple_count = 0;
	    status = rdu_qopen(global, rdf_table, (char *) &special_param, 
			       structsize, (PTR) &rdqef,
			       0 /* 0 datasize means RDD_QDATA is set up */);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	else			/* On GETNEXT restore some fields for QEU */
	{
	    global->rdf_qeucb.qeu_tup_length = structsize;
	    global->rdf_qeucb.qeu_count = rdqef.qrym_cnt; 
	    global->rdf_qeucb.qeu_output = rdqef.qrym_data; 
	    global->rdf_init |= RDF_IQDATA;	/* Inform rdu_qget of setup */
	}
	status = rdu_qget(global, &eof_found);	 /* Fetch the tuples */
	if (DB_SUCCESS_MACRO(status))
	{
	    if (eof_found)
	    {
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count == 0)
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
		else
		{
		    rdu_ierror(global, status, E_RD0011_NO_MORE_ROWS);
		}
	    }
	    else
	    {
		global->rdf_resources |= RDF_RMULTIPLE_CALLS;
	    }

	    /* Return to user the number of tuples retrieved */
	    rdfcb->rdf_rb.rdr_qtuple_count = global->rdf_qeucb.qeu_count;

	} /* endif the fetch was successful */
    }
    return (status);
}

/*{
** Name: rdf_getinfo - Request qrymod and statistics information of a table.
**
**	External call format:	status = rdf_call(RDF_GETINFO, &rdf_cb);
**
** Description:
**	This routine retrieve integrity, protection, tree, iiqrytext,
**	iiprocedure, iirule, iiistatistics, iievent & iihistogram info of a
**	table. Currently, one request is allowed for each rdf_getinfo() call.
**	
**	Because of the potential unbounded resources problem in the 
**	retrieve operation, the information(query text tuples and query
**	tree tuples) associated with integrity/protection/rule definitions of
**	a table can't be retrieved at once. Instead, a limited 
**	number of integrity/protection tuples is retrieved each time. 
**	The communication between PSF and RDF continues until a particular
**	integrity/protection/rule tuple in interest is found. Then, all the 
**	query tree information associated with this particular
**	tuple can be retrieved. RDF reuses the memory space of the 
**	tuples retrieved from the previous rdf_getinfo() call. This implies 
**	that the caller has to pass in the same table information 
**	block(rdf_cb.rdf_info_blk) in the subsequent rdf_getinfo()
**	calls for RDF to reuse the memory space.
**	
**	RDF internally opens and closes the table for the caller. If
**	RDR_GETNEXT is not specified, RDF opens the requested system 
**	catalogs and returns the tuples and the record access id(cursor) 
**	to the caller. The RDR_GETNEXT flag and the record access id
**	are required to inform RDF to get the next set of tuples without
**	opening the system catalogs again. RDF closes the table if it 
**	finds no more rows in the table or explicitly requested by the
**	caller through RDR_CLOSE flag.
**	
**  Usages:
**
**	To retrieve the procedure text (and other info) by id:
**
**	    rdr_types_mask = RDR_PROCEDURE
**
**	To retrieve the procedure text by name: 
**
**	    rdr_types_mask = RDR_PROCEDURE | RDR_BY_NAME;
**
**      Note that when obtaining dbproc info by name, RDR_BY_NAME access must be
**	specified and that the rdr_prc_name field and rdr_tabowner fields
**      must contain the procedure name and procedure owner; otherwise rdr_tabid
**	must contain dbproc id
**      The result will be returned in rdr_procedure.  A call to
**      RDF_UNFIX is required to release the lock on the text.
**          In order to support DBP GRANT, the same interface
**      which returns permit tuples for tables will be used.  This means
**      that an rdf_info block will be returned which contains the
**      permit tuples in rdr_ptuples.  In the rdf_info_blk block there will be a
**      rdr_dbp pointer to the DB_PROCEDURE structure which will contain
**      the table/procedure ID.  The call to RDF_UNFIX will release the
**      lock on this structure also.
**
**	To retrieve the tree information of an particular integrity :
**
**	    rdr_types_mask = RDR_INTEGRITIES | RDR_QTREE;
**
**	To retrieve the tree information of a particular protection :
**
**	    rdr_types_mask = RDR_PROTECT | RDR_QTREE;
**
**	To retrieve a view definition :
**
**	    rdr_types_mask = RDR_VIEW;
**
**	To retrieve the statistics information of a particular attribute :
**
**	    rdr_types_mask = RDR_STATISTICS;
**
**      To retrieve the information about a procedure
**
**          rdr_types_mask = RDR_PROCEDURE | RDR_BY_NAME
**
**	To retrieve the tree of a particular rule:
**
**	    rdr_types_mask = RDR_RULE | RDR_QTREE
**
**	To retrieve event tuples by dbevent name:
**
**	    rdr_types_mask = RDR_EVENT | RDR_BY_NAME
**
**	To retrieve event tuples by dbevent id :
**
**	    rdr_types_mask = RDR_EVENT
**
**	To retrieve iiintegrityidx tuples by constraint id:
**  
**	    rdr_2types_mask =  RDR2_CNS_INTEG
**	    RDF_RB.rdr_qrytuple	- PTR to PSF supplied buffer to read tuple into
**	    
** Inputs:
**
**      rdf_cb                          
**		.rdf_rb
**			.rdr_db_id	Database id.
**			.rdr_session_id Session id.
**			.rdr_tabid	Table id.
**                      .rdr_name.rdr_tabname - name of table
**                      .rdr_name.rdr_prcname - name of procedure
**                      .rdr_owner      name of owner for procedure
**                                      table/object
**			.rdr_types_mask	Type of information requested.
**					The mask can be:
**					RDR_PROCEDURE
**					RDR_INTEGRITIES
**					RDR_PROTECT
**					RDR_VIEW
**					RDR_STATISTICS
**					RDR_QTREE
**					RDR_RULE
**
**			.rdr_2types_mask RDR2_INDEP_PRIVILEGE_LIST
**					RDR2_INDEP_OBJECT_LIST
**					RDR2_CNS_INTEG
**
**			.rdr_hattr_no	OPF uses this field for specifying
**					the attribute number of the requested
**					statistics information.
**			.rdr_no_of_attr OPF uses this field for specifying
**					the total number of attributes in
**					the table.
**			.rdr_cellsize	OPF uses this field for specifying
**					cell data size of a histogram.
**			.rdr_qrymod_id	Protection or integrity number that
**					that wants to be retrieved.
**			.rdr_tree_id	Tree id for retrieving tree tuples of an
**					integrity,protection or rule definition.
**			.rdr_qtext_id;	query text id for retrieving query 
**					text tuples of a view, protection,
**					integrity or rule definition.
**			.rdr_fcb	A pointer to the structure which 
**					contains the poolid and caching
**					control information. All the table
**					information will be allocated in the
**					memory pool specified by the poolid.
**			.rdr_qrytuple	Ptr to buffer to constain tuples that
**					are not cached
**					
**		.rdf_info_blk		Pointer to the requested table 
**					information block. 
**					This must be the info block 
**					corresponding to the table id above.
**					This is for storing all the
**					information related to one table
**					together.
**
**
** Outputs:
**      rdf_cb                          
**		.rdf_rb
**			.rdr_rec_access_id The record access id for retrieving
**					   the next set of integrity, rule or
**					   protection tuples.
**			.rdr_protect	Pointer to protection information.
**			.rdr_integrity	Pointer to integrity information.
**			.rdr_procedure	Pointer to procedure information.
**			.rdr_rule	Pointer to rule information returned.
**					(This really the tree - if requested).
**			.rdr_qrytuple	Generic tuple pointer for simple
**					tuple retrievals.  This must be already
**					allocated by the caller for the specific
**					size.  For example, event tuples may
**					be retrieved into this field.  These
**					tuples need not be cached by RDF.
**		.rdf_info_blk		Pointer to the requested table 
**					information block. The pointer field is 
**					NULL if the corresponding information is
**					not requested.
**			.rdr_view	Pointer to view information.
**			.rdr_histogram	pointer to array of pointers to 
**					RDF revised version of iistatistics and 
**					iihistogram tuples.
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0201_PROC_NOT_FOUND
**					procedure definition not found, this
**                                      is a user error and will not be logged
**                                      by RDF.
**				E_RD0136_CONCURRENT_PROC
** 					invalid concurrent access to procedure
**					     object
**				E_RD0137_DUPLICATE_PROCS
**					more than one procedure defined with 
**                          		    same name
**				E_RD0138_QUERYTEXT
**					missing query text tuples
**				E_RD0001_NO_MORE_MEM
**					Out of memory.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of 
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of 
**					memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned
**					above.
**				E_RD000D_USER_ABORT
**					User abort.
**				E_RD0010_QEF_ERROR
**					Whatever error returned from QEF not
**					mentioned above.
**				E_RD0011_NO_MORE_ROWS
**					No more rows returned from QEU.
**					The system catalogs are internally
**					closed by RDF and rdd_qdata.qrym_cnt
**					contains the number of tuples actually
**					retrieved.
**				E_RD0012_BAD_HISTO_DATA
**					Bad histogram data.
**				E_RD0013_NO_TUPLE_FOUND
**					No tuple found.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	09-mar-89 (neil)
**	    modified to support rules.
**	27-oct-89 (neil)
**	    Rules - linted field names for rules.
**	    Alerters - added event extraction.
**	15-dec-89 (teg)
**	    add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**	    routines to use.
**	10-dec-90 (neil)
**	    Alerters: added event [permit] extraction.
**	28-may-92 (teresa)
**	    Merged in Star logic, made checksum calculation apply to local 
**	    cache as well as star cache, fixed bug from star code where checksum
**	    logic was being applied to sys_infoblk instead of usr_infoblk.
**	30-jun-92 (andre)
**	    defined interface to obtain list of objects and privileges on which
**	    a given dbproc depends;
**	    defined interface to obtain dbproc and dbevent info by id
**	09-jul-92 (teresa)
**	    Update statistics with non-found counters for SYBIL.
**	16-jul-92 (teresa)
**	    prototypes
**	14-sep-92 (teresa)
**	    change interface to rdu_rqrytupple to take table type instead of
**	    table id.
**	28-jan-93 (teresa)
**	    moved logic to read groups of tuples into routine rdf_readtuples().
**	25-feb-93 (teresa)
**	    modify to report E_RD0003 in non-xDEBUG mode when an invalid 
**	    parameter is supplied.
**	04-mar-93
**	  over and over to query iistatistics for tuples that do not exist.
**	16-feb-94 (teresa)
**	    refresh checksum whether or not tracepoint to skip setting
**	    checksums is set.  Also cleaned up checksum calculation for bug
**	    59336.
**      11-nov-94 (cohmi01)
**          Ensure that we checksum sys_infoblk if changed.
**	19-mar-02 (inkdo01)
**	    Add support for sequence tuple retrieval.
*/

DB_STATUS
rdf_getinfo(	RDF_GLOBAL	*global,
		RDF_CB		*rdfcb)
{

    DB_STATUS		status;
    RDF_TYPES		requests;
    RDF_TYPES		req2;
    RDR_INFO		*sys_infoblk = NULL;  /* init, so we know if changed */
    RDR_INFO		*usr_infoblk=rdfcb->rdf_info_blk;
    bool		create_procedure;/* TRUE if create procedure function is
					** requested */
    bool		get_event;	/* TRUE = request to get event data */
    bool		refreshed = FALSE;
    bool		no_infoblk = FALSE;

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */

    global->rdfcb = rdfcb;
    CLRDBERR(&rdfcb->rdf_error);
    requests = rdfcb->rdf_rb.rdr_types_mask;
    req2 = rdfcb->rdf_rb.rdr_2types_mask;
    create_procedure = (requests & RDR_PROCEDURE) != 0;

    /* Get_event does not use the RDF info block - uses rdr_qrytuple */
    get_event = (requests & RDR_EVENT) != 0;

    no_infoblk = ((req2 & RDR2_CNS_INTEG) != 0) || create_procedure ||
		 get_event || ((req2 & RDR2_SEQUENCE) != 0);

    /* Do this just in case xDEBUG is not set in later check */
    if(!no_infoblk)
    {	
	usr_infoblk = rdfcb->rdf_info_blk; /* get ptr to user info
					** block which may be the
					** same as the system info
					** block */
	global->rdf_ulhobject = (RDF_ULHOBJECT *)usr_infoblk->rdr_object;
	sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    }

#ifdef xDEBUG
    /* Check general input parameters. */
    if (rdfcb->rdf_rb.rdr_db_id == NULL ||
	rdfcb->rdf_rb.rdr_session_id == NULL ||
	( (requests & 
	    (RDR_PROCEDURE | RDR_INTEGRITIES | RDR_PROTECT | 
	     RDR_STATISTICS | RDR_VIEW | RDR_QTREE | RDR_RULE | RDR_EVENT)
	   == 0L) &&
	   (req2 & (RDR2_INDEP_PRIVILEGE_LIST | RDR2_INDEP_OBJECT_LIST |
	    	    RDR2_CNS_INTEG | RDR2_SEQUENCE)
	   == 0L)
	)
	||
	/* if requesting dbproc or dbevent info by id, id must be provided */
	(   (create_procedure || get_event)
	 && ~requests & RDR_BY_NAME
	 && !rdfcb->rdf_rb.rdr_tabid.db_tab_base
	)
	||
	(   
	    !create_procedure && !get_event && !no_infoblk
	 &&
	    (	
		!(usr_infoblk = rdfcb->rdf_info_blk)    /* get ptr to user info
							** block which may be the
							** same as the system info
							** block */
		||
		!(global->rdf_ulhobject = (RDF_ULHOBJECT *)usr_infoblk->rdr_object)
		||
		!(sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk)
		||
		!rdfcb->rdf_rb.rdr_tabid.db_tab_base
		||
		!rdfcb->rdf_info_blk
		||
		!rdfcb->rdf_info_blk->rdr_object
	    )
	)
	|| 
	rdfcb->rdf_rb.rdr_fcb == NULL
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }		
#endif

    if (req2 & RDR2_CNS_INTEG)
    {
	/* get constraints iiintegrityidx information */
	status = rdu_cnstr_integ(global);
    }
    else if (requests & RDR_PROCEDURE)
    {
	Rdi_svcb->rdvstat.rds_r5_proc++;
	/* get procedure object information */
	status = rdu_procedure(global,&refreshed);
    }
    else if (requests & RDR_EVENT)
    {
	Rdi_svcb->rdvstat.rds_r7_event++;
	status = rdu_event(global);	    /* Retrieve event tuple */
    }
    else if (req2 & RDR2_SEQUENCE)
    {
	Rdi_svcb->rdvstat.rds_r21_seqs++;
	status = rdu_sequence(global);	    /* Retrieve sequence tuple */
    }
    else if ( global->rdf_ulhobject &&
	      (global->rdf_ulhobject->rdf_sysinfoblk->rdr_invalid)
	    )
    {	/* If the system copy is invalidated, abort the function. */
	status = E_DB_SEVERE;
	rdu_ierror (global, status, E_RD000F_INFO_OUT_OF_DATE);
    }
    else if ( usr_infoblk &&
	      (usr_infoblk->rdr_invalid)
	    )
    {	/* If the system copy is invalidated, abort the function. */
	status = E_DB_SEVERE;
	rdu_ierror (global, status, E_RD000F_INFO_OUT_OF_DATE);
    }
    else if (requests & RDR_STATISTICS)
    {
	Rdi_svcb->rdvstat.rds_r11_stat++;
	status = rdu_statistics(global);    /* get histogram info for the
					    ** optimizer */
	usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_statistics, so
					    ** reget value */
	/* do any trace point processing */
	rdu_master_infodump(usr_infoblk, global, RDFGETINFO, requests);
    }
    else if (requests & RDR_VIEW)
    {
	Rdi_svcb->rdvstat.rds_r4_view++;
	status = rdu_view(global);	    /* get view info for the parser */
	usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_view, so
					    ** reget value */
	/* do any trace point processing */
	rdu_master_infodump(usr_infoblk, global, RDFGETINFO, requests);
    }
    else if (requests & RDR_INTEGRITIES)
    {
	Rdi_svcb->rdvstat.rds_r3_integ++;
	status = rdu_qtree(global, &rdfcb->rdf_rb.rdr_integrity, 
	    &Rdi_svcb->rdvstat.rds_b3_integ,
	    &Rdi_svcb->rdvstat.rds_c3_integ,
	    DB_INTG, RDR_INTEGRITIES, &refreshed);  /* read
					    ** parse tree for integrity */
    }
    else if (requests & RDR_PROTECT) 
    {
	Rdi_svcb->rdvstat.rds_r2_permit++;
	status = rdu_qtree(global, &rdfcb->rdf_rb.rdr_protect,
	    &Rdi_svcb->rdvstat.rds_b2_permit,
	    &Rdi_svcb->rdvstat.rds_c2_permit,
	    DB_PROT, RDR_PROTECT, &refreshed);	/* read parse
					    ** tree for protection */
    }
    else if (requests & RDR_RULE) 
    {
	Rdi_svcb->rdvstat.rds_r6_rule++;
	status = rdu_qtree(global, &rdfcb->rdf_rb.rdr_rule,
            &Rdi_svcb->rdvstat.rds_b6_rule,
            &Rdi_svcb->rdvstat.rds_c6_rule,
	    DB_RULE, RDR_RULE,&refreshed);    /* read parse tree for rule */
    }
    else
    {	/* invalid parameter */
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    /* update statistics on cache refresh */
    if (refreshed)
	Rdi_svcb->rdvstat.rds_x2_qtree++;

    return(status);
}

/*{
** Name: rdf_readtuples  - Read a set of tuples from a specified catalog into
**			   a buffer.
**
**	External call format:	status = rdf_call(RDF_READTUPLES, &rdf_cb);
**	=====================
**
**	This is a three part interface to read tuples from a DBMS catalog into
**	a buffer.  Rdf_readtuples does not reformat these tuple(s), but supplies
**	them directly to the user.  
**
**	RDF_READTUPLES Protocol:
**	========================
**
**	This is a three step protocol:
**	    1.  Open the catalog and read the first set of tuples.
**	    2.  Continue reading tuples until EOD.  This call should only occur
**		if EOD is NOT reached on step 1.
**	    3.	Close the catalog.
**
**	STEP ONE:
**	---------
**	  The caller must open the catalog by calling RDF_READTUPLES with an
**	  RDF_RB.rdr_update_op = RDR_OPEN.  RDF will open the catalog and start
**	  reading tuples.  One of three conditions will occur:
**	    - There is enough room in the buffer to hold all of the tuples.
**	      In this case, RDF_READTUPLES returns an error code of 
**	      E_RD0011_NO_MORE_ROWS.  (This should not be considered an error 
**	      by the caller, but is simply part of the communication protocol.)
**	    - There are no tuples which satisify the search condition.
**	      In this case RDF_READTUPLES returns an error code of 
**	      E_RD0013_NO_TUPLE_FOUND (and it is up to the caller to determine 
**	      whether this is an error or not.  In some cases, the caller does
**	      not expect to find any tuples).
**	    - There are more tuples than will fit into the read buffer.
**	      In this case, RDF sets an internal flag to indicate it is in
**	      a multicall read cycle (the RDF_RMULTIPLE_CALLS bit in 
**	      global->rdf_resources).  RDF returns a status of E_DB_OK.
**	  NOTE:  RDF_READTUPLES does NOT close the table in any of these cases.
**
**	STEP TWO:
**	---------
**	  The caller is expected to continue calling RDF_READTUPLES with 
**	  RDF_RB.rdr_update_op = RDR_GETNEXT until RDF reaches EOD and returns
**	  an error code of E_RD0011_NO_MORE_ROWS.  (If the first RDF_READTUPLES
**	  call returned E_RD0011_NO_MORE_ROWS or E_RD0013_NO_TUPLE_FOUND, do not
**	  call RDF_READTUPLES with RDF_RB.rdr_update_op = RDR_GETNEXT.)
**	  Also, the caller is NOT obligated to call RDF to read all possible 
**	  tuples.  If the caller finds what it is looking for, it does not need
**	  to continue reading tuples via RDR_GETNEXT.
**
**	STEP THREE:
**	-----------
**	  The caller MUST ALWAYS close an open catalog. (A catalog is considered
**	  open after the call to RDF_READTUPLES with rdr_update_op = RDR_OPEN,
**	  unless RDF_READTUPLES returns an error and the error code is not one
**	  of the expected protocol errors (E_RD0011_NO_MORE_ROWS or 
**	  E_RD0013_NO_TUPLE_FOUND.)  
**	  The caller must set RDF_RB.rdr_update_op = RDR_CLOSE to close the 
**	  catalog.
**	
**	Interface Details:
**	==================
**
**	RDF will read some tuples directly into user memory and will allocate
**	space on its cache to hold other tuples.  If RDF_READTUPLE must be
**	called with RDR_GETNEXT when it is using cache memory, RDF overwrites
**	the first set of tuples with the next.
**
**	RDF_READTUPLES retrieves the following sets of tuples:
**	    - iidbdepends tuples
**	    - integrities tuples
**	    - iipriv tuples
**	    - key tuples
**	    - permit tuples (for tables or events or procedures)
**	    - procedure parameters
**	    - rule tuples
**	    - security alarm tuples
**
**	The specific interfaces are as follows:
**
**	  IIDBDEPENDS tuples for views:
**	  ----------------------------
**	    RDR_RB.rdr_types_mask = RDR2_INDEP_OBJECT_LIST
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  IIDBDEPENDS tuples for procedures:
**	  ---------------------------------
**	    RDR_RB.rdr_types_mask  = RDR_PROCEDURE
**	    RDR_RB.rdr_2types_mask = RDR2_INDEP_OBJECT_LIST
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid	    
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  Key Tuples:
**	  ----------
**	    RDR_RB.rdr_2types_mask = RDR2_KEY_TUPLES
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify constraint id in RDR_RB.rdr_tabid	    
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  Integrity Tuples Associated with a Table:
**	  ----------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_INTEGRITIES
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid
**	    TUPLES RETURNED:  RDR_INFO.rdr_ituples  (cache memory)
**
**	  Permit Tuples Associated with a Table:
**	  -------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_PROTECT
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid	    
**	    TUPLES RETURNED:  RDR_INFO.rdr_ptuples  (cache memory)
**
**	  Permit Tuples Assocaited with an Event:
**	  --------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_EVENT | RDR_PROTECT
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify event id in RDR_RB.rdr_tabid
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  Permit Tuples Associated with a Sequence:
**	  ----------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_PROTECT
**	    RDR_RB.rdr_2types_mask = RDR2_SEQUENCE
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify sequence id in RDR_RB.rdr_tabid
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  Permit Tuples Associated with a Procedure:
**	  -----------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE;
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**          specify procedure ID in RDR_RB.rdr_tabid.
**	    TUPLES RETURNED:  RDR_INFO.rdr_ptuples  (cache memory)
**
**	  Procedure Parameters:
**	  ---------------------
**	    RDR_RB.rdr_2types_mask = RDR2_PROCEDURE_PARAMETERS
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**          specify procedure ID in RDR_RB.rdr_tabid.
**	    TUPLES RETURNED: RDR_INFO.rdr_pptuples
**
**	  List of Privileges on which a DBPROC or a View Depends:
**	  -------------------------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_PROCEDURE  or = RDR_VIEW
**	    rdr_2types_mask = RDR2_INDEP_PRIVILEGE_LIST
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify procedure or view id in RDR_RB.rdr_tabid	    
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**	    TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**	    
**	  Rule Tuples Associated with a Table:
**	  ------------------------------------
**	    rdr_types_mask = RDR_RULE
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid
**	    TUPLES RETURNED:  RDR_INFO.rdr_rtuples  (cache memory)
**
**	  Synonym:
**	  --------
**	    RDR_RB.rdr_2types_mask = RDR2_SYNONYM
**          specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**          specify synonym owner and name in rdf_rb->rdr_owner and 
**	    rdf_rb->rdr_name.rdr_synname
**	    specify number of tuples requested in RDF_RB.rdr_qtuple_count
**          TUPLES RETURNED:  RDF_RB.rdr_qrytuple   (user allocated memory)
**
**	  Alarm Tuples Associated with a Table:
**	  -------------------------------------
**	    RDR_RB.rdr_types_mask = RDR_SECALM
**	    specify RDR_RB.rdr_update_op (RDR_OPEN, RDR_GETNEXT or RDR_CLOSE)
**	    specify table id in RDR_RB.rdr_tabid	    
**	    TUPLES RETURNED:  RDR_INFO.rdr_atuples  (cache memory)
**
** Inputs:
**
**      rdf_cb                          
**		.rdf_rb
**			.rdr_db_id	Database id.
**			.rdr_session_id Session id.
**			.rdr_tabid	Table id.
**			.rdr_types_mask	Type of information requested.
**					The mask can be:
**					    RDR_EVENTS
**					    RDR_INTEGRITIES 
**					    RDR_PROCEDURE 
**					    RDR_PROTECT 
**					    RDR_RULE
**					    RDR_VIEW 
**					    RDR_SECALM
**
**			.rdr_2types_mask Type of Information Requested.
**					 The mask can be:
					    RDR2_SEQUENCE
**					    RDR2_INDEP_PRIVILEGE_LIST
**					    RDR2_SYNONYM
**					    RDR2_INDEP_OBJECT_LIST
**			.rdr_update_op	 Operation Instruction
**					    RDR_OPEN
**					    RDR_GETNEXT
**					    RDR_CLOSE
**			.rdr_qtuple_count  Number of integrity, protection or
**					   rule tuples to retrieve.
**			.rdr_rec_access_id The record access id for retrieving
**					   the next set of integrity, rule or
**					   protection tuples. This is required
**					   for RDR_GETNEXT and RDR_CLOSE.
**			.rdr_qrymod_id	Protection or integrity number that
**					that wants to be retrieved.
**			.rdr_qtext_id;	query text id for retrieving query 
**					text tuples of a view, protection,
**					integrity or rule definition.
**			.rdr_fcb	A pointer to the structure which 
**					contains the poolid and caching
**					control information. All the table
**					information will be allocated in the
**					memory pool specified by the poolid.
**		.rdf_info_blk		Pointer to the requested table 
**					information block. 
**					This must be the info block 
**					corresponding to the table id above.
**					This is for storing all the
**					information related to one table
**					together.
**
**
** Outputs:
**      rdf_cb                          
**		.rdf_rb
**			.rdr_rec_access_id The record access id for retrieving
**					   the next set of integrity, rule or
**					   protection tuples.
**			.rdr_qrytuple	Generic tuple pointer for simple
**					tuple retrievals.  This must be already
**					allocated by the caller for the specific
**					size.  For example, event tuples may
**					be retrieved into this field.  These
**					tuples need not be cached by RDF.
**		.rdf_info_blk		Pointer to the requested table 
**					information block. The pointer field is 
**					NULL if the corresponding information is
**					not requested.
**			.rdr_ituples	Pointer to integrity tuples.
**			.rdr_ptuples	Pointer to protection tuples.
**			.rdr_rtuples	Pointer to rule tuples.
**			.rdr_atuples    Pointer to alarm tuples.
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0001_NO_MORE_MEM
**					Out of memory.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of 
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of 
**					memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned
**					above.
**				E_RD000D_USER_ABORT
**					User abort.
**				E_RD0010_QEF_ERROR
**					Whatever error returned from QEF not
**					mentioned above.
**				E_RD0011_NO_MORE_ROWS
**					No more rows returned from QEU.
**					The system catalogs are internally
**					closed by RDF and rdd_qdata.qrym_cnt
**					contains the number of tuples actually
**					retrieved.
**				E_RD0013_NO_TUPLE_FOUND
**					No tuple found.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	28-jan-93 (teresa)
**	    Initial Creation by moving functionality out of RDF_GETINFO.
**	04-mar-93 (andre)
**          dbproc parameter tuples must be read into usr_infoblk->rdr_pptuples
**          and not usr_infoblk->rdr_rtuples
**	21-oct-93 (andre)
**	    added support for returning IISYNONYM tuples
**	29-nov-93 (robf)
**           Add support for security alarm tuples (RDR_SECALM)
**	14-jan-94 (teresa)
**	    fix bug 56728 (assure semaphores are NOT held across QEF/DMF calls
**	    during RDF_READTUPLES operation).
**	16-feb-94 (teresa)
**	    refresh checksum whether or not tracepoint to skip setting
**	    checksums is set.
**      16-mar-1998 (rigka01)
**	    Cross integrate change 434645 for bug 89105
**          Bug 89105 - clear up various access violations which occur when
**	    rdf memory is low.  In rdf_readtuples(), add
**	    DB_FAILURE_MACRO(status) call after each rdu_rqrytuple call.
**	6-may-02 (inkdo01)
**	    Change rdu_evprotect to _evsqprotect to read sequence protects, too.
**	24-feb-03 (inkdo01)
**	    Complete verification of sequence flags.
**	22-Mar-2005 (thaju02)
**	    Added support for retrieving iidbdepends tuples by key. (B114000)
*/

DB_STATUS
rdf_readtuples(	RDF_GLOBAL	*global,
		RDF_CB		*rdfcb)
{
    DB_STATUS		status;
    RDF_TYPES		requests, req2;
    RDR_INFO		*sys_infoblk = (RDR_INFO *) NULL;
    RDR_INFO		*usr_infoblk = rdfcb->rdf_info_blk;
    bool		get_evpriv, get_keys, get_i_objects, get_i_privileges;
    bool		get_syn, get_seqpriv, get_dbdepends;

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */
    global->rdfcb = rdfcb;
    CLRDBERR(&rdfcb->rdf_error);
    requests = rdfcb->rdf_rb.rdr_types_mask;
    req2 = rdfcb->rdf_rb.rdr_2types_mask;

    /*
    ** determine whether the caller is trying to obtain independent object or
    ** privilege list
    */
    get_i_objects = (req2 & RDR2_INDEP_OBJECT_LIST) != (RDF_TYPES) 0;
    get_i_privileges = (req2 &RDR2_INDEP_PRIVILEGE_LIST) != (RDF_TYPES) 0;
    get_keys = (req2 & RDR2_KEYS) != (RDF_TYPES) 0;
    get_evpriv = (requests == (RDR_PROTECT | RDR_EVENT));
    get_seqpriv = (requests & RDR_PROTECT) && (req2 & RDR2_SEQUENCE);
    get_syn = (req2 & RDR2_SYNONYM) != (RDF_TYPES) 0;
    get_dbdepends = (req2 & RDR2_DBDEPENDS) != (RDF_TYPES)0;

    if (   !get_evpriv && !get_i_objects && !get_i_privileges && !get_keys 
	&& !get_syn && !get_seqpriv && !get_dbdepends)
    {	
	global->rdf_ulhobject = (RDF_ULHOBJECT *)usr_infoblk->rdr_object;
	sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    }

#ifdef xDEBUG
    /* Check general input parameters. */
    if (rdfcb->rdf_rb.rdr_db_id == NULL ||
	rdfcb->rdf_rb.rdr_session_id == NULL ||
	(
	    (requests & (RDR_EVENT | RDR_INTEGRITIES | RDR_PROCEDURE |
			 RDR_PROTECT | RDR_RULE | RDR_VIEW | RDR_SECALM)
	    == 0)
	  &&
	    (req2 & (RDR2_INDEP_PRIVILEGE_LIST | RDR2_INDEP_OBJECT_LIST |
		     RDR2_PROCEDURE_PARAMETERS | RDR2_KEYS | RDR2_SYNONYM |
		     RDR2_SEQUENCE | RDR2_DBDEPENDS)
	    == 0)
	)
	||
	/*
	** if requessting independent object/privileges info, dependent object
	** id must be provided
	*/
	(
	    (get_evpriv || get_i_objects || get_i_privileges || get_keys ||
		get_seqpriv || get_dbdepends)
	 && !rdfcb->rdf_rb.rdr_tabid.db_tab_base
	)
	||
	(   
	    !get_evpriv && !get_i_objects && !get_i_privileges && !get_keys 
	 && !get_syn && !get_seqpriv
	 &&
	    (	
		!(usr_infoblk = rdfcb->rdf_info_blk)    /* get ptr to user info
							** block which may be the
							** same as the system info
							** block */
		||
		!(global->rdf_ulhobject = (RDF_ULHOBJECT *)usr_infoblk->rdr_object)
		||
		!(sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk)
		||
		!rdfcb->rdf_rb.rdr_tabid.db_tab_base
		||
    		!rdfcb->rdf_info_blk
		||
		!rdfcb->rdf_info_blk->rdr_object
	    )
	)
	|| 
	rdfcb->rdf_rb.rdr_fcb == NULL
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }		
#endif
	/*
	** set RDF_QTUPLE_ONLY bit in global->rdr_resources, since subordinate
	** routines check for this bit.
	*/
	global->rdf_resources |= RDF_QTUPLE_ONLY;

	if (requests & RDR_INTEGRITIES)
	{
	    status = rdu_rqrytuple(global, RDR_INTEGRITIES, 0L, RDF_RINTEGRITY,
		&Rdi_svcb->rdvstat.rds_r12_itups,
		&Rdi_svcb->rdvstat.rds_b12_itups,
		&Rdi_svcb->rdvstat.rds_c12_itups,
		&Rdi_svcb->rdvstat.rds_m12_itups,
		RD_IIINTEGRITY, &sys_infoblk->rdr_ituples,
		&((RDF_ULHOBJECT*)sys_infoblk->rdr_object)->rdf_sintegrity,
		&usr_infoblk->rdr_ituples,
		&((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->rdf_sintegrity,
		(i4)sizeof(DB_INTEGRITY));
	    if (DB_FAILURE_MACRO(status))
	       return (status);
	    usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_rqrytuple, so
					    ** reget value */
	    if ( 
	        (global->rdfcb->rdf_error.err_code  == E_RD0013_NO_TUPLE_FOUND) 
	        && 
		(status != E_DB_OK)
	       )
	    {
		/* integrity tuple was not found, report statistics */
		Rdi_svcb->rdvstat.rds_z12_itups++;
	    }

	}
	else if (requests & RDR_EVENT ||
	    req2 & RDR2_SEQUENCE)	/* Before checking for RDR_PROTECT */
	{
	    status = rdu_evsqprotect(global);
	}
 	else if (requests & RDR_PROTECT)
	{
	    status = rdu_rqrytuple(global, RDR_PROTECT, 0L, RDF_RPROTECTION, 
		&Rdi_svcb->rdvstat.rds_r14_ptups,
		&Rdi_svcb->rdvstat.rds_b14_ptups,
		&Rdi_svcb->rdvstat.rds_c14_ptups,
		&Rdi_svcb->rdvstat.rds_m14_ptups,
		RD_IIPROTECT, (RDD_QDATA **)&sys_infoblk->rdr_ptuples,
		&((RDF_ULHOBJECT*)sys_infoblk->rdr_object)->rdf_sprotection,
		(RDD_QDATA **)&usr_infoblk->rdr_ptuples,
		&((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->rdf_sprotection,
		(i4)sizeof(DB_PROTECTION) );
	    if (DB_FAILURE_MACRO(status))
	       return (status);
	    usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_rqrytuple, so
					    ** reget value */
	    if ( 
	        (global->rdfcb->rdf_error.err_code  == E_RD0013_NO_TUPLE_FOUND) 
	        && 
		(status != E_DB_OK)
	       )
	    {
		/* Protection tuple was not found, report statistics */
		Rdi_svcb->rdvstat.rds_z14_ptups++;
	    }
	}
 	else if (requests & RDR_RULE)
	{
	    status = rdu_rqrytuple(global, RDR_RULE, 0L, RDF_RRULE, 
		&Rdi_svcb->rdvstat.rds_r15_rtups,
		&Rdi_svcb->rdvstat.rds_b15_rtups,
		&Rdi_svcb->rdvstat.rds_c15_rtups,
		&Rdi_svcb->rdvstat.rds_m15_rtups,
		RD_IIRULE, (RDD_QDATA **)&sys_infoblk->rdr_rtuples,
		&((RDF_ULHOBJECT*)sys_infoblk->rdr_object)->rdf_srule,
		(RDD_QDATA **)&usr_infoblk->rdr_rtuples,
		&((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->rdf_srule,
		(i4)sizeof(DB_IIRULE) );
	    if (DB_FAILURE_MACRO(status))
	       return (status);
	    usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_rqrytuple, so
					    ** reget value */
	    if ( 
	        (global->rdfcb->rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND) 
	        && 
		(status != E_DB_OK)
	       )
	    {
		/* rule tuple was not found, report statistics */
		Rdi_svcb->rdvstat.rds_z15_rtups++;
	    }
	}
	else if (req2 & RDR2_PROCEDURE_PARAMETERS)
	{
	    status = rdu_rqrytuple(global, 0L, RDR2_PROCEDURE_PARAMETERS,
				    RDF_RPROCEDURE_PARAMETER,
				    &Rdi_svcb->rdvstat.rds_r18_pptups,
				    &Rdi_svcb->rdvstat.rds_b18_pptups,
				    &Rdi_svcb->rdvstat.rds_c18_pptups,
				    &Rdi_svcb->rdvstat.rds_m18_pptups,
				    RD_IIPROCEDURE_PARAMETER,
				    (RDD_QDATA **)&sys_infoblk->rdr_pptuples,
				    &((RDF_ULHOBJECT*)sys_infoblk->rdr_object)->
					rdf_sprocedure_parameter,
				    (RDD_QDATA **)&usr_infoblk->rdr_pptuples,
				    &((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->
					rdf_sprocedure_parameter,
				    (i4)sizeof(DB_PROCEDURE_PARAMETER) );
	    if (DB_FAILURE_MACRO(status))
	       return (status);

	    usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
						** call to rdu_rqrytuple, so
						** reget value */
	    if (
		(global->rdfcb->rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
		&&
		(status != E_DB_OK)
	       )
	    {
		/* rule tuple was not found, report statistics */
		Rdi_svcb->rdvstat.rds_z18_pptups++;
	    }
	}
 	else if (requests & RDR_SECALM)
	{
	    status = rdu_rqrytuple(global, RDR_SECALM, 0L, RDF_RSECALARM, 
		&Rdi_svcb->rdvstat.rds_r20_atups,
		&Rdi_svcb->rdvstat.rds_b20_atups,
		&Rdi_svcb->rdvstat.rds_c20_atups,
		&Rdi_svcb->rdvstat.rds_m20_atups,
		RD_IISECALARM, (RDD_QDATA **)&sys_infoblk->rdr_atuples,
		&((RDF_ULHOBJECT*)sys_infoblk->rdr_object)->rdf_ssecalarm,
		(RDD_QDATA **)&usr_infoblk->rdr_atuples,
		&((RDF_ULHOBJECT*)usr_infoblk->rdr_object)->rdf_ssecalarm,
		(i4)sizeof(DB_SECALARM) );
	    if (DB_FAILURE_MACRO(status))
	       return (status);
	    usr_infoblk = rdfcb->rdf_info_blk;  /* may have been reset by
					    ** call to rdu_rqrytuple, so
					    ** reget value */
	    if ( 
	        (global->rdfcb->rdf_error.err_code  == E_RD0013_NO_TUPLE_FOUND) 
	        && 
		(status != E_DB_OK)
	       )
	    {
		/* Alarms tuple was not found, report statistics */
		Rdi_svcb->rdvstat.rds_z20_atups++;
	    }
	}
	else if (get_i_privileges)
	{
	    /* obtain list of privileges on which a dbproc or a view depends */
	    status = rdu_usrbuf_read(global);
	}
	else if (get_i_objects)
	{
	    /* obtain list of objects on which a dbproc or a view depends */
	    status = rdu_i_objects(global);
	}
	else if (get_keys)
	{
	    /* obtain a list of key tuples */
	    status = rdu_usrbuf_read(global);
	}
	else if (get_syn || get_dbdepends)
	{
	    /* obtain a synonym tuple */
	    /* or obtain a dbdepend tuple for given independent object id */
	    status = rdu_usrbuf_read(global);
	}
	else
	{   /* this is a consistency check which should never be executed */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0071_NO_PROT_INT);
	}
	/* do any trace processing if the operation is one that reads tuples */
	if (
	    (rdfcb->rdf_rb.rdr_update_op == RDR_GETNEXT)
	   ||
	    (rdfcb->rdf_rb.rdr_update_op == RDR_OPEN)
	   )

		rdu_master_infodump(usr_infoblk, global, RDFGETINFO, requests);


	return(status);
}

/*{
** Name: rdu_cnstr_integ - Read iiintegrityidx tuples for a constraint
**
** Description:
**
**	This routine verifies that there is an integrity for a given
**	schema name/constraint name.  It does this as follows:
**	1.  Translate schema name to schema id by doing a keyed lookup in
**	    iischema using schemaname as the key.
**	2.  Use schema id/constraint name to do a keyed lookup in catalog
**	    iiintegrityidx.
**	3.  Return the iiintegrityidx tuple to the caller in 
**	    RDF_CB.rdr_qrytuple 
**
**	This routine will use rdu_qopen (to open iischema and iiintegrityidx),
**	rdu_qget (to extract iischema and iiintegrityidx tuples by key), and
**	rdu_qclose to close those catalogs.  This routine should never exit
**	with a catalog open, it should always call rdu_qclose on its way out.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			RDF Control Block
**	    .rdfrb			User RDF request block
**	      .rdr_2types_mask		RDR2_CNS_INTEG
**	      .rdr_name.rdr_cnstrname	constraint name.
**	      .rdr_owner		schema name.
**
** Outputs:
**      global
**	   .rdfcb
**	    .rdfrb			User RDF request block
**	     .rdr_qrytuple		Must be an allocated IIINTEGRITYIDX
**					tuple into which the result tuple
**					will be deposited.
**					If the tuple does not exist then the
**					result is ignored by caller.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      04-mar-93 (teresa)
**	    Initial creation for constraints
*/
static DB_STATUS
rdu_cnstr_integ(RDF_GLOBAL    *global)
{


    DB_STATUS		status, close_status;
    bool		eof_found;	/* To pass to rdu_get */
    RDR_RB		*rdf_rb;
    DB_IISCHEMA		schema_tuple;
    QEF_DATA		qefdata;	/* For QEU extraction call */
    RDD_QDATA		rdqef;		/* For rdu_qopen to use */

    rdf_rb = &global->rdfcb->rdf_rb;
    /* Validate parameters to request */
    if (  (~rdf_rb->rdr_2types_mask & RDR2_CNS_INTEG)
	||
	  (rdf_rb->rdr_qrytuple == NULL)
	||
	  (rdf_rb->rdr_name.rdr_cnstrname.db_constraint_name[0] == '\0')
	||
	  (rdf_rb->rdr_owner.db_own_name[0] == '\0')
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }
	
    /* control loop */
    do 
    {
	/* get schema id from schema name.  Start by setting up the
	** structures to call QEF for an iischema tuple 
	*/
	qefdata.dt_next = NULL;
	qefdata.dt_size = sizeof(DB_IISCHEMA);
	qefdata.dt_data = (PTR) &schema_tuple;
	rdqef.qrym_data = &qefdata;
	rdqef.qrym_alloc= 1;
	rdqef.qrym_cnt  = 1;
	rdf_rb->rdr_rec_access_id = NULL;
	rdf_rb->rdr_qtuple_count = 0;

	/* now go open the iischema table and look for a schema */
	status = rdu_qopen(global, RD_IISCHEMA, (char *) 0,
			   sizeof(DB_IISCHEMA), (PTR)&rdqef, 
			   0 /* 0 datasize means RDD_QDATA is set up */ );
	if (DB_FAILURE_MACRO(status))
	    break;
	/* Fetch the tuple */
	status = rdu_qget(global, &eof_found);
	if ( !DB_FAILURE_MACRO(status))
	{
	    if ( (global->rdf_qeucb.qeu_count == 0) || eof_found)
	    {
		/* set status to warning to keep this from being logged, set 
		** the error, then set status to failure for caller
		*/
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count != 1)
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
		status = E_DB_ERROR;
	    }
	}
	close_status = rdu_qclose(global); 	/* Close iischema catalog */
	if (close_status > status)
	    status = close_status;
	if (status != E_DB_OK)
	    break;
	
	/* if we get here, schema id is now in schema_tuple.db_schid.  Pass
	** this to rdu_qopen so it can use it to build the key into 
	** iiintegrityidx
	*/
	qefdata.dt_next = NULL;
	qefdata.dt_size = sizeof(DB_INTEGRITYIDX);
	qefdata.dt_data = rdf_rb->rdr_qrytuple;  /* use caller supplied 
						** memory for tuple */
	rdqef.qrym_data = &qefdata;
	rdqef.qrym_alloc= 1;
	rdqef.qrym_cnt  = 1;
	rdf_rb->rdr_rec_access_id = NULL;
	rdf_rb->rdr_qtuple_count = 0;

	/* now go open the iiintegrityidx catalog and get tuple */
	status = rdu_qopen(global, RD_IIINTEGRITYIDX, 
			   (char *) &schema_tuple.db_schid,
			   sizeof(DB_INTEGRITYIDX), (PTR)&rdqef, 
			   0 /* 0 datasize means RDD_QDATA is set up */ );
	if (DB_FAILURE_MACRO(status))
	    break;
	/* Fetch the tuple */
	status = rdu_qget(global, &eof_found);
	if ( !DB_FAILURE_MACRO(status))
	{
	    if ( (global->rdf_qeucb.qeu_count == 0) || eof_found)
	    {
		/* set status to warning to keep this from being logged, set 
		** the error, then set status to failure for caller
		*/
		status = E_DB_WARN;
		if (global->rdf_qeucb.qeu_count != 1)
		    rdu_ierror(global, status, E_RD0013_NO_TUPLE_FOUND);
		status = E_DB_ERROR;
	    }
	}
	close_status = rdu_qclose(global); 	/* Close iischema catalog */
	if (close_status > status)
	    status = close_status;

    } while (0);   /* end control loop */
    
    return(status);
}
