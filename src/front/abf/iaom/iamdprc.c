/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include	"iamstd.h"
#include	<ade.h>
#include	<fdesc.h>
#include	<ifid.h>
#include	<iltypes.h>
#include	<frmalign.h>
#include	<ifrmobj.h>
#include	"iamtbl.h"
#include	"iamfrm.h"
#include	"iamtok.h"

/**
** Name:	iamdprc.c -	Interpreted Frame Object Decoding Procedures.
**
** Description:
**	decoding procedures pointed to by Dec_tab
**
** The generic call interface for a decoding procedure:
**
**	<function>(addr_ref,n)
**
**	the caller passes in addr through addr_ref, the current spot for
**	decoding.
**
**	<function> passes back one of:
**		0 - done
**		1 - end of current buffer (another block is needed from
**			the database)
**		-1 - syntax error.
**
**	these functions in turn call the tok_ routines which get tokens from
**	the current buffer.
**
**	if 1 is returned, the decode routine also passes back the new address
**	through addr_ref, which is the spot in the array to resume decoding
**	once another block is read from the database.
**
**	The n argument is a count passed in by the caller.  = 0 the first
**	time, and is bumped for every recall following a "1" return.  used
**	to initialize state information when arrays of structures are being
**	decoded.
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Expand the maximum permissible size of allocated objects
**		(in sym_alloc).
**
**	Revision 6.3/03
**	11/08/90 (emerson)
**		Correct bug in sym_dec (bug 34385).
**
**	Revision 6.3  90/03  wong
**	Modified 'ind_dec()' and 'sym_dec()' to get 'state' by reference.  Also,
**	moved calls of 'tok_next()' to common location at beginning of all
**	decode loops.  Also, reference address for decoded object through
**	address reference alone rather than having it passed in.
**
**	Revision 6.1  88/08  wong
**	Modified to use 'FEreqmem()'.
**
**	Revision 6.0  86/08  bobm
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	oo_gdec - oo generic decode
**
** Description:
**	Array decoding routine for arrays which may be directly decoded
**	from the oo string.  Simply obtains tokens, puts the data at the
**	address, and bumps address appropriately.  Address pointers here
**	are handled as i2 pointers.  i2's are the smallest things decoded,
**	and i2 size is assumed to divide the others.
**
** Inputs:
**	addr_ref	{i2 **}  Reference to address into which to decode.
**
** Outputs:
**	addr_ref	{i2 **}  Address at which to resume decoding when end
**					of string encountered.
**
** Returns:
**		0	done
**		1	end of string
**		-1	syntax error
**
** History:
**	8/86 (rlm)	written
*/

i4
oo_gdec ( addr_ref )
i2	**addr_ref;
{
	register i2	*addr = *addr_ref;
	TOKEN		tok;

	for (;;)
	{
		_VOID_ tok_next(&tok);
		switch (tok.type)
		{
		case TOK_i2:
			*addr = tok.dat.i2val;
			++addr;
			break;
		case TOK_i4:
			*((i4 *) addr) = tok.dat.i4val;
			addr += sizeof(i4)/sizeof(i2);
			break;
		case TOK_f8:
			*((f8 *) addr) = tok.dat.f8val;
			addr += sizeof(f8)/sizeof(i2);
			break;
		case TOK_str:
			*((char **) addr) = tok.dat.str;
			addr += sizeof(char **)/sizeof(i2);
			break;
		case TOK_eos:
			*addr_ref = addr;
			return(1);
		case TOK_end:
			return(0);
		default:
			return(-1);
		} /* end token switch */
	} /* end for ever */
	/*NOTREACHED*/
}

/*
** ind_dec decodes the stack indirection section
*/
i4
ind_dec ( addr_ref, num, state )
IL_DB_VDESC		**addr_ref;
i4			num;
register i4		*state;	/* which state is being decoded. */
{
	register IL_DB_VDESC	*addr = *addr_ref;
	TOKEN			tok;
	i4	val;

	/* if first call, initialize state */
	if (num != 0)
		*state = 0;

	for (;;)
	{
		_VOID_ tok_next(&tok);
		/*
		** for IL_DB_VDESC, encoding string is entirely integers.
		** we convert both integer types to i4 up front, and make
		** this routine actually insensitive to which type of
		** integer we got.
		*/
		switch (tok.type)
		{
		case TOK_eos:
			*addr_ref = addr;
			return(1);
		case TOK_end:
			if (*state != 0)
				return(-1);
			return(0);
		case TOK_i2:
			val = tok.dat.i2val;
			break;
		case TOK_i4:
			val = tok.dat.i4val;
			break;
		default:
			return (-1);
		} /* end token switch */

		switch (*state)
		{
		case 0:
			addr->db_datatype = val;
			break;
		case 1:
			addr->db_length = val;
			break;
		case 2:
			addr->db_prec = val;
			break;
		case 3:
			addr->db_offset = val;
			++addr;
			break;
		} /* end state switch */

		*state = (*state+1)%4;
	} /* end for ever */
	/*NOTREACHED*/
}

