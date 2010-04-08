/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<ade.h>

/**
** Name:	affdesc.c -	Front-End ADF Return Function Instance Module.
**
** Description:
**	Contains a routine used to determine a function instance for a given
**	operator and its operands.  Defines:
**
** 	afe_fdesc()	return function instance for operator/operands.
**
** History:
**	Revision 6.4  90/02  wong
**	Modified to use ADF type resolution routines.
**
**	Revision 6.3  89/09  wong
**	Corrected and optimized no coercion loop.
**
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**
**	06/27/92 (dkh) - Added support for decimal datatype for 6.5.
**			 "coplength" fixed to handle decimal datatypes.
**	11/08/92 (dkh) - Added support for DB_ALL_TYPE.
**	7-sep-1993 (mgw) Bug #54250
**		The decimal datatype handling in "coplength" accidentally
**		broke nullability coersion by circumventing the
**		AFE_MKNULL_MACRO() call. This fixes that.
**	1-mar-94 (donc) Bug 59413
**		The decimal datatype handling in coplength() requires access
**		to the actual db_data generated for the i2 representation of
**		precision/scale.  This value is actually passed to the ADF
**		routine for verification of acceptable precision and scale
**		settings
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	10-may-1999 (shero03)
**	    Support Multivariate Functions (up to ADI_MAX_OPERANDS)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-feb-2001 (somsa01)
**	    In afe_fdesc(), changed ffound to be a boolean, and corrected
**	    its setting.
**/

DB_STATUS	adi_fitab();
DB_STATUS	adi_resolve();

static DB_STATUS	_error();
static DB_STATUS	coplength();

