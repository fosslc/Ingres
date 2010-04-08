# include	 <compat.h>
#include    <gl.h>
# include	 <lo.h> 
# include 	  <er.h>  
# include	 "lolocal.h" 

/*{
** Name:	LOisfull - See if we have a full pathname
**
** Description:
**	This is problematical in VMS: there are several possibilities:
**
**		1. No device or directory 	-- not full  	(FILE.TYPE)
**		2. No device and a top directory -- not full ([DIR]FILE.TYPE)
**		3. No device and no top directory -- not full ([.SUB]FILE.TYPE)
**		4. A device and a top directory -- full (DEV:[DIR]FILE.TYPE)
**		5. A device and no top directory-- ??? (DEV:[.SUB]FILE.TYPE)
**		6. A bare device		-- full (DEV:)
**
** 	We'll call number 5 full, although this isn't obvious; the point is
**	that we can't apply it as the tail in LOaddpath.  Ergo, all we look
**	for is the presence of a device.
**
** Inputs:
**	loc	LOCATION	location to check
**
** Outputs:
**	none
**
**	Returns:
** 		bool		TRUE if it's full
**
**	Exceptions:
** 		none
**
** Side Effects:
**
** History:
**  	10/89 (Mike S)	Use new LOCATION fields.
**      16-jul-93 (ed)
**	    added gl.h
*/

bool
LOisfull(loc)
LOCATION	*loc;
{
	return (bool)(loc->devlen != 0);
}
