/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: iiapi.h
**
** Description:
**
**      This file contains the input parameters and prototypes for all
**      Ingres OpenAPI functions.
**
** API Version History:
**
**	IIAPI_VERSION_1
**	    Initial Version.
**
**	IIAPI_VERSION_2
**	    IIapi_initialize() returns environment handle.
**	    IIapi_releaseEnv() frees environment handle resources.
**	    IIapi_setConnectParam() accepts environment handle.
**	    IIapi_connect() accepts environment handle.
**	    IIapi_connect() connection type parameter.
**
**	IIAPI_VERSION_3
**	    UCS-2 character data types.
**	    IIAPI_CP_LOGIN_LOCAL connection parameter.	
**
**	IIAPI_VERSION_4
**	    Eight-byte integer data type.
**	    IIAPI_EP_INT8_WIDTH environment parameter.
**	    IIAPI_CP_INT8_WIDTH connection parameter.
**
**	IIAPI_VERSION_5
**	    ANSI date/time data types.  
**	    IIAPI_COL_PROC{IN,OUT,INOUT}PARM descriptor column types.
**	    XA transactions: IIapi_xaStart(), IIapi_xaEnd(), 
**	    IIapi_xaPrepare(), IIapi_xaCommit(), IIapi_xaRollback().
**
**	IIAPI_VERSION_6
**	    Blob/Clob locator data types.
**	    IIapi_query() accepts query flags.
**	    Scrollable cursors: IIapi_position(), IIapi_scroll().
**	    IIAPI_EP_DATE_ALIAS environment parameter.
**	    IIAPI_CP_DATE_ALIAS connection parameter.
**
**	IIAPI_VERSION_7
**	    Boolean data type.
**	    Batch processing: IIapi_batch().
**	    Positional (unnamed) DB procedure parameeters.
**
**	IIAPI_VERSION_8
**	    Spatial data types.
##
## History:
##      01-sep-93 (ctham)
##          creation
##      11-apr-94 (ctham)
##          change data type of IIAPI_CATCHEVENTPARM's ce_eventTime to
##          II_LONG to make it more portable.
##          change sizeAdvise of IIAPI_CONNPARM to co_sizeAdvise to
##          adhere with the INGRES coding convention.
##      26-apr-94 (ctham)
##          add IIAPI_DATAVALUE to describe data value.
##          add nullable flags to IIAPI_DESCRIPTOR and IIAPI_SDDATA.
##      11-may-94 (ctham)
##          creates API C data types to replace INGRES C data types.
##	 31-may-94 (ctham)
##	     added prototypes for catalog access functions.  Added
##	     members to IIAPI_CATCHEVENTPARM.
##	14-Nov-94 (gordy)
##	    Removed catalog access functions and added connection
##	    parameter functions.  Removed IIAPI_SESSIONINFO structure
##	    and replaced with the connection parameters.
##	17-Nov-94 (gordy)
##	    Dropped IIapi_decodeNativeError() since can't get text
##	    for non-Ingres errors.
##	18-Nov-94 (gordy)
##	    Added defines for API level and added version info to
##	    IIapi_initialize() parameters.  Added member to connection
##	    parameter block for restarting transactions and dropped
##	    the associated connection parameter to IIapi_setConnectParam().
##	21-Nov-94 (gordy)
##	    Added gq_readonly to IIAPI_GETQINFOPARM.
##	10-Jan-95 (gordy)
##	    Added gq_procedureHandle to IIAPI_GETQINFOPARM.
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
##	 9-Feb-95 (gordy)
##	    Added IIapi_convertData().
##	13-Feb-95 (gordy)
##	    Added gq_flags and two new error messages.
##	16-Feb-95 (gordy)
##	    Added qy_parameters.  Added E_AP000D_PARAMETERS_REQUIRED.
##	22-Feb-95 (gordy)
##	    Standardized IIapi_convertData().
##	 3-Mar-95 (gordy)
##	    Fix platform dependent macros to be more universal.
##	 6-Mar-95 (gordy)
##	    Standardized on transaction ID rather than transaction name.
##	10-Mar-95 (gordy)
##	    Removed platform dependent definitions to iiapidep.h.
##	24-Mar-95 (gordy)
##	    Callback routines must use the II_CALLBACK symbol.
##	30-Mar-95 (gordy)
##	    Fixed E_AP_MASK definition which was losing upper 16 bits
##	    on 16 bit platform.
##	25-Apr-95 (gordy)
##	    Cleaned up Database Events.
##	10-May-95 (gordy)
##	    Cleanup work on connection parms.
##	16-May-95 (gordy)
##	    Fixed history comments so can be removed from deliverable.
##	19-May-95 (gordy)
##	    Fixed include statements.
##	13-Jun-95 (gordy)
##	    Added E_AP0016_ROW_DISCARDED and E_AP0017_SEGMENT_DISCARDED.
##	28-Jul-95 (gordy)
##	    Used fixed length types.  Mose counters should be II_INT2.
##	16-Feb-96 (gordy)
##	    Remove IIAPI_SDDATA (replaced by IIAPI_DESCRIPTOR, IIAPI_DATAVALUE).
##	23-Feb-96 (gordy)
##	    Removed IIAPI_COPYINFO and cleaned up IIAPI_COPYMAP and
##	    IIAPI_FDATADESCR to properly represent copy information.
##	15-Mar-96 (gordy)
##	    Make data length values unsigned to handle large BLOB segments.
##	22-Apr-96 (gordy)
##	    Finished support for COPY FROM.
##	 9-Jul-96 (gordy)
##	    Added IIapi_autocommit() and IIAPI_AUTOPARM.
##	12-Jul-96 (gordy)
##	    Added IIapi_getEvent() and IIAPI_GETEVENTPARM.
##	 9-Sep-96 (gordy)
##	    Added IIAPI_CP_DBMS_PASSWORD, IIAPI_CP_CENTURY_BOUNDARY
##	 7-Feb-97 (gordy)
##	    API Version 2: environment handles, connection types.
##	05-May-1998 (somsa01)
##	    Added define for MULTINATIONAL4 for OpenAPI.
##	15-Sep-98 (rajus01)
##	    Added E_AP0018_INVALID_DISCONNECT definition.
##	    Added IIapi_formatData(), IIapi_abort(), IIapi_setEnvParam()
##	    Added IIAPI_FORMATPARM, IIAPI_ABORTPARM, IIAPI_SETENVPRMPARM,
##	    IIAPI_TRACEPARM.
##	06-Oct-98 (rajus01)
##	    Added support for GCA_PROMPT.
##      03-Feb-99 (rajus01)
##          Added IIAPI_EP_EVENT_FUNC. Renamed IIAPI_EP_CAN_PROMPT to
##          IIAPI_EP_PROMPT_FUNC. Added IIAPI_EVENTPARM. 
##	11-Feb-99 (rajus01)
##	    Removed II_FAR from IIAPI_EVENTPARM structure definition.
##      23-May-00 (loera01) Bug 101463
##          Added support for passing global temporary tables argument 
## 	    into a DB procedure (GCA1_INVPROC message).
##	 1-Mar-01 (gordy)
##	    Added new datatypes IIAPI_NCHA_TYPE, IIAPI_NVCH_TYPE and
##	    IIAPI_LNVCH_TYPE.  Added IIAPI_VERSION_3 and IIAPI_LEVEL_2.
##      19-Aug-2002(wansh01)
##          Added support for IIAPI_CP_LOGIN_LOCAL as a connection parameter 
##	    for IIAPI_VERSION_3. 
##	 6-Jun-03 (gordy)
##	    Add definition for API MIB values.
##      02-Jan-04 (lunbr01) Bug 111540, Problem EDDB2#12
##          Added API definition for SQLState 23501 - UNIQUE_CONS_VIOLATION.
##	15-Mar-04 (gordy)
##	    Added IIAPI_VERSION_4 and IIAPI_LEVEL_3 for eight-byte integers,
##	    Env/Conn parameters IIAPI_EP_INT8_WIDTH and IIAPI_CP_INT8_WIDTH.
##	22-Oct-04 (gordy)
##	    Added descriptor specific error codes.
##	31-May-06 (gordy)
##	    Added IIAPI_VERSION_5 and IIAPI_LEVEL_4 for ANSI date/time.
##	28-Jun-06 (gordy)
##	    Added IIAPI_COL_PROC{IN,OUT,INOUT}PARM column types.
##	 7-Jul-06 (gordy)
##	    Added XA support functions.
##	 6-Oct-06 (gordy)
##	    Added defines for data type lengths.
##	24-Oct-06 (gordy)
##	    Added IIAPI_VERSION_6, IIAPI_LEVEL_5, and E_AP002C_LVL5_DATA_TYPE
##	    for Blob/Clob locators.  IIapi_query() now accepts query flags.
##	15-Mar-07 (gordy)
##	    Added support for scrollable cursors.
##	18-Jul-07 (gordy)
##	    Added environment and connection parameters for date alias:
##	    IIAPI_EP_DATE_ALIAS, IIAPI_CP_DATE_ALIAS.
##	11-Aug-08 (gordy)
##	    Added IIapi_getColumnInfo() and IIAPI_GETCOLINFOPARM.
##	 5-Mar-09 (gordy)
##	    Added E_AP0023_INVALID_NULL_DATA.
##	26-Oct-09 (gordy)
##	    Added IIAPI_VERSION_7, IIAPI_LEVEL_6, and E_AP002D_LVL6_DATA_TYPE
##	    for Boolean type.
##      16-Feb-10 (thich01)
##	    Added IIAPI_VERSION_8, IIAPI_LEVEL_7, and E_AP002E_LVL7_DATA_TYPE
##	    for Spatial types.
##	25-Mar-10 (gordy)
##	    Added IIapi_batch() function and parameter structure.
##	15-Apr-10 (gordy)
##	    Noted that positional database procedure parameters are
##	    are supported at IIAPI_VERSION_7/IIAPI_LEVEL_6.
##      17-Aug-2010 (thich01)
##          Added IIAPI_NBR_TYPE and II_API_GEOMC_TYPE
##      19-Aug-2010 (thich01)
**          Left a space at 60 for GCA SECL TYPE.
*/

# ifndef __IIAPI_H__
# define __IIAPI_H__


/*
** Platform Dependencies
**
** Platform dependent definitions are defined in a seperate header
** file, unique for each platform.  The following definitions must
** must be provided:
**
**	II_BOOL		boolean
**	II_CHAR		signed character
**	II_INT		signed integer
**	II_INT1		 8-bit, signed integer
**	II_INT2		16-bit, signed integer
**	II_INT4		32-bit, signed integer
**	II_INT8		64-bit, signed integer
**	II_LONG		32-bit, signed integer
**	II_UCHAR	unsigned character
**	II_UINT1	 8-bit, unsigned integer
**	II_UINT2	16-bit, unsigned integer
**	II_UINT4	32-bit, unsigned integer
**	II_UINT8	64-bit, unsigned integer
**	II_ULONG	32-bit, unsigned integer
**	II_FLOAT4	32-bit, signed float
**	II_FLOAT8	64-bit, signed float
**	II_PTR		void far pointer
**	II_VOID		void
**
**	II_FAR		long address
**	II_EXTERN	externally referenced functions
**	II_EXPORT	library entry points
**	II_CALLBACK	callback routines
*/

# include <iiapidep.h>


/*
** Common C Macros
*/

# ifndef NULL
# define NULL 0
# endif

# ifdef FALSE
# undef FALSE
# endif

# ifdef TRUE
# undef TRUE
# endif

# define TRUE  ((II_BOOL)1)
# define FALSE ((II_BOOL)0)



/*
** Name: II_SQLSTATE - API Function Return SQLSTATE
**
** Description:
**      This data type defines the SQLSTATE values returned
**	by IIapi_getErrorInfo().  These values should be kept
**	consistent with the definitions is sqlstate.h.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Dropped API SQLSTATEs.  Use only character representation.
*/

# define II_SQLSTATE_LEN	5

