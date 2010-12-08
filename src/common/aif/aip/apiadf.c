/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cm.h>
# include <cv.h>
# include <er.h>
# include <nm.h>
# include <st.h>
# include <tm.h>
# include <tmtz.h>

# include <iicommon.h>
# include <adf.h>
# include <adudate.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apiadf.h>
# include <apitrace.h>
# include <pm.h>
/*
** Name: apiadf.c
**
** Description:
**	This file contains routines which manage the ADF interface for API.
**
**	IIapi_initADF
**	IIapi_termADF
**	IIapi_initADFSession
**	IIapi_termADFSession
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	 8-Mar-95 (gordy)
**	    Made global variables local and added access functions.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Added global data structure.
**	    Separated ADF session initialization into environment.
**	14-mar-96 (lawst01)
**	   Windows 3.1 port changes - added include <tr.h>
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**	07-feb-97 (somsa01)
**	    Added setting of II_DATE_FORMAT, II_MONEY_FORMAT, II_MONEY_PREC,
**	    and II_DECIMAL to IIapi_initADF() and IIapi_initADFSession().
**	    (Bug #80254)
**	17-Mar-97 (gordy)
**	    Cleanup defaults for ADF parameters.
**	15-Sep-98 (rajus01)
**	    Added setting of II_CENTURY_BOUNDARY to IIapi_initADF().
**	    Set adf_slang in IIapi_initADFSession().
**	 7-Dec-99 (gordy)
**	    Removed redundant check of II_DATE_CENTURY_BOUNDARY.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	09-sep-2003 (abbjo03)
**	    Add missing parameters in call to adg_startup().
**	13-Apr-2004 (hanje04)
**	    SIR 111507
**	    Define adf_cb->df_outarg.ad_f8width in IIapi_initADFSession()
**	    which was added for BIGINT (i8) support.
**	    (See inkdo01 change 467781 for more info)
**	 8-Aug-07 (gordy)
**	    Support date type alias.
**       29-Nov-2007 (rapma01)
**          added include of pm.h to provide prototype for PMhost()
**       15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

/*
** The following routine is used by ADF to obtain
** the values used for converting 'today' and 'now'
** date constants to INGRES dates.
*/
static DB_STATUS adf_time(
		ADF_DBMSINFO	*dbi, 
		DB_DATA_VALUE	*dv1,
		DB_DATA_VALUE	*dvr,
		DB_ERROR	*err);

/*
** The following structure is used to initialize ADF
** so that 'today' and 'now' date constants will be
** processed correctly.  The table declares values
** maintained by the DBMS and accessible to ADF by
** function calls.  ADF calls the routine associated
** with '_bintim' when converting 'today' and 'now'
** to internal INGRES dates.
*/
static ADF_TAB_DBMSINFO adf_tab_dbms = 
{
    /* header of ADF_TAB_DBMSINFO */
    NULL, NULL, 
    sizeof(ADF_TAB_DBMSINFO), 
    ADTDBI_TYPE, 
    0, NULL, NULL, 
    ADTDBI_ASCII_ID,

    /* ADF_DBMSINFO single entry */
    1, 
    { { adf_time, "_bintim", 0, 0, { ADI_FIXED, 4, 0 }, DB_INT_TYPE, 0 } }
};



/*
** Name: adf_time
**
** Description:
**	Returns current time to ADF.
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
*/

static DB_STATUS 
adf_time(
ADF_DBMSINFO	*dbi, 
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dvr,
DB_ERROR	*err)
{
    *(i4 *)dvr->db_data = TMsecs();
    return( OK );
}



