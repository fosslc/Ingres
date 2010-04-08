# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  write out an array of pointers
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)wrarr.c	30.2	2/4/85";
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	09/03/88 (dkh) - Enlarged buffers for storing the form name.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	08/17/90 (dkh) - Fixed bug 21500.
**	07/07/93 (dkh) - Fixed bug 41090.  Changed label generation scheme
**			 for table field columns to remove possibility of
**			 creating duplicate labels.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


fcwrArr(str, x, num, start, count, frm, istf)
char	*str;
char	x;
i4	num;
i4	start;
i4	count;
char	*frm;
i4	istf;
{
	register i4	i;
	char		buf[100];
	char		type[100];
	char		*fmtstr;

	if (fclang == DSL_MACRO)
	{
		STprintf(type, ERx("%s **"), str);
		STprintf(buf, ERx("_a%c%d%d"), x, num, start);
	}
	else
	{
# ifdef IBM370
		STprintf(type, ERx("%s * const"), str);
# else
		STprintf(type, ERx("%s *"), str);
# endif /* IBM370 */
		STprintf(buf, ERx("_a%c%d%d[]"), x, num, start);
	}
	DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, type);
	if (count == 0)
		DSwrite(DSD_I4, 0, 0);
	else
	{
		for (i = 0; i < count; i++)
		{
			if (istf)
			{
				fmtstr = ERx("_%c%x_%x");
			}
			else
			{
				fmtstr = ERx("_%c%d%d");
			}
			STprintf(buf, fmtstr, x, num, start++);
			DSwrite(DSD_ADDR, buf, 0);
		}
	}
	DSfinal();
}
