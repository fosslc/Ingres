/*
** Copyright (c) 2004 Ingres Corporation
**
** frame.c
**
** Routines that display the frame structures
**
** DEFINES
**	vfFlddisp(fd)
**		FIELD	*fd
**
**	vfdispReg(reg)
**		REGFLD	*reg
**
**	vfTabdisp(tab, hilight)
**		TBLFLD	*tab
**		bool	flags
**
** HISTORY
**	written 3/31/82 (jrc)
**	1/28/85 (peterk) split out vfgetDataWin() into vfgetdata.c,
**		and vffrmcount() into vffrmcount.c
**	4/30/85 (gac)	Blink selected field instead of beeping with dollars.
**	03/13/87 (dkh) - Added support for ADTs.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	01-jun-89 (bruceb)
**		Use a smaller default buffer in vfFmtdisp().  Decreased from
**		4096 to 512.  (Arbitrary size.)
**	22-jun-89 (bruceb)
**		Fixed param declarations for vfClearFld(); missing a param.
**	19-sep-89 (bruceb)
**		Call vfgetSize() with precision argument.  For decimal support.
**	9/21/89 (elein) - (PC integration) Added declaration of
**			 frm to vfClearFld(frm, hdr)
**      04/10/90 (esd) - Added extra parm to calls to VTdispbox
**                       in vfFldBox and vfBoxDisp - see their histories
**                       for details.
**      06/12/90 (esd) - Add extra parm to VTputstring to indicate
**                       whether form reserves space for attribute bytes
**      06/22/90 (esd) - Don't display a null title (it could cause
**                       unwanted attribute bytes on either side).
**	06/26/92 (dkh) - Added support for rulers.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**
** Copyright (c) 2010 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<me.h>
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"

FUNC_EXTERN	STATUS	fmt_dispinfo();
FUNC_EXTERN	ADF_CB	*FEadfcb();


/*
** display the passed field
*/
VOID
vfFlddisp(fd)
FIELD	*fd;
{
	if (fd->fltag == FREGULAR)
		vfdispReg(fd->fld_var.flregfld);
# ifndef FORRBF
	else
		vfTabdisp(fd->fld_var.fltblfld, FALSE);
# endif
}

/*
** display a regular field
** by adding each component to the frame
*/
VOID
vfdispReg(fd)
REGFLD	*fd;
{

	vfClearFld(frame, &fd->flhdr);

	vfdispData(fd, FALSE);
# ifndef FORRBF
	vfFldBox(&fd->flhdr, FALSE);
# endif
	vfTitledisp(&fd->flhdr, FALSE);
}

/*{
** Name:	vfClearFld	- clear area of field to spaces 
**
** Description:
**		Clear the area under a field to spaces.
**		This is done so that any box/line features won't show
**		through unpainted areas inside of fields.
**
** Inputs:
**	FRAME	*frm;	- frame on that the field is on
**	FLDHDR	*hdr;	- field header of the field to clear
**
** Outputs:
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/18/88 (tom) - written for 6.1 release
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
vfClearFld(frm, hdr)
FRAME		*frm;
register FLDHDR *hdr;
{
	register i4	y;
	register i4	x;
	register i4	ny;
	register i4	nx;

	y = hdr->fhposy;
	x = hdr->fhposx;
	ny = hdr->fhmaxy;
	nx = hdr->fhmaxx;

	/*
	** If the field is boxed then we adjust so that we only clear
	** the area inside the box.. we want line joining of boxes
	** with the edge of the field's box.
	*/
	if (hdr->fhdflags & fdBOXFLD)
	{
		y++;
		x++;
		ny -= 2;
		nx -= 2;
	}

	/*
	** clear out the interior of the field to spaces.. so that any
	** box/line features which are under the box won't show through.
	*/
	VTClearArea(frm, y, x, ny, nx); 
}

/*
** display a table field
*/
# ifndef FORRBF
VOID
vfTabdisp(tab, hilight)
TBLFLD	*tab;
bool	hilight;
{
	FLDHDR	*hdr;
	FLDHDR	*chdr;
	FLDTYPE *ctype;
	FLDCOL	**colarr;
	FLDCOL	*col;
	i4	i;
	i4	j;
	i4	begy;
	i4	begx;
	i4	endy;
	i4	endx;
	i4	colcount;

	VTdisptf(frame, tab, hilight);
	hdr = &tab->tfhdr;
	vfTitledisp(hdr, FALSE);
	for (colcount = 0, colarr = tab->tfflds; colcount < tab->tfcols;
		colcount++, colarr++)
	{
		col = *colarr;
		chdr = &col->flhdr;
		ctype = &col->fltype;
		begy = hdr->fhposy + tab->tfstart;
		begx = ctype->ftdatax + hdr->fhposx;
		vfgetSize(ctype->ftfmt, ctype->ftdatatype, ctype->ftlength,
			ctype->ftprec, &i, &j, &endx);
		endy = begy + i - 1;
		endx = begx + j - 1;
		vfFmtdisp(chdr, ctype, begy, begx, endy, endx, FALSE);
	}
}
# endif

