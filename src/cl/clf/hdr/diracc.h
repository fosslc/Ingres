/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** DIRACC.H -- directory access header, shared througout the CL.
**
** This provides a portable interface to directory access routines
** that are used in LO and DI.  The interface presented is nearly the
** same as the SVR3 "dirent" style opendir/readdir, with one exception:
** the length of the filename must be obtained by going through the
** NAMLEN( p ) macro defined below.  
**
** Also, beware that "." and ".." may or may not be present, or be the
** first entries in the directory.
**
** Assumes that clconfig.h has been included already.
**
** History:
**	07-oct-88 (daveb)
**		Split from di so it can be shared between DI and LO.
**		The use of xDI... defines is kind of silly.
**		Integrate with LO versions.
**	28-Feb-1989 (fredv)
**		Added more ifdef xCL... stuff. If we see dirent.h and opendir()
**			exists, we use <dirent.h>, not <sys/dir.h>.
**	13-mar-89 (russ)
**		When defining the NAMLEN macro use the new xCL_037_SYSV_DIRS
**		define rather than xCL_025_DIRENT_EXISTS.  This define is
**		based on the outcome of the mkscrtdir.sh script.
**	13-Apr-1989 (fredv)
**		Removed junk left in last submit.
**	21-Mar-89 (markd)
**		Replace WECOV with sqs_us5.
**	31-may-90 (blaise)
**		Remove bogus definition of NAMLEN, which is no longer used.
**		The definition is false because it is based on the erroneous
**		assumption that SysV's d_reclen is the equivalent of bsd's
**		d_namlen.
**	10-may-1999 (walro03)
**		Remove obsolete version string sqs_us5.
*/

# ifndef CLCONFIG_H_INCLUDED
error "You didn't include clconfig.h before including diracc.h"
# endif

#if defined(xCL_024_DIRFUNC_EXISTS) || defined (xCL_025_DIRENT_EXISTS)

/*
** System provides opendir(), readdir(), etc.  Use its
** header file
*/

# ifdef xCL_025_DIRENT_EXISTS
# include <dirent.h>

#else /* xCL_025_DIRENT_EXISTS */

#ifdef xCL_026_NDIR_H_EXISTS
#include	<ndir.h>
#endif

#ifdef xCL_027_SYS_NDIR_H_EXISTS
#include	<sys/ndir.h>
#endif

#ifdef xCL_028_SYS_AUX_AUXDIR_H_EXISTS
#include	<sys/aux/auxdir.h>
#endif

/* In the Symmetry att universe use the BSD-compatible
** directory access dir.h instead of sys/dir.h  (markd)
*/

#ifndef DIRBLKSIZ
#include	<sys/dir.h>
#endif

# endif /*xCL_025_DIRENT_EXISTS */
#else	/* We need our own structures and routines */

/*
**	<dir.h> -- definitions for 4.2BSD-compatible directory access
**
**	last edit:	09-Jul-1983	D A Gwyn
*/

/* 
** System does not have opendir() etc.
** Provide compatible interface for standard AT&T directory
** structure.
*/

#define DIRBLKSIZ	512		/* size of directory block */
#define MAXNAMLEN	15		/* maximum filename length */
	/* NOTE:  MAXNAMLEN must be one less than a multiple of 4 */

struct direct				/* data from readdir() */
{
	long		d_ino;		/* inode number of entry */
	unsigned short	d_reclen;	/* length of this record */
	unsigned short	d_namlen;	/* length of string in d_name */
	char		d_name[MAXNAMLEN+1];	/* name of file */
};

typedef struct
{
	int	dd_fd;			/* file descriptor */
	int	dd_loc;			/* offset in block */
	int	dd_size;		/* amount of valid data */
	char	dd_buf[DIRBLKSIZ];	/* directory block */
}	DIR;				/* stream data from opendir() */

extern DIR		*opendir();
extern struct direct	*readdir();
extern void		closedir();

# endif	   /* xCL_024_DIRFUNC_EXISTS || xCL_025_DIRENT_EXISTS */


/* Now some tricky stuff to make old  stuff look like dirent */

# ifndef xCL_025_DIRENT_EXISTS
# 	define	dirent	direct
# endif
