# include	<compat.h>
# include	<er.h>
# include	<si.h>			/* for equel.h */
# define	EQ_PL1_LANG
# include	<eqsym.h>
# include	<equel.h>		/* for redeclaration errors */
# include	<ereq.h>

/*
+* Filename:	sypl1sym.c
** Purpose:	The PL/1-specific part of the symbol table manager.
**
** Defines:
**	symRsPL1	- Given a stack of entries determine the desired entry.
**	symDcPL1	- Enter a name into the symbol table.
**	symParLoop	- See if one entry is on another entry's parent chain.
** (Local):
-*	syPL1Update	- Update the info fields of an entry.
** Notes:
**
** History:
**	11-Nov-84	- Initial version. (mrw)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
+* Procedure:	symRsPL1
** Purpose:	Resolve a stack of entries to a symtab entry.
** Parameters:
**	result	- SYM **	- Will be set to resolved entry.
**	closure	- i4		- The closure of this reference.
**	use	- i4		- The sy_flag bits.
** Return Values:
**	i4
**	  syL_OK:	success; result has been set.
**	  syL_NO_NAMES:	no field names given; result is NULL.
**	  syL_BAD_REF:	NULL component pointer; result is set to last resolved
**			component (NULL if none).
**	  syL_NOT_FOUND:bad variable reference; result is set to last resolved
**			component (NULL if none).
**	  syL_AMBIG:	ambiguous reference; result is set to first name found.
-*	  syL_RECURSIVE:recursive data reference; never occurs.
** Notes:
**	- Given a stacked list of sym_tab entry pointers A0 ... An,
**	  return a pointer to an entry for A0 such that
**	  A0 in A1 in ... in Am, m >= n, represents a fully qualified
**	  structure reference.
**	  If more than one such A0 is satisfied report an error and return
**	  the first one found.  If none found report an error and return NULL.
**	- If the syFisREVERSE bit is set in "use" then this is PL/1
**	  and the names are stacked in the order:
**		lowest level number	    (bottom)
**			.			|
**			.			|  -- stack growth direction
**			.			v
**		highest level number	      (top)
**	  and we must walk the stack "backwards" (top to bottom).
**	  Remember that high level numbers indicate the most deeply nested
**	  children.
**	- Actually, this self-same code can be used for COBOL if you
**	  just set the visibility stuff appropriately.
**	- ALGORITHM:
**	  For each entry with the same name as A0:
**	    Foreach Ai:
**		Walk up the parent list Ai looking for Ai+1.
**		If out of parents w/o getting to An then try another A0
**		If got to An then this is a successful parent tree;
**		there are three cases:
**		    1.	The new one was defined deeper than the old;
**		    2.	The new one was defined at the same level;
**		    3.	The new one was defined shallower.
**		Case (1) can't happen.  Case (2) is ambiguous, but if one
**		was a top-level reference then it wins (can't be two top-level
**		in the same block).  Case (3) returns the old one and stops.
**
** Imports modified:
**	None.
*/

