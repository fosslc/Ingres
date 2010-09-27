/*
** Copyright (c) 1995, 2008 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <sl.h>
# include   <iicommon.h>
# include   <dbdbms.h>
# include   <ddb.h>
# include   <qefkon.h>
# include   <ulf.h>
# include   <ulm.h>
# include   <adf.h>
# include   <cs.h>
# include   <scf.h>
# include   <cut.h>
# include   <dmf.h>
# include   <dmtcb.h>
# include   <dmrcb.h>
# include   <dmmcb.h>
# include   <dmucb.h>
# include   <qefmain.h>
# include   <qefscb.h>
# include   <qsf.h>
# include   <qefrcb.h>
# include   <qefcb.h>
# include   <qefnode.h>
# include   <psfparse.h>
# include   <qefact.h>
# include   <qefqeu.h>
# include   <qeuqcb.h>
# include   <qefqp.h>
# include   <dudbms.h>
# include   <qefdsh.h>
# include   <rqf.h>
# include   <tpf.h>
# include   <qefqry.h>
# include   <qefcat.h>
# include   <ex.h>
# include   <tm.h>
# include   <ade.h>
# include   <sxf.h>
# include   <qefprotos.h>
# include   <usererror.h>

/*
** Name:	qefdata.c
**
** Description:	Global data for qef facility.
**
** History:
**      25-Oct-95 (fanra01)
**          Created.
**      20-feb-96 (canor01/tchdi01)
**          Added error messages
**             E_QE7612_PROD_MODE_ERR
**             E_QE7610_SPROD_UPD_IIDB
**          for the production mode project
**	23-sep-1996 (canor01)
**	    Updated.
**	14-nov-1996 (nanpr01)
**	    Disallow modify of temporary tables to diff page sizes.
**	    map the error message E_DM0183_CANT_MOD_TEMP_TABLE.
**	02-dec-1996 (nanpr01)
**	    Map E_DM0186_MAX_TUPLEN_EXCEEDED and E_DM0185_MAX_COL_EXCEEDED.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Map E_DM006A_TRAN_ACCESS_CONFLICT.
**	15-apr-1997 (nanpr01)
**	    Errors are not getting mapped consistently to user message.
**	    bug 80808.
**	3-Apr-98 (thaju02)
**	    Added E_QE011D_PERIPH_CONVERSION to the qef error map.
**	3-feb-99 (inkdo01)
**	    Added row producing proc errors (30C, 30D, 30E).
**	21-mar-99 (nanpr01)
**	    Support Raw locations.
**     19-jan-2000 (stial01)
**          Added E_DM00A1_ERROR_CONVERTING_TBL to the qef error map.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-apr-01 (inkdo01)
**	    Map QE030D to US098F (nested or dynamic call to row producing
**	    procedure).
**	04-Oct-2001 (jenjo02)
**	    Don't re-log E_RQ and E_TP messages which have already
**	    been logged.
**	15-Oct-2001 (jenjo02)
**	    Added E_DM0195/6/7/8 for Raw location support.
**	    Don't transform E_DM011A_ERROR_DB_DIR to 
**	    misleading E_QE0079_ERROR_DELETING_DB; the error can
**	    occur for a variety of reasons...
**	04-Dec-2001 (jenjo02)
**	    Added E_DM0199_INVALID_ROOT_LOCATION,
**	    added mapping of E_DM0192_CREATEDB_RONLY_NOTEXIST.
**	18-mar-02 (inkdo01)
**	    Added E_DM9D01 to E_QE00A0 mapping (sequence exceeded limit).
**	    Acceptance of E_DM9D0A_SEQUENCE_ERROR, generic
**	    Sequence Generator error.
**	20-Aug-2002 (jenjo02)
**	    Map E_DM004C_LOCK_RESOURCE_BUSY to US126C_LOCK_RESOURCE_BUSY
**	    instead of US125E timeout.
**	    Added mapping of E_DM006B_NOWAIT_LOCK_BUSY to US126C.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, E_DM004D_LOCK_TIMER_EXPIRED is returned 
**	    instead..
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added I_QE2037_CONSTRAINT_TAB_IDX_ERR.
**	6-Feb-2004 (schka24)
**	   DM0020 no longer happens, remove mapping.
**	09-Apr-2004 (jenjo02)
**	    Added mapping of DM019A to QE007F
**	20-Apr-2004 (gupsh01)
**	    Added  alter table alter column errors E_DM019B, E_DM019C 
**	    and E_DM019D.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPQEFLIBDATA
**	    in the Jamfile.
**	11-Nov-2004 (gorvi01)
**	    Added unextenddb error entry for E_QE028E.
**	28-Dec-2004 (jenjo02)
**	    Deleted E_QE028E, use E_USnnnn errors instead.
**      02-mar-2005 (stial01)
**          New error for page type V5 WITH NODUPLICATES (b113993)
**      10-may-2005 (raodi01) bug110709 INGSRV2470
**	    Added QEF error map for E_DM0200_MOD_TEMP_TABLE_KEY and
**	    E_QE00A3_MOD_TEMP_TABLE_KEY.
**      22-mar-2006 (toumi01)
**	    Add I_QE2038_CONSTRAINT_TAB_IDX_ERR for MODIFY constraint
**	    edit message.
**	26-Jun-2006 (jonj)
**	    Map DM011F to QE005C.
**      28-Feb-2007 (wonca01) BUG 117806
**          Correct error code mapping for E_DM0192.
**	23-Oct-2007 (kibro01) b118918
**	    Add new error message DM01A0
**	29-jan-2008 (dougi)
**	    Add QE00A8/9 to map to US0874/5 (first/offset n error).
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      21-Jan-2009 (horda03) Bug 121519
**          Map E_DM0021_TABLES_TOO_MANY to E_QE00AA_ERROR_CREATING_TABLE.
**	7-May-2009 (kschendel) b122040
**	    Map CUT child abort CU0204 to "query has been aborted", since
**	    it's analogous to a DMF interrupt.
**      16-Jun-2009 (hanal04) Bug 122117
**          Map E_DM016B_LOCK_INTR_FA to E_QE0022_QUERY_ABORTED
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DM0020_UNABLE_TO_SERIALIZE,
**	    DM0028_UNABLE_TO_MAKE_CONSISTENT,
**	    DM0029_ROW_UPDATE_CONFLICT, DM0012_MVCC_INCOMPATIBLE,
**	    DM0013_MVCC_DISABLED.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC: Map DM0028_UNABLE_TO_MAKE_CONSISTENT
**	    to US125A, already reported by dm0p.
**	19-Mar-2010 (gupsh01) SIR 123444
**	    Added error E_QE016C_RENAME_TAB_HAS_OBJS for alter table 
**	    rename support. 
**	21-apr-2010 (toumi01) SIR 122403
**	    Qef_err_map changes for column encryption.
**	04-Aug-2010 (miket) SIR 122403
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	25-aug-2010 (miket) SIR 122403 SD 145781
**	    Better msg for alter table not valid for encrypted tables.
**	09-Sep-2010 (miket) SIR 122403 SD 145781
**	    Fix cut-and-paste error; associate with E_DM00AD and 9413
**	    message E_QE0193 and not E_QE0192.
*/

/* Jam hints
**
LIBRARY = IMPQEFLIBDATA
**
*/

/*
**  Data from qeferror.c
*/

typedef struct	_QEF_ERR_MAP
{
    i4	err_no;		/* error code from other facilities */
    i4	qef_code;	/* mapped QEF error code
				** if E_QE0122_ALREADY_REPORTED, no additional
				** messages will be reported or logged. */
    i4		user_code;	/* mapped user error message number
				**  0 - the error is considered internal 
				** -1 - user code is the same as qef code */
    i4		log_flg;	/* if 1, log message in system error log */
} QEF_ERR_MAP;