/*
** Name: IIapi_initADF
**
** Description:
**	Perform global ADF initialization.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE otherwise.
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	19-Jan-96 (gordy)
**	    Added global data structure.  Extracted session level
**	    initialization into IIapi_initADFSession().
**	07-Feb-97 (somsa01)
**	    Added setting of II_DATE_FORMAT, II_MONEY_FORMAT, II_MONEY_PREC,
**	    and II_DECIMAL.  (Bug #80254)
**	17-Mar-97 (gordy)
**	    Cleanup defaults for ADF parameters.
**	24-Jun-1999 (shero03)
**	    Added ISO4.
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for Sir 92541.
**	 7-Dec-99 (gordy)
**	    Removed redundant check of II_DATE_CENTURY_BOUNDARY.
**	18-Oct-2007 (kibro01) b119318
**	    Use a common function in adu to determine valid date formats
**	 8-Aug-07 (gordy)
**	    Lookup date type alias.
**      27-Mar-2009 (hanal04) Bug 121857
**          Passing too many parameters to adg_startup(). Found whilst
**          building with xDEBUG defined in adf.h
*/

II_EXTERN II_BOOL
IIapi_initADF( II_VOID )
{
    STATUS	r_stat;
    i4		adf_size;
    char	*ptr, sym[100];

    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "IIapi_initADF: Initializing ADF\n" );

    adf_size = adg_srv_size();
    IIapi_static->api_adf_cb = MEreqmem( 0, adf_size, TRUE, &r_stat );
    
    if ( ! IIapi_static->api_adf_cb )
    {
        IIAPI_TRACE( IIAPI_TR_FATAL )
            ( "IIapi_initADF: error allocating ADF server control block\n" );
        return( FALSE );
    }
    
    r_stat = adg_startup( IIapi_static->api_adf_cb, adf_size, &adf_tab_dbms, 0);

    if ( r_stat != OK )  
    {
        IIAPI_TRACE( IIAPI_TR_ERROR )
            ( "IIapi_init_ADF: error initializing ADF 0x%x\n", r_stat );
        return( FALSE );
    }

    if ( (r_stat = TMtz_init( &IIapi_static->api_tz_cb )) == OK )
    {
	/*
	** The following checks II_DATE_CENTURY_BOUNDARY
	*/
	r_stat = TMtz_year_cutoff( &IIapi_static->api_year_cutoff );
    }

    if ( r_stat != OK )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_initADF: error initializing timezones 0x%x\n", r_stat );
	adg_shutdown();
	return( FALSE );
    }

    NMgtAt( "II_TIMEZONE_NAME", &ptr );
    if ( ptr  &&  *ptr )  IIapi_static->api_timezone = STalloc( ptr );

    IIapi_static->api_dfmt = DB_US_DFMT;
    NMgtAt( "II_DATE_FORMAT", &ptr );

    if ( ptr  &&  *ptr  )
    {
	i4 date_fmt;
	CVlower(ptr);

	/* Use common function to determine date formats (kibro01) b119318 */
	date_fmt = adu_date_format(ptr);
	if (date_fmt == -1)
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
	        ( "IIapi_initADF: invalid date format '%s', using default\n", 
		  ptr );
	} else
	{
	    IIapi_static->api_dfmt = date_fmt;
	}
    }

    IIapi_static->api_date_alias = AD_DATE_TYPE_ALIAS_INGRES;
    STprintf( sym, ERx("ii.%s.config.date_alias"), PMhost() );
    r_stat = PMget( sym, &ptr );

    if ( r_stat == OK  &&  ptr  &&  *ptr )
	if ( ! STbcompare( ptr, 0, IIAPI_EPV_ANSIDATE, 0, 1 ) )
	    IIapi_static->api_date_alias = AD_DATE_TYPE_ALIAS_ANSI;
	else  if ( ! STbcompare( ptr, 0, IIAPI_EPV_INGDATE, 0, 1 ) )
	    IIapi_static->api_date_alias = AD_DATE_TYPE_ALIAS_INGRES;
	else
	    IIapi_static->api_date_alias = AD_DATE_TYPE_ALIAS_NONE;

    IIapi_static->api_mfmt.db_mny_lort = DB_LEAD_MONY;
    STcopy( "$", IIapi_static->api_mfmt.db_mny_sym );
    NMgtAt( "II_MONEY_FORMAT", &ptr );

    if ( ptr  &&  *ptr )
    {
	CMtolower( ptr, ptr );

	if ((STlength(ptr) == 4) && (ptr[0] == 'n') &&
	    (ptr[1] == 'o') && (ptr[2] == 'n') && (ptr[3] == 'e'))
	{
	    IIapi_static->api_mfmt.db_mny_lort = DB_NONE_MONY;
	    IIapi_static->api_mfmt.db_mny_sym[0] = EOS;
	}
	else if ( (ptr[0] != 'l' && ptr[0] != 't') || ptr[1] != ':' ||
	     STlength( ptr ) > DB_MAXMONY + 2 )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_initADF: invalid money format '%s', using default\n", 
		  ptr );
	}
	else  
	{
	    STcopy( ptr + 2, IIapi_static->api_mfmt.db_mny_sym );
	    if ( ptr[0] == 't' )
	        IIapi_static->api_mfmt.db_mny_lort = DB_TRAIL_MONY;
	}
    }

    IIapi_static->api_mfmt.db_mny_prec = 2;
    NMgtAt( "II_MONEY_PREC", &ptr );

    if ( ptr  &&  *ptr )
	if ( *ptr == '0' )
	    IIapi_static->api_mfmt.db_mny_prec = 0;
	else  if ( *ptr == '1' )
	    IIapi_static->api_mfmt.db_mny_prec = 1;
	else  if ( *ptr != '2' )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_initADF: invalid money prec '%s', using default\n", 
		  ptr );
	}

    IIapi_static->api_decimal.db_decimal = '.';
    NMgtAt( "II_DECIMAL", &ptr );

    if ( ptr  &&  *ptr )
	if ( ptr[1] == EOS && (ptr[0] == '.' || ptr[0] == ',') )
	    IIapi_static->api_decimal.db_decimal = ptr[0];
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_initADF: invalid decimal char '%s', using default\n",
		  ptr );
	}

    return( TRUE );
}


