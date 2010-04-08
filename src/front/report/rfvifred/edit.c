/*
** edit.c
** contains edit commands
** Written (10/12/81)
**
**	Notes: (10/26/83) - put in changes made to fix BUG #1673
**			    (nml).
** Copyright (c) 2004 Ingres Corporation
**
**  History:
**	12/31/86 (dkh) - Added support for new activations.
**	03/14/87 (dkh) - Added support for ADTs.
**	22-apr-87 (bruceb)
**		When editing data format, set or blank the flag that
**		indicates if right-to-left input and display is
**		specified or not.
**	08/08/87 (dkh) - Usability changes for editing datatypes/formats.
**	08/14/87 (dkh) - ER changes.
**	10/02/87 (dkh) - Help file changes.
**	10/25/87 (dkh) - Error message cleanup.
**	05/18/88 (dkh) - Added user support for scrollable fields.
**	21-jul-88 (bruceb)	Fix for bug 2969.
**		Changes to vfEdtStr to specify maximum length.
**	08/02/88 (tom) - added test for overlapping data/title
**	10/05/88 (tom) - repaint screen message after 0045 error.
**	07-apr-89 (bruceb)
**		editData() modified to disallow display formats for restricted
**		fields (marked by VQ) that would change the field datatype.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	07-sep-89 (bruceb)
**		Editing trim now can involve editing trim attributes.  When
**		trim is placed on the screen, use those attributes.
**	19-sep-89 (bruceb)
**		Call vfgetSize() with a precision argument.  Done for decimal
**		support.
**	21-sep-89 (bruceb)
**		Trim attributes are only desired for Vifred, not RBF.
**	12/15/89 (dkh) - VMS shared lib changes.
**	12-feb-90 (bruceb)
**		Expand width of form if trim or title would extend past
**		right border.  Fixes unreported bug (it used to 'wrap'.)
**	07-mar-90 (bruceb)
**		Lint:  Add 5th param to FTaddfrs() calls.
**	13-mar-90 (bruceb)
**		Ifndef FORRBF out the editing of box trim in RBF as this
**		resolves some undefined symbols on the link.  (And, such
**		editing isn't used anyway.)
**	18-apr-90 (bruceb)
**		Lint cleanup; removed 'str' arg from putEdTitle.
**	6/1/90 (martym)
**		Put in support for break column options in RBF.
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**      06/12/90 (esd) - Add extra parm to VTputstring to indicate
**                       whether form reserves space for attribute bytes
**	07/05/90 (dkh) - Updated with interface change to IIVTlf().
**	16-jul-90 (bruceb)	Fix for bug 31639.
**		When determining if edited trim requires wider form, use
**		tr->trmx instead of globx--might be positioned at end of trim.
**	21-jul-90 (cmr)
**		Pass bool noUndo to IIRFSetBrkOptions() in case Report Layout
**		changes.  Make Undo a NOP if noUndo is TRUE.
**	22-aug-90 (bruceb)	(Similar to bug 31639.)
**		When determining if edited title requires wider form, use
**		fd->tx instead of globx--might be at end of title.
**	04-oct-90 (bruceb)
**		Trim off leading blanks in format strings.
**	01/11/91 (dkh) - Fixed bug 35271.
**	06/28/92 (dkh) - Added support for rulers in vifred/rbf.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2004 (somsa01)
**	    Added LEVEL1_OPTIM for i64_win to prevent SEGV in RBF from test
**	    rbf019.sep.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	"decls.h"
# include	<frsctblk.h> 
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	"ervf.h"
# include	"vfhlp.h"

# ifdef i64_win
# pragma optimize("s", on)
# endif

/*
** The definitions of VFBUFSIZ and VFTITLEBSIZ must match those of other
** code in Vifred and VT, and also column sizes in ii_trim and ii_fields.
*/
# define      VFBUFSIZ        150
# define      VFTITLEBSIZ     50

/*
** routines communicate through some global variables which describe
** the position currently on and the actual type of object it is
*/
static POS	*editPos ZERO_FILL;		/* current feature */

static SMLFLD	editFd	 ZERO_FILL;

GLOBALDEF	i4	vfedtxpand = 0;

GLOBALREF	FT_TERMINFO	IIVFterminfo;
GLOBALREF	FILE		*vfdfile;

FUNC_EXTERN	ADF_CB		*FEadfcb();
FUNC_EXTERN	bool		fmt_justify();
FUNC_EXTERN	VOID		IIAFddcDetermineDatatypeClass();

