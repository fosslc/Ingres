/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**
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
