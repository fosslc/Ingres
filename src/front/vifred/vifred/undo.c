/*
** undo.c
**
** contains the undo commands
** information for undo stored in a global
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	13-mar-87 (mgw)	bug # 11897
**		Added unVarInit() for reseting the global variables for
**		undoing commands so that vfcursor() can call it to
**		prevent AV when you perform the following sequence: edit
**		frame, add field, undo, end frame without saving, re-edit
**		frame, and immediately hit undo key (similar to bug 11897,
**		but not the same). To fix 11897 itself, I moved the
**		exChange()'s from after the big if statement in undoCom()
**		to before it so that spcTrrbld() gets the right endFrame.
**	03/13/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	10-feb-88 (bruceb)
**		Added code for undo's of scrolling field width (type->ftoper)
**		and auxiliary flags (hdr->fhd2flags.)
**	05/24/88 (dkh) - Added user support for scrollable fields.
**	24-apr-89 (bruceb)
**		Removed support for nonseq fields; no longer any such fields.
**	01-sep-89 (cmr)	added bool flag to delFeat() and got rid of references
**			to DETAIL and COLHEAD.
**	07-sep-89 (bruceb)
**		Now that trim can have attributes, undo must save and restore
**		both the text and the attributes.
**	13-dec-89 (bruceb)	Fix for bug 8711.
**		Undo of user editing a field display format will restore
**		type->ftlength.  This restores the datatype length for
**		character fields.
**	18-apr-90 (bruceb)
**		Lint cleanup; removed 'str' arg from call on putEdTitle,
**		and 'mvCode' from call on moveBack.
**	06/12/90 (esd) - Fix what appears to have been a bug at the end
**			 of undoCom: globx was not allowed at the right
**			 edge of the form (it was forced 1 to the right).
**	10/08/90 (dkh) - Fixed bug 33779.
**	06/28/92 (dkh) - Added support for rulers.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      25-sep-96 (mcgem01)
**              Global data moved to vfdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jul-2009 (kschendel) SIR 122138
**	    SPARC solaris compiled 64-bit flunks undo test, fix by properly
**	    declaring exChange (for pointers) and exi4Change (for i4's).
**	1-Sep-2009 (bonro01) SIR 122138
**	    Previous change introduced compile problems because of changed
**	    Prototype for clearLine() and allocLast()
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	"vfunique.h"
# include	"seq.h"			/* added for Bug #1673 fix (nml) */
# include	<er.h>
# include	"ervf.h"

/*
** global structure which holds enough information to undo
** commands
*/
static i4	unCom = unNONE;		/* last command */
static bool	unUndo = FALSE;		/* was an undo done last */
static POS	unOldPos = {0};		/* old pos for last feature changed */
static POS	*unNewPos = NULL;	/* pointer to new position */
static char	*unStr = NULL;		/* string of last object (trim, title)*/
static i4	unAttr = 0;		/* attributes of box or trim */
static SMLFLD	unFld = {0};		/* fd for last field moved */
static FMT	*unFmt	= NULL;		/* format of last field */
static char	*unFmtStr = NULL;
static char	*unVstr = NULL;		/* validation string of last field */
static char	*unVmsg = NULL;		/* val message of last field */
static char	*unFdef = NULL;		/* default value of last field */
static char	unfhdname[FE_MAXNAME + 1] = {0}; /* internal name of a field */
static i4	unFlgs = 0;		/* flags of last field */
static i4	un2Flgs = 0;		/* flags of last field */
static i4	unWdth = 0;		/* scroll width of last field */
static i4	unIntrp = 0;		/* interp code */
static bool	unHadColumn = FALSE;

# ifndef FORRBF
static FIELD	*unfield = 0;		/* undo for a table field */
# endif

/*
** RBF detail lines
*/

static i4	unEndFrame = 0;
static i4	unendxFrm = 0;

# ifndef FORRBF
extern	FIELD	*edtffld;
extern	POS	edtfps;
extern	POS	*ntfps;
# endif

GLOBALREF i4	vfoldundo;	/* added for fix to BUG 1588 (dkh). */
GLOBALREF bool	vfinundo;	/* added for fix to BUG 1587 (dkh). */

/*
** set the command and oldpos for the undo structre
**
** SIDE EFFECTS:
**	changes the command and oldpos fields
**	resets the line table to NORMAL
*/

