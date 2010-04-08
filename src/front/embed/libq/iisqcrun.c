/*
** Copyright (c) 1987, 2004 Ingres Corporation
**
*/

#include	<compat.h>
#include	<me.h>		/* 6-x_PC_80x86 */
#include	<pc.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<er.h>
#include	<cm.h>
#include	<si.h>		/* needed for iicxfe.h */
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<generr.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<gca.h>
#include	<iicgca.h>
#include	<eqlang.h>
#include	<eqrun.h>
#include	<iisqlca.h>
#include	<iilibq.h>
#include	<iilibqxa.h>
#include	<iicx.h>
#include	<iicxfe.h>
#include	<erlq.h>
#include	<sqlstate.h>    /* needed for SQLSTATE defines */

/**
+*  Name: iisqcrun.c - Runtime ESQL LIBQ interface and SQLCA utility routines. 
**
**  Description:
**	The routines in this file are called at runtime by an ESQL program.
**	Most of these routines help to manage the SQLCA.  Some of them 
**	interface between the embedded program and LIBQ; some are called
**	from LIBQ.  The runtime routines which execute the dynamic statements
**	can be found in the file iisqdrun.c
**
**  Defines:
**	IIsqInit	- Set up for embedded SQL statement and SQLCA
**	IIsqlcdInit	- same as IIsqInit, but includes SQLCODE parameter.
**      IIsqGdInit      - Set up Diagnostic Area for SQL statement.
**	IIsqSetSqlCode	- Assign status info to user SQLCA & SQLCODE.
**	IIsqUser	- Saves name from "IDENTIFIED BY" clause
**	IIsqConnect	- Interface to IIdbconnect() to connect to DBMS
**	IIsqMods	- Routine to set modifiers before SQL query.
**	IIsqFlush	- Single-row SELECT interface to IIflush
**	IIsqDisconnect  - Disconnect one or all sessions.
**	IIsqStop 	- Stop program execution after SQL STOP command
**	IIsqAbort	- Print ESQL error before aborting
**	IIsqEnd		- End an embedded SQL statement
**	IIsqWarn	- Assign warning status information to SQLCA
**	IIsqSaveErr	- Copy SQL error/message code and text into SQLCA
**	IIsqPrint	- Print error text of last error (SQLPRINT).
**	IIsqEvSetCode	- Set sqlcode if events are waiting.
**
**  Notes:
-*
**  History:
**	17-apr-1987	- Written (bjb)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	12-dec-1988	- Generic error handling. (ncg)
**	19-may-1988	- Changed global var names for multiple connects. (bjb)
**	06-nov-1989	- Updated for Phoenix/Alerters. (bjb)
**	04-apr-1990	- Fixed bug 20882 in IIsqSaveErr. (bjb)
**	07-may-1990	(barbara)
**	    Set II_Q_DISCON flag when disconnecting so that events just
**	    arriving are forgotten.
**	22-may-1990	(barbara)
**	    On SQL connections, set IIlbqcb->dml to QUEL before call to
**	    IIdbconnect.  If new connection is a multiple connection, the
**	    former IIlbqcb should be left with default (QUEL) dml.  IIdbconnect
**	    will set the new IIlbqcb to the appropriate dml.
**	18-may-1990	(barbara)
**	    When disconnecting (IIsqDisconnect) don't commit or interrupt
**	    if association has already failed because it will generate an
**	    error. (bug 21832)
**	15-oct-1990	(barbara)
**	    IIsqSaveError should save error in a SELECT loop even if this
**	    is not the first error.  Otherwise, INQUIRE_INGRES ERRORNO
**	    becomes out of sync with ERRORTEXT.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	01-mar-1991 (teresal)
**	    To support ESQL DISCONNECT ALL, renamed IIsqDisconnect to be
**	    IIsqSessionDis. Created new IIsqDisconnect as an interface
**	    to IIsqSessionDis to disconnect all sessions if required.
**	06-oct-1992 (larrym) bug 47056
**		Changed IIsqDisconnect to disconnect only USER sessions on a
**		DISCONNECT ALL.  User sessions are those with a non-negative
**		session_id.  Sessions with negative session_id's are used by
**		our tools; users shouldn't be able to disconnect those.
**	03-nov-1992 (larrym)
**		Added support for SQLCODE and DIAGNOSTIC AREA (SQLSTATE):
**		Added IIsqlcdInit function, which is the same as IIsqInit, but 
**		it includes the address of SQLCODE as a parameter.  It saves
**		this address in IIlibqcb.  Also added IIsqGdInit, which 
**		initializes the ANSI SQL Diagnostic Area.
**		Added IIsqSetSqlCode to set the sqlca.sqlcode, and SQLCODE 
**		if appropriate.  Changed statements that assign
**		sqlca.sqlcode directly to now call this function.
**	15-jan-1993 (larrym) 
**	    Fixed bug with disconnect all with negative session numbers.
**      18-jan-1993 (larrym)
**          Added <si.h>.  A modification to iicxfe.h required this
**	16-jun-1993 (larrym)
**	    fixed bug 50756.  See IIsqDisconnect and IIsqSessionDis.
**	07-oct-1993 (sandyd)
**	    Moved <fe.h> ahead of <ug.h> because of new dependency on TAGID.
**      28-oct-1996 (hanch04)
**          Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Nov-2001 (horda03) Bug 106237
**          Applications whose functions mix the use of SQLCODE can have
**          miscellaneous memory corruptions, as following a call to
**          IIsqlcdInit(), the error code is written to the address
**          contained in IIlbqcb->ii_lq_sqlcdata. An application which
**          invokes IIsqInit(), hasn't got the SQLCODE defined.
**	31-may-2005 (abbjo03)
**	    In IIsqStop, change -1 argument to PCexit() to FAIL, so that VMS
**	    won't issue a % SYSTEM-F-ABORT message.
**      12-jul-2007 (stial01)
**          IIsqEvSetCode() previous sqlcode can be IISQ_MESSAGE(700) (b118612)
**/

/* 
**  Name: IIsqSetSqlCode - Set sqlca.sqlcode and SQLCODE
**
**  Description:
**	This function will set the current IIlbqcb->ii_lq_sqlca.sqlcode. Also
**	if the user has indicated that they will be using the FIPS 
**	status variable SQLCODE, then this will copy the sqlca.sqlcode into
**	the users' SQLCODE.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	errnum 		- value to set sqlca.sqlcode to.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
-*
**  Side Effects:
**	sets the sqlca.sqlcode, and optionally the SQLCODE.
**
**  History:
**	13-oct-1992 (larrym)
**		written
*/

VOID
IIsqSetSqlCode( II_LBQ_CB *IIlbqcb, i4  errnum )
{

	if (IISQ_SQLCA_MACRO(IIlbqcb))
	{
	    /* our sqlca.sqlcode */
	    IIlbqcb->ii_lq_sqlca->sqlcode = errnum;

	    /* ANSI's SQLCODE */
	    if (IIlbqcb->ii_lq_sqlcdata) 
	        *IIlbqcb->ii_lq_sqlcdata = errnum;
	}
}

