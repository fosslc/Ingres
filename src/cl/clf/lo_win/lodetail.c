#include	<compat.h>
#include	<cm.h>
#include	<gl.h>
#include	<st.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*
 *	Copyright (c) 1995 Ingres Corporation
 *
 *	Name:
 *		LOdetail
 *
 *	Function:
 *		Returns various and sundry parts of a location in string
 *		form.
 *
 *	Arguments:
 *		loc:	a pointer to the location in question
 *		dev:	a pointer to a buffer where the device name is stored
 *		path:	a pointer to a buffer where the path will be stored
 *		fprefix:a pointer to a buffer where the part of a filename 
 *			before the dot will be stored.
 *		fsufix: a pointer to a buffer where the part of a filename 
 *			after the dot will be stored.
 *		version:a pointer to a buffer where the version will be stored.
 *			NULL on NT.
 *
 *	Result:
 *		stores the results in the given buffers.  Buffers should
 *		be of large enough size to hold the results.
 *
 *	Side Effects:
 *
 *	History:
 *	    26-may-1995 (emmag)
 *		Branched for NT.	
 *          29-apr-1996 (somsa01)
 *              The extracted path should not contain the drive or the filename.
 *	    04-jun-1997 (canor01)
 *	        Do not remove the trailing slash from the extracted path
 *		if the path is the root directory.
 **     23-Jun-2005 (fanra01)
 **         Sir 114805
 **         Add test and treatment of UNC path string.
 */
 STATUS
 LOdetail(loc, dev, path, fprefix, fsuffix, version)
 LOCATION	*loc;
 char		*dev;
 char		*path;
 char		*fprefix;
 char		*fsuffix;
 char		*version;
 {
    register char	*source; 
    register i4		length;
    char		*cptr;
	char*       uncpath;
    char*       p;
    
    if(!version || !path || !dev || !fprefix || !fsuffix)
        return LO_NULL_ARG;

    /* init everything to none */
    *version = '\0';
    *path = '\0';
    *dev = '\0';
    *fprefix = '\0';
    *fsuffix = '\0';

    if (source = loc->path)
    {
        if (CMcmpnocase( source + CMbytecnt(source), COLONSTRING ) == 0)
        {
            CMcpychar( source, dev );
            *(dev + CMbytecnt(dev)) = '\0';
            CMnext(source);
            CMnext(source);
        }
        if (((p = STstrindex( source, UNCSTRING, 0, FALSE )) != NULL) &&
            (p == source))
        {
            /* UNC filename */
            /*
            ** A path that starts with two slash '\' characters is assumed
            ** to be a UNC path of the format
            ** \\machine\nameddirectory\path. The resulting components
            ** are \\machine\nameddirectory and \path.
            */
            CMnext( p );
            CMnext( p );
            /*
            ** Scan the path string for the second backslash character as the
            ** start of the directory list, skipping the machine and the
            ** named directory.
            */
            if (((uncpath = STindex( p, SLASHSTRING, 0 )) != NULL) &&
                ((uncpath = STindex( uncpath + CMbytecnt(uncpath),
                    SLASHSTRING, 0 )) != NULL ))
            {
                length = uncpath - source;
                STlcopy( source, dev, length );
                source += length;
            }
	    }

  /* use source over loc->path, since source has been "stripped" of the drive letter */
        length = loc->fname - source;

	/* delete the trailing slash of path, unless it's the root directory */
        if (source[length - 1] == SLASH && length > 1)
        	length--;
        STlcopy(source, path, length);
    }
    if (source = loc->fname)
    {
        /* find last '.' in the filename */
        if(!(cptr = strrchr(source, '.')))
        {
            /* no extension */
            strcpy(fprefix, source);
        }
        else
        {
            memcpy(fprefix, source, cptr-source);
            fprefix[cptr-source] = '\0';
            strcpy(fsuffix, cptr+1);
        }
    }
    return OK;
}
