/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
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
**  Name: ADUAGINT.C - Aggregate functions for integers (the "i" datatype).
**
**  Description:
**      This file contains all of the routines needed for INGRES to perform 
**      any of the aggregate functions on the "i" datatype.
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**	    adu_N3i_avg_i() - Ag-next for avg(i) or avgu(i).
**          adu_E3i_avg_i() - Ag-end for avg(i) or avgu(i).
**	    adu_N4i_sum_i() - Ag-next for sum(i) or sumu(i).
**          adu_E4i_sum_i() - Ag-end for sum(i) or sumu(i).
**	    adu_N5i_min_i() - Ag-next for min(i).
**	    adu_N6i_max_i() - Ag-next for max(i).
**          adu_E0i_minmax_i() - Ag-end for min(i) or max(i).
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      06-aug-86 (thurston)
**          Converted to Jupiter standards.  The following history comments
**          have been preserved:
**
**		Revision 30.2  85/10/09  15:29:56  roger
**		Removed unneccessary "#ifdef UNS_CHAR" code.
**		Assignment to an i1 is no problem; e.g. if x = 22
**		and y = -1, (x += y) == (22 += 255) == 277, which,
**		when you lop off the ninth bit, is 21. What did he say?
**		
**		Revision 3.22.40.2  85/07/11  19:27:24  roger
**		Propagated fix for bug #4021.  Use I1_CHECK_MACRO to ensure
**		preservation of i1 values across assignments on machines
**		without signed characters.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	16-mar-87 (thurston)
**	    Added support for nullables.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**      08-mar-1993 (stevet)
**          Changed adu_N4i_sum_i() to use MHi4add() to handle adding I4 
**          values so that correct signal will be generated on MATH exceptions.
**      19-feb-1999 (matbe01)
**          Move include for ex.h after include for mh.h. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
**	8-Nov-2005 (schka24)
**	    FI for SUM(int) changed to return an i8, needs to happen here too!
**	    Allow for i8's in other aggs too.
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
** Name: adu_N3i_avg_i() - Ag-next for avg(i) or avgu(i).
**
** Description:
**      Compiles another "i" datavalue into the avg(i) aggregation.
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
adu_N3i_avg_i(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];
    f8		    newval;


    switch (dv_next->db_length)
    {
      case 1:
	newval = (i4)I1_CHECK_MACRO(*(i1 *)dv_next->db_data);
	break;

      case 2:
	newval = *(i2 *)dv_next->db_data;
	break;

      case 4:
	newval = *(i4 *)dv_next->db_data;
	break;

      case 8:
	newval = *(i8 *)dv_next->db_data;
	break;
    }

    ag_struct->adf_agcnt++;

# ifdef	SUN
    *curavg = *curavg + ((newval - *curavg) / ag_struct->adf_agcnt);
# else
    *curavg += ((newval - *curavg) / ag_struct->adf_agcnt);
# endif

    return(E_DB_OK);
}


/*{
** Name: adu_E3i_avg_i() - Ag-end for avg(i) or avgu(i).
**
** Description:
**      Returns the result of an avg(i) or avgu(i) aggregation.
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
adu_E3i_avg_i(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    f8		    *curavg = &ag_struct->adf_agfrsv[0];


    *(f8 *)dv_result->db_data = *curavg;
    return(E_DB_OK);
}


/*{
** Name: adu_N4i_sum_i() - Ag-next for sum(i) or sumu(i).
**
** Description:
**      Compiles another "i" datavalue into the sum(i) aggregation.
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
**      08-mar-1993 (stevet)
**          Call MHi4add() to handle adding I4 values so that correct signal
**          will be generated on MATH exceptions.
**	7-Nov-2005 (schka24)
**	    Int sums are now i8.
*/

DB_STATUS
adu_N4i_sum_i(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    i8		    *cursum = &ag_struct->adf_ag8rsv;
    i8              tempi;

    switch (dv_next->db_length)
    {
      case 1:
	tempi = *(i1 *)dv_next->db_data;
	break;

      case 2:
	tempi = *(i2 *)dv_next->db_data;
	break;

      case 4:
	tempi = *(i4 *)dv_next->db_data;
	break;

      case 8:
	tempi = *(i8 *)dv_next->db_data;
	break;
    }
    *cursum = MHi8add(*cursum, tempi);
    ag_struct->adf_agcnt++;

    return(E_DB_OK);
}


