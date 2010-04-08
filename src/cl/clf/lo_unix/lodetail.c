#include	<compat.h>
#include	<gl.h>
#include	<st.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*
 *Copyright (c) 2004 Ingres Corporation
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
 *		dev:	a pointer to a buffer where the device name will be stored
 *			NULL on UNIX.
 *		path:	a pointer to a buffer where the path will be stored
 *		fprefix:a pointer to a buffer where the part of a filename 
 *			before the dot will be stored.
 *		fsufix: a pointer to a buffer where the part of a filename 
 *			after the dot will be stored.
 *		version:a pointer to a buffer where the version will be stored.
 *			NULL on UNIX.
 *
 *	Result:
 *		stores the results in the given buffers.  Buffers should
 *		be of large enough size to hold the results.
 *
 *	Side Effects:
 *
 *	History:
 *		4/5/83 -- (mmm) written
 *		sept-90 (bobm) add LOcompose()
 *	    26-may-1993 (andys)
 *		Make sure there isn't a trailing / on the path element.
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	24-apr-1997 (canor01)
**	    Lack of a particular element can be shown by either a
**	    NULL pointer or an empty string.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
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
	register char	*source, *destination;
	register i4	length;
	char		*cptr;
	
	/* set the given NULLs on Unix */
	*dev = '\0';
	*version = '\0';

	if (loc->path == NULL)
	{
		*path = '\0';
	}
	else
	{
		length = loc->fname - loc->path;
		if (loc->path[length - 1] == SLASH)
			length--;
		STncpy( path, loc->path, length);
		path[ length ] = EOS;
	}
	if (loc->fname == NULL)
	{	
		*fprefix = '\0';
		*fsuffix = '\0';
	}
	else
	{
		/* break up filename */
		source = loc->fname;


		destination = fprefix;

		/* find last '.' in the filename */
		cptr = STrchr(source, '.');
		if (cptr == NULL)
		{
			/* no extension */
			STcopy(loc->fname, fprefix);
			*fsuffix = '\0';
		}
		else
		{
			/* deal with the extension */

			while ((*source != '\0') && (source != cptr))
			{
				*destination++ = *source++;
			}
			*destination = '\0';
			if (*source == '.')
			{
				source++;
				STcopy(source, fsuffix);
			}
			else
			{
				*fsuffix = '\0';
			}
	
		}
	}
	return(OK);
}

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
**		Sept-90 (bobm) - written.
**	    26-Oct-90 (pholman)
**		Added exit status from result of LOfroms.
**	    26-may-1993 (andys)
**		If there is only a path element passed in, then only pass 
**		PATH to LOfroms.
*/

LOcompose(dev, path, fprefix, fsuffix, version, loc)
char		*dev;
char		*path;
char		*fprefix;
char		*fsuffix;
char		*version;
LOCATION	*loc;
{
	char *ptr;
	i4 what;

	/*
	** dev / version ignored in UNIX environment.
	** This is intimately tied to the way LOfroms is implemented.
	**
	** The buffer passed to LOfroms becomes loc->string, so we
	** simply compose into the existing loc->string and let LOfroms
	** recover the correct settings for other parts.
	*/
	ptr = loc->string;
	if (path != NULL && *path != EOS)
	{
		STprintf(ptr,"%s/",path);
		ptr += STlength(ptr);
		what = PATH;
	}
	else
		what = FILENAME;
	if (fprefix != NULL && *fprefix != EOS)
	{
		STcopy(fprefix,ptr);
		ptr += STlength(ptr);
		what |= FILENAME;
	}
	if (fsuffix != NULL && *fsuffix != EOS)
	{
		STprintf(ptr,".%s", fsuffix);
		ptr += STlength(ptr);
		what |= FILENAME;
	}
	*ptr = '\0';

	return(LOfroms(what,loc->string,loc));
}
