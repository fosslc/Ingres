/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apiehndl.h>
# include <apishndl.h>
# include <apimisc.h>

/*
** Name: apivalid.c
**
** Description:
**	This file defines the parameter validation functions.
**
**      IIapi_validParmCount	Is param count of IIapi_putParms() valid?
**      IIapi_validPColCount	Is column count of IIapi_putColumn() valid?
**      IIapi_validGColCount	Is column count of IIapi_getColumn() valid?
**	IIapi_validDescriptor	Is descr info of IIapi_setDescriptor() valid?
**
** History:
**      30-sep-93 (ctham)
**          creation
**	10-Nov-94 (gordy)
**	    Multiple rows only allowed when fetching all columns.
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.  Added IIapi_validDescriptor().
**	16-Feb-95 (gordy)
**	    API can generate unique IDs, so some required
**	    parameters are now optional.  Changed parameters
**	    for cursor update/delete.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Lifted most restrictions on BLOBs for IIapi_validGColCount().
**	20-Jun-95 (gordy)
**	    Lifted most restrictions on BLOBs for IIapi_validParmCount().
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	 2-Oct-96 (gordy)
**	    Renamed event stuff to distiguish between Database Events
**	    and API events.
**      23-May-00 (loera01) Bug 101463
**          Added support for passing global temporary tables argument
**          into a DB procedure (GCA1_INVPROC message).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Sep-02 (gordy)
**	    Validate descriptor information against protocol level.
**	15-Mar-04 (gordy)
**	    Validate integer/float sizes.  Eight-byte ints supported
**	    at GCA_PROTOCOL_LEVEL_65.
**	31-May-06 (gordy)
**	    ANSI date/time types supported at GCA_PROTOCOL_LEVEL_66.
**	25-Oct-06 (gordy)
**	    Blob/Clob locator types supported at GCA_PROTOCOL_LEVEL_67.
**	 5-Mar-09 (gordy)
**	    Added IIapi_validDataValue().
**	26-Oct-09 (gordy)
**	    Boolean type supported at GCA_PROTOCOL_LEVEL_68.
**      17-Aug-2010 (thich01)
**          Spatial types supported at GCA_PROTOCOL_LEVEL_69.  Make changes to
**          treat spatial types like LBYTEs or NBR type as BYTE.
*/





/*
** Name: IIapi_validParmCount
**
** Description:
**	This function validates the input of IIapi_putParms() regarding
**	to its rules on BLOB and non-BLOB handling.  
**
** Input:
**	stmtHandl	Statement handle
**	putParmParm	Input parameters of IIapi_putParms().
**
** Output:
**	None.
**
** Return value:
**	II_BOOL		TRUE if the input is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	20-Jun-95 (gordy)
**	    Most restrictions on BLOBs eliminated.
*/

II_EXTERN II_BOOL
IIapi_validParmCount( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    II_BOOL	valid;

    /*
    ** Any non-empty subset is valid as long as all the service
    ** parameters are included in the first set.  There are no 
    ** restrictions on single segment BLOBs.  For multi-segment 
    ** BLOBs (the more Segments flag is set), the BLOB must be 
    ** the last parameter in the current group.  Note: we cannot 
    ** detect the case where two BLOBs are in the current group 
    ** and pp_moreSegments applies to the first.  In this case 
    ** we will send the first as a single segment BLOB and assume
    ** that the second has additional segments.
    */
    if ( putParmParm->pp_parmCount < 1  ||
	 (stmtHndl->sh_parmIndex + putParmParm->pp_parmCount)
				 > stmtHndl->sh_parmCount )
    {
	valid = FALSE;		/* Not a proper non-empty subset */
    }
    else  if ( putParmParm->pp_moreSegments  &&
	       ! IIapi_isBLOB( stmtHndl->sh_parmDescriptor[
			       stmtHndl->sh_parmIndex + 
			       putParmParm->pp_parmCount - 1 ].ds_dataType ) )
    {
	valid = FALSE;		/* Must be sending BLOB to set moreSegments */
    }
    else  if ( stmtHndl->sh_parmIndex )
	valid = TRUE;		/* Proper subset (not first) */
    else
    {
	i4 count;

	/*
	** Count the number of service parms expected.
	** Service parms, if present, are contiguous at
	** start of parameters.
	*/
	for( count = 0; 
	     count < stmtHndl->sh_parmCount  &&
	     stmtHndl->sh_parmDescriptor[ count ].ds_columnType == 
							IIAPI_COL_SVCPARM;
	     count++ );

	/*
	** Make sure we have all the service parms.
	*/
	valid = (count <= putParmParm->pp_parmCount) ? TRUE : FALSE;
    }

    return( valid );
}




