/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adudate.h>
#include    <aduint.h>
/* [@#include@]... */

/**
**
**  Name: ADUAGDATE.C - Aggregate functions for dates (the "date" datatype).
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "date" datatype.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**          adu_B0d_minmax_date() - Ag-begin for min(date) or max(date).
**	    adu_N0d_tot_date() - Ag-next for avg(date) and sum(date).
**	    adu_N5d_min_date() - Ag-next for min(date).
**	    adu_N6d_max_date() - Ag-next for max(date).
**	    adu_E3d_avg_date() - Ag-end for avg(date).
**	    adu_E4d_sum_date() - Ag-end for sum(date).
**          adu_E0d_minmax_date() - Ag-end for min(date) or max(date).
[@func_list@]...
**
**
**  History:
**      07-aug-86 (thurston)
**          Converted to Jupiter standards.
**	03-feb-87 (thurston)
**	    Replaced calls to adc_date_compare() with adu_4date_cmp().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**	13-jul-88 (thurston)
**	    Fixed a bug in adu_E0d_minmax_date() that was producing garbage
**	    results for QUEL min(date) and max(date) queries that aggregated
**	    zero rows ... the ag-work-space had never been set to anything.
**	    Should instead, check for zero rows, and call adc_getempty() to
**	    return a default value.  (NOTE:  For SQL, this routine will not be
**	    called when zero rows have been aggregated.  The higher level
**	    ade_execute_cx() will take care of this, or the adf_agend() routine
**	    itself.  This is because for SQL, the NULL value should be returned
**	    in these situations, not the default value.)
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru Adf_globs.
**	    This is done for user defined ADT support.
**	19-may-89 (jrb)
**	    Fixed bug in adu_E3d_avg_date where date status bits weren't
**	    getting set when corresponding interval fields were being filled in.
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
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL & correct struct initialisation.
**	16-Mar-2008 (kiria01) b120004
**	    As part of cleaning up timezone handling the internal date
**	    conversion routines now can detect and return errors.
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
** Name: adu_B0d_minmax_date() - Ag-begin for min(date) or max(date).
**
** Description:
**      Initializes one of min(date) or max(date) aggregations.
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
**      31-dec-1992 (stevet)
**          Added function prototypes.
*/

DB_STATUS
adu_B0d_minmax_date(
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
    ag_struct->adf_agwork.db_datatype = DB_DTE_TYPE;

    return(E_DB_OK);
}


/*{
** Name: adu_N0d_tot_date() - Ag-next for avg(date) and sum(date).
**
** Description:
**      Compiles another "date" datavalue into the avg(date) or sum(date)
**	aggregation.  We keep track of the status bits as we aggregate so
**	that the date normalization will be done correctly.  Also, if an
**	empty date occurs while processing the aggregate we record this in
**	the ag_struct because by definition the aggregate will have an
**	empty date result.
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
**	    The ag-struct may be modified.
**
** History:
**      06-nov-88 (thurston)
**          Initial creation ... coding may be done later.
**	02-dec-88 (jrb)
**	    Initial coding.
**	04-jan-89 (jrb)
**	    Changed the way this works to conform to LRC decisions regarding
**	    empty dates in SUM() and AVG().
**	31-oct-2006 (gupsh01)
**	    Modified to support new internal format for Dates.
**	16-Apr-2008 (kschendel)
**	    Count up "any-warnings" too.
*/

