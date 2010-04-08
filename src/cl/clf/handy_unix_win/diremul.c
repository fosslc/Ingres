/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <clconfig.h>

#ifdef xCL_024_DIRFUNC_EXISTS

/* shutup ranlib */
i4	diremul_dummy;

#else

#ifdef _REENTRANT
#error This code was not checked for reentrancy! (tutto01)
#endif

# include <diracc.h>
# include <sys/stat.h>

/*
** The following are from the directory emulation system sent out
** on the Usenet by Doug Gwyn of the Ballistics Research Lab.
** They have not been substantially modified for our use.
**
** In combination with the local "dir.h," they provide an interface
** compatible with the BSD directory functions on System V or V7
** systems using the "AT&T" directory structures.
**
*/

/* 
** The following have been copied from LOlist for 6.0 jupiter DIlist*()
** functions.  An effort is being made to use as little as possible in
** the jupiter server from the LO routines.  So this code has been copied
** and made static in the hope that LO routines will eventually not be
** linked into the jupiter server.  (mmm)
*/

/*
**	closedir -- C library extension routine
**
**	last edit:	21-Jan-1984	D A Gwyn
*/

extern void	free();
extern int	close();

void
closedir( dirp )
register DIR	*dirp;		/* stream from opendir() */
{
	(void)close( dirp->dd_fd );
	free( (char *)dirp );
}


/*
**	readdir -- C library extension routine for non-BSD UNIX
**
**	last edit:	09-Jul-1983	D A Gwyn
*/

extern char	*strncpy();
extern int	read(), strlen();

#define DIRSIZ	14

struct olddir
{
	ino_t	od_ino; 		/* inode */
	char	od_name[DIRSIZ];	/* filename */
};

struct direct *
readdir( dirp )
register DIR		*dirp;	/* stream from opendir() */
{
	register struct olddir	*dp;	/* -> directory data */

	for ( ; ; )
		{
		if ( dirp->dd_loc >= dirp->dd_size )
			dirp->dd_loc = dirp->dd_size = 0;

		if ( dirp->dd_size == 0 	/* refill buffer */
		  && (dirp->dd_size = read( dirp->dd_fd, dirp->dd_buf, 
					    DIRBLKSIZ
					  )
		     ) <= 0
		   )
			return NULL;	/* error or EOF */

		dp = (struct olddir *)&dirp->dd_buf[dirp->dd_loc];
		dirp->dd_loc += sizeof(struct olddir);

		if ( dp->od_ino != 0 )	/* not deleted entry */
			{
			static struct direct	dir;	/* simulated */

			dir.d_ino = dp->od_ino;
			(void)strncpy( dir.d_name, dp->od_name, DIRSIZ
				     );
			dir.d_name[DIRSIZ] = '\0';
			dir.d_namlen = strlen( dir.d_name );
			dir.d_reclen = sizeof(struct direct)
				     - MAXNAMLEN + 3
				     + dir.d_namlen - dir.d_namlen % 4;
			return &dir;	/* -> simulated structure */
			}
		}
}


/*
**	opendir -- C library extension routine
**
**	last edit:	09-Jul-1983	D A Gwyn
*/

#ifdef	BRL
#define open	_open			/* avoid emulation */
#endif

extern char	*malloc();
extern int	open(), close(), fstat();

DIR *
opendir( filename )
char		*filename;	/* name of directory */
{
	register DIR	*dirp;		/* -> malloc'ed storage */
	register int	fd;		/* file descriptor for read */
	struct stat	sbuf;		/* result of fstat() */

	if ( (fd = open( filename, 0 )) < 0 )
		return NULL;

	if ( fstat( fd, &sbuf ) < 0
	  || (sbuf.st_mode & S_IFMT) != S_IFDIR
	  || (dirp = (DIR *)malloc( sizeof(DIR) )) == NULL
	   )	{
		(void)close( fd );
		return NULL;		/* bad luck today */
		}

	dirp->dd_fd = fd;
	dirp->dd_loc = dirp->dd_size = 0;	/* refill needed */

	return dirp;
}

#endif	/* xCL_024_DIRFUNC_EXISTS */
