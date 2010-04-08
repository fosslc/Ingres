#include 	<compat.h>  
#include    <gl.h>
#include	<st.h>  
#include	<lo.h>  
# include 	  <er.h>  
#include	"lolocal.h"

/*{
** Name:	LOcompose	- 	Combine pieces into a location
**
** Description:
**	The opposite of LOdetail; combine pieces into a single location.
**
** Inputs:
**	dev	char *		device name
**	path	char *		directory name
**	fprefix	char *		filename 
**	fsuffix char *		filetype 
**	version	char *		version 
**
** Outputs:
**	loc	LOCATION *	Location created
**
**	Returns:
**		STATUS	OK or failure.
**
**	Exceptions:
**		none.
**
** Side Effects:
** 		none
** History:
**	10/89	(Mike S)	Initial version.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
*/
STATUS
LOcompose(dev, path, fprefix, fsuffix, version, loc)
char 	*dev;
char	*path;
char	*fprefix;
char	*fsuffix;
char	*version;
LOCATION	*loc;
{
	char	buffer[MAX_LOC+1];
	char 	*p = buffer;
	bool	haspath = FALSE;
	bool 	hasfilename = FALSE;
	LOCTYPE	what;
	LOCATION tmploc;
	STATUS status;

	/* Check args */
	if (loc == NULL || loc->string == NULL)
		return (LO_NULL_ARG);

	/* Form file name */
	if (dev != NULL && *dev != EOS)
	{
		STpolycat(2, dev, COLON, p);
		haspath = TRUE;
		p += STlength(p);
	}
	if (path != NULL && *path != EOS)
	{
		STpolycat(3, OBRK, path, CBRK, p);
		haspath = TRUE;
		p += STlength(p);
	}
	if (fprefix != NULL && *fprefix != EOS)
	{
		STcopy(fprefix, p);
		hasfilename = TRUE;
		p += STlength(p);
	}
	if (fsuffix != NULL && *fsuffix != EOS)
	{
		STpolycat(2, DOT, fsuffix, p);
		hasfilename = TRUE;
		p += STlength(p);
	}
	if (version != NULL && *version != EOS)
	{
		STpolycat(2, SEMI, version, p);
		hasfilename = TRUE;
		p += STlength(p);
	}
	*p = EOS;

	/* Make a new location */
	if (haspath && hasfilename)
		what = PATH & FILENAME;
	else if (haspath)
		what = PATH; 
	else if (hasfilename)
		what = FILENAME;
	else
		return (LO_NULL_ARG);

	if ((status = LOfroms(what, buffer, &tmploc)) != OK)
		return status;

	LOcopy(&tmploc, loc->string, loc);
	return (OK);
}
