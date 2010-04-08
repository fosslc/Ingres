/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<er.h>
#include	<me.h>
#include	<cm.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<generr.h>
#include	<gca.h>
#include	<eqrun.h>
#include	<iicgca.h>
#include	<eqlang.h>
#include	<iisqlda.h>
#include	<iirowdsc.h>
# include	<iisqlca.h>
#include	<iilibq.h>
#include	<erlq.h>

/**
+*  Name: iisqdrun.c - Runtime routines to handle ESQL dynamic statements.
**
**  Description:
**	This file contains the routines called from an ESQL program
**	to run a dynamic statement.
**
**  Defines:
**	IIsqExImmed	- Send dynamically formed query to DBMS
**	IIsqExStmt	- Instruct DBMS to execute a prepared statement
**	IIsqDaIn	- Send descriptor of variables to DBMS
**	IIsqPrepare	- Prepare (and possibly Describe) a statement
**	IIsqDescInput	- Describe Input into an SQLDA
**	IIsqDescribe	- Describe [Output] a statement into an SQLDA
**	IIcsDaGet	- Cursor fetch of values via an SQLDA
**  Locals:
**	IIsq_RdDesc	- Assign descriptive values to SQLDA.
**
**  Notes:
**	One of these routines (IIcsDaGet) is also a cursor routine, but
**	it appears in this module rather than in iicsrun.c because it does
**	not interract with the cursor "state".
**
**  History:
**	29-apr-1987	- written (bjb)
**	29-jun-1987 	- Code cleanup.  Help to quiet lint. (bab)
**	12-dec-1988	- Generic error processing. (ncg)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	26-oct-1992 (kathryn)
**	    Updated for Large Object (DATAHANDLER clause) support.
**	13-sep-1993 (sandyd)
**	    Added case for DB_VBYTE_TYPE in IIsqDaIn() and IIcsDaGet(), since 
**	    varbyte needs a length adjustment due to the count field just 
**	    like DB_VCH_TYPE.
**	02-jul-1996 (thoda04)
**	    Added function prototypes for Windows 3.1 port. 
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jun-2006 (kschendel)
**	    Added describe input.
**/


void	IIsq_RdDesc();
void	IILQprvProcValio(char *param_name,
    	                 i4    param_byref, /* UNUSED */
    	                 i2   *indaddr,
    	                 i4    isref,
    	                 i4    type,
    	                 i4    length,
    	                 PTR   addr1,
    	                 PTR   addr2); /* Only used for f8 by value */
void	IILQldh_LoDataHandler(i4 type, i2 *indvar, 
    	                      int	(*sqlhdlr)(), PTR hdlr_arg);
bool	IIsqdSetSqlvar(i4     lang,
    	               IISQLDA *sqd,
    	               i4      adf_types,
    	               char    *objname,
    	               DB_DATA_VALUE *objdbv);


/*{
**  Name: IIsqExImmed - Send dynamically formed query.
**
**  Description:
**	This routine is the run-time interface to the EXECUTE IMMEDIATE
**	statement.  It sends the dynamically specified query to the DBMS.
**	The EXECUTE IMMEDIATE statement when used with a SELECT query is
**	an INGRES (front-end) extension and is not understood by the DBMS.
**	Therefore this routine must distinguish between SELECT and
**	non-SELECT queries, and send only the non-SELECT queries with
**	the 'execute immediate' query.  SELECT queries are sent down
**	as is.  The ESQL preprocessor will have generated the necessary
**	looping construct to get the data back.
**
**  Inputs:
**	query	- Dynamic query string;
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Preprocessor generated code:
**  1.  Non-SELECT query.
**	EXEC SQL EXECUTE IMMEDIATE :query_buf;
**
**	IIsqInit(&sqlca);
**	IIsqExImmed(query_buf);
**	IIsyncup();
**
**      Query sent to DBMS:
**	execute immediate query
**
**  2.  SELECT query into variables using BULK SELECT loop.
**	EXEC SQL EXECUTE IMMEDIATE :select_buf into :var1, var2;
**	EXEC SQL BEGIN;
**	    program code;
**	EXEC SQL END;
**
**	IIsqInit(&sqlca);
**	IIsqMods(IImodDYNSEL);
**	IIsqExImmed(select_buf);
**	IIretinit();
**	if (IIerrtest() != 0) goto IIrtE1;
**     IIrtB1:
**	while (IInextget() != 0) {
**	    IIgetdomio(var1);
**	    IIgetdomio(var2);
**	    if (IIerrtest() != 0) goto IIrtB1;
**	      program code;
**	}
**	IIflush();
**     IIrtE1:;
**
**	Query sent to DBMS:
**	(contents of select_buf)
**
** 3.	Singleton SELECT query using descriptor 
**	EXEC SQL EXECUTE IMMEDIATE :select_buf USING DESCRIPTOR desc;
**
**	IIsqInit(&sqlca);
**	IIsqMods(IImodDYNSEL);
**	IIsqMods(IImodSINGLE);
**	IIsqExImmed(select_buf);
**	IIretinit();
**	if (IInextget() != 0) {
**	    IIcsDaGet(lang, desc);
**	}
**	IIsqFlush();
**
**	Query sent to DBMS:
**	(contents of select_buf);
**
**  Side Effects:
**	
**  History:
**	27-apr-1987 - written (bjb)
*/

