# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<multi.h>
# include	<ex.h>
# include	<er.h>
# include	<erfi.h>
# include	<scroll.h>


/*
**	qrydata.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	qrydata.c - Get query value for a field.
**
** Description:
**	Utilities to get query value for a field.
**	Routines defined:
**	- FDqrydata - Get query value for a field.
**
** History:
**	02/17/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	10/14/87 (dkh) - Made error messages trappable.
**	09-nov-87 (bab)
**		For scrolling fields, create a temporary format
**		with the same data type as before, but with a single
**		row of width equivalent to the entire scrolling buffer.
**		This is used when transferring data between the
**		display and internal buffers to avoid truncation of
**		the data to the size of the field's visible window.
**		Also handle detection of operators from proper end
**		of buffer when dealing with reversed fields.
**	09-feb-88 (bruceb)
**		fdSCRLFD now in fhd2flags; can't use most significant
**		bit of fhdflags.
**	29-apr-88 (bruceb)
**		After a failed fmt_cvt(), check against ABS of the datatype.
**	06-mar-90 (bruceb)
**		Pass down frm->frtag to IIFDssf(), and no longer MEfree fmtptr.
**	03-jul-90 (bruceb)	Fix for 31018.
**		Field data error messages now give field title and name when
**		available (instead of 'current field').
**	08/24/92 (dkh) - Fixed acc complaints.
**	12/28/92 (dkh) - Fixed bug 48170 & 48526.  Support for abf table field
**			 qualification now properly handles rows that
**			 are initially empty and later updated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/



static	i4	testdata = TRUE;


FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	FLDTYPE *FDgettype();
FUNC_EXTERN	FIELD	*FDfldofmd();
FUNC_EXTERN	STATUS	FDflterr();
FUNC_EXTERN	EX	FDadfxhdlr();
FUNC_EXTERN	VOID	IIFDssfSetScrollFmt();
FUNC_EXTERN	VOID	IIFDfdeFldDataError();

GLOBALREF	char	IIFDftFldTitle[];
GLOBALREF	char	IIFDfinFldInternalName[];




/*{
** Name:	FDqrydata - Synchronize query data for field.
**
** Description:
**	Synchronize the field value with the display buffer taking
**	into account of query operators at the beginning of the
**	buffer.	 Also, only data type checking is done on the
**	rest of the buffer.  Query operators occupy upto the
**	first tow positions in the display buffer.  Valid operators
**	are: "<", ">", "<=", ">=", "=" and "!=".  The absence
**	of one of these specific operators implies the "="
**	operator.  An empty display buffer implies no query operator.
**	Only do data type checking if "testdata" is set.  This is
**	a hook for tbacc.
**
**	This routine is the companion routine to getdata().  It fills
**	the storage area just like getdata(), but it looks for an
**	operator before the data.  It then places the operator into
**	the operator area of the field.
**
** Inputs:
**	frm		Form containing field to update.
**	fldno		Field number of field.
**	disponly	Flag identifying  field as updateable (FT_UPDATE)
**			or display only (FT_DISPONLY).
**	fldtype		Flag identifying field as a regular (FT_REGFLD)
**			or a table field (FT_TBLFLD).
**	col		Column number if field is a table field.
**	row		Row number if field is a table field.
**
** Outputs:
**	Returns:
**		TRUE	If update done.
**		FALSE	If bad data found in display buffer.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	JEN - 1 Nov 1981   (written)
**	NML -26 Sep 1983   Increased size of buffer to store
**			   data in for text - using DB_MAXTUPS as
**			   defined in ingres.h
**	NCG - 16 Jan 1984  Added option not to test data. This helps
**			   speed up scrolling query table fields.
**	02/17/87 (dkh) - Added procedure header.
**	02/19/91 (dkh) - Added support for 4gl table field qualifications.
*/
i4
FDqrydata(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	DB_DATA_VALUE	*dbv;
	DB_DATA_VALUE	*ddbv;
	DB_TEXT_STRING	*text;
	FIELD		*fld;
	TBLFLD		*tbl;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	FLDVAL		*val;
	i4		ocol;
	i4		orow;
	i4		retval = TRUE;
	i4		i;
	i4		qrycnt = 0;
	u_char		qryop1;
	u_char		qryop2;
	i4		dscount;
	u_char		*dsptr;
	u_char		*head;
	ER_MSGID	error_id;
	EX_CONTEXT	context;
	bool		reverse = FALSE;
	u_char		qryop[3];
	FMT		*fmtptr;
	bool		scrolling = FALSE;
	i4		*pflags;

	fld = FDfldofmd(frm, fldno, disponly);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = col;
		tbl->tfcurrow = row;
	}

	hdr = FDgethdr(fld);
	type = FDgettype(fld);
	val = FDgetval(fld);

	if (fldtype == FT_TBLFLD)
	{
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}

	/*
	**  Still need to put in change bit processing.
	*/

	/* get string from display buffer */

	ddbv = val->fvdsdbv;
	dbv = val->fvdbv;
	text = (DB_TEXT_STRING *) ddbv->db_data;

	dscount = text->db_t_count;
					/* GET OPERATOR */

	if (dscount == 0)		/* if nothing in a field */
	{
		type->ftoper = fdNOP;
		if (fldtype == FT_TBLFLD)
		{
			pflags = tbl->tffflags + (row * tbl->tfcols) + col;
			*pflags |= tfDISP_EMPTY;
		}
	}
	else
	{
		if (fldtype == FT_TBLFLD)
		{
			pflags = tbl->tffflags + (row * tbl->tfcols) + col;
			*pflags &= ~tfDISP_EMPTY;
		}

		if (hdr->fhdflags & fdREVDIR)
			reverse = TRUE;
		/* point at location of the potential query op */
		if (reverse)
		{
			head = text->db_t_text + dscount - 1;
			qryop[0] = *head;
			qryop[1] = *(head - 1);
			qryop[2] = EOS;
			FTswapparens((char *)qryop);
			dsptr = qryop;
		}
		else
		{
			head = dsptr = text->db_t_text;
		}

		if(*dsptr == '<')		/* get "<" or "<=" */
		{
			qryop1 = *dsptr++;
			qrycnt++;
			if (--dscount > 0 && *dsptr == '=') /* <= */
			{
				qryop2 = *dsptr++;
				qrycnt++;
				dscount--;
				type->ftoper = fdLE;
			}
			else			/* LESS THAN */
			{
				type->ftoper = fdLT;
			}
		}
		else if (*dsptr == '>')
		{
			qryop1 = *dsptr++;
			qrycnt++;
			if (--dscount > 0 && *dsptr == '=') /* >= */
			{
				qryop2 = *dsptr++;
				qrycnt++;
				dscount--;
				type->ftoper = fdGE;
			}
			else			/* GREATER THAN */
			{
				type->ftoper = fdGT;
			}
		}
		/* NOT EQUAL */
		else if (*dsptr == '!' && dscount > 1 && *(dsptr + 1) == '=')
		{
			qryop1 = *dsptr++;
			qryop2 = *dsptr++;
			qrycnt = 2;
			dscount -= 2;
			type->ftoper = fdNE;
		}
		else if (*dsptr == '=')		/* EQUAL */
		{
			qryop1 = *dsptr++;
			qrycnt++;
			dscount--;
			type->ftoper = fdEQ;
		}
		else		/* assume EQUAL operator for data in query */
		{
			type->ftoper = fdEQ;
		}
		if (dscount != text->db_t_count)
		{
			if (reverse)
			{
				dsptr = head - qrycnt;
				for (i = 0; i < dscount; i++)
				{
					*head-- = *dsptr--;
				}
				/* blank out the left end positions */
				*(dsptr + 1) = ' ';
				if (qrycnt == 2)
					*(dsptr + 2) = ' ';
				/* db_t_count is left unchanged */
			}
			else	/* LR field */
			{
				for (i = 0; i < dscount; i++)
				{
					*head++ = *dsptr++;
				}
				text->db_t_count = dscount;
			}
		}
	}

	/*
	** Table field scrolling in query mode must fetch the query operator,
	** and update it to the next row.  There is no need to go thru
	** FDqrydata() with data testing again -- this was done from
	** movetab(). (ncg)
	*/
	if (!testdata)
		return (TRUE);

	/*
	**  Do conversion from long text to internal value.
	**  Will add more detail error reporting later.
	*/
	if (text->db_t_count == 0)
	{
		_VOID_ adc_getempty(FEadfcb(), dbv);
	}
	else
	{
		if (hdr->fhtitle)
		{
		    STcopy(hdr->fhtitle, IIFDftFldTitle);
		}
		else
		{
		    IIFDftFldTitle[0] = EOS;
		}
		STcopy(hdr->fhdname, IIFDfinFldInternalName);
		if (EXdeclare(FDadfxhdlr, &context) == OK)
		{
			if (hdr->fhd2flags & fdSCRLFD)
				scrolling = TRUE;

			if (scrolling)
			{
				/*
				** create a fake, full scrolling-buffer-width
				** fmt struct
				*/
				IIFDssfSetScrollFmt(type->ftfmtstr, frm->frtag,
					(SCROLL_DATA *)val->fvdatawin, &fmtptr);
			}
			else
			{
				fmtptr = type->ftfmt;
			}

			if (fmt_cvt(FEadfcb(), fmtptr, ddbv, dbv,
				(bool) FALSE, 0) != OK)
			{
				if (abs(type->ftdatatype) == DB_INT_TYPE)
				{
					error_id = E_FI21CB_8651;
				}
				else if (abs(type->ftdatatype) == DB_FLT_TYPE)
				{
					error_id = E_FI21CC_8652;
				}
				else
				{
					error_id = E_FI21CD_8653;
				}
				IIFDfdeFldDataError(error_id, 0);
				retval = FALSE;
			}
		}
		else
		{
			retval = FALSE;
		}
		EXdelete();
	}

	/*
	**  Move query operators back, if necessary.
	*/
	if (qrycnt)
	{
		if (reverse)
		{
			dsptr = text->db_t_text;
			head = dsptr + qrycnt;
			for (i = 0; i < dscount; i++)
			{
				*dsptr++ = *head++;
			}
			FTswapparens((char *)qryop);
			*(head - 1) = *qryop;
			if (qrycnt == 2)
				*(head - 2) = *(qryop + 1);
		}
		else
		{
			head = text->db_t_text + dscount - 1;
			dsptr = head + qrycnt;
			for (i = 0; i < dscount; i++)
			{
				*dsptr-- = *head--;
			}
			text->db_t_text[0] = qryop1;
			if (qrycnt == 2)
			{
				text->db_t_text[1] = qryop2;
			}
			text->db_t_count += qrycnt;
		}
	}
	return(retval);
}


/*
** This flag is reset before copying operator up while TDscrolling.
*/
FDqrytest(test)
i4	test;
{
	testdata = test;
}
