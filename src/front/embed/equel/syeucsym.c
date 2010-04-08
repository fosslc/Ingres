# include	<compat.h>
# include	<st.h>			/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>			/* for equel.h */
# include	<equtils.h>
# define	EQ_EUC_LANG
# include	<eqsym.h>
# include	<equel.h>		/* for redeclaration errors */
# include	<ereq.h>

/*
+* Filename:	syeucsym.c
** Purpose:	The Euclid-specific symbol table routines.
**
** Defines:
**	symRsEuc	- Find the symtab entry referenced by a stack of names.
**	symDcEuc	- Enter a name into the symbol table.
**	sym_list_print	- Print a list of names, linked through st_type field.
**	sym_type_euc	- Set the type of all list entries to the passed type.
** (Local):
**	syEucUpdate	- Sets all of the info fields of a symtab entry.
**	sym_modify_euc	- Import, export, or make pervasive a declared name.
-*	sym_check_type_loop - Check for a type loop.
** Notes:
**
** History:
**	02-Oct-84	- Initial version. (mrw)
**	29-jun-87 (bab)	Code cleanup.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* The head of the family tree */
static SYM	*sym_start = NULL;

/*
+* Procedure:	symRsEuc
** Purpose:	Find the symtab entry referenced by a stack of names.
** Parameters:
**	result	- SYM **	- Where to stuff the found entry pointer.
**	closure	- i4		- Closure level (for visibility)
**	use	- i4		- syFisTYPE or syFisVAR
** Return Values:
**	i4
**	syL_OK:		success; result has been set.
**	syL_NO_NAMES:	no field names given; result is NULL.
**	syL_BAD_REF:	NULL component pointer; result is set to last resolved
**			component (NULL if none).
**	syL_NOT_FOUND:	bad variable reference; result is set to last resolved
**			component (NULL if none).
-*	syL_RECURSIVE:	recursive data reference; never occurs.
** Notes:
**	Find a visible top-level (or tag) entry; then match children.
**	NULL children are considered syL_BAD_REF.
**
** Imports modified:
**	None.
*/

i4
symRsEuc( result, closure, use )
SYM	**result;		/* where to stuff the found entry pointer */
i4	closure;		/* closure level (for visibility) */
i4	use;			/* syFisTYPE or syFisVAR */
{
    register SYM
		*s, *p, *q;
    register i4
		i, count;

    use &= syFuseMASK;		/* ignore base & const etc, bits */
    count = sym_f_count();
    if (count < 1)
    {
	*result = NULL;
	return syL_NO_NAMES;
    }

    /*
     * find a visible entry of the correct type and either:
     *	top-level (no parent)
     * or
     *	this is a tag lookup
     * this is so that "struct tag var" works even if "struct tag"
     * was defined nested in some other structure.
     * This (as well as syFisTAG in general) is really here for C.
     */

    for (p=sym_f_entry(0); p && syIS_VIS(p,closure); p=p->st_prev)
    {
	if (((p->st_parent==NULL) || syBitAnd(use,syFisTAG))
	  && syBitAnd(syFuseOF(p),use))
	    break;
    }
  /* did we find one? */
    if (p == NULL || (p->st_parent && !syBitAnd(use,syFisTAG))
      || !syIS_VIS(p,closure) || !syBitAnd(syFuseOF(p),use))
    {
#ifdef	xdbDEBUG
	dbDEBUG( xdbLEV6, (ERx("use=%x, closure=%d, "), use, closure) );
	if (p)
	{
	    dbDEBUG( xdbLEV6, (ERx("p=0x%p, parent=0x%p, "), p, p->st_parent) );
	    dbDEBUG( xdbLEV6, (ERx("vislim=%d, "), p->st_vislim) );
	    dbDEBUG( xdbLEV6, (ERx("IsVis=%d, "), syIS_VIS(p,closure)) );
	    dbDEBUG( xdbLEV6, (ERx("UseOf(p)=%x\n"), syFuseOF(p)) );
	} else
	    dbDEBUG( xdbLEV6, (ERx("p==NULL\n")) );
#endif	/* xdbDEBUG */
	*result = NULL;
	return syL_NOT_FOUND;
    }

  /* now check the children; if no child then slide on to the type */
    q = p;
    for (i=1; i<count; i++)
    {
	s = sym_f_entry( i );
	dbDEBUG( xdbLEV6, (ERx("s='%s', "),sym_str_name(s)) );
	dbDEBUG( xdbLEV6, (ERx("before walking types: p=%s, "),sym_str_name(p)) );

      /* at construction time we'll guarantee no type loops */
	while (p->st_type)
	    p = p->st_type;
	dbDEBUG( xdbLEV6, (ERx("after walking types: p=%s, "),sym_str_name(p)) );

      /* allow null component pointers; treat them as a bad reference */
	if (s == NULL)
	{
	    *result = p;
	    return syL_BAD_REF;
	}

	for (q=p->st_child; q && syIS_VIS(q,closure); q=q->st_sibling)
	{
	    if (s->st_name == q->st_name)
	    {
		p = q;
		break;
	    }
	}
	if ((q==NULL) || !syIS_VIS(q,closure))
	    break;
    }
    *result = p;	/* return as much as we've found */
    if (q && syIS_VIS(q,closure))
	return syL_OK;
    dbDEBUG(xdbLEV6, (ERx("q=0x%p, q %svisible\n"), q, 
	    syIS_VIS(q,closure)?ERx("in"):ERx("")));
    return syL_NOT_FOUND;
}