void
IIsqExImmed(query)
char	*query;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if (IIlbqcb->ii_lq_flags & II_L_DYNSEL)
    {
	char *cp = query;
	
	/* Search for [whitespace] ['(' {'('}] [whitespace] SELECT */

	while (*cp == '(' || CMwhite(cp))
	    CMnext(cp);
	if (STbcompare( ERx("select"), 7, cp, 7, TRUE ) != 0)
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ006F_DYNSEL, II_ERR, 0, (char *)0);
	    return;
	}
	IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, query);
    }
    else
    {
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx("execute immediate "));
	IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, query);
    }
}


/*{
**  Name: IIsqExStmt - Execute a prepared statement name.
**
**  Description:
**	Execute a statement that has been prepared in the DBMS possibly
**	with parameters.  The parameter specifications follow this call.
**	This routine is also called just to transmit the "using" syntax to
**	the DBMS and turn on the "comma separating" flag.
**
**  Inputs:
**	stmt_name	- Statement name (may be null if all we want is to
**			  turn on the "comma" flag).
**	using_vars	- Are there variables following? 1/0 - Yes/No.
**			  If there are then send the "using" clause.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**
**  Preprocessor generated code:
**	EXEC SQL EXECUTE s1 USING :ivar;
**
**	IIsqInit(&sqlca);
**	IIsqExStmt("s1", 1);
**	IIputdomio((char *)0,TRUE, DB_INT_TYPE, sizeof(ivar), &ivar);
**	IIsyncup();
**
**  Query sent to DBMS:
**	execute s1 [using ~V]
**
**  Preprocessor generated code:
**
**	EXEC SQL OPEN CURSOR USING :ivar
**
**	IIsqInit(&sqlca);
**	IIcsOpen("s1",id, id);
**	IIsqExStmt((char *)0, 1);
**	IIputdomio((char *)0, TRUE, DB_INT_TYPE, sizeof(ivar), &ivar);
**	IIcsQuery("s1", id, id);
**
**  Query sent to DBMS:
**	" using "
**
**  Side Effects:
**	
**  History:
**	27-apr-1987 - written (bjb)
*/

void
IIsqExStmt(stmt_name, using_vars)
char	*stmt_name;
i4	using_vars;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if (stmt_name != (char *)0)
    {
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx("execute "));
	IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, stmt_name);
    }
    if (using_vars)
    {
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" using "));
	/* Send variables with separating commas */
	IIlbqcb->ii_lq_curqry |= II_Q_1COMMA;
    }
}


