/*
**	decoding.c
*/

/*
** decoding.c
**
**	Routines to decode text back to binary data
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	19-jun-87 (bab)	Code cleanup.
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
# include	<si.h>
# include	<encoding.h>


i4
CFdecode(textbuf, block, binsize)
char	*textbuf;
char	*block;
i4	binsize;
{
	register char	*binp, *textp;
	char		c1, c2, c3, c4;
	char		outc1, outc2, outc3;
	i4		convsize = 0;

#ifndef EBCDIC
	/*
	** Convert any tilde ('~') characters in the text buffer passed
	** in back to backslashes ('\\').  The conversion was made in
	** the encoding routine so that the backslash wouldn't get
	** dropped as the field is retrieved from the database.
	*/
	for (textp = textbuf; *textp; textp++)
	{
		if (*textp == '~')
			*textp = '\\';
	}
#endif

	binp = block;
	textp = textbuf;
	while (binsize > 0)
	{
		c1 = *textp++ - SPACE;
		c2 = *textp++ - SPACE;
		c3 = *textp++ - SPACE;
		c4 = *textp++ - SPACE;
		outc1 = (c1 << 2) | ((c2 & MASK0_3) >> 4);
		outc2 = ((c2 & MASK4_7) << 4) | ((c3 & MASK0_1) >> 2);
		outc3 = ((c3 & MASK2_7) << 6) | c4;
		if (binsize >= 3)
		{
			*binp++ = outc1;
			*binp++ = outc2;
			*binp++ = outc3;
			binsize -= 3;
			convsize += 3;
		}
		else
		{
			switch (binsize % 3)
			{
			  case 1:
				*binp++ = outc1;
				binsize -= 1;
				convsize += 1;
				break;

			  case 2:
				*binp++ = outc1;
				*binp++ = outc2;
				binsize -= 2;
				convsize += 2;
				break;

			  default:
				return -1;
			}
		}
	}

	return (convsize);
}
