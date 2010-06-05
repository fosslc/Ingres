/*
** Copyright (c) 1989, 2008 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <cv.h>
#include    <tm.h>
#include    <lo.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <cm.h>
#include    <er.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmacb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <qefscb.h>
#include    <usererror.h>
#include    <qeaiproc.h>

#include    <ex.h>
#include    <tm.h>

#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>

/**
**
**  Name: QEAIPROC.C - Implement Internal Procedures
**
**  Description:
**
**	There are operations that a DUF type of FE utility program must
**	perform that do NOT file into the SQL/QUEL model very well:
**      creating of database directory and core catalogs, destroying a 
**	database, listing files in a database directory, deleteing files from 
**	a database directory,  or listing directories subordinate to an INGRES 
**	location that is not specifically tied to the database, obtaining 
**	information from the database config file, updating the database 
**	config file).
**
**	The purpose of QEF Internal functions function is to permit the user
**	to request the server perform those type of operations.  IT IS NOT
**	reasonable to write a QEF Internal Procedure for  something that
**	does fit well into the SQL model.  (If you are in doubt about wether
**	or not something fits into the SQL check with the DBMS System 
**	architect.)
**
**	Internal Procedure Actions should NOT be recursive.  If recursion is 
**	required, then you may need to implement it some other way.  The beauty
**	of Internal Procedures is that new "commands" can be added to the
**	server with minimum impact on the server.  Typically that will involve
**	adding the new procedure to the qea_iproc lookup table qef_procs[], 
**	adding their expected parameters to lookup table exp_proces[], writing 
**	a function to inferface between QEF and the DMF and provide new DMF
**	logic.  No changes should be required to PSF, OPF, OPC or the SQL
**	preprocessor.  To define a new QEF Internal Procedure:
**	    1.  Define a case_id constant in subroutine qea_iproc.
**	    2.  Add an entry to lookup table qea_procs containing:
**		a.  name of new procedure
**		b.  owner of new procedure
**		c.  case_id constant for new procedure.
**	    3.  Add an entry to lookup table exp_parm containing:
**		a.  number of expected parameters
**		b.  exp parameter info record for each parameter.  Each record
**		    contains:
**		    1.  Name of parameter
**		    2.  number of characters in parameter name
**		    3.  data type of parameter
**		    4.  spare (always 0)
**	    4.  Add the new procedure (identified by case_id) to qea_iproc's
**	        case statement.  That case can do any special processing, and/or
**		should call a subroutine to process the Internal Procedure.
**	    5.  Write new subroutine (if called from case statement) to 
**		implement QEF Internal Procedure.  This will usually involve
**		building a DMF control block and calling DMF.  [There is a
**		routine (QEA_MK_DMMCB) that makes a DMM_CB.]
**	    6.  Implement any DMF changes required for this procedure.
**
**	Internal Procedure Actions must set up their own DMF control blocks,
**	that will not be provided in the action header.  However, parameter
**	validation and transaction management are provided by qeq_query and
**	parent routines.
**
**	Internal Procedures are expected in the following format:
**
**	    CREATE PROCEDURE proc_name INTERNAL
**		[(param_nam [=] param_type {,(param_nam [=] param_type })]
**	    AS | =
**	      BEGIN
**		    return;
**	      END;
**
**	And the following rules apply:
**	1.  FE user must have System Catalog Update Privilege to create 
**	    a QEF Internal Procedure.
**	2.  if proc_name starts with "ii", PSF will force the owner to $ingres.
**	    All Internal QEF Procedures should start with "ii" and be owned
**	    by $ingres.
**
**
**  History:
**      14-apr-1989  (teg)
**	    Initial Creation.
**	11-Aug-1989 (teg)
**	    add check and patch table support.
**	08-feb-1991 (ralph)
**	    Fold database name, location name to lower case
**	23-oct-1992 (bryanp)
**	    Improve error handling following failure in dmf_call().
**	30-October-1992 (rmuth)
**	    Add file extend conversion support.
**      8-nov-92 (ed)
**          blank fill from 24 to 32 for DB_MAXNAME project
**	03-nov-92 (jrb)
**	    Created new iiQEF_alter_extension internal procedure for
**	    multi-location sorts project.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	08-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with qeu_secaudit
**	14-dec-92 (jrb)
**	    Removed "need_dbdir_flag" from the create_db and extend internal
**	    procs since convto60 won't be used for 6.5 and therefore won't
**	    need to use this code.
**	08-feb-1993 (rmuth)
**	    Add file extend checking to the check table option.
**	12-feb-93 (jhahn)
**          Added error function for constraints.
**	10-mar-93 (rickh)
**	    Added internal procedure to fabricate iidefault tuples for UDTs.
**	    This is for converting 6.4 databases to 6.5.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	8-apr-93 (rickh)
**	    Improved the UDT convertor to call a delimited id routine.
**	26-apr-1993 (rmuth)
**	    RE-integrate previous change which got lost in the last 
**	    integration.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Use STbcompare instead of MEcmp where appropriate to
**	    treat reserved procedure names and parameters case insensitively.
**	30-jun-1993(shailaja)
**	    Fixed compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-jun-93 (rickh)
**	    Added CV.H.
**	9-jul-93 (rickh)
**	    Made some ME calls conform to their prototypes.
**	19-oct-1993 (rachael) Bug 55767
**	    Upgradedb calls internal procedures qea_9_readconfig,
**	    qea_10_updconfig, and qea_11_deldmp_config to manipulate the
**	    configuration file. These procedures worked on VMS, but failed
**	    on UNIX because the location name II_DATABASE was converted to
**	    lowercase.  The attempted fix was to pass in "default" which then caused upgradedb to succeed
**	    on UNIX, but now fail on VMS because "default" was not found by
**	    the LO routine.
**	    The fix is to not convert the location name to lower case.
**	09-nov-93 (rblumer)
**	    Change iiqef_createdb to have two new parameters:
**	    translated_owner_name and untranslated_owner_name;
**	    changed qea_2create_db function to handle the new parameters.
**	28-mar-1994 (bryanp) B60598
**	    Remove extra computation of database.du_dbid from createdb code.
**	10-Nov-1994 (ramra01) b60913
**		Extra check to execute destroydb if not in use only
**      02-oct-1994 (Bin Li) B60932
**          For command like 'Destroydb <dbname>', if dbname is invalid, call
**          the psf_error and pass the corresponding dbname.
**      21-nov-1994 (juliet)
**          Modified qea_5add_location to ensure that the database
**          is not being extended to the same location for the same usage
**          again.
**	23-jan-1995 (cohmi01)
**	    Special handling in qea_5add_location() if called by mkrawloc 
**	    utility, as indicated by resurrected 'need_dir' flag. 
**      31-jan-1995 (stial01)
**          BUG 66510: qea_3destroy_db(): prepare list of locations for 
**          dmm_destroy() in case the database directory has been deleted and   
**          other locations cannot be destroyed by processing the config file.
**          Added get_extent_info(), get_loc_tuple().
**	17-feb-95 (forky01)
**	    Unitialized flag for maxlocks and lock timeout being passed to
**	    all qeu_open calls. Initialize qeu_mask = 0 for qeu_open calls.
**	    This was causing all of the internal procedures to take intended
**	    locks out as TABLE-LEVEL locks.  This caused random results for
**	    the locking system, and eventually leads to deadlock on concurrent
**	    sessions.  Fixes bug 66865 and maybe many others.
**      21-mar-1995 (harpa06)
**          Bug #67531: Destroydb would fail because of information in QQEU 
**	    stucture being accidently overwritten. Information being placed in 
**	    wrong array elements.
**	21-Dec-95 (nive)
**	    Bug 71480: In funciton get_extent_info, call to get_loc_tuple
**	    does not return location info for location name passed.
**	    Location names read from iiextend table whose length
**	    < 32 (happens for database upgraded from R6) have to be padded 
**	    with blanks upto 32 chars length. Also added error checking 
**	    for get_loc_tuple return value
**	12-Jan-96 (nive)
**	    Bug 71480 - Previous change lacked a variable definition in
**	    function get_extent_info.
**	16-sep-96 (nick)
**	    Tidy.
**	19-Aug-1997  (jenjo02)
**	    Piggyback ulm_palloc() with ulm_openstream() calls where appropriate.
**	    Define ULM stream types as private or shared.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**	07-mar-2000 (gupsh01)
**	    Added error checking for DMM_CREATE_DB call to check for readonly 
**	    database creation error, E_DM0192_CREATEDB_RONLY_NOTEXIST.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	19-mar-2001 (stephenb)
**	    Add ucollation parm to iiqef_create_db.
**	23-May-2001 (jenjo02)
**	    Added new procedure, iiQEF_check_loc_area, for ACCESSDB,
**	    EXTENDDB.
**	15-Oct-2001 (jenjo02)
**	    Area directories now made during CREATE/ALTER LOCATION.
**	    Deleted short-lived iiQEF_check_loc_area procedure.
**      08-may-2002 (stial01)
**          qea_2create_db() Init dbservice (DU_LP64) here, instead of frontend
**	02-feb-2004 (rigka01) bug 111600 INGSRV2666
**	    location paths may be longer than DB_MAXNAME so increase tempstr 
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	29-Apr-2004 (gorvi01)
**		Added qea_5del_location. This INTERNAL Procedure is used to unextend 
**		a database from location. 
**	29-Apr-2004 (gorvi01)
**		Added entries in qef_procs for iiqef_del_location. 
**		Added entries in qef_procs for del_location parameters.
**	9-Dec-2005 (kschendel)
**	    Get rid of some of the bogus i2 casts on mecopy.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	13-Feb-2006 (kschendel)
**	    Fix several annoying int == NULL warnings.
**	11-Apr-2008 (kschendel)
**	    Update QEU / DMF qualification call sequence.
**      06-jun-2008 (stial01)
**          Changed DMM_UPDATE_CONFIG to use new dmm_upd_map (b120475)
**      04-nov-2008 (stial01)
**          In qef_procs, define procedure name and owner without blanks.
**          In exp_parm define parameters without blanks
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Defines of other constants.
*/
#define			PATHLEN	    132
/* TEMPLEN must be big enough to contain DB_MAXNAME and MAX_LOC */ 
#define		 	TEMPLEN     MAX_LOC+1 

typedef	struct	_DM_COLUMN	QEA_ATTRIBUTE;

#define	QEF_ERROR( errorCode )	\
            error = errorCode;	\
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );

#define	BEGIN_CODE_BLOCK	for ( ; ; ) {
#define	EXIT_CODE_BLOCK		break;
#define	END_CODE_BLOCK		break; }

/*
[@forward_type_references@]
[@forward_function_references@]
[@group_of_defined_constants@]...
[@type_definitions@]
[@global_variable_definitions@]
*/

static void qea_mk_dmmcb(
    QEE_DSH		*dsh,
    DMM_CB		*dmm_cb);


static void qea_make_dmucb(
    QEE_DSH		*dsh,
    DMU_CB		*dmu_cb,
    DMU_KEY_ENTRY	**key,
    DMU_CHAR_ENTRY	*char_entry );


static DB_STATUS
qea_14_error( 
	QEF_DBP_PARAM           *db_parm,
	QEE_DSH                 *dsh);

static DB_STATUS
qea_15_make_udt_defaults(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

static DB_STATUS
UDTsieve(
void		*dummy,
QEU_QUAL_PARAMS	*qparams
);

static DB_STATUS
constructUDTdefaultTuple(
    QEE_DSH		*dsh,
    QEA_ATTRIBUTE	*attribute,
    DB_IIDEFAULT	*IIDEFAULTtuple,
    PTR			emptyUDTvalue,
    DB_TEXT_STRING	*unquotedDefaultValue
);

static DB_STATUS
get_loc_tuple(
QEE_DSH			*dsh,
DB_LOC_NAME             *locname,
DU_LOCATIONS            *loctup
);

static DB_STATUS
get_extent_info(
QEE_DSH			*dsh,
i4                 phase,
#define GET_EXTENT_COUNT      1
#define GET_EXTENT_AREAS      2
DB_DB_NAME              *dbname,
DMM_LOC_LIST            **loc_list,
i4                 *ext_countp
);


/*{
** Name: QEA_IPROC	- Perform QEF Internal Procedure Action
**
** Description:
**
**  Input parameters include a pointer to the QEF_AHD and a pointer to the
**  QEF_RCB.  By the time qea_iproc is called, the following relationship
**  exists between the QEF control blocks:
**
**	QEF_DSH:		    QEF_AHD(ahd_iproc)
**	:---------------------:	    :-----------------------:
**	: (dsh_param is null) :	    : ahd_procnam	    :
**	: dsh_act_ptr	      :---->: ahd_ownname	    :
**   +->: dsh_qp_ptr	      :--+  : ahd_type=QEA_IPROC    :
**   |	:---------------------:  |  :-----------------------:
**   |				 |
**   |		  +--------------+
**   |		  |
**   |		  V
**   |	QEF_QP_CB (Query Plan)			QEF_DBP[]
**   |	:-------------------------------:	:-----------------------:
**   |	:qp_dbp_params (ptr to array)	:------>: dbp_name (param name) :
**   |	:qp_ndbp_params (num of params ):       : dbp_rownno	        :
**   |	:qp_qmode  (QUERYMODE)	        :	: dbp_offset		:
**   |	:-------------------------------:	: dbp_dbv (ptr to data) :--+
**   |						:-----------------------:  |
**   |									   |
**   |						               +-----------+
**   |   QEF_CB						       |
**   |	:-----------------------:		DB_DATA_VALUE  V
**   +--: qef_dsh		:		:-------------------:
**	: qef_rcb		:		: db_data (PTR)	    :
**	: qef_qp (= Garbage)	:--+		: db_length	    :
**	: qef_dmt_id		:  |		: db_datatype	    :
**	:			:  |		: db_prec     	    :
**   +->:-----------------------:  |		:-------------------:
**   |				   |
**   |				   |
**   |		    QEF_RCB	   V
**   |		    :-----------------------:
**   +--------------: qef_cb		    :
**		    : qef_param (is garbage):
**		    :-----------------------:
**  
**	The QEF_AHD is important because it contains the (name,owner) pair
**	used to identify the QEF Internal Procedure.  Internal Lookup Table
**	QEF_PROCS contains the (name,owner) of all QEF Internal Procedures.
**	qea_iproc tries to find the AHD's procedure (name,owner) in this
**	lookup table.  If it cannot find a match, then the specified procedure
**	is considered NONEXISTENT, and an error is generated.  If a match is
**	found, the procedures case_id/offset is read from the lookup table.
**	That value serves as an index into table EXP_PARM[] and as a value
**	for the QEA_IPROC case statement.  IE, that value is how QEA_IPROC
**	determines what parameters are expected and what subroutine to call.
**
**	The Query Plan is important because it contains a pointer to an array
**	of QEF_DBPs.  Each QEF_DBP member descibes a single parameter.  There
**	will be qp_ndbp_params number of parameters for this procedure.
**	(We get the QP address from the QEF_CB, and we get the QEF_CB's address
**	from the QEF_RCB).
**
**	QEA_IPROC does not care about the user supplied parameters in the
**	QEF_RCB because they have already been substituted into the DSH by
**	qeq_query(). (The QP indicates the row and column offset into the DSH 
**	for each parameter.)  QEA_IPROC validates the parameters in the DSH 
**	against a lookup table of expected parameters (EXP_PARM[]).  It checks
**	the parameter name and the data type of that parameter.  If the number
**	of expected parameters does not match the number of actual parameters,
**	an error is generated.  If each of the parameter names does not match
**	the expected parameter name, an error is generated.  If the data type
**	of the parameter is not what QEA_IPROC expects, an error is genreated.
**
**	Once QEA_IPROC has validated the INTERNAL PROCEDURE and its parameters,
**	QEA_IPROC enters a CASE STATEMENT, using the case_id found in the
**	qef_procs lookup table.  As a minimum, the case statement section for
**	each procedure will call a specific QEF routine to implement that
**	Internal Procedure.  QEA_IPROC will not perform any error management 
**	on behalf of the Internal Procedure Implementation subroutine.  It will
**	merely pass the QEF_RCB.error and return status to qeq_query.
**
**	(qeq_query will do the appropriate transaction/statement management 
**	as determined by the error.err_code and status.)
**
** Inputs:
**    QEA_ACT				Action control block
**	.ahd_procnam			    name of stored proceduree
**	.ahd_ownname			    Owner of stored procedure
**	.ahd_type			    Action Type (QEA_IPROC)
**    QEF_RCB				QEF Reqeust Block
**	.qef_type			    Must be QEFRCB_CB
**	.qef_length			    Must be sizeof(QEF_RCB)
**	.qef_stat			    Transaction status
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**	    .qef_dsh				Data Segment Header ptr
**		.dsh_qp_ptr			    Query Plan Pointer
**		    .qp_dbp_params			Procedure parameters
**		    .qp_ndbp_params			Number of parameters
**	.qef_usr_param			    User Input Parameters
**	    
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    See each specifically called subroutine for side effects of that
**	    INTERNAL procedure.
**
** History:
**      14-apr-1989  (teg)
**	    Initial Creation.
**	11-Aug-1989 (teg)
**	    add check and patch table support.
**	02-apr-91 (teresa)
**	    add need_dbdir_flg support to iiqef_createdb
**	25-apr-91 (teresa)
**	    add need_dbdir_flg support to iiqef_add_location
**	02-oct-92 (teresa)
**	    allow optional parameters when verifying # parameters is correct 
**	    AND
**	    add support for new procedures:  ii_read_config, ii_update_config,
**					     and ii_del_dmp_config
**	30-October-1992 (rmuth)
**	    Add file extend conversion support.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Use STbcompare instead of MEcmp where appropriate to
**	    treat reserved procedure names and parameters case insensitively.
**	21-may-93 (andre)
**	    rather than using case-insensitive comparison on dbproc name and
**	    case-sensitive on dbproc owner names, we will make sure that the
**	    names from the internal table is cast into an appropriate case for
**	    regular identifiers before performing the comparison
**
[@history_template@]...
*/

DB_STATUS
qea_iproc(
QEF_AHD		*qea_act,
QEF_RCB		*qef_rcb )
{
/*--------------------------------------------------------------*/
/*  constants for lookup table of QEF Internal Procedures	*/
/*--------------------------------------------------------------*/
#define		    LIST_FILE	0	/* QEF procedure is iiQEF_list_file */
#define		    DROP_FILE	1	/* QEF procedure is iiQEF_drop_file */
#define		    CREATE_DB	2	/* QEF procedure is iiQEF_create_db */
#define		    DESTROY_DB	3	/* QEF procedure is iiQEF_destroy_db */
#define		    ALTER_DB	4	/* QEF procedure is iiQEF_alter_db */
#define		    ADD_LOCATION 5	/* QEF procedure is iiQEF_add_location*/
#define             CHECK_TBL   6       /* QEF procedure is iiQEF_check_table*/
#define             PATCH_TBL   7       /* QEF procedure is iiQEF_patch_table*/
#define		    FIND_DBS	8	/* QEF procedure is iiQEF_finddbs */
#define		    READ_CONFIG 9	/* QEF procedure is ii_read_config */
#define		    UPDATE_CONFIG 10	/* QEF procedure is ii_update_config */
#define		    DEL_DMP_CONFIG 11	/* QEF procedure is ii_del_dmp_config */
#define		    ALTER_EXT	12	/* QEF name is iiQEF_alter_extension */
#define             CONVERT_TABLE  13 /* QEF procedure is iiQEF_convert_table */
#define		    ERRORP      14      /* QEF procedure is ii_error */
#define             MAKE_UDT_DEFAULTS 15/* QEF procedure is iiQEF_make_udt_defaults */
#define		    DEL_LOCATION 16	/* QEF procedure is iiQEF_del_location*/
#define		    MAX_EXP_PARAM  13	/* count of qef procedure codes */

#define		    IDENTICAL	0	/*value MEcmp() returns if match found*/
#define		    PROC_CNT	( sizeof(qef_procs) / sizeof(qef_procs[0]) )
#define		    NONEXISTENT_PROC	-1

/*--------------------------------------------------------------*/
/*  Lookup table used to recoginze QEF Internal Procedures and  */
/*  to define who (which application ids) may use them		*/
/*--------------------------------------------------------------*/

    /* qef_procs has procedure name, procedure owner, INDEX value.
    ** Index value is used to index exp_param lookup table and as case
    ** in switch statement.
    */
static const struct 
  {
	char		*p_nam;
	char		*p_own;
	u_i4		case_id;
  } qef_procs[] =
    {
	{ "iiqef_list_file", "$ingres", LIST_FILE },
	{ "ii_list_file", "$ingres", LIST_FILE },   
	{ "iiqef_drop_file", "$ingres", DROP_FILE },   
	{ "ii_drop_file", "$ingres", DROP_FILE },
	{ "iiqef_create_db", "$ingres", CREATE_DB },
	{ "ii_create_db", "$ingres", CREATE_DB },
	{ "iiqef_destroy_db", "$ingres", DESTROY_DB },
	{ "ii_destroy_db", "$ingres", DESTROY_DB },
	{ "iiqef_alter_db", "$ingres", ALTER_DB },
	{ "ii_alter_db", "$ingres", ALTER_DB },
	{ "iiqef_add_location", "$ingres", ADD_LOCATION },
	{ "ii_add_location", "$ingres", ADD_LOCATION },
	{ "iiqef_check_table", "$ingres", CHECK_TBL },   
	{ "ii_check_table", "$ingres", CHECK_TBL },
	{ "iiqef_patch_table", "$ingres", PATCH_TBL },
	{ "ii_patch_table", "$ingres", PATCH_TBL },
	{ "iiqef_finddbs", "$ingres", FIND_DBS },
	{ "ii_finddbs", "$ingres", FIND_DBS },
	{ "ii_read_config_value", "$ingres", READ_CONFIG },
	{ "ii_update_config",  "$ingres", UPDATE_CONFIG },
	{ "ii_del_dmp_config", "$ingres", DEL_DMP_CONFIG },
	{ "iiqef_alter_extension", "$ingres", ALTER_EXT },
	{ "iiqef_convert_table", "$ingres", CONVERT_TABLE },
	{ "iierror", "$ingres", ERRORP },
	{ "iiqef_make_udt_defaults", "$ingres", MAKE_UDT_DEFAULTS },
	{ "iiqef_del_location", "$ingres", DEL_LOCATION },
	{ "ii_del_location", "$ingres", DEL_LOCATION }
    };

   /* exp_parm has num parameters and entry for each expected parameter.  Each
   ** parameter entry has name of parameter, num chars in parameter name,
   ** parameter type (DB_CHA_TYPE, DB_INT_TYPE) and a spare (always NULL).
   */
static const struct 
  {
	i4		exp_num;	    /* number of parameters expected */
	struct {			    /* array of parameters expected */
		    char	*p_arg;
		    DB_DT_ID    datatype;
		} p[MAX_EXP_PARAM];
  }  exp_parm[] =  /* name of expected parameters, in expected order */
	{   /*NUM     PARM NAME	      NUM   PARAM
	    **PARM		      CHAR  TYPE */
	    /* LIST_FILE */
	    {	4,
		{
		    { "directory", DB_CHA_TYPE },
		    { "tab", DB_CHA_TYPE },
		    { "owner", DB_CHA_TYPE },
		    { "col", DB_CHA_TYPE }
		}
	    },
	    /* DROP_FILE */
	    {	2,
		{
		    { "directory", DB_CHA_TYPE },
		    { "file", DB_CHA_TYPE },
		}
	    },
	    /* CREATE_DB */
	    {	13,
		{
		    { "dbname", DB_CHA_TYPE },
		    { "db_loc_name", DB_CHA_TYPE },
		    { "jnl_loc_name", DB_CHA_TYPE },
		    { "ckp_loc_name", DB_CHA_TYPE },
		    { "dmp_loc_name", DB_CHA_TYPE },
		    { "srt_loc_name", DB_CHA_TYPE },
		    { "db_access", DB_INT_TYPE },
		    { "collation", DB_CHA_TYPE },
		    { "need_dbdir_flg", DB_INT_TYPE },
		    { "db_service", DB_INT_TYPE },
		    { "translated_owner_name", DB_CHA_TYPE },
		    { "untranslated_owner_name", DB_CHA_TYPE },
		    { "ucollation", DB_CHA_TYPE }
		}
	    },
	    /* DESTROY_DB */
	    {	1,
		{ "dbname", DB_CHA_TYPE }
	    },
	    /* ALTER_DB */
	    {	6,
		{
		    { "dbname", DB_CHA_TYPE },
		    { "access_on", DB_INT_TYPE },
		    { "access_off", DB_INT_TYPE },
		    { "service_on", DB_INT_TYPE },
		    { "service_off", DB_INT_TYPE },
		    { "last_table_id", DB_INT_TYPE }
		}
	    },
	    /* ADD_LOCATION */
	    {	4,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_name", DB_CHA_TYPE },
		    { "access", DB_INT_TYPE },
		    { "need_dbdir_flg", DB_INT_TYPE }
		}
	    },
	    /* CHECK_TBL */
	    {	3,
		{
		    { "table_id", DB_INT_TYPE },
		    { "index_id", DB_INT_TYPE },
		    { "verbose", DB_CHA_TYPE }
		}
	    },
	    /* PATCH_TBL */
	    {	3,
		{
		    { "table_id", DB_INT_TYPE },
		    { "mode", DB_CHA_TYPE },
		    { "verbose", DB_CHA_TYPE }
		}
	    },
	    /* FIND_DBS */
	    {	5,
		{
		    { "loc_name", DB_CHA_TYPE },
		    { "loc_area", DB_CHA_TYPE },
		    { "codemap_exists", DB_INT_TYPE },
		    { "verbose_flag", DB_INT_TYPE },
		    { "priv_50dbs", DB_INT_TYPE }
		}
	    },
	    /* READ_CONFIG*/
	    {	3,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_area", DB_CHA_TYPE },
		    { "readtype", DB_INT_TYPE }
		}
	    },
	    /* UPDATE_CONFIG*/
	    {	3,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_area", DB_CHA_TYPE },
		    { "update_map", DB_INT_TYPE }
		}
	    },
	    /* DEL_DMP_CONFIG */
	    {	2,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_area", DB_CHA_TYPE }
		}
	    },
	    /* ALTER_EXT */
	    {	4,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_name", DB_CHA_TYPE },
		    { "drop_loc_type", DB_INT_TYPE },
		    { "add_loc_type", DB_INT_TYPE }
		}
	    },
	    /* CONVERT_TABLE */
	    {   2,
		{
		    { "table_id", DB_INT_TYPE },
		    { "index_id", DB_INT_TYPE }
		}
	    },
	    /* ERRORP */
            {   8,
                {
                    { "errorno", DB_INT_TYPE },
                    { "detail", DB_INT_TYPE },
                    { "p_count", DB_INT_TYPE },
                    { "p1", DB_VCH_TYPE },
                    { "p2", DB_VCH_TYPE },
		    { "p3", DB_VCH_TYPE },
		    { "p4", DB_VCH_TYPE },
		    { "p5", DB_VCH_TYPE }
                }
            },
	    /* MAKE_UDT_DEFAULTS */
	    {   0,
		{ /* no parameters */

		    { "", 0 }
		}
	    },
	    /* DEL_LOCATION */
	    {	4,
		{
		    { "database_name", DB_CHA_TYPE },
		    { "location_name", DB_CHA_TYPE },
		    { "access", DB_INT_TYPE },
		    { "need_dbdir_flg", DB_INT_TYPE }
		}
	    },
	};		    /* end of exp_parm[] array */

