/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>
#include	<oslconf.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:    oscall.c -		OSL Translator Call Statement Parameter Module.
**
** Description:
**	Contains routines that generate IL code to build the parameter
**	structures for call frame/procedure semantics and generate the call.
**	Defines:
**
**	osgencall()	generate IL code for parameter list and call.
**
** History:
**	Revision 5.1  86/10/17  16:39:33  wong
**	Initial revision.
**
**	Revision 6.0  87/06  wong
**	Added support for `Null' constants.  (87/06  wong)
**	Modified so that positional parameters are evaluated here.  (Necessary
**	to support ADF function calls, which evaluate their parameters as part
**	of the expression.)  02/87  (wong)
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in osgencall).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Modified osgencall (for bugs 39581, 41013, and 41014).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Small change in paramqry for bug 34846.
**
**	Revision 7.0
**	16-jun-92 (davel)
**		minor modification for decimal support.
**	25-aug-92 (davel)
**		add tkEXEPROC case for EXECUTE PROCEDURE.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
*/

VOID	ostmphold();

struct counts {
    i4	params;
    i4	formals;
    i4	queries;
    bool	dotall;
};

/*{
** Name:    osgencall() -	Generate IL Code for Parameter List and Call.
**
** Description:
**	This routines generates IL code to construct the parameter list
**	structure for a call to a frame or procedure and then generates code
**	for the call.
**	
**	The call
**		callproc x (expr1, expr2, ..., exprN);
**	generates
**		PARAM VALUE VALUE INT INT INT
**		TL1ELM VALUE
**		...
**		ENDLIST
**		PVELM VALUE INT
**		...
**		ENDLIST
**		CALL(F|P) VALUE
**
** Input:
**	callp	{OSNODE *}  The call frame/procedure node.
**	result	{OSSYM *}  The symbol table entry for the returned value.
**
** History:
**	09/86 (jhw) -- Written.
**	02/87 (jhw) -- Report multiple queries in parameter list.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Generate IL_CALLPL instead of IL_CALLP if the procedure is local
**	11/07/91 (emerson)
**		Added call to ostmphold (for bugs 39581, 41013, and 41014).
*/

static VOID	paramcnt();
static ILREF	paramqry();
static VOID	paramformals();
static VOID	params();

VOID
osgencall (callp, result)
register OSNODE	*callp;
OSSYM		*result;
{

    if (callp == NULL)
	return;

    /*
    ** "Hold" the enclosing temporary block (in case the called frame
    ** or procedure releases array row temporaries).
    */
    ostmphold();

    if (callp->n_parlist != NULL || result != NULL)
    { /* generate IL to set up parameter structure (including return value) */
	struct counts	counts;

	counts.params = counts.formals = counts.queries = 0;
	counts.dotall = FALSE;
	paramcnt(&callp->n_parlist, &counts);

	if (counts.queries > 1)
	    oscerr(OSEXTRQRY, 0);

	IGgenStmt(IL_PARAM, (IGSID *)NULL, 5,
		(result == NULL ? (ILREF)0 : result->s_ilref),
		(counts.queries == 0 ? (ILREF)0 : paramqry(callp->n_parlist)),
		(IL)counts.formals, (IL)counts.params, (ILREF)counts.dotall
	);
	if (counts.formals != 0)
	{
	    paramformals(callp->n_parlist);
	    IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	}
	if (counts.params != 0)
	    params(callp->n_parlist);
	IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
    }

    /* Call frame/procedure */

    if (callp->n_token == tkFCALL)
    {
	/* call a frame */
	IGgenStmt(IL_CALLF, (IGSID *)NULL, 1, osvalref(callp->n_proc));
    }
    else if (callp->n_token == tkEXEPROC)
    {
	/* generate for EXECUTE PROCEDURE */
	IGgenStmt(IL_CALLP|ILM_EXEPROC, (IGSID *)NULL, 
				1, osvalref(callp->n_proc));
    }
    else if (callp->n_psym == NULL || callp->n_psym->s_kind != OSFORM)
    {
	/* call a non-local procedure */
	IGgenStmt(IL_CALLP, (IGSID *)NULL, 1, osvalref(callp->n_proc));
    }
    else
    {
	/* call a local procedure */
	IGgenStmt(IL_CALLPL, callp->n_psym->s_sid, 1, osvalref(callp->n_proc));
    }

    osqgfree(callp->n_parlist);		/* free any query parameters */
}

/*
** Name:	paramcnt() -	Traverse Parameter List Counting Elements.
**
** Description:
**	This routine walks the tree representing a parameter list and counts
**	the number of parameters, formals, and queries in the list.
**
**	(Now processes expression and BYREF nodes into TLASSIGN nodes.)
**
** Input:
**	plist	{OSNODE **}  The address of a node or sub-tree of the parameter
**			     list, which may now be changed by this routine.
**	cnts	{struct counts *}  The counts structure.
**
** History:
**	09/86 (jhw) -- Written.
**	02/87 (jhw) -- Modified to process expression or BYREF nodes.
**	16-jun-92 (davel)
**		added case for DCONST (decimal support)
*/
static VOID
paramcnt (plist, cnts)
register OSNODE	**plist;
struct counts	*cnts;
{
    if (*plist != NULL)
    {
	register OSNODE	*pnode = *plist;

    	switch (pnode->n_token)
	{
	  case COMMA:
    	  {
	    paramcnt(&pnode->n_left, cnts);
	    paramcnt(&pnode->n_right, cnts);
	    break;
	  }

	  case DOTALL:
	  {
	    register i4	cnt = 0;

	    cnts->dotall = TRUE;
	    for (pnode = pnode->n_slist ; pnode != NULL ; pnode = pnode->n_next)
	    	++cnt;
	    cnts->params += cnt;
	    cnts->formals += cnt;
	    break;
	  }

	  case tkQUERY:
	    ++cnts->queries;
	    break;

	  case TLASSIGN:
	  {
	    ++cnts->params;
	    if (pnode->n_coln != (ILREF)0)
	    	++cnts->formals;
	    break;
	  }

	  case ARRAYREF:
	  case DOT:
	  case OP:
	  case UNARYOP:
	  case tkPCALL:
	  case VALUE:
	  case tkSCONST:
	  case tkICONST:
	  case tkFCONST:
	  case tkXCONST:
	  case tkDCONST:
	  case tkNULL:
	    ++cnts->params;
	    *plist = osmkassign((ILREF)0, osvarref(pnode), FALSE);
	    break;

	  case tkBYREF:
	    ++cnts->params;
	    *plist = osmkassign((ILREF)0, osvarref(pnode), TRUE);
	    break;

	  default:
	    osuerr(OSBUG, 1, ERx("osgencall()"));	/* exit ! */
	} /* end switch */
    }
}

/*
** Name:	paramqry() -	Return Query Node Reference.
**
** Description:
**	This routine walks a tree representing a parameter list looking
**	for a query node.  When it finds one, it generates a query structure
**	for the query and returns a reference to it.
**
** Input:
**	plist	{OSNODE *}  A node or sub-tree of the parameter list.
**
** History:
**	09/86 (jhw) -- Written.
**	09/20/92 (emerson)
**		Changed calling sequence to osqrygen (for bug 34846).
*/
static ILREF
paramqry (plist)
register OSNODE	*plist;
{
    if (plist != NULL)
    {
	if (plist->n_token == COMMA)
	{
	    ILREF	tmp;

	    if ((tmp = paramqry(plist->n_left)) != (ILREF)0)
		return tmp;
	    return paramqry(plist->n_right);
	}
	else if (plist->n_token != tkQUERY)
	    return (ILREF)0;
	else
	{ /* a query node */
	    osqrygen(plist, OS_CALLF, (bool)TRUE, (IGSID *)NULL, (IGSID *)NULL);
	    return plist->n_ilref;
	}
    }
    return (ILREF)0;
}

/*
** Name:	paramformals() -	Generate List of Formals.
**
** Description:
**	This routine walks a tree representing a parameter list generating
**	an IL list element for each formal it finds.
**
** Input:
**	plist	{OSNODE *}  A node or sub-tree of the parameter list.
**
** History:
**	09/86 (jhw) -- Written.
**	10/90 (jhw) -- Don't generate TL1ELM for parameter only (no keyword)
**		assignments.
*/
static VOID
paramformals (plist)
register OSNODE	*plist;
{
    if (plist != NULL)
    {
	if (plist->n_token == COMMA)
	{
	    paramformals(plist->n_left);
	    paramformals(plist->n_right);
	}
	else if ( plist->n_token == TLASSIGN && plist->n_coln != (ILREF)0 )
	    IGgenStmt(IL_TL1ELM, (IGSID *)NULL, 1, plist->n_coln);
	else if (plist->n_token == DOTALL)
	{
	    for (plist = plist->n_slist ; plist != NULL ; plist = plist->n_next)
	    {
		register OSNODE	*ref = plist->n_ele;
		register OSSYM	*symr;

		symr = (ref->n_tfref == NULL ? ref->n_sym : ref->n_tfref);
		IGgenStmt(IL_TL1ELM, (IGSID *)NULL,
			1, IGsetConst(DB_CHA_TYPE, symr->s_name)
		);
	    }
	}
    }
}

/*
** Name:	params() -	Generate List of Parameter Values.
**
** Description:
**	This routine walks a tree representing a parameter list generating
**	a parameter value element for each parameter.  A parameter value
**	element consist of the value and a reference flag showing how
**	the value is passed (by reference or value.)
**
** Input:
**	plist	{OSNODE *}  A node or sub-tree of the parameter list.
**
** History:
**	09/86 (jhw) -- Written.
*/
static VOID
params (plist)
register OSNODE	*plist;
{

    if (plist != NULL)
    {
	if (plist->n_token == COMMA)
	{
	    params(plist->n_left);
	    params(plist->n_right);
	}
	else if (plist->n_token == TLASSIGN)
	    IGgenStmt(IL_PVELM, (IGSID *)NULL,
			2, plist->n_tlexpr, plist->n_byreff
	    );
	else if (plist->n_token == DOTALL)
	{
	    for (plist = plist->n_slist ; plist != NULL ; plist = plist->n_next)
		IGgenStmt(IL_PVELM, (IGSID *)NULL,
			2, osvalref(plist->n_ele), FALSE
		);
	}
    }
}
