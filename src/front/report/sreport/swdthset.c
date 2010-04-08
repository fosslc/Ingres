/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <cv.h>		/* 6-x_PC_80x86 */
#include <st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include <stype.h>
#include <sglob.h>
#include <er.h>

/*
**   S_WIDTH_SET - Sets the width of the report page.
**	
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Error Messages:
**		None
**
**	History:
**	05-apr-90 (sylvia)
**		Initial revision.
**      11-oct-90 (sylviap)
**              Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN STATUS s_w_row();
FUNC_EXTERN char   *s_g_token();

VOID
IISRwds_WidthSet()
{
	char            *width = NULL;
	i4		type;
	i4             rtn_char;               /* dummy variable for sgskip */

	if (Cact_ren == NULL)
	{
		s_error(906, FATAL, NULL);
	}
	if (St_width_given)
	{       /* already specified */
		s_error(1028, NONFATAL, NULL);
	}
	save_em();

	/* Set up fields in Command */

	STcopy(NAM_WIDTH, Ctype);
	Csequence = 0;
	STcopy(ERx(" "),Csection);
	STcopy(ERx(" "),Cattid);
	STcopy(ERx(" "),Ccommand);

	type = s_g_skip(TRUE, &rtn_char);
	if ( type != TK_NUMBER && type != TK_DOLLAR)
		s_error(1029, FATAL, s_g_token(FALSE), NULL);
	width = s_g_token(TRUE);
	CVlower(width);
	STcopy(width,Ctext);
	_VOID_ s_w_row();

	get_em();
	St_width_given = TRUE;

	return;
}