/*--------------------------------------------------------------*/
/*  Local Variables						*/
/*--------------------------------------------------------------*/
    i2			i, proc_case;
    DB_STATUS		retstat;
    QEF_CB		*qef_cb=qef_rcb->qef_cb;
    QEE_DSH		*dsh=(QEE_DSH *)qef_cb->qef_dsh;
    QEF_QP_CB		*qp=dsh->dsh_qp_ptr;
    QEF_DBP_PARAM	*params;
    DMM_CB		dmm_cb;
    DB_DBP_NAME		upcase_dbpname, *dbpname_p;
    DB_OWN_NAME		upcase_ownname, *ownname_p;
    DB_ATT_NAME		upcase_attname, *attname_p;
    bool		upcase = ((*qef_cb->qef_dbxlate & CUI_ID_REG_U) != 0);
    DB_DBP_NAME		dbp_nam;
    DB_OWN_NAME		dbp_own;
    DB_ATT_NAME         dbp_parm;

	if (upcase)
	{
	    /*
	    ** if dbp name/owner/parameter need to be uppercased, make
	    ** dbpname_p, ownname_p, and attname_p point at upcase_dbpname,
	    ** upcase_ownname, and upcase_attname, respectively, to save the
	    ** expense of doing it for every row of the table
	    */
	    dbpname_p = &upcase_dbpname;
	    ownname_p = &upcase_ownname;
	    attname_p = &upcase_attname;
	}
	
	/*
	** first attempt to recognize the (procedure,owner) pair as
    	** an internal procedure
	*/
	for (proc_case=NONEXISTENT_PROC, i=0; i<PROC_CNT; i++)
	{
	    STmove(qef_procs[i].p_nam, ' ', 
			sizeof(dbp_nam), (char *)&dbp_nam);
	    STmove(qef_procs[i].p_own, ' ', 
			sizeof(dbp_own), (char *)&dbp_own);
	    if (upcase)
	    {
		/*
		** uppercase dbp name and owner from the internal table
		*/
		char	    *from, *last, *to;

		for (from = (char *) &dbp_nam,
		     last = from + sizeof(DB_DBP_NAME),
		     to = (char *) dbpname_p;

		     from < last;

		     CMnext(from), CMnext(to)
		    )
		{
		    CMtoupper(from, to);
		}

		for (from = (char *) &dbp_own,
		     last = from + sizeof(DB_OWN_NAME),
		     to = (char *) ownname_p;

		     from < last;

		     CMnext(from), CMnext(to)
		    )
		{
		    CMtoupper(from, to);
		}

	    }
	    else
	    {
		dbpname_p = (DB_DBP_NAME *)&dbp_nam;
		ownname_p = (DB_OWN_NAME *)&dbp_own;
	    }

	    if (   !MEcmp((PTR) &qea_act->qhd_obj.qhd_iproc.ahd_procnam,
		          (PTR) dbpname_p, sizeof(*dbpname_p))
		&& !MEcmp((PTR) &qea_act->qhd_obj.qhd_iproc.ahd_ownname,
			  (PTR) ownname_p, sizeof(*ownname_p))
	       )
	    {
		/* we have the procedure we want. */
		proc_case = qef_procs[i].case_id;
		break;
	    }
	}  /* end loop to recognize INTERNAL PROCEDURE */


	/* verify that the procedure we are processing has the same 
	** parameters that the QEF INTERNAL PROCEDURE EXPECTS.
	*/
	if (proc_case == NONEXISTENT_PROC)
	{
	    /* send error that this Internal Procedure does not exist */
	    dsh->dsh_error.err_code = E_QE0130_NONEXISTENT_IPROC;
            return (E_DB_ERROR);
    	}
	else if ((proc_case == UPDATE_CONFIG) || (proc_case == FIND_DBS))
	{
	    if (qp->qp_ndbp_params < exp_parm[proc_case].exp_num)
	    {
		/* 
		** These internal procedures support optional arguments, but 
		** certain ones must be specified.  In this case, not all of the
		** mandatory arguments were specified, so generate error that 
		** number of parameters is wrong. 
		*/
		dsh->dsh_error.err_code = E_QE0131_MISSING_IPROC_PARAM;
		return (E_DB_ERROR);
	    }
	}
    	else if (qp->qp_ndbp_params != exp_parm[proc_case].exp_num)
    	{
    	    /* generate error that number of parameters is wrong. */
	    dsh->dsh_error.err_code = E_QE0131_MISSING_IPROC_PARAM;
            return (E_DB_ERROR);
    	}

	/* validate that each parameter name,type is the one we expect */
	params = qp->qp_dbp_params;
	for (i=0; i<exp_parm[proc_case].exp_num; i++)
	{
	    STmove(exp_parm[proc_case].p[i].p_arg, ' ', 
		sizeof(dbp_parm), (char *)&dbp_parm);

	    if (upcase)
	    {
		/*
		** uppercase parameter name from the internal table
		*/
		char	    *from, *last, *to;

		for (from = (char *) &dbp_parm,
		     last = from + sizeof(dbp_parm),
		     to = (char *) attname_p;

		     from < last;

		     CMnext(from), CMnext(to)
		    )
		{
		    CMtoupper(from, to);
		}

	    }
	    else
	    {
		attname_p = (DB_ATT_NAME *)&dbp_parm;
	    }
	    
	    if (MEcmp((PTR) params[i].dbp_name, (PTR) attname_p,
		      sizeof(*attname_p))
	       )
	    {
		/* generate error that the parameter name is wrong */
		dsh->dsh_error.err_code = E_QE0132_WRONG_PARAM_NAME;
		return (E_DB_ERROR);

	    } /* endif parameter name was not correct */

	    /* now assure that parameter's type is what we expect */
	    if (exp_parm[proc_case].p[i].datatype != 
		params[i].dbp_dbv.db_datatype)
	    {
		/* generate message that parameter type is wrong */

		dsh->dsh_error.err_code = E_QE0133_WRONG_PARAM_TYPE;
		return (E_DB_ERROR);
	    }

	} /* end loop for each parameter to internal procedure */

	/*
	**  Now attempt to process that procedure.  If we did not recognize the
    	**  procedure, translate the error to something for qeq_query.
	*/

	switch (proc_case)
	{
	  case LIST_FILE:
	    retstat = qea_0list_file(qef_rcb,params,dsh);
	    break;

	  case DROP_FILE:
	    retstat = qea_1drop_file(qef_rcb,params,dsh);
	    break;

	  case CREATE_DB:
	    retstat = qea_2create_db(qef_rcb,params,dsh);
	    break;

	  case DESTROY_DB:
	    retstat = qea_3destroy_db(qef_rcb,params,dsh);
	    break;

	  case ALTER_DB:
	    retstat = qea_4alter_db(qef_rcb,params,dsh);
	    break;

	  case ADD_LOCATION:
	    retstat = qea_5add_location(qef_rcb, params, dsh);
	    break;

	  case DEL_LOCATION:
	    retstat = qea_5del_location(qef_rcb, params, dsh);
	    break;
    
	  case CHECK_TBL:
            retstat = qea_67_check_patch(qef_rcb,params,dsh,FALSE);
            break;

          case PATCH_TBL:
            retstat = qea_67_check_patch(qef_rcb,params,dsh,TRUE);
            break;

	  case FIND_DBS:
            retstat = qea_8_finddbs(qef_rcb,params,dsh);
            break;

	  case READ_CONFIG:
            retstat = qea_9_readconfig(qef_rcb,params,dsh);
            break;

	  case UPDATE_CONFIG:
            retstat = qea_10_updconfig(qef_rcb,params,dsh);
            break;

 	  case DEL_DMP_CONFIG:
            retstat = qea_11_deldmp_config(qef_rcb,params,dsh);
            break;

	  case ALTER_EXT:
	    retstat = qea_12alter_extension(qef_rcb, params, dsh);
	    break;

	  case CONVERT_TABLE:
	    retstat = qea_13_convert_table(qef_rcb,params,dsh);
	    break;

	  case ERRORP:
	    retstat = qea_14_error(params,dsh);
	    break;	

	  case MAKE_UDT_DEFAULTS:
	    retstat = qea_15_make_udt_defaults( qef_rcb, params,dsh );
	    break;

	  default:
	    /* send error message that procedure does not exist.  (Technically,
	    ** it should be impossible to get to the default case.)
	    */
	    dsh->dsh_error.err_code = E_QE0130_NONEXISTENT_IPROC;
	    retstat = E_DB_ERROR;
	    break;
	}

	return (retstat);
}

/*{
** Name: QEA_MK_DMMCB  - Make a generic (empty) DMM_CB
**
** Description:
**      This utility makes an "empty" dmm_cb.  The calling routine must
**	fill in user parameters.
**	    
** Inputs:
**    QEF_RCB				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    dmm_cb				Ptr to uninitialized DMM_CB
** Outputs:
**    *dmm_cb				Initializes the following dmm_cb 
**					fields:
**	.q_next					FW PTR (unchained)
**	.q_prev					BW PTR (unchained)
**	.length 				Size of DMM_CB
**	.type					Identifies type of CB
**	.s_reserved, .l_reserved		Reserved
**	.owner					CB Owner
**	.ascii_id				CB ASCII Identifier
**	.error					Error Commumication Block
**	.dmm_tran_id				DMF Transaction ID ptr
**
**	Returns:
**	    None.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      14-apr-1989  (teg)
**	    Initial Creation.
[@history_template@]...
*/
static void
qea_mk_dmmcb(
QEE_DSH            *dsh,
DMM_CB		   *dmm_cb )
{


    /* start by zeroing the whole structure, then fill in relevent fields */
    MEfill ( sizeof(DMM_CB), '\0', (PTR) dmm_cb);

    dmm_cb -> length 		= sizeof (DMM_CB);
    dmm_cb -> type   		= DMM_MAINT_CB;
    dmm_cb -> ascii_id		= DMM_ASCII_ID;
    dmm_cb -> dmm_tran_id	= dsh->dsh_dmt_id;
}

/*{
** Name: QEA_FINDLEN	- determine number of non-null characters in parameter
**
** Description:
**      this routine scans the input parameter character string looking for
**	the first whitespace character (blank, null, non-ascii, etc). It
**	returns the number of characters preceeding the whitespace.
**	However, the return values is GUARENTEED not to exceed the original
**	lenght of the input parameter.
**
** Inputs:
**      parm                            pointer to a char string parameter
**      len                             max size (Bytes) of char string in parm
**
** Outputs:
**      none
**
**	Returns:
**	    number of non-white characters in parm
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-apr-1989 (teg)
**	    created for Internal Procedures.
[@history_template@]...
*/
i4
qea_findlen(
char	    *parm,
i4	    len )
{
    char    *x;
    i4	    i;

    for(x=parm, i=0; i<len; CMnext(x), i++)
    {
	if (CMwhite(x))
	    return (i);
    }

    return (len);
}