/*
+* Procedure:	syEucUpdate
** Purpose:	Set the info fields for a symtab entry (internal).
** Parameters:
**	sy		- SYM *		- Entry to modify.
**	name		- char *	- Name of entry (for debugging).
**	record_level	- i4		- Field (nesting) level.
**	cur_scope	- i4		- Block in which defined.
**	state		- i4		- Flag bits.
**	closure		- i4		- Nearest enclosing closed scope.
**	space		- i4		- Address space.
** Return Values:
-*	SYM *	- NULL for error, S on success.
** Notes:
**	- Given a pointer to a symbol table entry, set all of the info fields.
**	- Install it appropriately.
**	- Delete the entry on any error.
**	- See also sym_type_euc for setting the type field.
**	- Sym_type_euc must check for type cycles.
**	- Negative record_level is special-case: sym_modify_euc is calling us
**	  (through symDcEuc) to IMPORT OPAQUE an already pervasive name so
**	  don't worry about redeclaration.
**	- Top-level entries can be checked for redeclaration immediately,
**	  but children have to wait until they're installed (because the
**	  check compares parents, and children to have proper parents until
**	  they're installed).
**	- ESQL complains about redeclaration errors only if the types are
**	  incompatible because there's no way to tell us about scopes.
**
** Imports modified:
**	Symbol table entry links are changed.
**	sym_start is set.
*/

