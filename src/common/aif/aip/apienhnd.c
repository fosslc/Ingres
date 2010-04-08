/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <tmtz.h>
# include <iicommon.h>
# include <adf.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apitname.h>
# include <apierhnd.h>
# include <apiadf.h>
# include <apitrace.h>

/*
** Name: apienhnd.c
**
** Description:
**	This file defines the environment handle management functions.
**
**      IIapi_createEnvHndl   Create environment handle.
**      IIapi_deleteEnvHndl   Delete environment handle.
**      IIapi_isEnvHndl       Validate environment handle.
**      IIapi_getEnvHndl      Get environment handle.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      21-Aug-98 (rajus01)
**          Added environment parameter to IIapi_cnvtGDVDescr().
**	    Added IIapi_setEnvironParm(), get_mathex_intval(),
**	    get_strtrunc_intval().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	03-Feb-99 (rajus01)
**	    Added IIAPI_EP_EVENT_FUNC. Renamed IIAPI_EP_CAN_PROMPT to
**	    IIAPI_EP_PROMPT_FUNC. Renamed IIAPI_UP_CAN_PROMPT to 
**	    IIAPI_UP_PROMPT_FUNC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Mar-04 (gordy)
**	    Added IIAPI_EP_INT8_WIDTH.
**	 8-Aug-07 (gordy)
**	    Added IIAPI_EP_DATE_ALIAS.
*/


static IIAPI_STATUS get_mathex_intval( II_CHAR *mathex, II_INT2 *val );
static IIAPI_STATUS get_strtrunc_intval( II_CHAR *trunc, II_INT2 *val );


/*
** Name: IIapi_createEnvHndl
**
** Description:
**	This function creates a environment handle.
**
** Input:
**	version		API version used by application.
**
** Ouput:
**	None.
**
** Returns:
**      envHndl		Environment handle or NULL if error.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_ENVHNDL *
IIapi_createEnvHndl( II_LONG version )
{
    IIAPI_ENVHNDL	*envHndl;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createEnvHndl: create an environment handle\n" );
    
    if ( ! ( envHndl = (IIAPI_ENVHNDL *)
	     MEreqmem( 0, sizeof(IIAPI_ENVHNDL), TRUE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createEnvHndl: can't alloc environment handle\n" );
	return( NULL );
    }
    
    /*
    ** Initialize environment handle parameters, provide default values.
    */
    if ( MUi_semaphore( &envHndl->en_semaphore ) != OK )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createEnvHndl: error initializing semaphore\n" );
	MEfree( (PTR)envHndl );
	return( NULL );
    }

    envHndl->en_header.hd_id.hi_hndlID = IIAPI_HI_ENV_HNDL;
    envHndl->en_header.hd_smi.smi_state = IIAPI_IDLE;
    envHndl->en_header.hd_delete = FALSE;

    QUinit( (QUEUE *)&envHndl->en_header.hd_id.hi_queue );
    QUinit( (QUEUE *)&envHndl->en_header.hd_errorList );
    envHndl->en_header.hd_errorQue = &envHndl->en_header.hd_errorList;

    envHndl->en_version = version;
    QUinit( &envHndl->en_connHndlList );
    QUinit( &envHndl->en_tranNameList );

    if ( ! IIapi_initADFSession( envHndl ) )
    {
	MUr_semaphore( &envHndl->en_semaphore );
	MEfree( (PTR)envHndl );
	return( NULL );
    }
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createEnvHndl: envHndl %p created\n", envHndl );
    
    return( envHndl );
}




/*
** Name: IIapi_deleteEnvHndl
**
** Description:
**	This function deletes an environment handle.
**
** Input:
**	envHndl		Environment handle to be deleted.
**
** Output:
**	None.
**
** Returns:
**	II_VOID
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
**	15-Aug-98 (rajus01)
**	    Added IIapi_clearEnvironParm().
*/

