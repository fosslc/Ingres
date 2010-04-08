/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <er.h>
#include    <gl.h>
#include    <lo.h>
#include    <cv.h>
#include    <dl.h>
#include    <me.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>

#include    <iifas.h>
#include    <ursm.h>

/**
**
**  Name: URSINTFC.C - URS Interface Services
**
**  Description:
**      This file contains the service routines for the Interfaces defined 
** 	to an application in the Frontier. Routines are included to get 
** 	interface information (from the repository), start the interface, 
** 	build and add to the interface method's parameter list, call the 
** 	interface's method(s), release the parameter list, and shut down 
** 	the interface.
**
**	urs_find_intfc		Get the requested interface by its identifier
**	urs_find_method		Get the requested method by its identifier
**	urs_start_intfc		Start the interface
** 	urs_shut_intfc		Shut down the interface
**	urs_start_intfc		Start the interface
** 	urs_exec_method		Execute the interface method
**	urs_begin_parm		Build an in/out parameter list
**	urs_add_parm		Add a parameter to the parameter list
**	urs_release_parm	Release an in/out parameter list
** 	urs_get_type		Get parameter type from the parameter list
** 	urs_refresh_intfc	Refresh (Stop/Start) the interface
** 	urs_get_intfc		Get the next interface def'n
**	urs_get_method		Get the next method def'n
**	urs_get_arg		Get the next argument def'n
**
**  History:
** 	28-Jan-1998 wonst02
** 	    Added urs_begin_parm, urs_add_parm.
** 	    Added urs_load_dll(), urs_free_dll() for OpenRoad.
** 	11-Mar-1998 wonst02
** 	    Remove ursm parameter from urs_add_parm; add urs_get_type.
**      21-Mar-1998 wonst02
**          Original Interface layer (moved some code from ursappl.c).
** 	16-Jun-1998 wonst02
** 	    Add get_intfc, get_method, get_arg routines.
** 	    Add non-OpenRoad method handling.
**	10-Aug-1998 fanra01, wonst02
**	    Fix some comments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Static globals
*/
	static	char *Ctype[] = {"FAS_ARG_NONE",
				 "FAS_ARG_IN",
				 "FAS_ARG_OUT",
				 "FAS_ARG_INOUT"};

/*
** Macros
*/

#define ISVARCHAR(t) \
	((t)==IIAPI_VCH_TYPE || (t)==IIAPI_VBYTE_TYPE || \
	 (t)==IIAPI_TXT_TYPE || (t)==IIAPI_LTXT_TYPE)

/*
** The following macro takes GCA values for type
** and length, and expects the data to point to a
** buffer which can hold the GCA form of the data.
*/
#define ISNULL(type,length,data)  \
	((type) > 0 ? FALSE  \
		  : (*(((II_CHAR *)(data)) + (length) - 1) & ADF_NVL_BIT  \
			? TRUE : FALSE) )

/*
** The following macros take the API values for type
** and length (as stored in the descriptor), and expect
** the data to point to a buffer which can hold the
** GCA form of the data.
*/
#define ISNULLDESC(descriptor,data)  \
	( (descriptor)->ds_nullable  \
	    ? (*((II_CHAR *)(data) + getGCALength(descriptor) - 1)  \
								& ADF_NVL_BIT \
		? TRUE : FALSE)  \
	    : FALSE )

#define SETNULL(length,data)  \
	(*(((II_CHAR *)(data)) + (length) - 1) = (II_CHAR)ADF_NVL_BIT)

#define SETNONULL(length,data)  \
	(*(((II_CHAR *)(data)) + (length) - 1) = 0)


/*{
** Name: urs_find_intfc - Get the requested interface by its identifier
**
** Description:
** 		Lookup the appropriate interface by its ID number.
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
** 		
**	Returns:
** 		A ptr to an interface block or NULL, if not found.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      05-jun-1998 wonst02
**          Original
*/
FAS_INTFC *
urs_find_intfc(URS_MGR_CB	*ursm,
	      i4		appl_id,
	      i4		intfc_id)
{
	FAS_APPL	*appl;
	FAS_INTFC	*intfc;

	appl = urs_get_appl(ursm, appl_id);
	if (appl == NULL)
	    return NULL;

/*	This should be a hash table lookup */
	for (intfc = appl->fas_appl_intfc;
	     intfc; 
	     intfc = intfc->fas_intfc_next)
	{
	    if (intfc->fas_intfc_id == intfc_id)
	     	break;
	}

	return intfc;
}

/*{
** Name: urs_find_method - Get the requested method by its identifier
**
** Description:
** 		Lookup the appropriate method by its ID number.
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
** 		
**	Returns:
** 		A ptr to an interface block or NULL, if not found.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      08-jun-1998 wonst02
**          Original
*/
FAS_METHOD *
urs_find_method(URS_MGR_CB	*ursm,
	       i4		appl_id,
	       i4		intfc_id,	       
	       i4		method_id)
{
	FAS_INTFC	*intfc;
	FAS_METHOD	*method;

	intfc = urs_find_intfc(ursm, appl_id, intfc_id);
	if (intfc == NULL)
	    return NULL;

/*	This should be a hash table lookup */
	for (method = intfc->fas_intfc_method;
	     method; 
	     method = method->fas_method_next)
	{
	    if (method->fas_method_id == method_id)
	     	break;
	}

	return method;
}

/*{
** Name: lookup_intfc - Lookup the requested interface
**
** Description:
** 	Look up the interface by its interface name.
** 
** 	NOTE: the caller must protect against simultaneous access and update
** 	to the list of applications by holding the fas_srvr_applsem semaphore.
**
** Inputs:
**      ursm			The URS Manager Control Block
** 	ursb			A User Request Services block
** 	addflag			If TRUE, add a new interface block if not found.
** 				NOTE: The caller had better be holding a
** 				semaphore for the application's interface list
** 				if the addflag is TRUE.
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
**
** Side Effects:
**	If addflag is TRUE and the interface is not found, this routine adds
** 	the interface to the application's interface list.
**
** History:
**      13-Apr-1998 wonst02
**          Original
*/
FAS_INTFC *
lookup_intfc(URS_MGR_CB		*ursm,
	     URSB		*ursb,
	     FAS_INTFC_NAME	intfclookup,
	     bool		addflag)
{
	FAS_APPL		*appl;
	FAS_INTFC		*intfc;

	/* -----------------3/30/98 3:41PM-------------------
	** This sequential search could be replaced by
	** a hash table lookup.
	** --------------------------------------------------*/
	for (appl = ursm->ursm_srvr->fas_srvr_appl;
	     appl;
	     appl = appl->fas_appl_next)
	{
	    for (intfc = appl->fas_appl_intfc; 
	    	 intfc; 
		 intfc = intfc->fas_intfc_next)
	    {	
	    	if (MEcmp(intfc->fas_intfc_name.db_name, 
		   	  intfclookup.db_name,
			  sizeof intfclookup.db_name) != 0)
		    continue;
		return intfc;
	    }
	}
	/*
	** The interface is not found--see if it should be added.
	*/
	if (addflag)
	    intfc = (FAS_INTFC *)
	    	urs_palloc(&appl->fas_appl_ulm_rcb, ursb, sizeof(FAS_INTFC));

	if (intfc)
	{
	    /*
	    ** Copy the basic info to the new block--the remainder 
	    ** of the information will come from the repository.
	    ** Add the new block to the front of the chain.
	    */
	    MEfill(sizeof(FAS_INTFC), 0, intfc);
	    STRUCT_ASSIGN_MACRO(intfclookup, intfc->fas_intfc_name);
	    intfc->fas_intfc_appl = appl;
	    intfc->fas_intfc_next = appl->fas_appl_intfc;
	    appl->fas_appl_intfc = intfc;
	}
	else
	    urs_error(E_UR0720_URS_INTFC_NOT_FOUND, URS_USERERR, 1,
		      sizeof intfclookup.db_name, intfclookup.db_name);
	return intfc;
}

