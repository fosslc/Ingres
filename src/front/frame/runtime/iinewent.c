/*
**	Copyright (c) 2004 Ingres Corporation
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
# include	<rtvars.h>


/**
** Name:	iinewent.c -	Routine to handle resume entry
**
** Description:
**	Routines defined are:
**		IIFRreResEntry		Handle 'resume entry'
**
** History:
**	23-mar-89 (bruceb)	Written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIFRreResEntry	- Like a 'resume to current' but with EA.
**
** Description:
**	Set up for an entry activation on return to the current field
**	on the form.  This is essentially nothing more than a
**	'## resume' (no-op) with a request for EA tossed in.
**	This is entirely a no-op if called from in a (non-display)
**	submenu.  Work is done by setting the record of 'last field
**	cursor was in' to a BADFLDNO.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## resume entry
**
*	{
**	    IIFRreResEntry();
**	    if (1) goto IIfdB1;
**	}
**
** Side Effects:
**
** History:
**	23-mar-89 (bruceb)	Written.
*/
i4
IIFRreResEntry()
{
    /*
    **	Check to see that the forms system has been
    **	initialized and that there is a current frame.
    */
    if (RTchkfrs(IIstkfrm))
    {
	/*
	** Check that not in the middle of a submenu statement.
	** This is a no-op if we are.
	*/
	if (!IIissubmu())
	{
	    /*
	    ** Set the 'original field' to an illegal value (reachable
	    ** fields have non-negative sequence numbers) such that
	    ** when comparing current and original locations on return
	    ** to the forms system the system will think the user has moved.
	    */
	    IIstkfrm->fdrunfrm->frres2->origfld = BADFLDNO;
	}
    }
}
