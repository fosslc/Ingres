/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ugexpr.h - header file for boolean expression evaluation code.
**
** History:
**	2-mar-1992 (pete)
**	    Initial version.
**	21-mar-1992 (pete)
**	    remove UG_STMT... definitions. Move them to abf!fg!framegen.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define UG_MAX_TOKSIZE	250	/* max size of token */
#define UG_MAX_BYTES_PER_CHAR      5	/* for multi-byte support */

/* constants used with EX for error processing */
#define UG_EXPR_FAIL 	 0x10	/* Error, return FAIL to caller */
#define UG_EXPR_CONTINUE 0x20	/* Warning, EXCONTINUE after print message */

/* constants that tell ugexpr.c what type of expression is being passed */
#define UG_BOOL_EXPR		1	/* no assignments. evaluates to T/F.
					** IF and WHILE statements use this.
					*/
#define UG_EXPR_OR_ASSIGN	2	/* expression may contain an assignment
					** e.g., "$a=1+$b/2", or maybe just
					** an expression (RHS of assignment),
					** e.g. (1+1)/3, $frame_name+'.ext',
					** or TRUE. SET statements use this.
					*/
/*}
** Name:	UG_EXPR_RESULT - describes result of expression evaluation.
**
** Description:
**
** History:
**	9-mar-1992 (pete)
**	    Initial version.
*/
typedef struct {
	i4 res_type;	/* result type. tells which union member to access. */
#define UG_NONE                 0x0
#define UG_DELIMITER		0x01
#define UG_AND_CONNECTOR	0x02	/* AND */
#define UG_OR_CONNECTOR		0x04	/* OR */

#define UG_BOOLEAN		0x10
#define UG_NUMERIC		0x20
#define UG_CHARACTER		0x40

#define UG_CONSTANT             0x100			/* constant */
#define UG_NUMERIC_CONSTANT     (UG_NUMERIC | UG_CONSTANT) /* 0, 1, 5000, etc */
#define UG_BOOLEAN_CONSTANT     (UG_BOOLEAN | UG_CONSTANT) /* TRUE or FALSE */
#define UG_CHARACTER_CONSTANT   (UG_CHARACTER | UG_CONSTANT) /* quoted string */

	union {
	    i4 ival;	/* Integer or Boolean result. */
	    char   *cval;	/* Character result. */
	} res;
} UG_EXPR_RESULT;

/*}
** Name:	UG_EXPR_TOKEN - info about a token returned from get_token().
**
** Description:
**	Tells information about a single token.
**
** History:
**	11-mar-1992 (pete)
**	    Initial version.
*/
typedef struct {
        i4  tok_type;		/* same types as UG_EXPR_RESULT.res_type */
        char token[UG_MAX_TOKSIZE+1];	/* the actual token (NULL terminated) */
} UG_EXPR_TOKEN;

/*}
** Name:	UG_EXPR_OPER_INFO - info about an operator.
**
** Description:
**	Information about an operator (e.g. a token delimiter).
**	Arrays of these can be used for the various types of operators
**	(delimiters, comparison operators, etc). For example,
**	an array of these is searched and compared with a token from a boolean
**	expression to see if the token is a comparison operator
**	(">=", "=", etc).
**
** History:
**	2-mar-1992 (pete)
**	    Initial version.
*/
typedef struct
{
	char *str_oper;	/* the actual operator as a string: "=", "!=", etc */
	i4 oper_len;	/* STlength of str_oper */
	i4 comp_oper;	/* symbolic name of oper */
#define UG_CO_NONE	0
#define UG_CO_EQ	1	/* = */
#define UG_CO_NE	2	/* <> or != */
#define UG_CO_GT	3	/* > */
#define UG_CO_GE	4	/* >= */
#define UG_CO_LT	5	/* < */
#define UG_CO_LE	6	/* <= */
} UG_EXPR_OPER_INFO;

/*}
** Name:	UG_EXPR_CALLBACKS - structure of function pointers.
**
** Description:
**	This structure contains function addresses used in the ugexpr.c code.
**	These are all call-back functions, which the caller must first register
**	with expr. Vision uses this; eventually, the W4gl Amethyst project
**	will too.
**	This was done to allow the expression evaluation code to be placed
**	in a shared library (UG), even though it needs to call several 
**	specific routines from the caller's library (ABF!FG, Amethyst).
**
** History:
**	2-mar-1992 (pete)
**	    Initial Version.
**	23-jun-1992 (pete)
**	    Removed structure members: isvar, vartoresult & resulttovar.
*/
typedef struct
{
	PTR  (*getmem)();	/* allocate memory */
} UG_EXPR_CALLBACKS;
