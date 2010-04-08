/*
**  ITinit.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**	Written - 6/22/85 (dkh)
**	19-jun-87 (bab)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	12/24/87 (dkh) - Performance changes.
**	02/18/89 (dkh) - Yet more performance changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<nm.h> 
# include	"it.h"
# include	<er.h> 


FUNC_EXTERN	i4	TDtgetent();
FUNC_EXTERN	i4	TDtgetnum();
FUNC_EXTERN	VOID	ITgetent();
FUNC_EXTERN	VOID	TDtcaloc();

static bool	ITinitonce = FALSE;


STATUS
ITinit()
{
	char		*sp;
	char		termbuf[100];
	char		genbuf[IT_TC_SIZE];

	if (ITinitonce)
		return(OK);

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

	if (TDtgetent(genbuf, termbuf, NULL) != 1)
	{
		return(FAIL);
	}
	ITinitonce = TRUE;
	ITgetent();
	return(OK);
}
