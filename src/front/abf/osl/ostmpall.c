/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>

/**
** Name:	ostmpall.c -	OSL Translator Allocate Temporaries Module.
**
** Description:
**	Contains routines used to allocate temporaries for intermediate code
**	generation by the OSL translator.  Temporaries are allocated in a block.
**	A block is just a list of symbol table entries for the temporaries.
**
**	Defines:
**
**	ostmpinit()	initialize temporaries module.
**	ostmpnewproc()	initialize temporaries module for new proc.
**	ostmpbeg()	begin a temporary block.
**	ostmpid()	return the ID of a temporary block.
**	ostmphold()	hold complex object temps in current temporary block.
**	ostmprls()	release held temps in specified temporary blocks.
**	ostmpfreeze()	freeze temps freed by nested temp blocks.
**	ostmpend()	end a temporary block.
**	ostmppromote()	promote a temporary block.
**	ostmpalloc()	allocate a temporary data-value place-holder.
**	osdatalloc()	allocate a temporary data-value.
**	osconstalloc()	allocate a constant temporary.
**
** History:
**	Revision 5.1  86/10/17  16:41:07  wong
**	Modified from the compiler version.  07-10/86  wong
**
**	Revision 6.0  87/06  wong
**	Supports all ADT types, including DB_BOO_TYPE.
**
**	Revision 6.3/03/00  90/07  wong
**	Separated 'osdatalloc()' from 'ostmpalloc()' to distinguish between
**	temporary data and temporary place-holders.  Also renamed 'ostmpconst()'
**	as 'osconstalloc()'.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Created ostmpinit; moved initialization of NullSym
**		from osconstalloc to ostmpinit.
**		Also created ostmpnewproc.
**	08/17/91 (emerson)
**		Added missing #include <me.h>.
**	08/17/91 (emerson)
**		Removed inadvertently duplicated #include <ex.h>.
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Changes for bugs 39581, 41013, and 41014 (array performance):
**		Created ostmpid, ostmphold, and ostmprls; modified ostmpend.
**		Also replaced 3 separate stacks (tmpblkstk, plcblkstk, and
**		const_stack) with a single stack (blkstk); LTpush and LTpop
**		are no longer used (several functions were modified).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Created ostmppromote and ostmpfreeze, and made very small
**		modifications (having to do with s_tmpil and s_tmpseq)
**		in several functions (for bug 34846).
**
**	Revision 7.0
**	29-jul-92 (davel)
**		Add precision argument to both ostmpalloc() and osdatalloc().
**		Added length and precision arguments to osconstalloc().
**      20-Jun-95 (fanra01)
**              Undefined CONST prior to typedef.  Microsoft defined type.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN ILALLOC	IGallocRef();


/*
** Name:    CONST -	Constant List Entry.
**
** Description:
**	The structure definition for a constant list entry.  The constant list
**	contains entries for all active constants.  Constant temporaries (which
**	contain a constant) are re-used if the constant is active.
*/
#ifdef CONST
#undef CONST
#endif  /* CONST */
typedef struct _con
{
    DB_DT_ID	type;	/* constant type */
    ILREF	refer;	/* constant IL reference */
    OSSYM	*tmp;	/* temporary holding constant */
    struct _con	*next;

} CONST;

/*
** Name:    BLKELT -	Block Stack Element
**
** Description:
**	This structure serves as an anchor for all lists associated with
**	a temp block.
**
** History:
**	09/20/92 (emerson)
**		Added tmpend_seq member (for bug 34846).
*/
typedef struct _blk
{
    CONST	*const_top; /* constant list top when temp block was created */
    OSSYM	*tmp;	/* list of temps allocated for this block */
    OSSYM	*plc;	/* list of place-holder temps allocated for this block*/

    i4	tmpend_seq; /* number of ostmpend's done before block started */

    i4		flags;	/* flags */

# define	BLK_HELD	0x0001

			/* temps in this block whose s_tmpil field is set
			** should be "held": their reference counts should be
			** bumped by the IL_DOT or IL_ARRAYREF that sets them,
			** and they should be released (reference count
			** decremented) at exit from the block).
			*/
    struct _blk	*next;

} BLKELT;

