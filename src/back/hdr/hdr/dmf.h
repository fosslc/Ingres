/*
** Copyright (c) 1985, 2010 Ingres Corporation
**
**
*/

/**
** Name: DMF.H - All global DMF definitions needed for external interface.
**
** Description:
**      This file contains the structure typedefs and definitions of 
**      symbolic constants that are global to all DMF external operations.
**      The external interface to DMF is through one function called
**      call.  The function is called with a DMF operation code 
**      and a call control block which varies depending on the 
**      type of operation.  The call sequence is as follows:
**
**          status = call(operation,&dmucb);  
**
**      The operation is one of the symbolic constants defined in
**      this file.  
**
**      To insure unique names the following convention
**      is used to denote each major category of operation or type.
**
**      DM_      - is used for all DMF external data types.
**      E_DMnnnn - is used for all DMF errors.
**      DMA      - is used for all DMF security auditing.
**      DMC_     - is used for all control operations and control blocks.
**      DMD_     - is used for all debug operations and control blocks.
**      DMF_     - is used for all global DMF information.
**	DMM_	 - is used for all maintanence operations.
**      DMR_     - is used for all record operations and control blocks.
**      DMT_     - is used for all table operations and control blocks.
**      DMU_     - is used for all utility operations and control blocks.
**      DMX_     - is used for all transaction operations and control blocks.
**       
**      The global data types, DMF errors, and operation codes are included
**      in this file.  The control blocks are defined in separate files, for 
**      example the utility control block can be found in DMUCB.H, the 
**      debug control block can be found in DMDCB.H, etc.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      04-aug-86 (jennifer)
**          Changed DM_I_RELATION_IDX to DM_I_RIDX for
**          7 char unique problem.
**      02-sep-86 (jennifer)
**          Added new command DMR_LOAD for qef.
**      03-sep-86 (jennifer)
**          Added new command DMR_DUMP_DATA for qef.
**      10-sep-86 (jennifer)
**          Added new error for illegal operation.
**      10-dec-86 (fred)
**          Added database database catalog table id's
**	12-mar-87 (rogerk)
**	    Added DM_MDATA struct for multi-row interface to DMR_LOAD.
**	01-aug-88 (greg)
**	    DM_SCONCUR_MAX changed from 09L to 9L (0 means octal)
**	02-aug-88 (teg)
**	    added E_DM007B and DMU_GET_TABID
**	 8-aug-88 (teg)
**	    created DM_FI_xxxx section.
**	31-aug-88 (rogerk)
**	    Added Write Behind thread control operation and return code.
**	30-nov-88 (rogerk)
**	    Added DMF_T0_VERSION constant for version "6.0"
**	28-feb-89 (rogerk)
**	    Added shared cache error number - E_DM0123_BAD_CACHE_NAME.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added DMF_[BI]_{GRPID,APLID}_TAB_ID
**	    for iiusergroup and iiapplication_id catalogs
**      16-mar-89 (neil)
**          Added reserved table id for iirule.
**      25-mar-89 (jennifer)
**          Added interface for security auditing. DMA_WRITE_RECORD
**      25-mar-89 (jennifer)
**          Added interface for security auditing. DMA_SET_STATE.
**      25-mar-89 (derek)
**          Added new maintenance operations. DMM_CREATE_DB, DMM_DESTROY_DB, DMM_ALTER_DB.
**      31-mar-89 (ralph)
**          GRANT Enhancements, Phase 2:
**          Added DMF_[BI]_DBPRIV_TAB_ID for the iidbpriv catalog
**	14-Apr-89 (teg)
**	    Added new maintenance operations: DMM_LISTFILE, DMM_DROPFILE.
**      15-jul-89 (ralph)
**          Added DMF_[BI]_SECSTATE_TAB_ID for the iisecuritystate catalog
**	25-jul-89 (sandyh)
**	    Added DM0133 error on invalid dcb during db access - bug 6533
**      06-nov-89 (fred)
**	    Added E_DM0066_BAD_KEY_TYPE error for reporting misuse of unsortable
**	    and/or unkeyable datatypes (for, initially, large object support).
**      25-nov-89 (neil)
**          Added reserved table id for iievent.
**	16-jan-90 (ralph)
**	    Added DMF_[BI]_DBLOC_TAB_ID for the iidbloc catalog.
**      24-Jan-1990 (fred)
**	    Added DM_{B,I}_ETAB_TAB_ID for the iiextensions catalog.
**	13-feb-90 (carol)
**	    Added reserved table ids for iidbms_comment and iisynonym catalogs.
**	15-feb-90 (teg)
**	    drop in check/patch table message constants.
**	27-feb-90 (teg)
**	    added check isam messages.
**	13-mar-90 (linda)
**	    Added 3 new Gateway error codes DM0135, DM0136, DM0137.
**	 9-apr-90 (sandyh)
**	    Added DM_LOGTRACE to enable tracing of LGwrite call headers.
**	18-apr-90 (rogerk)
**	    Added DMR_ALTER definition and error codes.
**	19-apr-90 (andre)
**	    Added key attribute ids for IISYNONYM.
**	11-jun-1990 (bryanp)
**	    Added E_DM0141_GWF_DATA_CVT_ERROR
**	    and   E_DM0142_GWF_ALREADY_REPORTED
**	08-aug-90 (ralph)
**	    Added DMA_SHOW_STATE
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	25-feb-91 (rogerk)
**	    Added external error number E_DM0146 for Set Nologging Abort error.
**	7-mar-1991 (bryanp)
**	    Added W_DM5059_BAD_KEY_GET for dm1u.c use (Btree index compression).
**	22-apr-91 (rogerk)
**	    Added external error number E_DM0147 for Set Nologging with
**	    online backup error.
**	23-jul-91 (markg)
**	   Added E_DM0148_USER_QUAL_ERROR for fix to bug 38679.
**	19-aug-91 (rogerk)
**	    Added E_DM0149_SETLG_SVPT_ABORT as part of bugfix for 38403.
**	29-may-92 (andre)
**	    reserved table id and define key attribute ids for IIPRIV
**	10-jun-92 (andre)
**	    reserved table id and defined key attribute ids for IIXPRIV
**	18-jun-92 (andre)
**	    reserved id and defined key attribute ids for IIXPROCEDURE and
**	    IIXEVENT
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS improvements.
**	22-sep-1992 (bryanp)
**	    Added operation codes for new thread types to dmf_call.
**	    Added new error messages for DMF special threads.
**	02-oct-92 (teresa)
**	    added DMM_CONFIG and DMM_DEL_DUMP_CONFIG to DM_OPERATION
**	05-oct-92 (robf)
**	    C2-SXA gateway catalogs
**	30-October-1992 (rmuth)
**	    Add new dmf_call DMU_CONVERT_TABLE and the 
**	    E_DM00A1_ERROR_CONVERTING_TBL error message.
**	02-nov-92 (jrb)
**	    Multi-location sorting project.  Added DM_WRKLOC_MAX and some
**	    error messages and changed one of Jennifer's error numbers because
**	    I already had it reserved.
**	09-nov-92 (rickh)
**	    Moved _DM_COLUMN from DMP.H to this header file so that it
**	    could be seen by GWF.H and friends.
**      02-nov-92 (anitap)
**          Added defines for table ids for IISCHEMA and IISCHEMAIDX.
**	18-jan-1993 (bryanp)
**	    Add message for CP_TIMER thread; add DMC_CP_TIMER operation code.
**	07-jan-93 (rickh)
**	    Added IIPROCEDURE_PARAMETER.
**	24-jan-93 (anitap)
**	    Changes for CREATE SCHEMA. Add another key to iischemaidx.
**	19-feb-93 (rickh)
**	    Removed second key of IIDEFAULTIDX and force first key to be the
**	    third attribute to agree with CREATEDB.
**	08-mar-1993 (rmuth)
**	    Add messages for VERIFYDB to recognise the new dm1p structures.
**	15-mar-1993 (jhahn)
**	    Added E_DM0150_DUPLICATE_KEY_STMT and
**	     E_DM0151_SIDUPLICATE_KEY_STMT.
**	13-may-1993 (robf)
**	    Secure 2.0: Added DM_[BI]_IISECID[X]_TAB_ID for iilabelmap[x]
**	15-may-1993 (rmuth)
**	    Add E_DM0067_READONLY_TABLE_ERR, E_DM0068_MODIFY_READONLY_ERR
**	    and E_DM0069_CONCURRENT_IDX_ERR.
**	19-may-93 (anitap)
**	    Added error message E_DM0154_DUPLICATE_ROW_NOTUPD for update 
**	    rowcount project.
**	24-may-1993 (robf)
**	    Secure 2.0: Added DMU_GET_SECID and E_DM00A2_CANT_CREATE_SECID,
**			E_DM00A3_SECID_TOO_MANY
**	15-jun-93 (rickh)
**	    Added symbols for iixprotext catalog.
**	7-jul-93 (robf)
**	     Added DM_[BI]_IILABAUDIT for iilabelaudit
**	26-jul-1993 (bryanp)
**	    Add FUNC_EXTERN for dmf_call(), and discussion of why prototyping
**	    is gross for this routine.
**      10-sep-1993 (pearl)
**          Add E_DM0152_DB_INCONSIST_DETAIL to report database name when it is
**          inconsistent.
**	01-oct-93 (jrb)
**	    Added W_DM5422_IIDBDB_NOT_JOURNALED.
**	17-nov-93 (stephenb)
**	    Added E_DM0156_DROP_SXA_SECURITY_PRIV.
**      30-aug-1994 (cohmi01)
**          Added E_DM006E_NON_BTREE_GETPREV.
**	09-jan-95 (nanpr01)
**	    Added E_DM0160_MODIFY_ALTER_STATUS_ERR
**	    Added E_DM0161_LOG_INCONSISTENT_TABLE_ERR
**	    Added E_DM0162_PHYS_INCONSISTENT_TABLE_ERR
**	18-jan-1995 (cohmi01)
**	    For RAW I/O, added RAWEXT_TAB defines for iirawextents table.
**	24-apr-1995 (cohmi01)
**	    For IOMASTER server, added E_DM0163_WRITE_ALONG_FAILURE,
**	    E_DM0164_READ_AHEAD_FAILURE, DMC_WRITE_ALONG, DMC_READ_AHEAD
**      17-may-95 (dilma04)
**          Added E_DM0170_READONLY_TABLE_INDEX_ERR 
**	08-may-1995 (shero03)
**	    For RTree, added defines for iirange table.
**	22-may-1995 (cohmi01)
**	    Added DMR_COUNT for aggregate optimization project.
**	21-aug-1995 (cohmi01)
**	    DMR_COUNT changed to generalized DMR_AGGREGATE request.
**      21-aug-1995 (tchdi01)
**          Added error flags for processing the production mode related
**          errors
**      06-mar-1996 (stial01)
**          Variable Page Size: added E_DM0157_NO_BMCACHE_BUFFERS
**          added E_DM0158_DMF_MAXTUPLEN_EXCEEDED
**	24-may-96 (stephenb)
**	    add DMC_REP_QMAN define for replicator queue management thread.
**      23-jul-1996 (ramra01 for bryanp)
**          Add new columns to iiattribute in support of Alter Table.
**          Add DMU_ALTER_TABLE opcode to the DM_OPERATION codes list.
**          Add E_DM00A5_ATBL_UNSUPPORTED.
**          Add E_DM00A6_ATBL_COL_INDEX_KEY.
**          Add E_DM00A7_ATBL_COL_INDEX_SEC.
**          Add E_DM0169_ALTER_TABLE_SUPP.
**	08-jul-96 (toumi01)
**	    Modified to support raat on axp.osf (64-bit Digital Unix).
**	    For raat facility ptr_size is used to chain buffers, so
**	    add pad to account for the fact that sizeof(PTR)=8 and
**	    sizeof(i4)=4 on this platform.
**	23-aug-96 (nick)
**	    Add E_DM0168_INCONSISTENT_INDEX_ERR.
**	13-dec-96 (cohmi01)
**	    Add Errors E_DM00A5, E_DM00A6, E_DM00A7
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added E_DM0171_BAD_CACHE_PROTOCOL and E_DM0172_DMCM_LOCK_FAIL.
**      14-nov-1996 (nanpr01)
**          bug 79257 - Cannot modify page sizes for in-memory temp table.
**	26-nov-1996 (nanpr01)
**	    Added message to log a warning if a pagesize passed to buffer
**	    manager is not configured. E_DM0184_PAGE_SIZE_NOT_CONF
**	02-dec-1996 (nanpr01)
**	    Fix up alter table messages. Added E_DM0185_MAX_COL_EXCEEDED.
**      27-feb-1997 (stial01)
**          Added E_DM010E_SET_LOCKMODE_ROW
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	23-Jan-1998 (hanch04)
**	    Change DM_LOC_MAX from 50 to 256
**	28-May-1998 (hanch04)
**	    Made DM_LOC_MAX conditional for INGRESII
**	17-Apr-1998 (jenjo02)
**	    Removed DMC_WRITE_BEHIND, added DMC_START_WB_THREADS.
**      18-jan-1999 (stial01)
**          Added W_DM5062_INDEX_KPERPAGE, W_DM5063_LEAF_KPERPAGE
**      15-Mar-1999 (hanal04)
**          Added E_DM9265_ADD_DB to allow E_DM004D_LOCK_TIMER_EXPIRED
**          to be folded to E_DM9265_ADD_DB in scf. b55788.
**      21-mar-1999 (nanpr01)
**          Added message for raw location support.
**      22-Mar-1999 (hweho01)
**          Extended the change dated 08-jul-96 by toumi01 to ris_u64
**          (AIX 64-bit platform).
**	24-Aug-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to list of 64-Bit platforms using
**	    modified raat support.
**	19-Apr-2001 (jenjo02)
**	    Added E_DM0194_BAD_RAW_PAGE_NUMBER.
**	11-may-2001 (devjo01)
**	    Added DMC_CSP_x operation codes for s103715.
**	08-jun-2001 (abbjo03)
**	    Added E_DM00AB_CANT_MAP_SHARED_MEM.
**	04-Dec-2001 (jenjo02)
**	    Added E_DM0199_INVALID_ROOT_LOCATION.
**	05-Mar-2002 (jenjo02)
**	    Added support for iisequence catalog.
**	20-Aug-2002 (jenjo02)
**	    Added E_DM006B_NOWAIT_LOCK_BUSY for TIMEOUT=NOWAIT
**      22-Oct-2002 (stial01)
**          Added DMX_XA_ABORT
**	19-Dec-2003 (schka24)
**	    Add the partitioning catalogs.
**	29-Jan-2004 (schka24)
**	    Remove iipartition, we didn't need it.
**      18-Feb-2004 (stial01)
**          DM_COLUMN changes for 256k rows
**	29-Apr-2004 (gorvi01)
**		Added constant for DMM_DEL_LOC. 
**		Changed value of DMM_NEXT_OP constant. 
**	29-Apr-2004 (gorvi01)
**		Added error code for location not existing E_DM0200_ERROR_DEL_LOC.
**		Added error code for files existing in directory
**		E_DM011E_ERROR_FILE_EXIST.
**      14-Jul-2004 (hanal04) Bug 112283 INGSRV2819
**          Added W_DM5064_BAD_ATTRIBUTE
**      20-Sep-2004 (devjo01)
**         Added E_DM006C_SERVER_CLASS_ELSEWHERE.
**	20-Dec-2004 (jenjo02)
**	    Deleted E_DM0200_ERROR_DEL_LOC which was overloaded and
**	    wrongly defined as 0x0201L,
**	    changed E_DM011E_ERROR_FILE_EXIST which was duplicated
**	    to E_DM019E_ERROR_FILE_EXIST.
**	29-Dec-2004 (schka24)
**	    Remove non-useful index iixprotect.
**	    Move config DSC_xxx versions here for upgradedb to see.
**	06-Apr-2005 (devjo01)
**	    Replace depreciated & unused E_DM006B_NOWAIT_LOCK_BUSY
**	    with E_DM006B_SERVER_CLASS_ONCE.
**      02-mar-2005 (stial01)
**          Added E_DM0159_NODUPLICATES (b113993)
**	09-may-2005 (devjo01)
**	    Add DMC_DIST_DEADLOCK_THR as the opcode to start a
**	    distributed deadlock thread.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	10-may-2005 (raodi01) bug110709 INGSRV2470
**	    Added E_DM0200_MOD_TEMP_TABLE_KEY
**	21-jun-2005 (devjo01)
**	    Add DMC_GLC_SUPPORT_THR as the opcode to start a
**	    GLC support thread.
**	11-Nov-2005 (jenjo02)
**	    Deleted E_DM011F_TRAN_ABORTED which was really
**	    a duplicate of E_DM010C_TRAN_ABORTED.
**      13-feb-2006 (horda03) Bug 112560/INGSRV2880
**          New DMT interfaces DMT_TABLE_LOCKED and
**          DMT_RELEASE_TABLE_LOCKS
**      21-Jan-2009 (horda03) Bug 121519
**          Add E_DM0026_TABLE_ID_WARNING, a warning message to
**          the user that the Database is nearing the DB_TAB_ID_MAX
**          limit for table/index identifiers. When this limit is
**          reaches, no new tables can be created in the DB.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added E_DM0201_NOLG_MUSTLOG_ERROR
**      16-Jun-2009 (hanal04) Bug 122117
**          Added E_DM016B_LOCK_INTR_FA
[@history_template@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DM_MDATA DM_MDATA;

/**
** Name: DMF symbolic constants. 
**
** Description:
**      This describes the valid operation codes for external 
**      DMF calls.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
*/

