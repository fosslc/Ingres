# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# define EQ_EUC_LANG	/* for eqsym.h */
# include	<eqsym.h>
# include	<equel.h>
# include	<eqrun.h>
# include	<eqscan.h>
# include	<eqgen.h>
# include	<ereq.h>
# include	<ere2.h>
# include	<eqpas.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

/*
+* Filename:	pasutil.c
** Purpose:	PASCAL utilities for the WITH statement & forward declarations.
**
** Defines:
**	pas_vadjust( sy, vlue )			- Adjust an entry's value
**	pas_prtval( vlue )			- Print a entry's value
**	pas_badindir( sy, name, dims, indir )	- Check for indir compatibility
**	pas_isbad( sy, dims, indir, indir )	- Local check for compatibility
**	pas_init()				- Do any PASCAL-specific init
**	pasWith( is_begin )			- Begin/End a WITH block
**	pasWthLookup( name, closure, use )	- Look up a name in a WITH block
**	pasWthAdd( sym, name )			- Save a symtab ptr
**	pasWthExit()				- Prune WITH stack on proc exit
**	pasWthDump()				- Dump the WITH stack
**	pas_set_sym( sy, tos, ndir, flags )	- Set stacked syms type to "sy"
**	pas_check_attribs( sy, vlue )		- Check attribs for conflict
**	pas_push( val )				- Push a name
**	pas_pop( count )			- Pop some names
**	pas_count()				- How many names were pushed?
**	pas_reset()				- Clear the integer stack
**	pas_semi_force()			- Force semicolon as next token
**	pas_lookup( p )				- (Local) Get attrib from string
**	pas_attrib()				- Gen stored text; return attrib
**	pas_text( buf )				- Store text eaten by SC_EAT
**	pas_declare( count, type_bits, the_type, the_indir, rec_level,
**	    blk_level, the_dims, attrib_flags ) - Declare a stack of names
**	pas_undeclared( name, lflags, rflags, count, the_type, rec_lev,
**	    blk_lev, undefsy )			- Declare an undeclared name
**	pas_isdecl( name, sy_flags, dfltsy, rec_lev, blk_lev, undefsy,
**	    the_type, the_siz )			- Ensure a name is declared
** Notes:
**
** History:
**	04-oct-85	- Written (mrw)
**	16-dec-1992 (larrym)
**	    Added SQLCODE and SQLSTATE support.  
**      08-30-93 (huffman)
**              add <st.h>
**	11-feb-98 (kinte01)
**		add a definiton of sc_dbglang
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

GLOBALREF SC_DBGCOMMENT *sc_dbglang ZERO_FILL;  /* Default is unused */

/*
** pas_isbad values
*/

# define	paGOOD		0
# define	paBADINDIR	1
# define	paBADSUBSCRPT	2
# define	paMUSTPACK	3
# define	paCANTPACK	4

/*
** the WITH stack
*/

typedef struct pas_with {
    short	pw_level;
    SYM		*pw_sym;
} PAS_WITH;

# define PAS_WTH_MAX	20

static i4	pas_w_level = 0;		/* current WITH block level */
static PAS_WITH	pas_wstack[PAS_WTH_MAX] ZERO_FILL,
		*pas_wptr = pas_wstack-1;

/*
** the string stack
*/

# define PAS_STK_MAX	2000

static char	*pas_stack[PAS_STK_MAX] ZERO_FILL;
static char	**stack_ptr = pas_stack;

/* The language-specific comment-function table */

i4	pasWthDump();
i4	pasLbSave();
i4	pasLbDump();

GLOBALDEF SC_DBGCOMMENT pas_htab[] = {
    { TRUE,  ERx("paswith"),pasWthDump,0, ERx("\t\tDump the WITH state") },
    { TRUE,  ERx("pasfile"),pasLbSave, 0, ERx("\t\tPreserve EQP temp file") },
    { TRUE,  ERx("paslab"), pasLbDump, 0, ERx("\t\tDump EQP label structure") },
    { FALSE, (char *)0,	    0,	       0, (char *)0 }
};

