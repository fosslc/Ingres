/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <lo.h>
# include <tm.h>
# include <si.h>
# include <st.h>

# include <rfapi.h>
# include "rfapilocal.h"

/*
** Name: rfapiutil.c
**
** Description:
** 
**	Unpublished utility modules used by Response File C API.
**
** History:
**
**    19-Sep-2006 (hanje04)
**	SIR 116907
**	Created.
**	13-Nov-2006 (hanje04)
**	    SIR 116907
**	    Add section headers to make response file more user readable.
**	    Format used is Windows .ini file and lines are "commented" on 
**	    Linux and UNIX.
**	20-May-2010 (hanje04)
**	    SIR 123791
**	    Write VW config to response file if set.
**	    (See rfapidata.c for full description)
**	12-Jul-2010 (hanje04)
**	    BUG 124081
**	    Add APPEND_HEADER if we're appending
**	    Make parameter writing more intelligent and check for values
**	    in each "section" before writing the header, excluding the lot
**	    if none are set.
**	    Replace individual option arrays with rfapiopsinfo[] which
**	    contains everything except camdb_ops[] which has been kept 
**	    separate as it's a special, rarely used case.
** 
*/

/*
** global refs for variable arrays
*/
GLOBALREF RFAPI_VAR *camdb_ops[]; /* CA MDB Options */

/*
** Function:
**
**       rfapi_getDateTime
**
** Description:
**
**      Get date and time in specific format:
**	
**		e.g. Thu Sep 28 13:14:24 2006
**
** Inputs:
**
**      None
**
** Outputs:
**
**      char	*dtbuf -	String containing current date and time in
**				required  format
**
** Returns:
**
**      None
*/

void
rfapi_getDateTime( char *dtbuf )
{
    SYSTIME	tm; /* system time in seconds */
    struct TMhuman	dthuman; /* human readable date and time structure */

    /* get the system time */
    TMnow( &tm );

    /* break it up into human readable format */
    TMbreak( &tm, &dthuman );

    STprintf( dtbuf, RFAPI_DATE_FORMAT,
		dthuman.wday,
		dthuman.month,
		dthuman.day,
		dthuman.hour,
		dthuman.mins,
		dthuman.sec,
		dthuman.year );
} 

