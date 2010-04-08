/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<erug.h>
#include	"feloc.h"	/* includes <me.h> */

/**
** Name:	fetag.c - Front-End Utility Tagged Memory Allocation Module.
**
** Description:
**	Contains routines used to allocate memory tags for use by the
**	Front-Ends.  Defines:
**
**	FEgettag()	return a new unique tag ID.
**	IIUGtagFree()	return a tag and memory for re-use.
**	FEbegintag()	begin new tag block for memory allocation.
**	FEendtag()	end tag block.
**	FElocktag()	lock/unlock a tag region.
**	FEtaggedlock()	test if current tag region is locked.
**
** History:
**	Revision 6.0  87/06/01  joe
**	Added 'IIUGtagFree()'.
**	13-jul-87 (bruceb)  Changed memory allocation to use 'MEreqmem()'.
**
**	Revision 3.0  84/10/31  ron
**	Initial revision.
**	10-jan-92 (szeng)
**		The comparision statement to check whether stack is
**		empty was wrong since the value of tag equal to MINTAGID
**		is valid value. This has caused the memory for this case
**		is never freed and brought memory leak (b40478) on ds3.ulx 
**		W4GL05 and might present the memory leak for Ingres base 
**		products as well. The correct way to check is the see 
**		whether (_topStack->tag < MINTAGID) or (_topStack->tag == -1) 
**		since _topStack is initialized as a pointer to {-1, NULL}. 
**		However considering the dependancy of initialization of
**		_topStack with this comparision statement I think testing
**		(_topStack->tag < MINTAGID) is a better choice. 
**	03/01/92 (dkh) - Modified above change to not even worry about
**			 tag values.  We know we are back to our original
**			 TAGNODE if struct member "last" is NULL.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF TAGID	iiUGcurTag;
GLOBALREF TAGID iiUGfirstTag;

/*{
** Name:	FEgettag() - Return a New, Unique Tag-ID.
**
** Description:
**	Returns a new, unique tag-ID.
**
** Returns:
**	{TAGID}  A unique tag-ID.
**
** History:
**	11/2/84 (rlk) - First Written
**	7/93 (Mike S) Now a cover for MEgettag
*/

TAGID
FEgettag(void)
{
	u_i2 tag;

	tag = MEgettag();
	if (tag == 0)
	    IIUGerr(E_UG002F_BadMemoryTag, 0, 0);

	return tag;
}

/*{
** Name:	IIUGtagFree() -	Return a Tag and Memory for Re-use.
**
** Description:
**	Returns a tag block ID for re-use when it is no longer needed.
**	The memory associated with the tag is also freed.
**
**	This routine should be used wherever the following call sequence
**	would be:
**
**		FEfree( FEendtag() );
**
**	Instead:
**
**		IIUGtagFree( FEendtag() );
**
** History:
**	1-jun-1987 (Joe)
**		Initial Version
**	15-jun-1987 (Joe)
**		Made it call FEfree before returning tag.
**	7/93 (Mike S) Now use MEfreetag
**	8/93 (Mike S) New calling sequence for MEfreetag
*/
VOID
IIUGtagFree ( TAGID tag )
{
	if (FEfree(tag) == OK)
	{
	    /* If FEfree didn't find an entry, this is a bogus tag. */
	    (VOID) MEfreetag(tag);
	}
}

static VOID	_pushTag();
static TAGID	_popTag();

/*{
** Name:	FEbegintag() -	Begin New Tag Block for Memory Allocation.
**
** Description:
**	Signals the start of a new tag block (ie., a region delimited by
**	successive 'FEbegintag()' and 'FEendtag()' calls).  All memory
**	allocated by 'FEreqmem()' within a tag region will be allocated with
**	the same tag ID.  Tag IDs for regions start at MINTAGID  + 1 (51.)
**
** Returns:
**	{TAGID}  A unique tag ID.
**
** History:
**	10/31/84 (rlk) - First Written
*/
TAGID
FEbegintag(void)
{
	/* Save the current tag region's ID */

	if (iiUGcurTag == 0)
	{
	    iiUGcurTag = iiUGfirstTag = FEgettag();
	}
	_pushTag(iiUGcurTag);

	/* Get the next unique tag-ID */

	return (iiUGcurTag = FEgettag());
}

/*{
** Name:	FEendtag() -	End Tag Block.
**
** Description:
**	Signals the end of the current tag block.
**
** Returns:
**	{TAGID}  The tag ID associated with the tag region being ended.
**
** History:
**	10/31/84 (rlk) - First Written
*/
TAGID
FEendtag(void)
{
	TAGID	oldtag;

	/* Save the current regions tag to pass back */

	oldtag = iiUGcurTag;

	/* Set the current tag-ID to that of the previous region */

	iiUGcurTag = _popTag();

	return oldtag;
}

/*}
** Name:	TAGNODE -	Tag Stack Element.
*/
typedef struct	_tgnode
{
	TAGID		tag;
	struct _tgnode	*last;
} TAGNODE;

