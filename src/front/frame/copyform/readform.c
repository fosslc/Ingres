/*
**	readform.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<nm.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<me.h>
# include       <ds.h>
# include       <feds.h>
# include       <fmt.h>
# include       <adf.h>
# include       <afe.h>
# include       <ft.h>
# include	<frame.h>
# include	<cm.h>
# include	"ercf.h"
# include	<cf.h>

/**
** Name:	readform.c - Read a copyform file and return a FRAME pointer
**
** Description:
**	This file contains code to read a form from a file that was copied
**	out by copyform.  It assumes that the file is in version 6 format
**	and that there are no fields on the non-sequenced chain.  Files
**	that don't follow these rules will mostly like cause strange
**	problems.  Since this is for internal use only, very simplistic
**	error checking is performed.
**
**	Originally written as a quick workaround to get demo of FEs
**	working on the DEC/ALPHA machine since no local DBMS was yet
**	available.
**
**	This has now become a supported routine.
**
** History:
**	11/03/92 (dkh) - Initial version.
**	02/20/93 (dkh) - Last clean up to make this a supported routine.
**	03/21/93 (dkh) - Removed prototypes since 6.4 code does not use
**			 ansi compiler.
**	03/31/93 (dkh) - Added support for non sequenced simple fields
**			 since some forms still have this old behavior.
**	08/08/93 (dkh) - Changed code to call FDwtblalloc() instead of
**			 FDtblalloc().  This eliminates an unncessary
**			 entry point from the shared libs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-feb-2009 (stial01)
**          Define copyform record buffer with size COPYFORM_MAX_REC
**/


FUNC_EXTERN	i4	FDfralloc();

static	void	getnext();
static 	void	err_msg();

/*
**  Structure for handling an array of fields from ii_fields catalog.
*/
typedef struct
{
	i4	type;			/* is this a regular field or tf col */
	union
	{
		FIELD	*vfd;		/* pointer to a regular field */
		FLDCOL	*vcol;		/* pointer to a table field column */
	} var;
} FLDARR;


/*{
** Name:	getnext - Get next component on a line.
**
** Description:
**	Gets the next component on a line.  Assumes that components are
**	separated by tabs (copyform version 6) and that last component
**	on a line ends with at a newline.  Once a component is found,
**	the tab character is replaced by EOS and the starting point
**	to look for the next component is set to character following
**	the tab.
**
** Inputs:
**	start 		Starting point to look
**
** Outputs:
**	next		Pointer to where next component begins
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/03/92 (dkh) - Initial version
*/
static void
getnext(start, next)
char	*start;
char	**next;
{
	char	*sp;
	char	*np;

	if (start == NULL)
	{
		*next = NULL;
		return;
	}
	sp = start;
	while (*sp != EOS && *sp != '\t' && *sp != '\n')
	{
		CMnext(sp);
	}
	if (*sp == '\t')
	{
		*next = sp + 1;
	}
	else
	{
		*next = NULL;
	}
	*sp = EOS;
}


/*{
** Name:	err_msg - Generic error message for IICFrfReadForm
**
** Description:
**	Routine to print out error message indicating that the
**	copyform file is somehow corrupted.
**
** Inputs:
**	name		Filename for error message
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/03/92 (dkh) - Initial version.
*/
static void
err_msg(name)
char	*name;
{
	IIUGerr(E_CF0025_BADFILE, UG_ERR_ERROR, 1, name);
}

/*{
** Name:	IICFrfReadForm - read a copyform file.
**
** Description:
**	Read in a form description from a copyform file, build a
**	FRAME structure based on the description and return the
**	pointer to the built structure.
**
**	Assumes that the copyform file is in version 6 format and that
**	there are **	no fields on the non-sequenced chain.  Files
**	not following these rules can't cause unexpected behavior.
**
**	The code in this routine can obviously be compacted and improved.
**	But it has been left alone since it is for internal use only and
**	makes it very easy to update should the copyform file format
**	change.
**
** Input params:
**	filename	Name of copyform file to read form description from
**
** Output params:
**	formname	Name of form that was read in
**
** Returns:
**	FRAME		Pointer to created FRAME structure
**	NULL		A NULL pointer if something went wrong
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	11/03/92 (dkh) - Hacked together get formindex to run standalone
**			 without a database connection.  This code ASSUMES
**			 that only form files are read and that there
**			 is only one per file.
**	02/20/93 (dkh) - Cleaned up code now that this needs to be supported.
*/



