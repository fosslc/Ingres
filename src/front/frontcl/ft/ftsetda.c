/*
**  ftsetda.c
**
**  Set the specified field to the passed displayed attribute.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/12/85 (dkh)
 * Revision 60.5  87/04/08  00:01:32  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.4  87/03/18  21:43:27  dave
 * Integrated ADT changes. (dkh)
 * 
 * Revision 60.3  86/11/21  07:46:25  joe
 * Lowercased FT*.h
 * 
 * Revision 60.2  86/11/18  21:56:18  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  21:56:08  dave
 * *** empty log message ***
 * 
 * Revision 40.5  85/11/21  19:29:38  dave
 * Added support for color. (dkh)
 * 
 * Revision 40.4  85/11/05  21:10:26  dave
 * extern to FUNC_EXTERN for routines. (dkh)
 * 
 * Revision 40.3  85/09/18  17:14:16  john
 * Changed i4=>nat, CVla=>CVna, CVal=>CVan, unsigned char=>u_char
 * 
 * Revision 40.2  85/09/07  22:50:19  dave
 * Fixed code to work correctly. (dkh)
 * 
 * Revision 1.2  85/09/07  20:46:56  dave
 * Fixed code to work correctly. (dkh)
 * 
 * Revision 1.1  85/07/19  11:43:55  dave
 * Initial revision
 * 
**	03/03/87 (dkh) - Added support for ADTs.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	05/02/87 (dkh) - Changed to support table field cursor tracking.
**	11/01/88 (dkh) - Performance changes.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	07/25/90 (dkh) - Added support for table field cell highlighting.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	08/07/97 (kitch01)
**           Bug 83664. Changing a display attribute of a tablefield column
**           causes the column highlight to be turned off when using Upfront.
**	28/05/98 (kitch01)
**			 Bug 91059. If the tablefield column is invisble the tablefield
**			 curcol and currow attributes are not reset to their original values.
**			 Amended so that these attributes are only changed for visible 
**			 columns.
**	12/04/99 (kitch01)
**           Bug 95546. Fields defined initially as invisible will not become 
**           visible in Upfront if the ABF initialize sections makes them visible.
**			 There is a check to see if the field is invisible and if it is any
**			 attribute change is ignored. Upfront cares about these changes though.
**           NOTE.FOR FUTURE ENHANCEMENT
**           Much of this code is not required for MWS and so could be bypassed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

FUNC_EXTERN	VOID	IIFTdcaDecodeCellAttr();
FUNC_EXTERN	WINDOW	*IIFTwffWindowForForm();

VOID
FTsetda(frm, fieldno, disponly, fldtype, col, row, attr)
FRAME	*frm;
i4	fieldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
i4	attr;
{
	FLDVAL	*val;
	FLDTYPE	*type;
	FLDHDR	*hdr;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDCOL	*column;
	i4	ocol;
	i4	orow;
	i4	*pflag;
	WINDOW	*win;

	if ((win = IIFTwffWindowForForm(frm, FALSE)) == NULL)
	{
		return;
	}

	if (disponly == FT_UPDATE)
	{
		fld = frm->frfld[fieldno];
	}
	else
	{
		fld = frm->frnsfld[fieldno];
	}

	if (fld->fltag == FTABLE)
	{
		tbl = fld->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		column = tbl->tfflds[col];
		/* Bug 95546. Carry on if MWS is active */
        if ((hdr->fhdflags & fdINVISIBLE ||
		     column->flhdr.fhdflags & fdINVISIBLE)
		     && !IIMWimIsMws())
		{
           return;
		}

		/* Bug 91059. This block of code has been moved to be
		** after the above test.
		*/
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = col;
		tbl->tfcurrow = row;

		/*
		**  Check if cell we are updating is part of
		**  a row that is highlighted with cursor
		**  tracking.  If it is, then just return.
		*/
		pflag = tbl->tffflags + (row * tbl->tfcols) + col;

