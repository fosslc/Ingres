# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqscsql.h>
# include	<eqlang.h>
# include	<ereq.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM =      dg8_us5
*/

/*
+* Filename:	csqscan.c
** Purpose:	C-dependent lexical utilities.
**
** Defines:	sc_iscomment()			- Is this line a comment?
**		sc_newline()			- Process end-of-line.
**		sc_label()			- Scan an optional label.
**		sc_scanhost()			- Scan host code.
**		sc_eatcomment()			- Eat host comment.
-*		sc_inhostsc()			- Is line within a host 
**						  string or a host comment?
**
** Notes:
**		This file is pulled in indirectly via esqyylex.c.
**
** History:	17-jul-1985	Written	for esqsclang.c (mrw)
** 		20-aug-1985	Extracted out C dependent code (ncg)
**		28-may-1987	Modified for Kanji (mrw)
**		09-feb-1993	Added flexible placement of SQL statements
**				within C host code. (teresal)
**		25-Aug-1993 (fredv)
**			included <st.h> for redefining STbcompare and
**			STlength to IIST...
**		14-apr-1994	Added support for C++ style comments.
**				(teresal)
**		22-Jun-1995 (walro03/mosjo01)
**				Added NO_OPTIM dg8_us5.  Symptom: loop in esql.
**				Works in DG/UX 3.00 MU05; fails in DG/UX 3.10 MU02.
**		25-Feb-1998	consi01 bug 92184 INGEMB 26
**				Don't take // as a C++ comment if we are
**				inside a string
**	14-May-1999 (kitch01)
**	   Bug 96482. Amend sc_eatcomment() to correctly eat an entire
**         comment following an EXEC statement.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Things called from the dml-state dispatch must be i4, not bool.
*/

/*
+* Procedure:	sc_iscomment
** Purpose:	Determine if a line is a comment line.
** Parameters:
**	cp	- char *	- Pointer to the line.
** Return Values:
-*	i4	- TRUE iff the line is a comment line.
** Notes:
**	Just scan through the line.
*/

i4
sc_iscomment( void )
{
    return (i4)FALSE;	/* C never has a special comment line */
}

/*
+* Procedure:	sc_newline
** Purpose:	Process end-of-line (language dependent).
** Parameters:
**	char *	- cp	- Pointer to line buffer.
** Return Values:
-*	0/1 - Not continued/Continued.
** Notes:
**	We will be in EXEC mode when called.
**	The first char of the line has already been read.
**
**	For languages that put the continuation mark at the end of 
**	original line (BASIC, some RATFORs), we'll have sc_operator('\\')
**	check if the next char is '\n'; if so, it'll ignore both.
**
** Imports modified:
**	Nothing.
*/

i4
sc_newline( cp )
char	*cp;
{
    return 1;
}

/*
+* Procedure:	sc_label
** Purpose:	Scan for a label (at start of line)
** Parameters:
**	cp	- char *	- Pointer to line buffer
** Return Values:
-*	char * - Pointer into line buffer at which to resume scanning
** Notes:
**	If we were at a label scan it, , copy into a buffer, and give 
**	it to our caller.  Return pointer to next char.
**	Otherwise return our argument.  This routine doesn't "eat"
**	any characters since this line may turn out to be host code.
**
** Imports modified:
**	None.
*/

char *
sc_label( lbl )
    char		**lbl;
{
    register char	*cp = SC_PTR;
    static char		buf[ SC_WORDMAX + 3]; 	/* Label + colon + Kanji */
    register char	*p = buf;
    bool		fullbuf = FALSE;  /* TRUE if label bigger than buf */

    *lbl = NULL;	/* default */
  /* look for a label: "^{white}alpha{alnum}+{white}:" */
    while (CMspace(cp) || *cp == '\t')
	CMnext(cp);
    if (CMnmstart(cp))
    {
	CMcpyinc(cp, p);
	while (CMnmchar(cp) && p < &buf[SC_WORDMAX])
	    CMcpyinc(cp, p);
	/*
	** label too large for buffer; skip remaining chars 
	*/
	if (p >= &buf[SC_WORDMAX] && CMnmchar(cp))
	{
	    fullbuf = TRUE;
	    while (CMnmchar(cp))
		CMnext(cp);
	}
	while (CMspace(cp) || *cp == '\t')
	    CMnext(cp);
	if (*cp++ == ':')		/* label! */
	{
	    *p++ = ':';
	    *p++ = '\0';
	    *lbl = buf;
	    if (fullbuf)
		er_write( E_EQ0216_scWRDLONG, EQ_ERROR, 2, SC_WORDMAX, buf );
	    return cp;			/* Resumption place for caller */
	}
    }
    return SC_PTR;			/* no label to skip */
}

