/*
** Copyright (c) 2004 Ingres Corporation
*/





# include <compat.h>
# include <cm.h>
# include <me.h>
# include <si.h>
# include <st.h>
# include <iiapi.h>
# include "aitfunc.h"
# include "aitdef.h"
# include "aitsym.h"
# include "aitproc.h"





/*
** Name: aitsym.c
**
** Description:
**	API symbol tables.
**
** History:
**	24-Mar-95 (gordy)
**	    Created from aitterm.h
**	10-May-95 (feige)
**	    Added CP_SYMSTR_TBL and CP_SYMINT_TBL.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	22-May-95 (gordy)
**	    Fixed typo in symbol.
**	13-Jun-95 (gordy)
**	    Finalized API datatypes.
**	14-Jan-96 (emmag)
**	    Integration of Jasmine changes.
**	 9-Sep-96 (gordy)
**	    Added IIAPI_CP_DBMS_PASSWORD and IIAPI_CP_CENTURY_BOUNDARY.
**	13-Mar-97 (gordy)
**	    API Version 2: connection types.
**	19-Aug-98 (rajus01)
**	    Added se_paramID_TBL, EP_SYMINT_TBL, EP_FLOAT4_STYLE_TBL, 
**	    EP_NUMERIC_TREATMENT_TBL, EP_MATH_EXCP_TBL,EP_SYMSTR_TBL
**	    definitions. 
**	03-Feb-99 (rajus01)
**	    Added support for handling callback functions in 
**	    IIAPI_SETENVPRMPARM. Added apitrace_func(),
**	    apiprompt_func(), apievent_func().
**	15-Mar-04 (gordy)
**	    Added support for 8 byte integers: IIAPI_CP_INT8_WIDTH,
**	    IIAPI_EP_INT8_WIDTH.
**      07-Sep-5 (crogr01)
**	    Completed support for MULTINATIONAL4 and ISO4 date formats.
**	16-Sep-2005 (hanje04)
** 	    Added missing comma from MULTINATIONAL4 change.
**	12-Sep-2005 (schka24)
**	    Add missing comma after iso4 entry.
**      17-Aug-2010 (thich01)
**          Add geospatial types to IIAPI types.
*/

static II_VOID apitrace_func( IIAPI_TRACEPARM *trace_parm );
static II_VOID apiprompt_func( IIAPI_PROMPTPARM *prompt_parm );
static II_VOID apievent_func( IIAPI_EVENTPARM	*event_parm );


GLOBALDEF AIT_SYMINT	in_version_TBL[MAX_in_version] =
{
       	{"IIAPI_VERSION_1",			IIAPI_VERSION_1},
	{"IIAPI_VERSION_2",			IIAPI_VERSION_2},
	{"IIAPI_VERSION",			IIAPI_VERSION}
};

GLOBALDEF AIT_SYMINT	sc_paramID_TBL[MAX_sc_paramID] =
{
	{"IIAPI_CP_CHAR_WIDTH",		IIAPI_CP_CHAR_WIDTH},		/* 1 */
	{"IIAPI_CP_TXT_WIDTH",		IIAPI_CP_TXT_WIDTH},		/* 2 */
	{"IIAPI_CP_INT1_WIDTH",		IIAPI_CP_INT1_WIDTH},		/* 3 */
	{"IIAPI_CP_INT2_WIDTH",		IIAPI_CP_INT2_WIDTH},		/* 4 */
	{"IIAPI_CP_INT4_WIDTH",		IIAPI_CP_INT4_WIDTH},		/* 5 */
	{"IIAPI_CP_INT8_WIDTH",		IIAPI_CP_INT8_WIDTH},		/* 6 */
	{"IIAPI_CP_FLOAT4_WIDTH",	IIAPI_CP_FLOAT4_WIDTH},		/* 7 */
	{"IIAPI_CP_FLOAT8_WIDTH",	IIAPI_CP_FLOAT8_WIDTH},		/* 8 */
	{"IIAPI_CP_FLOAT4_PRECISION",	IIAPI_CP_FLOAT4_PRECISION},	/* 9 */
	{"IIAPI_CP_FLOAT8_PRECISION",	IIAPI_CP_FLOAT8_PRECISION},	/* 10 */
	{"IIAPI_CP_MONEY_PRECISION",	IIAPI_CP_MONEY_PRECISION},	/* 10 */
	{"IIAPI_CP_FLOAT4_STYLE",	IIAPI_CP_FLOAT4_STYLE},		/* 21 */
	{"IIAPI_CP_FLOAT8_STYLE",	IIAPI_CP_FLOAT8_STYLE},		/* 32 */
	{"IIAPI_CP_NUMERIC_TREATMENT",	IIAPI_CP_NUMERIC_TREATMENT},	/* 43 */
	{"IIAPI_CP_MONEY_SIGN",		IIAPI_CP_MONEY_SIGN},		/* 54 */
	{"IIAPI_CP_MONEY_LORT",		IIAPI_CP_MONEY_LORT},		/* 65 */
	{"IIAPI_CP_DECIMAL_CHAR",	IIAPI_CP_DECIMAL_CHAR},		/* 76 */
	{"IIAPI_CP_MATH_EXCP",		IIAPI_CP_MATH_EXCP},		/* 18 */
	{"IIAPI_CP_STRING_TRUNC",	IIAPI_CP_STRING_TRUNC},		/* 19 */
	{"IIAPI_CP_DATE_FORMAT",	IIAPI_CP_DATE_FORMAT},		/* 20 */
	{"IIAPI_CP_TIMEZONE",		IIAPI_CP_TIMEZONE},		/* 21 */
	{"IIAPI_CP_SECONDARY_INX",	IIAPI_CP_SECONDARY_INX},	/* 22 */
	{"IIAPI_CP_RESULT_TBL",		IIAPI_CP_RESULT_TBL},		/* 23 */
	{"IIAPI_CP_SERVER_TYPE",	IIAPI_CP_SERVER_TYPE},		/* 24 */
	{"IIAPI_CP_NATIVE_LANG",	IIAPI_CP_NATIVE_LANG},		/* 25 */
	{"IIAPI_CP_NATIVE_LANG_CODE",	IIAPI_CP_NATIVE_LANG_CODE},	/* 26 */
	{"IIAPI_CP_APPLICATION",	IIAPI_CP_APPLICATION},		/* 27 */
	{"IIAPI_CP_APP_ID",		IIAPI_CP_APP_ID},		/* 28 */
	{"IIAPI_CP_GROUP_ID",		IIAPI_CP_GROUP_ID},		/* 29 */
	{"IIAPI_CP_EFFECTIVE_USER",	IIAPI_CP_EFFECTIVE_USER},	/* 30 */
	{"IIAPI_CP_EXCLUSIVE_SYS_UPDATE",
					IIAPI_CP_EXCLUSIVE_SYS_UPDATE},	/* 31 */
	{"IIAPI_CP_SHARED_SYS_UPDATE",	IIAPI_CP_SHARED_SYS_UPDATE},	/* 32 */
	{"IIAPI_CP_EXCLUSIVE_LOCK",	IIAPI_CP_EXCLUSIVE_LOCK},	/* 33 */
	{"IIAPI_CP_WAIT_LOCK",		IIAPI_CP_WAIT_LOCK},		/* 34 */
	{"IIAPI_CP_MISC_PARM",		IIAPI_CP_MISC_PARM},		/* 35 */
	{"IIAPI_CP_GATEWAY_PARM",	IIAPI_CP_GATEWAY_PARM},		/* 36 */
	{"IIAPI_CP_DBMS_PASSWORD",	IIAPI_CP_DBMS_PASSWORD},	/* 37 */
	{"IIAPI_CP_CENTURY_BOUNDARY",	IIAPI_CP_CENTURY_BOUNDARY}	/* 38 */
};