/*{
**  Name: Init_SQL - Set up SQL query and SQLCA.
**
**  Description:
**	Each SQL query is preceded by this call.  This call passes in
**	the user's SQLCA (which should be the SQLCA found in the common
**	area).  The DML language is set to SQL and SQLCA error handling
**	is set.
**
**	The DML language is reset to QUEL by the counterpart of IIsqInit(), 
**	which is IIsqEnd().  The routines in LIBQ will never modify the SQLCA
**	unless the DML language is SQL and the ii_lq_sqlca field is
**	a non-null pointer.
**	
**  Inputs:
**	sqc	- User's SQLCA structure.  If it is null then assume
**		  the SQLCA was not linked in and do no special error handling.
**		  In both cases make sure the DML is set to SQL.
**
**  Outputs:
**	sqc	- The fields of the SQLCA are set throughout the query.  
**		  The first error or condition is the one that sets a
**		  particular field.
**
**		.sqlcode	-  SQL return code - 
**				   0   - Executed successfully
**				   > 0 - Successful but with an exception.
**				         IISQ_NOTFOUND = No rows satisfy
**					 a query qualification.
**				   700 - A database message returned.
**				   710 - Event information awaits.
**				   < 0 - Is the negative value of an
**				         error number.  This error number
**					 is the generic error number (in
**					 current versions of INGRES this may
**					 still be the INGRES error number).
**    		.sqlerrm	- Error message when sqlcode < 0
**		    .sqlerrml	- Length of following error message.
**		    .sqlerrmc[]	- As much of error message (if any).
**    		.sqlerrp[]	- Internal module name (unused).
**		.sqlerrd[]	- Diagnostic values -
**				     sqlerrd[0] - DBMS-specific error number.
**				     sqlerrd[2] - how many rows were physically
**						  updated (or returned) for the
**						  last query.
**		.sqlwarn	- Warning indicators - when a warning is set
**				  then the field is set to 'W' otherwise the
**				  field is blank.
**		    .sqlwarn0	- Set to W when another warning is set.
**		    .sqlwarn1	- Truncation of string into variable.
**		    .sqlwarn2	- Elimination of null values in aggregate.
**		    .sqlwarn3	- # result variables != # result columns.
**		    .sqlwarn4	- PREPAREd UPDATE or DELETE without WHERE.
**		    .sqlwarn5	- Gateway incompatible SQL statement.
**		    .sqlwarn6	- Unit of work was terminated.
**		    .sqlwarn7	- Unused.
**   		.sqlext[]	- Unused.
**		
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	1. Sets the ii_lq_dml to be SQL.  This flag can also be set 
**	   explicitly by other front-ends.
**	2. Points the global SQLCA pointer to whatever was passed in (this
**	   may be null).
**	3. Sets the error structure to save and trap errors?
**	4. Sets sqlwarn6 to T to indicate we were in a transaction before the
**	   statement began.  This allows us later (in IIsqEnd) to use:
**		if (we were in a transaction & "transaction terminated" flag &
**		    an error occurred)
**			set sqlwarn6 to W;
**	
**  History:
**	14-apr-1987	- written (bjb)
**	27-sep-1988	- added transaction checking for sqlwarn6 (ncg)
**	04-feb-1991	- (kathryn) Bug 35588
**			  Blank out any existing error message in sqlerrmc.
**	03-nov-1992 (larrym)
**	    Changed to call IIsqSetSqlCode to set the sqlcode, instead of
**	    setting it directly.  IIsqSetSqlCode also set's an ANSI user's
**	    SQLCODE too.
**	02-dec-1992 (larrym)
**	    Added support for XA.
**      28-Nov-2001 (horda03) Bug 106237
**          This was the IIsqInit function. Renamed to allow distinction
**          between invocations of SQL with and without SQLCODE defined.
*/

/*VARARGS*/
static
void
Init_SQL(sqc)
register IISQLCA	*sqc;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		i;

    if (sqc != (IISQLCA *)0)
    {
	if (sqc->sqlcode < 0)
		STmove(ERx(" "),' ',IISQ_ERRMLEN,sqc->sqlerrm.sqlerrmc);
	sqc->sqlerrm.sqlerrml = 0; /* Means ignore text, even if present */
	for (i = 0; i < 6; i++)	/* No diagnostic info yet */
	    sqc->sqlerrd[i] = 0;
	sqc->sqlwarn.sqlwarn0 = ' ';
	sqc->sqlwarn.sqlwarn1 = ' ';
	sqc->sqlwarn.sqlwarn2 = ' ';
	sqc->sqlwarn.sqlwarn3 = ' ';
	sqc->sqlwarn.sqlwarn4 = ' ';
	sqc->sqlwarn.sqlwarn5 = ' ';
	sqc->sqlwarn.sqlwarn6 = (IIlbqcb->ii_lq_flags & II_L_XACT) ? 'T' : ' ';
	sqc->sqlwarn.sqlwarn7 = ' ';
    }
    /*
    ** Set the SQLCA to whatever was passed in.  This will avoid us using
    ** an old SQLCA pointer with later SQL calls which do NOT use an SQLCA
    ** (ie, our own front-ends).  If this pointer is null, then the internal
    ** SQLCA will be reset.
    */
    IIlbqcb->ii_lq_sqlca = sqc;
    IIlbqcb->ii_lq_dml = DB_SQL;

    /* Initialize sqlca and SQLCODE to no-error state */
    IIsqSetSqlCode( IIlbqcb, IIerOK );

    /* 
    ** XA stuff:  XA apps can only execute DML within context of an XA
    ** transaction.  This checks for compliance.
    */
    if (IIXA_CHECK_IF_XA_MACRO)
    {
	/* make sure we're in the proper state */
	IIXA_CHECK_SET_INGRES_STATE(
	      (IICX_XA_T_ING_INTERNAL | IICX_XA_T_ING_ACTIVE) );
	/* errors are caught and reported by the above function */
    }
}

/*{
** Name: IIsqInit - Initialise SQL query structures.
**
** Description:
**     See description of Init_SQL.
**
**     Each SQL query which does not have SQLCODE defined is preceded by
**     this call. It clears the SQLCODE address, before completing the
**     SQL query initialisation.
**
**  Inputs:
**      sqc     - User's SQLCA structure.  If it is null then assume
**                the SQLCA was not linked in and do no special error handling.
**                In both cases make sure the DML is set to SQL.
**
**  Outputs:
**      sqc     - The fields of the SQLCA are set throughout the query.  
**                The first error or condition is the one that sets a
**                particular field.
**
**  History:
**      28-Nov-2001 (horda03) Bug 106237
**             Created.
**	30-May-2002 (toumi01)
**		Modify "new" IIsqInit function for thread-safe libq.
*/
void
IIsqInit(sqc)
register IISQLCA        *sqc;
{
   II_THR_CB	*thr_cb = IILQthThread();
   II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

   /* Clear address of SQLCODE */
   IIlbqcb->ii_lq_sqlcdata = NULL;

   /* Initialise structures */
   Init_SQL(sqc);
}

/* 
**  Name: IIsqlcdInit - Save address of SQLCODE and call Init_SQL
**
**  Description:
**	If the user has indicated that they will be using the FIPS 
**	deprecated SQLCODE, then we will generate calls to IIsqlcdInit
**	instead of IIsqInit.  IIsqlcdInit has an additional argument to allow
**	passing the address of SQLCODE which we save.  Then the function 
**	calls Init_SQL.
**
**  Inputs:
**	sqc	- pointer to user's sqlca. 
**	sqlcd	- pointer to user's SQLCODE.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
-*
**  Side Effects:
**	Saves address of SQLCODE in IIlbqcb.
**	Also calls Init_SQL which has side effects (see above)
**
**  History:
**	12-oct-1992 (larrym)
**		written
*/

VOID
IIsqlcdInit(sqc,sqlcd)
register IISQLCA	*sqc;
i4			*sqlcd;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

	/* save address of SQLCODE */
	if (sqlcd) 
	{
	    IIlbqcb->ii_lq_sqlcdata = sqlcd;
	}
	/* now call Init_SQL. */
	Init_SQL(sqc);
}

/* 
**  Name: IIsqGdInit - - Set up Diagnostic Area for SQL statement.
**
**  Description:
**	If the user has indicated that they will be using the FIPS SQLSTATE
**	status variable (or the Diagnostic Area when we support it) then we
**	generate calls to this routine before EVERY SQL statement, not just
**	the ones that use WHENVER stuff.  This function initializes the 
**	DA (Diagnostic Area) and also passes the address of the user's 
**	SQLSTATE variable.
**
**  Inputs:
**	sqlsttype	- host data type of user's SQLSTATE variable.
**	sqlstdata	- address of the user's SQLSTATE.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
-*
**  Side Effects:
**	Initializes the ANSI SQL Diagnostic Area and sets RETURNED_SQLSTATE
**	to SS00000_SUCCESS.  If no errors occur, then it will be set 
**	correctly.  This may change in future releases.
**
**  History:
**	03-nov-1992 (larrym)
**		written
*/

VOID
IIsqGdInit( sqlsttype, sqlstdata)
i2		sqlsttype;
char		*sqlstdata;
{
    II_THR_CB *thr_cb = IILQthThread();

    thr_cb->ii_th_da.ii_da_usr_sqlsttype = sqlsttype;
    thr_cb->ii_th_da.ii_da_usr_sqlstdata = sqlstdata;
    thr_cb->ii_th_da.ii_da_infostate = II_DA_INIT;
    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, SS00000_SUCCESS );

    return;
}