/**
** Name:  DM_OPERATION - call operation code.
**
** Description:
**      This describes the valid operation codes for external 
**      DMF calls.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      02-sep-86 (jennifer)
**          Added new command DMR_LOAD for qef.
**      03-sep-86 (jennifer)
**          Added new command DMR_DUMP_DATA for qef.
**      25-mar-89 (jennifer)
**          Added new Audit operation DMA_WRITE_RECORD.
**      25-mar-89 (derek)
**          Added new Maintenance operations DMM_CREATE_DB, DMM_DESTROY_DB, DMM_ALTER_DB.
**	14-Apr-89 (teg)
**	    Added new maintenance operations: DMM_LISTFILE, DMM_DROPFILE.
**      25-mar-89 (jennifer)
**          Added interface for security auditing. DMA_SET_STATE
**	07-jan-90 (teresa)
**	    Added new maintenance operation: DMM_FINDDBS
**	23-may-91 (andre)
**	    defined a new control operation: DMC_RESET_EFF_USER_ID.
**	22-sep-1992 (bryanp)
**	    Added operation codes for new thread types to dmf_call:
**	    DMC_RECOVERY_THREAD, DMC_GROUP_COMMIT, DMC_FORCE_ABORT,
**	    DMC_LOGWRITER.
**	02-oct-92 (teresa)
**	    added DMM_CONFIG and DMM_DEL_DUMP_CONFIG to DM_OPERATION
**	18-jan-1993 (bryanp)
**	    Add DMC_CP_TIMER operation code.
**	21-aug-1995 (cohmi01)
**	    Add DMR_AGGREGATE - generalized aggregate processor.
**      22-jul-1996 (ramra01 for bryanp)
**          Add DMU_ALTER_TABLE opcode to the DM_OPERATION codes list.
**      01-aug-95 (nanpr01 for ICL keving)
**          Added DMC_LK_CALLBACK to start the LK Blocking Callback Thread.
**	15-Nov-1996 (jenjo02)
**	    Added DMC_DEADLOCK_THREAD for Deadlock Detector Thread.
**	3-feb-98 (inkdo01)
**	    Added DMC_SORT_THREAD for parallel sort threads.
**	13-apr-98 (inkdo01)
**	    Dropped DMC_SORT_THREAD in favour of factotum threads.
**	17-Apr-1998 (jenjo02)
**	    Removed DMC_WRITE_BEHIND, added DMC_START_WB_THREADS.
**	16-Apr-1999 (hanch04)
**	    Need 4 byte filler for 8 byte PTRs
**	11-may-2001 (devjo01)
**	    Recycled 4,5 & 6 for DMC_CSP_x operation codes.
**	09-Apr-2004 (jenjo02)
**	    Added DMR_UNFIX opcode, E_DM019A_ERROR_UNFIXING_PAGES
**	26-Jun-2006 (jonj)
**	    Delete DMX_XA_ABORT, add DMX_XA_START/_END/_PREPARE/
**	    _COMMIT/_ROLLBACK, add associated messages.
**	19-Sep-2006 (jonj)
**	    Added DMT_INVALIDATE_TCB.
**	04-Oct-2006 (jonj)
**	    Added DMT_CONTROL_LOCK.
**	23-Oct-2007 (kibro01) b118918
**	    Add E_DM01A0_INVALID_ALTCOL_DEFAULT
**	16-Mar-2010 (frima01) SIR 121619
**	    Add IIDBDB_ID defined as 1.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Add LOB functions for manual end-of-row, query end.
**	    Kill the annoying "L", these are ints, not longs.
*/
typedef i4 DM_OPERATION;
/*	1 thru 3 unused */
#define		DMC_CSP_MAIN		4  /* Control operations. */
#define		DMC_CSP_MSGMON		5
#define		DMC_CSP_MSGTHR		6
#define		DMC_RESET_EFF_USER_ID	7
#define		DMC_START_WB_THREADS    8
#define		DMC_FAST_COMMIT		9
#define		DMC_ALTER		10
#define		DMC_SHOW		11
#define		DMC_START_SERVER	12
#define		DMC_STOP_SERVER		13
#define		DMC_BEGIN_SESSION	14
#define		DMC_END_SESSION		15
#define		DMC_OPEN_DB		16
#define		DMC_CLOSE_DB		17
#define		DMC_ADD_DB		18
#define		DMC_DEL_DB		19
#define		DMD_ALTER_CB		20 /* Debug operations. */
#define		DMD_CANCEL_TRACE	21
#define		DMD_DISPLAY_TRACE	22
#define		DMD_GET_CB		23
#define		DMD_RELEASE_CB		24
#define		DMD_SET_TRACE		25
#define		DMD_SHOW_CB		26
#define		DMC_REP_QMAN		27 /* another control op */
#define		DMC_DIST_DEADLOCK_THR   28 /* another control op */
#define		DMC_GLC_SUPPORT_THR	29 /* another control op */
#define		DMR_DELETE		30 /* Record Operations. */
#define		DMR_GET			31
#define		DMR_POSITION		32
#define		DMR_PUT			33
#define		DMR_REPLACE		34
#define		DMR_LOAD		35
#define		DMR_DUMP_DATA		36
#define		DMR_ALTER		37
#define		DMR_AGGREGATE		38
#define		DMR_UNFIX		39
#define		DMPE_END_ROW		40 /* Manual end-of-row for BLOB */
#define		DMPE_QUERY_END		41 /* End-of-query for BLOB */
/* 42 thru 49 unused */
#define		DMT_ALTER		50 /* Table Operations. */
#define		DMT_CLOSE		51
#define		DMT_CREATE_TEMP		52
#define		DMT_DELETE_TEMP		53
#define		DMT_OPEN		54
#define		DMT_SHOW		55
#define		DMT_SORT_COST		56
#define		DMT_INVALIDATE_TCB	57
#define		DMT_TABLE_LOCKED	58
#define		DMT_RELEASE_TABLE_LOCKS	59
#define		DMU_CREATE_TABLE	60 /* Utility operations. */
#define		DMU_DESTROY_TABLE	61
#define		DMU_INDEX_TABLE		62
#define		DMU_MODIFY_TABLE	63
#define		DMU_RELOCATE_TABLE	64
#define		DMU_ALTER		65
#define		DMU_SHOW		66
#define		DMU_GET_TABID		67
#define		DMU_CONVERT_TABLE	68
#define		DMX_INFO		69 /* Transaction operations. */
#define		DMX_ABORT		70
#define		DMX_BEGIN		71
#define		DMX_COMMIT		72
#define		DMX_SECURE		73
#define		DMX_SAVEPOINT		74
#define		DMX_RESUME		75
#define		DMM_CREATE_DB		76
#define		DMM_DESTROY_DB		77
#define		DMM_ALTER_DB		78
#define		DMA_WRITE_RECORD	79
#define		DMM_LISTFILE		80
#define		DMM_DROPFILE		81
#define		DMM_ADD_LOC		82
#define		DMA_SET_STATE		83
#define		DMA_SHOW_STATE		84
#define		DMM_FINDDBS		85
#define		DMC_RECOVERY_THREAD	86
#define		DMC_CHECK_DEAD		87
#define		DMC_GROUP_COMMIT	88
#define		DMC_FORCE_ABORT		89
#define		DMC_LOGWRITER		90
#define		DMM_CONFIG		91
#define		DMM_DEL_DUMP_CONFIG	92
#define		DMC_CP_TIMER		93
/*	94 unused */
#define		DMC_WRITE_ALONG         95
#define		DMC_READ_AHEAD          96
#define		DMU_SETPRODUCTION       97
#define		DMU_ALTER_TABLE		98
#define		DMC_LK_CALLBACK         99
#define		DMC_DEADLOCK_THREAD	100
#define		DMU_PINDEX_TABLE	101
#define		DMS_OPEN_SEQ		102
#define		DMS_CLOSE_SEQ		103
#define		DMS_NEXT_SEQ		104
#define		DMM_DEL_LOC		105
#define		DMX_XA_START		106
#define		DMX_XA_END		107
#define		DMX_XA_PREPARE		108
#define		DMX_XA_COMMIT		109
#define		DMX_XA_ROLLBACK		110
#define		DMT_CONTROL_LOCK	111
/* Keep DM_NEXT_OP up to date!  should be last op + 1 */
#define		DM_NEXT_OP		112

/* typedef end 
**/

/*}
** Name:  DM_DATA - Data type for passing DMF variable length data.
**
** Description:
**      This typedef defines the structure for passing variable
**      length data to and from DMF.  
**
** History:
**      01-sep-85 (jennifer)
**          Created.
*/
typedef struct _DM_DATA
{
    char            *data_address;          /* Pointer to data space.  */
    i4         data_in_size;           /* Input data length. */
    i4         data_out_size;          /* Output data length. */
}   DM_DATA;

/*}
** Name:  DM_MDATA - Data type for passing DMF list of records.
**
** Description:
**      This typedef defines the structure for passing variable number of
**      data records to and from DMF.
**
**	This structure is defined to look just like a QEF_DATA struct so
**	that QEF can pass QEF_DATA lists to DMR_LOAD without having to
**	reformat them.  If the QEF_DATA structure changes, then this 
**	struct should change also (or visa versa).
**
** History:
**      12-mar-87 (rogerk)
**          Created.
**      30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _DM_MDATA
{
    DM_MDATA	    *data_next;		    /* Points to next data record */
    i4		    data_size;              /* Data length. */
    char            *data_address;          /* Pointer to data space.  */
};