# define II_SS00000_SUCCESS 		        "00000"
# define II_SS01000_WARNING 		        "01000"
# define II_SS01001_CURS_OPER_CONFLICT 		"01001"
# define II_SS01002_DISCONNECT_ERROR 		"01002"
# define II_SS01003_NULL_ELIM_IN_SETFUNC	"01003"
# define II_SS01004_STRING_RIGHT_TRUNC 		"01004"
# define II_SS01005_INSUF_DESCR_AREAS 		"01005"
# define II_SS01006_PRIV_NOT_REVOKED 		"01006"
# define II_SS01007_PRIV_NOT_GRANTED 		"01007"
# define II_SS01008_IMP_ZERO_BIT_PADDING	"01008"
# define II_SS01009_SEARCH_COND_TOO_LONG	"01009"
# define II_SS0100A_QRY_EXPR_TOO_LONG 		"0100A"
# define II_SS01500_LDB_TBL_NOT_DROPPED 	"01500"
# define II_SS01501_NO_WHERE_CLAUSE 		"01501"
# define II_SS02000_NO_DATA 		        "02000"
# define II_SS07000_DSQL_ERROR 		        "07000"
# define II_SS07001_USING_PARM_MISMATCH 	"07001"
# define II_SS07002_USING_TARG_MISMATCH 	"07002"
# define II_SS07003_CAN_EXEC_CURS_SPEC 		"07003"
# define II_SS07004_NEED_USING_FOR_PARMS	"07004"
# define II_SS07005_STMT_NOT_CURS_SPEC 		"07005"
# define II_SS07006_RESTR_DT_ATTR_ERR 		"07006"
# define II_SS07007_NEED_USING_FOR_RES 		"07007"
# define II_SS07008_INV_DESCR_CNT 	        "07008"
# define II_SS07009_INV_DESCR_IDX 	        "07009"
# define II_SS07500_CONTEXT_MISMATCH 		"07500"
# define II_SS08000_CONNECTION_EXCEPTION       	"08000"
# define II_SS08001_CANT_GET_CONNECTION        	"08001"
# define II_SS08002_CONNECT_NAME_IN_USE        	"08002"
# define II_SS08003_NO_CONNECTION               "08003"
# define II_SS08004_CONNECTION_REJECTED        	"08004"
# define II_SS08006_CONNECTION_FAILURE 		"08006"
# define II_SS08007_XACT_RES_UNKNOWN 		"08007"
# define II_SS08500_LDB_UNAVAILABLE 		"08500"
# define II_SS0A000_FEATUR_NOT_SUPPORTED	"0A000"
# define II_SS0A001_MULT_SERVER_XACTS 		"0A001"
# define II_SS0A500_INVALID_QRY_LANG 		"0A500"
# define II_SS21000_CARD_VIOLATION 		"21000"
# define II_SS22000_DATA_EXCEPTION 		"22000"
# define II_SS22001_STRING_RIGHT_TRUNC 		"22001"
# define II_SS22002_NULLVAL_NO_IND_PARAM       	"22002"
# define II_SS22003_NUM_VAL_OUT_OF_RANGE       	"22003"
# define II_SS22005_ASSIGNMENT_ERROR 		"22005"
# define II_SS22007_INV_DATETIME_FMT 		"22007"
# define II_SS22008_DATETIME_FLD_OVFLOW        	"22008"
# define II_SS22009_INV_TZ_DISPL_VAL 		"22009"
# define II_SS22011_SUBSTRING_ERROR 		"22011"
# define II_SS22012_DIVISION_BY_ZERO 		"22012"
# define II_SS22015_INTERVAL_FLD_OVFLOW        	"22015"
# define II_SS22018_INV_VAL_FOR_CAST 		"22018"
# define II_SS22019_INV_ESCAPE_CHAR 		"22019"
# define II_SS22021_CHAR_NOT_IN_RPRTR 		"22021"
# define II_SS22022_INDICATOR_OVFLOW 		"22022"
# define II_SS22023_INV_PARAM_VAL               "22023"
# define II_SS22024_UNTERM_C_STRING 		"22024"
# define II_SS22025_INV_ESCAPE_SEQ 		"22025"
# define II_SS22026_STRING_LEN_MISMATCH        	"22026"
# define II_SS22027_TRIM_ERROR 		        "22027"
# define II_SS22500_INVALID_DATA_TYPE 		"22500"
# define II_SS23000_CONSTR_VIOLATION 		"23000"
# define II_SS23501_UNIQUE_CONS_VIOLATION	"23501"
# define II_SS24000_INV_CURS_STATE 		"24000"
# define II_SS25000_INV_XACT_STATE 		"25000"
# define II_SS26000_INV_SQL_STMT_NAME 		"26000"
# define II_SS27000_TRIG_DATA_CHNG_ERR 		"27000"
# define II_SS28000_INV_AUTH_SPEC 	        "28000"
# define II_SS2A500_TBL_NOT_FOUND 	        "2A500"
# define II_SS2A501_COL_NOT_FOUND 	        "2A501"
# define II_SS2A502_DUPL_OBJECT 	        "2A502"
# define II_SS2A503_INSUF_PRIV 		        "2A503"
# define II_SS2A504_UNKNOWN_CURSOR 		"2A504"
# define II_SS2A505_OBJ_NOT_FOUND 	        "2A505"
# define II_SS2A506_INVALID_IDENTIFIER 		"2A506"
# define II_SS2A507_RESERVED_IDENTIFIER 	"2A507"
# define II_SS2B000_DEP_PRIV_EXISTS 		"2B000"
# define II_SS2C000_INV_CH_SET_NAME 		"2C000"
# define II_SS2D000_INV_XACT_TERMINATION	"2D000"
# define II_SS2E000_INV_CONN_NAME 	        "2E000"
# define II_SS33000_INV_SQL_DESCR_NAME 		"33000"
# define II_SS34000_INV_CURS_NAME 	        "34000"
# define II_SS35000_INV_COND_NUM 	        "35000"
# define II_SS37500_TBL_NOT_FOUND 	        "37500"
# define II_SS37501_COL_NOT_FOUND 	        "37501"
# define II_SS37502_DUPL_OBJECT 	        "37502"
# define II_SS37503_INSUF_PRIV 		        "37503"
# define II_SS37504_UNKNOWN_CURSOR 		"37504"
# define II_SS37505_OBJ_NOT_FOUND 	        "37505"
# define II_SS37506_INVALID_IDENTIFIER 		"37506"
# define II_SS37507_RESERVED_IDENTIFIER 	"37507"
# define II_SS3C000_AMBIG_CURS_NAME 		"3C000"
# define II_SS3D000_INV_CAT_NAME 	        "3D000"
# define II_SS3F000_INV_SCHEMA_NAME 		"3F000"
# define II_SS40000_XACT_ROLLBACK 	        "40000"
# define II_SS40001_SERIALIZATION_FAIL 		"40001"
# define II_SS40002_CONSTR_VIOLATION 		"40002"
# define II_SS40003_STMT_COMPL_UNKNOWN 		"40003"
# define II_SS42000_SYN_OR_ACCESSERR 		"42000"
# define II_SS42500_TBL_NOT_FOUND 	        "42500"
# define II_SS42501_COL_NOT_FOUND 	        "42501"
# define II_SS42502_DUPL_OBJECT 	        "42502"
# define II_SS42503_INSUF_PRIV 		        "42503"
# define II_SS42504_UNKNOWN_CURSOR 		"42504"
# define II_SS42505_OBJ_NOT_FOUND 	        "42505"
# define II_SS42506_INVALID_IDENTIFIER 		"42506"
# define II_SS42507_RESERVED_IDENTIFIER 	"42507"
# define II_SS44000_CHECK_OPTION_ERR 		"44000"
# define II_SS50000_MISC_ING_ERRORS 		"50000"
# define II_SS50001_INVALID_DUP_ROW 		"50001"
# define II_SS50002_LIMIT_EXCEEDED 		"50002"
# define II_SS50003_EXHAUSTED_RESOURCE 		"50003"
# define II_SS50004_SYS_CONFIG_ERROR 		"50004"
# define II_SS50005_GW_ERROR 		        "50005"
# define II_SS50006_FATAL_ERROR 	        "50006"
# define II_SS50007_INVALID_SQL_STMT_ID 	"50007"
# define II_SS50008_UNSUPPORTED_STMT 		"50008"
# define II_SS50009_ERROR_RAISED_IN_DBPROC 	"50009"
# define II_SS5000A_QUERY_ERROR 	        "5000A"
# define II_SS5000B_INTERNAL_ERROR 		"5000B"
# define II_SS5000C_FETCH_ORIENTATION 		"5000C"
# define II_SS5000D_INVALID_CURSOR_NAME 	"5000D"
# define II_SS5000E_DUP_STMT_ID 	        "5000E"
# define II_SS5000F_TEXTUAL_INFO 	        "5000F"
# define II_SS5000G_DBPROC_MESSAGE 		"5000G"
# define II_SS5000H_UNAVAILABLE_RESOURCE	"5000H"
# define II_SS5000I_UNEXP_LDB_SCHEMA_CHNG      	"5000I"
# define II_SS5000J_INCONSISTENT_DBMS_CAT 	"5000J"
# define II_SS5000K_SQLSTATE_UNAVAILABLE 	"5000K"
# define II_SS5000L_PROTOCOL_ERROR 		"5000L"
# define II_SS5000M_IPC_ERROR 		        "5000M"
# define II_SS5000N_OPERAND_TYPE_MISMATCH	"5000N"
# define II_SS5000O_INVALID_FUNC_ARG_TYPE	"5000O"
# define II_SS5000P_TIMEOUT_ON_LOCK_REQUEST	"5000P"
# define II_SS5000Q_DB_REORG_INVALIDATED_QP	"5000Q"
# define II_SS5000R_RUN_TIME_LOGICAL_ERROR	"5000R"
# define II_SSHZ000_RDA 		        "HZ000"


/*
** Name: API Error Codes
**
** Description:
**	Errors codes for error detectected by the API.
##
## History:
##	23-Jan-95 (gordy)
##	    Created.
##	13-Feb-95 (gordy)
##	    Added E_AP0014_INVALID_REPEAT_ID and E_AP0015_INVALID_TRANS_STMT.
##	16-Feb-95 (gordy)
##	    Added E_AP000D_PARAMETERS_REQUIRED.
##	13-Jun-95 (gordy)
##	    Added E_AP0016_ROW_DISCARDED and E_AP0017_SEGMENT_DISCARDED.
##	22-Oct-04 (gordy)
##	    Added AP0020, AP0021, AP0024, AP0025, AP0028, AP0029, AP002A.
##	31-May-06 (gordy)
##	    Added AP002B for ANSI date/time datatypes.
##	24-Oct-06 (gordy)
##	    Added AP002C for Blob/Clob locator datatypes.
##	 5-Mar-09 (gordy)
##	    Added E_AP0023_INVALID_NULL_DATA.
##	25-Mar-10 (gordy)
##	    Added E_AP001F_BATCH_UNSUPPORTED.
*/

# define E_AP_MASK	(201 * 0x10000)

# define E_AP0000_OK			(II_ULONG)(0x0000)
# define E_AP0001_CONNECTION_ABORTED	(II_ULONG)(E_AP_MASK + 0x0001)
# define E_AP0002_TRANSACTION_ABORTED	(II_ULONG)(E_AP_MASK + 0x0002)
# define E_AP0003_ACTIVE_TRANSACTIONS	(II_ULONG)(E_AP_MASK + 0x0003)
# define E_AP0004_ACTIVE_QUERIES	(II_ULONG)(E_AP_MASK + 0x0004)
# define E_AP0005_ACTIVE_EVENTS		(II_ULONG)(E_AP_MASK + 0x0005)
# define E_AP0006_INVALID_SEQUENCE	(II_ULONG)(E_AP_MASK + 0x0006)
# define E_AP0007_INCOMPLETE_QUERY	(II_ULONG)(E_AP_MASK + 0x0007)
# define E_AP0008_QUERY_DONE		(II_ULONG)(E_AP_MASK + 0x0008)
# define E_AP0009_QUERY_CANCELLED	(II_ULONG)(E_AP_MASK + 0x0009)
# define E_AP000A_QUERY_INTERRUPTED	(II_ULONG)(E_AP_MASK + 0x000A)
# define E_AP000B_COMMIT_FAILED		(II_ULONG)(E_AP_MASK + 0x000B)
# define E_AP000C_2PC_REFUSED		(II_ULONG)(E_AP_MASK + 0x000C)
# define E_AP000D_PARAMETERS_REQUIRED	(II_ULONG)(E_AP_MASK + 0x000D)
# define E_AP000E_INVALID_CONNECT_PARM	(II_ULONG)(E_AP_MASK + 0x000E)
# define E_AP000F_NO_CONNECT_PARMS	(II_ULONG)(E_AP_MASK + 0x000F)
# define E_AP0010_INVALID_COLUMN_COUNT	(II_ULONG)(E_AP_MASK + 0x0010)
# define E_AP0011_INVALID_PARAM_VALUE	(II_ULONG)(E_AP_MASK + 0x0011)
# define E_AP0012_INVALID_DESCR_INFO	(II_ULONG)(E_AP_MASK + 0x0012)
# define E_AP0013_INVALID_POINTER	(II_ULONG)(E_AP_MASK + 0x0013)
# define E_AP0014_INVALID_REPEAT_ID	(II_ULONG)(E_AP_MASK + 0x0014)
# define E_AP0015_INVALID_TRANS_STMT	(II_ULONG)(E_AP_MASK + 0x0015)
# define E_AP0016_ROW_DISCARDED		(II_ULONG)(E_AP_MASK + 0x0016)
# define E_AP0017_SEGMENT_DISCARDED	(II_ULONG)(E_AP_MASK + 0x0017)
# define E_AP0018_INVALID_DISCONNECT	(II_ULONG)(E_AP_MASK + 0x0018)
# define E_AP001F_BATCH_UNSUPPORTED	(II_ULONG)(E_AP_MASK + 0x001F)
# define E_AP0020_BYREF_UNSUPPORTED	(II_ULONG)(E_AP_MASK + 0x0020)
# define E_AP0021_GTT_UNSUPPORTED	(II_ULONG)(E_AP_MASK + 0x0021)
# define E_AP0022_XA_UNSUPPORTED	(II_ULONG)(E_AP_MASK + 0x0022)
# define E_AP0023_INVALID_NULL_DATA	(II_ULONG)(E_AP_MASK + 0x0023)
# define E_AP0024_INVALID_DATA_SIZE	(II_ULONG)(E_AP_MASK + 0x0024)
# define E_AP0025_SVC_DATA_TYPE		(II_ULONG)(E_AP_MASK + 0x0025)
# define E_AP0028_LVL1_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x0028)
# define E_AP0029_LVL2_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x0029)
# define E_AP002A_LVL3_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x002A)
# define E_AP002B_LVL4_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x002B)
# define E_AP002C_LVL5_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x002C)
# define E_AP002D_LVL6_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x002D)
# define E_AP002E_LVL7_DATA_TYPE	(II_ULONG)(E_AP_MASK + 0x002E)


/*
** Name: XA Error Codes
**
** Description:
**	Errors codes for XA operations.
##
## History:
##	11-Jul-06 (gordy)
##	    Created.
*/

# define IIAPI_XAER_ASYNC	-2
# define IIAPI_XAER_RMERR	-3
# define IIAPI_XAER_NOTA	-4
# define IIAPI_XAER_INVAL	-5
# define IIAPI_XAER_PROTO	-6
# define IIAPI_XAER_RMFAIL	-7
# define IIAPI_XAER_DUPID	-8
# define IIAPI_XAER_OUTSIDE	-9

# define IIAPI_XA_RDONLY	3
# define IIAPI_XA_RETRY		4
# define IIAPI_XA_HEURMIX	5
# define IIAPI_XA_HEURRB	6
# define IIAPI_XA_HEURCOM	7
# define IIAPI_XA_HEURHAZ	8
# define IIAPI_XA_NOMIGRATE	9

# define IIAPI_XA_RBROLLBACK	100
# define IIAPI_XA_RBCOMMFAIL	101
# define IIAPI_XA_RBDEADLOCK	102
# define IIAPI_XA_RBINTEGRITY	103
# define IIAPI_XA_RBOTHER	104
# define IIAPI_XA_RBPROTO	105
# define IIAPI_XA_RBTIMEOUT	106
# define IIAPI_XA_RBTRANSIENT	107


/*
** API MIB class definitions
*/

# define IIAPI_MIB_TRACE_LEVEL		"exp.aif.api.trace_level"
# define IIAPI_MIB_CONN			"exp.aif.api.conn"
# define IIAPI_MIB_CONN_TARGET		"exp.aif.api.conn.target"
# define IIAPI_MIB_CONN_USERID		"exp.aif.api.conn.userid"
# define IIAPI_MIB_CONN_GCA_ASSOC	"exp.aif.api.conn.gca_assoc"


/*
** Name: API Data Types
**
** Description:
**      The following defines the API data types.  
##	The values defined for this data type must kept consistent 
##	with the corresponding values defined for DB_DT_ID of iicommon.h.
##
## History:
##      11-may-94 (ctham)
##          creation
##	31-May-06 (gordy)
##	    Added ANSI date/time types.
##	 6-Oct-06 (gordy)
##	    Added data type lengths.
##	24-Oct-06 (gordy)
##	    Added Blob/Clob locator types.
##      16-Fev-10 (thich01)
##          Added spatial types.
*/

typedef II_INT2 IIAPI_DT_ID;

