/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_M_ACTION - put a command and some number of string parameters on line
**	and convert into TCMD structures.
**
**	Parameters:
**		command			string containing command.
**		param1,param2...	strings containing pieces of parameters
**					ended with a zero.
**					MAXIMUM OF 9 PARAMETERS (INCLUDING 0) !!
**
**	Returns:
**		none.
**
**	Error Messages:
**		Syserr: Bad command.
**
**	Trace Flags:
**		50, 54.
**
**	History:
**		2/20/82 (ps)	written.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	06-Apr-2005 (lakvi01)
**		Added the function return type so that it matches the declaration.
*/



/* VARARGS1 */
VOID
r_m_action(command,a1,a2,a3,a4,a5,a6,a7,a8,a9)
char	*command;
char	*a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	/* start of routine */


	if (command == NULL)
	{
		r_syserr(E_RW002F_r_m_action_Null_cmd);
	}


	*Cact_rtext = '\0';

	if (a1 != NULL)
	{
	   STcat(Cact_rtext, a1);
	   if (a2 != NULL)
	   {
	      STcat(Cact_rtext, a2);
	      if (a3 != NULL)
	      {
		 STcat(Cact_rtext, a3);
		 if (a4 != NULL)
		 {
		    STcat(Cact_rtext, a4);
		    if (a5 != NULL)
		    {
		       STcat(Cact_rtext, a5);
		       if (a6 != NULL)
		       {
			  STcat(Cact_rtext, a6);
			  if (a7 != NULL)
			  {
			     STcat(Cact_rtext, a7);
			     if (a8 != NULL)
			     {
				STcat(Cact_rtext, a8);
				if (a9 != NULL)
				{
				   r_syserr(E_RW0030_r_m_action_Too_many);
				}
			     }
			  }
		       }
		    }
		 }
	      }
	   }
	}

	STcopy(command, Cact_command);


	r_nxt_set();

	return;
}
