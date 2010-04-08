# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<ug.h>
# include	<frsctblk.h>
# include	"vfunique.h"
# include	<cm.h>
# include	"seq.h"
# include	<si.h>
# include	<st.h>
# include	<vt.h>
# include	<er.h>
# include	"ervf.h"
# include	"vfhlp.h"


/*
** enter.c
**
**	Routines which interact with the user while creating
**	new objects on a form as a result of a CREATE menuitem.
**	
**
**	History:
**	03/13/87 (dkh) - Added support for ADTs.
**	16-mar-87 (bruceb)
**		add code to enterData() to turn on/off fdREVDIR flag
**		used by Hebrew project to determine if a field is
**		left-to-right or right-to-left.
**	22-apr-87 (bruceb)
**		When entering data format, set or blank the flag that
**		indicates if right-to-left input and display is
**		specified or not.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/08/87 (dkh) - Usability changes for editing datatypes/formats.
**	08/14/87 (dkh) - ER changes.
**	09/02/87 (dkh) - Fixed jup bug 402.
**	10/02/87 (dkh) - Help file changes.
**	21-jul-88 (bruceb)	Fix for bug 2969.
**		Changes to vfgets and vfEdtStr to specify maximum length.
**	01-jun-89 (bruceb)
**		Removed fdAlloc() routine--not used.
**	08/28/89 (dkh) - Changed to make field titles optional when
**			 creating a new simple field.
**	01-sep-89 (cmr)	added RBF routine insSpecLine() for support of
**			dynamic creation of headers/footers.
**	19-sep-89 (bruceb)
**		Call vfgetSize() with a precision argument.  Done for
**		decimal support.
**	16-oct-89 (cmr)	added RBF routine insnLines().
**	12/05/89 (dkh) - Changed interface to VTcursor() so VTLIB could
**			 be moved in ii_framelib shared lib.
**	12/15/89 (dkh) - VMS shared lib changes.
**	24-jan-90 (cmr)	 Get rid of calls to newLimits(); superseded by
**			 one call to SectionsUpdate() by vfUpdate().
**	12-feb-90 (bruceb)
**		Expand width of form if trim or title would extend past
**		right border.  Fixes unreported bug (it used to 'wrap'.)
**	07-mar-90 (bruceb)
**		Lint:  Add 5th param to FTaddfrs() calls.
**	30-mar-90 (bruceb)
**		Force Vifred menu display back to first menuitem.
**	04/17/90 (dkh) - Fixed us #516 for garnet folks.  Changed
**			 insSpecLine() to call putEnTrim() with a
**			 value of TRUE (instead of FALSE) so that
**			 wrOver() will NOT be called by putEnTrim().
**			 The call to wrOver() was causing the special
**			 trim lines to get wiped out.  Not calling
**			 wrOver() is OK since the undo mechanism is
**			 not enabled for the new RBF layout stuff.
**	09-may-90 (bruceb)
**		Call VTcursor() with new arg indicating whether there is
**		'not' a menu associated with this call.
**	31-may-90 (bruceb)
**		VTcursor call in createField was missing the VFfeature param.
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
**      06/12/90 (esd) - When boxing a field on a 3270, leave space
**                       for an attribute between the box and title/data
**                       on the left as well as the right (bug 21618).
**      06/12/90 (esd) - Add new parameter to call to palloc indicating
**                       whether the feature requires attribute bytes
**                       on each side.
**      06/12/90 (esd) - Add extra parm to VTputstring to indicate
**                       whether form reserves space for attribute bytes
**	21-jul-90 (cmr)	 fix for bug 31759
**			 insnLines - do resetMoveLevel and clearLine here 
**			 instead of in the main LAYOUT/BREAKOPTIONS loop
**			 (in case a Cancel or End is done w/o any changes).
**	03-aug-90 (cmr)  fix for bug 32080 and 32083
**			 Change to previous fix (31759).  Check flag before
**			 doing initialization (should only happen once per
**			 Layout/BreakOptions operation not everytime this
**			 routine is called.
**	04-oct-90 (bruceb)
**		Trim off leading blanks in format strings.
**	12/26/90 (dkh) - Fixed bug 35056.  The 7th parameter was missing
**			 from call to palloc().
**	19-feb-92 (mgw) Bug #40853
**		In buildField(), check for FS_STAR case too, not just
**		FS_MINUS and FS_PLUS
**	06/20/92 (dkh) - Added support for decimal datatype for 6.5.
**	08/24/92 (dkh) - Fixed acc complaints.
**	09/10/92 (dkh) - Added support for rulers in vifred/rbf.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	11/10/93 (dkh) - Fixed bug 55669.  Straight edges will be updated
**			 correctly when rbf creates a new section by calling
**			 insnLines().
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


/*
** VFBUFSIZ and VFTITLEBSIZ had better match values in other routines,
** both in Vifred and VT code, and furthermore, had better match storage
** space in the ii_trim and ii_fields catalogs.
*/
# define	VFBUFSIZ	150
# define	VFTITLEBSIZ	50

