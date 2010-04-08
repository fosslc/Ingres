/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<iltypes.h>
#include	<lt.h>

/**
** Name:    igstksid.c -	OSL Interpreted Frame Object
**					SID Stack Manager Module.
** Description:
**	Contains routines used to maintain and access a LIFO stack of OSL
**	intermediate language SID references by any program that generates IL
**	code.  This stack is known colloquially as the `GOTO' stack.  Defines:
**
**	IGpushSID()	push SID onto stack.
**	IGpopSID()	pop SID from stack.
**	IGtopSID()	return SID from top of stack.
**
** History:
**	Revision 5.1  86/10/17  16:40:59  wong
**	Initial revision  (86/07  wong)
*/

static LIST	*sidstk = NULL;

/*{
** Name:    IGpushSID() -	Push SID onto Stack.
**
** Description:
**	Pushes the input SID reference onto the top of the SID stack.
**
** Input:
**	sid	{IGSID *}  The address of the SID to be pushed.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
IGpushSID (sid)
IGSID	*sid;
{
    LTpush(&sidstk, (PTR)sid, 0);
}

/*{
** Name:	IGpopSID() -	Pop SID from Stack.
**
** Description:
**	Pops the top element from the SID stack and returns it.
**
** Returns:
**	{IGSID *}  The top element of the SID stack.
**
** History:
**	07/86 (jhw) -- Written.
*/

IGSID *
IGpopSID ()
{
    return (IGSID *)LTpop(&sidstk);
}

/*{
** Name:    IGtopSID() -	Return SID From Top of Stack.
**
** Description:
**	Returns a reference to the SID that is presently on the top of the stack
**	but does not change the state of the stack.
**
** Returns:
**	{IGSID *}  The top element of the SID stack.
**
** History:
**	07/86 (jhw) -- Written.
*/

IGSID *
IGtopSID ()
{
    return (IGSID *)LTtop(sidstk);
}
