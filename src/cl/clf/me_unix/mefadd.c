# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<cs.h>
# include	<meprivate.h>
# include	<me.h>

/*
NO_OPTIM = dg8_us5
*/

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		MEfadd.c
**
**	Function:
**		MEfadd
**
**	Arguments:
**		ME_NODE	*block;		block to add to freelist.
**		i4 releasep;		flag to release memory at the end.
**
**	Result:
**		After insuring that 'block' is a proper address,
**			place 'block' into the free listsuch that
**			addresses of blocks are in ascending order.
**			Adjacent blocks	are coalesced.
**
**		called by MEfree() and MEtfree().
**
**		Returns STATUS: OK, ME_00_PTR, ME_OUT_OF_RANGE, ME_CORRUPTED.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 - (gb)
**			written
**
**		16-Dec-1986 -- (daveb)
**		Reworked to layer on malloc if desired.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <compat.h>";
**		put in check for MEfreelist.MEfirst when coalescing
**		backwards to start of queue; stop when at head of list.
**	3/91 (Mike S)
**	    Clear MEaskedfor after freeing, so we can tell it's free.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	03-jun-1996 (canor01)
**	    make MEstatus local for use with operating-system threads.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	07-Apr-2000 (jenjo02)
**	    Colaesce back through first node on free list rather than
**	    up to first node. The first two free nodes were not being
**	    coalesced when then should have been.
[@history_template@]...
*/


# define RELEASE_QUANTUM	(2 * BLOCKSIZE)


STATUS
MEfadd(
	ME_NODE	*block,
	i4 releasep)
{
    register ME_NODE *this;
    register ME_NODE *start;
    register ME_NODE *nextblk;	/* 1st address after end of 'block' */
    STATUS  MEstatus;

    MEstatus = OK;

    if ( block == NULL )
    {
	MEstatus = ME_00_PTR;
    }
    else if ( OUT_OF_DATASPACE(block) )
    {
	MEstatus = ME_OUT_OF_RANGE;
    }
    else
    {

	nextblk = NEXT_NODE(block);

	/*
	** The freelist IS sorted.  We might be able to take
	** advantage of this to speed up this linear search.
	** In practice the freelist is usually much
	** smaller than the allocated list, so it might not
	** really matter (daveb).
	*/

	/* adding inside existing freelist */
	start = (ME_NODE *)&MEfreelist;
	this = MEfreelist.MEfirst;

	while ( this != NULL && this != start && this < nextblk )
	    this = this->MEnext;

	if( this == NULL )
	    MEstatus = ME_CORRUPTED;

	/*
	** this->MEprev points to free node before 'block', the one
	** to free.  this points to the block in the free
	** list following 'block' to free.
	*/
	if ( MEstatus == OK )
	{
	    block->MEaskedfor = 0;
	    (void)QUinsert( (QUEUE *) block, (QUEUE *) this->MEprev );

	    /* Coalesce backwards until this is the start of the queue */
	    this = block;
	    while( this->MEprev != start &&
                 this->MEprev != this && NEXT_NODE(this->MEprev) == this )
	    {
		block = this->MEprev;
		block->MEsize += this->MEsize;
		(void)QUremove( (QUEUE *) this );
		this = block;
	    }

	    /* Coalesce forwards until the queue goes back to the start */
	    while( this->MEnext != start && NEXT_NODE(this) == this->MEnext )
	    {
		this->MEsize += this->MEnext->MEsize;
		(void)QUremove( (QUEUE *) this->MEnext );
	    }
	}
    }
			
    return(MEstatus);
}