/*{
** Name: lookup_method - Lookup the requested method
**
** Description:
** 	Look up the method by its interface name and method name.
** 
** 	NOTE: the caller must protect against simultaneous access and update
** 	to the list of applications by holding the fas_srvr_applsem semaphore.
**
** Inputs:
**      ursm			The URS Manager Control Block
**
** Outputs:
**	Returns:
** 		Pointer to FAS_METHOD control block, or
** 		NULL, if none found.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      30-Mar-1998 wonst02
**          Original
*/
FAS_METHOD *
lookup_method(URS_MGR_CB	*ursm,
	      URSB		*ursb,
	      FAS_INTFC_NAME	intfclookup,
	      FAS_METHOD_NAME	methodlookup)
{
	FAS_INTFC		*intfc;
	FAS_METHOD		*method;

	intfc = lookup_intfc(ursm, ursb, intfclookup, FALSE);
	if (intfc == NULL)
	    return NULL;

	/* -----------------3/30/98 3:41PM-------------------
	** This sequential search should be replaced by
	** a hash table lookup.
	** --------------------------------------------------*/

	for (method = intfc->fas_intfc_method;
	     method;
	     method = method->fas_method_next)
	{
	     if (MEcmp(method->fas_method_name.db_name, 
		       methodlookup.db_name,
		       sizeof methodlookup.db_name) == 0)
		return method;
	}

	urs_error(E_UR0700_URS_METHOD_NOT_FOUND, URS_USERERR, 2,
	    	  sizeof intfclookup.db_name, intfclookup.db_name,
		  sizeof methodlookup.db_name, methodlookup.db_name);
	return NULL;
}

/*{
** Name: lookup_arg - Lookup the requested argument by its identifier & type
**
** Description:
** 		Lookup the appropriate input or output argument by its number.
** 		The arguments are numbered 1, 2, 3, ....
**
** Inputs:
** 		method			The method containing the argument.
** 		arg_id			The integer argument number.
** 		arg_type		Either FAS_ARG_IN OR FAS_ARG_OUT.
**
** Outputs:
** 		
**	Returns:
** 		A ptr to an argument block.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      30-Mar-1998 wonst02
**          Original
** 	26-Jun-1998 wonst02
** 	    Pass FAS_METHOD ptr instead of URSB.
*/
FAS_ARG *
lookup_arg(FAS_METHOD	*method,
	   i4		arg_id,
	   i4		arg_type)
{
	FAS_ARG			*arg;
	i4			id = 0;

	for (arg = method->fas_method_arg; 
	     arg; 
	     arg = arg->fas_arg_next)
	{
	     if (arg->fas_arg_type == arg_type || 
	     	 arg->fas_arg_type == FAS_ARG_INOUT)
	     {
	     	++id;
	     	if (id == arg_id)
	     	    break;
	     }
	}

	if (arg == NULL)
	{
	    urs_error(E_UR0710_URS_ARG_NOT_FOUND, URS_INTERR, 3,
		      sizeof arg_id, &arg_id,
	    	      sizeof method->fas_method_name.db_name, 
		      	method->fas_method_name.db_name,
		      STlength(Ctype[arg_type]), Ctype[arg_type]);
	}
	return arg;
}

/*
** Name: getAPILength - get length of a data value
**
** Description:
**	This function returns the length of a data value as 
**	represented within the API (no null indicator)
**	depending on its GCA data type.  Note that the length 
**	of the GCA data value is different from the API data 
**	type.  See rule in file description above.
**
**	If the dataType indicates variable length data and
**	the data buffer parameter is not NULL and the data
**	value itself is not NULL, the length returned will 
**	be that of the actual data plus size of the length
**	indicator.
**
**	This function does not handle long data types.
**
** Input:
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
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
** 	07-apr-1998 wonst02
** 	    Cloned from apidatav.c
*/

II_UINT2
getAPILength 
(
    DB_DT_ID	dataType,
    II_LONG	dataLength,
    char	*buffer
)
{
    II_UINT2	var_len;
    II_LONG	length = dataLength;

    if ( dataType < 0 )  length--;

    if ( 
	 ISVARCHAR( abs( dataType ) )  &&
	 buffer != NULL  && 
	 ! ISNULL( dataType, dataLength, buffer ) 
       )
    {
	MECOPY_CONST_MACRO( buffer, sizeof( var_len ), (PTR)&var_len );
	length = sizeof( var_len ) + var_len;
    }

    return( (II_UINT2)length );
}

/*
** Name: getGCALength - get length of a data value
**
** Description:
**	This function returns the length of a data value as 
**	represented within GCA (including null indicator and
**	variable data length) depending on its API data type.  
**
**	This function does not handle large data types.
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
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
** 	07-apr-1998 wonst02
** 	    Cloned from apidatav.c
*/

II_UINT2
getGCALength( IIAPI_DESCRIPTOR *descriptor )
{
    return( (II_UINT2)(descriptor->ds_nullable 
		       ? descriptor->ds_length + 1 : descriptor->ds_length) );
}


/*
** Name: cnvtGCA2DataValue - copy GCA data into API format
**
** Description:
**	This function copies the GCA data into API format.  Caller 
**	must allocate data value buffer at least as large as the 
**	length in the descriptor.
**
**	This function does not handle long data types.  
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
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
** 	07-apr-1998 wonst02
** 	    Cloned from apidatav.c
*/

