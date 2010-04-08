# include <compat.h>
# include <er.h>
# include <si.h>
# include <st.h>
# include <equel.h>
# define EQ_EUC_LANG	/* for eqsym.h */
# include <eqsym.h>
# include <eqrun.h>
# include <eqgen.h>
# include <ereq.h>
# include <ere6.h>
# include <eqada.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	adautil.c
** Purpose:	ADA utilities for EQUEL/ADA declarations.
**
** Defines:
**	ada_vadjust()			- Adjust an entry's value
**	ada_prtval()			- Print a entry's value
**	ada_init()			- Do any ADA-specific init
**	ada_declare()			- Set stacked syms type to "sy"
**	ada_push()			- Push a name
**	ada_pop()			- Pop some names
**	ada_reset()			- Clear the name stack
**	ada_sizeof()			- Size an integer
** Notes:
**
** History:
**	04-oct-1985	- Written for PASCAL (mrw)
**	11-mar-1986	- Reduced to work for ADA (ncg)
**	06-oct-1989	- Allow more enumerated literals per configuration (ncg)
**      21-apr-1999 (hanch04)
**	    Added st.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** The name string stack - 
**
** This stack used to be 256 with the assumption that enumerated types can
** be stored in a single byte.  On Vax and Vads Ada we allow more (up to 1000)
** and the code generator does not make that assumption.  Representation clauses
** are also allowed which may increase the storage required for variables of
** this type (see the code generator).
*/

# define ADA_STK_MAX	1000

static char	*ada_stack[ADA_STK_MAX] ZERO_FILL;
static char	**stack_ptr = ada_stack;
static bool	stack_err = FALSE;	/* Controls multiple errors */


/*
+* Procedure:	ada_vadjust
** Purpose:	Adjust the value field of an entry according to given values
**		entered in a previously forward decalaration.
** Parameters:
**	sy	- SYM *	- The entry to adjust.
**	vlue	- i4	- The attribute bits to set.
** Return Values:
-*	None.
** Notes:
**	Copy in the value from the type pointer's declaration
*/

i4
ada_vadjust( sy, vlue )
SYM	*sy;
i4	vlue;
{
    sym_s_vlue( sy, ((i4)sym_g_vlue(sy)|vlue) );
}


/*
+* Procedure:	ada_prtval
** Purpose:	Print a string decoding the value field of an entry.
** Parameters:
**	class	- char	- The flag bits.
** Return Values:
-*	None.
** Notes:
**	None.
*/

i4
ada_prtval( vlue )
i4	vlue;			/* PUN!!! really "nat *" */
{
    static struct x {
	i4	mask;
	char	on;
    } names[] = {
	{ AvalENUM,	'E' },
	{ AvalUNSUPP,	'U' },
	{ AvalACCESS,	'A' },
	{ AvalCHAR,	'C' },
	{ 0,		' ' }
    };
    char	buf[12];
    char	*p = &buf[2];
    register struct x
		*x;

    buf[0] = buf[1] = ' ';
    for (x=names; x->mask; x++)
    {
	if (vlue & x->mask)
	    p[x-names] = x->on;
	else
	    p[x-names] = '-';
    }
    p[x-names] = '\0';
    trPrval( buf );
}


/*
+* Procedure:	ada_init
** Purpose:	Do all ADA-specific initialization.
** Parameters:
**	None.
** Return Values:
-*	None.
*/

ada_init()
{
    sym_adjval = ada_vadjust;		/* set value adjusters and printers */
    sym_prtval = ada_prtval;
    ada_reset(); 			/* reset name stack */
    gen_do_indent( G_IND_RESET ); 	/* start indenting at left margin */
}

/*
+* Procedure:	ada_declare
** Purpose:	Decalre the entries on top of the name stack, with the passed
**		attributes.
** Parameters:
**	count	- i4	- Top number of entries to declare.
**	reclev	- i4	- Record level at which to declare.
**	block	- i4	- Block level at which to declare.
**	typbits - i4	- Flag bits (syFisCONST, syFisTYPE, syFisFORWARD).
**	dims	- i4	- The "extra" dimensions of the declaration.
**	flags	- i4	- Any SYM value flags.
**	typesy	- SYM *	- The type to which to set the stacked entries.
** Return Values:
-*	SYM * - First entry entered into symbol table.
** Notes:
**      The names (string addresses) have been pushed on the local stack
**	with ada_push.  Pop the designated number of names and declare them
**	with the passed attributes.
**
**	For each entry popped, set the "btype", "dims", and "dsize" fields to
**	those of the passed "type" pointer, but add in the "extra" indirection
**	that appeared in this declaration.
**
**	The idea is that in a declaration such as
**	    a, b, c, d : integer;
**	we'll push "a", "b", "c", and "d" and stash their string names.
**	When we resolve "integer" we'll pop these entries and declare them
**	to point to the correct type. We don't have to worry about crossing
**	scope blocks because the stack will be empty at block exit.
**
** Imports modified:
**	The stacked entries.
*/

