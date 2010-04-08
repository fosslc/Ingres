/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>
#include    <er.h>
#include    <me.h>
#include    <si.h>
#include    <generr.h>
#include    "erls.h"
#include    <st.h>

#if defined(UNIX) || defined(NT_GENERIC)

/**
**  Name: iimfdata.c - Provide interface for MF COBOL data processing.
**
**  Description:
**	The routines in this file support the required numeric and string 
**	data processing for the MF COBOL processor.  These are required for
**	both LIBQ and the FRS.
**
**	See iimflibq.c for more details and global perspective.
**
**	Note that these interfaces do not follow the model of the coding
**	standards but rather make them simpler to call from a user COBOL
**	program (ie, all BY REFERENCE followed by BY VALUE).
**
**  Defines:
**	IIMFdebug		- Set/reset MF COBOL debug value.
**	IIMFpktof8		- Convert packed to f8.
**	IIMFf8topk		- Convert f8 to packed.
**	IIMFstrtof8		- Convert string to an f8.
**	IIMFsd			- Return dynamic C string from COBOL string.
**	SQL_F8TOPACK		- Dynamic SQL user cover to IIMFf8topk
**	SQL_PACKTOF8		- Dynamic SQL user cover to IIMFpktof8
**	SQL_ACCEPT		- Interface on top of MF ACCEPT due to bug.
**
**  Locals:
**	IIMFpack_verify		- Verify user arguments to COMP-3 conversions.
**
**  History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	01-apr-90 (neil)
**	   Byte alignment fixes.
**	25-Aug-1993 (fredv)
**		included <st.h>.
**	04-oct-95 (sarjo01)
**		Added NT_GENERIC to ifdef.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Global values for Packed-Decimal and String Processing */

/*
** Unsigned macros for packed-decimal (COMP-3) conversions:
**
** If your machine supports the "unsigned char" type make sure that you have
** the following in compat.h:
**	typedef unsigned char	u_i1;
** otherwise the following (which requires 
**	typedef char	u_i1;
**
** Most machines support "unsigned char" types.  If your machine does not then
** you must define NO_UN_I1.  Do this by issuing something like:
**	# ifdef host_xxyyzz
**	#   define NO_UN_I1
**	# endif
** This will maintain the protability of this file.
*/

/*
** The macro U_I1_MACRO returns the u_i1 that is being pointed at
*/
#ifdef NO_UN_I1
#	define U_I1_MACRO(p)	(*(u_i1 *)(p) & 0xFF)
#else
#	define U_I1_MACRO(p)	(*(u_i1 *)(p))
#endif

/* COMP-3 is really a packed-decimal string */
#define	COMP_3	u_i1

/*
** When processing to packed decimal we need to convert to a zero filled
** string.  STprintf does not support such a format, and not all machines
** support the "%0.0f" format (eg., ncr).  So we use the sprintf form
** STprintf does not understand this format:
** 	Leading spaces, no decimal point, no fraction digits, all <len> digits.
*/
# define	ADJUST_F8	ERx("%*.0f")

/* Maxmimum # of digits (characters) in a COMP-3 stream for string conversion */
#define	MAX_PK_CHAR	64

/* Maxmimum # of digits allowed in a COMP-3 variable*/
#define MAX_PK_DIGIT	18

/* Allow trace of intermediate data processing */
static	i4 trace_debug = 0;

static	i4	IIMFpack_verify();	/* Verify user arguments */


/*{
** Name: IIMFdebug 	- Trace set/reset debug value.
**
**
** Description:
**	Set the value of trace_debug.
**
** Inputs:
**      dbg 			1/0 - To trace or not to trace.
**
** Outputs:
**	trace_debug		Set/reset.
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
*/
VOID
IIMFdebug(dbg)
i4	dbg;
{
    trace_debug = dbg;
} /* IIMFdebug */

