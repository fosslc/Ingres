/*
** Copyright (c) 2004,2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
#include    <me.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/* [@#include@]... */

/**
**
**  Name: ADUAGSTAT.C - Aggregate functions to compute statistical functions
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the statistical aggregate functions introduced with the ANSI
**	SQL OLAP amendment.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      22-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
**      02-Nov-04 (nansa02)
**          Formula for regr_intercept was implemented incorrectly.(Bug 112397)         
**	07-Feb-2008 (kiria01) b119878
**	    Correct the return values on encountering NULL data
**	11-Sep-2008 (kiria01) b120884
**	    Protected against inevitable alignment errors when accessing
**	    tuple buffers directly.
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
** Name: adu_N7a_corrr2() - Ag-next for corr(y, x) or regr_r2(y, x).
**
** Description:
**      Accumulates sums of xi, yi, xi**2, yi**2 and xi*yi.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      22-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N7a_corrr2(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locxf8, locyf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;
    /* If either x or y value is null, skip this call. */
    if (dv_y->db_datatype < 0)
    {
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_x->db_datatype < 0)
    {
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    /* Increment value counter. */
    olapag_struct->adf_agcnt++;

    /* Load x, y data values. */
    MEmemmove(val.raw, dv_y->db_data, dv_y->db_length);
    locyf8 = (dv_y->db_length <= 5) ? val.f4val : val.f8val;
    MEmemmove(val.raw, dv_x->db_data, dv_x->db_length);
    locxf8 = (dv_x->db_length <= 5) ? val.f4val : val.f8val;

    /* Now accumulate the various required sums. */
    olapag_struct->adf_agfrsv[0] += locxf8;  /* sum(x) */
    olapag_struct->adf_agfrsv[1] += locyf8;  /* sum(y) */
    olapag_struct->adf_agfrsv[2] += locxf8*locxf8;  /* sum(x**2) */
    olapag_struct->adf_agfrsv[3] += locyf8*locyf8;  /* sum(y**2) */
    olapag_struct->adf_agfrsv[4] += locxf8*locyf8;  /* sum(x*y) */

    return(E_DB_OK);
}


/*{
** Name: adu_N8a_covsxy() - Ag-next for covar_pop(y, x), covar_samp(y, x) or
**				regr_sxy(y, x)
**
** Description:
**      Accumulates sums of xi, yi and xi*yi.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N8a_covsxy(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locxf8, locyf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;
    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Increment count, then load x & y values into local f8's. */
    olapag_struct->adf_agcnt++;

    MEmemmove(val.raw, dv_x->db_data, dv_x->db_length);
    locxf8 = (dv_x->db_length <= 5) ? val.f4val : val.f8val;
    MEmemmove(val.raw, dv_y->db_data, dv_y->db_length);
    locyf8 = (dv_y->db_length <= 5) ? val.f4val : val.f8val;

    /* Update sums and sum of products. */
    olapag_struct->adf_agfrsv[0] += locxf8;
    olapag_struct->adf_agfrsv[1] += locyf8;
    olapag_struct->adf_agfrsv[4] += locxf8*locyf8;

    return(E_DB_OK);
}


/*{
** Name: adu_N9a_ravgx() - Ag-next for regr_avgx(y, x)
**
** Description:
**      Accumulates sum of xi.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N9a_ravgx(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locxf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;

    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Increment count, then load x value into local f8. */
    olapag_struct->adf_agcnt++;

    MEmemmove(val.raw, dv_x->db_data, dv_x->db_length);
    locxf8 = (dv_x->db_length <= 5) ? val.f4val : val.f8val;

    /* Update sum. */
    olapag_struct->adf_agfrsv[0] += locxf8;

    return(E_DB_OK);
}


/*{
** Name: adu_N10a_ravgy() - Ag-next for regr_avgy(y, x)
**
** Description:
**      Accumulates sum of yi.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N10a_ravgy(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locyf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;

    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Increment count, then load y value into local f8. */
    olapag_struct->adf_agcnt++;

    MEmemmove(val.raw, dv_y->db_data, dv_y->db_length);
    locyf8 = (dv_y->db_length <= 5) ? val.f4val : val.f8val;

    /* Update sum. */
    olapag_struct->adf_agfrsv[1] += locyf8;

    return(E_DB_OK);
}


/*{
** Name: adu_N11a_rcnt() - Ag-next for regr_count(y, x)
**
** Description:
**      Accumulates count of non-null (x, y) pairs.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N11a_rcnt(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Just increment count. */
    olapag_struct->adf_agcnt++;

    return(E_DB_OK);
}


