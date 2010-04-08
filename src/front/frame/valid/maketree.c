/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h>
# include	<ds.h>
# include	<feds.h>
# include	<afe.h>
# include	<me.h>
# include	<er.h>
# include	"erfv.h"

/*
**	MAKETREE.c  -  Make a node in the validation tree
**
**	This routine creates the nodes of the validation tree
**	according to their function.  Operators are stored as
**	branch nodes and values and fields as leaf nodes.  Along
**	with the data in the node the node type is stored.
**	Binary v_operators check to see if the data types of the two
**	branches are the same.
**
**	Checks to see if syntax only analysis being done
**
**	Arguments:  value     -	 Unions of the values to be placed
**		    nodetype  -	 type of the node
**				 (binop, unop, const, field)
**		    datatype  -	 data type of the node
**		    left      -	 pointer to the left branch
**		    right     -	 pointer to the right branch
**
**	History:  15 Dec   1981 - written (JEN)
**		  20 April 1982 - modified (JRC)  (syntax only)
**		  05-jan-1987 (peter)	Changed calls to IIbadmem.
**	04/07/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bab) Code cleanup.
**	06/20/87 (dkh) - Fixed to allow nullable bool type for result.
**	08/14/87 (dkh) - ER changes.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	10/09/87 (dkh) - Fixed to allow comparison of two fields.
**	11/20/87 (dkh) - Changed handling of LIKE operator.
**	02/04/88 (dkh) - By decree of LRC, all string constants are varchar.
**	07-apr-88 (bruceb)
**		Changed length determination from sizeof(DB_TEXT_STRING)-1
**		to DB_CNTSIZE.  The prior calculation is incorrect.
**	08/31/89 (dkh) - Changed YYSTYPE to IIFVvstype.
**	09/15/89 (dkh) - Fixed bug 7370.
**	09/18/90 (dkh) - Fixed bug 33299.
**	09/18/90 (dkh) - Fixed bug 32960.
**	09/26/90 (dkh) - Reworked datatype compatibility checking to allow
**			 comparison of money to ints while still disallowing
**			 comparison of characters to ints.
**	06/13/92 (dkh) - Added support for decimal datatype for 6.5.
**	
**	27-jul-92 (thomasm) - turn off optimization for hp8
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	19-jan-1999 (muhpa01)
**	    Removed NO_OPTIM for hp8_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


FUNC_EXTERN	VOID	vl_typstr();
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	FLDTYPE *FDgettype();
FUNC_EXTERN	DB_LANG	IIAFdsDmlSet();
FUNC_EXTERN	FLDHDR	*FDgethdr();

FUNC_EXTERN	DB_STATUS    adi_0calclen();

GLOBALREF	FLDHDR	*IIFVflhdr;


# ifndef abs
# define	abs(x)	((x) >= 0 ? (x) : (-(x)))
# endif /* abs */

# ifndef reg
# define	reg	register
# endif /* reg */

