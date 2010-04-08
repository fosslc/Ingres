/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADGFEXI.C - Routine to manipulate FEXI table
**
**  Description:
**      This file contains all of the routines necessary for manipulating
**	the FEXI (Function EXternally Implemented) table.  This table contains
**	the address of the external function call which is used to
**	compute this operation.  It is assumed that ADF has already been
**	started (i.e. adg_startup has been called previously).
**
**	FEXIs provide a mechanism for allowing calls outside of ADF by
**	specifying the routine at run-time rather than at compile-time (the
**	latter would require linking in routines outside of ADF and the CL).
**	Certain ADF operations (e.g. resolve_table()) expect that a FEXI
**	entry will exist.
**
**	This file defines the following externally visible routines:
**
**	    adg_add_fexi	- Add an entry to the FEXI table
**
**
**  History:
**	09-mar-92 (jrb, merged by stevet)
**	    Created.
**      12-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definition@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adg_add_fexi() - Add entry to FEXI table
**
** Description:
**	Adds the callback function's address into the specified slot of the FEXI
**	table.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adg_func_addr			The address of the function which
**					should be called to implement this
**					function.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD1009_BAD_SRVCB		Bad ADF server CB.
**	    E_AD1019_NULL_FEXI_PTR	Caller tried setting callback to NULL.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    If everything went correctly, a new row will be added to the FEXI
**	    table in the ADF server CB.
**
** History:
**	09-mar-92 (jrb, merged by stevet)
**	    Created.
*/
#ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adg_add_fexi(
ADF_CB		*adf_scb,
ADI_OP_ID	adg_index_fexi,
DB_STATUS	(*adg_func_addr)())
#else
DB_STATUS
adg_add_fexi( adf_scb, adg_index_fexi, adg_func_addr)
ADF_CB		*adf_scb;
ADI_OP_ID	adg_index_fexi;
DB_STATUS	(*adg_func_addr)();
#endif
{
    if (    Adf_globs == NULL
	||  Adf_globs->adf_type != ADSRV_TYPE
	||  Adf_globs->adf_ascii_id != ADSRV_ASCII_ID
	|| !Adf_globs->Adi_inited
	||  Adf_globs->Adi_fexi == NULL
       )
    {
	return (adu_error(adf_scb, E_AD1009_BAD_SRVCB, 0));
    }

    /* Null function pointers are bad luck */
    if (adg_func_addr == NULL)
	return (adu_error(adf_scb, E_AD1019_NULL_FEXI_PTR, 0));
	
    /* add the entry */
    Adf_globs->Adi_fexi[adg_index_fexi].adi_fcn_fexi = adg_func_addr;
    
    return (E_DB_OK);
}
