/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>
# include <adf.h>
# include <adp.h>

# include <iiapi.h>
# include <api.h>
# include <apidatav.h>
# include <apienhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apidatav.c
**
** Description:
**	This file contains functions which converts data value 
**	and descriptor between GCA and API.  The API descriptor 
**	is represented by IIAPI_DESCRIPTOR and the API data value 
**	is represented by IIAPI_DATAVALUE.  The GCA descriptor 
**	and data value are represented by GCA_DATA_VALUE.  The 
**	descriptor and data parameters of API and GCA may have
**	different semantic, the differences are listed below.
**
**	parameter	API			GCA
**					            
**	data type	data type of data	data type +
**						nullable flag
**					            
**	length		[variable length] +	[variable length +]
**			length of data		length of data +
**						[null byte]
**					            
**	null flag	II_BOOLean in		included in the data buffer
**			IIAPI_DATAVALUE
**					            
**	nullable flag	II_BOOLean in		included in data type
**                      IIAPI_DESCRIPTOR
**    					            
**	data		[variable length] +	[variable length] +
**			data value		data value +
**						[null flag]
**
**	Note that the length for variable length data in IIAPI_DESCRIPTOR
**	includes any trailing padding, while the length in IIAPI_DATAVALUE
**	is just the actual text length (+ 2 byte length).
**
**	IIapi_getAPILength		Get length of GCA data in API terms.
**	IIapi_getGCALength		Get length of GCA data in GCA terms.
**	IIapi_cnvtDescr2GDV		copy API descriptor to GCA data value.
**	IIapi_cnvtDataValue2GDV		copy API data value to GCA data value.
**	IIapi_cnvtGDV2Descr		copy GCA data value to API descriptor.
**	IIapi_cnvtGDV2DataValue		copy GCA data value to API data value.
**	IIapi_loadColumns		copy GCA tuple to API data values.
**
** History:
**      04-may-94 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.
**	11-Nov-94 (gordy)
**	    Return internal length in buffer with variable length data.
**	 9-Dec-94 (gordy)
**	    Don't clear NULL byte if non-nullable.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	10-Mar-95 (gordy)
**	    Make sure dv_null is FALSE if not null.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	22-May-95 (gordy)
**	    Fixed problem with variable length types.
**	13-Jun-95 (gordy)
**	    Initial support for BLOBs.
**	20-Jun-95 (gordy)
**	    Additional support for sending BLOBs as parameters.
**	11-Jul-95 (gordy)
**	    Conditionalize BLOB support for 6.4 platforms.
**	13-Jul-95 (gordy)
**	    Add missing header files, casts for type correctness.
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Fixed tracing.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	16-Feb-96 (gordy)
**	    Removed IIAPI_SDDATA structure and associated functions.
**	15-Mar-96 (gordy)
**	    Added support for compressed varchars with IIapi_getCompLength()
**	    and IIapi_cnvtComp2DataValue().  Two byte data lengths should
**	    be unsigned.
**	27-Mar-96 (gordy)
**	    Made compressed varchar routines local and renamed.
**	 3-Apr-96 (gordy)
**	    BLOB segments may now be sent using the formatted GCA interface.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	20-May-96 (gordy)
**	    Changes in GCA buffering result in the BLOB end-of-segments
**	    indicator being available after reading a data segment, so
**	    check for end of blob after satisfying a segment request.
**	18-Sep-96 (gordy)
**	    Pad/fill data with appropriate values.
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**	18-Mar-97 (gordy)
**	    BLOB processing now fills the applications data value buffer
**	    rather than just returning what segment portion may be available.
**	    This will provide more consistent response to the application
**	    and fewer calls to IIapi_getColumns().
**      21-Aug-98 (rajus01)
**          Added environment parameter to IIapi_cnvtGDVDescr().
**	    BLOB segment length can be set through IIapi_setEnvParam().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	 1-Mar-01 (gordy)
**	    Added support for Unicode datatypes: IIAPI_NCHA_TYPE,
**	    IIAPI_NVCH_TYPE, IIAPI_LNVCH_TYPE.
**	 3-Aug-01 (thoda04)
**	    Added more trace info for IIapi_loadColumns.
**	12-Jul-02 (gordy)
**	    Fixed handling of long column values in cnvtComp2DataValue.
**	    Added cnvtGDV2DataValue to handle long fixed column values.
**	31-May-06 (gordy)
**	    New ANSI date/type types utilize precision.
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from MEcopy calls.
**	25-Oct-06 (gordy)
**	    Added support for Locators.  General cleanup to reduce duplication.
**	15-Mar-07 (gordy)
**	    Support scrollable cursors.
**	11-Aug-08 (gordy)
**	    Added additional column info.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Removed obsolete functions and updated remaining.
**	28-Jul-10 (kiria01) b124142
**	    Tighten the access to ADF_NVL_BIT
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs.
*/


/*
** Local functions.
*/
static	II_VOID	load_columns( IIAPI_STMTHNDL *, 
			      IIAPI_GETCOLPARM *, IIAPI_MSG_BUFF * );

static	II_VOID	checkBLOBSegment( IIAPI_STMTHNDL *, 
				  IIAPI_DESCRIPTOR *, IIAPI_MSG_BUFF * );

static	II_BOOL	load_blob( IIAPI_STMTHNDL *, IIAPI_GETCOLPARM *,
			   IIAPI_DESCRIPTOR *, IIAPI_MSG_BUFF *,
			   IIAPI_DATAVALUE * );

static	II_BOOL	cnvtGDV2DataValue( IIAPI_STMTHNDL *, IIAPI_DESCRIPTOR *, 
				   IIAPI_MSG_BUFF *, IIAPI_DATAVALUE *, 
				   II_BOOL );

static	II_BOOL	cnvtBLOB2DataValue( IIAPI_STMTHNDL *, IIAPI_DESCRIPTOR *,
				    IIAPI_MSG_BUFF *, IIAPI_DATAVALUE * );



/*
** The following macros take GCA values for type
** and length, and expect the data to point to a
** buffer which can hold the GCA form of the data.
**
** Can be used on LOB and compressed variable
** length data if length is specific for value.
*/
#define IIAPI_ISNULL(type,length,data)  \
	((type) > 0 ? FALSE  \
		  : (*(((II_CHAR *)(data)) + (length) - 1) & ADF_NVL_BIT  \
			? TRUE : FALSE) )

#define IIAPI_SETNULL(length,data)  \
	(*(((II_CHAR *)(data)) + (length) - 1) |= (II_CHAR)ADF_NVL_BIT)