GLOBALDEF AIT_SYMINT	se_paramID_TBL[MAX_se_paramID] =
{
	{"IIAPI_EP_CHAR_WIDTH",		IIAPI_EP_CHAR_WIDTH},		/* 1 */
	{"IIAPI_EP_TXT_WIDTH",		IIAPI_EP_TXT_WIDTH},		/* 2 */
	{"IIAPI_EP_INT1_WIDTH",		IIAPI_EP_INT1_WIDTH},		/* 3 */
	{"IIAPI_EP_INT2_WIDTH",		IIAPI_EP_INT2_WIDTH},		/* 4 */
	{"IIAPI_EP_INT4_WIDTH",		IIAPI_EP_INT4_WIDTH},		/* 5 */
	{"IIAPI_EP_INT8_WIDTH",		IIAPI_EP_INT8_WIDTH},		/* 6 */
	{"IIAPI_EP_FLOAT4_WIDTH",	IIAPI_EP_FLOAT4_WIDTH},		/* 7 */
	{"IIAPI_EP_FLOAT8_WIDTH",	IIAPI_EP_FLOAT8_WIDTH},		/* 8 */
	{"IIAPI_EP_FLOAT4_PRECISION",	IIAPI_EP_FLOAT4_PRECISION},	/* 9 */
	{"IIAPI_EP_FLOAT8_PRECISION",	IIAPI_EP_FLOAT8_PRECISION},	/* 10 */
	{"IIAPI_EP_MONEY_PRECISION",	IIAPI_EP_MONEY_PRECISION},	/* 11 */
	{"IIAPI_EP_FLOAT4_STYLE",	IIAPI_EP_FLOAT4_STYLE},		/* 12 */
	{"IIAPI_EP_FLOAT8_STYLE",	IIAPI_EP_FLOAT8_STYLE},		/* 13 */
	{"IIAPI_EP_NUMERIC_TREATMENT",	IIAPI_EP_NUMERIC_TREATMENT},	/* 14 */
	{"IIAPI_EP_MONEY_SIGN",		IIAPI_EP_MONEY_SIGN},		/* 15 */
	{"IIAPI_EP_MONEY_LORT",		IIAPI_EP_MONEY_LORT},		/* 16 */
	{"IIAPI_EP_DECIMAL_CHAR",	IIAPI_EP_DECIMAL_CHAR},		/* 17 */
	{"IIAPI_EP_MATH_EXCP",		IIAPI_EP_MATH_EXCP},		/* 18 */
	{"IIAPI_EP_STRING_TRUNC",	IIAPI_EP_STRING_TRUNC},		/* 19 */
	{"IIAPI_EP_DATE_FORMAT",	IIAPI_EP_DATE_FORMAT},		/* 20 */
	{"IIAPI_EP_TIMEZONE",		IIAPI_EP_TIMEZONE},		/* 21 */
	{"IIAPI_EP_NATIVE_LANG",	IIAPI_EP_NATIVE_LANG},		/* 22 */
	{"IIAPI_EP_NATIVE_LANG_CODE",	IIAPI_EP_NATIVE_LANG_CODE},	/* 23 */
	{"IIAPI_EP_CENTURY_BOUNDARY",	IIAPI_EP_CENTURY_BOUNDARY},	/* 24 */
	{"IIAPI_EP_MAX_SEGMENT_LEN",	IIAPI_EP_MAX_SEGMENT_LEN},	/* 25 */
	{"IIAPI_EP_TRACE_FUNC",		IIAPI_EP_TRACE_FUNC},		/* 26 */
	{"IIAPI_EP_PROMPT_FUNC",	IIAPI_EP_PROMPT_FUNC},		/* 27 */
	{"IIAPI_EP_EVENT_FUNC",		IIAPI_EP_EVENT_FUNC}		/* 28 */
};

