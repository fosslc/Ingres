# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<ereq.h>
# include	<ere3.h>
# include	<eqbas.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

/*
+* Filename:	<basutils.c>
** Purpose:	BASIC-specific preprocessor utilities
**
** Defines:
**	Bscan_lnum	Remove and save EQUEL/BASIC line numbers
**	Bsq_lnum	Remove and save ESQL/BASIC line numbers & labels
**	Bput_lnum	Generate line number (if any) and clear
**	Bdecl_errs	Issue error messages for wrong syntax on decls
**	B_isendmain	Returns TRUE if "END" token is end main stmt
-* Notes:
**
** History:
**	02-feb-86	- Written (bjb)
**	07-jul-86	- Updated for 6.0 (bjb)
**      08-30-93 (huffman)
**              add <st.h>
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**
*/

/* Buffers for BASIC line nums extracted from input and later emitted */
GLOBALDEF	char	bas_numbuf[ SC_NUMMAX + 1] ZERO_FILL;
# define	BAS_LABSIZE	31	/* Maximum size of BASIC label */

/*
+* Procedure:	Bscan_lnum
** Purpose:	Remove and save line number on EQUEL lines
** Parameters:
**	None
** Return Values:
-*	The current character in the input buffer.
** Notes:
**	This routine is called indirectly (via dml->dm_scan_lnum) 
** from the scanner to store away BASIC line numbers on EQUEL lines.  SC_PTR 
** is left pointing to the "##".  If no line number is found, but a "##" 
** appears after initial white space, SC_PTR is adjusted to point there.
** A BASIC line number consists of an integer between 1 and 32767
** terminated by a space or a tab.  (Our code allows up to 100 digits.)
**
** Imports modified:
**	SC_PTR - the scanner's pointer to input.
*/

void
Bscan_lnum()
{
    char 	*cp, *numptr;
    i4		numlen;

    cp = SC_PTR;

    bas_numbuf[0] = '\0';		/* Assume no line number */

    /* Scan leading white space */
    for (; CMspace(cp) || *cp == '\t'; CMnext(cp))
	;

    /* If this is an EQUEL line (with no line number), adjust SC_PTR */
    if (STbcompare(ERx("##"), 2, cp, 2, FALSE) == 0)
    {
	SC_PTR = cp;
	return;
    }

    /* No ##, no line number; return without changing SC_PTR */
    if (!CMdigit(cp))
	return; 

    /* A line number exists -- save a pointer to it and scan */
    for (numptr = cp; CMdigit(cp); CMnext(cp))
	;
    numlen = cp - numptr;

    /* If this is an EQUEL line, expect "##" */
    for (; CMspace(cp) || *cp == '\t'; CMnext(cp))
	;
    if (STbcompare(ERx("##"), 2, cp, 2, FALSE) != 0)	   /* Equel line? */
	return;

    /* Store away line number for later emitting */
    STlcopy( numptr, bas_numbuf, min(numlen,SC_NUMMAX) );
    /* Adjust scanner's input pointer */
    SC_PTR = cp;

}

/*{
+*  Name: Bsq_lnum - Scan for an optional ESQL/BASIC line number/label
**
**  Description:
**	This routine is called from the BASIC-specific scanner routines. 
**	It scans the line number and the label (both optional) copying them 
**	into a static buffer.
**	A BASIC line number consists of an integer between 1 and 32767
**	terminated by a space or a tab.  
**	A BASIC label consists of 1 to 31 characters, the first digit of 
**	which must be alphabetic.  The remaining characters may be any 
**	combination of leters, numbers, dollar signs, underscores or periods.  
**	Our code does not allow periods in labels.
**	Line numbers and labels may be present on any ESQL statement,
**	or BASIC declaration.  (However, they will not be emitted if
**	they appear on statements that don't generate code.)
**
**  Inputs:
**	cp		- Pointer to current position in input buffer
**
**  Outputs:
**	Returns:
**	    Pointer to current place in buffer after scanning number/label.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	06-jan-1986 - written (bjb)
**	21-jul-1987 - updated for 6.0 (bb)
*/

char *
Bsq_lnum( cp )
register 	char *cp;
{
    register	char	*p = bas_numbuf;
    char	*savep = bas_numbuf;
    char	*old_cp = cp;
    register	i4 	i = 0;

    bas_numbuf[0] = '\0';

    for (; CMspace(cp) || *cp == '\t'; CMnext(cp)) /* Scan leading whitespace */
	;

    old_cp = cp;
    if (CMdigit(cp))		/* Line number? */
    {
	while (i < 5 && CMdigit(cp))
	{
	    CMbyteinc(i, cp);
	    CMcpyinc(cp, p);
	}

	/* 
	** If number is not a legal number, we also know here that it's 
	** not a legal label (labels can't start with a digit).
	*/
	if (*cp != '\t' && !CMspace(cp))
	{
	    bas_numbuf[0] = '\0';
	    return old_cp;
	}
	savep = p;
	old_cp = ++cp;			/* Get ready to scan label */
    }

    for (; CMspace(cp) || *cp == '\t'; CMnext(cp));	/* Skip whitespace */

    /* Look for a label: "alpha{alnum}:" */
    if (!CMalpha(cp))
    {
	*savep = '\0';
	return cp;
    }
    old_cp = cp;
    *p++ = '\t';		/* Separate number and label by a tab */
    i = 0;
    while (CMnmchar(cp) && i < BAS_LABSIZE)
    {
	CMbyteinc(i, cp);
	CMcpyinc(cp, p);
    }

    for (; CMspace(cp) || *cp == '\t'; cp++);	/* Skip whitespace */

    if (*cp++ == ':')
    {
	*p++ = ':';
	*p++ = ' ';
	*p = '\0';
	return cp;
    }
    *savep = '\0';
    return old_cp;
}

