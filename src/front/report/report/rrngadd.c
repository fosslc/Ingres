/*
** Copyright (c) 2004 Ingres Corporation
*/
/* ===ICCS===\50\3\3\   ingres 5.0/03  (pyr.u42/01)     */

/* static char	Sccsid[] = "@(#)rrngadd.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<ug.h>
# include	<er.h>

/*
**   R_RNG_ADD - add a range_var/table combination to the RNG linked list.
**	The RANGE statement has been parsed by SREPORT, so it should
**	contain only two words--the range_var, and the table name.
**	Note that the strings are not moved, so they should reflect
**	immovable strings.
**
**	This routine has been used for .DATA statements, which are
**	executed as SQL 'SELECT *'.  Thus, there is no differentiation
**	between 'string1' and 'string2'.  Further, 'string' may represent
**	an owner.tablename construct where eithe component may be
**	a delimited identifier.
**
**	Parameters:
**	    If called because of a RANGE statement:
**		string1	string from the "rcoattid" field in the RCOMMANDS
**			table which should contain the range_var.
**		string2 string from the "rcotext" field in the RCOMMANDS
**			table which should contain the table name.
**
**	    If called because of a .DATA command:
**		string1	name of the table on which the report is to be based.
**			This is expected to be in the unnormalized form.
**		string2 NULL.
**
**	    Note the having string2 == NULL effectively differentiates SQL/QUEL
**	    contexts.
**
**	Returns:
**		TRUE	if ok.
**		FALSE	if anything goes wrong.
**
**	Called by:
**		r_qur_set (RANGE)
**		r_chk_dat (.DATA)
**
**	Side Effects:
**		May set the Ptr_rng_top pointer.
**
**	Error Messages:
**		1008.
**
**	Trace Flags:
**		2.0, 2.12.
**
**	History:
**		7/1/81 (ps)	written.
**		12/29/81 (ps)	modified for two table version.
**		7/7/82 (ps)	allow parameters in range statements.
**		12/19/86 (mgw)	Bug # 10896
**			Added CVlower() calls to lowercase the range variable
**			and tablename so they can be found in the symbol table.
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		08/16/89 (elein) garnet
**			Changed error message number for bad parameters
**		27-oct-1992 (rdrane)
**      		Redefined the interface to distinguish between QUEL
**			RANGE invocations and SQL .DATA invocations.  This is
**			necessary in order to effect proper casing of QUEL
**			variables while support 6.5 SQL delimited ID's.
**			Converted r_error() err_num values to hex to facilitates
**			lookup in errw.msg file.  Removed 'fetch' variable -
**			it wasn't being used to pass an address, so why not
**			let FEreqmem return directly into the REGISTER variable
**			rng?
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



bool
r_rng_add(string1,string2)
char	*string1;
char	*string2;
{
	/* internal declarations */

	register RNG	*rng;		/* fast ptr to new RNG structure */
	RNG		*fetch;		/* need to pass address to allocate */
	register RNG	*orng;		/* previous structure */
	char		*pval;		/* value of parameter */

	/* start of routine */



	if ((rng = (RNG *)FEreqmem ((u_i4)Rst4_tag,
			(u_i4)(sizeof(RNG)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_rng_add"));
	}

	if (*string1 == '$')
	{	/* parameter */
		if ((pval = r_par_get(string1+1)) == NULL)
		{	/* bad parameter */
			r_error(0x3F0, NONFATAL, string1, NULL);
			pval = string1;
		}
		string1 = pval;
	}		

	if ((string2 != NULL) && (*string2 == '$'))
	{	/* parameter */
		if ((pval = r_par_get(string2+1)) == NULL)
		{	/* bad parameter */
			r_error(0x3F0, NONFATAL, string2, NULL);
			pval = string2;
		}
		string2 = pval;
	}		

	if  (string2 != NULL)
	{
		/* Begin fix for bug 10896 */
		IIUGdbo_dlmBEobject(string1,FALSE);
		IIUGdbo_dlmBEobject(string2,FALSE);
		/* End fix for bug 10896 */
		rng->rng_rvar = string1;
		rng->rng_rtable = string2;
	}
	else
	{
		/*
		** For SQL, leave the tablename as is in its
		** full, unnormalized form.
		*/
		rng->rng_rvar = string1;
		rng->rng_rtable = string1;
	}

	if (Ptr_rng_top == NULL)
	{
		Ptr_rng_top = rng;
	}
	else
	{
		for (orng = Ptr_rng_top; orng->rng_below != NULL;
		     orng = orng->rng_below)
		{
			;
		}
		orng->rng_below = rng;
	}


	return(TRUE);
}
