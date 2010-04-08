/*
**  FTinsrtst.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  Note:
**	This routine is only called in query mode, as part of performing
**	a putoper, to place the query operator back on the screen in
**	front of the re-formatted value.  Since both the existing
**	contents and the operator were originally retrieved from the
**	field, where they fit, there is no chance of their now being too
**	long.
**
**  History:
**	24-apr-87 (bab)
**		Direction of placement of string in field when on screen
**		is now determined by whether field is left-to-right,
**		or reversed.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06-may-87 (bab)
**		Call FTswapparens to reverse (),[], {} and <> in RL fields.
**		Called on a copied string since some architectures would
**		probably complain about attempts to modify constant
**		strings--which is what 'string' points to.
**		Note: string is currently only one of the query operators.
**	06/19/87 (dkh) - Code cleanup.
**	09-nov-87 (bab)
**		Properly handle updating the underlying buffers (as well
**		as the screen) when dealing with reverse and/or scrolling
**		fields.
**	09-feb-88 (bruceb)
**		Moved fdSCRLFD to fhd2flags; can't use most significant
**		bit of fhdflags.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<cm.h>
# include	<scroll.h>

FUNC_EXTERN	WINDOW	*FTfindwin();
FUNC_EXTERN	VOID	FTswapparens();


FTinsrtstr(frm, fieldno, disponly, fldtype, col, row, string)
FRAME	*frm;
i4	fieldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
char	*string;
{
	DB_TEXT_STRING	*text;
	DB_DATA_VALUE	sdbv;
	FIELD		*fld;
	TBLFLD		*tbl;
	FLDVAL		*val;
	FLDHDR		*hdr;
	i4		ocol;
	i4		orow;
	i4		stlen;
	i4		curcount;
	i4		ncount;
	u_char		*cp;
	u_char		*src;
	u_char		*mvdest;
	u_char		*mvsrc;
	WINDOW		*win;
	char		tmpstr[100];

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
	}

	val = (*FTgetval)(fld);
	hdr = (*FTgethdr)(fld);
	stlen = STlength(string);
	src = val->fvbufr;
	text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
	curcount = text->db_t_count;

	if (hdr->fhdflags & fdREVDIR)
	{
		/*
		** The contents of reversed fields may be stored
		** 'short' due to having done a trim-white of the
		** end of the string (the beginning of the reversed
		** contents).  Thus, need to shift complete width
		** of the buffer.
		*/
		_VOID_ adc_lenchk(FEadfcb(), FALSE, val->fvdsdbv,
			&sdbv);
		ncount = sdbv.db_length;
		mvdest = src;
		mvsrc = mvdest + stlen;
		src += ncount - 1;

		/* Shift the characters over. */
		while (mvsrc <= src)
			*mvdest++ = *mvsrc++;

		STcopy(string, tmpstr);
		FTswapparens(tmpstr);

		/* Drop new characters in. */
		for (cp = (u_char *)tmpstr; *cp; *src-- = *cp++)
			;
	}
	else	/* LR field */
	{
		ncount = stlen + curcount;
		mvdest = src + ncount - 1;
		mvsrc = mvdest - stlen;

		/* Shift the characters over. */
		while (mvsrc >= src)
			*mvdest-- = *mvsrc--;

		/* Drop new characters in. */
		for (cp = (u_char *)string; *cp; *src++ = *cp++)
			;
	}
	text->db_t_count = ncount;

	if (hdr->fhd2flags & fdSCRLFD)
	{
		/*
		** Clear end of buffer in event that count
		** is less than complete length; also used to
		** reset scr_start and cur_pos.
		*/
		IIFTcsfClrScrollFld((SCROLL_DATA *)val->fvdatawin,
			(bool)(hdr->fhdflags & fdREVDIR ? TRUE : FALSE));
		if (ncount != 0)
		{
			MEcopy((PTR) val->fvbufr, (u_i2) ncount,
				(PTR)(((SCROLL_DATA *)val->fvdatawin)->left));
		}
	}

	/*
	**  Update window structure if necessary.
	*/
	if (frm == FTiofrm)
	{
		win = FTfindwin(fld, FTwin, FTutilwin);

		if (hdr->fhdflags & fdREVDIR)
		{
			TDmove(win, (i4) 0, win->_maxx - 1);
			for (cp = (u_char *)tmpstr; *cp; CMnext(cp))
			{
				TDrinsstr(win, cp);
			}
		}
		else
		{
			TDmove(win, (i4) 0, (i4) 0);
			for (cp = (u_char *)string; *cp; CMnext(cp))
			{
				TDinsstr(win, cp);
			}
		}
	}


	if (fld->fltag == FTABLE)
	{
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}

# ifdef DATAVIEW
	_VOID_ IIMWufUpdFld(frm, fieldno, disponly, col, row);
# endif	/* DATAVIEW */
}
