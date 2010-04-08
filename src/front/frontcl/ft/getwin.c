/*
**  Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	"ftfuncs.h"

/*
** getwin.c
**
** contains
**	FTgetwin(frm, curfld)
**		returns the data window of the field numbered curfld
**
** History:
**	22-mar-1984 -	Added QueryOnly fields. (ncg)
**	06/19/87 (dkh) - Code cleanup.
**	20-dec-88 (bruceb)
**		Added support for readonly and invisible fields.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**	01/09/90 (dkh) - Put in check to not alway set tfcurcol to zero
**			 if no columns are landable.  Column zero could
**			 be invisible.  Instead, set the first visible col.
**	07/24/90 (dkh) - A more complete fix for bug 30408.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** get the data window of field curfld on frame frm.
** If the field is a table field use the currow curcol to figure
** out which window to use.
**
** Additional code added to skip past display only columns
** in Table Fields.  If all columns are display only then
** return pointer to what the frame driver thinks is the
** current window in the table field. (dkh)
*/


FUNC_EXTERN	WINDOW	*FTfindwin();

GLOBALREF	WINDOW	*IIFTdatwin;


WINDOW *
FTgetwin(frm, curfld)
FRAME	*frm;
i4	*curfld;
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
	**  and Fill Modes.
	*/

	qrymd = (FTcurdisp() == fdmdQRY || FTcurdisp() == fdmdADD);

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
				return(FTfindwin(fld, FTwin, IIFTdatwin));
			}

			/* Skip over unsuitable fields */
			if (*curfld == frm->frfldno - 1)
				*curfld = 0;
			else
				(*curfld)++;
			/* 
			** Scanned through all fields, so return NULL
			** to signal no fields to set on.
			*/
			if (*curfld == ofld)
			{
				*curfld = 0;
				frm->frcurfld = 0;
				return (NULL);
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
			** Scanned through all fields, so return NULL
			** to signal no fields to set on.
			*/
			if (*curfld == ofld)
			{
				*curfld = 0;
				frm->frcurfld = 0;
				return (NULL);
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
				if (tbl->tfcurcol == tbl->tfcols -1)
					tbl->tfcurcol = 0;
				else
					(tbl->tfcurcol)++;
				/*
				**  Scanned through all columns set
				**  to first visible column (which
				**  may not necessarily be zero).
				*/
				if (tbl->tfcurcol == ocol)
				{
					for (i = 0; i < tbl->tfcols; i++)
					{
						tbl->tfcurcol = i;
						hdr = (*FTgethdr)(fld);
						if (!(hdr->fhdflags & fdINVISIBLE))
						{
							tbl->tfcurcol = i;
							break;
						}
					}
					break;
				}
			}
			else
			{
				/* Not skip regular column */
				break;
			}
		}
		return(FTfindwin(fld, FTwin, IIFTdatwin));
	}
}