GLOBALDEF AIT_SYMINT	co_apiLevel_TBL[MAX_co_apiLevel] =
{
	{"IIAPI_LEVEL_0",		IIAPI_LEVEL_0},			/* 0 */
	{"IIAPI_LEVEL_1",		IIAPI_LEVEL_1}			/* 1 */
};

GLOBALDEF AIT_SYMINT	co_type_TBL[MAX_co_type] =
{
	{"IIAPI_CT_SQL",		IIAPI_CT_SQL},			/* 0 */
	{"IIAPI_CT_NS",			IIAPI_CT_NS}			/* 1 */
};

GLOBALDEF AIT_SYMINT	CP_MONEY_LORT_TBL[MAX_CP_MONEY_LORT] =
{
	{"IIAPI_CPV_MONEY_LEAD_SIGN",	IIAPI_CPV_MONEY_LEAD_SIGN},	/* 0 */
	{"IIAPI_CPV_MONEY_TRAIL_SIGN",	IIAPI_CPV_MONEY_TRAIL_SIGN}	/* 1 */
};

GLOBALDEF AIT_SYMINT	CP_DATE_FORMAT_TBL[MAX_CP_DATE_FORMAT] =
{
	{"IIAPI_CPV_DFRMT_US",		IIAPI_CPV_DFRMT_US},		/* 0 */
	{"IIAPI_CPV_DFRMT_MULTI",	IIAPI_CPV_DFRMT_MULTI},		/* 1 */
	{"IIAPI_CPV_DFRMT_FINNISH",	IIAPI_CPV_DFRMT_FINNISH},	/* 2 */
	{"IIAPI_CPV_DFRMT_ISO",		IIAPI_CPV_DFRMT_ISO},		/* 3 */
	{"IIAPI_CPV_DFRMT_GERMAN",	IIAPI_CPV_DFRMT_GERMAN},	/* 4 */
	{"IIAPI_CPV_DFRMT_YMD",		IIAPI_CPV_DFRMT_YMD},		/* 5 */
	{"IIAPI_CPV_DFRMT_MDY",		IIAPI_CPV_DFRMT_MDY},		/* 6 */
	{"IIAPI_CPV_DFRMT_DMY",		IIAPI_CPV_DFRMT_DMY},		/* 7 */
	{"IIAPI_CPV_DFRMT_MLT4",	IIAPI_CPV_DFRMT_MLT4},		/* 8 */
	{"IIAPI_CPV_DFRMT_ISO4",	IIAPI_CPV_DFRMT_ISO4}		/* 9 */
};

GLOBALDEF AIT_SYMINT	CP_SERVER_TYPE_TBL[MAX_CP_SERVER_TYPE] =
{
	{"IIAPI_CPV_SVNORMAL",		IIAPI_CPV_SVNORMAL},		/* 1 */
	{"IIAPI_CPV_MONITOR",		IIAPI_CPV_MONITOR}		/* 2 */
};