/*{
** Name: QEA_0LIST_FILE	- implement iiQEF_listfile Internal Procedure
**
** Description:
**      This INTERNAL Procedure is to list all files subordinate to a
**	directory.  The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_listfile (
**	    directory=char(DB_MAXNAME) not null not default,
**	    table =char(DB_MAXNAME) not null not default,
**	    owner =char(DB_OWN_MAXNAME) not null not default,
**	    column=char(DB_MAXNAME) not null not default )
**	AS begin
**	      execute internal;
**	   end;
**
**	and evoked by:
**	
**	EXECUTE PROCEDURE iiQEF_listfile (directory = :full_pathname,
**		table = :existing_table, owner = :tables_owner,
**		column = :column_name)
**
**	The directory parameter should contain the full pathname to a
**	database default or extended data directory as a null terminated
**	ASCII string.  The table and owner table parameters should define
**	the table to list files to.  That table may contain any number of
**	columns, but the column specified in parameter "column" must be
**	a char(28) or larger.
**	
**	qea_0list_file creates an "empty" dmm_cb.  Then it fills in some
**	of the dmm_cb items from user parameters:
**		dmm_flags_mask	=   DMM_FILELIST
**		dmm_db_location =   directory parameter
**		dmm_table_name  =   table parameter
**		dmm_owner	=   owner parameter
**		dmm_att_name	=   column parameter
**		dmm_count	=   0
**	qea_0list_file calls DMF to do a DMM_LIST operation.  If the list 
**	is successful, qea_0list_file will take the dmm_cb.dmm_count and
**	put it into the QEF_RCB.dbp_return.
**
**	If dmf is unsuccessful, then qea_0list_file will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    User Specified Table is populated with tuples -- one tuple for
**	    each file found.  The filename is placed in the user specified
**	    column.  If the table contains any additional columns, those columns
**	    contain Default or Null values.
**
** History:
**      14-apr-1989  (teg)
**	    Initial Creation.
[@history_template@]...
*/
DB_STATUS
qea_0list_file(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;
    
    /* 
    **  set up initialized DMM_CB 
    */
    qea_mk_dmmcb(dsh, &dmm_cb );

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   DMM_FILELIST
    **		dmm_db_location =   directory parameter
    **		dmm_table_name  =   table parameter
    **		dmm_owner	=   owner parameter
    **		dmm_att_name	=   column parameter
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask		    =   DMM_FILELIST;

    /* 1st parameter = directory */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    dmm_cb.dmm_db_location.data_address  =
	    (char *) dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    /* 2nd parameter = table */
    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEfill( sizeof(DB_TAB_NAME), ' ', (PTR) dmm_cb.dmm_table_name.db_tab_name);
    MEcopy( (PTR)((char *) dsh->dsh_row[db_parm[1].dbp_rowno])+
		    db_parm[1].dbp_offset,
	    row_dbv.db_length, (PTR) dmm_cb.dmm_table_name.db_tab_name);
    
    /* 3rd parameter = owner */
    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    MEfill( sizeof(DB_OWN_NAME), ' ', (PTR) dmm_cb.dmm_owner.db_own_name);
    MEcopy( (PTR)((char *) dsh->dsh_row[db_parm[2].dbp_rowno]) + 
	    db_parm[2].dbp_offset, row_dbv.db_length, 
	    (PTR) dmm_cb.dmm_owner.db_own_name);

    /* 4th parameter = column */
    STRUCT_ASSIGN_MACRO( db_parm[3].dbp_dbv, row_dbv);
    MEfill( sizeof(DB_ATT_NAME), ' ', (PTR) dmm_cb.dmm_att_name.db_att_name);
    MEcopy( (PTR)((char *) dsh->dsh_row[db_parm[3].dbp_rowno])
	    + db_parm[3].dbp_offset, row_dbv.db_length, 
	    (PTR) dmm_cb.dmm_att_name.db_att_name);

    dmm_cb.dmm_count	= 0;

    /*
    **	call DMF to do list operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_LISTFILE, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb -> qef_dbp_status = dmm_cb.dmm_count;
    return (status);

}

/*{
** Name: QEA_1DROP_FILE	- implement iiQEF_dropfile Internal Procedure
**
** Description:
**      This INTERNAL Procedure is to delete a specified DI file from a
**	specifeid directory.  The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_dropfile (
**	    directory=char(DB_MAXNAME) not null not default,
**	    file  =char not null not default )
**	AS begin
**	      execute internal;
**	   end;
**
**	and evoked by:
**	
**	EXECUTE PROCEDURE iiQEF_dropfile (directory = :full_pathname,
**					  file = :filename)
**
**	The directory parameter should contain the full pathname to a
**	database default or extended data directory as a null terminated
**	ASCII string.  The file parameter should contain a filename
**	returned from the iiQEF_listfile() procedure -- and is a
**	temporary file not assoicated with the database session.
**	
**	qea_1drop_file creates an "empty" dmm_cb.  Then it fills in some
**	of the dmm_cb items from user parameters:
**		dmm_flags_mask	=   0
**		dmm_db_location =   directory parameter
**		dmm_filename	=   file name parameter
**	qea_1drop_file calls DMF to do a DMM_DROP operation.
**
**	If dmf is unsuccessful, then qea_1drop_file will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    User Specified FILE is removed from the directory.  No logging
**	    or logging is performed for this delete.
**
** History:
**      14-apr-1989  (teg)
**	    Initial Creation.
[@history_template@]...
*/
DB_STATUS
qea_1drop_file(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    /* 
    **  set up initialized DMM_CB 
    */
    qea_mk_dmmcb(dsh, &dmm_cb );

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask	=   0;

    /* 1st parameter = directory */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    dmm_cb.dmm_db_location.data_address  =
	    (char *) dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);    
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    /* 2nd parameter = filename */
    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    dmm_cb.dmm_filename.data_address  =
	    (char *) dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset;
    real_length = qea_findlen( (char *)dmm_cb.dmm_filename.data_address,
				row_dbv.db_length);    
    dmm_cb.dmm_filename.data_in_size  = real_length;
    dmm_cb.dmm_filename.data_out_size = real_length;

    /*
    **	call DMF to do list operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_DROPFILE, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb -> qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: QEA_2CREATE_DB	- implement iiQEF_create_db Internal Procedure
**
** Description:
**      This INTERNAL Procedure is used to create a database.
**	The procedure is defined as:
**
**	EXEC SQL CREATE PROCEDURE iiqef_create_db(
**		dbname         = char(32) not null not default,
**		db_loc_name    = char(32) not null not default,
**		jnl_loc_name   = char(32) not null not default,
**		ckp_loc_name   = char(32) not null not default,
**		dmp_loc_name   = char(32) not null not default,
**		srt_loc_name   = char(32) not null not default,
**		db_access      = integer  not null not default,
**		collation      = char(32) not null not default,
**		need_dbdir_flg = integer  not null not default,
**		db_service     = integer  not null not default,
**		translated_owner_name   = char(32) not null not default,
**		untranslated_owner_name = char(32) not null not default)
**	    AS BEGIN
**		EXECUTE INTERNAL;
**	    END;
**
**	and invoked by:
**	
**	    exec sql execute procedure iiQEF_create_db (
**		    dbname              =   :dbname,
**		    db_loc_name         =   :db_loc,
**		    jnl_loc_name        =   :jnl_loc,
**		    ckp_loc_name        =   :ckp_loc,
**		    dmp_loc_name        =   :dmp_loc,
**		    wrk_loc_name        =   :wrk_loc,
**		    db_access           =   :access,
**		    collation           =   :collation,
**		    need_dbdir_flg	=   :dir_flag,
**		    db_service          =   :dbservice,
**	    translated_owner_name	=   :trans_owner_name,
**	    untranslated_owner_name	=   :untrans_owner_name)
**
**	dbname is the name of the db being created, and db_loc_name is
**	the root data location; jnl, ckp, dmp, and wrk loc's are the journal,
**	checkpoint, dump, and initial work locations the db is to be created
**	with; collation is the collation sequence the db is to use;
**	need_dbdir_flg is NOT USED; I don't know what db_access and db_service
**	are.
**	
**	Translated_owner_name is the database-owner's name, translated
**	according to the case semantics of the database being created;
**	this is used by DMF as the dbowner_name to store in the CONFIG file.
**	
**	Untranslated_owner_name is the untranslated database-owner's name,
**	as obtained directly from the operating system or -u flag;
**	this is not currently used, but may become necessary if we decide to
**	store the untranslated name in the iidatabase catalog (i.e. add a new
**	field to that catalog).
**	
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	A database is created in the specified area owned by the caller.
**
** History:
**      15-may-1989  (Derek)
**	    Created.
**	08-feb-1991 (ralph)
**	    Fold database name, location name to lower case
**	02-apr-1991 (tersea)
**	    add need_dbdir_flg parameter so convto60 may also use this
**	    mechanism.  Also update description section since folks had changed
**	    parameter list and neglected to change description.
**	16-jan-1992 (teresa)
**	    add dbservice parameter to pick up a 6.4 RMS GW bugfix where the
**	    dbservice needs to be in the config file before the server connects
**	    to the database. (ingres63p change 262603)
**	23-oct-1992 (bryanp)
**	    Improve error handling following failure in dmf_call().
**	14-dec-92 (jrb)
**	    Removed "need_dbdir_flag" since convto60 won't be used for 6.5 and
**	    therefore won't need to use this code.
**	09-nov-93 (rblumer)
**	    Added translated_owner_name and untranslated_owner_name parameters.
**	    Translated_owner_name is passed to DMF to be put into the CONFIG
**	    file; untranslated_owner_name is currently unused but may be used
**	    in the future, to be put into the iidatabase tuple.
**	28-mar-1994 (bryanp) B60598
**	    Remove extra computation of database.du_dbid from createdb code.
**	    This line appeared to have been accidentally introduced into the
**		code a long time ago. Normally it wasn't causing any problems,
**		but if the timing were just right it was possible for the code
**		to generate one database ID for the iidatabase tuple, and then
**		to have the clock "tick over" to the next second and have the
**		code generate a different database ID value for the dmf call.
**	2-nov-1995 (angusm)
**		Location names are not always forced lowercase as for database
**		names: under default they will be lowercase, even delim ids,
**		under FIPS/SQL-92 uppercase, ditto
**	07-mar-2000 (gupsh01)
**	    Added error checking for DMM_CREATE_DB call to check for readonly 
**	    database creation error, E_DM0192_CREATEDB_RONLY_NOTEXIST.
**	19-mar-2001 (stephenb)
**	    Handle unicode collation sequence.
**	02-Apr-2001 (jenjo02)
**	    Initialize du_raw_start in DU_EXTEND.
**	04-Dec-2001 (jenjo02)
**	    Cleaned up error handling after DMM_CREATE_DB call.
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now an integer.
*/
DB_STATUS
qea_2create_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    DMR_ATTR_ENTRY	key;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	*key_ptr = &key;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_ERROR		error;
    i4		err_code;
    struct
    {
	i2		l_location_name;
	char		location_name[DB_LOC_MAXNAME];
	i2		l_area_name;
	char		location_area[DB_AREA_MAX];
	i4		location_type;
	i4		location_access;
    }			location_array[5];
    DMM_LOC_LIST	loc_list[4];
    PTR			loc_ptr[4];
    QEU_CB		qeu;
    DU_LOCATIONS	location;
    DU_DBACCESS		dbaccess;
    DU_EXTEND		dbextend;
    DU_DATABASE		database;
    int			private_db;
    int			dbservice;
    int			need_dbdir_flg;
    int			i;
    DB_STATUS		local_status;
    char		tempstr[TEMPLEN];
    GLOBALREF           QEF_S_CB	*Qef_s_cb;
    SXF_RCB	    	sxfrcb;
    i4		local_error;


    /*	Initialize location information. */

    location_array[0].location_type = DMM_L_DATA;
    location_array[1].location_type = DMM_L_JNL;
    location_array[2].location_type = DMM_L_CKP;
    location_array[3].location_type = DMM_L_DMP;
    location_array[4].location_type = DMM_L_WRK;
    location_array[0].location_access = DU_DBS_LOC;
    location_array[1].location_access = DU_JNLS_LOC;
    location_array[2].location_access = DU_CKPS_LOC;
    location_array[3].location_access = DU_DMPS_LOC;
    location_array[4].location_access = DU_WORK_LOC;

    qea_mk_dmmcb(dsh, &dmm);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    dmm.dmm_flags_mask = 0;

    /* 1st parameter = db_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);

    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm.dmm_db_name), (char *)&dmm.dmm_db_name);

    /* 7th parameter = db_access. */

    STRUCT_ASSIGN_MACRO(db_parm[6].dbp_dbv, row_dbv);
    private_db  = *(i4 *)((char *)dsh->dsh_row[db_parm[6].dbp_rowno] +
	db_parm[6].dbp_offset);

    /*	Check user has privilege to create a database. */

    if ((qef_cb->qef_ustat & DU_UCREATEDB) == 0)
    {
	DB_ERROR e_error;

	/*	No privilege. */

	dsh->dsh_error.err_code = E_US0060_CREATEDB_NO_PRIV;
	qef_rcb->qef_dbp_status = (i4) E_DB_ERROR;

	/* Must audit this failure. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&dmm.dmm_db_name,
			    &qef_cb->qef_user,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX2029_DATABASE_CREATE, SXF_A_FAIL | SXF_A_CREATE,
			    &e_error);

	return (E_DB_ERROR);
    }

    /*	Check collation name. */

    /* 8th parameter = collation */

    STRUCT_ASSIGN_MACRO(db_parm[7].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[7].dbp_rowno] + db_parm[7].dbp_offset,
	' ', sizeof(dmm.dmm_table_name), (char *)&dmm.dmm_table_name);

    /* 9th parameter = need_dbdir_flg */
    /* NOTE: this parameter is not currently used (it was added at one point
    **		to support CONVTO60, but we will no longer support CONVTO60
    **		for future releases of INGRES).
    */
    STRUCT_ASSIGN_MACRO(db_parm[8].dbp_dbv, row_dbv);
    need_dbdir_flg = *(i4 *)((char *)dsh->dsh_row[db_parm[8].dbp_rowno] +
	db_parm[8].dbp_offset);

    /* 10th parameter = db_service */
    STRUCT_ASSIGN_MACRO(db_parm[9].dbp_dbv, row_dbv);
    dbservice = *(i4 *)((char *)dsh->dsh_row[db_parm[9].dbp_rowno] +
		  db_parm[9].dbp_offset);
#if defined(LP64)
    dbservice |= DU_LP64;
#endif

    /* 11th parameter =  translated_owner_name;
    ** Copy it into the dmm_owner field to pass to DMF.
    */
    STRUCT_ASSIGN_MACRO(db_parm[10].dbp_dbv, row_dbv);
    MEfill( sizeof(DB_OWN_NAME), ' ', (PTR) dmm.dmm_owner.db_own_name);
    MEcopy( (PTR)((char *) dsh->dsh_row[db_parm[10].dbp_rowno]) + 
	    db_parm[10].dbp_offset, row_dbv.db_length, 
	    (PTR) dmm.dmm_owner.db_own_name);

    /* the 12th parameter is untranslated_owner_name;
    ** this is currently unused, but may become necessary if we ever
    ** decide to store the untranslated name in the iidatabase tuple
    ** (i.e. add a new field to the tuple).     (rblumer 09-nov-93)
    */

    /* 13th parameter = ucollation */

    STRUCT_ASSIGN_MACRO(db_parm[12].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[12].dbp_rowno] + db_parm[12].dbp_offset,
	' ', sizeof(dmm.dmm_utable_name), (char *)&dmm.dmm_utable_name);

    /*	Verify names, access to locations and get the physical area name. */

    {
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    for (i = 0; i < 5; i++)
	    {
		/*  Get next location name. */

		STRUCT_ASSIGN_MACRO(db_parm[i+1].dbp_dbv, row_dbv);
		location_array[i].l_location_name = row_dbv.db_length;
		MEmove(row_dbv.db_length,
		    (char *)dsh->dsh_row[db_parm[i+1].dbp_rowno]
			+ db_parm[i+1].dbp_offset,
		    ' ', sizeof(tempstr)-1, tempstr);
		tempstr[sizeof(tempstr)-1] = '\0';

/*
**		CVlower(tempstr);
	Locations are not forced lowercase.
	bug 71173 - cannot create db on alternate locations
	under FIPS/SQL-92
*/

		MEmove(sizeof(tempstr)-1, tempstr,
		    ' ', sizeof(location_array[i].location_name),
		    location_array[i].location_name);

		/*  Lookup the location. */

		qeu.qeu_count = 1;
		qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
		qeu.qeu_output = &qef_data;
		qef_data.dt_next = 0;
		qef_data.dt_size = sizeof(DU_LOCATIONS);
		qef_data.dt_data = (PTR)&location;
		qeu.qeu_getnext = QEU_REPO;
		qeu.qeu_klen = 1;
		qeu.qeu_key = &key_ptr;
		key.attr_number = DM_1_LOCATIONS_KEY;
		key.attr_operator = DMR_OP_EQ;
		key.attr_value = (char *)&location_array[i].l_location_name;
		qeu.qeu_qual = 0;
		qeu.qeu_qarg = 0;
		status = qeu_get(qef_cb, &qeu);
		if (status != E_DB_OK)
		{
		    status = E_DB_ERROR;
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			dsh->dsh_error.err_code = E_US0080_LOCATION_NOT_FOUND;
		    }
		    break;
		}

		/*  Check that access is allowed to this location. */

		if ((location_array[i].location_access & location.du_status) == 0)
		{
		    dsh->dsh_error.err_code = E_US0081_LOCATION_NOT_ALLOWED;
		    status = E_DB_ERROR;
		    break;
		}
		MEcopy( ( PTR ) location.du_area,
		     sizeof(location_array[i].location_area),
		     ( PTR ) location_array[i].location_area);
		location_array[i].l_area_name = location.du_l_area;
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	    if (local_status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = E_US0020_IILOCATIONS_OPEN;
		return (local_status);
	    }
	    if (status != E_DB_OK)
		return (status);
	}
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0020_IILOCATIONS_OPEN;
	    return (status);
	}
    }

    /*	Insert iidatabase tuple. */

    MEcopy( ( PTR ) &dmm.dmm_db_name, DB_DB_MAXNAME,
	    ( PTR ) database.du_dbname);
    STRUCT_ASSIGN_MACRO(qef_cb->qef_user, database.du_own);
    MEmove(location_array[0].l_location_name,
	location_array[0].location_name, ' ',
	sizeof(database.du_dbloc), (char *)&database.du_dbloc);
    MEmove(location_array[1].l_location_name,
	location_array[1].location_name, ' ',
	sizeof(database.du_jnlloc), (char *)&database.du_jnlloc);
    MEmove(location_array[2].l_location_name,
	location_array[2].location_name, ' ',
	sizeof(database.du_ckploc), (char *)&database.du_ckploc);
    MEmove(location_array[3].l_location_name,
	location_array[3].location_name, ' ',
	sizeof(database.du_dmploc), (char *)&database.du_dmploc);
    MEmove(location_array[4].l_location_name,
	location_array[4].location_name, ' ',
	sizeof(database.du_workloc), (char *)&database.du_workloc);
    database.du_access = private_db;
    database.du_dbservice = dbservice;
    database.du_dbcmptlvl = DU_CUR_DBCMPTLVL;
    database.du_1dbcmptminor = DU_1CUR_DBCMPTMINOR;
    database.du_dbid = TMsecs();

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
    qeu.qeu_db_id = qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IX;
    qeu.qeu_flag = 0;
    qeu.qeu_access_mode = DMT_A_WRITE;
    qeu.qeu_mask = 0;
    local_status = status = qeu_open(qef_cb, &qeu);
    while (status == E_DB_OK)
    {
	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_DATABASE);
	qeu.qeu_input = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_DATABASE);
	qef_data.dt_data = (PTR)&database;
	status = qeu_append(qef_cb, &qeu);
	if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	{
	    if (status == E_DB_OK)
	    {
		if (qeu.error.err_code == E_DM0048_SIDUPLICATE_KEY)
		{
		    database.du_dbid = TMsecs();
		    continue;
		}
		if (qeu.error.err_code == E_DM0045_DUPLICATE_KEY)
		    dsh->dsh_error.err_code = E_US0085_DATABASE_EXISTS;
	    }
	    else
		dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    status = E_DB_ERROR;
	}
	local_status = qeu_close(qef_cb, &qeu);    
	break;
    }

    if (local_status != E_DB_OK && local_status == status)
    {
	dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	return (local_status);
    }

    if (status != E_DB_OK)
    {
	return (status);
    }

    /*	Insert iiextend tuple for data extent. */

    dbextend.du_l_length = location_array[0].l_location_name;
    MEcopy( ( PTR ) location_array[0].location_name,
	location_array[0].l_location_name, ( PTR ) dbextend.du_lname);
    dbextend.du_d_length = sizeof(dbextend.du_dname);
    MEcopy( ( PTR ) &dmm.dmm_db_name, sizeof(dbextend.du_dname),
		 ( PTR ) dbextend.du_dname);
    dbextend.du_status = DU_EXT_OPERATIVE | DU_EXT_DATA;
    dbextend.du_raw_start = 0;
    dbextend.du_raw_blocks = 0;

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
    qeu.qeu_db_id = qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IX;
    qeu.qeu_flag = QEU_BYPASS_PRIV;
    qeu.qeu_access_mode = DMT_A_WRITE;
    qeu.qeu_mask = 0;
    local_status = status = qeu_open(qef_cb, &qeu);
    if (status == E_DB_OK)
    {
	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_EXTEND);
	qeu.qeu_input = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_EXTEND);
	qef_data.dt_data = (PTR)&dbextend;
	status = qeu_append(qef_cb, &qeu);
	if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	{
	    dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	    status = E_DB_ERROR;
	}
	local_status = qeu_close(qef_cb, &qeu);    
    }

    if (local_status != E_DB_OK && local_status != status)
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	return (local_status);
    }

    if (status != E_DB_OK)
    {
	return (status);
    }

    dbextend.du_l_length = location_array[4].l_location_name;
    MEcopy( ( PTR ) location_array[4].location_name,
	location_array[4].l_location_name, ( PTR ) dbextend.du_lname);
    dbextend.du_d_length = sizeof(dbextend.du_dname);
    MEcopy( ( PTR ) &dmm.dmm_db_name, sizeof(dbextend.du_dname),
		 ( PTR ) dbextend.du_dname);
    dbextend.du_status = DU_EXT_OPERATIVE | DU_EXT_WORK;
    dbextend.du_raw_start = 0;
    dbextend.du_raw_blocks = 0;

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
    qeu.qeu_db_id = qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IX;
    qeu.qeu_flag = QEU_BYPASS_PRIV;
    qeu.qeu_access_mode = DMT_A_WRITE;
    qeu.qeu_mask = 0;
    local_status = status = qeu_open(qef_cb, &qeu);
    if (status == E_DB_OK)
    {
	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_EXTEND);
	qeu.qeu_input = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_EXTEND);
	qef_data.dt_data = (PTR)&dbextend;
	status = qeu_append(qef_cb, &qeu);
	if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	{
	    dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	    status = E_DB_ERROR;
	}
	local_status = qeu_close(qef_cb, &qeu);    
    }

    if (local_status != E_DB_OK && local_status != status)
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	return (local_status);
    }

    if (status != E_DB_OK)
    {
	return (status);
    }

    /*	Convert locations into loc_list for DMM_CREATE_DB. */

    for (i = 0; i < 4; i++)
    {
	loc_ptr[i] = (PTR)&loc_list[i];
	loc_list[i].loc_type = location_array[i+1].location_type;
	MEcopy( ( PTR ) location_array[i+1].location_name,
		sizeof(loc_list[i].loc_name),
		( PTR ) &loc_list[i].loc_name);
	MEcopy( ( PTR ) location_array[i+1].location_area,
	    location_array[i+1].l_area_name,
	    ( PTR ) loc_list[i].loc_area );
	loc_list[i].loc_l_area = location_array[i+1].l_area_name;
    }

    /*	Call create core catalog DMF operation. */

    dmm.dmm_loc_list.ptr_address = (PTR)loc_ptr;
    dmm.dmm_loc_list.ptr_in_count = 4;
    dmm.dmm_loc_list.ptr_size = sizeof(DMM_LOC_LIST);
    dmm.dmm_db_location.data_address = location_array[0].location_area;
    dmm.dmm_db_location.data_in_size = location_array[0].l_area_name;
    MEcopy( ( PTR ) location_array[0].location_name,
	    sizeof(dmm.dmm_location_name),
	    ( PTR ) &dmm.dmm_location_name);
    dmm.dmm_db_id = database.du_dbid;
    dmm.dmm_db_service = database.du_dbservice;
    dmm.dmm_db_access = database.du_access;
    status = dmf_call(DMM_CREATE_DB, &dmm);
    if (status != E_DB_OK)
	dsh->dsh_error.err_code = dmm.error.err_code;
    else
    {
	qef_rcb -> qef_dbp_status = (i4) status;

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&dmm.dmm_db_name,
			    &qef_cb->qef_user,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX2029_DATABASE_CREATE, SXF_A_SUCCESS | SXF_A_CREATE,
			    &e_error);

	    if (status != E_DB_OK)
		dsh->dsh_error.err_code = e_error.err_code;
	}
    }

    return (status);
}