/*
+* Procedure:	Bput_lnum
** Purpose:	Controls emission of BASIC line numbers on EQUEL lines
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**	If there's a line number in the buffer, call code generator to
** emit it and then clear out buffer.
**
** Imports modified:
**	bas_numbuf
*/

void
Bput_lnum()
{
    if (bas_numbuf[0] != '\0')
	gen_Blinenum( bas_numbuf );
    bas_numbuf[0] = '\0';
}

/*
+* Procedure:	Bdecl_errs
** Purpose:	Given the type of BASIC declaration, emit error messages
**		about incorrect syntax
** Parameters:
**	class	- i4	- class of data declaration
**	type	- i4	- type of data
**	badnumeric - i4 - TRUE if 8-bit double or packed
**	dims    - i4    - indicates array subscripts seen
**	equals  - i4	- assignment clause/string length seen
**	
** Return Values:
-*	None
** Notes:
**	Called from the grammar on every declaration. 
**
** Imports modified:
**	None.
*/

void
Bdecl_errs( class, type, badnumeric, dims, equals )
i4	class, type;
i4	badnumeric, dims, equals;
{
    switch (class) {

    case BclDEFFUNC:
	if (dims || equals)	/* Subscript or equals clause in DEF param */
	    er_write( E_E30009_hbDEFPARM, EQ_ERROR, 0 );
	break;
 
    case BclEXVAR:
	if (!Bis_numeric(type))
	    er_write( E_E3000A_hbEXTNTYPE, EQ_ERROR, 0 );
	else if (dims || equals)	/* Subscript or equals clause in decl */
	    er_write( E_E3000B_hbEXTID, EQ_ERROR, 0 );
	break;

    case BclEXCONS:
	if (!Bis_numeric(type)  || badnumeric)
	    er_write( E_E3000A_hbEXTNTYPE, EQ_ERROR, 0 );
	else if (dims || equals)	/* Subscript or equals clause in decl */
	    er_write( E_E3000B_hbEXTID, EQ_ERROR, 0 );
	break;

    case BclCONSTANT:
	if (type == T_STRUCT || type == T_FORWARD)  /* User-defined record */
	    er_write( E_E3000C_hbCNSTYPE, EQ_ERROR, 0 );
	else if (dims)		/* Subscripts present */
	    er_write( E_E3000D_hbCNSUBS, EQ_ERROR, 0 );
	else if (!equals)	 /* Assignment clause missing */
	    er_write( E_E3000E_hbCNSASSG, EQ_ERROR, 0 );
	break;

    case BclDIMS:
	if (!dims) 		/* Subscripts missing off DIM statement */
	    er_write( E_E3000F_hbDIMSUB, EQ_ERROR, 0 );
	else if (equals)
	    er_write( E_E30010_hbDIMSTR, EQ_ERROR, 0 );
	break;

    default:	/* Not an def, external, constant or dims */
	if (equals)		/* Illegal string length */
	{
	    if (type != T_CHAR)
	    {
		er_write( E_E30011_hbSTR, EQ_ERROR, 0 );
	    }
	    else if (class != BclSTATIC 
		&& class != BclRECORD
		&& class != BclSUB)
	    {
		er_write( E_E30012_hbDYNSTR, EQ_ERROR, 0 );
	    }
	}
    }  	/* switch */
}

/*
+* Procedure:	B_isendmain
** Purpose:	Enables parser decide whether this is an "end main" statement
** Parameters:
**	None
** Return Values:
**	TRUE  - we are in an "end main" statement
-*	FALSE - not an "end main" statement
** Notes:
**	Called from BASIC parser after an "END" token has been seen.
** Its job is to establish whether the grammar is in an END ("end main"),
** END SUB, END FUNCTION or END DEF statement.  (The grammar runs into
** shift/reduce problems if it tries to decide this by rules alone.)
** Method:  Look into the input line buffer.  Decide that we are in an 
** "end main" statement if the next word in the scanner's input buffer
** is something other than : SUB, FUNCTION or DEF.
**
** Imports modified:
** 	None.
*/

bool
B_isendmain()
{
    register	char 	*cp;

    cp = SC_PTR;
    while ( CMspace(cp) || *cp == '\t' )
	CMnext(cp);
    if (STbcompare(ERx("sub"), 3, cp, 3, TRUE) == 0)
	return FALSE;
    if (STbcompare(ERx("function"), 8, cp, 8, TRUE) == 0)
	return FALSE;
    if (STbcompare(ERx("def"), 3, cp, 3, TRUE) == 0)
	return FALSE;
    return TRUE;
}
