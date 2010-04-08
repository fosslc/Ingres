/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUERR.H -	contains all definitions necessary to do system
**			utility error handling.
**
** Description:
**        This file contains all the definitions necessary to do system
**	utility error handling.
**
** Dependencies:
**	er.h
**
** History: 
**      04-Sep-86 (ericj)
**          Initial creation.
**	15-Jul-1988 (teg)
**	    Added msg constants for verifydb phase 1.
**	26-Jan-1989
**	    Added msg constants for VERIFYDB legal warning.
**	13-Feb-1989 (teg)
**	    Added msg constants for VERIFYDB phase III (iidbdb checks)
**	28-Feb-1989 (teg)
**	    Added msg constants for VERIFYDB phase IV (purge operation)
**      04-Mar-1989 (alexc)
**          Add new messages for starview.
**	    Error numbers:  0x1811 to 0x181b.
**      16-mar-1989 (alexc)
**          Merge with MAIN line, version 16.
**	03-May-1989 (teg)
**	    Merge Titan DUF into Mainline.
**	18-May-1989 (teg)
**	    Add new verifydb purge message (E_DU502D_NOFREE_PURGETABLE)
**	20-May-1989 (anton)
**	    Createdb collation error added
**	22-mar-1990 (linda)
**	    Added Gateway information & error messages.
**	12-jun-90 (teresa)
**	    added verifydb msg constants for checking of iisynonym catalog.
**	25-may-90 (edwin)
**	    One more Gateway msg.
**	20-jul-1990 (pete)
**	    Added new messages for 6.4.
**	31-Oct-90 (teresa)
**	    add S_DU1671_NOT_INDEX, S_DU1672_MISSING_IIINDEX,
**	    S_DU1673_NO_RULES_RELSTAT, E_DU5107_PRIVATE_STAR 
**      24-oct-90 (teresa)
**          new verifydb messages: S_DU033D_CLEAR_TCBINDEX, S_DU033E_SET_RULES,
**		S_DU038D_CLEAR_TCBINDEX, S_DU038E_SET_RULES, S_DU1671_NOT_INDEX,
**		S_DU1672_MISSING_IIINDEX, S_DU1673_NO_RULES_RELSTAT
**	    new cratedb message: E_DU5107_PRIVATE_STAR
**	    new finddbs messages: I_DU0031_NEWLOC_PROMPT_FI, 
**		I_DU0032_LOCAREA_PROMPT_FI, W_DU1049_LOCNAME_TOO_LONG_FI,
**		W_DU104A_AREANAME_TOO_LONG_FI, W_DU104B_DUP_LOCATION_FI,
**		W_DU104C_INVALID_LOC_FI
**	20-dec-90 (teresa)
**	    new finddbs message to improve fault tollerance:
**		W_DU104D_DILISTDIR_FAIL_FI
**	05-feb-91 (teresa)
**	    added W_DU1082_STAR_FECAT_WARN
**	20-feb-91 (teresa)
**	    added S_DU1674_QRYTEXT_RULE_MISSING and 
**	    S_DU1675_QRYTEXT_EVENT_MISSING
**	02-apr-91 (teresa)
**	    added E_DU_FATAL constant for error control
**	20-sep-91 (teresa)
**	    pick up the following 6.4 bugfixes/changes to pick up 6.4
**	    ingres63p changes 261241, 262607 and 263293
**	      05-jul-91 (teresa)
**		added W_DU1083, W_DU1084, E_DU2100-E_DU2105 to fix bug 38435.
**	      07-aug-91 (teresa) 
**		defined the following new UPGRADEDB message constants:
**		I_DU0080_UPGRADEDB_TERM to I_DU00B6_DB_IDENTIFIER,
**		W_DU1850_IIDBDB_ERR to W_DU1868_DESTROYED_DB,
**		E_DU6000_UPGRADEDB_USAGE to E_DU6017_CANT_OPEN_LOG.
**		also redefined missing message constant: 
**	08-oct-92 (robf)
**	    Added I_DU00CD_C2SXA_CRE and I_DU00CE_IIAUDIT for upgradedb
**      14-dec-92 (ed)
**          add E_DU3022_DB_UNIQUE_CR
**	14-dec-92 (jpk)
**	    add I_DU00CF_IIACCESS and I_DU00D0_IISCHEMA_CRE
**	21-dec-92 (robf)
**	    add W_DU1831_SUPUSR_OBSOLETE for no -s flag
**	10-jan-93 (jpk)
**	    add W_DU1085_SORTFLAG_DEPRECATED, W_DU1086_CANT_LOAD_PM_FILE
**	    W_DU1087_CANT_GET_HOSTNAME, and W_DU1088_PMSETDEFAULT_FAILED
**      29-Feb-1993 (jpk)
**	   Added ...
**	27-may-93 (teresa)
**	    Reserved a second set of message ranges for verifydb informational
**	    messages because first set is almost full.  The original set was:
**		DU0300 - DU034F - Suggested fixes
**		DU0350 - DU039F - Corresponding Actual fix notices
**	    The expansion set, for when this set fills up is:
**		DU0240 - DU029F - Suggested fixes.
**		DU0440 - DU049F - Corresponding Actual fix notices
**	    Also defined several new message constants for verifydb, to bring 
**	    verifydb up to a level to support 6.4 catalogs.
**	30-jul-93 (stephenb)
**	    Added errors for verifydb dbms_catalogs checks of iigw06_relation
**	    and iigw06_attribute.
**	10-oct-93 (anitap)
**	    defined new error message S_DU16A8_IIDBDEPENDS_MISSING
**	    FIPS constraints project. 
**	10-oct-93 (anitap)
**	    defined new error messages S_DU16A8_IIDBDEPENDS_MISSING & 
**	    S_DU16A9_DBDEPENDS_MISSING for FIPS constraints project.
**	    Corrected spelling of S_DU0320_DROP_FROM_IIINTERGRITY to
**	    S_DU0320_DROP_FROM_IIINTEGRITY.
**	    Also added S_DU0245_CREATE_SCHEMA and S_DU0445_CREATE_SCHEMA for
**	    CREATE SCHEMA project.
**	    Added S_DU0246_CREATE_CONSTRAINT & S_DU0446_CREATE_CONSTRAINT to
**	    create integrity after dropping the same.
**	10-nov-93 (rblumer)
**	    defined new error message I_DU0164_BAD_CASE_FLAGS;
**	    changed I_DU0160_BAD_CASE_FLAGS to I_DU0160_BAD_USERID_CASE.
**      16-dec-93 (andre)
**          defined S_DU16C0_NONEXISTENT_DEP_OBJ and
**          S_DU16C1_NONEXISTENT_INDEP_OBJ
**	20-dec-93 (robf)
**          Add more verifydb dbms messages
**	07-jan-94 (anitap)
**	    Removed define S_DU16A9_DBDEPENDS_MISSING and added define for
** 	    E_DU503B_EMPTY_IIPROCEDURE.
**	11-jan-94 (andre)
**	    defined S_DU16C2_UNEXPECTED_DEP_OBJ_TYPE and 
**	    S_DU16C3_UNEXPECTED_IND_OBJ_TYPE
**	18-jan-94 (andre)
**	    defined S_DU0247_DROP_CONSTRAINT, S_DU0447_DROP_CONSTRAINT,
**	    S_DU16C4_MISSING_INDEP_OBJ, S_DU16C5_MISSING_PERM_INDEP_OBJ
**	19-jan-94 (andre)
**	    defined S_DU0248_REDO_IIXPRIV, S_DU0249_INSERT_IIXPRIV_TUPLE,
**	    S_DU0448_REDO_IIXPRIV, S_DU0449_INSERTED_IIXPRIV_TUPLE, 
**	    S_DU16C7_EXTRA_IIXPRIV, S_DU16C8_MISSING_IIXPRIV,
**	    S_DU024A_REDO_IIXEVENT, S_DU024B_INSERT_IIXEVENT_TUPLE,
**	    S_DU044A_REDO_IIXEVENT, S_DU044B_INSERTED_IIXEVENT_TUPLE,
**	    S_DU16C9_EXTRA_IIXEVENT, S_DU16CA_MISSING_IIXEVENT,
**	    S_DU16CB_EXTRA_IIXPROCEDURE, S_DU16CC_MISSING_IIXPROCEDURE,
**	    S_DU024C_REDO_IIXPROC, S_DU024D_INSERT_IIXPROC_TUPLE,
**	    S_DU044C_REDO_IIXPROC, S_DU044D_INSERTED_IIXPROC_TUPLE,
**	    S_DU16CD_EXTRA_IIXPROTECT, S_DU16CE_MISSING_IIXPROTECT,
**	    S_DU024E_REDO_IIXPROT, S_DU024F_INSERT_IIXPROT_TUPLE, 
**	    S_DU044E_REDO_IIXPROT, S_DU044F_INSERTED_IIXPROT_TUPLE
**	20-jan-94 (andre)
**	    defined S_DU0250_DELETE_IIPRIV_TUPLE, S_DU0450_DELETED_IIPRIV_TUPLE,
**	    S_DU0251_MARK_DBP_DORMANT, S_DU0451_MARKED_DBP_DORMANT,
**	    S_DU16CF_BAD_OBJ_TYPE_IN_IIPRIV, S_DU16E0_NONEXISTENT_OBJ,
**	    S_DU16E1_NO_OBJ_DEPENDS_ON_DESCR, S_DU16E2_LACKING_INDEP_PRIV,
**	    S_DU16E3_LACKING_PERM_INDEP_PRIV, S_DU16E4_LACKING_CONS_INDEP_PRIV
**	21-jan-94 (andre)
**	    defined S_DU0252_SET_DB_DBP_INDEP_LIST, 
**	    S_DU0253_UBT_IS_A_DBMS_CATALOG, S_DU0452_SET_DB_DBP_INDEP_LIST,
**	    S_DU0453_CLEARED_DBP_UBT_ID, S_DU16E5_MISSING_INDEP_LIST,
**	    S_DU16E6_UNEXPECTED_INDEP_LIST, S_DU16E7_MISSING_UBT, 
**	    S_DU16E8_UBT_IS_A_CATALOG, S_DU16E9_UNEXPECTED_DBP_PERMS,
**	    S_DU16EA_UNEXPECTED_PERMOBJ_TYPE, S_DU16EB_NO_OBJECT_FOR_PERMIT,
**	    S_DU16EC_NAME_MISMATCH, S_DU16ED_INVALID_OPSET_FOR_OBJ
**	    S_DU0254_FIX_OBJNAME_IN_IIPROT, S_DU0454_FIXED_OBJNAME_IN_IIPROT
**	23-jan-94 (andre)
**	    defined S_DU16EE_INVALID_OPCTL_FOR_OBJ,
**	    S_DU16EF_QUEL_PERM_MARKED_AS_SQL, S_DU16F0_SQL_PERM_MARKED_AS_QUEL,
**	    S_DU0255_SET_PERM_LANG_TO_QUEL, S_DU0256_SET_PERM_LANG_TO_SQL,
**	    S_DU0455_SET_PERM_LANG_TO_QUEL, S_DU0456_SET_PERM_LANG_TO_SQL
**	24-jan-94 (andre)
**	    defined S_DU16F4_VIEW_NOT_ALWAYS_GRNTBL,
**	    S_DU0259_CLEAR_VGRANT_OK, S_DU0459_CLEARED_VGRANT_OK,
**	    S_DU16F5_MISSING_VIEW_INDEP_PRVS, S_DU16F1_MULT_PRIVS,
**	    S_DU0257_PERM_CREATED_BEFORE_10, S_DU0457_PERM_CREATED_BEFORE_10,
**	    S_DU16F2_PRE_10_PERMIT, S_DU16F3_LACKING_GRANT_OPTION,
**	    S_DU0258_DROP_DBEVENT, S_DU0458_DROPPED_DBEVENT
**	23-may-94 (jpk/kirke)
**	    Added defines for I_DU00D9_IIPRIV - I_DU00EF_UPG_SCHEMA.
**	11-jul-94 (robf)
**		I_DU00FC_IIEXTEND_INFO_CRE
**		I_DU00FB_IIDATABASE_INFO_CRE
**		I_DU00FA_IIROLEGRANTS_CRE
**		I_DU00F9_IILEVELAUDIT_CRE
**		I_DU00F8_IIROLES_CRE
**		I_DU00F7_IIPROFILES_CRE
**		I_DU00F6_IILABELAUDIT_CRE
**		I_DU00F5_IIPROFILE_CRE
**		I_DU00F4_IIROLEGRANT_CRE
**		I_DU00F3_IIPHYSICAL_COLUMNS_CRE
**		I_DU00F2_IISESSION_PRIVILEGES_CRE
**		I_DU00F1_IISECALARM_CRE
**		I_DU00F0_IILABELMAP_CRE
**	12-jul-94 (robf)
**           Added more messages for 1.1 ES upgradedb:
**              I_DU00FD_IIEVENT_CATALOG
**      21-mar-95 (chech02)
**           Added E_DU6019_CANT_UPGRADESQL92 for upgradedb.
**      07-jun-95 (chech02)
**           Added E_DU503C_TOO_MANY_VNODES for verifydb.
**	16-aug-95 (harpa06)
**	     Added E_DU351A_MULTI_DB_DESTRY for destroydb.
**	14-apr-97 (stephenb)
**	    Add message E_DU351B_INCOMPATIBLE_VERSION.
**	10-jul-97 (nanpr01)
**	    Add message E_DU503D_RUNMODE_INVALID,
**	    S_DU0422_DROP_UNIQUE_CONSTRAINT,S_DU0423_DROP_UNIQUE_CONSTRAINT,
**	    S_DU0424_DROP_CHECK_CONSTRAINT, S_DU0425_DROP_CHECK_CONSTRAINT,
**	    S_DU0426_DROP_REF_CONSTRAINT, S_DU0427_DROP_REF_CONSTRAINT,
**	    S_DU0428_DROP_PRIMARY_CONSTRAINT, S_DU0429_DROP_PRIMARY_CONSTRAINT,
**	    S_DU1711_UNIQUE_CONSTRAINT, S_DU1712_CHECK_CONSTRAINT, 
**	    S_DU1713_REFERENCES_CONSTRAINT, S_DU1714_PRIMARY_CONSTRAINT.
**      19-jan-2000 (stial01)
**          Added I_DU00FE_VERIFYING_TBL
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (hweho01)
**          To fix the parameter list mismatch problem, changed the function
**          prototype of du_error() to be a variable-arg list.
**	23-May-2001 (jenjo02)
**	    Added I_DU00FF_IIQEF_CHECK_LOC_AREA
**	15-Oct-2001 (jenjo02)
**	    Deleted short-lived I_DU00FF_IIQEF_CHECK_LOC_AREA. Area directories
**	    are now made by CREATE/ALTER LOCATION.
**      07-nov-2001 (stial01)
**          Added E_DU3303_BAD_PAGESIZE
**      09-jan-2002 (stial01)
**          New upgradedb messages for SIR 106745
**	11-jun-2004 (gupsh01)
**	    Added error messages for loadmdb utility.
**	31-Aug-2004 (schka24)
**	    Messages for r3 upgrade.
**	30-Dec-2004 (schka24)
**	    Remove iixprotect verifydb messages.
**	06-Feb-2004 (gupsh01)
**	    Added  E_DU6521_UNORM_NFC_WITH_NFD.
**      2-mar-2006 (stial01)
**          Added new messages for upgradedb
**	06-Aug-2007 (smeke01) b118877
**	    Added error message E_DU503E_INVALID_LOG_FILE
**      28-Feb-2008 (kibro01) b119427
**          Added S_DU16F6_INVALID_IIPROTECT
**	11-dec-2008 (stial01)
**	    Add message E_DU351C_INCOMPATIBLE_VERSION.
**      07-dec-2009 (joea)
**          Add E_DU241C_SELECT_ERR_UPGR_IIDEFAULT and
**          E_DU241D_INSERT_ERR_UPGR_IIDEFAULT.
**/

typedef	i4	DU_STATUS;

/*
[@function_reference@]...
*/


/*
**  Forward and/or External typedef/struct references.
*/


/*
[@type_reference@]...
*/


/*
**  Defines of other constants.
*/

/*
**      Define the possible statuses returned by system utility routines.
*/

#define			E_DU_OK		(DU_STATUS) 0
#define			E_DU_INFO	(DU_STATUS) 1
#define			E_DU_WARN	(DU_STATUS) 2
#define			E_DU_IERROR	(DU_STATUS) 3
#define			E_DU_UERROR	(DU_STATUS) 4
#define			E_DU_FATAL	(DU_STATUS) 5
/*
**    Define an error mask for the system utility routines.  This mask
**  was defined so that any error numbers returned by the system utilities
**  would not conflict with error numbers returned by the CL or with
**  any error numbers returned to the user in previous Ingres releases.
**  The F in "E_DUF_MASK" was added to aVOID namespace conflicts with
**  the CL def E_DU_MASK.
*/
#define			DU_MASK				0x0B
#define			E_DUF_MASK			(DU_MASK << 16)


/*
**      Define the upper boundaries for error type classifications.
*/

#define			DU_OK_BOUNDARY	    (E_DUF_MASK + 0x000FL)
#define			DU_INFO_BOUNDARY    (E_DUF_MASK + 0x0FFFL)
#define			DU_WARN_BOUNDARY    (E_DUF_MASK + 0x1FFFL)
#define			DU_IERROR_BOUNDARY  (E_DUF_MASK + 0x2FFFL)
#define			DU_UERROR_BOUNDARY  (E_DUF_MASK + 0xFFFFL)

/*  Define an error code that is mutually exclusive from all other possible
**  errors that can occur in the system utilities.
*/
#define			DU_NONEXIST_ERROR   -1