/*{
** Name: QEA_3DESTROY_DB	- implement iiQEF_destroy_db Internal Procedure
**
** Description:
**      This INTERNAL Procedure is used to destroy a database.
**	The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_destroy_db(
**	    dbname=char(DB_DB_MAXNAME) not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	and evoked by:
**	
**	EXECUTE PROCEDURE iiQEF_destroy_db(
**	    dbname = :db_name)
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	A database is destroyed in the specified area.
**
** History:
**      15-may-1989  (Derek)
**	    Created.
**	08-feb-1991 (ralph)
**	    Fold database name to lower case
**	7-dec-93 (robf)
**          Delete any security alarms on the database.
**	17-dec-93 (robf)
**          Delete query text associated with database alarms 
**	    from iiqrytext.
**      31-jan-1995 (stial01)
**          BUG 66510: If possible, prepare list of locations in dmm_loc_list.
**          The list will be used by dmm_destroy() if the database directory
**          was deleted, for destroying the other locations.
**	17-feb-1995 (forky01)
**	    BUG 66865: Change iiextend access to QEU_BYPASS_PRIV, iidbpriv
**	    to DMT_IX from DMT_X, iiqrytext to DMT_IX from X, iisecalarm
**	    to DMT_IX from DMT_X.  See also comment at top of file for 
**	    qeu_mask.
**	21-mar-1995 (harpa06)
**	    Bug #67531: Destroydb would fail because of information in QQEU 
**	    stucture being accidently overwritten. Information being placed 
**	    in wrong array elements.
**	06-mar-1996 (nanpr01)
**	    Change for variable page size project. Previously qeu_delete 
**	    was using DB_MAXTUP to get memory. With the bigger pagesize
**	    it was unnecessarily allocating much more space than was needed.
**	    Instead we initialize the correct tuple size, so that we can 
**	    allocate the actual size of the tuple instead of max possible
**	    size.
** 	24-may-1997 (wonst02)
** 	    Pass the database access mode to dmm_destroy for readonly databases.
**	10-Jan-2001 (jenjo02)
**	    Removed all those return() on error statements. Doing that
**	    will leak the ulm memory stream, so make sure that all code
**	    falls to the bottom to close the stream!
**	03-Feb-2004 (jenjo02)
**	    If createdb was aborted, iiextend may not have
**	    been populated; be more forgiving if the
**	    delete doesn't delete anything (qeu_count == 0) but 
**	    otherwise succeeds.
[@history_template@]...
*/
DB_STATUS
qea_3destroy_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DB_ERROR		error;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    STATUS		local_status;
    DU_DATABASE		database;
    DU_EXTEND		extend;
    DU_LOCATIONS	location;
    DU_LOCATIONS        root_loc;
    DB_PRIVILEGES	dbpriv;
    DB_SECALARM		dbalarm;
    DB_IIQRYTEXT        qtuple;
    DB_DB_NAME		db_name;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	key;
    DMR_ATTR_ENTRY	*key_ptr = &key;
    DMR_ATTR_ENTRY	qkey_array[2];
    DMR_ATTR_ENTRY	*qkey_ptr_array[2];
    QEU_CB		qeu;
    DB_ERROR		err;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    char		tempstr[TEMPLEN];
    GLOBALREF           QEF_S_CB	*Qef_s_cb;
    QEF_DATA		qqef_data;
    QEU_CB		qqeu;
    i4		err_code;
    bool		qrytext_opened=FALSE;
    PTR                 *loc_ptr;
    DMM_LOC_LIST        *loc_list;
    i4             ext_count;
    i4             loc_count;
    ULM_RCB             ulmRCB, *ulm = &ulmRCB;
    i4             ulm_err;
    i4             i;
    DU_LOCATIONS        tmp_loc;

    struct
    {
	i2		lname;		    /* length of ... */
	char		name[DB_LOC_MAXNAME];   /* A location name. */
    }			location_name;

    dsh->dsh_error.err_code = E_DB_OK;

    qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask = 0;

    /* 1st parameter = db_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);
    db_name = dmm_cb.dmm_db_name;

    {
	/*  Get record from iidatabase for this database. */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_DATABASE_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0010_NO_SUCH_DB;
		}
	    }
	    dmm_cb.dmm_db_access = database.du_access;
	    local_status = qeu_close(qef_cb, &qeu);    
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /*	Check privilege to destroy database. */
   
    {
	char	obj_name[sizeof(DB_OWN_NAME) * 2];

	if (MEcmp( ( PTR ) &database.du_own, ( PTR ) &qef_cb->qef_user,
		   sizeof(DB_OWN_NAME)))
	{
	    /* Must audit this failure. */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		DB_ERROR	e_error;

		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&db_name,
			    &database.du_own,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX202A_DATABASE_DROP, SXF_A_FAIL | SXF_A_DROP,
			    &e_error);
	    }

	    dsh->dsh_error.err_code = E_US0003_NO_DB_PERM;
	    qef_rcb->qef_dbp_status = (i4) E_DB_ERROR;
	    return (E_DB_ERROR);
	}
    }

    /* Determine number of iiextend tuples for this database */
    status = get_extent_info(dsh, GET_EXTENT_COUNT, 
		&dmm_cb.dmm_db_name, (DMM_LOC_LIST **)0, &ext_count);

    if (status != E_DB_OK)
	loc_count = ext_count = 0;
    else
	loc_count = ext_count + 3; /* plus default jnl,ckp,dump */

    do
    {
	/*
	** Open a memory stream to hold 'loc_count' DMM_LOC_LIST entries.
	** We need to pass all location information to dmf in case
	** the config file cannot be read.
	*/
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, *ulm);
	ulm->ulm_streamid = (PTR)NULL;

	if (!ext_count)
	    break;

	/* Open and allocate in one action */
	ulm->ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm->ulm_psize = ulm->ulm_blocksize =
		loc_count * (sizeof(PTR) + sizeof(DMM_LOC_LIST));
	if ((status = qec_mopen( ulm )) != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    ulm_err = E_QE001E_NO_MEM;
	    qef_error( ulm_err, 0L, status, &ulm_err, &dsh->dsh_error, 0 );
	    return (E_DB_ERROR);
	}

	loc_ptr = (PTR *)ulm->ulm_pptr;
	loc_list = (DMM_LOC_LIST*)((char*)loc_ptr + (loc_count * sizeof(PTR)));

	for (i = 0; i < loc_count; i++)
	{
	    loc_ptr[i] = (PTR)&loc_list[i];
	}

	/* Initialize the DMM_LOC_LIST entries for this database */
	status = get_extent_info(dsh, GET_EXTENT_AREAS, 
		&dmm_cb.dmm_db_name, (DMM_LOC_LIST **)loc_ptr, (i4 *)0);

	if (status != E_DB_OK)
	    break;
	
	/* Translate the default jnl location for the database. */
	status = get_loc_tuple(dsh, &database.du_jnlloc, &tmp_loc);

	if (status != E_DB_OK)
	    break;

	/* copy jnl location into DMM_LOC_LIST */
	MEmove( tmp_loc.du_l_lname,
		(PTR)tmp_loc.du_lname, ' ',
		sizeof (DB_LOC_NAME),
		(PTR)&loc_list[ext_count].loc_name);
	MEcopy( (PTR)tmp_loc.du_area, tmp_loc.du_l_area, 
		(PTR)loc_list[ext_count].loc_area);
	loc_list[ext_count].loc_l_area = tmp_loc.du_l_area;
	loc_list[ext_count].loc_type = DMM_L_JNL;

	/* Translate the default ckp location for the database. */
	status = get_loc_tuple(dsh, &database.du_ckploc, &tmp_loc);

	if (status != E_DB_OK)
	    break;

	/* copy ckp location into DMM_LOC_LIST */
	MEmove( tmp_loc.du_l_lname,
		(PTR)tmp_loc.du_lname, ' ',
		sizeof (DB_LOC_NAME),
		(PTR)&loc_list[ext_count + 1].loc_name);
	MEcopy( (PTR)tmp_loc.du_area, tmp_loc.du_l_area, 
		(PTR)loc_list[ext_count + 1].loc_area);
	loc_list[ext_count + 1].loc_l_area = tmp_loc.du_l_area;
	loc_list[ext_count + 1].loc_type = DMM_L_CKP;

	/* Translate the default dmp location for the database. */
	status = get_loc_tuple(dsh, &database.du_dmploc, &tmp_loc);

	if (status != E_DB_OK)
	    break;

	/* copy dmp location into DMM_LOC_LIST */
	MEmove( tmp_loc.du_l_lname,
		(PTR)tmp_loc.du_lname, ' ',
		sizeof (DB_LOC_NAME),
		(PTR)&loc_list[ext_count + 2].loc_name);
	MEcopy( (PTR)tmp_loc.du_area, tmp_loc.du_l_area, 
		(PTR)loc_list[ext_count + 2].loc_area);
	loc_list[ext_count + 2].loc_l_area = tmp_loc.du_l_area;
	loc_list[ext_count + 2].loc_type = DMM_L_DMP;

	/* Location info initialized so copy to dmm_cb */
	dmm_cb.dmm_loc_list.ptr_address = (PTR)loc_ptr;
	dmm_cb.dmm_loc_list.ptr_in_count = loc_count;
	dmm_cb.dmm_loc_list.ptr_size = sizeof(DMM_LOC_LIST);
    } while (FALSE);

    /* If we can't build the location list, proceed without it */
    if (status != E_DB_OK)
    {
	status = E_DB_OK;
    }

    /* Translate the default data location for the database. */
    status = get_loc_tuple(dsh, &database.du_dbloc, &root_loc);

    /*	Delete all iiextend tuples for this database. */

    if ( status == E_DB_OK )
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	MEcopy( ( PTR ) &db_name, DB_LOC_MAXNAME, ( PTR ) location_name.name);
	location_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;
	qeu.qeu_key = &key_ptr;
	key.attr_number = DM_1_EXTEND_KEY;
	key.attr_operator = DMR_OP_EQ;
	key.attr_value = (char *)&location_name;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    /* Delete the existing tuples, if any */

	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    status = qeu_delete(qef_cb, &qeu);
	    if ((status != E_DB_OK))
	    {
		dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
	    }
	}
	if (local_status == E_DB_OK)
	    local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	    status = local_status;
	}
    }

    /*	Delete all iidbpriv tuples for this database. */

    if ( status == E_DB_OK )
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DBPRIV_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DBPRIV_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX; 
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 0;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;

	while (status == E_DB_OK)
	{
	    /* Read the iidbpriv records that pertain to this database. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DB_PRIVILEGES);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DB_PRIVILEGES);
	    qef_data.dt_data = (PTR)&dbpriv;

	    status = qeu_get(qef_cb, &qeu);
	    if ((status != E_DB_OK) &&
		(qeu.error.err_code == E_QE0015_NO_MORE_ROWS))
	    {
		status = E_DB_OK;
		break;
	    }
	    else if ((status == E_DB_OK) && (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0086_DBPRIV_UPDATE;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
		break;
	    }
	    if (MEcmp( ( PTR ) &db_name,  ( PTR ) &dbpriv.dbpr_database,
			DB_DB_MAXNAME) == 0)
	    {
		/* Delete the existing tuple */

		status = qeu_delete(qef_cb, &qeu);
		if ((status != E_DB_OK) || (qeu.qeu_count != 1))
		{
		    dsh->dsh_error.err_code = E_US0086_DBPRIV_UPDATE;
		    if (status < E_DB_ERROR)
			status = E_DB_ERROR;
		    break;
		}
	    }
	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	}
	if (local_status == E_DB_OK)
	    local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0086_DBPRIV_UPDATE;
	    status = local_status;
	}
    }

    /*	Delete all iisecalarm tuples for this database. */

    if ( status == E_DB_OK )
    {
	qqeu.qeu_type = QEUCB_CB;
	qqeu.qeu_length = sizeof(QEUCB_CB);
        qqeu.qeu_tab_id.db_tab_base  = DM_B_QRYTEXT_TAB_ID;
        qqeu.qeu_tab_id.db_tab_index  = DM_I_QRYTEXT_TAB_ID;
        qqeu.qeu_db_id = qef_rcb->qef_db_id;
        qqeu.qeu_lk_mode = DMT_IX; 
        qqeu.qeu_flag = QEU_BYPASS_PRIV;
        qqeu.qeu_access_mode = DMT_A_WRITE;
	qqeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qqeu);
	if(local_status==OK)
		qrytext_opened=TRUE;

	qqeu.qeu_getnext = QEU_REPO;
	qqeu.qeu_klen = 0;
	qqeu.qeu_qual = 0;
	qqeu.qeu_qarg = 0;
	qqeu.qeu_count = 1;
	qqeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);
	qqeu.qeu_output = &qef_data;
	qqef_data.dt_next = 0;
	qqef_data.dt_size = sizeof(DB_IIQRYTEXT);
	qqef_data.dt_data = (PTR)&qtuple;

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_IISECALARM_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_IISECALARM_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);

	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 0;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DB_SECALARM);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_SECALARM);
	qef_data.dt_data = (PTR)&dbalarm;

	while (status == E_DB_OK)
	{
	    /* Read the iisecalarm records that pertain to this database. */

	    status = qeu_get(qef_cb, &qeu);
	    if ((status != E_DB_OK) &&
		(qeu.error.err_code == E_QE0015_NO_MORE_ROWS))
	    {
		status = E_DB_OK;
		break;
	    }
	    else if ((status == E_DB_OK) && (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US2476_9334_IISECALARM_ERROR;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
		break;
	    }
	    if (dbalarm.dba_objtype==DBOB_DATABASE &&
		MEcmp( ( PTR ) &db_name,  ( PTR ) &dbalarm.dba_objname,
			DB_DB_MAXNAME) == 0)
	    {
		/* Delete query text */
		qkey_ptr_array[0]= &qkey_array[0];
		qkey_ptr_array[1]= &qkey_array[1];
		qqeu.qeu_getnext = QEU_REPO;
		qqeu.qeu_klen = 2;
		qqeu.qeu_key = qkey_ptr_array;
		qkey_ptr_array[0]->attr_number = DM_1_QRYTEXT_KEY;
		qkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
		qkey_ptr_array[0]->attr_value = 
			(char*) &dbalarm.dba_txtid.db_qry_high_time;

/* Bug #67531: Put info in correct array element */

		qkey_ptr_array[1]->attr_number = DM_2_QRYTEXT_KEY;
		qkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
		qkey_ptr_array[1]->attr_value = 
			(char*) &dbalarm.dba_txtid.db_qry_low_time;

		qqeu.qeu_qual=NULL;
		qqeu.qeu_qarg=NULL;
		err_code=0;
	        for (;;)		/* For all query text tuples */
	        {
		    status = qeu_get(qef_cb, &qqeu);
		    if (status != E_DB_OK)
		    {
		        err_code = qqeu.error.err_code;
		        break;
		    }
		    qqeu.qeu_klen = 0;
		    qqeu.qeu_getnext = QEU_NOREPO;
		    status = qeu_delete(qef_cb, &qqeu);
		    if (status != E_DB_OK)
		    {
		        err_code = qqeu.error.err_code;
		        break;
		    }
	        } /* For all query text tuples */
	        if (err_code != E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = err_code;
		    break;
		}

		/* Delete the alarm tuple */

		status = qeu_delete(qef_cb, &qeu);
		if ((status != E_DB_OK) || (qeu.qeu_count != 1))
		{
		    dsh->dsh_error.err_code = E_US2476_9334_IISECALARM_ERROR;
		    if (status < E_DB_ERROR)
			status = E_DB_ERROR;
		    break;
		}
	    }
	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	}
	if(qrytext_opened)
	{
	    (VOID) qeu_close(qef_cb, &qqeu);    
	    qrytext_opened=FALSE;
	}
	if (local_status == E_DB_OK)
	    local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US2476_9334_IISECALARM_ERROR;
	    status = local_status;
	}
    }

    /*	Delete the iidatabase tuple for this database. */

    if ( status == E_DB_OK )
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;
	qeu.qeu_key = &key_ptr;
	key.attr_number = DM_1_DATABASE_KEY;
	key.attr_operator = DMR_OP_EQ;
	key.attr_value = (char *)&db_name;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);

	while (status == E_DB_OK)
	{
	    /* Read the iidbpriv records that pertain to this database. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;

	    status = qeu_get(qef_cb, &qeu);
	    if ((status != E_DB_OK) &&
		(qeu.error.err_code == E_QE0015_NO_MORE_ROWS))
	    {
		status = E_DB_OK;
		break;
	    }
	    else if ((status == E_DB_OK) && (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
		break;
	    }

	    /* Delete the exsting tuple */

	    status = qeu_delete(qef_cb, &qeu);
	    if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
	    }
	    break;
	}
	if (local_status == E_DB_OK)
	    local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    status = local_status;
	}
    }

    /*	Call DMF to perform the operation. */

    if ( status == E_DB_OK )
    {
	dmm_cb.dmm_db_location.data_address = root_loc.du_area;
	dmm_cb.dmm_db_location.data_in_size = root_loc.du_l_area;
	status = dmf_call(DMM_DESTROY_DB, &dmm_cb);
	if (status != E_DB_OK)
	    dsh->dsh_error.err_code = dmm_cb.error.err_code;
	/* Must audit the success. */
	else if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&db_name,
			    &database.du_own,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX202A_DATABASE_DROP, SXF_A_SUCCESS | SXF_A_DROP,
			    &e_error);
			       
	    if (status != E_DB_OK)
		dsh->dsh_error.err_code = e_error.err_code;
	}
    }

    /* tear down the memory stream */
    if ( ulm->ulm_streamid )
	ulm_closestream( ulm );

    qef_rcb->qef_dbp_status = (i4) status;
    
    return (status);
}

/*{
** Name: QEA_4ALTER_DB	- implement iiQEF_alter_db Internal Procedure
**
** Description:
**      This INTERNAL Procedure is used to alter a database.
**	The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_alter_db(
**	    dbname=char(DB_DB_MAXNAME) not null not default,
**	    access_on = integer not null not default,
**	    access_off = integer not null not default,
**	    service_on = integer not null not default,
**	    service_off = integer not null not default,
**	    last_table_id = integer not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	and evoked by:
**	
**	EXECUTE PROCEDURE iiQEF_alter_db(
**	    dbname = :db_name,
**	    access = :access, service = :service, last_table_id = :last_id)
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for 
**					    parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	The database state is altered.
**
** History:
**      15-may-1989  (Derek)
**	    Created.
**	08-feb-1991 (ralph)
**	    Fold database name to lower case
**	10-Nov-1994 (ramra01)
**	    check to ensure if cmd line option is NOWAIT for destroydb, then
**	    destroydb only if not in use
**      15-dec-1994 (nanpr01)
**		check to ensure if cmd line option is NOWAIT for destroydb, then
**		destroydb only if not in use
**      06-mar-1996 (nanpr01)
**	    Fixed Bin's change. Parameter should be sizeof -1 rather than sizeof
**          BUG # 65937
[@history_template@]...
*/
DB_STATUS
qea_4alter_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    DU_DATABASE		database;
    DU_LOCATIONS	location;
    DB_DB_NAME		db_name;
    DMR_ATTR_ENTRY	key;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	*key_ptr = &key;
    QEU_CB		qeu;
    STATUS		local_status;
    DB_ERROR		error;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    int			access_on, access_off;
    int			service_on, service_off;
    char		tempstr[TEMPLEN];
    i4             err;

    GLOBALREF           QEF_S_CB	*Qef_s_cb;
    struct
    {
	i2		lname;		    /* length of ... */
	char		name[DB_LOC_MAXNAME];   /* A location name. */
    }			location_name;

    qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask = 0;

    /* 1st parameter = db_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /* 2nd parameter = access_on. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    access_on = *(i4 *)((char *)dsh->dsh_row[db_parm[1].dbp_rowno] +
	db_parm[1].dbp_offset);

    /* 3rd parameter = access_off. */

    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    access_off = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
	db_parm[2].dbp_offset);

    /* 4th parameter = service_on. */

    STRUCT_ASSIGN_MACRO(db_parm[3].dbp_dbv, row_dbv);
    service_on = *(i4 *)((char *)dsh->dsh_row[db_parm[3].dbp_rowno] +
	db_parm[3].dbp_offset);

    /* 5th parameter = service_off. */

    STRUCT_ASSIGN_MACRO(db_parm[4].dbp_dbv, row_dbv);
    service_off = *(i4 *)((char *)dsh->dsh_row[db_parm[4].dbp_rowno] +
	db_parm[4].dbp_offset);

    /* 6th parameter = last_table_id */

    STRUCT_ASSIGN_MACRO(db_parm[5].dbp_dbv, row_dbv);
    dmm_cb.dmm_tbl_id = *(i4 *)((char *)dsh->dsh_row[db_parm[5].dbp_rowno]
			  + db_parm[5].dbp_offset);

    {
	/*  Get record from iidatabase for this database. */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	while (status == E_DB_OK)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_DATABASE_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&dmm_cb.dmm_db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0010_NO_SUCH_DB;

       /* --- liibi01 for command like destroydb <dbname>            --- */
       /* --- Fix bug 60932, call the qef_error directly to pass the --- */
       /* --- <dbname> as one argument for error code E_US0010.      --- */

		    qef_error(dsh->dsh_error.err_code, 0L, status, &err,
			&dsh->dsh_error, 1, 
			sizeof(tempstr)-1, tempstr );
                    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
		}
		break;
	    }

	    /* Delete the existing tuple */

	    status = qeu_delete(qef_cb, &qeu);
	    if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
		break;
	    }

	    /*	Reinsert the tuple with the changes applied. */

	    if (access_on)
		database.du_access |= access_on;
	    if (access_off)
	    {
		if (access_off == DU_DESTROYDBNOWAIT)
		{
		    dmm_cb.dmm_flags_mask = DMM_NOWAIT;
		    access_off = DU_DESTROYDB;
		}
		database.du_access &= ~access_off;
	    }
	    if (service_on)
		database.du_dbservice |= service_on;
	    if (service_off)
		database.du_dbservice &= ~service_off;

	    dmm_cb.dmm_db_service = database.du_dbservice;
	    dmm_cb.dmm_db_access = database.du_access;
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_input = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    status = qeu_append(qef_cb, &qeu);
	    if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (local_status == E_DB_OK)
	    local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    qef_rcb->qef_dbp_status = (i4) local_status;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    qef_rcb->qef_dbp_status = (i4) status;
	    return (status);
	}
    }

    {
	if (MEcmp( ( PTR ) &database.du_own,  ( PTR ) &qef_cb->qef_user,
			sizeof(DB_OWN_NAME)) &&
	    (qef_cb->qef_ustat & DU_USECURITY) == 0)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		DB_ERROR	e_error;

		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&db_name,
			    &database.du_own,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX273A_ALTER_DATABASE, SXF_A_FAIL | SXF_A_CONTROL,
			    &e_error);
	    }
			   
	    dsh->dsh_error.err_code = E_US0003_NO_DB_PERM;
	    qef_rcb->qef_dbp_status = (i4) E_DB_ERROR;
	    return (E_DB_ERROR);
	}
    }

    /*	Translate the default data location for the database. */

    {
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_READ;
	MEcopy( ( PTR ) &database.du_dbloc, DB_LOC_MAXNAME,
		 ( PTR ) location_name.name);
	location_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{    
	    /*  Lookup the location. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_LOCATIONS);
	    qef_data.dt_data = (PTR)&location;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_LOCATIONS_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&location_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
	    qef_rcb->qef_dbp_status = (i4) local_status;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    qef_rcb->qef_dbp_status = (i4) status;
	    return (status);
	}
    }

    /*	Call DMF to perform the operation. */

    dmm_cb.dmm_db_location.data_address = location.du_area;
    dmm_cb.dmm_db_location.data_in_size = location.du_l_area;
    dmm_cb.dmm_alter_op = DMM_TAS_ALTER;
    
    status = dmf_call(DMM_ALTER_DB, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
	qef_rcb->qef_dbp_status = (i4) status;
        return (status);
     }

    qef_rcb -> qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: QEA_5ADD_LOCATION - implement iiQEF_add_location Internal Procedure
**
** Description:
**      This INTERNAL Procedure is used to extend a database to a new 
**	location.
**
**	The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_add_location(
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_name = char(DB_LOC_MAXNAME) not null not default,
**	    access = integer not null not default,
**	    need_dbdir_flg = integer not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	and invoked by:
**	
**	EXECUTE PROCEDURE iiQEF_add_location(
**	    location_name = :loc_name, database_name = :db_name,
**	    access = :access, need_dbdir_flg = :flags)
**
**	database_name is the name of the database which you wish to extend;
**	location_name is the name of the location to which you wish to extend
**	the database; access is the type of the location (e.g. DATA or WORK);
**	need_dbdir_flag is unused.
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	A database is created in the specified area owned by the caller.
**
** History:
**      15-may-1989  (Derek)
**	    Created.
**	08-feb-1991 (ralph)
**	    Fold database name, location name to lower case
**	25-apr-1991 (teresa)
**	    add need_dbdir_flg parameter so convto60 may also use this
**	    mechanism.
**	03-nov-92 (jrb)
**	    Multi-Location Sorts.  Set iiextend's status field to indicate the
**	    type of location being extended.
**	14-dec-92 (jrb)
**	    Removed "need_dbdir_flag" since convto60 won't be used for 6.5 and
**	    therefore won't need to use this code.
**      21-nov-1994 (juliet)
**          Added a check to ensure that the database is not being extended
**          to the same location for the same usage as previously extended
**          to.
**	18-jan-1994 (cohmi01)
**	    Special handling if called by mkrawloc utility, as indicated
**	    by resurrected 'need_dir' flag. if caller is mkrawloc, turn 
**	    on raw bit in iiextend etc. If not, and extending db to a
**	    raw location, issue error.
**	2-nov-1995 (angusm)
**		Location names should not be forced lowercase. 
**		bug 71173.
**	12-Mar-2001 (jenjo02)
**	    Productize RAW locations, removed mkrawloc special handling.
**	02-Apr-2001 (jenjo02)
**	    DMF returns updated raw_start, raw_blocks in DMM_LOC_LIST;
**	    Store new iiextend tuple with raw_start value -after- 
**	    call to DMM_ADD_LOC instead of before.
**	    Added verification that raw location's area has not been
**	    extended to any other database.
**	10-May-2001 (jenjo02)
**	    Extends to raw locations were not being flagged as RAW
**	    if rawstart was zero.
**	11-May-2001 (jenjo02)
**	    Raw size request changed to percent of raw area instead
**	    of absolute number of blocks.
**	29-Jul-2005 (thaju02)
**	    Location cannot be used for both work and auxiliary work.
**	    (B114574)
**      28-Jan-2008 (hanal04) Bug 119760
**          Change that prevented location from being used for work and awork
**          blocked previously allowed work, work combination.
**	   
[@history_template@]...
*/
DB_STATUS
qea_5add_location(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    QEF_CB              *qef_cb;
    DB_STATUS		status  = E_DB_OK;
    DB_STATUS		local_status;
    DB_ERROR		e_error;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    ER_ARGUMENT         er_args[1];
    DB_ERROR		error;
    i4		loc_type = 0;
    i4		local_error;
    struct
    {
	i2		lname;		    /* length of ... */
	char		name[DB_LOC_MAXNAME];   /* A location name. */
    }			d_name, loc_name, xloc_name, wloc_name;
    DU_LOCATIONS	location, xlocation, wlocation;
    DU_LOCATIONS	db_location;
    DU_EXTEND		dbextend;
    DU_DATABASE		database;
    DMM_LOC_LIST	loc_list[1];
    PTR			loc_ptr[1];
    QEU_CB		qeu, xlqeu, wlqeu;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	key;
    DMR_ATTR_ENTRY	*key_ptr = &key;
    char		tempstr[TEMPLEN];
    int			need_dbdir_flg;
    i4		du_extend_type;
    GLOBALREF           QEF_S_CB	*Qef_s_cb;
    SXF_RCB	    	sxfrcb;
    i4			qeu_opened, xlqeu_opened, wlqeu_opened;

    qef_cb = dsh->dsh_qefcb;
    qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    /*
    ** Tell DMF we're extending to db.
    **
    ** By now the Location's Area path must exist.
    */
    dmm_cb.dmm_flags_mask = DMM_EXTEND_LOC_AREA;
    dmm_cb.dmm_db_id = TMsecs();

    /*	1st parameter = database_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /*	2nd parameter = location_area. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';

/*
**    CVlower(tempstr);

	Location-names are treated as for other objects
	as regards case. Previous line is commented out
	as an awful warning against the temptation to
	force lowercase. Bug 70947.
*/
	MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_location_name), 
	(char *)&dmm_cb.dmm_location_name);

    /* 3rd parameter = location type. */

    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    loc_type = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
	db_parm[2].dbp_offset);

    switch (loc_type)
    {
    case DU_DBS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_DATA;
	du_extend_type = DU_EXT_DATA;
	break;
    case DU_JNLS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_JNL;
	du_extend_type = 0;
	break;
    case DU_CKPS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_CKP;
	du_extend_type = 0;
	break;
    case DU_DMPS_LOC:
    	dmm_cb.dmm_loc_type = DMM_L_DMP;
	du_extend_type = 0;
	break;
    case DU_WORK_LOC:
	dmm_cb.dmm_loc_type = DMM_L_WRK;
	du_extend_type = DU_EXT_WORK;
	break;
    case DU_AWORK_LOC:
	/* we allow loc_type to be DU_AWORK_LOC (auxiliary work location) even
	** though this is not a valid location type, we allow it as a parameter
	** to iiQEF_add_location so that db's can be directly extended to work
	** locations in auxiliary mode.
	*/
	loc_type = DU_WORK_LOC;
	dmm_cb.dmm_loc_type = DMM_L_AWRK;
	du_extend_type = DU_EXT_AWORK;
	break;
    default:
	dmm_cb.dmm_loc_type = 0;
	du_extend_type = 0;
	loc_type = 0;
	break;
    }


    {
	/*  Get record from iidatabase for this database. */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_DATABASE_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&dmm_cb.dmm_db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0010_NO_SUCH_DB;
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
    }

    /*	Translate the new location for the database. */

    {
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_READ;
	MEcopy( ( PTR ) &dmm_cb.dmm_location_name, DB_LOC_MAXNAME,
		 ( PTR ) loc_name.name);
	loc_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	while (status == E_DB_OK)
	{    
	    /*  Lookup the location for the database. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_LOCATIONS);
	    qef_data.dt_data = (PTR)&location;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_LOCATIONS_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&loc_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0080_LOCATION_NOT_FOUND;
		}
	    }
	    else  
	    {
		if ((loc_type & location.du_status) == 0)
		{
		    dsh->dsh_error.err_code = E_US0081_LOCATION_NOT_ALLOWED;
		    status = E_DB_ERROR;
		}
		else if(status==E_DB_OK)
		{
		    /*  Save location information for DMF. */

		    loc_list[0].loc_type = dmm_cb.dmm_loc_type;
		    loc_list[0].loc_l_area = location.du_l_area;
		    MEcopy( ( PTR ) &dmm_cb.dmm_location_name,
			    DB_LOC_MAXNAME,
			     ( PTR ) &loc_list[0].loc_name);
		    MEcopy( ( PTR ) location.du_area,
				location.du_l_area,
				 ( PTR ) loc_list[0].loc_area);
		    
		    loc_list[0].loc_raw_pct = location.du_raw_pct;

		    /*
		    ** Now get the database base location (needed by DMF)
		    */

		    MEcopy( ( PTR ) &database.du_dbloc, DB_LOC_MAXNAME,
				 ( PTR ) loc_name.name);
		    loc_name.lname = DB_LOC_MAXNAME;
		    qeu.qeu_count = 1;
		    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
		    qeu.qeu_output = &qef_data;
		    qef_data.dt_next = 0;
		    qef_data.dt_size = sizeof(DU_LOCATIONS);
		    qef_data.dt_data = (PTR)&db_location;
		    qeu.qeu_getnext = QEU_REPO;
		    qeu.qeu_klen = 1;
		    qeu.qeu_key = &key_ptr;
		    key.attr_number = DM_1_LOCATIONS_KEY;
		    key.attr_operator = DMR_OP_EQ;
		    key.attr_value = (char *)&loc_name;
		    qeu.qeu_qual = 0;
		    qeu.qeu_qarg = 0;
		    status = qeu_get(qef_cb, &qeu);
		    if (status != E_DB_OK)
		    {
			status = E_DB_ERROR;
			if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
			{
			    dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
			}
		    }
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	    break;
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /* Check if database is already extended. */

    {
	xlqeu_opened = qeu_opened = wlqeu_opened = FALSE;

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_count = 1;
	qeu.qeu_mask = 0;
	qeu.qeu_getnext = QEU_REPO;

	/* RAW locations can only be extended once and to a single database */
	if ( location.du_raw_pct )
	{
	    /* Must read -all- iiextend rows */
	    qeu.qeu_klen = 0;

	    /* We'll need iiextend->iilocation for area cross-check */
	    xlqeu.qeu_type = QEUCB_CB;
	    xlqeu.qeu_length = sizeof(QEUCB_CB);
	    xlqeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
	    xlqeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
	    xlqeu.qeu_db_id = qef_rcb->qef_db_id;
	    xlqeu.qeu_lk_mode = DMT_IS;
	    xlqeu.qeu_flag = 0;
	    xlqeu.qeu_access_mode = DMT_A_READ;
	    xlqeu.qeu_mask = 0;
	    if ( (local_status = status = qeu_open(qef_cb, &xlqeu)) == E_DB_OK )
		xlqeu_opened = TRUE;
	}
	else
	{
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_EXTEND_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&d_name;
	    MEcopy( ( PTR ) &dmm_cb.dmm_db_name, DB_DB_MAXNAME,
		     ( PTR ) d_name.name);
	    d_name.lname = DB_LOC_MAXNAME;
	}

	if ( local_status == E_DB_OK  &&
	    (local_status = status = qeu_open(qef_cb, &qeu)) == E_DB_OK )
	{
	    qeu_opened = TRUE;
	}

	while (status == E_DB_OK)
	{    
	    /*  Lookup the iiextend. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_EXTEND);
	    qef_data.dt_data = (PTR)&dbextend;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    status = E_DB_OK;
		    break;
		}
	    }

	    /* Already extended to (a) database? */
	    if ((MEcmp( ( PTR ) dbextend.du_lname,
		 ( PTR ) &dmm_cb.dmm_location_name,
		  dbextend.du_l_length) == 0) &&
		(dbextend.du_status & du_extend_type))
	    {
		if ( (MEcmp( (PTR)&dmm_cb.dmm_db_name,
		       (PTR)&dbextend.du_dname,
		       DB_DB_MAXNAME)) == 0 )
		    /* Already extended to -this- database */
		    dsh->dsh_error.err_code = E_US0087_EXTEND_ALREADY;
		else
		    /* Raw, already extended to -another- database */
		    dsh->dsh_error.err_code = E_US0089_RAW_LOC_IN_USE;
		status = E_DB_ERROR;
		break;
	    }

	    /* location can not be specified for work and aux work */
	    if (((du_extend_type & DU_EXT_AWORK) &&
		 (dbextend.du_status & DU_EXT_WORK)) ||
                ((du_extend_type & DU_EXT_WORK) &&
                 (dbextend.du_status & DU_EXT_AWORK)))
	    {
		if (MEcmp((PTR)dbextend.du_lname,
                    (PTR)&dmm_cb.dmm_location_name,
                    dbextend.du_l_length) == 0) 
		{
		    /* same location name */
		    dsh->dsh_error.err_code = E_US008C_LOC_WORK_AND_AWORK;
		    status = E_DB_ERROR;
		    break;
		}
		else /* same area? */
		{
		    if (!wlqeu_opened)
		    {
			wlqeu.qeu_type = QEUCB_CB;
			wlqeu.qeu_length = sizeof(QEUCB_CB);
			wlqeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
			wlqeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
			wlqeu.qeu_db_id = qef_rcb->qef_db_id;
			wlqeu.qeu_lk_mode = DMT_IS;
			wlqeu.qeu_flag = 0;
			wlqeu.qeu_access_mode = DMT_A_READ;
			wlqeu.qeu_mask = 0;
			if ( (status = qeu_open(qef_cb, &wlqeu)) == E_DB_OK )
			    wlqeu_opened = TRUE;
			else
			    break;
		    }

		    wlqeu.qeu_count = 1;
		    wlqeu.qeu_tup_length = sizeof(DU_LOCATIONS);
		    wlqeu.qeu_output = &qef_data;
		    qef_data.dt_next = 0;
		    qef_data.dt_size = sizeof(DU_LOCATIONS);
		    qef_data.dt_data = (PTR)&wlocation;
		    wlqeu.qeu_getnext = QEU_REPO;
		    wlqeu.qeu_klen = 1;
		    wlqeu.qeu_key = &key_ptr;
		    key.attr_number = DM_1_LOCATIONS_KEY;
		    key.attr_operator = DMR_OP_EQ;
		    key.attr_value = (char *)&wloc_name;
		    MEcopy( ( PTR ) &dbextend.du_lname, 
			    dbextend.du_l_length,
			    ( PTR ) wloc_name.name);
		    wloc_name.lname = dbextend.du_l_length;
		    wlqeu.qeu_qual = 0;
		    wlqeu.qeu_qarg = 0;
		    status = qeu_get(qef_cb, &wlqeu);
		    if (status != E_DB_OK)
		    {
			status = E_DB_ERROR;
			if (wlqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
			    dsh->dsh_error.err_code = E_US0080_LOCATION_NOT_FOUND;
			break;
		    }
		    else if ( (MEcmp((PTR)&location.du_area,
			       (PTR)&wlocation.du_area,
			       DB_AREA_MAX)) == 0 )
		    {
			status = E_DB_ERROR;
			dsh->dsh_error.err_code = E_US008D_LOC_AREA_WORK_AND_AWORK;
			break;
		    }
		}
	    }

	    /*
	    ** If raw extend to another database, check that the
	    ** location's area is not the same. A raw area can
	    ** only be associated with a single database.
	    */

	    if ( dbextend.du_status & DU_EXT_RAW && 
		 xlqeu_opened &&
		 MEcmp( (PTR)&dmm_cb.dmm_db_name,
		       (PTR)&dbextend.du_dname, DB_DB_MAXNAME) )
	    {
		xlqeu.qeu_count = 1;
		xlqeu.qeu_tup_length = sizeof(DU_LOCATIONS);
		xlqeu.qeu_output = &qef_data;
		qef_data.dt_next = 0;
		qef_data.dt_size = sizeof(DU_LOCATIONS);
		qef_data.dt_data = (PTR)&xlocation;
		xlqeu.qeu_getnext = QEU_REPO;
		xlqeu.qeu_klen = 1;
		xlqeu.qeu_key = &key_ptr;
		key.attr_number = DM_1_LOCATIONS_KEY;
		key.attr_operator = DMR_OP_EQ;
		key.attr_value = (char *)&xloc_name;
		MEcopy( ( PTR ) &dbextend.du_lname, 
			dbextend.du_l_length,
			( PTR ) xloc_name.name);
		xloc_name.lname = dbextend.du_l_length;
		xlqeu.qeu_qual = 0;
		xlqeu.qeu_qarg = 0;
		status = qeu_get(qef_cb, &xlqeu);
		if (status != E_DB_OK)
		{
		    status = E_DB_ERROR;
		    if (xlqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
			dsh->dsh_error.err_code = E_US0080_LOCATION_NOT_FOUND;
		}
		else if ( (MEcmp((PTR)&location.du_area,
			   (PTR)&xlocation.du_area,
			   location.du_l_area)) == 0 )
		{
		    status = E_DB_ERROR;
		    dsh->dsh_error.err_code = E_US0088_RAW_AREA_IN_USE;
		}
	    }

	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	}
	if ( xlqeu_opened )
	    qeu_close(qef_cb, &xlqeu);    
	if (wlqeu_opened)
	    qeu_close(qef_cb, &wlqeu);
	if (status != E_DB_OK)
	{
	    if ( qeu_opened )
		qeu_close(qef_cb, &qeu);    
	    return (status);
	}
    }

    /*	Call DMF to perform the operation. */

    dmm_cb.dmm_db_location.data_address = db_location.du_area;
    dmm_cb.dmm_db_location.data_in_size = db_location.du_l_area;
    dmm_cb.dmm_loc_list.ptr_address = (PTR)loc_ptr;
    dmm_cb.dmm_loc_list.ptr_in_count = 1;
    dmm_cb.dmm_loc_list.ptr_size = sizeof(DMM_LOC_LIST);
    loc_ptr[0] = (PTR)&loc_list[0];

    status = dmf_call(DMM_ADD_LOC, &dmm_cb);
    if (status != E_DB_OK)
    {
	if (dmm_cb.error.err_code == E_DM0124_DB_EXISTS ||
	    dmm_cb.error.err_code == E_DM007E_LOCATION_EXISTS)
	{
	    /*
	    **	Race condition can have the location created but not
	    **	registered in DBDB.  Return success so that the
	    **	location is re-registered.
	    **  Note that this can also happen if the DB is
	    **  rolled forward without journals after being
	    **  unextended:
	    **		o extend db to loc1
	    **		o ckpdb
	    **		o unextenddb loc1 (deletes IIEXTEND)
	    **		o rollforwarddb -j 
	    **		    restores config to include loc1 but
	    **		    does not recover IIEXTEND.
	    */
	    status = E_DB_OK;
	}
	else
	{
	    qeu_close(qef_cb, &qeu);    
	    dsh->dsh_error.err_code = dmm_cb.error.err_code;
	    return (status);
	}
    }

    /* Now add an IIEXTEND tuple to dbdb */

    /* DMF has augmented loc_type with DMM_L_RAW if raw area */
    
    dbextend.du_l_length = DB_LOC_MAXNAME;
    dbextend.du_d_length = DB_DB_MAXNAME;
    MEcopy( ( PTR ) &dmm_cb.dmm_db_name, DB_DB_MAXNAME,
	 ( PTR ) dbextend.du_dname);
    MEcopy( ( PTR ) &dmm_cb.dmm_location_name, DB_LOC_MAXNAME,
	 ( PTR ) dbextend.du_lname);
    dbextend.du_status = DU_EXT_OPERATIVE | du_extend_type;

    /* Include raw_start, raw_blocks returned by DMF, zero if not raw */
    dbextend.du_raw_start  = loc_list[0].loc_raw_start;
    dbextend.du_raw_blocks = loc_list[0].loc_raw_blocks;

    if ( loc_list[0].loc_type & DMM_L_RAW )
	dbextend.du_status |= DU_EXT_RAW;

    qeu.qeu_count = 1;
    qeu.qeu_tup_length = sizeof(DU_EXTEND);
    qeu.qeu_input = &qef_data;
    qef_data.dt_next = 0;
    qef_data.dt_size = sizeof(DU_EXTEND);
    qef_data.dt_data = (PTR)&dbextend;
    status = qeu_append(qef_cb, &qeu);
    if ((status != E_DB_OK) || (qeu.qeu_count != 1))
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	status = E_DB_ERROR;
    }
    if ( local_status = qeu_close(qef_cb, &qeu) )
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	return (local_status);
    }
    if (status != E_DB_OK)
    {
	return (status);
    }

    /* Must audit that location was added. */

    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&dmm_cb.dmm_db_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(DB_DB_NAME), SXF_E_DATABASE,
			    I_SX201F_DATABASE_EXTEND, SXF_A_SUCCESS | SXF_A_EXTEND,
			    &e_error);

	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = e_error.err_code;
	    return (status);
	}
    }
    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}