/*
** sym_dec decodes the symbol table
**
** History:
**	11/08/90 (emerson)
**		Correct the logic that checks for old (6.3/02) fdesc format.
*/
#define MAXOLDSTATE	4
#define MAXNEWSTATE	6

i4
sym_dec ( addr_ref, num, state )
FDESCV2			**addr_ref;
i4			num;
register i4		*state;	/* which state is being decoded. */
{
	register FDESCV2	*addr = *addr_ref;
	TOKEN			tok;

	/* if first call, initialize state */
	if (num != 0)
		*state = 0;

	for (;;)
	{
		_VOID_ tok_next(&tok);
		switch (tok.type)
		{
		case TOK_eos:
			*addr_ref = addr;
			return(1);
		case TOK_end:
			if ( *state != 0 && *state != (MAXOLDSTATE + 1) )
				return(-1);
			if ( *state == (MAXOLDSTATE + 1) )
				++addr;
			*state = 0;
			return(0);
		default:
			break;
		} /* end token switch */

		switch (*state)
		{
		case 0: /* see also case 5 */
			if (tok.type != TOK_str)
				return(-1);
			addr->fdv2_name = tok.dat.str;
			break;
		case 1:
			if (tok.type != TOK_str)
				return(-1);
			addr->fdv2_tblname = tok.dat.str;
			break;
		case 2:
			if (tok.type != TOK_i4)
				return(-1);
			addr->fdv2_dbdvoff = tok.dat.i4val;
			break;
		case 3:
			if (tok.type != TOK_i2)
				return(-1);
			addr->fdv2_visible = tok.dat.i2val;
			break;
		case 4:
			if (tok.type != TOK_i2)
				return(-1);
			addr->fdv2_order = tok.dat.i2val;
			addr->fdv2_flags = 0;
			addr->fdv2_typename = NULL;
			break;

		/*
		** Additional information for runtime symbol table support.
		**	This stuff is optional for backward compatibility.
		*/

		case 5:
			if (tok.type == TOK_str)
			{
				/* 
				**  If we've reached a TOK_str, we're
				**  presumably looking at an old (6.3/02)
				**  fdesc, which is missing the last 2 members
				**  (corresponding to cases 5 and 6)
				**  so we'll start a new fdesc (++addr),
				**  go into state 0 and do what's appropriate
				**  for state 0.
				*/
				*state = 0;
				++addr;
				addr->fdv2_name = tok.dat.str;
				break;
			}
			if (tok.type != TOK_i2)
				return(-1);
			addr->fdv2_flags = tok.dat.i2val;
			break;
		case 6:
			if (tok.type != TOK_str)
				return(-1);
			addr->fdv2_typename = tok.dat.str;
			addr++;
			break;
		}

		*state = (*state + 1) % (MAXNEWSTATE + 1);

	} /* end for ever */
	/*NOTREACHED*/
}

/*
** hex_dec decodes an array of hexadecimal string constants
**
** History:
**	03/90 (jhw) -- Check for syntax error return from 'tok_next()'.
*/
i4
hex_dec ( hex_ref )
DB_TEXT_STRING		***hex_ref;
{
	register DB_TEXT_STRING	**hex = *hex_ref;
	TOKEN			tok;

	for (;;)
	{
		if ( tok_next(&tok) != OK )
			return -1;	/* error */
		switch(tok.type)
		{
		case TOK_xs:
			*hex++ = tok.dat.hexstr;
			break;	/* continue */
		case TOK_eos:
			*hex_ref = hex;
			return 1;
		case TOK_end:
			return 0;
		default:
			return -1;	/* error */
		} /* end token switch */
	} /* end for ever */
	/*NOTREACHED*/
}

/*{
** Name:	sym_alloc - allocate symbol table.
**
** Description:
**	Allocation routine for the symbol table.  We cannot use the same
**	routine that others use because we need to allocate an extra slot for
**	a NULL item to terminate the end of the list.  We also allocate zeroed
**	memory to assure that the list is NULL terminated.
**
** Inputs:
**	tag	{i2}  memory tag
**	nbytes	{nat}  number of bytes.
**
** Returns:
**	{PTR}	Address of allocated memory,
**		NULL on error.
**
** History:
**	12/86 (rlm)	written
**	08/88 (jhw)  Modified to use 'FEreqmem()'.
**	01/09/91 (emerson)
**		Expand the maximum permissible size of allocated objects.
**		This entails changing the call to allocation procedures
**		(e.g. sym_alloc); it now expects the size to be a u_i4.
*/

PTR
sym_alloc ( tag, nbytes )
u_i4		tag;
u_i4	nbytes;
{
	return FEreqlng( tag, nbytes + sizeof(FDESCV2), TRUE, (STATUS *)NULL );
}