FRAME *
IICFrfReadForm(filename, formname)
char	*filename;
char	*formname;
{
	/* internal declarations */

	char		inbuf[COPYFORM_MAX_REC]; /* character buffer */
	char		objname[FE_MAXNAME+1];
	char		*short_remark;
	char		*long_remark;
	LOCATION	loc_forms;
	FILE		*fd_forms;
	char		file_locbuf[MAX_LOC + 1];
	char		*orig_fname;
	char		*start;
	char		*next;
	i4		i;
	TRIM		*tr;
	FLDCOL		*col;
	FIELD		*fld;
	FRAME		*new_frame;
	i4		frm_col;
	i4		frm_lin;
	i4		frm_posx;
	i4		frm_posy;
	i4		frm_fldno;
	i4		frm_nsno;
	i4		frm_tno;
	i4		frm_version;
	i4		frm_scrtype;
	i4		frm_scrx;
	i4		frm_scry;
	i4		frm_dpix;
	i4		frm_dpiy;
	i4		frm_flags;
	i4		frm_2flags;
	i4		frm_totflds;
	i4		fld_seq;
	char		fld_name[FE_MAXNAME + 1];
	i4		fld_type;
	i4		fld_len;
	i4		fld_prec;
	i4		fld_width;
	i4		fld_lin;
	i4		fld_col;
	i4		fld_posy;
	i4		fld_posx;
	i4		fld_dwidth;
	i4		fld_dlin;
	i4		fld_dcol;
	char		fld_title[fdTILEN + 1];
	i4		fld_tcol;
	i4		fld_tlin;
	i4		fld_intrp;
	i4		fld_flags;
	i4		fld_2flags;
	i4		fld_font;
	i4		fld_ptsz;
	char		fld_def[fdDEFLEN + 1];
	char		fld_fmt[fdFMTLEN + 1];
	char		fld_vmsg[fdVMSGLEN + 1];
	char		fld_vchk[fdVSTRLEN + 1];
	i4		fld_ftype;
	i4		fld_subseq;
	i4		tr_col;
	i4		tr_lin;
	char		tr_string[fdTRIMLEN + 1];
	i4		tr_flags;
	i4		tr_2flags;
	i4		tr_font;
	i4		tr_ptsz;
	FLDHDR		*hdr;
	FLDTYPE		*ftype;
	FLDVAL		*val;
	TBLFLD		*tbl;
	FLDARR		*fld_arr;
	FLDARR		*ap;
	i4		intable;
	i4		found_trim;
	i4		numcols;
	i4		nsseq = 0;

	if (formname)
	{
	    *formname = EOS;
	}

	orig_fname = filename;

	/* Open the copyform file */
	STcopy(filename, file_locbuf);
	LOfroms(PATH & FILENAME, file_locbuf, &loc_forms);
	if (SIopen(&loc_forms, ERx("r"), &fd_forms) != OK)
	{
	    IIUGerr(E_CF0027_OPENFAILED, UG_ERR_ERROR, 1, filename);
	    return (NULL);
	}

	/* get first record */
	if (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) == ENDFILE)
	{
		err_msg(filename);
		return (NULL);
	}

	/* Check if file is appropriate version */
	if (STbcompare(inbuf, 11, ERx("COPYFORM:\t6"), 11, FALSE) != 0)
	{
		err_msg(filename);
		return(NULL);
	}

	if (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) == ENDFILE)
	{
		err_msg(filename);
		return (NULL);
	}
	if (STbcompare(inbuf, 6, ERx("FORM:\t"), 6, FALSE) != 0)
	{
		err_msg(filename);
		return(NULL);
	}
	start = inbuf;
	/*
	**  Throw away the FORM:\t component.
	*/
	getnext(start, &next);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the name value */
	getnext(start, &next);
	STcopy(start, objname);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* Grab the short remark and drop it into a black hole */
	getnext(start, &next);
	short_remark = start;
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* Grab the long remark and drop it into a black hole as well */
	getnext(start, &next);
	long_remark = start;

	/*
	**  Now grab the details of a form.
	*/
	if (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) == ENDFILE)
	{
		err_msg(filename);
		return (NULL);
	}
	start = inbuf;

	/* Throw away the first tab */
	getnext(start, &next);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* Get the col value. */
	getnext(start, &next);
	CVan(start, &frm_col);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the lin value. */
	getnext(start, &next);
	CVan(start, &frm_lin);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the posx value */
	getnext(start, &next);
	CVan(start, &frm_posx);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the posy value */
	getnext(start, &next);
	CVan(start, &frm_posy);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the fldno value */
	getnext(start, &next);
	CVan(start, &frm_fldno);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the nsno value */
	getnext(start, &next);
	CVan(start, &frm_nsno);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the trimno value */
	getnext(start, &next);
	CVan(start, &frm_tno);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the version value */
	getnext(start, &next);
	CVan(start, &frm_version);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the scrtype value */
	getnext (start, &next);
	CVan(start, &frm_scrtype);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the scrmaxx value */
	getnext(start, &next);
	CVan(start, &frm_scrx);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the scrmaxy value */
	getnext(start, &next);
	CVan(start, &frm_scry);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the dpix value */
	getnext(start, &next);
	CVan(start, &frm_dpix);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the dpiy value */
	getnext(start, &next);
	CVan(start, &frm_dpiy);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the flags value */
	getnext(start, &next);
	CVal(start, &frm_flags);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the 2flags value */
	getnext(start, &next);
	CVal(start, &frm_2flags);
	if ((start = next) == NULL)
	{
		err_msg(filename);
		return(NULL);
	}

	/* get the totflds value */
	getnext(start, &next);
	CVal(start, &frm_totflds);

	/*
	**  Now set up the FRAME structure, etc. so we can
	**  hang fields from it.
	*/
	if ((new_frame = FDnewfrm(objname)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-1"));
		return(NULL);
	}

	new_frame->frversion = frm_version;
	new_frame->frfldno = frm_fldno;
	new_frame->frnsno = frm_nsno;
	new_frame->frtrimno = frm_tno;
	new_frame->frmaxx = frm_col;
	new_frame->frmaxy = frm_lin;
	new_frame->frposx = frm_posx;
	new_frame->frposy = frm_posy;
	new_frame->frmflags = frm_flags;
	new_frame->frm2flags = frm_2flags;
	new_frame->frscrtype = frm_scrtype;
	new_frame->frscrxmax = frm_scrx;
	new_frame->frscrymax = frm_scry;
	new_frame->frscrxdpi = frm_dpix;
	new_frame->frscrydpi = frm_dpiy;

	if (!FDfralloc(new_frame))
	{
		IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-2"));
		return(NULL);
	}

	if (frm_totflds)
	{
		if ((fld_arr = (FLDARR *) MEreqmem((u_i4)0,
			(u_i4) (frm_totflds * sizeof(FLDARR)), TRUE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-3"));
			return(NULL);
		}
	}

	/*
	**  Now read in the fields, if any.
	*/
	if (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) == ENDFILE)
	{
		if (frm_totflds != 0 || frm_tno != 0)
		{
			err_msg(filename);
			return (NULL);
		}
		else
		{
			/*
			**  Do whatever cleanup is needed.
			*/
			if (formname)
			{
				STcopy(new_frame->frname, formname);
			}
			return(new_frame);
		}
	}

	found_trim = FALSE;

	if (STbcompare(inbuf, 6, ERx("FIELD:"), 6, FALSE) == 0)
	{
		while (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) != ENDFILE)
		{
			if (STbcompare(inbuf, 5, ERx("TRIM:"), 5, FALSE) == 0)
			{
				found_trim = TRUE;
				break;
			}
			start = inbuf;

			/* throw away the first tab */
			getnext(start, &next);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the seq value */
			getnext(start, &next);
			CVan(start, &fld_seq);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the name value */
			getnext(start, &next);
			STcopy(start, fld_name);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the datatype value */
			getnext(start, &next);
			CVan(start, &fld_type);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the length value */
			getnext(start, &next);
			CVan(start, &fld_len);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the prec value */
			getnext(start, &next);
			CVan(start, &fld_prec);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the field width value */
			getnext(start, &next);
			CVan(start, &fld_width);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the field lin value */
			getnext(start, &next);
			CVan(start, &fld_lin);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the field col value */
			getnext(start, &next);
			CVan(start, &fld_col);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the posy value */
			getnext(start, &next);
			CVan(start, &fld_posy);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the posx value */
			getnext(start, &next);
			CVan(start, &fld_posx);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the datawidth value */
			getnext(start, &next);
			CVan(start, &fld_dwidth);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the data lin value */
			getnext(start, &next);
			CVan(start, &fld_dlin);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the data col value */
			getnext(start, &next);
			CVan(start, &fld_dcol);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the title value */
			getnext(start, &next);
			STcopy(start, fld_title);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the title col value */
			getnext(start, &next);
			CVan(start, &fld_tcol);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the title lin value */
			getnext(start, &next);
			CVan(start, &fld_tlin);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the intrp value */
			getnext(start, &next);
			CVan(start, &fld_intrp);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the flags value */
			getnext(start, &next);
			CVal(start, &fld_flags);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the 2flags value */
			getnext(start, &next);
			CVal(start, &fld_2flags);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the font value */
			getnext(start, &next);
			CVan(start, &fld_font);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the ptsz value */
			getnext(start, &next);
			CVan(start, &fld_ptsz);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the default value */
			getnext(start, &next);
			STcopy(start, fld_def);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the format value */
			getnext(start, &next);
			STcopy(start, fld_fmt);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the valmsg value */
			getnext(start, &next);
			STcopy(start, fld_vmsg);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the valchk value */
			getnext(start, &next);
			STcopy(start, fld_vchk);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the field type value */
			getnext(start, &next);
			CVan(start, &fld_ftype);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the subseq value */
			getnext(start, &next);
			CVan(start, &fld_subseq);

			/*
			**  We finally got everything.  Now create the
			**  field and stuck it somewhere.  If its a
			**  simplefield or table field, stick it into the
			**  FRAME struct directly.  We also stick everything
			**  into the field array so that we can handle the
			**  tf columns later on.
			*/

			if (fld_ftype == FREGULAR || fld_ftype == FTABLE)
			{
				fld = FDnewfld(fld_name, fld_seq, fld_ftype);
				if (fld == NULL)
				{
					IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-4"));
					return(NULL);
				}

				fld_arr[fld_subseq].var.vfd = fld;
				if (fld_ftype == FTABLE)
				{
					tbl = fld->fld_var.fltblfld;
					hdr = &tbl->tfhdr;
				}
				else
				{
					hdr = &fld->fld_var.flregfld->flhdr;
					ftype = &fld->fld_var.flregfld->fltype;
					val = &fld->fld_var.flregfld->flval;
				}
				hdr->fhdflags = fld_flags;
				hdr->fhintrp = fld_intrp;
				hdr->fhd2flags = fld_2flags;
				hdr->fhdfont = fld_font;
				hdr->fhdptsz = fld_ptsz;

				/*
				**  If fld_seq is negative, then it is
				**  a non-sequenced simple field.  Table
				**  fields can not be non sequenced.
				*/
				if (fld_seq < 0)
				{
					new_frame->frnsfld[nsseq++] = fld;
				}
				else
				{
					new_frame->frfld[fld_seq] = fld;
				}
			}
			else
			{
				/* Must be a column field */
				col = FDnewcol(fld_name, fld_seq);
				if (col == NULL)
				{
					IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-5"));
					return(NULL);
				}
				fld_arr[fld_subseq].var.vcol = col;
				hdr = &col->flhdr;
				ftype = &col->fltype;
				hdr->fhdflags = fld_flags;
				hdr->fhd2flags = fld_2flags;
				hdr->fhdfont = fld_font;
				hdr->fhdptsz = fld_ptsz;
			}
			fld_arr[fld_subseq].type = fld_ftype;

			hdr->fhseq = fld_seq;
			hdr->fhmaxx = fld_col;
			hdr->fhmaxy = fld_lin;
			hdr->fhposx = fld_posx;
			hdr->fhposy = fld_posy;
			hdr->fhtitle = STalloc(fld_title);
			hdr->fhtitx = fld_tcol;
			hdr->fhtity = fld_tlin;
			if (fld_ftype == FTABLE)
			{
				tbl->tfrows = fld_len;
				tbl->tfcols = fld_width;
				tbl->tfwidth = fld_dwidth;
				tbl->tfstart = fld_dlin;
				if (!FDwtblalloc(fld->fld_var.fltblfld))
				{
					IIUGbmaBadMemoryAllocation(ERx("IICFrfReadForm-6"));
					return(NULL);
				}
			}
			else
			{
				ftype->ftdatatype = fld_type;
				ftype->ftwidth = fld_width;
				ftype->ftlength = fld_len;
				ftype->ftprec = fld_prec;
				ftype->ftdatax = fld_dcol;
				ftype->ftdataln = fld_dwidth;
				ftype->ftvalmsg = STalloc(fld_vmsg);
				ftype->ftdefault = STalloc(fld_def);
				if (*fld_vchk == EOS)
				{
					ftype->ftvalstr = NULL;
				}
				else
				{
					ftype->ftvalstr = STalloc(fld_vchk);
				}
				if (fld_ftype == FREGULAR)
				{
					val->fvdatay = fld_dlin;
				}
				ftype->ftfmtstr = STalloc(fld_fmt);
			}




		}
		if (!found_trim)
		{
			/*
			**  Do clean up and return.  May also want to
			**  do some sanity checks before returning.
			*/
			intable = FALSE;
			for (i = 0, ap = fld_arr; i < frm_totflds; i++, ap++)
			{
				switch(ap->type)
				{
				  case FREGULAR:
				  case FTABLE:
					if (intable)
					{
						if (tbl->tfcols != numcols)
						{
							err_msg(filename);
							return(NULL);
						}
						intable = FALSE;
					}
					if (ap->type == FTABLE)
					{
						fld = ap->var.vfd;
						tbl = fld->fld_var.fltblfld;
						hdr = &tbl->tfhdr;
						intable = TRUE;
						numcols = 0;
					}
					break;
				  case FCOLUMN:
					col = ap->var.vcol;
					hdr = &col->flhdr;
					if (!intable)
					{
						err_msg(filename);
						return(NULL);
					}
					if (!(hdr->fhseq >= 0 &&
						hdr->fhseq <= tbl->tfcols))
					{
						err_msg(filename);
						return(NULL);
					}
					tbl->tfflds[hdr->fhseq] = col;
					numcols++;
				}
			}
			if (formname)
			{
				STcopy(new_frame->frname, formname);
			}
			return(new_frame);
		}
	}

	/*
	**  Now read in trim, if any.
	*/
	if (STbcompare(inbuf, 5, ERx("TRIM:"), 5, FALSE) == 0)
	{
		i = 0;
		while (SIgetrec(inbuf, COPYFORM_MAX_REC, fd_forms) != ENDFILE)
		{
			start = inbuf;

			/* throw away the first tab */
			getnext(start, &next);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the col value */
			getnext(start, &next);
			CVan(start, &tr_col);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the lin value */
			getnext(start, &next);
			CVan(start, &tr_lin);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the string value */
			getnext(start, &next);
			STcopy(start, tr_string);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the flags value */
			getnext(start, &next);
			CVal(start, &tr_flags);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the 2flags value */
			getnext(start, &next);
			CVal(start, &tr_2flags);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the font value */
			getnext(start, &next);
			CVan(start, &tr_font);
			if ((start = next) == NULL)
			{
				err_msg(filename);
				return(NULL);
			}

			/* get the ptsz value */
			getnext(start, &next);
			CVan(start, &tr_ptsz);

			/*
			**  We finally got everything.  Now creat the
			**  trim structure and stick it into the frame.
			*/

			tr = FDnewtrim(tr_string, tr_lin, tr_col);
			tr->trmflags = tr_flags;
			tr->trm2flags = tr_2flags;
			tr->trmfont = tr_font;
			tr->trmptsz = tr_ptsz;

			new_frame->frtrim[i++] = tr;
		}
	}
	else
	{
		err_msg(filename);
		return(NULL);
	}

	/*
	**  Do cleanup and return.  May also want to do some
	**  sanity checking first.
	*/
	intable = FALSE;
	for (i = 0, ap = fld_arr; i < frm_totflds; i++, ap++)
	{
		switch(ap->type)
		{
		  case FREGULAR:
		  case FTABLE:
			if (intable)
			{
				if (tbl->tfcols != numcols)
				{
					err_msg(filename);
					return(NULL);
				}
				intable = FALSE;
			}
			if (ap->type == FTABLE)
			{
				fld = ap->var.vfd;
				tbl = fld->fld_var.fltblfld;
				hdr = &tbl->tfhdr;
				intable = TRUE;
				numcols = 0;
			}
			break;
		  case FCOLUMN:
			col = ap->var.vcol;
			hdr = &col->flhdr;
			if (!intable)
			{
				err_msg(filename);
				return(NULL);
			}
			if (!(hdr->fhseq >= 0 && hdr->fhseq <= tbl->tfcols))
			{
				err_msg(filename);
				return(NULL);
			}
			tbl->tfflds[hdr->fhseq] = col;
			numcols++;
		}
	}

	if (formname)
	{
		STcopy(new_frame->frname, formname);
	}
	return(new_frame);
}
