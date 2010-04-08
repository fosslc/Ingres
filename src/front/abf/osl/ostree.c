/*
**  Copyright (c) 1986, 2008, 2009 Ingres Corporation
*/

#include	<compat.h>
#include	<me.h>
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<fdesc.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>
#include	<oskeyw.h>
#include	<oslconf.h>

#if defined(i64_win)
#pragma optimize("", off)
#endif

/**
** Name:    ostree.c -	OSL Parser Tree Node Allocation Module.
**
** Description:
**	Routines for making and freeing nodes for an expression tree.
**      This file defines:
**
**	osmknode()	make a node.
**	osmkconst()	make a constant node.
**	osmkassign()	make an evaluated assignment node (OSL translator.)
**	oscpnode()	copy a tree operator node.
**	ostrfree()	free a tree.
**	ostralloc()	allocate a tree node.
**	ostrnfree()	free a tree node.
**	osiserr()	has an error occurred on this node?
**
** History:
**	Revision 5.1  86/12/11  10:49:36  wong
**	Added check for DML to distinguish DML expression context.
**	Modified to use keyword/operator strings (jhw 11/86.)
**	Modified from compiler version (jhw 10/86.)
**
**	Revision 6.0  87/06  wong
**	Support for ADTs through ADF.
**	Changes for symbol tables.  Added "ATTR" node, 'mkconst()'
**	routine.  (87/01)
**		
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (osmknode).
**	07/22/91 (emerson)
**		Fix for bug 38753 in opnode.
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Moderate changes for bug 34846 in osmknode.
**
**	Revision 7.0
**	16-jun-92 (davel)
**		added decimal support.
**	21-jul-92 (davel)
**		More decimal support: introduce n_prec as new member of an
**		OSNODE, and set from returned DBV from oschkop() in osnode().
**		This will be used (at least) as a new argument to osdatalloc().
**		Also, make osmknode set the length and precision of the
**		constant. Also fix the afe_dec_info call in osmknode() 
**		to use osDecimal, as the n_const for constants are the 
**		same as what was scanned.
**	25-aug-92 (davel)
**		Added case for tkEXEPROC in osmknode() and ostrfree().
**	11-jan-94 (donc) 58297, osmkconst()
**		tkXCONST relied on some earlier scan module to set the
**		b_t_count for n_cont.  Unfortunately b_t_count
**		was never set (in some cases a call to this function was
**		made with a bad address for this entry) and core'ing occurred.
**		I now set the length in this module properly via iiIG_vstr.
**	21-mar-94 (donc) 60470
**		Set expr->n_const for Hex Constants back to the original
**		value after doing the iiIG_vstr on the hex string.
**	29-may-2001 (rigka01) bug#80896 cross-int change #430964
**		extra blank appears when concatenating hex char to character
**		string; in osmkconst(), n_length should be set to half when  
** 		type is tkXCONST 
**	26-nov-2001 (rigka01) bug#105513 opnode()
**	    Compilation of the 4GL statement localvar=char('xxxx',1)
**	    results in an ABF syntax error due to SIGBUS in 4GL compiler
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-aug-2003 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent oslsql.exe from SEGVing
**	    during the run of 4GL SEP test fgl10.sep .
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-Aug-2009 (kschendel) b121804
**	    Void prototypes to keep gcc 4.3 happy.
*/

GLOBALREF char	osDecimal;	/* referenced in osmkconst()  */
bool osckadfproc();
/* _Is_ prefix for postfix RELOP operators */
static const char _Is_[] = ERx("is ");

/*{
** Name:	osmknode() -	Make a Node.
**
** History:
**	01/90 (jhw)
**	    Modify procedure check so all UNDEF procedures
**	    are also not ADF functions.
**	04/07/91 (emerson)
**	    Modifications for local procedures:
**	    Doctor VALUE node if symbol is a local procedure.
**	09/20/92 (emerson)
**	    Use N_ROW bit of n_flags instead of n_sub.  Added logic to set
**	    new N_COMPLEXOBJ, N_TABLEFLD, and N_ROW bits in n_flags.
**	    Added logic to place "value" into the new n_targlfrag member
**	    of a tkQUERY node.  (For bug 34846).
**	25-aug-92 (davel)
**	    Added tkEXEPROC for EXECUTE PROCEDURE.
**	27-nov-2001 (rodjo04) Bug 106438
**	    Added new case ONCLAUSE.
**	21-Jan-2008 (kiria01) b119806
**	    Extended grammar for postfix operators beyond IS NULL
**	    removes the need for th NULLOP node which is replaced
**	    by the ability of the RELOP nodes to support monadic
**	    operators as well as dyadic.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	07-Jun-2009 (kiria01) b122185
**	    Add CASE/IF expression support for SQL
*/
static VOID	_setUndef();
static VOID	opnode();
static VOID	opvalue();
static VOID	opcluster();
static bool	opfunc();