SYM *
syEucUpdate( sy, name, record_level, cur_scope, state, closure, space )
SYM	*sy;
char	*name;
i4	record_level, cur_scope, state, closure, space;
{
    bool	result;
    SYM		*elder;
    SYM		*clash;
    bool	sym_install();

    sy->st_vislim = cur_scope;
    sy->st_block = cur_scope;
    sy->st_reclev = record_level;
    sy->st_flags = state & ~syFisORPHAN;
    sy->st_space = space;

  /* this makes it pervasive */
    if (state & syFisPERVAD)
	sy->st_vislim = -sy->st_vislim;

    if (record_level <= 0)
    {
      /* negative means this is IMPORT OPAQUE; don't check for redeclaration */
	if (record_level == 0)	/* parents; children get checked lower */
	{
	    if (sym_is_redec(sy,closure,&clash))
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
		sym_q_delete( sy, NULL );
		symHint.sh_btype = T_NOHINT;
		return NULL;
	    }
	}

	symHint.sh_btype = T_NOHINT;

	/*
	** Careful! Consider
	**	TYPE
	**	    r = record
	**		xp : ^p;
	**	    end;
	**	    p = integer;
	** "p" must be entered as reclevel 0 in order to be found
	** when the real "p" is seen, but if we do that then "p"
	** will become the parent, which is wrong.
	** Forward type entries should be slipped in sideways, without
	** any side effects.
	**
	** Also consider
	** type
	**	rec = record
	** 		xfield: (red, blue, green)
	**	end;
	** "red", "blue", and "green" need to be seen as top-level entries
	** too, but also without disturbing the current parent.
	** Orphan constant entries should also be slipped in sideways.
	*/

	if ((state & (syFisFORWARD|syFisTYPE))==(syFisFORWARD|syFisTYPE))
	    return sy;
	if ((state & (syFisORPHAN|syFisCONST))==(syFisORPHAN|syFisCONST))
	    return sy;

	if (record_level < 0)
	    sym_start = NULL;
	else
	    sym_start = sy;
	return sy;
    } else if (sym_start == NULL)	/* "can't happen" */
    {
	er_write( E_EQ0251_syBADLEVEL, EQ_ERROR, 2, er_na(record_level), name );
	sym_q_delete( sy, NULL );
	return NULL;
    }

    result = sym_install( sy, sym_start, record_level, &elder );
  /* must check for redec now that sy has proper parent */
    if (result==FALSE || sym_is_redec(sy,closure,&clash))
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
	** If "sy" has an elder sibling then let it forget about "sy".
	** The parent doesn't have to forget since the first child
	** can't cause a redeclaration error.
	*/
	if (elder)
	    sym_s_sibling( elder, NULL );
	symHint.sh_btype = T_NOHINT;
	sym_q_delete( sy, NULL );
	return NULL;
    }

    symHint.sh_btype = T_NOHINT;
    return sy;
}

/*
+* Procedure:	symDcEuc
** Purpose:	Enter a name into the symbol table at the correct level.
** Parameters:
**	sy		- SYM *		- Entry to modify.
**	name		- char *	- Name of entry (for debugging).
**	rec_level	- i4		- Field (nesting) level.
**	cur_scope	- i4		- Block in which defined.
**	state		- i4		- Flag bits.
**	closure		- i4		- Nearest enclosing closed scope.
**	space		- i4		- Address space.
** Return Values:
-*	SYM *	- NULL for error, S on success.
** Notes:
**	- If this is a non-anonymous entry then try to find it first.
**	  If you find it, and it's in the same block, and either it's
**	  EXTERN or the new one is, then just return it (but turn off
**	  the syFisFORWARD bit if it was on and this is a real definition).
**	  Otherwise make a new entry.
**	  Note that if this is an EXTERN declaration then we don't care
**	  if the old entry was in our block.  These are C semantics; they
**	  might be different for other languages.
**	- syEucUpdate does most of the work.
**
** Imports modified:
**	Symbol table entry links are changed.
*/

SYM *
symDcEuc( name, rec_level, cur_scope, state, closure, space )
char	*name;
i4	rec_level;
i4	cur_scope;
i4	state;
i4	closure;
i4	space;
{
    register SYM
		*sy = NULL;
    SYM		*sy_my_parent;
    SYM		*sym_which_parent();

  /* anonymous entries always get declared */
    if (*name)
    {
	sy_my_parent = sym_which_parent( sym_start, rec_level );
      /* try to find our entry */
	sy = sym_resolve( sy_my_parent, name, closure, state );
    }

  /* did we get one? */
    if (sy)
    {
      /* if we're declaring it external then it needn't be in our block */
	if (state & syFisFORWARD)
	    return sy;
      /* else if it's in our block then we want it (or it's a conflict) */
	if (sy->st_block == cur_scope)
	{
	  /* if it's EXTERN then we want it */
	    if (sy->st_flags & syFisFORWARD)
	    {
	      /* so turn off the extern bit if this is a definition */
		if (!(state & syFisFORWARD))
		    sy->st_flags &= ~syFisFORWARD;
		/*
		** defs at level 0 become the ancestral head
		** (normally done in syEucUpdate)
		*/
		if (rec_level == 0)
		    sym_start = sy;
	      /* and return it as the "declaration" */
		return sy;
	    }
	}
    }

    /*
    ** Either didn't find one, or it wasn't the one we wanted,
    ** or neither this declaration nor the one we found is external
    ** (thus a new entry is required).
    ** This might be a redeclaration, in which case syEucUpdate will
    ** notice, complain, and fixup the symbol table.
    */
    sy = sym_lookup( name, TRUE, state, cur_scope );
    return syEucUpdate( sy, name, rec_level, cur_scope, state, closure, space );
}

