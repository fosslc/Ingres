/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<itline.h>
# include	"itdrloc.h"

/*{
** Name:	ITldclose	- Reset terminal to normal state.
**
** Description:
**	This routine resets the terminal to the state it was in
**	before "ITldopen()" was called.  For a vt100, this may
**	entail deassigning the line drawing character from
**	character set G1 and setting the current character
**	set back to G0.  This and routine "ITldopen()" must
**	coordinate the use and setting of any terminal state
**	information so that calls to "ITlopen()" and "ITldclose()",
**	in this order, can bedone an infinite number of times.
**
**	If the terminal does not support hardware line drawing, then
**	this routine does not have to twiddle the terminal at all.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		character string reset to G0 characrer set
**	
**	Exceptions:
**		None.
**
** Side Effects:
**	Terminal state and state information variables may be changed.
**
** History:
**	19-feb-1987 (yamamoto)
**		first written
*/

char *
ITldclose()
{
	return ((char *) drchtbl[DRLE].drchar);
}
