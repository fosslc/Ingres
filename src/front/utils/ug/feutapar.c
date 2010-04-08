/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    feutapar.c -	Front-End Command Parsing Routine.
**
** Description:
**	Contains a routine used to parse all the command-line arguments for a
**	program given descriptors of all the arguments.  This routine is part
**	of the Front-End Command Parsing Module.  Defines:
**	
**	IIUGuapParse()	parse and return parameters using parameter descriptor.
**
** History:
**	Revision 6.0  88/01/08  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name:    IIUGuapParse() -	Parse and Return Parameters Using Descriptor.
**
** Description:
**	Parse a command-line for a program according to the entry for the
**	program in the "utexe.def" file.  Then, get the parameter values
**	as specified by the input parameter descriptor array.
**
** Sample Use (somewhat abbreviated):
**
** FUNC_EXTERN     char    *IIUIpassword();
**
** static  char    *dbname = NULL;         ** database name **
** static  char    *user = ERx("");        ** -uusername flag **
** static  char    *xpipe = ERx("");       ** -X flag **
**
** ** Parameter Description **
**
** static ARG_DESC
**        args[] = {
**        ** Required **
**          {ERx("database"),   DB_CHR_TYPE,    FARG_PROMPT,    (PTR)&dbname},
**        ** Optional **
**          {ERx("user"),       DB_CHR_TYPE,    FARG_FAIL,      (PTR)&user},
**        ** Internal optional **
**	    {ERx("equel"),      DB_CHR_TYPE,    FARG_FAIL,      (PTR)&xpipe},
**	    NULL
** };
**
** STATUS
** program_name ( argc, argv )
** i4      argc;
** char    **argv;
** {
**      char    *password = ERx("");    ** -P flag to pass to FEningres() **
**      ARGRET  one_arg;                ** info on a single argument **
**      i4      arg_pos;                ** position of single arg in "args[]"**
**
**      if (IIUGuapParse(argc, argv, ERx("program_name"), args) != OK)
**                return FAIL;
**
**	** example of how to use IIUGuapParse with FEutaget: **
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
**	argc	{nat}  The number of command-line arguments.
**	argv	{char **}  The array of command-line arguments.
**	program	{char *}  The program name.
**	params	{ARG_DESC []}  The parameter descriptor array.
**
** Returns:
**	{STATUS}  OK, if there were no errors.
**		  FAIL, if there were errors including when a parameter was
**			required and prompted for but not entered.
**
** History:
**	01/88 (jhw) -- Written.
**	29-aug-90 (pete)
**              Added "Sample Call:" documentation above to make it easier
**              to figure out how to use this dude!
*/

STATUS
IIUGuapParse (argc, argv, program, params)
i4		argc;
char		**argv;
char		*program;
ARG_DESC	params[];
{
    register ARG_DESC	*p;
    ARGRET	rarg;		/* used in returning cmd line args */
    i4		pos;		/* position of flag on cmd line */

    if (FEutaopen(argc, argv, program) != OK)
	return FAIL;

    for (p = params ; p->_name != NULL ; ++p)
    {
	if (FEutaget(p->_name, 0, p->_fail_mode, &rarg, &pos) != OK)
	{
	    if (p->_fail_mode == FARG_PROMPT)
	    {
		FEutaclose();
		return FAIL;
	    }
	}
	else
	{
	    if (p->_type == DB_CHR_TYPE)
	    {
		*(char **)p->_ref = rarg.dat.name;
		if (p->_fail_mode == FARG_PROMPT &&
			(rarg.dat.name == NULL || *rarg.dat.name == EOS) )
		{
		    FEutaclose();
		    return FAIL;
		}
	    }
	    else if (p->_type == DB_INT_TYPE)
		*(i4 *)p->_ref = rarg.dat.num;
	    else if (p->_type == DB_FLT_TYPE)
		*(f8 *)p->_ref = rarg.dat.val;
	    else if (p->_type == DB_BOO_TYPE)
		*(bool *)p->_ref = TRUE;
	}
    }
    FEutaclose();

    return OK;
}
