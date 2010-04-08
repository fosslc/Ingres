# include 	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<si.h>
# include	<er.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<ereq.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	ARGMAN.C
** Purpose:	Maintains the routines for managing arguments for II functions.
**
** Defines:					- Argument adding calls.
**		arg_var_add( sym, name )	-   Variable argument.
**		arg_str_add( type, string )	-   String argument.
**		arg_int_add( integer )		-   Integer constant argument.
**		arg_float_add( real )		-   Floating constant argument.
**		arg_push()			-   Start an argument subset.
**						- Argument fetching calls.
**		arg_count()			-   How many were added.
**		arg_next()			-   Next argument to fetch.
**		arg_name( index )		-   Name of argument (string).
**		arg_num( index, type, ptr )	-   Numeric value of argument.
**		arg_sym( index )		-   Symbol table entry.
**		arg_type( index )		-   Type of argument.
**		arg_rep( index )		-   Representation of data.
**		arg_free()			- Done using arguments.
**		arg_dump( calname )		- Dump contents for debugging.
**		args_toend (first, last )	- Move arguments to end of list
** Locals:
**		arg_add( sym, type, tag, data )	- Real argument adder.
-*		arg_index( index )		- Index to argument mapper.
**		
** This set of routines is used by higher level functions to add arguments
** for setting up II calls, and by lower level routines that actually format
** arguments inside those.  The higher level routines only know that they must 
** add the argument, for a later call.  The lower level routines know what the
** arguments are supposed to look like in particular calls and all the 
** information is made available for verification. A typical call requires 3 
** arguments of 3 different types.  The lower level function formatters, 
** that actually dump the arguments, will request each argument and print it
** to the output file in its particular format.
**
** Example: Assume IIcall() needs two arguments, a string constant, and an 
**	    integer constant.  The higher level routines may store the two 
** arguments using the calls
**
**	arg_str_add( ARG_CHAR, "arg1" );
**	arg_int_add( 123 );
**
** The lower level routine dumping the arguments may use the calls
**
**	if (arg_count() == 2)
**	{
**	   arg_num( 2, ARG_INT, &intnum );
**	   gen( "IIcall( \"%s\", %d );", arg_name(1), intnum );
**	}
**
** which will generate
**
**      IIcall( "arg1", 123 );
**
** If the routine arg_push() is called then the topmost arguments added after
** the call will be visible till arg_free().  Once they are freed, the original
** arguments are revealed.
**
** History:
**		30-oct-1984 	- Written (ncg)
**		20-nov-1992 (Mike S)
**			Added args_to_end to allow re-ordering arguments
**              04-mar-1993 (smc)
**                      Cast parameter of MEfree to match prototype.
**                      Changed ARG_MAX to ARGMAN_MAX as ARG_MAX is defined
**                      in <limits.h> as a system limit on several SVR4 boxes
**			and in Alpha AXP OSF.
**	11-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	13-mar-1996 (thoda04)
**		Added function parameter prototypes for local static functions.
**                      
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* 
** Union of data to actually represent the content of the argument as the
** preprocesser stores it.
*/

typedef union {
    i4		a_nat;			/* An internal integer */
    f8		a_f8;			/*    or internal floating point */
    char	*a_str;			/* Any type of string (even integer) */
} ARG_DATA;

/* Single entry in the argument manager table */

typedef struct {
    SYM		*a_sym;			/* Symbol table entry if variable */
    i4		a_type;			/* Argument type from 'storer' */
    i4		a_tag;			/* Tag for a_data */
    ARG_DATA	a_data;			/* Real data of argument */
} ARG_ELM;

# define	ARGNULL		(ARG_ELM *)0

/*
** Argument manager uses a local array of arguments stored in order of
** addition, waiting to be used by a function call.
*/

# define	ARGMAN_MAX		15

static	    ARG_ELM	arg_arr[ ARGMAN_MAX ] ZERO_FILL;

/* Used to manage inputting ('storing') of arguments */

static  i4	arg_icnt = 0;		/* Number args inputted */
static  i4	arg_ierr = FALSE;	/* Error when inputting */
static	i4	arg_isub = 0;		/* Argument subset start number */

static	void	arg_add(SYM *asym, i4  atype, i4  atag, ARG_DATA *adata);
			                /* Adding routine to argument array */

/* Used to manage outputting ('calling' not 'storing') of arguments */