static i4	tmpend_seq = 0;		/* bumped on each call to ostmpend */

static OSSYM	*tmpfree = NULL;	/* temp. free list */
static OSSYM	*plcfree = NULL;	/* temp. place-holder free list */

static CONST	constants = {0};	/* constants list */
static CONST	*const_top = NULL;	/* constant list top (also free list) */

static BLKELT	*blkstk = NULL;		/* block stack */

static FREELIST	*const_pool = NULL;
static FREELIST	*blkelt_pool = NULL;

static OSSYM	*NullSym = NULL;

static VOID	_freeze();
static VOID	_freeTmp();
static VOID	_mergeTmp();
static OSSYM	*_allocSym();

/*{
** Name:	ostmpinit() -	Initialize Temporaries Module.
**
** Description:
**	Initializes the temporaries module.
**	Must be called once, before any other routines in this module.
**	Currently, its only function is to allocate cell pools and
**	a symbol for Null Constant.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:  Allocate cell pools.
*/
VOID
ostmpinit()
{
	DB_DATA_VALUE	dbtype;

	dbtype.db_datatype = -DB_LTXT_TYPE;
	dbtype.db_length = 1 + DB_CNTSIZE;
	dbtype.db_prec = 0;
	NullSym = ossymfll(ERx(""), OSTEMP, &dbtype, (OSSYM *)NULL);
	NullSym->s_ilref = IGallocRef(FALSE, NullSym->s_type, NullSym->s_length,
				      NullSym->s_scale);

	const_pool  = FElpcreate(sizeof(CONST));
	blkelt_pool = FElpcreate(sizeof(BLKELT));

	if (const_pool == NULL || blkelt_pool == NULL)
	{
		osuerr(OSNOMEM, 1, ERx("ostmpinit"));
	}
}

/*{
** Name:	ostmpnewproc() - Initialize Temporaries Module for New Proc.
**
** Description:
**	Initializes the temporaries module for a main procedure (or frame)
**	or a local procedure.  Must be called once when initiating a new
**	procedure, before calling any of the remaining routines in this module.
**
**	Temporaries used by previous procedure(s) may not be reused
**	by the new procedure.  This routine ensures this by discarding
**	the list of free temporary symbols.
**
**	Note that I don't free any memory.  A more sophisticated approach
**	would be to return the now unusable symbol table entries to the
**	pool of free symbol table entries, but this requires interaction
**	with ossymalloc.  Later, maybe.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		Use single block stack instead of 3 separate stacks.
*/
VOID
ostmpnewproc()
{
	/*
	** The block stack will be empty at this point,
	** except when certain sorts of syntax errors have occurred.
	** We won't bother returning its cells to the pool in that case.
	*/
	blkstk = NULL;

	tmpfree = NULL;
	plcfree = NULL;
	const_top = NULL;	/* are there cells we can return to the pool?*/

	MEfill(sizeof(constants), '\0', &constants);
}

/*{
** Name:	ostmpbeg() -	Begin a New Temporary Block.
**
** Description:
**	Begin a new temporary block.  This pushes a new element onto
**	the block stack; its associated temp lists are set empty;
**	the constant list top is saved.
**
** History:
**	07-aug-1985 (joe)
**		First written.
**	07/86 (jhw) -- Modified for translator.
**	09/87 (jhw) -- Added constant block support.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		Use single block stack instead of 3 separate stacks.
**	09/20/92 (emerson)
**		Save tmpend_seq in new block stack element (for bug 34846).
*/
VOID
ostmpbeg()
{
	BLKELT	*blkelt;

	blkelt = (BLKELT *)FElpget(blkelt_pool);
	if (blkelt == NULL)
	{
		osuerr(OSNOMEM, 1, ERx("ostmpbeg"));
	}

	blkelt->const_top = const_top;
	blkelt->tmp = NULL;
	blkelt->plc = NULL;
	blkelt->tmpend_seq = tmpend_seq;
	blkelt->flags = 0;
	blkelt->next = blkstk;
	blkstk = blkelt;
}

