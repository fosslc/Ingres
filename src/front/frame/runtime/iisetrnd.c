/*
**	Copyright (c) 2007 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<tm.h>
# include	<mh.h>

/**
** Name:	iisetrnd.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIset_random()
**	Private (static) routines defined:
**
** History:
**	29-Jan-2007 (kiria01) b117277
**	    Created to add runtime support for RANDOM_SEED.
**/

/*{
** Name:	IIset_random	-	Set random seed
**
** Description:
**	An SQL or EQUEL 'SET RANDOM_SEED' statement will generate a call to this routine.
**	It simply calls MHsrand2.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	seed		The seed to set. If passed as 0 then a seed
**			based on a sub-second timer will be used.
**
** Outputs:
**	None
**
** Returns:
**	void
**
** Exceptions:
**	none
**
** Side Effects:
**	Sets active global seed
**
** History:
**	26-Jan-2007 (kiria01) b117277
**	    Created to provide abf with the means of setting the global
**	    random seed within it's own or a built image.	
*/
void
IIset_random(i4 seed)
{
	if (!seed)
	{
		SYSTIME tm;
		TMnow(&tm);
		seed = (u_i4)tm.TM_secs ^ (u_i4)tm.TM_msecs;
	}
	MHsrand2(seed);
}
