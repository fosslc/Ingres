/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<eqrun.h>
#include	<abqual.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cgerror.h"

/**
** Name:	cgdb.c -	Code Generator Database Statements Module.
**
** Description:
**	Generates C code from the IL statements for a database-oriented
**	statements.  Includes a different 'IICGstmtGen()' routine for each
**	type of statement.
**
**	IICGdbsDbGen()		output code for non-repeatable DBMS statement.
**	IICGsdbSqldbGen()	output code for non-repeatable SQL statement.
**	IICGdbrDbRepeatGen()	output code for repeatable DBMS statement.
**	IICGsdrSqldbRepeatGen()	output code for repeatable SQL DBMS statement.
**	IICGdbcDbconstGen()
**	IICGdblDbvalGen()
**	IICGdbtDbvstrGen()
**	IICGdbvDbvarGen()
**	IICGdbpDbpred()
**	IICGgegGetEventGen()	generate code for a get event
**
** History:
**	Revision 6.0  87/06  arthur
**	Initial revision.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**	04/22/91 (emerson)
**		Modifications for alerters (in DbstmtGen).
**	05/03/91 (emerson)
**		Modifications for alerters: Created function
**		IICGgegGetEventGen to handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code).
**
**	Revision 6.5
**
**	29-sep-92 (davel)
**		Added routines for multi-session support.
**	05-nov-92 (davel)
**		Added support for DISCONNECT ALL.
**	10-nov-92 (davel)
**		Added IICGchsCheckSession() for new IL_CHKSESSION (to check
**		new session switched to in SET_SQL statements). Also modified
**		IICGconDBConnect() and IICGdscDBDisconnect() to pass session
**		ID as {nat *} instead of {nat} so that we can distinguish 
**		between no session ID specification by user vs. an invalid
**		specification.
**	09-mar-93 (davel)
**		Add support for connection name for CONNECT and DISCONNECT
**		statements.
**	29-mar-93 (davel)
**		Fix bug 50793 in IICGchsCheckSession().
**	19-nov-93 (robf)
**              Add support  for dbms passwords
**	2-july-1996 (angusm)
**		Modify DbstmtGen() to ignore embedded IL_GETFORM tokens in stream
**		if they follow immediately after IL_CRTTABLE. (bug 77352).
**	3-jul-1996 (angusm)
**		Extend previous fix for 'declare global temp table'
**		(bug 75153)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-sep-2000 (hanch04)
**	    code generated should use type i4 and not nat.
**	25-sep-2000 (hanch04)
**	    missed more nats, code generated should use type i4 and not nat.
**      03-feb-2005 (srisu02)
**          Moved the declaration of IICGdbtDbvstrGen() to a global level
**          due to compilation errors obtained on AIX
**/

GLOBALREF i4	iiCGqryID;

/*
** Name:	DbstmtGen() -	Output C Code for DBMS Statements.
**
** Description:
**	Generates C code to output database statements to the DBMS from the
**	IL code for the statement.
**
** IL statements:
**	DB-op (e.g., APPEND, UPDATE, GRANT, RANGE) <optional repeat-flag>
**      [IL_QID strvalQueryName intvalQueryId1 intvalQueryId2]
**	[ DBCONST VALUE
**	  DBVAL VALUE
**	  DBVSTR VALUE
**	  DBVAR VALUE
**	  QRYPRED INT INT
**	    TL2ELM VALUE VALUE
**	    ...
**	    ENDLIST
**	   ... ]
**	ENDLIST
**
**	All DBMS statements are represented by an initial IL statement (an
**	operator with no operands for non-repeatble DBMS statements or a single
**	operand that indicates whether it is a repeat query for DBMS statements
**	that can be repeated) followed by a (possibly empty) series of DBCONST,
**	DBVAL, DBVAR or QRYPRED IL statements.	The OSL translator has put all
**	constant parts of the statement to be passed to the DBMS into DBCONST
**	statements; each DB_DATA_VALUE used as a string into a DBVAL statement,
**	each DB_DATA_VALUE used as a string variable into a DBVSTR statement,
**	and each variable into a DBVAR statement.
**
**	If the IL opcode is IL_DBSTMT, the actual name of the DB statement
**	to be generated is in the immediately following DBCONST statement.
**
**	For each qualification function in the DB statement's where clause,
**	there will be one QRYPRED statement, followed by as many TL2ELM
**	statements as there are elements within the qualification function.
**	The QRYPRED statement's first operand is the type of the qual
**	function (as defined in "abqual.h"); the second operand is the
**	number of elements within the qual function (this operand is
**	used by the OSL interpreter, but can be ignored by the code generator).
**	In each TL2ELM statement, the first operand is the name of a field.
**	The second operand is the left-hand side (database expression)
**	of the qualification function element (e.g., "relname.colname").
**	The list is terminated by an ENDLIST statement.
**
**	This routine executes each DBCONST, DBVAL, DBVSTR, DBVAR or QRYPRED
**	statement in turn until it finds an ENDLIST statement.
**
**	For repeat queries, all DBVAR operators are preceded with code that
**	outputs an argument indicator for the variable DB_DATA_VALUE.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**	repeat	{bool}  Whether the DBMS statement is a repeat statement.
**
** History:
**    	13-feb-1990 (Joe)
**          Added IL_QID. JupBug #10015
**	09/90 (jhw) - Added IL_DBVSTR.
**	01/29/91 (emerson)
**		Correct typo (bug 35650) in fix to remove 32K limit on IL.
**	04/22/91 (emerson)
**		Modifications for alerters: support new IL_DBSTMT.
**	2-jul-1996(angusm)
**		OSL rule for OI version of CREATE TABLE can generate
**		IL_GETFORM tokens if a sub-select is present. Silently
**		ignore them. (bug 77352).
**	22-nov-1996(rodjo04)
**		Update for bug 77352. CREATE TABLE can generate multiple
**		IL_GETFORM tokens if a sub-select is present. Silently
**		ingore them.
**  29-jan-1997(rodjo04) 
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_DOT statements. We must process this statement. Also completely
**      ignore IL_GETFORM token.
**  07-mar-1997(rodjo04) 
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_ARRAYREF statements. We must process this statement.
**  22-may-1997(rodjo04)
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_GETFORM statements. We must process this statement to pick
**      up form variables.
**  28-may-1998 (rodjo04)
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate 
**      IL_GETROW statements.   We must process this statement to pick
**      up form variables, but we must do this first. 
*/
	VOID	IICGdbtDbvstrGen();
