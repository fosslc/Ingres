/*
**  Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfskip.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFSKIP - skip TCMD structures in the linked list until either the
**	end of the list, or an end of a type of block is found.
**
**	Parameters:
**		TRUE	if an end to the block of the type currently
**			in Cact_tcmd is to return.
**		FALSE	if an end of block is not to be found.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update Cact_tcmd;
**
**	Called by:
**		rFdisplay.
**
**	Trace Flags:
**		200, 215.
**
**	Error Messages:
**		none.
**
**	History:
**		2/15/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
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
rFskip(flag)
bool	flag;
{
	/* internal declarations */

	i4		stopcode;		/* code to stop the skip */

	/* start of routine */

#	ifdef	xRTR1
	if(TRgettrace(200,0) || TRgettrace(215,0))
	{
		SIprintf(ERx("rFskip:entry.\n"));
	}
#	endif

#	ifdef	xRTR2
	if (TRgettrace(215,0))
	{
		SIprintf(ERx("	flag:%d\n"),flag);
		r_pr_tcmd(Cact_tcmd,FALSE);
	}
#	endif

	if (flag)
	{
		stopcode = Cact_tcmd->tcmd_code;
	}

	while (Cact_tcmd!=NULL)
	{
		if (flag && (Cact_tcmd->tcmd_code==P_END))
		{
			if (stopcode == Cact_tcmd->tcmd_val.t_v_long)
			{	/* get out */
				break;
			}
		}
		Cact_tcmd = Cact_tcmd->tcmd_below;
	}

#	ifdef	xRTR2
	if (TRgettrace(215,0))
	{
		SIprintf(ERx("	At end of rFskip.\n"));
		r_pr_tcmd(Cact_tcmd,FALSE);
	}
#	endif

	return;
}
