/*
**Copyright (c) 1987, 2004 Ingres Corporation
*/

/**
** Name:	itcstr.h	-  Module header for interp C string handler
**
** Description:
**	Defines the entry points for the interpreter C string handler.
**	This module is used to convert between character typed
**	DBVs to a NULL-terminated ('\0') C string.
**
**	The module has the following protocol.  Upon startup of
**	the program, the module is already initialized.
**
**	When a DBV needs to be converted to a C string, call
**	IIITtcsToCStr (see its description for argument types...)
**	It will return the argument converted to a C string.  The
**	returned C string is in readonly memory (conceptually at least),
**	and will remain fixed until a call to IIITrcsResetCStr.  All
**	strings returned by successive calls to IIITtcsToCStr without
**	intervening IIITrcsResetCStr calls will remain valid until
**	the IIITrcsResetCStr call.  After the IIITrcsResetCStr call
**	any pointers returned by IIITtcsToCStr before that call are
**	no longer valid, and no guarantees are made about the memory
**	they point to.
**
**	It is a noop to call IIITrcsResetCStr if no IIITtcsToCStr calls
**	have been made since the last IIITrcsResetCStr.
**
**
** History:
**	25-oct-1988 (Joe)
**		First Written
**/

FUNC_EXTERN char	*IIITtcsToCStr();
FUNC_EXTERN VOID	IIITrcsResetCStr();