# ifdef FORRBF
FUNC_EXTERN	bool		IIRFIsBreakColumn();
FUNC_EXTERN	STATUS		IIRFSetBrkOptions();
FUNC_EXTERN	STATUS		IIRFSetNewPageOption();
FUNC_EXTERN 	Sec_node	*whichSec();
# endif

/*
** edit command has been entered
** Determine what is being edited then allow movement
*/

i4
edit2com()
{
	MENU	mu;
	i4	menu_val;
	FRS_EVCB evcb;
	FIELD   *ft;
#ifdef FORRBF
	bool	noUndo = FALSE;
#endif

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);
	editPos = onPos(globy, globx);
	if (editPos == NULL)
	{
		IIUGerr(E_VF003D_Can_only_select_the_E, UG_ERR_ERROR, 0);
		return;
	}

# ifdef FORRBF
	/*
	** See if need to expose the 'BreakOptions' menu option:
	*/
	if (editPos->ps_name == PFIELD)
	{
		ft = (FIELD *) editPos->ps_feat;
		if (IIRFIsBreakColumn(ft))
		{
			mu = rbfedFldMu2;
		}
		else
		{
			mu = rbfedFldMu1;
		}
	}
# else
	mu = edFldMu;
# endif
	switch (editPos->ps_name)
	{
	case PFIELD:
		FDToSmlfd(&editFd, editPos);

		for (;;)
		{
			FTclrfrsk();
			FTaddfrs(1, HELP, NULL, 0, 0);
			FTaddfrs(3, UNDO, NULL, 0, 0);
			vfmu_put(mu);
			VTdumpset(vfdfile);

			evcb.event = fdNOCODE;
			/*
			**  Fix for BUG 7123. (dkh)
			*/
			menu_val = FTgetmenu(mu, &evcb);
			if (evcb.event == fdopFRSK)
			{
				menu_val = evcb.intr_val;
			}
			VTdumpset(NULL);
			switch (menu_val)
			{
				case HELP:
# ifdef FORRBF
					vfhelpsub(RFH_DETAIL,
					 ERget(S_VF003E_Editing_Detail),
					 mu);
# else
					vfhelpsub(VFH_EDFLD,
					  ERget(S_VF003F_Editing_Field),
					  mu);
# endif
					continue;

				case ediTITLE:
				    scrmsg(ERget(F_VF0019_Edit_title));
					editTitle(editPos, &editFd);
					break;

				case ediDATA:
				    scrmsg(ERget(F_VF001A_Edit_format));
					editData(editPos, &editFd);
					break;

				case ediATTR:
# ifdef FORRBF
					{
						FLDHDR	*hdr;

						hdr = FDgethdr(
							editPos->ps_feat);
						noChanges &= 
							(bool) !rFcoptions();
						FTclear();
					}
# else
					{
						editAttr(editPos, &editFd);
					}
# endif
					break;

				case ediBREAK:
# ifdef FORRBF
					IIVTlf( frame, TRUE, FALSE );
					if (((whichSec(globy, &Sections))->sec_type == SEC_DETAIL))
					{
						_VOID_ IIRFSetBrkOptions(ft, &noUndo);
					}
					else
					{
						_VOID_ IIRFSetNewPageOption(ft, &noUndo);
					}
					IIVTlf( frame, FALSE, FALSE );
					if (noUndo)
					{
						unVarInit();
						noUndo = FALSE;
					}
					FTclear();
# endif
					break;

				case UNDO:
					break;
			}
			break;
		}
		break;

	case PTRIM:
# ifdef	FORRBF
		setGlobUn(edTRIM, editPos, NULL);
		editTrim(editPos);
# else
		IIVFteTrimEdit(editPos);
# endif
		break;

	case PBOX:
		if (editPos->ps_attr == IS_STRAIGHT_EDGE)
		{
			IIUGerr(E_VF016B_cant_edit_x_hair, UG_ERR_ERROR, 0);
			break;
		}
		vfBoxEdit(editPos);
		break;

	case PTABLE:
# ifndef FORRBF
		tfedit(&editFd, editPos);
		break;
# endif

	case PSPECIAL:

		/*
		**  Modified for fix to BUG 5167. (dkh)
		*/

		if (RBF)
		{
			IIUGerr(E_VF0040_Cant_edit_a_RBF_sect, UG_ERR_ERROR, 0);
		}
		else
		{
			IIUGerr(E_VF0041_Cant_edit_End_of_For, UG_ERR_ERROR, 0);
		}
		break;

	default:
		syserr(ERget(S_VF0042_Unknown_position_stru),
			editPos->ps_name);
	}
	vfTrigUpd();
	vfUpdate(globy, globx, TRUE);
}

