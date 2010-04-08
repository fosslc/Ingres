/*
**	Copyright (c) 1989, 1991 Ingres Corporation
**	All rights reserved.
*/

#include        <compat.h>
#include        <cm.h>
#include        <gl.h>
#include        <clconfig.h>
#include        "lolocal.h"
#include        <lo.h>
#include        <st.h>

/*
**	Name:
**		LOcompose
**
**	Description:
**		Put "LOdetail" pieces back together again.
**
**	Input:
**		dev, path fprefix, fsuffix, version: as for LOdetail.
**
**	Output:
**		loc - location composed from above.
**
**	History:
**	    Sept-90 (bobm) - written.
**	    26-Oct-90 (pholman)
**		Added exit status from result of LOfroms.
**          30-Apr-1996 (somsa01)
**              Added checks to see if incoming strings are null.
**          11-dec-1996 (wilan06)
**              Fix LOcompose - LOfroms uses & to combine its flags, not |
**	    14-may-97 (natjo01)
**		Handle case of just a device given.  
**	    04-jun-1997 (canor01)
**	 	Deal with special case of root directory.
**	    24-may-1999 (somsa01)
**		Include "st.h".
**          06-aug-1999 (mcgem01)
**              Replace nat and longnat with i4.
**      23-Jun-2005 (fanra01)
**          Sir 114805
**          Add test for the start of a UNC path in the device portion of the
**          composed components and copy into the target buffer.
*/

STATUS
LOcompose(
char		*dev,
char		*path,
char		*fprefix,
char		*fsuffix,
char		*version,
LOCATION	*loc)
{
    char *ptr;
    i4  what;
    bool devname = FALSE; 

    /*
    ** This is intimately tied to the way LOfroms is implemented.
    **
    ** The buffer passed to LOfroms becomes loc->string, so we
    ** simply compose into the existing loc->string and let LOfroms
    ** recover the correct settings for other parts.
    */
    ptr = loc->string;
    if(dev && *dev != EOS)
    {
        if (CMalpha( dev ))
        {
            CMcpychar( dev, ptr );
            CMnext( ptr );
            CMcpychar( COLONSTRING, ptr );
            CMnext( ptr );
            devname = TRUE;
        }
        if ((CMcmpnocase(dev, SLASHSTRING) == 0) &&
            (CMcmpnocase(dev + CMbytecnt(dev), SLASHSTRING) == 0))
        {
            strcpy( ptr, dev );
            ptr += strlen( dev );
            devname = TRUE;
        }
    }
    if (path && *path != EOS)
    {
	strcpy(ptr, path);
	ptr += strlen(ptr);
	what = PATH;
    }
    else
	what = 0;

    if (fprefix && *fprefix != EOS)
    {
	if(what == PATH && *(ptr-1) != SLASH)
	    *ptr++ = SLASH;
	strcpy(ptr, fprefix);
	ptr += strlen(ptr);
	what &= FILENAME;
    }

    if (fsuffix && *fsuffix != EOS)
    {
	if(what == PATH && *(ptr-1) != SLASH)
	    *ptr++ = SLASH;
	*ptr++ = '.';
	strcpy(ptr, fsuffix);
	what &= FILENAME;
    }

    /* if only a dev then make it the root path for LOfroms */ 
    if (devname && !what && STlength(loc->string) >= 4)
    {
	STcopy(SLASHSTRING, loc->string + 2);
	what = PATH; 
    }

    return LOfroms(what,loc->string,loc);
}