/*{
** Name:	afe_fdesc() -	Return Function Instance for Operator/Operands.
**
** Description:
**	Returns a function instance descriptor for a function given an operator
**	ID and the type of its operands.  (This routine looks at a table of all 
**	ADI_FI_DESCs for the given operator, and selects the one whose operand
**	data types best match the data types of the input operands.)  Also
**	returns the result type and length of the function. 
**
**	If no ADI_FI_DESC exists for the operator and the operand
**	types passed in, this routine will attempt to coerce the 
**	operand types into types which DO have an ADI_FI_DESC for the
**	given operator.  This routine will indicate which data type 
**	(if any) had to be coerced, and what type it had to be coerced into. 
**
**	NOTE: If coercion is not desired, then the coercops parameter should
**	be set to NULL.
**
** Inputs:
**	cb	{ADF_CB *}  A reference to the current ADF control block.
**	opid	{ADI_OP_ID}  The operator ID for the function.
**
**	ops			A properly filled AFE_OPERS for the
**				input operands.  Only the type and length
**				of the operands is used.
**
**	coercops		A properly filled AFE_OPERS for the data type
**				to which the original operands must be coerced.
**				Alternatively, NULL can be passed in, 
**				indicating that coercion is not desired.
**
**	result			Must point to a DB_DATA_VALUE.
**
**	fdesc			Must point to a ADI_FI_DESC.
**
** Outputs:
**	coercops		Contains the data type to which each operand
**				must be coerced in order to use the given
**				function.
**
**	result			The result type and length of the function.
**
**		.db_datatype	The datatype of the result.
**		.db_length	The length of the result.
**
**	fdesc			Is set to be the ADI_FI_DESC for the function.
**
** Returns:
**	OK			If successful
**
**	E_DB_ERROR		Some error occured.
**
**	If OK is not returned, the error code in cb.adf_errcb.ad_errcode
**	can be checked. Possible values and their meanings are:
**
**	E_AF6001_WRONG_NUMBER	If there are too many or too few operands.
**
**	E_AF6006_OPTYPE_MISMATCH No function instance descriptors
**				exist with the operand types passed 
**				in for this operator ID.
**
**	E_AF6008_AMBIGUOUS_OPID	If the data types of two operands cannot be
**				resolved to a single result data type.
**
**	E_AF6019_BAD_OPCOUNT	The operand count (in the 'afe_opcnt' field of
**				the AFE_OPERS structure) did not match any of
**				the operand counts in the function instance
**				descriptors for this operator ID.
**	returns from:
**		'adi_fitab()', 'adi_resolve()', etc.
**
** History:
**	31-aug-1993 (rdrane)
**		Ensure that all ME calls have address arguments cast to PTR
**		so the CL prototyping doesn't complain.
**	02/90 (jhw)  Modified to use ADF type resolution.  (This supersedes the
**		fix  for #5289 and #7163.)
**	01/90 (jhw)  Allow Nullable booleans.  JupBug #7163.
**	09/89 (jhw)  Optimized search loops and corrected error handling for no
**			coercion case.
**	08/89 (jhw)  Corrected strict comparison loop because it was not working
**		if the number of operators was ever different than the number
**		of arguments.
**	06/89 (jhw)  Changed to call 'iiafComputeLen()' and pass the result DBV
**		to be filled in based on the result DT_ID.  JupBug #6585.
**	05/89 (jhw)  Do not make the result Nullable for booleans or 'ifnull()'.
**			JupBug  #6134.
**	04/89 (jhw)  Corrected coercion check loop; was failing to increment
**		function instance when arguments mis-matched.  JupBug #5289.
**	Written	2/6/87	(dpr)
**	27-feb-2001 (somsa01)
**	    Changed ffound to be a boolean, and corrected its setting.
**	8-june-01 (inkdo01)
**	    Fixed result type determination to account for nullable opnds.
**	29-aug-01 (inkdo01)
**	    This guy doesn't seem to know about the fidesc's with operands
**	    that accept any data type (DB_ALL_TYPE). This change recognizes
**	    them.
**	16-jul-02 (hayke02)
**	     Call adi_resolve() with a FALSE varchar_precedence. This change
**	     fixes bug 109134.
**      31-aug-2004 (sheco02)
**          X-integrate change 466442 to main.
**	25-Sep-2008 (kiria01) b120946
**	    Disable the string-numeric operators from the function scan when
**	    afe_fdesc() is called without the coerceops parameter. Noticed
**	    from OpenRoad comparing dates.
*/
DB_STATUS
afe_fdesc (cb, opid, ops, coercops, result, fdesc)
ADF_CB			*cb;
ADI_OP_ID		opid;
register AFE_OPERS	*ops;
AFE_OPERS		*coercops;
DB_DATA_VALUE		*result;
ADI_FI_DESC		*fdesc;
{
	AFE_OPERS		*which_ops;
	register ADI_FI_DESC	*best;
	DB_DT_ID		op[ADI_MAX_OPERANDS];
	DB_DT_ID		nullop[ADI_MAX_OPERANDS];
	ADI_FI_ID		coerce_fid;
	DB_STATUS		rval;
	i4			i;

	/*
	** We're trying to find the specific ADF_FI_DESC (the function instance
	** ID corresponding to a given operator (an operator ID) and the operand
	** datatypes passed in.  This is accomplished by calling 'adi_fitab()',
	** which returns all the function instances for the given operator.
	** These function instance IDs are then searched for the one best
	** matching the datatypes that were passed in given certain coercability
	** rules. This is then our ADF_FI_DESC.  We then allocate space for the
	** function instance descriptor and return it.
	*/
	if (ops->afe_opcnt > ADI_MAX_OPERANDS || ops->afe_opcnt < 0)
		return afe_error(cb, E_AF6001_WRONG_NUMBER, 0);

	for (i = 0; i < ADI_MAX_OPERANDS; i++)
	{
	    if (i >= ops->afe_opcnt)
	    {
	    	op[i] = DB_NODT;
		nullop[i] = DB_NODT;
	    }
	    else
	    {
	    	op[i] = AFE_DATATYPE(ops->afe_ops[i]);      
		nullop[i] = ops->afe_ops[i]->db_datatype;
	    }
	}

	if (coercops == NULL || ops->afe_opcnt == 0)
	{ /* No coercions */
		register i4	j;
		ADI_FI_TAB	ftab;
		bool		ffound;

		if ((rval = adi_fitab(cb, opid, &ftab)) != OK)
			return rval;
		/* 
		** we want an exact match only. flip though the table of 
		** ADI_FI_DESCs and find the one that matches. if none
		** match, we assume we've got a bad opid.
		*/
	
		best = ftab.adi_tab_fi;

		if ( best->adi_numargs != ops->afe_opcnt )
			return _error(cb, E_AF6019_BAD_OPCOUNT, opid, op[0], op[1]);

		j = ftab.adi_lntab;
		if (best->adi_fitype == ADI_COMPARISON &&
			best->adi_numargs == 2 &&
			best->adi_dt[0] == DB_ALL_TYPE &&
			best->adi_dt[1] == DB_ALL_TYPE &&
			j > 1)
		{
		    /* Skip the full wild comparisons as they are only
		    ** processed properly in the context of adi_resolve
		    ** and here we are expecting to find an exact match
		    ** in operands. */
		    j--;
		    best++;
		}
		while (--j >= 0)
		{
		    /* 
		    ** We've found our ADF_FI_DESC if the number of
		    ** arguments and both operator types match. 
		    */
		    if ( best->adi_numargs != ops->afe_opcnt)
		        continue;
		    ffound = TRUE;
		    for (i = 0; i < ops->afe_opcnt && ffound; i++)
		    {
			if (best->adi_dt[i] != op[i] &&
				best->adi_dt[i] != DB_ALL_TYPE)
			    ffound = FALSE;	 
		    }   
		    if (ffound)
		       break;
		    best++;
		} /* end for loop */

		if ( j < 0 )
		{
			return _error( cb, E_AF6006_OPTYPE_MISMATCH,
					opid, op[0], op[1]
			);
		}

		/*
		** Indicate that the set of operands to use comes from ops, 
		** not coercops. After this else clause, we'll set up the
		** result dbdv.
		*/
		which_ops = ops;
	}
	else
	{ /* Coerce */
		ADI_RSLV_BLK	fi_blk;

		/*
		** Coercops aren't NULL, so client wants us to use the longer
		** method, which deals with coersion of operand types. First
		** however, check if coercops was set up correctly
		*/

		if (coercops->afe_opcnt != ops->afe_opcnt)
		{
			return afe_error(cb, E_AF6001_WRONG_NUMBER, 0);
		}

		fi_blk.adi_op_id = opid;

		fi_blk.adi_fidesc = NULL;
		fi_blk.adi_num_dts = ops->afe_opcnt;
		for (i = 0; i < ops->afe_opcnt; i++)
		    fi_blk.adi_dt[i] = op[i];

		if ( (rval = adi_resolve(cb, &fi_blk, FALSE)) != OK )
		{ /* error! */
			DB_STATUS	err = cb->adf_errcb.ad_errcode;

			if ( err == E_AD2061_BAD_OP_COUNT )
			{
				return _error( cb, E_AF6019_BAD_OPCOUNT,
						opid, op[0], op[1]
				);
			}

			/* Complain if no applicable function found */
			if ( err == E_AD2062_NO_FUNC_FOUND )
			{
				return _error( cb, E_AF6006_OPTYPE_MISMATCH,
						opid, op[0], op[1]
				);
			}

			/* Complain if ambiguous function found */
			if ( err == E_AD2063_FUNC_AMBIGUOUS )
			{
				return _error( cb, E_AF6008_AMBIGUOUS_OPID,
						opid, op[0], op[1]
				);
			}

			return rval;
		}

		/* Found the best instance. Set the result datatype later.*/

		best = fi_blk.adi_fidesc;

		/*
		** Figure out the result's length.  First we must get the
		** lengths of the operands after conversion, then we must use
		** these lengths to figure out the length of the result.
		*/

		/* get the length of the each operand */
		for (i = 0; i < ops->afe_opcnt; i++)
		{	
		     rval = coplength( cb, best->adi_dt[i], ops->afe_ops[i], 
					coercops->afe_ops[i], &coerce_fid);
		     if (rval != OK)
		       return rval;
		}

		/*
		** Indicate that the set of operands to use will come from 
		** coercops, not ops. Then we'll set up the result dbdv.
		*/
		which_ops = coercops;

	}	/* else there ARE coerceops */

	/*
	** Now set up the result DB data value.  Before loading the length
	** determine the result type to account for Nullability.
	*/
	if ( best->adi_npre_instr == ADE_0CXI_IGNORE )
		result->db_datatype = best->adi_dtresult;
	else if ( (rval = adi_res_type( cb, best, ops->afe_opcnt, nullop,
					&result->db_datatype ))
			!= OK )
	{
		return rval;
	}

	/*
	** Now figure out the result length of the operator being resolved.
	** Then return the status; if 'iiafComputeLen()' returned an error,
	** we'll send that to our caller who can determine if they want to
	** proceed or not.  If it completed OK, then all is well!
	*/
	rval = iiafComputeLen(cb, best, which_ops, result);

	/* copy the data in our best found function descriptor into fdesc */
	MEcopy((PTR)best, sizeof(*fdesc), (PTR)fdesc);

	return rval;
}


