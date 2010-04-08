/*
**  TERMSIZE.C
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	12/24/87 (dkh) - Performance changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<nm.h> 
# include	<er.h>

# define	reg		register

FUNC_EXTERN	i4	TDtgetent();
FUNC_EXTERN	i4	TDtgetnum();
FUNC_EXTERN	VOID	TDtcaploc();

VOID	TDfsize();

TDtermsize(width, line, initstr, buf)
i4	*width;
i4	*line;
char	**initstr;
char	*buf;
{
	char		*sp;
	char		termbuf[100];
	char		genbuf[1024];
	LOCATION	tloc;
	char		locbuf[MAX_LOC + 1];

	*width = 80;
	*line = 24;
	NMgtAt(ERx("TERM_INGRES"), &sp);
	if (sp == NULL || *sp == '\0')
	{
		NMgtAt(TERM_TYPE, &sp);
	}
	if (sp == NULL || *sp == '\0')
	{
		return(FAIL);
	}
	STlcopy(sp, termbuf, sizeof(termbuf)-1);
	CVlower(termbuf);

	TDtcaploc(&tloc, locbuf);

	if (TDtgetent(genbuf, termbuf, &tloc) != 1)
	{
		return(FAIL);
	}
	TDfsize(width, line, initstr, buf);
	return(OK);
}

/*
**  This routine gets all the terminal flags from the termcap database.
*/

void
TDfsize(width, line, initstr, buf)
reg	i4	*width;
reg	i4	*line;
char	**initstr;
char	*buf;
{

	reg	char	*namp;
	reg	char	***sp;
	char	*aoftspace = buf;
	char	**sizestrs[2];
	FUNC_EXTERN char	*TDtgetstr();


	sizestrs[0] = initstr;
	sizestrs[1] = 0;
	/*
	**  get "CO" column value and "LI" line value
	*/

	*width = TDtgetnum(ERx("co"));
	*line = TDtgetnum(ERx("li"));

	/*
	** get string values
	*/

	namp = ERx("is");
	sp = sizestrs;
	do
	{
		**sp = TDtgetstr(namp, &aoftspace);
		sp++;
		namp += 2;
	} while (*namp);
}