/*{
** Name:	ostmpid() -	Return the ID of a Temporary Block.
**
** Description:
**	Returns an identifer for the current temporary block.
**	May be used in a subsequent call to ostmprls (which see).
**
** History:
**	11/07/91 (emerson)
**		Created for bugs 39581, 41013, and 41014.
*/
PTR
ostmpid()
{
	return (PTR)blkstk;
}

/*{
** Name:	ostmphold() -	Hold Complex Object Temps in Current Temp Block.
**
** Description:
**	Marks the temp block (and all its enclosing temp blocks) as "held":
**	IL code will be modified and generated (by _freeTemp, called from
**	ostmprls and ostmpend) so that temps whose s_tmpil field is set
**	[representing complex objects (records or arrays)] will have their
**	reference count bumped when the temp is made to point to the complex
**	object and decremented when control leaves the temp block.
**	See _freeTemp for details.
**
**	A temp block should be held if any calls to frames or 4GL procedures
**	are issued while it's active, because the called frame or procedure
**	could remove a row of an array (or clear the array) that contains
**	a record or array pointed to by a place-holder temp.
**	[Obviously we want to avoid temps that point into the ozone].
**
**	If a temp block *is* held, it must be done before any calls are made
**	to ostmprls for an enclosing block while the held block is active.
**	[Such calls are typically made on a RETURN or ENDLOOP].
**	A safe strategy is to hold a temp block for a loop construct
**	as soon as it's created.  If it turns out not contain any
**	calls to frames or procedures, we will have generated a small
**	amount of unnecessary IL, but better safe then sorry.
**
**	A related admonition: If a temp block for a loop construct is held,
**	no temps whose s_tmpil field is set may be allocated in that block
**	(nor may IL_DOTs or IL_ARRAYREFs into them be generated)
**	after the top-of-loop label has been generated.
**
** History:
**	11/07/91 (emerson)
**		Created for bugs 39581, 41013, and 41014.
*/
VOID
ostmphold()
{
	if (blkstk != NULL)
	{
		blkstk->flags |= BLK_HELD;
	}
}

/*{
** Name:	ostmprls() -	Release Held Temps in Specified Temp Blocks.
**
** Description:
**	Generates IL to releases appropriate temps upon exit from a specified
**	temp block (e.g. on an ENDLOOP or RESUME).  The generated IL releases
**	appropriate temps allocated for blocks nested within the specified
**	temp block, but it does *not* release any temps in the specified block
**	itself; it's assumed that an ENDLOOP, for example, will transfer
**	control to the IL generated by the call to ostmpend for the block.
**
** Inputs:
**	blk_id	{PTR}	ID (returned by ostmpid) of the temp block being exited,
**			or NULL if "release" IL is to be generated for *all*
**			active temp blocks (e.g. for a RETURN).
**
** History:
**	11/07/91 (emerson)
**		Created for bugs 39581, 41013, and 41014.
*/
VOID
ostmprls( blk_id )
PTR	blk_id;
{
	BLKELT	*blkelt;
	i4	blkheld = 0;

	for (blkelt = blkstk; blkelt != (BLKELT *)blk_id; blkelt = blkelt->next)
	{
		blkheld |= (blkelt->flags & BLK_HELD);
		_freeTmp(blkelt->tmp, blkheld, (OSSYM **)NULL);
		_freeTmp(blkelt->plc, blkheld, (OSSYM **)NULL);
	}
}