/*{
**  Name: IIsqDaIn - Send descriptor of variables via IIputdomio
**		     IILQprvProcValio, or IILQldh_LoDataHandler.
**
**  Description:
**	Given an SQLDA send each variable and null indicator to IIputdomio
**	/IILQprvProcValio/IILQldh_LoDataHandler for input to the DBMS.
**
**	Each variable referenced by the SQLDA must have a legal type 
**	identifier, a valid user length, some associated data and, if the 
**	type is nullable, a 2-byte null indicator.  This routine loops through 
**	all the the variables pointed at by the sqlda, verifying that the 
**	variable is legal (ie, if the type is nullable then there must be a 
**	null indicator).  Some verification (ie, type is legal) is done lower 
**	down in the ADH routines.  However, character types are checked by this
**	routine.  We map DB_CHR_TYPE to DB_CHA_TYPE for non-C strings to 
**	indicate to ADH conversion routines that the string is not null-
**	terminated and blank trimming should be done.  C-strings are given
**	a zero length to force the ADH conversion routines to do an STlength(),
**	and are sent as DB_CHR_TYPE.  
**
**	A user-defined datahandler, rather than a host language variable, may
**	be specified by an SQLDA variable.  If a datahandler is specified, then
**	it will be responsible for tranmitting the data value to the dbms.
**      When the sqltype of an SQLVAR has been set to DB_HDLR_TYPE, then
**      IILQldh_LoDataHandler() will be called to invoke a user-defined
**      handler which will transmit the data value to the dbms. It is
**      expected that sqldata field contains a pointer to an IISQLHDLR 
**	structure, which contains two fields: 1) sqlhdlr and 2) sqlarg; 
**	where the sqlhdlr field is a pointer to a user-defined function and 
**	sqlarg is a pointer to an optional argument.  
**
**	Note that a user-defined datahandler may NOT be used to transmit a
**	database procedure parameter.
**
**	Note that the absolute value of the type is sent to IIputdomio(), 
**	because null types are decided on in the ADH conversion routines, 
**	depending on the presence of a null indicator.
**
**	If a serious error is found at any time during the loop, such as
**	a missing variable, the particular instance is skipped.
**
**	The value of SQLD may be zero to indicate no parameters are used.
**	If SQLD is greater than zero then this routine first sends the
**	"using" clause and turns on the "comma separators" (as is done in
**	IIsqExStmt with a hard coded list of variables).  Note that in
**	order to support older preprocessed files (when IIsqExStmt was
**	generated in front of IIsqDaIn with "using_flag" set) we check the
**	value of the "comma separator" flag first before generating the
**	"using" clause.  Files that were preprocessed before 6.2 and have
**	SQLD = 0 will generate a syntax error from the DBMS.
**
** 	For the dynamic execution of database procedures the procedure code
**	turns on an "exec" mode flag which allows us to call IILQprvProcValio.
**	In this case we also process SQLNAME for the parameter name.
**
**
**  Inputs:
**	lang		- Language constant.
**	sqd		- User's SQLDA.
**	  .sqln		- Number of "sqlvar" elements.
**	  .sqld		- Number of pertinent "sqlvar" elements >= 0.
**	  .sqlvar[]	- Array of structures:
** 	    .sqltype	- Type id of input variable.
**	    .sqllen	- Length of input variable.
**	    .sqldata	- Address of input variable.
**	    .sqlind	- Address of null indicator (or null).
**	    .sqlname    - Name of dynamic procedure execution parameter.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    Errors reported within IIputdomio (from ADH) and IILQprvProcValio.
**	    SQLDA		Null pointer to SQLDA.
**	    SQNULLDYN		Nullable type but no null indicator.
**	    SQVARDYN		Null data pointer.
**
**  Preprocessor generated code:
**	EXEC SQL EXECUTE s1 USING DESCRIPTOR :sqlda;
**
**	IIsqInit(&sqlca);
**	IIsqExStmt("s1", 0);	-- pre-6.2 was IIsqExStmt("s1", 1);
**	IIsqDaIn(EQ_C, &sqlda);
**	IIsyncup();
**
**	For EXECUTE PROCEDURE IIsqDaIn replaces the sequence of calls to
**	IILQprvProcValio.  See iidbproc.c for details.
**
**  Query sent to DBMS:
**	execute s1 [using ~V , ~V , ...]
**		    if SQLD > 0
**
**  Side Effects:
**	None
**
**  History:
**	30-apr-1987 - written (bjb)
**	29-dec-1988 - modified to allow SQLD = 0 (ncg)
**	24-may-1989 - allow support to execute a procedure (ncg)
**	26-oct-1992 - (kathryn) -- Large Object Support.
**	    Added support for DATAHANDLER type (DB_HDLR_TYPE), which specifies
**	    a user-defined datahandler to be invoked to transmit the data.
**	13-sep-1993 (sandyd)
**	    Added case for DB_VBYTE_TYPE since that needs a length adjustment
**	    due to the count field just like DB_VCH_TYPE.
**      17-Nov-2003 (hanal04) Bug 111251 INGEMB111
**          Added case for DB_NVCHR_TYPE since that needs a length adjustment
**          due to the count field just like DB_VCH_TYPE.
*/

