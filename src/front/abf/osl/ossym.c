/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<si.h>		/* 6-x_PC_80x86 */
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<cm.h>
#include	<st.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<ossym.h>

#include	<ilops.h>
#include	<iltypes.h>

/**
** Name:    ossym.c -	OSL Parser Symbol Table Module.
**
** Description:
**	Contains routines used to access and manipulate the symbol table
**	for the OSL parsers.  Defines:
**
**	osfld()		return symbol node for field.
**	ostab()		return symbol node for column of table field.
**	ossymput()	place new symbol into symbol table.
**	ossymfll()	allocate a symbol node and fill in the info. in it.
**	ossymlook()	find symbol in the symbol table or as a global.
**	ossympeek()	look in symtable for a symbol with specified parent.
**	ossymsearch()	look in symtable for a symbol with specified visibility.
**	ossymalloc()	allocate a symbol node.
**	ossymundef()	get the 'undefined' symbol.
**	ossymflush()	send a symbol to ILG.
**
**	Locals:
**	symhash()
**	normalize_name()
**
** History:
**	Revision 5.1  86/11/04  15:23:28  wong
**	Modified for OSL translator.
**
**	Revision 6.0  87/06  wong
**	Support case-insensitivity for symbols.  (87/06  jhw)
**	Support for ADTs.  (87/04  jhw)
**	Removed `dot' symbol entry support.  (87/01  jhw)
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures.
**		Added procedure ossymsearch.
**		Modified procedures ossymfll, ossymlook, ossympeek.
**
**	Revision 6.4/02
**	04/27/92 (emerson)
**		Fix for bug 43771 in ossymlook.
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Small fix in ossymlook (for bug 44056).
**
**	Revision 7.0
**	15-jun-92 (davel)
**		Initialized s_scale and s_length in ossymfll().
**	21-aug-92 (davel)
**		Add debug file pointer to call to ossymdump (which is now 
**		called from within igoutput.c).
**	14-may-97 (mcgem01)
**              A typo in the change to the search algorithm, resulted
**              in the use of a non-normailzed symboln in ossymsearch.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

READONLY DB_DATA_VALUE   osNodt = {NULL, 0, DB_NODT, 0};

FUNC_EXTERN	ILALLOC	IGallocRef();
FUNC_EXTERN	OSSYM	*osgetcurrentproc();

static i4	symhash();
static char *normalize_name();

/*{
** Name:    osfld() -	Return Symbol Node for Field.
**
** Description:
**	Returns a symbol node for a field, including hidden fields and
**	table fields.  If the field is not defined, then this routine
**	enters the field in the symbol table as undefined so that future
**	references will not generate extra error messages.
**
** Input:
**	parent	{OSSYM *}  Symbol table entry for the form.
**	name	{char *}  The name of the field.
**	context	{nat}  Context error message.
**
** Returns:
**	{OSSYM *}	The symbol table entry for the field.
*/
OSSYM *
osfld (parent, name, context)
OSSYM	*parent;
char	*name;
i4	context;
{
    register OSSYM	*s;

    if ((s = ossymlook(name, parent)) != NULL)
	return s;

    if (parent->s_kind == OSFORM)
	oscerr(context, 1, name);
    else
    {
	char	buf[OSBUFSIZE];

	_VOID_ STprintf(buf, ERx("%s.%s"), parent->s_name, name);
	oscerr(context, 1, buf);
    }
    return ossymundef(name, parent);
}

/*{
** Name:    ostab() -	Return Symbol Node for Column of Table Field.
**
** Description:
**	A reference to a table field has been made.
**	This builds a special symbol that contains all the information
**	needed for the table field reference.
**
** PARAMETER
**	field		{OSSYM *}
**		The pointer to the table field.
**
**	expr		{OSNODE *}
**		The pointer to the expression for the table field.
**
**	name		{char *}
**		The name of the column.
*/
OSSYM *
ostab (parent, table, column)
OSSYM	*parent;
char	*table;
char	*column;
{
    register OSSYM	*fld;
    register OSSYM	*col;

    if (parent->s_kind == OSFORM)
	fld = ossympeek(table, parent);
    else
	fld = parent;

    if (fld == NULL || (fld->s_kind != OSTABLE && fld->s_kind != OSUNDEF))
    {
	oscerr(OSNOTBLFLD, 2, table, parent->s_name);
	/*
	** If not a known table field, and not an undefined symbol,
	** don't attempt to reference a non-existent column.
	*/
	if (fld == NULL)
    	    fld = ossymundef(table, parent);
	else if (fld->s_kind != OSUNDEF)
	    return fld;
    }

    if ((col = ossympeek(column, fld)) == NULL)
    { /* no such column in table field */
	oscerr(OSNOCOL, 3, fld->s_parent->s_name, fld->s_name, column);
	col = ossymundef(column, fld);
    }
    return col;
}