GLOBALDEF AIT_SYMINT	qy_queryType_TBL[MAX_qy_queryType] =
{
	{"IIAPI_QT_QUERY",             	IIAPI_QT_QUERY},
	{"IIAPI_QT_SELECT_SINGLETON",	IIAPI_QT_SELECT_SINGLETON},
	{"IIAPI_QT_OPEN",               IIAPI_QT_OPEN},
	{"IIAPI_QT_CURSOR_DELETE",      IIAPI_QT_CURSOR_DELETE},
	{"IIAPI_QT_CURSOR_UPDATE",      IIAPI_QT_CURSOR_UPDATE},
	{"IIAPI_QT_DEF_REPEAT_QUERY",   IIAPI_QT_DEF_REPEAT_QUERY},
	{"IIAPI_QT_EXEC_REPEAT_QUERY",  IIAPI_QT_EXEC_REPEAT_QUERY},
	{"IIAPI_QT_EXEC_PROCEDURE",     IIAPI_QT_EXEC_PROCEDURE},

	{"IIAPI_QT_ALTER_GROUP",        IIAPI_QT_ALTER_GROUP},
	{"IIAPI_QT_ALTER_ROLE",         IIAPI_QT_ALTER_ROLE},
	{"IIAPI_QT_ALTER_TABLE",        IIAPI_QT_ALTER_TABLE},
	{"IIAPI_QT_COPY_FROM",          IIAPI_QT_COPY_FROM},
	{"IIAPI_QT_COPY_INTO",          IIAPI_QT_COPY_INTO},
	{"IIAPI_QT_CREATE_DBEVENT",     IIAPI_QT_CREATE_DBEVENT},
	{"IIAPI_QT_CREATE_GROUP",       IIAPI_QT_CREATE_GROUP},
	{"IIAPI_QT_CREATE_INDEX",       IIAPI_QT_CREATE_INDEX},
	{"IIAPI_QT_CREATE_INTEGRITY",   IIAPI_QT_CREATE_INTEGRITY},
	{"IIAPI_QT_CREATE_PROCEDURE",   IIAPI_QT_CREATE_PROCEDURE},
	{"IIAPI_QT_CREATE_ROLE",        IIAPI_QT_CREATE_ROLE},
	{"IIAPI_QT_CREATE_RULE",        IIAPI_QT_CREATE_RULE},
	{"IIAPI_QT_CREATE_TABLE",       IIAPI_QT_CREATE_TABLE},
	{"IIAPI_QT_CREATE_VIEW",        IIAPI_QT_CREATE_VIEW},
	{"IIAPI_QT_DELETE",             IIAPI_QT_DELETE},
	{"IIAPI_QT_DESCRIBE",           IIAPI_QT_DESCRIBE},
	{"IIAPI_QT_DIRECT_EXEC",        IIAPI_QT_DIRECT_EXEC},
	{"IIAPI_QT_DROP_DBEVENT",       IIAPI_QT_DROP_DBEVENT},
	{"IIAPI_QT_DROP_GROUP",         IIAPI_QT_DROP_GROUP},
	{"IIAPI_QT_DROP_INDEX",         IIAPI_QT_DROP_INDEX},
	{"IIAPI_QT_DROP_INTEGRITY",     IIAPI_QT_DROP_INTEGRITY},
	{"IIAPI_QT_DROP_PERMIT",        IIAPI_QT_DROP_PERMIT},
	{"IIAPI_QT_DROP_PROCEDURE",     IIAPI_QT_DROP_PROCEDURE},
	{"IIAPI_QT_DROP_ROLE",          IIAPI_QT_DROP_ROLE},
	{"IIAPI_QT_DROP_RULE",          IIAPI_QT_DROP_RULE},
	{"IIAPI_QT_DROP_TABLE",         IIAPI_QT_DROP_TABLE},
	{"IIAPI_QT_DROP_VIEW",          IIAPI_QT_DROP_VIEW},
	{"IIAPI_QT_EXEC",               IIAPI_QT_EXEC},
	{"IIAPI_QT_EXEC_IMMEDIATE",     IIAPI_QT_EXEC_IMMEDIATE},
	{"IIAPI_QT_GRANT",              IIAPI_QT_GRANT},
	{"IIAPI_QT_GET_DBEVENT",        IIAPI_QT_GET_DBEVENT},
	{"IIAPI_QT_INSERT_INTO",        IIAPI_QT_INSERT_INTO},
	{"IIAPI_QT_MODIFY",             IIAPI_QT_MODIFY},
	{"IIAPI_QT_PREPARE",            IIAPI_QT_PREPARE},
	{"IIAPI_QT_RAISE_DBEVENT",      IIAPI_QT_RAISE_DBEVENT},
	{"IIAPI_QT_REGISTER_DBEVENT",   IIAPI_QT_REGISTER_DBEVENT},
	{"IIAPI_QT_REMOVE_DBEVENT",     IIAPI_QT_REMOVE_DBEVENT},
	{"IIAPI_QT_REVOKE",             IIAPI_QT_REVOKE},
	{"IIAPI_QT_SAVE_TABLE",         IIAPI_QT_SAVE_TABLE},
	{"IIAPI_QT_SELECT",             IIAPI_QT_SELECT},
	{"IIAPI_QT_SET",                IIAPI_QT_SET},
	{"IIAPI_QT_UPDATE",             IIAPI_QT_UPDATE}
};

GLOBALDEF AIT_SYMINT	DT_ID_TBL[MAX_DT_ID] =
{
	{"IIAPI_BYTE_TYPE",   IIAPI_BYTE_TYPE},
	{"IIAPI_CHA_TYPE",    IIAPI_CHA_TYPE}, 
	{"IIAPI_CHR_TYPE",    IIAPI_CHR_TYPE}, 
	{"IIAPI_DEC_TYPE",    IIAPI_DEC_TYPE}, 
	{"IIAPI_DTE_TYPE",    IIAPI_DTE_TYPE}, 
	{"IIAPI_FLT_TYPE",    IIAPI_FLT_TYPE}, 
	{"IIAPI_GEOM_TYPE",   IIAPI_GEOM_TYPE},
	{"IIAPI_GEOMC_TYPE",  IIAPI_GEOMC_TYPE},
	{"IIAPI_HNDL_TYPE",   IIAPI_HNDL_TYPE},
	{"IIAPI_INT_TYPE",    IIAPI_INT_TYPE}, 
	{"IIAPI_LOGKEY_TYPE", IIAPI_LOGKEY_TYPE},
	{"IIAPI_LBYTE_TYPE",  IIAPI_LBYTE_TYPE}, 
	{"IIAPI_LINE_TYPE",   IIAPI_LINE_TYPE},
	{"IIAPI_LOGKEY_TYPE", IIAPI_LOGKEY_TYPE}, 
	{"IIAPI_LTXT_TYPE",   IIAPI_LTXT_TYPE},
	{"IIAPI_LVCH_TYPE",   IIAPI_LVCH_TYPE}, 
	{"IIAPI_MLINE_TYPE",  IIAPI_MLINE_TYPE},
	{"IIAPI_MNY_TYPE",    IIAPI_MNY_TYPE},  
	{"IIAPI_MPOINT_TYPE", IIAPI_MPOINT_TYPE},
	{"IIAPI_MPOLY_TYPE",  IIAPI_MPOLY_TYPE},
	{"IIAPI_NBR_TYPE",    IIAPI_NBR_TYPE},
	{"IIAPI_POINT_TYPE",  IIAPI_POINT_TYPE},
	{"IIAPI_POLY_TYPE",   IIAPI_POLY_TYPE},
	{"IIAPI_TABKEY_TYPE", IIAPI_TABKEY_TYPE},
	{"IIAPI_TXT_TYPE",    IIAPI_TXT_TYPE},  
	{"IIAPI_VBYTE_TYPE",  IIAPI_VBYTE_TYPE}, 
	{"IIAPI_VCH_TYPE",    IIAPI_VCH_TYPE}
};