/*
** The attribute scanning data structures
*/

# define	P_LINEMAX	400		/* STR_MAXBUF */
static char	*pas_ptr = NULL;

typedef struct {
    char	*at_name;
    i4		at_value;
} ATTRIB;

static ATTRIB	attribs[] = {
    { ERx("BYTE"),	PvalBYTE },
    { ERx("WORD"),	PvalSHORT },
    { ERx("LONG"),	PvalLONG },
    { ERx("QUAD"),	PvalQUAD },
    { ERx("INHERIT"),	PvalINHERIT },
    { ERx("EQUEL"),	PvalEQUEL },
    { ERx("ESQL"),	PvalESQL },
    { NULL,	0 }
};

/*
+* Procedure:	pas_vadjust
** Purpose:	Adjust the value field of an entry according to attributes.
** Parameters:
**	sy	- SYM *	- The entry to adjust.
**	vlue	- i4	- The attribute bits to set.
** Return Values:
-*	None.
** Notes:
**	Check for attribute conflicts and then set the value and dsize fields.
*/

i4
pas_vadjust( sy, vlue )
SYM	*sy;
i4	vlue;
{
    i4		val;
    i4		dsize;

  /* check for flag conflicts */
    dsize = pas_check_attribs( sy, &vlue, sym_g_dsize(sy) );
    val = (i4) sym_g_vlue(sy) | vlue;
    sym_s_vlue( sy, val );
    sym_s_dsize( sy, dsize );
}

/*
+* Procedure:	pas_prtval
** Purpose:	Print a string decoding the value field of an entry.
** Parameters:
**	class	- char	- The flag bits.
** Return Values:
-*	None.
** Notes:
**	None.
*/

pas_prtval( vlue )
i4	vlue;			/* PUN!!! really "i4 *" */
{
    static struct x {
	int	mask;
	char	on, off;
    } names[] = {
	{ PvalVARYING,	'V', '-' },
	{ PvalPACKED,	'P', '-' },
	{ PvalBYTE,	'B', '-' },
	{ PvalSHORT,	'S', '-' },
	{ PvalLONG,	'L', '-' },
	{ PvalQUAD,	'Q', '-' },
	{ PvalUNSUPP,	'U', '-' },
#ifdef	MVS
	{ PvalSTRPTR,	'R', '-' },
#endif
	{ 0,		' ', '-' }
    };
    char	buf[12];
    register struct x
		*x;

    for (x=names; x->mask; x++)
    {
	if (vlue & x->mask)
	    buf[x-names] = x->on;
	else
	    buf[x-names] = x->off;
    }
    buf[x-names] = '\0';
    trPrval( buf );
}

/*
+* Procedure:	pas_badindir
** Purpose:	Print an error message if the indirection or dims are bad.
** Parameters:
**	sy	- SYM *	- The symtab entry
**	name	- char *- The name of the var for error messages
**	dims	- i4	- The indexing used
**	indir	- i4	- The indirection used
** Return Values:
-*	None.
** Notes:
**	Let pas_isbad decide if anything's bad, then print a message if needed.
*/

i4
pas_badindir( sy, name, dims, indir )
SYM	*sy;
char	*name;
i4	dims;
i4	indir;
{
    i4		sy_flags;
    
  /* Type SET is illegal */
    if ((sy_flags=(i4)sym_g_vlue(sy)) & PvalUNSUPP)
    {
	er_write( E_E2001C_hpUNSUPP, EQ_ERROR, 1, name );
      /* turn it off so we won't complain again */
	sy_flags &= ~PvalUNSUPP;
	sym_s_vlue(sy, sy_flags);
    }

    switch (pas_isbad(sy, dims, indir))
    {
      case paBADINDIR:		/* Illegal indirection */
	er_write( E_E20008_hpINDIR, EQ_ERROR, 3, name, er_na(sym_g_indir(sy)),
								er_na(indir) );
	break;
      case paBADSUBSCRPT:	/* Illegal index */
	er_write( E_E20007_hpINDEX, EQ_ERROR, 3, name, er_na(sym_g_dims(sy)),
								er_na(dims) );
	break;
      case paMUSTPACK:		/* Must be packed or varying */
	er_write( E_E20005_hpDIMSBAD, EQ_ERROR, 1, name );
	break;
      case paCANTPACK:		/* Must not be packed or varying */
	er_write( E_E20011_hpNOTPACKED, EQ_ERROR, 1, name );
	break;
    }
}