# define	HASHSIZE	71
static OSSYM	*Stab[HASHSIZE] = {NULL};

/*{
** Name:    ossymput() -	Place New Symbol in Symbol Table.
**
** Description:
**	Place a new symbol into the symbol table.  The only type of
**	symbols that should be placed here are forms or variables.
**
** Input:
**	name	{char *}  The symbol name.
**
**	kind	{nat}  The kind of the symbol, OSFRAME, OSPROC, OSLABEL,
**			OSFIELD, OSCOLUMN, OSVAR, OSHIDCOL, OSTABLE.
**
**	dbdtype	{DB_DATA_VALUE *}  The DB_DATA_VALUE type description.
**
**	parent	{OSSYM *}  The object's parent.
**	flags	{nat}	to stash in runtime symbol table.
**
** History:
**	09/25/85 (scl) OSL should expand i2 symbols into sizeof(i4)
**	04/07/91 (emerson)
**		Modification for local procedures:
**		force name to be non-null (ossymfll and IGsymbol now expect it).
*/

OSSYM	*
ossymput (name, kind, dbdtype, parent, flags)
char		*name;
i4		kind;
DB_DATA_VALUE	*dbdtype;
OSSYM		*parent;
i4  		flags;
{
    OSSYM	*s;
    register OSSYM  **lt;
    char *normal_name;
    
    if (name == NULL)
    {
	name = ERx("");
    }
    if (ossympeek(name, parent) != NULL)
    {
#ifdef OSL_TRACE
        	SIprintf("ossymput: symbol already exists: name=|%s|\n",
                	name
                );
#endif /* OSL_TRACE */
        return NULL;
    }

#ifdef OSL_TRACE
	{
		char *pname = (parent == NULL || parent->s_name == NULL) ?
					 ERx("") : parent->s_name;
		i4 type = (dbdtype == NULL) ? 0 : dbdtype->db_datatype;

        	SIprintf("ossymput: new symbol: name=|%s|, kind=%d, type=%d, parent=|%s|\n",
                	name, kind, type,  pname
                );
	}
#endif /* OSL_TRACE */

    
    /*
    ** Allocate and fill in after getting the normalized string table entry.
    */
    normal_name = normalize_name(name);

    s = ossymfll(normal_name, kind, dbdtype, parent);
    s->s_flags = flags;
    
    ossymflush(s);
    
    /*
    ** Attach to symbol table.
    */
    
    /* use name in entry as filled in by 'ossymfll()' */
    lt = &Stab[symhash(s->s_name)];
    s->s_next = *lt;
    *lt = s;

    return s;
}

/*{
** Name:    ossymflush() -	Place Symbol in ILG Symbol Table.
**
** Description:
**	Place a symbol into the compiled symbol table.  
**
** Input:
**	s	{OSSYM *}  The symbol to send to ILG.
**
** History:
**	07/21/89 (billc) written
**	04/07/91 (emerson)
**		Modification for local procedures:
**		force parent and typename to be non-null
**		(ossymfll and IGsymbol now expect it).
*/
ossymflush (s)
register OSSYM	*s;
{
	if (s->s_kind == OSFIELD 
	  || s->s_kind == OSVAR 
	  || s->s_kind == OSGLOB 
	  || s->s_kind == OSACONST 
	  || s->s_kind == OSCOLUMN 
	  || s->s_kind == OSHIDCOL )
	{
		char	*parent = ERx("");
		char	*typename;
		char	visible;
		i4	length;

		if (s->s_type == DB_NODT)
		{
		    	s->s_ilref = 0;
			return;
		}

		length = s->s_length;
		switch (s->s_kind)
		{
		  case OSFIELD:
		  case OSCOLUMN:
			visible = 'v';
			break;
		  case OSACONST:
			if ( oschkstr(s->s_type) )
				length = 0;
			/* fall through . . . */
		  case OSGLOB:
			visible = 'g';
			break;
		  default:
			visible = 'h';
			break;
		}

		if (s->s_parent->s_kind == OSTABLE)
		{
			parent = s->s_parent->s_name;
		}

		typename = s->s_typename;
		if (typename == NULL)
		{
			typename = ERx("");
		}

	    	s->s_ilref = IGallocRef( (bool)(visible == 'g'),
						s->s_type, length, s->s_scale
		);

	    	IGsymbol(s->s_name, parent, s->s_ilref, 
				visible, (i4)s->s_flags, typename
			);
	}
}

