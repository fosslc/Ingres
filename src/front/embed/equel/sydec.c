# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equtils.h>
# include	<eqsym.h>
# include	<equel.h>
# include	<eqlang.h>
# include	<ereq.h>

/*
+* Filename:	sydeclare.c
** Purpose:	Common symbol table declaration/lookup routines.
**
** Defines:
** (Data):
**	syHashtab	- The hash table.
**	syNamtab	- Pointer to the name table.
**	sym_separator	- Pointer to string separating record components.
**	sym_forward	- If TRUE, then record names go top-down.
**	symHint		- Redeclaration hint structure (for ESQL).
** (Routines):
**	sym_init	- Initialize the symbol table.
**	sym_lookup	- Lookup (or install) a name in the symbol table.
**	sym_find	- Find a name in the symbol table but don't install it.
**	sym_resolve	- Find an entry with a specified parent.
**	sym_which_parent- Find what would be the parent of a declaration here.
**	sym_install	- Install an entry into its family tree.
**	sym_is_redec	- Test an entry to see if it is a redeclaration.
**	sym_parentage	- Build up a name.
**	sym_name_print 	- Turn a "var reference" into a printable name string.
**	sym_hint_type	- Let us know what the type of the next entry will be.
** (Local):
**	sym_rparentage	- Build up a name forwards (all but COBOL).
**	sym_eqname	-  A fast name compare.
**	sym_store_name	- Stash away a name in the name table.
-*	sym_hash	- Hash a name.
** Notes:
**
** History:
**	12-Oct-85	- Initial version (mrw)
**      12-Jun-86	- Whitesmiths cleanup (saw)
**	29-jun-87 (bab)	Code cleanup.
**	15-sep-93 (sandyd)
**		Fixed type mismatch on MEcopy() arguments.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	4-apr-1994 (teresal)
**		Add ESQL/C++ support - was going to add EQ_CPLUSPLUS to 
**		case statement in sym_init() routine, but case stmt
**		wasn't doing ANYTHING for any of the languages except
**		for COBOL. Cleaned up useless code.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      18-apr-96 (thoda04)
**          Added void return type to sym_hint_type().
**          Upgraded to ANSI style function prototypes for Windows 3.1: 
**             sym_init(), sym_lookup().
**	08-oct-96 (mcgem01)
**	    Global data moved to eqdata.c
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF SYM	*syHashtab[];

GLOBALREF STR_TABLE
		*syNamtab;
GLOBALREF char	*sym_separator;
GLOBALREF bool	sym_forward;
GLOBALREF struct symHint
		symHint;
syNAME		*sym_store_name();
static syNAME	*anonym ZERO_FILL;
static bool	ign_case = FALSE;

# define SY_NOLEVEL	(-1)		/* any level matches */

/*
+* Procedure:	sym_init
** Purpose:	Initialize the symbol table.
** Parameters:
**	ignore_case	- bool	- If TRUE then ignore case while hashing.
** Return Values:
-*	None.
** Notes:
**	- Really just initialize the name table (getting a new one if needed).
**	- Makes things serially reusable.
**	- If "ignore_case" is TRUE (non-FALSE) then lower-case while hashing
**	  (but store the name as given).
**
** Imports modified:
**	Nothing.
*/

void
sym_init( bool ignore_case )
{
    if (eq_islang(EQ_COBOL))
    {
	sym_separator = ERx(" IN ");
	sym_forward = FALSE;
    }

    if (syNamtab)
	str_free( syNamtab, NULL );
    else
	syNamtab = str_new( 2048 );
    anonym = sym_store_name( ERx("<anonym>"), 8 );
    ign_case = ignore_case != FALSE;
    sym_sinit();		/* initialize the scope stack */

  /* throw away the entire symbol table */
    sym_q_empty();
}

