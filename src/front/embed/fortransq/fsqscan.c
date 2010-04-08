# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqscsql.h>


/*
+* Filename:	forsqscan.c
** Purpose:	Language-dependent lexical utilities.
**
** Defines:	sc_iscomment()			- Is this line a comment?
**		sc_newline()			- Process end-of-line.
**		sc_label()			- Scan an optional label.
**		sc_skipmargin()			- Applicable only for card fmt.
**		scStrIsCont()			- Is this string cont. line?
**		sc_scanhost()             	- Scan host code: N/A in FORTRAN
**      	sc_eatcomment()           	- Eat comment: N/A in FORTRAN
**      	sc_inhostsc()             	- N/A for FORTRAN
-* Locals:	sc_cont()			- The guts of check for cont.
**
** Notes:
**		This file is based on the template esqsclang.c.
**		The modifications reflect the FORTRAN syntax and format rules
**		that we follow in scanning embedded statements.
**
** History:	17-Jul-85	Written	(mrw)
**		05-May-86	Modified (bjb)
**		25-jun-87	Updated for 6.0 (bjb)
**		16-feb-93	As part of flexible placement of SQL
**				statements changes, added sc_scanhost(),
**				sc_eatcomment(), and sc_inhostsc()
**				routines to this file. Note: since
**				ESQL/FORTRAN doesn't support flexible
**				placement of SQL statements, these
**				routines are no-ops. (teresal)
**		26-Aug-1993 (fredv) included <st.h>.
**		23-apr-1996 (thoda04)
**		    Define sc_skipmargin() as returning i4, the same as all the
**		    other languages, so that the function prototype will not flag it.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* 
** Allow testing of VMS preprocessor to be done on Unix.
*/
# ifdef VMS_TEST_ON_UNIX
# undef UNIX
# define VMS
# endif /* VMS_TEST_ON_UNIX */

/*
+* Procedure:	sc_iscomment
** Purpose:	Determine if a line is a comment line (FORTRAN dependent).
** Parameters:
**	None
** Return Values:
-*	i4	- TRUE iff the line is a comment line.
** Notes:
**	Check if comment indicator present in first position of line.
**	Assume that SC_PTR is pointing to first position of line.
**	Comment indicator may be 'C', 'c', '!', or '*'.  Position of
**	comment indicator is same for both tab and card formats.
**	Note that for tab format VMS rule is that '!' may be in any 
**	position on line; however, this routine doesn't check for that
**	type of comment.
*/

i4
sc_iscomment()
{
    register char	*cp = SC_PTR;	

    if (*cp == 'C' || *cp == 'c' || *cp == '!' || *cp == '*')
	return TRUE;
    return FALSE;
}

/*
+* Procedure:	sc_newline
** Purpose:	Process end-of-line (FORTRAN dependent).
** Parameters:
**	None
** Return Values:
**	TRUE -  This line is continued
-*	FALSE - This line is not continued
** Notes:
**	We are called from the scanner (sc_eqishost()).  If the last
**	line is not continued, we return FALSE which will cause the
**	scanner to push back a ';' statement terminator so that the
**	parser can reduce the last SQL statement.  (Most other languages
**	have a statement terminator and always return TRUE.)
**
** Imports modified:
**	Nothing.
*/

i4
sc_newline()
{
    bool	sc_cont();
    char	*cp;

    cp = SC_PTR;	
    if (sc_cont( &cp ))
    {
	SC_PTR = cp;
        return TRUE;
    }
    else
	return FALSE;
}

/*
+* Procedure:	sc_label
** Purpose:	Scan for a label (at start of line)
** Parameters:
**	lbl	- char **	- Pointer to label
** Return Values:
-*	char * - Pointer into line buffer at which to resume scanning
** Notes:
**	If we were at a label scan it, give it to our caller,
**	and return pointer to next char.
**	Otherwise return SC_PTR.
**	This routine currently scans labels in VMS-tab format and F77 format.
**	i.e, <= 5 char positions at the beginning of a line followed by a tab
**	(VMS) or a tab/space (F77).  
**	Initial spaces are accepted but they contribute towards the count.
**	Labels are composed of digits.
**
** Imports modified:
**	None.
*/

