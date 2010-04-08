# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<er.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqstmt.h>
# include	<equtils.h>
# include	<eqlang.h>
# include 	<equel.h>
# include	<ereq.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	DBQUEL.C
** Purpose:	Manages all database statements that get written to the Libq 
**		runtime routines.
** 
** Defines:	db_key( kword )			- Add a keywrd to db buffer.
**		db_op( op )			- Add an operator to db buffer.
**		db_var( dbtyp, var, varname,
**			indsym, indname)	- Database call using variable.
**		db_close( func )		- Flush database call with func.
**		db_linesend()			- Send text line.
**		db_send()			- Clear out buffer for others.
**		db_attname( str, num )		- Make name [+num] an attribute.
**		db_sattname( str, num )		- Return attribute name [+num].
**		db_sconst( str )		- Add a db string constant.
**		db_hexconst( str )		- Add a db hex constant.
**		db_uconst( str )		- Add a db Unicode literal.
**		form_sconst( str )		- Generate non-db string const.
** Locals:
**		db_add( str )			- Add data to database buffer.
-*		db_write()			- Contents of buffer to IIwrite.
** Notes:
**  1. The modules defined in this file all interact with Quel statements.
**     Most of them are obvious, as to how they are used, but some, 
**     like db_send() may be used externally too - see repeat.c as to how 
**     it uses db_send() before it switches output states.
**  2. The routine form_sconst() is also here, just because it is the only
**     extra utility for non-db statements.  Observe the different flag that
**     is sent to gen_makesconst() by db_sconst() and form_sconst().
**
** History:	14-dec-1984	- Written. (ncg)
**		02-aug-1985	- Added db_sattname and made db_add work for
**				  any length string. Added some SQL support
**				  for db functions. (ncg)
**	16-nov-1992 (lan)
**		Added new parameter, arg, to db_var.
**	19-nov-1992 (lan)
**		When the datahandler argument name is null, generate (char *)0.
**	08-dec-1992 (lan)
**		Removed redundant code in db_var.
**	14-dec-1992 (lan)
**		Added db_delimid to support delimited identifiers.
**	22-dec-1993 (teresal)
**		Add support for the "-check_eos" flag. Bug fix 55915.
**	13-mar-1996 (thoda04)
**		Added function parameter prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define DB_LINEMAX	255

/* Local bufferer of data to be sent in an IIwritedb call */
static	char	db_buf[ DB_LINEMAX + 1 ] ZERO_FILL;
static	i4	db_cnt = 0;

/* Routines to manage raw input and output to and from the database buffer */
static	void	db_add(char *str);
static	void	db_write(void);

/* Flag if a space is needed before or after a word - keyword/operator */
# define	DB_OP		0			/* No space */
# define	DB_KEY		1			/* Space required */

static	i4	db_last = DB_OP;

/*
+* Procedure:	db_key 
** Purpose:	Add a keyword to the database buffer.
**
** Parameters:	kword - char * - Word to add.
-* Returns:	None
**
** This routine will make sure that the next word added (if not an operator)
** will always be preceded by a blank.  Thus not only keywords are added via
** this routine, but also general words and numbers that will have to be
** separated by a blank at runtime.
*/

void
db_key( kword )
char	*kword;
{
    if (db_last == DB_KEY)
	db_add( ERx(" ") );
    db_add( kword );
    db_last = DB_KEY;
}

/*
+* Procedure:	db_op 
** Purpose:	Add an operator to the database buffer.
**
** Parameters:	op - char * - Operator to add.
-* Returns:	None
**
** As operators are always considered delimiters within any string sequence
** they do not need to be preceded or followed by a blank.
*/

void
db_op( op )
char	*op;
{
    db_add( op );
    db_last = DB_OP;
}

