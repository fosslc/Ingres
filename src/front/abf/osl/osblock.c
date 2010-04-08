/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osloop.h>
#include	<osquery.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:	osblock.c -	OSL Translator Loop Block Module.
**
** Description:
**	Contains routines used to maintain and access a stack of blocks.  Blocks
**	are generated for loop constructs such as WHILE or UNLOADTABLE.
**
**	This module uses a list to implement the stack, list elements being
**	allocated using the front-end pool allocation module.  Defines:
**
**	osblock()	push block for loop.
**	osendblock()	pop block for loop.
**	osblkbreak()	generate code to break out of a block.
**	osblkqry()	return query node for QUERY block.
**	osblkutbl()	check for UNLOADTABLE block.
**
** History:
**	Revision 6.4/02
**	10/02/91 (emerson)
**		osblkSID and osblkclose replaced by osblkbreak;
**		minor modification to osblock.
**		(For bugs 34788, 36427, 40195, 40196).
**	11/07/91 (emerson)
**		Modified osblock and osblkbreak
**		(for bugs 39581, 41013, and 41014: array performance).
**
**	Revision 6.0  87/05  wong
**	Added support for query while blocks.
**
**	Revision 5.1  86/10/17  16:39:19  wong
**	Initial revision.  86/07  wong
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

PTR	ostmpid();
VOID	ostmphold();
VOID	ostmprls();
VOID	osexitunld();

/*
** Name:    BLOCK -	Block List Element.
**
** Description:
**	The definition for an element of the block stack.
**
** History:
**	05/87 (jhw) -- Added query element.
**	07/86 (jhw) -- Defined.
*/

typedef struct b {
	i4	bk_type;	/* type of block, LP_WHILE, ... */
	IGSID	*bk_sid;	/* SID for block */
	OSSYM	*bk_id;		/* identifier for block */
	OSNODE	*bk_qry;	/* node for query block */
	PTR	bk_temp;	/* ID of temp block containing this block */
	struct b *bk_next;	/* next element */
} BLOCK;

/*
** Block Stack Element Pool
**	and Block Stack Data Structures.
*/

static FREELIST	*listp = NULL;		/* free-list */
static BLOCK	*blkstack = NULL;	/* stack */

/*{
** Name:    osblock() -	Push Block For Loop.
**
** Description:
**	Pushes a block for a loop onto the block stack.
**
** Inputs:
**	type	{nat}  The type of loop for the block.
**	sid	{IGSID *}  The SID for the block.
**	label	{char *}  The label ID (if any) for the block.
**	qry	{OSNODE *}  Node for query (optional.)
**
** Side Effects:
**	Pushes a new block on the block stack.
**
** History:
**	07/86 (jhw) -- Written.
**	10/02/91 (emerson)
**		Pass pointer to new BLOCK to oslblbeg.
**		for bugs 34788, 36427, 40195, 40196.
**	11/07/91 (emerson)
**		Added calls to ostmpid and ostmphold
**		(for bugs 39581, 41013, and 41014: array performance).
*/
/*VARARGS3*/
VOID
osblock (type, sid, label, qry)
i4	type;
IGSID	*sid;
char	*label;
OSNODE	*qry;
{
    register BLOCK	*newp;

    OSSYM	*oslblbeg();

    if ((listp == NULL && (listp = FElpcreate(sizeof(BLOCK))) == NULL) ||
		(newp = (BLOCK *)FElpget(listp)) == NULL)
	osuerr(OSNOMEM, 1, ERx("osblock"));

    newp->bk_type = type;
    newp->bk_sid = sid;
    newp->bk_id = (label == NULL ? NULL : oslblbeg(label, (PTR)newp));
    newp->bk_qry = ((type == LP_QUERY || type == LP_QWHILE) ? qry : NULL);
    newp->bk_temp = ostmpid();	/* ID of enclosing temp block */
    newp->bk_next = blkstack; blkstack = newp;	/* push */

    /*
    ** "hold" enclosing temp block (in case this block turns out
    ** to call frames or procedures that release array row temps).
    */
    ostmphold();
}

/*{
** Name:    osendblock() -	Pop Block For Loop.
**
** Description:
**	Pops the topmost block from the block stack.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
osendblock ()
{
    register BLOCK	*blkp;

    /*
    ** Check for NULL here is a security precaution only.
    ** Any error should be handled by the caller.
    */
    if ((blkp = blkstack) != NULL)
    {
	if (blkp->bk_id != NULL)
	    oslblend(blkp->bk_id);

	blkstack = blkp->bk_next;	/* pop */
	FElpret(listp, blkp);
    }
}

