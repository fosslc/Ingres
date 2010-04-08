/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h>
# include	<afe.h>
# include	<er.h>
# include	<erfi.h>
# include	"erfv.h"
# include	<ug.h>

/*
**	EVALTREE.c  -  Evaluate the Validation Tree of a Field
**
**	These routines evaluate the validation tree of the field
**	passed to v_evalfld().	Evaltree() recursively evaluates
**	nodes of a tree and passes information back in union structures.
**	This routine does cross checking and range checking of variables.
**	It uses the tree created by the parser in validation.
**
**	V_EVALFLD -
**
**	 Arguments:  fld - field to be evaluated
**
**	 Returns:  0  -	 Evaluation FALSE
**		   1  -	 Evaluation TRUE
**
**	V_EVALTREE -
**
**	 Arguments:  result  -	union of types of results to be returned
**		     node    -	node to be evaluated
**
**	 Returns:  0  -	 Error in evaluation
**		   1  -	 Evaluation ok
**
**	 History:
**		19-jun-87 (bruceb) Code cleanup.
**		08/14/87 (dkh) ER changes.
**		10/16/87 (dkh) - Made error messages trappable.
**		11/20/87 (dkh) - Changed handling of LIKE operator.
**		12/12/87 (dkh) - Changed swinerr to IIUGerr.
**		09/14/89 (dkh) - Fixdd bug 7402.
**		18-apr-90 (bruceb)
**			Lint changes; vl_evalfld now has only 1 arg.
**			Call vl_chklist with 3 args, not 4.
**              04/25/90 (esd) - If field fails validation, give FT3270
**                               an opportunity to position cursor
**                               and set frcurfld and tfcurrow/col.
**                               Add val and frm as 2nd and 3rd parms to
**				 vl_evalfld.
**		03-jul-90 (bruceb)	Fix for 31018.
**			Call IIFDfdeFldDataError() instead of IIFDerror().
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-oct-2009 (joea)
**          Declare compres in vl_comp as i1 to match DB_BOO_TYPE usage.
*/


# ifndef	reg
# define	reg	register
# endif


i4	vl_comp();

FUNC_EXTERN	i4	vl_coerce();
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	DB_LANG	IIAFdsDmlSet();
FUNC_EXTERN	VOID	IIFDfdeFldDataError();


