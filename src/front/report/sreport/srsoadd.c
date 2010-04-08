/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<ug.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	"ersr.h"

/*
**  S_RSO_ADD - add an RSO structure for another sort attribute to the
**	current report.	 If this is the first (report) RSO structure,
**	put the pointer into the REN structure also to start the linked
**	list.
**	If the RSO structure already exists, return with add of it.
**
**	Parameters:
**		name - name of sort attribute (or report,page or detail)
**
**		incorder - TRUE if rso_order field is to be set (for
**				sort attributes only).
**
**	Returns:
**		address of added RSO structure, or found structure.
**
**	Called by:
**		s_header, s_footer, s_rep_set.
**		s_detail.
**
**	Side Effects:
**		Sets the En_n_sorted field.
**
**	Trace Flags:
**		13.0, 13.3.
**
**	History:
**		6/1/81 (ps) - written.
**		3/20/90 (elein) performance
**			Change STcompare to STequal
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file. Eliminate references to
**			r_syserr() and use IIUGerr() directly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



RSO	*
s_rso_add(name,incorder)
char	*name;
bool	incorder;
{
	/* internal declarations */

	RSO	*rso;				/* temp ptr to new rso struct */
	RSO	*orso = NULL;			/* ptr to old rso struct */

	/* start of routine */



	if (name == NULL)
	{
		IIUGerr(E_SR000D_s_rso_add_Null_name,UG_ERR_FATAL,0);
	}

	/* if no report currently in effect, abort */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);	/* ABORT!!!!!! */
	}

	/* if RSO exists for this name, return it. If not, create one */

	for(rso=Cact_ren->ren_rsotop; rso!=NULL; rso=rso->rso_below)
	{
		if (STequal(name,rso->rso_name) )
		{
			return(rso);
		}
		orso = rso;		/* save old one */
	}

	/* gets here only if name not in list */

	rso = (RSO *) MEreqmem(0, (u_i4) sizeof(RSO), TRUE, (STATUS *) NULL);
	if (STlength(name) > DB_MAXNAME)
	{
		*(name+DB_MAXNAME) = '\0';		/* truncate it */
	}

	rso->rso_name = (char *) STalloc(name);

	if (orso == NULL)
	{
		Cact_ren->ren_rsotop = rso;
	}
	else
	{
		orso->rso_below = rso;
	}

	if (incorder == TRUE)
	{	/* add sort order */
		if (orso == NULL)
		{
			rso->rso_order = 0;
		}
		else
		{


			rso->rso_order = orso->rso_order+1;
		}

	}

	En_n_sorted++;

	return(rso);
}