II_VOID
cnvtGCA2DataValue 
(
    IIAPI_DESCRIPTOR	*descriptor,
    DB_DATA_VALUE	*dbdv,
    IIAPI_DATAVALUE	*dataValue
)
{
    IIAPI_DT_ID		api_datatype;

    if ( ISNULLDESC( descriptor, dbdv->db_data ) )
    {
	dataValue->dv_null = TRUE;
	dataValue->dv_length = 0;
    }
    else  
    {
	dataValue->dv_null = FALSE;

	/*
	** This may seem like a circular way of doing
	** things, but getAPILength() will handle
	** variable length data types by looking at the
	** actual data rather than just the descriptor.
	** Since getAPILength() requires the GCA
	** data length and all we have is the max API 
	** length in the descriptor, we also have to
	** call getGCALength().
	*/
	api_datatype = descriptor->ds_nullable 
				    ? -descriptor->ds_dataType 
				    : descriptor->ds_dataType;
	dataValue->dv_length = 
		getAPILength( dbdv->db_datatype,
			      getGCALength( descriptor ),
			      dbdv->db_data );

	switch ( api_datatype )
	{
	    case IIAPI_CHA_TYPE:
	    case IIAPI_CHR_TYPE:
		switch ( dbdv->db_datatype )
		{
		    case DB_CHA_TYPE:
		    case DB_CHR_TYPE:
			MEcopy( dbdv->db_data, 
				dbdv->db_length, 
				dataValue->dv_value );
			if ( dbdv->db_length < dataValue->dv_length )
			{
			    MEfill( dataValue->dv_length - dbdv->db_length, 
			    	    ' ',
				    (PTR)((char *)dataValue->dv_value + 
				    		  dbdv->db_length) );
			}
			break;
		    case DB_TXT_TYPE:
		    case DB_VCH_TYPE:
		    {
		    	u_i2	len = (u_i2)dbdv->db_length;

		    	I2ASSIGN_MACRO(len, *(u_i2 *)dataValue->dv_value);
			MEcopy( dbdv->db_data, 
				dbdv->db_length, 
				((char *)dataValue->dv_value) + sizeof len );
			if ( dbdv->db_length < dataValue->dv_length )
			{
			    MEfill( dataValue->dv_length - dbdv->db_length, ' ',
				    (PTR)((char *)dataValue->dv_value + 
				    		  sizeof len +
				    		  dbdv->db_length) );
			}
			break;
		    }
		}
		break;

	    case IIAPI_TXT_TYPE:
	    case IIAPI_VCH_TYPE:
		switch ( dbdv->db_datatype )
		{
		    case DB_CHA_TYPE:
		    case DB_CHR_TYPE:
		    {	
		    	u_i2	len = (u_i2)dbdv->db_length;

		    	MEcopy(dbdv->db_data,
			       dbdv->db_length,
			       ((char *)dataValue->dv_value) + 2);
			I2ASSIGN_MACRO(len, *(u_i2 *)dataValue->dv_value);
			break;
		    }		
		    case DB_TXT_TYPE:
		    case DB_VCH_TYPE:
		    	MEcopy(dbdv->db_data,
			       dbdv->db_length,
			       dataValue->dv_value);
			break;
		}
		break;
	    	
	    case IIAPI_INT_TYPE:
	    	switch (dataValue->dv_length)
		{
		    case 1 :
			*(char *)dataValue->dv_value = *(char *)dbdv->db_data;
			break;
		    case 2 :
		    	I2ASSIGN_MACRO(*(i2 *)dbdv->db_data, 
				       *(i2 *)dataValue->dv_value);
			break;
		    case 4 :
		    	I4ASSIGN_MACRO(*(i4 *)dbdv->db_data, 
				       *(i4 *)dataValue->dv_value);
			break;
		}
		break;
	    	
	    case IIAPI_FLT_TYPE:
	    	switch (dataValue->dv_length)
		{
		    case 4 :
		    	F4ASSIGN_MACRO(dbdv->db_data, dataValue->dv_value);
			break;
		    case 8 :
		    	F8ASSIGN_MACRO(dbdv->db_data, dataValue->dv_value);
			break;
		}
		break;
	}

    }

    return;
}

/*
** Name: cnvtDescr2GCA
**
** Description:
**	This function copies the content of the API descriptor
**	into the format of the GCA Data Value descriptor.
**
**	For large data types, the GCA length can not be calculated
**	here.  The ADP_PERIPHERAL header size is used.
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
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
** 	07-apr-1998 wonst02
** 	    Cloned from apidatav.c
*/

II_VOID
cnvtDescr2GCA
(
    IIAPI_DESCRIPTOR	*descriptor,
    DB_DATA_VALUE	*dbdv
)
{
	dbdv->db_datatype = descriptor->ds_nullable 
			   ? -descriptor->ds_dataType 
			   : descriptor->ds_dataType;

	dbdv->db_length = getGCALength( descriptor );

	dbdv->db_prec =
	    DB_PS_ENCODE_MACRO( descriptor->ds_precision, descriptor->ds_scale );
    
	return;
}

/*
** Name: cnvtDataValue2GCA
**
** Description:
**	This function copies the content of the API descriptor and
**	data value into the format of the GCA Data Value.  The GCA
**	data value buffer is allocated.
**
**	This function provides limited support for large data types.
**	The BLOB must be NULL or consist of only one segment.
**
** Input:
**	memTag		Tag for memory allocation.
**	descriptor	API descriptor
**	dataValue	API data value
**
** Output:
**	dbdv		GCA data value
**
** Returns:
**	II_BOOL		TRUE if success, FALSE if memory allocation failure.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
** 	07-apr-1998 wonst02
** 	    Cloned from apidatav.c
*/

II_BOOL
cnvtDataValue2GCA
(
    II_UINT2		memTag,
    IIAPI_DESCRIPTOR	*descriptor,
    IIAPI_DATAVALUE	*dataValue,
    DB_DATA_VALUE	*dbdv
)
{
    STATUS	status;

    dbdv->db_datatype = descriptor->ds_nullable 
			     ? -descriptor->ds_dataType 
			     : descriptor->ds_dataType;
    dbdv->db_prec =
	DB_PS_ENCODE_MACRO( descriptor->ds_precision, descriptor->ds_scale );

    {
	II_UINT2 length = min( descriptor->ds_length, dataValue->dv_length );

	dbdv->db_length = getGCALength( descriptor );
    
	dbdv->db_data = (char *)
		 MEreqmem(memTag, dbdv->db_length, TRUE , &status);
	if (dbdv->db_data == NULL)
	    return( FALSE );
    
	if ( ! descriptor->ds_nullable )
	{
	    MEcopy( dataValue->dv_value, length, dbdv->db_data );

	    if ( length < descriptor->ds_length )
		MEfill( descriptor->ds_length - length, 
			(descriptor->ds_dataType == IIAPI_CHA_TYPE  ||
			 descriptor->ds_dataType == IIAPI_CHR_TYPE) ? ' ' : '\0', 
			(PTR)((char *)dbdv->db_data + length) );
	}
	else  if ( dataValue->dv_null )
	{
	    MEfill( descriptor->ds_length, '\0', dbdv->db_data );
	    SETNULL(dbdv->db_length, dbdv->db_data);
	}
	else
	{
	    MEcopy( dataValue->dv_value, length, dbdv->db_data );

	    if ( length < descriptor->ds_length )
		MEfill( descriptor->ds_length - length, 
			(descriptor->ds_dataType == IIAPI_CHA_TYPE  ||
			 descriptor->ds_dataType == IIAPI_CHR_TYPE) ? ' ':'\0', 
			(PTR)((char *)dbdv->db_data + length) );

	    SETNONULL(dbdv->db_length, dbdv->db_data);
	}
    }

    return( TRUE );
}