/*
+* Procedure:	db_var 
** Purpose:	Send a variable to the database.
**
** Parameters:	dbtyp  - i4	- DB type - Id, Notrim, nullable or Regular.
**		var    - SYM *	_ Variable used with db statement.
**		varname- char *	- Fully qualified name of variable.
**		indsym - SYM *  - Indicator variable used with data variable
**		indname- char * - Name of indicator variable
**		arg    - char *	- Datahandler argument name.
-* Returns:	None
**
** Indicator variables are only allowed syntactically in conjunction with
** user variables.  Because of this syntax, the incoming indicator arguments
** are useful only when the incoming flag is DB_NOTRIM or DB_REG.
** In turn, we pass information about the null indicator to the argument 
** manager only in those cases where the code generator is expecting an extra 
** argument (IISETDOM, IINOTRIM and IIVARSTRING).
*/

void
db_var( dbtyp, var, varname, indsym, indname, arg )
i4	dbtyp;
SYM	*var;
char	*varname;
SYM	*indsym;
char	*indname;
char	*arg;
{
    if (db_last == DB_KEY)
	db_add( ERx(" ") );
    db_send();
    /* Add the argument and make the call */
    switch (dbtyp)
    {
      case DB_ID:
	if (eq->eq_flags & EQ_CHREOS)	/* If "-check_eos" generate IIsqMod() */
	   esq_eoscall(IImodCEOS);	
	arg_int_add( TRUE );		/* Trim most blanks at run-time */
	arg_int_add( 0 );		/* Null indicator */
	arg_var_add( var, varname );
	gen_call( IIWRITEDB );
	db_last = DB_KEY;
	break;

      case DB_NOTRIM:
	if (eq->eq_flags & EQ_CHREOS)	/* If "-check_eos" generate IIsqMod() */
	   esq_eoscall(IImodCEOS);	
	if (indsym != (SYM *)0)
	    arg_var_add( indsym, indname );
	else
	    arg_int_add( 0 );
	arg_var_add( var, varname );
	gen_call( IINOTRIM );		
	db_last = DB_OP;
	/* db_add( ERx(" ") );	Added for backend scanner bug.  Fix it now! */
	break;

      case DB_STRING:
	/* 
	** No null indicator expected, but gen_io expects the 
	** extra argument, so we dump it.
	*/
	if (eq->eq_flags & EQ_CHREOS)	/* If "-check_eos" generate IIsqMod() */
	   esq_eoscall(IImodCEOS);	
	arg_int_add( 0 );
	arg_var_add( var, varname );
	gen_call( IIVARSTRING );
	db_last = DB_OP;
	break;

      case DB_HDLR:		/* Datahandler */
	arg_int_add( 2 );
	arg_var_add( indsym, indname );
	arg_str_add( ARG_RAW, varname );
	arg_str_add( ARG_RAW, arg );
	gen_call( IIDATAHDLR );
	break;

      case DB_REG:
      default:
	if (eq->eq_flags & EQ_CHREOS)	/* If "-check_eos" generate IIsqMod() */
	   esq_eoscall(IImodCEOS);	
	if (indsym != (SYM *)0)
	    arg_var_add( indsym, indname );
	else
	    arg_int_add( 0 );
	arg_var_add( var, varname );
	gen_call( IISETDOM );
	db_last = DB_OP;
	/* db_add( ERx(" ") );	Added for backend scanner bug.  Fix it now! */
	break;
    }
}


/*
+* Procedure:	db_close 
** Purpose:	Close off the database statement, and send runtime sync-up
**	      	call.
**
** Parameters:	func - i4  - Runtime function to call.
-* Returns:	None
*/

void
db_close( func )
i4	func;
{
    db_send();
    gen_call( func );
}

/*
+* Procedure:	db_send 
** Purpose:	Make sure everything inside database buffer is sent out.
**
** Parameters:	None
-* Returns:	None
**
** This routine is called by various functions that require any database
** strings be sent before they can make their own calls.
*/

void
db_send()
{
    if (db_cnt > 0)
	db_write();
    db_last = DB_OP;
}

/*
+* Procedure:	db_linesend 
** Purpose:	Strip trailing whitespace from the buffer, write it out,
**		generate an II*PutCtrl call.
**
** Parameters:	None
-* Returns:	None
**
** This routine is called by sc_tokeat, which wishes to place newlines in
** the GCA buffer and, having maintained whitespace, wants to strip the
** trailing whitespace.
*/

void
db_linesend()
{
    db_cnt = STtrmwhite(db_buf);
    if (db_cnt > 0)
    {
	db_send();
	arg_int_add(II_cNEWLINE);
	gen_call( IIPUTCTRL );
    }
}