/*
+* Procedure:	sym_lookup
** Purpose:	Lookup (or install) a name in the symbol table.
** Parameters:
**	name	- char *	- The name to lookup.
**	install	- bool		- If TRUE then install it, else find it.
**	use	- i4		- The "use" bits (st_flags).
**	block	- i4		- The block at which it is being defined.
**	reclev	- i4		- The record level of definition.
** Return Values:
-*	SYM *	- Pointer to a symbol table entry for this name.
** Notes:
**	- Given a name find the most visible instance of it in the symbol table.
**	- If INSTALL then create a new entry, otherwise return the found one,
**	  if any.  Thus INSTALL differentiates between "lookup" and "declare".
**	- If not INSTALL then we don't care about the USE bits, but all
**	  should be off anyway.  Also don't care about BLOCK.  This lets
**	  sym_find pass in 0 for these parameters.
**	- Creating a new entry includes storing its name.
**	- Empty names are never stored.
**
** Imports modified:
**	Can change the symbol table.
*/

SYM *
sym_lookup( char *name, bool install, i4  use, i4  block )
{
    u_i2	h, len;
    register SYM
		*s,
		*new;
    bool	sym_eqname();
    SYM		*sym_q_new();

    dbDEBUG(xdbLEV7, (ERx("sym_lookup(%s, block=%d, use=%d, "), name, block, use));
    dbDEBUG(xdbLEV7, (ERx("%s) "), install ? ERx("declare"):ERx("lookup")));

  /* anonymous entry? */
    if (*name == '\0')
    {
	static u_i2	next_anon = 0;

      /*
       * this is an anonymous entry; therefore this must be a declaration.
       * it always creates a new entry.
       */
	if (install != TRUE)
	    er_write( E_EQ0256_syLOOKANON, EQ_ERROR, 0 );
	h = next_anon++;
	next_anon %= syHASH_SIZ;
      /* get a new entry and put it at the head of this chain */
	new = sym_q_new();
	new->st_hash = syHashtab[h];
	syHashtab[h] = new;
	new->st_name = anonym;
	new->st_type = NULL;
	new->st_block = block;	/* caller will reset block, but I don't care! */
	new->st_btype = new->st_dims = new->st_indir = new->st_dsize = 0;
	new->st_flags = 0;	/* anonym is never EXTERN */
	new->st_space = 0;
	new->st_prev = new->st_parent = new->st_child = new->st_sibling = NULL;
	new->st_value = 0;
	new->st_vislim = 0;
	new->st_reclev = 0;
	new->st_hval = h;
	dbDEBUG( xdbLEV7, (ERx("new=0x%p, hash=0x%p\n"), new, new->st_hash) );
	return new;
    } /* if anonymous */

    h = sym_hash( name );
    len = STlength( name );
  /* look for name on the appropriate hash chain */
    for (s=syHashtab[h]; s; s=s->st_hash)
    {
	dbDEBUG( xdbLEV7, (ERx("s=0x%p : next=0x%p : "), s, s->st_hash) );
	dbDEBUG( xdbLEV7, (ERx("name=%s, "), sym_str_name(s)) );
	if (sym_eqname(name,len,s->st_name))
	{
	    dbDEBUG( xdbLEV7, (ERx("match\n")) );

	    if (!install)
		return s;		/* we wuz looking for you */

	    /*
	    ** Make a new entry.
	    ** This is a redeclaration if previous was in same block,
	    ** but our caller will check and complain if so.
	    */

	  /* get a new entry and link it in at the head of the chain */
	    new = sym_q_new();
	    new->st_hash = syHashtab[h];
	    syHashtab[h] = new;
	  /* fill in useful fields */
	    new->st_name = s->st_name;	/* share name fields */
	    new->st_type = NULL;
	    new->st_prev = s;		/* previous use of this name */
	    new->st_flags = use;	/* will be reset by caller */
	    new->st_block = block;	/* will be reset by caller */
	  /* default fields */
	    new->st_parent = new->st_child = new->st_sibling = NULL;
	    new->st_value = 0;
	    new->st_vislim = 0;		/* vislim 0 means undefined */
	    new->st_reclev = 0;
	    new->st_hval = h;		/* remember for convenience */
	    new->st_btype = new->st_dims = new->st_indir = 0;
	    new->st_space = new->st_dsize = 0;
	    dbDEBUG(xdbLEV7, (ERx("new=0x%p, hash=0x%p\n"), new, new->st_hash));
	    return new;
	} /* if -- names matched */
    } /* for -- look for name on hash chain */

    dbDEBUG( xdbLEV7, (ERx("no match\n")) );
  /* not on this chain */
    if (!install)
	return NULL;		/* not found */

  /* get a new entry and put it at the head of this chain */
    new = sym_q_new();
    new->st_hash = syHashtab[h];
    syHashtab[h] = new;
    new->st_type = NULL;
    new->st_name = sym_store_name( name, len );
    new->st_flags = use;
    new->st_block = block;
    new->st_btype = new->st_dims = new->st_indir = new->st_dsize = 0;
    new->st_space = 0;
    new->st_prev = new->st_parent = new->st_child = new->st_sibling = NULL;
    new->st_value = 0;
    new->st_vislim = 0;
    new->st_reclev = 0;
    new->st_hval = h;
    dbDEBUG( xdbLEV7, (ERx("new=0x%p, hash=0x%p\n"), new, new->st_hash) );
    return new;
}

