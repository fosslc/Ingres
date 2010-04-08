/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfnamfld.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFNAMFLD - make a field name, given prefix and ordinal.
**	For use in the Structure form.
**
**	Parameters:
**		prefix	string of prefix.
**		ord	number of att ordinal.
**
**	Returns:
**		ptr to static buffer containing field name.
**
**	Called by:
**		rFmstruct, rFstruct, rFm_sort.
**
**	Side Effects:
**		none.
**
**	History:
**		8/29/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char	*
rFnamfld(prefix,ord)
char	*prefix;
i4	ord;
{
	/* internal declarations */

	static	char	tbuf[100];
	char		ibuf[10];

	/* start of routine */

	STcopy(prefix, tbuf);
	CVna(ord,ibuf);
	STcat(tbuf,ibuf);

	return(tbuf);
}
