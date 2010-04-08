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
#include	<ug.h>
#include	<ft.h>
#include	<adf.h>
#include	<fmt.h>
#include	<frame.h>
#include	<menu.h>
#include	<help.h>

/**
** Name:	fehelp.c -	Front End Help Utility.
**
** Description:
**	Contains the Front-End interface to the help utility.  Defines:
**
**	FEhelp()	basic forms help utility.
**	FEfehelp()	special forms help utility.
**				(called by VIFRED/RBF and IQUEL.)
**
**	The parameter name 'vfrfiq' which shows up in many
**	of these routines is a switch with the following
**	values (defined in help.h):
**
**		H_EQ - standard EQUEL forms user program.
**		H_FE - standard INGRES front-end
**		H_VFRBF - called from RBF or VIFRED.
**		H_IQUEL - called from IQUEL or ISQL output phase.
**		H_VIG	- called from graphics part of VIGRAPH.
**		H_QBFSRT - called from QBF sort (this suppresses
**			   field help, but then acts like H_FE).
**
**	This is used because each uses a slightly different
**	set of keys, interaction with menus, etc.
**
**	If these routines are called from EQUEL, with a value of
**	H_EQ, the help processing routines will assure that the
**	previous screen is redrawn etc upon return.  IF any other
**	value is given, the caller must check the return status
**	from FEfehelp to see whether or not a redraw of the screen
**	is required, and then perform the action if needed.
**
** History:
**	Revision 6.2  88/12  wong
**	Moved 'FEhelp()' and 'FEfehelp()' here to the UF directory, and
**	changed the names of the called routines 'FEhlpnam()' and 'FEfrshelp()'
**	to 'IIUGhlpname()' and 'IIRTfrshelp()'.
**
**	Revision 6.4 11/89  swilson
**	Changed standard vfrfiq for INGRES front-ends to be H_FE in order
**	to allow bringing up "WhatToDo" screen immediately.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
*/

bool	IIRTfrshelp();

/*{
** Name:	FEhelp() -	Forms Help Utility.
**
** Description:
**	This is the front end to the forms help utility for RTI
**	front ends.  This is called directly by RTI front ends,
**	running straight EQUEL forms calls.
**
**	This front end calls FEfehelp to continue the processing.
**
** Inputs:
**	name		- name of file for HELP/FRAME.
**	subject		- subject name for title of help frame.
**
** History:
**	05-oct-1985 (dkh)	Fixed to work correctly for vifred/rbf.
**      03-nov-1989 (swilson)   Changed to use H_FE to indicate front-end
*/
VOID
FEhelp ( char *name, char *subject )
{
	_VOID_ FEfehelp(name, subject, H_FE);
}


/*{
** Name:	FEfehelp() -	Forms Help Utility.
**
** Description:
**	This is the front end to the forms help utility for RTI
**	front ends which want to use one of the specil entries
**	to help.  This is called  by FEhelp with a value for H_EQ,
**	but if other entries are needed, they can call here.
**
**	This first checks to see if the II_HELPDIR name is set, in
**	which case, that directory is used for finding the help
**	files.  If unset, it uses the ING_FILES directory, with
**	the subdirectory defined by the II_LANGUAGE parameter.
**	The default for II_LANGUAGE is 'english'.
**
** Inputs:
**	name		- name of file for HELP/FRAME.
**	subject		- subject name for title of help frame.
**	vfrfiq		- value as described above.
**
** Returns:
**	{bool}	TRUE if the caller does not need to redraw the screen
**		     upon return, as no form changes were made.
**		FALSE if a redraw is needed.
**
** History:
**	14-nov-1983 (nadene)	first written.
**	01-aug-1985 (grant)	modified to have submenu.
**	01-oct-1985 (peter)	added help for fields.
**	05-oct-1985 (dkh)	Fixed to work correctly for vifred/rbf.
**	13-jan-1986 (peter)	Changed vfrf parameter to three valued.
**	30-jan-1986 (peter)	Changes after code review.
**	08-aug-1987 (peter)	Changed to support II_HELPDIR
**				and multiple language.
**	12/88 (jhw) - Added optional field help routine for 'IIRTfrshelp()'.
*/
bool
FEfehelp ( char *name, char *subject, i4 vfrfiq )
{
#ifndef	PCINGRES
	char		nam_buf[MAX_LOC+1];

	if ( IIUGhlpname(name, nam_buf) == OK )
	{
		return IIRTfrshelp(nam_buf, subject, vfrfiq, (VOID (*)())NULL);
	}
	return FALSE;
#else
	return TRUE;
#endif	/* PCINGRES */
}
