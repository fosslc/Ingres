/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>

/**
** Name:    ildata.c -	ABF Run-Time System to INTERP globaldef interface.
**
** Description:
**	These are declarations of the global variables used
**	in the runtime system used to pass data to the interpreter
**	(INTERP).
**
** History:
**
**       6-oct-93 (donc)
**		Added IIARgarGetArRtsprm() to allow access to globaldef
**		IIarRtspr	 
**	17-nov-93 (donc)
**		Added IIARcarClrArRtsprm() and IIARsarSetArRtsprm.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
*/

/* 
**  Pass ABRTSPRM to iiinterp
 */
GLOBALREF   PTR    IIarRtsprm ; 

/*{
** Name:	IIARgarGetArRtsprm - Return handle to IIarRtsprm.
**
** Description:
**	This routine simply returns the address of IIarRtsprm so users
**	outside of the abfrt directory can access the variable without
**	the need to export the variable across the shared library
**	interface for VMS and UNIX.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		Address of IIarRtsprm.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/6/93 (donc) - Initial version.
*/
PTR
IIARgarGetArRtsprm()
{
	return(IIarRtsprm);
}

/*{
** Name:	IIARcarClrArRtsprm - Clear handle to IIarRtsprm.
**
** Description:
**	This routine simply sets the address of pointed to by IIarRtsprm 
**	to NULL.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		Nothing
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/17/93 (donc) - Initial version.
*/
VOID
IIARcarClrArRtsprm()
{
	IIarRtsprm = NULL;
}

/*{
** Name:	IIARsarSetArRtsprm - Set handle to IIarRtsprm.
**
** Description:
**	This routine simply sets the address of pointed to by IIarRtsprm 
**	to whatever pointer is passed in.
**
** Inputs:
**	ABRTSPRM *nrtsprm    Pointer to an ABRTSPRM.
**
** Outputs:
**
**	Returns:
**		Nothing
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/17/93 (donc) - Initial version.
*/
VOID
IIARsarSetArRtsprm( PTR nrtsprm )
{
	IIarRtsprm = nrtsprm;
}