static	i4	arg_ocnt = 0;		/* Counter for outputting */
static  i4	arg_oerr = FALSE;	/* Error when outputting */

static	ARG_ELM *arg_index(i4 a);	/* Caller's index into argument array */

/* 
** Local constants to be used internally for tagging argument data.
** Note that these do not neccessarily correspond to argument types, as 
** user numerics (with type ARG_INT) may be represented as strings, and 
** thus have tag ARG__STR.
*/

# define	AR__DEF		ARG_NOTYPE
# define	AR__I4		ARG_INT
# define	AR__F8		ARG_FLOAT
# define	AR__STR		ARG_CHAR


/*
+* Procedure:	arg_var_add 
** Purpose:	Add a variable to the argument array.  
**
** Parameters: 	asym  - SYM *	- Symbol table entry.
** Parameters: 	aname - char *	- Fully qualified name.
-* Returns: 	None
*/

void
arg_var_add( asym, aname )
SYM	*asym;			/* Symbol table entry */
char	*aname;			/* Fully qualified name of entry */
{
    ARG_DATA	ad;

    if (aname == (char *)0)
	ad.a_str = ERx("IIargerr");
    else
	ad.a_str = aname;
    arg_add( asym, ARG_NOTYPE, AR__STR, &ad );
}

/*
+* Procedure:	arg_str_add 
** Purpose:	Add a string to the argument array.  
**
** Parameters: 	atype - i4	- Data type representation.
**		adata - char *	- String data value.
-* Returns: 	None
**
** Note that the string may be a real string constant, or the character 
** representation of a numeric. The incoming type may be ARG_INT, ARG_FLOAT,
** ARG_CHAR or ARG_RAW.
*/

void
arg_str_add( atype, adata )
i4	atype;
char	*adata;
{
    ARG_DATA	ad;

    ad.a_str = adata;
    arg_add( (SYM *)NULL, atype, AR__STR, &ad );
}

/*
+* Procedure:	arg_int_add 
** Purpose:	Add an integer constant to the argument array.
**
** Parameters: 	adata - i4  - An integer to be added.
-* Returns: 	None
**
** Note that the constant will probably an internally generated constant, as
** user constants will be passed through arg_str_add().
*/

void
arg_int_add( adata )
i4	adata;
{
    ARG_DATA	ad;

    ad.a_nat = adata;
    arg_add( (SYM *)NULL, ARG_INT, AR__I4, &ad );
}


/*
+* Procedure:	arg_float_add 
** Purpose:	Add a floating constant to the argument array.
**
** Parameters: 	adata - f8 - A floating constant.
-* Returns: 	None
**
** Note that the constant will probably an internally generated constant, as
** user constants will be passed through arg_str_add().
*/

void
arg_float_add( adata )
f8	adata;
{
    ARG_DATA	ad;

    ad.a_f8 = adata;
    arg_add( (SYM *)NULL, ARG_FLOAT, AR__F8, &ad );
}

/*
+* Procedure:	arg_add 
** Purpose:	Final routine to update the argument array with new data.
**
** Parameters: 	asym  - SYM *	   - Symbol table entry if variable,
**		atype - i4	   - Storer's argument type,
**		atag  - i4	   - Data type representation,
**		adata - ARG_DATA * - Real data (union of different types).
-* Returns: 	None
*/

static void
arg_add( asym, atype, atag, adata )
SYM		*asym;
i4		atype;
i4		atag;
ARG_DATA	*adata;
{
    register ARG_ELM	*arg;

    if (arg_icnt >= ARGMAN_MAX)
    {
	if (!arg_ierr)
	{
	    er_write( E_EQ0316_argMAX, EQ_ERROR, 1, er_na(ARGMAN_MAX) );
	    arg_ierr = TRUE;
	}
    }
    else
    {
	arg = &arg_arr[ arg_icnt++ ];
	arg->a_sym = asym;
	arg->a_type = atype;
	arg->a_tag = atag;
	/* Copy the union as one chunk */
	MEcopy( (char *)adata, (u_i2)sizeof(ARG_DATA), (char *)&arg->a_data );
    }
}

/*
+* Procedure:	args_to_end
** Purpose:	Move arguments which were already pushed to end of argument
**		array.
** 
** Parameters:	start - i4	First argument to move
**		end -   i4	Last argument to move.
-* Returns:	None
** 
** 'start' and 'end' are 1-based, as is arg_icnt.
**
** This just makes a copy of the argument array with different argument
** ordering, and copies it back.  If anything looks funny, we 
** generate a fatal error.
*/