#define IIAPI_SETNOTNULL(length,data)  \
	(*(((II_CHAR *)(data)) + (length) - 1) &= (II_CHAR)~ADF_NVL_BIT)

/*
** The following macros take the API values for type
** and length (as stored in the descriptor), and expect
** the data to point to a buffer which can hold the
** GCA form of the data.
*/
#define IIAPI_ISNULLDESC(descriptor,data)  \
	( (descriptor)->ds_nullable  \
	    ? (*((II_CHAR *)(data) + IIapi_getGCALength(descriptor) - 1)  \
								& ADF_NVL_BIT \
		? TRUE : FALSE)  \
	    : FALSE )

#define IIAPI_SETNULLDESC(descriptor,data)  \
	(*((II_CHAR *)(data) + IIapi_getGCALength(descriptor) - 1)  \
							|= (II_CHAR)ADF_NVL_BIT)

#define IIAPI_SETNOTNULLDESC(descriptor,data)  \
	(*((II_CHAR *)(data) + IIapi_getGCALength(descriptor) - 1)  \
							&= (II_CHAR)~ADF_NVL_BIT) 



/*
** Name: IIapi_getAPILength
**
** Description:
**	This function returns the length of a data value as 
**	represented within the API depending on its GCA data 
**	type.  
**
**	For variable length types, the length of the actual 
**	data will be returned if a data value is provided.
**	Otherwise, the maximum length is returned.
**
**	For LOB types, a data value is expected to be a data
**	segment (non-NULL LOB, not including the data segment
**	indicator).  Otherwise, the maximum segment length is 
**	returned.
**
**	Compressed variable length data values are not supported.
**
** Input:
**	envHndl		Environment handle (may be NULL).
**	dataType	Data type of the value.
**	length		Maximum length of the data value.
**	buffer		Data value (may be NULL).
**
** Output:
**	None.
**
** Returns:
**	II_INT2		Length of value in API format.
**
** History:
**      03-may-94 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.  Renamed.
**	11-Nov-94 (gordy)
**	    Return internal length in buffer with variable length data.
**	22-May-95 (gordy)
**	    Don't deref ptr as arg to IIAPI_ISNULL macro.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	25-Oct-06 (gordy) Support LOBs and Locators.  Added envHndl param.
*/

II_EXTERN II_UINT2
IIapi_getAPILength 
(
    IIAPI_ENVHNDL	*envHndl,
    IIAPI_DT_ID		dataType,
    II_LONG		dataLength,
    char		*buffer
)
{
    II_LONG length;

    switch( abs( dataType ) )
    {
    case IIAPI_LVCH_TYPE :
    case IIAPI_LBYTE_TYPE :
    case IIAPI_LNVCH_TYPE :
    case IIAPI_GEOM_TYPE :
    case IIAPI_POINT_TYPE :
    case IIAPI_MPOINT_TYPE :
    case IIAPI_LINE_TYPE :
    case IIAPI_MLINE_TYPE :
    case IIAPI_POLY_TYPE :
    case IIAPI_MPOLY_TYPE :
    case IIAPI_GEOMC_TYPE :
	if ( buffer != NULL )
	{
	    II_UINT2	seg_len;

	    MECOPY_CONST_MACRO( buffer, sizeof( seg_len ), (PTR)&seg_len );
	    length = sizeof( seg_len ) + 
		     (IIapi_isUCS2( dataType ) ? seg_len * 2 : seg_len);
	}
	else  if ( IIapi_isEnvHndl( envHndl )  &&
	           (envHndl->en_usrParm.up_mask1 & IIAPI_UP_MAX_SEGMENT_LEN) )
	    length = envHndl->en_usrParm.up_max_seglen;
	else
	    length = IIAPI_MAX_SEGMENT_LEN;
	break;

    case IIAPI_LCLOC_TYPE :
    case IIAPI_LBLOC_TYPE :
    case IIAPI_LNLOC_TYPE :
	length = sizeof( ADP_LOCATOR );
	break;

    default :
    {
	II_UINT2 var_len;

	if ( buffer == NULL  ||  ! IIapi_isVAR( dataType )  ||
	     IIAPI_ISNULL( dataType, dataLength, buffer ) )
	    length = (dataType < 0) ? dataLength - 1 : dataLength;
	else
	{
	    MECOPY_CONST_MACRO( buffer, sizeof( var_len ), (PTR)&var_len );
	    length = sizeof( var_len ) + 
		     (IIapi_isUCS2( dataType ) ? var_len * 2 : var_len);
	}
	break;
    }
    }

    return( (II_UINT2)length );
}



/*
** Name: IIapi_getGCALength
**
** Description:
**	This function returns the length of a data value as 
**	represented within GCA (including null indicator and
**	variable data length) depending on its API data type.  
**
**	For LOBs, only the fixed sized portion of the variable
**	segmented format is represented.
**
**	For variable length types which may be compressed, the
**	length returned is the maximum length.
**
** Input:
**	descriptor	API data descriptor.
**
** Output:
**	None.
**
** Return value:
**	II_UINT2	Length of the value in GCA format.
**
** History:
**	 7-Oct-94 (gordy)
**	    Created.
**	11-Nov-94 (gordy)
**	    Return internal length in buffer with variable length data.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	25-Oct-06 (gordy)
**	    Support LOBs (fixed portion) and Locators.
*/

II_EXTERN II_UINT2
IIapi_getGCALength( IIAPI_DESCRIPTOR *descriptor )
{
    II_UINT2	length;

    switch( descriptor->ds_dataType )
    {
    case IIAPI_LVCH_TYPE :
    case IIAPI_LBYTE_TYPE :
    case IIAPI_LNVCH_TYPE :	
    case IIAPI_GEOM_TYPE :
    case IIAPI_POINT_TYPE :
    case IIAPI_MPOINT_TYPE :
    case IIAPI_LINE_TYPE :
    case IIAPI_MLINE_TYPE :
    case IIAPI_POLY_TYPE :
    case IIAPI_MPOLY_TYPE :
    case IIAPI_GEOMC_TYPE :
	/*
	** For LOBs, the fixed sized portion of the 
	** variable length segmented format is used.
	** It includes the LOB header and the first
	** segment indicator.
	*/
	length = ADP_HDR_SIZE + sizeof( i4 );
	break;

    case IIAPI_LCLOC_TYPE :
    case IIAPI_LBLOC_TYPE :
    case IIAPI_LNLOC_TYPE :
	length = ADP_LOC_PERIPH_SIZE;
	break;

    default :
	length = descriptor->ds_length;
	break;
    }

    if ( descriptor->ds_nullable )  length++;	/* NULL indicator byte */
    return( length );
}