void
IIsqDaIn(lang, sqd)
i4	lang;
IISQLDA	*sqd;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    struct sqvar *sqv;
    i4		i;
    i4		sqlen, sqtype, numsq;
    i2		*sqind;
    char	errbuf[20];
    bool	is_dbp = FALSE;			/* Database procedure in use */
    char	sqname[IISQD_NAMELEN +1];	/* DBP parameter name and  */
    i4		nmlen;				/*		length    */
    IISQLHDLR   *sqhdlr;

    if (sqd == (IISQLDA *)0)
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ006E_SQLDA, II_ERR, 0, (char *)0);
	return;
    }

    /* If SQLD = 0 then no parameters are specified */
    if (sqd->sqld <= 0)
	return;

    /*
    ** Check if using a database procedure, otherwise if not following pre-6.2
    ** generated IIsqExStmt code then add "using" clause and turn on "commas".
    */
    if (IIlbqcb->ii_lq_prflags & II_P_EXEC)
    {
	is_dbp = TRUE;
    }
    else if ((IIlbqcb->ii_lq_curqry & II_Q_1COMMA) == 0)
    {
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" using "));
	IIlbqcb->ii_lq_curqry |= II_Q_1COMMA;
    }

    sqv = sqd->sqlvar;
    numsq = min(sqd->sqln, sqd->sqld);		/* sqld should be <= sqln */
    for (i = 0; i < numsq; i++, sqv++)
    {
	sqind = (i2 *)0;			/* Default */
  	if (sqv->sqltype < 0)
	{
	    if (sqv->sqlind == (i2 *)0)
	    {
		CVna( i +1, errbuf );
		IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_NEED_IND),
			 E_LQ006B_SQNULLDYN, II_ERR, 1, errbuf);
	    }
	    else
	    {
		sqind = sqv->sqlind; 
	    }
 	}
	if (sqv->sqldata == (PTR)0)
	{
	    CVna( i +1, errbuf );
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ006C_SQVARDYN, II_ERR, 1, errbuf);
	    continue;
	}
	/*
	** If DBP copy name - if the length is wrong assign an empty name which
	** will cause a "parameter error" in IILQprvProcValio.
	*/
	if (is_dbp)
	{
	    nmlen = sqv->sqlname.sqlnamel;
	    if (nmlen > 0 && nmlen <= IISQD_NAMELEN)
	    {
		MEcopy(sqv->sqlname.sqlnamec, nmlen, sqname);
		sqname[nmlen] = EOS;
	    }
	    else
	    {
		sqname[0] = EOS;
	    }
	}

	sqtype = abs(sqv->sqltype);		/* Defaults */
	sqlen = sqv->sqllen;
	/*
	** Send character data down as a "C string" only if language is C.
	*/
	if (sqtype == DB_CHR_TYPE || sqtype == DB_CHA_TYPE)
	{
	    if (lang == EQ_C)
	    {
		sqlen = 0;
		sqtype = DB_CHR_TYPE;
	    }
	    else
	    {
		sqtype = DB_CHA_TYPE; 
	    }
	}
	else if (sqtype == DB_VCH_TYPE || sqtype == DB_VBYTE_TYPE ||
		 sqtype == DB_NVCHR_TYPE)
	{
	    sqlen += DB_CNTSIZE;		/* Add in .db_t_count field */
	}

        if (is_dbp)
	    IILQprvProcValio(sqname, FALSE,
                             sqind, TRUE, sqtype, sqlen, sqv->sqldata, NULL);
	else if (sqtype == DB_HDLR_TYPE)
	{
	    sqhdlr = (IISQLHDLR *)sqv->sqldata;
	    IILQldh_LoDataHandler(2,sqind, sqhdlr->sqlhdlr,sqhdlr->sqlarg);
	}
	else 
	    IIputdomio(sqind, 1, sqtype, sqlen, sqv->sqldata);
   }

}

/*{
**  Name: IIsqPrepare - Prepare [and describe] a statement.
**
**  Description:
**	This routine sends a dynamically built statement to the DBMS with
**	a user specified statement name.
**	If the statement is also being described then it expects the DBMS
**	to return a descriptor which it assigns to the user's SQLDA  by calling
**	IIsq_RdDesc which is also called from IIsqDescribe.  IIsq_RdDesc
**	reads the LIBQ descriptor and maps it out to the user's external format.
**
**  Inputs:
**	lang		- Language constant for possible SQLDA details.
**	stmt_name	- Statement name being prepared.
**	sqd		- User's SQLDA to fill if this also has an INTO
**			  clause.  If this is not null then send
**			  "into iisqlda" to the DBMS.  This notifies it
**			  that we also want a description.
**	using_flag	- Was a USING clause specified?
**			   IIsqdNULUSE	- No
**			   IIsqdNAMUSE  - Using names.  Send the "using names"
**					  clause.  Only sent if the SQLDA was
**					  not null.
**			   IIsqdADFUSE  - Using "adftypes".  This internal
**					  using clause indicates that the
**					  description should include the ADF
**					  DB data value as well in sqldata.
**	query		- Query being prepared.
**
**  Outputs:
**	sqd		- The SQLDA is filled with the description of the
**			  query as returned from the DBMS.  See IIsq_RdDesc
**			  for the list of fields assigned.
**	    .sqld	- If this is not a SELECT query then the SQLD field 
**			  is set to 0, otherwise it is set to the number of 
**			  result columns.
**	Returns:
**	    Nothing
**	Errors:
**		Errors in getting descriptor are reported in IIsq_RdDesc().
**
**  Preprocessor generated code:
**	EXEC SQL PREPARE s1 INTO :sqlda USING NAMES FROM :qry_buf;
**
**	IIsqInit(&sqlca);
**	IIsqPrepare(EQ_C, "s1", &sqlda, IIsqdNAMUSE, qry_buf);
**
**  Query sent to DBMS:
**	prepare s1 [into iisqlda [using names]] from query
**
**  Side Effects:
**	1. Turns on the NAMES flag for the DBMS if the INTO clause was
**	   used.
**	
**  History:
**	04-may-1987 - written (bjb)
**	30-aug-1988 - modified for extended use of using_flag (ncg)
*/

