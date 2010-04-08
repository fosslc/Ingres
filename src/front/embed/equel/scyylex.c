# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include 	<eqlang.h>
# include 	<eqscan.h>
# include 	<equel.h>
# include	<eqsym.h>	/* Because of eqgr.h */
# include	<eqgr.h>
# include	<ereq.h>

/*
+* Filename:	SCYYLEX.C
** Purpose:	Interface between grammar and scanner through YY defined 
**		routines and variables for Yacc.
**
** Defines:	yylex()		   - Scanner called by Yacc (yyparse).
**				     Calls the routine specified in _dml.
**		yyerror( str )	   - Error routine called internally by yyparse.
**		yy_dodebug()	   - Turn on/off yydebug.
**		sc_addtok( t, s )  - Locally used to set yylval.
** Uses:
**		yylval		   - Yacc stack element filled by scanner.
-*		yydebug		   - Yacc debug flag.
**
** History:	11-Dec-1984	   - Rewritten (mrw)
** 		03-May-1988	   - yylowlex removed. (whc)
**              12-Dec-95 (fanra01)
**                  Modfied all externs to GLOBALREFs
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* really a union of different pointers and values */
GLOBALREF	GR_YYSTYPE	yylval;

/*
** These two variables are defined in scrdline.c
** Used to check if we have started to process the current line.
*/
GLOBALREF i4	lex_need_input;		/* Do we need to read in a new line? */
GLOBALREF i4	lex_is_newline;		/* Do we need to process the line? */


# ifdef	YYDEBUG
    GLOBALREF i4  	yydebug;
# endif /* YYDEBUG */

/*
+* Procedure:	yylex 
** Purpose:	Scanner for Yacc. Called by yyparse when it wants a token.
**
** Parameters:	None
** Returns:	i4 - Token value
** 
** Side Effects:
-*  1. Calls the routine pointed to by dml->dm_lex.
*/

i4
yylex()
{
    return (*dml->dm_lex)(SC_COOKED);
}

/*
+* Procedure:	sc_addtok 
** Purpose:	Fill Yacc's stack element 'yylval' with correct values.
**
** Parameters:	tag 	- i4  	 - Tag into union of YYSTYPE (defined by us
**				   in eqgr.h)
**		value	- char * - Real token value.
-* Returns:	None
**
** Yacc defines the global variable 'yylval' to be of the type YYSTYPE, whose
** definition we have control over through definition in eqgr.h.  When new 
** union members are added to this type, we should add the corresponding tags 
** here.  
** This routine is an interface between the scanner and yylval, and has no
** effect on what happens to the values in Yacc's stack when rules are reduced.
** Thus we only handle the types that may be coming from the scanner. Currently
** all 'yylval' members coming from the scanner are (char *).
*/

void
sc_addtok( tag, value )
i4	tag;
char	*value;
{
    switch (tag)
    {
      case SC_YYSTR:
	break;
    }
    yylval.s = value;
}

/*
+* Procedure:	yyerror 
** Purpose:	Internal Yacc error message.
**
** Parameters:	emsg - char * - String parameter: "syntax error" or "overflow".
-* Returns:	None
**
** The error message should be either "syntax error" or "overflow error".
** To be sure we can just search for an 'x'. These strings may become 
** integer constants on VMS, so this may have to ifdeff'ed.  The routines
** uses 'yylval' to determine the token in use at the time a syntax error
** ocurred.  If a syntax error has occurred, then Yacc has probably just
** eaten a terminal.
*/

void
yyerror( emsg )
char	*emsg;
{
# ifndef YACC_NEW
    /* 
    ** The name 'yychar' may not be standard across all systems and may
    ** have to be changed to be 'yytoken'.
    */
#  define	yytoken	yychar
# endif	/* YACC_NEW */

    GLOBALREF  i4   yytoken;

    if (STindex(emsg, ERx("x"), (i2)0) == (char *)0)		/* Syntax */
    {
	er_write( E_EQ0240_yyOVFL, EQ_FATAL, 0 );
	return;
    }

    if (yytoken == tok_special[TOK_CODE])
    {
	er_write( E_EQ0242_yySYNHOST, EQ_ERROR, 0 );
	return;
    }
    if (yylval.s == (char *)0)
    {
	er_write( E_EQ0241_yySYNTAX, EQ_ERROR, 0 );
	return;
    } 
    if (yytoken == SC_EOF)
    {
	er_write( E_EQ0245_yySYNEOF, EQ_ERROR, 0 );
	return;
    }
    if (dml_islang(DML_ESQL) && *(yylval.s)==';')
    {
	if (eq_islang(EQ_FORTRAN))
	{
	    er_write( E_EQ0243_yySYNTERM, EQ_ERROR, 1, ERx("FORTRAN") );
	    return;
	} else if (eq_islang(EQ_BASIC))
	{
	    er_write( E_EQ0243_yySYNTERM, EQ_ERROR, 1, ERx("BASIC") );
	    return;
	}
    }
    er_write( E_EQ0244_yySYNWRD, EQ_ERROR, 1, yylval.s );
}

/*
+* Procedure:	yy_dodebug 
** Purpose:	Toggle on/off yydebug flag.
**
** Parameters:	None
-* Returns:	None
*/

void
yy_dodebug()
{
# ifdef	YYDEBUG
    yydebug = 1 - yydebug;
# endif /* YYDEBUG */
}