/*}
** Name:  DM_ERR - Data type for common DMF error block.
**
** Description:
**      This typedef defines the structure for passing error 
**      codes to (for testing) and from DMF.  The DMF error code
**      is returned to the caller in this structure not as the
**      return value.  The only valid return values are:
**
**          E_DB_OK        -  Function performed successfully, no additional
**                            processing required. 
**          E_DB_ERROR     -  Function had error, take corrective action.
**          E_DB_WARN      -  Function had acceptable error condition such
**                            as end of file, check error code.
**          E_DB_FATAL     -  Function had fatal error, request abort.
**          E_DB_INFO      -  Function performed successfully, positive 
**                            condition indicated, such as table created.
**
**      When the rturn value is other than E_DB_OK, then the DMF error code
**      explicitly describing the error is returned in this structure.  This 
**      structure also provides for additional information, such as an index
**      specifying which entry in an input array is associated with the error.      
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**	15-feb-90 (teg)
**	    drop in check/patch table message constants.
**	27-feb-90 (teg)
**	    added check isam messages.
**      14-nov-1996 (nanpr01)
**          bug 79257 - Cannot modify page sizes for in-memory temp table.
**      26-nov-1996 (nanpr01)
**          Added message to log a warning if a pagesize passed to buffer
**          manager is not configured. E_DM0184_PAGE_SIZE_NOT_CONF.
**	06-dec-1996 (nanpr01)
**	    Bug 79484 : Added error message E_DM0187_BAD_OLD_LOCATION_NAME.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**      15-Mar-1999 (hanal04)
**          Added E_DM9265_ADD_DB to allow E_DM004D_LOCK_TIMER_EXPIRED
**          to be folded to E_DM9265_ADD_DB in scf. b55788.
**	24-apr-99 (stephenb)
**	    Add new errors for verifydb peripheral (coupon) checking
**    07-mar-2000 (gupsh01)
**        Added new error E_DM0192_CREATEDB_RONLY_NOTEXIST for readonly  
**        database creation error.
**      22-May-2000 (islsa01)
**          Bug 90402 : Added error message E_DM011F_TRAN_ABORTED.
**      1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 - Added Error message E_DM0173_RAAT_SI_TIMER
**	15-Oct-2001 (jenjo02)
**	    Added E_DM0195/6/7/8.
**	05-Mar-2002 (jenjo02)
**	    Added DM9D01 for Sequence Generators.
**	05-Mar-2003 (inkdo01)
**	    Added DM9D02 for Sequence Generators.
**	15-Jan-2004 (schka24)
**	    Added partitioning errors.
**	11-Mar-2004 (jenjo02)
**	    Added E_DM0024_NO_PARTITION_INFO,
**		  E_DM0025_PARTITION_MISMATCH
**	5-Apr-2004 (schka24)
**	    Take dm0023 back out, not used any more.
**	20-Apr-2004 (jenjo02)
**	    Added E_DM00AC_LOGFULL_COMMIT
**	15-Apr-2004 (gupsh01)
**	    Added alter table alter column error 
**	    E_DM019B_INVALID_ALTCOL_PARAM.
**	    E_DM019C_INVALID_ALTCOL_PARAM.
**	    E_DM019D_ADF_CONVERSION_FAILED.
**	27-feb-2005 (schka24)
**	    Move 9D00 sequence error here for qef to see.
**	1-Nov-2007 (kibro01) b119003
**	    Added E_DM9581_REP_SHADOW_DUPLICATE
**	12-Apr-2008 (kschendel)
**	    Added "dmf user error" for pass-back to qef.
**      21-Jan-2009 (horda03)
**          Add E_DM0026_TABLE_ID_WARNING.
**      10-Jul-2009 (hanal04) Bug 122130
**          Added E_DM0027_DB_INCONSISTENT_RO. 
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add E_DM0020_UNABLE_TO_SERIALIZE,
**	    E_DM0028_UNABLE_TO_MAKE_CONSISTENT,
**	    E_DM0029_ROW_UPDATE_CONFLICT,
**	    E_DM0012_MVCC_INCOMPATIBLE,
**	    E_DM0013_MVCC_DISABLED
*/
typedef struct _DM_ERR
{
    i4         err_code;               /* DMF error code  */
#define             E_DM_MASK                           (0x00030000L)
#define             E_DM0000_OK                         0L
#define             E_DM0001_EXCPTN_FRM_EXCPTN          (E_DM_MASK + 0x0001L)
#define             E_DM0006_BAD_ATTR_FLAGS             (E_DM_MASK + 0x0006L)
#define             E_DM0007_BAD_ATTR_NAME              (E_DM_MASK + 0x0007L)
#define             E_DM0008_BAD_ATTR_PRECISION         (E_DM_MASK + 0x0008L)
#define             E_DM0009_BAD_ATTR_SIZE              (E_DM_MASK + 0x0009L)
#define             E_DM000A_BAD_ATTR_TYPE              (E_DM_MASK + 0x000AL)
#define             E_DM000B_BAD_CB_LENGTH              (E_DM_MASK + 0x000BL)
#define             E_DM000C_BAD_CB_TYPE                (E_DM_MASK + 0x000CL)
#define             E_DM000D_BAD_CHAR_ID                (E_DM_MASK + 0x000DL)
#define             E_DM000E_BAD_CHAR_VALUE             (E_DM_MASK + 0x000EL)
#define             E_DM000F_BAD_DB_ACCESS_MODE         (E_DM_MASK + 0x000FL)
#define             E_DM0010_BAD_DB_ID                  (E_DM_MASK + 0x0010L)
#define             E_DM0011_BAD_DB_NAME                (E_DM_MASK + 0x0011L)
#define             E_DM0012_MVCC_INCOMPATIBLE          (E_DM_MASK + 0x0012L)
#define             E_DM0013_MVCC_DISABLED              (E_DM_MASK + 0x0013L)
#define             E_DM001A_BAD_FLAG                   (E_DM_MASK + 0x001AL)
#define             E_DM001B_BAD_INDEX                  (E_DM_MASK + 0x001BL)
#define             E_DM001C_BAD_KEY_SEQUENCE           (E_DM_MASK + 0x001CL)
#define             E_DM001D_BAD_LOCATION_NAME          (E_DM_MASK + 0x001DL)
#define             E_DM001E_DUP_LOCATION_NAME          (E_DM_MASK + 0x001EL)
#define             E_DM001F_LOCATION_LIST_ERROR        (E_DM_MASK + 0x001FL)
#define             E_DM0020_UNABLE_TO_SERIALIZE        (E_DM_MASK + 0x0020L)
#define             E_DM0021_TABLES_TOO_MANY            (E_DM_MASK + 0x0021L)
#define             E_DM0022_BAD_MASTER_OP		(E_DM_MASK + 0x0022L)
#define		    E_DM0023_USER_ERROR			(E_DM_MASK + 0x0023)
#define		    E_DM0024_NO_PARTITION_INFO		(E_DM_MASK + 0x0024L)
#define		    E_DM0025_PARTITION_MISMATCH		(E_DM_MASK + 0x0025L)
#define		    E_DM0026_TABLE_ID_WARNING		(E_DM_MASK + 0x0026L)
#define             E_DM0027_DB_INCONSISTENT_RO         (E_DM_MASK + 0x0027L)
#define		    E_DM0028_UNABLE_TO_MAKE_CONSISTENT	(E_DM_MASK + 0x0028L)
#define		    E_DM0029_ROW_UPDATE_CONFLICT	(E_DM_MASK + 0x0029L)
#define             E_DM002A_BAD_PARAMETER              (E_DM_MASK + 0x002AL)
#define             E_DM002B_BAD_RECORD_ID              (E_DM_MASK + 0x002BL)
#define             E_DM002C_BAD_SAVEPOINT_NAME         (E_DM_MASK + 0x002CL)
#define             E_DM002D_BAD_SERVER_ID              (E_DM_MASK + 0x002DL)
#define             E_DM002E_BAD_SERVER_NAME            (E_DM_MASK + 0x002EL)
#define             E_DM002F_BAD_SESSION_ID             (E_DM_MASK + 0x002FL)
#define             E_DM0030_BAD_SESSION_NAME           (E_DM_MASK + 0x0030L)
#define             E_DM0039_BAD_TABLE_NAME             (E_DM_MASK + 0x0039L)
#define             E_DM003A_BAD_TABLE_OWNER            (E_DM_MASK + 0x003AL)
#define             E_DM003B_BAD_TRAN_ID                (E_DM_MASK + 0x003BL)
#define             E_DM003C_BAD_TID                    (E_DM_MASK + 0x003CL)
#define             E_DM003E_DB_ACCESS_CONFLICT         (E_DM_MASK + 0x003EL)
#define             E_DM003F_DB_OPEN                    (E_DM_MASK + 0x003FL)
#define             E_DM0040_DB_OPEN_QUOTA_EXCEEDED     (E_DM_MASK + 0x0040L)
#define             E_DM0041_DB_QUOTA_EXCEEDED          (E_DM_MASK + 0x0041L)
#define             E_DM0042_DEADLOCK                   (E_DM_MASK + 0x0042L)
#define             E_DM0044_DELETED_TID                (E_DM_MASK + 0x0044L)
#define             E_DM0045_DUPLICATE_KEY              (E_DM_MASK + 0x0045L)
#define             E_DM0046_DUPLICATE_RECORD           (E_DM_MASK + 0x0046L)
#define             E_DM0047_CHANGED_TUPLE		(E_DM_MASK + 0x0047L) 
#define		    E_DM0048_SIDUPLICATE_KEY		(E_DM_MASK + 0x0048L)
#define             E_DM004A_INTERNAL_ERROR             (E_DM_MASK + 0x004AL)
#define             E_DM004B_LOCK_QUOTA_EXCEEDED        (E_DM_MASK + 0x004BL)
#define             E_DM004C_LOCK_RESOURCE_BUSY         (E_DM_MASK + 0x004CL)
#define             E_DM004D_LOCK_TIMER_EXPIRED         (E_DM_MASK + 0x004DL)
#define		    E_DM004E_LOCK_INTERRUPTED		(E_DM_MASK + 0x004EL)
#define             E_DM004F_LOCK_RETRY			(E_DM_MASK + 0x004FL)
#define             E_DM0050_TABLE_NOT_LOADABLE		(E_DM_MASK + 0x0050L)
#define             E_DM0052_COMPRESSION_CONFLICT	(E_DM_MASK + 0x0052L)
#define             E_DM0053_NONEXISTENT_DB             (E_DM_MASK + 0x0053L)
#define             E_DM0054_NONEXISTENT_TABLE          (E_DM_MASK + 0x0054L)
#define             E_DM0055_NONEXT                     (E_DM_MASK + 0x0055L)
#define             E_DM0056_NOPART                     (E_DM_MASK + 0x0056L)
#define             E_DM0057_NOROOM                     (E_DM_MASK + 0x0057L)
#define             E_DM0058_NOTIDP                     (E_DM_MASK + 0x0058L)
#define             E_DM0059_NOT_ALL_KEYS               (E_DM_MASK + 0x0059L)
#define             E_DM005A_CANT_MOD_CORE_STRUCT       (E_DM_MASK + 0x005AL)
#define             E_DM005B_SESSION_OPEN               (E_DM_MASK + 0x005BL)
#define             E_DM005C_SESSION_QUOTA_EXCEEDED     (E_DM_MASK + 0x005CL)
#define             E_DM005D_TABLE_ACCESS_CONFLICT      (E_DM_MASK + 0x005DL)
#define             E_DM005E_CANT_UPDATE_SYSCAT         (E_DM_MASK + 0x005EL)
#define             E_DM005F_CANT_INDEX_CORE_SYSCAT	(E_DM_MASK + 0x005FL)
#define             E_DM0060_TRAN_IN_PROGRESS           (E_DM_MASK + 0x0060L)
#define             E_DM0061_TRAN_NOT_IN_PROGRESS       (E_DM_MASK + 0x0061L)
#define             E_DM0062_TRAN_QUOTA_EXCEEDED        (E_DM_MASK + 0x0062L)
#define             E_DM0063_TRAN_TABLE_OPEN            (E_DM_MASK + 0x0063L)
#define             E_DM0064_USER_ABORT                 (E_DM_MASK + 0x0064L)
#define             E_DM0065_USER_INTR                  (E_DM_MASK + 0x0065L)
#define		    E_DM0066_BAD_KEY_TYPE		(E_DM_MASK + 0x0066L)
#define		    E_DM0067_READONLY_TABLE_ERR		(E_DM_MASK + 0x0067L)
#define		    E_DM0068_MODIFY_READONLY_ERR	(E_DM_MASK + 0x0068L)
#define		    E_DM0069_CONCURRENT_IDX_ERR		(E_DM_MASK + 0x0069L)
#define		    E_DM006A_TRAN_ACCESS_CONFLICT	(E_DM_MASK + 0x006AL)
#define		    E_DM006B_SERVER_CLASS_ONCE		(E_DM_MASK + 0x006BL)
#define             E_DM006C_SERVER_CLASS_ELSEWHERE     (E_DM_MASK + 0x006CL)
#define		    E_DM006D_BAD_OPERATION_CODE		(E_DM_MASK + 0x006DL)
#define             E_DM006E_NON_BTREE_GETPREV          (E_DM_MASK + 0x006EL)
#define		    E_DM006F_SERVER_ACTIVE		(E_DM_MASK + 0x006FL)
#define		    E_DM0070_SERVER_STOPPED		(E_DM_MASK + 0x0070L)
#define		    E_DM0071_LOCATIONS_TOO_MANY		(E_DM_MASK + 0x0071L)
#define		    E_DM0072_NO_LOCATION		(E_DM_MASK + 0x0072L)
#define		    E_DM0073_RECORD_ACCESS_CONFLICT	(E_DM_MASK + 0x0073L)
#define		    E_DM0074_NOT_POSITIONED		(E_DM_MASK + 0x0074L)
#define		    E_DM0075_BAD_ATTRIBUTE_ENTRY	(E_DM_MASK + 0x0075L)
#define		    E_DM0076_BAD_INDEX_ENTRY		(E_DM_MASK + 0x0076L)
#define             E_DM0077_BAD_TABLE_CREATE           (E_DM_MASK + 0x0077L)
#define             E_DM0078_TABLE_EXISTS               (E_DM_MASK + 0x0078L)
#define             E_DM0079_BAD_OWNER_NAME             (E_DM_MASK + 0x0079L)
#define             E_DM007A_BAD_DEVICE_ENTRY           (E_DM_MASK + 0x007AL)
#define             E_DM007B_CANT_CREATE_TABLEID	(E_DM_MASK + 0x007BL)
#define		    E_DM007C_BAD_KEY_DIRECTION		(E_DM_MASK + 0x007CL)
#define		    E_DM007D_BTREE_BAD_KEY_LENGTH	(E_DM_MASK + 0x007DL)
#define		    E_DM007E_LOCATION_EXISTS		(E_DM_MASK + 0x007EL)
#define		    E_DM007F_ERROR_STARTING_SERVER	(E_DM_MASK + 0x007FL)
#define		    E_DM0080_BAD_LOCATION_AREA          (E_DM_MASK + 0x0080L)
#define		    E_DM0081_NO_LOGGING_SYSTEM          (E_DM_MASK + 0x0081L)
#define		    E_DM0083_ERROR_STOPPING_SERVER	(E_DM_MASK + 0x0083L)
#define		    E_DM0084_ERROR_ADDING_DB		(E_DM_MASK + 0x0084L)
#define		    E_DM0085_ERROR_DELETING_DB		(E_DM_MASK + 0x0085L)
#define		    E_DM0086_ERROR_OPENING_DB		(E_DM_MASK + 0X0086L)
#define		    E_DM0087_ERROR_CLOSING_DB		(E_DM_MASK + 0X0087L)
#define		    E_DM0088_ERROR_ALTERING_SERVER	(E_DM_MASK + 0X0088L)
#define		    E_DM0089_ERROR_SHOWING_SERVER	(E_DM_MASK + 0X0089L)
#define		    E_DM008A_ERROR_GETTING_RECORD	(E_DM_MASK + 0X008AL)
#define		    E_DM008B_ERROR_PUTTING_RECORD	(E_DM_MASK + 0X008BL)
#define		    E_DM008C_ERROR_REPLACING_RECORD	(E_DM_MASK + 0X008CL)
#define		    E_DM008D_ERROR_DELETING_RECORD	(E_DM_MASK + 0X008DL)
#define		    E_DM008E_ERROR_POSITIONING		(E_DM_MASK + 0X008EL)
#define		    E_DM008F_ERROR_OPENING_TABLE	(E_DM_MASK + 0X008FL)
#define		    E_DM0090_ERROR_CLOSING_TABLE	(E_DM_MASK + 0X0090L)
#define		    E_DM0091_ERROR_MODIFYING_TABLE	(E_DM_MASK + 0X0091L)
#define		    E_DM0092_ERROR_INDEXING_TABLE	(E_DM_MASK + 0X0092L)
#define		    E_DM0093_ERROR_SORTING		(E_DM_MASK + 0X0093L)
#define		    E_DM0094_ERROR_BEGINNING_TRAN	(E_DM_MASK + 0X0094L)
#define		    E_DM0095_ERROR_COMMITING_TRAN	(E_DM_MASK + 0X0095L)
#define		    E_DM0096_ERROR_ABORTING_TRAN	(E_DM_MASK + 0X0096L)
#define		    E_DM0097_ERROR_SAVEPOINTING		(E_DM_MASK + 0X0097L)
#define             E_DM0098_NOJOIN                     (E_DM_MASK + 0X0098L)
#define		    E_DM0099_ERROR_RESUME_TRAN		(E_DM_MASK + 0X0099L)
#define		    E_DM009A_ERROR_SECURE_TRAN		(E_DM_MASK + 0X009AL)
#define             E_DM009B_ERROR_CHK_PATCH_TABLE      (E_DM_MASK + 0X009B)
#define             E_DM009D_BAD_TABLE_DESTROY          (E_DM_MASK + 0x009DL)
#define		    E_DM009E_CANT_OPEN_VIEW		(E_DM_MASK + 0x009EL)
#define		    E_DM009F_ILLEGAL_OPERATION		(E_DM_MASK + 0x009FL)
#define		    E_DM00A0_UNKNOWN_COLLATION		(E_DM_MASK + 0x00A0L)
#define		    E_DM00A1_ERROR_CONVERTING_TBL	(E_DM_MASK + 0x00A1L)
#define		    E_DM00A2_CANT_CREATE_SECID		(E_DM_MASK + 0x00A2L)
#define		    E_DM00A3_SECID_TOO_MANY		(E_DM_MASK + 0x00A3L)
#define		    E_DM00A4_SYSMOD_TOO_MANY_ATTS	(E_DM_MASK + 0x00A4L)
#define		    E_DM00A5_ATBL_UNSUPPORTED    	(E_DM_MASK + 0x00A5L)
#define		    E_DM00A6_ATBL_COL_INDEX_KEY  	(E_DM_MASK + 0x00A6L)
#define		    E_DM00A7_ATBL_COL_INDEX_SEC  	(E_DM_MASK + 0x00A7L)
#define		    E_DM00A8_BAD_ATT_ENTRY		(E_DM_MASK + 0x00A8L)
#define		    E_DM00A9_BAD_ATT_COUNT  		(E_DM_MASK + 0x00A9L)
#define		    E_DM00AA_BAD_ATT_KEY_COUNT		(E_DM_MASK + 0x00AAL)
#define		    E_DM00AB_CANT_MAP_SHARED_MEM	(E_DM_MASK + 0x00ABL)
#define		    E_DM00AC_LOGFULL_COMMIT		(E_DM_MASK + 0x00ACL)
#define		    E_DM00D0_LOCK_MANAGER_ERROR		(E_DM_MASK + 0x00D0L)
#define		    E_DM00D1_BAD_SYSCAT_MOD		(E_DM_MASK + 0x00D1L)
#define		    E_DM00E0_BAD_CB_PTR			(E_DM_MASK + 0x00E0L)
#define		    E_DM0100_DB_INCONSISTENT		(E_DM_MASK + 0x0100L)
#define		    E_DM0101_SET_JOURNAL_ON		(E_DM_MASK + 0x0101L)
#define		    E_DM0102_NONEXISTENT_SP		(E_DM_MASK + 0x0102L)
#define		    E_DM0103_TUPLE_TOO_WIDE		(E_DM_MASK + 0x0103L)
#define		    E_DM0104_ERROR_RELOCATING_TABLE	(E_DM_MASK + 0x0104L)
#define		    E_DM0105_ERROR_BEGIN_SESSION        (E_DM_MASK + 0x0105L)
#define             E_DM0106_ERROR_ENDING_SESSION       (E_DM_MASK + 0x0106L)
#define             E_DM0107_ERROR_ALTERING_SESSION     (E_DM_MASK + 0x0107L)
#define             E_DM0108_ERROR_DUMPING_DATA         (E_DM_MASK + 0x0108L)
#define             E_DM0109_ERROR_LOADING_DATA         (E_DM_MASK + 0x0109L)
#define             E_DM010A_ERROR_ALTERING_TABLE       (E_DM_MASK + 0x010AL)
#define             E_DM010B_ERROR_SHOWING_TABLE        (E_DM_MASK + 0x010BL)
#define             E_DM010C_TRAN_ABORTED		(E_DM_MASK + 0x010CL)
#define             E_DM010D_ERROR_ALTERING_DB          (E_DM_MASK + 0x010DL)
#define             E_DM010E_SET_LOCKMODE_ROW           (E_DM_MASK + 0x010EL)
#define		    E_DM010F_ISAM_BAD_KEY_LENGTH	(E_DM_MASK + 0x010FL)
#define		    E_DM0110_COMP_BAD_KEY_LENGTH	(E_DM_MASK + 0x0110L)
#define		    E_DM0111_MOD_IDX_UNIQUE		(E_DM_MASK + 0x0111L)
#define		    E_DM0112_RESOURCE_QUOTA_EXCEED	(E_DM_MASK + 0x0112L)
#define		    E_DM0113_DB_INCOMPATABLE		(E_DM_MASK + 0x0113L)
#define		    E_DM0114_FILE_NOT_FOUND		(E_DM_MASK + 0x0114L)
#define		    E_DM0115_FCMULTIPLE			(E_DM_MASK + 0x0115L)
#define		    E_DM0116_FAST_COMMIT		(E_DM_MASK + 0x0116L)
#define		    E_DM0117_WRITE_BEHIND		(E_DM_MASK + 0x0117L)
#define		    E_DM0118_CANT_OPEN_GATEWAY		(E_DM_MASK + 0x0118L)
#define		    E_DM0119_TRAN_ID_NOT_UNIQUE		(E_DM_MASK + 0x0119L)
#define		    E_DM011A_ERROR_DB_DIR		(E_DM_MASK + 0x011AL)
#define		    E_DM011B_ERROR_DB_CREATE		(E_DM_MASK + 0x011BL)
#define		    E_DM011C_ERROR_DB_DESTROY		(E_DM_MASK + 0x011CL)
#define		    E_DM011D_ERROR_DB_ALTER 		(E_DM_MASK + 0x011DL)
#define		    E_DM011E_ERROR_ADD_LOC		(E_DM_MASK + 0x011EL)
#define		    E_DM011F_INVALID_TXN_STATE		(E_DM_MASK + 0x011FL)
#define		    E_DM0120_DIS_TRAN_UNKNOWN		(E_DM_MASK + 0x0120L)
#define		    E_DM0121_DIS_TRAN_OWNER		(E_DM_MASK + 0x0121L)
#define		    E_DM0122_CANT_SECURE_IN_CLUSTER	(E_DM_MASK + 0x0122L)
#define		    E_DM0123_BAD_CACHE_NAME		(E_DM_MASK + 0x0123L)
#define		    E_DM0124_DB_EXISTS			(E_DM_MASK + 0x0124L)
#define		    E_DM0125_DDL_SECURITY_ERROR    	(E_DM_MASK + 0X0125L)
#define             E_DM0126_ERROR_WRITING_SECAUDIT     (E_DM_MASK + 0x0126L)
#define             E_DM0127_ERROR_SETTING_SECAUDIT     (E_DM_MASK + 0x0127L)
#define		    E_DM0128_SECURITY_ATTR_BADNAME	(E_DM_MASK + 0x0128L)
#define		    E_DM0129_SECURITY_CANT_UPDATE	(E_DM_MASK + 0x0129L)
#define		    E_DM012A_CONFIG_NOT_FOUND		(E_DM_MASK + 0x012AL)
#define		    E_DM012B_ERROR_XA_PREPARE		(E_DM_MASK + 0x012BL)
#define		    E_DM012C_ERROR_XA_COMMIT		(E_DM_MASK + 0x012CL)
#define		    E_DM012D_ERROR_XA_ROLLBACK		(E_DM_MASK + 0x012DL)
#define		    E_DM012E_ERROR_XA_START		(E_DM_MASK + 0x012EL)
#define		    E_DM012F_ERROR_XA_END		(E_DM_MASK + 0x012FL)
#define             E_DM0130_DMMLIST_ERROR              (E_DM_MASK + 0X0130L)
#define             E_DM0131_DMMDEL_ERROR               (E_DM_MASK + 0X0131L)
#define             E_DM0132_ILLEGAL_STMT		(E_DM_MASK + 0X0132L)
#define             E_DM0133_ERROR_ACCESSING_DB		(E_DM_MASK + 0X0133L)
#define		    E_DM0134_DIS_DB_UNKNOWN		(E_DM_MASK + 0x0134L)
#define             E_DM0135_NO_GATEWAY			(E_DM_MASK + 0X0135L)
#define             E_DM0136_GATEWAY_ERROR		(E_DM_MASK + 0X0136L)
#define             E_DM0137_GATEWAY_ACCESS_ERROR	(E_DM_MASK + 0X0137L)
#define		    E_DM0138_TOO_MANY_PAGES		(E_DM_MASK + 0x0138L)
#define		    E_DM0139_ERROR_ALTERING_RCB		(E_DM_MASK + 0x0139L)
#define		    E_DM013A_INDEX_COUNT_MISMATCH	(E_DM_MASK + 0x013AL)
#define		    E_DM013B_LOC_NOT_AUTHORIZED		(E_DM_MASK + 0x013BL)
#define		    E_DM013C_BAD_LOC_TYPE		(E_DM_MASK + 0x013CL)
#define		    E_DM013D_LOC_NOT_EXTENDED		(E_DM_MASK + 0x013DL)
#define		    E_DM013E_TOO_FEW_LOCS		(E_DM_MASK + 0x013EL)
#define		    E_DM013F_TOO_MANY_LOCS		(E_DM_MASK + 0x013FL)
#define		    E_DM0140_EMPTY_DIRECTORY		(E_DM_MASK + 0x0140L)
#define		    E_DM0141_GWF_DATA_CVT_ERROR		(E_DM_MASK + 0x0141L)
#define		    E_DM0142_GWF_ALREADY_REPORTED	(E_DM_MASK + 0x0142L)
#define		    E_DM0143_SAP_NOT_RUNNING		(E_DM_MASK + 0x0143L)
#define		    E_DM0144_NONFATAL_FINDBS_ERROR	(E_DM_MASK + 0x0144L)
#define		    E_DM0145_FATAL_FINDBS_ERROR		(E_DM_MASK + 0x0145L)
#define		    E_DM0146_SETLG_XCT_ABORT		(E_DM_MASK + 0x0146L)
#define		    E_DM0147_NOLG_BACKUP_ERROR		(E_DM_MASK + 0x0147L)
#define 	    E_DM0148_USER_QUAL_ERROR		(E_DM_MASK + 0x0148L)
#define 	    E_DM0149_SETLG_SVPT_ABORT		(E_DM_MASK + 0x0149L)
#define		    E_DM014A_CHECK_DEAD			(E_DM_MASK + 0x014AL)
#define		    E_DM014B_GROUP_COMMIT		(E_DM_MASK + 0x014BL)
#define		    E_DM014C_FORCE_ABORT		(E_DM_MASK + 0x014CL)
#define		    E_DM014D_LOGWRITER			(E_DM_MASK + 0x014DL)
#define		    E_DM014E_RECOVERY_THREAD		(E_DM_MASK + 0x014EL)
#define		    E_DM014F_CP_TIMER			(E_DM_MASK + 0x014FL)
#define             E_DM0150_DUPLICATE_KEY_STMT         (E_DM_MASK + 0x0150L)
#define		    E_DM0151_SIDUPLICATE_KEY_STMT	(E_DM_MASK + 0x0151L)
#define             E_DM0152_DB_INCONSIST_DETAIL        (E_DM_MASK + 0x0152L)
#define		    E_DM0154_DUPLICATE_ROW_NOTUPD	(E_DM_MASK + 0x0154L)
#define		    E_DM0155_B1_DB_NEEDS_B1_SERVER	(E_DM_MASK + 0x0155L)
#define		    E_DM0156_DROP_SXA_SECURITY_PRIV	(E_DM_MASK + 0x0156L)
#define             E_DM0157_NO_BMCACHE_BUFFERS         (E_DM_MASK + 0x0157L)
#define             E_DM0158_DMF_MAXTUPLEN_EXCEEDED     (E_DM_MASK + 0x0158L)
#define		    E_DM0159_NODUPLICATES		(E_DM_MASK + 0x0159L)
#define		    E_DM0160_MODIFY_ALTER_STATUS_ERR	(E_DM_MASK + 0x0160L)
#define		    E_DM0161_LOG_INCONSISTENT_TABLE_ERR (E_DM_MASK + 0x0161L)
#define		    E_DM0162_PHYS_INCONSISTENT_TABLE_ERR (E_DM_MASK + 0x0162L)
#define		    E_DM0163_WRITE_ALONG_FAILURE        (E_DM_MASK + 0x0163L)
#define		    E_DM0164_READ_AHEAD_FAILURE         (E_DM_MASK + 0x0164L)
#define		    E_DM0165_AGG_BAD_OPTS               (E_DM_MASK + 0x0165L)
#define		    E_DM0166_AGG_ADE_FAILED             (E_DM_MASK + 0x0166L)
#define		    E_DM0167_AGG_ADE_EXCEPTION          (E_DM_MASK + 0x0167L)
#define		    E_DM0168_INCONSISTENT_INDEX_ERR	(E_DM_MASK + 0x0168L)
#define             E_DM0169_ALTER_TABLE_SUPP           (E_DM_MASK + 0x0169L)
#define             E_DM016A_ERROR_XA_DISASSOC          (E_DM_MASK + 0x016AL)
#define             E_DM016B_LOCK_INTR_FA               (E_DM_MASK + 0x016BL)

#define             E_DM0170_READONLY_TABLE_INDEX_ERR   (E_DM_MASK + 0x0170L)
#define             E_DM0171_BAD_CACHE_PROTOCOL         (E_DM_MASK + 0x0171L)
#define             E_DM0172_DMCM_LOCK_FAIL             (E_DM_MASK + 0x0172L)
#define             E_DM0173_RAAT_SI_TIMER              (E_DM_MASK + 0x0173L)

# define            E_DM0180_SPROD_UPD_IIDB             (E_DM_MASK + 0x0180L)
# define            E_DM0181_PROD_MODE_ERR              (E_DM_MASK + 0x0181L)
# define            E_DM0182_PROD_LK_DB_ERR             (E_DM_MASK + 0x0182L)
#define             E_DM0183_CANT_MOD_TEMP_TABLE        (E_DM_MASK + 0x0183L)
#define             E_DM0184_PAGE_SIZE_NOT_CONF		(E_DM_MASK + 0x0184L)
#define             E_DM0185_MAX_COL_EXCEEDED		(E_DM_MASK + 0x0185L)
#define             E_DM0186_MAX_TUPLEN_EXCEEDED	(E_DM_MASK + 0x0186L)
#define             E_DM0187_BAD_OLD_LOCATION_NAME      (E_DM_MASK + 0x0187L)
#define		    E_DM0188_PINDEX_CRERR		(E_DM_MASK + 0x0188L)
#define		    E_DM0189_RAW_LOCATION_INUSE		(E_DM_MASK + 0x0189L)
#define		    E_DM0190_RAW_LOCATION_OCCUPIED	(E_DM_MASK + 0x0190L)
#define		    E_DM0191_RAW_LOCATION_MISSING	(E_DM_MASK + 0x0191L)
#define             E_DM0192_CREATEDB_RONLY_NOTEXIST	(E_DM_MASK + 0x0192L)
#define             E_DM0193_RAW_LOCATION_EXTEND	(E_DM_MASK + 0x0193L)
#define             E_DM0194_BAD_RAW_PAGE_NUMBER	(E_DM_MASK + 0x0194L)
#define             E_DM0195_AREA_IS_RAW		(E_DM_MASK + 0x0195L)
#define             E_DM0196_AREA_NOT_RAW		(E_DM_MASK + 0x0196L)
#define             E_DM0197_INVALID_RAW_USAGE		(E_DM_MASK + 0x0197L)
#define             E_DM0198_RUN_MKRAWAREA		(E_DM_MASK + 0x0198L)
#define             E_DM0199_INVALID_ROOT_LOCATION	(E_DM_MASK + 0x0199L)
#define             E_DM019A_ERROR_UNFIXING_PAGES	(E_DM_MASK + 0x019AL)
#define             E_DM019B_INVALID_ALTCOL_PARAM	(E_DM_MASK + 0x019BL)
#define             E_DM019C_ACOL_KEY_NOT_ALLOWED	(E_DM_MASK + 0x019CL)
#define             E_DM019D_ROW_CONVERSION_FAILED	(E_DM_MASK + 0x019DL)
#define		    E_DM019E_ERROR_FILE_EXIST		(E_DM_MASK + 0x019EL)
#define             E_DM01A0_INVALID_ALTCOL_DEFAULT	(E_DM_MASK + 0x01A0L)
#define             E_DM0200_MOD_TEMP_TABLE_KEY         (E_DM_MASK + 0x0200L) 
#define             E_DM0201_NOLG_MUSTLOG_ERROR         (E_DM_MASK + 0x0201L)
#define             E_DM9265_ADD_DB			(E_DM_MASK + 0x9265L)
#define             E_DM9581_REP_SHADOW_DUPLICATE	(E_DM_MASK + 0x9581L)
/*
** Externally visible security errors
*/
#define		    E_DM201E_SECURITY_DML_NOPRIV	(E_DM_MASK + 0x201EL)
#define		    E_DM201F_ROW_TABLE_LABEL		(E_DM_MASK + 0x201FL)

/*
** check/patch table warning messages range from 5000 to 51ff
*/
#define             W_DM5001_BAD_PG_NUMBER              (E_DM_MASK + 0x5001L)
#define             W_DM5002_BAD_DATA_PAGESTAT          (E_DM_MASK + 0x5002L)
#define             W_DM5003_UNEXPECTED_VERSION         (E_DM_MASK + 0x5003L)
#define             W_DM5004_STRUCTURAL_DAMAGE          (E_DM_MASK + 0x5004L)
#define             W_DM5005_INVALID_TID                (E_DM_MASK + 0x5005L)
#define             W_DM5006_BAD_ATTRIBUTE              (E_DM_MASK + 0x5006L)
#define             W_DM5007_ADF_ERROR                  (E_DM_MASK + 0x5007L)
#define             W_DM5008_TOO_MANY_BAD_ATTS          (E_DM_MASK + 0x5008L)
#define		    W_DM5009_MERGED_DATA_CHAIN		(E_DM_MASK + 0x5009L)
#define		    W_DM500A_CIRCULAR_DATA_CHAIN	(E_DM_MASK + 0x500AL)
#define		    W_DM500B_WRONG_PAGE_MAIN		(E_DM_MASK + 0x500BL)
#define		    W_DM500C_ORPHAN_DATA_PAGE		(E_DM_MASK + 0x500CL)
#define		    W_DM500D_WRONG_PAGE_OVFL		(E_DM_MASK + 0x500DL)
#define		    W_DM500E_UNKNOWN_BTREE_PAGE		(E_DM_MASK + 0x500EL)
#define		    W_DM500F_BAD_FREE_LIST_HDR		(E_DM_MASK + 0x500FL)
#define		    W_DM5010_BAD_HDR_PAGE_STAT		(E_DM_MASK + 0x5010L)
#define		    W_DM5011_ILLEGAL_DATA_PAGE		(E_DM_MASK + 0x5011L)
#define		    W_DM5012_NO_FREE_PAGES		(E_DM_MASK + 0x5012L)
#define		    W_DM5013_CIRCULAR_FREE_LIST		(E_DM_MASK + 0x5013L)
#define		    W_DM5014_FREE_PAGE_UNREADABLE	(E_DM_MASK + 0x5014L)
#define		    W_DM5015_FREE_PAGE_UNKNOWN		(E_DM_MASK + 0x5015L)
#define		    W_DM5016_NOT_A_FREE_PAGE		(E_DM_MASK + 0x5016L)
#define		    W_DM5017_NO_1ST_LEAF		(E_DM_MASK + 0x5017L)
#define		    W_DM5018_LEAF_CHAIN_BROKEN		(E_DM_MASK + 0x5018L)
#define		    W_DM5019_LEAF_CHAIN_DUPLICATE	(E_DM_MASK + 0x5019L)
#define		    W_DM501A_BAD_LINK_IN_CHAIN		(E_DM_MASK + 0x501AL)
#define		    W_DM501B_UNREF_LEAF_FROM_CHAIN	(E_DM_MASK + 0x501BL)
#define		    W_DM501C_REFERENCED_BTREE_ROOT	(E_DM_MASK + 0x501CL)
#define		    W_DM501D_REFERENCED_FREEHDR		(E_DM_MASK + 0x501DL)
#define		    W_DM501E_UNEXAMINED_PAGE		(E_DM_MASK + 0x501EL)
#define		    W_DM501F_ORPHAN_BTREE_DATA_PG	(E_DM_MASK + 0x501FL)
#define		    W_DM5020_ORPHAN_BTREE_FREE_PG	(E_DM_MASK + 0x5020L)
#define		    W_DM5021_ORPHAN_BTREE_LEAF_PG	(E_DM_MASK + 0x5021L)
#define		    W_DM5022_ORPHAN_BTREE_OVFL_PG	(E_DM_MASK + 0x5022L)
#define		    W_DM5023_ORPHAN_BTREE_INDEX		(E_DM_MASK + 0x5023L)
#define		    W_DM5024_ORPHAN_UNREADABLE_PG	(E_DM_MASK + 0x5024L)
#define		    W_DM5025_ORPHAN_ILLEGAL_PG		(E_DM_MASK + 0x5025L)
#define		    W_DM5026_IDX_CHAIN_BROKEN		(E_DM_MASK + 0x5026L)
#define		    W_DM5027_NOT_INDEX_PG		(E_DM_MASK + 0x5027L)
#define		    W_DM5028_MULT_REF_INDEX_PG		(E_DM_MASK + 0x5028L)
#define		    W_DM5029_WRONG_BT_NEXTPAGE		(E_DM_MASK + 0x5029L)
#define		    W_DM502A_WRONG_BT_DATA		(E_DM_MASK + 0x502AL)
#define		    W_DM502B_INVALID_BT_KIDS		(E_DM_MASK + 0x502BL)
#define		    W_DM502C_BAD_BID_OFFSET		(E_DM_MASK + 0x502CL)
#define		    W_DM502D_IDX_REFERENCES_SELF	(E_DM_MASK + 0x502DL)
#define		    W_DM502E_IDX_REFERENCES_ROOT	(E_DM_MASK + 0x502EL)
#define		    W_DM502F_IDX_REFERENCES_FREE	(E_DM_MASK + 0x502FL)
#define		    W_DM5030_NOT_LEAF_PG		(E_DM_MASK + 0x5030L)
#define		    W_DM5031_MULT_REFERENCED_LEAF	(E_DM_MASK + 0x5031L)
#define		    W_DM5032_MERGED_LEAF_CHAIN		(E_DM_MASK + 0x5032L)
#define		    W_DM5033_CIRCULAR_LEAF_CHAIN	(E_DM_MASK + 0x5033L)
#define		    W_DM5034_TOOMANY_NEG_INFINITY	(E_DM_MASK + 0x5034L)
#define		    W_DM5035_TOOMANY_POS_INFINITY	(E_DM_MASK + 0x5035L)
#define		    W_DM5036_CANT_READ_LEAF_TID		(E_DM_MASK + 0x5036L)
#define		    W_DM5037_NOT_LEAF_OVFL_PG		(E_DM_MASK + 0x5037L)
#define		    W_DM5038_WRONG_BTNEXTPG		(E_DM_MASK + 0x5038L)
#define		    W_DM5039_WRONG_OVFL_PAGEMAIN	(E_DM_MASK + 0x5039L)
#define		    W_DM503A_BAD_OVFL_PG_KEY		(E_DM_MASK + 0x503AL)
#define		    W_DM503B_BAD_OVFL_PARENT_KEY	(E_DM_MASK + 0x503BL)
#define		    W_DM503C_BAD_LEAF_KEY		(E_DM_MASK + 0x503CL)
#define		    W_DM503D_NO_SUCH_TID		(E_DM_MASK + 0x503DL)
#define		    W_DM503E_FIXED_LEAF			(E_DM_MASK + 0x503EL)
#define		    W_DM503F_NOT_A_LEAF			(E_DM_MASK + 0x503FL)
#define		    W_DM5040_KEY_HOLE			(E_DM_MASK + 0x5040L)
#define		    W_DM5041_MULTIREF_LEAFS		(E_DM_MASK + 0x5041L)
#define		    W_DM5042_NOT_DATA_PAGE		(E_DM_MASK + 0x5042L)
#define		    W_DM5043_LEAF_REFERENCES_SELF	(E_DM_MASK + 0x5043L)
#define		    W_DM5044_IDX_CHAIN_BROKEN		(E_DM_MASK + 0x5044L)
#define		    W_DM5045_NOT_INDEX_PG		(E_DM_MASK + 0x5045L)
#define		    W_DM5046_MULT_REF_INDEX_PG		(E_DM_MASK + 0x5046L)
#define		    W_DM5047_INVALID_NUM_KEYS		(E_DM_MASK + 0x5047L)
#define		    W_DM5048_INVALID_PAGE_OVFL		(E_DM_MASK + 0x5048L)
#define		    W_DM5049_INVALID_PAGE_OVFL		(E_DM_MASK + 0x5049L)
#define		    W_DM504A_MISSING_ISAM_KEY		(E_DM_MASK + 0x504AL)
#define		    W_DM504B_IDX_REFERENCES_SELF	(E_DM_MASK + 0x504BL)
#define		    W_DM504C_IDX_REFERENCES_ROOT	(E_DM_MASK + 0x504CL)
#define		    W_DM504D_INDEX_KEY_OFFSET		(E_DM_MASK + 0x504DL)
#define		    W_DM504E_WRONG_NUM_KEYS		(E_DM_MASK + 0x504EL)
#define		    W_DM504F_KEYS_NOT_SEQUENTIAL	(E_DM_MASK + 0x504FL)
#define		    W_DM5050_BAD_ISAM_KEY_ORDER		(E_DM_MASK + 0x5050L)
#define		    W_DM5051_MULTIREF_DATA_PAGE		(E_DM_MASK + 0x5051L)
#define		    W_DM5052_BAD_PAGE_MAIN		(E_DM_MASK + 0x5052L)
#define		    W_DM5053_BAD_ISAM_OVFL_PTR		(E_DM_MASK + 0x5053L)
#define		    W_DM5054_MULTIREF_ISAM_OVFL_PG	(E_DM_MASK + 0x5054L)
#define		    W_DM5055_BAD_OVFL_PAGEMAIN		(E_DM_MASK + 0x5055L)
#define		    W_DM5056_ORPHAN_ISAM_MAIN_PG	(E_DM_MASK + 0x5056L)
#define		    W_DM5057_ORPHAN_ISAM_INDEX_PG	(E_DM_MASK + 0x5057L)
#define		    W_DM5058_ORPHAN_ISAM_OVFL_PG	(E_DM_MASK + 0x5058L)
#define		    W_DM5059_BAD_KEY_GET		(E_DM_MASK + 0x5059L)
#define		    W_DM505A_CORRUPT_FHDR		(E_DM_MASK + 0x505AL)
#define		    W_DM505B_CORRUPT_FMAP		(E_DM_MASK + 0x505BL)
#define		    W_DM505C_FMAP_INCONSISTENCY		(E_DM_MASK + 0x505CL)
#define		    W_DM505D_TOTAL_FMAP_INCONSIS	(E_DM_MASK + 0x505DL)
#define		    W_DM505E_NO_ETAB_TABLE		(E_DM_MASK + 0x505EL)
#define		    W_DM505F_TOO_MANY_BAD_SEGS		(E_DM_MASK + 0x505FL)
#define		    W_DM5060_NO_ETAB_SEGMENT		(E_DM_MASK + 0x5060L)
#define		    W_DM5061_WRONG_BLOB_LENGTH		(E_DM_MASK + 0x5061L)
#define             W_DM5062_INDEX_KPERPAGE             (E_DM_MASK + 0x5062L)
#define             W_DM5063_LEAF_KPERPAGE              (E_DM_MASK + 0x5063L)
#define             W_DM5064_BAD_ATTRIBUTE              (E_DM_MASK + 0x5064L)

#define		    W_DM51FB_NO_CHAIN_MAP		(E_DM_MASK + 0x515BL)
#define             W_DM51FC_ERROR_PUTTING_PAGE         (E_DM_MASK + 0x515CL)
#define             W_DM51FD_TOO_MANY_BAD_PAGES         (E_DM_MASK + 0x51FDL)
#define             W_DM51FE_CANT_READ_PAGE             (E_DM_MASK + 0x51FEL)
#define             W_DM51FF_BAD_RELSPEC                (E_DM_MASK + 0x51FFL)

/*
** check/patch table informative messages range from 5200 to 53ff
*/
#define             I_DM5201_BAD_PG_NUMBER              (E_DM_MASK + 0x5201L)
#define             I_DM5202_FIXED_DATA_PAGE            (E_DM_MASK + 0x5202L)
#define             I_DM5204_STRUCTURAL_DAMAGE          (E_DM_MASK + 0x5204L)
#define             I_DM5205_INVALID_TID                (E_DM_MASK + 0x5205L)
#define             I_DM5206_BAD_ATTRIBUTE              (E_DM_MASK + 0x5006L)
#define             I_DM5208_TOO_MANY_BAD_ATTS          (E_DM_MASK + 0x5208L)
#define		    I_DM5209_FREE_PAGE			(E_DM_MASK + 0x5209L)
#define		    I_DM520A_LEAF_PAGE			(E_DM_MASK + 0x520AL)
#define		    I_DM520B_LEAF_OVERFLOW		(E_DM_MASK + 0x520BL)
#define		    I_DM520C_INDEX_PAGE			(E_DM_MASK + 0x520CL)
#define		    I_DM520D_ROOT_PAGE			(E_DM_MASK + 0x520DL)
#define		    I_DM520E_FREE_HDR_PAGE		(E_DM_MASK + 0x520EL)
#define		    I_DM520F_CHECK_LEAF			(E_DM_MASK + 0x520FL)
#define		    I_DM5210_CHECK_BINDEX		(E_DM_MASK + 0x5210L)
#define		    I_DM5211_NUM_BTREE_PAGES		(E_DM_MASK + 0x5211L)
#define		    I_DM5212_STARTING_BORPHAN		(E_DM_MASK + 0x5212L)
#define		    I_DM5213_CHECK_LEAF_PTRS		(E_DM_MASK + 0x5213L)
#define		    I_DM5214_UNABLE_TO_CK_ORPHANS	(E_DM_MASK + 0x5214L)
#define		    I_DM5215_EMPTY_LEAF			(E_DM_MASK + 0x5215L)
#define		    I_DM5216_EMPTY_LEAF_OVFL		(E_DM_MASK + 0x5216L)
#define		    I_DM5220_CHECK_IINDEX		(E_DM_MASK + 0x5220L)
#define		    I_DM5221_NUM_ISAM_PAGES		(E_DM_MASK + 0x5221L)
#define		    I_DM5222_STARTING_IORPHAN		(E_DM_MASK + 0x5222L)

#define		    I_DM53F9_FMAP_PAGE			(E_DM_MASK + 0x53F9L)
#define		    I_DM53FA_FHDR_PAGE			(E_DM_MASK + 0x53FAL)
#define		    I_DM53FB_CHECK_DONE			(E_DM_MASK + 0x53FBL)
#define             I_DM53FC_PATCH_DONE                 (E_DM_MASK + 0x53FCL)
#define             I_DM53FD_NOT_DATA_PAGE              (E_DM_MASK + 0x53FDL)
#define             I_DM53FE_DATA_PAGE                  (E_DM_MASK + 0x53FEL)


/* finddbs error and informative mesages range from 5400 to 5421 */
#define		    W_DM5400_DIOPEN_ERR			(E_DM_MASK + 0x5400L)
#define		    W_DM5401_LOC_DOESNOT_EXIST		(E_DM_MASK + 0x5401L)
#define		    W_DM5402_UNEXPECTED_LISTDIR_ERR	(E_DM_MASK + 0x5402L)
#define		    W_DM5403_ADMIN_READ_ERROR		(E_DM_MASK + 0x5403L)
#define		    W_DM5404_BAD_JNL_LOC		(E_DM_MASK + 0x5404L)
#define		    W_DM5405_BAD_CKP_LOC		(E_DM_MASK + 0x5405L)
#define		    W_DM5406_CANT_TRANSLATE_UCODE	(E_DM_MASK + 0x5406L)
#define		    W_DM5407_NO_V6_LOCATION		(E_DM_MASK + 0x5407L)
#define		    W_DM5408_MULTIPLE_V6_LOCATIONS	(E_DM_MASK + 0x5408L)
#define		    W_DM5409_CONFIG_READ_ERROR		(E_DM_MASK + 0x5409L)
#define		    W_DM540A_LOC_NOT_DEFINED		(E_DM_MASK + 0x540AL)

#define		    E_DM5410_CLOSE_ERROR		(E_DM_MASK + 0x5410L)
#define		    E_DM5411_ERROR_SEARCHING_LOC	(E_DM_MASK + 0x5411L)
#define		    E_DM5412_IIPHYSDB_ERROR		(E_DM_MASK + 0x5412L)
#define		    E_DM5413_IIPHYSEXT_ERROR		(E_DM_MASK + 0x5413L)
#define		    E_DM5414_ADMIN30_ERROR		(E_DM_MASK + 0x5414L)
#define		    E_DM5415_CONFIG_ERROR		(E_DM_MASK + 0x5415L)

#define		    I_DM5420_DB_FOUND			(E_DM_MASK + 0x5420L)
#define		    I_DM5421_50DB_FOUND			(E_DM_MASK + 0x5421L)

/* Misc. warning and informational messages */
#define		    W_DM5422_IIDBDB_NOT_JOURNALED	(E_DM_MASK + 0x5422L)

#define             E_DM9D00_IISEQUENCE_NOT_FOUND	(E_DM_MASK + 0x9D00L)
#define             E_DM9D01_SEQUENCE_EXCEEDED 		(E_DM_MASK + 0x9D01L)
#define             E_DM9D02_SEQUENCE_NOT_FOUND		(E_DM_MASK + 0x9D02L)
#define             E_DM9D0A_SEQUENCE_ERROR 		(E_DM_MASK + 0x9D0AL)

    i4         err_data;               /* Additional information such as
                                            ** index pointer to array item 
                                            ** in error. */
}   DM_ERR;