char *
sc_label(lbl)
char		**lbl;
{
    static char		buf[ SC_WORDMAX ];
    register char	*p = buf;
    register char	*cp, *old_cp;
    i4			i;

    *lbl = NULL;	/* default */
    cp = old_cp = SC_PTR;

    if (*cp == '\t')	/* No tabs before labels */
	return old_cp;

    i = 0;
    while (i < 4 && CMspace(cp))	/* Allow up to 4 columns of space */
    {
	CMbyteinc(i, cp);
	CMcpyinc(cp, p);
    }
    if (i > 4)				/* 4th space might have been Kanji */
	return old_cp;

  /* look for a label: "^digit{digit}+" */
    if (!CMdigit(cp))
	return old_cp;
    do {
	CMcpyinc(cp, p);
	i++;
    } while (CMdigit(cp) && i < 5);

    if (*cp == '\t' || CMspace(cp))  /* Space or tab must follow last digit */
    {
# ifndef UNIX
	STcopy( ERx("\tcontinue"), p );    /* A statement must follow label */
# else
	/* A statement in card format must follow label (daveb) */
	STcopy( ERx("      continue") + i, p );
# endif
	*lbl = buf;
	CMnext(cp);
	return cp;
    }
    return old_cp;

}

/*
+* Procedure:	sc_skipmargin
** Purpose:	Skip the margin on a new line.
** Parameters:
**	None.
** Return Values:
-*	Nothing.
** Notes:
**	This routine is currently called from scanner when we are on a new 
**	line in DML_DCL mode.  If this is a continued line, it advances
**	SC_PTR to skip the margin (tab format) and the line continuation 
**	indicator.  It assumes that SC_PTR points to the first character on
**	the line.
**
** Imports modified:
**	SC_PTR may be incremented.
*/

i4
sc_skipmargin()
{
    bool	sc_cont();
    char	*cp;

    cp = SC_PTR;

    if (sc_cont( &cp ))
    {
	SC_PTR = cp;
    }
    return (0);
}

/*
+* Procedure:	scStrIsCont
** Purpose:	Determine if the current line is a continued string.
** Parameters:
**	None.
** Return Values:
**	1 - Current line is a continuation line
-*	0 - Current line is not a continuation line
**
** Notes:
**	This routine, called only from sc_string, determines if the current
**	line is a continuation line.  It uses sc_cont() to do the detailed
**	checking (see below).
**
** Imports modified
**	The scanner's global input pointer, SC_PTR, will be modified
**	if this is a continuation line.
*/

i4
scStrIsCont(delim)
char	delim;
{
    char	*cp;
    bool	sc_cont();

    cp = SC_PTR;

    if (sc_cont( &cp ))
    {
	SC_PTR = cp;
        return TRUE;
    }
    else
	return FALSE;
}

/*{
+*  Name: sc_cont - Return true if line is a valid FORTRAN continuation line.
**
**  Description:
**	VMS:  Check for line continuation in ANSI format and VMS Tab format.
**	      VMS tab format continuation consists of an initial tab
**	      followed by a non-zero digit.  Note that an initial tab
**	      may be preceded by up to five spaces.
**	      ANSI format continuation consists of any character
**	      except zero or space in column 6.
**	Unix: 
**	      A continuation line is 1) a line with an ampersand (&) in the
**	      first position or 2) a line containing any char except blank
**	      or zero in column 6 (strictly speaking, columns 1-5 should be
**	      blank but we'll let the compiler flag errors).
**
**  Inputs:
**	cp		- (Indirect) pointer to place to start scanning.
**
**  Outputs:
**	cp 		- Will be incremented if this is a continuation line.
**	Returns:
**	    TRUE	- This is a continuation line 
**	    FALSE	- This is not a continuation line.
**	Errors:
**	    <error codes and description>
-*
**  Side Effects:
**	None
**	
**  History:
**	23-oct-1986 - written (cmr)
**	07-nov-1986 - revised for VMS to fix sir 10678 (bcw)
**	02-jul-1987 - updated for 6.0 (bjb)
**	19-apr-1990 - Fix for VMS bug 21264. If an initial tab preceded by 5
**		      spaces is NOT followed by a non-zero digit, return that
**		      this is not a continuation line. (teresal)
*/