/* VARARGS2 */
VOID
setGlobUn(com, ps, variant)
i4	com;
POS	*ps;
i4	*variant;
{
	TRIM	*tr;

	clearLine();
	unEndFrame = endFrame;
	unendxFrm = endxFrm;
	noChanges = FALSE;
	unCom = com;
	unUndo = FALSE;
	if (com & unLINE)
		unOldPos.ps_begy = (i4)ps;
	else if (ps != NULL)
		copyBytes(&unOldPos, ps, sizeof(POS));
	/*
	** now we need to fill in the variant information
	*/
	if (com == edBOX)
	{
		unAttr = *(i4 *)ps->ps_feat;
	}
	else if (com == edTRIM)
	{
		tr = (TRIM *) ps->ps_feat;
		unStr = tr->trmstr;
		unAttr = tr->trmflags;
	}
	else if (com == slBMARGIN)
	{
		unEndFrame = (i4) variant;
	}
	else if (com == slRMARGIN)
	{
		unendxFrm = (i4) variant;
	}
# ifndef FORRBF
	else if (com == edTABLE)
	{
		unfield = edtffld;
	}
# endif
	else if ((com & unFIELD) && (com & (unSEL | unEDT)))
	{
		FIELD	*ft;
		FLDHDR	*hdr;
		FLDTYPE *type;

		ft = (FIELD *)ps->ps_feat;
		hdr = FDgethdr(ft);
		type = FDgettype(ft);
		if (com & unEDT)
		{
			unFmt = type->ftfmt;
			unFmtStr = type->ftfmtstr;
			unVstr = type->ftvalstr;
			unVmsg = type->ftvalmsg;
			unFdef = type->ftdefault;
			unFlgs = hdr->fhdflags;
			un2Flgs = hdr->fhd2flags;
			unWdth = type->ftlength;
			unIntrp = hdr->fhintrp;
			STcopy(hdr->fhdname, unfhdname);
		}
		copyBytes(&unFld, (SMLFLD *) variant, sizeof(SMLFLD));
	}
	if (com == slCOLUMN || com == slTRIM || com == slFIELD ||
	    com == slTABLE || com == slTITLE || com == slDATA)
	{	/* get rid of hilighting before copying it */
		vfdisplay(ps, ps->ps_name, FALSE);
	}
	VTundosave(frame);
}

/*
** set the newPos  
*/
VOID
setGlMove(ps)
POS	*ps;
{
	unNewPos = ps;
}

/*
** the user has executed the undo command 
*/

