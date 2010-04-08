/*
**	iitbact.c
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
# include	<frserrno.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iitbact.c
**
** Description:
**
**	Public (extern) routines defined:
**		IItbact()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	17-apr-89 (bruceb)
**		Initialize IITBccvChgdColVals to FALSE.
**		Checked in IITBceColEnd().
**	25-apr-96 (chech02)
**		Added funtion type i4  to IItbact() for windows 3.1 port.
**	15-apr-97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	VOID		IInesterr();

GLOBALREF	TBSTRUCT	*IIcurtbl;

GLOBALREF	bool		IITBccvChgdColVals;

/*{
** Name:	IItbact		-	Prepare for LOADTABLE/UNLOADTABLE
**
** Description:
** 	If the table has an active data set (using an INITTABLE)
**	then update the data set to execute the following LOADTABLE
**	or UNLOADTABLE.
**
**	Update general I/O info. If loading then insert a new data
**	set record.  If the new record will be displayed then update
**	the table's knowledge of this.
**	If unloading then retrieve all the displayed table back into
**	the data set (things may have been changed meanwhile by puts,
**	gets, or user appending).
**
**	Updates the global IIcurtbl to the caller's table if the
**	routine can be correctly executed.
**	Can unload even if tb_display = 0, as all may be deleted.
**
**	This routine is part of TBACC's external interface.
**	
**
** Inputs:
**	frmnm		Name of the form
**	tabnm		Name of the tablefield
**	loading		Set if LOADTABLE
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
** Example and Code Generation:
**	## loadtable "form" "tbl" (col1 = buf)
**
**	if ( IItbact("form","tbl",1) != 0 )
**	{
**		IItcolset("col1",1,32,0,buf)
**	}
**
** Side Effects:
**
** History:
**	
*/

i4
IItbact (const char *frmnm, const char *tabnm, i4 loading)
{
	TBSTRUCT	*tb;			/* user's table field */
	register DATSET	*ds;			/* its data set */
	register TBROW	*rp;			/* record pointer for unload */
	register i4	i;

	/* general checking of arguments, update IIfrmio */
	if ( (tb = IItstart(tabnm, frmnm) ) == NULL)
	{
		return (FALSE);
	}

	if ( (ds = tb->dataset) == NULL)
	{
		/* table not initialized with data set */
		IIFDerror(TBNODATSET, 1, (char *) tb->tb_name);
		return (FALSE);
	}

	if (loading)
	{
		IITBccvChgdColVals = FALSE;
		tb->tb_state = tbLOAD;

		/* 
		** If the only row in the data set is the fake APPEND type
		** row then delete it, and start loading from the very top.
		** This is to avoid the user loading onto a "visually" empty
		** table field, and finding his first row is at row 2 and
		** not row 1.
		*/
		if ((tb->tb_mode == fdtfAPPEND || tb->tb_mode == fdtfQUERY) &&
			tb->tb_display == 1)
		{
			/* retrieve the top row and check if still empty */
			IIretrow(tb, (i4) fdtfTOP, ds->distop);
			/* is it still empty */
			if (ds->distop->rp_state == stNEWEMPT)
			{
				/* reset values so we load into first row */
				tb->tb_display = 0;
				ds->ds_records = 0;
				ds->distopix = 0; 

				IIt_rfree(ds, ds->distop);
				ds->top = NULL;
			}
		}

		if (ds->top == NULL)			/* list is empty */
		{
			/* start off list and update top and bot as same */
			IIdsinsert(tb, ds, (TBROW *) NULL, (TBROW *) NULL,
			    stUNCHANGED);
			ds->distop = ds->disbot = ds->top;
			ds->distopix = 1;     /* Initial ordinal position */

			/* Turn off READ mode */
			if (tb->tb_mode == fdtfUPDATE)
			{
			    tb->tb_fld->tfhdr.fhdflags &= ~fdtfREADONLY;
			    tb->tb_fld->tfhdr.fhdflags |= fdtfUPDATE;
			}
		}
		else 		/* append new row to bottom of data set */
		{
		    IIdsinsert(tb, ds, ds->bot, (TBROW *) NULL, stUNCHANGED);
		}
		/* update for loading in value */
		ds->crow = ds->bot;
		ds->ds_records++;

		/* check if new row will be displayed */
		if (tb->tb_display < tb->tb_numrows)
		{
			tb->tb_display++;
			/* update for display loading */
			tb->tb_rnum = tb->tb_display;	/* current row num */
			ds->disbot = ds->bot;
		}
	}
	else 						/* unloading */
	{
		if (IIunldstate() != NULL)
		{
			/* trying to nest UNLOADTABLE statements */
			IIFDerror(TBUNNEST, 1, (char *) tb->tb_name);
			IInesterr();
			return (FALSE);
		}
		/* no need to unload an empty table */
		if (ds->top == NULL && ds->dellist == NULL)
			return (FALSE);
		/*
		** If unloading a table in the I/O frame, then make sure
		** that the data in the display is put into the data windows.
		** This can be left out for non APPEND type tables, as the data
		** is always checked when put there. But for APPEND or UPDATE 
		** type tables this is neccessary.
		*/
		if (tb->tb_mode != fdtfREADONLY && tb->tb_display >= 1)
		{
			if (!FDckffld(IIfrmio->fdrunfrm, tb->tb_name))
			{
				IIsetferr((ER_MSGID) 1);
				return (FALSE);
			}
		}
		tb->tb_state = tbUNLOAD;
		/* update values of data set with currently displayed values */
		for (i = 0, rp = ds->distop; i < tb->tb_display;
		     i++, rp = rp->nxtr)
		{
			IIretrow(tb, i, rp);
		}
	}
	IIcurtbl = tb;
	return (TRUE);
}
