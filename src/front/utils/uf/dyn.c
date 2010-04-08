/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<frame.h>
# include	<er.h>
# include	<ug.h>
# include	<st.h>

/*
NO_OPTIM=i64_hpu
*/

/**
** Name:	fddynfrm.c -	Dynamic frame creation support funcitons
**
** Description:
**	This file implements functions which allow callers to build
**	up the components of a frame structure without having to
**	know much about the frame structure themselves.
**
**  This file defines:
**
** 	IIFDofOpenFrame 	- Start a dynamically created frame struct.
** 	IIFDcfCloseFrame 	- Finish up a dynamically created frame struct.
** 	IIFDattAddTextTrim 	- Add text trim to a dynamically created frame
** 	IIFDabtAddBoxTrim 	- Add box trim to a dynamically created frame
**	IIFDaetAddExistTrim	- Add an existing piece of trim to a dyn. frame
** 	IIFDacfAddCharFld 	- Add a character field to a dynamic frame
** 	IIFDaifAddIntFld 	- Add a integer field to a dynamic frame
** 	IIFDatfAddTblFld	- Add a table field to a dynamic frame
**	IIFDaefAddExistFld	- Add an existing field to a frame
**	IIFDaexAddExistTf	- Add an existing tablefield to a frame
** 	IIFDaccAddCharCol 	- Add a character column to a table field
** 	IIFDaicAddIntCol 	- Add a integer column to a table field
**	IIFDaecAddExistCol	- Add an existing column to a tablefield
**
** History:
**	03/16/89 (tom) - spec written
**	7/17/89 (Mike S) 
**		"Exist" routines added; use FCOLUMN to mean dynamic tablefield.
**	20-nov-89 (bruceb)
**		Moved code from frame to uf.
**	12/17/89 (Mike S)
**		Added ability for tablefield to have line separators.
**	8-jun-90 (Mike S) Default fields and columns to color 1.
**	08/11/91 (dkh) - Added changes so that all memory allocated
**			 for a dynamically created form is freed.
**      15-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchin's change (from 6.4) for axp_osf
**              03/17/93 (kchin) - Added NO_OPTIM for axp_osf.
**	23-apr-1996 (chech02)
**		Added function prototypes for windows 3.1 port.
**   15-jan-1999 (schte01)
**        Removed NO_OPTIM for axp_osf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-May-2003 (bonro01)
**	    Added NO_OPTIM for i64_hpu.
**	    Was getting SEGV in iiabf in function IIFDcfCloseFrame()
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*}
** Name:	DYNLIST	- dynamically allocated list node struct
**
** Description:
**	This node structure is allocated using a seperate tag.
**	Then with the call to close the dynamic frame creation the 
**	field pointers are taken out of these nodes and placed into 
**	allocated arrays. Then these nodes are freed, and the tag
**	can be reused.
**
** History:
**	03/17/89 (tom) - created
*/
typedef struct _dynlist
{
	struct	_dynlist *next;		/* pointer to the next node */
	PTR 	ptr;			/* pointer to caller's data */
} DYNLIST;

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN char *FEsalloc();

# define LOC static

/* static's */
#ifdef WIN16
LOC REGFLD *_add_fld(PTR,PTR,nat,nat,PTR,nat,nat,nat,nat,nat,nat,i4);
LOC FLDCOL *_add_col(PTR,PTR,PTR,PTR,nat,nat,nat,nat,i4);
LOC bool _set_type(PTR,nat,nat,nat,nat,PTR);
LOC bool _add_list(i4,PTR,PTR);
LOC VOID _frame_widen(PTR,nat,nat); 
#endif /* WIN16 */

LOC REGFLD *_add_fld();	/* function to add a regular field to the frame */
LOC FLDCOL *_add_col();	/* function to add a column to a table field */
LOC TBLFLD * _add_tblfield(); /* function to add a tablefield to the frame */
LOC bool _set_type();	/* function to set field/column type info */
LOC bool _add_list();	/* function to add to dynamically created list */
LOC VOID _frame_widen(); /* function to widen the frame dimensions */
LOC bool _col_alloc();	/* function to allocate array of col struct ptrs for
			   a table field */

static const	char _empty[] = ERx("");


/*{
** Name:	IIFDofOpenFrame - Start a dynamically created frame struct.
**
** Description:
**	This function starts the creation of a frame structure.
**	The idea is to allocate a frame structure and set the 
**	members to a default state.
**	Note that it is not necessary to know how many trim, or field
**	elements that there are when we call this function. Rather
**	trim and fields can be added to it in an adhoc manner
**	and then when the IIFDcfCloseFrame function is called
**	then the actual arrays of pointers to the various 
**	elements of the form are allocated and filled in.
**	In addition it is not necessary to know how large the
**	screen for the form should be, this too will be expanded
**	as the components are entered.
**
** Inputs:
**	char *name;	- name of the frame (copied)
**
** Outputs:
**	Returns:
**		FRAME *	- pointer to the allocated frame structure
**			 NULL if allocation failed.
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
*/

FRAME *
IIFDofOpenFrame(name)
char	*name;
{
	FRAME	*fr;
	TAGID	tag;

	/* NOTE: the frame structure members which we are using
		to hold tags are nat's and the TAGID is really an i2 so,
		although not type correct.. it will work on all systems */

	/* get a tag on which all of the frame structs elements
	   are to be allocated, allocation on this tag is ended
	   in IIFDcfCloseFrame, it is assumed that the caller's of
	   these routines will IIUGtagFree when they are through
	   with the frame */
	tag = FEbegintag();

	/* allocate the frame structure itself */
	fr = FDnewfrm(name);
	
	/* post the tag into the frame struct so that it can be freed later */
	fr->frtag = (i4) tag;

	/* since the frnsno field counter is no longer used, we will use 
	   it to store the allocation tag for the dynamic node storage,
	   these will be freed with the call to IIFDcfCloseFrame */
	fr->frnsno = (i4) FEgettag();
	return fr;
}

