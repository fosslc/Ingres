/*
** Copyright (c) 2004 Ingres Corporation
*/
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
# include	<afe.h>
# include	"fdfuncs.h"
# include	<me.h>
# include	<er.h>
# include	"erfd.h"
# include	<scroll.h>
# include	<ug.h>
# include	<st.h>

/*
NO_OPTIM = r64_us5
*/

/*
**	DATAFMT.c  -  Get data format and configure value to format.
**
**	Arguments:  fld - field to place data into
**
**	History:
**	06/05/87 (dkh) - Trim trailing blanks after formatting.
**	25-jun-87 (bab) Code cleanup.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09-nov-87 (bab)
**		For scrolling fields, create a temporary format
**		with the same data type as before, but with a single
**		row of width equivalent to the entire scrolling buffer.
**		This is used when transferring data between the
**		display and internal buffers to avoid truncation of
**		the data to the size of the field's visible window.
**	12/12/87 (dkh) - Performance changes and changed winerr to IIUGerr.
**	09-feb-88 (bruceb)
**		fdSCRLFD now in fhd2flags instead of fhdflags; can't
**		use most significant bit of fhdflags.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	16-may-88 (bruceb)
**		If format'ing fails for single row display due to
**		overflow, continue on to display the overflow stars
**		('*'), returning TRUE on completion.  That is, the
**		value can't be displayed, but it is a valid value.
**	23-mar-1989 (danielt)
**		added parameter to fmt_format()
**	07/27/89 (dkh) - Added special marker parameter on call to fmt_multi.
**	02/27/90 (dkh) - Fixed bug 9101.
**	06-mar-90 (bruceb)
**		Pass down frm->frtag to IIFDssf(), and no longer MEfree fmtptr.
**      3/21/91 (elein)         (b35574) Add FALSE parameter to call to
**                              fmt_multi. TRUE is used internally for boxed
**                              messages only.  They need to have control
**                              characters suppressed.
**      26-sep-91 (jillb/jroy--DGC) (from 6.3)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	08/24/92 (dkh) - Fixed acc complaints.
**	06/30/92 (dkh) - Added support for input edit masks.
**	15-Aug-1993 (fredv) - included <st.h>.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-Oct-2004 (hweho01)
**          Turned off optimization for 64-bit compiling on AIX platform,
**          prevent cbf from segmentation violation.
**          Compiler version : C for AIX version 6.006.
**      14-Jan-2005 (hweho01)
**          Removed "*" char. from the NO_OPTIM line, so mkjam can
**          process it correctly.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

FUNC_EXTERN	VOID	IIFDssfSetScrollFmt();

i4
FDdatafmt(frm, fldno, disponly, fldtype, col, row, hdr, type, val)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
FLDHDR	*hdr;
FLDTYPE *type;
FLDVAL	*val;
{
	FMT		*fmtptr;	/* pointer to proper format */
	DB_DATA_VALUE	*dbv;
	DB_DATA_VALUE	*ddbv;
	DB_DATA_VALUE	wksp;
	DB_TEXT_STRING	*text;
	DB_TEXT_STRING	*ftext;
	PTR		buffer;
	ADF_CB		*ladfcb;
	i4		rows = 0;
	i4		columns = 0;
	i4		length = 0;
	i4		i = 0;
	u_i2		dscount;
	u_char		*dsptr;
	i4		fcount;
	u_char		*fptr;
	u_char		*cp;
	u_char		*end;
	bool		more = FALSE;
	bool		scrolling = FALSE;
	i4		fmttype;

	if (type->ftfmt == NULL)	/* if not format, is an error. */
	{
		IIFDerror(FLTCONV, 1, hdr->fhdname);
		return(FALSE);
	}
	if (hdr->fhd2flags & fdSCRLFD)
		scrolling = TRUE;

	if (scrolling)
	{
		/* create a fake, full scrolling-buffer-width fmt struct */
		IIFDssfSetScrollFmt(type->ftfmtstr, frm->frtag,
			(SCROLL_DATA *)val->fvdatawin, &fmtptr);
	}
	else
	{
		fmtptr = type->ftfmt;
	}
	dbv = val->fvdbv;
	ddbv = val->fvdsdbv;
	ladfcb = FEadfcb();

	/*
	**  Find out information for formatting.
	*/
	if (fmt_size(ladfcb, fmtptr, dbv, &rows, &columns) != OK)
	{
		IIFDerror(FLTCONV, 1, hdr->fhdname);
		return(FALSE);
	}

	/*
	**  Just format output if single line output.
	*/
	if (rows == 1)
	{
		if (fmt_format(ladfcb, fmtptr, dbv, ddbv, (bool) TRUE) != OK)
		{
			if (ladfcb->adf_errcb.ad_errcode !=
				E_FM1000_FORMAT_OVERFLOW)
			{
				IIUGerr(E_FD0001_Could_not_format,
					UG_ERR_ERROR, 1, hdr->fhdname);
				return(FALSE);
			}
			/*
			** else, fall through, to display the
			** FORMAT_OVERFLOW stars.
			*/
		}
		else
		{
			/*
			**  Check to see if an edit mask has been
			**  specified.  If so, we must do call
			**  fmt_stvalid() to force any template
			**  constant characters to be displayed.
			**  We assume things in range mode or query
			**  mode don't come through here.
			*/
			fmttype = type->ftfmt->fmt_type;

			if (hdr->fhd2flags & fdUSEEMASK)
			{
				bool		dummy;
				u_char		buf[DB_GW4_MAXSTRING + 1];
				u_char		buf2[DB_GW4_MAXSTRING + 1];
				DB_TEXT_STRING	*t;
				u_char		*from;
				u_char		*to;
				i4		dslen;
				bool		isnullable;

				t = (DB_TEXT_STRING *) ddbv->db_data;
				MEcopy((PTR) t->db_t_text, t->db_t_count,
					(PTR) buf);
				buf[t->db_t_count] = EOS;
				if (fmttype == F_ST || fmttype == F_DT)
				{
					(void) fmt_stvalid(type->ftfmt, buf,
						type->ftfmt->fmt_width, 0,
						&dummy);
				}
			/*
				else if (type->ftfmt->fmt_sign == FS_NONE ||
					type->ftfmt->fmt_sign == FS_PLUS)
				{
					isnullable = AFE_NULLABLE(dbv->db_datatype);
					(void) fmt_ntmodify(type->ftfmt,
						isnullable, '@', buf, 0,
						buf2, &dummy);
				}
			*/
				dslen = STlength((char *) buf);
				if (dslen > (ddbv->db_length - DB_CNTSIZE))
				{
					dslen = ddbv->db_length - DB_CNTSIZE;
				}
				MEcopy((PTR) buf, (u_i2) dslen,
					(PTR) t->db_t_text);
				t->db_t_count = dslen;
			}
		}
	}
	else
	{
		wksp.db_datatype = DB_LTXT_TYPE;
		wksp.db_prec = 0;
		wksp.db_length = DB_CNTSIZE + columns;
		if ((wksp.db_data = MEreqmem((u_i4)0, (u_i4)wksp.db_length,
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			IIFDerror(FLTCONV, 1, hdr->fhdname);
			return(FALSE);
		}
		fmt_workspace(ladfcb, fmtptr, dbv, &length);
		if ((buffer = MEreqmem((u_i4)0, (u_i4)length, TRUE,
		    (STATUS *)NULL)) == NULL)
		{
			IIFDerror(FLTCONV, 1, hdr->fhdname);
			return(FALSE);
		}

		/*
		**  Do multi-line output.
		*/

		IIfmt_init(ladfcb, fmtptr, dbv, buffer);

		dscount = 0;
		text = (DB_TEXT_STRING *) ddbv->db_data;
		text->db_t_count = 0;
		dsptr = text->db_t_text;
		ftext = (DB_TEXT_STRING *) wksp.db_data;
		for (;;)
		{
			if (fmt_multi(ladfcb, fmtptr, dbv, buffer,
				&wksp, &more, TRUE, FALSE) != OK)
			{
				IIFDerror(FLTCONV, 1, hdr->fhdname);
				return(FALSE);
			}

			if (!more)
			{
				break;
			}

			/*
			** put into fields display buffer.
			*/
			fcount = ftext->db_t_count;
			fptr = ftext->db_t_text;
			for (i = 0; i < fcount; i++)
			{
				*dsptr++ = *fptr++;
			}
			dscount += fcount;
		}
		text->db_t_count = dscount;
	}


	/*
	**  Need to trim trailing blanks at this point.
	*/
	text = (DB_TEXT_STRING *) ddbv->db_data;
	dscount = text->db_t_count;
	end = text->db_t_text;
	cp = end + dscount - 1;
	while (cp >= end)
	{
		if (*cp != ' ')
		{
			break;
		}
		cp--;
		dscount--;
	}
	text->db_t_count = dscount;

	FTfldupd(frm, fldno, disponly, fldtype, col, row);

	/*
	**  Free up memory used by multi-line formatting.
	*/
	if (rows != 1)
	{
		MEfree(wksp.db_data);
		MEfree(buffer);
	}

	return ((i4) TRUE);
}