/*
** edit the trim on the passed position
*/
VOID
editTrim(ps)
POS	*ps;
{
	TRIM	*tr;
	u_char	*str;
	i4	len;
	i4	diff;

	scrmsg(ERget(F_VF001B_Edit_trim));

	tr = (TRIM *) ps->ps_feat;

	unLinkPos(ps);
	VTxydraw(frame, tr->trmy, tr->trmx);
	vfTestDump();
	str = vfEdtStr(frame, tr->trmstr, tr->trmy, tr->trmx, VFBUFSIZ);
	if (str == NULL || *str == '\0')
	{
		/*
		** The user has blanked out the string completely.
		** Pretend they deleted it. BUG 408
		*/
		vfersPos(ps);
		setGlobUn(dlTRIM, ps, NULL);
		setGlMove(ps);
		vffrmcount(ps, -1);
		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		return;
	}
	wrOver();
	VTdelstring(frame, tr->trmy, tr->trmx, STlength(tr->trmstr));
	tr->trmstr = saveStr(str);
	len = STlength((char *) str);
	if ((diff = tr->trmx + len - 1 - endxFrm) > 0)
	{
	    vfwider(frame, diff);
	}
	putEdTrim(ps, tr->trmstr, TRUE);
}

/*
** place the passed string as trim
*/
VOID
putEdTrim(ps, str, noUndo)
POS	*ps;
char	*str;
bool	noUndo;
{
	TRIM	*tr;

	tr = (TRIM *) ps->ps_feat;

	ps->ps_endx = ps->ps_begx + STlength(str) - 1;
	fitPos(ps, noUndo);
	insPos(ps, TRUE);
	VTputstring(frame, ps->ps_begy, ps->ps_begx, str, tr->trmflags, FALSE,
	    IIVFsfaSpaceForAttr);
	VTxydraw(frame, ps->ps_begy, ps->ps_begx);
	vfTestDump();
}

/*
** edit the trim on the passed position
*/
VOID
editTitle(ps, fd)
POS	*ps;
SMLFLD	*fd;
{
	FIELD	*ft;
	u_char	*str;
	FLDHDR	*hdr;
	i4	len;
	i4	diff;

	ft = (FIELD *) ps->ps_feat;

	hdr = FDgethdr(ft);
	setGlobUn(edTITLE, ps, fd);
	VTxydraw(frame, fd->ty, fd->tx);
	vfTestDump();
	str = vfEdtStr(frame, hdr->fhtitle, fd->ty, fd->tx, VFTITLEBSIZ);
	fd->tstr = saveStr(str);
	len = STlength((char *) str);
	if ((diff = fd->tx + len - 1 - endxFrm) > 0)
	{
	    vfwider(frame, diff);
	}
	putEdTitle(ps, fd, TRUE);
}

VOID
putEdTitle(ps, fd, noUndo)
POS	*ps;
SMLFLD	*fd;
bool	noUndo;
{
	FIELD	*ft;
	FLDHDR	*hdr;

	/* NOSTRICT */
	ft = (FIELD *) ps->ps_feat;

	hdr = FDgethdr(ft);
	hdr->fhtitle = fd->tstr;
	fd->tex = fd->tx + STlength(fd->tstr) - 1;
	fixField(ps, fd, (i4) noUndo,  PTITLE);
}