/*
** Name: IIapi_termADF
**
** Description:
**	Shutdown ADF.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	19-Jan-96 (gordy)
**	    Extracted session level shutdown to IIapi_termADFSession().
**	17-Mar-97 (gordy)
**	    Free global resources.
*/

II_EXTERN II_VOID
IIapi_termADF( II_VOID )
{
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "IIapi_termADF: Terminating ADF\n" );

    adg_shutdown();

    if ( IIapi_static->api_timezone )
    {
	MEfree( (PTR)IIapi_static->api_timezone );
	IIapi_static->api_timezone = NULL;
    }

    MEfree( IIapi_static->api_adf_cb );
    IIapi_static->api_adf_cb = NULL;

    return;
}



/*
** Name: IIapi_initADFSession
**
** Description:
**	Initialize an ADF session.
**
** Input:
**	envHndl		Environment handle for session.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE otherwise.
**
** History:
**	19-Jan-96 (gordy)
**	    Extracted for IIapi_initADF().
**	07-Feb-97 (somsa01)
**	    Added setting of II_DATE_FORMAT, II_MONEY_FORMAT, II_MONEY_PREC,
**	    and II_DECIMAL.  (Bug #80254)
**	17-Mar-97 (gordy)
**	    Cleanup defaults for ADF parameters.
**	13-Apr-2004 (hanje04)
**	    SIR 111507
**	    Define adf_cb->df_outarg.ad_f8width in IIapi_initADFSession()
**	    which was added for BIGINT (i8) support.
**	    (See inkdo01 change 467781 for more info)
**	23-Apr-2007 (kiria01) 118141
**	    Explicitly initialise Unicode items in the ADF_CB so that
**	    the Unicode session characteristics get reset per session.
**	11-May-2007 (gupsh01)
**	    Initialize adf_utf8_flag. This gets set in adginit.
**	 8-Aug-2007 (gordy)
**	    Set date type alias.
*/