/*
+* Procedure:	sym_find
** Purpose:	Find a name in the symbol table -- don't ever install it.
** Parameters:
**	name	- char *	- The name to find.
** Return Values:
-*	SYM *	- A pointer to the found entry, or NULL if none.
** Notes:
**	Don't generate a new entry; this is always just a lookup.
**
** Imports modified:
**	None.
*/

SYM *
sym_find( name )
char	*name;
{
register SYM	*s;
    
    s = sym_lookup( name, FALSE, 0, 0 );
#ifdef	xdbDEBUG
    dbDEBUG( xdbLEV8, (ERx("sym_find(%s)=0x%p\n"), name, s) );
    if (s)
	dbDEBUG( xdbLEV8, (ERx("vislim=%d"), s->st_vislim) );
    dbDEBUG( xdbLEV8, (ERx("\n")) );
#endif	/* xdbDEBUG */
    if (s && s->st_vislim != 0)
	return s;
    return NULL;
}

/*
+* Procedure:	sym_resolve
** Purpose:	Find the entry referenced by a name with the specified parent.
** Parameters:
**	parent	- SYM *		- Parent context.
**	name	- char *	- The name to look up.
**	closure	- i4		- Closure level (for visibility).
**	use	- i4		- syFisTYPE or syFisVAR.
** Return Values:
-*	SYM *	- Pointer to entry for that name; NULL if none.
** Notes:
**	- Find the "real" parent (slide to the end of the type chain),
**	  then find a visible entry of the correct type with that parent.
**	- TAG lookups don't require that the parents match (for C).
**
** Imports modified:
**	None.
*/