/*
** Name: IIMFpktof8 	- Convert a packed decimal (COMP-3) to an F8.
**
** Description:
**	NOTE: This routine may be replaced by internal INGRES processing of
**            of packed decimal at a future date.  This can only be done if
**      the tests confirm that (1) MF COBOL decimal format is the same as
**	the INGRES format and (2) the boundary conditions of certain value
**	results remain the same.
**	
**	This routine walks through the decimal value:
**	   Pick of the sign, look at 2 nibbles at a time converting to
**	   character,  retrofit the decimal point into the digit stream,
**	   and convert the string to an f8.
**
** Inputs:
**      dbl			Pointer to double variable to be filled.
**	pkvar			Pointer to packed decimal variable to scan.
**	precision		Length of decimal variabled.
**	scale			Scale of decimal variable.
**	sign			Is variable signed or not.
**
** Outputs:
**      dbl			Result f8 after conversion.
**
** Errors:
**	E_LS0110_PACKTOF8 - Unable to convert to f8.
**
** Example:
**		01 IIF8 PIC X(8) USAGE COMP-X.
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
**		CALL "IIMFpktof8" USING IIF8 P BY VALUE 5 3 1.
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	01-apr-90 (neil)
**	   Byte alignment fixes.
*/
VOID
IIMFpktof8(dbl, pkvar, precision, scale, sign)
f8	*dbl;
COMP_3	*pkvar;
i4	precision, scale, sign;
{
    char	buf[MAX_PK_CHAR+1];	/* For string conversion */
    u_i1	*pcp, ch, swapch;	/* For scanning string */
    u_i1	*pkdata = pkvar;	/* For scanning decimal */
    char	comp_err[30];		/* For conversion errors */
    double	d;			/* To avoid alignment problems */
    i4		i;

    if (trace_debug)
    {
	SIprintf(ERx("IIMFpktof8(0): precision = %d, scale = %d\n"),
		 precision, scale);
    }

    pcp = (u_i1 *)buf;
    if ((pkdata[precision/2] & 0xF) == 0xD)	/* Terminal & sign nibble */ 
	*pcp++ = '-';
    for (i = 0; i < precision/2 +1; i++) 	/* 2 nibbles per byte */
    {		
	*pcp++ = (U_I1_MACRO(pkdata) >> 4) + '0';  /* High nibble */
	*pcp++ = (*pkdata & 0xF) + '0';		   /* Low nibble (maybe sign) */
	pkdata++;
    }
    /* Terminate string and overwrite last sign nibble */
    *--pcp = '\0';
    if (scale) 					/* Decimal point needed? */
    {
	ch = '.';
	for (pcp -= scale; *pcp; pcp++) 	/* Shift the fraction over */
	{
	    swapch = *pcp;
	    *pcp = ch;
	    ch = swapch;
	}
	*pcp++ = ch;
	*pcp = '\0';				/* Terminate string */
    }
    d = 0.0;
    if (CVaf(buf, '.', &d) != OK)
    {
	STprintf(comp_err, ERx("%s9(%d)V9(%d)"),
		 sign ? ERx("S") : ERx(""), precision-scale, scale);
	IIlocerr(GE_DATA_EXCEPTION + GESC_FIXOVR, E_LS0110_PACKTOF8,
		 0, 2, buf, comp_err);
    }
    if (trace_debug)
	SIprintf(ERx("IIMFpktof8(1): result = %f\n"), d);
#ifdef BYTE_ALIGN
    MEcopy((PTR)&d, sizeof(d), (PTR)dbl);
#else
    *dbl = d;
#endif
} /* IIMFpktof8 */

