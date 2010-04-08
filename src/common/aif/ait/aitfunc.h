/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitfunc.h
**
** Description:
**	API Tester declarations for API function access.
**
** History:
**	27-Mar-95 (gordy)
**	    Created.
**	28-Apr-95 (feige)
**	    Updated field definitions of IIAPI_CATCHEVENTPARM.
**	14-Jun-95 (gordy)
**	    Finalized Two Phase Commit interface.
**	26-Feb-96 (feige)
**	    Updated COPYMAP interface, added two more macros:
**	    _gm_copyMap_cp_dbmsCount and _gm_copyMap_cp_dbmsDescr.
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	13-Mar-97 (gordy)
**	    Added IIapi_releaseEnv(), in_envHandle to IIapi_initialize(),
**	    and co_type to IIapi_connect().
**	19-Aug-98 (rajus01)
**	    Added IIapi_setEnvParam(), IIapi_abort(), IIapi_formatData().
**	03-Feb-98 (rajus01)
**	    Made ait_printData() as an external function.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Jul-02 (gordy)
**	    Removed buffer parameter from ait_printData() which now
**	    prints directly to output.
*/

# ifndef __AITFUNC_H__
# define __AITFUNC_H__



/*
** define IIapi function numbers
** IIapi_convertData is not included
*/

# define	_IIapi_autocommit		0
# define	_IIapi_cancel			1
# define	_IIapi_catchEvent		2
# define	_IIapi_close			3
# define	_IIapi_commit			4
# define	_IIapi_connect			5
# define	_IIapi_disconnect		6
# define	_IIapi_getColumns		7
# define	_IIapi_getCopyMap		8
# define	_IIapi_getDescriptor		9
# define	_IIapi_getErrorInfo		10
# define	_IIapi_getQueryInfo		11
# define	_IIapi_getEvent			12
# define	_IIapi_initialize		13
# define	_IIapi_modifyConnect		14
# define	_IIapi_prepareCommit		15
# define	_IIapi_putColumns		16
# define	_IIapi_putParms			17
# define	_IIapi_query			18
# define	_IIapi_registerXID		19
# define	_IIapi_releaseEnv		20
# define	_IIapi_releaseXID		21
# define	_IIapi_rollback			22
# define	_IIapi_savePoint		23
# define	_IIapi_setConnectParam		24
# define	_IIapi_setEnvParam		25
# define	_IIapi_setDescriptor		26
# define	_IIapi_terminate		27
# define	_IIapi_wait			28
# define	_IIapi_abort			29
# define	_IIapi_formatData		30



# define	MAX_FUN_NUM			31
# define	MAX_FIELD_NUM			13

/*
** define field numbers in each IIAPI_ parameter block
*/

/* 0--define fields of IIAPI_AUTOPARM */
# define	_ac_connHandle			0
# define	_ac_tranHandle			1

/* 1--define fields of IIAPI_CANCELPARM */
# define	_cn_stmtHandle			0

/* 2--define fields of IIAPI_CATCHEVENTPARM */
# define	_ce_connHandle			0
# define	_ce_selectEventName		1
# define	_ce_selectEventOwner		2
# define	_ce_eventHandle			3
# define	_ce_eventName			4
# define	_ce_eventOwner			5
# define	_ce_eventDB			6
# define	_ce_eventTime			7
# define	_ce_eventInfoAvail		8

/* 3--define fields of IIAPI_CLOSEPARM */
# define	_cl_stmtHandle			0

/* 4--define fields of IIAPI_COMMITPARM */
# define	_cm_tranHandle			0

/* 5--define fields of IIAPI_CONNPARM */
# define	_co_target			0
# define	_co_username			1
# define	_co_password			2
# define	_co_timeout			3
# define	_co_connHandle			4
# define	_co_tranHandle			5
# define	_co_sizeAdvise			6
# define	_co_apiLevel			7
# define	_co_type			8


/* 6--define fields of IIAPI_DISCONNPARM */
# define	_dc_connHandle			0

/* 7--define fields of IIAPI_GETCOLPARM */
# define	_gc_stmtHandle			0
# define	_gc_rowCount			1
# define	_gc_columnCount			2
# define	_gc_columnData			3
# define	_gc_rowsReturned		4
# define	_gc_moreSegments		5

/* 8--define fields of IIAPI_GETCOPYMAPPARM */
# define 	_gm_stmtHandle			0
# define	_gm_copyMap			1
# define	_gm_copyMap_cp_dbmsCount	2
# define	_gm_copyMap_cp_dbmsDescr	3

/* 9--define fields of IIAPI_GETDESCRPARM */
# define	_gd_stmtHandle			0
# define	_gd_descriptorCount		1
# define	_gd_descriptor			2

/* 10-define fields of IIAPI_GETEINFOPARM */
# define	_ge_stmtHandle			0
# define	_ge_dbmsSQLState		1
# define	_ge_dbmsNativeError		2
# define	_ge_severID			3
# define	_ge_severType			4
# define	_ge_severity			5
# define	_ge_errorType			6
# define	_ge_parmCount			7
# define	_ge_parm			8
# define	_ge_status			9
# define	_ge_ge_sQLState			10
# define	_ge_nativeError			11