SYM *
sym_resolve( parent, name, closure, use )
SYM	*parent;		/* parent context */
char	*name;			/* the name to look up */
i4	closure;		/* closure level (for visibility) */
i4	use;			/* syFisTYPE or syFisVAR */
{
    register SYM
		*s, *p;

    dbDEBUG( xdbLEV7, (ERx("sym_resolve(%s)\n"), name) );
    s = sym_find( name );
    if (s == NULL)
	return NULL;
    use &= syFuseMASK;		/* ignore base & const etc, bits */
    dbDEBUG( xdbLEV7, (ERx("use = 0x%x "), use) );

    dbDEBUG( xdbLEV7, (ERx("parent was 0x%p "), parent) );
    if (parent)
    {
	while (parent->st_type)
	    parent = parent->st_type;
    }
    dbDEBUG( xdbLEV7, (ERx("parent is 0x%p "), parent) );

  /* find a visible entry of the correct type with the correct parent */
    for (p=s; p && syIS_VIS(p,closure); p=p->st_prev)
    {
	dbDEBUG( xdbLEV7, (ERx("p->st_parent=0x%p, "), p->st_parent) );
	dbDEBUG( xdbLEV7, (ERx("syFuseOF(p)=0x%x, "), syFuseOF(p)) );
        /*
         * if correct use and either:
         *	parents matched
	 * or
	 *	this is a tag lookup
	 * then
	 *	we found it.
	 * this is so that "struct tag var" works even if "struct tag"
	 * was defined nested in some other structure.
	 * This (as well as syFisTAG in general) is really here for C.
	 */
	if (((p->st_parent==parent) || syBitAnd(use,syFisTAG))
	  && syBitAnd(syFuseOF(p),use))
	    break;
    }

  /* did we find one? */
    if (p == NULL || ((p->st_parent != parent) && !syBitAnd(use,syFisTAG))
      || !syIS_VIS(p,closure) || !syBitAnd(syFuseOF(p),use))
    {
#ifdef	xdbDEBUG
	dbDEBUG( xdbLEV7, (ERx("use=%x, closure=%d, "), use, closure) );
	if (p)
	{
	    dbDEBUG( xdbLEV7, (ERx("p=0x%p, parent=0x%p, "), p, p->st_parent) );
	    dbDEBUG( xdbLEV7, (ERx("vislim=%d, "), p->st_vislim) );
	    dbDEBUG( xdbLEV7, (ERx("IsVis=%d, "), syIS_VIS(p,closure)) );
	    dbDEBUG( xdbLEV7, (ERx("UseOf(p)=%x\n"), syFuseOF(p)) );
	} else
	    dbDEBUG( xdbLEV7, (ERx("p==NULL\n")) );
#endif	/* xdbDEBUG */
	return NULL;
    }

  /* yes, return it */
    dbDEBUG( xdbLEV7, (ERx("p=0x%p\n"), p) );
    return p;
}

/*
+* Procedure:	sym_which_parent
** Purpose:	Find the parent of an entry with the given level.
** Parameters:
**	sym_start	- SYM *	- The head of the current family tree.
**	rec_level	- i4	- The record (field) level of this entry.
** Return Values:
-*	SYM *	- The desired parent.
** Notes:
**	Walk down from the head of the family tree, looking for an entry
**	with the same or higher level.  If one at the same level then return
**	the last sibling's parent; else return the parent of the first (or only)
**	child of the previous level.
**
** Imports modified:
**	None.
*/

SYM *
sym_which_parent( sym_start, rec_level )
SYM	*sym_start;
i4	rec_level;
{
    register SYM
		*p;

    /*
    ** find the correct place in the ancestry tree
    */

  /* if no parent, or if we are top level, return no parent */
    if ((p = sym_start) == NULL)
	return p;
    if (rec_level <= 0)
	return NULL;

    for (;;)
    {
      /* if the new level is below us */
	if (rec_level >= p->st_reclev)
	{
	  /* find the last at this level */
	    while (p->st_sibling)
		p = p->st_sibling;
	  /* if at this level then just link it in here */
	    if (rec_level == p->st_reclev)
	    {
		return p->st_parent;
	    } else		/* rec_level > p->st_reclev */
	    {
	      /* it's below us */
		if (p->st_child)	/* children below us */
		{
		  /* there are children, so step down and try again */
		    p = p->st_child;
		    continue;
		} else
		{
		  /* no children, so link us in as first child */
		    return p;
		}
	    }
	} else				/* rec_level < p->st_reclev */
	{
	  /*
	   * huh? correct level is above us, so the user skipped a level
	   * going down and has now popped up to a missing level.
	   * we act like he gave us the current level number.
	   * Only possible for COBOL and PL/1.
	   */
	    while (p->st_sibling)
		p = p->st_sibling;
	    return p->st_parent;
	}
    }
}

