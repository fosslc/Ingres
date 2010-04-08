/*
**	ftfldupd.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ftfldupd.c - Update field to terminal display.
**
** Description:
**	This file contains the routine to update the terminal
**	display with whatever is in the display buffer.
**	Defines routine:
**	- FTfldupd - Update field display buffer to terminal.
**
** History:
**	03/03/87 (dkh) - Initial version.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	10/23/88 (dkh) - Performance changes.
**	12/05/88 (dkh) - More performance changes.
**	01/19/89 (dkh) - Fixed stale pointer problem for popups.
**	04/26/89 (dkh) - Fixed to get new win value if win is NULL.
**	06/17/89 (dkh) - Fixed typo for 04/26/89 change.  "prwin" can
**			 be NULL if updating to a field that is not
**			 on the display list.
**	12/30/89 (dkh) - Fixed up call to IIFTwffWindowForForm() since
**			 its interface has changed.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<scroll.h>
# include	"ftframe.h"

FUNC_EXTERN	WINDOW	*IIFTwffWindowForForm();

static	FRAME	*prfrm = NULL;
static	WINDOW	*prwin = NULL;


/*{
** Name:	FTfldupd - Update field display buffer to terminal.
**
** Description:
**	Take whatever is in the display buffer for the specified
**	field and update the information to the terminal screen
**	and any other necessary data structures.
**
** Inputs:
**	frm		Pointer to form containing field for update.
**	fieldno		Field number of field for update.
**	disponly	Indicates whether field is updateable or display only.
**	fldtype		Indicator for whether field is a regular or table field.
**	col		Column number if field is a table field.
**	row		Row number if field is a table field.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	03/03/87 (dkh) - Initial version.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	21-apr-88 (bruceb)
**		Call IIFTcsfClrScrollFld to zero out the scroll buffer
**		even when updating field not on currently displayed form.
*/
VOID
FTfldupd(frm, fieldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fieldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	FLDHDR	*usehdr = NULL;
	FLDTYPE	*type;
	FLDVAL	*val;
	i4	ocol;
	i4	orow;
	bool	reverse;
	WINDOW	*win;
	i4	i;
	i4	startrow;
	i4	lines;
	i4	offset = 0;

	if (disponly == FT_UPDATE)
	{
		fld = frm->frfld[fieldno];
	}
	else
	{
		fld = frm->frnsfld[fieldno];
	}

	if (fld->fltag == FTABLE)
	{
		tbl = fld->fld_var.fltblfld;
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = col;
		tbl->tfcurrow = row;
		FTtblhdr(&(tbl->tfhdr));
		usehdr = &(tbl->tfhdr);
	}

	hdr = (*FTgethdr)(fld);
	type = (*FTgettype)(fld);
	val = (*FTgetval)(fld);

	if (usehdr == NULL)
	{
		usehdr = hdr;
	}

	if (hdr->fhd2flags & fdSCRLFD)
	{
		reverse = (bool)(hdr->fhdflags & fdREVDIR ? TRUE : FALSE);
		IIFTcsfClrScrollFld(IIFTgssGetScrollStruct(fld), reverse);
	}

	FTstr_fld(hdr, type, val, frm);

	if (frm != prfrm || prwin == NULL)
	{
		prwin = win = IIFTwffWindowForForm(frm, FALSE);
		prfrm = frm;
	}
	else
	{
		win = prwin;
	}
	if (win != NULL)
	{
		if (win->_parent)
		{
			win = win->_parent;
			offset = 1;
		}
		startrow = usehdr->fhposy + val->fvdatay + offset;
		if (type->ftwidth == type->ftdataln)
		{
			lines = 1;
			win->_firstch[startrow] = 0;
			win->_lastch[startrow] = win->_maxx - 1;
		}
		else
		{
			lines = (type->ftwidth+type->ftdataln-1)/type->ftdataln;
			for (i = 0; i < lines; i++)
			{
				win->_firstch[startrow] = 0;
				win->_lastch[startrow++] = win->_maxx - 1;
			}
		}
	}

	if (fld->fltag == FTABLE)
	{
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
		FTtblhdr(NULL);
	}

# ifdef DATAVIEW
	_VOID_ IIMWufUpdFld(frm, fieldno, disponly, col, row);
# endif	/* DATAVIEW */
}


/*{
** Name:	IIFTrfuoResetFldUpdOpt - Reset Field Update Optimization Info.
**
** Description:
**	Reset the field update optimization that is stored in the
**	variables "prfrm" and "prwin".  This prevents stale pointers
**	from being used when they shouldn't.
**
** Inputs:
**	win		Window (for a popup) that is being popped off
**			the display list.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01/16/89 (dkh) - Initial version.
*/
VOID
IIFTrfuoResetFldUpdOpt(win)
WINDOW	*win;
{
	/*
	**  If "prwin" is the same as the window (for a popup)
	**  that is being popped by IIFTpdlPopDispList() in
	**  ftsyndsp.c then reset prfrm to NULL.  This prevents
	**  bad pointer information (in prwin) from being used.
	*/
	if (win == prwin)
	{
		prfrm = NULL;
	}
}
