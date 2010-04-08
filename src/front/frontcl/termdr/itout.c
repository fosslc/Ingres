/*
**  itout.c
**
**  Put out an international character.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**	Written - 6/22/85 (dkh)
**
**	12/03/85 (cgd) Removed incorrect call to CHunctrl()
**		which translated ESC to `^['.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	"it.h"



ITout(c, buf)
u_char	c;
u_char	*buf;
{
	u_char	*cp;
	u_char	*sp;

	if ((c > MAX_7B_ASCII) && ZZB)
	{
		cp = (u_char *) ZZB;
		while (*cp)
			*buf++ = *cp++;
		*buf++ = (u_char) (c - ((char) ZZC));
	}
	else
	{
		if (CMcntrl(&c))
		{
			sp = (u_char *) CMunctrl(&c);
			while(*sp)
			{
				*buf++ = *sp++;
			}
		}
		else
		{
			*buf++ = c;
		}
	}
	*buf = '\0';
}