/*
+* Procedure:	pas_isbad
** Purpose:	Return a code indicating if the used indir or dims is bad.
** Parameters:
**	sy	- SYM *	- The symtab entry
**	dims	- i4	- The indexing used
**	indir	- i4	- The indirection used
** Return Values:
-*	i4	- paGOOD, paBADINDIR, paBADSUBSCRPT, or paILLEGALDIMS.
** Notes:
**	a)  CHAR vars are one byte scalar quantities.
**	b)  CHAR and STRING vars are always sent by EQUEL by foreign mechanism
**	    (%descr or %stdescr).
**	c)  PACKED ARRAY OF CHAR and VARYING OF CHAR (hence known as "STRINGS")
**	    are assignment-compatible with literal strings. Although VMS PASCAL
**	    allows STRINGS to be indexed, it won't allow the result to be sent
**	    by foreign mechanism.  Therefore we won't allow them to be indexed.
**	d)  Non-PACKED and non-VARYING ARRAYS OF CHAR can be used iff a single
**	    element is being passed; give a special message if this is false.
**	e)  Indirection must always match; strings are not just pointers
**	    to char.
**	Rules:
**	    1.	PACKED ARRAY OF CHAR and VARYING OF CHAR must be indexed
**		one less then the symtab dimension (must send as a chunk).
**		ARRAY OF CHAR which is not PACKED or VARYING must send only
**		individual elements.
**	    2.	Anything else must have the same indexing.
**	    3.	Define EXACT_DIM_USAGE to get the "correct" usage -- but this
**		is incompatible with version 3.0 and earlier.  "Compatible"
**		usage is:
**			* Non-strings declared with any dimensions must be
**			  referenced with some dimensions.
**			* Non-strings declared without dimensions must be
**			  referenced without any dimensions.
**			* Strings declared with extra dimensions must be
**			  referenced with some dimensions.
**			* Strings declared without extra dimensions must be
**			  referenced without any dimensions.
*/

i4
pas_isbad( sy, dims, indir )
SYM	*sy;
i4	dims;
i4	indir;
{
    i4		sy_indir = sym_g_indir(sy);
    i4		sy_dims = sym_g_dims(sy);
    i4		sy_flags = (i4) sym_g_vlue(sy);

    if (indir != sy_indir)
	return paBADINDIR;

#ifdef	EXACT_DIM_USAGE
    if (sym_g_btype(sy) == T_CHAR)
    {
      /* Strings */
	if (sy_flags & (PvalPACKED|PvalVARYING))
	{
	    if (sy_dims == dims + 1)
		return paGOOD;
	    if (sy_dims == dims)
		return paCANTPACK;
	    return paBADSUBSCRPT;
	}
      /* Plain chars */
	if (sy_dims == dims + 1)
	    return paMUSTPACK;
	if (sy_dims == dims)
	    return paGOOD;
	return paBADSUBSCRPT;
    }
  /* Non-chars */
    if (sy_dims != dims)
	return paBADSUBSCRPT;
    return paGOOD;
#else	/* EXACT_DIM_USAGE */
    if (sym_g_btype(sy) == T_CHAR)
    {
      /* Strings */
	if (sy_flags & (PvalPACKED|PvalVARYING))
	{
	    sy_dims--;
	    if ((sy_dims && dims) || (!sy_dims && !dims))
		return paGOOD;
	    if (sy_flags & PvalPACKED)
	    {
		if (sy_dims)		/* && !dims */
		    return paBADSUBSCRPT;
		return paGOOD;
	    }
	  /* Varying */
	    if (!sy_dims)		/* && dims */
		return paCANTPACK;
	    return paBADSUBSCRPT;	/* sy_dims && !dims */
	}
      /* Plain chars */
	if ((sy_dims && dims) || (!sy_dims && !dims))
	    return paGOOD;
	if (sy_dims)			/* && !dims */
	    return paMUSTPACK;
	return paBADSUBSCRPT;		/* !sy_dims && dims */
    }
  /* Non-chars */
    if ((sy_dims && dims) || (!sy_dims && !dims))
	return paGOOD;
    return paBADSUBSCRPT;
#endif	/* EXACT_DIM_USAGE */
}

