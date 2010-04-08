/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<me.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<eqrun.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abqual.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"itcstr.h"

/* XXX move these to an appropriate header file */
#define MAX_CONNECT_OPTIONS	12
#define MAX_CONNECT_WITHS	12

/**
** Name:	ildb.c -	Execute database-oriented IL statements
**
** Description:
**	Executes database-oriented IL statements.
**
** History:
**	Revision 5.1  86/10/17  16:00:11  arthur
**	Code used in 'beta' release to DATEV.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.4
**	04/22/91 (emerson)
**		Modifications for alerters (in IIOdbDbstmtExec).
**	05/03/91 (emerson)
**		Modifications for alerters: Created new function
**		IIITgeeGetEventExec to handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code).
**	07/26/91 (emerson)
**		Change EVENT to DBEVENT (per LRC 7-3-91).
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added argument to the IIOgvGetVal() calls.
**	29-sep-92 (davel)
**		Added routines for multi-session support.
**	05-nov-92 (davel)
**		Added support for DISCONNECT ALL.
**	10-nov-92 (davel)
**		Added IIITchsCheckSession() to check new session after
**		something like a set_sql statement (or others in the future)
**		switches it.
**	09-mar-93 (davel)
**		Added connection name support to CONNECT and DISCONNECT 
**		statements.
**	29-mar-93 (davel)
**		Fix bug 50793 in IIITchsCheckSession().
**	19-nov-93 (robf)
**              Add dbms password handling
**	2-jul-1996(angusm)
**	Modify IIOdbDbstmtExec() to ignore IL_GETFORM silently
**	instead of generating error. (bug 77352).
**	5-jul-1996 (angusm)
**	Also ignore IL_DOT (declare global temp table), bug 75153
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	IIOdbDbstmtExec	-	Execute DB statements
**
** Description:
**	Executes the major piece of the different database statements.
**	This routines is called by several of the other routines in
**	this file.  It executes the code needed for the body of most
**	database statements.
**
**	There are 2 characteristics of a database statement, whether it
**	is an SQL or QUEL statement and whether it is a repeat statement.
**	However, in all these cases, there is a part of the statement
**	that is handled the same.  This is the part where the text
**	of the statement is generated.  This routines executes the
**	statements that give the text to LIBQ.
**
**	All database statements are handled in a very similar way.  The
**	database statement is decomposed into a list of pieces. Each piece
**	is either a constant, a variable that is used as a string, or a
**	variable whose value is to be passed to the DBMS.  For example,
**	in the statement:
**
**	    append to :mytablevar (c1 = "Column1",
**				   c2 = myvar2,
**				   c3 = myvar3 + myvar2)
**			   where R.y > myvar4;
**
**	    This is broken down into the pieces:
**
**			append to			Name of statement
**			:mytablevar			a variable used as a string
**			" (c1 = \"Column1\", c2 = "	a string constant
**			myvar2				a variable
**			", c3 = "			a string constant
**			myvar3				a variable
**			" + "				a string constant
**			myvar2				a variable
**			") where R.y > "		a string constant
**			myvar4				a variable
**
**	These pieces are reflected in the IL for the statement.
**
**	The job of this routine is to traverse through the list of
**	pieces and make the right LIBQ calls for that piece.  In most
**	cases, IIwritio is called to send the text to the DBMS.  For
**	variables and variables used as strings different calls have
**	to be made.  Also, for repeat queries, variables sent to
**	the DBMS must be preceded by a $N = .  This routine must generate
**	that (N is a monotonically increasting integer, starting at 0 -
**	in other words, the first variable gets $0 =, the second gets $1 = ...)
**
**
** IL statements:
**
**	The Generic DBstatement looks like:
**
**	DB_OP [intRepeated]
**	[IL_QID strvalQueryName intvalQueryId1 intvalQueryId2]
**	{
**		DBStatementPiece
**	}
**	IL_ENDLIST
**
**	DBStatementPiece is one of:
**
**		IL_DBCONST	constStringPiece
**		IL_DBVAL	valVariableUsedAsString
**		IL_DBVAR	valVariableToUseAsValue
**		IL_DBVSTR	valVariableToUseAsStringValue
**		IL_QRYPRED	A qualification function call.
**
**	If the IL opcode is IL_DBSTMT, the actual name of the DB statement
**	to be generated is in the immediately following DBCONST statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**	repeat	{bool}  Whether the statement being executed is a repeat.
**
** History:
**	11-nov-1988 (Joe)
**		Added the repeat parameter and changed to 6.0 call.
**	13-feb-1990 (Joe)
**		Added IL_QID.  JupBug #10015
**	09/90 (jhw) - Added support for IL_DBVAR, bugs #31075, #31342, #31614.
**	04/22/91 (emerson)
**		Modifications for alerters (add IL_DBSTMT).
**	02-jul-91 (johnr)
**		Removed escape character from text string.
**	2-jul-1996(angusm)
**		New OSL rule for OI CREATE TABLE can generate IL_GETFORM
**		tokens if subselect contain params. Silently ignore
**		IL_GETFORM (bug 77352).
**  29-jan-1997(rodjo04) 
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_DOT statements. We must process this statement. 
**  07-mar-1997(rodjo04) 
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_ARRAYREF statements. We must process this statement. 
**  22-may-1997(rodjo04)
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_GETFORM statements. We must process this statement to pick
**      up form variables. 
**  28-may-1998(rodjo04)
**      CREATE TABLE and DECLARE GLOBAL TEMPORARY TABLE can generate
**      IL_GETROW statements. We must process this statement FIRST to 
**      pick up form variables (table field).
**  10-mar-2000 (rodjo04) bug 98775
**       Added support for Float data type.
*/
VOID
IIOdbDbstmtExec ( stmt, repeat )
IL	*stmt;
bool	repeat;
{
	i4	rep_arg_no = 0;
        IL	*next;
        IL      *temp_op;

        temp_op = stmt;
        next = IIOgiGetIl(stmt);

        for (;;) 
        {
            if ( (*next&ILM_MASK) == IL_GETROW)
            {
                IIOgrGetrowExec(next);
                next = IIOgiGetIl(next);
                if ( (*next&ILM_MASK) == IL_TL2ELM)
                {
                    next = IIOgiGetIl(next);
                    if ( (*next&ILM_MASK) == IL_ENDLIST)
                    { 
                       /* don't exit */
                    }
                    else 
                    {
                        IIOerError(ILE_STMT, ILE_ILMISSING,
                        ERx("Endlist in IIOdbDbstmtExec"),(char *) NULL);
                        IIOdeDbErr();		/* suppress BE error message */
                        break;
                    }
                }
                else 
                {
                    IIOerError(ILE_STMT, ILE_ILMISSING,
                    ERx("Endlist in IIOdbDbstmtExec"),(char *) NULL);
                    IIOdeDbErr();		/* suppress BE error message */
                    break;
                }
            }
            else if ( (*next&ILM_MASK) == IL_ENDLIST)      
            {
                break;
            }
            next = IIOgiGetIl(next);
		}
        
    stmt = temp_op;

	if ( (*stmt&ILM_MASK) != IL_DBSTMT )
	{
		IIwritio(0, (i2 *) NULL, TRUE, DB_CHR_TYPE, 0,
			 (PTR) IIILtab[*stmt&ILM_MASK].il_name);
                IIwritio(0, (i2 *) NULL, TRUE, DB_CHR_TYPE, 1, (PTR) ERx(" "));
	}
	stmt = IIOgiGetIl(stmt);
	for (;;)
	{
	    switch (*stmt&ILM_MASK)
	    {
	      case IL_ENDLIST:
		return;

	      case IL_DBVAL:
	      {
		DB_DATA_VALUE	*dbv;

		dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
		 switch (AFE_DATATYPE(dbv))
		 {
		   case DB_INT_TYPE:
		   case DB_FLT_TYPE:
			   IIvarstrio((i2 *)NULL, TRUE, DB_DBV_TYPE, 0, (PTR)dbv);
			   break;
		   default:
		       IIwritio(TRUE, (i2 *) NULL, TRUE, DB_DBV_TYPE, 0, (PTR)dbv);
			   break;
		 }
		break;
	      }

	      case IL_DBVSTR:
		IIvarstrio( (i2 *)NULL, TRUE, DB_DBV_TYPE, 0,
				(PTR)IIOFdgDbdvGet(ILgetOperand(stmt, 1))
		);
		break;

	      case IL_DBCONST:
		IIwritio( 0, (i2 *) NULL, TRUE, DB_CHR_TYPE, 0,
			 (PTR) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL)
		);
		break;

	      case IL_DBVAR:
		if (repeat)
		{
		    /*
		    ** The size of the buffer was figured as follows:
		    ** it will contain " $N="  where N will be a number.
		    ** These means we need 4 bytes (the space, $, = and EOS)
		    ** plus the number of digits in the number.
		    ** Since the number of arguments is likely to be
		    ** less than 99,999,999, allowing 8 digits for N should
		    ** be good.
		    */
		    char	buf[12];

		    _VOID_ STprintf(buf, " $%d=", rep_arg_no++);
		    IIwritio(0, (i2 *) NULL, TRUE, DB_CHR_TYPE, 0, (PTR) buf);
		}
		IILQdtDbvTrim(IIOFdgDbdvGet(ILgetOperand(stmt, 1)));
		break;

	      case IL_QRYPRED:
	      {
		static	ABRT_QFLD	*qflds ZERO_FILL;
		static  i4		qfld_size  ZERO_FILL;
		i4			num_qflds_needed;
		ABRT_QUAL		qual[1];
		ABRT_QUAL		*qual_ptr;
		ABRT_QFLD		*qfld_ptr;
		STATUS			rval;

		STATUS	IIQG_send();
		STATUS	IIITbqsBuildQualStruct();

		num_qflds_needed = (i4) ILgetOperand(stmt, 2);
		if (qfld_size == 0 || qfld_size < num_qflds_needed)
		{
		    /*
		    ** This routine keeps an allocated array of QFLDs
		    ** around.  Note that the QFLDs are used as soon
		    ** as they are built, and qualification functions
		    ** aren't nested.  This means as an application runs,
		    ** the size of this array will eventually hit a high
		    ** point and memory won't be allocated for it.
		    */
		    if (qfld_size != 0)
			MEfree((PTR) qflds);
		    qfld_size = num_qflds_needed;
		    qflds = (ABRT_QFLD *) MEreqmem(0,
					  (u_i2) sizeof(ABRT_QFLD)*qfld_size,
					  FALSE,
					  &rval);
		    if (rval != OK)
		    {
			qfld_size = 0;
			qflds = NULL;
			IIOerError(ILE_STMT, ILE_NOMEM, (char *) NULL);
			IIOdeDbErr();		/* suppress BE error message */
			break;
		    }
		}
		qual_ptr = qual;
		qfld_ptr = qflds;
		if (IIITbqsBuildQualStruct(0,&stmt, &qual_ptr, &qfld_ptr) != OK)
		{
		    IIOerError(ILE_STMT, ILE_NOMEM, (char *) NULL);
		    IIOdeDbErr();
		    break;
		}
		/*
		** When IIITbqsBuildQualStruct returns, the pointer
		** will be on the ENDLIST for the QRYPRED.
		** This is the right place for it to be for the
		** loop to work properly, since the IIOgiGetIL will
		** be done at the bottom of the loop.
		*/
		IIARqualify(qual, IIQG_send, (PTR)NULL);
		break;
	      }

	      case IL_QID:
		/* Simply ignore this. */
		break;
		  case IL_GETFORM:
              {
                  (*(IIILtab[*stmt&ILM_MASK].il_func))(stmt);
		           break;
              }
		  case IL_DOT:
			  {
                  (*(IIILtab[*stmt&ILM_MASK].il_func))(stmt);
		           break;	
			  }	
          case IL_ARRAYREF:
              {
                  (*(IIILtab[*stmt&ILM_MASK].il_func))(stmt);
		           break;	
              }	
          case IL_GETROW:
              {
			    stmt = IIOgiGetIl(stmt);
                if ( (*stmt&ILM_MASK) == IL_TL2ELM)
                {
                    stmt = IIOgiGetIl(stmt);
                    if ( (*stmt&ILM_MASK) == IL_ENDLIST)
                    { 
                        /* don't exit */
                    }
                    else 
                    {
                        IIOerError(ILE_STMT, ILE_ILMISSING,
                        ERx("Endlist in IIOdbDbstmtExec"),(char *) NULL);
                        IIOdeDbErr();		/* suppress BE error message */
                        break;
                    }
                }
                else 
                {
                    IIOerError(ILE_STMT, ILE_ILMISSING,
                    ERx("Endlist in IIOdbDbstmtExec"),(char *) NULL);
                    IIOdeDbErr();		/* suppress BE error message */
                    break;
                }
                break;
              }  	
	      default:
		IIOerError(ILE_STMT, ILE_ILMISSING,
			   ERx("Endlist in IIOdbDbstmtExec"),
			   (char *) NULL);
		IIOdeDbErr();		/* suppress BE error message */
		break;
	    }
	    stmt = IIOgiGetIl(stmt);
	}
}

