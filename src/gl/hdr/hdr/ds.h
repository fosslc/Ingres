/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <dscl.h>

/**CL_SPEC
** Name:	DS.h	- Define DS function externs
**
** Specification:
**
** Description:
**	Contains DS function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	19-jun-1997 (kosma01)
**	    CL_PROTOTYPED is now set as a default define in
**	    cl/hdr/hdr/compat.h. This causes the AIX compiler
**	    to flag the DSwrite function anytime it is called
**	    with an argument that is not of type char *.
**	    The AIX compiler will not allow a function argument
**	    to be of type integer, (or i1, i2, i4, i4, f4, f8,
**	    char), when the prototype specifies a char *. 
**	    Instead of casting all offending arguments, remove
**	    the prototype for the dswrite function.
**	    The compile errors appear in directory 
**	    front/frame/compfrm and in file front/abf/abf/abftab.c
**      23-jun-99 (hweho01)
**          Extended the above change to ris_u64 (AIX 64-bit platform). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jun-2009 (kschendel) SIR 122138
**	    include r64_us5.
**/

FUNC_EXTERN VOID DSbegin(
#ifdef CL_PROTOTYPED
	    FILE	*file, 
	    i4		lang,
	    char	*name
#endif
);
FUNC_EXTERN STATUS DSclose(
#ifdef CL_PROTOTYPED
	    SH_DESC	*sh_desc
#endif
);
FUNC_EXTERN VOID DSend(
#ifdef CL_PROTOTYPED
	    FILE	*file, 
	    i4		lang
#endif
);
FUNC_EXTERN VOID DSfinal(
#ifdef CL_PROTOTYPED
	    void
#endif
);
FUNC_EXTERN STATUS DSinit(
#ifdef CL_PROTOTYPED
	    FILE	*file, 
	    i4		lang, 
	    i4		align, 
	    char	*lab, 
	    i4		vis, 
	    char	*type
#endif
);
FUNC_EXTERN STATUS DSwrite(
#ifdef CL_PROTOTYPED
# if !defined(any_aix)
	    i4		type, 
	    char	*val, 
	    i4		len
# endif  /* not aix */
#endif
);