/*{
** Name:	IIFDcfCloseFrame - Finish up a dynamically created frame struct.
**
** Description:
**	This function ends the creation of a frame structure.
**	The idea is to convert the linked lists of structure elements
**	by allocating arrays for them, and filling in the arrays.
**	then freeing the link list structures. 
**
** Inputs:
**	FRAME *fr;	- dynamic frame to close off
**
** Outputs:
**	Returns:
**		bool	- TRUE means success
**			  FALSE	 means allocation failure 
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
*/

bool
IIFDcfCloseFrame(FRAME *fr)
{
	TAGID	tag;
	register i4	i;
	register DYNLIST *fldlist;
	register FIELD	**fldarray;
	register DYNLIST *trimlist;
	register TRIM **trimarray;
	register i4	j;
	register DYNLIST *collist;
	register FLDCOL **colarray;
	register TBLFLD *tf;


	/* the call to FDfralloc looks at the frnsno field.. although
	   it probabaly shouldn't?? but to be safe we must get our
	   temporary tag value out of it before calling, 
	   also must retrieve our dynamic list pointers that we 
	   have been storing in the frame structure */
	tag = (TAGID) fr->frnsno;
	fr->frnsno = 0; /* set it to 0 */
	fldlist = (DYNLIST*)fr->frfld;
	trimlist = (DYNLIST*)fr->frtrim;

	/* allocate the trim and field pointer arrays */ 
	if (FDfralloc(fr) == FALSE)
		return (FALSE);
	
	fldarray = fr->frfld;
	trimarray = fr->frtrim;
	
	/* post fields starting at the end of the array, 
	   necessary cause we appended sequence fields to the
	   head of the chain */
	for (i = fr->frfldno; --i >= 0; fldlist	= fldlist->next)
	{
		/* when we encounter a dynamic table field we must place
		   its columns into a newly allocated array */
		if ((fldarray[i] = (FIELD*)fldlist->ptr)->fltag == FCOLUMN)
		{
			/* get table field pointer */
			tf = fldarray[i]->fld_var.fltblfld;

			/* extract the column list starting node */
			collist = (DYNLIST*)tf->tfflds; 

			/* allocate the field column structures */
			if (_col_alloc(tf) == FALSE)
				return (FALSE);
			colarray = tf->tfflds;

			/* post cols starting at the end of the array, 
			   necessary cause we appended sequence cols to the
			   head of the chain */
			for (j = tf->tfcols; --j >= 0; collist= collist->next)
			{
				colarray[j] = (FLDCOL*)collist->ptr;
			}
			
			/* because of the dynamic nature of 
			   table fields.. we wait until now to widen
			   the frame based on the size of the table field */
			_frame_widen(fr, 
				tf->tfhdr.fhposy + tf->tfhdr.fhmaxy - 1,
				tf->tfhdr.fhposx + tf->tfhdr.fhmaxx - 1);

			/* Change tag to show it's a tablefield */
			fldarray[i]->fltag = FTABLE;
		}
	}

	/* post to the trim array */
	for (i = fr->frtrimno; --i >= 0; trimlist = trimlist->next)
	{
		trimarray[i] = (TRIM*)trimlist->ptr;
	}

	/* by adding 1 (?!) is necessary to create a form big enough
	   to contain the components. */
	fr->frmaxx += 1;
	fr->frmaxy += 1;

	/* free the memory used for the DYNLIST nodes */
	IIUGtagFree(tag);

	/* end the tag on which we are allocating the frame structures */ 
	(VOID)FEendtag();

	/*
	**  Set dynamically created flag and move tag to frrngtag so that the
	**  memory allocated here can be reclaimed when IIdelfrm() is called
	**  later on.  Ideally, we should use one tag for each form
	**  allocation but that is not possible given the way FDcrfrm()
	**  is coded - sigh.
	**
	**  Note that the use of frrngtag here prevents dynamically created
	**  forms from using the range query stuff.  If this becomes necessary
	**  this code MUST change.
	*/
	fr->frmflags |= fdISDYN;
	fr->frrngtag = fr->frtag;
	fr->frtag = 0;

	return (TRUE);
}

/*{
** Name:	IIFDattAddTextTrim - Add text trim to dynamically created frame
**
** Description:
**	This function adds a piece of text trim to a dynamically created 
**	frame.
**	It is assumed that the callers of this function will not
**	call it in such as way that fields will be overlapped or
**	violate any other global frame constaints.
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	i4	lin;	- frame line coord of the trim (0 based)
**	i4	col;	- frame col coord of the trim (0 based)
**	i4	flags;	- trim flags
**	char	*str;	- string to use as trim (copied)
**
** Outputs:
**	Returns:
**		TRIM*	- pointer to the trim struct
**			  NULL if allocation error 
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
*/

TRIM *
IIFDattAddTextTrim(FRAME *fr, i4 lin, i4 col, i4 flags, const char *str)
{
	TRIM *trim;

	if (str == NULL || *str == EOS)
		str = ERx(" ");

	/* allocate the trim structure and post the string and coords */
	if ((trim = FDnewtrim(str, lin, col)) == NULL)
		return (NULL);

	/* widen frame dimensions if necessary */
	_frame_widen(fr, lin, col + STlength(str) - 1);

	/* post the trim flags */
	trim->trmflags = flags;

	/* add the trim to the dynamically created list of trim nodes */
	if (_add_list(fr->frnsno, (PTR) trim, (DYNLIST**) &fr->frtrim) 
			== FALSE)
		return (NULL);

	fr->frtrimno++;

	return trim;
}