/*{
** Name:	ostmpfreeze() -	Freeze temps freed by nested temp blocks.
**
** Description:
**	"Freezes" all temporaries that were freed at exit from
**	already-processed temp blocks that were nested within the current
**	temp block.  The frozen temps are held out of their free lists
**	until we exit from the current temp block.
**
**	The routine should be called after generating IL that can be
**	performed (in-line or out-of-line) from points later in the current
**	temp block (so that we won't generate IL later in the current.
**	temp block that uses the same temps as the performed IL).
**
**	The current specific use is for SELECT loops:  When we process
**	the target list, we save off IL to assign from temps into
**	the ultimate targets.  (The IL may (re)compute l-values).
**	We want to emit this IL after the initial QRYSINGLE, and
**	after every QRYNEXT.  We call ostmpfreeze immediately after
**	generating and saving off the IL.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
VOID
ostmpfreeze()
{
	_freeze(blkstk->tmpend_seq, &tmpfree, &blkstk->tmp);
	_freeze(blkstk->tmpend_seq, &plcfree, &blkstk->plc);
}

/*{
** Name:	_freeze() -	Internal routine to freeze recently-freed temps
**
** Description:
**	Searches a list of free temporary symbols.
**	Symbols that have "recently" been pushed onto the free list are marked
**	as "frozen", removed from the free list, and pushed onto a temp list.
**
** Inputs:
**	tmpseq	 {i4}	A free temp with s_tmpseq >= tmpseq is "recent".
**	freelist {OSSYM **}	Pointer to free list to be searched.
**	templist {OSSYM **}	Pointer to temp list onto which frozen symbols
**				are pushed.
** History:
**	09/20/92 (emerson)
**		Created.  (For bug 34846).
*/
static VOID
_freeze ( tmpseq, freelist, templist )
i4	tmpseq;
OSSYM	**freelist;
OSSYM	**templist;
{
	register OSSYM	*lt, *lt_prev;

	for ( lt_prev = NULL, lt = *freelist;
	      lt != NULL && lt->s_tmpseq >= tmpseq;
	      lt_prev = lt, lt = lt->s_next )
	{
		lt->s_ref |= OS_TMPFROZEN;
		lt->s_tmpil = NULL;		/* overlays s_tmpseq! */
	}

	if (lt_prev != NULL)
	{
		lt_prev->s_next = *templist;
		*templist = *freelist;
		*freelist = lt;
	}
}

/*{
** Name:	ostmpend() -	End a Temporary Block.
**
** Description:
**	Ends a temporary block.  Pops the current element off the block stack
**	and merges its associated temp lists into the corresponding free lists.
**	The constant list is restored to what is was at the corresponding
**	ostmpbeg [should elements be returned to the cell pool?]
**
**	Intermediate language code is generated to free dynamic string temps
**	and query temps, and to hold and free complex object temps whose s_tmpil
**	field is set (see _freeTmp for details).
**
** History:
**	07-aug-1985 (joe)
**		First written
**	07/86 (jhw) -- Modified for translator.
**	09/87 (jhw) -- Added constant block support.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		Use single block stack instead of 3 separate stacks.
**		Interface to _freeTmp has changed slightly.
**	09/20/92 (emerson)
**		Bump tmpend_seq (for bug 34846).
*/
VOID
ostmpend()
{
	BLKELT	*blkelt = blkstk;
	i4	blkheld = (blkelt->flags & BLK_HELD);

	const_top = blkelt->const_top;

	_freeTmp(blkelt->tmp, blkheld, &tmpfree);
	_freeTmp(blkelt->plc, blkheld, &plcfree);

	++tmpend_seq;

	blkstk = blkelt->next;

	if (blkstk != NULL)
	{
		blkstk->flags |= blkheld;
	}

	_VOID_ FElpret(blkelt_pool, (PTR)blkelt);
}

