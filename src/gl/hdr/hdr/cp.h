/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef CP_HDR_INCLUDED
# define CP_HDR_INCLUDED 1
#include    <cpcl.h>

/**CL_SPEC
** Name:	CP.h	- Define CP function externs
**
** Specification:
**
** Description:
**	Contains CP function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	24-jan-1994 (gmanning)
**	    Enclose CPopen() in ifndef.
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of cp.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#ifndef CPclose
FUNC_EXTERN STATUS CPclose(
#ifdef	CL_PROTOTYPED
	CPFILE		*CPfile
#endif
);
#endif

#ifndef CPflush
FUNC_EXTERN STATUS CPflush(
#ifdef	CL_PROTOTYPED
	CPFILE		*CPfile
#endif
);
#endif

#ifndef CPgetc
FUNC_EXTERN i4  CPgetc(
#ifdef	CL_PROTOTYPED
	CPFILE		*CPfile
#endif
);
#endif

#ifndef CPopen
FUNC_EXTERN STATUS CPopen(
#ifdef	CL_PROTOTYPED
	LOCATION	*loc,
	char		*file_mode,
	i2		file_type,
	i4		rec_length,
	CPFILE		*CPfile
#endif
);
#endif

#ifndef CPread
FUNC_EXTERN STATUS CPread(
#ifdef	CL_PROTOTYPED
	CPFILE		*CPfile,
	i4		size,
	i4		*count,
	char		*pointer
#endif
);
#endif

#ifndef CPwrite
FUNC_EXTERN STATUS CPwrite(
#ifdef	CL_PROTOTYPED
	i4		size,
	char		*pointer,
	i4		*count,
	CPFILE		*CPfile
#endif
);
#endif

# endif /* ! CP_HDR_INCLUDED */