/*
+* Procedure:	pas_init
** Purpose:	Do all PASCAL-specific initialization.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Just set the host-language specific comment function table.
*/

pas_init()
{
    sc_dbglang = pas_htab;
    sym_adjval = pas_vadjust;
    sym_prtval = pas_prtval;
    pas_w_level = 0;
    pas_wptr = pas_wstack-1;
    pas_reset();	/* make sure we start with a clean mode stack */
    pasLbInit();	/* reset goto-label pass */
  /* start indenting at the left margin */
    gen_do_indent( G_IND_RESET );
}

/*
+* Procedure:	pasWith
** Purpose:	Begin/End a WITH block.
** Parameters:
**	is_begin- bool	- TRUE ==> begin, FALSE ==> end
** Return Values:
-*	None.
** Notes:
**	Save the (field-stacked) symtab pointers with the current WITH block
**	nesting level if this is a Begin, else pop all of the symtab pointers
**	at the current nesting level.
**	BEGIN clears the (field) stack;
*/

void
pasWith( is_begin )
bool	is_begin;
{
    if (is_begin)
    {
	pas_w_level++;
    } else
    {
	if (pas_w_level < 1)
	{
	    er_write( E_E20021_hpWTHUFLOW, EQ_FATAL, 0 );
	} else
	{
	    do {
		--pas_wptr;
	    } while (pas_wptr >= pas_wstack && pas_wptr->pw_level==pas_w_level);
	    pas_w_level--;
	}
    }
}

/*
+* Procedure:	pasWthAdd
** Purpose:	Stash away a symtab pointer and an associated level.
** Parameters:
**	sy	- SYM *	- The symtab pointer.
**	name	- char *- The name of the entire entry, for error messages.
** Return Values:
-*	FALSE if we ran out of room, else TRUE.
** Notes:
**	Straightforward -- rejects attempts to add NULL pointers.
*/

bool
pasWthAdd( sy, name )
SYM	*sy;
char	*name;
{
    if (!sy)
    {
	er_write( E_E2001D_hpWTHNULL, EQ_FATAL, 1, name );
	return FALSE;
    } else if (pas_wptr >= &pas_wstack[PAS_WTH_MAX-1])
    {
	er_write( E_E2001E_hpWTHOFLOW, EQ_ERROR, 2, name, er_na(PAS_WTH_MAX) );
	return FALSE;
    } else
    {
	if (sym_g_btype(sy) == T_STRUCT)
	{
	    ++pas_wptr;
	    pas_wptr->pw_level = pas_w_level;
	    pas_wptr->pw_sym = sy;
	    return TRUE;
	} else
	{
	    er_write( E_E20020_hpWTHSTRUCT, EQ_ERROR, 1, name );
	    return FALSE;
	}
    }
}

/*
+* Procedure:	pasWthLookup
** Purpose:	Lookup a name honoring the current WITH blocks.
** Parameters:
**	name	- char *- The name to look up.
**	closure	- i4	- The closure in which to look it up.
**	use	- i4	- The usage bits to match.
** Return Values:
-*	SYM * - A pointer to the symtab entry if any, else NULL.
** Notes:
**	Resolve the name in the context of each WITH block, last first,
**	until one matches; then return it.  If none matches then
**	return an ordinary lookup.
*/

SYM *
pasWthLookup( name, closure, use )
char	*name;
i4	closure;
i4	use;
{
    register PAS_WITH	*w;
    SYM			*sy;

    for (w=pas_wptr; w >= pas_wstack; w--)
    {
	if (sy=sym_resolve(w->pw_sym, name, closure, use))
	    return sy;
    }
    return sym_resolve( (SYM *)0, name, closure, use );
}

