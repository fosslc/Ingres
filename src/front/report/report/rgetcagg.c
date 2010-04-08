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
# include	<cm.h>
# include	<st.h>
# include	<er.h>

/*
**   R_GET_CAGG - returns whether the name is "cum" or "cumulative".
**
**	Parameters:
**		name - ptr to name to check.
**
**	Returns:
**		TRUE if the name is "cum" or "cumulative".
**
**	History:
**		6/1/81 (ps) - written.
**		9-jul-86 (mgw) - Bug 9784
**			restrict allowable keywords to only those listed in
**			chapt. 4 or report writer manual to prevent restricting
**			number of words available to user for other purposes.
**		9-feb-1989 (danielt)
**			took out CVlower() (use STbcompare()).
*/

bool
r_gt_cagg(cname)
register	char	*cname;
{

	return (STbcompare(cname, 0,  ERx("cum"), 0, TRUE) == 0 ||
		STbcompare(cname, 0,  ERx("cumulative"), 0, TRUE) == 0);
}
