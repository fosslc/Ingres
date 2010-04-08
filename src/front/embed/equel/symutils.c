# include <compat.h>
# include <er.h>
# include <si.h>
# include <equel.h>
# include <eqsym.h>
# include <eqrun.h>
# include <ereq.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	symutils.c
** Purpose:	Symbol table miscellaneous utilities (struct assignment).
**
** Defines:
**	symExtType( sym )	- Resolve a forward ref's type (& dims & indir)
**	syStrInit( sy ) 	- Return the first struct member
**	syStrNext( sy )		- Return the next struct member
**	syStrFixup( sy )	- Remove tag entries from the child list
** Data:
-*	sym_adjval		- Ptr to client's sym_g_vlue() adjust routine
** Locals:
**	syAdjust( sym )		- (Local) recursive symtab resolver
** Notes:
**    symExtType:
**    ----------
**	1. Handling the FORWARD declaration.
**	  type xtype is forward;
**	    symDcEuc( &sy, "xtype", ..., syFisTYPE|syFisFORWARD, ... );
**	    if (sy)
**		sym_s_btype( sy, T_FORWARD );
**	2. Declaring a var of some (possibly FORWARD) type.
**	  var xvar is xtype;
**	    symDcEuc( &sy, "xvar", ..., syFisVAR, ... );
**	    the_type = sym_resolve( "xtype", syFisTYPE, ... );
**	    if (sy) {
**		sym_s_type( sy, the_type );
**		if (the_type)
**		    sym_s_btype( sy, sym_g_btype(the_type) );
**		else
**		    sym_s_btype( sy, T_NONE );
**	    }
**	3. The "real" definition of the previously FORWARD type.
**	  type xtype is "real type definition here";
**	    symDcEuc( &sy, "xtype", ..., syFisTYPE, ... );
**	4. Using a variable of some (possibly FORWARD) type.
**	  use xvar;
**	    sy = sym_resolve( ..., "xvar", ... );
**	    if (sy)
**		symExtType( sy );
**	5. Notes.
**	 a) symExtType generates any appropriate message, fills in the
**	    symtab entry if needed, and returns TRUE if all went ok,
**	    else FALSE; no one really needs the return value.
**	 b) The "value" field is not adjusted by "symExtType".  If you need to
**	    do something here, set the external integer function pointer
**	    "sym_adjval" to the address of a routine which will deal with it.
**
**    syStr*:
**    ______
**	    Currently just fetches the children, but might later
**	    be a generator, returning all descendants in (depth-first?) order.
**
**	    usage:
**		for (sy=syStrInit(sy); sy; sy=syStrNext(sy))
**		{
**			if (db_isBEtype(sy))
**			{
**				... assignment code here
**			} else
**				er_write( EsqNonBaseType, sym_str_name(sy) );
**		}
**
** History:
**	19-jul-85	- Written (mrw)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF i4	(*sym_adjval)();		/* for adjusting sy->st_value */

/*
+* Procedure:	symExtType
** Purpose:	Resolve the base type, etc, of a forward reference.
** Parameters:
**	sy	- SYM 	- Pointer to the symbol to be resolved.
** Return Values:
-*	bool	- TRUE iff the ref was defined, else FALSE (+ error message).
** Notes:
**	1.  Nothing to do unless we're pointing at a forward reference.
**	2.  Else walk the type chain, adding up the indirection and dimensions,
**	    until we run out or reach a defined type.
**	3.  If we got a type, add in the dimensions and indirection to our
**	    original one, and set them.  Also set the base type to the just-
**	    discovered type.
**	4.  If we didn't get a defined type then issue an error message,
**	    and reset the base type to avoid further error messages.
**	5.  Call this after looking up a name to fix up the symtab pointer
**	    that you got back.
*/

bool
symExtType( sy )
register SYM	*sy;
{
    register SYM
		*nsy;
    bool	syAdjust();

  /* no null pointers please */
    if (sy == NULL)
    {
	er_write( E_EQ0003_eqNULL, EQ_ERROR, 1, ERx("symExtType") );
	return FALSE;
    }

    /*
    ** if not pointing to an unresolved forward reference
    ** then there's nothing to be done
    */

    if (sym_g_btype(sy) != T_FORWARD)
	return TRUE;
    
    /*
    ** so we have to adjust the indirection & dimensions count, and the size
    ** of the last dimension.  syAdjust will fix up all the indirection,
    ** dimensions, btypes, and dsizes going down the chain.
    ** If it fails then report the error and set our btype to T_UNDEF,
    ** which does two things:
    **	1. It keeps us from generating this error again, should this same
    **	   variable be referenced again.
    **	2. It keeps higher levels from cascading errors (eg, bad indirection).
    **	   This requires cooperation higher up.
    */
    if (syAdjust(sy) == FALSE)
    {
	for (nsy=sy; nsy->st_type; nsy=nsy->st_type);
	er_write( E_EQ0268_syTYPEUNDEF, EQ_ERROR, 2, sym_str_name(sy), 
		sym_str_name(nsy) );
	sym_s_btype( sy, T_UNDEF );
	return FALSE;
    }
    return TRUE;
}