/*
** Name: IIMFf8topk 	- Convert an F8 to a packed decimal (COMP-3).
**
** Description:
**	NOTE: This routine may be replaced by internal INGRES processing of
**            of packed decimal at a future date.  This can only be done if
**      the tests confirm that (1) MF COBOL decimal format is the same as
**	the INGRES format and (2) the boundary conditions of certain value
**	results remain the same.
**	
**	This routine walks through the float value:
**	   Determine the sign, scale to no fractions, convert to an adjusted
**	   string, walk through the string filling 2 decimal nibbles at a
**	   time, put back sign.
**
** Inputs:
**      dbl			Pointer to double variable with value.
**	pkvar			Pointer to packed decimal variable to fill.
**	precision		Length of decimal variabled.
**	scale			Scale of decimal variable.
**	sign			Is variable signed or not.
**
** Outputs:
**      pkvar			Result decimal after conversion.
**
** Errors:
**	E_LS0111_F8TOPACK - Unable to convert to pack - overflow.
**
** Example:
**		01 IIF8 PIC X(8) USAGE COMP-X.
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
**		CALL "IIMFf8topk" USING IIF8 P BY VALUE 5 3 1.
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	01-apr-90 (neil)
**	   Byte alignment fixes.
*/
VOID
IIMFf8topk(dbl, pkvar, precision, scale, sign)
f8	*dbl;
COMP_3	*pkvar;		
i4	precision, scale, sign;
{
    char	buf[MAX_PK_CHAR+1];	/* For string conversion */
    u_i1	*pcp;			/* Points at string */
    u_i1	*pkdata = pkvar;	/* For filling decimal */
    u_i1	term;			/* Decimal sign value */
    int		plen = precision;	/* Precision to adjust */
    int		pscale = scale;		/* Scale to adjust */
    f8		d;			/* Local to use */
    char	comp_err[30];		/* For conversion errors */
    int		i;

#ifdef BYTE_ALIGN
    MEcopy((PTR)dbl, sizeof(d), (PTR)&d);
#else
    d = *dbl;
#endif

    if (trace_debug)
    {
	SIprintf(ERx("IIMFf8topk: prec = %d, scale = %d, sign = %d, f8 = %f\n"),
	   	 precision, scale, sign, d);
    }

    if((plen & 1) == 0)		/* Even precision == extra byte to pad with 0 */
	plen++;

    if (d < 0) 			/* Determine sign */
    {
	d = -d;
	if (sign)
	    term = 0xD;		/* Negative terminator */
	else
	    term = 0xF;		/* User chooses to ignore sign */
    } 
    else if (sign)
    {
	term = 0xC;		/* Positive terminator */
    }
    else
    {
	term = 0xF;		/* Sign does not apply */
    }

    /* Convert f8 to zero-filled numeric string */
    while (pscale-- > 0)		/* Scale to no fractions */
	d *= 10.0;

    /* STprintf does not know about this leading space-fill format */
    sprintf(buf, ADJUST_F8, plen, d);

    /* Blanks to zeros (sprintf may have added them) and get to EOS */
    for(pcp = (u_i1 *)buf; *pcp != '\0'; ++pcp)
    {
	if (*pcp == ' ')
	    *pcp = '0';
    }

    pcp -= plen;		/* Point pcp to first digit for conversion */

    /*
    ** If we're not pointing to the start of the digit buffer, or if we have
    ** an even precision and we're not pointing to the special added zero-fill
    ** byte, then issue an error.
    */
    if (   (pcp != (u_i1 *)buf)
	|| ((precision & 1) == 0 && buf[0] != '0')
       )
    {
	STprintf(comp_err, ERx("%s9(%d)V9(%d)"),
		 sign ? ERx("S") : ERx(""), precision-scale, scale);
	IIlocerr(GE_DATA_EXCEPTION + GESC_FIXOVR, E_LS0111_F8TOPACK,
		 0, 2, buf, comp_err);
	return;
    }

    /*
    ** buf has the string version of the number.  pcp may point into the
    ** string (rather than the start), in which case we've thrown away the
    ** most significant digits (which isn't exactly according to COBOL
    ** alignment rules).
    */
    for (i = 0; i < plen/2; i++) 	/* 2 nibbles per byte */
    {
	/* Word   =  High nibble	     	   + Low nibble */
	*pkdata++ = ((U_I1_MACRO(pcp) - '0') << 4) | (U_I1_MACRO(pcp+1) - '0');
	pcp += 2;
    }
    pkvar[plen/2] = ((U_I1_MACRO(pcp) - '0') <<4 ) | term;  /* Sign nibble */
}

/*
** Name: IIMFstrtof8 	- Convert a string to an F8.
**
** Description:
**	NOTE: This routine may be replaced by internal INGRES processing of
**            of packed decimal at a future date.  This can only be done if
**      the tests confirm that (1) MF COBOL decimal format is the same as
**	the INGRES format and (2) the boundary conditions of certain value
**	results remain the same.
**	
**	This routine is just a cover on top of CVaf.
**
** Inputs:
**      fstr			Null-terminated string to convert.
**
** Outputs:
**      dbl			F8 variable to fill.
**
** Errors:
**	E_LS0112_STRTOF8 - Error in conversion.
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	01-apr-90 (neil)
**	   Byte alignment fixes.
*/
VOID
IIMFstrtof8(fstr, dbl)
char	*fstr;
f8	*dbl;
{
    double	d;			/* To avoid alignment problems */

    if (trace_debug)
	SIprintf(ERx("IIMFstrtof8: fstr = <%s>\n"), fstr);
    d = 0.0;
    if (CVaf(fstr, '.', &d) != OK)
	IIlocerr(GE_DATA_EXCEPTION + GESC_FIXOVR, E_LS0112_STRTOF8, 0, 1, fstr);
    if (trace_debug)
	SIprintf(ERx("IIMFstrtof8: result = <%f>\n"), d);
#ifdef BYTE_ALIGN
    MEcopy((PTR)&d, sizeof(d), (PTR)dbl);
#else
    *dbl = d;
#endif
} /* IIMFstrtof8 */

