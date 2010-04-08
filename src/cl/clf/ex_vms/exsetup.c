/*
** Copyright (c) 1993, 2009 Ingres Corporation
*/
# include	<compat.h>
# include	<excl.h>
#include "exi.h"

/*
**
**      History:
**
**           25-jun-1993 (huffman)
**               Change include file references from xx.h to xxcl.h.
**
**	     04-oct-1995 (duursma)
**		 Performance improvements:
**		 No more calls to lib$get_invo...
**		 Instead, use UNIX CL method of linking EX_CONTEXT blocks
**		 together.
**	     07-may-1998 (kinte01)
**		 Initialize previous_context pointer to 0,
**        	 else EXdelete() can SIGBUS ->server death on
**        	 non-0 uninitialized pointer. (bug 89411)
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	29-jun-2009 (joea)
**	    The first argument should be a pointer to a function returning
**	    STATUS not to a function returning a pointer to i4.
*/

STATUS 
EXsetup(STATUS (*handler)(EX_ARGS *args), EX_CONTEXT *context)
{
        context->prev_context = 0;

	context->handler_address = handler;
	context->address_check = (PTR)context;

	i_EXpush(context);

	return OK;
}
