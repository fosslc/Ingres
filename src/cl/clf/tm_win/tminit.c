/*
** Copyright (c) 1987, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>

/**
**
**  Name: TMINT.C - Initialize time zone.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMinit	- initialize time zone.
**
**
** History:
 * Revision 1.1  88/08/05  13:46:51  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  16:03:56  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/*{
** Name: TMinit	- initialize time zone
**
** Description:
**	    Initialize the TM variable, timezone, for operating systems
**	that don't implement the concept.
**
**	Dummy routine for UNIX - always returns OK.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    OK			if the variable is properly initialized.
**	    TM_NOTIMEZONE	if the environment variable, II_TIMEZONE, is
**				not defined.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-Feb-86 (ericj) -- made a CL entry point and returns status.
**      15-sep-86 (seputis)
**          initial creation
**      06-jul-87 (mmm)
**          initial jupiter unix cl.
*/
STATUS
TMinit()
{
	return(OK);
}
