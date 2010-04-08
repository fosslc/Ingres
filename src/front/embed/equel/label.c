# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<equel.h>
# include	<equtils.h>
# include	<ereq.h>

/*
+* Filename:	label.c
** Purpose:	Routines to manage labels when moving in and out of
**		EQUEL blocks, their code generation and use.
**
** Defines:
**  		lbl_enter_block() -	Enter a new block - stack the old one.
**  		lbl_a_reset()	  -	Reset hi-water mark for activate blocks.
**  		lbl_exit_block()  -	Pop out of the current block.
**  		lbl_next()	  -	Returns the next label number.
**  		lbl_used()	  -	Mark the cur label "used" .
**  		lbl_gen()	  -	Gen "break" calls, return active mask.
**  		lbl_is_used()	  -	Tells whether or not the label is used.
**  		lbl_value()	  -	Returns the label of specified block.
**  		lbl_get_mode()	  -	Get mode of specified or current block.
**  		lbl_which()	  -	Turn mode into a "which" - stack to use.
**  		lbl_current()	  -	Returns the current label number.
**  		lbl_set_mode()	  -	Set the mode of the current block.
**		lbl_adset()	  -	Set the mode adjective of a block.
**		lbl_adget()	  -	Return the mode adjective of a block.
**  		lbl_init()	  -	Initialize the label manager.
**  		lbl_dump()	  -	Dump the label manager data structure.
** Locals:
-*		lbl_stk_dmp()     -	Dump individual label stack level.
** History:
**		18-jan-1985	Written (mrw)
**      12-Dec-95 (fanra01)
**              Added definitions for extracting data for DLLs on windows NT
**      18-apr-96 (thoda04)
**              Added void as return type for lbl_a_reset()
**              Cleaned up a couple of type warnings
**              Upgraded lbl_gen() to ANSI style func proto for Windows 3.1.
**	08-oct-96 (mcgem01)
**		Global data moved to eqdata.c
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** label stack definitions
*/

# define	LBL_MODNUM	4
# define	LBL_STKSIZ	10

typedef struct {
    struct lbl_stack {
	struct stk_sav {
	    i2	sav_cur;
	    i1	sav_flags;
	    i1	sav_use;
	} stk_sav[LBL_MODNUM];	/* one for each of the four kinds */
	i2	stk_mode;	/* which mode is "current" */
	i2	stk_adj;	/* mode "adjective" */
    } lbl_stack[LBL_STKSIZ];	/* the stacked info */
    struct lbl_cur {
	i2	cur_cur;	/* current value */
	i2	cur_high;	/* hi-water mark */
    } lbl_cur[LBL_MODNUM];	/* one for each of the four kinds */
} LBL_STACK;

GLOBALREF LBL_STACK	lbl_labels;
GLOBALREF i2		lbl_index;	/* index of the next free location */

/*
+* Procedure:	lbl_init
** Purpose: 	Init our state stuff.
** Notes:
**	Explicitly set the mode and adjective, even though LBLmNONE and
**	LBLaNONE are both currently zero (and should stay that way) and
**	are therefore correctly initialized by the MEfill, as otherwise
**	the code would break if these symbols were redefined.
**
** Parameters:	None
-* Returns:	None
*/

void
lbl_init()
{
    register struct lbl_stack	*l;

    MEfill( sizeof(lbl_labels), '\0', &lbl_labels );
    lbl_index = 0;
    for (l = &lbl_labels.lbl_stack[0]; l<&lbl_labels.lbl_stack[LBL_STKSIZ]; l++)
    {
	l->stk_mode = LBLmNONE;
	l->stk_adj = LBLaNONE;
    }
}

/*
+* Procedure:	lbl_enter_block
** Purpose:	Start a new block - save all the global information, and
**		push a new block on the stack.
**
** Parameters:	None
-* Returns:	None
*/

