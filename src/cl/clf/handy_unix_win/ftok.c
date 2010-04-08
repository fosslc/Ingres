/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <sys/types.h>
# include <sys/stat.h>
# include <compat.h>
# include <gl.h>
# include <clconfig.h>

# ifdef xCL_039_FTOK_EXISTS

/* shutup ranlib */
i4	ftok_dummy;

# else /* xCL_039_FTOK_EXISTS */

/*
**   FTOK.C -- an emulation of ftok(3).   
**
**
*/
key_t
ftok(path, id)
char *path;
char id;
{
	struct stat st;
	return(stat(path, &st) < 0 ? (key_t)-1 :
	    (key_t)((key_t)id << 24 | ((long)(unsigned)minor(st.st_dev)) << 16 |
		(unsigned)st.st_ino));
}

# endif	/* xCL_039_FTOK_EXISTS */