/*
+* Procedure:	sym_install
** Purpose:	Install an entry into the correct place in its family tree.
** Parameters:
**	s		- SYM *	- The entry to install.
**	sym_start	- SYM *	- The head of the current family tree.
**	rec_level	- i4	- The record (field) level of this entry.
**	elder		- SYM **- To be set to point to the elder sibling.
** Return Values:
-*	bool	- Now always returns TRUE (historical).
** Notes:
**	Walk down from the head of the family tree, looking for an entry
**	with the same or higher level.  If one at the same level then link
**	this in as the last sibling; else install as the first (or only)
**	child of the previous level.  Set "elder" so that the caller may
**	unlink it from the sibling chain should there be a redeclaration
**	error.  Caller doesn't need to know about the parent since there
**	can't be such an error (and besides, there is a parent back-link).
**
** Imports modified:
**	The parent, child, and sibling fields of the entry, its parent and
**	siblings.
*/

bool
sym_install( s, sym_start, rec_level, elder )
SYM	*s,
	*sym_start,
	**elder;
i4	rec_level;
{
    register SYM
		*p;

    *elder = NULL;

    /*
     * link the entry into the correct place in its ancestry tree
     */
    p = sym_start;
    for (;;)
    {
      /* if the new level is below us */
	if (rec_level >= p->st_reclev)
	{
	  /* find the last at this level */
	    while (p->st_sibling)
		p = p->st_sibling;
	  /* if at this level then just link it in here */
	    if (rec_level == p->st_reclev)
	    {
		p->st_sibling = s;
		s->st_parent = p->st_parent;
		*elder = p;
		return TRUE;
	    } else		/* rec_level > p->st_reclev */
	    {
	      /* it's below us */
		if (p->st_child)	/* children below us */
		{
		  /* there are children, so step down and try again */
		    p = p->st_child;
		    continue;
		} else
		{
		  /* no children, so link us in as first child */
		    p->st_child = s;
		    s->st_parent = p;
		    return TRUE;
		}
	    }
	} else				/* rec_level < p->st_reclev */
	{
	  /*
	   * huh? correct level is above us, so the user skipped a level
	   * going down and has now popped up to a missing level.
	   * we act like he gave us the current level number.
	   * Only possible for COBOL and PL/1.
	   */
	    while (p->st_sibling)
		p = p->st_sibling;
	    p->st_sibling = s;
	    s->st_parent = p->st_parent;
	    *elder = p;
	    return TRUE;
	}
    }
}

/*
+* Procedure:	sym_is_redec
** Purpose:	Test an entry to see if it is a redeclaration.
** Parameters:
**	s	- SYM *	- The entry to test.
**	closure	- i4	- Its closure.
**	which	- SYM **- Will receive conflicting entry.
** Return Values:
-*	bool	- Return TRUE iff the given entry is a redeclaration.
** Notes:
**	- it is a redeclaration iff:
**	  1.  the entry has the same parent as one of the previous uses
**	      of this name (including no parent), and
**	  2.  they are defined in the same block, and
**	  3.  they are in the same name space, and
**	  4.  neither is a forward declaration.
**	- Fill in "which" with the clashing entry so ESQL/PL1 can decide
**	  what to do about it.
**
** Imports modified:
**	None.
*/