bool
sc_cont( cp )
register char	**cp;
{
    register char 	*c = *cp;
    i4		i = 0;
# ifdef UNIX

    if ( c[0] == '&' )
    {
	CMnext(c);
	*cp = c;
	return TRUE;
    }

    while (i < 5 && CMspace(c))
    {
	CMbyteinc(i, c);
	CMnext(c);
    }
    if (i != 5)
	return FALSE;

    /* At this point columns 1 thru 5 are blank -- now we must check
    ** column 6 for a valid continuation marker.
    */
    if ( CMalpha(c) || (CMdigit(c) && *c != '0') || CMoper(c) )
    {
	CMnext(c);
	*cp = c;
	return TRUE;
    }
    return FALSE;
# else
    while (i < 5 && CMspace(c))		/* Skip up to 5 cols of spaces */
    {
	CMbyteinc(i, c);
	CMnext(c);
    }
    if (i > 5)
	return FALSE;

    if (*c == '\t') 
    {
	c++;
	if (*c >= '1' && *c <= '9')	/* Tab fmt continuation indicator */
	{
	    c++;
	    *cp = c;
	    return TRUE;
	}
	else
	/*
	** Not a valid tab format contination line.  Return FALSE here to
	** avoid falling through to the ANSI format continuation checking and
	** incorrectly returning TRUE if the statement contains a tab in column
	** 6. Bug 21264.
	*/
	{
	    return FALSE;
	}
    }

    if (i == 5 && !CMspace(c) && *c != '0')	/* ANSI format continuation */
    {
	c++;
	*cp = c;
	return TRUE;
    }
    return FALSE;
# endif /* UNIX */
}

/*
** Procedure:   sc_scanhost
** Purpose:     Do some crude parsing of host code to support flexible
**              placement of SQL code in Host code.
**
**	This routine parses host code to determine whether or not an SQL stmt
**	is on a line that begins with host code.  In the case of Fortran, this
**	will only occur in a logical IF stmt. (IF (x .eq. y) EXEC SQL ...)
**	The scanner parses files a line at a time.  This routine must parse 
**	a logical stmt, which may be split up in any fashion with 
**	continuation lines and comments.  
**		IF (x .eq. 'this(
**      C This is a comment
**	     &is a string)'
**	     &  )
**           & EXEC SQL ....
**
**	This routine keeps several statics that tell us where in the line we 
**	are. (Have we seen the "IF", are we in a host string, etc..)
**
**	1) If this is not a continuation line check for keyword "IF" and
**	   set in_hostif appropriately.
**      2) If this is a continuation line, and in_hostif is set then continue
**	   to parse the line for SQL statement.
**	We parse continuation lines until we have seen the outermost set of 
**	parens.  If anything other than white space or an  SQL stmt follows 
**	the parens we return false and reset the "in_hostif" variable to
**	FALSE.
**
**	Since only one executable stmt is allowed in a Fortran logical IF
**	stmt, and SQL stmts may generate several host language calls, when a
**	an SQL stmt appears in a "logical IF", we must generate a "block IF"
**	stmt.  This consists of adding the fortran keyword "then" before we
**	generate any code and an "ENDIF" after we have parsed and generated 
**	the code for the SQL stmt.  
**
**	When this routine finds an SQL stmt in a Fortran logical IF stmt, it
**	will return TRUE to yyesqlex which will add the the keyword "then"
**	to the host buffer and generate the line. This routine will update 
**	SC_PTR so that the next stmt processed will be the SQL stmt, and will
**	set a flag to indicate that an "END IF" should be generated after the
**      completion of this SQL stmt.
**
** Parameters:
**      N/A
**
** Return Values:
**	      TRUE	This line contains an SQL stmt and in logical IF.
**            FALSE     This is not a logical IF that contains SQL stmts.
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
**	10-jun-1993 (kathryn)
**	    Add functionality to support flexible placement of SQL stmts
**	    within host code for FIPS.  
**	25-jun-1993 (kathryn)
**	    Use CMnext rather than cp++. This is fortran IF stmt, but may
**	    contain host character string which could be double byte.
*/