/*
+* Procedure:	db_attname 
** Purpose:	Given a string and an optional number convert to a database
**		attribute name and write it out as a keyword.
** Parameters:	str - char * - User string.
**		num - i4      - Optional number (>0 if significant).
-* Returns:	None
*/

void
db_attname( str, num )
char	*str;
i4	num;
{
    db_key( db_sattname(str, num) );
}

/*
+* Procedure:	db_sattname 
** Purpose:	Given a string and an optional number convert to an attribute 
**		name that begins with an alphabetic, is made up of 
**		alphanumerics, and is less or equal to EDB_QRY_MAX characters 
**		("II" if failure).
** Parameters:	str - char * - User string.
**		num - i4      - Optional number (>0 if significant).
-* Returns:	Static attribute name.
** History:
**	22-jun-1988	- Modified for longer names (ncg)
**	20-oct-1988	- Bug 3760 - shortened QUEL names (ncg)
*/

char	*
db_sattname( str, num )
char	*str;
i4	num;
{
    static 	char	attbuf[ EDB_QRY_MAX +2 ];	/* 1 extra for 2-byte */
    register	char	*cp1;
    register	char	*cp2 = attbuf;
    register 	i4	maxlen;
    char		nbuf[ 20 ];

    maxlen = dml_islang(DML_ESQL) ? EDB_QRY_MAX : DB_MAXNAME;
    STcopy( ERx("II"), attbuf );
    nbuf[0] = '\0';
    if (num)			/* Convert number and save space in name */
    {
	CVna( num, nbuf );
	maxlen -= STlength( nbuf );
	STcat( attbuf, nbuf );
    }
    if (!str || *str == '\0')
	return attbuf;

    /* If name begins with digit then prepend the II */
    if (!CMnmstart(str))
	cp2 = &attbuf[2];
    /* Find first alpha numeric character in passed string */
    for (cp1 = str; !CMnmchar(cp1) && *cp1; CMnext(cp1))
	;
    if (!*cp1)
	return attbuf;

    /* Copy rest of string skipping non-alphanumerics */
    CMcpyinc(cp1, cp2);
    for (; *cp1 && (cp2 < attbuf+maxlen);)
    {
	if (CMnmchar(cp1))
	    CMcpyinc(cp1, cp2);
	else
	    CMnext(cp1);
    }
    *cp2 = '\0';
    STcat( cp2, nbuf );
    return attbuf;
}


/*
+* Procedure:	db_sconst 
** Purpose:	Add a string constant in a database statement.
**
** Parameters:	str - char * - String to add.
-* Returns:	None
**
** Note that the string is a Quel string and is always delimited by the '"'
** character by the time it reaches the backend.  The method of escaping it 
** in whatever host language we are using is managed by the Gen routines and 
** is not considered here.  All we have to tell gen_makesconst is that the 
** string will be nested in another string at runtime.
** For example, C  will know to use a \", while Cobol will use a "".
*/

void
db_sconst( str )
char	*str;
{
    i4	dummy;

    /* The string should be followed by a space before another keyword */
    if (dml_islang(DML_ESQL))
	db_key( gen_sqmakesconst(G_INSTRING, str, &dummy) );
    else
	db_key( gen_makesconst(G_INSTRING, str, &dummy) );
}

/*
+* Procedure:	db_delimid
** Purpose:	Add a delimited identifier in a database statement.
**
** Parameters:	str - char * - String to add.
-* Returns:	None
**
*/

void
db_delimid( str )
char	*str;
{
    i4	dummy;

    /* The string should be followed by a space before another keyword */
    if (dml_islang(DML_ESQL))
	db_key( gen_sqmakesconst(G_DELSTRING, str, &dummy) );
    else
	db_key( gen_makesconst(G_DELSTRING, str, &dummy) );
}

