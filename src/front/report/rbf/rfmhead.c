/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmhead.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_HEAD - print out the .HEADER block for a break attribute.
**	Currently, this will only contain a possible .NEWLINE or .NEED
**	command, and .NEWPAGE command.
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
**		8/24/83 (gac)	bug 1496 fixed -- now 1st page is not 
**				blank if newpage option is chosen.
**		1/29/87 (rld)	replaced call to f_fmtstr with reference 
**				to ATT.fmtstr
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		01-sep-89 (cmr)	surround an empty hdr with begin/end so that
**				we can write/detect it.
**		16-oct-89 (cmr)	remove refs to copt_whenprint; Sections list
**				controls break header creation.
**		22-nov-89 (cmr)	sec_list* routines now take another argument.
**		30-jan-90 (cmr) use new field sec_under in Sec_node to
**				determine underlining capability.
**	  	2/20/90 (martym)   Changed to return STATUS passed back by the 
**			     SREPORTLIB routines.
**		17-apr-90 (sylviap) 
**			Changed parameter list so can pass on report width to
**			rFm_sec.  Needed for .PAGEWIDTH support. (#129, #588)
**		04-jun-90 (cmr)
**			Put back in support for TFORMAT.  
**    	06-aug-90 (sylviap)
**            	For label reports, close the block at the beginning of the
**            	break header. (#32085)
**      29-aug-90 (sylviap)
**              Got rid of the parameter, rep_width. #32701.
**      2-nov-1992 (rdrane)
**		Remove all FUNCT_EXTERN's since they're already
**		declared in included hdr files.  Ensure that we
**		ignore unsupported datatypes (of which there
**		shouldn't be any).  Ensure attribute names
**		passed to s_w_action() in unnormalized form.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/



STATUS rFm_head(ord)
i4	ord;
{
	/* internal declarations */

	register COPT	*copt;
	register ATT	*att;
	Sec_node	*n;
	char		fmtstr[MAX_FMTSTR];
	char		tmp_buf[(FE_UNRML_MAXNAME + 1)];
	STATUS 		stat = OK;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0))
	{
		SIprintf(ERx("rFm_head: entry.\r\n"));
	}
#	endif

	copt = rFgt_copt(ord);
	if (copt == NULL)
	{
	    i4	tempord;

	    tempord = ord;
	    IIUGerr(E_RF0035_rFm_head__Bad_attribu, UG_ERR_FATAL, 1, &tempord);
	}

	if ( n = sec_list_find( SEC_BRKHDR, copt->copt_sequence, &Sections ) )
	{
		/*
		** We need to write out RBFHEAD because R/W may tack on
		** extra TCMDs for agg processing (A_OP) even if the user
		** did not explicitly create a header.
		*/
		RF_STAT_CHK(s_w_action(ERx("begin"), ERx("rbfhead"), (char *)0));

		/* For label reports, close the detail block (#32085) */
		if (St_style == RS_LABELS)
		{
			IIRFcbk_CloseBlock(TRUE);
		}

		RF_STAT_CHK(rFm_sec( n->sec_y, (n->next ? n->next->sec_y : 0), 
			n->sec_under, FALSE));

		if ((copt->copt_whenprint=='b') || (copt->copt_whenprint=='t'))
		{
			att = r_gt_att(ord);
			/*
			** Test is fix for bug #1020.  We don't want
			** to output anything if field has been deleted.
			*/
			if  ((att != (ATT *)NULL) && (att->att_format != NULL))
			{
				if (fmt_fmtstr(FEadfcb(), att->att_format, 
					fmtstr) != OK)
				{
			      		IIUGerr(E_RF0036_rFm_head_Can_t_get_fo,
				    		UG_ERR_FATAL, 0);
				}
				_VOID_ IIUGxri_id(att->att_name,&tmp_buf[0]);
				RF_STAT_CHK(s_w_action(ERx("tformat"),
					&tmp_buf[0], ERx("("), 
					fmtstr,ERx(")"),(char *)0));
			}
		}
		RF_STAT_CHK(s_w_action(ERx("end"), ERx("rbfhead"), (char *)0));
	}
	return(OK);
}
