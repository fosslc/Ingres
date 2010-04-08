/*
** Copyright (c) 2004 Ingres Corporation
**
** static char Sccsid = "@(#)DY.h	4.1  3/5/84"
*/

/*
** Defines external types needed for dynamic linking.
*/

typedef struct
{
	char	*sym_name;	/* Symbols name */
	i4	sym_value;	/* Its address */
} SYMTAB;