# define IIAPI_HNDL_TYPE    ((IIAPI_DT_ID) -1)	/* API Handle */
# define IIAPI_CHR_TYPE     ((IIAPI_DT_ID) 32)	/* Text Fixed */
# define IIAPI_CHA_TYPE	    ((IIAPI_DT_ID) 20)	/* Character Fixed */
# define IIAPI_VCH_TYPE	    ((IIAPI_DT_ID) 21)	/* Character Variable */
# define IIAPI_LVCH_TYPE    ((IIAPI_DT_ID) 22)	/* Character Long */
# define IIAPI_LCLOC_TYPE   ((IIAPI_DT_ID) 36)  /* Character Long Locator */
# define IIAPI_BYTE_TYPE    ((IIAPI_DT_ID) 23)	/* Binary Fixed */
# define IIAPI_VBYTE_TYPE   ((IIAPI_DT_ID) 24)	/* Binary Variable */
# define IIAPI_LBYTE_TYPE   ((IIAPI_DT_ID) 25)	/* Binary Long */
# define IIAPI_LBLOC_TYPE   ((IIAPI_DT_ID) 35)  /* Binary Long Locator */
# define IIAPI_NCHA_TYPE    ((IIAPI_DT_ID) 26)	/* Unicode Fixed */
# define IIAPI_NVCH_TYPE    ((IIAPI_DT_ID) 27)	/* Unicode Variable */
# define IIAPI_LNVCH_TYPE   ((IIAPI_DT_ID) 28)	/* Unicode Long */
# define IIAPI_LNLOC_TYPE   ((IIAPI_DT_ID) 29)  /* Unicode Long Locator */
# define IIAPI_TXT_TYPE     ((IIAPI_DT_ID) 37)	/* Text Variable */
# define IIAPI_LTXT_TYPE    ((IIAPI_DT_ID) 41)	/* Text Var (typeless NULL) */
# define IIAPI_INT_TYPE     ((IIAPI_DT_ID) 30)	/* Integer */
# define IIAPI_FLT_TYPE     ((IIAPI_DT_ID) 31)	/* Floating Point */
# define IIAPI_MNY_TYPE     ((IIAPI_DT_ID)  5)	/* Money */
# define IIAPI_DEC_TYPE	    ((IIAPI_DT_ID) 10)	/* Decimal */
# define IIAPI_BOOL_TYPE    ((IIAPI_DT_ID) 38)	/* Boolean */
# define IIAPI_DTE_TYPE     ((IIAPI_DT_ID)  3)	/* Ingres Date */
# define IIAPI_DATE_TYPE    ((IIAPI_DT_ID)  4)	/* ANSI Date */
# define IIAPI_TIME_TYPE    ((IIAPI_DT_ID)  8)	/* Ingres Time */
# define IIAPI_TMWO_TYPE    ((IIAPI_DT_ID)  6)	/* Time without Timezone */
# define IIAPI_TMTZ_TYPE    ((IIAPI_DT_ID)  7)	/* Time with Timezone */
# define IIAPI_TS_TYPE      ((IIAPI_DT_ID) 19)	/* Ingres Timestamp */
# define IIAPI_TSWO_TYPE    ((IIAPI_DT_ID)  9)	/* Timestamp without Timezone */
# define IIAPI_TSTZ_TYPE    ((IIAPI_DT_ID) 18)	/* Timestamp with Timezone */
# define IIAPI_INTYM_TYPE   ((IIAPI_DT_ID) 33)	/* Interval Year to Month */
# define IIAPI_INTDS_TYPE   ((IIAPI_DT_ID) 34)	/* Interval Day to Second */
# define IIAPI_LOGKEY_TYPE  ((IIAPI_DT_ID) 11)	/* Logical Key */
# define IIAPI_TABKEY_TYPE  ((IIAPI_DT_ID) 12)	/* Table Key */
# define IIAPI_GEOM_TYPE    ((IIAPI_DT_ID) 56)  /* Spatial Long Byte */
# define IIAPI_POINT_TYPE   ((IIAPI_DT_ID) 57)  /* Point Long Byte */
# define IIAPI_MPOINT_TYPE  ((IIAPI_DT_ID) 58)  /* MPoint Long Byte */
# define IIAPI_LINE_TYPE    ((IIAPI_DT_ID) 59)  /* Line Long Byte */
# define IIAPI_MLINE_TYPE   ((IIAPI_DT_ID) 61)  /* MLine Long Byte */
# define IIAPI_POLY_TYPE    ((IIAPI_DT_ID) 62)  /* Poly Long Byte */
# define IIAPI_MPOLY_TYPE   ((IIAPI_DT_ID) 63)  /* Mpoly Long Byte */
# define IIAPI_NBR_TYPE     ((IIAPI_DT_ID) 64)  /* NBR Byte */
# define IIAPI_GEOMC_TYPE   ((IIAPI_DT_ID) 65)	/* Geomc Long Byte */
# define IIAPI_IDATE_TYPE	IIAPI_DTE_TYPE
# define IIAPI_ADATE_TYPE	IIAPI_DATE_TYPE

# define IIAPI_I1_LEN		1
# define IIAPI_I2_LEN		2
# define IIAPI_I4_LEN		4
# define IIAPI_I8_LEN		8
# define IIAPI_F4_LEN		4
# define IIAPI_F8_LEN		8
# define IIAPI_MNY_LEN		8
# define IIAPI_BOOL_LEN		1
# define IIAPI_DTE_LEN		12
# define IIAPI_TIME_LEN		10
# define IIAPI_DATE_LEN		4
# define IIAPI_TS_LEN		14
# define IIAPI_INTYM_LEN	3
# define IIAPI_INTDS_LEN	12
# define IIAPI_LOGKEY_LEN	16
# define IIAPI_TABKEY_LEN	8
# define IIAPI_LOCATOR_LEN	4

# define IIAPI_IDATE_LEN	IIAPI_DTE_LEN
# define IIAPI_ADATE_LEN	IIAPI_DATE_LEN


/*
** Suggested length for long datatype segments
** when declaring for input (IIset_descriptor()).
*/
# define IIAPI_SEGMENT_LEN	2000




/*
** Name: IIAPI_STATUS - API Function Return Status
**
** Description:
**      This data type defines the return status of each API function.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Added basic failure status codes.
*/

typedef II_ULONG IIAPI_STATUS;

/* 
** Valid values for IIAPI_STATUS data type
** Negative values imply additional error
** information is available through an
** error handle.
*/

# define IIAPI_ST_SUCCESS		0
# define IIAPI_ST_MESSAGE		1
# define IIAPI_ST_WARNING		2
# define IIAPI_ST_NO_DATA		3
# define IIAPI_ST_ERROR			4
# define IIAPI_ST_FAILURE		5
# define IIAPI_ST_NOT_INITIALIZED	6
# define IIAPI_ST_INVALID_HANDLE	7
# define IIAPI_ST_OUT_OF_MEMORY		8




/*
** Name: IIAPI_QUERYTYPE - API Query Type
**
** Description:
**      This data type defines the possible query types accepted by the
**      IIapi_query function.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	16-Feb-95 (gordy)
##	    Made IIAPI_QT_EXEC unique so as to handle parameters.
*/

typedef II_ULONG IIAPI_QUERYTYPE;

/* Valid values for IIAPI_QUERYTYPE data type */

# define IIAPI_QT_BASE                 0
# define IIAPI_QT_QUERY                (IIAPI_QT_BASE + 0)
# define IIAPI_QT_SELECT_SINGLETON     (IIAPI_QT_BASE + 1)
# define IIAPI_QT_EXEC                 (IIAPI_QT_BASE + 2)
# define IIAPI_QT_OPEN                 (IIAPI_QT_BASE + 3)
# define IIAPI_QT_CURSOR_DELETE        (IIAPI_QT_BASE + 4)
# define IIAPI_QT_CURSOR_UPDATE        (IIAPI_QT_BASE + 5)
# define IIAPI_QT_DEF_REPEAT_QUERY     (IIAPI_QT_BASE + 6)
# define IIAPI_QT_EXEC_REPEAT_QUERY    (IIAPI_QT_BASE + 7)
# define IIAPI_QT_EXEC_PROCEDURE       (IIAPI_QT_BASE + 8)
# define IIAPI_QT_TOP                  IIAPI_QT_EXEC_PROCEDURE

# define IIAPI_QT_ALTER_GROUP          IIAPI_QT_QUERY
# define IIAPI_QT_ALTER_ROLE           IIAPI_QT_QUERY
# define IIAPI_QT_ALTER_TABLE          IIAPI_QT_QUERY
# define IIAPI_QT_COPY_FROM            IIAPI_QT_QUERY
# define IIAPI_QT_COPY_INTO            IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_DBEVENT       IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_GROUP         IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_INDEX         IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_INTEGRITY     IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_PROCEDURE     IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_ROLE          IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_RULE          IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_TABLE         IIAPI_QT_QUERY
# define IIAPI_QT_CREATE_VIEW          IIAPI_QT_QUERY
# define IIAPI_QT_DELETE               IIAPI_QT_QUERY
# define IIAPI_QT_DESCRIBE             IIAPI_QT_QUERY
# define IIAPI_QT_DIRECT_EXEC          IIAPI_QT_QUERY
# define IIAPI_QT_DROP_DBEVENT         IIAPI_QT_QUERY
# define IIAPI_QT_DROP_GROUP           IIAPI_QT_QUERY
# define IIAPI_QT_DROP_INDEX           IIAPI_QT_QUERY
# define IIAPI_QT_DROP_INTEGRITY       IIAPI_QT_QUERY
# define IIAPI_QT_DROP_PERMIT          IIAPI_QT_QUERY
# define IIAPI_QT_DROP_PROCEDURE       IIAPI_QT_QUERY
# define IIAPI_QT_DROP_ROLE            IIAPI_QT_QUERY
# define IIAPI_QT_DROP_RULE            IIAPI_QT_QUERY
# define IIAPI_QT_DROP_TABLE           IIAPI_QT_QUERY
# define IIAPI_QT_DROP_VIEW            IIAPI_QT_QUERY
# define IIAPI_QT_EXEC_IMMEDIATE       IIAPI_QT_QUERY
# define IIAPI_QT_GRANT                IIAPI_QT_QUERY
# define IIAPI_QT_GET_DBEVENT          IIAPI_QT_QUERY
# define IIAPI_QT_INSERT_INTO          IIAPI_QT_QUERY
# define IIAPI_QT_MODIFY               IIAPI_QT_QUERY
# define IIAPI_QT_PREPARE              IIAPI_QT_QUERY
# define IIAPI_QT_RAISE_DBEVENT        IIAPI_QT_QUERY
# define IIAPI_QT_REGISTER_DBEVENT     IIAPI_QT_QUERY
# define IIAPI_QT_REMOVE_DBEVENT       IIAPI_QT_QUERY
# define IIAPI_QT_REVOKE               IIAPI_QT_QUERY
# define IIAPI_QT_SAVE_TABLE           IIAPI_QT_QUERY
# define IIAPI_QT_SELECT               IIAPI_QT_QUERY
# define IIAPI_QT_SET                  IIAPI_QT_QUERY
# define IIAPI_QT_UPDATE               IIAPI_QT_QUERY




/*
** Name: IIAPI_DATAVALUE - Generic column data.
**
** Description:
**      This structure defines a placeholder of a data value. 
**      All parameters within this structure must be prefixed with dv_.
**
**      dv_null
**          a flag stating whether the value is a null data.
**      dv_length
**          the actual length of the data.
**      dv_value
**          the data buffer.
##
## History:
##      26-apr-94 (ctham)
##          creation.
##	15-Mar-96 (gordy)
##	    Make data length values unsigned to handle large BLOB segments.
*/

typedef struct _IIAPI_DATAVALUE
{
    II_BOOL	    dv_null;
    II_UINT2   	    dv_length;
    II_PTR 	    dv_value;
} IIAPI_DATAVALUE;




/*
** Name: IIAPI_DESCRIPTOR - Generic parameters.
**
** Description:
**      This structure defines description of a data value.  The data vaue
**      being described can have one of the following usages:
**
**      * A tuple data returned by the DBMS server in response to a 'copy'
**        or 'select' commands.
**
**      * Reference of a procedure parameter when a procedure parameter is
**        used as a updatable input.
**
**      * Procedure parameter when a procedure parameter is used as a
**        read-only input.
**
**      * Service parameter when the parameter is required as an input
**        to an API service, not to a SQL command.
**
**      * Query parameter when the parameter has a parameter marker
**        in the query sent.
**
**      All parameters within this structure must be prefixed with ds_.
**
**      ds_dataType
**          data type of the data value.  The valid values are defined
**          in dbms.h.
**      ds_nullable
**          a flag stating whether the data type is nullable.
**      ds_length
**          length of the data value
**      ds_precision
**          precision of the data value if the data type is decimal or float.
**      ds_scale
**          scale of the data value if the data type is decimal.
**      ds_columnType
**          The usage of the data value.
**      ds_columnName
**          This parameter contains the symbolic name of the data value.
##          The data types defined here must be kept consistent with
##          the INGRES data types defined in iicommon.h.
##
## History:
##      01-sep-93 (ctham)
##          creation
##      26-apr-94 (ctham)
##          add nullable flags to IIAPI_DESCRIPTOR and IIAPI_SDDATA.
##	15-Mar-96 (gordy)
##	    Make data length values unsigned to handle large BLOB segments.
##	28-Jun-06 (gordy)
##	    Added IIAPI_COL_PROC{IN,OUT,INOUT}PARM column types.
*/
typedef struct _IIAPI_DESCRIPTOR
{
    IIAPI_DT_ID     ds_dataType;
    II_BOOL	    ds_nullable;
    II_UINT2 	    ds_length;
    II_INT2 	    ds_precision;
    II_INT2 	    ds_scale;
    II_INT2 	    ds_columnType;

# define IIAPI_COL_TUPLE		0 /* tuple column type */
# define IIAPI_COL_PROCBYREFPARM	1 /* procedure byref parm column type */
# define IIAPI_COL_PROCPARM		2 /* procedure parm column type */
# define IIAPI_COL_SVCPARM		3 /* service parm column type */
# define IIAPI_COL_QPARM		4 /* query parm column type */
# define IIAPI_COL_PROCGTTPARM	  	5 /* procedure global temp table */

# define IIAPI_COL_PROCINPARM		2 /* procedure IN parm (alias) */
# define IIAPI_COL_PROCOUTPARM		6 /* procedure OUT parm column type */
# define IIAPI_COL_PROCINOUTPARM	1 /* procedure INOUT parm (alias) */

    II_CHAR II_FAR    *ds_columnName;

} IIAPI_DESCRIPTOR;




