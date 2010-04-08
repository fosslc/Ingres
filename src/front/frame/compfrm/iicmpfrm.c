# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<ut.h>

/*
**  interface to compfrm as UT_CALL module
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)iicompfrm.c	30.4	2/9/85";
**
**	2/9/85 (peterk) - eliminated EX style of return
**		since it appears to be unneccessary.
**	08/31/90 (dkh) - Integrated porting change ingres6202p/131116.
**	09/18/90 (dkh) - Put in explanation that got lost in the code shuffle.
*/

GLOBALREF	i4	fceqlcall;
GLOBALREF 	char	*fcnames[];
GLOBALREF	bool	fcfileopened;

i4
IIcompfrm(frmname, frameptr, symbol, outfile, macro)
char	*frmname;
FRAME	*frameptr;
char	*symbol;
char	*outfile;
i4	macro;
{
	fceqlcall = TRUE;

	/* Initialize */

	fcoutfp = NULL;
	fcntable = fcnames;
	fclang = DSL_C;
	fcrti = FALSE;
	fcstdalone = FALSE;
	fcfileopened = FALSE;

	/*
	**  Set form name.  Note that since we are only
	**  processing one form at a time and that the
	**  "fcnames" array (which is what fcntable points
	**  to) is initialized to NULL we don't need to
	**  put a NULL pointer guard after the form name
	**  pointer.  If we decide to go back to processing
	**  multiple forms at a time, then this code of course
	**  needs to change.
	*/
	*fcntable++ = frmname;

	/*
	**  Set the frame pointer.
	*/
	fcfrm = frameptr;

	/*
	**  Set the symbol name.
	*/
	fcname = symbol;

	/*
	**  Set the output file name.
	*/
	fcoutfile = outfile;

	/*
	**  Set macro toggle.
	*/
	if (macro)
	{
		fclang = DSL_MACRO;
	}

	return(fccomfrm());
}