/*}
** Name:  DM_IIxxxx - Defines for the table ids of the System tables.
**
** Description:
**      This section defines the table ids for the system tables.
**	NOTE: THESE CANNOT BE CHANGED RANDOMLY, AS CHANGES TO THESE
**	      NUMBERS WILL MAKE EXISTING CUSTOMER DATABASES UNREADABLE!
**	NOTE: DM_I_**_TAB_ID of an index MAY NOT be equal to DM_B_**_TAB_ID of
**	      any catalog AND may NOT be equal to DM_I_**_TAB_ID of any other
**	      index; for instance, if you have a catalog with id (34, 0) and an
**	      index on it with id (34,35) you may not create an index with id
**	      (*, 34) or (*, 35)
**
** History:
**      09-jul-86 (jennifer)
**          Created.
**	10-dec-86 (fred)
**	    Added dbdb catalog id's
**	22-Jul-88 (teg)
**	    Added catalog id for iiprocedure, iixdbdepends
**      14-mar-89 (ralph)
**          GRANT Enhancements, Phase 1:
**          Added DMF_[BI]_{GRPID,APLID}_TAB_ID
**          for iiusergroup and iiapplication_id catalogs
**      06-mar-89 (neil)
**          Added reserved table id for iirule.
**      31-mar-89 (ralph)
**          GRANT Enhancements, Phase 2:
**          Added DMF_[BI]_DBPRIV_TAB_ID for the iidbpriv catalog
**      25-oct-89 (neil)
**          Added reserved table id for iievent.
**	16-jan-90 (ralph)
**	    Added DMF_[BI]_DBLOC_TAB_ID for the iidbloc catalog.
**	24-jan-1990 (fred)
**	    Added DM_{B,I}_ETAB_TAB_ID for the iiextensions catalog.
**	28-dec-90 (teresa)
**	    Added table ids for iiphys_database, iiphys_extend and iitemp_locs
**	29-may-92 (andre)
**	    reserved table id and define key attribute ids for IIPRIV
**	10-jun-92 (andre)
**	    reserved table id and define key attribute ids for IIXPRIV
**	18-jun-92 (andre)
**	    reserved id and defined key attribute ids for IIXPROCEDURE and
**	    IIXEVENT
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  new index on iiintegrity, new index on iirule,
**	    new catalog iikey, new catalog iidefault with a secondary index.
**	05-oct-92 (robf)
**	    C2-SXA gateway catalogs
**      02-nov-92 (anitap)
**          CREATE SCHEMA: new catalog iischema and index on iischema.
**	24-jan-93 (anitap)
**	    Changes for CREATE SCHEMA. Add another key to iischemaidx.
**	19-feb-93 (rickh)
**	    Removed second key of IIDEFAULTIDX and force first key to be the
**	    third attribute to agree with CREATEDB.
**	16-mar-93 (andre)
**	    definition of IIREL_IDX tuple has been moved out of DMP.H into
**	    DBMS.H.  #define's for IIREL_IDX keys will now live in dmf.h.
**	24-may-1993 (robf)
**	    Removed xORANGE definitions for Secure 2.0
**	15-jun-93 (rickh)
**	    Added symbols for iixprotext catalog.
**	15-oct-93 (andre)
**	    Added symbols for IIXSYNONYM
**	26-nov-93 (robf)
**          Added IISECALARM definitions for new iisecalarm catalog
**	3-feb-94 (robf)
**          Correct spelling/cut-paste names for keyvalues.
**	4-mar-94 (robf)
**          Add IIROLEGRANT for iirolegrant catalog
**	27-may-1999 (nanpr01)
**	    Added new secondary index on owner name and rule name on
**	    iirule catalog for doing lookup for creating and destroying rules.
**	29-dec-1999 (somsa01)
**	    Changed DM_I_RULEIDX1_TAB_ID from 139 to 72. It was set to above
**	    DM_SCATALOG_MAX, and 72 is the next static table id not in use.
**	05-Mar-2002 (jenjo02)
**	    Added IISEQUENCE catalog, DM_B_SEQ_TAB_ID 73,
**	    pushed DM_SCATALOG_MAX to 74.
**	19-Dec-2003 (schka24)
**	    Added partitioning catalog ids.
**	29-Jan-2004 (schka24)
**	    Take iipartition back out.
**	3-jan-06 (dougi)
**	    Added IICOLCOMPARE catalog.
**	26-May-2006 (jenjo02)
**	    Upped DSC_VCURRENT to DSC_V9 for Ingres2007
*/

