/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfclrcopt.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h> 
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFCLR_COPT - clear out a COPT structure and set to a default.
**
**	Parameters:
**		copt	address of COPT to clear.
**
**	Returns:
**		copt	same address.
**
**	Called by:
**		rFmlayout.
**
**	Side Effects:
**		none.
**
**	History:
**		8/31/82 (ps)	written.
**		2/3/87 (rld)	changed to reflect change in COPT struct.
**      	9/22/89 (elein) UG changes ingresug change #90045
**		changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		08-jan-90 (cmr)	set defaults for brkhdr and brkftr.
**		08-feb-90 (cmr) the default for copt_order is now 'a'.
**		9-mar-1993 (rdrane)
*			Default copt_var_name to an empty string (no value/not yet set).
*/



COPT	*
rFclr_copt(copt)
register COPT	*copt;
{

	/* start of routine */

	if (copt == (COPT *)NULL)
	{
		return((COPT *)NULL);
	}

	copt->copt_sequence = 0;
	copt->copt_order = 'a';
	copt->copt_break = 'n';
	copt->copt_whenprint = 'a';
	copt->copt_newpage = FALSE;
	copt->copt_brkhdr = 'n';
	copt->copt_brkftr = 'n';
	copt->copt_select = 'n';
	copt->agginfo = NULL;
	copt->copt_var_name[0] = EOS;

	return(copt);
}