void
lbl_enter_block()
{
    register struct lbl_stack
	*l = &lbl_labels.lbl_stack[lbl_index];
    register int
	i;

    l->stk_mode = LBLmNONE;
    l->stk_adj = LBLaNONE;
    for (i=0; i<LBL_MODNUM; i++)
    {
	register struct stk_sav
		*s = &l->stk_sav[i];

	s->sav_cur = lbl_labels.lbl_cur[i].cur_cur;
	s->sav_flags = s->sav_use = 0;
    }

    if (++lbl_index >= LBL_STKSIZ)
	er_write( E_EQ0326_lblOVERFLOW, EQ_FATAL, 1, er_na(LBL_STKSIZ) );
}

/*
+* Procedure:	lbl_a_reset
** Purpose:	Reset hi-water mark for activate blocks.
**
** Parameters:	None
-* Returns:	None
*/

void
lbl_a_reset()
{
    register struct lbl_cur
    	*p = &lbl_labels.lbl_cur[LBL_INTR];

    p->cur_cur = p->cur_high = 0;
}

/*
+* Procedure:	lbl_exit_block
** Purpose:	Exit a block (pop it) and restore pervious state.
**
** Parameters:	None
-* Returns:	None
*/

void
lbl_exit_block()
{
    register struct lbl_stack
	*l;
    register struct lbl_cur
    	*p;
    register i4
	i;

    if (--lbl_index < 0)
	er_write( E_EQ0327_lblUNDERFLOW, EQ_FATAL, 0 );

    l = &lbl_labels.lbl_stack[lbl_index];
    for (i=0; i<LBL_MODNUM; i++)
    {
	register struct stk_sav
	    *s = &l->stk_sav[i];

	lbl_labels.lbl_cur[i].cur_cur = s->sav_cur;
    }
   /* restore highwater for activate interrupt values */
    p = &lbl_labels.lbl_cur[LBL_INTR];
    p->cur_high = p->cur_cur;
}

/*
+* Procedure:	lbl_next
** Purpose:	Return the next label number of this type
**
** Parameters:	which	- i4	- Type of label whose next number we want.
-* Returns:	i4		- Next label number.
*/

i4
lbl_next( which )
i4	which;
{
    register struct lbl_cur
	*l = &lbl_labels.lbl_cur[which];

    return l->cur_cur = ++l->cur_high;
}

/*
+* Procedure:	lbl_current
** Purpose:	Return the current value of the specified mode.
**
** Parameters:	which	- i4	- Mode of label whose current value we want.
-* Returns:	i4		- Current label number.
*/

i4
lbl_current( which )
i4	which;
{
    return lbl_labels.lbl_cur[which].cur_cur;
}

/*
+* Procedure:	lbl_set_mode
** Purpose:	Set the mode of the last (stacked) block.
**
** Parameters:	which	- i4	- Mode of to set the block with.
-* Returns:	None
*/

void
lbl_set_mode( which )
i4	which;
{
    if (lbl_index <= 0)
	er_write( E_EQ0327_lblUNDERFLOW, EQ_FATAL, 0 );
    lbl_labels.lbl_stack[lbl_index-1].stk_mode = (i2)which;
}

/*
+* Procedure:	lbl_used
** Purpose:	Mark the current "which" label as used in all blocks
**	  	for which it is active - only the blocks where the label
**		number is the same as the current one.
**
** Parameters:	which	- i4	- Mode of label to mark as used.
-* Returns:	None
*/

void
lbl_used( which )
i4	which;
{
    i4	cur_val,
	i;
    register struct stk_sav
	*p;

    cur_val = lbl_labels.lbl_cur[which].cur_cur;
    for (i=lbl_index-1; i>=0; i--)
    {
	p = &lbl_labels.lbl_stack[i].stk_sav[which];
      /* if it's a different label then stop marking it! */
	if (p->sav_cur != cur_val)
	    break;
	p->sav_use = 1;
    }
}

/*
+* Procedure:	lbl_is_used
** Purpose:	Return TRUE if the current label for the specified mode
**	  	has been used.
**
** Parameters:	which	- i4	- Mode of label we are finding out about.
-* Returns:	bool	- Has the current label been used under that mode ?
*/

bool
lbl_is_used( which )
i4	which;
{
    register struct lbl_stack
		*l;
    register i4	cur_val;

    l = &lbl_labels.lbl_stack[lbl_index];
    cur_val = l->stk_sav[which].sav_cur;

    for (; l >= lbl_labels.lbl_stack; l--)
    {
	if (l->stk_sav[which].sav_cur != cur_val)  /* got to a lower label */
	    return FALSE;
	if (l->stk_sav[which].sav_use)
	    return TRUE;
    }
    return FALSE;
}