i4
symRsPL1( result, closure, use )
SYM	**result;
i4	closure;
i4	use;
{
    register i4
		i,		/* current stack entry index */
		count;		/* how many entries on the stack */
    register SYM
		*tail,		/* most deeply nested entry on stack */
		*last_found,	/* remembers the last-found resolution */
		*next,		/* current stack entry */
		*parent;	/* walks up parent chain */
    SYM		*first_found,	/* remember the first-found resolution */
		*top;		/* highest entry in the list given us */

  /* stack direction stuff */
    i4		rev,	/* forwards (cobol) or backwards (pl/1)? */
		first,	/* front of the stack (cobol) or end (pl/1) */
		last;	/* one past end (cobol) or one before start (pl/1) */

    rev = use & syFisREVERSE;	/* which direction? */
    use &= syFuseMASK;
    count = sym_f_count();
    if (count < 1)
    {
	*result = NULL;
	return syL_NO_NAMES;
    }

  /* walk in what direction? */
    if (rev)		/* pl/1 */
    {
	first = count-1;
	last = 0;
    } else {		/* cobol */
	first = 0;
	last = count-1;
    }

    first_found = last_found = NULL;	/* start off without finding any */
    for (tail = sym_f_entry(first);
	 tail && syIS_VIS(tail,closure);
	 tail = tail->st_prev)
    {
	if (!syBitAnd(syFuseOF(tail),use))
	    continue;

	parent = tail;		/* so far the only guy */

	/*
	 * now we walk from most nested name to topmost name
	 */
	if (rev)
	{
	    for (i=first-1; i>=last && parent; i--)
	    {
		next = sym_f_entry( i );	/* current entry */
		if (next == NULL)	/* bad entry -- return partial */
		{
		    *result = tail;
		    return syL_BAD_REF;
		}

		/*
		 * walk up the parent chain looking for an entry
		 * with the current name
		 */
		for (parent = parent->st_parent;
		     parent && (parent->st_name != next->st_name);
		     parent = parent->st_parent);
	    }
	} else {	/* forward */
	    for (i=first+1; i<=last && parent; i++)
	    {
		next = sym_f_entry( i );	/* current entry */
		if (next == NULL)	/* bad entry -- return partial */
		{
		    *result = tail;
		    return syL_BAD_REF;
		}

		/*
		 * walk up the parent chain looking for an entry
		 * with the current name
		 */
		for (parent = parent->st_parent;
		     parent && (parent->st_name != next->st_name);
		     parent = parent->st_parent);
	    }
	} /* if -- reverse or forward */
	top = parent;			/* may be null */

        /*
         * if parent is NULL then we ran out of parents without
         * finding the current entry; otherwise we found parents
	 * for all the entries, and so "tail" points to the (correct)
	 * most-deeply nested entry.
         */

	if (parent)	/* if we found one */
	{
	  /* if we found one earlier */
	    if (last_found)
	    {
		/*
		 * the new one was defined either:
		 *   1. at a deeper block
		 *   2. at the same block
		 *   3. at a shallower block
		 * than the old one.
		 *
		 * 1) By construction, this can't happen.  We find the deepest
		 *    ones first.
		 * 2) This is ambiguous, but if the top reference is a top-level
		 *    entry (no parent) then we will resolve it in its favor.
		 *    It cannot be that both (all) top references are top-level
		 *    entries AND are in the same block, since this would have
		 *    been a redeclaration error at declare time.  We'll keep
		 *    on searching for a top-level reference.
		 * 3) Not only is this not ambiguous (old one wins) but we
		 *    don't have to look any further since any other matches
		 *    must also be case 3 (by construction).
		 */
		if (tail->st_block < last_found->st_block)	/* case 3 */
		{
		    *result = last_found; 	/* return the old one */
		    return syL_OK;
		}
	    } else /* if -- we didn't find one earlier */
		first_found = tail;	/* remember the first one we found */

	    last_found = tail;		/* remember the last one we found */

	    /*
	     * we get here iff :
	     *	a) We found an entry (tail), and
	     *	   i) There was a previous entry defined in the same block, or
	     *	  ii) There was no previous entry
	     *
	     * if the reference had a top-level qualification then it wins
	     * any existing (or possible future) ties.
	     */

	  /* highest entry is top-level -- it wins the ambiguity (if any) */
	    if (top && (top->st_parent == NULL))
	    {
		*result = tail;
		return syL_OK;
	    }
	} /* if -- we found one */
    } /* for */

  /* no object matches exactly */
    if (first_found == NULL)
    {
	*result = tail; 		/* if partial, then return that */
	return syL_NOT_FOUND;
    }

  /* both first_found and last_found are set */
    if (first_found == last_found)
    {
      /* exactly one object matches */
	*result = last_found;
	return syL_OK;
    }

    /*
     * they're different, and neither is top-level : this is ambiguous.
     */

    *result = first_found;	/* return the first one */
    return syL_AMBIG;
}

/*
+* Procedure:	syPL1Update
** Purpose:	Set the info fields for a symtab entry (internal).
** Parameters:
**	s		- SYM *		- Entry to modify.
**	name		- char *	- Name of entry (for debugging).
**	record_level	- i4		- Field (nesting) level.
**	cur_scope	- i4		- Block in which defined.
**	state		- i4		- Flag bits.
**	closure		- i4		- Nearest enclosing closed scope.
** Return Values:
-*	SYM *	- NULL for error, S on success.
** Notes:
**	- Given a pointer to a symbol table entry, set all of the info fields.
**	- Install it appropriately.
**	- Delete the entry on any error.
**	- Top-level entries can be checked for redeclaration immediately,
**	  but children have to wait until they're installed (because the
**	  check compares parents, and children to have proper parents until
**	  they're installed).
**	- ESQL complains about redeclaration errors only if the types are
**	  incompatible because there's no way to tell us about scopes.
**
** Imports modified:
**	Symbol table entry links are changed.
*/

