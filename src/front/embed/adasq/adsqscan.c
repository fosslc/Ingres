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
# include	<ere6.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	adasqscan.c
** Purpose:	ADA-dependent lexical utilities.
**
** Defines:	sc_iscomment()			- Is this line a comment?
**		sc_newline()			- Process end-of-line.
**		sc_label()			- Scan an optional label.
**		sc_scanhost()                   - Scan host code.
**		sc_eatcomment()                 - Eat host comment.
**		sc_inhostsc()                   - Is line within a host
**						  string or a host comment?
** Uses:
-*		lex_strbackup()			- To backup characters.
** Notes:
**		This file is pulled in indirectly via esqyylex.c.
**
** History:	17-Jul-85	Written	for esqsclang.c (mrw)
** 		20-Aug-85	Extracted out C dependent code (ncg)
** 		22-Nov-85	Modified for PL/I (ncg)
** 		01-Apr-86	Modified for ADA (ncg)
**		13-feb-93	Modified for flexible placement of SQL
**				statements. (teresal)		
**	26-aug-93 (dianeh)
**		Added #include of <st.h>.
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
**	none
** Return Values:
-*	i4	- TRUE iff the line is a comment line.
** Notes:
**	Just scan through the line.
*/

i4
sc_iscomment( void )
{
    return FALSE;	/* ADA never has a special comment line */
			/* Comments can begin anywhere          */
}

/*
+* Procedure:	sc_newline
** Purpose:	Process end-of-line (language dependent).
** Parameters:
**	None.
** Return Values:
-*	0 if this newline implies statement termination, 1 if it doesn't.
** Notes:
**	We will be in EXEC mode when called.
**	The first char of the line has already been read.
*/

i4
sc_newline()
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
**	If we were at a label eat it, give it to our caller,
**	and return pointer to next char.
**	Otherwise return our argument.
*/