/*{
** Name:	IIITsdeSqlDbExec - Execute a non-repeatable SQL statement
**
** Description:
**	This executes a non-repeatable SQL Database statement.  This uses
**	the common routine IIOdbDbstmtExec to do most of the work.  The
**	only difference for SQL statements is that a call to IIsqInit must
**	be made first.
**
** Inputs:
**	stmt		The IL statement for this SQL statement.
**
**
** IL Statements:
**
**	For the general form of DB statements see the comment at
**	the top of the file.
**
**	SQLOperator
**	{
**		DBStatementPiece
**	}
**	IL_ENDLIST
**
**	SQLOperator is one of:
**	  IL_COMMIT
**	  IL_CRTINDEX
**	  IL_CRTINTEG
**	  IL_CRTPERMIT
**	  IL_CRTTABLE
**	  IL_CRTUINDEX
**	  IL_CRTVIEW
**	  IL_DROP
**	  IL_DRPINDEX
**	  IL_DRPINTEG
**	  IL_DRPPERMIT
**	  IL_DRPTABLE
**	  IL_DRPVIEW
**	  IL_GRANT
**	  IL_ROLLBACK
**	  IL_SETSQL
**	  IL_SQLMODIFY
**
** History:
**	11-nov-1988 (Joe)
**		First Written.  Patterned after the code generator.
*/
VOID
IIITsdeSqlDbExec(stmt)
IL	*stmt;
{
    IIsqInit((PTR)NULL);
    IIOdbDbstmtExec(stmt, /* repeat = */ FALSE);
    IIsyncup((char *) NULL, 0);
}

