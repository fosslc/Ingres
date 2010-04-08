/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmfoot.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_FOOT - print out the .FOOTER block for a break attribute.
**	This will figure out the appropriate aggregates to print
**	at the bottom of the block.
**
**	Parameters:
**		ord	ordinal of break attribute.
**
**	Returns:
**		none.
**
**	Called by:
**		rFsave.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		Syserr: Bad attribute ordinal.
**
**	Trace Flags:
**		200.
**
**	History:
**		9/12/82 (ps)	written.
**		8/24/83 (gac)	bug 1496 fixed -- now 1st page is not blank 
**				if newpage option is chosen.
**		7/24/84 (gac)	bug 3743 fixed -- column options are now saved
**				if aggregates are not specified.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		01-sep-89 (cmr)	surround empty ftr with begin/end so that 
**				we can write/detect the ftr.
**		16-oct-89 (cmr) remove refs to copt_skip; Sections list
**				controls creation of break footers.
**		22-nov-22 (cmr)	removed call to rFchk_agg(), rFm_aggs() and
**				associated code; no longer needed now that
**				we have visible editable aggregates (rFm_sec()
**				does all the work now).
**		30-jan-90 (cmr) use new field sec_under in Sec_node to
**				determine underlining capability.
**	  	2/20/90 (martym)   Changed to return STATUS passed back by the 
**			     SREPORTLIB routines.
**              17-apr-90 (sylviap)
**                      Changed parameter list so can pass on report width to
**                      rFm_sec.  Needed for .PAGEWIDTH support. (#129, #588)
**		04-jun-90 (cmr) Put back in support for newpage on break.
**	06-aug-90 (sylviap)
**		For label reports, close the block at the beginning of the
**		break footer. (#32085)
**      29-aug-90 (sylviap)
**              Got rid of the parameter, rep_width. #32701.
**	2-nov-1992 (rdrane)
**		Remove all FUNCT_EXTERN's since they're already declared
**		in included hdr files.  Ensure that we ignore unsupported
**		datatypes (of which there shouldn't be any; it actually was,
**		didn't cast NULL to (ATT *)).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/



STATUS rFm_foot(ord)
i4	ord;
{
	/* internal declarations */

	register COPT	*copt;
	register ATT	*att;
	Sec_node	*n;
	char		ibuf[MAXRNAME+1];
	STATUS		stat = OK;

	/* start of routine */

	copt = rFgt_copt(ord);
	att = r_gt_att(ord);
	if ((copt == NULL) || (att == (ATT *)NULL))
	{
	    i4	tempord;

	    tempord = ord;
	    IIUGerr(E_RF0034_rFm_foot__Bad_attribu, UG_ERR_FATAL, 1, &tempord);
	}

	if ( n = sec_list_find( SEC_BRKFTR, copt->copt_sequence, &Sections ) )
	{
		RF_STAT_CHK(s_w_action(ERx("begin"), ERx("rbfhead"), (char *)0));

		/* For label reports, close the detail block  (#32085) */
	   	if (St_style == RS_LABELS)
		{
			IIRFcbk_CloseBlock(TRUE);
		}

		RF_STAT_CHK(rFm_sec( n->sec_y, (n->next ? n->next->sec_y : 0), 
			n->sec_under, FALSE));
		if (copt->copt_newpage)
		{
			/* skip to a new page */
			RF_STAT_CHK(s_w_action(ERx("newpage"),(char *)0));
		}
		RF_STAT_CHK(s_w_action(ERx("end"), ERx("rbfhead"), (char *)0));
	}


	return(OK);
}
