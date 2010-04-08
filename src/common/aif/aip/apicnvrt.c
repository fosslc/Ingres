/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cv.h>

# include <iicommon.h>
# include <adf.h>
# include <adudate.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apiadf.h>
# include <apidatav.h>
# include <apimisc.h>
# include <apitrace.h>
# include <aduucol.h>

/*
** Name: apicnvrt.c
**
** Description:
**	This file contains IIapi_convertData(), IIapi_formatData() functions.
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	22-Feb-95 (gordy)
**	    Changed interface to match API standard.
**	 8-Mar-95 (gordy)
**	    Silence compiler warnings.
**	10-Mar-95 (gordy)
**	    Dropped usage of EX around ADF.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	20-Jun-95 (gordy)
**	    Renamed IIapi_cnvtDataValue2GDV().
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Added tracing.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	 7-Feb-97 (gordy)
**	    This function always uses the default environment handle.
**	07-Feb-97 (somsa01)
**	    Allowed CVpka() and CVapk() to use the value from II_DECIMAL.
**	17-Mar-97 (gordy)
**	    Cleaned up ADF parameters.
**	18-Mar-97 (gordy)
**	    Input values for dv_length should be ignored for output buffers.
**	15-Aug-98 (rajus01)
**	    Added IIapi_formatData() function. Added a static function
**	    IIapi_resultData() to move the common code between
**	    IIapi_convertData() and IIapi_formatData().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-mar-2004 (gupsh01)
**	    Added support for converting to and from unicode. 
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          In IIapi_resultData(), make size of decimal buffer contingent on 
**          DB_MAX_DECPREC.
**     13-Apr-2009 (Ralph Loen) Bug 121914 
**          Move code for Unicode/multibyte conversions from 
**          IIapi_convertData() to IIapi_resultData().  In this way,
**          both IIapi_formatData() and IIapi_conververtData() can support
**          Unicode/mulitbyte conversions. 
**	25-Mar-10 (gordy)
**	    Use local buffers for most conversions, dynamically allocate
**	    buffers if required by object size.  Cleanup code organization.
*/

/* Forward references */

static IIAPI_STATUS IIapi_resultData( IIAPI_ENVHNDL *envHndl,
				      IIAPI_DATAVALUE *srcVal,
				      IIAPI_DATAVALUE *dstVal,
				      IIAPI_DESCRIPTOR *srcDesc,
				      IIAPI_DESCRIPTOR *dstDesc );

/*
** Name: IIapi_convertData
**
** Description: 
**	Perform conversion between INGRES datatypes.
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	22-Feb-95 (gordy)
**	    Changed interface to match API standard.
**	10-Mar-95 (gordy)
**	    Dropped usage of EX around ADF.
**	20-Jun-95 (gordy)
**	    Renamed IIapi_cnvtDataValue2GDV().
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Added tracing.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	 7-Feb-97 (gordy)
**	    This function always uses the default environment handle.
**	07-Feb-97 (somsa01)
**	    Allowed CVpka() and CVapk() to use the value from II_DECIMAL.
**	17-Mar-97 (gordy)
**	    Cleaned up ADF parameters.
**	18-Mar-97 (gordy)
**	    Input values for dv_length should be ignored for output buffers.
**	08-Sep-98 (rajus01)
**	    Added IIapi_resultData() internal function. 
**	02-mar-2004 (gupsh01)
**	    Added support for converting the unicode collation table.
**      13-Apr-2009 (Ralph Loen) Bug 121914 
**          Move code for Unicode/multibyte conversions from 
**          IIapi_convertData() to IIapi_resultData().  In this way,
**          both IIapi_formatData() and IIapi_conververtData() can support
**          Unicode/mulitbyte conversions. 
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_convertData( IIAPI_CONVERTPARM II_FAR *convertParm )
{
    IIAPI_ENVHNDL	*envHandl = IIapi_defaultEnvHndl();

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_convertData: converting data\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! convertParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_convertData: null convertData parameters\n" );
	return;
    }
    
    convertParm->cv_status = IIAPI_ST_SUCCESS;

    /*
    ** Make sure API is initialized.
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_convertData: API is not initialized\n" );
	convertParm->cv_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }

    convertParm->cv_status = IIapi_resultData( envHandl,
		      			       &convertParm->cv_srcValue,
		      			       &convertParm->cv_dstValue,
		      			       &convertParm->cv_srcDesc,
		      			       &convertParm->cv_dstDesc
					     );
    return;
}


/*
** Name: IIapi_formatData
**
** Description: 
**	Perform conversion between INGRES datatypes.
**
**	It uses environment handle provided by the application instead of 
**	default environment hanlde.
**
** History:
**	 15-Aug-98 (rajus01)
**	    Created.
*/