/*
** Name: IIAPI_FDATADESCR - Copy file information
**
** Description:
**      This structure defines description of data in the copy file.
**      All parameters within this structure must be prefixed with fd_.
**
**	fd_name
**	    column name
**	fd_type
**	    copy file datatype
**	fd_length
**	    copy file length
**	fd_prec
**	    copy file encoded precision and scale
**	fd_column
**	    descriptor index in copy map dbms descriptor
**	fd_funcID
**	    ADF function ID
**	fd_cvLen
**	    conversion length
**	fd_cvPrec
**	    conversion encoded precision and scale
**	fd_delimiter
**	    TRUE if delimiter
**	fd_delimLength
**	    length of delimiter
**	fd_delimValue
**	    delimiter character(s)
**	fd_nullable
**	    TRUE if WITH NULL specified
**	fd_nullInfo
**	    TRUE if null descriptor and value provided
**	fd_nullDescr
**	    descriptor for null value
**	fd_nullValue
**	    null value
##
## History:
##      06-sep-93 (ctham)
##          creation
##	16-Feb-96 (gordy)
##	    Remove IIAPI_SDDATA (replaced by IIAPI_DESCRIPTOR, IIAPI_DATAVALUE).
##	23-Feb-96 (gordy)
##	    Reworked to properly represent copy info.  Added conversion
##	    fields.  Converted the descriptor for the file entry into
##	    fields since it does not directly describe an API datatype.
##	15-Mar-96 (gordy)
##	    Make data length values unsigned to handle large BLOB segments.
##	22-Apr-96 (gordy)
##	    Finished support for COPY FROM.
*/

typedef struct _IIAPI_FDATADESCR
{
    
    /*
    ** File data description
    */

    II_CHAR II_FAR	*fd_name;
    II_INT2		fd_type;
    II_INT2		fd_length;
    II_INT2		fd_prec;
    
    /*
    ** Conversion info
    */

    II_LONG		fd_column;	/* Descriptor index in copy map */
    II_LONG		fd_funcID;	/* Conversion function ID */
    II_LONG		fd_cvLen;	/* Conversion length */
    II_LONG		fd_cvPrec;	/* Conversion encoded precision/scale */

    /*
    ** Delimiter Description and value
    */
    
    II_BOOL		fd_delimiter;	/* TRUE if delimiter */
    II_INT2		fd_delimLength;
    II_CHAR II_FAR	*fd_delimValue;
    
    /*
    ** NULL Description and Value
    */
    
    II_BOOL		fd_nullable;	/* TRUE if WITH NULL specified */
    II_BOOL		fd_nullInfo;	/* TRUE if null descr/value present */
    IIAPI_DESCRIPTOR	fd_nullDescr;
    IIAPI_DATAVALUE	fd_nullValue;
    
} IIAPI_FDATADESCR;




/*
** Name: IIAPI_COPYMAP - Copy Map
**
** Description:
**      This structure defines description of copy data.  All parameters
**      within this structure must be prefixed with cp_.
**
**	cp_copyFrom
**	    TRUE for COPY FROM, FALSE for COPY INTO
**	cp_flags
**	    copy status flags
**	cp_errorCount
**	    maximum errors allowed
**	cp_fileName
**	    copy file name
**	cp_logName
**	    log file name
**	cp_dbmsCount
**	    number of tuple descriptors
**	cp_dbmsDescr
**	    tuple descriptor
**	cp_fileCount
**	    number of elements in file
**	cp_fileDescr
**	    description of file elments.
##
## History:
##      06-sep-93 (ctham)
##          creation
##	23-Feb-96 (gordy)
##	    Reworked to properly represent copy info.  Added missing fields.
##	    The dbms tuple descriptor describes all columns in the table.
##	    The file descriptor describes the columns being copied to the
##	    file (may be a subset of the table columns) as well as any
##	    delimiter columns.  The two descriptors are indirectly related
##	    via fd_column in the file descriptor.
*/

typedef struct _IIAPI_COPYMAP
{
    
    II_BOOL			cp_copyFrom;
    II_ULONG			cp_flags;
    II_LONG			cp_errorCount;
    II_CHAR II_FAR		*cp_fileName;
    II_CHAR II_FAR		*cp_logName;
    II_INT2			cp_dbmsCount;
    IIAPI_DESCRIPTOR II_FAR	*cp_dbmsDescr;
    II_INT2			cp_fileCount;
    IIAPI_FDATADESCR II_FAR	*cp_fileDescr;
    
} IIAPI_COPYMAP;




/*
** Name: IIAPI_SVR_ERRINFO
**
** Description:
**	Server specific information associated with errors
**	and user messages sent by the server.
**
**	svr_id_error
**	    generic error code or encoded SQLSTATE
**	svr_local_error
**	    native server error code
**	svr_id_server
**	    server ID
**	svr_server_type
**	    server type
**	svr_severity
**	    message type and formatting
**	svr_parmCount
**	    number of message parameters
**	svr_parmDescr
**	    description of message parameters
**	svr_parmValue
**	    value of message parameters
##
## History:
##	23-Jan-95 (gordy)
##	    Created.
*/

typedef struct _IIAPI_SVR_ERRINFO
{

    II_LONG			svr_id_error;
    II_LONG			svr_local_error;
    II_LONG			svr_id_server;
    II_LONG			svr_server_type;
    II_LONG			svr_severity;

#define IIAPI_SVR_DEFAULT	0x0000
#define IIAPI_SVR_MESSAGE	0x0001
#define IIAPI_SVR_WARNING	0x0002
#define IIAPI_SVR_FORMATTED	0x0010

    II_INT2			svr_parmCount;
    IIAPI_DESCRIPTOR II_FAR	*svr_parmDescr;
    IIAPI_DATAVALUE  II_FAR	*svr_parmValue;

} IIAPI_SVR_ERRINFO;




/*
** Name: IIAPI_II_TRAN_ID - Ingres Transaction id.
**
** Description:
**      This structure defines an Ingres transaction id.  
##	It must be kept consistent with the IIAPI_II_TRAN_ID 
##	of iicommon.h.
##
## History:
##     11-may-94 (ctham)
##          creation
*/

typedef struct _IIAPI_II_TRAN_ID
{

    II_UINT4       it_highTran;       /* High part of transaction id */
    II_UINT4       it_lowTran;        /* Low part of transaction id  */

} IIAPI_II_TRAN_ID;




/*
** Name: IIAPI_II_DIS_TRAN_ID - Distributed Transaction id for Ingres.
**
** Description:
**      This structure defines a distributed transaction id for Ingres.
##      It must be kept consistent with DB_TRAN_MAXNAME and
##      IIAPI_II_DIS_TRAN_ID of iicommon.h.
##
## History:
##     11-may-94 (ctham)
##          creation
*/

typedef struct _IIAPI_II_DIS_TRAN_ID
{

    IIAPI_II_TRAN_ID  ii_tranID;

# define IIAPI_TRAN_MAXNAME  64

    II_CHAR	    ii_tranName[IIAPI_TRAN_MAXNAME];	

}   IIAPI_II_DIS_TRAN_ID;


/*
** Name: IIAPI_XA_TRAN_ID - Distributed Transaction id for Ingres.
**
** Description:
**      This structure defines a distributed transaction id for Ingres.
##      It must be kept consistent with DB_XA_DIS_TRAN_ID of iicommon.h.
##
## History:
##     11-may-94 (ctham)
##          creation
*/

typedef struct _IIAPI_XA_TRAN_ID
{
    
    II_LONG         xt_formatID;      /* Format ID-set by TP mon */
    II_LONG         xt_gtridLength;  /* Global transaction ID length */
    II_LONG         xt_bqualLength;  /* Branch Qualifier length */

# define IIAPI_XA_MAXGTRIDSIZE	64 /* for XA's transaction id */
# define IIAPI_XA_MAXBQUALSIZE	64
# define IIAPI_XA_XIDDATASIZE 	IIAPI_XA_MAXGTRIDSIZE + IIAPI_XA_MAXBQUALSIZE

    II_CHAR         xt_data[IIAPI_XA_XIDDATASIZE];
                                          /* Data has the gtrid and  */
                                          /* bqual concatenated      */
                                          /* If gtrid_length is 4 and*/
                                          /* bqual_length is 4, the  */
                                          /* 1st 8 bytes in data will*/
                                          /* contain gtrid and bqual.*/
    
} IIAPI_XA_TRAN_ID;




/*
** Name: IIAPI_XA_DIS_TRAN_ID - Distributed Transaction id for Ingres.
**
** Description:
**      This structure defines a distributed transaction id for Ingres.
##      It must be kept consistent with DB_EXTD_XA_DIS_TRAN_ID of iicommon.h.
##
## History:
##     11-may-94 (ctham)
##          creation
*/

typedef struct _IIAPI_XA_DIS_TRAN_ID
{
    
    IIAPI_XA_TRAN_ID     xa_tranID; /* XA Distributed tran id   */
    II_INT4	         xa_branchSeqnum;   /* Branch local tran seq num */
    II_INT4	         xa_branchFlag;     /* Local 2PC transaction flag */

#define IIAPI_XA_BRANCH_FLAG_NOOP  0x0000 /* For ENCINA: TUX middle trans */
#define IIAPI_XA_BRANCH_FLAG_FIRST 0x0001 /* For FIRST TUX "AS" prepared */
#define IIAPI_XA_BRANCH_FLAG_LAST  0x0002 /* For LAST TUX "AS" prepared */ 
#define IIAPI_XA_BRANCH_FLAG_2PC   0x0004 /* Is TUX tran  2PC */
#define IIAPI_XA_BRANCH_FLAG_1PC   0x0008 /* Is TUX tran  1PC */
    
} IIAPI_XA_DIS_TRAN_ID;




/*
** Name: IIAPI_TRAN_ID - Transaction ID
**
** Description:
**      This structure defines the transaction ID allowed by API.
**      All parameters within this structure must be prefixed with ti_.
**
**      ti_type
**          The type of transaction ID.
**      ti_iiXID
**          value of Ingres transaction ID.
**      ti_xaXID
**          value of XA transaction ID.
##
## History:
##      06-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_TRAN_ID
{
    
    II_ULONG 	    ti_type;

# define IIAPI_TI_IIXID 1
# define IIAPI_TI_XAXID 2

    union	    
    {
	IIAPI_II_DIS_TRAN_ID      iiXID;
	IIAPI_XA_DIS_TRAN_ID      xaXID;
    } ti_value;
    
} IIAPI_TRAN_ID;


/*
** Name: IIAPI_GENPARM - Generic parameters.
**
** Description:
**      This structure defines common subparameters described in the
**      the API specification.  Each API function contains one
**      argument of a specific API parameter structure.  Almost all
**      of the API parameter structures contain this generic parameter
**      structure as the member followed by specific parameters for
**      the API function call.  All parameters within
**      this structure must be prefixed with gp_.
**
**      gp_callback
**          call this function when the current API function completes.
**
**      gp_closure
**          the input parameter to gp_callback
**
**      gp_completed
**          TRUE if the current function has been completed.
**
**      gp_status
**          return status for current function
**
**      gp_errorHandle
**	     An error handle which may be passed to IIapi_getErrorInfo().
**	     Only valid if gp_status is IIAPI_ST_FAILURE, NULL otherwise.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
*/

typedef struct _IIAPI_GENPARM
{
    
    /* Common input parameters: */

    II_VOID	(II_FAR II_CALLBACK *gp_callback)( II_PTR closure, 
                                                   II_PTR parmBlock );
    II_PTR	gp_closure;
    
    /*
    ** Common output parameters:
    ** The output parameter values are only valid if gp_completed is set
    ** to TRUE by API.
    */
    
    II_BOOL		gp_completed;
    IIAPI_STATUS	gp_status;
    II_PTR		gp_errorHandle;
    
} IIAPI_GENPARM;


/*
** Name: IIAPI_ABORTPARM - input parameter for IIapi_abort()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_abort().  All parameters within
**      this structure must be prefixed with ab_.
**
**      ab_genParm
**          generic API parameters
**
**      ab_connHandle
**          connection handle identifying the connection to be released.
##
## History:
##      10-Aug-98 (rajus01)
##          Created.
*/

typedef struct _IIAPI_ABORTPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	ab_genParm;

    /* Input parameters: */
    
    II_PTR	ab_connHandle;

} IIAPI_ABORTPARM;


/*
** Name: IIAPI_AUTOPARM - input parameter for IIapi_autocommit()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_autocommit().  All parameters within
**      this structure must be prefixed with ac_.
**
**      ac_genParm
**          common API parameters.
**	ac_connHandle
**	    connection handle identifying the connection associated
**	    with the autocommit transaction.
**      ac_tranHandle
**          transaction handle identifying the autocommit transaction.
##
## History:
##      09-Jul-96 (gordy)
##          Created.
*/

typedef struct _IIAPI_AUTOPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   ac_genParm;
    
    /* Specific input parameters: */
    
    II_PTR	    ac_connHandle;
    
    /* Specific input & output parameters available at function return: */
    
    II_PTR 	    ac_tranHandle;
    
} IIAPI_AUTOPARM;


/*
** Name: IIAPI_BATCHPARM - input parameter for IIapi_batch()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_batch().  All parameters within
**      this structure must be prefixed with ba_.
**
**      ba_genParm
**          common API parameters
**      ba_connHandle
**          connection handle identifying the connection in which
**          the batch will be executed.
**      ba_queryType
**          the type of query.
**      ba_queryText
**          the text of query.
**	ba_parameters
**	    TRUE if sending parameters with query.
**	ba_flags
**	    batch execution flags.
**      ba_tranHandle
**          transaction handle identifying the transaction to which
**          the query belongs.
**      ba_stmtHandle
**          statement handle identifying the query to be invoked.
##
## History:
##	25-Mar-10 (gordy)
##	    Created.
*/

typedef struct _IIAPI_BATCHPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	ba_genParm;
    
    /* Specific input parameters: */
    
    II_PTR		ba_connHandle;
    IIAPI_QUERYTYPE	ba_queryType;
    II_CHAR II_FAR	*ba_queryText;
    II_BOOL		ba_parameters;
    II_UINT4		ba_flags;

    /* Specific Input+Output parameter(s): */
    
    II_PTR		ba_tranHandle;
    II_PTR		ba_stmtHandle;

} IIAPI_BATCHPARM;


/*
** Name: IIAPI_CANCELPARM - input parameter for IIapi_cancel()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_cancel().  All parameters within
**      this structure must be prefixed with cn_.
**
**      cn_genParm
**          generic API parameters
**      cn_stmtHandle
**          This parameter specifies the statement to be cancelled.
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_CANCELPARM	    /* structure id - cn */
{
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	    cn_genParm;
    
    /* Specific inputs parameters: */
    
    II_PTR		    cn_stmtHandle ;
    
} IIAPI_CANCELPARM;


/*
** Name: IIAPI_CATCHEVENTPARM - input parameter for IIapi_catchEvent()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_catchEvent().  All parameters within
**      this structure must be prefixed with ce_.
**
**      ce_genParm
**          generic API parameters
**      ce_connHandle
**          This parameter specifies the connection in which the
**          specific event should be caught.
**	ce_selectEventName
**          This parameter specifies the name of the event to be
**          caught.
**	ce_selectEventOwner
**          This parameter specifies the name of the event owner
**          for the event to be caught.
**      ce_eventHandle
**          This parameter specifies the event handle created
**          to identify this event registration.
**      ce_eventName
**	    The name of the event caught.
**      ce_eventOwner
**	    The owner of the event caught.
**      ce_eventDB
**          the database of the event caught.
**      ce_eventTime
**          the time the event occurred; a IIAPI_DTE_TYPE data value.
**      ce_eventInfoAvail
**          If TRUE call IIapi_getDescriptor() and IIapi_getColumns().
##
## History:
##      01-sep-93 (ctham)
##          creation
##      11-apr-94 (ctham)
##          change data type of IIAPI_CATCHEVENTPARM's ce_eventTime to
##          from II_LONG to make it more portable.
##	 31-may-94 (ctham)
##	     added ce_selectEventName and ce_selectEventOwner.
##	25-Apr-95 (gordy)
##	    Cleaned up Database Events.
*/