static bool in_hostif = FALSE;

bool
sc_scanhost(i4 scan_term)
{
    char	*cp; 			/* ptr to next char to scan */
    char	*saveptr; 		/* ptr to save start of EXEC SQL */
    char	*label; 		/* label for this host line */
    bool	iscont = FALSE;		/* is this a continuation line */
    static i4	parens = 0;		/* number of parens scanned so far */
    static bool	need_paren = TRUE;	/* looking for first paren */
    static bool	in_hoststring = FALSE;	/* are we in a host string? */

    cp = SC_PTR;

    /* If this is not an IF statement then skip the whole thing.
    ** In Fortran, SQL statements are only allowed on the same line as
    ** host code if the statement is an IF statement.
    */

    /* skip comment lines */
    if (sc_iscomment())
	return FALSE;

    /* cp points to char following continuation character */
    if (sc_cont(&cp))
    {
	if (!in_hostif)		
	    return FALSE;
        else
	    iscont = TRUE;
    }
    else
    {
	/* Labels allowed on If statements skip the label */
	cp = sc_label(&label);

       /* This is a new host statement - Want to clean up the state just in
       ** case there were errors on the last host IF statement.
       */
	in_hoststring = FALSE;
	parens = 0;
        in_hostif = FALSE;
        need_paren = FALSE;
    }

    while (CMwhite(cp) && (*cp != '\n'))
	CMnext(cp);

    if (!iscont)
    {
	/* This is a new statement - Check to see if its an IF stmt 
	** If not an IF stmt then just return
	*/

        if ((STlength(cp) > 2) && (STbcompare(cp, 2, ERx("if"), 2, TRUE) == 0))
        {
	    in_hostif = TRUE;
	    need_paren = TRUE;
	    cp += 2;
	    while (CMwhite(cp)&& (*cp != '\n'))
		CMnext(cp);
	}
	/* This is not a Fortran If statement no need to check any further */
	else
	{
	    in_hostif = FALSE;
	    return FALSE;
	}
    }

    if (sc_eatcomment(&cp,FALSE))
	return FALSE;

    /* We just skipped all the white space.  If we are not at the end of line
    ** then the next token must be the first left paren or this is
    ** a host code error.
    */
    if (need_paren && *cp != '\n')
    {
	if (*cp != '(')
	{
	    /* Not a valid fortran IF statement return and let the
	    ** compiler handle the host code error 
	    */
	    in_hostif = FALSE;
	    return FALSE;
	}
	else
	{
	    parens++;
	    cp++;
	    need_paren = FALSE;
	}
    }

    /* We have found the IF and the first left paren "IF (".
    ** Scan through the host statement until we find the matching (final)
    ** right paren or until end of line. 
    */

    while ((parens != 0) && (*cp != '\n') && (*cp != SC_EOFCH) )
    {
	    if (*cp == '\'')
	        in_hoststring = !in_hoststring;
	    if (!in_hoststring)
	    {
	        if (*cp == '(')
		    parens++;
	        else if (*cp == ')')
		    parens--;
		if (sc_eatcomment(&cp,FALSE))
		    return FALSE;
	    }
	    CMnext(cp);
    }

    if ((parens == 0) && (*cp != '\n'))
    {
	while (CMwhite(cp)&& (*cp != '\n'))
	    CMnext(cp);
	if (sc_eatcomment(&cp,FALSE))
	    return FALSE;
	saveptr = cp;

	/* Call sc_execstmt to determine if this is an embedded statement:
	** 	EXEC SQL, EXEC FRS, EXEC 4GL.
	*/

	if (sc_execstmt(&cp) != SC_EOF)
	{
	    eq->eq_flags |= EQ_ENDIF;
	    SC_PTR = saveptr;
	    return TRUE;
	} 
 	/* We found something after IF() which is not exec
	** This is host code only, so return false.
	*/
	else if ((saveptr != cp) || (*cp != '\n'))
	    in_hostif = FALSE;
    } 
    return FALSE;
}