/*{
+*  Name: IIsqUser - Save username from IDENTIFIED BY clause
**
**  Description:
**	Save "-u" flag followed by 'username' in a (static) global buffer 
**	for use by IIsqConnect as a flag to IIdbconnect(). 
**
**  Inputs:
**	usename		- Name qualifying "IDENTIFIED BY" clause
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Preprocessor generated code:
**	EXEC SQL CONNECT dbname IDENTIFIED BY username	;
**
**	IIsqInit(&sqlca);
**	IIsqUser("username");
**	IIsqConnect(0, "dbname", (char *)0);
-*
**  Side Effects:
**	Assigns value to global buffer.
**	
**  History:
**	22-apr-1987 - written (bjb)
*/

void
IIsqUser( usename )
char	*usename;
{
    II_THR_CB	*thr_cb = IILQthThread();

    thr_cb->ii_th_user[0] = '-';
    thr_cb->ii_th_user[1] = 'u';
    STlcopy( usename, &thr_cb->ii_th_user[2], LQ_MAXUSER );
}


/*{
+*  Name: IIsqConnect - Interface between ESQL program and IIdbconnect().
**
**  Description:
**	This routine sets the DML to SQL and passes on its parameters to 
**	IIdbconnect.  These parameters have been generated by the preprocessor 
**	for the "OPTIONS" clause on a "CONNECT" statement.  In addition,
**	IIsqConnect passes the "-u<username>" flag if a username has been saved
**	away by IIsqUser, and the "-$" flag to indicate to the DBMS that
**	the client is a FE process.
**
**  Inputs:
**	lang		Language (for historical reasons)
**	db		Database name
**	a1 thru a13	Arguments from OPTIONS clause
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    Errors starting up INGRES.  (Errors other than OS errors
**		will have been processed by IIdbConnect().)
**
**  Preprocessor generated code:
**	EXEC SQL CONNECT "dbname" OPTIONS = "-w" "-t"
**
**	IIsqInit(&sqlca);
**	IIsqConnect(EQ_C, "dbname", "-w", "-t", (char *)0);
**
**	EXEC SQL CONNECT "dbname" WITH :with_clause;
**
**	IIsqInit(&sqlca);
**	IIsqMods(IImodWITH);
**	IIsqConnect(EQ_C, "dbname", (char *)0);
**	IIsqParms(IIsqpINI, with_clause, DB_CHR_TYPE, (char *)0, 0);
**	IIsqParms(IIsqpEND, (char *)0, 0, (char *)0, 0);
**
**  Notes:
**	XA state checking is done in IIdbconnect.
-*
**  Side Effects:
**	May reset global buffer.
**	
**  History:
**	22-apr-1987 - written (bjb)
**	05-feb-1988 - added check for WITH clause. (ncg)
**	12-dec-1988 - now also may set sqlerrd[0]. (ncg)
**	22-may-1989 - Changed names of globals for multiple connects. (bjb)
**	22-may-1990	(barbara)
**	    On SQL connections, set IIlbqcb->dml to QUEL before call to
**	    IIdbconnect.  If new connection is a multiple connection, the
**	    former IIlbqcb should be left with default (QUEL) dml.  IIdbconnect
**	    will set the new IIlbqcb to the appropriate dml.
**	05-dec-1990 (barbara)
**	    Reset dml to QUEL inside IIdbconnect instead of here otherwise
**	    errors on creating a session don't get SQL semantics.
**	03-nov-1992 (larrym)
**	    Added SQLCODE and SQLSTATE support.  For the same reasons that
**	    we used to set sqlca->sqlcode directly, we now call IIsqSetSqlCode
**	    to set the sqlcode(s) and IILQdaSIsetItem to "set" the SQLSTATE.
**	07-jan-1993 (larrym)
**	    Removed parameter a14 to conform to code generated call to 
**	    IIsqConnect.   Also changed call to IIdbconnect to have a
**	    consistent number of params (18 total).
*/

void
IIsqConnect( lang, db, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
	   a12, a13 )
i4	lang;
char	*db, *a1, *a2, *a3, *a4, *a5, *a6, *a7,
	*a8, *a9, *a10, *a11, *a12, *a13;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( thr_cb->ii_th_user[0] != '\0')
    {
	IIdbconnect( DB_SQL, lang, db, ERx("-$"), thr_cb->ii_th_user, 
		     a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 );
	thr_cb->ii_th_user[0] = '\0';
    }
    else
    {
	IIdbconnect( DB_SQL, lang, db, ERx("-$"), 
		     a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, 
		     (PTR)0 );
    }

    /*
    ** Currently, a failed CONNECT due to an O/S error will have caused an
    ** error message to be printed directly to output by IIdbConnect() and 
    ** IIlocerr() will have  been bypassed.  Consequently, sqlca.sqlcode will 
    ** not have been set in this case.  Therefore, we explicitly set 
    ** sqlca.sqlocde in the case of a CONNECT which failed due to an O/S error.
    ** We also now explicitly set the RETURNED SQLSTATE in the Diagnostic Area
    ** (DA).
    */
    if (    IISQ_SQLCA_MACRO(IIlbqcb)
	&&  IIlbqcb->ii_lq_error.ii_er_num
	&&  (IIlbqcb->ii_lq_sqlca->sqlcode == 0)
       )
    {
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, 
			 SS08001_CANT_GET_CONNECTION);
	IIsqSetSqlCode( IIlbqcb, -IIlbqcb->ii_lq_error.ii_er_num );
	IIlbqcb->ii_lq_sqlca->sqlerrd[0] = IIlbqcb->ii_lq_error.ii_er_other;
    }

    /* 
    ** Reset dml.  All other queries go through IIqry_end() to reset the
    ** dml but database connect should not go through IIqry_end() because 
    ** it would cause some query stuff to be done (inappropriately at startup).
    **
    ** If we are processing a WITH clause, then the CONNECT-time code 
    ** returns to the caller at the end of IIsqParms, so the dml reset is
    ** done there.
    */
    if ( ! (thr_cb->ii_th_flags & II_T_WITHLIST) )
	IIlbqcb->ii_lq_dml = DB_QUEL;
}


/*{
**  Name: IIsqMods - Set GCA Modifiers before SQL query.
**
**  Description:
**	In some cases we need to set modifiers before sending a query through
**	the GCA interface.  This routine is called to set some LIBQ flags which
**	can later turn on specific GCA modifiers later on.
**
**	Currently the routine is called before the following situations:
**
**	 1. Before a singleton SELECT in order to notify GCA that this is
**	    not a SELECT loop (the INGRES default).  The input value is
**	    IImodSINGLE.
**	 2. Before a CONNECT statement with a WITH clause.  This informs
**	    LIBQ to tell GCA not to send the initial MD_ASSOC parameter,
**	    and to "wait" till the rest of the WITH clause.  The input
**	    value is IImodWITH.
**	 3. Before a EXECUTE IMMEDIATE of a SELECT statement to notify
**	    IIsqExImmed query is a SELECT.
**	 4. The Windows/4GL cursor implementation uses IImodNAMES before
**	    doing an open.
**	 5. Before a database retrieval statement if the "-blank_pad"
**	    preprocessor flag is on.
**	 6. Before any database statement that is sending a host variable
**	    to the server iff the "-check_eos" preprocessor flag is on.
**
**	When we do have more modifiers to set then the values passed from
**	the generated code should be defined in eqrun.h and we can map
**	them to local values later within LIBQ.
**
**  Inputs:
**	mod_flag		Modifier value defined in eqrun.h.
**				0           - Turn off any special flags.
**				IImodSINGLE - Flag singleton SELECT.
**				IImodWITH   - Flag WITH clause.
**				IImodDYNSEL - Flag dynamic SELECT.
**				IImodNAMES  - Cause column names to
**					      be gotten in descriptors.
**				IImodCPAD   - Cause "C" char variables to
**					      be blank padded to conform to 
**					      ANSI.
**				IImodNOCPAD - "C" char vars are null terminated 
**					      not blank padded - Ingres default.
**				IImodCEOS   - Look for EOS in "C" fixed length
**				              char vars - ANSI requirement.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Preprocessor generated code:
**	For IImodSINGLE see IIsqFlush().
**	For IImodWITH see IIsqConnect().
**	For IImodDYNSEL see IIsqExImmed().
**
**  Side Effects:
**	1. Sets IIlbqcb->ii_lq_flags to II_L_SINGLE so that IIqry_start
**	   will use it and turn it off... or
**	2. Sets IIglbcb->ii_gl_flags to II_G_WITH so that IIsqParms
**	   will use it and turn it off... or
**	3. Sets IIlbqcb->ii_lq_flags to II_L_DYNSEL for use by IIsqExImmed
**	   and IIcsDaGet.  Turned off in IIqry_end().
**	   will use it and turn it off... or
**	4. Sets IIglbcb->ii_gl_flags to II_L_NAMES so that IIqry_start
**	   will use it and turn if off.
**      5. Sets IIglbcb->ii_gl_flags to II_L_CHRPAD so that adh_chrcvt will
**	   will blank pad DB_CHR_TYPE to the length of the host variable - 1
**         and then EOS terminate.  This is required behavior for ANSI.
**	   an ESQL startup flag will generate IIsqMods pairs around
**	   groups of calls that get data (select, fetch, etc...).
**	6. Unsets the II_L_CHRPAD flag in IIglbcb->ii_gl_flags. 
**	
**  History:
**	20-aug-1987 - written (ncg)
**	05-feb-1988 - added IImodWITH (ncg)
**	05-sep-1989 - added IImodDYNSEL (barbara)
**	13-sep-1990 - Added support for IImodNAMES. (Joe)
**	07-jun-1993 (kathryn)
**	    Added support for IImodCPAD and IImodNOCPAD for FIPS.
**	17-nov-1993 (teresal)
**	    Add support for IImodCEOS and IImodNOCEOS for FIPS. Bug 55915.
*/

