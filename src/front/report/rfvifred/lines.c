/*
**	lines.c
*/

/*
** lines.c
** contains routines which deal with line table
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	1-25-85 (peterk) - separated from pos.c
**	03/13/87 (dkh) - Added support for ADTs.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	06/21/88 (dkh) - Fixed to handle form size in a saner manner.
**	24-apr-89 (bruceb)
**		No longer reference the non-seq field list.  There aren't
**		any such fields by the time code gets here.
**	05-jun-89 (bruceb)
**		Remove MEfree from spcTrrbld() since the memory being free'd
**		is allocated using FEreqmem.
**	01-sep-89 (cmr)	RBF changes to handle dynamic sections (hdrs/ftrs).
**	19-sep-89 (bruceb)
**		Call vfgetSize() with a precision argument.  Done for decimal
**		support.
**	12/15/89 (dkh) - VMS shared lib changes.
**      06/09/90 (esd) - Check IIVFsfaSpaceForAttr instead of
**                       calling FTspace, so that whether or not
**                       to leave room for attribute bytes can be
**                       controlled on a form-by-form basis.
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**      06/12/90 (esd) - Make the member frmaxx in FRAME during vifred
**                       include room for an attribute byte to the left
**                       of the end marker, as well as the end marker.
**                       (We won't always insert this attribute
**                       before the end marker, but we always leave
**                       room for it).
**      06/12/90 (esd) - Test for overlapping features by calling
**                       inRegion instead of invoking the 'within' macro
**                       (inRegion has been modified to allow two
**                       table fields or boxed regular fields to abut,
**                       even on 3270s).  Modify vfgrf similarly.
**      06/12/90 (esd) - Add new parameter to call to palloc indicating
**                       whether the feature requires attribute bytes
**                       on each side.
**      06/12/90 (esd) - Add extra parm to VTput[v]string to indicate
**                       whether form reserves space for attribute bytes
**	06/07/92 (dkh) - Change section markers to be rulers and adding
**			 cross hairs for 6.5.
**	02/26/92 (dkh) - Fixed bug 49802.  Typo in for loop caused incorrect
**			 section markers to be generated.
**	28-mar-95 (peeje01)
**	    Cross integration doublebyte changes label 6500db_su4_us42:
** **	    06-jul-92 (kirke)
** **		    Fixed handling of vertical trim when text contains
** **		    multi-byte characters.
**      22-Aug-1997 (wonst02)
**          Added parentheses-- '==' has higher precedence than bitwise '&':
**	    IIVFsfaSpaceForAttr && ((hdr->fhdflags & fdBOXFLD) == 0));, not:
**	    IIVFsfaSpaceForAttr && (hdr->fhdflags & fdBOXFLD == 0));
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<cm.h>
# include	<ug.h>
# include	"ervf.h"


# ifdef FORRBF
GLOBALDEF	Sec_List *vfrfsec = NULL;
# endif /* FORRBF */

GLOBALDEF	char	*vfvtrim = NULL;
GLOBALDEF	i4	vfvattrtrim = 0;

GLOBALREF	FT_TERMINFO	IIVFterminfo;

GLOBALDEF	POS	*IIVFvcross = NULL;
GLOBALDEF	POS	*IIVFhcross = NULL;
GLOBALDEF	bool	IIVFxh_disp = FALSE;
GLOBALDEF	bool	IIVFru_disp = FALSE;

FUNC_EXTERN	void	IIVFcross_hair();


