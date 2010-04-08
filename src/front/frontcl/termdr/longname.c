# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
/*
**  LONGNAME.c - Return long name of a terminal
*/

# define	reg	register

/*
**  This routine fills in "def" with the long name of the terminal.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
*/


static char	_longname[80] = {0};

char *
TDlongname(bp, def)
reg	char	*bp;
reg	char	*def;
{
	reg char	*cp;

	STcopy(def, _longname);
	while (*bp && *bp != ':' && *bp != '|')
		bp++;
	while (*bp == '|')
	{
		bp++;
		cp = _longname;
		while (*bp && *bp != ':' && *bp != '|')
		{
			*cp++ = *bp++;
		}
		*cp = 0;
	}
	return(_longname);
}
