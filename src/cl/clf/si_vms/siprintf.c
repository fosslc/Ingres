/*
** Copyright (c) 1982, 2000 Ingres Corporation
*/

# include	<compat.h>
# include       <gl.h>
# include	<si.h>
# include	<st.h>
# include	<stdarg.h>

/**
** Name:	siprintf.c -	Formatted Print Compatibility Module.
**
** Description:
**	This file contains the definitions of the routines used to output
**	formatted strings through the SI module.  ('SIdofrmt()' is used by
**	the string formatting routines of ST ('STprintf()' and 'STzprintf()',
**	as well.)
**
**	SIprintf()	formatted print to standard output
**	SIfprintf()	formatted print to stream file
**
** History:	
**		Revision 50.0  86/06/18  09:47:04  wong
**		Added support for '*' precision and widths.
**
**		03/09/83 -- (mmm)
**			VMS code brought to UNIX and modified--
**			CV lib calls used, still uses 'fcvt()' and 'ecvt()'
**			assembly routines found in "CVdecimal.s".
**	11/03/92 (dkh) - Changed to use the ansi c stadrag mechanism for dealing
**			 with variable number of arguments to a routine.
**      16-jul-93 (ed)
**	    added gl.h
**	19-may-98 (kinte01)
**	    Cross-integrate Unix change 433538 and remove ifdef Alpha as
**	    this file is only in the AlphaVMS CL
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	15-Jun-2000 (kinte01)
**	    Update varags usage to correct compiler warnings in Compaq C 6.2
*/


FUNC_EXTERN	VOID	SIdofrmt();
FUNC_EXTERN	STATUS	_flsbuf();

/*VARARGS1*/

VOID
SIprintf(const char *fmt, ...)

{
	va_list	ap2;

	va_start(ap2, fmt);
	SIdofrmt(stdout, fmt, _flsbuf, ap2);
	va_end(ap2);
}

/* VARARGS2 */

VOID
SIfprintf(FILE *ioptr, const char *fmt, ...)
{
	va_list	ap2;

	va_start(ap2, fmt);
	SIdofrmt(ioptr, fmt, _flsbuf, ap2);
	va_end(ap2);
}

