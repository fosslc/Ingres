/*
** encoding.c
**
**	Routine to encode binary data as text.
**
** Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	10/25/87 (dkh) - Error message cleanup.
**	08/29/88 (tom) - Eliminated IIUGerr call because the condition
**			 clearly would never arise.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"
/*
** N.B.
** Don't include decls.h because that brings in all of vifred!!
*/
# include	<encoding.h>
# include	 <si.h>


i4
CFencode(addr, textbuf, binsize)
char	*addr;
char	*textbuf;
i4	binsize;
{
	register char		c;
	register i4		state = 0;
	register u_char c1, c2, c3, c4, temp;
	register char		*cp;
	register char		*locp;
	i4			total = 0;
	i4			i;	/* loop counter */

	cp = textbuf;
	locp = addr;
	for (i = 0; i < binsize; i++, locp++)
	{
		c = *locp;
		switch(state)
		{
		  case 0:
			c1 = c & MASK0_1;
			c1 = c1 >> 2;
			c1 += SPACE;
			c2 = c & MASK2_7;
			c2 = c2 << 4;
			state = 1;
			break;

		  case 1:
			temp = c & MASK0_3;
			temp = temp >> 4;
			c2 |= temp;
			c2 += SPACE;
			c3 = c & MASK4_7;
			c3 = c3 << 2;
			state = 2;
			break;

		  case 2:
			temp = c & MASK0_5;
			temp = temp >> 6;
			c3 |= temp;
			c3 += SPACE;
			c4 = c & MASK6_7;
			c4 += SPACE;
			*cp++ = c1;
			*cp++ = c2;
			*cp++ = c3;
			*cp++ = c4;
			total += 3;
			state = 0;
			break;

		  default:
			/* there used to be an IIUGerr here.. but 
			   merely by looking in this function, it is easy
			   to determine that the default case will never
			   be taken.. state can acheive no values
			   other than 0, 1 and 2, all of which are covered. */
			return (-1);
		}
	}

	switch(state)
	{
	  case 0:
		*cp = '\0';
		break;

	  case 1:
		c2 += SPACE;
		*cp++ = c1;
		*cp++ = c2;
		*cp++ = SPACE;
		*cp++ = SPACE;
		*cp = '\0';
		total += 1;
		break;

	  case 2:
		c3 += SPACE;
		*cp++ = c1;
		*cp++ = c2;
		*cp++ = c3;
		*cp++ = SPACE;
		*cp = '\0';
		total += 2;
		break;

	  default:
		/* there used to be an IIUGerr here.. but
		   merely by looking in this function, it is easy
		   to determine that the default case will never
		   be taken.. state can acheive no values
		   other than 0, 1 and 2, all of which are covered. */
		return (-1);
	}
#ifndef EBCDIC
	/*
	** Replace any backslash ('\') characters with a tilde ('~'), so
	** that the backslash won't be dropped when the text field is
	** read out of the database.  CFdecode will replace any '~'s
	** with '\'s.
	*/
	for (cp = textbuf; *cp; cp++)
	{
		if (*cp == '\\')
			*cp = '~';
	}
#endif

	return (total);
}