/*{
** Name: QEA_5DEL_LOCATION - implement iiQEF_del_location Internal Procedure
**
** Description:
**      This INTERNAL Procedure is used to unextend a database from location. 
**
**	The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_del_location(
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_name = char(DB_LOC_MAXNAME) not null not default,
**	    access = integer not null not default,
**	    need_dbdir_flg = integer not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	and invoked by:
**	
**	EXECUTE PROCEDURE iiQEF_del_location(
**	    location_name = :loc_name, database_name = :db_name,
**	    access = :access, need_dbdir_flg = :flags)
**
**	database_name is the name of the database which you wish to unextend;
**	location_name is the name of the location to which you wish to unextend
**	the database from; access is the type of the location (e.g. DATA or WORK);
**	need_dbdir_flag is unused.
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
**
** History:
**      29-Apr-2004  (gorvi01)
**	    Created for Jupiter.
**	11-Nov-2004 (gorvi01)
**	    Modified functionality to fix BUG#113362. Unextending
**	    after unextenddb already done now correctly checks the
**	    iiextend and throws proper error message. 	  
**	20-Dec-2004 (jenjo02)
**	    Error code must be returned (consistently) into 
**	    dsh_error, not sometimes into qef_rcb->error.
**	    Cleaned up unreadable code, refined handling of
**	    errors to return E_US008A, E_US008B.
**	    Forgive not finding IIEXTEND and go on to check
**	    config file for extension before giving "not extended"
**	    error.
*/

DB_STATUS 
qea_5del_location(
    QEF_RCB	*qef_rcb,
    QEF_DBP_PARAM *db_parm,
    QEE_DSH *dsh )
{
    QEF_CB              *qef_cb;
    DB_STATUS		status  = E_DB_OK;
    DB_STATUS		local_status;
    DB_ERROR		e_error;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;    
    ER_ARGUMENT         er_args[1];
    DB_ERROR		error;
    i4			loc_type = 0;
    i4			local_error;
    struct
    {
	i2		lname;		    /* length of ... */
	char		name[DB_LOC_MAXNAME];   /* A location name. */
    }			d_name, loc_name, xloc_name;
    DU_LOCATIONS	location, xlocation;
    DU_LOCATIONS	db_location;
    DU_EXTEND		dbextend;
    DU_DATABASE		database;
    DMM_LOC_LIST	loc_list[1];
    PTR			loc_ptr[1];
    QEU_CB		qeu, xlqeu;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	key;
    DMR_ATTR_ENTRY	*key_ptr = &key;
    char		tempstr[DB_MAXNAME+1];
    int			need_dbdir_flg;
    i4			du_extend_type;
    GLOBALREF           QEF_S_CB	*Qef_s_cb;
    SXF_RCB	    	sxfrcb;
    i4			qeu_opened, xlqeu_opened;

    qef_cb = dsh->dsh_qefcb;
    qea_mk_dmmcb(dsh, &dmm_cb);
    dsh->dsh_error.err_code = 0;

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    /*
    ** Tell DMF we're unextending from db.
    **
    ** By now the Location's Area path must exist.
    */
    dmm_cb.dmm_flags_mask = DMM_EXTEND_LOC_AREA;
    dmm_cb.dmm_db_id = TMsecs();

    /*	1st parameter = database_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /*	2nd parameter = location_area. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';

/*
**    CVlower(tempstr);

	Location-names are treated as for other objects
	as regards case. Previous line is commented out
	as an awful warning against the temptation to
	force lowercase. Bug 70947.
*/
	MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_location_name), 
	(char *)&dmm_cb.dmm_location_name);

    /* 3rd parameter = location type. */

    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    loc_type = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
	db_parm[2].dbp_offset);

    switch (loc_type)
    {
    case DU_DBS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_DATA;
	du_extend_type = DU_EXT_DATA;
	break;
    case DU_JNLS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_JNL;
	du_extend_type = 0;
	break;
    case DU_CKPS_LOC:
	dmm_cb.dmm_loc_type = DMM_L_CKP;
	du_extend_type = 0;
	break;
    case DU_DMPS_LOC:
    	dmm_cb.dmm_loc_type = DMM_L_DMP;
	du_extend_type = 0;
	break;
    case DU_WORK_LOC:
	dmm_cb.dmm_loc_type = DMM_L_WRK;
	du_extend_type = DU_EXT_WORK;
	break;
    case DU_AWORK_LOC:
	/* we allow loc_type to be DU_AWORK_LOC (auxiliary work location) even
	** though this is not a valid location type, we allow it as a parameter
	** to iiQEF_add_location so that db's can be directly extended to work
	** locations in auxiliary mode.
	*/
	loc_type = DU_WORK_LOC;
	dmm_cb.dmm_loc_type = DMM_L_AWRK;
	du_extend_type = DU_EXT_AWORK;
	break;
    default:
	dmm_cb.dmm_loc_type = 0;
	du_extend_type = 0;
	loc_type = 0;
	break;
    }


    {
	/*  Get record from iidatabase for this database. */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
	qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
	qeu.qeu_db_id = qef_rcb->qef_db_id;
	qeu.qeu_lk_mode = DMT_IX;
	qeu.qeu_flag = 0;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_DATABASE_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&dmm_cb.dmm_db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0010_NO_SUCH_DB;
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
    }

    /*	Translate the new location for the database. */

    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
	qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
	qeu.qeu_db_id = qef_rcb->qef_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_flag = 0;
	qeu.qeu_access_mode = DMT_A_READ;
	MEcopy( ( PTR ) &dmm_cb.dmm_location_name, DB_LOC_MAXNAME,
		 ( PTR ) loc_name.name);
	loc_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	while (status == E_DB_OK)
	{    
	    /*  Lookup the location for the database. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_LOCATIONS);
	    qef_data.dt_data = (PTR)&location;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_LOCATIONS_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&loc_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0080_LOCATION_NOT_FOUND;
		}
	    }
	    else  
	    {
		if ((loc_type & location.du_status) == 0)
		{
		    dsh->dsh_error.err_code = E_US0081_LOCATION_NOT_ALLOWED;
		    status = E_DB_ERROR;
		}
		else if (status==E_DB_OK)
		{
		    /*  Save location information for DMF. */

		    loc_list[0].loc_type = dmm_cb.dmm_loc_type;
		    loc_list[0].loc_l_area = location.du_l_area;
		    MEcopy( ( PTR ) &dmm_cb.dmm_location_name,
			    DB_LOC_MAXNAME,
			    ( PTR ) &loc_list[0].loc_name);
		    MEcopy( ( PTR ) location.du_area,
			    location.du_l_area,
			    ( PTR ) loc_list[0].loc_area);
		
		    loc_list[0].loc_raw_pct = location.du_raw_pct;

		    /*
		    ** Now get the database base location (needed by DMF)
		    */

		    MEcopy( ( PTR ) &database.du_dbloc, DB_LOC_MAXNAME,
			    ( PTR ) loc_name.name);
		    loc_name.lname = DB_LOC_MAXNAME;
		    qeu.qeu_count = 1;
		    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
		    qeu.qeu_output = &qef_data;
		    qef_data.dt_next = 0;
		    qef_data.dt_size = sizeof(DU_LOCATIONS);
		    qef_data.dt_data = (PTR)&db_location;
		    qeu.qeu_getnext = QEU_REPO;
		    qeu.qeu_klen = 1;
		    qeu.qeu_key = &key_ptr;
		    key.attr_number = DM_1_LOCATIONS_KEY;
		    key.attr_operator = DMR_OP_EQ;
		    key.attr_value = (char *)&loc_name;
		    qeu.qeu_qual = 0;
		    qeu.qeu_qarg = 0;
		    status = qeu_get(qef_cb, &qeu);
		    if (status != E_DB_OK)
		    {
			status = E_DB_ERROR;
			if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
			{
			    dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
			}
		    }
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	    break;
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /* Check if database is already extended. */

    {	
	xlqeu_opened = qeu_opened = FALSE;
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
        qeu.qeu_db_id = qef_rcb->qef_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_count = 1;
	qeu.qeu_mask = 0;
	qeu.qeu_getnext = QEU_REPO;

	if ( location.du_raw_pct )
	{
	    /* Must read -all- iiextend rows */
	    qeu.qeu_klen = 0;

	    /* We'll need iiextend->iilocation for area cross-check */
	    xlqeu.qeu_type = QEUCB_CB;
	    xlqeu.qeu_length = sizeof(QEUCB_CB);
	    xlqeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
	    xlqeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
	    xlqeu.qeu_db_id = qef_rcb->qef_db_id;
	    xlqeu.qeu_lk_mode = DMT_IS;
	    xlqeu.qeu_flag = 0;
	    xlqeu.qeu_access_mode = DMT_A_READ;
	    xlqeu.qeu_mask = 0;
	    if ( (local_status = status = qeu_open(qef_cb, &xlqeu)) == E_DB_OK )
		    xlqeu_opened = TRUE;
	}
	else
	{
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_EXTEND_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&d_name;
	    MEcopy( ( PTR ) &dmm_cb.dmm_db_name, DB_DB_MAXNAME,
		     ( PTR ) d_name.name);
	    d_name.lname = DB_DB_MAXNAME;
	}

	if ( local_status == E_DB_OK  &&
		(local_status = status = qeu_open(qef_cb, &qeu)) == E_DB_OK )
	{
	    qeu_opened = TRUE;
	}

	while (status == E_DB_OK)
	{    
	    /*  Lookup the iiextend. */

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_EXTEND);
	    qef_data.dt_data = (PTR)&dbextend;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /* db not extended to this loc -- check config */
		    dsh->dsh_error.err_code = E_US008A_NOT_EXTENDED;
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    /* some qef error */
		    dsh->dsh_error.err_code = E_US001F_IIEXTEND_ERROR;
		    break;
		}

	    }

	    /* Already extended to (a) database? */
	    if ((MEcmp( ( PTR ) dbextend.du_lname,
		    ( PTR ) &dmm_cb.dmm_location_name,
		    dbextend.du_l_length) == 0) &&
		    (dbextend.du_status & du_extend_type))
	    {
		if ( (MEcmp( (PTR)&dmm_cb.dmm_db_name,
			(PTR)&dbextend.du_dname, DB_DB_MAXNAME)) == 0 )
		{
		    /* Already extended to -this- database */
		    status = E_DB_OK;
		}
		else
		{
		    /* Raw, already extended to -another- database */
		    dsh->dsh_error.err_code = E_US0089_RAW_LOC_IN_USE;
		    status = E_DB_ERROR;
		}
		break;
	    }

	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	}
	if ( xlqeu_opened )
	    qeu_close(qef_cb, &xlqeu);    
	if (status != E_DB_OK)
	{
	    if ( qeu_opened )
		qeu_close(qef_cb, &qeu);    
	    return (status);
	}
    }

    /*	Call DMF to perform the operation. */

    dmm_cb.dmm_db_location.data_address = db_location.du_area;
    dmm_cb.dmm_db_location.data_in_size = db_location.du_l_area;
    dmm_cb.dmm_loc_list.ptr_address = (PTR)loc_ptr;
    dmm_cb.dmm_loc_list.ptr_in_count = 1;
    dmm_cb.dmm_loc_list.ptr_size = sizeof(DMM_LOC_LIST);
    loc_ptr[0] = (PTR)&loc_list[0];

    status = dmf_call(DMM_DEL_LOC, &dmm_cb);
    if (status != E_DB_OK)
    {
	if ( dmm_cb.error.err_code == E_DM013D_LOC_NOT_EXTENDED )
	{
	    /*
	    ** Config not extended, but if IIEXTEND exists,
	    ** fall through to delete it.
	    */
	    if ( dsh->dsh_error.err_code == 0 )
		status = E_DB_OK;
	}
	else 
	{
	    if ( dmm_cb.error.err_code == E_DM019E_ERROR_FILE_EXIST )
		dsh->dsh_error.err_code = E_US008B_LOCATION_NOT_EMPTY;
	    else
		dsh->dsh_error.err_code = dmm_cb.error.err_code;
	}

	if ( status )
	{
	    qeu_close(qef_cb, &qeu);    
	    return (status);
	}
    }

    /* Now delete the IIEXTEND tuple from dbdb */

    qeu.qeu_getnext = QEU_NOREPO;
    qeu.qeu_klen = 0;
    status = qeu_delete(qef_cb, &qeu);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	status = E_DB_ERROR;
    }

    if ( local_status = qeu_close(qef_cb, &qeu) )
    {
	dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	return (local_status);
    }
    if (status != E_DB_OK)
    {
	return (status);
    }

    /* Must audit that location was deleted. */

    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
				(char *)&dmm_cb.dmm_db_name,
				(DB_OWN_NAME *)NULL,
				sizeof(DB_DB_NAME), SXF_E_DATABASE,
				I_SX201F_DATABASE_UNEXTEND, SXF_A_SUCCESS | SXF_A_UNEXTEND,
				&e_error);

	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = e_error.err_code;
	    return (status);
	}
    }
    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}