/*
+* Procedure:	pasWthExit
** Purpose:	Clean up the WITH stack on procedure exit.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Complain if there's still an open WITH block.
**	Toss all with blocks.
*/

void
pasWthExit()
{
    if (pas_wptr != pas_wstack-1)
    {
	er_write( E_E2001F_hpWTHOPEN, EQ_ERROR, 0 );
	pas_wptr = pas_wstack-1;
	pas_w_level = 0;
    }
}

/*
+* Procedure:	pasWthDump
** Purpose:	Dump the WITH stack.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Just walk through the table dumping entries.
*/

i4
pasWthDump()
{
    register PAS_WITH	*w;

    SIfprintf( eq->eq_dumpfile, ERx("With Table Dump (level = %d)\n"),
								pas_w_level );
    SIfprintf( eq->eq_dumpfile, ERx("Level\tName\n---\t--------\n") );
    for (w=pas_wstack; w<=pas_wptr; w++)
	SIfprintf( eq->eq_dumpfile, ERx("%d\t%s\n"),
	    w->pw_level, sym_str_name(w->pw_sym) );
}

/*
+* Procedure:	pas_set_sym
** Purpose:	Set the type field of the top stacked entries.
** Parameters:
**	srcsy	- SYM *	- The type to which to set "tgtsy".
**	tgtsy	- SYM * - The entry to be modified.
**	indir	- i4	- The "extra" indirection of the declaration.
**	dims	- i4	- The "extra" dimensions of the declaration.
**	flags	- i4	- Any "attribute" flags.
** Return Values:
-*	SYM	*tgtsy;
** Notes:
**	If there are any stacked entries (should always be true) then
**	Set the "type" of all of "tgtsy" to "srcsy", and the "btype", "indir",
**	"dims",  and "dsize" fields to those of the "srcsy", but add in the
**	"extra" indirection that appeared in this declaration.
**
**	The idea is that in a declaration such as
**	    a, b, c, d : integer;
**	we'll declare "a", "b", "c", and "d" and stack their symtab pointers.
**	When we resolve "integer" we'll set these stacked entries to point
**	to the correct type (and pop them).  We pay attention to the record
**	level since this declaration may have been nested in a record.
**	It's up to the caller to pop the stacked entries; we return the value
**	to which they should pop.  We don't have to worry about crossing scope
**	blocks because the stack will be empty at block exit.
**
** Imports modified:
**	The stacked entries.
*/

SYM *
pas_set_sym( srcsy, tgtsy, indir, dims, flags )
register SYM	*srcsy;
register SYM	*tgtsy;
i4		indir;
i4		dims;
i4		flags;
{
    i4		level;
    i4		sy_indir;
    i4		sy_dims;
    i4		sy_dsize;
    i4		sy_btype;

    if (tgtsy)
    {
	if (srcsy)
	{
	    sy_dsize = sym_g_dsize( srcsy );
	    sy_btype = sym_g_btype( srcsy );
	    if (sy_btype == T_FORWARD)
	    {
		sy_dims = dims;
		sy_indir = indir;
	    } else
	    {
		sy_dims = dims + sym_g_dims( srcsy );
		sy_indir = indir + sym_g_indir( srcsy );
	    }
	} else
	{
	    sy_indir = indir;
	    sy_dims =  dims;
	    sy_dsize = 0;
	    sy_btype = T_NONE;
	}

      /* check for flag conflicts */
	sy_dsize = pas_check_attribs( tgtsy, &flags, sy_dsize );

      /* adjust the target entry */
	sym_s_type( tgtsy, srcsy );
	sym_s_vlue( tgtsy, (i4) sym_g_vlue(srcsy) | flags );
	sym_s_dsize( tgtsy, sy_dsize );
	sym_s_btype( tgtsy, sy_btype );
	sym_s_dims( tgtsy, sym_g_dims(tgtsy)+sy_dims );
	sym_s_indir( tgtsy, sym_g_indir(tgtsy)+sy_indir );
    }
    return tgtsy;
}

