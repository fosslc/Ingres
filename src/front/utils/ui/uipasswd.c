/*
**	Copyright (c) 2009 Ingres Corporation
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
** Name:	uipasswd.c -	Check For User Password Flag Routine.
**
** Description:
**	Contains the routine used to check for the user password flag and to
**	prompt for a password if required.  Defines:
**
**	IIUIpassword()	check for user password flag.
**
** History:
**	Revision 6.4  89/12  wong
**	Initial revision.
**	22-may-1995 (wolf)
**	    IIUIrole() was added to fix bug 53689, but the pre-existing
**	    IIUIroleID() will suffice.  So remove IIUIrole().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIUIpassword() -	Check for User DBMS Password flag.
**
** Description:
**	Checks whether the input flag argument specifies the DBMS password
**	flag and that it does not specify the password.  It then prompts for
**	a password if it was not specified on as part of the flag.
**
** Sample Use:
**
**	FUNC_EXTERN     char    *IIUIpassword();
**
**      char    *password = ERx("");    ** -P flag to pass to FEningres() **
**      ARGRET  one_arg;                ** info on a single argument **
**      i4      arg_pos;                ** position of single arg in "args[]"**
**
**      ** check if -P specified **
**      if (FEutaopen(argc, argv, ERx("program_name")) != OK)
**          PCexit(FAIL);
**
**      if (FEutaget (ERx("password"), 0, FARG_FAIL, &one_arg, &arg_pos) == OK)
**      {
**          ** User specified -P. IIUIpassword prompts for password
**          ** value, if user didn't specify it. Note: If user specified
**          ** a password attached to -P (e.g. -Pfoobar, which isn't legal),
**          ** then FEutaget will issue an error message and exit (so this
**          ** block won't be run).
**          **
**          password = IIUIpassword(ERx("-P"));
**          password = (password != (char *) NULL) ? password : ERx("");
**      }
**
**      FEutaclose();
**
**      if ( FEingres(dbname, xpipe, user, password, (char *)NULL) != OK )
**      {
**      }
**
** Input:
**	flag	{char *}  The flag to be checked.
**
** Returns:
**	{char *}  The user password flag including the password,
**		  or NULL if the flag is not recognized as the user DBMS
**		  password flag.
**
** History:
**	12/89 (jhw) -- Written.
**	29-aug-90 (pete)
**		Added "Sample Call:" documentation above to make it easier
**		to figure out how to use this dude!
**	30-aug-1990 (Joe)
**	    Changed IIUIgdata reference to call of IIUIfedata.
**	14-Jan-2009 (kiria01) b120451
**	    Applied fuller prototypes in ANSI form
*/
char *
IIUIpassword (char *flag)
{
	if ( flag == NULL || *flag != '-' || *(flag+1) != 'P'
			|| *(flag+2) != EOS )
	{
		return NULL;	/* not user DBMS password flag or
				** password already specified with flag.
				*/
	}
	else
	{ /* prompt for password */
		char	buf[FE_MAXNAME+2+1];	/* "-P<password>" */

		(*IIUIfedata()->prompt_func)( ERget(S_UI0012_PasswordPrompt),
				1, (bool)TRUE, buf + 2, FE_MAXNAME, 0
		);
		if ( buf[2] != EOS )
		{ /* allocate new user DBMS password flag including password */
			buf[0] = '-';
			buf[1] = 'P';
			flag = STalloc(buf);
		}
		return flag;
	}
	/*NOT REACHED*/
}