GLOBALDEF AIT_SYMINT	ds_columnType_TBL[MAX_ds_columnType]=
{
	{"IIAPI_COL_TUPLE",        IIAPI_COL_TUPLE}, 
					/* tuple column type */
	{"IIAPI_COL_PROCBYREFPARM",IIAPI_COL_PROCBYREFPARM}, 
					/* procedure byref parm column type */
	{"IIAPI_COL_PROCPARM",     IIAPI_COL_PROCPARM}, 
					/* procedure parm column type */
	{"IIAPI_COL_SVCPARM",      IIAPI_COL_SVCPARM}, 
					/* service parm column type */
	{"IIAPI_COL_QPARM",        IIAPI_COL_QPARM}  
					/* query parm column type */
};


GLOBALDEF AIT_SYMINT	gq_mask_TBL[MAX_gq_mask] =
{
	{"IIAPI_GQ_ROW_COUNT",		IIAPI_GQ_ROW_COUNT},	/* 0x00000001*/
	{"IIAPI_GQ_CURSOR",		IIAPI_GQ_CURSOR},	/* 0x00000002*/
	{"IIAPI_GQ_PROCEDURE_RET",	IIAPI_GQ_PROCEDURE_RET},	
								/* 0x00000004*/
	{"IIAPI_GQ_PROCEDURE_ID",	IIAPI_GQ_PROCEDURE_ID},	/* 0x00000008*/
	{"IIAPI_GQ_REPEAT_QUERY_ID",	IIAPI_GQ_REPEAT_QUERY_ID},
								/* 0x00000010*/
	{"IIAPI_GQ_TABLE_KEY",		IIAPI_GQ_TABLE_KEY},	/* 0x00000020*/
	{"IIAPI_GQ_OBJECT_KEY",		IIAPI_GQ_OBJECT_KEY}	/* 0x00000040*/
};	

GLOBALDEF AIT_SYMINT	gp_status_TBL[MAX_gp_status] =
{
	{"IIAPI_ST_SUCCESS",		IIAPI_ST_SUCCESS},
	{"IIAPI_ST_MESSAGE",		IIAPI_ST_MESSAGE},
	{"IIAPI_ST_WARNING",		IIAPI_ST_WARNING},
	{"IIAPI_ST_NO_DATA",		IIAPI_ST_NO_DATA},
	{"IIAPI_ST_ERROR",		IIAPI_ST_ERROR},
	{"IIAPI_ST_FAILURE",		IIAPI_ST_FAILURE},
	{"IIAPI_ST_NOT_INITIALIZED",	IIAPI_ST_NOT_INITIALIZED},
	{"IIAPI_ST_INVALID_HANDLE",	IIAPI_ST_INVALID_HANDLE},
	{"IIAPI_ST_OUT_OF_MEMORY",	IIAPI_ST_OUT_OF_MEMORY}
	};

GLOBALDEF AIT_SYMINT	ti_type_TBL[MAX_ti_type] =
{
	{"IIAPI_TI_IIXID",		IIAPI_TI_IIXID},
	{"IIAPI_TI_XAXID",		IIAPI_TI_XAXID}
};

GLOBALDEF AIT_SYMINT	ge_type_TBL[MAX_ge_type] =
{
	{"IIAPI_GE_ERROR",		IIAPI_GE_ERROR},
	{"IIAPI_GE_WARNING",		IIAPI_GE_WARNING},
	{"IIAPI_GE_MESSAGE",		IIAPI_GE_MESSAGE}
};

GLOBALDEF AIT_SYMINT	gq_flags_TBL[MAX_gq_flags] =
{
	{"IIAPI_GQF_FAIL",		IIAPI_GQF_FAIL},
	{"IIAPI_GQF_ALL_UPDATED",	IIAPI_GQF_ALL_UPDATED},
	{"IIAPI_GQF_NULLS_REMOVED",	IIAPI_GQF_NULLS_REMOVED},
	{"IIAPI_GQF_UNKNOWN_REPEAT_QUERY",
					IIAPI_GQF_UNKNOWN_REPEAT_QUERY},
	{"IIAPI_GQF_END_OF_DATA",	IIAPI_GQF_END_OF_DATA},
	{"IIAPI_GQF_CONTINUE",		IIAPI_GQF_CONTINUE},
	{"IIAPI_GQF_INVALID_STATEMENT",	IIAPI_GQF_INVALID_STATEMENT},
	{"IIAPI_GQF_TRANSACTION_INACTIVE",
					IIAPI_GQF_TRANSACTION_INACTIVE},
	{"IIAPI_GQF_OBJECT_KEY",	IIAPI_GQF_OBJECT_KEY},
	{"IIAPI_GQF_TABLE_KEY",		IIAPI_GQF_TABLE_KEY},
	{"IIAPI_GQF_NEW_EFFECTIVE_USER",IIAPI_GQF_NEW_EFFECTIVE_USER},
	{"IIAPI_GQF_FLUSH_QUERY_ID",	IIAPI_GQF_FLUSH_QUERY_ID},
	{"IIAPI_GQF_ILLEGAL_XACT_STMT",	IIAPI_GQF_ILLEGAL_XACT_STMT}
};