void
IIsqPrepare(lang, stmt_name, sqd, using_flag, query)
i4	lang;
char	*stmt_name;
IISQLDA	*sqd;
i4	using_flag;
char	*query;
{

    if (sqd)
	IIqry_start(GCA_QUERY, GCA_NAMES_MASK, 0, ERx("prepare"));
    IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx("prepare "));
    IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, stmt_name);
    if (sqd)
    {
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" into iisqlda"));
	if (using_flag == IIsqdNAMUSE)
	    IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" using names"));
    }
    IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" from "));
    IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, query);
    if (sqd)
    {
	IIsq_RdDesc(lang, using_flag == IIsqdADFUSE, sqd);
    }
    else
    {
	IIsyncup((char *)0, 0);
    }
}


/*{
**  Name: IIsqDescInput - DESCRIBE INPUT into an SQLDA.
**
**  Description:
**	Given a dynamically prepared statement describe the query parameters
**	into a user's SQLDA.  The DBMS returns a standard tuple-descriptor
**	via IIsq_RdDesc, which does the mapping from the internal 
**	data lengths to the user's external format.
**
**	This call is pretty much the same as DESCRIBE [OUTPUT], without
**	any "using names" variant.
**
**  Inputs:
**	lang		- Language constant for possible SQLDA details.
**	stmt_name	- Statement name being described.
**	sqd		- User's SQLDA to fill.
**
**  Outputs:
**	sqd		- The SQLDA is filled with the description of the
**			  query as returned from the DBMS.  See IIsq_RdDesc
**			  for a complete list of the fields assigned.
**	    .sqld	  If there are no parameter markers in the statement,
**			  this is set to 0, otherwise it is set to the number
**			  of parameter markers.
**	Returns:
**	    Nothing
**	Errors:
**		SQLDA		SQLDA pointer is null.
**		Errors in getting descriptor are reported in IIsq_RdDesc().
**
**  Preprocessor generated code:
**	EXEC SQL DESCRIBE INPUT s1 USING [SQL] DESCRIPTOR :sqlda;
**
**	IIsqInit(&sqlca);
**	IIsqDescInput(EQ_C, "s1", &sqlda);
**
**  Query sent to DBMS:
**	describe input s1
**
**  Side Effects:
**	
**  History:
**	6-Jun-2006 (kschendel)
**	    DESCRIBE INPUT needs a slight variant of DESCRIBE.
*/

void
IIsqDescInput(lang, stmt_name, sqd)
i4	lang;
char	*stmt_name;
IISQLDA	*sqd;
{
    IIqry_start(GCA_QUERY, GCA_NAMES_MASK, 0, ERx("describe input"));
    IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx("describe input "));
    IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, stmt_name);
    if (sqd)
    {
	IIsq_RdDesc(lang, 0, sqd);
    }
    else
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ006E_SQLDA, II_ERR, 0, (char *)0);
	IIsyncup( (char *)0, 0 );
    }
}


