# include 	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>		/* INT etc */
# include	<equtils.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqstmt.h>
# include	<equel.h>
# include	<ereq.h>


/*
+* Filename:	REPEAT.C
** Purpose:	Maintains all the routines for managing Repeat Queries.
**
** Defines:	rep_begin( com, qry )	-	Begin Repeat Query with com.
**		rep_param()		-	Prepare for Repeat parameter.
**		rep_add(var, id, wnull)	- 	Add a Repeat parameter.
**		rep_close(func, qry)	- 	Close Repeat Query.
**		rep_recover()		-	Recover from syntax errors.
**		rep_dump()		- 	Dump local info for debugging.
** Globals:
**		(*dml->dm_repeat)()	- 	Hook for DML (ESQL) repeats.
** Locals:
**		rep_buffer()		-	Local bufferring routine.
**		
** This is used for Repeat Queries - to buffer out the definitions of the 
-* original queries, but dump the parameterized part to output.  
**
** Notes:
**
**    0. If your Equel version does not support Repeat queries then the eq_flags
**	 will be set to EQ_NOREPEAT.  The first time through it will
**	 print a warning, but treat the rest of the statement as regular
**	 non-Repeat Quel.
**
**    1. Preproccessor Code and Runtime Code:
**
**     A typical example is the append statement using a two variables for the 
**     Repeat Query parameters for columns:
**
**	##	char	t_col1[SIZE];
**	##	i4	t_col2;
**	##	repeat append to emp ( col1 = @t_col1, col2 = @t_col2 )
**
**     generates something like:
**
**	IIsexec(); 
**	while( !IInexec() )  {
**1.        IIwritedb( "execute eqfile1(" );
**	    IIsetdom( t_col1, CHAR, SIZE );
**	    IIwritedb( "," );  ^-------------------Value of first Parameter.
**	    IIsetdom( t_col2, INT, sizeof(i4) );
**2.        IIwritedb( ")" );  
**	    IIsyncup();
**    	    if ( !IInexec() )  {
**1b.           IIwritedb( "define query eqfile1 is append emp" );
**              IIwritedb( "(col1 = ");
**		IIwritedb( "$0 is c, col2 = "); 
**	        		^---------------First Parameter is character.
**2b.		IIwritedb( "$1 is i4)" );
**		IIsyncup();
**	        if ( IIerrtest() == 0 )  IIsexec(); 
**          }
**      }
**
**     The Preprocessor must buffer out the 'real' query into the 'define' part,
**     while changing the representation of the Repeat parameters - both for
**     the 'define' part and the 'execute' part.  At Runtime the loop is never
**     traversed more than 1.5 times. First try to 'execute'; if ok then it will
**     fall out the next time through the loop - IInexec() will only return 0 if
**     IIsexec() was just called. If not okay then IIsyncup() will return an
**     'unrecognized' error, (which is a special case of calling IIsexec() thus
**     allowing the next 'if' statement to be entered via IInexec()).  At this 
**     point the Query is defined, and if no normal errors, IIsexec() is called 
**     to allow entry back into the loop. No that once the query is defined the 
**     'execute' part will occur.
**
**    2. Code Emitter and Repeat Queries:  
**
**     To generate the correct output - the two database statements shown in
**     the above example - the code emitter must 'switch' output states,
**     between bufferring chars to the local sting table or actually writing
**     output strings to the output file.  Even though both of these functions
**     are related (as far as Repeat Queries are concerned), the code emitter
**     is generating two distinct statements (one happens to be going to a 
**     local buffer).  The local routines manage the syncronization of states
**     between the two emitting functions by using 'output state' pointers 
**     initialized with the local bufferring function.  
**     In the above example data between points 1 and 2 are written to output,
**     while data between 1b and 2b are buffered.
**
**     If the above example was reduced just to reflect the bufferring problem,
**     and calls to IIwritedb() were replaced by W(), and calls to IIsetdom() 
**     were replaced by V(), then the following path is a detail of how the 
**     outputter switches between the output file and the local buffer.
**
** Out: W("execute Q1");      W("("); V(var1);    W(","); V(var2);    W(")");
**      0----->-------+       +------->------+    +----->--------+    +--->1
**		      |       |	             |    |		 |    |
**		      |       |	             |    |		 |    |
**      +-----<-------+       +----<-------+ |    +-----<-----+  |    +--<---+
**      |				   | |    	      |  |	     |
**      +----->-------------------->-------+ +----->----------+  +----->-----+
** Buf: W("define query Q1 append e(col1="); W("$0 is c,col2="); W("$1 is i4)");
**
**    3. ESQL Preproccessor Code Generation
**
**     EXEC SQL REPEATED INSERT INTO t (c) VALUES (var);
**
**     generates something like:
**
**	IIsexec(); 
**	while(!IInexec())  {
**	    IIsqInit();					* (A)
**          IIwritedb( "execute part" );
**	    IIsqSync(); -- or IIsqRInit();		* (B)
**    	    if (!IInexec())  {
**		IIsqInit();				* (C)
**              IIwritedb( "define part" );
**		IIsqSync();				* (D)
**	        if (IIerrtest() == 0) IIsexec(); 
**          }
**      }
**
**      Places marked with a '*' are generated by indirect calls via
**	(*dml->dm_repeat)() which calls out to a provided function to grab
**	more information. In the code routines below, references are made
**	to the different spots.
** 
**   3.	New code generation (Jupiter) for the append statement in (1):
**
**	  IIsexec();
**	  while (IInexec() == 0) {
**	    IIexExec(II_EXQUERY,"eqfile1",11412,54999);
**	    IIsetdom(1,32,0,t_col1);
**	    IIsetdom(1,30,4,&t_col2);
**	    IIsyncup((char *)0,0);
**	    if (IInexec() == 0) {
**	      IIexDefine(II_EXQUERY,"eqfile1",11412,54999);
**	      IIwritedb("append to emp(col1=");
**	      IIwritedb("$0=");
**	      IIsetdom(1,32,0,t_col1);
**	      IIwritedb(",col2=");
**	      IIwritedb("$1=");
**	      IIsetdom(1,30,4,&t_col2);
**	      IIwritedb(")");
**	      IIexDefine(II_EXQREAD,"eqfile1",11412,54999);
**	      if (IIerrtest() == 0) {
**	        IIsexec();
**	      } --IIerrtest
**	    } -- IInexec
**	  } -- IInexec
**
**	Define and Execute don't pass text, and the commas and spaces in
**	the execute section are gone, since the execute code won't go through
**	the parser anymore.  The query identifier has 2 unique numbers as
**	its internal name.
**
**   4.	Repeat cursor code generation:
**
**	## int	numvar;
**	## declare cursor cx for repeat retrieve (num=emp.num+@numvar,
**	##	name=emp.name);
**	## open cursor cx;
**
**	generates:
**
**	  IIsexec();
**	  while (IInexec() == 0) {
**	    IIexExec(II_EXCURSOR,"cx",12028,27967);
**	    IIsetdom(1,30,4,&numvar);
**	    IIcsQuery("cx",12028,27967);
**	    if (IInexec() == 0) {
**	      IIexDefine(II_EXQUERY,"cx",12028,27967);
**	      IIwritedb("retrieve(num=emp.num+");
**	      IIwritedb("$0=");
**	      IIsetdom(1,30,4,&numvar);
**	      IIwritedb(",name=emp.name)");
**	      IIexDefine(II_EXQREAD,"cx",12028,27967);
**	      if (IIerrtest() == 0) {
**	        IIsexec();
**	      } -- IIerrtest
**	    } -- IInexec
**	  } -- IInexec
**
**	Note that the query identifier is the same as the cursor identifier.
**
**	There are only a few changes to implement all of this:
**
**	The new stuff simply uses the new cursor ID stuff, and calls new 
**	routines in the code generator instead of emitting the text of 
**	the query.  In order to mesh with the output-saving of the cursor 
**	routines, out_use_state was modified to return the previous state; 
**	when we switch states we use the old one rather than the default one.
**
**
** History:
**		15-oct-1984 	- Written (ncg)
**		25-apr-1986	- Modified to be driven by another DML (ncg)
**		29-aug-1986	- Modified to support the new-style Jupiter
**				  repeat code, and Jupiter repeat cursors in
**				  EQUEL. (mrw)
**		31-mar-1989	- Modified to generate a call to IIsqMods for a
**				  SQL singleton repeated select.  This change
**				  was needed to support gateways. (teresa) 
**	16-nov-1992 (lan)
**		Added new parameter, arg, to rep_add.
**	19-nov-1992 (lan)
**		When the datahandler argument name is null, generate (char *)0.
**	08-dec-1992 (lan)
**		Removed redundant code in rep_add.
**      13-apl-1995 (chech02)
**              If updates and inserts use the key word REPEATED and have
**              a DATAHANDLER, don't generate variable definition in rep_add().
**	18-mar-1996 (thoda04)
**		Added void return type to rep_recover() to cleanup warning.
**		Added function parameter prototype for rep_buffer().
**		(dm_repeat)() function takes a i4  parm, not a ptr.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* 
** Local string table for buffering out Repeat Queries.  The Repeat Queries 
** buffered into REPQSIZE string tables. Unless one item is greater than this
** size then there can be no overflow.
*/

