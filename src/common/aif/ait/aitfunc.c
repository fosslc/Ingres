/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<iiapi.h>
# include	"aitfunc.h"
# include	"aitdef.h"
# include	"aitsym.h"
# include	"aitproc.h"

/*
** Name: aitfunc.c
**
** Description:
**	API function access.
**
** History:
**	27-Mar-95 (gordy)
**	    Extracted from aitproc.c.
**	28-Apr-95 (feige)
**	    Updated api_catchEvent.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Fixed printing of long datatypes.
**	14-Jun-95 (gordy)
**	    Finalized Two Phase Commit interface.
**	13-Jul-95 (gordy)
**	    Fix printing for 16-bit systems.
**	14-Jul-95 (feige)
**	    Fix printing of handle info and ge_serverInfo.
**	    They are printed only when "-verbose" is set
**	    at command line. 
**	23-Feb-96 (gordy)
**	    Updated COPYMAP display for changes in API.
**	26-Feb-96 (feige)
**	    Updated COPYMAP interface, added cp_dbmsCount and
**	    cp_dbmsDescr to funTBL[_IIapi_getCopyMap].parmName.
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	20-Sep-96 (gordy)
**	    Slight adjustments to output.
**	10-Feb-97 (gordy)
**	    Added IIapi_releaseEnv(), in_envHandle to IIapi_initialize(),
**	    and co_type to IIapi_connect().
**	19-Aug-98 (rajus01)
**	    Added IIapi_setEnvParam(), IIapi_abort(), IIapi_formatData().
**	11-Sep-98 (rajus01)
**	    Added ait_printfmtData().
**	03-Feb-99 (rajus01)
**	    Removed the static definition for ait_PrintData() from here.
**	    Made changes to verbose level checking. 
**	10-Feb-99 (rajus01)
**	    Added api_hexdump() func.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Jul-02 (gordy)
**	    Removed buffer parameter from various functions and print
**	    results directly to output to handle long column values.
**	20-Feb-03 (gordy)
**	    Add formatting/display of table/object keys.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs or NBR type as BYTE.
*/



/*
** Local functions.
*/