static TAGNODE	*_getNode();

/* Tag stack - used with 'FEbegintag' and 'FEendtag()' */

static TAGNODE _tagStack = {-1, NULL};
static TAGNODE *_topStack = &_tagStack;	/* PTR to top of Tag stack */

/*
** Name:	_pushTag() -	Push the New Nag-ID Onto the Stack.
**
** Description:
**	Called by 'FEbegintag()' to push the new tag-ID onto the 
**	tag-ID stack.
**	
** Input:
**	tag	{TAGID}  The tag to be pushed.
**
** History:
**	10/31/84 (rlk) - First Written.
*/
static VOID
_pushTag ( tag )
TAGID	tag;
{
	register TAGNODE	*tnode;

	/*
	** The stack is implemented by a linked list.
	** Allocate the next node for the list.
	*/

	if ( (tnode = _getNode()) == (TAGNODE *)NULL )
	{
		IIUGbmaBadMemoryAllocation( ERx("fetag:  tag stack") );
	};

	tnode->tag = tag;
	tnode->last = _topStack;

	_topStack = tnode;
}

/*
** Name:	_popTag() -	Pop the Current Tag-ID Off the Stack.
**
** Description:
**	Called by 'FEendtag()' to pop the current tag-ID off the tag-ID stack.
**	
** Returns:
**	{TAGID}  The tag-ID that was popped or -1 on failure.
**	
** History:
**	10/31/84 (rlk) - First Written.
*/
static VOID	_freeNode();

static TAGID
_popTag ()
{
	register TAGNODE	*tnode;
	TAGID			oldtag;

	/* If stack not empty, pop it and return the tag popped */

	if ( _topStack->last == NULL )
	{
		return iiUGcurTag;
	}

	/* Retain the old tag for return */

	oldtag = _topStack->tag;

	/* Pop the stack freeing the popped node */

	tnode = _topStack;
	_topStack = _topStack->last;
	_freeNode(tnode);

	return oldtag;
}

/*
** Name:	_getNode() -	Get the Next Tag Stack Element.
**
** Description:
**	Dynamically allocates the next tag stack element.
**	
** Returns:
**	{TAGNODE *}  Reference to the allocated stack element.
**	
** History:
**	10/31/84 (rlk) - First Written.
**	13-jul-87 (bab)	Changed memory allocation to use MEreqmem.
*/
static TAGNODE	*_NodeFree = NULL;

static TAGNODE	*
_getNode()
{
	register TAGNODE	*nodp;

	if ( _NodeFree == NULL )
	{
		STATUS	stat;
#define			STKBLK 32

		if ( (_NodeFree =
			(TAGNODE*)MEreqmem(0, STKBLK * sizeof(TAGNODE), TRUE,
						&stat))
				== NULL)
			return NULL;

		for (nodp = _NodeFree ; nodp < &_NodeFree[STKBLK - 1] ; ++nodp)
			nodp->last = nodp + 1;
		nodp->last = NULL;
	}

	nodp = _NodeFree;
	_NodeFree = _NodeFree->last;

	return nodp;
}
/*
** Name:	_freeNode() - Free the Specified Tag Stack Element.
**
** Description:
**	Frees the tag stack element back to the system.
**	
** Input:
**	{TAGNODE *}  The tag node to be freed.
**
** Returns:
**	{STATUS}  OK or FAIL.
**	
** History:
**	10/31/84 (rlk) - First Written.
*/
static VOID
_freeNode ( tnode )
register TAGNODE	*tnode;
{
	MEfill(sizeof(*tnode), '\0', (PTR)tnode);

	/* Attach to the free list. */

	tnode->last = _NodeFree;
	_NodeFree = tnode;
}

/*{
** Name:	FElocktag() -	Lock/Unlock a Tag Region.
**
** Description:
**	Lock/unlock the current tag region so that one client
**	can indicate to another client to use the existing tag
**	region instead of starting a new (and nested) one.
**
** Input:
**	{bool}  A boolean to turn on/off the lock.
**
** Assumptions:
**	The clients know what they are doing.  No check is really done
**	to see if there is a current tag region.  We hope so.
**
** History:
**	10/04/86 (dkh) - First written for PC.
**	06/16/87 (dkh) - Integrated into 6.0 from PC.
*/
static bool	tag_locked = FALSE;

VOID
FElocktag ( bool lock )
{
	tag_locked = lock;
}

/*{
** Name:	 FEtaglocked() - Test if the Current Tag is Locked.
**
** Description:
**	Returns whether the current tag is locked.  If the tag is locked, then
**	the client should not start a new (and nested) tag region.
**
** Returns:
**	{bool}	TRUE - If the current tag is locked.
**		FALSE - if no lock is set.
**
** Assumptions:
**	The clients know what they are doing and believes
**	what the client tells us.
**
** History:
**	10/04/86 (dkh) - First written for PC.
**	06/16/87 (dkh) - Integrated into 6.0 from PC.
*/
bool
FEtaglocked (void)
{
	return tag_locked;
}

