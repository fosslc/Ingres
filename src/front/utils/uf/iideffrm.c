/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cm.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<frame.h>
# include	<flddesc.h>


/**
** Name:	iideffrm.c	Create default forms
**
** Description:
**	In many situations, creating a simple default form dynamically is
**	useful.  This routine produces either a default form with simple
**	fields or a default form with a tablefield.
**
**	IIFRmdfMakeDefaultForm	Create default form
**
** History:
**	7/20/89 (Mike S)	Initial Version
**	8/8/89 (Mike S)		Add scrolling field support
**	8/30/89 (Mike S)	Avoid extra line for no title
**	12/19/89 (dkh) - VMS shared lib changes.
**	3/6/90 (Mike S)		Change READONLY char *'s to char []'s.
**				(for VMS shareability).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FRAME		*IIFDofOpenFrame();
TBLFLD		*IIFDatfAddTblFld();
STATUS		IIFDaecAddExistCol();
STATUS		IIFDaefAddExistFld();
STATUS		IIFDffFormatFld();
STATUS		IIFDfcFormatColumn();
VOID		FTterminfo();

static	const	char _nl[] = ERx("\n");
static	const	char _empty[] = ERx("");
static	const	char _colon[] = ERx(":");

static bool 	insert_titles();
static VOID 	make_popup();
static STATUS 	make_sfframe();
static bool 	make_tfframe();
static VOID 	widen_tf();

/*{
** Name:	 IIFRmdfMakeDefaultForm - create default form
**
** Description:
**	Create a default form, either with simple fields or with a tablefield.
**	These forms are modelled on the qualification form and selection form
**	used for lookups:
**
**	The simplefield (qualifcation) form looks like:
**
**		+-----------------------+
**		|Title			|
**		|-----------------------|
**		|Field 1: _____		|
**		|  Fld 2: ______________|
**		+-----------------------+
**	that is, it's a boxed floating popup (if it fits), the colons line 
**	up, the field titles are right-justified, and the data fields left 
**	justified.
**
**	The table field (selection) form looks like:
**
**		+-----------------------+
**		|Title 			|
**		+-------+---------------+
**		|Field 1|Fld 2		|
**		|=======+===============+
**		|	|		|
**		|	|		|
**		|	|		|
**		+-------+---------------+
**
**	That is, it's a tablefield.  Again, it's a popup (this time unboxed)
**	if it fits.  If the title is wider than the tablefield, the tablefield
**	columns are stretched out to match it. The column titles are optional.
**
**	If the caller wants non-default behavior, he can diddle the frame 
**	structure returned to him.
**
**	The form title string may contain embedded newlines to indicate a
**	multi-line title.  Trailing newline(s) will result in blank line(s)
**	atfer the title.
**
** Inputs:
**	title		char *	Form title	
**	name		char *  Name of form (and tablefield, if any)
**	col_titles	bool	TRUE to use Column titles
**	num_fields	i4	Number of fields (or columns)
**	fld_desc	FLD_DESC *	Descriptors for fields (or columns)
**	tfrows		i4	Rows in tablefield (<= 0 for simple fields)
**
** Outputs:
**	frame		FRAME **	Frame constructed
**
**	Returns:
**		STATUS		OK
**				FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**	Allocates dynamic memory
**
** History:
**	7/20/89	(Mike S)	Initial version
*/
STATUS
IIFRmdfMakeDefaultForm(title, name, col_titles, num_flds, fld_desc, 
		       tfrows, frame)