/*
** Name: IIapi_cnvtDescr2GDV
**
** Description:
**	Convert API descriptor meta-data into GCA format.
**
**	For LOBs, the minimum fixed size is used.
**
** Input:
**	descriptor	API descriptor
**
** Output:
**	gcaDataValue	GCA data value
**
** Returns:
**	II_BOOL		TRUE if success, FALSE if memory allocation failure.
**
** History:
**     02-may-94 (ctham)
**         creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.
**	11-Nov-94 (gordy)
**	    Return internal length in buffer with variable length data.
**	 9-Dec-94 (gordy)
**	    Don't clear NULL byte if non-nullable.
**	20-Jun-95 (gordy)
**	    Renamed to more closely reflect functionality.
**	31-May-06 (gordy)
**	    New ANSI date/type types utilize precision.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN II_VOID
IIapi_cnvtDescr2GDV
(
    IIAPI_DESCRIPTOR	*descriptor,
    GCA_DATA_VALUE	*gcaDataValue
)
{
    gcaDataValue->gca_type = descriptor->ds_nullable 
			   ? -descriptor->ds_dataType : descriptor->ds_dataType;

    gcaDataValue->gca_l_value = IIapi_getGCALength( descriptor );

    if ( descriptor->ds_dataType == IIAPI_DEC_TYPE )
	gcaDataValue->gca_precision = 
	    DB_PS_ENCODE_MACRO(descriptor->ds_precision, descriptor->ds_scale);
    else
	gcaDataValue->gca_precision = descriptor->ds_precision;
    
    return;
}


/*
** Name: IIapi_cnvtDataValue2GDV
**
** Description:
**	This function copies the content of an API data value
**	into the GCA format.  Caller must ensure sufficient 
**	space, as indicated by IIapi_getGCALength(), exists 
**	for the formatted value.
**
**	This function does not support LOBs or LOB locators.
**
** Input:
**	descriptor	API descriptor
**	dataValue	API data value
**	gcaValue	GCA tuple
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**      04-may-94 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.
**	11-Nov-94 (gordy)
**	    Return internal length in buffer with variable length data.
**	 9-Dec-94 (gordy)
**	    Don't clear NULL byte if non-nullable.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	18-Sep-96 (gordy)
**	    Pad/fill data with appropriate values.
**	25-Oct-06 (gordy)
**	    Support NULL/single segment LOBs, Locators.
*/

II_EXTERN II_VOID
IIapi_cnvtDataValue2GDV
(
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_DATAVALUE	*dataValue,
    II_PTR		gcaValue
)
{
    II_UINT2 length;

    if ( descriptor->ds_nullable  &&  dataValue->dv_null )
    {
	length = IIapi_getGCALength( descriptor );
	MEfill( length, '\0', gcaValue );
	IIAPI_SETNULL( length, gcaValue );
        return;
    }

    length = min( descriptor->ds_length, dataValue->dv_length );
    MEcopy( dataValue->dv_value, length, gcaValue );

    if ( length < descriptor->ds_length )
	switch( descriptor->ds_dataType )
	{
	case IIAPI_NCHA_TYPE :
	{
	    u_i2 pad = 0x0020;	/* UCS-2 space */
	    u_i1 *ptr;

	    for( 
		 ptr = (u_i1 *)gcaValue + length; 
		 length < descriptor->ds_length; 
		 length += sizeof(pad), ptr += sizeof(pad) 
	       )
		MECOPY_CONST_MACRO( (PTR)&pad, sizeof(pad), (PTR)ptr );
	    break;
	}
	case IIAPI_CHA_TYPE :
	case IIAPI_CHR_TYPE :
	    MEfill( descriptor->ds_length - length, ' ', 
		    (PTR)((u_i1 *)gcaValue + length) );
	    break;

	default :
	    MEfill( descriptor->ds_length - length, '\0', 
		    (PTR)((u_i1 *)gcaValue + length) );
	    break;
	}

    length = descriptor->ds_length;
    if ( descriptor->ds_nullable )  IIAPI_SETNOTNULL( length + 1, gcaValue );

    return;
}



/*
** Name: IIapi_cnvtGDV2Descr
**
** Description:
**	This function copies the content of a GCA descriptor to
**	an API descriptor.
**
**	This function can handle long data types.
**
** Input:
**	envHndl		Environment handle associated with connection.
**	gcaDataType	GCA data type.
**	gcaLength	GCA data length.
**	gcaPrecision	GCA data precision.
**
** Output:
**	descriptor	API descriptor.
**
** Return:
**	VOID
**
** History:
**     02-may-94 (ctham)
**         creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.
**	13-Jun-95 (gordy)
**	    BLOB GCA info does not provide actual or segment length.
**	    Set BLOB API descriptors to max segment length.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	21-Oct-99 (rajus01)
**	    Changed BLOB segment length flag indicator from
**	    IIAPI_EP_MAX_SEGMENT_LEN to IIAPI_UP_MAX_SEGMENT_LEN.
**	31-May-06 (gordy)
**	    New ANSI date/type types utilize precision.
**	25-Mar-10 (gordy)
**	    Column name is no longer saved.
*/

II_EXTERN II_VOID
IIapi_cnvtGDV2Descr
(	
    IIAPI_ENVHNDL	*envHndl,
    IIAPI_DT_ID		gcaDataType,
    i4			gcaLength,
    i4			gcaPrecision,
    IIAPI_DESCRIPTOR	*descriptor
)
{
    descriptor->ds_columnType = IIAPI_COL_TUPLE;
    descriptor->ds_dataType = abs( gcaDataType );
    descriptor->ds_nullable = gcaDataType < 0;
    descriptor->ds_length = IIapi_getAPILength( envHndl, gcaDataType,
    						gcaLength, NULL );

    if ( descriptor->ds_dataType == IIAPI_DEC_TYPE )
    {
	descriptor->ds_precision = DB_P_DECODE_MACRO( gcaPrecision );
	descriptor->ds_scale = DB_S_DECODE_MACRO( gcaPrecision );
    }
    else
    {
	descriptor->ds_precision = gcaPrecision;
        descriptor->ds_scale = 0;
    }

    descriptor->ds_columnName = NULL;
    return;
}



