/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufutilM.c - mixed-case version of iiufutil.c
**
** Description:
**	This file includes iiufutil.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	09-dec-1991 (rudyw)
**		Add dummy renames for three routines which are already
**		mixed case in the file which this file includes (iiufutil.c).
**		The purpose is to avoid having these routines defined in
**		this routine exactly as they are in iiufutil.c cuz this
**		causes name conflict during linking with libingres.a.
**		The routines will now be found during linking in iiufutil.o
**		and not this wrapper file which only maps names to mixed case.
**		Introducing mixed case routines into the file iiufutil.c
**		was an inconsistency which causes problems in this module.
**	16-dec-1991 (rudyw)
**		Back out previous change due to a rejection. An alternative
**		to a dummy rename of IIxintrans, IIxouttrans, and II_sdesc
**		under the MIXED_CASE option is to extract iiufutil.o from
**		the deliverable libingres.a.
**      02-aug-1995 (morayf)
**              Added iips_ define for donc change of 17-dec-1993.
**      21-May-1997 (allmi01)
**              Added dummy renames for three routines, IIxintrans,
**              IIxouttrans, and II_sdesc, for SCO OpenServer (sos_us5)
**              to eliminate fatal error of multiply defined symbols.
**		(x-integ from 1.2/01 port)
*/
# ifndef NT_GENERIC

# ifdef MIXEDCASE_FORTRAN
# define	iinum_		IInum
# define	iips_		IIps
# define	iistr_		IIstr
# define	iisd_		IIsd
# define	iisdno_		IIsdno
# define	iislen_		IIslen
# define	iisadr_		IIsadr

# ifdef sos_us5
# define        II_sdesc        II_sdesc_dummy
# define        IIxouttrans     IIxouttrans_dummy
# define        IIxintrans      IIxintrans_dummy
# endif

# include "iiufutil.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufutilL_dummy;

# endif /* MIXEDCASE_FORTRAN */

# endif /* NT_GENERIC */