/*
** Procedure:   sc_eatcomment
** Purpose:     Determine if we have an in-line host comment. 
**              This routine is needed to support flexible placement of SQL 
**		statements.
**
** Parameters:
**      N/A
**
** Return Values:
**            FALSE     This is not a in-line host comment.
**	      TRUE	This begins an in-line host comment -- the rest
**			of the line is a host comment.
** Notes:
**   1)  If your compiler allows comments within a fortran "logical IF" stmt:
**                      if (x .eq. y)  !This is a comment
**                    & executable statement
**       Then the comment delimiter must be added here to allow the
**       executable statment to be an SQL stmt:
**                      if (x .eq. y)  !comment
**                    & exec sql commit
**   2)  Use sc_iscomment() to check for full-line comments.
**       (lines beginning with '*','C', 'c ', etc...)
**       This routine should check for in-line comments only.
**   3)  Curerntly this routine is only called from sc_scanhost when we
**       are sure that we are scanning a fortran "logical IF" statement.
**       Since UNIX does not allow comments within a logical IF stmt, UNIX
**       will always return FALSE. (it is a host coding error)
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
**	10-jun-1993  (kathryn)
**	    Change from dummy routine to add support for flexible placement of
**	    SQL statements in Fortran.  SQL statements are now allowed in
**	    fortran "logical if" statements.  VMS allows in-line comments.
*/

bool
sc_eatcomment (char **cp, bool eatall)
{
# ifdef VMS
    if (**cp == '!')
	 return TRUE;
#endif
    return FALSE;
}

/*
** Procedure:   sc_inhostsc 
** Purpose:     
**	Determine if this line is the continuation of a fortran IF statement.  
**	This is called from the ESQL scanner to determine if it should process
**	the current line as host code or look for EXEC SQL.
**	When we are processing an IF stmt we want sc_scanhost to find the
**	the SQL statement so that a block IF will be generated.
**		if (x .eq. y)	
**	     &  exec sql commit
**	generates:
**	  	if (x .eq. y)
**	     &  then
**		    sql stmts...
**	        endif
**
**	We are also attempting to catch the obscure case where an embedded stmt
**	may be in a literal string in the the IF stmt and should not generate
**	any SQL code.
**		if (x .eq. 'stmt is
**	     &  exec sql')
**
** Parameters:
**      None
**
** Return Values:
**      TRUE/FALSE     Is this line a continuation line of a Fortran IF stmt.
**
** History:
**      11-feb-1993     (teresal)
**                      Written to support flexible placement of SQL
**                      statements within host code as required by
**                      FIPS.
**	10-jun-1993 	(kathryn)
**          Change from dummy routine to add support for flexible placement of
**          SQL statements in Fortran.  SQL statements are now allowed in
**          fortran "logical if" statements.
*/

bool
sc_inhostsc(void)
{
	char *cp;
    cp = SC_PTR;
    if (in_hostif && sc_cont(&cp))
	return TRUE;
    return FALSE;
}