/*
** Function:
**
**       rfapi_writeRFHeader
**
** Description:
**
**      Write response file header to open file descriptor
**
** Inputs:
**
**      II_RFAPI_HANDLE	*handle	 - Pointer to intialized respone file handle
**	FILE	*fd	- Pointer to an open response file
**
** Outputs:
**
**      None
**
** Returns:
**
**      II_RFAPI_STATUS:
**          II_RF_ST_OK on success
**
**      on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_FAIL
**
** History:
**	02-Jul-2009 (hanje04)
**	    Bug 122227
**	    SD 137277
**	    Increase size of datebuf to prevent buffer overrun in
**	    when calling rfapi_GetDateTime() :-(
**	12-Jul-2010 (hanje04)
**	    BUG 124081
**	    Add APPEND_HEADER if we're appending
*/
II_RFAPI_STATUS
rfapi_writeRFHeader( II_RFAPI_HANDLE *handle,
			FILE *fd )
{
    char 	date_buf[50]; /* buffer date and time string string */

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    if ( SIisopen( fd ) != TRUE )
	return( II_RF_ST_FAIL );

    /* get time info */
    rfapi_getDateTime( date_buf );

    /* write out header  */
    if ( (*handle)->flags & II_RF_OP_APPEND )
	SIfprintf( fd, RFAPI_RESPONSE_FILE_APPEND_HEADER, date_buf );
    else
	SIfprintf( fd, RFAPI_RESPONSE_FILE_HEADER, date_buf );

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	rfapi_writeSecHeader
**
** Description:
**
**	Write given section header to open response file.
**
** Inputs:
**
**      II_RFAPI_HANDLE	*handle	 - Pointer to intialized respone file handle
**	II_RFAPI_STRING header - Section header to be written
**	FILE	*fd	- Pointer to an open response file
**
** Outputs:
**
**      None
**
** Returns:
**
**      II_RFAPI_STATUS:
**          II_RF_ST_OK on success
**
**      on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
rfapi_writeSecHeader( II_RFAPI_HANDLE *handle,
			II_RFAPI_STRING header,
			FILE *fd )
{
    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* check for valid header */
    if ( header == NULL || *header == '\0' )
	return( II_RF_ST_NULL_ARG );

    /* check response file is open */
    if ( SIisopen( fd ) != TRUE )
	return( II_RF_ST_FAIL );

    /* write the header */
    SIfprintf( fd, RFAPI_SECTION_HEADER, header );

    return( II_RF_ST_OK ) ;

}

/*
** Function:
**
**       rfapi_writeRFParams
**
** Description:
**
**      Write parameters to open response file. Wrapper function for 
**	rfapi_doWrite(). Scrolls through each of the parameter list
**	writing out what it needs to
**
** Inputs:
**
**      II_RFAPI_HANDLE	*handle	 - Pointer to intialized respone file handle
**	FILE	*fd	- Pointer to an open response file
**
** Outputs:
**
**      None
**
** Returns:
**
**      II_RFAPI_STATUS:
**          II_RF_ST_OK on success
**
**      on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_EMPTY_HANDLE
**	    ...
**	    II_RF_ST_FAIL
** History:
**	Sept-2006
**	    Created.
**	12-Jul-2010 (hanje04)
**	    BUG 124081
**	    Make parameter writing more intelligent and check for values
**	    in each "section" before writing the header, excluding the lot
**	    if none are set.
**	    Replace individual option arrays with rfapiopsinfo[] which
**	    contains everything except camdb_ops[] which has been kept 
**	    separate as it's a special, rarely used case.
*/
II_RFAPI_STATUS
rfapi_writeRFParams( II_RFAPI_HANDLE *handle,
			FILE *fd )
{
    II_RFAPI_STATUS	rfrc;
    i4	i = 0;
    i4	j = 0;
    bool foundops = FALSE;

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( (*handle)->param_list == NULL )
	return( II_RF_ST_EMPTY_HANDLE );

    /* Search the parameter lists and write out those set in the handle */
    while( rfapiopsinfo[i].ops )
    {
	foundops = FALSE;
	for( j = 0 ; rfapiopsinfo[i].ops[j] ; j++ )
	    if ( rfapi_findParam( handle, rfapiopsinfo[i].ops[j]->pname ) )
	    {
		foundops = TRUE;
		break;
	    }

	if ( foundops || (*handle)->flags & II_RF_OP_WRITE_DEFAULTS )
	{
	    rfrc = rfapi_writeSecHeader( handle, rfapiopsinfo[i].ophdr, fd );
	    if ( rfrc != II_RF_ST_OK )
		return( rfrc );

	    rfrc = rfapi_doWrite( handle, rfapiopsinfo[i].ops, fd );
	    if ( rfrc != II_RF_ST_OK )
		return( rfrc );

	}
	i++;
    }

    /* CA Options if requested */
    if ( (*handle)->flags & II_RF_OP_INCLUDE_MDB )
    {
	rfrc = rfapi_writeSecHeader( handle, RFAPI_SEC_MDB_OPTIONS, fd );
	if ( rfrc != II_RF_ST_OK )
	    return( rfrc );

	rfrc = rfapi_doWrite( handle, camdb_ops, fd );
	if ( rfrc != II_RF_ST_OK )
	    return( rfrc );
    }


    return( II_RF_ST_OK );
}

/*
** Function:
**
**       rfapi_findParam
**
** Description:
**
**      Find specified parameter in list
**
** Inputs:
**
**      II_RFAPI_HANDLE	*handle	 - Pointer to intialized respone file handle
**	II_RFAPI_PNAME	pname	- Name of parameter to find
**
** Outputs:
**
**      None
**
** Returns:
**
**      RFAPI_PARAMS * on success
**
**      NULL on failure:
*/
RFAPI_PARAMS *
rfapi_findParam	( II_RFAPI_HANDLE *handle,
		II_RFAPI_PNAME pname )
{
    RFAPI_PARAMS	*params_ptr;

    /* check handle is valid */
    if ( (*handle) == NULL || ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( NULL );


    /* scan param list looking for pname */
    params_ptr = (*handle)->param_list;
    while ( params_ptr != NULL && params_ptr->param.name != pname )
	params_ptr = params_ptr->next;

    /* return param_ptr */
    return( params_ptr );

}

/*
** Function:
**
**       rfapi_doWrite
**
** Description:
**
**      Write given parameter list to open response file
**
** Inputs:
**
**      II_RFAPI_HANDLE	*handle	 - Pointer to intialized respone file handle
**	RFAPI_VAR	**var_list - List of response file variable to write
**	FILE	*fd	- Pointer to an open response file
**
** Outputs:
**
**      None
**
** Returns:
**
**      II_RFAPI_STATUS:
**          II_RF_ST_OK on success
**
**      on failure:
**	    II_RF_ST_IO_ERROR
**	    ...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
rfapi_doWrite( II_RFAPI_HANDLE *handle,
		RFAPI_VAR **var_list, 
		FILE    *fd )
{
    RFAPI_PARAMS *params_ptr;
    i4	i=0;

    if ( SIisopen( fd ) != TRUE )
	return( II_RF_ST_IO_ERROR );

    /*
    ** scroll through the list of parameters in order and write out 
    ** the names and values
    */
    while ( var_list[i]  != NULL )
    {

	/* only print parameters valid for specified output format */
	if ( (*handle)->output_format & var_list[i]->valid_on )
	{
	    if ( params_ptr = rfapi_findParam( handle, var_list[i]->pname ) )
		SIfprintf( fd, "%s=%s\n",
			var_list[i]->vname, 
			(params_ptr)->param.value );
	    else if ( (*handle)->flags & II_RF_OP_WRITE_DEFAULTS &&
			(*handle)->output_format & var_list[i]->default_on ) 
		SIfprintf( fd, "%s=%s\n",
			var_list[i]->vname, 
			(*handle)->output_format & II_RF_DP_LINUX ?
			    var_list[i]->default_lnx :
			    var_list[i]->default_win );
	}
	i++;
    }

   return( II_RF_ST_OK );
}