/*
** Name: IIapi_validPColCount
**
** Description:
**	This function validates the input of IIapi_putColumns() regarding
**     to its rules on BLOB and non-BLOB handling.
**
**     stmtHandl   statement handle
**     putColParm  input parameters of IIapi_putColumns().
**
** Return value:
**     status  TRUE if the input column count is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
*/

II_EXTERN II_BOOL
IIapi_validPColCount( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTCOLPARM *putColParm )
{
    II_BOOL	valid = TRUE;

    /*
    ** Any non-empty subset is valid.  There are no 
    ** restrictions on single segment BLOBs.  For 
    ** multi-segment BLOBs (the more Segments flag 
    ** is set), the BLOB must be the last column in 
    ** the current group.  Note: we cannot detect 
    ** the case where two BLOBs are in the current 
    ** group and pc_moreSegments applies to the first.  
    ** In this case we will send the first as a single 
    ** segment BLOB and assume that the second has 
    ** additional segments.
    */
    if ( putColParm->pc_columnCount < 1  ||  
	 (stmtHndl->sh_colIndex + putColParm->pc_columnCount) 
				 > stmtHndl->sh_colCount )
    {
	valid = FALSE;		/* Not a proper non-empty subset */
    }
    else  if ( putColParm->pc_moreSegments  &&
	       ! IIapi_isBLOB( stmtHndl->sh_colDescriptor[
			       stmtHndl->sh_colIndex + 
			       putColParm->pc_columnCount - 1 ].ds_dataType ) )
    {
	valid = FALSE;		/* Must be sending BLOB to set moreSegments */
    }
    
    return( valid );
}


/*
** Name: IIapi_validGColCount
**
** Description:
**	This function validates the input of IIapi_getColumns().
**
**	1) Database Events or multi-row fetches must fetch all columns.
**
**	2) Single row fetches may not span rows but any non-empty subset 
**	   of the columns (in order) may be requested.
**
**	Some Database Event restrictions are passed through as valid
**	so that the proper error may be raised in the event system.
**	Most BLOB restrictions have been lifted with the condition
**	that data segments may be lost (with an appropriate warning).
**
** Input:
**	stmtHandl	Statement or event handle
**	getColParm	Input parameters of IIapi_getColumns().
**
** Output:
**	None.
**
** Return value:
**     II_BOOL		TRUE if column count is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	10-Nov-94 (gordy)
**	    Multiple rows only allowed when fetching all columns.
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	25-Apr-95 (gordy)
**	    Must also validate event handles.
**	13-Jun-95 (gordy)
**	    Most restrictions on BLOBs eliminated.
**	20-Jun-95 (gordy)
**	    Make sure subset is not empty.
*/