typedef struct _IIAPI_CATCHEVENTPARM
{
    IIAPI_GENPARM	ce_genParm;
    
    /* Specific input parameters: */
    
    II_PTR		ce_connHandle;
    II_CHAR II_FAR	*ce_selectEventName;
    II_CHAR II_FAR	*ce_selectEventOwner;
    
    /* Specific input & output parameters available at function return: */
    
    II_PTR		ce_eventHandle;
    
    /* Specific output parameters: */
    
    II_CHAR II_FAR	*ce_eventName;
    II_CHAR II_FAR	*ce_eventOwner;
    II_CHAR II_FAR	*ce_eventDB;
    IIAPI_DATAVALUE	ce_eventTime;
    II_BOOL		ce_eventInfoAvail;
    
} IIAPI_CATCHEVENTPARM;


/*
** Name: IIAPI_CLOSEPARM - input parameter for IIapi_close()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_close().  All parameters within
**      this structure must be prefixed with cl_.
**
**      cl_genParm
**          common API parameters
**      cl_stmtHandle
**          statement handle identifying the query to be closed.
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_CLOSEPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   cl_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    cl_stmtHandle;
    
} IIAPI_CLOSEPARM;


/*
** Name: IIAPI_COMMITPARM - input parameter for IIapi_commit()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_commit().  All parameters within
**      this structure must be prefixed with cm_.
**
**      cm_genParm
**          common API parameters.
**      cm_tranHandle
**          transaction handle identifying the transaction to be
**          commited.
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_COMMITPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   cm_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    cm_tranHandle;
    
} IIAPI_COMMITPARM;


/*
** Name: IIAPI_CONNPARM - input parameter for IIapi_connect()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_connect().  All parameters within
**      this structure must be prefixed with co_.
**
**      co_genParm
**          common API parameters.
**	co_type
**	    connection type
**      co_target
**          database name
**      co_username
**          user name
**      co_passowrd
**          passord
**      co_timeout
**          timeout in milli-seconds: -1 = none
**      co_options
**          GCA_MD_ASSOC data
**      co_connHandle
**          connection handle identifying the connection to be established.
**      co_sizeAdvise
**          size advised for a single buffer for this connection.
**      co_apiLevel
**          API level supported by the peer.
##
## History:
##      01-sep-93 (ctham)
##          creation
##      11-apr-94 (ctham)
##          change sizeAdvise of IIAPI_CONNPARM to co_sizeAdvise to
##          adhere with the INGRES coding convention.
##	14-Nov-94 (gordy)
##	    Removed the SESSIONINFO member.  The same functionality
##	    is now available with IIapi_setConnParam().
##	18-Nov-94 (gordy)
##	    Added defines for API level.  Added co_tranHandle to
##	    to facilitate restarting distributed transactions.
##	12-Feb-97 (gordy)
##	    Version 2: added connection type parameter.
##	25-Oct-06 (gordy)
##	    Level 5: LOB Locators.
##	26-Oct-09 (gordy)
##	    Level 6: Boolean.
##	16-Feb-10 (thich01)
##	    Level 7: Spatial.
*/

typedef struct _IIAPI_CONNPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	co_genParm;
    
    /* Specific input parameters: */
    
    II_CHAR II_FAR	*co_target;
    II_CHAR II_FAR	*co_username;
    II_CHAR II_FAR	*co_password;
    II_LONG		co_timeout;
    
    /* Specific input+output parameters available at function return */

    II_PTR		co_connHandle;
    II_PTR		co_tranHandle;
    
    /* Specific output parameters: */
    
    II_LONG		co_sizeAdvise;
    II_LONG		co_apiLevel;

# define IIAPI_LEVEL_0		0	/* Base data types */
# define IIAPI_LEVEL_1		1	/* Decimal, binary, BLOB */
# define IIAPI_LEVEL_2		2	/* National Character Set */
# define IIAPI_LEVEL_3		3	/* Eight-byte integers */
# define IIAPI_LEVEL_4		4	/* ANSI date/time types */
# define IIAPI_LEVEL_5		5	/* Blob/Clob locator types */
# define IIAPI_LEVEL_6		6	/* Boolean type */
# define IIAPI_LEVEL_7		7	/* Spatial types */
    
    /* Version 2 parameters */

    II_LONG		co_type;

# define IIAPI_CT_SQL		1	/* SQL DBMS Connections */
# define IIAPI_CT_NS		3	/* Name Server Connections */

} IIAPI_CONNPARM;


/*
**
** Name: IIAPI_CONVERTPARM - input parameter for IIapi_convertData()
**
** Description:
**	This structure defines the input and output parameters for
**	the API function IIAPI_convertData().  All parameters within
**	this structure must be prefixed with cv_.
**
**	cv_srcDesc
**	    Descriptor of original data type.
**
**	cv_srcValue
**	    Original data value.
**
**	cv_dstDesc
**	    Descriptor of resulting data type.
**
**	cv_dstValue
**	    Result data value.
**
**	cv_status
**	    Conversion status.
##
## History:
##	22-Feb-95 (gordy)
##	    Created.
*/

typedef struct _IIAPI_CONVERTPARM
{
    /* Input parameters */

    IIAPI_DESCRIPTOR	cv_srcDesc;
    IIAPI_DATAVALUE	cv_srcValue;
    IIAPI_DESCRIPTOR	cv_dstDesc;

    /* Output parameters */

    IIAPI_DATAVALUE	cv_dstValue;
    IIAPI_STATUS	cv_status;

} IIAPI_CONVERTPARM;


/*
**
** Name: IIAPI_FORMATPARM - input parameter for IIapi_formatData()
**
** Description:
**	This structure defines the input and output parameters for
**	the API function IIAPI_formatData().  All parameters within
**	this structure must be prefixed with fd_.
**
**	fd_envHandle
**	    environment handle for which the parameter is being formatted.
**
**	fd_srcDesc
**	    Descriptor of original data type.
**
**	fd_srcValue
**	    Original data value.
**
**	fd_dstDesc
**	    Descriptor of resulting data type.
**
**	fd_dstValue
**	    Result data value.
**
**	fd_status
**	    Conversion status.
##
## History:
##	14-Aug-98 (rajus01)
##	    Created.
*/

typedef struct _IIAPI_FORMATPARM
{
    /* Input parameters */

    II_PTR		fd_envHandle;
    IIAPI_DESCRIPTOR	fd_srcDesc;
    IIAPI_DATAVALUE	fd_srcValue;
    IIAPI_DESCRIPTOR	fd_dstDesc;

    /* Output parameters */

    IIAPI_DATAVALUE	fd_dstValue;
    IIAPI_STATUS	fd_status;

} IIAPI_FORMATPARM;


/*
** Name: IIAPI_DISCONNPARM - input parameter for IIapi_disconnect()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_disconnect().  All parameters within
**      this structure must be prefixed with dc_.
**
**      dc_genParm
**          generic API parameters
**      dc_connHandle
**          connection handle identifying the connection to be released.
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_DISCONNPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   dc_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    dc_connHandle;
    
} IIAPI_DISCONNPARM;


/*
** Name: IIAPI_GETCOLINFOPARM - input parameter for IIapi_getColumnInfo()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getColumnInfo().  All parameters within
**      this structure must be prefixed with gi_.
**
**	gi_stmtHandle
**	    Statement handle identifying the query of which the column
**	    info is requested.
**	gi_columnNumber
**	    Number of the column, starting with 1, for which info is
**	    being requested.
**	gi_status
**	    Request status.
**	gi_mask
**	    A mask specifying which column info is available.
**	gi_lobLength
**	    This parameter contains valid value if gq_mask contains
**	    IIAPI_GI_LOB_LENGTH.  It specifies the length of a LOB
**	    as provided by the DBMS.
##
## History:
##	11-Aug-08 (gordy)
##	    Created.
*/

typedef struct _IIAPI_GETCOLINFOPARM
{
    
    /* Specific input parameters: */
    
    II_PTR		gi_stmtHandle;
    II_INT2		gi_columnNumber;

    /*
    ** Specific output parameters:
    ** The mask indicates which response data is returned.
    */

    IIAPI_STATUS	gi_status;
    II_ULONG		gi_mask;

# define IIAPI_GI_LOB_LENGTH		0x0001

    II_UINT8		gi_lobLength;

} IIAPI_GETCOLINFOPARM;


/*
** Name: IIAPI_GETCOLPARM - input parameter for IIapi_getColumns()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getColumns().  All parameters within
**      this structure must be prefixed with gc_.
**
**      gc_genParm
**          common API parameters
**      gc_stmtHandle
**          statement handle
**      gc_rowCount
**          the number of rows to fetch
**      gc_columnCount
**          number of columns to fetch
**      gc_columnData
**          where to store results
**	 gc_rowsReturned
**	     number of rows returned
**      gc_moreSegments
**          more segments in BLOB
##
## History:
##      01-sep-93 (ctham)
##          creation
##	10-Nov-94 (gordy)
##	    Changed gc_prefetchrows to gc_rowCount
##	    and added gc_rowsReturned.
*/

typedef struct _IIAPI_GETCOLPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   gc_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    gc_stmtHandle;
    II_INT2	    gc_rowCount;
    II_INT2 	    gc_columnCount;
    
    /* Specific output parameters: */
    
    IIAPI_DATAVALUE II_FAR	*gc_columnData;
    II_INT2	    		gc_rowsReturned;
    II_BOOL 	    		gc_moreSegments;
    
} IIAPI_GETCOLPARM;


/*
** Name: IIAPI_GETCOPYMAPPARM - input parameter for IIapi_getCopyMap()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getCopyMap().  All parameters within
**      this structure must be prefixed with gm_.
**
**      gm_genParm
**          common API parameters
**      gm_stmtHandle
**          statement handle identifying the 'copy' statemetn which
**          requires a copy map in response.
**      gm_copyMap
**          the copy map returned from DBMS
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_GETCOPYMAPPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   gm_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    gm_stmtHandle;
    
    /* Specific output parameters: */
    
    IIAPI_COPYMAP   gm_copyMap;
    
} IIAPI_GETCOPYMAPPARM;


/*
** Name: IIAPI_GETDESCRPARM - input parameter for IIapi_getDescriptor()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getDescriptor().  All parameters within
**      this structure must be prefixed with gd_.
**
**      gd_genParm
**          common API parameters
**      gd_stmtHandle
**          statement handle identifying the query which requires
**          tupe description in response.
**      gd_descriptorCount
**          number of tuple descriptors.
**      d_descriptor
**          tuple descriptor.
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_GETDESCRPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   gd_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    gd_stmtHandle;
    
    /* Specific output parameters: */
    
    II_INT2	    		gd_descriptorCount;
    IIAPI_DESCRIPTOR II_FAR	*gd_descriptor;
    
} IIAPI_GETDESCRPARM;


/*
** Name: IIAPI_GETEINFOPARM - input parameter for IIapi_getErrorInfo()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getErrorInfo().  All parameters within
**      this structure must be prefixed with ge_.
**
**	ge_errorHandle
**	    error handle returned in generic parameters
**	ge_type
**	    type of message: error, warning, user
**	ge_SQLSTATE
**	    SQLSTATE of message
**	ge_errorCode
**	    error code
**	ge_message
**	    text of message
**	ge_serverInfoAvail
**	    TRUE if message from server and following structure populated
**	ge_serverInfo
**	    additional information from server
**	ge_status
**	    the status of IIapi_getErrorInfo().
##
## History:
##      01-sep-93 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
##	11-Jul-06 (gordy)
##	    Added IIAPI_GE_XAERR.
*/

typedef struct _IIAPI_GETEINFOPARM
{
    
    /* Specific input parameters: */
    
    II_PTR 	    		ge_errorHandle;
    
    /* Specific output parameters: */
    
    II_LONG			ge_type;

# define IIAPI_GE_ERROR		1
# define IIAPI_GE_WARNING	2
# define IIAPI_GE_MESSAGE	3
# define IIAPI_GE_XAERR		4

    II_CHAR			ge_SQLSTATE[ II_SQLSTATE_LEN + 1 ];
    II_LONG			ge_errorCode;
    II_CHAR II_FAR     		*ge_message;

    II_BOOL			ge_serverInfoAvail;
    IIAPI_SVR_ERRINFO II_FAR	*ge_serverInfo;

    IIAPI_STATUS		ge_status;
    
} IIAPI_GETEINFOPARM;


/*
** Name: IIAPI_GETEVENTPARM - input parameter for IIapi_getEvent()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getEvent().  All parameters within
**      this structure must be prefixed with gv_.
**
**      gv_genParm
**          common API parameters
**      gv_connHandle
**          connection handle
**      gv_timeout
**          timeout in milli-seconds: -1 = none
**	
##
## History:
##	12-Jul-96 (gordy)
##	    Created.
*/

typedef struct _IIAPI_GETEVENTPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   gv_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    gv_connHandle;
    II_LONG	    gv_timeout;
    
} IIAPI_GETEVENTPARM;


/*
** Name: IIAPI_GETQINFOPARM - input parameter for IIapi_getQueryInfo()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_getQueryInfo().  All parameters within
**      this structure must be prefixed with gq_.
**
**      gq_genParm
**          common API parameters.
**      gq_stmtHandle
**          statement handle identifying the query of which the response
**          data is desired.
**	gq_flags
**	    a set of flags detailing query status.
**      gq_mask
**          a mask specifying which response data is available.
**      gq_rowCount
**          This parameter contains valid value if gq_mask contains
**          IIAPI_GQ_ROW_COUNT.  It specifies the number of rows affected
**          by the query.
**	gq_readonly
**	    This parameter contains a valid valie if gq_mask contains
**	    IIAPI_GQ_CURSOR.  TRUE if the statement represents a READONLY 
**	    cursor, FALSE if the cursor is updatable.
**      gq_procedureReturn
**          This parameter contains valid value if gq_mask contains
**          IIAPI_GQ_RET_PROC.  It specifies the status of the database
**          procedure after execution.
**	gq_procedureHandle
**	    This parameter contains a valie value if gq_mask contains
**	    IIAPI_GQ_PROCEDURE_ID.  It contains the handle identifying
**	    a procedure.
**      gq_repeatQueryHandle
**          This parameter contains valid value if gq_mask contains
**          IIAPI_GQ_REPEAT_QUERY_ID.  It contains the handle identify
**          a repeatable query.
**      gq_tableKey
**          This parameter contains valid value if gq_mask contains
**          IIAPI_GQ_TABLE_KEY.  It contains the name of table key.
**      gq_objectKey
**          This parameter contains valid value if gq_mask contains
**          IIAPI_GQ_OBJECT_KEY.  It contains the name of object key.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	21-Nov-94 (gordy)
##	    Added gq_readonly.
##	10-Jan-95 (gordy)
##	    Added gq_procedureHandle.
##	10-Feb-95 (gordy)
##	    Reorganized.
##	13-Feb-95 (gordy)
##	    Added gq_flags.
##	22-May-95 (gordy)
##	    Fixed typo in symbol.
##	15-Mar-07 (gordy)
##	    Added cursor type, status, and position.
*/

