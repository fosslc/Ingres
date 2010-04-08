# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqscsql.h>
# include	<eqlang.h>
# include	<ereq.h>
# include	<ere4.h>
# include	<eqcob.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	cobsqcan.c
** Purpose:	Cobol-dependent lexical utilities for ESQL/COBOL
**
** Defines:
**	sc_iscomment		- Is this line a comment?
**	sc_newline		- Process end of line
**	sc_label		- Return pointer after label, if any
**	sc_skipmargin		- Skip margin of new line
**	scStrIsCont		- Is line a string continuation?
**	sc_isblank		- Is this line blank?
**	sc_scanhost		- Scan host code: N/A for COBOL
**	sc_eatcomment		- Eat host comment: N/A for COBOL
**	sc_inhostsc		- Host string or comment? N/A for COBOL
**
** History:
**	11-oct-85	- bjb (wrote based on esqsclang.c template)
**	21-aug-89	- teresal (added scan for COBOL sequence numbers to
**			           sc_label, sc_newline, and sc_iscomment)
**	20-nov-89 	- neil (allowed MF COBOL comments to be in col 1 even
**			 	though ANSI format).  Fixed a problem that
**				skipped over tabs and found source.
**	16-feb-1993	- teresal (As part of flexible placement of SQL 
**				   statements changes, added sc_scanhost(), 
**				   sc_eatcomment(), and sc_inhostsc() 
**				   routines to this file. Note: since 
**				   ESQL/COBOL doesn't support flexible 
**				   placement of SQL statements, these 
**				   routines are no-ops.)
**	21-jun-1993 (lan)
**		Fixed a NIST bug where '"' was not recognized as a quote
**		character on a continued string literal.  Fix is in scStrIsCont.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-dec-2004 (wanfr01)
**	    Bug 113804, INGSRV3137
**	    removed extra parameter on sc_iscomment call 
**	    compile fails on HP itanium.
**	24-Dec-2004 (shaha03)
**	    SIR #113754 Removed parameter in calling sc_iscomment(), 
**	    to keep the ansi-c compiler happy. 
**	26-Aug-2009 (kschendel) b121804
**	    Things called from the dml-state dispatch must be i4, not bool.
*/

/* Sequence area for COBOL card format is cols 1 thru 6 */
static char	sc_marg[] = ERx("      ");
# define	sc_mrgA		ERx("       ")
# define	SC_INDCOL 	7

char		*Cscan_seqno();		/* Sequence number scan routine */

/*
+* Procedure:	sc_iscomment
** Purpose:	Determine if a line is a comment line
** Parameters:
**	None.
** Return Values:
-*	bool	- TRUE iff the line is a comment line.
** Notes:
**	- Just scan through the line.
**	- For card format, comment indicator will be in col 7; we allow spaces
**	  and asterisks in sequence area (cols 1 thru 6).  Asterisks are allowed
**	  so that programs can have comment lines acceptable to both formats.
**	- Allow comments in column 1 even in ANSI format.
**	- For terminal format, indicator will be in col 1.
**	- This routine not only checks for comment indicator ("*"), but also 
**	  for new listing page("\") and conditional compilator indicator("D"), 
**	  because these are treated like comment lines by esql preprocessor.
**	- Stop when seeing a tab to avoid assuming source code is a [debug]
**	  comment as in column 7 on the 2nd line of the following example:
**		EXEC SQL DECLARE c CURSOR FOR SELECT * FROM t
**	   <tab><tab>FOR DIRECT UPDATE OF ...
**	                 ^
**
** Imports modified:
**	None.
*/

