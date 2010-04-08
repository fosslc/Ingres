/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<itline.h>
# include	"itdrloc.h"
# include	<nm.h>
# include	<te.h>
# include	<st.h>
# include	<er.h>

/*{
** Name:	ITldopen	- initialize the drawing characters.
**
** Description:
**	This routine gets the terminal name from emvironment variable,
**	then pick up the character sequence of drawing line from TERMCAP
**	entries.  If this done correctly, this function returns the
**	character pointer of the character set selection sequence that
**	designate the graphic character set as G1, otherwise returns 
**	NULL (the drawing line characters are consutituted by default 
**	characters, such as '-', '+' and '|').
**
** Inputs:
**
** Outputs:
**	Returns:
**		ESC sequence of the character set selection
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06-nov-1986 (yamamoto) - first written
**	08/14/87 (dkh) - ER changes.
**	9/87 (peterk) - bugfixes in ITldopen().
**	11/87 (peterk) - more bug fixes in ITldopen().
**	12/24/87 (dkh) - Changes to TDtgetent interface.
**	05/20/88 (dkh) - More ER changes.
**	02/18/89 (dkh) - Yet more performance changes.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
*/

FUNC_EXTERN	i4	TDtgetent();
FUNC_EXTERN	char	*STalloc();

static	char	tspace[200] = {0};	/* Space for capability strings */
static	char	*aoftspace = {0};	/* Address of tspace for relocation */

char *
ITldopen()
{
	char		*sp;
	char		termbuf[100];

	NMgtAt(ERx("TERM_INGRES"), &sp);
	if (sp == NULL || *sp == '\0')
		NMgtAt(TERM_TYPE, &sp);
	if (sp == NULL)
		sp = ERx("");

	STlcopy(sp, termbuf, sizeof(termbuf)-1);
	CVlower(termbuf);
	itlsetterm(termbuf);
	return ((char *) drchtbl[DRLD].drchar);
}


/*
**  Terminal initialization routines.
*/

itlsetterm(type)
register char 	*type;
{
	static	char	genbuf[2048];

	itprset();
	if (type[0] == '\0' || TDtgetent(genbuf, type, NULL) != 1)
	{
		itlsetdef();
		return;
	}
	aoftspace = tspace;
	itlzap();		/* get terminal description */
}


/*
**  This routine gets all the terminal falgs from the termcap database.
*/

itlzap()
{
	FUNC_EXTERN char	*TDtgetstr();
	register char	*namp;
	register i4	i;

	namp = ERx("ldlelsqaqbqcqdqeqfqgqhqiqjqk");

	for (i = 0; *namp != '\0'; i++, namp += 2)
	{
		if ((drchtbl[i].drchar =
			(u_char *) TDtgetstr(namp, &aoftspace)) == 0)
		{
			itlsetdef();
			return;
		}
		drchtbl[i].drlen = STlength((char *) drchtbl[i].drchar);
	}
}


itprset()
{
	drchtbl[DRLD].prchar = (u_char *) CHLD;
	drchtbl[DRLD].prlen = STlength(CHLD);

	drchtbl[DRLE].prchar = (u_char *) CHLE;
	drchtbl[DRLE].prlen = STlength(CHLE);

	drchtbl[DRLS].prchar = (u_char *) CHLS;
	drchtbl[DRLS].prlen = STlength(CHLS);

	drchtbl[DRQA].prchar = (u_char *) CHQA;
	drchtbl[DRQA].prlen = STlength(CHQA);

	drchtbl[DRQB].prchar = (u_char *) CHQB;
	drchtbl[DRQB].prlen = STlength(CHQB);

	drchtbl[DRQC].prchar = (u_char *) CHQC;
	drchtbl[DRQC].prlen = STlength(CHQC);

	drchtbl[DRQD].prchar = (u_char *) CHQD;
	drchtbl[DRQD].prlen = STlength(CHQD);

	drchtbl[DRQE].prchar = (u_char *) CHQE;
	drchtbl[DRQE].prlen = STlength(CHQE);

	drchtbl[DRQF].prchar = (u_char *) CHQF;
	drchtbl[DRQF].prlen = STlength(CHQF);

	drchtbl[DRQG].prchar = (u_char *) CHQG;
	drchtbl[DRQG].prlen = STlength(CHQG);

	drchtbl[DRQH].prchar = (u_char *) CHQH;
	drchtbl[DRQH].prlen = STlength(CHQH);

	drchtbl[DRQI].prchar = (u_char *) CHQI;
	drchtbl[DRQI].prlen = STlength(CHQI);

	drchtbl[DRQJ].prchar = (u_char *) CHQJ;
	drchtbl[DRQJ].prlen = STlength(CHQJ);

	drchtbl[DRQK].prchar = (u_char *) CHQK;
	drchtbl[DRQK].prlen = STlength(CHQK);
}


itlsetdef()
{
	drchtbl[DRLD].drchar = (u_char *) CHLD;
	drchtbl[DRLD].drlen = STlength(CHLD);

	drchtbl[DRLE].drchar = (u_char *) CHLE;
	drchtbl[DRLE].drlen = STlength(CHLE);

	drchtbl[DRLS].drchar = (u_char *) CHLS;
	drchtbl[DRLS].drlen = STlength(CHLS);

	drchtbl[DRQA].drchar = (u_char *) CHQA;
	drchtbl[DRQA].drlen = STlength(CHQA);

	drchtbl[DRQB].drchar = (u_char *) CHQB;
	drchtbl[DRQB].drlen = STlength(CHQB);

	drchtbl[DRQC].drchar = (u_char *) CHQC;
	drchtbl[DRQC].drlen = STlength(CHQC);

	drchtbl[DRQD].drchar = (u_char *) CHQD;
	drchtbl[DRQD].drlen = STlength(CHQD);

	drchtbl[DRQE].drchar = (u_char *) CHQE;
	drchtbl[DRQE].drlen = STlength(CHQE);

	drchtbl[DRQF].drchar = (u_char *) CHQF;
	drchtbl[DRQF].drlen = STlength(CHQF);

	drchtbl[DRQG].drchar = (u_char *) CHQG;
	drchtbl[DRQG].drlen = STlength(CHQG);

	drchtbl[DRQH].drchar = (u_char *) CHQH;
	drchtbl[DRQH].drlen = STlength(CHQH);

	drchtbl[DRQI].drchar = (u_char *) CHQI;
	drchtbl[DRQI].drlen = STlength(CHQI);

	drchtbl[DRQJ].drchar = (u_char *) CHQJ;
	drchtbl[DRQJ].drlen = STlength(CHQJ);

	drchtbl[DRQK].drchar = (u_char *) CHQK;
	drchtbl[DRQK].drlen = STlength(CHQK);
}


/*{
** Name:	ITsetvl	- Set vertical separator character.
**
** Description:
**	This routine sets the vertical separator that will be
**	used by routines "ITlndraw()" and "ITlnprint()".
**	Necessary to support the "-v" optin of the terminal monitor.
**
**	The default separator character is '|'.
**
** Inputs:
**	separator	Vertical separator character to set.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
** Side Effects:
**	None.
**
** History:
**	06-nov-1986 (yamamoto)
**		first written
**	10/23/87 (peterk) - modified to affect only .prchar and .prlen
**		(print chars) and leave .drchar and .drlen (line drawing
**		sequences) alone.
*/

VOID
ITsetvl(separator)
char	*separator;
{
	drchtbl[DRQK].prchar = (u_char *) STalloc(separator);
	drchtbl[DRQK].prlen  = STlength(separator);
}