GLOBALDEF AIT_SYMINT	CP_SYMINT_TBL[MAX_CP_SYMINT] =
{
	/*
	** CP_MONEY_LORT_TBL
	*/
        {"IIAPI_CPV_MONEY_LEAD_SIGN",   IIAPI_CPV_MONEY_LEAD_SIGN},     /* 0 */
        {"IIAPI_CPV_MONEY_TRAIL_SIGN",  IIAPI_CPV_MONEY_TRAIL_SIGN},    /* 1 */

	/*
	** CP_DATE_FORMAT_TBL
	*/
        {"IIAPI_CPV_DFRMT_US",          IIAPI_CPV_DFRMT_US},            /* 0 */
        {"IIAPI_CPV_DFRMT_MULTI",       IIAPI_CPV_DFRMT_MULTI},         /* 1 */
        {"IIAPI_CPV_DFRMT_FINNISH",     IIAPI_CPV_DFRMT_FINNISH},       /* 2 */
        {"IIAPI_CPV_DFRMT_ISO",         IIAPI_CPV_DFRMT_ISO},           /* 3 */
        {"IIAPI_CPV_DFRMT_GERMAN",      IIAPI_CPV_DFRMT_GERMAN},        /* 4 */
        {"IIAPI_CPV_DFRMT_YMD",         IIAPI_CPV_DFRMT_YMD},           /* 5 */
        {"IIAPI_CPV_DFRMT_MDY",         IIAPI_CPV_DFRMT_MDY},           /* 6 */
        {"IIAPI_CPV_DFRMT_DMY",		IIAPI_CPV_DFRMT_DMY},		/* 7 */
	{"IIAPI_CPV_DFRMT_MLT4",	IIAPI_CPV_DFRMT_MLT4},		/* 8 */
	{"IIAPI_CPV_DFRMT_ISO4",	IIAPI_CPV_DFRMT_ISO4},		/* 9 */

	/*
	** CP_SERVER_TYPE_TBL
	*/
        {"IIAPI_CPV_SVNORMAL",          IIAPI_CPV_SVNORMAL},            /* 1 */
        {"IIAPI_CPV_MONITOR",           IIAPI_CPV_MONITOR}              /* 2 */
};

GLOBALDEF AIT_SYMSTR	CP_FLOAT4_STYLE_TBL[MAX_CP_FLOAT4_STYLE] =
{
	{"IIAPI_CPV_EXPONENTIAL",	"e"			},
	{"IIAPI_CPV_FLOATF",		"f"			},
	{"IIAPI_CPV_FLOATDEC",		"n"			},
	{"IIAPI_CPV_FLOATDECALIGN",	"g"			}
};

GLOBALDEF AIT_SYMSTR	CP_NUMERIC_TREATMENT_TBL[MAX_CP_NUMERIC_TREATMENT] =
{
	{"IIAPI_CPV_DECASFLOAT", 	"f"			},
	{"IIAPI_CPV_DECASDEC",		"d"			}
};


GLOBALDEF AIT_SYMSTR	CP_MATH_EXCP_TBL[MAX_CP_MATH_EXCP] =
{
	{"IIAPI_CPV_RET_FATAL",		"fail"			},
	{"IIAPI_CPV_RET_WARN",		"warn"			},
	{"IIAPI_CPV_RET_IGNORE",	"ignore"		}
};


GLOBALDEF AIT_SYMSTR	CP_SECONDARY_INX_TBL[MAX_CP_SECONDARY_INX] =
{
	{"IIAPI_CPV_ISAM",		"isam"			},
	{"IIAPI_CPV_CISAM",		"cisam"			},
	{"IIAPI_CPV_BTREE",		"btree"			},
	{"IIAPI_CPV_CBTREE",		"cbtree"		},
	{"IIAPI_CPV_HEAP",		"heap"			},
	{"IIAPI_CPV_CHEAP",		"cheap"			},
	{"IIAPI_CPV_HASH",		"hash"			},
	{"IIAPI_CPV_HASH",		"chash"			}
};

GLOBALDEF AIT_SYMSTR	CP_SYMSTR_TBL[MAX_CP_SYMSTR] =
{
	/*
	** CP_FLOAT4_STYLE_TBL
	*/
        {"IIAPI_CPV_EXPONENTIAL",       "e"                     },
        {"IIAPI_CPV_FLOATF",            "f"                     },
        {"IIAPI_CPV_FLOATDEC",          "n"                     },
        {"IIAPI_CPV_FLOATDECALIGN",     "g"                     },

	/*
	** CP_NUMERIC_TREATMENT_TBL
	**/
        {"IIAPI_CPV_DECASFLOAT",        "f"                     },
        {"IIAPI_CPV_DECASDEC",          "d"                     },

	/*
	** CP_MATH_EXCP_TBL
	*/
        {"IIAPI_CPV_RET_FATAL",         "fail"                  },
        {"IIAPI_CPV_RET_WARN",          "warn"                  },
        {"IIAPI_CPV_RET_IGNORE",        "ignore"                },

	/*
	** CP_SECONDARY_INX_TBL
	*/
        {"IIAPI_CPV_ISAM",              "isam"                  },
        {"IIAPI_CPV_CISAM",             "cisam"                 },
        {"IIAPI_CPV_BTREE",             "btree"                 },
        {"IIAPI_CPV_CBTREE",            "cbtree"                },
        {"IIAPI_CPV_HEAP",              "heap"                  },
        {"IIAPI_CPV_CHEAP",             "cheap"                 },
        {"IIAPI_CPV_HASH",              "hash"                  },
        {"IIAPI_CPV_CHASH",             "chash"                 }
};