i4
sc_iscomment()
{
    char 	*cp;
    i4		i;

    cp = SC_PTR;
    i = 1;
    if (*cp == '*')	/* First column */
	return TRUE;
    if (eq_ansifmt)	/* Skip ahead to ANSI column indicator */
    {
	while (i < SC_INDCOL && *cp != '\t')	/* Exclude tabs */
	{
	    CMbyteinc(i, cp);
	    CMnext( cp );
	}
    }
    /*
    ** If the last char was a double byte space, then we ran over
    ** the indicator column by one byte.  This is a hard column limit
    ** so this can't be a comment line.
    */
    if (i > SC_INDCOL)
	return FALSE;
    if (*cp=='*' || *cp=='\\' || *cp=='D')
	return TRUE;
    return FALSE;
}
	

/*
+* Procedure:	sc_newline
** Purpose:	Process end-of-line (language dependent)
** Parameters:	
**	None.
** Return Values:
-*	0 if this newline implies statement termination, 1 if it doesn't.
** Notes:
**	Used in COBOL to look for sequence number.
*/

i4
sc_newline()
{
    if (eq_ansifmt)
	SC_PTR = Cscan_seqno(SC_PTR); /* Scan for ANSI COBOL sequence number */
    return 1;
}

/*
+* Procedure:	sc_label
** Purpose:	Scan for a label (at start of line)
** Parameters:
**	lbl	- char **	- Pointer to pointer to label string
** Return Values:
-*	char * - Pointer into line buffer at which to resume scanning
** Notes:
**	If we were at a label, scan it, copy into a buffer, and give 
**	it to our caller.  Return pointer to next char.
**	If it turned out not to be a label, return our line buffer argument.
**	This routine doesn't actually "eat" any chars since this line may
**	turn out to be host code.
**
** Imports modified:
**	None.
*/

char *
sc_label( lbl )
char		**lbl;
{
    register char	*cp = SC_PTR;
    static char		buf[ SC_WORDMAX + 2];	/* Label + a period */
    register char	*p;
    char		*startcp;
    bool		fullbuf = FALSE;    /* TRUE if label bigger than buf */
    
    *lbl = NULL;	/* default */

    if (eq_ansifmt)
	cp = Cscan_seqno(cp);	/* Scan for ANSI COBOL sequence number */

    if (dml->dm_exec == DML_DECL)
    {
	SC_PTR = cp;
	return cp;		/* Don't look for label in declaraction */
    }	

    startcp = cp;
    p = buf;

    /*
    ** look for a label: "^{white}alnum{alnum|'-'}.}" 
    */
    while (CMspace(cp) || *cp == '\t')
	CMnext( cp );
    if (!CMnmstart(cp) && !CMdigit(cp))
	return startcp;

  /* So now we've got the start of a label. */
    CMcpyinc( cp, p );
    while ((CMnmchar(cp) || *cp == '-') && p < &buf[SC_WORDMAX])
	CMcpyinc( cp, p );
    /* 
    ** label too large to fit in buffer, so skip remaining chars
    */
    if (p >= &buf[SC_WORDMAX] && (CMnmchar(cp) || *cp == '-'))
    {
	fullbuf = TRUE;
	while (CMnmchar(cp) || *cp == '-')
	    CMnext( cp );
    }
    if (*cp == '.')
    {
	*p++ = '.';
	*p++ = '\0';
	if (eq_ansifmt)
	    Cstr_label(buf); /* Store label for output with ANSI sequence no. */
	else
	    *lbl = buf;
	if (fullbuf)
	{
	    er_write( E_EQ0216_scWRDLONG, EQ_ERROR, 2, er_na(SC_WORDMAX),
								buf );
	}
	return ++cp;	/* resumption place for caller */
    }
    return startcp;
}

/*
+* Procedure:	sc_skipmargin
** Purpose:	Skip the margin on a new line.
** Parameters:
**	None.
** Return Values:
-*	Number of characters read.
**
** Notes:
**	You must have already ensured that this is not a HOST_CODE line.
**	The first char of the line has already been read.
**	FORTRAN skips whitespace to the statement.
**	C, COBOL, PASCAL and PL/1 do nothing.
**
** Imports modified:
**	None.
*/

i4
sc_skipmargin()
{
    return 0;
}