/*{
**  Name: IIsqDescribe - Describe a statement into an SQLDA.
**
**  Description:
**	Given a dynamically prepared statement describe the query results
**	in terms of data descriptions into a user's SQLDA.  The DBMS returns 
**	a descriptor via IIsq_RdDesc, which does the mapping from the internal 
**	data lengths to the user's external format.
**
**  Inputs:
**	lang		- Language constant for possible SQLDA details.
**	stmt_name	- Statement name being described.
**	sqd		- User's SQLDA to fill.
**	using_flag	- Was a USING clause specified?
**			   IIsqdNULUSE	- No
**			   IIsqdNAMUSE  - Using names.  Send the "using names"
**					  clause.  Only sent if the SQLDA was
**					  not null.
**			   IIsqdADFUSE  - Using "adftypes".  This internal
**					  using clause indicates that the
**					  description should include the ADF
**					  DB data value as well in sqldata.
**  Outputs:
**	sqd		- The SQLDA is filled with the description of the
**			  query as returned from the DBMS.  See IIsq_RdDesc
**			  for a complete list of the fields assigned.
**	    .sqld	  If this is not a SELECT query then the SQLD field 
**			  is set to 0, otherwise it is set to the number of 
**			  result columns.
**	Returns:
**	    Nothing
**	Errors:
**		SQLDA		SQLDA pointer is null.
**		Errors in getting descriptor are reported in IIsq_RdDesc().
**
**  Preprocessor generated code:
**	EXEC SQL DESCRIBE s1 INTO :sqlda;
**
**	IIsqInit(&sqlca);
**	IIsqDescribe(EQ_C, "s1", &sqlda, 1);
**
**  Query sent to DBMS:
**	describe s1 [using names]
**
**  Side Effects:
**	1. Turns on the NAMES flag for the DBMS.
**	
**  History:
**	04-may-1987 - written (bjb)
**	30-aug-1988 - modified for extended use of using_flag (ncg)
*/

void
IIsqDescribe(lang, stmt_name, sqd, using_flag)
i4	lang;
char	*stmt_name;
IISQLDA	*sqd;
i4	using_flag;
{
    IIqry_start(GCA_QUERY, GCA_NAMES_MASK, 0, ERx("describe"));
    IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx("describe "));
    IIwritio(TRUE, (i2 *)0, 1, DB_CHR_TYPE, 0, stmt_name);
    if (using_flag == IIsqdNAMUSE)
	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, ERx(" using names"));
    if (sqd)
    {
	IIsq_RdDesc(lang, using_flag == IIsqdADFUSE, sqd);
    }
    else
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ006E_SQLDA, II_ERR, 0, (char *)0);
	IIsyncup( (char *)0, 0 );
    }
}


/*{
**  Name: IIcsDaGet - Fetch values out via an SQLDA.
**
**  Description:
**	Given an SQLDA fill each variable (and null indicator) with data
**	from a column in the current row being fetched.  This is done by 
**	calling IIcsGetio() or IIgetdomio or IILQldh_LoDataHandler() for each 
**	SQLDA element.  The
**	number of SQLDA elements to be filled is min(sqld, sqln).  For
**	cursor fetches, IIcsGetio() takes care of all cursor related and
**	I/O related issues (the cursor has already been set up for this
**	row by IIcsRetrieve()).  This routine just walks through the user's
**	SQLDA.  
**
**	Note that the lower conversion routines (ADH) must distinguish between
**	C and non-C character variables in order to null terminate or blank pad.
**	This routine uses DB_CHA_TYPE to indicate blank padding and DB_CHR_TYPE
**	to indicate null termination.
**
**      A user-defined datahandler, rather than a host language variable, may
**      be specified by an SQLDA variable.  If a datahandler is specified, then
**      it will be responsible for tranmitting the data value from the dbms.
**      When the sqltype of an SQLVAR has been set to DB_HDLR_TYPE, then
**      IILQldh_LoDataHandler() will be called to invoke a user-defined
**      handler which will handle the data value returned from the dbms. It is
**      expected that sqldata field contains a pointer to an IISQLHDLR
**      structure, which contains two fields: 1) sqlhdlr and 2) sqlarg;
**      where the sqlhdlr field is a pointer to a user-defined function and
**      sqlarg is a pointer to an optional argument.
**	
**
**  Inputs:
**	lang		- Language constant.  If this is not C then flag
**			  lower routines to blank pad and not null terminate.
**	sqd		- User's SQLDA.
**	   .sqld	- Indicates how many columns to fetch (will have
**			  been set during the DESCRIBE statement).
**	   .sqln	- Indicates how many sqlvar[] elements user program
**			  has allocated (.sqln >= .sqld).
**
**  Outputs:
**	sqd		- The SQLDA sub-structures SQLVAR are filled with
**			  data.
**	Returns:
**	    Nothing
**	Errors:
**	    SQLDA		Pointer to SQLDA is null.
**	    SQNULLDYN 		Nullable type but no null indicator.
**	    SQVARDYN    	Null data pointer.
**	    Errors reported within IIcsGetio.
**
**  Preprocessor generated code:
**  1.  Fetch statement.
**
**	EXEC SQL FETCH c1 USING DESCRIPTOR :sqlda;
**
**	IIsqInit(&sqlca);
**	if (IIcsRetrieve("c1", num1, num2) != 0) {
**	    IIcsDaGet(EQ_C, &sqlda);
**	    IIcsERetrieve();
**	}
**
**  2.  Dynamic select statement.
**
**	EXEC SQL EXECUTE IMMEDIATE :select_stmt USING DESCRIPTOR :sqlda;
**
**  Side Effects:
**
**  History:
**	30-apr-1987 - written (bjb)
**	07-sep-1989 - updated for dynamic selects (bjb)
**      26-oct-1992 - (kathryn) -- Large Object Support.
**          Added support for DATAHANDLER type (DB_HDLR_TYPE), which specifies
**          a user-defined datahandler to be invoked to transmit the data.
**	13-sep-1993 (sandyd)
**	    Added case for DB_VBYTE_TYPE since that needs a length adjustment
**	    due to the count field just like DB_VCH_TYPE.
**	21-oct-2003 (gupsh01)
**	    Added case for handling DB_NVCHR_TYPE since it needs length 
**	    adjustment for the count field like DB_VCH_TYPE.
*/