FUNC_EXTERN	i4	vfnmunique();
FUNC_EXTERN	STATUS	IImumatch();
FUNC_EXTERN	STATUS	fmt_vfvalid();
FUNC_EXTERN	STATUS	fmt_ftot();
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	STATUS	afe_cancoerce();
FUNC_EXTERN	i4	VFfeature();


GLOBALREF	FT_TERMINFO	IIVFterminfo;
GLOBALREF	FILE		*vfdfile;

/*
** global structures for enter field
*/

# ifndef FORRBF
static REGFLD	newReg = {0};

# ifdef WSC
/*
**  Whitesmiths C permits the initialization of unions
**  (contra section 8.6 of the C Reference Manual in K & R).
**  It also demands that all external data, including unions,
**  be initialized (also contra sec. 8.6).
*/
static FIELD	newField ZERO_FILL;
# else
static FIELD	newField;
# endif

static FLDHDR	*newHdr = &newReg.flhdr;
static FLDTYPE	*newType = &newReg.fltype;
static FLDVAL	*newVal = &newReg.flval;
static POS	newTitle = {0};
static POS	newData = {0};
static bool	titleDone = FALSE;
static bool	dataDone = FALSE;
static bool	attrDone = FALSE;
# endif


/*{
** Name:	createBox	- create a new box and put in line table  
**
** Description:
**	Supervise user interaction to create a new box/line. 
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects: 
**
** History:
**	04-04-88 (tom) created
**	6-Jul-2009 (kschendel) SIR 122138
**	    SPARC solaris compiled 64-bit flunks undo test, fix by properly
**	    declaring exChange (for pointers) and exi4Change (for i4's).
*/
createBox()
{
	i4		startx;
	i4		starty;
	i4		endx;
	i4		endy;
	POS		*ps;
	i4		poscount;
	FLD_POS		*posarray = NULL;
	FRS_EVCB	evcb;

	startx = globx;
	starty = globy;

	scrmsg(ERget(F_VF0047_Pos_opp_corner));

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);
	evcb.event = fdNOCODE;

	FTclrfrsk();

	vfposarray(&poscount, &posarray);

	while (VTcursor(frame, &globy, &globx, endFrame - 1, endxFrm, poscount,
	    posarray, &evcb, VFfeature, RBF, FALSE, TRUE, FALSE, TRUE)
	    != fdopMENU)
	{
		;
	}

	if (starty != globy ||startx != globx) /* if not on the same point */
	{
		endy = globy;
		endx = globx;

		/* must get rid of flash plus before calling GlobUn so 
		   that GlobUn doesn't save the plus sign in the image
		   and then restore it when it saves the screen image */
		VTflashplus(frame, 0, 0, 0);

		setGlobUn(enBOX, (POS *) NULL, NULL);

		/* special case: since boxes do move things around
		   when they are created (fitPos action) we must handle
		   the bottom margin feature seperately */
		if (starty == endFrame)
		{
			/* 2 will give 1 blank line */
			vfMoveBottomDown(2);
		}

		if (starty > endy)	/* adjust orientation */
			exi4Change(&starty, &endy);
		if (startx > endx)
			exi4Change(&startx, &endx);

		ps = insBox(starty, startx, 
			endy - starty + 1, endx - startx + 1, 0);

		setGlMove(ps);

	} 
	else
	{
		VTflashplus(frame, 0, 0, 0);
	}
	MEfree(posarray);
}

