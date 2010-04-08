/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufg4L.c - lower-case version of iiufg4.c
**
** Description:
**	This file includes iiufg4.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	18-aug-1993 (lan)
**	    Created for EXEC 4GL.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iig4ac_ iig4ac
# define	iig4rr_ iig4rr
# define	iig4ir_ iig4ir
# define	iig4dr_ iig4dr
# define	iig4sr_ iig4sr
# define	iig4gr_ iig4gr
# define	iig4gg_ iig4gg
# define	iig4sg_ iig4sg
# define	iig4ga_ iig4ga
# define	iig4sa_ iig4sa
# define	iig4ch_ iig4ch
# define	iig4ud_ iig4ud
# define	iig4fd_ iig4fd
# define	iig4ic_ iig4ic
# define	iig4vp_ iig4vp
# define	iig4bp_ iig4bp
# define	iig4rv_ iig4rv
# define	iig4cc_ iig4cc
# define	iig4i4_ iig4i4
# define	iig4s4_ iig4s4
# define	iig4se_ iig4se

# include "iiufg4.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufg4L_dummy;

# endif /* MIXEDCASE_FORTRAN */
