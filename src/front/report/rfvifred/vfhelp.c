/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"decls.h"
#include	<help.h>

/*{
** Name:	vfhelp() -	forms help utility from VIFRED.
**
** Description:
**	This is the front end to the forms help utility for RTI
**	front ends VIFRED and RBF in the following situation.  When
**	the layout form is displayed but an EQUEL SUBMENU is being
**	used, the menu structure comes from the forms system, but
**	the form does not.  If the layout menu is being used, and
**	the detailed Menu structures are used, a call to vfhelpsub
**	should be used for help.  If EQUEL forms and EQUEL menus
**	(or submenus) are being used, a call to FEhelp should be made.
**
** Inputs:
**	name		- name of file for HELP/FRAME.
**	subject		- subject name for title of help frame.
**
** History:
**	05-oct-1985 (dkh)	Fixed to work correctly for vifred/rbf.
**	13-jan-1986 (peter)	Changed third parameter to several help calls.
**	30-jan-1986 (peter)	Change to remove param from fedohelp.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
vfhelp(name, subject)
char	*name;
char	*subject;
{
	if (!FEfehelp(name, subject, (i4)H_VFRBF))
	{
		FTclear();
		VTxydraw(frame, globy, globx);
	}
}


/*{
** Name:	vfhelpsub() -	forms help utility for VIFRED.
**
** Description:
**	This is the front end to the forms help utility for RTI
**	front ends VIFRED and RBF because they use a dif menu struct.
**	Note that if an EQUEL submenu is in use even in VIFRED and
**	RBF (as in the Create submenu) a call to vfhelp should be
**	used.
**
**	It puts the Help File directory in front of the file name
**	and then simply calls FEdohelp for the rest.
**
** Inputs:
**	name		- name of file for HELP/FRAME.
**	subject		- subject name for title of help frame.
**	menu		- menu structure already in place.
**
** History:
**	01-oct-1985 (peter)	written.
**	13-apr-88 (bruceb/sylviap)
**		Call FEhlpnam() to get full pathname for help file.
*/
VOID
vfhelpsub(name, subject, menu)
char	*name;
char	*subject;
MENU	menu;
{
	char		nam_buf[MAX_LOC+1];

	/* Add the Help File directory path to name */
	if ( IIUGhlpname(name, nam_buf) == OK )
	{
		if ( !IIRTdohelp(nam_buf, subject, menu, TRUE,
				H_VFRBF, (VOID (*)())NULL) )
		{
			FTclear();
		}
	}
}
