/*
**	Copyright (c) 1997 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<gl.h>
#include	<ex.h>
#include	<lo.h>
#include	<si.h>
#include	"silocal.h"


/*{
** Name:    SIopen() -	Open location stream.
**
** Description:
**	Open the given location and return a FILE* in desc.
**	Allowed modes are READ, WRITE, APPEND, READWRITE. Currently
**	READWRITE positions the stream at the beginning of the
**	file.  APPEND opens for writing at end of file or create
**	for writing if it doesn't exist.
**
**	the unix call "fopen" opens the file named by filename and associates
**	a stream with it.  Fopen returns a pointer to be used to identify 
**	the stream in subsequent operations.
**
**	returns STATUS: OK, FAIL, NO_PERM, NO_SUCH
**
** History:
**	03/09/83 (mmm) -- written
**	11/12/85 (gb)  -- Raise EX_NO_FD to call DIlru_close().
**	03/23/89 (kathryn) -- Bug #4533
**		Return SI_CANT_OPEN rather than LOerrno, So correct
**		Errorr Message Prints. There was no error message for LOerrno.
**	9/89 (bobm) - remove obsolete EX_NO_FD exception.
**	07/18/91 (vijay)
**		Fix for an aix bug. fopen("xx", "r\0+") is taken to be r+
**		on the RS/6000. this results in bug 38645. (iiinterp failure for
**		'call qbf'). Fix is to make sure it is "r\0\0".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	30-nov-94 (sweeney)
**	    Try to set the close on exec bit (bug # 31071).
**	12-jun-96 (somsa01)
**		Modified NT version; if file cannot be open, changed return 
**		error from errno to SI_CANT_OPEN.
**	10-feb-1997 (canor01)
**	    Remove Unix-specific code, since file now branched for NT.
**	    Enable the Windows NT "commit-to-disk" flag for fopen() to 
**	    allow emulation of fsync().
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      12-apr-2005 (crogr01)
**	    Bug 110563
**          Open files in _O_TEXT mode if _O_BINARY has not been specified
**          since we can not rely on the global _fmode being set to _O_TEXT.
*/


STATUS
SIopen(
LOCATION *	loc,		/* LOCATION containing name of file to open. */
char * 		mode,		/* "a","r","w","rw","ab","rb","wb", "rwb" or "rwt" */
FILE *		(*desc))	/* file pointer to opened file */
{
    register FILE *	tempfp;
    register char *	fmp;
    char		fmode[5];
    STATUS		status = OK;

    tempfp = (FILE *)NULL;
    fmp = fmode;
    if (mode[0] == 'r' || mode[0] == 'w' || mode[0] == 'a')
    {
	*fmp++ = mode[0];
    	switch (strlen(mode))
	{
	case 1:
		*fmp++ = 't'; 
		break;

	case 2:
		if (mode[0] == 'r' && mode[1] == 'w')
		{
			*fmp++ = '+';
			*fmp++ = 't';
		}
		else if (mode[1] == 'b')
		{
			*fmp++ = 'b';
		}
		else
		{
			status = SI_BAD_SYNTAX;
		}
		break;

	case 3:
		if	(strcmp(mode, "rwb") == 0)
		{
			*fmp++ = '+';
			*fmp++ = 'b';
			break;
		}
		else if	(strcmp(mode, "rwt") == 0)
		{
			*fmp++ = '+';
			*fmp++ = 't';
			break;
		}
		
/*		else	fall into the default case;	*/

	default:
		status = SI_BAD_SYNTAX;
		break;
	}
    }
    else
    {
	status = SI_BAD_SYNTAX;
    }
    *fmp++ = 'c'; /* enable commit-to-disk, i.e. fsync() */
    if (status == OK)
    {
	*fmp = EOS;

	status = (*desc = fopen(loc->string, fmode)) ? OK : SI_CANT_OPEN;
    }

    return status;
}

# ifdef SIfopen 
# undef SIfopen
# endif

/*{
** Name:    SIfopen() - Open location stream.
**
** Description:
**	returns STATUS: OK, FAIL, NO_PERM, NO_SUCH
**
** History:
**     19-jun-95 (emmag)
**         SIfopen should open the file in binary mode on desktop platforms.
**     06-Sep-05 (drivi01)
**         Replaced type == VAR_FILE with type==SI_VAR due to change 
**         478954 for Bug 111727 since SI_VAR value on windows changed
**          from 5 to 6 and SIfopen wasn't openning the file correctly.
*/

STATUS
SIfopen(
LOCATION *	loc,
char *		mode,		/* "a","r","w", or "rw" */
i4 		type,
i4      	length,
FILE *		(*desc))	/* file pointer to opened file */
{
	char	fmode[4];

	strcpy(fmode, mode);
	if ((type == SI_RACC) || (type == SI_BIN) || (type == SI_VAR))
		strcat(fmode, "b");
	return SIopen (loc, fmode, desc);
}
