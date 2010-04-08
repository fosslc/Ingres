# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<cs.h>
# include	<meprivate.h>
# include	<me.h>
# include	<qu.h>

/*
 *Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		MEfree.c
 *
 *	Function:
 *		MEfree
 *
 *	Arguments:
 *		void *	block;
 *
 *	Result:
 *		Frees the block of memory pointed to by 'block'.
 *
 *		First insures that the address passed in is in the
 *		process' data space.
 *
 *		Calls MEfadd or free() to add block to free list.
 *
 *		Returns STATUS: OK, ME_00_FREE, ME_OUT_OF_RANGE,
 *				ME_FREE_FIRST, ME_TR_FREE, ME_NOT_FOUND,
 *				ME_NO_FREE, ME_ERR_PROGRAMMER.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 - (gb)
 *			written
 *
 *		16-Dec-1986 -- (daveb)
 *		Rework to run on top of malloc(), change to PTR arg.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	21-Jan-91 (jkb)
**	    Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**	    "if( MEadvice == ME_USER_ALLOC )" so it works with
**	    ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**	3/91 (Mike S)
**	    Use fast freeing alogrithm; keep track of memory statistics for
**	    development debugging;
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_DONT_SHIP_PROTOS
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      03-jun-1996 (canor01)
**          Add synchronization and make MEstatus local to make ME
**          stream allocator thread-safe when used with operating-system
**          threads.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	11-May-2009 (kschendel) b122041
**	    Change pointer arg to void *, more appropriate and stops many
**	    gazillions of compiler warnings throughout (especially front).
*/


GLOBALREF  SIZE_TYPE	i_meactual;
GLOBALREF  SIZE_TYPE	i_meuser;

# ifdef OS_THREADS_USED
GLOBALREF CS_SYNCH      MEfreelist_mutex;
GLOBALREF CS_SYNCH      MElist_mutex;
# endif /* OS_THREADS_USED */

STATUS
#ifdef CL_DONT_SHIP_PROTOS
MEfree(
	void *block)
#else
MEfree(block)
void	*block;
#endif
{
    register ME_NODE *this;
    register ME_NODE *start;
    register ME_NODE *blknode;  /* ME_NODE for 'block */
    STATUS	MEstatus;

    MEstatus = OK;

    if ( block == NULL )
    {
	MEstatus = ME_00_FREE;
    }
    else if ( OUT_OF_DATASPACE(block) )
    {
	MEstatus = ME_OUT_OF_RANGE;
    }
    else if ( !MEsetup )
    {
	MEstatus = ME_FREE_FIRST;
    }
    else
    {
	blknode = (ME_NODE *)((char *)block - sizeof(ME_NODE));

	/* Look for block in allocated list.   */

# ifdef OS_THREADS_USED
        CS_synch_lock( &MElist_mutex );
# endif /* OS_THREADS_USED */

# ifndef ME_SLOW_FREE
	/* 
	** assume block is legit if the node looks like it points to an
	** allocated block.
        */
	this = blknode;
	if (blknode->MEaskedfor == 0)
	    MEstatus = ME_NO_FREE;
# else /* ME_SLOW_FREE */

	/* 
	** Check to make sure block is on allocated list.
	** This is an EXPENSIVE linear search through an 
	** unordered list (daveb).
	*/
	start = (ME_NODE *)&MElist;
	this = MElist.MEfirst;
	while ( this != NULL && this != start && this != blknode )
	    this = this->MEnext;

	if( this == NULL )
	{
	    MEstatus = ME_CORRUPTED;
	}
	else if ( this == start )
	{
	    MEstatus = ME_NO_FREE;
	}
# endif	/* ME_SLOW_FREE */

	if (MEstatus == OK)
	{
	    i_meactual -= this->MEsize;
	    i_meuser -= this->MEaskedfor;

	    /* cut from our allocated list */
	    (void)QUremove( (QUEUE *) this );
# ifdef OS_THREADS_USED
            CS_synch_unlock( &MElist_mutex );
# endif /* OS_THREADS_USED */

	    if( MEadvice != ME_USER_ALLOC )
	    {
# ifdef OS_THREADS_USED
                CS_synch_lock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
		MEstatus = MEfadd( this, TRUE );
# ifdef OS_THREADS_USED
                CS_synch_unlock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
	    }
	    else
		free( this );
	}
# ifdef OS_THREADS_USED
	else
            CS_synch_unlock( &MElist_mutex );
# endif /* OS_THREADS_USED */

    }

    return( (MEstatus == ME_CORRUPTED) ? ME_TR_FREE : MEstatus );
}