II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_formatData( IIAPI_FORMATPARM II_FAR *formatParm )
{
    IIAPI_ENVHNDL       *envHandl;

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_formatData: formatting data\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! formatParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_formatData: null formatData parameters\n" );
	return;
    }
    
    envHandl 		  = (IIAPI_ENVHNDL *)formatParm->fd_envHandle;
    formatParm->fd_status = IIAPI_ST_SUCCESS;

    /*
    ** Make sure API is initialized.
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_formatData: API is not initialized\n" );
	formatParm->fd_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }

    if ( ! IIapi_isEnvHndl( envHandl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_formatData: invalid environment handle\n" );
	formatParm->fd_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }

    formatParm->fd_status = IIapi_resultData( envHandl,
					      &formatParm->fd_srcValue,
		     			      &formatParm->fd_dstValue,
		     			      &formatParm->fd_srcDesc,
		      			      &formatParm->fd_dstDesc );
    return;
}

/*
** Name: IIapi_resultData
**
** Description: 
**	Internal routine for IIapi_convertData() and IIapi_formatData().
**	It uses environment handle provided by the application for
**	IIapi_formatData() as opposed to default environment hanlde.
**
** History:
**	 08-Sep-98 (rajus01)
**	    Created.
**       31-Jul-2008 (Ralph Loen) Bug 120718
**          Make size of decimal buffer contingent on DB_MAX_DECPREC 
**          instead of hard-coded value.
**       13-Apr-2009 (Ralph Loen) Bug 121914 
**          Move code for Unicode/multibyte conversions from 
**          IIapi_convertData() to IIapi_resultData().  In this way,
**          both IIapi_formatData() and IIapi_conververtData() can support
**          Unicode/mulitbyte conversions. 
**	11-May-2009 (kschendel) b122041
**	    Remove cast on aduucolinit call, proto now matches MEreqmem.
**	25-Mar-10 (gordy)
**	    Use local buffers for most conversions, dynamically allocate
**	    buffers if required by object size.  Cleanup code organization.
*/

static IIAPI_STATUS
IIapi_resultData( IIAPI_ENVHNDL *envHndl,
		  IIAPI_DATAVALUE *srcVal,
		  IIAPI_DATAVALUE *dstVal,
		  IIAPI_DESCRIPTOR *srcDesc,
		  IIAPI_DESCRIPTOR *dstDesc )
		  