/*
+* Procedure:	pas_check_attribs
** Purpose:	Check for attribute conflicts
** Parameters:
**	sy	- SYM *	- The entry whose attributes to check.
**	vlue	- i4 *	- The address of the bits to add to it.
**	dsize	- i4	- The assumed base size of the object.
** Return Values:
-*	i4	- The size to use for the object.
** Notes:
**	Complains if there are any conflicts.
**	Zeros "vlue" if garbage is given.
*/

i4
pas_check_attribs( sy, vlue, dsize )
SYM	*sy;
i4	*vlue;
i4	dsize;
{
    switch (*vlue & (PvalSIZES|PvalVARYING))
    {
      case 0:
      case PvalVARYING:
	break;
      case PvalBYTE:
	dsize = sizeof(char);
	break;
      case PvalSHORT:
	dsize = sizeof(short);
	break;
      case PvalLONG:
	dsize = sizeof(long);
	break;
      case PvalQUAD:
	dsize = 2*sizeof(long);
	break;
      default:
	er_write( E_E20001_hpBADATTRIB, EQ_ERROR, 1, sym_str_name(sy) );
	*vlue = 0;
	break;
    }
    return dsize;
}

/*
+* Procedure:	pas_push
** Purpose:	Save a string for later recall.
** Parameters:
**	val	- char *	- The value to save
** Return Values:
-*	None.
** Notes:
**	Just implement a stack.
*/

pas_push( val )
char	*val;
{
    if (stack_ptr > &pas_stack[PAS_STK_MAX-1])
	er_write( E_E20014_hpOVERFLOW, EQ_ERROR, 1, er_na(PAS_STK_MAX) );
    else
	*stack_ptr++ = val;
}

/*
+* Procedure:	pas_pop
** Purpose:	Recall a saved string.
** Parameters:
**	None.
** Return Values:
-*	char ** - The address of the first of the saved values.
** Notes:
**	The other half of a stack.
*/

char **
pas_pop( count )
{
    if (stack_ptr - count < pas_stack)
	er_write( E_E2001B_hpUNDERFLOW, EQ_FATAL, 0 );	/* fatal error */
    else
	stack_ptr -= count;
    return stack_ptr;
}

/*
+* Procedure:	pas_count
** Purpose:	How many names are on the stack?
** Parameters:
**	None.
** Return Values:
-*	i4	- The number of names on the stack.
** Notes:
**	This should never be negative!
*/

i4
pas_count()
{
    return stack_ptr - pas_stack;
}

/*
+* Procedure:	pas_reset
** Purpose:	Clear the integer stack (on statement end).
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Reset the stack in case we had an error in the middle of a statement
**	which left the stack unclean.
*/

i4
pas_reset()
{
    stack_ptr = pas_stack;
}

/*
+* Procedure:	pas_text
** Purpose:	For pas_attrib: save the SC_EATen buffer.
** Parameters:
**	buf	- char *	- The buffer address.
** Return Values:
-*	None.
** Notes:
**	If the buffer is still in use then "sc_eat" overflowed;
**	this is truly amazing.  So amazing that we won't even bother to
**	recover, just throw away the leading part.
*/

pas_text( buf )
char	*buf;
{
    if (pas_ptr)
    {
	char	badbuf[25];

	STlcopy( buf, badbuf, 24 );
	er_write( E_EQ0215_scSTRLONG, EQ_ERROR, 2, er_na(P_LINEMAX), badbuf );
	str_free( NULL, pas_ptr );
    }
    buf[P_LINEMAX] = '\0';	/* don't want string manager complaints */
    pas_ptr = (char *)str_add( NULL, buf );
}

/*
+* Procedure:	pas_lookup
** Purpose:	Return which, if any, size attribute string this is.
** Parameters:
**	ptr	- char *	- Pointer to the string to lookup.
** Return Values:
-*	i4	- The flag bit for this size.
** Notes:
**	We ignore case; the input string must be nul-terminated.
**	We also can return bits for INHERIT, EQUEL, and ESQL.
*/

