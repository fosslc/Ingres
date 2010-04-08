/*
** Copyright (c) 2004 Ingres Corporation 
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <me.h>
#include    <mh.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/* [@#include@]... */

/*
**  Name: ADUAGDEC.C - Aggregate functions for decimal
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "decimal" datatype.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**	    adu_B0n_agg_dec() - Ag-begin for all decimal aggs.
**	    adu_N0n_tot_dec() - Ag-next for avg[u](dec) or sum[u](dec).
**          adu_E3n_avg_dec() - Ag-end for avg(dec) or avgu(dec).
**          adu_E4n_sum_dec() - Ag-end for sum(dec) or sumu(dec).
**	    adu_N5n_min_dec() - Ag-next for min(dec).
**	    adu_N6n_max_dec() - Ag-next for max(dec).
**          adu_E0n_minmax_dec() - Ag-end for min(dec) or max(dec).
[@func_list@]...
**
**
**  History:
**	14-jul-89 (jrb)
**	    Created.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**      20-apr-1993 (stevet)
**          Added missing include <me.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      19-feb-1999 (matbe01)
**          Move include for ex.h after include for mh.h. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-oct-2004 (somsa01)
**	    In adu_N3n_avg_dec() and adu_N4n_sum_dec(), increased the size
**	    of temp to account for datatype nullability.
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
** Name: adu_B0n_agg_dec() - Ag-begin for all decimal aggregates
**
** Description:
**	Initialization aggregate struct for all decimal aggregates.  This
**	routine is required because we need extra workspace for processing
**	decimal aggregates.
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
**	ag_struct
**	    .adf_agwork
**		.db_datatype		Result datatype for the aggregate.
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
** History:
**      15-jul-86 (jrb)
**          Created.
*/

DB_STATUS
adu_B0n_agg_dec(
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
    ag_struct->adf_agwork.db_datatype = fidesc->adi_dtresult;

    /* Note that the rest of the adf_agwork space will be set up in the
    ** ag_next routines for the various aggregates.  We are done here.
    */
    return(E_DB_OK);
}


/*{
** Name: adu_N3n_avg_dec() - Ag-next for avg(dec) or avgu(dec).
**
** Description:
**      Compiles another "decimal" datavalue into an avg aggregation.  The
**	intermediate buffer will be the same as the result size.
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
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	09-aug-89 (jrb)
**	    Written.
**	15-oct-2004 (somsa01)
**	    Increased the size of temp by one to account for nullability
*/

