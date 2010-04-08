/*
** Copyright (c) 2004 Ingres Corporation
*/


/* static char	Sccsid[] = "@(#)scat.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h> 
# include	<sglob.h> 

/*
**	S_CAT  --  Concatenate given string to Ctext string.
**		If the combined length of the strings exceeds the
**		maximum length, write out Ctext string and copy
**		the given string to it.
**
**	
**	Parameters:
**		string	string to concatenate to Ctext.
**
**	Returns:
**		none.
**
**	Trace Flags:
		none.
**
**	History:
**		12/1/83 (gac)	written.
**		1/22/85	(gac)	fixed bug #4907 -- RBF trim >98 chars
**				no longer gets syserr when Filed, 
**				then SREPORTed.
**		2/20/90 (martym)
**			Casting all call(s) to s_w_row() to VOID. Since it
**			returns a STATUS now and we're not interested here.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Fix-up external declarations -
**			they've been moved to hdr files.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
s_cat(string)
char	*string;
{
 	if (STlength(Ctext)+STlength(string) > MAXRTEXT)
	{
		_VOID_ s_w_row();		/* write out old one */
		s_set_cmd(Ccommand);	/* set a new one */
 		if (STlength(string) > MAXRTEXT)
		{
 			s_error(0x38B, NONFATAL, string, Ccommand, NULL);
			return;
		}
	}

 	STcat(Ctext, string);
}