VTREE *
vl_maketree(value, nodetype, datatype, left, right)	/* MAKE TREE: */
reg IIFVvstype	*value;			/* union of data types */
reg i4		nodetype;		/* type of node */
reg i4		datatype;		/* data type of node */
VTREE		*left;			/* ptr to left branch */
VTREE		*right;			/* ptr to right branch */
{
	VTREE		*result;	/* result to pass back */
	DB_DATA_VALUE	*ldbvptr;
	DB_DATA_VALUE	*rdbvptr;
	DB_DATA_VALUE	ldbv;
	DB_DATA_VALUE	rdbv;
	DB_TEXT_STRING	*text;
	FIELD		*fld;
	FLDTYPE		*type;
	FLDCOL		*col;
	TBLFLD		*tbl;
	i4		*i4ptr;
	f8		*f8ptr;
	char		*f_name;

	f_name = ERx("MAKETREE");
	/* create a node */
	if ((result = (VTREE *)MEreqmem((u_i4)0, (u_i4)(sizeof(VTREE)), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vl_maketree"));
	}

	switch (nodetype)	/* process the node according to its datatype */
	{
	  case vUOP:			/* UNARY OPERATOR */
		if (value->type_type != v_opNOT && value->type_type != v_opNULL
			&& value->type_type != v_opNOTNL)
		{
			vl_par_error(f_name, ERget(E_FV000E_parser_error));
			return(NULL);
		}
		result->v_data.v_oper = value->type_type;
		break;

	  case vBOP:			/* BINARY OPERATOR */
		/* check to see that the data types of the two sides match */
		/*
		**  Also, must force coercion of the right side if
		**  needed.
		*/
		if (left->v_nodetype == vCONST && right->v_nodetype == vCONST)
		{
			vl_par_error(f_name, ERget(E_FV000F_Does_not_make_sense_t));
			return(NULL);
		}

		/*
		**  Can't compare a simple to a table field column.
		**  Give error and exit.
		*/
		if (left->v_nodetype == vATTR &&
			right->v_nodetype == vCOLSEL)
		{
			fld = left->v_data.v_fld;
			if (IIFVflhdr == FDgethdr(fld))
			{
				vl_par_error(f_name,
					ERget(E_FV001F_simple_to_col));
				return(NULL);
			}
		}
		if (left->v_nodetype == vCOLSEL &&
			right->v_nodetype == vATTR)
		{
			fld = right->v_data.v_fld;
			if (IIFVflhdr == FDgethdr(fld))
			{
				vl_par_error(f_name,
					ERget(E_FV001F_simple_to_col));
				return(NULL);
			}
		}

		result->v_data.v_oper = value->type_type;
		if (left->v_nodetype == vATTR || left->v_nodetype == vCOLSEL ||
			right->v_nodetype == vATTR ||
			right->v_nodetype == vCOLSEL)
		{
			if (left->v_nodetype == vATTR)
			{
				fld = left->v_data.v_fld;
				type = FDgettype(fld);
				ldbv.db_datatype = type->ftdatatype;
				ldbv.db_length = type->ftlength;
				ldbv.db_prec = type->ftprec;
				ldbvptr = &ldbv;
			}
			else if (left->v_nodetype == vCOLSEL)
			{
				fld = left->v_left->v_data.v_fld;
				tbl = fld->fld_var.fltblfld;
				col = tbl->tfflds[left->v_data.v_oper];
				type = &(col->fltype);
				ldbv.db_datatype = type->ftdatatype;
				ldbv.db_length = type->ftlength;
				ldbv.db_prec = type->ftprec;
				ldbvptr = &ldbv;
			}
			else
			{
				ldbvptr = &(left->v_data.v_dbv);
			}
			if (right->v_nodetype == vATTR)
			{
				fld = right->v_data.v_fld;
				type = FDgettype(fld);
				rdbv.db_datatype = type->ftdatatype;
				rdbv.db_length = type->ftlength;
				rdbv.db_prec = type->ftprec;
				rdbvptr = &rdbv;
			}
			else if (right->v_nodetype == vCOLSEL)
			{
				fld = right->v_left->v_data.v_fld;
				tbl = fld->fld_var.fltblfld;
				col = tbl->tfflds[right->v_data.v_oper];
				type = &(col->fltype);
				rdbv.db_datatype = type->ftdatatype;
				rdbv.db_length = type->ftlength;
				rdbv.db_prec = type->ftprec;
				rdbvptr = &rdbv;
			}
			else
			{
				rdbvptr = &(right->v_data.v_dbv);
			}
			if (!vl_docoerce(ldbvptr, rdbvptr, left, right, result,
				NULL))
			{
				return (NULL);
			}
		}
		break;

	  case vLIST:			/* LIST OF CONSTANTS */
		/* make sure their datatypes match */
		/* zap FLOAT4 check, since possible only from getlist() */
		if (right == NULL && !vl_syntax)
		{
			vl_par_error(f_name, ERget(E_FV0010_Parser_error));
			return(NULL);
		}
		if (right != NULL &&
			!vl_lscoerce(left, (VALROOT *)right))
		{
			/*
			**  Error message already put out by vl_lscoerce().
			*/
			return(NULL);
		}
		result->v_data.v_oper = 0;	/* really not needed */
		break;

	  case vCONST:			/* CONSTANT */
		ldbvptr = &(result->v_data.v_dbv);
		switch (datatype) /* set up the three different data types */
		{
		  case vVCHSTRING:
		  case vSTRING:			/* STRING */
			/* assign the v_string pointer to the union */
			text = (DB_TEXT_STRING *) value->string_type;
			ldbvptr->db_datatype = DB_VCH_TYPE;
			ldbvptr->db_length = text->db_t_count + DB_CNTSIZE;
			ldbvptr->db_prec = 0;
			ldbvptr->db_data = (PTR) text;
			break;

		  case vINT:			/* INTEGER */
			/* assign the integer */
			ldbvptr->db_datatype = DB_INT_TYPE;
			ldbvptr->db_length = 4;
			ldbvptr->db_prec = 0;
			if ((i4ptr = (i4 *)MEreqmem((u_i4)0, (u_i4)sizeof(i4),
			    FALSE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			*i4ptr = value->I4_type;
			ldbvptr->db_data = (PTR) i4ptr;
			break;

		  case vFLOAT:			/* FLOATING POINT */
			/* assign the floating point v_number */
			ldbvptr->db_datatype = DB_FLT_TYPE;
			ldbvptr->db_length = 8;
			ldbvptr->db_prec = 0;
			if ((f8ptr = (f8 *)MEreqmem((u_i4)0, (u_i4)sizeof(f8),
			    FALSE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			*f8ptr = value->F8_type;
			ldbvptr->db_data = (PTR) f8ptr;
			break;

		  case vDEC:		/* DECIMAL CONSTANT */
			ldbvptr->db_datatype = DB_DEC_TYPE;
			ldbvptr->db_length = value->dec_type->db_length;
			ldbvptr->db_prec = value->dec_type->db_prec;
			if ((ldbvptr->db_data = MEreqmem((u_i4) 0,
				(u_i4)ldbvptr->db_length, FALSE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			MEcopy(value->dec_type->db_data,
				(u_i2) ldbvptr->db_length, ldbvptr->db_data);
			break;
		}
		break;
	}

	result->v_nodetype = nodetype;	/* set the node type */
	result->v_constype = datatype;
	result->v_left = left;	/* and assign the left and right branches */
	if (nodetype == vLIST)
	{
		result->v_right.v_lstright = (VALROOT *)right;
	}
	else
	{
		result->v_right.v_treright = right;
	}

	return (result);
}


i4
vl_docoerce(ldbvptr, rdbvptr, left, right, node, vlist)
DB_DATA_VALUE	*ldbvptr;
DB_DATA_VALUE	*rdbvptr;
VTREE		*left;
VTREE		*right;
VTREE		*node;
VALLIST		*vlist;
{
	DB_DATA_VALUE	*xdbvptr;
	DB_DATA_VALUE	*oparr[2];
	DB_DATA_VALUE	*coparr[2];
	DB_DATA_VALUE	cldbv;
	DB_DATA_VALUE	crdbv;
	DB_DATA_VALUE	*cldbvptr;
	DB_DATA_VALUE	*crdbvptr;
	DB_DATA_VALUE	resdbv;
	AFE_OPERS	ops;
	AFE_OPERS	cops;
	ADI_OP_ID	opid;
	ADI_FI_ID	cfid;
	ADI_FI_DESC	fdesc;
	ADF_CB		*ladfcb;
	char		*opname;
	char		ltybuf[100];
	char		rtybuf[100];
	char		*f_name;
	DB_LANG		olddml = DB_NOLANG;
	bool		compatible;
	i2		prec;

	f_name = ERx("vl_docoerce");

	ladfcb = FEadfcb();

	switch(node->v_data.v_oper)
	{
		case v_opEQ:
			opname = ERx("=");
			break;

		case v_opLK:
			opname = ERx("like");
			olddml = IIAFdsDmlSet(DB_SQL);
			break;

		case v_opNE:
			opname = ERx("!=");
			break;

		case v_opLE:
			opname = ERx("<=");
			break;

		case v_opGE:
			opname = ERx(">=");
			break;

		case v_opLT:
			opname = ERx("<");
			break;

		case v_opGT:
			opname = ERx(">");
			break;

		default:
			vl_par_error(f_name, ERget(E_FV0011_Unkown_operator_found));
			return(FALSE);
	}
	if (afe_opid(ladfcb, opname, &opid) != OK)
	{
		if (olddml != DB_NOLANG)
		{
			olddml = IIAFdsDmlSet(olddml);
		}
		vl_par_error(f_name, ERget(E_FV0012_Error_getting_operato));
		return(FALSE);
	}

	if (olddml != DB_NOLANG)
	{
		olddml = IIAFdsDmlSet(olddml);
	}

	cldbvptr = &cldbv;
	crdbvptr = &crdbv;
	ops.afe_opcnt = 2;
	cops.afe_opcnt = 2;
	ops.afe_ops = oparr;
	cops.afe_ops = coparr;
	ops.afe_ops[0] = ldbvptr;
	ops.afe_ops[1] = rdbvptr;
	cops.afe_ops[0] = cldbvptr;
	cops.afe_ops[1] = crdbvptr;

	/*
	**  Check to see if the two DBVs are compatible.  Don't want
	**  to depend on afe_fdesc() since it sometimes coerce values
	**  to money before comparing, which is not what is desired
	**  in some case (e.g., comparing a c to an int should not
	**  force both to money before comparing).
	*/
	if (afe_fdesc(ladfcb, opid, &ops, &cops, &resdbv, &fdesc) != OK ||
		(rdbvptr->db_datatype != crdbvptr->db_datatype &&
		ldbvptr->db_datatype != cldbvptr->db_datatype))
	{
		vl_typstr(ldbvptr, ltybuf);
		vl_typstr(rdbvptr, rtybuf);
		vl_par_error(f_name, ERget(E_FV000C_types_not_compat),
			ltybuf, rtybuf);
		return(FALSE);
	}

	/*
	**  Check result datatype.  Absolute value is used
	**  in case the field datatype is nullable.
	*/
	if (abs(resdbv.db_datatype) != DB_BOO_TYPE)
	{
		/*
		**  Something MUST be wrong if we don't have
		**  the result of a comparison as a boolean
		**  value.
		*/
		vl_par_error(f_name, ERget(E_FV0013_Internal_parser_error));
		return(FALSE);
	}

	if (ldbvptr->db_datatype != cldbvptr->db_datatype)
	{
		/*
		**  Convert the left side node, which is
		**  usually a field or column value.
		*/
		if (vlist == NULL)
		{
			if ((left->v_right.v_atcvt = (VATTRCV *)MEreqmem(
				(u_i4)0, (u_i4)sizeof(VATTRCV), TRUE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
		}

		if (afe_ficoerce(ladfcb, ldbvptr, cldbvptr, &cfid) != OK)
		{
			vl_par_error(f_name, ERget(E_FV0014_Coercion_error));
			return(FALSE);
		}
		if ((cldbvptr->db_data = MEreqmem((u_i4)0,
			(u_i4)cldbvptr->db_length, TRUE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(f_name);
		}
		if ((xdbvptr = (DB_DATA_VALUE *)MEreqmem((u_i4)0,
		    (u_i4)sizeof(DB_DATA_VALUE),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(f_name);
		}
		MEcopy((PTR) cldbvptr, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) xdbvptr);
		if (vlist != NULL)
		{
			vlist->left_cfid = cfid;
			vlist->left_dbv = xdbvptr;
		}
		else
		{
			left->v_right.v_atcvt->vc_fiid = cfid;
			left->v_right.v_atcvt->vc_dbv = xdbvptr;
		}
	}
	if (rdbvptr->db_datatype != crdbvptr->db_datatype)
	{
		/*
		**  Check to see if right node is also a
		**  field or column value as well.  If so,
		**  set up for conversion at execution time.
		*/
		if (right != NULL && (right->v_nodetype == vATTR ||
			right->v_nodetype == vCOLSEL))
		{
			if ((right->v_right.v_atcvt =
			    (VATTRCV *)MEreqmem((u_i4)0,
			    (u_i4)sizeof(VATTRCV),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			if (afe_ficoerce(ladfcb, rdbvptr, crdbvptr, &cfid) !=OK)
			{
				vl_par_error(f_name, ERget(E_FV0015_Coercion_error));
				return(FALSE);
			}
			if ((crdbvptr->db_data = MEreqmem((u_i4)0,
				(u_i4)crdbvptr->db_length, TRUE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			if ((xdbvptr = (DB_DATA_VALUE *)MEreqmem((u_i4)0,
			    (u_i4)sizeof(DB_DATA_VALUE),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			MEcopy((PTR) crdbvptr, (u_i2) sizeof(DB_DATA_VALUE),
				(PTR) xdbvptr);
			right->v_right.v_atcvt->vc_fiid = cfid;
			right->v_right.v_atcvt->vc_dbv = xdbvptr;
		}
		else
		{
			/*
			**  Do conversion of right side CONSTANT.
			*/
			if (afe_ficoerce(ladfcb, rdbvptr, crdbvptr, &cfid) !=OK)
			{
				vl_par_error(f_name, ERget(E_FV0016_Coercion_error));
				return(FALSE);
			}
			if ((crdbvptr->db_data = MEreqmem((u_i4)0,
				(u_i4)crdbvptr->db_length, TRUE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(f_name);
			}
			ops.afe_opcnt = 1;
			ops.afe_ops = oparr;
			ops.afe_ops[0] =  rdbvptr;
			if (afe_clinstd(ladfcb, cfid, &ops, crdbvptr) != OK)
			{
				vl_par_error(f_name, ERget(E_FV0017_Failed_to_convert));
				return(FALSE);
			}
			if (rdbvptr->db_datatype == DB_TXT_TYPE ||
				rdbvptr->db_datatype == DB_VCH_TYPE)
			{
				MEfree(rdbvptr->db_data);
			}
			MEcopy((PTR) crdbvptr, (u_i2) sizeof(DB_DATA_VALUE),
				(PTR) rdbvptr);
		}

	}

	node->v_fiid = fdesc.adi_finstid;

	return(TRUE);
}



i4
vl_lscoerce(left, right)
VTREE	*left;
VALROOT *right;
{
	DB_DATA_VALUE	*ldbvptr;
	DB_DATA_VALUE	*rdbvptr;
	VALLIST		*node;
	DB_DATA_VALUE	ldbv;
	FIELD		*fld;
	FLDTYPE		*type;
	FLDCOL		*col;
	TBLFLD		*tbl;
	VTREE		tnode;
	VALLIST		*lsnode;
	i4		is_lookup = FALSE;

	if (left->v_nodetype == vATTR)
	{
		fld = left->v_data.v_fld;
		type = FDgettype(fld);
		ldbv.db_datatype = type->ftdatatype;
		ldbv.db_length = type->ftlength;
		ldbv.db_prec = type->ftprec;
		ldbvptr = &ldbv;
	}
	else if (left->v_nodetype == vCOLSEL)
	{
		fld = left->v_left->v_data.v_fld;
		tbl = fld->fld_var.fltblfld;
		col = tbl->tfflds[left->v_data.v_oper];
		type = &(col->fltype);
		ldbv.db_datatype = type->ftdatatype;
		ldbv.db_length = type->ftlength;
		ldbv.db_prec = type->ftprec;
		ldbvptr = &ldbv;
	}
	else
	{
		vl_par_error(ERx("vl_scoerce"), ERget(E_FV0019_Parser_error));
		return(FALSE);
	}

	tnode.v_data.v_oper = v_opEQ;

	if (right->vr_flags & IS_TBL_LOOKUP)
	{
		is_lookup = TRUE;
	}

	for (node = right->listroot; node != NULL; node = node->listnext)
	{
		rdbvptr = &(node->vl_dbv);
		if (is_lookup)
		{
			lsnode = NULL;
		}
		else
		{
			lsnode = node;
		}
		if (!vl_docoerce(ldbvptr, rdbvptr, left, NULL, &tnode, lsnode))
		{
			return(FALSE);
		}
		node->v_lfiid = tnode.v_fiid;
	}

	return(TRUE);
}
