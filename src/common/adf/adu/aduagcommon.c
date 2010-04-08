/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/*
[@#include@]...
*/

/**
**
**  Name: ADUAGCOMMON.C - Aggregates common to all types.
**
**  Description:
**	The aggregates "count" and "any" are common to all datatypes since they
**	are not type dependent.  The functions in this file implement these
**	aggregates that are common to all datatypes, as well as a general
**	purpose aggregate initializer.
**
**	The naming convention used for routine names is "adu_@$?_descriptive()"
**	where:
**		"@" is a capital letter as follows:
**			"B" = an ag-begin routine
**			"N" = an ag-next routine
**			"E" = an ag-end routine
**		"$" is a digit corresponding to the aggr operation as follows:
**			"1" = any
**			"2" = count or countu
**			"3" = avg or avgu
**			"4" = sum or sumu
**			"5" = min
**			"6" = max
**			"0" = (generally usefull)
**		"?" is a letter corresponding to the datatype(s) as follows:
**			"f" = f
**			"n" = decimal ("n" stands for "numeric")
**			"i" = i
**			"m" = money
**			"d" = date
**			"s" = c or text
**			"a" = any datatype
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**	    adu_B0a_noagwork() - Ag-begin for all aggregates that do not use
**				 extra work space.
**          adu_N2a_count() - Ag-next for "count(u)" aggregate for all
**			      datatypes.
**          adu_N1a_any() - Ag-next for "any" aggregate for all datatypes.
**          adu_E0a_anycount() - Ag-end for "count(u)" or "any" aggregates.
[@func_list@]...
**
**
**  History:
**      27-jun-86 (thurston)    
**          Modified to jupiter standards.
**	02-sep-86 (thurston)
**	    Had to fix problem with the ag-end routine for any or count (routine
**	    "adu_E0a_anycount()") whereby this routine was always placing the
**	    result into an i1.  This is fine for "any", but "count" requires
**	    an i4.  Now a check is made on the result data value's length to
**	    determine what to cast the result to.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru Adf_globs
**	    for user defined ADT support.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**      24-mar-1993 (smc)
**          Commented out text after endifs.
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
*/
/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_N2a_count() - Ag-next for "count(u)" aggregate for all datatypes.
**
** Description:
**      This is the "count" aggregate function for all datatypes. 
**      It will use the .adf_agcnt field of the ADF_AG_STRUCT to maintain 
**      the number of data values aggregated.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_next                         Ptr to the DB_DATA_VALUE which is the
**                                      next value to aggregate.  (This is not
**					even looked at by this function.)
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
**
**      Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    Could incur an integer overflow if more values are aggregated than
**	    can be counted in an i4.
**
** Side Effects:
**	    ag_struct->adf_agcnt will be incremented.
**
** History:
**	27-jun-86 (thurston)
**          Modified to jupiter standards.
*/

DB_STATUS
/*ARGSUSED*/
adu_N2a_count(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,   /* count actually doesn't care about the dv_next */
ADF_AG_STRUCT	*ag_struct)
{
    EXmath((i4)EX_ON);

    ag_struct->adf_agcnt++;	/* Increment the counter in the ag_struct */

    EXmath((i4)EX_OFF);
    return(E_DB_OK);
}


/*{
** Name: adu_N1a_any() - Ag-next for "any" aggregate for all datatypes.
**
** Description:
**      This is the "any" aggregate function for all datatypes. 
**      It will set the .adf_agcnt field of the ADF_AG_STRUCT to 1
**      to note that there has been at least one data value aggregated.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_next                         Ptr to the DB_DATA_VALUE which is the
**                                      next value to aggregate.  (This is not
**					even looked at by this function.)
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
**
**      Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    ag_struct->adf_agcnt will be set to 1.
**
** History:
**	27-jun-86 (thurston)
**          Modified to jupiter standards.
*/

DB_STATUS
/*ARGSUSED*/
adu_N1a_any(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,   /* any actually doesn't care about the dv_next */
ADF_AG_STRUCT	*ag_struct)
{
    ag_struct->adf_agcnt = 1;	/* Set the counter in the ag_struct to 1 */
    return(E_DB_OK);
}


