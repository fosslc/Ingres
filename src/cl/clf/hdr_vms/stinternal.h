/*
** Copyright (c) 1992, 1998 Ingres Corporation
**			    All rights reserved.
*/

/*
** Name: stinternal.h - internal ST header file.
**
** Description:
**      Contains declarations and definitions of ST functions that are 
**	referenced but not part of the CL spec.
**
** History
**	01-dec-1992 (pearl)
**	    Created.
**      16-Jun-98 (kinte01)
**          changed return type of STiflush to VOID instead
**          of int as the routine does not return anything. Fixes
**          DEC C 5.7 compiler warning
**
*/

#ifdef CL_BUILD_WITH_PROTOS
FUNC_EXTERN VOID
STiflush(
        char    c,
        FILE    *f);
#else
FUNC_EXTERN VOID STiflush();
#endif