/*
** Name: IIMFsd 	- Allocate a dynamic C string.
**
** Description:
**	This routine takes a COBOL string (with a length) that could not
**	be moved into a C string and allocates and fills a C string.
**	A single buffer is kept so this should NOT be used more than
**	once per statement.  Typically statements such as PREPARE or
**	EXECUTE IMMEDIATE which cannot be moved into COBOL buffers (if they
**	are too long) will require this extra step.
**
** Inputs:
**      fstr			String to convert.
**
** Outputs:
**      dbl			F8 variable to fill.
**
** Errors:
**	E_LS0101_SDALLOC - Allocation failure
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
*/
VOID
IIMFsd(len, cob_str, c_str)
i4	len;
char	*cob_str;
char	**c_str;
{
    static 	i4	bufsize = 0;	/* Size of current buffer allocated */
    static  	char	*c_buf;		/* Pointer to memory allocated */
    static	char	*alloc_err = ERx("IIMFsd-E_LS0101_SDALLOC");

    if (trace_debug)
    {
	SIprintf(ERx("IIMFsd: len = %d, cob_str = <%.*s>\n"),
		 len, len, cob_str);
    }
    if ((bufsize != 0) && (len > bufsize))
    {
	if (trace_debug)
	    SIprintf(ERx("IIMFsd: freeing because %d > %d\n"), len, bufsize);
	MEfree(c_buf);
    }
    if (len > bufsize)
    {
       bufsize = len + 256;
       c_buf = (char *)MEreqmem(0, bufsize, TRUE, NULL);
       if (c_buf == NULL)
       {
	   /* Severe error */
	   IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	   *c_str = alloc_err;
	   return;
       }
    }
    MEcopy(cob_str, len, c_buf);
    c_buf[len] = EOS;
    *c_str = c_buf;
} /* IIMFsd */

/*
** Name: SQL_PACKTOF8 	- Dynamic SQL user cover to IIMFpktof8
**
** Description:
**	This routine acts as a "friendlier" interface to the internal
**	routine to allow users Dynamic SQL access to f8 buffers.
**	
** Inputs:
**      f8_addr			Address of double variable to be filled.
**	pack_addr		Address of packed decimal variable to convert.
**	digits_before		Digits before the decimal point.
**	digits_after		Digits after the deimal point.
**	signed_var		1/0 - Is the packed variable signed.
**
** Outputs:
**      f8_addr			Result of the conversion returned to caller.
**
** Errors:
**	Those returned by the callees of this routine.
**
** Example:
**		01 F8 PIC X(8) USAGE COMP-X.
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
**		CALL "SQL_PACKTOF8" USING F8 P BY VALUE 2 3 1.
**
** History:
** 	28-nov-89 (neil)
**	   Written for MF COBOL Dynamic SQL interface.
*/
VOID
SQL_PACKTOF8(f8_addr, pack_addr, digits_before, digits_after, signed_var)
f8	*f8_addr;
COMP_3	*pack_addr;
i4	digits_before, digits_after, signed_var;
{
    if (IIMFpack_verify(ERx("SQL_PACKTOF8"), f8_addr, pack_addr,
			digits_before, digits_after, signed_var))
    {
	IIMFpktof8(f8_addr, pack_addr, digits_before + digits_after,
		   digits_after, signed_var);
    }
} /* SQL_PACKTOF8 */

/*
** Name: SQL_F8TOPACK 	- Dynamic SQL user cover to IIMFf8topk
**
** Description:
**	This routine acts as a "friendlier" interface to the internal
**	routine to allow users Dynamic SQL access to f8 buffers.
**	
** Inputs:
**      f8_addr			Address of double variable to be convert.
**	pack_addr		Address of packed decimal variable to be filled.
**	digits_before		Digits before the decimal point.
**	digits_after		Digits after the deimal point.
**	signed_var		1/0 - Is the packed variable signed.
**
** Outputs:
**      pack_addr		Result of the conversion returned to caller.
**
** Errors:
**	Those returned by the callees of this routine.
**
** Example:
**		01 F8 PIC X(8) USAGE COMP-X.
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
**		CALL "SQL_F8TOPACK" USING F8 P BY VALUE 2 3 1.
**
** History:
** 	28-nov-89 (neil)
**	   Written for MF COBOL Dynamic SQL interface.
*/
VOID
SQL_F8TOPACK(f8_addr, pack_addr, digits_before, digits_after, signed_var)
f8	*f8_addr;
COMP_3	*pack_addr;
i4	digits_before, digits_after, signed_var;
{
    if (IIMFpack_verify(ERx("SQL_F8TOPACK"), f8_addr, pack_addr,
			digits_before, digits_after, signed_var))
    {
	IIMFf8topk(f8_addr, pack_addr, digits_before + digits_after,
		   digits_after, signed_var);
    }
} /* SQL_F8TOPACK */