char *
sc_label( lbl )
char	**lbl;
{
    static char		buf[SC_WORDMAX + 5]; 	/* Label + << + >> + null */
    register char	*p = buf;
    register char	*cp = SC_PTR;
    char		*old_cp = cp;
    bool		fullbuf = FALSE;  /* TRUE if label bigger than buf */

    *lbl = '\0';	/* default */
    /* Look for a label: "^{white}<<{white}alpha{alnum}{white}>>" */
    while (CMspace(cp) || *cp == '\t')
	CMnext(cp);
    if (*cp == '<' && *(cp+1) == '<')
    {
	*p++ = *cp++;	/* < */
	*p++ = *cp++;	/* < */
	while (CMspace(cp) || *cp == '\t')
	    CMnext(cp);
	if (CMnmstart(cp))
	{
	    CMcpyinc(cp, p);
	    while (CMnmchar(cp) && p < &buf[SC_WORDMAX])
		CMcpyinc(cp, p);
	  /* label too large for buffer; skip remaining chars */
	    if (p >= &buf[SC_WORDMAX] && CMnmchar(cp))
	    {
		fullbuf = TRUE;
		while (CMnmchar(cp))
		    CMnext(cp);
	    }
	    while (CMspace(cp) || *cp == '\t')
		CMnext(cp);
	    if (*cp == '>' && *(cp+1) == '>')		/* label! */
	    {
		*p++ = *cp++;	/* > */
		*p++ = *cp++;	/* > */
		*p++ = '\0';
		*lbl = buf;
		if (fullbuf)
		    er_write(E_EQ0216_scWRDLONG, EQ_ERROR, 2, SC_WORDMAX, buf);
		return cp;		/* Resumption place for caller */
	    } /* if ends with ">>" */
	} /* if label name starts legally */
    } /* if starts with "<<" */
    return SC_PTR;			/* No label to skip */
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
**	ADA, C, PASCAL, and PL/I do nothing.
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
**	ADA strings can never be continued over lines.
*/

i4
scStrIsCont(char notused)
{
    return FALSE;
}

static bool in_comment = FALSE;
static bool in_hoststring = FALSE;

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
**		SQL statements within Ada host code. FIPS requires that
**		SQL statements can be anywhere a Ada statement can be.
**		For example, Ada statements and SQL statements can be mixed on
**		a single line, like so:
**
**			if (cond1 = 1) then EXEC SQL COMMIT;
**			elsif (cond1 = 2) then EXEC SQL ROLLBACK;
**			else EXEC SQL INSERT INTO t1 
**						VALUES ('test');
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
**			found one Ada statement or we were just scanning
**			for comments and strings.
** Notes:
**	Called from the ESQL scanner.
**	Resets SC_PTR
**
**	Host Language Constructs
**	======================== 
**	Ada string delimitor is the double quote
**	Ada terminator is the semicolon: ;
**		However, in Ada, new statements can also follow the
**			following Ada keywords:
**				then
**				else
**				loop
**				begin
**				do
**				=>
**
**		Thus, this routine treats the above Ada keywords as 
**		terminators. If one of the above keywords is found in
**		host code, it is treated like a terminator, 
**		and the line is scanned to look for another statement.
**	
**		Ada statements are allowed in the following places:
**		
**		1. After the terminator:
**			statement; {statement;}
**		2. Within Ada Compound Statements
**		   a. If Statement:
**			if condition then statement; {statement;}
**			elsif condition then statement; {statement;}
**			else statement; {statement;}
**			end if;
**		   b. Case Statement:
**			case expression is
**				when choice => statement; {statement;}
**				when choice => statement; {statement;}
**				when others => statement; {statement;}
**				end case;
**		   c. Loop Statement:
**			loop statement; {statement;}
**			end loop;
**		   d. Block Statement:
**			blockname:
**				begin statement; {statement;}
**			end blockname;
**		   e. Accept Statement
**			accept entry_name do statement; {statement;}
**			end;
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within Ada host code as required by 
**			FIPS.
**
*/

static char	*ada_term[] =
{
		ERx("begin"),
		ERx("do"),
		ERx("else"),
		ERx("loop"),
		ERx("then"),
		ERx(";"),
		ERx("=>"),
		(char *)0
};

bool
sc_scanhost(i4 scan_term)
{
    char 	*cp;	
    char	**term_ptr;
    char	save_char;
    i4		len;
    bool	found_term = FALSE;

    cp = SC_PTR;
    save_char = '\0';

    /* Scan the host code, look for comments, strings and terminator */

    while (*cp != '\n' && *cp != SC_EOFCH)
    {
	if (sc_eatcomment(&cp, 0))	/* Are we in a host comment? */
	    continue;			/* If so, eat it and continue */
	else if (*cp == '"')
	{
            if (in_hoststring)
            	in_hoststring = FALSE;
	    /* Make sure quote is not a character literal */
            else if (!(save_char == '\'' && *(cp+1) == '\''))
            	in_hoststring = TRUE;
	}
	else	 
	{
	    /* Look for Ada Terminator */	
	    for (term_ptr = ada_term; *term_ptr != NULL; ++term_ptr)
	    {
            	len = STlength(*term_ptr);		  	
		if (STbcompare(*term_ptr, 0, cp, len, TRUE) == 0)
		{
            	    if (!in_comment && !in_hoststring)
		    {
			cp = cp + (len-1);		/* Found Term */
			found_term = TRUE;
		    }
		break;
		}
	    }	
	}
	save_char = *cp;
	CMnext(cp);
	if (scan_term && found_term)
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
	else if (sc_eatcomment(&cp, 0))
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
**		eat it up till EOL. This routine is used to eat Ada comments
**		while parsing host code or at the end of SQL statements. 
**
**		Ada comments after host code will be generated to the
**		output file while Ada comments after SQL statements
**		are not.
**
** Parameters:
**	char **cp	Pointer to input string. 
**	bool eatall	Eat past the newline?
**			Ada comments stop at the newline so this flag is
**			not applicable for Ada.
**
** Return Values:
**	i4 - TRUE 	A host comment was found and eaten;
**			cp now points to the newline.
**	      FALSE	This is not a host comment; cp has
**			not been changed.
** Notes:
**
**	Host Language constructs
**	======================== 
**	Ada comments delimiter is: '--' until the end of the line.
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within Ada host code as required by 
**			FIPS.
*/

bool
sc_eatcomment (char **cp, bool eatall)
{
    /*	
    ** Check if this is the start of a comment (and make sure we
    ** are not in a host string. If so eat until the newline.
    */
    if ((**cp == '-') && (*(*cp+1) == '-') && !in_hoststring)
    {
	CMnext (*cp);
	CMnext (*cp);
	in_comment = TRUE;	
    }	
    else 
	return FALSE;
	
    while (**cp != '\n' && **cp != SC_EOFCH)
    {
	CMnext(*cp);
    }	
    in_comment = FALSE;
    return TRUE;
}

/*
** Procedure:	sc_inhostsc
** Purpose:	Determine if input line is within a multi-line host comment
**		or string. This routine is a no-op for Ada because Ada
**		comments and string literals cannot be continued over
**		multiple lines - therefore, always return FALSE.
**
** Parameters:
**	None
**
** Return Values:
**	      FALSE	Ada doesn't support multi-line comments or strings.
**
**
** History:
**	11-feb-1993	(teresal)
**			Written to support flexible placement of SQL 
**			statements within Ada host code as required by 
**			FIPS.
*/

bool
sc_inhostsc(void)
{
    return FALSE;
}
