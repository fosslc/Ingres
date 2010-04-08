/*
**  Copyright (c) 1986, 2003 Ingres Corporation
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<oskeyw.h>
#include	<oslconf.h>
#include	<iltypes.h>
#include	<ilops.h>

#if defined(i64_win)
#pragma optimize("", off)
#endif

/**
** Name:    ossubsys.c -	OSL Translator Call Sub-System Module.
**
** Description:
**	Contains routines that generate IL code to call an Ingres sub-system
**	with the indicated parameters.  Defines:
**
**	ossubsys()	generate IL code for sub-system call.
**
** History:
**	Revision 6.0  87/07  wong
**	Changed to concatenate the parameter list string by generating an
**	expression.  Also, fixed bug that was failing to concatenate,
**	"param=%S", at the end of the list when a "param" element was found.
**
**	Revision 5.1  86/10/17  16:41:03  wong
**	Initial revision.  10/07  wong
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-aug-2003 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent oslsql.exe from SEGVing
**	    during the run of 4GL SEP test fgl69.sep .
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
**/

/*{
** Name:    ossubsys() -	Generate IL Code for Sub-System Call.
**
** Description:
**	This routines generates IL code to construct the parameter list
**	structure for a call to an Ingres subsystem and then generates code
**	for the call.
**	
**	The call
**
**		call x (name1 = expr1, name2 = expr2, ..., nameN = exprN);
**
**	generates
**		[ code for expressions and for parameter list ]
**		CALSUBSYS VALUE VALUE INT
**		TL1ELM VALUE
**		...
**		ENDLIST
**
**	Note that a "param" element in the parameter list will be moved to
**	the end of the list as a special-case to support the Report Writer.
**
** Input:
**	subsys	{OSNODE *}  The node representing the subsystem name.
**	params	{OSNODE *}  A node list of the parameters.
**
** History:
**	10/86 (jhw) -- Written.
**	07/87 (jhw) -- Changed to concatenate parameter string by generating
**			an expression.
**	02-sep-93 (connie) Bug #50059
**		provided a len argument to the 2nd string in STbcompare in
**		order to be consistent with Utexe in the way of recognizing 
**		the parameter 'param'
*/
VOID
ossubsys (subsys, params)
OSNODE	*subsys;
OSNODE	*params;
{
    register OSNODE	*list;
    register char	*bp;
    OSNODE		*rparam = NULL;	/* report `param' parameter */
    U_ARG		expr;
    U_ARG		plus;
    i4			npar = 0;
    char		buf[IL_MAX_STRING+1];

    char	*iiIG_string();

    plus.u_cp = _Plus;	/* constant */

    bp = buf;
    expr.u_nodep = NULL;
    for (list = params ; list != NULL ; list = list->n_next)
    {
	register OSNODE	*pn;
	OSNODE		*np = list->n_ele;

	if (np == NULL || np->n_token != ASNOP || (pn=np->n_left) == NULL)
	    osuerr(OSBUG, 1, ERx("ossubsys"));

	if (list->n_next != NULL && rparam == NULL &&
		((pn->n_token == tkSCONST &&
			STbcompare(pn->n_const, 0, ERx("param"), 5, TRUE) == 0)
	       ||
		 (pn->n_token == tkID &&
			STbcompare(pn->n_value, 0, ERx("param"), 5, TRUE) == 0))
	   )
	{ /* `param' parameter for Report Writer */
	    rparam = np;
	    ostrfree(pn);
	}
	else
	{ /* All other parameters */
	    if (pn->n_token == VALUE ||
		    (bp - buf + STlength(pn->n_const) + 3) > sizeof(buf))
	    { /* flush buffer */
		if (bp != buf)
		{
		    U_ARG	carg;

		    *bp = EOS;
		    carg.u_nodep = osmkconst(tkSCONST, iiIG_string(buf));
		    expr.u_nodep = (expr.u_nodep != NULL
			? osmknode(OP, &plus, &expr, &carg) : carg.u_nodep);
		}
		bp = buf;
	    }
	    /* Append parameter name */
	    if (pn->n_token == VALUE)
	    {
		U_ARG	value;

		value.u_nodep = pn;
		expr.u_nodep = (expr.u_nodep != NULL
			? osmknode(OP, &plus, &expr, &value) : value.u_nodep);
	    }
	    else
	    {
		STcopy(pn->n_const, bp);
		bp += STlength(bp);
		ostrfree(pn);	/* free constant nodes only */
	    }
	    /* Append type */
	    bp += STlength( STprintf(bp, ERx("=%%%c"),
				oschkstr(np->n_right->n_type) ? 'S'
					: 'N'
				)
	    );
	    if (list->n_next != NULL || rparam != NULL)
		*bp++ = ',';

	    /* Re-use node for parameter value list */
	    np->n_token = TLASSIGN;
	    np->n_tlexpr = osvarref(np->n_right);
	}
	++npar;
    } /* end for */
    if (rparam != NULL)
    {
	static char	param[] = ERx("param=%%S");

	_VOID_ STprintf(bp, param);
	bp += sizeof(param) - 1;
	rparam->n_token = TLASSIGN;
	rparam->n_tlexpr = osvarref(rparam->n_right);
    }
    if (bp != buf)
    { /* flush buffer */
	U_ARG	carg;

	*bp = EOS;
	carg.u_nodep = osmkconst(tkSCONST, iiIG_string(buf));
	expr.u_nodep = (expr.u_nodep != NULL
		? osmknode(OP, &plus, &expr, &carg) : carg.u_nodep);
    }

    IGgenStmt(IL_CALSUBSYS, (IGSID *)NULL, 3, osvalref(subsys),
	expr.u_nodep != NULL ? osvalref(expr.u_nodep) : (ILREF)0,
	npar
    );

    /* Generate parameter value list */

    for (list = params ; list != NULL ; list = list->n_next)
    {
	if (list->n_ele != rparam)
	    IGgenStmt(IL_TL1ELM, (IGSID *)NULL, 1, list->n_ele->n_tlexpr);
    }
    if (rparam != NULL)
	IGgenStmt(IL_TL1ELM, (IGSID *)NULL, 1, rparam->n_tlexpr);
    IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);

    ostrfree(subsys);
    ostrfree(params);
}