/*{
** Name:    osblkbreak() -	Break out of Active Block.
**
** Description:
**	Generate code to break out of the specified active block
**	(the topmost block of the specified type(s)).
**	If a type of LP_LABEL is specified,
**	the active block with the specified label will be searched for.
**	More than one type (or no type) may be searched for;
**	the caller should OR together all the desired types.
**	See osloop.h for valid types.
**
**	Blocks are assumed to be followed by code to "clean up"
**	after breaking out of them, with the following omissions:
**	(1) UNLOADTABLE blocks are not followed by an ENDUNLD or ARRENDUNLD.
**	(2) QUERY blocks are not followed by a QRYBRK.
**	(3) Obviously, there's no code to clean up after a nested block
**	    (such code follows the nested block, not enclosing blocks).
**	The code generated by this routine includes code to compensate
**	for these omissions.
**
**	ENDLOOP, RESUME, VALIDATE, RETURN and EXIT semantics use this routine.
**	For ENDLOOP with a label, the non-standard type LP_LABEL is specified.
**	For ENDLOOP without a label, all standard types except LP_DISPLAY
**	(LP_NONDISPLAY) are specified.
**	For RESUME and VALIDATE, the type LP_DISPLAY is specified.
**	For RETURN and EXIT, no types (LP_NONE) are specified;
**	this causes all active blocks to be closed.
**
** Input:
**	types	{nat}	The type(s) of blocks to be searched for.
**	label	{char*}	Identifier for label.  Required iff types include
**			LP_LABEL.  In that case, an error message is printed
**			if the label is not defined or is out of scope.
**
** Returns:
**	{STATUS}	OK, or FAIL if no block was found meeting the caller's
**			specifications.  (In that case, it's up to the caller
**			to print an error message, if appropriate).
**
** History:
**	07/86 (jhw) -- Written.
**	08/87 (jhw) -- Added type support.
**	10/90 (jhw) -- Modified to pass query type to 'osqgbreak()' to generate
**		POPSUBMENU.  Part of fix for #31929.
**	10/02/91 (emerson)
**		Renamed (from osblkSID) and extensively revised
**		for bugs 34788, 36427, 40195, 40196.
**	11/07/91 (emerson)
**		Changes for bugs 39581, 41013, and 41014 (array performance):
**		Added calls to ostmprls and removed second (boolean) parm
**		to osqgbreak [it was set TRUE on the first two calls,
**		to request that the QUERY structure be freed,
**		but ostmprls now takes care of that].
**		Also call osexitunld instead of generating IL_ENDUNLD
**		(which isn't appropriate for an UNLOADTABLE of an array).
*/
PTR	oslblcheck();

/*VARARGS1*/
STATUS
osblkbreak (types, label)
register i4	types;
char		*label;
{
	register BLOCK	*blkp, *labblk = NULL;

	if (types & LP_LABEL)
	{
		labblk = (BLOCK *)oslblcheck(label);
		if (labblk != NULL)
		{
			labblk->bk_type |= LP_LABEL;
		}
	}

	for (blkp = blkstack; blkp != NULL; blkp = blkp->bk_next)
	{
		if (blkp->bk_type & types)
		{
			break;
		}
		switch (blkp->bk_type)
		{
		   case LP_UNLOADTBL:
			osexitunld();
			break;

		   case LP_QWHILE:
			osqgbreak(blkp->bk_qry);
			break;

		   case LP_QUERY:
			osqgbreak(blkp->bk_qry);
			IGgenStmt(IL_POPSUBMU, (IGSID *)NULL, 0);
			break;

		   case LP_DISPMENU:
			IGgenStmt(IL_POPSUBMU, (IGSID *)NULL, 0);
			break;
		}
	}

	if (labblk != NULL)
	{
		labblk->bk_type &= ~LP_LABEL;
	}

	if (blkp == NULL)
	{
		ostmprls((PTR)NULL);	/* release all appropriate temps */
		return FAIL;
	}

	switch (blkp->bk_type)
	{
	   case LP_UNLOADTBL:
		osexitunld();
		break;

	   case LP_QWHILE:
	   case LP_QUERY:
		osqgbreak(blkp->bk_qry);
		break;
	}

	ostmprls(blkp->bk_temp);		/* release all appropriate temps
						** in temp blocks inside the
						** block we're breaking out of.
						*/
	IGgenStmt(IL_GOTO, blkp->bk_sid, 0);	/* break out (finally!) */
	return OK;
}

/*{
** Name:    osblkqry() -	Return Query Node for Block.
**
** Description:
**	Returns the query node and block SID for the active query block
**	on the top of the block stack, if any.
**
** Outputs:
**	sid	{IGSID **}  SID reference for the query block
**			    (which is the end query SID.)
**
** Returns:
**	{OSNODE *}	The query node, which may be NULL if
**			no query block is active.
**
** History:
**	09/86 (jhw) -- Written.
*/
OSNODE *
osblkqry (sid)
IGSID	**sid;
{
    register BLOCK	*blkp;

    for (blkp = blkstack ; blkp != NULL ; blkp = blkp->bk_next)
    {
	if (blkp->bk_type == LP_QUERY || blkp->bk_type == LP_QWHILE)
	{
	    *sid = blkp->bk_sid;
	    return blkp->bk_qry;
	}
    }
    *sid = NULL;
    return NULL;
}

/*{
** Name:    osblkutbl() -	Check For Active UNLOADTABLE Block.
**
** Description:
**	Returns TRUE if an UNLOADTABLE block is active on the stack,
**	otherwise returns FALSE.
**
** Returns:
**	{bool}  TRUE if UNLOADTABLE active.
*/

bool
osblkutbl ()
{
    register BLOCK	*blkp;

    for (blkp = blkstack ; blkp != NULL ; blkp = blkp->bk_next)
    {
	if (blkp->bk_type == LP_UNLOADTBL)
	    return TRUE;
    }
    return FALSE;
}