bool
sym_is_redec( s, closure, which )
register SYM	*s;
i4		closure;
SYM		**which;
{
    register SYM	*p;
#ifdef	xdbDEBUG
  {
    register char	*ss;

    if (s->st_parent)
	ss = sym_str_name( s->st_parent );
    else
	ss = ERx("<none>");

    dbDEBUG( xdbLEV10, (ERx("sym_is_redec: s='%s' (%p), "), sym_str_name(s), s) );
    dbDEBUG( xdbLEV10, (ERx("s->st_parent='%s' (%p), "), ss, s->st_parent) );
    dbDEBUG( xdbLEV10, (ERx("syFuseOF(s)=%x\n"),syFuseOF(s)) );
  }
#endif	/* xdbDEBUG */

    *which = NULL;		/* no conflict so far */
  /* if s is external then no conflict */
    if (!syBitAnd(s->st_flags,syFisFORWARD))
    {
	for (p=s->st_prev; p && syIS_VIS(p,closure); p=p->st_prev)
	{
	    if (s->st_parent==p->st_parent && s->st_block==p->st_block)
	    {
		dbDEBUG( xdbLEV10, (ERx("p='%s' (%p), "), sym_str_name(p), p) );
		dbDEBUG( xdbLEV10, (ERx("syFuseOF(p)=%x\n"), syFuseOF(p)) );

		if ((sym_g_space(s) == sym_g_space(p))/* name space conflict? */
		  && !syBitAnd(p->st_flags,syFisFORWARD))/* neither external? */
		{
		    dbDEBUG( xdbLEV10, (ERx("return TRUE\n")) );
		    *which = p;		/* give back which one it was */
		    return TRUE;
		}
	    }
	}
    }
    dbDEBUG( xdbLEV10, (ERx("return FALSE\n")) );
    return FALSE;
}

/*
+* Procedure:	sym_rparentage
** Purpose:	Recursive routine to build up the full ancestral name
**		for an entry that wants it to be printed forwards (ie,
**		all except COBOL) -- internal.
** Parameters:
**	s	- SYM *		- Entry whose name is to be built.
**	p	- char *	- Buffer to fill with resulting name.
** Return Values:
-*	char *	- Address of next free byte in buffer.
** Notes:
**	- Recursively stuff my parent's name, then my own.
**	- Buffer is assumed to be big enough.
**
** Imports modified:
**	None.
*/

char *
sym_rparentage( p, s )
register char	*p;
register SYM	*s;
{
    GLOBALREF char	*sym_separator;
    GLOBALREF bool	sym_forward;

    if (s->st_parent)
    {
	p = sym_rparentage( p, s->st_parent );
	STprintf( p, ERx("%s"), sym_separator );
	p += STlength( p );
    }
    STprintf( p, ERx("%s"), sym_str_name(s) );
    p += STlength( p );
    return p;
}

/*
+* Procedure:	sym_parentage
** Purpose:	Stuff a buffer with the fully-qualified name of an entry
**	  	(in a syntax appropriate to the language at hand)
**	  	by walking the parent chain.
** Parameters:
**	s	- SYM *	- The entry whose name is to be generated.
** Return Values:
-*	char *	- The address of a static buffer containing the full name.
** Notes:
**	- Uses "sym_separator" and "sym_forward" to determine
**	  the appropriate syntax.
**	- Doesn't work so well for non-PL/1 entries with anonymous types
**	  in the parent chain (the types don't have parents).
**	- Used only in debugging because of the above reason, although since
**	  it is preferable for the grammar to use the same variable reference
**	  string as the user we wouldn't use this routine in translating
**	  variable references anyway.
**
** Imports modified:
**	None.
*/

char *
sym_parentage( s )
SYM	*s;
{
    GLOBALREF char	*sym_separator;
    GLOBALREF bool	sym_forward;
    static char	buf[256];
    register char
		*p = buf;

    *p = '\0';
    if (s == NULL)
	return buf;
    if (sym_forward)
    {
	p = sym_rparentage( p, s );
    } else
    {
	STprintf( p, ERx("%s"), sym_str_name(s) );
	p += STlength( p );
	while (s = s->st_parent)
	{
	    STprintf( p, ERx("%s%s"), sym_separator, sym_str_name(s) );
	    p += STlength( p );
	}
    }
    return buf;
}