/*{
** Name:	IIITqdeQuelDbExec - Execute a non-repeatable QUEL statement
**
** Description:
**	This executes a non-repeatable QUEL Database statement.  This
**	uses the common routine IIdbDbstmtExec to do most of the work.
**
** Inputs:
**	stmt		The IL statment for this QUEL statement.
**
** IL Statements:
**	For the general form of DB statements see the comment at the top
**	of the file.
**
**	QUELOperator
**	{
**		DBStatementPiece
**	}
**	IL_ENDLIST
**
**	QUELOperator is one of:
**		IL_ABORT
**		IL_BEGTRANS
**		IL_COPY
**		IL_CREATE
**		IL_DEFPERM
**		IL_DEFTEMP
**		IL_DESINTEG
**		IL_DESPERM
**		IL_DESTROY
**		IL_DRPPERM
**		IL_DRPTEMP
**		IL_ENDTRANS
**		IL_INDEX
**		IL_INTEGRITY
**		IL_MODIFY
**		IL_PERMIT
**		IL_RANGE
**		IL_RELOCATE
**		IL_RETINTO
**		IL_SAVE
**		IL_SAVEPOINT
**		IL_SET
**		IL_VIEW
**
** History:
**	11-nov-1988 (Joe)
**		First Written
*/
VOID
IIITqdeQuelDbExec(stmt)
IL	*stmt;
{
    IIOdbDbstmtExec(stmt, /* repeat = */ FALSE);
    IIsyncup((char *) NULL, 0);
}