typedef struct _IIAPI_GETQINFOPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	gq_genParm;
    
    /* Specific input parameters: */
    
    II_PTR		gq_stmtHandle;
    
    /*
    ** Specific output parameters:
    ** The mask indicates which response data is returned.
    */

    II_ULONG		gq_flags;

# define IIAPI_GQF_FAIL			0x0001	/* Query produced an error */
# define IIAPI_GQF_ALL_UPDATED		0x0002	/* SQLWARN4: no where clause */
# define IIAPI_GQF_NULLS_REMOVED	0x0004	/* SQLWARN2: NULLs eliminated */
# define IIAPI_GQF_UNKNOWN_REPEAT_QUERY	0x0008	/* Repeat Query not defined */
# define IIAPI_GQF_END_OF_DATA		0x0010	/* End of Data for cursor */
# define IIAPI_GQF_CONTINUE		0x0020	/* Copy stmt flow control */
# define IIAPI_GQF_INVALID_STATEMENT	0x0040	/* SQLWARN5: invalid stmt */
# define IIAPI_GQF_TRANSACTION_INACTIVE	0x0080	/* Inactive Transaction State */
# define IIAPI_GQF_OBJECT_KEY		0x0100	/* Object key returned */
# define IIAPI_GQF_TABLE_KEY		0x0200	/* Table key returned */
# define IIAPI_GQF_NEW_EFFECTIVE_USER	0x0400	/* Effective user ID changed */
# define IIAPI_GQF_FLUSH_QUERY_ID	0x0800	/* Query/Proc IDs invalidated */
# define IIAPI_GQF_ILLEGAL_XACT_STMT	0x1000	/* Transaction stmt attempted */

    II_ULONG		gq_mask;

# define IIAPI_GQ_ROW_COUNT		0x0001
# define IIAPI_GQ_CURSOR		0x0002
# define IIAPI_GQ_PROCEDURE_RET		0x0004
# define IIAPI_GQ_PROCEDURE_ID		0x0008
# define IIAPI_GQ_REPEAT_QUERY_ID	0x0010
# define IIAPI_GQ_TABLE_KEY		0x0020
# define IIAPI_GQ_OBJECT_KEY		0x0040
# define IIAPI_GQ_ROW_STATUS		0x0080

    II_LONG		gq_rowCount;
    II_BOOL		gq_readonly;
    II_LONG		gq_procedureReturn;
    II_PTR		gq_procedureHandle;
    II_PTR		gq_repeatQueryHandle;

# define IIAPI_TBLKEYSZ 8

    II_CHAR		gq_tableKey[IIAPI_TBLKEYSZ];

# define IIAPI_OBJKEYSZ 16

    II_CHAR		gq_objectKey[IIAPI_OBJKEYSZ];
    
    /* Version 6 parameters */

    II_ULONG		gq_cursorType;

# define IIAPI_CURSOR_UPDATE		0x0001	/* Updatable */
# define IIAPI_CURSOR_SCROLL		0x0002	/* Scrollable */

    II_ULONG		gq_rowStatus;

# define IIAPI_ROW_BEFORE		0x0001	/* Before first row */
# define IIAPI_ROW_FIRST		0x0002	/* First row */
# define IIAPI_ROW_LAST			0x0004	/* Last row */
# define IIAPI_ROW_AFTER		0x0008  /* After last row */
# define IIAPI_ROW_INSERTED		0x0010	/* Row added */
# define IIAPI_ROW_UPDATED		0x0020	/* Row changed */
# define IIAPI_ROW_DELETED		0x0040	/* Row removed */

    II_LONG		gq_rowPosition;

} IIAPI_GETQINFOPARM;


/*
** Name: IIAPI_INITPARM - input parameter for IIapi_initialize()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_intialize().  All parameters within
**      this structure must be prefixed with in_.
**
**	in_timeout
**	    timeout in milli-seconds: -1 = none.
**	in_version
**	    API interface version used by application.
**	in_envHandle
**	    environment handle for version >= 2.
**	in_status
**	    the status of IIapi_initialize().
##
## History:
##      17-may-94 (ctham)
##          creation
##	18-Nov-94 (gordy)
##	    Added in_version.
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
##	 7-Feb-97 (gordy)
##	    API Version 2: added in_envHandle.
##	25-Oct-06 (gordy)
##	    API Version 6: support for LOB locators.
##	26-Oct-09 (gordy)
##	    API Version 7: support for boolean.
##	16-Oct-10 (thich01)
##	    API Version 8: support for spatial.
*/

typedef struct _IIAPI_INITPARM
{
    
    /* Specific input parameters */

    II_LONG		in_timeout;
    II_LONG		in_version;

# define IIAPI_VERSION_1	1	/* Initial version */
# define IIAPI_VERSION_2	2	/* Environment handles */
# define IIAPI_VERSION_3	3	/* NCS data types */
# define IIAPI_VERSION_4	4	/* Eight-byte integers */
# define IIAPI_VERSION_5	5	/* ANSI date/time types */
# define IIAPI_VERSION_6	6	/* Blob/Clob locator types */
# define IIAPI_VERSION_7	7	/* Boolean type */
# define IIAPI_VERSION_8	8	/* Spatial types */
# define IIAPI_VERSION		IIAPI_VERSION_8

    /* Specific output parameters */

    IIAPI_STATUS	in_status;

    /* Version 2 parameters */

    II_PTR		in_envHandle;		/* output */

} IIAPI_INITPARM;


/*
** Name: IIAPI_MODCONNPARM - input parameter for IIapi_modifyConnect()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_modifyConnect().  All parameters within
**      this structure must be prefixed with mc_.
**
**      mc_genParm
**          generic API parameters
**      mc_connHandle
**          connection handle identifying the connection to be modified
**          
##
## History:
##	14-Nov-94 (gordy)
##	    Created.
*/

typedef struct _IIAPI_MODCONNPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   mc_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    mc_connHandle;
    
} IIAPI_MODCONNPARM;


/*
** Name: IIAPI_POSPARM - input parameter for IIapi_position()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_position().  All parameters within
**      this structure must be prefixed with po_.
**
**      po_genParm
**          common API parameters
**      po_stmtHandle
**          statement handle
**	po_anchor
**	    Reference anchor position
**	po_offset
**	    Row offset from anchor position
**      po_rowCount
**          the number of rows to fetch
##
## History:
##	15-Mar-07 (gordy)
##	    Created.
*/

typedef struct _IIAPI_POSPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   po_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    po_stmtHandle;
    II_UINT2	    po_reference;

# define IIAPI_POS_BEGIN	0x0001	/* Beginning of result-set */
# define IIAPI_POS_END		0x0002	/* End of result-set */
# define IIAPI_POS_CURRENT	0x0003	/* Current position in result-set */

    II_INT4	    po_offset;
    II_INT2	    po_rowCount;
    
} IIAPI_POSPARM;


/*
** Name: IIAPI_PREPCMTPARM - input parameter for IIapi_prepareCommit()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_prepareCommit().  All parameters within
**      this structure must be prefixed with pr_.
**
**      pr_genParm
**          common API parameters
**      pr_tranHandle
**          transaction handle identifying the transaction for the 2PC.
##
## History:
##	01-sep-93 (ctham)
##	    creation
*/

typedef struct _IIAPI_PREPCMTPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   pr_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    pr_tranHandle;
    
} IIAPI_PREPCMTPARM;


/*
** Name: IIAPI_PUTCOLPARM - input parameter for IIapi_putColumns()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_putColumns().  All parameters within
**      this structure must be prefixed with pc_.
**
**      pc_genParm
**          common API parameters
**      pc_stmtHandle
**          statement handle identifying the query which sending
**          tuple to the DBMS server is required.
**      pc_columnCount
**          number of columns to put into the database table.
**      pc_columnData
**          where to find data
**      pc_moreSegments
**          more segments in BLOB
##
## History:
##	01-sep-93 (ctham)
##	    creation
*/

typedef struct _IIAPI_PUTCOLPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   pc_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    		pc_stmtHandle;
    II_INT2 	    		pc_columnCount;
    IIAPI_DATAVALUE II_FAR	*pc_columnData;
    II_BOOL 	    		pc_moreSegments;
    
} IIAPI_PUTCOLPARM;


/*
** Name: IIAPI_PUTPARMPARM - input parameter for IIapi_putParms()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_putParms().  All parameters within
**      this structure must be prefixed with pp_.
**
**      pp_genParm
**          common API parameters
**      pp_stmtHandle
**          statement handle
**      pp_parmCount
**          number of parms to put
**      pp_parmData
**          where to find data
**      pp_moreSegments
**          more segments in BLOB
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_PUTPARMPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   pp_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    		pp_stmtHandle;
    II_INT2 	    		pp_parmCount;
    IIAPI_DATAVALUE II_FAR	*pp_parmData;
    II_BOOL 	    		pp_moreSegments;
    
} IIAPI_PUTPARMPARM;


/*
** Name: IIAPI_QUERYPARM - input parameter for IIapi_query()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_query().  All parameters within
**      this structure must be prefixed with qy_.
**
**      qy_genParm
**          common API parameters
**      qy_connHandle
**          connection handle identifying the connection in which
**          the query will be executed.
**      qy_queryType
**          the type of query.
**      qy_queryText
**          the text of query.
**	qy_parameters
**	    TRUE if sending parameters with query.
**      qy_tranHandle
**          transaction handle identifying the transaction to which
**          the query belongs.
**      qy_stmtHandle
**          statement handle identifying the query to be invoked.
**	qy_flags
**	    query execution flags.
##
## History:
##      01-sep-93 (ctham)
##          creation
##	16-Feb-95 (gordy)
##	    Added qy_parameters.
##	25-Oct-06 (gordy)
##	    Added qy_flags.
*/

typedef struct _IIAPI_QUERYPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	qy_genParm;
    
    /* Specific input parameters: */
    
    II_PTR		qy_connHandle;
    IIAPI_QUERYTYPE	qy_queryType;
    II_CHAR II_FAR	*qy_queryText;
    II_BOOL		qy_parameters;

    /* Specific Input+Output parameter(s): */
    
    II_PTR		qy_tranHandle;
    
    /* Specific Outputs available at function return: */
    
    II_PTR		qy_stmtHandle;
    
    /* Version 6 parameters */

    II_ULONG		qy_flags;

# define IIAPI_QF_LOCATORS		0x0001 /* Blob/Clob locators allowed */
# define IIAPI_QF_SCROLL		0x0002 /* Scrollable cursor */

} IIAPI_QUERYPARM;


/*
** Name: IIAPI_REGXIDPARM - input parameter for IIapi_registerXID()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_registerXID().  All parameters within
**      this structure must be prefixed with rg_.
**
**      rg_tranID
**          unique transaction name
**      rg_tranIdHandle
**          handle specifying the transaction ID to be created
**      rg_status
**          return status of RegisterXID
##
## History:
##	01-sep-93 (ctham)
##	    creation
*/

typedef struct _IIAPI_REGXIDPARM
{
    
    /* Input parameters: */
    
    IIAPI_TRAN_ID	rg_tranID;
    
    /* Output parameters: */
    
    II_PTR		rg_tranIdHandle;
    IIAPI_STATUS	rg_status;
    
} IIAPI_REGXIDPARM;


/*
** Name: IIAPI_RELXIDPARM - input parameter for IIapi_releaseXID()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_releaseXID().  All parameters within
**      this structure must be prefixed with rl_.
**
**      rl_tranIdHandle
**          handle specifying the transaction ID to be released
**      rl_status
**          return status of ReleaseXID
**      rl_sQLState
**          return SQLSTATE of ReleaseXID
**      rl_nativeError
**          return native error of ReleaseXID
##
## History:
##	01-sep-93 (ctham)
##	    creation
*/

typedef struct _IIAPI_RELXIDPARM
{
    
    /* Input parameters: */
    
    II_PTR 	    rl_tranIdHandle;
    
    /* Output parameters: */
    
    IIAPI_STATUS    rl_status;
    
} IIAPI_RELXIDPARM;


/*
** Name: IIAPI_RELENVPARM - input parameter for IIapi_releaseEnv()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_releaseEnv().  All parameters within
**      this structure must be prefixed with re_.
**
**	re_envHandle
**	    Environment handle to be release.
**	re_status
**	    The status of IIapi_releaseEnv().
##
## History:
##	 7-Feb-97 (gordy)
##	    Created.
*/

typedef struct _IIAPI_RELENVPARM
{
    
    /* Input parameters: */
    
    II_PTR		re_envHandle;

    /* Output parameters */
    
    IIAPI_STATUS	re_status;
    
} IIAPI_RELENVPARM;


/*
** Name: IIAPI_ROLLBACKPARM - input parameter for IIapi_rollback()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_rollback().  All parameters within
**      this structure must be prefixed with rb_.
**
**      rb_genParm
**          common API parameters
**      rb_tranHandle
**          handle specifying the transaction to be rolled back
**      rb_savePointHandle
**          Optional savepoint: null = none
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_ROLLBACKPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   rb_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    rb_tranHandle;
    II_PTR 	    rb_savePointHandle;
    
} IIAPI_ROLLBACKPARM;


/*
** Name: IIAPI_SAVEPTPARM - input parameter for IIapi_savePoint()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_savePoint().  All parameters within
**      this structure must be prefixed with sp_.
**
**      sp_genParm
**          common API parameters
**      sp_tranHandle
**          handle specifying transaction for which the savepoints*
**          are maintained
**      sp_savePoint
**          a null-terminated string specifying the savepoint
**      sp_savePointHandle
**          handle identifying the savepoint
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_SAVEPTPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	sp_genParm;
    
    /* Specific input parameters: */
    
    II_PTR		sp_tranHandle;
    II_CHAR II_FAR	*sp_savePoint;
    
    /* Specific output parameters: */
    
    II_PTR		sp_savePointHandle;
    
} IIAPI_SAVEPTPARM;


/*
** Name: IIAPI_SCROLLPARM - input parameter for IIapi_scroll()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_scroll().  All parameters within
**      this structure must be prefixed with sl_.
**
**      sl_genParm
**          common API parameters
**      sl_stmtHandle
**          statement handle
**	sl_orientation
**	    Scrolling orientation
**	sl_offset
**	    Row offset from anchor position
##
## History:
##	15-Mar-07 (gordy)
##	    Created.
*/

