
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<ex.h>
# include	<exinternal.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
** EXsetup -- Setup Context for Exception Handling
**
**	This routine is always present (what was NO_EX is now universal).
**	It is called out of the EXdeclare() macro defined in the global
**	EX.h:
**
** # define EXdeclare(x,y)	(EXsetup(x,y), setjmp((y)->jmpbuf))
**
**	History:
**	
 * Revision 1.1  90/03/09  09:14:51  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:41:06  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:49:06  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:09:37  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:04  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:36:24  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/25  13:10:14  mikem
**      use new lower case hdr file names.
**      
**      Revision 1.1  87/11/09  12:59:36  mikem
**      Initial revision
**      
 * Revision 1.1  87/07/30  18:04:12  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 1.3  86/04/25  17:01:09  daveb
**		Fix comments.
**		
 * Revision 1.2  86/04/24  12:02:08  daveb
 * Remove dead comments,
 * Make NO_EX universal.
 * 
**	7-Jul-1993 (daveb)
**	    prototyped, made void, made exsetfirst static.
**	    Handler returns STATUS, not i4.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      02-sep-93 (smc)
**          Made context cast a PTR to be portable to axp_osf. 
**	16-april-1998(angusm)
**	  Initialise previous_context pointer to 0,
**	  else EXdelete() can SIGBUS ->server death on
**	  non-0 uninitialised pointer. (bug 89411)
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
*/

static bool	exsetfirst = TRUE;

void
EXsetup( STATUS(*handler)(), EX_CONTEXT *context)
{

	context->prev_context = 0;

	if (exsetfirst)
	{
		exsetfirst = FALSE;
		i_EXestablish();
	}

	context->handler_address = handler;
	context->address_check = (PTR)context;

	i_EXpush(context);	
}