/* {@fix_me@} */
#define			DU_DEADLOCK_ERROR   4700


/*
**      Define the error codes that can be returned by the various
**  system utility routines.
*/

/* Information numbers range from 0x0010 to 0x0FFF */
#define			I_DU0010_UTIL_INTRO		(E_DUF_MASK + 0x0010L)
#define			I_DU0011_UTIL_SUCCESS		(E_DUF_MASK + 0x0011L)

/* FINDDBS informational numbers range from 0x0020 to 0x005F. */
#define			I_DU0020_FOUND_DB_FI		(E_DUF_MASK + 0x0020L)
#define			I_DU0021_FINDDBS_MODE_FI	(E_DUF_MASK + 0x0021L)
#define			I_DU0022_FINDDBS_INTRO_FI	(E_DUF_MASK + 0x0022L)
#define			I_DU0023_END_SEARCH_FI		(E_DUF_MASK + 0x0023L)
#define			I_DU0024_START_UPDATE_FI	(E_DUF_MASK + 0x0024L)
#define			I_DU0025_END_UPDATE_FI		(E_DUF_MASK + 0x0025L)
#define			I_DU0026_START_ANALYZE_FI	(E_DUF_MASK + 0x0026L)
#define			I_DU0027_END_ANALYZE_FI		(E_DUF_MASK + 0x0027L)
    /*
    ** note, I_DU0028_PROMPT_FORLOC_FI is being retired for 6.3/03 and above.
    ** this is left in this file as doucmentation to make porting folks jobs
    ** a hair easier.
    */
#define			I_DU0028_PROMPT_FORLOC_FI	(E_DUF_MASK + 0x0028L)
#define			I_DU0029_SCAN_AREA_FI		(E_DUF_MASK + 0x0029L)
#define			I_DU002A_SAME_DBNAME_FI		(E_DUF_MASK + 0x002AL)
#define			I_DU002B_ALL_REALDBS_FOUND_FI	(E_DUF_MASK + 0x002BL)
#define			I_DU002C_ALL_DBTUPS_FOUND_FI	(E_DUF_MASK + 0x002CL)
#define			I_DU002D_DBTUP_APPEND_FI	(E_DUF_MASK + 0x002DL)
#define			I_DU002E_USAGE_FI		(E_DUF_MASK + 0x002EL)
#define			I_DU002F_SUCCESS_FI		(E_DUF_MASK + 0x002FL)
#define			I_DU0030_ANLZ_SUCCESS_FI	(E_DUF_MASK + 0x0030L)
#define                 I_DU0031_NEWLOC_PROMPT_FI       (E_DUF_MASK + 0x0031L)
#define                 I_DU0032_LOCAREA_PROMPT_FI      (E_DUF_MASK + 0x0032L)

/* CONVTO60 informational numbers range from 0x0060 to 0x007A. */
#define			I_DU0061_CONVDB_INTRO		(E_DUF_MASK + 0x0061L)
#define			I_DU0062_DBMS60_OK		(E_DUF_MASK + 0x0062L)

/* upgradedb informational numbers range from 0x0080 to 0x00df */
#define			I_DU0080_UPGRADEDB_TERM		(E_DUF_MASK + 0x0080L)
#define			I_DU0081_UPGRADEDB_DONE		(E_DUF_MASK + 0x0081L)
#define			I_DU0082_UPGRADEDB_BEGIN	(E_DUF_MASK + 0x0082L)
#define			I_DU0083_COMPAT_INFO		(E_DUF_MASK + 0x0083L)
#define			I_DU0084_COMPAT_INFO		(E_DUF_MASK + 0x0084L)
#define			I_DU0085_IIDBDB_CONNECT		(E_DUF_MASK + 0x0085L)
#define			I_DU0086_SUPERUSER		(E_DUF_MASK + 0x0086L)
#define			I_DU0087_NOTSUPERUSER		(E_DUF_MASK + 0x0087L)
#define			I_DU0088_UPGRADE_ID		(E_DUF_MASK + 0x0088L)
#define			I_DU0089_DB_UPGRADED		(E_DUF_MASK + 0x0089L)
#define			I_DU008A_DB_NOT_UPGRADED	(E_DUF_MASK + 0x008AL)
#define			I_DU008B_UPGRADEDB_BYPASSED	(E_DUF_MASK + 0x008BL)
#define			I_DU008C_COLLECT_DBS		(E_DUF_MASK + 0x008CL)
#define			I_DU008D_UPGRADE_TARGET		(E_DUF_MASK + 0x008DL)
#define			I_DU008E_USER_DB_CONNECT	(E_DUF_MASK + 0x008EL)
#define			I_DU008F_NOT_UPGRADED		(E_DUF_MASK + 0x008FL)
#define			I_DU0090_IS_UPGRADED		(E_DUF_MASK + 0x0090L)
#define			I_DU0091_IIUSER_CATALOG		(E_DUF_MASK + 0x0091L)
#define			I_DU0092_CONVERTING_IIUSER	(E_DUF_MASK + 0x0092L)
#define			I_DU0093_IIUSERGROUP_EXISTS	(E_DUF_MASK + 0x0093L)
#define			I_DU0094_CRE_IIUSERGROUP	(E_DUF_MASK + 0x0094L)
#define			I_DU0095_IIROLE_EXISTS		(E_DUF_MASK + 0x0095L)
#define			I_DU0096_CRE_IIROLE		(E_DUF_MASK + 0x0096L)
#define			I_DU0097_IISECRYSTATE_EXISTS	(E_DUF_MASK + 0x0097L)
#define			I_DU0098_CRE_IISECURITYSTATE	(E_DUF_MASK + 0x0098L)
#define			I_DU009A_IIDBPRIV_EXISTS	(E_DUF_MASK + 0x009AL)
#define			I_DU009B_CRE_IIDBPRIV		(E_DUF_MASK + 0x009BL)
#define			I_DU009C_CRE_IIDBPRIVILEGES	(E_DUF_MASK + 0x009CL)
#define			I_DU009D_IIDATABASE_CURRENT	(E_DUF_MASK + 0x009DL)
#define			I_DU009E_UPG_IIDATABASE		(E_DUF_MASK + 0x009EL)
#define			I_DU009F_APPEND_DUMP_LOC	(E_DUF_MASK + 0x009FL)
#define			I_DU00A0_UPDATING_CAPS		(E_DUF_MASK + 0x00A0L)
#define			I_DU00A1_IIPROTECT_CURRENT	(E_DUF_MASK + 0x00A1L)
#define			I_DU00A2_UPG_IIPROTECT		(E_DUF_MASK + 0x00A2L)
#define			I_DU00A3_RECRE_IIPERMITS	(E_DUF_MASK + 0x00A3L)
#define			I_DU00A4_IIINTEGRITY_CURRENT	(E_DUF_MASK + 0x00A4L)
#define			I_DU00A5_UPG_IIINTEGRITY	(E_DUF_MASK + 0x00A5L)
#define			I_DU00A6_IIRULE_EXISTS		(E_DUF_MASK + 0x00A6L)
#define			I_DU00A7_CRE_RULES_CATS		(E_DUF_MASK + 0x00A7L)
#define			I_DU00A8_CRE_IIEVENT		(E_DUF_MASK + 0x00A8L)
#define			I_DU00A9_CRE_IIEVENTS		(E_DUF_MASK + 0x00A9L)
#define			I_DU00AA_EVENTS_CATS		(E_DUF_MASK + 0x00AAL)
#define			I_DU00AB_UPG_IICOLUMNS		(E_DUF_MASK + 0x00ABL)
#define			I_DU00AC_UPG_IIALTCOLUMNS	(E_DUF_MASK + 0x00ACL)
#define			I_DU00AD_UPG_IIFILE_INFO	(E_DUF_MASK + 0x00ADL)
#define			I_DU00AE_RECRE_IITABLES		(E_DUF_MASK + 0x00AEL)
#define			I_DU00AF_RECRE_IIINDEXES	(E_DUF_MASK + 0x00AFL)
#define			I_DU00B0_RECRE_IIPHYS_TBLS  	(E_DUF_MASK + 0x00B0L)
#define			I_DU00B1_RECRE_IIDBCONSTANTS	(E_DUF_MASK + 0x00B1L)
#define			I_DU00B2_RECRE_INDEX_COLS	(E_DUF_MASK + 0x00B2L)
#define			I_DU00B3_TRACE_01		(E_DUF_MASK + 0x00B3L)
#define			I_DU00B4_FE_UPGRADE		(E_DUF_MASK + 0x00B4L)
#define			I_DU00B5_TRACE_POINT		(E_DUF_MASK + 0x00B5L)
#define			I_DU00B6_DB_IDENTIFIER		(E_DUF_MASK + 0x00B6L)
#define			I_DU00B7_IIDBPRIV_UPDATE	(E_DUF_MASK + 0x00B7L)
#define			I_DU00B8_IIDBMS_COMMENT_CRE	(E_DUF_MASK + 0x00B8L)
#define			I_DU00B9_IISECTYPE_CRE		(E_DUF_MASK + 0x00B9L)
#define			I_DU00BA_IISYNONYM_CRE		(E_DUF_MASK + 0x00BAL)
#define			I_DU00BB_IISECURITY_ALARMS	(E_DUF_MASK + 0x00BBL)
#define			I_DU00BC_IIDB_COMMENTS_CRE	(E_DUF_MASK + 0x00BCL)
#define			I_DU00BD_IISYNONYMS_CRE		(E_DUF_MASK + 0x00BDL)
#define			I_DU00BE_IISECURITY_STATUS	(E_DUF_MASK + 0x00BEL)
#define			I_DU00BF_IIQEF_CREATE_DB	(E_DUF_MASK + 0x00BFL)
#define			I_DU00C0_IIQEF_ALTER_DB		(E_DUF_MASK + 0x00C0L)
#define			I_DU00C1_IIQEF_DESTROY_DB	(E_DUF_MASK + 0x00C1L)
#define			I_DU00C2_IIQEF_ADD_LOCATIONS	(E_DUF_MASK + 0x00C2L)
#define			I_DU00C3_IISECURITYSTATE	(E_DUF_MASK + 0x00C3L)
#define			I_DU00C4_IIUSERS_CRE		(E_DUF_MASK + 0x00C4L)
#define			I_DU00C5_IILOCATION_INFO	(E_DUF_MASK + 0x00C5L)
#define			I_DU00C6_IISECURITY_STATE	(E_DUF_MASK + 0x00C6L)
#define			I_DU00C7_UPDT_IIDBPRIV		(E_DUF_MASK + 0x00C7L)
#define			I_DU00C8_PRIV_DB_CURRENT	(E_DUF_MASK + 0x00C8L)
#define			I_DU00C9_STAR_STDCAT_CURRENT	(E_DUF_MASK + 0x00C9L)
#define			I_DU00CA_CRE_IIREGISTERED_OBJ	(E_DUF_MASK + 0x00CAL)
#define			I_DU00CB_CRE_STAR_CATS		(E_DUF_MASK + 0x00CBL)
#define			I_DU00CC_CRE_STAR_STDCATS	(E_DUF_MASK + 0x00CCL)
#define			I_DU00CD_C2SXA_CRE		(E_DUF_MASK + 0x00CDL)
#define			I_DU00CE_IIAUDIT		(E_DUF_MASK + 0x00CEL)
#define			I_DU00CF_IIACCESS		(E_DUF_MASK + 0x00CFL)
#define			I_DU00D0_IISCHEMA_CRE		(E_DUF_MASK + 0x00D0L)
#define			I_DU00D1_IIQEF_ADD_EXTENSIONS	(E_DUF_MASK + 0x00D1L)
#define			I_DU00D2_IIPROCPARAM_CRE	(E_DUF_MASK + 0x00D2L)
#define			I_DU00D3_IIDEFAULT_CRE		(E_DUF_MASK + 0x00D3L)
#define			I_DU00D4_IIKEY_CRE		(E_DUF_MASK + 0x00D4L)
#define			I_DU00D5_IIKEYS			(E_DUF_MASK + 0x00D5L)
#define			I_DU00D6_IICONSTRAINTS		(E_DUF_MASK + 0x00D6L)
#define			I_DU00D7_IIREF_CONSTRAINTS	(E_DUF_MASK + 0x00D7L)
#define			I_DU00D8_IICONSTRAINT_INDEXES	(E_DUF_MASK + 0x00D8L)
#define			I_DU00D9_IIPRIV			(E_DUF_MASK + 0x00D9L)
#define			I_DU00DA_MODIFY_CORE		(E_DUF_MASK + 0x00DAL)
#define			I_DU00DB_PREPARE_QT		(E_DUF_MASK + 0x00DBL)
#define			I_DU00DC_UPGR_BOOT_CAT		(E_DUF_MASK + 0x00DCL)
#define			I_DU00DD_IIINTEGRITY		(E_DUF_MASK + 0x00DDL)
#define			I_DU00DE_IIDBDEPENDS		(E_DUF_MASK + 0x00DEL)
#define			I_DU00DF_IIDEVICES		(E_DUF_MASK + 0x00DFL)
#define			I_DU00E0_IIPROCEDURE		(E_DUF_MASK + 0x00E0L)
#define			I_DU00E1_IISTATISTICS		(E_DUF_MASK + 0x00E1L)
#define			I_DU00E2_FILE_EXTEND_INGRES	(E_DUF_MASK + 0x00E2L)
#define			I_DU00E3_FILE_EXTEND_TBL	(E_DUF_MASK + 0x00E3L)
#define			I_DU00E4_MAKE_UDT_DEFAULTS	(E_DUF_MASK + 0x00E4L)
#define			I_DU00E5_FILE_EXTEND_USER	(E_DUF_MASK + 0x00E5L)
#define			I_DU00E6_FILE_EXTEND_TBL_ERR	(E_DUF_MASK + 0x00E6L)
#define			I_DU00E7_CONVERTING_TBL		(E_DUF_MASK + 0x00E7L)
#define			I_DU00E8_UPGRADING_VIEWS	(E_DUF_MASK + 0x00E8L)
#define			I_DU00E9_UPG_INGRES_VIEW	(E_DUF_MASK + 0x00E9L)
#define			I_DU00EA_UPG_USER_VIEW		(E_DUF_MASK + 0x00EAL)
#define			I_DU00EB_UPGRADING_PERMITS	(E_DUF_MASK + 0x00EBL)
#define			I_DU00EC_UPGRADING_INTEGRITIES	(E_DUF_MASK + 0x00ECL)
#define			I_DU00ED_UPGRADING_RULES	(E_DUF_MASK + 0x00EDL)
#define			I_DU00EF_UPG_SCHEMA		(E_DUF_MASK + 0x00EFL)
#define			I_DU00F0_IILABELMAP_CRE		(E_DUF_MASK + 0x00F0L)
#define			I_DU00F1_IISECALARM_CRE		(E_DUF_MASK + 0x00F1L)
#define			I_DU00F2_IISESSION_PRIVILEGES_CRE (E_DUF_MASK + 0x00F2L)
#define			I_DU00F3_IIPHYSICAL_COLUMNS_CRE	 (E_DUF_MASK + 0x00F3L)
#define			I_DU00F4_IIROLEGRANT_CRE	(E_DUF_MASK + 0x00F4L)
#define			I_DU00F5_IIPROFILE_CRE		(E_DUF_MASK + 0x00F5L)
#define			I_DU00F6_IILABELAUDIT_CRE	(E_DUF_MASK + 0x00F6L)
#define			I_DU00F7_IIPROFILES_CRE		(E_DUF_MASK + 0x00F7L)
#define			I_DU00F8_IIROLES_CRE		(E_DUF_MASK + 0x00F8L)
#define			I_DU00F9_IILEVELAUDIT_CRE	(E_DUF_MASK + 0x00F9L)
#define			I_DU00FA_IIROLEGRANTS_CRE	(E_DUF_MASK + 0x00FAL)
#define			I_DU00FB_IIDATABASE_INFO_CRE	(E_DUF_MASK + 0x00FBL)
#define			I_DU00FC_IIEXTEND_INFO_CRE	(E_DUF_MASK + 0x00FCL)
#define			I_DU00FD_IIEVENT_CATALOG        (E_DUF_MASK + 0x00FDL)
#define                 I_DU00FE_VERIFYING_TBL          (E_DUF_MASK + 0x00FEL)
#define			I_DU00FF_IISEQUENCE		(E_DUF_MASK + 0x00FFL)


/* SYSMOD informational numbers range from 0x0100 to 0x015F. */
/* Some are upgradedb too, upgradedb ran out of numbers */
#define			I_DU0100_MODIFYING_SY		(E_DUF_MASK + 0x0100L)
#define			I_DU0101_PARTITION_CRE		(E_DUF_MASK + 0x0101L)
#define			I_DU0102_DROPPING_STDCAT_VIEWS	(E_DUF_MASK + 0x0102L)
#define			I_DU0103_RECREATING_STDCAT_VIEWS  (E_DUF_MASK + 0x0103L)
#define			I_DU0104_UNLOADING_TREES	(E_DUF_MASK + 0x0104L)
#define			I_DU0105_RELOADING_TREES	(E_DUF_MASK + 0x0105L)
#define			I_DU0106_FIXING_64_GRANTS	(E_DUF_MASK + 0x0106L)
#define			I_DU0107_FIXING_CAT_GRANTS	(E_DUF_MASK + 0x0107L)
#define			I_DU0108_MODIFYING_PGSIZE	(E_DUF_MASK + 0x0108L)
#define			I_DU0109_USER_DB_RECONNECT	(E_DUF_MASK + 0x0109L)
#define			I_DU0110_DROPPING_REF_CONS	(E_DUF_MASK + 0x0110L)
#define			I_DU0111_RECREATING_REF_CONS	(E_DUF_MASK + 0x0111L)
#define			I_DU0112_DROPPING_CONS		(E_DUF_MASK + 0x0112L)
#define			I_DU0113_RECREATING_CONS	(E_DUF_MASK + 0x0113L)
#define			I_DU0114_CONVERTING_STAR	(E_DUF_MASK + 0x0114L)