void
IIcsDaGet(lang, sqd)
i4	lang;
IISQLDA	*sqd;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    register struct sqvar	*sqv;
    register	i4	i;
    i4		sqlen, sqtype, sqnum;
    i2		*sqind;
    char	errbuf[20];
    IISQLHDLR	*sqhdlr;

    if (sqd == (IISQLDA *)0)
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ006E_SQLDA, II_ERR, 0, (char *)0);
	return;
    }
    sqnum = min(sqd->sqld, sqd->sqln);
    if (sqnum != 0)
	sqv = &sqd->sqlvar[0];
    for (i = 0; i < sqnum; i++, sqv++)
    {
	sqind = (i2 *)0;		/* Default */
	if (sqv->sqltype < 0)
	{
	    if (sqv->sqlind == (i2 *)0)
	    {
		CVna( i +1, errbuf );
		IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_NEED_IND),
			 E_LQ006B_SQNULLDYN, II_ERR, 1, errbuf);
	    }
	    else
	    {
		sqind = sqv->sqlind;
	    }
	}
	if (sqv->sqldata == (PTR)0)
	{
	    CVna( i +1, errbuf );
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ006C_SQVARDYN, II_ERR, 1, errbuf);
	    continue;
	}
	sqtype = abs(sqv->sqltype);		/* Defaults */
	sqlen = sqv->sqllen;
	/*
	** Convert character data into "C string" only if language is C.
	*/
	if ((sqtype == DB_CHR_TYPE) && (lang != EQ_C))
	{
	    sqtype = DB_CHA_TYPE;
	}
	else if ((sqtype == DB_CHA_TYPE) && (lang == EQ_C))
	{
	    sqtype = DB_CHR_TYPE;
	}
	else if (sqtype == DB_VCH_TYPE || sqtype == DB_VBYTE_TYPE || 
		 sqtype == DB_NVCHR_TYPE)
	{
	    sqlen += DB_CNTSIZE;
	}
	
	if (sqtype == DB_HDLR_TYPE)
	{
	    sqhdlr = (IISQLHDLR *)sqv->sqldata;
	    IILQldh_LoDataHandler(1,sqind, sqhdlr->sqlhdlr,sqhdlr->sqlarg);
	}
	else if (IIlbqcb->ii_lq_flags & II_L_DYNSEL)
	    IIgetdomio( sqind, 1, sqtype, sqlen, sqv->sqldata );
	else
	    IIcsGetio( sqind, 1, sqtype, sqlen, sqv->sqldata );
    }
}

