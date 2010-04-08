/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_X_ADJUST - set up the flag for adjusting the next text, in
**	response to a .center, .right, or .left command.  All
**	printing text will be stored up in the adjusting buffers
**	until the next adjust text, or tab or newline command.
**
**	Parameters:
**		tcmd	TCMD structure of .center, .left or .right.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Changes St_adjusting, St_a_tcmd, St_left_margin,
**		St_right_margin..
**
**	Called by:
**		r_x_tcmd.
**
**	Trace Flags:
**		5.0, 5.11.
**
**	Error Messages:
**		Syserr:Width value too small.
**
**	History:
**		7/20/81 (ps)	written.
**		6/7/84	(gac)	changed r_eval_pos to f4.
**		12/21/89 (elein)
**		Fixed call to r_syserr
**		1/10/90 (elein)
** 			Ensure call to rsyserr uses i4 ptr
**		3/21/90 (elein) performance
**			Check St_no_write before calling r_set_dflt
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


VOID
r_x_adjust(tcmd)
TCMD	*tcmd;
{
	/* internal declarations */

	f4		position;		/* position of adjusting */
	i4		width;			/* width of current margins */

	/* start of routine */



	if( St_no_write)
	{
		r_set_dflt();
	}


	r_push_env();			/* save current environment */

	/* set up appropriate right and left margins */

	position = r_eval_pos(tcmd,St_cline->ln_curpos,TRUE);

	switch(tcmd->tcmd_code)
	{
		case(P_CENTER):
			/* width will be 2X the shortest distance from
			** position to right or left end of buffer */
			if (position < (En_lmax - position))
				width = 2 * position;
			else
				width = 2 * (En_lmax - position);

			break;

		case(P_RIGHT):
			width = position;
			break;

		case(P_LEFT):
			width = En_lmax - position;
			break;

	}


	if (width <= 0)
	{
		r_syserr(E_RW004C_r_x_adjust_Field_widt,&width);
	}

	St_left_margin = 0;
	St_right_margin = width;

	r_ln_clr(Ptr_cln_top);		/* set up adjusting buffers */

	St_adjusting = TRUE;
	St_a_tcmd = tcmd;

	return;
}