void
IIsqMods(mod_flag)
i4	mod_flag;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if (mod_flag == IImodSINGLE)
	IIlbqcb->ii_lq_flags |= II_L_SINGLE;
    else if (mod_flag == IImodWITH)
	thr_cb->ii_th_flags |= II_T_WITHLIST;
    else if (mod_flag == IImodDYNSEL)
	IIlbqcb->ii_lq_flags |= II_L_DYNSEL;
    else if (mod_flag == IImodNAMES)
	IIlbqcb->ii_lq_flags |= II_L_NAMES;
    else if (mod_flag == IImodCPAD)
	IIlbqcb->ii_lq_flags |= II_L_CHRPAD;
    else if (mod_flag == IImodNOCPAD)
	IIlbqcb->ii_lq_flags &= ~II_L_CHRPAD;
    else if (mod_flag == IImodCEOS)
	IIlbqcb->ii_lq_flags |= II_L_CHREOS;
    else if (mod_flag == IImodNOCEOS)
	IIlbqcb->ii_lq_flags &= ~II_L_CHREOS;
}

/*{
**  Name: IIsqFlush - Single-row interface to IIflush for SELECT.
**
**  Description:
**	This routine makes sure that there is at most one row returning
**	from a singleton SELECT.  It is generated at the end of a 
**	singleton SELECT statement after a row of data has been processed.
**	If only one row is returning, this routine falls through to IIflush()
**	to end off the query; if more than one row is returning, an error is 
**	recorded and IIbreak() is called to interrupt the results of the query.
**
**  Inputs:
**	filename		File name for debugging.
**	linenum			Line number for debugging.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    SQMULTSEL More that one row for singleton SELECT.
**
**  Preprocessor generated code:
**	EXEC SQL SELECT col1 INTO :ivar FROM t1;
**
**	IIsqInit(&sqlca);
**	IIsqMods(IImodSINGLE);
**	IIwritedb("select col1 from t1");
**	IIretinit();
**	if IInextget()) {
**	    IIgetdomio((char *)0,TRUE, DB_INT_TYPE, sizeof(ivar), &ivar);
**	}
**	IIsqFlush((char *)0, 0);
**
**  Side Effects:
**	Sets IIlbqcb->ii_lq_rowcount to 1 so that IIsqEnd will correctly
**	set sqlca.sqlerrd(3).
**	
**  History:
**	15-apr-1987 - written (bjb)
*/

void
IIsqFlush(filename, linenum)
char	*filename;
i4	linenum;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    /*
    ** ii_rd_colnum <= 0 if  (1) an error occurred in IIretinit() or
    ** (2) no rows were returned for this query.  In both these cases,
    ** we know that a multi-select has not happened.  In the latter case
    ** we specifically don't want to call IInextget() here otherwise
    ** it will incorrectly report the RETCOLS error based on
    ** ii_rd_colnum having a value of 0.
    */
    if ((IIlbqcb->ii_lq_retdesc.ii_rd_colnum > 0) && (IInextget() != 0))
    {
	IIlocerr(GE_CARDINALITY, E_LQ006A_SQMULTSEL, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_rowcount = 1;
	IIbreak();
    }
    else
    {
	IIflush( filename, linenum );
    }
}


/*{
+*  Name: IIsqSessionDis - Commit work and end the session with DBMS.
**
**  Description:
**	The default SQL action is to commit any work at the end of a
**	session.  IIsqSessionDis() sends a special "commit" command to 
**	the DBMS to override the default action of the INGRES DBMS, which 
**	is to rollback.  IIsqSessionDis() then call IIexit(), which 
**	terminates the session.
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None.
**  Notes:
**	XA state checking is done in IIsqDisconnect
-*
**  Side Effects:
**	The LIBQ global flags will be reset.
**	
**  History:
**	27-apr-1987 - written (bjb)
**	30-aug-1988 - modified to check for in_transaction before sending
**		      COMMIT.  This avoids the extra overhead of the common
**		      case: EXEC SQL COMMIT;
**			    EXEC SQL DISCONNECT; (ncg)
**	19-sep-1989 - Fixed bug #8139.  If disconnecting from a user error
**		      handler called while executing a database procedure,
**		      call IIbreak.  Can't rely on II_Q_INQRY flag being
**		      set because queries issued in the error handler
**		      (even though in error) will have reset that flag.(barbara)
**	07-may-1990	(barbara)
**	    Set II_Q_DISCON flag when disconnecting so that events just
**	    arriving are forgotten.
**	18-may-1990	(barbara)
**	    Don't commit or interrupt if association has already failed
**	    because attempts to communicate to dbms will generate errors.
**	    (bug 21832)
**	01-mar-1991	(teresal)
**	    Changed name of routine from IIsqDisconnect to IIsqSessionDis.
**	    IIsqDisconnect now calls IIsqSessionDis so a DISCONNECT ALL
**	    can be done from one level up.
*/

static VOID
IIsqSessionDis( II_THR_CB *thr_cb )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( IIlbqcb != &thr_cb->ii_th_defsess )
    {
	IIlbqcb->ii_lq_curqry |= II_Q_DISCON;

	if ( IIlbqcb->ii_lq_flags & II_L_CONNECT )
        {
	    if (   IIlbqcb->ii_lq_flags & II_L_RETRIEVE
	        || IIlbqcb->ii_lq_flags & II_L_DBPROC
	        || IIlbqcb->ii_lq_curqry & II_Q_INQRY)
	    {
	        IIbreak();
	        IIlbqcb->ii_lq_dml = DB_SQL;    /* IIbreak() resets DML */
	    }
	    if (IIlbqcb->ii_lq_flags & II_L_XACT)
	    {
	        if (!IIXA_CHECK_IF_XA_MACRO)	/* only commit non-XA xctns */
	        {
		    IIxact(IIxactSCOMMIT);
	        }
	        /* else, error CHECK */	
	    }
	    IIlbqcb->ii_lq_dml = DB_SQL;    /* IIxact() resets DML */
        }
	else
	{
	    /* association has failed, set SQLSTATE to say so */
	    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, 
			     SS08006_CONNECTION_FAILURE );
	}
    }
    else
    {
	/* not connected, set SQLSTATE to say so */
	IILQdaSIsetItem(thr_cb, IIDA_RETURNED_SQLSTATE, SS08003_NO_CONNECTION); 
    }

    IIexit();			/* Will reset flags and call IIqry_end() */
    return;
}


