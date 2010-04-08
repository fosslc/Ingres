# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  wrtrim.c
**  write out the trim for a frame
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  static	char	Sccsid[] = "@(#)wrtrim.c	30.2	2/4/85";
**	03/03/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	09/03/88 (dkh) - Enlarged buffers for storing the form name.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	08/17/90 (dkh) - Fixed bug 21500.
**	22-jan-1998 (fucch01)
**	    Added casts for DSwrite argument so it matches param
**	    type, gets rid of sgi_us5 compile errors.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


fcwrTrim(trm, num, trnum, frm)
register TRIM	**trm;
register i4	num,
		trnum;
char		*frm;
{
	register i4	i;
	register TRIM	*tr;
	char		buf[100];

	for (i = 0; i < trnum; i++, trm++, num++)
	{
		tr = *trm;
		STprintf(buf, ERx("_t0%d"), num);
# ifdef IBM370
		DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("const TRIM"));
# else
		DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("TRIM"));
# endif /* IBM370 */
		DSwrite(DSD_I4, (PTR)tr->trmx, 0);
		DSwrite(DSD_I4, (PTR)tr->trmy, 0);
		DSwrite(DSD_I4, (PTR)tr->trmflags, 0);
		DSwrite(DSD_I4, (PTR)tr->trm2flags, 0);
		DSwrite(DSD_I4, (PTR)tr->trmfont, 0);
		DSwrite(DSD_I4, (PTR)tr->trmptsz, 0);
		DSwrite(DSD_SPTR, tr->trmstr, 0);
		DSfinal(); 
	}
}
