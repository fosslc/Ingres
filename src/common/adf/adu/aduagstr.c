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
**  Name: ADUAGSTR.C - Aggr. funcs for strings (c, text, char, varchar).
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "c", "text", "char", and "varchar"
**      datatypes.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**          adu_B0s_minmax_str() - Ag-begin for min(c), max(c), min(text),
**				   max(text), min(char) or max(char),
**				   min(varchar) or max(varchar).
**	    adu_N5s_min_str() - Ag-next for min(c), min(text), min(char),
**				or min(varchar).
**	    adu_N6s_max_str() - Ag-next for max(c), max(text), max(char),
**				or max(varchar).
**          adu_E0s_minmax_str() - Ag-end for min(c), max(c), min(text),
**				   max(text), min(char) or max(char),
**				   min(varchar) or max(varchar).
[@func_list@]...
**
**
**  History:
**      07-aug-86 (thurston)
**          Converted to Jupiter standards.
**	29-sep-86 (thurston)
**	    Changed comments to reflect the fact that all of these routines
**	    will now work on "char" and "varchar" as well as "c" and "text".
**	    (This is because all of the low level routines that these routines
**	    use now work this way.)
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**	21-apr-87 (thurston)
**	    Added `QUEL pm flag' arg to adu_lexcomp() calls.
**	09-mar-88 (thurston)
**	    Fixed bugs that could cause READ ACCESS VIOLATION if input string
**	    was not as large as the `current min' or `current max' buffer in
**	    adu_N5s_min_str() and adu_N6s_max_str().
**	13-jul-88 (thurston)
**	    Fixed a bug in adu_E0s_minmax_str() that was producing garbage
**	    results for QUEL min(str) and max(str) queries that aggregated zero
**	    rows ... the ag-work-space had never been set to anything.  Should
**	    instead, check for zero rows, and call adc_getempty() to return a
**	    default value.  (NOTE:  For SQL, this routine will not be called
**	    when zero rows have been aggregated.  The higher level
**	    ade_execute_cx() will take care of this, or the adf_agend() routine
**	    itself.  This is because for SQL, the NULL value should be returned
**	    in these situations, not the default value.)
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru Adf_globs
**	    for UDADT support.
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
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_B0s_minmax_str() - Ag-begin for min(c), max(c), min(text),
**				max(text), min(char), max(char),
**				min(varchar), or max(varchar).
**
** Description:
**      Initializes one of min(c), max(c), min(text), max(text), min(char),
**	max(char), min(varchar), or max(varchar) aggregations.
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
**      07-aug-86 (thurston)
**          Initial creation and coding.
**	23-mar-91 (jrb)
**	    Removed check to insure workspace in agstruct was sufficient.
**	    This is because we now adjust the db_length in the agstruct in
**	    the ag_next action, and some queries cause ag_begin to be run
**	    more than once with the same ag_struct which invoked this error.
**	    This is part of the fix for bug 34502.
*/

DB_STATUS
adu_B0s_minmax_str(
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

    /* Now set the agnext and agend functions in the ag_struct */
    ag_struct->adf_agnx =
	    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO((i4)fid)].adi_func;
    ag_struct->adf_agnd =
	    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO((i4)fid)].adi_agend;

    /* Finish the init */
    ag_struct->adf_agcnt = 0;
    ag_struct->adf_agwork.db_datatype = fidesc->adi_dtresult;

    return(E_DB_OK);
}


/*{
** Name: adu_N5s_min_str() - Ag-next for min(c), min(text), min(char), or
**			     min(varchar).
**
** Description:
**      Compiles another "c", "text", "char", or "varchar" datavalue into
**	the min(c), min(text), min(char), or min(varchar) aggregation.
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
**      07-aug-86 (thurston)
**          Converted for Jupiter.
**	21-apr-87 (thurston)
**	    Added `QUEL pm flag' arg to adu_lexcomp() call.
**	09-mar-88 (thurston)
**	    Fixed bug that could cause READ ACCESS VIOLATION if input string
**	    was not as large as the `current min' buffer.
**	23-mar-91 (jrb)
**	    Changed db_length in agstruct to be length of value pointed to
**	    by db_data pointer.  Before, we left it as the size of the work-
**	    space, but this caused problems in the comparison routine.
**	    Bug 34502.
*/