/*
+* Procedure:	sc_skipmargin
** Purpose:	Skip the margin on a new line.
** Parameters:
**	None.
** Return Values:
-*	Number of characters read.
** Notes:
**	You must have already ensured that this is not a HOST_CODE line.
**	The first char of the line has already been read.
**	COBOL and FORTRAN skip whitespace to the statement field.
**	C, PASCAL, and PL/1 do nothing.
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
** Purpose:	Determine if the current line is a continuation line.
** Parameters:
**	None.
** Return Values:
-*	i4 - TRUE <==> this is a continuation line.
** Notes:
**	Just looks for a contination mark.
**	Called only from sc_string.
**	The first char of the line has already been read.
**
** Imports modified:
**	None.
*/

i4
scStrIsCont()
{
    return (i4)FALSE;
}

static bool in_comment = FALSE;
static bool in_cpp_comment = FALSE;
static bool in_hoststring = FALSE;

/*
** Procedure:	   sc_incomment()
** Purpose:	   To give read only access to state of in_comment flag
** Parameters:     None
** Return Values:  Value of in_comment flag
** History:
**      7-Apr-2009 : (coomi01) b121895
**                 Created.
*/
bool
sc_incomment()
{
    return in_comment;
}

/*
** Procedure:	sc_scanhost
** Purpose:	Do some crude parsing of host code. Scan host code and eat
**		host comments and host string literals while looking for 
**		a host terminator or the end of line. This routine is called
**		with two purposes:
**
**		scan_term = TRUE	Determine if the current 
**					host code line has multiple 
**					statements on a line. Host
**					code is scanned for a host
**					terminator. If one is found, this
**					routine scans to see if there
**					is another statment on the line, if
**					so, it will return TRUE and will
**					reset SC_PTR to point to the next
**					statement.
**		scan_term = FALSE	Scan until the end of the line. 
**					Don't look for a host terminator.
**					This option is used when we
**					are parsing host code in a declare
**					section and we just want to scan
**					for host comments and host strings.
**
**		This routine was written to support flexible placement of 
**		SQL statements within C host code. FIPS requires that
**		SQL statements can be anywhere a C statement can be.
**		For example, C statements and SQL statements can be mixed on
**		a single line, like so:
**
**			printf("hi;there"); EXEC SQL COMMIT; printf("hi");
**
**			switch(answer)
**			{
**				case 'Y': EXEC SQL COMMIT;
**				case 'N': EXEC SQL ROLLBACK;
**			}		
**
**		Or multiple SQL statements can be on one line:
**
**			EXEC FRS FORMS; EXEC SQL ROLLBACK;
**
** Parameters:
**	scan_term 	TRUE	Scan for a host terminator, if found, 
**				reset SC_PTR if another statement follows.
**			FALSE	Scan till EOL, eat host comments and strings. 
**
** Return Values:
**	bool  TRUE 	Found another statement on this line. SC_PTR is 
**			reset to point to the next statement.
**	      FALSE	This host line does not need to be split up. Only
**			found one C statement or we were just scanning
**			for comments and strings.
** Notes:
**	Called from the ESQL scanner.
**	Resets SC_PTR
**
**	Host Language Constructs
**	======================== 
**	C string delimitor is the double quote
**	C terminator is the semicolon: ;
**		However, in C, new statments can also follow the 
**		following C keywords/operators:
**			{
**			)
**			:
**			do
**			else
**		Thus, this routine treats the above C keywords/operators
**		as terminators. If one of the above characters/words is 
**		found in host code, it is treated like a terminator,
**		and the line is scanned to look for another statement.
**
**		C statements are allowed in the following places:
**
**		1. After the semicolon terminator:
**			statement; statement;
**		2. In a C compound statement or block of statements:
**			{ statement1; statement 2; }
**		3. Do statement:
**			do statement; while (expression);
**		4. While statement
**			while (expression) statement;
**		5. For statement
**			for (expression; expression; expression) statement;
**		6. If statement
**			if (expression) statement;
**			else if (expression) statement;
**			else statement;
**		7. Switch statement
**			switch (expression) {
**				case value1 : statement; statement; ...
**				case valuen : statement; statement; ...
**				default: statement; statement; ...
**			}
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within C host code as required by 
**			FIPS.
**
*/