/*{
** Name:	createLine	- create a line  (user invoked)
**
**
** Inputs:
**	none
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**		The line table is expanded, all items below are moveed
**		down.
**
** History:
**	04-04-88  (tom) - moved from delete.c ????
*/
VOID 
createLine()
{
	insLine(globy);
}


/*{
** Name:	createTrim	- create a piece of trim  (user invoked)
**
**
** Inputs:
**	none
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04-04-88  (tom) - renamed from enterTrim
*/
VOID
createTrim()
{
	i4	trx,
		try;
	i4	len;
	i4	diff;
	TRIM	*tr;
	POS	*ps;
	u_char	*str;

	scrmsg(ERget(F_VF0021_Enter_trim));
	trx = globx;
	try = globy;
	vfUpdate(try, trx, TRUE);
	vfsvImage();
	vfTestDump();
	str = vfgets(VFBUFSIZ);
	if (str == NULL || *str == '\0')
	{
		vfresImage();
		return;
	}
	vfresImage();
	setGlobUn(enTRIM, (POS *) NULL, NULL);
	tr = trAlloc(try, trx, (u_char *) saveStr(str));
	len = STlength((char *) str);
	ps = palloc(PTRIM, try, trx, 1, len, (i4 *) tr, IIVFsfaSpaceForAttr);
	vffrmcount(ps,  1);
	if ((diff = globx + len - 1 - endxFrm) > 0)
	{
	    vfwider(frame, diff);
	}
	putEnTrim(ps, FALSE);
}
#ifdef FORRBF
VOID
insSpecLine(x, y, str)
i4	x,y;
char	*str;
{
	TRIM	*tr;
	POS	*ps;
	char	*sp;

	if ((sp = (char *)MEreqmem((u_i4)0, (u_i4)(endxFrm + 2), TRUE,
		    (STATUS *)NULL)) == NULL)
  	{
	    IIUGbmaBadMemoryAllocation(ERx("insSpecLine"));
    	}
	spcBldtr(str, sp);
	tr = trAlloc(y, x, (u_char *) saveStr(sp));
	ps = palloc(PSPECIAL, y, x, 1, STlength(sp), (i4 *) tr, FALSE);
	vffrmcount(ps,  1);

	/*
	**  Pass in a value of TRUE for unUndo to putEnTrim() since
	**  we don't want to call wrOver() to wipe out the special
	**  trim lines.
	*/
	putEnTrim(ps, TRUE);
}
#endif
	

VOID
putEnTrim(ps, unUndo)
register POS	*ps;
bool		unUndo;
{

	if (!unUndo)
		wrOver();
	fitPos(ps, TRUE);
	insPos(ps, TRUE);

	vfTestDump();
}

/*{
** Name:	createField	- create a field (user invoked)
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
*/

# ifndef FORRBF