# define	REPQSIZE	400
static		STR_TABLE	*rep_st = STRNULL;

/* Current Repeat Query number to print with a hopefully unique file name */

static	i4		rep_num = 0;

/*
** Information for the Code Emitter - The final emitting function to use (just
** a character bufferer) and a pointer to the local 'emit' state to be used.
*/

static	i4		rep_buffer(char ch);
static	OUT_STATE	*rep_outstate = OUTNULL;
static	OUT_STATE	*rep_oldout = OUTNULL;

/* 
** Number of Repeat Query parameters - used as an index. 
** Negative value marks not Repeat.
*/

static	i4		rep_arg = -1;

/*
** The non-cursor query-id used to generate runtime arguments for the first
** call to IIexDefine (the define phase) is saved for use in the second call 
** to IIexDefine (the read-DBMS-id phase).
*/
static EDB_QUERY_ID	rep_qry_id;	

/*{
+* Procedure:	rep_begin 
** Purpose:	Begin a Repeat Query with the database command just reduced.
**
** Inputs:
** 	dbstmt - char * - Database statement to begin Repeat query.
**	qry    - EDB_QUERY_ID * - May be pointing at a query id.
** Outputs:
**	Returns:
**		None
**	Errors:
**		ErepNOTIMP	- Repeat queries not implemented.
-*
** Description:
**	For example "repeat append" will call this routine with "append" as 
**	an argument.  Start generating control flow for beginning of Repeat
**	Query loop, and set up output state for Repeat bufferer.
** Side Effects:
*/

