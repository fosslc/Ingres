/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	iitbfind.c - Perform single keystroke find.
**
** Description:
**	Perform single keystroke find.  User has (legitimately) requested
**	a single character Find in the current table field column.
**
**	IITBscfSingleChrFind() is used to handle the request.
**
** History:
**	01-feb-90 (bruceb)
**		Written--much code used from listpick's do_find() routine.
**	03-apr-90 (bruceb)
**		Check if table field display is empty before calling IItbsetio.
**		Some duplicate work, but less if I dup'd contents of IItbsetio.
**	27-apr-90 (bruceb)
**		Decision is now that searching for char that is only on the
**		current row will not beep.  So, don't.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      04/23/96 (chech02)
**              fixed compiler compliant for windows 3.1 port.
**	31/12/97 (kitch01)
**		Bug 87938. Add call to FDrsbRefreshScrollButton so that MWS can
**		reposition the scrollbar thumbwheel to the found rows position.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**/


# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<runtime.h>
# include	<cm.h>
# include	<er.h>

FUNC_EXTERN	COLDESC	*IIcdget();
FUNC_EXTERN	i4	IItcogetio();

GLOBALREF	TBSTRUCT	*IIcurtbl;


/*{
** Name:	IITBscfSingleChrFind - Perform single keystroke find.
**
** Description:
**	Find row following single character "find" function, and scroll to it.
**	Rows are requested based on typing the first non-blank character in the
**	column.  Since column may be of any character type, go through the ADT
**	interface to convert into string for comparison.  If no match is found
**	for the specified character, then 'beep' to indicate an error.
**
**	Blanks are ignored, matches are case-insensitive, and NULL rows
**	are ignored.
**
** Inputs:
**	frm	Frame on which search occurs.
**	chr	Character on which to base the search.  Possibly doublebyte.
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		None.
**
** Side Effects:
**	Issue a beep or scroll to the line found.
**
** History:
**	01-feb-90 (bruceb)
**		Written--much code used from listpick's do_find() routine.
*/
VOID
IITBscfSingleChrFind(frm, chr)
FRAME	*frm;
char	*chr;
{
	bool		found = FALSE;
	i4		crec;
	i4		colno;
	i4		rowno;
	i4		col_offset;
	char		*form;
	TBLFLD		*tfld;
	TBSTRUCT	*tb;
	DATSET		*ds;
	COLDESC		*col;
	TBROW		*row;
	TBROW		*crow;
	ADF_CB		*cb;
	i2		nullind;
	char		*ptr;
	char		txt[DB_GW4_MAXSTRING + 1];
	DB_DATA_VALUE	dbv;
	DB_EMBEDDED_DATA	edv;

	cb = FEadfcb();

	form = frm->frname;

	tfld = frm->frfld[frm->frcurfld]->fld_var.fltblfld;

	if (((tb = IItstart(tfld->tfhdr.fhdname, form)) == NULL)
	    || (tb->tb_display == 0))
	{
	    /* If no rows are displayed, than error out. */
	    FTbell();
	    return;
	}

	/*
	** Perform a 'getrow'.  Has the side effect of setting table field's
	** 'crow'.
	*/
	if (IItbsetio(2, form, tfld->tfhdr.fhdname, -2) == 0)
	{
	    FTbell();
	    return;
	}
	ds = tb->dataset;
	if (!ds || !ds->top)
	{
	    /* Bare table field or empty dataset. */
	    /*
	    ** Shouldn't ever be an empty dataset due to earlier test of
	    ** tb_display, but what the heck.
	    */
	    FTbell();
	    return;
	}
	crow = ds->crow;
	IItcogetio((short *)NULL, 1, DB_INT_TYPE, 4, &crec, ERx("_record"));

	/*
	** Stash away the current table field column so that it can be
	** restored after 'scroll to' is used (which changes to the first
	** column.)
	*/
	colno = tfld->tfcurcol;

	col = IIcdget(tb, tfld->tfflds[colno]->flhdr.fhdname);
	MEcopy((PTR) &(col->c_dbv), (u_i2) sizeof(DB_DATA_VALUE), (PTR) &dbv);
	col_offset = col->c_offset;

	/*
	** Set up structure to convert idbv into a string through
	** an embedded datatype.  This is wasteful, given that we are only
	** interested in the first character, but special casing for all the
	** various types would be nasty and would involve examining the
	** dbv internal structure for each character type.
	*/
	edv.ed_type = DB_CHR_TYPE;
	edv.ed_null = &nullind;
	edv.ed_length = DB_GW4_MAXSTRING + 1;
	edv.ed_data = (PTR)txt;

	/*
	** Now unload table.  First search those rows in the dataset that
	** follow the current row.  Then, if not found, search those rows
	** that precede it.
	*/
	for (rowno = crec + 1, row = crow->nxtr; row; rowno++, row = row->nxtr)
	{
	    dbv.db_data = (PTR)((char *)(row->rp_data) + col_offset);
	    if (adh_dbcvtev(cb, &dbv, &edv) != E_DB_OK)
	    {
		FEafeerr(cb);
		return;
	    }

	    /* Skip null entries. */
	    if (nullind == -1)
		continue;

	    /* Ignore leading white space. */
	    for (ptr = txt; *ptr != EOS && CMwhite(ptr); CMnext(ptr))
		;

	    if (CMcmpnocase(ptr, chr) == 0)
	    {
		/* Found the searched-for row. */
		found = TRUE;
		break;
	    }
	}
	if (!found)
	{
	    for (rowno = 1, row = ds->top; row != crow;
		rowno++, row = row->nxtr)
	    {
		dbv.db_data = (PTR)((char *)(row->rp_data) + col_offset);
		if (adh_dbcvtev(cb, &dbv, &edv) != E_DB_OK)
		{
		    FEafeerr(cb);
		    return;
		}

		/* Skip null entries. */
		if (nullind == -1)
		    continue;

		/* Ignore leading white space. */
		for (ptr = txt; *ptr != EOS && CMwhite(ptr); CMnext(ptr))
		    ;

		if (CMcmpnocase(ptr, chr) == 0)
		{
		    /* Found the searched-for row. */
		    found = TRUE;
		    break;
		}
	    }
	}

	if (!found)
	{
	    dbv.db_data = (PTR)((char *)(crow->rp_data) + col_offset);
	    if (adh_dbcvtev(cb, &dbv, &edv) != E_DB_OK)
	    {
		FEafeerr(cb);
		return;
	    }

	    /* It's a null entry, so return. */
	    if (nullind == -1)
		return;

	    /* Ignore leading white space. */
	    for (ptr = txt; *ptr != EOS && CMwhite(ptr); CMnext(ptr))
		;

	    if (CMcmpnocase(ptr, chr) != 0)
	    {
		/* No matches, so beep. */
		FTbell();
	    }
	    /*
	    ** Since the searched-for row is the current row, don't need
	    ** to do anything, so return.
	    */
	}
	else
	{
	    /* Scroll to the 'found' row, and reset the column. */
	    _VOID_ IIscr_scan(tb, rowno);

		FDrsbRefreshScrollButton(tb->tb_fld, ds->distopix, ds->ds_records);
	    tfld->tfcurcol = colno;
	}
}
