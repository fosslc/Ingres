# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"

/*
**  Copyright (c) 2004 Ingres Corporation
**
**  frfldcre.c
*/

i4	FDcfmt();
static	char	*frname;
FUNC_EXTERN	i4	IIFDemask();


/*
** frfldcre.c
**
**	Complete the fields for the frame driver.
**
** History:
**	Written.  10/21/82 (jrc)
**	03/05/87 (dkh) - Added support for ADTs.
**	05/01/87 (dkh) - Changed to call FDparse instead of FD2parse.
**	19-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	10/27/88 (dkh) - Performance changes.
**	04/07/89 (dkh) - Changed error CFFLSQ to E_FI1F52_8018.
**
**	Now that compiled forms are retrieved from the database, removed
**	database accesses from this routine.  All that's left is
**	creating windows for the fields, and doing the run-time parsing
**	of the validation checks.  1/11/85 (agh)
**	17-apr-90 (bruceb)
**		Lint changes; removed args in calls on FDwflalloc,
**		FDwtblalloc and FDwcolalloc.
**	19-feb-92 (leighb) DeskTop Porting Change:
**		IIbreak() has no arguments; deleted bogus ones.
**	12/12/92 (dkh) - Added support for edit masks.
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
*/
bool
FDbinfldcre(frm, wins)
FRAME	*frm;
bool	wins;
{
	register FIELD	**fldarr;	/* an array of fields */
	register FLDCOL	**colarr;	/* an array of table field columns */
	FIELD		*fld;		/* pointer to a field */
	TBLFLD		*tblfld;
	FLDCOL		**temp_tfflds;	/* to hold value of tfflds */
	i4		i, j;

	frname = frm->frname;

	/*
	** for BUG 1498 (no fields)
	*/
	if (frm->frnsno + frm->frfldno == 0)
		return (TRUE);

	for (i = 0, fldarr = frm->frnsfld; i < frm->frnsno; i++, fldarr++)
	{
		fld = *fldarr;
		/*
		** Allocate structs and window for the field
		*/
		if(FDwflalloc(fld, wins) == FALSE)
		{
			IIFDerror(CFFLAL, (i4) 2, frm->frname, fld->fldname);
			IIbreak();
			return (FALSE);
		}
		if (!FDsfldtrv(fld, FDcfmt, FDTOPROW))
		{
			return(FALSE);
		}
	}

	for (i = 0, fldarr = frm->frfld; i < frm->frfldno; i++, fldarr++)
	{
		fld = *fldarr;
		if (fld->fltag == FREGULAR)
		{
			/*
			** Allocate structs and window for the field
			*/
			if(FDwflalloc(fld, wins) == FALSE)
			{
				IIFDerror(CFFLAL, (i4) 2, frm->frname,
						fld->fldname);
				IIbreak();
				return (FALSE);
			}
		}
		else if (fld->fltag == FTABLE)
		{
			tblfld = fld->fld_var.fltblfld;
			/*
			** Temporarily stash away the value of tfflds,
			** since FDwtblalloc will try to create it anew.
			*/
			temp_tfflds = tblfld->tfflds;
			if (FDwtblalloc(tblfld) == FALSE)
			{
				IIFDerror(CFTBAL, 2, frm->frname,
					tblfld->tfhdr.fhdname);
				return (FALSE);
			}
			tblfld->tfflds = temp_tfflds;
			for (j = 0, colarr = tblfld->tfflds; j < tblfld->tfcols;
					j++, colarr++)
			{
				/*
				** Allocate structs and window for the column
				*/
				if (FDwcolalloc(tblfld, *colarr, wins) == FALSE)
				{
					IIFDerror(CFCLAL, 1, frm->frname);
					return (FALSE);
				}
			}
			/*
			**  Set tfcurcol to zero since vifred used to save
			**  things with this set equal to tfcols and
			**  causes AV in vifred "order" option.
			*/
			tblfld->tfcurcol = 0;
		}
		if (!FDsfldtrv(fld, FDcfmt, FDTOPROW))
		{
			return(FALSE);
		}
		if (wins)
		{
			FDsfldtrv(fld, IIFDemask, FDTOPROW);
		}
	}

	if (FDparse(frm))	/* if any fatal parse errors, abort */
	{
		return (FALSE);
	}
	return (TRUE);
}


i4
FDcfmt(hdr, type)
FLDHDR	*hdr;
FLDTYPE	*type;
{
	ADF_CB	*ladfcb;
	char	*fmtstr;
	i4	fmtsize;
	i4	fmtlen;
	PTR	blk;

	/*
	**  It is an error if there is no format string
	**  at this time.
	*/
	if (type->ftfmtstr != NULL)
	{
		ladfcb = FEadfcb();

		/*
		**  Don't do anything special to the format string
		**  since everything should we synced up by the
		**  time we get here.
		*/
		fmtstr = type->ftfmtstr;

		if ((fmt_areasize(ladfcb, fmtstr, &fmtsize) == OK) &&
			((blk = FEreqmem((u_i4)0, (u_i4)fmtsize,
			    FALSE, (STATUS *)NULL)) != NULL) &&
			(fmt_setfmt(ladfcb, fmtstr, blk, &(type->ftfmt),
				&fmtlen) == OK))
		{
			;
		}
		else
		{
			FEafeerr(ladfcb);
			IIFDerror(E_FI1F52_8018, (i4)2, frname, hdr->fhdname);
			return(FALSE);
		}
	}
	else
	{
		return(FALSE);
	}
	return(TRUE);
}


i4
IIFDemask(hdr, type)
FLDHDR	*hdr;
FLDTYPE	*type;
{
	i4	rows;

	/*
	**  Now check to see if we have anything to do with
	**  edit masks.
	*/
	if (hdr->fhd2flags & fdEMASK)
	{
		i4	rows;

		rows = (type->ftwidth + type->ftdataln - 1)/type->ftdataln;

		if (!(hdr->fhd2flags & fdSCRLFD) &&
			!(hdr->fhdflags & (fdNOECHO | fdREVDIR)) &&
			(type->ftfmt->fmt_emask != NULL ||
			(type->ftfmt->fmt_type == F_NT &&
			(type->ftfmt->fmt_sign == FS_NONE ||
			type->ftfmt->fmt_sign == FS_PLUS))) && rows == 1)
		{
			hdr->fhd2flags |= fdUSEEMASK;
		}
	}

	return(TRUE);
}
