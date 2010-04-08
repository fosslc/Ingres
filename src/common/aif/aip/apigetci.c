/*
** Copyright (c) 2008 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apishndl.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apigetci.c
**
** Description:
**	This file contains the IIapi_getColumnInfo() function interface.
**
**      IIapi_getColumnInfo  API get column information function
**
** History:
**	11-Aug-08 (gordy)
**	    Created.
*/




/*
** Name: IIapi_getColumnInfo
**
** Description:
**	This function provide an interface for the frontend application
**      to retrieve response data for a specific query.  Please refere to
**      the API user specification for function description.
**
**      getColInfoParm  input and output parameters for IIapi_getColumnInfo()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	11-Aug-08 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getColumnInfo( IIAPI_GETCOLINFOPARM II_FAR *getColInfoParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    II_UINT2		colIndex;

    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_getColumnInfo: return column information\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! getColInfoParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumnInfo: null getColInfo parameters\n" );
	return;
    }
    
    getColInfoParm->gi_status = IIAPI_ST_NO_DATA;
    getColInfoParm->gi_mask = 0;
    stmtHndl = (IIAPI_STMTHNDL *)getColInfoParm->gi_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumnInfo: API is not initialized\n" );
	getColInfoParm->gi_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }
    
    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_getColumnInfo: invalid statement handle = %p\n", stmtHndl);
	getColInfoParm->gi_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }
    
    if ( getColInfoParm->gi_columnNumber < 1  ||
         getColInfoParm->gi_columnNumber > stmtHndl->sh_colCount )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumnInfo: invalid column number = %d\n", 
	      getColInfoParm->gi_columnNumber );
	getColInfoParm->gi_status = IIAPI_ST_FAILURE;
    	return;
    }

    IIAPI_TRACE(IIAPI_TR_INFO)
	( "IIapi_getColumnInfo: stmtHndl = %p, column = %d\n", 
	  stmtHndl, getColInfoParm->gi_columnNumber );
    
    /* Convert to base 0 */
    colIndex = (II_UINT2)getColInfoParm->gi_columnNumber - 1;

    if ( stmtHndl->sh_colInfo  && 
	 stmtHndl->sh_colInfo[ colIndex ].ci_flags & IIAPI_CI_AVAIL )
    {
	if ( stmtHndl->sh_colInfo[ colIndex ].ci_flags & IIAPI_CI_LOB_LENGTH )
	{
	    getColInfoParm->gi_mask |= IIAPI_GI_LOB_LENGTH;
	    getColInfoParm->gi_lobLength = 
	    		stmtHndl->sh_colInfo[ colIndex ].ci_data.lob_length;
	}

    	getColInfoParm->gi_status = IIAPI_ST_SUCCESS;
    }

    return;
}