/*
** Name: IIapi_cnvtGDV2DataValue
**
** Description:
**	This function copies an entire GCA data value into API format.  
**	Caller must allocate data value buffer at least as large as the 
**	length in the descriptor.
**
**	This function does not handle LOBs, locators, partial or
**	compressed variable length data values.
**
** Input:
**	descriptor	descriptor of data value
**	buffer		data value
**
** Output:
**	dataValue	API buffer to store data value
**
** Returns:
**	Void.
**
** History:
**      03-may-94 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 7-Oct-94 (gordy)
**	    Fixed handling of variable length types.
**	10-Mar-95 (gordy)
**	    Make sure dv_null is FALSE if not null.
**	13-Jun-95 (gordy)
**	    Do not allocate data value buffers.  Caller must
**	    assure that dv_value is large enough for data.
**	    Return type changed to void.
*/

II_EXTERN II_VOID
IIapi_cnvtGDV2DataValue 
(
    IIAPI_DESCRIPTOR	*descriptor,
    char		*gcaValue,
    IIAPI_DATAVALUE	*dataValue
)
{
    if ( IIAPI_ISNULLDESC( descriptor, gcaValue ) )
    {
	dataValue->dv_null = TRUE;
	dataValue->dv_length = 0;
    	return;
    }

    dataValue->dv_null = FALSE;

    /*
    ** This may seem like a circular way of doing
    ** things, but IIapi_getAPILength() will handle
    ** variable length data types by looking at the
    ** actual data rather than just the descriptor.
    ** Since IIapi_getAPILength() requires the GCA
    ** data length and all we have is the max API 
    ** length in the descriptor, we also have to
    ** call IIapi_getGCALength().
    */
    dataValue->dv_length = 
	    IIapi_getAPILength( NULL, descriptor->ds_nullable 
				      ? -descriptor->ds_dataType 
				      : descriptor->ds_dataType,
				IIapi_getGCALength( descriptor ), gcaValue );

    MEcopy( gcaValue, dataValue->dv_length, dataValue->dv_value );
    return;
}



/*
** Name: IIapi_loadColumns
**
** Description:
**	Copy data values from GCA tuple to API data values.  This
**	routine handles Binary Large OBject columns, compressed 
**	variable length columns, and fixed length columns.  Multi-
**	row fetchs are also supported.
**
** Input:
**	stmtHndl	Statement Handle.
**	getColParm	IIapi_getColumns() parameters.
**	msgBuff		GCA Receive buffer.
**
** Output:
**	None.
**
** Returns:
**
** History:
**	13-Jun-95 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing.
**	15-Mar-96 (gordy)
**	    Added support for compressed varchars.
**	27-Mar-96 (gordy)
**	    Extracted individual column handling to load_columns().
**	15-Mar-07 (gordy)
**	    Track the number of rows returned for a cursor fetch.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN II_VOID
IIapi_loadColumns
(
    IIAPI_STMTHNDL	*stmtHndl,
    IIAPI_GETCOLPARM	*getColParm,
    IIAPI_MSG_BUFF	*msgBuff
)
{
    /*
    ** If no data available or request complete, we be done!
    */
    if ( msgBuff->length < 1  ||  ! stmtHndl->sh_colFetch  ||
	 getColParm->gc_rowsReturned >= getColParm->gc_rowCount )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "IIapi_loadColumns: nothing to do\n" );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_loadColumns: converting tuple data to API format\n" );
    
    /*
    ** Loop for each row in the request.
    */
    do
    {
	/*
	** Process columns in the current row.  The current
	** column request should not span rows, but may be
	** fewer than the remaining columns in the row.  If
	** this is a multi-row fetch, we will reset the
	** column request for the next row after processing
	** the current row.  The current row request will
	** either be satisfied, or the tuple data will be
	** exhausted.
	*/
	load_columns( stmtHndl, getColParm, msgBuff );

	/*
	** If there are still columns to be processed in
	** the current request, exit to get more data.
	*/
	if ( stmtHndl->sh_colFetch )  break;

	/*
	** We have completed the request for the current row.
	** We bump the number of rows returned since we either
	** completed a row or completed a partial fetch of a
	** row.  We will either exit at this point or move
	** on to the next row.
	*/
	getColParm->gc_rowsReturned++;

	/*
	** Check to see if we have completed a row.
	*/
	if ( stmtHndl->sh_colIndex >= stmtHndl->sh_colCount )
	{
	    /*
	    ** For cursors, decrement the remaining row count now
	    ** that a row has been completed.
	    */
	    if ( stmtHndl->sh_flags & IIAPI_SH_CURSOR )
		stmtHndl->sh_rowCount--;

	    /*
	    ** Set the column index for the start of the next row.
	    ** If there are more rows in the request, set the 
	    ** column fetch count for the next row.
	    */
	    stmtHndl->sh_colIndex = 0;

	    if ( getColParm->gc_rowsReturned < getColParm->gc_rowCount )
		stmtHndl->sh_colFetch = stmtHndl->sh_colCount;
	}

    } while( stmtHndl->sh_colFetch );

    return;
}



