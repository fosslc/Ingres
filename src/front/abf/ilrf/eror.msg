/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

/**
** Name:	eror.msg -	Message file for class OR
**
** Description:
**	This file defines the messages for class OR.
**
** History:
**	 3-aug-1987 (bobm)
**		Created.
**/

# define	_OR_CLASS	47
/*
**	irptrz.c
*/
/*
**	fatal errors - not supposed to happen
*/
S_OR0001_no_stack	"ILRF pointerize: no stack data area" /* DIAG */
S_OR0002_no_indir	"ILRF pointerize: no stack indirection section" /* DIAG */
S_OR0003_bad_DBD_offset	"ILRF pointerize: bad DBD offset" /* DIAG */
S_OR0004_bad_offset	"ILRF ptrize: bad symbol table offset" /* DIAG */
S_OR0005_deptrz		"ILRF depointerization error" /* DIAG */
/*
**	irlist.c
*/
/*
**	fatal errors in linked list utilities - not supposed to happen
*/
S_OR0006_NULL_pred	"list_unlink: NULL predecessor not head" /* DIAG */
S_OR0007_bad_tail	"list_unlink: non-null tail link" /* DIAG */
S_OR0008_NULL_pred	"list_head: NULL predecessor not head" /* DIAG */
S_OR0009_NULL_list	"list_head: NULL list" /* DIAG */
S_OR000A_bad_tail	"list_tail: null linked non-tail item" /* DIAG */
S_OR000B_NULL_tail	"list_tail: NULL tail" /* DIAG */
S_OR000C_bad_pred	"list_tail: NULL predecessor not head" /* DIAG */
S_OR000D_pool_alloc	"ILRF cannot create list pool" /* DIAG */
/*
** irfget.c
*/
F_OR0000_fetching	"Fetching '%0c' from the database ..."
