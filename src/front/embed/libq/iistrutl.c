/*
** Copyright (c) 2004 Ingres Corporation
*/

# include 	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>

/**
+*  Name: iistrutil.c - String utility file for EQUEL programs.
**
**  Defines:
**	IIstrconv - String conversion routine for host strings.
-*
**  History:
**	09-jun-1984	- Written. (ncg)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/

/*{
+*  Name: IIstrconv - General II string conversion utility.
**
**  Description:
**	Converts host strings (standard operation is to copy, lowercase
**	and trim white).
**
**  Inputs:
**    flag - Lowercase or Trim white
**    orig - Caller string
**    buf  - Internal buffer
**    len  - Length of buffer (if zero assume very large)
**
**  Outputs:
**	buf - Modified to contain converted string.
**	Returns:
**	    Pointer to buf.
**	Errors:
**	    None
-*
*/

char	*
IIstrconv(i4 flag, const char *orig, char *buf, i4 len)
{
    register	char	*s;

    if (orig == (char *)0)
	return (char *)0;
    if (len == 0)
	len = DB_GW4_MAXSTRING;
    s = buf;
    STlcopy( orig, s, len );
    if (flag & II_TRIM)
	STtrmwhite( s );
    if (flag & II_LOWER)
	CVlower( s );
    return s;
}