/* S_CONCUR SYSTEM TABLES which are required to create a DB. */

#define             DM_B_RELATION_TAB_ID         1L
#define             DM_I_RELATION_TAB_ID         0L

#define             DM_B_RIDX_TAB_ID             1L
#define             DM_I_RIDX_TAB_ID             2L
#define		    DM_1_RELIDX_KEY		 1
#define             DM_2_RELIDX_KEY		 2

#define             DM_B_ATTRIBUTE_TAB_ID        3L
#define             DM_I_ATTRIBUTE_TAB_ID        0L
#define             DM_B_INDEX_TAB_ID            4L
#define             DM_I_INDEX_TAB_ID            0L
#define		    DM_1_INDEX_KEY 		 1
#define             DM_2_INDEX_KEY		 2

/* SCONCUR tables which do not have to be part of templates. */

#define             DM_B_DEVICE_TAB_ID            8L
#define             DM_I_DEVICE_TAB_ID            0L

/* SCONCUR max base table id. */

#define             DM_SCONCUR_MAX               9L

/* SYSTEM TABLES  Identifiers and key attribute numbers. */

/*
** Maximum ID associated with an existing catalog
** this is defined here for your convenience only - as long as everyone 
** cooperates, there will be no need to scan through the existing list
** Note: before you increase syscat-max, are there any unused numbers?
** 
*/
#define		    DM_SYSCAT_MAX		 77L

