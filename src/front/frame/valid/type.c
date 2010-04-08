/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h>
# include	<afe.h>
# include	<er.h>
# include	"erfv.h"

/*
** routines which handle type compatibility among the validation
** v_strings.
**
**  History:
**	08/14/87 (dkh) - ER changes.
*/


FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	STATUS	afe_tyoutput();


VOID
vl_typstr(dbv, buf)
DB_DATA_VALUE	*dbv;
char		*buf;
{
	DB_USER_TYPE	usertype;

	if (afe_tyoutput(FEadfcb(), dbv, &usertype) != OK)
	{
		vl_par_error(ERx("vl_typstr"), ERget(E_FV001D_parser_error));
		return;
	}
	STcopy(usertype.dbut_name, buf);
	return;
}