static DB_STATUS
coplength (cb, operand_type, opval, copval, coerce_fidp)
ADF_CB		*cb;
DB_DT_ID	operand_type;
DB_DATA_VALUE	*opval;
DB_DATA_VALUE	*copval;
ADI_FI_ID	*coerce_fidp;
{
	DB_STATUS	rval;
	DB_DATA_VALUE	tdbv;

	/* Assume no coercion for now */
	copval->db_datatype = opval->db_datatype;
	copval->db_length = opval->db_length;
	copval->db_prec = opval->db_prec;
	copval->db_data = opval->db_data;

	/*
	**  If the datatype is DB_ALL_TYPE, then
	**  any datatype is valid.  So we don't need
	**  to have any coercion.
	*/
	if (operand_type == DB_ALL_TYPE)
	{
		return(OK);
	}

	/* 
	** Get coercion ID for the operand, if needed.  Compare the absolute
	** value of the operand as it may be nullable.  The "operand_type"
	** variable comes is from the 'adi_fitab[]' table, which does not
	** contain nullable (negative) datatypes.
	*/
	if (operand_type != abs(opval->db_datatype))
	{ /* Coerce operand */

		copval->db_datatype = operand_type;
		copval->db_length = 0;
		/*
		** if the original operand is Nullable, make the coercion
		** operand Nullable as well.
		*/
		if (AFE_NULLABLE_MACRO(opval->db_datatype))
		{
			AFE_MKNULL_MACRO(copval);
		}

		/*
		** Fix for bug 54250 - use copval->db_datatype here
		** instead of operand_type to take advantage of the
		** AFE_MKNULL_MACRO() call above.
		*/
		tdbv.db_datatype = copval->db_datatype;
		tdbv.db_length = 0;
		tdbv.db_prec = 0;
		rval = afe_ficoerce(cb, opval, &tdbv, coerce_fidp);
		if (rval != OK)
			return rval;

		/* Remember the result length of the coercion */
		copval->db_length = tdbv.db_length; 
		copval->db_prec = tdbv.db_prec;
	}

	return OK;
}

static DB_STATUS
_error ( cb, err, opid, op1, op2 )
ADF_CB		*cb;
ER_MSGID	err;
ADI_OP_ID	opid;
DB_DT_ID	op1;
DB_DT_ID	op2;
{
	DB_STATUS	rval;
	ADI_OP_NAME	opname;
	ADI_DT_NAME	op1type;
	ADI_DT_NAME	op2type;

	if ( (rval = adi_opname(cb, opid, &opname)) == OK ||
			(rval = adi_tyname(cb, abs(op1), &op1type)) == OK ||
			(rval = adi_tyname(cb, abs(op2), &op2type)) == OK )
		return afe_error( cb, err, 0, /* 0 for now until we can change
						messages.  (jhw) 06/89 */
					(PTR)opname.adi_opname,
					(PTR)op1type.adi_dtname,
					(PTR)op2type.adi_dtname
		);
	return rval;
}