II_EXTERN II_BOOL
IIapi_validGColCount( IIAPI_STMTHNDL *stmtHndl, IIAPI_GETCOLPARM *getColParm )
{
    II_BOOL		valid;

    if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)stmtHndl ) )
    {
	IIAPI_DBEVHNDL	*dbevHndl = (IIAPI_DBEVHNDL *)stmtHndl;
	IIAPI_DBEVCB	*dbevCB;

	if ( IIapi_isConnHndl( (IIAPI_CONNHNDL *)dbevHndl->eh_parent ) )
	{
	    /*
	    ** This isn't really OK, but the event state
	    ** machine will detect the sequencing error
	    ** and generate the correct message.  If we
	    ** return FALSE here, a misleading error will
	    ** be generated.
	    */
	    valid = TRUE;
	}
	else
	{
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;

	    /*
	    ** If there is no data, return TRUE so that the
	    ** event state machine can return the NO_DATA
	    ** status.  Otherwise, must be retrieving all
	    ** columns (assumes no BLOBs in event data).
	    ** We don't enforce a single row fetch, but
	    ** the event state machine will only return
	    ** one row.
	    */
	    if ( ! dbevCB->ev_count )
		valid = TRUE;
	    else
		valid = (getColParm->gc_columnCount == dbevCB->ev_count);
	}
    }
    else  if ( getColParm->gc_rowCount > 1 )
    {
	/*
	** When fetching more than one row, must fetch all columns.
	** Watch out for a multi-row request when we are not at the
	** start of a row.
	*/
	valid = ( ! stmtHndl->sh_colIndex  &&
		  getColParm->gc_columnCount == stmtHndl->sh_colCount );
    }
    else
    {
	/*
	** We allow the application to fetch any non-empty subset 
	** of the columns.  Our only restriction is that we do not 
	** span across rows.  If not fetching the entire row, we 
	** will buffer the remainder for subsequent requests.  To 
	** properly fetch a BLOB, it should be the last (or only) 
	** column being fetched.  If this guideline is not
	** followed, we can still operate by discarding all but
	** the first segement of the BLOB (generate a warning)
	** and continue with the rest of the row.
	*/
	valid = ( getColParm->gc_columnCount >= 1  &&
		  (stmtHndl->sh_colIndex + getColParm->gc_columnCount) 
					<= stmtHndl->sh_colCount );
    }

    return( valid );
}


/*
** Name: IIapi_validDescriptor
**
** Description:
**	Validates the input of IIapi_setDescriptor() to ensure that
**	the required parameters of the current query are present.
**
**	stmtHandl	Statement handle
**
** Return value:
**	II_ULONG	Error code, OK if no error.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	11-Jan-95 (gordy)
**	    Created.
**	16-Feb-95 (gordy)
**	    API can generate unique IDs, so some required
**	    parameters are now optional.  Changed parameters
**	    for cursor update/delete.
**	19-Sep-02 (gordy)
**	    Validate descriptor information against protocol level.
**	15-Mar-04 (gordy)
**	    Validate integer/float sizes.  Eight-byte ints supported
**	    at GCA_PROTOCOL_LEVEL_65.
**	31-May-06 (gordy)
**	    ANSI date/time types supported at GCA_PROTOCOL_LEVEL_66.
**	28-Jun-06 (gordy)
**	    OUT parameters backward supported as BYREF.
**	25-Oct-06 (gordy)
**	    Blob/Clob locator types supported at GCA_PROTOCOL_LEVEL_67.
**	26-Oct-09 (gordy)
**	    Boolean type supported at GCA_PROTOCOL_LEVEL_68.
*/