/* CREATEDB information numbers range from 0x0160 to 0x01AF. */
#define			I_DU0160_BAD_USERID_CASE	(E_DUF_MASK + 0x0160L)
#define			I_DU0161_USERID_CASE		(E_DUF_MASK + 0x0161L)
#define			I_DU0162_INSTALLATION_VALUE	(E_DUF_MASK + 0x0162L)
#define			I_DU0163_CMD_LINE_OVERRIDE	(E_DUF_MASK + 0x0163L)
#define			I_DU0164_BAD_CASE_FLAGS		(E_DUF_MASK + 0x0164L)


/* DESTROYDB information numbers range from 0x01B0 to 0x01FF. */

/* GENERAL DUF information numbers range from 0x0200 to 0x023F. */
#define			I_DU0200_CR_CORE_CATS		(E_DUF_MASK + 0x0200L)
#define			I_DU0201_CR_DBMS_CATS		(E_DUF_MASK + 0x0201L)
#define			I_DU0202_CR_DBDB_CATS		(E_DUF_MASK + 0x0202L)
#define			I_DU0203_MOD_DBMS_CATS		(E_DUF_MASK + 0x0203L)
#define			I_DU0204_MOD_DBDB_CATS		(E_DUF_MASK + 0x0204L)
#define			I_DU0205_CR_FE_CATS		(E_DUF_MASK + 0x0205L)
#define			I_DU0206_CR_SCI_CATS		(E_DUF_MASK + 0x0206L)
#define			I_DU0207_MOD_FE_CATS		(E_DUF_MASK + 0x0207L)




/* VERIFYDB informational numbers range from 0x0240 to 0x4ff. */

    /* DU0240 - DU029F: 
    ** 2nd set of verifydb catalog check operation -- suggested fixes
    **	    (use range 440 - 490 for corresponding actual fix notices)
    **
    **	     A 1-to-1 correspondence between suggested and actual fixes,
    **	     Using the same message constant except that the actual fix constant
    **	     is 200 HEX higher than the suggested fix.
    **	     Example:
    **		S_DU0240_FIX_A	- suggested fix A
    **		S_DU0440_FIX_A	- notice that fix A was performed
    **	      or
    **		S_DU0241_FIX_B	- suggested fix B
    **		S_DU0441_FIX_B	- notice that fix B was performed
    **
    **	NOTE: this range of number should not be used until the 1st range
    **	      (300-34F for suggested fix, 350-3FF for acutal fix) fills up
    */
#define			S_DU0240_SET_COMMENT		(E_DUF_MASK + 0x0240L)
#define			S_DU0241_DROP_THIS_COMMENT	(E_DUF_MASK + 0x0241L)
#define			S_DU0242_SET_DELIM_MIXED	(E_DUF_MASK + 0x0242L)
#define			S_DU0243_CLEAR_DELIM_UPPER	(E_DUF_MASK + 0x0243L)
#define			S_DU0244_CLEAR_REALUSER_MIXED	(E_DUF_MASK + 0x0244L)
#define			S_DU0245_CREATE_SCHEMA		(E_DUF_MASK + 0x0245L)
#define			S_DU0246_CREATE_CONSTRAINT	(E_DUF_MASK + 0x0246L)
#define			S_DU0247_DROP_CONSTRAINT        (E_DUF_MASK + 0x0247L)
#define			S_DU0248_DELETE_IIXPRIV		(E_DUF_MASK + 0x0248L)
#define			S_DU0249_INSERT_IIXPRIV_TUPLE	(E_DUF_MASK + 0x0249L)
#define			S_DU024A_DELETE_IIXEVENT	(E_DUF_MASK + 0x024AL)
#define			S_DU024B_INSERT_IIXEVENT_TUPLE	(E_DUF_MASK + 0x024BL)
#define			S_DU024C_DELETE_IIXPROC		(E_DUF_MASK + 0x024CL)
#define			S_DU024D_INSERT_IIXPROC_TUPLE	(E_DUF_MASK + 0x024DL)
/* 24E, 24F obsolete */
#define			S_DU0250_DELETE_IIPRIV_TUPLE	(E_DUF_MASK + 0x0250L)
#define			S_DU0251_MARK_DBP_DORMANT	(E_DUF_MASK + 0x0251L)
#define			S_DU0252_SET_DB_DBP_INDEP_LIST	(E_DUF_MASK + 0x0252L)
#define			S_DU0253_UBT_IS_A_DBMS_CATALOG	(E_DUF_MASK + 0x0253L)
#define			S_DU0254_FIX_OBJNAME_IN_IIPROT	(E_DUF_MASK + 0x0254L)
#define			S_DU0255_SET_PERM_LANG_TO_QUEL	(E_DUF_MASK + 0x0255L)
#define			S_DU0256_SET_PERM_LANG_TO_SQL 	(E_DUF_MASK + 0x0256L)
#define			S_DU0257_PERM_CREATED_BEFORE_10	(E_DUF_MASK + 0x0257L)
#define			S_DU0258_DROP_DBEVENT		(E_DUF_MASK + 0x0258L)
#define			S_DU0259_CLEAR_VGRANT_OK	(E_DUF_MASK + 0x0259L)

    /* Reserved for Verifydb Future Growth:  DU02A0 - DU02FF */

    /* 1st set of verifydb catalog check operation -- suggested fixes.
    **
    ** NOTE:  the corresponding messages of actual fixes are DU0350-DU03F9
    **	      There is a 1-1 correspondence between suggested fixes and actual
    **	      fix notices.  The message constant is the same except that the
    **	      actual fix notice is 50Hex higher.  Example:
    **		    S_DU0301_DROP_IIRELATION_TUPLE - suggestion to drop tuple
    **		    S_DU0351_DROP_IIRELATION_TUPLE - notice tuple was dropped
    **
    ** NOTE: when this range fills up, use the second set of numbers, with
    **		- suggested fixes from 240-29f
    **		- actual fix notices from 440-49f
    **	     A 1 to 1 correspondence between suggested and actual fixes.
    **	     Example:
    **		S_DU0240_FIX_A	- suggested fix a
    **		S_DU0440_FIX_A	- notice that fix A was performed
    */
#define			S_DU0300_PROMPT			(E_DUF_MASK + 0x0300L)
#define			S_DU0301_DROP_IIRELATION_TUPLE	(E_DUF_MASK + 0x0301L)
#define			S_DU0302_DROP_TABLE		(E_DUF_MASK + 0x0302L)
#define			S_DU0303_REPLACE_RELWID		(E_DUF_MASK + 0x0303L)
#define			S_DU0304_REPLACE_RELKEYS        (E_DUF_MASK + 0x0304L)
#define			S_DU0305_CLEAR_PRTUPS		(E_DUF_MASK + 0x0305L)
#define			S_DU0306_CLEAR_INTEGS		(E_DUF_MASK + 0x0306L)
#define			S_DU0307_CLEAR_CONCUR		(E_DUF_MASK + 0x0307L)
#define			S_DU0308_SET_CONCUR		(E_DUF_MASK + 0x0308L)
#define			S_DU0309_MAKE_TABLE		(E_DUF_MASK + 0x0309L)
#define			S_DU030A_CLEAR_CATALOG		(E_DUF_MASK + 0x030AL)
#define			S_DU030B_DROP_VIEW		(E_DUF_MASK + 0x030BL)
#define			S_DU030C_CLEAR_VBASE		(E_DUF_MASK + 0x030CL)
#define			S_DU030D_SET_IDXD		(E_DUF_MASK + 0x030DL)
#define			S_DU030E_FIX_RELIDXCOUNT	(E_DUF_MASK + 0x030EL)
#define			S_DU030F_CLEAR_RELIDXCOUNT	(E_DUF_MASK + 0x030FL)
#define			S_DU0310_CLEAR_OPTSTAT		(E_DUF_MASK + 0x0310L)
#define			S_DU0311_RESET_RELLOCOUNT	(E_DUF_MASK + 0x0311L)
#define			S_DU0312_FIX_RELDFILL		(E_DUF_MASK + 0x0312L)
#define			S_DU0313_FIX_RELLFILL		(E_DUF_MASK + 0x0313L)
#define			S_DU0314_FIX_RELIFILL		(E_DUF_MASK + 0x0314L)
#define			S_DU0315_CREATE_IIRELIDX	(E_DUF_MASK + 0x0315L)
#define			S_DU0316_CLEAR_IIRELIDX		(E_DUF_MASK + 0x0316L)
#define			S_DU0317_DROP_ATTRIBUTES	(E_DUF_MASK + 0x0317L)
#define			S_DU0318_DROP_INDEX		(E_DUF_MASK + 0x0318L)
#define			S_DU0319_SET_INDEX		(E_DUF_MASK + 0x0319L)
#define			S_DU031A_DROP_DEVRELID		(E_DUF_MASK + 0x031AL)
#define			S_DU031B_SET_MULTIPLE_LOC	(E_DUF_MASK + 0x031BL)
#define			S_DU031C_DEL_TREE		(E_DUF_MASK + 0x031CL)
#define			S_DU031D_SET_VIEW		(E_DUF_MASK + 0x031DL)
#define			S_DU031E_SET_PRTUPS		(E_DUF_MASK + 0x031EL)
#define			S_DU031F_SET_INTEGS		(E_DUF_MASK + 0x031FL)
#define			S_DU0320_DROP_FROM_IIINTEGRITY	(E_DUF_MASK + 0x0320L)
#define			S_DU0321_DROP_FROM_IIPROTECT	(E_DUF_MASK + 0x0321L)
#define			S_DU0322_DROP_IIDBDEPENDS_TUPLE	(E_DUF_MASK + 0x0322L)
#define			S_DU0323_SET_VBASE		(E_DUF_MASK + 0x0323L)
#define			S_DU0324_CREATE_IIXDBDEPENDS	(E_DUF_MASK + 0x0324L)
#define			S_DU0325_REDO_IIXDBDEPENDS	(E_DUF_MASK + 0x0325L)
#define			S_DU0326_DROP_FROM_IISTATISTICS	(E_DUF_MASK + 0x0326L)
#define			S_DU0327_SET_OPTSTAT		(E_DUF_MASK + 0x0327L)
#define			S_DU0328_FIX_SNUMCELLS		(E_DUF_MASK + 0x0328L)
#define			S_DU0329_DROP_FROM_IIHISTOGRAM	(E_DUF_MASK + 0x0329L)
#define			S_DU032A_SET_RELSPEC_TO_HASH	(E_DUF_MASK + 0x032AL)
#define			S_DU032B_SET_RELSPEC_TO_BTREE	(E_DUF_MASK + 0x032BL)
#define			S_DU032C_EMPTY_IIHISTOGRAM	(E_DUF_MASK + 0x032CL)
#define			S_DU032D_DROP_TABLE		(E_DUF_MASK + 0x032DL)
#define			S_DU032E_DROP_USER		(E_DUF_MASK + 0x032EL)
#define			S_DU032F_CLEAR_BADBITS		(E_DUF_MASK + 0x032FL)
#define			S_DU0330_DROP_IIDATABASE_TUPLE	(E_DUF_MASK + 0x0330L)
#define			S_DU0331_CREATE_IIEXTEND_ENTRY	(E_DUF_MASK + 0x0331L)
#define			S_DU0332_CREATE_DBACCESS_ENTRY	(E_DUF_MASK + 0x0332L)
#define			S_DU0333_MARK_DB_OPERATIVE	(E_DUF_MASK + 0x0333L)
#define			S_DU0334_UPDATE_IIDBID_IDX	(E_DUF_MASK + 0x0334L)
#define			S_DU0335_DROP_IIDBIDIDX		(E_DUF_MASK + 0x0335L)
#define			S_DU0336_UPDATE_IIDBIDIDX	(E_DUF_MASK + 0x0336L)
#define			S_DU0337_DROP_DBACCESS		(E_DUF_MASK + 0x0337L)
#define			S_DU0338_MARK_DB_PRIVATE	(E_DUF_MASK + 0x0338L)
#define			S_DU0339_DROP_LOCATION		(E_DUF_MASK + 0x0339L)
#define			S_DU033A_FIX_LOC_STAT		(E_DUF_MASK + 0x033AL)
#define			S_DU033B_DROP_IIEXTEND		(E_DUF_MASK + 0x033BL)
#define			S_DU033C_DROP_IISYNONYM		(E_DUF_MASK + 0x033CL)
#define                 S_DU033D_CLEAR_TCBINDEX         (E_DUF_MASK + 0x033DL)
#define                 S_DU033E_SET_RULES              (E_DUF_MASK + 0x033EL)
#define                 S_DU033F_REMOVE_STARCDBS        (E_DUF_MASK + 0x033FL)
#define                 S_DU0340_UPDATE_STARCDBS	(E_DUF_MASK + 0x0340L)
#define                 S_DU0341_WARN_IICDBID_IDX	(E_DUF_MASK + 0x0341L)
#define			S_DU0342_CLEAR_RULE		(E_DUF_MASK + 0x0342L)
#define			S_DU0343_DROP_EVENT		(E_DUF_MASK + 0x0343L)
#define			S_DU0344_DROP_DBP		(E_DUF_MASK + 0x0344L)
#define			S_DU0345_DROP_FROM_IIRULE	(E_DUF_MASK + 0x0345L)
#define			S_DU0346_SET_RULE		(E_DUF_MASK + 0x0346L)
#define			S_DU0347_CLEAR_DEFLT_GRP	(E_DUF_MASK + 0x0347L)
#define			S_DU0348_DROP_USR_FROM_GRP	(E_DUF_MASK + 0x0348L)
#define			S_DU0349_DROP_FROM_IIDBPRIV	(E_DUF_MASK + 0x0349L)
#define			S_DU034A_CLEAR_INVALID_PRIV	(E_DUF_MASK + 0x034AL)
#define			S_DU034B_DELETE_IIDEFAULT	(E_DUF_MASK + 0x034BL)
#define			S_DU034C_SET_INDEX		(E_DUF_MASK + 0x034CL)
#define			S_DU034D_DROP_IIGW06_RELATION	(E_DUF_MASK + 0x034DL)
#define			S_DU034E_DROP_IIGW06_ATTRIBUTE	(E_DUF_MASK + 0x034EL)
#define			S_DU034F_DROP_IIDBMS_COMMENT	(E_DUF_MASK + 0x034FL)


    /* 1st set of verifydb catalog check operation -- implemented fixes 
    ** DU0350-DU039F
    **
    ** NOTE:  the corresponding messages of suggested fixes are DU0300-DU034F
    **	      There is a 1-1 correspondence between suggested fixes and actual
    **	      fix notices.  The message constant is the same except that the
    **	      actual fix notice is 50Hex higher than the suggexted fix.  
    **	      Example:
    **		    S_DU0301_DROP_IIRELATION_TUPLE - suggestion to drop tuple
    **		    S_DU0351_DROP_IIRELATION_TUPLE - notice tuple was dropped
    **
    ** NOTE: when this range fills up, use the second set of numbers, with
    **		- suggested fixes from 240-29f
    **		- actual fix notices from 440 to 49f
    **	     A 1 to 1 correspondence between suggested and actual fixes.
    **	     Example:
    **		S_DU0240_FIX_A	- suggested fix a
    **		S_DU0440_FIX_A	- notice that fix A was performed
    */

