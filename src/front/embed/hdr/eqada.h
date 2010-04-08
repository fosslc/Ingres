/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	eqada.h
** Purpose:	Define constants that are known only to the ADA-dependent
-*		parts of the EQUEL and ESQL grammars.
** History:
**		10-jan-1985	- Written for EQUEL/C (ncg)
**		01-nov-1985	- Modified for EQUEL/PASCAL (mrw)
**		10-mar-1986	- Adapted to EQUEL/ADA (ncg, mrw)
**		18-mar-1988	- Modified for VERDIX ADA on UNIX (russ)
**		03-oct-1989	- Modified for dynamic configuartion (ncg)
**		08-dec-1992 (lan)
**			Added a constant for the datahandler.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** DCLGEN also includes this file, so we lock out all constants 
** that are not needed by DCLGEN.
*/

#ifndef DCLGEN_EXCLUDE

/*
** ADA language dependent constants.
*/

/* A_var flags to decide how to lookup a variable via GR_LOOKUP */
#   define	AvarDEC		0	/* Inside a declaration */
#   define	AvarUSE		1	/* Lookup as a variable usage */

/* A_with values that signal WITH EQUEL[_FORMS] has been issued */
#   define	AwithNONE	00	/* Nothing yet */
#   define	AwithEQUEL	01	/* ## with EQUEL */
#   define	AwithFORMS	02	/* ## with EQUEL_FORMS */

/* A_val flags for ADA (used with sym->st_value) */
#   define	AvalDEF		000	/* Default */
#   define	AvalENUM	001	/* Enumerated type */
#   define	AvalUNSUPP	002	/* Unsupported type */
#   define	AvalACCESS	004	/* Access type */
#   define	AvalCHAR 	010	/* Char type */

/* ADA symbol table block and closure (if fixed, as in ESQL) */
# define	A_CLOSURE	0
# define	A_BLOCK		1

/* Symbol table name space indices */
#   define	SY_NORMAL	0  /* Regular name space */
#   define	SY_TAG		1  /* Tag space (init anonymous record entry) */


/*
** ADA code generator stuff -- known to both EQUEL and ESQL code generators
*/

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments. 
*/

typedef struct {
    char	*g_IIname;			/* Name of function */
    i4		(*g_func)();			/* Argument dumper  */
    i4		g_flags;			/* Argument flags */
    i4		g_numargs;			/* How many arguments  */
} GEN_IIFUNC;

/*
** Constants to be used by gen_io and gen_data to tell what kind of
** information should be written together with the variable.
*/
# define	G_RET		0		/* Return */
# define	G_SET		1		/* Set */
# define	G_TRIM		2		/* If string var then trim */
# define	G_RETSET	3		/* Ret arg in Set position */
# define	G_SQLDA		4		/* Null SQLDA argument */
# define	G_DAT		5		/* Datahandler */

/* Special "pinned indent" value for gen_do_indent */
# define G_IND_PIN	(-1000)
# define G_IND_RESET	(-1001)	/* Reset to left margin for initialization */

#endif /* DCLGEN_EXCLUDE */

/*
** ADA configuration dependent stuff.
*/

/*
** Structure: GR_ADTYPE
**
** Description: 
**	This structure is from GR_TYPE with the extension of a flags field.
**	The gt_flags fields allows dynamic insertions of the different types
**	for the different compiler vendors.
*/
typedef struct {
    char	*gt_id;			/* Name of type */
    i2		gt_rep;			/* Constant representation */
    i2		gt_len;			/* Length of type */
    i4	gt_flags;		/* EQ_ADVADS or EQ_ADVAX or both */
} GR_ADTYPES;

/*
** Constant to be used when generating the SQLDA as an argument
** This string is the format to STprintf.
*/
# define ADA_SQLDA_ARG	ERx("%s'Address")
