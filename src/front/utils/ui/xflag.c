/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <uigdata.h>


/**
**  Name: xflag.c - Temporary cover from IIxflag to new interface IIx_flag.
**
**  Defines:
**	IIxflag		- Return pointer to static string for the -X flags.
**
**  History:
**	21-oct-1987	- Written (ncg)
**/

/* Global data */
FUNC_EXTERN STATUS	IIx_flag();

/*{
**  Name: IIxflag - Return value of the -X flags for GCA interface.
**
**  Description:
**	This routine used to be in LIBQ and return a static string with the
**	-X flags from the IN routines.  With the new communications this has
**	changed, but in order not to modify callers of UTexe, we provide this
**	cover as a temporary measure.  Callers of UTexe should modify their
**	code to the following:
**
**		if (IIx_flag(database_name, x_flag_buf) == OK)
**			UTexe( stuf, x_flag_buf, stuff);
**		else
**			error;
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Pointer to static buffer with the -X flags as returned by IIx_flag.
**	Errors:
**	    None
**
**  Side Effects:
**	
**  History:
**	21-oct-1987	- Written (ncg)
*/

char *
IIxflag()
{
    static char	x_flag_buf[MAX_LOC] ZERO_FILL;

    _VOID_ IIx_flag(IIUIdbdata()->database, x_flag_buf);
    return x_flag_buf;
}

