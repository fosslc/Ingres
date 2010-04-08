/*
** Copyright (c) 1993, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

/*
** Name: meactual.c
**
** History
**	18-jan-1993 (pearl)
**	    Add copyright statement.
**          Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**          headers.
**	08-feb-1993 (pearl)
**	    Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	11-aug-93 (ed)
**	    unconditional prototypes
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	08-feb-2001 (somsa01)
**	    Modified type of i_meactual and i_meuser to be of SIZE_TYPE.
*/

GLOBALREF SIZE_TYPE i_meactual;
GLOBALREF SIZE_TYPE i_meuser;

void
MEactual(
	SIZE_TYPE	*user,
	SIZE_TYPE	*actual)
{
    *user = i_meuser;
    *actual = i_meactual;
}
