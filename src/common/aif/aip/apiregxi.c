/*
** Copyright (c) 2004, 2006 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apithndl.h>
# include <apitname.h>
# include <apierhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiregxi.c
**
** Description:
**	This file contains the IIapi_registerXID() function interface.
**
**      IIapi_registerXID  API register transaction ID function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	17-Jan-95 (gordy)
**	    Conditionalize code for non-XA support.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 6-Mar-95 (gordy)
**	    Stardardize on transaction ID rather than transaction name.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Initialize the output parameter.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Removed unused MINICODE ifdef's.
**	 7-Feb-97 (gordy)
**	    Use default environment handle since there is no way
**	    to add handle to REGXIDPARM and maintain backward
**	    compatibility.
*/



/*
** Name: IIapi_registerXID
**
** Description:
**	This function provide an interface for the frontend application
**      to register a globally unique transaction ID with GCA-API for
**      future transactions.  Please refer to the API user
**      specification for function description.
**
**      regXIDParm  input and output parameters for IIapi_registerXID()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      01-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	17-Jan-95 (gordy)
**	    Conditionalize code for non-XA support.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 6-Mar-95 (gordy)
**	    Stardardize on transaction ID rather than transaction name.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	14-Jun-95 (gordy)
**	    Initialize the output parameter.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 7-Feb-97 (gordy)
**	    Use default environment handle since there is no way
**	    to add handle to REGXIDPARM and maintain backward
**	    compatibility.
**	10-Jun-06 (gordy)
**	    Changed parameter to IIapi_createTranName().
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_registerXID( IIAPI_REGXIDPARM II_FAR *regXIDParm )
{
    IIAPI_TRANNAME	*tranName;
    IIAPI_DBGDECLARE;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_registerXID: registering transaction ID with API\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! regXIDParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_registerXID: null registerXID parameters\n" );
	return;
    }
    
    regXIDParm->rg_status = IIAPI_ST_SUCCESS;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_registerXID: API is not initialized\n" );
	regXIDParm->rg_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }
    
    if ( 
	 regXIDParm->rg_tranID.ti_type != IIAPI_TI_IIXID 
#ifdef GCA_XA_SECURE
	 &&  regXIDParm->rg_tranID.ti_type != IIAPI_TI_XAXID 
#endif
       )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_registerXID: invalid tran ID\n" );
	regXIDParm->rg_status = IIAPI_ST_FAILURE;
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_registerXID: tran ID = %s\n", 
	  IIAPI_TRANID2STR( &regXIDParm->rg_tranID, IIAPI_DBGBUF ) ); 
    
    /*
    ** Create transaction name and check for uniqueness as well
    */
    if ( ! ( tranName = IIapi_createTranName( &regXIDParm->rg_tranID, 
					      IIapi_defaultEnvHndl() ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_registerXID: createTranName failed\n" );
	regXIDParm->rg_status = IIAPI_ST_OUT_OF_MEMORY;
	return;
    }
    
    regXIDParm->rg_tranIdHandle = (II_PTR)tranName;
   
    return;
}
