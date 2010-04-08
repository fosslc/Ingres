/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** multi.h
**	Frontend header file for multinational support.
**	This pulls in a global header file shared by
**	the front and back ends and then declares
**	an II routine that frontends use to get the multinational
**	options.
*/

# include	<intl.h>

FUNC_EXTERN MULTINATIONAL	*IImulti();
