/*
**  itgetent
**
**  This routine gets all the terminal flags from the termcap database
**  for international support.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**    Written - 6/22/85 (dkh)
**		04/11/86 (seiwald)	 Revamped.
**	08/14/87 (dkh) - ER changes.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
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

GLOBALREF	char	*ZZA;
GLOBALREF	char	*ZZB;
GLOBALREF	i4	ZZC;

void
ITgetent()
{
	static char	itspace[32] = {0};
	char		*b = itspace;
	FUNC_EXTERN char	*TDtgetstr();

	/*
	**  Get the offset value (ZC), and string values for translating
	**  input (ZA) and output (ZB) characters.
	*/

	ZZA = TDtgetstr(ERx("ZA"), &b );
	ZZB = TDtgetstr(ERx("ZB"), &b );
	ZZC = TDtgetnum(ERx("ZC"));
}
