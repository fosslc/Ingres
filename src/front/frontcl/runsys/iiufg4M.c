/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufg4M.c - lower-case version of iiufg4.c
**
** Description:
**	This file includes iiufg4.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	18-aug-1993 (lan)
**	    Created for EXEC 4GL.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iig4ac_ IIG4ac
# define	iig4rr_ IIG4rr
# define	iig4ir_ IIG4ir
# define	iig4dr_ IIG4dr
# define	iig4sr_ IIG4sr
# define	iig4gr_ IIG4gr
# define	iig4gg_ IIG4gg
# define	iig4sg_ IIG4sg
# define	iig4ga_ IIG4ga
# define	iig4sa_ IIG4sa
# define	iig4ch_ IIG4ch
# define	iig4ud_ IIG4ud
# define	iig4fd_ IIG4fd
# define	iig4ic_ IIG4ic
# define	iig4vp_ IIG4vp
# define	iig4bp_ IIG4bp
# define	iig4rv_ IIG4rv
# define	iig4cc_ IIG4cc
# define	iig4i4_ IIG4i4
# define	iig4s4_ IIG4s4
# define	iig4se_ IIG4se

# include "iiufg4.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufg4L_dummy;

# endif /* MIXEDCASE_FORTRAN */
