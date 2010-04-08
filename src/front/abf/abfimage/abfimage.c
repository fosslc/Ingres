/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<abfcnsts.h>
#include        <er.h>

/**
** Name:	abfimage.c -	ABF Image Builder.
**
** Description:
**	This files contains:
**
**	abfimage	Create an abf image.
**
** History:
**	Revision 6.0  9-jun-1987 (Joe)
**	Initial Version.
**	12/15/89 (dkh) - VMS shared lib changes.  Reference to IIarDbname
**			 is now via IIar_Dbname so we can vectorize
**			 access to IIarDbname.
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added call to IIAMgobGetIIamObjects() routine to pass data 
**		across ABF/IAOM boundary.  (See iaom/abfiaom.c for more info).
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF  char	**		IIar_Dbname;

FUNC_EXTERN OO_OBJECT **	IIAMgobGetIIamObjects();      

/*{
** Name:	abfimage() -	Main Line of ABF Image Builder.
**
** Description:
**	This routines initializes all the pieces of ABF needed
**	to create an image.  It then calls those pieces.
**
** Inputs:
**	applname	{char *}  The name of the application for which to
**				  create an image.
**
**	options		{nat}  Compilation options; force compile, etc.
**
**	nolink		{bool}  Whether to link the executable for the
**				application.
**
**	executable	{char *}  The name of the executable to create.  If this
**				  is empty, this routine should use a
**				  default name and should fill in the buffer
**				  pointed to by this variable with the name
**				  of the default executable.
**
** Outputs:
**	executable	{char *}  If empty on input, then on return it
**				  will contain the name of the executable
**				  built.
**
** Returns:
**	{STATUS}  OK if succeeded,
**		  FAIL otherwise.
**
** History:
**	9-jun-1987 (Joe)
**		Initial Version.
*/
STATUS
abfimage ( applname, options, nolink, executable )
char	*applname;
i4	options;
bool	nolink;
char	*executable;
{
	STATUS	IIABilnkImage();

	aboslver();
	IIABlnkInit(/* DYNAMICLINK = */ FALSE);
	IIOOinit( IIAMgobGetIIamObjects() );		 
	IIABidirInit(*IIar_Dbname);
	if (FEchkname(applname) != OK)
	{
		abusrerr(APPNAME, applname, (char *)NULL);
		return FAIL ;
	}

	CVlower(applname);
	return IIABilnkImage(applname, OC_UNDEFINED, executable, options);
}
