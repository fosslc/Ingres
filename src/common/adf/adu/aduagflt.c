/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <mh.h>
#include    <ex.h>
#include    <sl.h>
#include    <clfloat.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/* [@#include@]... */

/**
**
**  Name: ADUAGFLT.C - Aggregate functions for floats (the "f" datatype).
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "f" datatype.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**	    adu_N3f_avg_f() - Ag-next for avg(f) or avgu(f).
**          adu_E3f_avg_f() - Ag-end for avg(f) or avgu(f).
**	    adu_N4f_sum_f() - Ag-next for sum(f) or sumu(f).
**          adu_E4f_sum_f() - Ag-end for sum(f) or sumu(f).
**	    adu_N5f_min_f() - Ag-next for min(f).
**	    adu_N6f_max_f() - Ag-next for max(f).
**          adu_E0f_minmax_f() - Ag-end for min(f) or max(f).
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      07-aug-86 (thurston)
**          Converted to Jupiter standards.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**	15-apr-87 (thurston)
**	    Fixed bugs in adu_N5f_min_f() and adu_N6f_max_f() that were caused
**	    by code of this form:
**		if (....)
**		    if (....)
**			stmt;
**		else
**		    if (....)
**			stmt;
**	    The "else" gets used for the 2nd "if", not the 1st, as had been
**	    intended.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**      17-jan-1993 (stevet)
**          Added checks for floating point overflow detections (B58853).
**	26-apr-94 (vijay)
**	    Change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**      19-feb-1999 (matbe01)
**          Move include for ex.h after include for mh.h. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
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
** Name: adu_N3f_avg_f() - Ag-next for avg(f) or avgu(f).
**
** Description:
**      Compiles another "f" datavalue into the avg(f) aggregation.
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
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

DB_STATUS
adu_N3f_avg_f(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];
    f8		    newval;


    if (dv_next->db_length == 4)
	newval = *(f4 *)dv_next->db_data;
    else
	newval = *(f8 *)dv_next->db_data;

    ag_struct->adf_agcnt++;

# ifdef	SUN
    *curavg = *curavg + ((newval - *curavg) / ag_struct->adf_agcnt);
# else
    *curavg += ((newval - *curavg) / ag_struct->adf_agcnt);
# endif

    return(E_DB_OK);
}


/*{
** Name: adu_E3f_avg_f() - Ag-end for avg(f) or avgu(f).
**
** Description:
**      Returns the result of an avg(f) or avgu(f) aggregation.
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
adu_E3f_avg_f(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];


    *(f8 *)dv_result->db_data = *curavg;
    return(E_DB_OK);
}


/*{
** Name: adu_N4f_sum_f() - Ag-next for sum(f) or sumu(f).
**
** Description:
**      Compiles another "f" datavalue into the sum(f) aggregation.
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
adu_N4f_sum_f(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *cursum = &ag_struct->adf_agfrsv[0];


    EXmath((i4)EX_ON);

    if (dv_next->db_length == 4)
#ifdef SUN
	*cursum = *cursum + *(f4 *)dv_next->db_data;
    else
	*cursum = *cursum + *(f8 *)dv_next->db_data;
#else
	*cursum += *(f4 *)dv_next->db_data;
    else
	*cursum += *(f8 *)dv_next->db_data;
#endif

    ag_struct->adf_agcnt++;

    EXmath((i4)EX_OFF);
    return(E_DB_OK);
}


/*{
** Name: adu_E4f_sum_f() - Ag-end for sum(f) or sumu(f).
**
** Description:
**      Returns the result of an sum(f) or sumu(f) aggregation.
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
**      17-jan-1994 (stevet)
**          Added floating point overflow detections.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
*/

DB_STATUS
adu_E4f_sum_f(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *cursum = &ag_struct->adf_agfrsv[0];

    if (!MHdfinite (*cursum) || 
	(dv_result->db_length == 4 && CV_ABS_MACRO(*cursum) > FLT_MAX))
	EXsignal (EXFLTOVF, 0);

    if (dv_result->db_length == 4)
	*(f4 *)dv_result->db_data = *cursum;
    else
	*(f8 *)dv_result->db_data = *cursum;

    return(E_DB_OK);
}


/*{
** Name: adu_N5f_min_f() - Ag-next for min(f).
**
** Description:
**      Compiles another "f" datavalue into the min(f) aggregation.
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
**	15-apr-87 (thurston)
**	    Fixed bug caused by code of this form:
**		if (....)
**		    if (....)
**			stmt;
**		else
**		    if (....)
**			stmt;
**	    The "else" gets used for the 2nd "if", not the 1st, as had been
**	    intended.
*/

DB_STATUS
adu_N5f_min_f(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curmin = &ag_struct->adf_agfrsv[0];
    register f4	    f4val;
    register f8	    f8val;
    bool	    is_f4;


    if (is_f4 = (dv_next->db_length == 4))
	f4val = *(f4 *)dv_next->db_data;
    else
	f8val = *(f8 *)dv_next->db_data;

    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	if (is_f4)
	    *curmin = f4val;
	else
	    *curmin = f8val;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if	( is_f4  &&  *curmin > f4val)
	    *curmin = f4val;
	else if (!is_f4  &&  *curmin > f8val)
	    *curmin = f8val;
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N6f_max_f() - Ag-next for max(f).
**
** Description:
**      Compiles another "f" datavalue into the max(f) aggregation.
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
**	15-apr-87 (thurston)
**	    Fixed bug caused by code of this form:
**		if (....)
**		    if (....)
**			stmt;
**		else
**		    if (....)
**			stmt;
**	    The "else" gets used for the 2nd "if", not the 1st, as had been
**	    intended.
*/

DB_STATUS
adu_N6f_max_f(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curmax = &ag_struct->adf_agfrsv[0];
    register f4	    f4val;
    register f8	    f8val;
    bool	    is_f4;


    if (is_f4 = (dv_next->db_length == 4))
	f4val = *(f4 *)dv_next->db_data;
    else
	f8val = *(f8 *)dv_next->db_data;

    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	if (is_f4)
	    *curmax = f4val;
	else
	    *curmax = f8val;
    }
    else
    {
	ag_struct->adf_agcnt++;
	if	( is_f4  &&  *curmax < f4val)
	    *curmax = f4val;
	else if (!is_f4  &&  *curmax < f8val)
	    *curmax = f8val;
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E0f_minmax_f() - Ag-end for min(f) or max(f).
**
** Description:
**      Returns the result of an min(f) or max(f) aggregation.
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
adu_E0f_minmax_f(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *curminmax = &ag_struct->adf_agfrsv[0];


    if (dv_result->db_length == 4)
	*(f4 *)dv_result->db_data = *curminmax;
    else
	*(f8 *)dv_result->db_data = *curminmax;

    return(E_DB_OK);
}

/*
[@function_definition@]...
*/