/*
+* Procedure:	scStrIsCont
** Purpose:	Determine if the current line is a continuation line
** Parameters:
**	ch	- The quote character.
** Return Values:
-*	bool	- TRUE <==> this is a continuation line
** Notes:
**	Looks for a continuation mark, eating up intervening comment lines 
**		and blank lines.
**	Called only from sc_string.
**	The first char of the line has already been read.
**
** Imports modified:
**	Input characters may be scanned.
*/

i4
scStrIsCont( char ch )
{
    register char	*cp;
    register char	*ip;	
    i4			i;
    bool		sc_isblank();

    cp = SC_PTR;
    while (1)
    {
	if (sc_iscomment() || sc_isblank(cp))
	{
	    sc_readline();
	    cp = SC_PTR;
	    continue;
	}
	if (eq_ansifmt)
	{
	    for (i = 1; i < SC_INDCOL; i++)
		if (*cp++ == '\t')
		    return FALSE;
	}
	if (*cp++ != '-')
	    return FALSE;
	while (CMspace(cp) || *cp == '\t')
	    CMnext( cp );
	if (*cp == '\n')	/* Eat blank comment lines */
	{
	    sc_readline();
	    cp = SC_PTR;
	    continue;
	}
	if (*cp != ch && *cp != '"')		/* Can't be Kanji */
	{
	  /* Missing quotation mark at beginning of continued line */
	    er_write( E_E40014_hcbCONTIN, EQ_ERROR, 0 );
	} else
	    cp++;
	SC_PTR = cp;
	return TRUE;
    }
}


/*
+* Procedure:	sc_isblank
** Purpose:	Checks to see if line is blank
** Parameters:
**	cp	- * char	- pointer to line
** Return Values:
-*	TRUE/FALSE - line is blank
** Notes:
**	Currently only called by scStrIsCont	
*/

bool
sc_isblank( cp )
char	*cp;
{
    while (CMspace(cp) || *cp == '\t')
	CMnext( cp );
    if (*cp == '\n')
	return TRUE;
    else
	return FALSE;
}

/*
** Procedure:   sc_scanhost
** Purpose:     Do some crude parsing of host code to support flexible
**              placement of SQL code in Host code.
**
**              Since this feature is not supported in ESQL/COBOL, this
**              routine is a no-op and will always return FALSE.
**
** Parameters:
**      N/A
**
** Return Values:
**            FALSE     This host line does not need to be split up.
**
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**      in included as part of the COBOL Scanner routines so that the
**      ESQL/COBOL preprocessor can be sucessfully linked at build time.
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
*/

bool
sc_scanhost(i4 scan_term)
{
    return FALSE;
}

/*
** Procedure:   sc_eatcomment
** Purpose:     Determine if we have a host comment; if so,
**              eat it up. This routine is needed to support
**              flexible placement of SQL statements.
**
**              Since this feature is not supported for ESQL/COBOL, this
**              routine is a no-op and will always return FALSE.
**
** Parameters:
**      N/A
**
** Return Values:
**            FALSE     This is not a host comment.
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**      in included as part of the COBOL Scanner routines so that the
**      ESQL/COBOL preprocessor can be sucessfully linked at build time.
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
*/

bool
sc_eatcomment (char **cp, bool eatall)
{
    return FALSE;
}

/*
** Procedure:   sc_inhostsc
** Purpose:     Determine if input line is within a multi-line host comment
**              or string.  This routine is needed to support
**              flexible placement of SQL statements.
**
**              Since this feature is not supported for ESQL/COBOL, this
**              routine is a no-op and will always return FALSE.
**
** Parameters:
**      None
**
** Return Values:
**            FALSE     Line is not within a host comment or string.
**
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**      in included as part of the COBOL Scanner routines so that the
**      ESQL/COBOL preprocessor can be sucessfully linked at build time.
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
*/

bool
sc_inhostsc(void)
{
    return FALSE;
}