static i4
pas_lookup( ptr )
register char	*ptr;
{
    register ATTRIB	*at;

    for (at=attribs; at->at_name; at++)
	if (STbcompare(at->at_name, 0, ptr, 0, TRUE) == 0)
	    return at->at_value;
    return 0;
}

/*
+* Procedure:	pas_attrib
** Purpose:	Report on what attributes were set and emit the attribute expr.
** Parameters:
**	is_unit	- bool	- TRUE iff this attribute was for a compilation unit.
** Return Values:
-*	i4	- The attribute bits of interest.
** Notes:
**	We ignore all but size indications, and we ignore the parenthesized
**	stuff in them!
*/

i4
pas_attrib( is_unit )
bool	is_unit;
{
    register char	*p;
    register char	*q;
    i4			result = 0;

    if (pas_ptr == NULL)
	return 0;

    for (p=pas_ptr; *p;)
    {
	char	ch;

	while (*p && !CMalpha(p))
	    CMnext(p);
	if (*p == '\0')
	    break;
	q = p;
	while (CMalpha(p))
	    CMnext(p);
	ch = *p;
	*p = '\0';
	result |= pas_lookup(q);
	*p = ch;
	/* Save onto the rest of the string on seeing EQUEL */
	if (is_unit && ((result & PvalINHQUEL) == PvalINHQUEL))
	    break;
    }
    if (out_cur->o_charcnt)
	gen__obj( TRUE, ERx(" ") );	/* spit out a space before */

  /* [INHERIT('equel')] generates [INHERIT('II_FILES:eqenv.pen')] instead */
    if (is_unit && ((result & PvalINHQUEL) == PvalINHQUEL))
    {
	/*
	** Allow inclusion of other environments AFTER "EQUEL" - Bug 10249
	** p should be pointing at the rest of the inherit name, at least
	** the last part: "')]"
	*/
	gen_declare( p );
	result = PvalINHQUEL;
    } else
    {
	gen__obj( TRUE, pas_ptr );
	result &= ~PvalINHQUEL;
    }
    str_free( NULL, pas_ptr );
    pas_ptr = NULL;
    return result;
}

/*
+* Procedure:	pas_declare
** Purpose:	Declare PASCAL variables.
** Parameters:
**	count		- i4	- How many to declare.
**	type_bits	- i4	- Flag bits (eg, syFisCONST).
**	the_type	- SYM *	- The type pointer.
**	the_indir	- i4	- Indirections on this type.
**	rec_level	- i4	- Record nesting level.
**	blk_level	- i4	- Scope block level.
**	the_dims	- i4	- Array dimensions.
**	attrib_flags	- i4	- Attribute flags.
** Return Values:
-*	None.
** Notes:
**	The names (string addresses) have been pushed on an internal stack
**	with pas_push.  Pop the designated number of names and declare them
**	with pas_set_sym (which does all of the work).
*/

i4
pas_declare( count, type_bits, the_type, the_indir, rec_level, blk_level,
    the_dims, attrib_flags )
i4	count;
i4	type_bits;
SYM	*the_type;
i4	the_indir;
i4	rec_level;
i4	blk_level;
i4	the_dims;
i4	attrib_flags;
{
    register i4	i;
    register SYM	*sy;
    char		**names;
    i4			sy_btype;

    sy_btype = (the_type ? sym_g_btype(the_type) : T_UNDEF);
    names = pas_pop( count );

    for (i=0; i<count; i++)
    {
	if (dml_islang(DML_ESQL))
	{
	    sym_hint_type( the_type, sy_btype, the_indir );
	    /* Check for SQLSTATE or SQLCODE */
	    if(!STbcompare( names[i], 0, ERx("SQLSTATE"), 0, TRUE))
	    {
		if (sy_btype != T_CHAR)
		{
		    er_write( E_EQ0517_SQLSTATUS_DECL, EQ_ERROR, 2,
			    ERx("SQLSTATE"), ERx("PACKED ARRAY[1..5] OF CHAR;"));
		}
		else
		{
		    eq->eq_flags |= EQ_SQLSTATE;
		}
	    }
	    if (!(eq->eq_flags & EQ_NOSQLCODE)
	      && (!STbcompare( names[i], 0, ERx("SQLCODE"), 0, TRUE)))
 	    {
		if (sy_btype != T_INT)
		{
		    er_write( E_EQ0517_SQLSTATUS_DECL, EQ_ERROR, 2,
			ERx("SQLCODE"), ERx("SQLCODE: INTEGER;"));
		}
		else
		{
		    eq->eq_flags |= EQ_SQLCODE;
		}
	    }

	}
	sy = symDcEuc( names[i], rec_level, blk_level, type_bits, P_CLOSURE,
		SY_NORMAL );
	pas_set_sym( the_type, sy, the_indir, the_dims, attrib_flags );
    }
}

