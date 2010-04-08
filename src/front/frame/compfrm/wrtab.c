/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
** wrtab.c
** write out a table fields definition
**
**  History:
** static char	Sccsid[] = "@(#)wrtab.c	30.2	2/4/85";
**	03/03/87 (dkh) - Added support for ADTs.
**	05/21/87 (dkh) - Fixed to write out structs correctly.
**	06/01/87 (dkh) - Sync up with frame changes.
**	08/14/87 (dkh) - ER changes.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	04/18/90 (dkh) - Integrated IBM porting changes.
**	08/17/90 (dkh) - Fixed bug 21500.
**	07/07/93 (dkh) - Fixed bug 41090.  Changed label generation scheme
**			 for table field columns to remove possibility of
**			 creating duplicate labels.
**			 Also removed useless ifdefs.
**	22-jan-1998 (fucch01)
**	    Casted 2nd arg of DSwrite calls as type PTR to get rid of sgi_us5
**	    compiler warnings...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* VARARGS2 */
fcwrTab(fd, num, frm)
FIELD		*fd;
i4		num;
register char	*frm;
{
	register TBLFLD	*tab;
	char		buf[100];

	tab = fd->fld_var.fltblfld;
	fcwrCol(tab->tfflds, num, tab->tfcols, frm);
	fcwrArr(ERx("FLDCOL"), 'c', num, (i4) 0, tab->tfcols, frm, (i4) TRUE);
	STprintf(buf, ERx("_fC%d"), num);
	DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("TBLFLD"));
	fcwrHdr(&tab->tfhdr);
	DSwrite(DSD_I4, 0, 0);			/* tfscrup */
	DSwrite(DSD_I4, 0, 0);			/* tfscrdown */
	DSwrite(DSD_I4, 0, 0);			/* tfdelete */
	DSwrite(DSD_I4, 0, 0);			/* tfinsert */
	DSwrite(DSD_I4, 0, 0);			/* tfstate */
	DSwrite(DSD_I4, 0, 0);			/* tfputrow */
	DSwrite(DSD_I4, (PTR)tab->tfrows, 0);
	DSwrite(DSD_I4, 0, 0);			/* tfcurrow */
	DSwrite(DSD_I4, (PTR)tab->tfcols, 0);
	DSwrite(DSD_I4, 0, 0);			/* tfcurcol */
	DSwrite(DSD_I4, (PTR)tab->tfstart, 0);
	DSwrite(DSD_I4, (PTR)tab->tfwidth, 0);
	DSwrite(DSD_I4, 0, 0);			/* tflastrow */
	STprintf(buf, ERx("_ac%d0"), num);
	DSwrite(DSD_ARYADR, buf, 0);		/* tfflds */
	DSwrite(DSD_I4, 0, 0);			/* tfwins */
	DSwrite(DSD_I4, 0, 0);			/* tf1pad */
	DSwrite(DSD_I4, 0, 0);			/* tf2pad */
	DSwrite(DSD_I4, 0, 0);			/* tffflags */
	DSwrite(DSD_I4, 0, 0);			/* tfnscr */
	DSwrite(DSD_I4, 0, 0);			/* tffuture */
	DSwrite(DSD_I4, 0, 0);			/* tf2fu */
	DSfinal();
}