II_EXTERN II_BOOL
IIapi_initADFSession( IIAPI_ENVHNDL *envHndl )
{
    ADF_CB	*adf_cb;
    STATUS	r_stat;
    i4	timezone;
    char	*ptr;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
        ( "IIapi_initADFSession: Initializing an ADF session\n" );

    envHndl->en_adf_cb = MEreqmem( 0, sizeof( ADF_CB ), TRUE, &r_stat );
    
    if ( ! (adf_cb = (ADF_CB *)envHndl->en_adf_cb) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_initADFSession: error allocating ADF control block\n" );
	return( FALSE );
    }
    
    adf_cb->adf_srv_cb = IIapi_static->api_adf_cb;
    adf_cb->adf_tzcb = IIapi_static->api_tz_cb;
    adf_cb->adf_year_cutoff = IIapi_static->api_year_cutoff;
    adf_cb->adf_dfmt = IIapi_static->api_dfmt;
    adf_cb->adf_date_type_alias = IIapi_static->api_date_alias;
    adf_cb->adf_decimal.db_decspec = TRUE;
    adf_cb->adf_decimal.db_decimal = IIapi_static->api_decimal.db_decimal;
    adf_cb->adf_mfmt.db_mny_lort = IIapi_static->api_mfmt.db_mny_lort;
    adf_cb->adf_mfmt.db_mny_prec = IIapi_static->api_mfmt.db_mny_prec;
    adf_cb->adf_slang = IIapi_static->api_slang;

    STcopy(IIapi_static->api_mfmt.db_mny_sym, adf_cb->adf_mfmt.db_mny_sym);

    adf_cb->adf_outarg.ad_c0width = -1;
    adf_cb->adf_outarg.ad_t0width = -1;
    adf_cb->adf_outarg.ad_i1width = -1;
    adf_cb->adf_outarg.ad_i2width = -1;
    adf_cb->adf_outarg.ad_i4width = -1;
    adf_cb->adf_outarg.ad_i8width = -1;
    adf_cb->adf_outarg.ad_f4width = -1;
    adf_cb->adf_outarg.ad_f8width = -1;
    adf_cb->adf_outarg.ad_f4prec = -1;
    adf_cb->adf_outarg.ad_f8prec = -1;
    adf_cb->adf_outarg.ad_f4style = NULLCHAR;
    adf_cb->adf_outarg.ad_f8style = NULLCHAR;
    adf_cb->adf_errcb.ad_errmsgp = NULL;
    adf_cb->adf_exmathopt = ADX_IGN_MATHEX;
    adf_cb->adf_qlang = DB_SQL;
    adf_cb->adf_nullstr.nlst_length = 0;
    adf_cb->adf_nullstr.nlst_string = NULL;
    adf_cb->adf_maxstring = DB_MAXSTRING;
    adf_cb->adf_collation = NULL;
    adf_cb->adf_rms_context = -1;
    adf_cb->adf_proto_level = 0;
    adf_cb->adf_lo_context = 0;
    adf_cb->adf_strtrunc_opt = ADF_IGN_STRTRUNC;
    adf_cb->adf_ucollation = 0;
    adf_cb->adf_uninorm_flag = 0;
    adf_cb->adf_unisub_status = AD_UNISUB_OFF;
    adf_cb->adf_unisub_char[0] = '\0';
    adf_cb->adf_utf8_flag = 0;

    r_stat = adg_init( envHndl->en_adf_cb );

    if ( r_stat != OK )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_initADFSession: error initializing ADF 0x%x\n", r_stat );
	MEfree( envHndl->en_adf_cb );
	envHndl->en_adf_cb = NULL;
	return( FALSE );
    }
    
    return( TRUE );
}



/*
** Name: IIapi_termADFSession
**
** Description:
**	Shutdown an ADF session.
**
** Input:
**	envHndl		Environment handle for session.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	19-Jan-96 (gordy)
**	    Extracted from IIapi_termADF().
**	11-Dec-02 (gordy)
**	    Fixed tracing level.
*/

II_EXTERN II_VOID
IIapi_termADFSession( IIAPI_ENVHNDL *envHndl )
{
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
        ( "IIapi_termADFSession: Terminating ADF session\n" );

    adg_release( envHndl->en_adf_cb );
    MEfree( envHndl->en_adf_cb );
    envHndl->en_adf_cb = NULL;
    
    return;
}
