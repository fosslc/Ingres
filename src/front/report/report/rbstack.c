/*
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<er.h>
# include	<ug.h>

/**
** Name:	rbstack.c -	Report Text Action Commands Begin/End Stack.
**
** Description:
**   R_PSH_BE and R_POP_BE - routines to push and pop onto the 
**	begin/end stack, which holds information about the current
**	nesting of block commands.  These commands are currently
**	the .WITHIN/.ENDWITHIN and .BLOCK/.ENDBLOCK, though more
**	commands will be added in the future.  Note that separate
**	stacks are kept for the page and the report.
**
**	r_psh_be()	push text action command.
**	r_pop_be()	pop text action command.
**
** History:	
 * Revision 61.3  89/03/27  16:04:55  danielt
 * *** empty log message ***
 * 
 * Revision 61.2  88/12/20  15:06:23  sylviap
 * Changed to use FEreqmem.
 * 
 * Revision 61.1  88/03/25  22:01:18  sylviap
 * *** empty log message ***
 * 
 * Revision 60.5  87/08/16  10:34:00  peter
 * Remove ifdefs for xRTR1 xRTR2 xRTR3 xRTM
 * 
 * Revision 60.4  87/04/08  01:16:21  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.3  87/03/26  18:11:34  grant
 * Initial 6.0 changes calling FMT, AFE, and ADF libraries
 * 
 * Revision 60.2  86/11/19  19:00:29  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  19:00:14  dave
 * *** empty log message ***
 * 
**		Revision 50.0  86/05/19  15:52:15  wong
**		Returned pointer must be 'nat *', not just 'nat'.
**
**		1/7/82 (ps)	written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



/*
**   R_PSH_BE - push a TCMD structure block stack.
**
**	Parameters:
**		tcmd	address of TCMD to push.
**
**	Returns:
**		address of new St_cbe.
**
**	Called by:
**		r_x_within, r_x_block.
**
**	Side Effects:
**		Updates St_cbe.  May initialize Ptr_t_cbe.
**
**	Trace Flags:
**		150, 160, 161.
**
**	Error Messages:
**		none.
**
**	History:
**		1/7/82 (ps)	written.
**	19-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
*/

STK *
r_psh_be(tcmd)
register TCMD	*tcmd;
{
	register STK	*stk;		/* fast ptr to a STK */

	/* start of routine */

	
	if (Ptr_t_cbe == NULL)
	{	/* start the stack */
        	if ((stk = (STK *)MEreqmem ((u_i4) 0,
			(u_i4)(sizeof(STK)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_psh_be"));
		}

		Ptr_t_cbe = stk;
	}
	else if (St_cbe == NULL)
	{	/* nothing in stack now */
		stk = Ptr_t_cbe;
	}
	else if (St_cbe->stk_below == NULL)
	{	/* must add to stack */
        	if ((stk = (STK *)MEreqmem ((u_i4) 0,
			(u_i4)(sizeof(STK)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_psh_be"));
		}
		stk->stk_above = St_cbe;
		St_cbe->stk_below = stk;
	}
	else
	{	/* already have one, use it */
		stk = St_cbe->stk_below;
	}

	/* now add the TCMD add to the STK structure */

	stk->stk_tcmd = tcmd;

	St_cbe = stk;


	return stk;
}



/*
**   R_POP_BE - routine to pop a STK structure.
**	This routine is called with a code for a TCMD structure.  
**	That is, it is called with the code for the TCMD structure
**	which it ends, if any.  If it is called with one of these
**	codes (because of a command such as (.END WITHIN), it will
**	search up through the stack to find the structure that is
**	to be popped.
**
**	If it is called without a code (a code of P_ERROR), that
**	means that this is an unrestricted .END, and pops the
**	last TCMD on the list.
**
**	Parameters:
**		code	code of the TCMD structure to pop.
**
**	Returns:
**		address of TCMD which is popped.
**		NULL returned if none in stack.
**
**	Called by:
**		r_x_end.
**
**	Side Effects:
**		Changes St_cbe.
**
**	Trace Flags:
**		150, 160, 161.
**
**	Error Messages:
**		none.
**
**	History:
**		1/7/82 (ps)	written.
*/

TCMD	*
r_pop_be(code)
i2	code;
{
	/* internal declarations */

	register STK	*ostk=NULL;	/* fast ptr to stack */
	register TCMD	*otcmd=NULL;	/* fast prt to TCMD */

	/* start of routine */


	if (code == P_ERROR)
	{	/* simple .END.  End the last element in stack */
		ostk = St_cbe;
		if (ostk!=NULL)
		{
			otcmd = ostk->stk_tcmd;
		}
	}
	else
	{	/* and .END (command) found.  Find the match */
		for(ostk=St_cbe; ostk!=NULL; ostk=ostk->stk_above)
		{	/* move up the stack */
			otcmd = ostk->stk_tcmd;
			if (otcmd->tcmd_code == code)
			{	/* found the stack element to end */
				break;
			}
		}
	}

	/* Now move the rest of the stack up because there may be
	** other as yet unended TCMD structures */

	if (ostk!=NULL)
	{
		for(;ostk!=St_cbe && ostk!=NULL && ostk->stk_below!=NULL;
					ostk=ostk->stk_below)	
		{
			ostk->stk_tcmd = (ostk->stk_below)->stk_tcmd;
		}
		St_cbe = St_cbe->stk_above;
	}
	else
	{	/* error. Not found */
		otcmd = NULL;
	}


	return otcmd;
}
