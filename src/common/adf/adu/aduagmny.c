/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adumoney.h>
/* [@#include@]... */

/**
**
**  Name: ADUAGMNY.C - Aggregate functions for moneys (the "money" datatype).
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "money" datatype.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**	    adu_N3m_avg_mny() - Ag-next for avg(money) or avgu(money).
**          adu_E3m_avg_mny() - Ag-end for avg(money) or avgu(money).
**	    adu_N4m_sum_mny() - Ag-next for sum(money) or sumu(money).
**          adu_E4m_sum_mny() - Ag-end for sum(money) or sumu(money).
**	    adu_N5m_min_mny() - Ag-next for min(money).
**	    adu_N6m_max_mny() - Ag-next for max(money).
**          adu_E0m_minmax_mny() - Ag-end for min(money) or max(money).
[@func_list@]...
**
**
**  History:    
**      08-aug-86 (thurston)
**          Converted to Jupiter standards.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
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
** Name: adu_N3m_avg_mny() - Ag-next for avg(money) or avgu(money).
**
** Description:
**      Compiles another "money" datavalue into the avg(money) aggregation.
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
**      06-aug-86 (thurston)
**          Converted for Jupiter.
*/

DB_STATUS
adu_N3m_avg_mny(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];
    f8		    newval;


    newval = ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;

    ag_struct->adf_agcnt++;

# ifdef	SUN
    *curavg = *curavg + ((newval - *curavg) / ag_struct->adf_agcnt);
# else
    *curavg += ((newval - *curavg) / ag_struct->adf_agcnt);
# endif

    return (E_DB_OK);
}


/*{
** Name: adu_E3m_avg_mny() - Ag-end for avg(money) or avgu(money).
**
** Description:
**      Returns the result of an avg(money) or avgu(money) aggregation.
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
**	    {@return_description@}
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      07-aug-86 (thurston)
**          Initial creation and coding.
*/

DB_STATUS
adu_E3m_avg_mny(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];


    ((AD_MONEYNTRNL *)dv_result->db_data)->mny_cents = *curavg;

    /* Only now do we round the money value to two places */
    return (adu_11mny_round(adf_scb, (AD_MONEYNTRNL *)dv_result->db_data));
}


/*{
** Name: adu_N4m_sum_mny() - Ag-next for sum(money) or sumu(money).
**
** Description:
**      Compiles another "money" datavalue into the sum(money) aggregation.
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
**      06-aug-86 (thurston)
**          Converted for Jupiter.
*/

DB_STATUS
adu_N4m_sum_mny(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *cursum = &ag_struct->adf_agfrsv[0];


    EXmath((i4)EX_ON);

#ifdef SUN
	*cursum = *cursum + ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
#else
	*cursum += ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
#endif

    ag_struct->adf_agcnt++;

    EXmath((i4)EX_OFF);

    /*
    ** Check for money overflow.  If an overflow has occured, return the proper
    ** error code.  (Note:  MONEY OVERFLOW no longer signals an exception.)
    */
    if (((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents > AD_1MNY_MAX_NTRNL)
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    else if (((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents < AD_3MNY_MIN_NTRNL)
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

    return (E_DB_OK);
}


/*{
** Name: adu_E4m_sum_mny() - Ag-end for sum(money) or sumu(money).
**
** Description:
**      Returns the result of an sum(money) or sumu(money) aggregation.
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
**	    {@return_description@}
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      07-aug-86 (thurston)
**          Initial creation and coding.
*/

DB_STATUS
adu_E4m_sum_mny(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *cursum = &ag_struct->adf_agfrsv[0];


    ((AD_MONEYNTRNL *)dv_result->db_data)->mny_cents = *cursum;

    /* Only now do we round the money value to two places */
    return (adu_11mny_round(adf_scb, (AD_MONEYNTRNL *)dv_result->db_data));
}


/*{
** Name: adu_N5m_min_mny() - Ag-next for min(money).
**
** Description:
**      Compiles another "money" datavalue into the min(money) aggregation.
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
**      06-aug-86 (thurston)
**          Converted for Jupiter.
*/

DB_STATUS
adu_N5m_min_mny(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curmin = &ag_struct->adf_agfrsv[0];


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	*curmin = ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if (*curmin > ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents)
	    *curmin = ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
    }

    return (E_DB_OK);
}


/*{
** Name: adu_N6m_max_mny() - Ag-next for max(money).
**
** Description:
**      Compiles another "money" datavalue into the max(money) aggregation.
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
**      06-aug-86 (thurston)
**          Converted for Jupiter.
*/

DB_STATUS
adu_N6m_max_mny(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curmax = &ag_struct->adf_agfrsv[0];


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	*curmax = ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if (*curmax < ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents)
	    *curmax = ((AD_MONEYNTRNL *)dv_next->db_data)->mny_cents;
    }

    return (E_DB_OK);
}


/*{
** Name: adu_E0m_minmax_mny() - Ag-end for min(money) or max(money).
**
** Description:
**      Returns the result of an min(money) or max(money) aggregation.
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
**	    {@return_description@}
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      07-aug-86 (thurston)
**          Initial creation and coding.
*/

DB_STATUS
adu_E0m_minmax_mny(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *curminmax = &ag_struct->adf_agfrsv[0];


    ((AD_MONEYNTRNL *)dv_result->db_data)->mny_cents = *curminmax;

    return (E_DB_OK);
}

/*
[@function_definition@]...
*/
