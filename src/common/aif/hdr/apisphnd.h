/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apisphnd.h
**
** Description:
**	This file contains the definition of the savepoint handle.
**
** History:
**      14-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
*/

# ifndef __APISPHND_H__
# define __APISPHND_H__

# include <apithndl.h>


/*
** Name: IIAPI_SAVEPTHNDL
**
** Description:
**      This data type defines the save point handle.
**
** History:
**      14-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
*/

typedef struct IIAPI_SAVEPTHNDL
{

    IIAPI_HNDLID	sp_id;
    char		*sp_savePtName;

} IIAPI_SAVEPTHNDL;



/*
** Global functions.
*/

II_EXTERN II_BOOL	IIapi_createSavePtHndl( IIAPI_SAVEPTPARM *savePtParm );

II_EXTERN II_VOID	IIapi_deleteSavePtHndl( IIAPI_SAVEPTHNDL *savePtHndl );

II_EXTERN II_BOOL	IIapi_isSavePtHndl( IIAPI_SAVEPTHNDL *savePtHndl, 
					    IIAPI_TRANHNDL *tranHndl );

# endif				/* __APISPHND_H__ */
