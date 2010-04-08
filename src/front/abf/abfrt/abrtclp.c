/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		 
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"rts.h"
#include	"keyparam.h"

/**
** Name:	abrtclp.c - ABF Run-time System Command-line Parameter Routines.
**
** Description:
**	Contains the method routine that retrieves application-specific
**	comand-line parameters.
**
**	IIARclpCmdLineParms()   Return application-specific comand-line parms.
**
** History:
**	Revision 6.4
**	03/13/91 (emerson)
**		Created for Topaz release.
**	03/29/91 (emerson)
**		Replaced GLOBALDEFs by a function to set them;
**		GLOBALDEFs were causing problems for VMS shared libraries
**		(iltop was trying to set one but it's in a different library).
*/

/*
** One of the following two statics must be set to a nonnull value
** before the first call to IIARclpCmdLineParms.
**
** They are set by calling iiARsptSetParmText; the two arguments passed in
** are placed into the two static variables.
**
** The first static should be left NULL or set to point to
** a DB_TEXT_STRING containing application-specific command-line parameters
** (as is done by IIARmain, for imaged applications).
**
** If the first static is left NULL, the second static
** should be set to point to a function returning a pointer to a DB_TEXT_STRING
** (as is done by IIOtop, for interpreted applications).
** This callback function will be called on the first call
** to IIARclpCmdLineParms.  It should return a pointer to
** a DB_TEXT_STRING containing application-specific command-line parameters.
*/
static DB_TEXT_STRING *iiARptsParmTextString = (DB_TEXT_STRING *) NULL;
static DB_TEXT_STRING *(*iiARptfParmTextFunc)();

/*{
** Name:	iiARsptSetParmText() - Set static values for IIARclpCmdLineParms
**
** Description:
**	Sets static variables needed by IIARclpCmdLineParms.
**	Must be called before first call to IIARclpCmdLineParms.
**	See discussion of the static variables iiARptsParmTextString
**	and iiARptfParmTextFunc above.
**
** Inputs:
**	string	{DB_TEXT_STRING *}	The value for iiARptsParmTextString.
**	func	{DB_TEXT_STRING *(*)()}	The value for iiARptfParmTextFunc.
**
** History:
**	03/29/91 (emerson)
**		Created for Topaz release.
**	26-may-92 (leighb) DeskTop Porting Change:
**		If the Text String has already been loaded, then
**		don't overwrite it with a subsequent NULL.
*/
VOID
iiARsptSetParmText ( string, func )
DB_TEXT_STRING	*string;
DB_TEXT_STRING	*(*func)();
{
	if ((iiARptsParmTextString != (DB_TEXT_STRING *)NULL)	 
	 && (string == NULL))					 
		return;						 
	iiARptsParmTextString = string;
	iiARptfParmTextFunc   = func;
	return;
}

/*{
** Name:	IIARclpCmdLineParms() - Return application-specific comand-line
**					parameters.
** Description:
**	Returns application-specific comand-line parameters.
**	If an imaged application is being run, these are all parameters
**	after the parameter -a or -A; the value returned is the string 
**	formed by concatenating them, with a single space between each parm.
**	If an interpreted application is being run ("GO"), 
**	then on the first call to this function, the user is prompted
**	for application-specific comand-line parameters;
**	the value returned on the first call and all subsequent calls
**	is the string entered by the user on the first call.
**
** Inputs:
**	prm	{ABRTSPRM *}	The parameter structure for the call.
**	name	{char *}	Procedure name
**	proc	{ABRTSPRO *}	The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}	The pr_retvalue member of this structure
**				points to a DB_DATA_VALUE into which the 
**				result ("the value returned") will be placed.
** History:
**	03/13/91 (emerson)
**		Created for Topaz release.
*/
PTR
IIARclpCmdLineParms ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	DB_DATA_VALUE	retvalue;

	if ( iiARptsParmTextString == (DB_TEXT_STRING *)NULL )
	{
		iiARptsParmTextString = (*iiARptfParmTextFunc)();
	}
	retvalue.db_data = (PTR)iiARptsParmTextString;
	retvalue.db_datatype = DB_VCH_TYPE;
	retvalue.db_length = iiARptsParmTextString->db_t_count +
		sizeof(DB_TEXT_STRING) -
		sizeof(iiARptsParmTextString->db_t_text);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}