/*{
** Name:	IIITrqeRepeatQueryExec  - Execute a repeat query.
**
** Description:
**	This routine executes all repeat queries.  A parameter tells
**	whether the query is QUEL or SQL.  In either case, more
**	or less the same code is generated.
**
** IL Statements:
**
**	repeat_op	boolIsARepeat
**    	[IL_QID strvalQueryName intvalQueryId1 intvalQueryId2]
**	{
**	    DBstatementPiece
**	}
**	IL_ENDLIST
**
**    	IL_QID was added later than the repeat statements.  This means
**    	that it is possible to have a statement with the flag set,
**    	but without an IL_QID.  In this case it is a repeated query
**    	that has not been recompiled since IL_QID was added.  Since
**    	it is not possible to generate a unique query id at runtime, these
**    	old statements are not run repeated.
**
** Inputs:
**	stmt		The repeat query to execute.
**
**	sql		Whether it is an SQL query.
**
** History:
**	11-nov-1988 (Joe)
**		First Written
**	01/90 (jhw) -- Modified to use longs for query ID.  JupBug #7899.
**    	13-feb-1990 (Joe)
**	    Added IL_QID and code to use it for query id. JupBug #10015
*/
VOID
IIITrqeRepeatQueryExec(stmt, sql)
IL	*stmt;
bool	sql;
{
    IL		*next;
    char	*qid1;
    i4		qid2;
    i4		qid3;

    next = IIOgiGetIl(stmt);
    /*
    ** This checks the value of the repeat flag.  If it is FALSE,
    ** or if the next statement is not an IL_QID then run as
    ** non-repeat.  The test for IL_QID makes old repeat queries
    ** run non-repeated to avoid a bug with a non-unique query id.
    */
    if (!ILgetOperand(stmt, 1) || (*next&ILM_MASK) != IL_QID)
    {
	if (sql)
	    IIsqInit((PTR)NULL);
	IIOdbDbstmtExec(stmt, /* repeat = */ FALSE);
	IIsyncup((char *) NULL, 0);
    }
    else
    {
	/*
	** If we got here, then the next points to a IL_QID.  Use
	** the values it it for the query id.
	*/
	qid1 = (char *) IIOgvGetVal(ILgetOperand(next, 1), 
					DB_CHR_TYPE, (i4 *)NULL);
	qid2 = (i4) *((i4 *) IIOgvGetVal(ILgetOperand(next, 2), 
					DB_INT_TYPE, (i4 *)NULL));
	qid3 = (i4) *((i4 *) IIOgvGetVal(ILgetOperand(next, 3), 
					DB_INT_TYPE, (i4 *)NULL));

	IIsexec();
	/*
	** Note, this is a loop.  The reason is that the code works
	** as if the repeat query is already known to the DBMS, so
	** when it first comes it it tries to execute the query.  If
	** the DBMS does not know about the query, it defines it at
	** the bottom of the loop and then loops which causes it
	** to execute again.
	** Also note, stmt is not used to iterate through the IL
	** statements since it must be kept to mark the beginning
	** of the repeat statement for the define and the execution.
	*/
	while (IInexec() == 0)
	{
	    if (sql)
		IIsqInit((PTR)NULL);
	    IIexExec(1, qid1, qid2, qid3);
	    next = IIOgiGetIl(stmt);
	    /*
	    ** Note, this loop depends on the fact that repeat querys
	    ** can not have a qualification function, and thus, they
	    ** can not have an IL_QRYPRED in them.  Thus, there
	    ** will be only one ENDLIST for the query.
	    */
	    while ((*next&ILM_MASK) != IL_ENDLIST)
	    {
		if ((*next&ILM_MASK) == IL_DBVAR)
		{
		    IILQdtDbvTrim(IIOFdgDbdvGet(ILgetOperand(next, 1)));
		}
		next = IIOgiGetIl(next);
	    }
	    IIsyncup((char *) NULL, 0);
	    if (IInexec() == 0)
	    {
		if (sql)
		    IIsqInit((PTR)NULL);
		IIexDefine(1, qid1, qid2, qid3);
		IIOdbDbstmtExec(stmt, TRUE);
		IIexDefine(0, qid1, qid2, qid3);
		if (IIerrtest() == 0)
		    IIsexec();
	    }
	}
    }
}

