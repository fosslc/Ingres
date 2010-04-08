/*
** Copyright (c) 2006 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apithndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apixa.c
**
** Description:
**	This file contains the XA function interface.
**
**      IIapi_xaStart		Start an association with an XA transaction.
**	IIapi_xaEnd		End association with an XA transaction.
**	IIapi_xaPrepare		Prepare an XA transaction.
**	IIapi_xaCommit		Commit an XA transaction.
**	IIapi_xaRollback	Rollback an XA transaction.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/



/*
** Name: IIapi_xaStart
**
** Description:
**	This function provide an interface for the frontend application
**      to start an association with an XA transaction.  Please refer to
**      the API user specification for function description.
**
**      startParm	input and output parameters for IIapi_xaStart()
**
** Return value:
**      none.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaStart( IIAPI_XASTARTPARM II_FAR *startParm )
{
    IIAPI_ENVHNDL	*envHndl;
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_TRANNAME	*tranName;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_xaStart: Start association with XA transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! startParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: null XA Start parameters\n" );
	return;
    }
    
    startParm->xs_genParm.gp_completed = FALSE;
    startParm->xs_genParm.gp_status = IIAPI_ST_SUCCESS;
    startParm->xs_genParm.gp_errorHandle = NULL;
    startParm->xs_tranHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)startParm->xs_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: API is not initialized\n" );
	IIapi_appCallback( &startParm->xs_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    /*
    ** Verify parameters.
    */
    if ( startParm->xs_tranID.ti_type != IIAPI_TI_XAXID )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_xaStart: invalid XA ID: %d\n", 
					startParm->xs_tranID.ti_type );
	IIapi_appCallback( &startParm->xs_genParm, NULL, IIAPI_ST_FAILURE );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: invalid connection handle\n" );
	IIapi_appCallback( &startParm->xs_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    /* Should not fail (in a perfect world!) */
    if ( ! (envHndl = IIapi_getEnvHndl( (IIAPI_HNDL *)connHndl )) )
	envHndl = IIapi_defaultEnvHndl();
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );

    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: : Insufficient protocol level: %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_RMFAIL ) )
	    IIapi_appCallback( &startParm->xs_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &startParm->xs_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    switch( startParm->xs_flags )
    {
    case 0 :			break;	/* OK */
    case IIAPI_XA_JOIN :	break;	/* OK */

    default :
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: invalid XA flags: 0x%x\n", startParm->xs_flags );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_INVAL ) )
	    IIapi_appCallback( &startParm->xs_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &startParm->xs_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    /*
    ** An XA transaction may only be started when no other is active.
    ** NOTE: this will change when SUSPEND/RESUME is supported.
    */
    if ( ! IIapi_isQueEmpty( (QUEUE *)&connHndl->ch_tranHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: multiple active transactions not allowed\n" );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_OUTSIDE ) )
	    IIapi_appCallback( &startParm->xs_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &startParm->xs_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }
	
    /*
    ** Create a transaction handle
    **
    ** Transaction ID is saved in transaction name handle
    ** so that it may be associated with the transaction
    ** handle.
    */
    if ( ! (tranName = IIapi_createTranName( &startParm->xs_tranID, envHndl )) )
    {
        IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: createTranName failed\n" );
	IIapi_appCallback( &startParm->xs_genParm, 
			   (IIAPI_HNDL *)connHndl, IIAPI_ST_OUT_OF_MEMORY );
	return;
    }

    if ( ! (tranHndl = IIapi_createTranHndl( tranName, connHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaStart: createTranHndl failed\n" );
	IIapi_appCallback( &startParm->xs_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	IIapi_deleteTranName( tranName );
	return;
    }

    startParm->xs_tranHandle = (II_PTR)tranHndl;

    IIapi_uiDispatch( IIAPI_EV_XASTART_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)startParm );
    return;
}



/*
** Name: IIapi_xaEnd
**
** Description:
**	This function provide an interface for the frontend application
**      to end an association with an XA transaction.  Please refer to
**      the API user specification for function description.
**
**      endParm		input and output parameters for IIapi_xaEnd()
**
** Return value:
**      none.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaEnd( IIAPI_XAENDPARM II_FAR *endParm )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_xaEnd: End association with XA transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! endParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: null XA End parameters\n" );
	return;
    }
    
    endParm->xe_genParm.gp_completed = FALSE;
    endParm->xe_genParm.gp_status = IIAPI_ST_SUCCESS;
    endParm->xe_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)endParm->xe_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: API is not initialized\n" );
	IIapi_appCallback( &endParm->xe_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( endParm->xe_tranID.ti_type != IIAPI_TI_XAXID )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_xaEnd: invalid XA ID: %d\n", 
					endParm->xe_tranID.ti_type );
	IIapi_appCallback( &endParm->xe_genParm, NULL, IIAPI_ST_FAILURE );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: invalid connection handle\n" );
	IIapi_appCallback( &endParm->xe_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: : Insufficient protocol level: %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_RMFAIL ) )
	    IIapi_appCallback( &endParm->xe_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &endParm->xe_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    switch( endParm->xe_flags )
    {
    case 0 :			break;	/* OK */
    case IIAPI_XA_FAIL :	break;	/* OK */

    default :
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: invalid XA flags: 0x%x\n", endParm->xe_flags );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_INVAL ) )
	    IIapi_appCallback( &endParm->xe_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &endParm->xe_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    /*
    ** In order to end an association with an XA transaction,
    ** there must be an active transaction handle associated
    ** with the XID.
    */
    if ( ! (tranHndl = IIapi_findTranHndl( connHndl, &endParm->xe_tranID )) )
    {
    	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_NOTA ) )
	    IIapi_appCallback( &endParm->xe_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &endParm->xe_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
    	return;
    }

    /*
    ** Check for conditions which disallows deletion of transaction
    */
    if ( ! IIapi_isQueEmpty( (QUEUE *)&tranHndl->th_stmtHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaEnd: can't XA End with active statements\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0004_ACTIVE_QUERIES, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &endParm->xe_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &endParm->xe_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_XAEND_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)endParm );
    return;
}



/*
** Name: IIapi_xaPrepare
**
** Description:
**	This function provide an interface for the frontend application
**      to prepare an XA transaction.  Please refer to the API user 
**	specification for function description.
**
**      prepParm	input and output parameters for IIapi_xaPrepare()
**
** Return value:
**      none.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaPrepare( IIAPI_XAPREPPARM II_FAR *prepParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_xaPrepare: Prepare XA transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! prepParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaPrepare: null XA Prepare parameters\n" );
	return;
    }
    
    prepParm->xp_genParm.gp_completed = FALSE;
    prepParm->xp_genParm.gp_status = IIAPI_ST_SUCCESS;
    prepParm->xp_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)prepParm->xp_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaPrepare: API is not initialized\n" );
	IIapi_appCallback( &prepParm->xp_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( prepParm->xp_tranID.ti_type != IIAPI_TI_XAXID )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_xaPrepare: invalid XA ID: %d\n", 
					prepParm->xp_tranID.ti_type );
	IIapi_appCallback( &prepParm->xp_genParm, NULL, IIAPI_ST_FAILURE );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaPrepare: invalid connection handle\n" );
	IIapi_appCallback( &prepParm->xp_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaPrepare: : Insufficient protocol level: %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_RMFAIL ) )
	    IIapi_appCallback( &prepParm->xp_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &prepParm->xp_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    switch( prepParm->xp_flags )
    {
    case 0 :			break;	/* OK */

    default :
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaPrepare: invalid XA flags: 0x%x\n", prepParm->xp_flags );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_INVAL ) )
	    IIapi_appCallback( &prepParm->xp_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &prepParm->xp_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    /*
    ** Can't prepare transaction when active on current session.
    */
    if ( IIapi_findTranHndl( connHndl, &prepParm->xp_tranID ) )
    {
    	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_PROTO ) )
	    IIapi_appCallback( &prepParm->xp_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &prepParm->xp_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
    	return;
    }

    IIapi_uiDispatch( IIAPI_EV_XAPREP_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)prepParm );
    return;
}



/*
** Name: IIapi_xaCommit
**
** Description:
**	This function provide an interface for the frontend application
**      to commit an XA transaction.  Please refer to the API user 
**	specification for function description.
**
**      commitParm	input and output parameters for IIapi_xaCommit()
**
** Return value:
**      none.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaCommit( IIAPI_XACOMMITPARM II_FAR *commitParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_xaCommit: Commit XA transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! commitParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaCommit: null XA Commit parameters\n" );
	return;
    }
    
    commitParm->xc_genParm.gp_completed = FALSE;
    commitParm->xc_genParm.gp_status = IIAPI_ST_SUCCESS;
    commitParm->xc_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)commitParm->xc_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaCommit: API is not initialized\n" );
	IIapi_appCallback( &commitParm->xc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( commitParm->xc_tranID.ti_type != IIAPI_TI_XAXID )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_xaCommit: invalid XA ID: %d\n", 
					commitParm->xc_tranID.ti_type );
	IIapi_appCallback( &commitParm->xc_genParm, NULL, IIAPI_ST_FAILURE );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaCommit: invalid connection handle\n" );
	IIapi_appCallback( &commitParm->xc_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaCommit: : Insufficient protocol level: %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_RMFAIL ) )
	    IIapi_appCallback( &commitParm->xc_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &commitParm->xc_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    switch( commitParm->xc_flags )
    {
    case 0 :			break;	/* OK */
    case IIAPI_XA_1PC :		break;	/* OK */

    default :
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_xaCommit: invalid XA flags: 0x%x\n", commitParm->xc_flags);

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_INVAL ) )
	    IIapi_appCallback( &commitParm->xc_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &commitParm->xc_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    /*
    ** Can't commit transaction when active on current session.
    */
    if ( IIapi_findTranHndl( connHndl, &commitParm->xc_tranID ) )
    {
    	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_PROTO ) )
	    IIapi_appCallback( &commitParm->xc_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &commitParm->xc_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
    	return;
    }

    IIapi_uiDispatch( IIAPI_EV_XACOMMIT_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)commitParm );
    return;
}



/*
** Name: IIapi_xaRollback
**
** Description:
**	This function provide an interface for the frontend application
**      to rollback an XA transaction.  Please refer to the API user 
**	specification for function description.
**
**      rollParm	input and output parameters for IIapi_xaRollback()
**
** Return value:
**      none.
**
** History:
**	 7-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_xaRollback( IIAPI_XAROLLPARM II_FAR *rollParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_xaRollback: Rollback XA transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! rollParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaRollback: null XA Rollback parameters\n" );
	return;
    }
    
    rollParm->xr_genParm.gp_completed = FALSE;
    rollParm->xr_genParm.gp_status = IIAPI_ST_SUCCESS;
    rollParm->xr_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)rollParm->xr_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaRollback: API is not initialized\n" );
	IIapi_appCallback( &rollParm->xr_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( rollParm->xr_tranID.ti_type != IIAPI_TI_XAXID )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_xaRollback: invalid XA ID: %d\n", 
					rollParm->xr_tranID.ti_type );
	IIapi_appCallback( &rollParm->xr_genParm, NULL, IIAPI_ST_FAILURE );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaRollback: invalid connection handle\n" );
	IIapi_appCallback( &rollParm->xr_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
    {
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_xaRollback: : Insufficient protocol level: %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_RMFAIL ) )
	    IIapi_appCallback( &rollParm->xr_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &rollParm->xr_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    switch( rollParm->xr_flags )
    {
    case 0 :			break;	/* OK */

    default :
    	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_xaRollback: invalid XA flags: 0x%x\n", rollParm->xr_flags);

	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_INVAL ) )
	    IIapi_appCallback( &rollParm->xr_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &rollParm->xr_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
        return;
    }

    /*
    ** Can't rollback transaction when active on current session.
    */
    if ( IIapi_findTranHndl( connHndl, &rollParm->xr_tranID ) )
    {
    	if ( ! IIapi_xaError( (IIAPI_HNDL *)connHndl, IIAPI_XAER_PROTO ) )
	    IIapi_appCallback( &rollParm->xr_genParm, NULL,
	    		       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &rollParm->xr_genParm, 
	    		       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
    	return;
    }

    IIapi_uiDispatch( IIAPI_EV_XAROLL_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)rollParm );
    return;
}