static char     *c_term[] = {
                ERx(";"),
                ERx("{"),
                ERx(")"),
                ERx(":"),
                ERx("do"),
                ERx("else"),
                (char *)0
};

bool
sc_scanhost(i4 scan_term)
{
    char 	*cp;	
    char	save_char;
    bool	found_term = FALSE;
    char	**term_ptr;
    i4		len;

    cp = SC_PTR;
    save_char = '\0';

    /* Scan the host code, look for comments, strings and terminator */

    while (*cp != '\n' && *cp != SC_EOFCH)
    {
	if (sc_eatcomment(&cp, FALSE))	/* Are we in a host comment? */
	    continue;			/* If so, eat it and continue */
	else if (in_hoststring && 		
		 (*cp == '\\' && *(cp+1) == '\\'))
	{
	    CMnext(cp);			/* Eat escaped backslash in comment */
	    CMnext(cp);			/* so "\\" string won't confuse us */
	    continue;			/* when we check if quote is escaped */
	}
	else if (*cp == '"')		/* Are we in a host string */
	{
	    /* Make sure quote isn't escaped */
	    if (save_char != '\\')
	    {
                if (in_hoststring)
                    in_hoststring = FALSE;

		/* Make sure quote is not in a comment or character literal */
                else if (!in_comment &&  
			 !(save_char == '\'' && *(cp+1) == '\''))
                    in_hoststring = TRUE;
            }
	}
	else
	{
	    /* Look for C terminator */
	    for (term_ptr = c_term; *term_ptr != NULL; ++term_ptr)
	    {
		len = STlength(*term_ptr);
		if (STbcompare(*term_ptr, 0, cp, len, TRUE) == 0)
		{
                    if (!in_comment && !in_hoststring)
                    {
                        cp = cp + (len-1);          /* Found Term */
                        found_term = TRUE;
                    }
		    break;
		}
            }
        }
	save_char = *cp;
	CMnext(cp);
	if (found_term && scan_term)
	   break;
    } /* While (*cp != '\n') */

    if (*cp == '\n' || *cp == SC_EOFCH)
	return FALSE;
    
    /*
    ** We have found a terminator, now scan past this spot. 
    ** Is there anything out there besides white space, newline and/or
    ** host comments?
    */
    while (*cp != '\n' && *cp != SC_EOFCH)
    {
	if (CMwhite(cp))
	{
	    CMnext(cp);
	    continue;
	}
	else if (sc_eatcomment(&cp, FALSE))
	{
	    continue;
	}
	else
	{
	    /* Reset SC_PTR to point to start of newline */	
	    SC_PTR = cp;
	    return TRUE;
	}
    } /* End of while */	
    return FALSE;
}

