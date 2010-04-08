# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>



/*
**	fdmvtab.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdmvtab.c - Table field movement routines.
**
** Description:
**	Utility routines to handle movement in a table field.
**	Routines defined:
**	- FDqbfval - Set/reset validation checking on table field movement.
**	- FDmovetab - Process a movement in a table field.
**	- FDrightcol - Move to column on right of current column.
**	- FDscrdown - Scroll the table field down.
**	- FDdownrow - Process a move to the next (lower) row.
**	- FDuprow - Process a move to the previous row.
**
** History:
**	Additional code added to handle skipping display only columns.
**	Only FDrightcol is modified because FDleftcol is no longer called
**	by any one (dkh)
**	Added optional Query mode traversal (ncg)
**	Added Query Only column skipping (ncg)
**	02/17/87 (dkh) - Added header.
**	22-dec-88 (bruceb)
**		Handle readonly columns.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**      24-sep-96 (hanch04)
**              Global data moved to framdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN i4	FDgetdata();
FUNC_EXTERN i4	FDqrydata();
i4		FDcoltrv();

static i4	FDscrup();
static i4	FDscrdown();

GLOBALREF	bool	FDvalqbf ;



/*{
** Name:	FDqbfval - Set/reset validation check on table field movement.
**
** Description:
**	Special routine called by QBF to set/reset validation checking
**	on table field movement (e.g., moving between columns).  Just
**	sets passed in value to "FDvalqbf".  This is yet another hook
**	enable QBF to do RAW like I/O.
**
** Inputs:
**	val	Value to set "FDvalqbf" to.  FALSE means normal
**		validation checking while TRUE suppresses checking.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Normal validation processing is turned off if data passed in
**	is TRUE.  THis allows anything to be placed in a table field
**	column.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
VOID
FDqbfval(val)
bool	val;
{
	FDvalqbf = val;
}


/*
** FDRIGHTCOL
**
** Move the current column right.  Called when user hits RETURN or NROW.
*/

i4
FDrightcol(tbl, repts)
reg	TBLFLD	*tbl;
i4	repts;
{
	reg	i4	ocurcol;
	FLDCOL	*col;
	reg	FLDHDR	*hdr;
	i4	code;
	i4	qry;
	i4	(*routine)();

    /*
    ** Make sure column is valid, before continuing
    */
    if (tbl->tfhdr.fhdflags & fdtfQUERY)
	routine = FDqrydata;
    else
	routine = FDgetdata;

    if (!FDvalqbf && !FDcoltrv(tbl, tbl->tfcurrow, (FLDCOL *)NULL, routine))
	return (fdNOCODE);

    /*
    ** If there is only 1 column (readonly or not) then move right as normal.
    ** Calculate the number of rows or columns to move to based on the array
    ** format of the table field.
    */

    if (tbl->tfcols == 1)
    {
	tbl->tfcurrow += repts;
	/* tfcurcol is unchanged at 0 */
    }
    else
    {
	/*
	** Skip display only columns.  Save the original column number in case
 	** there is only one or zero regular columns.  Search until returning
	** to the original column or finding another regular column.
	*/
	ocurcol = tbl->tfcurcol;
	tbl->tfcurcol++;

	/*
	**  Query Only fields can br reached in Query
	**  and Fill modes.
	*/

	qry = tbl->tfhdr.fhdflags & fdtfQUERY
	    || tbl->tfhdr.fhdflags & fdtfAPPEND;

	while (tbl->tfcurcol != ocurcol)
	{
		/*
		** Pass the last column, update to next row.
		*/
		if (tbl->tfcurcol >= tbl->tfcols)
		{
			tbl->tfcurcol = 0;
			tbl->tfcurrow++;
			continue;
		}
		col = tbl->tfflds[tbl->tfcurcol];
		hdr = &col->flhdr;

		/*
		** Current column is display/query only, skip it and continue.
		*/
		if ((hdr->fhd2flags & fdREADONLY)
		    || (hdr->fhdflags & fdINVISIBLE)
		    || (hdr->fhdflags & fdtfCOLREAD)
		    || (!qry && hdr->fhdflags & fdQUERYONLY))
		{
			tbl->tfcurcol++;
		}
		else
		{
			/*
			** Found a regular column, exit search.
			*/
			break;
		}
	}

	/*
	** If we are back at the original column, and the original column
	** is read/query only, then all columns are read/query only.  So return
	** to first column.
	*/
	if (tbl->tfcurcol == ocurcol)
	{
		col = tbl->tfflds[tbl->tfcurcol];
		hdr = &col->flhdr;
		if ((hdr->fhd2flags & fdREADONLY)
		    || (hdr->fhdflags & fdINVISIBLE)
		    || (hdr->fhdflags & fdtfCOLREAD)
		    || (!qry && hdr->fhdflags & fdQUERYONLY))
		{
			tbl->tfcurcol = 0;
		}
	}
    }
    if (!FDvalqbf && tbl->tfcurrow > tbl->tflastrow)
    {
	    /*
	    ** Before the TDscroll get all the data
	    */
	    if (FDtbltrv(tbl, routine, FDDISPROW))
	    {
	    	if ((code = FDscrup(tbl)) != fdNOCODE)
    			return(code);
	    	tbl->tfcurrow = tbl->tfrows - 1;
	    }
    }
    return (fdNOCODE);
}