VOID
createField()
{
	i4		com;
	i4		starty = globy,
			startx = globx;
	char		*cp;
	i4		oendxfrm;
	i4		poscount;
	FLD_POS		*posarray = NULL;
	FRS_EVCB	evcb;
	DB_DATA_VALUE	dbv;
	ADF_CB		*ladfcb;
	bool		valid;
	bool		is_str;

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);

	newField.fltag = FREGULAR;
	newField.fld_var.flregfld = &newReg;
	MEfill(sizeof(FLDHDR), '\0', newHdr);
	MEfill(sizeof(FLDTYPE), '\0', newType);
	MEfill(sizeof(FLDVAL), '\0', newVal);
	newHdr->fhdname[0] = '\0';
	newType->ftdefault = saveStr(ERx("\0"));
	newHdr->fhseq = NEWFLDSEQ;
	newType->ftvalstr = saveStr(ERx("\0"));
	newType->ftvalmsg = saveStr(ERx("\0"));
	newType->ftdatatype = DB_NODT;
	titleDone = FALSE;
	dataDone = FALSE;
	oendxfrm = endxFrm;
	vfsvImage();

	ladfcb = FEadfcb();

	/* Force the next FTputmenu() call to re-construct the menu. */
	entField->mu_flags |= MU_NEW;

	for (;;)
	{
		evcb.event = fdNOCODE;
		FTclrfrsk();
		FTaddfrs(9, UNDO, NULL, 0, 0);
		FTaddfrs(1, HELP, NULL, 0, 0);
		FTaddfrs(3, DONE, NULL, 0, 0);
		vfmu_put(entField);
		vfposarray(&poscount, &posarray);
		if (VTcursor(frame, &globy, &globx, endFrame,
		    endxFrm + 1 + IIVFsfaSpaceForAttr,
		    poscount, posarray, &evcb, VFfeature, RBF, TRUE, FALSE,
		    FALSE, FALSE) == fdopSCRLT)
		{
			vfwider(frame, IIVFterminfo.cols/4);
			continue;
		}
		if (globx > endxFrm)
		{
			globx = endxFrm;
		}
		VTdumpset(vfdfile);

		if (evcb.event == fdopFRSK)
		{
			com = evcb.intr_val;
		}
		else if (evcb.event == fdopMUITEM)
		{
			if (IImumatch(entField, &evcb, &com) != OK)
			{
				continue;
			}
		}
		else
		{
			/*
			**  Fix for BUG 7123. (dkh)
			*/
			com = FTgetmenu(entField, &evcb);
			if (evcb.event == fdopFRSK)
			{
				com = evcb.intr_val;
			}
		}

		VTdumpset(NULL);
		if (com == QUIT)
			break;
		switch (com)
		{
		case HELP:
			vfhelpsub(VFH_NEWFLD,
				ERget(S_VF004D_Create_Field_Submenu),
				entField);
			break;

		case DONE:
			if (dataDone)
			{
				if (attrDone &&
					newType->ftdatatype != DB_NODT)
				{
					dbv.db_datatype =
						newType->ftdatatype;
					dbv.db_length =
						newType->ftlength;
					dbv.db_prec =
						newType->ftprec;
					if (fmt_vfvalid(ladfcb,
						newType->ftfmt,
						&dbv, &valid,
						&is_str) != OK ||
						!valid)
					{
					    IIUGerr(E_VF004E_no_compat,
						UG_ERR_ERROR, 0);
						break;
					}
					if (is_str)
					{
						newType->ftlength =
							dbv.db_length;
					}
					else if (abs(newType->ftdatatype) ==
						DB_DEC_TYPE)
					{
						newType->ftlength = dbv.db_length;
						newType->ftprec = dbv.db_prec;
					}
				}
				cp = newHdr->fhdname;
				if (!vffnmchk(cp,ERget(F_VF0011_field)))
				{
					break;
				}
				if (vfnmunique(newHdr->fhdname, TRUE,
					AFIELD) == FOUND)
				{
					IIUGerr(E_VF0052_name,
						UG_ERR_ERROR, 1,
						newHdr->fhdname);
					break;
				}

				/*
				**  Check format and datatype
				**  compatibility.
				*/

				vfresImage();

				/*
				**  Fix for BUG 7812. (dkh)
				*/
				buildField(oendxfrm);

				MEfree(posarray);
				return;
			}
			else if (titleDone)
			{
				IIUGerr(E_VF0053_enter_a_fmt,
					UG_ERR_ERROR, 0);
			}
			else if (dataDone)
			{
				IIUGerr(E_VF0054_enter_a_title,
					UG_ERR_ERROR, 0); 
			}
			else if (attrDone)
			{
				IIUGerr(E_VF0055_title_fmt,
					UG_ERR_ERROR, 0);
			}
			else
			{
				vfresImage();
				MEfree(posarray);
				return;
			}
			break;

		case UNDO:
			vfresImage();
			globy = starty;
			globx = startx;
			if (globy >= endFrame)
				starty = endFrame - 1;
			vfTestDump();
			MEfree(posarray);
			return;
		}
	}
	MEfree(posarray);
}
# endif

/*
** enter a title for a field
*/

