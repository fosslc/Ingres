/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	eqc.h
** Purpose:	Define constants that are known only to the C dependent
-*		parts of the EQUEL and ESQL grammars.
** History:
**		10-jan-1985	- Written for EQUEL (ncg)
**		14-aug-1985	- Updated for ESQL, moved to common hdr (ncg)
**		09-nov-1992	- Added a constant for Datahandler. (lan)
**	18-Dec-97 (gordy)
**	    Added global SQLCA host var reference.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** EQC - Define constants that are C dependent.
*/
	/* Private data types - known only by C */
#   define	T_TYPE		2
#   define	T_TYPEDEF 	3
#   define	T_EXTERN	4

    /* C_dec flags for C */
#   define	GR_C_USE	0		/* Lookup as a variable */
#   define	GR_C_DEC	1		/* Lookup as a TYPE */
#   define	GR_C_TAG	2		/* Lookup as a struct TAG */

    /*
    ** Symbol table name space indices
    **	- 0 is the default so it is never explicitly called
    **  - 1 C tag name space (nobody else will need to know these names)
    */
#   define	SY_NORMAL	0
#   define	SY_TAG		1

    /* Symbol table block and closure (if fixed, as in ESQL) */
# define	C_CLOSURE	0
# define	C_BLOCK		1

/*}
+*  Name: C_VALS - Maintains C symbol table value field.
**
**  Description:
**	This structure takes care of obscure C-type symbol table requirements.
**	1. When a variable is declared as a function, the the cv_flags are
**	   set, otherwise they are reset.
**	2. When a dimensioned buffer is used the dimension string is stored
**	   as a null terminated string in the bogus array.  This dimension
**	   is used for code generation in order to attempt to keep buffer
**	   limits at run-time (became a problem when we upped the size of our
**	   catalog fields).
-*
**  History:
**	20-mar-1987	- Written for 6.0 conversion. (ncg)
**	01-nov-1992	- Added define for RETVAR for GET_DATA stmt. (kathryn)
**	01-oct-1993 (kathryn)
**	    Added define for G_PUT.
*/

typedef struct {
    i1		cv_flags;
# define	  CvalDEFAULT  (i1)0
# define	  CvalFUNCVAR  (i1)1	/* Function variable */
# define	  CvalDIMS     (i1)2	/* Dimension string */
    char	cv_dims[1];		/* Bogus null terminated string - at
					** least a single null byte */
} C_VALS;

/*
** C gen stuff -- known to both EQUEL and ESQL code generators
*/

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For C there is very little
** argument information needed, as will probably be the case for regular 
** callable languages. For others (Cobol) this table will need to be more 
** complex and the code may be split up into partially table driven, and 
** partially special cased.
**
** This is the ESQL-specific part of the table
*/

typedef struct {
    char	*g_IIname;				/* Name of function */
    i4		(*g_func)();				/* Argument dumper  */
    i4		g_flags;				       /* Arg flags */
    i4		g_numargs;				       /* How many  */
} GEN_IIFUNC;

/*
** Constants to be used by gen_io() to tell what kind of information should
** be written together with the variable.
*/
# define	G_RET		0		/* Return */
# define	G_SET		1		/* Set */
# define	G_RETSET	2		/* Really a Return, but looks
						** like a Set.  Used for 
						** PROMPT. */
# define	G_RETVAR	3		/* A Return with other args
						** passed by reference.
						*/
# define	G_DAT		4		/* Datahandler */
# define	G_PUT		5		/* A Put Data stmt */

/*
** Globals
*/

GLOBALREF char	*sqlca_var;

