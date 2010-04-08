/*
**  vi.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**
**  08/19/85 - DKH - Added support for sideway scrolling in a field
**		     also added table field support.
**  08/25/86 - bruceb - changed order of rgbuf struct entries used to determine
**		     length of MEcopy.	Also, for now, disallow editing of
**		     fields whose line width is > MAXEDITSZ.  Disallow such
**		     editing to side-step bug 10020; fixing the bug was
**		     postponed until it has been determined what a correct
**		     fix should be.
**  02/19/87 - KY  - Added Double Bytes characters handling.
**  09/26/86 (a.dea) -- Change ternary statements into
**		     if-then-else-s because stupid IBM/CMS compiler
**		     has trouble under certain conditions.
**  24-apr-87 (bruceb)
**	Added FTputline and FTgetline to allow for editing of reversed
**	fields.	 NOTE:	Am NOT calling FTswapparens (which reverses (),
**	[], {} and <> for RL fields) on the assumption that the majority
**	of users of RL fields will be using RL editors.	 The reasoning
**	is that an RL entry of "*[z-a]" will be placed in the file as
**	"]a-z[*", and subsequently read into the editor as "*[z-a]" again,
**	and vice-versa.
**	06/03/87 (dkh) - Added forward declaration of FTgetline and
**			 FTputline to eliminate compiler complaints.
**	08/14/87 (dkh) - ER changes.
**	08/21/87 (dkh) - Changed handling for resetting terminal.
**	09/04/87 (dkh) - Added new argument to TDrestore() call.
**	25-nov-87 (bruceb)
**		Added scrolling field code; includes IIFTpslPutScrollLine.
**	12/12/87 (dkh) - Using IIUGerr.
**	09-feb-88 (bruceb)
**		Scrolling flag now in fhd2flags; can't use most significant
**		bit of fhdflags.
**	06/18/88 (dkh) - Integrated MVS changes.
**	02-jul-90 (bruceb)	Fix for bug 30446.
**		Upper- or lowercase the strings returned by the editor as
**		the field attributes require.
**	11/02/90 (dkh) - Put in correct parameter for displaying error
**			 message E_FT0017.
**	10-oct-91 (seg)
**		SIgetrec should be declared in si.h
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<ut.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	"ftrange.h"
# include	<lo.h>
# include	<si.h>
# include	<te.h>
# include	<cm.h>
# include	<cv.h>
# include	<er.h>
# include	<erft.h>
# include	<scroll.h>
# include	<ug.h>

# define	MAXEDITSZ	500
# define	EBUFSIZE	512
# define	RG_BUFSIZE	2048

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	RGRFLD	*FTgetrg();
FUNC_EXTERN	STATUS	IIFTpslPutScrollLine();

static	VOID	FTgetline();
static	VOID	FTputline();

VOID
FTvi(frm, fldno, datawin)
reg FRAME	*frm;
reg i4		fldno;
reg WINDOW	*datawin;
{
	reg	char		*cp;
	reg	char		**line;
	reg	u_char		*dx;
	reg	char		**linedx;
	reg	i4		i;
	i4			maxy;
	i4			maxx;
	i4			remain;
	reg	i4		restofstring;
	reg	i4		reclength;
	FILE			*fp;
	RGRFLD			*rgptr;
	LOCATION		loc;
	LOCATION		savloc;
	u_char			bufr[EBUFSIZE];
	u_char			bufdx[EBUFSIZE];
	char			locbuf[MAX_LOC];
	char			rgbuf[RG_BUFSIZE];
	bool			inrngmd = FALSE;
	bool			reverse = FALSE;
	FIELD			*fld;
	FLDHDR			*hdr;
	bool			scrolling = FALSE;
	SCROLL_DATA		*scrl_fld;
	i4			scrl_width;
	char			*start_pos;
	bool			lower = FALSE;
	bool			upper = FALSE;

	fld = frm->frfld[fldno];
	hdr = (*FTgethdr)(fld);

	maxy = datawin->_maxy;
	maxx = datawin->_maxx;

	FTbotsav();

	if (hdr->fhd2flags & fdSCRLFD)
	{
		scrolling = TRUE;
		scrl_fld = IIFTgssGetScrollStruct(fld);
		scrl_width = scrl_fld->right - scrl_fld->left + 1;
	}

	/*
	** Check that the text to be edited isn't over our arbitrary
	** limit.  Aren't allowing long text until decision reached
	** on what to do about splitting/reforming lines that are
	** too long (too long means that editors can't handle lines
	** that long.)
	*/
	if ((maxx > MAXEDITSZ) || (scrolling && scrl_width > MAXEDITSZ))
	{
		maxx = MAXEDITSZ;
		IIUGerr(E_FT0017_Currently_unable_to_e, UG_ERR_ERROR,
			1, (scrolling ? (PTR) &scrl_width : (PTR) &maxx));
		FTbotrst();
		return;
	}

	if (NMt_open(ERx("w"), ERx("frs"), ERx("edt"), &loc, &fp) != OK)
	{
		IIUGerr(E_FT0018_Could_not_open_tempor, UG_ERR_ERROR, 0);
		FTbotrst();
		return;
	}

	LOcopy(&loc, locbuf, &savloc);

	if (hdr->fhdflags & fdREVDIR)
	{
		reverse = TRUE;
	}

	if (scrolling)
	{
		FTgetline(scrl_fld->left, (u_i2)scrl_width, reverse, bufr);
		STtrmwhite((char *) bufr);
		STcat((char *) bufr, ERx("\n"));
		SIputrec(bufr, fp);
	}
	else
	{
		if (frm->frmflags & fdRNGMD)
		{
			inrngmd = TRUE;
		}
		if (inrngmd)
		{
			i4	length;
			i4	copylen;
			u_char	*tempcp;
	
			rgptr = FTgetrg(frm, fldno);
			if (rgptr->hcur != NULL)
			{
				cp = rgptr->hbuf;
				length = rgptr->hcur - rgptr->hbuf + 1;
				while (length > 0)
				{
# ifdef WSC
					if (length >= maxx)
					    copylen = maxx;
					else
					    copylen = length;
# else
					copylen =
					    (length >= maxx) ? maxx : length;
# endif
					MEcopy((PTR) cp, (u_i2) copylen,
						(PTR) bufr);
					tempcp = bufr + copylen;
					CMprev(tempcp, bufr);
					if (tempcp + 1 == bufr + copylen
						&& CMdbl1st(tempcp))
					{
						copylen--;
					}
					length -= copylen;
					bufr[copylen] = '\0';
					STtrmwhite((char *) bufr);
					STcat((char *) bufr, ERx("\n"));
					SIputrec(bufr, fp);
					cp += copylen;
				}
			}
		}
		line = datawin->_y;
		for (i = 0; i < maxy; i++, line++)
		{
			FTgetline(*line, (u_i2)maxx, reverse, bufr);
			STtrmwhite((char *) bufr);
			STcat((char *) bufr, ERx("\n"));
			SIputrec(bufr, fp);
		}
	
		if (inrngmd)
		{
			if (rgptr->tcur != NULL)
			{
				i4	length;
				i4	copylen;
				u_char	*tempcp;
	
				length = rgptr->tend - rgptr->tcur + 1;
				cp = rgptr->tcur;
				while (length > 0)
				{
# ifdef WSC
					if (length >= maxx)
					    copylen = maxx;
					else
					    copylen = length;
# else
					copylen =
					    (length >= maxx) ? maxx : length;
# endif
					MEcopy((PTR) cp, (u_i2) copylen,
						(PTR) bufr);
					tempcp = bufr + copylen;
					CMprev(tempcp, bufr);
					if (tempcp + 1 == bufr + copylen
						&& CMdbl1st(tempcp))
					{
						copylen--;
					}
					length -= copylen;
					bufr[copylen] = '\0';
					STtrmwhite((char *) bufr);
					STcat((char *) bufr, ERx("\n"));
					SIputrec(bufr, fp);
					cp += copylen;
				}
			}
		}
	}

	SIclose(fp);

	TDrefresh(stdmsg);
	TEflush();
	TDrestore(TE_NORMAL, FALSE);
	SIprintf(ERget(S_FT0019__nStarting_Editor____));
	SIflush(stdout);
	if (UTedit(NULL, &savloc) != OK)
	{
		TDrestore(TE_FORMS, FALSE);
		TDrefresh(curscr);
		IIUGerr(E_FT001A_Could_not_start_Edito, UG_ERR_ERROR, 0);
		FTbotrst();
		return;
	}
	TDrestore(TE_FORMS, FALSE);
	TDrefresh(curscr);

	if (SIopen(&savloc, ERx("r"), &fp) != OK)
	{
		IIUGerr(E_FT001B_Could_not_open_tempor, UG_ERR_ERROR, 0);
		FTbotrst();
		return;
	}

	TDerschars(datawin);
	line = datawin->_y;
	linedx = datawin->_dx;
	i = 0;
	if (scrolling)
	{
		remain = scrl_width;
		IIFTcsfClrScrollFld(scrl_fld, reverse);
		start_pos = scrl_fld->scr_start;
	}
	else
	{
		remain = maxx;
	}
	cp = *line;
	dx = (u_char *) *linedx;

	if (inrngmd)
	{
		rgptr->hcur = NULL;
		rgptr->tcur = NULL;
		rgbuf[0] = '\0';
	}

	if (hdr->fhdflags & fdLOWCASE)
	{
	    lower = TRUE;
	}
	else if (hdr->fhdflags & fdUPCASE)
	{
	    upper = TRUE;
	}

	while (SIgetrec(bufr, (i4)EBUFSIZE - 1, fp) == OK)
	{
		reclength = STlength((char *) bufr);
		bufr[--reclength] = '\0';
		reclength = STtrmwhite((char *) bufr);
		if (lower)
		{
		    CVlower(bufr);
		}
		else if (upper)
		{
		    CVupper(bufr);
		}

		if (scrolling)
		{
			if (reclength > 0)
			{
				if (IIFTpslPutScrollLine(bufr, reclength,
					reverse, &start_pos, &remain) != OK)
				{
					break;
				}
			}
		}
		else
		{
			if (i >= maxy)
			{
				if (inrngmd)
				{
					i4	rgbflen;
					i4	bufrlen;
	
					/*
					**  Add to tail buffer.
					**  If there is no room
					**  add more.
					*/
					rgbflen = STlength(rgbuf);
					bufrlen = STlength((char *) bufr);
					if (rgbflen + bufrlen < RG_BUFSIZE)
					{
						STcat(rgbuf, (char *) bufr);
						continue;
					}
				}
	
				IIUGerr(E_FT001C_Data_in_file_too_big_,
					UG_ERR_ERROR, 0);
				FTbotrst();
				break;
			}
			else
			{
				u_char	*tempcp = bufr;
				u_char	*tempdx = bufdx;
	
				while (*tempcp)
				{
					*tempdx = '\0';
					if (CMdbl1st(tempcp))
					{
						*(tempdx+1) = _DBL2ND;
					}
					CMbyteinc(tempdx, tempcp);
					CMnext(tempcp);
				}
			}
			if (reclength > remain)
			{
				FTputline(bufr, bufdx, (u_i2)remain, maxx,
					reverse, cp, dx);
				if (CMdbl1st(cp+remain-1) &&
					*(dx+remain-1) == '\0')
				{
					*(cp+remain-1) = PSP;
					*(dx+remain-1) = _PS;
					remain--;
				}
				restofstring = reclength - remain;
				cp = (char *) &bufr[remain];
				dx = &bufdx[remain];
				while (restofstring > 0 && ++i < maxy)
				{
					line++;
					linedx++;
					if (restofstring > maxx)
					{
						restofstring -= maxx;
						FTputline(cp, dx, (u_i2)maxx,
							maxx, reverse, *line,
							*linedx);
						cp += maxx;
						dx += maxx;
						if (CMdbl1st(*line+maxx-1) &&
						    *(*linedx+maxx-1) == '\0')
						{
							cp--;
							dx--;
							*(*line+maxx-1) = PSP;
							*(*linedx+maxx-1) = _PS;
						}
					}
					else
					{
						remain = maxx - restofstring;
						FTputline(cp, dx,
							(u_i2)restofstring,
							maxx, reverse, *line,
							*linedx);
						cp = *line;
						cp += restofstring;
						dx = (u_char *) *linedx;
						dx += restofstring;
						restofstring = 0;
					}
				}
				if (i >= maxy)
				{
					if (inrngmd)
					{
						i4	rgbflen;
						i4	bufrlen;
	
						/*
						**  Add to tail buffer.
						**  If there is no room
						**  add more.
						*/
	
						rgbflen = STlength(rgbuf);
						bufrlen = STlength(cp);
						if (rgbflen + bufrlen
							< RG_BUFSIZE)
						{
							STcat(rgbuf, cp);
							continue;
						}
					}
	
					IIUGerr(E_FT001D_Data_in_file_too_big_,
						UG_ERR_ERROR, 0);
					FTbotrst();
					break;
				}
			}
			else
			{
				FTputline(bufr, bufdx, (u_i2)reclength, maxx,
					reverse, cp, dx);
				remain = maxx;
				line++;
				cp = *line;
				linedx++;
				dx = (u_char *) *linedx;
				i++;
			}
		}
	}


	if (scrolling)
	{
		if (reverse)
		{
			IIFTsfaScrollFldAddstr(datawin, scrl_width,
				(scrl_fld->right - (maxx * maxy) + 1));
		}
		else
		{
			IIFTsfaScrollFldAddstr(datawin, scrl_width,
				scrl_fld->left);
		}
	}
	TDtouchwin(datawin);
	if (reverse)
		TDmove(datawin, (i4)0, datawin->_maxx - 1);
	else
		TDmove(datawin, (i4)0, (i4)0);
	SIclose(fp);

	/*
	**  Changed from LOdelete for fix to BUG 5205. (dkh)
	*/

	LOpurge(&savloc, (i4) 0);

	if (inrngmd)
	{
		FTtlxadd(rgptr, rgbuf);
	}
}

