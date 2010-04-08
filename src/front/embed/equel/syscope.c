# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<equtils.h>
# include	<equel.h>			/* just for eq_dumpfile */
# include	<eqsym.h>
# include	<ereq.h>

/*
+* Filename:	syscope.c
** Purpose:	Scope routines for the symbol table manager.
**
** Defines:
**	sym_sinit	- Initialize the scope stuff.
**	sym_s_debug	- Print debugging scope info.
**	sym_s_begin	- Begin a scope.
**	sym_s_xport	- Note an exported name.
**	sym_s_end	- End a scope.
**	sym_s_val	- Get the current block number.
-*	sym_s_which	- Get the block number of a node.
** Notes:
**
** History:
**	14-Oct-84	- Initial version. (mrw)
**      12-Jun-86       - Whitesmiths cleanup (saw)
**	12-nov-93 (tad)
**		Bug #56449
**		Replaced %x with %p in sym_s_debug()
**      12-Dec-95 (fanra01)
**              Modfied all externs to GLOBALREFs
**      18-apr-96 (thoda04)
**              Added void return type to sym_sinit().
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct scope {
    SYM		*s_save;	/* saved next_symtab pointer */
    syNAME	*s_name;	/* saved next_syNamtab pointer */
    short	s_closure,	/* saved nearest-enclosing closed scope index */
		s_export;	/* number of exported symbols */
} sySCOPE;

static sySCOPE	syScopStack[sySCOP_SIZ] ZERO_FILL;
static sySCOPE	*syScopPtr = syScopStack;

/*
+* Procedure:	sym_sinit
** Purpose:	Initialize the scope stack.
** Parameters:
**	None.
**	p2	- type	- use
** Return Values:
-*	None.
** Notes:
**	Just reset the scope-stack pointer.
*/

void
sym_sinit()
{
    syScopPtr = syScopStack;
}

/*
+* Procedure:	sym_s_debug
** Purpose:	Print out some info useful for debugging scopes.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Print the name, closure, and depth of all active scopes.
**
** Imports modified:
**	None.
*/

void
sym_s_debug()
{
    register sySCOPE	*s;
    GLOBALREF STR_TABLE
		*syNamtab;
    register FILE
		*dp = eq->eq_dumpfile;

    for (s=syScopStack; s<syScopPtr; s++)
	SIfprintf( dp, ERx("%d: (0x%p, %d)\n"),
	    s-syScopStack, s->s_name, s->s_closure );
    SIfprintf( dp, ERx("\n") );
    SIflush( dp );
    str_dump( syNamtab, ERx("name table") );
}

/*
+* Procedure:	sym_s_begin
** Purpose:	Open a new scope, using the given closure.
** Parameters:
**	closure	- i4	- The scope's closure.
** Return Values:
-*	None.
** Notes:
**	Check for overflow.
**
** Imports modified:
**	None.
*/

void
sym_s_begin( closure )
i4	closure;
{
    GLOBALREF STR_TABLE
		*syNamtab;

  /* 
  ** on overflow report a Fatal error - do not worry about pruning string
  ** table space that is not allocated.
  */
    if (++syScopPtr > &syScopStack[sySCOP_SIZ-1])
	er_write( E_EQ0262_sySSTKOFLO, EQ_FATAL, 1, er_na(sySCOP_MAX) );
    else
    {
	syScopPtr->s_name = (syNAME *) str_mark( syNamtab );
	syScopPtr->s_closure = closure;
    }
}

/*
+* Procedure:	sym_s_xport
** Purpose:	Raise the highwater mark for the name table.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	We just exported a name; remember it as this scopes highwater mark.
**
** Imports modified:
**	None.
*/

void
sym_s_xport()
{
    GLOBALREF STR_TABLE
		*syNamtab;

    syScopPtr->s_name = (syNAME *) str_mark( syNamtab );
}

/*
+* Procedure:	sym_s_end
** Purpose:	End a scope
** Parameters:
**	current_scope	- i4	- The current scope.
** Return Values:
-*	i4	- The new closure.
** Notes:
**	- Check for scope-stack underflow.
**	- Prune the name table back to the current high-water mark.
**	- For every entry in the symbol table that was defined in, imported
**	  into, or exported from, this scope:
**		- If it was imported into or exported from this scope then
**		  decrement its visibility (we're going away).
**		- If it was imported pervasive into this scope then make
**		  it back to plain visible.
**		- Else it was defined here; get rid of it.  Clear our parent
**		  link while where at it (he doesn't need to find us anymore).
**
** Imports modified:
**	None.
*/

i4
sym_s_end( current_scope )
i4	current_scope;
{
    register u_i2
		h;
    register SYM
		*p, *last_p;
    GLOBALREF STR_TABLE
		*syNamtab;
    GLOBALREF SYM	*syHashtab[];

    if (syScopPtr > syScopStack)
    {
      /* prune name table */
	str_free( syNamtab, (char *) syScopPtr->s_name );
	for (h=0; h<syHASH_SIZ; h++)
	{
	    last_p = NULL;
	  /*
	  ** foreach entry on this chain defined in,
	  ** imported into, or exported from, this scope
	  ** (excludes names visible from enclosing open scopes):
	  */
	    for( p=syHashtab[h];
	      p && (syIS_PERV(p) || p->st_vislim>=current_scope); )
	    {
		if (p->st_block < current_scope)    /* exported or imported */
		{
		    if (p->st_vislim > 0)
			p->st_vislim--;
		    else if (syIS_PERV(p))
		    {
			if (-p->st_vislim == current_scope)
			    p->st_vislim = current_scope-1;
		    } else		/* p->st_vislim == 0 */
			er_write( E_EQ0264_sySVISZERO, EQ_ERROR, 1, 
			        sym_str_name(p) );
		    last_p = p;
		    p = p->st_hash;
		} else			/* p->st_block == current_scope */
		{
		    register SYM	*next_p;

		    next_p = p->st_hash;
		  /*
		   * If I get deleted then so will all of my siblings;
		   * they are found not by walking child or sibling
		   * links, but by following the hash links.
		   * Thus we can all make our parent disown us
		   * before we commit suicide.
		   */
		    if (p->st_parent)
			p->st_parent->st_child = NULL;
		    sym_q_delete( p, last_p );
		    p = next_p;
		}
	    }
	}
    } else
	er_write( E_EQ0263_sySSTKUFLO, EQ_ERROR, 0 );
    return (syScopPtr--)->s_closure;
}

/*
+* Procedure:	sym_s_val
** Purpose:	Get the current scope level.
** Parameters:
**	None.
** Return Values:
-*	i4	- The current scope level.
** Notes:
**	None.
**
** Imports modified:
**	None.
*/

i4
sym_s_val()
{
    return syScopPtr - syScopStack;
}

/*
+* Procedure:	sym_s_which
** Purpose:	Return the number of the block to which the entry belongs.
** Parameters:
**	e	- SYM *	- The node for which the block number is desired.
** Return Values:
-*	i4	- The block number.
** Notes:
**	If we need this it should be a macro in <eqsym.h>.
**
** Imports modified:
**	None.
*/

i4
sym_s_which( e )
    SYM		*e;
{
    return (i4)(e->st_block);
}
