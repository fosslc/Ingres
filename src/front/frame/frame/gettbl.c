/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
*
**	static	char	Sccsid[] = "@(#)gettbl.c	30.2	2/4/85"
*/


/*
** GETTBL.C
**
** empty all the data still in the table to the user
** empty from the top 
**
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	03/05/87 (dkh) - Added support for ADTs.
**	THESE ROUTINES ARE NOT USED.
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
# include	"fdfuncs.h"

/*
** FDTBLGET
**	empty the table field's data by calling puttop
** RETURNS
**	TRUE	no problems encountered
**	FALSE	if fdgetrow returns FALSE
**		if tblfld does not exist or is not a table field
*/
FDtblget()
{
}

/*
** FDGETROW
**
** get the value from row i and put in the data row
*/
FDcopyrow()
{
}
