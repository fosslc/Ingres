/*
** Copyright (c) 2004 Ingres Corporation
*/


# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abfcnsts.h>
# include	<ut.h>
/* VARARGS3 */
FUNC_EXTERN	i4 abexeprog();

/*
**	abfgbf.c
**		Interface to graph.
**	
**	Defines
**		abgbfgraf()
*/

/*
**	abgbfgraf
**		Draw a graph.  Taken from iigraph.
**
**	BUG 4564
**	Just call GRAPH to draw the graph since it will handle
**	logicals and all that.
**
**	Parameters:
**
**
**	Returns:
**
**
**	Called by:
**
**
**	Side Effects:
**
**
**	Trace Flags:
**
**
**	Error Messages:
**
**
**	History:
**		Written 11/7/82 (jen)
**		Modified 6/10/85 (jrc)
**			To call graph directly.
**	03/22/91 (emerson)
**		Fix interoperability bug 36589:
**		Change all calls to abexeprog to remove the 3rd parameter
**		so that images generated in 6.3/02 will continue to work.
**		(Generated C code may contain calls to abexeprog).
**		This parameter was introduced in 6.3/03/00, but all calls
**		to abexeprog specified it as TRUE.  See abfrt/abrtexe.c
**		for further details.
**
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
abgbfgraf(dbname, grafname, device, output)
	char	*dbname;			/* Database name */
	char	*grafname;			/* Graph name */
	char	*device;			/* Name of device to use */
	char	*output;			/* Output device */
{
 	char		buf[20];	/* buffer for loc LOCATION */
 	i4		rval = OK;

	if ((rval = abexeprog(ERx("graph"), ERx("name = %S"),
					1, grafname, NULL)) != OK)
	{

		STprintf(buf, ERx("%x"), rval);
		abusrerr(ABPROGERR, ERget(FE_graph), buf, NULL);
	}
	return (rval);
}