/*{
** Name:	IIFDqetAddExistTrim - Add an existing piece of time to a frame
**
** Description:
**	Add an existing piece of trim, optionally applying an offset, to
**	a dynamic frame.  It is the caller's responsibility that the trim not
**	overlap another piece of trim or a field.
**
** Inputs:
**	fr	FRAME *		frame to add trim to
**	trim	TRIM *		trim to add
**	xoffset	i4		x-coordinate offset
**	yoffset	i4		y-coordinate offset
**
** Outputs:
**	none
**
**	Returns:
**			OK
**			FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	7/12/89 (Mike S)	Initial version
*/
STATUS
IIFDaetAddExistTrim(fr, trim, xoffset, yoffset)
FRAME	*fr;
TRIM	*trim;
i4	xoffset;
i4	yoffset;
{
	i4 trimwidth;
	i4 trimheight;
	char boxstring[50];

	/*
	** For box trim, parse the width and height from the string.  Other
	** trim has height 1 and width equal to the string length.
	*/
	if ((trim->trmflags & fdBOXFLD ) != 0)
	{
		if (STscanf(trim->trmstr, ERx("%d:%d"), 
			    &trimheight, &trimwidth) != 2)
			return (FAIL);	/* Something's wrong */
		/* Generate a new string, to be SURE we're unique */
		STprintf(boxstring, ERx("%d:%d:%d"), trimheight, 
			 trimwidth, fr->frtrimno);
		trim->trmstr = FEsalloc(boxstring);
	}
	else
	{
		trimheight = 1;
		trimwidth = STlength(trim->trmstr);
	}

	/* Link trim onto dynamic list */
	if (! _add_list(fr->frnsno, (PTR)trim, (DYNLIST **) &fr->frtrim))
		return FAIL;

	/* Widen frame, if need be */
	trim->trmx += xoffset;
	trim->trmy += yoffset;
	_frame_widen(fr, trim->trmy + trimheight - 1, 
			 trim->trmx + trimwidth - 1);

	/* Increment trim count */
	fr->frtrimno++;

	return OK;
}

/*{
** Name:	IIFDabtAddBoxTrim - Add box trim to a dynamically created frame
**
** Description:
**	This function adds a piece of box trim to a dynamically created 
**	frame.
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	i4	lin;	- frame line coord of the trim (0 based)
**	i4	col;	- frame col coord of the trim (0 based)
**	i4	endlin;	- end line frame coord of the trim (0 based)
**	i4	endcol;	- end col frame coord of the trim (0 based)
**	i4	flags;	- trim flags (not necessary to post
**				the BOX attribute.. this routine does that)
**
** Outputs:
**	Returns:
**		TRIM*	- pointer to the trim struct
**			  NULL if allocation error 
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
**	7/13/89 (Mike S) - Add unique identifier to box trim string
*/

TRIM *
IIFDabtAddBoxTrim(FRAME *fr, i4 lin, i4 col, i4 endlin, i4 endcol, i4 flags)
{
	char str[50];
	TRIM *trim;

	/* create the format string giving the box dimensions */
	STprintf(str, ERx("%d:%d:%d"), endlin - lin + 1, endcol - col + 1,
		fr->frtrimno);

	/* allocate the trim structure and post the string and coords */
	if ((trim = FDnewtrim(str, lin, col)) == NULL)
		return (NULL);
	
	/* widen frame dimensions if necessary */
	_frame_widen(fr, endlin, endcol);

	/* post the trim flags */
	trim->trmflags = flags | fdBOXFLD;

	/* add the trim to the dynamically created list of trim nodes */
	if (_add_list(fr->frnsno, (PTR) trim, (DYNLIST**) &fr->frtrim)
			== FALSE)
		return (NULL);

	fr->frtrimno++;

	return trim;
}

/*{
** Name:	IIFDacfAddCharFld - Add a character field to a dynamic frame
**
** Description:
**	This function adds a character field to a dynamically created frame.
**	It is assumed that the callers of this function will not
**	call it in such as way that fields will be overlapped or
**	violate any other global frame constaints.
**	The idea is to set up all of the aspects of the field structure
**	according to the following assumptions:
**		- field is to be left justified 
**		- field has a data type of varchar
**		- field is not nullable
**		- no validation is supported in this interface
**		- no default value is supported by this interface
**		- the field gets the next available sequence number 
**		  so the caller must add fields in sequence order
**		- does not support calculations required for boxed fields.
**
**	(after this function returns the caller can fill in
**	validation strings, default values etc.)
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	char	*name;	- internal name of the field  (copied)
**	i4	titlin;	- title's lin frame position (0 based)
**	i4	titcol;	- title's col frame position (0 based)
**	char	*title;	- title string (copied)
**	i4	lin;	- frame line position of the data window (0 based)
**	i4	col;	- frame col position of the data window (0 based)
**	i4	width;	- screen width of the field's data window
**	i4	nlines;	- number of lines that the character field consumes
**	i4	dlen;	- length of string (not including null byte, and length
**			  byte associated with a varchar data type)
**	i4	flags;	- display flags (note that the scroll flag is 
**			  detected by noticing that width*nlines != dlen)
**
** Outputs:
**	Returns:
**		REGFLD*	- returns pointer to the new field struct
**			  NULL if bad allocation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
*/

REGFLD *
IIFDacfAddCharFld(fr, name, titlin, titcol, title, 
			lin, col, width, nlines, dlen, flags)
FRAME	*fr;
char	*name;
i4	titlin;
i4	titcol;
char	*title;
i4	lin;
i4	col;
i4	width;
i4	nlines;
i4	dlen;
i4	flags;
{
	return _add_fld(fr, name, titlin, titcol, title, 
			lin, col, width, nlines, DB_VCH_TYPE, dlen, flags);
}

