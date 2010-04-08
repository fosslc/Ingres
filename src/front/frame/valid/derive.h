/*
**	derive.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	derive.h - Include header file for derived fields support.
**
** Description:
**	This file contains various defined constants and structure
**	declarations for the derived fields parser.
**
**  History:
**	06/05/89 (dkh) - Initial version.
**	06/23/92 (dkh) - Added support for decimal.
**	10/28/92 (dkh) - Changed ALPHA to DV_ALPHA for compiling on DEC/ALPHA.
**	10-Nov-1999 (kitch01)
*		Bug 98227 added comma to the list of valid tokens
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* MANIFEST CONSTANTS */
# define	MAXSTRING	255	/* max length of strings */
# define	GOVAL		-1	/* semantic value for command names */

/* CHARACTER TYPES */
# define	DV_ALPHA	1	/* an alpha character */
# define	NUMBR		2	/* a numeric character */
# define	OPATR		3	/* an operator character */
# define	PUNCT		4	/* a punctuation character */
# define	CNTRL		5	/* a control character */

/* derived field operators */
# define	d_opMINUS	1	/* the minus operator */
# define	d_opPLUS	2	/* the plus operator */
# define	d_opTIMES	3	/* the multiply operator */
# define	d_opDIV		4	/* the divide operator */
# define	d_opEXP		5	/* the exponentiation operator */


/*}
** Name:	IIFVdstype - derivation parser stack type.
**
** Description:
**	The stack type for the derivation parser.
**
** History:
**	06/05/89 (dkh) - Initial version.
*/
typedef union
{
	i4		type_type;	/* a stack type */
	struct dnode 	*node_type;	/* a derivation tree node type */
	i4		I4_type;	/* an interger type */
	f8		F8_type;	/* a floating point type */
	DB_TEXT_STRING	*string_type;	/* a string type (varchar) */
	char		*name_type;	/* a name type */
	DB_DATA_VALUE	*dec_type;	/* a decimal data value */
} IIFVdstype;

/* SPECIAL TOKENS for scanner */
/*}
** Name:	struct dspec - Special derivation tokens structure.
**
** Description:
**	Contains special token values for the derivation parser.
**
** History:
**	06/05/89 (dkh) - Initial version.
**	10-nov-1999  (kitch01)
**		Bug 98227. Added comma to the list of valid tokens
*/
struct dspec
{
	i2	i4const;	/* the integer token */
	i2	f8const;	/* the floating point token */
	i2	name;		/* the name token */
	i2	lparen;		/* the left parenthesis token */
	i2	rparen;		/* the right parenthesis token */
	i2	lbrak;		/* the left square bracket token */
	i2	rbrak;		/* the right square bracket token */
	i2	period;		/* the period token */
	i2	comma;		/* the comma token */
	i2	svconst;	/* the (varchar) string token */
	i2	plus;		/* the plus operator token */
	i2	minus;		/* the minus operator token */
	i2	times;		/* the multiply operator token */
	i2	div;		/* the divide operator token */
	i2	exp;		/* the exponentiation operator token */
	i2	sum;		/* the sum operator token */
	i2	d_max;		/* the max operator token */
	i2	d_min;		/* the min operator token */
	i2	average;	/* the average operator token */
	i2	count;		/* the count operator token */
	i2	sconst;		/* the string constant token */
	i2	decconst;	/* the decimal constant token */
};

/* declarations */
GLOBALREF struct dspec	IIFVdTokens;		/* special tokens table */
