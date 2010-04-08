/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:	uffldhlp.qc -	Help on Current Field or Column Routine.
**
** Description:
**	Contains the routine that provides help on the current field or
**	column on the form.  Defines:
**
**	IIUFfieldHlep()	provide help on the current field or column.
**
** History:
**	Revision 6.4  89/11  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIUFfieldHelp() -	Provide Help on the Current Field
**						or Column.
** Description:
**	Calls the (FRS) RunTime help facility for fields, which either provides
**	a selection list or displays information on the field or column.  The
**	current form, field and column are determined and passed to the help
**	facility.
**
** History:
**	11/89 (jhw) -- Written.
*/
VOID
IIUFfieldHelp()
##{
##	i4	istable;
##	char	formname[FE_MAXNAME+1];
##	char	fldname[FE_MAXNAME+1];
##	char	colname[FE_MAXNAME+1];

##	INQUIRE_FRS FORM ( formname = NAME, fldname = FIELD );

	if ( fldname[0] == EOS || formname[0] == EOS )
		return;

##	INQUIRE_FRS FIELD formname ( istable = TABLE(fldname) );
	if ( istable != 1 )
	{
		colname[0] = EOS;
	}
	else
	{ /* table field */
##		INQUIRE_FRS TABLE formname ( colname = COLUMN(fldname) );
	}

	FEhvals(formname, fldname, colname);
##}