/*
+* Procedure:	sym_name_print
** Purpose:	Turn a "variable reference" into a printable name string.
** Parameters:
**	p1	- type	- use
**	p2	- type	- use
** Return Values:
**	char *	- Pointer to a static buffer containing the reference
-*		  in an appropriate syntax.
** Notes:
**	Uses the "normal" name stack for input.
**
** Imports modified:
**	None.
*/

char *
sym_name_print()
{
    int		i;
    int		count;
    static char	buf[256];
    register char
		*p = buf;
    GLOBALREF char	*sym_separator;

    *p = '\0';
    count = sym_f_count();
    if (count == 0)
	return buf;
    STprintf( p, ERx("%s"), sym_f_name(0) );
    p += STlength(p);
    for (i=1; i<count; i++)
    {
	STprintf( p, ERx("%s%s"), sym_separator, sym_f_name(i) );
	p += STlength(p);
    }
    return buf;
}

/*
+* Procedure:	sym_hint_type
** Purpose:	Hint about the next declaration (for ESQL redeclaration errors).
** Parameters:
**	type	- SYM *	- What the next type will be.
**	btype	- i4	- What the next base type will be.
**	indir	- i4	- What the next indirection will be.
** Return Values:
-*	None.
** Notes:
**	Just save this stuff in a static place; symEucUpdate and symPL1Update
**	check it on redeclaration to see if they should complain.  (After which
**	they clear it).
**
** Imports modified:
**	symHint.
*/

void
sym_hint_type( type, btype, indir )
SYM	*type;
i4	btype;
i4	indir;
{
    symHint.sh_type = type;
    symHint.sh_btype = btype;
    symHint.sh_indir = indir;
}

/*
+* Procedure:	sym_eqname
** Purpose:	A fast name compare (internal).
** Parameters:
**	name	- char *	- Input name string.
**	stname	- char *	- Symbol table entry name (with length byte).
**	len	- i4		- The length of NAME.
** Return Values:
-*	bool	- TRUE iff NAME and STNAME are equal (possibly modulo case).
** Notes:
**	Uses STbcompare to do the work; first checks the length.
**
** Imports modified:
**	None.
*/

bool
sym_eqname( name, len, stname )
char	*name, *stname;
i4	len;
{
    if (len != *stname++)
	return FALSE;
    return (bool)(STbcompare(name,len,stname,len,ign_case)==0);
}

/*
+* Procedure:	sym_store_name
** Purpose:	Stash away a name in the name table (internal).
** Parameters:
**	name	- char *	- The name to stash.
**	len	- i4		- The length of the name.
** Return Values:
-*	syNAME * - Pointer to where the name was stashed.
** Notes:
**	Allocates some space in the name table and copies it there.
**
** Imports modified:
**	Modifies the name table.
*/

syNAME *
sym_store_name( name, len )
char	*name;
i4	len;
{
    register syNAME
		*p;

    p = (syNAME *) str_space( syNamtab, len+2 ); /* +1 for length, +1 for nul */
    *p = len;
    MEcopy( (PTR)name, len+1, (PTR)(p+1) );
    return p;
}

/*
+* Procedure:	sym_hash
** Purpose:	Hash a name.
** Parameters:
**	s	- char *	- The name to hash.
** Return Values:
-*	i4	- The hash value.
** Notes:
**	If we are to ignore case then use only the lower-case versions
**	of the letters.
**
** Imports modified:
**	None.
*/

i4
sym_hash( s )
register char	*s;
{
    register i4	val = 0;
    char    		chbuf[2];

    while (*s)
    {
	chbuf[1] = '\0';
	if (ign_case)
	    CMtolower(s, chbuf);
	else
	    CMcpychar(s, chbuf);

	val += (chbuf[0] & I1MASK) + (chbuf[1] & I1MASK); /* May be 2-byte */
	CMnext(s);
    }
    return val % syHASH_SIZ;
}