#define             DM_B_QRYTEXT_TAB_ID          20L
#define             DM_I_QRYTEXT_TAB_ID          0L
#define             DM_1_QRYTEXT_KEY             1L
#define             DM_2_QRYTEXT_KEY             2L
#define             DM_3_QRYTEXT_KEY             3L
#define             DM_4_QRYTEXT_KEY             4L
#define             DM_B_TREE_TAB_ID             21L
#define             DM_I_TREE_TAB_ID             0L
#define             DM_1_TREE_KEY                1L
#define             DM_2_TREE_KEY                2L
#define             DM_3_TREE_KEY                6L
#define             DM_4_TREE_KEY                5L
#define             DM_B_PROTECT_TAB_ID          22L
#define             DM_I_PROTECT_TAB_ID          0L
#define             DM_1_PROTECT_KEY             1L
#define             DM_2_PROTECT_KEY             2L

/* iixprotect index was (22,39), tabid 39 now iicolcompare */

#define             DM_B_INTEGRITY_TAB_ID        23L
#define             DM_I_INTEGRITY_TAB_ID        0L
#define             DM_1_INTEGRITY_KEY           1L
#define             DM_2_INTEGRITY_KEY           2L

#define             DM_B_INTEGRITYIDX_TAB_ID     23L
#define             DM_I_INTEGRITYIDX_TAB_ID     53L
#define             DM_1_INTEGRITYIDX_KEY        1L
#define             DM_2_INTEGRITYIDX_KEY        2L
#define             DM_3_INTEGRITYIDX_KEY        3L