/*{
+*  Name: IIsqDisconnect - Interface to IIsqSessionDis to end one session
**			   or all sessions.
**
**  Description:
**	Call IIsqSessionDis to disconnect just one session (either the
**	current session or a user specified session),
**	otherwise, if a DISCONNECT ALL was given, loop through all
**	sessions disconnecting each one.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None.
**  Preprocessor generated code:
**	EXEC SQL DISCONNECT
**
**	IIsqInit(&sqlca);
**	IIsqDisconnect();
-*
**  Side Effects:
**	The LIBQ global flags will be reset.
**	
**  History:
**	01-mar-1991 - written. Note old IIsqDisconnect is now called
**		      IIsqSessionDis. (teresal)
**	25-mar-1991 - Switch sessions by calling IILQssSetSqlio
**		      (SET_INGRES interface). (teresal)
**	06-oct-1992 (larrym) bug 47056
**	    Changed IIsqDisconnect to disconnect only USER sessions on a
**	    DISCONNECT ALL.  User sessions are those with a non-negative
**	    session_id.  Sessions with negative session_id's are used by
**	    our tools; users shouldn't be able to disconnect those.
**	04-nov-1992 (larrym)
**	    For single connects (i.e., not DISCONNECT ALL) check for valid
**	    connection; if not connected in specified session, gen an SQLSTATE.
**	05-nov-1992 (larrym)
**	    Added SQLSTATE support.  If trying to disconnect a specific 
**	    session, and it doesn't exist, then need to generate SQLSTATE
**	    exception.
**	02-dec-1992 (larrym)
**	    added XA support.  XA app's can't disconnect unless done so
**	    by (through?) libqxa.
**	02-mar-1993 (larrym)
**	    Modified for XPG4 connections.  All sessions are multi.  If a user
**	    tries to disconnect from a session that is not current, we'll 
**	    switch to it, disconnect, then switch back.
**	15-jun-1993 (larrym)
**	    Fixed bug 50756.  You can now disconnect from a session that
**	    is not your current one.  Modified  iilqsess.c code so that 
**	    switching to a non-existent session doesn't generate an error.
**	    we generate that error now (as a bad disconnect, not a bad
**	    session switch).
**	23-Aug-1993 (larrym)
**	    Fixed bug 54432; we weren't reseting IIglbcb->ii_gl_flags |= 
**	    II_G_CONNECT after switching back to original session after
**	    a disconnect session (other session);
**	28-Oct-1993 (donc)
**	    Fixed bug 47881; After attempting to disconnect from a non-        
**	    existent session, ii_gl_sstate->ss_idsave continued to hold
**          the value for the non-existent session.  This caused subsequent
**	    disconnects to fail with an error indicating that the valid session
**	    was non-existent.  If one tries to disconnect from an invalid 
**	    session, ii_gl_sstate->ss_idsave will be reinitialized to 0. 
*/
void
IIsqDisconnect()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*savelqp; /* Pointer to an II_LBQ_CB list element */
    II_LBQ_CB	*dislqp;  /* Pointer to an II_LBQ_CB list element */

    /* XA stuff:  Disconnects only allowed from INTERNAL XA call */
    if (IIXA_CHECK_IF_XA_MACRO)
    {
	if (IIXA_CHECK_SET_INGRES_STATE(IICX_XA_T_ING_INTERNAL))
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ00D5_XA_ILLEGAL_STMT, 
		     II_ERR, 0, (PTR)0);
	    return;
	}
    }

    /* check for attempt to disconnect from session 0 (illegal) */
    if ( thr_cb->ii_th_flags & II_T_SESSZERO )
    {
	IIlocerr(GE_LOGICAL_ERROR, E_LQ00BC_SESSZERO, II_ERR, 0, (PTR)0);
        thr_cb->ii_th_flags &= ~II_T_SESSZERO;
	return;
    }

    if ( thr_cb->ii_th_sessid == IIsqdisALL )
    {
	/*
	** Make sure all sessions are in a state to
	** be disconnected.  Only one current session
	** is permitted and it must belong to this
	** thread.  Internal tool sessions (negative
	** session IDs) are ignored.
	*/
	if ( MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) != OK )
	    dislqp = thr_cb->ii_th_session;
	else
	{
	    for( 
		 dislqp = IIglbcb->ii_gl_sstate.ss_first; 
		 dislqp; 
		 dislqp = dislqp->ii_lq_next 
	       )
	    {
		if ( 
		     dislqp->ii_lq_sid > 0  &&
		     dislqp->ii_lq_flags & II_L_CURRENT  &&  
		     dislqp != thr_cb->ii_th_session
		   )  
		{
		    break;
		}
	    }

	    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
	}

	if ( dislqp )
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ00CA_SESS_ACTIVE, II_ERR, 0, NULL);
	    return;
	}

	/*
	** Disconnect all sessions (one at a time).
	** Loop ends when we fail to find a session
	** which may be disconnected.
	*/
	for(;;)
	{
	    bool active;

	    if ( MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) != OK )
		break;	/* should not happen */

	    /*
	    ** Search for a session which can be disconnected.
	    ** Inactive sessions or our current session can be
	    ** disconnected.  Internal sessions are ignored.
	    ** There should not be any active sessions other
	    ** than our own since we tested for this above,
	    ** but on multi-threaded systems its possible for 
	    ** some other thread to activate a session while
	    ** we are in the processes of disconnecting
	    ** sessions.  So active sessions are also ignored
	    ** unless there are we find no other session to
	    ** disconnect, in which case an error is created.
	    ** 
	    */
	    for( 
		 dislqp = IIglbcb->ii_gl_sstate.ss_first, active = FALSE; 
		 dislqp; 
		 dislqp = dislqp->ii_lq_next 
	       )
	    {
		if ( dislqp->ii_lq_sid <= 0 )
		    continue;		/* Internal sessions ignored */
		else  if ( ! (dislqp->ii_lq_flags & II_L_CURRENT)  ||
			   dislqp == thr_cb->ii_th_session )
		    break;		/* Disconnect this session */
		else
		{
		    active = TRUE;	/* Error if no available session */
		    continue;		/* Active sessions ignored */
		}
	    }

	    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );

	    if ( dislqp )
	    {
		/*
		** Failure to switch to the session
		** indicates an active session which
		** we ignore for now.  The active
		** session will be handled in the
		** search loop above.
		*/
		if ( IILQssSwitchSession( thr_cb, dislqp ) == OK )
		    IIsqSessionDis( thr_cb );
	    }
	    else
	    {
		/*
		** There are no more sessions to be
		** disconnected.  There should not
		** be any active sessions remaining.
		*/
		if ( active )
		    IIlocerr( GE_LOGICAL_ERROR, 
			      E_LQ00CB_SESS_ACTIVE, II_ERR, 0, NULL );
		break;
	    }
	}
    }
    else
    {
	/*
	** Disconnecting current session or session
	** identified by session ID or connection name.
	*/
	if ( thr_cb->ii_th_flags & II_T_CONNAME )
	{
	    /* Disconnect by connection name. */
	    dislqp = IILQscConnNameHandle( thr_cb->ii_th_cname );
	    thr_cb->ii_th_flags &= ~II_T_CONNAME;
	}
	else  if ( thr_cb->ii_th_sessid )
	{
	    /* Disconnect by session ID. */
	    dislqp = IILQfsSessionHandle( thr_cb->ii_th_sessid );
	    thr_cb->ii_th_sessid = 0;
	}
	else
	{
	    /* Disconnect current session */
	    dislqp = thr_cb->ii_th_session;
	}

	if ( ! dislqp )
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ00BF_SESSNOTFOUND, II_ERR, 0, NULL);
	else  if ( thr_cb->ii_th_session == dislqp )
	    IIsqSessionDis( thr_cb );		/* Disconnect our session */
	else
	{
	    /*
	    ** Need to temporarily switch to target session.
	    */
	    savelqp = thr_cb->ii_th_session;

	    if ( IILQssSwitchSession( thr_cb, dislqp ) != OK )
		IIlocerr( GE_LOGICAL_ERROR, 
			  E_LQ00CA_SESS_ACTIVE, II_ERR, 0, NULL );
	    else
	    {
		IIsqSessionDis( thr_cb );
		IILQssSwitchSession( thr_cb, savelqp );
	    }
	}
    }

    return;
}

/*{
+*  Name: IIsqStop - Stop program execution after SQL STOP command
**
**  Description:
**	IIsqStop() is called because the embedded program has indicated
**	(via the WHENEVER statement) that it wants to stop on errors.
**	Before exiting, this routine prints "SQL STOP" followed by the
**	number and text of the latest error message. 
**
**  Inputs:
**	sqc		Embedded program's SQLCA structure (may be null)
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None
**
**  Preprocessor generated code:
**	EXEC SQL WHENEVER SQLERROR STOP;
**	EXEC SQL <statement>
**	
**	IIsqInit(&sqlca);
**	Various "II" calls to execute statement;
**	if (sqlca.sqlcode < 0) IIsqStop(&sqlca);
-*
**  Side Effects:
**	None.
**	
**  History:
**	27-apr-1987 - written (bjb)
**	12-dec-1988 - uses IImsgutil to display errors - from gateway. (ncg)
**	05-jan-1989 - Display event if stopping whenever event. (bjb)
*/

