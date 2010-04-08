/*
**	Copyright (c) 1983, Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<st.h>
#include	<clconfig.h>
#include	<pc.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<io.h>

/**
** Name:	louniq.c -	CL Location Module Make a Unique Location.
**
** History:
**	Revision 6.2  89/02/23  daveb, seng
**	Rename LO.h to lolocal.h
**	89/07  wong  Modified to make location using input path location
**			for uniqueness.
**
**	Revision 2.0  83/03/09  mikem
**	Initial revision.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
*/

/*{
** Name:	LOuniq() -	Make a Unique Location.
**
** Description:
**	Make unique location.  It appends process ID using 'mktemp()' so the
**	location is guaranteed to be unique with respect to other processes on
**	the system, if not within the current process.  At present, it does not
**	guarantee a unique location if a suffix is specified.  Note also that
**	uniqueness cannot be guaranteed on an NFS file system.
**
**	To work correctly, the input path location must contain the path for
**	the file when 'LOuniq()' is called (any file name will be ignored.)
**	Then the input pattern string is prepended to an internal alphanumeric
**	pattern guaranteed (with respect to other calls of 'LOuniq()' for the
**	same directory) to be unique.  A NULL or empty path will use the
**	current process path to determine uniqueness.
**
**	The resulting file name including the input path is placed in
**	the input path location.  The file is not created.
**
** History:
**	19-may-95 (emmag)
**	    Branched for NT.
**	06-may-1996 (canor01)
**	    Added changes from unix version to pull pathname correctly
**	    from the location.  Used GetTempFileName() API function
**	    instead of the tmpnam() hack.
**	07-may-1996 (canor01)
**	    Added support for Unix version's side-effect of building
**	    just a filename with a zero-length path given.
**
*/

STATUS
LOuniq(pat, suffix, pathloc)
char			*pat;
char			*suffix;
register LOCATION	*pathloc;
{
    STATUS  LOfroms();

    char    dirbuf[MAX_LOC+1];    // Path + file name buffer.
    char    outbuf[MAX_LOC+1];    // Path + file name buffer.
    char    *nmp;              // file name pointer.

    if ( pathloc == NULL )
    {
	return FAIL;
    }

    /*
    ** If no path is given, the caller is probably trying to
    ** use a side-effect of LOuniq() to format a filename using
    ** the PID.
    */
    if ( pathloc->string && *pathloc->string == EOS )
    {
	if ( pat && *pat != EOS )
	    STcopy( pat, outbuf );
	else
	    outbuf[0] = EOS;
	nmp = outbuf;
	STcat( nmp, "XXXXXX" );
	if ( _mktemp( nmp ) == NULL )
	    return FAIL;

	/* add suffix if there is one */ 

	if ( suffix != NULL && *suffix != EOS )
	    STpolycat(2, ".", suffix, nmp + STlength(nmp));

    }
    else
    {

	memset(dirbuf, 0, MAX_LOC+1);

	/* Get path from input path location */
	if ( pathloc->path == NULL || pathloc->path == pathloc->fname )
	{
	    STcopy( ".", dirbuf );
	    nmp = dirbuf;
	}
	else
	{ 
	    /* get path from input */
	    if ( pathloc->fname == NULL )
		nmp = dirbuf + STlcopy(pathloc->path, dirbuf, sizeof(dirbuf)-1);
	    else
	    {
		/* assert:  STlength(pathloc->path) < sizeof(dirbuf) */
		(void) STlcopy(pathloc->path, dirbuf, sizeof(dirbuf) - 1);
		nmp = dirbuf + ( pathloc->fname - pathloc->path );
		if ( *(nmp-1) == '\\' )
		    *nmp = EOS;
	    }
	    nmp = dirbuf;
	}

	if(!GetTempFileName( nmp, pat, 0, outbuf ))
	    return FAIL;
    }

    /*
    ** Copy the generated filename into pathloc->string.
    ** Lord help the caller who provided too small a buffer...
    */
    strcpy(pathloc->string, outbuf);

    /* store new filename */
    if (LOfroms(PATH & FILENAME, pathloc->string, pathloc) != OK)
    {
       return FAIL;
    }

    return OK;
}