static VOID	api_autocommit( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_catchEvent( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_connect( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_formatData( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_getColumns( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_getCopyMap( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_getDescriptor( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_getQueryInfo( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_initialize( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_query( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_registerXID( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_releaseEnv( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_releaseXID( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_savePoint( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_setConnectParam( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_setEnvParam( AITCB *aitcb, II_PTR parmBlock );
static VOID	api_terminate( AITCB *aitcb, II_PTR parmBlock );
static VOID	ait_formatString( char *string, i4  length );
static VOID 	print_descr( char *var1, i4  index, 
			     char *var2, IIAPI_DESCRIPTOR *descr );
static VOID	ait_printfmtData( PTR envHndl,
				   IIAPI_DESCRIPTOR *descriptor,
				   IIAPI_DATAVALUE *datavalue );
static VOID	api_hexdump( char *ibuf, i4 len );



/*
** API function information
*/

GLOBALDEF API_FUNC_INFO funTBL[ MAX_FUN_NUM ] = 
{
    { /* 0 */
	"IIapi_autocommit",
	AIT_ASYNC,
	IIapi_autocommit,
	api_autocommit,
	"IIAPI_AUTOPARM",
	sizeof(IIAPI_AUTOPARM),
	{
	    "ac_connHandle",
	    "ac_tranHandle",
	    "_NULL"
	},
	-1
    },
	
    { /* 1 */
	"IIapi_cancel",
	AIT_ASYNC,
	IIapi_cancel,
	NULL,
	"IIAPI_CANCELPARM",
	sizeof(IIAPI_CANCELPARM),
	{
	    "cn_stmtHandle",
	    "_NULL"
	},
	-1
    },

    { /* 2 */
	"IIapi_catchEvent",
	AIT_ASYNC,
	IIapi_catchEvent,
	api_catchEvent,
	"IIAPI_CATCHEVENTPARM",
	sizeof(IIAPI_CATCHEVENTPARM),
	{
	    "ce_connHandle",
	    "ce_selectEventName",
	    "ce_selectEventOwner",
	    "ce_eventHandle",
	    "ce_eventName",
	    "ce_eventOwner",
	    "ce_eventDB",
	    "ce_eventTime",
	    "ce_eventInfoAvail",
	    "_NULL"
	},
	-1
    },

    { /* 3 */
	"IIapi_close",
	AIT_ASYNC,
	IIapi_close,
	NULL,
	"IIAPI_CLOSEPARM",
	sizeof(IIAPI_CLOSEPARM),
	{
	    "cl_stmtHandle"	,
	    "_NULL"
	},
	-1
    },
	
    { /* 4 */
	"IIapi_commit",
	AIT_ASYNC,
	IIapi_commit,
	NULL,
	"IIAPI_COMMITPARM",
	sizeof(IIAPI_COMMITPARM),
	{
	    "cm_tranHandle",
	    "_NULL"
	},
	-1
    },
	
    { /* 5 */
	"IIapi_connect",
	AIT_ASYNC,
	IIapi_connect,
	api_connect,
	"IIAPI_CONNPARM",
	sizeof(IIAPI_CONNPARM),
	{
	    "co_target",
	    "co_username",
	    "co_password",
	    "co_timeout",
	    "co_connHandle",
	    "co_tranHandle",
	    "co_sizeAdvise",
	    "co_apiLevel",
	    "co_type",
	    "_NULL"
	},
	-1
    },

    { /* 6 */
	"IIapi_disconnect",
	AIT_ASYNC,
	IIapi_disconnect,
	NULL,
	"IIAPI_DISCONNPARM",
	sizeof(IIAPI_DISCONNPARM),
	{
	    "dc_connHandle",
	    "_NULL"
	},
	-1
    },

    { /* 7 */
	"IIapi_getColumns",
	AIT_ASYNC,
	IIapi_getColumns,
	api_getColumns,
	"IIAPI_GETCOLPARM",
	sizeof(IIAPI_GETCOLPARM),
	{
	    "gc_stmtHandle",
	    "gc_rowCount",
	    "gc_columnCount",
	    "gc_columnData",
	    "gc_rowsReturned",
	    "gc_moreSegments",
	    "_NULL"
	},
	_gc_columnData
    },

    { /* 8 */
	"IIapi_getCopyMap",
	AIT_ASYNC,
	IIapi_getCopyMap,
	api_getCopyMap,
	"IIAPI_GETCOPYMAPPARM",
	sizeof(IIAPI_GETCOPYMAPPARM),
	{
	    "gm_stmtHandle",
	    "gm_copyMap",
	    "cp_dbmsCount",
	    "cp_dbmsDescr",
	    "_NULL"
	},
	-1
    },

    { /* 9 */
	"IIapi_getDescriptor",
	AIT_ASYNC,
	IIapi_getDescriptor,
	api_getDescriptor,
	"IIAPI_GETDESCRPARM",
	sizeof(IIAPI_GETDESCRPARM),
	{
	    "gd_stmtHandle",
	    "gd_descriptorCount",
	    "gd_descriptor",
	    "_NULL"
	},
	_gd_descriptor
    },

    { /* 10 */
	"IIapi_getErrorInfo",
	AIT_SYNC,
	IIapi_getErrorInfo,
	NULL,
	"IIAPI_GETEINFOPARM",
	sizeof(IIAPI_GETEINFOPARM),
	{
	    "ge_stmtHandle",
	    "ge_dbmsSQLState",
	    "ge_dbmsNativeError",
	    "ge_severID",
	    "ge_severType",
	    "ge_severity",
	    "ge_errorType",
	    "ge_parmCount",
	    "ge_parm",
	    "ge_status",
	    "ge_sQLState",
	    "ge_nativeError",
	    "_NULL"
	},
	-1
    },

    { /* 11 */
	"IIapi_getQueryInfo",
	AIT_ASYNC,
	IIapi_getQueryInfo,
	api_getQueryInfo,
	"IIAPI_GETQINFOPARM",
	sizeof(IIAPI_GETQINFOPARM),
	{
	    "gq_stmtHandle",
	    "gq_flags",	
	    "gq_mask",
	    "gq_rowCount",
	    "gq_readonly",
	    "gq_procedureReturn",
	    "gq_procedureHandle",
	    "gq_repeatQueryHandle",
	    "gq_tableKey[TBL_KEY_SZ]",	
	    "gq_objectKey[OBJ_KEY_SZ]",
	    "_NULL"
	},
	-1
    },

    { /* 12 */
	"IIapi_getEvent",
	AIT_ASYNC,
	IIapi_getEvent,
	NULL,
	"IIAPI_GETEVENTPARM",
	sizeof(IIAPI_GETEVENTPARM),
	{
	    "gv_connHandle",
	    "gv_timeout",
	    "_NULL"
	},
	-1
    },

    { /* 13 */
	"IIapi_initialize",
	AIT_SYNC,
	IIapi_initialize,
	api_initialize,
	"IIAPI_INITPARM",
	sizeof(IIAPI_INITPARM),
	{
	    "in_timeout",
	    "in_version",
	    "in_envHandle",
	    "_NULL"
	},
	-1
    },

    { /* 14 */	
	"IIapi_modifyConnect",
	AIT_ASYNC,
	IIapi_modifyConnect,
	NULL,
	"IIAPI_MODCONNPARM",
	sizeof(IIAPI_MODCONNPARM),
	{
	    "mc_connHandle",
	    "_NULL"
	},
	-1
    },

    { /* 15 */
	"IIapi_prepareCommit",
	AIT_ASYNC,
	IIapi_prepareCommit,
	NULL,
	"IIAPI_PREPCMTPARM",
	sizeof(IIAPI_PREPCMTPARM),
	{
	    "pr_tranHandle",
	    "_NULL"
	},
	-1
    },

    { /* 16 */
	"IIapi_putColumns",
	AIT_ASYNC,
	IIapi_putColumns,
	NULL,
	"IIAPI_PUTCOLPARM",
	sizeof(IIAPI_PUTCOLPARM),
	{
	    "pc_stmtHandle",
	    "pc_columnCount",
	    "pc_columnData",
	    "pc_moreSegments",
	    "_NULL"
	},
	_pc_columnData
    },

    { /* 17 */
	"IIapi_putParms",
	AIT_ASYNC,
	IIapi_putParms,
	NULL,
	"IIAPI_PUTPARMPARM",
	sizeof(IIAPI_PUTPARMPARM),
	{
	    "pp_stmtHandle",
	    "pp_parmCount",
	    "pp_parmData",
	    "pp_moreSegments",
	    "_NULL"
	},
	_pp_parmData
    },

    { /* 18 */
	"IIapi_query",
	AIT_ASYNC,
	IIapi_query,
	api_query,
	"IIAPI_QUERYPARM",
	sizeof(IIAPI_QUERYPARM),
	{
	    "qy_connHandle",
	    "qy_queryType",
	    "qy_queryText",
	    "qy_parameters",
	    "qy_tranHandle",
	    "qy_stmtHandle",
	    "_NULL"
	},
	-1
    },

    { /* 19 */
	"IIapi_registerXID",
	AIT_SYNC,
	IIapi_registerXID,
	api_registerXID,
	"IIAPI_REGXIDPARM",
	sizeof(IIAPI_REGXIDPARM),
	{
	    "ti_type",
	    "it_highTran",
	    "it_lowTran",
	    "rg_tranIdHandle",
	    "rg_status",
	    "_NULL"
	},
	-1
    },

    { /* 20 */
	"IIapi_releaseEnv",
	AIT_SYNC,
	IIapi_releaseEnv,
	api_releaseEnv,
	"IIAPI_RELENVPARM",
	sizeof(IIAPI_RELENVPARM),
	{
	    "re_envHandle",
	    "re_status",
	    "_NULL"
	},
	-1
    },

    { /* 21 */
	"IIapi_releaseXID",
	AIT_SYNC,
	IIapi_releaseXID,
	api_releaseXID,
	"IIAPI_RELXIDPARM",
	sizeof(IIAPI_RELXIDPARM),
	{
	    "rl_tranIdHandle",
	    "rl_status",
	    "_NULL"
	},
	-1
    },

    { /* 22 */
	"IIapi_rollback",
	AIT_ASYNC,
	IIapi_rollback,
	NULL,
	"IIAPI_ROLLBACKPARM",
	sizeof(IIAPI_ROLLBACKPARM),
	{
	    "rb_tranHandle",
	    "rb_savePointHandle",
	    "_NULL"
	},
	-1
    },

    { /* 23 */
	"IIapi_savePoint",
	AIT_ASYNC,
	IIapi_savePoint,
	api_savePoint,
	"IIAPI_SAVEPTPARM",
	sizeof(IIAPI_SAVEPTPARM),
	{
	    "sp_tranHandle",
	    "sp_savePoint",
	    "sp_savePointHandle",
	    "_NULL"
	},
	-1
    },

    { /* 24 */
	"IIapi_setConnectParam",
	AIT_ASYNC,
	IIapi_setConnectParam,
	api_setConnectParam,
	"IIAPI_SETCONPRMPARM",
	sizeof(IIAPI_SETCONPRMPARM),
	{
	    "sc_connHandle",
	    "sc_paramID",
	    "sc_paramValue",
	    "_NULL"
	},
	_sc_paramValue
    },

    { /* 25 */
	"IIapi_setEnvParam",
	AIT_SYNC,
	IIapi_setEnvParam,
	api_setEnvParam,
	"IIAPI_SETENVPRMPARM",
	sizeof(IIAPI_SETENVPRMPARM),
	{
	    "se_envHandle",
	    "se_paramID",
	    "se_paramValue",
	    "se_status",
	    "_NULL"
	},
	_se_paramValue
    },

    { /* 26 */
	"IIapi_setDescriptor",
	AIT_ASYNC,
	IIapi_setDescriptor,
	NULL,
	"IIAPI_SETDESCRPARM",
	sizeof(IIAPI_SETDESCRPARM),
	{
	    "sd_stmtHandle",
	    "sd_descriptorCount",
	    "sd_descriptor",
	    "_NULL"
	},
	_sd_descriptor
    },

    { /* 27 */
	"IIapi_terminate",
	AIT_SYNC,
	IIapi_terminate,
	api_terminate,
	"IIAPI_TERMPARM",
	sizeof(IIAPI_TERMPARM),
	{
	    "tm_status",
	    "_NULL"
	},
	-1
    },

    { /* 28 */
	"IIapi_wait",
	AIT_SYNC,
	IIapi_wait,
	NULL,
	"IIAPI_WAITPARM",
	sizeof(IIAPI_WAITPARM),
	{
	    "wt_timeout",
	    "_NULL"
	},
	-1
    },

    { /* 29 */
	"IIapi_abort",
	AIT_ASYNC,
	IIapi_abort,
	NULL,
	"IIAPI_ABORTPARM",
	sizeof(IIAPI_ABORTPARM),
	{
	    "ab_connHandle",
	    "_NULL"
	},
	-1
    },

    { /* 30 */
	"IIapi_formatData",
	AIT_SYNC,
	IIapi_formatData,
	api_formatData,
	"IIAPI_FORMATPARM",
	sizeof(IIAPI_FORMATPARM),
	{
	    "fd_envHandle",
	    "fd_srcDesc",
	    "fd_srcValue",
	    "fd_dstDesc",
	    "fd_dstValue",
	    "fd_status",
	    "_NULL"
	},

	-1
    }

};



/*
** Name: AITapi_results() - status and error reporting function
** 
** Description:
**	This function examine the status of API function upon completion, 
**	and prints error message if any.
**
** Inputs
**	status  - gp_status of current API function call
**	errHndl - gp_errorHandle of current API function call
**
** Outputs:
**	None.
**
** return:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    ge_serverInfo will not be printed
**	    if "-verbose" is not set at the
**	    command line.
*/

FUNC_EXTERN VOID
AITapi_results( IIAPI_STATUS status, II_PTR errHndl )
{
    IIAPI_GETEINFOPARM 	ge;
    IIAPI_DESCRIPTOR	*descr;
    IIAPI_DATAVALUE	*val;
    i4  		i;

    /*
    ** Print API function call status.
    */
    for ( i=0; i < MAX_gp_status; i++ )
    	if ( status == (IIAPI_STATUS)gp_status_TBL[i].longVal )
	    break;

    if ( i < MAX_gp_status )
    	SIfprintf( outputf, "\tgp_status = %ld %s\n", status,
					gp_status_TBL[i].symbol );
    else
	SIfprintf( outputf, "\tgp_status = %ld unknown gp_status\n", status );	

    /*
    ** Retrieve error information.
    */
    if ( errHndl == NULL )
	return;

    ge.ge_errorHandle = errHndl;

    do
    {
	IIapi_getErrorInfo( &ge );

	if ( ge.ge_status != IIAPI_ST_SUCCESS )
	    break;

	SIfprintf( outputf, "\tIIapi_getErrorInfo:\n" );

	for ( i = 0; i < MAX_ge_type; i++ )
	    if ( ge.ge_type == ge_type_TBL[i].longVal )
		break;

        if ( i < MAX_ge_type )
    	SIfprintf( outputf, "\t\tge_type = %ld %s\n", ge.ge_type,
					ge_type_TBL[i].symbol );
    	else
	    SIfprintf( outputf, "\t\tge_type = %ld unknown ge_type\n", 
				ge.ge_type );	
	
	SIfprintf( outputf, "\t\tge_SQLSATE = %s\n", ge.ge_SQLSTATE );
	SIfprintf( outputf, "\t\tge_errCode = %ld\n", ge.ge_errorCode );
	SIfprintf( outputf, "\t\tge_message = %s\n", ge.ge_message );
	
	/*
	** ge_serverInfo will only be
	** printed out when "-verbose"
	** flag is set at the command
	** line and ge_serverInfoAvail
	** is TRUE.
	*/
	if (
	     verboseLevel > 0 &&
	     ge.ge_serverInfoAvail == TRUE
	   )
	{
	    SIfprintf( outputf, "\t\tsvr_id_error = %ld\n", 
			ge.ge_serverInfo->svr_id_error );
	    SIfprintf( outputf, "\t\tsvr_local_error = %ld\n", 
			ge.ge_serverInfo->svr_local_error );
	    SIfprintf( outputf, "\t\tsvr_id_server = %ld\n", 
			ge.ge_serverInfo->svr_id_server );
	    SIfprintf( outputf, "\t\tsvr_server_type = %ld\n", 
			ge.ge_serverInfo->svr_server_type );

	    switch( ge.ge_serverInfo->svr_server_type )
	    {
		case	IIAPI_SVR_DEFAULT:
		    SIfprintf( outputf, "\t\tsvr_severity = %lx %s\n",
				ge.ge_serverInfo->svr_severity, 
				"IIAPI_SVR_DEFAULT" );	
	
		    break;

		case	IIAPI_SVR_MESSAGE:
		    SIfprintf( outputf, "\t\tsvr_severity = %lx %s\n",
				ge.ge_serverInfo->svr_severity, 
				"IIAPI_SVR_MESSAGE" );	
	
		    break;

		case	IIAPI_SVR_WARNING:
		    SIfprintf( outputf, "\t\tsvr_severity = %lx %s\n",
				ge.ge_serverInfo->svr_severity, 
				"IIAPI_SVR_WARNING" );	
	
		    break;

		case	IIAPI_SVR_FORMATTED:
		    SIfprintf( outputf, "\t\tsvr_severity = %lx %s\n",
				ge.ge_serverInfo->svr_severity, 
				"IIAPI_SVR_FORMATTED" );	
	
		    break;
		
		default:
		    SIfprintf( outputf, "svr_severity: unknown = %lx\n",
		               ge.ge_serverInfo->svr_severity );
	     }	/* switch */

	    /*
	    ** svr_parmCount, svr_parmDescr, svr_parmValue 
	    */
	    descr = ge.ge_serverInfo->svr_parmDescr;
	    val = ge.ge_serverInfo->svr_parmValue;

	    for ( i = 0; i < ge.ge_serverInfo->svr_parmCount; i++ )
	    {
		SIfprintf( outputf, "\t\tsvr_parm[%d] = ", i );
		ait_printData( descr, val );

		descr++;
		val++;
	    }	/* for */
	}	/* if */
    } while( 1 );

    return;
}






/*
** Name: api_autocommit
**
** Description:
**	Output results of IIapi_autocommit().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_connect() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	 9-Jul-96 (gordy)
**	    Created.
*/

static VOID
api_autocommit( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_AUTOPARM	*autoparm = (IIAPI_AUTOPARM *)parmBlock;

    if ( verboseLevel > 1 )
    {
	SIfprintf( outputf, "\tac_tranHandle = 0x%lx\n", 
		   autoparm->ac_tranHandle );
    }
   
    return;
}



/*
** Name: api_catchEvent
**
** Description:
**	Output results of IIapi_catchEvent().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_catchEvent() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	28-Apr-95 (feige)
**	    Updated output.
**	14-Jul-95 (feige)
**	    Print ce_eventHandle and ce_eventTime
**	    when "-verbose" is set.
*/

static VOID
api_catchEvent( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_CATCHEVENTPARM	*ce = (IIAPI_CATCHEVENTPARM *)parmBlock;
    IIAPI_DESCRIPTOR		descr;
    
    if ( verboseLevel > 1 )
	SIfprintf( outputf, "\tce_eventHandle = 0x%lx\n", ce->ce_eventHandle );
    SIfprintf( outputf, "\tce_eventName = %s\n", ce->ce_eventName );
    SIfprintf( outputf, "\tce_eventOwner = %s\n", ce->ce_eventOwner );
    SIfprintf( outputf, "\tce_eventDB = %s\n", ce->ce_eventDB );

    if ( verboseLevel > 1 )
	if ( ce->ce_eventTime.dv_null == TRUE )
	    SIfprintf( outputf, "\tce_eventTime = NULL\n" );
	else
	{
	    descr.ds_dataType = IIAPI_DTE_TYPE;
	    descr.ds_nullable = FALSE;
	    descr.ds_length = ce->ce_eventTime.dv_length;
	    descr.ds_precision = 0;
	    descr.ds_scale = 0;
	    descr.ds_columnType = IIAPI_COL_TUPLE;
	    descr.ds_columnName = NULL;

	    SIfprintf( outputf, "\tce_eventTime = " );
	    ait_printData( &descr, &ce->ce_eventTime );
	}

    SIfprintf( outputf, "\tce_eventInfoAvail == %s\n",
	       ce->ce_eventInfoAvail ? "TRUE" : "FALSE" );

    return;
}


/*
** Name: api_formatData
**
** Description:
**	Output results of IIapi_formatData().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_formatData() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	19-Jul-98 (rajus01)
**	    Created.
**	10-Feb-99 (rajus01)
**	    Don't dump the hex for decimal, date, money data types unless
**	    the verbose level is greater than or equal to 1.
*/

static VOID
api_formatData( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_FORMATPARM		*fd = (IIAPI_FORMATPARM *)parmBlock;
    
    AITapi_results( fd->fd_status, NULL );

    if ( fd->fd_dstValue.dv_null == TRUE )
	SIfprintf( outputf, "\tfd_dstValue = NULL\n" );
    else
    {
	if ( 
	     verboseLevel >= 1  ||
	     (fd->fd_dstDesc.ds_dataType != IIAPI_DTE_TYPE  &&
	      fd->fd_dstDesc.ds_dataType != IIAPI_MNY_TYPE  &&
	      fd->fd_dstDesc.ds_dataType != IIAPI_DEC_TYPE)
	   ) 
	{
	    SIfprintf( outputf, "\tfd_dstValue = " );
	    ait_printfmtData( fd->fd_envHandle, &fd->fd_dstDesc,
					&fd->fd_dstValue );
	}
    }

    return;
}



/*
** Name: api_connect
**
** Description:
**	Output results of IIapi_connect().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_connect() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Print co_connHandle and co_tranHandle
**	    only when "-verbose" is set.
*/

static VOID
api_connect( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_CONNPARM	*conn = (IIAPI_CONNPARM *)parmBlock;

    if ( verboseLevel > 1 )
    {
	SIfprintf( outputf, "\tco_connHandle = 0x%lx\n", conn->co_connHandle );
	SIfprintf( outputf, "\tco_tranHandle = 0x%lx\n", conn->co_tranHandle );
    }
    if ( verboseLevel > 0 )
        SIfprintf( outputf, "\tco_sizeAdvise = %ld\n", conn->co_sizeAdvise );

    switch( conn->co_apiLevel )
    {
	case IIAPI_LEVEL_0:
	    SIfprintf( outputf, "\tco_apiLevel = %ld IIAPI_LEVEL_0\n",
			conn->co_apiLevel );

	    break;

	case IIAPI_LEVEL_1:
	    SIfprintf( outputf, "\tco_apiLevel = %ld IIAPI_LEVEL_1\n",
			conn->co_apiLevel );

	    break;
	
	default:
	    SIfprintf( outputf, "\tco_apiLevel = %ld unknown apiLevel\n",
			conn->co_apiLevel );

	    break;
    }	/* switch */
   
    return;
}






/*
** Name: api_getColumns
**
** Description:
**	Output results of IIapi_getColumns().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_getColumns() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	22-Mar-95 (feige)
**	    Add break after repeat = FALSE in the if branch.
*/

static VOID
api_getColumns( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_GETCOLPARM	*gcol = (IIAPI_GETCOLPARM *)parmBlock;
    IIAPI_DATAVALUE	*columnData;
    IIAPI_DESCRIPTOR	*dptr;
    i4			i, j;

    if ( aitcb->func->descriptor )
    {
	columnData = gcol->gc_columnData;

	for ( i = 0; i < gcol->gc_rowsReturned; i++ )
	{
	    dptr = aitcb->func->descriptor;

	    for ( j = 0; j < gcol->gc_columnCount; j++ )
	    {
		 SIfprintf( outputf, "\tgc_columnData[%ld] = ", 
			    i * gcol->gc_columnCount + j );	
		 ait_printData( dptr, columnData );

		 columnData++; 
		 dptr++;
	    }
	}
    }	

    SIfprintf( outputf, "\tgc_rowsReturned = %ld\n", 
	       (II_LONG)gcol->gc_rowsReturned );

    SIfprintf( outputf, "\tgc_moreSegments = %s\n",
	       gcol->gc_moreSegments ? "TRUE" : "FALSE" );

    return;
}






/*
** Name: api_getCopyMap
**
** Description:
**	Output results of IIapi_getCopyMap().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_getCopyMap() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	23-Feb-96 (gordy)
**	    Updated COPYMAP display for changes in API.
*/

static VOID
api_getCopyMap( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_GETCOPYMAPPARM	*gcm = (IIAPI_GETCOPYMAPPARM *)parmBlock;
    IIAPI_FDATADESCR		*fd;
    i4				i, j;
    
    SIfprintf( outputf, "\tcp_copyFrom = %s\n",
	       gcm->gm_copyMap.cp_copyFrom ? "TRUE" : "FALSE" );

    SIfprintf( outputf, "\tcp_flags = 0x%x\n", gcm->gm_copyMap.cp_flags );

    SIfprintf( outputf, "\tcp_errorCount = %d\n",
	       gcm->gm_copyMap.cp_errorCount ); 

    SIfprintf( outputf, "\tcp_fileName = %s\n", 
	       gcm->gm_copyMap.cp_fileName 
	       ? gcm->gm_copyMap.cp_fileName : "<null>" );

    SIfprintf( outputf, "\tcp_logName = %s\n", 
	       gcm->gm_copyMap.cp_logName
	       ? gcm->gm_copyMap.cp_logName: "<null>" );

    SIfprintf( outputf, "\tcp_dbmsCount = %d\n", gcm->gm_copyMap.cp_dbmsCount );

    for( i = 0; i < gcm->gm_copyMap.cp_dbmsCount; i++ )
	print_descr( "cp_dbmsDescr", i, "", &gcm->gm_copyMap.cp_dbmsDescr[i] );

    SIfprintf(outputf, "\tcp_fileCount = %d\n", gcm->gm_copyMap.cp_fileCount);

    for( i = 0; i < gcm->gm_copyMap.cp_fileCount; i++ )
    {
	fd = &gcm->gm_copyMap.cp_fileDescr[i];

	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_name = %s\n", i, fd->fd_name );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_type = %d\n", i, fd->fd_type );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_length = %d\n", i, fd->fd_length );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_prec = %d\n", i, fd->fd_prec );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_column = %d\n", i, fd->fd_column );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_funcID = %d\n", i, fd->fd_funcID );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_cvLen = %d\n", i, fd->fd_cvLen );
	SIfprintf( outputf, 
		   "\tcp_fileDescr[%d].fd_cvPrec = %d\n", i, fd->fd_cvPrec );

	SIfprintf( outputf, "\tcp_fileDescr[%d].fd_delimiter == %s\n",
		   i, fd->fd_delimiter ? "TRUE" : "FALSE" );

	if ( fd->fd_delimiter )
	{
	    SIfprintf( outputf, "\tcp_fileDescr[%d].fd_delimLength = %d\n",
		       i, fd->fd_delimLength );

	    SIfprintf( outputf, "\tcp_fileDescr[%d].fd_delimValue =",
		       i, fd->fd_delimiter );

	    for( j = 0; j < fd->fd_delimLength; j++ )
		SIfprintf( outputf, " 0x%x", fd->fd_delimValue[j] );

	    SIfprintf( outputf, "\n" );
	}

	SIfprintf( outputf, "\tcp_fileDescr[%d].fd_nullable == %s\n",
		   i, fd->fd_nullable ? "TRUE" : "FALSE" );

	if ( fd->fd_nullable )
	{
	    SIfprintf( outputf, "\tcp_fileDescr[%d].fd_nullInfo == %s\n",
		       i, fd->fd_nullInfo ? "TRUE" : "FALSE" );

	    if ( fd->fd_nullInfo )
	    {
		print_descr( "cp_fileDescr", 
			     i, ".fd_nullDescr", &fd->fd_nullDescr );

		SIfprintf( outputf, "\tcp_fileDescr[%d].fd_nullValue = ", i );
		ait_printData( &fd->fd_nullDescr, &fd->fd_nullValue );
	    }
	}
    }

    return;
}






/*
** Name: api_getDescriptor
**
** Description:
**	Output results of IIapi_getDescriptor().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_getDescriptor() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
*/

static VOID
api_getDescriptor( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_GETDESCRPARM	*getd = (IIAPI_GETDESCRPARM *)parmBlock;
    i4			i;

    SIfprintf( outputf, "\tgd_descriptorCount = %ld\n",
		getd->gd_descriptorCount ); 

    for ( i = 0; i < getd->gd_descriptorCount; i++ )
	print_descr( "gd_descriptor", i, "", &getd->gd_descriptor[i] );

    return;
}






/*
** Name: api_getQueryInfo
**
** Description:
**	Output resulst of IIapi_getQueryInfo().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_getQueryInfo() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Print gq_procedureHandle and gq_repeatQueryHandle
**	    only when "-verbose" is set.
**	20-Feb-03 (gordy)
**	    Add formatting/display of table/object keys.
*/

static VOID
api_getQueryInfo( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_GETQINFOPARM	*getq = (IIAPI_GETQINFOPARM *)parmBlock;
    i4			i;

   SIfprintf( outputf, "\tgq_flags = 0x%lx", getq->gq_flags );

   for ( i = 0; i < MAX_gq_flags; i++ )
       if ( gq_flags_TBL[i].longVal & getq->gq_flags )
	   SIfprintf( outputf, " %s", gq_flags_TBL[i].symbol );

   SIfprintf( outputf, "\n" );

   if ( getq->gq_mask & IIAPI_GQ_ROW_COUNT )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_GQ_ROW_COUNT\n" );
	SIfprintf( outputf, "\tgq_rowCount = %ld\n", getq->gq_rowCount );
   }

    if ( getq->gq_mask & IIAPI_GQ_CURSOR )
	SIfprintf( outputf, "\tgetq->gq_readonly = %s\n",
		   getq->gq_readonly ? "TRUE" : "FALSE" );

   if ( getq->gq_mask & IIAPI_GQ_PROCEDURE_RET )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_PROCEDURE_RET\n" );
	SIfprintf( outputf, "\tgq_procedureReturn = %ld\n", 
			    getq->gq_procedureReturn );
   }

   if ( getq->gq_mask & IIAPI_GQ_PROCEDURE_ID )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_PROCEDURE_ID\n" );
	if ( verboseLevel > 1 )
	    SIfprintf( outputf, "\tgq_procedureHandle = 0x%lx\n", 
		       getq->gq_procedureHandle );
   }

   if ( getq->gq_mask & IIAPI_GQ_REPEAT_QUERY_ID )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_GQ_REPEAT_QUERY_ID\n" );
	if ( verboseLevel > 1 )
	    SIfprintf( outputf, "\tgq_repeatQueryHandle = 0x%lx\n", 
		       getq->gq_repeatQueryHandle );
   }

   if ( getq->gq_mask & IIAPI_GQ_TABLE_KEY )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_TABLE_KEY\n" );
	SIfprintf( outputf, "\tgq_tableKey = " );
	ait_formatString( getq->gq_tableKey, IIAPI_TBLKEYSZ );
   }

   if ( getq->gq_mask & IIAPI_GQ_OBJECT_KEY )
   {
	SIfprintf( outputf, "\tgq_mask = IIAPI_GQ_OBJECT_KEY\n" );
	SIfprintf( outputf, "\tgq_objectKey = " );
	ait_formatString( getq->gq_objectKey, IIAPI_OBJKEYSZ );
   }

   return;
}






/*
** Name: api_initialize
**
** Description:
**	Output results of IIapi_initialize().
**
** Inputs:
**	aitcb		Tester control block.
**	parmBlock	IIapi_initialize() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
api_initialize( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_INITPARM	*init = (IIAPI_INITPARM *)parmBlock;

    AITapi_results( init->in_status, NULL );

    if ( init->in_version >= IIAPI_VERSION_2  &&  verboseLevel > 1 )
	SIfprintf( outputf, "\tin_envHandle = 0x%lx\n", init->in_envHandle );

    return;
}






/*
** Name: api_query
**
** Description:
**	Output results of IIapi_query().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_query() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Print qy_tranHandle and qy_stmtHandle
**	    only when "-verbose" is set.
*/

static VOID
api_query( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_QUERYPARM	*qury = (IIAPI_QUERYPARM *)parmBlock;
 
    if ( verboseLevel > 1 )
    {
	SIfprintf( outputf, "\tqy_tranHandle = 0x%lx\n", qury->qy_tranHandle );
	SIfprintf( outputf, "\tqy_stmtHandle = 0x%lx\n", qury->qy_stmtHandle );
    }

    return;
}






/*
** Name: api_registerXID
**
** Description:
**	Output results of IIapi_registerXID().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_registerXID() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	14-Jun-95 (gordy)
**	    Created.
**	14-Jul-95 (feige)
**	    Print rg_tranIdHandle only
**	    when "-verbose" is set.
*/

static VOID
api_registerXID( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_REGXIDPARM	*regXID = (IIAPI_REGXIDPARM *)parmBlock;

    AITapi_results( regXID->rg_status, NULL );

    if ( verboseLevel > 1 )
	SIfprintf(outputf, "\trg_tranIdHandle = 0x%lx\n", regXID->rg_tranIdHandle);

    return;
}






/*
** Name: api_releaseEnv
**
** Description:
**	Output results of IIapi_releaseEnv().
**
** Inputs:
**	aitcb		Tester control block.
**	parmBlock	IIapi_releaseEnv() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	10-Feb-97 (gordy)
**	    Created.
*/

static VOID
api_releaseEnv( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_RELENVPARM	*relEnv = (IIAPI_RELENVPARM *)parmBlock;

    AITapi_results( relEnv->re_status, NULL );

    return;
}




/*
** Name: api_releaseXID
**
** Description:
**	Output results of IIapi_releaseXID().
**
** Inputs:
**	aitcb		Tester control block.
**	parmBlock	IIapi_releaseXID() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	14-Jun-95 (gordy)
**	    Created.
*/

static VOID
api_releaseXID( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_RELXIDPARM	*relXID = (IIAPI_RELXIDPARM *)parmBlock;

    AITapi_results( relXID->rl_status, NULL );

    return;
}






/*
** Name: api_savePoint
**
** Description:
**	Output results of IIapi_savePoint().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_savePoint() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Print sp_savePointHandle
**	    only when "-verbose" is set.
*/

static VOID
api_savePoint( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_SAVEPTPARM	*savept = (IIAPI_SAVEPTPARM *)parmBlock;

    if ( verboseLevel > 1 )
	SIfprintf( outputf, "\tsp_savePointHandle = 0x%lx\n", 
		   savept->sp_savePointHandle );

    return;
}






/*
** Name: api_setConnectParam
**
** Description:
**	Output results of IIapi_setConnectParam().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_setConnectParam() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Print sc_connHandle only when
**	    "-verbose" is set.
*/

static VOID
api_setConnectParam( AITCB *aitcb, II_PTR parmBlock)
{
    IIAPI_SETCONPRMPARM	*setc = (IIAPI_SETCONPRMPARM *)parmBlock;

    if ( verboseLevel > 1 )
	SIfprintf( outputf, "\tsc_connHandle = 0x%lx\n", setc->sc_connHandle );

    return;
}


/*
** Name: api_setEnvParam
**
** Description:
**	Output results of IIapi_setEnvParam().
**
** Inputs
**	aitcb		Tester control block.
**	parmBlock	IIapi_setConnectParam() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
** 	19-Aug-98 (rajus01)
**	    Created.
*/
static VOID
api_setEnvParam( AITCB *aitcb, II_PTR parmBlock)
{
    IIAPI_SETENVPRMPARM	*sete = (IIAPI_SETENVPRMPARM *)parmBlock;

    AITapi_results( sete->se_status, NULL );

    return;
}



/*
** Name: api_terminate
**
** Description:
**	Output results of IIapi_terminate().
**
** Inputs:
**	aitcb		Tester control block.
**	parmBlock	IIapi_terminate() parameter block.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
api_terminate( AITCB *aitcb, II_PTR parmBlock )
{
    IIAPI_TERMPARM	*term = (IIAPI_TERMPARM *)parmBlock;

    AITapi_results( term->tm_status, NULL );

    return;
}






/*
** Name: ait_printData - print column data
**
** Description:
**	This function translate column data into ASCII string based on its
**     data type.
**
** Input:
**	descrptor  - descriptor of the column data
**	dataValue  - column data
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	13-Jun-95 (gordy)
**	    Fixed printing of long datatypes.
**	12-Jul-02 (gordy)
**	    Removed buffer parameter.  Print directly to output.
*/

VOID
ait_printData 
( 
    IIAPI_DESCRIPTOR  	*descriptor,
    IIAPI_DATAVALUE   	*dataValue
)
{
    IIAPI_CONVERTPARM	cv;
    char		buffer[128];
    char	    	*buf = buffer;
    i4		    	i;
    II_INT2		length;
    II_FLOAT4	    	*float4;
    II_FLOAT8 	    	*float8;
    II_INT1	    	*integer1;
    II_INT2	    	*integer2;
    II_INT4	    	*integer4;

    if ( dataValue->dv_null )
    {
        SIfprintf( outputf, "( NULL )\n" );
	return;
    }
    
    switch( abs( descriptor->ds_dataType ) )
    {
	case IIAPI_LOGKEY_TYPE:
	case IIAPI_TABKEY_TYPE:
	    for( i = 0; i < dataValue->dv_length; i++ )
	    {
		char c, d;

		c = ( ( II_CHAR * ) dataValue->dv_value )[i];
		d = ( c >> 4 ) & 0x0f;
		*buf++ = ( d < 10 ) ? d + '0' : (d - 10) + 'A';
		d = c & 0x0f;
		*buf++ = ( d < 10 ) ? d + '0' : (d - 10) + 'A';
		*buf++ = ' ';
	    }
	    *buf = '\0';
	    SIfprintf( outputf, "%s\n", buffer );
	    break;

	case IIAPI_LBYTE_TYPE:
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
	case IIAPI_LTXT_TYPE:
	case IIAPI_LVCH_TYPE:
	case IIAPI_TXT_TYPE:
	case IIAPI_VBYTE_TYPE:
	case IIAPI_VCH_TYPE:
	    MEcopy( dataValue->dv_value, sizeof( length ), (PTR)&length );
	    ait_formatString( (char *)dataValue->dv_value + sizeof( length ), 
			      (i4)length );
	    break;

	case IIAPI_BYTE_TYPE:
	case IIAPI_NBR_TYPE:
	case IIAPI_CHA_TYPE:
	case IIAPI_CHR_TYPE:
	    ait_formatString( (char *)dataValue->dv_value, 
			      (i4)dataValue->dv_length );
	    break;

	case IIAPI_FLT_TYPE:
	    switch( dataValue->dv_length )
	    {
		case 4:
		    float4 = (II_FLOAT4 *)dataValue->dv_value;
		    SIfprintf( outputf, "%f\n", (II_FLOAT8)*float4, '.' );
		    break;

		case 8:
		    float8 = (II_FLOAT8 *)dataValue->dv_value;
		    SIfprintf( outputf, "%f\n", *float8, '.' );
		    break;

		default:
		    SIfprintf( outputf, "invalid float length\n" );
		    break;
	    }
	    break;

	case IIAPI_INT_TYPE:
	    switch( dataValue->dv_length )
	    {
		case 1:
		    integer1 = (II_INT1 *)dataValue->dv_value;
		    SIfprintf( outputf, "%d\n", (i4)*integer1 );
		    break;

		case 2:
		    integer2 = (II_INT2 *)dataValue->dv_value;
		    SIfprintf( outputf, "%d\n", (i4)*integer2 );
		    break;

		case 4:
		    integer4 = (II_INT4 *)dataValue->dv_value;
		    SIfprintf( outputf, "%ld\n", *integer4 );
		    break;

		default:
		    SIfprintf( outputf, "invalid integer length\n" );
		    break;
	    }
	    break;

	case IIAPI_DTE_TYPE:
	case IIAPI_DEC_TYPE:
	case IIAPI_MNY_TYPE:
	    cv.cv_srcDesc.ds_dataType = descriptor->ds_dataType;
	    cv.cv_srcDesc.ds_nullable = descriptor->ds_nullable;
	    cv.cv_srcDesc.ds_length = descriptor->ds_length;
	    cv.cv_srcDesc.ds_precision = descriptor->ds_precision;
	    cv.cv_srcDesc.ds_scale = descriptor->ds_scale;
	    cv.cv_srcDesc.ds_columnType = descriptor->ds_columnType;
	    cv.cv_srcDesc.ds_columnName = descriptor->ds_columnName;

	    cv.cv_srcValue.dv_null = dataValue->dv_null;
	    cv.cv_srcValue.dv_length = dataValue->dv_length;
	    cv.cv_srcValue.dv_value = dataValue->dv_value;

	    cv.cv_dstDesc.ds_dataType = IIAPI_CHA_TYPE;
	    cv.cv_dstDesc.ds_nullable = FALSE;
	    cv.cv_dstDesc.ds_length = MAX_VAR_LEN;
	    cv.cv_dstDesc.ds_precision = 0;
	    cv.cv_dstDesc.ds_scale = 0;
	    cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
	    cv.cv_dstDesc.ds_columnName = NULL;

	    cv.cv_dstValue.dv_null = FALSE;
	    cv.cv_dstValue.dv_length = cv.cv_dstDesc.ds_length;
	    cv.cv_dstValue.dv_value = buffer;

	    IIapi_convertData( &cv );

	    if ( cv.cv_status != IIAPI_ST_SUCCESS )
	    {
		SIfprintf( outputf, "(invalid type)\n" );
		break;
	    }

	    buffer[ cv.cv_dstValue.dv_length ] = '\0';
	    SIfprintf( outputf, "%s\n", buffer );
	    break;
	
	default:
	    SIfprintf( outputf, "invalid type\n" );
	    break;
    }
    
    return;
}




/* Name: ait_formatString
**
** Description:
**	Format a string value for display.
**
** Input:
**	string		String to be displayed.
**	length		Length of string.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	13-Jun-95 (gordy)
**	    Created.
**	12-Jul-02 (gordy)
**	    Removed buffer parameter.  Print directly to output.
*/

static VOID
ait_formatString( char *string, i4  length )
{
    i4  i, j;
    char	buff[ 512 ];
    char	*buffer = buff;

    if ( length > 72 )  *buffer++ = '\n';
    *buffer++ = '"';

    for ( j = 72; length > 0; length--, string++, j-- )
    {
	if ( ! j )
	{
	    *buffer++ = '"';
	    *buffer = '\0';
	    SIfprintf( outputf, "%s\n", buff );
	    buffer = buff;
	    *buffer++ = '"';
	    j = 72;
	}

	if ( CMprint( string ) )
	    *buffer++ = *string;
	else
	{
	    STprintf( buffer, "\\%03.03d", *string );
	    buffer += 4;
	}
    }

    *buffer++ = '"';
    *buffer = '\0';
    SIfprintf( outputf, "%s\n", buff );

    return;
}



/*
** Name: print_descr() - descriptor printing function
**
** Description:
**	This function prints descriptor value.
**
** Inputs
**	var1  - partial name of the descriptor variable
**	index - index to the descriptor array
**	var2  - partial name of the descriptor variable
**	descr - descriptor
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	23-Feb-96 (gordy)
**	    Converted descriptor parameter to a pointer.
*/

static VOID
print_descr
( 
    char 		*var1, 
    i4  		index, 
    char 		*var2, 
    IIAPI_DESCRIPTOR 	*descr 
)
{
    i4		i;

    for ( i = 0; i < MAX_DT_ID; i++ )
    	if ( descr->ds_dataType == DT_ID_TBL[i].longVal )
    	{
	    SIfprintf( 
	    	outputf,
		"\t%s[%d]%s.ds_dataType = %d %s\n",
		var1,
		index,
		var2,
		(i4)descr->ds_dataType,
		DT_ID_TBL[i].symbol );

	    break;
	}

    if ( i == MAX_DT_ID )
	SIfprintf( 
	    outputf,
	    "\t%s[%d]%s.ds_dataType = %d %s\n",
	    var1,
	    index,
	    var2,
	    (i4)descr->ds_dataType,
	    "unknown dataType" );

    if ( descr->ds_nullable == TRUE )
	SIfprintf( 
	    outputf,
	    "\t%s[%d]%s.ds_nullable = TRUE\n",
	    var1,
	    index,
	    var2 );
    else 
    	SIfprintf( 
	    outputf,
	    "\t%s[%d]%s.ds_nullable = FALSE\n",
	    var1,
	    index,
	    var2 );

    SIfprintf( 
	outputf,
	"\t%s[%d]%s.ds_length = %ld\n",
	var1,
	index,
	var2,
	descr->ds_length
	); 

    SIfprintf( 
	outputf,
	"\t%s[%d]%s.ds_precision = %ld\n",
	var1,
	index,
	var2,
	descr->ds_precision
	); 

    SIfprintf( 
	outputf,
	"\t%s[%d]%s.ds_scale = %ld\n",
	var1,
	index,
	var2,
	descr->ds_scale
	); 


    for ( i = 0; i < MAX_ds_columnType; i++ )
    {
    	if ( descr->ds_columnType 
		 == ds_columnType_TBL[i].longVal )
	{
	    SIfprintf( 
	    	outputf,
		"\t%s[%d]%s.ds_columnType = %ld %s\n",
		var1,
		index,
		var2,
		descr->ds_columnType,
		ds_columnType_TBL[i].symbol );

	    break;
	}
     }

    if ( i == MAX_ds_columnType )
	SIfprintf( 
	    outputf,
	    "\t%s[%d]%s.ds_columnType = %ld %s\n",
	    var1,
	    index,
	    var2,
	    descr->ds_columnType,
	    "unknown columnType" );

    SIfprintf( 
	outputf,
	"\t%s[%d]%s.ds_columnName = %s\n",
	var1,
	index,
	var2,
	descr->ds_columnName
	); 

    return;
}

/*
** Name: ait_printfmtData - print column data
**
** Description:
**	This function translate column data into ASCII string based on its
**      data type. Similar to ait_printData() except that it takes an
**	environment handle.
**
** Input:
**	descrptor  - descriptor of the column data
**	dataValue  - column data
**	envHndl	   - Environment handle.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	11-Sep-98 (rajus01)
**	12-Jul-02 (gordy)
**	    Removed buffer parameter.  Print directly to output.
*/
static VOID
ait_printfmtData 
( 
    PTR			envHndl,
    IIAPI_DESCRIPTOR  	*descriptor,
    IIAPI_DATAVALUE   	*dataValue
)
{
    IIAPI_FORMATPARM	fmt;
    char		buffer[ 128 ];
    char	    	*buf = buffer;
    i4		    	i;
    II_INT2		length;
    II_FLOAT4	    	*float4;
    II_FLOAT8 	    	*float8;
    II_INT1	    	*integer1;
    II_INT2	    	*integer2;
    II_INT4	    	*integer4;

    fmt.fd_envHandle = envHndl; 

    if ( dataValue->dv_null )
    {
	SIfprintf( outputf, "( NULL )" );
	return;
    }
    
    switch( abs( descriptor->ds_dataType ) )
    {
	case IIAPI_LOGKEY_TYPE:
	case IIAPI_TABKEY_TYPE:
	    for( i = 0; i < dataValue->dv_length; i++ )
	    {
		char c, d;

		c = ( ( II_CHAR * ) dataValue->dv_value )[i];
		d = ( c >> 4 ) & 0x0f;
		*buf++ = ( d < 10 ) ? d + '0' : (d - 10) + 'A';
		d = c & 0x0f;
		*buf++ = ( d < 10 ) ? d + '0' : (d - 10) + 'A';
		*buf++ = ' ';
	    }
	    *buf = '\0';
	    SIfprintf( outputf, "%s\n", buf );
	    break;

	case IIAPI_LBYTE_TYPE:
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
	case IIAPI_LTXT_TYPE:
	case IIAPI_LVCH_TYPE:
	case IIAPI_TXT_TYPE:
	case IIAPI_VBYTE_TYPE:
	case IIAPI_VCH_TYPE:
	    MEcopy( dataValue->dv_value, sizeof( length ), (PTR)&length );
	    ait_formatString( (char *)dataValue->dv_value + sizeof( length ), 
			      (i4)length );
	    break;

	case IIAPI_BYTE_TYPE:
	case IIAPI_NBR_TYPE:
	case IIAPI_CHA_TYPE:
	case IIAPI_CHR_TYPE:
	    ait_formatString( (char *)dataValue->dv_value, 
			      (i4)dataValue->dv_length );
	    break;

	case IIAPI_FLT_TYPE:
	    switch( dataValue->dv_length )
	    {
		case 4:
		    float4 = (II_FLOAT4 *)dataValue->dv_value;
		    SIfprintf( outputf, "%f\n", (II_FLOAT8)*float4, '.' );
		    break;

		case 8:
		    float8 = (II_FLOAT8 *)dataValue->dv_value;
		    SIfprintf( outputf, "%f\n", *float8, '.' );
		    break;

		default:
		    SIfprintf( outputf, "invalid float length\n" );
		    break;
	    }
	    break;

	case IIAPI_INT_TYPE:
	    switch( dataValue->dv_length )
	    {
		case 1:
		    integer1 = (II_INT1 *)dataValue->dv_value;
		    SIfprintf( outputf, "%d\n", (i4)*integer1 );
		    break;

		case 2:
		    integer2 = (II_INT2 *)dataValue->dv_value;
		    SIfprintf( outputf, "%d\n", (i4)*integer2 );
		    break;

		case 4:
		    integer4 = (II_INT4 *)dataValue->dv_value;
		    SIfprintf( outputf, "%ld\n", *integer4 );
		    break;

		default:
		    SIfprintf( outputf, "invalid integer length\n" );
		    break;
	    }
	    break;

	case IIAPI_DTE_TYPE:
	case IIAPI_DEC_TYPE:
	case IIAPI_MNY_TYPE:
	    api_hexdump( (char *) dataValue->dv_value, dataValue->dv_length );
	    break;
	
	default:
	    SIfprintf( outputf, "invalid type\n" );
	    break;
    }
    
    return;
}


/*
** Name: api_hexdump -  Produces hex dump of the input buffer.
**
** Description:
**	Outputs hex values for DECIMAL, DATE, MONEY data types.
**
** Input:
**	ibuf	   - input buffer
**	len	   - input buffer length.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	10-Feb-99 (rajus01)
**	12-Jul-02 (gordy)
**	    Removed buffer parameter.  Print directly to output.
*/

static VOID   
api_hexdump( char *ibuf, i4  len )
{
    char *hexchar = "0123456789ABCDEF";
    char hex[ 49 ]; /* 3 * 16 + \0 */
    char asc[ 17 ];

    i4  i = 0;

    for( ; len--; ibuf++ )
    {
	hex[3*i] = hexchar[ (*ibuf >> 4) & (char)0x0F ];
	hex[3*i+1] = hexchar[ *ibuf & (char)0x0F ];
	hex[3*i+2] =  ' ';
	asc[i++] = CMprint(ibuf) && !(*ibuf & (char)0x80) ? *ibuf : '.' ;
	hex[3*i] = asc[i] = '\0';

	if( !( i %= 16 ) || !len )
	    SIfprintf(outputf, "%s %s\n", hex, asc );
    }

    return;
}