static VOID
DbstmtGen (stmt, repeat)
register IL	*stmt;
bool		repeat;
{
	register IL	*next;
	register i4	argn = 0;
	IL		il_op;
	IL		prev;
        IL              *temp_op;
	char		buf[CGSHORTBUF];

	VOID	IICGdbcDbconstGen();
	VOID	IICGdblDbvalGen();
	VOID	IICGdbvDbvarGen();
	IL	*IICGdbpDbpred();

	il_op = *stmt & ILM_MASK;
        temp_op = stmt;

    next = IICGgilGetIl(stmt);
    il_op = *next & ILM_MASK;

    for (;;) 
	{
       if  (il_op == IL_GETROW)
       {
            IICGgrwGetrowGen(next);
            next = IICGgilGetIl(next);
            il_op = *next & ILM_MASK;
            if (il_op == IL_TL2ELM)
            {
                next = IICGgilGetIl(next);
                il_op = *next & ILM_MASK;
                if  (il_op == IL_ENDLIST)
                {
                   /* don't exit */
                }
                else
                {
                    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
                    break;
                }
            }
            else 
            {
                IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
                break;
            }
       }
       else if (il_op == IL_ENDLIST)
       {
          break;
	   }
	   next = IICGgilGetIl(next);
       il_op = *next & ILM_MASK;
	}	   

    stmt = temp_op;
    il_op = *stmt & ILM_MASK;
	
	/* Output DBMS statement name plus a trailing space */

	if ( il_op != IL_DBSTMT )
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIwritio"));
		IICGocaCallArg( _Zero );
		IICGocaCallArg( _ShrtNull );  /* null indicator */
		IICGocaCallArg( _One );
		_VOID_ CVna(DB_CHR_TYPE, buf);	/* string type */
		IICGocaCallArg(buf);
		IICGocaCallArg( _Zero );
		IICGocaCallArg( STprintf(buf, ERx("\"%s \""),
					IIILtab[il_op].il_name) );
		IICGoceCallEnd();
		IICGoseStmtEnd();
	}

	/* Output body of DBMS statement */
	prev = il_op;
	next = IICGgilGetIl(stmt);
	for (;;)
	{
		il_op = *next & ILM_MASK;
		if (il_op == IL_ENDLIST)
		{
			break;
		}
		else if (il_op == IL_DBCONST)
		{
			IICGdbcDbconstGen(next);
		}
		else if (il_op == IL_DBVAL)
		{
			IICGdblDbvalGen(next);
		}
		else if (il_op == IL_DBVSTR )
		{
			IICGdbtDbvstrGen(next);
		}
		else if (il_op == IL_DBVAR)
		{
			if (repeat)
			{ /* output argument indicator for repeat query */
				IICGosbStmtBeg();
				IICGocbCallBeg(ERx("IIwritio"));
				IICGocaCallArg( _Zero );
				IICGocaCallArg( _ShrtNull );  /* null indicator */
				IICGocaCallArg( _One );
				_VOID_ CVna(DB_CHR_TYPE, buf);
				IICGocaCallArg(buf);
				IICGocaCallArg( _Zero );
				IICGocaCallArg(STprintf(buf, ERx("\" $%d=\""), argn++));
				IICGoceCallEnd();
				IICGoseStmtEnd();
			}
			IICGdbvDbvarGen(next);
		}
		else if (il_op == IL_QRYPRED)
		{
			/*
			** The QRYPRED statement is followed by a list of TL2ELM
			** statements, terminated by an ENDLIST statement.
			*/
			next = IICGdbpDbpred(next);
		}
		else if (il_op == IL_QID)
		{
		/* Simply Ignore this */
		}
		else if (il_op == IL_GETFORM) 
		{
            IICGgtfGetformGen(next);
        }
		else if  (il_op == IL_DOT) 
		{
			IICGdotDotGen(next);
		}
		else if  (il_op == IL_ARRAYREF)
		{
			IICGarrArrayGen(next);
		}
                else if  (il_op == IL_GETROW)
		{
            next = IICGgilGetIl(next);
            il_op = *next & ILM_MASK;
            if (il_op == IL_TL2ELM)
            {
                next = IICGgilGetIl(next);
                il_op = *next & ILM_MASK;
                if  (il_op == IL_ENDLIST)
                {
				  /* don't exit */
                }
                else
                {
                    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
                    break;
                }
            } 
            else
            {
                IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
                break;
            }
        }
		else
		{
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
			break;
		}
		prev = il_op;
		next = IICGgilGetIl(next);
	}
	return;
}