/*{
** Name:	_freeTmp() -	Internal routine to free a list of temps.
**
** Description:
**	Searches a list of temporary symbols and generates IL to free
**	dynamic string and query temporary symbols (IL_DYCFREE or IL_QRYFREE).
**
**	If the temp list has been "held" (by a call to ostmphold),
**	then each temporary symbol in the list whose s_tmpil field is set
**	will receive special processing: the IL_DOT or IL_ARRAYREF
**	instruction pointed to by s_tmpil will have its ILM_HOLD_TARG
**	modifier bit set (which will cause the object pointed to by
**	the temp to be held), and an IL_RELEASEOBJ instruction
**	will be generated for the temp.
**
**	Optionally adds the temp list to a free list.  (This includes
**	resetting appropriate fields in the temps in the list).
**
** Inputs:
**	templist {OSSYM *}	Temp list to be searched.
**	blkheld	{nat}		Has the temp list been held?
**	free	{OSSYM **}	Pointer to free list to which the temp list
**				is to be added, or NULL.
** History:
**	11/07/91 (emerson)
**		Added this description.  Modified the interface slightly
**		to make it callable from the new ostmprls as well as ostmpend.
**		Removed the LTpop of the now-defunct temp list stack.
**		Added logic to hold and release specified object temps
**		(for bugs 39581, 41013, and 41014).
**	09/20/92 (emerson)
**		Add logic to handle "frozen" temporaries.
**		Set new s_tmpseq member from tmpend_seq.  (For bug 34846).
*/
static VOID
_freeTmp ( templist, blkheld, free )
OSSYM	*templist;
i4	blkheld;
OSSYM	**free;
{
	register OSSYM	*lt = templist;

	if (templist == NULL)
	{
		return;
	}

	for (;;)
	{
		if (!(lt->s_ref & OS_TMPFROZEN))
		{
			if (lt->s_type == DB_DYC_TYPE && lt->s_length == 0)
			{
				IGgenStmt( IL_DYCFREE, (IGSID *)NULL,
					   1, lt->s_ilref );

				if (free != NULL)
				{
					lt->s_type = DB_VCH_TYPE;
				}
			}
			else if (lt->s_type == DB_QUE_TYPE)
			{
				IGgenStmt( IL_QRYFREE, (IGSID *)NULL,
					   1, lt->s_ilref );
			}

			if (lt->s_tmpil != NULL)
			{
				if (blkheld)
				{
					*((IL *)lt->s_tmpil) |= ILM_HOLD_TARG;
					IGgenStmt( IL_RELEASEOBJ, (IGSID *)NULL,
						   1, lt->s_ilref );
				}
			}
		}
		if (free != NULL)
		{
			lt->s_ref &= ~OS_TMPFROZEN;
			lt->s_tmpseq = tmpend_seq;	/* overlays s_tmpil! */
		}

		if (lt->s_next == NULL)
		{
			break;
		}
		lt = lt->s_next;
	}

	if (free != NULL)
	{
		lt->s_next = *free;
		*free = templist;
	}
}

