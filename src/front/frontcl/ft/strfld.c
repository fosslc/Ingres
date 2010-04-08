/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	strfld.c  -  Move String to Field
**
**	Move a string into a field.
**
**	NOTE - This file not using IIUGmsg since it does not
**	restore the message line.
**
**	Arguments:  string -  string to move
**		    fld	   -  field to move data into
**
**	History:  JEN - 1 Nov 1981  (written)
**		  DKH - 17 July 1985 Added support for noecho fields.
**		  DKH - 21 Aug. 1985 Added support for invisible table fields.
**	03/03/87 (dkh) - Added support for ADTs.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	09-nov-87 (bruceb)
**		Added code to handle scrolling fields.  A field's
**		underlying scroll buffer (pointed to by fvdatawin)
**		is updated from the display dbv.
**	12/12/87 (dkh) - Performance changes.
**	09-feb-88 (bruceb)
**		Scrolling flag now in fhd2flags; can't use most
**		significant bit of fhdflags.
**	29-mar-88 (bruceb)
**		TDerschars when < full size of the field, not
**		< size of datatype.  This is so that e.g. comparison
**		is done against 201 chars in case of c200.67, instead
**		of against 200 chars.
**	21-apr-88 (bruceb)
**		Moved call on IIFTcsfClrScrollFld into FTfldupd to be
**		done whether or not the form is visible (else if
**		replacement value is a blank string, the buffer just
**		waits around to be re-drawn when first char entered
**		into field.)
**	05/06/88 (dkh) - Updated for Venus.
**	08/04/88 (dkh) - Fixed problem with scroll field data not
**			 being updated if the form is not visible.
**	12/05/88 (dkh) - Performance changes.
**	22-dec-88 (bruceb)
**		Invisibility now may apply to regular fields.
**	10-feb-89 (bruceb)
**		Set up scroll struct pointer for all scrolling fields,
**		not just when count != 0.
**	03/22/89 (dkh) - Reduced optimization (for now) so displaying
**			 the same popup back to back wont't crash and burn.
**	12-dec-89 (bruceb)
**		Make sure invisible columns stay that way...
**	12/30/89 (dkh) - Update windows for field values if form is
**			 a popup behind a full screen form.
**	08/14/90 (dkh) - Fixed bug 32507.
**	08/20/90 (dkh) - Removed noecho checks since it is redundant.
**	08/24/92 (dkh) - Fixed acc complaints.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<erft.h>
# include	<scroll.h>
# include	<er.h>

static	FLDHDR	*tblhdr = NULL;

static	WINDOW	*pwin = NULL;
static	FRAME	*pfrm = NULL;

FUNC_EXTERN	WINDOW	*IIFTwffWindowForForm();

FTtblhdr(hdr)
FLDHDR	*hdr;
{
	tblhdr = hdr;
}



