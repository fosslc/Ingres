/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/* [@#include@]... */

/**
**
**  Name: ADUAGLOGK.C - Aggregate functions for logical keys
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "logical key" datatypes.
**	These routines work both on the "table_key" and "object_key" data
**	types.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**          adu_B0l_minmax_logk() - Ag-begin for min(date) or max(date).
**	    adu_N5l_min_logk() - Ag-next for min(date).
**	    adu_N6l_max_logk() - Ag-next for max(date).
**          adu_E0l_minmax_logk() - Ag-end for min(date) or max(date).
**
**  History:
**	10-aug-89 (mikem)
**	    Stole code from aduagdate.c and modified for logical keys.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
[@forward_type_references@]
*/


/*
**  Forward and/or External function references.
*/

/*
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_B0l_minmax_logk() - Ag-begin for min() or max() of logical keys.
**
** Description:
**      Initializes one of min(logical_key) or max(logical_key) aggregations.
**	Handles both table_key and object_key datatypes.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ag_struct                       Ptr to the ADF_AG_STRUCT to be inited.
**	    .adf_agfi			Function instance ID of the aggregation
**					this struct is to be inited for.
**	    .adf_agwork			DB_DATA_VALUE to be used for workspace.
**		.db_length		Length of DB_DATA_VALUE.  This must be
**					at least as long as the .adi_agwsdv_len
**					field states in the function instance
**					description for this aggregate.
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
**	    E_AD2010_BAD_FIID
**	    E_AD4002_FIID_IS_NOT_AG
**	    E_AD4003_AG_WORKSPACE_TOO_SHORT
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Various fields of the ag_struct will be modified.
**
** History:
**      10-aug-89 (mikem)
**          Initial creation and coding.
**	09-mar-1999 (shero03)
**	    Support an array of datatypes
*/

DB_STATUS
adu_B0l_minmax_logk(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct)
{
    ADI_FI_ID          fid = ag_struct->adf_agfi;
    ADI_FI_DESC        *fidesc;
    DB_STATUS          db_stat;

    /* First, make sure we have a proper f.i. ID, and get the fi_desc for it */
    if (db_stat = adi_fidesc(adf_scb, fid, &fidesc))
	return(db_stat);

    /* Make sure f.i. is an aggregate */
    if (fidesc->adi_fitype != ADI_AGG_FUNC)
	return(adu_error(adf_scb, E_AD4002_FIID_IS_NOT_AG, 0));

    /* Make sure the length of the agwork DB_DATA_VALUE is long enough */
    if (ag_struct->adf_agwork.db_length < fidesc->adi_agwsdv_len)
	return(adu_error(adf_scb, E_AD4003_AG_WORKSPACE_TOO_SHORT, 0));

    /* Now set the agnext and agend functions in the ag_struct */
    ag_struct->adf_agnx =
	    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO((i4)fid)].adi_func;
    ag_struct->adf_agnd =
	    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO((i4)fid)].adi_agend;

    /* Finish the init */
    ag_struct->adf_agcnt = 0;
    ag_struct->adf_agwork.db_datatype = fidesc->adi_dt[0];

    return(E_DB_OK);
}


/*{
** Name: adu_N5l_min_logk() - Ag-next for logical_key(date).
**
** Description:
**      Compiles another "logical_key" datavalue into the min() aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_next                         Ptr to the DB_DATA_VALUE which is the
**                                      next value to aggregate.
**      ag_struct                       Ptr to the ADF_AG_STRUCT being used for
**					this aggregation.
**	    .adf_agwork			DB_DATA_VALUE to use for workspace.
**	       / .db_length		Length of DB_DATA_VALUE.
**	    +-<  .db_datatype		Datatype of DB_DATA_VALUE.
**	    |  \ .db_data  		Pointer to the data area to use as
**	    |				workspace.
**          |
**          |
**          +--- The caller should not have to worry about setting these if
**               they have not touched the ag_struct since ag-begin ... and
**               they better not have!
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
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The ag-struct and the data area pointed to by the workspace
**	    DB_DATA_VALUE may be modified.
**
** History:
**	10-aug-89 (mikem)
**	    created.
*/

DB_STATUS
adu_N5l_min_logk(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmin = &ag_struct->adf_agwork;
    DB_STATUS	    db_stat;
    i4		    cmp;


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	curmin->db_length = dv_next->db_length;
        MEcopy(dv_next->db_data, curmin->db_length, curmin->db_data); 
    }
    else
    {
	ag_struct->adf_agcnt++;

	if (db_stat = adu_1logkey_cmp(adf_scb, curmin, dv_next, &cmp))
	    return(db_stat);

	if (cmp > 0)
	{
	    MEcopy(dv_next->db_data, curmin->db_length, curmin->db_data); 
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N6l_max_logk() - Ag-next for max(logical_key).
**
** Description:
**      Compiles another "logical key" datavalue into the max() aggregation.
**	Handles both table_key and object_key data types.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_next                         Ptr to the DB_DATA_VALUE which is the
**                                      next value to aggregate.
**      ag_struct                       Ptr to the ADF_AG_STRUCT being used for
**					this aggregation.
**	    .adf_agwork			DB_DATA_VALUE to use for workspace.
**	       / .db_length		Length of DB_DATA_VALUE.
**	    +-<  .db_datatype		Datatype of DB_DATA_VALUE.
**	    |  \ .db_data  		Pointer to the data area to use as
**	    |				workspace.
**          |
**          |
**          +--- The caller should not have to worry about setting these if
**               they have not touched the ag_struct since ag-begin ... and
**               they better not have!
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
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The ag-struct and the data area pointed to by the workspace
**	    DB_DATA_VALUE may be modified.
**
** History:
**	10-aug-89 (mikem)
**	    created.
*/

DB_STATUS
adu_N6l_max_logk(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmax = &ag_struct->adf_agwork;
    DB_STATUS	    db_stat;
    i4		    cmp;


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	curmax->db_length = dv_next->db_length;
        MEcopy(dv_next->db_data, curmax->db_length, curmax->db_data); 
    }
    else
    {
	ag_struct->adf_agcnt++;

	if (db_stat = adu_1logkey_cmp(adf_scb, curmax, dv_next, &cmp))
	    return(db_stat);
	if (cmp < 0)
	{
	    MEcopy(dv_next->db_data, curmax->db_length, curmax->db_data); 
	}
    }

    return(E_DB_OK);
}

/*{
** Name: adu_E0d_minmax_logk() - Ag-end for min() or max() of logical keys.
**
** Description:
**      Returns the result of an min() or max() logical key aggregations.
**	Handles both table_key and object_key datatypes.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ag_struct                       Ptr to the ADF_AG_STRUCT being used for
**					this aggregation.
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
**      dv_result                       The result of the aggregate is placed
**					in DB_DATA_VALUE.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-aug-89 (mikem)
**	    created.
*/

DB_STATUS
adu_E0l_minmax_logk(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_DATA_VALUE   *curminmax = &ag_struct->adf_agwork;
    DB_STATUS	    db_stat;


    if (ag_struct->adf_agcnt > 0)
    {
	MEcopy(	(PTR) curminmax->db_data,
		dv_result->db_length,
		(PTR) dv_result->db_data
	      );
	db_stat = E_DB_OK;
    }
    else
    {
	db_stat = adc_getempty(adf_scb, dv_result);
    } 
    return(db_stat);
}