/*
+* Procedure:	lbl_gen
** Purpose:	When "breaking out" of nested loops, generate the run-time
**		loop-break calls, as well as returning the level at which
**		the matching mode was found. 
**
**		Search the stack (in reverse time order) and generate 
**		appropriate "end" calls for any modes not in "mask".
**
** Parameters:	mask	- i4	- Use mask we are searching for.
**		do_gen	- bool	- Should we generate "breakout" calls ?
** Returns:	i4 	- Level at which matching mode was found, otherwise 
-*		     	  return -1 if no such mode was found.
** Example:
**		retrieve ( target list )
**		{
**		    display f 
**		    activate menuitem m:
**		    {
**			unloadtable f t (target list)
**			{
**			    if (err)
**				endretrieve - lbl_gen(LBLmRETLOOP,TRUE);
**			}
**		    }
**		    finalize
**		}
**
** History:
**	28-feb-00 (inkdo01)
**	    Added LBLmEXECLOOP for execute procedure loop exits.
*/

i4
lbl_gen( i4  mask, bool do_gen )
{
    register struct lbl_stack
	*l;
    register i4	i;

    for (i = lbl_index-1; i >= 0; i--)
    {
        l = &lbl_labels.lbl_stack[i];
	if (l->stk_mode & mask)
	    return i;

	if (!do_gen)
	    continue;

	switch (l->stk_mode)
	{
	  case LBLmRETLOOP:
	    gen_call( IIBREAK );
	    break;
	  case LBLmEXECLOOP:
	    gen_call( IIBREAK );
	    break;
	  case LBLmDISPLAY:
	    /*
	    ** DISPLAY [SUBMENU] gets its own block, either DISPLAY
	    ** or NESTMU.  Generate breakout code only when popping
	    ** out of this top block; ignore popping out of the
	    ** (subordinate) activate blocks.
	    */
	    if (l->stk_adj == LBLaDISPLAY)
		gen_call( IIENDFRM );
	    else if (l->stk_adj == LBLaNESTMU)
		gen_call( IIENDNEST );
	    break;
	  case LBLmFORMDATA:
	    gen_call( IIENDFRM );
	    break;
	  case LBLmTBDATA:
	    gen_call( IITDATEND );
	    break;
	  case LBLmTBUNLD:
	    gen_call( IITUNEND );
	    break;
	  case LBLmSUBMENU:		/* Do nothing - no loop */
	    break;
	}
    }
    return LBL_NOLEV;
}

/*
+* Procedure:	lbl_value
** Purpose:	Return the label number of the specified type at the specified
**		level.
**
** Parameters:	mode	- i4	- Type for whom we want a label number.
**		level	- i4	- Level at which we want the label number.
-* Returns:	i4		- Label number we found.
*/

i4
lbl_value( mode, level )
i4	mode,
	level;
{
    if (level < 0)
	return lbl_labels.lbl_cur[lbl_which(mode)].cur_cur;
    return lbl_labels.lbl_stack[level].stk_sav[lbl_which(mode)].sav_cur;
}

/*
+* Procedure:	lbl_which
** Purpose:	Transforms a mode into a "which" - which stack to use.
** 
** Parameters:	mode	- i4	- Mode from user.
-* Returns:	i4		- "which"stack to use.
**
** History:
**	28-feb-00 (inkdo01)
**	    Added LBLmEXECLOOP for execute procedure loop exits.
*/

i4
lbl_which( mode )
i4	mode;
{
    switch (mode)
    {
      case LBLmDISPLAY:
	return LBL_FORM;
      case LBLmRETLOOP:
      case LBLmEXECLOOP:
	return LBL_RET;
      case LBLmFORMDATA:
	return LBL_FORM;
      case LBLmTBDATA:
	/* FALL THROUGH! */
      case LBLmTBUNLD:
	/* FALL THROUGH! */
      case LBLmTBLOOP:
	return LBL_TBL;
      case LBLmSUBMENU:
	return LBL_FORM;
      default:			/* "can't happen" */
	er_write( E_EQ0328_lblUNKNOWNMOD, EQ_ERROR, 1, er_na(mode) );
	return LBL_RET;
    }
}

