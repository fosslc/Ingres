/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** systypes.h -- wrapper for sys/types.h
**
** Some of the things in sys/types.h are defined in <compat.h>, so if
** if is included later it will not compile.  By including this file,
** we can get the sys/types.h definitions either before or after including
** <compat.h>
**
** History:
**	07-oct-88 (daveb)
**		Split from compat.h
**	13-mar-89 (russ)
**		Added defines for unsigned types.  These defines are
**		duplicated in compat.h;  however, they must be undef'ed
**		before including sys/types.h, or they may cause syntax
**		errors therein.
**	18-jan-91 (mikem/fredp)
**		Added #ifdef increased the number of per-process file
**		descriptors on running an SunOS4.1/SunDBE OS.  We 
**		pre-define FD_SETSIZE before including <sys/types.h> so that
**		the macros and structure definitions will reflect the increased
**		number of file descriptors.
**	09-aug-91 (seng)
**		The IBM RS/6000 has an element in a structure which has the
**		name "reg" defined in <sys/m_types.h>.  Thus the statement
**			"#define reg register"
**		will cause many problems.  Undefine "reg" before including
**		<sys/types.h> (which includes <m_types.h>) and redefine it 
**		after.
**	28-apr-93 (sweeney)
**		Apparently the #define of "reg" to "register" is
**		redundant. (c.f. mail from daveb). So get rid of it.
**		It has been known to cause problems with choice
**		of variable names.
*/

# ifndef SYSTYPES_H_INCLUDED
# define SYSTYPES_H_INCLUDED

# undef uchar
# undef u_char
# undef ushort
# undef u_short
# undef ulong
# undef u_long

# ifdef	xCL_083_USE_SUN_DBE_INCREASED_FDS
# define	SUNDBE_NOFILE	1024
# define	FD_SETSIZE	SUNDBE_NOFILE
# endif

# include <sys/types.h>

# define	uchar	unsigned char
# define	u_char	unsigned char
# define	ushort	unsigned short
# define	u_short	unsigned short
# define	ulong	unsigned long
# define	u_long	unsigned long

# endif
