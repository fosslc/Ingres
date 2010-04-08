/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ft.h>
#include	<adf.h>
#include	<fmt.h>
#include	<frame.h>
#include	<menu.h>
#include	<help.h>

/**
** Name:	iifhelp.c	- EQUEL/SQL Help File Statements Support.
**
** Description:
**	Defines the routines for the HELPFILE and HELP_FRS statements.
**	This file defines:
**
**	IIhelpfile	- EQUEL HELPFILE command.
**	IIfrshelp	- EQUEL 'help_frs' command.
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

bool	IIRTfrshelp();

/*{
** Name:	IIhelpfile() -	cover for HELPFILE routine.
**
** Description:
**	This routine is the cover for the 'HELPFILE subject filename'
**	command in EQUEL, which corresponds to the HELP/FRAME
**	combination in the RTI forms help facility.  It takes a file,
**	and displays it in a table field, and provides menu selections
**	for scanning through that file.  If it cannot find the file,
**	it will print an appropriate error message.
**	
** Inputs:
**	filename	- name of file containing help text.
**	subject		- subject matter for title.
**
** Returns:
**	{bool}	TRUE if successful.
**
** History:
**	16-feb-1983 (john) - Extract from the original runtime.c 
**	21-sep-1984 (dave) - Now calls FThelp to do the work.
**	01-oct-1985 (peter) - Changed to call FEhframe.
**	03-dec-1985 (neil) - Make sure strings are trimmed.
*/
# define	SUBWIDTH	70
bool
IIhelpfile(subj, flnm)
char	*subj;				/* subject of message */
char	*flnm;				/* file to retrieve message from */
{
	char	filename[MAX_LOC+1];	/* filename buffer */
	char	subject[SUBWIDTH+1];	/* subject buffer */

	if (flnm == NULL || subj == NULL)
		return FALSE;

	_VOID_ STlcopy(flnm, filename, sizeof(filename)-1);
	_VOID_ STtrmwhite(filename);
	_VOID_ STlcopy(subj, subject, sizeof(subject)-1);
	_VOID_ STtrmwhite(subject);
	return IIRTfrshelp(filename, subject, H_FE, (VOID (*)())NULL);
}

/*{
** Name:	IIfrshelp() -	cover for HELP_FRS routine'.
**
** Description:
**	This routine is the cover for the 'HELP_FRS (subject = subj,
**	file = filename)' command in EQUEL, which corresponds to the 
**	full set of HELP in the RTI forms help facility.  It takes a file,
**	and displays it in a table field, and provides menu selections
**	for scanning through that file.  If it cannot find the file,
**	it will print an appropriate error message.  It automatically
**	provides help for KEYS and VALUES.
**
**	This routine will be extended in the future to allow more
**	selective use of the RTI help facility through the 'type'
**	parameter.
**	
** Inputs:
**	type		- type of the help to do.  Currently, only
**			  value of 0 is allowed.
**	filename	- name of file containing help text.
**	subject		- subject matter for title.
**
** Returns:
**	{bool}	TRUE if successful.
**
** History:
**	01-oct-1985 (peter) - Changed to call FEhframe.
**	03-dec-1985 (neil) - Make sure strings are trimmed.
**	13-jan-1986 (peter) - Change param on call to FEfrshelp.
*/
bool
IIfrshelp(type, subj, flnm)
i4	type;
char	*subj;				/* subject of message */
char	*flnm;				/* file to retrieve message from */
{
	char	filename[MAX_LOC+1];	/* filename buffer */
	char	subject[SUBWIDTH+1];	/* subject buffer */

	if (flnm == NULL || subj == NULL)
		return FALSE;

	_VOID_ STlcopy(flnm, filename, sizeof(filename)-1);
	_VOID_ STtrmwhite(filename);
	_VOID_ STlcopy(subj, subject, sizeof(subject)-1);
	_VOID_ STtrmwhite(subject);
	return IIRTfrshelp(filename, subject, H_EQ, (VOID (*)())NULL);
}
