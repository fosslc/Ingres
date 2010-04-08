/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		UTerr.h -- UT error codes
**
**	History:
**		???? - first written
**		Sept. 90 (bobm) - integrated compiler/link warning
**			and fail codes, added comment block.
*/

# define	UT_ED_CALL	3600	/* UTedit: NULL filename argument  */
# define	UT_PR_CALL	3630	/* UTprint: NULL filename argument  */
# define	UT_CO_FILE	3660	/* UTcompile: couldn't open UTcom.def*/
# define	UT_CO_IN	3661	/* UTcompile: input file doesn't exist*/
# define	UT_CO_SUF	3662	/* UTcsuffix: bad suffix */
# define	UT_CO_LINES	3663	/* UTcsuffix: out of lines */
# define	UT_CO_CHARS	3664	/* UTcsuffix: out of chars */
# define	UT_EXE_DEF	3665	/* Couldn't open UTexe */
# define	UT_LD_DEF	3666	/* Couldn't open utld.def */
# define	UT_LD_OBJS	3667	/* Too many objects */
# define	UT_CMP_WARNINGS	3668	/* Compiled with warnings */
# define	UT_CMP_FAIL	3669	/* Compilation failed */
# define	UT_LNK_WARNINGS	3668	/* Linked with warnings */
# define	UT_LNK_FAIL	3669	/* Link failed */