/*
** Used to get text from the window (for the edit file).
** When a reversed field, get the rightmost char first.
**
** dest is assumed to be large enough for the copy to work.
** (If this is not the case, then this indicates a bug.)
*/
static VOID
FTgetline(source, length, reverse, dest)
char		*source;
u_i2		length;
bool		reverse;
reg char	*dest;
{
	reg char	*from;

	if (reverse)
	{
		from = source + length - 1;
		while (from >= source)
			*dest++ = *from--;
		*dest = EOS;
	}
	else
	{
		MEcopy(source, length, dest);
		dest[length] = EOS;
	}
}

/*
** used to put text to the screen (from the edit file).
** when a reversed field, need to place the first char
** in the rightmost position of the window.
** strings are potentially shorter than the window's width.
**
** Unlike FTgetline, dest is not null terminated since the
** copy is directly to the window struct.
**
** dest and destdx are assumed to be large enough for the copy to work.
** (If this is not the case, then this indicates a bug.)
*/
static VOID
FTputline(source, sourcedx, length, maxx, reverse, dest, destdx)
reg char	*source;
reg char	*sourcedx;
reg u_i2	length;
i4		maxx;
bool		reverse;
char		*dest;
char		*destdx;
{
	reg char	*to;
	reg char	*todx;

	if (reverse)
	{
		to = dest + maxx - 1;
		todx = destdx + maxx - 1;
		for ( ; length > 0; length--)
		{
			*to-- = *source++;
			*todx-- = *sourcedx++;
		}
	}
	else
	{
		MEcopy(source, length, dest);
		MEcopy(sourcedx, length, destdx);
	}
}

