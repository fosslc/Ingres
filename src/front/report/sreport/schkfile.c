/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	<st.h>
# include	<er.h>

/*
**   S_CHK_FILE - check a file name to see whether it contains a
**	extension or not.  If not, add the default (.rw) extension
**	to the file name.
**
**	Parameters:
**		fname	file name.
**
**	Returns:
**		returns fname if already has an extension, else it
**		returns ptr to a new allocated name containing
**		the name of the file, with extension added.
**
**	Called by:
**		s_readin.
**
**	Side Effects:
**		none.
**
**	History:
**		10/24/82 (ps)	written.
**		25-mar-1987 (yamamoto)
**			Modified for double byte character name.
**		1/25/90 (elein)
**			Changed decl of fbuf to MAX_LOC
**		3-26-93 (fraser)
**			Allocate enough memory for fbuf.  The caller 
**			eventually passes the string which s_chk_file returns
**			to LOfroms, which requires a buffer of size MAX_LOC.
**			I didn't check for errors from MEreqmem because
**			the function has no provision for indicating an
**			error.
**
**			Note that there are a number of other problems with
**			this routine: 1) it doesn't do what it purports
**			to do; 2) it has VMS-specific code.  If one felt
**			like it, this routine could be rewritten using
**			LO: LOdetail to split out the extension and
**			examine it; LOcompose to add an extension if
**			necessary.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

char	*
s_chk_file(fname)
char	*fname;
{
	/* internal declarations */

	char		*fbuf;		/* buffer to hold file name */
	i4		len;		/* length of the name */
	char		*pptr;
	char		*bptr;
	STATUS		status;

	/* start of routine */
	fbuf = (char *) MEreqmem( (u_i4) 0, (u_i4) MAX_LOC + 1,
					(bool) FALSE, &status);
	STcopy(fname,fbuf);
	len = STlength(fbuf);
	if (len == 0)
		return(NULL);

	pptr = STrchr(fbuf, '.');
	bptr = STrchr(fbuf, ']');

	if (pptr != NULL && pptr > bptr)
		return (fname);

	STcat(fbuf,ERx(".rw"));
	return (fbuf);
}
