# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>
# include	<cs.h>
# include	<qu.h>

/*
**
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		MEtfree.c
**
**	Function:
**		MEtfree
**
**	Arguments:
**		i2	tag;
**
**	Result:
**		Free all blocks of memory on allocated list with
**		MEtag value of 'tag'.
**
**		Returns STATUS: OK, ME_00_PTR, ME_OUT_OF_RANGE, ME_BD_TAG,
**				ME_FREE_FIRST, ME_TR_TFREE, ME_NO_TFREE.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 - (gb)
**			written
**
**		17-Dec-1986 - (daveb)
**		Changed for layering on malloc.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**		3/91 (Mike S)
**		Use fast deallocation.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external ME function references;
**              these are now all in either me.h or meprivate.h.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes, changed to match me.h
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      09-jun-1995 (canor01)
**          added semaphore protection for lower-level free function
**	03-jun-1996 (canor01)
**	    made MEstatus local for use with operating-system threads.
*/

/* Externs */

STATUS
MEtfree(
	u_i2	tag)
{
    register ME_NODE *this;
    register ME_NODE *start;
    register ME_NODE *next;
    bool	tag_found = FALSE;
    STATUS	MEstatus;

    MEstatus = OK;

    if (tag <= 0)			/* check for proper tag */
    {
	MEstatus = ME_BD_TAG;
    }
    else if (!MEsetup)		/* can't free if never allocated */
    {
	MEstatus = ME_FREE_FIRST;
    }
    else
    {
	MEstatus  = IIME_ftFreeTag(tag);
    }

    return( (MEstatus == ME_CORRUPTED) ? ME_TR_TFREE : MEstatus );
}
