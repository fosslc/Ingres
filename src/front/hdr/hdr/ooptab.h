/*
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*}
** Name:	procTabEntry -	Object Manager Procedure Table Entry Structure.
**
** Description:
**	This structure describes an entry in the Object Manager procedure table.
**
** History:
**	01/86 (ptk) -- Defined.
*/

typedef	struct {
	OOID	opid;
	char	*psymbol;
	OOID	(*pentry)();
} procTabEntry;
