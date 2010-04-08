# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  wrfield.c
**  write out a fields definition
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	20-may-88 (bruceb)
**		DSwrite DSD_IADDR rather than DSD_CADDR; now casting
**		to (int *) rather than (char *).
**	09/03/88 (dkh) - Enlarged buffers for storing the form name.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	08/17/90 (dkh) - Fixed bug 21500.
**	08/03/93 (dkh) - Changed the code generated for internal use to no
**			 longer depend on frame61.h.  This makes building
**			 a release simpler.
**	22-jan-1998 (fucch01)
**		Casted 2nd arg's of DSwrite calls as type PTR (equivalent
**		to char *), to match the parameter type and get rid of
**		errors preventing compilation on sgi_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


fcwrField(fd, num, fdnum, frm)
FIELD		**fd;
register i4	num;
i4		fdnum;
char		*frm;
{
	register FIELD	*ft;
	register i4	i;
	char		buf[100];

	for (i = 0; i < fdnum; i++, num++, fd++)
	{
		ft = *fd;
		if (ft->fltag == FREGULAR)
		{
			STprintf(buf, ERx("_fC%d"), num);
			fcwrReg(ft, buf);
		}
		else
		{
			fcwrTab(ft, num, frm);
		}
		STprintf(buf, ERx("_f0%d"), num);
# ifdef IBM370
		DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("const FIELD"));
# else
		if (fcrti == TRUE)
		{
			DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL,
				ERx("IFIELD"));
		}
		else
		{
			DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL,
				ERx("FIELD"));
		}
# endif /* IBM370 */
		DSwrite(DSD_I4, (PTR)ft->fltag, 0);
		STprintf(buf, ERx("_fC%d"), num);
		DSwrite(DSD_IADDR, buf, 0);
		DSfinal();
	}
}