/*{
** Name:	IIFTpslPutScrollLine	- Put line from editor into scroll buf.
**
** Description:
**	Put a line from the editor into the scroll buffer.  Attention is paid
**	to the direction in which to place the text.
**
** Inputs:
**	bufr		Contents of a line from the editor file.
**	reclength	Length of contents of bufr.
**	reverse		Indication of whether field is LR or RL.
**	start_pos	Memory location for placement of first byte from bufr.
**	remain		Remaining space in the scroll buffer.
**
** Outputs:
**	start_pos	Updated to point to the location to place the first byte
**			from the next editor line.
**	remain		Updated to indicate space remaining for further text.
**
**	Returns:
**		OK
**		FAIL	Indicate no further space available in scroll buffer.
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	25-nov-87 (bruceb)	Initial implementation.
*/
STATUS
IIFTpslPutScrollLine(bufr, reclength, reverse, start_pos, remain)
char	*bufr;
i4	reclength;
bool	reverse;
char	**start_pos;
i4	*remain;
{
	STATUS	retval = OK;

	if (reclength > *remain)
	{
		FTwinerr(ERget(E_FT001D_Data_in_file_too_big_), FALSE);
		retval = FAIL;
		reclength = *remain;
	}
	(*remain) -= reclength;

	if (reverse)
	{
		for (; reclength > 0; reclength--)
			*(*start_pos)-- = *bufr++;
	}
	else
	{
		MEcopy(bufr, (u_i2)reclength, *start_pos);
		*start_pos += reclength;
	}

	if (*remain > 0)
	{
		/*
		** Place blank between successive lines of the file.
		** Blank is guaranteed by the use of IIFTcsfClrScrollFld.
		*/
		(*remain)--;
		if (reverse)
			(*start_pos)--;
		else
			(*start_pos)++;
	}

	return (retval);
}