vl_evtree(result, node) /* EVALUATE TREE: */
reg VDATA	*result;		/* return result value */
reg VTREE	*node;			/* pointer to a node in a tree */
{
	VDATA	res1;			/* result of left branch of tree */
	VDATA	res2;			/* result of right branch of tree */
	FLDVAL	*FDgetval();		/* added routine declaration -nml */
	i4	isnull;


	/*
	**  EVALUATION OF NODE -
	**
	**  Nodes are evaluated according to their type.  There are
	**  three different types - constants, attributes or fields,
	**  unary v_operators, and binary v_operators.	The node returns
	**  a value in correspondence with its type.
	*/
	switch (node->v_nodetype)
	{
	  /*
	  **  CONSTANT NODE -
	  **
	  **  Constant nodes contain constant values to be evaluated
	  **  against various fields.  There are three type of constants -
	  **  FLOATS, INTEGERS, and CHARACTERS.	 The values are returned
	  **  in the union of the result relation.
	  */
	  case vCONST:
		return (vl_getval(node, (FLDVAL *) NULL, result));

	  /*
	  **  ATTRIBUTE NODE -
	  **
	  **  Attribute or field nodes store a pointer to a field in a frame.
	  **  Here the value is extracted from the field and placed in the
	  **  result union value.  The node and field are evaluated according
	  **  to their data type - DB_CHR_TYPE, DB_INT_TYPE, or DB_FLT_TYPE.
	  */
	  case vATTR:
	  {
		FLDVAL	*val;

		val = FDgetval(node->v_data.v_fld);

		return (vl_getval(node, val, result));
	  }

	  /*
	  **  COLUMN NODE -
	  **
	  **  A column node selects a column from a table field.
	  */
	  case vCOLSEL:
	  {
		TBLFLD	*tbl;
		i4	colnum;
		FLDVAL	*val;


		tbl = node->v_left->v_data.v_fld->fld_var.fltblfld;
		colnum = node->v_data.v_oper;
		val = fdtblacc(tbl, tbl->tfcurrow, colnum);

		return (vl_getval(node, val, result));
	  }

	  /*
	  **  UNARY OPERATOR NODE -
	  **
	  **  Presently there is only one type of unary v_operator, the NOT.
	  **  Only the right side of the tree is evaluated and the result
	  **  returned is complimented.	 This, in turn is returned to whomever
	  **  called this node.
	  */
	  case vUOP:


		switch (node->v_data.v_oper)
		{
		  case v_opNULL:
			if (vl_evtree(&res1, node->v_right.v_treright))
			{
				if (adc_isnull(FEadfcb(), &(res1.v_dbv),
					&isnull) == OK && isnull)
				{
					result->v_res = TRUE;
				}
				else
				{
					result->v_res = FALSE;
				}
				return(TRUE);
			}
			return(FALSE);

		  case v_opNOTNL:
			if (vl_evtree(&res1, node->v_right.v_treright))
			{
				if (adc_isnull(FEadfcb(), &(res1.v_dbv),
					&isnull) == OK && !isnull)
				{
					result->v_res = TRUE;
				}
				else
				{
					result->v_res = FALSE;
				}
				return(TRUE);
			}
			return(FALSE);

		  case v_opNOT:

			if (vl_evtree(&res1, node->v_right.v_treright))
			{


				if (res1.v_res == V_NULL)
				{
					result->v_res = res1.v_res;
				}
				else
				{
					result->v_res = !(res1.v_res);
				}
				return (TRUE);
			}

		  case v_opMINUS:
			IIUGerr(E_FV0002_Bad_unary_function, UG_ERR_ERROR, 0);
			return(FALSE);
		}

	  /*
	  **  BINARY OPERATOR NODE -
	  **
	  **  The binary node evaluates both sides of the tree at the node and
	  **  performs whatever binary operation is called for on the tree.
	  **  Currently, only logical and boolean binary operations are
	  **  performed.  They are AND, OR, EQUAL, LESS THAN, LESS THAN or
	  **  EQUAL, GREATER THAN, GREATER THAN or EQUAL, and NOT EQUAL.
	  **
	  **  Some assumptions are made.  Logical v_operators only operate on
	  **  other logical or boolean v_operators.  Boolean v_operators operate
	  **  on attributes of the same type.  Boolean v_operators only operate
	  **  on attributes or constants.  The parser should take care of these
	  **  conditions.
	  **
	  **  Boolean v_operators extract the values of their left and right
	  **  subtrees and perform whatever operation on them.
	  */
	  case vBOP:


		/*
		**  Left and right values are extracted first.	Their evaluation
		**  is independent of the operation performed on them.	This is
		**  the reason for the assumption.  Performing the operation
		**  should require as little code as possible.
		*/
		if (vl_evtree(&res1, node->v_left) == FALSE)
			return (FALSE);
		if (vl_evtree(&res2, node->v_right.v_treright) == FALSE)
			return (FALSE);

		result->v_res = vl_comp(node, node->v_data.v_oper,
			&res1, &res2);
		return (TRUE);

	  case vLIST:

		/*
		** removed comparison of datatypes between list and
		** left node, since this is done in resolvtre if
		** getlist is involved, and in maketree otherwise.
		*/

		if (vl_evtree(&res1, node->v_left) == FALSE)
			return (FALSE);

		if (vl_chklist(&res2, &res1, node->v_right.v_lstright) == FALSE)
		{
			return (FALSE);
		}
		result->v_res = res2.v_res;


		return (TRUE);
	}

	return (FALSE);
}

vl_evalfld(type, val, frm)		/* EVALUATE FIELD: */
FLDTYPE *type;
FLDVAL	*val;
FRAME	*frm;
{
	VDATA result;			/* union containing result */


	if (type->ftvalchk == NULL)	/* if no tree to evaluate */
		return (TRUE);			/* return true */
	/*
	**  Evaluate the tree of a field.  If evaltree retuns 0 the evaluation
	**  was unsuccessful.  Corrupt tree or something.  Print out an
	**  appropriate message on level above.
	*/
	if(vl_evtree(&result, type->ftvalchk) == FALSE)
		return (FALSE);

	/*
	**  Result can be V_NULL, TRUE or FALSE.  Validation
	**  check passes if result is V_NULL or TRUE.
	*/
	if (result.v_res == FALSE)
	{
		if (type->ftvalmsg != NULL)
		{
			if (type->ftvalmsg[0] != '\0')
			{
# ifdef FT3270
                                IIFTfbfFlagBadField(frm, val);
# endif
				IIFDfdeFldDataError(E_FI2199_8601, 1,
				    type->ftvalmsg);
				return (FALSE);
			}
		}
		IIFDfdeFldDataError(E_FI2198_8600, 0);
		return (FALSE);
	}
	else
	{
		return (TRUE);
	}
}