DB_STATUS
adu_N3n_avg_dec(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curdv = &ag_struct->adf_agwork;
    ADI_FI_DESC	    fidesc;
    u_char	    temp[DB_MAX_DECLEN+1];
    i4		    prec;
    i4		    scale;

    if (ag_struct->adf_agcnt == 0)
    {
	/* Use an intermediate buffer with precision=DB_MAX_DECPREC and
	** scale=min(precision, input_scale+1).  Length was already set
	** appropriately by caller and it was checked in ag_begin.
	*/
	prec  = DB_MAX_DECPREC;
	scale = min(prec, DB_S_DECODE_MACRO(dv_next->db_prec)+1);
	curdv->db_prec = DB_PS_ENCODE_MACRO(prec, scale);

	if (CVpkpk( (PTR)dv_next->db_data,
			(i4)DB_P_DECODE_MACRO(dv_next->db_prec),
			(i4)DB_S_DECODE_MACRO(dv_next->db_prec),
			(i4)prec,
			(i4)scale,
		    (PTR)curdv->db_data)
	    == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
    }
    else
    {
	MEcopy((PTR)curdv->db_data, curdv->db_length, (PTR)temp);

	MHpkadd((PTR)dv_next->db_data,
		    (i4)DB_P_DECODE_MACRO(dv_next->db_prec),
		    (i4)DB_S_DECODE_MACRO(dv_next->db_prec),
		(PTR)temp,
		    (i4)DB_P_DECODE_MACRO(curdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(curdv->db_prec),
		(PTR)curdv->db_data,
		    &prec,
		    &scale);

	/* We ignore new precision and scale.  This is because precision
	** will always be DB_MAX_DECPREC (the rules say that this is the
	** limit) and scale will never change because we are aggregating
	** values with the same scale every time.  Changing formulas for
	** how the result size of additions are calculated, or changing
	** how avg() works may mean changing this code.
	*/
    }

    ag_struct->adf_agcnt++;

    return(E_DB_OK);
}


/*{
** Name: adu_N4n_sum_dec() - Ag-next for sum(dec) or sumu(dec).
**
** Description:
**      Compiles another "decimal" datavalue into a sum aggregate.  The
**	intermediate buffer will be the same as the result size.
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
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	26-jul-89 (jrb)
**	    Written.
**	05-jan-2005 (zhahu02)
**	    Updated for temp size (INGSRV3029/b113705).
**	15-oct-2004 (somsa01)
**	    Increased the size of temp by one to account for nullability
*/

DB_STATUS
adu_N4n_sum_dec(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_DATA_VALUE   *curdv = &ag_struct->adf_agwork;
    ADI_FI_DESC	    fidesc;
    u_char	    temp[DB_MAX_DECLEN+1];
    i4		    prec;
    i4		    scale;

    if (ag_struct->adf_agcnt == 0)
    {
	/* Use an intermediate buffer with precision=DB_MAX_DECPREC and
	** scale=input_scale.  Length was already set appropriately by caller
	** and and it was checked in ag_begin.
	*/
	prec  = DB_MAX_DECPREC;
	scale = DB_S_DECODE_MACRO(dv_next->db_prec);
	curdv->db_prec = DB_PS_ENCODE_MACRO(prec, scale);

	if (CVpkpk( (PTR)dv_next->db_data,
			(i4)DB_P_DECODE_MACRO(dv_next->db_prec),
			(i4)DB_S_DECODE_MACRO(dv_next->db_prec),
			(i4)prec,
			(i4)scale,
		    (PTR)curdv->db_data)
	    == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
    }
    else
    {
	MEcopy((PTR)curdv->db_data, curdv->db_length, (PTR)temp);

	MHpkadd((PTR)dv_next->db_data,
		    (i4)DB_P_DECODE_MACRO(dv_next->db_prec),
		    (i4)DB_S_DECODE_MACRO(dv_next->db_prec),
		(PTR)temp,
		    (i4)DB_P_DECODE_MACRO(curdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(curdv->db_prec),
		(PTR)curdv->db_data,
		    &prec,
		    &scale);

	/* We ignore new precision and scale.  This is because precision
	** will always be DB_MAX_DECPREC (the rules say that this is the
	** limit) and scale will never change because we are aggregating
	** values with the same scale every time.  Changing formulas for
	** how the result size of additions are calculated, or changing
	** how sum() works may mean changing this code.
	*/
    }

    ag_struct->adf_agcnt++;

    return(E_DB_OK);
}


/*{
** Name: adu_E3n_avg_dec() - Ag-end for avg(dec) or avgu(dec).
**
** Description:
**      Returns the result of an avg(dec) or avgu(dec) aggregation.
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
**	    EXDECOVF			Decimal overflow
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	26-jul-89 (jrb)
**	    Written.
*/

DB_STATUS
adu_E3n_avg_dec(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_STATUS		    db_stat;
    DB_DATA_VALUE	    *tot_dv = &ag_struct->adf_agwork;
    u_char		    nrows[DB_MAX_DECLEN];
    i4			    rslt_prec;
    i4			    rslt_scale;

    /* if no rows then return default value */
    if (ag_struct->adf_agcnt == 0)
    {
	if (db_stat = adc_getempty(adf_scb, dv_result))
	    return(db_stat);
    }
    else
    {
	/* calculate the aggregate result:
	**   1) convert number-of-rows to decimal
	**   2) divide total of all values by number-of-rows
	*/

	if (CVlpk((i4)ag_struct->adf_agcnt,
			AD_I4_TO_DEC_PREC,
			AD_I4_TO_DEC_SCALE,
			(PTR)nrows)
	    != OK)
	{
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}

	MHpkdiv((PTR)tot_dv->db_data,
		    (i4)DB_P_DECODE_MACRO(tot_dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(tot_dv->db_prec),
		(PTR)nrows,
		    (i4)AD_I4_TO_DEC_PREC,
		    (i4)AD_I4_TO_DEC_SCALE,
		(PTR)dv_result->db_data,
		    &rslt_prec,
		    &rslt_scale);
    }

    /* We are done since the result precision and scale will be correct.
    ** If the intermediate buffer size was 31,s, then the result of the
    ** above division will be 31,max(0, 31-31+s-0) which is 31,s (note
    ** the dependence on the formula for the result scale of decimal
    ** division!).
    */

    return(E_DB_OK);
}



/*{
** Name: adu_E4n_sum_dec() - Ag-end for sum(dec) or sumu(dec).
**
** Description:
**      Returns the result of a sum(dec) or sumu(dec) aggregation.
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
**	    EXDECOVF
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	26-jul-89 (jrb)
**	    Written.
*/

DB_STATUS
adu_E4n_sum_dec(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_STATUS		    db_stat;
    DB_DATA_VALUE	    *tot_dv = &ag_struct->adf_agwork;

    /* if no rows then return default value */
    if (ag_struct->adf_agcnt == 0)
    {
	if (db_stat = adc_getempty(adf_scb, dv_result))
	    return(db_stat);
    }
    else
    {
	/* move workspace into result; result will already be set up for
	** precision=31, scale=input_scale; note the dependence on the result
	** formulas for the sum aggregate
	*/
	
	MEcopy(tot_dv->db_data, tot_dv->db_length, dv_result->db_data);
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N5n_min_dec() - Ag-next for min(dec).
**
** Description:
**      Compiles another "dec" datavalue into the min(dec) aggregation.
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
**	    adf_agwork			Workspace for agg
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
**      ag_struct                       Ptr to the ADF_AG_STRUCT being used for
**					this aggregation.
**	    .db_prec			Will be updated on first iteration.
**	    .db_data			Will be updated with latest min value.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** History:
**	26-jul-89 (jrb)
**	    Written.
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
*/

DB_STATUS
adu_N5n_min_dec(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_STATUS		    db_stat;
    DB_DATA_VALUE	    *curmindv = &ag_struct->adf_agwork;
    ADI_FI_DESC		    *fidesc;

    if (ag_struct->adf_agcnt == 0)
    {
	/* set result precision/scale and length */
	if (db_stat = adi_fidesc(adf_scb, ag_struct->adf_agfi, &fidesc))
	    return(db_stat);
	    
	if (db_stat = adi_0calclen(adf_scb, &fidesc->adi_lenspec, 1,
			&dv_next, curmindv))
	{
	    return(db_stat);
	}
		    
        MEcopy(dv_next->db_data, curmindv->db_length, curmindv->db_data);
    }
    else
    {
	if (MHpkcmp((PTR)dv_next->db_data,
			(i4)DB_P_DECODE_MACRO(dv_next->db_prec),
			(i4)DB_S_DECODE_MACRO(dv_next->db_prec),
		    (PTR)curmindv->db_data,
			(i4)DB_P_DECODE_MACRO(curmindv->db_prec),
			(i4)DB_S_DECODE_MACRO(curmindv->db_prec)) < 0)
	{
	    MEcopy(dv_next->db_data,
			dv_next->db_length,
			curmindv->db_data);
	}
    }

    ag_struct->adf_agcnt++;
    		
    return(E_DB_OK);
}


/*{
** Name: adu_N6n_max_dec() - Ag-next for max(dec).
**
** Description:
**      Compiles another "dec" datavalue into the max(dec) aggregation.
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
**      ag_struct                       Ptr to the ADF_AG_STRUCT being used for
**					this aggregation.
**	    .db_prec			Will be updated on first iteration.
**	    .db_data			Will be updated with latest max value.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** History:
**	26-jul-89 (jrb)
**	    Written.
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
*/

DB_STATUS
adu_N6n_max_dec(		
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    DB_STATUS		    db_stat;
    DB_DATA_VALUE	    *curmaxdv = &ag_struct->adf_agwork;
    ADI_FI_DESC		    *fidesc;

    if (ag_struct->adf_agcnt == 0)
    {
	/* set result precision/scale and length */
	if (db_stat = adi_fidesc(adf_scb, ag_struct->adf_agfi, &fidesc))
	    return(db_stat);
	    
	if (db_stat = adi_0calclen(adf_scb, &fidesc->adi_lenspec, 1,
			&dv_next, curmaxdv))
	{
	    return(db_stat);
	}
		    
	
        MEcopy(dv_next->db_data, curmaxdv->db_length, curmaxdv->db_data); 
    }
    else
    {
	if (MHpkcmp((PTR)dv_next->db_data,
			(i4)DB_P_DECODE_MACRO(dv_next->db_prec),
			(i4)DB_S_DECODE_MACRO(dv_next->db_prec),
		    (PTR)curmaxdv->db_data,
			(i4)DB_P_DECODE_MACRO(curmaxdv->db_prec),
			(i4)DB_S_DECODE_MACRO(curmaxdv->db_prec)) > 0)
	{
	    MEcopy(dv_next->db_data,
			dv_next->db_length,
			curmaxdv->db_data);
	}
    }

    ag_struct->adf_agcnt++;
		
    return(E_DB_OK);
}


/*{
** Name: adu_E0n_minmax_dec() - Ag-end for min(dec) or max(dec).
**
** Description:
**      Returns the result of an min(dec) or max(dec) aggregation.
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
** History:
**	26-jul-89 (jrb)
**	    Written.
*/

DB_STATUS
adu_E0n_minmax_dec(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    DB_STATUS		    db_stat;
    DB_DATA_VALUE   	    *curdv = &ag_struct->adf_agwork;

    /* For the min/max aggs, we know that the workspace dv was set with the
    ** same prec/scale and length as the result; this means we need only copy
    ** the db_data field from the workspace to the db_data field of the result.
    */

    if (ag_struct->adf_agcnt == 0)
    {
	if (db_stat = adc_getempty(adf_scb, dv_result))
	    return(db_stat);
    }
    else
    {
	MEcopy(curdv->db_data, curdv->db_length, dv_result->db_data);
    }


    /* Set db_length back to proper value in case ag_begin action is called
    ** again (this happens for the QUEL byval or SQL "group by")
    */ 
    curdv->db_length = DB_MAX_DECLEN;

    return(E_DB_OK);
}

/*
[@function_definition@]...
*/
