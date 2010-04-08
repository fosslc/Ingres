/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	fixstate.c
*/



/*
** FIXSTATE.c
**
** If the field is a table field and it has been TDscrolled then
** when need to do all the TDscroll
**
** History:
**	03/05/87 (dkh) - Added support for ADTs.
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

i4
FDfixstate(frm, curfld)
    FRAME	*frm;
    i4		curfld;
{
    reg FIELD	*fld;
    reg TBLFLD	*tbl;

    fld = frm->frfld[curfld];
    if (fld->fltag == FREGULAR)
    	return fdNOCODE;
    tbl = fld->fld_var.fltblfld;
    switch (tbl->tfstate)
    {
      case tfNORMAL:
    	return fdNOCODE;

      case tfSCRUP:
    	/*
    	** The field is TDscrolling up. The user has already
    	** got control, and hopefully entered a value for the
    	** new row.  If tfputrow is true, the user has entered
    	** a value.  Otherwise, he has not.
    	*/
    	if (--tbl->tfcurrow > tbl->tflastrow)
    	{
    		return(tbl->tfscrup);
    	}
	if (tbl->tfcurrow < tbl->tflastrow)
		tbl->tfcurrow = tbl->tflastrow;
    	break;

      case tfSCRDOWN:
    	if (++tbl->tfcurrow < 0)
    	{
    		return(tbl->tfscrdown);
    	}
	/*
	** BUG 1429 - make sure the row numbers cannot be inconsistent, if a
	** call forced a change in the current row during an activate TDscroll.
	*/
	if (tbl->tfcurrow > 0)
		tbl->tfcurrow = 0;
    	break;

      case tfFILL:
    	if (tbl->tfinsert == fdNOCODE)
    	    break;
    	tbl->tfstate = tfINSERT;
    	tbl->tfcurrow = 0;
    	return(tbl->tfinsert);

      case tfINSERT:
    	/*
    	** when we get here, currow should already have been
    	** entered.
    	*/
    	if (++tbl->tfcurrow >= tbl->tfrows)
    	{
    		tbl->tfcurrow = 0;
    		break;
    	}
    	return(tbl->tfinsert);

      default:
    	return fdNOCODE;
    }
    FTmarkfield(frm, curfld, FT_UPDATE, FT_TBLFLD, (i4) -1, (i4) -1);

    tbl->tfstate = tfNORMAL;
    return fdNOCODE;
}
