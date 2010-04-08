/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitbind.h - aitbind.c shared variables and functions header file
**
** Description:
**	Aitbind declarations.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Removed things local to aitbind.c and declared extern functions.
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	13-Mar-97 (gordy)
**	    Added IIapi_releaseEnv().
**	19-Aug-98 (rajus01)
**	    Added IIapi_setEnvParam(), IIapi_abort(), IIapi_formatData().
**	11-Feb-99 (rajus01)
**	   Added formatData_srcVal().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





# ifndef __AITBIND_H__
# define __AITBIND_H__





/*
** External functions.
*/
FUNC_EXTERN VOID 	var_assgn_abort( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID	var_assgn_auto( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_cancel( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_catchEvent( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_close( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_commit( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_connect( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_disconnect( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getColumns( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getCopyMap( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getDescriptor( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getErrorInfo( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getQueryInfo( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_getEvent( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_initialize( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_modifyConnect( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_prepareCommit( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_putColumns( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_putParms( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_query( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_registerXID( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_releaseEnv( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_releaseXID( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_rollback( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_savePoint( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_setConnectParam(i4 parmIdx, i4  parmArrIdx);
FUNC_EXTERN VOID 	var_assgn_setDescriptor( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_terminate( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_wait( i4  parmIdx, i4  parmArrIdx );
FUNC_EXTERN VOID 	var_assgn_setEnvParam(i4 parmIdx, i4  parmArrIdx);
FUNC_EXTERN VOID 	var_assgn_frmtData(i4 parmIdx, i4  parmArrIdx);
FUNC_EXTERN VOID        formatData_srcVal( char *line_buf );





# endif			/* __AITBIND_H__ */