/*{
** Name:	ostmppromote() -	Promote a Temporary Block.
**
** Description:
**	Ends a temporary block.  Pops the current element off the block stack
**	and pushes its associated temp lists onto the corresponding temp lists
**	of an ancestor temp block (the parent if depth = 1, grandparent
**	if depth = 2, etc).
**
**	The constant list is restored to what is was at the corresponding
**	ostmpbeg.  [We're being conservative here; further refinements could
**	probably be made].
**
**	No intermediate language code is generated.
**
** Inputs:
**	depth	{nat}	The depth of the ancestor block with which
**			this block's temp lists will be merged.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
VOID
ostmppromote(depth)
i4	depth;
{
	BLKELT	*blkelt = blkstk;
	i4	blkheld = (blkelt->flags & BLK_HELD);

	const_top = blkelt->const_top;

	while (depth > 0)
	{
		if (blkelt == NULL)
		{
			osuerr(OSBUG, 1, ERx("ostmppromote"));
		}
		blkelt = blkelt->next;
		--depth;
	}

	_mergeTmp(blkstk->tmp, &blkelt->tmp);
	_mergeTmp(blkstk->plc, &blkelt->plc);

	blkelt = blkstk;
	blkstk = blkelt->next;

	if (blkstk != NULL)
	{
		blkstk->flags |= blkheld;
	}

	_VOID_ FElpret(blkelt_pool, (PTR)blkelt);
}

/*{
** Name:	_mergeTmp() -	Internal routine to merge two list of temps.
**
** Description:
**	Adds one temp list to the beginning of another temp list.
**
** Inputs:
**	templist {OSSYM *}	Temp list to be added.
**	targ	{OSSYM **}	Pointer to temp list to which
**				the first temp list is to be added.
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
static VOID
_mergeTmp ( templist, targ )
OSSYM	*templist;
OSSYM	**targ;
{
	register OSSYM	*lt;

	if (templist == NULL)
	{
		return;
	}

	for (lt = templist; lt->s_next != NULL; lt = lt->s_next);

	lt->s_next = *targ;
	*targ = templist;
}

/*{
** Name:	ostmpalloc() -	Allocate a Temporary Place Holder
**					in the Current Block.
** Description:
**	Allocate a temporary data value place holder in the current block.  This
**	adds a symbol table entry for the temporary to the list of those
**	current.  A block must be active (i.e., 'ostmpbeg()' must have been
**	called.)
**
** Inputs:
**	type	{DB_DT_ID}  Type of the temporary to allocate.  This
**				is a DB data type defined in "dbms.h".
**	len	{nat}	Length of the temporary to allocate.
**	prec	{i2}	Precision of the temporary to allocate.
**
** Returns:
**	{OSSYM *}  Symbol table node for the temporary (reference.)
**
** History:
**	07/90 (jhw) -- Written.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		Use single block stack instead of 3 separate stacks.
**	16-jun-92 (davel)
**		add decimal type mapping; also change switch to abs(type).
**	29-jul-92 (davel)
**		add precision argument.
*/
OSSYM *
ostmpalloc ( type, len, prec )
register DB_DT_ID	type;
i4  len;
i2  prec;
{
	register OSSYM	*s;
	i4		dyc_len;
	DB_DATA_VALUE	dbtype;

	if (blkstk == NULL)
	{
		osuerr(OSBUG, 1, ERx("ostmpalloc"));
	}

	switch (abs(type))
	{
	  case DB_NODT:
	  /* error support */
		dbtype.db_datatype = DB_INT_TYPE;
		dbtype.db_length = sizeof(i4);
		break;
	  case DB_BOO_TYPE:
	  /* boolean implementation */
		dbtype.db_datatype = DB_INT_TYPE;
		dbtype.db_length = sizeof(i1);
		break;
	  case DB_DYC_TYPE:
		dbtype.db_datatype =
				AFE_NULLABLE(type) ? -DB_VCH_TYPE : DB_VCH_TYPE;
		dyc_len = ADE_LEN_UNKNOWN;
		dbtype.db_length = 0;
		break;
	  default:
		dbtype.db_datatype = type;
		dbtype.db_length = len;
		break;
	} /* end switch - type mapping */

	dbtype.db_prec = prec;

#ifdef OSL_TRACE
        SIprintf("ostmpalloc: dbtype/len/prec = %d %d 0x%04x\n",
                dbtype.db_datatype, dbtype.db_length, dbtype.db_prec
                );
#endif /* OSL_TRACE */

	s = _allocSym(FALSE, &dbtype, &plcfree);

#ifdef OSL_TRACE
        SIprintf("ostmpalloc: symbol allocated: ilref=%d\n", 
                s->s_ilref
                );
#endif /* OSL_TRACE */

	/* Initialize for dynamic strings */
	if (type == DB_DYC_TYPE)
	{
		IGgenStmt(IL_DYCALLOC, (IGSID *)NULL,
			1, s->s_ilref, (ILREF)max(dyc_len,0)
		);
		/* Hack for now?
		if (dyc_len == 0 || dyc_len == ADE_LEN_UNKNOWN)
		*/
		s->s_type = DB_DYC_TYPE;
	}

	/* add to current temp. list */
	s->s_next = blkstk->plc;
	blkstk->plc = s;

	return s;
}

