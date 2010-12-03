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
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN VOID DSbegin(
	    FILE	*file, 
	    i4		lang,
	    char	*name
);
FUNC_EXTERN STATUS DSclose(
	    SH_DESC	*sh_desc
);
FUNC_EXTERN VOID DSend(
	    FILE	*file, 
	    i4		lang
);
FUNC_EXTERN VOID DSfinal(
	    void
);
FUNC_EXTERN STATUS DSinit(
	    FILE	*file, 
	    i4		lang, 
	    i4		align, 
	    char	*lab, 
	    i4		vis, 
	    char	*type
);

/* DSwrite should be:
** DSwrite(i4, anything, i4)
** except that the "anything" really can be anything, including NOT
** pointers.  The call ought to be redone so as to always pass a true
** pointer to a value, not sometimes a pointer and sometimes a value
** and sometimes who knows what.  Until then, avoid prototyping,
** because it might confuse the compiler.  (kschendel)
*/
FUNC_EXTERN STATUS DSwrite();
