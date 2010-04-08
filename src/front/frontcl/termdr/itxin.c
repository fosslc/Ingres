/*
**  itxin.c
**
**  Scan src buffer translating any special lead in characters
**  to the appropriate 8 bit value.  Translated text is placed
**  in dst buffer that is also passed to this routine.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**    6/27/85 - Written (dkh)
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"it.h"


ITxin(srcbuf, dstbuf)
char	*srcbuf;
char	*dstbuf;
{
	u_char	*sp;
	u_char	*dp;
	u_char	*zap;
	u_char	*tp;
	bool		found;

	if (ZZA)
	{
		sp = (u_char *) srcbuf;
		dp = (u_char *) dstbuf;
		while (*sp)
		{
			if (*sp == *ZZA)
			{
				zap = (u_char *) ZZA;
				zap++;
				tp = sp;
				tp++;
				found = TRUE;
				while (*zap && *tp)
				{
					if (*tp++ != *zap++)
					{
						found = FALSE;
						break;
					}
				}

				/*
				**  Regardless whether we have a special
				**  lead in sequence or not, we must check
				**  to make sure there is a character to
				**  add the offset to.  Otherwise, pretend
				**  we didn't find any match.
				*/

				if (!found || *tp == '\0')
				{
					*dp++ = *sp++;
				}
				else
				{
					*dp++ = (u_char) (*tp + (char) ZZC);
					sp = ++tp;
				}
			}
			else
			{
				*dp++ = *sp++;
			}
		}
		*dp = '\0';
	}
	else
	{
		STcopy(srcbuf, dstbuf);
	}
}