/* 11--define fields of IIAPI_GETQINFOPARM */
# define	_gq_stmtHandle			0
# define	_gq_flags			1
# define	_gq_mask			2
# define	_gq_rowCount			3
# define	_gq_readonly			4
# define	_gq_procedureReturn		5
# define	_gq_procedureHandle		6
# define	_gq_repeatQueryHandle		7
# define	_gq_tableKey			8	/* ??? */
# define	_gq_objectKey			9	/* ??? */

/* 12--define fields of IIAPI_GETEVENTPARM */
# define	_gv_connHandle			0
# define	_gv_timeout			1

/* 13--define fields of IIAPI_INITPARM */
# define	_in_timeout			0
# define	_in_version			1
# define	_in_envHandle			2

/* 14--define fields of IIAPI_MODCONNPARM */
# define	_mc_connHandle			0

/* 15--define fields of IIAPI_PREPCMTPARM */
# define	_pr_tranHandle			0

/* 16--define fields of IIAPI_PUTCOLPARM */
# define	_pc_stmtHandle			0
# define	_pc_columnCount			1
# define	_pc_columnData			2
# define	_pc_moreSegments		3

/* 17--define fields of IIAPI_PUTPARMPARM */
# define	_pp_stmtHandle			0
# define	_pp_parmCount			1
# define	_pp_parmData			2
# define	_pp_moreSegments		3
#
/* 18--define fields of IIAPI_QUERYPARM */
# define	_qy_connHandle			0
# define	_qy_queryType			1
# define	_qy_queryText			2
# define	_qy_parameters			3
# define	_qy_tranHandle			4
# define	_qy_stmtHandle			5

/* 19--define fields of IIAPI_REGXIDPARM */
# define	_ti_type			0
# define	_it_highTran			1
# define	_it_lowTran			2
# define	_rg_tranIdHandle		3
# define	_rg_status			4

/* 20--define fields of IIAPI_RELENVPARM */
# define	_re_envHandle			0
# define	_re_status			1

/* 21--define fields of IIAPI_RELXIDPARM */
# define	_rl_tranIdHandle		0
# define	_rl_status			1

/* 22--define fields of IIAPI_ROLLBACKPARM */
# define	_rb_tranHandle			0
# define	_rb_savePointHandle		1

/* 23--define fields of IIAPI_SAVEPTPARM */
# define	_sp_tranHandle			0
# define	_sp_savePoint			1
# define	_sp_savePointHandle		2

/* 24--define fields of IIAPI_SETCONPRMPARM */
# define	_sc_connHandle			0
# define	_sc_paramID			1
# define	_sc_paramValue			2

/* 25--define fields of IIAPI_SETENVPRMPARM */
# define	_se_envHandle			0
# define	_se_paramID			1
# define	_se_paramValue			2
# define	_se_status			3

/* 26--define fields of IIAPI_SETDESCRPARM */
# define	_sd_stmtHandle			0
# define	_sd_descriptorCount		1
# define	_sd_descriptor			2

/* 27--define fields of IIAPI_TERMPARM */
# define	_tm_status			0

/* 28--define fields of IIAPI_WAITPARM */
# define	_wt_timeout			0

/* 29--define fields of IIAPI_ABORTPARM */
# define	_ab_connHandle			0

/* 30--define fields of IIAPI_FORMATPARM */
# define	_fd_envHandle			0
# define	_fd_srcDesc			1
# define	_fd_srcValue			2
# define	_fd_dstDesc			3
# define	_fd_dstValue			4
# define	_fd_status			5




/*
** Name: apiFunInfo - information for API function and parameter 
**
** Description:
**	This type defines API function name, parameter name, paramater
**	field name, size of parameter block, and the order for value
**	assignment if any
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Added function pointers.
*/

typedef struct	_apiFunInfo
{	

    char	*funName;		/* Name of the function */
    i4		flags;			/* Additional info */
    void	(*api_func)();		/* API function pointer */
    void	(*results)();		/* API results processing function */
    char	*parmBlockName;		/* Name of the parameter block */
    i4		parmBlockSize;		/* sizeof(IIAPI_xxxxPARM) */
    char	*parmName[MAX_FIELD_NUM];
					/* field names in parameter block */

    /* 
    ** the index to the field in parmName
    ** array which can be assigned value
    ** only if the value(s) of some other
    ** field(s) have been decided. This is
    ** implemented in the way that this
    ** field is always the last one to
    ** assign value no mater what field
    ** assignment order is used in the
    ** input file.
    ** -1 if no order required.
    */
    i4		orderReqedParm; 	

} API_FUNC_INFO;


/* 
** Additional info flags
*/

#define	AIT_SYNC	0x01
#define	AIT_ASYNC	0x02





/*
** Global Variables.
*/

GLOBALREF API_FUNC_INFO funTBL[ MAX_FUN_NUM ]; 





/*
** External Functions.
*/

FUNC_EXTERN VOID	AITapi_results( IIAPI_STATUS status, II_PTR errHndl );
FUNC_EXTERN VOID	ait_printData( IIAPI_DESCRIPTOR *descriptor,
				IIAPI_DATAVALUE *datavalue );


# endif				/* __AITFUNC_H__ */