void
args_toend(start, end)
i4  start;
i4  end;
{
    ARG_ELM *temparr;
    i4  argind;
    i4  tmpind;

    if (start <= 0 || start > end || end > arg_icnt)
	er_write(E_EQ051B_ARGS_TOEND_ERROR, EQ_FATAL, 0);

    if (end == arg_icnt)
	return;		/* Nothing to do */

    /* Make a copy of the argument array */
    temparr = (ARG_ELM *)MEreqmem(0, sizeof(ARG_ELM) * arg_icnt, 
				  FALSE, (STATUS *)0);

    /* Copy the args not to be moved to the end to the temporary array */
    for (argind = 0, tmpind = 0; argind < arg_icnt; argind++)
    {
	if (argind < start - 1 || argind > end - 1)
	{
	    MEcopy((PTR)&arg_arr[argind], (u_i2)sizeof(ARG_ELM), 
		   (PTR)&temparr[tmpind]);

	    tmpind++;
	}
    }

    /* Copy the args to be moved to the end to the temporary array */
    MEcopy((PTR)&arg_arr[start - 1], 
	   (u_i2)((end - start + 1) * sizeof(ARG_ELM)),
	   (PTR)&temparr[tmpind]);

    /* Copy the whole thing back */
    MEcopy((PTR)temparr, (u_i2)(arg_icnt * sizeof(ARG_ELM)), (PTR)arg_arr);

    MEfree((PTR)temparr);
}

/*
+* Procedure:	arg_push 
** Purpose:	Start an argument subset.  
** 
** Parameters:	None
-* Returns:	None
**
** This just marks the top of the argument array, till the next arg_free().  
** All arguments referenced will start at arg_isub.  This really should be 
** managed as a stack.
*/

void
arg_push()
{
    arg_isub = arg_icnt;		/* Save original input counter */
}

/*
** The following functions are called by the users of the arguments -
** the routines that generate the calls to the runtime functions.
*/

/*
+* Procedure:	arg_count 
** Purpose:	How many arguments are there ?
**
** Parameters: 	None
-* Returns: 	i4 - The number of stored arguments.
*/

i4
arg_count()
{
    return arg_icnt - arg_isub;
}

/*
+* Procedure:	arg_next 
** Purpose:	Get the next argument probably for a varying number of args.
**
** Parameters: 	None
** Returns: 	i4 - The index (beginning at 1) of the next argument, or 0
-*		if reaching the last argument.
**
** Typical usage is:
**	while (i = arg_next())
**		use( arg_name(i) );
**
** This routine does not return the C index, as it would would have to 
** terminate with another condition.  Instead callers will use indices >= 1.
*/

i4
arg_next()
{
    return ++arg_ocnt > (arg_icnt-arg_isub) ? arg_isub : arg_ocnt+arg_isub;
}

/*
+* Procedure:	arg_name 
** Purpose:	Get the name out of the argument content of the indexed 
**	      	argument.  The caller probably knows that there is a name 
** 		there and which number argument.
**
** Parameters: 	i4 	- Argument index (beginning at 1).
-* Returns: 	char *	- Name stored in that argument.
**
** If the corresponding argument does not have a name, return dummy one.
*/

char	*
arg_name( a )
i4	a;
{
    register ARG_ELM *arg;

    arg = arg_index( a );
    if (arg->a_tag != AR__STR)
	return ERx("IIargerr");
    else
	return arg->a_data.a_str;	/* Possibly null */
}


/*
+* Procedure:	arg_num 
** Purpose:	Get numeric data out of the argument content of the indexed 
**	     	argument.  The caller knows there is a number.
**
** Parameters: 	a	- i4	- Argument index (beginning at 1),
**		atype	- i4	- Type requested (integer of floating),
**		numptr	- i4  *	- Address of numeric result.
-* Returns: 	None
**
** If the corresponding argument does not have number do nothing.
*/

void
arg_num( a, atype, numptr )
i4	a;
i4	atype;
i4	*numptr;
{
    register ARG_ELM *arg;

    arg = arg_index( a );
    if (atype == ARG_INT && arg->a_tag == AR__I4)
	*numptr = arg->a_data.a_nat;
    else if (atype == ARG_FLOAT && arg->a_tag == AR__F8)
    {
	f8     *f8num;

	f8num = (f8 *)numptr;
	*f8num = arg->a_data.a_f8;
    }
}


