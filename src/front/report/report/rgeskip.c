/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_G_ESKIP - returns type of next token by calling r_s_skip.
**		If we are at the end of the rtext string, get the next
**		rtext action string if the next command is the same
**		as the current one.
**
**	Parameters:
**		none.
**
**	Returns:
**		type of next token.
**
**	Called by:
**		r_p_text, r_p_format, r_p_pos, r_p_width, r_p_within.
**
**	Side Effects:
**		May set Cact_rtext.  May advance current action (r_gch_act).
**
**	Trace Flags:
**		none.
**
**	History:
**		12/5/83 (gac)	written to allow unlimited-length parameters.
**		12/1/84 (rlm)	modified action retrieval for ACT reduction
**		9/5/89 (elein)
**			Added check for St_cache flag. Some commands cannot
**			do lookahead, but still come thru here because of
**			new expression handling.  St_cache flag is usually
**			true, but set to false in ractset.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_g_eskip()
{
	ACT	*next_act,*r_gch_act(),*r_ch_look();
	i4	type;

	type = r_g_skip();
	if (type == TK_ENDSTRING)
	{
		next_act = r_ch_look ();
		if (next_act != NULL && St_cache == TRUE &&
		    STequal(next_act->act_section, Cact_type) &&
		    STequal(next_act->act_attid, Cact_attribute) &&
		    STequal(next_act->act_command, Cact_command) )
		{
			/* actually update action - must reset pointers! */
			if ((next_act = r_gch_act ((i2) 0, (i4) 1)) == NULL)
				r_syserr (E_RW0028_r_g_eskip_Pointer_err);
			Cact_rtext = next_act->act_text;
			Cact_attribute = next_act->act_attid;
			Cact_type = next_act->act_section;
			Cact_command = next_act->act_command;
			r_g_set(Cact_rtext);
			type = r_g_skip();
		}
	}

	return(type);
}
