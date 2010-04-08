/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<fmt.h>
# include	<scroll.h>
# include	<ug.h>
# include	<cm.h>
# include	<er.h>
# include	<adf.h>
# include	"erfd.h"


/**
** Name:	scrlfmt.c -	Create fmt struct for scrolling field
**
** Description:
**	This file defines:
**
**	IIFDssfSetScrollFmt	Create fmt struct for scrolling field
**
** History:
**	11-nov-87 (bab)	Initial implementation.
**	25-nov-87 (bab)
**		Moved to frame directory (from ft), and create FMT
**		struct via fmt routines.
**	04/07/89 (dkh) - Added parameter to IIUGbmaBadMemoryAllocation().
**	06-mar-90 (bruceb)
**		Set, then use, scroll->scr_fmt instead of recreating the
**		FMT struct every call to this routine.  Use FEreqmem instead
**		of MEreqmem, and use the (new) tag param on the reqmem call.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/


/*{
** Name:	IIFDssfSetScrollFmt	- Create fmt struct for scrolling fld
**
** Description:
**	Create a fmt structure for a scrolling field from a passed in
**	format.  E.g. Given a format such as c24 and an underlying scroll
**	buffer of width 32, this routine should return the equivalent of a
**	c32 format.  Essentially, set the fmt_width member to the 'width'
**	parameter, and fmt_prec to 0.
**
**	This is so that the display dbv will be allowed to contain the
**	full scroll buffer width of text rather than the shorter displayed
**	width.
**
** Inputs:
**	ofmtstr		Displayed data format (e.g., the c24).
**	tag		Memory tag to use on the FEreqmem call.
**	scroll		Pointer to the underlying scroll struct.
**
** Outputs:
**	scroll		scroll->scr_fmt now points to *scrollfmt.
**	scrollfmt	The created, full-width format (e.g., the c32).
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	11-nov-87 (bab)	Initial implementation.
*/
VOID
IIFDssfSetScrollFmt(ofmtstr, tag, scroll, scrollfmt)
char		*ofmtstr;
i4		tag;
SCROLL_DATA	*scroll;
FMT		**scrollfmt;
{
    i4		width;
    i4		fmtsize;
    PTR		blk;
    char	fmtstr[50];	/* longer than any real fmt string */
    char	*cp;
    char	*fmtp;
    char	widthstr[10];

    if (scroll->scr_fmt)
    {
	*scrollfmt = (FMT *)(scroll->scr_fmt);
    }
    else
    {
	width = scroll->right - scroll->left + 1;
	cp = ofmtstr;
	fmtp = fmtstr;
	while (CMalpha(cp))
	{
		CMcpyinc(cp, fmtp);
	}
	CVna(width, widthstr);
	cp = widthstr;
	while (*cp)
	{
		*fmtp++ = *cp++;
	}
	*fmtp = EOS;

	if (fmt_areasize(FEadfcb(), fmtstr, &fmtsize) == OK)
	{
		if ((blk = FEreqmem((u_i4)tag, (u_i4)fmtsize, TRUE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFDssfSetScrollFmt"));
		}
		_VOID_ fmt_setfmt(FEadfcb(), fmtstr, blk, scrollfmt,
			(i4 *)NULL);
		scroll->scr_fmt = (PTR)*scrollfmt;
	}
	else
	{
		IIUGerr(E_FD0024_bad_scroll_fmt, UG_ERR_FATAL,
			2, ofmtstr, fmtstr);
	}
    }
}
