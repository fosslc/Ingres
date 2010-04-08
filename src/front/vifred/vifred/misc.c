/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** misc.c
**
** some useful things which don't belong anyplace
**
** History:
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>


/*
** save a string
*/

char *
saveStr(str)
char	*str;
{
	char	*cp;

	if ((cp = (char *)FEreqmem((u_i4)0, (u_i4)(STlength(str)+1), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("saveStr"));
	}

	STcopy(str, cp);
	return(cp);
}

/*
** copy size bytes
*/

VOID
copyBytes(to, frm, size)
register char	*to,
		*frm;
register i4	size;
{
	MEcopy(frm, (u_i2) size, to);
}