II_EXTERN II_ULONG
IIapi_validDescriptor( IIAPI_STMTHNDL *stmtHndl, 
		       II_LONG descCount, IIAPI_DESCRIPTOR *descriptor )
{
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl((IIAPI_HNDL *)stmtHndl);
    II_LONG		i, svcparms = 0;

    /*
    ** Must have at least 1 descriptor.
    */
    if ( descCount < 1  ||  ! descriptor )
	return( E_AP0010_INVALID_COLUMN_COUNT );

    switch( stmtHndl->sh_queryType )
    {
	case IIAPI_QT_OPEN :
	    /*
	    ** 1 optional parameter: cursor name.
	    */
	    if ( descriptor[ 0 ].ds_columnType == IIAPI_COL_SVCPARM  &&
		 descriptor[ 0 ].ds_dataType != IIAPI_CHA_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );

	    svcparms = 1;
	    break;

	case IIAPI_QT_CURSOR_DELETE :
	    /*
	    ** 1 required parameters: cursor ID.
	    ** No other parameters allowed.
	    */
	    if ( descCount != 1 )
		return( E_AP0010_INVALID_COLUMN_COUNT );
	    else  if ( descriptor[ 0 ].ds_columnType != IIAPI_COL_SVCPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( descriptor[ 0 ].ds_dataType != IIAPI_HNDL_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );

	    svcparms = 1;
	    break;

	case IIAPI_QT_CURSOR_UPDATE :
	    /*
	    ** 1 required parameter: cursor ID.
	    */
	    if ( descriptor[ 0 ].ds_columnType != IIAPI_COL_SVCPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( descriptor[ 0 ].ds_dataType != IIAPI_HNDL_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );

	    svcparms = 1;
	    break;

	case IIAPI_QT_EXEC_PROCEDURE :
	    /*
	    ** 1 required parameter: procedure name or ID.
	    ** 1 optional parameter: procedure owner.
	    */
	    if ( descriptor[ 0 ].ds_columnType != IIAPI_COL_SVCPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( descriptor[ 0 ].ds_dataType != IIAPI_CHA_TYPE  &&
		       descriptor[ 0 ].ds_dataType != IIAPI_HNDL_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );
	    else
		svcparms = 1;

	    if ( descCount > 1  &&  
		 descriptor[ 1 ].ds_columnType == IIAPI_COL_SVCPARM )
	    {
		if ( descriptor[ 0 ].ds_dataType == IIAPI_HNDL_TYPE  ||
		     descriptor[ 1 ].ds_dataType != IIAPI_CHA_TYPE )
		    return( E_AP0025_SVC_DATA_TYPE );

		svcparms = 2;
	    }
	    break;

	case IIAPI_QT_DEF_REPEAT_QUERY :
	    /*
	    ** 3 optional parameters: query ID of 2 integers and a name.
	    ** Must all be present or none.
	    */
	    if ( descriptor[ 0 ].ds_columnType != IIAPI_COL_SVCPARM )
		svcparms = 0;
	    else  if ( descCount < 3 )
		return( E_AP0010_INVALID_COLUMN_COUNT );
	    else  if ( descriptor[ 1 ].ds_columnType != IIAPI_COL_SVCPARM  ||
		       descriptor[ 2 ].ds_columnType != IIAPI_COL_SVCPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( descriptor[ 0 ].ds_dataType != IIAPI_INT_TYPE  ||
		       descriptor[ 1 ].ds_dataType != IIAPI_INT_TYPE  ||
		       descriptor[ 2 ].ds_dataType != IIAPI_CHA_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );
	    else
		svcparms = 3;
	    break;

	case IIAPI_QT_EXEC_REPEAT_QUERY :
	    /*
	    ** 1 required parameter: repeat query ID.
	    */
	    if ( descriptor[ 0 ].ds_columnType != IIAPI_COL_SVCPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( descriptor[ 0 ].ds_dataType != IIAPI_HNDL_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );

	    svcparms = 1;
	    break;

	default :
	    /*
	    ** 0 required parameters.
	    ** Optional parameters verified below.
	    */
	    svcparms = 0;
	    break;
    }

    /*
    ** Check remaining parameters.
    */
    for( i = svcparms; i < descCount; i++ )
    {
	/*
	** Remaining parameters should be procedure parameters
	** or regular query parameters.
	*/
	if ( stmtHndl->sh_queryType == IIAPI_QT_EXEC_PROCEDURE )
	    switch( descriptor[ i ].ds_columnType )
	    {
	    case IIAPI_COL_PROCINPARM :  break;

	    case IIAPI_COL_PROCOUTPARM :
	    case IIAPI_COL_PROCINOUTPARM :
		if ( connHndl  &&
		     connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_60 )
		    return( E_AP0020_BYREF_UNSUPPORTED );
		break;
		
	    case IIAPI_COL_PROCGTTPARM :
		if ( connHndl  &&
		     connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_62 )
		    return( E_AP0021_GTT_UNSUPPORTED );
		break;
		
	    default :  return( E_AP0012_INVALID_DESCR_INFO );
	    }
        else  if ( descriptor[ i ].ds_columnType != IIAPI_COL_QPARM )
	    return( E_AP0012_INVALID_DESCR_INFO );

	switch( descriptor[ i ].ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( descriptor[ i ].ds_length )
	    {
	    case 1 :
	    case 2 :
	    case 4 :
		break;
	    
	    case 8 :
		if ( connHndl  &&  
		     connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_65 )
		    return( E_AP002A_LVL3_DATA_TYPE );
		break;

	    default :
		return( E_AP0024_INVALID_DATA_SIZE );
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( descriptor[ i ].ds_length )
	    {
	    case 4 :
	    case 8 :
		break;
	    
	    default :
		return( E_AP0024_INVALID_DATA_SIZE );
	    }
	    break;

	case IIAPI_BYTE_TYPE :
	case IIAPI_VBYTE_TYPE :
	case IIAPI_LBYTE_TYPE :
	case IIAPI_LVCH_TYPE :
	case IIAPI_DEC_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_60 )
		return( E_AP0028_LVL1_DATA_TYPE );
	    break;
	
	case IIAPI_NCHA_TYPE :
	case IIAPI_NVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_64 )
		return( E_AP0029_LVL2_DATA_TYPE );
	    break;

	case IIAPI_DATE_TYPE :
	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	case IIAPI_TMTZ_TYPE :
	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	case IIAPI_TSTZ_TYPE :
	case IIAPI_INTYM_TYPE :
	case IIAPI_INTDS_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_66 )
		return( E_AP002B_LVL4_DATA_TYPE );
	    break;

	case IIAPI_LCLOC_TYPE :
	case IIAPI_LBLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_67 )
		return( E_AP002C_LVL5_DATA_TYPE );
	    else  if (  descriptor[ i ].ds_length != IIAPI_LOCATOR_LEN )
		return( E_AP0024_INVALID_DATA_SIZE );
	    break;

	case IIAPI_BOOL_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_68 )
		return( E_AP002D_LVL6_DATA_TYPE );
	    else  if (  descriptor[ i ].ds_length != IIAPI_BOOL_LEN )
		return( E_AP0024_INVALID_DATA_SIZE );
	    break;

	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_NBR_TYPE :
	case IIAPI_GEOMC_TYPE :
	    if ( connHndl  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_69 )
		return( E_AP002E_LVL7_DATA_TYPE );
	    break;

	}
    }

    return( OK );
}


/*
** Name: IIapi_validDataValue
**
** Description:
**	Validate a data value array based on the meta-data
**	provided in the descriptor array.
**
** Input:
**	count		Number of descriptors/data values.
**	desc		Descriptor array.
**	dv		Data value array.
**
** Output:
**	None.
**
** Return value:
**	II_ULONG	Error code.  OK if no error.
**
** History:
**	 5-Mar-09 (gordy)
**	    Created.
*/

II_EXTERN II_ULONG
IIapi_validDataValue( II_LONG count, 
		      IIAPI_DESCRIPTOR *desc, IIAPI_DATAVALUE *dv )
{
    for( ; count > 0; count--, desc++, dv++ )
    {
	if ( dv->dv_null )
	    if ( desc->ds_nullable )
		continue;	/* Nullable value is null */
	    else
		return( E_AP0023_INVALID_NULL_DATA );

    	switch( desc->ds_dataType )
	{
	case IIAPI_BYTE_TYPE :
	case IIAPI_NBR_TYPE :
	case IIAPI_CHR_TYPE :
	case IIAPI_CHA_TYPE :
	case IIAPI_NCHA_TYPE :
	    /*
	    ** DV length will be truncated or padded to DESC length.
	    */
	    break;

	case IIAPI_LBYTE_TYPE :
	case IIAPI_LVCH_TYPE :
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
	    ** DV must contain the embeded segment length indicator.
	    */
	    if ( dv->dv_length < sizeof( i2 ) )
	    	return( E_AP0024_INVALID_DATA_SIZE );
	    break;

	case IIAPI_VBYTE_TYPE :
	case IIAPI_VCH_TYPE :
	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	case IIAPI_NVCH_TYPE :
	    /*
	    ** DV must contain the embeded variable length indicator.
	    */
	    if ( dv->dv_length < sizeof( i2 ) )
	    	return( E_AP0024_INVALID_DATA_SIZE );
	    break;

	default :
	    /*
	    ** DV length must match DESC length for fixed length types.
	    */
	    if ( dv->dv_length != desc->ds_length )
	    	return( E_AP0024_INVALID_DATA_SIZE );
	    break;
	}
    }

    return( OK );
}