#define             DM_B_DEPENDS_TAB_ID          24L
#define             DM_I_DEPENDS_TAB_ID          0L
#define             DM_1_DEPENDS_KEY             1L
#define             DM_2_DEPENDS_KEY             2L
#define             DM_3_DEPENDS_KEY             3L
#define             DM_4_DEPENDS_KEY             4L
#define             DM_B_STATISTICS_TAB_ID       25L
#define             DM_I_STATISTICS_TAB_ID       0L

#define             DM_1_STATISTICS_KEY          1L
#define             DM_2_STATISTICS_KEY          2L

#define             DM_B_HISTOGRAM_TAB_ID        26L
#define             DM_I_HISTOGRAM_TAB_ID        0L
#define             DM_1_HISTOGRAM_KEY           1L
#define             DM_2_HISTOGRAM_KEY           2L

#define             DM_B_COLCOMPARE_TAB_ID       39L
#define             DM_I_COLCOMPARE_TAB_ID       0L
#define             DM_1_COLCOMPARE_KEY          1L
#define             DM_2_COLCOMPARE_KEY          3L

#define             DM_B_DBP_TAB_ID		 27L
#define             DM_I_DBP_TAB_ID		 0L
#define             DM_1_DBP_KEY 		 1L
#define             DM_2_DBP_KEY 		 2L

#define             DM_B_XDEPENDS_TAB_ID         24L
#define             DM_I_XDEPENDS_TAB_ID         28L
#define             DM_1_XDEPENDS_KEY            1L
#define             DM_2_XDEPENDS_KEY            2L
#define             DM_3_XDEPENDS_KEY            3L
#define             DM_4_XDEPENDS_KEY            4L

#define             DM_B_RULE_TAB_ID             29L    /* iirule */
#define             DM_I_RULE_TAB_ID             0L
#define             DM_1_RULE_KEY                5L
#define             DM_2_RULE_KEY                6L

#define             DM_B_RULEIDX_TAB_ID          29L    /* iiruleidx */
#define             DM_I_RULEIDX_TAB_ID          54L
#define             DM_1_RULEIDX_KEY             1L
#define             DM_2_RULEIDX_KEY             2L

#define             DM_B_RULEIDX1_TAB_ID         29L    /* iiruleidx1 */
#define             DM_I_RULEIDX1_TAB_ID         72L
#define             DM_1_RULEIDX1_KEY             1L
#define             DM_2_RULEIDX1_KEY             2L

#define             DM_B_DBLOC_TAB_ID           30L    /* iidbloc */
#define             DM_I_DBLOC_TAB_ID            0L
#define             DM_1_DBLOC_KEY               1L
#define             DM_B_EVENT_TAB_ID		 31L    /* iievent */
#define             DM_I_EVENT_TAB_ID		 0L
#define             DM_1_EVENT_KEY               1L
#define             DM_2_EVENT_KEY               2L

#define		    DM_B_COMMENT_TAB_ID		 32L	/* iidbms_comment */
#define		    DM_I_COMMENT_TAB_ID		 0L
#define             DM_1_COMMENT_KEY		1L      /* keyed on table id */
#define             DM_2_COMMENT_KEY            2L      /* ditto */
#define             DM_3_COMMENT_KEY            3L      /* and attribute id */

#define		    DM_B_SYNONYM_TAB_ID		33L	/* iisynonym	  */
#define		    DM_I_SYNONYM_TAB_ID		0L
#define             DM_1_SYNONYM_KEY		1L
#define             DM_2_SYNONYM_KEY            2L

#define		    DM_B_XSYN_TAB_ID		33L	/* iixsynonym */
#define             DM_I_XSYN_TAB_ID		63L
#define             DM_1_XSYN_KEY		1L
#define             DM_2_XSYN_KEY		2L

#define		    DM_B_PRIV_TAB_ID		34L	/* iipriv */
#define		    DM_I_PRIV_TAB_ID		0L
#define             DM_1_PRIV_KEY		1L
#define             DM_2_PRIV_KEY		2L
#define             DM_3_PRIV_KEY		3L

#define		    DM_B_XPRIV_TAB_ID		34L     /* iixpriv */
#define             DM_I_XPRIV_TAB_ID           35L
#define             DM_1_XPRIV_KEY               1L
#define             DM_2_XPRIV_KEY               2L
#define             DM_3_XPRIV_KEY               3L

#define		    DM_B_XDBP_TAB_ID		27L	/* iixprocedure */
#define             DM_I_XDBP_TAB_ID		36L
#define             DM_1_XDBP_KEY		 1L
#define             DM_2_XDBP_KEY                2L

#define		    DM_B_XEVENT_TAB_ID		31L	/* iixevent */
#define             DM_I_XEVENT_TAB_ID          37L
#define             DM_1_XEVENT_KEY              1L
#define             DM_2_XEVENT_KEY		 2L


#define		    DM_B_ETAB_TAB_ID		38L /* iiextended_relation */
#define		    DM_I_ETAB_TAB_ID		0L
#define		    DM_1_ETAB_KEY		1

#define		    DM_B_RANGE_TAB_ID		71L	/* iirange */
#define		    DM_I_RANGE_TAB_ID		0L

#define		    DM_B_SEQ_TAB_ID		73L	/* iisequence */
#define		    DM_I_SEQ_TAB_ID		0L
#define		    DM_1_SEQ_KEY		1L	   /* name */
#define		    DM_2_SEQ_KEY		2L	   /* owner */

/* Partitioned table catalogs */

#define		DM_B_DSCHEME_TAB_ID		74L	/* iidistscheme */
#define		DM_I_DSCHEME_TAB_ID		0L
#define		DM_1_DSCHEME_KEY		1L	/* mastid */
#define		DM_2_DSCHEME_KEY		2L	/* mastidx */
#define		DM_3_DSCHEME_KEY		3L	/* levelno */

#define		DM_B_DISTCOL_TAB_ID		75L	/* iidistcol */
#define		DM_I_DISTCOL_TAB_ID		0L
#define		DM_1_DISTCOL_KEY		1L	/* mastid */
#define		DM_2_DISTCOL_KEY		2L	/* mastidx */
#define		DM_3_DISTCOL_KEY		3L	/* levelno */
#define		DM_4_DISTCOL_KEY		4L	/* colseq */

#define		DM_B_DISTVAL_TAB_ID		76L	/* iidistval */
#define		DM_I_DISTVAL_TAB_ID		0L
#define		DM_1_DISTVAL_KEY		1L	/* mastid */
#define		DM_2_DISTVAL_KEY		2L	/* mastidx */
#define		DM_3_DISTVAL_KEY		3L	/* levelno */
#define		DM_4_DISTVAL_KEY		4L	/* valseq */
#define		DM_5_DISTVAL_KEY		5L	/* colseq */

#define		DM_B_PNAME_TAB_ID		77L	/* iipartname */
#define		DM_I_PNAME_TAB_ID		0L
#define		DM_1_PNAME_KEY			1L	/* mastid */
#define		DM_2_PNAME_KEY			2L	/* mastidx */
#define		DM_3_PNAME_KEY			3L	/* levelno */
#define		DM_4_PNAME_KEY			4L	/* partseq */

/*  Database Database system tables */

#define		    DM_B_LOCATIONS_TAB_ID	40L
#define		    DM_I_LOCATIONS_TAB_ID	0L
#define             DM_1_LOCATIONS_KEY          2L
#define		    DM_B_DATABASE_TAB_ID	41L
#define		    DM_I_DATABASE_TAB_ID	0L

#define             DM_1_DATABASE_KEY           1L

#define		    DM_B_DBACCESS_TAB_ID	42L
#define		    DM_I_DBACCESS_TAB_ID	0L

#define             DM_1_DBACCESS_KEY           1L
#define             DM_2_DBACCESS_KEY           2L

#define		    DM_B_EXTEND_TAB_ID		43L
#define		    DM_I_EXTEND_TAB_ID		0L
#define             DM_1_EXTEND_KEY             2L
#define		    DM_B_USER_TAB_ID		44L
#define		    DM_I_USER_TAB_ID		0L
#define             DM_1_USER_KEY               1L
#define		    DM_B_APLID_TAB_ID		45L
#define		    DM_I_APLID_TAB_ID		0L
#define             DM_1_APLID_KEY              1L
#define		    DM_B_GRPID_TAB_ID		46L
#define		    DM_I_GRPID_TAB_ID		0L
#define             DM_1_GRPID_KEY              1L
#define             DM_2_GRPID_KEY              2L
#define             DM_B_DBPRIV_TAB_ID          47L
#define             DM_I_DBPRIV_TAB_ID          0L
#define             DM_1_DBPRIV_KEY             2L
#define             DM_2_DBPRIV_KEY             3L
#define             DM_3_DBPRIV_KEY             1L
#define             DM_B_SECSTATE_TAB_ID        48L
#define             DM_I_SECSTATE_TAB_ID        0L

    /* iitemp_locations is keyed on area, not on lname */
#define		    DM_B_TEMP_LOC_TAB_ID	49L
#define		    DM_I_TEMP_LOC_TAB_ID	0L
#define             DM_1_TEMP_LOC_KEY           3L
    /*  iiphys_locations is NOT keyed during DMF access */
#define		    DM_B_PHYS_DB_TAB_ID		50L
#define		    DM_I_PHYS_DB_TAB_ID		0L

    /* iiphys_extend is NOT keyed during DMF access */
#define		    DM_B_PHYS_EXTEND_TAB_ID	51L
#define		    DM_I_PHYS_EXTEND_TAB_ID	0L
    /* iitemp_codemap is keyed on ucode */
#define		    DM_B_CODEMAP_TAB_ID		52L
#define		    DM_I_CODEMAP_TAB_ID		0L
#define             DM_1_CODEMAP_KEY		2L

#define		    DM_B_IIKEY_TAB_ID		55L	/* iikey */
#define		    DM_I_IIKEY_TAB_ID		0L
#define		    DM_1_IIKEY_KEY		1L	/* hash on constraint id */
#define		    DM_2_IIKEY_KEY		2L

#define		    DM_B_IIDEFAULT_TAB_ID	56L	/* iidefault */
#define		    DM_I_IIDEFAULT_TAB_ID	0L
#define		    DM_1_IIDEFAULT_KEY		1L	/* hash on default id */
#define		    DM_2_IIDEFAULT_KEY		2L

#define		    DM_B_IIDEFAULTIDX_TAB_ID	56L
#define		    DM_I_IIDEFAULTIDX_TAB_ID	57L
#define		    DM_1_IIDEFAULTIDX_KEY	1L /* hash on string */

/*
**	Used by C2-SXA gateway
*/
#define		    DM_B_GW06_REL_TAB_ID	58L	/* iigw06_relation */
#define		    DM_I_GW06_REL_TAB_ID	0L	/* iigw06_relation */

#define		    DM_B_GW06_ATTR_TAB_ID	59L	/* iigw06_attribute*/
#define		    DM_I_GW06_ATTR_TAB_ID	0L	/* iigw06_attribute*/

#define             DM_B_IISCHEMA_TAB_ID        60L   /* iischema */
#define             DM_I_IISCHEMA_TAB_ID        0L
#define             DM_1_IISCHEMA_KEY           1L    /* hash on schema name */ 

#define             DM_B_IISCHEMAIDX_TAB_ID     60L
#define             DM_I_IISCHEMAIDX_TAB_ID     61L
#define             DM_1_IISCHEMAIDX_KEY        1L      /* iischemaidx */
#define		    DM_2_IISCHEMAIDX_KEY	2L	

/*
**	More catalogs
*/

#define		    DM_B_IIPROC_PARAM_TAB_ID	62L	/*iiprocedure_parameter*/
#define		    DM_I_IIPROC_PARAM_TAB_ID	0L
#define		    DM_1_IIPROC_PARAM_KEY		1L /* hash on proc id */
#define		    DM_2_IIPROC_PARAM_KEY		2L

/*
** Security index catalogs
*/
#define		    DM_B_IISECID_TAB_ID		65L	/* iilabelmap */
#define		    DM_I_IISECID_TAB_ID		0L	
#define		    DM_1_IISECID_PARAM_KEY	1L	/* key on id */

#define		    DM_B_IISECIDX_TAB_ID	65L	/* iilabelmapidx */
#define		    DM_I_IISECIDX_TAB_ID	66L	
#define		    DM_1_IISECIDX_PARAM_KEY	1L	/* key on label */

#define		    DM_B_IILABAUDIT_TAB_ID	67L	/* iilabelaudit */
#define		    DM_I_IILABAUDIT_TAB_ID	0L	
#define		    DM_1_IILABAUDIT_KEY	1L	/* key on label */

#define		    DM_B_IIPROFILE_TAB_ID	68L	/* iiprofile */
#define		    DM_I_IIPROFILE_TAB_ID	0L	
#define		    DM_1_IIPROFILE_KEY	1L	/* key on name */

