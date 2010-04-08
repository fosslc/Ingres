/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpflag.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include 	<rtype.h> 
# include 	<rglob.h> 
# include 	<st.h>
# include 	<cm.h>

/*
**   R_P_FLAG - set up a command which has no parameters.
**	There are no parameters to these commands.
**
**	Parameters:
**		code	code of command to use if no garbage.
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set, r_p_end.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		104.
**
**	Trace Flags:
**		100, 102.
**
**	History:
**		1/6/82 (ps)	written.
**		10-apr-90 (cmr)	If the action code is C_RBFAGGS pick out the
**				aggregate number and store it in Cact_tcmd.
**		4/26/90 (martym) Added code to extract the source of data 
**				information that may follow a P_RBFSETUP code.
**            11/5/90 (elein) 34229
**                    Ignore trailing words on BEGIN and BEGIN RBF*.
**	30-sep-1992 (rdrane)
**		Replace En_relation with reference to global
**		FE_RSLV_NAME En_ferslv.name.  Converted r_error()
**		err_num value to hex to facilitate lookup in errw.msg
**		file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_p_flag(code)
ACTION	code;
{
	i4	type, rc, src;
	char    *srcname;

	Cact_tcmd->tcmd_code = code;

	type = r_g_skip();
	if (code == P_RBFAGGS && type == TK_NUMBER)
	{
		/* pick out the agg # and store it in Cact_tcmd */
		if ((rc = r_g_long( &Cact_tcmd->tcmd_val.t_v_long)) < 0)
			r_error(0x67, NONFATAL, Cact_tname, Cact_attribute, 
				Cact_command, Cact_rtext, NULL);
		type = r_g_skip();
	}
	else
	if (code == P_RBFSETUP && type == TK_COMMA)
	{
		CMnext(Tokchar);
		if (r_g_long(&src) < 0)
		{
			r_error(0x67, NONFATAL, Cact_tname, Cact_attribute, 
				Cact_command, Cact_rtext, NULL);
		}
		type = r_g_skip();
		if (type != TK_COMMA)
		{
			r_error(0x68, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
		}
		CMnext(Tokchar);
		type = r_g_skip();
		if (type != TK_ALPHA)
		{
			r_error(0x68, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
		}
		srcname = r_g_name();
		if (En_program == PRG_RBF)
		{
			_VOID_ STcopy(srcname, En_ferslv.name);
			En_SrcTyp = src;
		}
		type = r_g_skip();
	}
		

	while (type != TK_ENDSTRING)
	{
		CMnext(Tokchar);
		type = r_g_skip();
	}
	return;
}
