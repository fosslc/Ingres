/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	"LOerr.h"

/*
**	History:
**
**	10/87 (bobm)	Integrate 5.0 changes
**
**	9/87 (bobm)	LODIRMAGIC, Obsolete MAXFNAMELEN - LO_NM_LEN
**			serves this purpose
**	07-mar-89 (russ)Removed the MKDIR_SYSCALL define.  This is
**			replaced by the more general xCL_009_RMDIR_EXISTS
**			and xCL_008_MKDIR_EXISTS defines from clconfig.h.
 * 
 * Revision 60.2  86/12/02  09:58:45  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:58:29  joe
 * *** empty log message ***
 * 
**		Revision 1.2  86/04/24  16:46:57  daveb
**		Define x42_FILESYS for several machines,
**		remove comment cruft.
**		
**		Revision 30.3  86/03/03  12:07:07  adele
**		Define x42_FILESYS for hp9000s200
**
**		Revision 30.2  85/09/16  17:22:32  roger
**		Removed redundant constant BYTESIZE (== BITSPERBYTE).
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Force error if <clconfig.h> has not been included.
**	28-feb-08 (smeke01) b120003
**	    Add definition of BACKSLASHSTRING for lotoes() function.
**
*/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before lolocal.h (in cl/clf/lo)"
# endif

/* Constants */

# define	MAXSTRINGBUF	120	/* max length of a string buffer */
# define	SLASH		'/'
# define	SLASHSTRING 	"/"
# define	BACKSLASH	'\\'
# define	BACKSLASHSTRING	"\\"

# ifndef	NULL
# define	NULL		'\0'
# endif

# define	BYTEMASK	0177

/* used to tell whether to open a file on successive calls to LOlist */

# define	FILECLOSED	-1

/* magic number used in the LOwcard, LOwnext calls */
# define	LODIRMAGIC	2001