/*
** Name: SQL_ACCEPT 	- Interface to MF ACCEPT.
**
** Description:
**	This interface is provided due to an MF side-effect of always
**	refreshing the screen on any ACCEPT.  This interface was really
**	written for the tests and is not a documented interface.  Note,
**	however, that the Dynamic SQL Terminal Monitor Appendix does use
**	the ACCEPT statement and does need an extra ACCEPT statement.
**	
** Inputs:
**	out_len			Length of result buffer.
**
** Outputs:
**      end_file		Set to 'T' or 'F' if EOF or not.
**	out_buffer		Blank-filled result line (if not EOF).
**
** Example:
**		01 BUF PIC X(100).
**		01 EOF PIC X.
**		CALL "SQL_ACCEPT" USING EOF, BUF BY VALUE 100.
**
** History:
** 	28-nov-89 (neil)
**	   Written for MF COBOL Dynamic SQL interface.
*/
VOID
SQL_ACCEPT(end_file, out_buffer, out_len)
char	*end_file;
char	*out_buffer;
i4	out_len;
{
    char	buf[500];
    char	*icp;
    int		i;

    if (SIgetrec(buf, (i4)(sizeof(buf)-1), stdin) == OK)
    {
	*end_file = 'F';
	for (i = 0, icp = buf; i < out_len && *icp && *icp != '\n'; i++, icp++)
	    out_buffer[i] = *icp;
	for (; i < out_len; i++)
	    out_buffer[i] = ' ';
    }
    else
    {
	*end_file = 'T';
    }
} /* SQL_ACCEPT */

/*
** Name: IIMFpack_verify - Verify user arguments to Dynamic SQL interface.
**
** Description:
**	This routine checks the argeuments and prints an error.
**	
** Inputs:
**	call_name		Name of user-called routine.
**      f8_addr			Address of double variable.
**	pack_addr		Address of packed decimal variable.
**	digits_before		Digits before the decimal point.
**	digits_after		Digits after the deimal point.
**	signed_var		1/0 - Is the packed variable signed.
**
** Returns:
**	TRUE/FALSE - Success/Failure
**
** Errors:
** 	E_LS0113_CALLPACK - Invalide type description.
**
** History:
** 	28-nov-89 (neil)
**	   Written for MF COBOL Dynamic SQL interface.
*/
static i4
IIMFpack_verify(call_name, f8_addr, pack_addr, digits_before,
		digits_after, signed_var)
char	*call_name;
f8	*f8_addr;
COMP_3	*pack_addr;
i4	digits_before, digits_after, signed_var;
{
    char	err_buf[50];
    char	err_vals[50];
    char	*err_arg = err_buf;

    if (trace_debug)
    {
	SIprintf(ERx("%s: f8, pack, before = %d, after = %d, sign = %d\n"),
		 call_name, digits_before, digits_after, signed_var);
    }

    if (f8_addr == NULL)
	err_arg = ERx("1 = NULL");
    else if (pack_addr == NULL)
	err_arg = ERx("2 = NULL");
    else if (digits_before < 0)
	err_arg = ERx("3 < 0");
    else if (digits_after < 0)
	err_arg = ERx("4 < 0");
    else if (digits_before + digits_after > MAX_PK_DIGIT)
	STprintf(err_arg, ERx("3 + 4 > %d"), MAX_PK_DIGIT);
    else if (digits_before + digits_after == 0)
	err_arg = ERx("3 + 4 = 0");
    else if (signed_var != 0 && signed_var != 1)
	STprintf(err_arg, ERx("5 = %d"), signed_var);
    else
	return TRUE;

    /* Print error */
    STprintf(err_vals, ERx("%d, %d, %d"), digits_before, digits_after,
	     signed_var);
    IIlocerr(GE_DATA_EXCEPTION + GESC_TYPEINV, E_LS0113_CALLPACK,
	     0, 3, call_name, err_vals, err_arg);
    return FALSE;
} /* IIMFpack_verify */

#endif /* UNIX */
