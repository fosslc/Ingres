/*
** Copyright (c) 2007 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apishndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiscroll.c
**
** Description:
**	This file contains the IIapi_scroll and IIapi_position() 
**	function interfaces.
**
**	IIapi_scroll		API cursor scroll function
**      IIapi_position		API cursor position function
**
** History:
**	15-Mar-07 (gordy)
**	    Created.
*/




/*
** Name: IIapi_scroll
**
** Description:
**	This function provide an interface for the frontend application
**      to scrolla cursor and retrieve tuple data from the DBMS server.  
**	Please refer to the API user specification for function description.
**
**      scrollParm	Input and output parameters for IIapi_scroll()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	15-Mar-07 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_scroll( IIAPI_SCROLLPARM II_FAR *scrollParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_CONNHNDL	*connHndl;
    bool		valid;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_scroll: scroll cursor\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! scrollParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_scroll: null scrollParms parameters\n" );
	return;
    }
    
    scrollParm->sl_genParm.gp_completed = FALSE;
    scrollParm->sl_genParm.gp_status = IIAPI_ST_SUCCESS;
    scrollParm->sl_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)scrollParm->sl_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_scroll: API is not initialized\n" );
	IIapi_appCallback( &scrollParm->sl_genParm, 
			   NULL, IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( 
    	 ! IIapi_isStmtHndl( stmtHndl )  ||  
	 IIAPI_STALE_HANDLE( stmtHndl )  ||
	 ! (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl))
       )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_scroll: invalid handle %p\n", stmtHndl );
	IIapi_appCallback( &scrollParm->sl_genParm, 
			   NULL, IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_scroll: handle = %p\n", stmtHndl );
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_scroll: orientation = %d, offset = %d\n", 
	    scrollParm->sl_orientation, scrollParm->sl_offset );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    /*
    ** Validate parameters.
    */
    switch( scrollParm->sl_orientation )
    {
    case IIAPI_SCROLL_NEXT :
	valid = TRUE;
	break;

    case IIAPI_SCROLL_BEFORE :
    case IIAPI_SCROLL_FIRST :
    case IIAPI_SCROLL_PRIOR :
    case IIAPI_SCROLL_CURRENT :
    case IIAPI_SCROLL_LAST :
    case IIAPI_SCROLL_AFTER :
    case IIAPI_SCROLL_ABSOLUTE :
    case IIAPI_SCROLL_RELATIVE :
	if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67 )
	    valid = TRUE;
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_scroll: scrolling not supported\n" );
	    valid = FALSE;
	}
    	break;

    default :
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_scroll: invalid orientation: %d\n",
	      (II_LONG)scrollParm->sl_orientation );
	valid = FALSE;
	break;
    }

    if ( ! valid )
    {
	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS22003_NUM_VAL_OUT_OF_RANGE, 
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &scrollParm->sl_genParm, 
			       NULL, IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &scrollParm->sl_genParm, 
	    		       (IIAPI_HNDL *)stmtHndl, IIAPI_ST_FAILURE );
        return;
    }

    IIapi_uiDispatch( IIAPI_EV_SCROLL_FUNC, 
    		      (IIAPI_HNDL *)stmtHndl, (II_PTR)scrollParm );
    return;
}



/*
** Name: IIapi_position
**
** Description:
**	This function provide an interface for the frontend application
**      to position a cursor and retrieve tuple data from the DBMS server.  
**	Please refer to the API user specification for function description.
**
**      posParm		Input and output parameters for IIapi_position()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	15-Mar-07 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_position( IIAPI_POSPARM II_FAR *posParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_CONNHNDL	*connHndl;
    bool		valid;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_position: position cursor\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! posParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_position: null posParms parameters\n" );
	return;
    }
    
    posParm->po_genParm.gp_completed = FALSE;
    posParm->po_genParm.gp_status = IIAPI_ST_SUCCESS;
    posParm->po_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)posParm->po_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_position: API is not initialized\n" );
	IIapi_appCallback( &posParm->po_genParm, 
			   NULL, IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( 
    	 ! IIapi_isStmtHndl( stmtHndl )  ||
	 IIAPI_STALE_HANDLE( stmtHndl )  ||
         ! (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl ))
       )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_position: invalid handle %p\n", stmtHndl );
	IIapi_appCallback( &posParm->po_genParm, 
			   NULL, IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_position: handle = %p\n", stmtHndl );
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_position: reference = %d, offset = %d, rows = %d\n", 
	    posParm->po_reference, posParm->po_offset, posParm->po_rowCount );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    /*
    ** Validate parameters.
    */
    switch( posParm->po_reference )
    {
    case IIAPI_POS_BEGIN :
    case IIAPI_POS_END :
	if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67 )
	    valid = TRUE;
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_scroll: scrolling not supported\n" );
	    valid = FALSE;
	}
    	break;

    case IIAPI_POS_CURRENT :
	if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67  ||
	     posParm->po_offset == 1 )
	    valid = TRUE;
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_scroll: scrolling not supported\n" );
	    valid = FALSE;
	}
	break;

    default :
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_position: invalid reference: %d\n",
	      (II_LONG)posParm->po_reference );
	valid = FALSE;
	break;
    }

    if ( posParm->po_rowCount < 
         ((connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67) ? 0 : 1) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_position: invalid row count: %d\n",
	      (II_LONG)posParm->po_rowCount );
	valid = FALSE;
    }

    if ( ! valid )
    {
	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS22003_NUM_VAL_OUT_OF_RANGE, 
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &posParm->po_genParm, 
			       NULL, IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &posParm->po_genParm, 
	    		       (IIAPI_HNDL *)stmtHndl, IIAPI_ST_FAILURE);
        return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_POSITION_FUNC, 
    		      (IIAPI_HNDL *) stmtHndl, (II_PTR)posParm );
    return;
}
