/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmpbot.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_PBOT - set up the page bottom in a default report.
**	This will contain the current page number, centered.
**	This will also print out aggregates if needed.
**
**	Parameters:
**		labove	number of lines above the footing.
**		lbelow	number of lines below the footing.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		rFsave.
**
**	Trace Flags:
**		180, 185.
**
**	History:
**		4/4/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		22-nov-22 (cmr)	removed call to rFchk_agg(), rFm_aggs() and
**				associated code; no longer needed now that
**				we have visible editable aggregates (rFm_sec()
**				does all the work now).
**	  2/20/90 (martym)   Changed to return STATUS passed back by the 
**			     SREPORTLIB routines.
**      2-nov-1992 (rdrane)
**          Remove all FUNCT_EXTERN's since they're already
**          declared in included hdr files.  Ensure that we generate
**          string constants with single, not double quotes.
**	2-jul-1993 (rdrane)
**		Added support for user override of hdr/ftr date/time
**		and page # formats.  Output the report variables with
**		reference to the declared formats.
**	13-jul-1993 (rdrane)
**		Eliminate space between current_page and its open format
**		parens.  This is consistent w/ SREPORT formatting, and
**		eliminates DIFF's between RBF archive and COPYREP/SREPORT
**		results.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/



STATUS rFm_pbot(labove,lbelow)
i4	labove;
i4	lbelow;
{
	/* internal declarations */

	char	ibuf[20];		/* hold result of CVna */
	STATUS  stat = OK;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0) || TRgettrace(185,0))
	{
		SIprintf(ERx("rFm_pbot: entry.\r\n"));
	}
#	endif

#	ifdef	xRTR1
	if (TRgettrace(185,0))
	{
		SIprintf(ERx("	labove:%d\r\n"),labove);
		SIprintf(ERx("	lbelow:%d\r\n"),lbelow);
	}
#	endif

	RF_STAT_CHK(s_w_action(ERx("begin"), ERx("rbfpbot"), (char *)0));
	if (labove > 0)
	{	/* lines above the footing */
		CVna(labove,ibuf);
		RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
	}

	if  (Opt.rpageno_inc_fmt == ERx('y'))
	{
		RF_STAT_CHK(s_w_action(ERx("center"),(char *)0));
		RF_STAT_CHK(s_w_action(ERx("print"), 
			    ERx("page_number($"),
			    RBF_NAM_PAGENO_FMT,ERx(")"),(char *)0));
		RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
	}

	if (lbelow > 0)
	{	/* lines below the footing */
		CVna(lbelow,ibuf);
		RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
	}

	RF_STAT_CHK(s_w_action(ERx("end"), ERx("rbfpbot"), (char *)0));

	return(OK);
}
