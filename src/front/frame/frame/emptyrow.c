/*
**  emptyrow.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  Routine to see if a row in a table field is completely empty.
**
**  History
**	6/13/85 - Created. (dkh)
**	02/20/87 (dkh) - Added support for ADTs.
**	25-jul-89 (bruceb)
**		Ignore derived columns in determination of emptiness.
**	17-apr-90 (bruceb)
**		Removed unused frm, fldno arguments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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

# define	XLG_STRING	4096

/*
**  Please note that this routine assumes that there are
**  no read-only table fields. (dkh)
**  Another assumption is that the display buffers for any
**  field is always up to date.
*/

FDemptyrow(tbl, row)
TBLFLD	*tbl;
i4	row;
{
	FLDVAL		*val;
	FLDCOL		**fcol;
	FLDHDR		*hdr;
	DB_DATA_VALUE	*dbv;
	DB_TEXT_STRING	*text;
	i4		col;

	val = tbl->tfwins + (row * tbl->tfcols);
	fcol = tbl->tfflds;

	for (col = 0; col < tbl->tfcols; col++, val++, fcol++)
	{
	    hdr = &(*fcol)->flhdr;
	    if (!(hdr->fhd2flags & fdDERIVED))
	    {
		dbv = val->fvdsdbv;
		text = (DB_TEXT_STRING *) dbv->db_data;
		if (text->db_t_count != 0)
		{
		    return(FALSE);
		}
	    }
	}
	return(TRUE);
}