/*
** build the lines table
*/
VFNODE **
buildLines(frm, sec, cs, len)
FRAME	*frm;
Sec_List *sec;
CS	*cs;
i2	len;
{
	i4	i;
	i4	nline;
	i4	ncol;
	i4	trimno;
	TRIM	**src;
	TRIM	**dst;
	TRIM	*tr;

	if (frm == NULL)
		return (NULL);

	if ((line = (VFNODE **)FEreqmem((u_i4)0,
	    (u_i4)((frm->frmaxy) * sizeof(VFNODE *)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("buildLines"));
	}

	for (i = 0; i < frm->frfldno; i++)
	{
		if (frm->frfld[i] == NULL)
			syserr(ERget(S_VF007B_buildLines__null_fiel));
		insFld(frm->frfld[i]);
	}
	/* No need to look at frnsno; there aren't any non-seq fields. */

	/*
	** Since BOXes are stored as TRIM in the frame structure...
	** and since vifred does not want the fake TRIM elements..
	** We take these out of the trim table now.
	*/
	trimno = frm->frtrimno;
	src = dst = frm->frtrim;
	for (i = 0; i < trimno; i++)
	{
		if (*src == NULL)
			syserr(ERget(S_VF007D_buildLines__null_trim));

		if ((*src)->trmflags & fdBOXFLD)
		{
			tr = *src++;
			frm->frtrimno--;
			STscanf(tr->trmstr, ERx("%d:%d"), &nline, &ncol);
			insBox(tr->trmy, tr->trmx, nline, ncol, 
						tr->trmflags & ~fdBOXFLD);
		}
		else
		{
			insTrim(*dst++ = *src++);
		}
	}

	/*
	**  Subtract one since we want to go to zero based indexing
	**  for the end of form marker.  The value in frame->frmaxy
	**  includes the end marker.
	*/
	endFrame = frame->frmaxy - 1;

	/*
	**  Subtract 3 since endxFrm is the last column in the form
	**  and we also want to go to zero based indexing.  The value
	**  in frame->frmaxx includes room for the end marker plus
	**  an attribute byte to its left.  (We don't always insert 
	**  an attribute before the end marker, but we always leave
	**  room for it.)
	*/
	endxFrm = frame->frmaxx - 3;

	allocLast(frm);

	trimLines(sec, cs, len);

# ifdef FORRBF
	vfrfsec = sec;
# endif /* FORRBF */

	return (line);
}


spcVBldtr(str, cp)
char	*str;
char	*cp;
{
	i4	length;
	i4	first;
	i4	last;
	i4	i;
	i4	j;
	i4	k;
	char	*cp1;

	if (IIVFru_disp)
	{
		last = endFrame - 1;

		for (i = 0, j = 1, k = 0; i <= last; i++)
		{
			if (j == 5)
			{
				*cp++ = '+';
				j++;
			}
			else if (j == 10)
			{
				if (++k >= 10)
				{
					k = 0;
				}
				switch (k)
				{
			  	case 0:
					*cp++ = '0';
					break;

			  	case 1:
					*cp++ = '1';
					break;

			  	case 2:
					*cp++ = '2';
					break;

			  	case 3:
					*cp++ = '3';
					break;

			  	case 4:
					*cp++ = '4';
					break;

			  	case 5:
					*cp++ = '5';
					break;

			  	case 6:
					*cp++ = '6';
					break;

			  	case 7:
					*cp++ = '7';
					break;

			  	case 8:
					*cp++ = '8';
					break;

			  	case 9:
					*cp++ = '9';
					break;
				}
				j = 1;
			}
			else
			{
				*cp++ = '|';
				j++;
			}
		}
		*cp++ = '+';
	}
	else
	{

# ifndef DOUBLEBYTE
		length = STlength(str);
# else
	cp1 = str;
	length = 0;
	while (*cp1 != EOS)
        {
		length++;
		CMnext(cp1);
	}
# endif /* #ifndef DOUBLEBYTE */


		last = ((endFrame < IIVFterminfo.lines && endFrame > length)
			? endFrame : IIVFterminfo.lines/2);

		first = (last - length + 1)/2;

		for (i = 0; i <= endFrame; )
		{
			for (j = 0; j < first && i <= endFrame; i++, j++)
			{
				*cp++ = '|';
			}

			if (endFrame - i - 1 > length)
			{
				for (cp1 = str; *cp1 && i <= endFrame; i++)
				{
					*cp++ = *cp1++;
				}
			}

			for (; j < last && i <= endFrame; i++, j++)
			{
				*cp++ = '|';
			}
		}
		if (cp[-1] == '|')
		{
			cp[-1] = '+';
		}
	}

	*cp = '\0';
}

VOID
spcVTrim()
{
	if (vfvtrim != NULL)
		MEfree(vfvtrim);

	/*
	**  Adding 2 to endFrame since endFrame is zero based
	**  indexing and also for null terminator.
	*/
	if ((vfvtrim = (char *)MEreqmem((u_i4)0, (u_i4)(endFrame + 2),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("spcVTrim"));
	}
	spcVBldtr(ERget(F_VF0018_End), vfvtrim);
	if (IIVFru_disp)
	{
		vfvattrtrim |= fd4COLOR;
	}
	else
	{
		vfvattrtrim &= ~fd4COLOR;
	}
	VTputvstr(frame, 0, endxFrm + 1 + IIVFsfaSpaceForAttr,
	    vfvtrim, vfvattrtrim, FALSE, IIVFsfaSpaceForAttr);
}


static	char	*endMess = NULL;

VOID
IIVFlninit()
{
	endMess = ERget(F_VF0025_End_of_Form);
}

VOID
trimLines(sec, cs, len)
Sec_List *sec;
CS	*cs;
i2	len;
{
# ifdef FORRBF
	Sec_node	*p;

	for ( p = sec->head; p != sec->tail; p = p->next )
	{
		spcTrim( p->sec_name, p->sec_y, 0 );
	}
	vfAscTrim(cs, len);
# endif
	if (RBF || line[endFrame] != NULL)
	{
		vfnewLine(1);
		newFrame(1);
	}
#ifdef FORRBF
	spcTrim( p->sec_name, endFrame, 0);
# else
	spcTrim(endMess, endFrame, 0);
# endif

	if (IIVFxh_disp)
	{
		IIVFcross_hair();
	}

	spcVTrim();
}


void
IIVFcross_hair()
{
	i4		startcol;
	i4		startline;
	i4		xh_posy = 0;
	i4		last_detail = 0;
	i4		endx = endxFrm;
	i4		endy = endFrame;
	Sec_List	*sec;
	i4		attr = 0;


# ifdef FORRBF
	Sec_node	*p;
	i4		saw_detail = FALSE;

	sec = &Sections;

	for ( p = sec->head; p != sec->tail; p = p->next )
	{
		if (p->sec_type == SEC_DETAIL)
		{
			saw_detail = TRUE;
		}
		else if (saw_detail == TRUE)
		{
			last_detail = p->sec_y - 1;
			saw_detail = FALSE;
		}
	}
# endif /* FORRBF */

	startcol = endx;

	if (RBF)
	{
		startline = last_detail;
	}
	else
	{
		startline = endy - 1;
	}

	attr |= fd4COLOR;

	/*
	**  Using IIFVterminfo.cols - 1 for start x position for
	**  vertical cross hair since the code uses zero based indexing.
	*/
	if (IIVFvcross == NULL)
	{
		IIVFvcross = insBox(0, startcol, endy, 1, attr);
		IIVFvcross->ps_attr = IS_STRAIGHT_EDGE;
	}
	else
	{
		IIVFvcross->ps_begx = startcol;
		IIVFvcross->ps_begy = 0;
		IIVFvcross->ps_endx = startcol;
		IIVFvcross->ps_endy = endy - 1;

		insPos(IIVFvcross, FALSE);
	}

	/*
	**  Using IIFVterminfo.lines - 2 for start y position for
	**  horizontal cross hair since the code uses zero based indexing
	**  and the bottom line contains the menu line.  This may need
	**  to change if the menu is at the top rather than the bottom.
	*/
	if (IIVFhcross == NULL)
	{
		IIVFhcross = insBox(startline, 0, 1, endx + 1, attr);
		IIVFhcross->ps_attr = IS_STRAIGHT_EDGE;
	}
	else
	{
		IIVFhcross->ps_begx = 0;
		IIVFhcross->ps_begy = startline;
		IIVFhcross->ps_endx = endx;
		IIVFhcross->ps_endy = startline;

		insPos(IIVFhcross, FALSE);
	}
}

VOID
IIVFrvxRebuildVxh()
{
	IIVFvcross->ps_endy = endFrame - 1;

	insPos(IIVFvcross, FALSE);
}



# ifdef JUNK
spcFind(beginy)
i4	beginy;
{
	i4	i;
	VFNODE	*nd;
	POS	*ps;

	for (i = beginy; i <= endFrame; i++)
	{
		if ((nd = line[i]) == NULL)
		{
			continue;
		}
		if ((ps = nd->nd_pos) == NULL)
		{
			continue;
		}
		if (ps->ps_name == PSPECIAL)
		{
			return (i);
		}
	}
	return (endFrame);
}
# endif

/*
**  Rebuild the special trim when the form gets expanded in
**  width.  Care must be taken for the RBF case since the
**  special title, head and detail markers may have its
**  position changed.
*/

VOID
spcRbld()
{
	char	*cp;
# ifdef FORRBF
	Sec_node	*p;
# endif

	/*
	**  Adding 2 to endxFrm since endxFrm is zero based
	**  indexing and also for null terminator.
	*/
	if ((cp = (char *)MEreqmem((u_i4)0, (u_i4)(endxFrm + 2), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("spcRbld"));
	}
# ifdef FORRBF
	for ( p = vfrfsec->head; p ; p = p->next )
	{
		spcTrrbld(p->sec_name, p->sec_y, 0, cp);
	}
# else /* FORRBF */
	spcTrrbld(endMess, endFrame, 0, cp);

# endif /* FORRBF */
	MEfree(cp);
}

VOID
spcTrrbld(str, y, x, buf)
char	*str;
i4	y;
i4	x;
char	*buf;
{
	char	*cp;
	VFNODE	*nd;
	POS	*ps;
	TRIM	*tr;

	spcBldtr(str, buf);
	cp = saveStr(buf);
	nd = line[y];
	ps = nd->nd_pos;
	tr = (TRIM *) ps->ps_feat;

	tr->trmstr = cp;
	if (IIVFru_disp)
	{
		tr->trmflags |= fd4COLOR;
	}
	else
	{
		tr->trmflags &= ~fd4COLOR;
	}
	ps->ps_endx = endxFrm;
	VTputstring(frame, y, x, cp, tr->trmflags, FALSE, IIVFsfaSpaceForAttr);
}

void
tick_mark(one_to_ten, tens_digit, mark)
i4	*one_to_ten;
i4	*tens_digit;
char	*mark;
{
	if (*one_to_ten == 5)
	{
		*mark = '+';
		*one_to_ten = *one_to_ten + 1;
	}
	else if (*one_to_ten == 10)
	{
		if ((*tens_digit = *tens_digit + 1) >= 10)
		{
			*tens_digit = 0;
		}
		switch (*tens_digit)
		{
		  case 0:
			*mark= '0';
			break;

		  case 1:
			*mark = '1';
			break;

		  case 2:
			*mark = '2';
			break;

		  case 3:
			*mark = '3';
			break;

		  case 4:
			*mark = '4';
			break;

		  case 5:
			*mark = '5';
			break;

		  case 6:
			*mark = '6';
			break;

		  case 7:
			*mark = '7';
			break;

		  case 8:
			*mark = '8';
			break;

		  case 9:
			*mark = '9';
			break;
		}
		*one_to_ten = 1;
	}
	else
	{
		*mark = '-';
		*one_to_ten = *one_to_ten + 1;
	}
}

VOID
spcBldtr(str, buf)
char	*str;
char	*buf;
{
	i4	i;
	i4	j;
	i4	k;
	i4	l;
	char	*cp = buf;
	i4	halfscr;
	i4	len;
	i4	first;
	i4	last;
	char	*cp1;
	char	mark;

	len = STlength(str);

	/*
	**  If string is longer thatn the width of the form, use
	**  a shorter generic string.
	*/
	if (len >= endxFrm - 2)
	{
		len = STlength(str = ERget(F_VF0018_End));
	}

	halfscr = (endxFrm < IIVFterminfo.cols && endxFrm > len)
		? endxFrm : IIVFterminfo.cols/2;

	first = (halfscr - len + 1)/2;

	if (IIVFru_disp)
	{
		for (i = 0, j = 1, k = 0; i <= endxFrm; )
		{
			for (l = 0; l < first && i <= endxFrm; l++, i++)
			{
				tick_mark(&j, &k, &mark);
				*cp++ = mark;
			}

			if (endxFrm - i > len)
			{
				for (cp1 = str; *cp1 && i <= endxFrm; l++, i++)
				{
					*cp++ = *cp1++;

					/*
					**  Call tick_mark() to update j & k but
					**  don't use the tick mark that is set
					**  since we are filling in the section
					**  name.
					*/
					tick_mark(&j, &k, &mark);
				}
			}

			for (; l < halfscr && i <= endxFrm; l++, i++)
			{
				tick_mark(&j, &k, &mark);
				*cp++ = mark;
			}
		}
	}
	else
	{
		last = endxFrm;
		for (i = 0; i <= last; )
		{
			for (j = 0; j < first && i <= last; j++, i++)
			{
				*cp++ = '-';
			}

			if (last - i > len)
			{
				for (cp1 = str; *cp1 && i <= last; j++, i++)
				{
					*cp++ = *cp1++;
				}
			}

			for (; j < halfscr && i <= last; j++, i++)
			{
				*cp++ = '-';
			}
		}
	}

	*cp = '\0';
}

VOID
spcTrim(str, y, x)
char	*str;
i4	y,
	x;
{
	TRIM		*tr;
	POS		*ps;
	register char	*cp;
	char		*sp;

	/*
	**  Adding 3 to endxFrm since endxFrm is zero based
	**  indexing and also for null terminator.
	*/
	if ((sp = (char *)MEreqmem((u_i4)0, (u_i4)(endxFrm + 3), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("spcTrim"));
	}

	spcBldtr(str, sp);
	cp = saveStr(sp);
	tr = trAlloc(y, x, cp);
	ps = palloc(PSPECIAL, y, x, 1, endxFrm + 1, tr, FALSE);
	insPos(ps, TRUE);
	VTputstring(frame, y, x, cp, 0, FALSE, IIVFsfaSpaceForAttr);
	MEfree(sp);
}

/*
** insert a field in the lines structure
** all pos which point to one feature are linked
** in a circular list on group field
*/
VOID
insFld(fd)
FIELD	*fd;
{

	if (fd->fltag == FREGULAR)
	{
		insReg(fd);
	}
# ifndef FORRBF
	else
	{
		insTab(fd);
	}
# endif
	vfFlddisp(fd);
}

# ifndef FORRBF
VOID
insTab(fd)
FIELD	*fd;
{
	TBLFLD	*tab;
	FLDHDR	*hdr;
	i4	start;
	POS	*ps;
	FLDCOL	**col;
	i4	i;

	tab = fd->fld_var.fltblfld;
	hdr = &tab->tfhdr;
	start = hdr->fhposy,
	ps = palloc(PTABLE, start, hdr->fhposx, hdr->fhmaxy, hdr->fhmaxx,
		    (char *) fd, FALSE);
	for (i = 0, col = tab->tfflds; i < tab->tfcols; i++, col++)
	{
		vffillType(&(*col)->fltype);
	}
	insPos(ps, TRUE);
}
# endif

VOID
insReg(fd)
FIELD	*fd;
{
	i4	i;
	i4	start;
	POS	*ps;
	FLDHDR	*hdr;
	FLDTYPE *type;

	hdr = FDgethdr(fd);
	type = FDgettype(fd);
	start = hdr->fhposy,
	vffillType(type);
	/*
	** Code added for fix to bug 1437
	** (dkh)
	*/
	if (RBF)
	{
		vfgetSize(type->ftfmt, type->ftdatatype, type->ftlength,
			type->ftprec, &(hdr->fhmaxy), &(hdr->fhmaxx), &i);
	}
	ps = palloc(PFIELD, start, hdr->fhposx, hdr->fhmaxy, hdr->fhmaxx,
		    (char *) fd,
		    IIVFsfaSpaceForAttr && ((hdr->fhdflags & fdBOXFLD) == 0));
	insPos(ps, TRUE);
}

VOID
vffillType(type)
FLDTYPE *type;
{
	char	buf[BUFSIZ];

	if (type->ftfmtstr == NULL)
	{
		/*
		**  Temporary buffer for call to saveStr() must be
		**  on stack.
		*/
		vfFDFmtstr(type, buf);
		type->ftfmtstr = saveStr(buf);
		type->ftfmt = vfchkFmt(type->ftfmtstr);
		if (type->ftfmt == NULL)
			syserr(ERget(S_VF007E_INSFLD__can_t_create_));
	}
	else if (type->ftfmt == NULL)
	{
		type->ftfmt = vfchkFmt(type->ftfmtstr);
		if (type->ftfmt == NULL)
			syserr(ERget(S_VF007E_INSFLD__can_t_create_));
	}
	if (type->ftdefault == NULL)
	{
		type->ftdefault = saveStr(ERx("\0"));
	}
	if (type->ftvalstr == NULL)
	{
		type->ftvalstr = saveStr(ERx("\0"));
	}
	if (type->ftvalmsg == NULL)
	{
		type->ftvalmsg = saveStr(ERx("\0"));
	}
}

			
/*{
** Name:	insBox	- create and insert a new box into the line table 
**
** Description:
**		Given the box parms put the box into the line table. Note:
**		boxes have no corresponding entrys in the frame structure
**		because of old user frame structure compatability.
**
** Inputs:
**
** Outputs:
**	i4	sy;	- start y
**	i4	sx;	- start x
**	i4	nline;	- number of lines in box 
**	i4	ncol;	- number of columns in box 
**	i4	atr;	- attribute
**
**	Returns:
**		nothing
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/13/88  (tom)  - written
*/
POS *
insBox(sy, sx, nline, ncol, atr)
i4	sy;
i4	sx;
i4	nline;
i4	ncol;
i4	atr;
{
	i4	*p;
	POS	*ps;

	/* allocate for the attribute of the box */
	if ((p = (i4 *)FEreqmem((u_i4)0, 
		(u_i4)sizeof(i4), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("insBox"));
	}

	*p = atr;		/* set attribute */

	ps = palloc(PBOX, sy, sx, nline, ncol, (i4 *) p, FALSE);
	
	insPos(ps, FALSE);
	return (ps);
}

/*
** insert a trim in the lines structure
** since trim are one line they have no one in their group
*/
VOID
insTrim(tr)
TRIM	*tr;
{
	POS	*ps;

	ps = palloc(PTRIM, tr->trmy, tr->trmx, 1, STlength(tr->trmstr),
		    (char *) tr, IIVFsfaSpaceForAttr);
	insPos(ps, TRUE);
}


/*
** insert a new position in the lines table
** if there is an overlap then print error message
*/
VOID
insPos(ps, checkOver)
register POS	*ps;
bool	checkOver;
{
	register i4	i;
	register VFNODE **nd;
	VFNODE		*newnd;

	newnd = ndalloc(ps);
	nd = &(line[ps->ps_begy]);
	for (i = ps->ps_begy; i <= ps->ps_endy; i++, nd++)
		insert(newnd, nd, i, checkOver);
}

VOID
insert(new, nd, i, checkOver)
VFNODE	*new;
VFNODE	**nd;
i4	i;
bool	checkOver;
{
	POS	*ps = new->nd_pos;

	while (*nd != NULL && ps->ps_begx > (*nd)->nd_pos->ps_begx)
	{
		if (	checkOver 
		   && 	(*nd)->nd_pos->ps_name != PBOX 
		   &&	inRegion(ps, (*nd)->nd_pos)
		   )
		{
			errOver(ps, (*nd)->nd_pos);
		}
		nd = vflnAdrNext(*nd, i);
	}
	*vflnAdrNext(new, i) = *nd;
	*nd = new;
}

/*
** build an adjacency list for the passed ps
*/
VFNODE *
vfgrfbuild(ps)
register POS	*ps;
{
	register i4	i;
	register VFNODE **nd;
	VFNODE		*newnd;

	newnd = ndalloc(ps);
	nd = &(line[ps->ps_begy]);
	for (i = ps->ps_begy; i <= ps->ps_endy && i <= endFrame; i++, nd++)
		vfgrf(newnd, nd, i);
	return (newnd);
}

VOID
vfgrf(new, nd, i)
VFNODE	*new;
VFNODE	**nd;
i4	i;
{
	POS	*ps = new->nd_pos;
	POS	*np;

	while (  *nd != NULL
	      && ( np = (*nd)->nd_pos,
	         ps->ps_begx > np->ps_endx
	         || np->ps_name == PBOX
		 )
	      )
	{
		nd = vflnAdrNext(*nd, i);
	}
	*vflnAdrNext(new, i) = *nd;
}

VOID
errOver(p1, p2)
POS	*p1,
	*p2;
{
	FTclose(NULL);

	SIfprintf(stderr, ERget(S_VF007F__rexisting___d___d___), p2->ps_begx,
	     p2->ps_begy, p2->ps_endx, p2->ps_endy);
	prFeature(p2);
	SIfprintf(stderr, ERget(S_VF0080__rnew___d___d____d___), p1->ps_begx,
	     p1->ps_begy, p1->ps_endx, p1->ps_endy);
	prFeature(p1);
	syserr(ERget(S_VF0081__rerror_in_existing_f));
	SIflush(stderr);
}
