# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  wrcol.c
**  write out the Columns for a table field
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)wrcol.c	30.2	2/4/85";
**
**  History:
**	05/01/87 (dkh) - Deleted call to fcwrVtree. (dkh)
**	05/21/87 (dkh) - Fixed to write out structs correctly.
**	08/14/87 (dkh) - ER changes.
**	11/12/87 (dkh) - Code cleanup.
**	09/03/88 (dkh) - Enlarged buffers for storing the form name.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	08/17/90 (dkh) - Fixed bug 21500.
**	07/07/93 (dkh) - Fixed bug 41090.  Changed label generation scheme
**			 for table field columns to remove possibility of
**			 creating duplicate labels.
**			 Also removed useless ifdefs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


fcwrCol(col, num, conum, frm)
register FLDCOL	**col;
register i4	num;
register i4	conum;
char		*frm;
{
	register i4	i;
	register FLDCOL	*co;
	char		buf[100];
	FLDTYPE		*type;

	for (i = 0; i < conum; i++, col++)
	{
		co = *col;

		/*
		**  To remove possibility of creating duplicate
		**  labels for table field columns, add an underscore
		**  between the field and column numbers.  Also print
		**  the numbers in hex to compensate for the lost of
		**  one position (due to the new underscore) in the label.
		*/
		STprintf(buf, ERx("_c%x_%x"), num, i);
		type = &co->fltype;
		DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("FLDCOL"));
		fcwrHdr(&co->flhdr);
		fcwrType(type);
		DSfinal();
	}
}