/*
** FDSCRUP
**
** TDscroll the table field up
*/
static i4
FDscrup(tbl)
    reg TBLFLD	*tbl;
{
    if (tbl->tfscrup != fdNOCODE)
    {
	tbl->tfstate = tfSCRUP;
    	return tbl->tfscrup;
    }
    return fdNOCODE;
}

/*
** FDSCRDOWN
**
** TDscroll the table field down
*/
static i4
FDscrdown(tbl)
    reg TBLFLD	*tbl;
{
    if (tbl->tfscrdown != fdNOCODE)
    {
	tbl->tfstate = tfSCRDOWN;
   	return tbl->tfscrdown;
    }
    return fdNOCODE;
}

/*
** FDDOWNROW
**
** move down a row in the table field
*/

i4
FDdownrow(tbl, repts)
    reg TBLFLD	*tbl;
    i4		repts;
{
    i4	code;
    i4	(*routine)();

    /*
    ** Make sure row is valid, before continuing
    */
    if (tbl->tfhdr.fhdflags & fdtfQUERY)
	routine = FDqrydata;
    else
	routine = FDgetdata;

    if (!FDrowtrv(tbl, tbl->tfcurrow,  routine))
	return (fdNOCODE);
    tbl->tfcurrow += repts;
    if (tbl->tfcurrow > tbl->tflastrow)
    {
	    /*
	    ** Before the TDscroll get all the data
	    */
	    if (FDtbltrv(tbl,  routine, FDDISPROW))
	    {
	    	if ((code = FDscrup(tbl)) != fdNOCODE)
    			return(code);
	    	tbl->tfcurrow = tbl->tfrows - 1;
	    }
    }
    return fdNOCODE;
}

/*
** FDUPROW
**
** move up a row in the table field
*/

i4
FDuprow(tbl, repts)
    reg TBLFLD	*tbl;
    i4		repts;
{
    i4	code;
    i4	(*routine)();

    /*
    ** Make sure row is valid, before continuing
    */
    if (tbl->tfhdr.fhdflags & fdtfQUERY)
	routine = FDqrydata;
    else
	routine = FDgetdata;

    if (!FDrowtrv(tbl, tbl->tfcurrow, routine))
	return (fdNOCODE);
    tbl->tfcurrow -= repts;
    if (tbl->tfcurrow < 0)
    {
	    /*
	    ** Before the TDscroll get all the data
	    */
	    if (FDtbltrv(tbl, routine, FDDISPROW))
	    {
	    	if ((code = FDscrdown(tbl)) != fdNOCODE)
    			return(code);
	    	tbl->tfcurrow = 0;
	    }
    }
    return fdNOCODE;
}