void
rep_begin( dbstmt, qry )
char	*dbstmt;
EDB_QUERY_ID 
	*qry;
{
    i4			qry_flag;

    if (eq->eq_flags & EQ_NOREPEAT)
    {
	er_write( E_EQ0377_repNOTIMP, EQ_WARN, 0 );
	return;
    }

    /* Initialize string table */
    if (rep_st == STRNULL)
	rep_st = str_new( REPQSIZE );

    /* No parameters yet */
    rep_arg = 0;

    /* Remember if we're in a cursor */
    if (qry != NULL)
    	qry_flag = II_EXCURSOR;
    else
	qry_flag = II_EXQUERY;

    gen_call( IISEXEC );
    gen_loop( G_OPEN, L_BEGREP, L_ENDREP, ++rep_num, IINEXEC,  C_EQ, 0 );
    
    /* Begin execution part to output */

  /* Hook for ESQL to generate "IIsqInit(&sqlca)" */
    if (dml->dm_repeat)
	(*dml->dm_repeat)((i4)FALSE);
    /*
    ** If "qry" is non-NULL, then we assume it is a query identifier
    ** for an active cursor.  Otherwise we'll generate a new one
    ** for this repeat statement.
    */
    if (qry == NULL)
    {
      /* Get a unique query name */
	edb_unique( &rep_qry_id, db_sattname(eq->eq_filename, rep_num) );
	qry = &rep_qry_id;
    }
    arg_int_add( qry_flag );
    arg_str_add( ARG_CHAR, EDB_QRYNAME(qry) );
    arg_int_add( EDB_QRY1NUM(qry) );
    arg_int_add( EDB_QRY2NUM(qry) );
    gen_call( IIEXEXEC );

    db_send();				/* Make sure it is sent before switch */
    /* Start using secondary code emitter state with the local bufferer */
    rep_outstate = out_init_state( rep_outstate, rep_buffer );
    rep_oldout = out_use_state( rep_outstate );

    /* Begin definition part to buffered area - later printed in an If block */
    gen_do_indent( 1 );

    arg_int_add( qry_flag );
    arg_str_add( ARG_CHAR, EDB_QRYNAME(qry) );
    arg_int_add( EDB_QRY1NUM(qry) );
    arg_int_add( EDB_QRY2NUM(qry) );
    gen_call( IIEXDEFINE );
    /* All Quel and db_key(dbstmt) till the first '@' will be buffered */
}