SYM *
syPL1Update( s, name, record_level, cur_scope, state, closure )
SYM	*s;
char	*name;
i4	record_level, cur_scope, state, closure;
{
    bool	result;
    SYM		*elder;
    SYM		*clash;
    static SYM	*sym_start = NULL;
    bool	sym_install();
#define	SY_NORMAL	0		/* actual value doesn't really matter */

    s->st_vislim = cur_scope;
    s->st_block = cur_scope;
    s->st_reclev = record_level;
    s->st_flags = state;
    s->st_space = SY_NORMAL;

    if (record_level == 0)	/* parents; children get checked lower */
    {
	if (sym_is_redec(s,closure,&clash))
	{
	    if (dml_islang(DML_ESQL))
	    {
		if (symHint.sh_btype != T_NOHINT)
		{
		  /* complain only if they don't match well enough */
		    if (symHint.sh_type != clash->st_type ||
			symHint.sh_btype != clash->st_btype ||
			symHint.sh_indir != clash->st_indir)
		    {
			er_write( E_EQ0267_sySQREDEC, EQ_ERROR, 1, name );
		    }
		} else
		    er_write( E_EQ0267_sySQREDEC, EQ_ERROR, 1, name );
	    } else
	    {
		er_write( E_EQ0260_syREDECLARE, EQ_ERROR, 1, name );
	    }
	    symHint.sh_btype = T_NOHINT;
	    sym_q_delete( s, NULL );
	    return NULL;
	}
	sym_start = s;
	symHint.sh_btype = T_NOHINT;
	return s;
    } else if (sym_start == NULL)	/* "can't happen" */
    {
	er_write( E_EQ0251_syBADLEVEL, EQ_ERROR, 2, er_na(record_level), name );
	symHint.sh_btype = T_NOHINT;
	sym_q_delete( s, NULL );
	return NULL;
    }

    result = sym_install( s, sym_start, record_level, &elder );
  /* must check for redec now that s has proper parent */
    if (result==FALSE || sym_is_redec(s,closure,&clash))
    {
	if (result)
	{
	    if (dml_islang(DML_ESQL))
	    {
		if (symHint.sh_btype != T_NOHINT)
		{
		  /* complain only if they don't match well enough */
		    if (symHint.sh_type != clash->st_type ||
			symHint.sh_btype != clash->st_btype ||
			symHint.sh_indir != clash->st_indir)
		    {
			er_write( E_EQ0267_sySQREDEC, EQ_ERROR, 1, name );
		    }
		} else
		    er_write( E_EQ0267_sySQREDEC, EQ_ERROR, 1, name );
	    } else
	    {
		er_write( E_EQ0260_syREDECLARE, EQ_ERROR, 1, name );
	    }
	}
	/*
	** If "s" has an elder sibling then let it forget about "s".
	** The parent doesn't have to forget since the first child
	** can't cause a redeclaration error.
	*/
	if (elder)
	    sym_s_sibling( elder, NULL );
	symHint.sh_btype = T_NOHINT;
	sym_q_delete( s, NULL );
	return NULL;
    }

    symHint.sh_btype = T_NOHINT;
    return s;
}

/*
+* Procedure:	symDcPL1
** Purpose:	Enter a name into the symbol table at the correct level.
** Parameters:
**	s		- SYM *		- Entry to modify.
**	name		- char *	- Name of entry (for debugging).
**	record_level	- i4		- Field (nesting) level.
**	cur_scope	- i4		- Block in which defined.
**	state		- i4		- Flag bits.
**	closure		- i4		- Nearest enclosing closed scope (always
**					  0 for PL/1: all scopes are open).
** Return Values:
-*	SYM *	- NULL for error, S on success.
** Notes:
**	- If EXTERNs were involved, so that no new entry was made, just
**	  return (sym_lookup manages the syFisFORWARD bit).
**	- syPL1Update does all of the work.
**
** Imports modified:
**	Symbol table entry links are changed.
*/

SYM *
symDcPL1( name, record_level, cur_scope, state, closure )
char	*name;
i4	record_level,
	cur_scope,
	state,
	closure;		/* always 0 for PL/1; all scopes are open */
{
    register SYM
		*s;

    s = sym_lookup( name, TRUE, state, cur_scope );
    return syPL1Update( s, name, record_level, cur_scope, state, closure );
}

/*
+* Procedure:	symParLoop
** Purpose:	Check for an ancestral loop. Used in "me LIKE likeme" to ensure
**		that "likeme" is not a parent of "me".
** Parameters:
**	me	- SYM *	- The child to find on its parent chain.
**	likeme	- SYM * - The LIKE entry to compare to.
** Return Values:
-*	bool	- TRUE if the 'likeme' entry is on 'me's parent chain.
** Notes:
**	Just walk up the parent chain looking for the given entry.
**	"Me" must already be installed (so that it has the right parent).
**
** Imports modified:
**	None.
*/

bool
symParLoop( me, likeme )
    register SYM	*me,			/* bottom of ancestral chain */
			*likeme;		/* node to look for */
{
    register SYM	*sy;

    for (sy=me->st_parent; sy; sy=sy->st_parent)
	if (sy == likeme)
	    return TRUE;
    return FALSE;
}
