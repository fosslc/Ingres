/*
** Copyright (c) 1983, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<clconfig.h>
#include	<gl.h>
#include	<ex.h>
#include	<lo.h>
#include	<si.h>
#include	"silocal.h"
# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif


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
**	11-sep-96 (bespi01) Bug: #102615/Problem: INGSRV#974
**		Unix.
**		Fixed file descriptor leak by setting the close on exec flag.
**      03-jun-1999 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jan-2001 (bespi01) Bug: #102615/Problem: INGSRV#974
**	    Unix.
**	    Fixed file descriptor leak by setting the close on on exec flag.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.  Remove NT_GENERIC section.
*/

STATUS
SIopen (loc, mode, desc)
register LOCATION	*loc;
register char		*mode;		/* "a","r","w", or "rw" */
FILE			*(*desc);	/* file pointer to opened file */
{
    register char	*fmp;
    char		fmode[3];
    STATUS		status = OK;

    FILE		*fopen();

    fmode[2] = EOS;
    fmp = fmode;
    if (mode[1] == EOS && (mode[0] == 'r' || mode[0] == 'w' || mode[0] == 'a'))
    {
	*fmp++ = mode[0];
    }
    else if (mode[0] == 'r' && mode[1] == 'w' && mode[2] == EOS)
    {
	*fmp++ = 'r'; *fmp++ = '+';
    }
    else
    {
	status = SI_BAD_SYNTAX;
    }

    if (status == OK)
    {
	*fmp = EOS;
#ifdef LARGEFILE64
	if ((*desc = fopen64(loc->string, fmode)) == (FILE *)NULL)
#else
	if ((*desc = fopen(loc->string, fmode)) == (FILE *)NULL)
#endif /* LARGEFILE64 */
	    status = SI_CANT_OPEN;
    }

    /* set the close on exec bit, where possible */

# ifdef xCL_006_FCNTL_H_EXISTS
    if (*desc)
	fcntl(fileno(*desc), F_SETFD, FD_CLOEXEC); 
# endif

    return status;
}
