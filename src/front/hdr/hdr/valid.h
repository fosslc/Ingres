/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	VALID.H
**	contains the global structures and variable declarations needed
**	by the lexical analyzer.  This includes Manifest Constants and
**	certain variables for internal communication purposes. Thus,
**	extreme care should be exercised when modifying this file.
**
**  History:
**	04/04/87 (dkh) - Added support for ADTs.
**	09/05/87 (dkh) - Moved error numbers to erfi.msg.
**	11/20/87 (dkh) - Changed handling of LIKE operator.
**	06/23/89 (dkh) - Added support for derived fields.  Specifically
**			 changed the YYSTYPE structure name to be
**			 IIFVvstype so that it is possible to have
**			 two distinct parsers in the forms system.
**			 Also, a side benefit is that the change will
**			 allow a user to also include a yacc parser
**			 in their forms application.
**	06/11/92 (dkh) - Added support for decimal datatype for 6.5.
**	10/28/92 (dkh) - Changed ALPHA to VL_ALPHA for compiling on DEC/ALPHA.
**	30-2ep-1996 (canor01)
**	    Replace 'extern' with GLOBALREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* MANIFEST CONSTANTS */
# define	MAXSTRING	255	/* max length of strings */
# define	GOVAL		-1	/* semantic value for command names */

# ifndef	WARN
# define	WARN		0
# define	FATAL		1
# define	NOACTION	2
# endif

/* CHARACTER TYPES */
# define	VL_ALPHA	1
# define	NUMBR		2
# define	OPATR		3
# define	PUNCT		4
# define	CNTRL		5

/* DATA TYPES */
# define	vNONE		(i4)(-1)	/* special error type */
# define	vLOGICAL	(i4)1
# define	vSTRING		(i4)2
# define	vINT		(i4)3
# define	vFLOAT		(i4)4
# define	vFLOAT4		(i4)5		/*  table lookup of f4s
						**  for bug 4034 (dkh)
						*/
# define	vVCHSTRING	(i4)6
# define	vDEC		(i4)7

/* NODE TYPES */
# define	vUOP		(i4)1
# define	vBOP		(i4)2
# define	vCONST		(i4)3
# define	vATTR		(i4)4
# define	vLIST		(i4)5
# define	vCOL		(i4)6
# define	vCOLSEL		(i4)7

/*
** type checking contexts
** see type.c
*/
# define	vNONSTRICT	01
# define	vLR		02

/* YYSTYPE structure for yacc stack, yyval, and yylval */
typedef union
{
	i4		type_type;
	struct valroot	*root_type;
	VTREE		*tree_type;
	i4		I4_type;
	f8		F8_type;
	DB_TEXT_STRING	*string_type;
	char		*name_type;
	DB_DATA_VALUE	*dec_type;
} IIFVvstype;

/* SPECIAL TOKENS for scanner */
struct special
{
	i2	sconst;
	i2	i4const;
	i2	f8const;
	i2	name;
	i2	lparen;
	i2	rparen;
	i2	lbrak;
	i2	rbrak;
	i2	period;
	i2	comma;
	i2	relop;
	i2	uminus;
	i2	lbop;
	i2	luop;
	i2	in;
	i2	is;
	i2	not;
	i2	null;
	i2	like;
	i2	svconst;
	i2	decconst;
};

/* declarations */
GLOBALREF struct special	Tokens;			/* special tokens table */
GLOBALREF char		*Lastok;		/* last token read in */
GLOBALREF i2		Opcode;			/* opcode for current token */
GLOBALREF bool		Newline;		/* last char read = newln */

GLOBALREF FRAME	*vl_curfrm;
GLOBALREF FLDHDR	*vl_curhdr;
GLOBALREF FLDTYPE	*vl_curtype;
GLOBALREF FLDVAL	*vl_curval;
GLOBALREF bool	vl_syntax;		/* check syntax only */
GLOBALREF char	*vl_buffer;

/*
**	LEXICAL CODES
*/

# define	v_opAND		1
# define	v_opOR		2
# define	v_opNOT		3
# define	v_opEQ		4
# define	v_opNE		5
# define	v_opLT		6
# define	v_opLE		7
# define	v_opGT		8
# define	v_opGE		9
# define	v_opDOT		10		/* column name */
# define	v_opMINUS	11
# define	v_opNULL	12
# define	v_opNOTNL	13
# define	v_opLK		14

/*
**	TRACE ARGUMENTS
*/

/*	Evaluation trace	*/
# define	veALL		1
# define	veWHERE		2
# define	veLIST		3
# define	veOP		4
# define	veINT		5
# define	veFLOAT		6
# define	veSTRING	7

/*
**  Definition for NULL value used in
**  validation evaluation.
*/
# define	V_NULL		2