#define			S_DU0351_DROP_IIRELATION_TUPLE	(E_DUF_MASK + 0x0351L)
#define			S_DU0352_DROP_TABLE		(E_DUF_MASK + 0x0352L)
#define			S_DU0353_REPLACE_RELWID		(E_DUF_MASK + 0x0353L)
#define			S_DU0354_REPLACE_RELKEYS        (E_DUF_MASK + 0x0354L)
#define			S_DU0355_CLEAR_PRTUPS		(E_DUF_MASK + 0x0355L)
#define			S_DU0356_CLEAR_INTEGS		(E_DUF_MASK + 0x0356L)
#define			S_DU0357_CLEAR_CONCUR		(E_DUF_MASK + 0x0357L)
#define			S_DU0358_SET_CONCUR		(E_DUF_MASK + 0x0358L)
#define			S_DU0359_MAKE_TABLE		(E_DUF_MASK + 0x0359L)
#define			S_DU035A_CLEAR_CATALOG		(E_DUF_MASK + 0x035AL)
#define			S_DU035B_DROP_VIEW		(E_DUF_MASK + 0x035BL)
#define			S_DU035C_CLEAR_VBASE		(E_DUF_MASK + 0x035CL)
#define			S_DU035D_SET_IDXD		(E_DUF_MASK + 0x035DL)
#define			S_DU035E_FIX_RELIDXCOUNT	(E_DUF_MASK + 0x035EL)
#define			S_DU035F_CLEAR_RELIDXCOUNT	(E_DUF_MASK + 0x035FL)
#define			S_DU0360_CLEAR_OPTSTAT		(E_DUF_MASK + 0x0360L)
#define			S_DU0361_RESET_RELLOCOUNT	(E_DUF_MASK + 0x0361L)
#define			S_DU0362_FIX_RELDFILL		(E_DUF_MASK + 0x0362L)
#define			S_DU0363_FIX_RELLFILL		(E_DUF_MASK + 0x0363L)
#define			S_DU0364_FIX_RELIFILL		(E_DUF_MASK + 0x0364L)
#define			S_DU0365_CREATE_IIRELIDX	(E_DUF_MASK + 0x0365L)
#define			S_DU0366_CLEAR_IIRELIDX		(E_DUF_MASK + 0x0366L)
#define			S_DU0367_DROP_ATTRIBUTES	(E_DUF_MASK + 0x0367L)
#define			S_DU0368_DROP_INDEX		(E_DUF_MASK + 0x0368L)
#define			S_DU0369_SET_INDEX		(E_DUF_MASK + 0x0369L)
#define			S_DU036A_DROP_DEVRELID		(E_DUF_MASK + 0x036AL)
#define			S_DU036B_SET_MULTIPLE_LOC	(E_DUF_MASK + 0x036BL)
#define			S_DU036C_DEL_TREE		(E_DUF_MASK + 0x036CL)
#define			S_DU036D_SET_VIEW		(E_DUF_MASK + 0x036DL)
#define			S_DU036E_SET_PRTUPS		(E_DUF_MASK + 0x036EL)
#define			S_DU036F_SET_INTEGS		(E_DUF_MASK + 0x036FL)
#define			S_DU0370_DROP_FROM_IIINTEGRITY	(E_DUF_MASK + 0x0370L)
#define			S_DU0371_DROP_FROM_IIPROTECT	(E_DUF_MASK + 0x0371L)
#define			S_DU0372_DROP_IIDBDEPENDS_TUPLE	(E_DUF_MASK + 0x0372L)
#define			S_DU0373_SET_VBASE		(E_DUF_MASK + 0x0373L)
#define			S_DU0374_CREATE_IIXDBDEPENDS	(E_DUF_MASK + 0x0374L)
#define			S_DU0375_REDO_IIXDBDEPENDS	(E_DUF_MASK + 0x0375L)
#define			S_DU0376_DROP_FROM_IISTATISTICS	(E_DUF_MASK + 0x0376L)
#define			S_DU0377_SET_OPTSTAT		(E_DUF_MASK + 0x0377L)
#define			S_DU0378_FIX_SNUMCELLS		(E_DUF_MASK + 0x0378L)
#define			S_DU0379_DROP_FROM_IIHISTOGRAM	(E_DUF_MASK + 0x0379L)
#define			S_DU037A_SET_RELSPEC_TO_HASH	(E_DUF_MASK + 0x037AL)
#define			S_DU037B_SET_RELSPEC_TO_BTREE	(E_DUF_MASK + 0x037BL)
#define			S_DU037C_EMPTY_IIHISTOGRAM	(E_DUF_MASK + 0x037CL)
#define			S_DU037D_DROP_TABLE		(E_DUF_MASK + 0x037DL)
#define			S_DU037E_DROP_USER		(E_DUF_MASK + 0x037EL)
#define			S_DU037F_CLEAR_BADBITS		(E_DUF_MASK + 0x037FL)
#define			S_DU0380_DROP_IIDATABASE_TUPLE	(E_DUF_MASK + 0x0380L)
#define			S_DU0381_CREATE_IIEXTEND_ENTRY	(E_DUF_MASK + 0x0381L)
#define			S_DU0382_CREATE_DBACCESS_ENTRY	(E_DUF_MASK + 0x0382L)
#define			S_DU0383_MARK_DB_OPERATIVE	(E_DUF_MASK + 0x0383L)
#define			S_DU0384_UPDATE_IIDBID_IDX	(E_DUF_MASK + 0x0384L)
#define			S_DU0385_DROP_IIDBIDIDX		(E_DUF_MASK + 0x0385L)
#define			S_DU0386_UPDATE_IIDBIDIDX	(E_DUF_MASK + 0x0386L)
#define			S_DU0387_DROP_DBACCESS		(E_DUF_MASK + 0x0387L)
#define			S_DU0388_MARK_DB_PRIVATE	(E_DUF_MASK + 0x0388L)
#define			S_DU0389_DROP_LOCATION		(E_DUF_MASK + 0x0389L)
#define			S_DU038A_FIX_LOC_STAT		(E_DUF_MASK + 0x038AL)
#define			S_DU038B_DROP_IIEXTEND		(E_DUF_MASK + 0x038BL)
#define			S_DU038C_DROP_IISYNONYM		(E_DUF_MASK + 0x038CL)
#define                 S_DU038D_CLEAR_TCBINDEX         (E_DUF_MASK + 0x038DL)
#define                 S_DU038E_SET_RULES              (E_DUF_MASK + 0x038EL)
#define                 S_DU038F_REMOVE_STARCDBS        (E_DUF_MASK + 0x038FL)
#define                 S_DU0390_UPDATE_STARCDBS	(E_DUF_MASK + 0x0390L)
#define			S_DU0392_CLEAR_RULE		(E_DUF_MASK + 0x0392L)
#define			S_DU0393_DROP_EVENT		(E_DUF_MASK + 0x0393L)
#define			S_DU0394_DROP_DBP		(E_DUF_MASK + 0x0394L)
#define			S_DU0395_DROP_FROM_IIRULE	(E_DUF_MASK + 0x0395L)
#define			S_DU0396_SET_RULE		(E_DUF_MASK + 0x0396L)
#define			S_DU0397_CLEAR_DEFLT_GRP	(E_DUF_MASK + 0x0397L)
#define			S_DU0398_DROP_USR_FROM_GRP	(E_DUF_MASK + 0x0398L)
#define			S_DU0399_DROP_FROM_IIDBPRIV	(E_DUF_MASK + 0x0399L)
#define			S_DU039A_CLEAR_INVALID_PRIV	(E_DUF_MASK + 0x039AL)
#define			S_DU039B_DELETE_IIDEFAULT	(E_DUF_MASK + 0x039BL)
#define			S_DU039C_SET_INDEX		(E_DUF_MASK + 0x039CL)
#define			S_DU039D_DROP_IIGW06_RELATION	(E_DUF_MASK + 0x039DL)
#define			S_DU039E_DROP_IIGW06_ATTRIBUTE	(E_DUF_MASK + 0x039EL)
#define			S_DU039F_DROP_IIDBMS_COMMENT	(E_DUF_MASK + 0x039FL)


    /* Verifydb PURGE operation Messages range from 0x400 to 0x41F */
#define			S_DU0400_DEL_TEMP_FILE		(E_DUF_MASK + 0x0400L)
#define			S_DU0401_DROP_EXPIRED_RELATION	(E_DUF_MASK + 0x0401L)

#define			S_DU0410_DEL_TEMP_FILE		(E_DUF_MASK + 0x0410L)
#define			S_DU0411_DROP_EXPIRED_RELATION	(E_DUF_MASK + 0x0411L)
    /* verifydb TABLE and XTABLE Messages range from 0x420 - 0x42F */
#define			S_DU0420_DROP_INDEX		(E_DUF_MASK + 0x0420L)
#define			S_DU0421_DROP_INDEX		(E_DUF_MASK + 0x0421L)
#define			S_DU0422_DROP_UNIQUE_CONSTRAINT (E_DUF_MASK + 0x0422L)
#define			S_DU0423_DROP_UNIQUE_CONSTRAINT (E_DUF_MASK + 0x0423L)
#define			S_DU0424_DROP_CHECK_CONSTRAINT	(E_DUF_MASK + 0x0424L)
#define			S_DU0425_DROP_CHECK_CONSTRAINT	(E_DUF_MASK + 0x0425L)
#define			S_DU0426_DROP_REF_CONSTRAINT	(E_DUF_MASK + 0x0426L)
#define			S_DU0427_DROP_REF_CONSTRAINT	(E_DUF_MASK + 0x0427L)
#define			S_DU0428_DROP_PRIMARY_CONSTRAINT (E_DUF_MASK + 0x0428L)
#define			S_DU0429_DROP_PRIMARY_CONSTRAINT (E_DUF_MASK + 0x0429L)
    /* verifydb REFRESH_LBS Messages range from 0x430 - 0x43F */
#define			S_DU0430_INSERT_CAP		(E_DUF_MASK + 0x0430L)
#define			S_DU0431_CAP_INSERTED		(E_DUF_MASK + 0x0431L)
#define			S_DU0432_UPDATE_CAP		(E_DUF_MASK + 0x0432L)
#define			S_DU0433_CAP_UPDATED		(E_DUF_MASK + 0x0433L)


    /* 2nd set of verifydb catalog check operation -- Actual fixes
    ** DU0440 to DU049F
    **		(use range 240 - 29F for corresponding suggested fix notices)
    **	     A 1 to 1 correspondence between suggested and actual fixes,
    **	     Using the same message constant except that the actual fix constant
    **	     is 200 HEX higher than the suggested fix.
    **	     Example:
    **		S_DU0240_FIX_A	- suggested fix A
    **		S_DU0440_FIX_A	- notice that fix A was performed
    **	      or
    **		S_DU0241_FIX_B	- suggested fix B
    **		S_DU0441_FIX_B	- notice that fix B was performed
    **
    **	NOTE: this range of number should not be used until the 1st range
    **	      (300-34F for suggested fix, 350-3FF for acutal fix) fills up
    */
#define			S_DU0440_SET_COMMENT		(E_DUF_MASK + 0x0440L)
#define			S_DU0441_DROP_THIS_COMMENT	(E_DUF_MASK + 0x0441L)
#define			S_DU0442_SET_DELIM_MIXED	(E_DUF_MASK + 0x0442L)
#define			S_DU0443_CLEAR_DELIM_UPPER	(E_DUF_MASK + 0x0443L)
#define			S_DU0444_CLEAR_REALUSER_MIXED	(E_DUF_MASK + 0x0444L)
#define			S_DU0445_CREATE_SCHEMA		(E_DUF_MASK + 0x0445L)
#define			S_DU0446_CREATE_CONSTRAINT	(E_DUF_MASK + 0x0446L)
#define			S_DU0447_DROP_CONSTRAINT	(E_DUF_MASK + 0x0447L)
#define			S_DU0448_DELETED_IIXPRIV	(E_DUF_MASK + 0x0448L)
#define			S_DU0449_INSERTED_IIXPRIV_TUPLE	(E_DUF_MASK + 0x0449L)
#define			S_DU044A_DELETED_IIXEVENT	(E_DUF_MASK + 0x044AL)
#define			S_DU044B_INSERTED_IIXEVENT_TUPLE (E_DUF_MASK + 0x044BL)
#define			S_DU044C_DELETED_IIXPROC	(E_DUF_MASK + 0x044CL)
#define			S_DU044D_INSERTED_IIXPROC_TUPLE	(E_DUF_MASK + 0x044DL)
/* 44E, 44F obsolete */
#define			S_DU0450_DELETED_IIPRIV_TUPLE	(E_DUF_MASK + 0x0450L)
#define			S_DU0451_MARKED_DBP_DORMANT	(E_DUF_MASK + 0x0451L)
#define			S_DU0452_SET_DB_DBP_INDEP_LIST	(E_DUF_MASK + 0x0452L)
#define			S_DU0453_CLEARED_DBP_UBT_ID	(E_DUF_MASK + 0x0453L)
#define			S_DU0454_FIXED_OBJNAME_IN_IIPROT (E_DUF_MASK + 0x0454L)
#define			S_DU0455_SET_PERM_LANG_TO_QUEL	(E_DUF_MASK + 0x0455L)
#define			S_DU0456_SET_PERM_LANG_TO_SQL	(E_DUF_MASK + 0x0456L)
#define			S_DU0457_PERM_CREATED_BEFORE_10	(E_DUF_MASK + 0x0457L)
#define			S_DU0458_DROPPED_DBEVENT	(E_DUF_MASK + 0x0458L)
#define			S_DU0459_CLEARED_VGRANT_OK	(E_DUF_MASK + 0x0459L)

    /* verifydb debug messages range from 0x4a0 to 0x4bf */
#define			S_DU04A0_SHOW_MODE		(E_DUF_MASK + 0x04A0L)
#define			S_DU04A1_SHOW_SCOPE		(E_DUF_MASK + 0x04A1L)
#define			S_DU04A2_SHOW_SCOPE_NAME	(E_DUF_MASK + 0x04A2L)
#define			S_DU04A3_SHOW_OPERATION		(E_DUF_MASK + 0x04A3L)
#define			S_DU04A4_SHOW_TABLE_NAME	(E_DUF_MASK + 0x04A4L)
#define			S_DU04A5_SHOW_USER		(E_DUF_MASK + 0x04A5L)
    /* verifydb processing messages range from 0x4C0 to 0x4ff */
#define			S_DU04C0_CKING_CATALOGS		(E_DUF_MASK + 0x04C0L)
#define			S_DU04C1_CATALOGS_CHECKED	(E_DUF_MASK + 0x04C1L)
#define			S_DU04C2_FORCING_CONSISTENT	(E_DUF_MASK + 0x04C2L)
#define			S_DU04C3_DATABASE_PATCHED	(E_DUF_MASK + 0x04C3L)
#define			S_DU04C4_DROPPING_TABLE		(E_DUF_MASK + 0x04C4L)
#define			S_DU04C5_TABLE_DROPPED		(E_DUF_MASK + 0x04C5L)
#define			S_DU04C6_USER_ABORT		(E_DUF_MASK + 0x04C6L)
#define			S_DU04C7_PURGE_START		(E_DUF_MASK + 0x04C7L)
#define			S_DU04C8_TEMPPURGE_START	(E_DUF_MASK + 0x04C8L)
#define			S_DU04C9_EXPPURGE_START		(E_DUF_MASK + 0x04C9L)
#define			S_DU04CA_PURGE_DONE		(E_DUF_MASK + 0x04CAL)
#define			S_DU04CB_START_TABLE_OP		(E_DUF_MASK + 0x04CBL)
#define			S_DU04CC_TABLE_OP_DONE		(E_DUF_MASK + 0x04CCL)
#define			S_DU04CD_ACCESSCHECK_START	(E_DUF_MASK + 0x04CDL)
#define			S_DU04CE_ACCESSIBLE		(E_DUF_MASK + 0x04CEL)
#define			S_DU04CF_INACCESSIBLE		(E_DUF_MASK + 0x04CFL)
#define			S_DU04D0_DDB_WITH_INGRES_CDB	(E_DUF_MASK + 0x04D0L)
#define			S_DU04D1_DDB_CDB_NOT_INGRES	(E_DUF_MASK + 0x04D1L)
#define			S_DU04D2_REFRESH_START		(E_DUF_MASK + 0x04D2L)
#define			S_DU04D3_REFRESH_DONE		(E_DUF_MASK + 0x04D3L)

#define			S_DU04FF_CONTINUE_PROMPT        (E_DUF_MASK + 0x04FFL)

    /*-- Distributed database: Informational numbers range from 0x0500 to 0x051f --*/
#define			I_DU0500_STAR_UTIL_INTRO	(E_DUF_MASK + 0x0500L)
#define			I_DU0501_STAR_UTIL_SUCCESS	(E_DUF_MASK + 0x0501L)
#define			I_DU0502_STAR_UTIL_DDB_ENTRY	(E_DUF_MASK + 0x0502L)
#define			I_DU0503_STAR_UTIL_GENERAL   	(E_DUF_MASK + 0x0503L)

/* added for Titan */
#define			I_DU0504_ALL_DDBTUPS_FOUND_FI	(E_DUF_MASK + 0x0504L)
#define			I_DU0505_ALL_CDBTUPS_FOUND_FI   (E_DUF_MASK + 0x0505L)
#define			I_DU0506_ALL_STARTUPS_FOUND_FI	(E_DUF_MASK + 0x0506L)
#define			I_DU0507_DDBTUP_APPEND_FI	(E_DUF_MASK + 0x0507L)
#define			I_DU0508_ALL_DDBCDB_FOUND_FI    (E_DUF_MASK + 0x0508L)
#define			I_DU0509_DDB_FOUND_FI           (E_DUF_MASK + 0x0509L)
#define			I_DU0510_SEARCH_DDB_FI          (E_DUF_MASK + 0x0510L)
#define			I_DU0511_RESTORE_DDB_FI         (E_DUF_MASK + 0x0511L)
/*############################################################*/

/* Gateways 0x520 - 0x550    */
#define			I_DU0520_GW_UTIL_INTRO		(E_DUF_MASK + 0X0520L)
#define			I_DU0521_GW_UTIL_GENERAL	(E_DUF_MASK + 0X0521L)

/* Warning numbers range from 0x1000 to 0x1FFF */
#define			W_DU1000_DB_NOT_FOUND		(E_DUF_MASK + 0x1000L)
#define			W_DU1001_NO_IIEXT_ENTRIES	(E_DUF_MASK + 0x1001L)
#define			W_DU1010_UTIL_ABORT		(E_DUF_MASK + 0x1010L)
#define			W_DU1011_INTERRUPT		(E_DUF_MASK + 0x1011L)
#define			W_DU1012_UNKNOWN_EX		(E_DUF_MASK + 0x1012L)
#define			W_DU1020_DEL_DIRNOTFOUND	(E_DUF_MASK + 0x1020L)
#define			W_DU1021_DEL_FILENOTFOUND	(E_DUF_MASK + 0x1021L)
/* Warnings specific to SYSMOD range from 1030 to 103F. */
#define			W_DU1030_UNKNOWN_CATALOG_SY	(E_DUF_MASK + 0x1030L)
#define                 W_DU1031_NOMOD_FE_CAT           (E_DUF_MASK + 0x1031L)
#define                 W_DU1032_FECAT_IGNORE           (E_DUF_MASK + 0x1032L)

