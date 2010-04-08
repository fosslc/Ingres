/*
**	iiscrfake.c
**
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
# include	<menu.h>
# include	<runtime.h> 

/**
** Name:	iiscrfake.c	-	Manage 'fake' tblfld row
**
** Description:
**
**	Public (extern) routines defined:
**		IIscr_fake()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	25-jul-89 (bruceb)
**		New argument in call on IIdsinsert().
**	04/20/90 (dkh) - Fixed us #196 by marking fake rows with special
**			 bits.
**	07/15/92 (dkh) - Added support for edit masks.
**	02/05/92 (dkh) - Updated to handle interface change to IIdisprow().
**	03/14/93 (dkh) - Fixed bug 49883.  Don't clear columns with
**			 edit masks enabled.
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	15-jan-1998 (kitch01)
**		Bug 88208. After adding a fake row  to an empty dataset 
**		set the ordinal position to 1.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	void	IIFDscoSetCROpt();

/*{
** Name:	IIscr_fake	-	Scroll in a 'fake' row
**
** Description:
** 	Scroll in a fake row for an APPEND type table, so that user
**	has a corresponding data set record to his row in the 
**	displayed table.
**
**	Depending on the state of the displayed table (empty, not full
**	or full) insert a new record into the dataset list update the
**	records and display the dummy row.  All append type rows have
**	a state stNEWEMPT (in contrast to loaded rows that are 
**	initially stUNCHANGED).
**
**
** Inputs:
**	tb		Ptr to tablefield
**	ds		Ptr to the dataset
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	18-feb-1987	- Changed ref to rp_rec to new rp_data (drh)
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	15-jan-1998 (kitch01)
**		Bug 88208. After adding a fake row  to an empty dataset 
**		set the ordinal position to 1.
**	
*/

IIscr_fake(tb, ds)
register TBSTRUCT	*tb;
register DATSET		*ds;
{
	i4	*rowinfo;
	TBLFLD	*tfld;

	tfld = tb->tb_fld;

	IIFDscoSetCROpt(TRUE);

	if (ds->top == NULL)				/* display is empty */
	{
		/* start list and make top and bot the same */
		IIdsinsert(tb, ds, (TBROW *) NULL, (TBROW *) NULL, stNEWEMPT);
		ds->distop = ds->disbot = ds->top;
		/* b88208 */
		ds->distopix = 1;     /* keep track of ordinal position */
		tb->tb_display++;
		/* Update the display's idbv's. */
		IIdisprow(tb, (i4) fdtfTOP, ds->disbot);
		/* clear displayed row so as not to get default values*/
		FDclrrow(tb->tb_fld, (i4) fdtfTOP);
		rowinfo = tfld->tffflags;
		*rowinfo |= fdtfFKROW;
		*rowinfo &= ~fdtfRWCHG;
	}
	else if (tb->tb_display < tb->tb_numrows)	/* append to display */
	{
		/* append new row to bot (disbot), and update display */
		IIdsinsert(tb, ds, ds->bot, (TBROW *) NULL, stNEWEMPT);
		ds->disbot = ds->disbot->nxtr;
		tb->tb_display++;
		/* Update the display's idbv's. */
		IIdisprow(tb, tb->tb_display - 1, ds->disbot);
		/* clear displayed row so as not to get default values*/
		FDclrrow(tb->tb_fld, tb->tb_display - 1);
		rowinfo = tfld->tffflags +((tb->tb_display - 1) * tfld->tfcols);
		*rowinfo |= fdtfFKROW;
		*rowinfo &= ~fdtfRWCHG;
	}
	else						/* scroll required */
	{
		IIdsinsert(tb, ds, ds->bot, (TBROW *) NULL, stNEWEMPT);
		IIretrow(tb, (i4) fdtfTOP, ds->distop);	/* save */
		ds->disbot = ds->disbot->nxtr;
		ds->distop = ds->distop->nxtr;
		ds->distopix++;     /* keep track of ordinal position */

		/* scroll and display */
		FDcopytab(tb->tb_fld, (i4) fdtfTOP, (i4) fdtfBOT);
		/* Update the display's idbv's. */
		IIdisprow(tb, (i4) fdtfBOT, ds->disbot);
		/* clear displayed row so as not to get default values*/
		FDclrrow(tb->tb_fld, (i4) fdtfBOT);
		rowinfo = tfld->tffflags + ((tfld->tfrows - 1) * tfld->tfcols);
		*rowinfo |= fdtfFKROW;
		*rowinfo &= ~fdtfRWCHG;
	}
	ds->ds_records++;

	IIFDscoSetCROpt(FALSE);

	return (TRUE);
}
