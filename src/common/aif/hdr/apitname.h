/*
** Copyright (c) 2004, 2006 Ingres Corporation
*/

/*
** Name: apitname.h
**
** Description:
**	This file contains the definition of the transaction name.
**
** History:
**      14-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleanup common handle structures.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	17-Jan-96 (gordy)
**	    Added environment handles.
*/

# ifndef __APITNAME_H__
# define __APITNAME_H__

# include <apienhnd.h>


/*
** Name: IIAPI_TRAN_NAME
**
** Description:
**      This data type defines the transaction name handle.
**
** History:
**      14-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleanup common handle structures.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	10-Jul-06 (gordy)
**	    Changed parameters to IIapi_createTranName().
*/

typedef struct _IIAPI_TRANNAME
{

    IIAPI_HNDLID	tn_id;
    IIAPI_ENVHNDL	*tn_envHndl;
    IIAPI_TRAN_ID	tn_tranID;
    QUEUE		tn_tranHndlList;

} IIAPI_TRANNAME;




/*
** Global Functions.
*/

II_EXTERN II_BOOL	IIapi_initTranName( II_VOID );

II_EXTERN II_VOID	IIapi_termTranName( II_VOID );

II_EXTERN IIAPI_TRANNAME *IIapi_createTranName( IIAPI_TRAN_ID *tranID,
						IIAPI_ENVHNDL *envHndl );

II_EXTERN II_VOID	IIapi_deleteTranName( IIAPI_TRANNAME *tranName );

II_EXTERN II_BOOL	IIapi_isTranName( IIAPI_TRANNAME *tranName );

# endif /* __APITNAME_H__ */