OSNODE *
osmknode (token, value, left, right)
i4		token;
register U_ARG	*value;
register U_ARG	*left;
register U_ARG	*right;
{
    register OSNODE	*result;		/* result to pass back */
    bool		no_type_check;

    OSNODE	*ostralloc();

    no_type_check = (token & DML) != 0;
    token &= ~DML;

    if ((result = ostralloc()) == NULL)
	osuerr(OSNOMEM, 1, ERx("osmknode"));	/* exit! */

    result->n_token = token;

    switch (token)
    {
      case ATTR:
	if (value->u_cp == NULL)
	    osuerr(OSNULLVAL, 1, ERx("object name/attribute"));  /* exit! */
	else
	{
	    result->n_name = value->u_cp;
	    result->n_attr = ((left == NULL) ? NULL : left->u_cp);
	    result->n_type = DB_NODT;
	}
	break;

      case VALUE:
	if (value->u_symp == NULL || (left != NULL && left->u_symp == NULL))
		osuerr(OSNULLVAL, 1, ERx("field reference"));  /* exit! */
	else
	{
		OSSYM	*sym = value->u_symp;

		result->n_sym = sym;
		result->n_tfref = NULL;
		result->n_sexpr = NULL;
		result->n_sref = 0;
		result->n_flags = 0;

		/*
		** If VALUE node is an erroneous reference to a local proc,
		** doctor the node to indicate no value is present.
		*/
		if (sym->s_kind == OSFORM)
		{
			result->n_ilref = 0;
			result->n_type = DB_NODT;
			result->n_length = 0;
			result->n_prec = 0;
		}
		else	/* not a local proc */
		{
			result->n_ilref = sym->s_ilref;
			result->n_type = sym->s_type;
			result->n_length = sym->s_length;
			result->n_prec = sym->s_scale;

			if (sym->s_flags & (FDF_RECORD|FDF_ARRAY))
			{
				result->n_flags |= (N_COMPLEXOBJ | N_ROW);

				if (sym->s_flags & FDF_ARRAY)
				{
					result->n_flags &= ~N_ROW;
				}
			}
			else			/* not a record or array */
			{
				if (sym->s_kind == OSTABLE)
				{
					result->n_flags |= N_TABLEFLD;
				}
				if (left != NULL)
				{
					result->n_tfref = left->u_symp;
					result->n_flags |= N_ROW;
				}
				if (right != NULL)
				{
					result->n_sexpr = right->u_nodep;
					result->n_flags |= N_ROW;
				}
			}
		}
	}
	break;

      case DOT:
	if (left->u_nodep == NULL || right->u_nodep == NULL)
	{
		  osuerr(OSNULLVAL, 1, ERx("record attribute dereference"));
	}
	else
	{
		result->n_type = right->u_nodep->n_type;
		result->n_left = left->u_nodep;
		result->n_right = right->u_nodep;
		if ( !no_type_check )
		{
			result->n_flags = result->n_right->n_flags;
			result->n_ilref = result->n_right->n_ilref;
			result->n_length = result->n_right->n_length;
			result->n_prec = result->n_right->n_prec;
		}
	}
	break;

      case ARRAYREF:
	if (left->u_nodep == NULL || right->u_nodep == NULL)
	{
		  osuerr(OSNULLVAL, 1, ERx("array subscript"));
	}
	else
	{
		/* type is the type of the right-most part of the reference.
		** so if we got 'a.b[2]', this is the type of the element 'b'.
		*/
		result->n_type = left->u_nodep->n_type;
		result->n_left = left->u_nodep;
		result->n_flags = result->n_left->n_flags | N_ROW;
		result->n_ilref = result->n_left->n_ilref;

		/* this is the integer expression that subscripts. */
		result->n_right = right->u_nodep;
	}
	break;

      case OP:
	if (left->u_nodep == NULL || right->u_nodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("operator"));   /* exit! */
	else if (left->u_nodep->n_token == tkNULL ||
			right->u_nodep->n_token == tkNULL)
	{
	    oscerr(OSNULLEXPR, 0);
	    result->n_type = DB_NODT;
	    result->n_fiid = ADI_NOFI;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	else if (no_type_check)
	{
	    result->n_type = DB_NODT;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	else
	{
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
#ifdef _CLUSTER	    /* Code to use when clustering desired for strings */
	    if (!oschkstr(result->n_left->n_type) ||
		    !oschkstr(result->n_right->n_type) ||
			result->n_right->n_token != OP ||
			    !osw_compare(result->n_right->n_value, _Plus))
#endif
		opnode(result);
#ifdef _CLUSTER	    /* Code to use when clustering desired for strings */
	    else
	    { /* cluster string concatenation */
		opcluster(right->u_nodep, result);
		result = right->u_nodep;
	    }
#endif
	}
	break;

      case UNARYOP:
	if (left->u_nodep == NULL)
	    osuerr(OSNULLCONST, 1, ERx("unary operator"));	/* exit! */
	else if (no_type_check)
	{
	    result->n_type = DB_NODT;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	}
	else
	{
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    opnode(result);
	}
	result->n_right = NULL;
	break;

     case RELOP:
	if (left->u_nodep == NULL ||
	    right->u_nodep == NULL && !osw_compare_pfx(value->u_cp, _Is_, 3))
	    osuerr(OSNULLVAL, 1, ERx("relational operator"));    /* exit! */
	else if (left->u_nodep->n_token == tkNULL ||
		right->u_nodep && right->u_nodep->n_token == tkNULL)
	{
	    oscerr(OSNULLCMP, 0);
	    result->n_type = DB_NODT;
	    result->n_fiid = ADI_NOFI;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	else if (no_type_check)
	{
	    result->n_type = DB_NODT;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	else
	{
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	    _setUndef(result);
	    opnode(result);
	}
	break;

      case LOGOP:
	if (left->u_nodep == NULL ||
	 	(right->u_nodep == NULL && !osw_compare(value->u_cp, _Not)))
	    osuerr(OSNULLVAL, 1, ERx("logical operator"));	/* exit! */
	else if (no_type_check)
	{
	    result->n_type = DB_NODT;
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	else
	{
	    result->n_value = value->u_cp;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	    opnode(result);
	}
	break;

      case PARENS:
	if (left == NULL)
	    osuerr(OSBUG, 1, ERx("osmknode(parens)"));
	else
	{
	    result->n_type =
			left->u_nodep != NULL ? left->u_nodep->n_type : DB_NODT;
	    result->n_value = NULL;
	    result->n_left = left->u_nodep;
	    result->n_right = NULL;
	}
	break;

      case COMMA:
   	if (left->u_nodep == NULL || right->u_nodep == NULL)
	    osuerr(OSBUG, 1, ERx("osmknode(comma)"));
	else
	{
	    result->n_type = left->u_nodep->n_type;
	    result->n_value = NULL;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	break;

      case BLANK:
   	if (left->u_nodep == NULL || right->u_nodep == NULL)
	    osuerr(OSBUG, 1, ERx("osmknode(blank)"));
	else
	{
	    result->n_type = left->u_nodep->n_type;
	    result->n_value = NULL;
	    result->n_left = left->u_nodep;
	    result->n_right = right->u_nodep;
	}
	break;

      case tkBYREF:
	if (left->u_symp == NULL)
	       osuerr(OSNULLVAL, 1, ERx("byref field"));    /* exit! */
	result->n_actual = left->u_symp;
	result->n_type = left->u_symp->s_type;
	break;

      case ASNOP:
	if (left->u_nodep == NULL || right->u_nodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("assignment operator"));
	result->n_value = value->u_cp;
	result->n_left = left->u_nodep;
	result->n_right = right->u_nodep;
	break;

      case PREDE:
	result->n_dbname = value->u_cp;
	result->n_dbattr = left->u_cp;
	result->n_pfld = right->u_symp;
	break;

      case NPRED:
   	result->n_type = DB_BOO_TYPE;
	result->n_plist = value->u_nodep;
	result->n_pnum = 0;
	break;

      case NLIST:
	result->n_ele = value->u_nodep;
	result->n_next = left->u_nodep;
	break;

      case AGGLIST:
	result->n_agexpr = value->u_nodep;
	result->n_agbylist = left->u_nodep;
	result->n_agqual = right->u_nodep;
	break;

      case DOTALL:
	result->n_slist = value->u_nodep;
	break;

      case tkASSIGN:
	if (left->u_symp == NULL || right->u_nodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("procedure parameter"));
	result->n_type = left->u_symp->s_type;
	result->n_lhs = left->u_symp;
	result->n_rhs = right->u_nodep;
	break;

      case tkEXEPROC:
      {
	register OSNODE	*proc = left->u_nodep;
	register OSNODE	*params = right->u_nodep;

	result->n_psym = NULL;
	if (value == NULL || left == NULL || right == NULL ||
			left->u_nodep == NULL)
	{
		osuerr(OSNULLVAL, 1, ERx("execute procedure"));	/* exit !*/
	}
	else
	{
		/* Database Procedures always return an i4 */
		result->n_token = tkEXEPROC;
		result->n_type = DB_INT_TYPE;
		result->n_length = sizeof(i4);
		result->n_prec = 0;
		result->n_rlen = sizeof(i4);

		result->n_proc = proc;
		result->n_parlist = params;
	}
	break;
      }

      case tkPCALL:
      {
	register OSNODE	*proc = left->u_nodep;
	register OSNODE	*params = right->u_nodep;

	result->n_psym = NULL;
	if (value == NULL || left == NULL || right == NULL ||
			left->u_nodep == NULL)
	{
		osuerr(OSNULLVAL, 1, ERx("procedure call"));	/* exit !*/
	}
	else if ( proc->n_token != tkID
			|| ( (result->n_psym = value->u_symp) != NULL
				&& result->n_psym->s_kind != OSUNDEF )
			|| !opfunc(proc->n_value, params) )
	{ /* procedure call */
	    register OSSYM   *psym = result->n_psym;
	    if (psym == NULL)
	    {
		result->n_type = DB_NODT;
		result->n_rlen = 0;
		result->n_prec = 0;
	    }
	    else
	    {
		result->n_type = psym->s_type;
		result->n_rlen = psym->s_length;
		result->n_prec = psym->s_scale;
	    }
	    result->n_proc = proc;
	    result->n_parlist = params;
	}
	else
	{ /* function call */
	    result->n_value = proc->n_value;
	    result->n_parlist = params;
	    if (params == NULL)
	    {
		result->n_left = result->n_right = NULL;
	    	result->n_token = UNARYOP;
	    }
	    else
	    {
	    	result->n_left = (params->n_token == COMMA) ?
				params->n_left : params;
	    	result->n_right = (params->n_token == COMMA) ?
				params->n_right : NULL;
	    	result->n_token = (result->n_right == NULL) ? UNARYOP : OP;

	    	if (params->n_token == COMMA)
	    	{
			params->n_left = params->n_right = NULL;
			ostrfree(params);
	    	}
	    }
	    ostrfree(proc);

	    opnode(result);
	}
	break;
      }

      case tkFCALL:
	if (value == NULL || left == NULL || right == NULL ||
			left->u_nodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("frame call"));	/* exit! */
	result->n_fsym = value->u_symp;
	result->n_type = (result->n_fsym == NULL) ? DB_NODT :
				result->n_fsym->s_type;
	result->n_frame = left->u_nodep;
	result->n_parlist = right->u_nodep;
	break;

      case tkQUERY:
	if (value == NULL || left == NULL || right == NULL ||
			right->u_qrynodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("query construction"));
	result->n_type = DB_NODT;
	result->n_formobj = left->u_nodep;
	result->n_query = right->u_qrynodep;
	result->n_child = NULL;
	result->n_targlfrag = value->u_p;
	break;
 
      case tkCASE:
      case tkWHEN:
      case tkTHEN:
	if (left)
	    result->n_left = left->u_nodep;
	else
	    result->n_left = NULL;
	if (right)
	    result->n_right = right->u_nodep;
	else
	    result->n_right = NULL;
	if (no_type_check || result->n_token != tkCASE)
	    result->n_type = DB_NODT;
	else
	{
	    /* IF CASE support is ever to be added to OSL itself
	    ** then here ought to be case datatype resolution code */
	}
	break;

      case ASSOCOP:
	if (left->u_nodep == NULL || right->u_nodep == NULL)
	    osuerr(OSNULLVAL, 1, ERx("associative"));
	result->n_type = DB_NODT;	/* associative operators have no type */
	result->n_value = value->u_cp;
	result->n_left = left->u_nodep;
	result->n_right = right->u_nodep;
	break;

      case tkSCONST:
      case tkXCONST:
      case tkICONST:
      case tkFCONST:
      case tkDCONST:
      case tkNULL:
	osuerr(OSBUG, 1, ERx("osmknode(constant) (use 'osmkconst()')"));/* exit! */

      case tkID:
	osuerr(OSBUG, 1, ERx("osmknode(identifier) (use 'osmkident()')"));/* exit! */

	  case ONCLAUSE:
   	
	    result->n_type = DB_NODT;
	    result->n_value = NULL;
	    result->n_left = ((left == NULL) ? NULL : left->u_nodep);
	    result->n_right =((right == NULL) ? NULL : right->u_nodep);
	    break;

      default:
	osuerr(OSBUG, 1, ERx("osmknode(unknown)"));	/* exit! */

    } /* end switch */

    return result;
}

/*
** Name:	_setUndef() -	Set Types for Undefined Procedure Nodes.
**
** Description:
**	Check the input expression node for children that are undefined
**	procedure nodes.  If so, then map types from the defined child
**	(if any) so that reasonable expression results can be generated.
**
** Input:
**	np	{OSNODE *}  The operator result node (address.)
**
** Side Effects:
**	Child nodes will
**
** History:
**	05/89 (jhw) -- Written.
*/
static VOID
_setUndef ( np )
register OSNODE	*np;
{
	register OSNODE	*left = np->n_left;

	if ( left != NULL )
	{
	  	if ( np->n_token == UNARYOP  ||
	  		np->n_token == RELOP && !np->n_right)
		{ /* unary */
			if ( left->n_type == DB_NODT &&
					left->n_token == tkPCALL )
			{
				left->n_type = DB_INT_TYPE;
				left->n_rlen = sizeof(i4);
				left->n_prec = 0;
			}
		}
		else
		{ /* binary op */
			register OSNODE	*right = np->n_right;
			DB_DATA_VALUE	dbv;

			if ( left->n_type == DB_NODT &&
					left->n_token == tkPCALL )
			{
				opvalue(right, &dbv);
				left->n_type = dbv.db_datatype;
				left->n_rlen = dbv.db_length;
				left->n_prec = dbv.db_prec;
			}
			else if ( right->n_type == DB_NODT &&
					right->n_token == tkPCALL )
			{
				opvalue(left, &dbv);
				right->n_type = dbv.db_datatype;
				right->n_rlen = dbv.db_length;
				right->n_prec = dbv.db_prec;
			}
		}
	}
}

/*
** Name:	opnode() -	Make ADF Type Correct Operator Node.
**
** Description:
**	This routine fills in an operator node with the ADF operator ID and
**	type information.  If the operator has incompatible types, an error
**	is reported.  Otherwise, nodes are created if any of the operands
**	requires coercion.
**
** Input:
**	np	{OSNODE *}  The operator node (address.)
**
** Side Effects:
**	Node may have coercion operator nodes inserted for the children.
**
** History:
**	02/87 (jhw)  Written.
**	09/89 (jhw)  Added check for COMMA operands, which would be present
**		if too many operands were passed to a function.  JupBug #8161.
**	12/90 (Mike S) Add check for undefined procedure in expression.
**		       Bug 34535.
**	07/22/91 (emerson) Fix for bug 38753: The previously-added check for
**		an undefined procedure in an expression blew up when attempting
**		to print the error message in the case where the procedure
**		name was contained in a string variable, because in this case
**		the tkPCALL node was built with a NULL symbol-table pointer
**		(vn_objsym) for the procedure object.  [This was certainly true
**		prior to 6.4.  My changes for local procedures introduced a bug
**		that muddied the waters: vn_objsym could potentially be set
**		to garbage instead of NULL.  This bug is being fixed as part
**		of this change.  See oslgram.my for details.]
**		I have added a new error message (E_OS0266) for this case.
**	31-jul-92 (edf)
**		Removed static from the definition of op_vch_change to
**		increase its link-time scope (ie, needed to make call to it
**		from oslgram.my).
**	21-jul-92 (davel)
**		Save the result precision in the n_prec member of the OSNODE.
**	28-feb-94 (donc) Bug 59413
**		Allow 4GL to call decimal() function with 1, 2 or 3 parameters.
**    26-nov-2001 (rigka01) bug#105513 opnode()
**            Compilation of the 4GL statement localvar=char('xxxx',1)
**            results in an ABF syntax error due to SIGBUS in 4GL compiler
*/

VOID	op_vch_change();

static VOID
opnode (np) 
register OSNODE	*np;
{
	register OSNODE	*left = np->n_left;
	register OSNODE	*right = NULL;
	register OSNODE	*dtemp = NULL;
	register i4	i;
	AFE_OPERS	oplist;
	DB_DATA_VALUE   ops[3];
	DB_DATA_VALUE   *oparr[3];
	AFE_OPERS	coplist;
	DB_DATA_VALUE   cops[3];
	DB_DATA_VALUE   *coparr[3];
	DB_DATA_VALUE   value;
	OSSYM		*procsym;

	OSNODE	*ostralloc();

	i4	type_err = np->n_token == LOGOP ? OSLOGTYPCHK : OSTYPECHK;
   	i2	temp_ps; 
	i4	temp_p; 
	i4	temp_s; 
        PTR 	ivalue;
	STATUS 	stat;

	np->n_fiid = ADI_NOFI;	/* mark node as having no function */
	np->n_type = DB_NODT;

        MEfill(sizeof(DB_DATA_VALUE),0,&ops[0]);
        MEfill(sizeof(DB_DATA_VALUE),0,&ops[1]);
        MEfill(sizeof(DB_DATA_VALUE),0,&ops[2]);

	if ( left != NULL )
	{
		if ( (left->n_token == VALUE && left->n_sym->s_kind == OSTABLE) )
		{
			oscerr(type_err, 1, np->n_value);
			return;
		}
	  	else if ( left->n_type == DB_NODT )
		{ /* undefined */
			if (left->n_token == tkPCALL)
			{
				procsym = left->n_tag.n_call.vn_objsym;
				if ( procsym == NULL )
				{
					oscerr(E_OS0266_ProcInVar, 0);
				}
				else
				{
					oscerr(E_OS0263, 1, procsym->s_name);
				}
			}
			return;
		}

	  	if ( np->n_token != UNARYOP  &&
	  		(np->n_token != RELOP ||
				!osw_compare_pfx(np->n_value, _Is_, 3))  &&
	  		(np->n_token != LOGOP ||
				!osw_compare(np->n_value, _Not)) )
		{ /* check right branch */
	  		right = np->n_right;
			if ( ( right->n_token == VALUE   &&
		   			right->n_sym->s_kind == OSTABLE ) )
			{
				oscerr(type_err, 1, np->n_value);
				return;
			}
			else if ( right->n_type == DB_NODT )
			{ /* undefined */
				if (right->n_token == tkPCALL)
				{
					procsym = right->n_tag.n_call.vn_objsym;
					if ( procsym == NULL )
					{
						oscerr(E_OS0266_ProcInVar, 0);
					}
					else
					{
						oscerr(E_OS0263, 1,
							procsym->s_name);
					}
				}
				return;
			}
		}
	}

	/* 
	** dangerous magic: if we have a char temp in an expression,
	** convert it to a varchar.
	*/
	if (np->n_token == OP || np->n_token == UNARYOP || np->n_token == RELOP)
	{ /* check for char temp, change if found. */
		op_vch_change(left);

		/* an ESCAPE string must be DB_CHA_TYPE, damnit. */
		if (right != NULL && !osw_compare(np->n_value, _Escape))
			op_vch_change(right);
	}

	/* Get function for node */
		
	oplist.afe_ops = oparr;

	if (left == NULL)
	{
		oplist.afe_opcnt = 0;
	}
	else
	{
		oparr[0] = &ops[0];
		oplist.afe_opcnt = 2;

		if ( osw_compare( np->n_value, ERx("decimal") ) ) {
		    oparr[1] = &ops[1];
		    ivalue = MEreqmem((u_i4)0, (u_i4)6, TRUE, &stat);

		    if ( left->n_token != COMMA ) 
		    {
		        opvalue(left, oparr[0]);
                        if ( right == NULL ) 
			{
	                    switch (abs(left->n_type))
	    		    {
	      			case DB_FLT_TYPE:
	      			case DB_DEC_TYPE:
	      			case DB_MNY_TYPE:
	        		   temp_ps = 
					DB_PS_ENCODE_MACRO(AD_FP_TO_DEC_PREC,
					AD_FP_TO_DEC_SCALE);
				    break;
	      			case DB_INT_TYPE:
		    		    temp_ps = 
					DB_PS_ENCODE_MACRO(AD_I4_TO_DEC_PREC,
					AD_I4_TO_DEC_SCALE);
		    		    break;
				default:
			            oscerr(type_err, 1, np->n_value);
		                    np->n_flags |= N_ERROR;
		                    return;
		            }
			    CVna((i4)temp_ps, ivalue ); 
			    np->n_right = osmkconst( tkICONST, ivalue );
			    np->n_token = OP;
			    right = np->n_right;
			}
			else
			{
			    if ( abs(right->n_type) != DB_INT_TYPE ) 
			    { 
			        oscerr(type_err, 1, np->n_value);
		                np->n_flags |= N_ERROR;
		                return;
			    }
			    CVan( right->n_value, &temp_p ); 
			    temp_ps = DB_PS_ENCODE_MACRO( temp_p, 0);
			}
		    }
		    else
		    {   /* Hack Attack... */
			if ( abs(right->n_type) != DB_INT_TYPE 
			  || abs(left->n_right->n_type) != DB_INT_TYPE ) 
			{ 
			    oscerr(type_err, 1, np->n_value);
		            np->n_flags |= N_ERROR;
		            return;
			}
			CVan( left->n_right->n_value, &temp_p ); 
			CVan( right->n_value, &temp_s ); 
			temp_ps = DB_PS_ENCODE_MACRO( temp_p, temp_s );
			left->n_right = NULL;
		        MEcopy( (PTR)left->n_left,(u_i2)sizeof(OSNODE),
			 (PTR)left ); 	
		        opvalue(left, oparr[0]);
		    }
		    CVna((i4)temp_ps, ivalue ); 
		    right->n_value = ivalue;
		    right->n_length = 2;
		    right->n_prec = temp_ps; 
		    oparr[1]->db_data = (PTR)&temp_ps;
		    opvalue(np->n_right, oparr[1]);
		}
		else
		{
		    opvalue(left, oparr[0]);
		    if ( np->n_token == UNARYOP  
		      || np->n_token == RELOP && osw_compare_pfx(np->n_value, _Is_, 3)
		      || (np->n_token == LOGOP 
		      && osw_compare(np->n_value, _Not)) )
		    {
			oplist.afe_opcnt = 1;
		    }
		    else
		    {
			oparr[1] = &ops[1];
			opvalue(right, oparr[1]);
		    }
	        }
	}
   
	coplist.afe_opcnt = oplist.afe_opcnt;
	coplist.afe_ops = coparr;
	coparr[0] = &cops[0];
	coparr[1] = &cops[1];
	coparr[2] = &cops[2];

	np->n_fiid = oschkop(np->n_value, &oplist, &coplist, &value);
	if (np->n_fiid == ADI_NOFI)
	{ /* Incompatible types for operator */
		if ( osckadfproc(np->n_value) )
			oscerr(OSFUNCOPS, 1, np->n_value);
		else if (np->n_token == UNARYOP)
			oscerr(OSUMINUS, 0);
		else
			oscerr(type_err, 1, np->n_value);
		np->n_flags |= N_ERROR;
		return;
	}
	np->n_type = value.db_datatype;
	np->n_length = value.db_length;
	np->n_prec = value.db_prec;

	/* Operation OK, so now insert coercion nodes */
  
	for (i = 0 ; i < oplist.afe_opcnt ; ++i)
	{
		register DB_DATA_VALUE  *cop = &cops[i];
		register OSNODE	*newp;
  
		if (ops[i].db_datatype == cop->db_datatype)
		{
			/* no coercion necessary */
			continue;
		}

		if ((newp = ostralloc()) == NULL)
			osuerr(OSNOMEM, 1, ERx("opnode()"));
		newp->n_token = UNARYOP;
		newp->n_left = (i == 0 ? np->n_left : np->n_right);
		newp->n_right = NULL;

		newp->n_fiid = oschkcoerce(ops[i].db_datatype, 
						cop->db_datatype);
		newp->n_type = cop->db_datatype;
		newp->n_length = cop->db_length;
		newp->n_prec = cop->db_prec;
 
		if (i == 0)
			np->n_left = newp;
		else
			np->n_right = newp;
	} /* end for */
}

/*
** Name:	op_vch_change() -	change a string constant to a varchar.
**
** Description:
**	This routine takes a string constant, currently set up to be 
**	stored (in the dbms and in a temp var) as a CHAR type, and converts
**	it to a VARCHAR.  This solves a number of bugs, since string
**	constants used to be strangely padded, which rightly vexed customers.
**
**	String constants are supposed to be of type VARCHAR for 4GL/SQL and
**	type TEXT for 4GL/QUEL, which follows the DBMS semantics for string
**	constants.
**
** Input:
**	node	{OSNODE *}  The node, which may be a string constant.
**
** Side Effects:
**	The whole routine is a side effect.
**
** History:
**	04/89 (billc)  Written.
**	30-jul-92 (davel)
**		If we switch the n_type of the node, also update the n_length.
*/

VOID
op_vch_change(node)
OSNODE *node;
{
	if (node != NULL
	  && node->n_token == tkSCONST 
	  && (node->n_type == DB_CHA_TYPE || node->n_type == DB_CHR_TYPE) )
	{
		char *iiIG_vstr();
		char *tmp = node->n_const;

		node->n_const = iiIG_vstr(tmp, STlength(tmp));
		node->n_type = (QUEL ? DB_TXT_TYPE : DB_VCH_TYPE);
		node->n_length = 
		    ((DB_TEXT_STRING *)node->n_const)->db_t_count + DB_CNTSIZE;
	}
}

/*
** Name:	opvalue() -	Return DB Type for Node.
**
** Description:
**	This routine fills in a DB_DATA_VALUE that describes
**	the type of a tree node.
**
** Input:
**	np	{OSNODE *}  The tree node (address.)
**	dbv	{DB_DATA_VALUE *}  The DB_DATA_VALUE to be filled in.
**
** History:
**	02/87 (jhw) -- Written.
**	21-jul-92 (davel)
**		changes associated with adding decimal support and addition
**		of new members of OSNODE structure (n_prec and n_length).
*/

static VOID
opvalue (np, dbv)
register OSNODE		*np;
DB_DATA_VALUE	*dbv;
{
    i4 length = 0;

    if (np->n_type == DB_DYC_TYPE)
    	dbv->db_datatype = ( QUEL ? DB_TXT_TYPE : DB_VCH_TYPE );
    else
    	dbv->db_datatype = np->n_type;

    /* just about all node types just get the length/precision from the node
    ** directly.
    */
    switch (np->n_token)
    {
      case DOT:
	/* look at the leaf node for the true datatype. */
	opvalue(np->n_right, dbv);
	return;
      case ARRAYREF:
	/* look at the leaf node for the true datatype. */
	opvalue(np->n_left, dbv);
	return;
      case tkPCALL:
	if ( dbv->db_datatype != DB_NODT )
	{
		length = np->n_psym != NULL ? np->n_psym->s_length : 0;
		dbv->db_prec = np->n_psym != NULL ? np->n_psym->s_scale : 0;
	}
	else
	{
		dbv->db_datatype = DB_INT_TYPE;
		length = sizeof(i4);
		dbv->db_prec = 0;
	}
	break;
      case VALUE:
      case tkSCONST:
      case tkDCONST:
      case tkXCONST:
      case tkICONST:
      case tkFCONST:
      case tkNULL:
      case OP:
      case UNARYOP:
      case RELOP:
      case LOGOP:
	length = np->n_length;
	dbv->db_prec = np->n_prec;
	break;
      default:
	osuerr(OSBUG, 1, ERx("opvalue()"));	/* exit! */
    } /* end switch */

    dbv->db_length = length;
}

/*
** Name:    opcluster() -   Cluster String Concatenation.
**
** Description:
**	This routine clusters a tree of string concatenation operators into a
**	left-most node list by inserting a new clustered node list as its
**	left-most element.
**
**	Note that this routine should be applied to each string concatenation
**	node as it is built guaranteeing that each branch is a clustered node
**	list.
**
** Input:
**	np	{OSNODE *}  Clustered string concatenation node list.
**	new	{OSNODE *}  New node (list) to be added to cluster.
**
** History:
**	03/87 (jhw) -- Written.
*/
static VOID
opcluster (np, new)
register OSNODE	*np;
register OSNODE	*new;
{
    if (np->n_left->n_token == OP)
	opcluster(np->n_left, new);
    else
    {
	new->n_right = np->n_left;
	opnode(new);
	np->n_left = new;
    }
    opnode(np);
}

/*
** Name:    opfunc() -	Check if ADF Function Operator Name.
**
** Input:
**	fct_name    {char *}  ADF function operator name.
**	params	    {OSNODE *}  Tree containing parameters.
**
** History:
**	02/87 (jhw) -- Written.
*/
static bool	of_ck();

static bool
opfunc (fct_name, params)
char	*fct_name;
OSNODE	*params;
{
    if ( !osckadfproc(fct_name) )
	return FALSE;

    if (!of_ck(params))
    { /* parameters don't match */
	oscerr(OSTYPECHK, 1, fct_name);
	return FALSE;
    }

    return TRUE;
}

static bool
of_ck(node)
OSNODE *node;
{
	if (node == NULL)
		return TRUE;
	else if ( node->n_token == DOTALL  
	  || node->n_token == TLASSIGN  
	  || node->n_token == tkBYREF  )
	{
		return FALSE;
	}
	else if (node->n_token == COMMA
	  && (!of_ck(node->n_left) || !of_ck(node->n_right)))
	{
		return FALSE;
	}
	return TRUE;
}

/*{
** Name:	osmkconst() -	Make a Constant Node.
**
** Description:
**	This routine returns a node representing a constant of
**	value and type specified by the input parameters.
**
** Input:
**	token	{nat}  The token for the constant (maps to type.)
**			tkICONST ==> DB_INT_TYPE (integer)
**			tkFCONST ==> DB_FLT_TYPE (float)
**			tkDCONST ==> DB_DEC_TYPE (packed decimal)
**			tkSCONST ==> DB_CHA_TYPE (string)
**			tkXCONST ==> DB_VCH_TYPE (hexadecimal string)
**			tkNULL   ==> -DB_LTXT_TYPE (null value)
**	value	{char *}  The string representing the constant value.
**
** Returns:
**	{OSNODE *}  The constant node.
**
** History:
**	01/87 (jhw) -- Written.
**	16-jun-92 (davel)
**		added support for decimal.
**	30-jul-92 (davel)
**		also set the constant node's length and precision.
**	11-jan-94 (donc) 58297
**		tkXCONST relied on some earlier scan module to set the
**		b_t_count for n_cont.  Unfortunately b_t_count
**		was never set (in some cases a call to this function was
**		made with a bad address for this entry) and core'ing occurred.
**		I now set the length in this module properly via iiIG_vstr.
**	29-may-2001 (rigka01) bug#80896 cross-int change #430964
**		extra blank appears when concatenating hex char to character
**		string; in osmkconst(), n_length should be set to half when  
** 		type is tkXCONST 
*/
OSNODE *
osmkconst (type, value)
i4	type;
char	*value;
{
    register OSNODE	*np;

    OSNODE	*ostralloc();

    if ((np = ostralloc()) == NULL)
	osuerr(OSNOMEM, 1, ERx("osmkconst"));
    else
    {
	np->n_token = type;
	np->n_const = value;
	np->n_flags = N_READONLY;
	np->n_prec = 0;		/* will be overridden for dec const. below */

	switch (type)
	{
	  case tkSCONST:
	    np->n_type = DB_CHA_TYPE;
	    np->n_length = STlength(np->n_const);
	    break;
	  case tkICONST:
	    np->n_type = DB_INT_TYPE;
	    np->n_length = sizeof(i4);
	    break;
	  case tkFCONST:
	    np->n_type = DB_FLT_TYPE;
	    np->n_length = sizeof(f8);
	    break;
	  case tkDCONST:
	    {
		i4 prec, sc, len;

		_VOID_ afe_dec_info(np->n_const, osDecimal, &len, &prec, &sc);
		np->n_type = DB_DEC_TYPE;
		np->n_length = len;
		np->n_prec = DB_PS_ENCODE_MACRO(prec, sc);
		break;
	    }
	  case tkXCONST:
            {	
		char *iiIG_vstr();
		char *tmp = np->n_const;
	        np->n_type = DB_VCH_TYPE;
		np->n_const = iiIG_vstr(tmp, STlength(tmp));
	        if (((DB_TEXT_STRING *)np->n_const)->db_t_count != 0)
		  np->n_length = 
		    (((DB_TEXT_STRING *)np->n_const)->db_t_count/2)+DB_CNTSIZE;
		else
		  np->n_length = 
		    ((DB_TEXT_STRING *)np->n_const)->db_t_count + DB_CNTSIZE;
		np->n_const = value;
	        break;
            }
	  case tkNULL:
	    np->n_type = -DB_LTXT_TYPE;
	    np->n_length = 1 + DB_CNTSIZE;
	    break;
	}
    }

    return np;
}

/*{
** Name:	osmkident() -	Make a ID Node.
**
** Description:
**	This routine returns a node representing an identifier as
**	specified by the input parameters.
**
** Input:
**	value	{char *}  The identifier string.
**	node	{OSNODE *}  Node associated with identifier.
**
** Returns:
**	{OSNODE *}  The ID node.
**
** History:
**	05/87 (jhw) -- Written.
*/
OSNODE *
osmkident (value, node)
char	*value;
OSNODE	*node;
{
    register OSNODE	*np;

    OSNODE	*ostralloc();

    if ((np = ostralloc()) == NULL)
	osuerr(OSNOMEM, 1, ERx("osmkident"));
    else
    {
	np->n_token = tkID;
	np->n_type = DB_NODT;
	np->n_value = value;
	np->n_left = node;
	np->n_right = NULL;
    }

    return np;
}

/*{
** Name:	osmkassign() -	Make an Evaluated Assignment Node
**					(OSL Translator.)
** Description:
**
** History:
**	09/86 (jhw) -- Written.
*/
OSNODE *
osmkassign (aelement, avalue, byref)
u_i2	aelement;
u_i2	avalue;
bool	byref;
{
    register OSNODE	*np;

    OSNODE	*ostralloc();

    if ((np = ostralloc()) == NULL)
        osuerr(OSNOMEM, 1, ERx("osmkassign"));

    np->n_token = TLASSIGN;
    np->n_type = DB_NODT;
    np->n_coln = aelement;
    np->n_tlexpr = avalue;
    np->n_byreff = byref;

    return np;
}

/*{
** Name:    oscpnode() -	Copy a Tree Operator Node.
**
** Description:
**	Creates a new node that is a copy of the input node, which can only
**	be a non-leaf operator node, and sets the children of the new node
**	to the input children.
**
** Input:
**	node	{OSNODE *}  The node to be copied.
**	left	{OSNODE *}  New left child.
**	right	{OSNODE *}  New right child.
**
** Returns:
**	{OSNODE *}  A copy of the input node.
**
** History:
**	09/87 (jhw) -- Written.
*/
/*VARARGS2*/
OSNODE *
oscpnode (node, left, right)
OSNODE	*node;
OSNODE	*left;
OSNODE	*right;
{
    register OSNODE	*result;		/* result to pass back */

    OSNODE	*ostralloc();

    if ((result = ostralloc()) == NULL)
	osuerr(OSNOMEM, 1, ERx("oscpnode"));	/* exit! */

    MEcopy((PTR)node, sizeof(*result), (PTR)result);
    switch (node->n_token)
    {
      case OP:
      case LOGOP:
      case RELOP:
      case ASNOP:
      case ASSOCOP:
      case COMMA:
      case BLANK:
	result->n_left = left;
	result->n_right = right;
	break;

      case UNARYOP:
      case PARENS:
	result->n_left = left;
	break;

      default:
	osuerr(OSBUG, 1, ERx("oscpnode(type)"));
	/*NOTREACHED*/
    }
    return result;
}

/*{
** Name:	ostrfree() -	Free Expression Tree.
**
** Description:
**	Free the space taken up by a tree.  The nodes are returned to
**	a free list for later use by the compiler.  The space is not
**	returned to the free storage pool.
*/

void
ostrfree (OSNODE *tr)
{
    if (tr == NULL || tr->n_token == FREE_NODE || tr->n_token == 0)
	return;

    /* Note:  symbol nodes have are never freed. */
    switch (tr->n_token)
    {
      /*
      ** Fields, attributes and constants need nothing freed.
      */
      case ATTR:
      case tkSCONST:
      case tkXCONST:
      case tkICONST:
      case tkFCONST:
      case tkDCONST:
      case tkNULL:
	break;

      case VALUE:
	if (tr->n_sexpr != NULL)
	{
	    ostrfree(tr->n_sexpr);
	}
	break;

      case PREDE:
	break;

      case NPRED:
	ostrfree(tr->n_plist);
	break;

      /*
      ** With a list the element and the remainder have to be freed.
      */
      case NLIST:
	ostrfree(tr->n_ele);
	ostrfree(tr->n_next);
	break;

      /*
      ** With parentheses or a unary operator, only the left need be freed.
      */
      case UNARYOP:
      case PARENS:
	ostrfree(tr->n_left);
	break;

      /*
      ** With an aggregate target list, 3 subsidiary nodes must be freed.
      */
      case AGGLIST:
	ostrfree(tr->n_agexpr);
	ostrfree(tr->n_agbylist);
	ostrfree(tr->n_agqual);
	break;

      /*
      ** With the node representing a '.all', one child node must be freed.
      */
      case DOTALL:
	ostrfree(tr->n_rownum);
	break;

      /*
      ** Procedure parameter passed by reference.
      */
      case tkBYREF:
	break;

      /*
      ** Assignment statement within parameter list of OSL procedure call.
      */
      case tkASSIGN:
	ostrfree(tr->n_rhs);
	break;

      /*
      ** For a procedure or frame call, the name
      ** and parameter list must be freed.
      */
      case tkPCALL:
      case tkFCALL:
      case tkEXEPROC:
	ostrfree(tr->n_object);
	ostrfree(tr->n_parlist);
	break;

      /*
      ** Query node
      */
      case tkQUERY:
	if (tr->n_query != NULL)
	    osqryfree(tr->n_query);
	if (tr->n_formobj != NULL)
		ostrfree(tr->n_formobj);
	ostrfree(tr->n_child);
	break;

      case ARRAYREF:
	/* right-hand node was freed by osvalref */
	ostrfree(tr->n_left);
	break;

      /*
      ** Nodes with token RELOP are the only ones that may used
      ** the 'lhstemp' and 'rhstemp' symbol elements, but these
      ** are temporaries and don't need to be freed so fall through ...
      */
      case RELOP:
      /*
      ** With the remainder, both the right and left must be freed.
      */
      case DOT:
      case tkID:
      case OP:
      case LOGOP:
      case ASNOP:
      case COMMA:
      case BLANK:
      case ASSOCOP:
      case ONCLAUSE:
	ostrfree(tr->n_left);
	if (tr->n_right)
	    ostrfree(tr->n_right);
	break;

      case FRSCONST:
      case TLASSIGN:
	break;

      case tkCASE:
      case tkWHEN:
      case tkTHEN:
	if (tr->n_left)
	    ostrfree(tr->n_left);
	if (tr->n_right);
	    ostrfree(tr->n_right);
	break;
      default:
    	osuerr(OSBUG, 1, ERx("ostrfree(unknown)"));	/* ERROR */
    	break;		/* free this node anyway ... */
    } /* end switch */
    ostrnfree(tr);
}

/*
** OSL Parser Tree Node Allocation
*/

#define FREESIZE 10
static OSNODE	*Trfree = NULL;

/*{
** ostralloc
*/
OSNODE *
ostralloc()
{
    	OSNODE	*alloc;

    	if (Trfree == NULL)
    	{
		i4 i;

    		Trfree = (OSNODE *)MEreqmem(0, 
			(u_i2) (sizeof(*Trfree) * FREESIZE), 
				TRUE, (STATUS*) NULL
		);
		for (i = 0; i < (FREESIZE - 1); i++)
			Trfree[i].n_left = &Trfree[i + 1];
    	}
	
	alloc = Trfree;
	Trfree = Trfree->n_left;
	alloc->n_left = NULL;
	
	return alloc;
}

/*{
** Name:	ostrnfree() -	Free OSNODE Structure.
**
** Description:
**	Frees only the OSNODE structure by attaching it to the free-list
**	if it is not already free.
**
** History:
**	08/86 (jhw) -- Written.
*/
ostrnfree (node)
register OSNODE	*node;
{
	i4 token = node->n_token;

	MEfill(sizeof(*node), '\0', (PTR)node);
	node->n_token = FREE_NODE;
    	if (token != FREE_NODE)
    	{
		/* Attach to the free list. */
		node->n_left = Trfree;
		Trfree = node;
    	}
}

/*{
** Name:	osiserr() -	Has an error been reported on this node?
**
** Description:
**
** History:
**	10/89 (billc) -- Written.
*/
bool
osiserr (node)
OSNODE	*node;
{
	OSSYM *sym;

	if (node == NULL || node->n_flags & N_ERROR)
		return TRUE;
	if ((sym = osnodesym(node)) != NULL && sym->s_kind == OSUNDEF)
		return TRUE;
	return FALSE;
}