/*{
** Name:	ossymfll()	- Allocate and Fill in a Symbol.
**
** Description:
**	Allocate a symbol node and fill in the information in it.
**
** Input:
**	name	{char *} Symbol name.  Must be non-null.
**	kind	{nat}  One of OSVAR, OSFIELD ...
**	dbdtype	{DB_DATA_VALUE *}  DB data value type.
**	parent	{OSSYM *}  Parent symbol node, can be NULL.
**
** Returns:
**	{OSSYM *}  New symbol node (reference).
**
** History:
**	08-aug-1985 (joe)
**		Broken out of ossymput so I could allocate temps without
**		putting them in the symbol table.
**	10/87 (jhw) -- Added support for constants.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Lumped OSACONST in with OSVAR and OSGLOB under s_vars.
**		Added OSFORM case.  Also, for general cleanliness,
**		made OSLABEL, OSFRAME, and OSPROC symbols true children
**		of their parent.  (Before, the parent's s_sibling field
**		was set to point to the new child.  Now, the parent's
**		s_children[0] field is set to point to the new child).
**		Also generalized the OSUNDEF case to work for most any
**		kind of parent.
**	15-jun-92 (davel)
**		Initialize s_scale and s_length.
*/

OSSYM *
ossymfll (name, kind, dbdtype, parent)
char		*name;
i4		kind;
DB_DATA_VALUE	*dbdtype;
OSSYM		*parent;
{
	register OSSYM	*s;

	OSSYM	*ossymalloc();		/* static forward reference */

	/*
	** Fill in the info.
	*/
	s = ossymalloc();
	s->s_name = name;
	s->s_kind = kind;
	s->s_ref = OS_NOTREF;
	s->s_scale = 0;
	s->s_length = 0;
	s->s_brightness = OS_DARK;

	if (dbdtype != NULL)
	{
		s->s_type = dbdtype->db_datatype;
		s->s_typename = dbdtype->db_data;
		s->s_length = dbdtype->db_length;
		s->s_scale = dbdtype->db_prec;
	}

	/*
	** Fill in the parent info.
	*/
	s->s_parent = parent;
	if (parent != NULL)
	{
		OSSYM **lt;

		switch (kind)
		{
		  case OSFRAME:
		  case OSPROC:
		  case OSLABEL:
			lt = &parent->s_children[0];
			break;

		  case OSFORM:
			lt = &parent->s_subprocs;
			break;

		  case OSTABLE:
			lt = &parent->s_tables;
			break;

		  case OSCOLUMN:
		  case OSHIDCOL:
			lt = &parent->s_columns;
			break;

		  case OSRATTR:
			lt = &parent->s_attributes;
			break;

		  case OSGLOB:
		  case OSVAR:
		  case OSACONST:
			lt = &parent->s_vars;
			break;

		  case OSFIELD:
			lt = &parent->s_fields;
			break;

		  case OSUNDEF:
			switch (parent->s_kind)
			{
			  case OSFORM:
				lt = &parent->s_vars;
				break;

			  case OSTABLE:
				lt = &parent->s_columns;
				break;

			  case OSRATTR:
			  case OSGLOB:
			  case OSVAR:
			  case OSACONST:
				lt = &parent->s_attributes;
				break;

			  default:
				lt = &parent->s_children[0];
				break;
			}
			break;

		  default:
			oscerr(OSBUG, 1, ERx("ossymfll()"));
			break;
		} /* end switch */
		s->s_sibling = *lt;
		*lt = s;
	}
	return s;
}