/*
+* Procedure:	pas_undeclared
** Purpose:	Handle undeclared names on the RHS of a declaration.
** Parameters:
**	name	- char *- The undeclared name.
**	lflags	- i4	- The type bits with which to declare the LHS.
**	rflags	- i4	- The type bits with which to declare the RHS.
**	count	- i4	- How many names on the LHS there are.
**	the_type- SYM *	- The type to declare the RHS with.
**	rec_lev - i4	- The record nesting level.
**	blk_lev - i4	- The scope block level.
**	undefsy - SYM *	- The "undefined" entry.
** Return Values:
-*	SYM *	- The entry for "name".
** Notes:
**	Declare the RHS name with type "the_type", complain about it,
**	then declare the LHS with the type of the RHS.  The names of the LHS
**	must have already been pushed on the name stack.
*/

SYM *
pas_undeclared(name, lflags, rflags, count, the_type, rec_lev, blk_lev, undefsy)
char	*name;
i4	lflags;
i4	rflags;
i4	count;
SYM	*the_type;
i4	rec_lev;
i4	blk_lev;
SYM	*undefsy;
{
    register SYM
		*sy;
    char	*type_name;

    if (lflags & syFisCONST)
	type_name = ERx("CONST");
    else
	type_name = ERx("TYPE");
    er_write( E_E2001A_hpUNDEF, EQ_ERROR, 2, type_name, name );
    sy = symDcEuc( name, rec_lev, blk_lev, rflags, P_CLOSURE, SY_NORMAL );
    if (sy)
    {
	sym_s_btype( sy, sym_g_btype(the_type) );
	sym_s_dsize( sy, sym_g_dsize(the_type) );
    } else
    {
	sy = undefsy;
    }

  /* now declare the left-hand name */
    pas_declare( count, lflags, sy );

    return sy;
}

/*
+* Procedure:	pas_isdecl
** Purpose:	Ensure that a name is declared (declare it if not).
** Parameters:
**	name	 - char * - The name to check.
**	sy_flags - i4	  - Declaration bits.
**	dfltsy   - SYM *  - Type to give it if undeclared.
**	rec_lev  - i4	  - Record nesting level if undeclared.
**	blk_lev  - i4	  - Scope block level if undeclared.
**	undefsy  - SYM *  - 
**	the_type - i4 *  - Pointer to gr->gr_type.
**	the_size - i4 *  - Pointer to gr->P_size.
** Return Values:
-*	SYM *	- A pointer to the entry for the given name.
** Notes:
**	If it's not declared, declare it.
**	Set the type and size, and return the symtab pointer.
*/

SYM *
pas_isdecl( name, sy_flags, dfltsy, rec_lev, blk_lev, undefsy, the_type,
		the_size )
char	*name;
i4	sy_flags;
SYM	*dfltsy;
i4	rec_lev;
i4	blk_lev;
SYM	*undefsy;
i4	*the_type;
i4	*the_size;
{
    register SYM	*sy;

    sy = sym_resolve( (SYM *)0, name, P_CLOSURE, sy_flags );
    if (!sy)
	sy = pas_undeclared( name, sy_flags, sy_flags, 0, dfltsy, rec_lev,
				blk_lev, undefsy );
    *the_type = sym_g_btype(sy);
    *the_size = sym_g_dsize(sy);
    return sy;
}