FTstr_fld(hdr, type, val, frm)
reg FLDHDR	*hdr;
reg FLDTYPE	*type;
reg FLDVAL	*val;
FRAME		*frm;
{
	DB_TEXT_STRING	*text;
	i4		count;
	u_char		*string;
	FLDHDR		*usehdr = hdr;
	bool		reverse;
	bool		scrlfld;
	SCROLL_DATA	*scroll;
	i4		lines;
	i4		fldsize;
	WINDOW		*usewin = FTwin;
	i4		ignore_full;


	/*
	**  If field or column is invisible, then we want to
	**  return only if the field is noecho and is not
	**  a scrollable field.  These conditions are necessary
	**  so that the scrolling structs are properly updated
	**  and usable when the field becomes visible.
	*/
	if (tblhdr != NULL)
	{
		if ((tblhdr->fhdflags & fdINVISIBLE) &&
			!(hdr->fhd2flags & fdSCRLFD))
		{
			return(TRUE);
		}
	}
	if ((hdr->fhdflags & fdINVISIBLE) &&
		!(hdr->fhd2flags & fdSCRLFD))
	{
		return(TRUE);
	}

	if (!(hdr->fhdflags & fdNOECHO))
	{
		/*
		**  Take care of scrolling field stuff even
		**  if not the current form.
		*/
		scrlfld = (bool)(hdr->fhd2flags & fdSCRLFD ? TRUE : FALSE);
		text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		count = text->db_t_count;
		if (scrlfld)
		{
			scroll = (SCROLL_DATA *)val->fvdatawin;

			if (count != 0)
			{
				/*
				** Use of MEcopy from 'left' is OK even for
				** reverse fields since that would indicate that
				** there are 'leading' blanks in that field.
				*/
				MEcopy((PTR) val->fvbufr, (u_i2) count,
					(PTR) scroll->left);
			}

			/*
			**  If we have an invisible field, we can now return.
			*/
			if ((tblhdr != NULL && tblhdr->fhdflags & fdINVISIBLE)||
				(hdr->fhdflags & fdINVISIBLE))
			{
				return(TRUE);
			}
		}

		/*
		**  If we are not updating the current form,
		**  check to see if we are updating a possibly
		**  visible form.
		*/
		if (frm != FTiofrm)
		{
			/*
			**  Call IIFTwffWindowForForm() to find the
			**  window for the form that is passed in.
			**  If a NULL pointer is returned then
			**  form is not at all visible.  Passed in
			**  form is probably behind another full
			**  screen form.
			if (frm == pfrm)
			{
				usewin = pwin;
			}
			else
			{
				pfrm = frm;
				pwin = usewin = IIFTwffWindowForForm(frm);
			}
			*/
			if (frm->frmflags & fdISPOPUP)
			{
				ignore_full = TRUE;
			}
			else
			{
				ignore_full = FALSE;
			}
			usewin = IIFTwffWindowForForm(frm, ignore_full);
			if (usewin == NULL)
			{
				return(FALSE);
			}
		}

		if (tblhdr != NULL)
		{
			usehdr = tblhdr;
		}
		if (type->ftwidth == type->ftdataln)
		{
			lines = 1;
		}
		else
		{
			lines = (type->ftwidth+type->ftdataln-1)/type->ftdataln;
		}
		if (TDsubwin(usewin, lines,
			type->ftdataln, usehdr->fhposy + val->fvdatay,
			usehdr->fhposx + type->ftdatax, FTutilwin) == NULL)
		{
			return(FALSE);
		}

		fldsize = lines * type->ftdataln;

		reverse = (bool)(hdr->fhdflags & fdREVDIR ? TRUE : FALSE);
		if (scrlfld)
		{
			if (reverse)
			{
				/*
				** Want to add string from proper point in the
				** display buffer.  For an RL field, this is
				** that point in the buffer such that an LR
				** addition starting there will cover the rest
				** (i.e. the beginning) of the buffer.
				**  for example:
				**  +----------------------------------------+
				**  |            display buffer              |
				**  +----------------------------------------+
				**                        ^
				**
				**                        +------------------+
				**                        |  visible window  |
				**                        +------------------+
				** This is equivalent to the width of the
				** visible window (maxy * maxx) before the end
				** of fvbufr (fvbufr plus total width of scroll
				** buffer).
				**
				** Needed just for reversed scrolling fields
				** because LR fields start at fvbufr's start,
				** and non-scrolling RL fields have an fvbufr
				** width equivalent to the width of the visible
				** window, so the calculation would result in
				** simply pointing to the start of fvbufr.
				*/
				string = val->fvbufr
					+ (i4)(scroll->right - scroll->left)
					- (FTutilwin->_maxy * FTutilwin->_maxx)
					+ 1;
			}
			else
			{
				string = val->fvbufr;
			}
		}
		else
		{
			string = val->fvbufr;
		}

		/*
		**  If string to add to window is shorter than
		**  the size of the window (row * col), then
		**  need to clear window first.
		*/
		if (count < fldsize)
		{
			TDerschars(FTutilwin);
		}

		if (count != 0
			&& TDxaddstr(FTutilwin, (i4) count, string) == FAIL
			&& count > fldsize && !scrlfld)
		{
			FTwinerr(ERget(F_FT000F_ERROR__field__s_trunc), TRUE,
				hdr->fhdname);
			return(FALSE);
		}

	}
	return(TRUE);
}
