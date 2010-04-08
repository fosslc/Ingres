# include	<compat.h>
# include	<clconfig.h>
# include	<gl.h>
# include	<pc.h>
# include	<lo.h>
# include	"LOerr.h"

/* LOrename
**
**	Rename ("move" in Unix parlance) a file.
**
**Copyright (c) 2004 Ingres Corporation
**	History
**		03/dd/83 -- (mmm) written
**		04/12/85 -- (rbr) Rewrote to accomodate renaming
**				  of directories on Unix.
**		09/18/86 -- (greg) zap directory delete code.
**	31-jan-92 (bonobo)
**	    Replaced multiple question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	05-jun-95 (sweeney)
**	    Use rename(2) if we have it.
*/

STATUS
LOrename(old_loc, new_loc)
LOCATION	*old_loc;
LOCATION	*new_loc;
{
	STATUS	ret_val = OK;
	i2	whatzit;


	LOisdir(old_loc, &whatzit);

	if (whatzit != ISFILE)
	{
		ret_val = LO_NOT_FILE;
	}
	else
	{
		/* if this call fails errors will show up on the
		** next step.  It doesn't check for error here because
		** -1 can come back if the file doesn't exist or other
		** errors.  If the file doesn't exist then that's just
		** fine.  
		*/

# ifdef xCL_035_RENAME_EXISTS
		ret_val = rename( old_loc->string, new_loc->string);
# else
		unlink(new_loc->string);

		if (link(old_loc->string, new_loc->string))
		{
			ret_val = FAIL;
		}
		else if (unlink(old_loc->string))
		{
			ret_val = FAIL;
		}
# endif
	}

	return (ret_val);
}