/*
+* Procedure:	lbl_get_mode
** Purpose:	Return the global mode of the specified level.  Modes of
**		levels are exclusive (you cannot have a block which is both
**		Retrieve and Formdata!)
**
** Parameters:	level	- i4  - Level we want the mode of.
-* Returns:	i4	     - Mode of the specified level.
**
** Notes:
**	- level -1 means the last stacked level (useless).
**	  if there's no current level then return mode 0 (none).
*/

i4
lbl_get_mode( level )
i4	level;
{
    register i4	index;

    if (level < 0)
    {
	if (lbl_index > 0)
	    index = lbl_index-1;
	else
	    return 0;
    } else
	index = level;
    return lbl_labels.lbl_stack[index].stk_mode;
}

/*
+* Procedure:	lbl_adget
** Purpose:	Return the global adjective of the specified level.
**
** Parameters:	level	- i4  - Level we want the mode of.
-* Returns:		  i4  - Adjective of the specified level.
**
** Notes:
**	- level -1 means the last stacked level.
**	  if there's no current level then return adjective LBLaNONE.
*/

i4
lbl_adget( level )
i4	level;
{
    register i4	index;

    if (level < 0)
    {
	if (lbl_index > 0)
	    index = lbl_index-1;
	else
	    return LBLaNONE;
    } else
	index = level;
    return lbl_labels.lbl_stack[index].stk_adj;
}

/*
+* Procedure:	lbl_adset
** Purpose:	Set the global adjective of the specified level.
**
** Parameters:	level	- i4  - Level for which we want to set the adjective.
**		adj	- i4  - The adjective value.
-* Returns:		  None.
**
** Notes:
**	- level -1 means the last stacked level.
**	  if there's no current level then return.
*/

VOID
lbl_adset( level, adj )
i4	level;
i4	adj;
{
    register i4	index;

    if (level < 0)
    {
	if (lbl_index > 0)
	    index = lbl_index-1;
	else
	    return;
    } else
	index = level;
    lbl_labels.lbl_stack[index].stk_adj = (i2)adj;
}

/*
+* Procedure:	lbl_dump
** Purpose:	Dump the label information for debugging.
**
** Parameters:	None
-* Returns:	None
*/

static char	*lbl_names = ERx("RFTI");

void
lbl_dump()
{
    i4		i;
    FILE	*dp = eq->eq_dumpfile;
    void	lbl_stk_dmp();

    SIfprintf( dp, ERx("\n*** Label Manager Dump ***\nHighwater:         ") );
    for (i=0; i<LBL_MODNUM; i++)
	SIfprintf( dp, ERx("%c: (h=%d, c=%d)  "),
	    lbl_names[i], lbl_labels.lbl_cur[i].cur_high,
	    lbl_labels.lbl_cur[i].cur_cur );
    SIfprintf( dp, ERx("\n") );
    for (i=lbl_index-1; i>=0; i--)
    {
	struct lbl_stack	*l = &lbl_labels.lbl_stack[i];

	SIfprintf( dp, ERx("%d: mode/adj=%02o/%02o, "),
	    i, l->stk_mode, l->stk_adj );
	lbl_stk_dmp( dp, l->stk_sav );
	SIfprintf( dp, ERx("\n") );
    }
    SIfprintf( dp, ERx("\n") );
    SIflush( dp );
}

/*
+* Procedure:	lbl_stk_dmp
** Purpose:	Dump individual label stack element.
**
** Parameters:	dp	- FILE *	- Debug output file.
**		p	- stk_sav *	- Level ptr we are currently dumping.
-* Returns:	None
*/

void
lbl_stk_dmp( dp, p )
FILE	*dp;
struct stk_sav
	*p;
{
    i4		i;

    for (i=0; i<LBL_MODNUM; i++)
    {
	SIfprintf( dp, ERx("%c(u=%d c=%d f=%x) "),
	    lbl_names[i], p->sav_use, p->sav_cur, p->sav_flags );
	p++;
    }
}