DB_STATUS
adu_N0d_tot_date(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_STATUS		db_stat;
    i4			*istat    = &ag_struct->adf_agirsv[0];
    i4			*emptyres = &ag_struct->adf_agirsv[1];
    i4			*imonths  = &ag_struct->adf_agirsv[2];
    f8			*isecs	  = &ag_struct->adf_agfrsv[0];
    AD_NEWDTNTRNL	datenext;
    register i1		status;
    
    if (db_stat = adu_6to_dtntrnl (adf_scb, dv_next, &datenext))
	return (db_stat);
    if (datenext.dn_status & AD_DN_ABSOLUTE)
    {
	/* no absolute dates allowed--increment warning count and return */
	adf_scb->adf_warncb.ad_agabsdate_cnt++;
	adf_scb->adf_warncb.ad_anywarn_cnt++;

	return(E_DB_OK);
    }

    /* we have a valid date value--increment row counter */
    ag_struct->adf_agcnt++;

    status = datenext.dn_status;
    *istat |= (i4)status;

    /* check for empty date and return if we have it */
    if (status == AD_DN_NULL)
    {
	/* emptyres will start out with a 0 value; if we encounter an empty */
	/* date we set it to 1						    */
	*emptyres = 1;
	
	return(E_DB_OK);
    }

    /* accumulate next value */

    if (status & AD_DN_YEARSPEC)
	*imonths += datenext.dn_year * 12;

    if (status & AD_DN_MONTHSPEC)
	*imonths += datenext.dn_month;

    if (status & AD_DN_DAYSPEC)
    {
	/* the following is tricky: if we omit the (i4) cast then inside    */
	/* the parens will be an i4 plus a u_i2 which, by definition gives  */
	/* a u_i4.  If this gets casted to an f8 and added to isecs, we    */
	/* will get weird results.  (This case arises when we have negative */
	/* days in a date interval.)					    */
	*isecs += (i4)(I1_CHECK_MACRO(datenext.dn_day)) * AD_40DTE_SECPERDAY;
    }

    if (status & AD_DN_TIMESPEC)
	*isecs += (f8)datenext.dn_seconds;

    return(E_DB_OK);
}


/*{
** Name: adu_N5d_min_date() - Ag-next for min(date).
**
** Description:
**      Compiles another "date" datavalue into the min(date) aggregation.
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
**	03-feb-87 (thurston)
**	    Replaced call to adc_date_compare() with adu_4date_cmp().
**	31-oct-2006 (gupsh01)
**	    Modified to support new internal format for Dates.
*/