# ifndef FORRBF
i4
enterTitle()
{
	register i4	i;
	i4		tx,
			ty;
	register u_char *str,
			*cp;

	if (titleDone)
	{
		IIUGerr(E_VF0056_A_title_has_already_b, UG_ERR_ERROR, 0);
		return;
	}
	titleDone = TRUE;
	tx = globx;
	ty = globy;
	newTitle.ps_begx = tx; newTitle.ps_endx = tx;
	newTitle.ps_begy = ty; newTitle.ps_endy = ty;
	if (dataDone)
	{
		if (inRegion(&newTitle, &newData)
		    && ty >= newData.ps_begy && ty <= newData.ps_endy)
		{
			IIUGerr(E_VF0057_Cant_put_title_here, UG_ERR_ERROR, 0);
			titleDone = FALSE;
			return;
		}
	}
	scrmsg(ERget(F_VF0022_Enter_title));
	VTxydraw(frame, globy, globx);
	vfTestDump();
	str = vfgets(VFTITLEBSIZ);
	if (STcompare(str, ERx("")) == 0)
	{
		titleDone = FALSE;
		return;
	}
	newHdr->fhtitle = saveStr(str);
	newTitle.ps_endx = tx + STlength((char *) str) - 1;
	if (dataDone)
	{
		if (inRegion(&newTitle, &newData)
		   && ty >= newData.ps_begy && ty <= newData.ps_endy)
		{
			IIUGerr(E_VF0059_Title_Overlaps_fmt, UG_ERR_ERROR, 0);
			titleDone = FALSE;
			return;
		}
	}
	else
	{
		if (globx + STlength((char *) str) + 1 < endxFrm + 1)
		{
			globx += STlength((char *) str)+1;
		}
		else if (globy+1 < endFrame)
		{
			globy++;
		}
	}
	newHdr->fhtitx = tx;
	newHdr->fhtity = ty;
	cp = (u_char *) newHdr->fhdname;
	if (*cp != '\0')
		return;
	for (i = 0; i < FE_MAXNAME && *str;/* cp++,*/ CMnext(str))
	{
		if (CMnmchar(str))
		{
			CMbyteinc(i, str);
			if (i <= FE_MAXNAME)
			{
				CMtolower(str, cp);
				CMnext(cp);
			}
		}
	}
	*cp = '\0';
}
# endif

/*
** enter a data window for the new field
*/

