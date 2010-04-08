/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfcsreset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFCSRESET - reorder the CS structures, as returned from VFRBF,
**	into a new CS array which is En_n_attribs i4  because
**	the ordering of the attributes will be random
**	on return from the form, and in fact, some columns may be
**	deleted from the report, so there may be fewer than
**	En_n_attribs in the form.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		rFdisplay.
**
**	Side Effects:
**		Change values in the ATT structures.
**		Resets the value of Cs_top.
**		Sets St_right_margin.
**
**	Trace Flags:
**		180, 181.
**
**	Error Messages:
**		Syserr: CS structure not associated with column.
**
**	History:
**		8/29/82 (ps)	written.
**		25-jan-1989 (danielt)
**			Changed to use MEreqmem()
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



rFcsreset()
{
	/* internal declarations */

	CS		*cs;		/* ptr to CS for att */
	register LIST	*list;		/* fast ptr to list element */
	register i4	i;		/* counter */
	FIELD		*f;		/* pointer to field */
	CS		*ntop;		/* new Cs_top value */
	CS		*ncs;		/* new CS ptr */
	ATTRIB		ordinal;	/* ordinal of this attribute */
	bool		associated;	/* associated to column */

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0) || TRgettrace(181,0))
	{
		SIprintf(ERx("rFcsreset: entry.\r\n"));
	}
#	endif

	ntop = (CS *) MEreqmem(0,(u_i4) En_n_attribs*sizeof(CS),
						TRUE,(STATUS *) NULL);

	/* first reorder the attributes into the ntop arrary */

	for (i=1; i<=Cs_length; i++)
	{
		cs = rFgt_cs(i);

#		ifdef	xRTR1
		if (TRgettrace(181,0))
		{
			SIprintf(ERx("	Next CS ord from VIFRED: %d\r\n"),i);
			rFpr_cs(cs);
		}
#		endif

		associated = FALSE;
		for (list=cs->cs_flist; list!=NULL; list=list->lt_next)
		{	/* identify the attribute */
			if ((f=list->lt_field) == NULL)
			{
				continue;
			}
			ordinal = r_mtch_att(f->fldname);
			if ((ordinal>0) && (ordinal<=En_n_attribs))
			{	/* found the attribute.	 Store list in ntop */

#				ifdef	xRTR3
				if (TRgettrace(181,0))
				{
					SIprintf(ERx("		Match on ordinal:%d: %s\r\n"),
						ordinal, f->fldname);
				}
#				endif

				ncs = &(ntop[ordinal-1]);
				ncs->cs_flist = cs->cs_flist;
				ncs->cs_tlist = cs->cs_tlist;
				associated = TRUE;
				break;
			}
		}
		if (!associated)
		    /* not associated.  ERROR */
		    IIUGerr(E_RF0025_rFcsreset___CS_struct, UG_ERR_FATAL, 0);
	}

	Cs_top = ntop;

#	ifdef	xRTR3
	if (TRgettrace(181,0))
	{
		SIprintf(ERx("	At end of rFcsreset. CS_TOP:%p.\r\n"),Cs_top);
	}
#	endif

	return;
}
