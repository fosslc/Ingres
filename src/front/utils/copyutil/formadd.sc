/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<cf.h>

/**
** Name:	formadd.sc	- Post add function for forms.
**
** Description:
**	This file contains the post add function for forms.
**
**	IICUpfaPostFormAdd
**
** History:
**	Revision 6.0  87/09/16  joe
**	Initial Version.
**	01-sep-88 (kenl)
**		Removed SQL end and begin transaction statements.
**		Added call ti IIUI...Xaction() routines.
**	11/09/88 (dkh) - Fixed venus bug 3851.
**	12/30/89 (dkh) - Removed declarations of Cfversion and Cf_silent
**			 since they are now in copyform!cfglobs.c.
**      21-jan-98 (rodjo04)
**              Bug 83370: Added static boolean variable Quit_flag. Added
**              function SetQuit_flag() so that outside functions can set
**              Quit_flag. Also added check in IICUpfaPostFormAdd(). 
**              If Quit_flag is TRUE, then do not force commit.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
static bool Quit_flag = FALSE;

GLOBALREF bool Cf_silent;

STATUS
IICUpfaPostFormAdd (class, level, name, ids)
OOID	class;
i4	level;
char	*name;
IDSTACK	*ids;
{
	STATUS		rval;
	FORMINFO	forms[1];


	forms[0].name = name;
	forms[0].id = ids->idstk[0];
	Cf_silent = TRUE;
	/*
	** The addition of the form in copy util uses a transcation.
	** end it here and let cfcompile start a new one, then
	** start a new one for copyutil.
	*/

	if (FEinMST())
	{
	    /* end transaction */
	    IIUIendXaction();
	}
        cfcompile(forms, 1);

	/*
	**  Need to commit changes made to forms
	**  system catalogs since compform only
	**  commits works when it is running
	**  standalone.  This was necessary to make
	**  fecvt60 run correctly.
	*/
       if (!Quit_flag)
       	   exec sql commit work;

	/* begin transaction */
	IIUIbeginXaction();

	return OK;
}

VOID SetQuit_flag(qflag_stat)
bool qflag_stat; 
{
   Quit_flag = qflag_stat;
}

 