void
IIsqStop(sqc)
register IISQLCA	*sqc;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    II_ERR_DESC	*lbqerr = &IIlbqcb->ii_lq_error;

    /* 
    ** XA stuff.  Disallow WHENEVER SQLERROR STOP for XA.
    */

    if (IIXA_CHECK_IF_XA_MACRO)
    {
	/* error CHECK */
	return;
    }

    IIlbqcb->ii_lq_sqlca = sqc;
    IIlbqcb->ii_lq_dml = DB_SQL;
    if (sqc != (IISQLCA *)0) 
    {
	if (sqc->sqlcode == IISQ_EVENT)
	{
	    IILQedEvDisplay( IIlbqcb, IIlbqcb->ii_lq_event.ii_ev_list );
	    IILQefEvFree( IIlbqcb );
	}
	else if (   sqc->sqlcode < 0
	         && -sqc->sqlcode == lbqerr->ii_er_snum
	         && lbqerr->ii_er_smsg)
	{
	    IImsgutil(lbqerr->ii_er_smsg);
	}
    }

    /*
    ** IIabort may indirectly dump any still-saved error messages from
    ** the SQLCA.  By reseting the SQLCA here, we avoid the reprinting
    ** of the last message.
    */
    IIlbqcb->ii_lq_sqlca = (IISQLCA *)0;
    /* 
    ** "SQL STOP" followed by the text of the last error gets printed.
    ** It's not in the message file because it shouldn't be translated.
    ** (The words "SQL" and "STOP" are key words.)
    */
    IImsgutil(ERx("SQL STOP.\n"));
    IIabort();
    PCexit( FAIL );
}


/*{
+*  Name: IIsqAbort - Print latest ESQL error message, if any, before exiting.
**
**  Description:
**	This routine is called from IIabort() before the DBMS is disconnected
**	(usually because something serious like a syserr occurred).  If the 
**	error has been logged as an SQL error (i.e., the "sqlcode" field in
**	the users SQLCA is negative), IIsqAbort() arranges for the error 
**	number and text to be be printed out.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    Any saved LIBQ errors will be printed. 
-*
**  Side Effects:
**	None.
**	
**  History:
**	27-apr-1987 - written (bjb)
**	12-dec-1988 - uses IImsgutil to display errors - from gateway. (ncg)
*/

VOID
IIsqAbort( II_LBQ_CB *IIlbqcb )
{
    register	II_ERR_DESC	*lbqerr = &IIlbqcb->ii_lq_error;
    register	i4 	sqcode = IIlbqcb->ii_lq_sqlca->sqlcode;
    
    if (    (sqcode < 0) 
	&&  (-sqcode == lbqerr->ii_er_snum)
	&&  (lbqerr->ii_er_smsg != (char *)0)
       )
    {
	IImsgutil(lbqerr->ii_er_smsg);
    }
}
    

/*{
+*  Name: IIsqEnd - End an embedded SQL statement - complement of IIsqInit()
**
**  Description:
**	IIsqEnd() assumes that it is only called if the current statement
**	an SQL statement and the embedded program has used an SQLCA.  It 
**	sets status information in the SQLCA as follows:
**
**	Note that references to sqlca.sqlerrd are in user terms and not in
**	C terms (ie, zero based).  For example, sqlerrd(3) means sqlerrd[2]
**	in C.
**
**	Rowcount information.  
**	-   sqlca.sqlerrd(3) indicates to the embedded program how many rows 
**	    were processed or returned.  This field is to be set after a 
**	    DELETE, FETCH, INSERT, SELECT, UPDATE, COPY INTO/FROM, and CREATE 
**	    AS SELECT.   This routine assumes that 'ii_lq_rowcount' reflects 
**	    the number of rows processed by all of the above statements and 
**	    contains a negative value after all other SQL statements.  In 
**	    other words, sqlca.sqlerrd(3) can rely on ii_lq_rowcount.	
**	 -  sqlca.sqlcode = IISQ_NOTFOUND (100) indicates to the embedded 
**	    program that no rows satisfied one of the above statements.  This
**	    status information is set only if the SQL statement executed
**	    without any error.
**      -   Note that rowcount information is not saved if this is a
**	    nested query because any such information belongs to the "parent"
**	    query.
**
**	DBMS status information.
**	-   sqlca.sqlwarn.sqlwarn2 indicates to the embedded program that null
**	    values were eliminated from aggregates.
**	-   sqlca.sqlwarn.sqlwarn4 indicates that an UPDATE or DELETE statement
**	    was PREPAREd without a WHERE clause.
**	-   sqlca.sqlwarn.sqlwarn5 indicates that a legal SQL statement was
**	    sent to an incompatible DBMS (gateway).
**	-   sqlca.sqlwarn.sqlwarn6 indicates that a transaction was terminated
**	    (explicitly or implicitly).
**	-   These pieces of status information are found in 'ii_lq_qrystat',
**	    copied from a status variable in GCA.
**
**	Miscellaneous DBMS/Gateway information.
**	-   sqlca.sqlerrd(4) reports the cost of a query after PREPARE.
**	-   sqlca.sqlerrd(*), sqlca.sqlerrp and sqlca.sqlext are collected
**	    from the response structure.
**	    
**	Note that the embedded program's SQLCA is not specifically released
**	by this routine.  There are instances where holding on to it is
**	useful, e.g. when ending a (illegal) nested query -- the outer
**	RETRIEVE/SELECT should not lose its pointer to the SQLCA.  The
**	risk of not releasing the SQLCA is in the case of a program
**	switching to an SQL statement that is not using an SQLCA and has not 
**	gone through IIsqInit().  Preprocessed code will never do this;
**	however, internal code might do this.  All internal products using
**	SQL must used IIsqInit before every SQL statement, or at least
**	set IIlbqcb->ii_lq_sqlca.
**	
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**
**	IIlbqcb.ii_lq_rowcount  		Number of rows processed
**	IIlbqcb.ii_lq_qrystat 			Query status information
**	IIlbqcb.ii_lq_retdesc.ii_rd_flags 	Nested query information
**	IIlbqcb.ii_lq_retdesc.ii_rd_flags 	Nested query information
**	IIlbqcb.ii_lq_gca.cgc_resp		Gateway information
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	Assigns status information to user's SQLCA (see above).
**	
**  History:
**	13-apr-1987 - written (bjb)
**	23-jun-1988 - added more status reporting for gateways. (ncg)
**	01-sep-1988 - checked if gca is set before retrieving data.  User may
**		      never have sucessfully entered DB. (ncg)
**	27-sep-1988 - added transaction checking for sqlwarn6 (ncg)
**	12-dec-1988 - sqlerrd[0] is no longer reset as may hold error #. (ncg)
**	15-dec-1989 - Call IIsqSetEvCode to set event status (bjb)
**	03-nov-1992 (larrym)
**	    Added SQLCODE and SQLSTATE support.  Set's each for the 
**	    not_found/no_data condition.
**	21-Apr-1994 (larrym)
**	    Fixed bug 59216.  IIqry_end now calls IIsqEnd unconditionally
**	    (it used to only call it when the user had an SQLCA) so it
**	    can set SQLSTATE. Anything that tries to touch the SQLCA now
**	    checks for it's existence.
**      15-Jan-2009 (coomi01) b121331
**          select first N : was returning N-1 row. 
*/