/*
** Name: load_columns
**
** Description:
**	Copy data values from GCA tuple to API data values.  This
**	routine handles Binary Large OBject columns, compressed 
**	variable length columns, and fixed length columns.  This
**	routine only reads columns for the current row.  Multi-row
**	requests must call this routine for each row.
**
** Input:
**	stmtHndl	Statement Handle.
**	getColParm	IIapi_getColumns() parameters.
**	msgBuff		GCA Receive buffer.
**
** Output:
**	None.
**
** Returns:
**
** History:
**	13-Jun-95 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing.
**	15-Mar-96 (gordy)
**	    Added support for compressed varchars.
**	27-Mar-96 (gordy)
**	    Extracted from IIapi_loadColumns() for single row processing.
**	20-May-96 (gordy)
**	    Changes in GCA buffering result in the BLOB end-of-segments
**	    indicator being available after reading a data segment, so
**	    check for end of blob after satisfying a segment request.
**	18-Mar-97 (gordy)
**	    Extracted buffer processing for BLOBs to facilitate segment
**	    concatenation.
**	12-Jul-02 (gordy)
**	    Check for column values which are split across GCA buffers.
**	    Call cnvtGDV2DataValue for long data values.
**	25-Oct-06 (gordy)
**	    Use cnvtGDV2DataValue for all non-LOB values.
**	11-Aug-08 (gordy)
**	    Clear all column info at start of row and mark info for 
**	    column as available before advancing to next column.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

static II_VOID
load_columns
(
    IIAPI_STMTHNDL	*stmtHndl,
    IIAPI_GETCOLPARM	*getColParm,
    IIAPI_MSG_BUFF	*msgBuff
)
{
    IIAPI_DESCRIPTOR	*descr;
    IIAPI_DATAVALUE	*value;
    i4			length;
    u_i2		len;
    bool		done;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_loadColumns: %d columns starting with %d, %d total columns\n",
	  (II_LONG)stmtHndl->sh_colFetch, 
	  (II_LONG)stmtHndl->sh_colIndex, 
	  (II_LONG)stmtHndl->sh_colCount );

    descr = &stmtHndl->sh_colDescriptor[ stmtHndl->sh_colIndex ];
    value = &getColParm->gc_columnData[ (getColParm->gc_rowsReturned * 
					 getColParm->gc_columnCount) +
					 getColParm->gc_columnCount -
					 stmtHndl->sh_colFetch ];

    /*
    ** Process columns until all done or input buffer exhausted.
    */
    for( ; stmtHndl->sh_colFetch; 
	 stmtHndl->sh_colFetch--, stmtHndl->sh_colIndex++, descr++, value++ )
    {
	/*
	** Clear column info at start of row.  The more segments
	** flag must not be set otherwise the first column is a
	** LOB which has already been partially processed.  There
	** must also be at least some data available for the first
	** column, otherwise we may be clearing the info when the
	** last row has already been received.
	*/
	if ( 
	     stmtHndl->sh_colIndex == 0  &&
	     ! (stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS)  &&
	     msgBuff->length > 0 
	   )
	{
	    i2 i;

	    for( i = 0; i < stmtHndl->sh_colCount; i++ )
	        stmtHndl->sh_colInfo[ i ].ci_flags = 0;
	}

	/*
	** Blobs are broken into segments of variable lengths
	** and require special handling to load segments using 
	** internal data value info.
	*/
	if ( IIapi_isBLOB( descr->ds_dataType ) )
	    done = load_blob( stmtHndl, getColParm, descr, msgBuff, value );
	else
	    done = cnvtGDV2DataValue( stmtHndl, descr, msgBuff, value, 
		(stmtHndl->sh_flags & IIAPI_SH_COMP_VARCHAR) ? TRUE : FALSE );

	if ( ! done )  break;	/* Need more data for current column */

	stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |=
								IIAPI_CI_AVAIL;

	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_loadColumns: loaded data for column %d\n",
	      (II_LONG)stmtHndl->sh_colIndex );
    }

    return;
}



/*
** Name: checkBLOBSegment
**
** Description:
**	Checks the GCA buffer for the next segment indicator
**	(MORE_SEGMENTS flag must be set).  If it is the 
**	end-of-segments indicator, removes the indicator and 
**	(optional) null byte from the buffer and turns off the 
**	MORE_SEGMENTS flag.  Otherwise, nothing is done.
**
**	The null byte value is ignored (if present).  The null 
**	byte is only significant when the BLOB header is im-
**	mediatly followed by an end-of-segments indicator.  
**	This case is completely handled by cnvtBLOB2DataValue() 
**	and so this routine should never be called when the null 
**	indicator is significant.
**
**	This routine does nothing if segment purging is active.
**
** Input:
**	stmtHndl	Statement Handle.
**	descriptor	API descriptor of BLOB.
**	msgBuff		GCA tuple buffer.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**	13-Jun-95 (gordy)
**	    Created.
**	11-Aug-08 (gordy)
**	    Mark column info as available before advancing to next column.
**	26-Jan-10 (rajus01) SD issue 142188, Bug 123159
**	    Decrement the rowcount when the tuple is completed.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

static II_VOID
checkBLOBSegment
( 
    IIAPI_STMTHNDL	*stmtHndl, 
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_MSG_BUFF	*msgBuff 
)
{
    i4  indicator;
    i4  length = sizeof( indicator );

    /*
    ** We only check when we know it is safe to do so.
    ** We must be reading segments, but not purging.
    ** and we require the request to be complete to
    ** avoid scenarios to complicated to explain or
    ** properly handle.
    */
    if ( stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS  &&
         ! ( stmtHndl->sh_flags & IIAPI_SH_PURGE_SEGMENTS )  &&
	 ! stmtHndl->sh_colFetch  &&  msgBuff->length >= length )
    {
	/*
	** Peak at the indicator.
	*/
	MECOPY_CONST_MACRO( msgBuff->data, length, (PTR)&indicator );

	if ( ! indicator )
	{
	    /*
	    ** We have the end-of-segments indicator.
	    ** Only consume it if the (optional) null
	    ** byte is also available (if not nullable,
	    ** the test is redundant).  The null byte
	    ** value is ignored (see description).
	    */
	    if ( descriptor->ds_nullable )  length++;

	    if ( msgBuff->length >= length )
	    {
		IIAPI_TRACE( IIAPI_TR_VERBOSE )
		    ( "checkBLOBSegment: found end-of-segments\n" );

		msgBuff->data += length;
		msgBuff->length -= length;

		/*
		** Since we have now finished the BLOB,
		** advance the column index to the next
		** column (wrap if finished all columns).
		** Since we check for sh_colFetch to be 
		** 0, we assume the gc_rowsReturned is
		** set properly and we are not in the
		** midst of a multi-row fetch.
		*/
		stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |=
								IIAPI_CI_AVAIL;
		if ( ++stmtHndl->sh_colIndex >= stmtHndl->sh_colCount )
		{
		    /* SD issue 142188; Bug 123159.
		    ** Decrement the row count when the tuple is
		    ** completed.
		    */
		    if ( stmtHndl->sh_flags & IIAPI_SH_CURSOR )  
			stmtHndl->sh_rowCount--;
		    stmtHndl->sh_colIndex = 0;
		}
		stmtHndl->sh_flags &= ~IIAPI_SH_MORE_SEGMENTS;
	    }
	}
    }

    return;
}


