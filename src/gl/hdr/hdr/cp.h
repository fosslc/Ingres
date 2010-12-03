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
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

#ifndef CPclose
FUNC_EXTERN STATUS CPclose(
	CPFILE		*CPfile
);
#endif

#ifndef CPflush
FUNC_EXTERN STATUS CPflush(
	CPFILE		*CPfile
);
#endif

#ifndef CPgetc
FUNC_EXTERN i4  CPgetc(
	CPFILE		*CPfile
);
#endif

#ifndef CPopen
FUNC_EXTERN STATUS CPopen(
	LOCATION	*loc,
	char		*file_mode,
	i2		file_type,
	i4		rec_length,
	CPFILE		*CPfile
);
#endif

#ifndef CPread
FUNC_EXTERN STATUS CPread(
	CPFILE		*CPfile,
	i4		size,
	i4		*count,
	char		*pointer
);
#endif

#ifndef CPwrite
FUNC_EXTERN STATUS CPwrite(
	i4		size,
	char		*pointer,
	i4		*count,
	CPFILE		*CPfile
);
#endif

# endif /* ! CP_HDR_INCLUDED */