#define		    DM_B_IISECALARM_TAB_ID	69L	/* iisecalarm */
#define		    DM_I_IISECALARM_TAB_ID	0L	/* iisecalarm */
#define             DM_1_IISECALARM_KEY         6L	/* Key on type/id */
#define             DM_2_IISECALARM_KEY         3L
#define             DM_3_IISECALARM_KEY         4L

#define		    DM_B_IIROLEGRANT_TAB_ID	70L	/* iirolegrant */
#define		    DM_I_IIROLEGRANT_TAB_ID	 0L
#define		    DM_1_IIROLEGRANT_KEY	 4L	/* grantee name */
#define		    DM_2_IIROLEGRANT_KEY	 1L	/* role name */



/* SCATALOG max base table id. */

#define             DM_SCATALOG_MAX             100L

/*}
** Name:  DM_PTR - Data type for passing DMF indirect variable length data.
**
** Description:
**      This typedef defines the structure for passing variable length array
**	of pointers to fixed length objects to DMF.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
*/
typedef struct _DM_PTR
{
    PTR		    ptr_address;            /* Pointer to array of pointers. */
    i4         ptr_in_count;           /* Input number of array pointers. */
    i4         ptr_out_count;          /* Output number of pointers used. */
    i4	    ptr_size;		    /* The size of each object. */
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
    i4	    ptr_size_filler;	    /* Pad for raat, which writes
					       a pointer to ptr_size */
#endif
}   DM_PTR;
/*
** Set Trace defintions for user tracing.
*/
#define		    DM_IOTRACE		    10L
#define             DM_LOCKTRACE             1L
#define		    DM_LOGTRACE		    11L
/*
** Set Location Maximum value.
*/
#define		    DM_LOC_MAX		   256L	    /* max data locations */
#define		    DM_WRKLOC_MAX	   256L	    /* max work locations */
#define             DM_FNAME_LENGTH         12

/*}
** Name:  DM_Fxxxx - Defines default fill factors for all storage types.
**
** Description:
**      This section defines the default fill factors for assorted
**	storage types.
**
** History:
**      8-8-88     (teg)
**          Created.
*/
#define		    DM_F_HEAP		    100L    /* heap */
#define		    DM_F_CHEAP		    100L    /* compressed heap */
#define		    DM_FI_ISAM		    100L    /* isam index */
#define		    DM_F_ISAM		    80L	    /* isam data */
#define		    DM_F_CISAM		    100L    /* compressed isam data */
#define		    DM_F_HASH		    50L	    /* hash */
#define		    DM_F_CHASH		    75L	    /* compressed hash */
#define		    DM_F_BTREE		    80L	    /* btree data */
#define		    DM_F_CBTREE		    100L    /* compressed btree data */
#define		    DM_FI_BTREE		    80L	    /* btree index */
#define		    DM_FL_BTREE		    70L	    /* btree leaf */

/*{
** Name:  DM_COLUMN - DMF internal structure for attribute record.
**
** Description:
**      This typedef defines the attribute record used to describe 
**      attributes of a table.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	19-Oct-88 (teg)
**	    Reserved Attxtra for GATEWAYS users.
**	21-mar-89 (sandyh)
**	    Added B1 security labeling.
**	06-mar-89 (neil)
**          Rules: Added ATT_RULE.
**	10-apr-89 (mikem)
**	    Logical key development.  Added ATT_WORM and ATT_SYS_MAINTAINED.
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  added default id.  Added ATT_KNOWN_NOT_NULLABLE.
**	26-oct-92 (rickh)
**	    Moved this structure from DMP.H and renamed it _DM_COLUMN.
**	24-may-93 (robf)
**	    Added ATT_SECID
**      22-jul-1996 (ramra01 for bryanp)
**          Add new columns to iiattribute in support of Alter Table.
**	03-mar-2004 (gupsh01)
**	    Added new columns to iiattribute, to support alter table 
**	    alter column.
**	9-dec-04 (inkdo01)
**	    Added attcollID column to support coluimn level collations.
**	6-june-2008 (dougi)
**	    Added flags for identity columns.
**	29-Sept-2009 (troal01)
**	    Added attgeomtype and attsrid.
*/
struct _DM_COLUMN
{
#define			DM_1_ATTRIBUTE_KEY  1
#define			DM_2_ATTRIBUTE_KEY  2
     DB_TAB_ID       attrelid;              /* Identifier associated with 
                                            ** this table. */
     i2              attid;                 /* Attribute ordinal number. */
     i2		     attxtra;		    /* underlaying GATEWAY data type.
					    ** The INGRES data type may not
					    ** match the gateway data type.  The
					    ** values that are used in this
					    ** field will vary from gateway to
					    ** gateway.
					    */
     DB_ATT_NAME     attname;               /* Attribute name. */
     i4              attoff;                /* Offset in bytes of attribute
                                            ** in record. */
     i4              attfml;                /* Length in bytes. */
     i2              attkey;                /* Flag used to indicate this is
                                            ** part of a key. */
#define                  ATT_NOTKEY         0L
#define                  ATT_KEY            1L
     i2              attflag;                  /* Used to indicate types. */
#define                  ATT_NDEFAULT       0x001 /* NOT NULLABLE, NO DEFAULT */
#define			 ATT_KNOWN_NOT_NULLABLE		0x002
					/* SQL2:  this column cannot
					** contain NULLs.  replaces ATT_NULL */
#define                  ATT_DEFAULT        0x004 /* not used */
#define                  ATT_DESCENDING     0x008 /* keyed in descending order*/
#define                  ATT_RULE           0x010 /* rules applied to column */
#define			 ATT_WORM	    0x020 /* attribute can only be 
						  ** written once (on append)
						  */
#define			 ATT_SYS_MAINTAINED 0x040 /* attribute can only be 
						  ** changed by the dbms core 
						  ** system.
						  */
#define			 ATT_HIDDEN	    0x080 /* Attribute is hidden in
						  ** table.* or table.all
						  ** type references.
						  */
#define			 ATT_PERIPHERAL	   0x0100 /*
						  ** Attribute's actual value is
						  ** stored in a peripheral
						  ** table.
						  */
#define			 ATT_IDENTITY_ALWAYS 0x200 /*
						  ** ALWAYS identity column.
						  */
#define			 ATT_IDENTITY_BY_DEFAULT 0x400 /*
						  ** BY DEFAULT identity column.
						  */
#define			 ATT_SEC_KEY	   0x2000 /* This attribute is the 
						  ** security logical key for
						  ** the record.
						  */
#define			 ATT_SEC_LBL	   0x4000 /* This attribute is the 
						  ** security label for the
						  ** record.
						  */
     DB_DEF_ID	     attDefaultID;	/* this is the index in iidefaults
					** of the tuple containing the
					** user specified default for this
					** column.
					*/
     u_i2	     attintl_id;            /* internal row identifier */
     u_i2	     attver_added;          /* version row added       */
     u_i2	     attver_dropped;        /* version row dropped     */
     u_i2	     attval_from;           /* previous intl_id value  */
     i2              attfmt;                /* Attribute type. */
#define                  ATT_INT            DB_INT_TYPE
#define                  ATT_FLT            DB_FLT_TYPE
#define                  ATT_CHR            DB_CHR_TYPE
#define                  ATT_TXT            DB_TXT_TYPE
#define                  ATT_DTE            DB_DTE_TYPE
#define                  ATT_MNY            DB_MNY_TYPE
#define			 ATT_CHA	    DB_CHA_TYPE
#define			 ATT_VCH            DB_VCH_TYPE
#define			 ATT_SECID          DB_SECID_TYPE
     i2              attfmp;		    /* Precision for unsigned integer.*/
     u_i2	     attver_altcol;         /* version row altered */
     u_i2	     attcollID;		    /* explicitly declared Collation */
     i4			 attsrid;			/* geometry spatial reference system id */
     i2			 attgeomtype;		/* geometry data type */
     char	     attfree[10];	    /*   F R E E   S P A C E   */
};

/*}
** Name:  DMF_ATTR_ENTRY - Description of each attribute of a database table.
**
** Description:
**      This structure is used to describe the attributes records to be
**      inserted in the system tables for a database table create operation.
**      The DMU_CB.dmu_attr_array field contains a pointer to an array
**      containing entries of this type.
**
** History:
**	07-Dec-09 (troal01)
**		Moved here from back!hdr!hdr!dmucb.h.
*/

typedef struct _DMF_ATTR_ENTRY
{
    DB_ATT_NAME     attr_name;              /* Name of table attribute. */
    i4         attr_type;              /* Type of attribute.
                                            ** Must one of the following:
                                            ** DB_INT_TYPE, DB_FLT_TYPE,
                                            ** DB_CHR_TYPE, DB_TXT_TYPE,
                                            ** DB_DTE_TYPE, or
                                            ** DB_MNY_TYPE.
				            **
					    ** For DMU_C_DROP_ALTER this field
					    ** is used to pass the att_intlid
					    ** that identifies the internal
					    ** column identifier that does not
					    ** change during add/drop
					    */

    i4         attr_size;              /* Size of attribute. */
					    /* For DMU_C_DROP_ALTER this field
					    ** is used to pass the att_no
					    ** that identifies the ordinal
					    ** column identifier that changes
					    ** as result of add/drop column.
					    */
    i4         attr_precision;         /* Precision of attribute.
                                            ** For decimal type only. */
    i4         attr_flags_mask;        /* Used to indicate a non-
                                            ** nullable, non-defaultable
                                            ** attribute. */
#define                 DMU_F_NDEFAULT      0x01 /* NOT NULLABLE, NO DEFAULT */
#define                 DMU_F_KNOWN_NOT_NULLABLE  0x02 /* ANSI semantics */
#define                 DMU_F_DEFAULT       0x04 /* not used */
#define                 DMU_F_DESCENDING    0x08 /* keyed in descending order */
#define                 DMU_F_RULE          0x10
					    /* This flag is not really
					    ** required for DMU operations,
					    ** but since it is already reserved
					    ** for DMT alter operations this
					    ** spot is also reserved for DMU.
					    ** Same as ATT_RULE for attflag.
					    */
#define                 DMU_F_WORM          0x20
					    /* attribute can only be
					    ** written once (on append)
					    */
#define                 DMU_F_SYS_MAINTAINED 0x40
					    /* attribute can only be
					    ** changed by the dbms core
					    ** system.
					    */
#define			DMU_F_HIDDEN	    0x80
					    /*
					    ** Datatype doesn't appear in
					    ** SELECT * or HELP
					    */
#define			DMU_F_PERIPHERAL    0x100
					    /*
					    ** Datatype of attribute is
					    ** peripherally stored.
					    */
#define			DMU_F_IDENTITY_ALWAYS 0x200
					    /*
					    ** ALWAYS identity column
					    */
#define			DMU_F_IDENTITY_BY_DEFAULT 0x400
					    /*
					    ** BY DEFAULT identity column
					    */
#define			DMU_F_SEC_KEY	   0x2000 /* This attribute is the
						  ** security key for
						  ** the record.
						  */
#define 		DMU_F_LEGAL_ATTR_BITS  (\
				DMU_F_NDEFAULT |\
				DMU_F_KNOWN_NOT_NULLABLE | \
				DMU_F_DEFAULT |\
				DMU_F_DESCENDING |\
				DMU_F_RULE |\
				DMU_F_WORM|\
				DMU_F_SYS_MAINTAINED|\
				DMU_F_HIDDEN|\
				DMU_F_PERIPHERAL|\
				DMU_F_IDENTITY_ALWAYS|\
				DMU_F_IDENTITY_BY_DEFAULT|\
				DMU_F_SEC_KEY )
					    /* Turn on bit for every legal value
					    ** that can be in this field.  Must
					    ** update this if you add attribute
					    ** stats.
					    */
    DB_DEF_ID	    attr_defaultID;  	    /* default id */

    DB_IIDEFAULT    *attr_defaultTuple;
			/* if this is not NULL, then
			** PSF has filled all the fields in the default
			** descriptor except for dbd_def_id.  QEF will write
			** the default tuple, then fill in dbd_def_id, then
			** call DMF.  DMF will shove the dbd_def_id into
			** the iiattributes tuple for this column.
			*/
    i4		attr_collID;	/* collation ID (for character columns) */
    DB_IISEQUENCE   *attr_seqTuple;
			/* for identity columns, this is the IISEQUENCE
			** tuple that defines the sequence
			*/
    i4		attr_srid; /* Spatial reference ID */
    i2		attr_geomtype; /* Geometry type code */
}   DMF_ATTR_ENTRY;


/* Core catalog version numbers in the database config file.  The
** config file itself is defined privately to DMF, but upgradedb needs
** to help out with these versions on occasion.  So, define the actual
** version constants here.
*/

#define			DSC_PREHISTORIC	0L
#define			DSC_V1		1L
#define			DSC_V2		2L	/* 6.4 and ?earlier? */
#define			DSC_V3		3L	/* OpenIngres 1.x format */
#define			DSC_V4		4L	/* O/II 2.0, v2 pages */
#define			DSC_V5		5L	/* II 2.5, large files */
#define			DSC_V6		6L	/* II 2.6, raw locs */
#define			DSC_V7		7L	/* r3, partitions, v5 pages */
#define			DSC_V8		8L	/* r3.0.2, attcollid added */
#define			DSC_V9		9L	/* Ingres2007:*/
						/* iiindex.idom[32]->[64] */
#ifdef NOT_UNTIL_CLUSTERED_INDEXES
#define			DSC_VCURRENT	DSC_V9
#else
#define DSC_VCURRENT DSC_V9  /* until clustered is released */
#endif



/*
** The public interface to DMF is dmf_call. It would be nice to prototype
** this function, but it's not very useful because the second argument to
** dmf_call is sometimes a (DMC_CB *), sometimes a (DMT_CB *), sometimes a
** (DMR_CB *), sometimes a (DMU_CB *), etc. So the only thing you can do
** with the second argument is make it a "PTR", which ends up meaning that
** every caller of DMF must cast the second argument to (PTR), which means
** that there's not much value in prototyping it and we force a lot of
** code to be touched. So, instead, we just declare it old-style.
*/
FUNC_EXTERN DB_STATUS	dmf_call(/*
			    DM_OPERATION    operation,
			    void 	    *control_block
				*/);

/*
** Additional defines
*/

#define			IIDBDB_ID	1