/*
** Name: load_blob
**
** Description:
**	Processes BLOB segments in the GCA input buffer until the BLOB
**	is completed, the output data value buffer is full, or the GCA
**	input buffer is exhausted.  Returns TRUE when the BLOB is
**	completed.
**
**	If the output data value buffer gets filled and the current
**	column request is complete, standard column processing is
**	performed and FALSE is returned so that the processing is 
**	not duplicated.
**
** Input:
**	stmtHndl	Statement Handle.
**	getColParm	IIapi_getColumns() parameters.
**	descriptor	API descriptor of datavalue.
**	msgBuff		GCA tuple buffer containing data.
**
** Output:
**	dataValue	API datavalue buffer.
**
** Returns:
**	II_BOOL		TRUE if BLOB completed, FALSE if need more data.
**
** History:
**	18-Mar-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

static II_BOOL
load_blob
(
    IIAPI_STMTHNDL	*stmtHndl,
    IIAPI_GETCOLPARM	*getColParm,
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_MSG_BUFF	*msgBuff,
    IIAPI_DATAVALUE	*dataValue
)
{
    if ( ! (stmtHndl->sh_flags & IIAPI_SH_PURGE_SEGMENTS ) )
    {
	/*
	** Process the BLOB segments in the input buffer
	** until we empty the input buffer, the end of
	** the BLOB is reached, or we fill the output
	** buffer.
	*/
	do
	{
	    if ( ! cnvtBLOB2DataValue(stmtHndl,descriptor,msgBuff,dataValue) )
		return( FALSE );

	    if ( ! (stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
		return( TRUE );

	} while( dataValue->dv_length < descriptor->ds_length );

	/*
	** We have satisfied the request if this is the
	** last column of a single row request.  Other-
	** wise we must purge the rest of the BLOB before
	** processing the rest of the request.
	*/
	if ( stmtHndl->sh_colFetch <= 1  &&  getColParm->gc_rowCount <= 1 )
	{
	    /*
	    ** There is a (very small) chance that we
	    ** we filled the output buffer and read the
	    ** last input segment at the same time.  In
	    ** this case the end-of-segments indicator
	    ** has not been processed and may or may not
	    ** be in the input buffer.
	    **
	    ** Normally we would just return TRUE at this
	    ** point, but the special case complicates
	    ** things.  If the end-of-segments indicator
	    ** is in the input buffer, we want to process
	    ** it so that the application can be told
	    ** that this is the last segment.  The check
	    ** can be made by calling checkBLOBSegment(),
	    ** but some special conditions must be handled.
	    **
	    ** CheckBLOBSegment() requires the request to be 
	    ** complete and will change the column index if 
	    ** the end-of-segments indicator is processed.
	    ** The result of all this is that we must do the
	    ** work to finish the column processing and return
	    ** FALSE so that the caller will not duplicate the
	    ** work.
	    */
	    stmtHndl->sh_colFetch--;
	    checkBLOBSegment( stmtHndl, descriptor, msgBuff );
	    return( FALSE );
	}

	stmtHndl->sh_flags |= IIAPI_SH_PURGE_SEGMENTS;
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "cnvtBLOB2DataValue: begin purging segments\n" );
    }

    /*
    ** Purge the input buffer of BLOB segments
    ** until we find the end of the blob or we
    ** empty the input buffer.
    */
    while( stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS )
	if ( ! cnvtBLOB2DataValue( stmtHndl, descriptor, msgBuff, dataValue ) )
	    return( FALSE );
    
    /*
    ** We must be done with the BLOB.
    */
    return( TRUE );
}



/*
** Name: cnvtGDV2DataValue
**
** Description:
**	This function copies a GCA data value into API format, even
**	those which may be split across multiple GCA receive buffers.  
**	Caller must allocate data value buffer at least as large as 
**	the length in the descriptor.
**
**	This function does not handle long data types.
**
** Input:
**	stmtHndl	Statement Handle.
**	descriptor	API descriptor of data value
**	msgBuff		GCA buffer.
**	dataValue	API buffer with partial data value.
**	compressed	TRUE if variable length values are compressed.
**
** Output:
**	dataValue	API buffer to store data value
**
** Returns:
**	II_BOOL		TRUE if value processed, FALSE if need more data.
**
** History:
**	12-Jul-02 (gordy)
**	    Created.
**	25-Oct-06 (gordy)
**	    Support compressed variable length data types and locators.
**	11-Aug-08 (gordy)
**	    Added addition column info to statement handle.  Extract
**	    LOB length from locator header as additional column info.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

static II_BOOL
cnvtGDV2DataValue
(
    IIAPI_STMTHNDL	*stmtHndl,
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_MSG_BUFF	*msgBuff,
    IIAPI_DATAVALUE	*dataValue,
    II_BOOL		compressed
)
{
    i4 data_len, avail_len;

    /*
    ** Check that any minimum required length 
    ** is available and determine GCA data length.
    */
    switch( descriptor->ds_dataType )
    {
    case IIAPI_VCH_TYPE :
    case IIAPI_VBYTE_TYPE :
    case IIAPI_NVCH_TYPE :
    case IIAPI_TXT_TYPE :
    case IIAPI_LTXT_TYPE :
	if ( ! compressed )
	    data_len = descriptor->ds_length;
	else
	{
	    u_i2 len;

	    /*
	    ** The embeded length indicator is required.  It may
	    ** have been already loaded (dv_length > 0) or may
	    ** be available in the input buffer.  Otherwise, wait
	    ** until it is available.
	    */
	    if ( dataValue->dv_length )
		MEcopy( dataValue->dv_value, sizeof( len ), (PTR)&len );
	    else  if ( msgBuff->length >= sizeof( len ) )  
		MEcopy( (PTR)msgBuff->data, sizeof( len ), (PTR)&len );
	    else
	    {
		IIAPI_TRACE( IIAPI_TR_VERBOSE )
		    ( "cnvtGDV2DataValue: need embedded length (%d bytes, %d available)\n",
		      sizeof( len ), msgBuff->length );
		return( FALSE );
	    }

	    /*
	    ** Get the actual length of the compressed column.
	    */
	    data_len = sizeof( len ) + 
		       (IIapi_isUCS2(descriptor->ds_dataType) ? len * 2 : len);
	}
    	break;

    case IIAPI_LCLOC_TYPE :
    case IIAPI_LBLOC_TYPE :
    case IIAPI_LNLOC_TYPE :
    {
	i4	tag;
	u_i4	len0, len1;
	u_i1	*ptr = msgBuff->data;

	/*
	** The ADF PERIPHERAL header must be read along with
	** the ADP locator value.
	*/
	if ( msgBuff->length < (ADP_HDR_SIZE + sizeof(ADP_LOCATOR)) )
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtGDV2DataValue: need LOCATOR (%d bytes, %d available)\n",
		  ADP_HDR_SIZE + sizeof(ADP_LOCATOR), msgBuff->length );
	    return( FALSE );
	}

	/*
	** With the full ADP locator object available, read
	** the header and set remaining data length.  Column
	** info is availabe once the header is read.
	*/
	msgBuff->data += ADP_HDR_SIZE;
	msgBuff->length -= ADP_HDR_SIZE;
	stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |= 
								IIAPI_CI_AVAIL;
	MECOPY_CONST_MACRO( ptr, sizeof( tag ), (PTR)&tag );

	if ( tag == ADP_P_LOCATOR )
	{
	    ptr += sizeof( tag );
	    MECOPY_CONST_MACRO( ptr, sizeof( len0 ), (PTR)&len0 );
	    ptr += sizeof( len0 );
	    MECOPY_CONST_MACRO( ptr, sizeof( len1 ), (PTR)&len1 );
	    stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |=
	    						IIAPI_CI_LOB_LENGTH;
	    stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_data.lob_length =
					((u_i8)len0 << 32) | (u_i8)len1;
	}

	data_len = sizeof( ADP_LOCATOR );
    	break;
    }
    default :
	data_len = descriptor->ds_length;
    	break;
    }

    /*
    ** Determine amount of data available to be loaded:
    ** smaller of amount in input buffer and amount not 
    ** yet loaded.
    */
    avail_len = min( msgBuff->length, data_len - dataValue->dv_length );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "cnvtGDV2DataValue: loading %d of %d bytes (%d loaded)\n",
	  avail_len, data_len, (II_LONG)dataValue->dv_length );

    /*
    ** Load available data in input buffer.
    */
    if ( avail_len )
    {
	MEcopy( (PTR)msgBuff->data, avail_len, 
		(PTR)((char *)dataValue->dv_value + dataValue->dv_length) );
	msgBuff->data += avail_len;
	msgBuff->length -= avail_len;
	dataValue->dv_length += avail_len;
    }

    /*
    ** Check to see if all data has been loaded.
    */
    if ( dataValue->dv_length < data_len )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "cnvtGDV2DataValue: insufficient data: need %d bytes\n",
	      data_len - dataValue->dv_length );
	return( FALSE );
    }

    /*
    ** For nullable columns, check the NULL byte.
    */
    if ( descriptor->ds_nullable )
    {
	/*
	** Check to see if NULL byte is in input buffer.
	*/
	if ( ! msgBuff->length )  
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtGDV2DataValue: need NULL byte (1 byte, 0 available\n" );
	    return( FALSE );
	}

	if ( ! (*msgBuff->data & ADF_NVL_BIT) )
	    dataValue->dv_null = FALSE;
	else
	{
	    dataValue->dv_null = TRUE;
	    dataValue->dv_length = 0;	/* Force to 0 since set above */
	}

	msgBuff->data++;	/* Remove NULL byte from input buffer */
	msgBuff->length--;
    }

    if ( IIapi_isVAR( descriptor->ds_dataType )  &&  ! dataValue->dv_null )
    {
	/*
	** Variable length types only return the length of
	** the actual data, not their fully padded length.
	*/
	u_i2 len;

	MEcopy( dataValue->dv_value, sizeof( len ), (PTR)&len );
	dataValue->dv_length = sizeof( len ) +
		    (IIapi_isUCS2( descriptor->ds_dataType ) ? len * 2 : len);
    }

    return( TRUE );
}



