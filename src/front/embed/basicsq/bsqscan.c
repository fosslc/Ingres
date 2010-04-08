/*
** Copyright (c) 1986, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include       <st.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqscsql.h>

/**
+*  Name:	bassqscan.c - BASIC dependent ESQL scanner routines.
**  Purpose:	Language-dependent lexical utilities.
**
**  Defines:
**	Bsq_continue()			Is current line to be continued?
**	sc_newline()			Is current line a continuation line?
**	sc_iscomment			Is current line a BASIC comment line?
**	sc_label()			Scan and save line num/label
**	sc_skipmargin()			Skip margin on a declaration
**	scStrIsCont()			Is current line a string continuation?
**	sc_scanhost()			Scan host code - N/A for Basic
**	sc_eatcomment()			Eat host comment - N/A for Basic
**	sc_inhostsc()			Host string or comment? - N/A for Basic
**
**  Notes:
**	This file is based on the template esqsclang.c.
**	The modifications reflect the BASIC syntax and format rules
**	that we follow in scanning embedded statements.
**
**	ESQL/BASIC has no statement terminators.  This has
**	necessitated several special workarounds in the scanner
**	and grammar as follows:
**
**	1.  Fake terminators on most EXEC statements.
**	    Since the G grammar expects terminators at the end of
**	    embedded statements, we have faked a statement terminator
**	    token in the scanner.  In general, if we're in DML_EXEC
**	    mode and we've seen a newline, we send back a terminator
**	    token if the next line is NOT a continuation line.
**	
**	2.  No fake terminators on host declarations.
**	    The grammar rules for declarations do not require terminators.  
**	    When the scanner is in DML_DECL mode no terminator token will be 
**	    sent. Note, however, that line continuation is allowed for 
**	    declarations.
**
**	3.  No fake terminators on EXEC SQL DECLARE TABLE statements.
**	    Since the DECLARE TABLE rule is completely handled in
**	    the L grammar, terminators are not required.  No
**	    terminator token will be generated because the grammar
**	    turns DML_DECL mode back on when it has reduced the
**	    whole statement.
**
**	4.  No fake terminators on EXEC SQL INCLUDE statements.
**	    For INCLUDE statements within a declaration section, the 
**	    L grammar does not require a terminator.  However, the
**	    INCLUDE statement in G DOES require a terminator.  The 
**	    scanner cannot use the fake terminator mechanism on 
**	    INCLUDE statements, however, because it must be in sync 
**	    with the grammar before the include file is scanned.  The 
**	    solution is to make a special rule "host_inc_term" to be 
**	    used instead of "host_term" on include statements.  
**	    "host_inc_term gets defined in all L grammars.  In BASIC 
**	    it's a null rule.
-*
**  History:
**	05-jan-1986	- written (bjb)
**	16-jul-1987	- updated for 6.0 (bjb)
**	15-jan-1989	- fixed bug #4456 (bjb)
**			Repeated line number bug where we generated duplicate
**			line numbers when a statement with optional last 
**			part has optional part omitted.  Caused by look-
**			ahead mechanism in parser causing calling sequence
**			lex_isempty -> sc_iscomment -> Bsq_lnum.  The last
**			routine will zap line number of current statement as
**			it looks ahead.  Fix in sc_continue() is 
**			1) if last line was NOT continued, we are not
**			'within' an embedded statement -- return FALSE
**			without calling Bsq_lnum()
**			2) if last line was continued, don't look for a linenum.
**			A line number on a BASIC continued line is illegal.
**	16-feb-1993	As part of flexible placement of SQL statements
**			changes, added sc_scanhost(), sc_eatcomment(), and
**			sc_inhostsc() routines to this file. Note: 
**			since Basic doesn't support flexible placement of
**			SQL statements, these routines are no-ops.
**	26-feb-1993 (lan)
**		Fixed bug 49739 where Basic line continuation was not working.
**      08-30-93 (huffman)
**              add <st.h>
**      14-may-1996 (thoda04)
**              Define sc_skipmargin() as returning nat, the same as all the
**              other languages, so that the function prototype will not flag it.
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**
**/