/*{
** Name:	ossymundef()	- create and return a 'undefined' symbol.
**
** Description:
**	Allocate an 'undefined' symbol node, for marking an error.
**
** Input:
**	name	{char *} Symbol name.
**	parent	{OSSYM *}  Parent symbol node, can be NULL.
**
** Returns:
**	{OSSYM *}  New symbol node (reference).
**
** History:
**	10/89 (billc) -- written
*/
OSSYM *
ossymundef(name, parent)
char *name;
OSSYM *parent;
{
	OSSYM *sym;
	
    	if ((sym = ossympeek(name, parent)) == NULL || sym->s_kind != OSUNDEF)
	{
    		sym = ossymput(name, OSUNDEF, &osNodt, parent, 0);
	}
	return sym;
}

/*{
** Name:	ossymlook() -	Find Symbol in Symbol Table.
**
**	Find a symbol in the symbol table given the name and its parent.
**	If the symbol is not found and the parent is an OSFORM symbol
**	(representing the scope of a frame, procedure, or local procedure),
**	see if there is a global in the database with that name.
**	If so, retrieve the global symbol and add it to our symbol table.
**	Make it a child of the parent specified by the caller,
**	except in the case where the parent has adopted all children
**	of "bright" symbols.  This represents the case where the specified
**	parent is the "top" OSFORM symbol (FormSym in oslgram.my).
**	The "bright" symbols are all the OSFORM symbols for the "current"
**	local procedure (or main procedure or frame) and its "ancestors",
**	all the way up throught the "top" OSFORM symbol.  In this case,
**	we make the global symbol a child of the OSFORM symbol representing
**	the *current* local procedure.
**
** Input:
**	name	{char *}  The name of the symbol for which to look.
**	parent	{OSSYM *}  The parent of the symbol.
**
** History:
**	15-jul-86 (mgw) - bug # 8918  Also use non-case comparison to support
**	case insensitivity for names in the symbol table.
**	04/07/91 (emerson)
**		Modifications for local procedures: Add logic
**		to make the global symbol a child of the oldest
**		OSFORM ancestor (if the specified parent is an OSFORM).
**	04/21/92 (emerson)
**		Fix for bug 43771: Back out the modification I made on 04/07/91,
**		and add logic to make a new global symbol a child of the
**		*current* local procedure, in the case where the called
**		specifies that the parent has adopted all "bright" children
**		(i.e. where the parent is the form or main procedure).
**		The problem is that adding an entry to the symbol table causes
**		a DBV to be allocated, and if we're in a local procedure,
**		the DBV will "belong" to the local procedure and get overlaid by
**		the DBVs for other local procedures at run time.
**		So other local procedures can't use the same symbol table
**		entry that was created by the first local procedure.
**		With the fix, the behavior will now be that
**		if two local procedures refer the same global object,
**		and the main frame/procedure hasn't referred to it, then
**		each local procedure will create a different symbol table
**		entry (and associated DBV) for the global object.
**	09/20/92 (emerson)
**		Don't look for a global symbol unless the parent is a "form"
**		(i.e. something whose children are variables or constants).
**		This was causing various misleading error symptoms in certain
**		cases where there was a reference to non-existent attribute
**		or column of a record, array, or tablefield, and there
**		happened to be a global variable of the same name.
**		It was also part of the reason for bug 44056.  (The rest
**		of the reason was that the "var_colon: COLID" production
**		of the grammar had an inappropriate call to ossymlook).
*/
OSSYM *
ossymlook (name, parent)
    char	*name;
    OSSYM       *parent;
{
    OSSYM	*lt;
    OSSYM	*osgl_fetch();
    
    lt = ossympeek(name, parent);
    
    if (lt == NULL && !QUEL && parent->s_kind == OSFORM)
    {
	if ( parent->s_ref & OS_ADOPT_CHILDREN_OF_BRIGHT )
	{
	    lt = osgl_fetch ( osgetcurrentproc(), name );
	}
	else
	{
	    lt = osgl_fetch ( parent, name );
	}
    }
    
    return lt;
}

/*{
** Name:	ossympeek() -	Find Symbol with Specified Parent.
**
**	Find a symbol in the symbol table given the name and its parent.
**	Hash the name and look down the hash chain for the youngest symbol
**	with the specified name and parent.  If the parent has adopted all
**	children of "bright" symbols then any adopted child of the specified
**	parent will be selected if it (the child) has the specified name.
**
**	Note that the only symbol that may be such a parent
**	is the OSFORM symbol representing the top form or procedure;
**	osdefineproc has ensured that the current form or procedure
**	and its OSFORM ancestors (and no other symbols) have been marked
**	as "bright".
**
**	Also note that no symbol is ever marked as "extra bright",
**	except temporarily by this routine; "bright" is normally
**	the highest possible brightness.
**
** Input:
**	name	{char *}  The name of the symbol for which to look.
**	parent	{OSSYM *}  The parent of the symbol.
**
** History:
**	15-jul-86 (mgw) - bug # 8918  Also use non-case comparison to support
**	case insensitivity for names in the symbol table.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved logic to the new routine ossymsearch,
**		and made this routine a cover for it,
**		so that we'll handle adopted children properly.
*/