GLOBALDEF const QEF_ERR_MAP Qef_err_map[] = 
{
    0L,				    E_QE0004_NO_TRANSACTION,	    2177,   0,
    0L,				    E_QE0005_CANNOT_ESCALATE,	    -1,	    1,
    0L,				    E_QE0006_TRANSACTION_EXISTS,    2172,   0,
    0L,				    E_QE0007_NO_CURSOR,		    4600,   0,
    0L,				    E_QE0008_CURSOR_NOT_OPENED,	    4601,   0,
    0L,				    E_QE0009_NO_PERMISSION,	    4602,   0,
    0L,				    E_QE000A_READ_ONLY,		    4603,   0,
    0L,				    E_QE000B_ADF_INTERNAL_FAILURE,  -1,	    1,
    0L,				    E_QE000C_CURSOR_ALREADY_OPENED, 4604,   0,
    E_UL0005_NOMEM,		    E_QE000D_NO_MEMORY_LEFT,	    -1,	    1,
    E_QS0001_NOMEM,		    E_QE000D_NO_MEMORY_LEFT,	    -1,	    1,
    E_QS0019_UNKNOWN_OBJ,	    E_QE0014_NO_QUERY_PLAN,	    -1,	    1,
    E_AD2005_BAD_DTLEN,		    E_QE0308_COL_SIZE_MISCOMPILED,  -1,	    0,
    0L,				    E_QE000E_ACTIVE_COUNT_EXCEEDED, 4707,   1,
    0L,				    E_QE000F_OUT_OF_OTHER_RESOURCES,-1,	    1,
    0L,				    E_QE0010_DUPLICATE_ROW,	    4504,   0,
    0L,				    E_QE0012_DUPLICATE_KEY_INSERT,  4500,   0,
    0L,				    E_QE0013_INTEGRITY_FAILED,	    4505,   0,
    0L,				    E_QE0017_BAD_CB,		    -1,	    1,
    0L,				    E_QE0018_BAD_PARAM_IN_CB,	    -1,	    1,
    0L,				    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    0L,				    E_QE001A_DATATYPE_OVERFLOW,	    0,	    0,
    0L,				    E_QE001B_COPY_IN_PROGRESS,	    5866,   0,
    0L,				    E_QE001C_NO_COPY,		    5867,   0,
    0L,				    E_QE001D_FAIL,		    -1,	    1,
    0L,				    E_QE001E_NO_MEM,		    -1,	    1,
    0L,				    E_QE001F_INVALID_REQUEST,	    -1,	    1,
    0L,				    E_QE0020_OPEN_CURSOR,	    4604,   0,
    E_CU0204_ASYNC_STATUS,	    E_QE0022_QUERY_ABORTED,         4708,   1,
    0L,				    E_QE0023_INVALID_QUERY,	    0,	    0,
    0L,				    E_QE0025_USER_ERROR,	    0,	    0,
    0L,				    E_QE0026_DEFERRED_EXISTS,	    4607,   0,
    0L,				    E_QE0027_BAD_QUERY_ID,	    -1,	    1,
    0L,				    E_QE003A_FOPEN_FROM,	    5805,   0,
    0L,				    E_QE003B_FOPEN_TO,		    5806,   0,
    0L,				    E_QE003C_COLUMN_TOO_SMALL,	    5808,   0,
    0L,				    E_QE003D_BAD_INPUT_STRING,	    5809,   0,
    0L,				    E_QE003E_UNEXPECTED_EOF,	    5810,   0,
    0L,				    E_QE003F_UNTERMINATED_C0,	    5815,   0,
    0L,				    E_QE0040_NO_FULL_PATH,	    5816,   0,
    0L,				    E_QE0041_DUPLICATES_IGNORED,    5819,   0,
    0L,				    E_QE0042_CONTROL_TO_BLANK,	    5820,   0,
    0L,				    E_QE0043_TRUNC_C0,		    5821,   0,
    0L,				    E_QE0044_ILLEGAL_FTYPE,	    5824,   0,
    0L,				    E_QE0045_BINARY_F_T0,	    5825,   0,
    0L,				    E_QE0046_ERROR_ON_ROW,	    5826,   0,
    0L,				    E_QE0047_INVALID_CHAR,	    5827,   0,
    0L,				    E_QE004B_NOT_ALL_ROWS,	    0,	    0,
    0L,				    E_QE004C_NOT_ZEROONE_ROWS,	    4502,   0,
    0L,				    E_QE004D_DUPLICATE_KEY_UPDATE,  4501,   0,
    0L,				    E_QE0050_TEMP_TABLE_EXISTS,	    -1,	    1,
    0L,				    E_QE0097_NOT_A_VIEW,	    -1,     1,
    0L,				    E_QE0310_INVALID_TPROC_ACT,     -1,	    1,
    0L,				    E_QE009C_UNKNOWN_ERROR,	    -1,	    1,
    0L,				    E_QE00A1_IISEQ_NOTDEFINED,	    -1,	    1,
    0L,				    E_QE00A6_NO_DATA,         	    -1,	    1,
    0L,				    E_QE00A8_BAD_FIRSTN,	    2164,   1,
    0L,				    E_QE00A9_BAD_OFFSETN,	    2165,   1,
    0L,				    E_QE0105_CORRUPTED_CB,	    -1,     1,
    E_DM0006_BAD_ATTR_FLAGS,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM0007_BAD_ATTR_NAME,	    E_QE002D_BAD_ATTR_NAME,	    4611,   0,
    E_DM0008_BAD_ATTR_PRECISION,    E_QE002F_BAD_ATTR_PRECISION,    4613,   0,
    E_DM0009_BAD_ATTR_SIZE,	    E_QE002E_BAD_ATTR_SIZE,	    4612,   0,
    E_DM000A_BAD_ATTR_TYPE,	    E_QE002C_BAD_ATTR_TYPE,	    4610,   0,
    E_DM000B_BAD_CB_LENGTH,         E_QE001D_FAIL,		    -1,	    1,
    E_DM000C_BAD_CB_TYPE,           E_QE001D_FAIL,		    -1,	    1,
    E_DM000D_BAD_CHAR_ID,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM000E_BAD_CHAR_VALUE,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM000F_BAD_DB_ACCESS_MODE,    E_QE005F_BAD_DB_ACCESS_MODE,    0004,   0,
    E_DM0010_BAD_DB_ID,		    E_QE0060_BAD_DB_ID,		    -1,	    0,
    E_DM0011_BAD_DB_NAME,	    E_QE0061_BAD_DB_NAME,	    0016,   0,
    E_DM0012_MVCC_INCOMPATIBLE,	    -1,				    4620,   0,
    E_DM0013_MVCC_DISABLED,	    -1,				    4620,   0,
    E_DM001A_BAD_FLAG,		    E_QE001D_FAIL,		    -1,	    1,
    E_DM001B_BAD_INDEX,		    E_QE0062_BAD_INDEX,		    -1,	    0,
    E_DM001C_BAD_KEY_SEQUENCE,	    E_QE004A_KEY_SEQ,		    5528,   0,
    E_DM001D_BAD_LOCATION_NAME,	    E_QE0030_BAD_LOCATION_NAME,	    4614,   0,
    E_DM001E_DUP_LOCATION_NAME,	    E_QE009A_DUP_LOCATION_NAME,	    -1,	    0,
    E_DM001F_LOCATION_LIST_ERROR,   E_QE009B_LOCATION_LIST_ERROR,   -1,	    0,
    E_DM0020_UNABLE_TO_SERIALIZE,   -1,				    4699,   0,
    E_DM0021_TABLES_TOO_MANY,       E_QE00AA_ERROR_CREATING_TABLE,  2093,   1,
    E_DM0028_UNABLE_TO_MAKE_CONSISTENT,    -1L,			    4698,   0,
    E_DM0029_ROW_UPDATE_CONFLICT,   -1L,			    -1,     0,
    E_DM002A_BAD_PARAMETER,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM002B_BAD_RECORD_ID,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM002C_BAD_SAVEPOINT_NAME,    E_QE0032_BAD_SAVEPOINT_NAME,    2170,   0,
    E_DM002D_BAD_SERVER_ID,         E_QE001D_FAIL,		    -1,	    1,
    E_DM002E_BAD_SERVER_NAME,       E_QE001D_FAIL,		    -1,	    1,
    E_DM002F_BAD_SESSION_ID,        E_QE001D_FAIL,		    -1,	    1,
    E_DM0030_BAD_SESSION_NAME,      E_QE001D_FAIL,		    -1,	    1,
    E_DM0039_BAD_TABLE_NAME,	    E_QE0028_BAD_TABLE_NAME,	    4608,   0,
    E_DM003A_BAD_TABLE_OWNER,	    E_QE0063_BAD_TABLE_OWNER,	    4609,   0,
    E_DM003B_BAD_TRAN_ID,	    E_QE0064_BAD_TRAN_ID,	    -1,	    1,
    E_DM003C_BAD_TID,		    E_QE0033_BAD_TID,		    -1,	    1,
    E_DM003E_DB_ACCESS_CONFLICT,    E_QE0065_DB_ACCESS_CONFLICT,    4617,   0,
    E_DM003F_DB_OPEN,		    E_QE0066_DB_OPEN,		    4618,   0,
    E_DM0040_DB_OPEN_QUOTA_EXCEEDED,E_QE0067_DB_OPEN_QUOTA_EXCEEDED,4707,   1,
    E_DM0041_DB_QUOTA_EXCEEDED,	    E_QE0068_DB_QUOTA_EXCEEDED,	    4707,   1,
# ifdef xDEV_TEST
    E_DM0042_DEADLOCK,		    E_QE002A_DEADLOCK,		    4700,   1,
# else
    E_DM0042_DEADLOCK,		    E_QE002A_DEADLOCK,		    4700,   0,
# endif
    E_DM0044_DELETED_TID,	    E_QE0069_DELETED_TID,	    -1,	    0,
    E_DM0045_DUPLICATE_KEY,	    E_QE0039_DUPLICATE_KEY,	    5521,   0,
    E_DM0046_DUPLICATE_RECORD,	    E_QE0038_DUPLICATE_RECORD,	    4616,   0,
    E_DM0047_CHANGED_TUPLE,	    E_QE0011_AMBIGUOUS_REPLACE,	    4701,   0,
    E_DM0048_SIDUPLICATE_KEY,	    E_QE0039_DUPLICATE_KEY,	    5521,   0,
    E_DM004B_LOCK_QUOTA_EXCEEDED,   E_QE0034_LOCK_QUOTA_EXCEEDED,   4705,   1,
    E_DM004C_LOCK_RESOURCE_BUSY,    E_QE0035_LOCK_RESOURCE_BUSY,    4716,   0,
    E_DM004D_LOCK_TIMER_EXPIRED,    E_QE0036_LOCK_TIMER_EXPIRED,    4702,   0,
    E_DM004F_LOCK_RETRY,	    -1L,			    -1,	    0,
    E_DM0050_TABLE_NOT_LOADABLE,    -1L,			    -1,	    0,
    E_DM0052_COMPRESSION_CONFLICT,  E_QE0118_COMPRESSION_CONFLICT,  5984,   0,
    E_DM0053_NONEXISTENT_DB,	    E_QE006A_NONEXISTENT_DB,	    4619,   0,
    E_DM0054_NONEXISTENT_TABLE,	    E_QE0031_NONEXISTENT_TABLE,	    5845,   0,
    E_DM0055_NONEXT,                E_QE0015_NO_MORE_ROWS,	     0,	    0,
    E_DM0056_NOPART,                -1L,			    -1,	    0,
    E_DM0057_NOROOM,		    -1L,			    -1,	    0,
    E_DM0058_NOTIDP,		    -1L,			    -1,	    1,
    E_DM0059_NOT_ALL_KEYS,	    -1L,			    -1,	    0,
    E_DM005A_CANT_MOD_CORE_STRUCT,  E_QE006C_CANT_MOD_CORE_SYSCAT,  -1,	    0,
    E_DM005B_SESSION_OPEN,	    -1L,			    -1,	    1,
    E_DM005C_SESSION_QUOTA_EXCEEDED,E_QE006B_SESSION_QUOTA_EXCEEDED,4707,   1,
    E_DM005D_TABLE_ACCESS_CONFLICT, E_QE0051_TABLE_ACCESS_CONFLICT, 5006,   0,
    E_DM005E_CANT_UPDATE_SYSCAT,    -1L,			    -1,	    0,
    E_DM005F_CANT_INDEX_CORE_SYSCAT,E_QE006D_CANT_INDEX_CORE_SYSCAT,5305,   0,
    E_DM0060_TRAN_IN_PROGRESS,	    E_QE006E_TRAN_IN_PROGRESS,	    2172,   0,
    E_DM0061_TRAN_NOT_IN_PROGRESS,  E_QE006F_TRAN_NOT_IN_PROGRESS,  2177,   0,
    E_DM0062_TRAN_QUOTA_EXCEEDED,   E_QE0070_TRAN_QUOTA_EXCEEDED,   -1,     1,
    E_DM0063_TRAN_TABLE_OPEN,	    E_QE0071_TRAN_TABLE_OPEN,	    -1,	    1,
    E_DM0064_USER_ABORT,	    E_QE0022_QUERY_ABORTED,         4708,   1,
    E_DM0065_USER_INTR,		    E_QE0022_QUERY_ABORTED,	    4708,   1,
    E_DM0066_BAD_KEY_TYPE,	    E_QE0022_QUERY_ABORTED,	    9410,   1,	
    E_DM0067_READONLY_TABLE_ERR,    I_QE2034_READONLY_TABLE_ERR,    5350,   1,
    E_DM0068_MODIFY_READONLY_ERR,   -1L,			    -1,	    0,
    E_DM0069_CONCURRENT_IDX_ERR,    -1L,			    5351,   0,
    E_DM006A_TRAN_ACCESS_CONFLICT,  E_QE005B_TRAN_ACCESS_CONFLICT,  5007,   0,
    E_DM006D_BAD_OPERATION_CODE,    E_QE001D_FAIL,		    -1,	    1,
    E_DM006F_SERVER_ACTIVE,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM0070_SERVER_STOPPED,	    -1L,			    -1,	    0,
    E_DM0071_LOCATIONS_TOO_MANY,    E_QE0072_LOCATIONS_TOO_MANY,    -1,	    0,
    E_DM0072_NO_LOCATION,	    -1L,                            -1,     1,
    E_DM0073_RECORD_ACCESS_CONFLICT,-1L,			    -1,	    0,
    E_DM0074_NOT_POSITIONED,	    E_QE0021_NO_ROW,		    4605,   0,
    E_DM0075_BAD_ATTRIBUTE_ENTRY,   E_QE001D_FAIL,		    -1,	    1,
    E_DM0076_BAD_INDEX_ENTRY,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM0077_BAD_TABLE_CREATE,	    E_QE0074_BAD_TABLE_CREATE,	    4707,   0,
    E_DM0078_TABLE_EXISTS,	    E_QE002B_TABLE_EXISTS,	    5102,   0,
    E_DM0079_BAD_OWNER_NAME,	    E_QE0029_BAD_OWNER_NAME,	    4609,   0,
    E_DM007B_CANT_CREATE_TABLEID,   E_QE0116_CANT_ALLOCATE_DBP_ID,  -1,     0,
    0L,				    E_QE0117_NONEXISTENT_DBP,	    -1,	    0,
    0L,				    E_QE011C_NONEXISTENT_SYNONYM,    0,     1,
    0L,				    E_QE0201_BAD_SORT_KEY,	    6338,   0,
    E_DM007C_BAD_KEY_DIRECTION,	    E_QE0075_BAD_KEY_DIRECTION,	    -1,	    0,
    E_DM007D_BTREE_BAD_KEY_LENGTH,  E_QE0076_BTREE_BAD_KEY,	    5538,   0,
    E_DM007E_LOCATION_EXISTS,	    E_QE0077_LOCATION_EXISTS,	    4614,   0,
    E_DM0080_BAD_LOCATION_AREA,	    -1L,			    -1,	    0,
    E_DM0083_ERROR_STOPPING_SERVER, E_QE001D_FAIL,		    -1,	    1,
    E_DM0084_ERROR_ADDING_DB,	    E_QE0078_ERROR_ADDING_DB,	    -1,	    0,
    E_DM0085_ERROR_DELETING_DB,	    E_QE0079_ERROR_DELETING_DB,	    -1,	    0,
    E_DM0086_ERROR_OPENING_DB,	    E_QE007A_ERROR_OPENING_DB,	    -1,	    0,
    E_DM0087_ERROR_CLOSING_DB,	    E_QE007B_ERROR_CLOSING_DB,	    -1,	    0,
    E_DM0088_ERROR_ALTERING_SERVER, -1L,			    -1,	    0,
    E_DM0089_ERROR_SHOWING_SERVER,  -1L,			    -1,	    0,
    E_DM008A_ERROR_GETTING_RECORD,  E_QE007C_ERROR_GETTING_RECORD,  -1,	    1,
    E_DM008B_ERROR_PUTTING_RECORD,  E_QE007D_ERROR_PUTTING_RECORD,  -1,	    1,
    E_DM008C_ERROR_REPLACING_RECORD,E_QE007E_ERROR_REPLACING_RECORD,-1,	    1,
    E_DM008D_ERROR_DELETING_RECORD, E_QE0037_ERROR_DELETING_RECORD, 4615,   0,
    E_DM008E_ERROR_POSITIONING,	    E_QE0080_ERROR_POSITIONING,	    -1,	    1,
    E_DM008F_ERROR_OPENING_TABLE,   E_QE0081_ERROR_OPENING_TABLE,   -1,	    1,
    E_DM0090_ERROR_CLOSING_TABLE,   E_QE0082_ERROR_CLOSING_TABLE,   -1,	    1,
    E_DM0091_ERROR_MODIFYING_TABLE, E_QE0083_ERROR_MODIFYING_TABLE, -1,	    1,
    E_DM0092_ERROR_INDEXING_TABLE,  E_QE0084_ERROR_INDEXING_TABLE,  -1,	    1,
    E_DM0093_ERROR_SORTING,	    E_QE0085_ERROR_SORTING,	    -1,	    1,
    E_DM0094_ERROR_BEGINNING_TRAN,  E_QE0086_ERROR_BEGINNING_TRAN,  -1,	    0,
    E_DM0095_ERROR_COMMITING_TRAN,  E_QE0087_ERROR_COMMITING_TRAN,  -1,	    0,
    E_DM0096_ERROR_ABORTING_TRAN,   E_QE0088_ERROR_ABORTING_TRAN,   -1,	    0,
    E_DM0097_ERROR_SAVEPOINTING,    E_QE0089_ERROR_SAVEPOINT,	    -1,	    0,
    E_DM0098_NOJOIN,		    -1L,			    -1,	    0,
    E_DM0099_ERROR_RESUME_TRAN,	    E_QE0057_ERROR_RESUME_TRAN,	    -1,	    0,
    E_DM009A_ERROR_SECURE_TRAN,	    E_QE0058_ERROR_SECURE_TRAN,	    -1,	    0,
    E_DM009B_ERROR_CHK_PATCH_TABLE, -1L,			    -1,     0,
    E_DM009D_BAD_TABLE_DESTROY,	    E_QE008A_BAD_TABLE_DESTROY,	    -1,	    0,
    E_DM009E_CANT_OPEN_VIEW,	    E_QE008B_CANT_OPEN_VIEW,	    -1,	    0,
    E_DM009F_ILLEGAL_OPERATION,	    E_QE001D_FAIL,		    -1,	    1,
    E_DM00A1_ERROR_CONVERTING_TBL,        -1L,                      -1,     1,
    E_DM00A5_ATBL_UNSUPPORTED,            -1L,		            -1,     1,	
    E_DM00A6_ATBL_COL_INDEX_KEY,          -1L,		            -1,     0,	
    E_DM00A7_ATBL_COL_INDEX_SEC,          -1L,		            -1,     0,	
    E_DM00AD_ENCRYPT_NO_ALTER_TABLE, E_QE0193_ENCRYPT_NO_ALTER_TABLE, 9413, 1,
    E_DM00D0_LOCK_MANAGER_ERROR,    E_QE008C_LOCK_MANAGER_ERROR,    -1,	    1,
    E_DM00D1_BAD_SYSCAT_MOD,	    E_QE008D_BAD_SYSCAT_MOD,	    -1,	    0,
    E_DM0100_DB_INCONSISTENT,	    E_QE0099_DB_INCONSISTENT,	    38,	    1,
    E_DM0101_SET_JOURNAL_ON,	    -1L,			    -1,	    0,
    E_DM0102_NONEXISTENT_SP,	    E_QE0016_INVALID_SAVEPOINT,	    2170,   0,
    E_DM0103_TUPLE_TOO_WIDE,	    E_QE0048_TUP_TOO_WIDE,	    2045,   0,
    E_DM0104_ERROR_RELOCATING_TABLE,E_QE008E_ERROR_RELOCATING_TABLE,-1,	    0,
    E_DM0105_ERROR_BEGIN_SESSION,   E_QE001D_FAIL,		    -1,	    1,
    E_DM0106_ERROR_ENDING_SESSION,  E_QE001D_FAIL,		    -1,	    1,
    E_DM0107_ERROR_ALTERING_SESSION,E_QE001D_FAIL,		    -1,	    1,
    E_DM0108_ERROR_DUMPING_DATA,    E_QE008F_ERROR_DUMPING_DATA,    -1,	    0,
    E_DM0109_ERROR_LOADING_DATA,    E_QE0090_ERROR_LOADING_DATA,   4707,    0,
    E_DM010A_ERROR_ALTERING_TABLE,  E_QE0091_ERROR_ALTERING_TABLE,  -1,     0,
    E_DM010B_ERROR_SHOWING_TABLE,   E_QE0092_ERROR_SHOWING_TABLE,   -1,	    0,
    E_DM010C_TRAN_ABORTED,	    E_QE0024_TRANSACTION_ABORTED,   4706,   1,
    E_DM010D_ERROR_ALTERING_DB,	    E_QE0093_ERROR_ALTERING_DB,	    -1,     0,
    E_DM010F_ISAM_BAD_KEY_LENGTH,   E_QE0094_ISAM_BAD_KEY,	   5537,    0,
    E_DM0110_COMP_BAD_KEY_LENGTH,   E_QE0095_COMP_BAD_KEY,	   5527,    0,
    E_DM0111_MOD_IDX_UNIQUE,	    E_QE0096_MOD_IDX_UNIQUE,	    -1,	    1,
    E_DM0112_RESOURCE_QUOTA_EXCEED, E_QE0052_RESOURCE_QUOTA_EXCEED, 4707,   1,
    E_DM0114_FILE_NOT_FOUND,	    -1L,			    -1,	    0,
    E_DM0119_TRAN_ID_NOT_UNIQUE,    E_QE0053_TRAN_ID_NOT_UNIQUE,    4710,   1,
    E_DM011A_ERROR_DB_DIR,	    -1L,			    -1,	    0,
    E_DM011C_ERROR_DB_DESTROY,	    E_QE0079_ERROR_DELETING_DB,	    -1,	    0,
    E_DM011F_INVALID_TXN_STATE,	    E_QE005C_INVALID_TRAN_STATE,    -1,     1,
    E_DM0120_DIS_TRAN_UNKNOWN,	    E_QE0054_DIS_TRAN_UNKNOWN,	    4711,   1,
    E_DM0121_DIS_TRAN_OWNER,	    E_QE0055_DIS_TRAN_OWNER,	    4712,   1,
    E_DM0122_CANT_SECURE_IN_CLUSTER, E_QE0056_CANT_SECURE_IN_CLUSTER, 4713,   1,
    E_DM0125_DDL_SECURITY_ERROR,    I_QE2033_SEC_VIOLATION,         6500,   0,
    E_DM0129_SECURITY_CANT_UPDATE,  I_QE2033_SEC_VIOLATION,         6500,   1,
    0L,				    E_QE0130_NONEXISTENT_IPROC,	    -1,	    1,
    0L,				    E_QE0131_MISSING_IPROC_PARAM,   -1,	    1,
    0L,				    E_QE0132_WRONG_PARAM_NAME,	    -1,	    1,
    0L,				    E_QE0133_WRONG_PARAM_TYPE,	    -1,	    1,
    E_DM0130_DMMLIST_ERROR,	    E_QE0134_LISTFILE_ERROR,	    -1,	    1,
    E_DM0131_DMMDEL_ERROR,	    E_QE0135_DROPFILE_ERROR,	    -1,	    1,
    E_DM0132_ILLEGAL_STMT,	    E_QE0059_ILLEGAL_STMT,	    4714,   1,
    E_DM0134_DIS_DB_UNKNOWN,	    E_QE005A_DIS_DB_UNKNOWN,	    4715,   1,
    E_DM0136_GATEWAY_ERROR,         E_QE0400_GATEWAY_ERROR,         -1,     1,
    E_DM0137_GATEWAY_ACCESS_ERROR,  E_QE0401_GATEWAY_ACCESS_ERROR,  -1,     1,
    E_DM013C_BAD_LOC_TYPE,	    E_QE0243_CNF_INCONSIST,	    0,	    1,
    E_DM013D_LOC_NOT_EXTENDED,	    E_QE0243_CNF_INCONSIST,	    0,	    1,
    E_DM013E_TOO_FEW_LOCS,	    E_QE0244_TOO_FEW_LOCS,	    -1,	    0,
    E_DM013F_TOO_MANY_LOCS,	    E_QE0245_TOO_MANY_LOCS,	    -1,	    0,
    E_DM0140_EMPTY_DIRECTORY,	    -1L,			     0,	    0,
    E_DM0141_GWF_DATA_CVT_ERROR,    E_QE0122_ALREADY_REPORTED,	    -1,     0,
    E_DM0142_GWF_ALREADY_REPORTED,  E_QE0122_ALREADY_REPORTED,	    -1,     0,
    E_DM0144_NONFATAL_FINDBS_ERROR, E_QE0136_FINDDBS_ERROR,	    -1,	    1,
    E_DM0145_FATAL_FINDBS_ERROR,    E_QE0137_SEVERE_FINDDBS_ERR,    -1,	    1,
    E_DM0146_SETLG_XCT_ABORT,  	    -1L,	    		    -1,     0,
    E_DM0149_SETLG_SVPT_ABORT,      E_QE0122_ALREADY_REPORTED,	    -1,     0,
    E_DM0150_DUPLICATE_KEY_STMT,    E_QE0039_DUPLICATE_KEY,	    5521,   0,
    E_DM0151_SIDUPLICATE_KEY_STMT,  E_QE0039_DUPLICATE_KEY,	    5521,   0,
    E_DM0157_NO_BMCACHE_BUFFERS,    E_QE0309_NO_BMCACHE_BUFFERS,    -1,     1,
    E_DM0159_NODUPLICATES,	    E_QE0048_TUP_TOO_WIDE,	    6428,   0, 
    E_DM0160_MODIFY_ALTER_STATUS_ERR,     -1L,		            -1,     0,	
    E_DM0161_LOG_INCONSISTENT_TABLE_ERR,  -1L,			    5353,   0,
    E_DM0162_PHYS_INCONSISTENT_TABLE_ERR, -1L,		            5354,   0,
    E_DM0168_INCONSISTENT_INDEX_ERR,      -1L,		            5357,   0,
    E_DM0169_ALTER_TABLE_SUPP, 	    E_QE009D_ALTER_TABLE_SUPP,      -1,     0,
    E_DM016B_LOCK_INTR_FA,	    E_QE0022_QUERY_ABORTED,	    4708,   1,
    E_DM0170_READONLY_TABLE_INDEX_ERR,I_QE2034_READONLY_TABLE_ERR,  5355,   1,
    E_DM0174_ENCRYPT_LOCKED,        E_QE0190_ENCRYPT_LOCKED,        9407,   1,
    E_DM0177_RECORD_NOT_ENCRYPTED,  E_QE0191_RECORD_NOT_ENCRYPTED,  9408,   1,
    E_DM0178_PASSPHRASE_FAILED_CRC, E_QE0192_PASSPHRASE_FAILED_CRC, 9409,   1,
    E_DM0181_PROD_MODE_ERR,         E_QE7612_PROD_MODE_ERR,         -1,     1,
    E_DM0180_SPROD_UPD_IIDB,        E_QE7610_SPROD_UPD_IIDB,        -1,     1,
    E_DM0183_CANT_MOD_TEMP_TABLE,   E_QE009E_CANT_MOD_TEMP_TABLE,   2049,   0,
    E_DM0185_MAX_COL_EXCEEDED,      -1L,			    2011,   0,
    E_DM0186_MAX_TUPLEN_EXCEEDED,   -1L,   			    2045,   0,
    E_DM0189_RAW_LOCATION_INUSE,    -1L,			    -1,	    0,
    E_DM0190_RAW_LOCATION_OCCUPIED, -1L,			    -1,	    0,
    E_DM0191_RAW_LOCATION_MISSING,  -1L,			    -1,	    0,
    E_DM0192_CREATEDB_RONLY_NOTEXIST,-1L,			    144,	    0,
    E_DM0193_RAW_LOCATION_EXTEND,   -1L,			    6359,   0,
    E_DM0195_AREA_IS_RAW,	    -1L,			    6368,   0,
    E_DM0196_AREA_NOT_RAW,          -1L,			    6369,   0,
    E_DM0197_INVALID_RAW_USAGE,     -1L,			    6366,   0,
    E_DM0198_RUN_MKRAWAREA,	    -1L,			    6367,   0,
    E_DM0199_INVALID_ROOT_LOCATION, -1L,			    6373,   0,
    E_DM019A_ERROR_UNFIXING_PAGES,  E_QE007F_ERROR_UNFIXING_PAGES,  -1,	    1,
    E_DM019B_INVALID_ALTCOL_PARAM,  -1L,			    6422,   0,
    E_DM019C_ACOL_KEY_NOT_ALLOWED,  -1L,			    6423,   0,
    E_DM019D_ROW_CONVERSION_FAILED, -1L,			    -1,   0,
    E_DM019F_INVALID_ENCRYPT_INDEX, -1L,			    9411,   0,
    E_DM01A0_INVALID_ALTCOL_DEFAULT,-1L,			    -1,   0,
    E_DM0200_MOD_TEMP_TABLE_KEY,    E_QE00A3_MOD_TEMP_TABLE_KEY,    2053,   0, 
    E_DM201E_SECURITY_DML_NOPRIV,   I_QE2033_SEC_VIOLATION,         6501,   0,
    E_DM201F_ROW_TABLE_LABEL,       I_QE2033_SEC_VIOLATION,         6502,   0,
    E_DM9D01_SEQUENCE_EXCEEDED,	    E_QE00A0_SEQ_LIMIT_EXCEEDED,    6421,   0,
    E_DM9D02_SEQUENCE_NOT_FOUND,    E_QE00A2_CURRVAL_NONEXTVAL,     -1,     0,
    E_DM9D0A_SEQUENCE_ERROR,	    E_QE0122_ALREADY_REPORTED,      -1,     0,
    0L,				    E_QE0110_NO_ROWS_QUALIFIED,	    -1,	    0,
    E_UL0101_TABLE_ID,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0104_FULL_TABLE,	    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0105_FULL_CLASS,	    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0106_OBJECT_ID,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0108_DUPLICATE,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0109_NFND,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010A_MEMBER,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010B_NOT_MEMBER,	    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010C_UNKNOWN,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010D_TABLE_MEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010E_TCB_MEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL010F_BUCKET_MEM,	    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0110_CHDR_MEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0111_OBJ_MEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0112_INIT_SEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0113_TABLE_SEM,		    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    E_UL0117_OBJECT_SEM,	    E_QE0019_NON_INTERNAL_FAILURE,  -1,	    1,
    0L,				    E_QE0111_ERROR_SENDING_MESSAGE, -1,	    1,
    0L,				    E_QE0112_DBP_RETURN,	    -1,	    0,
    0L,				    E_QE0113_PARAM_CONVERSION,	    -1,	    0,
    0L,				    E_QE0114_PARAM_MISSING,	    -1,	    0,
    0L,				    E_QE0115_TABLES_MODIFIED,	    -1,	    0,
    0L,				    E_QE011B_PARAM_MISSING,	    -1,	    0,
    0L,				    E_QE011D_PERIPH_CONVERSION,	    -1,	    0,
    0L,				    E_QE0123_PARAM_INVALID,	    -1,	    0,
    0L,				    E_QE0124_PARAM_MANY,	    -1,	    0,
    0L,				    E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT,	    -1,	    0,
    0L,				    E_QE0139_CANT_ADD_REF_CONSTRAINT, -1,   0,
    0L,				    E_QE013A_CANT_CREATE_CONSTRAINT, -1,    0,
    0L,				    E_QE0140_CANT_EXLOCK_REF_TABLE, -1,	    0,
    0L,				    E_QE0144_CANT_ADD_CHECK_CONSTRAINT, -1,	    0,
    0L,				    E_QE0145_CANT_EXLOCK_CHECK_TABLE, -1,	    0,
    0L,				    E_QE0166_NO_CONSTRAINT_IN_SCHEMA, -1,   0,
    0L,				    E_QE0167_DROP_CONS_WRONG_TABLE, -1,     0,
    0L,				    E_QE0168_ABANDONED_CONSTRAINT,  -1,	    0,
    0L,				    E_QE0169_UNKNOWN_OBJTYPE_IN_DROP, -1,   1,
    0L,				    E_QE016A_NOT_DROPPABLE_CONS,    -1,     0,
    0L,				    E_QE016B_COLUMN_HAS_OBJECTS,    -1,     0,
    0L,				    E_QE016C_RENAME_TAB_HAS_OBJS,   9373,     1,
    0L,				    E_QE016D_RENAME_TAB_HAS_PROC,   0,     1,
    0L,				    E_QE016E_RENAME_TAB_HAS_VIEW,   0,     1,
    0L,				    E_QE016F_RENAME_TAB_HAS_RULE,   0,     1,
    0L,				    E_QE0170_RENAME_TAB_HAS_NGPERM, 0,     1,
    0L,				    E_QE0171_RENAME_TAB_HAS_CONS,   0,     1,
    0L,				    E_QE0172_RENAME_TAB_HAS_SECALARM, 0,    1,
    0L,				    E_QE0173_RENAME_TAB_IS_CATALOG,   9366,     0,
    0L,				    E_QE0174_RENAME_TAB_IS_SYSGEN,   9367,     0,
    0L,				    E_QE0175_RENAME_TAB_IS_ETAB,   9368,     0,
    0L,				    E_QE0176_RENAME_TAB_IS_GATEWAY,   9369,     0,
    0L,				    E_QE0177_RENAME_TAB_IS_PARTITION,   9370,     0,
    0L,				    E_QE0178_RENAME_TAB_IS_INCONSIST,   9371,     0,
    0L,				    E_QE0179_RENAME_TAB_IS_READONLY,   9372,     0,
    0L,				    E_QE0180_RENAME_TAB_HAS_INTEGRITY,   0,     1,
    0L,				    E_QE0181_RENAME_COL_HAS_OBJS,   9374,     1,
    0L,				    E_QE0199_CALL_ALLOC,	    -1,     0,
    0L,				    E_QE019A_EVENT_MESSAGE,	    -1,     0,
E_DM0126_ERROR_WRITING_SECAUDIT,    E_QE019C_SEC_WRITE_ERROR,       -1,     1,
    0L,                             E_QE019C_SEC_WRITE_ERROR,       -1,     1,
    0L,				    E_QE0200_NO_EVENTS,		    -1,     0,
    0L,				    E_QE0202_RESOURCE_CHCK_INVALID, -1,	    1,
    0L,				    E_QE0203_DIO_LIMIT_EXCEEDED,    -1,	    0,
    0L,				    E_QE0204_ROW_LIMIT_EXCEEDED,    -1,	    0,
    0L,				    E_QE0205_CPU_LIMIT_EXCEEDED,    -1,	    0,
    0L,				    E_QE0206_PAGE_LIMIT_EXCEEDED,   -1,	    0,
    0L,				    E_QE0207_COST_LIMIT_EXCEEDED,   -1,	    0,
    0L,				    E_QE0208_EXCEED_MAX_CALL_DEPTH, -1,	    0,
    0L,				    E_QE0209_BAD_RULE_STMT, 	    -1,	    0,

    0L,				    E_QE020A_EVENT_EXISTS,	    -1,     0,
    0L,				    E_QE020B_EVENT_ABSENT,	    -1,     0,
    0L,				    E_QE020C_EV_REGISTER_EXISTS,    -1,     0,
    0L,				    E_QE020D_EV_REGISTER_ABSENT,    -1,     0,
    0L,				    E_QE020E_EV_PERMIT_ABSENT,      -1,     0,
    0L,				    E_QE020F_EVENT_SCF_FAIL,	    -1,     1,

    0L,				    E_QE0210_QEC_ALTER, 	    -1,	    1,
    0L,				    E_QE0211_DEFAULT_GROUP,	    6248,   0,
    0L,				    E_QE0212_DEFAULT_MEMBER,	    6249,   0,
    0L,				    E_QE0213_GRPID_EXISTS,	    6258,   0,
    0L,				    E_QE0214_GRPID_NOT_EXIST,	    6260,   0,
    0L,				    E_QE0215_GRPMEM_EXISTS,	    6262,   0,
    0L,				    E_QE0216_GRPMEM_NOT_EXIST,	    6263,   0,
    0L,				    E_QE0217_GRPID_NOT_EMPTY,	    6264,   0,
    0L,				    E_QE0218_APLID_EXISTS,	    6259,   0,
    0L,				    E_QE0219_APLID_NOT_EXIST,	    6261,   0,
    0L,				    E_QE021A_DBPRIV_NOT_AUTH,	    6281,   0,
    0L,				    E_QE021B_DBPRIV_NOT_DBDB,	    6282,   0,
    0L,				    E_QE021C_DBPRIV_DB_UNKNOWN,	    6291,   0,
    0L,				    E_QE021D_DBPRIV_NOT_GRANTED,    6292,   0,
    0L,				    E_QE021E_DBPRIV_USER_UNKNOWN,   6293,   0,
    0L,				    E_QE021F_DBPRIV_GROUP_UNKNOWN,  6294,   0,
    0L,				    E_QE0220_DBPRIV_ROLE_UNKNOWN,   6295,   0,
    0L,				    E_QE0221_GROUP_USER,	    6265,   0,
    0L,				    E_QE0222_GROUP_ROLE,	    6266,   0,
    0L,				    E_QE0223_ROLE_USER,		    6267,   0,
    0L,				    E_QE0224_ROLE_GROUP,	    6268,   0,
    0L,				    E_QE0227_GROUP_BAD_USER,	    6269,   0,
    0L,				    E_QE0228_DBPRIVS_REMOVED,	    6298,   0,
    0L,                             E_QE0229_DBPRIV_NOT_GRANTED,    6299,   0,
    0L,				    E_QE022A_IIUSERGROUP,	    6250,   1,
    0L,				    E_QE022B_IIAPPLICATION_ID,	    6251,   1,
    0L,				    E_QE022C_IIDBPRIV,		    6280,   1,
    0L,				    E_QE022D_IIDATABASE,	    6296,   1,
    0L,				    E_QE022E_IIUSER,		    6297,   1,
    0L,				    E_QE022F_SCU_INFO_ERROR,	     0,     1,
    0L,				    E_QE0230_USRGRP_NOT_EXIST,	    6390,   0,
    0L,				    E_QE0231_USRMEM_ADDED,	    6391,   0,
    0L,				    E_QE0232_USRMEM_DROPPED,	    6392,   0,
    0L,				    E_QE0233_ALLDBS_NOT_AUTH,	    6342,   0,
    0L,				    E_QE0234_USER_ROLE,		    6393,   0,
    0L,				    E_QE0235_USER_GROUP,	    6394,   0,
    0L,				    E_QE0237_DROP_DBA,		    6339,   0,
    0L,                             E_QE0238_NO_MSTRAN,		    2228,   0,
    0L,				    E_QE0240_BAD_LOC_TYPE,	    -1,	    0,
    0L,				    E_QE0241_DB_NOT_EXTENDED,	    -1,	    0,
    0L,				    E_QE0242_BAD_EXT_TYPE,	    -1,	    0,

    0L,				    E_QE0253_ALL_PRIV_NONE_REVOKED, -1,	    0,
    0L,				    E_QE0254_SOME_PRIV_NOT_REVOKED, -1,	    0,
    0L,				    E_QE0280_MAC_ERROR,		    -1,     1,
    0L,				    E_QE0281_DEF_LBL_ERROR,	    9328,   1,
    0L,				    E_QE0282_IIPROFILE,		    -1,     1,
    0L,				    E_QE0283_DUP_OBJ_NAME,          6395,   0,
    0L,				    E_QE0284_OBJECT_NOT_EXISTS,     6396,   0,
    0L,				    E_QE0285_PROFILE_USER_MRG_ERR,  -1,     1,
    0L,				    E_QE0286_PROFILE_ABSENT,        -1,     0,
    0L,				    E_QE0287_PROFILE_IN_USE,        -1,     0,
    0L,				    E_QE0288_DROP_DEFAULT_PROFILE,  -1,     0,
    0L,				    E_QE0289_EXTEND_DB_LOC_LABEL,   -1,     0,
    0L,				    E_QE028A_EXTEND_DB_LOC_ERR,     -1,     1,

    0L,				    E_QE0300_USER_MSG,		     0,	    1,
    0L,				    E_QE0301_TBL_SIZE_CHANGE,	     0,	    0,
    0L,                             E_QE0302_SCHEMA_EXISTS,         -1,     0,
    0L,                             E_QE030A_TTAB_PARM_ERROR,       -1,     0,
    0L,                             E_QE030B_RULE_PROC_MISMATCH,    2442,   0,
    0L,                             E_QE030C_BYREF_IN_ROWPROC,	    -1,	    1,
    0L,                             E_QE030D_NESTED_ROWPROCS,	    2447,   1,
    0L,                             E_QE030E_RULE_ROWPROCS,	    -1,	    1,
    0L,				    E_QE0258_ILLEGAL_XACT_STMT,	    -1,     1,
/* Distributed errors */
/*  RQF error codes start here */
    0L,                             E_QE0500_RQF_GENERIC_ERR,       -1,     1,
    E_RQ0001_WRONG_COLUMN_COUNT,    E_QE0501_WRONG_COLUMN_COUNT,    -1,     1,
    E_RQ0002_NO_TUPLE_DESCRIPTION,  E_QE0502_NO_TUPLE_DESCRIPTION,  -1,     1,
    E_RQ0003_TOO_MANY_COLUMNS,      E_QE0503_TOO_MANY_COLUMNS,      -1,     1,
    E_RQ0004_BIND_BUFFER_TOO_SMALL, E_QE0504_BIND_BUFFER_TOO_SMALL, -1,     1,
    E_RQ0005_CONVERSION_FAILED,     E_QE0505_CONVERSION_FAILED,     -1,     1,
    E_RQ0006_CANNOT_GET_ASSOCIATION,E_QE0506_CANNOT_GET_ASSOCIATION,-1,     0,
    E_RQ0007_BAD_REQUEST_CODE,      E_QE0507_BAD_REQUEST_CODE,      -1,     1,
    E_RQ0008_SCU_MALLOC_FAILED,     E_QE0508_SCU_MALLOC_FAILED,     -1,     1,
    E_RQ0009_ULM_STARTUP_FAILED,    E_QE0509_ULM_STARTUP_FAILED,    -1,     1,
    E_RQ0010_ULM_OPEN_FAILED,       E_QE0510_ULM_OPEN_FAILED,       -1,     1,
    E_RQ0011_INVALID_READ,          E_QE0511_INVALID_READ,          -1,     1,
    E_RQ0012_INVALID_WRITE,         E_QE0512_INVALID_WRITE,         -1,     1,
    E_RQ0013_ULM_CLOSE_FAILED,      E_QE0513_ULM_CLOSE_FAILED,      -1,     1,
    E_RQ0014_QUERY_ERROR,           E_QE0514_QUERY_ERROR,           -1,     0,
    E_RQ0015_UNEXPECTED_MESSAGE,    E_QE0515_UNEXPECTED_MESSAGE,    -1,     1,
    E_RQ0016_CONVERSION_ERROR,      E_QE0516_CONVERSION_ERROR,      -1,     1,
    E_RQ0017_NO_ACK,                E_QE0517_NO_ACK,                -1,     1,
    E_RQ0018_SHUTDOWN_FAILED,       E_QE0518_SHUTDOWN_FAILED,       -1,     1,
    E_RQ0019_COMMIT_FAILED,         E_QE0519_COMMIT_FAILED,         -1,     1,
    E_RQ0020_ABORT_FAILED,          E_QE0520_ABORT_FAILED,          -1,     1,
    E_RQ0021_BEGIN_FAILED,          E_QE0521_BEGIN_FAILED,          -1,     1,
    E_RQ0022_END_FAILED,            E_QE0522_END_FAILED,            -1,     1,
    E_RQ0023_COPY_FROM_EXPECTED,    E_QE0523_COPY_FROM_EXPECTED,    -1,     1,
    E_RQ0024_COPY_DEST_FAILED,      E_QE0524_COPY_DEST_FAILED,      -1,     1,
    E_RQ0025_COPY_SOURCE_FAILED,    E_QE0525_COPY_SOURCE_FAILED,    -1,     1,
    E_RQ0026_QID_EXPECTED,          E_QE0526_QID_EXPECTED,          -1,     1,
    E_RQ0027_CURSOR_CLOSE_FAILED,   E_QE0527_CURSOR_CLOSE_FAILED,   -1,     1,
    E_RQ0028_CURSOR_FETCH_FAILED,   E_QE0528_CURSOR_FETCH_FAILED,   -1,     1,
    E_RQ0029_CURSOR_EXEC_FAILED,    E_QE0529_CURSOR_EXEC_FAILED,    -1,     1,
    E_RQ0030_CURSOR_DELETE_FAILED,  E_QE0530_CURSOR_DELETE_FAILED,  -1,     1,
    E_RQ0031_INVALID_CONTINUE,      E_QE0531_INVALID_CONTINUE,      -1,     1,
    E_RQ0032_DIFFERENT_TUPLE_SIZE,  E_QE0532_DIFFERENT_TUPLE_SIZE,  -1,     1,
    E_RQ0033_FETCH_FAILED,          E_QE0533_FETCH_FAILED,          -1,     1,
    E_RQ0034_COPY_CREATE_FAILED,    E_QE0534_COPY_CREATE_FAILED,    -1,     1,
    E_RQ0035_BAD_COL_DESC_FORMAT,   E_QE0535_BAD_COL_DESC_FORMAT,   -1,     1,
    E_RQ0036_COL_DESC_EXPECTED,     E_QE0536_COL_DESC_EXPECTED,     -1,     1,
    E_RQ0037_II_LDB_NOT_DEFINED,    E_QE0537_II_LDB_NOT_DEFINED,    -1,     1,
    E_RQ0038_ULM_ALLOC_FAILED,      E_QE0538_ULM_ALLOC_FAILED,      -1,     0,
    E_RQ0039_INTERRUPTED,           E_QE0022_QUERY_ABORTED,         4708,   0,
    E_RQ0040_UNKNOWN_REPEAT_Q,      E_QE0540_UNKNOWN_REPEAT_Q,      -1,     1,
    E_RQ0041_ERROR_MSG_FROM_LDB,    E_QE0541_ERROR_MSG_FROM_LDB,    -1,     0,
    E_RQ0042_LDB_ERROR_MSG,         E_QE0542_LDB_ERROR_MSG,         -1,     0,
    E_RQ0043_CURSOR_UPDATE_FAILED,  E_QE0543_CURSOR_UPDATE_FAILED,  -1,     1,
    E_RQ0044_CURSOR_OPEN_FAILED,    E_QE0544_CURSOR_OPEN_FAILED,    -1,     1,
    E_RQ0045_CONNECTION_LOST,       E_QE0545_CONNECTION_LOST,       -1,     1,
    E_RQ0046_RECV_TIMEOUT,          E_QE0546_RECV_TIMEOUT,          -1,     1,
    E_RQ0047_UNAUTHORIZED_USER,     E_QE0547_UNAUTHORIZED_USER,     -1,     0,
    E_RQ0048_SECURE_FAILED,         E_QE0548_SECURE_FAILED,         -1,     1,
    E_RQ0049_RESTART_FAILED,        E_QE0549_RESTART_FAILED,        -1,     0,
    E_QE0500_RQF_GENERIC_ERR,       E_QE0500_RQF_GENERIC_ERR,       -1,     1,
/*  TPF error codes start here */
    0L,                             E_QE0700_TPF_GENERIC_ERR,       -1,     1,
    E_TP0001_INVALID_REQUEST,       E_QE0701_INVALID_REQUEST,       -1,     1,
    E_TP0002_SCF_MALLOC_FAILED,     E_QE0702_SCF_MALLOC_FAILED,     -1,     1,
    E_TP0003_SCF_MFREE_FAILED,      E_QE0703_SCF_MFREE_FAILED,      -1,     1,
    E_TP0004_MULTI_SITE_WRITE,      E_QE0704_MULTI_SITE_WRITE,      -1,     1,
    E_TP0005_UNKNOWN_STATE,         E_QE0705_UNKNOWN_STATE,         -1,     1,
    E_TP0006_INVALID_EVENT,         E_QE0706_INVALID_EVENT,         -1,     1,
    E_TP0007_INVALID_TRANSITION,    E_QE0707_INVALID_TRANSITION,    -1,     1,
    E_TP0008_TXN_BEGIN_FAILED,      E_QE0708_TXN_BEGIN_FAILED,      -1,     1,
    E_TP0009_TXN_FAILED,            E_QE0709_TXN_FAILED,            -1,     1,
    E_TP0010_SAVEPOINT_FAILED,      E_QE0710_SAVEPOINT_FAILED,      -1,     1,
    E_TP0011_SP_ABORT_FAILED,       E_QE0711_SP_ABORT_FAILED,       -1,     1,
    E_TP0012_SP_NOT_EXIST,          E_QE0712_SP_NOT_EXIST,          -1,     1,
    E_TP0013_AUTO_ON_NO_SP,         E_QE0713_AUTO_ON_NO_SP,         -1,     1,
    E_TP0014_ULM_STARTUP_FAILED,    E_QE0714_ULM_STARTUP_FAILED,    -1,     1,
    E_TP0015_ULM_OPEN_FAILED,       E_QE0715_ULM_OPEN_FAILED,       -1,     1,
    E_TP0016_ULM_ALLOC_FAILED,      E_QE0716_ULM_ALLOC_FAILED,      -1,     1,
    E_TP0017_ULM_CLOSE_FAILED,      E_QE0717_ULM_CLOSE_FAILED,      -1,     1,
    E_TP0020_INTERNAL,              E_QE0720_TPF_INTERNAL,          -1,     0,
    E_TP0021_LOGDX_FAILED,          E_QE0721_LOGDX_FAILED,          -1,     1,
    E_TP0022_SECURE_FAILED,         E_QE0722_SECURE_FAILED,         -1,     1,
    E_TP0023_COMMIT_FAILED,         E_QE0723_COMMIT_FAILED,         -1,     1,
    E_TP0024_DDB_IN_RECOVERY,       E_QE0724_DDB_IN_RECOVERY,       -1,     1,
    E_QE0700_TPF_GENERIC_ERR,       E_QE0700_TPF_GENERIC_ERR,       -1,     1,
/*  QEF/STAR error codes start here */
    0L,                             E_QE0902_NO_LDB_TAB_ON_CRE_LINK,-1,     1,
    0L,                             E_QE0950_INVALID_DROP_LINK,     -1,     1,
    0L,                             E_QE0960_CONNECT_ON_CONNECT,    -1,     1,
    0L,                             E_QE0961_CONNECT_ON_XACTION,    -1,     1,
    0L,                             E_QE0962_DDL_CC_ON_XACTION,     -1,     0,
    0L,                             E_QE096E_INVALID_DISCONNECT,    -1,     1,
    0L,                             E_QE0970_BAD_USER_NAME,         -1,     1,
    0L,                             E_QE0980_LDB_REPORTED_ERROR,    -1,     1,
    0L,                             E_QE0990_2PC_BAD_LDB_MIX,       -1,     1,
/*  QEF/STAR error codes end here */
    0L,				    E_QE1001_NO_ACTIONS_IN_QP,	    -1,	    1,
    0L,				    E_QE1002_BAD_ACTION_COUNT,	    -1,	    1,
    0L,				    E_QE1003_NO_VALID_LIST,	    -1,	    1,
    0L,				    E_QE1004_TABLE_NOT_OPENED,	    -1,	    1,
    0L,				    E_QE1005_WRONG_JOIN_NODE,	    -1,	    1,
    0L,				    E_QE1006_BAD_SORT_DESCRIPTOR,   -1,	    1,
    0L,				    E_QE1007_BAD_SORT_COUNT,	    -1,	    1,
    0L,				    E_QE1009_DSH_NOTMATCH_QP,	    -1,	    1,
    0L,				    E_QE1010_HOLDING_SORT_BUFFER,    0,	    1,
    0L,                             I_QE2037_CONSTRAINT_TAB_IDX_ERR, 9360,  1,
    0L,                             I_QE2038_CONSTRAINT_TAB_IDX_ERR, 9361,  1,
/*
** SXF errors 
*/
    E_SX0017_SXAS_ALTER      ,	    -1,				    -1,     0,
    E_SX002B_AUDIT_NOT_ACTIVE,	    -1,				    -1,     0,

    0L,				    0L,				     0,	    0
};