/*
** Name: cnvtBLOB2DataValue
**
** Description:
**	Converts BLOB data values from GCA tuple format to API format.
**	A single input BLOB segment (or portion) is appended to the 
**	output segment.  The output segment is initialized on the first
**	call to this routine for a new BLOB.
**	
**	If starting a new BLOB (as indicated by no MORE_SEGMENTS flag),
**	reads the ADP_PERIPHERAL header and sets the MORE_SEGMENTS flag.
**	Reads a segment indicator.  If end-of-segments, completes the
**	processing of the BLOB and clears the MORE_SEGMENTS flag.  Other-
**	wise, appends the available portion of the next segment to the
**	output segment.
**
**	Returns FALSE if insufficient data to process header or segment.
**	Additional data may remain in input buffer after processing.
**
** Input:
**	stmtHndl	Statement Handle.
**	descriptor	API descriptor of datavalue.
**	msgBuff		GCA tuple buffer containing data.
**
** Output:
**	dataValue	API datavalue buffer.
**
** Returns:
**	II_BOOL		FALSE if need more data, TRUE if successful.
**
** History:
**	13-Jun-95 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	18-Mar-97 (gordy)
**	    Added the ability to append segments to support 
**	    re-segmentation to fill the output buffer.
**	11-Aug-08 (gordy)
**	    Extract LOB length from header as addition column info.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

static II_BOOL
cnvtBLOB2DataValue 
(
    IIAPI_STMTHNDL	*stmtHndl,
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_MSG_BUFF	*msgBuff,
    IIAPI_DATAVALUE	*dataValue
)
{
    u_i2	char_size = IIapi_isUCS2( descriptor->ds_dataType ) ? 2 : 1;
    i4		indicator = 0;
    i4		length;
    u_i2	seg_len;

    /*
    ** The output buffer needs to be initialized with a 
    ** zero length segment.  Subsequent processing then 
    ** only needs to append input segments to the current 
    ** output segment.  The initial output data value 
    ** length is set to 0 (see IIapi_getColumns()) so 
    ** that we can detect when initialization is required.  
    ** Any non-zero value indicates an existing segment.
    */
    if ( ! dataValue->dv_length )
    {
	seg_len = 0;
	dataValue->dv_null = FALSE;
	dataValue->dv_length = sizeof( seg_len );
	MECOPY_CONST_MACRO( (PTR)&seg_len, 
			    sizeof( seg_len ), dataValue->dv_value );
    }

    /*
    ** The MORE_SEGMENTS flag gets set once the BLOB
    ** header is read.  If it is not set, we need to
    ** read the header.
    */
    if ( ! ( stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS ) )
    {
	i4	tag;
	u_i4	len0, len1;
	u_i1	*ptr = msgBuff->data;

	/*
	** BLOB processing assumes that if this is a NULL or
	** zero length BLOB, the entire BLOB is processed with
	** the header information.  In addition to the header,
	** we need to have access to the first segment indicator
	** and possibly the NULL byte.  Note that if the BLOB
	** is nullable but not NULL, the additional byte will
	** come from the segment length of the first segment.
	*/
	length = ADP_HDR_SIZE + sizeof( indicator ) + 
		 (descriptor->ds_nullable ? 1 : 0);

	if ( msgBuff->length < length )
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtBLOB2DataValue: need BLOB header for column %d\n",
		  (II_LONG)stmtHndl->sh_colIndex );
	    return( FALSE );
	}

	/*
	** Remove the header from the input buffer.  LOB 
	** column info is available once the header has 
	** been read.  Extract header data as column info.
	*/
	msgBuff->data += ADP_HDR_SIZE;
	msgBuff->length -= ADP_HDR_SIZE;
	stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |= 
								IIAPI_CI_AVAIL;
	MECOPY_CONST_MACRO( ptr, sizeof( tag ), (PTR)&tag );

	if ( tag == ADP_P_GCA )
	{
	    ptr += sizeof( tag );
	    MECOPY_CONST_MACRO( ptr, sizeof( len0 ), (PTR)&len0 );
	    ptr += sizeof( len0 );
	    MECOPY_CONST_MACRO( ptr, sizeof( len1 ), (PTR)&len1 );
	    stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_flags |=
	    						IIAPI_CI_LOB_LENGTH;
	    stmtHndl->sh_colInfo[ stmtHndl->sh_colIndex ].ci_data.lob_length =
					((u_i8)len0 << 32) | (u_i8)len1;
	}

	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "cnvtBLOB2DataValue: processed BLOB header for column %d\n",
	      (II_LONG)stmtHndl->sh_colIndex );

	/*
	** At this point we begin processing segments.
	*/
	stmtHndl->sh_flags |= IIAPI_SH_MORE_SEGMENTS;
    }

    /*
    ** Process the segment indicator.
    */
    length = sizeof( indicator );

    if ( msgBuff->length >= length )
    {
	/*
	** To consume the indicator, must have sufficient remaining
	** data to satisfy the request.  There are three cases:
	**
	** Indicator value	Nullable	Bytes Needed
	** ---------------	--------	------------
	**        0		   N		0 
	**        0		   Y		1 (NULL byte)
	**        1		  Y/N		3 (segment length + data)
	*/
	MECOPY_CONST_MACRO( msgBuff->data, length, (PTR)&indicator );
	length += indicator ? sizeof( seg_len ) + char_size
			    : (descriptor->ds_nullable ? 1 : 0);
    }

    if ( msgBuff->length < length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ("cnvtBLOB2DataValue: need segment indicator for column %d\n",
	     (II_LONG)stmtHndl->sh_colIndex);
	return( FALSE );
    }

    /*
    ** Remove the indicator from the input buffer.
    */
    msgBuff->data += sizeof( indicator );
    msgBuff->length -= sizeof( indicator );

    /*
    ** If the indicator is 0, this is the end of the
    ** data segments and the BLOB.  Otherwise a
    ** segment follows.
    */
    if ( ! indicator )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "cnvtBLOB2DataValue: found end-of-segments indicator\n" );

	/*
	** During purging we ignore the NULL byte since
	** the data value was initialized when purging
	** started.
	*/
	if ( ! (stmtHndl->sh_flags & IIAPI_SH_PURGE_SEGMENTS) )
	    dataValue->dv_null = 
		descriptor->ds_nullable 
		    ? IIAPI_ISNULL(-descriptor->ds_dataType, 1, msgBuff->data)
		    : FALSE;

	/*
	** Remove NULL byte from input buffer and
	** terminate BLOB segment processing.
	*/
	if ( descriptor->ds_nullable )
	{
	    msgBuff->data++;
	    msgBuff->length--;
	}

	stmtHndl->sh_flags &= ~(IIAPI_SH_MORE_SEGMENTS |
				IIAPI_SH_PURGE_SEGMENTS);
    }
    else
    {
	/*
	** Read the segment length.  Limit the length
	** to what is available in the input buffer.
	*/
	MECOPY_CONST_MACRO( msgBuff->data, sizeof( seg_len ), (PTR)&seg_len );
	msgBuff->data += sizeof( seg_len );
	msgBuff->length -= sizeof( seg_len );
	length = min( seg_len, msgBuff->length / char_size ) * char_size;

	if ( stmtHndl->sh_flags & IIAPI_SH_PURGE_SEGMENTS )
	{
	    /*
	    ** Remove the available portion of
	    ** the segment from the input buffer.
	    */
	    msgBuff->data += length;
	    msgBuff->length -= length;
	    stmtHndl->sh_flags |= IIAPI_SH_LOST_SEGMENTS;

	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtBLOB2DataValue: discarding segment of %d bytes\n",
		  length );
	}
	else
	{
	    u_i2 new_len;

	    /*
	    ** We further limit the segment length to be
	    ** processed by the space available in the
	    ** output buffer.
	    */
	    length = min(length, descriptor->ds_length - dataValue->dv_length);
	    length -= length % char_size;	/* Whole characters only */

	    /*
	    ** Append input segment data to output segment.
	    */
	    MEcopy( msgBuff->data, length, 
		    (PTR)((char *)dataValue->dv_value + dataValue->dv_length) );
	    msgBuff->data += length;
	    msgBuff->length -= length;
	    dataValue->dv_length += length;

	    /*
	    ** Update the output embedded segment length.
	    */
	    new_len = (dataValue->dv_length - sizeof( new_len )) / char_size;
	    MECOPY_CONST_MACRO( (PTR)&new_len, 
				sizeof( new_len ), dataValue->dv_value );

	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtBLOB2DataValue: processed segment of %d bytes\n",
		  length );
	}

	/*
	** If the entire input segment was not consumed,
	** patch what remains to be a valid BLOB segment.
	** Create a segment indicator and segment length
	** to represent the remaining data.  There is 
	** room in the input buffer for this info to be 
	** pre-pended to what remains since the same 
	** info was removed from the input buffer above.
	*/
	if ( (length /= char_size) != seg_len )
	{
	    seg_len -= length;				/* Segment length */
	    msgBuff->data -= sizeof( seg_len );
	    msgBuff->length += sizeof( seg_len );
	    MECOPY_CONST_MACRO((PTR)&seg_len, sizeof(seg_len), msgBuff->data);

	    msgBuff->data -= sizeof( indicator );	/* Indicator */
	    msgBuff->length += sizeof( indicator );
	    MECOPY_CONST_MACRO( (PTR)&indicator, 
				sizeof( indicator ), msgBuff->data );

	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "cnvtBLOB2DataValue: split segment, %d bytes remain\n",
		  (II_LONG)seg_len );
	}
    }

    return( TRUE );
}