/*{
** Name: qea_67_check_patch	- Implement iiQEF_check_table and
**					    iiQEF_patch_table
**						    Internal Procedures
**
** Description:
**      This internal procedure is to execute the Check Table or Patch Table
**	operations.  (Those operations permit DMF to examine (or patch) the
**	file that holds a user's table data.  It checks the integrity of data
**	on a page and the integrity of indexs and page chains).
**
**	The procedure to evoke the Check Table operation is:
**
**	CREATE PROCEDURE iiQEF_check_table (
**		table_id i4 not null not default,
**		index_id i4 not null not default,
**		verbose = char(10) not null not default)
**      AS begin
**            execute internal;
**         end;
**         
**      and evoked by:
**
**      EXECUTE PROCEDURE iiQEF_check_table (table_id = :iirelation.reltid,
**                                           index_id = :iirelation.reltidx,
**					     verbose =  :verbose_flag)
**	    where verbose_flag is either "verbose" or "noverbose"
**
**	The procedure to evoke the Patch Table operation is:
**
**      CREATE PROCEDURE iiQEF_patch_table (
**		table_id i4 not null not default,
**              mode char(8) not null not default,
**		verbose = char(10) not null not default)
**      AS begin
**            execute internal;
**         end;                                    
**
**
**      EXECUTE PROCEDURE iiQEF_patch_table (table_id = :iirelation.reltid,
**                                           mode = 'force',
**					     verbose =  :verbose_flag)
**	    where verbose_flag is either "verbose" or "noverbose"
**
**      or
**
**      EXECUTE PROCEDURE iiQEF_patch_table (table_id = :iirelation.reltid,
**                                           mode = 'save_data',
**					     verbose =  :verbose_flag)
**	    where verbose_flag is either "verbose" or "noverbose"
**
**      In either case,  table_id is the base table id, snf index_id is the
**	tables's index id (zero unless the table is a secondary index).
**	The mode parameter can have two values: force (use strict algorithm) or
**	save_data (use forgiving algorithm.)
**
**	This routine creates an "empty" dmu_cb.  Then it fills in some
**	of the dmucb items from user parameters:
**
**	    dmu_flags_mask  =	DMU_CHECK or (DMU_PATCH and perhaps DMU_FORCE)
**	    dmu_tab_id	    =	table_id, 0 for PATCH
**	or
**	    dmu_tab_id	    =	table_id, index_id for check.
**
**
**	If dmf is unsuccessful, then this routine will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    Several messages will be sent to the FE from DMF and will bypass
**	    QEF.  These messages are warning or informative in nature.  Any
**	    dmf error will cause dmf to return to QEF using standard error
**	    reporting mechanisms.
**
** History:
**      10-Aug-89  (teg)
**          Initical creation.
**	07-nov-89  (teg)
**	    added 2nd parameter to check_table to permit checking of index
**	    tables.
**	23-oct-92 (teresa)
**	    Implemented VERBOSE support for sir 42498.
**      30-Dec-92 (jhahn)
**          Fixed dsh_row casting.
**	26-apr-1993 (rmuth)
**	    Pass the BITMAP option to verify routines.
**	27-apr-99 (stephenb)
**	    Add DMU_T_PERIPHERAL attribute for both check and patch operations
[@history_template@]...
*/
DB_STATUS
qea_67_check_patch(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh,
i4			patch_flag )
{

    DB_STATUS		status  = E_DB_OK;
    DMU_CB		dmu_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;
    u_i4		*table_id;
    u_i4		*index_id;
    char		*mode;
    char		*verbose;
    DMU_KEY_ENTRY	key_entry[1];
    DMU_KEY_ENTRY	*key;
    DMU_CHAR_ENTRY	char_entry[2];

    /* 
    **  set up initialized DMM_CB 
    */
    key = &key_entry[0];
    qea_make_dmucb(dsh, &dmu_cb, &key, &char_entry[0] );
    dmu_cb.dmu_char_array.data_in_size = sizeof(char_entry);

    /*  fill in list specific parameters from procedure arguments:
    **		dmu_flags_mask	=   DMU_PATCH (if patch_flag=TRUE),
    **				    DMU_CHECK (if pathc_flag=FALSE)
    **				    DMU_PATCH & DMU_FORCE (if mode=strict)
    **		dmu_tbl_id	=   table id parameter, possibly index_id
    */

    if (patch_flag)
    {
	char_entry[0].char_id = DMU_VERIFY;
	char_entry[0].char_value = DMU_V_PATCH;
	char_entry[1].char_id = DMU_VOPTION;
	char_entry[1].char_value = DMU_T_LINK | DMU_T_RECORD | 
		DMU_T_ATTRIBUTE | DMU_T_PERIPHERAL;

	/* 
	**  check to see if the "force" mode was selected  --
	**  2nd parameter = mode 
	*/
        STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
	mode = (char *) dsh->dsh_row[db_parm[1].dbp_rowno] + 
		db_parm[1].dbp_offset;
	real_length = qea_findlen( mode, row_dbv.db_length);    
	if ( MEcmp( (PTR)mode, (PTR) "force", real_length) == 0)
	{
	    char_entry[0].char_value = DMU_V_FPATCH;
	}
	dmu_cb.dmu_tbl_id.db_tab_index = 0;
    }
    else
    {
	/* the 2nd parameter is the index table id */

	char_entry[0].char_id = DMU_VERIFY;
	char_entry[0].char_value = DMU_V_VERIFY;
	char_entry[1].char_id = DMU_VOPTION;
	char_entry[1].char_value = DMU_T_BITMAP | DMU_T_LINK | DMU_T_RECORD | 
		DMU_T_ATTRIBUTE | DMU_T_PERIPHERAL;

	STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
	index_id = (u_i4 *)((char *)dsh->dsh_row[db_parm[1].dbp_rowno] +
		db_parm[1].dbp_offset);
	dmu_cb.dmu_tbl_id.db_tab_index = *index_id;
    }

    /* 1st parameter = table's base id */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    table_id = (u_i4 *)((char *)dsh->dsh_row[db_parm[0].dbp_rowno] +
		db_parm[0].dbp_offset);
    dmu_cb.dmu_tbl_id.db_tab_base = *table_id;

    /* 
    **  Third parameter is the verbose flag.  If this is set, then indicate that
    **  the operation should display informative messages.  This is accomplished
    **	by adding DMU_V_VERBOSE to the value of the DMU_VERIFY's char_value
    **  entry.
    */
    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    verbose = (char *) dsh->dsh_row[db_parm[2].dbp_rowno] + 
	    db_parm[2].dbp_offset;
    real_length = qea_findlen( verbose, row_dbv.db_length);    
    if ( MEcmp( (PTR)verbose, (PTR) "verbose", real_length) == 0)
    {
	char_entry[0].char_value += DMU_V_VERBOSE;
    }

    /*
    **	call DMF to do list operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMU_MODIFY_TABLE, &dmu_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmu_cb.error.err_code;
        return (status);
     }

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: qea_8_finddbs - implement iiQEF_finddbs Internal Procedure
**
** Description:
**      This INTERNAL Procedure is to find all databases in a specified 
**	location. The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_finddbs (
**	    loc_name	    = char(DB_LOC_MAXNAME) not null not default,
**	    loc_area	    = char(DB_MAXNAME) not null not default,
**	    codemap_exists  = integer,
**	    verbose_flag    = integer,
**	    priv_50dbs	    = integer)
**	AS begin
**	      execute internal;
**	   end;
**
**	and evoked by:
**	
**	EXECUTE PROCEDURE iiQEF_finddbs (loc_name = :name_of_location,
**					 loc_area = :location_full_pathname,
**					 codemap_exists = :flag1,
**					 verbose_flag = :flag2,
**					 priv_50dbs = :flag3);
**
**	The loc_name parameter should contain the logical name assocaited
**	with a location.  This should be obtained from iilocations.lname.
**	The loc_area parameter should contain the full pathname to a
**	database default or extended data location as a null terminated
**	ASCII string.  This should be obtained from iilocations.area.
**
**	qea_8_finddbs creates an "empty" dmm_cb.  Then it fills in some
**	of the dmm_cb items from user parameters:
**		dmm_flags_mask		=   set conditionally based on P3 and P4
**						If p3 != 0 => DMM_UNKNOWN_DBA
**						If p4 != 0 => DMM_VERBOSE
**						If p5 != 0 => DMM_PRIV_50DBS
**		dmm_db_location		=   loc_area parameter
**		dmm->dmm_location_name	=   loc_name parameter
**		dmm_count		=   0
**	qea_8_finddbs calls DMF to do a DMM_FINDDBS operation.  If the operation
**	is successful, qea_8_finddbs will take the dmm_cb.dmm_count and
**	put it into the QEF_RCB.dbp_return.
**
**	If dmf is unsuccessful, then qea_8_finddbs will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement.
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    SYSTEM work catalogs iiphys_database and iiphys_extend may be
**	    populated with tuples describing user databases.
**
** History:
**      07-jan-1991 (teresa)
**	    Initial Creation.
**      30-Dec-92 (jhahn)
**          Fixed dsh_row casting.
[@history_template@]...
*/
DB_STATUS
qea_8_finddbs(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;
    u_i4		*int_value;
    
    /* 
    **  set up initialized DMM_CB 
    */
    qea_mk_dmmcb(dsh, &dmm_cb );

    /*  fill in list specific parameters from procedure arguments:
    **	    dmm_flags_mask    = set conditionally based on P3 and P4
    **				  If p3 != 0 => DMM_UNKNOWN_DBA
    **				  If p4 != 0 => DMM_VERBOSE
    **				  If p5 != 0 => DMM_PRIV_50DBS
    **	    dmm_db_location   = loc_area parameter
    **	    dmm_location_name = loc_name parameter
    **	    dmm_count	      = 0
    */

    /* 
    **	set dmm_flags_mask conditionally based on parameters 3 and 4, where
    ** if p3 != 0, set bit DMM_UNKNOWN_DBA and if p4 != 0 set bit 
    ** DMM_PRIV_50DBS.
    */
    dmm_cb.dmm_flags_mask = 0;
    STRUCT_ASSIGN_MACRO(db_parm[3].dbp_dbv, row_dbv);
    int_value = (u_i4 *)((char *)dsh->dsh_row[db_parm[3].dbp_rowno] + 
		db_parm[3].dbp_offset);
    if (*int_value)
	dmm_cb.dmm_flags_mask |= DMM_UNKNOWN_DBA;
    STRUCT_ASSIGN_MACRO(db_parm[4].dbp_dbv, row_dbv);
    int_value = (u_i4 *)((char *)dsh->dsh_row[db_parm[4].dbp_rowno] + 
		db_parm[4].dbp_offset);
    if (*int_value)
	dmm_cb.dmm_flags_mask |= DMM_VERBOSE;
    STRUCT_ASSIGN_MACRO(db_parm[5].dbp_dbv, row_dbv);
    int_value = (u_i4 *)((char *)dsh->dsh_row[db_parm[5].dbp_rowno] + 
		db_parm[5].dbp_offset);
    if (*int_value)
	dmm_cb.dmm_flags_mask |= DMM_PRIV_50DBS;

    /* 1st parameter = loc_name */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    dmm_cb.dmm_db_location.data_address  =
	    (char *) dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset;
    MEfill( sizeof(DB_LOC_NAME), ' ', (PTR) dmm_cb.dmm_location_name.db_loc_name);
    MEcopy( (PTR)((char *) dsh->dsh_row[db_parm[0].dbp_rowno] + 
	    db_parm[0].dbp_offset), row_dbv.db_length, 
	    (PTR) dmm_cb.dmm_location_name.db_loc_name);

    /* 2nd  parameter = loc_area */
    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    dmm_cb.dmm_db_location.data_address  =
	    (char *) dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    dmm_cb.dmm_count	= 0;

    /*
    **	call DMF to do list operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_FINDDBS, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb->qef_dbp_status = dmm_cb.dmm_count;
    return (status);
}

/*{
** Name: qea_12alter_extension - implement iiQEF_alter_extension internal proc
**
** Description:
**      This internal procedure is used to change the location status bits in
**	the config file.
**	
**	The procedure is defined as:
**
**	CREATE PROCEDURE iiQEF_alter_extension (
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_name = char(DB_LOC_MAXNAME) not null not default,
**	    drop_loc_type = integer not null not default,
**	    add_loc_type  = integer not null not default)
**	AS begin
**	      EXECUTE INTERNAL;
**	   end;
**
**	and invoked with:
**	
**	EXECUTE PROCEDURE iiQEF_alter_extension (
**	    database_name = :db_name,
**	    location_name = :loc_name,
**	    drop_loc_type = :old_loc_type,
**	    add_loc_type  = :new_loc_type);
**
** Inputs:
**    qef_rcb				QEF Request Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for param
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Request Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	The iiextend catalog in IIDBDB is modified.
**	DMF is called to alter bits in the configuration file.
**
** History:
**      22-feb-90  (jrb)
**	    Created.
**	28-feb-06 (hayke02)
**	    Jump out of location loop when we find a work or aux work location
**	    (as opposed to a data location). This allows a location to be used
**	    for data and work/aux work, and then allows the work/aux work
**	    location to be altered to aux work/work. This change fixes bug
**	    102500, problem INGSRV 1263.
[@history_template@]...
*/
DB_STATUS
qea_12alter_extension(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    QEF_CB              *qef_cb;
    DB_STATUS		status  = E_DB_OK;
    DB_STATUS		local_status;
    DMM_CB		dmm_cb;
    DB_DATA_VALUE	row_dbv;
    i4             error;
    i4		du_add_type;
    i4		du_drop_type;
    i4		dmm_add_type;
    i4		dmm_drop_type;
    i4		old_extstat;
    struct
    {
	i2		lname;		    /* length of ... */
	char		name[DB_LOC_MAXNAME];   /* A location name. */
    }			d_name, loc_name;
    DU_LOCATIONS	location;
    DU_EXTEND		dbextend;
    DU_DATABASE		database;
    DMM_LOC_LIST	loc_list[2];
    PTR			loc_ptr[2];
    QEU_CB		qeu;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	key;
    DMR_ATTR_ENTRY	*key_ptr = &key;

    qef_cb = dsh->dsh_qefcb;
    qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   directory parameter
    **		dmm_filename    =   file name parameter
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask = 0;


    /*	1st parameter = database_name */
    
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);


    /*	2nd parameter = location_name */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(dmm_cb.dmm_location_name), 
        (char *)&dmm_cb.dmm_location_name);


    /* 3rd parameter = drop location type. */

    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    du_drop_type = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
	db_parm[2].dbp_offset);
    if (du_drop_type == DU_EXT_DATA)
	dmm_drop_type = DMM_L_DATA;
    else if (du_drop_type == DU_EXT_WORK)
	dmm_drop_type = DMM_L_WRK;
    else if (du_drop_type == DU_EXT_AWORK)
	dmm_drop_type = DMM_L_AWRK;
    else
	dmm_drop_type = 0, du_drop_type = 0;


    /* 4th parameter = add location type. */

    STRUCT_ASSIGN_MACRO(db_parm[3].dbp_dbv, row_dbv);
    du_add_type = *(i4 *)((char *)dsh->dsh_row[db_parm[3].dbp_rowno] +
	db_parm[3].dbp_offset);
    if (du_add_type == DU_EXT_DATA)
	dmm_add_type = DMM_L_DATA;
    else if (du_add_type == DU_EXT_WORK)
	dmm_add_type = DMM_L_WRK;
    else if (du_add_type == DU_EXT_AWORK)
	dmm_add_type = DMM_L_AWRK;
    else
	dmm_add_type = 0, du_add_type = 0;

    /* Validate drop location and add location types; currently we allow only
    ** changing from WORK to AWORK and vice-versa
    */
    if (    (du_drop_type != DU_EXT_WORK  ||  du_add_type != DU_EXT_AWORK)
	&&  (du_drop_type != DU_EXT_AWORK  ||  du_add_type != DU_EXT_WORK)
    )
    {
	dsh->dsh_error.err_code = E_QE0240_BAD_LOC_TYPE;
	return (E_DB_ERROR);
    }

    {
	/*  Get record from iidatabase for this database. */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_DATABASE_TAB_ID;
	qeu.qeu_db_id = qef_rcb->qef_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_flag = 0;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask = 0;
	local_status = status = qeu_open(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_DATABASE);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_DATABASE);
	    qef_data.dt_data = (PTR)&database;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_DATABASE_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&dmm_cb.dmm_db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US0010_NO_SUCH_DB;
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /* Open iilocations catalog */
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_LOCATIONS_TAB_ID;
	qeu.qeu_db_id = qef_rcb->qef_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_flag = 0;
	qeu.qeu_access_mode = DMT_A_READ;
	MEcopy( ( PTR ) &dmm_cb.dmm_location_name, DB_LOC_MAXNAME,
		 ( PTR ) loc_name.name);
	loc_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	while (status == E_DB_OK)
	{    
	    /* look up default root location for this database in
	    ** iilocations catalog (we need to pass the area of this
	    ** location to dmm so it can find the config file)
	    */
	    MEcopy( ( PTR ) &database.du_dbloc, DB_LOC_MAXNAME,
		 ( PTR ) loc_name.name);
	    loc_name.lname = DB_LOC_MAXNAME;
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_LOCATIONS);
	    qef_data.dt_data = (PTR)&location;
	    qeu.qeu_getnext = QEU_REPO;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_LOCATIONS_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&loc_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
		}
	    }
	    local_status = qeu_close(qef_cb, &qeu);    
	    break;
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /*	Open iiextend catalog to make sure named database is extended to
    **  the specified location.
    */
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
	qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
	qeu.qeu_db_id = qef_rcb->qef_db_id;
	qeu.qeu_lk_mode = DMT_IX;
	qeu.qeu_flag = 0;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_count = 1;
	qeu.qeu_klen = 1;
	qeu.qeu_key = &key_ptr;
	key.attr_number = DM_1_EXTEND_KEY;
	key.attr_operator = DMR_OP_EQ;
	key.attr_value = (char *)&d_name;
	MEcopy( ( PTR ) &dmm_cb.dmm_db_name, DB_DB_MAXNAME,
		 ( PTR ) d_name.name);
	d_name.lname = DB_LOC_MAXNAME;
	qeu.qeu_mask = 0;

	local_status = status = qeu_open(qef_cb, &qeu);
	while (status == E_DB_OK)
	{    
	    /* Look up the location */
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_EXTEND);
	    qef_data.dt_data = (PTR)&dbextend;
	    status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		status = E_DB_ERROR;
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /* db not extended to this loc -- error */
		    dsh->dsh_error.err_code = E_QE0241_DB_NOT_EXTENDED;
		    break;
		}
		else
		{
		    /* some qef error */
		    dsh->dsh_error.err_code = E_US001F_IIEXTEND_ERROR;
		    break;
		}
	    }

	    /* jump out of the loop if we've found a matching work or aux
	    ** work location
	    */
	    if (MEcmp( ( PTR ) dbextend.du_lname,
			( PTR ) &dmm_cb.dmm_location_name,
			( u_i2 ) dbextend.du_l_length) == 0
		&&
		(dbextend.du_status & DU_EXT_WORK
		||
		dbextend.du_status & DU_EXT_AWORK))
	    {
		old_extstat = dbextend.du_status;
		break;
	    }

	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	}

	if (status == E_DB_OK  &&  (du_drop_type & dbextend.du_status) == 0)
	{
	    /* type of location we've been asked to drop is incompatible
	    ** with how the database was extended to that location--error
	    */
	    status = E_DB_ERROR;
	    dsh->dsh_error.err_code = E_QE0242_BAD_EXT_TYPE;
	}

	while (status == E_DB_OK)
	{
	    /* This is slightly stupid, but since there is no interface in
	    ** qeu for updating non-key columns, we will instead delete the
	    ** old tuple and insert a new one.
	    */
	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen = 0;
	    status = qeu_delete(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
		break;
	    }

	    /* whip up a new iiextend tuple with new status bits */
	    dbextend.du_l_length = DB_LOC_MAXNAME;
	    dbextend.du_d_length = DB_DB_MAXNAME;
	    MEcopy( ( PTR ) &dmm_cb.dmm_db_name, DB_DB_MAXNAME,
			 ( PTR ) dbextend.du_dname);
	    MEcopy( ( PTR ) &dmm_cb.dmm_location_name, DB_LOC_MAXNAME,
		 ( PTR ) dbextend.du_lname);
	    dbextend.du_status = (old_extstat & ~du_drop_type) | du_add_type;

	    /* append new tuple to iiextend */
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    qeu.qeu_input = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_EXTEND);
	    qef_data.dt_data = (PTR)&dbextend;
	    status = qeu_append(qef_cb, &qeu);
	    if ((status != E_DB_OK) || (qeu.qeu_count != 1))
	    {
		dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
		status = E_DB_ERROR;
	    }
	    local_status = qeu_close(qef_cb, &qeu);
	    break;
	}
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_US0083_EXTEND_UPDATE;
	    return (local_status);
	}
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /*	Call DMF to perform the operation. */

    /* fill in database's root location area and list of location types */
    dmm_cb.dmm_alter_op = DMM_EXT_ALTER;
    dmm_cb.dmm_db_location.data_address = location.du_area;
    dmm_cb.dmm_db_location.data_in_size = location.du_l_area;
    dmm_cb.dmm_loc_list.ptr_address = (PTR)loc_ptr;
    dmm_cb.dmm_loc_list.ptr_in_count = 2;
    dmm_cb.dmm_loc_list.ptr_size = sizeof(DMM_LOC_LIST);
    loc_ptr[0] = (PTR)&loc_list[0];
    loc_ptr[1] = (PTR)&loc_list[1];

    /* The only thing we use this array for is the location types; everything
    ** else is ignored.
    */
    loc_list[0].loc_type = dmm_drop_type;
    loc_list[1].loc_type = dmm_add_type;

    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_ALTER_DB, &dmm_cb);
    if (status != E_DB_OK)
    {
    	dsh->dsh_error.err_code = dmm_cb.error.err_code;
	return (status);
    }

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}


/*{
** Name: QEA_MAKE_DMUCB  - Make a generic (empty) DMU_CB
**
** Description:
**      This utility makes an "empty" dmu_cb.  The calling routine must
**	fill in user parameters.
**	    
** Inputs:
**    dsh				Data segment header
**    dmu_cb				Ptr to uninitialized DMM_CB
**    key				Ptr to uninitialized DMU_KEY_ENTRY
**    char_entry			Ptr to uninitialized DMU_CHAR_ENTRY
** Outputs:
**    *dmu_cb				Initializes the following dmm_cb 
**					fields:
**	.q_next					FW PTR (unchained)
**	.q_prev					BW PTR (unchained)
**	.length 				Size of DMU_CB??
**	.type					Identifies type of CB
**	.s_reserved, .l_reserved		Reserved
**	.owner					CB Owner
**	.ascii_id				CB ASCII Identifier
**	.error					Error Commumication Block
**	.dmm_tran_id				DMF Transaction ID ptr
**	.dmu_db_id				Open database identifier
**     *qef_rcb				If error, return error code in status
**	.error.err_code			    E_QE022F_SCU_INFO_ERROR
**					    or E_DB_OK
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      11-aug-89  (teg)
**	    Initial Creation.
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	23-Dec-2005 (kschendel)
**	    Don't know why we insist on asking SCF, the qefcb knows the DB id.
**	    (As does the qef RCB.)
[@history_template@]...
*/
static void
qea_make_dmucb(
QEE_DSH		   *dsh,
DMU_CB		   *dmu_cb,
DMU_KEY_ENTRY	   **key,
DMU_CHAR_ENTRY	   *char_entry )
{

    /* start by zero filling the whole structure */

    MEfill ( sizeof(DMU_CB), '\0', (PTR) dmu_cb);
    MEfill ( sizeof(DMU_CHAR_ENTRY), '\0', (PTR) char_entry);
    MEfill ( sizeof(DMU_KEY_ENTRY), '\0', (PTR) *key);

    /* fill in some standard values */

    dmu_cb -> length			= sizeof (DMU_CB);
    dmu_cb -> type			= DMU_UTILITY_CB;
    dmu_cb -> ascii_id			= DMU_ASCII_ID;
    dmu_cb -> dmu_tran_id		= dsh->dsh_dmt_id;
    dmu_cb -> dmu_db_id			= dsh->dsh_qefcb->qef_dbid;
    dmu_cb -> dmu_key_array.ptr_size	= sizeof(DMU_KEY_ENTRY);
    dmu_cb -> dmu_key_array.ptr_address	= (PTR) key;
    dmu_cb -> dmu_char_array.data_address = (PTR) char_entry;

}

