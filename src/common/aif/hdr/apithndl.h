/*
** Copyright (c) 2004, 2006 Ingres Corporation
*/

/*
** Name: apithndl.h
**
** Description:
**	This file contains the definition of the transaction handle.
**
** History:
**      14-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	21-Oct-94 (gordy)
**	    Fix savepoint processing.
**	31-Oct-94 (gordy)
**	    Removed unused member th_end.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	14-Jun-95 (gordy)
**	    Added th_tranNameQue so that the transaction handle
**	    can be placed on the IIAPI_TRANNAME Queue.
**	 9-Jul-96 (gordy)
**	    Added support for autocommit transactions.
**	 2-Oct-96 (gordy)
**	    Simplified function callbacks.
**	11-Jul-06 (gordy)
**	    Added IIapi_findTranHndl().
*/

# ifndef __APITHNDL_H__
# define __APITHNDL_H__

# include <apichndl.h>
# include <apitname.h>


/*
** Name: IIAPI_TRANHNDL
**
** Description:
**      This data type defines the transaction handle.
**
** History:
**      14-sep-93 (ctham)
**          creation
**	21-Oct-94 (gordy)
**	    Removed unused member th_savePtHndl.
**	31-Oct-94 (gordy)
**	    Removed unused member th_end.
**	14-Jun-95 (gordy)
**	    Added th_tranNameQue so that the transaction handle
**	    can be placed on the IIAPI_TRANNAME Queue.
**	 9-Jul-96 (gordy)
**	    Added support for autocommit transactions.
**	 2-Oct-96 (gordy)
**	    Simplified function callbacks.
**	 3-Sep-98 (gordy)
**	    Added function to abort handle.
*/

typedef struct IIAPI_TRANHNDL
{
    IIAPI_HNDL		th_header;
    
    /*
    ** Transaction specific parameters
    */
    IIAPI_CONNHNDL	*th_connHndl;	/* parent */
    IIAPI_TRANNAME	*th_tranName;	/* if unique global name is used */
    QUEUE		th_tranNameQue;	/* For inclusion on tranName Queue */
    
    /*
    ** Handle Queues
    */
    QUEUE		th_stmtHndlList;
    QUEUE		th_savePtHndlList;
    
    /*
    ** API Function call info.
    */
    II_BOOL		th_callback;
    IIAPI_GENPARM	*th_parm;

} IIAPI_TRANHNDL;



/*
** Global functions.
*/

II_EXTERN IIAPI_TRANHNDL 	*IIapi_createTranHndl(IIAPI_TRANNAME *tranName,
						      IIAPI_CONNHNDL *connHndl);

II_EXTERN II_VOID 		IIapi_deleteTranHndl(IIAPI_TRANHNDL *tranHndl);

II_EXTERN IIAPI_TRANHNDL	*IIapi_findTranHndl( IIAPI_CONNHNDL *connHndl,
						     IIAPI_TRAN_ID *tranID );

II_EXTERN II_VOID		IIapi_abortTranHndl( IIAPI_TRANHNDL *tranHndl,
						     II_LONG errorCode,
						     char *SQLState,
						     IIAPI_STATUS status );

II_EXTERN II_BOOL		IIapi_isTranHndl( IIAPI_TRANHNDL *tranHndl );

II_EXTERN IIAPI_TRANHNDL	*IIapi_getTranHndl( IIAPI_HNDL *handle );

# endif /* __APITHNDL_H__ */