static i4	bas_continued = 0;    /* Remember if last line was continued */
char		*Bsq_lnum();	      /* Line number handler (see basutils.c) */

/*{
+*  Name: Bsq_continue - Check if the current line is being continued.
**
**  Description:
**	A BASIC line is marked as being continued if it ends with
**	a '&\n'.  This routine is called from sc_operator (via
**	the function pointer sc_linecont()) when a "&" is encountered.  
**	It checks to see if the operator '&' is followed by a newline, 
**	indicating a continued line.  If so, a static flag (bas_continued)
**	is set.  Sc_newline (called from the scanner on newlines in EXEC
**	mode) will check this flag in order to tell the scanner whether 
**	the next line is continued.  In DECL mode, the scanner "ignores"
**	continued lines; however, the bas_continued flag must be reset
**	and this is done is sc_skipmargin().
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns	
**	    1	The current line is being continued
**	    0	This line is not being continued
-*
**  Side Effects:
**	    If this line is being continued, SC_PTR will be advanced,
**	    and the static variable bas_continued will be set to TRUE.
**	
**  History:
**	25-nov-1986 - written (bjb)
*/

i4
Bsq_continue()
{
    if (*SC_PTR == '&' && *(SC_PTR +1) == '\n')
    {
	bas_continued = TRUE;
	SC_PTR++;
	return 1;
    }
    return 0;
}


/*{
+*  Name: sc_iscomment - Is this a BASIC comment line?
**
**  Description:
**	This function is called (via function pointer dml->dm_iscomment) from 
**	lex_isempty() in the scanner.  It is called to check for host 
**	comments within SQL statements.  
**	For BASIC the comment indicator is (! or REM).  A line number and/or
**	label may be present at the beginning of a comment line and the 
**	comment may immediately follow without any intervening white space.  If
**	there is no line number, there should be at least one space at the 
**	beginning of the line, but we don't actually check for that.
**	Note that although we check for a BASIC line number/label, the grammar 
**	will not be emitting line numbers or labels because the comment lines 
**	within statements are not emitted.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    TRUE/FALSE (This line IS/ISN'T a comment line)
-*
**  Side Effects:
**	None
**
**  History:
**	20-nov-1986 - written (bjb)
**	15-jan-1989 - fixed bug #4456 (bjb)
**	26-feb-1993 (lan)
**		Fixed bug 49739 where Basic line continuation was not working:
**		cp was not initialized.  cp now points to the input.
*/

i4
sc_iscomment()
{
    register char	*cp;

    /* If last line wasn't continued, we're in a new statement */
    if (!bas_continued)
	return FALSE;

    /* cp = Bsq_lnum( SC_PTR ); */
    cp = SC_PTR;

    /* Scan optional white space */
    for (; CMspace(cp) || *cp == '\t'; CMnext(cp))
	;
    if (*cp == '!')
	return TRUE;
    if (*cp == 'r' || *cp == 'R')	/* Don't trust STbcompare on null str */
    {
	if (STbcompare( cp, 3, ERx("rem"), 3, TRUE) == 0)
	    return TRUE;
    }
    return FALSE;
}


/*{
+*  Name: sc_newline - Process a newline (might be a continuation line)
**
**  Description:
**	This routine is called from the scanner at the beginning of
**	a newline if we're in EXEC mode (i.e., processing embedded 
**	statements).  BASIC continuation lines are marked by an ('&\n')
**	on the previous line.  The routine Bsq_continue() in this module
**	has set a flag if the previous line was being continued.  If the
**	flag is set, this routine returns 1, indicating to the
**	scanner to continue processing the current ESQL statement.  If
**	the flag is not set (i.e., this is not a continuation line), we 
**	return 0.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    1 	- This is a contiued line.
**	    0   - This is not a continued line.
-*
**  Side Effects:
**	    Resets static variable bas_continued.
**	
**  History:
**	20-nov-1986 - written (bjb)
*/