typedef struct _IIAPI_SCROLLPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   sl_genParm;
    
    /* Specific input parameters: */
    
    II_PTR 	    sl_stmtHandle;
    II_UINT2	    sl_orientation;

# define IIAPI_SCROLL_BEFORE	0x0001	/* Before first row */
# define IIAPI_SCROLL_FIRST	0x0002	/* First row */
# define IIAPI_SCROLL_PRIOR	0x0003	/* Previous row */
# define IIAPI_SCROLL_CURRENT	0x0004	/* Current row */
# define IIAPI_SCROLL_NEXT	0x0005	/* Next row */
# define IIAPI_SCROLL_LAST	0x0006	/* Last row */
# define IIAPI_SCROLL_AFTER	0x0007	/* After last row */
# define IIAPI_SCROLL_ABSOLUTE	0x0008	/* Specific row */
# define IIAPI_SCROLL_RELATIVE	0x0009	/* From current row */

    II_INT4	    sl_offset;
    
} IIAPI_SCROLLPARM;


/*
** Name: IIAPI_SETENVPRMPARM - input parameter for IIapi_setEnvParm()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_setEnvParm().  All parameters within
**      this structure must be prefixed with se_.
**
**	se_envHandle
**	    environment handle for which the parameter is being set.
**
**	se_paramID
**	    numeric ID of the parameter being set (defined below).
**
**	se_paramValue
**	    pointer to value of the parameter being set (type defined below).
**	    all II_CHAR values must be null terminated strings.
**
**	se_status
**	    the status of IIapi_setEnvParm().
**
## History:
##	10-Aug-98 (rajus01)
##	    Created.
##	15-Mar-04 (gordy)
##	    Added IIAPI_EP_INT8_WIDTH.
*/

typedef struct _IIAPI_SETENVPRMPARM
{

    /* Input parameters */

    II_PTR 	    se_envHandle;
    II_LONG	    se_paramID;
    II_PTR	    se_paramValue;

    /* Output parameters */

    IIAPI_STATUS    se_status;
    
} IIAPI_SETENVPRMPARM;


/*
** Environment logicals which may be specified through
** IIapi_setEnvParam() and their datatypes.
*/

/* Name			Description				Type       */
/* ----------------	---------------------------------	---------- */
/* dateFormat		Ingres internal date format		II_LONG */

/* charWidth		width of c columns			II_LONG */
# define IIAPI_EP_CHAR_WIDTH		1

/* txtWidth		width of text columns			II_LONG */
# define IIAPI_EP_TXT_WIDTH		2

/* int1Width		width of one byte integer		II_LONG */
# define IIAPI_EP_INT1_WIDTH		3

/* int2Width		width of two byte integer		II_LONG */
# define IIAPI_EP_INT2_WIDTH		4

/* int4Width		width of four byte integer		II_LONG */
# define IIAPI_EP_INT4_WIDTH		5

/* int8Width		width of eigth byte integer		II_LONG */
# define IIAPI_EP_INT8_WIDTH		28

/* float4Width		width of four byte float		II_LONG */
# define IIAPI_EP_FLOAT4_WIDTH		6

/* float8Width		width of eight byte float		II_LONG */
# define IIAPI_EP_FLOAT8_WIDTH		7

/* float4Precision	precision of four byte float		II_LONG */
# define IIAPI_EP_FLOAT4_PRECISION	8

/* float8Precision	precision of eight byte float		II_LONG */
# define IIAPI_EP_FLOAT8_PRECISION	9

/* moneyPrecision	money precision				II_LONG */
# define IIAPI_EP_MONEY_PRECISION	10

/* float4Style		four byte float format style.		II_CHAR */
# define IIAPI_EP_FLOAT4_STYLE		11

# define IIAPI_EPV_EXPONENTIAL		"e"
# define IIAPI_EPV_FLOATF		"f"
# define IIAPI_EPV_FLOATDEC		"n"
# define IIAPI_EPV_FLOATDECALIGN	"g"

/* float8Style		eight byte float format style		II_CHAR */
# define IIAPI_EP_FLOAT8_STYLE		12

/* numericTreatment	treatment of numeric literals		II_CHAR */
# define IIAPI_EP_NUMERIC_TREATMENT	13

# define IIAPI_EPV_DECASFLOAT		"f"
# define IIAPI_EPV_DECASDEC		"d"

/* moneySign		money sign				II_CHAR */
# define IIAPI_EP_MONEY_SIGN		14

/* moneyLort		leading or trailing money sign		II_LONG */
# define IIAPI_EP_MONEY_LORT		15

# define IIAPI_EPV_MONEY_LEAD_SIGN	0
# define IIAPI_EPV_MONEY_TRAIL_SIGN	1

/* decimalChar		decimal character			II_CHAR */
# define IIAPI_EP_DECIMAL_CHAR		16

/* mathExcp		treatment of math exception		II_CHAR */
# define IIAPI_EP_MATH_EXCP		17

# define IIAPI_EPV_RET_FATAL		"fail"
# define IIAPI_EPV_RET_WARN		"warn"
# define IIAPI_EPV_RET_IGNORE		"ignore"

/* stringTrunc		truncation of strings			II_CHAR */
# define IIAPI_EP_STRING_TRUNC		18

/* dateFormat		Ingres internal date format		II_LONG */
# define IIAPI_EP_DATE_FORMAT		19

# define IIAPI_EPV_DFRMT_US		0
# define IIAPI_EPV_DFRMT_MULTI		1
# define IIAPI_EPV_DFRMT_FINNISH	2
# define IIAPI_EPV_DFRMT_ISO		3
# define IIAPI_EPV_DFRMT_GERMAN		4
# define IIAPI_EPV_DFRMT_YMD		5
# define IIAPI_EPV_DFRMT_MDY		6
# define IIAPI_EPV_DFRMT_DMY		7
# define IIAPI_EPV_DFRMT_MLT4		8
# define IIAPI_EPV_DFRMT_ISO4		9

/* timezone		name of timezone			II_CHAR */
# define IIAPI_EP_TIMEZONE		20

/* nativeLang		native language for errors (string)	II_CHAR */
# define IIAPI_EP_NATIVE_LANG		21

/* nativeLang		native language for errors (code)	II_LONG */
# define IIAPI_EP_NATIVE_LANG_CODE	22

/* centuryBoundary	Date Century Boundary, Rollover		II_LONG */
# define IIAPI_EP_CENTURY_BOUNDARY	23

/* max segment length	BLOBS segment length			II_LONG */
# define IIAPI_EP_MAX_SEGMENT_LEN	24	

/* trace function pointer  To output trace messages to app..	II_PTR */
# define IIAPI_EP_TRACE_FUNC		25	

/* prompt function pointer  can prompt??			II_PTR */
# define IIAPI_EP_PROMPT_FUNC		26	

/* event function pointer   Database events Info to app..	II_PTR */
# define IIAPI_EP_EVENT_FUNC		27	

/*	 IIAPI_EP_INT8_WIDTH		28	See above */

/* Date alias		Datatype alias: ingresdate, ansidate	II_CHAR */
# define IIAPI_EP_DATE_ALIAS		29

# define IIAPI_EPV_INGDATE		"ingresdate"
# define IIAPI_EPV_ANSIDATE		"ansidate"

# define IIAPI_EP_BASE	IIAPI_EP_CHAR_WIDTH
# define IIAPI_EP_TOP	IIAPI_EP_DATE_ALIAS


/*
** Name: IIAPI_PROMPTPARM - Prompt Data parameters.
**
** Description:
**      This structure defines the input and output parameters to 
**      support prompt messages from the backend to the application.
**	All parameters within this structure must be prefixed with pd_.
**
**	pd_envHandle
**	    environment handle.
**
**      pd_connHandle
**	    connection handle identifying the connection.
**
**	pd_flags
**	    Prompt flgs.
**
**	pd_timeout
**	    Timeout in seconds
**
**	pd_msg_len	
**	    Prompt message length.
**
**	pd_message
**	    Prompt message.
**
**	pd_max_reply	
**	    Maximum reply length.
**
**	pd_rep_flags
**	    Response flags.
**
**	pd_rep_len
**	    Length of reply.
**
**	pd_reply
**	    Reply.
##
## History:
##	13-Oct-98 (rajus01)
##	    Created.
*/
typedef struct _IIAPI_PROMPTPARM
{

    /* Input parameters */
    II_PTR		pd_envHandle;
    II_PTR		pd_connHandle;
    II_LONG		pd_flags;	/* Prompt flags */

# define IIAPI_PR_NOECHO	0x01	/* Prompt no echo */
# define IIAPI_PR_TIMEOUT	0x02	/* Timeout available in pd_prtimeout */

    II_LONG		pd_timeout;	/* Timeout in seconds */
    II_UINT2		pd_msg_len;	/* Length of the message */
    II_CHAR		*pd_message;	/* Prompt message */
    II_UINT2		pd_max_reply;	/* Maximum length of reply */

    /* Ouput parameters */

    II_LONG		pd_rep_flags;	/* Response flags */

# define IIAPI_REPLY_TIMEOUT	0x01	/* Prompt timed out */

    II_UINT2		pd_rep_len;	/* Reply length  */
    II_CHAR		*pd_reply;	/* Reply */

} IIAPI_PROMPTPARM;


/*
** Name: IIAPI_TRACEPARM - Trace parameters.
**
** Description:
**      This structure defines the input and output parameters to 
**      output trace messages from the backend to the application.
**	All parameters within this structure must be prefixed with tr_.
**
**	tr_envHandle
**	    environment handle.
**
**      tr_connHandle
**	    connection handle identifying the connection.
**
**      tr_length
**	    output trace message length.
**    
**	tr_message
**	    output trace message.
**
##
## History:
##	11-Sep-98 (rajus01)
##	    Created.
*/

typedef struct _IIAPI_TRACEPARM
{

    /* Input parameters */
    II_PTR		tr_envHandle;
    II_PTR		tr_connHandle;
    
    /* Output parameters */
    II_INT4		tr_length;
    II_CHAR		*tr_message;
    
} IIAPI_TRACEPARM;


/*
** Name: IIAPI_EVENTPARM - Database events info.
**
** Description:
**      This structure defines the input and output parameters to 
**      output Database events info to the application.
**      All parameters within this structure must be prefixed with ev_.
**
**      ev_envHandle
**	    environment handle.
**      ev_connHandle
**	    connection handle identifying the connection.
**      ev_eventName
**	    The name of the event caught.
**      ev_eventOwner
**	    The owner of the event caught.
**      ev_eventDB
**          the database of the event caught.
**      ev_eventTime
**          the time the event occurred; a IIAPI_DTE_TYPE data value.
**
##
## History:
##	27-Jan-99 (rajus01)
##	    Created.
*/

typedef struct _IIAPI_EVENTPARM
{

    /* Input parameters */
    II_PTR		ev_envHandle;
    II_PTR		ev_connHandle;

    /* Output parameters */
    II_CHAR		*ev_eventName;
    II_CHAR 		*ev_eventOwner;
    II_CHAR 		*ev_eventDB;
    IIAPI_DATAVALUE	ev_eventTime;
    
} IIAPI_EVENTPARM;


/*
** Name: IIAPI_SETCONPRMPARM - input parameter for IIapi_setConnectParm()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_setConnectParm().  All parameters within
**      this structure must be prefixed with sc_.
**
**      sc_genParm
**          generic API parameters
**      sc_connHandle
**          connection handle identifying the connection for which the
**          parameter is being set.
**	 sc_paramID
**	     numeric ID of the parameter being set (defined below).
**	 sc_paramValue
**	     pointer to value of the parameter being set (type defined below).
**	     all II_CHAR values must be null terminated strings.
##
## History:
##	14-Nov-94 (gordy)
##	    Created.
##	10-May-95 (gordy)
##	    Cleanup work on connection parms.
##      19-Aug-2002(wansh01)
##           Added IIAPI_CP_LOGIN_LOCAL for login type local. 
##	15-Mar-04 (gordy)
##	    Added IIAPI_CP_INT8_WIDTH.
*/

typedef struct _IIAPI_SETCONPRMPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   sc_genParm;

    /* Specific input+output available at function return */

    II_PTR 	    sc_connHandle;
    
    /* Specific input parameters: */
    
    II_LONG	    sc_paramID;
    II_PTR	    sc_paramValue;
    
} IIAPI_SETCONPRMPARM;


/*
** Connection Parameters which may be specified through
** IIapi_setConnectParm() and their datatypes.
*/

/* Name			Description				Type       */
/* ----------------	---------------------------------	---------- */

/* charWidth		width of c columns			II_LONG */
# define IIAPI_CP_CHAR_WIDTH		1

/* txtWidth		width of text columns			II_LONG */
# define IIAPI_CP_TXT_WIDTH		2

/* int1Width		width of one byte integer		II_LONG */
# define IIAPI_CP_INT1_WIDTH		3

/* int2Width		width of two byte integer		II_LONG */
# define IIAPI_CP_INT2_WIDTH		4

/* int4Width		width of four byte integer		II_LONG */
# define IIAPI_CP_INT4_WIDTH		5

/* int8Width		width of eight byte integer		II_LONG */
# define IIAPI_CP_INT8_WIDTH		39

/* float4Width		width of four byte float		II_LONG */
# define IIAPI_CP_FLOAT4_WIDTH		6

/* float8Width		width of eight byte float		II_LONG */
# define IIAPI_CP_FLOAT8_WIDTH		7

/* float4Precision	precision of four byte float		II_LONG */
# define IIAPI_CP_FLOAT4_PRECISION	8

/* float8Precision	precision of eight byte float		II_LONG */
# define IIAPI_CP_FLOAT8_PRECISION	9

/* moneyPrecision	money precision				II_LONG */
# define IIAPI_CP_MONEY_PRECISION	10

/* float4Style		four byte float format style.		II_CHAR */
# define IIAPI_CP_FLOAT4_STYLE		11

# define IIAPI_CPV_EXPONENTIAL		IIAPI_EPV_EXPONENTIAL
# define IIAPI_CPV_FLOATF		IIAPI_EPV_FLOATF
# define IIAPI_CPV_FLOATDEC		IIAPI_EPV_FLOATDEC
# define IIAPI_CPV_FLOATDECALIGN	IIAPI_EPV_FLOATDECALIGN

/* float8Style		eight byte float format style		II_CHAR */
# define IIAPI_CP_FLOAT8_STYLE		12

/* numericTreatment	treatment of numeric literals		II_CHAR */
# define IIAPI_CP_NUMERIC_TREATMENT	13

# define IIAPI_CPV_DECASFLOAT		IIAPI_EPV_DECASFLOAT
# define IIAPI_CPV_DECASDEC		IIAPI_EPV_DECASDEC

/* moneySign		money sign				II_CHAR */
# define IIAPI_CP_MONEY_SIGN		14

/* moneyLort		leading or trailing money sign		II_LONG */
# define IIAPI_CP_MONEY_LORT		15