OSSYM *
ossympeek (name, parent)
char	*name;
OSSYM	*parent;
{
	if ( parent->s_ref & OS_ADOPT_CHILDREN_OF_BRIGHT )
	{
		return ossymsearch( name, OS_DARK, OS_BRIGHT );
	}
	else
	{
		i2	save_brightness;
		OSSYM	*sym;

		save_brightness = parent->s_brightness;
		parent->s_brightness = OS_EXTRA_BRIGHT;
		sym = ossymsearch( name, OS_DARK, OS_EXTRA_BRIGHT );
		parent->s_brightness = save_brightness;
		return sym;
	}
}

/*{
** Name:	ossymsearch() -	Find Symbol of Specified Visibilty.
**
**	Find a symbol in the symbol table given its name and "visibilty".
**	Hash the name and look down the hash chain for the youngest
**	"visible" symbol with the specified name.  A symbol will be "visible"
**	if it's at least as bright as the specified minimum brightness
**	and its parent is at least as bright as the specified minimum
**	parent brightness.
**
** Input:
**	name			{char *}  The name of the symbol.
**	min_brightness		{nat}	  The minimum brightness acceptable
**					  for the symbol.
**	min_parent_brightness	{nat}	  The minimum brightness acceptable
**					  for the symbol's parent.
**
** Usage Notes:
**	To use ossymsearch effectively, you should understand how
**	"brightness" (the s_brightness field) is set for symbol table entries.
**	Here's a brief explanation (copied from ossym.h):
**
**	OS_DARK		indicates a non-OSFORM symbol.
**	OS_DIM		indicates an OSFORM symbol that's *not* an ancestor
**			of the current form or procedure.
**	OS_BRIGHT	indicates an OSFORM symbol for the current form or
**			procedure, or one of its OSFORM ancestors.
**	OS_EXTRA_BRIGHT is a special value used only by ossympeek.
**
**	Given the above, you can see that various sorts of searches
**	are possible.  For example:
**
**	sym = ossymsearch( name, OS_DARK,   OS_BRIGHT )	yields a field, var,
**							constant, or local proc
**							with the specified name
**							that's declared in the
**							current frame or proc or
**							in an ancestor thereof.
**
**	sym = ossymsearch( name, OS_DIM,    OS_DARK   )	yields a local proc
**							or form or main proc
**							with the specified name.
**
**	sym = ossymsearch( name, OS_DIM,    OS_DIM    )	yields a local proc
**							with the specified name.
**
**	sym = ossymsearch( name, OS_DIM,    OS_BRIGHT )	yields a local proc
**							with the specified name
**							that's declared in the
**							current frame or proc or
**							in an ancestor thereof.
**
**	sym = ossymsearch( name, OS_BRIGHT, OS_BRIGHT )	yields a local proc
**							with the specified name
**							that's an ancestor of
**							the current local proc.
** History:
**	04/07/91 (emerson)
**		Written for local procedures.
**  12-feb-97 (rodjo04)  
**      Changed search algoithm so that it will now function with
**      change made to symhash().
**	14-may-97 (mcgem01)
**		A typo in the change to the search algorithm, resulted
**		in the use of a non-normailzed symbol introducting
**		bug 81514.
**
*/

OSSYM *
ossymsearch ( name, min_brightness, min_parent_brightness )
char	*name;
i4	min_brightness;
i4	min_parent_brightness;
{
	register OSSYM	*lt;
	char *normal_name;

	normal_name = normalize_name(name);

	for ( lt = Stab[ symhash( normal_name ) ]; lt != NULL; lt = lt->s_next )
	{
		if ( ( lt->s_name == normal_name || 
			( CMcmpnocase(name, lt->s_name) == 0 
		      && STbcompare(name, 0, lt->s_name, 0, TRUE) == 0)
			 ) 
		      && lt->s_brightness >= min_brightness
		      && lt->s_parent->s_brightness >= min_parent_brightness
		   )
		{
			break;
		}
	}

#ifdef OSL_TRACE
	if (lt == NULL)
	{
        	SIprintf("ossymsearch: symbol not found: name=|%s|\n",
                	normal_name
                );
	}
	else
	{
        	SIprintf("ossymsearch: symbol found: name=|%s|\n",
                	normal_name
                );
	}
#endif /* OSL_TRACE */

	return lt;
}