char		*title;
char		*name;
bool		col_titles;
i4		num_flds;
FLD_DESC	*fld_desc;
i4		tfrows;
FRAME		**frame;
{
	bool	opened = FALSE;	/* Have we opened a dynamic frame */
	bool	tblfld = (tfrows > 0);
				/* Are we building a tablefield */
	i4	curline = (tblfld) ? 1 : 0; 	/* Current line */
	i4	titcol = (tblfld) ? 1 : 0;
				/* Column to start titles at */
	i4	titlen = 0;	/* Longest title line */
	i4	titlines;	/* Number of title lines */
	char	buffer[200];	/* Formatiing buffer */
	TRIM	*boxtrim;	/* Box trim */
	TBLFLD	*tf;		/* Tablefield */
	i4	retval;		/* Return value */
	i4	tfadjust;	/* Amount to adjust tablefield by */

	/* Assume failure */
	retval = FAIL;

	/* Open frame structure */
	if ((*frame = IIFDofOpenFrame(name)) == NULL)
		goto cleanup;
	opened = TRUE;

	/* Insert titles at top of form */
	if (title != NULL && *title != EOS)
	{
		if (!insert_titles(*frame, 
				   STtalloc((i4)(*frame)->frtag, title), 
			   	   &curline, &titlen, titcol))
			goto cleanup;
		titlines = curline;
	}
	else
	{
		titlines = curline = 0;
	}

	/* Make the rest of the frame */
	if (tblfld)
	{
		if (!make_tfframe(*frame, num_flds, fld_desc, tfrows, 
				  col_titles, curline, &boxtrim, &tf))
			goto cleanup;
	}
	else
	{
		if ( (retval = make_sfframe(*frame, num_flds, fld_desc, curline,
						&boxtrim)) != OK )
			goto cleanup;
	}

	/* Make the frame structure */
	opened = FALSE;
	if (!IIFDcfCloseFrame(*frame)) 
		goto cleanup;

	/*
	** If it's a tablefield, and the title is wider than the data, 
	** widen the tablefield for appearance's sake.  
	*/
	if (tblfld)
	{
		/* Be sure there's room for title and box trim */
		(*frame)->frmaxx = max((*frame)->frmaxx, titlen + 2);

		tfadjust = (*frame)->frmaxx - tf->tfhdr.fhmaxx;
		if (tfadjust > 0)
			widen_tf(tf, tfadjust);
	}

	/* Extend the box trim to its proper size */
	if (boxtrim != NULL)
	{
		if (tblfld)
		{
			/* 
			** A box with corners at (0,0) and 
			** (formwidth-1,titlines+1) 
			*/
			STprintf(buffer, "%d:%d:-1", titlines+2, 
				 (*frame)->frmaxx) ;
		}
		else
		{
			/* 
			** A line from (0, titlines) to 
			** (formwidth-1, titlines) 
			*/
			STprintf(buffer, "1:%d:-1", (*frame)->frmaxx);
		}
		boxtrim->trmstr = STtalloc((*frame)->frtag, buffer);
	}

	/* Make it a popup (if we can) */
	make_popup(*frame, !tblfld);
	retval = OK;

cleanup:
	/* If the dynamic frame is open, close it */
	if (opened)
		_VOID_ IIFDcfCloseFrame(*frame);
	return (retval);
}

/*
**	Insert titles at top of form.
*/
static bool
insert_titles(frame, title, curline, titlen, titcol)
FRAME	*frame;		/* frame being constructed */
char	*title;		/* Character titles */
i4	*curline;	/* Current form line */
i4	*titlen;	/* Maximum title length */
i4	titcol;		/* Column to start title at */
{
	/* Insert title lines, if any */
	while ((title != NULL) && (*title != EOS))
	{
		char *nl;	/* '\n' which terminates line */		
		char *next;	/* Next	line */
		char *p;

		/* Find start of next line */
		nl = STindex(title, _nl, 0);
		if (nl == NULL)
		{
			next = NULL;		/* Last line */
		}
		else
		{
			p = nl;
			next = CMnext(nl);
			*p = EOS;		/* Null-terminate line */
		}

		/* Add title line */
		if (IIFDattAddTextTrim(frame, (*curline)++, titcol, 0, title) 
			== NULL)
			return(FALSE);
		*titlen = max(*titlen, STlength(title));
		title = next;
	}
	/*
	** We stopped looping for one of two reasons:
	** 1. *title == EOS.  title ended with a newline, and we should add a 
	**    trailing blank line.
	** 2. title == NULL. title didn't end with a newline.
	*/
	if (title != NULL) 
		(*curline)++;
	return (TRUE);
}

/* 
**	Widen the tablefield 
*/
static VOID
widen_tf(tf, tfadjust)
TBLFLD 	*tf;		/* Tablefield to widen */
i4	tfadjust;	/* Amount to widen it by */
{
	i4	baseadjust;	/* Amount to widen each column by */
	i4	extra;		/* Number of columns to widen by one more */
	i4	total;		/* Total adjustments so far */
	i4	i;

	/* Get widening parameters */
	baseadjust = tfadjust / tf->tfcols;
	extra = tfadjust % tf->tfcols;

	/* Adjust all columns */
	for (i = 0, total = 0; i < tf->tfcols; i++)
	{
		register FLDCOL *colptr = tf->tfflds[i];
				/* Column pointer */
		register FLDHDR *hdr = &colptr->flhdr;
		register FLDTYPE *type = &colptr->fltype;
		i4 amt;	/* amount we're adjusting by */

		amt = (i < extra) ? baseadjust + 1 : baseadjust;

		/* 
		** Increase field, title, and data start by total adjustments 
		** so far.  Increase field size by current amount.
		*/
		hdr->fhposx += total;
		hdr->fhtitx += total;
		type->ftdatax += total;
		hdr->fhmaxx += amt;
		type->ftwidth += amt;
		type->ftdataln += amt;
		total += amt;		/* Add current adjustment into total*/
	}
	/* Widen tablefield */
	tf->tfhdr.fhmaxx += tfadjust;
}