/*{
** Name:	IIFDaifAddIntFld - Add a integer field to a dynamic frame
**
** Description:
**	This function adds a integer field to a dynamically created frame.
**	It is assumed that the callers of this function will not
**	call it in such as way that fields will be overlapped or
**	violate any other global frame constaints.
**	The idea is to set up all of the aspects of the field structure
**	according to the following assumptions:
**		- field is to be left justified 
**		- field has a data type of i4 
**		- field is not nullable
**		- no validation is supported in this interface
**		- no default value is supported by this interface
**		- the field gets the next available sequence number 
**		  so the caller must add fields in sequence order
**		- does not support calculations required for boxed fields.
**
**	(after this function returns the caller can fill in
**	validation strings, default values etc.)
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	char	*name;	- internal name of the field  (copied)
**	i4	titlin;	- title's line frame position (0 based)
**	i4	titcol;	- title's col frame position (0 based)
**	char	*title;	- title string (copied)
**	i4	lin;	- frame line position of the data window (0 based)
**	i4	col;	- frame col position of the data window (0 based)
**	i4	width;	- width of the frame window
**	i4	flags;	- display flags
**
** Outputs:
**	Returns:
**		REGFLD*	- returns pointer to the new field struct
**			  NULL if bad allocation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
*/

REGFLD *
IIFDaifAddIntFld(fr, name, titlin, titcol, title, lin, col, width, flags) 
FRAME	*fr;
char	*name;
i4	titlin;
i4	titcol;
char	*title;
i4	lin;
i4	col;
i4	width;
i4	flags;
{
	return _add_fld(fr, name, titlin, titcol, title, 
			lin, col, width, 1, DB_INT_TYPE, 4, flags);
}

/*{
** Names:	IIFDatfAddTblFld- Add a table field to a dynamic frame
**
** Description:
**	These functions add a table field for insertion of 
**	columns as part of dynamic frame creation.
**	It is assumed that the callers of these function will not
**	call them in such as way that fields will be overlapped or
**	violate any other global frame constaints.
**	The idea is to set up all of the aspects of the table field
**	structure according to the following assumptions:
**		- each time a column is added the table field header struct
**		  is updated with the new width given the 
**		  column characteristics.
**		- the table field does not display row separator lines 
**		- the same number of columns will be added to the table 
**		  as are indicated in the input argument ncols.. it
**		  is not necessary that this be openended.
**		- the highlight current row is set
**		- no validation is supported in this interface
**		- no default value is supported by this interface
**
*	(after this function returns the caller can fill in
**	validation strings, default values etc.)
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	char	*name;	- internal name of the field (copied)
**	i4	lin;	- frame line position of the upper left corner of the
**			  data window box (0 based)
**	i4	col;	- frame col position of the upper left corner of the
**			  data window box (0 based)
**	i4	nrows;	- number of rows in the table field
**	bool    title;  - whether to title the columns
**	bool    lines;  - whether to separate rows with lines
**
** Outputs:
**	Returns:
**		TBLFLD*	- returns pointer to the new field struct
**			  NULL if bad allocation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
**	7/11/89 (Mike S) - title argument
**	12/17/89 (Mike S) - lines argument
*/

TBLFLD *
IIFDatfAddTblFld(fr, name, lin, col, nrows, title, lines) 
FRAME	*fr;
char	*name;
i4	lin;
i4	col;
i4	nrows;
bool	title;
bool	lines;
{
	FIELD	*fld;
	TBLFLD	*tf;
	FLDHDR	*hdrp;

	/* allocate the new table field struct */
	if ((fld = FDnewfld(name, fr->frfldno, FTABLE)) == NULL)
		return (NULL);

	/* assign some convenience pointers */
	tf = fld->fld_var.fltblfld;
	hdrp = &tf->tfhdr;

	tf->tfrows = nrows;	/* set the number of data rows in the tf */
	tf->tfstart = (title ? 3 : 1);	/* default */

	/* start out the width at 1.. may be increased if multi-line columns
	   are added.. (this really should be called tfheight) */
	tf->tfwidth = 1;

	hdrp->fhtitle = _empty; /* no title */
	hdrp->fhtitx =
	hdrp->fhtity = 1;	/* default */
	hdrp->fhposx = col;
	hdrp->fhposy = lin;

	/* starting default for maxx is 1 + FTspace(), this assumes that each
	   added column (of which there must be at least one) adds
	   it's 'width' + one for the column divider + attribute bytes for
	   IBM 3270 (if nec.) */
	hdrp->fhmaxx = 1 + FTspace();

	/* Number of lines in tablefield:
	**	1. nrows (We'll begin by assuming one line per row)
	**	2. 2 for top and bottom lines.
	**	3. 2 more if we have column titles.
	**	4. nrows - 1 if we have row separators
	**
	** If a multi-line column is added which is more than tfwidth
	** in height.. then it will recalculate fhmaxy. 
	*/
	hdrp->fhmaxy = nrows + 2 + (title ? 2 : 0) + (lines ? (nrows-1) : 0);
	
	/* add the field to the dynamically created list of fields */
	if (_add_list(fr->frnsno, (PTR) fld, (DYNLIST**) &fr->frfld)
			== FALSE)
		return (NULL);

	/* set the flags according to our default assumptions */
	hdrp->fhdflags = 
		  fdROWHLIT 	/* highlight the current row */
		| fdBOXFLD;	/* it's a boxed field */

	/* Set the title and line flags according to our instructions */
	if (!title) hdrp->fhdflags |= fdNOTFTITLE; 	/* no title */
	if (!lines) hdrp->fhdflags |= fdNOTFLINES;	/* no lines */

	/* bump the number of fields in the frame */
	fr->frfldno++;

	/* Change the tag to FCOLUMN, so we know it's a dynamic tablefield */
	fld->fltag = FCOLUMN;

	return (tf);
}

