/*
** Copyright (c) 2002 Ingres Corporation
**
*/
#include <compat.h>
#include <cm.h>
#include <lo.h>
#include <st.h>
#include <er.h>
#include <clconfig.h>
#include <lolocal.h>

/**
** Name: lovalid.c - is this LOCATION valid?
**
** Description:
**	This file contains code to validate a LOCATION, ensuring that it does
**	not contain any disallowed characters.
**
**	LOisvalid	- is this LOCATION valid?
**
** History:
**	12-apr-2002 (abbjo03)
**	    Created.
**	30-Dec-2004 (wansh01)
**	    Bug #113667 Prob# INGNET 158
**	    allowed '[' and ']' as valid char in the pathname.
**	03-Jun-2005 (drivi01)
**	    Modified LOisvalid to perform a check on order of
**	    components in a path as well as validity of 
**	    characters in the components.
**   23-Jun-2005 (fanra01)
**      Bug
**      Add empty string test for filename for path only verification.
**  23-Jun-2005 (fanra01)
**      Sir 114805
**      Add UNC path handling to the device test.
**      Add a status code as an output.
*/
# define SET_RETURN(retstat, rc) \
if (retstat != NULL) \
{ \
    *retstat = rc; \
}

/*{
** Name: LOisvalid	- is this LOCATION valid?
**
** Description:
**      LOisvalid returns TRUE if loc does not contain any unsupported
**	characters. On most platforms, excluding path component delimiters such
**	as slash, backslash, colon, and square brackets, a LOCATION can only
**	contain alphanumeric characters or underscore. On some platforms we may
**	also allow embedded spaces within a path component.
**
** Inputs:
**      loc	the location in question
**
** Outputs:
**	retstat     Status code if return is FALSE.
**
** Returns:
**	FALSE	if the location contains disallowed characters
**
** History:
**	12-apr-2002 (abbjo03)
**	    Written.
**	30-Dec-2004 (wansh01)
**	    Bug #113667 Prob# INGNET 158
**	    allowed '[' and ']' as valid char in the pathname.
**  23-Jun-2005 (fanra01)
**      Treat a UNC device differently, no need to append a device separator.
**      Don't fail if file component is an empty string.
**  23-Aug-2005 (drivi01)
**	Same as change #478752 for bug 115052.
**	Modify LOisvalid function to detect digit as a leading
**	character.
*/
bool
LOisvalid(
LOCATION	*loc,
STATUS* retstat )
{
    char	dev[MAX_LOC];
    char	path[MAX_LOC];
    char	partPath[MAX_LOC];
    char	file[MAX_LOC];
    char	ext[MAX_LOC];
    char	vers[MAX_LOC];
    char	*p;
    i4		drType;
    i4		nameStart = FALSE;
    STATUS  status = OK;
    
    if ((status = LOdetail(loc, dev, path, file, ext, vers)) != OK)
    {
        SET_RETURN(retstat, status);
	    return FALSE;
    }
	
	/* for NT, we'll assume dev is a single character A-Z and vers is empty */

	/* Verify that a drive specified is a valid drive */
    if (dev[0] != '\0')
    {
        if (CMalpha( &dev[0] ))
        {
            drType = GetDriveType(STcat(dev, ERx(COLONSTRING)));
        }
        if ((CMcmpnocase( dev, SLASHSTRING) == 0) &&
            (CMcmpnocase( dev + CMbytecnt( dev ), SLASHSTRING) == 0))
        {
            drType = GetDriveType( dev );
        }
        if (drType != DRIVE_FIXED 
             && drType != DRIVE_REMOVABLE 
             && drType != DRIVE_REMOTE)
        {
            SET_RETURN(retstat, LO_BAD_DEVICE);
            return FALSE;
        }
    }
	/* Verify that path consists of valid characters */
    for (p = path; *p != EOS; CMnext(p))
	{
		if (!(CMpath( p )))
        {
            SET_RETURN(retstat, LO_NOT_PATH);
			return FALSE;
        }
   	    /* If we're at the beginning of directory name in the path
	    ** check that it's a valid first character.
	    */
	    if (nameStart && !CMnmstart( p ) && !CMdigit( p ))
        {
            SET_RETURN(retstat, LO_NOT_PATH);
			return FALSE;
        }
     	nameStart = FALSE;

     	if(*p == SLASH)
	    {
			nameStart = TRUE;
	    }
	}

	/* Verify that filename consists of valid characters */
    if (*file && !CMnmstart( file ) && !CMdigit( file ))
    {
        SET_RETURN(retstat, LO_NOT_FILE);
		return FALSE;
    }
    for (p = file; *p != EOS; CMnext(p))
		if  (!(CMpath( p ) || CMpathctrl( p )))
		{
            SET_RETURN(retstat, LO_NOT_FILE);
			return FALSE;
		}
    
	/* Verify that filename suffix consists of valid characters */
    for (p = ext; *p != EOS; CMnext(p))
		if (!(CMpath(p) || CMpathctrl( p )))
		{
            SET_RETURN(retstat, LO_NOT_FILE);
			return FALSE;
		}
    
    return TRUE;
}