/*{
** Name: adu_N12a_rintsl() - Ag-next for regr_intercept(y, x) or
**		regr_slope(y, x)
**
** Description:
**      Accumulates sums of xi, yi, xi**2 and xi*yi.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N12a_rintsl(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locxf8, locyf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;

    /* If either x or y value is null, skip this call. */
    if (dv_y->db_datatype < 0)
    {
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_x->db_datatype < 0)
    {
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    /* Increment value counter. */
    olapag_struct->adf_agcnt++;

    /* Load x, y data values. */
    MEmemmove(val.raw, dv_y->db_data, dv_y->db_length);
    locyf8 = (dv_y->db_length <= 5) ? val.f4val : val.f8val;
    MEmemmove(val.raw, dv_x->db_data, dv_x->db_length);
    locxf8 = (dv_x->db_length <= 5) ? val.f4val : val.f8val;

    /* Now accumulate the various required sums. */
    olapag_struct->adf_agfrsv[0] += locxf8;  /* sum(x) */
    olapag_struct->adf_agfrsv[1] += locyf8;  /* sum(y) */
    olapag_struct->adf_agfrsv[2] += locxf8*locxf8;  /* sum(x**2) */
    olapag_struct->adf_agfrsv[4] += locxf8*locyf8;  /* sum(x*y) */

    return(E_DB_OK);
}


/*{
** Name: adu_N13a_rsxx() - Ag-next for regr_sxx(y, x)
**
** Description:
**      Accumulates sums of xi and xi**2.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N13a_rsxx(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locxf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;

    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Increment count, then load x value into local f8. */
    olapag_struct->adf_agcnt++;

    MEmemmove(val.raw, dv_x->db_data, dv_x->db_length);
    locxf8 = (dv_x->db_length <= 5) ? val.f4val : val.f8val;

    /* Update sums. */
    olapag_struct->adf_agfrsv[0] += locxf8;
    olapag_struct->adf_agfrsv[2] += locxf8*locxf8;

    return(E_DB_OK);
}


/*{
** Name: adu_N14a_rsyy() - Ag-next for regr_syy(y, x)
**
** Description:
**      Accumulates sums of yi and yi**2.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_y	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the dependent variable (y).
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the independent variable (x).
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/

DB_STATUS
adu_N14a_rsyy(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_y,
DB_DATA_VALUE	*dv_x,
ADF_AG_OLAP	*olapag_struct)
{

    f8	locyf8;
    union {
	f8 f8val;
	f4 f4val;
	char raw[12];
    } val;

    if (dv_x->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_x->db_data)[dv_x->db_length-1] != 0) return E_DB_OK;
    }
    if (dv_y->db_datatype < 0)
    {
	/* If null value, don't process this datum. */
	if (((u_char *)dv_y->db_data)[dv_y->db_length-1] != 0) return E_DB_OK;
    }

    /* Increment count, then load y value into local f8. */
    olapag_struct->adf_agcnt++;

    MEmemmove(val.raw, dv_y->db_data, dv_y->db_length);
    locyf8 = (dv_y->db_length <= 5) ? val.f4val : val.f8val;

    /* Update sums. */
    olapag_struct->adf_agfrsv[1] += locyf8;
    olapag_struct->adf_agfrsv[3] += locyf8*locyf8;

    return(E_DB_OK);
}


/*{
** Name: adu_N15a_stvar() - Ag-next for std_pop(x), stddev_samp(x), var_pop(x)
**				or var_samp(x)
**
** Description:
**      Accumulates sums of xi and xi**2.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv_x	                        Ptr to the DB_DATA_VALUE which is the
**                                      next value of the aggregated variable.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
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
**      23-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Removed this function - executed inline in ade_execute_cx.
*/