{
    ADF_CB	*adf_cb  = (ADF_CB *)envHndl->en_adf_cb;
    STATUS	status;

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_resultData: Entry.\n" );

    if ( ! srcVal->dv_value  ||  ! dstVal->dv_value  || srcVal->dv_null )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_resultData: invalid IIAPI_DATAVALUE information\n" );
	return( IIAPI_ST_FAILURE );
    }

    if ( srcDesc->ds_dataType == IIAPI_CHA_TYPE  &&
	 dstDesc->ds_dataType == IIAPI_DEC_TYPE )
    {
	/* Include sign, leading 0, decimal & null terminator */
	char		buffer[ DB_MAX_DECPREC + 4 ];
	II_UINT2	len;

	if ( dstDesc->ds_length < 
	     DB_PREC_TO_LEN_MACRO( dstDesc->ds_precision ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
	        ( "IIapi_resultData: invalid decimal data value length\n" );
	    return( IIAPI_ST_FAILURE );
	}

	len = min( srcDesc->ds_length, srcVal->dv_length );
	len = min( len, sizeof( buffer ) - 1 );

	MEcopy( srcVal->dv_value, len, buffer );
	buffer[ len ] = EOS;

	status = CVapk( buffer, adf_cb->adf_decimal.db_decimal,
			dstDesc->ds_precision, dstDesc->ds_scale, 
			dstVal->dv_value );

	if ( status != OK )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_resultData: conversion error 0x%x\n", status );
	    return( IIAPI_ST_FAILURE );
	}

	dstVal->dv_null = FALSE;
	dstVal->dv_length = DB_PREC_TO_LEN_MACRO( dstDesc->ds_precision );
    }
    else  if ( srcDesc->ds_dataType == IIAPI_DEC_TYPE  &&
	       dstDesc->ds_dataType == IIAPI_CHA_TYPE )
    {
	/* Include sign, leading 0, decimal & null terminator */
	char		buffer[ DB_MAX_DECPREC + 4 ];
	II_UINT2	len;
    	i4		olen;

	len = min( dstDesc->ds_length, sizeof(buffer) - 1 );

	status = CVpka( srcVal->dv_value, 
			srcDesc->ds_precision, srcDesc->ds_scale, 
			adf_cb->adf_decimal.db_decimal, (i4)len, 
			srcDesc->ds_scale, CV_PKLEFTJUST, buffer, &olen );

	if ( status != OK )  
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
	        ( "IIapi_resultData: conversion error 0x%x\n", status );
	    return( IIAPI_ST_FAILURE );; 
	}

	/*
	** Remove EOS if included in length,
	** and copy result to destination.
	*/
	if ( olen > 0  &&  ! buffer[ olen - 1 ] )  olen--;
	MEcopy( buffer, len, dstVal->dv_value );

	/*
	** Blank pad if necessary.
	*/
	if ( olen < dstDesc->ds_length )
	{
	    MEfill( dstDesc->ds_length - (u_i2)olen, ' ', 
		    (char *)dstVal->dv_value + olen );

	    olen = dstDesc->ds_length;
	}

	dstVal->dv_null = FALSE;
	dstVal->dv_length = (u_i2)olen;
    }
    else  
    {
	DB_DATA_VALUE	src, dst;
	GCA_DATA_VALUE	srcGDV, dstGDV;
	char		srcBuff[ 256 ], dstBuff[ 256 ];
	char		*sptr, *dptr;
	II_BOOL		input_uni = FALSE;
	II_BOOL		output_uni = FALSE;

        /* check the unicode convert parameters */
        if ( ( srcDesc->ds_dataType == IIAPI_NCHA_TYPE ) ||
             ( srcDesc->ds_dataType == IIAPI_NVCH_TYPE ) )
            input_uni = TRUE;
    
        if ( ( dstDesc->ds_dataType == IIAPI_NCHA_TYPE ) ||
             ( dstDesc->ds_dataType == IIAPI_NVCH_TYPE ) )
            output_uni = TRUE;
    
        if ( input_uni != output_uni )
        {
            if ( IIapi_static->api_unicol_init == FALSE)
            {
		char		col[] = "udefault";
		CL_ERR_DESC	sys_err;

                if ( (status = aduucolinit( (char *)col, MEreqmem, 
				(ADUUCETAB **)&IIapi_static->api_ucode_ctbl,
				&IIapi_static->api_ucode_cvtbl,  
				&sys_err )) )
                {
		    IIAPI_TRACE( IIAPI_TR_FATAL )
			("IIapi_resultData: aduucolinit error 0x%x\n", status);
                    return( IIAPI_ST_FAILURE );
                }

                IIapi_static->api_unicol_init = TRUE;
            }

            adf_cb->adf_ucollation = IIapi_static->api_ucode_ctbl;
        }

	IIapi_cnvtDescr2GDV( srcDesc, &srcGDV );
	IIapi_cnvtDescr2GDV( dstDesc, &dstGDV );

	if ( srcGDV.gca_l_value <= sizeof( srcBuff ) )
	    sptr = srcBuff;
	else  if ( ! (sptr = (char *)MEreqmem( 0, srcGDV.gca_l_value, 
							FALSE, NULL )) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
	        ( "IIapi_resultData: can't allocate src buffer %d bytes\n", 
		  srcGDV.gca_l_value );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}

	if ( dstGDV.gca_l_value <= sizeof( dstBuff ) )
	    dptr = dstBuff;
	else  if ( ! (dptr = (char *)MEreqmem( 0, dstGDV.gca_l_value, 
							FALSE, NULL )) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
	        ( "IIapi_resultData: can't allocate dst buffer %d bytes\n", 
		  dstGDV.gca_l_value );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}

	IIapi_cnvtDataValue2GDV( srcDesc, srcVal, sptr );
	IIapi_cnvtDataValue2GDV( dstDesc, dstVal, dptr );

	src.db_datatype = (DB_DT_ID)srcGDV.gca_type;
	src.db_length   = srcGDV.gca_l_value;
	src.db_prec     = (i2)srcGDV.gca_precision;
	src.db_data     = sptr;

	dst.db_datatype = (DB_DT_ID)dstGDV.gca_type;
	dst.db_length   = dstGDV.gca_l_value;
	dst.db_prec     = (i2)dstGDV.gca_precision;
	dst.db_data     = dptr;

	status = adc_cvinto( envHndl->en_adf_cb, &src, &dst );
	if ( status == OK )  IIapi_cnvtGDV2DataValue( dstDesc, dptr, dstVal );

	if ( sptr != srcBuff )  MEfree( sptr );
	if ( dptr != dstBuff )  MEfree( dptr );

	if ( status != OK )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
	        ( "IIapi_resultData: conversion error 0x%x\n", status );
	    return( IIAPI_ST_FAILURE );
	}
    }

    return ( IIAPI_ST_SUCCESS );
}
