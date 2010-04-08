/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	 <rglob.h>
# include        <er.h>
# include	"ersr.h"

/*
**   S_RSO_FIND - find an RSO structure for a given name.
**
**	Parameters:
**		name - name of sort attribute (or report,page or detail)
**
**	Returns:
**		address of matching RSO structure or NULL.
**
**	Called by:
**		s_header, s_footer.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		13.0, 13.3.
**
**	History:
**		6/1/81 (ps) - written.
**		9/19/89 (elein) Bug 8055 Gateways
**		Check explicitly for report, page or detail.
**		These elements were initialized in lower case
**		(NAM_*) in srenset.qsc, and they so need a case
**		insentive comparison.
**		1/26/90 (elein) 9876
**		When using STbcompare, do not send the real lengths
**		use 0 or else matches like page = pageno will come
**		up causing 'undocumented' keywords.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



RSO	*
s_rso_find(name)
char	*name;
{
	/* internal declarations */

	RSO	*rso;				/* temp ptr to new rso struct */
	char 	*tname;				/* Real Name or PAGE/REPORT/DETAIL */
	i4 	namelen;			/* Name length */

	/* start of routine */

	if (name == NULL)
	{
		r_syserr(E_SR000E_s_rso_find_Null_name);
	}

	/* if no report currently in effect, abort */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);	/* ABORT!!!!!! */
	}

	/* for gateways, we may fail the comparison against
	 * REPORT, PAGE or DETAIL due to case sensitiviy.
	 * Find out if we've got one of those and use
	 * the constant name instead.  That is how it is
	 * initialized in the Cact_ren list
	 */
	namelen = STlength(name);
	if( STbcompare(name, 0, NAM_REPORT, 0, TRUE ) == 0 )
	{
		tname = NAM_REPORT;
	}
	else if( STbcompare(name, 0, NAM_PAGE, 0, TRUE ) == 0 )
	{
		tname = NAM_PAGE;
	}
	else if( STbcompare(name, 0, NAM_DETAIL, 0, TRUE ) == 0 )
	{
		tname = NAM_DETAIL;
	}
	else
	{
		tname = name;
	}

	/* find corresponding name to this one */

	for(rso=Cact_ren->ren_rsotop; rso!=NULL; rso=rso->rso_below)
	{
		if (STequal(tname,rso->rso_name) )
		{
			return(rso);
		}
	}


	return(NULL);
}
