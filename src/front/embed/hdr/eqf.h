/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:    eqf.h
** Purpose:     Define constants that are known only to the FORTRAN dependent
-*              parts of the EQUEL and ESQL grammars.
** History:
**	08-dec-1992 (lan)
**		Added a constant for the datahandler.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** EQF - Define constants that are FORTRAN dependent.
*/
	/* Private data types - known only by FORTRAN */
#   define	T_TYPE		2
#   define	T_TYPEDEF 	3
#   define	T_EXTERN	4


/* ## declare flags (F_usedec) */
#   define	FDEC_NONE	0		/* no ## declare */
#   define	FDEC_DECLARE	2		/* ## declare */
#   define	FDEC_FORMS	6		/* ## declare forms */
#   define	FDEC_BLOCK	1		/* ## dec in block - OR'ed in */

    /* F_dec flags for FORTRAN */
#   define	GR_F_USE	0		/* Lookup as a variable */
#   define	GR_F_DEC	1		/* Lookup as a TYPE */
#   define	GR_F_TAG	2		/* Lookup as a struct TAG */

    /*
    ** Symbol table name space indices
    **	- 0 is the default so it is never explicitly called
    **	- 1 is FORTRAN tag name space (nobody else will need to know these
    **	  names)
    */
#   define	SY_NORMAL	0
#   define	SY_TAG		1


    /* FORTRAN closure is always 0 (no nested scopes) */
# define	F_CLOSURE	0

    /* 
    ** Most F77 compilers treat backslash as an escape character, like
    ** the C-compiler.  We must compensate for this when generating
    ** string constants.
    */
# ifdef UNIX
# define F77ESCAPE
# endif /* UNIX */

# ifdef CMS
#   define	EQ_ADD_FNAME
# endif
     
/*
** Constants to be used by gen_io() to tell what kind of information should
** be written together with the variable.
*/

# define	G_NONE		0			/* No flag */
# define	G_SET		1			/* Set */
# define	G_RET		2			/* Return */
# define	G_RETSET	3		/* Really a Return, but looks
						** like a Set with respect that
						** the non-io-args are BEFORE
						** the io-arg rather than after.
						** Used for ** PROMPT. */
# define	G_NAME		4			/* By Name */
# define	G_RAW		5			/* Takes Raw Args */
# define	G_DAT		6		/* Datahandler */

/* "how" for gen__int and gen__float */

# define	FbyNULL		0	/* unused */
# define	FbyNAME		1	/* unadorned */
# define	FbyVAL		2	/* %val(%s) */
# define	FbyREF		3	/* %s */

/*
** Table of II calls - 
**     We use similar logic in FORTRAN as in C.
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments. 
*/

typedef struct {
    char	*g_IIname;				/* Name of function */
    i4		(*g_func)();				/* Argument dumper  */
    i4		g_flags;				/* Arg flags */
    i4		g_numargs;				/* How many  */
} GEN_IIFUNC;