/*{
** Name: urs_shut_intfc	Shut down the interface
**
** Description:
**      Stop the Interface.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	intfc				Address of FAS interface block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 	    E_DB_OK			Interface stopped OK
** 	    E_DB_WARN			Interface was not stared
** 	    E_DB_ERROR			Error stopping interface
** 					(ursb_error field has more info)
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
*/
DB_STATUS
urs_shut_intfc(URS_MGR_CB	*ursm,
	       FAS_INTFC	*intfc,
	       URSB		*ursb)
{
	DB_STATUS	status = E_DB_OK;
	int		s = OK;
	int 		(*pAS_Terminate) ();

	if (!(intfc->fas_intfc_flags & FAS_STARTED))
	{
	    return E_DB_WARN;
	}
	pAS_Terminate = (int (*)()) intfc->fas_intfc_appl->fas_appl_addr[AS_TERMINATE];
	s = (*pAS_Terminate) ();
	if (s != OK)
	{
	    status = E_DB_ERROR;
	    SET_URSB_ERROR(ursb, E_UR0115_INTFC_SHUTDOWN, status)
	    urs_error(E_UR0115_INTFC_SHUTDOWN, URS_INTERR, 1,
	    	      sizeof(s), &s);
	}
	intfc->fas_intfc_flags ^= FAS_STARTED;

	return status;
}

/*{
** Name: urs_start_intfc	Start the interface
**
** Description:
**      Start the interface.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	intfc				A filled-in FAS interface block
**
** Outputs:
**
**	Returns:
** 	    E_DB_OK			Interface started OK
** 	    E_DB_WARN			Interface was already started
** 	    E_DB_ERROR			Error starting interface
** 					(ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
*/
DB_STATUS
urs_start_intfc(URS_MGR_CB	*ursm,
		FAS_INTFC	*intfc)
{
	char			intfc_name[DB_MAXNAME + INTFC_PARM_LENGTH + 2];
	char			*name = intfc->fas_intfc_name.db_name;
	char			*nameend;
	i4			namesize;
	int			s = OK;
	int 			(*pAS_Initiate) ();

	if (intfc->fas_intfc_flags & FAS_STARTED)
	{
	    return E_DB_WARN;
	}

	nameend = STindex(name, ERx(" "), sizeof intfc->fas_intfc_name.db_name);
	if (nameend == NULL)
		nameend = name + sizeof intfc->fas_intfc_name.db_name;
	namesize = nameend - name;
	MEcopy(name, namesize, intfc_name);
	intfc_name[namesize++] = ' ';
	STlcopy(intfc->fas_intfc_parm, intfc_name + namesize, 
		STlength(intfc->fas_intfc_parm));

	pAS_Initiate = (int (*)()) intfc->fas_intfc_appl->fas_appl_addr[AS_INITIATE];
	s = (*pAS_Initiate) (intfc_name);
	/* -----------------4/7/98 2:52PM--------------------
	** FIXME: AS_Initiate is not returning a valid return code
	** --------------------------------------------------*/
	
	intfc->fas_intfc_flags |= FAS_STARTED;
	return E_DB_OK;
}

/*{
** Name: urs_exec_method	Execute the interface method
**
** Description:
**      Execute the interface method.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block, containing:
** 	.ursb_parm			ptr to the parm info, including
** 					method name, interface name,
**	  IIFAS_DATASET	*input_dataset	ptr to the method's input parameters, and
**	  IIFAS_DATASET	*output_dataset	ptr to the method's output parameters.
**
** Outputs:
** 	ursb				User Request Services Block, containing:
** 	 .ursb_parm			ptr to the parm info, including
**	  IIFAS_DATASET	*output_dataset	ptr to the method's output parameters.
**
** Returns:
** 	E_DB_OK				All is well
** 	E_DB_ERROR			Something went awry; see ursb_error.
**
** Side Effects:
**	none
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
** 	18-Jun-1998 wonst02
** 	    Add non-OpenRoad method handling.
*/
DB_STATUS
urs_exec_method(URS_MGR_CB	*ursm,
	    	URSB		*ursb)
{
	FAS_METHOD	*method = (FAS_METHOD *)ursb->ursb_method;
	FAS_APPL	*appl = method->fas_method_intfc->fas_intfc_appl;
	URSB_PARM	*ursbparm = ursb->ursb_parm;
	long		as_status;
	int		s;

	if (appl->fas_appl_type == FAS_APPL_OPENROAD)
	{
	    char	intfc_name[DB_MAXNAME + 1];
	    char	*nameimg;
	    int 	(*pAS_ExecuteMethod) ();
	    /*
	    ** Execute the OpenRoad method; the result comes
	    ** back via output_dataset
	    */
	    STcopy(ursbparm->interfacename, intfc_name);
	    CVlower(intfc_name);
	    nameimg = STindex(intfc_name, ERx(".img"), 0);
	    if (nameimg != NULL)
		*nameimg = '\0';
	    pAS_ExecuteMethod = (int (*)()) appl->fas_appl_addr[AS_EXECUTEMETHOD];
	    s = (*pAS_ExecuteMethod) (intfc_name,
				      ursbparm->methodname,
				      ursbparm->sNumDescs,
				      ursbparm->pDescs,
				      ursbparm->sNumInValues,
				      ursbparm->pInValues,
				      &ursbparm->sNumOutValues, 
				      &ursbparm->pOutValues,
				      &as_status);
	}
/*FIXME: Temporarily, treat Frontier as special... need to fix C/C++ parms!   */
	else if (STcompare(ursbparm->interfacename, "Frontier") == 0)         
	{                                                                     
	    int 	(*pMethod) ();                                        
									      
	    pMethod = (int (*)()) method->fas_method_addr;                    
	    s = ((*pMethod) (ursm, ursb) == E_DB_OK) ? AS_OK : AS_ERROR;      
	}                                                                     
	else
	{
	    /*
	    ** Call the C/C++ method
	    */
	    s = urs_call_method(ursm, ursb);
	}		      

	switch (s)
	{
	    case AS_OK:
		ursb->ursb_num_ele = method->fas_method_outargcount;
		ursb->ursb_num_recs = ursbparm->sNumOutValues / 
			      		method->fas_method_outargcount;
	    	break;
	    case AS_ERROR:
	    	urs_error(E_UR0140_AS_ERROR, URS_INTERR, 1,
			  sizeof s, &s);
	    	SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
		break;
	    case AS_UNKNOWN_INTERFACE:
	    	urs_error(E_UR0141_AS_UNKNOWN_INTERFACE, URS_INTERR, 1,
		    STlength(ursbparm->interfacename), ursbparm->interfacename);
	    	SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
		break;
	    case AS_UNKNOWN_METHOD:
	    	urs_error(E_UR0142_AS_UNKNOWN_METHOD, URS_INTERR, 1,
			  STlength(ursbparm->methodname), ursbparm->methodname);
	    	SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
		break;
	    case AS_MEMORY_FAILURE:
	    	urs_error(E_UR0143_AS_MEMORY_FAILURE, URS_INTERR, 0);
	    	SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
		break;
	    default:
	    	urs_error(E_UR0140_AS_ERROR, URS_INTERR, 1,
			  sizeof s, &s);
	    	SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
		break;
	}

	return (s == AS_OK) ? E_DB_OK : E_DB_ERROR;
}

