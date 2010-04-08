/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	functions.h -- function declarations for frame functions
**
**	History:
**	25-jun-87 (bab)	Changed defs of SI[f]printf to VOID.
*/

# include	<kst.h>

FUNC_EXTERN KEYSTRUCT	    *FTgetc();
FUNC_EXTERN KEYSTRUCT	    *FTgetch();
FUNC_EXTERN KEYSTRUCT	    *FTTDgetch();

# ifdef lint
/* VARARGS */
FUNC_EXTERN VOID		SIprintf();
/* VARARGS */
FUNC_EXTERN VOID		SIfprintf();
/* VARARGS */
FUNC_EXTERN char		*STprintf();
# endif