/*
** edit the data format on the passed position
*/
VOID
editData(ps, fd)
POS	*ps;
SMLFLD	*fd;
{
	FIELD		*ft;
	FLDTYPE		*type;
	FLDHDR		*hdr;
	u_char		*str;
	u_char		*str2;
# ifdef FORRBF
	i4		otype;
	char		ntype;
# endif
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	sdbv;
	DB_DATA_VALUE	fltdbv;
	ADF_CB		*ladfcb;
	u_char		fmtbuf[100];
	bool		valid;
	bool		numeric;
	bool		typeok;
	bool		reversed;
	bool		is_str;
	i4		x;
	i4		y;
	i4		usrlen;
	i4		ndlen;
	u_char		blank_str[VFBUFSIZ + 1];
	i4		len_badfmt;

	ft = (FIELD *) ps->ps_feat;

	ladfcb = FEadfcb();
	fltdbv.db_datatype = DB_FLT_TYPE;

	type = FDgettype(ft);
	hdr = FDgethdr(ft);
# ifdef FORRBF
	if (RBF)
	{
		otype = type->ftdatatype;
	}
# endif
	setGlobUn((i4) edDATA, ps, fd);
	VTxydraw(frame, fd->dy, fd->dx);
	do
	{
		str = (u_char *) type->ftfmtstr;
		/*
		** NOTE:  Current length for data format specification is
		** 0 (default).  May later wish to make this 50, in keeping
		** with the storage space in ii_fields (which is less than
		** that allowed in ii_encoded_forms.)
		*/
		str = vfEdtStr(frame, str, fd->dy, fd->dx, 0);
		str2 = (u_char *)STskipblank((char *) str,
			STlength((char *) str));
		if (str2 != NULL)
		{
		    str = str2;
		}
		type->ftfmt = vfchkFmt(str);
		while (type->ftfmt == NULL)
		{
			IIUGerr(E_VF0044_not_legal_fmt, UG_ERR_ERROR, 1, str);

			/*
			**  Blank out bad fmt string.
			*/
			len_badfmt = STlength((char *) str);
			MEfill((u_i2)len_badfmt, ' ', blank_str);
			blank_str[len_badfmt] = EOS;
			VTputstring(frame, fd->dy, fd->dx, blank_str,
				(i4) 0, (bool) FALSE, (bool) FALSE);

			str = (u_char *) type->ftfmtstr;
			scrmsg(ERget(F_VF001A_Edit_format));
			VTxydraw(frame, fd->dy, fd->dx);
			vfTestDump();
			str = vfEdtStr(frame, str, fd->dy, fd->dx, 0);
			str2 = (u_char *)STskipblank((char *) str,
				STlength((char *) str));
			if (str2 != NULL)
			{
			    str = str2;
			}
			type->ftfmt = vfchkFmt(str);
		}
		typeok = TRUE;
		/*
		** Ignore return value since only checks are that second and
		** third args aren't NULL, and here that's guaranteed.
		** Just using to see if reversed.
		*/
		_VOID_ fmt_isreversed(ladfcb, type->ftfmt, &reversed);
		if (reversed)
			hdr->fhdflags |= fdREVDIR;
		else
			hdr->fhdflags &= ~fdREVDIR;

		/*
		** Extra code added for RBF.  Will not allow user
		** to change datatype from char to numeric and
		** vice versa.
		** Fix for bug 1306. (dkh)
		** Bug 1298 will be taken care of by fix to 1306.
		*/

		dbv.db_datatype = type->ftdatatype;
		dbv.db_length = type->ftlength;
		dbv.db_prec = type->ftprec;
		if (fmt_vfvalid(ladfcb, type->ftfmt, &dbv, &valid,
			&is_str) != OK)
		{
			IIUGerr(E_VF0045_Failed_format_compati,
				UG_ERR_ERROR, 0);
			scrmsg(ERget(F_VF001A_Edit_format));
			typeok = FALSE;
		}
		else
		{
			if (!valid)
			{
				if (RBF)
				{
					IIUGerr(E_VF0046_New_fmt_not_compat,
						UG_ERR_ERROR, 0);
					scrmsg(ERget(F_VF001A_Edit_format));
					typeok = FALSE;
				}
				else if (hdr->fhd2flags & fdVQLOCK)
				{
					IIUGerr(E_VF0122_disp_fmt_restricted,
						UG_ERR_ERROR, 0);
					scrmsg(ERget(F_VF001A_Edit_format));
					typeok = FALSE;
				}
				else
				{
					if (fmt_ftot(ladfcb, type->ftfmt,
						&dbv) != OK)
					{
						IIUGerr(E_VF004A_Fmt_to_data,
							UG_ERR_ERROR, 0);
						typeok = FALSE;
					}
					type->ftdatatype = dbv.db_datatype;
					type->ftlength = dbv.db_length;
					type->ftprec = dbv.db_prec;

					/*
					**  A datatype change always
					**  cancels scrollability for
					**  a field.
					*/
					hdr->fhd2flags &= ~fdSCRLFD;
				}
			}
			else
			{
				/*
				**  Check to see if fmt is mult-line.
				**  If it is cancel scrollability for field.
				*/
				_VOID_ fmt_size(ladfcb, type->ftfmt, NULL,
					&y, &x);
				if (y != 1)
				{
					hdr->fhd2flags &= ~fdSCRLFD;
				}
				/*
				**  Check for a format justification
				**  in the format.  If there is one,
				**  cancel scrollability for field.
				*/
				if (fmt_justify(ladfcb, type->ftfmt))
				{
					hdr->fhd2flags &= ~fdSCRLFD;
				}

				/*
				**  If datatype is decimal, make sure
				**  we pick up any changes to the
				**  precision/scale value.
				*/
				if (abs(dbv.db_datatype) == DB_DEC_TYPE)
				{
					type->ftlength = dbv.db_length;
					type->ftprec = dbv.db_prec;
				}

				if (is_str)
				{
					if (hdr->fhd2flags & fdSCRLFD)
					{
						/*
						**  Need to compare current
						**  data length with
						**  length from format.
						**  If format length is
						**  greater than or equal
						**  to data length,
						**  cancel scrollability and
						**  set data length to be
						**  format length.
						*/
						ndlen = dbv.db_length;
						dbv.db_length = type->ftlength;
						_VOID_ adc_lenchk(ladfcb,
						    FALSE, &dbv, &sdbv);
						usrlen = sdbv.db_length;
						if (x >= usrlen)
						{
						    type->ftlength = ndlen;
						    hdr->fhd2flags &= ~fdSCRLFD;
						}
					}
					else
					{
						type->ftlength = dbv.db_length;
					}
				}
			}
		}
		if (typeok)
		{
			/*
			**  Check if we need to add leading '-'
			**  sign on the format.
			*/
			if (!RBF)
			{
				if (afe_cancoerce(ladfcb, &dbv, &fltdbv,
					&numeric) != OK)
				{
					IIUGerr(E_VF0047_Error_checking_for_co,
						UG_ERR_ERROR, 0);
					typeok = FALSE;
				}
				if (numeric && *str != '-' && *str != '+')
				{
					STprintf(fmtbuf, ERx("-%s"), str);
					str = fmtbuf;
					if ((type->ftfmt = vfchkFmt(str)) ==
						NULL)
					{
						IIUGerr(E_VF0048_no_fmtstruct,
							UG_ERR_ERROR, 0);
						typeok = FALSE;
					}
				}
			}
			type->ftfmtstr = vfsaveFmt(str);
			typeok = putEdData(type->ftfmt, ps, fd, TRUE);
		}
	} while (!typeok);
}