/*
+* Procedure:	arg_sym 
** Purpose:	Return the symbol table entry of indexed argument.
**
** Parameters: 	a - i4  - Argument index (beginning at 1).
-* Returns: 	SYM *  - Symbol table entry stored in that argument.
*/

SYM	*
arg_sym( a )
i4	a;
{
    register ARG_ELM *arg;

    arg = arg_index( a );
    return arg->a_sym;
}


/*
+* Procedure:	arg_type 
** Purpose:	Return the argument type of indexed argument.
**
** Parameters: 	a - i4  - Argument index (beginning at 1).
-* Returns: 	i4     - Storer's type of that argument.
*/

i4
arg_type( a )
i4	a;
{
    register ARG_ELM *arg;

    arg = arg_index( a );
    return arg->a_type;
}


/*
+* Procedure:	arg_rep 
** Purpose:	Return the argument data representation of indexed argument.
**
** Parameters: 	a - i4  - Argument index (beginning at 1).
-* Returns: 	i4     - Data type representation of that argument.
**
** This is used mainly because numeric data may have been added as a string
** constant - in a case of "12" the type is ARG_INT but the rep is ARG_CHAR.
*/

i4
arg_rep( a )
i4	a;
{
    register ARG_ELM *arg;

    arg = arg_index( a );
    return arg->a_tag;
}


/*
+* Procedure:	arg_index 
** Purpose:	Given an index (beginning at 1) into the argument array, 
**	       	return an argument pointer.
**
** Parameters: 	a - i4	  - Argument index (beginning at 1).
-* Returns: 	ARG_ELM * - Pointer to corresponding argument.
**
** The passed index is either hard coded by the caller (knows the number of
** arguments to a particular function) or generated by arg_next() for a 
** varying number of arguments.
*/

static ARG_ELM	*
arg_index( a )
i4	a;
{
    if (a > arg_icnt || a <= 0)	/* Index in [ 1..arg_icnt ] */
    {
	arg_oerr = TRUE;
	a = 1;				/* Return first argument */
    }
    return &arg_arr[ a - 1 + arg_isub ];
}


/*
+* Procedure:	arg_free 
** Purpose:	Free counters that manage static array.
**
** Parameters: 	None
-* Returns: 	None
*/

void
arg_free()
{
    if (arg_oerr)
	er_write( E_EQ0317_argNOARGS, EQ_ERROR, 1, er_na(arg_icnt) );
    arg_icnt = arg_isub;		/* If subset then restore in_count */
    arg_isub = 0;
    arg_ierr = FALSE;
    arg_ocnt = 0;
    arg_oerr = FALSE;
}

/*
+* Procedure:	arg_dump 
** Purpose:	Dump all information about current arguments stored.
**
** Parameters: 	calname - char * - Caller's name.
-* Returns: 	None
*/

void
arg_dump( calname )
char	*calname;
{
    register	ARG_ELM	*arg;
    register	i4	i;
    register	FILE	*df = eq->eq_dumpfile;

    if (calname == (char *)NULL)
	calname = ERx("Arg_array");
    SIfprintf( df, ERx("ARG_DUMP: %s\n"), calname );
    SIfprintf( df, ERx("  arg_icnt = %d, arg_ierr = %d, arg_isub = %d, "), 
	       arg_icnt, arg_ierr, arg_isub );
    SIfprintf( df, ERx("arg_ocnt = %d, arg_oerr = %d\n"), arg_ocnt, arg_oerr );
    for (i = 0; i < arg_icnt; i++)
    {
	arg = &arg_arr[ i ];
	SIfprintf( df, ERx("  arg[%2d]: sym = %p, typ = %d, tag = %d, data = "),
		   i, arg->a_sym, arg->a_type, arg->a_tag );
	switch (arg->a_tag)
	{
	  case AR__I4: 
	    SIfprintf( df, ERx("(i) %d"), arg->a_data.a_nat );
	    break;

	  case AR__F8: 
	    SIfprintf( df, ERx("(f) %f"), arg->a_data.a_f8, '.' );
	    break;

	  case AR__STR: 
	    SIfprintf( df, ERx("(s) %s"), arg->a_data.a_str ? 
		       arg->a_data.a_str : ERx("<null>") );
	    break;
	}
	if  ( i < arg_icnt -1 )
	    SIfprintf( df, ERx(" ,") );
	SIfprintf( df, ERx("\n") );
    }
    SIflush( df );
}