/*
** Procedure:	sc_eatcomment
** Purpose:	Determine if we have a host comment; if so, 
**		eat it up. This routine is used to eat C comments
**		within host code or at the end of SQL statements. 
**
**		C comments within host code will be generated to the
**		output file while C comments after SQL statements
**		are not.
**
** Parameters:
**	char **cp	Pointer to input string. 
**	bool eatall	Eat past the newline?
**			TRUE - eat the entire comment even if it spans
**			       lines. This is set to TRUE after we 
**			       have just parsed an SQL statement.
**			FALSE - Just parse up to the newline.
**			
**
** Return Values:
**	i4 - TRUE 	A host comment was found and eaten;
**			cp now points to the next character
**			after the comment.
**	      FALSE	This is not a host comment; cp has
**			not been changed.
** Notes:
**	This routine will scan the current line for comments.
**	If the comment is continued over multiple lines, this routine
**	will return TRUE and will eat up until it finds a newline or if
**	if eatall is set to TRUE, the entire comment is eaten.
**
**	Host Language constructs
**	======================== 
**	C comments delimiters are: '/' followed by '*'  and
**				   '*' followed by '/'.
**
**	C++ comments can be C comments (see above) or C++ only style
**	comments which start with '//' until the newline.
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within C host code as required by 
**			FIPS.
**	14-apr-1994	(teresal)
**			Added ESQL/C++ support for C++ style comments.
**	25-Feb-1998	consi01 bug 92184 INGEMB 26
**			Don't take // as a C++ comment if we are
**			inside a string
**	14-May-1999 (kitch01)
**	   Bug 96482. If eatall is set the code is supposed to remove
**         the entire comment even if it spans lines. (see above for input
**         parms). This was not being done if the start comment was followed
**         immediately by a new line.
*/

bool
sc_eatcomment (char **cp, bool eatall)
{
    /*	
    ** Check if this is the start of a comment or if we are already in a 
    ** comment; if so, eat until end of comment or newline
    */

    /* If ESQL/C++, determine if this is a C++ style comment */
    if (eq_islang(EQ_CPLUSPLUS) && (**cp == '/') && (*(*cp+1) == '/'))
    {
	if (in_hoststring) /* ignore occurance of // as */
            return FALSE;  /* we are inside a string    */

	CMnext (*cp);
	CMnext (*cp);
	in_comment = TRUE;	
	in_cpp_comment = TRUE;
    }	
    /* Look for a C style comment */
    else if ((**cp == '/') && (*(*cp+1) == '*') && !in_hoststring)
    {
	CMnext (*cp);
	CMnext (*cp);
	in_comment = TRUE;	
    }	
    else if (!in_comment)		/* Within a comment?, if no, return */
	return FALSE;

    /* Bug 96482. Get the next line if we're supposed to eat the entire
    ** comment and we are at the end of the line following the start 
    */
    if (eatall && **cp == '\n')
    {
       sc_readline();
       *cp = SC_PTR;
    }
	
    while (**cp != '\n' && **cp != SC_EOFCH)
    {
	if (in_cpp_comment)
	    CMnext(*cp);
	else
	{
	    if (**cp == '*' && *(*cp+1) == '/')
	    {
	    	CMnext(*cp);
	    	CMnext(*cp);
	    	in_comment = FALSE;
	    	break;
	    }
	    CMnext(*cp);
	    if (eatall && **cp == '\n')
	    {
	   	sc_readline();
	   	*cp = SC_PTR;
	    }
	}
    }	
    if (in_cpp_comment)
    {
	in_cpp_comment = FALSE;
	in_comment = FALSE;
    }	
    return TRUE;
}

/*
** Procedure:	sc_inhostsc
** Purpose:	Determine if input line is within a multi-line host comment
**		or string. This routine is called from the ESQL
**		scanner to determine if it should process the current line
**		as host code or look for EXEC SQL. This routine is used
**		to catch the following situations:
**
**			/* Commment over multiple lines
**				EXEC SQL statement.....
**
**			printf("Multi-line host string literal\
**				EXEC SQL statement .......");
**
**			Neither statement above should generate SQL code.
**
** Parameters:
**	None
**
** Return Values:
**	bool  TRUE 	Line is within a host comment or host string.
**	      FALSE	Line is not within a host comment or string.
**
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within C host code as required by 
**			FIPS.
*/

bool
sc_inhostsc(void)
{
    if (in_hoststring || in_comment)
	return TRUE;
    else
	return FALSE;
}