/*{
** Name: QEA_9_READCONFIG - read a value from the config file.
**
** Description:
**      This INTERNAL Procedure is used to read a piece of data from the
**	config file of the database that the server is currently connected
**	to.  This procedure should not be used unless the server is connected
**	to this database, AS NO db LOCKING IS PERFORMED DUE TO THE ASSUMPTION
**	THAT WE ARE ALREADY CONNECTED TO THE DB WE WISH TO READ THE CONFIG FILE
**	FROM.
**
**	This is a specialized procedure that is NOT placed into the DBMS
**	catalogs.  Instead RDF knows what the procedure definition should be,
**	and RDF will behaves as though the procedure is created as:
**
**	CREATE PROCEDURE ii_read_config_value(
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_area = char(256) not null not default,
**	    readtype = integer not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	This procedure should only be evoked by upgradedb, and is evoked by:
**	
**	EXECUTE PROCEDURE ii_read_config_file(database_name = :db_name,
**					      location_area = :area, 
**					      readtype = :read_type)
**	where read_type must be one of the following:
**	    QEA_STATUS	    0x001   - read dsc_status from config file 
**	    QEA_CMPTLVL	    0x002   - read dsc_dbcmptlvl from config file
**	    QEA_CMPTMINOR   0x004   - read dsc_1dbcmptminor from config file
**	    QEA_ACCESS	    0x008   - read access from confif file
**	    QEA_SERVICE     0x010   - read service value from config file
**
**	If read_type does have atleast one of these bits set, an error will be 
**	returned!  If read_type has any bits set besides what is specified here,
**	an error will be returned!
**
**	The procedure will return the value read in the format of an i4 as the
**	procedure return value.
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	Procedure returnes the i4 value read from the config file.
**
** History:
**	02-oct-92 (teresa)
**	    initial creation
**	19-oct-1993 (rachael) Bug 55767
**	    Don't convert the location name to lowercase.
**	13-Feb-2006 (kschendel)
**	    Compiler caught typo in compat-minor fetch, guess we never do that.
**	
*/
DB_STATUS
qea_9_readconfig(
			QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh)
{
    char		area[LO_PATH_MAX+1];
    char		tempstr[TEMPLEN];
    DB_DATA_VALUE	row_dbv;
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    i4			real_length;
    i4			readtype;

    /* make basic dmc_cb */
     qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   
    **		dmm_db_location =   location area parameter
    **		dmm_db_name	=   database name
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask = 0;
    dmm_cb.dmm_count = 0;

    /* 3rd parameter is readtype */
    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    readtype = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
	db_parm[2].dbp_offset);

    /* if exactly one valid bit flag is set in the readtype, then set the
    ** corresponding request for DMF.  Otherwise, return an error 
    */
    if ((readtype & QEA_STATUS) && (! (readtype & ~QEA_STATUS))  )
    {
	dmm_cb.dmm_flags_mask = DMM_1GET_STATUS;
    }
    else if ((readtype & QEA_CMPTLVL) && (! (readtype & ~QEA_CMPTLVL))  )
    {
	dmm_cb.dmm_flags_mask = DMM_2GET_CMPTLVL;
    }
    else if ((readtype & QEA_CMPTMINOR) && (! (readtype & ~QEA_CMPTMINOR))  )
    {
	dmm_cb.dmm_flags_mask = DMM_3GET_CMPTMIN;
    }
    else if ((readtype & QEA_ACCESS) && (! (readtype & ~QEA_ACCESS))  )
    {
	dmm_cb.dmm_flags_mask = DMM_4GET_ACCESS;
    }
    else if ((readtype & QEA_SERVICE) && (! (readtype & ~QEA_SERVICE))  )
    {
	dmm_cb.dmm_flags_mask = DMM_5GET_SERVICE;
    }
    else
    {
	/* either no valid bits are set, or more than one bit is set */
	dsh->dsh_error.err_code = E_QE0133_WRONG_PARAM_TYPE;
	return (E_DB_ERROR);
    }

    /*	1st parameter = database_name */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /*	2nd parameter = location_area. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(area), area);
    dmm_cb.dmm_db_location.data_address = area;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    /*
    **	call DMF to do read config operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_CONFIG, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb->qef_dbp_status = dmm_cb.dmm_count;
    return (status);
}

/*{
** Name: QEA_10_UPDCONFIG - update one or more fields in the config file.
**
** Description:
**      This INTERNAL Procedure is used to udpate one or more fields from
**	the config file, providing the database is currently connected to the
**	database of the config file it wishes to modify.  NOTE THAT THIS 
**	INTERNAL PROCEDURE SHOULD NOT BE USED UNLESS THE FRONT END HAS ALREADY
**	OPENED THE DATABASE THAT CONTAINS THE TARGET CONFIG FILE.
**  
**	It will update any of the following fields:
**	    UPDATE_MAP:		Config field:	    paramater containing value:
**	    -----------------	----------------    ---------------------------
**	    QEA_UPD_STATUS	dsc_status	    status
**	    QEA_UPD_CMPTLVL	dsc_dbcmptlvl	    cmptlvl
**	    QEA_UPD_CMPTMINOR	dsc_1dbcmptminor    cmptminor
**	    QEA_UPD_ACCESS	dsc_dbaccess	    access
**	    QEA_UPD_SERVICE	dsc_dbservice	    service
**	    QEA_UPD_CVERSION	dsc_c_version	    cversion
**
**	This is a specialized procedure that is NOT placed into the DBMS
**	catalogs.  Instead RDF knows what the procedure definition should be,
**	and RDF will behaves as though the procedure is created as:
**
**	CREATE PROCEDURE ii_update_config(
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_area = char(256) not null not default,
**	    udpate_map = integer not null not default,
**	    status = integer,
**	    cmptlvl = integer,
**	    cmptminor = integer,
**	    access = integer,
**	    service = integer,
**	    cversion = integer)
**	AS begin
**	      execute internal;
**	   end;
**
**	This procedure should only be evoked by upgradedb, and is evoked by:
**	
**	EXECUTE PROCEDURE ii_update_config(
**	    database_name = :db_name,
**	    location_area = :area, 
**	    update_map = :map[, 
**	    status = :dsc_status, cmptlvl = :dsc_dbcmptlvl, 
**	    cmptminor = :dsc_1dbcmptminor, access = :dsc_dbaccess, 
**	    service = :dsc_dbservice, cversion = :dsc_c_version])
**	where map is a bit_map of which fields to update:
**	    QEA_UPD_STATUS    0x0001  -- use status value to upd. config file
**	    QEA_UPD_CMPTLVL   0x0002  -- use cmptlvl value to upd. config file
**	    QEA_UPD_CMPTMINOR 0x0004  -- use cmptminor value to upd. config file
**	    QEA_UPD_ACCESS    0x0010  -- use access value to upd. config file
**	    QEA_UPD_SERVICE   0x0020  -- use service value to upd. config file
**	    QEA_UPD_CVERSION  0x0040  -- use cversion value to upd. config file
**	    
**	If map does have atleast one of these bits set, an error will be 
**	returned!  If map has any bits set besides what is specified here,
**	an error will be returned!
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	Specified fields in the config file are updated.  IN the process, the
**	config file is locked and then unlocked.
**
** History:
**	02-oct-92 (teresa)
**	    initial creation
**	23-oct-92 (teresa)
**	    fixed QEA_UPD_ACCESS case.
**	30-Dec-92 (jhahn)
**	    Fixed dsh_row casting.
**	19-oct-1993 (rachael) Bug 55767
**	    Don't convert the location name to lowercase.
**	29-Dec-2004 (schka24)
**	    Allow updating of dsc_c_version.
*/
DB_STATUS
qea_10_updconfig(	QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh)
{
    char		area[LO_PATH_MAX+1];
    char		tempstr[TEMPLEN];
    DB_DATA_VALUE	row_dbv;
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    i4			map;
    i4			expected_map;
    i4			int_val;
    i4			real_length;

    /* make basic dmc_cb */
     qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   DMM_UPDATE_CONFIG
    **		dmm_db_location =   location area parameter
    **		dmm_db_name	=   database name
    **		dmm_count	=   0
    **	    plus any of the following if specified:
    **		dmm_status	=   status parameter
    **		dmm_dbcmptlvl	=   cmptlvl parameter
    **		dmm_dbcmptminor =   cmptminor parameter
    **		dmm_dbaccess	=   access parameter
    **		dmm_dbservice	=   dbservice parameter
    */

    dmm_cb.dmm_flags_mask = DMM_UPDATE_CONFIG;
    dmm_cb.dmm_count = 0;
    dmm_cb.dmm_upd_map = 0;

    /* 3rd parameter is update_map */
    STRUCT_ASSIGN_MACRO(db_parm[2].dbp_dbv, row_dbv);
    map = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
		db_parm[2].dbp_offset);
    expected_map = QEA_UPD_STATUS | QEA_UPD_CMPTLVL | QEA_UPD_CMPTMINOR |
		    QEA_UPD_ACCESS | QEA_UPD_SERVICE | QEA_UPD_CVERSION;
    if ( ((map & expected_map) == 0) || (map & ~expected_map))
    {
	/* either no update map flags are set, or invalid ones are set.
	** in either case, report an error 
	*/
	dsh->dsh_error.err_code = E_QE0133_WRONG_PARAM_TYPE;
	return (E_DB_ERROR);
    }

    /*	1st parameter = database_name */
    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /*	2nd parameter = location_area. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(area), area);
    dmm_cb.dmm_db_location.data_address = area;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    if (map & QEA_UPD_STATUS)
    {
	/* status is parameter # 4 */
	STRUCT_ASSIGN_MACRO(db_parm[3].dbp_dbv, row_dbv);
	int_val = 
		*(i4 *)((char *)dsh->dsh_row[db_parm[3].dbp_rowno] +
			db_parm[3].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_STATUS;
	dmm_cb.dmm_status = int_val;	
    }

    if (map & QEA_UPD_CMPTLVL)
    {
	/* comptlvl is parameter #5 */
	STRUCT_ASSIGN_MACRO(db_parm[4].dbp_dbv, row_dbv);
	int_val = 
		*(i4 *)((char *)dsh->dsh_row[db_parm[4].dbp_rowno] +
		db_parm[4].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_CMPTLVL;
	dmm_cb.dmm_cmptlvl = (u_i4) int_val;
    }

    if (map & QEA_UPD_CMPTMINOR)
    {
	/* compatminor is parameter # 6 */
	STRUCT_ASSIGN_MACRO(db_parm[5].dbp_dbv, row_dbv);
	int_val = 
		*(i4 *)((char *)dsh->dsh_row[db_parm[5].dbp_rowno] +
		db_parm[5].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_CMPTMINOR;
	dmm_cb.dmm_dbcmptminor = int_val;	
    }

    if (map & QEA_UPD_ACCESS)
    {
	/* access is parameter # 7 */
	STRUCT_ASSIGN_MACRO(db_parm[6].dbp_dbv, row_dbv);
	int_val = 
	     *(i4 *)((char *)dsh->dsh_row[db_parm[6].dbp_rowno] +
		db_parm[6].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_ACCESS;
	dmm_cb.dmm_db_access = int_val;	
    }

    if (map & QEA_UPD_SERVICE)
    {
	/* service is parameter # 8 */
	STRUCT_ASSIGN_MACRO(db_parm[7].dbp_dbv, row_dbv);
	int_val = 
		*(i4 *)((char *)dsh->dsh_row[db_parm[7].dbp_rowno] +
		db_parm[7].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_SERVICE;
	dmm_cb.dmm_db_service = int_val;	
    }

    if (map & QEA_UPD_CVERSION)
    {
	/* cversion is parameter # 9 */
	STRUCT_ASSIGN_MACRO(db_parm[8].dbp_dbv, row_dbv);
	int_val = 
		*(i4 *)((char *)dsh->dsh_row[db_parm[8].dbp_rowno] +
		db_parm[8].dbp_offset);
	dmm_cb.dmm_upd_map |= DMM_UPD_CVERSION;
	dmm_cb.dmm_cversion = int_val;	
    }

    /*
    **	call DMF to do config file update operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_CONFIG, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: qea_11_deldmp_config - delete dump config file
**
** Description:
**      This INTERNAL Procedure is used to delete the dump config file
**	assocaiated with an open database that is being upgraded.  This
**	procedure should only be called if the Front end has already opened
**	the database, as it does not lock the database.
**
**	This is a specialized procedure that is NOT placed into the DBMS
**	catalogs.  Instead RDF knows what the procedure definition should be,
**	and RDF will behaves as though the procedure is created as:
**
**	CREATE PROCEDURE ii_del_dmp_config(
**	    database_name = char(DB_DB_MAXNAME) not null not default,
**	    location_area = char(256) not null not default)
**	AS begin
**	      execute internal;
**	   end;
**
**	This procedure should only be evoked by upgradedb, and is evoked by:
**	
**	EXECUTE PROCEDURE ii_del_dmp_config(database_name = :db_name,
**					    location_area = :area)
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	The dump location config file is deleted.
**
** History:
**	02-oct-92 (teresa)
**	    initial creation
**	19-oct-1993 (rachael) Bug 55767
**	    Don't convert the location name to lowercase.
*/
DB_STATUS
qea_11_deldmp_config(	QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh)
{
    char		area[LO_PATH_MAX+1];
    char		tempstr[TEMPLEN];
    DB_DATA_VALUE	row_dbv;
    DB_STATUS		status  = E_DB_OK;
    DMM_CB		dmm_cb;
    i4			real_length;

    /* make basic dmc_cb */
    qea_mk_dmmcb(dsh, &dmm_cb);

    /*  fill in list specific parameters from procedure arguments:
    **		dmm_flags_mask	=   0
    **		dmm_db_location =   location area parameter
    **		dmm_db_name	+   database name
    **		dmm_count	=   0
    */

    dmm_cb.dmm_flags_mask = 0;
    dmm_cb.dmm_count = 0;
    /*	1st parameter = database_name */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[0].dbp_rowno] + db_parm[0].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    CVlower(tempstr);
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(dmm_cb.dmm_db_name), (char *)&dmm_cb.dmm_db_name);

    /*	2nd parameter = location_area. */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    MEmove(row_dbv.db_length,
	(char *)dsh->dsh_row[db_parm[1].dbp_rowno] + db_parm[1].dbp_offset,
	' ', sizeof(tempstr)-1, tempstr);
    tempstr[sizeof(tempstr)-1] = '\0';
    MEmove(sizeof(tempstr)-1, tempstr,
	' ', sizeof(area), area);
    dmm_cb.dmm_db_location.data_address = area;
    real_length = qea_findlen( (char *)dmm_cb.dmm_db_location.data_address,
				row_dbv.db_length);
    dmm_cb.dmm_db_location.data_in_size  = real_length;
    dmm_cb.dmm_db_location.data_out_size = real_length;

    /*
    **	call DMF to do dump config file delete operation
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMM_DEL_DUMP_CONFIG, &dmm_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmm_cb.error.err_code;
        return (status);
     }

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: qea_13_convert_table	- Implement iiQEF_convert_table 
**					    Internal Procedure 
**
** Description:
**	This internal procedure is used to convert a table to a new format.
**
**	The procedure to evoke the convert Table operation is:
**
**	CREATE PROCEDURE iiQEF_convert_table (
**		table_id i4 not null not default,
**		index_id i4 not null not default
**      AS begin
**            execute internal;
**         end;
**         
**      and evoked by:
**
**      EXECUTE PROCEDURE iiQEF_convert_table (table_id = iirelation.reltid,
**                                           index_id = iirelation.reltidx)
**
**
**      The table_id is the base table id and the index_id is the
**	tables's index id (zero unless the table is a secondary index).
**
**	This routine creates an "empty" dmu_cb.  Then it fills in some
**	of the dmucb items from user parameters:
**
**	    dmu_tab_id	    =	table_id, index_id.
**
**
**	If dmf is unsuccessful, then this routine will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				Array of ptrs to user parameters.  Each
**					element contains:
**	.dbp_name			    name of parameter
**	.dbp_rowno			    row offset into dsh to parameter
**	.dbp_offset			    col offset into dsh to parameter
**	.dbp_dbv			    ptr to ADF DB_DATA_VALUE for parameter
**    dsh				Data Segment Header (contains parameter
**					 values)
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    Any
**	    dmf error will cause dmf to return to QEF using standard error
**	    reporting mechanisms.
**
** History:
**	30-October-1992 (rmuth)
**          Initial creation.
**	30-Dec-92 (jhahn)
**	    Fixed dsh_row casting.
[@history_template@]...
*/
DB_STATUS
qea_13_convert_table(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{

    DB_STATUS		status  = E_DB_OK;
    DMU_CB		dmu_cb;
    DB_DATA_VALUE	row_dbv;
    i4			real_length;
    u_i4		*table_id;
    u_i4		*index_id;
    char		*mode;
    DMU_KEY_ENTRY	key_entry[1];
    DMU_KEY_ENTRY	*key;
    DMU_CHAR_ENTRY	char_entry[2];

    /* 
    **  set up initialized DMM_CB 
    */
    key = &key_entry[0];
    qea_make_dmucb(dsh, &dmu_cb, &key, &char_entry[0] );
    dmu_cb.dmu_char_array.data_in_size = sizeof(char_entry);

    /* Copy the table_id into the dmu_cb */

    STRUCT_ASSIGN_MACRO(db_parm[0].dbp_dbv, row_dbv);
    table_id = (u_i4 *)((char *)dsh->dsh_row[db_parm[0].dbp_rowno] +
		db_parm[0].dbp_offset);
    dmu_cb.dmu_tbl_id.db_tab_base = *table_id;

    /* Copy the index_id into the dmu_cb */

    STRUCT_ASSIGN_MACRO(db_parm[1].dbp_dbv, row_dbv);
    index_id = (u_i4 *)((char *)dsh->dsh_row[db_parm[1].dbp_rowno] +
		db_parm[1].dbp_offset);
    dmu_cb.dmu_tbl_id.db_tab_index = *index_id;


    /*
    **	call DMF to do list operation, the error returned is not mapped
    ** and will cause an internal error message to be generated.
    */
    dsh->dsh_error.err_code = E_DB_OK;
    status = dmf_call(DMU_CONVERT_TABLE, &dmu_cb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmu_cb.error.err_code;
    }

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: qea_14_error   - Implement iiQEF_error
**                                         Internal Procedure
**
** Description:
**      This internal procedure is used to convert a table to a new format.
**
**      The procedure to evoke the convert Table operation is:
**
**      CREATE PROCEDURE iiQEF_convert_table (
**              table_id i4 not null not default,
**              index_id i4 not null not default
**      AS begin
**            execute internal;
**         end;
**
**      and evoked by:
**
**      EXECUTE PROCEDURE iiQEF_convert_table (table_id = iirelation.reltid,
**                                           index_id = iirelation.reltidx)
**
**
**      The table_id is the base table id and the inde_id is the
**      tables's index id (zero unless the table is a secondary index).
**
**      This routine creates an "empty" dmu_cb.  Then it fills in some
**      of the dmucb items from user parameters:
**
**          dmu_tab_id     =   table_id, index_id.
**
**
**      If dmf is unsuccessful, then this routine will return DMF's error
**      code and error status.  Qeq_query will process the error by aborting
**      the execute procedure statemnt.
**
** Inputs:
**    params                            Array of ptrs to user parameter.  Each
**                                      element contains:
**     .dbp_name                           name of parameter
**      .dbp_rowno                          row offset into dsh to parameter
**      .dbp_offset                         col offset into dsh to parameter
**      .dbp_dbv                           ptr to ADF DB_DATA_VALUE for parameter
**    dsh                               Data Segment Header (contains parmeter
**                                       values)
** Outputs:
**    qef_rcb                          QEF Reqeust Block
**      qef_dbp_return                     number of files listed into table
**      error.err_code                     error status
**
**      Returns:
**          E_DB_OK
**          E_DB_WARNING
**          E_DB_ERROR
**          E_DB_FATAL
**     Exceptions:
**          none
**
** Side Effects:
**          Any
**          dmf error will cause dmf to return to QEF using standard error
**          reporting mechanisms.
**
** History:
**      30-October-1992 (Jhahn)
**          Initial creation.
[@history_template@]...
*/
static DB_STATUS
qea_14_error( 
	QEF_DBP_PARAM           *db_parm,
	QEE_DSH                 *dsh)
{
#define MAX_PARAMS 5

    DB_STATUS           status  = E_DB_OK;
    i4             err_no, detail, param_count;
    i4             err;
    int                 i;
    DB_TEXT_STRING      *(params[MAX_PARAMS]);

    err_no = *(i4 *)((char *)dsh->dsh_row[db_parm[0].dbp_rowno] +
                          db_parm[0].dbp_offset);
    detail = *(i4 *)((char *)dsh->dsh_row[db_parm[1].dbp_rowno] +
                              db_parm[1].dbp_offset);
    param_count = *(i4 *)((char *)dsh->dsh_row[db_parm[2].dbp_rowno] +
                               db_parm[2].dbp_offset);

    for (i = 0; i<MAX_PARAMS; i++)
    {
        params[i] = ((DB_TEXT_STRING *)dsh->dsh_row[db_parm[3+i].dbp_rowno] +
                               db_parm[3+i].dbp_offset);
    }
    qef_error(err_no, detail, E_DB_ERROR, &err, &dsh->dsh_error, param_count,
              (i4) params[0]->db_t_count, params[0]->db_t_text,
              (i4) params[1]->db_t_count, params[1]->db_t_text,
              (i4) params[2]->db_t_count, params[2]->db_t_text,
              (i4) params[3]->db_t_count, params[3]->db_t_text,
              (i4) params[4]->db_t_count, params[4]->db_t_text);

    return (E_DB_ERROR);
}

/*{
** Name: qea_15_make_udt_defaults - Implement iiQEF_make_udt_defaults
**					    Internal Procedure 
**
** Description:
**	This internal procedure is used to write a default tuple to
**	IIDEFAULT for each UDT in the database.
**
**	Here's how to create this procedure:
**
**	CREATE PROCEDURE iiQEF_make_udt_defaults
**      AS begin
**            execute internal;
**         end;
**         
**      And here's how to invoke it:
**
**      EXECUTE PROCEDURE iiQEF_make_udt_defaults;
**
**	This routine should only be called when upgrading a database to 65.
**
**	The first time a 65 server connects to a 64 database, a DMF routine
**	(dm2d\makeDefaultID) will loop through IIATTRIBUTE, filling in
**	canonical default IDs based on datatype, defaultability, and
**	nullability.  When makeDefaultID stumbles across a UDT with a
**	default, it will confess its unworthiness to convert the tuple by
**	marking the default ID as DB_DEF_ID_NEEDS_CONVERTING.
**
**	Later on, CREATEDB will invoke iiQEF_make_udt_defaults, triggering
**	the following code.  This function will again loop through
**	IIATTRIBUTE, looking for tuples marked DB_DEF_ID_NEEDS_CONVERTING.
**	For each of these tuples, a new default ID will be allocated
**	and a new default tuple will be written describing the default
**	value for the UDT.
**
**	Simple, no?
**
**	Why don't we do all the work in DMF you might ask.  Well.  We
**	thought it would be cleaner if all the knowledge of how to
**	create defaults were localized in one facility, viz., the
**	might QEF.
**
**	This routine will call DMF to loop through IIATTRIBUTE.
**	If dmf is unsuccessful, then this routine will return DMF's error
**	code and error status.  Qeq_query will process the error by aborting
**	the execute procedure statement
**
** Inputs:
**    qef_rcb				QEF Reqeust Block
**	.qef_cb				    QEF Session Control Block ptr
**	    .qef_dmt_id			 	DMF Transaction ID ptr
**    params				This procedure takes no parameters.
**    dsh				Data Segment Header (contains parameter
**					 values, of which of course there are
**					 none.
** Outputs:
**    qef_rcb				QEF Reqeust Block
**	qef_dbp_return			    number of files listed into table
**	error.err_code			    error status
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    A dmf error will cause us to return to QEF using standard error
**	    reporting mechanisms.
**
** History:
**	11-mar-93 (rickh)
**	    Am Anfang war die Tat.
[@history_template@]...
*/

#define	UNQUOTED_DEFAULT_LENGTH	\
	( DB_MAX_COLUMN_DEFAULT_LENGTH + DB_CNTSIZE )

static DB_STATUS
qea_15_make_udt_defaults(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh )
{
    GLOBALREF QEF_S_CB *Qef_s_cb;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status  = E_DB_OK;
    QEU_CB	    	read_qeu,  *rqeu = &read_qeu,
    			IIDEFAULTqeu_cb,
    			IIDEFAULTIDXqeu_cb;
    QEU_QUAL_PARAMS	qparams;
    bool		IIATTRIBUTEopen = FALSE,
    			IIDEFAULTopened = FALSE,
    			IIDEFAULTIDXopened = FALSE;
    QEA_ATTRIBUTE	IIATTRIBUTEtuple;
    DB_IIDEFAULT	*IIDEFAULTtuple;
    PTR			emptyUDTvalue;
    QEF_DATA		qef_data;
    i4		error;
    ULM_RCB		ulmRCB, *ulm = &ulmRCB;
    DB_TEXT_STRING	*unquotedDefaultValue;

    BEGIN_CODE_BLOCK	/* something to break out of */

	/*
	** open a memory stream to hold the default tuple and its empty
	** value.  we do this because these are potentially large objects
	** that should not live on our tiny stack.
	*/

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, *ulm);
        if ((status = qec_mopen( ulm )) != E_DB_OK)
        {
	    status = E_DB_ERROR;
	    error = E_QE001E_NO_MEM;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    EXIT_CODE_BLOCK;
        }

	/* so allocate space for the re-usable default tuple */

        ulm->ulm_psize = sizeof( DB_IIDEFAULT );
        status = qec_malloc(ulm);
        if (status != E_DB_OK)
        {
            error = ulm->ulm_error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
            EXIT_CODE_BLOCK;
        }
	IIDEFAULTtuple = ( DB_IIDEFAULT * ) ulm->ulm_pptr;

	/*
	** ... and space for the re-usable empty default value.  a UDT 
	** can't be larger than the maximum tuple size.
	*/

        ulm->ulm_psize = DB_MAXTUP;
        status = qec_malloc(ulm);
        if (status != E_DB_OK)
        {
            error = ulm->ulm_error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
            EXIT_CODE_BLOCK;
        }
	emptyUDTvalue = ulm->ulm_pptr;

	/*
	** ... and space for the re-usable storage for the conversion of
	** the empty value to an unquoted string.
	*/

        ulm->ulm_psize = UNQUOTED_DEFAULT_LENGTH;
        status = qec_malloc(ulm);
        if (status != E_DB_OK)
        {
            error = ulm->ulm_error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
            EXIT_CODE_BLOCK;
        }
	unquotedDefaultValue = ( DB_TEXT_STRING * ) ulm->ulm_pptr;

	/* open IIATTRIBUTE for writing */

	rqeu->qeu_type = QEUCB_CB;
	rqeu->qeu_length = sizeof(QEUCB_CB);
	rqeu->qeu_tab_id.db_tab_base  = DM_B_ATTRIBUTE_TAB_ID; 
	rqeu->qeu_tab_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

	rqeu->qeu_db_id = qef_rcb->qef_db_id;
	rqeu->qeu_lk_mode = DMT_IX;
	rqeu->qeu_access_mode = DMT_A_WRITE;
	rqeu->qeu_flag = 0;
	rqeu->qeu_mask = 0;
	status = qeu_open(qef_cb, rqeu);	/* open IIATTRIBUTE */
	if (status != E_DB_OK)
	{
	    error = rqeu->error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    EXIT_CODE_BLOCK;
	}
	IIATTRIBUTEopen = TRUE;

	/* get IIATTRIBUTE tuples describing UDTs which have defaults */

	rqeu->qeu_tup_length = sizeof( QEA_ATTRIBUTE );
	rqeu->qeu_getnext = QEU_REPO;   /*
					** this will be reset to QEU_NOREPO
					** after the first qeu_get()
					*/

	rqeu->qeu_count = 1;	    /* will look up a tuple at a time */

	rqeu->qeu_klen = 0;
	rqeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;
	rqeu->qeu_flag = 0;

	rqeu->qeu_input = rqeu->qeu_output = &qef_data;
	qef_data.dt_next = NULL;
	qef_data.dt_size = sizeof( QEA_ATTRIBUTE );
	qef_data.dt_data = ( PTR ) &IIATTRIBUTEtuple;

	/* now loop through IIATTRIBUTE */

	for ( ; ; )
	{
	    rqeu->qeu_qual = UDTsieve;
	    rqeu->qeu_qarg = &qparams;

	    status = qeu_get( qef_cb, rqeu );
	    if ( status != E_DB_OK )
	    {
	        error = rqeu->error.err_code;
		if ( error == E_QE0015_NO_MORE_ROWS )
		{   status = E_DB_OK; }
		else
		{  qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 ); }

		break;	/* end of loop through IIATTRIBUTE */
	    }
	    else
	    {
	        rqeu->qeu_getnext = QEU_NOREPO;
	    }

	    /* if we haven't opened IIDEFAULT yet, do so */

	    if ( IIDEFAULTopened == FALSE )
	    {
		status = qeu_tableOpen( qef_rcb, &IIDEFAULTqeu_cb,
				    ( u_i4 ) DM_B_IIDEFAULT_TAB_ID,
				    ( u_i4 ) DM_I_IIDEFAULT_TAB_ID );
 		if ( status != E_DB_OK )	{ break; }

		IIDEFAULTopened = TRUE;
	    }

	    /* if we haven't opened IIDEFAULTIDX yet, do so */

	    if ( IIDEFAULTIDXopened == FALSE )
	    {
		status = qeu_tableOpen( qef_rcb, &IIDEFAULTIDXqeu_cb,
				    ( u_i4 ) DM_B_IIDEFAULTIDX_TAB_ID,
				    ( u_i4 ) DM_I_IIDEFAULTIDX_TAB_ID );
 		if ( status != E_DB_OK )	{ break; }

		IIDEFAULTIDXopened = TRUE;
	    }

	    /* construct an IIDEFAULT tuple for this UDT */

	    status = constructUDTdefaultTuple( dsh, &IIATTRIBUTEtuple,
			IIDEFAULTtuple, emptyUDTvalue, unquotedDefaultValue );
	    if ( status != E_DB_OK )
	    {
		/*
		** it may happen that this UDT has an oversized default.
		** we don't know what to do with such a default, so we
		** will leave it uncoverted and go on to the next
		** UDT.
		*/

		if ( dsh->dsh_error.err_code == E_QE0305_UDT_DEFAULT_TOO_BIG )
		{   status = E_DB_OK; continue; }
		else	{ break; }
	    }	/* endif we couldn't construct a default */

	    /*
	    ** see if such a tuple is in IIDEFAULT.  if not, add it.
	    ** this incredible routine not only finds or fabricates a
	    ** default id, IT ACTUALLY WRITES THAT ID INTO THE IIATTRIBUTE
	    ** TUPLE!
	    */

	    status = qeu_findOrMakeDefaultID( qef_rcb,
					  &IIDEFAULTqeu_cb,
					  &IIDEFAULTIDXqeu_cb,
					  IIDEFAULTtuple,
					  &IIATTRIBUTEtuple.attDefaultID );
	    if ( status != E_DB_OK )	{ break; }

	    /* write the updated IIATTRIBUTE tuple */

	    rqeu->qeu_qual = NULL;
	    rqeu->qeu_qarg = NULL;
	    status = qeu_replace ( qef_cb, rqeu );
	    if ( status != E_DB_OK )
	    {
	        error = rqeu->error.err_code;
		qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
		break;
	    }
	}	/* end of loop through IIATTRIBUTE */

    END_CODE_BLOCK

    /* close the catalogs we opened */

    if ( IIATTRIBUTEopen == TRUE )
    {   ( VOID ) qeu_close( qef_cb, rqeu ); }
    if ( IIDEFAULTopened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &IIDEFAULTqeu_cb );    }
    if ( IIDEFAULTIDXopened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &IIDEFAULTIDXqeu_cb );    }

    /* tear down the memory stream */

    ulm_closestream( ulm );

    qef_rcb->qef_dbp_status = (i4) status;
    return (status);
}

/*{
** Name: UDTsieve - Qualify an iiattribute tuple by its UDTness
**
** Called from DMF via:
**	    qparams.qeu_rowaddr = address-of-tuple;
**	    status = UDTsieve(ignore, &qparams);
**	    returns with qparams.qeu_retval set to ADE_TRUE or ADE_NOT_TRUE
**
** Description:
**	This is a DMF qualification function that is called while reading
**	IIATTRIBUTE during CREATEDB.  This function qualifies all tuples
**	whose default ID has been set to DB_DEF_ID_NEEDS_CONVERTING.
**	(Upon the first connection of a 65 server to a pre-65 database,
**	DMF will mark the default IDs of IIATTRIBUTE tuples with this
**	id for all attributes that are UDTs taking defaults.)
**	Qualified tuples are returned to the qea_15_make_udt_defaults routine
**	for further processing.
**
** Inputs:
**	qparams		Address of QEU_QUAL_PARAMS with row addr set
**
** Outputs:
**	Sets qparams.qeu_retval: ADE_TRUE if qualifies, ADE_NOT_TRUE if not
**	Returns E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**	12-mar-93 (rickh)
**	    Creation.
**	11-Apr-2008 (kschendel)
**	    New style usage sequence.
*/
static DB_STATUS
UDTsieve(void *toss, QEU_QUAL_PARAMS *qparams)
{
    QEA_ATTRIBUTE	*tuple = (QEA_ATTRIBUTE *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if ( ( tuple->attDefaultID.db_tab_base == DB_DEF_ID_NEEDS_CONVERTING ) &&
	 ( tuple->attDefaultID.db_tab_index == 0 ) )
    {
	qparams->qeu_retval = ADE_TRUE;		/* Qualified! */
    }

    return (E_DB_OK);
}

/*{
** Name: constructUDTdefaultTuple - make a default tuple for a UDT
**
** Description:
**	This routine constructs a default tuple for a UDT, suitable for
**	stuffing into IIDEFAULT.
**
**
** Inputs:
**	dsh			QEF goodies
**	attribute		description of the UDT
**
** Outputs:
**	IIDEFAULTtuple		default tuple to fill in.  we will fill
**				in the default value.  the default id
**				will be divined elsewhere.
**	emptyUDTvalue		this is scratch space in which to pour
**				the UDT's default before we coerce it into
**				a long string.
**
**      Returns:
**	    E_DB_OK
**	    E_DB_WARNING
**	    E_DB_ERROR
**	    E_DB_FATAL
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**	12-mar-93 (rickh)
**	    Creation.
**	8-apr-93 (rickh)
**	    Improved the UDT convertor to call a delimited id routine.
**	16-dec-04 (inkdo01)
**	    Added a couple of collID's.
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/

static DB_STATUS
constructUDTdefaultTuple(
    QEE_DSH		*dsh,
    QEA_ATTRIBUTE	*attribute,
    DB_IIDEFAULT	*IIDEFAULTtuple,
    PTR			emptyUDTvalue,
    DB_TEXT_STRING	*unquotedDefaultValue
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    ADF_CB	   	*adf_scb = dsh->dsh_adf_cb;
    ADI_FI_ID	   	conv_func_id;
    ADF_FN_BLK	   	conv_func_blk;
    DB_DATA_VALUE  	db_data;
    char	   	*defaultString;
    u_i4		quotedStringLength, unquotedStringLength;
    i4			i;
    i4	   	error;
    i4		maxDefaultSize = DB_MAX_COLUMN_DEFAULT_LENGTH;

    /* clean the slate */

    dsh->dsh_error.err_code = 0;
    MEfill( sizeof(DB_IIDEFAULT), NULLCHAR, ( PTR ) IIDEFAULTtuple );

    /*
    ** use absolute value of datatype to make sure it is non-null,
    ** as we always want to get a non-null value back
    */

    db_data.db_datatype = abs( attribute->attfmt );
    db_data.db_length   = attribute->attfml;
    db_data.db_prec     = attribute->attfmp;
    db_data.db_data	= emptyUDTvalue;
    db_data.db_collID   = attribute->attcollID;

    /*
    ** if datatype is nullable,
    ** subtract 1 so that null byte is not counted in the length
    */
    if ( attribute->attfmt < 0 )	{ db_data.db_length--; }

    /* now get the empty value for this UDT */

    status = adc_getempty (adf_scb, &db_data);
    if (DB_FAILURE_MACRO(status))
    { QEF_ERROR( adf_scb->adf_errcb.ad_errcode ); return (status); }

    /* get function for converting value to long-text */

    status = adi_ficoerce(adf_scb, db_data.db_datatype, DB_LTXT_TYPE,
    		  &conv_func_id);
    if (DB_FAILURE_MACRO(status))
    { QEF_ERROR( adf_scb->adf_errcb.ad_errcode ); return (status); }

    /*
    ** set up function block to convert the default value,
    ** and call ADF to execute it.  set up the 1st and only parameter
    */

    conv_func_blk.adf_fi_desc = NULL;
    conv_func_blk.adf_dv_n = 1;
    STRUCT_ASSIGN_MACRO(db_data, conv_func_blk.adf_1_dv);

    /*
    **  set up the result value;
    ** reserve enough space for the largest possible default, and
    ** include space for the 'count' field in a long-text data value
    */

    conv_func_blk.adf_r_dv.db_datatype	= DB_LTXT_TYPE;
    conv_func_blk.adf_r_dv.db_prec	= 0;
    conv_func_blk.adf_r_dv.db_length	= UNQUOTED_DEFAULT_LENGTH;
    conv_func_blk.adf_r_dv.db_data	= ( PTR ) unquotedDefaultValue;
    conv_func_blk.adf_r_dv.db_collID	= -1;
    conv_func_blk.adf_fi_id		= conv_func_id;

    conv_func_blk.adf_pat_flags = AD_PAT_NO_ESCAPE;
	    
    /* coerce the empty UDT value to a long string */

    status = adf_func(adf_scb, &conv_func_blk);
    if (DB_FAILURE_MACRO(status))
    { QEF_ERROR( adf_scb->adf_errcb.ad_errcode ); return (status); }

    /*
    ** have to add single quotes around the text value (ANSI semantics),
    ** so if the size of the converted text is bigger than MAX-2,
    ** we can't fit this default into our IIDEFAULT catalog
    */

    unquotedStringLength = unquotedDefaultValue->db_t_count;
    quotedStringLength = maxDefaultSize;
	
    status = cui_idunorm(	unquotedDefaultValue->db_t_text,
				&unquotedStringLength,
				(u_char *) IIDEFAULTtuple->dbd_def_value,
				&quotedStringLength,
				CUI_ID_QUO | CUI_ID_NOLIMIT, &dsh->dsh_error );

    IIDEFAULTtuple->dbd_def_length = quotedStringLength;

    if (DB_FAILURE_MACRO(status))
    {
	if ( dsh->dsh_error.err_code == E_US1A25_ID_TOO_LONG)
	{
	    error = E_QE0305_UDT_DEFAULT_TOO_BIG;
	    (VOID) qef_error( E_QE0305_UDT_DEFAULT_TOO_BIG, 0L, status, &error,
			  &dsh->dsh_error, 2,
			  DB_MAXNAME, &attribute->attname,
			  sizeof( maxDefaultSize ), &maxDefaultSize );
	}
	else 
	{
	    error = dsh->dsh_error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	}
	return( E_DB_ERROR );
    }

    return( status );
}


/*{
** Name: get_loc_tuple -        Get iilocations tuple for specified locname
**
** Description:
**      This routine gets the iilocations tuple for the specified
**      database, logical location name.
**
** Inputs:
**	qef_rcb			QEF goodies
**      locname                 Location name
**
** Outputs:
**      loctup                  Location tuple returned.
**
**      Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**      31-jan-95 (stial01)
**          BUG 66510: created
*/
static DB_STATUS
get_loc_tuple(
QEE_DSH			*dsh,
DB_LOC_NAME             *locname,
DU_LOCATIONS            *loctup)
{
    DMR_ATTR_ENTRY      key;
    DMR_ATTR_ENTRY      *key_ptr = &key;
    DU_LOCATIONS        location;
    DB_STATUS           status  = E_DB_OK;
    DB_STATUS           local_status;
    QEF_CB		*qefcb = dsh->dsh_qefcb;
    QEU_CB              qeu;
    QEF_DATA            qef_data;
    struct
    {
	i2              lname;              /* length of ... */
	char            name[DB_LOC_MAXNAME];   /* A location name. */
    }                   location_name;

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
    qeu.qeu_db_id = qefcb->qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IS;
    qeu.qeu_flag = 0;
    qeu.qeu_access_mode = DMT_A_READ;
    MEcopy( ( PTR ) locname, DB_LOC_MAXNAME, ( PTR ) location_name.name);
    location_name.lname = DB_LOC_MAXNAME;
    qeu.qeu_mask = 0;

    local_status = status = qeu_open(qefcb, &qeu);
    if (status == E_DB_OK)
    {    
	/*  Lookup the location. */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_LOCATIONS);
	qef_data.dt_data = (PTR)loctup;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;
	qeu.qeu_key = &key_ptr;
	key.attr_number = DM_1_LOCATIONS_KEY;
	key.attr_operator = DMR_OP_EQ;
	key.attr_value = (char *)&location_name;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	status = qeu_get(qefcb, &qeu);
	if (status != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		dsh->dsh_error.err_code = E_US001D_IILOCATIONS_ERROR;
	    }
	}
	local_status = qeu_close(qefcb, &qeu);    
    }
    if (local_status != E_DB_OK)
    {
	dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	return (local_status);
    }
    return (status);
}


