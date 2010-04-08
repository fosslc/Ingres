/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	 <fmt.h> 
# include	<cm.h>
# include	"format.h"

/*
**   F_GNUM - return an unsigned short integer from a string.
**
**	Parameters:
**		string - start of string.
**		number - address of place for result.
**
**	Returns:
**		Ptr to string, after processing.  This is return
**		regardless of whether it is good or bad.
**
**		To find out if an error occurs, check the value
**		of number on return.  If negative, an error occurred.
**		This is because this routine works on unsigned integers.
**
**	Called by:
**		f_setfmt.
**
**	History:
**		6/20/81 (ps) - written.
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char	*
f_gnum(string,number)
register char	*string;
register i4	*number;
{
	/* start of routine */

	*number = 0;

	/* make sure it starts out with a digit */

	if (!CMdigit(string))
	{
		*number = -1;
		return(string);
	}

	while (CMdigit(string))
	{
		*number = (10 * (*number)) + (*string - '0');
		CMnext(string);
	}

	return(string);
}