/*{
** Name:	osdatalloc() -	Allocate a Temporary Data Value
**					in the Current Block.
** Description:
**	Allocate a temporary data value in the current block.  This adds a
**	symbol table entry for the temporary to the list of those current.  A
**	block must be active (i.e., 'ostmpbeg()' must have been called.)
**
** Inputs:
**	type	{DB_DT_ID}  Type of the temporary to allocate.  This
**				is a DB data type defined in "dbms.h".
**	len	{nat}	Length of the temporary to allocate.  (Non-zero!)
**	prec	{i2}	Precision of the temporary to allocate.
**
** Returns:
**	{OSSYM *}  Symbol table node for the temporary (reference.)
**
** History:
**	07-aug-1985 (joe)
**		First written.
**	03-oct-1985 (agh)
**		Special-cased type N_OLRET, since Equel doesn't know
**		about the OL_RET structure definition.
**	07/86 (jhw) -- Modified for OSL translator.
**	06/87 (jhw) -- Supports ADTs.
**	09/89 (jhw) -- Modified Nullable types (was -DB_*_TYPE) for
**			portability.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		Use single block stack instead of 3 separate stacks.
**	26-jun-92 (davel)
**		added precision as a third argument.
*/
OSSYM *
osdatalloc ( type, len, prec )
register DB_DT_ID   type;
i4	    len;
i2	    prec;
{
	register OSSYM	*s;
	i4		dyc_len;
	DB_DATA_VALUE	dbtype;

	if (blkstk == NULL)
	{
		osuerr(OSBUG, 1, ERx("osdatalloc"));
	}

	switch (abs(type))
	{
	  case DB_NODT:
	  /* error support */
		dbtype.db_datatype = DB_INT_TYPE;
		dbtype.db_length = sizeof(i4);
		break;
	  case DB_BOO_TYPE:
	  /* boolean implementation */
		dbtype.db_datatype = DB_INT_TYPE;
		dbtype.db_length = sizeof(i1);
		break;
	  case DB_DYC_TYPE:
		dbtype.db_datatype =
				AFE_NULLABLE(type) ? -DB_VCH_TYPE : DB_VCH_TYPE;
		dyc_len = len == 0 ? ADE_LEN_UNKNOWN : len;
		dbtype.db_length = 0;
		break;
	  default:
		dbtype.db_datatype = type;
		dbtype.db_length = len;
		break;
	} /* end switch - type mapping */

	dbtype.db_prec = prec;

#ifdef OSL_TRACE
        SIprintf("osdatalloc: dbtype/len/prec = %d %d 0x%04x\n",
                dbtype.db_datatype, dbtype.db_length, dbtype.db_prec
                );
#endif /* OSL_TRACE */

	s = _allocSym(TRUE, &dbtype, &tmpfree);

#ifdef OSL_TRACE
        SIprintf("osdatalloc: symbol allocated: ilref=%d\n", 
                s->s_ilref
                );
#endif /* OSL_TRACE */

	/* Initialize for dynamic strings */
	if (type == DB_DYC_TYPE)
	{
		IGgenStmt(IL_DYCALLOC, (IGSID *)NULL,
			1, s->s_ilref, (ILREF)max(dyc_len,0)
		);
		/* Hack for now?
		if (dyc_len == 0 || dyc_len == ADE_LEN_UNKNOWN)
		*/
			s->s_type = DB_DYC_TYPE;
	}

	/* add to current temp. list */
	s->s_next = blkstk->tmp;
	blkstk->tmp = s;

	return s;
}