/*{
** Name:	ossymalloc() -	Allocate A Symbol Node.
**
** Description:
**	Allocate a symbol node for use by the OSL parser.  Symbol nodes
**	are allocated in a pool and returned as needed.
**
**	Note:  Symbol nodes are never freed!!  They may be re-used if they are
**	temporary, but the temporary allocation module manages that function.
**
** Returns:
**	{OSSYM *}  The address of the symbol node.
**
** History:
**	07/87 (jhw) -- Re-written.
*/

static OSSYM	*Symfree = NULL;
#define	SYMBLK 	32

OSSYM *
ossymalloc ()
{
    register OSSYM	*symp;

    if (Symfree == NULL)
    {
	STATUS	stat;

	if ((Symfree = (OSSYM*)MEreqmem(0, SYMBLK * sizeof(OSSYM), TRUE, &stat))
			== NULL)
	    osuerr(OSNOMEM, 1, ERx("ossymalloc"));

	for (symp = Symfree ; symp < &Symfree[SYMBLK - 1] ; ++symp)
	    symp->s_next = symp + 1;
	symp->s_next = NULL;
    }

    symp = Symfree;
    Symfree = Symfree->s_next;

    return symp;
}

/*
** Name:    symhash() -	Hash Symbol Name for Symbol Entry.
**
** Description:
**	Hash the symbol name by value (i.e., name returned by 'iiIG_string()'.)
**
** Input:
**	name	{char *}  Name to hash.
**
** Returns:
**	{nat}  Hash value for name.
**
** History:
**	15-jul-86 (mgw)
**		bug # 8919  Alter hashing algorithm to hash on the
**		lower case representation of the name rather than on the 
**		address of the storage for the name.
**	20-aug-92 (davel)
**		Change hashing algorithm back to hashing on the address
**		of the storage for the name, as now we are ensured of
**		this address already being in the string table.
**      06-dec-93 (smc)
**		Bug #58882
**          	Commented lint pointer truncation warning.
**  12-feb-97 (rodjo04)
**      bug # 79308 Changed hashing algorithm back to hashing on the
**      lower case representation of the name rather than on the 
**      address of the storage for the name.
*/
static i4
symhash (name)
register char	*name;
{
    	
    register i4    value = 0;
    register char       *sp;
    char                buf[OSBUFSIZE+1];

    STcopy(name, buf);
    CVlower(buf);

    for (sp = buf ; *sp != EOS ; ++sp)
        value += (value << 2) + *sp;

    if ((value %= HASHSIZE) < 0)
        value = -value;
    return (i4)value;

}
/*{
** Name:	ossymdump() -	dump the symbol table
**
**	Dump out the symbol table into the IL debug file.
**
** Input:
**	dfile_p         {FILE *}  Debug file pointer.
**
** History:
*/
ossymdump (dfile_p)
FILE   *dfile_p;
{
	i4 i;

	if (dfile_p == NULL)
		return;

	for (i = 0; i < HASHSIZE; i++)
	{
		OSSYM	*lt;

		for (lt = Stab[i]; lt != NULL; lt = lt->s_next)
		{
			char *p = ERx("<none>");

			if (lt->s_parent != NULL)
				p = lt->s_parent->s_name;
			SIfprintf(dfile_p,
				"\tsym[%d]\t\"%s\",\tkind=%d,\tparent=\"%s\"\n",
				i, lt->s_name, lt->s_kind, p);
		}
	}
}
static char *
normalize_name (name)
char *name;
{
	char *iiIG_string();

	char buffer[OSBUFSIZE];
	char *bp = buffer;

	if (name == NULL)
		return name;

	do
	{
		if (*name == '\"')
		{
			name++;
		}
		*bp = *name++;

	} while (*bp++ != EOS);

	/* force to lower case */
	CVlower(buffer);
	return iiIG_string(buffer);
}