VOID
IIsqEnd( II_THR_CB *thr_cb )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IISQLCA	*sql = IIlbqcb->ii_lq_sqlca;
    i4		rowcount = IIlbqcb->ii_lq_rowcount;	

    if ((IIlbqcb->ii_lq_retdesc.ii_rd_flags & II_R_NESTERR) == 0)
    {
	if ((IISQ_SQLCA_MACRO(IIlbqcb)) && (rowcount >= 0))
	{
	    sql->sqlerrd[2] = rowcount;
	}
	if ((rowcount == 0) || (IIlbqcb->ii_lq_qrystat & GCA_END_QUERY_MASK))
	{

	    /* Set SQLSTATE, SQLCODE, and sqlca.sqlcode to Not Found */
	    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, SS02000_NO_DATA ); 
	    
	    /* 
	    ** Set sqlcode to Not Found when the buffer is empty
	    */
	    if (rowcount == 0)
	    {
		if ((IISQ_SQLCA_MACRO(IIlbqcb)) && (sql->sqlcode >=0))
		    IIsqSetSqlCode( IIlbqcb, IISQ_NOTFOUND );
	    }
	}
    }
    if (IIlbqcb->ii_lq_qrystat & GCA_REMNULS_MASK)
    {
	IIsqWarn( thr_cb, 2 );
    }
    if (IIlbqcb->ii_lq_qrystat & GCA_ALLUPD_MASK)
    {
	IIsqWarn( thr_cb, 4 );
    }
    if (IIlbqcb->ii_lq_qrystat & GCA_INVSTMT_MASK)
    {
	IIsqWarn( thr_cb, 5 );
    }

    /* the rest of the module deals with SQLCA. */
    if (!IISQ_SQLCA_MACRO(IIlbqcb))
	return;
    /*
    ** If we were in a transaction at the start of the statement (sqlwarn6 = T)
    ** and we read the "transaction over" flag and some error occurred, then
    ** this error terminated the transaction.  This is marked with sqlwarn6.
    */
    if (sql->sqlwarn.sqlwarn6 == 'T')
    {
	if ((IIlbqcb->ii_lq_qrystat & GCA_LOG_TERM_MASK) && (sql->sqlcode < 0))
	    IIsqWarn( thr_cb, 6 );
	else
	    sql->sqlwarn.sqlwarn6 = ' ';
    }
    /* If entered GCA then assign gateway data */
    if (IIlbqcb->ii_lq_gca)
    {
    /*  sql->sqlerrd[0] = IIlbqcb->ii_lq_error.ii_er_other */
	sql->sqlerrd[1] = IIlbqcb->ii_lq_gca->cgc_resp.gca_errd1;
    /*  sql->sqlerrd[2] = assigned above */
	sql->sqlerrd[3] = IIlbqcb->ii_lq_gca->cgc_resp.gca_cost;
	sql->sqlerrd[4] = IIlbqcb->ii_lq_gca->cgc_resp.gca_errd4;
	sql->sqlerrd[5] = IIlbqcb->ii_lq_gca->cgc_resp.gca_errd5;
    }
    IIsqEvSetCode( IIlbqcb );		/* Set sqlcode for presence of events */
}


/*{
+*  Name: IIsqWarn() - Put warning status information into SQLCA
**
**  Description:
**	IIsqWarn() sets a warning indicator in the embedded program's
**	SQLCA.  There are 7 "SQLWARN" fields in the SQLCA with the
**	following meanings to the embedded program:
**	SQLWARN0	When this is set, then at least one other 
**			warning indicator is set.
**	SQLWARN1	Set on truncation of character string assignments
**			into host variables. 
**			(SS01004_STRING_RIGHT_TRUNC)
**	SQLWARN2	Set on elimination of null values from aggregates.
**			(SS01003_NULL_ELIM_IN_SETFUNC)
**	SQLWARN3	Set on mismatching number of result columns and
**		 	result variables. 
**			(SS21000_CARD_VIOLATION)
**	SQLWARN4	Set when PREPAREing an UPDATE or DELETE statement
**			without a WHERE clause.
**			(SS01501_NO_WHERE_CLAUSE)
**			Actually, it's SS50000_MISC_ERRORS for now.
**	SQLWARN5	Gateway incompatible SQL statement was used.
**			(SS50005_GW_ERROR)
**	SQLWARN6	Transaction was terminated either by user or DBMS.
**			(SS40000_XACT_ROLLBACK)
**	SQLWARN7	Unused
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	warning		- The number of the "SQLWARN" field to set.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	The user's SQLCA (pointed at by the field IIlbqcb->ii_lq_sqlca)
**	will be updated with warning information.
**	
**  History:
**	15-apr-1987 - written (bjb)
**	31-oct-1992 (larrym)
**		Added SQLSTATEs support.
**	21-Apr-1994 (larrym)
**	    Fixed bug 59216.  IIsqWarn can get called now even if the user
**	    hasn't declared an SQLCA, so we need to deal with that now. 
**	    We still want to set SQLSTATE.
*/

VOID
IIsqWarn( II_THR_CB *thr_cb, i4  warning)
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IISQLCA 	*sql = IIlbqcb->ii_lq_sqlca;

    if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn0 = 'W';

    switch (warning)
    {
      case 1:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn1 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			 SS01004_STRING_RIGHT_TRUNC );
	break;
      case 2:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn2 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			 SS01003_NULL_ELIM_IN_SETFUNC );
	break;
      case 3:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn3 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			 SS21000_CARD_VIOLATION );
	break;
      case 4:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn4 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			 SS01501_NO_WHERE_CLAUSE );
	break;
      case 5:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn5 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,  SS50005_GW_ERROR );
	break;
      case 6:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn6 = 'W';
	IILQdaSIsetItem(thr_cb, IIDA_RETURNED_SQLSTATE, SS40000_XACT_ROLLBACK);
	break;
      case 7:
	if (IISQ_SQLCA_MACRO(IIlbqcb)) sql->sqlwarn.sqlwarn7 = 'W';
	IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, SS50000_MISC_ERRORS );
	break;
    }
}


/*{
+*  Name: IIsqSaveErr - Copy SQL error/message code and text into SQLCA
**
**  Description:
**	This routine is called from LIBQ's error-saving routine, IIer_save().
**	It gets called whenever an error or database procedure message has
**	occurred in the execution of an embedded SQL statement.  This
**	routine assumes that the caller has already verified that the
**	query language is SQL and that the program has declared an SQLCA.
**
**	If an error has NOT already occured in the current statement (the 
**	.sqlcode of the SQLCA is non-negative), IIsqSaveErr copies the
**	negative value of the generic error number and up to IISQ_ERRMLEN
**	bytes of the error text into the embedded program's SQLCA.  The
**	DBMS-specific (or LIBQ) error number gets put into .sqlerrd[0].
**
**	Under normal circumstances, only the first error is saved.  If we
**	are in the scope of a database procedure execution (II_L_DBPROC),
**	then each error is saved (and overrides the previous one), as the
**	calling application gains control for each event (errors or messages).
**	SELECT loops also save each error.
**
**	This routine also gets called when a message is issued from within 
**	a database procedure and in this case the sqlcode is set to
**	IISQ_MESSAGE (and as much of the text is copied).
**
**  Inputs:
**	IIlbqcb			Current session control block.
**	iserr			TRUE/FALSE - Error or procedure message.
**	genericnum		Generic error number (this gets put into the
**				sqlcode for applications).  In case of 
**				procedure messages, this number is ignored. 
**	errnum			The number of the DBMS-specific (or LIBQ) error
**				or the procedures message number.  This gets
**				put into sqlerrd[0] - even if procedure
**				message number.
**	errtxt			Pointer to text describing the error/message.
**				May be null if this is a message.
**
**  Outputs:
**	SQLCA (pointed at by IIlbqcb->ii_lq_sqlca
**	    .sqlcode		If 'iserr' then this field will be assigned
**				the negative of the current 'genericnum'.
**				Note that if this field is already negative 
**				and we are not in a procedure (ie, an error
**				already occurred in this statement), this 
**				field will remain unchanged.  If we are in
**				a database procedure then this field gets
**				rewritten.
**				If 'not iserr', then this field is assigned
**				IISQ_MESSAGE.
**	    .sqlerrd[0]		Always gets assigned the value of 'errnum'.
**	    .sqlerrml		Will be assigned up to 70 bytes of the
**				current error message if .sqlcode is assigned
**				a value.  The text will not be null terminated.
**	    .sqlerrmc		Will contain the number of bytes copied into
**				.sqlerrml.
**	
**	Returns:
**	    TRUE		The error (code and text) has been recorded
**				in the embedded program's SQLCA.
**	    FALSE		The error has not been recorded.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	13-apr-1987 - written (bjb)
**	11-may-1988 - modified to allow multiple error recordings and messages
**		      if during procedure execution (ncg)
**	12-dec-1988 - Now handles generic error numbers - from gateway. (ncg)
**	05-oct-1989 - Bug #8166(DG) - Must pad sqlerrmc with blanks. (bjb)
**	04-apr-1990 (barbara)
**	    Fixed bug 20882.  genericnum may already be negative if, for
**	    instance, program has errortype set to dbmserror and error
**	    is user error raised in a database procedure.  Make sure it
**	    is entered in sqlcode as negative.
**	12-Oct-1990 (kathryn)
**	    When saving error message text into SQLCA, Remove "\n" characters.
**	    Since the SQLCA error buffer is only (IISQ_ERRMLEN) 70 characters,
**	    the entire saved error message should be output on 1 line.
**	15-oct-1990 (barbara)
**	    Even if existing error, save this error if we are in SELECT loop.
**	04-feb-1991 (kathryn) Bug 35588
**	    No need to pad sqlerrmc with blanks here - We now blank this out
**	    in IIsqInit(), so there should never be an existing message in 
**	    this field.
**	01-apr-1991 (kathryn)
**	    Save error if user defined error handler is set.  Back out
**	    above fix.  sqlerrmc does need to be padded with blanks.
**	03-nov-1992 (larrym)
**	    Added SQLCODE support.  Now calls IIsqSetSqlCode instead of
**	    setting sqlca->sqlcode directly.
*/