/* Warnings specific to FINDDBS range from 1040 to 105F. */
#define			W_DU1040_FOUND_EXTDB_FI		(E_DUF_MASK + 0x1040L)
#define			W_DU1041_NOLOC_FI		(E_DUF_MASK + 0x1041L)
#define			W_DU1042_NO_CNFLOC_FI		(E_DUF_MASK + 0x1042L)
#define			W_DU1043_SAME_DBNAME_FI		(E_DUF_MASK + 0x1043L)
#define			W_DU1044_REALDBS_NOT_FOUND_FI	(E_DUF_MASK + 0x1044L)
#define			W_DU1045_DBTUPS_NOT_FOUND_FI	(E_DUF_MASK + 0x1045L)
#define			W_DU1046_DUP_DBTUP_FI		(E_DUF_MASK + 0x1046L)
#define			W_DU1047_SAME_DB_FI		(E_DUF_MASK + 0x1047L)
#define			W_DU1048_ABORT_FI  		(E_DUF_MASK + 0x1048L)
#define			W_DU1049_LOCNAME_TOO_LONG_FI	(E_DUF_MASK + 0x1049L)
#define			W_DU104A_AREANAME_TOO_LONG_FI	(E_DUF_MASK + 0x104AL)
#define			W_DU104B_DUP_LOCATION_FI	(E_DUF_MASK + 0x104BL)
#define			W_DU104C_INVALID_LOC_FI		(E_DUF_MASK + 0x104CL)
#define			W_DU104D_DILISTDIR_FAIL_FI	(E_DUF_MASK + 0x104DL)

/* CONVTO60 warning numbers range from 0x1060 to 0x107A. */
#define			W_DU1062_DBMS60_FAIL		(E_DUF_MASK + 0x1062L)

/* Warnings specific to CREATEDB range from 1080 to 108F. */
#define			W_DU1080_RUN_DESTROYDB		(E_DUF_MASK + 0x1080L)
#define                 W_DU1081_CANT_READ_PRODS_LST    (E_DUF_MASK + 0x1081L)
#define			W_DU1082_STAR_FECAT_WARN	(E_DUF_MASK + 0x1082L)
#define			W_DU1083_BAD_STARCONNECT	(E_DUF_MASK + 0x1083L)
#define			W_DU1084_NO_FECAT_CRE		(E_DUF_MASK + 0x1084L)
#define			W_DU1085_SORTFLAG_DEPRECATED	(E_DUF_MASK + 0x1085L)
#define			W_DU1086_CANT_LOAD_PM_FILE	(E_DUF_MASK + 0x1086L)
#define			W_DU1087_CANT_GET_HOSTNAME	(E_DUF_MASK + 0x1087L)
#define			W_DU1088_PMSETDEFAULT_FAILED	(E_DUF_MASK + 0x1088L)
/* Warnings specific to FINDDBS when trying to build tuples for 5.0 databases
** or partially converted databases.
*/
#define			W_DU1520_NO_UCODE50_FILE	(E_DUF_MASK + 0x1520L)
#define			W_DU1521_BAD_UCODE		(E_DUF_MASK + 0x1521L)
#define			W_DU1522_NO_50AREA		(E_DUF_MASK + 0x1522L)
#define			W_DU1523_BAD_50ADMIN_READ	(E_DUF_MASK + 0x1523L)
#define			W_DU1524_TOO_MANY_LOCS   	(E_DUF_MASK + 0x1524L)

/* VERIFYDB warning numbers range from 0x1600 to 0x17ff.      */
#define			S_DU1600_INVALID_RELID		(E_DUF_MASK + 0x1600L)
#define			S_DU1601_INVALID_ATTID		(E_DUF_MASK + 0x1601L)
#define			S_DU1602_DUPLICATE_ATTIDS	(E_DUF_MASK + 0x1602L)
#define			S_DU1603_MISSING_ATTIDS		(E_DUF_MASK + 0x1603L)
#define			S_DU1604_ATTID_MISMATCH		(E_DUF_MASK + 0x1604L)
#define			S_DU1605_INCORRECT_RELWID	(E_DUF_MASK + 0x1605L)
#define			S_DU1606_KEYMISMATCH		(E_DUF_MASK + 0x1606L)
#define			S_DU1607_INVALID_RELSPEC	(E_DUF_MASK + 0x1607L)
#define			S_DU1608_MARKED_AS_CATALOG	(E_DUF_MASK + 0x1608L)
#define			S_DU1609_NOT_CATALOG		(E_DUF_MASK + 0x1609L)
#define			S_DU1610_NO_TABLE_FILE		(E_DUF_MASK + 0x1610L)
#define			S_DU1611_NO_PROTECTS		(E_DUF_MASK + 0x1611L)
#define			S_DU1612_NO_INTEGS		(E_DUF_MASK + 0x1612L)
#define			S_DU1613_BAD_CONCUR		(E_DUF_MASK + 0x1613L)
#define			S_DU1614_MISSING_CONCUR		(E_DUF_MASK + 0x1614L)
#define			S_DU1615_NOT_VIEW		(E_DUF_MASK + 0x1615L)
#define			S_DU1616_MISSING_VIEW_DEF	(E_DUF_MASK + 0x1616L)
#define			S_DU1617_NO_DBDEPENDS		(E_DUF_MASK + 0x1617L)
#define			S_DU1618_NO_QUERRYTEXT		(E_DUF_MASK + 0x1618L)
#define			S_DU1619_NO_VIEW		(E_DUF_MASK + 0x1619L)
#define			S_DU161A_NO_BASE_FOR_INDEX	(E_DUF_MASK + 0x161AL)
#define			S_DU161B_NO_2NDARY		(E_DUF_MASK + 0x161BL)
#define			S_DU161C_WRONG_NUM_INDEXES	(E_DUF_MASK + 0x161CL)
#define			S_DU161D_NO_INDEXS		(E_DUF_MASK + 0x161DL)
#define			S_DU161E_NO_STATISTICS		(E_DUF_MASK + 0x161EL)
#define			S_DU161F_WRONG_NUM_LOCS		(E_DUF_MASK + 0x161FL)
#define			S_DU1620_INVALID_LOC		(E_DUF_MASK + 0x1620L)
#define			S_DU1621_INVALID_FILLFACTOR	(E_DUF_MASK + 0x1621L)
#define			S_DU1622_INVALID_LEAF_FILLFTR	(E_DUF_MASK + 0x1622L)
#define			S_DU1623_INVALID_INDEX_FILLFTR	(E_DUF_MASK + 0x1623L)
#define			S_DU1624_MISSING_IIRELIDX	(E_DUF_MASK + 0x1624L)
#define			S_DU1625_MISSING_IIRELATION	(E_DUF_MASK + 0x1625L)
#define			S_DU1626_INVALID_ATTRELID	(E_DUF_MASK + 0x1626L)
#define			S_DU1627_INVALID_ATTFMT		(E_DUF_MASK + 0x1627L)
#define			S_DU1628_INVALID_BASEID		(E_DUF_MASK + 0x1628L)
#define			S_DU1629_IS_INDEX		(E_DUF_MASK + 0x1629L)
#define			S_DU162A_INVALID_INDEXKEYS	(E_DUF_MASK + 0x162AL)
#define			S_DU162B_INVALID_DEVRELIDX	(E_DUF_MASK + 0x162BL)
#define			S_DU162C_LOCATIONS_EXIST	(E_DUF_MASK + 0x162CL)
#define			S_DU162D_UNKNOWN_TREEMODE	(E_DUF_MASK + 0x162DL)
#define			S_DU162E_INVALID_TREEID		(E_DUF_MASK + 0x162EL)
#define			S_DU162F_NO_TREEBASE		(E_DUF_MASK + 0x162FL)
#define			S_DU1630_NO_VIEW_RELSTAT	(E_DUF_MASK + 0x1630L)
#define			S_DU1631_NO_PROTECT_RELSTAT	(E_DUF_MASK + 0x1631L)
#define			S_DU1632_NO_INTEGS_RELSTAT	(E_DUF_MASK + 0x1632L)
#define			S_DU1633_MISSING_TREE_SEQ	(E_DUF_MASK + 0x1633L)
#define			S_DU1634_NO_BASE_FOR_INTEG	(E_DUF_MASK + 0x1634L)
#define			S_DU1635_NO_TREE_FOR_INTEG	(E_DUF_MASK + 0x1635L)
#define			S_DU1636_NO_INTEG_QRYTEXT	(E_DUF_MASK + 0x1636L)
#define			S_DU1637_NO_BASE_FOR_PERMIT	(E_DUF_MASK + 0x1637L)
#define			S_DU1638_NO_TREE_FOR_PERMIT	(E_DUF_MASK + 0x1638L)
#define			S_DU1639_NO_PERMIT_QRYTEXT	(E_DUF_MASK + 0x1639L)
#define			S_DU163A_UNKNOWN_QRYMODE	(E_DUF_MASK + 0x163AL)
#define			S_DU163B_INVALID_QRYTEXT_ID	(E_DUF_MASK + 0x163BL)
#define			S_DU163C_QRYTEXT_VIEW_MISSING	(E_DUF_MASK + 0x163CL)
#define			S_DU163D_QRYTEXT_PERMIT_MISSING	(E_DUF_MASK + 0x163DL)
#define			S_DU163E_QRYTEXT_INTEG_MISSING	(E_DUF_MASK + 0x163EL)
#define			S_DU163F_QRYTEXT_DBP_MISSING	(E_DUF_MASK + 0x163FL)
#define			S_DU1640_NONEXISTENT_DEID	(E_DUF_MASK + 0x1640L)
#define			S_DU1641_NONEXISTENT_INDEP_TBL	(E_DUF_MASK + 0x1641L)
#define			S_DU1642_INID_NO_VBASE		(E_DUF_MASK + 0x1642L)
#define			S_DU1643_DEPENDENT_NOT_VIEW	(E_DUF_MASK + 0x1643L)
#define			S_DU1644_MISSING_IIXDBDEPENDS	(E_DUF_MASK + 0x1644L)
#define			S_DU1645_MISSING_IIDBDEPENDS	(E_DUF_MASK + 0x1645L)
#define			S_DU1646_INVALID_STATISTIC_COL	(E_DUF_MASK + 0x1646L)
#define			S_DU1647_NO_OPSTAT_IN_BASE	(E_DUF_MASK + 0x1647L)
#define			S_DU1648_MISSING_HISTOGRAM	(E_DUF_MASK + 0x1648L)
#define			S_DU1649_STATS_ON_INDEX		(E_DUF_MASK + 0x1649L)
#define			S_DU164A_NO_STATS_FOR_HISTOGRAM	(E_DUF_MASK + 0x164AL)
#define			S_DU164B_HISTOGRAM_SEQUENCE_ERR	(E_DUF_MASK + 0x164BL)
#define			S_DU164C_NO_BASE_FOR_STATS	(E_DUF_MASK + 0x164CL)
#define			S_DU164D_INVAL_SYS_CAT_RELSPEC	(E_DUF_MASK + 0x164DL)
#define			S_DU164E_DUPLICATE_TREE_SEQ	(E_DUF_MASK + 0x164EL)
#define			S_DU164F_QRYTEXT_SEQUENCE_ERR	(E_DUF_MASK + 0x164FL)
#define			S_DU1650_QRYTEXT_DUP_ERR	(E_DUF_MASK + 0x1650L)
#define			S_DU1651_INVALID_IIHISTOGRAM	(E_DUF_MASK + 0x1651L)
#define			S_DU1652_HISTOGRAM_DUP_ERR	(E_DUF_MASK + 0x1652L)
#define			S_DU1653_RUN_VERIFYDB		(E_DUF_MASK + 0x1653L)
#define			S_DU1654_INVALID_USERNAME	(E_DUF_MASK + 0x1654L)
#define			S_DU1655_INVALID_USERSTAT	(E_DUF_MASK + 0x1655L)
#define			S_DU1656_INVALID_DBNAME		(E_DUF_MASK + 0x1656L)
#define			S_DU1657_INVALID_DBDEV		(E_DUF_MASK + 0x1657L)
#define			S_DU1658_INVALID_DBOWN		(E_DUF_MASK + 0x1658L)
#define			S_DU1659_MISSING_IIEXTEND	(E_DUF_MASK + 0x1659L)
#define		        S_DU165A_INVALID_CKPDEV		(E_DUF_MASK + 0x165AL)
#define		        S_DU165B_INVALID_JNLDEV		(E_DUF_MASK + 0x165BL)
#define		        S_DU165C_INVALID_SORTDEV	(E_DUF_MASK + 0x165CL)
#define		        S_DU165D_DBA_HAS_NO_ACCESS	(E_DUF_MASK + 0x165DL)
#define			S_DU165E_BAD_DB			(E_DUF_MASK + 0x165EL)
#define			S_DU165F_UNCONVERTED_DB		(E_DUF_MASK + 0x165FL)
#define			S_DU1660_INCOMPATIBLE_DB	(E_DUF_MASK + 0x1660L)
#define			S_DU1661_INOPERATIVE_DB		(E_DUF_MASK + 0x1661L)
#define			S_DU1662_INCOMPAT_MINOR		(E_DUF_MASK + 0x1662L)
#define			S_DU1663_ZERO_DBID		(E_DUF_MASK + 0x1663L)
#define			S_DU1664_DUPLICATE_DBID		(E_DUF_MASK + 0x1664L)
#define			S_DU1665_NO_2NDARY_INDEX	(E_DUF_MASK + 0x1665L)
#define			S_DU1666_NO_IIDATABASE_ENTRY	(E_DUF_MASK + 0x1666L)
#define			S_DU1667_WRONG_IIDBIDIDX_TID	(E_DUF_MASK + 0x1667L)
#define			S_DU1668_NONUSER_WITH_ACCESS	(E_DUF_MASK + 0x1668L)
#define			S_DU1669_NOSUCH_DB		(E_DUF_MASK + 0x1669L)
#define			S_DU166A_PRIV_NOT_GLOBAL	(E_DUF_MASK + 0x166AL)
#define			S_DU166B_INVALID_LOCNAME	(E_DUF_MASK + 0x166BL)
#define			S_DU166C_BAD_LOC_STAT		(E_DUF_MASK + 0x166CL)
#define			S_DU166D_NOSUCH_DB		(E_DUF_MASK + 0x166DL)
#define			S_DU166E_NOSUCH_LOC		(E_DUF_MASK + 0x166EL)
#define			S_DU166F_NONUNIQUE_SYNONYM	(E_DUF_MASK + 0x166FL)
#define			S_DU1670_INVALID_SYNONYM	(E_DUF_MASK + 0x1670L)
#define			S_DU1671_NOT_INDEX		(E_DUF_MASK + 0x1671L)
#define			S_DU1672_MISSING_IIINDEX	(E_DUF_MASK + 0x1672L)
#define			S_DU1673_NO_RULES_RELSTAT	(E_DUF_MASK + 0x1673L)
#define                 S_DU1674_QRYTEXT_RULE_MISSING	(E_DUF_MASK + 0x1674L)
#define                 S_DU1675_QRYTEXT_EVENT_MISSING	(E_DUF_MASK + 0x1675L)
#define			S_DU1676_NOSUCH_DDB		(E_DUF_MASK + 0x1676L)
#define			S_DU1677_NOSUCH_CDB		(E_DUF_MASK + 0x1677L)
#define			S_DU1678_WRONG_DDB_OWNER	(E_DUF_MASK + 0x1678L)
#define			S_DU1679_WRONG_CDB_OWNER	(E_DUF_MASK + 0x1679L)
#define			S_DU167A_WRONG_CDB_ID		(E_DUF_MASK + 0x167AL)
#define			S_DU167B_NO_IISTARCDBS_ENTRY	(E_DUF_MASK + 0x167BL)
#define			S_DU167C_QRYTEXT_DBP_MISSING	(E_DUF_MASK + 0x167CL)
#define			S_DU167D_NO_RULES		(E_DUF_MASK + 0x167DL)
#define			S_DU167E_NO_TXT_FOR_EVENT	(E_DUF_MASK + 0x167EL)
#define			S_DU167F_EVENTID_NOT_UNIQUE	(E_DUF_MASK + 0x167FL)
#define                 S_DU1680_NO_TXT_FOR_DBP		(E_DUF_MASK + 0x1680L)
#define                 S_DU1681_DBPID_NOT_UNIQUE	(E_DUF_MASK + 0x1681L)
#define                 S_DU1682_NO_BASE_FOR_RULE	(E_DUF_MASK + 0x1682L)
#define                 S_DU1683_NO_RULE_RELSTAT	(E_DUF_MASK + 0x1683L)
#define			S_DU1684_NO_RULE_QRYTEXT	(E_DUF_MASK + 0x1684L)
#define			S_DU1685_NO_DBP_FOR_RULE	(E_DUF_MASK + 0x1685L)
#define			S_DU1686_NO_TIME_FOR_RULE	(E_DUF_MASK + 0x1686L)
#define                 S_DU1687_INVALID_DMPDEV		(E_DUF_MASK + 0x1687L)
#define                 S_DU1688_INVL_DEFAULT_GRP	(E_DUF_MASK + 0x1688L)
#define                 S_DU1689_NOSUCH_USER		(E_DUF_MASK + 0x1689L)
#define                 S_DU168A_NOSUCH_DB		(E_DUF_MASK + 0x168AL)
#define                 S_DU168B_NOSUCH_USER		(E_DUF_MASK + 0x168BL)
#define                 S_DU168C_INVALID_PRIVS		(E_DUF_MASK + 0x168CL)
#define			S_DU168D_SCHEMA_MISSING		(E_DUF_MASK + 0x168DL)
#define			S_DU168E_UNREFERENCED_DEFTUPLE	(E_DUF_MASK + 0x168EL)
#define			S_DU168F_ILLEGAL_KEY		(E_DUF_MASK + 0x168FL)
#define			S_DU1690_MISSING_KEYTUP		(E_DUF_MASK + 0x1690L)
#define			S_DU1691_MISSING_RULETUP	(E_DUF_MASK + 0x1691L)
#define			S_DU1692_MISSING_DBPROCTUP	(E_DUF_MASK + 0x1692L)
#define			S_DU1693_MISSING_INDEXTUP	(E_DUF_MASK + 0x1693L)
#define			S_DU1694_INVALID_DEFAULT	(E_DUF_MASK + 0x1694L)
#define			S_DU1695_NOT_MARKED_AS_IDX	(E_DUF_MASK + 0x1695L)
#define			S_DU1696_IIINDEX_MISSING	(E_DUF_MASK + 0x1696L)
#define			S_DU1697_MISSING_RELTUP		(E_DUF_MASK + 0x1697L)
#define			S_DU1698_MISSING_GW06ATT	(E_DUF_MASK + 0x1698L)
#define			S_DU1699_NONZERO_GW06IDX	(E_DUF_MASK + 0x1699L)
#define			S_DU169A_MISSING_GW06REL	(E_DUF_MASK + 0x169AL)
#define			S_DU169B_MISSING_IIATTRIBUTE	(E_DUF_MASK + 0x169BL)
#define			S_DU169C_NONZERO_GW06ATTIDX	(E_DUF_MASK + 0x169CL)
#define			S_DU169D_MISSING_IIGW06REL	(E_DUF_MASK + 0x169DL)
#define			S_DU169E_MISSING_COM_BASE	(E_DUF_MASK + 0x169EL)
#define			S_DU169F_NO_COMMENT_RELSTAT	(E_DUF_MASK + 0x169FL)
#define			S_DU16A0_BAD_COL_COMMENT	(E_DUF_MASK + 0x16A0L)
#define			S_DU16A1_BAD_TBL_COMMENT	(E_DUF_MASK + 0x16A1L)
#define			S_DU16A2_COMMENT_DUP_ERR	(E_DUF_MASK + 0x16A2L)
#define			S_DU16A3_COMMENT_SEQ_ERR	(E_DUF_MASK + 0x16A3L)
#define			S_DU16A4_NONDELIM_UPPER_ERR	(E_DUF_MASK + 0x16A4L)
#define			S_DU16A5_NONDELIM_LOWER_ERR	(E_DUF_MASK + 0x16A5L)
#define			S_DU16A6_SCHIZOID_DELIM_ID	(E_DUF_MASK + 0x16A6L)
#define                 S_DU16A7_REALUSER_CASE_ERR      (E_DUF_MASK + 0x16A7L)
#define			S_DU16A8_IIDBDEPENDS_MISSING	(E_DUF_MASK + 0x16A8L)
#define                 S_DU16AA_RESET_DEFLT_PROF       (E_DUF_MASK + 0x16AAL)
#define                 S_DU16AB_INVALID_PROFNAME       (E_DUF_MASK + 0x16ABL)
#define                 S_DU16AC_DROP_PROFILE           (E_DUF_MASK + 0x16ACL)
#define                 S_DU16AD_PROFILE_DROPPED        (E_DUF_MASK + 0x16ADL)
#define                 S_DU16AE_INVALID_PROFSTAT       (E_DUF_MASK + 0x16AEL)
#define                 S_DU16AF_CLEAR_PROFILE_BITS     (E_DUF_MASK + 0x16AFL)
#define                 S_DU16B0_PROFILE_BITS_CLEARED   (E_DUF_MASK + 0x16B0L)
#define                 S_DU16B1_NO_DEFAULT_PROFILE     (E_DUF_MASK + 0x16B1L)
#define                 S_DU16B2_ADD_DEFAULT_PROF       (E_DUF_MASK + 0x16B2L)
#define                 S_DU16B3_DEFAULT_PROF_ADDED     (E_DUF_MASK + 0x16B3L)
#define                 S_DU16B4_INVLD_USER_PROFILE     (E_DUF_MASK + 0x16B4L)
#define                 S_DU16B5_USER_PROF_RESET        (E_DUF_MASK + 0x16B5L)
#define                 S_DU16C0_NONEXISTENT_DEP_OBJ    (E_DUF_MASK + 0x16C0L)
#define                 S_DU16C1_NONEXISTENT_INDEP_OBJ  (E_DUF_MASK + 0x16C1L)
#define			S_DU16C2_UNEXPECTED_DEP_OBJ_TYPE (E_DUF_MASK + 0x16C2L)
#define			S_DU16C3_UNEXPECTED_IND_OBJ_TYPE (E_DUF_MASK + 0x16C3L)
#define			S_DU16C4_MISSING_INDEP_OBJ	(E_DUF_MASK + 0x16C4L)
#define			S_DU16C5_MISSING_PERM_INDEP_OBJ (E_DUF_MASK + 0x16C5L)
#define			S_DU16C7_EXTRA_IIXPRIV		(E_DUF_MASK + 0x16C7L)
#define			S_DU16C8_MISSING_IIXPRIV	(E_DUF_MASK + 0x16C8L)
#define			S_DU16C9_EXTRA_IIXEVENT		(E_DUF_MASK + 0x16C9L)
#define			S_DU16CA_MISSING_IIXEVENT	(E_DUF_MASK + 0x16CAL)
#define			S_DU16CB_EXTRA_IIXPROCEDURE	(E_DUF_MASK + 0x16CBL)
#define			S_DU16CC_MISSING_IIXPROCEDURE	(E_DUF_MASK + 0x16CCL)
/* 16CD, 16CE obsolete */
#define			S_DU16CF_BAD_OBJ_TYPE_IN_IIPRIV (E_DUF_MASK + 0x16CFL)