# ifndef FORRBF
i4
enterData()
{
	i4	dx;
	i4	dy;
	i4	diff;
	i4	x;
	i4	y;
	i4	dum;
	u_char	*str;
	bool	reversed;

	if (dataDone)
	{
		IIUGerr(E_VF010F_A_display_format_has, UG_ERR_ERROR, 0);
		return;
	}

	dx = globx;
	dy = globy;
	newData.ps_begx = dx; newData.ps_endx = dx;
	newData.ps_begy = dy; newData.ps_endy = dy;

	if (titleDone)
	{
		if (inRegion(&newData, &newTitle) && newTitle.ps_begy == dy)
		{
			IIUGerr(E_VF005A_Cant_put_fmt, UG_ERR_ERROR, 0);
			dataDone = FALSE;
			return;
		}
	}
	scrmsg(ERget(F_VF0023_Enter_format));
	vfUpdate(dy, dx, TRUE);
	vfTestDump();
	{
		/*
		** NOTE:  Current length for data format specification is
		** 0 (default).  May later wish to make this 50, in keeping
		** with the storage space in ii_fields (which is less than
		** that allowed in ii_encoded_forms.)
		*/
		if (dataDone)
		{
			VTdelstring(frame, dy, dx, 
				STlength(newType->ftfmtstr));
			str = vfEdtStr(frame, newType->ftfmtstr, dy, dx, 0);
		}
		else
		{
			str = vfgets(0);
		}
		if (STcompare(str, ERx("")) == 0)
		{
			if (dataDone)
			{
				newType->ftfmt = NULL;
				newType->ftfmtstr = NULL;
			}
			dataDone = FALSE;
			return;
		}
		str = (u_char *)STskipblank((char *)str, STlength((char *)str));
		/* str won't be NULL since the above STcompare was done. */
		while ((newType->ftfmt = vfchkFmt(str)) == NULL)
		{
			IIUGerr(E_VF0044_not_legal_fmt, UG_ERR_ERROR, 1, str);
			scrmsg(ERget(F_VF0023_Enter_format));
			VTdelstring(frame, dy, dx, STlength((char *) str));
			VTxydraw(frame, dy, dx);
			/* See NOTE above */
			str = vfgets(0);
			if (STcompare(str, ERx("")) == 0)
			{
				if (dataDone)
				{
					newType->ftfmt = NULL;
					newType->ftfmtstr = NULL;
				}
				dataDone = FALSE;
				return;
			}
		}
		if (dataDone)
		{
			VTputstring(frame, dy, dx, str, 0, (bool) FALSE,
			    IIVFsfaSpaceForAttr);
		}
		/*
		** Ignore return value since only checks are that second and
		** third args aren't NULL, and here that's guaranteed.
		** Just using to see if reversed.
		*/
		_VOID_ fmt_isreversed(FEadfcb(), newType->ftfmt, &reversed);
		if (reversed)
			newHdr->fhdflags |= fdREVDIR;
		else
			newHdr->fhdflags &= ~fdREVDIR;
		newType->ftdatax = dx;
		newVal->fvdatay = dy;
		newType->ftfmtstr = vfsaveFmt((str));
		vfgetSize(newType->ftfmt, 0, 0, 0, &y, &x, &dum);
		diff = dx + x - 1 - endxFrm;
		if (diff > 0)
		{

			vfwider(frame, diff);
		}
	}
	newData.ps_endx = dx + x - 1;
	newData.ps_endy = dy + y - 1;
	if (titleDone)
	{
		if (inRegion(&newData, &newTitle)
		    && newTitle.ps_begy >= newData.ps_begy
		    && newTitle.ps_begy <= newData.ps_endy)
		{
			if (dataDone)
			{
				newType->ftfmt = NULL;
				newType->ftfmtstr = NULL;
				VTdelstring(frame, dy, dx, 
					STlength((char *) str));
			}
			IIUGerr(E_VF005A_Cant_put_fmt, UG_ERR_ERROR, 0);
			dataDone = FALSE;
			return;
		}
	}
	else
	{
		if (globx > 0)
		{
			globx = max(0, globx - 12);
		}
		else if (globy > 0)
		{
			globy--;
		}
	}
	dataDone = TRUE;
}
# endif


/*
** enter attributes
*/

# ifndef FORRBF

i4
enterAttr()
{
	nattrCom(&newField, FALSE);
	attrDone = TRUE;
}
# endif

/*
** build a new field from the global stuff we have
*/