GLOBALDEF AIT_SYMINT	EP_SYMINT_TBL[MAX_EP_SYMINT] =
{
	/*
	** EP_MONEY_LORT_TBL
	*/
        {"IIAPI_EPV_MONEY_LEAD_SIGN",   IIAPI_EPV_MONEY_LEAD_SIGN},     /* 0 */
        {"IIAPI_EPV_MONEY_TRAIL_SIGN",  IIAPI_EPV_MONEY_TRAIL_SIGN},    /* 1 */

	/*
	** EP_DATE_FORMAT_TBL
	*/
        {"IIAPI_EPV_DFRMT_US",          IIAPI_EPV_DFRMT_US},            /* 0 */
        {"IIAPI_EPV_DFRMT_MULTI",       IIAPI_EPV_DFRMT_MULTI},         /* 1 */
        {"IIAPI_EPV_DFRMT_FINNISH",     IIAPI_EPV_DFRMT_FINNISH},       /* 2 */
        {"IIAPI_EPV_DFRMT_ISO",         IIAPI_EPV_DFRMT_ISO},           /* 3 */
        {"IIAPI_EPV_DFRMT_GERMAN",      IIAPI_EPV_DFRMT_GERMAN},        /* 4 */
        {"IIAPI_EPV_DFRMT_YMD",         IIAPI_EPV_DFRMT_YMD},           /* 5 */
        {"IIAPI_EPV_DFRMT_MDY",         IIAPI_EPV_DFRMT_MDY},           /* 6 */
        {"IIAPI_EPV_DFRMT_DMY",         IIAPI_EPV_DFRMT_DMY},           /* 7 */
	{"IIAPI_EPV_DFRMT_MLT4",	IIAPI_EPV_DFRMT_MLT4},		/* 8 */
	{"IIAPI_EPV_DFRMT_ISO4",	IIAPI_EPV_DFRMT_ISO4}		/* 9 */
};

GLOBALDEF AIT_SYMSTR	EP_FLOAT4_STYLE_TBL[MAX_EP_FLOAT4_STYLE] =
{
	{"IIAPI_EPV_EXPONENTIAL",	"e"			},
	{"IIAPI_EPV_FLOATF",		"f"			},
	{"IIAPI_EPV_FLOATDEC",		"n"			},
	{"IIAPI_EPV_FLOATDECALIGN",	"g"			}
};

GLOBALDEF AIT_SYMSTR	EP_NUMERIC_TREATMENT_TBL[MAX_EP_NUMERIC_TREATMENT] =
{
	{"IIAPI_EPV_DECASFLOAT", 	"f"			},
	{"IIAPI_EPV_DECASDEC",		"d"			}
};

GLOBALDEF AIT_SYMSTR	EP_MATH_EXCP_TBL[MAX_EP_MATH_EXCP] =
{
	{"IIAPI_EPV_RET_FATAL",		"fail"			},
	{"IIAPI_EPV_RET_WARN",		"warn"			},
	{"IIAPI_EPV_RET_IGNORE",	"ignore"		}
};

GLOBALDEF AIT_SYMFUNC	EP_FUNC_TBL[MAX_EP_FUNC] =
{
	{"APItest_trace_func",  apitrace_func },
	{"APItest_prompt_func", apiprompt_func },
	{"APItest_event_func",  apievent_func }
};

GLOBALDEF AIT_SYMINT	EP_MONEY_LORT_TBL[MAX_EP_MONEY_LORT] =
{
	{"IIAPI_EPV_MONEY_LEAD_SIGN",	IIAPI_EPV_MONEY_LEAD_SIGN},	/* 0 */
	{"IIAPI_EPV_MONEY_TRAIL_SIGN",	IIAPI_EPV_MONEY_TRAIL_SIGN}	/* 1 */
};

GLOBALDEF AIT_SYMINT	EP_DATE_FORMAT_TBL[MAX_EP_DATE_FORMAT] =
{
	{"IIAPI_EPV_DFRMT_US",		IIAPI_EPV_DFRMT_US},		/* 0 */
	{"IIAPI_EPV_DFRMT_MULTI",	IIAPI_EPV_DFRMT_MULTI},		/* 1 */
	{"IIAPI_EPV_DFRMT_FINNISH",	IIAPI_EPV_DFRMT_FINNISH},	/* 2 */
	{"IIAPI_EPV_DFRMT_ISO",		IIAPI_EPV_DFRMT_ISO},		/* 3 */
	{"IIAPI_EPV_DFRMT_GERMAN",	IIAPI_EPV_DFRMT_GERMAN},	/* 4 */
	{"IIAPI_EPV_DFRMT_YMD",		IIAPI_EPV_DFRMT_YMD},		/* 5 */
	{"IIAPI_EPV_DFRMT_MDY",		IIAPI_EPV_DFRMT_MDY},		/* 6 */
	{"IIAPI_EPV_DFRMT_DMY",		IIAPI_EPV_DFRMT_DMY},		/* 7 */
	{"IIAPI_EPV_DFRMT_MLT4",	IIAPI_EPV_DFRMT_MLT4}		/* 8 */
};

