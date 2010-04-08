/*
**  itxout.c
**
**  Scan src buffer translating any characters greater than 0177
**  to the appropriate escape sequence for output to the terminal.
**  Translated text is placed in dst buffer that is also
**  passed to this routine.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**    6/27/85 - Written (dkh)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"it.h"


i4
ITxout(srcbuf, dstbuf, count)
char	*srcbuf;
char	*dstbuf;
i4	count;
{
	i4		i;
	i4		j;
	u_char	*sp;
	u_char	*dp;
	u_char	*zcp;

# ifndef IBM370
	if (ZZB)
	{
		sp = (u_char *) srcbuf;
		dp = (u_char *) dstbuf;

		for (i = 0, j = 0; i < count; i++, sp++)
		{
			if (*sp > MAX_7B_ASCII)
			{
				zcp = (u_char *) ZZB;
				while (*zcp)
				{
					*dp++ = *zcp++;
					j++;
				}
				*dp++ = (u_char) (*sp - (char) ZZC);
				j++;
			}
			else
			{
				*dp++ = *sp;
				j++;
			}
		}
		*dp = '\0';
	}
	else
# endif
	{
		MEcopy(srcbuf, (u_i2) count, dstbuf);
		dstbuf[count] = '\0';
		j = count;
	}
	return(j);
}