i4
vl_comp(node, oper, res1, res2)
VTREE		*node;
i4		oper;
reg VDATA	*res1, *res2;
{
	AFE_OPERS	ops;
	DB_DATA_VALUE	*oparr[2];
	DB_DATA_VALUE	resdbv;
	ADF_CB		*ladfcb;
	ADI_FI_ID	fid;
	i4		isnull;
	i1		compres;
	DB_LANG		olddml = DB_NOLANG;

	switch (oper)
	{
	  case v_opAND:
		if (res1->v_res == FALSE || res2->v_res == FALSE)
		{
			return(FALSE);
		}
		else if (res1->v_res == V_NULL || res2->v_res == V_NULL)
		{
			return(V_NULL);
		}
		else
		{
			return(TRUE);
		}

	  case v_opOR:
		if (res1->v_res == TRUE || res2->v_res == TRUE)
		{
			return(TRUE);
		}
		else if (res1->v_res == V_NULL || res2->v_res == V_NULL)
		{
			return(V_NULL);
		}
		else
		{
			return(FALSE);
		}

	  default:
		break;
	}

	/*
	**  Always use plain DB_BOO_TYPE since check for
	**  field having a NULL value is done above.  No
	**  need to have a nullable bool type here.
	*/
	resdbv.db_datatype = DB_BOO_TYPE;
	resdbv.db_length = sizeof(compres);
	resdbv.db_prec = 0;
	resdbv.db_data = (PTR) &compres;

	ops.afe_opcnt = 2;
	ops.afe_ops = oparr;
	ops.afe_ops[0] = &(res1->v_dbv);
	ops.afe_ops[1] = &(res2->v_dbv);
	ladfcb = FEadfcb();
	if (adc_isnull(ladfcb, &(res1->v_dbv), &isnull) != OK)
	{
		return(FALSE);
	}
	if (isnull)
	{
		return(V_NULL);
	}
	if (adc_isnull(ladfcb, &(res2->v_dbv), &isnull) != OK)
	{
		return(FALSE);
	}
	if (isnull)
	{
		return(V_NULL);
	}

	if ((fid = node->v_fiid) == ADI_NOFI)
	{
		IIUGerr(E_FV0003_Inconsistency_in_data, UG_ERR_ERROR, 0);
		return(FALSE);
	}

	if (oper == v_opLK)
	{
		olddml = IIAFdsDmlSet(DB_SQL);
	}

	if (afe_clinstd(ladfcb, fid, &ops, &resdbv) != OK)
	{
		if (olddml != DB_NOLANG)
		{
			olddml = IIAFdsDmlSet(olddml);
		}
		IIUGerr(E_FV0004_Internal_error, UG_ERR_ERROR, 0);
		return(FALSE);
	}

	if (olddml != DB_NOLANG)
	{
		olddml = IIAFdsDmlSet(olddml);
	}

	return ((i4) compres);
}

vl_getval(node, val, result)
reg	VTREE	*node;
reg	FLDVAL	*val;
reg	VDATA	*result;
{
	i4	type;
	DB_DATA_VALUE	*dbv1ptr;
	DB_DATA_VALUE	*dbv2ptr;
	DB_DATA_VALUE	*cdbvptr;
	DB_DATA_VALUE	*oparr[2];
	AFE_OPERS	ops;

	type = node->v_nodetype;
	dbv1ptr = &(result->v_dbv);
	if (type == vCONST)
	{
		dbv2ptr = &(node->v_data.v_dbv);
	}
	else
	{
		dbv2ptr = val->fvdbv;
		if (node->v_right.v_atcvt != NULL)
		{
			cdbvptr = node->v_right.v_atcvt->vc_dbv;
			ops.afe_opcnt = 1;
			ops.afe_ops = oparr;
			ops.afe_ops[0] = dbv2ptr;
			if (afe_clinstd(FEadfcb(),
				node->v_right.v_atcvt->vc_fiid, &ops,
				cdbvptr) != OK)
			{
				IIUGerr(E_FV0005_Conversion_error,
					UG_ERR_ERROR, 0);
				return(FALSE);
			}
			dbv2ptr = cdbvptr;
		}
	}

	MEcopy((PTR) dbv2ptr, (u_i2) sizeof(DB_DATA_VALUE), (PTR) dbv1ptr);

	return (TRUE);
}