/*
+* Procedure:	syAdjust
** Purpose:	Fix up an incompletely resolved symtab entry.
** Parameters:
**	sy	- SYM *	- The entry to fix.
** Return Values:
-*	bool - TRUE iff we resolved all of the entries below us.
** Notes:
**	A btype of T_FORWARD flags incomplete resolution.
**	Resolve our type (if any), and then use it to fix our indirection,
**	dimensions, dsize, and btype.  Fixing our btype flags our completeness,
**	so we won't do any work next time through.  This routine is recursive.
*/

bool
syAdjust( sy )
SYM	*sy;
{
    register SYM
		*nsy;

  /* are we completely resolved? then nothing to do */
    if (sym_g_btype(sy) != T_FORWARD)
	return TRUE;

  /* if we point to someone, (recursively) fix them up, then fix us */
    if (nsy=sy->st_type)
    {
	if (syAdjust(nsy) == FALSE)	/* didn't resolve the bottom */
	    return FALSE;
	sym_s_indir( sy, sym_g_indir(sy)+sym_g_indir(nsy) );
	sym_s_dims( sy, sym_g_dims(sy)+sym_g_dims(nsy) );
	sym_s_dsize( sy, sym_g_dsize(nsy) );
	sym_s_btype( sy, sym_g_btype(nsy) );
	if (sym_adjval)
	    (*sym_adjval)( sy, sym_g_vlue(nsy) );
	return TRUE;
    }
    sym_s_btype( sy, T_UNDEF );
    return FALSE;
}

/*
+* Procedure:	syStrInit
** Purpose:	Fetch the first child of a symtab entry (if any).
** Parameters:
**	sy	- SYM *	- A pointer to the node whose children are desired.
** Return Values:
-*	SYM *	- A pointer to the first child, or NULL if none.
** Notes:
**	If the first child is a TAG, then skip it and go it its sibling
**	(if any).
**	See the module header notes.
**
** Imports modified:
**	None.
*/

SYM *
syStrInit( sy )
SYM	*sy;
{
    /*
    ** walk the type chain -- guaranteed no loops.
    ** children and types are mutually exclusive.
    */
    while (sy->st_type)
	sy = sy->st_type;
    sy = sym_g_child(sy);
    if (sy && syBitAnd(sy->st_flags,syFisTAG))
	sy = syStrNext(sy);
    return sy;
}

/*
+* Procedure:	syStrNext
** Purpose:	Return the next member of a structure.
** Parameters:
**	sy	- SYM *	- Ptr to a symtab entry, whose next sibling is wanted.
** Return Values:
-*	SYM *	- A pointer to the next sibling, or NULL if none.
** Notes:
**	Skip any structure tag siblings (we only want the vars).
**	See the module header notes.
**
** Imports modified:
**	None.
*/

SYM *
syStrNext( sy )
SYM	*sy;
{
    register SYM	*sib;

    for (sib=sym_g_sibling(sy); sib && syBitAnd(sib->st_flags,syFisTAG);
	sib=sym_g_sibling(sib));
    return sib;
}

/*
+* Procedure:	syStrFixup
** Purpose:	Unlink anonymous entries from the ancestry chain.
** Parameters:
**	sy	- SYM *	- The entry to detach.
** Return Values:
-*	None.
** Notes:
**	Structure tags are usually linked in as a child variable,
**	which they aren't.  syStrInit and syStrNext skip over them for
**	backward compatibility, but this routine disinherits them.
**	They still acknowledge their parent, though.
**
**	You shouldn't call this routine from ADA after the module
**	scoping rules (syada.c) have been added, as this entry will
**	never get deleted and will still be on it's hash chain.
**	We'll worry about this when we upgrade ADA.
**
**	Note that it is never necessary to call this routine: syStrInit
**	and syStrNext do the right thing anyway.  This is just to clean
**	up the trees.
*/

VOID
syStrFixup( sy )
SYM	*sy;
{
    register SYM	*prev, *next;

  /* make sure it's a tag entry */
    if (!sy || !syBitAnd(sy->st_flags,syFisTAG))
	return;

    if (prev=sym_g_parent(sy))
    {
	next = sym_g_child(prev);
	if (next == sy)
	{
	    prev->st_child = sym_g_sibling(next);
	} else
	{
	    for (prev=next; next=sym_g_sibling(next); prev=next)
	    {
		if (next == sy)
		{
		    prev->st_sibling = sym_g_sibling(next);
		    break;
		}
	    }
	}
    }
}
