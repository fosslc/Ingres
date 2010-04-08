# include	<compat.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqlang.h>
# include       <eqsym.h>
# include       <eqgen.h>

/*
+* Filename:	SCLEXUTIL.C
** Purpose:	Lexical utilities.
**
-* Defines:	lex_isempty()			- TRUE iff input line is empty.
**
** History:	04-dec-1984	Written	(mrw)
**		20-may-1987	Modified for kanji/new scanner (mrw)
**              18-mar-1996 (thoda04)
**                  Added #include eqgen.h for out_emit function prototype.
**                  Added #include eqsym.h for eqgen.h.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
+* Procedure:	lex_isempty
** Purpose:	Is the input line empty.
**
** Parameters:
**		lex_hostcode - i4	- Was last line host code?
** Returns:	bool 	     - 	TRUE iff the current line is empty, has only
**				whitespace, or is a FORTRAN or COBOL comment
-*				immediately following EQUEL code.
** Notes:
**	SC_PTR is positioned at beginning of line.
*/

bool
lex_isempty( lex_hostcode )
i4	lex_hostcode;
{
    register char	*p = SC_PTR;
    i4			emit_empty;

    emit_empty = eq_islang(EQ_C) && eq->eq_flags & EQ_SETLINE;

    /*
    ** Strip FORTRAN and COBOL comments in EQUEL (for VMS compatibility)
    ** if we have just read EQUEL code, as they may come out embedded in EQUEL
    ** generated code.
    **
    ** ESQL always wants to strip comments (if dml->dm_comment says to check);
    ** this will be true only for FORTRAN and COBOL (currently).
    */
    if (dml_islang(DML_EQUEL))
    {
	if (    (!lex_hostcode)
	     && (    (   eq_islang(EQ_FORTRAN)
		      && (*p=='c' || *p=='C' || *p=='!' || *p=='*')
		     )
		  || 
		     (   eq_islang(EQ_COBOL)
		      && (*p=='*')
		     )
		)
	    )

	{
	    /* Ignore the current input line - and go to next line */
	    return TRUE;
	}
    } else if (!lex_hostcode && dml->dm_iscomment && (*dml->dm_iscomment)())
    {
	if (emit_empty)
	    out_emit( p );
	/* Ignore the current input line - and go to next line */
	return TRUE;
    }

  /* May be a line of blanks and tabs */
    for (; CMwhite(p) && *p != '\n'; CMnext(p));
    if (*p != '\n')
	return FALSE;
    if (emit_empty)
	out_emit( p );
    return TRUE;
}