DB_STATUS
adu_N5d_min_date(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmin = &ag_struct->adf_agwork;
    AD_NEWDTNTRNL    datemin;
    AD_NEWDTNTRNL    datenext;
    DB_STATUS	    db_stat;
    i4		    cmp;


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
        MEcopy(dv_next->db_data, curmin->db_length, curmin->db_data); 
    }
    else
    {
	ag_struct->adf_agcnt++;
	

        if (db_stat = adu_6to_dtntrnl (adf_scb, curmin, &datemin))
	    return (db_stat);

        if (db_stat = adu_6to_dtntrnl (adf_scb, dv_next, &datenext))
	    return (db_stat);

	/* check for NULL dates compared */
	if (	datenext.dn_status != AD_DN_NULL
	    &&	datemin.dn_status != AD_DN_NULL
	   )
	{
	    if (db_stat = adu_4date_cmp(adf_scb, curmin, dv_next, &cmp))
		return(db_stat);
	    if (cmp > 0)
	    {
		MEcopy(dv_next->db_data, curmin->db_length,
							    curmin->db_data); 
	    }
	}
	else
	{
	    if (datemin.dn_status == AD_DN_NULL)
	    {
		MEcopy(dv_next->db_data, curmin->db_length,
							    curmin->db_data); 
	    }
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N6d_max_date() - Ag-next for max(date).
**
** Description:
**      Compiles another "date" datavalue into the max(date) aggregation.
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
**	03-feb-87 (thurston)
**	    Replaced call to adc_date_compare() with adu_4date_cmp().
**	31-oct-2006 (gupsh01)
**	    Modified to support new internal format for Dates.
*/

DB_STATUS
adu_N6d_max_date(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curmax = &ag_struct->adf_agwork;
    AD_NEWDTNTRNL    datemax;
    AD_NEWDTNTRNL    datenext;
    DB_STATUS	    db_stat;
    i4		    cmp;


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
        MEcopy(dv_next->db_data, curmax->db_length, curmax->db_data); 
    }
    else
    {
	ag_struct->adf_agcnt++;

        if (db_stat = adu_6to_dtntrnl (adf_scb, curmax, &datemax))
	    return (db_stat);

        if (db_stat = adu_6to_dtntrnl (adf_scb, dv_next, &datenext))
	    return (db_stat);

	/* check for NULL dates compared */
	if (	datenext.dn_status != AD_DN_NULL
	    &&	datemax.dn_status != AD_DN_NULL
	   )
	{
	    if (db_stat = adu_4date_cmp(adf_scb, curmax, dv_next, &cmp))
		return(db_stat);
	    if (cmp < 0)
	    {
		MEcopy(dv_next->db_data, curmax->db_length,
							    curmax->db_data); 
	    }
	}
	else
	{
	    if (datemax.dn_status == AD_DN_NULL)
	    {
		MEcopy(dv_next->db_data, curmax->db_length,
							    curmax->db_data); 
	    }
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E3d_avg_date() - Ag-end for avg(date).
**
** Description:
**      Returns the result of an avg(date) aggregation.
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
**      06-nov-88 (thurston)
**          Initial creation ... coding may be done later.
**	02-dec-88 (jrb)
**	    Initial coding.
**	04-jan-89 (jrb)
**	    Changed the way this works to conform to LRC decisions regarding
**	    empty dates in AVG().
**	19-may-89 (jrb)
**	    Fixed bug where date status bits weren't getting set when
**	    corresponding interval fields were being filled in.
**	31-oct-2006 (gupsh01)
**	    Modified to support new internal format for Dates.
**	10-Feb-2008 (kiria01) b119885
**	    Also ensure that the res structure is fully iniialised.
**       2-Apr-2009 (hanal04) Bug 121881
**          We can not call adu_6to_dtntrnl() to initialise res. We don't
**          have an input value. dv_result is an address to put the 
**          aggregate result not an input value.
*/

DB_STATUS
adu_E3d_avg_date(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_STATUS		db_stat;
    i4			*istat    = &ag_struct->adf_agirsv[0];
    i4			*emptyres = &ag_struct->adf_agirsv[1];
    i4			*imonths  = &ag_struct->adf_agirsv[2];
    f8			*isecs	  = &ag_struct->adf_agfrsv[0];
    AD_NEWDTNTRNL	res;
    i4			mremain;
    i4			days;
    f8			ftemp;

    MEfill ((u_i2)sizeof(res), NULLCHAR, (PTR) &res);

    /* if no rows aggregated or if empty date was in any row then we return */
    /* the empty date; if we were in SQL and no rows were aggregated this   */
    /* routine would never get called and the result would be NULL	    */

    if (ag_struct->adf_agcnt == 0  ||  *emptyres == 1)
    {
	res.dn_status   = AD_DN_NULL;
	res.dn_dttype	 = dv_result->db_datatype;

	return (adu_7from_dtntrnl (adf_scb,dv_result, &res));
    }

    /* divide by number of rows and place results into res db_data_value */
    mremain       = *imonths % ag_struct->adf_agcnt;
    res.dn_month = *imonths / ag_struct->adf_agcnt;
    if (res.dn_month != 0)
	*istat |= AD_DN_MONTHSPEC;

    /* get remaining time in terms of millisecs and divide by # of rows	    */
    ftemp = (mremain * AD_13DTE_DAYPERMONTH * AD_40DTE_SECPERDAY + *isecs)
	    / (f8)ag_struct->adf_agcnt;

    /* we have to partially normalize now because the isecs f8 might not   */
    /* fit into an i4 so we can't just put it into res.dn_seconds	    */
    
    days = (i4)(ftemp / AD_40DTE_SECPERDAY);
    if (days != 0)
	*istat |= AD_DN_DAYSPEC;
	
    ftemp -= (f8)days*AD_40DTE_SECPERDAY;
    res.dn_seconds = (i4)ftemp;
    if (res.dn_seconds != 0)
	*istat |= AD_DN_TIMESPEC;

    ftemp -= (f8)res.dn_seconds;
    res.dn_nsecond = (i4)(ftemp*AD_50DTE_NSPERS);
    if (res.dn_nsecond != 0)
        *istat |= AD_DN_TIMESPEC;

    res.dn_day  = days;
    res.dn_status  = (char)*istat;
    res.dn_year    = 0;

    if (days > 0)
    {
	res.dn_status |= AD_DN_DAYSPEC;
    }
    /* normalize date interval */
    adu_2normldint(&res);

    return (adu_7from_dtntrnl (adf_scb, dv_result, &res));
}


/*{
** Name: adu_E4d_sum_date() - Ag-end for sum(date).
**
** Description:
**      Returns the result of an sum(date) aggregation.
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
**      06-nov-88 (thurston)
**          Initial creation ... coding may be done later.
**	06-dec-88 (jrb)
**	    Initial coding.
**	04-jan-89 (jrb)
**	    Changed the way this works to conform to LRC decisions regarding
**	    empty dates in SUM().
**	31-oct-2006 (gupsh01)
**	    Modified to support new internal format for Dates.
**	10-Feb-2008 (kiria01) b119885
**	    Also ensure that the res structure is fully iniialised.
**       2-Apr-2009 (hanal04) Bug 121881
**          We can not call adu_6to_dtntrnl() to initialise res. We don't
**          have an input value. dv_result is an address to put the 
**          aggregate result not an input value.
*/

DB_STATUS
adu_E4d_sum_date(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_STATUS		db_stat;
    i4			*istat    = &ag_struct->adf_agirsv[0];
    i4			*emptyres = &ag_struct->adf_agirsv[1];
    i4			*imonths  = &ag_struct->adf_agirsv[2];
    f8			*isecs	  = &ag_struct->adf_agfrsv[0];
    AD_NEWDTNTRNL	res;
    i4			days;

    MEfill ((u_i2)sizeof(res), NULLCHAR, (PTR) &res);

    /* if no rows aggregated or if empty date was in any row then we return */
    /* the empty date; if we were in SQL and no rows were aggregated this   */
    /* routine would never get called and the result would be NULL	    */

    if (ag_struct->adf_agcnt == 0  ||  *emptyres == 1)
    {
	res.dn_status   = AD_DN_NULL;
	res.dn_dttype	 = dv_result->db_datatype;

	return (adu_7from_dtntrnl (adf_scb, dv_result, &res));
    }   

    /* we have to partially normalize now because the isecs f8 might not   */
    /* fit into an i4 so we can't just put it into res->dn_seconds	    */
    days	    = (i4)(*isecs / AD_40DTE_SECPERDAY);
    res.dn_seconds = (i4)(*isecs - (days * AD_40DTE_SECPERDAY));
    res.dn_month   = *imonths;
    res.dn_day     = days;
    res.dn_status  = (char)*istat;
    res.dn_year    = 0;

    if (days > 0)
    {
	res.dn_status |= AD_DN_DAYSPEC;
    }

    /* normalize date interval */
    adu_2normldint(&res);

    if (res.dn_year > 800 || res.dn_year < -800)
    {
	return (adu_error(adf_scb, E_AD505A_DATEYROVFLO, 2,
			  (i4)sizeof(res.dn_year),
			  (i4 *) &res.dn_year));
    }
    return (adu_7from_dtntrnl (adf_scb, dv_result, &res));
}


/*{
** Name: adu_E0d_minmax_date() - Ag-end for min(date) or max(date).
**
** Description:
**      Returns the result of an min(date) or max(date) aggregation.
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
**	    Fixed a bug that was producing garbage results for QUEL min(date)
**	    and max(date) queries that aggregated zero rows ... the
**	    ag-work-space had never been set to anything.  Should instead,
**	    check for zero rows, and call adc_getempty() to return a default
**	    value.  (NOTE:  For SQL, this routine will not be called when zero
**	    rows have been aggregated.  The higher level ade_execute_cx() will
**	    take care of this, or the adf_agend() routine itself.  This is
**	    because for SQL, the NULL value should be returned in these
**	    situations, not the default value.)
*/

DB_STATUS
adu_E0d_minmax_date(
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

/*
[@function_definition@]...
*/
