/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmsort.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_SORT - set up the SORT records for the RCOMMANDS relation.
**	This uses the information in the COPT structure to 
**	set up the records.  
**
**	This routine first scans the copt_sequence fields to assure that
**	they are in ascending order without skips (i.e. 1,2...).
**
**	Note that this routine assumes that the COPT structures are
**	up to date with the form.
**
**	Parameters:
**		none.
**
**	Returns:
**		STATUS. OK   -- If successful.
**			FAIL -- If not successful.
**			(status passed back by SREPORTLIB)
**
**	Called by:
**		rFsave.
**
**	Side Effects:
**		May change copt_sequence values.  Also may change
**		the att->att_brk_ordinal field  for attributes.
**
**	Trace Flags:
**		180, 188.
**
**	History:
**		8/30/82 (ps)	written.
**		9/22/89 (elein) UG changes ingresug change #90045
**			changed <rbftype.h> & <rbfglob.h> to "rbftype.h" &
**			"rbfglob.h"
**		2/20/90 (martym)
**			Changed to return STATUS passed back by the 
**			SREPORTLIB routines.
**		30-oct-1992 (rdrane)
**			Remove all FUNCT_EXTERN's since they're already
**			declared in included hdr files.  Ditto declaration
**			of r_gt_agg().  Ensure that we ignore unsupported
**			datatypes (of which there shouldn't be any).  If
**			any are found, return FAIL.
**		27-jan-1993 (rdrane)
**			Generate the sort attribute name as unnormalized.
**		9-dec-1993 (rdrane)
**			Oops!  Don't generate the sort attribute name as
**			unnormalized (b54950).
*/


STATUS rFm_sort()
{
	/* internal declarations */

	register i2	i;		/* fast counters */
	register COPT	*copt;		/* ptr to COPT for this att */
	register ATT	*att;		/* fast ptr to att */
	i2		minord;		/* attord of current minimum */
	i2		nextseq;	/* next sort sequence */
	i2		minseq;		/* temp minimum sequence number */
	STATUS		stat = OK;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0) || TRgettrace(188,0))
	{
		SIprintf(ERx("rFm_sort: entry.\r\n"));
	}
#	endif

	save_em();			/* save current environment */

	/* set up the SREPORT fields for the sort variables */

	STcopy(NAM_SORT, Ctype);
	Csequence = 0;
	STcopy(ERx(" "), Csection);

	/* go through the list of attributes to find the sort attributes
	** in order */

	for (nextseq=1;;nextseq++)
	{
		minord = 0;
		minseq = 0;
		for (i=1; i<=En_n_attribs; i++)
		{	/* find the minimum sort sequence */
			copt = rFgt_copt(i);
			if ((copt->copt_sequence>=nextseq) 
				&& ((minord==0)||(copt->copt_sequence<minseq)))
			{	/* new minimum sort sequence */
				minseq = copt->copt_sequence;
				minord = i;
			}
		}
		/* minord will be the ordinal of the next sort att */
		if (minord == 0)
		{	/* no more sort attributes */
			break;
		}
		/* Next one will have real sequence of NEXTSEQ */
		copt = rFgt_copt(minord);
		copt->copt_sequence = nextseq;
		if  ((att = r_gt_att(minord)) == (ATT *)NULL)
		{
			stat = FAIL;
			break;
		}
		s_rso_add(att->att_name,TRUE);
		STcopy(att->att_name,Cattid);
		att->att_brk_ordinal = copt->copt_sequence;
		STprintf(Ctext,ERx("%c"),copt->copt_order);
		STprintf(Ccommand,ERx("%c"),copt->copt_break);
		RF_STAT_CHK(s_w_row());
	}

	get_em();		/* retrieve old values of Cattid,... */

	return(stat);
}
