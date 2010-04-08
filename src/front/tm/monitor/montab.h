/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**  MONTAB.H -- included by monitor.c and montab.roc
*/

/*
**  COMMAND TABLE
**	To add synonyms for commands, add entries to this table
*/

struct cntrlwd
{
	char	*name;
	i4	code;
};

GLOBALREF struct cntrlwd	Controlwords[];

GLOBALREF struct cntrlwd	Syscntrlwds[];