II_EXTERN II_VOID
IIapi_deleteEnvHndl( IIAPI_ENVHNDL *envHndl )
{
    QUEUE *q;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteEnvHndl: delete environment handle %p\n", envHndl );
    
    /*
    ** Do connection handles first to make sure
    ** all transaction handles are gone when we
    ** do the transaction name handles.
    */
    for( 
	 q = envHndl->en_connHndlList.q_next;
	 q != &envHndl->en_connHndlList;
	 q = envHndl->en_connHndlList.q_next 
       )
	IIapi_deleteConnHndl( (IIAPI_CONNHNDL *)q );

    for( 
	 q = envHndl->en_tranNameList.q_next;
	 q != &envHndl->en_tranNameList;
	 q = envHndl->en_tranNameList.q_next 
       )
	IIapi_deleteTranName( (IIAPI_TRANNAME *)q );

    IIapi_clearEnvironParm( envHndl );

    IIapi_cleanErrorHndl( &envHndl->en_header );
    IIapi_termADFSession( envHndl );

    envHndl->en_header.hd_id.hi_hndlID = ~IIAPI_HI_ENV_HNDL;
    MUr_semaphore( &envHndl->en_semaphore );
    MEfree( (II_PTR)envHndl );
    
    return;
}




/*
** Name: IIapi_isEnvHndl
**
** Description:
**     This function returns TRUE if the input is a valid environment
**     handle.
**
** Input:
**	envHndl		Environment handle to be validated
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if environment handle is valid, FALSE otherwise.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_isEnvHndl( IIAPI_ENVHNDL *envHndl )
{
    return( envHndl  && 
	    envHndl->en_header.hd_id.hi_hndlID == IIAPI_HI_ENV_HNDL );
}




/*
** Name: IIapi_getEnvHndl
**
** Description:
**	This function returns the environment handle in the generic handle
**      parameter.
**
** Input:
**	handle		Handle header of generic handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_ENVHNDL *	Environment handle, NULL if none.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_ENVHNDL *
IIapi_getEnvHndl( IIAPI_HNDL *handle )
{
    IIAPI_DBEVHNDL *dbevHndl;

    if ( handle )
    {
	switch( handle->hd_id.hi_hndlID )
	{
	    case IIAPI_HI_ENV_HNDL:
		break;

	    case IIAPI_HI_CONN_HNDL:
		handle = (IIAPI_HNDL *)
			 ((IIAPI_CONNHNDL *)handle)->ch_envHndl;
		break;

	    case IIAPI_HI_TRAN_HNDL:
		handle = (IIAPI_HNDL *)
			 ((IIAPI_TRANHNDL *)handle)->th_connHndl->ch_envHndl;
		break;

	    case IIAPI_HI_STMT_HNDL:
		handle = (IIAPI_HNDL *)((IIAPI_STMTHNDL *)handle)->
					sh_tranHndl->th_connHndl->ch_envHndl;
		break;

	    case IIAPI_HI_DBEV_HNDL:
		dbevHndl = (IIAPI_DBEVHNDL *)handle;

		if ( IIapi_isConnHndl((IIAPI_CONNHNDL *)dbevHndl->eh_parent) )
		    handle = (IIAPI_HNDL *)
			    ((IIAPI_CONNHNDL *)dbevHndl->eh_parent)->ch_envHndl;
		else
		{
		    IIAPI_DBEVCB *dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
		    handle = (IIAPI_HNDL *)dbevCB->ev_connHndl->ch_envHndl;
		}
		break;

	    default:
		handle = NULL;
		break;
	}
    }
    
    return( (IIAPI_ENVHNDL *)handle );
}

/*
** Name: IIapi_clearEnvironParm
**
** Description:
**	Free memory allocated for environment parameters.
**
** History:
**	12-Aug-98 (rajus01)
**	    Created.
**	 8-Aug-07 (gordy)
**	    Added date alias parameter.
*/

