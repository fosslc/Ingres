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
** Name: rfapi.c
**
** Description:
** 
**	Main module for the Response File C API
**
** History:
**
**	05-Sep-2006 (hanje04)
**	    SIR 116907
**	    Created.
**	13-Jul-2010 (hanje04)
**	    BUG 124081
**	    Add option II_RF_OP_APPEND to allow appending to existing response
**	    file rather than generating a new one
** 
*/

GLOBALREF const II_RFAPI_STRING rfapierrs[];

/*
** Function:
**
**	 IIrfapi_initNew
**
** Description:
**
**	Initialize new handler for generating response file.
**
** Inputs:
**
**	II_RFAPI_STRING response_file - Full path to response file to create
**	II_RFAPI_PLATFORM format - Output format for response file (UNIX or
**				   Windows) 
**	II_RFAPI_OPTIONS flags	- Response file options mask
**
** Outputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_FILE_EXISTS
**	    II_RF_ST_INVALID_FORMAT
**	    II_RF_ST_MEM_ERROR
**	    II_RF_ST_CREATE_FAIL
**	    II_RF_ST_FILE_NOT_FOUND
**	...
**	    II_RF_ST_FAIL
*/

II_RFAPI_STATUS
IIrfapi_initNew		( II_RFAPI_HANDLE *handle,
			II_RFAPI_PLATFORMS format,
			II_RFAPI_STRING response_file,
			II_RFAPI_OPTIONS flags )
{
    LOCATION	rfloc; /* response file LO location */
    u_i2	memtag; /* ME tag for memory requests */
    STATUS	retval;

    /* check we haven't been pased nowt  */
    if ( response_file == NULL )
	return( II_RF_ST_NULL_ARG );

    /* check we've been passed one platform only */
    switch( format & II_RF_DP_ALL  )
    { 
	case II_RF_DP_WINDOWS:
		break;
	case II_RF_DP_LINUX:
		break;
	default:
		return( II_RF_ST_INVALID_FORMAT );
    }

    /* convert string to LO LOCATION */
    LOfroms( PATH & FILENAME, response_file, &rfloc );

    /*
    ** check file doesn't already exist if we don't want it but 
    ** is there if we do
    */
    if ( flags & II_RF_OP_APPEND )
    {
        if ( LOexist( &rfloc ) != OK )
	    return( II_RF_ST_FILE_NOT_FOUND );
    }
    else
        if ( LOexist( &rfloc ) == OK )
	    return( II_RF_ST_FILE_EXISTS );

    /* 
    ** create tag and allocate memory. Store the tag as we'll 
    ** use it later for all memory requests associated with this list
    */
    memtag = MEgettag();
    if ( memtag == 0 )
	return( II_RF_ST_MEM_ERROR );

    *handle = (II_RFAPI_HANDLE)MEreqmem( memtag,
					sizeof( RFAPI_LIST ),
					TRUE, NULL );
    if ( *handle == NULL )
	return ( II_RF_ST_MEM_ERROR );

    (*handle)->memtag = memtag; 

    /* Copy in the location info */
    (*handle)->locbuf = MEreqmem( (*handle)->memtag,
				STlen(response_file),
				TRUE, NULL );
    LOcopy( &rfloc, (*handle)->locbuf, &(*handle)->response_file_loc );

    /* initialize rest of structure */
    (*handle)->output_format = II_RF_DP_NONE;
    (*handle)->param_list = NULL;
    (*handle)->flags = II_RF_OP_INITIALIZED;
    
    /* Set the user options */
    (*handle)->flags |= flags;
    (*handle)->output_format = format;

    /* create empty response file */
    if ( ~flags&II_RF_OP_APPEND && LOcreate( &rfloc ) != OK )
    {
	/* free up memory before returning */
	MEtfree( memtag );
	(*handle) == NULL;
	return( II_RF_ST_CREATE_FAIL );
    }

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_setOutFileLoc
**
** Description:
**
**	Set or change the output location for the response file
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**	II_RFAPI_STRING newfileloc - Full path to response file to create
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_FILE_EXISTS
**	    II_RF_ST_CREATE_FAIL
**	    II_RF_ST_FILE_NOT_FOUND
**	...
**	    II_RF_ST_FAIL
*/

II_RFAPI_STATUS
IIrfapi_setOutFileLoc( II_RFAPI_HANDLE	*handle,
			II_RFAPI_STRING newfileloc )
{
    LOCATION	rfloc; /* response file LO location */

    /* check handle is valid */
    if ( (*handle) == NULL )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* check we haven't been pased nowt  */
    if ( newfileloc == NULL )
	return( II_RF_ST_NULL_ARG );

    /* convert string to LO LOCATION */
    LOfroms( PATH & FILENAME, newfileloc, &rfloc );

    /* check the output file */
    if ( (*handle)->flags & II_RF_OP_APPEND )
    {
	if ( LOexist( &rfloc ) != OK )
	    return( II_RF_ST_FILE_NOT_FOUND );
    }
    else
	/* create new response file */
	if ( LOcreate( &rfloc ) != OK )
	    return( II_RF_ST_CREATE_FAIL );

    /* remove previous file name from handle */
    MEfree( (*handle)->locbuf );

    /* create new buffer */
    (*handle)->locbuf = MEreqmem( (*handle)->memtag,
				STlen(newfileloc),
				TRUE, NULL );

    if ( (*handle)->locbuf == NULL )
	return( II_RF_ST_MEM_ERROR );
 
    /* copy new location into place */
    LOcopy( &rfloc, (*handle)->locbuf, &(*handle)->response_file_loc );

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_setPlatform
**
** Description:
**
**	Set or change the output platform for the response file
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**	II_RFAPI_PLATFORM newformat - Output format
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_INVALID_FORMAT
**	...
**	    II_RF_ST_FAIL
*/

II_RFAPI_STATUS
IIrfapi_setPlatform( II_RFAPI_HANDLE	*handle,
			II_RFAPI_PLATFORMS newformat )
{
    /* check handle is valid */
    if ( (*handle) == NULL )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* check we've been passed one platform only */
    switch( newformat & II_RF_DP_ALL )
    { 
	case II_RF_DP_WINDOWS:
		break;
	case II_RF_DP_LINUX:
		break;
	default:
		return( II_RF_ST_INVALID_FORMAT );
    }

    /* set new platform */
    (*handle)->output_format = newformat;

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_addParam
**
** Description:
**
**	Add response file parameter entry
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**	II_RFAPI_PARAM *newformat - Pointer to parameter structure to add
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_INVALID_FORMAT
**	    II_RF_ST_MEM_ERROR
**	    II_RF_ST_PARAM_EXISTS
**	...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
IIrfapi_addParam( II_RFAPI_HANDLE *handle,
		II_RFAPI_PARAM *param )
{
    RFAPI_PARAMS	*new_param_entry;

    /* check handle is valid */
    if ( (*handle) == NULL )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* check for null parameter */
    if ( param == NULL )
	return( II_RF_ST_NULL_ARG ); 

    /* find end of param list */
    if ( (*handle)->param_list == NULL ) /* new list, no entries */
    {
        /* create first element in list */
	(*handle)->param_list = (RFAPI_PARAMS *)
				    MEreqmem( (*handle)->memtag,
				    sizeof(RFAPI_PARAMS),
				    TRUE,
				    NULL );
	new_param_entry = (*handle)->param_list;

	if ( new_param_entry == NULL )
	    return ( II_RF_ST_MEM_ERROR );
	
	/* initialize */
	new_param_entry->prev = NULL;
	new_param_entry->next = NULL;
    }
    else
    {
	RFAPI_PARAMS *end_of_list;

	/*
	** Check if parameter is already set. Duplicate entries are not 
	** permitted so return an error if we find one.
        */
        if ( rfapi_findParam( handle, param->name ) != NULL )
	    return( II_RF_ST_PARAM_EXISTS );

	/* find the end of the list */
	end_of_list = (*handle)->param_list;
	while ( end_of_list->next != NULL )
	    end_of_list = end_of_list->next;

	/* create new entry */
	end_of_list->next = (RFAPI_PARAMS *)MEreqmem( (*handle)->memtag,
				sizeof(RFAPI_PARAMS),
				TRUE, NULL );
	new_param_entry = end_of_list->next;

	if ( new_param_entry == NULL )
	    return ( II_RF_ST_MEM_ERROR );


	/* initialize */ 
	new_param_entry->prev = end_of_list;
	new_param_entry->next = NULL;
		
    }

    /* store the values */
    new_param_entry->param.name = param->name;
    new_param_entry->param.vlen = param->vlen;
    new_param_entry->param.value = MEreqmem ( (*handle)->memtag,
						param->vlen,
						TRUE, NULL );
    STncpy( new_param_entry->param.value,
	    param->value,
	    param->vlen );

    /* return OK */
    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_addParam
**
** Description:
**
**	Update value for existing response file parameter. If parameter is
**	not set and II_RF_OP_ADD_ON_UPDATE is set in the handle it will
**	be added.
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**	II_RFAPI_PARAM *new_param - Pointer to parameter structure to update
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_INVALID_FORMAT
**	    II_RF_ST_MEM_ERROR
**	    II_RF_ST_PARAM_NOT_FOUND
**	...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
IIrfapi_updateParam	( II_RFAPI_HANDLE *handle,
			II_RFAPI_PARAM *new_param )
{
    RFAPI_PARAMS	*param_ptr;

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* sanity check param */
    if ( new_param == NULL || new_param->value == NULL )
	return( II_RF_ST_FAIL );
    
    /* find the value to be updated */
    param_ptr = rfapi_findParam( handle, new_param->name );

    if ( param_ptr != NULL )
    {
	 /* we found what we were looking for */
	if ( param_ptr->param.vlen > new_param->vlen )
	{
	    /*
	    ** new string will fit in old slot so just
	    ** overwrite the existing value
	    */
	    STncpy( param_ptr->param.value,
		    new_param->value,
		    param_ptr->param.vlen );
	}
	else
	{
	    /* new value is longer */
	    /* remove entry from the list */
	    RFAPI_REMOVE_PARAM_MACRO(param_ptr, handle);

	    /* free the old memory */
	    MEfree( (PTR)param_ptr );

	    /* Add the new entry and return */
	    return( IIrfapi_addParam( handle, new_param ) );
	    
	}
    }
    else if ( (*handle)->flags & II_RF_OP_ADD_ON_UPDATE )
	/* param not in list but as we've been told to add it */
	return( IIrfapi_addParam( handle, new_param ) );
    else
	/* param not found, return error */
	return( II_RF_ST_PARAM_NOT_FOUND );

}

/*
** Function:
**
**	 IIrfapi_removeParam
**
** Description:
**
**	Remove response file parameter from list.
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**	II_RFAPI_PNAME pname - Name of parameter to remove
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_PARAM_NOT_FOUND
**	...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
IIrfapi_removeParam( II_RFAPI_HANDLE *handle,
			II_RFAPI_PNAME pname )
{

    RFAPI_PARAMS	*param_ptr;

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* find the value to be updated */
    param_ptr = rfapi_findParam( handle, pname );

    if ( param_ptr != NULL )
    {
	/* new value is longer */
	/* remove entry from the list */
	RFAPI_REMOVE_PARAM_MACRO(param_ptr, handle);

	/* free the old memory */
	MEfree( (PTR)param_ptr );
    }
    else
        return( II_RF_ST_PARAM_NOT_FOUND );

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_writeRespFile
**
** Description:
**
**	Write response file to disk
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_EMPTY_HANDLE
**	    II_RF_ST_FILE_NOT_FOUND
**	...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
IIrfapi_writeRespFile( II_RFAPI_HANDLE *handle )
{
    FILE	*resp_fd; /* file descriptor for response file */
    II_RFAPI_STATUS rfrc;
    char	*mode;

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( ! (*handle)->flags & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );
    else if ( (*handle)->param_list == NULL )
	return( II_RF_ST_EMPTY_HANDLE );

    /* check the response file exists */
    if ( LOexist( &(*handle)->response_file_loc ) != OK )
	return( II_RF_ST_FILE_NOT_FOUND );

    /* open file for writing */
    mode = ( (*handle)->flags & II_RF_OP_APPEND ) ? "a" : "w" ;
    SIopen( &(*handle)->response_file_loc, mode, &resp_fd );

    /* write header */
    if ( rfrc = rfapi_writeRFHeader( handle, resp_fd ) != II_RF_ST_OK )
    {
	SIclose( resp_fd );
	return( rfrc );
    }

    /* write package info */
    if ( rfrc = rfapi_writeRFParams( handle, resp_fd ) != II_RF_ST_OK )
    {
	SIclose( resp_fd );
	return( rfrc );
    }

    /* close newly written response file */
    SIclose( resp_fd );

    /* clean up if we need to */
    if ( (*handle)->flags & II_RF_OP_CLEANUP_AFTER_WRITE )
	IIrfapi_cleanup( handle );

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_cleanup
**
** Description:
**
**	Remove all parameters from list and free all memory associated
**	with handle
**
** Inputs:
**
**	II_RFAPI_HANDLE	*handle	- Handle to initialized response file
**				  structure.
**
** Outputs:
**
**	None
**
** Returns:
**
**	II_RFAPI_STATUS:
**	    II_RF_ST_OK on success
**
**	on failure:
**	    II_RF_ST_NULL_ARG
**	    II_RF_ST_MEM_ERROR
**	...
**	    II_RF_ST_FAIL
*/
II_RFAPI_STATUS
IIrfapi_cleanup( II_RFAPI_HANDLE *handle )
{
    u_i2 tag;

    /* check handle is valid */
    if ( (*handle) == NULL  )
	return( II_RF_ST_NULL_ARG );
    else if ( (*handle)->memtag & II_RF_OP_INITIALIZED )
	return( II_RF_ST_FAIL );

    /* save the tag so we can free it after freeing the memory */
    tag = (*handle)->memtag;

    /* free all memory associated with handle */
    if ( MEtfree( tag ) != OK )
	return( II_RF_ST_MEM_ERROR );

    handle=NULL;
    MEfreetag( tag );

    return( II_RF_ST_OK );
}

/*
** Function:
**
**	 IIrfapi_errString
**
** Description:
**
**	Lookup "name" (string) of an II_RFAPI_STATUS code.
**
** Inputs:
**
**	II_RFAPI_STATUS	*errcode - Error code to lookup.
**
** Outputs:
**
**	None
**
** Returns:
**
**	    II_RFAPI_STRING containing the "name" of "errcode"
*/
II_RFAPI_STRING
IIrfapi_errString( II_RFAPI_STATUS errcode )
{
    /* check we've been passed a valid error code */
    if ( errcode < RFAPI_STATUS_MAX )
	/* just return value from errors array */
	return( rfapierrs[ errcode ] );
    else
	return( rfapierrs[ II_RF_ST_UNKNOWN_ERROR ] );
}