/*
		if (*pflag & fdROWHLIT)
		{
			tbl->tfcurcol = ocol;
			tbl->tfcurrow = orow;
			return;
		}
*/

		/*
		**  Now we must deal with attributes set on the
		**  dataset value or the cell or the column, in
		**  that order.
		**
		**  If dataset or cell attributes are set, then
		**  we just 'OR' them into the "attr" values.
		*/
		if (*pflag & (dsALLATTR | tfALLATTR))
		{
			IIFTdcaDecodeCellAttr(*pflag, &attr);
		}

		/*
		**  Now OR in column attributes to get everything.
		*/
		attr |= (column->flhdr.fhdflags & fdDISPATTR);

		/*
		**  Finally, OR in row highlighting (if necessary)
		**  to eliminate screen flicker.
		*/
		/*  Bug 83664. Dont do this if MWS active as it causes
		**  highlighting on the column to be lost if the columns
		**  display attribute(s) are changed.
		*/
		if ((*pflag & fdROWHLIT) && !IIMWimIsMws())
		{
			attr |= fdRVVID;
		}
	}
	else
	{
		hdr = (*FTgethdr)(fld);
		/* Bug 95546. Carryon if MWS is active */
		if ((hdr->fhdflags & fdINVISIBLE) && !IIMWimIsMws())
		{
			return;
		}
	}

	val = (*FTgetval)(fld);
	type = (*FTgettype)(fld);

	if (fld->fltag == FTABLE)
	{
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}

	if (TDsubwin(win, (type->ftwidth + type->ftdataln - 1)/type->ftdataln,
		type->ftdataln, hdr->fhposy + val->fvdatay,
		hdr->fhposx + type->ftdatax, FTutilwin) == NULL)
	{
		return;
	}

/*
	TDersda(FTutilwin);
*/

	if (attr & (fdRVVID | fdBLINK | fdUNLN | fdCHGINT | fdCOLOR))
	{
		TDputattr(FTutilwin, attr);
	}
	else
	{
		TDersda(FTutilwin);
	}
	TDtouchwin(FTutilwin);

# ifdef DATAVIEW
	_VOID_ IIMWsaSetAttr(frm, fieldno, disponly, col, row,
		attr & fdDISPATTR);
# endif	/* DATAVIEW */
}



/*{
** Name:	IIFTdcaDecodeCellAttr - Decode cell attribute into frame set.
**
** Description:
**	This routine decodes any dataset or cell display attributes into
**	the normal set of bit values that the forms system expects.
**	The decoded values are 'OR'ed together with the passed set of
**	values.
**
** Inputs:
**	cellflag	Attributes for a cell
**
** Outputs:
**	attr		Normal forms system attributes.
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
**	07/22/90 (dkh) - Initial version.
*/
VOID
IIFTdcaDecodeCellAttr(cellflag, attr)
i4	cellflag;
i4	*attr;
{
	if (cellflag & dsRVVID)
	{
		*attr |= fdRVVID;
	}
	if (cellflag & dsBLINK)
	{
		*attr |= fdBLINK;
	}
	if (cellflag & dsUNLN)
	{
		*attr |= fdUNLN;
	}
	if (cellflag & dsCHGINT)
	{
		*attr |= fdCHGINT;
	}
	if (cellflag & ds1COLOR)
	{
		*attr |= fd1COLOR;
	}
	if (cellflag & ds2COLOR)
	{
		*attr |= fd2COLOR;
	}
	if (cellflag & ds3COLOR)
	{
		*attr |= fd3COLOR;
	}
	if (cellflag & ds4COLOR)
	{
		*attr |= fd4COLOR;
	}
	if (cellflag & ds5COLOR)
	{
		*attr |= fd5COLOR;
	}
	if (cellflag & ds6COLOR)
	{
		*attr |= fd6COLOR;
	}
	if (cellflag & ds7COLOR)
	{
		*attr |= fd7COLOR;
	}
	if (cellflag & tfRVVID)
	{
		*attr |= fdRVVID;
	}
	if (cellflag & tfBLINK)
	{
		*attr |= fdBLINK;
	}
	if (cellflag & tfUNLN)
	{
		*attr |= fdUNLN;
	}
	if (cellflag & tfCHGINT)
	{
		*attr |= fdCHGINT;
	}
	if (cellflag & tf1COLOR)
	{
		*attr |= fd1COLOR;
	}
	if (cellflag & tf2COLOR)
	{
		*attr |= fd2COLOR;
	}
	if (cellflag & tf3COLOR)
	{
		*attr |= fd3COLOR;
	}
	if (cellflag & tf4COLOR)
	{
		*attr |= fd4COLOR;
	}
	if (cellflag & tf5COLOR)
	{
		*attr |= fd5COLOR;
	}
	if (cellflag & tf6COLOR)
	{
		*attr |= fd6COLOR;
	}
	if (cellflag & tf7COLOR)
	{
		*attr |= fd7COLOR;
	}
}
