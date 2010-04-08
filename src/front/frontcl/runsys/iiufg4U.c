/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufg4U.c - upper-case version of iiufg4.c
**
** Description:
**	This file includes iiufg4.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	18-aug-1993 (lan)
**	    Created for EXEC 4GL.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iig4ac_ IIG4AC
# define	iig4rr_ IIG4RR
# define	iig4ir_ IIG4IR
# define	iig4dr_ IIG4DR
# define	iig4sr_ IIG4SR
# define	iig4gr_ IIG4GR
# define	iig4gg_ IIG4GG
# define	iig4sg_ IIG4SG
# define	iig4ga_ IIG4GA
# define	iig4sa_ IIG4SA
# define	iig4ch_ IIG4CH
# define	iig4ud_ IIG4UD
# define	iig4fd_ IIG4FD
# define	iig4ic_ IIG4IC
# define	iig4vp_ IIG4VP
# define	iig4bp_ IIG4BP
# define	iig4rv_ IIG4RV
# define	iig4cc_ IIG4CC
# define	iig4i4_ IIG4I4
# define	iig4s4_ IIG4S4
# define	iig4se_ IIG4SE

# include "iiufg4.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufg4L_dummy;

# endif /* MIXEDCASE_FORTRAN */