/*{
+* Procedure:	rep_param 
** Purpose:	Prepare to accept the Repeat paramater.
**
** Input:
**	None.
** Output:
**	Returns:
**		None.
**	Errors:
-*		ErepARG	- Not in a repeat statement.
**
** Description:
**	The '@' was just detected, so switch the output states to get the real 
**	variable dumped to output, and later buffer the Quel description.  
**
**	The sequence of Yacc events in the grammar:
**		rep_rule: '@' { rep_param(); } variable { rep_add(); } ;
** Side Effects:
*/

void
rep_param()
{
    if (eq->eq_flags & EQ_NOREPEAT)
	return;

    if (rep_arg < 0)
    {
	er_write( E_EQ0376_repARG, EQ_ERROR, 0 );
	return;
    }
    db_send();				/* Send buffered data before switch */
    _VOID_ out_use_state( rep_oldout );	/* Default state to output */
    gen_do_indent( -1 );		/* Only in the While block */
    /* Will be followed by normal variable sent to the database in output */
}


/*{
+* Procedure:	rep_add 
** Purpose:	Add a parameter to the 'define' part of the Repeat Query.
**
** Input:
**	varsym  - SYM *  - Variable to add.
**	varid 	- char * - It's name.
**	indsym	- SYM *	 - Host indicator (may be null).
**	indid	- char * - Indicator's name.
**	arg	- char * - Datahandler argument name.
** Output:
**	Returns:
**		Nothing.
**	Errors:
-*		None
**
** The variable has been written to output, as a database variable, and now 
** must be defined in the buffered 'define' part.  Prior to 6.0, the variable
** definition was described the query string (e.g. "$0 is i4"); now it is sent 
** to the DBMS via a DB_DATA_VALUE similar to that describing the actual data.
*/

void
rep_add( varsym, varid, indsym, indid, arg )
SYM	*varsym, *indsym;			/* Symbol table pointers */
char	*varid, *indid;
char	*arg;
{
    char	buf[20];

    if (eq->eq_flags & EQ_NOREPEAT)
	return;

    if (rep_arg < 0)
	return;

    db_send();					/* Clear output before switch */
    _VOID_ out_use_state( rep_outstate );	/* Buffered state for define */
    gen_do_indent( 1 );				/* In a nested If block */
    if (arg == (char *)0)
    {
      /* Emit a leading blank because '$' could be legal in a name in 6.0 */
        STprintf( buf, ERx(" $%d="), rep_arg++ );
        db_key( buf );
        db_send();
        if (indsym != (SYM *)0)
	    arg_var_add( indsym, indid );	
        else
	    arg_int_add( 0 );
        arg_var_add( varsym, varid );
        gen_call( IISETDOM );
    /* Continue to add rest of Query to buffer */
    }
    else
    {
	/* Datahandler - Generates IILQldh_LoDataHandler */
	arg_int_add( 2 );
	arg_var_add( indsym, indid );
	arg_str_add( ARG_RAW, varid );
	arg_str_add( ARG_RAW, arg );
	gen_call( IIDATAHDLR );
    }
}



/*{
+* Procedure:	rep_buffer 
** Purpose:	Buffer characters that are written by the code emitter to
**		a local string table buffer.
**
** Input:
**	ch - char - Character to buffer.
** Output:
**	Returns:
**		i4	- Zero (unused).
**	Errors:
-*		None.
** Description:
**	Indirectly called via the code emitter state.
*/

static 	i4
rep_buffer( char ch )
{
    if (ch != '\0')
	str_chadd( rep_st, ch );
    return 0;
}


/*{
+* Procedure:	rep_close 
** Purpose:	Terminate Repeat Query statement.
**
** Input:
**	dbfunc - i4  	        - Closing function.
**	qry    - EDB_QUERY_ID * - May be pointing at a query id.
**	single_sel - i4	- Indicates a singleton select (May be NULL). 
** Output:
**	Returns:
**		Nothing.
**	Errors:
-*		None.
*/