II_EXTERN II_VOID
IIapi_clearEnvironParm( IIAPI_ENVHNDL *envHndl )
{
    IIAPI_USRPARM	*usrParm = &envHndl->en_usrParm;

    if ( usrParm->up_decfloat )
    {
	MEfree( (PTR)usrParm->up_decfloat );
	usrParm->up_decfloat = NULL;
    }

    if ( usrParm->up_mathex )
    {
	MEfree( (PTR)usrParm->up_mathex );
	usrParm->up_mathex = NULL;
    }

    if ( usrParm->up_timezone )
    {
	MEfree( (PTR)usrParm->up_timezone );
	usrParm->up_timezone = NULL;
    }

    if ( usrParm->up_strtrunc )
    {
	MEfree( (PTR)usrParm->up_strtrunc );
	usrParm->up_strtrunc = NULL;
    }

    if ( usrParm->up_date_alias )
    {
	MEfree( (PTR)usrParm->up_date_alias );
	usrParm->up_date_alias = NULL;
    }

    return;
}

/*
** Name: IIapi_setEnvironParm
**
** Description:
**	Internal routine for IIapi_setEnvParam() to actually
**	save the environment parameter in the environment handle.
**
**	envHndl
**	parmID		Environment Parameter ID
**	parmValue	Pointer to parameter value, type specific to parmID
**	
** Returns:
**
**	IIAPI_STATUS	IIAPI_ST_SUCCESS
**			IIAPI_ST_OUT_OF_MEMORY
**			IIAPI_ST_FAILURE - bad parameter
**
** History:
**	15-Aug-98 (rajus01)
**	    Created.
**	14-Oct-98 (rajus01)
**	    Added support for GCA_PROMPT.
**	11-Dec-02 (gordy)
**	    Fixed tracing level.
**	15-Mar-04 (gordy)
**	    Added IIAPI_EP_INT8_WIDTH.
**	 8-Aug-07 (gordy)
**	    Added IIAPI_EP_DATE_ALIAS.
*/