i4
sc_newline()
{
    if (bas_continued == TRUE)
    {
	bas_continued = FALSE;
	return 1;
    }
    return 0;
}

/*{
+*  Name: sc_label - Scan for a line number/label at the beginning of a line
**
**  Description:
**	A BASIC line number must be an integer between 1 and 32767 (we
**	scan up to 5 digits).
**	A BASIC label is a 1 to 31-character name followed by a
**	colon.  The first character must be alphabetic, the following
**	characters may include alphas, digits, dollar signs, underscores and
**	periods.  (We don't allow periods.)
**	This routine calls Bsq_lnum() which scans an optional line number and 
**	label and stores it into the global buffer bas_linebuf for later 
**	emitting.  It returns the scanner's label buffer as null.
**
**  Inputs:
**	lbl		- (Indirect) pointer to label buffer
**
**  Outputs:
**	Returns:
**	    Pointer into scanner's buffer at which to resume scanning.
**	Errors:
**	    None.	
-*
**  Side Effects:
**	Sets scanner's label buffer to null.
**	
**  History:
**	25-nov-1986 - written (bjb)
*/

char *
sc_label( lbl )
char		**lbl;
{
    *lbl= NULL;				/* Tell scanner there's no label */
    return (Bsq_lnum( SC_PTR ));
}

/*{
+*  Name: sc_skipmargin - Skip the margin on a DECL-type line.
**
**  Description:
**	This routine is called from the ESQL scanner if DECL mode is on
** 	(i.e., host declaration section) OR if the scanner is in raw-mode.
**	Skipping the margin in a BASIC program means scanning an optional
**	line number and label.  This routine calls Bsq_lnum() to scan and
**	store away the line number/label for later emitting.
**	This line may be a continuation line.  In DECL mode we allow
**	declarations to continue over multiple lines without or without
**	the BASIC continuation operator.  However, the flag, bas_continued,
**	must be reset.
**
**  Inputs:
**	cp		- current position in scanner's input buffer
**
**  Outputs:
**	Nothing.
**	Errors:
**	    None.
-*
**  Size effects:
**	The scanner's pointer to input may be advanced.
**	The static flag, bas_continued, is reset.
**
**  History:
**	25-nov-1986  written (bjb)
*/

i4
sc_skipmargin()
{
    SC_PTR = Bsq_lnum( SC_PTR );
    bas_continued = FALSE;
    return 0;
}

/*{
+*  Name: scStrIsCont - Determine if the current line is a continued string
**
**  Description:
**	Since there is no string continuation in BASIC, this routine
**	always returns FALSE.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    FALSE - This line is not a continued string
-*
**	
**  History:
**	26-nov-1986 - written (bjb)
*/

i4
scStrIsCont()
{
    return FALSE;
}

/*
** Procedure:   sc_scanhost
** Purpose:     Do some crude parsing of host code to support flexible 
**		placement of SQL code in Host code. 
**
**		Since this feature is not supported in Basic, this 
**		routine is a no-op and will always return FALSE. 
**
** Parameters:
**	N/A
**
** Return Values:
**            FALSE     This host line does not need to be split up.
**
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**	in included as part of the Basic Scanner routines so that the 
**	ESQL/Basic preprocessor can be sucessfully linked at build time.
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
**		flexible placement of SQL statements.
**
**		Since this feature is not supported for Basic, this 
**		routine is a no-op and will always return FALSE. 
**
** Parameters:
**      N/A
**
** Return Values:
**            FALSE     This is not a host comment.
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**	in included as part of the Basic Scanner routines so that the 
**	ESQL/Basic preprocessor can be sucessfully linked at build time.
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
**		flexible placement of SQL statements.
**
**		Since this feature is not supported for Basic, this 
**		routine is a no-op and will always return FALSE. 
**
** Parameters:
**      None
**
** Return Values:
**            FALSE     Line is not within a host comment or string.
**
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**	in included as part of the Basic Scanner routines so that the 
**	ESQL/Basic preprocessor can be sucessfully linked at build time.
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
