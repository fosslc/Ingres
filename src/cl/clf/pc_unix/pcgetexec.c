/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <pc.h>
#include    <st.h>
#include    <errno.h>
#include    <clsigs.h>

#include <stdlib.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

static long	get_file_mode(char *path);
static char	*getPathElement(register char *path, register char *buffer);

/*
** Name: PCgetexecname	    - exec the lp64 version of the running exe
**
** Description:
** 	exec the lp64 version of the running exe
**
** Inputs:
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	15-aug-2002 (hanch04/somsa01)
**	    Created
**	15-Jun-2004 (schka24)
**	    Watch out for buffer overrun
**	28-Apr-2005 (hanje04)
**	    Fix up prototypes to quiet warnings.
**	23-Jun-2008 (bonro01)
**	    Replace deprecated getwd() with getcwd().
*/
STATUS
PCgetexecname(char *argv0, char *buf)
{
    char	*path;

    if (*argv0 == '/')
    {
	strcpy(buf, argv0);
	return(0);
    }

    if (*argv0 == '.' && *(argv0+1) == '/')
    {
	strcpy(buf, argv0 + 2);
	goto found;
    }
    if (*argv0 == '.' && *(argv0+1) == '.' && *(argv0+2) == '/' )
    {
	strcpy(buf, argv0);
	goto found;
    }

    path = getenv("PATH");

    while(path != 0)
    {
	path = getPathElement(path, buf);
	if (*buf == '\0')
	    continue;  /* ignore empty components (::) */
	strcat(buf, "/");
	strcat(buf, argv0);
	if (access(buf, X_OK) == 0)
	{
	    /* watch out for directories that happen to match */
	    if((get_file_mode(buf) & S_IFMT) == S_IFDIR)
		continue;

	    /* Got it.  Now if it is not absolute, just prepend cwd */
	    if (*buf != '/')
	    {
		char	tmpbuf[PATH_MAX];

found:
		strcpy(tmpbuf, buf);
		if (getcwd(buf,PATH_MAX) == 0)
		    break;
		strcat(buf, "/");
		strcat(buf, tmpbuf);
	    }
	    return(0);
	}
    }

    /* Not found */
    *buf = 0;
    return(-1);
}

static long
get_file_mode(char *path)
{
    struct stat	stats;

    if (stat(path, &stats) < 0)
	return(-1);

    return(stats.st_mode);
}

/*  Strip off first element of path into buf.  Returns new path. */
static char *
getPathElement(register char *path, register char *buf)
{
    char *bufstart;
    i4 len;

    len = PATH_MAX-32;		/* leave room for what we're running */
    bufstart = buf;
    while (--len >= 0 && (*buf = *path++) != '\0')
    {
	if (*buf == ':')
	{
	    *buf = '\0';
	    return(path);
	}
	++buf;
    }
 
    if (len < 0)
	*bufstart = '\0';	/* Wipe element if too long */
    
    return(0);
}
