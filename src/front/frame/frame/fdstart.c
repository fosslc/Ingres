/*
**  fdstart.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	01/07/87 (dkh) - Broke up file to eliminate undefined references
**			 for stand alone frs utilities.
**	03/05/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bruceb)	Code cleanup.
**	19-dec-88 (bruceb)
**		Added support for READONLY and INVISIBLE fields.  Can't land
**		on an invisible field or a readonly simple field.  Can't land
**		on a readonly column unless all columns are equally
**		'unreachable'.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**	01/02/90 (dkh) - If no columns are landable, then change starting
**			 column to be first visible column rather than just
**			 the first column.
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
# include	"fdfuncs.h"

/*
** fdstart.c
**
** contains
**	FDstart()	Return sequence number of first usable field on
**			form that is at or after the 'curfld'.
** History:
**		22-mar-1984 -	Added QueryOnly fields. (ncg)
*/

/*
** Get the sequence number of the first usable field on the form that is
** at or after 'curfld'.  If that field is a table field, set the field's
** current column.
**
** This code skips past display only columns in Table Fields.  If all columns
** are display only then set current column to the first column that is
** visible.  We naturally assume that some column is visible else
** the whole table field would be marked invisible.
** Additionally, handle query only fields/columns when not in query mode.
*/

i4
FDstart(frm, curfld)
FRAME		*frm;
register i4	*curfld;
{
	register FIELD	*fld;
	TBLFLD		*tbl;
	register FLDCOL	*col;
	FLDHDR		*hdr;
	i4		ofld;
	i4		ocol;
	i4		qrymd;
	i4		i;

	ofld = *curfld;

	/*
	**  Queryonly fields can be reached in Query
	**  and Fill modes.
	*/

	qrymd = (FDcurdisp() == fdmdQRY || FDcurdisp() == fdmdADD);

	for(;;)
	{
		fld = frm->frfld[*curfld];
		if (fld->fltag == FREGULAR)
		{
			hdr = &fld->fld_var.flregfld->flhdr;

			/*
			** If not readonly or invisible, or if query mode
			** or not queryonly, then allow all field access.
			*/
			if (!(hdr->fhd2flags & fdREADONLY)
			    && !(hdr->fhdflags & fdINVISIBLE)
			    && (qrymd || !(hdr->fhdflags & fdQUERYONLY)))
			{
				return(*curfld);
			}

			/* Skip over unsuitable fields */
			if (*curfld == frm->frfldno - 1)
				*curfld = 0;
			else
				(*curfld)++;
			/* 
			** Scanned through all fields, so return -1
			** to signal no fields to set on.
			*/
			if (*curfld == ofld)
			{
				*curfld = 0;
				frm->frcurfld = 0;
				return(-1);
			}
			else
			{
				continue;
			}
		}

		/* Table Field: */
		tbl = fld->fld_var.fltblfld;

		/* Can't land on an invisible table field at all */
		if (tbl->tfhdr.fhdflags & (fdINVISIBLE | fdTFINVIS))
		{
			if (*curfld == frm->frfldno - 1)
				*curfld = 0;
			else
				(*curfld)++;
			/* 
			** Scanned through all fields, so return -1
			** to signal no fields to set on.
			*/
			if (*curfld == ofld)
			{
				*curfld = 0;
				frm->frcurfld = 0;
				return(-1);
			}
			else
			{
				continue;
			}
			
		}

		ocol = tbl->tfcurcol;

		/*
		** Skip columns if displayonly, readonly or invisible, or
		** queryonly and the table field is not in Query mode (OK
		** also if in Fill mode.)  Do not exit the table field in
		** any case.
		*/
		qrymd = tbl->tfhdr.fhdflags & fdtfQUERY
			|| tbl->tfhdr.fhdflags & fdtfAPPEND;

		for(;;)
		{
			col = tbl->tfflds[tbl->tfcurcol];
			hdr = &col->flhdr;
			if ((hdr->fhdflags & fdtfCOLREAD)
			    || (hdr->fhd2flags & fdREADONLY)
			    || (hdr->fhdflags & fdINVISIBLE)
			    || (!qrymd && hdr->fhdflags & fdQUERYONLY))
			{
				if (tbl->tfcurcol == tbl->tfcols - 1)
					tbl->tfcurcol = 0;
				else
					(tbl->tfcurcol)++;
				/*
				**  Scanned through all columns, set to 
				**  first visible column.
				*/
				if (tbl->tfcurcol == ocol)
				{
					for (i = 0; i < tbl->tfcols; i++)
					{
						col = tbl->tfflds[i];
						hdr = &col->flhdr;
						if (!(hdr->fhdflags & fdINVISIBLE))
						{
							break;
						}
					}
					tbl->tfcurcol = i;
					break;
				}
			}
			else
			{
				/* Do not skip regular column */
				break;
			}
		}
		return(*curfld);
	}
}
