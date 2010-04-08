# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<ereq.h>
# include	<ere2.h>

/*
** Copyright (c) 1984, 2001 Ingres Corporation
*/

/*
+* Filename:	passqscan.c
** Purpose:	PASCAL-dependent lexical utilities.
**
** Defines:	sc_iscomment()			- Is this line a comment?
**		sc_newline()			- Process end-of-line.
**		sc_label()			- Scan an optional label.
**		sc_scanhost()			- Scan host code: N/A for PASCAL
**		sc_eatcomment()			- Eat host commt: N/A for PASCAL
**		sc_inhostsc()			- N/A for PASCAL
** Notes:
**		This file is pulled in indirectly via esqyylex.c.
**
** History:	17-Jul-85	Written	for esqsclang.c (mrw)
** 		20-Aug-85	Extracted out C dependent code (ncg)
** 		22-Nov-85	Modified for PL/I (ncg)
** 		01-Apr-86	Modified for ADA (ncg)
** 		24-Jun-86	Modified for PASCAL (ncg)
** 		03-Jun-87	Modified for 6.0 (ncg)
**		16-feb-93	As part of flexible placement of SQL
**                              statements changes, added sc_scanhost(),
**                              sc_eatcomment(), and sc_inhostsc()
**                              routines to this file. Note: since
**                              ESQL/PASCAL doesn't support flexible
**                              placement of SQL statements, these
**				routines are no-ops. (teresal)
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/*
+* Procedure:	sc_iscomment
** Purpose:	Determine if a line is a comment line.
** Parameters:
**	cp	- char *	- Pointer to the line.
** Return Values:
-*	bool	- TRUE iff the line is a comment line.
** Notes:
**	Just scan through the line.
*/

bool
sc_iscomment( cp )
char	*cp;
{
    return FALSE;	/* PASCAL never has a special comment line */
			/* Comments can begin anywhere          */
}

/*
+* Procedure:	sc_newline
** Purpose:	Process end-of-line (language dependent).
** Parameters:
**	None
** Return Values:
-*	0/1 - Not continued/Continued
** Notes:
**	We will be in EXEC mode when called.
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
**	lbl	- char *	- Pointer to label buffer
** Return Values:
-*	char * - Pointer into line buffer at which to resume scanning
** Notes:
**	If we were at a label eat it, give it to our caller,
**	and return pointer to next char.
**	Otherwise return our argument.
*/

char *
sc_label( lbl )
char		**lbl;
{
    register char	*cp = SC_PTR;
    static char		buf[SC_WORDMAX + 3]; 	/* Label + colon + Kanji */
    register char	*p = buf;
    bool		fullbuf = FALSE;  /* TRUE if label bigger than buf */

    *lbl = NULL;	/* default */
  /* look for a label: "^{white}alnum{alnum}+{white}:" */
    while (CMspace(cp) || *cp == '\t')
	CMnext(cp);
    if (CMnmstart(cp) || CMdigit(cp))
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
    return SC_PTR;			/* No label to skip */
}

/*
+* Procedure:	sc_skipmargin
** Purpose:	Skip the margin on a new line.
** Parameters:
**	None.
** Return Values:
-*	None
** Notes:
**	1. You must have already ensured that this is not a HOST_CODE line.
**	   ADA, C, PASCAL, and PL/I do nothing.
**	2. Adjust SC_PTR if skipping margin.
*/

i4
sc_skipmargin()
{
    /* No margin */
}

/*
+* Procedure:	scStrIsCont
** Purpose:	Determine if the current line is a continuation line.
** Parameters:
**	None.
** Return Values:
-*	bool - TRUE <==> this is a continuation line.
** Notes:
**	PASCAL strings can never be continued over lines.
*/

bool
scStrIsCont()
{
    return FALSE;
}

/*
** Procedure:   sc_scanhost
** Purpose:     Do some crude parsing of host code to support flexible
**              placement of SQL code in Host code.
**
**              Since this feature is not supported in ESQL/PASCAL, this
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
**      in included as part of the PASCAL Scanner routines so that the
**      ESQL/PASCAL preprocessor can be sucessfully linked at build time.
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
**              Since this feature is not supported for ESQL/PASCAL, this
**              routine is a no-op and will always return FALSE.
**
** Parameters:
**      N/A
**
** Return Values:
**            FALSE     This is not a host comment.
** Notes:
**      This routine is called from the ESQL scanner. This dummy routine
**      in included as part of the PASCAL Scanner routines so that the
**      ESQL/PASCAL preprocessor can be sucessfully linked at build time.
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
**              Since this feature is not supported for ESQL/PASCAL, this
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
**      in included as part of the PASCAL Scanner routines so that the
**      ESQL/PASCAL preprocessor can be sucessfully linked at build time.
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