bool
putEdData(fmt, ps, fd, noUndo)
FMT	*fmt;
POS	*ps;
SMLFLD	*fd;
bool	noUndo;
{
	FIELD		*ft;
	i4		y;
	i4		x;
	i4		extent;
	FLDTYPE		*type;
	DB_DATA_VALUE	dbv;
	bool		valid;

	ft = (FIELD *) ps->ps_feat;
	type = FDgettype(ft);
	dbv.db_datatype = type->ftdatatype;
	dbv.db_length = type->ftlength;
	dbv.db_prec = type->ftprec;
	if (fmt_isvalid(FEadfcb(), type->ftfmt, &dbv, &valid) != OK)
	{
		IIUGerr(E_VF0049_Failed_format_compati, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	if (!valid)
	{
		if (fmt_ftot(FEadfcb(), fmt, &dbv) != OK)
		{
			IIUGerr(E_VF004A_Fmt_to_data, UG_ERR_ERROR, 0);
			return (FALSE);
		}
		type->ftlength = dbv.db_length;
		type->ftdatatype = dbv.db_datatype;
		type->ftprec = dbv.db_prec;
	}

	vfgetSize(fmt, type->ftdatatype, type->ftlength, type->ftprec,
		&y, &x, &extent);
	if ((fd->dex = fd->dx + x - 1) >= endxFrm + 1)
	{
		i4	diff;

		if ((diff = fd->dex - endxFrm) == 0)
		{
			diff = IIVFterminfo.cols/4;
		}
		vfwider(frame, diff);
		vfTestDump();
	}
	fd->dey = fd->dy + y - 1;
	if (RBF && extent == 0)		/* is a c0 field */
		extent = x + x;
	type->ftwidth = extent;
	type->ftdataln = x;

	fixField(ps, fd, (i4) noUndo, (i4) PDATA);

	return (TRUE);
}

# ifndef FORRBF
editAttr(ps, fd)
register POS	*ps;
register SMLFLD *fd;
{
	setGlobUn((i4) edATTR, ps, fd);
	attrCom((FIELD *) editPos->ps_feat, FALSE);
	VTxydraw(frame, (i4) -1, (i4) -1);
}
# endif
