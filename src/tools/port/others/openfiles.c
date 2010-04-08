/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**      History:
**	    07-May-95 (johnst)
**		Changed to use ulimit instead of _NFILES or getdtablesize
**		on Unixware 2.0 (usl.us5), which means including generic.h.
*/

# include	<stdio.h>
# include	<sys/types.h>
# include	<sys/stat.h>

# include "generic.h"
# if defined(usl_us5)
# include     <ulimit.h>
# endif

/*
** Show info about any non-standard open files
*/

main()
{
	return(openfiles());
}

openfiles()
{
	int	i;
	int	rc = 0;
	struct	stat statbuf;
	int	nfiles;
# ifndef _NFILE
# if defined(usl_us5)
	nfiles = (int)ulimit(4, 0L);
# else /* usl_us5 */
	extern	int	getdtablesize();

	nfiles = getdtablesize();
# endif /* usl_us5 */

# else
	nfiles = _NFILE;
# endif
	for (i = 0; i< nfiles ; i++)
	{
		if (-1 != fstat(i, &statbuf))
		{
			fprintf(stderr, 
		"fd %d:  inode %d, uid %d, gid %d, size %d, mode 0%o\n", 
				i, 
				statbuf.st_ino,
				statbuf.st_uid,
				statbuf.st_gid,
				statbuf.st_size,
				statbuf.st_mode);

			rc = 1;
		}
	}
	return(rc);
}