/*{
** Name:	IIITqreQuelRepeatExec - Execute a QUEL repeat query.
**
** Description:
**	A QUEL repeat query comes to here.  This routine simply
**	call the IIITrqeRepeatQueryExec routine that does the
**	real work.  This simply notes the that language is QUEL.
**
** Inputs:
**	stmt		The IL statement for this query.
**
** History:
**	18-nov-1988 (Joe)
**		First Written
*/
IIITqreQuelRepeatExec(stmt)
IL	*stmt;
{
    IIITrqeRepeatQueryExec(stmt, /* sql = */ FALSE);
}

/*{
** Name:	IIITsreSqlRepeatExec - Execute an SQL repeat query.
**
** Description:
**	An SQL repeat query comes to here.  This routine simply
**	call the IIITrqeRepeatQueryExec routine that does the
**	real work.  This simply notes the that language is SQL.
**
** Inputs:
**	stmt		The IL statement for this query.
**
** History:
**	18-nov-1988 (Joe)
**		First Written
*/
IIITsreSqlRepeatExec(stmt)
IL	*stmt;
{
    IIITrqeRepeatQueryExec(stmt, /* sql = */ TRUE);
}

/*{
** Name:	IIITgeeGetEventExec - Execute a Get Dbevent
**
** Description:
**	A GETEVENT statement is interpreted here.  The format is:
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
VOID IILQesEvStat( );

i4
IIITgeeGetEventExec( stmt )
IL	*stmt;
{
	int	secs;

	secs = IIITvtnValToNat( ILgetOperand( stmt, 1 ), 0,
				ERx("GET DBEVENT") );
	IILQesEvStat( 0, secs );
	return 0;
}

/*{
** Name:	IIITconDBConnect() - Establish new database connection
**
** Description:
**
** IL statements:
**	IL_CONNECT databasename
**          {
**	         IL_TL2ELM  optionname optionvalue
**          }
**	    IL_ENDLIST
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	29-sep-92 (davel)
**		Written.
**	10-nov-92 (davel)
**		Change session ID to be a {nat *} instead of {nat}.
**	09-mar-93 (davel)
**		Added connection name support.
**	31-mar-93 (davel)
**		Fix bug 50850 - don't use IIITtcsToCStr() to get the connection
**		name, as this will trim trailing blanks if the name is in
**		a char variable. Use IIOgvGetVal with a type of DB_CHR_TYPE
**		instead.
*/
VOID
IIITconDBConnect ( stmt )
register IL	*stmt;
{
	char		*flag = NULL;
	char		*dbname = NULL;
	char		*username = NULL;
	char		*conn_name = NULL;
	i4		*sess_ptr = NULL;
	char		*dbms_password = NULL;
	i4		sess_id;
	char		*opts[MAX_CONNECT_OPTIONS] ZERO_FILL;
	char		*withkeys[MAX_CONNECT_WITHS];
	i4		withtypes[MAX_CONNECT_WITHS];
	char		*withvals[MAX_CONNECT_WITHS];
	i4		nopts = 0;
	i4		nwiths = 0;
	IL		ilarg;
	i4 		i;

	VOID IIARndcNewDBConnect();

	dbname = IIITtcsToCStr(ILgetOperand(stmt, 1));

	stmt = IIOgiGetIl(stmt);

	while ((*stmt&ILM_MASK) == IL_TL2ELM)
	{
		flag = (char *) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL);
		if (STcompare(flag, ERx("ii_user")) == 0)
		{
			username = IIITtcsToCStr(ILgetOperand(stmt, 2));
		}
		else if (STcompare(flag, ERx("ii_conn")) == 0)
		{
			conn_name = (char *) IIOgvGetVal(ILgetOperand(stmt, 2), 
						DB_CHR_TYPE, (i4 *)NULL);
		}
		else if (STcompare(flag, ERx("ii_dbms_password")) == 0)
		{
			dbms_password= IIITtcsToCStr(ILgetOperand(stmt, 2));
		}
		else if (STcompare(flag, ERx("ii_sess")) == 0)
		{
			sess_id = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2),
					(i4) 0, 
					ERx("CONNECT session ID"));
			sess_ptr = &sess_id;
		}
		else if (STcompare(flag, ERx("ii_flag")) == 0)
		{
			if (nopts >= MAX_CONNECT_OPTIONS)
			{
			    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
						(char *) NULL);
			    return;
			}
			opts[nopts++] = IIITtcsToCStr(ILgetOperand(stmt, 2));
		}
		else
		{
			DB_DATA_VALUE 	*dbv;
			DB_DT_ID	type;

			if (nwiths >= MAX_CONNECT_WITHS)
			{
			    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
						(char *) NULL);
			    return;
			}

			ilarg = ILgetOperand(stmt, 2);
			dbv = IIOFdgDbdvGet(ilarg);
			type = AFE_DATATYPE(dbv);
			if (AFE_NULLABLE(dbv->db_datatype) && AFE_ISNULL(dbv) )
			{
			    /* XXX: for now just silently skip... */
			    ;
			}
			else
			{
			    withkeys[nwiths] = flag;
			    withtypes[nwiths] = type;
			    withvals[nwiths++] = (char *) IIOgvGetVal(ilarg, 
						type, NULL);
			}
		}
		stmt = IIOgiGetIl(stmt);
	}
	if ((*stmt&ILM_MASK) != IL_ENDLIST )
	{
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		return;
	}
	/* Have ABFRT issue the connect statement */
	IIARndcNewDBConnect(dbname, username, sess_ptr, conn_name, 
		nopts, opts, nwiths, withkeys, withtypes, withvals,
		dbms_password);

	/* reset any saved strings */
	IIITrcsResetCStr();

	return;
}

