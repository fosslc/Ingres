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
# include	<menu.h>
# include	<runtime.h>
# include	<rtvars.h>

/**
** Name:	iigotonx.c
**
** Description:
**	Handle the 'RESUME NEXT' statement
**
**	Public (extern) routines defined:
**		IIresnext()
**
** History:
**	14-nov-88 (bruceb)
**		Made resume next in an activate timeout just return.
**		Trigger off of IIfrscb's control block rather than IIstkfrm's.
**	01-mar-89 (bruceb)
**		Made resume next in an entry activation just return.
**	07/18/93 (dkh) - Added support for resume nextfield/previousfield.
**	07/21/93 (dkh) - Changed code to use IIFRresNEXT and IIFRresPREV
**			 instead of hard coded values.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIresnext	-	Resume to next field
**
** Description:
**	The EQUEL "## RESUME NEXT" statement generates a call to this
**	routine.  Calls FDgotonext to go to the next field.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
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
** Example and Code Generation:
**	## resume next
**
**	IIresnext();
**	goto IIfdBegin1;
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	21-oct-1983  -  Put in ncg's rewrite to use oldcode (nml)
**	12/23/86 (dkh) - Added support for new activations.
*/

IIresnext()
{
	FRAME		*frm;
	FRS_EVCB	*evcb;

	if (!RTchkfrs(IIstkfrm))
		return(FALSE);

	frm = IIstkfrm->fdrunfrm;

	/*
	**  If there is an event waiting, set flag
	**  to indicate a resume next was issued
	**  to continue the event.  The events
	**  of interest are: 1) the menu key,
	**  2) a menu item or 3) a frs key was
	**  selected.
	*/
	evcb = IIfrscb->frs_event;

	/*
	**  A ## resume next inside an activate timeout block is considered
	**  functionally equivalent to a ## resume statement.  So, do nothing.
	**
	**  Also do nothing when already processing an entry activation.
	*/
	if ((evcb->event != fdopTMOUT) && (evcb->entry_act == FALSE))
	{
		if (!evcb->processed)
		{
			switch (evcb->event)
			{
				case fdopMENU:
				case fdopMUITEM:
				case fdopFRSK:
					evcb->continued = TRUE;
					break;
			}
		}

		if (evcb->continued == FALSE)
		{
			/* This is a 'goto next field' operation. */
			FDgotonext(frm, evcb);
		}
	}

	return(TRUE);
}



/*{
** Name:	IIFRgotofld - Go to the next or previous field.
**
** Description:
**	This routine is the entry point for supporting the resume
**	nextfield/previousfield statement.
**
**	If an invalid value for the direction to go is specified,
**	the code simply returns.
**
** Inputs:
**	dir		Direction to resume.  If value is IIFRresNEXT (0),
**			it is a resume nextfield.  A value of IIFRresPREV (1)
**			indicates a resume previousfield.
**
** Outputs:
**	None.
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
**	07/18/93 (dkh) - Initial version.
*/
void
IIFRgotofld(i4 dir)
{
	FRAME		*frm;
	FRS_EVCB	*evcb;

	/*
	**  Check to make sure the world is OK.
	*/
	if (!RTchkfrs(IIstkfrm))
		return;

	frm = IIstkfrm->fdrunfrm;
	evcb = IIfrscb->frs_event;

	/*
	**  Set event control block depending on which direction
	**  to go in.  This allows FDgotonext to work correctly.
	*/
	if (dir == IIFRresNEXT)
	{
		evcb->event = fdopNEXT;
	}
	else if (dir == IIFRresPREV)
	{
		evcb->event = fdopPREV;
	}
	else
	{
		return;
	}

	FDgotonext(frm, evcb);
}