/*{
+*  Name: IIsq_RdDesc - Get tuple description into user's SQLDA
**
**  Description:
**	IIsq_RdDesc is called by IIsqPrepare and IIsqDescribe to transfer
**	information from LIBQ's row descriptor(IIlbqcb.ii_lq_retdesc.ii_rd_desc)
**	into the embedded program's SQLDA.  The information consists of: the 
**	number of columns; and for each column, its type, length, length of 
**	colname, and colname.
**
**	If adf_types is set then the internal ADF DB_DATA_VALUE is returned
**	in the data field together with the external type description.
**
**	Length information (i.e., the length of the data being described)
**	must be converted from the internal representation to a user 
**	representation.  This means that for nullable types, we subtract
**	one byte from the length, for TEXT and VCH types, we subtract
**	the length of the count field, and for DATE and MONEY we return 0.
**
**	Type Mapping that are not defaults:
**
**	DBMS						User
**	Varying Length Char (TXT, LTXT, VCH)		VCH
**	Fixed Length (CHR, CHA)				CHA
**
**	This internal type mapping is done by IIsqdSetSqlvar.
**
**  Inputs:
**	lang			Host language for processing.
**	adf_types		Indicates whether we should return the ADF
**				descriptor to the caller.
**	sqd			Pointer to the embedded program's SQLDA struct.
**				Assumed by this routine to be non-null.
**	    .sqln		Number of "sqvar" structures within the SQLDA
**				allocated by program.  If sqln < the number 
**				of columns to be described from DBMS then this 
**				routine fills in sqln column descriptors in
**				the SQLDA.
**
**  Outputs:
**	sqd
**	    .sqldaid		Initialized to "SQLDA   ".
**	    .sqldabc		Initialized to current size of SQLDA.
**	    .sqld	  	If this is not a SELECT query then the 
**				SQLD field is set to 0, otherwise it is set 
**				to the number of result columns.
**	    .sqlvar[sqln]	A sqln-dimension array of column descriptors.
**		.sqltype	Datatype id of column.
**	    	.sqllen		Length of data in column.  (See routine
**				description).
**		.sqldata	If adf_types then pointed at DB_DATA_VALUE.
**	    	.sqlname	Column name information
**		    .sqlnamel	Length of column name.
**	    	    .sqlnamec	Column name (no EOS termination).
**
**	Returns:
**	    Nothing.
**	Errors:
**	    SQDESC 	Descriptors not read from DBMS
-*
**  Side Effects:
**	
**  History:
**	04-may-1987 - written (bjb)
**	25-may-1988 - modified to use IIsqdSetSqlvar (ncg)
**	30-aug-1988 - added adf_types to allow internal use (ncg)
**	22-may-1988 - Changed names of globals for multiple connect. (bjb)
**	6-Jun-2006 (kschendel)
**	    Fix up loop to not assume WITH NAMES, as DESCRIBE INPUT does
**	    not return names.
*/

void
IIsq_RdDesc(lang, adf_types, sqd)
i4		lang;
i4		adf_types;
IISQLDA		*sqd;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    STATUS		stat;
    ROW_DESC		*rowdesc;
    ROW_COLNAME		*rname;
    i4			i;
    bool		with_names;

    /* Initialize id and byte-count of SQLDA */
    STmove( ERx("SQLDA"), ' ', 8, sqd->sqldaid );
    sqd->sqldabc = sizeof(IISQLDA) + (sizeof(sqd->sqlvar[0]) * (sqd->sqln-1));

    /* Send PREPARE/DESCRIBE query and read results */
    IIcgc_end_msg(IIlbqcb->ii_lq_gca);	/* Send query to DBMS */
    stat = IIqry_read( IIlbqcb, GCA_TDESCR );
    if (stat == FAIL || II_DBERR_MACRO(IIlbqcb))
    {
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ006D_SQDESC, II_ERR, 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR; 
	}
	IIqry_end();
	return;
    }
    rowdesc = &IIlbqcb->ii_lq_retdesc.ii_rd_desc;
    if (IIrdDescribe(GCA_TDESCR, rowdesc) != OK)
    {
	IIlocerr(GE_LOGICAL_ERROR, E_LQ006D_SQDESC, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return;
    }
    sqd->sqld = 0;			/* None yet */

    if (rowdesc->rd_numcols == 0)	/* No descriptors: not a SELECT */
    {
	IIqry_end();
	return;
    }

    /* Assign data type info into descriptors */
    with_names = (rowdesc->rd_flags & RD_NAMES) != 0;
    if (with_names)
	rname = rowdesc->rd_names;
    for (i = 0; i < rowdesc->rd_numcols; i++)
    {
	DB_DATA_VALUE	*rdbv   = &rowdesc->RD_DBVS_MACRO(i);
	char		nmbuf[RD_MAXNAME+1];

	nmbuf[0] = '\0';
	if (with_names)
	{
	    MEcopy(rname->rd_nmbuf, rname->rd_nmlen, nmbuf);
	    nmbuf[rname->rd_nmlen] = EOS;
	    ++ rname;
	}
	if (!IIsqdSetSqlvar(lang, sqd, adf_types, nmbuf, rdbv))
	{
	    sqd->sqld = (i2)rowdesc->rd_numcols; /* # of columns from DBMS */
	    break;
	}
    }
    IIqry_end();
}
