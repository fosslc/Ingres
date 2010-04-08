/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	static	char	*Sccsid = "@(#)forcebrws.c	1.2	1/31/85";
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

/*
** FORCEBRWS.C
**
**	Returns TRUE if the current field of the frame is a table
**	field which is display only, or the current column in the table field
**	is display only.
**	The reason you need forcebrowse for table fields is that they are not
**	put on the frnsfld[] array as other table fields are.  Their mode is
**	set at runtime.  The columns' modes are set in Vifred, but cannot be
**	on a separate list than the parent table field. These both now use
**	the masking stuff in fhdflags.
**	
**	History:
**		27-Jul-1983	Rewritten (ncg)
**		22-mar-1984	True for Query only fields too (ncg)
**		19-dec-88 (bruceb)
**			Handle readonly fields and columns.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototype.
*/

i4		FTcurdisp();

STATUS
FTforcebrowse(frm, curfld)
	FRAME	*frm;
	i4	curfld;
{
	FIELD	*fld;
	TBLFLD 	*tf;
	REGFLD 	*rf;
	FLDCOL	*col;

	fld = frm->frfld[curfld];
	if (fld->fltag == FREGULAR)
	{
		rf = fld->fld_var.flregfld; 
		/* Not Query mode and field is queryonly or readonly field */
		return((rf->flhdr.fhd2flags & fdREADONLY)
		    || ((FTcurdisp() != fdmdQRY) && (FTcurdisp() != fdmdADD)
			&& (rf->flhdr.fhdflags & fdQUERYONLY)));
	}
	tf = fld->fld_var.fltblfld;

	/* table field is readonly */
	if (tf->tfhdr.fhdflags & fdtfREADONLY)
		return (TRUE);

	/* in middle of an FDmove() while in table field */
	if (tf->tfcurcol < 0 || tf->tfcurcol >= tf->tfcols)
		return (FALSE);

	col = tf->tfflds[tf->tfcurcol];
	/*
	** Column may be displayonly or readonly, or table field is not
	** Query and the column is queryonly.
	*/
	return((col->flhdr.fhd2flags & fdREADONLY)
	    || (col->flhdr.fhdflags & fdtfCOLREAD)
	    || (!(tf->tfhdr.fhdflags & fdtfQUERY)
		&& !(tf->tfhdr.fhdflags & fdtfAPPEND)
		&& (col->flhdr.fhdflags & fdQUERYONLY)));
}
