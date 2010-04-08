/*
**  invistf.c
**
**  Routines to support invisible table fields.
**  These routines are only called if the table field
**  is in the form that is currently displayed on
**  the terminal screen.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 08/21/85 (dkh)
**	85/08/21  20:27:49  dave
**		Initial revision
**	85/08/22  12:38:34  dave
**		Changed third arg to FTtblbld from TRUE to FALSE. (dkh)
**	85/08/23  13:06:12  dave
**		Initial version in 4.0 rplus path. (dkh)
**	85/09/18  17:15:21  john
**		Changed i4=>nat, CVla=>CVna, CVal=>CVan, unsigned char=>u_char
**	86/11/18  22:00:21  dave
**		Initial60 edit of files
**	86/11/21  07:47:47  joe
**		Lowercased FT*.h
**	87/04/08  00:01:59  joe
**		Added compat, dbms and fe.h
**	06/19/87 (dkh) - Code cleanup.
**	05-jan-89 (bruceb)
**		Additional parameter in call on FTtblbld to
**		indicate that the original caller wasn't Printform.
**	19-jan-89 (bruceb)
**		Cleaned up the history, extern -> GLOBALREF, changed comment.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change 6202p/131520.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

GLOBALREF	bool	samefrm;

/*
**  Make table field identified by "fldno" in frame "frm"
**  invisible.
**
**  Note that there is a flaw here.  If the cursor is on a table
**  field and it is made invisible the cursor will land on the
**  now invisible table field when the forms system regains
**  control.  If one wants to make a table field invisible,
**  PLEASE do a resume to a different field afterwards to
**  avoid having the cursor land on the now invisible table
**  field.
*/

FTtfinvis(frm, fldno, fldtype)
FRAME	*frm;
i4	fldno;
i4	fldtype;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	WINDOW	*win;

	if (fldtype == FT_UPDATE)
	{
		fld = frm->frfld[fldno];
	}
	else
	{
		fld = frm->frnsfld[fldno];
	}

	tbl = fld->fld_var.fltblfld;
	hdr = &(tbl->tfhdr);
	win = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx,
	    hdr->fhposy, hdr->fhposx, FTutilwin);
	TDerase(win);
	TDrefresh(FTwin);

# ifdef DATAVIEW
	_VOID_ IIMWtvTblVis(frm, fldno, fldtype, FALSE);
# endif	/* DATAVIEW */
}



/*
**  Make table field identified by "fldno" in frame "frm"
**  visible.
*/

FTtfvis(frm, fldno, fldtype)
FRAME	*frm;
i4	fldno;
i4	fldtype;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	bool	ssamefrm;

	if (fldtype == FT_UPDATE)
	{
		fld = frm->frfld[fldno];
	}
	else
	{
		fld = frm->frnsfld[fldno];
	}

	tbl = fld->fld_var.fltblfld;

	/*
	** Hack to get the table field to reappear on the terminal.
	*/

	ssamefrm = samefrm;
	samefrm = FALSE;

# ifdef DATAVIEW
	_VOID_ IIMWtvTblVis(frm, fldno, fldtype, TRUE);
# endif	/* DATAVIEW */

	FTtblbld(frm, tbl, FALSE, FTwin, (bool)FALSE);

	/*
	**  Don't need to reset the _BOXFIX flag for the
	**  window since that will be taken care of by
	**  the TDrefresh call below.
	*/

	samefrm = ssamefrm;

	TDtouchwin(FTwin);
	TDrefresh(FTwin);
}
