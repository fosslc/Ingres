/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	eqpas.h
** Purpose:	Define constants that are known only to the PASCAL-dependent
-*		parts of the EQUEL and ESQL grammars.
** History:
**		10-jan-1985	- Written for EQUEL (ncg)
**		14-aug-1985	- Updated for ESQL, moved to common hdr (ncg)
**	14-jan-1993 (lan)
**		Added a constant for the datahandler.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** EQP - Define constants that are PASCAL dependent.
*/
	/* Private data types - known only by PASCAL */
#   define	T_TYPE		2
#   define	T_TYPEDEF 	3
#   define	T_EXTERN	4

    /* P_var flags for PASCAL */
#   define	PvarUSE		0		/* Lookup as a variable */
#   define	PvarDEC		1		/* Lookup as a TYPE */
#   define	PvarTAG		2		/* Lookup as a struct TAG */

    /* P_mode flags for PASCAL */
#   define	PmodNONE	0		/* not accepting any just now */
#   define	PmodLABEL	1		/* in LABEL section */
#   define	PmodCONST	2		/* in CONST section */
#   define	PmodTYPE	3		/* in TYPE section */
#   define	PmodVAR		4		/* in VAR section */

/* flag values -- for gr->P_val and sym->st_value -- from attributes */
#   define	PvalBYTE	0x001
#   define	PvalSHORT	0x002
#   define	PvalLONG	0x004
#   define	PvalQUAD	0x008
#   define	PvalSIZES	(PvalBYTE|PvalSHORT|PvalLONG|PvalQUAD)
#   define	PvalVARYING	0x010
#   define	PvalPACKED	0x020
#   define	PvalUNSUPP	0x040	/* PASCAL type SET or QUADRUPLE */
#   define	PvalINHERIT	0x080	/* [INHERIT] attribute */
#   define	PvalEQUEL	0x100	/* [INHERIT('EQUEL')] attribute */
#   define	PvalESQL	0x200	/* [INHERIT('ESQL')] attribute */
#   define	PvalINHQUEL	(PvalINHERIT|PvalEQUEL)
#   define	PvalINHSQL	(PvalINHERIT|PvalESQL)
#   define	PvalSTRPTR	0x400	/* Type STRINGPTR */

    /*
    ** Symbol table name space indices
    **	- 0 is the default so it is never explicitly called
    **  - 1 C tag name space (nobody else will need to know these names)
    */
#   define	SY_NORMAL	0
#   define	SY_TAG		1

    /* Symbol table block and closure (if fixed, as in ESQL) */
# define	P_CLOSURE	0
# define	P_BLOCK		1

    /* function return types */
SYM	*pasWthLookup();
void	pasLbInit(), pasLbProc(), pasLbLabel();
i4  	pasLbFile();

/*
** PASCAL gen stuff -- known to both EQUEL and ESQL code generators
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
# define	G_TRIM		2		/* If string var then trim */
# define	G_RETSET	3		/* Really a Return, but looks
						** like a Set with respect that
						** the non-io-args are BEFORE
						** the io-arg rather than after.
						** Used for ** PROMPT. */
# define	G_DAT		4		/* Datahandler */

/* Special "pinned indent" value for gen_do_indent */
# define G_IND_PIN	(-1000)
# define G_IND_RESET	(-1001)	/* reset to left margin for initialization */

/* Flags to be used with gen_sdesc for coding PASCAL string descriptors */
# define 	PsdCONST	(-1)		/* String constant */
# define 	PsdNONE		0		/* No function call */
# define 	PsdREG		1		/* Via IIsd */
# define 	PsdNOTRIM	2		/* Via IIsd_no[trim] */

/*
** Constant to be used when generating the SQLDA as an argument
** This string is the format to STprintf.
*/
# ifdef VMS
# 	define	PAS_SQDA_ARG	ERx("%%ref %s")
# else
# 	define	PAS_SQDA_ARG	ERx("addr(%s)")	/* ? */
# endif /* VMS */