/*{
** Name: adu_B0a_noagwork() - Ag-begin for all aggregates that do not use extra
**			    work space.
**
** Description:
**      This routine initializes all aggregates that do not require extra work
**      space to be processed.  (i.e. the do not need a ptr to a DB_DATA_VALUE
**      in the ag-struct.) 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ag_struct                       Ptr to an ADF_AG_STRUCT to be inited.
**	    .adf_agfi			Function instance ID of the aggregation
**					this struct is to be inited for.
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
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Various fields of the ag_struct will be modified.
**
** History:
**	27-jun-86 (thurston)
**          Modified to jupiter standards.
**	7-Nov-2005 (schka24)
**	    Init new i8 work cell, init other "reserved" properly.
*/

DB_STATUS
adu_B0a_noagwork(
ADF_CB             *adf_scb,
ADF_AG_STRUCT      *ag_struct)
{
    ADI_FI_ID          fid = ag_struct->adf_agfi;
    ADI_FI_DESC        *fidesc;
    DB_STATUS          db_stat;
    i4                 i;

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
    ag_struct->adf_ag8rsv = 0;
    for(i=0; i<3; i++)
    {
	ag_struct->adf_agirsv[i] = 0;
	ag_struct->adf_agfrsv[i] = 0.0;
    }
    ag_struct->adf_agfrsv[3] = ag_struct->adf_agfrsv[4] = 0.0;

    return(E_DB_OK);
}

/*{
** Name: adu_E0a_anycount() - Ag-end for "count(u)" or "any" aggregates.
**
** Description:
**      This routine finalizes the "count" or "any" aggregates setting the
**	result into the supplied DB_DATA_VALUE.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ag_struct                       Ptr to an ADF_AG_STRUCT to be inited.
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
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK
**
**		The following returns will only happen when the system is
**		compiled with the xDEBUG option:
**
**          E_AD4004_BAD_AG_DTID
**          E_AD4005_NEG_AG_COUNT
**          E_AD9999_INTERNAL_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-jul-86 (thurston)
**          Initially written.
**	02-sep-86 (thurston)
**	    Had to fix problem whereby this routine was always placing the
**	    result into an i1.  This is fine for "any", but "count" requires
**	    an i4.  Now a check is made on the result data value's length to
**	    determine what to cast the result to.
*/

DB_STATUS
/*ARGSUSED*/
adu_E0a_anycount(
ADF_CB		   *adf_scb,	    /* Only used when compiled with xDEBUG */
ADF_AG_STRUCT      *ag_struct,
DB_DATA_VALUE      *dv_result)
{
    DB_STATUS          db_stat = E_DB_OK;
#ifdef xDEBUG
    ADI_FI_ID          fid = ag_struct->adf_agfi;
    ADI_FI_DESC        *fidesc;
    ADI_OP_ID          opid;

    for (;;)	/* something to break out of */
    {
	/* First, check we have a proper f.i. ID, and get the fi_desc for it */
	if (db_stat = adi_fidesc(adf_scb, fid, &fidesc))
	    break;

	/* Make sure f.i. is either ADI_ANY_OP, ADI_CNT_OP, or ADI_CNTAL_OP */
	if (   (opid = fidesc->adi_fiopid) != ADI_ANY_OP
	    &&  opid != ADI_CNT_OP
	    &&  opid != ADI_CNTAL_OP
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	/* Check supplied datatype ... both ANY and COUNT return "i" */
	if (dv_result->db_datatype != DB_INT_TYPE)
	{
	    db_stat = adu_error(adf_scb, E_AD4004_BAD_AG_DTID, 0);
	    break;
	}

	/* Check supplied length ... ANY returns a 1, COUNT a 4 */
	if (dv_result->db_length != (opid == ADI_ANY_OP ? 1 : 4))
	{
	    db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	/* Make sure the .adf_agcnt field is not negative */
	if (ag_struct->adf_agcnt < 0)
	{
	    db_stat = adu_error(adf_scb, E_AD4005_NEG_AG_COUNT, 0);
	    break;
	}

	db_stat = E_DB_OK;
	break;
    }


    if (DB_SUCCESS_MACRO(db_stat))
#endif /* xDEBUG */
    {
	switch (dv_result->db_length)
	{
	  case 1:
	    *(i1 *)dv_result->db_data = ag_struct->adf_agcnt;
	    break;
	  case 2:
	    *(i2 *)dv_result->db_data = ag_struct->adf_agcnt;
	    break;
	  case 4:
	    *(i4 *)dv_result->db_data = ag_struct->adf_agcnt;
	    break;
	  default:
	    db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	}
    }

    return (db_stat);
}
/*
[@function_definition@]...
*/
