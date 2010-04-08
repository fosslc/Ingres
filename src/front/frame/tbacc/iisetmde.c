/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
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
# include	<runtime.h> 
# include	<erfi.h> 
# include       <er.h>

/**
** Name:	iisetmde.c
**	Dynamically set a table field's mode.
**
** Description:
**
**	Public (extern) routines defined:
**		IITBdsmDynSetMode()
**	Private (static) routines defined:
**
** History:
**	11-oct-89 (bruceb)	Written.
**	08/16/91 (dkh) - Added fix to clear out dataset attributes from
**			 table field celss when a table field is cleared.
**	03/01/92 (dkh) - Renamed IItclr() to IITBtclr() to avoid name
**			 space conflicts with eqf generated symbols.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	i4	IITBtclr();
FUNC_EXTERN	i4	IIscr_fake();
FUNC_EXTERN	i4	IIvalfld();


/*{
** Name:	IITBdsmDynSetMode	- Dynamically set a table field mode.
**
** Description:
**	Dynamically set a table field's mode.  The meaning of setting a
**	table field mode differs depending on the old and new modes.  The
**	semantics are:
**  
** +--------------------------------------------------------------------------+
** |     \                                                                    |
** |New    \                    Starting Mode                                 |
** | Mode    \                                                                |
** +----------+---------------------------------------------------------------+
** |          | Fill        | Query (A) | Update           | Read             |
** +==========================================================================+
** |Fill      | No-op       | Clear fld | Add fake row (B) | Add fake row (B) |
** +--------------------------------------------------------------------------+
** |Query (A) | Clear fld   | Clear fld | Clear fld        | Clear fld        |
** +--------------------------------------------------------------------------+
** |Update    | No-op       | Clear fld | No-op            | No-op            |
** +--------------------------------------------------------------------------+
** |Read      | Val fld (C) | Clear fld | Val fld (C)      | No-op            |
** +--------------------------------------------------------------------------+
** 
**   (A) Clear field:  Dataset is cleared and hidden columns remain as
**       previously defined.
** 
**   (B) Add fake row:  Table fields in either update or read mode may contain
**       no rows at all.  Table fields in fill mode always have at least one
**       row.  Add this row if required.
** 
**   (C) Validate field:  Validate the table field prior to changing the mode.
**       Should the validation fail, the mode will NOT be changed.
**   
**   Setting the mode to the currently set value (except query mode to query
**   mode) does absolutely nothing.  Setting the mode to update from either
**   fill or read will change the mode but otherwise do nothing.
** 
** Inputs:
**	tb		Table field to be re-moded.
**	mode		Mode in which to place the table field.
**
** Outputs:
**
** Returns:
**	VOID
**
** Exceptions:
**	None
**
** Side Effects:
**	May change the number of rows in the table field, the number of
**	rows displayed, the current row, ...
**
** History:
**	11-oct-89 (bruceb)	Written.
*/

VOID
IITBdsmDynSetMode(TBSTRUCT *tb, i4 mode, FRAME *frm)
{
    i4		oldmode = tb->tb_mode;
    FLDHDR	*hdr;

    hdr = &(tb->tb_fld->tfhdr);

    if ((mode == fdtfQUERY) || (oldmode == fdtfQUERY))
    {
	hdr->fhdflags &= ~fdtfMASK;
	hdr->fhdflags |= mode;
	tb->tb_mode = mode;
	IITBtclr(tb, frm);	/* Clear field. */
    }
    else if (mode != oldmode)
    {
	if (mode == fdtfREADONLY)	/* New mode is 'read'--check data. */
	{
	    /*
	    ** Need to validate based on old mode--validation of read mode
	    ** is a no-op.
	    */
	    if (IIvalfld(tb->tb_name))
	    {
		hdr->fhdflags &= ~fdtfMASK;
		hdr->fhdflags |= mode;
		tb->tb_mode = mode;
	    }
	    else
	    {
		IIFDerror(E_FI226D_8813_TFModeUnchgd, 1, tb->tb_name);
	    }
	}
	else	/* New mode is 'fill' or 'update'. */
	{
	    hdr->fhdflags &= ~fdtfMASK;
	    hdr->fhdflags |= mode;
	    tb->tb_mode = mode;

	    if (mode == fdtfAPPEND)	/* New mode is 'fill'. */
	    {
		/*
		** Old mode was either 'update' or 'read'.  Add fake row if
		** empty dataset.
		*/
		if (!tb->dataset->top)
		{
		    IIscr_fake(tb, tb->dataset);
		}
	    }
	}
    }
}
