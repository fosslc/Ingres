/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h> 
# include	<sglob.h> 
# include	<cm.h>
# include	<er.h>

/*{
** Name:	s_nam_set() -	Setup For New Report Specification with Name.
**
** Description:
**   S_NAM_SET - set up for a new report specification.  This is called
**	when the .NAME command is encountered in the input.  It sets
**	up a REN structure for this report, and starts the linked lists.
**	The format of the .NAME command is:
**		.NAME repname [f]
**	with "repname" being the report name, and the optional "f"
**	specified for RBF reports.
**
** Parameters:
**	none.
**
** Returns:
**	None.
**
** Called by:
**	s_readin().
**
** Side Effects:
**	none.
**
** History:
**	6/1/81 (ps)	written.
**	12/22/81 (ps)	modified for two table version.
**	4/4/82 (ps)	added call to s_ren_set.
**	10/26/82 (ps)	add check for "f" parameter.
**	05/08/86 (jhw)  Note: 's_ren_set()' allocates name and report type,
**			so don't do it here.  Also, made VOID.
**	24-mar-1987 (yamamoto)
**		Changed CH macros to CM.
**	4-jan-1989 (danielt)
**		added parameter to s_ren_set call.
**	06-feb-90 (sylviap)
**		Checks for the length of the report name.  If too long, then
**		flags error.
**      11-oct-90 (sylviap)
**              Added new paramter to s_g_skip.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

VOID
s_nam_set()
{
	i4		type;			/* type of token for name */
	char		rname[MAXLINE+1];	/* buffer for report name */
	char		*rtype;			/* type of report */
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */

	type = s_g_skip(TRUE, &rtn_char);
	if ((type != TK_ALPHA) && (type != TK_NUMBER))
		s_error(0x389, FATAL, s_g_token(FALSE), NULL);	/* a bad name */
	STcopy(s_g_name(FALSE), rname);	/* get report name
					**	(BUG #10224:  save in buffer)
					*/
	if (STlength(rname) > FE_MAXNAME)		/* US #409 */
		s_error(0x389, FATAL, rname, NULL);	/* name too long */
	if (s_g_skip(TRUE, &rtn_char) == TK_ALPHA)
	{
		rtype = s_g_name(FALSE);
		CMtolower(rtype, rtype);
		rtype = (rtype[0] == 'f') ? ERx("f") : ERx("s");
		_VOID_ s_g_skip(TRUE, &rtn_char);
	}
	else
	{
		rtype = ERx("s");
	}

	s_ren_set(rname, rtype, -1);
}