# ifndef FORRBF
void
buildField(oendxfrm)
i4	oendxfrm;
{
	i4		y;
	i4		x;
	i4		fx;
	i4		fy;
	i4		fex;
	i4		fey;
	i4		extent;
	FIELD		*fld;
	REGFLD		*fd;
	REGFLD		*nf;
	POS		*ps;
	SMLFLD		smlfd;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	FLDVAL		*val;
	i4		spacesize;
	i4		tendxfrm = 0;
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	fltdbv;
	ADF_CB		*ladfcb;
	char		*fmtstr;
	char		fmtbuf[100];
	bool		numeric;
	i4		diff;
	bool		attr_reqd;

	spacesize = IIVFsfaSpaceForAttr;

	ladfcb = FEadfcb();
	fltdbv.db_datatype = DB_FLT_TYPE;

	fld = FDnewfld(frame, 1, FREGULAR);
	fd = fld->fld_var.flregfld;
	nf = &newReg;
	copyBytes((char *) fd, (char *) nf,  sizeof(REGFLD));
	hdr = FDgethdr(fld);
	type = FDgettype(fld);
	val = FDgetval(fld);

	if ((diff = newTitle.ps_endx - endxFrm) > 0)
	{
	    vfwider(frame, diff);
	}

	/*
	**  Fix for BUG 7812. (dkh)
	*/
	if (oendxfrm != endxFrm)
	{
		tendxfrm = endxFrm;
		endxFrm = oendxfrm;
	}

	setGlobUn(enFIELD, (POS *) NULL, NULL);

	/*
	**  Fix for BUG 7812. (dkh)
	*/
	if (tendxfrm)
	{
		endxFrm = tendxfrm;
	}

	/*
	**  If no datatype already specified, then get the default
	**  based on the specified format.  We are assuming that
	**  if ftdatatype is DB_NODT, then no datatype information
	**  was specified, directly or indirectly.
	*/
	if (type->ftdatatype == DB_NODT)
	{
		if (fmt_ftot(FEadfcb(), type->ftfmt, &dbv) != OK)
		{
			syserr(ERget(S_VF005C_Inconsistency_while_c));
		}
		type->ftlength = dbv.db_length;
		type->ftdatatype = dbv.db_datatype;
		type->ftprec = dbv.db_prec;
	}
# ifdef JUNK
	else if (abs(type->ftdatatype) == DB_DEC_TYPE)
	{
		/*
		**  If a decimal datatype, need to set the precision/scale
		**  from the size of the field.
		**
		**  We've already done
		*/
		(void) fmt_vfvalid(ladfcb, type->ftfmt, &dbv, &valid, &is_str);
	}
# endif
	else
	{
		dbv.db_datatype = type->ftdatatype;
	}

	_VOID_ afe_cancoerce(ladfcb, &dbv, &fltdbv, &numeric);
	if (numeric)
	{
		/*
		**  Add a leading sign.	 Don't need to check
		**  return value for following fmt calls since
		**  things should be all checked out by now.
		*/
		fmtstr = type->ftfmtstr;
		/* For bug 40853 - add FS_STAR case */
		if (!RBF && *fmtstr != '-' && *fmtstr != '+' && *fmtstr != '*')
		{
			STprintf(fmtbuf, ERx("-%s"), fmtstr);
			fmtstr = fmtbuf;
			if ((type->ftfmt = vfchkFmt(fmtstr)) == NULL)
			{
				syserr(ERget(S_VF005D_Could_not_build_forma),
					TRUE);
			}
			type->ftfmtstr = saveStr(fmtstr);
		}
	}

	vfgetSize(type->ftfmt, type->ftdatatype, type->ftlength,
		type->ftprec, &y, &x, &extent);
	if (hdr->fhtitle == NULL)
	{
		hdr->fhtitle = ERx("");
		fx = hdr->fhtitx = type->ftdatax;
		fy = hdr->fhtity = val->fvdatay;
		fex = type->ftdatax + x - 1;
		fey = val->fvdatay + y - 1;
	}
	else
	{
		fx = min(hdr->fhtitx, type->ftdatax);
		fy = min(hdr->fhtity, val->fvdatay);
		fex = max(hdr->fhtitx + STlength(hdr->fhtitle) - 1,
			type->ftdatax + x - 1);
		fey = max(hdr->fhtity, val->fvdatay + y - 1);
	}
	attr_reqd = spacesize;
	if (hdr->fhdflags & fdBOXFLD)
	{
		if (fy == 0 || fx <= spacesize || fex >= endxFrm - spacesize)
		{
			IIUGerr(E_VF000B_Cant_box_this_field, UG_ERR_ERROR, 0);
			hdr->fhdflags &= (~fdBOXFLD);
		}
		else
		{
			fx  -= 1 + spacesize;
			fex += 1 + spacesize;
			fy--;
			fey++;
			attr_reqd = FALSE;
		}
	}
	ps = palloc(PFIELD, fy, fx, fey - fy + 1, fex - fx + 1,
		(i4 *) fld, attr_reqd);
	wrOver();

	fitPos(ps, TRUE);
	hdr->fhposx = fx;
	hdr->fhposy = fy;
	hdr->fhmaxx = fex - fx + 1;
	hdr->fhmaxy = fey - fy + 1;
	type->ftwidth = extent;

	type->ftdataln = x;
	insPos(ps, TRUE);
	val->fvdatay -= fy;
	type->ftdatax -= fx;
	hdr->fhtity -= fy;
	hdr->fhtitx -= fx;
	FDToSmlfd(&smlfd, ps);
	vfFlddisp(fld);
	vffrmcount(ps, 1);
	vfUpdate(fy, fx, TRUE);
	vfTestDump();
}
# endif

/*
** allocate a trim structure
*/
TRIM *
trAlloc(y, x, str)
i4	y,
	x;