GLOBALDEF AIT_SYMSTR	EP_SYMSTR_TBL[MAX_EP_SYMSTR] =
{
	/*
	** EP_FLOAT4_STYLE_TBL
	*/
        {"IIAPI_EPV_EXPONENTIAL",       "e"                     },
        {"IIAPI_EPV_FLOATF",            "f"                     },
        {"IIAPI_EPV_FLOATDEC",          "n"                     },
        {"IIAPI_EPV_FLOATDECALIGN",     "g"                     },

	/*
	** EP_NUMERIC_TREATMENT_TBL
	**/
        {"IIAPI_EPV_DECASFLOAT",        "f"                     },
        {"IIAPI_EPV_DECASDEC",          "d"                     },

	/*
	** EP_MATH_EXCP_TBL
	*/
        {"IIAPI_EPV_RET_FATAL",         "fail"                  },
        {"IIAPI_EPV_RET_WARN",          "warn"                  },
        {"IIAPI_EPV_RET_IGNORE",        "ignore"                }

};


/*
** Name: apitrace_func		For Trace Messages
**
** Description:
** 	Trace callback function
**
** History:
**      03-Feb-99 (rajus01)
**          Created.
*/
static II_VOID 
apitrace_func( IIAPI_TRACEPARM *traceParm )
{

    SIfprintf( outputf, "\t===== Trace Message =====\n" );

    if ( verboseLevel > 1 )
    {
        SIfprintf( outputf, "\ttr_envHandle = 0x%lx\n",
					traceParm->tr_envHandle ); 
        SIfprintf( outputf, "\ttr_connHandle = 0x%lx\n",
					traceParm->tr_connHandle ); 
    }

    SIfprintf( outputf, "\ttr_length = %d\n", traceParm->tr_length ); 

    if( traceParm->tr_length > 0 )
	SIfprintf( outputf, "\ttr_message = %s\n", traceParm->tr_message ); 

    SIfprintf( outputf, "\t=========================\n" );

    return;
}


/*
** Name: apiprompt_func		For prompt Messages
**
** Description:
** 	Prompt callback function
**
** History:
**      03-Feb-99 (rajus01)
**          Created.
*/
static II_VOID 
apiprompt_func( IIAPI_PROMPTPARM *promptParm )
{

    SIfprintf( outputf, "\t===== Prompt Message =====\n" );

    STcopy(password, promptParm->pd_reply);
    promptParm->pd_rep_len = STlength( promptParm->pd_reply );
    promptParm->pd_rep_flags = 0;

    if ( verboseLevel > 1 )
    {
        SIfprintf( outputf, "\tpd_envHandle\t= 0x%lx\n",
					promptParm->pd_envHandle ); 
        SIfprintf( outputf, "\tpd_connHandle\t= 0x%lx\n",
					promptParm->pd_connHandle ); 
    }
    
    if( promptParm->pd_flags & IIAPI_PR_NOECHO )
 	SIfprintf( outputf, "\tpd_flags\t= IIAPI_PR_NOECHO \n" );
    if( promptParm->pd_flags & IIAPI_PR_TIMEOUT )
    {
 	SIfprintf( outputf, "\tpd_flags\t= IIAPI_PR_TIMEOUT \n" );
 	SIfprintf( outputf, "\tpd_timeout\t= %d\n", promptParm->pd_timeout );
    }

    SIfprintf( outputf, "\tpd_msg_len\t= %d\n", promptParm->pd_msg_len );
    SIfprintf( outputf, "\tpd_message\t= %s\n", promptParm->pd_message );
    SIfprintf( outputf, "\tpd_max_reply\t= %d\n", promptParm->pd_max_reply );

    SIfprintf( outputf, "\t=========================\n" );

    return;   
}

/*
** Name: apievent_func		For Dbevent Messages
**
** Description:
** 	Dbevent callback function
**
** History:
**      03-Feb-99 (rajus01)
**          Created.
**	12-Jul-02 (gordy)
**	    ait_printData() now prints directly to output.
*/
static II_VOID 
apievent_func( IIAPI_EVENTPARM *eventParm )
{

    SIfprintf( outputf, "\t===== Dbevent Message =====\n" );

    if ( verboseLevel > 1 )
    {
        SIfprintf( outputf, "\tev_envHandle = 0x%lx\n",
					eventParm->ev_envHandle ); 
        SIfprintf( outputf, "\tev_connHandle = 0x%lx\n",
					eventParm->ev_connHandle ); 
    }

    SIfprintf( outputf, "\tev_eventName\t= %s\n", eventParm->ev_eventName ); 
    SIfprintf( outputf, "\tev_eventOwner\t= %s\n", eventParm->ev_eventOwner ); 
    SIfprintf( outputf, "\tev_eventDB\t= %s\n", eventParm->ev_eventDB ); 

    if ( verboseLevel >= 1 )
    {
        if ( eventParm->ev_eventTime.dv_null == TRUE )
	    SIfprintf( outputf, "\tev_eventTime = NULL\n" );
        else
        {
	    IIAPI_DESCRIPTOR        descr;

	    descr.ds_dataType = IIAPI_DTE_TYPE;
	    descr.ds_nullable = FALSE;
	    descr.ds_length = eventParm->ev_eventTime.dv_length;
	    descr.ds_precision = 0;
	    descr.ds_scale = 0;
	    descr.ds_columnType = IIAPI_COL_TUPLE;
	    descr.ds_columnName = NULL;
            SIfprintf( outputf, "\tev_eventTime\t= " ); 
	    ait_printData( &descr, &eventParm->ev_eventTime );
        }
    }

    SIfprintf( outputf, "\t=========================\n" );

    return;
}