II_EXTERN IIAPI_STATUS
IIapi_setEnvironParm( IIAPI_SETENVPRMPARM *envParm  ) 
{
    IIAPI_ENVHNDL    *envHndl = (IIAPI_ENVHNDL *)envParm->se_envHandle;
    ADF_CB     	     *adf_cb  = (ADF_CB *)envHndl->en_adf_cb;
    IIAPI_USRPARM    *usrParm = &envHndl->en_usrParm;
    STATUS	     status;
    i4		     i;

    IIAPI_TRACE( IIAPI_TR_INFO ) 
    ( "IIapi_setEnvParam: paramID = %d.\n", envParm->se_paramID );

    switch( envParm->se_paramID )
    {
	case IIAPI_EP_CHAR_WIDTH:
	    adf_cb->adf_outarg.ad_c0width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_CWIDTH ;
	    break;

	case IIAPI_EP_TXT_WIDTH:
	    adf_cb->adf_outarg.ad_t0width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_TWIDTH ;
	    break;

	case IIAPI_EP_INT1_WIDTH:
	    adf_cb->adf_outarg.ad_i1width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_I1WIDTH ;
	    break;

	case IIAPI_EP_INT2_WIDTH:
	    adf_cb->adf_outarg.ad_i2width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_I2WIDTH ;
	    break;

	case IIAPI_EP_INT4_WIDTH:
	    adf_cb->adf_outarg.ad_i4width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_I4WIDTH ;
	    break;

	case IIAPI_EP_INT8_WIDTH:
	    adf_cb->adf_outarg.ad_i8width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_I8WIDTH ;
	    break;

	case IIAPI_EP_FLOAT4_WIDTH:
	    adf_cb->adf_outarg.ad_f4width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_F4WIDTH ;
	    break;

	case IIAPI_EP_FLOAT8_WIDTH:
	    adf_cb->adf_outarg.ad_f8width = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_F8WIDTH ;
	    break;

	case IIAPI_EP_FLOAT4_PRECISION:
	    adf_cb->adf_outarg.ad_f4prec = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_F4PRECISION ;
	    break;

	case IIAPI_EP_FLOAT8_PRECISION:
	    adf_cb->adf_outarg.ad_f8prec = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_F8PRECISION ;
	    break;

	case IIAPI_EP_MONEY_PRECISION:
	    adf_cb->adf_mfmt.db_mny_prec = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_MPREC;
	    break;

	case IIAPI_EP_MONEY_SIGN:
	    if( STlength( (II_CHAR *)envParm->se_paramValue ) > DB_MAXMONY )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid money sign '%s'\n",
		   (II_CHAR *)envParm->se_paramValue );
		return( IIAPI_ST_FAILURE );
	    }
	    STcopy( (II_CHAR *)envParm->se_paramValue,
					adf_cb->adf_mfmt.db_mny_sym);
	    usrParm->up_mask1 |= IIAPI_UP_MSIGN;
	    break;

	case IIAPI_EP_MONEY_LORT:
	    adf_cb->adf_mfmt.db_mny_lort = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_MLORT;
	    break;

	case IIAPI_EP_FLOAT4_STYLE:

	    if( STlength( (char *)envParm->se_paramValue ) > 1 ) 
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid float4 style '%s'\n",
				(char *)envParm->se_paramValue );
		return( IIAPI_ST_FAILURE );
	    }
	    else
	    {
		char  	*f4style = (char *)envParm->se_paramValue;
	    	adf_cb->adf_outarg.ad_f4style = f4style[ 0 ];
	    }
	    usrParm->up_mask1 |= IIAPI_UP_F4STYLE;
	    break;

	case IIAPI_EP_FLOAT8_STYLE:

	    if( STlength( (char *)envParm->se_paramValue ) > 1 ) 
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid float8 style '%s'\n",
				(char *)envParm->se_paramValue );
		return( IIAPI_ST_FAILURE );
	    }
	    else
	    {
		char  	*f8style = (char *)envParm->se_paramValue;
	    	adf_cb->adf_outarg.ad_f8style = f8style[ 0 ];
	    }
	    usrParm->up_mask1 |= IIAPI_UP_F8STYLE;
	    break;

	case IIAPI_EP_NUMERIC_TREATMENT:
	    if ( usrParm->up_decfloat )
		MEfree( (PTR)usrParm->up_decfloat );
	    if ( ! ( usrParm->up_decfloat = 
				STalloc( (char *)envParm->se_paramValue ) ) )
		goto allocFail;
	    usrParm->up_mask1 |= IIAPI_UP_DECFLOAT;
	    break;

	case IIAPI_EP_MATH_EXCP:
	{
	    II_INT2	val = 0;
	    IIAPI_STATUS  status = IIAPI_ST_FAILURE;

	    if ( usrParm->up_mathex )
		MEfree( (PTR)usrParm->up_mathex );
	    if ( ! ( usrParm->up_mathex = 
				STalloc( (char *)envParm->se_paramValue ) ) )
		goto allocFail;

	    status = get_mathex_intval( usrParm->up_mathex, &val ); 

	    if( status == IIAPI_ST_SUCCESS )
	        adf_cb->adf_exmathopt = val;
	    else
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid math exeception option '%d'\n",
		   val );
		return( IIAPI_ST_FAILURE );
	    }

	    usrParm->up_mask1 |= IIAPI_UP_MATHEX;
	}
   	break;

	case IIAPI_EP_STRING_TRUNC:
	{
	    II_INT2		val = 0;
	    IIAPI_STATUS	status = IIAPI_ST_FAILURE;

	    if ( usrParm->up_strtrunc)
		MEfree( (PTR)usrParm->up_strtrunc );
	    if ( ! ( usrParm->up_strtrunc = 
				STalloc( (char *)envParm->se_paramValue ) ) )
		goto allocFail;

	    status = get_strtrunc_intval( usrParm->up_strtrunc, &val );

	    if( status == IIAPI_ST_SUCCESS )
	        adf_cb->adf_strtrunc_opt = val;
	    else
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid string truncation option '%d'\n", val );
		return( IIAPI_ST_FAILURE );
	    }

	    usrParm->up_mask1 |= IIAPI_UP_STRTRUNC;
	}
	    break;

	case IIAPI_EP_DATE_FORMAT:
	{
	    II_LONG *val = (II_LONG *) envParm->se_paramValue;

	    if( *val  < IIAPI_EPV_DFRMT_US ||
		*val  > IIAPI_EPV_DFRMT_ISO4 )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid date format '%d'\n", *val );
		return( IIAPI_ST_FAILURE );
	    }
	    else
		adf_cb->adf_dfmt = *val;

	    usrParm->up_mask1 |= IIAPI_UP_DATE_FORMAT;
	}
	break;

	case IIAPI_EP_TIMEZONE:
	{
	    STATUS	status;

	    /* Search for correct timezone name */

	    status = TMtz_lookup( (char *)envParm->se_paramValue,
						&adf_cb->adf_tzcb );
	    if( status == TM_TZLKUP_FAIL)
	    {
		/* Load new table */
		MUp_semaphore( &IIapi_static->api_semaphore );
		status = TMtz_load( (char *)envParm->se_paramValue,
						&adf_cb->adf_tzcb );
		MUv_semaphore( &IIapi_static->api_semaphore );
	    }
	    if( status != OK )  
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid timezone '%s'\n",
					(char *)envParm->se_paramValue);
		return( IIAPI_ST_FAILURE );
	    }

	    if ( usrParm->up_timezone )
		MEfree( (PTR)usrParm->up_timezone );

	    if ( ! ( usrParm->up_timezone = 
				STalloc( (char *)envParm->se_paramValue ) ) )
		goto allocFail;

	    usrParm->up_mask1 |= IIAPI_UP_TIMEZONE;
	}
	break;

	case IIAPI_EP_DECIMAL_CHAR: 
	{
	    char	*val = (char *)envParm->se_paramValue;

	    if( val && *val )
		if ( val[1] == EOS  && ( val[0] == '.'  || val[0] == ',') )
		    adf_cb->adf_decimal.db_decimal = val[0];
		else
		{
		    IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_setEnvironParm: invalid decimal char '%s'\n", val);
		    return( IIAPI_ST_FAILURE );
	  	}

	    usrParm->up_mask1 |= IIAPI_UP_DECIMAL;
	}
	break;

	case IIAPI_EP_NATIVE_LANG:
	    if ( ERlangcode( (II_CHAR *)envParm->se_paramValue, &i ) != OK )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_setEnvironParm: unknown language\n" );
		return( IIAPI_ST_FAILURE );
	    }
	    else
	        adf_cb->adf_slang = i;

	    usrParm->up_mask1 |= IIAPI_UP_NLANGUAGE;

	    break;

	case IIAPI_EP_NATIVE_LANG_CODE:
	    adf_cb->adf_slang = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_NLANGUAGE;
	    break;

	case IIAPI_EP_CENTURY_BOUNDARY:
	{
	    II_LONG	year_cutoff = *(II_LONG *)envParm->se_paramValue;

	    if( year_cutoff <= TM_DEF_YEAR_CUTOFF ||
		year_cutoff > TM_MAX_YEAR_CUTOFF )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setEnvironParm: invalid century boundary '%d'\n",
						year_cutoff );
		return( IIAPI_ST_FAILURE );
	    }
	    else
	        adf_cb->adf_year_cutoff = year_cutoff;

	    usrParm->up_mask1 |= IIAPI_UP_CENTURY_BOUNDARY;
	}
	break;

	case IIAPI_EP_MAX_SEGMENT_LEN:
	    usrParm->up_max_seglen = *(II_LONG *)envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_MAX_SEGMENT_LEN;
	    break;

	case IIAPI_EP_TRACE_FUNC:
	    usrParm->up_trace_func = (void (*)())envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_TRACE_FUNC;
	    break;

	case IIAPI_EP_PROMPT_FUNC:
	    usrParm->up_prompt_func = (void (*)())envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_PROMPT_FUNC;
	    break;

	case IIAPI_EP_EVENT_FUNC:
	    usrParm->up_event_func = (void (*)())envParm->se_paramValue;
	    usrParm->up_mask1 |= IIAPI_UP_EVENT_FUNC;
	    break;

	case IIAPI_EP_DATE_ALIAS: 
	    if ( usrParm->up_date_alias )
		MEfree( (PTR)usrParm->up_date_alias );

	    if ( ! (usrParm->up_date_alias = 
				STalloc( (char *)envParm->se_paramValue )) )
		goto allocFail;

	    usrParm->up_mask1 |= IIAPI_UP_DATE_ALIAS;
	    break;

	default:
	    /*
	    ** This should not happen!!!!!!
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_setEnvironParm: invalid param ID %d\n",envParm->se_paramID);
	    return( IIAPI_ST_FAILURE );
    }

    return( IIAPI_ST_SUCCESS );

    allocFail :

	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_setEnvironParm: memory allocation failed\n" );

	return( IIAPI_ST_OUT_OF_MEMORY );

}

/*
** Name: get_mathex_intval
**
** Description:
**      Translate string math exception to numeric math exception.
**
** Input:
**      mathex          String  math exception
**
** Output:
**      val             Numeric math exception
**
** Returns:
**      IIAPI_ST_SUCCESS  ( or )
**	IIAPI_ST_FAILURE (invalid string value).
**
** History:
**      19-Aug-98 (rajus01)
**          Created.
*/