u_char	*str;
{
	TRIM	*tr;

	if ((tr = (TRIM *)FEreqmem((u_i4)0, (u_i4)sizeof(struct trmstr), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("trAlloc"));
	}
	tr->trmy = y;
	tr->trmx = x;
	tr->trmstr = (char *) str;
	return (tr);
}

/*
** get a string from the user
*/
u_char *
vfgets(maxlen)
i4	maxlen;
{
	static	u_char	result[VFBUFSIZ + 1];
	u_char		buf[VFBUFSIZ + 1];

	/* clearly, maxlen had better be less than VFBUFSIZ */
	VTgetstring(frame, globy, globx, endxFrm, ERx(""), buf,
		VT_GET_NEW, maxlen);
	STcopy(buf, result);
	vfTestDump();
	return (result);
}


# ifdef FORRBF
VOID
insnLines( y, numlines, initialized )
i4	y, numlines;
bool	initialized;
{
	register VFNODE	*lt;
	register POS	*ps;

	if (!initialized)
	{
		resetMoveLevel();
		clearLine();
	}

	/*
	**  Remove cross hairs before inserting new lines
	**  for a new rbf section.
	*/
	if (IIVFxh_disp)
	{
		vfersPos(IIVFhcross);
		vfersPos(IIVFvcross);
		unLinkPos(IIVFhcross);
		unLinkPos(IIVFvcross);
	}

	while ( numlines-- )
	{
		nextMoveLevel();
		doInsLine( y );
	}

	/*
	**  Redisplay cross hairs if necessary.
	*/
	if (IIVFxh_disp)
	{
		IIVFvcross->ps_endy = endFrame - 1;
		if (IIVFhcross->ps_begy >= endFrame)
		{
			IIVFhcross->ps_begy = IIVFhcross->ps_endy =
                                endFrame - 1;
		}
		insPos(IIVFhcross, FALSE);
		insPos(IIVFvcross, FALSE);
	}
}
#endif
VOID
insLine(y)
i4	y;
{
	i4	delhcross = FALSE;
	register VFNODE	*lt;
	register POS	*ps;

	if (IIVFxh_disp && IIVFhcross->ps_begy >= y)
	{
		delhcross = TRUE;
		vfersPos(IIVFhcross);
		unLinkPos(IIVFhcross);
	}

	if (IIVFxh_disp)
	{
		vfersPos(IIVFvcross);
		unLinkPos(IIVFvcross);
	}

	for (lt = line[y]; lt != NNIL; lt = vflnNext(lt, y))
	{
		ps = lt->nd_pos;
		if (ps->ps_begy != y)
		{
			IIUGerr(E_VF003C_Cant_insert_line_ins, UG_ERR_ERROR, 0);
			if (IIVFxh_disp)
			{
				if (delhcross)
				{
					insPos(IIVFhcross, FALSE);
				}
				insPos(IIVFvcross, FALSE);
			}
			return;
		}
	}

	setGlobUn(opLINE, (POS *) y, 0);
	setGlMove((POS *) NULL);

	doInsLine(y);

	if (IIVFxh_disp)
	{
		if (delhcross)
		{
			if (IIVFhcross->ps_begy >= endFrame)
			{
				IIVFhcross->ps_begy = IIVFhcross->ps_endy =
					endFrame - 1;
			}
			insPos(IIVFhcross, FALSE);
		}

		IIVFrvxRebuildVxh();
	}
}


/*{
** Name:	doInsLine	- do line insert, no checks, no undo
**
**
** Description:
**		Called from insLine and also from form resize code.
**		This is a utility function which should only affect
**		the line table and related structures. It is important
**		that it not update the screen, nor post undo structures.
**
** Inputs:
**	y	- line number we want to insert before.
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/28/88 (tom) - written
*/
VOID
doInsLine(y)
i4	y;
{
	register VFNODE **lp;
	register VFNODE **nlp;
	register i4	i;

	vfnewLine(1);
	nlp = &line[endFrame+1];
	lp = nlp - 1;
	for (i = endFrame + 1; i > y; i--)
	{
		adjustPos(*lp, 1, i);
		*nlp-- = *lp--;
	}
	*nlp = NULL;
	newFrame(1);
}
