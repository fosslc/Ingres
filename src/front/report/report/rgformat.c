/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_G_FORMAT - get a format specification from the token line
**	and set up an FMT structure.  This calls fmt_setfmt to
**	set up the FMT structure after allocating the needed
**	space for the structures.
**
**	Parameters:
**		none.
**
**	Returns:
**		0 - error, no format set up.
**		address of FMT structure set up, if good.
**
**	Side Effects:
**		Changes Tokchar.
**
**	Calls:
**		get_space and f_setfmt.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		2.0, 2.8.
**
**	History:
**		6/22/81 (ps) 	written.
**		23-jan-1989 (danielt)
**			changed to use MEreqmem().
**		8/14/89 (elein) garnet
**			Add variable evaluation for formats
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



FMT	*
r_g_format()
{
	/* internal declarations */

	FMT	*fmt;				/* test format structure */
	PTR	area;
	i4	size;
	STATUS	status;
	i4	fmtlen;
	char	*aname;
	char	*saveTok;
	i4	varlen;

	/* start of routine */


	if (Tokchar == NULL)
	{
		return(NULL);
	}
	varlen = 0;
	saveTok = Tokchar;
	if ( r_g_skip() == TK_DOLLAR)
	{
		CMnext(Tokchar);	/* skip $ */
		aname = r_g_name();
		varlen = STlength(aname) + 1; /* one for the dollar */
		if ((Tokchar = r_par_get(aname)) == NULL)
		{	/* Couldn't find parameter */
			r_error(1008, NONFATAL, aname, NULL);
			return (NULL);
		}
	}



	/* allocate area needed to build a FMT structure */

	fmt_areasize(Adf_scb, Tokchar, &size);
	area = (PTR) MEreqmem(0,(u_i4) size,TRUE,(STATUS *) NULL);

	/* set up the FMT structure */

	status = fmt_setfmt(Adf_scb, Tokchar, area, &fmt, &fmtlen);
	/*
	 * In case we switched Tokchar to evaluate a variable, switch back
	 * Increment past the format (fmtlen) or variable (varlen)
	 * depending on which it was.
	 */
	Tokchar = saveTok;
	Tokchar += varlen == 0 ? fmtlen:varlen;

	if (status != OK)
	{
		FEafeerr(Adf_scb);
		return(NULL);
	}
	return(fmt);
}
