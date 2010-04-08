/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
**  TOKENS.Y -- operator and keyword tables for QUEL
**
**	SCANNER KEYWORD TABLES
**
**	Keywords, tokens, and opcode tuples are included in this file
**	The keyword table MUST be in sorted order.
**	The operator table does not need to be sorted
**
**  History:
**	06/12/92 (dkh) - Added support for decimal datatype for 6.5.
*/


/*
** Tokens
** a structure initialized here to contain the
** yacc generated tokens for the indicated
** terminal symbols.
*/
GLOBALDEF struct special	Tokens =
{
	SCONST,
	I4CONST,
	F8CONST,
	NAME,
	LPAREN,
	RPAREN,
	LBRAK,
	RBRAK,
	PERIOD,
	COMMA,
	RELOP,
	UMINUS,
	LBOP,
	LUOP,
	IN,
	IS,
	NOT,
	ANULL,
	LIKE,
	SVCONST,
	DECCONST
};