/*{
** Name: adu_E7a_corr() - Ag-end for corr(y, x)
**
** Description:
**      Returns result of corr(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      24-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E7a_corr(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumx2, sumy, sumy2, sumxy, res;
    FUNC_EXTERN f8 MHipow();
    i4	count;

    /* First check for null result (if 0 rows in aggregate OR if
    ** count()*sum(x**2) = sum(x)**2 or count()*sum(y**2) = sum(y)**2). */

    sumx = olapag_struct->adf_agfrsv[0];
    sumy = olapag_struct->adf_agfrsv[1];
    sumx2 = olapag_struct->adf_agfrsv[2];
    sumy2 = olapag_struct->adf_agfrsv[3];

    if ((count = olapag_struct->adf_agcnt) <= 1 ||
	count * sumx2 == sumx*sumx ||
	count * sumy2 == sumy*sumy)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for correlation is:
	**	 sqrt((count()*sum(x*y) - sum(x)*sum(y))**2 /
	**	    ((count()*sum(x**2) - sum(x)*sum(x)) *
	**	     (count()*sum(y**2) - sum(y)*sum(y))))
	*/
	sumxy = olapag_struct->adf_agfrsv[4];

	res = sqrt(MHipow((count*sumxy - sumx*sumy), 2) / 
		((count*sumx2 - sumx*sumx) * (count*sumy2 - sumy*sumy)));
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E8a_covpop() - Ag-end for covar_pop(y, x)
**
** Description:
**      Returns result of covar_pop(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E8a_cpop(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)

{

    f8	sumx, sumy, sumxy, res;
    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    sumx = olapag_struct->adf_agfrsv[0];
    sumy = olapag_struct->adf_agfrsv[1];

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for covar_pop is:
	**	 (sum(x*y)-sum(x)*sum(y)/count()) / count()
	*/
	sumxy = olapag_struct->adf_agfrsv[4];

	res = (sumxy - sumx*sumy/count) / count;
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E9a_csamp() - Ag-end for covar_samp(y, x)
**
** Description:
**      Returns result of covar_samp(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E9a_csamp(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)

{

    f8	sumx, sumy, sumxy, res;
    i4	count;

    /* First check for null result (if 0 OR 1 rows in aggregate). */

    sumx = olapag_struct->adf_agfrsv[0];
    sumy = olapag_struct->adf_agfrsv[1];

    if ((count = olapag_struct->adf_agcnt) <= 1)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for covar_pop is:
	**	 (sum(x*y)-sum(x)*sum(y)/count()) / (count() - 1)
	*/
	sumxy = olapag_struct->adf_agfrsv[4];

	res = (sumxy - sumx*sumy/count) / (count-1);
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E10a_ravgx() - Ag-end for regr_avgx(y, x)
**
** Description:
**      Returns result of regr_avgx(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E10a_ravgx(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)

{

    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_avgx is:
	**	 sum(x) / count()
	*/

	f8 res = olapag_struct->adf_agfrsv[0] / count;
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E11a_ravgy() - Ag-end for regr_avgy(y, x)
**
** Description:
**      Returns result of regr_avgy(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E11a_ravgy(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)

{

    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_avgy is:
	**	 sum(y) / count()
	*/

	f8 res = olapag_struct->adf_agfrsv[1] / count;
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E11a_rcnt() - Ag-end for regr_count(y, x)
**
** Description:
**      Returns result of regr_count(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E12a_rcnt(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)

{

    /* regr_count(y, x) simply returns the row count. */

    MEmemmove(dv_result->db_data, &olapag_struct->adf_agcnt, sizeof(i4));

    return(E_DB_OK);
}


/*{
** Name: adu_E13a_rint() - Ag-end for regr_intercept(y, x)
**
** Description:
**      Returns result of regr_intercept(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E13a_rint(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumx2, sumy, sumxy, res;
    i4	count;

    /* First check for null result (if 0 rows in aggregate OR if
    ** count()*sum(x**2) = sum(x)**2. */

    sumx = olapag_struct->adf_agfrsv[0];
    sumx2 = olapag_struct->adf_agfrsv[2];

    if ((count = olapag_struct->adf_agcnt) == 0 ||
	count * sumx2 == sumx*sumx)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_intercept is:
	**	 (sum(y)*sum(x**2) - sum(x)*sum(x*y)) /
	**	    (count()*sum(x**2) - sum(x)*sum(x))
	*/
	sumy = olapag_struct->adf_agfrsv[1];
	sumxy = olapag_struct->adf_agfrsv[4];

	res = (sumy*sumx2 - sumx*sumxy) / 
		(count*sumx2 - sumx*sumx);
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E14a_rr2() - Ag-end for regr_r2(y, x)
**
** Description:
**      Returns result of regr_r2(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
**	10-oct-02 (inkdo01)
**	    Neglected to initialize sumy, sumy2 for first formula.
*/
DB_STATUS
adu_E14a_rr2(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumx2, sumy, sumy2, sumxy, res;
    FUNC_EXTERN f8 MHipow();
    i4	count;

    /* First check for null result (if 0 rows in aggregate OR if
    ** count()*sum(x**2) = sum(x)**2. */

    sumx = olapag_struct->adf_agfrsv[0];
    sumy = olapag_struct->adf_agfrsv[1];
    sumx2 = olapag_struct->adf_agfrsv[2];
    sumy2 = olapag_struct->adf_agfrsv[3];

    if ((count = olapag_struct->adf_agcnt) <= 1 ||
	count * sumx2 == sumx*sumx ||
	count * sumy2 == sumy*sumy)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_r2 is:
	**	if count()*sum(y**2) = sum(y)*sum(y) then 1,
	**	else (count()*sum(x*y) - sum(x)*sum(y))**2 /
	**	    ((count()*sum(x**2) - sum(x)*sum(x)) *
	**	     (count()*sum(y**2) - sum(y)*sum(y)))
	*/
	sumy = olapag_struct->adf_agfrsv[1];
	sumy2 = olapag_struct->adf_agfrsv[3];
	sumxy = olapag_struct->adf_agfrsv[4];

	if (count*sumy2 == sumy*sumy)
	    res = 1.0;
	else
	    res = MHipow((count*sumxy - sumx*sumy), 2) / 
		((count*sumx2 - sumx*sumx) * (count*sumy2 - sumy*sumy));
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E15a_rslope() - Ag-end for regr_slope(y, x)
**
** Description:
**      Returns result of regr_slope(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E15a_rslope(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumx2, sumy, sumxy, res;
    i4	count;

    /* First check for null result (if 0 rows in aggregate OR if
    ** count()*sum(x**2) = sum(x)**2. */

    sumx = olapag_struct->adf_agfrsv[0];
    sumx2 = olapag_struct->adf_agfrsv[2];

    if ((count = olapag_struct->adf_agcnt) == 0 ||
	count * sumx2 == sumx*sumx)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_slope is:
	**	 (count()*sum(x*y) - sum(x)*sum(y)) /
	**	    (count()*sum(x**2) - sum(x)*sum(x))
	*/
	sumy = olapag_struct->adf_agfrsv[1];
	sumxy = olapag_struct->adf_agfrsv[4];

	res = (count*sumxy - sumx*sumy) / 
		(count*sumx2 - sumx*sumx);
	MEmemmove(dv_result->db_data, &res, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E16a_rsxx() - Ag-end for regr_sxx(y, x)
**
** Description:
**      Returns result of regr_sxx(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E16a_rsxx(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumx2;
    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_sxx is:
	**	 sum(x**2) - sum(x)*sum(x)/count()
	*/
	sumx = olapag_struct->adf_agfrsv[0];
	sumx2 = olapag_struct->adf_agfrsv[2] - sumx*sumx/count;
	MEmemmove(dv_result->db_data, &sumx2, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E17a_rsxy() - Ag-end for regr_sxy(y, x)
**
** Description:
**      Returns result of regr_sxy(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E17a_rsxy(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumx, sumy, sumxy;
    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_sxy is:
	**	 sum(x*y) - sum(x)*sum(y)/count()
	*/
	sumx = olapag_struct->adf_agfrsv[0];
	sumy = olapag_struct->adf_agfrsv[1];
	sumxy = olapag_struct->adf_agfrsv[4] - sumx*sumy/count;
	MEmemmove(dv_result->db_data, &sumxy, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E18a_rsyy() - Ag-end for regr_syy(y, x)
**
** Description:
**      Returns result of regr_syy(y, x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      27-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Modifications for new parameter structure (part of aggregate
**	    code overhaul).
*/
DB_STATUS
adu_E18a_rsyy(
ADF_CB          *adf_scb,
ADF_AG_OLAP	*olapag_struct,
DB_DATA_VALUE	*dv_result)
{

    f8	sumy, sumy2;
    i4	count;

    /* First check for null result (if 0 rows in aggregate). */

    if ((count = olapag_struct->adf_agcnt) == 0)
    {
	if (dv_result->db_datatype < 0) 	/* nullable result */
	    ((u_char *)dv_result->db_data)[dv_result->db_length - 1] = ADF_NVL_BIT;
	else
	    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
    }
    else 
    {
	/* Formula for regr_syy is:
	**	 sum(y**2) - sum(y)*sum(y)/count()
	*/
	sumy = olapag_struct->adf_agfrsv[1];
	sumy2 = olapag_struct->adf_agfrsv[3] - sumy*sumy/count;
	MEmemmove(dv_result->db_data, &sumy2, sizeof(f8));
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E19a_stpop() - Ag-end for stddev_pop(x)
**
** Description:
**      Returns result of stddev_pop(x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      23-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Removed this function - executed inline in ade_execute_cx.
*/


/*{
** Name: adu_E20a_stsamp() - Ag-end for stddev_samp(x)
**
** Description:
**      Returns result of stddev_samp(x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      23-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Removed this function - executed inline in ade_execute_cx.
*/


/*{
** Name: adu_E21a_vpop() - Ag-end for var_pop(x)
**
** Description:
**      Returns result of var_pop(x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      23-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Removed this function - executed inline in ade_execute_cx.
*/


/*{
** Name: adu_E22a_vsamp() - Ag-end for var_samp(x)
**
** Description:
**      Returns result of var_samp(x) aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      olapag_struct                   Ptr to the ADF_AG_OLAP being used for
**					this aggregation.
**      dv_result                       The result of the aggregate is placed in
**                                      DB_DATA_VALUE.
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
**      23-sep-99 (inkdo01)
**	    Written.
**	22-june-01 (inkdo01)
**	    Removed this function - executed inline in ade_execute_cx.
*/
