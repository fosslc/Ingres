/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uigdata.h>
#include	<erui.h>

/**
** Name:	uiroleid.c -	Check Role Flag Routine.
**
** Description:
**	Contains the routine used to check for the role flag and to prompt
**	for the password if one was not specified.  Defines:
**
**	IIUIroleID()	check role flag.
**
** History:
**	Revision 6.4  89/12  wong
**	Initial revision.
**/


/*{
** Name:	IIUIroleID() -	Check DBMS Role Flag.
**
** Description:
**	Checks whether the input flag argument specifies the DBMS role flag and
**	identifier and that the password was not specified as part of it.  Then
**	prompts for a password if it was not specified on as part of the flag.
**
** Input:
**	flag	{char *}  The flag to be checked.
**
** Returns:
**	{char *}  The role ID flag including the password,
**		  or NULL if the flag is not recognized as the DBMS role flag.
**
** History:
**	12/89 (jhw) -- Written.
**	30-aug-1990 (Joe)
**	    Changed IIUIgdata to the IIUIfedata function.
**      4-apr-91 (blaise)
**          Users are no longer allowed to use the "role/password" syntax with
**          the -R flag (other than in the terminal monitor); they will always
**          be prompted for a password.
**          Don't return NULL if flag contains a slash; this was causing access
**          violation. Instead don't let the user know that slash is special;
**          prompt for a password for the whole parameter (including the /),
**          append the password and pass the whole thing to the dbms. One
**          precaution has been added: if the flag contains a slash we append a
**          "/" and the password entered anyway, even if the user didn't enter
**          a password when prompted. This is to make sure the dbms gives an
**          error by passing it "-Rxxx/yyy/" rather than "-Rxxx/yyy".
**          Bug #36540.
**	5-aug-91 (blaise)
**	    Backed out the above change. IIUIroleID returns NULL if the flag
**	    contains a slash; error handling is done by calling functions.
*/
char *
IIUIroleID ( flag )
char	*flag;
{
	char	*cp;

	if ( flag == NULL || *flag != '-' || *(flag+1) != 'R'
			|| (cp = STindex(flag, "/", 0)) != NULL )
	{
		return NULL;	/* not DBMS role flag or
				** password already specified as part of flag
				*/
	}
	else
	{ /* prompt for password */
		char	buf[2*FE_MAXNAME + 3 + 1];	/* "-R<role>/<pass>" */

		STcopy(flag, buf);	/* copy flag and role ID */

		/* Prompt for password into buffer (after role ID) */
		cp = buf + STlength(buf) + 1;
		(*IIUIfedata()->prompt_func)( ERget(S_UI0013_RolePasswordPrompt),
				1, TRUE, cp, sizeof(buf) - 1 - (cp - buf),
					1, (PTR)(buf + 2)
		);
		if ( *cp != EOS )
		{ /* allocate new role flag including appended password */
			*--cp = '/';
			flag = STalloc(buf);
		}
		return flag;
			
	}
	/*NOT REACHED*/
}