/*{
** Name:	IIFDaefAddExistFld	- Add an existing field to a frame
**
** Description:
**	Add an existing field structure to a dynamic frame.  This field
**	structure might be generated by IIFDffFormatField or might come
**	from an existing frame structure.  Offsets are applied to (optionally) 
**	move the field into a new position.  
**	
**	The usual caveats apply; the caller must insure that this new field 
**	doesn't overlap any previously added fields.  Only simple fields are
**	supported by this interface.
**
** Inputs:
**	fr		FRAME *	frame to add field to
**	field		FIELD *	field to add (modified)
**	xoffset		i4	x offset to apply to field
**	yoffset		i4	y offset to apply to field
**
** Outputs:
**	none
**
**	Returns:
**		STATUS		OK
**				FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/11/89 (Mike S)	Initial version
*/
STATUS
IIFDaefAddExistFld(fr, field, xoffset, yoffset)
FRAME	*fr;
FIELD	*field;
i4	xoffset;
i4	yoffset;
{
	register FLDHDR *hdrp = &field->fld_var.flregfld->flhdr;
	i4 	maxx;
	i4	maxy;

	/* Add field to dynamic list */
	if (! _add_list(fr->frnsno, (PTR)field, (DYNLIST **)&fr->frfld))
		return (FAIL);

	/* Apply offsets to field */
	hdrp->fhposx += xoffset;
	hdrp->fhposy += yoffset;

	/* widen frame dimensions, if need be */
	maxx = hdrp->fhposx + hdrp->fhmaxx - 1;
	maxy = hdrp->fhposy + hdrp->fhmaxy - 1;
	_frame_widen(fr, maxy, maxx);

	/* Check for a scrolling field */
	if ((hdrp->fhd2flags | fdSCRLFD) != 0)
		fr->frmflags |= fdSCRLFD;

	/* Set field sequence number */
	hdrp->fhseq = fr->frfldno++;

	return (OK);
}

/*{
** Name:	IIFDaexAddExistTf	- Add an existing tablefield to a frame
**
** Description:
**	Add an existing tablefield structure to a dynamic frame.  This 
**	tablefield must be complete before this routine is called; changes
**	to it won't be reflected in the parent FRAME structure.
**	
**	The usual caveats apply; the caller must insure that this new field 
**	doesn't overlap any previously added fields.  
**
** Inputs:
**	fr		FRAME *	frame to add field to
**	field		FIELD *	tablefield to add (modified)
**	xoffset		i4	x offset to apply to tablefield
**	yoffset		i4	y offset to apply to tablefield
**
** Outputs:
**	none
**
**	Returns:
**		STATUS		OK
**				FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/17/89 (Mike S)	Initial version
*/
STATUS
IIFDaexAddExistTf(fr, field, xoffset, yoffset)
FRAME	*fr;
FIELD	*field;
i4	xoffset;
i4	yoffset;
{
	TBLFLD	*tf = field->fld_var.fltblfld;	
	register FLDHDR *hdrp = &tf->tfhdr;
	i4 	maxx;
	i4	maxy;
	i4 	i;

	/* Add field to dynamic list */
	if (! _add_list(fr->frnsno, (PTR)field, (DYNLIST **)&fr->frfld))
		return (FAIL);

	/* Apply offsets to field */
	hdrp->fhposx += xoffset;
	hdrp->fhposy += yoffset;

	/* widen frame dimensions, if need be */
	maxx = hdrp->fhposx + hdrp->fhmaxx - 1;
	maxy = hdrp->fhposy + hdrp->fhmaxy - 1;
	_frame_widen(fr, maxy, maxx);

	/* Check for a scrolling column */
	for (i = 0; i < tf->tfcols; i++)
	{
		if ((tf->tfflds[i]->flhdr.fhd2flags | fdSCRLFD) != 0)
		{
			fr->frmflags |= fdSCRLFD;
			break;
		}
	}

	/* Set field sequence number */
	hdrp->fhseq = fr->frfldno++;

	return (OK);
}

/*{
** Name:	IIFDaccAddCharCol - Add a character field to a table field
**
** Description:
**	This function adds a character column to a table field
**	of a dynamically created frame.
**	The idea is to set up all of the aspects of the column structure
**	according to the following assumptions:
**		- column is to be left justified 
**		- column has a data type of varchar
**		- column is not nullable
**		- no validation is supported in this interface
**		- no default value is supported by this interface
**		- it is assumed that the table field was allocated with
**		  enough room for this column.
**		- the column is assigned to the next position in the table
**		  field, assigned from left to right.
**		- the table field struct is updated with the new dimensions
**		  that the addition of this column necessitates.
**
**	(after this function returns the caller can fill in
**	validation strings, default values etc.)
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	TBLFLD	*tf;	- table field that this column is to be placed in
**	char	*name;	- internal name of the field  (copied)
**	char	*title; - title for column; ignored if tablefield doesn't have
**			  column titles; copied otherwise 
**	i4	width;	- width of the frame window
**	i4	nlines;	- number of lines that the character field consumes
**	i4	dlen;	- length of string (not including null byte, and length
**			  byte associated with a varchar data type)
**	i4	flags;	- display flags (note that the scroll flag is 
**			  detected and set by noticing that 
**			  width*nlines != dlen, meaning that it is not
**			  necessary to set the scroll flag)
**
** Outputs:
**	Returns:
**		FLDCOL * - returns pointer to the new column struct
**			  NULL if bad allocation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
*/

FLDCOL *
IIFDaccAddCharCol(fr, tf, name, title, width, nlines, dlen, flags)
FRAME	*fr;
TBLFLD	*tf;
char	*name;
char	*title;
i4	width;
i4	nlines;
i4	dlen;
i4	flags;
{
	return _add_col(fr, tf, name, title, width, nlines, 
			DB_VCH_TYPE, dlen, flags);
}