# define IIAPI_CPV_MONEY_LEAD_SIGN	IIAPI_EPV_MONEY_LEAD_SIGN
# define IIAPI_CPV_MONEY_TRAIL_SIGN	IIAPI_EPV_MONEY_TRAIL_SIGN

/* decimalChar		decimal character			II_CHAR */
# define IIAPI_CP_DECIMAL_CHAR		16

/* mathExcp		treatment of math exception		II_CHAR */
# define IIAPI_CP_MATH_EXCP		17

# define IIAPI_CPV_RET_FATAL		IIAPI_EPV_RET_FATAL
# define IIAPI_CPV_RET_WARN		IIAPI_EPV_RET_WARN
# define IIAPI_CPV_RET_IGNORE		IIAPI_EPV_RET_IGNORE

/* stringTrunc		truncation of strings			II_CHAR */
# define IIAPI_CP_STRING_TRUNC		18

/* dateFormat		Ingres internal date format		II_LONG */
# define IIAPI_CP_DATE_FORMAT		19

# define IIAPI_CPV_DFRMT_US		IIAPI_EPV_DFRMT_US
# define IIAPI_CPV_DFRMT_MULTI		IIAPI_EPV_DFRMT_MULTI
# define IIAPI_CPV_DFRMT_FINNISH	IIAPI_EPV_DFRMT_FINNISH
# define IIAPI_CPV_DFRMT_ISO		IIAPI_EPV_DFRMT_ISO
# define IIAPI_CPV_DFRMT_GERMAN		IIAPI_EPV_DFRMT_GERMAN
# define IIAPI_CPV_DFRMT_YMD		IIAPI_EPV_DFRMT_YMD
# define IIAPI_CPV_DFRMT_MDY		IIAPI_EPV_DFRMT_MDY
# define IIAPI_CPV_DFRMT_DMY		IIAPI_EPV_DFRMT_DMY
# define IIAPI_CPV_DFRMT_MLT4		IIAPI_EPV_DFRMT_MLT4
# define IIAPI_CPV_DFRMT_ISO4		IIAPI_EPV_DFRMT_ISO4

/* timezone		name of timezone			II_CHAR */
# define IIAPI_CP_TIMEZONE		20

/* secondaryInx		default secondary index structure	II_CHAR */
# define IIAPI_CP_SECONDARY_INX		21

# define IIAPI_CPV_ISAM			"isam"
# define IIAPI_CPV_CISAM		"cisam"
# define IIAPI_CPV_BTREE		"btree"
# define IIAPI_CPV_CBTREE		"cbtree"
# define IIAPI_CPV_HEAP			"heap"
# define IIAPI_CPV_CHEAP		"cheap"
# define IIAPI_CPV_HASH			"hash"
# define IIAPI_CPV_CHASH		"chash"

/* resultTbl		default structure for result table.	II_CHAR */
# define IIAPI_CP_RESULT_TBL		22

/* serverType		type of server function desired.	II_LONG */
# define IIAPI_CP_SERVER_TYPE		23

# define IIAPI_CPV_SVNORMAL		0
# define IIAPI_CPV_MONITOR		1

/* nativeLang		native language for errors (string)	II_CHAR */
# define IIAPI_CP_NATIVE_LANG		24

/* nativeLang		native language for errors (code)	II_LONG */
# define IIAPI_CP_NATIVE_LANG_CODE	25

/* application		-A option				II_LONG */
# define IIAPI_CP_APPLICATION		26

/* appID		application ID or password		II_CHAR */
# define IIAPI_CP_APP_ID		27

/* groupID		group ID				II_CHAR */
# define IIAPI_CP_GROUP_ID		28

/* effectiveUser	effective user name			II_CHAR */
# define IIAPI_CP_EFFECTIVE_USER	29

/* exclusiveSysUpdate	update system catalog: exclusive lock	II_BOOL */
# define IIAPI_CP_EXCLUSIVE_SYS_UPDATE	30

/* sharedSysUpdate	update system catalog: shared lock	II_BOOL */
# define IIAPI_CP_SHARED_SYS_UPDATE	31

/* exclusiveLock	lock the DB exclusively			II_BOOL */
# define IIAPI_CP_EXCLUSIVE_LOCK	32

/* waitLock		wait on DB				II_BOOL */
# define IIAPI_CP_WAIT_LOCK		33

/* miscParam		miscellaneous parameters		II_CHAR */
# define IIAPI_CP_MISC_PARM		34

/* gatewayParm		miscellaneous gateway parmeters		II_CHAR */
# define IIAPI_CP_GATEWAY_PARM		35

/* dbmsPassword		DBMS user password			II_CHAR */
# define IIAPI_CP_DBMS_PASSWORD		36	

/* centuryBoundary	Date Century Boundary, Rollover		II_LONG */
# define IIAPI_CP_CENTURY_BOUNDARY	37

/*login type local	 					II_BOOL */
# define IIAPI_CP_LOGIN_LOCAL		38

/*	 IIAPI_CP_INT8_WIDTH		39	See above. */

/* Date alias		Datatype alias: ingresdate, ansidate	II_CHAR */
# define IIAPI_CP_DATE_ALIAS		40

# define IIAPI_CPV_INGDATE		IIAPI_EPV_INGDATE
# define IIAPI_CPV_ANSIDATE		IIAPI_EPV_ANSIDATE

# define IIAPI_CP_BASE	IIAPI_CP_CHAR_WIDTH
# define IIAPI_CP_TOP	IIAPI_CP_DATE_ALIAS


/*
** Name: IIAPI_SETDESCRPARM - input parameter for IIapi_setDescriptor()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_setDescriptor().  All parameters within
**      this structure must be prefixed with sd_.
**
**      sd_genParm
**          common API parameters
**      sd_stmtHandle
**          statement handle
**      sd_descriptorCount
**          number of tuple descriptor
**      sd_descriptor
**          tuple descriptor
##
## History:
##      01-sep-93 (ctham)
##          creation
*/

typedef struct _IIAPI_SETDESCRPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM   sd_genParm;
    
    /* Specific input parameters: */
    
    II_PTR			sd_stmtHandle;
    II_INT2			sd_descriptorCount;
    IIAPI_DESCRIPTOR II_FAR	*sd_descriptor;
    
} IIAPI_SETDESCRPARM;


/*
** Name: IIAPI_TERMPARM - input parameter for IIapi_terminate()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_intialize().  All parameters within
**      this structure must be prefixed with tm_.
**
**	 tm_status
**	     the status of IIapi_terminate().
##
## History:
##      17-may-94 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
*/

typedef struct _IIAPI_TERMPARM
{
    
    /* Specific output parameters */
    
    IIAPI_STATUS	tm_status;
    
} IIAPI_TERMPARM;


/*
** Name: IIAPI_WAITPARM - input parameter for IIapi_wait()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_wait().  All parameters within
**      this structure must be prefixed with wt_.
**
**      wt_timeout
**          timeout in milli-seconds: -1 = none
##
## History:
##      01-sep-93 (ctham)
##          creation
##	23-Jan-95 (gordy)
##	    Cleaned up error handling.
*/

typedef struct _IIAPI_WAITPARM
{
    
    /* Input parameters: */
    
    II_LONG		wt_timeout;
    
    /* Output parameters: */

    IIAPI_STATUS	wt_status;

} IIAPI_WAITPARM;


/*
** Name: IIAPI_XASTARTPARM - input parameter for IIapi_xaStart()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_xaStart().  All parameters within
**      this structure must be prefixed with xs_.
**
**      xs_genParm
**          common API parameters
**      xs_connHandle
**          connection handle identifying the connection which will
**	    be associated with the XA transaction
**      xs_tranID
**          unique transaction ID
**	xs_flags
**	    XA flags
**	xs_tranHandle
**	    new transaction handle
##
## History:
##	07-Jul-06 (gordy)
##	    Created.
*/

typedef struct _IIAPI_XASTARTPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	xs_genParm;
    
    /* Input parameters: */
    
    II_PTR		xs_connHandle;
    IIAPI_TRAN_ID	xs_tranID;
    II_ULONG		xs_flags;

# define IIAPI_XA_JOIN		0x00200000	/* Join existing transaction */
    
    /* Specific output parameters: */
    
    II_PTR		xs_tranHandle;
    
} IIAPI_XASTARTPARM;


/*
** Name: IIAPI_XAENDPARM - input parameter for IIapi_xaEnd()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_xaEnd().  All parameters within
**      this structure must be prefixed with xe_.
**
**      xe_genParm
**          common API parameters
**      xe_connHandle
**          connection handle identifying the connection which 
**	    is associated with the XA transaction
**      xe_tranID
**          unique transaction ID
**	xe_flags
**	    XA flags
##
## History:
##	07-Jul-06 (gordy)
##	    Created.
*/

typedef struct _IIAPI_XAENDPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	xe_genParm;
    
    /* Input parameters: */
    
    II_PTR		xe_connHandle;
    IIAPI_TRAN_ID	xe_tranID;
    II_ULONG		xe_flags;

# define IIAPI_XA_FAIL		0x20000000	/* Transaction failure */
    
} IIAPI_XAENDPARM;


/*
** Name: IIAPI_XAPREPPARM - input parameter for IIapi_xaPrepare()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_xaPrepare().  All parameters within
**      this structure must be prefixed with xp_.
**
**      xp_genParm
**          common API parameters
**      xp_connHandle
**          connection handle identifying the connection to use
**	    to prepare the XA transaction
**      xp_tranID
**          unique transaction ID
**	xp_flags
**	    XA flags
##
## History:
##	07-Jul-06 (gordy)
##	    Created.
*/

typedef struct _IIAPI_XAPREPPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	xp_genParm;
    
    /* Input parameters: */
    
    II_PTR		xp_connHandle;
    IIAPI_TRAN_ID	xp_tranID;
    II_ULONG		xp_flags;
    
} IIAPI_XAPREPPARM;


/*
** Name: IIAPI_XACOMMITPARM - input parameter for IIapi_xaCommit()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_xaCommit().  All parameters within
**      this structure must be prefixed with xc_.
**
**      xc_genParm
**          common API parameters
**      xc_connHandle
**          connection handle identifying the connection to use
**	    to commit the XA transaction
**      xc_tranID
**          unique transaction ID
**	xc_flags
**	    XA flags
##
## History:
##	07-Jul-06 (gordy)
##	    Created.
*/

typedef struct _IIAPI_XACOMMITPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	xc_genParm;
    
    /* Input parameters: */
    
    II_PTR		xc_connHandle;
    IIAPI_TRAN_ID	xc_tranID;
    II_ULONG		xc_flags;

# define IIAPI_XA_1PC		0x40000000	/* One-phase commit */
    
} IIAPI_XACOMMITPARM;


/*
** Name: IIAPI_XAROLLPARM - input parameter for IIapi_xaRollback()
**
** Description:
**      This structure defines the input and output parameters for
**      the API function IIapi_xaRollback().  All parameters within
**      this structure must be prefixed with xr_.
**
**      xr_genParm
**          common API parameters
**      xp_connHandle
**          connection handle identifying the connection to use
**	    to rollback the XA transaction
**      xr_tranID
**          unique transaction ID
**	xr_flags
**	    XA flags
##
## History:
##	07-Jul-06 (gordy)
##	    Created.
*/

typedef struct _IIAPI_XAROLLPARM
{
    
    /* Common input+output parameters: */
    
    IIAPI_GENPARM	xr_genParm;
    
    /* Input parameters: */
    
    II_PTR		xr_connHandle;
    IIAPI_TRAN_ID	xr_tranID;
    II_ULONG		xr_flags;

} IIAPI_XAROLLPARM;


/*
** API function prototypes - defines API functions and their arguments.
*/
II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_abort( IIAPI_ABORTPARM II_FAR *abortParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_autocommit( IIAPI_AUTOPARM II_FAR *autoParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_batch( IIAPI_BATCHPARM II_FAR *batchParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_cancel( IIAPI_CANCELPARM II_FAR *cancelParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_catchEvent( IIAPI_CATCHEVENTPARM II_FAR *catchEventParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_close( IIAPI_CLOSEPARM II_FAR *closeParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_commit( IIAPI_COMMITPARM II_FAR *commitParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_connect( IIAPI_CONNPARM II_FAR *connParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_convertData( IIAPI_CONVERTPARM II_FAR *convertParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_disconnect( IIAPI_DISCONNPARM II_FAR *disconnParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_formatData( IIAPI_FORMATPARM II_FAR *formatParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getColumnInfo( IIAPI_GETCOLINFOPARM II_FAR *getColInfoParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getColumns( IIAPI_GETCOLPARM II_FAR *getColParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getCopyMap( IIAPI_GETCOPYMAPPARM II_FAR *getCopyMapParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getDescriptor( IIAPI_GETDESCRPARM II_FAR *getDescrParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getErrorInfo( IIAPI_GETEINFOPARM II_FAR *getEInfoParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getEvent( IIAPI_GETEVENTPARM II_FAR *getEventParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_getQueryInfo( IIAPI_GETQINFOPARM II_FAR *getQInfoParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_initialize( IIAPI_INITPARM II_FAR *initParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_modifyConnect( IIAPI_MODCONNPARM II_FAR *modifyConnParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_position( IIAPI_POSPARM II_FAR *posParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_prepareCommit( IIAPI_PREPCMTPARM II_FAR *prepCmtParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_putColumns( IIAPI_PUTCOLPARM II_FAR *putColParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_putParms( IIAPI_PUTPARMPARM II_FAR *putParmParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_query( IIAPI_QUERYPARM II_FAR *queryParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_registerXID( IIAPI_REGXIDPARM II_FAR *regXIDParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_releaseEnv( IIAPI_RELENVPARM II_FAR *relEnvParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_releaseXID( IIAPI_RELXIDPARM II_FAR *relXIDParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_rollback( IIAPI_ROLLBACKPARM II_FAR *rollbackParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_savePoint( IIAPI_SAVEPTPARM II_FAR *savePtParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_scroll( IIAPI_SCROLLPARM II_FAR *scrollParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_setConnectParam( IIAPI_SETCONPRMPARM II_FAR *setConPrmParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_setDescriptor( IIAPI_SETDESCRPARM II_FAR *setDescrParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_setEnvParam( IIAPI_SETENVPRMPARM II_FAR *setEnvParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_terminate( IIAPI_TERMPARM II_FAR *termParm );

II_EXTERN II_VOID II_FAR II_EXPORT 
IIapi_wait( IIAPI_WAITPARM II_FAR *waitParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaStart( IIAPI_XASTARTPARM II_FAR *startParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaEnd( IIAPI_XAENDPARM II_FAR *endParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaPrepare( IIAPI_XAPREPPARM II_FAR *prepParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaCommit( IIAPI_XACOMMITPARM II_FAR *commitParm );

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaRollback( IIAPI_XAROLLPARM II_FAR *rollbackParm );

# endif				/* __IIAPI_H__ */