DB_STATUS
adu_N5s_min_str(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmin = &ag_struct->adf_agwork;
    DB_STATUS	    db_stat;
    i4		    cmp;
    u_i2	    cplen;

    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	cplen = min(curmin->db_length, dv_next->db_length);
        MEcopy(dv_next->db_data, cplen, curmin->db_data);
	curmin->db_length = cplen;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if (db_stat = adu_lexcomp(adf_scb, (i4) FALSE, curmin, dv_next, &cmp))
	    return(db_stat);
	if (cmp > 0)
	{
	    cplen = min(curmin->db_length, dv_next->db_length);
	    MEcopy(dv_next->db_data, cplen, curmin->db_data);
	    curmin->db_length = cplen;
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N6s_max_str() - Ag-next for max(c), max(text), max(char), or
**			     max(varchar).
**
** Description:
**      Compiles another "c", "text", "char", or "varchar" datavalue into the
**	max(c), max(text), max(char), or max(varchar) aggregation.
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
**          +--> The caller should not have to worry about setting these if
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
**      07-aug-86 (thurston)
**          Converted for Jupiter.
**	21-apr-87 (thurston)
**	    Added `QUEL pm flag' arg to adu_lexcomp() call.
**	09-mar-88 (thurston)
**	    Fixed bug that could cause READ ACCESS VIOLATION if input string
**	    was not as large as the `current max' buffer.
**	23-mar-91 (jrb)
**	    Changed db_length in agstruct to be length of value pointed to
**	    by db_data pointer.  Before, we left it as the size of the work-
**	    space, but this caused problems in the comparison routine.
**	    Bug 34502.
*/

DB_STATUS
adu_N6s_max_str(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmax = &ag_struct->adf_agwork;
    DB_STATUS	    db_stat;
    i4		    cmp;
    u_i2	    cplen;

    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	cplen = min(curmax->db_length, dv_next->db_length);
        MEcopy(dv_next->db_data, cplen, curmax->db_data); 
	curmax->db_length = cplen;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if (db_stat = adu_lexcomp(adf_scb, (i4) FALSE, curmax, dv_next, &cmp))
	    return(db_stat);
	if (cmp < 0)
	{
	    cplen = min(curmax->db_length, dv_next->db_length);
	    MEcopy(dv_next->db_data, cplen, curmax->db_data);
	    curmax->db_length = cplen;
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E0s_minmax_str() - Ag-end for min(c), max(c), min(text),
**				max(text), min(char), max(char),
**				min(varchar), or max(varchar).
**
** Description:
**      Returns the result of a min(c), max(c), min(text), max(text), min(char),
**	max(char), min(varchar), or max(varchar) aggregation.
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
**      07-aug-86 (thurston)
**          Initial creation and coding.
**	13-jul-88 (thurston)
**	    Fixed a bug that was producing garbage results for QUEL min(str) and
**	    max(str) queries that aggregated zero rows ... the ag-work-space had
**	    never been set to anything.  Should instead, check for zero rows,
**	    and call adc_getempty() to return a default value.  (NOTE:  For SQL,
**	    this routine will not be called when zero rows have been aggregated.
**	    The higher level ade_execute_cx() will take care of this, or the
**	    adf_agend() routine itself.  This is because for SQL, the NULL value
**	    should be returned in these situations, not the default value.)
*/

DB_STATUS
adu_E0s_minmax_str(
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
    return (db_stat);
}

/*
[@function_definition@]...
*/