static IIAPI_STATUS
get_mathex_intval( II_CHAR *mathex, II_INT2 *val )
{
    if ( ! mathex  ||  ! *mathex )
	return( IIAPI_ST_FAILURE );

    else  if ( ! STbcompare( mathex, 0, IIAPI_EPV_RET_IGNORE, 3, TRUE ))
	*val = ADX_IGN_MATHEX;

    else  if ( ! STbcompare( mathex, 0,IIAPI_EPV_RET_WARN, 3, TRUE ) )
	*val = ADX_WRN_MATHEX;

    else  if ( ! STbcompare( mathex, 0, IIAPI_EPV_RET_FATAL, 3, TRUE ) )
	*val = ADX_ERR_MATHEX;

    else
	return( IIAPI_ST_FAILURE );

    return( IIAPI_ST_SUCCESS );
}

/*
** Name: get_strtrunc_intval
**
** Description:
**      Translate string string trunc option to numeric 
**	string trunc option.
**
** Input:
**      trunc          String string truncation option 
**
** Output:
**      val             Numeric string truncation option
**
** Returns:
**      IIAPI_ST_SUCCESS  ( or ) 
**	IIAPI_ST_FAILURE (invalid string value).
**
** History:
**      19-Aug-98 (rajus01)
**          Created.
*/
static IIAPI_STATUS
get_strtrunc_intval( II_CHAR *trunc, II_INT2 *val )
{
    if ( ! trunc  ||  ! *trunc )
	return( IIAPI_ST_FAILURE );

    else  if ( ! STbcompare( trunc, 0, IIAPI_EPV_RET_IGNORE, 3, TRUE ))
	*val = ADF_IGN_STRTRUNC;

    else  if ( ! STbcompare( trunc, 0,IIAPI_EPV_RET_WARN, 3, TRUE ) )
	*val = ADF_WRN_STRTRUNC;

    else  if ( ! STbcompare( trunc, 0, IIAPI_EPV_RET_FATAL, 3, TRUE ) )
	*val = ADF_ERR_STRTRUNC;

    else
	return( IIAPI_ST_FAILURE );

    return( IIAPI_ST_SUCCESS );
}