/*{
** Name: get_extent_info -      Process iiextend records for db  
**
** Description:
**      This routine reads all iiextend tuples for the specified database.
**
**      If phase == GET_EXTENT_COUNT -> Return the  count of iiextend tuples 
**                                      for this database
**      If phase == GET_EXTENT_AREAS -> Return area information for all 
**                                      iiextend tuples for this database
**
** Inputs:
**	dsh			QEF goodies
**      phase                   GET_EXTENT_COUNT
**                              GET_EXTENT_AREAS
**      dbname                  database name
**
** Outputs:
**      loc_list                If phase == GET_EXTENT_AREAS, location  
**                              info for all iiextend tuples will be returned
**                              here.
**      ext_countp              If phase == GET_EXTENT_COUNT, the number of 
**                              iiextend tuples will be returned here.
**
**      Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**      31-jan-95 (stial01)
**          BUG 66510: created
**	21-Dec-95 (nive)
**	    Bug 71480: Location names read from iiextend table whose length
**	    < 32 (happens for database upgraded from R6) have to be padded 
**	    with blanks upto 32 chars length. Also added error checking 
**	    for get_loc_tuple return value
**	12-Jan-96 (nive)
**	    Bug 71480 - Previous change lacked a variable definition 
**	    (i4 j)
*/
static DB_STATUS
get_extent_info(
QEE_DSH			*dsh,
i4                 phase,
DB_DB_NAME              *dbname,
DMM_LOC_LIST            **loc_list,
i4                 *ext_countp)
{
    DMR_ATTR_ENTRY      key;
    DMR_ATTR_ENTRY      *key_ptr = &key;
    DB_STATUS           status  = E_DB_OK;
    DB_STATUS           local_status;
    DU_EXTEND           dbextend;
    QEF_CB		*qefcb = dsh->dsh_qefcb;
    QEU_CB              qeu;
    QEF_DATA            qef_data;
    DU_LOCATIONS        loctup;
    i4             i,j;
    struct
    {
	i2              lname;              /* length of ... */
	char            name[DB_LOC_MAXNAME];   /* A location name. */
    }       db_name;

    /* 
    ** Open the iiextend catalog, get all locations that this db is
    ** extended:
    ** if (phase == GET_EXTENT_COUNT)  Return the count of iiextend records
    **                                 for this database.
    ** if (phase == GET_EXTENT_AREAS)  Return location information for all
    **                                 iiextend records for this database.
    */
    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
    qeu.qeu_db_id = qefcb->qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IS;
    qeu.qeu_flag = QEU_BYPASS_PRIV;
    qeu.qeu_access_mode = DMT_A_READ;
    MEcopy( ( PTR ) dbname, DB_DB_MAXNAME, ( PTR ) db_name.name);
    db_name.lname = DB_DB_MAXNAME;
    qeu.qeu_mask = 0;

    if (phase == GET_EXTENT_COUNT)
	*ext_countp = 0;

    local_status = status = qeu_open(qefcb, &qeu);
    if (status == E_DB_OK)
    {
	qeu.qeu_getnext = QEU_REPO; /* reposition using key */
        for (i = 0 ; ; i++ )
	{    
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DU_EXTEND);
	    qeu.qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DU_EXTEND);
	    qef_data.dt_data = (PTR)&dbextend;
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = &key_ptr;
	    key.attr_number = DM_1_EXTEND_KEY;
	    key.attr_operator = DMR_OP_EQ;
	    key.attr_value = (char *)&db_name;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    
	    status = qeu_get(qefcb, &qeu);
	    if (status != E_DB_OK) 
	    {
		if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    /* some qef error */
		    dsh->dsh_error.err_code = E_US001F_IIEXTEND_ERROR;
		    break;
		}
	    }

	    if (phase == GET_EXTENT_COUNT)
		(*ext_countp)++;
	    else   /* GET_EXTENT_AREAS */
	    {
/* 
bug 71480 - pad dbextend lname with blanks to make length 32 
*/
		if (dbextend.du_l_length < DB_LOC_MAXNAME )
		{
		    for ( j = dbextend.du_l_length; j < DB_LOC_MAXNAME; j ++)
				dbextend.du_lname[j] = ' ';
       		}
		status = get_loc_tuple(dsh,
				(DB_LOC_NAME *)&dbextend.du_lname, &loctup);
/*
bug 71480 - added status error checking
*/
		if ( status != E_DB_OK )
			break ;

		/* add to loc_list */
		MEmove( loctup.du_l_lname,
			(PTR)loctup.du_lname, ' ',
			sizeof (DB_LOC_NAME),
			(PTR)&loc_list[i]->loc_name);
		MEcopy( (PTR)loctup.du_area, loctup.du_l_area, 
			(PTR)loc_list[i]->loc_area);
		loc_list[i]->loc_l_area = loctup.du_l_area;
		if (dbextend.du_status & DU_EXT_DATA)
		    loc_list[i]->loc_type = DMM_L_DATA;
		else if (dbextend.du_status & DU_EXT_WORK)
		    loc_list[i]->loc_type = DMM_L_WRK;
		else if (dbextend.du_status & DU_EXT_AWORK)
		    loc_list[i]->loc_type = DMM_L_AWRK;

	    }

	    qeu.qeu_getnext = QEU_NOREPO;   /* continue from previous call */
	    qeu.qeu_klen = 0;
	}

	/* Close iiextend catalog */
	local_status = qeu_close(qefcb, &qeu);
    }

    if (local_status != E_DB_OK)
    {
	dsh->dsh_error.err_code = E_US0082_DATABASE_UPDATE;
	return (local_status);
    }

    return (status);
}
