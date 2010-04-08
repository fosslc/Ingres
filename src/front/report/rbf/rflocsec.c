/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rflocsec.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 "rbftype.h"
# include	 "rbfglob.h"

/*
**   RFLOC_SEC - find which section a Y location is in.
**
**	Parameters:
**		y	coordinate to check.
**
**	Returns:
**		section either SEC_DETAIL, SEC_TITLE, SEC_HEADING
**			or 0 if none.
**
**	Called by:
**		numerous routines.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		180, 182.
**
**	History:
**		4/4/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		01-sep-89 (cmr)
**			revamp to take into consideration multiple
**			headers and footers (report/page/break).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
rFloc_sec(y)
i4	y;
{
	/* internal declarations */

	i4	sect = 0;		/* return value */
	Sec_node	*q, *p;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0))
	{
		SIprintf(ERx("rFloc_sec: entry.\r\n"));
	}
#	endif

#	ifdef	xRTR3
	if (TRgettrace(182,0))
	{
		SIprintf(ERx("	    y on entry: %d\r\n"),y);
	}
#	endif

	p = Sections.head;
	while ( p )
	{
		q = p;
		p = p->next;
		if ( p && y >= q->sec_y && y < p->sec_y )
			sect = q->sec_type;
	}

#	ifdef	xRTR3
	if (TRgettrace(182,0))
	{
		SIprintf(ERx("	    Exit from rFloc_sec. Section:%d\r\n"),sect);
	}
#	endif

	return(sect);
}