/*{
** Name:	IIITdscDBDisconnect() - Disconnect database connection
**
** Description:
**
** IL statements:
**	IL_DISCONNECT
**          {
**	         IL_TL2ELM  optionname optionvalue
**          }
**	    IL_ENDLIST
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	30-sep-92 (davel)
**		Written.
**	05-nov-92 (davel)
**		Added support for DISCONNECT ALL.
**	10-nov-92 (davel)
**		Change session ID to be a {nat *} instead of {nat}.
**	09-mar-93 (davel)
**		Added connection name support.
**	31-mar-93 (davel)
**		Fix bug 50850 - don't use IIITtcsToCStr() to get the connection
**		name, as this will trim trailing blanks if the name is in
**		a char variable. Use IIOgvGetVal with a type of DB_CHR_TYPE
**		instead.
*/
VOID
IIITdscDBDisconnect ( stmt )
register IL	*stmt;
{
	char		*flag;
	char		*conn_name = NULL;
	i4		*sess_ptr = NULL;
	i4		sess_id;

	stmt = IIOgiGetIl(stmt);

	while ((*stmt&ILM_MASK) == IL_TL2ELM)
	{
		flag = (char *) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL);
		if (STcompare(flag, ERx("ii_sess")) == 0)
		{
			sess_id = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2),
					(i4) 0, 
					ERx("DISCONNECT session ID"));
			sess_ptr = &sess_id;
		}
		else if (STcompare(flag, ERx("ii_conn")) == 0)
		{
			conn_name = (char *) IIOgvGetVal(ILgetOperand(stmt, 2), 
						DB_CHR_TYPE, (i4 *)NULL);
		}
		else if (STcompare(flag, ERx("ii_all")) == 0)
		{
			sess_id = (i4) IIsqdisALL;
			sess_ptr = &sess_id;
		}
		else
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
						(char *) NULL);
			return;

		}
		stmt = IIOgiGetIl(stmt);
	}
	if ((*stmt&ILM_MASK) != IL_ENDLIST )
	{
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		return;
	}
	/* Have ABFRT issue the disconnect statement */
	IIARnddNewDBDisconnect(sess_ptr, conn_name);

	return;
}

/*{
** Name:	IIITchsCheckSession() - Check new DB session.
**
** Description:
**
** IL statements:
**	IL_CHKCONNECT
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	10-nov-92 (davel)
**		Written.
**	29-mar-93 (davel)
**		Fix bug 50793 - do not advance IL stmt.
*/
VOID
IIITchsCheckSession ( stmt )
register IL	*stmt;
{
	IIARchsCheckSession();
	return;
}
