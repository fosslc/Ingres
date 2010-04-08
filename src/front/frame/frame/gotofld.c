/*
**	gotofld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	gotofld.c - Go to a particular field.
**
** Description:
**	These routines allow an upper level program to move the frame
**	driver to different fields in the frame when a frame interrupts.
**	Routines defined:
**	- FDgotofld - Go to a particular field.
**	- FDgotonext - Continue to the next/previous field.
**	- FDtfsgfix - Fix a table field before leaving it.
**
** History:
**	JEN - 1 Nov 1981  (written)
**	12/23/86 (dkh) - Added support for new activations.
**	24-nov-86 (bruceb)	Fix for b10892
**		Whenever a resume [next | field] is done, check if
**		old field is a table field, and if going to a new
**		field, reset tfcurrow and tfcurcol.  FTtfsgfix()
**		now provides this functionality rather than simply
**		checking for tfcurrow out of bounds.
**	14-jan-87 (bruceb)
**		Reset tfcurrow to 0 if it would otherwise be out
**		of bounds even if resuming to the same table field
**		that user was in prior to the activate.
**	02/15/87 (dkh) - Added header.
**	05/19/89 (dkh) - Fixed bug 5398.
**	18-apr-90 (bruceb)
**		Lint changes; FDtfsgfix now has 2 instead of 3 args.
**	01/12/91 (dkh) - Fixed bug 35327.
**	08/11/91 (dkh) - Added change to prevent alerter events from
**			 being triggered when no menuitems were selected
**			 or in between a column/scroll activation combination.
**  17-jun-97 (rodjo04) -Fixed bug 83098
**     Added check to FDtfsgfix() to see if we are currently scrolling.   
**     If so then don't change current row value. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	GOTOFLD.c  -  Go To a Particular Field
**	
**	
**	
**	FDGOTONEXT() - go to the next field in the frame.
**	
**	Arguments: frm - frame to be driven.
**	
**	History:  JEN - 1 Nov 1981  (written)
*/



# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>



/*{
** Name:	FDgotofld - Go to a field.
**
** Description:
**	Given a form and a field name, find the field in the
**	form and make it the current field.  Before changing
**	the current field, check to see if the old current field
**	is a table field.  If it is check to make sure the
**	current row and column are valid for the table field.
**	This is all done by FDtfsgfix().
**
** Inputs:
**	frm	Form that contains field.
**	fname	Name of field to go to.
**
** Outputs:
**	Returns:
**		TRUE	If field exists in the form.
**		FALSE	If field does not exist in form.
**	Exceptions:
**		None.
**
** Side Effects:
**	If the old current field was a table field, the current
**	row and column for it may be changed.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDgotofld(frm, fname)			/* FDGOTOFLD: */
FRAME	*frm;					/* frame to be driven */
char	*fname;				/* name of field to go to */
{
    i4	fldno;					/* field index */
    FIELD *fld;
    FIELD *ofld;

    /* Check each field for name passed and set current field
    ** pointer to that field.
    */
    for(fldno = 0; fldno < frm->frfldno; fldno++)
    {
	fld = frm->frfld[fldno];
	if (STcompare(fname, FDGFName(fld)) == 0)
	{
    		ofld = frm->frfld[frm->frcurfld];
    		if (ofld->fltag == FTABLE &&
			ofld->fld_var.fltblfld->tfhdr.fhdflags & fdROWHLIT)
    		{
			frm->frmflags |= fdREBUILD;
    		}
		/*
		**  Check to see if old current field is
		**  a table field and if so, set current row and col.
		*/
		FDtfsgfix(frm, fldno);

		frm->frcurfld = fldno;
		frm->frcurnm = FDGFName(fld);
		return (TRUE);
	}
    }

    return (FALSE);
}



/*{
** Name:	FDgotonext - Process a resume next request.
**
** Description:
**	A resume next request is only valid from an activate
**	field or activate scroll{up/down} block.  This
**	essentially completes the action that cause the
**	activate to occur.  The commands that are handled
**	here are the nextfield, previousfield, downline and
**	upline commands.  Continuation for menu, menuitem
**	and frskey selection is handled in the RUNTIME layer.
**	A new current field is set at the end of this
**	routine, by FDmove().
**
** Inputs:
**	frm	Form to process the continuation on.
**	evcb	An event control block providing the
**		command to continue with.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Can cause an activate scroll to be raised if the
**	continuation is a downline at the bottom of a
**	table field or an upline at the top of one.
**
** History:
**	<manual entries>
*/
VOID
FDgotonext(frm, evcb)				/* FDGOTONEXT: */
FRAME		*frm;				/* frame to drive */
FRS_EVCB	*evcb;
{
	i4	fldno;
	i4	ofldno;
	i4	oldcode;
	i4	rval;
	FIELD	*ofld;
	bool	eval_aggs;

	ofldno = fldno = frm->frcurfld;
	oldcode = evcb->event;
	if (oldcode == 0 || oldcode == fdopMENU)
	{
		oldcode = fdopNEXT;
	}
	else if (oldcode == fdopERR || oldcode == fdopADUP)
	{
		oldcode = fdopRET;
	}

	/*
	**  Set eval_aggs to TRUE before calling FDmove() to take
	**  care of aggregate recalculations that may be triggered
	**  by FDmove().
	*/
	eval_aggs = evcb->eval_aggs;
	evcb->eval_aggs = TRUE;
	rval = FDmove(oldcode, frm, &fldno, 1);
	evcb->eval_aggs = eval_aggs;

	if (rval == fdopSCRDN || rval == fdopSCRUP)
	{
		FDsetscroll(rval);
		frm->frmflags |= fdIN_SCRLL;
	}

	if (ofldno != fldno)
	{
    		ofld = frm->frfld[ofldno];
    		if (ofld->fltag == FTABLE &&
			ofld->fld_var.fltblfld->tfhdr.fhdflags & fdROWHLIT)
    		{
			frm->frmflags |= fdREBUILD;
    		}
	}

	/*
	**  Check to see if old current field is
	**  a table field and if so, set current row and col.
	*/
	FDtfsgfix(frm, fldno);

	frm->frcurfld = fldno;
}


FDtfsgfix(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fld;
	TBLFLD	*tbl;

	fld = frm->frfld[frm->frcurfld];
	if (fld->fltag == FTABLE)
	{
		tbl = fld->fld_var.fltblfld;
		if (frm->frcurfld != fldno)
		{
			tbl->tfcurrow = 0;
			tbl->tfcurcol = 0;
		}
		else if (tbl->tfcurrow < 0 || tbl->tfcurrow >= tbl->tfrows)
		{
            if (!(frm->frmflags & fdIN_SCRLL))
			   tbl->tfcurrow = 0;
		}
	}
}