#define			S_DU16D0_QRYTEXT_ALARM_MISSING  (E_DUF_MASK + 0x16D0L)
#define			S_DU16D1_NO_BASE_FOR_ALARM      (E_DUF_MASK + 0x16D1L)
#define			S_DU16D2_DROP_FROM_IISECALARM   (E_DUF_MASK + 0x16D2L)
#define			S_DU16D3_IISECALARM_DROP        (E_DUF_MASK + 0x16D3L)
#define			S_DU16D4_NO_ALARM_RELSTAT       (E_DUF_MASK + 0x16D4L)
#define			S_DU16D5_SET_ALMTUPS            (E_DUF_MASK + 0x16D5L)
#define			S_DU16D6_ALMTUPS_SET            (E_DUF_MASK + 0x16D6L)
#define			S_DU16D7_NO_ALARM_QRYTEXT       (E_DUF_MASK + 0x16D7L)
#define			S_DU16D8_NO_ALARM_EVENT         (E_DUF_MASK + 0x16D8L)

#define			S_DU16E0_NONEXISTENT_OBJ	(E_DUF_MASK + 0x16E0L)
#define			S_DU16E1_NO_OBJ_DEPENDS_ON_DESCR (E_DUF_MASK + 0x16E1L)
#define			S_DU16E2_LACKING_INDEP_PRIV	(E_DUF_MASK + 0x16E2L)
#define			S_DU16E3_LACKING_PERM_INDEP_PRIV (E_DUF_MASK + 0x16E3L)
#define			S_DU16E4_LACKING_CONS_INDEP_PRIV (E_DUF_MASK + 0x16E4L)
#define			S_DU16E5_MISSING_INDEP_LIST	(E_DUF_MASK + 0x16E5L)
#define			S_DU16E6_UNEXPECTED_INDEP_LIST	(E_DUF_MASK + 0x16E6L)
#define			S_DU16E7_MISSING_UBT		(E_DUF_MASK + 0x16E7L)
#define			S_DU16E8_UBT_IS_A_CATALOG	(E_DUF_MASK + 0x16E8L)
#define			S_DU16E9_UNEXPECTED_DBP_PERMS	(E_DUF_MASK + 0x16E9L)
#define			S_DU16EA_UNEXPECTED_PERMOBJ_TYPE (E_DUF_MASK + 0x16EAL)
#define			S_DU16EB_NO_OBJECT_FOR_PERMIT	(E_DUF_MASK + 0x16EBL)
#define			S_DU16EC_NAME_MISMATCH		(E_DUF_MASK + 0x16ECL)
#define			S_DU16ED_INVALID_OPSET_FOR_OBJ	(E_DUF_MASK + 0x16EDL)
#define			S_DU16EE_INVALID_OPCTL_FOR_OBJ  (E_DUF_MASK + 0x16EEL)
#define			S_DU16EF_QUEL_PERM_MARKED_AS_SQL (E_DUF_MASK + 0x16EFL)
#define			S_DU16F0_SQL_PERM_MARKED_AS_QUEL (E_DUF_MASK + 0x16F0L)
#define			S_DU16F1_MULT_PRIVS		(E_DUF_MASK + 0x16F1L)
#define			S_DU16F2_PRE_10_PERMIT		(E_DUF_MASK + 0x16F2L)
#define			S_DU16F3_LACKING_GRANT_OPTION	(E_DUF_MASK + 0x16F3L)
#define			S_DU16F4_VIEW_NOT_ALWAYS_GRNTBL	(E_DUF_MASK + 0x16F4L)
#define			S_DU16F5_MISSING_VIEW_INDEP_PRVS (E_DUF_MASK + 0x16F5L)
#define			S_DU16F6_INVALID_IIPROTECT	(E_DUF_MASK + 0x16F6L)


#define		        S_DU1700_NO_PURGE_AUTHORITY	(E_DUF_MASK + 0x1700L)
#define			S_DU1701_TEMP_FILE_FOUND	(E_DUF_MASK + 0X1701L)
#define			S_DU1702_EXPIRED_TABLE_FOUND	(E_DUF_MASK + 0X1702L)
#define			S_DU1703_DELETE_ERR		(E_DUF_MASK + 0X1703L)

#define			S_DU1705_AFLAG_WARNING		(E_DUF_MASK + 0X1705L)
#define			S_DU1706_NOT_STARDB		(E_DUF_MASK + 0X1706L)
#define			S_DU1707_LCL_CAP_UNKNOWN	(E_DUF_MASK + 0x1707L)
#define			S_DU1708_LCL_CAP_MISSING	(E_DUF_MASK + 0x1708L)
#define			S_DU1709_LCL_CAP_DIFFERENT	(E_DUF_MASK + 0x1709L)

#define			S_DU1710_2NDARY_INDEX		(E_DUF_MASK + 0x1710L)
#define			S_DU1711_UNIQUE_CONSTRAINT	(E_DUF_MASK + 0x1711L)
#define			S_DU1712_CHECK_CONSTRAINT	(E_DUF_MASK + 0x1712L)
#define			S_DU1713_REFERENCES_CONSTRAINT	(E_DUF_MASK + 0x1713L)
#define			S_DU1714_PRIMARY_CONSTRAINT	(E_DUF_MASK + 0x1714L)

#define			S_DU1750_LABEL_PURGE		(E_DUF_MASK + 0x1750L)
#define			S_DU1751_LABEL_SCAN_TBL_COL	(E_DUF_MASK + 0x1751L)
#define			S_DU1752_LABEL_ALL_USED		(E_DUF_MASK + 0x1752L)
#define			S_DU1753_LABEL_PURGING		(E_DUF_MASK + 0x1753L)
#define			S_DU1754_LABEL_UNREF		(E_DUF_MASK + 0x1754L)
#define			S_DU1755_LABEL_ROW_CNT		(E_DUF_MASK + 0x1755L)

#define			S_DU17FC_TABLE_PATCH_WARNING	(E_DUF_MASK + 0x17FCL)
#define			S_DU17FD_CATALOG_PATCH_WARNING	(E_DUF_MASK + 0x17FDL)
#define			S_DU17FE_FORCE_CONSISTENT_WARN	(E_DUF_MASK + 0x17FEL)

#define			S_DU17FF_FIX_INTERACTIVELY	(E_DUF_MASK + 0x17FFL)
/*############################################################*/

    /*-- Distributed Database:	Warning numbers range from 0x1800 to 0x183f --*/
#define			W_DU1800_STAR_CDB_NOT_FOUND	(E_DUF_MASK + 0x1800L)
    /*-- StarView Warning messages --*/
#define			W_DU1801_SV_NO_DDB_SELECTED	(E_DUF_MASK + 0x1801L)
#define			W_DU1802_SV_BAD_NODENAME	(E_DUF_MASK + 0x1802L)
#define			W_DU1803_SV_BAD_DDBNAME		(E_DUF_MASK + 0x1803L)
#define			W_DU1804_SV_CANT_GET_LDBLIST	(E_DUF_MASK + 0x1804L)
#define			W_DU1805_SV_CANT_OPEN_DDB	(E_DUF_MASK + 0x1805L)
#define			W_DU1806_SV_CANT_DIRECT_CONNECT	(E_DUF_MASK + 0x1806L)
#define			W_DU1807_SV_CANT_GET_DDB_INFO	(E_DUF_MASK + 0x1807L)
#define			W_DU1808_SV_CANT_GET_LDB_INFO	(E_DUF_MASK + 0x1808L)
#define			W_DU1809_SV_CANT_DIR_DISCONNECT	(E_DUF_MASK + 0x1809L)
#define			W_DU180A_SV_CANT_OPEN_IIDBDB	(E_DUF_MASK + 0x180AL)
#define			W_DU180B_SV_CANT_DISCONNECT	(E_DUF_MASK + 0x180BL)
#define			W_DU180C_SV_CANT_OPEN_DDB	(E_DUF_MASK + 0x180CL)
#define			W_DU180D_SV_CANT_FIND_VNODE	(E_DUF_MASK + 0x180DL)
#define			W_DU180E_SV_ENTER_BAD_VNODE	(E_DUF_MASK + 0x180EL)
#define			W_DU180F_SV_CANT_DIRCONNECT_LDB	(E_DUF_MASK + 0x180FL)
#define			W_DU1810_SV_CANT_CLOSE_DDB	(E_DUF_MASK + 0x1810L)
#define			W_DU1811_SV_CURSOR_NOT_IN_TABLE	(E_DUF_MASK + 0x1811L)
#define			W_DU1812_SV_EMPTY_TABLE		(E_DUF_MASK + 0x1812L)
#define			W_DU1813_SV_CANT_DROP_REG_TABLE	(E_DUF_MASK + 0x1813L)
#define			W_DU1814_SV_CANT_DROP_TABLE	(E_DUF_MASK + 0x1814L)
#define			W_DU1815_SV_CANT_DROP_VIEW	(E_DUF_MASK + 0x1815L)
#define			W_DU1816_SV_CANT_REGISTER_TABLE	(E_DUF_MASK + 0x1816L)
#define			W_DU1817_SV_ERROR_IN_CRITERIA	(E_DUF_MASK + 0x1817L)
#define			W_DU1818_SV_DB_DOES_NOT_EXIST	(E_DUF_MASK + 0x1818L)
#define			W_DU1819_SV_RM_NONE_REG_OBJ	(E_DUF_MASK + 0x1819L)
#define			W_DU181A_SV_CANT_REG_FROM_CRIT	(E_DUF_MASK + 0x181AL)
#define			W_DU181B_SV_NONE_FUNCTIONAL	(E_DUF_MASK + 0x181BL)
#define			W_DU181C_SV_CANT_FIND_STRING	(E_DUF_MASK + 0x181CL)
#define			W_DU181D_SV_EMPTY_LDB		(E_DUF_MASK + 0x181DL)
#define			W_DU181E_SV_DUPLICATE_REG	(E_DUF_MASK + 0x181EL)
#define                 W_DU181F_SV_NO_DDBS_FOUND       (E_DUF_MASK + 0x181FL) 

    /*-- Destroydb --*/
#define			W_DU1830_STAR_DESTRY_CDB	(E_DUF_MASK + 0x1830L)
#define                 W_DU1831_SUPUSR_OBSOLETE        (E_DUF_MASK + 0x1831L)

/* added for Titan */
#define			W_DU1840_DDBTUPS_NOT_FOUND_FI   (E_DUF_MASK + 0x1840L)
#define			W_DU1841_CDBTUPS_NOT_FOUND_FI	(E_DUF_MASK + 0x1841L)
#define			W_DU1842_STARTUPS_NOT_FOUND_FI  (E_DUF_MASK + 0x1842L)
#define			W_DU1843_DUP_STARTUP_FI		(E_DUF_MASK + 0x1843L)
#define			W_DU1844_DDBCDB_NOT_FOUND_FI	(E_DUF_MASK + 0x1844L)
#define			W_DU1845_DBOPEN_FAIL_FI		(E_DUF_MASK + 0x1845L)
#define			W_DU1846_DDB_RESTORE_FAIL_FI    (E_DUF_MASK + 0x1846L)
#define			W_DU1847_DDBDUPLICATE_ERROR_FI  (E_DUF_MASK + 0x1847L)

    /* upgradedb warning numbers range from 1850 to 189f */