/*
** display the title
*/
VOID
vfTitledisp(hdr, hilight)
FLDHDR	*hdr;
bool	hilight;
{
	if (hdr->fhtitle == NULL || hdr->fhtitle[0] == EOS)
		return;
	VTputstring(frame, hdr->fhtity + hdr->fhposy, hdr->fhtitx + hdr->fhposx,
		hdr->fhtitle, 0, hilight, IIVFsfaSpaceForAttr);
}

/*
** write the actual format string on the data window
*/
VOID
vfdispData(fp, hilight)
register REGFLD *fp;
bool	hilight;
{
	POS		ps;
	FLDTYPE		*type;
	FLDHDR		*hdr;

	type = &fp->fltype;
	hdr = &fp->flhdr;
	vfgetDataWin(fp, &ps);
	vfFmtdisp(hdr, type, ps.ps_begy, ps.ps_begx, ps.ps_endy, ps.ps_endx,
		hilight);
}

VOID
vfFmtdisp(hdr, type, begy, begx, endy, endx, hilight)
FLDHDR	*hdr;
FLDTYPE *type;
i4	begy;
i4	begx;
i4	endy;
i4	endx;
bool	hilight;
{
	reg	i4	i;
	i4		dataln;
	i4		nbegy;
	char		*cp;
	char		*sp;
	char		buf[512 + 1];

	sp = cp = buf;
	if ((dataln = endx - begx + 1) > 512)
	{
		if ((sp = (char *)MEreqmem((u_i4)0, (u_i4)(dataln + 1), TRUE,
		    (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("vfFmtdisp"));
		}
		cp = sp;
	}

	/*
	**  Get information for displaying format/datatype for
	**  a field in layout.
	**  fmt_dispinfo() does not look at the length so we don't
	**  need to set it.
	*/
	if (fmt_dispinfo(FEadfcb(), type->ftfmt, WINCHAR, sp) != OK)
	{
		syserr(ERget(S_VF006D_VIFRED__vfFmtdisp__Fa));
	}

	if (hdr->fhdflags & fdSTOUT)
	{
		hdr->fhdflags |= fdRVVID;
	}
	VTputstring(frame, begy, begx, sp, hdr->fhdflags, hilight,
		IIVFsfaSpaceForAttr);

	/*
	**  Make entire buffer consist of WINCHAR for multi-line
	**  format display.
	*/
	cp = sp;
	for (i = 0; i < dataln; i++)
	{
		*cp++ = WINCHAR;
	}
	*cp = '\0';

	for (i = 1, nbegy = begy; i < endy - begy + 1; i++)
	{
		nbegy++;
		VTputstring(frame, nbegy, begx, sp, hdr->fhdflags, hilight,
			IIVFsfaSpaceForAttr);
	}
	if (sp != buf)
	{
		MEfree(sp);
	}
}

/*
** vfFldBox
**
** History:
**	04/10/90  (esd) - added extra parm FALSE to call to VTdispbox
**			  to tell VT3270 *not* to clear the positions
**			  under the box before drawing the box
*/


vfFldBox(hdr, hilight)
reg FLDHDR	*hdr;
bool	hilight;
{
	if (!(hdr->fhdflags & fdBOXFLD))
		return;
	VTdispbox(frame, hdr->fhposy, hdr->fhposx, hdr->fhmaxy, hdr->fhmaxx,
		(i4) 0, hilight, TRUE, FALSE);
}



/*{
** Name:	vfBoxDisp	- display a box given it's position struct
**
** Description:
**	Given position struct and hilight flag draw a box feature.
**
** Inputs:
**	POS  *ps;	- the position struct
**	bool hilite;	- hilight flag
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	04/18/88  (tom) - first written
**      04/10/90  (esd) - Added extra parm TRUE to call to VTdispbox
**                        to tell VT3270 to clear the positions
**                        under the box before drawing the box.
**                        This causes the box to "float" above features
**                        such as fields and trim that might otherwise
**                        obscure it.  As near as I can determine,
**                        this is desired by all callers of vfBoxDisp.
*/
VOID
vfBoxDisp(ps, hilight)
POS	*ps;
bool	hilight;
{
	i4	y, x, endy, endx;

		y = ps->ps_begy; 
		x = ps->ps_begx, 
		endy = ps->ps_endy;
		endx = ps->ps_endx;
		VTdispbox(frame, y, x, endy - y + 1, endx - x + 1, 
			*((i4 *)ps->ps_feat), hilight, FALSE, TRUE);

}