/*{
** Name:	IIFDaicAddIntCol - Add a integer column to a table field
**
** Description:
**	This function adds a integer field to a dynamically created frame.
**	The idea is to set up all of the aspects of the field structure
**	according to the following assumptions:
**		- column is to be left justified 
**		- column has a data type of i4 
**		- column is not nullable
**		- no validation is supported in this interface
**		- no default value is supported by this interface
**		- it is assumed that the table field was allocated with
**		  enough room for this column.
**		- the column is assigned to the next position in the table
**		  field, assigned from left to right.
**		- the table field struct is updated with the new dimensions
**		  that the addition of this column necessitates.
**
**	(after this function returns the caller can fill in
**	validation strings, default values etc.)
**
** Inputs:
**	FRAME	*fr;	- dynamically created frame
**	TBLFLD	*tf;	- table field that this column is to be placed in
**	char	*name;	- internal name of the field  (copied)
**	char	*title; - title for column; ignored if tablefield doesn't have
**			  column titles; copied otherwise
**	i4	width;	- width of the frame window
**	i4	flags;	- display flags (note that the scroll flag is 
**			  detected by noticing that width*nlines != dlen)
**
** Outputs:
**	Returns:
**		FLDCOL * - returns pointer to the new column struct
**			  NULL if bad allocation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/16/89 (tom) - spec written
**	03/17/89 (tom) - coded
*/

FLDCOL *
IIFDaicAddIntCol(fr, tf, name, title, width, flags) 
FRAME	*fr;
TBLFLD	*tf;	
char	*name;
char	*title;
i4	width;
i4	flags;
{
	return _add_col(fr, tf, name, title, width, 1, DB_INT_TYPE, 4, flags);
}

/*{
** Name:	IIFDaecAddExistCol - Add existing column to tablefield.
**
** Description:
**	Add an existing column structure to a dynamic tablefield.  This column
**	structure might be generated by IIFDffFormatColumn or might come
**	from an existing tablefield structure. The column is placed at the 
**	current rightmost position in the tablefield.
**
**	If the tablefield doesn't contain column titles, any column title 
**	will be dropped.
**
** Inputs:
**	fr	FRAME *		dynamic frame
**	tf	TBLFLD *	Tablefield to add column to
**	column	FLDCOL *	Column to add
**
** Outputs:
**	none
**
**	Returns:
**		STATUS		OK
**				FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/11/89 (Mike S)	Initial version
*/
STATUS
IIFDaecAddExistCol(fr, tf, column)
FRAME	*fr;
TBLFLD	*tf;
FLDCOL	*column;
{
	bool	coltitles = 	(tf->tfhdr.fhdflags & fdNOTFTITLE) == 0;
	bool	lines = 	(tf->tfhdr.fhdflags & fdNOTFLINES) == 0;
	register FLDHDR 	*hdrp = &column->flhdr;
	register FLDTYPE	*typep = &column->fltype;
	i4 nlines;		/* number of lines for column */

	/* Add the column to the tablefield column list */
	if (! _add_list(fr->frnsno, (PTR)column, (DYNLIST **) &tf->tfflds))
		return (FAIL);

	/*
	** If the tablefield has column titles, add one, if need be.  If the
	** tablefield lacks them, insure there isn't one.
	*/
	if (coltitles)
	{
		if (hdrp->fhtitle == NULL) hdrp->fhtitle = _empty;
		if (hdrp->fhtity < 0) hdrp->fhtity = 1;
	}
	else
	{
		hdrp->fhtitle = _empty;
		hdrp->fhtity = -1;
	}

	/* Place column at right of tablefield, one line high */
	hdrp->fhposx = hdrp->fhtitx = typep->ftdatax = tf->tfhdr.fhmaxx;
	hdrp->fhposy = 0;
	hdrp->fhmaxy = 1;

	/* if the number of lines in this column is greater than
	** the number of lines in the tallest column we have seen so
	** far, then we must increase the table field's y dimension.
	** In addition to the data rows, we have 2 rows for an untitled
	** tablefield and 4 for a titled one and possibly nrows-1 for separators
	*/ 
	nlines = (typep->ftwidth  + typep->ftdataln - 1) / typep->ftdataln;
	if (nlines > tf->tfwidth)
	{
		tf->tfwidth = nlines;
		tf->tfhdr.fhmaxy = tf->tfrows * nlines + 2 + 
				   (coltitles ? 2 : 0) + 
				   (lines ? (tf->tfrows-1) : 0);
	}

	/* Bump the tablefield's size, allowing space for two separators */
	tf->tfhdr.fhmaxx += hdrp->fhmaxx + 2 * FTspace() + 1;

	/* Check for a scrolling column */
	if ((hdrp->fhd2flags | fdSCRLFD) != 0)
		fr->frmflags |= fdSCRLFD;

	/* Set the column sequence counter */
	hdrp->fhseq = tf->tfcols++;

	return (OK);
}

/**** functions below are local to this file ****/ 



/*{
** Name:	_add_fld	- add a field to the frame
**
** Description:
**	Subroutine to do much of the work of adding a regular field
**	to the frame (this is called for character and integer fields).
**
** Inputs:
**	FRAME	*fr;	- frame to add the new field or column to
**	TBLFLD	*tf;	- table field to add column to (NULL = regular field)
**	char	*name;	- internal name of the field  (copied)
**	i4	titlin;	- title's lin frame position (0 based)
**	i4	titcol;	- title's col frame position (0 based)
**	char	*title;	- title string (copied)
**	i4	lin;	- frame lin position of the data window (0 based)
**	i4	col;	- frame col position of the data window (0 based)
**	i4	width;	- screen width of the field's data area
**	i4	nlines;	- number of lines the field consumes on the screen
**	i4	type;	- type of data
**	i4	dlen;	- length of the buffer to contain the data
**	i4	flags;	- field flags
**
** Outputs:
**	Returns:
**		REGFLD*	- return pointer to the field struct
**			 NULL if allocation error
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/17/89 (tom) - written
*/

LOC REGFLD*
_add_fld(fr, name, titlin, titcol, title, 
			lin, col, width, nlines, type, dlen, flags)
