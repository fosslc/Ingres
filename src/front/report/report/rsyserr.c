/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	 <ex.h> 
# include	 <ug.h> 
# include	 <er.h> 

/*
**   R_SYSERR - system error message print and abort.  This simply calls
**	IIUGerr().  This can be called with up to five
**	arguments and acts like a IIUGfmt().
**
**	Parameters:
**		msgid		- identifes error message format.
**		p0,... p9	- Up to 10 arguments allowed, ala IIUGfmt.
**
**	Returns:
**		Aborts.
**
**	History:
**		3/12/81 (ps) - written.
**		8/18/87 (peterk) - modifed to call IIUGerr().
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



/* VARARGS1 */
VOID
r_syserr(msgid, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
ER_MSGID	msgid;
PTR	p0, p1, p2, p3, p4, p5, p6, p7, p8, p9;
{
	IIUGerr(msgid, UG_ERR_FATAL, 10, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
	/* NOTREACHED */

}