/*
** Name:	sqInitGen() -	Output C Code for Call to 'IIsqInit()'.
**
** Description:
**	Generates C code for a call on 'IIsqInit()', which must precede
**	every DBMS statement that uses SQL syntax or semantics.
**
** History:
**	08/88 (jhw) - Written.
*/
static VOID
sqInitGen ()
{ /* IIsqInit((char *) NULL); */
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIsqInit"));
	IICGocaCallArg( _NullPtr );
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*
** Name:	syncupGen() -	Output C Code for Call to 'IIsyncup()'.
**
** Description:
**	Generates C code for a call on 'IIsyncup()', which synchronizes the
**	application with the DBMS.
**
** History:
**	08/88 (jhw) - Written.
*/
static VOID
syncupGen ()
{ /* IIsyncup((char *) NULL, 0); */
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIsyncup"));
	IICGocaCallArg( _NullPtr );
	IICGocaCallArg( _Zero );
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:	IICGdbsDbstmtGen() - Output C Code for Non-repeat DBMS Statement.
**
** Description:
**	Generates C code to output a non-repeatable database statement to the
**	DBMS from the IL code for the statement.  (See 'IICGdbrDbRepeatGen()'
**	for repeatable DBMS statements.)
**
** IL statements:
**	DB-op (e.g., BEGTRANS, RANGE, DEFINE)
**
**		See header comments in 'DbstmtGen()' for further information
**		on the handling of OSL database statements.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
*/
VOID
IICGdbsDbstmtGen (stmt)
IL	*stmt;
{
	DbstmtGen(stmt, FALSE /* not a repeatable query */);

	/* Output code to synchronize with the DBMS */
	syncupGen();

	return;
}

/*{
** Name:	IICGsdbSqldbGen() -	Output C Code for Non-repeat SQL DBMS Statement.
**
** Description:
**	Generates C code to output a non-repeatable SQL database statement
**	to the DBMS from the IL code for the statement.  (See
**	'IICGsdrSqldbRepeatGen()' for repeatable SQL database statements.)
**
** IL statements:
**	SQLDB-op (e.g., GRANT, ROLLBACK, COMMIT)
**
**		See header comments in 'IICGdbsDbstmtGen()' and 'DbstmtGen()'
**		for further information on the handling of OSL database
**		statements.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
*/
VOID
IICGsdbSqldbGen (stmt)
IL	*stmt;
{
	/*
	** Generate the call to 'IIsqInit()' that must
	** precede any SQL statement sent to the DBMS.
	*/
	sqInitGen();
	/*
	** Now generate the calls for the non-repeatable DBMS statement itself.
	*/
	IICGdbsDbstmtGen(stmt);
	return;
}

/*{
** Name:	IICGdbrDbRepeatGen() - Output C Code for Repeatable DBMS Statement.
**
** Description:
**	Generates C code to output a repeatable DML statement to the DBMS from
**	the IL code for the statement.  If the DML statement is not repeated,
**	it is output as non-repeatable database statement.  Otherwise, code
**	is output to execute and define a repeatable DML query.
**
** IL statements:
**	DB-op (e.g., APPEND, INSERT) repeat-flag
**      [IL_QID strvalQueryName intvalQueryId1 intvalQueryId2]
**
**		See header comments in 'DbstmtGen()' for further information
**		on the handling of OSL database statements.
**
**      IL_QID was added later than the repeat statements.  This means
**      that it is possible to have a statement with the flag set,
**      but without an IL_QID.  In this case it is a repeated query
**      that has not been recompiled since IL_QID was added.  Since
**      it is not possible to generate a unique query id at runtime, these
**      old statements are not run repeated.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	12/89 (jhw) -- Modified query ID to be based on query ID.
**    	13-feb-1990 (Joe)
**	    Added IL_QID and code to use it for query id. JupBug #10015.
**	07/90 (jhw) -- Generate 'IIsqInit()' before 'IIexec()' of repeat query.
**		Bug #30748.
**      14-apr-2003 (rodjo04) bug 109908
**            Because of change to bug 106438, we should now solve for IL_DBVSTR in
**            IICGdbrDbRepeatGen();
*/

static VOID
sexecGen ()
{ /* IIsexec(); */
	IICGosbStmtBeg();
	IICGocbCallBeg( ERx("IIsexec") );
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

static bool	sqlp = FALSE;

VOID
IICGdbrDbRepeatGen (stmt)
register IL	*stmt;
{
	VOID	IICGdbvDbvarGen();

	register IL	*next;
	IL              il_op;

	next = IICGgilGetIl(stmt);
        /*
        ** This checks the value of the repeat flag.  If it is FALSE,
        ** or if the next statement is not an IL_QID then run as
        ** non-repeat.  The test for IL_QID makes old repeat queries
        ** run non-repeated to avoid a bug with a non-unique query id.
        */
        if (!ILgetOperand(stmt, 1) || (*next&ILM_MASK) != IL_QID)
	{ /* not a repeat query */
		DbstmtGen(stmt, FALSE);

		syncupGen();
	}
	else
	{ /* a repeat query */
                IL	qid_nm;
                IL	qid1;
                IL	qid2;

                /*
                ** If we got here then next points to the IL_QID.  Use
                ** the values in it for the query id.  We get the IL operand`
                ** values and will use IICGgvlGetVal to get the values.
                */
                qid_nm = ILgetOperand(next, 1);
                qid1 = ILgetOperand(next, 2);
                qid2 = ILgetOperand(next, 3);

/*
**		IIsexec();
**		while (IInexec() == 0) {
**			if ( sqlp )
**				IIsqInit((char *)NULL);
**			IIexExec(1,"query name",qid1,qid2);
**			'IIputdomio()'s for variables;
**			IIsyncup((char *)0,0);
**			if (IInexec() == 0) {
**				if ( sqlp )
**					IIsqInit((char *)NULL);
**				IIexDefine(1,"query name",qid1,qid2);
**			**	query body (generate through 'Dbstmtgen()') **
**				IIexDefine(0,"query name",qid1,qid2);
**				if (IIerrtest() == 0) {
**					IIsexec();
**				}
**			}
**		}
*/
		sexecGen();

		/* While loop */
		IICGowbWhileBeg();
		IICGocbCallBeg(ERx("IInexec"));
		IICGoceCallEnd();
		IICGoicIfCond( (char *)NULL, CGRO_EQ, _Zero );
		IICGowkWhileBlock();

			if ( sqlp )
				sqInitGen();

			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIexExec"));
			IICGocaCallArg( _One );
                        IICGocaCallArg(IICGgvlGetVal(qid_nm, DB_CHR_TYPE));
                        IICGocaCallArg(IICGgvlGetVal(qid1, DB_INT_TYPE));
                        IICGocaCallArg(IICGgvlGetVal(qid2, DB_INT_TYPE));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			/* Output code to send down variables */

			next = IICGgilGetIl(next);
			for (;;)
			{
				il_op = *next & ILM_MASK;
				if (il_op == IL_ENDLIST)
				{
					break;
				}
				else if (il_op == IL_DBVAR)
				{
					IICGdbvDbvarGen(next);
				}
                                else if (il_op == IL_DBVSTR)
				{
					IICGdbtDbvstrGen(next);
				}
				else if (il_op == IL_QRYPRED)
				{
					/*
					** Qualification predicates cannot be supported for repeat
					** queries and should never have been generated by the
					** translators.
					*/
					IICGerrError(CG_ILMISSING, ERx("not QRYPRED"), (char*)NULL);
				}
				else if (il_op != IL_DBCONST && il_op != IL_DBVAL)
				{
					IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
					break;
				}
				next = IICGgilGetIl(next);
			}
	
			syncupGen();
	
			IICGoibIfBeg();
			IICGocbCallBeg(ERx("IInexec"));
			IICGoceCallEnd();
			IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
			IICGoikIfBlock();
	
				if ( sqlp )
				{
					sqInitGen();
					sqlp = FALSE;
				}

				IICGosbStmtBeg();
				IICGocbCallBeg(ERx("IIexDefine"));
				IICGocaCallArg( _One );
                                IICGocaCallArg(IICGgvlGetVal(qid_nm,
                                                             DB_CHR_TYPE));
                                IICGocaCallArg(IICGgvlGetVal(qid1,
                                                             DB_INT_TYPE));
                                IICGocaCallArg(IICGgvlGetVal(qid2,
                                                             DB_INT_TYPE));
				IICGoceCallEnd();
				IICGoseStmtEnd();
	
				DbstmtGen(stmt, TRUE);
	
				IICGosbStmtBeg();
				IICGocbCallBeg(ERx("IIexDefine"));
				IICGocaCallArg( _Zero );
                                IICGocaCallArg(IICGgvlGetVal(qid_nm,
                                                             DB_CHR_TYPE));
                                IICGocaCallArg(IICGgvlGetVal(qid1,
                                                             DB_INT_TYPE));
                                IICGocaCallArg(IICGgvlGetVal(qid2,
                                                             DB_INT_TYPE));

				IICGoceCallEnd();
				IICGoseStmtEnd();
	
				IICGoibIfBeg();
				IICGocbCallBeg(ERx("IIerrtest"));
				IICGoceCallEnd();
				IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
				IICGoikIfBlock();
					sexecGen();
				IICGoixIfEnd();

			IICGoixIfEnd();

		IICGowxWhileEnd();		/* end while block */
	}
	return;
}

/*{
** Name:	IICGsdrSqldbRepeatGen() - Output C Code for a Repeat SQL DBMS Statement.
**
** Description:
**	Generates C code to output a repeatable SQL database statement to the
**	DBMS from the IL code for the statement.  (See 'IICGsdsSqldbGen()' for
**	non-repeatable SQL database statements.)
**
** IL statements:
**	SQLDB-op (DELETE FROM, INSERT, UPDATE) repeat-flag
**
**		See header comments in 'IICGdbrDbRepeatGen()' and 'DbstmtGen()'
**		for further information on the handling of OSL database
**		statements.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
*/
VOID
IICGsdrSqldbRepeatGen (stmt)
IL	*stmt;
{
	/*
	** Generate the call to 'IIsqInit()' that must
	** precede any SQL statement sent to the DBMS.
	*/
	sqInitGen();
	/*
	** Now generate the calls for the repeatable DBMS statement itself.
	*/
	sqlp = TRUE;
	IICGdbrDbRepeatGen(stmt);
	return;
}

/*{
** Name:	IICGdbcDbconstGen() -	Pass a string constant to the DBMS.
**
** Description:
**	Generates C code to pass a string constant to the DBMS.
**
** IL statements:
**	DBCONST VALUE
**
**		See header comment in 'DbstmtGen()' for further information.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**  10-mar-2000 (rodjo04) bug 98775
**       Added support for Float data type.
**  31-jul-2000 (rodjo04) bug 98775
**       Fixed a typo/syntax problem of above fix.
**
*/
VOID
IICGdbcDbconstGen (stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIwritio"));
	IICGocaCallArg( _Zero );
	IICGocaCallArg( _ShrtNull );  /* null indicator */
	IICGocaCallArg( _One );
	_VOID_ CVna(DB_CHR_TYPE, buf);
	IICGocaCallArg(buf);
	IICGocaCallArg( _Zero );
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGdblDbvalGen() - Pass a variable name to the backend
**
** Description:
**	Passes a variable name to the backend.
**
** IL statements:
**	DBVAL VALUE
**
**		See header comment in 'DbstmtGen()' for further information.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	06/87 (agh) -- Written.
*/
VOID
IICGdblDbvalGen (stmt)
register IL	*stmt;
{
	char		buf[CGSHORTBUF];
	bool		int_type;
	DB_DT_ID	type;

	DB_DT_ID	IICGvltValType();

	IICGosbStmtBeg();
	type = IICGvltValType(ILgetOperand(stmt, 1));
    switch (abs(type))
	{
	 case DB_INT_TYPE:
	 case DB_FLT_TYPE:
		IICGocbCallBeg(ERx("IIvarstrio"));
		IICGocaCallArg( _ShrtNull );	/* null indicator */
		IICGocaCallArg( _One );		/* isvar == TRUE */
		_VOID_ CVna(DB_DBV_TYPE, buf);
		IICGocaCallArg(buf);
		IICGocaCallArg( _Zero );
		IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
		IICGoceCallEnd();
		IICGoseStmtEnd();
		break;
	 default:
		IICGocbCallBeg(ERx("IIwritio"));
		IICGocaCallArg( _One );		/* trim == TRUE */
		IICGocaCallArg( _ShrtNull );	/* null indicator */
		IICGocaCallArg( _One );		/* isvar == TRUE */
		_VOID_ CVna(DB_DBV_TYPE, buf);
		IICGocaCallArg(buf);
		IICGocaCallArg( _Zero );
		IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
		IICGoceCallEnd();
		IICGoseStmtEnd();
        break;
	}

	return;
}

/*{
** Name:	IICGdbtDbvstrGen() -	Pass a Variable String to the DBMS.
**
** Description:
**	Passes a variable string to the DBMS.
**
** IL statements:
**	DBVSTR VALUE
**
**		See header comment in 'DbstmtGen()' for further information.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	09/90 (jhw) -- Written.  Bugs #31075, #31342, #31614.
*/
VOID
IICGdbtDbvstrGen ( stmt )
register IL	*stmt;
{
	char		buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIvarstrio"));
	IICGocaCallArg( _ShrtNull );	/* null indicator */
	IICGocaCallArg( _One );		/* isvar == TRUE */
	_VOID_ CVna(DB_DBV_TYPE, buf);
	IICGocaCallArg(buf);
	IICGocaCallArg( _Zero );
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGdbvDbvarGen() - Pass a variable to the DBMS.
**
** Description:
**	Passes a variable to the DBMS.
**
** IL statements:
**	DBVAR VALUE
**
**		See header comment in 'DbstmtGen()' for further information.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	06/87 (agh) -- Written.
**	10/88 (jhw) -- Changed to call 'IILQdtDbvTrim()', which trims CHAR
**		and C DBVs before sending them to the DBMS.
*/
VOID
IICGdbvDbvarGen ( stmt )
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IILQdtDbvTrim"));
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:	IICGdbpDbpred() -	Qual function in a non-query DB stmt
**
** Description:
**	Generates code for a qualification function in a non-query database
**	statements (e.g., append, insert, delete).
**
** IL statements:
**	QRYPRED INT INT			} One QRYPRED/TL2ELM combination will
**	TL2ELM VALUE VALUE		} appear for each qualification function
**	  ...				} in the where clause.
**	ENDLIST
**
**		See header comment in IICGdbsDbstmtGen() for
**		further information.
**
**	For query statements (retrieve/select), QRYPRED statements are
**	processed in IICGqryBuildQryGen().
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** Returns:
**	{IL *}	A pointer to the last IL statement dealing with the
**		qualification function.
**
** History:
**	06/87 (agh) -- Written.
**	03/90 (jhw) -- Modified to call 'IIARqualify()' instead of
**			'IIARrpgRtsPredGen()'.  JupBug #9409.
*/
IL *
IICGdbpDbpred (stmt)
register IL	*stmt;
{
	register IL	*next;
	IL		*pred_stmt = stmt;

	IICGobbBlockBeg();
	IICGostStructBeg(ERx("static ABRT_QFLD"), ERx("IIqfld0"), 0, CG_ZEROINIT);
	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
		IICGerrError(CG_ILMISSING, ERx("tl2elm"), (char *) NULL);
	}
	else do
	{
		IICGoqfQualfldEle(next);
		next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);
	if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	IICGoqfQualfldEle((IL *) NULL);
	IICGosdStructEnd();
	IICGostStructBeg(ERx("static ABRT_QUAL"), ERx("IIqual0"), 0, CG_INIT);
	IICGoqlQualInit(pred_stmt, 0);
	IICGosdStructEnd();

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARqualify"));
	IICGocaCallArg(ERx("_PTR_ &IIqual0"));
	IICGocaCallArg(ERx("IIQG_send"));
	IICGocaCallArg(ERx("_PTR_ NULL"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGobeBlockEnd();

	return next;
}

/*{
** Name:	IICGgegGetEventGen - Generate Code for a Get Event
**
** Description:
**	Code is generated here for a GETEVENT statement.  The format is:
**
**		GETEVENT VALUE
**
**	where VALUE is a reference to an integer constant or dbv
**	containing the number of seconds to wait (-1 = forever).
**
** Inputs:
**	stmt		The GETEVENT IL statement.
**
** History:
**	05/03/91 (emerson)
**		Written.
*/
i4
IICGgegGetEventGen( stmt )
IL	*stmt;
{
	char		buf[ CGBUFSIZE ];
	char		*secs;

	secs = IICGgvlGetVal( ILgetOperand( stmt, 1 ), DB_INT_TYPE );
	buf[ 0 ] = EOS;
	(VOID) STcat( buf, ERx("(int)") );
	(VOID) STcat( buf, secs );
	IICGosbStmtBeg( );
	IICGocbCallBeg( ERx("IILQesEvStat") );
	IICGocaCallArg( _Zero );
	IICGocaCallArg( buf );
	IICGoceCallEnd( );
	IICGoseStmtEnd( );
	return 0;
}

/*{
** Name:	IICGconDBConnect() - Establish new DB connection
**
** Description:
**	Builds structures needed by IIARndcNewDBConnection(), which establishes
**	a new database connection.
**
** IL statements:
**	CONNECT VALUE
**	[ TL2ELM VALUE VALUE ]
**	  ...
**	ENDLIST
**
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	30-sep-92 (davel)
**		First written.
**	10-nov-92 (davel)
**		Modified so that sess_id is a {nat *} instead of a {nat},
**		so that we can distinguish between no session ID specification
**		and an invalid one.
**	09-mar-93 (davel)
**		Add support for connection name.
*/
VOID
IICGconDBConnect ( stmt )
register IL	*stmt;
{
    register IL	*next;
    i4		nopts = 0;		/* number of connect flags */
    i4		nwiths = 0;		/* number of with clauses */
    char	buf[CGSHORTBUF];

    IICGobbBlockBeg();
    IICGostStructBeg(ERx("char *"), ERx("dbname"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("username = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("conn_name = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("dbms_password = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("i4 *"), ERx("sess_id = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("opts"), 12, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("withkeys"), 12, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("withvals"), 12, CG_NOINIT);
    IICGostStructBeg(ERx("i4"), ERx("withtypes"), 12, CG_NOINIT);

    /* assign database name */
    IICGovaVarAssign(ERx("dbname"), 
			IICGgvlGetVal(ILgetOperand(stmt,1), DB_CHR_TYPE) );

    next = IICGgilGetIl(stmt);
    /*
    ** Read in the 2-valued list of connect options.
    */
    while ((*next&ILM_MASK) == IL_TL2ELM)
    {
	    char *flag;
	    IL  ref;
	    DB_DT_ID type;

	    ref = ILgetOperand( next, 1 );
	    (VOID) IIORcqConstQuery( &IICGirbIrblk, - ref, DB_CHR_TYPE, 
						(PTR *) &flag, (i4 *)NULL );
	    if (STcompare(flag, ERx("ii_user")) == 0)
	    {
		IICGovaVarAssign(ERx("username"), 
			IICGgvlGetVal(ILgetOperand(next,2), DB_CHR_TYPE) );
	    }
	    else if (STcompare(flag, ERx("ii_conn")) == 0)
	    {
		IICGovaVarAssign(ERx("conn_name"), 
			IICGgvlGetVal(ILgetOperand(next,2), DB_CHR_TYPE) );
	    }
	    else if (STcompare(flag, ERx("ii_dbms_password")) == 0)
	    {
		IICGovaVarAssign(ERx("dbms_password"), 
			IICGgvlGetVal(ILgetOperand(next,2), DB_CHR_TYPE) );
	    }
	    else if (STcompare(flag, ERx("ii_flag")) == 0)
	    {
	        IICGoaeArrEleAssign(ERx("opts"), (char *) NULL, nopts++,
			IICGgvlGetVal(ILgetOperand(next,2), DB_CHR_TYPE) );
	    }
	    else if (STcompare(flag, ERx("ii_sess")) == 0)
	    {
		    IICGosbStmtBeg();
		    STprintf(buf, ERx("sess_id = &%s"),
			IICGgvlGetVal(ILgetOperand(next,2), DB_INT_TYPE) );
		    IICGoprPrint(buf);
		    IICGoseStmtEnd();
	    }
	    else	/* a with clause */
	    {
		ref = ILgetOperand( next, 2);
	 	type = IICGvltValType(ref);
		if ( AFE_NULLABLE(type))
		{
			type = -type;
		} 
	        IICGoaeArrEleAssign(ERx("withkeys"), (char *) NULL, nwiths,
			IICGgvlGetVal(ILgetOperand(next,1), DB_CHR_TYPE) );
		STprintf( buf, ERx("%d"), type);
	        IICGoaeArrEleAssign(ERx("withtypes"), (char *) NULL, nwiths,
			buf);
		if (type == DB_CHR_TYPE)
		{
		    IICGoaeArrEleAssign(ERx("withvals"), (char *)NULL, nwiths++,
				IICGgvlGetVal(ref, DB_CHR_TYPE));
		}
		else if (type == DB_INT_TYPE)
		{
		    IICGosbStmtBeg();
		    STprintf(buf, ERx("withvals[%d] = (char *)&%s"), nwiths++, 
				IICGgvlGetVal(ref, DB_INT_TYPE));
		    IICGoprPrint(buf);
		    IICGoseStmtEnd();
		}
	    }

	    next = IICGgilGetIl(next);
    }
    if ((*next&ILM_MASK) != IL_ENDLIST)
        IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

    /* have ABFRT issue the connect */
    IICGosbStmtBeg();
    IICGocbCallBeg(ERx("IIARndcNewDBConnect"));
    IICGocaCallArg(ERx("dbname"));
    IICGocaCallArg(ERx("username"));
    IICGocaCallArg(ERx("sess_id"));
    IICGocaCallArg(ERx("conn_name"));
    STprintf(buf, ERx("(i4)%d"), nopts);
    IICGocaCallArg(buf);
    IICGocaCallArg(ERx("opts"));
    STprintf(buf, ERx("(i4)%d"), nwiths);
    IICGocaCallArg(buf);
    IICGocaCallArg(ERx("withkeys"));
    IICGocaCallArg(ERx("withtypes"));
    IICGocaCallArg(ERx("withvals"));
    IICGocaCallArg(ERx("dbms_password"));
    IICGoceCallEnd();
    IICGoseStmtEnd();

    IICGrcsResetCstr();
    IICGobeBlockEnd();

    return;
}

/*{
** Name:	IICGdscDBDisconnect() - Disconnect a DB connection
**
** Description:
**	Builds structures needed by IIARndcNewDBConnection(), which disconnects
**	an established session.
**
** IL statements:
**	DISCONNECT
**	[ TL2ELM VALUE VALUE ]
**	ENDLIST
**
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	30-sep-92 (davel)
**		First written.
**	05-nov-92 (davel)
**		Added support for DISCONNECT ALL.
**	10-nov-92 (davel)
**		Modified so that sess_id is a {nat *} instead of a {nat},
**		so that we can distinguish between no session ID specification
**		and an invalid one.
**	09-mar-93 (davel)
**		Add support for connection name.
*/
VOID
IICGdscDBDisconnect ( stmt )
register IL	*stmt;
{
    register IL	*next;
    char	buf[CGSHORTBUF];

    IICGobbBlockBeg();
    IICGostStructBeg(ERx("i4 *"), ERx("sess_ptr = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("char *"), ERx("conn_name = NULL"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("i4"), ERx("sess_id"), 0, CG_NOINIT);

    next = IICGgilGetIl(stmt);
    /*
    ** Read in the 2-valued list of disconnect options; currently only sess ID
    */
    while ((*next&ILM_MASK) == IL_TL2ELM)
    {
	    char *flag;
	    IL  ref;

	    ref = ILgetOperand( next, 1 );
	    (VOID) IIORcqConstQuery( &IICGirbIrblk, - ref, DB_CHR_TYPE, 
						(PTR *) &flag, (i4 *)NULL );
	    if (STcompare(flag, ERx("ii_sess")) == 0)
	    {
		IICGovaVarAssign(ERx("sess_id"),
			IICGgvlGetVal(ILgetOperand(next,2), DB_INT_TYPE) );
		IICGovaVarAssign(ERx("sess_ptr"),  ERx("&sess_id"));
	    }
	    else if (STcompare(flag, ERx("ii_conn")) == 0)
	    {
		IICGovaVarAssign(ERx("conn_name"), 
			IICGgvlGetVal(ILgetOperand(next,2), DB_CHR_TYPE) );
	    }
	    else if (STcompare(flag, ERx("ii_all")) == 0)
	    {
		STprintf(buf, ERx("%d"), IIsqdisALL);
		IICGovaVarAssign(ERx("sess_id"),  buf);
		IICGovaVarAssign(ERx("sess_ptr"),  ERx("&sess_id"));
	    }
	    next = IICGgilGetIl(next);
    }
    if ((*next&ILM_MASK) != IL_ENDLIST)
        IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

    /* have ABFRT issue the connect */
    IICGosbStmtBeg();
    IICGocbCallBeg(ERx("IIARnddNewDBDisconnect"));
    IICGocaCallArg(ERx("sess_ptr"));
    IICGocaCallArg(ERx("conn_name"));
    IICGoceCallEnd();
    IICGoseStmtEnd();

    IICGrcsResetCstr();
    IICGobeBlockEnd();
    return;
}

/*{
** Name:	IICGchsCheckSession() - Check new DB session.
**
** Description:
**	Calls IIARchsCheckSession(), which checks to see if new session
**	is a valid 4GL session.  This IL is generated after a SET_SQL
**	statement switches the DB session, and since it may switch to a 
**	session initiated in a 3GL procedure, some special ABFRT processing
**	may need to be performed.
**
** IL statements:
**	IL_CHKCONNECT
**
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	10-nov-92 (davel)
**		First written.
**	29-mar-93 (davel)
**		Fix bug 50793 - do not advance the stmt.
*/
VOID
IICGchsCheckSession ( stmt )
register IL	*stmt;
{
    /* have ABFRT connect the new session*/
    IICGosbStmtBeg();
    IICGocbCallBeg(ERx("IIARchsCheckSession"));
    IICGoceCallEnd();
    IICGoseStmtEnd();

    return;
}
