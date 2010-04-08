/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)ringerr.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_ING_ERR - routine is called by the IIerror routine in EQUEL
**	if any INGRES errors occur while using the report formatter.
**	This simply makes note of the error in a global variable
**	and allows EQUEL error processing to continue.
**
**	Parameters:
**		err_num - the INGRES error number.
**	
**	Returns:
**		err_num, unchanged.  This allows the EQUEL
**		error processing to continue, and a message will be
**		printed.
**
**	Side Effects:
**		Set global variable Err_ingres to the error number.
**
**	Called by:
**		IIerror. (in EQUEL)
**
**	Trace Flags:
**		2.0.
**
**	History:
**		3/5/81 (ps) - written.
**		11/3/89 (elein) 
**			Ignore errors if silent and continue on setup error
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_ing_err (err_num)
i4	*err_num;
{
	/* start of routine */



	Err_ingres = *err_num;
	St_ing_error = TRUE;
	/*
	** If this is a setup or cleanup error
	** and the want errors ignored 
	** and they don't even want to see the messages
	**   Pretend everything is ok
	*/
	if( St_in_setup && St_setup_continue && St_silent)
	{
		return ((i4)0);
	}
		
	return (*err_num);

}