SYM *
ada_declare( count, reclev, block, typbits, dims, flags, typesy )
i4	count;
i4	reclev;
i4	block;
i4	typbits;
i4	dims;
i4	flags;
SYM	*typesy;
{
    register i4	i;
    register SYM	*sy;
    SYM			*retsy;		/* Return value */
    char		**names;
    char		**ada_pop();
    i4			sy_dsize, sy_dims, sy_btype, sy_flags;

    /* Keep from recomputing the target values */
    sy_flags = flags;
    if (typesy)
    {
	sy_dsize = sym_g_dsize( typesy );
	sy_btype = sym_g_btype( typesy );
	sy_flags |= (i4) sym_g_vlue(typesy);
	if (sy_btype == T_FORWARD)
	    sy_dims = dims;	/* later added with type's dims in symExtType */
	else
	    sy_dims = dims + sym_g_dims( typesy );
    }
    else
    {
	sy_dims =  dims;
	sy_dsize = 0;
	sy_btype = T_UNDEF;
    }

    retsy = (SYM *)0;			/* default return value */
    names = ada_pop( &count );		/* count may be changed on error */
    for (i = 0; i < count; i++)
    {
	if (dml_islang(DML_ESQL))
	    sym_hint_type( typesy, sy_btype, 0 /* indir */ );
	sy = symDcEuc( names[i], reclev, block, typbits, A_CLOSURE, SY_NORMAL );
	if (sy)
	{
	    /* Check for SQLSTATE or SQLCODE */
	    if(!STcompare( names[i], ERx("SQLSTATE")) && (dml_islang(DML_ESQL)))
	    {
		if (sy_btype != T_CHAR)
		{
		    er_write( E_EQ0517_SQLSTATUS_DECL, EQ_ERROR, 2,
			    ERx("SQLSTATE"), ERx("SQLSTATE: String(1..5);"));
		}
		else
		    eq->eq_flags |= EQ_SQLSTATE;
	    }
	    if((dml_islang(DML_ESQL))
	      && !(eq->eq_flags & EQ_NOSQLCODE)
	      && !(STcompare( names[i], ERx("SQLCODE"))))
 	    {
		if ((sy_btype != T_INT) || (sy_dsize != 4))
		{
		    er_write( E_EQ0517_SQLSTATUS_DECL, EQ_ERROR, 2,
			ERx("SQLCODE"), ERx("SQLCODE: Integer;"));
		}
		else
		    eq->eq_flags |= EQ_SQLCODE;
	    }
	    sym_s_btype( sy, sy_btype );
	    sym_s_dims( sy, sy_dims );
	    sym_s_dsize( sy, sy_dsize );
	    sym_s_vlue( sy, sy_flags );
	    if (typesy)
		sym_s_type( sy, typesy );
	    if (retsy == (SYM *)0)
		retsy = sy;
	}
    }
    return retsy;		/* caller may use this */
}

/*
+* Procedure:	ada_push
** Purpose:	Save a string for later recall.
** Parameters:
**	val	- char *	- The value to save
** Return Values:
-*	None.
** Notes:
**	Just implement a stack.
*/

ada_push( val )
char	*val;
{
    if (stack_ptr >= &ada_stack[ADA_STK_MAX-1])
    {
	if (!stack_err)
	{
	    er_write( E_E60007_haNMOVERFLOW, EQ_ERROR, 2, val,
							er_na(ADA_STK_MAX) );
	    stack_err = TRUE;
	}
    }
    else
	*stack_ptr++ = val;
}

/*
+* Procedure:	ada_pop
** Purpose:	Recall top few saved strings.
** Parameters:
**	count - i4  * - Number to recall. May be changed by callee if an
**			error occurred.
** Return Values:
-*	char ** - The address of the first of the saved values.
** Notes:
**	The other half of a stack.
*/

char **
ada_pop( count )
i4	*count;
{
    if (stack_ptr - ada_stack < *count)
    {
	*count = stack_ptr - ada_stack;
	stack_ptr = ada_stack;
	if (!stack_err)		/* May have been caused by overflow */
	{
	    er_write( E_E60008_haNMUNDERFLOW, EQ_ERROR, 0 );
	    stack_err = TRUE;
	}
    }
    else
	stack_ptr -= *count;
    return stack_ptr;
}


/*
+* Procedure:	ada_reset
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
ada_reset()
{
    stack_ptr = ada_stack;
    stack_err = FALSE;
}

/*
+* Procedure:	ada_sizeof
** Purpose:	Compute the number of bytes required to hold the given number.
** Parameters:
**	num	- i4	- The value to test.
** Return Values:
-*	i4	- The size in bytes of the int need to hold "num".
** Notes:
**	Assumes signed 2-complement arithmetic and 8-bit bytes,
**	2-byte words, and 4-byte longwords.
*/

i4
ada_sizeof( num )
i4	num;
{
    typedef struct {
	i4	tbl_max;
	i4	tbl_size;	/* in bytes */
    } TBL;
    static TBL table[] = {
	{127,	1},
	{32767,	2},
	{0,	4}
    };
    TBL		*tbl_p = table;

    if (num < 0)
	num = -num -1;

    for (tbl_p=table; tbl_p->tbl_max; tbl_p++)
    {
	if (num <= tbl_p->tbl_max)
	    return tbl_p->tbl_size;
    }
    return tbl_p->tbl_size;
}