/*
+* Procedure:	sym_modify_euc
** Purpose:	Import, export or pervade a previously declared name (internal).
** Parameters:
**	name		- char *	- Name of entry (for debugging).
**	record_level	- i4		- Field (nesting) level.
**	cur_scope	- i4		- Block in which defined.
**	state		- i4		- Flag bits.
**	closure		- i4		- Nearest enclosing closed scope.
** Return Values:
-*	SYM *	- NULL for error, S on success.
** Notes:
**	IMPORT OPAQUE of an already PERVASIVE name is funny; we've decided
**	to allow it by making a copy of the name (no children since its OPAQUE).
**
** Imports modified:
**	Symbol table entry links are changed.
*/

bool
sym_modify_euc( name, record_level, cur_scope, state, closure )
char	*name;
i4	record_level, cur_scope, state, closure;
{
    register SYM
		*s;
    SYM		*bogus;	/*
			** bogus gets changed to real value if this
			** code is ever used (bab for ncg)
			*/

    s = sym_find( name );
    if (s == NULL)
    {
	er_write( E_EQ0266_syUNDECSTAT, EQ_ERROR, 1, name );
	return FALSE;
    }

    if (state & syFisIMPORT)
    {
      /* can previous scope see this name? */
	if (!syIS_VIS(s,syMIN(closure,cur_scope-1)))
	{
	    er_write( E_EQ0258_syNOTVISIBL, EQ_ERROR,1, name );
	    return FALSE;
	}

      /* already made visible in this scope? */
	if (s->st_vislim==cur_scope || -s->st_vislim==cur_scope)
	{
	    er_write( E_EQ0261_syREIMPORT, EQ_ERROR, 1, name );
	    return FALSE;
	}

      /*
       * already pervasive? this is a funny case.
       * if it's IMPORT OPAQUE then we'll just make a copy;
       * otherwise we'll ignore it.
       */
	if (syIS_PERV(s) )
	{
	    if (state & syFisOPAQUE)
		return (bool)
		   (symDcEuc(name,-1,cur_scope,state,closure,0) ? TRUE:FALSE);
	    return TRUE;
	}

      /* this actually does the importing */
	if (s->st_vislim < syVIS_MAX )
	    s->st_vislim++;
	else
	    er_write( E_EQ0253_syDEEPIMPORT, EQ_ERROR, 1, name );

      /* flag pervasive */
	if (state & syFisPERVAD)
	    s->st_vislim = -s->st_vislim;

	sym_q_head( s );
    } else if (state & syFisEXPORT)
    {
      /* is this name already visible to the next outer scope? */
	if (sym_is_redec(s,syMIN(closure,cur_scope-1), &bogus))
	{
	    er_write( E_EQ0254_syEXPORTVIS, EQ_ERROR, 1, name );
	    return FALSE;
	}

      /* this exports it */
	s->st_block--;
	sym_s_xport();		/* make the name stick in the name table */
	/* sym_q_head( s ); */	/* I don't think we need to do this */
    } else if (state & syFisPERVAD)
    {
	s->st_vislim = -s->st_vislim;		/* flag pervasive */
    } else		/* just defined here */
    {
	er_write( E_EQ0257_syMODIFY, EQ_ERROR, 1, name );
    }

    return TRUE;
}