/*
**	Make frame a popup (if it fits on the screen)
*/
static VOID
make_popup(frame, border)
FRAME	*frame;		/* Frame structure to change */
bool	border;		/* Whether to border it */
{
	i4	bord;	/* Space border takes up */
	FT_TERMINFO	terminfo;

	FTterminfo(&terminfo);

	bord = border ? 2 : 0;
	if (frame->frmaxy + bord > terminfo.lines ||
	    frame->frmaxx + bord > terminfo.cols )
		return;		/* It won't fit */

	/* Make it a popup */
        frame->frmflags |= (border ? (fdISPOPUP|fdBOXFR) : fdISPOPUP);
}

/*
**	Make a simplefield frame below the titles.
**
** History:
**	feb-1992 (wong) - Increased buffer size to better handle field
**		titles (previous size was 20!)  Bug #41657.
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
*/
static STATUS
make_sfframe(frame, num_flds, fld_desc, curline, boxtrim)
FRAME	*frame;		/* Frame structure */
i4	num_flds;	/* Number of fields */
FLD_DESC *fld_desc;	/* Field descriptors */
i4	curline;	/* Line to start at */
TRIM	**boxtrim;	/* Box trim produced (returned) */
{
	i4		mxftit;	/* Longest title */
	register i4  	i;

	/*
	** Add a piece of box trim below the titles.  We'll extend it
	** to the full form width later.  Skip this if the title was 
	** blank.
	*/
	if (curline > 0)
	{
		if ( (*boxtrim = IIFDabtAddBoxTrim(frame, curline, 0, 
					     curline, 1, (i4)0)) == NULL )
			return FAIL;
		++curline;
	}
	else
	{
		*boxtrim = NULL;
	}

	/* Get the biggest title length */
	for (i = num_flds, mxftit = 0; --i >= 0 ; )
		mxftit = max(mxftit, STlength(fld_desc[i].title));

	/* Format and add each field */
	for ( i = 0 ; i < num_flds ; ++i )
	{
		STATUS	status;
		FIELD	*field;
		char	*bufp;
		char	buffer[132+1];

		if ( (status = IIFDffFormatField(
				fld_desc[i].name, i, curline++,
				mxftit + 2,
				mxftit - STlength(fld_desc[i].title), 
				STpolycat(2, fld_desc[i].title, _colon, buffer),
				FE_FMTMAXCHARWIDTH, (i4)fdUNLN,
				fld_desc[i].type, 0, 
				fld_desc[i].maxwidth, &field)) != OK
			|| (status = IIFDaefAddExistFld(frame, field, 0, 0))
				!= OK )
		{
			return status;
		}
	}

	return OK;
}

/*
**	Make a tablefield frame below the titles.
*/
static bool
make_tfframe(frame, num_flds, fld_desc, tfrows, col_titles, curline, 
	     boxtrim, tf)
FRAME	*frame;		/* Frame structure */
i4	num_flds;	/* Number of fields */
FLD_DESC *fld_desc;	/* Field descriptors */
i4	tfrows;		/* Rows in tablefield */
bool	col_titles;	/* Whehter to use column titles */
i4	curline;	/* Line to start at */
TRIM	**boxtrim;	/* Box trim produced (returned) */
TBLFLD	**tf;		/* Tablefield produced (returned) */
{
	i4	i;

	/*
	** Add a piece of box trim.  Later we'll extend it to the
	** proper size. Skip this if there was no title.
	*/
	if (curline > 0)
	{
		*boxtrim = IIFDabtAddBoxTrim(frame, 0, 0, 1, 1, (i4)0);
		if (*boxtrim == NULL)
			return (FALSE);
	}
	else
	{
		*boxtrim = NULL;
	}
		
	/* Format and add the tablefield */
	if ((*tf = IIFDatfAddTblFld(frame, frame->frname, curline, 0, tfrows, 
				    col_titles, FALSE)) == NULL)
		return (FALSE);

	/* Format and add all the columns */
	for (i = 0; i < num_flds; i++)
	{
		char *title = col_titles ? fld_desc[i].title : _empty;
		FLDCOL *col;

		if (IIFDfcFormatColumn( fld_desc[i].name, 0, title, 
					fld_desc[i].type,
					fld_desc[i].maxwidth, &col) != OK)
			return (FALSE);
		if (IIFDaecAddExistCol(frame, *tf, col) != OK)
			return(FALSE);
	}
	return (TRUE);
}