/*{
** Name: urs_begin_parm	- Begin an in/out parameter list
**
** Description:
** 	Lookup the method and begin the parameter list.
**
** Inputs:
** 	ursb				A User Request Services Block
** 	    .ursb_parm			The parm info, including: the
** 					method name, interface name, and
** 					a DB_DATA_VALUE having the number of
** 					parameters.
**
** Outputs:
** 	ursb
** 	    .ursb_parm
** 		.pDescs			Ptr to a IIAPI descriptor array.
** 		.error			DB_ERROR, if return code not OK
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
*/
DB_STATUS
urs_begin_parm(URS_MGR_CB	*ursm,
	       URSB		*ursb)
{
	ULM_RCB		*ulm = &ursb->ursb_ulm;
	FAS_METHOD	*method;
	FAS_SRVR	*fas = ursm->ursm_srvr;
	FAS_ARG		*arg;
	FAS_INTFC_NAME	intfclookup;
	FAS_METHOD_NAME	methodlookup;
	IIAPI_DESCRIPTOR*apids;
	i4		i4count;
	char		*nameend;
	i4		namesize;
	DB_STATUS	status;

	/* -----------------4/13/98 11:18AM------------------
	** FIXME: For now, get exclusive access when using the
	** application list and running an application: W4GL
	** can run only one app at a time.
	** --------------------------------------------------*/
	CSp_semaphore( TRUE, &fas->fas_srvr_applsem );
	STmove(ursb->ursb_parm->interfacename, ' ', 
	       sizeof intfclookup.db_name, intfclookup.db_name);
	STmove(ursb->ursb_parm->methodname, ' ', 
	       sizeof methodlookup.db_name, methodlookup.db_name);

	method = lookup_method(ursm, ursb, intfclookup, methodlookup);
	ursb->ursb_method = (PTR)method;
	if (method == NULL)
	{	
	    CSv_semaphore( &fas->fas_srvr_applsem );
	    SET_URSB_ERROR(ursb, E_UR0102_BEGIN_PARM, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	if ((method->fas_method_flags & FAS_DISABLED) ||
	    (method->fas_method_intfc->fas_intfc_flags & FAS_DISABLED) ||	
	    (method->fas_method_intfc->fas_intfc_appl->fas_appl_flags 
	    	& FAS_DISABLED))
	{	
	    CSv_semaphore( &fas->fas_srvr_applsem );
	    urs_error(E_UR0703_URS_METHOD_DISABLED, URS_USERERR, 2,
	    	      STlength(ursb->ursb_parm->methodname), 
		      ursb->ursb_parm->methodname,
		      STlength(ursb->ursb_parm->interfacename),
		      ursb->ursb_parm->interfacename);
	    SET_URSB_ERROR(ursb, E_UR0102_BEGIN_PARM, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	/* The first parameter has the number of input & in/out parameters */
	I4ASSIGN_MACRO(*ursb->ursb_parm->data_value.db_data, i4count);
	if (i4count != method->fas_method_inargcount)
	{
	    CSv_semaphore( &fas->fas_srvr_applsem );
	    urs_error(E_UR0701_URS_WRONG_ARGCOUNT, URS_CALLERR, 3,
		      STlength(ursb->ursb_parm->methodname),
		      ursb->ursb_parm->methodname,
		      sizeof i4count, &i4count,
		      sizeof method->fas_method_inargcount, 
		      &method->fas_method_inargcount);
	    SET_URSB_ERROR(ursb, E_UR0102_BEGIN_PARM, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	/* If no arguments at all, take an early out */
	if (method->fas_method_argcount == 0)
	    return E_DB_OK;

	/*
	** Allocate a memory stream for the method's parameter list.
	*/
	STRUCT_ASSIGN_MACRO(ursm->ursm_ulm_rcb, *ulm);
	ulm->ulm_flags = ULM_PRIVATE_STREAM;
	ulm->ulm_memleft = &ursb->ursb_memleft;
	ursb->ursb_memleft = URS_APPMEM_DEFAULT;
	if ((status = ulm_openstream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0312_ULM_OPENSTREAM_ERROR, URS_INTERR, 3,
	    	      sizeof(*ulm->ulm_memleft), ulm->ulm_memleft,
	    	      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
	    	      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize);
	    if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, status)
	    else
	    	SET_URSB_ERROR(ursb, E_UR0102_BEGIN_PARM, status)
	    return status;
	}

	/* Allocate memory for the DESCRIPTOR & DATAVALUE arrays. */
	ursb->ursb_parm->pDescs = (IIAPI_DESCRIPTOR *)
	     urs_palloc(ulm, ursb, 
			method->fas_method_argcount * 
			(sizeof(IIAPI_DESCRIPTOR)+100*sizeof(IIAPI_DATAVALUE)));
/*FIXME: Oooh, ugly! Use 100 parameter limit for now... */
/*	replace with the real C/C++ methodology when available. */
	if (ursb->ursb_parm->pDescs == NULL)
	{	
	    SET_URSB_ERROR(ursb, E_UR0206_URS_BEGIN_PARM_ERR, E_DB_ERROR)
	    return E_DB_ERROR;
	}
	ursb->ursb_parm->pInValues = (IIAPI_DATAVALUE *)
		(ursb->ursb_parm->pDescs + method->fas_method_argcount);
	ursb->ursb_parm->pOutValues = (IIAPI_DATAVALUE *)
		(ursb->ursb_parm->pInValues + method->fas_method_inargcount);
	ursb->ursb_parm->sNumDescs = (short)method->fas_method_argcount;
	ursb->ursb_parm->sNumInValues = 0;
	ursb->ursb_parm->sNumOutValues = 0;

	/*
	** Build the parameter descriptor array.
	*/
	for (arg = method->fas_method_arg, apids = ursb->ursb_parm->pDescs;
	     arg;
	     arg = arg->fas_arg_next, apids++)
	{
	    nameend = STindex(arg->fas_arg_name.db_name, ERx(" "), 
				  sizeof arg->fas_arg_name.db_name);
	    if (nameend == NULL)
	        nameend = arg->fas_arg_name.db_name + 
			      sizeof arg->fas_arg_name.db_name;
	    namesize = nameend - arg->fas_arg_name.db_name;
	    apids->ds_columnName = (char *)urs_palloc(ulm, ursb, namesize+1);
	    if (apids->ds_columnName == NULL)
	    {	
	    	SET_URSB_ERROR(ursb, E_UR0206_URS_BEGIN_PARM_ERR, E_DB_ERROR)
	    	return E_DB_ERROR;
	    }
	    MEcopy(arg->fas_arg_name.db_name, namesize, apids->ds_columnName);
	    apids->ds_columnName[namesize] = '\0';
	    if (arg->fas_arg_data.db_datatype < 0)
	    {	
	    	apids->ds_nullable = TRUE;
	    	apids->ds_dataType = -1 * arg->fas_arg_data.db_datatype;
	    }
	    else 
	    {
	    	apids->ds_nullable = FALSE;
	    	apids->ds_dataType = arg->fas_arg_data.db_datatype;
	    }	
	    apids->ds_length = (II_UINT2)arg->fas_arg_data.db_length;
	    if (ISVARCHAR(apids->ds_dataType))
		apids->ds_length += 2;
	    apids->ds_precision = DB_P_DECODE_MACRO(arg->fas_arg_data.db_prec);
	    apids->ds_scale = DB_S_DECODE_MACRO(arg->fas_arg_data.db_prec);
	    apids->ds_columnType = (arg->fas_arg_type == FAS_ARG_IN) ? 
	    	IIAPI_COL_PROCPARM : IIAPI_COL_PROCBYREFPARM;
	}
	return E_DB_OK;
}

/*{
** Name: urs_add_parm	Add a parameter to the parameter list
**
** Description:
** 	Uses the information passed in the DB_DATA_VALUE struct about one
** 	parameter to add to the parameter list.
**
** Inputs:
** 	ursb				A User Request Services Block
** 	    .ursb_flags			Usage: URSB_INPUT/_OUTPUT/_INOUT.
** 	    .ursb_parm			The add info, including: the
** 					DB_DATA_VALUE and ptr to IIFAS_DATASET.
**
** Outputs:
** 	ursb
** 	    .ursb_parm
** 		.pDescs			Contains the added parameter descriptor.
** 		.error			DB_ERROR, if return code not OK
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**	Increments ursb->ursb_parm->sNumInValues or sNumOutValues to step to
** 	the next element.
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
** 	06-Apr-1998 wonst02
** 	    Convert to IIAPI descriptors and values.
** 	18-Jun-1998 wonst02
** 	    Add capability to build output values.
*/
DB_STATUS
urs_add_parm(URSB	*ursb)
{
	u_i2		dv_length;
	i4		ele_id;
	i4		arg_type;
	DB_DATA_VALUE	*dbdv;
	IIAPI_DESCRIPTOR*apids;
	IIAPI_DATAVALUE	*apidv;
	URSB_PARM	*parm = ursb->ursb_parm;
	ULM_RCB		*ulm = &ursb->ursb_ulm;
	FAS_METHOD 	*method = (FAS_METHOD *)ursb->ursb_method;
	FAS_ARG		*arg;
	DB_STATUS	status = E_DB_OK;

	if ((parm == NULL) || (parm->pDescs == NULL))
	{
	    urs_error(E_UR0401_URS_ADD_PARM_USAGE, URS_CALLERR, 2,
		      sizeof parm, parm,
		      sizeof parm->pDescs,
		      (parm) ? parm->pDescs :
		      	(IIAPI_DESCRIPTOR *)NULL);
	    SET_URSB_ERROR(ursb, E_UR0401_URS_ADD_PARM_USAGE, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	dbdv = &parm->data_value;

	/* Ptr to next parm in the set */
	if (ursb->ursb_flags != URSB_OUTPUT)
	{	
	    apidv = parm->pInValues + parm->sNumInValues++;
	    ele_id = parm->sNumInValues;
	    arg_type = FAS_ARG_IN;
	}
	else
	{	
	    ele_id = parm->sNumOutValues % method->fas_method_outargcount + 1;
	    apidv = parm->pOutValues + parm->sNumOutValues++;
	    arg_type = FAS_ARG_OUT;
	}
	arg = lookup_arg(method, ele_id, arg_type);
	if (arg == NULL)
	{	
	    SET_URSB_ERROR(ursb, E_UR0205_URS_ADD_PARM_ERR, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	apids = parm->pDescs + arg->fas_arg_id - 1;
	dv_length = apids->ds_length;
	/* Allocate enough space for the value */
	apidv->dv_value = (char *)urs_palloc(ulm, ursb, dv_length);
	if (apidv->dv_value == NULL)
	{	
	    SET_URSB_ERROR(ursb, E_UR0205_URS_ADD_PARM_ERR, E_DB_ERROR)
	    return E_DB_ERROR;
	}
	cnvtGCA2DataValue(apids, dbdv, apidv);

	return status;
}

/*{
** Name: urs_release_parm	Release an in/out parameter list
**
** Description:
** 	Release memory allocated by the interface method for the output
** 	parameter set. Release the ULM stream associated with the input
** 	parameter set and the in/out parameter list.
**
** Inputs:
** 	ursb					A User Request Services Block
** 	    .ursb_ulm			The ULM rcb for the stream.
**
** Outputs:
** 	ursb
** 	    .ursb_parm
** 		.pInValues		The ptr is set NULL.
** 		.error				DB_ERROR, if return code not OK
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
** 	10-Jun-1998 wonst02
** 	    Release the semaphore AFTER calling AS_MEfree.
*/
DB_STATUS
urs_release_parm(URS_MGR_CB	*ursm,
	         URSB		*ursb)
{
	FAS_SRVR	*fas = ursm->ursm_srvr;
	ULM_RCB		*ulm = &ursb->ursb_ulm;
	DB_STATUS	status = E_DB_OK;
	STATUS		s = OK;

	if (ursb->ursb_parm)
	{
	    PTR		ptr = (PTR)ursb->ursb_parm->pOutValues;
	    FAS_METHOD	*method = (FAS_METHOD *)ursb->ursb_method;
	    FAS_APPL	*appl = NULL;

	    ursb->ursb_parm->pOutValues = (IIAPI_DATAVALUE *)NULL;
	    ursb->ursb_parm->pDescs = (IIAPI_DESCRIPTOR *)NULL;
	    ursb->ursb_parm->pInValues = (IIAPI_DATAVALUE *)NULL;
	    if ((method) && (method->fas_method_intfc))
	    	appl = method->fas_method_intfc->fas_intfc_appl;
	    if ((ptr) && (appl))
	    {	
/*FIXME: Temporarily, treat Frontier as special... need to fix C/C++ parms!   */
	    	if ((appl->fas_appl_type == FAS_APPL_CPP) &&
		    (STcompare(ursb->ursb_parm->interfacename, "Frontier") != 0))
		{
		    IIAPI_DATAVALUE	*parm;

		    for (parm = (IIAPI_DATAVALUE *)ptr;
	     		 parm < (IIAPI_DATAVALUE *)ptr + 
			 	method->fas_method_outargcount;
	     		 parm++)
		    {
	    		if (parm->dv_value)
			    MEfree(parm->dv_value);
		    }
		}	
	    	if ((appl->fas_appl_type == FAS_APPL_OPENROAD) &&
		    (appl->fas_appl_addr[AS_MEFREE]))
	    	{
		    int	(*pAS_MEfree) ();

		    pAS_MEfree = (int (*)()) appl->fas_appl_addr[AS_MEFREE];
		    s = (*pAS_MEfree) (ptr);
	            if (s != OK)
	            {
		    	urs_error(E_UR0303_MEFREE_ERROR, URS_INTERR, 0);
	            	SET_URSB_ERROR(ursb, E_UR0203_URS_FREE_PARM_ERR, 
				       E_DB_ERROR)
	            }
	    	}
	    }
	}

	/* -----------------4/13/98 11:20AM------------------
	** FIXME: Release the one-at-a-time application semaphore.
	** --------------------------------------------------*/
	CSv_semaphore( &fas->fas_srvr_applsem );

	/*
	** De-Allocate memory stream for the interface parameter list.
	*/
	if ((status = ulm_closestream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0313_ULM_CLOSESTREAM_ERROR, URS_INTERR, 1,
		      sizeof(ulm->ulm_error.err_code),
		      &ulm->ulm_error.err_code);
	    status = E_DB_ERROR;
	    SET_URSB_ERROR(ursb, E_UR0203_URS_FREE_PARM_ERR, status)
	}

	return status;
}

/*{
** Name: urs_get_type	Get a parameter's type from the parameter list
**
** Description:
** 	Uses the information passed in the parameter list of the II_FAS_DATASET
** 	parameter to add the type to the DB_DATA_VALUE struct.
**
** Inputs:
** 	ursb
** 	    .ursb_parm
** 		.pDescs			Array of IIAPI descriptor pointers.
** 		.pOutValues		Array of IIAPI value pointers.
**
** Outputs:
** 	ursb
** 	    .ursb_parm
** 		.data_value		The DB_DATA_VALUE type description.
** 		.error			DB_ERROR, if return code not OK
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**      21-Mar-1998 wonst02
**          Original Interface layer.
** 	07-Apr-1998 wonst02
** 	    Convert to IIAPI descriptors and values.
*/
DB_STATUS
urs_get_type(URSB		*ursb)
{
	DB_DATA_VALUE	*dbdv;
	FAS_ARG		*arg;
	IIAPI_DESCRIPTOR*apids;
	ULM_RCB		*ulm = &ursb->ursb_ulm;

	if ((ursb->ursb_parm == NULL) || (ursb->ursb_parm->pOutValues == NULL))
	{
	    urs_error(E_UR0402_URS_GET_TYPE_NULL, URS_CALLERR, 2,
		      sizeof ursb->ursb_parm, ursb->ursb_parm,
		      sizeof ursb->ursb_parm->pOutValues,
		      (ursb->ursb_parm) ? ursb->ursb_parm->pOutValues :
			(IIAPI_DATAVALUE *)NULL);
	    SET_URSB_ERROR(ursb, E_UR0403_URS_GET_TYPE_USAGE, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	dbdv = &ursb->ursb_parm->data_value;
	/* Ptr to next output parm */
	arg = lookup_arg((FAS_METHOD *)ursb->ursb_method, 
			 ursb->ursb_parm->sNumDescs, FAS_ARG_OUT);
	if (arg == NULL)
	{
	    SET_URSB_ERROR(ursb, E_UR0403_URS_GET_TYPE_USAGE, E_DB_ERROR)
	    return E_DB_ERROR;
	}
	apids = ursb->ursb_parm->pDescs + arg->fas_arg_id - 1;
	cnvtDescr2GCA(apids, dbdv);
		
	return E_DB_OK;
}

/*{
** Name: urs_refresh_intfc - Refresh (Stop/Start) the interface
**
** Description:
**      Stop and restart the interface, getting new information from repository.
** 
**  1.	Search for the interface name in the internal list.
**  2.	Quiesce the interface, i.e., no transactions can be executing any
** 	method within the interface when stopping the interface.
**  3.	Call urs_shut_intfc to stop the interface (via AS_Terminate).
**  4.	Read the new definition of the interface from the repository and
** 	add or replace the information in the interface control block.
**  5.	Call urs_start_intfc to start the interface again (via AS_Initiate).
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				URS Request Block
** 	    .ursb_parm
** 		.interfacename		Name of interface to be refreshed
**
** Outputs:
**
**	Returns:
** 	    E_DB_OK			Interface refreshed OK
** 	    E_DB_ERROR			Error getting interfaces definition
** 					(ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	10-Apr-1998 wonst02
** 	    Add URS_REFRESH_INTFC.
*/
DB_STATUS
urs_refresh_intfc(URS_MGR_CB	*ursm,
		  URSB		*ursb)
{
	FAS_METHOD	*method = (FAS_METHOD *)ursb->ursb_method;
	FAS_SRVR	*fas = ursm->ursm_srvr;
	DB_STATUS	status = E_DB_OK;

/*FIXME: Temporarily shut down the whole mess and start it back up.*/
	/*
	**  1.	Search for the interface name in the internal list.
	*/
	CSp_semaphore( TRUE, &fas->fas_srvr_applsem );
	status = urs_shutdown(ursb);

	/*
	**  2.	Quiesce the interface, i.e., no transactions can be executing 
	** 	any method within the interface when stopping the interface.
	*/
	/* -----------------4/13/98 11:23AM------------------
	** FIXME: By holding the fas_srvr_applsem, we have
	** already quiesced the interface. When W4GL can run
	** more than one app at a time, change this.
	** --------------------------------------------------*/

	/*
	**  3.	Call urs_shut_intfc to stop the interface (via AS_Terminate).
	*/
	
	/*
	**  4.	Read the new definition of the interface from the repository and
	** 	add or replace the information in the interface control block.
	*/

	/*
	**  5.	Call urs_start_intfc to start the interface (via AS_Initiate).
	*/
	if (! DB_FAILURE_MACRO(status) )
	    status = urs_startup(ursb);

	/*
	** Release the semaphore for the application list.
	*/
	CSv_semaphore( &fas->fas_srvr_applsem );
	if (DB_FAILURE_MACRO(status))
	    return status;

	return E_DB_OK;
}

/*{
** Name: 
** 	urs_get_intfc	Get the interface def'ns
**
** Description:
** 	Get the interface definitions and pass back the interface name, path, 
** 	and parm in the URS request block as multiple rows.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				URS Request Block ptr
**
** Outputs:
** 	ursb				URS Request Block,
** 	    .ursb_parm			  containing:
**		.sNumOutValues		    number of output values
**		.pOutValues		    array of output value ptrs
**
**	Returns:
**		E_DB_OK			Info returned OK
**		E_DB_ERROR		Parameter could not be built... more
** 					  info in the ursb_error field.
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	16-jun-1998 wonst02
** 	    Original
*/
DB_STATUS
urs_get_intfc(URS_MGR_CB	*ursm,
	      URSB		*ursb)
{
	FAS_APPL	*appl;
	FAS_INTFC	*intfc;
	URSB_PARM	*parm = ursb->ursb_parm;
	DB_STATUS	status;

	ursb->ursb_flags = URSB_OUTPUT;  /* All output parameters */	
	for (appl = ursm->ursm_srvr->fas_srvr_appl;
	     appl;
	     appl = appl->fas_appl_next)
	{
	    for (intfc = appl->fas_appl_intfc; 
	    	 intfc; 
		 intfc = intfc->fas_intfc_next)
	    {	
		/*
		** Add the output parm information
		*/
		parm->data_value.db_data = (PTR)intfc->fas_intfc_name.db_name;
		parm->data_value.db_length = (i4)
			sizeof intfc->fas_intfc_name.db_name;
		parm->data_value.db_datatype = (DB_DT_ID)DB_CHA_TYPE;
		parm->data_value.db_prec = (i2)0;

		status = urs_add_parm(ursb);
		if (status)
	    	    return E_DB_ERROR;

		parm->data_value.db_data = (PTR)intfc->fas_intfc_loc.string;
		parm->data_value.db_length = (i4)
			STlength(intfc->fas_intfc_loc.string);
		parm->data_value.db_datatype = (DB_DT_ID)DB_CHR_TYPE;
		parm->data_value.db_prec = (i2)0;

		status = urs_add_parm(ursb);
		if (status)
	    	    return E_DB_ERROR;

		parm->data_value.db_data = (PTR)intfc->fas_intfc_parm;
		parm->data_value.db_length = (i4)
			STlength(intfc->fas_intfc_parm);
		parm->data_value.db_datatype = (DB_DT_ID)DB_CHR_TYPE;
		parm->data_value.db_prec = (i2)0;

		status = urs_add_parm(ursb);
		if (status)
	    	    return E_DB_ERROR;
	    }
	}

	return E_DB_OK;
}

/*{
** Name:
**	urs_get_method	Get the methods for an interface
**
** Description:
** 	Get the methods for an interface and return the method names and
** 	numbers of arguments in the URS request block as multiple rows.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				URS Request Block ptr
** 	    .ursb_parm			  containing:
**		.pInValues		    ptr to interface name argument.
**
** Outputs:
** 	ursb				URS Request Block,
** 	    .ursb_parm			  containing:
**		.sNumOutValues		    number of output values,
**		.pOutValues		    array of output value ptrs.
**
**	Returns:
**		E_DB_OK			Info returned OK
** 		E_DB_WARN		Interface name not found
**		E_DB_ERROR		Parameter could not be built... more
** 					  info in the ursb_error field.
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	24-jun-1998 wonst02
** 	    Original
*/
DB_STATUS
urs_get_method(URS_MGR_CB	*ursm,
	       URSB		*ursb)
{
	FAS_INTFC	*intfc;
	FAS_METHOD	*method;
	URSB_PARM	*parm = ursb->ursb_parm;
	FAS_INTFC_NAME	intfclookup;
	DB_STATUS	status;

	MEcopy((char *)parm->pInValues->dv_value + 2,	/* Use ll as length   */
	       *(u_i2 *)parm->pInValues->dv_value,	/*   of varchar       */
	       intfclookup.db_name);
	MEfill(sizeof intfclookup.db_name - *(u_i2 *)parm->pInValues->dv_value,
	       ' ', intfclookup.db_name + *(u_i2 *)parm->pInValues->dv_value);
	
	intfc = lookup_intfc(ursm, ursb, intfclookup, FALSE);
	if (intfc == NULL)
	    return E_DB_WARN;

	ursb->ursb_flags = URSB_OUTPUT;
	for (method = intfc->fas_intfc_method;
	     method;
	     method = method->fas_method_next)
	{
	    /*
	    ** Add the output parm information
	    */
	    parm->data_value.db_data = (PTR)method->fas_method_name.db_name;
	    parm->data_value.db_length = (i4)
	    	sizeof method->fas_method_name.db_name;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_CHA_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;

	    parm->data_value.db_data = (PTR)&method->fas_method_argcount;
	    parm->data_value.db_length = (i4)sizeof method->fas_method_argcount;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	}

	return E_DB_OK;
}

/*{
** Name:
**	urs_get_arg	Get the next argument def'n
**
** Description:
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				URS Request Block
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	16-jun-1998 wonst02
** 	    Original
*/
DB_STATUS
urs_get_arg(URS_MGR_CB	*ursm,
	    URSB	*ursb)
{
	FAS_METHOD	*method;
	FAS_ARG		*arg;
	URSB_PARM	*parm = ursb->ursb_parm;
	FAS_INTFC_NAME	intfclookup;
	FAS_METHOD_NAME	methodlookup;
	IIAPI_DT_ID     ds_dataType;
	II_BOOL		ds_nullable;
	II_UINT2 	ds_length;
	II_INT2 	ds_precision;
	II_INT2 	ds_scale;
	II_INT2 	ds_columnType;
	DB_STATUS	status;

	MEcopy((char *)parm->pInValues->dv_value + 2,	/* Use ll as length   */
	       *(u_i2 *)parm->pInValues->dv_value,      /*   of varchar       */
	       intfclookup.db_name);
	MEfill(sizeof intfclookup.db_name - *(u_i2 *)parm->pInValues->dv_value,
	       ' ', intfclookup.db_name + *(u_i2 *)parm->pInValues->dv_value);
	MEcopy((char *)(parm->pInValues+1)->dv_value + 2, /* Use ll as length */ 
	       *(u_i2 *)(parm->pInValues+1)->dv_value,    /*   of varchar     */
	       methodlookup.db_name);
	MEfill(sizeof methodlookup.db_name - *(u_i2 *)(parm->pInValues+1)->dv_value,
	       ' ', methodlookup.db_name + *(u_i2 *)(parm->pInValues+1)->dv_value);
	
	method = lookup_method(ursm, ursb, intfclookup, methodlookup);
	if (method == NULL)
	    return E_DB_WARN;

	/*
	** Add the output parm information
	*/
	ursb->ursb_flags = URSB_OUTPUT;
	for (arg = method->fas_method_arg;
	     arg;
	     arg = arg->fas_arg_next)
	{
	    /* ArgName */
	    parm->data_value.db_data = (PTR)arg->fas_arg_name.db_name;
	    parm->data_value.db_length = (i4)
	    	sizeof arg->fas_arg_name.db_name;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_CHA_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_dataType */
	    ds_dataType = (arg->fas_arg_data.db_datatype >= 0) ?
	    		   arg->fas_arg_data.db_datatype :
	    		   arg->fas_arg_data.db_datatype * -1;
	    parm->data_value.db_data = (PTR)&ds_dataType;
	    parm->data_value.db_length = (i4)sizeof ds_dataType;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_nullable */
	    ds_nullable = (arg->fas_arg_data.db_datatype >= 0) ? 0 : 1;
	    parm->data_value.db_data = (PTR)&ds_nullable;
	    parm->data_value.db_length = (i4)sizeof ds_nullable;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_length */
	    ds_length = (II_UINT2)arg->fas_arg_data.db_length;
	    parm->data_value.db_data = (PTR)&ds_length;
	    parm->data_value.db_length = (i4)sizeof ds_length;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_precision */
	    ds_precision = DB_P_DECODE_MACRO(arg->fas_arg_data.db_prec);
	    parm->data_value.db_data = (PTR)&ds_precision;
	    parm->data_value.db_length = (i4)sizeof ds_precision;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_scale */
	    ds_scale = DB_S_DECODE_MACRO(arg->fas_arg_data.db_prec);
	    parm->data_value.db_data = (PTR)&ds_scale;
	    parm->data_value.db_length = (i4)sizeof ds_scale;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	    /* ds_columnType */
	    ds_columnType = (II_INT2)arg->fas_arg_type;
	    parm->data_value.db_data = (PTR)&ds_columnType;
	    parm->data_value.db_length = (i4)sizeof ds_columnType;
	    parm->data_value.db_datatype = (DB_DT_ID)DB_INT_TYPE;
	    parm->data_value.db_prec = (i2)0;
	    status = urs_add_parm(ursb);
	    if (status)
	        return E_DB_ERROR;
	}

	return E_DB_OK;
}	