#define			W_DU1850_IIDBDB_ERR		(E_DUF_MASK + 0x1850L)
#define			W_DU1851_JOURNAL_WARN		(E_DUF_MASK + 0x1851L)
#define			W_DU1852_CDB_AND_DDB		(E_DUF_MASK + 0x1852L)
#define			W_DU1853_CDB_SKIPPED		(E_DUF_MASK + 0x1853L)
#define			W_DU1854_ERROR_SKIP		(E_DUF_MASK + 0x1854L)
#define			W_DU1855_TRACE_3		(E_DUF_MASK + 0x1855L)
#define			W_DU1856_TRACE_4		(E_DUF_MASK + 0x1856L)
#define			W_DU1857_UNCONVERTED_V5DB	(E_DUF_MASK + 0x1857L)
#define			W_DU1858_INOPERATIVE_DB		(E_DUF_MASK + 0x1858L)
#define			W_DU1859_WRONG_LEVEL		(E_DUF_MASK + 0x1859L)
#define			W_DU185A_ALREADY_CURRENT	(E_DUF_MASK + 0x185AL)
#define			W_DU185B_BAD_DUMP_INFO		(E_DUF_MASK + 0x185BL)
#define			W_DU185C_BAD_CONFIG_READ	(E_DUF_MASK + 0x185CL)
#define			W_DU185D_IS_JOURNALED		(E_DUF_MASK + 0x185DL)
#define			W_DU185E_BAD_CONFIG_UPDATE	(E_DUF_MASK + 0x185EL)
#define			W_DU185F_BAD_DELETE		(E_DUF_MASK + 0x185FL)
#define			W_DU1860_SKIP_DB		(E_DUF_MASK + 0x1860L)
#define			W_DU1861_JNL_WARNING		(E_DUF_MASK + 0x1861L)
#define			W_DU1862_CNF_UPDATE_FAIL	(E_DUF_MASK + 0x1862L)
#define			W_DU1863_CNF_UPDATE_FAIL	(E_DUF_MASK + 0x1863L)
#define			W_DU1864_FE_CAT_FAIL		(E_DUF_MASK + 0x1864L)
#define			W_DU1865_MISSING_UTEXE_DEF	(E_DUF_MASK + 0x1865L)
#define			W_DU1866_BAD_UTEXE_DEF		(E_DUF_MASK + 0x1866L)
#define			W_DU1867_UGRADEFE_FAIL		(E_DUF_MASK + 0x1867L)
#define			W_DU1868_DESTROYED_DB		(E_DUF_MASK + 0x1868L)
#define			W_DU1869_NOMODIFY		(E_DUF_MASK + 0X1869L)
#define			W_DU186A_MISSING_SYSCAT		(E_DUF_MASK + 0X186AL)
#define			W_DU186B_REGISTER_FAILED	(E_DUF_MASK + 0X186BL)
#define			W_DU186C_UPGRADEDB_UNLOADDB	(E_DUF_MASK + 0x186CL)
#define			W_DU186D_UPGRADEDB_SQL		(E_DUF_MASK + 0x186DL)
#define			W_DU186E_VIEW_UPGRADE		(E_DUF_MASK + 0x186EL)
#define			W_DU186F_INTEG_UPGRADE		(E_DUF_MASK + 0x186FL)
#define			W_DU1870_PROC_UPGRADE		(E_DUF_MASK + 0x1870L)
#define			W_DU1871_CONS_UPGRADE		(E_DUF_MASK + 0x1871L)
#define			W_DU1872_UPGRADEDB_TREE_DB	(E_DUF_MASK + 0x1872L)
#define			W_DU1873_UPGRADEDB_TREE		(E_DUF_MASK + 0x1873L)

/* Internal error numbers range from 0x2000 to 0x2FFF */
#define			E_DU2000_BAD_ERLOOKUP		(E_DUF_MASK + 0x2000L)
#define			E_DU2001_BAD_ERROR_PARAMS	(E_DUF_MASK + 0x2001L)
#define			E_DU2002_BAD_SYS_ERLOOKUP	(E_DUF_MASK + 0x2002L)
#define			E_DU2003_BAD_ACT_JNL_DIR	(E_DUF_MASK + 0x2003L)
#define			E_DU2004_BAD_CKPT_DIR		(E_DUF_MASK + 0x2004L)
#define			E_DU2005_BAD_DFLT_DB_DIR	(E_DUF_MASK + 0x2005L)
#define			E_DU2006_BAD_XPIR_JNL_DIR	(E_DUF_MASK + 0x2006L)
#define			E_DU2007_BAD_EXT_DB_DIR		(E_DUF_MASK + 0x2007L)
#define			E_DU2008_BAD_FULL_JNL_DIR	(E_DUF_MASK + 0x2008L)
#define			E_DU2009_BAD_JNL_DIR		(E_DUF_MASK + 0x2009L)
#define			E_DU200A_BAD_DB_BOOT		(E_DUF_MASK + 0x200AL)
#define			E_DU200B_BAD_IPROC_CREATE	(E_DUF_MASK + 0x200BL)
#define			E_DU200C_BAD_IPROC_EXECUTE	(E_DUF_MASK + 0x200CL)
#define			E_DU200D_BAD_IPROC_CREATE	(E_DUF_MASK + 0x200DL)
#define			E_DU200E_BAD_IPROC_EXECUTE	(E_DUF_MASK + 0x200EL)
#define			E_DU200F_BAD_IPROC_CREATE	(E_DUF_MASK + 0x200FL)
#define			E_DU2010_BAD_IPROC_EXECUTE	(E_DUF_MASK + 0x2010L)
#define			E_DU2011_BAD_IPROC_CREATE	(E_DUF_MASK + 0x2011L)
#define			E_DU2012_BAD_IPROC_EXECUTE	(E_DUF_MASK + 0x2012L)
#define			E_DU2013_GW_STDCATS_INT_ERR	(E_DUF_MASK + 0x2013L)
#define                 E_DU2014_GRANTING_SEL_ON_CATS   (E_DUF_MASK + 0x2014L)

#define			E_DU2100_BAD_RECONNECT		(E_DUF_MASK + 0x2100L)
#define			E_DU2101_BAD_DISCONNECT		(E_DUF_MASK + 0x2101L)
#define			E_DU2102_BAD_STARDISCONNECT	(E_DUF_MASK + 0x2102L)
#define			E_DU2103_BAD_COMMIT		(E_DUF_MASK + 0x2103L)
#define			E_DU2104_BAD_LOCALCONNECT	(E_DUF_MASK + 0x2104L)
#define			E_DU2105_BAD_LOCALDISCONNECT	(E_DUF_MASK + 0x2105L)
#define			E_DU2106_UNOPENED_DB		(E_DUF_MASK + 0x2106L)

#define			E_DU2410_GEN_CL_FAIL		(E_DUF_MASK + 0x2410L)
#define			E_DU2411_BAD_LOINGPATH		(E_DUF_MASK + 0x2411L)
#define			E_DU2412_CANT_TRANSLATE_LOC	(E_DUF_MASK + 0x2412L)
#define			E_DU2414_BAD_CASE_LABEL		(E_DUF_MASK + 0x2414L)
#define			E_DU2415_BAD_DMP_DIR            (E_DUF_MASK + 0x2415L)

#define			E_DU2416_NOCOPY_SYSCAT		(E_DUF_MASK + 0x2416L)
#define			E_DU2417_NODROP_SYSCAT		(E_DUF_MASK + 0x2417L)
#define			E_DU2418_NOCREATE_SYSCAT	(E_DUF_MASK + 0x2418L)
#define			E_DU2419_NOREPOP_SYSCAT		(E_DUF_MASK + 0x2419L)
#define			E_DU241A_SYSCAT_UPGRADE_FAILED	(E_DUF_MASK + 0x241AL)
#define			E_DU241B_NODROP_TEMP		(E_DUF_MASK + 0x241BL)
#define E_DU241C_SELECT_ERR_UPGR_IIDEFAULT (E_DUF_MASK + 0x241CL)
#define E_DU241D_INSERT_ERR_UPGR_IIDEFAULT (E_DUF_MASK + 0x241DL)


/* VERIFYDB numbers range from 0x2e00 to 0x2eff.	      */


/* Numbers ranging from 2F00 to 2FFF are specific to CONVTO60. */
#define			E_DU2F00_TOO_MANY_ARGS		(E_DUF_MASK + 0x2F00L)


/* User and enviroment error numbers range from 0x3000 to 0xFFFF */

#define			E_DU3000_BAD_USRNAME		(E_DUF_MASK + 0x3000L)
#define			E_DU3001_USER_NOT_KNOWN		(E_DUF_MASK + 0x3001L)
#define			E_DU3002_ALIAS_NOT_KNOWN	(E_DUF_MASK + 0x3002L)
#define			E_DU3003_NOT_SUPER_USER		(E_DUF_MASK + 0x3003L)
#define			E_DU3004_NO_ALIAS_FLAG		(E_DUF_MASK + 0x3004L)
#define			E_DU3005_NOT_AUTHORIZED		(E_DUF_MASK + 0x3005L)
#define			E_DU3006_CANT_OPEN_IIDBDB	(E_DUF_MASK + 0x3006L)
#define			E_DU3010_BAD_DBNAME		(E_DUF_MASK + 0x3010L)
#define			E_DU3011_NOT_DBA_DS		(E_DUF_MASK + 0x3011L)
#define			E_DU3012_NODS_DBDB_DS		(E_DUF_MASK + 0x3012L)
#define			E_DU3014_NO_DBNAME		(E_DUF_MASK + 0x3014L)
#define			E_DU3015_CANT_DESTROY_50DB	(E_DUF_MASK + 0x3015L)
#define			E_DU3016_50_DB_FOUND		(E_DUF_MASK + 0x3016L)
#define			E_DU3020_DB_NOT_FOUND		(E_DUF_MASK + 0x3020L)
#define			E_DU3021_DB_EXISTS_CR		(E_DUF_MASK + 0x3021L)
#define			E_DU3022_DB_UNIQUE_CR		(E_DUF_MASK + 0x3022L)
#define			E_DU3030_NO_LOGNAME		(E_DUF_MASK + 0x3030L)
#define			E_DU3040_NO_DEF_DBLOC		(E_DUF_MASK + 0x3040L)
#define			E_DU3041_NO_JNLLOC		(E_DUF_MASK + 0x3041L)
#define			E_DU3042_NO_CKPLOC		(E_DUF_MASK + 0x3042L)
#define			E_DU3043_NO_SORTLOC		(E_DUF_MASK + 0x3043L)
#define			E_DU3044_NO_DMPLOC		(E_DUF_MASK + 0x3044L)
#define			E_DU3045_BAD_LOCNAME		(E_DUF_MASK + 0x3045L)
#define			E_DU3046_BAD_DBLOC		(E_DUF_MASK + 0x3046L)
#define			E_DU3047_BAD_CKPLOC		(E_DUF_MASK + 0x3047L)
#define			E_DU3048_BAD_JNLLOC		(E_DUF_MASK + 0x3048L)
#define			E_DU3049_BAD_SORTLOC		(E_DUF_MASK + 0x3049L)
#define                 E_DU3050_BAD_DMPLOC             (E_DUF_MASK + 0x3050L)
#define                 E_DU3051_NO_DMPLOC              (E_DUF_MASK + 0x3051L)
#define			E_DU3052_BAD_RDONLYLOC		(E_DUF_MASK + 0x3052L)
#define			E_DU3060_NO_IIEXT_ENTRIES_DS	(E_DUF_MASK + 0x3060L)
#define			E_DU3064_NO_DBCAT_ENTRY_DS	(E_DUF_MASK + 0x3064L)
#define			E_DU3070_BAD_COLLATION		(E_DUF_MASK + 0x3070L)
#define			E_DU3080_DELETE_DIR_DS		(E_DUF_MASK + 0x3080L)
#define			E_DU3081_NO_ABFLOG_DS 		(E_DUF_MASK + 0x3081L)
#define			E_DU3100_UCODE_OLDUSAGE		(E_DUF_MASK + 0x3100L)
#define			E_DU3101_UNKNOWN_FLAG_DS	(E_DUF_MASK + 0x3101L)
#define			E_DU3102_NO_CONFIRM_DS		(E_DUF_MASK + 0x3102L)
#define			E_DU3121_UNKNOWN_FLAG_CR	(E_DUF_MASK + 0x3121L)
#define			E_DU3122_CONFLICTING_FLAGS	(E_DUF_MASK + 0x3122L)
#define                 E_DU3129_DBDB_NODMPLOC_CR       (E_DUF_MASK + 0x3129L)
#define			E_DU3130_NO_CREATEDB_CR		(E_DUF_MASK + 0x3130L)
#define			E_DU3131_MISSIING_DBDB		(E_DUF_MASK + 0x3131L)
#define			E_DU3132_BAD_DBDBNAME_CR	(E_DUF_MASK + 0x3132L)
#define			E_DU3133_NO_DBDB_FLAG_CR	(E_DUF_MASK + 0x3133L)
#define			E_DU3134_DBDB_NODBLOC_CR	(E_DUF_MASK + 0x3134L)
#define			E_DU3135_DBDB_NOJNLLOC_CR	(E_DUF_MASK + 0x3135L)
#define			E_DU3136_DBDB_NOCKPLOC_CR	(E_DUF_MASK + 0x3136L)
#define			E_DU3137_DBDB_PRIVATE_CR	(E_DUF_MASK + 0x3137L)
#define			E_DU3138_DBDB_NOSORTLOC_CR	(E_DUF_MASK + 0x3138L)
#define			E_DU3139_DBDB_NORDONLYLOC_CR	(E_DUF_MASK + 0x3139L)
#define			E_DU3140_DBDB_NOT_SUPERUSER_CR	(E_DUF_MASK + 0x3140L)
#define			E_DU314A_CONFIG_FILE_TOOBIG_CR	(E_DUF_MASK + 0x314AL)
#define			E_DU3150_BAD_FILE_CREATE	(E_DUF_MASK + 0x3150L)
#define			E_DU3151_BAD_FILE_OPEN		(E_DUF_MASK + 0x3151L)
#define			E_DU3152_BAD_SENSE		(E_DUF_MASK + 0x3152L)
#define			E_DU3153_BAD_READ		(E_DUF_MASK + 0x3153L)
#define			E_DU3154_BAD_ALLOC		(E_DUF_MASK + 0x3154L)
#define			E_DU3155_BAD_WRITE		(E_DUF_MASK + 0x3155L)
#define			E_DU3156_BAD_FLUSH		(E_DUF_MASK + 0x3156L)
#define			E_DU3157_BAD_CLOSE		(E_DUF_MASK + 0x3157L)
#define			E_DU3158_BAD_PAGE_NUM		(E_DUF_MASK + 0x3158L)
#define			E_DU3170_BAD_USRF_OPEN		(E_DUF_MASK + 0x3170L)
#define			E_DU3171_USRF_NOT_FOUND		(E_DUF_MASK + 0x3171L)
#define			E_DU3172_BAD_USRF_RECORD	(E_DUF_MASK + 0x3172L)
#define			E_DU3180_BAD_TEMPLATE_COPY	(E_DUF_MASK + 0x3180L)
#define			E_DU3190_BAD_LKINIT		(E_DUF_MASK + 0x3190L)
#define			E_DU3191_BAD_LK_REQUEST		(E_DUF_MASK + 0x3191L)
#define			E_DU3192_BAD_LKLIST_RELEASE	(E_DUF_MASK + 0x3192L)
#define			E_DU3193_BAD_LKLIST_CREATE	(E_DUF_MASK + 0x3193L)
#define			E_DU3195_DB_LKBUSY		(E_DUF_MASK + 0x3195L)
#define			E_DU31A0_BAD_DIRFILE_DELETE	(E_DUF_MASK + 0x31A0L)
#define			E_DU31A1_BAD_DIR_DELETE		(E_DUF_MASK + 0x31A1L)
#define			E_DU31A2_BAD_FILE_DELETE	(E_DUF_MASK + 0x31A2L)
#define			E_DU31B0_BAD_DIR_CREATE		(E_DUF_MASK + 0x31B0L)
#define			E_DU31C0_BAD_LGOPEN		(E_DUF_MASK + 0x31C0L)
#define			E_DU31C1_BAD_LGALTER		(E_DUF_MASK + 0x31C1L)
#define			E_DU31C2_BAD_LGADD		(E_DUF_MASK + 0x31C2L)
#define			E_DU31C3_BAD_LGBEGIN		(E_DUF_MASK + 0x31C3L)
#define			E_DU31C4_BAD_LGEVENT		(E_DUF_MASK + 0x31C4L)
#define			E_DU31C5_BAD_LGCLOSE		(E_DUF_MASK + 0x31C5L)
#define			E_DU31C6_BAD_LGREMOVE		(E_DUF_MASK + 0x31C6L)
#define			E_DU31C7_BAD_LGEND		(E_DUF_MASK + 0x31C7L)
#define			E_DU31C8_ARCHIVER_NOT_RUNNING	(E_DUF_MASK + 0x31C8L)
#define			E_DU31D0_DBID_MAXTRYS		(E_DUF_MASK + 0x31D0L)
#define			E_DU3200_BAD_APPEND		(E_DUF_MASK + 0x3200L)
#define			E_DU3201_BAD_REPLACE		(E_DUF_MASK + 0x3201L)
#define			E_DU3202_BAD_DELETE		(E_DUF_MASK + 0x3202L)
#define                 E_DU3203_BAD_REGISTER           (E_DUF_MASK + 0x3203L)
#define                 E_DU3204_BAD_CREATE             (E_DUF_MASK + 0x3204L)
#define                 E_DU3205_BAD_VIEW               (E_DUF_MASK + 0x3205L)
#define			E_DU3300_UNKNOWN_FLAG_SY	(E_DUF_MASK + 0x3300L)
#define			E_DU3303_BAD_PAGESIZE		(E_DUF_MASK + 0X3303L)
#define			E_DU3304_BAD_SYSTEM_CATALOG_SY	(E_DUF_MASK + 0x3304L)
#define			E_DU3310_NOT_DBA_SY		(E_DUF_MASK + 0x3310L)
#define			E_DU3312_DBDB_ONLY_CAT_SY	(E_DUF_MASK + 0x3312L)
#define			E_DU3314_NOSUCH_CAT_SY		(E_DUF_MASK + 0x3314L)
#define			E_DU3400_BAD_MEM_ALLOC		(E_DUF_MASK + 0x3400L)
#define			E_DU3500_TOO_MANY_ARGS_FI	(E_DUF_MASK + 0x3500L)
#define			E_DU3501_BAD_FLAG_FI		(E_DUF_MASK + 0x3501L)
#define			E_DU3510_SAME_DBNAME_FI		(E_DUF_MASK + 0x3510L)
#define                 E_DU3511_NOCR_FE_CATS           (E_DUF_MASK + 0x3511L)
#define                 E_DU3512_BAD_CLIENT_NM          (E_DUF_MASK + 0x3512L)
#define                 E_DU3513_NO_WRITE_ACCESS        (E_DUF_MASK + 0x3513L)
#define                 E_DU3514_EMPTY_CLIENT_LIST      (E_DUF_MASK + 0x3514L)
#define			E_DU3515_CLIENT_NM_CONFLICT	(E_DUF_MASK + 0x3515L)
#define		        E_DU3516_DBDB_NOSECURITY        (E_DUF_MASK + 0x3516L)
#define			E_DU3519_LEGAL_REG_DELIM_ID	(E_DUF_MASK + 0x3519L)
#define			E_DU351A_MULTI_DB_DESTRY	(E_DUF_MASK + 0x351AL)
#define			E_DU351B_INCOMPATIBLE_VERSION	(E_DUF_MASK + 0x351BL)
#define			E_DU351C_INCOMPATIBLE_VERSION	(E_DUF_MASK + 0x351CL)


