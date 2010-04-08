/*
** vftofrm.c
**
** contains routines which write out the internal frame
** structure to the external structure
**
** History
**	01/30/85 (drh)	Changed UTcomm call to UTexe call for cfe/ft integration
**	02/01/85 (drh)	Copied PeterK's change moving allocArr into it's own
**			source file.
**
**	2/85	(ac)	Made necessary changes for vifred calling
**			writefrm module at different point.
**	24-apr-89 (bruceb)
**		Zapped def of nslist.  No longer any nonseq fields.
**	28-feb-90 (bruceb)
**		New parameter (bypass_submenu) to vfToFrm() that is
**		passed down to OOsaveSequence().  Indicates whether or
**		not the 'EditInfo  Save' submenu is displayed or bypassed.
**
**	Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"


GLOBALDEF FIELD **fldlist = NULL;
GLOBALDEF TRIM	**trmlist = NULL;
GLOBALDEF i4	tnumfield = 0;


i4
vfToFrm(bypass_submenu)
bool	bypass_submenu;
{
# ifndef FORRBF
	i4	retval;
	i4	trimno;

	/*
	** remember the number of trim elements because vfseqbuild will
	** modify because it must add in the box features for the purposes
	** of writing out the frame.
	*/
	trimno = frame->frtrimno;

	/* user interactions and write the form */
	retval = getFrmName(bypass_submenu);

	/*
	** restore the saved trim count cause it only accounts for the 
	** actual trim structures during normal edit operations
	*/
	frame->frtrimno = trimno;

	return (retval);
# else
	vfseqbuild(FALSE);
	return(TRUE);
# endif /* FORRBF */
}


