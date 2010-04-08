/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<rtsdata.h>

/**
** Name:    rtsdata.c -	ABF Run-Time System Global Data Definitions File.
**
** Description:
**	These are declarations of the global variables used
**	in the runtime system.
**
**	The runtime system is used in both the definition stage and
**	the executable image.
**
** History:
**	12/19/89 (dkh) - VMS shared lib changes.  Added IIar_Dbname and
**			 IIar_Status so references to IIarDbname and IIarStatus
**			 can be vectorized.
**	10/14/90 (dkh) - Moved IIar_Dbname and IIar_Status to abf!abmain.c
**			 so they are not exported across shared library
**			 interface for both VMS and UNIX.  Access to
**			 IIarDbname and IIarStatus are sent back by
**			 calls to IIARgadGetArDbname() and IIARgasGetArStatus().
**	03/26/91 (emerson)
**		Added IIarBatch (for "non-forms-mode" support).
**	06-nov-92 (davel)
**		Added IIarNoDB (to support applications running without a 
**		DB connection).
**	12-nov-92 (davel)
**		Added IIARsadSetArDbname() to set/clear IIarDbname.
**       6-oct-93 (donc)
**		Added IIARgarArRtsprm() to allow access to globaldef
**		IIarRtspr	 
**	 7-oct-93 (donc)
**		Moved IIarRtsprm and IIARgarArRtsprm to their own
**		file ildata.c. 
**	29-oct-1993 (mgw) Bug #49551
**		Add abfrtlib Global, IIarUserApp, for determining whether
**		we're in a user application or not in iiarFormInit()
**		(and elsewhere if needed).
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	19-apr-1999 (hanch04)
**	    Added st.h
*/

/*
**	The names below (IIarBatch and IIarNoDB) are only usable by the abfrt 
**	directory.  No other directory requires access to them.
*/
GLOBALREF bool		IIarBatch ;
GLOBALREF bool		IIarNoDB  ;
GLOBALREF bool		IIarUserApp  ;

/*
**	The names below (IIarDbname and IIarStatus) are only usable by
**	the abfrt directory.  Access from outside the abfrt directory
**	(such as abf and abfimage) must be accomplished by handles
**	obtained by calling IIARgadGetArDbname() and IIargasGetArStatus().
*/

/*
** The database
*/
GLOBALREF char		*IIarDbname ;

/*
**  ABF runtime status
*/
GLOBALREF STATUS	IIarStatus ;	/* exit status */

/*
** Abrret and cAbrret were taken out to better support
** sharing of the abf runtime system.
** They have been moved to abfmain.
*/


/*{
** Name:	IIARgadGetArDbname - Return handle to IIarDbname.
**
** Description:
**	This routine simply returns the address of IIarDbname so users
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
**		Address of IIarDbname.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/14/90 (dkh) - Initial version.
*/
char **
IIARgadGetArDbname()
{
	return(&IIarDbname);
}

static char dbname_buffer[FE_MAXDBNM+1];

/*{
** Name:	IIARsadSetArDbname - Set IIarDbname.
**
** Description:
**	This routine copies the specified string into the static dbname_buffer,
**	and then points the global variable IIarDbname to this buffer.  If the
**	specified string is NULL, then IIarDbname is set to NULL, indicating
**	that no session is currently active.
**
** Inputs:
**	dbname	{char *}	new DB name.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Exceptions:
**	None.
**
** Side Effects:
**	Sets global IIarDbname to point to static copy of the specified name.
**
** History:
**	12-nov-92 (davel)
**		Initial version.
*/
VOID
IIARsadSetArDbname (dbname)
char *dbname;
{
	if (dbname == NULL || *dbname == EOS)
	{
		IIarDbname = NULL;
	}
	else
	{
		STcopy(dbname, dbname_buffer);
		IIarDbname = dbname_buffer;
	}
	return;
}
/*{
** Name:	IIARgasGetArStatus - Return handle to IIarStatus.
**
** Description:
**	This routine simply returns the address of IIarStatus so users
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
**		Address of IIarStatus.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/14/90 (dkh) - Initial version.
*/
STATUS *
IIARgasGetArStatus()
{
	return(&IIarStatus);
}
