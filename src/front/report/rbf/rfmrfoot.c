/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmrfoot.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_RFOOT - set up the report footer.  This will contain only
**	the aggregates over the report.
**
**	Parameters:
**		none.
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
**		180.
**
**	History:
**		9/12/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		22-nov-89 (cmr)	removed call to rFchk_agg(), rFm_aggs() and
**				associated code; no longer needed now that
**				we have visible editable aggregates (rFm_sec()
**				does all the work now).
**		12/15/89  (martym) (garnet)
**			Added support for Labels style reports.
**		30-jan-90 (cmr) use new field sec_under in Sec_node to
**				determine underlining capability.
**	  	2/20/90 (martym)   Changed to return STATUS passed back by the 
**			     SREPORTLIB routines.
**              17-apr-90 (sylviap)
**                      Changed parameter list so can pass on report width to
**                      rFm_sec.  Needed for .PAGEWIDTH support. (#129, #588)
**		04-jun-90 (cmr)
**			Put back in support for TFORMAT; add arg to rFm_sec.
**              07-aug-90 (sylviap)
**			Now generates the END BLOCK for labels in an IF 
**			statement. (#32085)
**      	29-aug-90 (sylviap)
**              	Got rid of the parameter, rep_width. #32701.
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/

FUNC_EXTERN STATUS rFm_sec();
FUNC_EXTERN STATUS s_w_action();


STATUS rFm_rfoot()
{

	Sec_node	*n;
	STATUS 		stat = OK;


	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0))
	{
		SIprintf(ERx("rFm_rfoot: entry.\r\n"));
	}
#	endif
	if ( n = sec_list_find( SEC_REPFTR, 0, &Sections ) )
	{
		RF_STAT_CHK(s_w_action(ERx("begin"), ERx("rbfhead"), (char *)0));

		if (St_style == RS_LABELS)
		{
			/* Add code to close the block (#32085) */
			IIRFcbk_CloseBlock(FALSE);
		}

		RF_STAT_CHK(rFm_sec( n->sec_y, (n->next ? n->next->sec_y : 0), 
				n->sec_under, FALSE));
		RF_STAT_CHK(s_w_action(ERx("end"), ERx("rbfhead"), (char *)0));
	}

	return(OK);
}