/*
+* Procedure:	sym_list_print
** Purpose:	Print a list of names, linked through the st_type field.
** Parameters:
**	list	- SYM *	- Pointer to the head of the list.
** Return Values:
-*	char *	- Pointer to the static buffer containing the print string.
** Notes:
**	Uses a static internal buffer.
**
** Imports modified:
**	None.
*/

char *
sym_list_print( list )
register SYM	*list;
{
    static char	buf[256];
    register char
		*p = buf;

    STprintf( p, sym_str_name(list) );
    p += STlength( p );

    while (list = list->st_type)
    {
	STprintf( p, ERx(", %s"), sym_str_name(list) );
	p += STlength( p );
    }
    return buf;
}

/*
+* Procedure:	sym_type_euc
** Purpose:	Set the type of all entries in a list to a type.
** Parameters:
**	list	- SYM *	- The head of the list.
**	type	- SYM *	- The type to which to set the entries.
** Return Values:
-*	None.
** Notes:
**	Check each entry to see if it would cause a type-loop, and
**	throw away its type if so.
** NOTE:
**	We have dropped the "list" capability; it doesn't seem necessary,
**	and it causes problems (but I've left the code in for a while
**	anyway)..
**
**	Scenario:
**		extern my_type	*my_ptr;
**
**	Grammars would like to ignore the "extern" except for setting the
**	syFisFORWARD bit, but if we find an entry for "my_ptr" rather than
**	making a new one, the "type" field might already be set, and it's
**	very difficult for the grammar to figure out.  Therefore we'll
**	make the grammar set the types one-at-a-time.
**
** Imports modified:
**	None.
*/

void
sym_type_euc( list, type )
register SYM	*list, *type;
{
    register SYM
		*s;
    bool	sym_check_type_loop();

    if (list==NULL || type==NULL)
	return;

    dbDEBUG( xdbLEV1, (ERx("sym_type_euc(%s --> "), sym_list_print(list)) );
    dbDEBUG( xdbLEV1, (ERx("%s)\n"), sym_str_name(type)) );

  /* This entry already has a type */
    if (list->st_type)
	return;

  /* Gets executed exactly once */
    while (list)
    {
	s = list->st_type;		/* next entry in list */
	if (sym_check_type_loop(list,type) == TRUE)
	    list->st_type = type;	/* set this entry's type */
	else
	    list->st_type = NULL;	/* stop it right here */
	list = s;			/* walk the list */
    }
}

/*
+* Procedure:	sym_check_type_loop
** Purpose:	Check if making OLD point to NEW would cause a loop (internal).
** Parameters:
**	old	- SYM *	- The entry who's "type" pointer will be set.
**	new	- SYM *	- The entry to which "old"'s "type" pointer will point.
** Return Values:
-*	bool	- FALSE if a pointer is NULL or would cause a loop, else TRUE.
** Notes:
**	See if OLD is already on NEW's type chain.
**
** Imports modified:
**	None.
*/

bool
sym_check_type_loop( old, new )
SYM	*old, *new;
{
    register SYM
	*p;

    if (old==NULL || new==NULL)
    {
	er_write( E_EQ0252_syCHEKNULL, EQ_ERROR, 0 );
	return FALSE;
    }

    for (p=new->st_type; p; p=p->st_type)
    {
      /* don't need to check "new", since here's where we build them! */
	if (p == old)
	{
	    er_write( E_EQ0265_syTYPLOOP, EQ_ERROR, 2, sym_str_name(old), 
		    sym_str_name(new) );
	    return FALSE;
	}

      /* but we'll check it anyway -- infinite loops aren't nice */
	if (p == new)
	{
	    er_write( E_EQ0265_syTYPLOOP, EQ_ERROR, 2, sym_str_name(p), 
		    sym_str_name(new) );
	    return FALSE;
	}
    }
    return TRUE;
}