FRAME	*fr;
char	*name;
i4	titlin;
i4	titcol;
char	*title;
i4	lin;
i4	col;
i4	width;	
i4	nlines;
i4	type;
i4	dlen;
i4	flags;	
{
	FIELD	*field;
	FLDHDR	*hdrp;
	FLDTYPE	*typep;
	FLDVAL	*valp;
	register i4	beglin;
	register i4	begcol;
	bool	scroll;
	i4	endlin;
	i4	endcol;
	i4	endtitcol;
	i4	enddatcol;

	/* allocate for the field structures */
	if ((field = FDnewfld(name, fr->frfldno, FREGULAR)) == NULL)
		return (NULL);

	/* add the field to the dynamically created list of fields */
	if (_add_list(fr->frnsno, (PTR) field, (DYNLIST**) &fr->frfld)
			== FALSE)
		return (NULL);

	/* assign structure member pointers */
	hdrp =  &field->fld_var.flregfld->flhdr;
	typep = &field->fld_var.flregfld->fltype;
	valp = &field->fld_var.flregfld->flval;

	/* allocate, copy and assign the title string */
	if ((hdrp->fhtitle = FEsalloc(title)) == NULL)
		return (NULL);

	/* calculate and post the start x and y of the whole field window */
	hdrp->fhposx = begcol =  (titcol < col) ? titcol : col;
	hdrp->fhposy = beglin =  (titlin < lin) ? titlin : lin;
	
	/* post the field relative positions of the title and data window */
	hdrp->fhtitx = titcol - begcol;
	hdrp->fhtity = titlin - beglin;
	typep->ftdatax = col - begcol;
	valp->fvdatay = lin - beglin;

	/* calculate the size of the field's window */
	endtitcol = titcol + STlength(title) - 1;
	enddatcol = col + width - 1;
	endcol = (endtitcol > enddatcol) ? endtitcol : enddatcol; 
	endlin = (titlin > lin + nlines - 1) ? titlin : lin + nlines - 1; 
	hdrp->fhmaxx = endcol - begcol + 1; 
	hdrp->fhmaxy = endlin - beglin + 1;
	
	/* If no color is specified, use color 1 */
	if ((flags & fdCOLOR) == 0)
		flags |= fd1COLOR;

	/* set the header flags */
	hdrp->fhdflags = flags;

	/* set the type and if it's scrollable then set flags */
	if (_set_type(typep, type, dlen, width, nlines, &scroll) == FALSE)
		return (NULL);
	if (scroll)
	{
		hdrp->fhd2flags |= fdSCRLFD;
		fr->frmflags |= fdSCRLFD;
	}

	/* widen frame dimensions if necessary */
	_frame_widen(fr, endlin, endcol);

	/* bump the number of fields on this form */
	fr->frfldno++;

	return field->fld_var.flregfld;
}

/*{
** Name:	_add_col	- add a column to a table field
**
** Description:
**	Subroutine to do much of the work of adding a regular field
**	to the frame (this is called for character and integer fields).
**
** Inputs:
**	FRAME	*fr;	- frame to add the new field or column to
**	TBLFLD	*tf;	- table field to add column to (NULL = regular field)
**	char	*name;	- internal name of the field  (copied)
**	char	*title; - title for column; ignored if tablefield doesn't have
**			  column titles; copied otherwise.
**	i4	width;	- screen width of the field's data area
**	i4	nlines;	- number of lines the field consumes on the screen
**	i4	type;	- type of data
**	i4	dlen;	- length of the buffer to contain the data
**	i4	flags;	- field flags
**
** Outputs:
**	Returns:
**		FLDCOL*	- return pointer to the new column struct
**			 NULL if allocation error
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/17/89 (tom) - written
*/

LOC FLDCOL *
_add_col(fr, tf, name, title, width, nlines, type, dlen, flags)
FRAME	*fr;
TBLFLD	*tf;
char	*name;
char	*title;
i4	width;	
i4	nlines;
i4	type;
i4	dlen;
i4	flags;	
{
	FLDCOL	*fldcol;
	register FLDHDR	*hdrp;
	FLDTYPE	*typep;
	i4	colwidth;
	bool	scroll;
	bool	coltitles = (tf->tfhdr.fhdflags & fdNOTFTITLE) == 0;
	bool	lines = (tf->tfhdr.fhdflags & fdNOTFLINES) == 0;
	
	/* allocate the FLDCOL structure */ 
	if ((fldcol = FDnewcol(name, tf->tfcols)) == NULL)
		return (NULL);

	/* assign structure member pointers */
	hdrp = &fldcol->flhdr;
	typep = &fldcol->fltype;

	/* add the column to the dynamically created list of columns */
	if (_add_list(fr->frnsno, (PTR) fldcol, (DYNLIST**) &tf->tfflds)
			== FALSE)
		return (NULL);

	/* 
	** If the tablefield supports column titles use it.  Otherwise, 
	** set title to an empty string.
	*/
	if (coltitles)
	{
		hdrp->fhtitle = FEsalloc(title);
		hdrp->fhtity = 1;
	}
	else
	{
		hdrp->fhtitle = _empty;
		hdrp->fhtity = -1;
	}

	/* all three col values start at the same column */
	hdrp->fhposx =  
	hdrp->fhtitx = 
	typep->ftdatax = tf->tfhdr.fhmaxx;

	/*
	** the column width is 1 plus the larger of:
	** 1.	The data size plus two FTspaces for the protected
	**	field bytes which surround the separator line on an IBM 3270.
	** and
	** 2.	The title size.
	*/
	colwidth = max(STlength(hdrp->fhtitle), width + 2 * FTspace());
	tf->tfhdr.fhmaxx += colwidth + 1;

	/* if the number of lines in this column is greater than
	** the number of lines in the tallest column we have seen so
	** far, then we must increase the table field's y dimension.
	** In addition to the data rows, we have 2 rows for an untitled
	** tablefield and 4 for a titled one and possibly nrows-1 for separators
	*/ 
	if (nlines > tf->tfwidth)
	{
		tf->tfwidth = nlines;
		tf->tfhdr.fhmaxy = tf->tfrows * nlines + 2 + 
				   (coltitles ? 2 : 0) + 
				   (lines ? (tf->tfrows-1) : 0);
	}

	/* y positions just get set to default values */
	hdrp->fhposy = 0; 

	/* post the size of the field's window */
	hdrp->fhmaxx = colwidth;
	hdrp->fhmaxy = 1;	/* default?? */
	
	/* If no color is specified, use color 1 */
	if ((flags & fdCOLOR) == 0)
		flags |= fd1COLOR;

	/* set the header flags */
	hdrp->fhdflags = flags;

	/* set the type and if it's scrollable then set flags */
	if (_set_type(typep, type, dlen, width, nlines, &scroll) == FALSE)
		return (NULL);
	if (scroll)
	{
		hdrp->fhd2flags |= fdSCRLFD;
		fr->frmflags |= fdSCRLFD;
	}

	/* bump the table field's column counter */
	tf->tfcols++;

	return (fldcol);
}

