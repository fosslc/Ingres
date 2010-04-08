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
# include	<cm.h> 
# include	<stype.h> 
# include	<sglob.h> 

/*{
** Name:	s_include() -	Include file for report specification
**
** Description:
**   S_INCLUDE - include file for a new report specification.  This is called
**	when the .INCLUDE command is encountered in the input.
**	The format of the .INCLUDE command is:
**		.INCLUDE filename
**	with "filename" being the include file,
**	specified for RBF reports.
**
** Parameters:
**	none.
**
** Returns:
**	None.
**
** Called by:
**	s_process().
**
** Side Effects:
**	none.
**
** Trace Flags:
**
** Error Messages:
**	905.
**
** History:
**	8/21/89 (elein) garnet
**		Created
**      11-oct-90 (sylviap)
**              Added new paramter to s_g_skip.
**	8/1/91 (elein) b38885
**			Allow quoted strings for file names to escape special chars,
**			But strip the quotes from the string if necessary.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

VOID
s_include()
{
	char     inc_file[MAXLINE+1]; /* buffer for file name */
	char		*name;	/* ptr to file name */
	i4             rtn_char;               /* dummy variable for sgskip */
	i4             length;               /* dummy variable for sgskip */
	i4             token;               /* dummy variable for sgskip */

	/* start of routine */



   switch(token = s_g_skip(TRUE, &rtn_char))
   {
      case(TK_QUOTE):
      case(TK_SQUOTE):
         name = s_g_string(FALSE, token );
			CMnext(name);
			STcopy( name, inc_file);
			length = STlength(inc_file) - 1;
			inc_file[ length ] = EOS;
         break;

      default:
         name = s_g_token(FALSE);
			STcopy( name, inc_file);
         break;
   }

	s_readin(inc_file);
	return;

}
