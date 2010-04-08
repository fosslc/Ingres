/*
**    Copyright (c) 1993, 2000 Ingres Corporation
*/
# include	<compat.h>
# include	<excl.h>
# include	<starlet.h>
# include       <astjacket.h>

/*
**
**      History:
**
**           25-jun-1993 (huffman)
**               Change include file references from xx.h to xxcl.h.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

static	setcount = 0;

void
EXinterrupt(i4 new_state)
{

	/*  nestable request to disable interrupts  */

	if (new_state == EX_OFF)
	{
		if (setcount > 0)
		{
			setcount++;
		}
		else
		{
			sys$setast(0);
			setcount++;
		}
		return;
	}

	/*  nestable request to enable interrupts  */

	if (new_state == EX_ON)
	{
		if (--setcount > 0)
			return;
	}

	/*
	**	Actually turn interrupts on.  (EX_RESET request comes here, or
	**	setcount counted down to zero.)
	*/

	setcount = 0;
	sys$setast(1);
}

int
EXchkintr()
{
	return setcount;
}