#define			E_DU3600_INTERRUPT		(E_DUF_MASK + 0x3600L)
#define			E_DU3601_ACCESS_VIOLATION	(E_DUF_MASK + 0x3601L)

/* Numbers ranging from 4000 - 4?FF are specific to CONVTO60. */

/* Audit/status file errors: 4000 - 400F */
#define			E_DU4000_BAD_STATF_READ		(E_DUF_MASK + 0x4000L)

#define			E_DU4010_RENAME_FILE_EXISTS	(E_DUF_MASK + 0x4010L)

#define			E_DU4012_MAXAREA		(E_DUF_MASK + 0x4012L)

/* qrymod reload errors: 4020 - 402F */
#define			E_DU4020_DATABASE_OPEN		(E_DUF_MASK + 0x4020L)
#define			E_DU4021_CANT_OPEN_QRYMOD_FILE	(E_DUF_MASK + 0x4021L)
#define			E_DU4022_BAD_QRYMOD_FILE_READ	(E_DUF_MASK + 0x4022L)
#define			E_DU4023_WRONG_DATABASE		(E_DUF_MASK + 0x4023L)
#define			E_DU4024_BAD_CHANGE_USER_FLAG	(E_DUF_MASK + 0x4024L)


/* VERIFYDB numbers range from 0x5000 to 0x50ff */
#define			E_DU5000_INVALID_INPUT_NAME	(E_DUF_MASK + 0x5000L)
#define			E_DU5001_INVALID_MODE_FLAG	(E_DUF_MASK + 0x5001L)
#define			E_DU5002_INVALID_SCOPE_FLAG	(E_DUF_MASK + 0x5002L)
#define			E_DU5003_INVALID_OPERATION_FLAG	(E_DUF_MASK + 0x5003L)
#define			E_DU5004_TOO_MANY_NAMES_IN_LIST (E_DUF_MASK + 0x5004L)
#define			E_DU5005_SPECIFY_ONLY_1_TABLE	(E_DUF_MASK + 0x5005L)
#define			E_DU5006_UNKNOWN_PARAMETER	(E_DUF_MASK + 0x5006L)
#define			E_DU5007_SPECIFY_ALL_FLAGS	(E_DUF_MASK + 0x5007L)
#define			E_DU5008_SPECIFY_TABLE_NAME	(E_DUF_MASK + 0x5008L)
#define			E_DU5009_SPECIFY_DB_NAME	(E_DUF_MASK + 0x5009L)
#define			E_DU5010_NO_SYSCAT_FILE		(E_DUF_MASK + 0x5010L)
#define			E_DU501A_CANT_CONNECT		(E_DUF_MASK + 0x501AL)
#define			E_DU501B_CANT_CONNECT_AS_USER	(E_DUF_MASK + 0x501BL)
#define			E_DU501C_EMPTY_IIRELATION	(E_DUF_MASK + 0x501CL)
#define			E_DU501D_SCOPE_NOT_IMPLEMENTED	(E_DUF_MASK + 0x501DL)
#define			E_DU501E_OP_NOT_IMPLEMENTED	(E_DUF_MASK + 0x501EL)
#define			E_DU501F_CATALOG_CHECK_ERR	(E_DUF_MASK + 0x501FL)
#define			E_DU5020_DB_PATCH_ERR		(E_DUF_MASK + 0x5020L)
#define			E_DU5021_EMPTY_IITREE		(E_DUF_MASK + 0x5021L)
#define			E_DU5022_EMPTY_IIDBDEPENDS	(E_DUF_MASK + 0x5022L)
#define			E_DU5023_CANT_OPEN_LOG_FILE	(E_DUF_MASK + 0x5023L)
#define			E_DU5024_TBL_DROP_ERR		(E_DUF_MASK + 0x5024L)
#define			E_DU5025_NO_SUCH_TABLE		(E_DUF_MASK + 0x5025L)
#define			E_DU5026_EMPTY_IIUSER		(E_DUF_MASK + 0x5026L)
#define			E_DU5027_CMPTLVL_MISMATCH	(E_DUF_MASK + 0x5027L)
#define			E_DU5028_CMPTMINOR_MISMATCH	(E_DUF_MASK + 0x5028L)
#define			E_DU5029_EMPTY_IIDATABASE	(E_DUF_MASK + 0x5029L)
#define			E_DU502A_PURGE_ERR		(E_DUF_MASK + 0x502AL)
#define			E_DU502B_CANT_MAKE_PURGELIST	(E_DUF_MASK + 0x502BL)
#define			E_DU502C_UNABLE_TO_DELETE_FILE	(E_DUF_MASK + 0x502CL)
#define			E_DU502D_NOFREE_PURGETABLE	(E_DUF_MASK + 0x502DL)
#define			E_DU502E_TABLE_OP_ERROR		(E_DUF_MASK + 0x502EL)
#define			E_DU502F_CANT_PATCH_SCONDUR	(E_DUF_MASK + 0x502FL)
#define			E_DU5030_CANT_PATCH_INDEX	(E_DUF_MASK + 0x5030L)
#define			E_DU5031_TABLE_DOESNT_EXIST	(E_DUF_MASK + 0x5031L)
#define			E_DU5032_NO_AUTHORITY		(E_DUF_MASK + 0x5032L)
#define			E_DU5033_REPORTMODE_INVALID	(E_DUF_MASK + 0x5033L)
#define			E_DU5034_EXPIRED_SYNTAX_ERROR	(E_DUF_MASK + 0x5034L)
#define			E_DU5035_REPORTMODE_ONLY	(E_DUF_MASK + 0x5035L)
#define			E_DU5036_NODBS_IN_SCOPE		(E_DUF_MASK + 0x5036L)
#define			E_DU5037_INVALID_ACCESS_USR	(E_DUF_MASK + 0x5037L)
#define			E_DU5038_CANT_PATCH_VIEW	(E_DUF_MASK + 0x5038L)
#define			E_DU5039_REFRESH_ERR		(E_DUF_MASK + 0x5039L)
#define			E_DU503A_ILLEGAL_FLAG_COMBO	(E_DUF_MASK + 0x503AL)
#define			E_DU503B_EMPTY_IIPROCEDURE	(E_DUF_MASK + 0x503BL)
#define                 E_DU503C_TOO_MANY_VNODES        (E_DUF_MASK + 0x503CL)
#define                 E_DU503D_RUNMODE_INVALID	(E_DUF_MASK + 0x503DL)
#define                 E_DU503E_INVALID_LOG_FILE	(E_DUF_MASK + 0x503EL)

/*############################################################*/

    /*-- Distributed Destroydb: User Error numbers range from 0x5100 to 0x5105 --*/
#define			E_DU5100_STAR_CDB_DEST_ONLY	(E_DUF_MASK + 0x5100L)
#define			E_DU5101_STAR_CANT_DEST_CDB	(E_DUF_MASK + 0x5101L)
#define			E_DU5102_STAR_DDB_DONT_EXIST	(E_DUF_MASK + 0x5102L)

    /*-- Distributed Createdb:	    User Error numbers range from 0x5106 to 0x510f --*/
#define			E_DU5107_PRIVATE_STAR		(E_DUF_MASK + 0x5107L)
#define			E_DU5108_STAR_DDB_NAME_TOO_LONG	(E_DUF_MASK + 0x5108L)
#define			E_DU5109_STAR_INVALID_NAME	(E_DUF_MASK + 0x5109L)
#define			E_DU510A_STAR_CDB_ALREADY_EXIST	(E_DUF_MASK + 0x510AL)
#define			E_DU510B_STAR_DDB_ALREADY_EXIST	(E_DUF_MASK + 0x510BL)
#define			E_DU510C_STAR_DBDB_NOT_LEGAL	(E_DUF_MASK + 0x510CL)
#define			E_DU510D_STAR_DDB_EQS_CDB	(E_DUF_MASK + 0x510DL)
#define			E_DU510E_STAR_TOO_MANY_ARGS	(E_DUF_MASK + 0x510EL)
#define			E_DU510F_STAR_CANT_FIND_VNODE	(E_DUF_MASK + 0x510FL)

    /*-- StarView:	    User Error numbers range from 0x5110 to 0x512f --*/
#define			E_DU5110_SV_NOT_STAR_SERVER	(E_DUF_MASK + 0x5110L)
#define			E_DU5111_SV_CANT_OPEN_IIDBDB	(E_DUF_MASK + 0x5111L)
#define			E_DU5112_SV_BAD_START_NAME	(E_DUF_MASK + 0x5112L)

    /*-- Gateways:	    User error numbers range from 0x5130 --*/
#define			E_DU5130_GW_INVALID_NAME	(E_DUF_MASK + 0X5130L)
#define			E_DU5131_GW_DBDB_NOT_LEGAL	(E_DUF_MASK + 0X5131L)

    /* upgradedb User and enviroment error numbers range from 0x6000 to 0x61ff*/
#define			E_DU6000_UPGRADEDB_USAGE	(E_DUF_MASK + 0X6000L)
#define			E_DU6001_FAILED_CNF_READ	(E_DUF_MASK + 0X6001L)
#define			E_DU6002_BAD_IIDBDB_CONNECT	(E_DUF_MASK + 0X6002L)
#define			E_DU6003_UNKNOWN_VERSION	(E_DUF_MASK + 0X6003L)
#define			E_DU6004_WRONG_VERSION		(E_DUF_MASK + 0X6004L)
#define			E_DU6005_NONEXISTENT_IIDBDB	(E_DUF_MASK + 0X6005L)
#define			E_DU6006_INOPERATIVE_IIDBDB	(E_DUF_MASK + 0X6006L)
#define			E_DU6007_NONEXISTENT_USER	(E_DUF_MASK + 0X6007L)
#define			E_DU6008_IIDBDB_WRONG_LEVEL	(E_DUF_MASK + 0X6008L)
#define			E_DU6009_IIDBDB_UPGR_DISALLOW	(E_DUF_MASK + 0X6009L)
#define			E_DU600A_IIDBDB_JOURNALED	(E_DUF_MASK + 0X600AL)
#define			E_DU600B_ACCESS_UPDATE_ERR	(E_DUF_MASK + 0X600BL)
#define			E_DU600C_CMPT_UPDATE_ERR	(E_DUF_MASK + 0X600CL)
#define			E_DU600D_BAD_IIDBDB_RECONNECT	(E_DUF_MASK + 0X600DL)
#define			E_DU600E_NO_DUMP_INFO		(E_DUF_MASK + 0X600EL)
#define			E_DU600F_BAD_DUMP_CLEANUP	(E_DUF_MASK + 0X600FL)
#define			E_DU6010_NOT_SUPERUSER		(E_DUF_MASK + 0X6010L)
#define			E_DU6011_NOT_DBA		(E_DUF_MASK + 0X6011L)
#define			E_DU6012_NONEXISTENT_TARGET	(E_DUF_MASK + 0x6012L)
#define			E_DU6013_NO_MORE_MEMORY		(E_DUF_MASK + 0X6013L)
#define			E_DU6014_DBNAME_TWICE		(E_DUF_MASK + 0X6014L)
#define			E_DU6015_BAD_CLIENT_NM		(E_DUF_MASK + 0X6015L)
#define			E_DU6016_CANT_CRE_DIR		(E_DUF_MASK + 0X6016L)
#define			E_DU6017_CANT_OPEN_LOG		(E_DUF_MASK + 0X6017L)
#define			E_DU6018_CANT_CSINITIATE	(E_DUF_MASK + 0X6018L)
#define			E_DU6019_CANT_UPGRADESQL92      (E_DUF_MASK + 0X6019L)
#define			E_DU601A_NOUPDATE_SYSCAT	(E_DUF_MASK + 0X601AL)

    /* Loadmdb User and enviroment error numbers range from 0x6500 to 0x66ff*/
#define			E_DU6500_FAIL_UCTR_COMASST_LOAD	(E_DUF_MASK + 0X6500L)
#define			E_DU6501_FAIL_ICAN_SVCMGNT_LOAD	(E_DUF_MASK + 0X6501L)
#define			E_DU6502_FAIL_AUTOSYS_JVM_LOAD	(E_DUF_MASK + 0X6502L)
#define			E_DU6503_FAIL_UCTR_SVCPLUS_LOAD	(E_DUF_MASK + 0X6503L)
#define			E_DU6504_FAIL_DATA_TRSFRM_LOAD	(E_DUF_MASK + 0X6504L)
#define			E_DU6505_FAIL_ETRST_DIRECT_LOAD (E_DUF_MASK + 0X6505L)
#define			E_DU6506_FAIL_AFSN_WBENCH_LOAD	(E_DUF_MASK + 0X6506L)
#define			E_DU6507_FAIL_ETRST_2020_LOAD	(E_DUF_MASK + 0X6507L)
#define			E_DU6508_FAIL_CLVP_PORTAL_LOAD	(E_DUF_MASK + 0X6508L)
#define			E_DU6509_FAIL_UCTR_MGMTPTL_LOAD	(E_DUF_MASK + 0X6509L)
#define			E_DU650A_FAIL_CLVP_CNTMGMT_LOAD	(E_DUF_MASK + 0X650AL)
#define			E_DU650B_FAIL_HVST_ALL_LOAD 	(E_DUF_MASK + 0X650BL)
#define			E_DU650C_FAIL_ETRST_PKI_LOAD	(E_DUF_MASK + 0X650CL)
#define			E_DU650D_FAIL_ARCSEREV_LOAD	(E_DUF_MASK + 0X650DL)
#define			E_DU650E_FAIL_UCTR_LOTUS_LOAD	(E_DUF_MASK + 0X650EL)
#define			E_DU650F_FAIL_MODEL_MGR_LOAD	(E_DUF_MASK + 0X650FL)
#define			E_DU6510_FAIL_UCTR_WEBSPHR_LOAD (E_DUF_MASK + 0X6510L)
#define			E_DU6511_FAIL_ETRST_VULMGR_LOAD	(E_DUF_MASK + 0X6511L)
#define			E_DU6512_FAIL_ALFSN_PRCMGR_LOAD	(E_DUF_MASK + 0X6512L)
#define			E_DU6513_FAIL_CLVP_WKFL_LOAD	(E_DUF_MASK + 0X6513L)
#define			E_DU6514_FAIL_UCTR_TSREORG_LOAD	(E_DUF_MASK + 0X6514L)
#define			E_DU6515_FAIL_UCTR_ENTDBA_LOAD	(E_DUF_MASK + 0X6515L)
#define			E_DU6516_FAIL_UCTR_DBANLZR_LOAD	(E_DUF_MASK + 0X6516L)
#define			E_DU6517_FAIL_UCTR_DBM_LOAD	(E_DUF_MASK + 0X6517L)
#define			E_DU6518_FAIL_UCTR_DBPERFM_LOAD	(E_DUF_MASK + 0X6518L)
#define			E_DU6519_FAIL_UCTR_WSDM_LOAD	(E_DUF_MASK + 0X6519L)
#define			E_DU651A_FAIL_ETRST_EIAM_LOAD	(E_DUF_MASK + 0X651AL)
#define			E_DU651B_FAIL_MDB_CAT_LOAD	(E_DUF_MASK + 0X651BL)
#define			E_DU6520_LOADMDB_FAILED		(E_DUF_MASK + 0X6520L)
#define			E_DU6521_UNORM_NFC_WITH_NFD	(E_DUF_MASK + 0X6521L)

/*
[@group_of_defined_constants@]...
*/


/*}
** Name: DU_ERROR -	error-handling control block.
**
** Description:
**        This is the error-handling control block for the database utility
**	control block.  In addition to handling errors, this block will also
**	be used for printing warnings and informative messages.
**
** History:
**      04-Sep-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
typedef	struct	_DU_ERROR
{
    DU_STATUS		    du_status;		    /* The status associated
						    ** with the formatted mess-
						    ** age in du_errmsg.
						    */
    /* {@fix_me@} */
    i4			    du_ingerr;		    /* The ingres error that
						    ** generated this message.
						    */
    i4		    du_utilerr;		    /* If non-zero, this is the
						    ** database utility error
						    ** that is associated with
						    ** the message in .du_errmsg
						    */
    STATUS		    du_clerr;		    /* If the error was the
						    ** the result of a CL error,
						    ** this will contain the
						    ** CL error status.
						    */
    CL_SYS_ERR		    du_clsyserr;	    /* The operating sytem
						    ** error number.
						    */
    char		    du_errmsg[ER_MAX_LEN];  /* The buffer to format a
						    ** message in.
						    */
    i4			    du_flags;		    /* Flags used to customize
						    ** error handling procedures
						    ** Note, du_reset_err() will
						    ** turn off all flags.
						    */
#define			DU_SAVEMSG	00001	    /* Informs error handler
						    ** to not print informative
						    ** and warning msgs and not
						    ** to reset the error control
						    ** block.
						    */
}   DU_ERROR;


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS   du_error( DU_ERROR *, int, int, ...);

FUNC_EXTERN VOID    du_reset_err();	/* Reset system-utility error-handling
					** control block
					*/