i4
undoCom()
{
	reg	POS	*ps = unNewPos;
	reg	i4	y = unOldPos.ps_begy,
			x = unOldPos.ps_begx;
		char	*cp;
		SMLFLD	oldFd;
		i4	unCode;
		i4	flx,
			flex,
			fley,
			fly;
		FIELD	*fd;
		FLDTYPE *type;
		i4	lnat;

# ifndef FORRBF
		FLDHDR	*hdr;
		char	buf[FE_MAXNAME + 1];
# endif

	if (unCom == unNONE)
	{
		return;
	}
	else if (unCom == dlLINE)
	{
		insLine(unOldPos.ps_begy);
		vfTrigUpd();
		return;
	}
	else if (unCom == opLINE)
	{
		delLine(unOldPos.ps_begy);
		vfTrigUpd();
		return;
	}
	/*
	** basically, if the command is a delete and we are undoing the delete
	** or an enter and we are undoing an undo of the enter,
	** or a select, we don't
	** wish to unlink the position
	*/
	if (	(unUndo && unCom == dlCOLUMN) 
	   || 	(!unUndo && unCom == enCOLUMN)
	   )
	{
		unLinkCol(ps);
	}
	else if 
	(
	      ! (	unCom == slCOLUMN 
		||	unCom == slFIELD 
		|| 	unCom == slBOX
		|| 	unCom == slTRIM 
		|| 	unCom == slTABLE 
		|| 	unCom == slBMARGIN 
		|| 	unCom == slRMARGIN 
		|| 	(!unUndo && (unCom & unDEL)) 
		|| 	(unUndo && (unCom & unENT))
		)
	)
	{
		unLinkPos(ps);
	}
	VTswapimage(frame);

	if (unCom == slBMARGIN)
	{
		vflchg(unEndFrame, TRUE);
		return;
	}
	else if (unCom == slRMARGIN)
	{
		vfwchg(unendxFrm);
		return;
	}

# ifndef FORRBF
	if (unCom == edTABLE || unCom == edBOX)
	{
		copyBytes(&edtfps, &unOldPos, sizeof(POS));
	}
# endif
	copyBytes(&unOldPos, ps, sizeof(POS));
	if (!unUndo)
	{
		unCode = PUNDO;
	}
	else
	{
		unCode = PMOVE;
	}
	/*
	** "vfoldundo" need to fix BUG 1588.
	** Gives the limit of how big the form
	** really will be when the undo is finished.
	** (dkh).
	*/
	vfoldundo = unEndFrame;
	moveBack(unCode, 0);
	exi4Change(&unEndFrame, &endFrame);
	exi4Change(&unendxFrm, &endxFrm);

	if (	unCom == slCOLUMN 
	   || 	unCom == slBOX 
	   || 	unCom == slTRIM 
	   || 	unCom == slFIELD 
	   || 	unCom == slTABLE
	   )
	{
		vfTestDump();
		globy = y;
		globx = x;
	}
	else if (unCom == slTITLE)
	{
		/*
		** (y,x) are where to put the title.
		** they are found in unFld. FIX BUG 553
		*/
		y = unFld.ty;
		x = unFld.tx;
		flx = unFld.dx;
		fly = unFld.dy;
		flex = unFld.dex;
		fley = unFld.dey;
		/*
		** placetitle wants the fd for the current field.
		*/
		FDToSmlfd(&unFld, ps);
		MEcopy((PTR) &unFld, (u_i2) sizeof(SMLFLD), (PTR) &oldFd);
		oldFd.dx = flx;
		oldFd.dy = fly;
		oldFd.dex = flex;
		oldFd.dey = fley;
		placeTitle(ps, &oldFd, y, x, FALSE);
		globy = y;
		globx = x;
	}
	else if (unCom == slDATA)
	{
		/*
		** (y,x) are where to put the data window.
		** they are found in unFld. FIX BUG 553
		*/
		y = unFld.dy;
		x = unFld.dx;
		flx = unFld.tx;
		fly = unFld.ty;
		flex = unFld.tex;
		fley = unFld.tey;
		/*
		** placedata wants the fd for the current field.
		*/
		FDToSmlfd(&unFld, ps);
		MEcopy((PTR) &unFld, (u_i2) sizeof(SMLFLD), (PTR) &oldFd);
		oldFd.tx = flx;
		oldFd.ty = fly;
		oldFd.tex = flex;
		oldFd.tey = fley;
		placeData(ps, &oldFd, y, x, FALSE);
		globy = y;
		globx = x;
	}
	else if (unCom == dlBOX)
	{
		if (unUndo)
		{
			delFeat(ps, FALSE, FALSE);
		}
		else
		{
			insPos(ps, FALSE);
			vfTrigUpd();
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
	else if (unCom == dlTRIM)
	{
		if (unUndo)
		{
			delFeat(ps, FALSE, FALSE);
		}
		else
		{
			vffrmcount(ps, 1);
			insPos(ps, TRUE);
			/*
			** when ps was deleted, it was removed from
			** its column, but the column pointer
			** was left intacted.
			*/
			if (ps->ps_column != NULL)
				insColumn(ps->ps_column, ps);
			reDisplay(ps);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# ifdef FORRBF
	else if (unCom == dlCOLUMN)
	{
		/*
		** a delete of a column is a delete of a field
		*/
		fd = (FIELD *)ps->ps_feat;
		if (unUndo)
		{
			vfcolcount(ps, -1);
			ersCol(ps);
		}
		else
		{
			POS	*col;

			vfcolcount(ps, 1);
			col = ps;
			do {
				col = col->ps_column;
				insPos(col, TRUE);
				reDisplay(col);
			} while (col != ps);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif

# ifndef FORRBF
	else if (unCom == dlTABLE)
	{
		fd = (FIELD *)ps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		if (unUndo)
		{
			vfdelnm(hdr->fhdname, AFIELD);
			delFeat(ps, FALSE, FALSE);
		}
		else
		{
			if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
			{
				vfstinconsist(hdr->fhdname);
			}
			vffrmcount(ps, 1);
			insPos(ps, TRUE);
			vfFlddisp(fd);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif
	else if (unCom == dlFIELD)
	{
		fd = (FIELD *)ps->ps_feat;
# ifndef FORRBF
		hdr = FDgethdr(fd);
# endif
		type = FDgettype(fd);
		if (unUndo)
		{
# ifndef FORRBF
			vfdelnm(hdr->fhdname, AFIELD);
# endif
			delFeat(ps, FALSE, FALSE);
		}
		else
		{
# ifndef FORRBF
			if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
			{
				vfstinconsist(hdr->fhdname);
			}
# endif
			/*
			** when ps was deleted, it was removed from
			** its column, but the column pointer
			** was left intacted.
			*/
			if (ps->ps_column != NULL)
			{
				insColumn(ps->ps_column, ps);
				/*
				** BUG 796 -- see delete.c
				*/
				vfcolcount(ps, 1);
			}
			else
				vffrmcount(ps, 1);
			insPos(ps, TRUE);
			vfFlddisp(fd);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
	else if (unCom == enFIELD)
	{
		fd = (FIELD *)ps->ps_feat;
# ifndef FORRBF
		hdr = FDgethdr(fd);
# endif
		type = FDgettype(fd);
		if (unUndo)
		{
# ifndef FORRBF
			if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
			{
				vfstinconsist(hdr->fhdname);
			}
# endif
			vfinundo = TRUE;	/* added for BUG 1587 (dkh). */
			vffrmcount(ps, 1);
			fitPos(ps, TRUE);
			insPos(ps, TRUE);
			vfersPos(ps);
			vfFlddisp(ps->ps_feat);
			vfinundo = FALSE;	/* added for BUG 1587 (dkh). */
		}
		else
		{
# ifndef FORRBF
			vfdelnm(hdr->fhdname, AFIELD);
# endif
			vffrmcount(ps, -1);

		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# ifndef FORRBF
	else if (unCom == enTABLE)
	{
		fd = (FIELD *)ps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		if (unUndo)
		{
			if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
			{
				vfstinconsist(hdr->fhdname);
			}
			vfinundo = TRUE;	/* added for BUG 1587 (dkh). */
			vffrmcount(ps, 1);
			fitPos(ps, TRUE);
			insPos(ps, TRUE);
			vfersPos(ps);
			vfTabdisp(fd->fld_var.fltblfld, (i4) 0);
			vfinundo = FALSE;	/* added for BUG 1587 (dkh). */
		}
		else
		{
			vfdelnm(hdr->fhdname, AFIELD);
			vffrmcount(ps, -1);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif
	else if (unCom == enBOX)
	{
		if (unUndo)
		{
			insPos(ps, FALSE);
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
	else if (unCom == enTRIM)
	{
		if (unUndo)
		{
# ifdef FORRBF
			if (unHadColumn)	/* Fixed bug #3836 (gac) */
			{
				insColumn(ps->ps_column, ps);
			}
# endif
			frame->frtrimno++;
			putEnTrim(ps, unUndo);
		}
		else
		{
# ifdef FORRBF
			/* Fixed bug #3836 (gac) */
			if (ps->ps_column != NULL)
			{
				unHadColumn = TRUE;
				delColumn(ps->ps_column, ps);
			}
			else
			{
				unHadColumn = FALSE;
			}
# endif
			frame->frtrimno--;
		}
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# ifdef FORRBF
	else if (unCom == enCOLUMN)
	{
		if (unUndo)
		{
			POS	*col;

			vfcolcount(ps, 1);
			col = ps;
			do {
				col = col->ps_column;
				insPos(col, TRUE);
				reDisplay(col);
			} while (col != ps);
		}
		else
		{
			vfcolcount(ps, -1);
		}
		globy = ps->ps_column->ps_begy;
		globx = ps->ps_column->ps_begx;
	}
# endif
# ifndef FORRBF
	else if (unCom == edBOX)
	{
		lnat = unAttr;
		unAttr = *(i4 *)ps->ps_feat;
		copyBytes(ps, &edtfps, sizeof(POS));
		*(i4 *)ps->ps_feat = lnat;
		insPos(ps, FALSE);
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif /* FORRBF */
	else if (unCom == edTRIM)
	{
		cp = unStr;
		lnat = unAttr;
		unStr = ((TRIM *)ps->ps_feat)->trmstr;
		unAttr = ((TRIM *)ps->ps_feat)->trmflags;
		((TRIM *)ps->ps_feat)->trmflags = lnat;
		putEdTrim(ps, cp, FALSE);
		((TRIM *)ps->ps_feat)->trmstr = cp;
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
	else if (unCom == edTITLE)
	{
		FDToSmlfd(&oldFd, ps);
		putEdTitle(ps, &unFld, FALSE);
		copyBytes(&unFld, &oldFd, sizeof(SMLFLD));
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# ifndef FORRBF
	else if (unCom == edTABLE)
	{
		fd = (FIELD *) ps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		vfdelnm(hdr->fhdname, AFIELD);
		copyBytes(ps, &edtfps, sizeof(POS));
		fitPos(ps, TRUE);
		insPos(ps, TRUE);
		fd = (FIELD *)ps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
		{
			vfstinconsist(hdr->fhdname);
		}
		vfTabdisp(fd->fld_var.fltblfld, FALSE);
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif
	else if (unCom == edDATA)
	{
		fd = (FIELD *)ps->ps_feat;
		type = FDgettype(fd);
		FDToSmlfd(&oldFd, ps);
		exi4Change(&type->ftlength, &unWdth);
		exChange(&type->ftfmtstr, &unFmtStr);
		exChange((char **)&type->ftfmt, (char **)&unFmt);
		putEdData(type->ftfmt, ps, &unFld, FALSE);
		copyBytes(&unFld, &oldFd, sizeof(SMLFLD));
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# ifndef FORRBF
	else if (unCom == edATTR)
	{
		/*
		** just exchange attributes
		*/
		fd = (FIELD *)ps->ps_feat;
		hdr = FDgethdr(fd);
		type = FDgettype(fd);
		exChange(&type->ftvalstr, &unVstr);
		exChange(&type->ftvalmsg, &unVmsg);
		exChange(&type->ftdefault, &unFdef);
		exi4Change(&hdr->fhdflags, &unFlgs);
		exi4Change(&hdr->fhd2flags, &un2Flgs);
		exi4Change(&type->ftlength, &unWdth);
		exi4Change(&hdr->fhintrp, &unIntrp);
		vfdelnm(hdr->fhdname, AFIELD);
		STcopy(hdr->fhdname, buf);
		STcopy(unfhdname, hdr->fhdname);
		STcopy(buf, unfhdname);
		if (vfnmunique(hdr->fhdname, TRUE, AFIELD) == FOUND)
		{
			vfstinconsist(hdr->fhdname);
		}
		FDToSmlfd(&oldFd, ps);
		fixField(ps, &oldFd, FALSE, PTITLE);
		globy = ps->ps_begy;
		globx = ps->ps_begx;
	}
# endif
	unUndo = !unUndo;
	if (globy >= endFrame)
		globy = endFrame - 1;
	if (globx > endxFrm)
		globx = endxFrm;
	vfUpdate(globy, globx, TRUE);
}

/*
** exchange the two gizmos they must be pointers
*/
void exChange(char **g1, char **g2)
{
	char *gt;

	gt = *g1;
	*g1 = *g2;
	*g2 = gt;
}

/*
** exchange the two gizmos they must be i4s 
*/
void exi4Change(i4 *g1, i4 *g2)
{
	reg	i4	gt;

	gt = *g1;
	*g1 = *g2;
	*g2 = gt;
}

/*
** clear the ps_mask of all positions stored in the line table
**
*/
void clearLine()
{
	reg	VFNODE	**linep;
	reg	VFNODE	*lp;
	POS		*ps;
	register i4	i;

	for (linep = &line[i = 0]; i <= endFrame; i++, linep++)
	{
		for (lp = *linep; lp != NULL; lp = vflnNext(lp, i))
		{
			ps = lp->nd_pos;
			if (ps->ps_begy == i)
				ps->ps_mask = PNORMAL;
		}

	}
}

VOID
wrOver()
{
	VTundores(frame);
}

VOID
vfsvImage()
{
	VTsvimage(frame);
}

VOID
vfresImage()
{
	VTresimage(frame);
}

void allocLast(frm)
FRAME	*frm;
{
	VTundoalloc(frm);
}


/*
** unVarInit - clear the global variables for undoing commands
**
** History:
**	13-mar-87 (mgw) bug 11897
**		created.
*/
VOID
unVarInit()
{
	/* Bug 11897: clear UNDO.C's global structures for undoing commands */
	unCom = unNONE;
	unUndo = FALSE;
	unNewPos = NULL;
	unStr = NULL;
	unAttr = 0;
	unFmt = NULL;
	unFmtStr = NULL;
	unVstr = NULL;
	unVmsg = NULL;
	unFdef = NULL;
	unFlgs = 0;
	un2Flgs = 0;
	unWdth = 0;
	unIntrp = 0;
	unHadColumn = FALSE;
	MEfill((u_i2) sizeof(POS), (unsigned char) 0, &unOldPos);
	MEfill((u_i2) sizeof(SMLFLD), (unsigned char) 0, &unFld);
	MEfill((u_i2) 20, (unsigned char) 0, &unfhdname[0]);
	/* End Bug 11897 */
}