/*{
+*  Name: db_hexconst - Add a hex constant to a database statement.
**
**  Description:
**	Given a legal hex constant (the scanner made sure it is legal), add
**	it to a query.  Because a hex constant (format: x'pair{pair}') includes
**	quotes in it, it must be sent to the host language string maker.
**	Because no quotes can be embedded with the hex constant, we treat
**	it as a regular string.  The only difference may be that the caller
**	returns with doubled singled quotes (ie: FORTRAN).
**
**  Inputs:
**	str	- Hex string constant.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	20-jan-1987	- Written (ncg)
*/

void
db_hexconst(str)
char	*str;
{
    i4	dummy;

    /* The string should be followed by a space before another keyword */
    if (dml_islang(DML_ESQL))
	db_key( gen_sqmakesconst(G_REGSTRING, str, &dummy) );
    else
	db_key( gen_makesconst(G_REGSTRING, str, &dummy) );
}

/*{
+*  Name: db_uconst - Add a Unicode literal to a database statement.
**
**  Description:
**
**  Inputs:
**	str	- Unicode literal.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	8-dec-05 (inkdo01)
**	    Cloned from db_hexconst.
*/

void
db_uconst(str)
char	*str;
{
    i4	dummy;

    db_key( gen_sqmakesconst(G_REGSTRING, str, &dummy) );
}

/*{
+*  Name: db_dtconst - Add a date/time literal to a database statement.
**
**  Description:
**
**  Inputs:
**	str	- date/time literal.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	15-aug-2007 (dougi)
**	    Cloned from db_uconst.
*/

void
db_dtconst(str)
char	*str;
{
    i4	dummy;

    db_key( gen_sqmakesconst(G_REGSTRING, str, &dummy) );
}

/*
+* Procedure:	form_sconst 
** Purpose:	Return a string constant not used with database 
**		statements to the caller.
**
** Parameters:	str - char * - String to parse.
** Returns:	Converted and newly allocated string if there were changes, 
-*		Otherwise original string.
**
** Note that the string is a regular string (not nested inside another one).
** The only thing that gen_makesconst() should worry about is how to escape
** host language quotes and escapes themselves.
*/

char	*
form_sconst( str )
char	*str;
{
    i4		diff;			/* String was changed */
    char	*s;

    if (dml_islang(DML_ESQL))
	s = gen_sqmakesconst(G_REGSTRING, str, &diff);
    else
	s = gen_makesconst(G_REGSTRING, str, &diff);
    if (diff)
	return str_add( STRNULL, s );	/* Read the converted string */
    return str;				/* Return the original */
}


/*
+* Procedure:	db_add 
** Purpose:	Local routine to manage the database buffer. If it is too full,
**	    	then it writes out what it has.
** Parameters:	str - char * - String to add to buffer.
-* Returns:	None
**
** Note that this routine may cause whatever is in the buffer to be written 
** out inside a call before the argument is added.
*/

static void
db_add( str )
register char	*str;
{
    register i4	len;
    register i4	datlen;		/* For looong strings */
    char		*datptr;

    if ((len = STlength(str)) <= DB_LINEMAX)	/* It fits */
    {
	/* 
	** If total length is too long then send out buffer. This tries to 
	** preserve the original strings that are being sent within the same 
	** call at runtime.
	*/
	if (len + db_cnt > DB_LINEMAX)
	    db_write();
	STcat( db_buf, str );
	db_cnt += len;
	return;
    }

    /* 
    ** A loooong string has been sent. First empty out buffer, then split the
    ** string into pieces and send till done with the string. This will not
    ** preserve the string at run-time.
    */
    if (db_cnt > 0)
	db_write();
    datptr = str;
    while (len > 0)
    {
	datlen = CMcopy( datptr, min(len, DB_LINEMAX), db_buf );
	db_buf[ datlen ] = '\0';
	db_write();
	len -= datlen;
	datptr += datlen;
    }
}

/*
+* Procedure:	db_write 
** Purpose:	Write out the contents of the database buffer to an IIwritedb 
**	      	call.
**
** Parameters:	None
-* Returns:	None
*/

static void
db_write()
{
    arg_int_add( FALSE );	/* Don't trim constant blanks at run-time */
    arg_int_add( 0 );		/* Null indicator */
    arg_str_add( ARG_CHAR, db_buf );
    gen_call( IIWRITEDB );			/* Special case it */
    db_cnt = 0;
    db_buf[0] = '\0';
}
