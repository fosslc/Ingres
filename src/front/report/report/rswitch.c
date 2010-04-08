/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rswitch.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_SWITCH - switch environments between the page environment
**	and the report environment.  This changes a number of
**	program pointers from one set to another.
**
**	Parameters:
**		none.  A check is made to see if St_in_pbreak is
**			TRUE.  If TRUE, then change to page buffers.
**			If FALSE, change to report buffers.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_set, r_do_page.
**
**	Side Effects:
**		Changes margins, Cact_tcmd, Line buffers, BE stack, Within
**		variables and pushes the environment.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		160, 162.
**
**	History:
**		1/13/82 (ps)	written.
**		5/11/83 (nl) -  Put in changes made since code was ported 
**		5/11/83 (nl) -  Put in changes made since code was ported
**				to UNIX. Added refs to St_h_tname and 
**				St_h_attribute and St_h_written to fix
**				bug 937.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_switch()
{
	/* internal declarations */

	static	bool	firsttime = TRUE;	/* first time to this routine */
	static	TCMD	*rtcmd = NULL;		/* store old version of report Cact_tcmd */

	/* start of routine */



	if (St_in_pbreak)
	{	/* switch to page.  Save current environment */

		r_push_env();

		/* Save current header and break info */

		St_h_tname = Cact_tname;
		St_h_attribute = Cact_attribute;
		St_h_written = Cact_written;

		St_left_margin = St_p_lm;
		St_right_margin = St_p_rm;
		St_underline = FALSE;
		Cact_attribute = NAM_PAGE;

		Ptr_t_cbe = Ptr_t_pbe;
		St_cbe = St_pbe;
		St_cwithin = St_pwithin;
		St_ccoms = St_pcoms;
		St_c_cblk = St_c_pblk;
		rtcmd = Cact_tcmd;
		Cact_tcmd = NULL;
		Cact_written = FALSE;
		return;
	}

	if (!firsttime)
	{
		r_pop_env();
	}

	firsttime = FALSE;

	/* in either case */

	Ptr_t_cbe = Ptr_t_rbe;
	St_cbe = St_rbe;
	St_cwithin = St_rwithin;
	St_ccoms = St_rcoms;
	St_c_cblk = St_c_rblk;

	Cact_tcmd = rtcmd;

	Cact_tname = St_h_tname;
	Cact_attribute = St_h_attribute;
	Cact_written = St_h_written;

	return;
}