/*{
** Name:	_allocSym() -	Internal routine to allocate a temp symbol
**
** Description:
**	Searches a specified free list for an available temp symbol
**	of a specified type and length.  If an appropriate temp symbol
**	is found, it is extracted from the free list.  If none is found,
**	a temp symbol is allocated.
**
** Inputs:
**	needsdata {bool}	Is this *not* a place-holder temp?
**	dbdv	{DB_DATA_VALUE*} A dbdv containing the desired type and length.
**	free	{OSSYM **}	Pointer to free list to be searched.
**
** Returns:
**	{OSSYM *}  Symbol table node for the temporary (reference.)
**
** History:
**	11/07/91 (emerson)
**		Added this description.
**	09/20/92 (emerson)
**		Clear s_tmpil before exiting, because s_tmpseq now overlays it.
**		(s_tmpil is for allocated temps and s_tmpseq is for free temps).
**	21-apr-94 (connie) Bug #61028
**		Need to match length in addition to datatype even for place
**		holder temp, otherwise ADE will reject incompatible length.
*/
static OSSYM *
_allocSym ( needsdata, dbdv, free )
bool			needsdata;
register DB_DATA_VALUE	*dbdv;
OSSYM			**free;
{
	register OSSYM	*s = NULL;

	if ( *free != NULL )
	{
		register OSSYM	**last;

		last = free;
		for ( s = *free ; s != NULL ; s = s->s_next )
		{
			/* fix 61028: need to match length in addition to
			** datatype, otherwise ADE will reject it
			*/
			if ( s->s_type == dbdv->db_datatype
			  && dbdv->db_length == s->s_length
			  && (!needsdata || dbdv->db_prec == s->s_scale )
			   )
			{
				*last = s->s_next;  /* extract from free list */
				break;
			}
			last = &s->s_next;
		}
	}

	if ( s == NULL )
	{ /* none free, allocate one */
		i4	len;

		s = ossymfll(ERx(""), OSTEMP, dbdv, (OSSYM *)NULL);
		len = dbdv->db_length;
		/* Hack! */
		if ( dbdv->db_length == ADE_LEN_UNKNOWN )
		{
		    switch ( AFE_DATATYPE(dbdv) )
		    {
		    case DB_VCH_TYPE:
		    case DB_TXT_TYPE:
			len = DB_GW4_MAXSTRING + DB_CNTSIZE;
			break;
		    case DB_CHR_TYPE:
		    case DB_CHA_TYPE:
			len = DB_GW4_MAXSTRING;
			break;
		    }
		    if ( AFE_NULLABLE(dbdv->db_datatype) )
			len += 1;
		}
		/* end Hack! */
		s->s_ilref = IGallocRef(!needsdata, s->s_type, len, s->s_scale);
	}

	s->s_tmpil = NULL;  /* was overlaid by s_tmpseq when temp in free list*/
	return s;
}

/*{
** Name:	osconstalloc() -	Allocate a Constant Temporary.
**
** Description:
**	Allocates a temporary place-holder for a constant and generates code to
**	place the constant in it.
**
** Input:
**	type	{DB_DT_ID}	Constant type.
**	length	{nat}  		Constant length.
**	prec	{i2}  		Constant precision.
**	value	{char *}	Constant value.
**
** Returns:
**	{OSSYM *}  Temporary symbol containing constant value.
**
** History:
**	09/87 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		moved logic to initialize NullSym to ostmpinit.
**	11/07/91 (emerson)
**		Modified for bugs 39581, 41013, and 41014:
**		moved logic to initialize CONST cell pool to ostmpinit.
**	29-jul-92 (davel)
**		Added length and precision agruments.
*/
OSSYM *
osconstalloc (type, length, prec, value)
DB_DT_ID	type;
i4		length;
i2		prec;
char		*value;
{
    register OSSYM	*tmp;

    if ( !AFE_NULLABLE(type) )
    { /* non-Null constant */
	ILREF		cref = IGsetConst(type, value);
	register CONST	*cp;

	for (cp = &constants ; const_top != NULL && cp != const_top->next ;
		cp = cp->next)
	{ /* search for constant in active list */
	    if (cp->type == type && cp->refer == cref)
	    { /* re-use constant temporary reference */
		tmp = cp->tmp;
		break;
	    }
	}

	if (const_top == NULL || cp == const_top->next)
	{ /* allocate new constant */

	    if (cp == NULL)
	    {
		cp = (CONST *)FElpget(const_pool);
		if (cp == NULL)
	        {
		    osuerr(OSNOMEM, 1, ERx("osconstalloc"));
	        }
	    }

	    /* Allocate constant temporary place holder */
	    tmp = ostmpalloc(type, length, prec);
	    IGgenStmt(IL_MOVCONST, (IGSID *)NULL, 2, tmp->s_ilref, cref);

	    /* Add to constants list */
	    cp->type = type;
	    cp->refer = cref;
	    cp->tmp = tmp;

	    if (const_top != NULL && const_top->next == NULL)
	    { /* add new one to end of list */
		const_top->next = cp;
		cp->next = NULL;
	    }

	    const_top = cp;	/* push on constants list */
	}
    }
    else
    { /* Null constant */
	IGgenStmt(IL_MOVCONST, (IGSID *)NULL, 2, NullSym->s_ilref, (ILREF)0);
	tmp = NullSym;
    }
    return tmp;
}
