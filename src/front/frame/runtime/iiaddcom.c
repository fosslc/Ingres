
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
** Name:	iiaddcomm.c
**
** Description:
**	Sets the interrupt code on the ctrl char passed.  When the user
**	enters the control character, the frame driver 	returns from
**	FDputfrm() with the value passed in this routine.
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**		16-feb-1983  -  Extracted from original runtime.c (jen)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIactcomm	-	Set activation code for cntrl char
**
** Description:
**	Add an activation code for a control character to the set of
**	activations for the current frame.
**
**	This routine is part of the external interface to RUNTIME.  
**	
** Inputs:
**	commval		Code for the control character
**	val		Activation value to set
**
** Outputs:
**	
**
** Returns:
**	i4		TRUE
**			FALSE	if no frame is current
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## activate command control_y
**
**	IIactcomm(c_CTRLY, activate_val);
**
** Side Effects:
**
** History:
**	
*/

IIactcomm(commval, val)
i4	commval;	/* The control character code */
i4	val;		/* Value to return */
{
	COMMS	*runcm;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	/*
	**	Get the control character command structure to record the
	**	which have been set.  Then inform the frame driver what
	**	the code it should return for this control char.
	*/
	runcm = IIstkfrm->fdruncm;
	FTaddcomm((u_char) IIvalcomms[commval], val);
	runcm[commval].c_comm = TRUE;
	runcm[commval].c_val = val;

	return (TRUE);
}
