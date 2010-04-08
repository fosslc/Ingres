/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	static	char	*Sccsid = "%W%	%G%";
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	09/05/87 (dkh) - More ER changes.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ertb.h"


IItbug(errmsg)
char	*errmsg;
{
	char		bugbuf[ER_MAX_LEN];
	char		msgbuf[256];
	FUNC_EXTERN i4	IIerrnum();

	STcopy(errmsg, msgbuf);
	STprintf(bugbuf, ERget(S_TB0003_INTERNAL_EQUEL_BUG), msgbuf);
	IIerrdisp(bugbuf);
}
