/*
** EQBAS.H - Information required only by Equel/BASIC.
**
** Defines BASIC type and error constants.
**
** History:	23-jan-1986 - Written (bjb)
**		02-aug-1990 - Modified for decimal (teresal)
**	14-jan-1993 (lan)
**		Added a constant for the datahandler.
**	16-feb-2000 (kinte01)
**		Add support for SFLOAT, TFLOAT for AlphaVMS
**		SINGLE & DOUBLE are not supported AlphaVMS. (SIR 100394)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Sizes of BASIC data types
*/
# define	BszBYTE		1
# define	BszWORD		2
# define	BszLONG		4
# define	BszSFLT		4
# define	BszTFLT		8
# define	BszSING		4
# define	BszDOUB		8
# define	BszDPREC	15	/* Default size	for Decimal (15,2) */
# define	BszDSCALE	2

/* 
** Is this type a numeric?
*/
# define	Bis_numeric( x ) ( x == T_INT || x == T_FLOAT || x == T_PACKED )

/* 
** Class of BASIC declaration
*/
# define	BclVAR		1		/* DECLARE variable */
# define	BclCONSTANT	2		/* DECLARE constant */
# define	BclEXVAR	3		/* EXTERNAL variable */
# define	BclEXCONS	4		/* EXTERNAL constant */
# define	BclSTATIC	5		/* COMMON/MAP statement */
# define	BclDIMS		6		/* DIMENSION statement */
# define	BclRECORD	7		/* RECORD statement */
# define	BclEXTERN	8		/* EXTERN statement */
# define	BclDEFFUNC	9		/* DEF function definition */
# define	BclSUB		10		/* FUNCTION or SUB subprogram */

/* 
** B_dec flags in gr structure
*/
# define	BdecUSE		0		/* Look up as a type */
# define	BdecDEC		1 		/* Look up as a variable */

/*
** B_seendec flags in gr strcuture
*/
# define	BingNOSEEN	0		/* ## decl ingres not seen */
# define	BingMESG	1		/* Shut up error messages */
# define	BingSEEN	2		/* ## decl ingres seen */

/*
** B_dims flags in gr structure
*/
# define	BdimsDEF	0		/* Default */
# define	BdimsNEED	1		/* Subscripts were required */
# define	BdimsEXTRA	2		/* Subscripts werent required */

/*
** Block level indicators -- used for B_blk field in gr struct
*/
# define	BlevNONE	0		/* Not in main or sub */
# define	BlevPROG	1		/* In main or sub/func */
# define	BlevINDEF	2		/* In a DEF func */

/*
** What kind of module we're in -- used for B_rtn in gr struc
*/
# define	BinNONE		0x00		/* Not in anything yet */
# define	BinMAIN		0x01		/* In (implicit) main */
# define	BinFUNC		0x02		/* In a FUNCTION */
# define	BinSUB		0x04		/* In a SUBROUTINE */
# define	BinDEF		0x10		/* In a nested DEF */

/*
** Constants to be used by gen_io() to tell what kind of information should
** be written together with the variable.
*/
# define	G_SET		0			/* Set */
# define	G_RET		1			/* Return */
# define	G_RETSET	2	/* Format of Set, but really Return */ 
# define	G_DAT		3	/* Datahandler */

/*
** Symbol table name space indices
**	- 0 is the default so it is never explicitly called
*/
#   define	SY_NORMAL	0

/* Special "pinned indent" value for gen_do_indent */
# define G_IND_PIN	(-1000)
# define G_IND_RESET	(-1001)	/* reset to left margin for initialization */

/*
** Table of II call structure - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.
*/

typedef struct {
    char	*g_IIname;				/* Name of function */
    i4		(*g_func)();				/* Argument dumper  */
    i4		g_flags;				       /* Arg flags */
    i4		g_numargs;				       /* How many  */
} GEN_IIFUNC;

/* Closure is always 0 */
# define	BSCLOSURE	0