/*{
** Name:	_set_type	- set FLDTYPE struct
**
** Description:
**	Common routine to set FLDTYPE structure elements.
**
** Inputs:
**	FLDTYPE	*typep;		- pointer to type struct
**	i4	type;		- data type
**	i4	dlen;		- data length
**	i4	width;		- screen width
**	i4	nlines;		- number of lines
**	bool	*scroll;	- pointer to caller's flag so
**				  we can post whether the field scrolls
**
** Outputs:
**	Returns:
**		bool	- TRUE if the field scrolls else FALSE
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/19/89 (tom) - written	
**	7/12/89 (Mike S) Fix calculation of ftdataln
*/
LOC bool
_set_type(typep, type, dlen, width, nlines, scroll)
FLDTYPE	*typep;
i4	type;
i4	dlen;
i4	width;
i4	nlines;
bool	*scroll;
{
	char	fmtstr[50];

	/* initial assumption is that the field doesn't scroll */
	*scroll = FALSE;

	/* set various strings to null */
	typep->ftvalstr = _empty;
	typep->ftvalmsg = _empty;
	typep->ftdefault = _empty;
	typep->ftwidth = width;

	switch (type)		/* set the type dependent members */
	{
	case DB_VCH_TYPE:
		/* add in the length bytes of a varchar type */
		typep->ftlength = 2 + dlen;

		/* if > 1 line, then use different STprintf format */
		if (nlines > 1)
		{
			STprintf(fmtstr, ERx("c%d.%d"), dlen, width); 
			/* if it's a multiline field then the width
			   must account for all the lines??? */
			typep->ftwidth = width;
			
		}
		else	/* field is only 1 line high */
		{	
			/* if string length is > than the screen width */
			if (dlen > width)
			{
				/* set scroll flag */
				*scroll = TRUE;
			}
			STprintf(fmtstr, ERx("c%d"), width); 
		}
		break;

	case DB_INT_TYPE:
		STprintf(fmtstr, ERx("i%d"), width); 
		typep->ftlength = dlen; 
		break;
	}

	/* post the screen width of the data window */
	typep->ftdataln = (width + nlines - 1) / nlines;

	/* post the format string */
	if ((typep->ftfmtstr = FEsalloc(fmtstr)) == NULL)
		return FALSE; 

	/* post the field/column's data type */
	typep->ftdatatype = type;

	return (TRUE);
}

/*{
** Name:	_add_list	- add a node to a list
**
** Description:
**	Allocate for and add a node to the list of nodes.
**	The list nodes are allocated using ME functions so that
**	they may be freed.
**
** Inputs:
**	PTR	ptr;	pointer to place in the new node
**	DYNLIST **list;	pointer to the pointer to the list of nodes 
**
** Outputs:
**	Returns:
**		bool	- TRUE means all was OK
**			  FALSE means all was OK
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/17/89 (tom) - spec written
*/

LOC bool
_add_list(tag, ptr, list)
i4	tag;
register PTR	ptr;
register DYNLIST **list;
{
	register DYNLIST *node;
	
	if ( (node = (DYNLIST*) FEreqmem((u_i4)tag, (u_i4)sizeof(DYNLIST),
			FALSE, (STATUS *)NULL)) == NULL)
		return (FALSE);
	
	node->ptr = ptr;
	node->next = *list;
	*list = node;
	return (TRUE);
}

/*{
** Name:	_frame_widen	- widen a frame if necessary
**
** Description:
**	Given an ending col and lin coord of a component being added..
**	adjust the frame's frmaxx and frmaxy elements so that
**	the frame will be big enough to contain the element.
**
** Inputs:
**	FRAME *fr;	- frame to be adjusted 
**	 i4	endlin;	- ending line position to test
**	 i4	endcol;	- ending col position to test
**
** Outputs:
**	Returns:
**		VOID
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/19/89 (tom) - written
*/

LOC VOID 
_frame_widen(fr, endlin, endcol)
register FRAME *fr;
register i4	endlin;
register i4	endcol;
{
	
	if (fr->frmaxy < endlin)
		fr->frmaxy = endlin;
	if (fr->frmaxx < endcol)	
		fr->frmaxx = endcol;
}

/*{
** Name:	_col_alloc - allocate column array
**
** Description:
**	Given a ptr to a table field struct.. allocate
**	an array of pointers to the column structs.
**
** Inputs:
**	TBLFLD *tf;	- table field to allocate columns for
**
** Outputs:
**	Returns:
**		VOID
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/20/89 (tom) - spec written
*/

LOC bool 
_col_alloc(tf)
register TBLFLD *tf;
{
	
	if ((tf->tfflds = (FLDCOL**) FEreqmem((u_i4)0, 
			(u_i4)sizeof(FLDCOL*) * tf->tfcols, FALSE,
			(STATUS*) NULL)) == NULL)
		return (FALSE);
	return (TRUE);
}
