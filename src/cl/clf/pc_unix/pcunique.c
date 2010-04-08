/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	 <compat.h>
# include	 <gl.h>
# include	 <cs.h>
# include	 <st.h>
# include	 <pc.h>

/* PCunique -- generate a unique character string
**	for a given user.
**
**	History
**		written 1/24/84 (dreyfus) from VMS CL
*/

VOID
PCunique(numb)
char	*numb;
{
	static	char	uniq_str[8] = "\0";
	static	bool	got_uniq = FALSE;
	i4	numbuf;

	if (!got_uniq)
	{
		PCpid(&numbuf);
		STprintf(uniq_str, "%x", numbuf);
		got_uniq = TRUE;
	}
	STcopy(uniq_str, numb);
}
/* END COMPATIBILITY */
