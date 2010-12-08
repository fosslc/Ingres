/*
** Copyright (c) 1989, Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <add.h>
#include    <ade.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adfops.h>

/**
**
**  Name: ADIRESTY.C -	Find result type given fi desc and input datatypes.
**
**  Description:
**	This file contains a function adi_restype for allowing clients to find
**	the result datatype if they know the function instance descriptor
**	(returned by adi_resolve, adi_fidesc, etc.) and input datatypes.
**
**          adi_restype - find result type given inputs 
[@func_list@]...
**
**
**  History:
**      08-jun-89 (jrb)
**	    Initial creation.
**	27-jun-89 (jrb)
**	    Made any() return a non-nullable result in adi_res_type.
**	09-mar-92 (jrb, merged by stevet)
**	    Added special case for iftrue op for outer join work.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	17-Mar-1999 (shero03)
**	    support array of operands.
[@history_template@]...
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
** Name: adi_res_type	- return result type given fi descriptor and input dts
**
** Description:
**	This routine accepts a function instance descriptor and 0, 1, or 2
**	datatypes and returns the datatype of the result.  The result type is
**	already available in the fi descriptor field "adi_dtresult," but this
**	routine applies rules about nullability of the result depending on the
**	nullability of the input(s) and sometimes the operation being performed
**	and the query language in use.
**
**	The current rules for nullability are:
**
**	    If any input is nullable, then the result type is nullable.  If
**	    there are no inputs (i.e. the operation takes 0 parameters) then
**	    the result type is non-nullable.
**
**	Exceptions to the above:
**	
**	    Aggregates are an exception: for QUEL all aggregate results are
**	    non-nullable (and the result of a QUEL aggregate over 0 rows is the
**	    default value for the result datatype of the aggregate).  For SQL
**	    all aggregate results are nullable (and the result of an SQL
**	    aggregate over 0 rows is the NULL value).
**
**	    Another exception: count(column_name), count(*), and any() have a
**	    non-nullable result type even in SQL (and, of course, the result of
**	    a count() or an any() over 0 rows is 0).
**
**	    Another exception: iftrue(s, v) returns the nullable version of v's
**	    type regardless of whether v is nullable or not.
**	    
**	    And another: Last_identity() is always nullable even though it
**	    has no parameters.
**
** Inputs:
**	adf_scb				Pointer to an adf session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.  This is
**					necessary to determine the result type
**					of certain aggregate operations.
**	adi_fidesc			A pointer to the fi descriptor being
**					used (usually determined using
**					adi_resolve).
**	adi_dt1				The first datatype, if any.
**	adi_dt2				The second datatype, if any.
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
**	    adi_res_dt			The result datatype.
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
**      08-jun-89 (jrb)
**	    Initial creation.
**	27-jun-89 (jrb)
**	    Added any() to the list of aggregate functions which always have a
**	    non-nullable result type.  This shouldn't have been necessary since
**	    any() is QUEL-only, and QUEL aggregates always have a non-nullable
**	    result type, but OPF flattening code uses any() with SQL queries.
**	09-mar-92 (jrb, merged by stevet)
**	    Made iftrue op a special case since this operation always returns
**	    the nullable version of its second parameter contrary to the normal
**	    rule listed above.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	17-Mar-1999 (shero03)
**	    support array of operands.
**	7-Apr-2007 (kschendel)
**	    Support exact-type UDF's and functions with more than two
**	    input operands.
**	20-oct-2009 (stephenb)
**	    Add last_identity exception
**	18-Mar-2010 (kiria01) b123438
**	    Support SINGLETON aggregate for scalar sub-query support.
**	28-Jul-2010 (kiria01) b124142
**	    SINGLETON should always be nullable.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_res_type(
ADF_CB		*adf_scb,
ADI_FI_DESC	*adi_fidesc,
i4		adi_numdts,
DB_DT_ID	adi_dt[],
DB_DT_ID	*adi_res_dt)
{
    ADI_OP_ID	    op   = adi_fidesc->adi_fiopid;
    i4		    nops = adi_fidesc->adi_numargs;

    /* special case for iftrue op, always nullable */
    if (op == ADI_IFTRUE_OP)
    {
	*adi_res_dt = -abs(adi_dt[1]);
	return(E_DB_OK);
    }
    /* also for last_identity() which is always nullable */
    else if (op == ADI_LASTID_OP)
    {
	*adi_res_dt = -abs(adi_fidesc->adi_dtresult);
	return(E_DB_OK);
    }

    /* assume non-nullable (or nullable UDF) */
    *adi_res_dt = adi_fidesc->adi_dtresult;

    if (adi_fidesc->adi_fitype == ADI_AGG_FUNC)
    {
	if (adf_scb->adf_qlang == DB_SQL
	  &&  op != ADI_CNT_OP
	  &&  op != ADI_CNTAL_OP
	  &&  op != ADI_ANY_OP)
	    *adi_res_dt = - *adi_res_dt;	/* Agg result is nullable */
    }
    else if (*adi_res_dt > 0
       && (adi_fidesc->adi_finstid < ADD_USER_FISTART
	   || adi_fidesc->adi_npre_instr != ADE_0CXI_IGNORE) )
    {
	/* Non-aggregate, and not a user FI marked NONULLSKIP.  (Can't
	** look at NONULLSKIP directly because it doesn't fit in fiflags!)
	** Determine result nullability based on inputs nullability.
	*/
	while (--nops >= 0)
	{
	    if (adi_dt[nops] < 0)
	    {
		/* Nullable operand means nullable result */
		*adi_res_dt = - *adi_res_dt;
		break;
	    }
	}
    }
    
    return(E_DB_OK);	
}

/*
[@function_definition@]...
*/