/*{
** Name: adu_E4i_sum_i() - Ag-end for sum(i) or sumu(i).
**
** Description:
**      Returns the result of an sum(i) or sumu(i) aggregation.
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
**	7-Nov-2005 (schka24)
**	    sum(int) now returns i8.
*/

DB_STATUS
adu_E4i_sum_i(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    i8		    *cursum = &ag_struct->adf_ag8rsv;


    *(i8 *)dv_result->db_data = *cursum;
    return(E_DB_OK);
}


/*{
** Name: adu_N5i_min_i() - Ag-next for min(i).
**
** Description:
**      Compiles another "i" datavalue into the min(i) aggregation.
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
adu_N5i_min_i(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    i8		    *curmin = &ag_struct->adf_ag8rsv;


    if (ag_struct->adf_agcnt < 1)
    {
	ag_struct->adf_agcnt = 1;
	switch (dv_next->db_length)
	{
	  case 1:
	    *curmin = *(i1 *)dv_next->db_data;
	    break;

	  case 2:
	    *curmin = *(i2 *)dv_next->db_data;
	    break;

	  case 4:
	    *curmin = *(i4 *)dv_next->db_data;
	    break;

	  case 8:
	    *curmin = *(i8 *)dv_next->db_data;
	    break;
	}
    }
    else
    {
	ag_struct->adf_agcnt++;
	switch (dv_next->db_length)
	{
	  case 1:
	    if (*curmin > I1_CHECK_MACRO(*(i1 *)dv_next->db_data))
		*curmin = *(i1 *)dv_next->db_data;
	    break;

	  case 2:
	    if (*curmin > *(i2 *)dv_next->db_data) 
		*curmin = *(i2 *)dv_next->db_data;
	    break;

	  case 4:
	    if (*curmin > *(i4 *)dv_next->db_data) 
		*curmin = *(i4 *)dv_next->db_data;
	    break;

	  case 8:
	    if (*curmin > *(i8 *)dv_next->db_data) 
		*curmin = *(i8 *)dv_next->db_data;
	    break;
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_N6i_max_i() - Ag-next for max(i).
**
** Description:
**      Compiles another "i" datavalue into the max(i) aggregation.
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
adu_N6i_max_i(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_next,
ADF_AG_STRUCT	*ag_struct)
{
    i8		*curmax = &ag_struct->adf_ag8rsv;


    if (ag_struct->adf_agcnt < 1)
    {
        ag_struct->adf_agcnt = 1;
	switch (dv_next->db_length)
	{
	  case 1:
	    *curmax = *(i1 *)dv_next->db_data;
	    break;

	  case 2:
	    *curmax = *(i2 *)dv_next->db_data;
	    break;

	  case 4:
	    *curmax = *(i4 *)dv_next->db_data;
	    break;

	  case 8:
	    *curmax = *(i8 *)dv_next->db_data;
	    break;
	}
    }
    else
    {
	ag_struct->adf_agcnt++;
	switch (dv_next->db_length)
	{
	  case 1:
	    if (*curmax < I1_CHECK_MACRO(*(i1 *)dv_next->db_data))
		*curmax = *(i1 *)dv_next->db_data;
	    break;

	  case 2:
	    if (*curmax < *(i2 *)dv_next->db_data) 
		*curmax = *(i2 *)dv_next->db_data;
	    break;

	  case 4:
	    if (*curmax < *(i4 *)dv_next->db_data) 
		*curmax = *(i4 *)dv_next->db_data;
	    break;

	  case 8:
	    if (*curmax < *(i8 *)dv_next->db_data) 
		*curmax = *(i8 *)dv_next->db_data;
	    break;
	}
    }

    return(E_DB_OK);
}


/*{
** Name: adu_E0i_minmax_i() - Ag-end for min(i) or max(i).
**
** Description:
**      Returns the result of an min(i) or max(i) aggregation.
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
adu_E0i_minmax_i(
ADF_CB          *adf_scb,
ADF_AG_STRUCT	*ag_struct,
DB_DATA_VALUE	*dv_result)
{
    i8		    *curminmax = &ag_struct->adf_ag8rsv;


    switch (dv_result->db_length)
    {
      case 1:
	*(i1 *)dv_result->db_data = *curminmax;	/* {@fix_me@} ... is this OK? */
	break;

      case 2:
	*(i2 *)dv_result->db_data = *curminmax;
	break;

      case 4:
	*(i4 *)dv_result->db_data = *curminmax;
	break;

      case 8:
	*(i8 *)dv_result->db_data = *curminmax;
	break;
    }

    return(E_DB_OK);
}

/*
[@function_definition@]...
*/
