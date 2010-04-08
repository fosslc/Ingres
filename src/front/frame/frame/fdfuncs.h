/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	functions.h
*/

/*
**	functions.h -- function declarations for frame functions
**
**	History:
**		19-jun-87 (bab)
**			changed types of SI[f]printf to VOID to match CL spec.
**		25-jun-87 (bab)
**			added several function defs to shut up lint.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN i4			FDgetdata();
FUNC_EXTERN FLDHDR		*FDgethdr();
FUNC_EXTERN FLDTYPE		*FDgettype();
FUNC_EXTERN FLDVAL		*FDgetval();
FUNC_EXTERN i4			FDmove();
FUNC_EXTERN i4			FDcurdisp();
FUNC_EXTERN i4			FDqrydata();
FUNC_EXTERN VOID		FDbadmem();
FUNC_EXTERN VOID		FDerror();
FUNC_EXTERN VOID		FDFTsetiofrm();
FUNC_EXTERN VOID		FDFTsmode();
FUNC_EXTERN VOID		FDftbl();
FUNC_EXTERN VOID		FDdmpmsg();
FUNC_EXTERN VOID		FDdmpcur();

# ifdef lint
/* VARARGS */
FUNC_EXTERN VOID		SIprintf();
/* VARARGS */
FUNC_EXTERN VOID		SIfprintf();
/* VARARGS */
FUNC_EXTERN char		*STprintf();
# endif