void
rep_close( dbfunc, qry, single_sel )
i4		dbfunc;
EDB_QUERY_ID	*qry;
i4		single_sel;
{
    if (eq->eq_flags & EQ_NOREPEAT)
    {
	db_close( dbfunc );			/* Close 'execute' part */
	return;
    }

    if (rep_arg < 0)
	return;

    if (qry == NULL)			/* Use saved id if non-cursor */
	qry = &rep_qry_id;

    /* Begin 'define' control structure in output then write buffered part */
    db_send();
    _VOID_ out_use_state( rep_oldout );
    /* Close 'execute' part */
    if (dbfunc != IICSQUERY)
    {
	gen_do_indent( -1 );
	gen_call( dbfunc );
    }
    else
    {
	/* Outdentation already done in ecs_query */
	arg_str_add( ARG_CHAR, EDB_QRYNAME(qry) );
	arg_int_add( EDB_QRY1NUM(qry) );
	arg_int_add( EDB_QRY2NUM(qry) );
	gen_call( IICSQUERY );
    }
    gen_if( G_OPEN, IINEXEC, C_EQ, 0 );

  /* Hook for ESQL to generate "IIsqInit(&sqlca)". */
  /* If ESQL singleton repeated select, also generate "IIsqMods" */
    if (dml->dm_repeat)
	(*dml->dm_repeat)(single_sel);
	
    /* Dump everything that was buffered */
    str_chget( rep_st, (char *)0, rep_oldout->o_func );
    str_free( rep_st, (char *)0 );
    /* 
    ** Close 'define' part.
    ** For 6.0 (at runtime), instead of calling IIsyncup, we call IIexDefine 
    ** with a "read" flag which means read the DBMS query_id before
    ** doing a sync-up.
    */

  /* Repeat cursors may have had "for readonly" added at open time. */
    if (dbfunc == IICSQUERY)
    {
	arg_int_add( II_EXGETRDO );
	arg_str_add( ARG_CHAR, (char *)0 );
	gen_call( IICSRDO );
    }
    
    arg_int_add( II_EXQREAD );
    arg_str_add( ARG_CHAR, EDB_QRYNAME(qry) );
    arg_int_add( EDB_QRY1NUM(qry) );
    arg_int_add( EDB_QRY2NUM(qry) );
    gen_call( IIEXDEFINE );

    gen_if( G_OPEN, IIERRTEST, C_EQ, 0 );
    gen_call( IISEXEC );
    gen_if( G_CLOSE, IIERRTEST, C_0, 0 );

    gen_if( G_CLOSE, IINEXEC, C_0, 0 );
    gen_loop( G_CLOSE, L_BEGREP, L_ENDREP, rep_num, IINEXEC, C_0, 0 );

    if (dbfunc == IICSQUERY)	/* Extra outdent level for buffered cursor */ 
	gen_do_indent( -1 );
    rep_arg = -1;			/* Flag that Repeat Query is done */
}

/*{
+* Procedure:	rep_recover
** Purpose:	Recover from syntax errors in a repeat statement.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		None.
**	Errors:
-*		None.
** Description:
**	Just flush any pending output and then empty the saved output buffer.
** Side Effects:
**	Tosses any pending output.
*/

void
rep_recover()
{
    db_send();
    str_free( rep_st, (char *)0 );
}

/*{
+* Procedure:	rep_dump 
** Purpose:	Dump all information about the current Repeat Query.
**
** Inputs:
**	calname - char * - Caller's name.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
-*		None.
*/

void
rep_dump( calname )
char	*calname;
{
    register	FILE	*df = eq->eq_dumpfile;

    if (calname == (char *)NULL)
	calname = ERx("Repeat_stats");
    SIfprintf( df, ERx("REP_DUMP: %s\n"), calname );
    if (rep_st == STRNULL)
    {
	SIfprintf( df, ERx(" No Repeat Queries yet.\n") );
    }
    else
    {
	SIfprintf( df, ERx(" rep_num = %d, rep_arg = %d (<0 means not Repeat)\n"),
		   rep_num, rep_arg );
	SIfprintf( df, ERx(" rep_qry_name = '%s', rep_qry_nums = %d,%d\n"),
		   EDB_QRYNAME(&rep_qry_id),
		   EDB_QRY1NUM(&rep_qry_id), EDB_QRY2NUM(&rep_qry_id));
	str_dump( rep_st, ERx("Repeat_string") );
    }
    SIflush( df );
}