i4
IIsqSaveErr
(
    II_LBQ_CB	*IIlbqcb,
    bool	iserr,
    i4	genericnum,
    i4	errnum,
    char	*errtext
)
{
    u_i4	errlen;
    i4		copylen;
    char	*sqerrmc;
    char	*cp;

    /* Check if already an error, and not inside procedure or retrieve loop */
    if (   iserr
	&& IIlbqcb->ii_lq_sqlca->sqlcode < 0
	&& (IIlbqcb->ii_lq_flags & II_L_DBPROC) == 0
	&& (IIlbqcb->ii_lq_flags & II_L_RETRIEVE) == 0
	&& (!IIglbcb->ii_gl_hdlrs.ii_err_hdl))
    {
	return FALSE;
    }


    /* Bug 20883 - see comments above */
    if (genericnum < 0)
	genericnum = -genericnum;
    /* 
    ** If a program error, return TRUE?  i.e. we want IIsave_err to
    ** save if this is outside the ESQL error range.  On the other
    ** hand, will error numbers outside the "PROGRAM" range ever crop
    ** up on an ESQL statement??
    */
    if (iserr)
	/* somebody else should have done an IILQdaSIsetItem already */
        IIsqSetSqlCode( IIlbqcb, -genericnum );
    else
	/* somebody else should have done an IILQdaSIsetItem already */
	IIsqSetSqlCode( IIlbqcb, IISQ_MESSAGE );

    IIlbqcb->ii_lq_sqlca->sqlerrd[0] = errnum;
    if (errtext)		/* May be none for messages */
    {
	copylen = min( STlength(errtext), IISQ_ERRMLEN );
	sqerrmc = IIlbqcb->ii_lq_sqlca->sqlerrm.sqlerrmc;
        if (copylen < IISQ_ERRMLEN)
	    STmove( ERx(" "), ' ', IISQ_ERRMLEN - copylen, sqerrmc + copylen );
	errlen = CMcopy( errtext, copylen, sqerrmc );
	while ((cp = STindex(sqerrmc,ERx("\n"),IISQ_ERRMLEN)) != NULL)
		*cp = ' ';
	IIlbqcb->ii_lq_sqlca->sqlerrm.sqlerrml = errlen;
    }
    return TRUE;
}


/*{
+*  Name: IIsqPrint - Print error/message text of last statement.
**
**  Description:
**	IIsqPrint is called if an error or message has occurred during 
**	exection of an ESQL statement and the WHENEVER ... CALL SQLPRINT error 
**	handling is set.  The error or message text will have already been
**	saved into LIBQ's ii_lq_error structure (see iierror.c).
**
**  Inputs:
**	sqlca			Points to the SQLCA from the ESQL program
**	    .sqlcode		< 0   if we have been called "ON SQLERROR"
**				= 700 if we have been called "ON SQLMESSAGE"
**
**  Outputs:
**	Nothing
**	Returns:
**	    Void
**	Errors:
**	    None.
**
**  Preprocessor generated code:
**   1. EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;
**	EXEC SQL DROP t
**
**	IIsqInit(&sqlca);
**	IIwritio(0,(short *)0,1,32,0,"drop t");
**	IIsyncup((char *)0,0);
**	if (sqlca.sqlcode < 0) IIsqPrint(&sqlca);
**
**   2. EXEC SQL WHENEVER SQLMESSAGE CALL SQLPRINT;
**   	EXEC SQL WHENEVER SQLERROR CALL xyz;
**	EXEC SQL EXECUTE PROCEDURE p;
**	
**	IIsqInit(&sqlca);
**	IILQpriProcInit(IIPROC_EXEC, "p");
**	while (IILQprsProcStatus() != 0) {
**	    if (sqlca.sqlcode < 0)
**		xyz();
**	    else if (sqlca.sqlcode == 700)
**		IIsqPrint(&sqlca);
**	}
**
**  Side Effects:
**	Contents of IIlbqcb.ii_lq_error.ii_er_smsg printed if error.
**	
**  History:
**	01-may-1987 - written (bjb)
**	11-may-1988 - modified to also print user messages from database
**		      procedures (ncg)
**	12-dec-1988 - uses IImsgutil to display errors - from gateway. (ncg)
**	07-dec-1989 - Print out events in a loop. (barbara)
**	19-apr-1991 - Events will now also be consumed. (teresal) 
*/

void
IIsqPrint(sqlca)
IISQLCA	*sqlca;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    II_ERR_DESC	*lbqerr = &IIlbqcb->ii_lq_error;

    /* Print only if an error occurred */
    if (sqlca->sqlcode < 0 && lbqerr->ii_er_smsg)
    {
	IImsgutil(lbqerr->ii_er_smsg);
    }
    else if (sqlca->sqlcode == IISQ_MESSAGE)	/* User message */
    {
	IIdbmsg_disp( IIlbqcb );
    }
    else if (sqlca->sqlcode == IISQ_EVENT)	/* Event */
    {
	/* Special handling to print all events on the queue */
	while (IIlbqcb->ii_lq_event.ii_ev_list != (II_EVENT *)0)
	{
	    /* Make sure event is consumed */
	    IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_CONSUMED;
	    IILQedEvDisplay( IIlbqcb, IIlbqcb->ii_lq_event.ii_ev_list );
	    IILQefEvFree( IIlbqcb );
	}
    }
}

/*{
+*  Name: IIsqEvSetCode - Set sqlcode for event processing.
**
**  Description:
**	This routine is called to set sqlcode to IISQ_EVENT if there
**	is (unconsumed) event information on LIBQ's II_EVENT list.  The sqlcode
**	setting informs the application that there is event information
**	waiting to be read.  It also triggers the WHENEVER SQLEVENT
**	processing.
**
**	For example, the statement:
**		WHENEVER SQLEVENT CALL ev_proc;
**
**	generates after each statement (except GET EVENT)
**
**		[else] if (sqlca.sqlcode == 710)
**		    ev_proc();
**
**	This routine is called at the end of processing database statements
**	(from IIsqEnd).  In a couple of places it is called directly:
**
**	1)  after processing a GET EVENT statement -- because GET EVENT
**	    does not go through IIqry_end/ IIsqEnd; and
**
**	2)  after an event has been added to LIBQ's II_EVENT list -- to
**	    ensure correct WHENEVER SQLEVENT processing in dbproc execution
**	    loops.
**
** Inputs:
**	IIlbqcb		Current session control block.
** Outputs:
**	None.
**
** History:
**	15-dec-1989 (barbara)
**	    Written for Phoenix/Alerters.
**	16-apr-1991 (teresal)
**	    Check if first event has already been consumed, if so look
**	    if any more events on the list to determine if sqlcode needs to
**	    be set.
**	03-nov-1992 (larrym)
**	    Added SQLCODE support.   Now calls IIsqSetSqlCode instead of
**	    setting sqlca->sqlcode directly.
*/

VOID
IIsqEvSetCode( II_LBQ_CB *IIlbqcb )
{
    if (   IISQ_SQLCA_MACRO(IIlbqcb) 
	&& (IIlbqcb->ii_lq_sqlca->sqlcode == 0 ||
	    IIlbqcb->ii_lq_sqlca->sqlcode == IISQ_MESSAGE)
	&& IIlbqcb->ii_lq_event.ii_ev_list != (II_EVENT *)0)
    {
	if (!(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
	    /* There is no SQLSTATE for events. */
	    IIsqSetSqlCode( IIlbqcb, IISQ_EVENT );
	else if (IIlbqcb->ii_lq_event.ii_ev_list->iie_next != (II_EVENT *)0)
	    /* There is no SQLSTATE for events. */
	    IIsqSetSqlCode( IIlbqcb, IISQ_EVENT );
    }
}
